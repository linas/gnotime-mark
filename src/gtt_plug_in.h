/*   Report Plugins for GTimeTracker - a time tracker
 *   Copyright (C) 2001 Linas Vepstas <linas@linas.org>
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

#ifndef GTT_PLUG_IN_H
#define GTT_PLUG_IN_H

#include <glib.h>
#include <gtk/gtk.h>

/**
 * Somewhat misnamed, this is really just a description of the
 * reports & report menu that the user can customize.  Each
 * 'plugin' is just an entry in the report menu.
 */

typedef struct GttPlugin_s
{
  char *name; /* Name of report, shows up in menu */
  char *tooltip;
  char *path;     /* Path to report in file system */
  char *last_url; /* Place where user last saved this report */

} GttPlugin;

/* Simple allocator */
GttPlugin *gtt_plugin_new (const char *name, const char *path);
GttPlugin *gtt_plugin_copy (GttPlugin *orig);
void gtt_plugin_free (GttPlugin *plg);

/*-------------------------------------------- */
/* A really simple, stupid GUI that allows user to enter in the path
 * name to a ghtml file, and then paste it into a menu.  The idea
 * here is to make it as easy as possible for new users to create
 * modified/custom reports.
 */

typedef struct NewPluginDialog_s NewPluginDialog;

NewPluginDialog *new_plugin_dialog_new (void);
void new_plugin_dialog_show (NewPluginDialog *dlg);
void new_plugin_dialog_destroy (NewPluginDialog *dlg);

void new_report (GtkWidget *widget, gpointer data);

/*-------------------------------------------- */
/* A fairly complex GUI allowing the user to edit the Reports menu,
 * and add/remove/rearrange/rename reports.
 */

typedef struct PluginEditorDialog_s PluginEditorDialog;

PluginEditorDialog *edit_plugin_dialog_new (void);
void edit_plugin_dialog_show (PluginEditorDialog *dlg);
void edit_plugin_dialog_destroy (PluginEditorDialog *dlg);

void report_menu_edit (GtkWidget *widget, gpointer data);

#endif // GTT_PLUG_IN_H
