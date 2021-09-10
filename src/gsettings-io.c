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

static gchar *get_maybe_str(GSettings *gsettings, const gchar *key);
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
		GSettings *data_settings = g_settings_get_child(gsettings, "data");
		save_count = g_settings_get_int(data_settings, "save-count");
		config_data_url = g_settings_get_string(data_settings, "url");
		g_object_unref(data_settings);
		data_settings = NULL;
	}

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
		GSettings *data_settings = g_settings_get_child(gsettings, "data");
		set_int(data_settings, "save-count", save_count);
		set_str(data_settings, "url", config_data_url);
		g_object_unref(data_settings);
		data_settings = NULL;
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
