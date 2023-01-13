/*   Config file input/output handling for GnoTime
 *   Copyright (C) 1997,98 Eckehard Berns
 *   Copyright (C) 2001,2002,2003 Linas Vepstas <linas@linas.org>
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

#include "gtt_file_io.h"

#include <errno.h>
#include <glib.h>
#include <gnome.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gtt.h"
#include "gtt_application_window.h"
#include "gtt_current_project.h"
#include "gtt_err_throw.h"
#include "gtt_gsettings_io.h"
#include "gtt_menus.h"
#include "gtt_plug_in.h"
#include "gtt_preferences.h"
#include "gtt_project.h"
#include "gtt_project_p.h"
#include "gtt_timer.h"
#include "gtt_toolbar.h"

#ifdef USE_GTT_DEBUG_FILE
#define GTT_CONF "/gtt-DEBUG"
#else /* not DEBUG */
#define GTT_CONF "/" GTT_APP_NAME
#endif /* not DEBUG */

static const char *gtt_config_filepath = NULL;

int cur_proj_id = -1;
int run_timer = FALSE;
time_t last_timer = -1;
extern char *first_proj_title; /* command line flag */
int save_count = 0;

/* ============================================================= */
/* Configuration file I/O routines:
 * Note that this file supports reading from several old, 'obsolete'
 * config file formats taht GTT has used over the years.  We support
 * these reads so that users do not get left out in the cold when
 * upgrading from old versions of GTT.  All 'saves' are in the new
 * format (currently, GSettings).
 *
 * 1) Current is GSettings.
 *
 * Note that some of the older config files also contained project
 * data in them.  The newer versions stored project data seperately
 * from the app config data.
 */

/* ======================================================= */

void
gtt_load_config (void)
{
  gtt_gsettings_load ();
  gtt_config_filepath = NULL;
}

/* ======================================================= */

void
gtt_post_data_config (void)
{
  /* Assume we've already read the XML data, and just
   * set the current project */
  cur_proj_set (gtt_project_locate_from_id (cur_proj_id));

  /* Over-ride the current project based on the
   * command-line setting */
  if (first_proj_title)
    {
      GList *node;
      node = gtt_project_list_get_list (master_list);
      for (; node; node = node->next)
        {
          GttProject *prj = node->data;
          if (!gtt_project_get_title (prj))
            continue;

          /* set project based on command line */
          if (0 == strcmp (gtt_project_get_title (prj), first_proj_title))
            {
              cur_proj_set (prj);
              break;
            }
        }
    }

  /* FIXME: this is a mem leak, depending on usage in main.c */
  first_proj_title = NULL;

  /* reset the clocks, if needed */
  if (0 < last_timer)
    {
      set_last_reset (last_timer);
      zero_daily_counters (NULL);
    }

  /* if a project is running, then set it running again,
   * otherwise be sure to stop the clock. */
  if (FALSE == run_timer)
    {
      cur_proj_set (NULL);
    }
}

void
gtt_post_ctree_config (void)
{
  gchar *xpn = NULL;

  /* Assume the ctree has been set up.  Now punch in the final
   * bit of ctree state.
   */

  /* Restore the expander state */
  xpn = gtt_gsettings_get_expander ();
  if (xpn)
    {
      gtt_projects_tree_set_expander_state (projects_tree, xpn);
    }
}

/* ======================================================= */
/* Save only the GUI configuration info, not the actual data */

void
gtt_save_config (void)
{
  gtt_gsettings_save ();
}

/* ======================================================= */

const char *
gtt_get_config_filepath (void)
{
  return gtt_config_filepath;
}

/* =========================== END OF FILE ========================= */
