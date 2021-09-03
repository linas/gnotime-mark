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
#include "internal_data.h"
#include "proj_p.h"

void
gtt_interval_destroy(GttInterval *const ivl)
{
	g_return_if_fail(NULL != ivl);

	gtt_interval_unhook(ivl);
	g_free(ivl);
}

void
gtt_interval_freeze(GttInterval *const ivl)
{
	g_return_if_fail((NULL != ivl) && (NULL != ivl->parent) &&
	                 (NULL != ivl->parent->parent));

	ivl->parent->parent->frozen = TRUE;
}

int
gtt_interval_get_fuzz(const GttInterval *const ivl)
{
	g_return_val_if_fail(NULL != ivl, 0);

	return ivl->fuzz;
}

GttTask *
gtt_interval_get_parent(const GttInterval *const ivl)
{
	g_return_val_if_fail(NULL != ivl, NULL);

	return ivl->parent;
}

time_t
gtt_interval_get_start(const GttInterval *const ivl)
{
	g_return_val_if_fail(NULL != ivl, 0);

	return ivl->start;
}

time_t
gtt_interval_get_stop(const GttInterval *const ivl)
{
	g_return_val_if_fail(NULL != ivl, 0);

	return ivl->stop;
}

gboolean
gtt_interval_is_first_interval(const GttInterval *const ivl)
{
	g_return_val_if_fail((NULL != ivl) && (NULL != ivl->parent) &&
	                         (NULL != ivl->parent->interval_list),
	                     TRUE);

	if ((GttInterval *)ivl->parent->interval_list->data == ivl)
	{
		return TRUE;
	}

	return FALSE;
}

gboolean
gtt_interval_is_last_interval(const GttInterval *const ivl)
{
	g_return_val_if_fail((NULL != ivl) && (NULL != ivl->parent) &&
	                         (NULL != ivl->parent->interval_list),
	                     TRUE);

	if ((GttInterval *)((g_list_last(ivl->parent->interval_list))->data) == ivl)
	{
		return TRUE;
	}

	return FALSE;
}

gboolean
gtt_interval_is_running(const GttInterval *const ivl)
{
	g_return_val_if_fail(NULL != ivl, FALSE);

	return (gboolean)ivl->running;
}

/**
 * @brief Same as gtt_interval_merge_up instead incrementing stop time
 * @param[in] ivl Merge given interval with direct successor
 * @return The merged interval
 */
GttInterval *
gtt_interval_merge_down(GttInterval *const ivl)
{
	g_return_val_if_fail(NULL != ivl, NULL);

	int more_fuzz;
	int ivl_len;
	GList *node;
	GttInterval *merge;
	GttTask *prnt;

	prnt = ivl->parent;
	if (!prnt)
		return NULL;

	for (node = prnt->interval_list; node; node = node->next)
	{
		if (ivl == node->data)
			break;
	}
	if (!node)
		return NULL;
	node = node->next;
	if (!node)
		return NULL;

	merge = node->data;
	if (!merge)
		return NULL;

	/* the fuzz is the gap between stop and start times */
	more_fuzz = ivl->start - merge->stop;
	ivl_len = ivl->stop - ivl->start;
	if (more_fuzz > ivl_len)
		more_fuzz = ivl_len;

	merge->stop += ivl_len;
	if (ivl->fuzz > merge->fuzz)
		merge->fuzz = ivl->fuzz;
	if (more_fuzz > merge->fuzz)
		merge->fuzz = more_fuzz;
	gtt_interval_destroy(ivl);

	proj_refresh_time(prnt->parent);
	return merge;
}

/**
 * @brief Merge given interval with direct predecessor
 *
 * The merge is done by decrementing the start time. The fuzz factor is set
 * according to the greater one of the two. The new interval is running if the
 * first one was.
 *
 * @param[in] ivl Merge given interval with direct predecessor
 * @return The merged interval
 */
GttInterval *
gtt_interval_merge_up(GttInterval *const ivl)
{
	g_return_val_if_fail(NULL != ivl, NULL);

	int more_fuzz;
	int ivl_len;
	GList *node;
	GttInterval *merge;
	GttTask *prnt;

	prnt = ivl->parent;
	if (!prnt)
		return NULL;

	for (node = prnt->interval_list; node; node = node->next)
	{
		if (ivl == node->data)
			break;
	}
	if (!node)
		return NULL;
	node = node->prev;
	if (!node)
		return NULL;

	merge = node->data;
	if (!merge)
		return NULL;

	/* the fuzz is the gap between stop and start times */
	more_fuzz = merge->start - ivl->stop;
	ivl_len = ivl->stop - ivl->start;
	if (more_fuzz > ivl_len)
		more_fuzz = ivl_len;

	merge->start -= ivl_len;
	if (ivl->fuzz > merge->fuzz)
		merge->fuzz = ivl->fuzz;
	if (more_fuzz > merge->fuzz)
		merge->fuzz = more_fuzz;

	gtt_interval_destroy(ivl);

	proj_refresh_time(prnt->parent);
	return merge;
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

/**
 * @brief Create a new interval and insert it after the passed one
 * @param[in] where The interval after which the new instance shall be inserted
 * @return A new GttInterval instance
 */
GttInterval *
gtt_interval_new_insert_after(GttInterval *const where)
{
	g_return_val_if_fail(NULL != where, NULL);

	GttInterval *ivl;
	GttTask *tsk;
	int idx;

	tsk = where->parent;
	if (!tsk)
		return NULL;

	/* clone the other interval */
	ivl = g_new0(GttInterval, 1);
	ivl->parent = tsk;
	ivl->start = where->start;
	ivl->stop = where->stop;
	ivl->running = FALSE;
	ivl->fuzz = where->fuzz;

	idx = g_list_index(tsk->interval_list, where);
	tsk->interval_list = g_list_insert(tsk->interval_list, ivl, idx);

	/* Don't do a refresh, since the refresh will probably
	 * cull this interval for being to short, or merge it,
	 * or something.  Refresh only after some edit has occurred.
	 */
	/* proj_refresh_time (tsk->parent); */

	return ivl;
}

void
gtt_interval_set_fuzz(GttInterval *const ivl, const int st)
{
	g_return_if_fail(NULL != ivl);

	ivl->fuzz = st;
	if (ivl->parent)
		proj_modified(ivl->parent->parent);
}

void
gtt_interval_set_running(GttInterval *const ivl, const gboolean st)
{
	g_return_if_fail(NULL != ivl);

	ivl->running = st;
	if (ivl->parent)
		proj_modified(ivl->parent->parent);
}

void
gtt_interval_set_start(GttInterval *const ivl, const time_t st)
{
	g_return_if_fail(NULL != ivl);

	ivl->start = st;
	if (st > ivl->stop)
		ivl->stop = st;
	if (ivl->parent)
		proj_refresh_time(ivl->parent->parent);
}

void
gtt_interval_set_stop(GttInterval *const ivl, const time_t st)
{
	g_return_if_fail(NULL != ivl);

	ivl->stop = st;
	if (st < ivl->start)
		ivl->start = st;
	if (ivl->parent)
		proj_refresh_time(ivl->parent->parent);
}

/**
 * @brief Split list of intervals in two pieces
 * @param ivl
 * @param newtask
 */
void
gtt_interval_split(GttInterval *const ivl, GttTask *const newtask)
{
	g_return_if_fail((NULL != ivl) && (NULL != newtask));

	int is_running = 0;
	gint idx;
	GttProject *prj;
	GttTask *prnt;
	GList *node;
	GttInterval *first_ivl;

	prnt = ivl->parent;
	if (!prnt)
		return;
	prj = prnt->parent;
	if (!prj)
		return;
	node = g_list_find(prnt->interval_list, ivl);
	if (!node)
		return;

	gtt_task_remove(newtask);

	/* avoid misplaced running intervals, stop the task */
	first_ivl = (GttInterval *)(prnt->interval_list->data);
	is_running = first_ivl->running;
	if (is_running)
	{
		/* don't call stop here, avoid dispatching redraw events */
		gtt_project_timer_update(prj);
		first_ivl->running = FALSE;
	}

	/* chain the new task into proper order in the parent project */
	idx = g_list_index(prj->task_list, prnt);
	idx++;
	prj->task_list = g_list_insert(prj->task_list, newtask, idx);
	newtask->parent = prj;

	/* Rechain the intervals.  We do this by hand, since it
	 * seems that glib doesn't provide this function. */
	if (node->prev)
	{
		node->prev->next = NULL;
		node->prev = NULL;
	} else
	{
		prnt->interval_list = NULL;
	}

	newtask->interval_list = node;

	for (; node; node = node->next)
	{
		GttInterval *nivl = node->data;
		nivl->parent = newtask;
	}

	if (is_running)
		gtt_project_timer_start(prj);

	proj_refresh_time(prnt->parent);
}

/**
 * @brief Cause notifiers to be sent
 *
 * Additionally this will run interval scrubbing methods. This may cause the
 * indicated interval to be deleted (i.e. merged into a surrouning interval). To
 * avoid a dangling pointer to freed memory do not use the input pointer again.
 * Instead replace the input pointer with the returned value which will either
 * be the input interval itself or a nearby interval if the input one was
 * deleted.
 *
 * @param[in] ivl The interval for which notifications shall be sent again
 * @return The interval itself or a nearby one if it got scrubbed
 */
GttInterval *
gtt_interval_thaw(GttInterval *ivl)
{
	g_return_val_if_fail((NULL != ivl) && (NULL != ivl->parent) &&
	                         (NULL != ivl->parent->parent),
	                     NULL);
	g_return_val_if_fail((NULL != ivl->parent) && (NULL != ivl->parent->parent),
	                     ivl);

	ivl->parent->parent->frozen = FALSE;
	ivl = scrub_intervals(ivl->parent, ivl);
	proj_refresh_time(ivl->parent->parent);
	return ivl;
}

void
gtt_interval_unhook(GttInterval *ivl)
{
	g_return_if_fail(NULL != ivl);

	/* Unhook myself from the chain */
	ivl->parent->interval_list = g_list_remove(ivl->parent->interval_list, ivl);
	proj_refresh_time(ivl->parent->parent);
	ivl->parent = NULL;
}
