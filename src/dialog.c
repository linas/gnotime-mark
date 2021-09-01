/*
 * GnoTime help browser display helper function
 *
 * Copyright (C) 2004 Linas Vepstas <linas@linas.org>
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

#include "dialog.h"

/**
 * @brief Display the GnoTime help using the system's default application
 * @param[in] widget An optional window which would become an error message's
 *   parent
 * @param[in] data An optional character array naming a particular section of
 *   the documentation which shall be shown
 */
void
gtt_help_popup(GtkWidget *widget, gpointer data)
{
	char *section = data;

	// Determine the URI to be shown
	gchar *uri = NULL;
	if ((NULL == section) || (0 == g_strcmp0("", section)))
	{
		section = NULL;
		uri = g_strdup("ghelp:gnotime");
	} else
	{
		uri = g_strdup_printf("ghelp:gnotime#%s", section);
	}

	// Show the determined URI
	GError *error = NULL;
	const gboolean res = gtk_show_uri(NULL, uri, GDK_CURRENT_TIME, &error);
	g_free(uri);
	uri = NULL;

	// Handle errors
	if ((FALSE == res) || (NULL != error))
	{
		g_warning("Failed to display help: %s", error->message);

		GtkWidget *mb = gtk_message_dialog_new(
				GTK_IS_WINDOW(widget) ? GTK_WINDOW(widget) : NULL, GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", error->message);
		g_signal_connect(G_OBJECT(mb), "response", G_CALLBACK(gtk_widget_destroy),
		                 mb);
		gtk_widget_show(mb);
		gtk_widget_destroy(mb);
		mb = NULL;
		g_clear_error(&error);
	}
}
