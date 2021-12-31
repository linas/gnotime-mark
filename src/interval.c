/*   Copyright (C) 2021      Markus Prasser
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

#include "interval.h"
#include "proj.h"
#include "proj_p.h"

static void gtt_interval_remove_from_parent_task (GttInterval *ivl);

static void
gtt_interval_remove_from_parent_task (GttInterval *ivl)
{
	if (NULL == ivl->parent)
	{
		return;
	}

	gtt_task_remove_interval (ivl->parent, ivl);
	ivl->parent = NULL;
}

GttInterval *
gtt_interval_new (void)
{
	return g_new0 (GttInterval, 1);
}
