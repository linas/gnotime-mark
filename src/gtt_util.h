/*   utilities for GTimeTracker - a time tracker
 *   Copyright (C) 2001 Linas Vepstas
 * Copyright (C) 2023      Markus Prasser
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

#ifndef GTT_UTIL_H
#define GTT_UTIL_H

#include <gtk/gtk.h>

/* ------------------------------------------------------------------ */
/* some gtk-like utilities */
void xxxgtk_textview_set_text (GtkTextView *text, const char *str);
char *xxxgtk_textview_get_text (GtkTextView *text);

/* ------------------------------------------------------------------ */
/* Functions that used to be in qof,m but are not there any longer. */
size_t xxxqof_print_hours_elapsed_buff (char *buff, size_t len, int secs,
                                        gboolean show_secs);

size_t xxxqof_print_date_time_buff (char *buff, size_t len, time_t secs);
size_t xxxqof_print_date_buff (char *buff, size_t len, time_t t);
size_t xxxqof_print_time_buff (gchar *buff, size_t len, time_t secs);
gboolean xxxqof_is_same_day (time_t ta, time_t tb);

size_t xxxqof_print_minutes_elapsed_buff (char *buff, size_t len, int secs,
                                          gboolean show_secs);

size_t xxxqof_print_date_dmy_buff (char *buff, size_t len, int day, int month,
                                   int year);

#endif // GTT_UTIL_H
