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

#include "gtt_gsettings_io.h"

#include <errno.h>
#include <glib.h>
#include <gnome.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"
#include "cur-proj.h"
#include "err-throw.h"
#include "file-io.h"
#include "gtt.h"
#include "menus.h"
#include "plug-in.h"
#include "prefs.h"
#include "proj.h"
#include "proj_p.h"
#include "timer.h"
#include "toolbar.h"

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

/* ============================================================= */
/* Configuration file I/O routines:
 * Note that this file supports reading from several old, 'obsolete'
 * config file formats taht GTT has used over the years.  We support
 * these reads so that users do not get left out in the cold when
 * upgrading from old versions of GTT.  All 'saves' are in the new
 * file format (currently, GConf-2).
 *
 * 1) Oldest format is data stuck into a ~/.gtimetrackerrc file
 *    and is handled by the project_list_load_old() routine.
 * 2) Next is Gnome-1 Gnome-Config files in ~/.gnome/gtt
 * 3) Next is Gnome-2 Gnome-Config files in ~/.gnome2/GnoTime
 * 4) Current is GConf2 system.
 *
 * Note that some of the older config files also contained project
 * data in them.  The newer versions stored project data seperately
 * from the app config data.
 */

/* RC_NAME is old, depricated; stays here for backwards compat. */
#define RC_NAME ".gtimetrackerrc"

static const char *
build_rc_name_old (void)
{
  static char *buf = NULL;

  if (g_getenv ("HOME") != NULL)
    {
      buf = g_concat_dir_and_file (g_getenv ("HOME"), RC_NAME);
    }
  else
    {
      buf = g_strdup (RC_NAME);
    }
  return buf;
}

static void
project_list_load_old (void)
{
  FILE *f;
  const char *realname;
  GttProject *proj = NULL;
  char s[1024];
  int i;

  realname = build_rc_name_old ();
  gtt_config_filepath = realname;

  if (NULL == (f = fopen (realname, "rt")))
    {
      gtt_err_set_code (GTT_CANT_OPEN_FILE);
      gtt_config_filepath = ""; /* Don't spew obsolete filename to new users */
#ifdef ENOENT
      if (errno == ENOENT)
        return;
#endif
      g_warning ("could not open %s\n", realname);
      return;
    }
  printf ("GTT: Info: Importing .gtimetrackerrc config file\n");

  errno = 0;
  while ((!feof (f)) && (!errno))
    {
      if (!fgets (s, 1023, f))
        continue;
      if (s[0] == '#')
        continue;
      if (s[0] == '\n')
        continue;
      if (s[0] == ' ')
        {
          /* desc for last project */
          while (s[strlen (s) - 1] == '\n')
            s[strlen (s) - 1] = 0;
          gtt_project_set_desc (proj, &s[1]);
        }
      else if (s[0] == 't')
        {
          /* last_timer */
          last_timer = (time_t)atol (&s[1]);
        }
      else if ((s[0] >= '0') && (s[0] <= '9'))
        {
          time_t day_secs, ever_secs;

          /* new project */
          proj = gtt_project_new ();
          gtt_project_list_append (master_list, proj);
          ever_secs = atol (s);
          for (i = 0; s[i] != ' '; i++)
            ;
          i++;
          day_secs = atol (&s[i]);
          gtt_project_compat_set_secs (proj, ever_secs, day_secs, last_timer);
          for (; s[i] != ' '; i++)
            ;
          i++;
          while (s[strlen (s) - 1] == '\n')
            s[strlen (s) - 1] = 0;
          gtt_project_set_title (proj, &s[i]);
        }
    }
  if ((errno) && (!feof (f)))
    goto err;
  fclose (f);

  gtt_project_list_compute_secs ();

  return;

err:
  fclose (f);
  gtt_err_set_code (GTT_FILE_CORRUPT);
  g_warning ("error reading %s\n", realname);
}

/* ======================================================= */

#define GET_INT(str)                                                          \
  ({                                                                          \
    strcpy (p, str);                                                          \
    gnome_config_get_int (s);                                                 \
  })

#define GET_BOOL(str)                                                         \
  ({                                                                          \
    strcpy (p, str);                                                          \
    gnome_config_get_bool (s);                                                \
  })

#define GET_STR(str)                                                          \
  ({                                                                          \
    strcpy (p, str);                                                          \
    gnome_config_get_string (s);                                              \
  })

static void
gtt_load_gnome_config (const char *prefix)
{
  char *s, *p;
  int prefix_len;
  int i, num;

#define TOKLEN 120
  prefix_len = 0;
  if (prefix)
    prefix_len = strlen (prefix);
  s = g_new (char, prefix_len + TOKLEN);
  if (!s)
    return;
  s[0] = 0;
  if (prefix)
    strcpy (s, prefix);
  p = &s[prefix_len];

  /* If already running, and we are over-loading a new file,
   * then save the currently running project, and try to set it
   * running again ... */
  if (gtt_project_get_title (cur_proj) && (!first_proj_title))
    {
      /* we need to strdup because title is freed when
       * the project list is destroyed ... */
      first_proj_title = g_strdup (gtt_project_get_title (cur_proj));
    }

  /* ------------ */
  num = 0;
  for (i = 0; -1 < num; i++)
    {
      g_snprintf (p, TOKLEN, "/CList/ColumnWidth%d=-1", i);
      num = gnome_config_get_int (s);
      if (-1 < num)
        {
          //			ctree_set_col_width (global_ptw, i, num);
        }
    }

  g_free (s);
}

/* ======================================================= */

void
gtt_load_config (void)
{
  const char *h;
  char *s;

  /* Check for gconf2, and use that if it exists */
  if (gtt_gconf_exists ())
    {
      gtt_gsettings_load ();
      gtt_config_filepath = NULL;
      return;
    }

  /* gnotime breifly used the gnome2 gnome_config file */
  if (gnome_config_has_section (GTT_CONF "/Misc"))
    {
      printf ("GTT: Info: Importing ~/.gnome2/" GTT_CONF " file\n");
      gtt_load_gnome_config (GTT_CONF);
      gtt_config_filepath = gnome_config_get_real_path (GTT_CONF);

      /* The data file will be in the same directory ...
       * so prune filename to get the directory */
      char *p = strrchr (gtt_config_filepath, '/');
      if (p)
        *p = 0x0;
      return;
    }

  /* Look for a gnome-1.4 era gnome_config file */
  h = g_get_home_dir ();
  s = g_new (char, strlen (h) + 120);
  strcpy (s, "=");
  strcat (s, h);
  strcat (s, "/.gnome/gtt=/Misc");
  if (gnome_config_has_section (s))
    {
      strcpy (s, "=");
      strcat (s, h);
      strcat (s, "/.gnome/gtt=");
      printf ("GTT: Info: Importing ~/.gnome/gtt file\n");
      gtt_load_gnome_config (s);
      strcpy (s, h);
      strcat (s, "/.gnome");
      gtt_config_filepath = s;
      return;
    }
  g_free (s);

  /* OK, try to load the oldest file format */
  project_list_load_old ();

  return;
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
  if (gtt_gconf_exists ())
    {
      xpn = gtt_gsettings_get_expander ();
    }
  else
    {
      xpn = gnome_config_get_string (GTT_CONF "/Display/ExpanderState");
    }
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
