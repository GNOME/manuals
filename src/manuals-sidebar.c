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
#include "manuals-tree-expander.h"

struct _ManualsSidebar
{
  GtkWidget          parent_instance;

  GtkListView       *list_view;

  ManualsRepository *repository;
};

G_DEFINE_FINAL_TYPE (ManualsSidebar, manuals_sidebar, GTK_TYPE_WIDGET)

enum {
  PROP_0,
  PROP_REPOSITORY,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

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
  gtk_widget_class_bind_template_child (widget_class, ManualsSidebar, list_view);

  properties[PROP_REPOSITORY] =
    g_param_spec_object ("repository", NULL, NULL,
                         MANUALS_TYPE_REPOSITORY,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  g_type_ensure (MANUALS_TYPE_TREE_EXPANDER);
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
