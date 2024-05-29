/*
 * manuals-sdk-dialog.c
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

#include "manuals-sdk-dialog.h"

struct _ManualsSdkDialog
{
  AdwPreferencesWindow  parent_instance;

  GListStore           *installers;

  GtkFilterListModel   *installed;
  GtkListBox           *installed_list_box;
  GtkFilterListModel   *available;
  GtkListBox           *available_list_box;
  GtkStack             *stack;
};

G_DEFINE_FINAL_TYPE (ManualsSdkDialog, manuals_sdk_dialog, ADW_TYPE_PREFERENCES_WINDOW)

static GtkWidget *
create_sdk_row (gpointer item,
                gpointer user_data)
{
  ManualsSdkReference *reference = item;
  const char *subtitle;
  const char *title;
  GtkWidget *row;

  g_assert (MANUALS_IS_SDK_REFERENCE (reference));
  g_assert (MANUALS_IS_SDK_DIALOG (user_data));

  title = manuals_sdk_reference_get_title (reference);
  subtitle = manuals_sdk_reference_get_subtitle (reference);

  row = g_object_new (ADW_TYPE_ACTION_ROW,
                      "title", title,
                      "subtitle", subtitle,
                      NULL);

  g_object_set_data_full (G_OBJECT (row),
                          "MANUALS_SDK_REFERENCE",
                          g_object_ref (reference),
                          g_object_unref);

  return row;
}

static int
ref_sorter (gconstpointer a,
            gconstpointer b,
            gpointer      user_data)
{
  ManualsSdkReference *ref_a = (gpointer)a;
  ManualsSdkReference *ref_b = (gpointer)b;
  const char *title_a = manuals_sdk_reference_get_title (ref_a);
  const char *title_b = manuals_sdk_reference_get_title (ref_b);
  gboolean a_is_gnome = strstr (title_a, "GNOME") != NULL;
  gboolean b_is_gnome = strstr (title_b, "GNOME") != NULL;

  if (a_is_gnome && !b_is_gnome)
    return -1;

  if (!a_is_gnome && b_is_gnome)
    return 1;

  return g_strcmp0 (title_a, title_b);
}

static void
manuals_sdk_dialog_dispose (GObject *object)
{
  ManualsSdkDialog *self = (ManualsSdkDialog *)object;

  gtk_widget_dispose_template (GTK_WIDGET (self), MANUALS_TYPE_SDK_DIALOG);

  g_clear_object (&self->installers);

  G_OBJECT_CLASS (manuals_sdk_dialog_parent_class)->dispose (object);
}

static void
manuals_sdk_dialog_class_init (ManualsSdkDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = manuals_sdk_dialog_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/app/devsuite/Manuals/manuals-sdk-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, ManualsSdkDialog, available);
  gtk_widget_class_bind_template_child (widget_class, ManualsSdkDialog, available_list_box);
  gtk_widget_class_bind_template_child (widget_class, ManualsSdkDialog, installed);
  gtk_widget_class_bind_template_child (widget_class, ManualsSdkDialog, installed_list_box);
  gtk_widget_class_bind_template_child (widget_class, ManualsSdkDialog, stack);

  g_type_ensure (MANUALS_TYPE_SDK_REFERENCE);
}

static void
manuals_sdk_dialog_init (ManualsSdkDialog *self)
{
  g_autoptr(GtkFlattenListModel) flatten = NULL;
  g_autoptr(GtkSortListModel) sorted = NULL;
  g_autoptr(GtkCustomSorter) sorter = NULL;

  self->installers = g_list_store_new (MANUALS_TYPE_SDK_INSTALLER);

  gtk_widget_init_template (GTK_WIDGET (self));

  flatten = gtk_flatten_list_model_new (NULL);
  gtk_flatten_list_model_set_model (flatten, G_LIST_MODEL (self->installers));

  sorted = gtk_sort_list_model_new (NULL, NULL);
  sorter = gtk_custom_sorter_new (ref_sorter, NULL, NULL);
  gtk_sort_list_model_set_model (sorted, G_LIST_MODEL (flatten));
  gtk_sort_list_model_set_sorter (sorted, GTK_SORTER (sorter));

  gtk_filter_list_model_set_model (self->installed, G_LIST_MODEL (sorted));
  gtk_filter_list_model_set_model (self->available, G_LIST_MODEL (sorted));

  gtk_list_box_bind_model (self->installed_list_box,
                           G_LIST_MODEL (self->installed),
                           create_sdk_row, self, NULL);
  gtk_list_box_bind_model (self->available_list_box,
                           G_LIST_MODEL (self->available),
                           create_sdk_row, self, NULL);
}

ManualsSdkDialog *
manuals_sdk_dialog_new (void)
{
  return g_object_new (MANUALS_TYPE_SDK_DIALOG, NULL);
}

void
manuals_sdk_dialog_add_installer (ManualsSdkDialog    *self,
                                  ManualsSdkInstaller *installer)
{
  g_return_if_fail (MANUALS_IS_SDK_DIALOG (self));
  g_return_if_fail (MANUALS_IS_SDK_INSTALLER (installer));

  g_list_store_append (self->installers, installer);
}

static DexFuture *
manuals_sdk_dialog_present_cb (DexFuture *completed,
                               gpointer   user_data)
{
  ManualsSdkDialog *self = user_data;

  g_assert (MANUALS_IS_SDK_DIALOG (self));

  gtk_stack_set_visible_child_name (self->stack, "list");

  return NULL;
}

void
manuals_sdk_dialog_present (ManualsSdkDialog *self)
{
  g_autoptr(GPtrArray) futures = NULL;
  GListModel *model;
  DexFuture *future;
  guint n_items;

  g_return_if_fail (MANUALS_IS_SDK_DIALOG (self));

  model = G_LIST_MODEL (self->installers);
  n_items = g_list_model_get_n_items (model);
  futures = g_ptr_array_new_with_free_func (dex_unref);

  g_assert (n_items > 0);

  for (guint i = 0; i < n_items; i++)
    {
      g_autoptr(ManualsSdkInstaller) installer = g_list_model_get_item (model, i);

      g_ptr_array_add (futures,
                       manuals_sdk_installer_load (installer));
    }

  future = dex_future_allv ((DexFuture **)futures->pdata, futures->len);
  future = dex_future_finally (future,
                               manuals_sdk_dialog_present_cb,
                               g_object_ref (self),
                               g_object_unref);
  dex_future_disown (future);

  gtk_window_present (GTK_WINDOW (self));
}
