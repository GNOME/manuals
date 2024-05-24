/*
 * manuals-window.c
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

#include <glib/gi18n.h>

#include "manuals-application.h"
#include "manuals-book.h"
#include "manuals-path-bar.h"
#include "manuals-sdk.h"
#include "manuals-sidebar.h"
#include "manuals-tab.h"
#include "manuals-window.h"

struct _ManualsWindow
{
  PanelWorkspace        parent_instance;

  ManualsRepository    *repository;
  GSignalGroup         *visible_tab_signals;

  PanelDock            *dock;
  ManualsSidebar       *sidebar;
  AdwTabView           *tab_view;

  guint                 disposed : 1;
};

G_DEFINE_FINAL_TYPE (ManualsWindow, manuals_window, PANEL_TYPE_WORKSPACE)

enum {
  PROP_0,
  PROP_REPOSITORY,
  PROP_VISIBLE_TAB,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

ManualsWindow *
manuals_window_new (void)
{
  return g_object_new (MANUALS_TYPE_WINDOW, NULL);
}

static AdwTabPage *
manuals_window_add_tab_internal (ManualsWindow *self,
                                 ManualsTab    *tab)
{
  AdwTabPage *page;

  g_assert (MANUALS_IS_WINDOW (self));
  g_assert (MANUALS_IS_TAB (tab));

  page = adw_tab_view_add_page (self->tab_view, GTK_WIDGET (tab), NULL);

  g_object_bind_property (tab, "title", page, "title", G_BINDING_SYNC_CREATE);
  g_object_bind_property (tab, "icon", page, "icon", G_BINDING_SYNC_CREATE);
  g_object_bind_property (tab, "loading", page, "loading", G_BINDING_SYNC_CREATE);

  return page;
}

static void
manuals_window_update_actions (ManualsWindow *self)
{
  ManualsTab *tab;
  gboolean can_go_forward = FALSE;
  gboolean can_go_back = FALSE;

  g_assert (MANUALS_IS_WINDOW (self));

  if (self->disposed)
    return;

  if ((tab = manuals_window_get_visible_tab (self)))
    {
      can_go_back = manuals_tab_can_go_back (tab);
      can_go_forward = manuals_tab_can_go_forward (tab);
    }

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "tab.go-back", can_go_back);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "tab.go-forward", can_go_forward);
}

static void
manuals_window_tab_view_notify_selected_page_cb (ManualsWindow *self,
                                                 GParamSpec    *pspec,
                                                 AdwTabView    *tab_view)
{
  ManualsTab *tab;

  g_assert (MANUALS_IS_WINDOW (self));
  g_assert (ADW_IS_TAB_VIEW (tab_view));

  tab = manuals_window_get_visible_tab (self);
  g_signal_group_set_target (self->visible_tab_signals, tab);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VISIBLE_TAB]);
}

static void
manuals_window_tab_go_back_action (GtkWidget  *widget,
                                   const char *action_name,
                                   GVariant   *param)
{
  manuals_tab_go_back (manuals_window_get_visible_tab (MANUALS_WINDOW (widget)));
}

static void
manuals_window_tab_go_forward_action (GtkWidget  *widget,
                                      const char *action_name,
                                      GVariant   *param)
{
  manuals_tab_go_forward (manuals_window_get_visible_tab (MANUALS_WINDOW (widget)));
}

static void
manuals_window_tab_new_action (GtkWidget  *widget,
                               const char *action_name,
                               GVariant   *param)
{
  ManualsWindow *self = MANUALS_WINDOW (widget);
  ManualsTab *tab;

  g_assert (MANUALS_IS_WINDOW (self));

  tab = manuals_window_get_visible_tab (self);
  tab = manuals_tab_duplicate (tab);

  manuals_window_add_tab (self, tab);
  manuals_window_set_visible_tab (self, tab);
}

static void
manuals_window_tab_close_action (GtkWidget  *widget,
                                 const char *action_name,
                                 GVariant   *param)
{
  ManualsWindow *self = MANUALS_WINDOW (widget);
  ManualsTab *tab;
  AdwTabPage *page;

  g_assert (MANUALS_IS_WINDOW (self));

  if ((tab = manuals_window_get_visible_tab (self)) &&
      (page = adw_tab_view_get_page (self->tab_view, GTK_WIDGET (tab))))
    adw_tab_view_close_page (self->tab_view, page);

  if (NULL == manuals_window_get_visible_tab (self))
    gtk_window_destroy (GTK_WINDOW (self));
}

static void
manuals_window_sidebar_focus_search_action (GtkWidget  *widget,
                                            const char *action_name,
                                            GVariant   *param)
{
  ManualsWindow *self = MANUALS_WINDOW (widget);

  g_assert (MANUALS_IS_WINDOW (self));

  panel_dock_set_reveal_start (self->dock, TRUE);
  manuals_sidebar_focus_search (self->sidebar);
}

static void
manuals_window_constructed (GObject *object)
{
  ManualsWindow *self = (ManualsWindow *)object;

  G_OBJECT_CLASS (manuals_window_parent_class)->constructed (object);

#if 0
  /* For some reason this causes librsvg to segfault */
  gtk_widget_add_css_class (GTK_WIDGET (object), "devel");
#endif

  manuals_sidebar_set_repository (self->sidebar, self->repository);
  manuals_sidebar_focus_search (self->sidebar);
}

static void
manuals_window_dispose (GObject *object)
{
  ManualsWindow *self = (ManualsWindow *)object;

  self->disposed = TRUE;

  gtk_widget_dispose_template (GTK_WIDGET (self), MANUALS_TYPE_WINDOW);

  g_clear_object (&self->repository);

  G_OBJECT_CLASS (manuals_window_parent_class)->dispose (object);
}

static void
manuals_window_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  ManualsWindow *self = MANUALS_WINDOW (object);

  switch (prop_id)
    {
    case PROP_REPOSITORY:
      g_value_set_object (value, manuals_window_get_repository (self));
      break;

    case PROP_VISIBLE_TAB:
      g_value_set_object (value, manuals_window_get_visible_tab (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
manuals_window_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  ManualsWindow *self = MANUALS_WINDOW (object);

  switch (prop_id)
    {
    case PROP_REPOSITORY:
      self->repository = g_value_dup_object (value);
      break;

    case PROP_VISIBLE_TAB:
      manuals_window_set_visible_tab (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
manuals_window_class_init (ManualsWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = manuals_window_constructed;
  object_class->dispose = manuals_window_dispose;
  object_class->get_property = manuals_window_get_property;
  object_class->set_property = manuals_window_set_property;

  properties[PROP_REPOSITORY] =
    g_param_spec_object ("repository", NULL, NULL,
                         MANUALS_TYPE_REPOSITORY,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_VISIBLE_TAB] =
    g_param_spec_object ("visible-tab", NULL, NULL,
                         MANUALS_TYPE_TAB,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/app/devsuite/Manuals/manuals-window.ui");

  gtk_widget_class_bind_template_child (widget_class, ManualsWindow, dock);
  gtk_widget_class_bind_template_child (widget_class, ManualsWindow, sidebar);
  gtk_widget_class_bind_template_child (widget_class, ManualsWindow, tab_view);

  gtk_widget_class_install_action (widget_class, "sidebar.focus-search", NULL, manuals_window_sidebar_focus_search_action);
  gtk_widget_class_install_action (widget_class, "tab.go-back", NULL, manuals_window_tab_go_back_action);
  gtk_widget_class_install_action (widget_class, "tab.go-forward", NULL, manuals_window_tab_go_forward_action);
  gtk_widget_class_install_action (widget_class, "tab.close", NULL, manuals_window_tab_close_action);
  gtk_widget_class_install_action (widget_class, "tab.new", NULL, manuals_window_tab_new_action);

  g_type_ensure (MANUALS_TYPE_PATH_BAR);
  g_type_ensure (MANUALS_TYPE_SIDEBAR);
  g_type_ensure (MANUALS_TYPE_TAB);
}

static void
manuals_window_init (ManualsWindow *self)
{
  gtk_window_set_title (GTK_WINDOW (self), _("Manuals"));

  self->visible_tab_signals = g_signal_group_new (MANUALS_TYPE_TAB);
  g_signal_connect_object (self->visible_tab_signals,
                           "bind",
                           G_CALLBACK (manuals_window_update_actions),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->visible_tab_signals,
                           "unbind",
                           G_CALLBACK (manuals_window_update_actions),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_group_connect_object (self->visible_tab_signals,
                                 "notify::can-go-back",
                                 G_CALLBACK (manuals_window_update_actions),
                                 self,
                                 G_CONNECT_SWAPPED);
  g_signal_group_connect_object (self->visible_tab_signals,
                                 "notify::can-go-forward",
                                 G_CALLBACK (manuals_window_update_actions),
                                 self,
                                 G_CONNECT_SWAPPED);

  gtk_widget_action_set_enabled (GTK_WIDGET (self), "tab.go-back", FALSE);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "tab.go-forward", FALSE);

  gtk_widget_init_template (GTK_WIDGET (self));

  g_signal_connect_object (self->tab_view,
                           "notify::selected-page",
                           G_CALLBACK (manuals_window_tab_view_notify_selected_page_cb),
                           self,
                           G_CONNECT_SWAPPED);
  manuals_window_add_tab (self, manuals_tab_new ());
}

void
manuals_window_add_tab (ManualsWindow *self,
                        ManualsTab    *tab)
{
  g_return_if_fail (MANUALS_IS_WINDOW (self));
  g_return_if_fail (MANUALS_IS_TAB (tab));

  manuals_window_add_tab_internal (self, tab);
}

ManualsTab *
manuals_window_get_visible_tab (ManualsWindow *self)
{
  AdwTabPage *page;

  g_return_val_if_fail (MANUALS_IS_WINDOW (self), NULL);

  if (self->tab_view == NULL)
    return NULL;

  if ((page = adw_tab_view_get_selected_page (self->tab_view)))
    return MANUALS_TAB (adw_tab_page_get_child (page));

  return NULL;
}

void
manuals_window_set_visible_tab (ManualsWindow *self,
                                ManualsTab    *tab)
{
  AdwTabPage *page;

  g_return_if_fail (MANUALS_IS_WINDOW (self));
  g_return_if_fail (MANUALS_IS_TAB (tab));

  if ((page = adw_tab_view_get_page (self->tab_view, GTK_WIDGET (tab))))
    adw_tab_view_set_selected_page (self->tab_view, page);
}

ManualsRepository *
manuals_window_get_repository (ManualsWindow *self)
{
  g_return_val_if_fail (MANUALS_IS_WINDOW (self), NULL);

  return self->repository;
}

ManualsWindow *
manuals_window_from_widget (GtkWidget *widget)
{
  GtkWidget *window;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  window = gtk_widget_get_ancestor (widget, MANUALS_TYPE_WINDOW);

  g_return_val_if_fail (MANUALS_IS_WINDOW (window), NULL);

  return MANUALS_WINDOW (window);
}

void
manuals_window_navigate_to (ManualsWindow      *self,
                            ManualsNavigatable *navigatable)
{
  const char *uri;

  g_return_if_fail (MANUALS_IS_WINDOW (self));
  g_return_if_fail (MANUALS_IS_NAVIGATABLE (navigatable));

  if ((uri = manuals_navigatable_get_uri (navigatable)))
    {
      ManualsTab *tab = manuals_window_get_visible_tab (self);

      if (manuals_application_control_is_pressed ())
        {
          tab = manuals_tab_new ();
          manuals_window_add_tab (self, tab);
          manuals_window_set_visible_tab (self, tab);
        }

      manuals_tab_set_navigatable (tab, navigatable);
      gtk_widget_grab_focus (GTK_WIDGET (tab));
    }
  else
    {
      panel_dock_set_reveal_start (self->dock, TRUE);
      manuals_sidebar_reveal (self->sidebar, navigatable);
    }
}
