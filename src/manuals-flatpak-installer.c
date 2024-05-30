/*
 * manuals-flatpak-installer.c
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
#include "manuals-flatpak-installer.h"
#include "manuals-flatpak-reference.h"

struct _ManualsFlatpakInstaller
{
  ManualsSdkInstaller parent_instance;
  GPtrArray *installations;
  DexFuture *load;
};

G_DEFINE_FINAL_TYPE (ManualsFlatpakInstaller, manuals_flatpak_installer, MANUALS_TYPE_SDK_INSTALLER)

static DexFuture *
manuals_flatpak_installer_load (ManualsSdkInstaller *installer)
{
  return dex_ref (MANUALS_FLATPAK_INSTALLER (installer)->load);
}

static void
manuals_flatpak_installer_dispose (GObject *object)
{
  ManualsFlatpakInstaller *self = (ManualsFlatpakInstaller *)object;

  dex_clear (&self->load);
  g_clear_pointer (&self->installations, g_ptr_array_unref);

  G_OBJECT_CLASS (manuals_flatpak_installer_parent_class)->dispose (object);
}

static void
manuals_flatpak_installer_class_init (ManualsFlatpakInstallerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ManualsSdkInstallerClass *installer_class = MANUALS_SDK_INSTALLER_CLASS (klass);

  object_class->dispose = manuals_flatpak_installer_dispose;

  installer_class->load = manuals_flatpak_installer_load;
}

static void
manuals_flatpak_installer_init (ManualsFlatpakInstaller *self)
{
  self->installations = g_ptr_array_new_with_free_func (g_object_unref);
}

static DexFuture *
manuals_flatpak_installer_monitor (ManualsFlatpakInstaller *self,
                                   FlatpakInstallation     *installation)
{
  const char *system_tags[] = { _("System"), NULL };
  const char *user_tags[] = { _("User"), NULL };
  g_autoptr(GPtrArray) refs = NULL;
  g_autoptr(GError) error = NULL;
  const char * const *tags;

  g_assert (MANUALS_IS_FLATPAK_INSTALLER (self));
  g_assert (FLATPAK_IS_INSTALLATION (installation));

  if (!(refs = dex_await_boxed (list_refs_by_kind (installation, FLATPAK_REF_KIND_RUNTIME), &error)))
    return dex_future_new_for_error (g_steal_pointer (&error));

  if (flatpak_installation_get_is_user (installation))
    tags = user_tags;
  else
    tags = system_tags;

  for (guint i = 0; i < refs->len; i++)
    {
      FlatpakRef *ref = g_ptr_array_index (refs, i);
      const char *name = flatpak_ref_get_name (ref);
      g_autoptr(ManualsFlatpakReference) reference = NULL;

      if (!g_str_has_suffix (name, ".Docs"))
        continue;

      /* Ignore KDE as we are extremely unlikely to have any
       * data we can injest there.
       */
      if (g_str_has_prefix (name, "org.kde."))
        continue;

      if (!g_str_equal (flatpak_ref_get_arch (ref),
                        flatpak_get_default_arch ()))
        continue;

      reference = manuals_flatpak_reference_new (installation, ref);
      manuals_sdk_reference_set_tags (MANUALS_SDK_REFERENCE (reference), tags);

      manuals_sdk_installer_add (MANUALS_SDK_INSTALLER (self),
                                 MANUALS_SDK_REFERENCE (reference));
    }

  return dex_future_new_for_boolean (TRUE);
}

static DexFuture *
manuals_flatpak_installer_load_fiber (gpointer user_data)
{
  ManualsFlatpakInstaller *self = user_data;
  g_autoptr(GPtrArray) installations = NULL;
  g_autoptr(GPtrArray) futures = NULL;
  g_autoptr(GError) error = NULL;

  g_assert (MANUALS_IS_FLATPAK_INSTALLER (self));

  if (!(installations = dex_await_boxed (manuals_flatpak_load_installations (), &error)))
    return dex_future_new_for_error (g_steal_pointer (&error));

  futures = g_ptr_array_new_with_free_func (dex_unref);
  for (guint i = 0; i < installations->len; i++)
    g_ptr_array_add (futures,
                     manuals_flatpak_installer_monitor (self,
                                                        g_ptr_array_index (installations, i)));

  if (futures->len > 0)
    dex_await (dex_future_allv ((DexFuture **)futures->pdata, futures->len), NULL);

  return dex_future_new_for_boolean (TRUE);
}

ManualsSdkInstaller *
manuals_flatpak_installer_new (void)
{
  ManualsFlatpakInstaller *self;

  self = g_object_new (MANUALS_TYPE_FLATPAK_INSTALLER,
                       "title", _("Flatpak"),
                       NULL);

  self->load = dex_scheduler_spawn (NULL, 0,
                                    manuals_flatpak_installer_load_fiber,
                                    g_object_ref (self),
                                    g_object_unref);

  return MANUALS_SDK_INSTALLER (self);
}
