/* GnoTime help popup wrapper
 *
 * Copyright (C) 2004 Linas Vepstas <linas@linas.org>
 * Copyright (C) 2021      Markus Prasser
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

#include <glib/gi18n.h>

/**
 * @brief Open the documentation with the system's appropriate default
 * application
 * @param[in] widget The widget that determines on which screen the help shall
 * be shown
 * @param[in] data An optional documentation section to display to the user
 */
void
gtt_help_popup (GtkWidget *const widget, gconstpointer data)
{
	const gchar *section = data;

	gchar *uri           = NULL;
	if ((section != NULL) && (0 != strcmp ("", section)))
	{
		uri = g_strdup_printf ("ghelp:gnotime#%s", section);
	}
	else
	{
		uri = g_strdup_printf ("ghelp:gnotime");
	}

	GError *error = NULL;
	if (!gtk_show_uri (GTK_IS_WIDGET (widget) ? gtk_widget_get_screen (widget)
																						: NULL,
										 uri, GDK_CURRENT_TIME, &error))
	{
		GtkWidget *mb = gtk_message_dialog_new (
				GTK_IS_WINDOW (widget) ? GTK_WINDOW (widget) : NULL, GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
				_ ("Help could not be displayed:\n\n%s"), error->message);
		g_signal_connect (G_OBJECT (mb), "response",
											G_CALLBACK (gtk_widget_destroy), mb);
		gtk_widget_show (mb);
		mb = NULL;

		g_error_free (error);
		error = NULL;
	}

	g_free (uri);
	uri = NULL;
}
