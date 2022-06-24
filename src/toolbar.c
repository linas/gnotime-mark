/*   GTimeTracker - a time tracker
 *   Copyright (C) 1997,98 Eckehard Berns
 *   Copyright (C) 2002,2003 Linas Vepstas <linas@linas.org>
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
#include <string.h>

#include "app.h"
#include "dialog.h"
#include "gtt.h"
#include "journal.h"
#include "menucmd.h"
#include "menus.h"
#include "myoaf.h"
#include "prefs.h"
#include "timer.h"
#include "toolbar.h"

typedef struct _MyToolbar MyToolbar;

struct _MyToolbar
{
	GtkToolbar *tbar;
	GtkWidget *new_w;
	GtkWidget *cut, *copy, *paste; /* to make them sensible as needed */
	GtkWidget *journal_button;
	GtkWidget *prop_w;
	GtkWidget *timer_button;
	GtkImage  *timer_button_image;
	GtkWidget *calendar_w;
	GtkWidget *pref;
	GtkWidget *help;
	GtkWidget *exit;

	int spa;
	int spb;
	int spc;
};

MyToolbar *mytbar = NULL;

/* ================================================================= */
/* This routine updates the appearence/behaviour of the toolbar.
 * In particular, the 'paste' button becomes active when there
 * is something to paste, and the timer button toggles it's
 * image when a project timer is started/stopped.
 */

void
toolbar_set_states(void)
{
	g_return_if_fail(mytbar != NULL);
	g_return_if_fail(mytbar->tbar != NULL);
	g_return_if_fail(GTK_IS_TOOLBAR(mytbar->tbar));

	if (config_show_toolbar)
	{
		update_toolbar_sections();
	}
	else
	{
		/* Rebuild the toolbar so that it is really hidden. There should be
		   a better way of doing this.
		*/
		update_toolbar_sections();
		return;
	}

	if (mytbar->tbar && mytbar->tbar->tooltips)
	{
		if (config_show_tb_tips)
			gtk_tooltips_enable(mytbar->tbar->tooltips);
		else
			gtk_tooltips_disable(mytbar->tbar->tooltips);
	}

	if (mytbar->paste)
	{
		gtk_widget_set_sensitive(mytbar->paste, have_cutted_project());
	}

	if (mytbar->timer_button_image)
	{
		gtk_image_set_from_stock (mytbar->timer_button_image,
				 ((timer_is_running()) ?
				     GNOME_STOCK_TIMER_STOP :
				     GNOME_STOCK_TIMER),
				 GTK_ICON_SIZE_LARGE_TOOLBAR);
	}
}

/* ================================================================= */
/* A small utility routine to use a stock image with custom text,
 * and put the whole thing into the toolbar
 */
static GtkWidget *
toolbar_append_stock_button (GtkToolbar *toolbar,
                             const gchar *text,
                             const gchar *tooltip_text,
                             const gchar *stock_icon_id,
                             GtkSignalFunc callback,
                             gpointer user_data)
{
	GtkWidget *w, *image;

	image = gtk_image_new_from_stock (stock_icon_id,
	                GTK_ICON_SIZE_LARGE_TOOLBAR);
	w = gtk_toolbar_append_item(toolbar, text, tooltip_text,
	                NULL, image,
	                callback, user_data);
	return w;
}

/* ================================================================= */
/* Assemble the buttons in the toolbar.  Which ones
 * are visible depends on the config settings.
 * Returns a pointer to the (still hidden) GtkToolbar
 */

/* ================================================================= */
/* TODO: I have to completely rebuild the toolbar, when I want to add or
   remove items. There should be a better way now */

#define ZAP(w)                                      \
   if (w) { gtk_container_remove(tbc, (w)); (w) = NULL; }

#define ZING(pos)                                   \
   if (pos) { gtk_toolbar_remove_space (mytbar->tbar, (pos)); (pos)=0; }

void
update_toolbar_sections(void)
{
	GtkContainer *tbc;
	GtkWidget *tb;

	if (!app_window) return;
	if (!mytbar) return;

	tbc = GTK_CONTAINER(mytbar->tbar);
	ZING (mytbar->spa);
	ZING (mytbar->spb);
	ZING (mytbar->spc);

	ZAP (mytbar->new_w);
	ZAP (mytbar->cut);
	ZAP (mytbar->copy);
	ZAP (mytbar->paste);
	ZAP (mytbar->journal_button);
	ZAP (mytbar->prop_w);
	ZAP (mytbar->timer_button);
	ZAP (mytbar->calendar_w);
	ZAP (mytbar->pref);
	ZAP (mytbar->help);
	ZAP (mytbar->exit);

	tb = build_toolbar();
	gtk_widget_show(tb);
}

/* ======================= END OF FILE ======================= */
