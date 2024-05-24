/*
 * manuals-search-entry.c
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

#include "manuals-search-entry.h"
#include "manuals-search-query.h"

#define SEARCH_DELAY_MSEC 200

struct _ManualsSearchEntry
{
  GtkWidget  parent_instance;

  GtkText   *text;
  GtkLabel  *count_label;

  guint      changed_source;
};

G_DEFINE_FINAL_TYPE (ManualsSearchEntry, manuals_search_entry, GTK_TYPE_WIDGET)

enum {
  BEGIN_SEARCH,
  N_SIGNALS
};

static guint signals[N_SIGNALS];

static gboolean
uint_to_label (GBinding     *binding,
               const GValue *from_value,
               GValue       *to_value,
               gpointer      user_data)
{
  guint count = g_value_get_uint (from_value);

  if (count > 0)
    /* translators: %u is replaced with the number of results */
    g_value_take_string (to_value, g_strdup_printf (_("%u Results"), count));

  return TRUE;
}

static gboolean
manuals_search_entry_timeout_cb (gpointer user_data)
{
  ManualsSearchEntry *self = user_data;
  g_autoptr(ManualsSearchQuery) query = NULL;
  const char *text;

  g_assert (MANUALS_IS_SEARCH_ENTRY (self));

  self->changed_source = 0;

  text = gtk_editable_get_text (GTK_EDITABLE (self->text));

  query = manuals_search_query_new ();
  manuals_search_query_set_text (query, text);

  g_signal_emit (self, signals[BEGIN_SEARCH], 0, query);

  g_object_bind_property_full (query, "n-items",
                               self->count_label, "label",
                               G_BINDING_SYNC_CREATE,
                               uint_to_label, NULL, NULL, NULL);

  return G_SOURCE_REMOVE;
}

static void
manuals_search_entry_text_activate_cb (ManualsSearchEntry *self,
                                       GtkText            *text)
{
  g_assert (MANUALS_IS_SEARCH_ENTRY (self));
  g_assert (GTK_IS_TEXT (text));

  gtk_widget_activate_action (GTK_WIDGET (self), "search.activate-first", NULL);
}

static void
manuals_search_entry_text_changed_cb (ManualsSearchEntry *self,
                                      GtkText            *text)
{
  g_assert (MANUALS_IS_SEARCH_ENTRY (self));
  g_assert (GTK_IS_TEXT (text));

  g_clear_handle_id (&self->changed_source, g_source_remove);

  self->changed_source = g_timeout_add_full (G_PRIORITY_LOW,
                                             SEARCH_DELAY_MSEC,
                                             manuals_search_entry_timeout_cb,
                                             g_object_ref (self),
                                             g_object_unref);
}

static gboolean
manuals_search_entry_grab_focus (GtkWidget *widget)
{
  return gtk_widget_grab_focus (GTK_WIDGET (MANUALS_SEARCH_ENTRY (widget)->text));
}

static void
manuals_search_entry_dispose (GObject *object)
{
  ManualsSearchEntry *self = (ManualsSearchEntry *)object;
  GtkWidget *child;

  gtk_widget_dispose_template (GTK_WIDGET (self), MANUALS_TYPE_SEARCH_ENTRY);

  if ((child = gtk_widget_get_first_child (GTK_WIDGET (self))))
    gtk_widget_unparent (child);

  g_clear_handle_id (&self->changed_source, g_source_remove);

  G_OBJECT_CLASS (manuals_search_entry_parent_class)->dispose (object);
}

static void
manuals_search_entry_class_init (ManualsSearchEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = manuals_search_entry_dispose;

  widget_class->grab_focus = manuals_search_entry_grab_focus;

  signals[BEGIN_SEARCH] =
    g_signal_new ("begin-search",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1, MANUALS_TYPE_SEARCH_QUERY);

  gtk_widget_class_set_template_from_resource (widget_class, "/app/devsuite/Manuals/manuals-search-entry.ui");
  gtk_widget_class_set_css_name (widget_class, "entry");
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_bind_template_child (widget_class, ManualsSearchEntry, count_label);
  gtk_widget_class_bind_template_child (widget_class, ManualsSearchEntry, text);
  gtk_widget_class_bind_template_callback (widget_class, manuals_search_entry_text_activate_cb);
  gtk_widget_class_bind_template_callback (widget_class, manuals_search_entry_text_changed_cb);
}

static void
manuals_search_entry_init (ManualsSearchEntry *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
