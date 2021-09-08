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

#include <gio/gio.h>

static GSettings *gsettings = NULL;

static void set_int(GSettings *gsettings, const gchar *key, gint value);

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
}

void
gtt_gsettings_save()
{
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
}

static void
set_int(GSettings *gsettings, const gchar *key, gint value)
{
	if (!g_settings_set_int(gsettings, key, value))
	{
		g_warning("Failed to set integer value %d for key \"%s\"", value, key);
	}
}
