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

#include "manuals-progress.h"

G_BEGIN_DECLS

DexFuture *manuals_flatpak_load_installations   (void);
DexFuture *manuals_flatpak_installation_install (FlatpakInstallation *installation,
                                                 FlatpakRemoteRef    *remote_ref,
                                                 ManualsProgress     *progress,
                                                 GCancellable        *cancellable);

typedef struct
{
  FlatpakInstallation *installation;
  DexPromise *promise;
  GCancellable *cancellable;
  FlatpakRefKind ref_kind;
} ListRefsByKind;;

static void
list_refs_by_kind_free (ListRefsByKind *state)
{
  g_clear_object (&state->installation);
  g_clear_object (&state->cancellable);
  dex_clear (&state->promise);
  g_free (state);
}

static inline gpointer
list_installed_refs_by_kind_thread (gpointer data)
{
  ListRefsByKind *state = data;
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

  list_refs_by_kind_free (state);

  return NULL;
}

static inline DexFuture *
list_installed_refs_by_kind (FlatpakInstallation *installation,
                             FlatpakRefKind       ref_kind)
{
  g_autoptr(GThread) thread = NULL;
  DexPromise *promise = dex_promise_new_cancellable ();
  ListRefsByKind *state = g_new0 (ListRefsByKind, 1);

  g_set_object (&state->installation, installation);
  g_set_object (&state->cancellable, dex_promise_get_cancellable (promise));
  state->promise = dex_ref (promise);
  state->ref_kind = ref_kind;

  thread = g_thread_new ("[manuals-flatpak]",
                         list_installed_refs_by_kind_thread,
                         state);

  return DEX_FUTURE (promise);
}

static inline gpointer
list_refs_by_kind_thread (gpointer data)
{
  ListRefsByKind *state = data;
  g_autoptr(GPtrArray) remotes = NULL;
  g_auto(GValue) value = G_VALUE_INIT;
  GPtrArray *refs;
  GError *error = NULL;

  g_assert (state != NULL);
  g_assert (FLATPAK_IS_INSTALLATION (state->installation));
  g_assert (!state->cancellable || G_IS_CANCELLABLE (state->cancellable));

  remotes = flatpak_installation_list_remotes (state->installation,
                                               state->cancellable,
                                               &error);
  if (error != NULL)
    goto handle_error;

  g_assert (remotes != NULL);

  refs = g_ptr_array_new_with_free_func (g_object_unref);

  for (guint i = 0; i < remotes->len; i++)
    {
      FlatpakRemote *remote = g_ptr_array_index (remotes, i);
      const char *remote_name = flatpak_remote_get_name (remote);
      g_autoptr(GPtrArray) remote_refs = NULL;

      remote_refs = flatpak_installation_list_remote_refs_sync_full (state->installation,
                                                                     remote_name,
                                                                     FLATPAK_QUERY_FLAGS_NONE,
                                                                     state->cancellable,
                                                                     &error);

      if (remote_refs != NULL)
        {
          for (guint j = 0; j < remote_refs->len; j++)
            {
              FlatpakRef *ref = g_ptr_array_index (remote_refs, j);
              const char *ref_name = flatpak_ref_get_name (ref);

              if (g_str_has_suffix (ref_name, ".Docs"))
                g_ptr_array_add (refs, g_object_ref (ref));
            }
        }

      g_clear_error (&error);
    }

  g_value_init (&value, G_TYPE_PTR_ARRAY);
  g_value_take_boxed (&value, refs);

handle_error:
  if (error != NULL)
    dex_promise_reject (state->promise, g_steal_pointer (&error));
  else
    dex_promise_resolve (state->promise, &value);

  list_refs_by_kind_free (state);

  return NULL;
}

static inline DexFuture *
list_refs_by_kind (FlatpakInstallation *installation,
                   FlatpakRefKind       ref_kind)
{
  g_autoptr(GThread) thread = NULL;
  DexPromise *promise = dex_promise_new_cancellable ();
  ListRefsByKind *state = g_new0 (ListRefsByKind, 1);

  g_set_object (&state->installation, installation);
  g_set_object (&state->cancellable, dex_promise_get_cancellable (promise));
  state->promise = dex_ref (promise);
  state->ref_kind = ref_kind;

  thread = g_thread_new ("[manuals-flatpak]",
                         list_refs_by_kind_thread,
                         state);

  return DEX_FUTURE (promise);
}

G_END_DECLS
