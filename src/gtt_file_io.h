/*   file io routines for GTimeTracker - a time tracker
 *   Copyright (C) 1997,98 Eckehard Berns
 * Copyright (C) 2023      Markus Prasser
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

#ifndef GTT_FILE_IO_H
#define GTT_FILE_IO_H

#include <glib.h>

/* file-io.c and file-io.h is mostly involved in saving and restoring
 * user preference data to the default user config file (in .gnome2/gnotime)
 */

/* The routine gtt_save_config() will save configuration/user-preference
 *    data using GSettings.
 *
 * The routine gtt_load_config() will load GTT configuration data
 *    from either GSettings or from gnome_config.  It will attempt to
 *    load data from the latest storage mechanism first, and then
 *    fallig back to older file formats.  This routine is 'backwards
 *    compatible', in that it will load old config files formats if
 *    it can't find the newer ones first.
 *    If an error occurs, a GttErrCode is set.
 */
void gtt_save_config (void);
void gtt_load_config (void);

/* The gtt_post_data_config() routine should be called *after* the
 *    project data has been loaded. It performs some final configuration
 *    and setup, such as setting the last (current) active project,
 *    starting timers, etc.
 *
 * The gtt_post_ctree_config() routine should be called *after* the
 *    ctree is mostly set up.  It does a final bit of ctree setup,
 *    viz. restoring expander state.
 */

void gtt_post_data_config (void);
void gtt_post_ctree_config (void);

#endif // GTT_FILE_IO_H
