/*   GTimeTracker - a time tracker
 *   Copyright (C) 1997,98 Eckehard Berns
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

#include <config.h>
#include <gnome.h>
 
#include "dialog.h"
#include "gtt.h"

static void 
dialog_setup(GnomeDialog *dlg, GtkBox **vbox_return)
{
	g_return_if_fail(dlg != NULL);
	gnome_dialog_set_close(dlg, TRUE);
	if (vbox_return) *vbox_return = GTK_BOX(dlg->vbox);
}


void 
new_dialog_ok(const char *title, GtkWidget **dlg, GtkBox **vbox,
		       const char *s, GCallback sigfunc, gpointer data)
{
	GtkWidget *dia;
	char *tmp;

	tmp = g_strdup_printf(GTT_APP_PROPER_NAME " - %s", title);
	dia = gnome_dialog_new(tmp, s, NULL);
	if (dlg) *dlg = dia;
	dialog_setup(GNOME_DIALOG(dia), vbox);
	
	if (sigfunc)
	       gnome_dialog_button_connect(GNOME_DIALOG(dia), 0,
					   sigfunc, data);

	g_free (tmp);
}


void 
new_dialog_ok_cancel(const char *title, GtkWidget **dlg, GtkBox **vbox,
			  const char *s_ok, GCallback sigfunc, gpointer data,
			  const char *s_cancel, GCallback c_sigfunc, gpointer c_data)
{
	char *tmp;
	GtkWidget *dia;
	
	g_return_if_fail(s_ok != NULL);
	g_return_if_fail(s_cancel != NULL);
	g_return_if_fail(title != NULL);

	tmp = g_strdup_printf(GTT_APP_PROPER_NAME " - %s", title);
	dia = gnome_dialog_new(tmp, s_ok, s_cancel, NULL);
	if (dlg) *dlg = dia;
	dialog_setup(GNOME_DIALOG(dia), vbox);

	if (sigfunc)
		gnome_dialog_button_connect(GNOME_DIALOG(dia), 0,
					    sigfunc, data);

	if (c_sigfunc)
		gnome_dialog_button_connect(GNOME_DIALOG(dia), 1,
					    c_sigfunc, c_data);

	gnome_dialog_set_default(GNOME_DIALOG(dia), 0);

	g_free (tmp);
}


void 
msgbox_ok(const char *title, const char *text, const char *ok_text,
	       GCallback func)
{
	char *s;

	GtkWidget *mbox;

	s = g_strdup_printf(GTT_APP_PROPER_NAME " - %s", title);
	mbox = gnome_message_box_new(text, GNOME_MESSAGE_BOX_WARNING, ok_text, NULL, NULL);

	if (func) g_signal_connect(G_OBJECT(mbox), "clicked", func, NULL);
	gtk_window_set_title(GTK_WINDOW(mbox), s);
	gtk_widget_show(mbox);

	g_free (s);
}



void 
msgbox_ok_cancel(const char *title, const char *text,
		      const char *ok_text, const char *cancel_text,
		      GCallback func)
{
	char *s;
	GtkWidget *mbox;

	s = g_strdup_printf(GTT_APP_PROPER_NAME " - %s", title);

	mbox = gnome_message_box_new(text, GNOME_MESSAGE_BOX_QUESTION, ok_text, cancel_text, NULL);
	gnome_dialog_set_default(GNOME_DIALOG(mbox), 1);
	if (func) g_signal_connect(G_OBJECT(mbox), "clicked",
			   func, NULL);
	gtk_window_set_title(GTK_WINDOW(mbox), s);
	gtk_widget_show(mbox);

	g_free (s);
}

void 
qbox_ok_cancel(const char *title, const char *text,
			  const char *ok_text, GCallback sigfunc, gpointer data,
			  const char *cancel_text, GCallback c_sigfunc, gpointer c_data)
{
	char *s;
	GtkWidget *mbox;

	s = g_strdup_printf(GTT_APP_PROPER_NAME " - %s", title);

	mbox = gnome_message_box_new(text, GNOME_MESSAGE_BOX_QUESTION, ok_text, cancel_text, NULL);
	gnome_dialog_set_default(GNOME_DIALOG(mbox), 1);
	gtk_window_set_title(GTK_WINDOW(mbox), s);

	if (sigfunc)
		gnome_dialog_button_connect(GNOME_DIALOG(mbox), 0,
					    sigfunc, data);

	if (c_sigfunc)
		gnome_dialog_button_connect(GNOME_DIALOG(mbox), 1,
					    c_sigfunc, c_data);

	gtk_widget_show(mbox);

	g_free (s);
}

