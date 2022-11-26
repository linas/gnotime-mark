/* GnoTime - a to-do list organizer, diary and billing system
 * Copyright (C) 2022      Markus Prasser
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "gtt_gsettings_io_p.h"

void
gtt_gsettings_get_str (GSettings *const settings, const gchar *const key,
                       gchar **const value)
{
  if (NULL != *value)
    {
      g_free (*value);
      *value = NULL;
    }

  *value = g_settings_get_string (settings, key);
}

void
gtt_gsettings_set_bool (GSettings *const settings, const gchar *const key,
                        const gboolean value)
{
  if (G_UNLIKELY (FALSE == g_settings_set_boolean (settings, key, value)))
    {
      g_warning ("Failed to set GSettings boolean option \"%s\" to value: %s",
                 key, value ? "true" : "false");
    }
}

void
gtt_gsettings_set_int (GSettings *const settings, const gchar *const key,
                       const gint value)
{
  if (G_UNLIKELY (FALSE == g_settings_set_int (settings, key, value)))
    {
      g_warning ("Failed to set GSettings integer option \"%s\" to value: %d",
                 key, value);
    }
}

void
gtt_gsettings_set_str (GSettings *const settings, const gchar *const key,
                       const gchar *const value)
{
  if (G_UNLIKELY (FALSE == g_settings_set_string (settings, key, value)))
    {
      g_warning (
          "Failed to set GSettings string option \"%s\" to value: \"%s\"", key,
          value);
    }
}
