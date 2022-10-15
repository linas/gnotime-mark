/*   Keyboard inactivity timout dialog for GTimeTracker - a time tracker
 *   Copyright (C) 2001,2002,2003 Linas Vepstas <linas@linas.org>
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

#include <gnome.h>
#include <string.h>

#include <qof.h>

#include "active-dialog.h"
#include "cur-proj.h"
#include "dialog.h"
#include "proj-query.h"
#include "proj.h"
#include "util.h"

int config_no_project_timeout;

struct GttActiveDialog_s
{
  GtkDialog *dlg;
  GtkButton *yes_btn;
  GtkButton *no_btn;
  GtkButton *help_btn;
  GtkLabel *active_label;
  GtkLabel *credit_label;
  GtkOptionMenu *project_menu;
  guint timeout_event_source;
};

void show_active_dialog (GttActiveDialog *ad);

static gboolean
active_timeout_func (gpointer data)
{
  GttActiveDialog *active_dialog = (GttActiveDialog *)data;
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
      active_dialog->timeout_event_source = g_timeout_add_seconds (
          timeout, active_timeout_func, active_dialog);
    }
}

/* =========================================================== */

static void
dialog_help (GObject *obj, GttActiveDialog *dlg)
{
  gtt_help_popup (GTK_WIDGET (dlg->dlg), "idletimer");
}

/* =========================================================== */

static void
dialog_close (GObject *obj, GttActiveDialog *dlg)
{
  dlg->dlg = NULL;

  if (!cur_proj)
    {
      schedule_active_timeout (config_no_project_timeout, dlg);
    }
}

/* =========================================================== */

static void
dialog_kill (GObject *obj, GttActiveDialog *dlg)
{
  gtk_widget_destroy (GTK_WIDGET (dlg->dlg));
  dlg->dlg = NULL;
  if (!cur_proj)
    {
      schedule_active_timeout (config_no_project_timeout, dlg);
    }
}

/* =========================================================== */

static void
start_proj (GObject *obj, GttActiveDialog *dlg)
{
  GtkMenu *menu;
  GtkWidget *w;
  GttProject *prj;

  /* Start the project that the user has selected from the menu */
  menu = GTK_MENU (gtk_option_menu_get_menu (dlg->project_menu));
  w = gtk_menu_get_active (menu);
  prj = g_object_get_data (G_OBJECT (w), "prj");

  cur_proj_set (prj);
  dialog_kill (obj, dlg);
}

/* =========================================================== */

static void
setup_menus (GttActiveDialog *dlg)
{
  GtkMenuShell *menushell;
  GList *prjlist, *node;
  char *msg;

  msg = _ ("No project timer is currently running in GnoTime.  "
           "Do you want to start a project timer running?  "
           "If so, you can select a project from the menu below, "
           "and click 'Start' to start the project running.  "
           "Otherwise, just click 'Cancel' to do nothing.");

  gtk_label_set_text (dlg->active_label, msg);

  msg = _ ("You can credit this project with the time that you worked "
           "on it but were away from the keyboard.  Enter a time below, "
           "the project will be credited when you click 'Start'");

  gtk_label_set_text (dlg->credit_label, msg);

  menushell = GTK_MENU_SHELL (gtk_menu_new ());

  /* Give user a list only of the unfinished projects,
   * so that there isn't too much clutter ... */
  prjlist = gtt_project_get_unfinished ();
  for (node = prjlist; node; node = node->next)
    {
      GttProject *prj = node->data;
      GtkWidget *item;
      item = gtk_menu_item_new_with_label (gtt_project_get_title (prj));
      g_object_set_data (G_OBJECT (item), "prj", prj);
      gtk_menu_shell_append (menushell, item);
      gtk_widget_show (item);
    }
  gtk_option_menu_set_menu (dlg->project_menu, GTK_WIDGET (menushell));
}

/* =========================================================== */
/* XXX the new GtkDialog is broken; it can't hide-on-close,
 * unlike to old, deprecated GnomeDialog.  Thus, we have to
 * do a heavyweight re-initialization each time.  Urgh.
 */

static void
active_dialog_realize (GttActiveDialog *id)
{
  GtkWidget *active_dialog = gtk_dialog_new ();
  id->dlg = GTK_DIALOG (active_dialog);
  gtk_dialog_set_has_separator (GTK_DIALOG (active_dialog), TRUE);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (active_dialog), FALSE);
  gtk_window_set_modal (GTK_WINDOW (active_dialog), TRUE);
  gtk_widget_set_name (active_dialog, "Active Dialog");
  gtk_window_set_position (GTK_WINDOW (active_dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_resizable (GTK_WINDOW (active_dialog), TRUE);
  gtk_window_set_title (GTK_WINDOW (active_dialog),
                        _ ("Start Project Dialog"));

  GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (id->dlg));

  GtkWidget *hbox4 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox4, "hbox4");

  GtkWidget *image2 = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION,
                                                GTK_ICON_SIZE_DIALOG);
  gtk_misc_set_alignment (GTK_MISC (image2), 0.5, 0.5);
  gtk_misc_set_padding (GTK_MISC (image2), 0, 0);
  gtk_widget_set_name (image2, "image2");
  gtk_widget_show (image2);

  gtk_box_pack_start (GTK_BOX (hbox4), image2, TRUE, TRUE, 0);

  GtkWidget *vbox4 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox4, "vbox4");

  GtkWidget *scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_placement (GTK_SCROLLED_WINDOW (scrolledwindow1),
                                     GTK_CORNER_TOP_LEFT);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
                                  GTK_POLICY_NEVER, GTK_POLICY_NEVER);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow1),
                                       GTK_SHADOW_NONE);
  gtk_widget_set_name (scrolledwindow1, "scrolledwindow1");

  GtkWidget *viewport1
      = gtk_viewport_new (gtk_scrolled_window_get_hadjustment (
                              GTK_SCROLLED_WINDOW (scrolledwindow1)),
                          gtk_scrolled_window_get_vadjustment (
                              GTK_SCROLLED_WINDOW (scrolledwindow1)));
  gtk_container_set_border_width (GTK_CONTAINER (viewport1), 1);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport1), GTK_SHADOW_NONE);
  gtk_widget_set_name (viewport1, "viewport1");

  GtkWidget *active_label = gtk_label_new (_ ("Dummy Text Do Not Translate"));
  id->active_label = GTK_LABEL (active_label);
  gtk_label_set_justify (GTK_LABEL (active_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (active_label), TRUE);
  gtk_label_set_selectable (GTK_LABEL (active_label), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (active_label), TRUE);
  gtk_label_set_use_underline (GTK_LABEL (active_label), FALSE);
  gtk_misc_set_alignment (GTK_MISC (active_label), 0.5, 0.5);
  gtk_misc_set_padding (GTK_MISC (active_label), 0, 0);
  gtk_widget_set_name (active_label, "active label");
  gtk_widget_show (active_label);

  gtk_container_add (GTK_CONTAINER (viewport1), active_label);
  gtk_widget_show (viewport1);

  gtk_container_add (GTK_CONTAINER (scrolledwindow1), viewport1);
  gtk_widget_show (scrolledwindow1);

  gtk_box_pack_start (GTK_BOX (vbox4), scrolledwindow1, TRUE, FALSE, 5);

  GtkWidget *hseparator1 = gtk_hseparator_new ();
  gtk_widget_set_name (hseparator1, "hseparator1");
  gtk_widget_show (hseparator1);

  gtk_box_pack_start (GTK_BOX (vbox4), hseparator1, FALSE, TRUE, 0);

  GtkWidget *project_option_menu = gtk_option_menu_new ();
  id->project_menu = GTK_OPTION_MENU (project_option_menu);
  gtk_widget_set_can_focus (project_option_menu, TRUE);
  gtk_widget_set_name (project_option_menu, "project option menu");

  GtkWidget *project_menu = gtk_menu_new ();
  gtk_widget_set_name (project_menu, "project menu");
  gtk_widget_show (project_menu);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (project_option_menu),
                            project_menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (project_option_menu), -1);
  gtk_widget_show (project_option_menu);

  gtk_box_pack_start (GTK_BOX (vbox4), project_option_menu, FALSE, FALSE, 5);

  GtkWidget *credit_label = gtk_label_new (_ ("Dummy Text Do Not Translate"));
  id->credit_label = GTK_LABEL (credit_label);
  gtk_label_set_justify (GTK_LABEL (credit_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (credit_label), TRUE);
  gtk_label_set_selectable (GTK_LABEL (credit_label), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (credit_label), TRUE);
  gtk_label_set_use_underline (GTK_LABEL (credit_label), FALSE);
  gtk_misc_set_alignment (GTK_MISC (credit_label), 0.5, 0.5);
  gtk_misc_set_padding (GTK_MISC (credit_label), 0, 0);
  gtk_widget_set_name (credit_label, "credit label");
  gtk_widget_show (credit_label);

  gtk_box_pack_start (GTK_BOX (vbox4), credit_label, FALSE, FALSE, 3);

  GtkWidget *time_credit_entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (time_credit_entry), FALSE);
  gtk_entry_set_editable (GTK_ENTRY (time_credit_entry), TRUE);
  gtk_entry_set_has_frame (GTK_ENTRY (time_credit_entry), TRUE);
  gtk_entry_set_invisible_char (GTK_ENTRY (time_credit_entry), '*');
  gtk_entry_set_max_length (GTK_ENTRY (time_credit_entry), 0);
  gtk_entry_set_visibility (GTK_ENTRY (time_credit_entry), TRUE);
  gtk_widget_set_can_focus (time_credit_entry, TRUE);
  gtk_widget_set_name (time_credit_entry, "time credit entry");
  gtk_widget_show (time_credit_entry);

  gtk_box_pack_start (GTK_BOX (vbox4), time_credit_entry, FALSE, FALSE, 6);
  gtk_widget_show (vbox4);

  gtk_box_pack_start (GTK_BOX (hbox4), vbox4, TRUE, TRUE, 0);
  gtk_widget_show (hbox4);

  gtk_box_pack_start_defaults (GTK_BOX (content_area), hbox4);
  gtk_widget_show (content_area);

  GtkWidget *action_area = gtk_dialog_get_action_area (GTK_DIALOG (id->dlg));
  gtk_button_box_set_layout (GTK_BUTTON_BOX (action_area), GTK_BUTTONBOX_END);

  GtkWidget *helpbutton1 = gtk_button_new_from_stock (GTK_STOCK_HELP);
  id->help_btn = GTK_BUTTON (helpbutton1);
  gtk_button_set_relief (GTK_BUTTON (helpbutton1), GTK_RELIEF_NORMAL);
  gtk_widget_set_can_default (helpbutton1, TRUE);
  gtk_widget_set_can_focus (helpbutton1, TRUE);
  gtk_widget_set_name (helpbutton1, "helpbutton1");
  g_signal_connect (G_OBJECT (id->help_btn), "clicked",
                    G_CALLBACK (dialog_help), id);
  gtk_widget_show (helpbutton1);
  gtk_box_pack_start_defaults (GTK_BOX (action_area), helpbutton1);

  GtkWidget *yes_button = gtk_button_new_from_stock (GTK_STOCK_EXECUTE);
  id->yes_btn = GTK_BUTTON (yes_button);
  gtk_button_set_relief (GTK_BUTTON (yes_button), GTK_RELIEF_NORMAL);
  gtk_widget_set_can_default (yes_button, TRUE);
  gtk_widget_set_can_focus (yes_button, TRUE);
  gtk_widget_set_name (yes_button, "yes button");
  gtk_widget_set_tooltip_text (
      yes_button, _ ("Restart the same project that used to be running."));
  g_signal_connect (G_OBJECT (id->yes_btn), "clicked", G_CALLBACK (start_proj),
                    id);
  gtk_widget_show (yes_button);
  gtk_box_pack_start_defaults (GTK_BOX (action_area), yes_button);

  GtkWidget *no_button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  id->no_btn = GTK_BUTTON (no_button);
  gtk_button_set_relief (GTK_BUTTON (no_button), GTK_RELIEF_NORMAL);
  gtk_widget_set_can_default (no_button, TRUE);
  gtk_widget_set_can_focus (no_button, TRUE);
  gtk_widget_set_name (no_button, "no button");
  gtk_widget_set_tooltip_text (no_button, _ ("Do not restart the timer."));
  g_signal_connect (G_OBJECT (id->no_btn), "clicked", G_CALLBACK (dialog_kill),
                    id);
  gtk_widget_show (no_button);
  gtk_box_pack_start_defaults (GTK_BOX (action_area), no_button);

  gtk_widget_show (action_area);

  g_signal_connect (G_OBJECT (id->dlg), "destroy", G_CALLBACK (dialog_close),
                    id);
}

/* =========================================================== */

GttActiveDialog *
active_dialog_new (void)
{
  GttActiveDialog *ad;

  ad = g_new0 (GttActiveDialog, 1);
  ad->dlg = NULL;

  return ad;
}

/* =========================================================== */

void
show_active_dialog (GttActiveDialog *ad)
{
  g_return_if_fail (ad);

  g_return_if_fail (!cur_proj);

  /* Due to GtkDialog broken-ness, re-realize the GUI */
  if (NULL == ad->dlg)
    {
      active_dialog_realize (ad);
      setup_menus (ad);

      gtk_widget_show (GTK_WIDGET (ad->dlg));
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

  g_return_if_fail (ad);
  g_return_if_fail (ad->dlg);

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
