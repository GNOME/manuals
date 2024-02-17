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

#include "manuals-sidebar.h"

struct _ManualsSidebar
{
  GtkWidget   parent_instance;

  GtkListBox *list_box;
};

G_DEFINE_FINAL_TYPE (ManualsSidebar, manuals_sidebar, GTK_TYPE_WIDGET)

static void
manuals_sidebar_dispose (GObject *object)
{
  ManualsSidebar *self = (ManualsSidebar *)object;
  GtkWidget *child;

  gtk_widget_dispose_template (GTK_WIDGET (self), MANUALS_TYPE_SIDEBAR);

  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self))))
    gtk_widget_unparent (child);

  G_OBJECT_CLASS (manuals_sidebar_parent_class)->dispose (object);
}

static void
manuals_sidebar_class_init (ManualsSidebarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = manuals_sidebar_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Manuals/manuals-sidebar.ui");
  gtk_widget_class_set_css_name (widget_class, "sidebar");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_bind_template_child (widget_class, ManualsSidebar, list_box);
}

static void
manuals_sidebar_init (ManualsSidebar *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
