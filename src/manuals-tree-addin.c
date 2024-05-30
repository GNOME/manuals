/*
 * manuals-tree-addin.c
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

#include "manuals-application.h"
#include "manuals-book.h"
#include "manuals-heading.h"
#include "manuals-repository.h"
#include "manuals-sdk.h"
#include "manuals-tab.h"
#include "manuals-tree-addin.h"
#include "manuals-window.h"

struct _ManualsTreeAddin
{
  GObject parent_instance;
};

static DexFuture *
apply_children_posible (DexFuture *completed,
                        gpointer   user_data)
{
  IdeTreeNode *node = user_data;
  gboolean children_possible;

  g_assert (DEX_IS_FUTURE (completed));
  g_assert (dex_future_is_resolved (completed));
  g_assert (IDE_IS_TREE_NODE (node));

  children_possible = dex_await_boolean (dex_ref (completed), NULL);

  if (children_possible)
    {
      ide_tree_node_set_children_possible (node, TRUE);
      ide_tree_node_set_icon_name (node, "pan-end-symbolic");
      ide_tree_node_set_expanded_icon_name (node, "pan-down-symbolic");
    }

  return NULL;
}

static void
manuals_tree_addin_build_node (IdeTreeAddin *addin,
                               IdeTreeNode  *node)
{
  g_assert (MANUALS_IS_TREE_ADDIN (addin));
  g_assert (IDE_IS_TREE_NODE (node));

  if (ide_tree_node_holds (node, MANUALS_TYPE_SDK))
    {
      ManualsSdk *sdk = ide_tree_node_get_item (node);
      g_autofree char *title = manuals_sdk_dup_title (sdk);
      const char *icon_name = manuals_sdk_get_icon_name (sdk);

      ide_tree_node_set_title (node, title);
      ide_tree_node_set_icon_name (node, icon_name);
      ide_tree_node_set_children_possible (node, TRUE);
      ide_tree_node_set_is_header (node, TRUE);
    }
  else if (ide_tree_node_holds (node, MANUALS_TYPE_BOOK))
    {
      ManualsBook *book = ide_tree_node_get_item (node);
      const char *title = manuals_book_get_title (book);

      ide_tree_node_set_title (node, title);
      ide_tree_node_set_icon_name (node, "book-symbolic");
      ide_tree_node_set_children_possible (node, TRUE);
      ide_tree_node_set_is_header (node, TRUE);
    }
  else if (ide_tree_node_holds (node, MANUALS_TYPE_HEADING))
    {
      ManualsHeading *heading = ide_tree_node_get_item (node);
      const char *title = manuals_heading_get_title (heading);
      DexFuture *future;

      ide_tree_node_set_title (node, title);
      ide_tree_node_set_children_possible (node, FALSE);

      future = manuals_heading_has_children (heading);
      future = dex_future_then (future,
                                apply_children_posible,
                                g_object_ref (node),
                                g_object_unref);
      dex_future_disown (future);
    }
}

static DexFuture *
manuals_tree_addin_add_children (DexFuture *completed,
                                 gpointer   user_data)
{
  IdeTreeNode *node = user_data;
  g_autoptr(GListModel) list = NULL;
  guint n_items;

  g_assert (DEX_IS_FUTURE (completed));
  g_assert (dex_future_is_resolved (completed));
  g_assert (IDE_IS_TREE_NODE (node));

  list = dex_await_object (dex_ref (completed), NULL);
  n_items = g_list_model_get_n_items (list);

  for (guint i = 0; i < n_items; i++)
    {
      g_autoptr(GObject) item = g_list_model_get_item (list, i);
      g_autoptr(IdeTreeNode) child = ide_tree_node_new ();

      ide_tree_node_set_item (child, item);
      ide_tree_node_insert_before (child, node, NULL);
    }

  return dex_future_new_for_boolean (TRUE);
}

static void
manuals_tree_addin_build_children_async (IdeTreeAddin        *addin,
                                         IdeTreeNode         *node,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             user_data)
{
  g_autoptr(DexAsyncResult) result = NULL;
  DexFuture *future = NULL;

  g_assert (MANUALS_IS_TREE_ADDIN (addin));
  g_assert (IDE_IS_TREE_NODE (node));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  result = dex_async_result_new (addin, cancellable, callback, user_data);

  if (ide_tree_node_holds (node, MANUALS_TYPE_REPOSITORY))
    {
      ManualsRepository *repository = ide_tree_node_get_item (node);

      future = manuals_repository_list_sdks (repository);
      future = dex_future_then (future,
                                manuals_tree_addin_add_children,
                                g_object_ref (node),
                                g_object_unref);
    }
  else if (ide_tree_node_holds (node, MANUALS_TYPE_SDK))
    {
      ManualsSdk *sdk = ide_tree_node_get_item (node);

      future = manuals_sdk_list_books (sdk);
      future = dex_future_then (future,
                                manuals_tree_addin_add_children,
                                g_object_ref (node),
                                g_object_unref);
    }
  else if (ide_tree_node_holds (node, MANUALS_TYPE_BOOK))
    {
      ManualsBook *book = ide_tree_node_get_item (node);

      future = manuals_book_list_headings (book);
      future = dex_future_then (future,
                                manuals_tree_addin_add_children,
                                g_object_ref (node),
                                g_object_unref);
    }
  else if (ide_tree_node_holds (node, MANUALS_TYPE_HEADING))
    {
      ManualsHeading *heading = ide_tree_node_get_item (node);

      future = manuals_heading_list_headings (heading);
      future = dex_future_then (future,
                                manuals_tree_addin_add_children,
                                g_object_ref (node),
                                g_object_unref);
    }
  else
    {
      future = dex_future_new_for_boolean (TRUE);
    }

  g_assert (DEX_IS_FUTURE (future));

  dex_async_result_await (result, g_steal_pointer (&future));
}

static gboolean
manuals_tree_addin_build_children_finish (IdeTreeAddin  *addin,
                                          GAsyncResult  *result,
                                          GError       **error)
{
  g_assert (MANUALS_IS_TREE_ADDIN (addin));
  g_assert (DEX_IS_ASYNC_RESULT (result));

  return dex_async_result_propagate_boolean (DEX_ASYNC_RESULT (result), error);
}

static gboolean
manuals_tree_addin_node_activated (IdeTreeAddin *addin,
                                   IdeTree      *tree,
                                   IdeTreeNode  *node)
{

  g_assert (MANUALS_IS_TREE_ADDIN (addin));
  g_assert (IDE_IS_TREE (tree));
  g_assert (IDE_IS_TREE_NODE (node));

  if (ide_tree_node_holds (node, MANUALS_TYPE_BOOK) ||
      ide_tree_node_holds (node, MANUALS_TYPE_HEADING))
    {
      ManualsWindow *window = manuals_window_from_widget (GTK_WIDGET (tree));
      g_autoptr(ManualsNavigatable) navigatable = manuals_navigatable_new_for_resource (ide_tree_node_get_item (node));
      ManualsTab *tab = manuals_window_get_visible_tab (window);

      if (!tab || manuals_application_control_is_pressed ())
        {
          tab = manuals_tab_new ();
          manuals_window_add_tab (window, tab);
          manuals_window_set_visible_tab (window, tab);
        }

      manuals_tab_set_navigatable (tab, navigatable);
      gtk_widget_grab_focus (GTK_WIDGET (tab));
    }

  return !ide_tree_node_get_children_possible (node);
}

static void
tree_addin_iface_init (IdeTreeAddinInterface *iface)
{
  iface->build_node = manuals_tree_addin_build_node;
  iface->build_children_async = manuals_tree_addin_build_children_async;
  iface->build_children_finish = manuals_tree_addin_build_children_finish;
  iface->node_activated = manuals_tree_addin_node_activated;
}

G_DEFINE_FINAL_TYPE_WITH_CODE (ManualsTreeAddin, manuals_tree_addin, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (IDE_TYPE_TREE_ADDIN, tree_addin_iface_init))

static void
manuals_tree_addin_class_init (ManualsTreeAddinClass *klass)
{
}

static void
manuals_tree_addin_init (ManualsTreeAddin *self)
{
}
