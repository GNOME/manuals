/*
 * manuals-search-view.h
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

#include <gtk/gtk.h>

#include "manuals-navigatable.h"
#include "manuals-search-query.h"

G_BEGIN_DECLS

typedef enum _ManualsSearchColumns
{
  MANUALS_SEARCH_COLUMN_NAME = 1,
  MANUALS_SEARCH_COLUMN_SDK  = 1 << 1,
  MANUALS_SEARCH_COLUMN_BOOK = 1 << 2,
} ManualsSearchColumns;

#define MANUALS_SEARCH_COLUMN_ALL \
  (MANUALS_SEARCH_COLUMN_NAME | \
   MANUALS_SEARCH_COLUMN_BOOK | \
   MANUALS_SEARCH_COLUMN_SDK)

#define MANUALS_TYPE_SEARCH_VIEW (manuals_search_view_get_type())

G_DECLARE_FINAL_TYPE (ManualsSearchView, manuals_search_view, MANUALS, SEARCH_VIEW, GtkWidget)

void                manuals_search_view_search       (ManualsSearchView    *self,
                                                      ManualsSearchQuery   *query);
void                manuals_search_view_display      (ManualsSearchView    *self,
                                                      GListModel           *results,
                                                      ManualsSearchColumns  columns);
ManualsNavigatable *manuals_search_view_dup_selected (ManualsSearchView    *self);
void                manuals_search_view_move_down    (ManualsSearchView    *self);

G_END_DECLS
