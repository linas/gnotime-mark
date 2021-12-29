/* Implement a catch-throw-like error mechanism for gtt
 *
 * Copyright (C) 2001 Linas Vepstas
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

#ifndef GTT_ERR_THROW_H_
#define GTT_ERR_THROW_H_

#include <glib.h>

typedef enum
{
	GTT_NO_ERR = 0,
	GTT_CANT_OPEN_FILE,
	GTT_CANT_WRITE_FILE,
	GTT_NOT_A_GTT_FILE,
	GTT_FILE_CORRUPT,
	GTT_UNKNOWN_TOKEN,
	GTT_UNKNOWN_VALUE,
	GTT_CANT_WRITE_CONFIG
} GttErrCode;

GttErrCode gtt_err_get_code (void);
void gtt_err_set_code (GttErrCode code);
gchar *gtt_err_to_string (GttErrCode code, const char *filename);

#endif // GTT_ERR_THROW_H_
