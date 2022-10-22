/*   GUI dialog for global application preferences for GTimeTracker
 *   Copyright (C) 1997,98 Eckehard Berns
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

#include <glade/glade.h>
#include <gnome.h>
#include <qof.h>
#include <string.h>

#include "app.h"
#include "cur-proj.h"
#include "dialog.h"
#include "gtt.h"
#include "prefs.h"
#include "timer.h"
#include "toolbar.h"
#include "util.h"

/* globals */
int config_show_secs = 0;
int config_show_statusbar = 1;
int config_show_clist_titles = 1;
int config_show_subprojects = 1;
int config_show_title_ever = 1;
int config_show_title_year = 0;
int config_show_title_month = 0;
int config_show_title_week = 0;
int config_show_title_lastweek = 0;
int config_show_title_day = 1;
int config_show_title_yesterday = 0;
int config_show_title_current = 0;
int config_show_title_desc = 1;
int config_show_title_task = 1;
int config_show_title_estimated_start = 0;
int config_show_title_estimated_end = 0;
int config_show_title_due_date = 0;
int config_show_title_sizing = 0;
int config_show_title_percent_complete = 0;
int config_show_title_urgency = 1;
int config_show_title_importance = 1;
int config_show_title_status = 0;

int config_show_toolbar = 1;
int config_show_tb_tips = 1;
int config_show_tb_new = 1;
int config_show_tb_ccp = 0;
int config_show_tb_journal = 1;
int config_show_tb_calendar = 0;
int config_show_tb_prop = 1;
int config_show_tb_timer = 1;
int config_show_tb_pref = 0;
int config_show_tb_help = 1;
int config_show_tb_exit = 1;

char *config_logfile_name = NULL;
char *config_logfile_start = NULL;
char *config_logfile_stop = NULL;
int config_logfile_use = 0;
int config_logfile_min_secs = 0;

int config_daystart_offset = 0;
int config_weekstart_offset = 0;

int config_time_format = TIME_FORMAT_LOCALE;

char *config_currency_symbol = NULL;
int config_currency_use_locale = 1;

char *config_data_url = NULL;

typedef struct _PrefsDialog
{
  GladeXML *gtxml;
  GnomePropertyBox *dlg;
  GtkCheckButton *show_secs;
  GtkCheckButton *show_statusbar;
  GtkCheckButton *show_clist_titles;
  GtkCheckButton *show_subprojects;

  GtkCheckButton *show_title_importance;
  GtkCheckButton *show_title_urgency;
  GtkCheckButton *show_title_status;
  GtkCheckButton *show_title_ever;
  GtkCheckButton *show_title_year;
  GtkCheckButton *show_title_month;
  GtkCheckButton *show_title_week;
  GtkCheckButton *show_title_lastweek;
  GtkCheckButton *show_title_day;
  GtkCheckButton *show_title_yesterday;
  GtkCheckButton *show_title_current;
  GtkCheckButton *show_title_desc;
  GtkCheckButton *show_title_task;
  GtkCheckButton *show_title_estimated_start;
  GtkCheckButton *show_title_estimated_end;
  GtkCheckButton *show_title_due_date;
  GtkCheckButton *show_title_sizing;
  GtkCheckButton *show_title_percent_complete;

  GtkCheckButton *logfileuse;
  GtkWidget *logfilename_l;
  GtkFileChooser *logfilename;
  GtkWidget *logfilestart_l;
  GtkEntry *logfilestart;
  GtkWidget *logfilestop_l;
  GtkEntry *logfilestop;
  GtkWidget *logfileminsecs_l;
  GtkEntry *logfileminsecs;

  GtkEntry *shell_start;
  GtkEntry *shell_stop;

  GtkCheckButton *show_toolbar;
  GtkCheckButton *show_tb_tips;
  GtkCheckButton *show_tb_new;
  GtkCheckButton *show_tb_ccp;
  GtkCheckButton *show_tb_journal;
  GtkCheckButton *show_tb_pref;
  GtkCheckButton *show_tb_timer;
  GtkCheckButton *show_tb_prop;
  GtkCheckButton *show_tb_help;
  GtkCheckButton *show_tb_exit;

  GtkEntry *idle_secs;
  GtkEntry *no_project_secs;
  GtkEntry *daystart_secs;
  GtkComboBox *daystart_menu;
  GtkComboBox *weekstart_menu;

  GtkRadioButton *time_format_am_pm;
  GtkRadioButton *time_format_24_hs;
  GtkRadioButton *time_format_locale;

  GtkEntry *currency_symbol;
  GtkWidget *currency_symbol_label;
  GtkCheckButton *currency_use_locale;

} PrefsDialog;

static GtkCheckButton *dlgwid (GladeXML *gtxml, const gchar *strname,
                               PrefsDialog *dlg);
static void entry_to_char (GtkEntry *e, gchar **str);
static GtkWidget *getchwid (GladeXML *gtxml, const gchar *name,
                            PrefsDialog *dlg);
static GtkWidget *getwid (GladeXML *gtxml, const gchar *name,
                          PrefsDialog *dlg);
static void set_active (GtkCheckButton *chk_btn, int option);
static void set_val (int *to, int from, int *change);
static void show_check (GtkCheckButton *chk_btn, int *option, int *change);
static GtkCheckButton *tbwid (GladeXML *gtxml, const gchar *strname,
                              PrefsDialog *dlg);

/* Update the properties of the project view according to current settings */

void
prefs_update_projects_view (void)
{
  GList *columns = NULL;

  if (config_show_title_importance)
    {
      columns = g_list_insert (columns, "importance", -1);
    }

  if (config_show_title_urgency)
    {
      columns = g_list_insert (columns, "urgency", -1);
    }

  if (config_show_title_status)
    {
      columns = g_list_insert (columns, "status", -1);
    }

  if (config_show_title_ever)
    {
      columns = g_list_insert (columns, "time_ever", -1);
    }

  if (config_show_title_year)
    {
      columns = g_list_insert (columns, "time_year", -1);
    }

  if (config_show_title_month)
    {
      columns = g_list_insert (columns, "time_month", -1);
    }

  if (config_show_title_week)
    {
      columns = g_list_insert (columns, "time_week", -1);
    }

  if (config_show_title_lastweek)
    {
      columns = g_list_insert (columns, "time_lastweek", -1);
    }

  if (config_show_title_yesterday)
    {
      columns = g_list_insert (columns, "time_yesterday", -1);
    }

  if (config_show_title_day)
    {
      columns = g_list_insert (columns, "time_today", -1);
    }

  if (config_show_title_current)
    {
      columns = g_list_insert (columns, "time_task", -1);
    }

  /* The title column is mandatory */
  columns = g_list_insert (columns, "title", -1);

  if (config_show_title_desc)
    {
      columns = g_list_insert (columns, "description", -1);
    }

  if (config_show_title_task)
    {
      columns = g_list_insert (columns, "task", -1);
    }

  if (config_show_title_estimated_start)
    {
      columns = g_list_insert (columns, "estimated_start", -1);
    }

  if (config_show_title_estimated_end)
    {
      columns = g_list_insert (columns, "estimated_end", -1);
    }

  if (config_show_title_due_date)
    {
      columns = g_list_insert (columns, "due_date", -1);
    }

  if (config_show_title_sizing)
    {
      columns = g_list_insert (columns, "sizing", -1);
    }

  if (config_show_title_percent_complete)
    {
      columns = g_list_insert (columns, "percent_done", -1);
    }

  gtt_projects_tree_set_visible_columns (projects_tree, columns);
  g_list_free (columns);

  gtk_tree_view_set_enable_tree_lines (GTK_TREE_VIEW (projects_tree),
                                       config_show_subprojects);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (projects_tree),
                                     config_show_clist_titles);
}

void
prefs_set_show_secs ()
{
  gtt_projects_tree_set_show_seconds (projects_tree, config_show_secs);
}

/* ============================================================== */
/** parse an HH:MM:SS string for the time returning seconds
 * XXX should probably use getdate or fdate or something like that
 */

static int
scan_time_string (const char *str)
{
  int hours = 0, minutes = 0, seconds = 0;
  char buff[24];
  strncpy (buff, str, 24);
  buff[23] = 0;
  char *p = strchr (buff, ':');
  if (p)
    *p = 0;
  hours = atoi (buff);
  if (p)
    {
      char *m = ++p;
      p = strchr (m, ':');
      if (p)
        *p = 0;
      minutes = atoi (m);
      if (p)
        {
          seconds = atoi (++p);
        }
    }
  seconds %= 60;
  minutes %= 60;
  hours %= 24;

  int totalsecs = hours * 3600 + minutes * 60 + seconds;
  if (12 * 3600 < totalsecs)
    totalsecs -= 24 * 3600;
  return totalsecs;
}

/* ============================================================== */

static void
toolbar_sensitive_cb (GtkWidget *w, PrefsDialog *odlg)
{
  int state;

  state = GTK_TOGGLE_BUTTON (odlg->show_toolbar)->active;
  gtk_widget_set_sensitive (GTK_WIDGET (odlg->show_tb_new), state);
  gtk_widget_set_sensitive (GTK_WIDGET (odlg->show_tb_ccp), state);
  gtk_widget_set_sensitive (GTK_WIDGET (odlg->show_tb_journal), state);
  gtk_widget_set_sensitive (GTK_WIDGET (odlg->show_tb_pref), state);
  gtk_widget_set_sensitive (GTK_WIDGET (odlg->show_tb_timer), state);
  gtk_widget_set_sensitive (GTK_WIDGET (odlg->show_tb_prop), state);
  gtk_widget_set_sensitive (GTK_WIDGET (odlg->show_tb_help), state);
  gtk_widget_set_sensitive (GTK_WIDGET (odlg->show_tb_exit), state);

  // gtk_widget_set_sensitive(odlg->logfilename_l, state);
}

/* ============================================================== */

static void
prefs_set (GnomePropertyBox *pb, gint page, PrefsDialog *odlg)
{
  int state;

  if (0 == page)
    {
      int change = 0;

      show_check (odlg->show_title_importance, &config_show_title_importance,
                  &change);
      show_check (odlg->show_title_urgency, &config_show_title_urgency,
                  &change);
      show_check (odlg->show_title_status, &config_show_title_status, &change);
      show_check (odlg->show_title_ever, &config_show_title_ever, &change);
      show_check (odlg->show_title_year, &config_show_title_year, &change);
      show_check (odlg->show_title_month, &config_show_title_month, &change);
      show_check (odlg->show_title_week, &config_show_title_week, &change);
      show_check (odlg->show_title_lastweek, &config_show_title_lastweek,
                  &change);
      show_check (odlg->show_title_day, &config_show_title_day, &change);
      show_check (odlg->show_title_yesterday, &config_show_title_yesterday,
                  &change);
      show_check (odlg->show_title_current, &config_show_title_current,
                  &change);
      show_check (odlg->show_title_desc, &config_show_title_desc, &change);
      show_check (odlg->show_title_task, &config_show_title_task, &change);
      show_check (odlg->show_title_estimated_start,
                  &config_show_title_estimated_start, &change);
      show_check (odlg->show_title_estimated_end,
                  &config_show_title_estimated_end, &change);
      show_check (odlg->show_title_due_date, &config_show_title_due_date,
                  &change);
      show_check (odlg->show_title_sizing, &config_show_title_sizing, &change);
      show_check (odlg->show_title_percent_complete,
                  &config_show_title_percent_complete, &change);

      if (change)
        {
          prefs_update_projects_view ();
        }
    }
  if (1 == page)
    {

      /* display options */
      state = GTK_TOGGLE_BUTTON (odlg->show_secs)->active;
      if (state != config_show_secs)
        {
          config_show_secs = state;
          prefs_set_show_secs ();
          update_status_bar ();
          if (status_bar)
            gtk_widget_queue_resize (status_bar);
          start_main_timer ();
        }
      if (GTK_TOGGLE_BUTTON (odlg->show_statusbar)->active)
        {
          gtk_widget_show (GTK_WIDGET (status_bar));
          config_show_statusbar = 1;
        }
      else
        {
          gtk_widget_hide (GTK_WIDGET (status_bar));
          config_show_statusbar = 0;
        }
      if (GTK_TOGGLE_BUTTON (odlg->show_clist_titles)->active)
        {
          config_show_clist_titles = 1;
        }
      else
        {
          config_show_clist_titles = 0;
        }

      if (GTK_TOGGLE_BUTTON (odlg->show_subprojects)->active)
        {
          config_show_subprojects = 1;
        }
      else
        {
          config_show_subprojects = 0;
        }
      prefs_update_projects_view ();
    }

  if (2 == page)
    {
      /* shell command options */
      entry_to_char (odlg->shell_start, &config_shell_start);
      entry_to_char (odlg->shell_stop, &config_shell_stop);
    }

  if (3 == page)
    {
      /* log file options */
      config_logfile_use = GTK_TOGGLE_BUTTON (odlg->logfileuse)->active;
      config_logfile_name = gtk_file_chooser_get_filename (odlg->logfilename);
      entry_to_char (odlg->logfilestart, &config_logfile_start);
      entry_to_char (odlg->logfilestop, &config_logfile_stop);
      config_logfile_min_secs
          = atoi (gtk_entry_get_text (odlg->logfileminsecs));
    }

  if (4 == page)
    {
      int change = 0;

      /* toolbar */
      config_show_toolbar = GTK_TOGGLE_BUTTON (odlg->show_toolbar)->active;
      config_show_tb_tips = GTK_TOGGLE_BUTTON (odlg->show_tb_tips)->active;

      /* toolbar sections */
      show_check (odlg->show_tb_new, &config_show_tb_new, &change);
      show_check (odlg->show_tb_ccp, &config_show_tb_ccp, &change);
      show_check (odlg->show_tb_journal, &config_show_tb_journal, &change);
      show_check (odlg->show_tb_prop, &config_show_tb_prop, &change);
      show_check (odlg->show_tb_timer, &config_show_tb_timer, &change);
      show_check (odlg->show_tb_pref, &config_show_tb_pref, &change);
      show_check (odlg->show_tb_help, &config_show_tb_help, &change);
      show_check (odlg->show_tb_exit, &config_show_tb_exit, &change);

      if (change)
        {
          update_toolbar_sections ();
        }

      toolbar_set_states ();
    }

  if (5 == page)
    {
      int change = 0;
      config_idle_timeout
          = atoi (gtk_entry_get_text (GTK_ENTRY (odlg->idle_secs)));
      config_no_project_timeout
          = atoi (gtk_entry_get_text (GTK_ENTRY (odlg->no_project_secs)));

      if (timer_is_running ())
        {
          start_idle_timer ();
        }
      else
        {
          start_no_project_timer ();
        }

      /* Hunt for the hour-of night on which to start */
      const char *buff = gtk_entry_get_text (odlg->daystart_secs);
      int off = scan_time_string (buff);
      set_val (&config_daystart_offset, off, &change);

      int day = gtk_combo_box_get_active (odlg->weekstart_menu);
      set_val (&config_weekstart_offset, day, &change);

      if (change)
        {
          /* Need to recompute everything, including the bining */
          gtt_project_list_compute_secs ();
          gtt_projects_tree_update_all_rows (projects_tree);
        }
    }

  if (6 == page)
    {
      if (gtk_toggle_button_get_active (
              GTK_TOGGLE_BUTTON (odlg->time_format_am_pm)))
        {
          config_time_format = TIME_FORMAT_AM_PM;
        }
      if (gtk_toggle_button_get_active (
              GTK_TOGGLE_BUTTON (odlg->time_format_24_hs)))
        {
          config_time_format = TIME_FORMAT_24_HS;
        }
      if (gtk_toggle_button_get_active (
              GTK_TOGGLE_BUTTON (odlg->time_format_locale)))
        {
          config_time_format = TIME_FORMAT_LOCALE;
        }

      entry_to_char (odlg->currency_symbol, &config_currency_symbol);
      int state = GTK_TOGGLE_BUTTON (odlg->currency_use_locale)->active;
      if (config_currency_use_locale != state)
        {
          config_currency_use_locale = state;
        }
    }

  /* Also save them the to file at this point */
  save_properties ();
}

/* ============================================================== */

static void
logfile_sensitive_cb (GtkWidget *w, PrefsDialog *odlg)
{
  int state;

  state = GTK_TOGGLE_BUTTON (odlg->logfileuse)->active;
  gtk_widget_set_sensitive (GTK_WIDGET (odlg->logfilename), state);
  gtk_widget_set_sensitive (odlg->logfilename_l, state);
  gtk_widget_set_sensitive (GTK_WIDGET (odlg->logfilestart), state);
  gtk_widget_set_sensitive (odlg->logfilestart_l, state);
  gtk_widget_set_sensitive (GTK_WIDGET (odlg->logfilestop), state);
  gtk_widget_set_sensitive (odlg->logfilestop_l, state);
  gtk_widget_set_sensitive (GTK_WIDGET (odlg->logfileminsecs), state);
  gtk_widget_set_sensitive (odlg->logfileminsecs_l, state);
}

static void
currency_sensitive_cb (GtkWidget *w, PrefsDialog *odlg)
{
  int state;

  state = GTK_TOGGLE_BUTTON (w)->active;
  gtk_widget_set_sensitive (GTK_WIDGET (odlg->currency_symbol), !state);
  gtk_widget_set_sensitive (GTK_WIDGET (odlg->currency_symbol_label), !state);
}

static void
options_dialog_set (PrefsDialog *odlg)
{
  char s[30];

  set_active (odlg->show_secs, config_show_secs);
  set_active (odlg->show_statusbar, config_show_statusbar);
  set_active (odlg->show_clist_titles, config_show_clist_titles);
  set_active (odlg->show_subprojects, config_show_subprojects);

  set_active (odlg->show_title_importance, config_show_title_importance);
  set_active (odlg->show_title_urgency, config_show_title_urgency);
  set_active (odlg->show_title_status, config_show_title_status);
  set_active (odlg->show_title_ever, config_show_title_ever);
  set_active (odlg->show_title_year, config_show_title_year);
  set_active (odlg->show_title_month, config_show_title_month);
  set_active (odlg->show_title_week, config_show_title_week);
  set_active (odlg->show_title_lastweek, config_show_title_lastweek);
  set_active (odlg->show_title_day, config_show_title_day);
  set_active (odlg->show_title_yesterday, config_show_title_yesterday);
  set_active (odlg->show_title_current, config_show_title_current);
  set_active (odlg->show_title_desc, config_show_title_desc);
  set_active (odlg->show_title_task, config_show_title_task);
  set_active (odlg->show_title_estimated_start,
              config_show_title_estimated_start);
  set_active (odlg->show_title_estimated_end, config_show_title_estimated_end);
  set_active (odlg->show_title_due_date, config_show_title_due_date);
  set_active (odlg->show_title_sizing, config_show_title_sizing);
  set_active (odlg->show_title_percent_complete,
              config_show_title_percent_complete);

  if (config_shell_start)
    gtk_entry_set_text (odlg->shell_start, config_shell_start);
  else
    gtk_entry_set_text (odlg->shell_start, "");

  if (config_shell_stop)
    gtk_entry_set_text (odlg->shell_stop, config_shell_stop);
  else
    gtk_entry_set_text (odlg->shell_stop, "");

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (odlg->logfileuse),
                                config_logfile_use);
  if (config_logfile_name)
    gtk_file_chooser_set_filename (odlg->logfilename, config_logfile_name);
  else
    gtk_file_chooser_unselect_all (odlg->logfilename);

  if (config_logfile_start)
    gtk_entry_set_text (odlg->logfilestart, config_logfile_start);
  else
    gtk_entry_set_text (odlg->logfilestart, "");

  if (config_logfile_stop)
    gtk_entry_set_text (odlg->logfilestop, config_logfile_stop);
  else
    gtk_entry_set_text (odlg->logfilestop, "");

  g_snprintf (s, sizeof (s), "%d", config_logfile_min_secs);
  gtk_entry_set_text (GTK_ENTRY (odlg->logfileminsecs), s);

  logfile_sensitive_cb (NULL, odlg);

  /* toolbar sections */
  set_active (odlg->show_toolbar, config_show_toolbar);
  set_active (odlg->show_tb_tips, config_show_tb_tips);
  set_active (odlg->show_tb_new, config_show_tb_new);
  set_active (odlg->show_tb_ccp, config_show_tb_ccp);
  set_active (odlg->show_tb_journal, config_show_tb_journal);
  set_active (odlg->show_tb_prop, config_show_tb_prop);
  set_active (odlg->show_tb_timer, config_show_tb_timer);
  set_active (odlg->show_tb_pref, config_show_tb_pref);
  set_active (odlg->show_tb_help, config_show_tb_help);
  set_active (odlg->show_tb_exit, config_show_tb_exit);

  toolbar_sensitive_cb (NULL, odlg);

  /* misc section */
  g_snprintf (s, sizeof (s), "%d", config_idle_timeout);
  gtk_entry_set_text (GTK_ENTRY (odlg->idle_secs), s);

  g_snprintf (s, sizeof (s), "%d", config_no_project_timeout);
  gtk_entry_set_text (GTK_ENTRY (odlg->no_project_secs), s);

  /* Set the correct menu item based on current values */
  int hour;
  if (0 < config_daystart_offset)
    {
      hour = (config_daystart_offset + 1800) / 3600;
    }
  else
    {
      hour = (config_daystart_offset - 1800) / 3600;
    }
  if (-3 > hour)
    hour = -3; /* menu runs from 9pm */
  if (6 < hour)
    hour = 6; /* menu runs till 6am */
  hour += 3;  /* menu starts at 9PM */
  gtk_combo_box_set_active (odlg->daystart_menu, hour);

  /* Print the daystart offset as a string in 24 hour time */
  int secs = config_daystart_offset;
  if (0 > secs)
    secs += 24 * 3600;
  char buff[24];
  xxxqof_print_hours_elapsed_buff (buff, 24, secs, config_show_secs);
  gtk_entry_set_text (odlg->daystart_secs, buff);

  /* Set the correct menu item based on current values */
  int day = config_weekstart_offset;
  gtk_combo_box_set_active (odlg->weekstart_menu, day);

  switch (config_time_format)
    {
    case TIME_FORMAT_AM_PM:
      gtk_toggle_button_set_active (
          GTK_TOGGLE_BUTTON (odlg->time_format_am_pm), TRUE);
      gtk_toggle_button_set_active (
          GTK_TOGGLE_BUTTON (odlg->time_format_24_hs), FALSE);
      gtk_toggle_button_set_active (
          GTK_TOGGLE_BUTTON (odlg->time_format_locale), FALSE);
      break;
    case TIME_FORMAT_24_HS:
      gtk_toggle_button_set_active (
          GTK_TOGGLE_BUTTON (odlg->time_format_am_pm), FALSE);
      gtk_toggle_button_set_active (
          GTK_TOGGLE_BUTTON (odlg->time_format_24_hs), TRUE);
      gtk_toggle_button_set_active (
          GTK_TOGGLE_BUTTON (odlg->time_format_locale), FALSE);
      break;

    case TIME_FORMAT_LOCALE:
      gtk_toggle_button_set_active (
          GTK_TOGGLE_BUTTON (odlg->time_format_am_pm), FALSE);
      gtk_toggle_button_set_active (
          GTK_TOGGLE_BUTTON (odlg->time_format_24_hs), FALSE);
      gtk_toggle_button_set_active (
          GTK_TOGGLE_BUTTON (odlg->time_format_locale), TRUE);
      break;
    }

  g_snprintf (s, sizeof (s), "%s", config_currency_symbol);
  gtk_entry_set_text (GTK_ENTRY (odlg->currency_symbol), s);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (odlg->currency_use_locale),
                                config_currency_use_locale);

  /* set to unmodified as it reflects the current state of the app */
  gnome_property_box_set_modified (GNOME_PROPERTY_BOX (odlg->dlg), FALSE);
}

/* ============================================================== */

static void
daystart_menu_changed (gpointer data, GtkComboBox *w)
{
  PrefsDialog *dlg = data;

  int hour = gtk_combo_box_get_active (dlg->daystart_menu);

  g_return_if_fail (hour >= 0);

  hour += -3; /* menu starts at 9PM */

  int secs = hour * 3600;
  if (0 > secs)
    secs += 24 * 3600;
  char buff[24];
  xxxqof_print_hours_elapsed_buff (buff, 24, secs, config_show_secs);
  gtk_entry_set_text (dlg->daystart_secs, buff);
}

/* ============================================================== */

static void
display_options (PrefsDialog *dlg)
{
  GtkWidget *w;
  GladeXML *gtxml = dlg->gtxml;

  w = getchwid (gtxml, "show secs", dlg);
  dlg->show_secs = GTK_CHECK_BUTTON (w);

  w = getchwid (gtxml, "show statusbar", dlg);
  dlg->show_statusbar = GTK_CHECK_BUTTON (w);

  w = getchwid (gtxml, "show header", dlg);
  dlg->show_clist_titles = GTK_CHECK_BUTTON (w);

  w = getchwid (gtxml, "show sub", dlg);
  dlg->show_subprojects = GTK_CHECK_BUTTON (w);
}

static void
field_options (PrefsDialog *dlg)
{
  GladeXML *gtxml = dlg->gtxml;

  dlg->show_title_importance = dlgwid (gtxml, "show importance", dlg);
  dlg->show_title_urgency = dlgwid (gtxml, "show urgency", dlg);
  dlg->show_title_status = dlgwid (gtxml, "show status", dlg);
  dlg->show_title_ever = dlgwid (gtxml, "show ever", dlg);
  dlg->show_title_year = dlgwid (gtxml, "show year", dlg);
  dlg->show_title_month = dlgwid (gtxml, "show month", dlg);
  dlg->show_title_week = dlgwid (gtxml, "show week", dlg);
  dlg->show_title_lastweek = dlgwid (gtxml, "show lastweek", dlg);
  dlg->show_title_day = dlgwid (gtxml, "show day", dlg);
  dlg->show_title_yesterday = dlgwid (gtxml, "show yesterday", dlg);
  dlg->show_title_current = dlgwid (gtxml, "show current", dlg);
  dlg->show_title_desc = dlgwid (gtxml, "show desc", dlg);
  dlg->show_title_task = dlgwid (gtxml, "show task", dlg);
  dlg->show_title_estimated_start
      = dlgwid (gtxml, "show estimated_start", dlg);
  dlg->show_title_estimated_end = dlgwid (gtxml, "show estimated_end", dlg);
  dlg->show_title_due_date = dlgwid (gtxml, "show due_date", dlg);
  dlg->show_title_sizing = dlgwid (gtxml, "show sizing", dlg);
  dlg->show_title_percent_complete
      = dlgwid (gtxml, "show percent_complete", dlg);
}

static void
shell_command_options (PrefsDialog *dlg)
{
  GtkWidget *e;
  GladeXML *gtxml = dlg->gtxml;

  e = getwid (gtxml, "start project", dlg);
  dlg->shell_start = GTK_ENTRY (e);

  e = getwid (gtxml, "stop project", dlg);
  dlg->shell_stop = GTK_ENTRY (e);
}

static void
logfile_options (PrefsDialog *dlg)
{
  GtkWidget *w;
  GladeXML *gtxml = dlg->gtxml;

  w = getchwid (gtxml, "use logfile", dlg);
  dlg->logfileuse = GTK_CHECK_BUTTON (w);
  gtk_signal_connect (GTK_OBJECT (w), "clicked",
                      GTK_SIGNAL_FUNC (logfile_sensitive_cb), (gpointer *)dlg);

  w = glade_xml_get_widget (gtxml, "filename label");
  dlg->logfilename_l = w;

  w = glade_xml_get_widget (gtxml, "logfile path");
  dlg->logfilename = GTK_FILE_CHOOSER (w);
  gtk_signal_connect_object (GTK_OBJECT (dlg->logfilename), "file-set",
                             GTK_SIGNAL_FUNC (gnome_property_box_changed),
                             GTK_OBJECT (dlg->dlg));

  w = glade_xml_get_widget (gtxml, "fstart label");
  dlg->logfilestart_l = w;

  w = getwid (gtxml, "fstart", dlg);
  dlg->logfilestart = GTK_ENTRY (w);

  w = glade_xml_get_widget (gtxml, "fstop label");
  dlg->logfilestop_l = w;

  w = getwid (gtxml, "fstop", dlg);
  dlg->logfilestop = GTK_ENTRY (w);

  w = glade_xml_get_widget (gtxml, "fmin label");
  dlg->logfileminsecs_l = w;

  w = getwid (gtxml, "fmin", dlg);
  dlg->logfileminsecs = GTK_ENTRY (w);
}

static void
toolbar_options (PrefsDialog *dlg)
{
  GtkWidget *w;
  GladeXML *gtxml = dlg->gtxml;

  w = getchwid (gtxml, "show toolbar", dlg);
  dlg->show_toolbar = GTK_CHECK_BUTTON (w);

  gtk_signal_connect (GTK_OBJECT (w), "clicked",
                      GTK_SIGNAL_FUNC (toolbar_sensitive_cb), (gpointer *)dlg);

  dlg->show_tb_tips = tbwid (gtxml, "show tips", dlg);
  dlg->show_tb_new = tbwid (gtxml, "show new", dlg);
  dlg->show_tb_ccp = tbwid (gtxml, "show ccp", dlg);
  dlg->show_tb_journal = tbwid (gtxml, "show journal", dlg);
  dlg->show_tb_prop = tbwid (gtxml, "show prop", dlg);
  dlg->show_tb_timer = tbwid (gtxml, "show timer", dlg);
  dlg->show_tb_pref = tbwid (gtxml, "show pref", dlg);
  dlg->show_tb_help = tbwid (gtxml, "show help", dlg);
  dlg->show_tb_exit = tbwid (gtxml, "show exit", dlg);
}

static void
misc_options (PrefsDialog *dlg)
{
  GtkWidget *w;
  GladeXML *gtxml = dlg->gtxml;

  w = getwid (gtxml, "idle secs", dlg);
  dlg->idle_secs = GTK_ENTRY (w);

  GtkWidget *const vbox4 = glade_xml_get_widget (gtxml, "vbox4");

  GtkWidget *const frame9 = gtk_frame_new (_ ("No Project Timeout"));
  gtk_container_set_border_width (GTK_CONTAINER (frame9), 4);
  gtk_frame_set_label_align (GTK_FRAME (frame9), 0, 0.5);
  gtk_widget_set_name (frame9, "frame9");

  GtkWidget *const table7 = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table7), 8);
  gtk_table_set_row_spacings (GTK_TABLE (table7), 3);
  gtk_widget_set_name (table7, "table7");

  GtkWidget *const label19 = gtk_label_new (_ ("Idle Seconds:"));
  gtk_label_set_justify (GTK_LABEL (label19), GTK_JUSTIFY_CENTER);
  gtk_misc_set_alignment (GTK_MISC (label19), 0, 0.5);
  gtk_widget_set_name (label19, "label19");
  gtk_widget_show (label19);

  gtk_table_attach (GTK_TABLE (table7), label19, 0, 1, 0, 1, GTK_FILL, 0, 0,
                    0);

  GtkWidget *const no_project_secs = gtk_entry_new ();
  dlg->no_project_secs = GTK_ENTRY (no_project_secs);
  gtk_entry_set_invisible_char (GTK_ENTRY (no_project_secs), '*');
  gtk_widget_set_can_focus (no_project_secs, TRUE);
  gtk_widget_set_name (no_project_secs, "no project secs");
  gtk_widget_set_tooltip_text (
      no_project_secs,
      _ ("A warning will be displayed after this number of seconds with no "
         "running project.  Set to -1 to disable."));
  gtk_signal_connect_object (GTK_OBJECT (no_project_secs), "changed",
                             GTK_SIGNAL_FUNC (gnome_property_box_changed),
                             GTK_OBJECT (dlg->dlg));
  gtk_widget_show (no_project_secs);

  gtk_table_attach (GTK_TABLE (table7), no_project_secs, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  gtk_widget_show (table7);

  gtk_container_add (GTK_CONTAINER (frame9), table7);
  gtk_widget_show (frame9);

  gtk_box_pack_start_defaults (GTK_BOX (vbox4), frame9);

  GtkWidget *const frame8 = gtk_frame_new (_ ("End of Day/Week"));
  gtk_frame_set_label_align (GTK_FRAME (frame8), 0, 0.5);
  gtk_widget_set_name (frame8, "frame8");

  GtkWidget *const table6 = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table6), 8);
  gtk_table_set_row_spacings (GTK_TABLE (table6), 3);
  gtk_widget_set_name (table6, "table6");

  GtkWidget *const weekstart_combobox = gtk_combo_box_text_new ();
  dlg->weekstart_menu = GTK_COMBO_BOX (weekstart_combobox);
  gtk_widget_set_events (weekstart_combobox, GDK_BUTTON_PRESS_MASK
                                                 | GDK_BUTTON_RELEASE_MASK
                                                 | GDK_POINTER_MOTION_HINT_MASK
                                                 | GDK_POINTER_MOTION_MASK);
  gtk_widget_set_name (weekstart_combobox, "weekstart combobox");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (weekstart_combobox),
                                  _ ("Sunday"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (weekstart_combobox),
                                  _ ("Monday"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (weekstart_combobox),
                                  _ ("Tuesday"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (weekstart_combobox),
                                  _ ("Wednesday"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (weekstart_combobox),
                                  _ ("Thursday"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (weekstart_combobox),
                                  _ ("Friday"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (weekstart_combobox),
                                  _ ("Saturday"));
  gtk_signal_connect_object (GTK_OBJECT (weekstart_combobox), "changed",
                             GTK_SIGNAL_FUNC (gnome_property_box_changed),
                             GTK_OBJECT (dlg->dlg));
  gtk_widget_show (weekstart_combobox);

  gtk_table_attach (GTK_TABLE (table6), weekstart_combobox, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  GtkWidget *const label17 = gtk_label_new (_ ("New Day Starts At:"));
  gtk_misc_set_alignment (GTK_MISC (label17), 0, 0.5);
  gtk_widget_set_name (label17, "label17");
  gtk_widget_show (label17);

  gtk_table_attach (GTK_TABLE (table6), label17, 0, 1, 0, 1, GTK_FILL, 0, 0,
                    0);

  GtkWidget *const label18 = gtk_label_new (_ ("New Week Starts On:"));
  gtk_misc_set_alignment (GTK_MISC (label18), 0, 0.5);
  gtk_widget_set_name (label18, "label18");
  gtk_widget_show (label18);

  gtk_table_attach (GTK_TABLE (table6), label18, 0, 1, 1, 2, GTK_FILL, 0, 0,
                    0);

  GtkWidget *const hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox1, "hbox1");

  GtkWidget *const daystart_entry = gtk_entry_new ();
  dlg->daystart_secs = GTK_ENTRY (daystart_entry);
  gtk_entry_set_invisible_char (GTK_ENTRY (daystart_entry), '*');
  gtk_widget_set_can_focus (daystart_entry, TRUE);
  gtk_widget_set_name (daystart_entry, "daystart entry");
  gtk_widget_set_tooltip_text (
      daystart_entry,
      _ ("The time of night at which one day ends and the next day begins.  "
         "By default midnight, you can set this to any value."));
  gtk_signal_connect_object (GTK_OBJECT (daystart_entry), "changed",
                             GTK_SIGNAL_FUNC (gnome_property_box_changed),
                             GTK_OBJECT (dlg->dlg));
  gtk_widget_show (daystart_entry);

  gtk_box_pack_start_defaults (GTK_BOX (hbox1), daystart_entry);

  GtkWidget *const daystart_combobox = gtk_combo_box_text_new ();
  dlg->daystart_menu = GTK_COMBO_BOX (daystart_combobox);
  gtk_widget_set_events (daystart_combobox, GDK_BUTTON_PRESS_MASK
                                                | GDK_BUTTON_RELEASE_MASK
                                                | GDK_POINTER_MOTION_HINT_MASK
                                                | GDK_POINTER_MOTION_MASK);
  gtk_widget_set_name (daystart_combobox, "daystart combobox");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (daystart_combobox),
                                  _ ("9 PM"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (daystart_combobox),
                                  _ ("10 PM"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (daystart_combobox),
                                  _ ("11 PM"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (daystart_combobox),
                                  _ ("12 Midnight"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (daystart_combobox),
                                  _ ("01 AM"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (daystart_combobox),
                                  _ ("02 AM"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (daystart_combobox),
                                  _ ("03 AM"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (daystart_combobox),
                                  _ ("04 AM"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (daystart_combobox),
                                  _ ("05 AM"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (daystart_combobox),
                                  _ ("06 AM"));
  gtk_signal_connect_object (GTK_OBJECT (daystart_combobox), "changed",
                             GTK_SIGNAL_FUNC (daystart_menu_changed), dlg);
  gtk_signal_connect_object (GTK_OBJECT (daystart_combobox), "changed",
                             GTK_SIGNAL_FUNC (gnome_property_box_changed),
                             GTK_OBJECT (dlg->dlg));
  gtk_widget_show (daystart_combobox);

  gtk_box_pack_start_defaults (GTK_BOX (hbox1), daystart_combobox);
  gtk_widget_show (hbox1);

  gtk_table_attach (GTK_TABLE (table6), hbox1, 1, 2, 0, 1, GTK_FILL, GTK_FILL,
                    0, 0);
  gtk_widget_show (table6);

  gtk_container_add (GTK_CONTAINER (frame8), table6);
  gtk_widget_show (frame8);

  gtk_box_pack_start_defaults (GTK_BOX (vbox4), frame8);
}

static void
time_format_options (GtkWidget *const vbox, PrefsDialog *dlg)
{
  GtkWidget *frame10 = gtk_frame_new (_ ("Time format"));
  gtk_container_set_border_width (GTK_CONTAINER (frame10), 4);
  gtk_frame_set_label_align (GTK_FRAME (frame10), 0, 0.5);
  gtk_widget_set_name (frame10, "frame10");

  GtkWidget *const alignment1 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment1), 0, 0, 12, 0);
  gtk_widget_set_name (alignment1, "alignment1");

  GtkWidget *const vbox6 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox6, "vbox6");

  GtkWidget *const time_format_am_pm
      = gtk_radio_button_new_with_mnemonic (NULL, _ ("12 hours"));
  dlg->time_format_am_pm = GTK_RADIO_BUTTON (time_format_am_pm);
  gtk_button_set_use_underline (GTK_BUTTON (time_format_am_pm), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (time_format_am_pm), TRUE);
  gtk_widget_set_can_focus (time_format_am_pm, TRUE);
  gtk_widget_set_name (time_format_am_pm, "time_format_am_pm");
  gtk_signal_connect_object (GTK_OBJECT (time_format_am_pm), "toggled",
                             GTK_SIGNAL_FUNC (gnome_property_box_changed),
                             GTK_OBJECT (dlg->dlg));
  gtk_widget_show (time_format_am_pm);

  // FIXME: Find out why the button group mechanism is not working
  GSList *const time_format_btn_grp
      = gtk_radio_button_get_group (GTK_RADIO_BUTTON (time_format_am_pm));

  gtk_box_pack_start (GTK_BOX (vbox6), time_format_am_pm, FALSE, FALSE, 0);

  GtkWidget *const time_format_24_hs = gtk_radio_button_new_with_mnemonic (
      time_format_btn_grp, _ ("24 hours"));
  dlg->time_format_24_hs = GTK_RADIO_BUTTON (time_format_24_hs);
  gtk_button_set_use_underline (GTK_BUTTON (time_format_24_hs), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (time_format_24_hs), TRUE);
  gtk_widget_set_can_focus (time_format_24_hs, TRUE);
  gtk_widget_set_name (time_format_24_hs, "time_format_24_hs");
  gtk_signal_connect_object (GTK_OBJECT (time_format_24_hs), "toggled",
                             GTK_SIGNAL_FUNC (gnome_property_box_changed),
                             GTK_OBJECT (dlg->dlg));
  gtk_widget_show (time_format_24_hs);

  gtk_box_pack_start (GTK_BOX (vbox6), time_format_24_hs, FALSE, FALSE, 0);

  GtkWidget *const time_format_locale = gtk_radio_button_new_with_mnemonic (
      time_format_btn_grp, _ ("Use my locale formating"));
  dlg->time_format_locale = GTK_RADIO_BUTTON (time_format_locale);
  gtk_button_set_use_underline (GTK_BUTTON (time_format_locale), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (time_format_locale), TRUE);
  gtk_widget_set_can_focus (time_format_locale, TRUE);
  gtk_widget_set_name (time_format_locale, "time_format_locale");
  gtk_signal_connect_object (GTK_OBJECT (time_format_locale), "toggled",
                             GTK_SIGNAL_FUNC (gnome_property_box_changed),
                             GTK_OBJECT (dlg->dlg));
  gtk_widget_show (time_format_locale);

  gtk_box_pack_start (GTK_BOX (vbox6), time_format_locale, FALSE, FALSE, 0);
  gtk_widget_show (vbox6);

  gtk_container_add (GTK_CONTAINER (alignment1), vbox6);
  gtk_widget_show (alignment1);

  gtk_container_add (GTK_CONTAINER (frame10), alignment1);
  gtk_widget_show (frame10);

  gtk_box_pack_start_defaults (GTK_BOX (vbox), frame10);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (time_format_locale), TRUE);
}

static void
currency_options (GtkWidget *const vbox, PrefsDialog *dlg)
{
  GtkWidget *frame11 = gtk_frame_new (_ ("Currency settings"));
  gtk_container_set_border_width (GTK_CONTAINER (frame11), 4);
  gtk_frame_set_label_align (GTK_FRAME (frame11), 0, 0.5);
  gtk_widget_set_name (frame11, "frame11");

  GtkWidget *table8 = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table8), 8);
  gtk_table_set_row_spacings (GTK_TABLE (table8), 3);
  gtk_widget_set_name (table8, "table8");

  GtkWidget *currency_use_locale
      = gtk_check_button_new_with_label (_ ("Use my locale formating"));
  dlg->currency_use_locale = GTK_CHECK_BUTTON (currency_use_locale);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (currency_use_locale), TRUE);
  gtk_widget_set_can_focus (currency_use_locale, TRUE);
  gtk_widget_set_events (currency_use_locale,
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
                             | GDK_POINTER_MOTION_HINT_MASK
                             | GDK_POINTER_MOTION_MASK);
  gtk_widget_set_name (currency_use_locale, "currency_use_locale");
  gtk_signal_connect (GTK_OBJECT (currency_use_locale), "clicked",
                      GTK_SIGNAL_FUNC (currency_sensitive_cb),
                      (gpointer *)dlg);
  gtk_signal_connect_object (GTK_OBJECT (currency_use_locale), "toggled",
                             GTK_SIGNAL_FUNC (gnome_property_box_changed),
                             GTK_OBJECT (dlg->dlg));
  gtk_widget_show (currency_use_locale);

  gtk_table_attach (GTK_TABLE (table8), currency_use_locale, 0, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *currency_symbol_label = gtk_label_new (_ ("Currency Symbol"));
  dlg->currency_symbol_label = currency_symbol_label;
  gtk_label_set_justify (GTK_LABEL (currency_symbol_label),
                         GTK_JUSTIFY_CENTER);
  gtk_misc_set_alignment (GTK_MISC (currency_symbol_label), 0, 0.5);
  gtk_widget_set_name (currency_symbol_label, "currency_symbol_label");
  gtk_widget_show (currency_symbol_label);

  gtk_table_attach (GTK_TABLE (table8), currency_symbol_label, 0, 1, 1, 2,
                    GTK_FILL, 0, 0, 0);

  GtkWidget *currency_symbol = gtk_entry_new ();
  dlg->currency_symbol = GTK_ENTRY (currency_symbol);
  gtk_entry_set_invisible_char (GTK_ENTRY (currency_symbol), '*');
  gtk_entry_set_max_length (GTK_ENTRY (currency_symbol), 6);
  gtk_entry_set_text (GTK_ENTRY (currency_symbol), "$");
  gtk_widget_set_can_focus (currency_symbol, TRUE);
  gtk_widget_set_name (currency_symbol, "currency_symbol");
  gtk_widget_set_tooltip_text (
      currency_symbol,
      _ ("Place your local currency symbol for use in the reports"));
  gtk_signal_connect_object (GTK_OBJECT (currency_symbol), "changed",
                             GTK_SIGNAL_FUNC (gnome_property_box_changed),
                             GTK_OBJECT (dlg->dlg));
  gtk_widget_show (currency_symbol);

  gtk_table_attach (GTK_TABLE (table8), currency_symbol, 1, 2, 1, 2,
                    GTK_EXPAND, 0, 0, 0);

  gtk_widget_show (table8);

  gtk_container_add (GTK_CONTAINER (frame11), table8);
  gtk_widget_show (frame11);

  gtk_box_pack_start_defaults (GTK_BOX (vbox), frame11);
}

/* ============================================================== */

static void
help_cb (GnomePropertyBox *propertybox, gint page_num, gpointer data)
{
  gtt_help_popup (GTK_WIDGET (propertybox), data);
}

static PrefsDialog *
prefs_dialog_new (void)
{
  PrefsDialog *dlg;
  GladeXML *gtxml;

  dlg = g_malloc (sizeof (PrefsDialog));

  gtxml = gtt_glade_xml_new ("glade/prefs.glade", "Global Preferences");
  dlg->gtxml = gtxml;

  dlg->dlg = GNOME_PROPERTY_BOX (
      glade_xml_get_widget (gtxml, "Global Preferences"));

  gtk_signal_connect (GTK_OBJECT (dlg->dlg), "help", GTK_SIGNAL_FUNC (help_cb),
                      "preferences");

  gtk_signal_connect (GTK_OBJECT (dlg->dlg), "apply",
                      GTK_SIGNAL_FUNC (prefs_set), dlg);

  /* ------------------------------------------------------ */
  /* grab the various entry boxes and hook them up */
  display_options (dlg);
  field_options (dlg);
  shell_command_options (dlg);
  logfile_options (dlg);
  toolbar_options (dlg);
  misc_options (dlg);
  time_format_options (glade_xml_get_widget (gtxml, "vbox6"), dlg);
  currency_options (glade_xml_get_widget (gtxml, "vbox6"), dlg);

  gnome_dialog_close_hides (GNOME_DIALOG (dlg->dlg), TRUE);
  return dlg;
}

/* ============================================================== */

static PrefsDialog *dlog = NULL;

void
prefs_dialog_show (void)
{
  if (!dlog)
    dlog = prefs_dialog_new ();

  options_dialog_set (dlog);
  gtk_widget_show (GTK_WIDGET (dlog->dlg));
}

static GtkCheckButton *
dlgwid (GladeXML *const gtxml, const gchar *const strname,
        PrefsDialog *const dlg)
{
  return GTK_CHECK_BUTTON (getchwid (gtxml, strname, dlg));
}

static void
entry_to_char (GtkEntry *const e, gchar **const str)
{
  const gchar *s = gtk_entry_get_text (e);

  if (s[0])
    {
      if (NULL != *str)
        {
          g_free (*str);
        }
      *str = g_strdup (s);
    }
  else
    {
      if (NULL != *str)
        {
          g_free (*str);
        }
      *str = NULL;
    }
}

static GtkWidget *
getchwid (GladeXML *const gtxml, const gchar *const name,
          PrefsDialog *const dlg)
{
  GtkWidget *const wdgt = glade_xml_get_widget (gtxml, name);

  gtk_signal_connect_object (GTK_OBJECT (wdgt), "toggled",
                             GTK_SIGNAL_FUNC (gnome_property_box_changed),
                             GTK_OBJECT (dlg->dlg));

  return wdgt;
}

static GtkWidget *
getwid (GladeXML *const gtxml, const gchar *const name, PrefsDialog *const dlg)
{
  GtkWidget *const wdgt = glade_xml_get_widget (gtxml, name);

  gtk_signal_connect_object (GTK_OBJECT (wdgt), "changed",
                             GTK_SIGNAL_FUNC (gnome_property_box_changed),
                             GTK_OBJECT (dlg->dlg));

  return wdgt;
}

static void
set_active (GtkCheckButton *const chk_btn, const int option)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (chk_btn), option);
}

static void
set_val (int *const to, const int from, int *const change)
{
  if (*to != from)
    {
      *change = 1;
      *to = from;
    }
}

static void
show_check (GtkCheckButton *const chk_btn, int *const option,
            int *const change)
{
  const gboolean state
      = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (chk_btn));
  if (state != *option)
    {
      *change = 1;
      *option = state;
    }
}

static GtkCheckButton *
tbwid (GladeXML *const gtxml, const gchar *const strname,
       PrefsDialog *const dlg)
{
  return GTK_CHECK_BUTTON (getchwid (gtxml, strname, dlg));
}

/* ==================== END OF FILE ============================= */
