/*
 * manuals-window.h
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

#include <libpanel.h>

#include "manuals-repository.h"
#include "manuals-tab.h"

G_BEGIN_DECLS

#define MANUALS_TYPE_WINDOW (manuals_window_get_type())

G_DECLARE_FINAL_TYPE (ManualsWindow, manuals_window, MANUALS, WINDOW, PanelWorkspace)

ManualsWindow     *manuals_window_from_widget         (GtkWidget          *widget);
ManualsWindow     *manuals_window_new                 (ManualsRepository  *repository);
void               manuals_window_add_tab             (ManualsWindow      *self,
                                                       ManualsTab         *tab);
ManualsTab        *manuals_window_get_visible_tab     (ManualsWindow      *self);
void               manuals_window_set_visible_tab     (ManualsWindow      *self,
                                                       ManualsTab         *tab);
ManualsRepository *manuals_window_get_repository      (ManualsWindow      *self);
void               manuals_window_navigate_to         (ManualsWindow      *self,
                                                       ManualsNavigatable *navigatable);

G_END_DECLS
