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

#include <gio/gio.h>

static GSettings *gsettings = NULL;

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
