/*********************************************************************
 *
 * Copyright (C) 2007, 2009,  Goedson Teixeira Paixao
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Filename:      status-icon.c
 * Author:        Goedson Teixeira Paixao <goedson@debian.org>
 * Description:   GnoTime's status icon implementation
 *
 * Created at:    Fri Oct 12 11:45:39 2007
 * Modified at:   Fri Nov 20 22:01:59 2009
 * Modified by:   Goedson Teixeira Paixao <goedson@debian.org>
 ********************************************************************/

#include "gtt_status_icon.h"
#include "gtt_timer.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

extern GtkWidget *app_window; /* global top-level window */

static GtkStatusIcon *status_icon;
static gboolean timer_active;

static void
status_icon_activated (GtkStatusIcon *status_icon, gpointer data)
{
  if (timer_active)
    {
      gen_stop_timer ();
    }
  else
    {
      gen_start_timer ();
    }
}

static void
status_icon_menuitem_visibility (GtkWidget *toggle, gpointer *user_data)
{
  if (GTK_WIDGET_VISIBLE (app_window))
    gtk_widget_hide (app_window);
  else
    gtk_widget_show (app_window);
}

static void
status_icon_popup_menu (GtkStatusIcon *status_icon, guint button,
                        guint activate_time, gpointer user_data)
{
  GtkWidget *menu = gtk_menu_new ();
  GtkWidget *menuitem
      = gtk_check_menu_item_new_with_mnemonic (_ ("_Hide main window"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
                                  !GTK_WIDGET_VISIBLE (app_window));
  g_signal_connect (G_OBJECT (menuitem), "toggled",
                    G_CALLBACK (status_icon_menuitem_visibility), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show_all (menu);
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, button,
                  activate_time);
}

void
gtt_status_icon_create ()
{
  status_icon = gtk_status_icon_new_from_stock (GTK_STOCK_MEDIA_STOP);
  gtk_status_icon_set_tooltip (status_icon, _ ("Timer is not running"));
  g_signal_connect (G_OBJECT (status_icon), "activate",
                    G_CALLBACK (status_icon_activated), NULL);
  g_signal_connect (G_OBJECT (status_icon), "popup-menu",
                    G_CALLBACK (status_icon_popup_menu), NULL);
}

void
gtt_status_icon_destroy ()
{
  g_object_unref (G_OBJECT (status_icon));
}

void
gtt_status_icon_start_timer (GttProject *prj)
{
  gtk_status_icon_set_from_stock (status_icon, GTK_STOCK_MEDIA_RECORD);
  gchar *text = g_strdup_printf (_ ("Timer running for %s"),
                                 gtt_project_get_title (prj));
  gtk_status_icon_set_tooltip (status_icon, text);
  g_free (text);
  timer_active = TRUE;
}

void
gtt_status_icon_stop_timer (GttProject *prj)
{
  gtk_status_icon_set_tooltip (status_icon, _ ("Timer is not running"));
  gtk_status_icon_set_from_stock (status_icon, GTK_STOCK_MEDIA_STOP);
  timer_active = FALSE;
}
