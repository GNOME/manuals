/*
 * manuals-sdk-reference.c
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

#include "manuals-sdk-reference.h"

typedef struct
{
  char *title;
  char *subtitle;
  char **tags;
  guint installed : 1;
} ManualsSdkReferencePrivate;

enum {
  PROP_0,
  PROP_INSTALLED,
  PROP_SUBTITLE,
  PROP_TITLE,
  PROP_TAGS,
  N_PROPS
};

G_DEFINE_TYPE_WITH_PRIVATE (ManualsSdkReference, manuals_sdk_reference, G_TYPE_OBJECT)

static GParamSpec *properties[N_PROPS];

static DexFuture *
manuals_sdk_reference_real_install (ManualsSdkReference *self,
                                    ManualsProgress     *progress,
                                    GCancellable        *cancellable)
{
  return dex_future_new_reject (G_IO_ERROR,
                                G_IO_ERROR_NOT_SUPPORTED,
                                "Not supported");
}

static gboolean
manuals_sdk_reference_real_equal (ManualsSdkReference *self,
                                  ManualsSdkReference *other)
{
  return self == other;
}

static void
manuals_sdk_reference_dispose (GObject *object)
{
  ManualsSdkReference *self = (ManualsSdkReference *)object;
  ManualsSdkReferencePrivate *priv = manuals_sdk_reference_get_instance_private (self);

  g_clear_pointer (&priv->title, g_free);
  g_clear_pointer (&priv->subtitle, g_free);
  g_clear_pointer (&priv->tags, g_strfreev);

  G_OBJECT_CLASS (manuals_sdk_reference_parent_class)->dispose (object);
}

static void
manuals_sdk_reference_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  ManualsSdkReference *self = MANUALS_SDK_REFERENCE (object);
  ManualsSdkReferencePrivate *priv = manuals_sdk_reference_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_INSTALLED:
      g_value_set_boolean (value, priv->installed);
      break;

    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;

    case PROP_SUBTITLE:
      g_value_set_string (value, priv->subtitle);
      break;

    case PROP_TAGS:
      g_value_set_boxed (value, priv->tags);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
manuals_sdk_reference_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  ManualsSdkReference *self = MANUALS_SDK_REFERENCE (object);
  ManualsSdkReferencePrivate *priv = manuals_sdk_reference_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_INSTALLED:
      manuals_sdk_reference_set_installed (self, g_value_get_boolean (value));
      break;

    case PROP_TITLE:
      if (g_set_str (&priv->title, g_value_get_string (value)))
        g_object_notify_by_pspec (object, pspec);
      break;

    case PROP_SUBTITLE:
      if (g_set_str (&priv->subtitle, g_value_get_string (value)))
        g_object_notify_by_pspec (object, pspec);
      break;

    case PROP_TAGS:
      manuals_sdk_reference_set_tags (self, g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
manuals_sdk_reference_class_init (ManualsSdkReferenceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = manuals_sdk_reference_dispose;
  object_class->get_property = manuals_sdk_reference_get_property;
  object_class->set_property = manuals_sdk_reference_set_property;

  klass->equal = manuals_sdk_reference_real_equal;
  klass->install = manuals_sdk_reference_real_install;

  properties[PROP_INSTALLED] =
    g_param_spec_boolean ("installed", NULL, NULL,
                         FALSE,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_SUBTITLE] =
    g_param_spec_string ("subtitle", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_TAGS] =
    g_param_spec_boxed ("tags", NULL, NULL,
                        G_TYPE_STRV,
                        (G_PARAM_READWRITE |
                         G_PARAM_EXPLICIT_NOTIFY |
                         G_PARAM_STATIC_STRINGS));

  properties[PROP_TITLE] =
    g_param_spec_string ("title", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
manuals_sdk_reference_init (ManualsSdkReference *self)
{
}

const char *
manuals_sdk_reference_get_title (ManualsSdkReference *self)
{
  ManualsSdkReferencePrivate *priv = manuals_sdk_reference_get_instance_private (self);

  g_return_val_if_fail (MANUALS_IS_SDK_REFERENCE (self), NULL);

  return priv->title;
}

const char *
manuals_sdk_reference_get_subtitle (ManualsSdkReference *self)
{
  ManualsSdkReferencePrivate *priv = manuals_sdk_reference_get_instance_private (self);

  g_return_val_if_fail (MANUALS_IS_SDK_REFERENCE (self), NULL);

  return priv->subtitle;
}

gboolean
manuals_sdk_reference_equal (ManualsSdkReference *self,
                             ManualsSdkReference *other)
{
  return MANUALS_SDK_REFERENCE_GET_CLASS (self)->equal (self, other);
}

gboolean
manuals_sdk_reference_get_installed (ManualsSdkReference *self)
{
  ManualsSdkReferencePrivate *priv = manuals_sdk_reference_get_instance_private (self);

  g_return_val_if_fail (MANUALS_IS_SDK_REFERENCE (self), FALSE);

  return priv->installed;
}

void
manuals_sdk_reference_set_installed (ManualsSdkReference *self,
                                     gboolean             installed)
{
  ManualsSdkReferencePrivate *priv = manuals_sdk_reference_get_instance_private (self);

  g_return_if_fail (MANUALS_IS_SDK_REFERENCE (self));

  installed = !!installed;

  if (priv->installed != installed)
    {
      priv->installed = installed;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INSTALLED]);
    }
}

const char * const *
manuals_sdk_reference_get_tags (ManualsSdkReference *self)
{
  ManualsSdkReferencePrivate *priv = manuals_sdk_reference_get_instance_private (self);

  g_return_val_if_fail (MANUALS_IS_SDK_REFERENCE (self), NULL);

  return (const char * const *)priv->tags;
}

void
manuals_sdk_reference_set_tags (ManualsSdkReference *self,
                                const char * const  *tags)
{
  ManualsSdkReferencePrivate *priv = manuals_sdk_reference_get_instance_private (self);
  char **copy;

  g_return_if_fail (MANUALS_IS_SDK_REFERENCE (self));

  copy = g_strdupv ((char **)tags);
  g_free (priv->tags);
  priv->tags = copy;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TAGS]);
}

DexFuture *
manuals_sdk_reference_install (ManualsSdkReference *self,
                               ManualsProgress     *progress,
                               GCancellable        *cancellable)
{
  g_return_val_if_fail (MANUALS_IS_SDK_REFERENCE (self), NULL);
  g_return_val_if_fail (MANUALS_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable), NULL);

  return MANUALS_SDK_REFERENCE_GET_CLASS (self)->install (self, progress, cancellable);
}
