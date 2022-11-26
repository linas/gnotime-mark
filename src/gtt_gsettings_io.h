/*   GSettings input/output handling for GnoTime
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

#ifndef GTT_GSETTINGS_IO_H
#define GTT_GSETTINGS_IO_H

#include <glib.h>

/**
 * The gtt_gsettings_save() routine will save all of the GTT attributes into
 * the GSettings system.
 */
void gtt_gsettings_save (void);

/**
 * The gtt_gsettings_load() routine will fetch all of the GTT attributes from
 * the GSettings system.
 */
void gtt_gsettings_load (void);

/**
 * The gtt_save_reports_menu() routine saves only the reports menu
 * attributes to the GSettings system.
 */
void gtt_save_reports_menu (void);

/* quick hack */
char *gtt_gsettings_get_expander (void);

#endif // GTT_GSETTINGS_IO_H
