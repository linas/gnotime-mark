/*   Keyboard inactivity timout dialog for GTimeTracker - a time tracker
 *   Copyright (C) 2001,2002,2003 Linas Vepstas <linas@linas.org>
 *   Copyright (C) 2007 Goedson Paixao <goedson@debian.org>
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

#include <gdk/gdkx.h>
#include <glib.h>
#include <gnome.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>

#include <qof.h>

#include "app.h"
#include "cur-proj.h"
#include "dialog.h"
#include "idle-dialog.h"
#include "proj.h"
#include "util.h"

int config_idle_timeout = -1;

struct GttIdleDialog_s
{
  GtkDialog *dlg;
  GtkButton *yes_btn;
  GtkButton *no_btn;
  GtkButton *help_btn;
  GtkLabel *idle_label;
  GtkLabel *credit_label;
  GtkLabel *time_label;
  GtkRange *scale;

  Display *display;
  gboolean xss_extension_supported;
  XScreenSaverInfo *xss_info;
  guint timeout_event_source;

  gboolean visible;

  GttProject *prj;
  time_t last_activity;
  time_t previous_credit;
};

static gboolean idle_timeout_func (gpointer data);

static void
schedule_idle_timeout (gint timeout, GttIdleDialog *idle_dialog)
{
  if (idle_dialog->timeout_event_source != 0)
    {
      g_source_remove (idle_dialog->timeout_event_source);
    }
  if (timeout > 0 && idle_dialog->xss_extension_supported)
    {
      /* If we already have an idle timeout
       * sceduled, cancel it.
       */
      idle_dialog->timeout_event_source
          = g_timeout_add_seconds (timeout, idle_timeout_func, idle_dialog);
    }
}

static gboolean
idle_timeout_func (gpointer data)
{
  GttIdleDialog *idle_dialog = (GttIdleDialog *)data;
  GdkWindow *gdk_window = gtk_widget_get_root_window (app_window);
  XID drawable = gdk_x11_drawable_get_xid (GDK_DRAWABLE (gdk_window));
  Status xss_query_ok = XScreenSaverQueryInfo (idle_dialog->display, drawable,
                                               idle_dialog->xss_info);
  if (xss_query_ok)
    {
      int idle_seconds = idle_dialog->xss_info->idle / 1000;
      if (cur_proj != NULL && config_idle_timeout > 0
          && idle_seconds >= config_idle_timeout)
        {
          time_t now = time (0);
          idle_dialog->last_activity = now - idle_seconds;
          show_idle_dialog (idle_dialog);
          /* schedule a new timeout for one minute ahead to
             update the dialog. */
          schedule_idle_timeout (60, idle_dialog);
        }
      else
        {
          if (cur_proj != NULL)
            {
              schedule_idle_timeout (config_idle_timeout - idle_seconds,
                                     idle_dialog);
            }
          else if (idle_dialog->visible)
            {
              raise_idle_dialog (idle_dialog);
              schedule_idle_timeout (60, idle_dialog);
            }
        }
    }
  return FALSE;
}

/* =========================================================== */

static void
help_cb (GObject *obj, GttIdleDialog *dlg)
{
  gtt_help_popup (GTK_WIDGET (dlg->dlg), "idletimer");
}

/* =========================================================== */

static void
dialog_close (GObject *obj, GttIdleDialog *dlg)
{
  dlg->dlg = NULL;
  dlg->visible = FALSE;
}

/* =========================================================== */

static void
dialog_kill (GObject *obj, GttIdleDialog *dlg)
{
  gtk_widget_destroy (GTK_WIDGET (dlg->dlg));
  dlg->dlg = NULL;
  dlg->visible = FALSE;
}

/* =========================================================== */

static void
restart_proj (GObject *obj, GttIdleDialog *dlg)
{
  dlg->last_activity = time (0); /* bug fix, sometimes events are lost */
  cur_proj_set (dlg->prj);
  dialog_kill (obj, dlg);
}

/* =========================================================== */

static void
adjust_timer (GttIdleDialog *dlg, time_t adjustment)
{
  GttInterval *ivl;
  time_t stop;

  ivl = gtt_project_get_first_interval (dlg->prj);
  stop = gtt_interval_get_stop (ivl);
  stop -= dlg->previous_credit;
  stop += adjustment;
  gtt_interval_set_stop (ivl, stop);

  dlg->previous_credit = adjustment;
}

/* =========================================================== */
/* Gnome Pango is picky about having bare ampersands in text.
 * Escape the ampersands into proper html.
 * Basically, replace & by &amp; unless its already &amp;
 * free the returned string when done.
 */

static char *
util_escape_html_markup (const char *str)
{
  char *p;
  char *ret;

  if (str == NULL)
    return g_strdup ("");

  p = strchr (str, '&');
  if (!p)
    return g_strdup (str);

  /* count number of ampersands */
  int ampcnt = 0;
  while (p)
    {
      ampcnt++;
      p = strchr (p + 1, '&');
    }

  /* make room for the escapes */
  int len = strlen (str);
  ret = g_new0 (char, len + 4 * ampcnt + 1);

  /* replace & by &amp; unless its already &amp; */
  p = strchr (str, '&');
  const char *start = str;
  while (p)
    {
      strncat (ret, start, p - start);
      if (strncmp (p, "&amp;", 5))
        {
          strcat (ret, "&amp;");
        }
      else
        {
          strcat (ret, "&");
        }
      start = p + 1;
      p = strchr (start, '&');
    }
  strcat (ret, start);
  return ret;
}

/* =========================================================== */

static void
display_value (GttIdleDialog *dlg, time_t credit)
{
  char tbuff[30];
  char mbuff[130];
  char *msg;
  time_t now = time (0);
  time_t idle_time;

  /* Set a value for the thingy under the slider */
  if (3600 > credit)
    {
      xxxqof_print_minutes_elapsed_buff (tbuff, 30, credit, TRUE);
      g_snprintf (mbuff, 130, _ ("%s minutes"), tbuff);
    }
  else
    {
      xxxqof_print_hours_elapsed_buff (tbuff, 30, credit, FALSE);
      g_snprintf (mbuff, 130, _ ("%s hours"), tbuff);
    }
  gtk_label_set_text (dlg->time_label, mbuff);

  /* Set a value in the main message; show hours,
   * or minutes, as is more appropriate.
   */
  if (3600 > credit)
    {
      msg = g_strdup_printf (
          _ ("The timer will be credited "
             "with %ld minutes since the last keyboard/mouse "
             "activity.  If you want to change the amount "
             "of time credited, use the slider below to "
             "adjust the value."),
          (credit + 30) / 60);
    }
  else
    {
      msg = g_strdup_printf (_ ("The timer will be credited "
                                "with %s hours since the last keyboard/mouse "
                                "activity.  If you want to change the amount "
                                "of time credited, use the slider below to "
                                "adjust the value."),
                             tbuff);
    }
  gtk_label_set_text (dlg->credit_label, msg);
  g_free (msg);

  /* Update the total elapsed time part of the message */
  idle_time = now - dlg->last_activity;

  char *ptitle = util_escape_html_markup (gtt_project_get_title (dlg->prj));
  char *pdesc = util_escape_html_markup (gtt_project_get_desc (dlg->prj));
  if (3600 > idle_time)
    {
      msg = g_strdup_printf (_ ("The keyboard and mouse have been idle "
                                "for %ld minutes.  The currently running "
                                "project, <b><i>%s - %s</i></b>, "
                                "has been stopped. "
                                "Do you want to restart it?"),
                             (idle_time + 30) / 60, ptitle, pdesc);
    }
  else
    {
      msg = g_strdup_printf (_ ("The keyboard and mouse have been idle "
                                "for %ld:%02ld hours.  The currently running "
                                "project (%s - %s) "
                                "has been stopped. "
                                "Do you want to restart it?"),
                             (idle_time + 30) / 3600,
                             ((idle_time + 30) / 60) % 60, ptitle, pdesc);
    }

  gtk_label_set_markup (dlg->idle_label, msg);
  g_free (msg);
  g_free (ptitle);
  g_free (pdesc);
}

/* =========================================================== */

static void
value_changed (GObject *obj, GttIdleDialog *dlg)
{
  double slider_value;
  time_t credit;
  time_t now = time (0);

  slider_value = gtk_range_get_value (dlg->scale);
  slider_value /= 90.0;
  slider_value *= (now - dlg->last_activity);

  credit = (time_t)slider_value;

  display_value (dlg, credit); /* display value in GUI */
  adjust_timer (dlg, credit);  /* change value in data store */
}

/* =========================================================== */
/* XXX the new GtkDialog is broken; it can't hide-on-close,
 * unlike to old, deprecated GnomeDialog.  Thus, we have to
 * do a heavyweight re-initialization each time.  Urgh.
 */

static void
idle_dialog_realize (GttIdleDialog *id)
{
  id->prj = NULL;

  GtkWidget *const idle_dialog = gtk_dialog_new ();
  id->dlg = GTK_DIALOG (idle_dialog);
  gtk_dialog_set_has_separator (GTK_DIALOG (idle_dialog), TRUE);
  gtk_widget_set_name (idle_dialog, "Idle Dialog");
  gtk_window_set_destroy_with_parent (GTK_WINDOW (idle_dialog), FALSE);
  gtk_window_set_modal (GTK_WINDOW (idle_dialog), TRUE);
  gtk_window_set_position (GTK_WINDOW (idle_dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_resizable (GTK_WINDOW (idle_dialog), TRUE);
  gtk_window_set_title (GTK_WINDOW (idle_dialog),
                        _ ("Restart Idle Timer Dialog"));
  g_signal_connect (G_OBJECT (idle_dialog), "destroy",
                    G_CALLBACK (dialog_close), id);

  GtkWidget *const action_area
      = gtk_dialog_get_action_area (GTK_DIALOG (idle_dialog));
  gtk_button_box_set_layout (GTK_BUTTON_BOX (action_area), GTK_BUTTONBOX_END);

  GtkWidget *const helpbutton1 = gtk_button_new_from_stock (GTK_STOCK_HELP);
  id->help_btn = GTK_BUTTON (helpbutton1);
  gtk_button_set_relief (GTK_BUTTON (helpbutton1), GTK_RELIEF_NORMAL);
  gtk_widget_set_can_default (helpbutton1, TRUE);
  gtk_widget_set_can_focus (helpbutton1, TRUE);
  gtk_widget_set_name (helpbutton1, "helpbutton1");
  g_signal_connect (G_OBJECT (helpbutton1), "clicked", G_CALLBACK (help_cb),
                    id);
  gtk_widget_show (helpbutton1);

  gtk_box_pack_start_defaults (GTK_BOX (action_area), helpbutton1);

  GtkWidget *const yes_button = gtk_button_new_from_stock (GTK_STOCK_YES);
  id->yes_btn = GTK_BUTTON (yes_button);
  gtk_button_set_relief (GTK_BUTTON (yes_button), GTK_RELIEF_NORMAL);
  gtk_widget_set_can_default (yes_button, TRUE);
  gtk_widget_set_can_focus (yes_button, TRUE);
  gtk_widget_set_name (yes_button, "yes button");
  gtk_widget_set_tooltip_text (
      yes_button, _ ("Restart the same project that used to be running."));
  g_signal_connect (G_OBJECT (yes_button), "clicked",
                    G_CALLBACK (restart_proj), id);
  gtk_widget_show (yes_button);

  gtk_box_pack_start_defaults (GTK_BOX (action_area), yes_button);

  GtkWidget *const no_button = gtk_button_new_from_stock (GTK_STOCK_NO);
  id->no_btn = GTK_BUTTON (no_button);
  gtk_button_set_relief (GTK_BUTTON (no_button), GTK_RELIEF_NORMAL);
  gtk_widget_set_can_default (no_button, TRUE);
  gtk_widget_set_can_focus (no_button, TRUE);
  gtk_widget_set_name (no_button, "no button");
  gtk_widget_set_tooltip_text (no_button, _ ("Do not restart the timer."));
  g_signal_connect (G_OBJECT (no_button), "clicked", G_CALLBACK (dialog_kill),
                    id);
  gtk_widget_show (no_button);

  gtk_box_pack_start_defaults (GTK_BOX (action_area), no_button);

  gtk_widget_show (action_area);

  GtkWidget *const content_area
      = gtk_dialog_get_content_area (GTK_DIALOG (idle_dialog));
  gtk_box_set_homogeneous (GTK_BOX (content_area), FALSE);
  gtk_box_set_spacing (GTK_BOX (content_area), 0);

  GtkWidget *const hbox4 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox4, "hbox4");

  GtkWidget *const image2 = gtk_image_new_from_stock (
      GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
  gtk_misc_set_alignment (GTK_MISC (image2), 0.5, 0.5);
  gtk_misc_set_padding (GTK_MISC (image2), 0, 0);
  gtk_widget_set_name (image2, "image2");
  gtk_widget_show (image2);

  gtk_box_pack_start (GTK_BOX (hbox4), image2, TRUE, TRUE, 0);

  GtkWidget *const vbox4 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox4, "vbox4");

  GtkWidget *const scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_placement (GTK_SCROLLED_WINDOW (scrolledwindow1),
                                     GTK_CORNER_TOP_LEFT);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
                                  GTK_POLICY_NEVER, GTK_POLICY_NEVER);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow1),
                                       GTK_SHADOW_NONE);
  gtk_widget_set_name (scrolledwindow1, "scrolledwindow1");

  GtkWidget *const viewport1
      = gtk_viewport_new (gtk_scrolled_window_get_hadjustment (
                              GTK_SCROLLED_WINDOW (scrolledwindow1)),
                          gtk_scrolled_window_get_vadjustment (
                              GTK_SCROLLED_WINDOW (scrolledwindow1)));
  gtk_container_set_border_width (GTK_CONTAINER (viewport1), 1);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport1), GTK_SHADOW_NONE);
  gtk_widget_set_name (viewport1, "viewport1");

  GtkWidget *const idle_label = gtk_label_new ("Dummy Text Do Not Translate");
  id->idle_label = GTK_LABEL (idle_label);
  gtk_label_set_justify (GTK_LABEL (idle_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (idle_label), TRUE);
  gtk_label_set_selectable (GTK_LABEL (idle_label), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (idle_label), TRUE);
  gtk_label_set_use_underline (GTK_LABEL (idle_label), FALSE);
  gtk_misc_set_alignment (GTK_MISC (idle_label), 0.5, 0.5);
  gtk_misc_set_padding (GTK_MISC (idle_label), 0, 0);
  gtk_widget_set_name (idle_label, "idle label");
  gtk_widget_show (idle_label);

  gtk_container_add (GTK_CONTAINER (viewport1), idle_label);
  gtk_widget_show (viewport1);

  gtk_container_add (GTK_CONTAINER (scrolledwindow1), viewport1);
  gtk_widget_show (scrolledwindow1);

  gtk_box_pack_start (GTK_BOX (vbox4), scrolledwindow1, TRUE, FALSE, 5);

  GtkWidget *const hseparator1 = gtk_hseparator_new ();
  gtk_widget_set_name (hseparator1, "hseparator1");
  gtk_widget_show (hseparator1);

  gtk_box_pack_start (GTK_BOX (vbox4), hseparator1, FALSE, TRUE, 0);

  GtkWidget *const scrolledwindow2 = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_placement (GTK_SCROLLED_WINDOW (scrolledwindow2),
                                     GTK_CORNER_TOP_LEFT);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow2),
                                  GTK_POLICY_NEVER, GTK_POLICY_NEVER);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow2),
                                       GTK_SHADOW_NONE);
  gtk_widget_set_name (scrolledwindow2, "scrolledwindow2");

  GtkWidget *const viewport2
      = gtk_viewport_new (gtk_scrolled_window_get_hadjustment (
                              GTK_SCROLLED_WINDOW (scrolledwindow2)),
                          gtk_scrolled_window_get_vadjustment (
                              GTK_SCROLLED_WINDOW (scrolledwindow2)));
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport2), GTK_SHADOW_NONE);
  gtk_widget_set_name (viewport2, "viewport2");

  GtkWidget *const credit_label
      = gtk_label_new ("Dummy Text Do Not Translate");
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

  gtk_container_add (GTK_CONTAINER (viewport2), credit_label);
  gtk_widget_show (viewport2);

  gtk_container_add (GTK_CONTAINER (scrolledwindow2), viewport2);
  gtk_widget_show (scrolledwindow2);

  gtk_box_pack_start (GTK_BOX (vbox4), scrolledwindow2, TRUE, FALSE, 5);

  GtkWidget *const scale = gtk_hscale_new (
      GTK_ADJUSTMENT (gtk_adjustment_new (33.9914, 0, 100, 1, 5, 10)));
  id->scale = GTK_RANGE (scale);
  gtk_range_set_inverted (GTK_RANGE (scale), FALSE);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_CONTINUOUS);
  gtk_scale_set_digits (GTK_SCALE (scale), 4);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_widget_set_can_focus (scale, TRUE);
  gtk_widget_set_name (scale, "scale");
  g_signal_connect (G_OBJECT (scale), "value_changed",
                    G_CALLBACK (value_changed), id);
  gtk_widget_show (scale);

  gtk_box_pack_start (GTK_BOX (vbox4), scale, FALSE, FALSE, 3);

  GtkWidget *const time_label = gtk_label_new ("hh:mm");
  id->time_label = GTK_LABEL (time_label);
  gtk_label_set_justify (GTK_LABEL (time_label), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (time_label), FALSE);
  gtk_label_set_selectable (GTK_LABEL (time_label), FALSE);
  gtk_label_set_use_markup (GTK_LABEL (time_label), FALSE);
  gtk_label_set_use_underline (GTK_LABEL (time_label), FALSE);
  gtk_misc_set_alignment (GTK_MISC (time_label), 0.5, 0.5);
  gtk_misc_set_padding (GTK_MISC (time_label), 0, 0);
  gtk_widget_set_name (time_label, "time label");
  gtk_widget_show (time_label);

  gtk_box_pack_start (GTK_BOX (vbox4), time_label, FALSE, FALSE, 6);

  gtk_widget_show (vbox4);

  gtk_box_pack_start (GTK_BOX (hbox4), vbox4, TRUE, TRUE, 0);

  gtk_widget_show (hbox4);

  gtk_box_pack_start (GTK_BOX (content_area), hbox4, TRUE, TRUE, 0);

  gtk_widget_show (content_area);
}

/* =========================================================== */

GttIdleDialog *
idle_dialog_new (void)
{
  GttIdleDialog *id;

  id = g_new0 (GttIdleDialog, 1);
  id->prj = NULL;

  gchar *display_name = gdk_get_display ();
  id->display = XOpenDisplay (display_name);
  if (id->display == NULL)
    {
      g_warning ("Could not open display %s", display_name);
    }
  else
    {
      int xss_events, xss_error;
      id->xss_extension_supported
          = XScreenSaverQueryExtension (id->display, &xss_events, &xss_error);
      if (id->xss_extension_supported)
        {
          id->xss_info = XScreenSaverAllocInfo ();
          if (config_idle_timeout > 0)
            {
              schedule_idle_timeout (config_idle_timeout, id);
            }
        }
      else
        {
          g_warning (
              _ ("The XScreenSaver is not supported on this display.\n"
                 "The idle timeout functionality will not be available."));
        }
    }
  g_free (display_name);

  return id;
}

/* =========================================================== */

void
show_idle_dialog (GttIdleDialog *id)
{
  time_t now;
  time_t idle_time;
  GttProject *prj = cur_proj;

  if (!id)
    return;
  if (0 > config_idle_timeout)
    return;
  if (!prj)
    return;

  now = time (0);
  idle_time = now - id->last_activity;

  /* Due to GtkDialog broken-ness, re-realize the GUI */
  if (NULL == id->dlg)
    {
      idle_dialog_realize (id);
    }

  /* Mark the idle dialog as visible so we don't start the no
   * project timeout timer when it's not needed.
   */
  id->visible = TRUE;

  /* Stop the timer on the current project */
  cur_proj_set (NULL);

  id->prj = prj;

  /* The idle timer can trip because gtt was left running
   * on a laptop, which was them put in suspend mode (i.e.
   * by closing the cover).  When the laptop is resumed,
   * the poll_last_activity will return the many hours/days
   * that the laptop has been shut down, and merely stoping
   * the timer (as above) will credit hours/days to the
   * current active project.  We don't want this, we need
   * to undo this damage.
   */
  id->previous_credit = idle_time;
  adjust_timer (id, config_idle_timeout);

  raise_idle_dialog (id);
}

/* =========================================================== */

void
raise_idle_dialog (GttIdleDialog *id)
{
  g_return_if_fail (id);
  g_return_if_fail (id->dlg);

  /* Now, draw the messages in the GUI popup. */
  display_value (id, config_idle_timeout);

  /* The following will raise the window, and put it on the current
   * workspace, at least if the metacity WM is used. Haven't tried
   * other window managers.
   */

  gtk_window_present (GTK_WINDOW (id->dlg));
  id->visible = TRUE;
}

void
idle_dialog_activate_timer (GttIdleDialog *idle_dialog)
{
  schedule_idle_timeout (config_idle_timeout, idle_dialog);
}

void
idle_dialog_deactivate_timer (GttIdleDialog *idle_dialog)
{
  if (idle_dialog->timeout_event_source != 0)
    {
      g_source_remove (idle_dialog->timeout_event_source);
      idle_dialog->timeout_event_source = 0;
    }
}

gboolean
idle_dialog_is_visible (GttIdleDialog *idle_dialog)
{
  return idle_dialog->visible;
}

/* =========================== END OF FILE ============================== */
