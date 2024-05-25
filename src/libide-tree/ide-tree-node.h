/* ide-tree-node.h
 *
 * Copyright 2018-2022 Christian Hergert <chergert@redhat.com>
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

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define IDE_TYPE_TREE_NODE (ide_tree_node_get_type())

typedef enum
{
  IDE_TREE_NODE_VISIT_BREAK    = 0,
  IDE_TREE_NODE_VISIT_CONTINUE = 0x1,
  IDE_TREE_NODE_VISIT_CHILDREN = 0x3,
} IdeTreeNodeVisit;

typedef enum
{
  IDE_TREE_NODE_FLAGS_NONE       = 0,
  IDE_TREE_NODE_FLAGS_DESCENDANT = 1 << 0,
  IDE_TREE_NODE_FLAGS_ADDED      = 1 << 1,
  IDE_TREE_NODE_FLAGS_CHANGED    = 1 << 2,
  IDE_TREE_NODE_FLAGS_REMOVED    = 1 << 3,
} IdeTreeNodeFlags;

#define IDE_TREE_NODE_FLAGS_VCS_MASK \
  (IDE_TREE_NODE_FLAGS_DESCENDANT |  \
   IDE_TREE_NODE_FLAGS_ADDED      |  \
   IDE_TREE_NODE_FLAGS_CHANGED    |  \
   IDE_TREE_NODE_FLAGS_REMOVED)

G_DECLARE_FINAL_TYPE (IdeTreeNode, ide_tree_node, IDE, TREE_NODE, GObject)

/**
 * IdeTreeTraverseFunc:
 * @node: an #IdeTreeNode
 * @user_data: closure data provided to ide_tree_node_traverse()
 *
 * This function prototype is used to traverse a tree of #IdeTreeNode.
 *
 * Returns: #IdeTreeNodeVisit, %IDE_TREE_NODE_VISIT_BREAK to stop traversal.
 */
typedef IdeTreeNodeVisit (*IdeTreeTraverseFunc) (IdeTreeNode *node,
                                                 gpointer     user_data);

/**
 * IdeTreeNodeCompare:
 * @node: an #IdeTreeNode that iterate over children
 * @child: an #IdeTreeNode to be inserted
 *

 * This callback function is a convenience wrapper around GCompareFunc
 *
 * Returns: int
 */
typedef int (*IdeTreeNodeCompare) (IdeTreeNode *node,
                                   IdeTreeNode *child);

IdeTreeNode      *ide_tree_node_new                    (void);
guint            ide_tree_node_get_n_children          (IdeTreeNode         *self);
const char       *ide_tree_node_get_title              (IdeTreeNode         *self);
void              ide_tree_node_set_title              (IdeTreeNode         *self,
                                                        const char          *title);
GIcon            *ide_tree_node_get_icon               (IdeTreeNode         *self);
void              ide_tree_node_set_icon               (IdeTreeNode         *self,
                                                        GIcon               *icon);
GIcon            *ide_tree_node_get_expanded_icon      (IdeTreeNode         *self);
void              ide_tree_node_set_expanded_icon      (IdeTreeNode         *self,
                                                        GIcon               *expanded_icon);
void              ide_tree_node_set_icon_name          (IdeTreeNode         *self,
                                                        const char          *icon_name);
void              ide_tree_node_set_expanded_icon_name (IdeTreeNode         *self,
                                                        const char          *expanded_icon_name);
IdeTreeNodeFlags  ide_tree_node_get_flags              (IdeTreeNode         *self);
void              ide_tree_node_set_flags              (IdeTreeNode         *self,
                                                        IdeTreeNodeFlags     flags);
gboolean          ide_tree_node_get_vcs_ignored        (IdeTreeNode         *self);
void              ide_tree_node_set_vcs_ignored        (IdeTreeNode         *self,
                                                        gboolean             ignored);
gboolean          ide_tree_node_get_has_error          (IdeTreeNode         *self);
void              ide_tree_node_set_has_error          (IdeTreeNode         *self,
                                                        gboolean             has_error);
gboolean          ide_tree_node_get_destroy_item       (IdeTreeNode         *self);
void              ide_tree_node_set_destroy_item       (IdeTreeNode         *self,
                                                        gboolean             destroy_item);
gpointer          ide_tree_node_get_item               (IdeTreeNode         *self);
void              ide_tree_node_set_item               (IdeTreeNode         *self,
                                                        gpointer             item);
gboolean          ide_tree_node_get_children_possible  (IdeTreeNode         *self);
void              ide_tree_node_set_children_possible  (IdeTreeNode         *self,
                                                        gboolean             children_possible);
gboolean          ide_tree_node_get_reset_on_collapse  (IdeTreeNode         *self);
void              ide_tree_node_set_reset_on_collapse  (IdeTreeNode         *self,
                                                        gboolean             reset_on_collapse);
gboolean          ide_tree_node_get_use_markup         (IdeTreeNode         *self);
void              ide_tree_node_set_use_markup         (IdeTreeNode         *self,
                                                        gboolean             use_markup);
gboolean          ide_tree_node_get_is_header          (IdeTreeNode         *self);
void              ide_tree_node_set_is_header          (IdeTreeNode         *self,
                                                        gboolean             is_header);
IdeTreeNode      *ide_tree_node_get_first_child        (IdeTreeNode         *self);
IdeTreeNode      *ide_tree_node_get_last_child         (IdeTreeNode         *self);
IdeTreeNode      *ide_tree_node_get_prev_sibling       (IdeTreeNode         *self);
IdeTreeNode      *ide_tree_node_get_next_sibling       (IdeTreeNode         *self);
void              ide_tree_node_set_parent             (IdeTreeNode         *self,
                                                        IdeTreeNode         *node);
void              ide_tree_node_remove                 (IdeTreeNode         *self,
                                                        IdeTreeNode         *child);
void              ide_tree_node_unparent               (IdeTreeNode         *self);
IdeTreeNode      *ide_tree_node_get_parent             (IdeTreeNode         *self);
IdeTreeNode      *ide_tree_node_get_root               (IdeTreeNode         *self);
gboolean          ide_tree_node_holds                  (IdeTreeNode         *self,
                                                        GType                type);
void              ide_tree_node_insert_after           (IdeTreeNode         *node,
                                                        IdeTreeNode         *parent,
                                                        IdeTreeNode         *previous_sibling);
void              ide_tree_node_insert_before          (IdeTreeNode         *node,
                                                        IdeTreeNode         *parent,
                                                        IdeTreeNode         *next_sibling);
void              ide_tree_node_insert_sorted          (IdeTreeNode         *self,
                                                        IdeTreeNode         *child,
                                                        IdeTreeNodeCompare   cmpfn);
void              ide_tree_node_traverse               (IdeTreeNode         *self,
                                                        GTraverseType        traverse_type,
                                                        GTraverseFlags       traverse_flags,
                                                        gint                 max_depth,
                                                        IdeTreeTraverseFunc  traverse_func,
                                                        gpointer             user_data);

G_END_DECLS
