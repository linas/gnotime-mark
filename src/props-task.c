/*   Task Properties for GTimeTracker - a time tracker
 *   Copyright (C) 2001,2002,2003,2004 Linas Vepstas <linas@linas.org>
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

#include <gnome.h>
#include <string.h>

#include "dialog.h"
#include "proj.h"
#include "props-task.h"
#include "util.h"

typedef struct PropTaskDlg_s
{
  GtkDialog *dlg;
  GtkEntry *memo;
  GtkTextView *notes;
  GtkOptionMenu *billstatus;
  GtkOptionMenu *billable;
  GtkOptionMenu *billrate;
  GtkEntry *unit;

  GttTask *task;

  /* The goal of 'ignore events' is to prevent an inifinite
   * loop of cascading events as we modify the project and the GUI.
   */
  gboolean ignore_events;

  /* The goal of the freezes is to prevent more than one update
   * of windows per second.  The problem is that without this,
   * there would be one event per keystroke, which could cause
   * a redraw of e.g. the journal window.  In such a case, even
   * moderate typists on a slow CPU could saturate the CPU entirely.
   */
  gboolean task_freeze;

} PropTaskDlg;

static GtkWidget *btagged (GtkWidget *wdgt, PropTaskDlg *dlg);
static gint get_menu (GtkOptionMenu *wdgt, const gchar *name);
static void mentry (GtkOptionMenu *wdgt, const gchar *name, guint order,
                    gint val);
static GtkOptionMenu *mugged (GtkWidget *wdgt, GtkWidget *menu,
                              PropTaskDlg *dlg);
static GtkWidget *ntagged (GtkWidget *wdgt, PropTaskDlg *dlg);
static GtkWidget *texted (GtkWidget *wdgt, PropTaskDlg *dlg);

/* ============================================================== */

#define TSK_SETUP(dlg)                                                        \
  if (NULL == dlg->task)                                                      \
    return;                                                                   \
  if (dlg->ignore_events)                                                     \
    return;                                                                   \
                                                                              \
  dlg->ignore_events = TRUE;                                                  \
  dlg->task_freeze = TRUE;                                                    \
  gtt_task_freeze (dlg->task);

/* ============================================================== */
/* Copy from widget to gtt objects */

static void
save_task_notes (GtkWidget *w, PropTaskDlg *dlg)
{
  const gchar *cstr;
  gchar *str;

  TSK_SETUP (dlg);

  cstr = gtk_entry_get_text (dlg->memo);
  if (cstr && cstr[0])
    {
      gtt_task_set_memo (dlg->task, cstr);
    }
  else
    {
      gtt_task_set_memo (dlg->task, "");
      gtk_entry_set_text (dlg->memo, "");
    }

  str = xxxgtk_textview_get_text (dlg->notes);
  gtt_task_set_notes (dlg->task, str);
  g_free (str);

  dlg->ignore_events = FALSE;
}

static void
save_task_billinfo (GtkWidget *w, PropTaskDlg *dlg)
{
  GttBillStatus status;
  GttBillable able;
  GttBillRate rate;
  int ivl;

  TSK_SETUP (dlg);

  ivl = (int)(60.0 * atof (gtk_entry_get_text (dlg->unit)));
  gtt_task_set_bill_unit (dlg->task, ivl);

  status = (GttBillStatus)get_menu (dlg->billstatus, "billstatus");
  gtt_task_set_billstatus (dlg->task, status);

  able = (GttBillable)get_menu (dlg->billable, "billable");
  gtt_task_set_billable (dlg->task, able);

  rate = (GttBillRate)get_menu (dlg->billrate, "billrate");
  gtt_task_set_billrate (dlg->task, rate);

  dlg->ignore_events = FALSE;
}

/* ============================================================== */
/* Copy values from gnotime object to widget */

static void
do_set_task (GttTask *tsk, PropTaskDlg *dlg)
{
  GttBillStatus status;
  GttBillable able;
  GttBillRate rate;
  char buff[132];

  if (!tsk)
    {
      dlg->task = NULL;
      gtk_entry_set_text (dlg->memo, "");
      xxxgtk_textview_set_text (dlg->notes, "");
      gtk_entry_set_text (dlg->unit, "0.0");
      return;
    }

  /* Set the task, even if its same as the old task.  Do this because
   * the widget may contain rejected edit values.  */
  dlg->task = tsk;
  TSK_SETUP (dlg);

  gtk_entry_set_text (dlg->memo, gtt_task_get_memo (tsk));
  xxxgtk_textview_set_text (dlg->notes, gtt_task_get_notes (tsk));

  g_snprintf (buff, 132, "%g", ((double)gtt_task_get_bill_unit (tsk)) / 60.0);
  gtk_entry_set_text (dlg->unit, buff);

  status = gtt_task_get_billstatus (tsk);
  if (GTT_HOLD == status)
    gtk_option_menu_set_history (dlg->billstatus, 0);
  else if (GTT_BILL == status)
    gtk_option_menu_set_history (dlg->billstatus, 1);
  else if (GTT_PAID == status)
    gtk_option_menu_set_history (dlg->billstatus, 2);

  able = gtt_task_get_billable (tsk);
  if (GTT_BILLABLE == able)
    gtk_option_menu_set_history (dlg->billable, 0);
  else if (GTT_NOT_BILLABLE == able)
    gtk_option_menu_set_history (dlg->billable, 1);
  else if (GTT_NO_CHARGE == able)
    gtk_option_menu_set_history (dlg->billable, 2);

  rate = gtt_task_get_billrate (tsk);
  if (GTT_REGULAR == rate)
    gtk_option_menu_set_history (dlg->billrate, 0);
  else if (GTT_OVERTIME == rate)
    gtk_option_menu_set_history (dlg->billrate, 1);
  else if (GTT_OVEROVER == rate)
    gtk_option_menu_set_history (dlg->billrate, 2);
  else if (GTT_FLAT_FEE == rate)
    gtk_option_menu_set_history (dlg->billrate, 3);

  dlg->ignore_events = FALSE;
}

/* ============================================================== */

static void
redraw (GttProject *prj, gpointer data)
{
  PropTaskDlg *dlg = data;
  do_set_task (dlg->task, dlg);
}

static void
close_cb (GtkWidget *w, PropTaskDlg *dlg)
{
  GttProject *prj;
  prj = gtt_task_get_parent (dlg->task);
  gtt_project_remove_notifier (prj, redraw, dlg);

  dlg->ignore_events = FALSE;
  gtt_task_thaw (dlg->task);
  dlg->task_freeze = FALSE;
  save_task_notes (w, dlg);
  save_task_billinfo (w, dlg);
  dlg->task = NULL;
  gtk_widget_hide (GTK_WIDGET (dlg->dlg));
}

static PropTaskDlg *global_dlog = NULL;

static void
destroy_cb (GtkWidget *w, PropTaskDlg *dlg)
{
  close_cb (w, dlg);
  global_dlog = NULL;
  g_free (dlg);
}

/* ============================================================== */

static PropTaskDlg *
prop_task_dialog_new (void)
{
  PropTaskDlg *dlg = NULL;

  dlg = g_new0 (PropTaskDlg, 1);

  GtkWidget *task_properties = gtk_dialog_new ();
  dlg->dlg = GTK_DIALOG (task_properties);
  gtk_dialog_set_has_separator (GTK_DIALOG (task_properties), TRUE);
  gtk_widget_set_name (task_properties, "Task Properties");
  gtk_window_set_destroy_with_parent (GTK_WINDOW (task_properties), FALSE);
  gtk_window_set_modal (GTK_WINDOW (task_properties), FALSE);
  gtk_window_set_position (GTK_WINDOW (task_properties), GTK_WIN_POS_NONE);
  gtk_window_set_resizable (GTK_WINDOW (task_properties), TRUE);
  gtk_window_set_title (GTK_WINDOW (task_properties), _ ("Diary Notes"));

  g_signal_connect (G_OBJECT (task_properties), "close", G_CALLBACK (close_cb),
                    dlg);
  g_signal_connect (G_OBJECT (task_properties), "destroy",
                    G_CALLBACK (destroy_cb), dlg);

  GtkWidget *const action_area
      = gtk_dialog_get_action_area (GTK_DIALOG (task_properties));
  gtk_button_box_set_layout (GTK_BUTTON_BOX (action_area), GTK_BUTTONBOX_END);

  GtkWidget *const help_button = gtk_button_new_from_stock (GTK_STOCK_HELP);
  gtk_button_set_relief (GTK_BUTTON (help_button), GTK_RELIEF_NORMAL);
  gtk_widget_set_can_default (help_button, TRUE);
  gtk_widget_set_can_focus (help_button, TRUE);
  gtk_widget_set_name (help_button, "help button");
  g_signal_connect (G_OBJECT (help_button), "clicked",
                    G_CALLBACK (gtt_help_popup), "properties");
  gtk_widget_show (help_button);

  gtk_box_pack_start_defaults (GTK_BOX (action_area), help_button);

  GtkWidget *const okbutton1 = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_button_set_relief (GTK_BUTTON (okbutton1), GTK_RELIEF_NORMAL);
  gtk_widget_set_can_default (okbutton1, TRUE);
  gtk_widget_set_can_focus (okbutton1, TRUE);
  gtk_widget_set_name (okbutton1, "okbutton1");
  g_signal_connect (G_OBJECT (okbutton1), "clicked", G_CALLBACK (close_cb),
                    dlg);
  gtk_widget_show (okbutton1);

  gtk_box_pack_start_defaults (GTK_BOX (action_area), okbutton1);

  gtk_widget_show (action_area);

  GtkWidget *const content_area
      = gtk_dialog_get_content_area (GTK_DIALOG (task_properties));

  GtkWidget *const notebook = gtk_notebook_new ();
  gtk_notebook_popup_disable (GTK_NOTEBOOK (notebook));
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), TRUE);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), TRUE);
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_widget_set_can_focus (notebook, TRUE);
  gtk_widget_set_name (notebook, "notebook");

  GtkWidget *label5 = gtk_label_new (_ ("Diary Notes"));
  gtk_label_set_justify (GTK_LABEL (label5), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label5), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label5), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label5), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label5), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label5), 0.5, 0.5);
  gtk_misc_set_padding (GTK_MISC (label5), 0, 0);
  gtk_widget_set_name (label5, "label5");
  gtk_widget_show (label5);

  GtkWidget *const task_table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (task_table), 7);
  gtk_table_set_col_spacings (GTK_TABLE (task_table), 7);
  gtk_table_set_row_spacings (GTK_TABLE (task_table), 7);
  gtk_widget_set_name (task_table, "task table");

  GtkWidget *const label1 = gtk_label_new (_ ("Diary Entry:"));
  gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label1), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label1), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label1), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label1), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label1), 0, 0);
  gtk_widget_set_name (label1, "label1");
  gtk_widget_show (label1);

  gtk_table_attach (GTK_TABLE (task_table), label1, 0, 1, 0, 1, GTK_FILL, 0, 0,
                    0);

  GtkWidget *const entry3 = gnome_entry_new ("task_memo");
  gnome_entry_set_max_saved (GNOME_ENTRY (entry3), 10);
  gtk_widget_set_name (entry3, "entry3");

  GtkWidget *const memo_box = gnome_entry_gtk_entry (GNOME_ENTRY (entry3));
  dlg->memo = GTK_ENTRY (ntagged (memo_box, dlg));
  gtk_entry_set_activates_default (GTK_ENTRY (memo_box), FALSE);
  gtk_entry_set_editable (GTK_ENTRY (memo_box), TRUE);
  gtk_entry_set_has_frame (GTK_ENTRY (memo_box), TRUE);
  gtk_entry_set_invisible_char (GTK_ENTRY (memo_box), '*');
  gtk_entry_set_max_length (GTK_ENTRY (memo_box), 0);
  gtk_entry_set_visibility (GTK_ENTRY (memo_box), TRUE);
  gtk_widget_set_can_focus (memo_box, TRUE);
  gtk_widget_set_name (memo_box, "memo box");
  gtk_widget_set_tooltip_text (
      memo_box, _ ("A short description to attach to this block of time."));
  gtk_widget_show (memo_box);

  gtk_widget_show (entry3);

  gtk_table_attach (GTK_TABLE (task_table), entry3, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const label2 = gtk_label_new (_ ("Notes:"));
  gtk_label_set_justify (GTK_LABEL (label2), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label2), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label2), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label2), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label2), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0);
  gtk_misc_set_padding (GTK_MISC (label2), 0, 0);
  gtk_widget_set_name (label2, "label2");
  gtk_widget_show (label2);

  gtk_table_attach (GTK_TABLE (task_table), label2, 0, 1, 1, 2, GTK_FILL, 0, 0,
                    0);

  GtkWidget *const scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_placement (GTK_SCROLLED_WINDOW (scrolledwindow1),
                                     GTK_CORNER_TOP_LEFT);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow1),
                                       GTK_SHADOW_IN);
  gtk_widget_set_name (scrolledwindow1, "scrolledwindow1");

  GtkWidget *const notes_box = gtk_text_view_new ();
  dlg->notes = GTK_TEXT_VIEW (texted (notes_box, dlg));
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (notes_box), TRUE);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (notes_box), TRUE);
  gtk_text_view_set_indent (GTK_TEXT_VIEW (notes_box), 0);
  gtk_text_view_set_justification (GTK_TEXT_VIEW (notes_box),
                                   GTK_JUSTIFY_LEFT);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (notes_box), 0);
  gtk_text_view_set_pixels_above_lines (GTK_TEXT_VIEW (notes_box), 0);
  gtk_text_view_set_pixels_below_lines (GTK_TEXT_VIEW (notes_box), 0);
  gtk_text_view_set_pixels_inside_wrap (GTK_TEXT_VIEW (notes_box), 0);
  gtk_text_view_set_right_margin (GTK_TEXT_VIEW (notes_box), 0);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (notes_box), GTK_WRAP_WORD);
  gtk_widget_set_can_focus (notes_box, TRUE);
  gtk_widget_set_name (notes_box, "notes box");
  gtk_widget_set_tooltip_text (notes_box,
                               _ ("Type detailed diary entry notes here."));
  gtk_widget_show (notes_box);

  gtk_container_add (GTK_CONTAINER (scrolledwindow1), notes_box);
  gtk_widget_show (scrolledwindow1);

  gtk_table_attach_defaults (GTK_TABLE (task_table), scrolledwindow1, 1, 2, 1,
                             2);

  gtk_widget_show (task_table);

  gtk_notebook_insert_page (GTK_NOTEBOOK (notebook), task_table, label5, 0);

  GtkWidget *const table1 = gtk_table_new (4, 3, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table1), 7);
  gtk_table_set_col_spacings (GTK_TABLE (table1), 7);
  gtk_table_set_row_spacings (GTK_TABLE (table1), 7);
  gtk_widget_set_name (table1, "table1");

  GtkWidget *const label16 = gtk_label_new (_ ("Billing Status:"));
  gtk_label_set_justify (GTK_LABEL (label16), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label16), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label16), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label16), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label16), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label16), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label16), 0, 0);
  gtk_widget_set_name (label16, "label16");
  gtk_widget_show (label16);

  gtk_table_attach (GTK_TABLE (table1), label16, 0, 1, 0, 1, GTK_FILL, 0, 0,
                    0);

  GtkWidget *const billstatus_menu = gtk_option_menu_new ();
  gtk_widget_set_can_focus (billstatus_menu, TRUE);
  gtk_widget_set_name (billstatus_menu, "billstatus menu");
  gtk_widget_set_tooltip_text (
      billstatus_menu,
      _ ("Is this task ready to be billed to the customer? \"Hold\" means "
         "maybe, but not yet, needs review.   \"Bill\" means print this on "
         "the next invoice.   \"Paid\" means that it should no longer be "
         "included on invoices."));

  GtkWidget *const convertwidget12 = gtk_menu_new ();
  gtk_widget_set_name (convertwidget12, "convertwidget12");

  GtkWidget *const convertwidget13
      = gtk_menu_item_new_with_mnemonic (_ ("Hold"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget13), TRUE);
  gtk_widget_set_name (convertwidget13, "convertwidget13");
  gtk_widget_show (convertwidget13);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget12), convertwidget13);

  GtkWidget *const convertwidget14
      = gtk_menu_item_new_with_mnemonic (_ ("Bill"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget14), TRUE);
  gtk_widget_set_name (convertwidget14, "convertwidget14");
  gtk_widget_show (convertwidget14);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget12), convertwidget14);

  GtkWidget *const convertwidget15
      = gtk_menu_item_new_with_mnemonic (_ ("Paid"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget15), TRUE);
  gtk_widget_set_name (convertwidget15, "convertwidget15");
  gtk_widget_show (convertwidget15);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget12), convertwidget15);

  gtk_widget_show (convertwidget12);

  dlg->billstatus = mugged (billstatus_menu, convertwidget12, dlg);
  gtk_option_menu_set_history (GTK_OPTION_MENU (billstatus_menu), 0);
  gtk_widget_show (billstatus_menu);

  gtk_table_attach (GTK_TABLE (table1), billstatus_menu, 1, 2, 0, 1, GTK_FILL,
                    0, 0, 0);

  GtkWidget *const label15 = gtk_label_new (_ ("Billable:"));
  gtk_label_set_justify (GTK_LABEL (label15), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label15), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label15), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label15), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label15), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label15), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label15), 0, 0);
  gtk_widget_set_name (label15, "label15");
  gtk_widget_show (label15);

  gtk_table_attach (GTK_TABLE (table1), label15, 0, 1, 1, 2, GTK_FILL, 0, 0,
                    0);

  GtkWidget *const billable_menu = gtk_option_menu_new ();
  gtk_widget_set_can_focus (billable_menu, TRUE);
  gtk_widget_set_name (billable_menu, "billable menu");
  gtk_widget_set_tooltip_text (
      billable_menu,
      _ ("How should this task be billed?  \"Billable\" means bill to the "
         "customer in the normal fashion.   \"Not Billable\" means we can't "
         "ask for money for this, don't print on the invoice.   \"No Charge\" "
         "means print on the invoice as 'free/no-charge'."));

  GtkWidget *const convertwidget8 = gtk_menu_new ();
  gtk_widget_set_name (convertwidget8, "convertwidget8");

  GtkWidget *const convertwidget9
      = gtk_menu_item_new_with_mnemonic (_ ("Billable"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget9), TRUE);
  gtk_widget_set_name (convertwidget9, "convertwidget9");
  gtk_widget_show (convertwidget9);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget8), convertwidget9);

  GtkWidget *const convertwidget10
      = gtk_menu_item_new_with_mnemonic (_ ("Not Billable"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget10), TRUE);
  gtk_widget_set_name (convertwidget10, "convertwidget10");
  gtk_widget_show (convertwidget10);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget8), convertwidget10);

  GtkWidget *const convertwidget11
      = gtk_menu_item_new_with_mnemonic (_ ("No Charge"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget11), TRUE);
  gtk_widget_set_name (convertwidget11, "convertwidget11");
  gtk_widget_show (convertwidget11);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget8), convertwidget11);

  gtk_widget_show (convertwidget8);

  dlg->billable = mugged (billable_menu, convertwidget8, dlg);
  gtk_option_menu_set_history (GTK_OPTION_MENU (billable_menu), 0);
  gtk_widget_show (billable_menu);

  gtk_table_attach (GTK_TABLE (table1), billable_menu, 1, 2, 1, 2, GTK_FILL, 0,
                    0, 0);

  GtkWidget *const label14 = gtk_label_new (_ ("Billing Rate:"));
  gtk_label_set_justify (GTK_LABEL (label14), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label14), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label14), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label14), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label14), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label14), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label14), 0, 0);
  gtk_widget_set_name (label14, "label14");
  gtk_widget_show (label14);

  gtk_table_attach (GTK_TABLE (table1), label14, 0, 1, 2, 3, GTK_FILL, 0, 0,
                    0);

  GtkWidget *const billrate_menu = gtk_option_menu_new ();
  gtk_widget_set_can_focus (billrate_menu, TRUE);
  gtk_widget_set_name (billrate_menu, "billrate menu");
  gtk_widget_set_tooltip_text (billrate_menu,
                               _ ("Fee rate to be charged for this task."));

  GtkWidget *const convertwidget3 = gtk_menu_new ();
  gtk_widget_set_name (convertwidget3, "convertwidget3");

  GtkWidget *const convertwidget4
      = gtk_menu_item_new_with_mnemonic (_ ("Regular"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget4), TRUE);
  gtk_widget_set_name (convertwidget4, "convertwidget4");
  gtk_widget_show (convertwidget4);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget3), convertwidget4);

  GtkWidget *const convertwidget5
      = gtk_menu_item_new_with_mnemonic (_ ("Overtime"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget5), TRUE);
  gtk_widget_set_name (convertwidget5, "convertwidget5");
  gtk_widget_show (convertwidget5);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget3), convertwidget5);

  GtkWidget *const convertwidget6
      = gtk_menu_item_new_with_mnemonic (_ ("OverOver"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget6), TRUE);
  gtk_widget_set_name (convertwidget6, "convertwidget6");
  gtk_widget_show (convertwidget6);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget3), convertwidget6);

  GtkWidget *const convertwidget7
      = gtk_menu_item_new_with_mnemonic (_ ("Flat Fee"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget7), TRUE);
  gtk_widget_set_name (convertwidget7, "convertwidget7");
  gtk_widget_show (convertwidget7);

  gtk_menu_shell_append (GTK_MENU_SHELL (convertwidget3), convertwidget7);

  gtk_widget_show (convertwidget3);

  dlg->billrate = mugged (billrate_menu, convertwidget3, dlg);
  gtk_option_menu_set_history (GTK_OPTION_MENU (billrate_menu), 0);
  gtk_widget_show (billrate_menu);

  gtk_table_attach (GTK_TABLE (table1), billrate_menu, 1, 2, 2, 3, GTK_FILL, 0,
                    0, 0);

  GtkWidget *const label12 = gtk_label_new (_ ("Billing Block:"));
  gtk_label_set_justify (GTK_LABEL (label12), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label12), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label12), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label12), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label12), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label12), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label12), 0, 0);
  gtk_widget_set_name (label12, "label12");
  gtk_widget_show (label12);

  gtk_table_attach (GTK_TABLE (table1), label12, 0, 1, 3, 4, GTK_FILL, 0, 0,
                    0);

  GtkWidget *const entry4 = gnome_entry_new ("bill_unit");
  gnome_entry_set_max_saved (GNOME_ENTRY (entry4), 10);
  gtk_widget_set_name (entry4, "entry4");

  GtkWidget *const unit_box = gnome_entry_gtk_entry (GNOME_ENTRY (entry4));
  dlg->unit = GTK_ENTRY (btagged (unit_box, dlg));
  gtk_entry_set_activates_default (GTK_ENTRY (unit_box), FALSE);
  gtk_entry_set_editable (GTK_ENTRY (unit_box), TRUE);
  gtk_entry_set_has_frame (GTK_ENTRY (unit_box), TRUE);
  gtk_entry_set_invisible_char (GTK_ENTRY (unit_box), '*');
  gtk_entry_set_max_length (GTK_ENTRY (unit_box), 0);
  gtk_entry_set_visibility (GTK_ENTRY (unit_box), TRUE);
  gtk_widget_set_can_focus (unit_box, TRUE);
  gtk_widget_set_name (unit_box, "unit box");
  gtk_widget_set_tooltip_text (unit_box,
                               _ ("The billed unit of time will be rounded to "
                                  "an integer multiple of this time."));
  gtk_widget_show (unit_box);

  gtk_widget_show (entry4);

  gtk_table_attach (GTK_TABLE (table1), entry4, 1, 2, 3, 4,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const label13 = gtk_label_new (_ ("minutes"));
  gtk_label_set_justify (GTK_LABEL (label13), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label13), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label13), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label13), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label13), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label13), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label13), 0, 0);
  gtk_widget_set_name (label13, "label13");
  gtk_widget_show (label13);

  gtk_table_attach (GTK_TABLE (table1), label13, 2, 3, 3, 4, GTK_FILL, 0, 0,
                    0);

  gtk_widget_show (table1);

  GtkWidget *const label9 = gtk_label_new (_ ("Billing"));
  gtk_label_set_justify (GTK_LABEL (label9), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label9), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label9), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label9), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label9), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label9), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label9), 0, 0);
  gtk_widget_show (label9);

  gtk_notebook_insert_page (GTK_NOTEBOOK (notebook), table1, label9, 1);

  gtk_box_pack_start (GTK_BOX (content_area), notebook, TRUE, TRUE, 0);

  gtk_widget_show (notebook);

  gtk_widget_show (content_area);

  gtk_widget_show (task_properties);

  /* ------------------------------------------------------ */
  /* associate values with the three option menus */

  mentry (dlg->billstatus, "billstatus", 0, GTT_HOLD);
  mentry (dlg->billstatus, "billstatus", 1, GTT_BILL);
  mentry (dlg->billstatus, "billstatus", 2, GTT_PAID);

  mentry (dlg->billable, "billable", 0, GTT_BILLABLE);
  mentry (dlg->billable, "billable", 1, GTT_NOT_BILLABLE);
  mentry (dlg->billable, "billable", 2, GTT_NO_CHARGE);

  mentry (dlg->billrate, "billrate", 0, GTT_REGULAR);
  mentry (dlg->billrate, "billrate", 1, GTT_OVERTIME);
  mentry (dlg->billrate, "billrate", 2, GTT_OVEROVER);
  mentry (dlg->billrate, "billrate", 3, GTT_FLAT_FEE);

  dlg->ignore_events = FALSE;
  dlg->task_freeze = FALSE;
  gtk_widget_hide_on_delete (GTK_WIDGET (dlg->dlg));

  return dlg;
}

/* ============================================================== */

void
gtt_diary_timer_callback (gpointer nuts)
{
  /* If there was a more elegant timer add func,
   * we wouldn't need this global */
  PropTaskDlg *dlg = global_dlog;
  if (!dlg)
    return;
  dlg->ignore_events = TRUE;
  if (dlg->task_freeze)
    {
      gtt_task_thaw (dlg->task);
      dlg->task_freeze = FALSE;
    }
  dlg->ignore_events = FALSE;
}

void
prop_task_dialog_show (GttTask *task)
{
  GttProject *prj;
  if (!task)
    return;
  if (!global_dlog)
    global_dlog = prop_task_dialog_new ();

  do_set_task (task, global_dlog);

  prj = gtt_task_get_parent (task);
  gtt_project_add_notifier (prj, redraw, global_dlog);

  gtk_widget_show (GTK_WIDGET (global_dlog->dlg));
}

static GtkWidget *
btagged (GtkWidget *const wdgt, PropTaskDlg *const dlg)
{
  g_signal_connect (G_OBJECT (wdgt), "changed",
                    G_CALLBACK (save_task_billinfo), dlg);
  return wdgt;
}

static gint
get_menu (GtkOptionMenu *wdgt, const gchar *name)
{
  GtkWidget *const menu = gtk_option_menu_get_menu (wdgt);
  GtkWidget *const menu_item = gtk_menu_get_active (GTK_MENU (menu));
  return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menu_item), name));
}

static void
mentry (GtkOptionMenu *const wdgt, const gchar *const name, const guint order,
        const gint val)
{
  GtkMenu *const menu = GTK_MENU (gtk_option_menu_get_menu (wdgt));
  gtk_option_menu_set_history (wdgt, order);
  GtkWidget *const menu_item = gtk_menu_get_active (menu);
  g_object_set_data (G_OBJECT (menu_item), name, GINT_TO_POINTER (val));
}

static GtkOptionMenu *
mugged (GtkWidget *const wdgt, GtkWidget *const menu, PropTaskDlg *const dlg)
{
  gtk_option_menu_set_menu (GTK_OPTION_MENU (wdgt), menu);
  GtkWidget *const mw = gtk_option_menu_get_menu (GTK_OPTION_MENU (wdgt));
  g_signal_connect (G_OBJECT (mw), "selection_done",
                    G_CALLBACK (save_task_billinfo), dlg);
  return GTK_OPTION_MENU (wdgt);
}

static GtkWidget *
ntagged (GtkWidget *const wdgt, PropTaskDlg *const dlg)
{
  g_signal_connect (G_OBJECT (wdgt), "changed", G_CALLBACK (save_task_notes),
                    dlg);
  return wdgt;
}

static GtkWidget *
texted (GtkWidget *const wdgt, PropTaskDlg *const dlg)
{
  GtkTextBuffer *const tvb = gtk_text_view_get_buffer (GTK_TEXT_VIEW (wdgt));

  g_signal_connect (G_OBJECT (tvb), "changed", G_CALLBACK (save_task_notes),
                    dlg);

  return wdgt;
}

/* ===================== END OF FILE =========================== */
