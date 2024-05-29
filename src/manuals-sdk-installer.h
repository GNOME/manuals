/*
 * manuals-sdk-installer.h
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

#pragma once

#include <libdex.h>

#include "manuals-progress.h"
#include "manuals-sdk-reference.h"

G_BEGIN_DECLS

#define MANUALS_TYPE_SDK_INSTALLER (manuals_sdk_installer_get_type())

G_DECLARE_DERIVABLE_TYPE (ManualsSdkInstaller, manuals_sdk_installer, MANUALS, SDK_INSTALLER, GObject)

struct _ManualsSdkInstallerClass
{
  GObjectClass parent_class;

  DexFuture *(*load) (ManualsSdkInstaller *self);
};

DexFuture *manuals_sdk_installer_load   (ManualsSdkInstaller *self);
void       manuals_sdk_installer_add    (ManualsSdkInstaller *self,
                                         ManualsSdkReference *ref);
void       manuals_sdk_installer_remove (ManualsSdkInstaller *self,
                                         ManualsSdkReference *ref);

G_END_DECLS
