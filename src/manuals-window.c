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
#include "manuals-search-entry.h"
#include "manuals-search-result.h"
#include "manuals-search-view.h"
#include "manuals-tab.h"
#include "manuals-window.h"

#define MODE_EMPTY   "empty"
#define MODE_SEARCH  "search"
#define MODE_TABS    "tabs"
#define MODE_LISTING "listing"

struct _ManualsWindow
{
  AdwApplicationWindow  parent_instance;

  const char           *mode;

  ManualsRepository    *repository;
  GSignalGroup         *visible_tab_signals;

  GtkStack             *omni_stack;
  ManualsPathModel     *path_model;
  ManualsSearchEntry   *search_entry;
  ManualsSearchView    *search_view;
  GtkStack             *stack;
  AdwTabOverview       *tab_overview;
  AdwTabView           *tab_view;
  AdwToolbarView       *toolbar_view;

  guint                 disposed : 1;
};

G_DEFINE_FINAL_TYPE (ManualsWindow, manuals_window, ADW_TYPE_APPLICATION_WINDOW)

enum {
  PROP_0,
  PROP_REPOSITORY,
  PROP_MODE,
  PROP_VISIBLE_TAB,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

ManualsWindow *
manuals_window_new (void)
{
  return g_object_new (MANUALS_TYPE_WINDOW, NULL);
}

static void
set_mode (ManualsWindow *self,
          const char    *mode)
{
  self->mode = mode;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODE]);

  if (g_str_equal (mode, MODE_EMPTY))
    {
      gtk_stack_set_visible_child_name (self->stack, "empty");
      gtk_stack_set_visible_child_name (self->omni_stack, "search");
      adw_toolbar_view_set_top_bar_style (self->toolbar_view, ADW_TOOLBAR_FLAT);
      gtk_widget_grab_focus (GTK_WIDGET (self->search_entry));
    }
  else if (g_str_equal (mode, MODE_SEARCH))
    {
      gtk_stack_set_visible_child_name (self->stack, "search");
      gtk_stack_set_visible_child_name (self->omni_stack, "search");
      adw_toolbar_view_set_top_bar_style (self->toolbar_view, ADW_TOOLBAR_FLAT);
      gtk_widget_grab_focus (GTK_WIDGET (self->search_entry));
    }
  else if (g_str_equal (mode, MODE_LISTING))
    {
      gtk_stack_set_visible_child_name (self->stack, "search");
      gtk_stack_set_visible_child_name (self->omni_stack, "browse");
      adw_toolbar_view_set_top_bar_style (self->toolbar_view, ADW_TOOLBAR_FLAT);
      gtk_widget_grab_focus (GTK_WIDGET (self->search_view));
    }
  else if (g_str_equal (mode, MODE_TABS))
    {
      gtk_stack_set_visible_child_name (self->stack, "tabs");
      gtk_stack_set_visible_child_name (self->omni_stack, "browse");
      adw_toolbar_view_set_top_bar_style (self->toolbar_view, ADW_TOOLBAR_RAISED);
      gtk_widget_grab_focus (GTK_WIDGET (self->tab_view));
    }
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
manuals_window_search_entry_focus_enter_cb (ManualsWindow           *self,
                                            GtkEventControllerFocus *focus)
{
  g_assert (MANUALS_IS_WINDOW (self));
  g_assert (GTK_IS_EVENT_CONTROLLER_FOCUS (focus));

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
manuals_window_search_entry_begin_search_cb (ManualsWindow      *self,
                                             ManualsSearchQuery *query,
                                             ManualsSearchEntry *search_entry)
{
  g_assert (MANUALS_IS_WINDOW (self));
  g_assert (MANUALS_IS_SEARCH_QUERY (query));
  g_assert (MANUALS_IS_SEARCH_ENTRY (search_entry));

  gtk_stack_set_visible_child_name (self->stack, "search");

  manuals_search_view_search (self->search_view, query);
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
manuals_window_begin_search_action (GtkWidget  *widget,
                                    const char *action_name,
                                    GVariant   *param)
{
  ManualsWindow *self = MANUALS_WINDOW (widget);

  g_assert (MANUALS_IS_WINDOW (self));

  set_mode (self, MODE_SEARCH);
}

static void
manuals_window_toggle_overview_action (GtkWidget  *widget,
                                       const char *action_name,
                                       GVariant   *param)
{
  ManualsWindow *self = MANUALS_WINDOW (widget);

  g_assert (MANUALS_IS_WINDOW (self));

  adw_tab_overview_set_open (self->tab_overview,
                             !adw_tab_overview_get_open (self->tab_overview));
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

static DexFuture *
manuals_window_browse_root_cb (DexFuture *completed,
                               gpointer   user_data)
{
  ManualsWindow *self = user_data;
  g_autoptr(ManualsNavigatable) navigatable = NULL;
  g_autoptr(GListModel) model = NULL;

  g_assert (MANUALS_IS_WINDOW (self));
  g_assert (DEX_IS_FUTURE (completed));

  model = dex_await_object (dex_ref (completed), NULL);

  /* If there is only one SDK, just dive into it */
  if (g_list_model_get_n_items (model) == 1)
    {
      g_autoptr(GObject) instance = g_list_model_get_item (model, 0);
      navigatable = manuals_navigatable_new_for_resource (instance);
    }
  else
    {
      navigatable = manuals_navigatable_new_for_resource (G_OBJECT (self->repository));
    }

  manuals_window_show_listing (self, navigatable);

  return dex_future_new_for_boolean (TRUE);
}

static void
manuals_window_browse_root_action (GtkWidget  *widget,
                                   const char *action_name,
                                   GVariant   *param)
{
  ManualsWindow *self = MANUALS_WINDOW (widget);
  DexFuture *future;

  g_assert (MANUALS_IS_WINDOW (self));

  future = manuals_repository_list_sdks (self->repository);
  future = dex_future_then (future,
                            manuals_window_browse_root_cb,
                            g_object_ref (self),
                            g_object_unref);

  dex_future_disown (future);
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
}

static AdwTabPage *
manuals_window_tab_overview_create_tab_cb (ManualsWindow  *self,
                                           AdwTabOverview *tab_overview)
{
  g_assert (MANUALS_IS_WINDOW (self));
  g_assert (ADW_IS_TAB_OVERVIEW (tab_overview));

  return manuals_window_add_tab_internal (self, manuals_tab_new ());
}

static gboolean
is_viewing_tabs (gpointer    instance,
                 const char *visible_child_name)
{
  return g_strcmp0 (visible_child_name, "tabs") == 0;
}

static void
manuals_window_activate_first_action (GtkWidget  *widget,
                                      const char *action_name,
                                      GVariant   *param)
{
  ManualsWindow *self = (ManualsWindow *)widget;
  g_autoptr(ManualsNavigatable) navigatable = NULL;
  ManualsTab *tab;

  g_assert (MANUALS_IS_WINDOW (self));

  if (!(navigatable = manuals_search_view_dup_selected (self->search_view)))
    return;

  if (!(tab = manuals_window_get_visible_tab (self)) || manuals_application_control_is_pressed ())
    {
      tab = manuals_tab_new ();
      manuals_window_add_tab (self, tab);
    }

  manuals_tab_set_navigatable (tab, navigatable);
  manuals_window_set_visible_tab (self, tab);
}

static void
manuals_window_search_move_down (GtkWidget  *widget,
                                 const char *action_name,
                                 GVariant   *param)
{
  ManualsWindow *self = (ManualsWindow *)widget;

  g_assert (MANUALS_IS_WINDOW (self));

  manuals_search_view_move_down (self->search_view);
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

  object_class->dispose = manuals_window_dispose;
  object_class->get_property = manuals_window_get_property;
  object_class->set_property = manuals_window_set_property;

  properties[PROP_MODE] =
    g_param_spec_string ("mode", NULL, NULL,
                         NULL,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

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

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Manuals/manuals-window.ui");

  gtk_widget_class_bind_template_child (widget_class, ManualsWindow, omni_stack);
  gtk_widget_class_bind_template_child (widget_class, ManualsWindow, path_model);
  gtk_widget_class_bind_template_child (widget_class, ManualsWindow, search_entry);
  gtk_widget_class_bind_template_child (widget_class, ManualsWindow, search_view);
  gtk_widget_class_bind_template_child (widget_class, ManualsWindow, stack);
  gtk_widget_class_bind_template_child (widget_class, ManualsWindow, tab_overview);
  gtk_widget_class_bind_template_child (widget_class, ManualsWindow, tab_view);
  gtk_widget_class_bind_template_child (widget_class, ManualsWindow, toolbar_view);

  gtk_widget_class_bind_template_callback (widget_class, manuals_window_search_entry_begin_search_cb);
  gtk_widget_class_bind_template_callback (widget_class, manuals_window_search_entry_focus_enter_cb);
  gtk_widget_class_bind_template_callback (widget_class, manuals_window_tab_overview_create_tab_cb);
  gtk_widget_class_bind_template_callback (widget_class, is_viewing_tabs);

  gtk_widget_class_install_action (widget_class, "win.begin-search", NULL, manuals_window_begin_search_action);
  gtk_widget_class_install_action (widget_class, "tab.go-back", NULL, manuals_window_tab_go_back_action);
  gtk_widget_class_install_action (widget_class, "tab.go-forward", NULL, manuals_window_tab_go_forward_action);
  gtk_widget_class_install_action (widget_class, "tab.close", NULL, manuals_window_tab_close_action);
  gtk_widget_class_install_action (widget_class, "win.toggle-overview", NULL, manuals_window_toggle_overview_action);
  gtk_widget_class_install_action (widget_class, "tab.new", NULL, manuals_window_tab_new_action);
  gtk_widget_class_install_action (widget_class, "win.browse-root", NULL, manuals_window_browse_root_action);
  gtk_widget_class_install_action (widget_class, "search.activate-first", NULL, manuals_window_activate_first_action);
  gtk_widget_class_install_action (widget_class, "search.move-down", NULL, manuals_window_search_move_down);

  g_type_ensure (MANUALS_TYPE_PATH_BAR);
  g_type_ensure (MANUALS_TYPE_SEARCH_ENTRY);
  g_type_ensure (MANUALS_TYPE_SEARCH_VIEW);
  g_type_ensure (MANUALS_TYPE_TAB);
}

static void
manuals_window_init (ManualsWindow *self)
{
#ifdef DEVELOPMENT_BUILD
  gtk_widget_add_css_class (GTK_WIDGET (self), "devel");
#endif

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

  set_mode (self, MODE_EMPTY);
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
    {
      set_mode (self, MODE_TABS);
      adw_tab_view_set_selected_page (self->tab_view, page);
      gtk_stack_set_visible_child_name (self->stack, "tabs");
    }
}

ManualsRepository *
manuals_window_get_repository (ManualsWindow *self)
{
  g_return_val_if_fail (MANUALS_IS_WINDOW (self), NULL);

  return self->repository;
}

typedef struct _ShowListing
{
  ManualsWindow      *self;
  ManualsNavigatable *navigatable;
} ShowListing;

static void
show_listing_free (gpointer data)
{
  ShowListing *show = data;

  g_clear_object (&show->self);
  g_clear_object (&show->navigatable);
  g_free (show);
}

static ShowListing *
show_listing_new (ManualsWindow      *self,
                  ManualsNavigatable *navigatable)
{
  ShowListing *show;

  show = g_new0 (ShowListing, 1);
  show->self = g_object_ref (self);
  show->navigatable = g_object_ref (navigatable);

  return show;
}

static gpointer
wrap_in_result_func (gpointer item,
                     gpointer user_data)
{
  g_autoptr(GObject) object = item;
  ManualsSearchResult *result;

  result = manuals_search_result_new (0);
  manuals_search_result_set_item (result, object);

  return result;
}

static DexFuture *
manuals_window_show_listing_cb (DexFuture *completed,
                                gpointer   data)
{
  g_autoptr(GtkMapListModel) map = NULL;
  ManualsSearchColumns columns = MANUALS_SEARCH_COLUMN_ALL;
  ShowListing *show = data;
  GObject *item;

  g_assert (show != NULL);
  g_assert (MANUALS_IS_WINDOW (show->self));
  g_assert (MANUALS_IS_NAVIGATABLE (show->navigatable));


  item = manuals_navigatable_get_item (show->navigatable);

  if (MANUALS_IS_REPOSITORY (item))
    columns &= ~(MANUALS_SEARCH_COLUMN_SDK|MANUALS_SEARCH_COLUMN_BOOK);
  else if (MANUALS_IS_SDK (item))
    columns &= ~MANUALS_SEARCH_COLUMN_BOOK;

  manuals_path_model_set_navigatable (show->self->path_model, show->navigatable);
  map = gtk_map_list_model_new (dex_await_object (dex_ref (completed), NULL),
                                wrap_in_result_func, NULL, NULL);
  manuals_search_view_display (show->self->search_view,
                               G_LIST_MODEL (map),
                               columns);
  set_mode (show->self, MODE_LISTING);

  return dex_future_new_for_boolean (TRUE);
}

void
manuals_window_show_listing (ManualsWindow      *self,
                             ManualsNavigatable *navigatable)
{
  DexFuture *future;

  g_return_if_fail (MANUALS_IS_WINDOW (self));
  g_return_if_fail (MANUALS_IS_NAVIGATABLE (navigatable));

  future = manuals_navigatable_find_children (navigatable);
  future = dex_future_then (future,
                            manuals_window_show_listing_cb,
                            show_listing_new (self, navigatable),
                            show_listing_free);

  dex_future_disown (future);
}
