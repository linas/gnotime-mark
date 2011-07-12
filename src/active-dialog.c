/*   Keyboard inactivity timout dialog for GTimeTracker - a time tracker
 *   Copyright (C) 2001,2002,2003 Linas Vepstas <linas@linas.org>
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

#include "config.h"
#include <glib.h>
#include <glib/gi18n.h>

#include <glade/glade.h>
#include <string.h>

#include <qof.h>

#include "cur-proj.h"
#include "active-dialog.h"
#include "proj.h"
#include "proj-query.h"
#include "util.h"
#include "dialog.h"


int config_no_project_timeout;

struct GttActiveDialog_s 
{
	GladeXML    *gtxml;
	GtkDialog   *dlg;
	GtkButton   *yes_btn;
	GtkButton   *no_btn;
	GtkButton   *help_btn;
	GtkLabel    *active_label;
	GtkLabel    *credit_label;
	GtkComboBox *project_menu;
	guint        timeout_event_source;
};


void show_active_dialog (GttActiveDialog *ad);

static gboolean
active_timeout_func (gpointer data)
{
	GttActiveDialog *active_dialog = (GttActiveDialog *) data;
	show_active_dialog (active_dialog);

	/* Mark the timer as inactive */
	active_dialog->timeout_event_source = 0;

	/* deactivate the timer */
	return FALSE;
}

/* =========================================================== */

static void
schedule_active_timeout (gint timeout, GttActiveDialog *active_dialog)
{
	if (timeout > 0)
	{
		if (active_dialog->timeout_event_source)
		{
			g_source_remove (active_dialog->timeout_event_source);
		}
		active_dialog->timeout_event_source = g_timeout_add_seconds (timeout, active_timeout_func, active_dialog);
	}
}

/* =========================================================== */

static void
dialog_help (GObject *obj, GttActiveDialog *dlg)
{
	gtt_help_popup (GTK_WIDGET(dlg->dlg), "idletimer");
}

/* =========================================================== */

static void
dialog_close (GObject *obj, GttActiveDialog *dlg)
{
	dlg->dlg = NULL;
	dlg->gtxml = NULL;

	if (!cur_proj)
	{
		schedule_active_timeout (config_no_project_timeout, dlg);
	}

}

/* =========================================================== */

static void
dialog_kill (GObject *obj, GttActiveDialog *dlg)
{
	gtk_widget_destroy (GTK_WIDGET(dlg->dlg));
	dlg->dlg = NULL;
	dlg->gtxml = NULL;
	if (!cur_proj)
	{
		schedule_active_timeout (config_no_project_timeout, dlg);
	}
}

/* =========================================================== */

static void
start_proj (GObject *obj, GttActiveDialog *dlg)
{
	GtkTreeModel *tree_model;
	GtkTreeIter iter;
	GttProject *prj;

	/* start the project selected by the user, if any */
	if (gtk_combo_box_get_active_iter (dlg->project_menu, &iter))
	{
		tree_model = gtk_combo_box_get_model (dlg->project_menu);
		gtk_tree_model_get (tree_model, &iter, 1, &prj, -1);
		cur_proj_set (prj);
		dialog_kill (obj, dlg);
	}
}

/* =========================================================== */

static void
setup_menus (GttActiveDialog *dlg)
{
	GtkListStore *list_store;
	GtkTreeIter iter;
	GList *prjlist, *node;
	char * msg;

	msg = _("No project timer is currently running in GnoTime.  "
	        "Do you want to start a project timer running?  "
	        "If so, you can select a project from the menu below, "
	        "and click 'Start' to start the project running.  "
	        "Otherwise, just click 'Cancel' to do nothing.");

	gtk_label_set_text (dlg->active_label, msg);

	msg = _("You can credit this project with the time that you worked "
	        "on it but were away from the keyboard.  Enter a time below, "
	        "the project will be credited when you click 'Start'");
						 
	gtk_label_set_text (dlg->credit_label, msg);

	list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);

	/* Give user a list only of the unfinished projects, 
	 * so that there isn't too much clutter ... */
	prjlist = gtt_project_get_unfinished ();
	for (node = prjlist; node; node=node->next)
	{
		GttProject *prj = node->data;
		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, &iter,
							0, gtt_project_get_title (prj),
							1, prj,-1);
	}
	gtk_combo_box_set_model (dlg->project_menu, GTK_TREE_MODEL (list_store));
}

/* =========================================================== */
/* XXX the new GtkDialog is broken; it can't hide-on-close,
 * unlike to old, deprecated GnomeDialog.  Thus, we have to
 * do a heavyweight re-initialization each time.  Urgh.
 */

static void
active_dialog_realize (GttActiveDialog * id)
{
	GtkWidget *w;
	GladeXML *gtxml;
	GtkCellRenderer *renderer;

	gtxml = gtt_glade_xml_new ("glade/active.glade", "Active Dialog");
	id->gtxml = gtxml;

	id->dlg = GTK_DIALOG (glade_xml_get_widget (gtxml, "Active Dialog"));

	id->yes_btn = GTK_BUTTON(glade_xml_get_widget (gtxml, "yes button"));
	id->no_btn  = GTK_BUTTON(glade_xml_get_widget (gtxml, "no button"));
	id->help_btn = GTK_BUTTON(glade_xml_get_widget (gtxml, "helpbutton1"));
	id->active_label = GTK_LABEL (glade_xml_get_widget (gtxml, "active label"));
	id->credit_label = GTK_LABEL (glade_xml_get_widget (gtxml, "credit label"));
	w = glade_xml_get_widget (gtxml, "project option menu");
	id->project_menu = GTK_COMBO_BOX (w);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (w), renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (w), renderer, "text", 0, NULL);

	g_signal_connect(G_OBJECT(id->dlg), "destroy",
	          G_CALLBACK(dialog_close), id);

	g_signal_connect(G_OBJECT(id->yes_btn), "clicked",
	          G_CALLBACK(start_proj), id);

	g_signal_connect(G_OBJECT(id->no_btn), "clicked",
	          G_CALLBACK(dialog_kill), id);

	g_signal_connect(G_OBJECT(id->help_btn), "clicked",
	          G_CALLBACK(dialog_help), id);

}

/* =========================================================== */

GttActiveDialog *
active_dialog_new (void)
{
	GttActiveDialog *ad;

	ad = g_new0 (GttActiveDialog, 1);
	ad->gtxml = NULL;

	return ad;
}

/* =========================================================== */

void 
show_active_dialog (GttActiveDialog *ad)
{
	g_return_if_fail(ad);

	g_return_if_fail(!cur_proj);

	/* Due to GtkDialog broken-ness, re-realize the GUI */
	if (NULL == ad->gtxml)
	{
		active_dialog_realize (ad);
		setup_menus (ad);
	
		gtk_widget_show (GTK_WIDGET(ad->dlg));
	}
	else
	{
		raise_active_dialog (ad);
	}
}

/* =========================================================== */

void 
raise_active_dialog (GttActiveDialog *ad)
{

	g_return_if_fail(ad);
	g_return_if_fail(ad->gtxml);

	/* The following will raise the window, and put it on the current
	 * workspace, at least if the metacity WM is used. Haven't tried
	 * other window managers.
	 */
	gtk_window_present (GTK_WINDOW (ad->dlg));
}

/* =========================================================== */

void
active_dialog_activate_timer (GttActiveDialog *active_dialog)
{
	schedule_active_timeout (config_no_project_timeout, active_dialog);
}

void
active_dialog_deactivate_timer (GttActiveDialog *active_dialog)
{
	if (active_dialog->timeout_event_source)
	{
		g_source_remove (active_dialog->timeout_event_source);
		active_dialog->timeout_event_source = 0;
	}
}

/* =========================== END OF FILE ============================== */

