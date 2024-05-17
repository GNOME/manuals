/*
 * manuals-flatpak.h
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

#include <flatpak.h>
#include <libdex.h>

G_BEGIN_DECLS

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

static inline DexFuture *
load_installations (void)
{
  DexPromise *promise = dex_promise_new ();
  g_autoptr(GThread) thread = g_thread_new ("[manuals-flatpak]",
                                            load_installations_thread,
                                            dex_ref (promise));
  return DEX_FUTURE (promise);
}

typedef struct _ListInstalledRuntimes
{
  FlatpakInstallation *installation;
  DexPromise *promise;
  GCancellable *cancellable;
  FlatpakRefKind ref_kind;
} ListInstalledRefsByKind;;

static void
list_installed_refs_by_kind_free (ListInstalledRefsByKind *state)
{
  g_clear_object (&state->installation);
  g_clear_object (&state->cancellable);
  dex_clear (&state->promise);
  g_free (state);
}

static inline gpointer
list_installed_refs_by_kind_thread (gpointer data)
{
  ListInstalledRefsByKind *state = data;
  g_auto(GValue) value = G_VALUE_INIT;
  GPtrArray *refs;
  GError *error = NULL;

  refs = flatpak_installation_list_installed_refs_by_kind (state->installation,
                                                           state->ref_kind,
                                                           state->cancellable,
                                                           &error);

  g_value_init (&value, G_TYPE_PTR_ARRAY);
  g_value_take_boxed (&value, refs);

  if (error != NULL)
    dex_promise_reject (state->promise, g_steal_pointer (&error));
  else
    dex_promise_resolve (state->promise, &value);

  list_installed_refs_by_kind_free (state);

  return NULL;
}

static inline DexFuture *
list_installed_refs_by_kind (FlatpakInstallation *installation,
                             FlatpakRefKind       ref_kind)

{
  g_autoptr(GThread) thread = NULL;
  DexPromise *promise = dex_promise_new_cancellable ();
  ListInstalledRefsByKind *state = g_new0 (ListInstalledRefsByKind, 1);

  g_set_object (&state->installation, installation);
  g_set_object (&state->cancellable, dex_promise_get_cancellable (promise));
  state->promise = dex_ref (promise);
  state->ref_kind = ref_kind;

  thread = g_thread_new ("[manuals-flatpak]",
                         list_installed_refs_by_kind_thread,
                         state);

  return DEX_FUTURE (promise);
}

G_END_DECLS
