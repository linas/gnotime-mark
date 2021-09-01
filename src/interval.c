/*
 * Project structure handling for GnoTime - a time tracker
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

#include "interval.h"
#include "proj_p.h"

int
gtt_interval_get_fuzz(const GttInterval *const ivl)
{
	if (NULL == ivl)
	{
		return 0;
	}

	return ivl->fuzz;
}

time_t
gtt_interval_get_start(const GttInterval *const ivl)
{
	if (NULL == ivl)
	{
		return 0;
	}

	return ivl->start;
}

time_t
gtt_interval_get_stop(const GttInterval *const ivl)
{
	if (NULL == ivl)
	{
		return 0;
	}

	return ivl->stop;
}

gboolean
gtt_interval_is_first_interval(const GttInterval *const ivl)
{
	if ((NULL == ivl) || (NULL == ivl->parent) ||
	    (NULL == ivl->parent->interval_list))
	{
		return TRUE;
	}

	if ((GttInterval *)ivl->parent->interval_list->data == ivl)
	{
		return TRUE;
	}

	return FALSE;
}

gboolean
gtt_interval_is_last_interval(const GttInterval *const ivl)
{
	if ((NULL == ivl) || (NULL == ivl->parent) ||
	    (NULL == ivl->parent->interval_list))
	{
		return TRUE;
	}

	if ((GttInterval *)((g_list_last(ivl->parent->interval_list))->data) == ivl)
	{
		return TRUE;
	}

	return FALSE;
}

gboolean
gtt_interval_is_running(const GttInterval *const ivl)
{
	if (NULL == ivl)
	{
		return FALSE;
	}

	return (gboolean)ivl->running;
}

GttInterval *
gtt_interval_new(void)
{
	GttInterval *const ivl = g_new0(GttInterval, 1);

	ivl->fuzz = 0;
	ivl->parent = NULL;
	ivl->running = FALSE;
	ivl->start = 0;
	ivl->stop = 0;

	return ivl;
}
