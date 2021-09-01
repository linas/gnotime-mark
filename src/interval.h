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

#ifndef GTT_INTERVAL_H_
#define GTT_INTERVAL_H_

#include <glib.h>

#include <sys/time.h>

typedef struct gtt_interval_s GttInterval;

int gtt_interval_get_fuzz(const GttInterval *ivl);
time_t gtt_interval_get_start(const GttInterval *ivl);
time_t gtt_interval_get_stop(const GttInterval *ivl);
gboolean gtt_interval_is_first_interval(const GttInterval *ivl);
gboolean gtt_interval_is_last_interval(const GttInterval *ivl);
gboolean gtt_interval_is_running(const GttInterval *ivl);
GttInterval *gtt_interval_new(void);

#endif // GTT_INTERVAL_H_
