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

#include <glade/glade.h>
#include <gnome.h>
#include <string.h>

#include "dialog.h"
#include "proj.h"
#include "props-task.h"
#include "util.h"

typedef struct PropTaskDlg_s
{
  GladeXML *gtxml;
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
static GtkOptionMenu *mugged (GtkWidget *wdgt, PropTaskDlg *dlg);
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
  GladeXML *gtxml;

  dlg = g_new0 (PropTaskDlg, 1);

  gtxml = gtt_glade_xml_new ("glade/task_properties.glade", "Task Properties");
  dlg->gtxml = gtxml;

  dlg->dlg = GTK_DIALOG (glade_xml_get_widget (gtxml, "Task Properties"));

  glade_xml_signal_connect_data (gtxml, "on_help_button_clicked",
                                 GTK_SIGNAL_FUNC (gtt_help_popup),
                                 "properties");

  glade_xml_signal_connect_data (gtxml, "on_ok_button_clicked",
                                 GTK_SIGNAL_FUNC (close_cb), dlg);

  g_signal_connect (G_OBJECT (dlg->dlg), "close", G_CALLBACK (close_cb), dlg);
  g_signal_connect (G_OBJECT (dlg->dlg), "destroy", G_CALLBACK (destroy_cb),
                    dlg);

  /* ------------------------------------------------------ */
  /* grab the various entry boxes and hook them up */

  dlg->memo
      = GTK_ENTRY (ntagged (glade_xml_get_widget (gtxml, "memo box"), dlg));
  dlg->notes = GTK_TEXT_VIEW (
      texted (glade_xml_get_widget (gtxml, "notes box"), dlg));

  dlg->billstatus
      = mugged (glade_xml_get_widget (gtxml, "billstatus menu"), dlg);
  dlg->billable = mugged (glade_xml_get_widget (gtxml, "billable menu"), dlg);
  dlg->billrate = mugged (glade_xml_get_widget (gtxml, "billrate menu"), dlg);
  dlg->unit
      = GTK_ENTRY (btagged (glade_xml_get_widget (gtxml, "unit box"), dlg));

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
mugged (GtkWidget *const wdgt, PropTaskDlg *const dlg)
{
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
