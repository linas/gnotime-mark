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

#ifndef GTT_INTERNAL_DATA_H_
#define GTT_INTERNAL_DATA_H_

#include <sys/time.h>

typedef struct gtt_interval_s GttInterval;
typedef struct gtt_project_s GttProject;
typedef struct gtt_task_s GttTask;

time_t get_midnight(time_t last);
void proj_modified(GttProject *proj);
void proj_refresh_time(GttProject *proj);
void project_compute_secs(GttProject *proj);
GttInterval *scrub_intervals(GttTask *tsk, GttInterval *handle);

#endif // GTT_INTERNAL_DATA_H_
