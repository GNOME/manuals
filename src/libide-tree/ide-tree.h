/* ide-tree.h
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

#include "ide-tree-node.h"

G_BEGIN_DECLS

#define IDE_TYPE_TREE (ide_tree_get_type())

typedef struct _IdeTreeAddin IdeTreeAddin;

G_DECLARE_DERIVABLE_TYPE (IdeTree, ide_tree, IDE, TREE, GtkWidget)

struct _IdeTreeClass
{
  GtkWidgetClass parent_class;
};

IdeTree     *ide_tree_new                  (void);
IdeTreeNode *ide_tree_get_root             (IdeTree              *self);
void         ide_tree_set_root             (IdeTree              *self,
                                            IdeTreeNode          *root);
GMenuModel  *ide_tree_get_menu_model       (IdeTree              *self);
void         ide_tree_set_menu_model       (IdeTree              *self,
                                            GMenuModel           *menu_model);
void         ide_tree_show_popover_at_node (IdeTree              *self,
                                            IdeTreeNode          *node,
                                            GtkPopover           *popover);
gboolean     ide_tree_is_node_expanded     (IdeTree              *self,
                                            IdeTreeNode          *node);
void         ide_tree_collapse_node        (IdeTree              *self,
                                            IdeTreeNode          *node);
void         ide_tree_expand_to_node       (IdeTree              *self,
                                            IdeTreeNode          *node);
void         ide_tree_expand_node          (IdeTree              *self,
                                            IdeTreeNode          *node);
void         ide_tree_expand_node_async    (IdeTree              *self,
                                            IdeTreeNode          *node,
                                            GCancellable         *cancellable,
                                            GAsyncReadyCallback   callback,
                                            gpointer              user_data);
gboolean     ide_tree_expand_node_finish   (IdeTree              *self,
                                            GAsyncResult         *result,
                                            GError              **error);
IdeTreeNode *ide_tree_get_selected_node    (IdeTree              *self);
void         ide_tree_set_selected_node    (IdeTree              *self,
                                            IdeTreeNode          *node);
void         ide_tree_invalidate_all       (IdeTree              *self);
void         ide_tree_add_addin            (IdeTree              *self,
                                            IdeTreeAddin         *addin);
void         ide_tree_remove_addin         (IdeTree              *self,
                                            IdeTreeAddin         *addin);

G_END_DECLS
