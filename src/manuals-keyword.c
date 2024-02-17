/* manuals-keyword.c
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
#include "manuals-keyword.h"
#include "manuals-repository.h"

struct _ManualsKeyword
{
  GomResource parent_instance;
  gint64 id;
  gint64 book_id;
  char *deprecated;
  char *kind;
  char *name;
  char *uri;
  char *since;
  char *stability;
};

enum {
  PROP_0,
  PROP_BOOK_ID,
  PROP_DEPRECATED,
  PROP_ID,
  PROP_KIND,
  PROP_NAME,
  PROP_URI,
  PROP_SINCE,
  PROP_STABILITY,
  N_PROPS
};

G_DEFINE_FINAL_TYPE (ManualsKeyword, manuals_keyword, GOM_TYPE_RESOURCE)

static GParamSpec *properties [N_PROPS];

static void
manuals_keyword_finalize (GObject *object)
{
  ManualsKeyword *self = (ManualsKeyword *)object;

  g_clear_pointer (&self->deprecated, g_free);
  g_clear_pointer (&self->kind, g_free);
  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->uri, g_free);
  g_clear_pointer (&self->since, g_free);
  g_clear_pointer (&self->stability, g_free);

  G_OBJECT_CLASS (manuals_keyword_parent_class)->finalize (object);
}

static void
manuals_keyword_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  ManualsKeyword *self = MANUALS_KEYWORD (object);

  switch (prop_id)
    {
    case PROP_BOOK_ID:
      g_value_set_int64 (value, manuals_keyword_get_book_id (self));
      break;

    case PROP_DEPRECATED:
      g_value_set_string (value, manuals_keyword_get_deprecated  (self));
      break;

    case PROP_ID:
      g_value_set_int64 (value, manuals_keyword_get_id  (self));
      break;

    case PROP_KIND:
      g_value_set_string (value, manuals_keyword_get_kind (self));
      break;

    case PROP_NAME:
      g_value_set_string (value, manuals_keyword_get_name (self));
      break;

    case PROP_URI:
      g_value_set_string (value, manuals_keyword_get_uri (self));
      break;

    case PROP_SINCE:
      g_value_set_string (value, manuals_keyword_get_since (self));
      break;

    case PROP_STABILITY:
      g_value_set_string (value, manuals_keyword_get_stability (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
manuals_keyword_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ManualsKeyword *self = MANUALS_KEYWORD (object);

  switch (prop_id)
    {
    case PROP_BOOK_ID:
      manuals_keyword_set_book_id (self, g_value_get_int64 (value));
      break;

    case PROP_DEPRECATED:
      manuals_keyword_set_deprecated (self, g_value_get_string (value));
      break;

    case PROP_ID:
      manuals_keyword_set_id (self, g_value_get_int64 (value));
      break;

    case PROP_KIND:
      manuals_keyword_set_kind (self, g_value_get_string (value));
      break;

    case PROP_NAME:
      manuals_keyword_set_name (self, g_value_get_string (value));
      break;

    case PROP_URI:
      manuals_keyword_set_uri (self, g_value_get_string (value));
      break;

    case PROP_SINCE:
      manuals_keyword_set_since (self, g_value_get_string (value));
      break;

    case PROP_STABILITY:
      manuals_keyword_set_stability (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
manuals_keyword_class_init (ManualsKeywordClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GomResourceClass *resource_class = GOM_RESOURCE_CLASS (klass);

  object_class->finalize = manuals_keyword_finalize;
  object_class->get_property = manuals_keyword_get_property;
  object_class->set_property = manuals_keyword_set_property;

  properties[PROP_BOOK_ID] =
    g_param_spec_int64 ("book-id", NULL, NULL,
                        0, G_MAXINT64, 0,
                        (G_PARAM_READWRITE |
                         G_PARAM_EXPLICIT_NOTIFY |
                         G_PARAM_STATIC_STRINGS));

  properties[PROP_DEPRECATED] =
    g_param_spec_string ("deprecated", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_ID] =
    g_param_spec_int64 ("id", NULL, NULL,
                        0, G_MAXINT64, 0,
                        (G_PARAM_READWRITE |
                         G_PARAM_EXPLICIT_NOTIFY |
                         G_PARAM_STATIC_STRINGS));

  properties[PROP_KIND] =
    g_param_spec_string ("kind", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_NAME] =
    g_param_spec_string ("name", NULL, NULL,
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

  properties[PROP_SINCE] =
    g_param_spec_string ("since", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  properties[PROP_STABILITY] =
    g_param_spec_string ("stability", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gom_resource_class_set_table (resource_class, "keywords");
  gom_resource_class_set_primary_key (resource_class, "id");
  gom_resource_class_set_reference (resource_class, "book-id", "books", "id");
  gom_resource_class_set_notnull (resource_class, "name");
}

static void
manuals_keyword_init (ManualsKeyword *self)
{
}

gint64
manuals_keyword_get_id (ManualsKeyword *self)
{
  g_return_val_if_fail (MANUALS_IS_KEYWORD (self), 0);

  return self->id;
}

void
manuals_keyword_set_id (ManualsKeyword *self,
                        gint64          id)
{
  g_return_if_fail (MANUALS_IS_KEYWORD (self));
  g_return_if_fail (id >= 0);

  if (self->id != id)
    {
      self->id = id;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_BOOK_ID]);
    }
}

gint64
manuals_keyword_get_book_id (ManualsKeyword *self)
{
  g_return_val_if_fail (MANUALS_IS_KEYWORD (self), 0);

  return self->book_id;
}

void
manuals_keyword_set_book_id (ManualsKeyword *self,
                             gint64          book_id)
{
  g_return_if_fail (MANUALS_IS_KEYWORD (self));
  g_return_if_fail (book_id >= 0);

  if (self->book_id != book_id)
    {
      self->book_id = book_id;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_BOOK_ID]);
    }
}

const char *
manuals_keyword_get_kind (ManualsKeyword *self)
{
  g_return_val_if_fail (MANUALS_IS_KEYWORD (self), NULL);

  return self->kind;
}

void
manuals_keyword_set_kind (ManualsKeyword *self,
                          const char     *kind)
{
  g_return_if_fail (MANUALS_IS_KEYWORD (self));

  if (g_set_str (&self->kind, kind))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_KIND]);
}

const char *
manuals_keyword_get_since (ManualsKeyword *self)
{
  g_return_val_if_fail (MANUALS_IS_KEYWORD (self), NULL);

  return self->since;
}

void
manuals_keyword_set_since (ManualsKeyword *self,
                           const char     *since)
{
  g_return_if_fail (MANUALS_IS_KEYWORD (self));

  if (g_set_str (&self->since, since))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SINCE]);
}

const char *
manuals_keyword_get_deprecated (ManualsKeyword *self)
{
  g_return_val_if_fail (MANUALS_IS_KEYWORD (self), NULL);

  return self->deprecated;
}

void
manuals_keyword_set_deprecated (ManualsKeyword *self,
                                const char     *deprecated)
{
  g_return_if_fail (MANUALS_IS_KEYWORD (self));

  if (g_set_str (&self->deprecated, deprecated))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DEPRECATED]);
}

const char *
manuals_keyword_get_name (ManualsKeyword *self)
{
  g_return_val_if_fail (MANUALS_IS_KEYWORD (self), NULL);

  return self->name;
}

void
manuals_keyword_set_name (ManualsKeyword *self,
                          const char     *name)
{
  g_return_if_fail (MANUALS_IS_KEYWORD (self));

  if (g_set_str (&self->name, name))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_NAME]);
}

const char *
manuals_keyword_get_uri (ManualsKeyword *self)
{
  g_return_val_if_fail (MANUALS_IS_KEYWORD (self), NULL);

  return self->uri;
}

void
manuals_keyword_set_uri (ManualsKeyword *self,
                         const char     *uri)
{
  g_return_if_fail (MANUALS_IS_KEYWORD (self));

  if (g_set_str (&self->uri, uri))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_URI]);
}

const char *
manuals_keyword_get_stability (ManualsKeyword *self)
{
  g_return_val_if_fail (MANUALS_IS_KEYWORD (self), NULL);

  return self->stability;
}

void
manuals_keyword_set_stability (ManualsKeyword *self,
                               const char     *stability)
{
  g_return_if_fail (MANUALS_IS_KEYWORD (self));

  if (g_set_str (&self->stability, stability))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_STABILITY]);
}

DexFuture *
manuals_keyword_find_by_uri (ManualsRepository *repository,
                             const char        *uri)
{
  g_autoptr(GomFilter) filter = NULL;
  g_auto(GValue) value = G_VALUE_INIT;

  g_return_val_if_fail (MANUALS_IS_REPOSITORY (repository), NULL);
  g_return_val_if_fail (uri != NULL, NULL);

  g_value_init (&value, G_TYPE_STRING);
  g_value_set_string (&value, uri);
  filter = gom_filter_new_eq (MANUALS_TYPE_KEYWORD, "uri", &value);

  return manuals_repository_find_one (repository, MANUALS_TYPE_KEYWORD, filter);
}

DexFuture *
manuals_keyword_find_book (ManualsKeyword *self)
{
  g_autoptr(ManualsRepository) repository = NULL;
  g_autoptr(GomFilter) filter = NULL;
  g_auto(GValue) value = G_VALUE_INIT;

  g_return_val_if_fail (MANUALS_IS_KEYWORD (self), NULL);

  g_object_get (self, "repository", &repository, NULL);
  g_value_init (&value, G_TYPE_INT64);
  g_value_set_int64 (&value, self->book_id);
  filter = gom_filter_new_eq (MANUALS_TYPE_BOOK, "id", &value);

  return manuals_repository_find_one (repository, MANUALS_TYPE_BOOK, filter);
}
