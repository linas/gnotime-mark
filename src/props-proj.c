/*   Project Properties for GTimeTracker - a time tracker
 *   Copyright (C) 1997,98 Eckehard Berns
 *   Copyright (C) 2001,2002,2003 Linas Vepstas <linas@linas.org>
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
#include "props-proj.h"
#include "util.h"

typedef struct _PropDlg
{
  GladeXML *gtxml;
  GnomePropertyBox *dlg;
  GtkEntry *title;
  GtkEntry *desc;
  GtkTextView *notes;

  GtkEntry *regular;
  GtkEntry *overtime;
  GtkEntry *overover;
  GtkEntry *flatfee;

  GtkEntry *minimum;
  GtkEntry *interval;
  GtkEntry *gap;

  GtkOptionMenu *urgency;
  GtkOptionMenu *importance;
  GtkOptionMenu *status;

  GnomeDateEdit *start;
  GnomeDateEdit *end;
  GnomeDateEdit *due;

  GtkEntry *sizing;
  GtkEntry *percent;

  GttProject *proj;
} PropDlg;

static GtkWidget *connect_changed (GtkWidget *wdgt, PropDlg *dlg);
static GnomeDateEdit *dated (GtkWidget *wdgt, PropDlg *dlg);
static gpointer get_menu (GtkOptionMenu *wdgt, const gchar *key);
static void mentry (GtkOptionMenu *wdgt, const gchar *name, guint order,
                    int val);
static GtkOptionMenu *mugged (GtkWidget *wdgt, PropDlg *dlg);
static GtkTextView *texted (GtkWidget *wdgt, PropDlg *dlg);

/* ============================================================== */

static void
prop_set (GnomePropertyBox *pb, gint page, PropDlg *dlg)
{
  long ivl;
  const gchar *cstr;
  gchar *str;
  double rate;
  time_t tval;

  if (!dlg->proj)
    return;

  if (0 == page)
    {
      gtt_project_freeze (dlg->proj);
      cstr = gtk_entry_get_text (dlg->title);
      if (cstr && cstr[0])
        {
          gtt_project_set_title (dlg->proj, cstr);
        }
      else
        {
          gtt_project_set_title (dlg->proj, _ ("empty"));
          gtk_entry_set_text (dlg->title, _ ("empty"));
        }

      gtt_project_set_desc (dlg->proj, gtk_entry_get_text (dlg->desc));
      str = xxxgtk_textview_get_text (dlg->notes);
      gtt_project_set_notes (dlg->proj, str);
      g_free (str);
      gtt_project_thaw (dlg->proj);
    }

  if (1 == page)
    {
      gtt_project_freeze (dlg->proj);
      rate = atof (gtk_entry_get_text (dlg->regular));
      gtt_project_set_billrate (dlg->proj, rate);
      rate = atof (gtk_entry_get_text (dlg->overtime));
      gtt_project_set_overtime_rate (dlg->proj, rate);
      rate = atof (gtk_entry_get_text (dlg->overover));
      gtt_project_set_overover_rate (dlg->proj, rate);
      rate = atof (gtk_entry_get_text (dlg->flatfee));
      gtt_project_set_flat_fee (dlg->proj, rate);
      gtt_project_thaw (dlg->proj);
    }

  if (2 == page)
    {
      gtt_project_freeze (dlg->proj);
      ivl = atoi (gtk_entry_get_text (dlg->minimum));
      gtt_project_set_min_interval (dlg->proj, ivl);
      ivl = atoi (gtk_entry_get_text (dlg->interval));
      gtt_project_set_auto_merge_interval (dlg->proj, ivl);
      ivl = atoi (gtk_entry_get_text (dlg->gap));
      gtt_project_set_auto_merge_gap (dlg->proj, ivl);
      gtt_project_thaw (dlg->proj);
    }
  if (3 == page)
    {
      gtt_project_freeze (dlg->proj);

      ivl = (long)get_menu (dlg->urgency, "urgency");
      gtt_project_set_urgency (dlg->proj, (GttRank)ivl);
      ivl = (long)get_menu (dlg->importance, "importance");
      gtt_project_set_importance (dlg->proj, (GttRank)ivl);

      ivl = (long)get_menu (dlg->status, "status");
      gtt_project_set_status (dlg->proj, (GttProjectStatus)ivl);

      tval = gnome_date_edit_get_time (dlg->start);
      gtt_project_set_estimated_start (dlg->proj, tval);
      tval = gnome_date_edit_get_time (dlg->end);
      gtt_project_set_estimated_end (dlg->proj, tval);
      tval = gnome_date_edit_get_time (dlg->due);
      gtt_project_set_due_date (dlg->proj, tval);

      rate = atof (gtk_entry_get_text (dlg->sizing));
      ivl = rate * 3600.0;
      gtt_project_set_sizing (dlg->proj, ivl);

      ivl = atoi (gtk_entry_get_text (dlg->percent));
      gtt_project_set_percent_complete (dlg->proj, ivl);

      gtt_project_thaw (dlg->proj);
    }
}

/* ============================================================== */

static void
do_set_project (GttProject *proj, PropDlg *dlg)
{
  GttProjectStatus status;
  GttRank rank;
  time_t tval;
  char buff[132];
  time_t now = time (NULL);

  if (!dlg)
    return;

  if (!proj)
    {
      /* We null these out, because old values may be left
       * over from an earlier project */
      dlg->proj = NULL;
      gtk_entry_set_text (dlg->title, "");
      gtk_entry_set_text (dlg->desc, "");
      xxxgtk_textview_set_text (dlg->notes, "");
      gtk_entry_set_text (dlg->regular, "0.0");
      gtk_entry_set_text (dlg->overtime, "0.0");
      gtk_entry_set_text (dlg->overover, "0.0");
      gtk_entry_set_text (dlg->flatfee, "0.0");
      gtk_entry_set_text (dlg->minimum, "0");
      gtk_entry_set_text (dlg->interval, "0");
      gtk_entry_set_text (dlg->gap, "0");

      gnome_date_edit_set_time (dlg->start, now);
      gnome_date_edit_set_time (dlg->end, now);
      gnome_date_edit_set_time (dlg->due, now + 86400);
      gtk_entry_set_text (dlg->sizing, "0.0");
      gtk_entry_set_text (dlg->percent, "0");
      return;
    }

  /* set all the values. Do this even is new project is same as old
   * project, since widget may be holding rejected changes. */
  dlg->proj = proj;

  gtk_entry_set_text (dlg->title, gtt_project_get_title (proj));
  gtk_entry_set_text (dlg->desc, gtt_project_get_desc (proj));
  xxxgtk_textview_set_text (dlg->notes, gtt_project_get_notes (proj));

  /* hack alert should use local currencies for this */
  g_snprintf (buff, 132, "%.2f", gtt_project_get_billrate (proj));
  gtk_entry_set_text (dlg->regular, buff);
  g_snprintf (buff, 132, "%.2f", gtt_project_get_overtime_rate (proj));
  gtk_entry_set_text (dlg->overtime, buff);
  g_snprintf (buff, 132, "%.2f", gtt_project_get_overover_rate (proj));
  gtk_entry_set_text (dlg->overover, buff);
  g_snprintf (buff, 132, "%.2f", gtt_project_get_flat_fee (proj));
  gtk_entry_set_text (dlg->flatfee, buff);

  g_snprintf (buff, 132, "%d", gtt_project_get_min_interval (proj));
  gtk_entry_set_text (dlg->minimum, buff);
  g_snprintf (buff, 132, "%d", gtt_project_get_auto_merge_interval (proj));
  gtk_entry_set_text (dlg->interval, buff);
  g_snprintf (buff, 132, "%d", gtt_project_get_auto_merge_gap (proj));
  gtk_entry_set_text (dlg->gap, buff);

  rank = gtt_project_get_urgency (proj);
  if (GTT_UNDEFINED == rank)
    gtk_option_menu_set_history (dlg->urgency, 0);
  else if (GTT_LOW == rank)
    gtk_option_menu_set_history (dlg->urgency, 1);
  else if (GTT_MEDIUM == rank)
    gtk_option_menu_set_history (dlg->urgency, 2);
  else if (GTT_HIGH == rank)
    gtk_option_menu_set_history (dlg->urgency, 3);

  rank = gtt_project_get_importance (proj);
  if (GTT_UNDEFINED == rank)
    gtk_option_menu_set_history (dlg->importance, 0);
  else if (GTT_LOW == rank)
    gtk_option_menu_set_history (dlg->importance, 1);
  else if (GTT_MEDIUM == rank)
    gtk_option_menu_set_history (dlg->importance, 2);
  else if (GTT_HIGH == rank)
    gtk_option_menu_set_history (dlg->importance, 3);

  status = gtt_project_get_status (proj);
  if (GTT_NO_STATUS == status)
    gtk_option_menu_set_history (dlg->status, 0);
  else if (GTT_NOT_STARTED == status)
    gtk_option_menu_set_history (dlg->status, 1);
  else if (GTT_IN_PROGRESS == status)
    gtk_option_menu_set_history (dlg->status, 2);
  else if (GTT_ON_HOLD == status)
    gtk_option_menu_set_history (dlg->status, 3);
  else if (GTT_CANCELLED == status)
    gtk_option_menu_set_history (dlg->status, 4);
  else if (GTT_COMPLETED == status)
    gtk_option_menu_set_history (dlg->status, 5);

  tval = gtt_project_get_estimated_start (proj);
  if (-1 == tval)
    tval = now;
  gnome_date_edit_set_time (dlg->start, tval);
  tval = gtt_project_get_estimated_end (proj);
  if (-1 == tval)
    tval = now + 3600;
  gnome_date_edit_set_time (dlg->end, tval);
  tval = gtt_project_get_due_date (proj);
  if (-1 == tval)
    tval = now + 86400;
  gnome_date_edit_set_time (dlg->due, tval);

  g_snprintf (buff, 132, "%.2f",
              ((double)gtt_project_get_sizing (proj)) / 3600.0);
  gtk_entry_set_text (dlg->sizing, buff);
  g_snprintf (buff, 132, "%d", gtt_project_get_percent_complete (proj));
  gtk_entry_set_text (dlg->percent, buff);

  /* set to unmodified as it reflects the current state of the project */
  gnome_property_box_set_modified (GNOME_PROPERTY_BOX (dlg->dlg), FALSE);
}

/* ============================================================== */

static void
wrapper (void *gobj, void *data)
{
  gnome_property_box_changed (GNOME_PROPERTY_BOX (data));
}

/* ================================================================= */

static void
help_cb (GnomePropertyBox *propertybox, gint page_num, gpointer data)
{
  gtt_help_popup (GTK_WIDGET (propertybox), data);
}

static PropDlg *
prop_dialog_new (void)
{
  PropDlg *dlg;
  GladeXML *gtxml;

  dlg = g_new0 (PropDlg, 1);

  gtxml = gtt_glade_xml_new ("glade/project_properties.glade",
                             "Project Properties");
  dlg->gtxml = gtxml;

  dlg->dlg = GNOME_PROPERTY_BOX (
      glade_xml_get_widget (gtxml, "Project Properties"));

  gtk_signal_connect (GTK_OBJECT (dlg->dlg), "help", GTK_SIGNAL_FUNC (help_cb),
                      "projects-editing");

  gtk_signal_connect (GTK_OBJECT (dlg->dlg), "apply",
                      GTK_SIGNAL_FUNC (prop_set), dlg);

  /* ------------------------------------------------------ */
  /* grab the various entry boxes and hook them up */

  dlg->title = GTK_ENTRY (
      connect_changed (glade_xml_get_widget (gtxml, "title box"), dlg));
  dlg->desc = GTK_ENTRY (
      connect_changed (glade_xml_get_widget (gtxml, "desc box"), dlg));
  dlg->notes = texted (glade_xml_get_widget (gtxml, "notes box"), dlg);

  dlg->regular = GTK_ENTRY (
      connect_changed (glade_xml_get_widget (gtxml, "regular box"), dlg));
  dlg->overtime = GTK_ENTRY (
      connect_changed (glade_xml_get_widget (gtxml, "overtime box"), dlg));
  dlg->overover = GTK_ENTRY (
      connect_changed (glade_xml_get_widget (gtxml, "overover box"), dlg));
  dlg->flatfee = GTK_ENTRY (
      connect_changed (glade_xml_get_widget (gtxml, "flatfee box"), dlg));

  GtkWidget *interval_table = glade_xml_get_widget (gtxml, "interval table");

  GtkWidget *label31 = gtk_label_new (_ ("Minimum Interval: "));
  gtk_label_set_justify (GTK_LABEL (label31), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label31), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label31), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label31), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label31), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label31), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label31), 0, 0);
  gtk_widget_set_name (label31, "label31");
  gtk_widget_show (label31);

  gtk_table_attach (GTK_TABLE (interval_table), label31, 0, 1, 0, 1, GTK_FILL,
                    0, 0, 0);

  GtkWidget *entry22 = gnome_entry_new ("min_interval");
  gnome_entry_set_max_saved (GNOME_ENTRY (entry22), 10);
  gtk_widget_set_name (entry22, "entry22");

  GtkWidget *minimum_box = gnome_entry_gtk_entry (GNOME_ENTRY (entry22));
  dlg->minimum = GTK_ENTRY (connect_changed (minimum_box, dlg));
  gtk_entry_set_activates_default (GTK_ENTRY (minimum_box), FALSE);
  gtk_entry_set_editable (GTK_ENTRY (minimum_box), TRUE);
  gtk_entry_set_has_frame (GTK_ENTRY (minimum_box), TRUE);
  gtk_entry_set_invisible_char (GTK_ENTRY (minimum_box), '*');
  gtk_entry_set_max_length (GTK_ENTRY (minimum_box), 0);
  gtk_entry_set_visibility (GTK_ENTRY (minimum_box), TRUE);
  gtk_widget_set_can_focus (minimum_box, TRUE);
  gtk_widget_set_name (minimum_box, "minimum box");
  gtk_widget_set_tooltip_text (
      minimum_box, _ ("Intervals smaller than this will be discarded"));
  gtk_widget_show (minimum_box);

  gtk_widget_show (entry22);

  gtk_table_attach (GTK_TABLE (interval_table), entry22, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *label34 = gtk_label_new (_ ("seconds"));
  gtk_label_set_justify (GTK_LABEL (label34), GTK_JUSTIFY_RIGHT);
  gtk_label_set_line_wrap (GTK_LABEL (label34), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label34), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label34), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label34), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label34), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label34), 0, 0);
  gtk_widget_set_name (label34, "label34");
  gtk_widget_show (label34);

  gtk_table_attach (GTK_TABLE (interval_table), label34, 2, 3, 0, 1, GTK_FILL,
                    0, 0, 0);

  GtkWidget *label32 = gtk_label_new (_ ("Auto-merge Interval:"));
  gtk_label_set_justify (GTK_LABEL (label32), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label32), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label32), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label32), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label32), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label32), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label32), 0, 0);
  gtk_widget_set_name (label32, "label32");
  gtk_widget_show (label32);

  gtk_table_attach (GTK_TABLE (interval_table), label32, 0, 1, 1, 2, GTK_FILL,
                    0, 0, 0);

  GtkWidget *entry23 = gnome_entry_new ("merge_interval");
  gnome_entry_set_max_saved (GNOME_ENTRY (entry23), 10);
  gtk_widget_set_name (entry23, "entry23");

  GtkWidget *interval_box = gnome_entry_gtk_entry (GNOME_ENTRY (entry23));
  dlg->interval = GTK_ENTRY (connect_changed (interval_box, dlg));
  gtk_entry_set_activates_default (GTK_ENTRY (interval_box), FALSE);
  gtk_entry_set_editable (GTK_ENTRY (interval_box), TRUE);
  gtk_entry_set_has_frame (GTK_ENTRY (interval_box), TRUE);
  gtk_entry_set_invisible_char (GTK_ENTRY (interval_box), '*');
  gtk_entry_set_max_length (GTK_ENTRY (interval_box), 0);
  gtk_entry_set_visibility (GTK_ENTRY (interval_box), TRUE);
  gtk_widget_set_can_focus (interval_box, TRUE);
  gtk_widget_set_name (interval_box, "interval box");
  gtk_widget_set_tooltip_text (
      interval_box,
      _ ("Time below which an interval is merged with its neighbors"));
  gtk_widget_show (interval_box);

  gtk_widget_show (entry23);

  gtk_table_attach (GTK_TABLE (interval_table), entry23, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *label35 = gtk_label_new (_ ("seconds"));
  gtk_label_set_justify (GTK_LABEL (label35), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label35), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label35), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label35), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label35), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label35), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label35), 0, 0);
  gtk_widget_set_name (label35, "label35");
  gtk_widget_show (label35);

  gtk_table_attach (GTK_TABLE (interval_table), label35, 2, 3, 1, 2, GTK_FILL,
                    0, 0, 0);

  GtkWidget *label33 = gtk_label_new (_ ("Auto-merge Gap"));
  gtk_label_set_justify (GTK_LABEL (label33), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label33), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label33), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label33), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label33), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label33), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label33), 0, 0);
  gtk_widget_set_name (label33, "label33");
  gtk_widget_show (label33);

  gtk_table_attach (GTK_TABLE (interval_table), label33, 0, 1, 2, 3, GTK_FILL,
                    0, 0, 0);

  GtkWidget *entry24 = gnome_entry_new ("merge_gap");
  gnome_entry_set_max_saved (GNOME_ENTRY (entry24), 10);
  gtk_widget_set_name (entry24, "entry24");

  GtkWidget *gap_box = gnome_entry_gtk_entry (GNOME_ENTRY (entry24));
  dlg->gap = GTK_ENTRY (connect_changed (gap_box, dlg));
  gtk_entry_set_activates_default (GTK_ENTRY (gap_box), FALSE);
  gtk_entry_set_editable (GTK_ENTRY (gap_box), TRUE);
  gtk_entry_set_has_frame (GTK_ENTRY (gap_box), TRUE);
  gtk_entry_set_invisible_char (GTK_ENTRY (gap_box), '*');
  gtk_entry_set_max_length (GTK_ENTRY (gap_box), 0);
  gtk_entry_set_visibility (GTK_ENTRY (gap_box), TRUE);
  gtk_widget_set_can_focus (gap_box, TRUE);
  gtk_widget_set_name (gap_box, "gap box");
  gtk_widget_set_tooltip_text (
      gap_box, _ ("If the gap between intervals is smaller than this, the "
                  "intervals will be merged together."));
  gtk_widget_show (gap_box);

  gtk_widget_show (entry24);

  gtk_table_attach (GTK_TABLE (interval_table), entry24, 1, 2, 2, 3,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *label36 = gtk_label_new (_ ("seconds"));
  gtk_label_set_justify (GTK_LABEL (label36), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label36), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label36), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label36), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label36), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label36), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label36), 0, 0);
  gtk_widget_set_name (label36, "label36");
  gtk_widget_show (label36);

  gtk_table_attach (GTK_TABLE (interval_table), label36, 2, 3, 2, 3, GTK_FILL,
                    0, 0, 0);

  GtkWidget *sizing_table = glade_xml_get_widget (gtxml, "sizing table");

  GtkWidget *label38 = gtk_label_new (_ ("Urgency:"));
  gtk_label_set_justify (GTK_LABEL (label38), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label38), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label38), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label38), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label38), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label38), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label38), 0, 0);
  gtk_widget_set_name (label38, "label38");
  gtk_widget_show (label38);

  gtk_table_attach (GTK_TABLE (sizing_table), label38, 0, 1, 0, 1, GTK_FILL, 0,
                    0, 0);

  GtkWidget *urgency = gtk_option_menu_new ();
  gtk_widget_set_can_focus (urgency, TRUE);
  gtk_widget_set_name (urgency, "urgency menu");
  gtk_widget_set_tooltip_text (
      urgency, _ ("Does this item need immediate attention? Note that some "
                  "urgent tasks might not be important.  For example, Bill "
                  "may want you to answer his email today, but you may have "
                  "better things to do today"));

  GtkWidget *urgency_menu = gtk_menu_new ();
  gtk_widget_set_name (urgency_menu, "convertwidget3");

  GtkWidget *urgency_menu_item1 = gtk_menu_item_new_with_label (_ ("Not Set"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (urgency_menu_item1), TRUE);
  gtk_widget_set_name (urgency_menu_item1, "convertwidget4");
  gtk_widget_show (urgency_menu_item1);

  gtk_menu_shell_append (GTK_MENU_SHELL (urgency_menu), urgency_menu_item1);

  GtkWidget *urgency_menu_item2 = gtk_menu_item_new_with_label (_ ("Low"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (urgency_menu_item2), TRUE);
  gtk_widget_set_name (urgency_menu_item2, "convertwidget5");
  gtk_widget_show (urgency_menu_item2);

  gtk_menu_shell_append (GTK_MENU_SHELL (urgency_menu), urgency_menu_item2);

  GtkWidget *urgency_menu_item3 = gtk_menu_item_new_with_label (_ ("Medium"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (urgency_menu_item3), TRUE);
  gtk_widget_set_name (urgency_menu_item3, "convertwidget6");
  gtk_widget_show (urgency_menu_item3);

  gtk_menu_shell_append (GTK_MENU_SHELL (urgency_menu), urgency_menu_item3);

  GtkWidget *urgency_menu_item4 = gtk_menu_item_new_with_label (_ ("High"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (urgency_menu_item4), TRUE);
  gtk_widget_set_name (urgency_menu_item4, "convertwidget7");
  gtk_widget_show (urgency_menu_item4);

  gtk_menu_shell_append (GTK_MENU_SHELL (urgency_menu), urgency_menu_item4);
  gtk_widget_show (urgency_menu);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (urgency), urgency_menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (urgency), 0);
  dlg->urgency = mugged (urgency, dlg);
  gtk_widget_show (urgency);

  gtk_table_attach (GTK_TABLE (sizing_table), urgency, 1, 2, 0, 1, GTK_FILL, 0,
                    0, 0);

  GtkWidget *label46 = gtk_label_new (_ ("            "));
  gtk_label_set_justify (GTK_LABEL (label46), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label46), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label46), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label46), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label46), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label46), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label46), 0, 0);
  gtk_widget_set_name (label46, "label46");
  gtk_widget_show (label46);

  gtk_table_attach (GTK_TABLE (sizing_table), label46, 2, 3, 0, 1, GTK_FILL, 0,
                    0, 0);

  GtkWidget *label47 = gtk_label_new (_ ("            "));
  gtk_label_set_justify (GTK_LABEL (label47), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label47), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label47), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label47), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label47), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label47), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label47), 0, 0);
  gtk_widget_set_name (label47, "label47");
  gtk_widget_show (label47);

  gtk_table_attach (GTK_TABLE (sizing_table), label47, 3, 4, 0, 1, GTK_FILL, 0,
                    0, 0);

  GtkWidget *label39 = gtk_label_new (_ ("Importance:"));
  gtk_label_set_justify (GTK_LABEL (label39), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label39), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label39), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label39), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label39), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label39), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label39), 0, 0);
  gtk_widget_set_name (label39, "label39");
  gtk_widget_show (label39);

  gtk_table_attach (GTK_TABLE (sizing_table), label39, 0, 1, 1, 2, GTK_FILL, 0,
                    0, 0);

  GtkWidget *importance = gtk_option_menu_new ();
  gtk_widget_set_can_focus (importance, TRUE);
  gtk_widget_set_name (importance, "importance menu");
  gtk_widget_set_tooltip_text (
      importance,
      _ ("How important is it to perform this task?  Not everything important "
         "is urgent.  For example, it is important to file a tax return every "
         "year, but you have a lot of time to get ready to do this."));

  GtkWidget *importance_menu = gtk_menu_new ();
  gtk_widget_set_name (importance_menu, "convertwidget8");

  GtkWidget *importance_menu_item1
      = gtk_menu_item_new_with_label (_ ("Not Set"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (importance_menu_item1),
                                   TRUE);
  gtk_widget_set_name (importance_menu_item1, "convertwidget9");
  gtk_widget_show (importance_menu_item1);

  gtk_menu_shell_append (GTK_MENU_SHELL (importance_menu),
                         importance_menu_item1);

  GtkWidget *importance_menu_item2 = gtk_menu_item_new_with_label (_ ("Low"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (importance_menu_item2),
                                   TRUE);
  gtk_widget_set_name (importance_menu_item2, "convertwidget10");
  gtk_widget_show (importance_menu_item2);

  gtk_menu_shell_append (GTK_MENU_SHELL (importance_menu),
                         importance_menu_item2);

  GtkWidget *importance_menu_item3
      = gtk_menu_item_new_with_label (_ ("Medium"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (importance_menu_item3),
                                   TRUE);
  gtk_widget_set_name (importance_menu_item3, "convertwidget11");
  gtk_widget_show (importance_menu_item3);

  gtk_menu_shell_append (GTK_MENU_SHELL (importance_menu),
                         importance_menu_item3);

  GtkWidget *importance_menu_item4 = gtk_menu_item_new_with_label (_ ("High"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (importance_menu_item4),
                                   TRUE);
  gtk_widget_set_name (importance_menu_item4, "convertwidget12");
  gtk_widget_show (importance_menu_item4);

  gtk_menu_shell_append (GTK_MENU_SHELL (importance_menu),
                         importance_menu_item4);
  gtk_widget_show (importance_menu);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (importance), importance_menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (importance), 0);
  dlg->importance = mugged (importance, dlg);
  gtk_widget_show (importance);

  gtk_table_attach (GTK_TABLE (sizing_table), importance, 1, 2, 1, 2, GTK_FILL,
                    0, 0, 0);

  GtkWidget *label40 = gtk_label_new (_ ("Status:"));
  gtk_label_set_justify (GTK_LABEL (label40), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label40), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label40), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label40), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label40), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label40), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label40), 0, 0);
  gtk_widget_set_name (label40, "label40");
  gtk_widget_show (label40);

  gtk_table_attach (GTK_TABLE (sizing_table), label40, 0, 1, 2, 3, GTK_FILL, 0,
                    0, 0);

  GtkWidget *status = gtk_option_menu_new ();
  gtk_widget_set_can_focus (status, TRUE);
  gtk_widget_set_name (status, "status menu");
  gtk_widget_set_tooltip_text (status,
                               _ ("What is the status of this project?"));

  GtkWidget *status_menu = gtk_menu_new ();
  gtk_widget_set_name (status_menu, "convertwidget13");

  GtkWidget *status_menu_item1
      = gtk_menu_item_new_with_label (_ ("No Status"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (status_menu_item1), TRUE);
  gtk_widget_set_name (status_menu_item1, "convertwidget14");
  gtk_widget_show (status_menu_item1);

  gtk_menu_shell_append (GTK_MENU_SHELL (status_menu), status_menu_item1);

  GtkWidget *status_menu_item2
      = gtk_menu_item_new_with_label (_ ("Not Started"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (status_menu_item2), TRUE);
  gtk_widget_set_name (status_menu_item2, "convertwidget15");
  gtk_widget_show (status_menu_item2);

  gtk_menu_shell_append (GTK_MENU_SHELL (status_menu), status_menu_item2);

  GtkWidget *status_menu_item3
      = gtk_menu_item_new_with_label (_ ("In Progress"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (status_menu_item3), TRUE);
  gtk_widget_set_name (status_menu_item3, "convertwidget16");
  gtk_widget_show (status_menu_item3);

  gtk_menu_shell_append (GTK_MENU_SHELL (status_menu), status_menu_item3);

  GtkWidget *status_menu_item4 = gtk_menu_item_new_with_label (_ ("On Hold"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (status_menu_item4), TRUE);
  gtk_widget_set_name (status_menu_item4, "convertwidget17");
  gtk_widget_show (status_menu_item4);

  gtk_menu_shell_append (GTK_MENU_SHELL (status_menu), status_menu_item4);

  GtkWidget *status_menu_item5
      = gtk_menu_item_new_with_label (_ ("Cancelled"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (status_menu_item5), TRUE);
  gtk_widget_set_name (status_menu_item5, "convertwidget18");
  gtk_widget_show (status_menu_item5);

  gtk_menu_shell_append (GTK_MENU_SHELL (status_menu), status_menu_item5);

  GtkWidget *status_menu_item6
      = gtk_menu_item_new_with_label (_ ("Completed"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (status_menu_item6), TRUE);
  gtk_widget_set_name (status_menu_item6, "convertwidget19");
  gtk_widget_show (status_menu_item6);

  gtk_menu_shell_append (GTK_MENU_SHELL (status_menu), status_menu_item6);
  gtk_widget_show (status_menu);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (status), status_menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (status), 0);
  dlg->status = mugged (status, dlg);
  gtk_widget_show (status);

  gtk_table_attach (GTK_TABLE (sizing_table), status, 1, 2, 2, 3, GTK_FILL, 0,
                    0, 0);

  GtkWidget *label41 = gtk_label_new (_ ("Planned Start:"));
  gtk_label_set_justify (GTK_LABEL (label41), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label41), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label41), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label41), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label41), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label41), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label41), 0, 0);
  gtk_widget_set_name (label41, "label41");
  gtk_widget_show (label41);

  gtk_table_attach (GTK_TABLE (sizing_table), label41, 0, 1, 3, 4, GTK_FILL, 0,
                    0, 0);

  GtkWidget *start_date = gnome_date_edit_new (0, TRUE, TRUE);
  dlg->start = dated (start_date, dlg);
  gnome_date_edit_set_popup_range (GNOME_DATE_EDIT (start_date), 7, 19);
  gtk_widget_set_name (start_date, "start date");
  gtk_widget_show (start_date);

  gtk_table_attach (GTK_TABLE (sizing_table), start_date, 1, 4, 3, 4,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *label42 = gtk_label_new (_ ("Planned Finish:"));
  gtk_label_set_justify (GTK_LABEL (label42), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label42), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label42), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label42), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label42), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label42), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label42), 0, 0);
  gtk_widget_set_name (label42, "label42");
  gtk_widget_show (label42);

  gtk_table_attach (GTK_TABLE (sizing_table), label42, 0, 1, 4, 5, GTK_FILL, 0,
                    0, 0);

  GtkWidget *end_date = gnome_date_edit_new (0, TRUE, TRUE);
  dlg->end = dated (end_date, dlg);
  gnome_date_edit_set_popup_range (GNOME_DATE_EDIT (end_date), 7, 19);
  gtk_widget_set_name (end_date, "end date");
  gtk_widget_show (end_date);

  gtk_table_attach (GTK_TABLE (sizing_table), end_date, 1, 4, 4, 5,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *label43 = gtk_label_new (_ ("Due Date:"));
  gtk_label_set_justify (GTK_LABEL (label43), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label43), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label43), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label43), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label43), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label43), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label43), 0, 0);
  gtk_widget_set_name (label43, "label43");
  gtk_widget_show (label43);

  gtk_table_attach (GTK_TABLE (sizing_table), label43, 0, 1, 5, 6, GTK_FILL, 0,
                    0, 0);

  GtkWidget *due_date = gnome_date_edit_new (0, TRUE, TRUE);
  dlg->due = dated (due_date, dlg);
  gnome_date_edit_set_popup_range (GNOME_DATE_EDIT (due_date), 7, 19);
  gtk_widget_set_name (due_date, "due date");
  gtk_widget_show (due_date);

  gtk_table_attach (GTK_TABLE (sizing_table), due_date, 1, 4, 5, 6,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *label44 = gtk_label_new (_ ("Hours to Finish:"));
  gtk_label_set_justify (GTK_LABEL (label44), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label44), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label44), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label44), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label44), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label44), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label44), 0, 0);
  gtk_widget_set_name (label44, "label44");
  gtk_widget_show (label44);

  gtk_table_attach (GTK_TABLE (sizing_table), label44, 0, 1, 6, 7, GTK_FILL, 0,
                    0, 0);

  GtkWidget *sizing_box = gtk_entry_new ();
  dlg->sizing = GTK_ENTRY (connect_changed (sizing_box, dlg));
  gtk_entry_set_activates_default (GTK_ENTRY (sizing_box), FALSE);
  gtk_entry_set_editable (GTK_ENTRY (sizing_box), TRUE);
  gtk_entry_set_has_frame (GTK_ENTRY (sizing_box), TRUE);
  gtk_entry_set_invisible_char (GTK_ENTRY (sizing_box), '*');
  gtk_entry_set_max_length (GTK_ENTRY (sizing_box), 0);
  gtk_entry_set_visibility (GTK_ENTRY (sizing_box), TRUE);
  gtk_widget_set_can_focus (sizing_box, TRUE);
  gtk_widget_set_name (sizing_box, "sizing box");
  gtk_widget_show (sizing_box);

  gtk_table_attach (GTK_TABLE (sizing_table), sizing_box, 1, 2, 6, 7,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *label45 = gtk_label_new (_ ("% Complete"));
  gtk_label_set_justify (GTK_LABEL (label45), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label45), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label45), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label45), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label45), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label45), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label45), 0, 0);
  gtk_widget_set_name (label45, "label45");
  gtk_widget_show (label45);

  gtk_table_attach (GTK_TABLE (sizing_table), label45, 0, 1, 7, 8, GTK_FILL, 0,
                    0, 0);

  GtkWidget *percent_box = gtk_entry_new ();
  dlg->percent = GTK_ENTRY (connect_changed (percent_box, dlg));
  gtk_entry_set_activates_default (GTK_ENTRY (percent_box), FALSE);
  gtk_entry_set_editable (GTK_ENTRY (percent_box), TRUE);
  gtk_entry_set_has_frame (GTK_ENTRY (percent_box), TRUE);
  gtk_entry_set_invisible_char (GTK_ENTRY (percent_box), '*');
  gtk_entry_set_max_length (GTK_ENTRY (percent_box), 0);
  gtk_entry_set_visibility (GTK_ENTRY (percent_box), TRUE);
  gtk_widget_set_can_focus (percent_box, TRUE);
  gtk_widget_set_name (percent_box, "percent box");
  gtk_widget_show (percent_box);

  gtk_table_attach (GTK_TABLE (sizing_table), percent_box, 1, 2, 7, 8,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  /* ------------------------------------------------------ */
  /* initialize menu values */

  mentry (dlg->urgency, "urgency", 0, GTT_UNDEFINED);
  mentry (dlg->urgency, "urgency", 1, GTT_LOW);
  mentry (dlg->urgency, "urgency", 2, GTT_MEDIUM);
  mentry (dlg->urgency, "urgency", 3, GTT_HIGH);

  mentry (dlg->importance, "importance", 0, GTT_UNDEFINED);
  mentry (dlg->importance, "importance", 1, GTT_LOW);
  mentry (dlg->importance, "importance", 2, GTT_MEDIUM);
  mentry (dlg->importance, "importance", 3, GTT_HIGH);

  mentry (dlg->status, "status", 0, GTT_NO_STATUS);
  mentry (dlg->status, "status", 1, GTT_NOT_STARTED);
  mentry (dlg->status, "status", 2, GTT_IN_PROGRESS);
  mentry (dlg->status, "status", 3, GTT_ON_HOLD);
  mentry (dlg->status, "status", 4, GTT_CANCELLED);
  mentry (dlg->status, "status", 5, GTT_COMPLETED);

  gnome_dialog_close_hides (GNOME_DIALOG (dlg->dlg), TRUE);

  return dlg;
}

/* ============================================================== */

static void
redraw (GttProject *prj, gpointer data)
{
  PropDlg *dlg = data;
  do_set_project (prj, dlg);
}

/* ============================================================== */

static PropDlg *dlog = NULL;

void
prop_dialog_show (GttProject *proj)
{
  if (!dlog)
    dlog = prop_dialog_new ();

  gtt_project_remove_notifier (dlog->proj, redraw, dlog);
  do_set_project (proj, dlog);
  gtt_project_add_notifier (proj, redraw, dlog);
  gtk_widget_show (GTK_WIDGET (dlog->dlg));
}

void
prop_dialog_set_project (GttProject *proj)
{
  if (!dlog)
    return;

  gtt_project_remove_notifier (dlog->proj, redraw, dlog);
  do_set_project (proj, dlog);
  gtt_project_add_notifier (proj, redraw, dlog);
}

static GtkWidget *
connect_changed (GtkWidget *const wdgt, PropDlg *const dlg)
{
  gtk_signal_connect_object (GTK_OBJECT (wdgt), "changed",
                             GTK_SIGNAL_FUNC (gnome_property_box_changed),
                             GTK_OBJECT (dlg->dlg));

  return wdgt;
}

static GnomeDateEdit *
dated (GtkWidget *const wdgt, PropDlg *const dlg)
{
  gtk_signal_connect_object (GTK_OBJECT (wdgt), "date_changed",
                             GTK_SIGNAL_FUNC (gnome_property_box_changed),
                             GTK_OBJECT (dlg->dlg));
  gtk_signal_connect_object (GTK_OBJECT (wdgt), "time_changed",
                             GTK_SIGNAL_FUNC (gnome_property_box_changed),
                             GTK_OBJECT (dlg->dlg));

  return GNOME_DATE_EDIT (wdgt);
}

static gpointer
get_menu (GtkOptionMenu *const wdgt, const gchar *const key)
{
  GtkWidget *const menu = gtk_option_menu_get_menu (wdgt);
  GtkWidget *const menu_item = gtk_menu_get_active (GTK_MENU (menu));

  return g_object_get_data (G_OBJECT (menu_item), key);
}

static void
mentry (GtkOptionMenu *const wdgt, const gchar *const name, const guint order,
        const int val)
{
  GtkMenu *const menu = GTK_MENU (gtk_option_menu_get_menu (wdgt));
  gtk_option_menu_set_history (wdgt, order);
  GtkWidget *const menu_item = gtk_menu_get_active (menu);
  g_object_set_data (G_OBJECT (menu_item), name, GINT_TO_POINTER (val));
}

static GtkOptionMenu *
mugged (GtkWidget *const wdgt, PropDlg *const dlg)
{
  GtkWidget *const mw = gtk_option_menu_get_menu (GTK_OPTION_MENU (wdgt));
  gtk_signal_connect_object (GTK_OBJECT (mw), "selection_done",
                             GTK_SIGNAL_FUNC (gnome_property_box_changed),
                             GTK_OBJECT (dlg->dlg));

  return GTK_OPTION_MENU (wdgt);
}

static GtkTextView *
texted (GtkWidget *const wdgt, PropDlg *const dlg)
{
  GtkTextView *const tv = GTK_TEXT_VIEW (wdgt);
  GtkTextBuffer *const tvb = gtk_text_view_get_buffer (tv);
  g_signal_connect_object (G_OBJECT (tvb), "changed", G_CALLBACK (wrapper),
                           G_OBJECT (dlg->dlg), 0);

  return tv;
}

/* ==================== END OF FILE ============================= */
