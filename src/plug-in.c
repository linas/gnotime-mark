/*   Report Plugins for GTimeTracker - a time tracker
 *   Copyright (C) 2001 Linas Vepstas <linas@linas.org>
 * Copyright (C) 2022      Markus Prasser
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "gtt_gsettings_io.h"

#include <glib.h>
#include <gnome.h>

#include "app.h"
#include "journal.h"
#include "menus.h"
#include "plug-in.h"
#include "util.h"

#include <gio/gio.h>

struct NewPluginDialog_s
{
  GtkDialog *dialog;
  GtkEntry *plugin_name;
  GtkFileChooser *plugin_path;
  GtkEntry *plugin_tooltip;
  GnomeApp *app;
};

/* ============================================================ */

GttPlugin *
gtt_plugin_new (const char *nam, const char *pth)
{
  GttPlugin *plg;

  if (!nam || !pth)
    return NULL;

  plg = g_new0 (GttPlugin, 1);
  plg->name = g_strdup (nam);
  plg->path = g_strdup (pth);
  plg->tooltip = NULL;

  return plg;
}

GttPlugin *
gtt_plugin_copy (GttPlugin *orig)
{
  GttPlugin *plg;

  if (!orig)
    return NULL;

  plg = g_new0 (GttPlugin, 1);
  plg->name = NULL;
  if (orig->name)
    plg->name = g_strdup (orig->name);

  plg->path = NULL;
  if (orig->path)
    plg->path = g_strdup (orig->path);

  plg->tooltip = NULL;
  if (orig->tooltip)
    plg->tooltip = g_strdup (orig->tooltip);

  plg->last_url = NULL;
  if (orig->last_url)
    plg->last_url = g_strdup (orig->last_url);

  return plg;
}

void
gtt_plugin_free (GttPlugin *plg)
{
  if (!plg)
    return;
  if (plg->name)
    g_free (plg->name);
  if (plg->path)
    g_free (plg->path);
  if (plg->tooltip)
    g_free (plg->tooltip);
  if (plg->last_url)
    g_free (plg->last_url);
}

/* ============================================================ */

static void
new_plugin_create_cb (GtkWidget *w, gpointer data)
{
  const char *title, *tip;
  NewPluginDialog *dlg = data;

  /* Get the dialog contents */
  title = gtk_entry_get_text (dlg->plugin_name);
  char *path = gtk_file_chooser_get_uri (dlg->plugin_path);
  tip = gtk_entry_get_text (dlg->plugin_tooltip);

  /* Do a basic sanity check */
  GFile *plugin_file = g_file_new_for_path (path);
  const gboolean exists = g_file_query_exists (plugin_file, NULL);
  g_object_unref (plugin_file);
  plugin_file = NULL;
  if (!exists)
    {
      gchar *msg;
      GtkWidget *mb;
      msg = g_strdup_printf (_ ("Unable to open the report file %s\n"), path);
      mb = gnome_message_box_new (msg, GNOME_MESSAGE_BOX_ERROR,
                                  GTK_STOCK_CLOSE, NULL);
      gtk_widget_show (mb);
      /* g_free (msg);   XXX memory leak needs fixing. */
    }
  else
    {
      GttPlugin *plg;
      GnomeUIInfo entry[2];

      /* Create the plugin */
      plg = gtt_plugin_new (title, path);
      plg->tooltip = g_strdup (tip);

      /* Add the thing to the Reports menu */
      entry[0].type = GNOME_APP_UI_ITEM;
      entry[0].label = plg->name;
      entry[0].hint = plg->tooltip;
      entry[0].moreinfo = invoke_report;
      entry[0].user_data = plg;
      entry[0].unused_data = NULL;
      entry[0].pixmap_type = GNOME_APP_PIXMAP_STOCK;
      entry[0].pixmap_info = GNOME_STOCK_MENU_BLANK;
      entry[0].accelerator_key = 0;
      entry[0].ac_mods = (GdkModifierType)0;

      entry[1].type = GNOME_APP_UI_ENDOFINFO;

      // gnome_app_insert_menus (dlg->app,  N_("Reports/<Separator>"), entry);

      gtt_reports_menu_prepend_entry (dlg->app, entry);

      /* Save to file, too.  That way, if system core dumps later,
       * at least we managed to get this set of changes saved. */
      gtt_save_reports_menu ();

      /* zero-out entries, so next time user doesn't see them again */
      /* Uh, no, don't
      gtk_entry_set_text (dlg->plugin_name, "");
      gtk_entry_set_text (dlg->plugin_path, "");
      gtk_entry_set_text (dlg->plugin_tooltip, "");
      */
    }
  g_free (path);
  gtk_widget_hide (GTK_WIDGET (dlg->dialog));
}

static void
new_plugin_cancel_cb (GtkWidget *w, gpointer data)
{
  NewPluginDialog *dlg = data;
  gtk_widget_hide (GTK_WIDGET (dlg->dialog));
}

/* ============================================================ */

NewPluginDialog *
new_plugin_dialog_new (void)
{
  NewPluginDialog *dlg;

  dlg = g_malloc (sizeof (NewPluginDialog));
  dlg->app = GNOME_APP (app_window);

  GtkWidget *plugin_new = gtk_dialog_new ();
  dlg->dialog = GTK_DIALOG (plugin_new);
  gtk_widget_set_name (plugin_new, "plugin new");
  gtk_window_set_type_hint (GTK_WINDOW (plugin_new),
                            GDK_WINDOW_TYPE_HINT_NORMAL);

  GtkWidget *dialog_vbox1
      = gtk_dialog_get_content_area (GTK_DIALOG (plugin_new));
  gtk_box_set_spacing (GTK_BOX (dialog_vbox1), 8);

  GtkWidget *table1 = gtk_table_new (3, 2, FALSE);
  gtk_widget_set_name (table1, "table1");

  GtkWidget *label1 = gtk_label_new (_ ("Name:"));
  gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_CENTER);
  gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);
  gtk_widget_set_name (label1, "label1");
  gtk_widget_show (label1);

  gtk_table_attach (GTK_TABLE (table1), label1, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

  GtkWidget *plugin_name = gtk_entry_new ();
  dlg->plugin_name = GTK_ENTRY (plugin_name);
  gtk_entry_set_invisible_char (GTK_ENTRY (plugin_name), '*');
  gtk_widget_set_can_focus (plugin_name, TRUE);
  gtk_widget_set_name (plugin_name, "plugin name");
  gtk_widget_set_tooltip_text (
      plugin_name, _ ("Title that will appear in the 'Reports' menu."));
  gtk_widget_show (plugin_name);

  gtk_table_attach (GTK_TABLE (table1), plugin_name, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *label2 = gtk_label_new (_ ("Path:"));
  gtk_label_set_justify (GTK_LABEL (label2), GTK_JUSTIFY_CENTER);
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
  gtk_widget_set_name (label2, "label2");
  gtk_widget_show (label2);

  gtk_table_attach (GTK_TABLE (table1), label2, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);

  GtkWidget *plugin_path = gtk_file_chooser_button_new (
      _ ("Select a File"), GTK_FILE_CHOOSER_ACTION_OPEN);
  dlg->plugin_path = GTK_FILE_CHOOSER (plugin_path);
  gtk_widget_set_events (plugin_path, GDK_BUTTON_PRESS_MASK
                                          | GDK_BUTTON_RELEASE_MASK
                                          | GDK_POINTER_MOTION_HINT_MASK
                                          | GDK_POINTER_MOTION_MASK);
  gtk_widget_set_name (plugin_path, "plugin path");
  gtk_widget_show (plugin_path);

  gtk_table_attach_defaults (GTK_TABLE (table1), plugin_path, 1, 2, 1, 2);

  GtkWidget *label3 = gtk_label_new (_ ("Tooltip:"));
  gtk_label_set_justify (GTK_LABEL (label3), GTK_JUSTIFY_CENTER);
  gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);
  gtk_widget_set_name (label3, "label3");
  gtk_widget_show (label3);

  gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 2, 3, GTK_FILL, 0, 0, 0);

  GtkWidget *plugin_tooltip = gtk_entry_new ();
  dlg->plugin_tooltip = GTK_ENTRY (plugin_tooltip);
  gtk_entry_set_invisible_char (GTK_ENTRY (plugin_tooltip), '*');
  gtk_widget_set_can_focus (plugin_tooltip, TRUE);
  gtk_widget_set_name (plugin_tooltip, "plugin tooltip");
  gtk_widget_set_tooltip_text (plugin_tooltip,
                               _ ("Tooltip that will show when the pointer "
                                  "hovers over this menu item."));
  gtk_widget_show (plugin_tooltip);

  gtk_table_attach (GTK_TABLE (table1), plugin_tooltip, 1, 2, 2, 3,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_widget_show (table1);

  gtk_box_pack_start_defaults (GTK_BOX (dialog_vbox1), table1);
  gtk_widget_show (dialog_vbox1);

  GtkWidget *dialog_action_area1
      = gtk_dialog_get_action_area (GTK_DIALOG (plugin_new));
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1),
                             GTK_BUTTONBOX_END);

  GtkWidget *ok_button = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_widget_set_can_default (ok_button, TRUE);
  gtk_widget_set_can_focus (ok_button, TRUE);
  gtk_widget_set_name (ok_button, "ok_button");
  g_signal_connect (G_OBJECT (ok_button), "clicked",
                    G_CALLBACK (new_plugin_create_cb), dlg);
  gtk_widget_show (ok_button);
  gtk_box_pack_start_defaults (GTK_BOX (dialog_action_area1), ok_button);

  GtkWidget *cancel_button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_set_can_default (cancel_button, TRUE);
  gtk_widget_set_can_focus (cancel_button, TRUE);
  gtk_widget_set_name (cancel_button, "cancel_button");
  g_signal_connect (G_OBJECT (cancel_button), "clicked",
                    G_CALLBACK (new_plugin_cancel_cb), dlg);
  gtk_widget_show (cancel_button);
  gtk_box_pack_start_defaults (GTK_BOX (dialog_action_area1), cancel_button);

  gtk_widget_show (dialog_action_area1);

  gtk_widget_show (plugin_new);

  gtk_widget_hide_on_delete (GTK_WIDGET (dlg->dialog));

  return dlg;
}

/* ============================================================ */

void
new_plugin_dialog_show (NewPluginDialog *dlg)
{
  if (!dlg)
    return;
  gtk_widget_show (GTK_WIDGET (dlg->dialog));
}

void
new_plugin_dialog_destroy (NewPluginDialog *dlg)
{
  if (!dlg)
    return;
  gtk_widget_destroy (GTK_WIDGET (dlg->dialog));
  g_free (dlg);
}

/* ============================================================ */

static NewPluginDialog *pdlg = NULL;

void
new_report (GtkWidget *widget, gpointer data)
{
  if (!pdlg)
    pdlg = new_plugin_dialog_new ();
  gtk_widget_show (GTK_WIDGET (pdlg->dialog));
}

/* ====================== END OF FILE ========================= */
