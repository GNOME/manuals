/*
 * manuals-tab.c
 *
 * Copyright 2024 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"

#include <libdex.h>
#include <webkit/webkit.h>

#include <glib/gi18n.h>

#include "manuals-heading.h"
#include "manuals-keyword.h"
#include "manuals-tab.h"
#include "manuals-utils.h"
#include "manuals-repository.h"
#include "manuals-window.h"

struct _ManualsTab
{
  GtkWidget           parent_instance;

  ManualsNavigatable *navigatable;

  WebKitWebView      *web_view;
};

G_DEFINE_FINAL_TYPE (ManualsTab, manuals_tab, GTK_TYPE_WIDGET)

enum {
  PROP_0,
  PROP_CAN_GO_BACK,
  PROP_CAN_GO_FORWARD,
  PROP_ICON,
  PROP_LOADING,
  PROP_NAVIGATABLE,
  PROP_TITLE,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];
static const char style_sheet_css[] =
  "#main { box-shadow: none !important; }\n"
  ".devhelp-hidden { display: none; }\n"
  ".toc { background: transparent !important; }\n"
  ":root { --body-bg: #ffffff !important; }\n"
  "@media (prefers-color-scheme: dark) {\n"
  "  :root { --body-bg: #1e1e1e !important; }\n"
  "}\n"
  ;

static void
manuals_tab_web_view_notify_is_loading_cb (ManualsTab *self)
{
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_LOADING]);
}

static void
manuals_tab_web_view_notify_title_cb (ManualsTab *self)
{
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TITLE]);
}

static void
manuals_tab_web_view_notify_favicon_cb (ManualsTab *self)
{
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ICON]);
}

static void
manuals_tab_back_forward_list_changed_cb (ManualsTab *self)
{
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CAN_GO_BACK]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_CAN_GO_FORWARD]);
}

static ManualsWindow *
manuals_tab_get_window (ManualsTab *self)
{
  g_assert (MANUALS_IS_TAB (self));

  return MANUALS_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (self), MANUALS_TYPE_WINDOW));
}

typedef struct _DecidePolicy
{
  ManualsTab               *self;
  WebKitPolicyDecision     *decision;
  WebKitPolicyDecisionType  decision_type;
} DecidePolicy;

static void
decide_policy_free (DecidePolicy *state)
{
  g_clear_object (&state->self);
  g_clear_object (&state->decision);
  g_free (state);
}

static DexFuture *
manuals_tab_decide_policy_fiber (gpointer user_data)
{
  WebKitNavigationPolicyDecision *navigation_decision;
  WebKitNavigationAction *navigation_action;
  g_autoptr(GObject) resource = NULL;
  g_auto(GValue) uri_value = G_VALUE_INIT;
  ManualsRepository *repository;
  ManualsWindow *window;
  DecidePolicy *state = user_data;
  const char *uri;
  gboolean open_new_tab = FALSE;
  int button;
  int modifiers;

  g_assert (state != NULL);
  g_assert (MANUALS_IS_TAB (state->self));
  g_assert (WEBKIT_IS_NAVIGATION_POLICY_DECISION (state->decision));

  if (!(window = manuals_tab_get_window (state->self)))
    goto ignore;

  repository = manuals_window_get_repository (window);

  navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION (state->decision);
  navigation_action = webkit_navigation_policy_decision_get_navigation_action (navigation_decision);
  uri = webkit_uri_request_get_uri (webkit_navigation_action_get_request (navigation_action));

  /* middle click or ctrl-click -> new tab */
  button = webkit_navigation_action_get_mouse_button (navigation_action);
  modifiers = webkit_navigation_action_get_modifiers (navigation_action);
  open_new_tab = (button == 2 || (button == 1 && modifiers == GDK_CONTROL_MASK));

  /* Pass-through API requested things */
  if (button == 0 && modifiers == 0)
    {
      webkit_policy_decision_use (state->decision);
      return dex_future_new_for_boolean (TRUE);
    }

  if (g_str_equal (uri, "about:blank"))
    {
      manuals_window_add_tab (window, manuals_tab_new ());
      goto ignore;
    }

  if (g_strcmp0 ("file", g_uri_peek_scheme (uri)) != 0)
    {
      g_autoptr(GtkUriLauncher) launcher = gtk_uri_launcher_new (uri);
      gtk_uri_launcher_launch (launcher, GTK_WINDOW (window), NULL, NULL, NULL);
      goto ignore;
    }

  if ((resource = dex_await_object (manuals_heading_find_by_uri (repository, uri), NULL)) ||
      (resource = dex_await_object (manuals_keyword_find_by_uri (repository, uri), NULL)))
    {
      g_autoptr(ManualsNavigatable) navigatable = NULL;
      ManualsTab *tab = manuals_window_get_visible_tab (window);

      if (open_new_tab)
        {
          tab = manuals_tab_duplicate (tab);
          manuals_window_add_tab (window, tab);
        }

      navigatable = manuals_navigatable_new_for_resource (resource);
      manuals_tab_set_navigatable (tab, navigatable);

      goto ignore;
    }

  webkit_policy_decision_use (state->decision);

  return dex_future_new_for_boolean (TRUE);

ignore:
  webkit_policy_decision_ignore (state->decision);

  return dex_future_new_for_boolean (TRUE);
}

static gboolean
manuals_tab_web_view_decide_policy_cb (ManualsTab               *self,
                                       WebKitPolicyDecision     *decision,
                                       WebKitPolicyDecisionType  decision_type,
                                       WebKitWebView            *web_view)
{
  DecidePolicy *state;

  g_assert (MANUALS_IS_TAB (self));
  g_assert (WEBKIT_IS_POLICY_DECISION (decision));
  g_assert (WEBKIT_IS_WEB_VIEW (web_view));

  if (decision_type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION)
    return GDK_EVENT_PROPAGATE;

  state = g_new0 (DecidePolicy, 1);
  state->self = g_object_ref (self);
  state->decision = g_object_ref (decision);
  state->decision_type = decision_type;

  dex_future_disown (dex_scheduler_spawn (NULL, 0,
                                          manuals_tab_decide_policy_fiber,
                                          state,
                                          (GDestroyNotify)decide_policy_free));

  return GDK_EVENT_STOP;
}

static void
manuals_tab_constructed (GObject *object)
{
  ManualsTab *self = (ManualsTab *)object;
  g_autoptr(WebKitUserStyleSheet) style_sheet = NULL;
  WebKitUserContentManager *ucm;
  WebKitWebsiteDataManager *manager;
  WebKitNetworkSession *session;
  WebKitSettings *webkit_settings;

  G_OBJECT_CLASS (manuals_tab_parent_class)->constructed (object);

  webkit_settings = webkit_web_view_get_settings (self->web_view);
  webkit_settings_set_enable_back_forward_navigation_gestures (webkit_settings, TRUE);
  webkit_settings_set_enable_html5_database (webkit_settings, FALSE);
  webkit_settings_set_enable_html5_local_storage (webkit_settings, FALSE);
  webkit_settings_set_user_agent_with_application_details (webkit_settings, "GNOME-Manuals", PACKAGE_VERSION);

  style_sheet = webkit_user_style_sheet_new (style_sheet_css,
                                             WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                                             WEBKIT_USER_STYLE_LEVEL_USER,
                                             NULL, NULL);
  ucm = webkit_web_view_get_user_content_manager (self->web_view);
  webkit_user_content_manager_add_style_sheet (ucm, style_sheet);

  session = webkit_web_view_get_network_session (self->web_view);
  manager = webkit_network_session_get_website_data_manager (session);
  webkit_website_data_manager_set_favicons_enabled (manager, TRUE);
}

static void
manuals_tab_css_changed (GtkWidget         *widget,
                         GtkCssStyleChange *change)
{
  ManualsTab *self = (ManualsTab *)widget;
  GdkRGBA background;

  g_assert (GTK_IS_WIDGET (widget));

  if (adw_style_manager_get_dark (adw_style_manager_get_default ()))
    gdk_rgba_parse (&background, "#1e1e1e");
  else
    gdk_rgba_parse (&background, "#ffffff");

  webkit_web_view_set_background_color (self->web_view, &background);
}

static void
manuals_tab_dispose (GObject *object)
{
  ManualsTab *self = (ManualsTab *)object;
  GtkWidget *child;

  gtk_widget_dispose_template (GTK_WIDGET (self), MANUALS_TYPE_TAB);

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self))))
    gtk_widget_unparent (child);

  g_clear_object (&self->navigatable);

  G_OBJECT_CLASS (manuals_tab_parent_class)->dispose (object);
}

static void
manuals_tab_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  ManualsTab *self = MANUALS_TAB (object);

  switch (prop_id)
    {
    case PROP_CAN_GO_BACK:
      g_value_set_boolean (value, manuals_tab_can_go_back (self));
      break;

    case PROP_CAN_GO_FORWARD:
      g_value_set_boolean (value, manuals_tab_can_go_forward (self));
      break;

    case PROP_ICON:
      g_value_take_object (value, manuals_tab_dup_icon (self));
      break;

    case PROP_LOADING:
      g_value_set_boolean (value, manuals_tab_get_loading (self));
      break;

    case PROP_NAVIGATABLE:
      g_value_set_object (value, manuals_tab_get_navigatable (self));
      break;

    case PROP_TITLE:
      g_value_set_string (value, manuals_tab_get_title (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
manuals_tab_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  ManualsTab *self = MANUALS_TAB (object);

  switch (prop_id)
    {
    case PROP_NAVIGATABLE:
      manuals_tab_set_navigatable (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
manuals_tab_class_init (ManualsTabClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = manuals_tab_constructed;
  object_class->dispose = manuals_tab_dispose;
  object_class->get_property = manuals_tab_get_property;
  object_class->set_property = manuals_tab_set_property;

  widget_class->css_changed = manuals_tab_css_changed;

  properties[PROP_CAN_GO_BACK] =
    g_param_spec_boolean ("can-go-back", NULL, NULL,
                          FALSE,
                          (G_PARAM_READABLE |
                           G_PARAM_STATIC_STRINGS));

  properties[PROP_CAN_GO_FORWARD] =
    g_param_spec_boolean ("can-go-forward", NULL, NULL,
                          FALSE,
                          (G_PARAM_READABLE |
                           G_PARAM_STATIC_STRINGS));

  properties[PROP_ICON] =
    g_param_spec_object ("icon", NULL, NULL,
                         G_TYPE_ICON,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_LOADING] =
    g_param_spec_boolean ("loading", NULL, NULL,
                          FALSE,
                          (G_PARAM_READABLE |
                           G_PARAM_STATIC_STRINGS));

  properties[PROP_NAVIGATABLE] =
    g_param_spec_object ("navigatable", NULL, NULL,
                         MANUALS_TYPE_NAVIGATABLE,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_TITLE] =
    g_param_spec_string ("title", NULL, NULL,
                         NULL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/app/devsuite/Manuals/manuals-tab.ui");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_bind_template_child (widget_class, ManualsTab, web_view);

  g_type_ensure (WEBKIT_TYPE_WEB_VIEW);
}

static void
manuals_tab_init (ManualsTab *self)
{
  WebKitBackForwardList *back_forward_list;

  gtk_widget_init_template (GTK_WIDGET (self));

  back_forward_list = webkit_web_view_get_back_forward_list (self->web_view);

  g_signal_connect_object (self->web_view,
                           "decide-policy",
                           G_CALLBACK (manuals_tab_web_view_decide_policy_cb),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->web_view,
                           "notify::is-loading",
                           G_CALLBACK (manuals_tab_web_view_notify_is_loading_cb),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->web_view,
                           "notify::favicon",
                           G_CALLBACK (manuals_tab_web_view_notify_favicon_cb),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->web_view,
                           "notify::title",
                           G_CALLBACK (manuals_tab_web_view_notify_title_cb),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (back_forward_list,
                           "changed",
                           G_CALLBACK (manuals_tab_back_forward_list_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);
}

ManualsTab *
manuals_tab_new (void)
{
  return g_object_new (MANUALS_TYPE_TAB, NULL);
}

ManualsTab *
manuals_tab_duplicate (ManualsTab *self)
{
  ManualsTab *copy;

  g_return_val_if_fail (!self || MANUALS_IS_TAB (self), NULL);

  copy = g_object_new (MANUALS_TYPE_TAB, NULL);

  if (self != NULL)
    {
      /* TODO: Maybe copy backforward list here? */
      manuals_tab_set_navigatable (copy, self->navigatable);
    }

  return copy;
}

const char *
manuals_tab_get_title (ManualsTab *self)
{
  const char *title;

  g_return_val_if_fail (MANUALS_IS_TAB (self), NULL);

  title = webkit_web_view_get_title (self->web_view);

  if (_g_str_empty0 (title) && self->navigatable != NULL)
    title = manuals_navigatable_get_title (self->navigatable);

  if (_g_str_empty0 (title))
    title = _("Empty Page");

  return title;
}


GIcon *
manuals_tab_dup_icon (ManualsTab *self)
{
  GdkTexture *texture;

  g_return_val_if_fail (MANUALS_IS_TAB (self), NULL);

  if ((texture = webkit_web_view_get_favicon (self->web_view)))
    return g_object_ref (G_ICON (texture));

  return NULL;
}

gboolean
manuals_tab_get_loading (ManualsTab *self)
{
  g_return_val_if_fail (MANUALS_IS_TAB (self), FALSE);

  return webkit_web_view_is_loading (self->web_view);
}

gboolean
manuals_tab_can_go_back (ManualsTab *self)
{
  g_return_val_if_fail (MANUALS_IS_TAB (self), FALSE);

  return webkit_web_view_can_go_back (self->web_view);
}

gboolean
manuals_tab_can_go_forward (ManualsTab *self)
{
  g_return_val_if_fail (MANUALS_IS_TAB (self), FALSE);

  return webkit_web_view_can_go_forward (self->web_view);
}

void
manuals_tab_go_back (ManualsTab *self)
{
  g_return_if_fail (MANUALS_IS_TAB (self));

  webkit_web_view_go_back (self->web_view);
}

void
manuals_tab_go_forward (ManualsTab *self)
{
  g_return_if_fail (MANUALS_IS_TAB (self));

  webkit_web_view_go_forward (self->web_view);
}

ManualsNavigatable *
manuals_tab_get_navigatable (ManualsTab *self)
{
  g_return_val_if_fail (MANUALS_IS_TAB (self), NULL);

  return self->navigatable;
}

void
manuals_tab_set_navigatable (ManualsTab         *self,
                             ManualsNavigatable *navigatable)
{
  g_return_if_fail (MANUALS_IS_TAB (self));
  g_return_if_fail (!navigatable || MANUALS_IS_NAVIGATABLE (navigatable));

  if (g_set_object (&self->navigatable, navigatable))
    {
      const char *uri = NULL;

      if (navigatable != NULL)
        uri = manuals_navigatable_get_uri (navigatable);

      if (uri != NULL)
        webkit_web_view_load_uri (self->web_view, uri);

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_NAVIGATABLE]);
    }
}
