/*
 * manuals-sdk-reference.h
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

G_BEGIN_DECLS

#define MANUALS_TYPE_SDK_REFERENCE (manuals_sdk_reference_get_type())

G_DECLARE_DERIVABLE_TYPE (ManualsSdkReference, manuals_sdk_reference, MANUALS, SDK_REFERENCE, GObject)

struct _ManualsSdkReferenceClass
{
  GObjectClass parent_class;

  gboolean   (*equal)   (ManualsSdkReference *self,
                         ManualsSdkReference *other);
  DexFuture *(*install) (ManualsSdkReference *self,
                         ManualsProgress     *progress,
                         GCancellable        *cancellable);
};

gboolean            manuals_sdk_reference_get_installed (ManualsSdkReference *self);
void                manuals_sdk_reference_set_installed (ManualsSdkReference *self,
                                                         gboolean             installed);
const char         *manuals_sdk_reference_get_title     (ManualsSdkReference *self);
const char         *manuals_sdk_reference_get_subtitle  (ManualsSdkReference *self);
gboolean            manuals_sdk_reference_equal         (ManualsSdkReference *self,
                                                         ManualsSdkReference *other);
const char * const *manuals_sdk_reference_get_tags      (ManualsSdkReference *self);
void                manuals_sdk_reference_set_tags      (ManualsSdkReference *self,
                                                         const char * const  *tags);
DexFuture          *manuals_sdk_reference_install       (ManualsSdkReference *self,
                                                         ManualsProgress     *progress,
                                                         GCancellable        *cancellable);

G_END_DECLS
