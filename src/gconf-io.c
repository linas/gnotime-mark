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

/* TODO: Get rid of this global state */
static GSettings *global_gsettings = NULL;

#define GTT_GCONF "/apps/gnotime"

static gchar *get_nullable_str(GSettings *gsettings, const char *key);
static void gtt_restore_reports_menu(GnomeApp *app, GSettings *gsettings);
static void set_bool(GSettings *gsettings, const gchar *key, gboolean value);
static void set_int(GSettings *gsettings, const char *key, gint value);
static void set_nullable_str(GSettings *gsettings, const char *key,
                             const gchar *value);
static void set_str(GSettings *gsettings, const char *key, const gchar *value);

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
	GSettings *gsettings_misc = g_settings_get_child(global_gsettings, "misc");
	g_settings_set_int(gsettings_misc, "num-reports", i);
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
	GSettings *gsettings_geometry = g_settings_get_child(gsettings, "geometry");
	set_int(gsettings_geometry, "width", w);
	set_int(gsettings_geometry, "height", h);
	set_int(gsettings_geometry, "x", x);
	set_int(gsettings_geometry, "y", y);

	{
		int vp, hp;
		notes_area_get_pane_sizes(global_na, &vp, &hp);
		set_int(gsettings_geometry, "v-paned", vp);
		set_int(gsettings_geometry, "h-paned", hp);
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
	set_nullable_str(gsettings_display, "expander-state", xpn);

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
	GSettings *gsettings_actions = g_settings_get_child(gsettings, "actions");
	set_nullable_str(gsettings_actions, "start-command", config_shell_start);
	set_nullable_str(gsettings_actions, "stop-command", config_shell_stop);

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
			list = g_slist_prepend(list, g_variant_new_int32(w));
		}

		list = g_slist_reverse(list);
		const guint list_length = g_slist_length(list);
		GVariantType *int_array_variant_type = g_variant_type_new("ai");
		GVariantBuilder *builder = g_variant_builder_new(int_array_variant_type);
		for (i = 0; i < list_length; ++i)
		{
			g_variant_builder_add_value(builder, g_slist_nth(list, i)->data);
		}
		GSettings *gsettings_clist = g_settings_get_child(gsettings, "clist");
		GVariant *val_arr = g_variant_builder_end(builder);
		g_settings_set_value(gsettings_clist, "column-widths", val_arr);
		g_slist_free(list);
		g_variant_builder_unref(builder);
		builder = NULL;
		g_variant_type_free(int_array_variant_type);
		int_array_variant_type = NULL;
	}

	/* ------------- */
	/* Use string for time, to avoid integer conversion problems */
	g_snprintf(s, sizeof(s), "%ld", time(0));
	GSettings *gsettings_misc = g_settings_get_child(gsettings, "misc");
	set_str(gsettings_misc, "last-timer", s);
	set_int(gsettings_misc, "idle-timeout", config_idle_timeout);
	set_int(gsettings_misc, "no-project-timeout", config_no_project_timeout);
	set_int(gsettings_misc, "autosave-period", config_autosave_period);
	set_int(gsettings_misc, "timer-running", timer_is_running());
	set_int(gsettings_misc, "curr-project", gtt_project_get_id(cur_proj));
	g_settings_set_int(gsettings_misc, "num-projects", -1);

	set_int(gsettings_misc, "day-start-offset", config_daystart_offset);
	set_int(gsettings_misc, "week-start-offset", config_weekstart_offset);

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

static void
gtt_restore_reports_menu(GnomeApp *app, GSettings *gsettings)
{
	int i, num;
	char s[120], *p;
	GnomeUIInfo *reports_menu;
	GConfClient *client;

	client = gconf_client_get_default();

	/* Read in the user-defined report locations */
	GSettings *gsettings_misc = g_settings_get_child(gsettings, "misc");
	num = g_settings_get_int(gsettings_misc, "num-reports");
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
	global_gsettings = gsettings;
	int i;
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
	GSettings *gsettings_misc = g_settings_get_child(gsettings, "misc");
	g_settings_get_int(gsettings_misc, "curr-project");

	config_idle_timeout = g_settings_get_int(gsettings_misc, "idle-timeout");
	config_no_project_timeout =
			g_settings_get_int(gsettings_misc, "no-project-timeout");
	config_autosave_period =
			g_settings_get_int(gsettings_misc, "autosave-period");
	config_daystart_offset =
			g_settings_get_int(gsettings_misc, "day-start-offset");
	config_weekstart_offset =
			g_settings_get_int(gsettings_misc, "week-start-offset");

	GSettings *gsettings_geometry = g_settings_get_child(gsettings, "geometry");
	/* Reset the main window width and height to the values
	 * last stored in the config file.  Note that if the user
	 * specified command-line flags, then the command line
	 * over-rides the config file. */
	if (!geom_place_override)
	{
		int x, y;
		x = g_settings_get_int(gsettings_geometry, "x");
		y = g_settings_get_int(gsettings_geometry, "y");
		gtk_widget_set_uposition(GTK_WIDGET(app_window), x, y);
	}
	if (!geom_size_override)
	{
		int w, h;
		w = g_settings_get_int(gsettings_geometry, "width");
		h = g_settings_get_int(gsettings_geometry, "height");

		gtk_window_set_default_size(GTK_WINDOW(app_window), w, h);
	}

	{
		int vp, hp;
		vp = g_settings_get_int(gsettings_geometry, "v-paned");
		hp = g_settings_get_int(gsettings_geometry, "h-paned");
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
	GSettings *gsettings_actions = g_settings_get_child(gsettings, "actions");
	config_shell_start = get_nullable_str(gsettings_actions, "start-command");
	config_shell_stop = get_nullable_str(gsettings_actions, "stop-command");

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
		GSettings *gsettings_clist = g_settings_get_child(gsettings, "clist");
		GVariant *col_widths =
				g_settings_get_value(gsettings_clist, "column-widths");
		gsize n_col_widths = g_variant_n_children(col_widths);
		for (i = 0; i < n_col_widths; ++i)
		{
			gint col_width;
			g_variant_get_child(col_widths, i, "i", &col_width);
			gtt_projects_tree_set_col_width(projects_tree, i, col_width);
		}
		g_variant_unref(col_widths);
		col_widths = NULL;
	}

	/* Read in the user-defined report locations */
	gtt_restore_reports_menu(GNOME_APP(app_window), gsettings);

	run_timer = g_settings_get_int(gsettings_misc, "timer-running");
	/* Use string for time, to avoid unsigned-long problems */
	gchar *tmp_timer = g_settings_get_string(gsettings_misc, "last-timer");
	last_timer = (time_t)atol(tmp_timer);
	g_free(tmp_timer);
	tmp_timer = NULL;

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
gtt_gconf_get_expander(GSettings *gsettings)
{
	GSettings *gsettings_display = g_settings_get_child(gsettings, "display");
	return get_nullable_str(gsettings_display, "expander-state");
}

static gchar *
get_nullable_str(GSettings *gsettings, const char *key)
{
	GVariant *outer_val = g_settings_get_value(gsettings, key);
	GVariant *nullable_val = g_variant_get_maybe(outer_val);
	gchar *ret_str = NULL;
	if (nullable_val)
	{
		gsize str_size;
		ret_str = g_variant_dup_string(nullable_val, &str_size);
	}
	g_variant_unref(nullable_val);
	nullable_val = NULL;
	g_variant_unref(outer_val);
	outer_val = NULL;

	return ret_str;
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

static void
set_int(GSettings *gsettings, const char *key, gint value)
{
	const gboolean rc = g_settings_set_int(gsettings, key, value);

	if (FALSE == rc)
	{
		printf("Failed to set GSettings option: %s\n", key);
	}
}

static void
set_nullable_str(GSettings *gsettings, const char *key, const gchar *value)
{
	GVariant *new_val = g_variant_new("ms", value);
	const gboolean rc = g_settings_set_value(gsettings, key, new_val);
	g_variant_unref(new_val);
	new_val = NULL;

	if (FALSE == rc)
	{
		printf("Failed to set GSettings option: %s\n", key);
	}
}

static void
set_str(GSettings *gsettings, const char *key, const gchar *value)
{
	const gboolean rc = g_settings_set_string(gsettings, key, value);

	if (FALSE == rc)
	{
		printf("Failed to set GSettings option: %s\n", key);
	}
}
