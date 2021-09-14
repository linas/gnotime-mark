/*
 * GSettings configuration handling for GnoTime - a time tracker
 *
 * Copyright (C) 2021      Markus Prasser <markuspg@users.noreply.github.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "gsettings-io.h"
#include "app.h"
#include "prefs.h"
#include "toolbar.h"

#include <gio/gio.h>

int save_count = 0;

static GSettings *gsettings = NULL;

static GSList *get_array_int(GSettings *gsettings, const gchar *key);
static gchar *get_maybe_str(GSettings *gsettings, const gchar *key);
static void set_array_int(GSettings *gsettings, const gchar *key, GSList *ints);
static void set_bool(GSettings *gsettings, const gchar *key, gboolean value);
static void set_int(GSettings *gsettings, const gchar *key, gint value);
static void set_maybe_str(GSettings *gsettings, const gchar *key,
                          const gchar *value);
static void set_str(GSettings *gsettings, const gchar *key, const gchar *value);

gboolean
gtt_gsettings_init()
{
	if (NULL != gsettings)
	{
		g_warning("Duplicate initialization of the GSettings subsystem");
		return TRUE;
	}

	gsettings = g_settings_new("com.github.goedson.gnotime");
	if (NULL != gsettings)
	{
		return TRUE;
	}

	g_warning("Failed to initialize the GSettings subsystem");
	return FALSE;
}

void
gtt_gsettings_load()
{
	{
		GSettings *actions_settings = g_settings_get_child(gsettings, "actions");
		config_shell_start = get_maybe_str(actions_settings, "start-command");
		config_shell_stop = get_maybe_str(actions_settings, "stop-command");
		g_object_unref(actions_settings);
		actions_settings = NULL;
	}

	{
		GSettings *clist_settings = g_settings_get_child(gsettings, "clist");
		GSList *node, *list = get_array_int(clist_settings, "column-widths");
		gint i;
		for (i = 0, node = list; node != NULL; node = node->next, ++i)
		{
			const gint num = GPOINTER_TO_INT(node->data);
			if (-1 < num)
			{
				gtt_projects_tree_set_col_width(projects_tree, i, num);
			}
		}
		g_object_unref(clist_settings);
		clist_settings = NULL;
	}

	{
		GSettings *data_settings = g_settings_get_child(gsettings, "data");
		save_count = g_settings_get_int(data_settings, "save-count");
		config_data_url = g_settings_get_string(data_settings, "url");
		g_object_unref(data_settings);
		data_settings = NULL;
	}

	{
		GSettings *display_settings = g_settings_get_child(gsettings, "display");
		config_show_title_desc =
				g_settings_get_boolean(display_settings, "show-desc");
		config_show_title_due_date =
				g_settings_get_boolean(display_settings, "show-due-date");
		config_show_title_estimated_end =
				g_settings_get_boolean(display_settings, "show-estimated-end");
		config_show_title_estimated_start =
				g_settings_get_boolean(display_settings, "show-estimated-start");
		config_show_title_importance =
				g_settings_get_boolean(display_settings, "show-importance");
		config_show_title_percent_complete =
				g_settings_get_boolean(display_settings, "show-percent-complete");
		config_show_secs = g_settings_get_boolean(display_settings, "show-secs");
		prefs_set_show_secs();
		config_show_title_sizing =
				g_settings_get_boolean(display_settings, "show-sizing");
		config_show_title_status =
				g_settings_get_boolean(display_settings, "show-status");
		config_show_statusbar =
				g_settings_get_boolean(display_settings, "show-statusbar");
		config_show_subprojects =
				g_settings_get_boolean(display_settings, "show-sub-projects");
		config_show_clist_titles =
				g_settings_get_boolean(display_settings, "show-table-header");
		config_show_title_current =
				g_settings_get_boolean(display_settings, "show-time-current");
		config_show_title_day =
				g_settings_get_boolean(display_settings, "show-time-day");
		config_show_title_ever =
				g_settings_get_boolean(display_settings, "show-time-ever");
		config_show_title_lastweek =
				g_settings_get_boolean(display_settings, "show-time-last-week");
		config_show_title_month =
				g_settings_get_boolean(display_settings, "show-time-month");
		config_show_title_task =
				g_settings_get_boolean(display_settings, "show-task");
		config_show_title_week =
				g_settings_get_boolean(display_settings, "show-time-week");
		config_show_title_year =
				g_settings_get_boolean(display_settings, "show-time-year");
		config_show_title_yesterday =
				g_settings_get_boolean(display_settings, "show-time-yesterday");
		config_show_title_urgency =
				g_settings_get_boolean(display_settings, "show-urgency");
		g_object_unref(display_settings);
		display_settings = NULL;
	}
	prefs_update_projects_view();

	// Restore the main window height and width. Note that commandline flags
	// specified by the user override the values stored in the configuration
	{
		GSettings *geometry_settings = g_settings_get_child(gsettings, "geometry");
		if (FALSE == geom_place_override)
		{
			const gint x = g_settings_get_int(geometry_settings, "x");
			const gint y = g_settings_get_int(geometry_settings, "y");
			gtk_widget_set_uposition(GTK_WIDGET(app_window), x, y);
		}
		if (FALSE == geom_size_override)
		{
			const gint h = g_settings_get_int(geometry_settings, "height");
			const gint w = g_settings_get_int(geometry_settings, "width");
			gtk_window_set_default_size(GTK_WINDOW(app_window), w, h);
		}
		const gint hp = g_settings_get_int(geometry_settings, "h-paned");
		const gint vp = g_settings_get_int(geometry_settings, "v-paned");
		notes_area_set_pane_sizes(global_na, vp, hp);

		g_object_unref(geometry_settings);
		geometry_settings = NULL;
	}

	{
		GSettings *log_file_settings = g_settings_get_child(gsettings, "log-file");
		config_logfile_start = get_maybe_str(log_file_settings, "entry-start");
		config_logfile_stop = get_maybe_str(log_file_settings, "entry-stop");
		config_logfile_name = get_maybe_str(log_file_settings, "filename");
		config_logfile_min_secs = g_settings_get_int(log_file_settings, "min-secs");
		config_logfile_use = g_settings_get_boolean(log_file_settings, "use");
		g_object_unref(log_file_settings);
		log_file_settings = NULL;
	}

	{
		GSettings *report_settings = g_settings_get_child(gsettings, "report");
		config_currency_symbol =
				g_settings_get_string(report_settings, "currency-symbol");
		config_currency_use_locale =
				g_settings_get_boolean(report_settings, "currency-use-locale");
		g_object_unref(report_settings);
		report_settings = NULL;
	}

	const gboolean config_show_tb_ccp_before = config_show_tb_ccp;
	const gboolean config_show_tb_exit_before = config_show_tb_exit;
	const gboolean config_show_tb_help_before = config_show_tb_help;
	const gboolean config_show_tb_journal_before = config_show_tb_journal;
	const gboolean config_show_tb_new_before = config_show_tb_new;
	const gboolean config_show_tb_pref_before = config_show_tb_pref;
	const gboolean config_show_tb_prop_before = config_show_tb_prop;
	const gboolean config_show_tb_timer_before = config_show_tb_timer;
	const gboolean config_show_tb_tips_before = config_show_tb_tips;
	{
		GSettings *toolbar_settings = g_settings_get_child(gsettings, "toolbar");
		config_show_tb_ccp = g_settings_get_boolean(toolbar_settings, "show-ccp");
		config_show_tb_exit = g_settings_get_boolean(toolbar_settings, "show-exit");
		config_show_tb_help = g_settings_get_boolean(toolbar_settings, "show-help");
		config_show_tb_journal =
				g_settings_get_boolean(toolbar_settings, "show-journal");
		config_show_tb_new = g_settings_get_boolean(toolbar_settings, "show-new");
		config_show_tb_pref = g_settings_get_boolean(toolbar_settings, "show-pref");
		config_show_tb_prop = g_settings_get_boolean(toolbar_settings, "show-prop");
		config_show_tb_timer =
				g_settings_get_boolean(toolbar_settings, "show-timer");
		config_show_tb_tips = g_settings_get_boolean(toolbar_settings, "show-tips");
		config_show_toolbar =
				g_settings_get_boolean(toolbar_settings, "show-toolbar");
		g_object_unref(toolbar_settings);
		toolbar_settings = NULL;
	}

	if ((config_show_tb_ccp_before != config_show_tb_ccp) ||
	    (config_show_tb_exit_before != config_show_tb_exit) ||
	    (config_show_tb_help_before != config_show_tb_help) ||
	    (config_show_tb_journal_before != config_show_tb_journal) ||
	    (config_show_tb_new_before != config_show_tb_new) ||
	    (config_show_tb_pref_before != config_show_tb_pref) ||
	    (config_show_tb_prop_before != config_show_tb_prop) ||
	    (config_show_tb_timer_before != config_show_tb_timer) ||
	    (config_show_tb_tips_before != config_show_tb_tips))
	{
		update_toolbar_sections();
	}
}

void
gtt_gsettings_save()
{
	{
		GSettings *actions_settings = g_settings_get_child(gsettings, "actions");
		set_maybe_str(actions_settings, "start-command", config_shell_start);
		set_maybe_str(actions_settings, "stop-command", config_shell_stop);
		g_object_unref(actions_settings);
		actions_settings = NULL;
	}

	{
		GSettings *clist_settings = g_settings_get_child(gsettings, "clist");
		gint i, w;
		GSList *list = NULL;
		for (i = 0, w = 0; - 1 < w; i++)
		{
			w = gtt_projects_tree_get_col_width(projects_tree, i);
			if (0 > w)
			{
				break;
			}
			list = g_slist_prepend(list, GINT_TO_POINTER(w));
		}
		list = g_slist_reverse(list);
		set_array_int(clist_settings, "column-widths", list);
		g_slist_free(list);
		g_object_unref(clist_settings);
		clist_settings = NULL;
	}

	{
		GSettings *data_settings = g_settings_get_child(gsettings, "data");
		set_int(data_settings, "save-count", save_count);
		set_str(data_settings, "url", config_data_url);
		g_object_unref(data_settings);
		data_settings = NULL;
	}

	{
		GSettings *display_settings = g_settings_get_child(gsettings, "display");
		gchar *xpn = gtt_projects_tree_get_expander_state(projects_tree);
		set_str(display_settings, "expander-state", xpn);
		g_free(xpn);
		xpn = NULL;
		set_bool(display_settings, "show-desc", config_show_title_desc);
		set_bool(display_settings, "show-due-date", config_show_title_due_date);
		set_bool(display_settings, "show-estimated-end",
		         config_show_title_estimated_end);
		set_bool(display_settings, "show-estimated-start",
		         config_show_title_estimated_start);
		set_bool(display_settings, "show-importance", config_show_title_importance);
		set_bool(display_settings, "show-percent-complete",
		         config_show_title_percent_complete);
		set_bool(display_settings, "show-secs", config_show_secs);
		set_bool(display_settings, "show-sizing", config_show_title_sizing);
		set_bool(display_settings, "show-status", config_show_title_status);
		set_bool(display_settings, "show-statusbar", config_show_statusbar);
		set_bool(display_settings, "show-sub-projects", config_show_subprojects);
		set_bool(display_settings, "show-table-header", config_show_clist_titles);
		set_bool(display_settings, "show-task", config_show_title_task);
		set_bool(display_settings, "show-time-current", config_show_title_current);
		set_bool(display_settings, "show-time-day", config_show_title_day);
		set_bool(display_settings, "show-time-ever", config_show_title_ever);
		set_bool(display_settings, "show-time-last-week",
		         config_show_title_lastweek);
		set_bool(display_settings, "show-time-month", config_show_title_month);
		set_bool(display_settings, "show-time-week", config_show_title_week);
		set_bool(display_settings, "show-time-year", config_show_title_year);
		set_bool(display_settings, "show-time-yesterday",
		         config_show_title_yesterday);
		set_bool(display_settings, "show-urgency", config_show_title_urgency);
		g_object_unref(display_settings);
		display_settings = NULL;
	}
	// Save the window location and size
	{
		GSettings *geometry_settings = g_settings_get_child(gsettings, "geometry");
		gint h, hp, vp, w, x, y;
		gdk_window_get_origin(app_window->window, &x, &y);
		gdk_window_get_size(app_window->window, &w, &h);
		notes_area_get_pane_sizes(global_na, &vp, &hp);
		set_int(geometry_settings, "height", h);
		set_int(geometry_settings, "h-paned", hp);
		set_int(geometry_settings, "v-paned", vp);
		set_int(geometry_settings, "width", w);
		set_int(geometry_settings, "x", x);
		set_int(geometry_settings, "y", y);
		g_object_unref(geometry_settings);
		geometry_settings = NULL;
	}

	{
		GSettings *log_file_settings = g_settings_get_child(gsettings, "log-file");
		set_maybe_str(log_file_settings, "entry-start", config_logfile_start);
		set_maybe_str(log_file_settings, "entry-stop", config_logfile_stop);
		set_maybe_str(log_file_settings, "filename", config_logfile_name);
		set_int(log_file_settings, "min-secs", config_logfile_min_secs);
		set_bool(log_file_settings, "use", config_logfile_use);
		g_object_unref(log_file_settings);
		log_file_settings = NULL;
	}

	{
		GSettings *report_settings = g_settings_get_child(gsettings, "report");
		set_str(report_settings, "currency-symbol", config_currency_symbol);
		set_bool(report_settings, "currency-use-locale",
		         config_currency_use_locale);
		g_object_unref(report_settings);
		report_settings = NULL;
	}

	{
		GSettings *toolbar_settings = g_settings_get_child(gsettings, "toolbar");
		set_bool(toolbar_settings, "show-ccp", config_show_tb_ccp);
		set_bool(toolbar_settings, "show-exit", config_show_tb_exit);
		set_bool(toolbar_settings, "show-help", config_show_tb_help);
		set_bool(toolbar_settings, "show-new", config_show_tb_new);
		set_bool(toolbar_settings, "show-journal", config_show_tb_journal);
		set_bool(toolbar_settings, "show-pref", config_show_tb_pref);
		set_bool(toolbar_settings, "show-prop", config_show_tb_prop);
		set_bool(toolbar_settings, "show-timer", config_show_tb_timer);
		set_bool(toolbar_settings, "show-tips", config_show_tb_tips);
		set_bool(toolbar_settings, "show-toolbar", config_show_toolbar);
		g_object_unref(toolbar_settings);
		toolbar_settings = NULL;
	}
}

static GSList *
get_array_int(GSettings *const gsettings, const gchar *const key)
{
	GSList *items = NULL;

	GVariant *container = g_settings_get_value(gsettings, key);

	gsize length;
	gconstpointer data =
			g_variant_get_fixed_array(container, &length, sizeof(gint));
	const gint *ints = data;
	gsize i = 0;
	for (; i < length; ++i)
	{
		items = g_slist_prepend(items, GINT_TO_POINTER(ints[i]));
	}
	items = g_slist_reverse(items);

	g_variant_unref(container);
	container = NULL;

	return items;
}

static gchar *
get_maybe_str(GSettings *const gsettings, const gchar *const key)
{
	gchar *res = NULL;

	GVariant *outer = g_settings_get_value(gsettings, key);

	GVariant *maybe = g_variant_get_maybe(outer);
	if (NULL != maybe)
	{
		res = g_strdup(g_variant_get_string(maybe, NULL));
		g_variant_unref(maybe);
		maybe = NULL;
	}

	g_variant_unref(outer);
	outer = NULL;

	return res;
}

static void
set_array_int(GSettings *const gsettings, const gchar *const key, GSList *ints)
{
	const guint list_size = g_slist_length(ints);
	GVariant **data = g_new0(GVariant *, list_size + 1);
	guint i;
	for (i = 0; i < list_size; ++i)
	{
		data[i] = g_variant_new_int32(GPOINTER_TO_INT(g_slist_nth(ints, i)));
	}
	GVariantType *int_type = g_variant_type_new("i");
	GVariant *val = g_variant_new_array(int_type, data, list_size);

	if (!g_settings_set_value(gsettings, key, val))
	{
		g_warning("Failed to set integer array value for key \"%s\"", key);
	}
	val = NULL;

	g_variant_type_free(int_type);
	int_type = NULL;

	g_free(data);
	data = NULL;
}

static void
set_bool(GSettings *const gsettings, const gchar *const key,
         const gboolean value)
{
	if (!g_settings_set_boolean(gsettings, key, value))
	{
		g_warning("Failed to set boolean value %d for key \"%s\"", value, key);
	}
}

static void
set_int(GSettings *const gsettings, const gchar *const key, const gint value)
{
	if (!g_settings_set_int(gsettings, key, value))
	{
		g_warning("Failed to set integer value %d for key \"%s\"", value, key);
	}
}

static void
set_maybe_str(GSettings *const gsettings, const gchar *const key,
              const gchar *const value)
{
	if (!g_settings_set(gsettings, key, "ms", value))
	{
		g_warning("Failed to set maybe string value \"%s\" for key \"%s\"",
		          value ? value : "nothing", key);
	}
}

static void
set_str(GSettings *const gsettings, const gchar *const key,
        const gchar *const value)
{
	if (!g_settings_set_string(gsettings, key, value))
	{
		g_warning("Failed to set string value \"%s\" for key \"%s\"", value, key);
	}
}
