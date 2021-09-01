/*   project structure handling for GTimeTracker - a time tracker
 *   Copyright (C) 1997,98 Eckehard Berns
 *   Copyright (C) 2001,2003 Linas Vepstas <linas@linas.org>
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

#include <glib.h>
#include <libintl.h> /*conflicts with <libgnome/gnome-i18n.h> on some systems */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <qof.h>

#include "err-throw.h"
#include "internal_data.h"
#include "interval.h"
#include "log.h"
#include "prefs.h"  /* XXX tmp hack for config_* */
#include "proj.h"
#include "proj_p.h"
#include "query.h"  /* temp hack for query */

#define _(X) gettext(X)

/* XXX replace this notifier by GObject signals someday */
typedef struct notif_s
{
	GttProjectChanged func;
	gpointer user_data;
} Notifier;

/* hack alert -- plist should be made to belong to a book
 * And its worse than that ... there code that explicitly assumes
 * there's only one global list, and tha is bad and broken. But
 * there it is.
 */
GttProjectList * global_plist = NULL;

QofBook *global_book = NULL;

static int task_suspend (GttTask *tsk);

/* ============================================================= */

static int next_free_id = 1;

GttProject *
gtt_project_new(void)
{
	GttProject *proj;

	proj = g_new0(GttProject, 1);
	proj->title = g_strdup ("");
	proj->desc = g_strdup ("");
	proj->notes = g_strdup ("");
	proj->custid = NULL;

	proj->min_interval = 3;
	proj->auto_merge_interval = 60;
	proj->auto_merge_gap = 60;

	proj->billrate = 10.0;
	proj->overtime_rate = 15.0;
	proj->overover_rate = 20.0;
	proj->flat_fee = 1000.0;

	proj->estimated_start = -1;
	proj->estimated_end = -1;
	proj->due_date = -1;
	proj->sizing = 0;
	proj->percent_complete = 0;
	proj->urgency = GTT_UNDEFINED;
	proj->importance = GTT_UNDEFINED;
	proj->status = GTT_NOT_STARTED;

	proj->task_list = NULL;
	proj->sub_projects = NULL;
	proj->parent = NULL;
	proj->listeners = NULL;
	proj->private_data = NULL;

	proj->being_destroyed = FALSE;
	proj->frozen = FALSE;
	proj->dirty_time = FALSE;

	proj->secs_ever = 0;
	proj->secs_day = 0;
	proj->secs_yesterday = 0;
	proj->secs_week = 0;
	proj->secs_lastweek = 0;
	proj->secs_month = 0;
	proj->secs_year = 0;

	proj->id = next_free_id;
	next_free_id ++;

	qof_instance_init (&proj->inst, GTT_PROJECT_ID, global_book);
	return proj;
}


GttProject *
gtt_project_new_title_desc(const char *t, const char *d)
{
	GttProject *proj;

	proj = gtt_project_new();
	if (t) { g_free(proj->title); proj->title = g_strdup (t); }
	if (d) { g_free(proj->desc); proj->desc = g_strdup (d); }
	return proj;
}



GttProject *
gtt_project_dup(GttProject *proj)
{
	GList *node;
	GttProject *p;

	if (!proj) return NULL;
	p = gtt_project_new();

	g_free(p->title);
	g_free(p->desc);
	g_free(p->notes);

	p->title = g_strdup(proj->title);
	p->desc = g_strdup(proj->desc);
	p->notes = g_strdup(proj->notes);
	if (proj->custid) p->custid = g_strdup(proj->custid);

	p->min_interval = proj->min_interval;
	p->auto_merge_interval = proj->auto_merge_interval;
	p->auto_merge_gap = proj->auto_merge_gap;

	p->billrate = proj->billrate;
	p->overtime_rate = proj->overtime_rate;
	p->overover_rate = proj->overover_rate;
	p->flat_fee = proj->flat_fee;

	p->estimated_start = proj->estimated_start;
	p->estimated_end = proj->estimated_end;
	p->due_date = proj->due_date;
	p->sizing = proj->sizing;
	p->percent_complete = proj->percent_complete;
	p->urgency = proj->urgency;
	p->importance = proj->importance;
	p->status = proj->status;


	/* Don't copy the tasks.  Do copy the sub-projects */
	for (node=proj->sub_projects; node; node=node->next)
	{
		GttProject *sub = node->data;
		sub = gtt_project_dup (sub);
		gtt_project_append_project (p, sub);
	}

	p->sub_projects = NULL;
	return p;
}


/* remove the project from any lists, etc. */
void
gtt_project_remove(GttProject *p)
{
	/* if we are in someone elses list, remove */
	if (p->parent)
	{
		p->parent->sub_projects =
			g_list_remove (p->parent->sub_projects, p);
		p->parent = NULL;
	}
	else
	{
		/* else we are in the master list ... remove */
		global_plist->prj_list = g_list_remove(global_plist->prj_list, p);
	}
}


void
gtt_project_destroy(GttProject *proj)
{
	if (!proj) return;

	proj->being_destroyed = TRUE;
	gtt_project_remove(proj);

	if (proj->title) g_free(proj->title);
	proj->title = NULL;

	if (proj->desc) g_free(proj->desc);
	proj->desc = NULL;

	if (proj->notes) g_free(proj->desc);
	proj->notes = NULL;

	if (proj->custid) g_free(proj->custid);
	proj->custid = NULL;

	if (proj->task_list)
	{
		while (proj->task_list)
		{
			/* destroying a task pops it off the list */
			gtt_task_destroy (proj->task_list->data);
		}
	}
	proj->task_list = NULL;

	if (proj->sub_projects)
	{
		while (proj->sub_projects)
		{
			/* destroying a sub-project pops it off the list */
			gtt_project_destroy (proj->sub_projects->data);
		}
	}

	/* remove notifiers as well */
	{
		Notifier * ntf;
		GList *node;

		for (node = proj->listeners; node; node=node->next)
		{
			ntf = node->data;
			g_free (ntf);
		}
		if (proj->listeners) g_list_free (proj->listeners);
	}
	proj->private_data = NULL;
	g_free(proj);
}

void
gtt_project_set_guid (GttProject *prj, const GUID *guid)
{
	qof_entity_set_guid (&prj->inst.entity, guid);
}


const GUID *
gtt_project_get_guid (GttProject *prj)
{
	return qof_entity_get_guid (&prj->inst.entity);
}

/* ========================================================= */
/* set/get project titles and descriptions */

void
gtt_project_set_title(GttProject *proj, const char *t)
{
	if (!proj) return;
	if (proj->title) g_free(proj->title);
	if (!t)
	{
		proj->title = g_strdup ("");
		return;
	}
	proj->title = g_strdup(t);
	proj_modified (proj);
}



void
gtt_project_set_desc(GttProject *proj, const char *d)
{
	if (!proj) return;
	if (proj->desc) g_free(proj->desc);
	if (!d)
	{
		proj->desc = g_strdup ("");
		return;
	}
	proj->desc = g_strdup(d);
	proj_modified (proj);
}

void
gtt_project_set_notes(GttProject *proj, const char *d)
{
	if (!proj) return;
	if (proj->notes) g_free(proj->notes);
	if (!d) {
		proj->notes = g_strdup ("");
		return;
	}
	proj->notes = g_strdup(d);
	proj_modified (proj);
}

void
gtt_project_set_custid(GttProject *proj, const char *d)
{
	if (!proj) return;
	if (proj->custid) g_free(proj->custid);
	if (!d)
	{
		proj->custid = NULL;
		return;
	}
	proj->custid = g_strdup(d);
	proj_modified (proj);
}

const char *
gtt_project_get_title (GttProject *proj)
{
	if (!proj) return NULL;
	return (proj->title);
}

const char *
gtt_project_get_desc (GttProject *proj)
{
	if (!proj) return NULL;
	return (proj->desc);
}

const char *
gtt_project_get_notes (GttProject *proj)
{
	if (!proj) return NULL;
	return (proj->notes);
}

const char *
gtt_project_get_custid (GttProject *proj)
{
	if (!proj) return NULL;
	return (proj->custid);
}

void
gtt_project_set_billrate (GttProject *proj, double r)
{
	if (!proj) return;
	proj->billrate = r;
	proj_modified (proj);
}

double
gtt_project_get_billrate (GttProject *proj)
{
	if (!proj) return 0.0;
	return proj->billrate;
}

void
gtt_project_set_overtime_rate (GttProject *proj, double r)
{
	if (!proj) return;
	proj->overtime_rate = r;
	proj_modified (proj);
}

double
gtt_project_get_overtime_rate (GttProject *proj)
{
	if (!proj) return 0.0;
	return proj->overtime_rate;
}

void
gtt_project_set_overover_rate (GttProject *proj, double r)
{
	if (!proj) return;
	proj->overover_rate = r;
	proj_modified (proj);
}

double
gtt_project_get_overover_rate (GttProject *proj)
{
	if (!proj) return 0.0;
	return proj->overover_rate;
}

void
gtt_project_set_flat_fee (GttProject *proj, double r)
{
	if (!proj) return;
	proj->flat_fee = r;
	proj_modified (proj);
}

double
gtt_project_get_flat_fee (GttProject *proj)
{
	if (!proj) return 0.0;
	return proj->flat_fee;
}

void
gtt_project_set_min_interval (GttProject *proj, int r)
{
	if (!proj) return;
	proj->min_interval = r;
	proj_modified (proj);
}

int
gtt_project_get_min_interval (GttProject *proj)
{
	if (!proj) return 3;
	return proj->min_interval;
}

void
gtt_project_set_auto_merge_interval (GttProject *proj, int r)
{
	if (!proj) return;
	proj->auto_merge_interval = r;
	proj_modified (proj);
}

int
gtt_project_get_auto_merge_interval (GttProject *proj)
{
	if (!proj) return 0;
	return proj->auto_merge_interval;
}

void
gtt_project_set_auto_merge_gap (GttProject *proj, int r)
{
	if (!proj) return;
	proj->auto_merge_gap = r;
	proj_modified (proj);
}

int
gtt_project_get_auto_merge_gap (GttProject *proj)
{
	if (!proj) return 0;
	return proj->auto_merge_gap;
}

void
gtt_project_set_private_data (GttProject *proj, gpointer data)
{
	if (!proj) return;
	proj->private_data = data;
}

gpointer
gtt_project_get_private_data (GttProject *proj)
{
	if (!proj) return NULL;
	return proj->private_data;
}

void
gtt_project_set_estimated_start (GttProject *proj, time_t r)
{
	if (!proj) return;
	proj->estimated_start = r;
	proj_modified (proj);
}

time_t
gtt_project_get_estimated_start (GttProject *proj)
{
	if (!proj) return 0;
	return proj->estimated_start;
}

void
gtt_project_set_estimated_end (GttProject *proj, time_t r)
{
	if (!proj) return;
	proj->estimated_end = r;
	proj_modified (proj);
}

time_t
gtt_project_get_estimated_end (GttProject *proj)
{
	if (!proj) return 0;
	return proj->estimated_end;
}

void
gtt_project_set_due_date (GttProject *proj, time_t r)
{
	if (!proj) return;
	proj->due_date = r;
	proj_modified (proj);
}

time_t
gtt_project_get_due_date (GttProject *proj)
{
	if (!proj) return 0;
	return proj->due_date;
}

void
gtt_project_set_sizing (GttProject *proj, int r)
{
	if (!proj) return;
	proj->sizing = r;
	proj_modified (proj);
}

int
gtt_project_get_sizing (GttProject *proj)
{
	if (!proj) return 0;
	return proj->sizing;
}

void
gtt_project_set_percent_complete (GttProject *proj, int r)
{
	if (!proj) return;
	if (0 > r) r = 0;
	if (100 < r) r = 100;
	proj->percent_complete = r;
	proj_modified (proj);
}

int
gtt_project_get_percent_complete (GttProject *proj)
{
	if (!proj) return 0;
	return proj->percent_complete;
}

void
gtt_project_set_urgency (GttProject *proj, GttRank r)
{
	if (!proj) return;
	proj->urgency = r;
	proj_modified (proj);
}

GttRank
gtt_project_get_urgency (GttProject *proj)
{
	if (!proj) return 0;
	return proj->urgency;
}

void
gtt_project_set_importance (GttProject *proj, GttRank r)
{
	if (!proj) return;
	proj->importance = r;
	proj_modified (proj);
}

GttRank
gtt_project_get_importance (GttProject *proj)
{
	if (!proj) return 0;
	return proj->importance;
}

void
gtt_project_set_status (GttProject *proj, GttProjectStatus r)
{
	if (!proj) return;
	proj->status = r;
	proj_modified (proj);
}

GttProjectStatus
gtt_project_get_status (GttProject *proj)
{
	if (!proj) return 0;
	return proj->status;
}

/* =========================================================== */

void
gtt_project_set_id (GttProject *proj, int new_id)
{
	if (!proj) return;

	if (gtt_project_locate_from_id (new_id))
	{
		g_warning ("a project with id =%d already exists\n", new_id);
	}

	/* We try to conserve id numbers by seeing if this was a fresh
	 * project. Conserving numbers is good, since things are less
	 * confusing when looking at the data. */
	if ((proj->id+1) == next_free_id) next_free_id--;

	proj->id = new_id;
	if (new_id >= next_free_id) next_free_id = new_id + 1;
}

int
gtt_project_get_id (GttProject *proj)
{
	if (!proj) return -1;
	return proj->id;
}

/* =========================================================== */
/* compatibility interface */

void
gtt_project_compat_set_secs (GttProject *proj, int sever, int sday, time_t last)
{
	time_t midnight;
	GttTask *tsk;
	GttInterval *ivl;

	/* Get the midnight of the last update */
	midnight = get_midnight (last);

	/* Old GTT data will just be one big interval
	 * lumped under one task */
	tsk = gtt_task_new ();
	gtt_task_set_memo (tsk, _("Old GTT Tasks"));

	ivl = gtt_interval_new();
	ivl->stop = midnight - 1;
	ivl->start = midnight - 1 - sever + sday;
	gtt_task_add_interval (tsk, ivl);

	if (0 < sday)
	{
		ivl = gtt_interval_new();
		ivl->start = midnight +1;
		ivl->stop = midnight + sday + 1;
		gtt_task_add_interval (tsk, ivl);
	}

	gtt_project_append_task (proj, tsk);

	/* All new data will get its own task */
	tsk = gtt_task_new ();
	gtt_task_set_memo (tsk, _("New Diary Entry"));
	gtt_project_append_task (proj, tsk);

	proj_refresh_time (proj);
}

/* =========================================================== */
/* add subprojects, tasks */

void
gtt_project_append_project (GttProject *proj, GttProject *child)
{
	if (!child) return;
	gtt_project_remove(child);
	if (!proj)
	{
		/* if no parent specified, put in top-level list */
		gtt_project_list_append (global_plist, child);
		return;
	}

	proj->sub_projects = g_list_append (proj->sub_projects, child);
	child->parent = proj;
}

void
gtt_project_insert_before(GttProject *p, GttProject *before_me)
{
	gint pos;

	if (!p) return;
	gtt_project_remove(p);

	/* no before ?? then append to master list */
	if (!before_me)
	{
		global_plist->prj_list = g_list_append (global_plist->prj_list, p);
		p->parent = NULL;
		return;
	}
	else
	{
		if (!before_me->parent)
		{
			/* if before_me has no parent, then its in the
			 * master list */
			pos  = g_list_index (global_plist->prj_list, before_me);

			/* this shouldn't happen ....node should be found */
			if (0 > pos)
			{
				global_plist->prj_list = g_list_append (global_plist->prj_list, p);
				p->parent = NULL;
				return;
			}
			global_plist->prj_list = g_list_insert (global_plist->prj_list, p, pos);
			p->parent = NULL;
			return;
		}
		else
		{
			GList *sub;
			sub = before_me->parent->sub_projects;
			pos  = g_list_index (sub, before_me);

			/* this shouldn't happen ....node should be found */
			if (0 > pos)
			{
				sub = g_list_append (sub, p);
				before_me->parent->sub_projects = sub;
				p->parent = before_me->parent;
				return;
			}
			sub = g_list_insert (sub, p, pos);
			before_me->parent->sub_projects = sub;
			p->parent = before_me->parent;
			return;
		}
	}
}

void
gtt_project_insert_after(GttProject *p, GttProject *after_me)
{
	gint pos;

	if (!p) return;
	gtt_project_remove(p);

	/* no after ?? then prepend to master list */
	if (!after_me)
	{
		global_plist->prj_list = g_list_prepend (global_plist->prj_list, p);
		p->parent = NULL;
		return;
	}
	else
	{
		if (!after_me->parent)
		{
			/* if after_me has no parent, then its in the
			 * master list */
			pos  = g_list_index (global_plist->prj_list, after_me);

			/* this shouldn't happen ....node should be found */
			if (0 > pos)
			{
				global_plist->prj_list = g_list_prepend (global_plist->prj_list, p);
				p->parent = NULL;
				return;
			}
			pos ++;
			global_plist->prj_list = g_list_insert (global_plist->prj_list, p, pos);
			p->parent = NULL;
			return;
		}
		else
		{
			GList *sub;
			sub = after_me->parent->sub_projects;
			pos  = g_list_index (sub, after_me);

			/* this shouldn't happen ....node should be found */
			if (0 > pos)
			{
				sub = g_list_prepend (sub, p);
				after_me->parent->sub_projects = sub;
				p->parent = after_me->parent;
				return;
			}
			pos ++;
			sub = g_list_insert (sub, p, pos);
			after_me->parent->sub_projects = sub;
			p->parent = after_me->parent;
			return;
		}
	}
}

void
gtt_project_reparent (GttProject *proj, GttProject *parent, int position)
{
	GList **new_list = NULL;
	GList **old_list = NULL;
	if (parent)
	{
		new_list = &parent->sub_projects;
	}
	else
	{
		new_list = &global_plist->prj_list;
	}

	if (proj->parent)
	{
		old_list = &proj->parent->sub_projects;
	}
	else
	{
		old_list = &global_plist->prj_list;
	}

	*old_list = g_list_remove (*old_list, proj);
	*new_list = g_list_insert (*new_list, proj, position);
	proj->parent = parent;

}


void
gtt_project_append_task (GttProject *proj, GttTask *task)
{
	if (!proj || !task) return;

	/* if task has a different parent, then reparent */
	if (task->parent)
	{
		task->parent->task_list =
			g_list_remove (task->parent->task_list, task);
		proj_refresh_time(task->parent);
	}

	proj->task_list = g_list_append (proj->task_list, task);
	task->parent = proj;
	proj_refresh_time(proj);
}

void
gtt_project_prepend_task (GttProject *proj, GttTask *task)
{
	int is_running = FALSE;
	if (!proj || !task) return;

	/* if task has a different parent, then reparent */
	if (task->parent)
	{
		task->parent->task_list =
			g_list_remove (task->parent->task_list, task);
		proj_refresh_time(task->parent);
	}

	/* avoid misplaced running intervals, stop the task */
	if (proj->task_list)
	{
		GttTask *leadtask = proj->task_list->data;
		is_running = task_suspend (leadtask);
	}

	proj->task_list = g_list_prepend (proj->task_list, task);
	proj->current_task = task;
	task->parent = proj;

	if (is_running) gtt_project_timer_start (proj);
	proj_refresh_time(proj);
}


GList *
gtt_project_get_children (GttProject *proj)
{
	if (!proj) return NULL;
	return proj->sub_projects;
}

GttProject *
gtt_project_get_parent (GttProject *proj)
{
	if (!proj) return NULL;
	return proj->parent;
}


GList *
gtt_project_get_tasks (GttProject *proj)
{
	if (!proj) return NULL;
	return proj->task_list;
}

GttTask *
gtt_project_get_first_task (GttProject *proj)
{
	if (!proj) return NULL;
	if (!proj->task_list) return NULL;
	return (proj->task_list->data);
}

GttTask *
gtt_project_get_current_task (GttProject *proj)
{
	if (!proj) return NULL;
	if (!proj->current_task)
		return gtt_project_get_first_task(proj);

	return (proj->current_task);
}

void
gtt_project_set_current_task (GttProject *proj, GttTask *newCurrentTask)
{
	int is_running = 0;

	if (!proj || !newCurrentTask) return;

	if (proj->current_task)
		is_running = task_suspend(proj->current_task);
	proj->current_task = newCurrentTask;
	if (is_running) gtt_project_timer_start(proj);
	proj_refresh_time(proj);
}

GttInterval *
gtt_project_get_first_interval (GttProject *proj)
{
	GttTask *tsk;
	if (!proj) return NULL;
	if (!proj->task_list) return NULL;
	tsk = proj->task_list->data;
	if (!tsk) return NULL;
	if (!tsk->interval_list) return NULL;
	return (tsk->interval_list->data);
}

/* =========================================================== */
/* get totals */


int
gtt_project_foreach (GttProject *prj, GttProjectCB cb, gpointer data)
{
	int rc;
	GList *node;
	if (!prj || !cb) return 0;

	rc = cb (prj, data);
	if (0 == rc) return 0;

	for (node=prj->sub_projects; node; node=node->next)
	{
		GttProject *subprj = node->data;
		if (!subprj) continue;  /* this can't really happen */
		rc = gtt_project_foreach (subprj, cb, data);
		if (0 == rc) return 0;
	}
	return rc;
}

static int
prj_total_secs_day (GttProject *prj, gpointer data)
{
	*((int *) data) += prj->secs_day;
	return 1;
}

static int
prj_total_secs_yesterday (GttProject *prj, gpointer data)
{
	*((int *) data) += prj->secs_yesterday;
	return 1;
}

static int
prj_total_secs_week (GttProject *prj, gpointer data)
{
	*((int *) data) += prj->secs_week;
	return 1;
}

static int
prj_total_secs_lastweek (GttProject *prj, gpointer data)
{
	*((int *) data) += prj->secs_lastweek;
	return 1;
}

static int
prj_total_secs_month (GttProject *prj, gpointer data)
{
	*((int *) data) += prj->secs_month;
	return 1;
}

static int
prj_total_secs_year (GttProject *prj, gpointer data)
{
	*((int *) data) += prj->secs_year;
	return 1;
}

static int
prj_total_secs_ever (GttProject *prj, gpointer data)
{
	*((int *) data) += prj->secs_ever;
	return 1;
}

static int
prj_total_secs_current (GttProject *prj, gpointer data)
{
	*((int *) data) += gtt_project_get_secs_current (prj);
	return 1;
}

/* this routine adds up total day-secs for this project, and its sub-projects */
int
gtt_project_total_secs_day (GttProject *proj)
{
	int total = 0;
	if (!proj) return 0;
	gtt_project_foreach (proj, prj_total_secs_day, &total);
	return total;
}

int
gtt_project_total_secs_yesterday (GttProject *proj)
{
	int total = 0;
	if (!proj) return 0;
	gtt_project_foreach (proj, prj_total_secs_yesterday, &total);
	return total;
}

int
gtt_project_total_secs_week (GttProject *proj)
{
	int total = 0;
	if (!proj) return 0;
	gtt_project_foreach (proj, prj_total_secs_week, &total);
	return total;
}

int
gtt_project_total_secs_lastweek (GttProject *proj)
{
	int total = 0;
	if (!proj) return 0;
	gtt_project_foreach (proj, prj_total_secs_lastweek, &total);
	return total;
}

int
gtt_project_total_secs_month (GttProject *proj)
{
	int total = 0;
	if (!proj) return 0;
	gtt_project_foreach (proj, prj_total_secs_month, &total);
	return total;
}

int
gtt_project_total_secs_year (GttProject *proj)
{
	int total = 0;
	if (!proj) return 0;
	gtt_project_foreach (proj, prj_total_secs_year, &total);
	return total;
}

int
gtt_project_total_secs_ever (GttProject *proj)
{
	int total = 0;
	if (!proj) return 0;
	gtt_project_foreach (proj, prj_total_secs_ever, &total);
	return total;
}

int
gtt_project_total_secs_current (GttProject *proj)
{
	int total = 0;
	if (!proj) return 0;
	gtt_project_foreach (proj, prj_total_secs_current, &total);
	return total;
}

int
gtt_project_get_secs_day (GttProject *proj)
{
	if (!proj) return 0;
	return proj->secs_day;
}

int
gtt_project_get_secs_yesterday (GttProject *proj)
{
	if (!proj) return 0;
	return proj->secs_yesterday;
}

int
gtt_project_get_secs_week (GttProject *proj)
{
	if (!proj) return 0;
	return proj->secs_week;
}

int
gtt_project_get_secs_lastweek (GttProject *proj)
{
	if (!proj) return 0;
	return proj->secs_lastweek;
}

int
gtt_project_get_secs_month (GttProject *proj)
{
	if (!proj) return 0;
	return proj->secs_month;
}

int
gtt_project_get_secs_year (GttProject *proj)
{
	if (!proj) return 0;
	return proj->secs_year;
}

int
gtt_project_get_secs_ever (GttProject *proj)
{
	if (!proj) return 0;
	return proj->secs_ever;
}

int
gtt_project_get_secs_current (GttProject *proj)
{
	GttTask *tsk;
	if (!proj) return 0;
	if (!proj->task_list) return 0;
	tsk = gtt_project_get_current_task (proj);

	return gtt_task_get_secs_ever(tsk);
}

/* This routine adds up total number of projects, and sub-projects */
static int
prj_total (GttProject *prj, gpointer data)
{
	*((int *) data) += 1;  /* just count one */
	return 1;
}

int
gtt_project_total (GttProject *proj)
{
	int total = 0;
	if (!proj) return 0;
	gtt_project_foreach (proj, prj_total, &total);
	return total;
}

/* =========================================================== */

int
gtt_project_list_total_secs_day (void)
{
	GList *node;
	int total = 0;
	if (!global_plist->prj_list) return 0;

	for (node=global_plist->prj_list; node; node=node->next)
	{
		GttProject *proj = node->data;
		total += gtt_project_total_secs_day (proj);
	}
	return total;
}

int
gtt_project_list_total_secs_yesterday (void)
{
	GList *node;
	int total = 0;
	if (!global_plist->prj_list) return 0;

	for (node=global_plist->prj_list; node; node=node->next)
	{
		GttProject *proj = node->data;
		total += gtt_project_total_secs_yesterday (proj);
	}
	return total;
}

int
gtt_project_list_total (void)
{
	GList *node;
	int total = 0;
	if (!global_plist->prj_list) return 0;

	for (node=global_plist->prj_list; node; node=node->next)
	{
		GttProject *proj = node->data;
		total += gtt_project_total (proj);
	}
	return total;
}

static void
children_modified (GttProject *prj)
{
	GList *node;
	if (!prj) return;
	for (node= prj->sub_projects; node; node=node->next)
	{
		GttProject * subprj = node->data;
		children_modified (subprj);
	}
	proj_modified (prj);
}

void
gtt_project_list_compute_secs (void)
{
	GList *node;
	for (node= global_plist->prj_list; node; node=node->next)
	{
		GttProject * prj = node->data;
		proj_refresh_time (prj);
		children_modified (prj);
	}
}

/* =========================================================== */
/* even notification subsystem */

void
gtt_project_add_notifier (GttProject *prj,
                       GttProjectChanged cb,
                       gpointer user_stuff)
{
	Notifier * ntf;

	if (!prj || !cb) return;

	ntf = g_new0 (Notifier, 1);
	ntf->func = cb;
	ntf->user_data = user_stuff;
	prj->listeners = g_list_append (prj->listeners, ntf);
}

void
gtt_project_remove_notifier (GttProject *prj,
                       GttProjectChanged cb,
                       gpointer user_stuff)
{
	Notifier * ntf;
	GList *node;

	if (!prj || !cb) return;

	for (node = prj->listeners; node; node=node->next)
	{
		ntf = node->data;
		if ((ntf->func == cb) && (ntf->user_data == user_stuff)) break;
	}

	if (node)
	{
		prj->listeners = g_list_remove (prj->listeners, ntf);
		g_free (ntf);
	}
}

void
gtt_project_freeze (GttProject *prj)
{
	if (!prj) return;
	prj->frozen = TRUE;
}

void
gtt_project_thaw (GttProject *prj)
{
	if (!prj) return;
	prj->frozen = FALSE;
	proj_refresh_time (prj);
}

void
gtt_task_freeze (GttTask *tsk)
{
	if (!tsk ||!tsk->parent) return;
	tsk->parent->frozen = TRUE;
}

void
gtt_task_thaw (GttTask *tsk)
{
	if (!tsk ||!tsk->parent) return;
	tsk->parent->frozen = FALSE;
	proj_refresh_time (tsk->parent);
}

/* =========================================================== */

void
gtt_clear_daily_counter (GttProject *proj)
{
	time_t midnight;
	int not_done = TRUE;
	GList *tsk_node, *in;
	int is_running;

	if (!proj) return;
	if (!proj->task_list) return;

	is_running = task_suspend ((GttTask *) proj->task_list->data);

	gtt_project_freeze (proj);
	midnight = get_midnight (-1);

	/* loop over tasks */
	for (tsk_node= proj->task_list; tsk_node; tsk_node=tsk_node->next)
	{
		GttTask * task = tsk_node->data;

		not_done = TRUE;
		while (not_done)
		{
			not_done = FALSE;
			for (in= task->interval_list; in; in=in->next)
			{
				GttInterval *ivl = in->data;

				/* only nuke the ones that started after midnight.
			 	 * The ones that started before midnight remain */
				if (ivl->start >= midnight)
				{
					task->interval_list =
					     g_list_remove (task->interval_list, ivl);
					g_free (ivl);
					not_done = TRUE;
					break;
				}
			}
		}
	}
	gtt_project_thaw (proj);
	if (is_running) gtt_project_timer_start (proj);
}

/* =========================================================== */

void
gtt_project_timer_start (GttProject *proj)
{
	GttTask *task;
	GttInterval *ival;
	time_t now;

	if (!proj) return;

	/* By definition, the current task is not anymore the one at the head
	 * of the list, but rather the one denoted by GttProject->current_task
	 * and the current interval is  at the head
	 * of the task */
	if (NULL == proj->task_list)
	{
		task = gtt_task_new();
		gtt_task_set_memo (task, _("New Diary Entry"));
		proj->current_task = task;
	}
	else
	{
		if (NULL == proj->current_task)
		{
			task = proj->task_list->data;
			proj->current_task = task;
		}
		else
		{
			task = proj->current_task;
		}

		g_return_if_fail (task);
	}

	now = time(0);

	/* only add a new interval if there's been a bit of a gap,
	 * otherwise, reuse the most recent running interval.  */
	if (task->interval_list)
	{
		int delta;
		ival = task->interval_list->data;
		delta = now - ival->stop;

		if (delta <= proj->auto_merge_gap)
		{

			ival->start += delta;
			ival->fuzz += delta;
			ival->stop = now;
			ival->running = TRUE;
			return;
		}
	}

	ival = g_new0 (GttInterval, 1);
	ival->start = now;
	ival->stop = ival->start;
	ival->running = TRUE;
	task->interval_list = g_list_prepend (task->interval_list, ival);
	ival->parent = task;

	/* don't add the task until after we've done above */
	if (NULL == proj->task_list)
	{
		gtt_project_append_task (proj, task);
	}
}

void
gtt_project_timer_update (GttProject *proj)
{
	GttTask *task;
	GttInterval *ival;
	time_t prev_update, now, diff;

	if (!proj) return;
	g_return_if_fail (proj->task_list);
	g_return_if_fail (proj->current_task);

	/* By definition, the current task is not anymore the one at the head
	 * of the list, but rather the one denoted by GttProject->current_task
	 * and the current interval is  at the head
	 * of the task */
	task = proj->current_task;
	g_return_if_fail (task);

	/* Its possible that there are no intervals (which implies
	 * that the timer isn't running). */
	if (NULL == task->interval_list) return;
	ival = task->interval_list->data;
	g_return_if_fail (ival);

	/* If timer isn't running, do nothing.  Normally,
	 * this function should never be called when timer is stopped,
	 * but there are a few rare cases (e.g. clear daily counter).
	 * where it is.
	 */
	if (FALSE == ival->running) return;

	/* compute the delta change, update cached data */
	now = time(0);

	/* hack alert -- why aren't we checking a midnight rollover? */
	prev_update = ival->stop;
	ival->stop = now;
	diff = now - prev_update;
	proj->secs_ever += diff;
	proj->secs_day += diff;
	proj->secs_week += diff;
	proj->secs_month += diff;
	proj->secs_year += diff;
}

void
gtt_project_timer_stop (GttProject *proj)
{
	GttTask *task;
	GttInterval *ival;

	if (!proj) return;
	if (!proj->current_task) return;

	gtt_project_timer_update (proj);

	/* Also note that the timer really has stopped. */
	task = proj->current_task;
	g_return_if_fail (task);

	/* its 'legal' to have no intervals, which implies
	 * that the timer isn't running anyway. */
	if (task->interval_list)
	{
		ival = task->interval_list->data;
		g_return_if_fail (ival);
		ival->running = FALSE;
	}

	/* When we stop the timer, call proj_refresh_time(),
	 * as this will do several things: first, it will
	 * scrub away short intervals and/or short gaps,
	 * and, secondly, it will force redraws of affected
	 * windows so that the old data doesn't show.
	 */
	proj_refresh_time (proj);
}

/* =========================================================== */

static void
prj_obj_foreach_helper (GList *prjlist, QofEntityForeachCB cb, gpointer data)
{
	GList *node;

	for (node=prjlist; node; node=node->next)
	{
		GttProject *prj = node->data;
		cb ((QofEntity *) prj, data);
		prj_obj_foreach_helper (prj->sub_projects, cb, data);
	}
}

void
gtt_project_obj_foreach (QofCollection *coll, QofEntityForeachCB cb, gpointer data)
{
	prj_obj_foreach_helper (global_plist->prj_list, cb, data);
}

static int
prj_obj_order (GttProject *a, GttProject *b)
{
	return 0;
}

static QofObject prj_object_def =
{
interface_version: QOF_OBJECT_VERSION,
e_type:            GTT_PROJECT_ID,
type_label:        GTT_PROJECT_ID,
create:            NULL,
book_begin:        NULL,
book_end:          NULL,
is_dirty:          NULL,
mark_clean:        NULL,
foreach:           gtt_project_obj_foreach,
printable:         NULL,
};

/* =========================================================== */
/* bogus wrappers */
static QofTime *
prj_obj_get_earliest (GttProject *prj, QofParam *qpm)
{
	// static time return, not thread safe, but a hack that will do for now.
	static QofTime *qt = NULL;
	if (NULL == qt) qt = qof_time_new();
	time_t gt = gtt_project_get_earliest_start (prj, TRUE);
	qof_time_set_secs (qt, gt);
	return qt;
}

static QofTime *
prj_obj_get_latest (GttProject *prj, QofParam *qpm)
{
	static QofTime *qt = NULL;
	if (NULL == qt) qt = qof_time_new();
	time_t gt = gtt_project_get_latest_stop (prj, TRUE);
	qof_time_set_secs (qt, gt);
	return qt;
}

gboolean
gtt_project_obj_register (void)
{
	global_book = qof_book_new();

/* Associate an ASCII name to each getter, as well as the return type */
static QofParam params[] = {
		{ GTT_PROJECT_EARLIEST, QOF_TYPE_TIME, (QofAccessFunc)prj_obj_get_earliest, NULL},
		{ GTT_PROJECT_LATEST, QOF_TYPE_TIME, (QofAccessFunc)prj_obj_get_latest, NULL},
		{ NULL },
	};

	qof_class_register (GTT_PROJECT_ID, (QofSortFunc)prj_obj_order, params);
	return qof_object_register (&prj_object_def);
}

/* =========================================================== */

GttTask *
gtt_task_new (void)
{
	GttTask *task;

	task = g_new0(GttTask, 1);
	task->parent = NULL;
	task->memo = g_strdup (_("New Diary Entry"));
	task->notes = g_strdup ("");
	task->billable = GTT_BILLABLE;
	task->billrate = GTT_REGULAR;
	task->billstatus = GTT_BILL,
	task->bill_unit = 900;
	task->interval_list = NULL;

	qof_instance_init (&task->inst, GTT_TASK_ID, global_book);
	return task;
}

GttTask *
gtt_task_copy (GttTask *old)
{
	GttTask *task;

	if (!old) return gtt_task_new();

	task = g_new0(GttTask, 1);
	task->parent = NULL;
	task->memo = g_strdup (old->memo);
	task->notes = g_strdup (old->notes);

	/* inherit the properties ... important for user */
	task->billable = old->billable;
	task->billrate = old->billrate;
	task->billstatus = old->billstatus;
	task->bill_unit = old->bill_unit;
	task->interval_list = NULL;

	qof_instance_init (&task->inst, GTT_TASK_ID, global_book);
	return task;
}

static int
task_suspend (GttTask *tsk)
{
	int is_running = 0;

	/* avoid misplaced running intervals, stop the task */
	if (tsk->interval_list)
	{
		GttInterval *first_ivl;
		first_ivl = (GttInterval *) (tsk->interval_list->data);
		is_running = first_ivl -> running;
		if (is_running)
		{
			/* don't call stop here, avoid dispatching redraw events */
			gtt_project_timer_update (tsk->parent);
			first_ivl->running = FALSE;
		}
	}
	return is_running;
}


void
gtt_task_destroy (GttTask *task)
{
	int is_running;
	if (!task) return;

	is_running = task_suspend (task);
	if (task->parent)
	{
		task->parent->task_list =
			g_list_remove (task->parent->task_list, task);
		if (is_running) gtt_project_timer_start (task->parent);
		proj_refresh_time (task->parent);
		task->parent = NULL;
	}

	if (task->memo) g_free(task->memo);
	task->memo = NULL;
	if (task->notes) g_free(task->notes);
	task->notes = NULL;
	if (task->interval_list)
	{
		GList *node;
		for (node = task->interval_list; node; node=node->next)
		{
			/* free the individual intervals */
			g_free (node->data);
		}
		g_list_free (task->interval_list);
		task->interval_list = NULL;
	}
}

void
gtt_task_remove (GttTask *task)
{
	int is_running;
	if (!task) return;

	is_running = task_suspend (task);

	GttProject *project = task->parent;

	if (project)
	{
		project->task_list =
			g_list_remove (project->task_list, task);
		gtt_project_set_current_task(project,
				gtt_project_get_first_task(project));
		if (is_running) gtt_project_timer_start (project);
		proj_refresh_time (project);
		task->parent = NULL;
	}
}


void
gtt_task_insert (GttTask *where, GttTask *insertee)
{
	GttProject *prj;
	int idx;
	int is_running = 0;

	if (!where || !insertee) return;

	prj = where->parent;
	if (!prj) return;

	gtt_task_remove (insertee);
	is_running = task_suspend (where);

	insertee->parent = prj;
	idx = g_list_index (prj->task_list, where);
	prj->task_list = g_list_insert (prj->task_list, insertee, idx);

	if (is_running) gtt_project_timer_start (prj);
	proj_refresh_time (prj);
}

GttTask *
gtt_task_new_insert (GttTask *old)
{
	int is_running = 0;
	GttProject *prj;
	GttTask *task;
	int idx;

	if (!old) return NULL;

	task = gtt_task_new();

	/* Inherit the properties ... important for user */
	task->billable = old->billable;
	task->billrate = old->billrate;
	task->billstatus = old->billstatus;
	task->bill_unit = old->bill_unit;
	task->interval_list = NULL;

	/* chain into place */
	prj = old->parent;
	task->parent = prj;
	if (!prj) return task;

	/* avoid misplaced running intervals, stop the task */
	is_running = task_suspend (old);

	idx = g_list_index (prj->task_list, old);
	prj->task_list = g_list_insert (prj->task_list, task, idx);
	prj->current_task = task;

	if (is_running) gtt_project_timer_start (prj);
	proj_refresh_time (prj);
	return task;
}

void
gtt_task_set_guid (GttTask *tsk, const GUID *guid)
{
	qof_entity_set_guid (&tsk->inst.entity, guid);
}

const GUID *
gtt_task_get_guid (GttTask *tsk)
{
	return qof_entity_get_guid (&tsk->inst.entity);
}

void
gtt_task_add_interval (GttTask *tsk, GttInterval *ival)
{
	if (!tsk || !ival) return;
	gtt_interval_unhook (ival);
	tsk->interval_list = g_list_prepend (tsk->interval_list, ival);
	ival->parent = tsk;
	proj_refresh_time (tsk->parent);
}

void
gtt_task_append_interval (GttTask *tsk, GttInterval *ival)
{
	if (!tsk || !ival) return;
	gtt_interval_unhook (ival);
	tsk->interval_list = g_list_append (tsk->interval_list, ival);
	ival->parent = tsk;
	proj_refresh_time (tsk->parent);
}

void
gtt_task_set_memo(GttTask *tsk, const char *m)
{
	if (!tsk) return;
	if (tsk->memo) g_free(tsk->memo);
	if (!m)
	{
		tsk->memo = g_strdup ("");
		return;
	}
	tsk->memo = g_strdup(m);
	proj_modified (tsk->parent);
}

void
gtt_task_set_notes(GttTask *tsk, const char *m)
{
	if (!tsk) return;
	if (tsk->notes) g_free(tsk->notes);
	if (!m)
	{
		tsk->notes = g_strdup ("");
		return;
	}
	tsk->notes = g_strdup(m);
	proj_modified (tsk->parent);
}

const char *
gtt_task_get_memo (GttTask *tsk)
{
	if (!tsk) return NULL;
	return tsk->memo;
}

const char *
gtt_task_get_notes (GttTask *tsk)
{
	if (!tsk) return NULL;
	return tsk->notes;
}

void
gtt_task_set_billable (GttTask *tsk, GttBillable b)
{
	if (!tsk) return;
	tsk->billable = b;
	proj_modified (tsk->parent);
}

GttBillable
gtt_task_get_billable (GttTask *tsk)
{
	if (!tsk) return GTT_NOT_BILLABLE;
	return tsk->billable;
}

void
gtt_task_set_billrate (GttTask *tsk, GttBillRate b)
{
	if (!tsk) return;
	tsk->billrate = b;
	proj_modified (tsk->parent);
}

GttBillRate
gtt_task_get_billrate (GttTask *tsk)
{
	if (!tsk) return GTT_REGULAR;
	return tsk->billrate;
}

void
gtt_task_set_billstatus (GttTask *tsk, GttBillStatus b)
{
	if (!tsk) return;
	tsk->billstatus = b;
	proj_modified (tsk->parent);
}

GttBillStatus
gtt_task_get_billstatus (GttTask *tsk)
{
	if (!tsk) return GTT_HOLD;
	return tsk->billstatus;
}

void
gtt_task_set_bill_unit (GttTask *tsk, int b)
{
	if (!tsk) return;
	tsk->bill_unit = b;
	proj_modified (tsk->parent);
}

int
gtt_task_get_bill_unit (GttTask *tsk)
{
	if (!tsk) return GTT_REGULAR;
	return tsk->bill_unit;
}

GList *
gtt_task_get_intervals (GttTask *tsk)
{
	if (!tsk) return NULL;
	return tsk->interval_list;
}

gboolean
gtt_task_is_first_task (GttTask *tsk)
{
	if (!tsk || !tsk->parent || !tsk->parent->task_list) return TRUE;

	if ((GttTask *) tsk->parent->task_list->data == tsk) return TRUE;
	return FALSE;
}

gboolean
gtt_task_is_last_task (GttTask *tsk)
{
	if (!tsk || !tsk->parent || !tsk->parent->task_list) return TRUE;

	GList *last = g_list_last (tsk->parent->task_list);
	if ((GttTask *) last->data == tsk) return TRUE;
	return FALSE;
}

GttProject *
gtt_task_get_parent (GttTask *tsk)
{
	if (!tsk) return NULL;
	return tsk->parent;
}

/* =========================================================== */

void
gtt_task_merge_up (GttTask *tsk)
{
	GttTask *mtask;
	GList * node;
	GttProject *prj;
	if (!tsk) return;

	prj = tsk->parent;
	if (!prj || !prj->task_list) return;

	node = g_list_find (prj->task_list, tsk);
	if (!node) return;

	node = node->prev;
	if (!node) return;

	mtask = node->data;

	for (node=tsk->interval_list; node; node=node->next)
	{
		GttInterval *ivl = node->data;
		ivl->parent = mtask;
	}
	mtask->interval_list = g_list_concat (mtask->interval_list, tsk->interval_list);
	tsk->interval_list = NULL;
}

/* =========================================================== */

int
gtt_task_get_secs_ever (GttTask *tsk)
{
	GList *node;
	int total = 0;

	for (node=tsk->interval_list; node; node=node->next)
	{
		GttInterval * ivl = node->data;
		total += ivl->stop - ivl->start;
	}
	return total;
}

time_t
gtt_task_get_secs_earliest (GttTask *tsk)
{
	GList *node;
	if (NULL == tsk->interval_list) return 0;

	time_t earliest = INT_MAX;

	for (node=tsk->interval_list; node; node=node->next)
	{
		GttInterval * ivl = node->data;
		if (ivl->start < earliest) earliest = ivl->start;
	}
	return earliest;
}

time_t
gtt_task_get_secs_latest (GttTask *tsk)
{
	GList *node;
	if (NULL == tsk->interval_list) return 0;

	time_t latest = INT_MIN;

	for (node=tsk->interval_list; node; node=node->next)
	{
		GttInterval * ivl = node->data;
		if (ivl->stop > latest) latest = ivl->stop;
	}
	return latest;
}

/* =========================================================== */

/* ============================================================= */

/* ============================================================= */
/* project_list -- simple wrapper around glib routines */
/* hack alert -- this list should probably be replaced with a
 * GNode tree ... */


GttProjectList *
gtt_project_list_new (void)
{
	GttProjectList *gpl;
	gpl = g_new0(GttProjectList, 1);
	gpl->prj_list = NULL;
	global_plist = gpl;   /* XXX one of many nasty hacks */
	return gpl;
}

GList *
gtt_project_list_get_list (GttProjectList *gpl)
{
	return gpl->prj_list;
}

void
gtt_project_list_append(GttProjectList *gpl, GttProject *p)
{
	gtt_project_insert_before (p, NULL);
}


void
gtt_project_list_destroy(GttProjectList *gpl)
{
	while (gpl->prj_list)
	{
		/* destroying a project pops the plist ... */
		gtt_project_destroy(gpl->prj_list->data);
	}

	g_free (gpl);
}

static GList *
project_list_sort(GList *prjs, int (cmp)(const void *, const void *))
{
	GList *node;
	prjs = g_list_sort (prjs, cmp);
	for (node = prjs; node; node=node->next)
	{
		GttProject *prj = node->data;
		if (prj->sub_projects)
		{
			prj->sub_projects = project_list_sort (prj->sub_projects, cmp);
		}
	}
	return prjs;
}

/* -------------------- */
/* given id, walk the tree of projects till we find it. */

static GttProject *
locate_from_id (GList *prj_list, int prj_id)
{
	GList *node;
	for (node = prj_list; node; node=node->next)
	{
		GttProject *prj = node->data;
		if (prj_id == prj->id) return prj;

		/* recurse to handle sub-projects */
		if (prj->sub_projects)
		{
			prj = locate_from_id (prj->sub_projects, prj_id);
			if (prj) return prj;
		}
	}
	return NULL;  /* not found */
}

GttProject *
gtt_project_locate_from_id (int prj_id)
{
	return locate_from_id (global_plist->prj_list, prj_id);
}

/* ==================================================================== */
/* sort funcs */


static int
cmp_day(const void *aa, const void *bb)
{
	GttProject *a = (GttProject *)aa;
	GttProject *b = (GttProject *)bb;
	return (gtt_project_total_secs_day(b) - gtt_project_total_secs_day(a));
}

static int
cmp_yesterday(const void *aa, const void *bb)
{
	GttProject *a = (GttProject *)aa;
	GttProject *b = (GttProject *)bb;
	return (gtt_project_total_secs_yesterday(b) - gtt_project_total_secs_yesterday(a));
}

static int
cmp_week(const void *aa, const void *bb)
{
	GttProject *a = (GttProject *)aa;
	GttProject *b = (GttProject *)bb;
	return (gtt_project_total_secs_week(b) - gtt_project_total_secs_week(a));
}

static int
cmp_lastweek(const void *aa, const void *bb)
{
	GttProject *a = (GttProject *)aa;
	GttProject *b = (GttProject *)bb;
	return (gtt_project_total_secs_lastweek(b) - gtt_project_total_secs_lastweek(a));
}

static int
cmp_month(const void *aa, const void *bb)
{
	GttProject *a = (GttProject *)aa;
	GttProject *b = (GttProject *)bb;
	return (gtt_project_total_secs_month(b) - gtt_project_total_secs_month(a));
}

static int
cmp_year(const void *aa, const void *bb)
{
	GttProject *a = (GttProject *)aa;
	GttProject *b = (GttProject *)bb;
	return (gtt_project_total_secs_year(b) - gtt_project_total_secs_year(a));
}

static int
cmp_ever(const void *aa, const void *bb)
{
	GttProject *a = (GttProject *)aa;
	GttProject *b = (GttProject *)bb;
	return (gtt_project_total_secs_ever(b) - gtt_project_total_secs_ever(a));
}

static int
cmp_current(const void *aa, const void *bb)
{
	GttProject *a = (GttProject *)aa;
	GttProject *b = (GttProject *)bb;
	return (gtt_project_total_secs_current(b) - gtt_project_total_secs_current(a));
}


static int
cmp_title(const void *aa, const void *bb)
{
	const GttProject *a = aa;
	const GttProject *b = bb;
	return strcmp(a->title, b->title);
}

static int
cmp_desc(const void *aa, const void *bb)
{
	const GttProject *a = aa;
	const GttProject *b = bb;
	if (!a->desc) {
		return (b->desc == NULL)? 0 : 1;
	}
	if (!b->desc) {
		return -1;
	}
	return strcmp(a->desc, b->desc);
}

static int
cmp_start(const void *aa, const void *bb)
{
	const GttProject *a = aa;
	const GttProject *b = bb;
	return (b->estimated_start - a->estimated_start);
}

static int
cmp_end(const void *aa, const void *bb)
{
	const GttProject *a = aa;
	const GttProject *b = bb;
	return (b->estimated_end - a->estimated_end);
}


static int
cmp_due(const void *aa, const void *bb)
{
	const GttProject *a = aa;
	const GttProject *b = bb;
	return (b->due_date - a->due_date);
}


static int
cmp_sizing(const void *aa, const void *bb)
{
	const GttProject *a = aa;
	const GttProject *b = bb;
	return (b->sizing - a->sizing);
}

static int
cmp_percent(const void *aa, const void *bb)
{
	const GttProject *a = aa;
	const GttProject *b = bb;
	return (b->percent_complete - a->percent_complete);
}

static int
cmp_urgency(const void *aa, const void *bb)
{
	const GttProject *a = aa;
	const GttProject *b = bb;
	return (b->urgency - a->urgency);
}

static int
cmp_importance(const void *aa, const void *bb)
{
	const GttProject *a = aa;
	const GttProject *b = bb;
	return (b->importance - a->importance);
}

static int
cmp_status(const void *aa, const void *bb)
{
	const GttProject *a = aa;
	const GttProject *b = bb;
	return (b->status - a->status);
}

#define DO_SORT(CMP_FUNC)                                   \
	gboolean is_top = FALSE;                                 \
	GttProject *parent;                                      \
	GList *prjs = gps->prj_list;                             \
	if (!prjs) return;                                       \
	if (prjs == gps->prj_list) is_top = TRUE;                \
	parent = ((GttProject *) (prjs->data))->parent;          \
	prjs = project_list_sort(prjs, CMP_FUNC);                \
	if (is_top) gps->prj_list = prjs;                        \
	else if (parent) parent->sub_projects = prjs;            \


void
project_list_sort_day(GttProjectList *gps)
{
	DO_SORT(cmp_day);
}

void
project_list_sort_yesterday(GttProjectList *gps)
{
	DO_SORT(cmp_yesterday);
}

void
project_list_sort_week(GttProjectList *gps)
{
	DO_SORT(cmp_week);
}

void
project_list_sort_lastweek(GttProjectList *gps)
{
	DO_SORT(cmp_lastweek);
}

void
project_list_sort_month(GttProjectList *gps)
{
	DO_SORT(cmp_month);
}

void
project_list_sort_year(GttProjectList *gps)
{
	DO_SORT(cmp_year);
}

void
project_list_sort_ever(GttProjectList *gps)
{
	DO_SORT(cmp_ever);
}

void
project_list_sort_current(GttProjectList *gps)
{
	DO_SORT(cmp_current);
}

void
project_list_sort_title(GttProjectList *gps)
{
	DO_SORT(cmp_title);
}

void
project_list_sort_desc(GttProjectList *gps)
{
	DO_SORT(cmp_desc);
}

void
project_list_sort_start(GttProjectList *gps)
{
	DO_SORT(cmp_start);
}

void
project_list_sort_end(GttProjectList *gps)
{
	DO_SORT(cmp_end);
}

void
project_list_sort_due(GttProjectList *gps)
{
	DO_SORT(cmp_due);
}

void
project_list_sort_sizing(GttProjectList *gps)
{
	DO_SORT(cmp_sizing);
}

void
project_list_sort_percent(GttProjectList *gps)
{
	DO_SORT(cmp_percent);
}

void
project_list_sort_urgency(GttProjectList *gps)
{
	DO_SORT(cmp_urgency);
}

void
project_list_sort_importance(GttProjectList *gps)
{
	DO_SORT(cmp_importance);
}

void
project_list_sort_status(GttProjectList *gps)
{
	DO_SORT(cmp_status);
}

/* =========================== END OF FILE ========================= */
