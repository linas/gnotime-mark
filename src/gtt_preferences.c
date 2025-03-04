/*   GUI dialog for global application preferences for GTimeTracker
 *   Copyright (C) 1997,98 Eckehard Berns
 *   Copyright (C) 2001 Linas Vepstas <linas@linas.org>
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

#include <qof.h>
#include <string.h>

#include "gtt.h"
#include "gtt_application_window.h"
#include "gtt_current_project.h"
#include "gtt_help_popup.h"
#include "gtt_preferences.h"
#include "gtt_property_box.h"
#include "gtt_timer.h"
#include "gtt_toolbar.h"
#include "gtt_util.h"

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
  GttPropertyBox *dlg;
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

void
entry_to_char (GtkEntry *const entry, gchar **const str)
{
  const gchar *const txt = gtk_entry_get_text (entry);

  if (NULL != *str)
    {
      g_free (*str);
    }

  if (txt[0])
    {
      *str = g_strdup (txt);
    }
  else
    {
      *str = NULL;
    }
}

void
show_check (GtkCheckButton *const btn, int *const val, int *const changed)
{
  const gint state = GTK_TOGGLE_BUTTON (btn)->active;

  if (*val != state)
    {
      *changed = 1;
      *val = state;
    }
}

void
set_val (int *const to, const int from, int *const changed)
{
  if (from != *to)
    {
      *changed = 1;
      *to = from;
    }
}

static void
prefs_set (GttPropertyBox *pb, gint page, PrefsDialog *odlg)
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

void
set_active (GtkCheckButton *const btn, const int val)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (btn), val);
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
  gtt_property_box_set_modified (GTT_PROPERTY_BOX (odlg->dlg), FALSE);
}

/* ============================================================== */

static void
daystart_menu_changed (GtkWidget *w, gpointer data)
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

GtkWidget *
getwid (GtkWidget *const widget, PrefsDialog *const dlg)
{
  gtk_signal_connect_object (GTK_OBJECT (widget), "changed",
                             GTK_SIGNAL_FUNC (gtt_property_box_changed),
                             GTK_OBJECT (dlg->dlg));

  return widget;
}

GtkWidget *
getchwid (GtkWidget *const widget, PrefsDialog *const dlg)
{
  gtk_signal_connect_object (GTK_OBJECT (widget), "toggled",
                             GTK_SIGNAL_FUNC (gtt_property_box_changed),
                             GTK_OBJECT (dlg->dlg));

  return widget;
}

static void
display_options (PrefsDialog *dlg)
{
  GtkWidget *const frame1 = gtk_frame_new (_ ("Project List Display"));
  gtk_container_set_border_width (GTK_CONTAINER (frame1), 4);
  gtk_frame_set_label_align (GTK_FRAME (frame1), 0, 0.5);
  gtk_widget_set_name (frame1, "frame1");

  GtkWidget *const table1 = gtk_table_new (4, 1, FALSE);
  gtk_widget_set_name (table1, "table1");

  GtkWidget *const show_secs
      = gtk_check_button_new_with_label (_ ("Show Seconds"));
  dlg->show_secs = GTK_CHECK_BUTTON (getchwid (show_secs, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_secs), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_secs), TRUE);
  gtk_widget_set_can_focus (show_secs, TRUE);
  gtk_widget_set_name (show_secs, "show secs");
  gtk_widget_show (show_secs);

  gtk_table_attach (GTK_TABLE (table1), show_secs, 0, 1, 0, 1, GTK_FILL, 0, 0,
                    0);

  GtkWidget *const show_statusbar
      = gtk_check_button_new_with_label (_ ("Show Status Bar"));
  dlg->show_statusbar = GTK_CHECK_BUTTON (getchwid (show_statusbar, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_statusbar), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_statusbar), TRUE);
  gtk_widget_set_can_focus (show_statusbar, TRUE);
  gtk_widget_set_name (show_statusbar, "show statusbar");
  gtk_widget_show (show_statusbar);

  gtk_table_attach (GTK_TABLE (table1), show_statusbar, 0, 1, 1, 2, GTK_FILL,
                    0, 0, 0);

  GtkWidget *const show_header
      = gtk_check_button_new_with_label (_ ("Show Table Header"));
  dlg->show_clist_titles = GTK_CHECK_BUTTON (getchwid (show_header, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_header), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_header), TRUE);
  gtk_widget_set_can_focus (show_header, TRUE);
  gtk_widget_set_name (show_header, "show header");
  gtk_widget_show (show_header);

  gtk_table_attach (GTK_TABLE (table1), show_header, 0, 1, 2, 3, GTK_FILL, 0,
                    0, 0);

  GtkWidget *const show_sub
      = gtk_check_button_new_with_label (_ ("Show Sub-Projects"));
  dlg->show_subprojects = GTK_CHECK_BUTTON (getchwid (show_sub, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_sub), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_sub), TRUE);
  gtk_widget_set_can_focus (show_sub, TRUE);
  gtk_widget_set_name (show_sub, "show sub");
  gtk_widget_show (show_sub);

  gtk_table_attach (GTK_TABLE (table1), show_sub, 0, 1, 3, 4, GTK_FILL, 0, 0,
                    0);

  gtk_widget_show (table1);

  gtk_container_add (GTK_CONTAINER (frame1), table1);
  gtk_widget_show (frame1);

  GtkWidget *const label1 = gtk_label_new (_ ("Display"));
  gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_CENTER);
  gtk_widget_set_name (label1, "label1");
  gtk_widget_show (label1);

  gtt_property_box_append_page (dlg->dlg, frame1, label1);
}

static void
field_options (PrefsDialog *dlg)
{
  GtkWidget *const frame7 = gtk_frame_new (_ ("View Project Fields"));
  gtk_container_set_border_width (GTK_CONTAINER (frame7), 4);
  gtk_frame_set_label_align (GTK_FRAME (frame7), 0, 0.5);
  gtk_widget_set_name (frame7, "frame7");

  GtkWidget *const table5 = gtk_table_new (18, 1, FALSE);
  gtk_widget_set_name (table5, "table5");

  GtkWidget *const show_importance
      = gtk_check_button_new_with_label (_ ("Show Project Importance"));
  dlg->show_title_importance
      = GTK_CHECK_BUTTON (getchwid (show_importance, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_importance), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_importance), TRUE);
  gtk_widget_set_can_focus (show_importance, TRUE);
  gtk_widget_set_name (show_importance, "show importance");
  gtk_widget_show (show_importance);

  gtk_table_attach (GTK_TABLE (table5), show_importance, 0, 1, 0, 1, GTK_FILL,
                    0, 0, 0);

  GtkWidget *const show_urgency
      = gtk_check_button_new_with_label (_ ("Show Project Urgency"));
  dlg->show_title_urgency = GTK_CHECK_BUTTON (getchwid (show_urgency, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_urgency), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_urgency), TRUE);
  gtk_widget_set_can_focus (show_urgency, TRUE);
  gtk_widget_set_name (show_urgency, "show urgency");
  gtk_widget_show (show_urgency);

  gtk_table_attach (GTK_TABLE (table5), show_urgency, 0, 1, 1, 2, GTK_FILL, 0,
                    0, 0);

  GtkWidget *const show_status
      = gtk_check_button_new_with_label (_ ("Show Project Status"));
  dlg->show_title_status = GTK_CHECK_BUTTON (getchwid (show_status, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_status), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_status), TRUE);
  gtk_widget_set_can_focus (show_status, TRUE);
  gtk_widget_set_name (show_status, "show status");
  gtk_widget_show (show_status);

  gtk_table_attach (GTK_TABLE (table5), show_status, 0, 1, 2, 3, GTK_FILL, 0,
                    0, 0);

  GtkWidget *const show_ever
      = gtk_check_button_new_with_label (_ ("Show Total Time Ever"));
  dlg->show_title_ever = GTK_CHECK_BUTTON (getchwid (show_ever, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_ever), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_ever), TRUE);
  gtk_widget_set_can_focus (show_ever, TRUE);
  gtk_widget_set_name (show_ever, "show ever");
  gtk_widget_show (show_ever);

  gtk_table_attach (GTK_TABLE (table5), show_ever, 0, 1, 3, 4, GTK_FILL, 0, 0,
                    0);

  GtkWidget *const show_year
      = gtk_check_button_new_with_label (_ ("Show Time This Year"));
  dlg->show_title_year = GTK_CHECK_BUTTON (getchwid (show_year, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_year), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_year), TRUE);
  gtk_widget_set_can_focus (show_year, TRUE);
  gtk_widget_set_name (show_year, "show year");
  gtk_widget_show (show_year);

  gtk_table_attach (GTK_TABLE (table5), show_year, 0, 1, 4, 5, GTK_FILL, 0, 0,
                    0);

  GtkWidget *const show_month
      = gtk_check_button_new_with_label (_ ("Show Time This Month"));
  dlg->show_title_month = GTK_CHECK_BUTTON (getchwid (show_month, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_month), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_month), TRUE);
  gtk_widget_set_can_focus (show_month, TRUE);
  gtk_widget_set_name (show_month, "show month");
  gtk_widget_show (show_month);

  gtk_table_attach (GTK_TABLE (table5), show_month, 0, 1, 5, 6, GTK_FILL, 0, 0,
                    0);

  GtkWidget *const show_week
      = gtk_check_button_new_with_label (_ ("Show Time This Week"));
  dlg->show_title_week = GTK_CHECK_BUTTON (getchwid (show_week, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_week), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_week), TRUE);
  gtk_widget_set_can_focus (show_week, TRUE);
  gtk_widget_set_name (show_week, "show week");
  gtk_widget_show (show_week);

  gtk_table_attach (GTK_TABLE (table5), show_week, 0, 1, 6, 7, GTK_FILL, 0, 0,
                    0);

  GtkWidget *const show_lastweek
      = gtk_check_button_new_with_label (_ ("Show Time Last Week"));
  dlg->show_title_lastweek = GTK_CHECK_BUTTON (getchwid (show_lastweek, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_lastweek), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_lastweek), TRUE);
  gtk_widget_set_can_focus (show_lastweek, TRUE);
  gtk_widget_set_name (show_lastweek, "show lastweek");
  gtk_widget_show (show_lastweek);

  gtk_table_attach (GTK_TABLE (table5), show_lastweek, 0, 1, 7, 8, GTK_FILL, 0,
                    0, 0);

  GtkWidget *const show_day
      = gtk_check_button_new_with_label (_ ("Show Time Today"));
  dlg->show_title_day = GTK_CHECK_BUTTON (getchwid (show_day, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_day), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_day), TRUE);
  gtk_widget_set_can_focus (show_day, TRUE);
  gtk_widget_set_name (show_day, "show day");
  gtk_widget_show (show_day);

  gtk_table_attach (GTK_TABLE (table5), show_day, 0, 1, 8, 9, GTK_FILL, 0, 0,
                    0);

  GtkWidget *const show_yesterday
      = gtk_check_button_new_with_label (_ ("Show Time Yesterday"));
  dlg->show_title_yesterday
      = GTK_CHECK_BUTTON (getchwid (show_yesterday, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_yesterday), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_yesterday), TRUE);
  gtk_widget_set_can_focus (show_yesterday, TRUE);
  gtk_widget_set_name (show_yesterday, "show yesterday");
  gtk_widget_show (show_yesterday);

  gtk_table_attach (GTK_TABLE (table5), show_yesterday, 0, 1, 9, 10, GTK_FILL,
                    0, 0, 0);

  GtkWidget *const show_current = gtk_check_button_new_with_label (
      _ ("Show Time For The Current Diary Entry"));
  dlg->show_title_current = GTK_CHECK_BUTTON (getchwid (show_current, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_current), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_current), TRUE);
  gtk_widget_set_can_focus (show_current, TRUE);
  gtk_widget_set_name (show_current, "show current");
  gtk_widget_show (show_current);

  gtk_table_attach (GTK_TABLE (table5), show_current, 0, 1, 10, 11, GTK_FILL,
                    0, 0, 0);

  GtkWidget *const show_desc
      = gtk_check_button_new_with_label (_ ("Show Project Description"));
  dlg->show_title_desc = GTK_CHECK_BUTTON (getchwid (show_desc, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_desc), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_desc), TRUE);
  gtk_widget_set_can_focus (show_desc, TRUE);
  gtk_widget_set_name (show_desc, "show desc");
  gtk_widget_show (show_desc);

  gtk_table_attach (GTK_TABLE (table5), show_desc, 0, 1, 11, 12, GTK_FILL, 0,
                    0, 0);

  GtkWidget *const show_task
      = gtk_check_button_new_with_label (_ ("Show Current Diary Entry"));
  dlg->show_title_task = GTK_CHECK_BUTTON (getchwid (show_task, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_task), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_task), TRUE);
  gtk_widget_set_can_focus (show_task, TRUE);
  gtk_widget_set_name (show_task, "show task");
  gtk_widget_show (show_task);

  gtk_table_attach (GTK_TABLE (table5), show_task, 0, 1, 12, 13, GTK_FILL, 0,
                    0, 0);

  GtkWidget *const show_estimated_start = gtk_check_button_new_with_label (
      _ ("Show Planned Project Start Date"));
  dlg->show_title_estimated_start
      = GTK_CHECK_BUTTON (getchwid (show_estimated_start, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_estimated_start), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_estimated_start), TRUE);
  gtk_widget_set_can_focus (show_estimated_start, TRUE);
  gtk_widget_set_name (show_estimated_start, "show estimated_start");
  gtk_widget_show (show_estimated_start);

  gtk_table_attach (GTK_TABLE (table5), show_estimated_start, 0, 1, 13, 14,
                    GTK_FILL, 0, 0, 0);

  GtkWidget *const show_estimated_end
      = gtk_check_button_new_with_label (_ ("Show Planned Project End Date"));
  dlg->show_title_estimated_end
      = GTK_CHECK_BUTTON (getchwid (show_estimated_end, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_estimated_end), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_estimated_end), TRUE);
  gtk_widget_set_can_focus (show_estimated_end, TRUE);
  gtk_widget_set_name (show_estimated_end, "show estimated_end");
  gtk_widget_show (show_estimated_end);

  gtk_table_attach (GTK_TABLE (table5), show_estimated_end, 0, 1, 14, 15,
                    GTK_FILL, 0, 0, 0);

  GtkWidget *const show_due_date
      = gtk_check_button_new_with_label (_ ("Show Project Due Date"));
  dlg->show_title_due_date = GTK_CHECK_BUTTON (getchwid (show_due_date, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_due_date), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_due_date), TRUE);
  gtk_widget_set_can_focus (show_due_date, TRUE);
  gtk_widget_set_name (show_due_date, "show due_date");
  gtk_widget_show (show_due_date);

  gtk_table_attach (GTK_TABLE (table5), show_due_date, 0, 1, 15, 16, GTK_FILL,
                    0, 0, 0);

  GtkWidget *const show_sizing
      = gtk_check_button_new_with_label (_ ("Show Estimated Effort"));
  dlg->show_title_sizing = GTK_CHECK_BUTTON (getchwid (show_sizing, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_sizing), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_sizing), TRUE);
  gtk_widget_set_can_focus (show_sizing, TRUE);
  gtk_widget_set_name (show_sizing, "show sizing");
  gtk_widget_set_tooltip_text (
      show_sizing, _ ("Show the 'szing', that is, the estimated amount of "
                      "work that it will take to perform this project."));
  gtk_widget_show (show_sizing);

  gtk_table_attach (GTK_TABLE (table5), show_sizing, 0, 1, 16, 17, GTK_FILL, 0,
                    0, 0);

  GtkWidget *const show_percent_complete
      = gtk_check_button_new_with_label (_ ("Show Percent Complete"));
  dlg->show_title_percent_complete
      = GTK_CHECK_BUTTON (getchwid (show_percent_complete, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (show_percent_complete), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_percent_complete), TRUE);
  gtk_widget_set_can_focus (show_percent_complete, TRUE);
  gtk_widget_set_name (show_percent_complete, "show percent_complete");
  gtk_widget_show (show_percent_complete);

  gtk_table_attach (GTK_TABLE (table5), show_percent_complete, 0, 1, 17, 18,
                    GTK_FILL, 0, 0, 0);

  gtk_widget_show (table5);

  gtk_container_add (GTK_CONTAINER (frame7), table5);
  gtk_widget_show (frame7);

  GtkWidget *const label9 = gtk_label_new (_ ("Fields"));
  gtk_label_set_justify (GTK_LABEL (label9), GTK_JUSTIFY_CENTER);
  gtk_widget_set_name (label9, "label9");
  gtk_widget_show (label9);

  gtt_property_box_append_page (dlg->dlg, frame7, label9);
}

static void
shell_command_options (PrefsDialog *dlg)
{
  GtkWidget *const frame2 = gtk_frame_new (_ ("Shell Commands"));
  gtk_container_set_border_width (GTK_CONTAINER (frame2), 4);
  gtk_frame_set_label_align (GTK_FRAME (frame2), 0, 0.5);
  gtk_widget_set_name (frame2, "frame2");

  GtkWidget *const table2 = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table2), 8);
  gtk_table_set_row_spacings (GTK_TABLE (table2), 3);
  gtk_widget_set_name (table2, "table2");

  GtkWidget *const label6 = gtk_label_new (_ ("Start Project Command:"));
  gtk_label_set_justify (GTK_LABEL (label6), GTK_JUSTIFY_CENTER);
  gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);
  gtk_widget_set_name (label6, "label6");
  gtk_widget_show (label6);

  gtk_table_attach (GTK_TABLE (table2), label6, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

  GtkWidget *const start_project = gtk_entry_new ();
  dlg->shell_start = GTK_ENTRY (getwid (start_project, dlg));
  gtk_entry_set_text (GTK_ENTRY (start_project),
                      _ ("echo shell start id=%D \"%t\" xx\"%d\" %T  "
                         "%Hxx%Mxx%S hours=%h min=%m secs=%s"));
  gtk_widget_set_can_focus (start_project, TRUE);
  gtk_widget_set_events (start_project, GDK_BUTTON_PRESS_MASK
                                            | GDK_BUTTON_RELEASE_MASK
                                            | GDK_POINTER_MOTION_HINT_MASK
                                            | GDK_POINTER_MOTION_MASK);
  gtk_widget_set_name (start_project, "start project");
  gtk_widget_set_tooltip_text (start_project,
                               _ ("Enter a shell command to be executed "
                                  "whenever projects are switched."));
  gtk_widget_show (start_project);

  gtk_table_attach (GTK_TABLE (table2), start_project, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  GtkWidget *const label7 = gtk_label_new (_ ("Stop Project Command:"));
  gtk_label_set_justify (GTK_LABEL (label7), GTK_JUSTIFY_CENTER);
  gtk_misc_set_alignment (GTK_MISC (label7), 0, 0.5);
  gtk_widget_set_name (label7, "label7");
  gtk_widget_show (label7);

  gtk_table_attach (GTK_TABLE (table2), label7, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);

  GtkWidget *const stop_project = gtk_entry_new ();
  dlg->shell_stop = GTK_ENTRY (getwid (stop_project, dlg));
  gtk_entry_set_text (GTK_ENTRY (stop_project),
                      _ ("echo shell stop id=%D \"%t\" xx\"%d\" %T  "
                         "%Hxx%Mxx%S hours=%h min=%m secs=%s"));
  gtk_widget_set_can_focus (stop_project, TRUE);
  gtk_widget_set_events (stop_project, GDK_BUTTON_PRESS_MASK
                                           | GDK_BUTTON_RELEASE_MASK
                                           | GDK_POINTER_MOTION_HINT_MASK
                                           | GDK_POINTER_MOTION_MASK);
  gtk_widget_set_name (stop_project, "stop project");
  gtk_widget_set_tooltip_text (
      stop_project,
      _ ("Enter a shell command to be executed when no projects are active."));
  gtk_widget_show (stop_project);

  gtk_table_attach (GTK_TABLE (table2), stop_project, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  gtk_widget_show (table2);

  gtk_container_add (GTK_CONTAINER (frame2), table2);
  gtk_widget_show (frame2);

  GtkWidget *const label2 = gtk_label_new (_ ("Shell"));
  gtk_label_set_justify (GTK_LABEL (label2), GTK_JUSTIFY_CENTER);
  gtk_widget_set_name (label2, "label2");
  gtk_widget_show (label2);

  gtt_property_box_append_page (dlg->dlg, frame2, label2);
}

static void
logfile_options (PrefsDialog *dlg)
{
  GtkWidget *const frame5 = gtk_frame_new (_ ("Logfile"));
  gtk_container_set_border_width (GTK_CONTAINER (frame5), 4);
  gtk_frame_set_label_align (GTK_FRAME (frame5), 0, 0.5);
  gtk_widget_set_name (frame5, "frame5");

  GtkWidget *const table4 = gtk_table_new (5, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table4), 8);
  gtk_table_set_row_spacings (GTK_TABLE (table4), 3);
  gtk_widget_set_name (table4, "table4");

  GtkWidget *const use_logfile
      = gtk_check_button_new_with_label (_ ("Use Logfile"));
  dlg->logfileuse = GTK_CHECK_BUTTON (getchwid (use_logfile, dlg));
  gtk_button_set_use_underline (GTK_BUTTON (use_logfile), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (use_logfile), TRUE);
  gtk_widget_set_can_focus (use_logfile, TRUE);
  gtk_widget_set_name (use_logfile, "use logfile");
  gtk_signal_connect (GTK_OBJECT (use_logfile), "clicked",
                      GTK_SIGNAL_FUNC (logfile_sensitive_cb), (gpointer *)dlg);
  gtk_widget_show (use_logfile);

  gtk_table_attach (GTK_TABLE (table4), use_logfile, 0, 1, 0, 1, GTK_FILL, 0,
                    0, 0);

  GtkWidget *const filename_label = gtk_label_new (_ ("Filename:"));
  dlg->logfilename_l = filename_label;
  gtk_label_set_justify (GTK_LABEL (filename_label), GTK_JUSTIFY_CENTER);
  gtk_misc_set_alignment (GTK_MISC (filename_label), 0, 0.5);
  gtk_widget_set_name (filename_label, "filename label");
  gtk_widget_show (filename_label);

  gtk_table_attach (GTK_TABLE (table4), filename_label, 0, 1, 1, 2, GTK_FILL,
                    0, 0, 0);

  GtkWidget *const logfile_path = gtk_file_chooser_button_new (
      _ ("Select a File"), GTK_FILE_CHOOSER_ACTION_OPEN);
  dlg->logfilename = GTK_FILE_CHOOSER (logfile_path);
  gtk_widget_set_events (logfile_path, GDK_BUTTON_PRESS_MASK
                                           | GDK_BUTTON_RELEASE_MASK
                                           | GDK_POINTER_MOTION_HINT_MASK
                                           | GDK_POINTER_MOTION_MASK);
  gtk_widget_set_name (logfile_path, "logfile path");
  gtk_signal_connect_object (GTK_OBJECT (logfile_path), "file-set",
                             GTK_SIGNAL_FUNC (gtt_property_box_changed),
                             GTK_OBJECT (dlg->dlg));
  gtk_widget_show (logfile_path);

  gtk_table_attach (GTK_TABLE (table4), logfile_path, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  GtkWidget *const fstart_label = gtk_label_new (_ ("Entry Start:"));
  dlg->logfilestart_l = fstart_label;
  gtk_label_set_justify (GTK_LABEL (fstart_label), GTK_JUSTIFY_CENTER);
  gtk_misc_set_alignment (GTK_MISC (fstart_label), 0, 0.5);
  gtk_widget_set_name (fstart_label, "fstart label");
  gtk_widget_show (fstart_label);

  gtk_table_attach (GTK_TABLE (table4), fstart_label, 0, 1, 2, 3, GTK_FILL, 0,
                    0, 0);

  GtkWidget *const fstart = gtk_entry_new ();
  dlg->logfilestart = GTK_ENTRY (getwid (fstart, dlg));
  gtk_widget_set_can_focus (fstart, TRUE);
  gtk_widget_set_events (
      fstart, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
                  | GDK_POINTER_MOTION_HINT_MASK | GDK_POINTER_MOTION_MASK);
  gtk_widget_set_name (fstart, "fstart");
  gtk_widget_set_tooltip_text (
      fstart, _ ("Entry that will be logged when a project starts. Use %t for "
                 "the project title, %d for the description, etc. See the "
                 "manual for more options."));
  gtk_widget_show (fstart);

  gtk_table_attach (GTK_TABLE (table4), fstart, 1, 2, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  GtkWidget *const fstop_label = gtk_label_new (_ ("Entry Stop:"));
  dlg->logfilestop_l = fstop_label;
  gtk_label_set_justify (GTK_LABEL (fstop_label), GTK_JUSTIFY_CENTER);
  gtk_misc_set_alignment (GTK_MISC (fstop_label), 0, 0.5);
  gtk_widget_set_name (fstop_label, "fstop label");
  gtk_widget_show (fstop_label);

  gtk_table_attach (GTK_TABLE (table4), fstop_label, 0, 1, 3, 4, GTK_FILL, 0,
                    0, 0);

  GtkWidget *const fstop = gtk_entry_new ();
  dlg->logfilestop = GTK_ENTRY (getwid (fstop, dlg));
  gtk_widget_set_can_focus (fstop, TRUE);
  gtk_widget_set_events (fstop, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
                                    | GDK_POINTER_MOTION_HINT_MASK
                                    | GDK_POINTER_MOTION_MASK);
  gtk_widget_set_name (fstop, "fstop");
  gtk_widget_set_tooltip_text (
      fstop, _ ("Entry that will be logged when the project stops.  Use %t "
                "for the porject title, %d for the project description, etc. "
                "See the manual for more options."));
  gtk_widget_show (fstop);

  gtk_table_attach (GTK_TABLE (table4), fstop, 1, 2, 3, 4,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  GtkWidget *const fmin_label = gtk_label_new (_ ("Min Recorded:"));
  dlg->logfileminsecs_l = fmin_label;
  gtk_label_set_justify (GTK_LABEL (fmin_label), GTK_JUSTIFY_CENTER);
  gtk_misc_set_alignment (GTK_MISC (fmin_label), 0, 0.5);
  gtk_widget_set_name (fmin_label, "fmin label");
  gtk_widget_show (fmin_label);

  gtk_table_attach (GTK_TABLE (table4), fmin_label, 0, 1, 4, 5, GTK_FILL, 0, 0,
                    0);

  GtkWidget *const fmin = gtk_entry_new ();
  dlg->logfileminsecs = GTK_ENTRY (getwid (fmin, dlg));
  gtk_widget_set_can_focus (fmin, TRUE);
  gtk_widget_set_events (fmin, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
                                   | GDK_POINTER_MOTION_HINT_MASK
                                   | GDK_POINTER_MOTION_MASK);
  gtk_widget_set_name (fmin, "fmin");
  gtk_widget_set_tooltip_text (
      fmin, _ ("Switches between projects that happen faster than this will "
               "not be logged (enter the number of seconds)"));
  gtk_widget_show (fmin);

  gtk_table_attach (GTK_TABLE (table4), fmin, 1, 2, 4, 5,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  gtk_widget_show (table4);

  gtk_container_add (GTK_CONTAINER (frame5), table4);
  gtk_widget_show (frame5);

  GtkWidget *const label3 = gtk_label_new (_ ("Logfile"));
  gtk_label_set_justify (GTK_LABEL (label3), GTK_JUSTIFY_CENTER);
  gtk_widget_set_name (label3, "label3");
  gtk_widget_show (label3);

  gtt_property_box_append_page (dlg->dlg, frame5, label3);
}

static void
toolbar_options (PrefsDialog *dlg)
{
  GtkWidget *const vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox1, "vbox1");

  GtkWidget *const frame3 = gtk_frame_new (_ ("Toolbar"));
  gtk_container_set_border_width (GTK_CONTAINER (frame3), 4);
  gtk_frame_set_label_align (GTK_FRAME (frame3), 0, 0.5);
  gtk_widget_set_name (frame3, "frame3");

  GtkWidget *const vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox2, "vbox2");

  GtkWidget *const show_toolbar
      = gtk_check_button_new_with_mnemonic (_ ("Show Toolbar"));
  dlg->show_toolbar = GTK_CHECK_BUTTON (getchwid (show_toolbar, dlg));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (show_toolbar), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_toolbar), TRUE);
  gtk_widget_set_can_focus (show_toolbar, TRUE);
  gtk_widget_set_name (show_toolbar, "show toolbar");
  gtk_signal_connect (GTK_OBJECT (show_toolbar), "clicked",
                      GTK_SIGNAL_FUNC (toolbar_sensitive_cb), (gpointer *)dlg);
  gtk_widget_show (show_toolbar);

  gtk_box_pack_start (GTK_BOX (vbox2), show_toolbar, FALSE, FALSE, 0);

  GtkWidget *const show_tips
      = gtk_check_button_new_with_mnemonic (_ ("Show Tooltips"));
  dlg->show_tb_tips = GTK_CHECK_BUTTON (getchwid (show_tips, dlg));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (show_tips), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_tips), TRUE);
  gtk_widget_set_can_focus (show_tips, TRUE);
  gtk_widget_set_name (show_tips, "show tips");
  gtk_widget_show (show_tips);

  gtk_box_pack_start (GTK_BOX (vbox2), show_tips, FALSE, FALSE, 0);

  gtk_widget_show (vbox2);

  gtk_container_add (GTK_CONTAINER (frame3), vbox2);
  gtk_widget_show (frame3);

  gtk_box_pack_start (GTK_BOX (vbox1), frame3, TRUE, TRUE, 0);

  GtkWidget *const frame4 = gtk_frame_new (_ ("Toolbar Segments"));
  gtk_container_set_border_width (GTK_CONTAINER (frame4), 4);
  gtk_frame_set_label_align (GTK_FRAME (frame4), 0, 0.5);
  gtk_widget_set_name (frame4, "frame4");

  GtkWidget *const vbox3 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox3, "vbox3");

  GtkWidget *const show_new
      = gtk_check_button_new_with_mnemonic (_ ("Show `New'"));
  dlg->show_tb_new = GTK_CHECK_BUTTON (getchwid (show_new, dlg));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (show_new), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_new), TRUE);
  gtk_widget_set_can_focus (show_new, TRUE);
  gtk_widget_set_name (show_new, "show new");
  gtk_widget_show (show_new);

  gtk_box_pack_start (GTK_BOX (vbox3), show_new, FALSE, FALSE, 0);

  GtkWidget *const show_ccp
      = gtk_check_button_new_with_mnemonic (_ ("Show `Cut', `Copy', `Paste'"));
  dlg->show_tb_ccp = GTK_CHECK_BUTTON (getchwid (show_ccp, dlg));
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_ccp), TRUE);
  gtk_widget_set_can_focus (show_ccp, TRUE);
  gtk_widget_set_name (show_ccp, "show ccp");
  gtk_widget_show (show_ccp);

  gtk_box_pack_start (GTK_BOX (vbox3), show_ccp, FALSE, FALSE, 0);

  GtkWidget *const show_journal
      = gtk_check_button_new_with_mnemonic (_ ("Show `Journal'"));
  dlg->show_tb_journal = GTK_CHECK_BUTTON (getchwid (show_journal, dlg));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (show_journal), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_journal), TRUE);
  gtk_widget_set_can_focus (show_journal, TRUE);
  gtk_widget_set_name (show_journal, "show journal");
  gtk_widget_show (show_journal);

  gtk_box_pack_start (GTK_BOX (vbox3), show_journal, FALSE, FALSE, 0);

  GtkWidget *const show_prop
      = gtk_check_button_new_with_mnemonic (_ ("Show `Properties'"));
  dlg->show_tb_prop = GTK_CHECK_BUTTON (getchwid (show_prop, dlg));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (show_prop), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_prop), TRUE);
  gtk_widget_set_can_focus (show_prop, TRUE);
  gtk_widget_set_name (show_prop, "show prop");
  gtk_widget_show (show_prop);

  gtk_box_pack_start (GTK_BOX (vbox3), show_prop, FALSE, FALSE, 0);

  GtkWidget *const show_timer
      = gtk_check_button_new_with_mnemonic (_ ("Show `Timer'"));
  dlg->show_tb_timer = GTK_CHECK_BUTTON (getchwid (show_timer, dlg));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (show_timer), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_timer), TRUE);
  gtk_widget_set_can_focus (show_timer, TRUE);
  gtk_widget_set_name (show_timer, "show timer");
  gtk_widget_show (show_timer);

  gtk_box_pack_start (GTK_BOX (vbox3), show_timer, FALSE, FALSE, 0);

  GtkWidget *const show_pref
      = gtk_check_button_new_with_mnemonic (_ ("Show `Preferences'"));
  dlg->show_tb_pref = GTK_CHECK_BUTTON (getchwid (show_pref, dlg));
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_pref), TRUE);
  gtk_widget_set_can_focus (show_pref, TRUE);
  gtk_widget_set_name (show_pref, "show pref");
  gtk_widget_show (show_pref);

  gtk_box_pack_start (GTK_BOX (vbox3), show_pref, FALSE, FALSE, 0);

  GtkWidget *const show_help
      = gtk_check_button_new_with_mnemonic (_ ("Show `Help'"));
  dlg->show_tb_help = GTK_CHECK_BUTTON (getchwid (show_help, dlg));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (show_help), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_help), TRUE);
  gtk_widget_set_can_focus (show_help, TRUE);
  gtk_widget_set_name (show_help, "show help");
  gtk_widget_show (show_help);

  gtk_box_pack_start (GTK_BOX (vbox3), show_help, FALSE, FALSE, 0);

  GtkWidget *const show_exit
      = gtk_check_button_new_with_mnemonic (_ ("Show `Quit'"));
  dlg->show_tb_exit = GTK_CHECK_BUTTON (getchwid (show_exit, dlg));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (show_exit), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (show_exit), TRUE);
  gtk_widget_set_can_focus (show_exit, TRUE);
  gtk_widget_set_name (show_exit, "show exit");
  gtk_widget_show (show_exit);

  gtk_box_pack_start (GTK_BOX (vbox3), show_exit, FALSE, FALSE, 0);

  gtk_widget_show (vbox3);

  gtk_container_add (GTK_CONTAINER (frame4), vbox3);
  gtk_widget_show (frame4);

  gtk_box_pack_start (GTK_BOX (vbox1), frame4, TRUE, TRUE, 0);

  gtk_widget_show (vbox1);

  GtkWidget *const label4 = gtk_label_new (_ ("Toolbar"));
  gtk_label_set_justify (GTK_LABEL (label4), GTK_JUSTIFY_CENTER);
  gtk_widget_set_name (label4, "label4");
  gtk_widget_show (label4);

  gtt_property_box_append_page (dlg->dlg, vbox1, label4);
}

static void
misc_options (PrefsDialog *dlg)
{
  GtkWidget *const vbox4 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox4, "vbox4");

  GtkWidget *const frame6 = gtk_frame_new (_ ("Inactivity Timeout"));
  gtk_container_set_border_width (GTK_CONTAINER (frame6), 4);
  gtk_frame_set_label_align (GTK_FRAME (frame6), 0, 0.5);
  gtk_widget_set_name (frame6, "frame6");

  GtkWidget *const table3 = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table3), 8);
  gtk_table_set_row_spacings (GTK_TABLE (table3), 3);
  gtk_widget_set_name (table3, "table3");

  GtkWidget *const label8 = gtk_label_new (_ ("Idle Seconds:"));
  gtk_label_set_justify (GTK_LABEL (label8), GTK_JUSTIFY_CENTER);
  gtk_misc_set_alignment (GTK_MISC (label8), 0, 0.5);
  gtk_widget_set_name (label8, "label8");
  gtk_widget_show (label8);

  gtk_table_attach (GTK_TABLE (table3), label8, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

  GtkWidget *const idle_secs = gtk_entry_new ();
  dlg->idle_secs = GTK_ENTRY (getwid (idle_secs, dlg));
  gtk_entry_set_invisible_char (GTK_ENTRY (idle_secs), '*');
  gtk_widget_set_can_focus (idle_secs, TRUE);
  gtk_widget_set_name (idle_secs, "idle secs");
  gtk_widget_set_tooltip_text (
      idle_secs, _ ("The current active project will be made inactive after "
                    "there has been no keyboard/mouse activity after this "
                    "number of seconds.  Set to -1 to disable."));
  gtk_widget_show (idle_secs);

  gtk_table_attach (GTK_TABLE (table3), idle_secs, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  gtk_widget_show (table3);

  gtk_container_add (GTK_CONTAINER (frame6), table3);
  gtk_widget_show (frame6);

  gtk_box_pack_start (GTK_BOX (vbox4), frame6, TRUE, TRUE, 0);

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
  dlg->no_project_secs = GTK_ENTRY (getwid (no_project_secs, dlg));
  gtk_entry_set_invisible_char (GTK_ENTRY (no_project_secs), '*');
  gtk_widget_set_can_focus (no_project_secs, TRUE);
  gtk_widget_set_name (no_project_secs, "no project secs");
  gtk_widget_set_tooltip_text (
      no_project_secs,
      _ ("A warning will be displayed after this number of seconds with no "
         "running project.  Set to -1 to disable."));
  gtk_widget_show (no_project_secs);

  gtk_table_attach (GTK_TABLE (table7), no_project_secs, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  gtk_widget_show (table7);

  gtk_container_add (GTK_CONTAINER (frame9), table7);
  gtk_widget_show (frame9);

  gtk_box_pack_start (GTK_BOX (vbox4), frame9, TRUE, TRUE, 0);

  GtkWidget *const frame8 = gtk_frame_new (_ ("End of Day/Week"));
  gtk_frame_set_label_align (GTK_FRAME (frame8), 0, 0.5);
  gtk_widget_set_name (frame8, "frame8");

  GtkWidget *const table6 = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table6), 8);
  gtk_table_set_row_spacings (GTK_TABLE (table6), 3);
  gtk_widget_set_name (table6, "table6");

  GtkWidget *const label17 = gtk_label_new (_ ("New Day Starts At:"));
  gtk_misc_set_alignment (GTK_MISC (label17), 0, 0.5);
  gtk_widget_set_name (label17, "label17");
  gtk_widget_show (label17);

  gtk_table_attach (GTK_TABLE (table6), label17, 0, 1, 0, 1, GTK_FILL, 0, 0,
                    0);

  GtkWidget *const hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox1, "hbox1");

  GtkWidget *const daystart_entry = gtk_entry_new ();
  dlg->daystart_secs = GTK_ENTRY (getwid (daystart_entry, dlg));
  gtk_entry_set_invisible_char (GTK_ENTRY (daystart_entry), '*');
  gtk_widget_set_can_focus (daystart_entry, TRUE);
  gtk_widget_set_name (daystart_entry, "daystart entry");
  gtk_widget_set_tooltip_text (
      daystart_entry,
      _ ("The time of night at which one day ends and the next day begins.  "
         "By default midnight, you can set this to any value."));
  gtk_widget_show (daystart_entry);

  gtk_box_pack_start (GTK_BOX (hbox1), daystart_entry, TRUE, TRUE, 0);

  GtkWidget *const daystart_combobox = gtk_combo_box_new_text ();
  dlg->daystart_menu = GTK_COMBO_BOX (getwid (daystart_combobox, dlg));
  gtk_widget_set_events (daystart_combobox, GDK_BUTTON_PRESS_MASK
                                                | GDK_BUTTON_RELEASE_MASK
                                                | GDK_POINTER_MOTION_HINT_MASK
                                                | GDK_POINTER_MOTION_MASK);
  gtk_widget_set_name (daystart_combobox, "daystart combobox");
  gtk_combo_box_append_text (GTK_COMBO_BOX (daystart_combobox), "9 PM");
  gtk_combo_box_append_text (GTK_COMBO_BOX (daystart_combobox), "10 PM");
  gtk_combo_box_append_text (GTK_COMBO_BOX (daystart_combobox), "11 PM");
  gtk_combo_box_append_text (GTK_COMBO_BOX (daystart_combobox), "12 Midnight");
  gtk_combo_box_append_text (GTK_COMBO_BOX (daystart_combobox), "01 AM");
  gtk_combo_box_append_text (GTK_COMBO_BOX (daystart_combobox), "02 AM");
  gtk_combo_box_append_text (GTK_COMBO_BOX (daystart_combobox), "03 AM");
  gtk_combo_box_append_text (GTK_COMBO_BOX (daystart_combobox), "04 AM");
  gtk_combo_box_append_text (GTK_COMBO_BOX (daystart_combobox), "05 AM");
  gtk_combo_box_append_text (GTK_COMBO_BOX (daystart_combobox), "06 AM");
  g_signal_connect (G_OBJECT (daystart_combobox), "changed",
                    G_CALLBACK (daystart_menu_changed), dlg);
  gtk_widget_show (daystart_combobox);

  gtk_box_pack_start (GTK_BOX (hbox1), daystart_combobox, TRUE, TRUE, 0);

  gtk_widget_show (hbox1);

  gtk_table_attach (GTK_TABLE (table6), hbox1, 1, 2, 0, 1, GTK_FILL, GTK_FILL,
                    0, 0);

  GtkWidget *const label18 = gtk_label_new (_ ("New Week Starts On:"));
  gtk_misc_set_alignment (GTK_MISC (label18), 0, 0.5);
  gtk_widget_set_name (label18, "label18");
  gtk_widget_show (label18);

  gtk_table_attach (GTK_TABLE (table6), label18, 0, 1, 1, 2, GTK_FILL, 0, 0,
                    0);

  GtkWidget *const weekstart_combobox = gtk_combo_box_new_text ();
  dlg->weekstart_menu = GTK_COMBO_BOX (getwid (weekstart_combobox, dlg));
  gtk_widget_set_events (weekstart_combobox, GDK_BUTTON_PRESS_MASK
                                                 | GDK_BUTTON_RELEASE_MASK
                                                 | GDK_POINTER_MOTION_HINT_MASK
                                                 | GDK_POINTER_MOTION_MASK);
  gtk_widget_set_name (weekstart_combobox, "weekstart, combobox");
  gtk_combo_box_append_text (GTK_COMBO_BOX (weekstart_combobox), _ ("Sunday"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (weekstart_combobox), _ ("Monday"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (weekstart_combobox),
                             _ ("Tuesday"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (weekstart_combobox),
                             _ ("Wednesday"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (weekstart_combobox),
                             _ ("Thursday"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (weekstart_combobox), _ ("Friday"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (weekstart_combobox),
                             _ ("Saturday"));
  gtk_widget_show (weekstart_combobox);

  gtk_table_attach (GTK_TABLE (table6), weekstart_combobox, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  gtk_widget_show (table6);

  gtk_container_add (GTK_CONTAINER (frame8), table6);
  gtk_widget_show (frame8);

  gtk_box_pack_start (GTK_BOX (vbox4), frame8, TRUE, TRUE, 0);

  gtk_widget_show (vbox4);

  GtkWidget *const label5 = gtk_label_new (_ ("Misc"));
  gtk_label_set_justify (GTK_LABEL (label5), GTK_JUSTIFY_CENTER);
  gtk_widget_set_name (label5, "label5");
  gtk_widget_show (label5);

  gtt_property_box_append_page (dlg->dlg, vbox4, label5);
}

static void
time_format_options (GtkWidget *const vbox, PrefsDialog *dlg)
{
  GtkWidget *const frame10 = gtk_frame_new (_ ("Time format"));
  gtk_container_set_border_width (GTK_CONTAINER (frame10), 4);
  gtk_frame_set_label_align (GTK_FRAME (frame10), 0, 0.5);
  gtk_widget_set_name (frame10, "frame10");

  GtkWidget *const alignment1 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment1), 0, 0, 12, 0);
  gtk_widget_set_name (alignment1, "alignment1");

  GtkWidget *const vbox6 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox6, "vbox6");

  GtkWidget *const time_format_am_pm
      = gtk_radio_button_new_with_mnemonic (NULL, _ (_ ("12 hours")));
  dlg->time_format_am_pm
      = GTK_RADIO_BUTTON (getchwid (time_format_am_pm, dlg));
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (time_format_am_pm), TRUE);
  gtk_widget_set_can_focus (time_format_am_pm, TRUE);
  gtk_widget_set_name (time_format_am_pm, "time_format_am_pm");
  gtk_widget_show (time_format_am_pm);

  gtk_box_pack_start (GTK_BOX (vbox6), time_format_am_pm, FALSE, FALSE, 0);

  GtkWidget *const time_format_24_hs = gtk_radio_button_new_with_mnemonic (
      gtk_radio_button_get_group (GTK_RADIO_BUTTON (time_format_am_pm)),
      _ (_ ("24 hours")));
  dlg->time_format_24_hs
      = GTK_RADIO_BUTTON (getchwid (time_format_24_hs, dlg));
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (time_format_24_hs), TRUE);
  gtk_widget_set_can_focus (time_format_24_hs, TRUE);
  gtk_widget_set_name (time_format_24_hs, "time_format_24_hs");
  gtk_widget_show (time_format_24_hs);

  gtk_box_pack_start (GTK_BOX (vbox6), time_format_24_hs, FALSE, FALSE, 0);

  GtkWidget *const time_format_locale = gtk_radio_button_new_with_mnemonic (
      gtk_radio_button_get_group (GTK_RADIO_BUTTON (time_format_24_hs)),
      _ (_ ("Use my locale formating")));
  dlg->time_format_locale
      = GTK_RADIO_BUTTON (getchwid (time_format_locale, dlg));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (time_format_locale), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (time_format_locale), TRUE);
  gtk_widget_set_can_focus (time_format_locale, TRUE);
  gtk_widget_set_name (time_format_locale, "time_format_locale");
  gtk_widget_show (time_format_locale);

  gtk_box_pack_start (GTK_BOX (vbox6), time_format_locale, FALSE, FALSE, 0);

  gtk_widget_show (vbox6);

  gtk_container_add (GTK_CONTAINER (alignment1), vbox6);
  gtk_widget_show (alignment1);

  gtk_container_add (GTK_CONTAINER (frame10), alignment1);
  gtk_widget_show (frame10);

  gtk_box_pack_start (GTK_BOX (vbox), frame10, TRUE, TRUE, 0);
}

static void
currency_options (GtkWidget *const vbox, PrefsDialog *dlg)
{
  GtkWidget *const frame11 = gtk_frame_new (_ ("Currency settings"));
  gtk_container_set_border_width (GTK_CONTAINER (frame11), 4);
  gtk_frame_set_label_align (GTK_FRAME (frame11), 0, 0.5);
  gtk_widget_set_name (frame11, "frame11");

  GtkWidget *const table8 = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table8), 8);
  gtk_table_set_row_spacings (GTK_TABLE (table8), 3);
  gtk_widget_set_name (table8, "table8");

  GtkWidget *const currency_use_locale
      = gtk_check_button_new_with_label (_ ("Use my locale formating"));
  dlg->currency_use_locale
      = GTK_CHECK_BUTTON (getchwid (currency_use_locale, dlg));
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
  gtk_widget_show (currency_use_locale);

  gtk_table_attach (GTK_TABLE (table8), currency_use_locale, 0, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const currency_symbol_label
      = gtk_label_new (_ ("Currency Symbol"));
  dlg->currency_symbol_label = currency_symbol_label;
  gtk_label_set_justify (GTK_LABEL (currency_symbol_label),
                         GTK_JUSTIFY_CENTER);
  gtk_misc_set_alignment (GTK_MISC (currency_symbol_label), 0, 0.5);
  gtk_widget_set_name (currency_symbol_label, "currency_symbol_label");
  gtk_widget_show (currency_symbol_label);

  gtk_table_attach (GTK_TABLE (table8), currency_symbol_label, 0, 1, 1, 2,
                    GTK_FILL, 0, 0, 0);

  GtkWidget *const currency_symbol = gtk_entry_new ();
  dlg->currency_symbol = GTK_ENTRY (getwid (currency_symbol, dlg));
  gtk_entry_set_invisible_char (GTK_ENTRY (currency_symbol), '*');
  gtk_entry_set_max_length (GTK_ENTRY (currency_symbol), 6);
  gtk_entry_set_text (GTK_ENTRY (currency_symbol), "$");
  gtk_widget_set_can_focus (currency_symbol, TRUE);
  gtk_widget_set_name (currency_symbol, "currency_symbol");
  gtk_widget_set_tooltip_text (
      currency_symbol,
      _ ("Place your local currency symbol for use in the reports"));
  gtk_widget_show (currency_symbol);

  gtk_table_attach (GTK_TABLE (table8), currency_symbol, 1, 2, 1, 2,
                    GTK_EXPAND, 0, 0, 0);

  gtk_widget_show (table8);

  gtk_container_add (GTK_CONTAINER (frame11), table8);
  gtk_widget_show (frame11);

  gtk_box_pack_start (GTK_BOX (vbox), frame11, TRUE, TRUE, 0);
}

/* ============================================================== */

static void
help_cb (GttPropertyBox *propertybox, gint page_num, gpointer data)
{
  gtt_help_popup (GTK_WIDGET (propertybox), data);
}

static PrefsDialog *
prefs_dialog_new (void)
{
  PrefsDialog *dlg;

  dlg = g_malloc (sizeof (PrefsDialog));

  GtkWidget *const global_preferences = gtt_property_box_new ();
  dlg->dlg = GTT_PROPERTY_BOX (global_preferences);
  gtk_widget_set_name (global_preferences, "Global Preferences");
  gtk_window_set_resizable (GTK_WINDOW (global_preferences), FALSE);

  gtk_signal_connect (GTK_OBJECT (dlg->dlg), "help", GTK_SIGNAL_FUNC (help_cb),
                      "preferences");

  gtk_signal_connect (GTK_OBJECT (dlg->dlg), "apply",
                      GTK_SIGNAL_FUNC (prefs_set), dlg);

  /* ------------------------------------------------------ */
  /* grab the various entry boxes and hook them up */
  field_options (dlg);
  display_options (dlg);
  shell_command_options (dlg);
  logfile_options (dlg);
  toolbar_options (dlg);
  misc_options (dlg);

  GtkWidget *const vbox6 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox6, "vbox6");

  time_format_options (vbox6, dlg);
  currency_options (vbox6, dlg);

  gtk_widget_show (vbox6);

  GtkWidget *const label21 = gtk_label_new (_ ("Reports"));
  gtk_widget_set_name (label21, "label21");
  gtk_widget_show (label21);

  gtt_property_box_append_page (dlg->dlg, vbox6, label21);

  gtk_widget_show (global_preferences);

  gtt_dialog_close_hides (GTT_DIALOG (dlg->dlg), TRUE);
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

/* ==================== END OF FILE ============================= */
