/*   GConf2 input/output handling for GTimeTracker - a time tracker
 *   Copyright (C) 2003 Linas Vepstas <linas@linas.org>
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

#include "gtt_gsettings_gnomeui.h"
#include "gtt_gsettings_io_p.h"

#include <gconf/gconf-client.h>
#include <gconf/gconf.h>
#include <glib.h>
#include <gnome.h>

#include "app.h"
#include "cur-proj.h"
#include "gtt.h"
#include "menus.h"
#include "plug-in.h"
#include "prefs.h"
#include "timer.h"
#include "toolbar.h"

#include <gio/gio.h>

static void get_str (GSettings *setts, const gchar *key, gchar **value);
static void init_gsettings (void);
static void set_bool (GSettings *setts, const gchar *key, gboolean value);
static void set_int (GSettings *setts, const gchar *key, gint value);
static void set_str (GSettings *setts, const gchar *key, const gchar *value);

/* XXX these should not be externs, they should be part of
 * some app-global structure.
 */
extern char *first_proj_title; /* command line flag */
extern time_t last_timer;      /* XXX */
extern int cur_proj_id;
extern int run_timer;

int save_count = 0;

static GSettings *settings = NULL;

#define GTT_GCONF "/apps/gnotime"

/* ======================================================= */

void
gtt_save_reports_menu (void)
{
  init_gsettings ();

  int i;
  char s[120], *p;
  GnomeUIInfo *reports_menu;
  GConfClient *client;

  client = gconf_client_get_default ();
  reports_menu = gtt_get_reports_menu ();

  /* Write out the customer report info */
  for (i = 0; GNOME_APP_UI_ENDOFINFO != reports_menu[i].type; i++)
    {
      GttPlugin *plg = reports_menu[i].user_data;
      g_snprintf (s, sizeof (s), GTT_GCONF "/Reports/%d/", i);
      p = s + strlen (s);
      strcpy (p, "Name");
      F_SETSTR (s, plg->name);

      strcpy (p, "Path");
      F_SETSTR (s, plg->path);

      strcpy (p, "Tooltip");
      F_SETSTR (s, plg->tooltip);

      strcpy (p, "LastSaveURL");
      F_SETSTR (s, plg->last_url);

      *p = 0;
      gtt_save_gnomeui_to_gsettings (client, s, &reports_menu[i]);
    }
  SETINT ("/Misc/NumReports", i);
}

/* ======================================================= */
/* Save only the GUI configuration info, not the actual data */
/* XXX fixme -- this should really use GConfChangeSet */

void
gtt_gsettings_save (void)
{
  char s[120];
  int x, y, w, h;
  const char *xpn;

  GConfEngine *gengine;
  GConfClient *client;

  init_gsettings ();

  gengine = gconf_engine_get_default ();
  client = gconf_client_get_for_engine (gengine);
  SETINT ("/dir_exists", 1);

  // Geometry -----------------------------------------------------------------
  {
    GSettings *geometry = g_settings_get_child (settings, "geometry");

    /* save the window location and size */
    gdk_window_get_origin (app_window->window, &x, &y);
    gdk_window_get_size (app_window->window, &w, &h);
    set_int (geometry, "width", w);
    set_int (geometry, "height", h);
    set_int (geometry, "x", x);
    set_int (geometry, "y", y);

    {
      int vp, hp;
      notes_area_get_pane_sizes (global_na, &vp, &hp);
      set_int (geometry, "v-paned", vp);
      set_int (geometry, "h-paned", hp);
    }

    g_object_unref (geometry);
    geometry = NULL;
  }
  /* ------------- */
  /* save the configure dialog values */
  SETBOOL ("/Display/ShowSecs", config_show_secs);
  SETBOOL ("/Display/ShowStatusbar", config_show_statusbar);
  SETBOOL ("/Display/ShowSubProjects", config_show_subprojects);
  SETBOOL ("/Display/ShowTableHeader", config_show_clist_titles);
  SETBOOL ("/Display/ShowTimeCurrent", config_show_title_current);
  SETBOOL ("/Display/ShowTimeDay", config_show_title_day);
  SETBOOL ("/Display/ShowTimeYesterday", config_show_title_yesterday);
  SETBOOL ("/Display/ShowTimeWeek", config_show_title_week);
  SETBOOL ("/Display/ShowTimeLastWeek", config_show_title_lastweek);
  SETBOOL ("/Display/ShowTimeMonth", config_show_title_month);
  SETBOOL ("/Display/ShowTimeYear", config_show_title_year);
  SETBOOL ("/Display/ShowTimeEver", config_show_title_ever);
  SETBOOL ("/Display/ShowDesc", config_show_title_desc);
  SETBOOL ("/Display/ShowTask", config_show_title_task);
  SETBOOL ("/Display/ShowEstimatedStart", config_show_title_estimated_start);
  SETBOOL ("/Display/ShowEstimatedEnd", config_show_title_estimated_end);
  SETBOOL ("/Display/ShowDueDate", config_show_title_due_date);
  SETBOOL ("/Display/ShowSizing", config_show_title_sizing);
  SETBOOL ("/Display/ShowPercentComplete", config_show_title_percent_complete);
  SETBOOL ("/Display/ShowUrgency", config_show_title_urgency);
  SETBOOL ("/Display/ShowImportance", config_show_title_importance);
  SETBOOL ("/Display/ShowStatus", config_show_title_status);

  xpn = gtt_projects_tree_get_expander_state (projects_tree);
  SETSTR ("/Display/ExpanderState", xpn);

  // Toolbar ------------------------------------------------------------------
  {
    GSettings *toolbar = g_settings_get_child (settings, "toolbar");

    set_bool (toolbar, "show-toolbar", config_show_toolbar);
    set_bool (toolbar, "show-tips", config_show_tb_tips);
    set_bool (toolbar, "show-new", config_show_tb_new);
    set_bool (toolbar, "show-ccp", config_show_tb_ccp);
    set_bool (toolbar, "show-journal", config_show_tb_journal);
    set_bool (toolbar, "show-prop", config_show_tb_prop);
    set_bool (toolbar, "show-timer", config_show_tb_timer);
    set_bool (toolbar, "show-pref", config_show_tb_pref);
    set_bool (toolbar, "show-help", config_show_tb_help);
    set_bool (toolbar, "show-exit", config_show_tb_exit);

    g_object_unref (toolbar);
    toolbar = NULL;
  }

  /* ------------- */
  if (config_shell_start)
    {
      SETSTR ("/Actions/StartCommand", config_shell_start);
    }
  else
    {
      UNSET ("/Actions/StartCommand");
    }

  if (config_shell_stop)
    {
      SETSTR ("/Actions/StopCommand", config_shell_stop);
    }
  else
    {
      UNSET ("/Actions/StopCommand");
    }

  /* ------------- */
  SETBOOL ("/LogFile/Use", config_logfile_use);
  if (config_logfile_name)
    {
      SETSTR ("/LogFile/Filename", config_logfile_name);
    }
  else
    {
      UNSET ("/LogFile/Filename");
    }

  if (config_logfile_start)
    {
      SETSTR ("/LogFile/EntryStart", config_logfile_start);
    }
  else
    {
      SETSTR ("/LogFile/EntryStart", "");
    }

  if (config_logfile_stop)
    {
      SETSTR ("/LogFile/EntryStop", config_logfile_stop);
    }
  else
    {
      SETSTR ("/LogFile/EntryStop", "");
    }

  SETINT ("/LogFile/MinSecs", config_logfile_min_secs);

  // Data ---------------------------------------------------------------------
  {
    GSettings *data = g_settings_get_child (settings, "data");

    set_str (data, "url", config_data_url);
    set_int (data, "save-count", save_count);

    g_object_unref (data);
    data = NULL;
  }

  /* ------------- */
  {
    long i, w;
    GSList *list = NULL;
    for (i = 0, w = 0; -1 < w; i++)
      {
        w = gtt_projects_tree_get_col_width (projects_tree, i);
        if (0 > w)
          break;
        list = g_slist_prepend (list, (gpointer)w);
      }
    list = g_slist_reverse (list);
    SETLIST ("/CList/ColumnWidths", GCONF_VALUE_INT, list);
    g_slist_free (list);
  }

  /* ------------- */
  /* Use string for time, to avoid integer conversion problems */
  g_snprintf (s, sizeof (s), "%ld", time (0));
  SETSTR ("/Misc/LastTimer", s);
  SETINT ("/Misc/IdleTimeout", config_idle_timeout);
  SETINT ("/Misc/NoProjectTimeout", config_no_project_timeout);
  SETINT ("/Misc/AutosavePeriod", config_autosave_period);
  SETINT ("/Misc/TimerRunning", timer_is_running ());
  SETINT ("/Misc/CurrProject", gtt_project_get_id (cur_proj));
  SETINT ("/Misc/NumProjects", -1);

  SETINT ("/Misc/DayStartOffset", config_daystart_offset);
  SETINT ("/Misc/WeekStartOffset", config_weekstart_offset);

  SETINT ("/time_format", config_time_format);

  SETSTR ("/Report/CurrencySymbol", config_currency_symbol);
  SETBOOL ("/Report/CurrencyUseLocale", config_currency_use_locale);

  /* Write out the user's report menu structure */
  gtt_save_reports_menu ();

  /* Sync to file.
   * XXX if this fails, the error is serious, and there should be a
   * graphical popup.
   */
  {
    GError *err_ret = NULL;
    gconf_client_suggest_sync (client, &err_ret);
    if (NULL != err_ret)
      {
        printf ("GTT: GConf: Sync Failed\n");
      }
  }
}

/* ======================================================= */

gboolean
gtt_gconf_exists (void)
{
  GError *err_ret = NULL;
  GConfClient *client;
  GConfValue *gcv;

  init_gsettings ();

  /* Calling gconf_engine_dir_exists() on a non-existant directory
   * completely hoses that directory for future use. Its Badddd.
   * rc = gconf_engine_dir_exists (gengine, GTT_GCONF, &err_ret);
   * gconf_client_dir_exists() is no better.
   * Actually, the bug is that the dirs are unusable only while
   * gconf is still running. Upon reboot, its starts working OK.
   * Hack around it by trying to fetch a key.
   */

  client = gconf_client_get_default ();
  gcv = gconf_client_get (client, GTT_GCONF "/dir_exists", &err_ret);
  if ((NULL == gcv) || (FALSE == GCONF_VALUE_TYPE_VALID (gcv->type)))
    {
      if (err_ret)
        printf ("GTT: Error: gconf_exists XXX err %s\n", err_ret->message);
      return FALSE;
    }

  return TRUE;
}

/* ======================================================= */

void
gtt_restore_reports_menu (GnomeApp *app)
{
  int i, num;
  char s[120], *p;
  GnomeUIInfo *reports_menu;
  GConfClient *client;

  init_gsettings ();

  client = gconf_client_get_default ();

  /* Read in the user-defined report locations */
  num = GETINT ("/Misc/NumReports", 0);
  reports_menu = g_new0 (GnomeUIInfo, num + 1);

  for (i = 0; i < num; i++)
    {
      GttPlugin *plg;
      const char *name, *path, *tip, *url;

      g_snprintf (s, sizeof (s), GTT_GCONF "/Reports/%d/", i);
      p = s + strlen (s);

      strcpy (p, "Name");
      name = F_GETSTR (s, "");

      strcpy (p, "Path");
      path = F_GETSTR (s, "");

      strcpy (p, "Tooltip");
      tip = F_GETSTR (s, "");

      strcpy (p, "LastSaveURL");
      url = F_GETSTR (s, "");

      plg = gtt_plugin_new (name, path);
      plg->tooltip = g_strdup (tip);
      plg->last_url = g_strdup (url);

      *p = 0;
      gtt_restore_gnomeui_from_gsettings (client, s, &reports_menu[i]);

      /* fixup */
      reports_menu[i].user_data = plg;
    }
  reports_menu[i].type = GNOME_APP_UI_ENDOFINFO;

  gtt_set_reports_menu (app, reports_menu);
}

/* ======================================================= */

void
gtt_gsettings_load (void)
{
  int i, num;
  int _n, _c, _j, _p, _t, _o, _h, _e;
  GConfClient *client;

  init_gsettings ();

  client = gconf_client_get_default ();
  gconf_client_add_dir (client, GTT_GCONF, GCONF_CLIENT_PRELOAD_RECURSIVE,
                        NULL);

  /* If already running, and we are over-loading a new file,
   * then save the currently running project, and try to set it
   * running again ... */
  if (gtt_project_get_title (cur_proj) && (!first_proj_title))
    {
      /* We need to strdup because title is freed when
       * the project list is destroyed ... */
      first_proj_title = g_strdup (gtt_project_get_title (cur_proj));
    }

  _n = config_show_tb_new;
  _c = config_show_tb_ccp;
  _j = config_show_tb_journal;
  _p = config_show_tb_prop;
  _t = config_show_tb_timer;
  _o = config_show_tb_pref;
  _h = config_show_tb_help;
  _e = config_show_tb_exit;

  /* Get last running project */
  cur_proj_id = GETINT ("/Misc/CurrProject", -1);

  config_idle_timeout = GETINT ("/Misc/IdleTimeout", 300);
  config_no_project_timeout = GETINT ("/Misc/NoProjectTimeout", 300);
  config_autosave_period = GETINT ("/Misc/AutosavePeriod", 60);
  config_daystart_offset = GETINT ("/Misc/DayStartOffset", 0);
  config_weekstart_offset = GETINT ("/Misc/WeekStartOffset", 0);

  // Geometry -----------------------------------------------------------------
  {
    GSettings *geometry = g_settings_get_child (settings, "geometry");

    /* Reset the main window width and height to the values
     * last stored in the config file.  Note that if the user
     * specified command-line flags, then the command line
     * over-rides the config file. */
    if (!geom_place_override)
      {
        int x, y;
        x = g_settings_get_int (geometry, "x");
        y = g_settings_get_int (geometry, "y");
        gtk_widget_set_uposition (GTK_WIDGET (app_window), x, y);
      }
    if (!geom_size_override)
      {
        int w, h;
        w = g_settings_get_int (geometry, "width");
        h = g_settings_get_int (geometry, "height");

        gtk_window_set_default_size (GTK_WINDOW (app_window), w, h);
      }

    {
      int vp, hp;
      vp = g_settings_get_int (geometry, "v-paned");
      hp = g_settings_get_int (geometry, "h-paned");
      notes_area_set_pane_sizes (global_na, vp, hp);
    }

    g_object_unref (geometry);
    geometry = NULL;
  }

  config_show_secs = GETBOOL ("/Display/ShowSecs", FALSE);

  prefs_set_show_secs ();

  config_show_clist_titles = GETBOOL ("/Display/ShowTableHeader", FALSE);
  config_show_subprojects = GETBOOL ("/Display/ShowSubProjects", TRUE);
  config_show_statusbar = GETBOOL ("/Display/ShowStatusbar", TRUE);

  config_show_title_ever = GETBOOL ("/Display/ShowTimeEver", TRUE);
  config_show_title_day = GETBOOL ("/Display/ShowTimeDay", TRUE);
  config_show_title_yesterday = GETBOOL ("/Display/ShowTimeYesterday", FALSE);
  config_show_title_week = GETBOOL ("/Display/ShowTimeWeek", FALSE);
  config_show_title_lastweek = GETBOOL ("/Display/ShowTimeLastWeek", FALSE);
  config_show_title_month = GETBOOL ("/Display/ShowTimeMonth", FALSE);
  config_show_title_year = GETBOOL ("/Display/ShowTimeYear", FALSE);
  config_show_title_current = GETBOOL ("/Display/ShowTimeCurrent", FALSE);
  config_show_title_desc = GETBOOL ("/Display/ShowDesc", TRUE);
  config_show_title_task = GETBOOL ("/Display/ShowTask", TRUE);
  config_show_title_estimated_start
      = GETBOOL ("/Display/ShowEstimatedStart", FALSE);
  config_show_title_estimated_end
      = GETBOOL ("/Display/ShowEstimatedEnd", FALSE);
  config_show_title_due_date = GETBOOL ("/Display/ShowDueDate", FALSE);
  config_show_title_sizing = GETBOOL ("/Display/ShowSizing", FALSE);
  config_show_title_percent_complete
      = GETBOOL ("/Display/ShowPercentComplete", FALSE);
  config_show_title_urgency = GETBOOL ("/Display/ShowUrgency", TRUE);
  config_show_title_importance = GETBOOL ("/Display/ShowImportance", TRUE);
  config_show_title_status = GETBOOL ("/Display/ShowStatus", FALSE);

  prefs_update_projects_view ();

  // Toolbar ------------------------------------------------------------------
  {
    GSettings *toolbar = g_settings_get_child (settings, "toolbar");

    config_show_toolbar = g_settings_get_boolean (toolbar, "show-toolbar");
    config_show_tb_tips = g_settings_get_boolean (toolbar, "show-tips");
    config_show_tb_new = g_settings_get_boolean (toolbar, "show-new");
    config_show_tb_ccp = g_settings_get_boolean (toolbar, "show-ccp");
    config_show_tb_journal = g_settings_get_boolean (toolbar, "show-journal");
    config_show_tb_prop = g_settings_get_boolean (toolbar, "show-prop");
    config_show_tb_timer = g_settings_get_boolean (toolbar, "show-timer");
    config_show_tb_pref = g_settings_get_boolean (toolbar, "show-pref");
    config_show_tb_help = g_settings_get_boolean (toolbar, "show-help");
    config_show_tb_exit = g_settings_get_boolean (toolbar, "show-exit");

    g_object_unref (toolbar);
    toolbar = NULL;
  }

  /* ------------ */
  config_shell_start = GETSTR ("/Actions/StartCommand",
                               "echo start id=%D \\\"%t\\\"-\\\"%d\\\" %T  "
                               "%H-%M-%S hours=%h min=%m secs=%s");
  config_shell_stop = GETSTR ("/Actions/StopCommand",
                              "echo stop id=%D \\\"%t\\\"-\\\"%d\\\" %T  "
                              "%H-%M-%S hours=%h min=%m secs=%s");

  /* ------------ */
  config_logfile_use = GETBOOL ("/LogFile/Use", FALSE);
  config_logfile_name = GETSTR ("/LogFile/Filename", NULL);
  config_logfile_start
      = GETSTR ("/LogFile/EntryStart", _ ("project %t started"));
  config_logfile_stop
      = GETSTR ("/LogFile/EntryStop", _ ("stopped project %t"));
  config_logfile_min_secs = GETINT ("/LogFile/MinSecs", 3);

  /* ------------ */
  config_time_format = GETINT ("/time_format", 3);

  config_currency_symbol = GETSTR ("/Report/CurrencySymbol", "$");
  config_currency_use_locale = GETBOOL ("/Report/CurrencyUseLocale", TRUE);

  // Data ---------------------------------------------------------------------
  {
    GSettings *data = g_settings_get_child (settings, "data");

    save_count = g_settings_get_int (data, "save-count");
    get_str (data, "url", &config_data_url);

    g_object_unref (data);
    data = NULL;
  }

  /* ------------ */
  {
    GSList *node, *list = GETINTLIST ("/CList/ColumnWidths");
    for (i = 0, node = list; node != NULL; node = node->next, i++)
      {
        num = (long)(node->data);
        if (-1 < num)
          {
            gtt_projects_tree_set_col_width (projects_tree, i, num);
          }
      }
  }

  /* Read in the user-defined report locations */
  gtt_restore_reports_menu (GNOME_APP (app_window));

  run_timer = GETINT ("/Misc/TimerRunning", 0);
  /* Use string for time, to avoid unsigned-long problems */
  last_timer = (time_t)atol (GETSTR ("/Misc/LastTimer", "-1"));

  /* redraw the GUI */
  if (config_show_statusbar)
    {
      gtk_widget_show (status_bar);
    }
  else
    {
      gtk_widget_hide (status_bar);
    }

  update_status_bar ();
  if ((_n != config_show_tb_new) || (_c != config_show_tb_ccp)
      || (_j != config_show_tb_journal) || (_p != config_show_tb_prop)
      || (_t != config_show_tb_timer) || (_o != config_show_tb_pref)
      || (_h != config_show_tb_help) || (_e != config_show_tb_exit))
    {
      update_toolbar_sections ();
    }
}

gchar *
gtt_gsettings_get_expander (void)
{
  init_gsettings ();

  GConfClient *client = gconf_client_get_default ();
  return GETSTR ("/Display/ExpanderState", NULL);
}

static void
get_str (GSettings *const setts, const gchar *const key, gchar **const value)
{
  if (NULL != *value)
    {
      g_free (*value);
      *value = NULL;
    }

  *value = g_settings_get_string (setts, key);
}

static void
init_gsettings (void)
{
  if (G_LIKELY (NULL != settings))
    {
      return; // Initialized already, nothing to do
    }

  settings = g_settings_new ("com.github.goedson.gnotime");
}

static void
set_bool (GSettings *const setts, const gchar *const key, const gboolean value)
{
  if (FALSE == g_settings_set_boolean (setts, key, value))
    {
      g_warning (
          _ ("Failed to set boolean GSettings option \"%s\" to value: %s"),
          key, (FALSE == value) ? "false" : "true");
    }
}

static void
set_int (GSettings *const setts, const gchar *const key, const gint value)
{
  if (FALSE == g_settings_set_int (setts, key, value))
    {
      g_warning (
          _ ("Failed to set integer GSettings option \"%s\" to value: %d"),
          key, value);
    }
}

static void
set_str (GSettings *setts, const gchar *key, const gchar *const value)
{
  if (FALSE == g_settings_set_string (setts, key, value))
    {
      g_warning (
          _ ("Failed to set string GSettings option \"%s\" to value: \"%s\""),
          key, value);
    }
}

/* =========================== END OF FILE ========================= */
