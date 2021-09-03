/*
 * ASCII/tab/delim/etc. data export for GnoTime
 *
 * Copyright (C) 1997,98 Eckehard Berns
 * Copyright (C) 2001,2002 Linas Vepstas <linas@linas.org>
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

#include "export.h"
#include "app.h"
#include "ghtml.h"

#include <gio/gio.h>
#include <glib/gi18n.h>

typedef struct export_format_s export_format_t;

struct export_format_s
{
	GtkFileChooser *picker;     // Widget for export file selection
	gchar *filename;            // Filename or path for the export file
	GFile *ofile;               // File to write the exported data to
	GFileOutputStream *ostream; // Output stream to write to the export file
	GttGhtml *ghtml;            // Output device used for generating the export
	const char *template;       // Output template
};

static void export_err(GttGhtml *gxp, int errcode, const char *msg,
                       export_format_t *xp);
static export_format_t *export_format_new(void);
static gint export_projects(export_format_t *xp);
static void export_really(GtkWidget *widget, export_format_t *const xp);
static void export_show_error_message(GtkWindow *parent, char *msg);
static void export_write(GttGhtml *gxp, const char *str, size_t len,
                         export_format_t *xp);

/**
 * @brief Helper callback for displaying an export error message to the user
 * @param[in] gxp unused
 * @param[in] errcode unused
 * @param[in] msg The error message which shall be shown to the user
 * @param[in] xp The export_format_t instance related to the error
 */
static void
export_err(G_GNUC_UNUSED GttGhtml *gxp, G_GNUC_UNUSED int errcode,
           const char *const msg, export_format_t *const xp)
{
	gchar *message = g_strdup_printf(_("Error exporting data: %s"), msg);
	export_show_error_message(GTK_WINDOW(xp->picker), message);
	g_free(message);
	message = NULL;
}

/**
 * @brief Choose a file path and export the project data in the given format
 * @param[in] widget unused
 * @param[in] data Path of the template file of the desired export format
 */
void
export_file_picker(G_GNUC_UNUSED GtkWidget *widget, gpointer data)
{
	const char *const template_filename = data;

	GtkWidget *dialog = gtk_file_chooser_dialog_new(
			_("Tab-Delimited Export"), GTK_WINDOW(app_window),
			GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),
	                                               TRUE);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		export_format_t *xp = export_format_new();
		xp->picker = GTK_FILE_CHOOSER(dialog);
		xp->template = gtt_ghtml_resolve_path(template_filename, NULL);
		export_really(dialog, xp);
		g_free(xp);
		xp = NULL;
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));
	dialog = NULL;
}

/**
 * @brief Create a new zero-initialised instance of export_format_t
 * @return The new zero-initialised instance of export_format_t
 */
static export_format_t *
export_format_new(void)
{
	export_format_t *const rc = g_new0(export_format_t, 1);

	rc->filename = NULL;
	rc->ghtml = NULL;
	rc->ofile = NULL;
	rc->ostream = NULL;
	rc->picker = NULL;
	rc->template = NULL;

	return rc;
}

/**
 * @brief Export the currently selected project
 * @param[in] xp The export_format_t instance related to the export
 * @return 0 in any case
 */
static gint
export_projects(export_format_t *xp)
{
	// Get the currently selected project
	GttProject *prj = gtt_projects_tree_get_selected_project(projects_tree);
	if (NULL == prj)
	{
		return 0;
	}

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

/**
 * @brief Conduct the actual export
 * @param[in] widget unused
 * @param[in] xp The export_format_t instance for the export
 */
static void
export_really(G_GNUC_UNUSED GtkWidget *widget, export_format_t *const xp)
{
	xp->filename = gtk_file_chooser_get_filename(xp->picker);

	GError *error = NULL;
	xp->ofile = g_file_new_for_path(xp->filename);
	xp->ostream =
			g_file_replace(xp->ofile, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &error);
	if ((NULL == xp->ostream) || (NULL != error))
	{
		char *msg = g_strdup_printf(_("File %s could not be opened"), xp->filename);
		export_show_error_message(GTK_WINDOW(xp->picker), msg);
		g_free(msg);
		msg = NULL;
		g_free(xp->filename);
		xp->filename = NULL;
		g_clear_error(&error);
		g_clear_object(&xp->ofile);
		return;
	}

	if (export_projects(xp))
	{
		export_show_error_message(GTK_WINDOW(xp->picker),
		                          _("Error occured during export"));
	}
	if (!g_output_stream_close(G_OUTPUT_STREAM(xp->ostream), NULL, &error))
	{
		g_warning("Failed to close export file \"%s\": %s", xp->filename,
		          error->message);
		g_clear_error(&error);
	}
	g_clear_object(&xp->ostream);
	g_clear_object(&xp->ofile);
	g_free(xp->filename);
	xp->filename = NULL;
}

/**
 * @brief Display an error message concerning a failed export
 * @param[in] parent An optional parent GtkWindow for the error dialog
 * @param[in] msg The error message which shall be displayed
 */
static void
export_show_error_message(GtkWindow *const parent, char *const msg)
{
	GtkWidget *dialog = gtk_message_dialog_new_with_markup(
			parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
			_("<b>Gnotime export error</b>"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s",
	                                         msg);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	dialog = NULL;
}

/**
 * @brief Print out the data using the Guile based infrastructure
 * @param[in] gxp TODO
 * @param[in] str Character array of the to be saved data
 * @param[in] len The length in bytes of the to be saved data
 * @param[in] xp The export_format_t instance which initiated the write
 */
static void
export_write(GttGhtml *gxp, const char *str, size_t len, export_format_t *xp)
{
	gsize buflen = len;
	gsize off = 0;

	while (0 < buflen)
	{
		GError *error = NULL;
		gssize bytes_written = g_output_stream_write(
				G_OUTPUT_STREAM(xp->ostream), &str[off], buflen, NULL, &error);
		if ((0 > bytes_written) || (NULL != error))
		{
			g_warning("Failed to write to export file \"%s\": %s", xp->filename,
			          error->message);
			break;
		}
		off += bytes_written;
		buflen -= bytes_written;
	}
}
