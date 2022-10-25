/*   Report Menu Editor for GTimeTracker - a time tracker
 *   Copyright (C) 2001,2003 Linas Vepstas <linas@linas.org>
 * Copyright (C) 2022      Markus Prasser
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

#include "gtt_gsettings_io.h"

#include <glib.h>
#include <gnome.h>

#include "app.h"
#include "dialog.h"
#include "journal.h"
#include "menus.h"
#include "plug-in.h"
#include "util.h"

struct PluginEditorDialog_s
{
  GtkDialog *dialog;

  GtkTreeView *treeview;
  GtkTreeStore *treestore;
  GArray *menus; /* array of GnomeUIInfo */

  GtkTreeSelection *selection;
  gboolean have_selection;
  GtkTreeIter curr_selection;
  gboolean do_redraw;

  GtkEntry *plugin_name; /* AKA 'Label' */
  GtkFileChooser *plugin_path;
  GtkEntry *plugin_tooltip;

  GnomeApp *app;
};

#define NCOLUMNS 4
#define PTRCOL (NCOLUMNS - 1)

/* ============================================================ */
/* Redraw one row of the tree widget */

static void
edit_plugin_redraw_row (struct PluginEditorDialog_s *ped, GtkTreeIter *iter,
                        GnomeUIInfo *uientry)
{
  GttPlugin *plg;
  GValue val = { G_TYPE_INVALID };
  GValue pval = { G_TYPE_INVALID };

  if (!uientry || !uientry->user_data)
    return;
  plg = uientry->user_data;

  g_value_init (&val, G_TYPE_STRING);
  g_value_set_string (&val, plg->name);
  gtk_tree_store_set_value (ped->treestore, iter, 0, &val);

  g_value_set_string (&val, plg->path);
  gtk_tree_store_set_value (ped->treestore, iter, 1, &val);

  g_value_set_string (&val, plg->tooltip);
  gtk_tree_store_set_value (ped->treestore, iter, 2, &val);

  g_value_init (&pval, G_TYPE_POINTER);
  g_value_set_pointer (&pval, uientry);
  gtk_tree_store_set_value (ped->treestore, iter, PTRCOL, &pval);
}

static void
edit_plugin_redraw_tree (struct PluginEditorDialog_s *ped)
{
  int i, rc;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GnomeUIInfo *uientry;

  /* Walk the current menu list */
  model = GTK_TREE_MODEL (ped->treestore);
  uientry = (GnomeUIInfo *)ped->menus->data;

  rc = gtk_tree_model_get_iter_first (model, &iter);
  for (i = 0; i < ped->menus->len; i++)
    {
      if (GNOME_APP_UI_ENDOFINFO == uientry[i].type)
        break;
      if (0 == rc)
        {
          gtk_tree_store_append (ped->treestore, &iter, NULL);
        }
      edit_plugin_redraw_row (ped, &iter, &uientry[i]);
      rc = gtk_tree_model_iter_next (model, &iter);
    }

  /* Now, delete the excess rows */
  while (rc)
    {
      GtkTreeIter next = iter;
      rc = gtk_tree_model_iter_next (model, &next);
      gtk_tree_store_remove (ped->treestore, &iter);
      iter = next;
    }
}

/* ============================================================ */

static void
edit_plugin_widgets_to_item (PluginEditorDialog *dlg, GnomeUIInfo *gui)
{
  const char *title, *path, *tip;
  GttPlugin *plg;

  if (!gui)
    return;

  /* Get the dialog contents */
  title = gtk_entry_get_text (dlg->plugin_name);
  path = gtk_file_chooser_get_uri (dlg->plugin_path);

  if (!path)
    {
      GtkWidget *mb;
      mb = gtk_message_dialog_new (
          NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
          _ ("You must specify a complete filepath to the report, "
             "including a leading slash.  The file that you specify "
             "must exist."));
      g_signal_connect (G_OBJECT (mb), "response",
                        G_CALLBACK (gtk_widget_destroy), mb);
      gtk_widget_show (mb);
    }

  tip = gtk_entry_get_text (dlg->plugin_tooltip);
  if (!path)
    path = "";
  if (!tip)
    path = "";

  /* set the values into the item */
  plg = gui->user_data;
  if (plg->name)
    g_free (plg->name);
  plg->name = g_strdup (title);
  if (plg->path)
    g_free (plg->path);
  plg->path = g_strdup (path);
  if (plg->tooltip)
    g_free (plg->tooltip);
  plg->tooltip = g_strdup (tip);

  gui->type = GNOME_APP_UI_ITEM;
  gui->label = plg->name;
  gui->hint = plg->tooltip;
  gui->moreinfo = invoke_report;
  gui->unused_data = NULL;
  gui->pixmap_type = GNOME_APP_PIXMAP_STOCK;
  gui->pixmap_info = GNOME_STOCK_BLANK;
  gui->accelerator_key = 0;
  gui->ac_mods = (GdkModifierType)0;
}

static void
edit_plugin_item_to_widgets (PluginEditorDialog *dlg, GnomeUIInfo *gui)
{
  GttPlugin *plg;

  if (!gui)
    return;
  plg = gui->user_data;

  gtk_entry_set_text (dlg->plugin_name, plg->name);
  gtk_file_chooser_set_uri (dlg->plugin_path, plg->path);
  gtk_entry_set_text (dlg->plugin_tooltip, plg->tooltip);
}

static void
edit_plugin_clear_widgets (PluginEditorDialog *dlg)
{
  gtk_entry_set_text (dlg->plugin_name, _ ("New Item"));
  gtk_file_chooser_unselect_all (dlg->plugin_path);
  gtk_entry_set_text (dlg->plugin_tooltip, "");
}

/* ============================================================ */
/* This callback is called when the user clicks on a different
 * tree-widget row.
 */

static void
edit_plugin_tree_selection_changed_cb (GtkTreeSelection *selection,
                                       gpointer data)
{
  PluginEditorDialog *dlg = data;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean have_selection;

  have_selection = gtk_tree_selection_get_selected (selection, &model, &iter);
  dlg->do_redraw = FALSE;
  if (dlg->have_selection)
    {
      GnomeUIInfo *curr_item;
      GValue val = { G_TYPE_INVALID };
      gtk_tree_model_get_value (model, &dlg->curr_selection, PTRCOL, &val);
      curr_item = g_value_get_pointer (&val);

      /* Save current values of widgets to current item */
      edit_plugin_widgets_to_item (dlg, curr_item);
    }

  if (have_selection)
    {
      GnomeUIInfo *curr_item;
      GValue val = { G_TYPE_INVALID };
      gtk_tree_model_get_value (model, &iter, PTRCOL, &val);
      curr_item = g_value_get_pointer (&val);

      dlg->have_selection = TRUE;
      dlg->curr_selection = iter;
      edit_plugin_item_to_widgets (dlg, curr_item);
    }
  else
    {
      dlg->have_selection = FALSE;
      edit_plugin_clear_widgets (dlg);
    }
  dlg->do_redraw = TRUE;
}

/* ============================================================ */
/*  This callback is called whenever a user types into the
 *  name or tooltip widgets, and causes the affected
 *  ctree row to be redrawn.
 */

static void
edit_plugin_changed_cb (GtkWidget *w, gpointer data)
{
  GnomeUIInfo *curr_item;
  GValue val = { G_TYPE_INVALID };
  PluginEditorDialog *dlg = data;

  if (!dlg->do_redraw)
    return;
  if (FALSE == dlg->have_selection)
    return;

  gtk_tree_model_get_value (GTK_TREE_MODEL (dlg->treestore),
                            &dlg->curr_selection, PTRCOL, &val);
  curr_item = g_value_get_pointer (&val);

  /* Save current values of widgets to current item */
  edit_plugin_widgets_to_item (dlg, curr_item);
  edit_plugin_redraw_row (dlg, &dlg->curr_selection, curr_item);
}

/* ============================================================ */
/* These routines make and delete a private, dialog-local copy
 * of menu system to be edited. They also do misc widget initialization
 * that needs to be done on a per-edit frequency.
 */

static void
edit_plugin_setup (PluginEditorDialog *dlg)
{
  int i, nitems;
  GnomeUIInfo *sysmenus;

  /* Copy-in existing menus from the system */
  sysmenus = gtt_get_reports_menu ();
  for (i = 0; GNOME_APP_UI_ENDOFINFO != sysmenus[i].type; i++)
    {
    }
  nitems = i + 1;

  dlg->menus = g_array_new (TRUE, FALSE, sizeof (GnomeUIInfo));
  dlg->menus = g_array_append_vals (dlg->menus, sysmenus, nitems);
  sysmenus = (GnomeUIInfo *)dlg->menus->data;
  for (i = 0; i < nitems; i++)
    {
      GttPlugin *plg = sysmenus[i].user_data;
      plg = gtt_plugin_copy (plg);
      sysmenus[i].user_data = plg;
      if (plg)
        {
          sysmenus[i].label = plg->name;
          sysmenus[i].hint = plg->tooltip;
        }
    }

  /* Redraw the tree */
  edit_plugin_redraw_tree (dlg);

  /* Hook up the row-selection callback */
  dlg->have_selection = FALSE;

  /* clear out the various widgets */
  edit_plugin_clear_widgets (dlg);
}

static void
edit_plugin_cleanup (PluginEditorDialog *dlg)
{
  GnomeUIInfo *sysmenus;
  int i;

  /* Free our local copy of menu structure */
  sysmenus = (GnomeUIInfo *)dlg->menus->data;
  for (i = 0; GNOME_APP_UI_ENDOFINFO != sysmenus[i].type; i++)
    {
    }
  {
    gtt_plugin_free (sysmenus[i].user_data);
  }
  g_array_free (dlg->menus, TRUE);
  dlg->menus = NULL;

  /* Unselect row in tree widget. */
  gtk_tree_selection_unselect_all (dlg->selection);
  dlg->have_selection = FALSE;

  /* Empty the tree widget too */
  gtk_tree_store_clear (dlg->treestore);
}

/* ============================================================ */
/* Copy the user's changes back to the system.
 */

static void
edit_plugin_apply_cb (GtkWidget *w, gpointer data)
{
  PluginEditorDialog *dlg = data;
  GnomeUIInfo *dlgmenu, *sysmenu;
  int i, nitems;

  /* Copy from local copy to system menus */
  dlgmenu = (GnomeUIInfo *)dlg->menus->data;
  nitems = dlg->menus->len;
  sysmenu = g_new0 (GnomeUIInfo, nitems);
  memcpy (sysmenu, dlgmenu, nitems * sizeof (GnomeUIInfo));
  for (i = 0; i < nitems - 1; i++)
    {
      GttPlugin *plg = dlgmenu[i].user_data;
      plg = gtt_plugin_copy (plg);
      sysmenu[i].user_data = plg;
      if (plg)
        {
          sysmenu[i].label = plg->name;
          sysmenu[i].hint = plg->tooltip;
        }
    }
  gtt_set_reports_menu (dlg->app, sysmenu);
}

static void
edit_plugin_commit_cb (GtkWidget *w, gpointer data)
{
  PluginEditorDialog *dlg = data;

  edit_plugin_apply_cb (w, data);
  edit_plugin_cleanup (dlg);
  gtk_widget_hide (GTK_WIDGET (dlg->dialog));

  /* Save to file, too.  That way, if system core dumps later,
   * at least we managed to get this set of changes saved. */
  gtt_save_reports_menu ();
}

/* ============================================================ */
/* Throw away the users changes made in this dialog.  Just clean up,
 * and that's it.
 */

static void
edit_plugin_cancel_cb (GtkWidget *w, gpointer data)
{
  PluginEditorDialog *dlg = data;

  edit_plugin_cleanup (dlg);
  gtk_widget_hide (GTK_WIDGET (dlg->dialog));
}

/* ============================================================ */
/* Get numeric index of the selected row */

static int
edit_plugin_get_index_of_selected_item (PluginEditorDialog *dlg)
{
  int i;
  GnomeUIInfo *curr_item;
  GnomeUIInfo *sysmenus;
  GValue val = { G_TYPE_INVALID };

  if (FALSE == dlg->have_selection)
    return -1;
  if (!dlg->menus)
    return -1;

  /* Get selected item */
  gtk_tree_model_get_value (GTK_TREE_MODEL (dlg->treestore),
                            &dlg->curr_selection, PTRCOL, &val);
  curr_item = g_value_get_pointer (&val);

  sysmenus = (GnomeUIInfo *)dlg->menus->data;
  for (i = 0; GNOME_APP_UI_ENDOFINFO != sysmenus[i].type; i++)
    {
      if (curr_item == &sysmenus[i])
        return i;
    }
  return -1;
}

/* ============================================================ */
/* Get the Iter, in the tree, of the indicated item */
static void
edit_plugin_get_iter_of_item (PluginEditorDialog *dlg, GnomeUIInfo *item,
                              GtkTreeIter *iter)
{
  int i, rc;
  GnomeUIInfo *uientry;
  GtkTreeModel *model;

  model = GTK_TREE_MODEL (dlg->treestore);
  rc = gtk_tree_model_get_iter_first (model, iter);

  if (!dlg->menus)
    return;
  uientry = (GnomeUIInfo *)dlg->menus->data;

  for (i = 0; i < dlg->menus->len; i++)
    {
      if (GNOME_APP_UI_ENDOFINFO == uientry[i].type)
        break;

      if (0 == rc)
        break;
      if (item == &uientry[i])
        return;

      rc = gtk_tree_model_iter_next (model, iter);
    }
}

/* ============================================================ */
/* Add and delete menu items callbacks */

static void
edit_plugin_add_cb (GtkWidget *w, gpointer data)
{
  PluginEditorDialog *dlg = data;
  GnomeUIInfo item, *uientry;
  GtkTreeIter iter;
  int index;
  GttPlugin *plg;

  /* Create a plugin, copy widget values into it. */
  plg = gtt_plugin_new ("x", "/x");
  if (!plg)
    return;

  item.user_data = plg;
  edit_plugin_widgets_to_item (dlg, &item);

  /* Insert item into list, or, if no selection, append */
  index = edit_plugin_get_index_of_selected_item (dlg);
  if (0 > index)
    index = dlg->menus->len - 1;

  g_array_insert_val (dlg->menus, index, item);

  /* Redraw the tree */
  edit_plugin_redraw_tree (dlg);

  /* Select the new row. Not strictly needed, unless there
   * had not been any selection previously.
   */
  uientry = (GnomeUIInfo *)dlg->menus->data;
  edit_plugin_get_iter_of_item (dlg, &uientry[index], &iter);
  gtk_tree_selection_select_iter (dlg->selection, &iter);
}

static void
edit_plugin_delete_cb (GtkWidget *w, gpointer data)
{
  int row;
  GnomeUIInfo *sysmenus;
  PluginEditorDialog *dlg = data;

  if (FALSE == dlg->have_selection)
    return;

  /* Get selected item */
  row = edit_plugin_get_index_of_selected_item (dlg);
  if (-1 == row)
    return;

  /* DO NOT delete the end-of-array marker */
  sysmenus = (GnomeUIInfo *)dlg->menus->data;
  if (GNOME_APP_UI_ENDOFINFO == sysmenus[row].type)
    return;

  dlg->menus = g_array_remove_index (dlg->menus, row);
  /* XXX mem leak .. should delete ui item */

  /* Redraw the tree */
  edit_plugin_redraw_tree (dlg);

  /* Update selected row, as appropriate */
  dlg->have_selection = FALSE;
  edit_plugin_tree_selection_changed_cb (dlg->selection, dlg);
}

/* ============================================================ */
#define ITER_EQ(a, b)                                                         \
  (((a).stamp == (b).stamp) && ((a).user_data == (b).user_data)               \
   && ((a).user_data2 == (b).user_data2)                                      \
   && ((a).user_data3 == (b).user_data3))

static gboolean
gtk_tree_model_iter_prev (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  int rc;
  GtkTreeIter cur, prev;

  rc = gtk_tree_model_get_iter_first (tree_model, &cur);
  if (ITER_EQ (cur, *iter))
    return 0;
  while (rc)
    {
      prev = cur;
      rc = gtk_tree_model_iter_next (tree_model, &cur);
      if (0 == rc)
        break;
      if (ITER_EQ (cur, *iter))
        {
          *iter = prev;
          return 1;
        }
    }
  return 0;
}

static void
edit_plugin_set_selection (PluginEditorDialog *dlg, int offset)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean have_sel;
  int rc;

  have_sel = gtk_tree_selection_get_selected (dlg->selection, &model, &iter);
  if (!have_sel)
    return;

  if (0 < offset)
    {
      while (offset)
        {
          rc = gtk_tree_model_iter_next (model, &iter);
          if (0 == rc)
            return;
          offset--;
        }
      gtk_tree_selection_select_iter (dlg->selection, &iter);
    }
  else
    {
      while (offset)
        {
          rc = gtk_tree_model_iter_prev (model, &iter);
          if (0 == rc)
            return;
          offset++;
        }
      gtk_tree_selection_select_iter (dlg->selection, &iter);
    }
}

/* ============================================================ */
/* Swap current selection with menu item at offset */

static void
edit_plugin_move_menu_item (PluginEditorDialog *dlg, int offset)
{
  int row, rowb;
  GnomeUIInfo *sysmenus, itema, itemb;

  if (FALSE == dlg->have_selection)
    return;

  /* Get selected item */
  row = edit_plugin_get_index_of_selected_item (dlg);
  if (-1 == row)
    return;

  /* DO NOT move the end-of-array marker */
  sysmenus = (GnomeUIInfo *)dlg->menus->data;
  if (GNOME_APP_UI_ENDOFINFO == sysmenus[row].type)
    return;

  rowb = row + offset;
  if ((0 > rowb) || (rowb >= dlg->menus->len))
    return;

  itema = g_array_index (dlg->menus, GnomeUIInfo, row);
  itemb = g_array_index (dlg->menus, GnomeUIInfo, rowb);

  g_array_index (dlg->menus, GnomeUIInfo, row) = itemb;
  g_array_index (dlg->menus, GnomeUIInfo, rowb) = itema;

  /* Redraw the tree */
  dlg->have_selection = FALSE;
  edit_plugin_redraw_tree (dlg);
  edit_plugin_set_selection (dlg, offset);
}

static void
edit_plugin_up_button_cb (GtkWidget *w, gpointer data)
{
  edit_plugin_move_menu_item (data, -1);
}

static void
edit_plugin_down_button_cb (GtkWidget *w, gpointer data)
{
  edit_plugin_move_menu_item (data, 1);
}

/* ============================================================ */

static void
edit_plugin_left_button_cb (GtkWidget *w, gpointer data)
{
  printf ("left button clicked\n");
}

/* ============================================================ */

static void
edit_plugin_right_button_cb (GtkWidget *w, gpointer data)
{
  printf ("right button clicked\n");
}

/* ============================================================ */
/* Create a new copy of the edit dialog; intialize all widgets, etc. */

PluginEditorDialog *
edit_plugin_dialog_new (void)
{
  PluginEditorDialog *dlg;
  int i;
  const char *col_titles[NCOLUMNS];

  dlg = g_malloc (sizeof (PluginEditorDialog));
  dlg->app = GNOME_APP (app_window);

  GtkWidget *const plugin_editor = gtk_dialog_new ();
  dlg->dialog = GTK_DIALOG (plugin_editor);
  gtk_widget_set_name (plugin_editor, "Plugin Editor");
  gtk_window_set_title (GTK_WINDOW (plugin_editor),
                        _ ("GnoTime Report Menu Editor"));
  gtk_window_set_type_hint (GTK_WINDOW (plugin_editor),
                            GDK_WINDOW_TYPE_HINT_NORMAL);

  GtkWidget *const action_area
      = gtk_dialog_get_action_area (GTK_DIALOG (plugin_editor));
  gtk_button_box_set_layout (GTK_BUTTON_BOX (action_area), GTK_BUTTONBOX_END);

  GtkWidget *const helpbutton1 = gtk_button_new_from_stock (GTK_STOCK_HELP);
  gtk_widget_set_can_default (helpbutton1, TRUE);
  gtk_widget_set_can_focus (helpbutton1, TRUE);
  gtk_widget_set_name (helpbutton1, "helpbutton1");
  gtk_widget_show (helpbutton1);

  gtk_box_pack_start_defaults (GTK_BOX (action_area), helpbutton1);

  GtkWidget *const cancelbutton1
      = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_set_can_default (cancelbutton1, TRUE);
  gtk_widget_set_can_focus (cancelbutton1, TRUE);
  gtk_widget_set_name (cancelbutton1, "cancelbutton1");
  gtk_widget_set_tooltip_text (
      cancelbutton1, _ ("Abandon all changes and close this dialog window."));
  g_signal_connect (G_OBJECT (cancelbutton1), "clicked",
                    G_CALLBACK (edit_plugin_cancel_cb), dlg);
  gtk_widget_show (cancelbutton1);

  gtk_box_pack_start_defaults (GTK_BOX (action_area), cancelbutton1);

  GtkWidget *const applybutton1 = gtk_button_new_from_stock (GTK_STOCK_APPLY);
  gtk_widget_set_can_default (applybutton1, TRUE);
  gtk_widget_set_can_focus (applybutton1, TRUE);
  gtk_widget_set_name (applybutton1, "applybutton1");
  gtk_widget_set_tooltip_text (applybutton1,
                               _ ("Apply the changes to the reports menu."));
  g_signal_connect (G_OBJECT (applybutton1), "clicked",
                    G_CALLBACK (edit_plugin_apply_cb), dlg);
  gtk_widget_show (applybutton1);

  gtk_box_pack_start_defaults (GTK_BOX (action_area), applybutton1);

  GtkWidget *const okbutton1 = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_widget_set_can_default (okbutton1, TRUE);
  gtk_widget_set_can_focus (okbutton1, TRUE);
  gtk_widget_set_name (okbutton1, "okbutton1");
  gtk_widget_set_tooltip_text (okbutton1,
                               _ ("Apply the changes to the reports menu, and "
                                  "close this dialog window."));
  g_signal_connect (G_OBJECT (okbutton1), "clicked",
                    G_CALLBACK (edit_plugin_commit_cb), dlg);
  gtk_widget_show (okbutton1);

  gtk_box_pack_start_defaults (GTK_BOX (action_area), okbutton1);

  gtk_widget_show (action_area);

  GtkWidget *const content_area
      = gtk_dialog_get_content_area (GTK_DIALOG (plugin_editor));

  GtkWidget *const hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox1, "hbox1");

  GtkWidget *const vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox1, "vbox1");

  GtkWidget *const scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_can_focus (scrolledwindow1, TRUE);
  gtk_widget_set_name (scrolledwindow1, "scrolledwindow1");

  GtkWidget *const editor_treeview = gtk_tree_view_new ();
  dlg->treeview = GTK_TREE_VIEW (editor_treeview);
  gtk_widget_set_can_focus (editor_treeview, TRUE);
  gtk_widget_set_name (editor_treeview, "editor treeview");
  gtk_widget_show (editor_treeview);

  gtk_container_add (GTK_CONTAINER (scrolledwindow1), editor_treeview);
  gtk_widget_show (scrolledwindow1);

  gtk_box_pack_start_defaults (GTK_BOX (vbox1), scrolledwindow1);

  GtkWidget *const hbuttonbox1 = gtk_hbutton_box_new ();
  gtk_widget_set_name (hbuttonbox1, "hbuttonbox1");

  GtkWidget *const up_button = gtk_button_new_from_stock (GTK_STOCK_GO_UP);
  gtk_widget_set_can_default (up_button, TRUE);
  gtk_widget_set_can_focus (up_button, TRUE);
  gtk_widget_set_name (up_button, "up button");
  gtk_widget_set_tooltip_text (
      up_button, _ ("Move the selected menu item up by one entry."));
  g_signal_connect (G_OBJECT (up_button), "clicked",
                    G_CALLBACK (edit_plugin_up_button_cb), dlg);
  gtk_widget_show (up_button);

  gtk_box_pack_start_defaults (GTK_BOX (hbuttonbox1), up_button);

  GtkWidget *const down_button = gtk_button_new_from_stock (GTK_STOCK_GO_DOWN);
  gtk_widget_set_can_default (down_button, TRUE);
  gtk_widget_set_can_focus (down_button, TRUE);
  gtk_widget_set_name (down_button, "down button");
  gtk_widget_set_tooltip_text (
      down_button, _ ("Move the selected menu item down by one entry."));
  g_signal_connect (G_OBJECT (down_button), "clicked",
                    G_CALLBACK (edit_plugin_down_button_cb), dlg);
  gtk_widget_show (down_button);

  gtk_box_pack_start_defaults (GTK_BOX (hbuttonbox1), down_button);

  GtkWidget *const left_button = gtk_button_new_from_stock (GTK_STOCK_GO_BACK);
  gtk_widget_set_can_default (left_button, TRUE);
  gtk_widget_set_can_focus (left_button, TRUE);
  gtk_widget_set_name (left_button, "left button");
  g_signal_connect (G_OBJECT (left_button), "clicked",
                    G_CALLBACK (edit_plugin_left_button_cb), dlg);
  gtk_widget_show (left_button);

  gtk_box_pack_start_defaults (GTK_BOX (hbuttonbox1), left_button);

  GtkWidget *const right_button
      = gtk_button_new_from_stock (GTK_STOCK_GO_FORWARD);
  gtk_widget_set_can_default (right_button, TRUE);
  gtk_widget_set_can_focus (right_button, TRUE);
  gtk_widget_set_name (right_button, "right button");
  g_signal_connect (G_OBJECT (right_button), "clicked",
                    G_CALLBACK (edit_plugin_right_button_cb), dlg);
  gtk_widget_show (right_button);

  gtk_box_pack_start_defaults (GTK_BOX (hbuttonbox1), right_button);

  gtk_widget_show (hbuttonbox1);

  gtk_box_pack_start (GTK_BOX (vbox1), hbuttonbox1, FALSE, TRUE, 0);

  gtk_widget_show (vbox1);

  gtk_box_pack_start_defaults (GTK_BOX (hbox1), vbox1);

  GtkWidget *const vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox2, "vbox2");

  GtkWidget *const table1 = gtk_table_new (4, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table1), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table1), 2);
  gtk_widget_set_name (table1, "table1");

  GtkWidget *const label1 = gtk_label_new (_ ("Label:"));
  gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);
  gtk_widget_set_name (label1, "label1");
  gtk_widget_show (label1);

  gtk_table_attach (GTK_TABLE (table1), label1, 0, 1, 0, 1, GTK_FILL, 0, 4, 0);

  GtkWidget *const plugin_name = gtk_entry_new ();
  dlg->plugin_name = GTK_ENTRY (plugin_name);
  gtk_entry_set_invisible_char (GTK_ENTRY (plugin_name), '*');
  gtk_widget_set_can_focus (plugin_name, TRUE);
  gtk_widget_set_name (plugin_name, "plugin name");
  gtk_widget_set_tooltip_text (plugin_name,
                               _ ("Enter the name for the menu item here."));
  g_signal_connect (G_OBJECT (plugin_name), "changed",
                    G_CALLBACK (edit_plugin_changed_cb), dlg);
  gtk_widget_show (plugin_name);

  gtk_table_attach (GTK_TABLE (table1), plugin_name, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const label2 = gtk_label_new (_ ("Path:"));
  gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);
  gtk_widget_set_name (label2, "label2");
  gtk_widget_show (label2);

  gtk_table_attach (GTK_TABLE (table1), label2, 0, 1, 1, 2, GTK_FILL, 0, 4, 0);

  GtkWidget *const plugin_path
      = gtk_file_chooser_button_new (NULL, GTK_FILE_CHOOSER_ACTION_OPEN);
  dlg->plugin_path = GTK_FILE_CHOOSER (plugin_path);
  gtk_widget_set_events (plugin_path, GDK_BUTTON_PRESS_MASK
                                          | GDK_BUTTON_RELEASE_MASK
                                          | GDK_POINTER_MOTION_HINT_MASK
                                          | GDK_POINTER_MOTION_MASK);
  gtk_widget_set_name (plugin_path, "plugin path");
  g_signal_connect (G_OBJECT (plugin_path), "changed",
                    G_CALLBACK (edit_plugin_changed_cb), dlg);
  gtk_widget_show (plugin_path);

  gtk_table_attach_defaults (GTK_TABLE (table1), plugin_path, 1, 2, 1, 2);

  GtkWidget *const label3 = gtk_label_new (_ ("Tooltip:"));
  gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);
  gtk_widget_set_name (label3, "label3");
  gtk_widget_show (label3);

  gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 2, 3, GTK_FILL, 0, 4, 0);

  GtkWidget *const plugin_tooltip = gtk_entry_new ();
  dlg->plugin_tooltip = GTK_ENTRY (plugin_tooltip);
  gtk_entry_set_invisible_char (GTK_ENTRY (plugin_tooltip), '*');
  gtk_widget_set_can_focus (plugin_tooltip, TRUE);
  gtk_widget_set_name (plugin_tooltip, "plugin tooltip");
  gtk_widget_set_tooltip_text (
      plugin_tooltip, _ ("Enter the popup hint for the menu item here."));
  g_signal_connect (G_OBJECT (plugin_tooltip), "changed",
                    G_CALLBACK (edit_plugin_changed_cb), dlg);
  gtk_widget_show (plugin_tooltip);

  gtk_table_attach (GTK_TABLE (table1), plugin_tooltip, 1, 2, 2, 3,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const label4 = gtk_label_new (_ ("Icon:"));
  gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);
  gtk_widget_set_name (label4, "label4");
  gtk_widget_show (label4);

  gtk_table_attach (GTK_TABLE (table1), label4, 0, 1, 3, 4, GTK_FILL, 0, 4, 0);

  GtkWidget *const icon_path
      = gtk_file_chooser_button_new (NULL, GTK_FILE_CHOOSER_ACTION_OPEN);
  gtk_widget_set_events (
      icon_path, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
                     | GDK_POINTER_MOTION_HINT_MASK | GDK_POINTER_MOTION_MASK);
  gtk_widget_set_name (icon_path, "icon path");
  gtk_widget_show (icon_path);

  gtk_table_attach_defaults (GTK_TABLE (table1), icon_path, 1, 2, 3, 4);

  gtk_widget_show (table1);

  gtk_box_pack_start_defaults (GTK_BOX (vbox2), table1);

  GtkWidget *const table2 = gtk_table_new (2, 2, TRUE);
  gtk_table_set_col_spacings (GTK_TABLE (table2), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table2), 4);
  gtk_widget_set_name (table2, "table2");

  GtkWidget *const add_button = gtk_button_new_with_mnemonic (_ ("Add"));
  gtk_button_set_use_underline (GTK_BUTTON (add_button), TRUE);
  gtk_widget_set_can_focus (add_button, TRUE);
  gtk_widget_set_name (add_button, "add button");
  gtk_widget_set_tooltip_text (
      add_button, _ ("Add a new menu entry at the selected position."));
  g_signal_connect (G_OBJECT (add_button), "clicked",
                    G_CALLBACK (edit_plugin_add_cb), dlg);
  gtk_widget_show (add_button);

  gtk_table_attach (GTK_TABLE (table2), add_button, 0, 1, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const child_button
      = gtk_button_new_with_mnemonic (_ ("Add Child"));
  gtk_button_set_use_underline (GTK_BUTTON (child_button), TRUE);
  gtk_widget_set_can_focus (child_button, TRUE);
  gtk_widget_set_name (child_button, "child button");
  gtk_widget_show (child_button);

  gtk_table_attach (GTK_TABLE (table2), child_button, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const separator_button
      = gtk_button_new_with_mnemonic (_ ("Add Separator"));
  gtk_button_set_use_underline (GTK_BUTTON (separator_button), TRUE);
  gtk_widget_set_can_focus (separator_button, TRUE);
  gtk_widget_set_name (separator_button, "separator button");
  gtk_widget_set_tooltip_text (
      separator_button, _ ("Add a horizontal bar (separator) to the menu."));
  gtk_widget_show (separator_button);

  gtk_table_attach (GTK_TABLE (table2), separator_button, 0, 1, 1, 2,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  GtkWidget *const delete_button = gtk_button_new_with_mnemonic (_ ("Delete"));
  gtk_button_set_use_underline (GTK_BUTTON (delete_button), TRUE);
  gtk_widget_set_can_focus (delete_button, TRUE);
  gtk_widget_set_name (delete_button, "delete button");
  gtk_widget_set_tooltip_text (delete_button,
                               _ ("Delete the selected menu entry."));
  g_signal_connect (G_OBJECT (delete_button), "clicked",
                    G_CALLBACK (edit_plugin_delete_cb), dlg);
  gtk_widget_show (delete_button);

  gtk_table_attach (GTK_TABLE (table2), delete_button, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  gtk_widget_show (table2);

  gtk_box_pack_start_defaults (GTK_BOX (vbox2), table2);

  GtkWidget *const frame1 = gtk_frame_new (_ ("Accelerator"));
  gtk_frame_set_label_align (GTK_FRAME (frame1), 0, 0.5);
  gtk_widget_set_name (frame1, "frame1");

  GtkWidget *const vbox3 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox3, "vbox3");

  GtkWidget *const hbox3 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox3, "hbox3");

  GtkWidget *const label7 = gtk_label_new (_ ("Modifiers:"));
  gtk_widget_set_name (label7, "label7");
  gtk_widget_show (label7);

  gtk_box_pack_start (GTK_BOX (hbox3), label7, FALSE, FALSE, 4);

  GtkWidget *const checkbutton1
      = gtk_check_button_new_with_mnemonic (_ ("Ctrl"));
  gtk_button_set_use_underline (GTK_BUTTON (checkbutton1), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (checkbutton1), TRUE);
  gtk_widget_set_can_focus (checkbutton1, TRUE);
  gtk_widget_set_name (checkbutton1, "checkbutton1");
  gtk_widget_show (checkbutton1);

  gtk_box_pack_start (GTK_BOX (hbox3), checkbutton1, FALSE, FALSE, 0);

  GtkWidget *const checkbutton2
      = gtk_check_button_new_with_mnemonic (_ ("Shift"));
  gtk_button_set_use_underline (GTK_BUTTON (checkbutton2), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (checkbutton2), TRUE);
  gtk_widget_set_can_focus (checkbutton2, TRUE);
  gtk_widget_set_name (checkbutton2, "checkbutton2");
  gtk_widget_show (checkbutton2);

  gtk_box_pack_start (GTK_BOX (hbox3), checkbutton2, FALSE, FALSE, 0);

  GtkWidget *const checkbutton3
      = gtk_check_button_new_with_mnemonic (_ ("Alt"));
  gtk_button_set_use_underline (GTK_BUTTON (checkbutton3), TRUE);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (checkbutton3), TRUE);
  gtk_widget_set_can_focus (checkbutton3, TRUE);
  gtk_widget_set_name (checkbutton3, "checkbutton3");
  gtk_widget_show (checkbutton3);

  gtk_box_pack_start (GTK_BOX (hbox3), checkbutton3, FALSE, FALSE, 0);

  gtk_widget_show (hbox3);

  gtk_box_pack_start_defaults (GTK_BOX (vbox3), hbox3);

  GtkWidget *const hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox2, "hbox2");

  GtkWidget *const label6 = gtk_label_new (_ ("Key:"));
  gtk_widget_set_name (label6, "label6");
  gtk_widget_show (label6);

  gtk_box_pack_start (GTK_BOX (hbox2), label6, FALSE, FALSE, 4);

  GtkWidget *const entry4 = gtk_entry_new ();
  gtk_entry_set_invisible_char (GTK_ENTRY (entry4), '*');
  gtk_widget_set_can_focus (entry4, TRUE);
  gtk_widget_set_name (entry4, "entry4");
  gtk_widget_set_tooltip_text (
      entry4, _ ("Enter the keyboard shortcut for this menu item."));
  gtk_widget_show (entry4);

  gtk_box_pack_start_defaults (GTK_BOX (hbox2), entry4);

  gtk_widget_show (hbox2);

  gtk_box_pack_start_defaults (GTK_BOX (vbox3), hbox2);

  gtk_widget_show (vbox3);

  gtk_container_add (GTK_CONTAINER (frame1), vbox3);
  gtk_widget_show (frame1);

  gtk_box_pack_start_defaults (GTK_BOX (vbox2), frame1);

  gtk_widget_show (vbox2);

  gtk_box_pack_start_defaults (GTK_BOX (hbox1), vbox2);

  gtk_widget_show (hbox1);

  gtk_box_pack_start_defaults (GTK_BOX (content_area), hbox1);

  gtk_widget_show (content_area);

  gtk_widget_show (plugin_editor);

  /* ------------------------------------------------------ */
  /* Set up the Treeview Widget */
  {
    GType col_type[NCOLUMNS];
    for (i = 0; i < NCOLUMNS - 1; i++)
      {
        col_type[i] = G_TYPE_STRING;
      }
    col_type[NCOLUMNS - 1] = G_TYPE_POINTER;
    dlg->treestore = gtk_tree_store_newv (NCOLUMNS, col_type);
  }
  gtk_tree_view_set_model (dlg->treeview, GTK_TREE_MODEL (dlg->treestore));

  /* Set up the columns in the treeview widget */
  col_titles[0] = "Name";
  col_titles[1] = "Path";
  col_titles[2] = "Tooltip";
  for (i = 0; i < NCOLUMNS - 1; i++)
    {
      GtkTreeViewColumn *col;
      GtkCellRenderer *renderer;

      renderer = gtk_cell_renderer_text_new ();

      col = gtk_tree_view_column_new_with_attributes (col_titles[i], renderer,
                                                      "text", i, NULL);

      gtk_tree_view_insert_column (dlg->treeview, col, i);
    }

  /* Copy-in existing menus from the system */
  edit_plugin_setup (dlg);

  /* Hook up the row-selection callback */
  dlg->have_selection = FALSE;
  dlg->selection = gtk_tree_view_get_selection (dlg->treeview);
  gtk_tree_selection_set_mode (dlg->selection, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (dlg->selection), "changed",
                    G_CALLBACK (edit_plugin_tree_selection_changed_cb), dlg);

  gtk_widget_hide_on_delete (GTK_WIDGET (dlg->dialog));

  return dlg;
}

/* ============================================================ */

void
edit_plugin_dialog_show (PluginEditorDialog *dlg)
{
  if (!dlg)
    return;
  gtk_widget_show (GTK_WIDGET (dlg->dialog));
}

void
edit_plugin_dialog_destroy (PluginEditorDialog *dlg)
{
  if (!dlg)
    return;
  gtk_widget_destroy (GTK_WIDGET (dlg->dialog));
  g_free (dlg);
}

/* ============================================================ */

static PluginEditorDialog *epdlg = NULL;

void
report_menu_edit (GtkWidget *widget, gpointer data)
{
  if (!epdlg)
    epdlg = edit_plugin_dialog_new ();
  edit_plugin_setup (epdlg);
  gtk_widget_show (GTK_WIDGET (epdlg->dialog));
}

/* ====================== END OF FILE ========================= */
