/*
 * manuals-sdk-installer.c
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

#include "manuals-sdk-installer.h"

typedef struct
{
  char *title;
  GListStore *refs;
} ManualsSdkInstallerPrivate;

enum {
  PROP_0,
  PROP_TITLE,
  N_PROPS
};

static void list_model_iface_init (GListModelInterface *iface);

G_DEFINE_TYPE_WITH_CODE (ManualsSdkInstaller, manuals_sdk_installer, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (ManualsSdkInstaller)
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, list_model_iface_init))

static GParamSpec *properties[N_PROPS];

static DexFuture *
manuals_sdk_installer_real_load (ManualsSdkInstaller *self)
{
  return dex_future_new_for_boolean (TRUE);
}

static void
manuals_sdk_installer_dispose (GObject *object)
{
  ManualsSdkInstaller *self = (ManualsSdkInstaller *)object;
  ManualsSdkInstallerPrivate *priv = manuals_sdk_installer_get_instance_private (self);

  g_clear_object (&priv->refs);
  g_clear_pointer (&priv->title, g_free);

  G_OBJECT_CLASS (manuals_sdk_installer_parent_class)->dispose (object);
}

static void
manuals_sdk_installer_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  ManualsSdkInstaller *self = MANUALS_SDK_INSTALLER (object);
  ManualsSdkInstallerPrivate *priv = manuals_sdk_installer_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
manuals_sdk_installer_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  ManualsSdkInstaller *self = MANUALS_SDK_INSTALLER (object);
  ManualsSdkInstallerPrivate *priv = manuals_sdk_installer_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_TITLE:
      if (g_set_str (&priv->title, g_value_get_string (value)))
        g_object_notify_by_pspec (object, pspec);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
manuals_sdk_installer_class_init (ManualsSdkInstallerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = manuals_sdk_installer_dispose;
  object_class->get_property = manuals_sdk_installer_get_property;
  object_class->set_property = manuals_sdk_installer_set_property;

  klass->load = manuals_sdk_installer_real_load;

  properties[PROP_TITLE] =
    g_param_spec_string ("title", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
manuals_sdk_installer_init (ManualsSdkInstaller *self)
{
  ManualsSdkInstallerPrivate *priv = manuals_sdk_installer_get_instance_private (self);

  priv->refs = g_list_store_new (MANUALS_TYPE_SDK_REFERENCE);
  g_signal_connect_object (priv->refs,
                           "items-changed",
                           G_CALLBACK (g_list_model_items_changed),
                           self,
                           G_CONNECT_SWAPPED);
}

DexFuture *
manuals_sdk_installer_load (ManualsSdkInstaller *self)
{
  g_return_val_if_fail (MANUALS_IS_SDK_INSTALLER (self), NULL);

  return MANUALS_SDK_INSTALLER_GET_CLASS (self)->load (self);
}

static void
manuals_sdk_installer_notify_installed_cb (ManualsSdkInstaller *self,
                                           GParamSpec          *pspec,
                                           ManualsSdkReference *reference)
{
  g_assert (MANUALS_IS_SDK_INSTALLER (self));
  g_assert (MANUALS_IS_SDK_REFERENCE (reference));

  g_object_ref (reference);
  manuals_sdk_installer_remove (self, reference);
  manuals_sdk_installer_add (self, reference);
  g_object_unref (reference);
}

void
manuals_sdk_installer_add (ManualsSdkInstaller *self,
                           ManualsSdkReference *ref)
{
  ManualsSdkInstallerPrivate *priv = manuals_sdk_installer_get_instance_private (self);

  g_return_if_fail (MANUALS_IS_SDK_INSTALLER (self));
  g_return_if_fail (MANUALS_IS_SDK_REFERENCE (ref));

  g_signal_connect_object (ref,
                           "notify::installed",
                           G_CALLBACK (manuals_sdk_installer_notify_installed_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_list_store_append (priv->refs, ref);
}

void
manuals_sdk_installer_remove (ManualsSdkInstaller *self,
                              ManualsSdkReference *ref)
{
  ManualsSdkInstallerPrivate *priv = manuals_sdk_installer_get_instance_private (self);
  guint n_items;

  g_return_if_fail (MANUALS_IS_SDK_INSTALLER (self));
  g_return_if_fail (MANUALS_IS_SDK_REFERENCE (ref));

  g_signal_handlers_disconnect_by_func (ref,
                                        G_CALLBACK (manuals_sdk_installer_notify_installed_cb),
                                        self);

  n_items = g_list_model_get_n_items (G_LIST_MODEL (priv->refs));

  for (guint i = 0; i < n_items; i++)
    {
      g_autoptr(ManualsSdkReference) element = g_list_model_get_item (G_LIST_MODEL (priv->refs), i);

      if (manuals_sdk_reference_equal (element, ref))
        {
          g_list_store_remove (priv->refs, i);
          break;
        }
    }
}

static GType
manuals_sdk_installer_get_item_type (GListModel *model)
{
  return MANUALS_TYPE_SDK_REFERENCE;
}

static guint
manuals_sdk_installer_get_n_items (GListModel *model)
{
  ManualsSdkInstaller *self = MANUALS_SDK_INSTALLER (model);
  ManualsSdkInstallerPrivate *priv = manuals_sdk_installer_get_instance_private (self);

  return g_list_model_get_n_items (G_LIST_MODEL (priv->refs));
}

static gpointer
manuals_sdk_installer_get_item (GListModel *model,
                                guint       position)
{
  ManualsSdkInstaller *self = MANUALS_SDK_INSTALLER (model);
  ManualsSdkInstallerPrivate *priv = manuals_sdk_installer_get_instance_private (self);

  return g_list_model_get_item (G_LIST_MODEL (priv->refs), position);
}

static void
list_model_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = manuals_sdk_installer_get_item_type;
  iface->get_n_items = manuals_sdk_installer_get_n_items;
  iface->get_item = manuals_sdk_installer_get_item;
}
