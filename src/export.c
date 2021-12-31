/* Project data export
 *
 * Copyright (C) 1997,98 Eckehard Berns
 * Copyright (C) 2001,2002 Linas Vepstas <linas@linas.org>
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

#include "export.h"
#include "ghtml.h"
#include "projects-tree.h"

#include <gio/gio.h>
#include <glib/gi18n.h>

extern GtkWidget *app_window;
extern GttProjectsTree *projects_tree;

typedef struct export_format_s export_format_t;

struct export_format_s
{
	GtkFileChooser *picker;     // File selection dialog
	gchar *filename;            // The name of the file to write the export to
	GFile *ofile;               // File to write the data to for export
	GFileOutputStream *ostream; // Output stream to write to the export file
	GttGhtml *ghtml;            // Output device to generate the export data
	const gchar *template;      // The template to generate the export from
};

static void export_err (GttGhtml *gxp, int errcode, const char *msg,
												export_format_t *xp);
static export_format_t *export_format_new (void);
static gint export_project (export_format_t *xp);
static void export_really (GtkWidget *widget, export_format_t *xp);
static void export_show_error_message (GtkWindow *parent, gchar *msg);
static void export_write (GttGhtml *gxp, const char *str, size_t len,
													export_format_t *xp);

/**
 * @brief Assemble and display an error message
 * @param[in] gxp Unused
 * @param[in] errcode Unused
 * @param[in] msg An explanatory message of the cause of the occured error
 * @param[in] xp An export_format_t structure holding the respective
 * GtkFileChooser instance
 */
static void
export_err (G_GNUC_UNUSED GttGhtml *const gxp, G_GNUC_UNUSED const int errcode,
						const char *const msg, export_format_t *const xp)
{
	gchar *message = g_strdup_printf (_ ("Error exporting data: %s"), msg);
	export_show_error_message (GTK_WINDOW (xp->picker), message);
	g_free (message);
	message = NULL;
}

/**
 * @brief Allocate and return a new instance of export_format_t
 * @return Pointer to the allocated memory for an export_format_t instance
 */
static export_format_t *
export_format_new (void)
{
	return g_new0 (export_format_t, 1);
}

/**
 * @brief Export the project currently activated in the projects tree through
 * the GNU Guile based infrastructure
 * @param[in] xp An export_format_t structure whose data is being used for the
 * export
 * @return `0` always
 */
static gint
export_project (export_format_t *const xp)
{
	// Get the currently selected project
	GttProject *const prj
			= gtt_projects_tree_get_selected_project (projects_tree);
	if (NULL == prj)
	{
		return 0;
	}

	xp->ghtml = gtt_ghtml_new ();
	gtt_ghtml_set_stream (xp->ghtml, xp, NULL,
												(GttGhtmlWriteStream) export_write, NULL,
												(GttGhtmlError) export_err);

	gtt_ghtml_display (xp->ghtml, xp->template, prj);

	gtt_ghtml_destroy (xp->ghtml);
	xp->ghtml = NULL;

	g_free ((char *) xp->template);
	xp->template = NULL;

	return 0;
}

/**
 * @brief Conduct the actual export of the project data
 * @param widget
 * @param xp
 */
static void
export_really (GtkWidget *widget, export_format_t *xp)
{
	xp->filename  = gtk_file_chooser_get_filename (xp->picker);

	GError *error = NULL;
	xp->ofile     = g_file_new_for_path (xp->filename);
	xp->ostream = g_file_replace (xp->ofile, NULL, FALSE, G_FILE_CREATE_PRIVATE,
																NULL, &error);
	if (NULL == xp->ostream)
	{
		gchar *msg = g_strdup_printf (_ ("File \"%s\" could not be opened: %s"),
																	xp->filename, error->message);
		export_show_error_message (GTK_WINDOW (xp->picker), msg);
		g_free (msg);
		msg = NULL;
		g_error_free (error);
		error = NULL;
		g_clear_object (&xp->ofile);
		g_free (xp->filename);
		xp->filename = NULL;
		return;
	}

	const gint rc = export_project (xp);
	if (rc)
	{
		export_show_error_message (GTK_WINDOW (xp->picker),
															 _ ("Error occured during export"));
		return;
	}
	if (!g_output_stream_close (G_OUTPUT_STREAM (xp->ostream), NULL, &error))
	{
		g_warning ("Failed to close export file \"%s\": %s", xp->filename,
							 error->message);
		g_error_free (error);
		error = NULL;
	}
	g_clear_object (&xp->ostream);
	g_clear_object (&xp->ofile);
}

/**
 * @brief Display an error message dialog with a given error message
 * @param[in] parent An optional parent window for the error message dialog
 * @param[in] msg An explanatory error message which shall be displayed
 */
static void
export_show_error_message (GtkWindow *const parent, gchar *const msg)
{
	GtkWidget *dialog = gtk_message_dialog_new_with_markup (
			parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
			_ ("<b>Gnotime export error</b>"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s",
																						msg);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	dialog = NULL;
}

/*
 * Print out the projects using the standard guile-based
 * printing infrastructure.
 */
/**
 * @brief Write data to the export file
 * @param[in] gxp Unused
 * @param[in] str The data which shall be written to the file
 * @param[in] len The size of the to-be-written data in bytes
 * @param[in] xp An export_format_t structure carrying the file stream to write
 * to
 */
static void
export_write (G_GNUC_UNUSED GttGhtml *const gxp, const char *const str,
							const size_t len, export_format_t *const xp)
{
	gsize buflen = len;
	gsize off    = 0;

	while (buflen > 0)
	{
		GError *error              = NULL;
		const gssize bytes_written = g_output_stream_write (
				G_OUTPUT_STREAM (xp->ostream), &str[off], buflen, NULL, &error);
		if (0 > bytes_written)
		{
			g_warning ("Failed to write to export file \"%s\": %s", xp->filename,
								 error->message);
			g_error_free (error);
			error = NULL;
			break;
		}
		off += bytes_written;
		buflen -= bytes_written;
	}
}

/**
 * @brief Choose a file for export and export data according to the passed
 * template
 * @param[in] widget Unused
 * @param[in] data The template which shall be used by the GNU Guile based
 * infrastructure to generate the export data and format
 */
void
export_file_picker (GtkWidget *widget, gpointer data)
{
	const gchar *const template_filename = data;

	GtkWidget *dialog                    = gtk_file_chooser_dialog_new (
												 _ ("Tab-Delimited Export"), GTK_WINDOW (app_window),
												 GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
												 GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
																									TRUE);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		export_format_t *xp = export_format_new ();
		xp->picker          = GTK_FILE_CHOOSER (dialog);
		xp->template        = gtt_ghtml_resolve_path (template_filename, NULL);
		export_really (dialog, xp);
		g_free (xp);
		xp = NULL;
	}
	gtk_widget_destroy (GTK_WIDGET (dialog));
	dialog = NULL;
}
