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

#ifndef GTT_INTERVAL_H_
#define GTT_INTERVAL_H_

#include <glib.h>

#include <time.h>

typedef struct gtt_interval_s GttInterval;
typedef struct gtt_task_s GttTask;

GttInterval *gtt_interval_new (void);
void gtt_interval_destroy (GttInterval *ivl);

void gtt_interval_freeze (GttInterval *ivl);
GttInterval *gtt_interval_thaw (GttInterval *ivl);

GttTask *gtt_interval_get_parent (const GttInterval *ivl);

int gtt_interval_get_fuzz (const GttInterval *ivl);
void gtt_interval_set_fuzz (GttInterval *ivl, int fuzz);
gboolean gtt_interval_is_running (const GttInterval *ivl);
void gtt_interval_set_running (GttInterval *ivl, gboolean running);
time_t gtt_interval_get_start (const GttInterval *ivl);
void gtt_interval_set_start (GttInterval *ivl, time_t start);
time_t gtt_interval_get_stop (const GttInterval *ivl);
void gtt_interval_set_stop (GttInterval *ivl, time_t stop);

gboolean gtt_interval_is_first_interval (const GttInterval *ivl);
gboolean gtt_interval_is_last_interval (const GttInterval *ivl);

GttInterval *gtt_interval_merge_down (GttInterval *ivl);
GttInterval *gtt_interval_merge_up (GttInterval *ivl);
GttInterval *gtt_interval_new_insert_after (GttInterval *where);
void gtt_interval_split (GttInterval *ivl, GttTask *newtask);

#endif // GTT_INTERVAL_H_
