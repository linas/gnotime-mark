/*   Project Properties for GTimeTracker - a time tracker
 *   Copyright (C) 1997,98 Eckehard Berns
 *   Copyright (C) 2001,2002,2003 Linas Vepstas <linas@linas.org>
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

#include "gtt_props_dlg_project.h"

#include <glade/glade.h>
#include <gnome.h>
#include <string.h>

#include "gtt_date_edit.h"
#include "gtt_entry.h"
#include "gtt_help_popup.h"
#include "gtt_project.h"
#include "gtt_util.h"

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

  GttDateEdit *start;
  GttDateEdit *end;
  GttDateEdit *due;

  GtkEntry *sizing;
  GtkEntry *percent;

  GttProject *proj;
} PropDlg;

/* ============================================================== */

#define GET_MENU(WIDGET, NAME)                                                \
  ({                                                                          \
    GtkWidget *menu, *menu_item;                                              \
    menu = gtk_option_menu_get_menu (WIDGET);                                 \
    menu_item = gtk_menu_get_active (GTK_MENU (menu));                        \
    (g_object_get_data (G_OBJECT (menu_item), NAME));                         \
  })

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

      ivl = (long)GET_MENU (dlg->urgency, "urgency");
      gtt_project_set_urgency (dlg->proj, (GttRank)ivl);
      ivl = (long)GET_MENU (dlg->importance, "importance");
      gtt_project_set_importance (dlg->proj, (GttRank)ivl);

      ivl = (long)GET_MENU (dlg->status, "status");
      gtt_project_set_status (dlg->proj, (GttProjectStatus)ivl);

      tval = gtt_date_edit_get_time (dlg->start);
      gtt_project_set_estimated_start (dlg->proj, tval);
      tval = gtt_date_edit_get_time (dlg->end);
      gtt_project_set_estimated_end (dlg->proj, tval);
      tval = gtt_date_edit_get_time (dlg->due);
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

      gtt_date_edit_set_time (dlg->start, now);
      gtt_date_edit_set_time (dlg->end, now);
      gtt_date_edit_set_time (dlg->due, now + 86400);
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
  gtt_date_edit_set_time (dlg->start, tval);
  tval = gtt_project_get_estimated_end (proj);
  if (-1 == tval)
    tval = now + 3600;
  gtt_date_edit_set_time (dlg->end, tval);
  tval = gtt_project_get_due_date (proj);
  if (-1 == tval)
    tval = now + 86400;
  gtt_date_edit_set_time (dlg->due, tval);

  g_snprintf (buff, 132, "%.2f",
              ((double)gtt_project_get_sizing (proj)) / 3600.0);
  gtk_entry_set_text (dlg->sizing, buff);
  g_snprintf (buff, 132, "%d", gtt_project_get_percent_complete (proj));
  gtk_entry_set_text (dlg->percent, buff);

  /* set to unmodified as it reflects the current state of the project */
  gnome_property_box_set_modified (GNOME_PROPERTY_BOX (dlg->dlg), FALSE);
}

/* ============================================================== */

#define TAGGED(WDGT)                                                          \
  ({                                                                          \
    gtk_signal_connect_object (GTK_OBJECT (WDGT), "changed",                  \
                               GTK_SIGNAL_FUNC (gnome_property_box_changed),  \
                               GTK_OBJECT (dlg->dlg));                        \
    WDGT;                                                                     \
  })

#define DATED(WDGT)                                                           \
  ({                                                                          \
    gtk_signal_connect_object (GTK_OBJECT (WDGT), "date_changed",             \
                               GTK_SIGNAL_FUNC (gnome_property_box_changed),  \
                               GTK_OBJECT (dlg->dlg));                        \
    gtk_signal_connect_object (GTK_OBJECT (WDGT), "time_changed",             \
                               GTK_SIGNAL_FUNC (gnome_property_box_changed),  \
                               GTK_OBJECT (dlg->dlg));                        \
    GTT_DATE_EDIT (WDGT);                                                     \
  })

static void
wrapper (void *gobj, void *data)
{
  gnome_property_box_changed (GNOME_PROPERTY_BOX (data));
}

#define TEXTED(WDGT)                                                          \
  ({                                                                          \
    GtkTextBuffer *buff = gtk_text_view_get_buffer (GTK_TEXT_VIEW (WDGT));    \
    g_signal_connect_object (G_OBJECT (buff), "changed",                      \
                             G_CALLBACK (wrapper), G_OBJECT (dlg->dlg), 0);   \
    WDGT;                                                                     \
  })

#define MUGGED(WDGT)                                                          \
  ({                                                                          \
    GtkWidget *mw;                                                            \
    mw = gtk_option_menu_get_menu (GTK_OPTION_MENU (WDGT));                   \
    gtk_signal_connect_object (GTK_OBJECT (mw), "selection_done",             \
                               GTK_SIGNAL_FUNC (gnome_property_box_changed),  \
                               GTK_OBJECT (dlg->dlg));                        \
    GTK_OPTION_MENU (WDGT);                                                   \
  })

#define MENTRY(WIDGET, NAME, ORDER, VAL)                                      \
  {                                                                           \
    GtkWidget *menu_item;                                                     \
    GtkMenu *menu = GTK_MENU (gtk_option_menu_get_menu (WIDGET));             \
    gtk_option_menu_set_history (WIDGET, ORDER);                              \
    menu_item = gtk_menu_get_active (menu);                                   \
    g_object_set_data (G_OBJECT (menu_item), NAME, (gpointer)VAL);            \
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

  GtkWidget *title_table = glade_xml_get_widget (gtxml, "title table");

  GtkWidget *const title_label = gtk_label_new (_ ("Project Title:"));
  gtk_label_set_justify (GTK_LABEL (title_label), GTK_JUSTIFY_RIGHT);
  gtk_label_set_line_wrap (GTK_LABEL (title_label), FALSE);
  gtk_label_set_selectable (GTK_LABEL (title_label), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (title_label), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (title_label), FALSE);
  gtk_misc_set_alignment (GTK_MISC (title_label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (title_label), 0, 0);
  gtk_widget_set_name (title_label, "label18");
  gtk_widget_show (title_label);

  gtk_table_attach (GTK_TABLE (title_table), title_label, 0, 1, 0, 1, GTK_FILL,
                    0, 0, 0);

  GtkWidget *const entry7 = gtt_entry_new ("project-title");
  gtt_entry_set_max_saved (GTT_ENTRY (entry7), 10);
  gtk_widget_set_name (entry7, "entry7");

  GtkWidget *const title_box = gtt_entry_gtk_entry (GTT_ENTRY (entry7));
  dlg->title = GTK_ENTRY (TAGGED (title_box));
  gtk_entry_set_activates_default (GTK_ENTRY (title_box), FALSE);
  gtk_entry_set_editable (GTK_ENTRY (title_box), TRUE);
  gtk_entry_set_has_frame (GTK_ENTRY (title_box), TRUE);
  gtk_entry_set_invisible_char (GTK_ENTRY (title_box), '*');
  gtk_entry_set_max_length (GTK_ENTRY (title_box), 0);
  gtk_entry_set_visibility (GTK_ENTRY (title_box), TRUE);
  gtk_widget_set_can_default (title_box, TRUE);
  gtk_widget_set_name (title_box, "title box");
  gtk_widget_set_tooltip_text (title_box,
                               _ ("A title to assign to this project"));
  gtk_widget_show (title_box);

  gtk_widget_show (entry7);

  gtk_table_attach (GTK_TABLE (title_table), entry7, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const desc_label = gtk_label_new (_ ("Project Description:"));
  gtk_label_set_justify (GTK_LABEL (desc_label), GTK_JUSTIFY_RIGHT);
  gtk_label_set_line_wrap (GTK_LABEL (desc_label), FALSE);
  gtk_label_set_selectable (GTK_LABEL (desc_label), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (desc_label), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (desc_label), FALSE);
  gtk_misc_set_alignment (GTK_MISC (desc_label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (desc_label), 0, 0);
  gtk_widget_set_name (desc_label, "label19");
  gtk_widget_show (desc_label);

  gtk_table_attach (GTK_TABLE (title_table), desc_label, 0, 1, 1, 2, GTK_FILL,
                    0, 0, 0);

  GtkWidget *const entry9 = gtt_entry_new ("project-description");
  gtt_entry_set_max_saved (GTT_ENTRY (entry9), 10);
  gtk_widget_set_name (entry9, "entry9");

  GtkWidget *const desc_box = gtt_entry_gtk_entry (GTT_ENTRY (entry9));
  dlg->desc = GTK_ENTRY (TAGGED (desc_box));
  gtk_entry_set_activates_default (GTK_ENTRY (desc_box), FALSE);
  gtk_entry_set_editable (GTK_ENTRY (desc_box), TRUE);
  gtk_entry_set_has_frame (GTK_ENTRY (desc_box), TRUE);
  gtk_entry_set_invisible_char (GTK_ENTRY (desc_box), '*');
  gtk_entry_set_max_length (GTK_ENTRY (desc_box), 0);
  gtk_entry_set_visibility (GTK_ENTRY (desc_box), TRUE);
  gtk_widget_set_can_default (desc_box, TRUE);
  gtk_widget_set_name (desc_box, "desc box");
  gtk_widget_set_tooltip_text (
      desc_box,
      _ ("a short description that will be printed on the invoice."));
  gtk_widget_show (desc_box);

  gtk_widget_show (entry9);

  gtk_table_attach (GTK_TABLE (title_table), entry9, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const notes_label = gtk_label_new (_ ("Notes:"));
  gtk_label_set_justify (GTK_LABEL (notes_label), GTK_JUSTIFY_RIGHT);
  gtk_label_set_line_wrap (GTK_LABEL (notes_label), FALSE);
  gtk_label_set_selectable (GTK_LABEL (notes_label), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (notes_label), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (notes_label), FALSE);
  gtk_misc_set_alignment (GTK_MISC (notes_label), 0, 0);
  gtk_misc_set_padding (GTK_MISC (notes_label), 0, 0);
  gtk_widget_set_name (notes_label, "label20");
  gtk_widget_show (notes_label);

  gtk_table_attach (GTK_TABLE (title_table), notes_label, 0, 1, 2, 3, GTK_FILL,
                    0, 0, 0);

  GtkWidget *const scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_placement (GTK_SCROLLED_WINDOW (scrolledwindow1),
                                     GTK_CORNER_TOP_LEFT);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow1),
                                       GTK_SHADOW_IN);
  gtk_widget_set_name (scrolledwindow1, "scrolledwindow1");

  GtkWidget *const notes_box = gtk_text_view_new ();
  dlg->notes = GTK_TEXT_VIEW (TEXTED (notes_box));
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
                               _ ("Internal notes about the project that will "
                                  "not be printed on an invoice."));
  gtk_widget_show (notes_box);

  gtk_container_add (GTK_CONTAINER (scrolledwindow1), notes_box);
  gtk_widget_show (scrolledwindow1);

  gtk_table_attach (GTK_TABLE (title_table), scrolledwindow1, 1, 2, 2, 3,
                    GTK_FILL, 0, 0, 0);

  GtkWidget *rate_table = glade_xml_get_widget (gtxml, "rate table");

  GtkWidget *const label22 = gtk_label_new (_ ("Regular Rate:"));
  gtk_label_set_justify (GTK_LABEL (label22), GTK_JUSTIFY_RIGHT);
  gtk_label_set_line_wrap (GTK_LABEL (label22), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label22), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label22), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label22), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label22), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label22), 0, 0);
  gtk_widget_set_name (label22, "label22");
  gtk_widget_show (label22);

  gtk_table_attach (GTK_TABLE (rate_table), label22, 0, 1, 0, 1, GTK_FILL, 0,
                    0, 0);

  GtkWidget *const combo_regular = gtt_entry_new ("regular-rate");
  gtt_entry_set_max_saved (GTT_ENTRY (combo_regular), 10);
  gtk_widget_set_name (combo_regular, "combo regular");

  GtkWidget *const regular_box
      = gtt_entry_gtk_entry (GTT_ENTRY (combo_regular));
  dlg->regular = GTK_ENTRY (TAGGED (regular_box));
  gtk_entry_set_activates_default (GTK_ENTRY (regular_box), FALSE);
  gtk_entry_set_editable (GTK_ENTRY (regular_box), TRUE);
  gtk_entry_set_has_frame (GTK_ENTRY (regular_box), TRUE);
  gtk_entry_set_invisible_char (GTK_ENTRY (regular_box), '*');
  gtk_entry_set_max_length (GTK_ENTRY (regular_box), 0);
  gtk_entry_set_visibility (GTK_ENTRY (regular_box), TRUE);
  gtk_widget_set_can_default (regular_box, TRUE);
  gtk_widget_set_name (regular_box, "regular box");
  gtk_widget_set_tooltip_text (
      regular_box,
      _ ("The dollars per hour normally charged for this project."));
  gtk_widget_show (regular_box);

  gtk_widget_show (combo_regular);

  gtk_table_attach (GTK_TABLE (rate_table), combo_regular, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const label26 = gtk_label_new (_ ("        "));
  gtk_label_set_justify (GTK_LABEL (label26), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label26), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label26), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label26), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label26), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label26), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label26), 0, 0);
  gtk_widget_set_name (label26, "label26");
  gtk_widget_show (label26);

  gtk_table_attach (GTK_TABLE (rate_table), label26, 2, 3, 0, 1, GTK_FILL, 0,
                    0, 0);

  GtkWidget *const label23 = gtk_label_new (_ ("Overtime Rate:"));
  gtk_label_set_justify (GTK_LABEL (label23), GTK_JUSTIFY_RIGHT);
  gtk_label_set_line_wrap (GTK_LABEL (label23), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label23), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label23), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label23), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label23), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label23), 0, 0);
  gtk_widget_set_name (label23, "label23");
  gtk_widget_show (label23);

  gtk_table_attach (GTK_TABLE (rate_table), label23, 0, 1, 1, 2, GTK_FILL, 0,
                    0, 0);

  GtkWidget *const overtime_combo = gtt_entry_new ("overtime-rate");
  gtt_entry_set_max_saved (GTT_ENTRY (overtime_combo), 10);
  gtk_widget_set_name (overtime_combo, "overtime combo");

  GtkWidget *const overtime_box
      = gtt_entry_gtk_entry (GTT_ENTRY (overtime_combo));
  dlg->overtime = GTK_ENTRY (TAGGED (overtime_box));
  gtk_entry_set_activates_default (GTK_ENTRY (overtime_box), FALSE);
  gtk_entry_set_editable (GTK_ENTRY (overtime_box), TRUE);
  gtk_entry_set_has_frame (GTK_ENTRY (overtime_box), TRUE);
  gtk_entry_set_invisible_char (GTK_ENTRY (overtime_box), '*');
  gtk_entry_set_max_length (GTK_ENTRY (overtime_box), 0);
  gtk_entry_set_visibility (GTK_ENTRY (overtime_box), TRUE);
  gtk_widget_set_can_default (overtime_box, TRUE);
  gtk_widget_set_name (overtime_box, "overtime box");
  gtk_widget_set_tooltip_text (
      overtime_box,
      _ ("The dollars per hour charged for overtime work on this project."));
  gtk_widget_show (overtime_box);

  gtk_widget_show (overtime_combo);

  gtk_table_attach (GTK_TABLE (rate_table), overtime_combo, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const label27 = gtk_label_new (_ (""));
  gtk_label_set_justify (GTK_LABEL (label27), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label27), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label27), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label27), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label27), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label27), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label27), 0, 0);
  gtk_widget_set_name (label27, "label27");
  gtk_widget_show (label27);

  gtk_table_attach (GTK_TABLE (rate_table), label27, 2, 3, 1, 2, GTK_FILL, 0,
                    0, 0);

  GtkWidget *const label24 = gtk_label_new (_ ("Double-Overtime Rate:"));
  gtk_label_set_justify (GTK_LABEL (label24), GTK_JUSTIFY_RIGHT);
  gtk_label_set_line_wrap (GTK_LABEL (label24), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label24), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label24), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label24), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label24), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label24), 0, 0);
  gtk_widget_set_name (label24, "label24");
  gtk_widget_show (label24);

  gtk_table_attach (GTK_TABLE (rate_table), label24, 0, 1, 2, 3, GTK_FILL, 0,
                    0, 0);

  GtkWidget *const overover_combo = gtt_entry_new ("overover-rate");
  gtt_entry_set_max_saved (GTT_ENTRY (overover_combo), 10);
  gtk_widget_set_name (overover_combo, "overover combo");

  GtkWidget *const overover_box
      = gtt_entry_gtk_entry (GTT_ENTRY (overover_combo));
  dlg->overover = GTK_ENTRY (TAGGED (overover_box));
  gtk_entry_set_activates_default (GTK_ENTRY (overover_box), FALSE);
  gtk_entry_set_editable (GTK_ENTRY (overover_box), TRUE);
  gtk_entry_set_has_frame (GTK_ENTRY (overover_box), TRUE);
  gtk_entry_set_invisible_char (GTK_ENTRY (overover_box), '*');
  gtk_entry_set_max_length (GTK_ENTRY (overover_box), 0);
  gtk_entry_set_visibility (GTK_ENTRY (overover_box), TRUE);
  gtk_widget_set_can_default (overover_box, TRUE);
  gtk_widget_set_name (overover_box, "overover box");
  gtk_widget_set_tooltip_text (
      overover_box, _ ("The over-overtime rate (overtime on Sundays, etc.)"));
  gtk_widget_show (overover_box);

  gtk_widget_show (overover_combo);

  gtk_table_attach (GTK_TABLE (rate_table), overover_combo, 1, 2, 2, 3,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const label28 = gtk_label_new (_ (""));
  gtk_label_set_justify (GTK_LABEL (label28), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label28), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label28), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label28), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label28), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label28), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label28), 0, 0);
  gtk_widget_set_name (label28, "label28");
  gtk_widget_show (label28);

  gtk_table_attach (GTK_TABLE (rate_table), label28, 2, 3, 2, 3, GTK_FILL, 0,
                    0, 0);

  GtkWidget *const label25 = gtk_label_new (_ ("Flat Fee:"));
  gtk_label_set_justify (GTK_LABEL (label25), GTK_JUSTIFY_RIGHT);
  gtk_label_set_line_wrap (GTK_LABEL (label25), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label25), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label25), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label25), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label25), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label25), 0, 0);
  gtk_widget_set_name (label25, "label25");
  gtk_widget_show (label25);

  gtk_table_attach (GTK_TABLE (rate_table), label25, 0, 1, 3, 4, GTK_FILL, 0,
                    0, 0);

  GtkWidget *const flatfee_combo = gtt_entry_new ("flat-fee");
  gtt_entry_set_max_saved (GTT_ENTRY (flatfee_combo), 10);
  gtk_widget_set_name (flatfee_combo, "flatfee combo");

  GtkWidget *const flatfee_box
      = gtt_entry_gtk_entry (GTT_ENTRY (flatfee_combo));
  dlg->flatfee = GTK_ENTRY (TAGGED (flatfee_box));
  gtk_entry_set_activates_default (GTK_ENTRY (flatfee_box), FALSE);
  gtk_entry_set_editable (GTK_ENTRY (flatfee_box), TRUE);
  gtk_entry_set_has_frame (GTK_ENTRY (flatfee_box), TRUE);
  gtk_entry_set_invisible_char (GTK_ENTRY (flatfee_box), '*');
  gtk_entry_set_max_length (GTK_ENTRY (flatfee_box), 0);
  gtk_entry_set_visibility (GTK_ENTRY (flatfee_box), TRUE);
  gtk_widget_set_can_default (flatfee_box, TRUE);
  gtk_widget_set_name (flatfee_box, "flatfee box");
  gtk_widget_set_tooltip_text (
      flatfee_box, _ ("If this project is billed for one price no matter how "
                      "long it takes, enter the fee here."));
  gtk_widget_show (flatfee_box);

  gtk_widget_show (flatfee_combo);

  gtk_table_attach (GTK_TABLE (rate_table), flatfee_combo, 1, 2, 3, 4,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const label29 = gtk_label_new (_ ("                        "));
  gtk_label_set_justify (GTK_LABEL (label29), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (label29), FALSE);
  gtk_label_set_selectable (GTK_LABEL (label29), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (label29), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (label29), FALSE);
  gtk_misc_set_alignment (GTK_MISC (label29), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label29), 0, 0);
  gtk_widget_set_name (label29, "label29");
  gtk_widget_show (label29);

  gtk_table_attach (GTK_TABLE (rate_table), label29, 2, 3, 3, 4, GTK_FILL, 0,
                    0, 0);

  GtkWidget *interval_table = glade_xml_get_widget (gtxml, "interval table");

  GtkWidget *const label31 = gtk_label_new (_ ("Minimum Interval: "));
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

  GtkWidget *const entry22 = gtt_entry_new ("min-interval");
  gtt_entry_set_max_saved (GTT_ENTRY (entry22), 10);
  gtk_widget_set_name (entry22, "entry22");

  GtkWidget *const minimum_box = gtt_entry_gtk_entry (GTT_ENTRY (entry22));
  dlg->minimum = GTK_ENTRY (TAGGED (minimum_box));
  gtk_entry_set_activates_default (GTK_ENTRY (minimum_box), FALSE);
  gtk_entry_set_editable (GTK_ENTRY (minimum_box), TRUE);
  gtk_entry_set_has_frame (GTK_ENTRY (minimum_box), TRUE);
  gtk_entry_set_invisible_char (GTK_ENTRY (minimum_box), '*');
  gtk_entry_set_max_length (GTK_ENTRY (minimum_box), 0);
  gtk_entry_set_visibility (GTK_ENTRY (minimum_box), TRUE);
  gtk_widget_set_can_default (minimum_box, TRUE);
  gtk_widget_set_name (minimum_box, "minimum box");
  gtk_widget_set_tooltip_text (
      minimum_box, _ ("Intervals smaller than this will be discarded"));
  gtk_widget_show (minimum_box);

  gtk_widget_show (entry22);

  gtk_table_attach (GTK_TABLE (interval_table), entry22, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const label34 = gtk_label_new (_ ("seconds"));
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

  GtkWidget *const label32 = gtk_label_new (_ ("Auto-merge Interval:"));
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

  GtkWidget *const entry23 = gtt_entry_new ("merge-interval");
  gtt_entry_set_max_saved (GTT_ENTRY (entry23), 10);
  gtk_widget_set_name (entry23, "entry23");

  GtkWidget *const interval_box = gtt_entry_gtk_entry (GTT_ENTRY (entry23));
  dlg->interval = GTK_ENTRY (TAGGED (interval_box));
  gtk_entry_set_activates_default (GTK_ENTRY (interval_box), FALSE);
  gtk_entry_set_editable (GTK_ENTRY (interval_box), TRUE);
  gtk_entry_set_has_frame (GTK_ENTRY (interval_box), TRUE);
  gtk_entry_set_invisible_char (GTK_ENTRY (interval_box), '*');
  gtk_entry_set_max_length (GTK_ENTRY (interval_box), 0);
  gtk_entry_set_visibility (GTK_ENTRY (interval_box), TRUE);
  gtk_widget_set_can_default (interval_box, TRUE);
  gtk_widget_set_name (interval_box, "interval box");
  gtk_widget_set_tooltip_text (
      interval_box,
      _ ("Time below which an interval is merged with its neighbors"));
  gtk_widget_show (interval_box);

  gtk_widget_show (entry23);

  gtk_table_attach (GTK_TABLE (interval_table), entry23, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const label35 = gtk_label_new (_ ("seconds"));
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

  GtkWidget *const label33 = gtk_label_new (_ ("Auto-merge Gap:"));
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

  GtkWidget *const entry24 = gtt_entry_new ("merge-gap");
  gtt_entry_set_max_saved (GTT_ENTRY (entry24), 10);
  gtk_widget_set_name (entry24, "entry24");

  GtkWidget *const gap_box = gtt_entry_gtk_entry (GTT_ENTRY (entry24));
  dlg->gap = GTK_ENTRY (TAGGED (gap_box));
  gtk_entry_set_activates_default (GTK_ENTRY (gap_box), FALSE);
  gtk_entry_set_editable (GTK_ENTRY (gap_box), TRUE);
  gtk_entry_set_has_frame (GTK_ENTRY (gap_box), TRUE);
  gtk_entry_set_invisible_char (GTK_ENTRY (gap_box), '*');
  gtk_entry_set_max_length (GTK_ENTRY (gap_box), 0);
  gtk_entry_set_visibility (GTK_ENTRY (gap_box), TRUE);
  gtk_widget_set_can_default (gap_box, TRUE);
  gtk_widget_set_name (gap_box, "gap box");
  gtk_widget_set_tooltip_text (
      gap_box, _ ("If the gap between intervals is smaller than this, the "
                  "intervals will be merged together."));
  gtk_widget_show (gap_box);

  gtk_widget_show (entry24);

  gtk_table_attach (GTK_TABLE (interval_table), entry24, 1, 2, 2, 3,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const label36 = gtk_label_new (_ ("seconds"));
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

  gtk_signal_connect (GTK_OBJECT (dlg->dlg), "help", GTK_SIGNAL_FUNC (help_cb),
                      "projects-editing");

  gtk_signal_connect (GTK_OBJECT (dlg->dlg), "apply",
                      GTK_SIGNAL_FUNC (prop_set), dlg);

  /* ------------------------------------------------------ */
  /* grab the various entry boxes and hook them up */

  GtkWidget *const sizing_table = glade_xml_get_widget (gtxml, "sizing table");

  GtkWidget *const label38 = gtk_label_new (_ ("Urgency:"));
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

  GtkWidget *const urgency_menu = gtk_option_menu_new ();
  dlg->urgency = MUGGED (urgency_menu);
  gtk_widget_set_can_focus (urgency_menu, TRUE);
  gtk_widget_set_name (urgency_menu, "urgency menu");
  gtk_widget_set_tooltip_text (
      urgency_menu, _ ("Does this item need immediate attention? Note that "
                       "some urgent tasks might not be important.  For "
                       "example, Bill may want you to answer his email today, "
                       "but you may have better things to do today."));

  GtkWidget *const convertwidget3 = gtk_menu_new ();
  gtk_widget_set_name (convertwidget3, "convertwidget3");

  GtkWidget *const convertwidget4
      = gtk_menu_item_new_with_label (_ ("Not Set"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget4), TRUE);
  gtk_widget_set_name (convertwidget4, "convertwidget4");
  gtk_widget_show (convertwidget4);

  gtk_menu_append (GTK_MENU_SHELL (convertwidget3), convertwidget4);

  GtkWidget *const convertwidget5 = gtk_menu_item_new_with_label (_ ("Low"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget5), TRUE);
  gtk_widget_set_name (convertwidget5, "convertwidget5");
  gtk_widget_show (convertwidget5);

  gtk_menu_append (GTK_MENU_SHELL (convertwidget3), convertwidget5);

  GtkWidget *const convertwidget6
      = gtk_menu_item_new_with_label (_ ("Medium"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget6), TRUE);
  gtk_widget_set_name (convertwidget6, "convertwidget6");
  gtk_widget_show (convertwidget6);

  gtk_menu_append (GTK_MENU_SHELL (convertwidget3), convertwidget6);

  GtkWidget *const convertwidget7 = gtk_menu_item_new_with_label (_ ("High"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget7), TRUE);
  gtk_widget_set_name (convertwidget7, "convertwidget7");
  gtk_widget_show (convertwidget7);

  gtk_menu_append (GTK_MENU_SHELL (convertwidget3), convertwidget7);

  gtk_widget_show (convertwidget3);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (urgency_menu), convertwidget3);
  gtk_option_menu_set_history (GTK_OPTION_MENU (urgency_menu), 0);
  gtk_widget_show (urgency_menu);

  gtk_table_attach (GTK_TABLE (sizing_table), urgency_menu, 1, 2, 0, 1,
                    GTK_FILL, 0, 0, 0);

  GtkWidget *const label46 = gtk_label_new (_ ("            "));
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

  GtkWidget *const label47 = gtk_label_new (_ ("            "));
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

  GtkWidget *const label39 = gtk_label_new (_ ("Importance:"));
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

  GtkWidget *const importance_menu = gtk_option_menu_new ();
  dlg->importance = MUGGED (importance_menu);
  gtk_widget_set_can_focus (importance_menu, TRUE);
  gtk_widget_set_name (importance_menu, "importance menu");
  gtk_widget_set_tooltip_text (
      importance_menu,
      _ ("How important is it to perform this task?  Not everything important "
         "is urgent.  For example, it is important to file a tax return every "
         "year, but you have a lot of time to get ready to do this."));

  GtkWidget *const convertwidget8 = gtk_menu_new ();
  gtk_widget_set_name (convertwidget8, "convertwidget8");

  GtkWidget *const convertwidget9
      = gtk_menu_item_new_with_label (_ ("Not Set"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget9), TRUE);
  gtk_widget_set_name (convertwidget9, "convertwidget9");
  gtk_widget_show (convertwidget9);

  gtk_menu_append (GTK_MENU_SHELL (convertwidget8), convertwidget9);

  GtkWidget *const convertwidget10 = gtk_menu_item_new_with_label (_ ("Low"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget10), TRUE);
  gtk_widget_set_name (convertwidget10, "convertwidget10");
  gtk_widget_show (convertwidget10);

  gtk_menu_append (GTK_MENU_SHELL (convertwidget8), convertwidget10);

  GtkWidget *const convertwidget11
      = gtk_menu_item_new_with_label (_ ("Medium"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget11), TRUE);
  gtk_widget_set_name (convertwidget11, "convertwidget11");
  gtk_widget_show (convertwidget11);

  gtk_menu_append (GTK_MENU_SHELL (convertwidget8), convertwidget11);

  GtkWidget *const convertwidget12 = gtk_menu_item_new_with_label (_ ("High"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget12), TRUE);
  gtk_widget_set_name (convertwidget12, "convertwidget12");
  gtk_widget_show (convertwidget12);

  gtk_menu_append (GTK_MENU_SHELL (convertwidget8), convertwidget12);

  gtk_widget_show (convertwidget8);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (importance_menu), convertwidget8);
  gtk_option_menu_set_history (GTK_OPTION_MENU (importance_menu), 0);
  gtk_widget_show (importance_menu);

  gtk_table_attach (GTK_TABLE (sizing_table), importance_menu, 1, 2, 1, 2,
                    GTK_FILL, 0, 0, 0);

  GtkWidget *const label40 = gtk_label_new (_ ("Status:"));
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

  GtkWidget *const status_menu = gtk_option_menu_new ();
  dlg->status = MUGGED (status_menu);
  gtk_widget_set_can_focus (status_menu, TRUE);
  gtk_widget_set_name (status_menu, "status menu");
  gtk_widget_set_tooltip_text (status_menu,
                               _ ("What is the status of this project?"));

  GtkWidget *const convertwidget13 = gtk_menu_new ();
  gtk_widget_set_name (convertwidget13, "convertwidget3");

  GtkWidget *const convertwidget14
      = gtk_menu_item_new_with_label (_ ("No Status"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget14), TRUE);
  gtk_widget_set_name (convertwidget14, "convertwidget14");
  gtk_widget_show (convertwidget14);

  gtk_menu_append (GTK_MENU_SHELL (convertwidget13), convertwidget14);

  GtkWidget *const convertwidget15
      = gtk_menu_item_new_with_label (_ ("Not Started"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget15), TRUE);
  gtk_widget_set_name (convertwidget15, "convertwidget15");
  gtk_widget_show (convertwidget15);

  gtk_menu_append (GTK_MENU_SHELL (convertwidget13), convertwidget15);

  GtkWidget *const convertwidget16
      = gtk_menu_item_new_with_label (_ ("In Progress"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget16), TRUE);
  gtk_widget_set_name (convertwidget16, "convertwidget16");
  gtk_widget_show (convertwidget16);

  gtk_menu_append (GTK_MENU_SHELL (convertwidget13), convertwidget16);

  GtkWidget *const convertwidget17
      = gtk_menu_item_new_with_label (_ ("On Hold"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget17), TRUE);
  gtk_widget_set_name (convertwidget17, "convertwidget17");
  gtk_widget_show (convertwidget17);

  gtk_menu_append (GTK_MENU_SHELL (convertwidget13), convertwidget17);

  GtkWidget *const convertwidget18
      = gtk_menu_item_new_with_label (_ ("Cancelled"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget18), TRUE);
  gtk_widget_set_name (convertwidget18, "convertwidget18");
  gtk_widget_show (convertwidget18);

  gtk_menu_append (GTK_MENU_SHELL (convertwidget13), convertwidget18);

  GtkWidget *const convertwidget19
      = gtk_menu_item_new_with_label (_ ("Completed"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (convertwidget19), TRUE);
  gtk_widget_set_name (convertwidget19, "convertwidget19");
  gtk_widget_show (convertwidget19);

  gtk_menu_append (GTK_MENU_SHELL (convertwidget13), convertwidget19);

  gtk_widget_show (convertwidget13);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (status_menu), convertwidget13);
  gtk_option_menu_set_history (GTK_OPTION_MENU (status_menu), 0);
  gtk_widget_show (status_menu);

  gtk_table_attach (GTK_TABLE (sizing_table), status_menu, 1, 2, 2, 3,
                    GTK_FILL, 0, 0, 0);

  GtkWidget *const label41 = gtk_label_new (_ ("Planned Start:"));
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

  GtkWidget *const start_date = gtt_date_edit_new_flags (
      0, GTT_DATE_EDIT_24_HR | GTT_DATE_EDIT_SHOW_TIME);
  dlg->start = DATED (start_date);
  gtt_date_edit_set_popup_range (GTT_DATE_EDIT (start_date), 7, 19);
  gtk_widget_set_name (start_date, "start date");
  gtk_widget_show (start_date);

  gtk_table_attach (GTK_TABLE (sizing_table), start_date, 1, 4, 3, 4,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const label42 = gtk_label_new (_ ("Planned Finish:"));
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

  GtkWidget *const end_date = gtt_date_edit_new_flags (
      0, GTT_DATE_EDIT_24_HR | GTT_DATE_EDIT_SHOW_TIME);
  dlg->end = DATED (end_date);
  gtt_date_edit_set_popup_range (GTT_DATE_EDIT (end_date), 7, 19);
  gtk_widget_set_name (end_date, "end date");
  gtk_widget_show (end_date);

  gtk_table_attach (GTK_TABLE (sizing_table), end_date, 1, 4, 4, 5,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const label43 = gtk_label_new (_ ("Due Date:"));
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

  GtkWidget *const due_date = gtt_date_edit_new_flags (
      0, GTT_DATE_EDIT_24_HR | GTT_DATE_EDIT_SHOW_TIME);
  dlg->due = DATED (due_date);
  gtt_date_edit_set_popup_range (GTT_DATE_EDIT (due_date), 7, 19);
  gtk_widget_set_name (due_date, "due date");
  gtk_widget_show (due_date);

  gtk_table_attach (GTK_TABLE (sizing_table), due_date, 1, 4, 5, 6,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const label44 = gtk_label_new (_ ("Hours to Finish:"));
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

  GtkWidget *const sizing_box = gtk_entry_new ();
  dlg->sizing = GTK_ENTRY (TAGGED (sizing_box));
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

  GtkWidget *const label45 = gtk_label_new (_ ("% Complete"));
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

  GtkWidget *const percent_box = gtk_entry_new ();
  dlg->percent = GTK_ENTRY (TAGGED (percent_box));
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

  MENTRY (dlg->urgency, "urgency", 0, GTT_UNDEFINED);
  MENTRY (dlg->urgency, "urgency", 1, GTT_LOW);
  MENTRY (dlg->urgency, "urgency", 2, GTT_MEDIUM);
  MENTRY (dlg->urgency, "urgency", 3, GTT_HIGH);

  MENTRY (dlg->importance, "importance", 0, GTT_UNDEFINED);
  MENTRY (dlg->importance, "importance", 1, GTT_LOW);
  MENTRY (dlg->importance, "importance", 2, GTT_MEDIUM);
  MENTRY (dlg->importance, "importance", 3, GTT_HIGH);

  MENTRY (dlg->status, "status", 0, GTT_NO_STATUS);
  MENTRY (dlg->status, "status", 1, GTT_NOT_STARTED);
  MENTRY (dlg->status, "status", 2, GTT_IN_PROGRESS);
  MENTRY (dlg->status, "status", 3, GTT_ON_HOLD);
  MENTRY (dlg->status, "status", 4, GTT_CANCELLED);
  MENTRY (dlg->status, "status", 5, GTT_COMPLETED);

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

/* ==================== END OF FILE ============================= */
