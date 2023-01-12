/* GConf2 input/output handling for GTimeTracker - a time tracker
 * Copyright (C) 2023      Markus Prasser
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "gtt_gsettings_io_p.h"

GSList *
gtt_settings_get_array_int (GSettings *const settings, const gchar *const key)
{
  GSList *ret = NULL;

  GVariant *var = g_settings_get_value (settings, key);

  const gsize n_items = g_variant_n_children (var);
  gsize i;
  for (i = 0; i < n_items; ++i)
    {
      gint val;
      g_variant_get_child (var, i, "i", &val);
      ret = g_slist_prepend (ret, GINT_TO_POINTER (val));
    }

  g_variant_unref (var);
  var = NULL;

  ret = g_slist_reverse (ret);

  return ret;
}

void
gtt_settings_get_maybe_str (GSettings *const settings, const gchar *const key,
                            gchar **const value)
{
  if (NULL != *value)
    {
      g_free (*value);
      *value = NULL;
    }

  GVariant *val = g_settings_get_value (settings, key);

  GVariant *maybe = g_variant_get_maybe (val);
  if (NULL != maybe)
    {
      gsize len;
      *value = g_strdup (g_variant_get_string (maybe, &len));

      g_variant_unref (maybe);
      maybe = NULL;
    }

  g_variant_unref (val);
  val = NULL;
}

void
gtt_settings_get_str (GSettings *const settings, const gchar *const key,
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
gtt_settings_set_array_int (GSettings *const settings, const gchar *const key,
                            GSList *const value)
{
  GVariantType *variant_type = g_variant_type_new ("ai");

  GVariantBuilder bldr;
  g_variant_builder_init (&bldr, variant_type);

  GSList *node;
  for (node = value; node != NULL; node = node->next)
    {
      g_variant_builder_add (&bldr, "i", GPOINTER_TO_INT (node->data));
    }

  GVariant *val = g_variant_builder_end (&bldr);
  if (G_UNLIKELY (FALSE == g_settings_set_value (settings, key, val)))
    {
      g_warning ("Failed to set integer array option \"%s\"", key);
    }

  g_variant_type_free (variant_type);
  variant_type = NULL;
}

void
gtt_settings_set_bool (GSettings *const settings, const gchar *const key,
                       const gboolean value)
{
  if (G_UNLIKELY (FALSE == g_settings_set_boolean (settings, key, value)))
    {
      g_warning ("Failed to set boolean option \"%s\" to: %s", key,
                 FALSE == value ? "false" : "true");
    }
}

void
gtt_settings_set_int (GSettings *const settings, const gchar *const key,
                      const gint value)
{
  if (G_UNLIKELY (FALSE == g_settings_set_int (settings, key, value)))
    {
      g_warning ("Failed to set integer option \"%s\" to: %d", key, value);
    }
}

void
gtt_settings_set_maybe_str (GSettings *const settings, const gchar *const key,
                            const gchar *const value)
{
  GVariant *maybe = g_variant_new ("ms", value);

  if (G_UNLIKELY (FALSE == g_settings_set_value (settings, key, maybe)))
    {
      g_warning ("Failed to set maybe string option \"%s\" to: \"%s\"", key,
                 value ? value : "<nothing>");
    }
}

void
gtt_settings_set_str (GSettings *const settings, const gchar *const key,
                      const gchar *const value)
{
  if (G_UNLIKELY (FALSE == g_settings_set_string (settings, key, value)))
    {
      g_warning ("Failed to set string option \"%s\" to: \"%s\"", key, value);
    }
}
