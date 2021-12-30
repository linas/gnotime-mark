/* Catch/block signals, X11 errors, graceful clean-up
 *
 * Copyright (C) 1997,1998 Eckehard Berns
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

#include "err.h"
#include "gtt.h"

#include <X11/Xlib.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#undef DIE_ON_NORMAL_ERROR

static void die (void);
static void sig_handler (int signum);
#ifdef DIE_ON_NORMAL_ERROR
static int x11_error_handler (Display *d, XErrorEvent *e);
#endif // DIE_ON_NORMAL_ERROR
static int x11_io_error_handler (Display *d);

/**
 * @brief Print informative string, attempt to save, clean-up and terminate
 */
static void
die (void)
{
	fprintf (stderr, " - saving and dying\n");
	save_all ();
	unlock_gtt ();
	exit (1);
}

/**
 * @brief sig_handler Print received signal and call die for clean-up
 * @param[in] signum The number of the received signal
 */
static void
sig_handler (const int signum)
{
	fprintf (stderr, "%s: Signal %d caught", GTT_APP_NAME, signum);
	die ();
}

#ifdef DIE_ON_NORMAL_ERROR
/**
 * @brief Print warning on received X11 error and call die for clean-up
 * @param[in] d Unused
 * @param[in] e Unused
 * @return `0` always
 */
static int
x11_error_handler (G_GNUC_UNUSED Display *const d,
									 G_GNUC_UNUSED XErrorEvent *const e)
{
	fprintf (stderr, "%s: X11 error caught", GTT_APP_NAME);
	die ();

	return 0; // Avoid warning on missing return value
}
#endif // DIE_ON_NORMAL_ERROR

/**
 * @brief Print warning on received X11 IO error and call die for clean-up
 * @param[in] d Unused
 * @return `0` always
 */
static int
x11_io_error_handler (G_GNUC_UNUSED Display *const d)
{
	fprintf (stderr, "%s: fatal X11 io error caught", GTT_APP_NAME);
	die ();
	return 0; // Avoid warning on missing return value
}

/**
 * @brief Initialize the signal handling of GnoTime
 *
 * This function should be called once on each application startup.
 */
void
err_init (void)
{
	static gboolean inited = FALSE;

	if (FALSE != inited)
	{
		g_warning ("Signal handling of %s got initialized already", GTT_APP_NAME);
		return;
	}

#ifdef DIE_ON_NORMAL_ERROR
	XSetErrorHandler (x11_error_handler);
#endif // DIE_ON_NORMAL_ERROR

	signal (SIGINT, sig_handler);
	signal (SIGKILL, sig_handler);
	signal (SIGTERM, sig_handler);
	signal (SIGHUP, sig_handler);
	signal (SIGPIPE, sig_handler);

	XSetIOErrorHandler (x11_io_error_handler);

	inited = TRUE;
}
