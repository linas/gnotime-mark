/*
 * Project structure handling support for GnoTime - a time tracker
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

#include "internal_data.h"
#include "interval.h"
#include "prefs.h"
#include "proj_p.h"

// TODO: Replace this notifier by GObject signals someday
typedef struct notif_s
{
	GttProjectChanged func;
	gpointer user_data;
} Notifier;

static GttInterval *get_closest(GList *node);
static time_t get_month(time_t last);
static time_t get_newyear(time_t last);
static time_t get_sunday(time_t last);

static GttInterval *
get_closest(GList *node)
{
	if (node->next)
		return node->next->data;
	if (node->prev)
		return node->prev->data;
	return NULL;
}

time_t
get_midnight(time_t last)
{
	struct tm lt;
	time_t midnight;

	if (0 >= last)
	{
		last = time(0);
	}

	/* If config_daystart_offset == 3*3600 then the new day
	 * will start at 3AM, for example.  */
	last -= config_daystart_offset;

	memcpy(&lt, localtime(&last), sizeof(struct tm));
	lt.tm_sec = 0;
	lt.tm_min = 0;
	lt.tm_hour = 0;
	midnight = mktime(&lt);

	midnight += config_daystart_offset;

	return midnight;
}

static time_t
get_month(time_t last)
{
	struct tm lt;
	time_t first;

	if (0 >= last)
	{
		last = time(0);
	}

	memcpy(&lt, localtime(&last), sizeof(struct tm));
	lt.tm_sec = 0;
	lt.tm_min = 0;
	lt.tm_hour = 0;
	lt.tm_mday = 1;
	first = mktime(&lt);

	first += config_daystart_offset;
	return first;
}

static time_t
get_newyear(time_t last)
{
	struct tm lt;
	time_t newyear;

	if (0 >= last)
	{
		last = time(0);
	}

	memcpy(&lt, localtime(&last), sizeof(struct tm));
	lt.tm_sec = 0;
	lt.tm_min = 0;
	lt.tm_hour = 0;
	lt.tm_mday -= lt.tm_yday;
	newyear = mktime(&lt);

	newyear += config_daystart_offset;
	return newyear;
}

static time_t
get_sunday(time_t last)
{
	struct tm lt;
	time_t sunday;

	if (0 >= last)
	{
		last = time(0);
	}

	memcpy(&lt, localtime(&last), sizeof(struct tm));
	lt.tm_sec = 0;
	lt.tm_min = 0;
	lt.tm_hour = 0;
	lt.tm_mday -= lt.tm_wday;
	sunday = mktime(&lt);

	/* If config_weekstart_offset == 1 then a new week starts
	 * on monday, not sunday. */
	sunday += 24 * 3600 * config_weekstart_offset + config_daystart_offset;

	return sunday;
}

void
proj_modified(GttProject *proj)
{
	if (NULL == proj)
	{
		return;
	}
	if (proj->being_destroyed)
	{
		return;
	}
	if (proj->frozen)
	{
		return;
	}

	GList *node = proj->listeners;
	/* let listeners know that the times have changed */
	for (; node; node = node->next)
	{
		Notifier *const ntf = node->data;
		(ntf->func)(proj, ntf->user_data);
	}
}

void
proj_refresh_time(GttProject *proj)
{
	GList *node;

	if (!proj)
		return;
	if (proj->being_destroyed)
		return;
	proj->dirty_time = TRUE;
	if (proj->frozen)
		return;
	project_compute_secs(proj);

	/* let listeners know that the times have changed */
	for (node = proj->listeners; node; node = node->next)
	{
		Notifier *ntf = node->data;
		(ntf->func)(proj, ntf->user_data);
	}
}

void
project_compute_secs(GttProject *proj)
{
	int total_ever = 0;
	int total_day = 0;
	int total_yesterday = 0;
	int total_week = 0;
	int total_lastweek = 0;
	int total_month = 0;
	int total_year = 0;
	time_t midnight, sunday, month, newyear;
	GList *tsk_node, *ivl_node, *prj_node;

	if (!proj)
		return;

	/* Total up the subprojects first */
	for (prj_node = proj->sub_projects; prj_node; prj_node = prj_node->next)
	{
		GttProject *prj = prj_node->data;
		project_compute_secs(prj);
	}

	midnight = get_midnight(-1);
	sunday = get_sunday(-1);
	month = get_month(-1);
	newyear = get_newyear(-1);

	/* Total up time spent in various tasks.
	 * XXX None of these total handle daylight savings correctly.
	 */
	for (tsk_node = proj->task_list; tsk_node; tsk_node = tsk_node->next)
	{
		GttTask *task = tsk_node->data;
		scrub_intervals(task, NULL);
		for (ivl_node = task->interval_list; ivl_node; ivl_node = ivl_node->next)
		{
			GttInterval *ivl = ivl_node->data;
			total_ever += ivl->stop - ivl->start;

			/* Accum time today. */
			if (ivl->start >= midnight)
			{
				total_day += ivl->stop - ivl->start;
			} else if (ivl->stop > midnight)
			{
				total_day += ivl->stop - midnight;
			}

			/* Accum time yesterday. */
			if (ivl->start < midnight)
			{
				if (ivl->start >= midnight - 24 * 3600)
				{
					if (ivl->stop <= midnight)
					{
						total_yesterday += ivl->stop - ivl->start;
					} else
					{
						total_yesterday += midnight - ivl->start;
					}
				} else /* else .. it started before midnight yesterday */
						if (ivl->stop > midnight - 24 * 3600)
				{
					if (ivl->stop <= midnight)
					{
						total_yesterday += ivl->stop - (midnight - 24 * 3600);
					} else
					{
						total_yesterday += 24 * 3600;
					}
				}
			}

			/* Accum time this week. */
			if (ivl->start >= sunday)
			{
				total_week += ivl->stop - ivl->start;
			} else if (ivl->stop > sunday)
			{
				total_week += ivl->stop - sunday;
			}

			/* Accum time last week. */
			if (ivl->start < sunday)
			{
				if (ivl->start >= sunday - 7 * 24 * 3600)
				{
					if (ivl->stop <= sunday)
					{
						total_lastweek += ivl->stop - ivl->start;
					} else
					{
						total_lastweek += sunday - ivl->start;
					}
				} else /* else .. it started before sunday last week */
						if (ivl->stop > sunday - 7 * 24 * 3600)
				{
					if (ivl->stop <= sunday)
					{
						total_lastweek += ivl->stop - (sunday - 7 * 24 * 3600);
					} else
					{
						total_lastweek += 7 * 24 * 3600;
					}
				}
			}

			/* Accum time this month */
			if (ivl->start >= month)
			{
				total_month += ivl->stop - ivl->start;
			} else if (ivl->stop > month)
			{
				total_month += ivl->stop - month;
			}
			if (ivl->start >= newyear)
			{
				total_year += ivl->stop - ivl->start;
			} else if (ivl->stop > newyear)
			{
				total_year += ivl->stop - newyear;
			}
		}
	}

	proj->secs_ever = total_ever;
	proj->secs_day = total_day;
	proj->secs_yesterday = total_yesterday;
	proj->secs_week = total_week;
	proj->secs_lastweek = total_lastweek;
	proj->secs_month = total_month;
	proj->secs_year = total_year;
	proj->dirty_time = FALSE;
}

/* =========================================================== */
/* =========================================================== */
/* Recomputed cached data.  Scrub it while we're at it. */
/* Scrub algorithm as follows (as documented in header files):
 *     Discard all intervals shorter than proj->min_interval
 *     Merge all intervals whose gaps between them are shorter
 *     than proj->auto_merge_gap
 *     Merge all intervals that are shorter than
 *     proj->auto_merge_interval into the nearest interval.
 *     (but only do this if the nearest is in the same day).
 */
GttInterval *
scrub_intervals(GttTask *tsk, GttInterval *handle)
{
	GttProject *prj;
	GList *node;
	int mini, merge, mgap;
	int not_done = TRUE;
	int save_freeze;

	/* Prevent recursion */
	prj = tsk->parent;
	g_return_val_if_fail(prj, FALSE);
	save_freeze = prj->frozen;
	prj->frozen = TRUE;

	/* First, eliminate very short intervals */
	mini = prj->min_interval;
	while (not_done)
	{
		not_done = FALSE;
		for (node = tsk->interval_list; node; node = node->next)
		{
			GttInterval *ivl = node->data;
			int len = ivl->stop - ivl->start;

			/* Should never see negative intervals */
			g_return_val_if_fail((0 <= len), handle);
			if ((FALSE == ivl->running) && (len <= mini) &&
			    (0 != ivl->start)) /* don't whack new ivls */
			{
				if (handle == ivl)
					handle = get_closest(node);
				tsk->interval_list = g_list_remove(tsk->interval_list, ivl);
				ivl->parent = NULL;
				g_free(ivl);
				not_done = TRUE;
				break;
			}
		}
	}

	/* Merge intervals with small gaps between them */
	mgap = prj->auto_merge_gap;
	not_done = TRUE;
	while (not_done)
	{
		not_done = FALSE;
		for (node = tsk->interval_list; node; node = node->next)
		{
			GttInterval *ivl = node->data;
			GttInterval *nivl;
			int gap;

			if (ivl->running)
				continue;
			if (!node->next)
				break;
			nivl = node->next->data;
			gap = ivl->start - nivl->stop;
			if (0 > gap)
				continue; /* out of order ivls */
			if ((mgap > gap) || (ivl->fuzz > gap) || (nivl->fuzz > gap))
			{
				GttInterval *rc = gtt_interval_merge_down(ivl);
				if (handle == ivl)
					handle = rc;
				not_done = TRUE;
				break;
			}
		}
	}

	/* Merge short intervals into neighbors */
	merge = prj->auto_merge_interval;
	not_done = TRUE;
	while (not_done)
	{
		not_done = FALSE;
		for (node = tsk->interval_list; node; node = node->next)
		{
			GttInterval *ivl = node->data;
			int gap_up = 1000000000;
			int gap_down = 1000000000;
			int do_merge = FALSE;
			int len;

			if (ivl->running)
				continue;
			if (0 == ivl->start)
				continue; /* brand-new ivl */
			len = ivl->stop - ivl->start;
			if (len > merge)
				continue;
			if (node->next)
			{
				GttInterval *nivl = node->next->data;

				/* Merge only if the intervals are in the same day */
				if (get_midnight(ivl->start) == get_midnight(nivl->stop))
				{
					gap_down = ivl->start - nivl->stop;
					do_merge = TRUE;
				}
			}
			if (node->prev)
			{
				GttInterval *nivl = node->prev->data;

				/* Merge only if the intervals are in the same day */
				if (get_midnight(nivl->start) == get_midnight(ivl->stop))
				{
					gap_up = nivl->start - ivl->stop;
					do_merge = TRUE;
				}
			}
			if (!do_merge)
				continue;
			if (gap_up < gap_down)
			{
				GttInterval *rc = gtt_interval_merge_up(ivl);
				if (handle == ivl)
					handle = rc;
			} else
			{
				GttInterval *rc = gtt_interval_merge_down(ivl);
				if (handle == ivl)
					handle = rc;
			}
			not_done = TRUE;
			break;
		}
	}

	prj->frozen = save_freeze;
	return handle;
}
