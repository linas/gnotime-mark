/*   ASCII/tab/delim/etc. data export for GnoTime
 *   Copyright (C) 1997,98 Eckehard Berns
 *   Copyright (C) 2001,2002 Linas Vepstas <linas@linas.org>
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

#include "export.h"
#include "app.h"
#include "ghtml.h"

#include <glib/gi18n.h>

/* Project data export */

#define gtt_sure_string(x) ((x) ? (x) : "")

typedef struct export_format_s export_format_t;

struct export_format_s
{
	GtkFileChooser *picker; /* URI picker (file selection) */
	const char *uri;        /* aka filename */
	GFile *file;
	GFileOutputStream *ostream;
	GttGhtml *ghtml;      /* output device */
	const char *template; /* output template */
};

static export_format_t *export_format_new(void);
static void export_show_error_message(GtkWindow *parent, char *msg);

static export_format_t *
export_format_new(void)
{
	export_format_t *rc;
	rc = g_new0(export_format_t, 1);
	rc->picker = NULL;
	rc->uri = NULL;
	rc->ghtml = NULL;
	rc->template = NULL;
	return rc;
}

/*
 * Displays an error message dialog
 *
 */
static void
export_show_error_message(GtkWindow *parent, char *msg)
{
	GtkWidget *dialog = gtk_message_dialog_new_with_markup(
			parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
			_("<b>Gnotime export error</b>"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s",
	                                         msg);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

/*
 * Print out the projects using the standard guile-based
 * printing infrastructure.
 */

static void
export_write(GttGhtml *gxp, const char *str, size_t len, export_format_t *xp)
{
	gsize bytes_written = 0;
	GError *err = NULL;
	if (!g_output_stream_write_all(G_OUTPUT_STREAM(xp->ostream), str, len,
	                               &bytes_written, NULL, NULL) ||
	    err)
	{
		char *msg = g_strdup_printf(_("Failed to write to file %s: %s"), xp->uri,
		                            err->message);
		export_show_error_message(GTK_WINDOW(xp->picker), msg);
		g_free(msg);
		g_error_free(err);
	}
}

static void
export_err(GttGhtml *gxp, int errcode, const char *msg, export_format_t *xp)
{
	char *message = g_strdup_printf(_("Error exporting data: %s"), msg);
	export_show_error_message(GTK_WINDOW(xp->picker), message);
	g_free(message);
}

static gint
export_projects(export_format_t *xp)
{
	GttProject *prj;

	/* Get the currently selected project */
	prj = gtt_projects_tree_get_selected_project(projects_tree);
	if (!prj)
		return 0;

	xp->ghtml = gtt_ghtml_new();
	gtt_ghtml_set_stream(xp->ghtml, xp, NULL, (GttGhtmlWriteStream)export_write,
	                     NULL, (GttGhtmlError)export_err);

	gtt_ghtml_display(xp->ghtml, xp->template, prj);

	gtt_ghtml_destroy(xp->ghtml);
	xp->ghtml = NULL;

	g_free((char *)xp->template);
	xp->template = NULL;

	return 0;
}

static void
export_really(GtkWidget *widget, export_format_t *xp)
{
	gboolean rc;

	xp->uri = gtk_file_chooser_get_filename(xp->picker);

	GError *err = NULL;
	xp->file = g_file_new_for_path(xp->uri);
	xp->ostream =
			g_file_replace(xp->file, NULL, FALSE, G_FILE_CREATE_PRIVATE, NULL, &err);
	if (!xp->ostream || err)
	{
		char *msg = g_strdup_printf(_("File %s could not be opened: %s"), xp->uri,
		                            err->message);
		export_show_error_message(GTK_WINDOW(xp->picker), msg);
		g_free(msg);
		g_error_free(err);

		return;
	}

	rc = export_projects(xp);
	if (rc)
	{
		export_show_error_message(GTK_WINDOW(xp->picker),
		                          _("Error occured during export"));
		return;
	}
	if (!g_output_stream_close(G_OUTPUT_STREAM(xp->ostream), NULL, &err) || err)
	{
		char *msg = g_strdup_printf(_("File %s could not be closed: %s"), xp->uri,
		                            err->message);
		export_show_error_message(GTK_WINDOW(xp->picker), msg);
		g_free(msg);
		g_error_free(err);
	}
	g_object_unref(xp->file);
}

void
export_file_picker(GtkWidget *widget, gpointer data)
{
	export_format_t *xp;
	GtkWidget *dialog;
	const char *template_filename = data;

	dialog = gtk_file_chooser_dialog_new(
			_("Tab-Delimited Export"), GTK_WINDOW(app_window),
			GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),
	                                               TRUE);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		xp = export_format_new();
		xp->picker = GTK_FILE_CHOOSER(dialog);
		xp->template = gtt_ghtml_resolve_path(template_filename, NULL);
		export_really(dialog, xp);
		g_free(xp);
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));
}
