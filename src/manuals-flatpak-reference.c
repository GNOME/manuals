/*
 * manuals-flatpak-reference.c
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

#include "manuals-flatpak.h"
#include "manuals-flatpak-reference.h"

struct _ManualsFlatpakReference
{
  ManualsSdkReference parent_instance;
  FlatpakInstallation *installation;
  FlatpakRef *ref;
};

enum {
  PROP_0,
  PROP_INSTALLATION,
  PROP_REF,
  N_PROPS
};

G_DEFINE_FINAL_TYPE (ManualsFlatpakReference, manuals_flatpak_reference, MANUALS_TYPE_SDK_REFERENCE)

static GParamSpec *properties[N_PROPS];

static DexFuture *
manuals_flatpak_reference_installed_cb (DexFuture *completed,
                                        gpointer   user_data)
{
  ManualsFlatpakReference *self = user_data;
  manuals_sdk_reference_set_installed (MANUALS_SDK_REFERENCE (self), TRUE);
  return dex_ref (completed);
}

static DexFuture *
manuals_flatpak_reference_install (ManualsSdkReference *reference,
                                   ManualsProgress     *progress,
                                   GCancellable        *cancellable)
{
  ManualsFlatpakReference *self = (ManualsFlatpakReference *)reference;
  DexFuture *future;

  g_assert (MANUALS_IS_FLATPAK_REFERENCE (self));
  g_assert (MANUALS_IS_PROGRESS (progress));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  future = manuals_flatpak_installation_install (self->installation,
                                                 FLATPAK_REMOTE_REF (self->ref),
                                                 progress,
                                                 cancellable);
  future = dex_future_then (future,
                            manuals_flatpak_reference_installed_cb,
                            g_object_ref (self),
                            g_object_unref);

  return future;
}

static void
manuals_flatpak_reference_dispose (GObject *object)
{
  ManualsFlatpakReference *self = (ManualsFlatpakReference *)object;

  g_clear_object (&self->installation);
  g_clear_object (&self->ref);

  G_OBJECT_CLASS (manuals_flatpak_reference_parent_class)->dispose (object);
}

static void
manuals_flatpak_reference_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  ManualsFlatpakReference *self = MANUALS_FLATPAK_REFERENCE (object);

  switch (prop_id)
    {
    case PROP_INSTALLATION:
      g_value_set_object (value, self->installation);
      break;

    case PROP_REF:
      g_value_set_object (value, self->ref);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
manuals_flatpak_reference_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  ManualsFlatpakReference *self = MANUALS_FLATPAK_REFERENCE (object);

  switch (prop_id)
    {
    case PROP_INSTALLATION:
      self->installation = g_value_dup_object (value);
      break;

    case PROP_REF:
      self->ref = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
manuals_flatpak_reference_class_init (ManualsFlatpakReferenceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ManualsSdkReferenceClass *sdk_reference_class = MANUALS_SDK_REFERENCE_CLASS (klass);

  object_class->dispose = manuals_flatpak_reference_dispose;
  object_class->get_property = manuals_flatpak_reference_get_property;
  object_class->set_property = manuals_flatpak_reference_set_property;

  sdk_reference_class->install = manuals_flatpak_reference_install;

  properties[PROP_REF] =
    g_param_spec_object ("ref", NULL, NULL,
                         FLATPAK_TYPE_REF,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_INSTALLATION] =
    g_param_spec_object ("installation", NULL, NULL,
                         FLATPAK_TYPE_INSTALLATION,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));
  
  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
manuals_flatpak_reference_init (ManualsFlatpakReference *self)
{
}

ManualsFlatpakReference *
manuals_flatpak_reference_new (FlatpakInstallation *installation,
                               FlatpakRef          *ref)
{
  g_autoptr(FlatpakInstalledRef) installed = NULL;
  g_autofree char *title = NULL;
  g_autofree char *subtitle = NULL;
  const char *name;
  const char *arch;
  const char *branch;

  g_return_val_if_fail (FLATPAK_IS_INSTALLATION (installation), NULL);
  g_return_val_if_fail (FLATPAK_IS_REF (ref), NULL);

  name = flatpak_ref_get_name (ref);
  arch = flatpak_ref_get_arch (ref);
  branch = flatpak_ref_get_branch (ref);

  /* Check if this is installed already */
  installed = flatpak_installation_get_installed_ref (installation,
                                                      FLATPAK_REF_KIND_RUNTIME,
                                                      name, arch, branch,
                                                      NULL, NULL);

  if (g_str_equal (name, "org.gnome.Sdk.Docs"))
    {
      title = g_strdup_printf (_("GNOME %s"), branch);
      subtitle = g_strdup_printf (_("Documentation for the GNOME %s SDK"),
                                  branch);
    }
  else if (g_str_equal (name, "org.freedesktop.Sdk.Docs"))
    {
      title = g_strdup_printf (_("FreeDesktop %s"), branch);
    }
  else if (g_str_equal (name, "org.kde.Sdk.Docs"))
    {
      title = g_strdup_printf (_("KDE %s"), branch);
    }
  else
    {
      title = g_strdup_printf ("%s %s", name, branch);
    }

  return g_object_new (MANUALS_TYPE_FLATPAK_REFERENCE,
                       "title", title,
                       "subtitle", subtitle,
                       "installed", installed != NULL,
                       "installation", installation,
                       "ref", ref,
                       NULL);
}
