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

void
gtt_interval_destroy (GttInterval *const ivl)
{
	g_return_if_fail (NULL != ivl);

	gtt_interval_remove_from_parent_task (ivl);
	g_free (ivl);
}

void
gtt_interval_freeze (GttInterval *const ivl)
{
	g_return_if_fail ((NULL != ivl) && (NULL != ivl->parent)
										&& (NULL != ivl->parent->parent));

	ivl->parent->parent->frozen = TRUE;
}

int
gtt_interval_get_fuzz (const GttInterval *const ivl)
{
	g_return_val_if_fail (NULL != ivl, 0);

	return ivl->fuzz;
}

GttTask *
gtt_interval_get_parent (const GttInterval *const ivl)
{
	g_return_val_if_fail (NULL != ivl, NULL);

	return ivl->parent;
}

time_t
gtt_interval_get_start (const GttInterval *const ivl)
{
	g_return_val_if_fail (NULL != ivl, 0);

	return ivl->start;
}

time_t
gtt_interval_get_stop (const GttInterval *const ivl)
{
	g_return_val_if_fail (NULL != ivl, 0);

	return ivl->stop;
}

gboolean
gtt_interval_is_first_interval (const GttInterval *const ivl)
{
	g_return_val_if_fail ((NULL != ivl) && (NULL != ivl->parent)
														&& (NULL != ivl->parent->interval_list),
												TRUE);

	if ((GttInterval *) ivl->parent->interval_list->data == ivl)
	{
		return TRUE;
	}

	return FALSE;
}

gboolean
gtt_interval_is_last_interval (const GttInterval *const ivl)
{
	g_return_val_if_fail ((NULL != ivl) && (NULL != ivl->parent)
														&& (NULL != ivl->parent->interval_list),
												TRUE);

	if ((GttInterval *) ((g_list_last (ivl->parent->interval_list))->data)
			== ivl)
	{
		return TRUE;
	}

	return FALSE;
}

gboolean
gtt_interval_is_running (const GttInterval *const ivl)
{
	g_return_val_if_fail (NULL != ivl, FALSE);

	return (gboolean) ivl->running;
}

GttInterval *
gtt_interval_merge_down (GttInterval *ivl)
{
	g_return_val_if_fail (NULL != ivl, NULL);

	GttTask *const parent = ivl->parent;
	g_return_val_if_fail (NULL != parent, NULL);

	// Try to locate the interval's node in the parent task's interval list
	GList *node = parent->interval_list;
	for (; node; node = node->next)
	{
		if (ivl == node->data)
		{
			break;
		}
	}
	// Return with an error message if the interval could not be found
	if (NULL == node)
	{
		g_warning ("Encountered interval unknown by its parent task");
		return NULL;
	}
	// Find the succeeding node and return silently, if the interval is the last
	node = node->next;
	if (NULL == node)
	{
		return NULL;
	}

	GttInterval *merge = node->data;
	g_return_val_if_fail (NULL != merge, NULL);

	// The fuzz is the gap between stop and start times
	time_t more_fuzz = ivl->start - merge->stop;
	time_t ivl_len   = ivl->stop - ivl->start;
	if (more_fuzz > ivl_len)
	{
		more_fuzz = ivl_len;
	}

	merge->stop += ivl_len;
	if (ivl->fuzz > merge->fuzz)
	{
		merge->fuzz = ivl->fuzz;
	}
	if (more_fuzz > merge->fuzz)
	{
		merge->fuzz = more_fuzz;
	}
	gtt_interval_destroy (ivl);

	gtt_project_refresh_time (parent->parent);

	return merge;
}

GttInterval *
gtt_interval_merge_up (GttInterval *const ivl)
{
	g_return_val_if_fail (NULL != ivl, NULL);

	GttTask *const parent = ivl->parent;
	g_return_val_if_fail (NULL != parent, NULL);

	// Try to locate the interval's node in the parent task's interval list
	GList *node = parent->interval_list;
	for (; node; node = node->next)
	{
		if (ivl == node->data)
		{
			break;
		}
	}
	// Return with an error message if the interval could not be found
	if (NULL == node)
	{
		g_warning ("Encountered interval unknown by its parent task");
		return NULL;
	}
	// Find the preceding node and return silently, if the interval is the first
	node = node->prev;
	if (NULL == node)
	{
		return NULL;
	}

	GttInterval *merge = node->data;
	g_return_val_if_fail (NULL != merge, NULL);

	// The fuzz is the gap between stop and start times
	time_t more_fuzz = merge->start - ivl->stop;
	time_t ivl_len   = ivl->stop - ivl->start;
	if (more_fuzz > ivl_len)
	{
		more_fuzz = ivl_len;
	}

	merge->start -= ivl_len;
	if (ivl->fuzz > merge->fuzz)
	{
		merge->fuzz = ivl->fuzz;
	}
	if (more_fuzz > merge->fuzz)
	{
		merge->fuzz = more_fuzz;
	}

	gtt_interval_destroy (ivl);

	gtt_project_refresh_time (parent->parent);

	return merge;
}

GttInterval *
gtt_interval_new (void)
{
	return g_new0 (GttInterval, 1);
}

GttInterval *
gtt_interval_new_insert_after (GttInterval *const where)
{
	g_return_val_if_fail (NULL != where, NULL);

	GttTask *const tsk = where->parent;
	g_return_val_if_fail (NULL != tsk, NULL);

	// Clone the other interval
	GttInterval *ivl   = g_new0 (GttInterval, 1);
	ivl->parent        = tsk;
	ivl->fuzz          = where->fuzz;
	ivl->running       = FALSE;
	ivl->start         = where->start;
	ivl->stop          = where->stop;

	const gint idx     = g_list_index (tsk->interval_list, where);
	tsk->interval_list = g_list_insert (tsk->interval_list, ivl, idx);

	// No refresh is being conducted, since the refresh would probably cull the
	// new interval for being too short or merge it or else. The refresh should
	// occur only after some edit was done
	// gtt_project_refresh_time (tsk->parent);

	return ivl;
}

void
gtt_interval_set_fuzz (GttInterval *const ivl, const int fuzz)
{
	g_return_if_fail (NULL != ivl);

	ivl->fuzz = fuzz;

	if (ivl->parent)
	{
		gtt_project_modified (ivl->parent->parent);
	}
}

void
gtt_interval_set_running (GttInterval *const ivl, const gboolean running)
{
	g_return_if_fail (NULL != ivl);

	ivl->running = running;
	if (ivl->parent)
	{
		gtt_project_modified (ivl->parent->parent);
	}
}

void
gtt_interval_set_start (GttInterval *const ivl, const time_t start)
{
	g_return_if_fail (NULL != ivl);

	ivl->start = start;
	if (start > ivl->stop)
	{
		ivl->stop = start;
	}

	if (ivl->parent)
	{
		gtt_project_refresh_time (ivl->parent->parent);
	}
}

void
gtt_interval_set_stop (GttInterval *const ivl, const time_t stop)
{
	g_return_if_fail (NULL != ivl);

	ivl->stop = stop;
	if (stop < ivl->start)
	{
		ivl->start = stop;
	}

	if (ivl->parent)
	{
		gtt_project_refresh_time (ivl->parent->parent);
	}
}

void
gtt_interval_split (GttInterval *ivl, GttTask *newtask)
{
	g_return_if_fail ((NULL != ivl) && (NULL != newtask));

	GttTask *parent = ivl->parent;
	g_return_if_fail (NULL != parent);

	GttProject *prj = parent->parent;
	g_return_if_fail (NULL != prj);

	GList *node = g_list_find (parent->interval_list, ivl);
	g_return_if_fail (NULL != node);

	gtt_task_remove (newtask);

	// Avoid misplaced running intervals, stop the task
	GttInterval *first_ivl    = (GttInterval *) (parent->interval_list->data);
	const gboolean is_running = first_ivl->running;
	if (is_running)
	{
		// Stop shall not be called here to avoid dispatching redraw events
		gtt_project_timer_update (prj);
		first_ivl->running = FALSE;
	}

	// The new task shall be chained in proper order in the parent project
	gint idx = g_list_index (prj->task_list, parent);
	idx++;
	prj->task_list  = g_list_insert (prj->task_list, newtask, idx);
	newtask->parent = prj;

	// The intervals are being re-chained by hand, since GLib does not seem to
	// provide a function for this
	if (node->prev)
	{
		node->prev->next = NULL;
		node->prev       = NULL;
	}
	else
	{
		parent->interval_list = NULL;
	}

	newtask->interval_list = node;

	for (; node; node = node->next)
	{
		GttInterval *nivl = node->data;
		nivl->parent      = newtask;
	}

	if (is_running)
	{
		gtt_project_timer_start (prj);
	}

	gtt_project_refresh_time (parent->parent);
}

GttInterval *
gtt_interval_thaw (GttInterval *ivl)
{
	g_return_val_if_fail ((NULL != ivl) && (NULL != ivl->parent)
														&& (NULL != ivl->parent->parent),
												ivl);

	ivl->parent->parent->frozen = FALSE;
	ivl                         = gtt_task_scrub_intervals (ivl->parent, ivl);
	gtt_project_refresh_time (ivl->parent->parent);

	return ivl;
}
