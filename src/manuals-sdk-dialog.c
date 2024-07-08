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

#include <glib/gi18n.h>

#include "manuals-install-button.h"
#include "manuals-sdk-dialog.h"
#include "manuals-tag.h"

struct _ManualsSdkDialog
{
  AdwPreferencesDialog  parent_instance;

  GListStore           *installers;

  GtkFilterListModel   *installed;
  AdwPreferencesGroup  *installed_group;
  GtkListBox           *installed_list_box;
  GtkFilterListModel   *available;
  AdwPreferencesGroup  *available_group;
  GtkListBox           *available_list_box;
  GtkStack             *stack;
};

G_DEFINE_FINAL_TYPE (ManualsSdkDialog, manuals_sdk_dialog, ADW_TYPE_PREFERENCES_DIALOG)

static DexFuture *
manuals_sdk_dialog_cancel (DexFuture *completed,
                           gpointer   user_data)
{
  manuals_install_button_cancel (MANUALS_INSTALL_BUTTON (user_data));
  return NULL;
}

static DexFuture *
manuals_sdk_dialog_ensure_done (DexFuture *completed,
                                gpointer   user_data)
{
  manuals_progress_done (MANUALS_PROGRESS (user_data));
  return NULL;
}

static void
manuals_sdk_dialog_install_cb (ManualsSdkReference  *reference,
                               ManualsProgress      *progress,
                               GCancellable         *cancellable,
                               ManualsInstallButton *button)
{
  DexFuture *future;

  g_assert (MANUALS_IS_SDK_REFERENCE (reference));
  g_assert (MANUALS_IS_PROGRESS (progress));
  g_assert (G_IS_CANCELLABLE (cancellable));
  g_assert (MANUALS_IS_INSTALL_BUTTON (button));

  future = manuals_sdk_reference_install (reference, progress, cancellable);
  future = dex_future_finally (future,
                               manuals_sdk_dialog_cancel,
                               g_object_ref (button),
                               g_object_unref);
  future = dex_future_finally (future,
                               manuals_sdk_dialog_ensure_done,
                               g_object_ref (progress),
                               g_object_unref);

  dex_future_disown (future);
}

static void
manuals_sdk_dialog_cancel_cb (ManualsSdkReference  *reference,
                              ManualsProgress      *progress,
                              GCancellable         *cancellable,
                              ManualsInstallButton *button)
{
  g_assert (MANUALS_IS_SDK_REFERENCE (reference));
  g_assert (!progress || MANUALS_IS_PROGRESS (progress));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));
  g_assert (MANUALS_IS_INSTALL_BUTTON (button));

  g_cancellable_cancel (cancellable);
}

static GtkWidget *
create_sdk_row (gpointer item,
                gpointer user_data)
{
  ManualsSdkReference *reference = item;
  const char * const *tags;
  const char *subtitle;
  const char *title;
  GtkWidget *row;

  g_assert (MANUALS_IS_SDK_REFERENCE (reference));
  g_assert (MANUALS_IS_SDK_DIALOG (user_data));

  title = manuals_sdk_reference_get_title (reference);
  subtitle = manuals_sdk_reference_get_subtitle (reference);
  tags = manuals_sdk_reference_get_tags (reference);

  row = g_object_new (ADW_TYPE_ACTION_ROW,
                      "title", title,
                      "subtitle", subtitle,
                      NULL);

  if (tags != NULL)
    {
      for (guint i = 0; tags[i]; i++)
        adw_action_row_add_suffix (ADW_ACTION_ROW (row),
                                   g_object_new (MANUALS_TYPE_TAG,
                                                 "css-classes", (const char * const []) { "installation", NULL },
                                                 "value", tags[i],
                                                 "valign", GTK_ALIGN_CENTER,
                                                 NULL));
    }

  if (!manuals_sdk_reference_get_installed (reference))
    {
      GtkWidget *button;

      button = g_object_new (MANUALS_TYPE_INSTALL_BUTTON,
                             "label", _("Install"),
                             "valign", GTK_ALIGN_CENTER,
                             NULL);
      g_signal_connect_object (button,
                               "install",
                               G_CALLBACK (manuals_sdk_dialog_install_cb),
                               reference,
                               G_CONNECT_SWAPPED);
      g_signal_connect_object (button,
                               "cancel",
                               G_CALLBACK (manuals_sdk_dialog_cancel_cb),
                               reference,
                               G_CONNECT_SWAPPED);

      adw_action_row_add_suffix (ADW_ACTION_ROW (row), button);
      adw_action_row_set_activatable_widget (ADW_ACTION_ROW (row), button);
    }

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
installed_items_changed_cb (ManualsSdkDialog *self,
                            guint             position,
                            guint             removed,
                            guint             added,
                            GListModel       *model)
{
  guint n_items;

  g_assert (MANUALS_IS_SDK_DIALOG (self));
  g_assert (G_IS_LIST_MODEL (model));

  n_items = g_list_model_get_n_items (model);

  gtk_widget_set_visible (GTK_WIDGET (self->installed_group), !!n_items);
}

static void
available_items_changed_cb (ManualsSdkDialog *self,
                            guint             position,
                            guint             removed,
                            guint             added,
                            GListModel       *model)
{
  guint n_items;

  g_assert (MANUALS_IS_SDK_DIALOG (self));
  g_assert (G_IS_LIST_MODEL (model));

  n_items = g_list_model_get_n_items (model);

  gtk_widget_set_visible (GTK_WIDGET (self->available_group), !!n_items);
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
  gtk_widget_class_bind_template_child (widget_class, ManualsSdkDialog, available_group);
  gtk_widget_class_bind_template_child (widget_class, ManualsSdkDialog, available_list_box);
  gtk_widget_class_bind_template_child (widget_class, ManualsSdkDialog, installed);
  gtk_widget_class_bind_template_child (widget_class, ManualsSdkDialog, installed_group);
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

  g_signal_connect_object (self->installed,
                           "items-changed",
                           G_CALLBACK (installed_items_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (self->available,
                           "items-changed",
                           G_CALLBACK (available_items_changed_cb),
                           self,
                           G_CONNECT_SWAPPED);

  gtk_list_box_bind_model (self->installed_list_box,
                           G_LIST_MODEL (self->installed),
                           create_sdk_row, self, NULL);
  gtk_list_box_bind_model (self->available_list_box,
                           G_LIST_MODEL (self->available),
                           create_sdk_row, self, NULL);

  installed_items_changed_cb (self, 0, 0, 0, G_LIST_MODEL (self->installed));
  available_items_changed_cb (self, 0, 0, 0, G_LIST_MODEL (self->available));
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
  guint n_items;

  g_assert (MANUALS_IS_SDK_DIALOG (self));

  n_items = g_list_model_get_n_items (G_LIST_MODEL (self->available))
          + g_list_model_get_n_items (G_LIST_MODEL (self->installed));

  gtk_stack_set_visible_child_name (self->stack,
                                    n_items ? "list" : "empty");

  return NULL;
}

void
manuals_sdk_dialog_present (ManualsSdkDialog *self,
                            GtkWidget        *parent)
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

  adw_dialog_present (ADW_DIALOG (self), parent);
}
