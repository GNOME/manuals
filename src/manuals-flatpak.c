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

#include <glib/gi18n.h>

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

typedef struct
{
  FlatpakInstallation *installation;
  FlatpakRemoteRef *remote_ref;
  ManualsJob *job;
  GCancellable *cancellable;
  DexPromise *promise;
} Install;

static void
install_free (Install *install)
{
  g_clear_object (&install->installation);
  g_clear_object (&install->remote_ref);
  g_clear_object (&install->job);
  g_clear_object (&install->cancellable);
  dex_clear (&install->promise);
  g_free (install);
}

static void
progress_cb (const char *status,
             guint       progress,
             gboolean    estimating,
             gpointer    user_data)
{
  ManualsJob *job = user_data;

  g_assert (MANUALS_IS_JOB (job));

  manuals_job_set_subtitle (job, status);
  manuals_job_set_fraction (job, .1 + (((double)progress / 100.) * .9));
}

static gpointer
install_thread (gpointer user_data)
{
  Install *state = user_data;
  GError *error = NULL;

  g_assert (state != NULL);
  g_assert (FLATPAK_IS_INSTALLATION (state->installation));
  g_assert (FLATPAK_IS_REMOTE_REF (state->remote_ref));
  g_assert (MANUALS_IS_JOB (state->job));
  g_assert (!state->cancellable || G_IS_CANCELLABLE (state->cancellable));

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  flatpak_installation_install (state->installation,
                                flatpak_remote_ref_get_remote_name (state->remote_ref),
                                flatpak_ref_get_kind (FLATPAK_REF (state->remote_ref)),
                                flatpak_ref_get_name (FLATPAK_REF (state->remote_ref)),
                                flatpak_ref_get_arch (FLATPAK_REF (state->remote_ref)),
                                flatpak_ref_get_branch (FLATPAK_REF (state->remote_ref)),
                                progress_cb,
                                state->job,
                                state->cancellable,
                                &error);
  G_GNUC_END_IGNORE_DEPRECATIONS

  if (error != NULL)
    dex_promise_reject (state->promise, g_steal_pointer (&error));
  else
    dex_promise_resolve_boolean (state->promise, TRUE);

  install_free (state);

  return NULL;
}

DexFuture *
manuals_flatpak_installation_install (FlatpakInstallation *installation,
                                      FlatpakRemoteRef    *remote_ref,
                                      ManualsProgress     *progress,
                                      GCancellable        *cancellable)
{
  g_autoptr(GThread) thread = NULL;
  g_autofree char *title = NULL;
  DexPromise *promise;
  Install *install;

  g_return_val_if_fail (FLATPAK_IS_INSTALLATION (installation), NULL);
  g_return_val_if_fail (FLATPAK_IS_REMOTE_REF (remote_ref), NULL);
  g_return_val_if_fail (MANUALS_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable), NULL);

  promise = dex_promise_new ();

  install = g_new0 (Install, 1);
  g_set_object (&install->installation, installation);
  g_set_object (&install->remote_ref, remote_ref);
  g_set_object (&install->cancellable, cancellable);
  install->job = manuals_progress_begin_job (progress);
  install->promise = dex_ref (promise);

  title = g_strdup_printf (_("Installing %s/%s/%s"),
                           flatpak_ref_get_name (FLATPAK_REF (remote_ref)),
                           flatpak_ref_get_arch (FLATPAK_REF (remote_ref)),
                           flatpak_ref_get_branch (FLATPAK_REF (remote_ref)));
  manuals_job_set_title (install->job, title);
  manuals_job_set_fraction (install->job, .1);

  thread = g_thread_new ("[manuals-flatpak]", install_thread, install);

  return DEX_FUTURE (promise);
}
