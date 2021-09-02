/*
 * A catch-throw like error mechanism for GnoTime
 *
 * Copyright (C) 2001 Linas Vepstas
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

#include "err-throw.h"

#include <glib/gi18n.h>

static GttErrCode err = GTT_NO_ERR;

/**
 * @brief Retrieve the currently set error code
 * @return The currently set error code
 */
GttErrCode
gtt_err_get_code(void)
{
	return err;
}

/**
 * @brief Set an error code
 *
 * This in conjunction with gtt_err_get_code can be used to implement a
 * poor-man's try-catch mechanismas in the following example:
 *
 *     gtt_err_set_code(GTT_NO_ERR); // Start of try block
 *     {
 *       // Do stuff ..
 *     }
 *     switch (gtt_err_get_code()) { // Catch block
 *       case GTT_NO_ERR: break;
 *       case GTT_ERR_FOO: // Try to recover
 *     }
 *
 * @param[in] code The error code to be set
 */
void
gtt_err_set_code(const GttErrCode code)
{
	// Clear the error if requested
	if (GTT_NO_ERR == code)
	{
		err = GTT_NO_ERR;
		return;
	}

	// If an error code is set already, do not overwrite it
	if (GTT_NO_ERR != err)
	{
		return;
	}

	// Otherwise set it
	err = code;
}

/**
 * @brief Return a human-readable error message suitable for display
 * @param[in] code The error code for which the message shall be created
 * @param[in] filename The name or path of the file related to the error
 * @return A newly allocated error message
 */
gchar *
gtt_err_to_string(const GttErrCode code, const char *const filename)
{
	char *ret = NULL;

	switch (code)
	{
	case GTT_NO_ERR:
		ret = g_strdup(_("No Error"));
		break;
	case GTT_CANT_OPEN_FILE:
		ret = g_strdup_printf(_("Cannot open the project data file\n\t%s\n"),
		                      filename);
		break;
	case GTT_CANT_WRITE_FILE:
		ret = g_strdup_printf(_("Cannot write the project data file\n\t%s\n"),
		                      filename);
		break;
	case GTT_NOT_A_GTT_FILE:
		ret = g_strdup_printf(_("The file\n\t%s\n"
		                        "doesn't seem to be a GnoTime project data file\n"),
		                      filename);
		break;
	case GTT_FILE_CORRUPT:
		ret = g_strdup_printf(_("The file\n\t%s\n"
		                        "seems to be corrupt\n"),
		                      filename);
		break;
	case GTT_UNKNOWN_TOKEN:
		ret = g_strdup_printf(
				_("An unknown token was found during the parsing of\n\t%s\n"),
				filename);
		break;
	case GTT_UNKNOWN_VALUE:
		ret = g_strdup_printf(
				_("An unknown value was found during the parsing of\n\t%s\n"),
				filename);
		break;
	case GTT_CANT_WRITE_CONFIG:
		ret = g_strdup_printf(_("Cannot write the config file\n\t%s\n"), filename);
		break;
	}

	return ret;
}
