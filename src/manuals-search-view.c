/*
 * manuals-search-view.c
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
#include <gom/gom.h>

#include "manuals-application.h"
#include "manuals-book.h"
#include "manuals-heading.h"
#include "manuals-keyword.h"
#include "manuals-navigatable.h"
#include "manuals-repository.h"
#include "manuals-sdk.h"
#include "manuals-search-result.h"
#include "manuals-search-view.h"
#include "manuals-tab.h"
#include "manuals-tag.h"
#include "manuals-utils.h"
#include "manuals-window.h"

struct _ManualsSearchView
{
  GtkWidget             parent_instance;

  ManualsSearchQuery   *query;

  GtkColumnView        *column_view;
  GtkColumnViewColumn  *name_column;
  GtkColumnViewColumn  *book_column;
  GtkColumnViewColumn  *sdk_column;
};

G_DEFINE_FINAL_TYPE (ManualsSearchView, manuals_search_view, GTK_TYPE_WIDGET)

static void
manuals_search_view_column_view_activate_cb (ManualsSearchView *self,
                                             guint              position,
                                             GtkColumnView     *column_view)
{
  g_autoptr(ManualsSearchResult) result = NULL;
  ManualsNavigatable *navigatable;
  GtkSelectionModel *model;
  ManualsWindow *window;
  ManualsTab *tab;
  GObject *item;

  g_assert (MANUALS_IS_SEARCH_VIEW (self));
  g_assert (GTK_IS_COLUMN_VIEW (column_view));

  model = gtk_column_view_get_model (column_view);
  result = g_list_model_get_item (G_LIST_MODEL (model), position);
  navigatable = manuals_search_result_get_item (result);
  item = manuals_navigatable_get_item (navigatable);

  window = MANUALS_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (self), MANUALS_TYPE_WINDOW));

  if (MANUALS_IS_SDK (item))
    {
      manuals_window_show_listing (window, navigatable);
      return;
    }

  tab = manuals_window_get_visible_tab (window);
  if (tab == NULL || manuals_application_control_is_pressed ())
    {
      tab = manuals_tab_duplicate (tab);
      manuals_window_add_tab (window, tab);
    }

  manuals_tab_set_navigatable (tab, navigatable);
  manuals_window_set_visible_tab (window, tab);
}

static gboolean
non_empty_string (gpointer    instance,
                  const char *str)
{
  return !_g_str_empty0 (str);
}

static char *
format_language (GtkListItem *list_item,
                 const char  *language)
{
  if (language && strcmp (language, "c") == 0)
    return g_strdup ("C");

  return g_strdup (language);
}

static char *
get_sdk_title (GtkListItem *list_item,
               GObject     *item)
{
  g_autoptr(ManualsRepository) repository = NULL;
  const char *title = NULL;
  gint64 book_id = 0;
  gint64 sdk_id;

  g_assert (GOM_IS_RESOURCE (item));

  g_object_get (item, "repository", &repository, NULL);

  if (MANUALS_IS_KEYWORD (item))
    book_id = manuals_keyword_get_book_id (MANUALS_KEYWORD (item));

  if (MANUALS_IS_HEADING (item))
    book_id = manuals_heading_get_book_id (MANUALS_HEADING (item));

  if (MANUALS_IS_BOOK (item))
    book_id = manuals_book_get_id (MANUALS_BOOK (item));

  if (book_id != 0 &&
      (sdk_id = manuals_repository_get_cached_sdk_id (repository, book_id)))
    title = manuals_repository_get_cached_sdk_title (repository, sdk_id);

  return g_strdup (title);
}

static char *
get_book_title (GtkListItem *list_item,
                GObject     *item)
{
  g_autoptr(ManualsRepository) repository = NULL;
  const char *title = NULL;
  gint64 book_id = 0;

  g_assert (GOM_IS_RESOURCE (item));

  g_object_get (item, "repository", &repository, NULL);

  if (MANUALS_IS_KEYWORD (item))
    book_id = manuals_keyword_get_book_id (MANUALS_KEYWORD (item));

  if (MANUALS_IS_HEADING (item))
    book_id = manuals_heading_get_book_id (MANUALS_HEADING (item));

  if (book_id != 0)
    title = manuals_repository_get_cached_book_title (repository, book_id);

  return g_strdup (title);
}

static void
manuals_search_view_dispose (GObject *object)
{
  ManualsSearchView *self = (ManualsSearchView *)object;
  GtkWidget *child;

  gtk_widget_dispose_template (GTK_WIDGET (self), MANUALS_TYPE_SEARCH_VIEW);

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self))))
    gtk_widget_unparent (child);

  g_clear_object (&self->query);

  G_OBJECT_CLASS (manuals_search_view_parent_class)->dispose (object);
}

static void
manuals_search_view_class_init (ManualsSearchViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = manuals_search_view_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/app/devsuite/Manuals/manuals-search-view.ui");
  gtk_widget_class_set_css_name (widget_class, "searchview");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);

  gtk_widget_class_bind_template_child (widget_class, ManualsSearchView, column_view);
  gtk_widget_class_bind_template_child (widget_class, ManualsSearchView, book_column);
  gtk_widget_class_bind_template_child (widget_class, ManualsSearchView, name_column);
  gtk_widget_class_bind_template_child (widget_class, ManualsSearchView, sdk_column);

  gtk_widget_class_bind_template_callback (widget_class, manuals_search_view_column_view_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, non_empty_string);
  gtk_widget_class_bind_template_callback (widget_class, get_book_title);
  gtk_widget_class_bind_template_callback (widget_class, get_sdk_title);
  gtk_widget_class_bind_template_callback (widget_class, format_language);

  g_type_ensure (MANUALS_TYPE_NAVIGATABLE);
  g_type_ensure (MANUALS_TYPE_SEARCH_RESULT);
  g_type_ensure (MANUALS_TYPE_TAG);
}

static void
manuals_search_view_init (ManualsSearchView *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
set_visible_columns (ManualsSearchView    *self,
                     ManualsSearchColumns  columns)
{
  gtk_column_view_column_set_visible (self->name_column,
                                      !!(columns & MANUALS_SEARCH_COLUMN_NAME));
  gtk_column_view_column_set_visible (self->sdk_column,
                                      !!(columns & MANUALS_SEARCH_COLUMN_SDK));
  gtk_column_view_column_set_visible (self->book_column,
                                      !!(columns & MANUALS_SEARCH_COLUMN_BOOK));
}

typedef struct _Search
{
  ManualsSearchView *self;
  ManualsSearchQuery *query;
} Search;

static void
search_free (Search *search)
{
  g_clear_object (&search->self);
  g_clear_object (&search->query);
  g_free (search);
}

static DexFuture *
manuals_search_view_show_results (DexFuture *completed,
                                  gpointer   user_data)
{
  Search *search = user_data;

  g_assert (search != NULL);
  g_assert (MANUALS_IS_SEARCH_VIEW (search->self));
  g_assert (MANUALS_IS_SEARCH_QUERY (search->query));
  g_assert (DEX_IS_FUTURE (completed));

  if (search->query == search->self->query)
    {
      g_autoptr(GtkNoSelection) no = gtk_no_selection_new (g_object_ref (G_LIST_MODEL (search->query)));

      gtk_column_view_set_model (search->self->column_view, GTK_SELECTION_MODEL (no));
      set_visible_columns (search->self, MANUALS_SEARCH_COLUMN_ALL);
    }

  return dex_future_new_for_boolean (TRUE);
}

void
manuals_search_view_search (ManualsSearchView  *self,
                            ManualsSearchQuery *query)
{
  ManualsRepository *repository;
  ManualsWindow *window;
  DexFuture *future;
  Search *search;

  g_return_if_fail (MANUALS_IS_SEARCH_VIEW (self));
  g_return_if_fail (MANUALS_IS_SEARCH_QUERY (query));

  if (!g_set_object (&self->query, query))
    return;

  search = g_new0 (Search, 1);
  search->self = g_object_ref (self);
  search->query = g_object_ref (query);

  window = MANUALS_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (self), MANUALS_TYPE_WINDOW));
  repository = manuals_window_get_repository (window);

  future = manuals_search_query_execute (query, repository);
  future = dex_future_then (future,
                            manuals_search_view_show_results,
                            search,
                            (GDestroyNotify)search_free);

  dex_future_disown (future);
}

void
manuals_search_view_display (ManualsSearchView    *self,
                             GListModel           *results,
                             ManualsSearchColumns  columns)
{
  g_autoptr(GtkNoSelection) no = NULL;

  g_return_if_fail (MANUALS_IS_SEARCH_VIEW (self));
  g_return_if_fail (!results || G_IS_LIST_MODEL (results));

  no = gtk_no_selection_new (g_object_ref (results));

  g_set_object (&self->query, NULL);
  gtk_column_view_set_model (self->column_view, GTK_SELECTION_MODEL (no));
  set_visible_columns (self, columns);
}

ManualsNavigatable *
manuals_search_view_dup_selected (ManualsSearchView *self)
{
  g_autoptr(ManualsSearchResult) result = NULL;
  GtkSelectionModel *model;

  g_return_val_if_fail (MANUALS_IS_SEARCH_VIEW (self), NULL);

  /* TODO: Selection */

  model = gtk_column_view_get_model (self->column_view);

  if ((result = g_list_model_get_item (G_LIST_MODEL (model), 0)))
    {
      ManualsNavigatable *navigatable;

      g_assert (MANUALS_IS_SEARCH_RESULT (result));

      if ((navigatable = manuals_search_result_get_item (MANUALS_SEARCH_RESULT (result))))
        {
          g_assert (MANUALS_IS_NAVIGATABLE (navigatable));
          return g_object_ref (navigatable);
        }
    }

  return NULL;
}

void
manuals_search_view_move_down (ManualsSearchView *self)
{
  g_return_if_fail (MANUALS_IS_SEARCH_VIEW (self));

  gtk_widget_child_focus (GTK_WIDGET (self->column_view), GTK_DIR_TAB_FORWARD);
  gtk_column_view_scroll_to (self->column_view, 0, NULL, GTK_LIST_SCROLL_SELECT, NULL);
}
