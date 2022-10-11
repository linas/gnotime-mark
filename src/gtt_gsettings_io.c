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

static GSList *get_int_arr (GSettings *setts, const gchar *key);
static void get_maybe_str (GSettings *setts, const gchar *key, gchar **value);
static void init_gsettings (void);
static void set_bool (GSettings *setts, const gchar *key, gboolean value);
static void set_int_arr (GSettings *setts, const gchar *key, GSList *value);
static void set_maybe_str (GSettings *setts, const gchar *key,
                           const gchar *value);

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
  GnomeUIInfo *reports_menu;

  reports_menu = gtt_get_reports_menu ();

  /* Write out the customer report info */
  for (i = 0; GNOME_APP_UI_ENDOFINFO != reports_menu[i].type; i++)
    {
      GttPlugin *plg = reports_menu[i].user_data;

      gchar *path = g_strdup_printf (
          "/com/github/goedson/gnotime/reports/report%d/", i);

      GSettings *report = g_settings_new_with_path (
          "com.github.goedson.gnotime.reports", path);

      set_str (report, "name", plg->name);
      set_str (report, "path", plg->path);
      set_str (report, "tooltip", plg->tooltip);
      set_maybe_str (report, "last-save-url", plg->last_url);

      gtt_save_gnomeui_to_gsettings (report, &reports_menu[i]);

      g_object_unref (report);
      report = NULL;

      g_free (path);
      path = NULL;
    }
  {
    GSettings *misc = g_settings_get_child (settings, "misc");
    set_int (misc, "num-reports", i);
    g_object_unref (misc);
    misc = NULL;
  }
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

  // Display ------------------------------------------------------------------
  {
    GSettings *display = g_settings_get_child (settings, "display");

    /* save the configure dialog values */
    set_bool (display, "show-secs", config_show_secs);
    set_bool (display, "show-statusbar", config_show_statusbar);
    set_bool (display, "show-sub-projects", config_show_subprojects);
    set_bool (display, "show-table-header", config_show_clist_titles);
    set_bool (display, "show-time-current", config_show_title_current);
    set_bool (display, "show-time-day", config_show_title_day);
    set_bool (display, "show-time-yesterday", config_show_title_yesterday);
    set_bool (display, "show-time-week", config_show_title_week);
    set_bool (display, "show-time-last-week", config_show_title_lastweek);
    set_bool (display, "show-time-month", config_show_title_month);
    set_bool (display, "show-time-year", config_show_title_year);
    set_bool (display, "show-time-ever", config_show_title_ever);
    set_bool (display, "show-desc", config_show_title_desc);
    set_bool (display, "show-task", config_show_title_task);
    set_bool (display, "show-estimated-start",
              config_show_title_estimated_start);
    set_bool (display, "show-estimated-end", config_show_title_estimated_end);
    set_bool (display, "show-due-date", config_show_title_due_date);
    set_bool (display, "show-sizing", config_show_title_sizing);
    set_bool (display, "show-percent-complete",
              config_show_title_percent_complete);
    set_bool (display, "show-urgency", config_show_title_urgency);
    set_bool (display, "show-importance", config_show_title_importance);
    set_bool (display, "show-status", config_show_title_status);

    xpn = gtt_projects_tree_get_expander_state (projects_tree);
    set_maybe_str (display, "expander-state", xpn);

    g_object_unref (display);
    display = NULL;
  }

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

  // Actions ------------------------------------------------------------------
  {
    GSettings *actions = g_settings_get_child (settings, "actions");

    set_maybe_str (actions, "start-command", config_shell_start);
    set_maybe_str (actions, "stop-command", config_shell_stop);

    g_object_unref (actions);
    actions = NULL;
  }

  // Logfile ------------------------------------------------------------------
  {
    GSettings *logfile = g_settings_get_child (settings, "logfile");

    set_bool (logfile, "use", config_logfile_use);
    set_maybe_str (logfile, "filename", config_logfile_name);
    set_maybe_str (logfile, "entry-start", config_logfile_start);
    set_maybe_str (logfile, "entry-stop", config_logfile_stop);
    set_int (logfile, "min-secs", config_logfile_min_secs);

    g_object_unref (logfile);
    logfile = NULL;
  }

  // Data ---------------------------------------------------------------------
  {
    GSettings *data = g_settings_get_child (settings, "data");

    set_str (data, "url", config_data_url);
    set_int (data, "save-count", save_count);

    g_object_unref (data);
    data = NULL;
  }

  // CList --------------------------------------------------------------------
  {
    GSettings *clist = g_settings_get_child (settings, "clist");

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
    set_int_arr (clist, "column-widths", list);
    g_slist_free (list);

    g_object_unref (clist);
    clist = NULL;
  }

  // Misc ---------------------------------------------------------------------
  {
    GSettings *misc = g_settings_get_child (settings, "misc");

    /* Use string for time, to avoid integer conversion problems */
    g_snprintf (s, sizeof (s), "%ld", time (0));
    set_str (misc, "last-timer", s);
    set_int (misc, "idle-timeout", config_idle_timeout);
    set_int (misc, "no-project-timeout", config_no_project_timeout);
    set_int (misc, "autosave-period", config_autosave_period);
    set_int (misc, "timer-running", timer_is_running ());
    set_int (misc, "current-project", gtt_project_get_id (cur_proj));
    set_int (misc, "num-projects", -1);

    set_int (misc, "day-start-offset", config_daystart_offset);
    set_int (misc, "week-start-offset", config_weekstart_offset);

    g_object_unref (misc);
    misc = NULL;
  }

  set_int (settings, "time-format", config_time_format);

  // Report -------------------------------------------------------------------
  {
    GSettings *report = g_settings_get_child (settings, "report");

    set_str (report, "currency-symbol", config_currency_symbol);
    set_bool (report, "currency-use-locale", config_currency_use_locale);

    g_object_unref (report);
    report = NULL;
  }

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

void
gtt_restore_reports_menu (GnomeApp *app)
{
  int i, num;
  GnomeUIInfo *reports_menu;

  init_gsettings ();

  /* Read in the user-defined report locations */
  {
    GSettings *misc = g_settings_get_child (settings, "misc");
    num = g_settings_get_int (misc, "num-reports");
    g_object_unref (misc);
    misc = NULL;
  }
  reports_menu = g_new0 (GnomeUIInfo, num + 1);

  for (i = 0; i < num; i++)
    {
      GttPlugin *plg;
      const char *name, *path, *tip;
      gchar *url = NULL;

      gchar *setts_path = g_strdup_printf (
          "/com/github/goedson/gnotime/reports/report%d/", i);

      GSettings *report = g_settings_new_with_path (
          "com.github.goedson.gnotime.reports", setts_path);

      name = g_settings_get_string (report, "name");
      path = g_settings_get_string (report, "path");
      tip = g_settings_get_string (report, "tooltip");
      get_maybe_str (report, "last-save-url", &url);

      plg = gtt_plugin_new (name, path);
      plg->tooltip = g_strdup (tip);
      plg->last_url = g_strdup (url);

      gtt_restore_gnomeui_from_gsettings (report, &reports_menu[i]);

      /* fixup */
      reports_menu[i].user_data = plg;

      g_object_unref (report);
      report = NULL;

      g_free (setts_path);
      setts_path = NULL;
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

  // Misc ---------------------------------------------------------------------
  {
    GSettings *misc = g_settings_get_child (settings, "misc");

    /* Get last running project */
    cur_proj_id = g_settings_get_int (misc, "current-project");

    config_idle_timeout = g_settings_get_int (misc, "idle-timeout");
    config_no_project_timeout
        = g_settings_get_int (misc, "no-project-timeout");
    config_autosave_period = g_settings_get_int (misc, "autosave-period");
    config_daystart_offset = g_settings_get_int (misc, "day-start-offset");
    config_weekstart_offset = g_settings_get_int (misc, "week-start-offset");

    g_object_unref (misc);
    misc = NULL;
  }

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

  // Display ------------------------------------------------------------------
  {
    GSettings *display = g_settings_get_child (settings, "display");

    config_show_secs = g_settings_get_boolean (display, "show-secs");

    prefs_set_show_secs ();

    config_show_clist_titles
        = g_settings_get_boolean (display, "show-table-header");
    config_show_subprojects
        = g_settings_get_boolean (display, "show-sub-projects");
    config_show_statusbar = g_settings_get_boolean (display, "show-statusbar");

    config_show_title_ever
        = g_settings_get_boolean (display, "show-time-ever");
    config_show_title_day = g_settings_get_boolean (display, "show-time-day");
    config_show_title_yesterday
        = g_settings_get_boolean (display, "show-time-yesterday");
    config_show_title_week
        = g_settings_get_boolean (display, "show-time-week");
    config_show_title_lastweek
        = g_settings_get_boolean (display, "show-time-last-week");
    config_show_title_month
        = g_settings_get_boolean (display, "show-time-month");
    config_show_title_year
        = g_settings_get_boolean (display, "show-time-year");
    config_show_title_current
        = g_settings_get_boolean (display, "show-time-current");
    config_show_title_desc = g_settings_get_boolean (display, "show-desc");
    config_show_title_task = g_settings_get_boolean (display, "show-task");
    config_show_title_estimated_start
        = g_settings_get_boolean (display, "show-estimated-start");
    config_show_title_estimated_end
        = g_settings_get_boolean (display, "show-estimated-end");
    config_show_title_due_date
        = g_settings_get_boolean (display, "show-due-date");
    config_show_title_sizing = g_settings_get_boolean (display, "show-sizing");
    config_show_title_percent_complete
        = g_settings_get_boolean (display, "show-percent-complete");
    config_show_title_urgency
        = g_settings_get_boolean (display, "show-urgency");
    config_show_title_importance
        = g_settings_get_boolean (display, "show-importance");
    config_show_title_status = g_settings_get_boolean (display, "show-status");

    prefs_update_projects_view ();

    g_object_unref (display);
    display = NULL;
  }

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

  // Actions ------------------------------------------------------------------
  {
    GSettings *actions = g_settings_get_child (settings, "actions");

    get_maybe_str (actions, "start-command", &config_shell_start);
    get_maybe_str (actions, "stop-command", &config_shell_stop);

    g_object_unref (actions);
    actions = NULL;
  }

  // Logfile ------------------------------------------------------------------
  {
    GSettings *logfile = g_settings_get_child (settings, "logfile");

    config_logfile_use = g_settings_get_boolean (logfile, "use");
    get_maybe_str (logfile, "filename", &config_logfile_name);
    get_maybe_str (logfile, "entry-start", &config_logfile_start);
    get_maybe_str (logfile, "entry-stop", &config_logfile_stop);
    config_logfile_min_secs = g_settings_get_int (logfile, "min-secs");

    g_object_unref (logfile);
    logfile = NULL;
  }

  /* ------------ */
  config_time_format = g_settings_get_int (settings, "time-format");

  // Report -------------------------------------------------------------------
  {
    GSettings *report = g_settings_get_child (settings, "report");

    get_str (report, "currency-symbol", &config_currency_symbol);
    config_currency_use_locale
        = g_settings_get_boolean (report, "currency-use-locale");

    g_object_unref (report);
    report = NULL;
  }

  // Data ---------------------------------------------------------------------
  {
    GSettings *data = g_settings_get_child (settings, "data");

    save_count = g_settings_get_int (data, "save-count");
    get_str (data, "url", &config_data_url);

    g_object_unref (data);
    data = NULL;
  }

  // CList --------------------------------------------------------------------
  {
    GSettings *clist = g_settings_get_child (settings, "clist");

    GSList *list = get_int_arr (clist, "column-widths");

    GSList *node = NULL;
    for (i = 0, node = list; node != NULL; node = node->next, i++)
      {
        num = (long)(node->data);
        if (-1 < num)
          {
            gtt_projects_tree_set_col_width (projects_tree, i, num);
          }
      }
    g_slist_free (list);
    list = NULL;

    g_object_unref (clist);
    clist = NULL;
  }

  /* Read in the user-defined report locations */
  gtt_restore_reports_menu (GNOME_APP (app_window));

  // Misc ---------------------------------------------------------------------
  {
    GSettings *misc = g_settings_get_child (settings, "misc");

    run_timer = g_settings_get_int (misc, "timer-running");
    /* Use string for time, to avoid unsigned-long problems */
    gchar *val = g_settings_get_string (misc, "last-timer");
    last_timer = (time_t)atol (val);
    g_free (val);
    val = NULL;

    g_object_unref (misc);
    misc = NULL;
  }

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

  GSettings *display = g_settings_get_child (settings, "display");

  gchar *expander_state = NULL;
  get_maybe_str (display, "expander-state", &expander_state);

  g_object_unref (display);
  display = NULL;

  return expander_state;
}

static GSList *
get_int_arr (GSettings *setts, const gchar *key)
{
  GVariant *value = g_settings_get_value (setts, key);

  GSList *res = NULL;

  const gsize n_children = g_variant_n_children (value);
  gsize i;
  for (i = 0; i < n_children; ++i)
    {
      gint val;
      g_variant_get_child (value, i, "i", &val);
      res = g_slist_prepend (res, GINT_TO_POINTER (val));
    }

  g_variant_unref (value);
  value = NULL;

  res = g_slist_reverse (res);

  return res;
}

static void
get_maybe_str (GSettings *const setts, const gchar *const key,
               gchar **const value)
{
  if (NULL != *value)
    {
      g_free (*value);
      *value = NULL;
    }

  GVariant *val = g_settings_get_value (setts, key);

  GVariant *str = g_variant_get_maybe (val);
  if (NULL != str)
    {
      gsize str_size;
      *value = g_strdup (g_variant_get_string (str, &str_size));
      g_variant_unref (str);
      str = NULL;
    }

  g_variant_unref (val);
  val = NULL;
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
set_int_arr (GSettings *setts, const gchar *key, GSList *value)
{
  GVariantBuilder builder;
  g_variant_builder_init (&builder, G_VARIANT_TYPE_ARRAY);

  GSList *node = NULL;
  for (node = value; node != NULL; node = node->next)
    {
      g_variant_builder_add (&builder, "i", GPOINTER_TO_INT (node->data));
    }

  GVariant *var = g_variant_builder_end (&builder);

  if (FALSE == g_settings_set_value (setts, key, var))
    {
      g_warning (_ ("Failed to set integer array GSettings option \"%s\""),
                 key);
    }
}

static void
set_maybe_str (GSettings *setts, const gchar *key, const gchar *const value)
{
  GVariantType *val_type = g_variant_type_new ("s");

  GVariant *str_val = NULL;
  if (NULL != value)
    {
      str_val = g_variant_new_string (value);
    }
  GVariant *val = g_variant_new_maybe (val_type, str_val);
  if (FALSE == g_settings_set_value (setts, key, val))
    {
      g_warning (_ ("Failed to set maybe string GSettings option \"%s\" to "
                    "value: \"%s\""),
                 key, (NULL != value) ? value : "<nothing>");
    }

  g_variant_type_free (val_type);
  val_type = NULL;
}

/* =========================== END OF FILE ========================= */
