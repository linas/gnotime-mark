/* GnomeUI to GSettings input/output handling for GTimeTracker - a time tracker
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

#include "gtt_gsettings_gnomeui.h"

#include "gtt_gsettings_io_p.h"

/* ======================================================= */
/* Convert gnome enums to strings */

#define CASE(x)                                                               \
  case x:                                                                     \
    return #x;

static const char *
gnome_ui_info_type_to_string (GnomeUIInfoType typ)
{
  switch (typ)
    {
      CASE (GNOME_APP_UI_ENDOFINFO);
      CASE (GNOME_APP_UI_ITEM);
      CASE (GNOME_APP_UI_TOGGLEITEM);
      CASE (GNOME_APP_UI_RADIOITEMS);
      CASE (GNOME_APP_UI_SUBTREE);
      CASE (GNOME_APP_UI_SEPARATOR);
      CASE (GNOME_APP_UI_HELP);
      CASE (GNOME_APP_UI_BUILDER_DATA);
      CASE (GNOME_APP_UI_ITEM_CONFIGURABLE);
      CASE (GNOME_APP_UI_SUBTREE_STOCK);
      CASE (GNOME_APP_UI_INCLUDE);
    }
  return "";
}

static const char *
gnome_ui_pixmap_type_to_string (GnomeUIPixmapType typ)
{
  switch (typ)
    {
      CASE (GNOME_APP_PIXMAP_NONE);
      CASE (GNOME_APP_PIXMAP_STOCK);
      CASE (GNOME_APP_PIXMAP_DATA);
      CASE (GNOME_APP_PIXMAP_FILENAME);
    }
  return "";
}

/* ======================================================= */
/* Convert strings to gnome enums */

#define MATCH(str, x)                                                         \
  if (0 == strcmp (str, #x))                                                  \
    return x;

static GnomeUIInfoType
string_to_gnome_ui_info_type (const char *str)
{
  MATCH (str, GNOME_APP_UI_ENDOFINFO);
  MATCH (str, GNOME_APP_UI_ITEM);
  MATCH (str, GNOME_APP_UI_TOGGLEITEM);
  MATCH (str, GNOME_APP_UI_RADIOITEMS);
  MATCH (str, GNOME_APP_UI_SUBTREE);
  MATCH (str, GNOME_APP_UI_SEPARATOR);
  MATCH (str, GNOME_APP_UI_HELP);
  MATCH (str, GNOME_APP_UI_BUILDER_DATA);
  MATCH (str, GNOME_APP_UI_ITEM_CONFIGURABLE);
  MATCH (str, GNOME_APP_UI_SUBTREE_STOCK);
  MATCH (str, GNOME_APP_UI_INCLUDE);
  return 0;
}

static GnomeUIPixmapType
string_to_gnome_ui_pixmap_type (const char *str)
{
  MATCH (str, GNOME_APP_PIXMAP_NONE);
  MATCH (str, GNOME_APP_PIXMAP_STOCK);
  MATCH (str, GNOME_APP_PIXMAP_DATA);
  MATCH (str, GNOME_APP_PIXMAP_FILENAME);
  return 0;
}

/* ======================================================= */
/* Save the contents of a GnomeUIInfo structure with GSettings */

void
gtt_save_gnomeui_to_gsettings (GSettings *const setts, GnomeUIInfo *const gui)
{
  if (!gui || !setts)
    return;

  if (GNOME_APP_UI_ENDOFINFO == gui->type)
    return;

  /* Store the info */
  set_str (setts, "type", gnome_ui_info_type_to_string (gui->type));
  set_str (setts, "label", gui->label);
  set_str (setts, "hint", gui->hint);
  set_str (setts, "pixmap-type",
           gnome_ui_pixmap_type_to_string (gui->pixmap_type));
  set_str (setts, "pixmap-info", gui->pixmap_info);
  set_int (setts, "accelerator-key", gui->accelerator_key);
  set_int (setts, "ac-mods", gui->ac_mods);
}

/* ======================================================= */
/* Restore the contents of a GnomeUIInfo structure from GSettings */

void
gtt_restore_gnomeui_from_gsettings (GSettings *const setts,
                                    GnomeUIInfo *const gui)
{
  if (!gui || !setts)
    return;

  /* Restore the info */
  gchar *tmp_str = NULL;

  get_str (setts, "type", &tmp_str);
  gui->type = string_to_gnome_ui_info_type (tmp_str);
  gui->label = g_settings_get_string (setts, "label");
  gui->hint = g_settings_get_string (setts, "hint");
  get_str (setts, "pixmap-type", &tmp_str);
  gui->pixmap_type = string_to_gnome_ui_pixmap_type (tmp_str);
  gui->pixmap_info = g_settings_get_string (setts, "pixmap-info");
  gui->accelerator_key = g_settings_get_int (setts, "accelerator-key");
  gui->ac_mods = g_settings_get_int (setts, "ac-mods");

  g_free (tmp_str);
  tmp_str = NULL;
}

/* ======================= END OF FILE ======================== */
