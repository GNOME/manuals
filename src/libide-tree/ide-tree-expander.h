/* ide-tree-expander.h
 *
 * Copyright 2022 Christian Hergert <chergert@redhat.com>
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

#define IDE_TYPE_TREE_EXPANDER (ide_tree_expander_get_type())

G_DECLARE_FINAL_TYPE (IdeTreeExpander, ide_tree_expander, IDE, TREE_EXPANDER, GtkWidget)

GtkWidget      *ide_tree_expander_new                    (void);
GMenuModel     *ide_tree_expander_get_menu_model         (IdeTreeExpander *self);
void            ide_tree_expander_set_menu_model         (IdeTreeExpander *self,
                                                          GMenuModel      *menu_model);
GIcon          *ide_tree_expander_get_icon               (IdeTreeExpander *self);
void            ide_tree_expander_set_icon               (IdeTreeExpander *self,
                                                          GIcon           *icon);
void            ide_tree_expander_set_icon_name          (IdeTreeExpander *self,
                                                          const char      *icon_name);
GIcon          *ide_tree_expander_get_expanded_icon      (IdeTreeExpander *self);
void            ide_tree_expander_set_expanded_icon      (IdeTreeExpander *self,
                                                          GIcon           *icon);
void            ide_tree_expander_set_expanded_icon_name (IdeTreeExpander *self,
                                                          const char      *expanded_icon_name);
const char     *ide_tree_expander_get_title              (IdeTreeExpander *self);
void            ide_tree_expander_set_title              (IdeTreeExpander *self,
                                                          const char      *title);
gboolean        ide_tree_expander_get_ignored            (IdeTreeExpander *self);
void            ide_tree_expander_set_ignored            (IdeTreeExpander *self,
                                                          gboolean         ignored);
GtkWidget      *ide_tree_expander_get_suffix             (IdeTreeExpander *self);
void            ide_tree_expander_set_suffix             (IdeTreeExpander *self,
                                                          GtkWidget       *suffix);
GtkTreeListRow *ide_tree_expander_get_list_row           (IdeTreeExpander *self);
void            ide_tree_expander_set_list_row           (IdeTreeExpander *self,
                                                          GtkTreeListRow  *list_row);
gpointer        ide_tree_expander_get_item               (IdeTreeExpander *self);
gboolean        ide_tree_expander_get_use_markup         (IdeTreeExpander *self);
void            ide_tree_expander_set_use_markup         (IdeTreeExpander *self,
                                                          gboolean         use_markup);
void            ide_tree_expander_show_popover           (IdeTreeExpander *self,
                                                          GtkPopover      *popover);

G_END_DECLS
