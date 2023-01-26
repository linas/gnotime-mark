/*   Edit Interval Properties for GTimeTracker - a time tracker
 *   Copyright (C) 2001,2003 Linas Vepstas <linas@linas.org>
 * Copyright (C) 2023      Markus Prasser
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

#include "gtt_props_dlg_interval.h"

#include <glib.h>
#include <glib/gi18n.h>

#include <stdio.h>
#include <string.h>

#include "gtt_date_edit.h"
#include "gtt_project.h"
#include "gtt_util.h"

struct EditIntervalDialog_s
{
  GttInterval *interval;
  GtkWidget *interval_edit;
  GtkWidget *start_widget;
  GtkWidget *stop_widget;
  GtkWidget *fuzz_widget;
};

/* ============================================================== */
/* interval dialog edits */

static void
hide_dialog (GtkDialog *const dialog, G_GNUC_UNUSED gpointer user_data)
{
  gtk_widget_hide (GTK_WIDGET (dialog));
}

static void
interval_edit_apply_cb (GtkWidget *w, gpointer data)
{
  EditIntervalDialog *dlg = (EditIntervalDialog *)data;
  GtkWidget *menu, *menu_item;
  GttTask *task;
  GttProject *prj;
  time_t start, stop, tmp;
  int fuzz, min_invl;

  start = gtt_date_edit_get_time (GTT_DATE_EDIT (dlg->start_widget));
  stop = gtt_date_edit_get_time (GTT_DATE_EDIT (dlg->stop_widget));

  /* If user reversed start and stop, flip them back */
  if (start > stop)
    {
      tmp = start;
      start = stop;
      stop = tmp;
    }

  /* Caution: we must avoid setting very short time intervals
   * through this interface; otherwise the interval will get
   * scrubbed away on us, and we'll be holding an invalid pointer.
   * In fact, we should probably assume the pointer is invalid
   * if prj is null ...
   */

  task = gtt_interval_get_parent (dlg->interval);
  prj = gtt_task_get_parent (task);
  min_invl = gtt_project_get_min_interval (prj);
  if (min_invl >= stop - start)
    stop = start + min_invl + 1;

  gtt_interval_freeze (dlg->interval);
  gtt_interval_set_start (dlg->interval, start);
  gtt_interval_set_stop (dlg->interval, stop);

  menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (dlg->fuzz_widget));
  menu_item = gtk_menu_get_active (GTK_MENU (menu));
  fuzz = GPOINTER_TO_INT (
      g_object_get_data (G_OBJECT (menu_item), "fuzz_factor"));

  gtt_interval_set_fuzz (dlg->interval, fuzz);

  /* The thaw may cause  the interval to change.  If so, redo the GUI. */
  dlg->interval = gtt_interval_thaw (dlg->interval);
  edit_interval_set_interval (dlg, dlg->interval);
}

static void
interval_edit_ok_cb (GtkWidget *w, gpointer data)
{
  EditIntervalDialog *dlg = (EditIntervalDialog *)data;
  interval_edit_apply_cb (w, data);
  gtk_widget_hide (dlg->interval_edit);
  dlg->interval = NULL;
}

static void
interval_edit_cancel_cb (GtkWidget *w, gpointer data)
{
  EditIntervalDialog *dlg = (EditIntervalDialog *)data;
  gtk_widget_hide (dlg->interval_edit);
  dlg->interval = NULL;
}

/* ============================================================== */
/* Set values into interval editor widgets */

void
edit_interval_set_interval (EditIntervalDialog *dlg, GttInterval *ivl)
{
  GtkWidget *w;
  GtkOptionMenu *fw;
  time_t start, stop;
  int fuzz;

  if (!dlg)
    return;
  dlg->interval = ivl;

  if (!ivl)
    {
      w = dlg->start_widget;
      gtt_date_edit_set_time (GTT_DATE_EDIT (w), 0);
      w = dlg->stop_widget;
      gtt_date_edit_set_time (GTT_DATE_EDIT (w), 0);

      fw = GTK_OPTION_MENU (dlg->fuzz_widget);
      gtk_option_menu_set_history (fw, 0);
      return;
    }

  w = dlg->start_widget;
  start = gtt_interval_get_start (ivl);
  gtt_date_edit_set_time (GTT_DATE_EDIT (w), start);

  w = dlg->stop_widget;
  stop = gtt_interval_get_stop (ivl);
  gtt_date_edit_set_time (GTT_DATE_EDIT (w), stop);

  fuzz = gtt_interval_get_fuzz (dlg->interval);
  fw = GTK_OPTION_MENU (dlg->fuzz_widget);

  /* OK, now set the initial value */
  gtk_option_menu_set_history (fw, 0);
  if (90 < fuzz)
    gtk_option_menu_set_history (fw, 1);
  if (450 < fuzz)
    gtk_option_menu_set_history (fw, 2);
  if (750 < fuzz)
    gtk_option_menu_set_history (fw, 3);
  if (1050 < fuzz)
    gtk_option_menu_set_history (fw, 4);
  if (1500 < fuzz)
    gtk_option_menu_set_history (fw, 5);
  if (2700 < fuzz)
    gtk_option_menu_set_history (fw, 6);
  if (5400 < fuzz)
    gtk_option_menu_set_history (fw, 7);
  if (9000 < fuzz)
    gtk_option_menu_set_history (fw, 8);
  if (6 * 3600 < fuzz)
    gtk_option_menu_set_history (fw, 9);
}

/* ============================================================== */
/* interval popup actions */

EditIntervalDialog *
edit_interval_dialog_new (void)
{
  EditIntervalDialog *dlg;
  GtkWidget *w, *menu, *menu_item;

  dlg = g_malloc (sizeof (EditIntervalDialog));
  dlg->interval = NULL;

  GtkWidget *const interval_edit = gtk_dialog_new ();
  dlg->interval_edit = interval_edit;
  gtk_dialog_set_has_separator (GTK_DIALOG (interval_edit), TRUE);
  gtk_widget_set_name (interval_edit, "Interval Edit");
  gtk_window_set_destroy_with_parent (GTK_WINDOW (interval_edit), FALSE);
  gtk_window_set_modal (GTK_WINDOW (interval_edit), FALSE);
  gtk_window_set_position (GTK_WINDOW (interval_edit), GTK_WIN_POS_NONE);
  gtk_window_set_resizable (GTK_WINDOW (interval_edit), TRUE);
  gtk_window_set_title (GTK_WINDOW (interval_edit), _ (""));
  g_signal_connect_after (G_OBJECT (interval_edit), "close",
                          G_CALLBACK (hide_dialog), NULL);

  GtkWidget *const action_area
      = gtk_dialog_get_action_area (GTK_DIALOG (interval_edit));
  gtk_button_box_set_layout (GTK_BUTTON_BOX (action_area), GTK_BUTTONBOX_END);

  GtkWidget *const ok_button = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_button_set_relief (GTK_BUTTON (ok_button), GTK_RELIEF_NORMAL);
  gtk_widget_set_can_default (ok_button, TRUE);
  gtk_widget_set_can_focus (ok_button, TRUE);
  gtk_widget_set_name (ok_button, "ok_button");
  g_signal_connect (G_OBJECT (ok_button), "clicked",
                    G_CALLBACK (interval_edit_ok_cb), dlg);
  gtk_widget_show (ok_button);

  gtk_box_pack_start (GTK_BOX (action_area), ok_button, TRUE, TRUE, 0);

  GtkWidget *const apply_button = gtk_button_new_from_stock (GTK_STOCK_APPLY);
  gtk_button_set_relief (GTK_BUTTON (apply_button), GTK_RELIEF_NORMAL);
  gtk_widget_set_can_default (apply_button, TRUE);
  gtk_widget_set_can_focus (apply_button, TRUE);
  gtk_widget_set_name (apply_button, "apply_button");
  g_signal_connect (G_OBJECT (apply_button), "clicked",
                    G_CALLBACK (interval_edit_apply_cb), dlg);
  gtk_widget_show (apply_button);

  gtk_box_pack_start (GTK_BOX (action_area), apply_button, TRUE, TRUE, 0);

  GtkWidget *const cancel_button
      = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_button_set_relief (GTK_BUTTON (cancel_button), GTK_RELIEF_NORMAL);
  gtk_widget_set_can_default (cancel_button, TRUE);
  gtk_widget_set_can_focus (cancel_button, TRUE);
  gtk_widget_set_name (cancel_button, "cancel_button");
  g_signal_connect (G_OBJECT (cancel_button), "clicked",
                    G_CALLBACK (interval_edit_cancel_cb), dlg);
  gtk_widget_show (cancel_button);

  gtk_box_pack_start (GTK_BOX (action_area), cancel_button, TRUE, TRUE, 0);

  GtkWidget *const help_button = gtk_button_new_from_stock (GTK_STOCK_HELP);
  gtk_button_set_relief (GTK_BUTTON (help_button), GTK_RELIEF_NORMAL);
  gtk_widget_set_can_default (help_button, TRUE);
  gtk_widget_set_can_focus (help_button, TRUE);
  gtk_widget_set_name (help_button, "help_button");
  gtk_widget_show (help_button);

  gtk_box_pack_start (GTK_BOX (action_area), help_button, TRUE, TRUE, 0);

  GtkWidget *const content_area
      = gtk_dialog_get_content_area (GTK_DIALOG (interval_edit));

  GtkWidget *const table1 = gtk_table_new (3, 2, FALSE);
  gtk_widget_set_name (table1, "table1");

  GtkWidget *const label1 = gtk_label_new (_ ("Start"));
  gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label1), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label1), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label1), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label1), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label1), 0, 0);
  gtk_widget_set_name (label1, "label1");
  gtk_widget_show (label1);

  gtk_table_attach (GTK_TABLE (table1), label1, 0, 1, 0, 1, GTK_FILL, 0, 10,
                    6);

  GtkWidget *start_date = gtt_date_edit_new_flags (
      0, GTT_DATE_EDIT_24_HR | GTT_DATE_EDIT_DISPLAY_SECONDS
             | GTT_DATE_EDIT_SHOW_TIME);
  dlg->start_widget = start_date;
  gtt_date_edit_set_popup_range (GTT_DATE_EDIT (start_date), 7, 19);
  gtk_widget_set_name (start_date, "start_date");
  gtk_widget_show (start_date);

  gtk_table_attach (GTK_TABLE (table1), start_date, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 4, 0);

  GtkWidget *const label2 = gtk_label_new (_ ("Stop"));
  gtk_label_set_justify (GTK_LABEL (label2), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label2), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label2), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label2), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label2), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label2), 0, 0);
  gtk_widget_set_name (label2, "label2");
  gtk_widget_show (label2);

  gtk_table_attach (GTK_TABLE (table1), label2, 0, 1, 1, 2, GTK_FILL, 0, 10,
                    6);

  GtkWidget *const stop_date = gtt_date_edit_new_flags (
      0, GTT_DATE_EDIT_24_HR | GTT_DATE_EDIT_DISPLAY_SECONDS
             | GTT_DATE_EDIT_SHOW_TIME);
  dlg->stop_widget = stop_date;
  gtt_date_edit_set_popup_range (GTT_DATE_EDIT (stop_date), 7, 19);
  gtk_widget_set_name (stop_date, "stop_date");
  gtk_widget_show (stop_date);

  gtk_table_attach (GTK_TABLE (table1), stop_date, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, 0, 4, 0);

  GtkWidget *const label3 = gtk_label_new (_ ("Start Fuzz"));
  gtk_label_set_justify (GTK_LABEL (label3), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label3), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label3), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label3), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label3), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label3), 0, 0);
  gtk_widget_set_name (label3, "label3");
  gtk_widget_show (label3);

  gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 2, 3, GTK_FILL, 0, 10,
                    6);

  GtkWidget *const fuzz_menu = gtk_option_menu_new ();
  dlg->fuzz_widget = fuzz_menu;
  gtk_widget_set_can_focus (fuzz_menu, TRUE);
  gtk_widget_set_name (fuzz_menu, "fuzz_menu");
  gtk_widget_set_tooltip_text (fuzz_menu,
                               _ ("Set how Uncertain the Start Time Is"));

  GtkWidget *const convertwidget1 = gtk_menu_new ();
  gtk_widget_set_name (convertwidget1, "convertwidget1");

  GtkWidget *const convertwidget2
      = gtk_menu_item_new_with_mnemonic (_ ("Exact Time"));
  gtk_widget_set_name (convertwidget2, "convertwidget2");
  gtk_widget_show (convertwidget2);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget1), convertwidget2);

  GtkWidget *const convertwidget3
      = gtk_menu_item_new_with_mnemonic (_ ("5 Min"));
  gtk_widget_set_name (convertwidget3, "convertwidget3");
  gtk_widget_show (convertwidget3);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget1), convertwidget3);

  GtkWidget *const convertwidget4
      = gtk_menu_item_new_with_mnemonic (_ ("10 Min"));
  gtk_widget_set_name (convertwidget4, "convertwidget4");
  gtk_widget_show (convertwidget4);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget1), convertwidget4);

  GtkWidget *const convertwidget5
      = gtk_menu_item_new_with_mnemonic (_ ("15 Min"));
  gtk_widget_set_name (convertwidget5, "convertwidget5");
  gtk_widget_show (convertwidget5);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget1), convertwidget5);

  GtkWidget *const convertwidget6
      = gtk_menu_item_new_with_mnemonic (_ ("20 Min"));
  gtk_widget_set_name (convertwidget6, "convertwidget6");
  gtk_widget_show (convertwidget6);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget1), convertwidget6);

  GtkWidget *const convertwidget7
      = gtk_menu_item_new_with_mnemonic (_ ("30 Min"));
  gtk_widget_set_name (convertwidget7, "convertwidget7");
  gtk_widget_show (convertwidget7);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget1), convertwidget7);

  GtkWidget *const convertwidget8
      = gtk_menu_item_new_with_mnemonic (_ ("1 Hour"));
  gtk_widget_set_name (convertwidget8, "convertwidget8");
  gtk_widget_show (convertwidget8);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget1), convertwidget8);

  GtkWidget *const convertwidget9
      = gtk_menu_item_new_with_mnemonic (_ ("2 Hours"));
  gtk_widget_set_name (convertwidget9, "convertwidget9");
  gtk_widget_show (convertwidget9);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget1), convertwidget9);

  GtkWidget *const convertwidget10
      = gtk_menu_item_new_with_mnemonic (_ ("3 Hours"));
  gtk_widget_set_name (convertwidget10, "convertwidget10");
  gtk_widget_show (convertwidget10);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget1), convertwidget10);

  GtkWidget *const convertwidget11
      = gtk_menu_item_new_with_mnemonic (_ ("Today"));
  gtk_widget_set_name (convertwidget11, "convertwidget11");
  gtk_widget_show (convertwidget11);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget1), convertwidget11);

  gtk_widget_show (convertwidget1);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (fuzz_menu), convertwidget1);
  gtk_option_menu_set_history (GTK_OPTION_MENU (fuzz_menu), 0);
  gtk_widget_show (fuzz_menu);

  gtk_table_attach (GTK_TABLE (table1), fuzz_menu, 1, 2, 2, 3, GTK_FILL, 0, 4,
                    0);

  gtk_widget_show (table1);

  gtk_box_pack_start (GTK_BOX (content_area), table1, FALSE, FALSE, 0);

  /* ----------------------------------------------- */
  /* install option data by hand ... ugh
   * wish glade did this for us .. */
  w = dlg->fuzz_widget;
  menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (w));

  gtk_option_menu_set_history (GTK_OPTION_MENU (w), 0);
  menu_item = gtk_menu_get_active (GTK_MENU (menu));
  g_object_set_data (G_OBJECT (menu_item), "fuzz_factor", GINT_TO_POINTER (0));

  gtk_option_menu_set_history (GTK_OPTION_MENU (w), 1);
  menu_item = gtk_menu_get_active (GTK_MENU (menu));
  g_object_set_data (G_OBJECT (menu_item), "fuzz_factor",
                     GINT_TO_POINTER (300));

  gtk_option_menu_set_history (GTK_OPTION_MENU (w), 2);
  menu_item = gtk_menu_get_active (GTK_MENU (menu));
  g_object_set_data (G_OBJECT (menu_item), "fuzz_factor",
                     GINT_TO_POINTER (600));

  gtk_option_menu_set_history (GTK_OPTION_MENU (w), 3);
  menu_item = gtk_menu_get_active (GTK_MENU (menu));
  g_object_set_data (G_OBJECT (menu_item), "fuzz_factor",
                     GINT_TO_POINTER (900));

  gtk_option_menu_set_history (GTK_OPTION_MENU (w), 4);
  menu_item = gtk_menu_get_active (GTK_MENU (menu));
  g_object_set_data (G_OBJECT (menu_item), "fuzz_factor",
                     GINT_TO_POINTER (1200));

  gtk_option_menu_set_history (GTK_OPTION_MENU (w), 5);
  menu_item = gtk_menu_get_active (GTK_MENU (menu));
  g_object_set_data (G_OBJECT (menu_item), "fuzz_factor",
                     GINT_TO_POINTER (1800));

  gtk_option_menu_set_history (GTK_OPTION_MENU (w), 6);
  menu_item = gtk_menu_get_active (GTK_MENU (menu));
  g_object_set_data (G_OBJECT (menu_item), "fuzz_factor",
                     GINT_TO_POINTER (3600));

  gtk_option_menu_set_history (GTK_OPTION_MENU (w), 7);
  menu_item = gtk_menu_get_active (GTK_MENU (menu));
  g_object_set_data (G_OBJECT (menu_item), "fuzz_factor",
                     GINT_TO_POINTER (7200));

  gtk_option_menu_set_history (GTK_OPTION_MENU (w), 8);
  menu_item = gtk_menu_get_active (GTK_MENU (menu));
  g_object_set_data (G_OBJECT (menu_item), "fuzz_factor",
                     GINT_TO_POINTER (3 * 3600));

  gtk_option_menu_set_history (GTK_OPTION_MENU (w), 9);
  menu_item = gtk_menu_get_active (GTK_MENU (menu));
  g_object_set_data (G_OBJECT (menu_item), "fuzz_factor",
                     GINT_TO_POINTER (12 * 3600));

  /* gnome_dialog_close_hides(GNOME_DIALOG(dlg->interval_edit), TRUE); */
  gtk_widget_hide_on_delete (dlg->interval_edit);
  return dlg;
}

/* ============================================================== */

void
edit_interval_dialog_show (EditIntervalDialog *dlg)
{
  if (!dlg)
    return;
  gtk_widget_show (GTK_WIDGET (dlg->interval_edit));
}

void
edit_interval_dialog_destroy (EditIntervalDialog *dlg)
{
  if (!dlg)
    return;
  gtk_widget_destroy (GTK_WIDGET (dlg->interval_edit));
  g_free (dlg);
}

void
edit_interval_set_close_callback (EditIntervalDialog *dlg, GCallback f,
                                  gpointer data)
{
  g_signal_connect (dlg->interval_edit, "close", f, data);
}

/* ===================== END OF FILE ==============================  */
