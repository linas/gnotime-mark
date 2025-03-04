/*   Notes Area display of project notes for GTimeTracker
 *   Copyright (C) 2003 Linas Vepstas <linas@linas.org>
 * Copyright (C) 2023      Markus Prasser
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

#include "gtt_notes_area.h"

#include <glib-object.h>

#include "gtt_menus.h"
#include "gtt_project.h"
#include "gtt_props_dlg_task.h"
#include "gtt_util.h"

struct NotesArea_s
{
  GtkPaned *vpane; /* top level pane */
  GtkContainer
      *projects_tree_holder; /* scrolled widget that holds the projects tree */

  GtkPaned *hpane; /* left-right divider */

  GtkEntry *proj_title;
  GtkEntry *proj_desc;
  GtkTextView *proj_notes;

  GtkComboBox *task_combo;
  GtkTextView *task_notes;

  GtkButton *close_proj;
  GtkButton *close_task;
  GtkButton *new_task;
  GtkButton *edit_task;

  GttProject *proj;

  /* The goal of 'ignore events' is to prevent an inifinite
   * loop of cascading events as we modify the project and the GUI.
   */
  gboolean ignore_events;

  /* The goal of the freezes is to prevent more than one update
   * of windows per second.  The problem is that without this,
   * there would be one event per keystroke, which could cause
   * a redraw of e.g. the journal window.  In such a case, even
   * moderate typists on a slow CPU could saturate the CPU entirely.
   */
  gboolean proj_freeze;
  GttTask *task_freeze;
};

#ifdef GTT_TODO_COLUMN_MENU
static GtkWidget *build_column_menu (void);
#endif // GTT_TODO_COLUMN_MENU

/* ============================================================== */

#define TSK_SETUP                                                             \
  GttTask *tsk;                                                               \
  const char *str;                                                            \
  if (NULL == na->proj)                                                       \
    return;                                                                   \
  if (na->ignore_events)                                                      \
    return;                                                                   \
                                                                              \
  na->ignore_events = TRUE;                                                   \
  tsk = gtt_project_get_current_task (na->proj);                              \
  if (NULL == tsk)                                                            \
    {                                                                         \
      tsk = gtt_task_new ();                                                  \
      gtt_project_prepend_task (na->proj, tsk);                               \
    }                                                                         \
  if (tsk != na->task_freeze)                                                 \
    {                                                                         \
      /* Try to avoid race condition if another task */                       \
      /* is created while this task is frozen. */                             \
      if (NULL != na->task_freeze)                                            \
        gtt_task_thaw (na->task_freeze);                                      \
      na->task_freeze = tsk;                                                  \
    }

#ifdef UNUSED_CODE_RIGHT_NOW
static void
task_memo_changed (GtkEntry *entry, NotesArea *na)
{
  TSK_SETUP;
  str = gtk_entry_get_text (entry);
  gtt_task_set_memo (tsk, str);
  na->ignore_events = FALSE;
}
#endif /* UNUSED_CODE_RIGHT_NOW */

/* ============================================================== */

static void
task_notes_changed (GtkTextBuffer *entry, NotesArea *na)
{
  TSK_SETUP;
  str = xxxgtk_textview_get_text (na->task_notes);
  gtt_task_set_notes (tsk, str);
  na->ignore_events = FALSE;
}

/* ============================================================== */

#define PRJ_SETUP                                                             \
  const char *str;                                                            \
  if (NULL == na->proj)                                                       \
    return;                                                                   \
  if (na->ignore_events)                                                      \
    return;                                                                   \
                                                                              \
  if (FALSE == na->proj_freeze)                                               \
    {                                                                         \
      na->proj_freeze = TRUE;                                                 \
    }                                                                         \
  na->ignore_events = TRUE;

static void
proj_title_changed (GtkEntry *entry, NotesArea *na)
{
  PRJ_SETUP
  str = gtk_entry_get_text (entry);
  gtt_project_set_title (na->proj, str);
  na->ignore_events = FALSE;
}

/* ============================================================== */

static void
proj_desc_changed (GtkEntry *entry, NotesArea *na)
{
  PRJ_SETUP
  str = gtk_entry_get_text (entry);
  gtt_project_set_desc (na->proj, str);
  na->ignore_events = FALSE;
}

/* ============================================================== */
static GttTask *
tasks_model_get_task (GtkTreeModel *model, GtkTreeIter *iter)
{

  GValue v = { G_TYPE_INVALID };
  gtk_tree_model_get_value (model, iter, 1, &v);
  GttTask *task = (GttTask *)g_value_get_pointer (&v);
  g_value_unset (&v);
  return task;
}

/* ============================================================== */

static void
proj_notes_changed (GtkTextBuffer *entry, NotesArea *na)
{
  PRJ_SETUP
  str = xxxgtk_textview_get_text (na->proj_notes);
  gtt_project_set_notes (na->proj, str);
  na->ignore_events = FALSE;
}

/* ============================================================== */
/* This routine will cause pending events to get delivered. */

void
gtt_notes_timer_callback (NotesArea *na)
{
  if (!na)
    return;
  na->ignore_events = TRUE;
  if (na->task_freeze)
    {
      gtt_task_thaw (na->task_freeze);
      na->task_freeze = NULL;
    }
  if (na->proj_freeze)
    {
      na->proj_freeze = FALSE;
      gtt_project_thaw (na->proj);
    }
  na->ignore_events = FALSE;
}

/* ============================================================== */
/* These are some strange routines used to close off paned areas.
 * They are weird, they need to guess at sizes to close off the
 * panes.  This should really be a gkpaned built-in function.
 */

#define CLOSED_MARGIN 10

static void
close_proj_area (GtkButton *but, NotesArea *na)
{
  int hpane_width;
  int hpane_div;

  hpane_width = GTK_WIDGET (na->hpane)->allocation.width;
  hpane_div = gtk_paned_get_position (na->hpane);

  if (hpane_div > hpane_width - CLOSED_MARGIN)
    {
      int vpane_height;
      vpane_height = GTK_WIDGET (na->vpane)->allocation.height;
      gtk_paned_set_position (na->vpane, vpane_height);
    }
  else
    {
      gtk_paned_set_position (na->hpane, 0);
    }
}

static void
close_task_area (GtkButton *but, NotesArea *na)
{
  int hpane_width;
  int hpane_div;

  hpane_width = GTK_WIDGET (na->hpane)->allocation.width;
  hpane_div = gtk_paned_get_position (na->hpane);

  /* XXX we really need only the first test, but the second
   * one deals iwth a freaky gtk vpaned bug that makes this
   * hidden button active.  Whatever.
   */
  if ((hpane_div < CLOSED_MARGIN) || (hpane_div > hpane_width - CLOSED_MARGIN))
    {
      int vpane_height;
      vpane_height = GTK_WIDGET (na->vpane)->allocation.height;
      gtk_paned_set_position (na->vpane, vpane_height);
    }
  else
    {
      gtk_paned_set_position (na->hpane, hpane_width);
    }
}

/* ============================================================== */

static void
new_task_cb (GtkButton *but, NotesArea *na)
{
  GttTask *tsk;
  if (NULL == na->proj)
    return;
  // if (na->ignore_events) return;

  // na->ignore_events = TRUE;
  tsk = gtt_task_new ();
  gtt_project_prepend_task (na->proj, tsk);
  prop_task_dialog_show (tsk);
  if (NULL != na->task_freeze)
    gtt_task_thaw (na->task_freeze);
  gtt_task_freeze (tsk);
  na->task_freeze = tsk;

  // na->ignore_events = FALSE;
}

/* ============================================================== */
static void
edit_task_cb (GtkButton *but, NotesArea *na)
{
  if (NULL == na->proj)
    return;
  GttTask *tsk = gtt_project_get_current_task (na->proj);
  prop_task_dialog_show (tsk);
  if (NULL != na->task_freeze)
    gtt_task_thaw (na->task_freeze);
  gtt_task_freeze (tsk);
  na->task_freeze = tsk;
}

/* ============================================================== */
static void
task_selected_cb (GtkComboBox *combo, gpointer dialog)
{
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_combo_box_get_model (combo);
  NotesArea *na = (NotesArea *)dialog;

  na->ignore_events = TRUE;

  gtk_combo_box_get_active_iter (combo, &iter);
  GttTask *task = tasks_model_get_task (model, &iter);
  GttProject *project = gtt_task_get_parent (task);

  gtt_project_set_current_task (project, task);

  const char *str = gtt_task_get_notes (task);
  if (!str)
    str = "";
  xxxgtk_textview_set_text (na->task_notes, str);

  na->ignore_events = FALSE;
}

/* ============================================================== */

enum
{
  COL_ID,
  COL_TASK_MEMO,
  COL_TASK_POINTER,
  NUM_COLS
};

/* ============================================================== */

GtkEntry *
connect_entry (GtkWidget *const widget, GCallback cb, NotesArea *const na)
{
  g_signal_connect (G_OBJECT (widget), "changed", cb, na);

  return GTK_ENTRY (widget);
}

GtkTextView *
connect_text (GtkWidget *const widget, GCallback cb, NotesArea *const na)
{
  GtkTextView *const tv = GTK_TEXT_VIEW (widget);

  GtkTextBuffer *const tb = gtk_text_view_get_buffer (tv);
  g_signal_connect (G_OBJECT (tb), "changed", cb, na);

  return tv;
}

NotesArea *
notes_area_new (void)
{
  NotesArea *dlg;

  dlg = g_new0 (NotesArea, 1);

  GtkWidget *const top_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (top_window, "top window");
  gtk_window_set_title (GTK_WINDOW (top_window), _ ("window1"));

  GtkWidget *const notes_vpane = gtk_vpaned_new ();
  dlg->vpane = GTK_PANED (notes_vpane);
  gtk_widget_set_can_focus (notes_vpane, TRUE);
  gtk_widget_set_name (notes_vpane, "notes vpane");

  GtkWidget *const ctree_holder = gtk_scrolled_window_new (NULL, NULL);
  dlg->projects_tree_holder = GTK_CONTAINER (ctree_holder);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ctree_holder),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_can_focus (ctree_holder, TRUE);
  gtk_widget_set_name (ctree_holder, "ctree holder");
  gtk_widget_show (ctree_holder);

  gtk_paned_pack1 (GTK_PANED (notes_vpane), ctree_holder, FALSE, TRUE);

  GtkWidget *const leftright_hpane = gtk_hpaned_new ();
  dlg->hpane = GTK_PANED (leftright_hpane);
  gtk_widget_set_can_focus (leftright_hpane, TRUE);
  gtk_widget_set_name (leftright_hpane, "leftright pane");

  GtkWidget *const vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox1, "vbox1");

  GtkWidget *const hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox1, "hbox1");

  GtkWidget *const label1 = gtk_label_new (_ ("Project Title:"));
  gtk_widget_set_name (label1, "label1");
  gtk_widget_show (label1);

  gtk_box_pack_start (GTK_BOX (hbox1), label1, FALSE, FALSE, 0);

  GtkWidget *const close_proj_button = gtk_button_new ();
  dlg->close_proj = GTK_BUTTON (close_proj_button);
  gtk_button_set_relief (GTK_BUTTON (close_proj_button), GTK_RELIEF_NONE);
  gtk_widget_set_can_focus (close_proj_button, TRUE);
  gtk_widget_set_name (close_proj_button, "close proj button");
  gtk_widget_set_receives_default (close_proj_button, FALSE);
  gtk_widget_set_tooltip_text (close_proj_button,
                               _ ("Close the project subwindow"));

  GtkWidget *const image1
      = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_name (image1, "image1");
  gtk_widget_show (image1);

  gtk_button_set_image (GTK_BUTTON (close_proj_button), image1);
  g_signal_connect (G_OBJECT (close_proj_button), "clicked",
                    G_CALLBACK (close_proj_area), dlg);
  gtk_widget_show (close_proj_button);

  gtk_box_pack_start (GTK_BOX (hbox1), close_proj_button, FALSE, FALSE, 0);

  GtkWidget *const proj_title_entry = gtk_entry_new ();
  dlg->proj_title
      = connect_entry (proj_title_entry, G_CALLBACK (proj_title_changed), dlg);
  gtk_widget_set_can_focus (proj_title_entry, TRUE);
  gtk_widget_set_name (proj_title_entry, "proj title entry");
  gtk_widget_set_tooltip_text (proj_title_entry,
                               _ ("Edit the project title in this box"));
  gtk_widget_show (proj_title_entry);

  gtk_box_pack_start (GTK_BOX (hbox1), proj_title_entry, TRUE, TRUE, 4);

  gtk_widget_show (hbox1);

  gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, FALSE, 0);

  GtkWidget *const hbox3 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox3, "hbox3");

  GtkWidget *const label3 = gtk_label_new (_ ("Desc:"));
  gtk_widget_set_name (label3, "label3");
  gtk_widget_show (label3);

  gtk_box_pack_start (GTK_BOX (hbox3), label3, FALSE, FALSE, 0);

  GtkWidget *const proj_desc_entry = gtk_entry_new ();
  dlg->proj_desc
      = connect_entry (proj_desc_entry, G_CALLBACK (proj_desc_changed), dlg);
  gtk_widget_set_can_focus (proj_desc_entry, TRUE);
  gtk_widget_set_name (proj_desc_entry, "proj desc entry");
  gtk_widget_set_tooltip_text (proj_desc_entry,
                               _ ("Edit the project description"));
  gtk_widget_show (proj_desc_entry);

  gtk_box_pack_start (GTK_BOX (hbox3), proj_desc_entry, TRUE, TRUE, 4);

  gtk_widget_show (hbox3);

  gtk_box_pack_start (GTK_BOX (vbox1), hbox3, FALSE, FALSE, 2);

  GtkWidget *const scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow1),
                                       GTK_SHADOW_IN);
  gtk_widget_set_can_focus (scrolledwindow1, TRUE);
  gtk_widget_set_name (scrolledwindow1, "scrolledwindow1");

  GtkWidget *const proj_notes_textview = gtk_text_view_new ();
  dlg->proj_notes = connect_text (proj_notes_textview,
                                  G_CALLBACK (proj_notes_changed), dlg);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (proj_notes_textview),
                               GTK_WRAP_WORD);
  gtk_widget_set_can_focus (proj_notes_textview, TRUE);
  gtk_widget_set_name (proj_notes_textview, "proj notes textview");
  gtk_widget_show (proj_notes_textview);

  gtk_container_add (GTK_CONTAINER (scrolledwindow1), proj_notes_textview);
  gtk_widget_show (scrolledwindow1);

  gtk_box_pack_start (GTK_BOX (vbox1), scrolledwindow1, TRUE, TRUE, 0);

  gtk_widget_show (vbox1);

  gtk_paned_pack1 (GTK_PANED (leftright_hpane), vbox1, FALSE, TRUE);

  GtkWidget *const vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox2, "vbox2");

  GtkWidget *const hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox2, "hbox2");

  GtkWidget *const label2 = gtk_label_new (_ ("Diary Entry:"));
  gtk_widget_set_name (label2, "label2");
  gtk_widget_show (label2);

  gtk_box_pack_start (GTK_BOX (hbox2), label2, FALSE, FALSE, 0);

  GtkWidget *const diary_entry_combo = gtk_combo_box_new ();
  dlg->task_combo = GTK_COMBO_BOX (diary_entry_combo);
  gtk_widget_set_name (diary_entry_combo, "diary_entry_combo");
  g_signal_connect (G_OBJECT (diary_entry_combo), "changed",
                    G_CALLBACK (task_selected_cb), dlg);
  gtk_widget_show (diary_entry_combo);

  gtk_box_pack_start (GTK_BOX (hbox2), diary_entry_combo, TRUE, TRUE, 0);

  GtkWidget *const edit_diary_entry_button
      = gtk_button_new_from_stock (GTK_STOCK_EDIT);
  dlg->edit_task = GTK_BUTTON (edit_diary_entry_button);
  gtk_widget_set_can_focus (edit_diary_entry_button, TRUE);
  gtk_widget_set_events (edit_diary_entry_button,
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
                             | GDK_POINTER_MOTION_HINT_MASK
                             | GDK_POINTER_MOTION_MASK);
  gtk_widget_set_name (edit_diary_entry_button, "edit_diary_entry_button");
  gtk_widget_set_receives_default (edit_diary_entry_button, TRUE);
  g_signal_connect (G_OBJECT (edit_diary_entry_button), "clicked",
                    G_CALLBACK (edit_task_cb), dlg);
  gtk_widget_show (edit_diary_entry_button);

  gtk_box_pack_start (GTK_BOX (hbox2), edit_diary_entry_button, FALSE, FALSE,
                      0);

  GtkWidget *const new_diary_entry_button
      = gtk_button_new_from_stock (GTK_STOCK_NEW);
  dlg->new_task = GTK_BUTTON (new_diary_entry_button);
  gtk_button_set_use_underline (GTK_BUTTON (new_diary_entry_button), TRUE);
  gtk_widget_set_can_focus (new_diary_entry_button, TRUE);
  gtk_widget_set_name (new_diary_entry_button, "new_diary_entry_button");
  gtk_widget_set_receives_default (new_diary_entry_button, FALSE);
  gtk_widget_set_tooltip_text (new_diary_entry_button,
                               _ ("Create a new diary entry"));
  g_signal_connect (G_OBJECT (new_diary_entry_button), "clicked",
                    G_CALLBACK (new_task_cb), dlg);
  gtk_widget_show (new_diary_entry_button);

  gtk_box_pack_start (GTK_BOX (hbox2), new_diary_entry_button, FALSE, FALSE,
                      0);

  GtkWidget *const close_diary_button = gtk_button_new ();
  dlg->close_task = GTK_BUTTON (close_diary_button);
  gtk_button_set_relief (GTK_BUTTON (close_diary_button), GTK_RELIEF_NONE);
  gtk_widget_set_can_focus (close_diary_button, TRUE);
  gtk_widget_set_name (close_diary_button, "close diary button");
  gtk_widget_set_receives_default (close_diary_button, FALSE);

  GtkWidget *const image2
      = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_name (image2, "image2");
  gtk_widget_show (image2);

  gtk_button_set_image (GTK_BUTTON (close_diary_button), image2);
  g_signal_connect (G_OBJECT (close_diary_button), "clicked",
                    G_CALLBACK (close_task_area), dlg);
  gtk_widget_show (close_diary_button);

  gtk_box_pack_start (GTK_BOX (hbox2), close_diary_button, FALSE, FALSE, 0);

  gtk_widget_show (hbox2);

  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, TRUE, 0);

  GtkWidget *const scrolledwindow2 = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow2),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow2),
                                       GTK_SHADOW_IN);
  gtk_widget_set_can_focus (scrolledwindow2, TRUE);
  gtk_widget_set_name (scrolledwindow2, "scrolledwindow2");

  GtkWidget *const diary_notes_textview = gtk_text_view_new ();
  dlg->task_notes = connect_text (diary_notes_textview,
                                  G_CALLBACK (task_notes_changed), dlg);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (diary_notes_textview),
                               GTK_WRAP_WORD);
  gtk_widget_set_can_focus (diary_notes_textview, TRUE);
  gtk_widget_set_name (diary_notes_textview, "diary notes textview");
  gtk_widget_show (diary_notes_textview);

  gtk_container_add (GTK_CONTAINER (scrolledwindow2), diary_notes_textview);
  gtk_widget_show (scrolledwindow2);

  gtk_box_pack_start (GTK_BOX (vbox2), scrolledwindow2, TRUE, TRUE, 0);

  gtk_widget_show (vbox2);

  gtk_paned_pack2 (GTK_PANED (leftright_hpane), vbox2, TRUE, TRUE);

  gtk_widget_show (leftright_hpane);

  gtk_paned_pack2 (GTK_PANED (notes_vpane), leftright_hpane, TRUE, TRUE);

  gtk_widget_show (notes_vpane);

  gtk_container_add (GTK_CONTAINER (top_window), notes_vpane);

  gtk_combo_box_set_model (dlg->task_combo, NULL);
  GtkCellRenderer *cell;
  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (dlg->task_combo), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (dlg->task_combo), cell,
                                  "text", 0, NULL);
  g_object_set (cell, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

  gtk_widget_show (GTK_WIDGET (dlg->vpane));

  dlg->proj = NULL;
  dlg->ignore_events = FALSE;
  dlg->proj_freeze = FALSE;
  dlg->task_freeze = NULL;

  return dlg;
}

static void
notes_area_choose_task (NotesArea *na, GttTask *task)
{

  GtkTreeIter iter;
  GtkTreeModel *model = gtk_combo_box_get_model (na->task_combo);

  if (gtk_tree_model_get_iter_first (model, &iter))
    {
      do
        {
          GttTask *t = tasks_model_get_task (model, &iter);

          if (task == t)
            {
              gtk_combo_box_set_active_iter (na->task_combo, &iter);
              break;
            }
        }
      while (gtk_tree_model_iter_next (model, &iter));
    }
  else
    {
      g_warning ("Trying to select task with empty tree model\n");
    }
}

/* ============================================================== */
void
combo_model_add_task (gpointer task_p, gpointer model_p)
{
  GttTask *task = (GttTask *)task_p;
  GtkListStore *model = GTK_LIST_STORE (model_p);
  GtkTreeIter iter;

  gtk_list_store_append (model, &iter);

  gtk_list_store_set (model, &iter, 0, gtt_task_get_memo (task), 1, task, -1);
}

/* ============================================================== */
static GtkTreeModel *
build_task_combo_model (GttProject *proj)
{
  GtkTreeModel *model
      = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER));

  g_list_foreach (gtt_project_get_tasks (proj), combo_model_add_task, model);
  return model;
}

/* ============================================================== */
/* This routine copies data from the data engine, and pushes it
 * into the GUI.
 */

static void
notes_area_do_set_project (NotesArea *na, GttProject *proj)
{
  const char *str;
  GttTask *tsk;

  if (!na)
    return;
  if (na->ignore_events)
    return;

  /* Calling gtk_entry_set_text makes 'changed' events happen,
   * which causes us to get the entry text, which exposes a gtk
   * bug.  So we work around the bug and save cpu time by ignoring
   * change events during a mass update. */
  na->ignore_events = TRUE;

  /* Note Bene its OK to have the proj be null: this has the
   * effect of clearing all the fields out.
   */
  na->proj = proj;

  /* Fetch data from the data engine, stuff it into the GUI. */
  str = gtt_project_get_title (proj);
  if (!str)
    str = "";
  gtk_entry_set_text (na->proj_title, str);

  str = gtt_project_get_desc (proj);
  if (!str)
    str = "";
  gtk_entry_set_text (na->proj_desc, str);

  str = gtt_project_get_notes (proj);
  if (!str)
    str = "";
  xxxgtk_textview_set_text (na->proj_notes, str);

  GtkTreeModel *model = build_task_combo_model (proj);
  gtk_combo_box_set_model (na->task_combo, model);
  g_object_unref (model);

  tsk = gtt_project_get_current_task (proj);
  if (tsk == NULL)
    tsk = gtt_project_get_first_task (proj);

  notes_area_choose_task (na, tsk);

  na->ignore_events = FALSE;
}

/* ============================================================== */

static void
redraw (GttProject *prj, gpointer data)
{
  NotesArea *na = data;
  if (na->ignore_events)
    return;
  notes_area_do_set_project (na, prj);
}

/* ============================================================== */

void
notes_area_set_project (NotesArea *na, GttProject *proj)
{
  if (na->proj != NULL)
    {
      gtt_project_remove_notifier (na->proj, redraw, na);
      na->proj = NULL;
    }
  if (proj != NULL)
    {
      gtt_project_add_notifier (proj, redraw, na);
    }

  notes_area_do_set_project (na, proj);
}

/* ============================================================== */

GtkWidget *
notes_area_get_widget (NotesArea *nadlg)
{
  if (!nadlg)
    return NULL;
  return GTK_WIDGET (nadlg->vpane);
}

static void
projects_tree_selection_changed (GtkTreeSelection *selection,
                                 gpointer user_data)
{
  NotesArea *nadlg = (NotesArea *)user_data;
  GttProjectsTree *gpt
      = GTT_PROJECTS_TREE (gtk_tree_selection_get_tree_view (selection));
  GttProject *prj = gtt_projects_tree_get_selected_project (gpt);

  notes_area_set_project (nadlg, prj);
}

static int
projects_tree_clicked (GtkWidget *ptree, GdkEvent *event, gpointer data)
{
  GdkEventButton *bevent = (GdkEventButton *)event;
  // GttProjectsTree *projects_tree = GTT_PROJECTS_TREE (ptree);
  GtkMenuShell *menu;

  if (!(event->type == GDK_BUTTON_PRESS && bevent->button == 3))
    {
      return FALSE;
    }

  menu = menus_get_popup ();
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, bevent->time);

  return FALSE;
}

void
notes_area_add_projects_tree (NotesArea *nadlg, GttProjectsTree *ptree)
{
  if (!nadlg)
    return;

  GtkTreeSelection *tree_selection
      = gtk_tree_view_get_selection (GTK_TREE_VIEW (ptree));
  g_signal_connect (tree_selection, "changed",
                    G_CALLBACK (projects_tree_selection_changed), nadlg);

  gtk_container_add (nadlg->projects_tree_holder, GTK_WIDGET (ptree));
  gtk_widget_show_all (GTK_WIDGET (nadlg->projects_tree_holder));

  g_signal_connect (GTK_WIDGET (ptree), "button_press_event",
                    G_CALLBACK (projects_tree_clicked), NULL);
}

void
notes_area_get_pane_sizes (NotesArea *na, int *vp, int *hp)
{
  if (!na)
    return;
  if (vp)
    *vp = gtk_paned_get_position (na->vpane);
  if (hp)
    *hp = gtk_paned_get_position (na->hpane);
}

void
notes_area_set_pane_sizes (NotesArea *na, int vp, int hp)
{
  if (!na)
    return;
  gtk_paned_set_position (na->vpane, vp);
  gtk_paned_set_position (na->hpane, hp);
}

#ifdef GTT_TODO_COLUMN_MENU
static GtkWidget *
build_column_menu (void)
{
  GtkWidget *const column_menu = gtk_menu_new ();

  GtkWidget *const sort_up = gtk_image_menu_item_new ();
  gtk_menu_item_set_label (GTK_MENU_ITEM (sort_up), _ ("Sort"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (sort_up), TRUE);
  gtk_widget_set_name (sort_up, "sort_up");
  gtk_widget_set_tooltip_text (
      sort_up, _ ("Sort the entries in this column in alphabetical order."));

  GtkWidget *const sort_up_label = gtk_image_new_from_stock (
      GTK_STOCK_SORT_ASCENDING, GTK_ICON_SIZE_MENU);
  gtk_misc_set_alignment (GTK_MISC (sort_up_label), 0.5, 0.5);
  gtk_misc_set_padding (GTK_MISC (sort_up_label), 0, 0);
  gtk_widget_set_name (sort_up_label, "image7");
  gtk_widget_show (sort_up_label);

  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (sort_up), sort_up_label);
  gtk_widget_show (sort_up);

  gtk_menu_append (GTK_MENU_SHELL (column_menu), sort_up);

  GtkWidget *const move_left = gtk_image_menu_item_new ();
  gtk_menu_item_set_label (GTK_MENU_ITEM (move_left), _ ("Move Left"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (move_left), TRUE);
  gtk_widget_set_name (move_left, "move_left");
  gtk_widget_set_tooltip_text (move_left, _ ("Move this column to the left."));

  GtkWidget *const move_left_label
      = gtk_image_new_from_stock (GTK_STOCK_GO_BACK, GTK_ICON_SIZE_MENU);
  gtk_misc_set_alignment (GTK_MISC (move_left_label), 0.5, 0.5);
  gtk_misc_set_padding (GTK_MISC (move_left_label), 0, 0);
  gtk_widget_set_name (move_left_label, "image8");
  gtk_widget_show (move_left_label);

  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (move_left),
                                 move_left_label);
  gtk_widget_show (move_left);

  gtk_menu_append (GTK_MENU_SHELL (column_menu), move_left);

  GtkWidget *const move_right = gtk_image_menu_item_new ();
  gtk_menu_item_set_label (GTK_MENU_ITEM (move_right), _ ("Move Right"));
  gtk_menu_item_set_use_underline (GTK_MENU_ITEM (move_right), TRUE);
  gtk_widget_set_name (move_right, "move_right");
  gtk_widget_set_tooltip_text (move_right,
                               _ ("Move this column to the right."));

  GtkWidget *const move_right_label
      = gtk_image_new_from_stock (GTK_STOCK_MEDIA_FORWARD, GTK_ICON_SIZE_MENU);
  gtk_misc_set_alignment (GTK_MISC (move_right_label), 0.5, 0.5);
  gtk_misc_set_padding (GTK_MISC (move_right_label), 0, 0);
  gtk_widget_set_name (move_right_label, "image9");
  gtk_widget_show (move_right_label);

  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (move_right),
                                 move_right_label);
  gtk_widget_show (move_right);

  gtk_menu_append (GTK_MENU_SHELL (column_menu), move_right);

  return column_menu;
}
#endif // GTT_TODO_COLUMN_MENU

/* ========================= END OF FILE ======================== */
