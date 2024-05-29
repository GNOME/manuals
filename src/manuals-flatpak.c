/*
 * manuals-flatpak.c
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

#include "manuals-flatpak.h"

static inline gpointer
load_installations_thread (gpointer data)
{
  g_autoptr(DexPromise) promise = data;
  g_autoptr(GPtrArray) installations = g_ptr_array_new_with_free_func (g_object_unref);
  g_autoptr(GPtrArray) system = NULL;
  g_autoptr(FlatpakInstallation) user = NULL;
  g_auto(GValue) value = G_VALUE_INIT;

  if (g_file_test ("/.flatpak-info", G_FILE_TEST_EXISTS))
    {
      g_autoptr(GFile) user_path = g_file_new_build_filename (g_get_home_dir (), ".local", "share", "flatpak", NULL);
      g_autoptr(GFile) host_path = g_file_new_for_path ("/var/run/host/var/lib/flatpak");
      g_autoptr(FlatpakInstallation) host = NULL;

      if ((user = flatpak_installation_new_for_path (user_path, TRUE, NULL, NULL)))
        g_ptr_array_add (installations, g_object_ref (user));

      if ((host = flatpak_installation_new_for_path (host_path, FALSE, NULL, NULL)))
        g_ptr_array_add (installations, g_object_ref (host));
    }
  else
    {
      if ((user = flatpak_installation_new_user (NULL, NULL)))
        g_ptr_array_add (installations, g_object_ref (user));

      if ((system = flatpak_get_system_installations (NULL, NULL)))
        g_ptr_array_extend (installations, system, (GCopyFunc)g_object_ref, NULL);
    }

  g_value_init (&value, G_TYPE_PTR_ARRAY);
  g_value_set_boxed (&value, installations);

  dex_promise_resolve (promise, &value);

  return NULL;
}

DexFuture *
manuals_flatpak_load_installations (void)
{
  static DexPromise *promise;
  g_autoptr(GThread) thread = NULL;

  if (promise != NULL)
    return dex_ref (promise);

  promise = dex_promise_new ();
  thread = g_thread_new ("[manuals-flatpak]",
                         load_installations_thread,
                         dex_ref (promise));

  return dex_ref (promise);
}
