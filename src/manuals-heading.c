/*
 * manuals-heading.c
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

#include "manuals-book.h"
#include "manuals-heading.h"
#include "manuals-repository.h"

struct _ManualsHeading
{
  GomResource parent_instance;
  gint64 id;
  gint64 parent_id;
  gint64 book_id;
  char *title;
  char *uri;
};

G_DEFINE_FINAL_TYPE (ManualsHeading, manuals_heading, GOM_TYPE_RESOURCE)

enum {
  PROP_0,
  PROP_ID,
  PROP_PARENT_ID,
  PROP_BOOK_ID,
  PROP_TITLE,
  PROP_URI,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
manuals_heading_finalize (GObject *object)
{
  ManualsHeading *self = (ManualsHeading *)object;

  g_clear_pointer (&self->title, g_free);
  g_clear_pointer (&self->uri, g_free);

  G_OBJECT_CLASS (manuals_heading_parent_class)->finalize (object);
}

static void
manuals_heading_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  ManualsHeading *self = MANUALS_HEADING (object);

  switch (prop_id)
    {
    case PROP_ID:
      g_value_set_int64 (value, manuals_heading_get_id (self));
      break;

    case PROP_PARENT_ID:
      g_value_set_int64 (value, manuals_heading_get_parent_id (self));
      break;

    case PROP_BOOK_ID:
      g_value_set_int64 (value, manuals_heading_get_book_id (self));
      break;

    case PROP_TITLE:
      g_value_set_string (value, manuals_heading_get_title (self));
      break;

    case PROP_URI:
      g_value_set_string (value, manuals_heading_get_uri (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
manuals_heading_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ManualsHeading *self = MANUALS_HEADING (object);

  switch (prop_id)
    {
    case PROP_ID:
      manuals_heading_set_id (self, g_value_get_int64 (value));
      break;

    case PROP_PARENT_ID:
      manuals_heading_set_parent_id (self, g_value_get_int64 (value));
      break;

    case PROP_BOOK_ID:
      manuals_heading_set_book_id (self, g_value_get_int64 (value));
      break;

    case PROP_TITLE:
      manuals_heading_set_title (self, g_value_get_string (value));
      break;

    case PROP_URI:
      manuals_heading_set_uri (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
manuals_heading_class_init (ManualsHeadingClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GomResourceClass *resource_class = GOM_RESOURCE_CLASS (klass);

  object_class->finalize = manuals_heading_finalize;
  object_class->get_property = manuals_heading_get_property;
  object_class->set_property = manuals_heading_set_property;

  properties[PROP_ID] =
    g_param_spec_int64 ("id", NULL, NULL,
                        0, G_MAXINT64, 0,
                        (G_PARAM_READWRITE |
                         G_PARAM_EXPLICIT_NOTIFY |
                         G_PARAM_STATIC_STRINGS));

  properties[PROP_PARENT_ID] =
    g_param_spec_int64 ("parent-id", NULL, NULL,
                        0, G_MAXINT64, 0,
                        (G_PARAM_READWRITE |
                         G_PARAM_EXPLICIT_NOTIFY |
                         G_PARAM_STATIC_STRINGS));

  properties[PROP_BOOK_ID] =
    g_param_spec_int64 ("book-id", NULL, NULL,
                        0, G_MAXINT64, 0,
                        (G_PARAM_READWRITE |
                         G_PARAM_EXPLICIT_NOTIFY |
                         G_PARAM_STATIC_STRINGS));

  properties[PROP_TITLE] =
    g_param_spec_string ("title", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_URI] =
    g_param_spec_string ("uri", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gom_resource_class_set_table (resource_class, "headings");
  gom_resource_class_set_primary_key (resource_class, "id");
  gom_resource_class_set_notnull (resource_class, "title");
  gom_resource_class_set_reference (resource_class, "parent-id", "headings", "id");
  gom_resource_class_set_reference (resource_class, "book-id", "books", "id");
}

static void
manuals_heading_init (ManualsHeading *self)
{
}

const char *
manuals_heading_get_title (ManualsHeading *self)
{
  g_return_val_if_fail (MANUALS_IS_HEADING (self), NULL);

  return self->title;
}

void
manuals_heading_set_title (ManualsHeading *self,
                           const char     *title)
{
  g_return_if_fail (MANUALS_IS_HEADING (self));

  if (g_set_str (&self->title, title))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TITLE]);
}

const char *
manuals_heading_get_uri (ManualsHeading *self)
{
  g_return_val_if_fail (MANUALS_IS_HEADING (self), NULL);

  return self->uri;
}

void
manuals_heading_set_uri (ManualsHeading *self,
                         const char     *uri)
{
  g_return_if_fail (MANUALS_IS_HEADING (self));

  if (g_set_str (&self->uri, uri))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_URI]);
}

gint64
manuals_heading_get_id (ManualsHeading *self)
{
  g_return_val_if_fail (MANUALS_IS_HEADING (self), 0);

  return self->id;
}

void
manuals_heading_set_id (ManualsHeading *self,
                        gint64          id)
{
  g_return_if_fail (MANUALS_IS_HEADING (self));
  g_return_if_fail (id >= 0);

  if (self->id != id)
    {
      self->id = id;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ID]);
    }
}

gint64
manuals_heading_get_parent_id (ManualsHeading *self)
{
  g_return_val_if_fail (MANUALS_IS_HEADING (self), 0);

  return self->parent_id;
}

void
manuals_heading_set_parent_id (ManualsHeading *self,
                               gint64          parent_id)
{
  g_return_if_fail (MANUALS_IS_HEADING (self));
  g_return_if_fail (parent_id >= 0);

  if (self->parent_id != parent_id)
    {
      self->parent_id = parent_id;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PARENT_ID]);
    }
}

gint64
manuals_heading_get_book_id (ManualsHeading *self)
{
  g_return_val_if_fail (MANUALS_IS_HEADING (self), 0);

  return self->book_id;
}

void
manuals_heading_set_book_id (ManualsHeading *self,
                             gint64          book_id)
{
  g_return_if_fail (MANUALS_IS_HEADING (self));
  g_return_if_fail (book_id >= 0);

  if (self->book_id != book_id)
    {
      self->book_id = book_id;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_BOOK_ID]);
    }
}

DexFuture *
manuals_heading_find_parent (ManualsHeading *self)
{
  g_autoptr(GomFilter) filter = NULL;
  g_autoptr(ManualsRepository) repository = NULL;
  GValue parent_id = G_VALUE_INIT;

  g_return_val_if_fail (MANUALS_IS_HEADING (self), NULL);

  g_object_get (self,
                "repository", &repository,
                NULL);

  if (repository == NULL || self->parent_id <= 0)
    return manuals_heading_find_book (self);

  g_value_init (&parent_id, G_TYPE_INT64);
  g_value_set_int64 (&parent_id, self->parent_id);
  filter = gom_filter_new_eq (MANUALS_TYPE_HEADING, "id", &parent_id);
  g_value_unset (&parent_id);

  return manuals_repository_find_one (repository, MANUALS_TYPE_HEADING, filter);
}

DexFuture *
manuals_heading_find_sdk (ManualsHeading *self)
{
  g_autoptr(GomFilter) filter = NULL;
  g_autoptr(ManualsRepository) repository = NULL;
  GValue book_id = G_VALUE_INIT;

  g_return_val_if_fail (MANUALS_IS_HEADING (self), NULL);

  g_object_get (self,
                "repository", &repository,
                NULL);

  if (repository == NULL || self->book_id <= 0)
    return dex_future_new_reject (G_IO_ERROR,
                                  G_IO_ERROR_NOT_FOUND,
                                  "Failed to locate SDK");

  g_value_init (&book_id, G_TYPE_INT64);
  g_value_set_int64 (&book_id, self->book_id);
  filter = gom_filter_new_eq (MANUALS_TYPE_SDK, "id", &book_id);
  g_value_unset (&book_id);

  return manuals_repository_find_one (repository, MANUALS_TYPE_SDK, filter);
}

DexFuture *
manuals_heading_list_headings (ManualsHeading *self)
{
  g_autoptr(ManualsRepository) repository = NULL;
  g_autoptr(GomFilter) filter = NULL;
  g_auto(GValue) value = G_VALUE_INIT;

  g_return_val_if_fail (MANUALS_IS_HEADING (self), NULL);

  g_object_get (self, "repository", &repository, NULL);

  if (repository == NULL)
    return dex_future_new_reject (G_IO_ERROR,
                                  G_IO_ERROR_NOT_SUPPORTED,
                                  "No repository to query");

  g_value_init (&value, G_TYPE_INT64);
  g_value_set_int64 (&value, self->id);
  filter = gom_filter_new_eq (MANUALS_TYPE_HEADING, "parent-id", &value);

  return manuals_repository_list (repository, MANUALS_TYPE_HEADING, filter);
}

DexFuture *
manuals_heading_find_book (ManualsHeading *self)
{
  g_autoptr(ManualsRepository) repository = NULL;
  g_autoptr(GomFilter) filter = NULL;
  g_auto(GValue) value = G_VALUE_INIT;

  g_return_val_if_fail (MANUALS_IS_HEADING (self), NULL);

  g_object_get (self, "repository", &repository, NULL);

  if (repository == NULL)
    return dex_future_new_reject (G_IO_ERROR,
                                  G_IO_ERROR_NOT_SUPPORTED,
                                  "No repository to query");

  g_value_init (&value, G_TYPE_INT64);
  g_value_set_int64 (&value, self->book_id);
  filter = gom_filter_new_eq (MANUALS_TYPE_BOOK, "id", &value);

  return manuals_repository_find_one (repository, MANUALS_TYPE_BOOK, filter);
}

DexFuture *
manuals_heading_find_by_uri (ManualsRepository *repository,
                             const char        *uri)
{
  g_autoptr(GomFilter) filter = NULL;
  g_auto(GValue) value = G_VALUE_INIT;

  g_return_val_if_fail (MANUALS_IS_REPOSITORY (repository), NULL);
  g_return_val_if_fail (uri != NULL, NULL);

  g_value_init (&value, G_TYPE_STRING);
  g_value_set_string (&value, uri);
  filter = gom_filter_new_eq (MANUALS_TYPE_HEADING, "uri", &value);

  return manuals_repository_find_one (repository, MANUALS_TYPE_HEADING, filter);
}
