/* manuals-application.c
 *
 * Copyright 2024 Christian Hergert
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"

#include <glib/gi18n.h>

#include "manuals-application.h"
#include "manuals-devhelp-importer.h"
#include "manuals-flatpak-importer.h"
#include "manuals-jhbuild-importer.h"
#include "manuals-purge-missing.h"
#include "manuals-system-importer.h"
#include "manuals-window.h"

#define DELAY_FOR_IMPORT_TIMEOUT_MSEC 100

struct _ManualsApplication
{
  AdwApplication     parent_instance;

  DexFuture         *repository;
  DexFuture         *import;
  ManualsProgress   *import_progress;
  char              *storage_dir;

  guint              import_active : 1;
};

G_DEFINE_FINAL_TYPE (ManualsApplication, manuals_application, ADW_TYPE_APPLICATION)

enum {
  PROP_0,
  PROP_IMPORT_ACTIVE,
  PROP_IMPORT_PROGRESS,
  N_PROPS
};

enum {
  INVALIDATE_CONTENTS,
  N_SIGNALS
};

static GParamSpec *properties[N_PROPS];
static guint signals[N_SIGNALS];

ManualsApplication *
manuals_application_new (const char        *application_id,
                         GApplicationFlags  flags)
{
  g_return_val_if_fail (application_id != NULL, NULL);

  g_debug ("Running manuals with app-id %s", application_id);

  return g_object_new (MANUALS_TYPE_APPLICATION,
                       "application-id", application_id,
                       "flags", flags,
                       NULL);
}

gboolean
manuals_application_control_is_pressed (void)
{
  GdkDisplay *display = gdk_display_get_default ();
  GdkSeat *seat = gdk_display_get_default_seat (display);
  GdkDevice *keyboard = gdk_seat_get_keyboard (seat);
  GdkModifierType modifiers = gdk_device_get_modifier_state (keyboard) & gtk_accelerator_get_default_mod_mask ();

  return !!(modifiers & GDK_CONTROL_MASK);
}

static DexFuture *
manuals_application_activate_cb (DexFuture *completed,
                                 gpointer   user_data)
{
  g_autoptr(ManualsRepository) repository = NULL;
  ManualsApplication *self = user_data;
  ManualsWindow *window;

  g_assert (DEX_IS_FUTURE (completed));
  g_assert (MANUALS_IS_APPLICATION (self));

  g_application_release (G_APPLICATION (self));

  repository = dex_await_object (dex_ref (self->repository), NULL);
  window = manuals_window_new (repository);

  gtk_window_present (GTK_WINDOW (window));

  return dex_future_new_for_boolean (TRUE);
}

static void
manuals_application_activate (GApplication *app)
{
  ManualsApplication *self = MANUALS_APPLICATION (app);
  GtkWindow *window;
  DexFuture *future;

  g_assert (MANUALS_IS_APPLICATION (app));

  if ((window = gtk_application_get_active_window (GTK_APPLICATION (app))))
    {
      gtk_window_present (window);
      return;
    }

  g_application_hold (app);

  /* We want to wait until import has completed or a small
   * delay, whichever is sooner so that we are less likely
   * to flash the "loading" screen when showing the window
   * on subsequent runs where everything is already imported.
   */
  future = dex_future_first (dex_ref (self->import),
                             dex_timeout_new_msec (DELAY_FOR_IMPORT_TIMEOUT_MSEC),
                             NULL);
  future = dex_future_finally (future,
                               manuals_application_activate_cb,
                               g_object_ref (app),
                               g_object_unref);
  dex_future_disown (future);
}

static DexFuture *
manuals_application_import (DexFuture *completed,
                            gpointer   user_data)
{
  ManualsApplication *self = user_data;
  g_autoptr(ManualsRepository) repository = NULL;
  g_autoptr(ManualsImporter) purge = NULL;
  g_autoptr(ManualsImporter) flatpak = NULL;
  g_autoptr(ManualsImporter) jhbuild = NULL;
  g_autoptr(ManualsImporter) system = NULL;

  g_assert (MANUALS_IS_APPLICATION (self));

  repository = dex_await_object (dex_ref (completed), NULL);
  purge = manuals_purge_missing_new ();
  flatpak = manuals_flatpak_importer_new ();
  system = manuals_system_importer_new ();
  jhbuild = manuals_jhbuild_importer_new ();

  self->import_active = TRUE;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_IMPORT_ACTIVE]);

  return dex_future_all (manuals_importer_import (purge, repository, self->import_progress),
                         manuals_importer_import (system, repository, self->import_progress),
                         manuals_importer_import (flatpak, repository, self->import_progress),
                         manuals_importer_import (jhbuild, repository, self->import_progress),
                         NULL);

}

static int
manuals_application_command_line (GApplication            *app,
                                  GApplicationCommandLine *cmdline)
{
  ManualsApplication *self = (ManualsApplication *)app;
  GVariantDict *dict;
  gboolean new_window;

  g_assert (MANUALS_IS_APPLICATION (self));
  g_assert (G_IS_APPLICATION_COMMAND_LINE (cmdline));

  dict = g_application_command_line_get_options_dict (cmdline);

  if (!g_variant_dict_lookup (dict, "new-window", "b", &new_window))
    new_window = FALSE;

  if (new_window)
    g_action_group_activate_action (G_ACTION_GROUP (self), "new-window", NULL);
  else
    g_application_activate (G_APPLICATION (self));

  return G_APPLICATION_CLASS (manuals_application_parent_class)->command_line (app, cmdline);
}

static DexFuture *
manuals_application_import_complete (DexFuture *completed,
                                     gpointer   user_data)
{
  ManualsApplication *self = user_data;

  g_assert (MANUALS_IS_APPLICATION (self));

  self->import_active = FALSE;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_IMPORT_ACTIVE]);

  g_signal_emit (self, signals[INVALIDATE_CONTENTS], 0);

  return NULL;
}

static void
manuals_application_progress_notify_fraction_cb (ManualsApplication *self,
                                                 GParamSpec         *pspec,
                                                 ManualsProgress    *progress)
{
  g_assert (MANUALS_IS_APPLICATION (self));
  g_assert (MANUALS_IS_PROGRESS (progress));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_IMPORT_PROGRESS]);
}

static void
manuals_application_startup (GApplication *application)
{
  ManualsApplication *self = (ManualsApplication *)application;
  g_autofree char *storage_path = NULL;
  DexFuture *future;

  g_application_set_resource_base_path (application, "/app/devsuite/Manuals");

  /* Setup the progress helper we'll use in the app */
  self->import_progress = manuals_progress_new ();
  g_signal_connect_object (self->import_progress,
                           "notify::fraction",
                           G_CALLBACK (manuals_application_progress_notify_fraction_cb),
                           self,
                           G_CONNECT_SWAPPED);

  /* Figure out where storage is going to be including SQLite db */
  self->storage_dir = g_build_filename (g_get_user_data_dir (),
                                        g_application_get_application_id (application),
                                        NULL);
  storage_path = g_build_filename (self->storage_dir, "manuals.sqlite", NULL);
  g_mkdir_with_parents (self->storage_dir, 0770);

  /* Start loading the repository asynchronously */
  self->repository = manuals_repository_open (storage_path);

  /* Start importing into the repository after SQLite db has opened.
   * Emit the signal to request windows reload contents when that
   * is finished. Otherwise they may not pick up newly loaded contents
   * when the application starts up.
   */
  future = dex_future_then (dex_ref (self->repository),
                            manuals_application_import,
                            g_object_ref (self),
                            g_object_unref);
  future = dex_future_finally (future,
                               manuals_application_import_complete,
                               g_object_ref (self),
                               g_object_unref);
  self->import = g_steal_pointer (&future);

  G_APPLICATION_CLASS (manuals_application_parent_class)->startup (application);
}

static void
manuals_application_shutdown (GApplication *application)
{
  ManualsApplication *self = (ManualsApplication *)application;

  G_APPLICATION_CLASS (manuals_application_parent_class)->shutdown (application);

  g_clear_pointer (&self->storage_dir, g_free);
  g_clear_object (&self->import_progress);
  dex_clear (&self->import);
  dex_clear (&self->repository);
}

static void
manuals_application_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  ManualsApplication *self = MANUALS_APPLICATION (object);

  switch (prop_id)
    {
    case PROP_IMPORT_ACTIVE:
      g_value_set_boolean (value, manuals_application_get_import_active (self));
      break;

    case PROP_IMPORT_PROGRESS:
      g_value_set_double (value, manuals_application_get_import_progress (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
manuals_application_class_init (ManualsApplicationClass *klass)
{
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = manuals_application_get_property;

  app_class->activate = manuals_application_activate;
  app_class->startup = manuals_application_startup;
  app_class->shutdown = manuals_application_shutdown;
  app_class->command_line = manuals_application_command_line;

  properties[PROP_IMPORT_ACTIVE] =
    g_param_spec_boolean ("import-active", NULL, NULL,
                          FALSE,
                          (G_PARAM_READABLE |
                           G_PARAM_STATIC_STRINGS));

  properties[PROP_IMPORT_PROGRESS] =
    g_param_spec_double ("import-progress", NULL, NULL,
                         0, 1, 0,
                         (G_PARAM_READABLE |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  signals[INVALIDATE_CONTENTS] =
    g_signal_new ("invalidate-contents",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
}

static void
manuals_application_new_window_action (GSimpleAction *action,
                                       GVariant      *parameter,
                                       gpointer       user_data)
{
  ManualsApplication *self = user_data;

  g_assert (MANUALS_IS_APPLICATION (self));

  g_application_hold (G_APPLICATION (self));

  dex_future_disown (dex_future_then (dex_ref (self->repository),
                                      manuals_application_activate_cb,
                                      g_object_ref (self),
                                      g_object_unref));
}

static void
manuals_application_about_action (GSimpleAction *action,
                                  GVariant      *parameter,
                                  gpointer       user_data)
{
  static const char *developers[] = {"Christian Hergert", NULL};
  ManualsApplication *self = user_data;
  GtkWindow *window = NULL;

  g_assert (MANUALS_IS_APPLICATION (self));

  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  adw_show_about_dialog (GTK_WIDGET (window),
                         "application-name", _("Manuals"),
                         "application-icon", APP_ID,
                         "developer-name", "Christian Hergert",
                         "version", PACKAGE_VERSION,
                         "developers", developers,
                         "copyright", "© 2024 Christian Hergert",
                         "license-type", GTK_LICENSE_GPL_3_0,
                         NULL);
}

static void
manuals_application_quit_action (GSimpleAction *action,
                                 GVariant      *parameter,
                                 gpointer       user_data)
{
  ManualsApplication *self = user_data;

  g_assert (MANUALS_IS_APPLICATION (self));

  g_application_quit (G_APPLICATION (self));
}

static const GActionEntry app_actions[] = {
  { "quit", manuals_application_quit_action },
  { "about", manuals_application_about_action },
  { "new-window", manuals_application_new_window_action },
};

static void
manuals_application_init (ManualsApplication *self)
{
  static const GOptionEntry main_entries[] = {
    { "new-window", 0, 0, G_OPTION_ARG_NONE, NULL, N_("New manuals window") },
    { NULL }
  };

  g_application_set_default (G_APPLICATION (self));

  g_application_add_main_option_entries (G_APPLICATION (self), main_entries);

  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   app_actions,
                                   G_N_ELEMENTS (app_actions),
                                   self);
  gtk_application_set_accels_for_action (GTK_APPLICATION (self),
                                         "app.quit",
                                         (const char *[]) { "<primary>q", NULL });
}

gboolean
manuals_application_get_import_active (ManualsApplication *self)
{
  g_return_val_if_fail (MANUALS_IS_APPLICATION (self), FALSE);

  return self->import_active;
}

double
manuals_application_get_import_progress (ManualsApplication *self)
{
  g_return_val_if_fail (MANUALS_IS_APPLICATION (self), 0);

  return manuals_progress_get_fraction (self->import_progress);
}
