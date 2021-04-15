/*   Edit Interval Properties for GTimeTracker - a time tracker
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

#include <glade/glade.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "proj.h"
#include "props-invl.h"
#include "util.h"

struct EditIntervalDialog_s
{
	GttInterval *interval;
	GladeXML *gtxml;
	GtkWidget *interval_edit;
	GtkWidget *start_calendar;
	GtkWidget *start_hour;
	GtkWidget *start_minute;
	GtkWidget *start_second;
	GtkWidget *stop_calendar;
	GtkWidget *stop_hour;
	GtkWidget *stop_minute;
	GtkWidget *stop_second;
	GtkWidget *fuzz_widget;
};

/* ============================================================== */
/* interval dialog edits */

static void
interval_edit_apply_cb(GtkWidget *w, gpointer data)
{
	EditIntervalDialog *dlg = (EditIntervalDialog *)data;
	GtkWidget *menu, *menu_item;
	GttTask *task;
	GttProject *prj;
	time_t start, stop, tmp;
	int fuzz, min_invl;

	gint day = 0, month = 0, year = 0;
	const gint start_hour =
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dlg->start_hour));
	const gint start_minute =
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dlg->start_minute));
	const gint start_second =
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dlg->start_second));
	g_object_get(G_OBJECT(dlg->start_calendar), "day", &day, "month", &month,
	             "year", &year, NULL);
	GDateTime *start_date_time = g_date_time_new_local(
			year, month, day, start_hour, start_minute, start_second);
	start = g_date_time_to_unix(start_date_time);
	g_date_time_unref(start_date_time);
	start_date_time = NULL;
	const gint stop_hour =
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dlg->stop_hour));
	const gint stop_minute =
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dlg->stop_minute));
	const gint stop_second =
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dlg->stop_second));
	g_object_get(G_OBJECT(dlg->stop_calendar), "day", &day, "month", &month,
	             "year", &year, NULL);
	GDateTime *stop_date_time = g_date_time_new_local(year, month, day, stop_hour,
	                                                  stop_minute, stop_second);
	stop = g_date_time_to_unix(stop_date_time);
	g_date_time_unref(stop_date_time);
	stop_date_time = NULL;

	/* If user reversed start and stop, flip them back */
	if (start > stop)
	{
		tmp = start;
		start = stop;
		stop = tmp;
	}

	/* Caution: we must avoid setting very short time intervals
	 * through this interface; otherwise the interval will get
	 * scrubbed away on us, and we'll be holding an invalid pointer.
	 * In fact, we should probably assume the pointer is invalid
	 * if prj is null ...
	 */

	task = gtt_interval_get_parent(dlg->interval);
	prj = gtt_task_get_parent(task);
	min_invl = gtt_project_get_min_interval(prj);
	if (min_invl >= stop - start)
		stop = start + min_invl + 1;

	gtt_interval_freeze(dlg->interval);
	gtt_interval_set_start(dlg->interval, start);
	gtt_interval_set_stop(dlg->interval, stop);

	menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(dlg->fuzz_widget));
	menu_item = gtk_menu_get_active(GTK_MENU(menu));
	fuzz = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menu_item), "fuzz_factor"));

	gtt_interval_set_fuzz(dlg->interval, fuzz);

	/* The thaw may cause  the interval to change.  If so, redo the GUI. */
	dlg->interval = gtt_interval_thaw(dlg->interval);
	edit_interval_set_interval(dlg, dlg->interval);
}

static void
interval_edit_ok_cb(GtkWidget *w, gpointer data)
{
	EditIntervalDialog *dlg = (EditIntervalDialog *)data;
	interval_edit_apply_cb(w, data);
	gtk_widget_hide(dlg->interval_edit);
	dlg->interval = NULL;
}

static void
interval_edit_cancel_cb(GtkWidget *w, gpointer data)
{
	EditIntervalDialog *dlg = (EditIntervalDialog *)data;
	gtk_widget_hide(dlg->interval_edit);
	dlg->interval = NULL;
}

/* ============================================================== */
/* Set values into interval editor widgets */

void
edit_interval_set_interval(EditIntervalDialog *dlg, GttInterval *ivl)
{
	GtkOptionMenu *fw;
	time_t start, stop;
	int fuzz;

	if (!dlg)
		return;
	dlg->interval = ivl;

	if (!ivl)
	{
		GDateTime *now = g_date_time_new_now_local();
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlg->start_hour),
		                          g_date_time_get_hour(now));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlg->start_minute),
		                          g_date_time_get_minute(now));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlg->start_second),
		                          g_date_time_get_second(now));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlg->stop_hour),
		                          g_date_time_get_hour(now));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlg->stop_minute),
		                          g_date_time_get_minute(now));
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlg->stop_second),
		                          g_date_time_get_second(now));
		g_date_time_unref(now);
		now = NULL;

		fw = GTK_OPTION_MENU(dlg->fuzz_widget);
		gtk_option_menu_set_history(fw, 0);
		return;
	}

	start = gtt_interval_get_start(ivl);
	GDateTime *start_time = g_date_time_new_from_unix_utc(start);
	gint day = g_date_time_get_day_of_month(start_time);
	gint month = g_date_time_get_month(start_time);
	gint year = g_date_time_get_year(start_time);
	g_object_set(G_OBJECT(dlg->start_calendar), "day", &day, "month", &month,
	             "year", &year, NULL);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlg->start_hour),
	                          g_date_time_get_hour(start_time));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlg->start_minute),
	                          g_date_time_get_minute(start_time));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlg->start_second),
	                          g_date_time_get_second(start_time));
	g_date_time_unref(start_time);
	start_time = NULL;

	stop = gtt_interval_get_stop(ivl);
	GDateTime *stop_time = g_date_time_new_from_unix_utc(stop);
	day = g_date_time_get_day_of_month(stop_time);
	month = g_date_time_get_month(stop_time);
	year = g_date_time_get_year(stop_time);
	g_object_set(G_OBJECT(dlg->stop_calendar), "day", &day, "month", &month,
	             "year", &year, NULL);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlg->stop_hour),
	                          g_date_time_get_hour(stop_time));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlg->stop_minute),
	                          g_date_time_get_minute(stop_time));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dlg->stop_second),
	                          g_date_time_get_second(stop_time));
	g_date_time_unref(stop_time);
	stop_time = NULL;

	fuzz = gtt_interval_get_fuzz(dlg->interval);
	fw = GTK_OPTION_MENU(dlg->fuzz_widget);

	/* OK, now set the initial value */
	gtk_option_menu_set_history(fw, 0);
	if (90 < fuzz)
		gtk_option_menu_set_history(fw, 1);
	if (450 < fuzz)
		gtk_option_menu_set_history(fw, 2);
	if (750 < fuzz)
		gtk_option_menu_set_history(fw, 3);
	if (1050 < fuzz)
		gtk_option_menu_set_history(fw, 4);
	if (1500 < fuzz)
		gtk_option_menu_set_history(fw, 5);
	if (2700 < fuzz)
		gtk_option_menu_set_history(fw, 6);
	if (5400 < fuzz)
		gtk_option_menu_set_history(fw, 7);
	if (9000 < fuzz)
		gtk_option_menu_set_history(fw, 8);
	if (6 * 3600 < fuzz)
		gtk_option_menu_set_history(fw, 9);
}

/* ============================================================== */
/* interval popup actions */

EditIntervalDialog *
edit_interval_dialog_new(void)
{
	EditIntervalDialog *dlg;
	GladeXML *glxml;
	GtkWidget *w, *menu, *menu_item;

	dlg = g_malloc(sizeof(EditIntervalDialog));
	dlg->interval = NULL;

	glxml = gtt_glade_xml_new("glade/interval_edit.glade", "Interval Edit");
	dlg->gtxml = glxml;

	dlg->interval_edit = glade_xml_get_widget(glxml, "Interval Edit");

	glade_xml_signal_connect_data(glxml, "on_ok_button_clicked",
	                              GTK_SIGNAL_FUNC(interval_edit_ok_cb), dlg);

	glade_xml_signal_connect_data(glxml, "on_apply_button_clicked",
	                              GTK_SIGNAL_FUNC(interval_edit_apply_cb), dlg);

	glade_xml_signal_connect_data(glxml, "on_cancel_button_clicked",
	                              GTK_SIGNAL_FUNC(interval_edit_cancel_cb), dlg);

	dlg->start_calendar = glade_xml_get_widget(glxml, "calendar_start");
	dlg->start_hour = glade_xml_get_widget(glxml, "spinbutton_start_hour");
	dlg->start_minute = glade_xml_get_widget(glxml, "spinbutton_start_minute");
	dlg->start_second = glade_xml_get_widget(glxml, "spinbutton_start_second");
	dlg->stop_calendar = glade_xml_get_widget(glxml, "calendar_stop");
	dlg->stop_hour = glade_xml_get_widget(glxml, "spinbutton_stop_hour");
	dlg->stop_minute = glade_xml_get_widget(glxml, "spinbutton_stop_minute");
	dlg->stop_second = glade_xml_get_widget(glxml, "spinbutton_stop_second");
	dlg->fuzz_widget = glade_xml_get_widget(glxml, "fuzz_menu");

	/* ----------------------------------------------- */
	/* install option data by hand ... ugh
	 * wish glade did this for us .. */
	w = dlg->fuzz_widget;
	menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(w));

	gtk_option_menu_set_history(GTK_OPTION_MENU(w), 0);
	menu_item = gtk_menu_get_active(GTK_MENU(menu));
	g_object_set_data(G_OBJECT(menu_item), "fuzz_factor", GINT_TO_POINTER(0));

	gtk_option_menu_set_history(GTK_OPTION_MENU(w), 1);
	menu_item = gtk_menu_get_active(GTK_MENU(menu));
	g_object_set_data(G_OBJECT(menu_item), "fuzz_factor", GINT_TO_POINTER(300));

	gtk_option_menu_set_history(GTK_OPTION_MENU(w), 2);
	menu_item = gtk_menu_get_active(GTK_MENU(menu));
	g_object_set_data(G_OBJECT(menu_item), "fuzz_factor", GINT_TO_POINTER(600));

	gtk_option_menu_set_history(GTK_OPTION_MENU(w), 3);
	menu_item = gtk_menu_get_active(GTK_MENU(menu));
	g_object_set_data(G_OBJECT(menu_item), "fuzz_factor", GINT_TO_POINTER(900));

	gtk_option_menu_set_history(GTK_OPTION_MENU(w), 4);
	menu_item = gtk_menu_get_active(GTK_MENU(menu));
	g_object_set_data(G_OBJECT(menu_item), "fuzz_factor", GINT_TO_POINTER(1200));

	gtk_option_menu_set_history(GTK_OPTION_MENU(w), 5);
	menu_item = gtk_menu_get_active(GTK_MENU(menu));
	g_object_set_data(G_OBJECT(menu_item), "fuzz_factor", GINT_TO_POINTER(1800));

	gtk_option_menu_set_history(GTK_OPTION_MENU(w), 6);
	menu_item = gtk_menu_get_active(GTK_MENU(menu));
	g_object_set_data(G_OBJECT(menu_item), "fuzz_factor", GINT_TO_POINTER(3600));

	gtk_option_menu_set_history(GTK_OPTION_MENU(w), 7);
	menu_item = gtk_menu_get_active(GTK_MENU(menu));
	g_object_set_data(G_OBJECT(menu_item), "fuzz_factor", GINT_TO_POINTER(7200));

	gtk_option_menu_set_history(GTK_OPTION_MENU(w), 8);
	menu_item = gtk_menu_get_active(GTK_MENU(menu));
	g_object_set_data(G_OBJECT(menu_item), "fuzz_factor",
	                  GINT_TO_POINTER(3 * 3600));

	gtk_option_menu_set_history(GTK_OPTION_MENU(w), 9);
	menu_item = gtk_menu_get_active(GTK_MENU(menu));
	g_object_set_data(G_OBJECT(menu_item), "fuzz_factor",
	                  GINT_TO_POINTER(12 * 3600));

	/* gnome_dialog_close_hides(GNOME_DIALOG(dlg->interval_edit), TRUE); */
	gtk_widget_hide_on_delete(dlg->interval_edit);
	return dlg;
}

/* ============================================================== */

void
edit_interval_dialog_show(EditIntervalDialog *dlg)
{
	if (!dlg)
		return;
	gtk_widget_show(GTK_WIDGET(dlg->interval_edit));
}

void
edit_interval_dialog_destroy(EditIntervalDialog *dlg)
{
	if (!dlg)
		return;
	gtk_widget_destroy(GTK_WIDGET(dlg->interval_edit));
	g_free(dlg);
}

void
edit_interval_set_close_callback(EditIntervalDialog *dlg, GCallback f,
                                 gpointer data)
{
	g_signal_connect(dlg->interval_edit, "close", f, data);
}

/* ===================== END OF FILE ==============================  */
