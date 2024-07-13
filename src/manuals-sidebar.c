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

#include <libide-tree/ide-tree.h>

#include "manuals-application.h"
#include "manuals-keyword.h"
#include "manuals-navigatable.h"
#include "manuals-search-query.h"
#include "manuals-search-result.h"
#include "manuals-sidebar.h"
#include "manuals-tag.h"
#include "manuals-tree-addin.h"
#include "manuals-tree-expander.h"
#include "manuals-utils.h"
#include "manuals-window.h"

struct _ManualsSidebar
{
  GtkWidget           parent_instance;

  GtkListView        *search_view;
  GtkSearchEntry     *search_entry;
  GtkStack           *stack;
  GtkButton          *back_button;
  IdeTree            *tree;
  GtkBox             *box;

  DexFuture          *query;
  ManualsRepository  *repository;
  ManualsNavigatable *reveal;

  guint               reveal_expand : 1;
};

G_DEFINE_FINAL_TYPE (ManualsSidebar, manuals_sidebar, GTK_TYPE_WIDGET)

enum {
  PROP_0,
  PROP_REPOSITORY,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void
manuals_sidebar_activate (ManualsSidebar *self,
                          guint           position)
{
  g_autoptr(ManualsSearchResult) result = NULL;
  ManualsNavigatable *navigatable;
  GtkSelectionModel *model;
  ManualsWindow *window;
  ManualsTab *tab;

  g_assert (MANUALS_IS_SIDEBAR (self));

  if (position == GTK_INVALID_LIST_POSITION)
    return;

  model = gtk_list_view_get_model (self->search_view);

  if (!(result = g_list_model_get_item (G_LIST_MODEL (model), position)) ||
      !(navigatable = manuals_search_result_get_item (result)))
    return;

  window = MANUALS_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (self), MANUALS_TYPE_WINDOW));

  g_assert (MANUALS_IS_NAVIGATABLE (navigatable));
  g_assert (MANUALS_IS_WINDOW (window));

  tab = manuals_window_get_visible_tab (window);

  if (!tab || manuals_application_control_is_pressed ())
    {
      tab = manuals_tab_new ();
      manuals_window_add_tab (window, tab);
      manuals_window_set_visible_tab (window, tab);
    }

  manuals_tab_set_navigatable (tab, navigatable);
}

static void
manuals_sidebar_selection_changed_cb (ManualsSidebar     *self,
                                      guint               position,
                                      guint               n_items,
                                      GtkSingleSelection *selection)
{
  g_assert (MANUALS_IS_SIDEBAR (self));
  g_assert (GTK_IS_SINGLE_SELECTION (selection));

  manuals_sidebar_activate (self, gtk_single_selection_get_selected (selection));
}

static gboolean
manuals_sidebar_key_pressed_cb (ManualsSidebar        *self,
                                guint                  keyval,
                                guint                  keycode,
                                GdkModifierType        state,
                                GtkEventControllerKey *controller)
{
  g_assert (MANUALS_IS_SIDEBAR (self));
  g_assert (GTK_IS_EVENT_CONTROLLER_KEY (controller));

  switch (keyval)
    {
    case GDK_KEY_Down:
    case GDK_KEY_KP_Down:
      {
        GtkSelectionModel *model = gtk_list_view_get_model (self->search_view);
        guint selected = gtk_single_selection_get_selected (GTK_SINGLE_SELECTION (model));
        guint n_items = g_list_model_get_n_items (G_LIST_MODEL (model));

        if (n_items > 0 && selected + 1 == n_items)
          return GDK_EVENT_PROPAGATE;

        selected++;

        gtk_single_selection_set_selected (GTK_SINGLE_SELECTION (model), selected);
      }
      return GDK_EVENT_STOP;

    case GDK_KEY_Up:
    case GDK_KEY_KP_Up:
      {
        GtkSelectionModel *model = gtk_list_view_get_model (self->search_view);
        guint selected = gtk_single_selection_get_selected (GTK_SINGLE_SELECTION (model));

        if (selected == 0)
          return GDK_EVENT_PROPAGATE;

        if (selected == GTK_INVALID_LIST_POSITION)
          selected = 0;
        else
          selected--;

        gtk_single_selection_set_selected (GTK_SINGLE_SELECTION (model), selected);
      }
      return GDK_EVENT_STOP;

    default:
      return GDK_EVENT_PROPAGATE;
    }
}

static void
manuals_sidebar_search_activate_cb (ManualsSidebar *self,
                                    GtkSearchEntry *search_entry)
{
  GtkSelectionModel *model;
  guint position;

  g_assert (MANUALS_IS_SIDEBAR (self));
  g_assert (GTK_IS_SEARCH_ENTRY (search_entry));

  model = gtk_list_view_get_model (self->search_view);
  position = gtk_single_selection_get_selected (GTK_SINGLE_SELECTION (model));

  manuals_sidebar_activate (self, position);
}

static DexFuture *
manuals_sidebar_select_first (DexFuture *completed,
                              gpointer   user_data)
{
  ManualsSidebar *self = user_data;

  g_assert (MANUALS_IS_SIDEBAR (self));

  manuals_sidebar_activate (self, 0);

  return NULL;
}

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
      gtk_widget_set_visible (GTK_WIDGET (self->back_button), FALSE);
    }
  else
    {
      g_autoptr(ManualsSearchQuery) query = manuals_search_query_new ();
      g_autoptr(GtkSelectionModel) selection = NULL;

      manuals_search_query_set_text (query, text);

      /* Hold on to the future so we can cancel it by releasing when a
       * new search comes in.
       */
      selection = g_object_new (GTK_TYPE_SINGLE_SELECTION,
                                "autoselect", FALSE,
                                "can-unselect", TRUE,
                                "model", query,
                                NULL);
      g_signal_connect_object (selection,
                               "selection-changed",
                               G_CALLBACK (manuals_sidebar_selection_changed_cb),
                               self,
                               G_CONNECT_SWAPPED);
      gtk_list_view_set_model (self->search_view, selection);

      self->query = manuals_search_query_execute (query, self->repository);

      dex_future_disown (dex_future_then (dex_ref (self->query),
                                          manuals_sidebar_select_first,
                                          g_object_ref (self),
                                          g_object_unref));

      gtk_stack_set_visible_child_name (self->stack, "search");
      gtk_widget_set_visible (GTK_WIDGET (self->back_button), TRUE);
    }
}

static void
manuals_sidebar_search_view_activate_cb (ManualsSidebar *self,
                                         guint           position,
                                         GtkListView    *list_view)
{
  manuals_sidebar_activate (self, position);
}

static void
browse_action (GtkWidget  *widget,
               const char *action,
               GVariant   *param)
{
  ManualsSidebar *self = MANUALS_SIDEBAR (widget);

  gtk_editable_set_text (GTK_EDITABLE (self->search_entry), "");
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
  g_clear_object (&self->reveal);

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

  gtk_widget_class_bind_template_child (widget_class, ManualsSidebar, back_button);
  gtk_widget_class_bind_template_child (widget_class, ManualsSidebar, box);
  gtk_widget_class_bind_template_child (widget_class, ManualsSidebar, search_entry);
  gtk_widget_class_bind_template_child (widget_class, ManualsSidebar, search_view);
  gtk_widget_class_bind_template_child (widget_class, ManualsSidebar, stack);
  gtk_widget_class_bind_template_child (widget_class, ManualsSidebar, tree);

  gtk_widget_class_bind_template_callback (widget_class, manuals_sidebar_key_pressed_cb);
  gtk_widget_class_bind_template_callback (widget_class, manuals_sidebar_search_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, manuals_sidebar_search_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, manuals_sidebar_search_view_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, nonempty_to_boolean);
  gtk_widget_class_bind_template_callback (widget_class, lookup_sdk_title);

  gtk_widget_class_install_action (widget_class, "sidebar.browse", NULL, browse_action);

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
  g_type_ensure (IDE_TYPE_TREE);
}

static void
manuals_sidebar_init (ManualsSidebar *self)
{
  g_autoptr(ManualsTreeAddin) addin = NULL;

  gtk_widget_init_template (GTK_WIDGET (self));

  addin = g_object_new (MANUALS_TYPE_TREE_ADDIN, NULL);
  ide_tree_add_addin (self->tree, IDE_TREE_ADDIN (addin));
  ide_tree_invalidate_all (self->tree);
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
      g_autoptr(IdeTreeNode) root = ide_tree_node_new ();

      ide_tree_node_set_item (root, repository);
      ide_tree_set_root (self->tree, root);

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

static void
expand_node_cb (GObject      *object,
                GAsyncResult *result,
                gpointer      user_data)
{
  g_autoptr(DexPromise) promise = user_data;
  g_autoptr(GError) error = NULL;

  if (!ide_tree_expand_node_finish (IDE_TREE (object), result, &error))
    dex_promise_reject (promise, g_steal_pointer (&error));
  else
    dex_promise_resolve_boolean (promise, TRUE);
}

static DexFuture *
expand_node (IdeTree     *tree,
             IdeTreeNode *node)
{
  DexPromise *promise = dex_promise_new_cancellable ();
  ide_tree_expand_node_async (tree,
                              node,
                              dex_promise_get_cancellable (promise),
                              expand_node_cb,
                              dex_ref (promise));
  return DEX_FUTURE (promise);
}

static gboolean
node_matches (IdeTreeNode        *node,
              ManualsNavigatable *navigatable)
{
  gpointer node_item = ide_tree_node_get_item (node);
  gpointer nav_item = manuals_navigatable_get_item (navigatable);
  gint64 node_id = 0;
  gint64 nav_id = 0;

  if (node_item == nav_item)
    return TRUE;

  if (G_OBJECT_TYPE (node_item) != G_OBJECT_TYPE (nav_item))
    return FALSE;

  g_object_get (node_item, "id", &node_id, NULL);
  g_object_get (nav_item, "id", &nav_id, NULL);

  return node_id == nav_id;
}

static DexFuture *
manuals_sidebar_reveal_fiber (gpointer user_data)
{
  ManualsSidebar *self = user_data;
  g_autoptr(ManualsNavigatable) reveal = NULL;
  g_autoptr(GPtrArray) chain = NULL;
  ManualsNavigatable *parent;
  IdeTreeNode *node;

  g_assert (MANUALS_IS_SIDEBAR (self));

  if (!(reveal = g_steal_pointer (&self->reveal)))
    goto completed;

  chain = g_ptr_array_new_with_free_func (g_object_unref);
  parent = g_object_ref (reveal);

  while (parent != NULL)
    {
      g_ptr_array_insert (chain, 0, parent);
      parent = dex_await_object (manuals_navigatable_find_parent (parent), NULL);
    }

  /* repository is always index 0 */
  g_ptr_array_remove_index (chain, 0);

  node = ide_tree_get_root (self->tree);

  while (node != NULL && chain->len > 0)
    {
      g_autoptr(ManualsNavigatable) navigatable = g_object_ref (g_ptr_array_index (chain, 0));
      IdeTreeNode *child;
      gboolean found = FALSE;

      g_ptr_array_remove_index (chain, 0);

      dex_await (expand_node (self->tree, node), NULL);

      for (child = ide_tree_node_get_first_child (node);
           child != NULL;
           child = ide_tree_node_get_next_sibling (child))
        {
          if (node_matches (child, navigatable))
            {
              node = child;
              found = TRUE;
              break;
            }
        }

      if (!found)
        break;
    }

  if (node != NULL)
    {
      ide_tree_set_selected_node (self->tree, node);

      if (self->reveal_expand)
        ide_tree_expand_node (self->tree, node);
    }

  gtk_stack_set_visible_child_name (self->stack, "browse");

completed:
  return dex_future_new_for_boolean (TRUE);
}

void
manuals_sidebar_reveal (ManualsSidebar     *self,
                        ManualsNavigatable *navigatable,
                        gboolean            expand)
{
  g_return_if_fail (MANUALS_IS_SIDEBAR (self));
  g_return_if_fail (!navigatable || MANUALS_IS_NAVIGATABLE (navigatable));

  g_set_object (&self->reveal, navigatable);

  self->reveal_expand = !!expand;

  gtk_editable_set_text (GTK_EDITABLE (self->search_entry), "");
  gtk_stack_set_visible_child_name (self->stack, "browse");

  dex_future_disown (dex_scheduler_spawn (NULL, 0,
                                          manuals_sidebar_reveal_fiber,
                                          g_object_ref (self),
                                          g_object_unref));
}

void
manuals_sidebar_reload (ManualsSidebar *self)
{
  g_autoptr(IdeTreeNode) root = NULL;

  g_return_if_fail (MANUALS_IS_SIDEBAR (self));

  root = ide_tree_node_new ();
  ide_tree_node_set_item (root, self->repository);
  ide_tree_set_root (self->tree, root);
}

void
manuals_sidebar_set_enabled (ManualsSidebar *self,
                             gboolean        enabled)
{
  g_return_if_fail (MANUALS_IS_SIDEBAR (self));

  gtk_widget_set_sensitive (GTK_WIDGET (self->box), enabled);
}
