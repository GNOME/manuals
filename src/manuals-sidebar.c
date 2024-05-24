/*
 * manuals-sidebar.c
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

#include "manuals-application.h"
#include "manuals-keyword.h"
#include "manuals-navigatable.h"
#include "manuals-search-query.h"
#include "manuals-search-result.h"
#include "manuals-sidebar.h"
#include "manuals-tag.h"
#include "manuals-tree-expander.h"
#include "manuals-utils.h"
#include "manuals-window.h"

struct _ManualsSidebar
{
  GtkWidget           parent_instance;

  GtkListView        *browse_view;
  GtkListView        *search_view;
  GtkSearchEntry     *search_entry;
  GtkStack           *stack;

  DexFuture          *query;
  ManualsRepository  *repository;
};

G_DEFINE_FINAL_TYPE (ManualsSidebar, manuals_sidebar, GTK_TYPE_WIDGET)

enum {
  PROP_0,
  PROP_REPOSITORY,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void
manuals_sidebar_search_changed_cb (ManualsSidebar *self,
                                   GtkSearchEntry *search_entry)
{
  g_autofree char *text = NULL;

  g_assert (MANUALS_IS_SIDEBAR (self));
  g_assert (GTK_IS_SEARCH_ENTRY (search_entry));

  dex_clear (&self->query);

  text = g_strstrip (g_strdup (gtk_editable_get_text (GTK_EDITABLE (search_entry))));

  if (_g_str_empty0 (text))
    {
      gtk_stack_set_visible_child_name (self->stack, "browse");
    }
  else
    {
      g_autoptr(ManualsSearchQuery) query = manuals_search_query_new ();
      g_autoptr(GtkNoSelection) selection = NULL;

      manuals_search_query_set_text (query, text);

      /* Hold on to the future so we can cancel it by releasing when a
       * new search comes in.
       */
      self->query = manuals_search_query_execute (query, self->repository);
      selection = gtk_no_selection_new (g_object_ref (G_LIST_MODEL (query)));
      gtk_list_view_set_model (self->search_view, GTK_SELECTION_MODEL (selection));

      gtk_stack_set_visible_child_name (self->stack, "search");
    }
}

static void
manuals_sidebar_search_view_activate_cb (ManualsSidebar *self,
                                         guint           position,
                                         GtkListView    *list_view)
{
  g_autoptr(ManualsSearchResult) result = NULL;
  ManualsNavigatable *navigatable;
  GtkSelectionModel *model;
  ManualsWindow *window;
  ManualsTab *tab;

  g_assert (MANUALS_IS_SIDEBAR (self));
  g_assert (GTK_IS_LIST_VIEW (list_view));

  model = gtk_list_view_get_model (list_view);
  result = g_list_model_get_item (G_LIST_MODEL (model), position);
  navigatable = manuals_search_result_get_item (result);

  if (navigatable == NULL)
    return;


  window = MANUALS_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (self), MANUALS_TYPE_WINDOW));

  g_assert (MANUALS_IS_NAVIGATABLE (navigatable));
  g_assert (MANUALS_IS_WINDOW (window));

  tab = manuals_window_get_visible_tab (window);

  if (manuals_application_control_is_pressed ())
    {
      tab = manuals_tab_new ();
      manuals_window_add_tab (window, tab);
      manuals_window_set_visible_tab (window, tab);
    }

  manuals_tab_set_navigatable (tab, navigatable);
}

static gboolean
nonempty_to_boolean (gpointer    instance,
                     const char *data)
{
  return !_g_str_empty0 (data);
}

static char *
lookup_sdk_title (gpointer        instance,
                  ManualsKeyword *keyword)
{
  g_autoptr(ManualsRepository) repository = NULL;
  gint64 book_id;
  gint64 sdk_id;

  g_assert (!keyword || MANUALS_IS_KEYWORD (keyword));

  if (keyword == NULL)
    return NULL;

  g_object_get (keyword, "repository", &repository, NULL);
  book_id = manuals_keyword_get_book_id (keyword);
  sdk_id = manuals_repository_get_cached_sdk_id (repository, book_id);

  return g_strdup (manuals_repository_get_cached_sdk_title (repository, sdk_id));
}

static void
manuals_sidebar_dispose (GObject *object)
{
  ManualsSidebar *self = (ManualsSidebar *)object;
  GtkWidget *child;

  gtk_widget_dispose_template (GTK_WIDGET (self), MANUALS_TYPE_SIDEBAR);

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self))))
    gtk_widget_unparent (child);

  dex_clear (&self->query);
  g_clear_object (&self->repository);

  G_OBJECT_CLASS (manuals_sidebar_parent_class)->dispose (object);
}

static void
manuals_sidebar_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  ManualsSidebar *self = MANUALS_SIDEBAR (object);

  switch (prop_id)
    {
    case PROP_REPOSITORY:
      g_value_set_object (value, manuals_sidebar_get_repository (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
manuals_sidebar_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ManualsSidebar *self = MANUALS_SIDEBAR (object);

  switch (prop_id)
    {
    case PROP_REPOSITORY:
      manuals_sidebar_set_repository (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
manuals_sidebar_class_init (ManualsSidebarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = manuals_sidebar_dispose;
  object_class->get_property = manuals_sidebar_get_property;
  object_class->set_property = manuals_sidebar_set_property;

  gtk_widget_class_set_template_from_resource (widget_class, "/app/devsuite/Manuals/manuals-sidebar.ui");
  gtk_widget_class_set_css_name (widget_class, "sidebar");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);

  gtk_widget_class_bind_template_child (widget_class, ManualsSidebar, browse_view);
  gtk_widget_class_bind_template_child (widget_class, ManualsSidebar, search_entry);
  gtk_widget_class_bind_template_child (widget_class, ManualsSidebar, search_view);
  gtk_widget_class_bind_template_child (widget_class, ManualsSidebar, stack);

  gtk_widget_class_bind_template_callback (widget_class, manuals_sidebar_search_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, manuals_sidebar_search_view_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, nonempty_to_boolean);
  gtk_widget_class_bind_template_callback (widget_class, lookup_sdk_title);

  properties[PROP_REPOSITORY] =
    g_param_spec_object ("repository", NULL, NULL,
                         MANUALS_TYPE_REPOSITORY,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  g_type_ensure (MANUALS_TYPE_TREE_EXPANDER);
  g_type_ensure (MANUALS_TYPE_NAVIGATABLE);
  g_type_ensure (MANUALS_TYPE_SEARCH_RESULT);
  g_type_ensure (MANUALS_TYPE_TAG);
}

static void
manuals_sidebar_init (ManualsSidebar *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

ManualsRepository *
manuals_sidebar_get_repository (ManualsSidebar *self)
{
  g_return_val_if_fail (MANUALS_IS_SIDEBAR (self), NULL);

  return self->repository;
}

void
manuals_sidebar_set_repository (ManualsSidebar    *self,
                                ManualsRepository *repository)
{
  g_return_if_fail (MANUALS_IS_SIDEBAR (self));
  g_return_if_fail (MANUALS_IS_REPOSITORY (repository));

  if (g_set_object (&self->repository, repository))
    {
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_REPOSITORY]);
    }
}

void
manuals_sidebar_focus_search (ManualsSidebar *self)
{
  g_return_if_fail (MANUALS_IS_SIDEBAR (self));

  gtk_widget_grab_focus (GTK_WIDGET (self->search_entry));
  gtk_editable_select_region (GTK_EDITABLE (self->search_entry), 0, -1);
}

void
manuals_sidebar_reveal (ManualsSidebar     *self,
                        ManualsNavigatable *navigatable)
{
  g_return_if_fail (MANUALS_IS_SIDEBAR (self));
  g_return_if_fail (!navigatable || MANUALS_IS_NAVIGATABLE (navigatable));

  gtk_editable_set_text (GTK_EDITABLE (self->search_entry), "");
  gtk_stack_set_visible_child_name (self->stack, "browse");
}
