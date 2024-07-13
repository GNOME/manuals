/*
 * manuals-search-result-private.h
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

#include "manuals-search-result.h"
#include "manuals-search-model.h"

G_BEGIN_DECLS

struct _ManualsSearchResult
{
  GObject             parent_instance;
  GObject            *item;
  guint               position;
  ManualsSearchModel *model;
  GList               link;
};

void manuals_search_model_release (ManualsSearchModel  *self,
                                   ManualsSearchResult *result);

G_END_DECLS
