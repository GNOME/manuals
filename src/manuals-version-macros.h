/*
 * manuals-version-macros.h
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

#include <glib.h>

#include "manuals-version.h"

#ifndef _MANUALS_EXTERN
#define _MANUALS_EXTERN extern
#endif

#ifdef MANUALS_DISABLE_DEPRECATION_WARNINGS
# define MANUALS_DEPRECATED _MANUALS_EXTERN
# define MANUALS_DEPRECATED_FOR(f) _MANUALS_EXTERN
# define MANUALS_UNAVAILABLE(maj,min) _MANUALS_EXTERN
#else
# define MANUALS_DEPRECATED G_DEPRECATED _MANUALS_EXTERN
# define MANUALS_DEPRECATED_FOR(f) G_DEPRECATED_FOR(f) _MANUALS_EXTERN
# define MANUALS_UNAVAILABLE(maj,min) G_UNAVAILABLE(maj,min) _MANUALS_EXTERN
#endif

#define MANUALS_VERSION_1_0 (G_ENCODE_VERSION (1, 0))

#if (MANUALS_MINOR_VERSION == 99)
# define MANUALS_VERSION_CUR_STABLE (G_ENCODE_VERSION (MANUALS_MAJOR_VERSION + 1, 0))
#elif (MANUALS_MINOR_VERSION % 2)
# define MANUALS_VERSION_CUR_STABLE (G_ENCODE_VERSION (MANUALS_MAJOR_VERSION, MANUALS_MINOR_VERSION + 1))
#else
# define MANUALS_VERSION_CUR_STABLE (G_ENCODE_VERSION (MANUALS_MAJOR_VERSION, MANUALS_MINOR_VERSION))
#endif

#if (MANUALS_MINOR_VERSION == 99)
# define MANUALS_VERSION_PREV_STABLE (G_ENCODE_VERSION (MANUALS_MAJOR_VERSION + 1, 0))
#elif (MANUALS_MINOR_VERSION % 2)
# define MANUALS_VERSION_PREV_STABLE (G_ENCODE_VERSION (MANUALS_MAJOR_VERSION, MANUALS_MINOR_VERSION - 1))
#else
# define MANUALS_VERSION_PREV_STABLE (G_ENCODE_VERSION (MANUALS_MAJOR_VERSION, MANUALS_MINOR_VERSION - 2))
#endif

/**
 * MANUALS_VERSION_MIN_REQUIRED:
 *
 * A macro that should be defined by the user prior to including
 * the manualsing.h header.
 *
 * The definition should be one of the predefined Manuals version
 * macros: %MANUALS_VERSION_1_0, ...
 *
 * This macro defines the lower bound for the Manuals API to use.
 *
 * If a function has been deprecated in a newer version of Manuals,
 * it is possible to use this symbol to avoid the compiler warnings
 * without disabling warning for every deprecated function.
 */
#ifndef MANUALS_VERSION_MIN_REQUIRED
# define MANUALS_VERSION_MIN_REQUIRED (MANUALS_VERSION_CUR_STABLE)
#endif

/**
 * MANUALS_VERSION_MAX_ALLOWED:
 *
 * A macro that should be defined by the user prior to including
 * the manualsing.h header.

 * The definition should be one of the predefined Manuals version
 * macros: %MANUALS_VERSION_1_0, %MANUALS_VERSION_1_2,...
 *
 * This macro defines the upper bound for the Manuals API to use.
 *
 * If a function has been introduced in a newer version of Manuals,
 * it is possible to use this symbol to get compiler warnings when
 * trying to use that function.
 */
#ifndef MANUALS_VERSION_MAX_ALLOWED
# if MANUALS_VERSION_MIN_REQUIRED > MANUALS_VERSION_PREV_STABLE
#  define MANUALS_VERSION_MAX_ALLOWED (MANUALS_VERSION_MIN_REQUIRED)
# else
#  define MANUALS_VERSION_MAX_ALLOWED (MANUALS_VERSION_CUR_STABLE)
# endif
#endif

#if MANUALS_VERSION_MAX_ALLOWED < MANUALS_VERSION_MIN_REQUIRED
#error "MANUALS_VERSION_MAX_ALLOWED must be >= MANUALS_VERSION_MIN_REQUIRED"
#endif
#if MANUALS_VERSION_MIN_REQUIRED < MANUALS_VERSION_1_0
#error "MANUALS_VERSION_MIN_REQUIRED must be >= MANUALS_VERSION_1_0"
#endif

#define MANUALS_AVAILABLE_IN_ALL                  _MANUALS_EXTERN

#if MANUALS_VERSION_MIN_REQUIRED >= MANUALS_VERSION_1_0
# define MANUALS_DEPRECATED_IN_1_0                MANUALS_DEPRECATED
# define MANUALS_DEPRECATED_IN_1_0_FOR(f)         MANUALS_DEPRECATED_FOR(f)
#else
# define MANUALS_DEPRECATED_IN_1_0                _MANUALS_EXTERN
# define MANUALS_DEPRECATED_IN_1_0_FOR(f)         _MANUALS_EXTERN
#endif

#if MANUALS_VERSION_MAX_ALLOWED < MANUALS_VERSION_1_0
# define MANUALS_AVAILABLE_IN_1_0                 MANUALS_UNAVAILABLE(1, 0)
#else
# define MANUALS_AVAILABLE_IN_1_0                 _MANUALS_EXTERN
#endif

