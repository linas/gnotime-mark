/*   GConf2 input/output handling for GTimeTracker - a time tracker
 *   Copyright (C) 2003 Linas Vepstas <linas@linas.org>
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

#include <gconf/gconf-client.h>
#include <gconf/gconf.h>
#include <glib.h>
#include <gnome.h>

#include "app.h"
#include "cur-proj.h"
#include "gconf-gnomeui.h"
#include "gconf-io-p.h"
#include "gconf-io.h"
#include "gtt.h"
#include "menus.h"
#include "plug-in.h"
#include "prefs.h"
#include "timer.h"
#include "toolbar.h"

/* XXX these should not be externs, they should be part of
 * some app-global structure.
 */
extern int save_count;         /* XXX */
extern char *first_proj_title; /* command line flag */
extern time_t last_timer;      /* XXX */
extern int cur_proj_id;
extern int run_timer;

#define GTT_GCONF "/apps/gnotime"

static void set_bool(GSettings *gsettings, const gchar *key, gboolean value);

/* ======================================================= */

void
gtt_save_reports_menu(void)
{
	int i;
	char s[120], *p;
	GnomeUIInfo *reports_menu;
	GConfClient *client;

	client = gconf_client_get_default();
	reports_menu = gtt_get_reports_menu();

	/* Write out the customer report info */
	for (i = 0; GNOME_APP_UI_ENDOFINFO != reports_menu[i].type; i++)
	{
		GttPlugin *plg = reports_menu[i].user_data;
		g_snprintf(s, sizeof(s), GTT_GCONF "/Reports/%d/", i);
		p = s + strlen(s);
		strcpy(p, "Name");
		F_SETSTR(s, plg->name);

		strcpy(p, "Path");
		F_SETSTR(s, plg->path);

		strcpy(p, "Tooltip");
		F_SETSTR(s, plg->tooltip);

		strcpy(p, "LastSaveURL");
		F_SETSTR(s, plg->last_url);

		*p = 0;
		gtt_save_gnomeui_to_gconf(client, s, &reports_menu[i]);
	}
	SETINT("/Misc/NumReports", i);
}

/* ======================================================= */
/* Save only the GUI configuration info, not the actual data */
/* XXX fixme -- this should really use GConfChangeSet */

void
gtt_gconf_save(GSettings *gsettings)
{
	char s[120];
	int x, y, w, h;
	const char *xpn;

	GConfEngine *gengine;
	GConfClient *client;

	gengine = gconf_engine_get_default();
	client = gconf_client_get_for_engine(gengine);
	SETINT("/dir_exists", 1);

	/* ------------- */
	/* save the window location and size */
	gdk_window_get_origin(app_window->window, &x, &y);
	gdk_window_get_size(app_window->window, &w, &h);
	SETINT("/Geometry/Width", w);
	SETINT("/Geometry/Height", h);
	SETINT("/Geometry/X", x);
	SETINT("/Geometry/Y", y);

	{
		int vp, hp;
		notes_area_get_pane_sizes(global_na, &vp, &hp);
		SETINT("/Geometry/VPaned", vp);
		SETINT("/Geometry/HPaned", hp);
	}
	/* ------------- */
	/* save the configure dialog values */
	GSettings *gsettings_display = g_settings_get_child(gsettings, "display");
	set_bool(gsettings_display, "show-secs", config_show_secs);
	set_bool(gsettings_display, "show-statusbar", config_show_statusbar);
	set_bool(gsettings_display, "show-sub-projects", config_show_subprojects);
	set_bool(gsettings_display, "show-table-header", config_show_clist_titles);
	set_bool(gsettings_display, "show-time-current", config_show_title_current);
	set_bool(gsettings_display, "show-time-day", config_show_title_day);
	set_bool(gsettings_display, "show-time-yesterday",
	         config_show_title_yesterday);
	set_bool(gsettings_display, "show-time-week", config_show_title_week);
	set_bool(gsettings_display, "show-time-last-week",
	         config_show_title_lastweek);
	set_bool(gsettings_display, "show-time-month", config_show_title_month);
	set_bool(gsettings_display, "show-time-year", config_show_title_year);
	set_bool(gsettings_display, "show-time-ever", config_show_title_ever);
	set_bool(gsettings_display, "show-desc", config_show_title_desc);
	set_bool(gsettings_display, "show-task", config_show_title_task);
	set_bool(gsettings_display, "show-estimated-start",
	         config_show_title_estimated_start);
	set_bool(gsettings_display, "show-estimated-end",
	         config_show_title_estimated_end);
	set_bool(gsettings_display, "show-due-date", config_show_title_due_date);
	set_bool(gsettings_display, "show-sizing", config_show_title_sizing);
	set_bool(gsettings_display, "show-percent-complete",
	         config_show_title_percent_complete);
	set_bool(gsettings_display, "show-urgency", config_show_title_urgency);
	set_bool(gsettings_display, "show-importance", config_show_title_importance);
	set_bool(gsettings_display, "show-status", config_show_title_status);

	xpn = gtt_projects_tree_get_expander_state(projects_tree);
	SETSTR("/Display/ExpanderState", xpn);

	/* ------------- */
	GSettings *gsettings_toolbar = g_settings_get_child(gsettings, "toolbar");
	set_bool(gsettings_toolbar, "show-toolbar", config_show_toolbar);
	set_bool(gsettings_toolbar, "show-tips", config_show_tb_tips);
	set_bool(gsettings_toolbar, "show-new", config_show_tb_new);
	set_bool(gsettings_toolbar, "show-ccp", config_show_tb_ccp);
	set_bool(gsettings_toolbar, "show-journal", config_show_tb_journal);
	set_bool(gsettings_toolbar, "show-prop", config_show_tb_prop);
	set_bool(gsettings_toolbar, "show-timer", config_show_tb_timer);
	set_bool(gsettings_toolbar, "show-pref", config_show_tb_pref);
	set_bool(gsettings_toolbar, "show-help", config_show_tb_help);
	set_bool(gsettings_toolbar, "show-exit", config_show_tb_exit);

	/* ------------- */
	if (config_shell_start)
	{
		SETSTR("/Actions/StartCommand", config_shell_start);
	} else
	{
		UNSET("/Actions/StartCommand");
	}

	if (config_shell_stop)
	{
		SETSTR("/Actions/StopCommand", config_shell_stop);
	} else
	{
		UNSET("/Actions/StopCommand");
	}

	/* ------------- */
	GSettings *gsettings_log = g_settings_get_child(gsettings, "log-file");
	set_bool(gsettings_log, "use", config_logfile_use);
	if (config_logfile_name)
	{
		SETSTR("/LogFile/Filename", config_logfile_name);
	} else
	{
		UNSET("/LogFile/Filename");
	}

	if (config_logfile_start)
	{
		SETSTR("/LogFile/EntryStart", config_logfile_start);
	} else
	{
		SETSTR("/LogFile/EntryStart", "");
	}

	if (config_logfile_stop)
	{
		SETSTR("/LogFile/EntryStop", config_logfile_stop);
	} else
	{
		SETSTR("/LogFile/EntryStop", "");
	}

	SETINT("/LogFile/MinSecs", config_logfile_min_secs);

	/* ------------- */
	SETSTR("/Data/URL", config_data_url);
	SETINT("/Data/SaveCount", save_count);

	/* ------------- */
	{
		long i, w;
		GSList *list = NULL;
		for (i = 0, w = 0; - 1 < w; i++)
		{
			w = gtt_projects_tree_get_col_width(projects_tree, i);
			if (0 > w)
				break;
			list = g_slist_prepend(list, (gpointer)w);
		}
		list = g_slist_reverse(list);
		SETLIST("/CList/ColumnWidths", GCONF_VALUE_INT, list);
		g_slist_free(list);
	}

	/* ------------- */
	/* Use string for time, to avoid integer conversion problems */
	g_snprintf(s, sizeof(s), "%ld", time(0));
	SETSTR("/Misc/LastTimer", s);
	SETINT("/Misc/IdleTimeout", config_idle_timeout);
	SETINT("/Misc/NoProjectTimeout", config_no_project_timeout);
	SETINT("/Misc/AutosavePeriod", config_autosave_period);
	SETINT("/Misc/TimerRunning", timer_is_running());
	SETINT("/Misc/CurrProject", gtt_project_get_id(cur_proj));
	SETINT("/Misc/NumProjects", -1);

	SETINT("/Misc/DayStartOffset", config_daystart_offset);
	SETINT("/Misc/WeekStartOffset", config_weekstart_offset);

	SETINT("/time_format", config_time_format);

	SETSTR("/Report/CurrencySymbol", config_currency_symbol);
	GSettings *gsettings_report = g_settings_get_child(gsettings, "report");
	set_bool(gsettings_report, "currency-use-locale", config_currency_use_locale);

	/* Write out the user's report menu structure */
	gtt_save_reports_menu();

	/* Sync to file.
	 * XXX if this fails, the error is serious, and there should be a
	 * graphical popup.
	 */
	{
		GError *err_ret = NULL;
		gconf_client_suggest_sync(client, &err_ret);
		if (NULL != err_ret)
		{
			printf("GTT: GConf: Sync Failed\n");
		}
	}
}

/* ======================================================= */

gboolean
gtt_gconf_exists(void)
{
	GError *err_ret = NULL;
	GConfClient *client;
	GConfValue *gcv;

	/* Calling gconf_engine_dir_exists() on a non-existant directory
	 * completely hoses that directory for future use. Its Badddd.
	 * rc = gconf_engine_dir_exists (gengine, GTT_GCONF, &err_ret);
	 * gconf_client_dir_exists() is no better.
	 * Actually, the bug is that the dirs are unusable only while
	 * gconf is still running. Upon reboot, its starts working OK.
	 * Hack around it by trying to fetch a key.
	 */

	client = gconf_client_get_default();
	gcv = gconf_client_get(client, GTT_GCONF "/dir_exists", &err_ret);
	if ((NULL == gcv) || (FALSE == GCONF_VALUE_TYPE_VALID(gcv->type)))
	{
		if (err_ret)
			printf("GTT: Error: gconf_exists XXX err %s\n", err_ret->message);
		return FALSE;
	}

	return TRUE;
}

/* ======================================================= */

void
gtt_restore_reports_menu(GnomeApp *app)
{
	int i, num;
	char s[120], *p;
	GnomeUIInfo *reports_menu;
	GConfClient *client;

	client = gconf_client_get_default();

	/* Read in the user-defined report locations */
	num = GETINT("/Misc/NumReports", 0);
	reports_menu = g_new0(GnomeUIInfo, num + 1);

	for (i = 0; i < num; i++)
	{
		GttPlugin *plg;
		const char *name, *path, *tip, *url;

		g_snprintf(s, sizeof(s), GTT_GCONF "/Reports/%d/", i);
		p = s + strlen(s);

		strcpy(p, "Name");
		name = F_GETSTR(s, "");

		strcpy(p, "Path");
		path = F_GETSTR(s, "");

		strcpy(p, "Tooltip");
		tip = F_GETSTR(s, "");

		strcpy(p, "LastSaveURL");
		url = F_GETSTR(s, "");

		plg = gtt_plugin_new(name, path);
		plg->tooltip = g_strdup(tip);
		plg->last_url = g_strdup(url);

		*p = 0;
		gtt_restore_gnomeui_from_gconf(client, s, &reports_menu[i]);

		/* fixup */
		reports_menu[i].user_data = plg;
	}
	reports_menu[i].type = GNOME_APP_UI_ENDOFINFO;

	gtt_set_reports_menu(app, reports_menu);
}

/* ======================================================= */

void
gtt_gconf_load(GSettings *gsettings)
{
	int i, num;
	int _n, _c, _j, _p, _t, _o, _h, _e;
	GConfClient *client;

	client = gconf_client_get_default();
	gconf_client_add_dir(client, GTT_GCONF, GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);

	/* If already running, and we are over-loading a new file,
	 * then save the currently running project, and try to set it
	 * running again ... */
	if (gtt_project_get_title(cur_proj) && (!first_proj_title))
	{
		/* We need to strdup because title is freed when
		 * the project list is destroyed ... */
		first_proj_title = g_strdup(gtt_project_get_title(cur_proj));
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
	cur_proj_id = GETINT("/Misc/CurrProject", -1);

	config_idle_timeout = GETINT("/Misc/IdleTimeout", 300);
	config_no_project_timeout = GETINT("/Misc/NoProjectTimeout", 300);
	config_autosave_period = GETINT("/Misc/AutosavePeriod", 60);
	config_daystart_offset = GETINT("/Misc/DayStartOffset", 0);
	config_weekstart_offset = GETINT("/Misc/WeekStartOffset", 0);

	/* Reset the main window width and height to the values
	 * last stored in the config file.  Note that if the user
	 * specified command-line flags, then the command line
	 * over-rides the config file. */
	if (!geom_place_override)
	{
		int x, y;
		x = GETINT("/Geometry/X", 10);
		y = GETINT("/Geometry/Y", 10);
		gtk_widget_set_uposition(GTK_WIDGET(app_window), x, y);
	}
	if (!geom_size_override)
	{
		int w, h;
		w = GETINT("/Geometry/Width", 442);
		h = GETINT("/Geometry/Height", 272);

		gtk_window_set_default_size(GTK_WINDOW(app_window), w, h);
	}

	{
		int vp, hp;
		vp = GETINT("/Geometry/VPaned", 250);
		hp = GETINT("/Geometry/HPaned", 220);
		notes_area_set_pane_sizes(global_na, vp, hp);
	}

	GSettings *gsettings_display = g_settings_get_child(gsettings, "display");
	config_show_secs = g_settings_get_boolean(gsettings_display, "show-secs");

	prefs_set_show_secs();

	config_show_clist_titles =
			g_settings_get_boolean(gsettings_display, "show-table-header");
	config_show_subprojects =
			g_settings_get_boolean(gsettings_display, "show-sub-projects");
	config_show_statusbar =
			g_settings_get_boolean(gsettings_display, "show-statusbar");

	config_show_title_ever =
			g_settings_get_boolean(gsettings_display, "show-time-ever");
	config_show_title_day =
			g_settings_get_boolean(gsettings_display, "show-time-day");
	config_show_title_yesterday =
			g_settings_get_boolean(gsettings_display, "show-time-yesterday");
	config_show_title_week =
			g_settings_get_boolean(gsettings_display, "show-time-week");
	config_show_title_lastweek =
			g_settings_get_boolean(gsettings_display, "show-time-last-week");
	config_show_title_month =
			g_settings_get_boolean(gsettings_display, "show-time-month");
	config_show_title_year =
			g_settings_get_boolean(gsettings_display, "show-time-year");
	config_show_title_current =
			g_settings_get_boolean(gsettings_display, "show-time-yesterday");
	config_show_title_desc =
			g_settings_get_boolean(gsettings_display, "show-desc");
	config_show_title_task =
			g_settings_get_boolean(gsettings_display, "show-task");
	config_show_title_estimated_start =
			g_settings_get_boolean(gsettings_display, "show-estimated-start");
	config_show_title_estimated_end =
			g_settings_get_boolean(gsettings_display, "show-estimated-end");
	config_show_title_due_date =
			g_settings_get_boolean(gsettings_display, "show-due-date");
	config_show_title_sizing =
			g_settings_get_boolean(gsettings_display, "show-sizing");
	config_show_title_percent_complete =
			g_settings_get_boolean(gsettings_display, "show-percent-complete");
	config_show_title_urgency =
			g_settings_get_boolean(gsettings_display, "show-urgency");
	config_show_title_importance =
			g_settings_get_boolean(gsettings_display, "show-importance");
	config_show_title_status =
			g_settings_get_boolean(gsettings_display, "show-status");

	prefs_update_projects_view();

	/* ------------ */
	GSettings *gsettings_toolbar = g_settings_get_child(gsettings, "toolbar");
	config_show_toolbar =
			g_settings_get_boolean(gsettings_toolbar, "show-toolbar");
	config_show_tb_tips = g_settings_get_boolean(gsettings_toolbar, "show-tips");
	config_show_tb_new = g_settings_get_boolean(gsettings_toolbar, "show-new");
	config_show_tb_ccp = g_settings_get_boolean(gsettings_toolbar, "show-ccp");
	config_show_tb_journal =
			g_settings_get_boolean(gsettings_toolbar, "show-journal");
	config_show_tb_prop = g_settings_get_boolean(gsettings_toolbar, "show-prop");
	config_show_tb_timer =
			g_settings_get_boolean(gsettings_toolbar, "show-timer");
	config_show_tb_pref = g_settings_get_boolean(gsettings_toolbar, "show-pref");
	config_show_tb_help = g_settings_get_boolean(gsettings_toolbar, "show-help");
	config_show_tb_exit = g_settings_get_boolean(gsettings_toolbar, "show-exit");

	/* ------------ */
	config_shell_start =
			GETSTR("/Actions/StartCommand", "echo start id=%D \\\"%t\\\"-\\\"%d\\\" "
	                                    "%T  %H-%M-%S hours=%h min=%m secs=%s");
	config_shell_stop =
			GETSTR("/Actions/StopCommand", "echo stop id=%D \\\"%t\\\"-\\\"%d\\\" %T "
	                                   " %H-%M-%S hours=%h min=%m secs=%s");

	/* ------------ */
	GSettings *gsettings_log = g_settings_get_child(gsettings, "log-file");
	config_logfile_use = g_settings_get_boolean(gsettings_log, "use");
	config_logfile_name = GETSTR("/LogFile/Filename", NULL);
	config_logfile_start = GETSTR("/LogFile/EntryStart", _("project %t started"));
	config_logfile_stop = GETSTR("/LogFile/EntryStop", _("stopped project %t"));
	config_logfile_min_secs = GETINT("/LogFile/MinSecs", 3);

	/* ------------ */
	config_time_format = GETINT("/time_format", 3);

	GSettings *gsettings_report = g_settings_get_child(gsettings, "report");
	config_currency_symbol = GETSTR("/Report/CurrencySymbol", "$");
	config_currency_use_locale =
			g_settings_get_boolean(gsettings_report, "currency-use-locale");
	/* ------------ */
	save_count = GETINT("/Data/SaveCount", 0);
	config_data_url = GETSTR("/Data/URL", XML_DATA_FILENAME);

	/* ------------ */
	{
		GSList *node, *list = GETINTLIST("/CList/ColumnWidths");
		for (i = 0, node = list; node != NULL; node = node->next, i++)
		{
			num = (long)(node->data);
			if (-1 < num)
			{
				gtt_projects_tree_set_col_width(projects_tree, i, num);
			}
		}
	}

	/* Read in the user-defined report locations */
	gtt_restore_reports_menu(GNOME_APP(app_window));

	run_timer = GETINT("/Misc/TimerRunning", 0);
	/* Use string for time, to avoid unsigned-long problems */
	last_timer = (time_t)atol(GETSTR("/Misc/LastTimer", "-1"));

	/* redraw the GUI */
	if (config_show_statusbar)
	{
		gtk_widget_show(status_bar);
	} else
	{
		gtk_widget_hide(status_bar);
	}

	update_status_bar();
	if ((_n != config_show_tb_new) || (_c != config_show_tb_ccp) ||
	    (_j != config_show_tb_journal) || (_p != config_show_tb_prop) ||
	    (_t != config_show_tb_timer) || (_o != config_show_tb_pref) ||
	    (_h != config_show_tb_help) || (_e != config_show_tb_exit))
	{
		update_toolbar_sections();
	}
}

gchar *
gtt_gconf_get_expander(void)
{
	GConfClient *client = gconf_client_get_default();
	return GETSTR("/Display/ExpanderState", NULL);
}

static void
set_bool(GSettings *gsettings, const gchar *key, gboolean value)
{
	const gboolean rc = g_settings_set_boolean(gsettings, key, value);

	if (FALSE == rc)
	{
		printf("Failed to set GSettings option: %s\n", key);
	}
}
