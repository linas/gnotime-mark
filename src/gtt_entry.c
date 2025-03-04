/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * Copyright (C) 2023      Markus Prasser
 * All rights reserved.
 *
 * This file is part of GnoTime (originally the Gnome Library).
 *
 * GnoTime is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Library General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * GnoTime is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Library General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with the GnoTime; see the file COPYING.LIB.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
/*
  @NOTATION@
 */

/* GttEntry widget - combo box with auto-saved history
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#undef GTK_DISABLE_DEPRECATED

#include "config.h"

#include "gtt_entry.h"

#include <gtk/gtk.h>

#include <gio/gio.h>
#include <glib/gi18n.h>

#include <stdio.h>
#include <string.h>

enum
{
  PROP_0,
  PROP_HISTORY_ID,
  PROP_GTK_ENTRY
};

#define DEFAULT_MAX_HISTORY_SAVED 10 // This seems to make more sense than 60

struct _GttEntryPrivate
{
  gchar *history_id;

  GList *items;

  guint16 max_saved;
  guint changed : 1;
  guint saving_history : 1;
  GSettings *settings;
#ifdef GTT_ENTRY_HISTORY_TODO
  guint gconf_notify_id;
#endif // GTT_ENTRY_HISTORY_TODO
};

struct item
{
  gboolean save;
  gchar *text;
};

static void gtt_entry_class_init (GttEntryClass *class);
static void gtt_entry_init (GttEntry *gentry);
static void gtt_entry_finalize (GObject *object);
static void gtt_entry_destroy (GtkObject *object);

static void gtt_entry_get_property (GObject *object, guint param_id,
                                    GValue *value, GParamSpec *pspec);
static void gtt_entry_set_property (GObject *object, guint param_id,
                                    const GValue *value, GParamSpec *pspec);
#ifdef GTT_ENTRY_EDITABLE_TODO
static void gtt_entry_editable_init (GtkEditableClass *iface);
#endif // GTT_ENTRY_EDITABLE_TODO
static void gtt_entry_load_history (GttEntry *gentry);
static void gtt_entry_save_history (GttEntry *gentry);

static char *build_gsettings_path (GttEntry *gentry);

G_DEFINE_TYPE (GttEntry, gtt_entry, GTK_TYPE_COMBO)

enum
{
  ACTIVATE_SIGNAL,
  LAST_SIGNAL
};
static int gtt_entry_signals[LAST_SIGNAL] = { 0 };

static gboolean
gtt_entry_mnemonic_activate (GtkWidget *widget, gboolean group_cycling)
{
  gboolean handled;
  GttEntry *entry;

  entry = GTT_ENTRY (widget);

  group_cycling = group_cycling != FALSE;

  if (!GTK_WIDGET_IS_SENSITIVE (GTK_COMBO (entry)->entry))
    handled = TRUE;
  else
    g_signal_emit_by_name (GTK_COMBO (entry)->entry, "mnemonic_activate",
                           group_cycling, &handled);

  return handled;
}

static void
gtt_entry_class_init (GttEntryClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GObjectClass *gobject_class;

  object_class = (GtkObjectClass *)class;
  gobject_class = (GObjectClass *)class;
  widget_class = (GtkWidgetClass *)class;

  gtt_entry_signals[ACTIVATE_SIGNAL] = g_signal_new (
      "activate", G_TYPE_FROM_CLASS (gobject_class), G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (GttEntryClass, activate), NULL, NULL,
      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  class->activate = NULL;

  object_class->destroy = gtt_entry_destroy;
  gobject_class->finalize = gtt_entry_finalize;
  gobject_class->set_property = gtt_entry_set_property;
  gobject_class->get_property = gtt_entry_get_property;
  widget_class->mnemonic_activate = gtt_entry_mnemonic_activate;

  g_object_class_install_property (
      gobject_class, PROP_HISTORY_ID,
      g_param_spec_string ("history_id", _ ("History ID"), _ ("History ID"),
                           NULL, G_PARAM_READWRITE));
  g_object_class_install_property (
      gobject_class, PROP_GTK_ENTRY,
      g_param_spec_object ("gtk_entry", _ ("GTK entry"), _ ("The GTK entry"),
                           GTK_TYPE_ENTRY, G_PARAM_READABLE));
}

static void
gtt_entry_get_property (GObject *object, guint param_id, GValue *value,
                        GParamSpec *pspec)
{
  GttEntry *entry = GTT_ENTRY (object);

  switch (param_id)
    {
    case PROP_HISTORY_ID:
      g_value_set_string (value, entry->_priv->history_id);
      break;
    case PROP_GTK_ENTRY:
      g_value_set_object (value, gtt_entry_gtk_entry (entry));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
gtt_entry_set_property (GObject *object, guint param_id, const GValue *value,
                        GParamSpec *pspec)
{
  GttEntry *entry = GTT_ENTRY (object);

  switch (param_id)
    {
    case PROP_HISTORY_ID:
      gtt_entry_set_history_id (entry, g_value_get_string (value));
      gtt_entry_load_history (entry);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
entry_changed (GtkWidget *widget, gpointer data)
{
  GttEntry *gentry;

  gentry = data;
  gentry->_priv->changed = TRUE;

  g_signal_emit_by_name (gentry, "changed");
}

static void
entry_activated (GtkWidget *widget, gpointer data)
{
  GttEntry *gentry;
  const gchar *text;

  gentry = data;

  text = gtk_entry_get_text (GTK_ENTRY (widget));

  if (!gentry->_priv->changed || (strcmp (text, "") == 0))
    {
      gentry->_priv->changed = FALSE;
    }
  else
    {
      gtt_entry_prepend_history (gentry, TRUE,
                                 gtk_entry_get_text (GTK_ENTRY (widget)));
    }

  g_signal_emit (gentry, gtt_entry_signals[ACTIVATE_SIGNAL], 0);
}

static void
gtt_entry_init (GttEntry *gentry)
{
  gentry->_priv = g_new0 (GttEntryPrivate, 1);

  gentry->_priv->changed = FALSE;
  gentry->_priv->history_id = NULL;
  gentry->_priv->items = NULL;
  gentry->_priv->max_saved = DEFAULT_MAX_HISTORY_SAVED;

  g_signal_connect (gtt_entry_gtk_entry (gentry), "changed",
                    G_CALLBACK (entry_changed), gentry);
  g_signal_connect (gtt_entry_gtk_entry (gentry), "activate",
                    G_CALLBACK (entry_activated), gentry);
  gtk_combo_disable_activate (GTK_COMBO (gentry));
  gtk_combo_set_case_sensitive (GTK_COMBO (gentry), TRUE);
}

/**
 * gtt_entry_new
 * @history_id: If not %NULL, the text id under which history data is stored
 *
 * Description: Creates a new GttEntry widget.  If  @history_id is
 * not %NULL, then the history list will be saved and restored between
 * uses under the given id.
 *
 * Returns: Newly-created GttEntry widget.
 */
GtkWidget *
gtt_entry_new (const gchar *history_id)
{
  GttEntry *gentry;

  gentry = g_object_new (GTT_TYPE_ENTRY, "history_id", history_id, NULL);

  return GTK_WIDGET (gentry);
}

static void
free_item (gpointer data, gpointer user_data)
{
  struct item *item;

  item = data;
  g_free (item->text);
  g_free (item);
}

static void
free_items (GttEntry *gentry)
{
  g_list_foreach (gentry->_priv->items, free_item, NULL);
  g_list_free (gentry->_priv->items);
  gentry->_priv->items = NULL;
}

static void
gtt_entry_destroy (GtkObject *object)
{
  GttEntry *gentry;

  /* Note: destroy can run multiple times */

  g_return_if_fail (object != NULL);
  g_return_if_fail (GTT_IS_ENTRY (object));

  gentry = GTT_ENTRY (object);

  if (gentry->_priv->settings != NULL)
    {
#ifdef GTT_ENTRY_HISTORY_TODO
      if (gentry->_priv->gconf_notify_id != 0)
        {
          gconf_client_notify_remove (gentry->_priv->gconf_client,
                                      gentry->_priv->gconf_notify_id);
          gentry->_priv->gconf_notify_id = 0;
        }
#endif // GTT_ENTRY_HISTORY_TODO

      g_clear_object (&gentry->_priv->settings);
    }

  GTK_OBJECT_CLASS (gtt_entry_parent_class)->destroy (object);
}

static void
gtt_entry_finalize (GObject *object)
{
  GttEntry *gentry;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GTT_IS_ENTRY (object));

  gentry = GTT_ENTRY (object);

  g_free (gentry->_priv->history_id);
  gentry->_priv->history_id = NULL;

  free_items (gentry);

  g_free (gentry->_priv);
  gentry->_priv = NULL;

  G_OBJECT_CLASS (gtt_entry_parent_class)->finalize (object);
}

/**
 * gtt_entry_gtk_entry
 * @gentry: Pointer to GttEntry object.
 *
 * Description: Obtain pointer to GttEntry's internal text entry
 *
 * Returns: Pointer to GtkEntry widget.
 */
GtkWidget *
gtt_entry_gtk_entry (GttEntry *gentry)
{
  g_return_val_if_fail (gentry != NULL, NULL);
  g_return_val_if_fail (GTT_IS_ENTRY (gentry), NULL);

  return GTK_COMBO (gentry)->entry;
}

#ifdef GTT_ENTRY_HISTORY_TODO
static void
gtt_entry_history_changed (GConfClient *client, guint cnxn_id,
                           GConfEntry *entry, gpointer user_data)
{
  GttEntry *gentry;

  GDK_THREADS_ENTER ();

  gentry = GTT_ENTRY (user_data);

  /* If we're getting a notification from saving our own
   * history, don't reload it.
   */
  if (gentry->_priv->saving_history)
    {
      gentry->_priv->saving_history = FALSE;

      goto end;
    }

  gtt_entry_load_history (gentry);

end:
  GDK_THREADS_LEAVE ();
}
#endif // GTT_ENTRY_HISTORY_TODO

/* FIXME: Make this static */

/**
 * gtt_entry_set_history_id
 * @gentry: Pointer to GttEntry object.
 * @history_id: the text id under which history data is stored
 *
 * Description: Set the id of the history list. This function cannot be
 * used to change and already existing id.
 */
void
gtt_entry_set_history_id (GttEntry *gentry, const gchar *history_id)
{
  g_return_if_fail (gentry != NULL);
  g_return_if_fail (GTT_IS_ENTRY (gentry));

  if (gentry->_priv->history_id != NULL)
    {
      g_warning ("The program is trying to change an existing "
                 "GttEntry history id. This operation is "
                 "not allowed.");

      return;
    }

  if (history_id == NULL)
    return;

  gentry->_priv->history_id = g_strdup (history_id);

  /* Register with gconf */
  gchar *path = build_gsettings_path (gentry);

  gentry->_priv->settings = g_settings_new_with_path (
      "com.github.goedson.gnotime.entry-histories", path);

#ifdef GTT_ENTRY_HISTORY_TODO
  gentry->_priv->gconf_notify_id = gconf_client_notify_add (
      gentry->_priv->gconf_client, key, gtt_entry_history_changed, gentry,
      NULL, NULL);
#endif // GTT_ENTRY_HISTORY_TODO

  g_free (path);
  path = NULL;
}

/**
 * gtt_entry_get_history_id
 * @gentry: Pointer to GttEntry object.
 *
 * Description: Returns the current history id of the GttEntry widget.
 *
 * Returns: The current history id.
 */
const gchar *
gtt_entry_get_history_id (GttEntry *gentry)
{
  g_return_val_if_fail (gentry != NULL, NULL);
  g_return_val_if_fail (GTT_IS_ENTRY (gentry), NULL);

  return gentry->_priv->history_id;
}

/**
 * gtt_entry_set_max_saved
 * @gentry: Pointer to GttEntry object.
 * @max_saved: Maximum number of history items to save
 *
 * Description: Set internal limit on number of history items saved
 * to the config file.
 * Zero is an acceptable value for @max_saved, but the same thing is
 * accomplished by setting the history id of @gentry to %NULL.
 */
void
gtt_entry_set_max_saved (GttEntry *gentry, guint max_saved)
{
  g_return_if_fail (gentry != NULL);
  g_return_if_fail (GTT_IS_ENTRY (gentry));

  gentry->_priv->max_saved = max_saved;
}

/**
 * gtt_entry_get_max_saved
 * @gentry: Pointer to GttEntry object.
 *
 * Description: Get internal limit on number of history items saved
 * to the config file
 * See #gtt_entry_set_max_saved().
 *
 * Returns: An unsigned integer
 */
guint
gtt_entry_get_max_saved (GttEntry *gentry)
{
  g_return_val_if_fail (gentry != NULL, 0);
  g_return_val_if_fail (GTT_IS_ENTRY (gentry), 0);

  return gentry->_priv->max_saved;
}

static char *
build_gsettings_path (GttEntry *gentry)
{
  return g_strdup_printf (
      "/com/github/goedson/gnotime/entry-histories/entry-history-%s/",
      gentry->_priv->history_id);
}

static void
set_combo_items (GttEntry *gentry)
{
  GttEntryPrivate *priv;
  GList *l;
  char *text;

  priv = gentry->_priv;

  /* Save the contents of the entry so that setting the new list of
   * strings will not overwrite it.
   */

  text = gtk_editable_get_chars (GTK_EDITABLE (GTK_COMBO (gentry)->entry), 0,
                                 -1);

  /* Build the list of strings out of our items */

  if (priv->items)
    {
      GList *strings = NULL;

      for (l = priv->items; l; l = l->next)
        {
          struct item *item = l->data;

          strings = g_list_prepend (strings, item->text);
        }

      strings = g_list_reverse (strings);

      gtk_combo_set_popdown_strings (GTK_COMBO (gentry), strings);

      g_list_free (strings);
    }
  else
    {
      gtk_list_clear_items (GTK_LIST (GTK_COMBO (gentry)->list), 0, -1);
    }

  /* Restore the text in the entry and clear our changed flag. */

  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (gentry)->entry), text);
  g_free (text);

  priv->changed = FALSE;
}

static void
gtt_entry_add_history (GttEntry *gentry, gboolean save, const gchar *text,
                       gboolean append)
{
  GttEntryPrivate *priv;
  struct item *item;
  GList *list;
  gboolean changed;

  g_return_if_fail (gentry != NULL);
  g_return_if_fail (GTT_IS_ENTRY (gentry));
  g_return_if_fail (
      text != NULL); /* FIXME: should we just return without warning? */

  priv = gentry->_priv;

  item = g_new (struct item, 1);
  item->save = save;
  item->text = g_strdup (text);

  /* Remove duplicates */
  changed = FALSE;
  list = priv->items;

  while (list != NULL)
    {
      struct item *data = list->data;

      if (strcmp (data->text, text) == 0)
        {
          free_item (data, NULL);
          priv->items = g_list_delete_link (priv->items, list);

          changed = TRUE;
          break;
        }

      list = g_list_next (list);
    }

  /* Really add the history item */
  if (append)
    priv->items = g_list_append (priv->items, item);
  else
    priv->items = g_list_prepend (priv->items, item);

  /* Update combo list */
  set_combo_items (gentry);

  /* Save the history if needed */
  if (changed || save)
    gtt_entry_save_history (gentry);
}

/**
 * gtt_entry_prepend_history
 * @gentry: Pointer to GttEntry object.
 * @save: If %TRUE, history entry will be saved to config file
 * @text: Text to add
 *
 * Description: Adds a history item of the given @text to the head of
 * the history list inside @gentry.  If @save is %TRUE, the history
 * item will be saved in the config file (assuming that @gentry's
 * history id is not %NULL).
 * Duplicates are automatically removed from the history list.
 * The history list is automatically saved if needed.
 */
void
gtt_entry_prepend_history (GttEntry *gentry, gboolean save, const gchar *text)
{
  g_return_if_fail (gentry != NULL);
  g_return_if_fail (GTT_IS_ENTRY (gentry));
  g_return_if_fail (
      text != NULL); /* FIXME: should we just return without warning? */

  gtt_entry_add_history (gentry, save, text, FALSE);
}

/**
 * gtt_entry_append_history
 * @gentry: Pointer to GttEntry object.
 * @save: If %TRUE, history entry will be saved to config file
 * @text: Text to add
 *
 * Description: Adds a history item of the given @text to the tail
 * of the history list inside @gentry.  If @save is %TRUE, the
 * history item will be saved in the config file (assuming that
 * @gentry's history id is not %NULL).
 * Duplicates are automatically removed from the history list.
 * The history list is automatically saved if needed.
 */
void
gtt_entry_append_history (GttEntry *gentry, gboolean save, const gchar *text)
{
  g_return_if_fail (gentry != NULL);
  g_return_if_fail (GTT_IS_ENTRY (gentry));
  g_return_if_fail (
      text != NULL); /* FIXME: should we just return without warning? */

  gtt_entry_add_history (gentry, save, text, TRUE);
}

static void
gtt_entry_load_history (GttEntry *gentry)
{
  struct item *item;

  g_return_if_fail (gentry != NULL);
  g_return_if_fail (GTT_IS_ENTRY (gentry));

  if (gentry->_priv->history_id == NULL)
    return;

  g_return_if_fail (gentry->_priv->settings != NULL);

  free_items (gentry);

  gchar **gsettings_items
      = g_settings_get_strv (gentry->_priv->settings, "history");

  gsize item_idx;
  for (item_idx = 0; NULL != gsettings_items[item_idx]; ++item_idx)
    {

      item = g_new (struct item, 1);
      item->save = TRUE;
      item->text = g_strdup (gsettings_items[item_idx]);

      g_free (gsettings_items[item_idx]);
      gsettings_items[item_idx] = NULL;

      gentry->_priv->items = g_list_append (gentry->_priv->items, item);
    }

  set_combo_items (gentry);

  g_free (gsettings_items);
  gsettings_items = NULL;
}

/**
 * gtt_entry_clear_history
 * @gentry: Pointer to GttEntry object.
 *
 * Description:  Clears the history.
 */
void
gtt_entry_clear_history (GttEntry *gentry)
{
  g_return_if_fail (gentry != NULL);
  g_return_if_fail (GTT_IS_ENTRY (gentry));

  free_items (gentry);

  set_combo_items (gentry);

  gtt_entry_save_history (gentry);
}

static gboolean
check_for_duplicates (const gchar **gsettings_items, const struct item *item)
{

  const gchar *gsettings_item;
  for (gsettings_item = gsettings_items[0]; NULL != gsettings_item;
       ++gsettings_item)
    {

      if (strcmp (gsettings_item, item->text) == 0)
        {
          return FALSE;
        }
    }

  return TRUE;
}

static void
gtt_entry_save_history (GttEntry *gentry)
{
  GList *items;
  struct item *item;
  gint n;

  g_return_if_fail (gentry != NULL);
  g_return_if_fail (GTT_IS_ENTRY (gentry));

  if (gentry->_priv->history_id == NULL)
    return;

  g_return_if_fail (gentry->_priv->settings != NULL);

  const gsize n_items = g_list_length (gentry->_priv->items);
  const gchar **gsettings_items = g_new0 (const gchar *, n_items);

  gsize item_idx;
  for (item_idx = 0, n = 0, items = gentry->_priv->items;
       items && n < gentry->_priv->max_saved; items = items->next, n++)
    {
      item = items->data;

      if (item->save && check_for_duplicates (gsettings_items, item))
        {
          gsettings_items[item_idx] = item->text;
          ++item_idx;
        }
    }

  /* Save the list */
  gentry->_priv->saving_history = TRUE;
  g_settings_set_strv (gentry->_priv->settings, "history", gsettings_items);

  g_free (gsettings_items);
  gsettings_items = NULL;
}

#ifdef GTT_ENTRY_EDITABLE_TODO
static void
insert_text (GtkEditable *editable, const gchar *text, gint length,
             gint *position)
{
  GtkWidget *entry = gtt_entry_gtk_entry (GTT_ENTRY (editable));
  gtk_editable_insert_text (GTK_EDITABLE (entry), text, length, position);
}

static void
delete_text (GtkEditable *editable, gint start_pos, gint end_pos)
{
  GtkWidget *entry = gtt_entry_gtk_entry (GTT_ENTRY (editable));
  gtk_editable_delete_text (GTK_EDITABLE (entry), start_pos, end_pos);
}

static gchar *
get_chars (GtkEditable *editable, gint start_pos, gint end_pos)
{
  GtkWidget *entry = gtt_entry_gtk_entry (GTT_ENTRY (editable));
  return gtk_editable_get_chars (GTK_EDITABLE (entry), start_pos, end_pos);
}

static void
set_selection_bounds (GtkEditable *editable, gint start_pos, gint end_pos)
{
  GtkWidget *entry = gtt_entry_gtk_entry (GTT_ENTRY (editable));
  gtk_editable_select_region (GTK_EDITABLE (entry), start_pos, end_pos);
}

static gboolean
get_selection_bounds (GtkEditable *editable, gint *start_pos, gint *end_pos)
{
  GtkWidget *entry = gtt_entry_gtk_entry (GTT_ENTRY (editable));
  return gtk_editable_get_selection_bounds (GTK_EDITABLE (entry), start_pos,
                                            end_pos);
}

static void
set_position (GtkEditable *editable, gint position)
{
  GtkWidget *entry = gtt_entry_gtk_entry (GTT_ENTRY (editable));
  gtk_editable_set_position (GTK_EDITABLE (entry), position);
}

static gint
get_position (GtkEditable *editable)
{
  GtkWidget *entry = gtt_entry_gtk_entry (GTT_ENTRY (editable));
  return gtk_editable_get_position (GTK_EDITABLE (entry));
}

/* Copied from gtkentry */
static void
do_insert_text (GtkEditable *editable, const gchar *new_text,
                gint new_text_length, gint *position)
{
  GtkEntry *entry = GTK_ENTRY (gtt_entry_gtk_entry (GTT_ENTRY (editable)));
  gchar buf[64];
  gchar *text;

  if (*position < 0 || *position > entry->text_length)
    *position = entry->text_length;

  g_object_ref (G_OBJECT (editable));

  if (new_text_length <= 63)
    text = buf;
  else
    text = g_new (gchar, new_text_length + 1);

  text[new_text_length] = '\0';
  strncpy (text, new_text, new_text_length);

  g_signal_emit_by_name (editable, "insert_text", text, new_text_length,
                         position);

  if (new_text_length > 63)
    g_free (text);

  g_object_unref (G_OBJECT (editable));
}

/* Copied from gtkentry */
static void
do_delete_text (GtkEditable *editable, gint start_pos, gint end_pos)
{
  GtkEntry *entry = GTK_ENTRY (gtt_entry_gtk_entry (GTT_ENTRY (editable)));

  if (end_pos < 0 || end_pos > entry->text_length)
    end_pos = entry->text_length;
  if (start_pos < 0)
    start_pos = 0;
  if (start_pos > end_pos)
    start_pos = end_pos;

  g_object_ref (G_OBJECT (editable));

  g_signal_emit_by_name (editable, "delete_text", start_pos, end_pos);

  g_object_unref (G_OBJECT (editable));
}

static void
gtt_entry_editable_init (GtkEditableClass *iface)
{
  /* Just proxy to the GtkEntry */
  iface->do_insert_text = do_insert_text;
  iface->do_delete_text = do_delete_text;
  iface->insert_text = insert_text;
  iface->delete_text = delete_text;
  iface->get_chars = get_chars;
  iface->set_selection_bounds = set_selection_bounds;
  iface->get_selection_bounds = get_selection_bounds;
  iface->set_position = set_position;
  iface->get_position = get_position;
}
#endif // GTT_ENTRY_EDITABLE_TODO
