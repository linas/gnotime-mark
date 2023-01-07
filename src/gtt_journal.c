/*   Display & Edit Journal of Timestamps for GnoTime - a time tracker
 *   Copyright (C) 2001,2002,2003,2004 Linas Vepstas <linas@linas.org>
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

#define _GNU_SOURCE

#include "config.h"

#include "gtt_journal.h"

#include <glade/glade.h>
#include <gnome.h>
#include <gtkhtml/gtkhtml.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>

#include <qof.h>

#include "gtt_application_window.h"
#include "gtt_ghtml.h"
#include "gtt_help_popup.h"
#include "gtt_menus.h"
#include "gtt_plug_in.h"
#include "gtt_project.h"
#include "gtt_props_dlg_interval.h"
#include "gtt_props_dlg_task.h"
#include "gtt_util.h"

#include <gio/gio.h>

/* This struct is a mish-mash of stuff relating to the
 * HTML display window, and the various actions and etc.
 * that can be taken from it. */

typedef struct wiggy_s
{
  GttGhtml *gh;
  GtkHTML *html;
  GtkHTMLStream *html_stream;
  GtkWidget *top;
  GttProject *prj;
  char *filepath; /* file containing report template */

  /* Interval edit menu widgets */
  GttInterval *interval;
  GtkWidget *interval_popup;
  GtkWidget *interval_paste;
  GtkWidget *interval_merge_up;
  GtkWidget *interval_merge_down;
  GtkWidget *interval_move_up;
  GtkWidget *interval_move_down;
  EditIntervalDialog *edit_ivl;

  /* Task edit menu widgets */
  GttTask *task;
  GtkWidget *task_popup;
  GtkWidget *task_delete_memo;
  GtkWidget *task_paste;

  /* Mouse fly-over help */
  GtkWidget *hover_help_window;
  GtkLabel *hover_label;
  guint hover_timeout_id;
  guint hover_kill_id;

  GFile *ofile;
  GFileOutputStream *ostream;
  GttPlugin *plg; /* file path save history */

  /* Publish-to-URL dialog */
  GtkWidget *publish_popup;
  GtkEntry *publish_entry;
} Wiggy;

static void do_show_report (const char *, GttPlugin *, KvpFrame *,
                            GttProject *, gboolean, GList *);

/* ============================================================== */
/* Routines that take html and mash it into browser. */

static void
wiggy_open (GttGhtml *pl, gpointer ud)
{
  Wiggy *wig = (Wiggy *)ud;

  /* open the browser for writing */
  wig->html_stream = gtk_html_begin (wig->html);
}

static void
wiggy_close (GttGhtml *pl, gpointer ud)
{
  Wiggy *wig = (Wiggy *)ud;

  /* close the browser stream */
  gtk_html_end (wig->html, wig->html_stream, GTK_HTML_STREAM_OK);
}

static void
wiggy_write (GttGhtml *pl, const char *str, size_t len, gpointer ud)
{
  Wiggy *wig = (Wiggy *)ud;

  /* write to the browser stream */
  gtk_html_write (wig->html, wig->html_stream, str, len);
}

static void
wiggy_error (GttGhtml *pl, int err, const char *msg, gpointer ud)
{
  Wiggy *wig = (Wiggy *)ud;
  GtkHTML *html = wig->html;
  GtkHTMLStream *stream;
  char buff[1000], *p;

  stream = gtk_html_begin (html);

  if (404 == err)
    {
      p = buff;
      p = g_stpcpy (p, "<html><body><h1>");
      p = g_stpcpy (p, _ ("Error 404 Not Found"));
      p = g_stpcpy (p, "</h1>");
      p += sprintf (p, _ ("The file %s was not found."),
                    (msg ? (char *)msg : _ ("(null)")));

      p = g_stpcpy (p, "</body></html>");
      gtk_html_write (html, stream, buff, p - buff);
    }
  else
    {
      p = buff;
      p = g_stpcpy (p, "<html><body><h1>");
      p = g_stpcpy (p, _ ("Unkown Error"));
      p = g_stpcpy (p, "</h1></body></html>");
      gtk_html_write (html, stream, buff, p - buff);
    }

  gtk_html_end (html, stream, GTK_HTML_STREAM_OK);
}

/* ============================================================== */
/* Routine that take html and mash it into a file. */

static void
file_write_helper (GttGhtml *pl, const char *str, size_t len, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;

  gsize buflen = len;
  gsize off = 0;

  while (1)
    {
      GError *error = NULL;
      const gssize bytes_written = g_output_stream_write (
          G_OUTPUT_STREAM (wig->ostream), &str[off], buflen, NULL, &error);
      if (0 > bytes_written)
        {
          g_warning ("Failed to write HTML file: %s", error->message);

          g_error_free (error);
          error = NULL;

          break;
        }
      if (0 == buflen)
        {
          break;
        }

      off += bytes_written;
      buflen -= bytes_written;
    }
}

/* ============================================================== */

static void
remember_uri (Wiggy *wig, const char *filename)
{
  /* Remember history, on a per-report basis */
  if (wig->plg
      && ((NULL == wig->plg->last_url) || (0 == wig->plg->last_url[0])
          || (0 == strncmp (wig->plg->last_url, "file:/", 6))))
    {
      if (wig->plg->last_url)
        g_free (wig->plg->last_url);
      wig->plg->last_url = g_strdup_printf ("file:%s", filename);
    }
}

static void
save_to_gio (Wiggy *wig, const char *filename)
{
  /* Try to open the file for writing */
  wig->ofile = g_file_new_for_path (filename);

  GError *error = NULL;
  wig->ostream = g_file_replace (wig->ofile, NULL, FALSE, G_FILE_CREATE_NONE,
                                 NULL, &error);
  if (NULL == wig->ostream)
    {
      GtkWidget *mb;
      mb = gtk_message_dialog_new (
          GTK_WINDOW (wig->top),
          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
          GTK_BUTTONS_CLOSE, _ ("Unable to open the file %s\n%s"), filename,
          error->message);
      g_signal_connect (G_OBJECT (mb), "response",
                        G_CALLBACK (gtk_widget_destroy), mb);
      gtk_widget_show (mb);

      g_error_free (error);
      error = NULL;

      g_clear_object (&wig->ofile);
    }
  else
    {
      /* Cause ghtml to output the html again, but this time
       * using raw file-io handlers instead. */
      gtt_ghtml_set_stream (wig->gh, wig, NULL, file_write_helper, NULL,
                            wiggy_error);
      gtt_ghtml_show_links (wig->gh, FALSE);
      gtt_ghtml_display (wig->gh, wig->filepath, wig->prj);
      gtt_ghtml_show_links (wig->gh, TRUE);

      if (FALSE
          == g_output_stream_close (G_OUTPUT_STREAM (wig->ostream), NULL,
                                    &error))
        {
          g_warning ("Failed to close HTML file: %s", error->message);

          g_error_free (error);
          error = NULL;
        }

      g_clear_object (&wig->ostream);
      g_clear_object (&wig->ofile);

      /* Reset the html out handlers back to the browser */
      gtt_ghtml_set_stream (wig->gh, wig, wiggy_open, wiggy_write, wiggy_close,
                            wiggy_error);
    }
}

static void
save_to_file (Wiggy *wig, const char *uri)
{

#if BORKEN_STILL_GET_X11_TRAFFIC_WHICH_HOSES_THINGS
  /* If its an remote system URI, we fork/exec, because
   * GIO can take a looooong time to respond ...
   */
  if (0 == strncmp (uri, "ssh://", 6))
    {
      pid_t pid;
      pid = fork ();
      if (0 == pid)
        {
          save_to_gio (wig, uri);

          /* exit the child as cleanly as we can ... do NOT
           * generate any socket/X11/graphics/gtk traffic.  */
          execl ("/bin/true", "/bin/true", NULL);
          execl ("/bin/false", "/bin/false", NULL);
        }
      else if (0 > pid)
        {
          g_warning ("unable to fork\n");
          save_to_gio (wig, uri);
        }
      else
        {
          /* else we are parent, child will save for us */
          sched_yield ();
        }
    }
#endif

  save_to_gio (wig, uri);
}

/* ============================================================== */
/* engine callbacks */

static void
redraw (GttProject *prj, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;

  gtt_ghtml_display (wig->gh, wig->filepath, wig->prj);
}

/* ============================================================== */
/* Global clipboard, allows cut task to be reparented to a different
 * project.  List of cut tasks allows for infinite undo. */

static GList *cutted_task_list = NULL;

/* ============================================================== */
/* Interval Popup Menu actions */

void
edit_interval_close_cb (GtkWidget *edit_ivl, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;
  wig->edit_ivl = NULL;
}

static void
interval_new_clicked_cb (GtkWidget *w, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;

  if (NULL == wig->edit_ivl)
    wig->edit_ivl = edit_interval_dialog_new ();

  edit_interval_set_close_callback (wig->edit_ivl,
                                    G_CALLBACK (edit_interval_close_cb), wig);

  wig->interval = gtt_interval_new_insert_after (wig->interval);
  edit_interval_set_interval (wig->edit_ivl, wig->interval);
  edit_interval_dialog_show (wig->edit_ivl);
}

static void
interval_edit_clicked_cb (GtkWidget *dw, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;

  if (NULL == wig->edit_ivl)
    wig->edit_ivl = edit_interval_dialog_new ();
  edit_interval_set_close_callback (wig->edit_ivl,
                                    G_CALLBACK (edit_interval_close_cb), wig);
  edit_interval_set_interval (wig->edit_ivl, wig->interval);
  edit_interval_dialog_show (wig->edit_ivl);
}

static void
interval_delete_clicked_cb (GtkWidget *w, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;
  gtt_interval_destroy (wig->interval);
  wig->interval = NULL;
}

static void
interval_merge_up_clicked_cb (GtkWidget *w, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;
  gtt_interval_merge_up (wig->interval);
  wig->interval = NULL;
}

static void
interval_merge_down_clicked_cb (GtkWidget *w, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;
  gtt_interval_merge_down (wig->interval);
  wig->interval = NULL;
}

static void
interval_move_up_clicked_cb (GtkWidget *w, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;
  GttTask *tsk = gtt_interval_get_parent (wig->interval);
  GttProject *prj = gtt_task_get_parent (tsk);
  GList *tasks = gtt_project_get_tasks (prj);
  if (!tasks)
    return;
  GList *this_task = g_list_find (tasks, tsk);
  if (!this_task)
    return;
  GList *prev_task = this_task->prev;
  if (!prev_task)
    return;
  GttTask *newtask = prev_task->data;
  gtt_task_append_interval (newtask, wig->interval);
}

static void
interval_move_down_clicked_cb (GtkWidget *w, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;
  GttTask *tsk = gtt_interval_get_parent (wig->interval);
  GttProject *prj = gtt_task_get_parent (tsk);
  GList *tasks = gtt_project_get_tasks (prj);
  if (!tasks)
    return;
  GList *this_task = g_list_find (tasks, tsk);
  if (!this_task)
    return;
  GList *next_task = this_task->next;
  if (!next_task)
    return;
  GttTask *newtask = next_task->data;
  gtt_task_add_interval (newtask, wig->interval);
}

static void
interval_insert_memo_cb (GtkWidget *w, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;
  GttTask *newtask;
  if (!wig->interval)
    return;

  /* Try to get billrates consistent across gap */
  newtask = gtt_interval_get_parent (wig->interval);
  newtask = gtt_task_copy (newtask);
  gtt_task_set_memo (newtask, _ ("New Diary Entry"));
  gtt_task_set_notes (newtask, "");

  gtt_interval_split (wig->interval, newtask);
  prop_task_dialog_show (newtask);
}

static void
interval_paste_memo_cb (GtkWidget *w, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;
  GttTask *newtask = NULL;

  if (!cutted_task_list || !wig->interval)
    return;

  /* Pop one off the stack, if stack has any depth to it */
  if (NULL == cutted_task_list->next)
    {
      newtask = gtt_task_copy (cutted_task_list->data);
    }
  else
    {
      newtask = cutted_task_list->data;
      cutted_task_list->data = NULL;
      cutted_task_list
          = g_list_delete_link (cutted_task_list, cutted_task_list);
    }

  gtt_interval_split (wig->interval, newtask);
}

static void
interval_popup_cb (Wiggy *wig)
{
  gtk_menu_popup (GTK_MENU (wig->interval_popup), NULL, NULL, NULL, wig, 1, 0);
  if (cutted_task_list)
    {
      gtk_widget_set_sensitive (wig->interval_paste, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (wig->interval_paste, FALSE);
    }

  if (gtt_interval_is_first_interval (wig->interval))
    {
      gtk_widget_set_sensitive (wig->interval_merge_up, FALSE);
      gtk_widget_set_sensitive (wig->interval_move_up, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (wig->interval_merge_up, TRUE);
      gtk_widget_set_sensitive (wig->interval_move_up, FALSE);
    }

  if (gtt_interval_is_last_interval (wig->interval))
    {
      gtk_widget_set_sensitive (wig->interval_merge_down, FALSE);
      gtk_widget_set_sensitive (wig->interval_move_down, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (wig->interval_merge_down, TRUE);
      gtk_widget_set_sensitive (wig->interval_move_down, FALSE);
    }

  GttTask *tsk = gtt_interval_get_parent (wig->interval);
  if (gtt_task_is_first_task (tsk))
    {
      gtk_widget_set_sensitive (wig->interval_move_up, FALSE);
    }
  if (gtt_task_is_last_task (tsk))
    {
      gtk_widget_set_sensitive (wig->interval_move_down, FALSE);
    }
}

/* ============================================================== */
/* Task Popup Menu Actions */

void
new_task_ui (GtkWidget *w, gpointer data)
{
  GttProject *prj;
  GttTask *newtask;

  prj = gtt_projects_tree_get_selected_project (projects_tree);
  if (!prj)
    return;

  newtask = gtt_task_new ();
  gtt_project_prepend_task (prj, newtask);
  prop_task_dialog_show (newtask);
}

/* ============================================================== */

void
edit_task_ui (GtkWidget *w, gpointer data)
{
  GttProject *prj;
  GttTask *task;

  prj = gtt_projects_tree_get_selected_project (projects_tree);
  if (!prj)
    return;

  task = gtt_project_get_first_task (prj);
  prop_task_dialog_show (task);
}

/* ============================================================== */

static void
task_new_task_clicked_cb (GtkWidget *w, gpointer data)
{
  GttTask *newtask;
  Wiggy *wig = (Wiggy *)data;
  newtask = gtt_task_new_insert (wig->task);
  prop_task_dialog_show (newtask);
}

static void
task_edit_task_clicked_cb (GtkWidget *w, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;
  prop_task_dialog_show (wig->task);
}

static void
task_delete_memo_clicked_cb (GtkWidget *w, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;

  /* It is physically impossible to cut just the memo, without
   * also cutting the time entries, when its the first one. */
  if (gtt_task_is_first_task (wig->task))
    return;

  gtt_task_merge_up (wig->task);

  GList *ctl = g_list_prepend (cutted_task_list, wig->task);
  gtt_task_remove (wig->task);
  cutted_task_list = ctl;
}

static void
task_delete_times_clicked_cb (GtkWidget *w, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;

  GList *ctl = g_list_prepend (cutted_task_list, wig->task);
  gtt_task_remove (wig->task);
  cutted_task_list = ctl;
}

static void
task_copy_clicked_cb (GtkWidget *w, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;

  GttTask *tsk = gtt_task_copy (wig->task);
  GList *ctl = g_list_prepend (cutted_task_list, tsk);
  cutted_task_list = ctl;
}

static void
task_paste_clicked_cb (GtkWidget *w, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;
  GttTask *newtask = NULL;

  if (!cutted_task_list || !wig->task)
    return;

  /* Pop one off the stack, if stack has any depth to it */
  newtask = cutted_task_list->data;
  cutted_task_list->data = NULL;
  cutted_task_list = g_list_delete_link (cutted_task_list, cutted_task_list);

  gtt_task_insert (wig->task, newtask);
}

static void
task_new_interval_cb (GtkWidget *w, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;

  if (NULL == wig->edit_ivl)
    wig->edit_ivl = edit_interval_dialog_new ();

  wig->interval = gtt_interval_new ();
  gtt_task_add_interval (wig->task, wig->interval);

  edit_interval_set_interval (wig->edit_ivl, wig->interval);
  edit_interval_dialog_show (wig->edit_ivl);
}

static void
task_popup_cb (Wiggy *wig)
{
  gtk_menu_popup (GTK_MENU (wig->task_popup), NULL, NULL, NULL, wig, 1, 0);
  if (gtt_task_is_first_task (wig->task))
    {
      gtk_widget_set_sensitive (wig->task_delete_memo, FALSE);
    }
  else
    {
      gtk_widget_set_sensitive (wig->task_delete_memo, TRUE);
    }

  if (cutted_task_list)
    {
      gtk_widget_set_sensitive (wig->task_paste, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (wig->task_paste, FALSE);
    }
}

/* ============================================================== */

#if LATER
static void
on_print_clicked_cb (GtkWidget *w, gpointer data)
{
  GladeXML *glxml;
  glxml = gtt_glade_xml_new ("glade/not-implemented.glade", "Not Implemented");
}
#endif

/* ============================================================== */
/* Publish Button handlers */

static void
on_publish_clicked_cb (GtkWidget *w, gpointer data)
{
  Wiggy *wig = data;

  /* Remind use of the last url that they used. */
  if (wig->plg && wig->plg->last_url)
    {
      gtk_entry_set_text (wig->publish_entry, wig->plg->last_url);
    }
  gtk_widget_show (wig->publish_popup);
}

static void
on_pub_cancel_clicked_cb (GtkWidget *w, gpointer data)
{
  Wiggy *wig = data;
  gtk_widget_hide (wig->publish_popup);
}

static void
on_pub_ok_clicked_cb (GtkWidget *w, gpointer data)
{
  Wiggy *wig = data;
  const char *uri = gtk_entry_get_text (wig->publish_entry);

  remember_uri (wig, uri);
  if (0 == strncmp (uri, "mailto:", 7))
    {
      GtkWidget *mb;
      mb = gtk_message_dialog_new (
          GTK_WINDOW (wig->publish_popup),
          // GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
          GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
          _ ("mailto: URL is not supported at this time"));
      g_signal_connect (G_OBJECT (mb), "response",
                        G_CALLBACK (gtk_widget_destroy), mb);
      gtk_widget_show (mb);
    }
  else
    {
      save_to_file (wig, uri);
    }
  gtk_widget_hide (wig->publish_popup);
}

/* ============================================================== */

static void
on_save_clicked_cb (GtkWidget *w, gpointer data)
{
  GtkWidget *dialog;
  Wiggy *wig = (Wiggy *)data;

  dialog = gtk_file_chooser_dialog_new (
      _ ("Save HTML To File"), GTK_WINDOW (wig->top),
      GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                  TRUE);

  /* Manually set a per-report history thingy */
  if (wig->plg && wig->plg->last_url
      && (0 == strncmp ("file:/", wig->plg->last_url, 6)))
    {
      gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog),
                                     wig->plg->last_url);
    }
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      char *filename
          = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      remember_uri (wig, filename);
      save_to_file (wig, filename);
      g_free (filename);
    }
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
on_close_clicked_cb (GtkWidget *w, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;

  if (NULL == wig->top)
    return;                     /* avoid recursive double-free */
  GtkWidget *topper = wig->top; /* avoid recursion */
  wig->top = NULL;

  /* Unplug the timout function, so that timer doesn't
   * pop after the destroy has happened. */
  if (wig->hover_timeout_id)
    {
      gtk_timeout_remove (wig->hover_timeout_id);
      wig->hover_timeout_id = 0;
      gtk_widget_hide (wig->hover_help_window);
    }
  if (wig->hover_kill_id)
    {
      gtk_timeout_remove (wig->hover_kill_id);
      wig->hover_kill_id = 0;
      gtk_widget_hide (wig->hover_help_window);
    }

  /* kill the display widget */
  gtk_widget_destroy (topper);

  /* close the main journal window ... everything */
  if (wig->prj)
    gtt_project_remove_notifier (wig->prj, redraw, wig);
  edit_interval_dialog_destroy (wig->edit_ivl);
  wig->prj = NULL;

  gtt_ghtml_destroy (wig->gh);
  g_free (wig->filepath);

  wig->gh = NULL;
  wig->html = NULL;
  g_free (wig);
}

static void
destroy_cb (GtkObject *ob, gpointer data)
{
}

static void
on_refresh_clicked_cb (GtkWidget *w, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;
  redraw (wig->prj, data);
}

/* ============================================================== */
/* html events */

static void
html_link_clicked_cb (GtkHTML *doc, const gchar *url, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;
  gpointer addr = NULL;
  char *str;

  /* h4x0r al3rt -- bare-naked pointer refernces ! */
  /* decode the address buried in the URL (if its there) */
  str = strstr (url, "0x");
  if (str)
    {
      addr = (gpointer)strtoul (str, NULL, 16);
    }

  if (0 == strncmp (url, "gtt:interval", 12))
    {
      wig->interval = addr;
      wig->task = NULL;
      if (addr)
        interval_popup_cb (wig);
    }
  else if (0 == strncmp (url, "gtt:task", 8))
    {
      wig->task = addr;
      wig->interval = NULL;
      if (addr)
        task_popup_cb (wig);
    }
  else if (0 == strncmp (url, "gtt:proj", 8))
    {
      GttProject *prj = addr;
      char *path;

      wig->task = NULL;
      wig->interval = NULL;

      path = gtt_ghtml_resolve_path (JOURNAL_REPORT, wig->filepath);
      do_show_report (path, NULL, NULL, prj, FALSE, NULL);
    }
  else
    {
      /* All other URL's are handed off to GIO, which will
       * deal with them more or less appropriately.  */
      do_show_report (url, NULL, NULL, NULL, FALSE, NULL);
    }
}

/* ============================================================== */

static void
html_url_requested_cb (GtkHTML *doc, const gchar *url, GtkHTMLStream *handle,
                       gpointer data)
{
  Wiggy *wig = data;
  const char *path = gtt_ghtml_resolve_path (url, wig->filepath);
  if (!path)
    return;

  GFile *ifile = g_file_new_for_path (path);

  GError *error = NULL;
  GFileInputStream *istream = g_file_read (ifile, NULL, &error);
  if (NULL == istream)
    {
      g_warning ("Failed to open file for reading: %s", error->message);

      g_error_free (error);
      error = NULL;

      g_object_unref (ifile);
      ifile = NULL;

      return;
    }

#define BSZ 16000
  char buff[BSZ];
  while (1)
    {
      const gsize bytes_read = g_input_stream_read (G_INPUT_STREAM (istream),
                                                    buff, BSZ, NULL, &error);
      if (0 > bytes_read)
        {
          g_warning ("Failed to read from file: %s", error->message);

          g_error_free (error);
          error = NULL;

          break;
        }
      if (0 == bytes_read)
        {
          break; // EOF
        }

      gtk_html_write (doc, handle, buff, bytes_read);
    }

  if (FALSE == g_input_stream_close (G_INPUT_STREAM (istream), NULL, &error))
    {
      g_warning ("Failed to close file after reading: %s", error->message);

      g_error_free (error);
      error = NULL;
    }

  g_object_unref (istream);
  istream = NULL;
  g_object_unref (ifile);
  ifile = NULL;
}

/* ============================================================== */
/* Display a tool-tip type of message when the user pauses thier
 * mouse over a URL.   If mouse pointer doesn't move for a
 * second, popup a window.
 *
 * XXX we were going to do something fancy with this, but now I
 * forget what ...
 * XXX we should display memos, etc.
 */

static char *
get_hover_msg (const gchar *url)
{
  char *str;
  gpointer addr = NULL;

  /* h4x0r al3rt bare-naked pointer parsing! */
  str = strstr (url, "0x");
  if (str)
    {
      addr = (gpointer)strtoul (str, NULL, 16);
    }

  /* XXX todo- -- it would be nice to make these popups
   * depend on the type of report tht the user is viewing.
   * should we pull them out of a scheme markup ??
   *
   * See http://developer.gnome.org/doc/API/2.4/pango/PangoMarkupFormat.html
   * for allowed markup contents.
   */
  if (addr && (0 == strncmp ("gtt:task:", url, 9)))
    {
      GttTask *task = addr;
      const char *memo = gtt_task_get_memo (task);
      const char *notes = gtt_task_get_notes (task);
      char *msg = g_strdup_printf ("<b><big>%s</big></b>\n%s\n", memo, notes);
      return msg;
    }

  if (0 == strncmp (url, "gtt:proj:", 9))
    {
      GttProject *prj = addr;
      const char *title = gtt_project_get_title (prj);
      const char *desc = gtt_project_get_desc (prj);
      const char *notes = gtt_project_get_notes (prj);
      char *msg = g_strdup_printf ("<b><big>%s</big></b>\n"
                                   "<b>%s</b>\n"
                                   "%s",
                                   title, desc, notes);
      return msg;
    }

  char *msg = _ ("Left-click to bring up menu");
  return g_strdup (msg);
}

static gint
hover_kill_func (gpointer data)
{
  Wiggy *wig = data;

  gtk_widget_hide (wig->hover_help_window);
  return 0;
}

static gint
hover_timer_func (gpointer data)
{
  Wiggy *wig = data;

  gint px = 0, py = 0, rx = 0, ry = 0;
  gtk_widget_get_pointer (wig->hover_help_window, &px, &py);
  gtk_window_get_position (GTK_WINDOW (wig->hover_help_window), &rx, &ry);
  rx += px;
  ry += py;
  rx += 25; /* move it out from under the cursor shape */
  gtk_window_move (GTK_WINDOW (wig->hover_help_window), rx, ry);
  gtk_widget_show (wig->hover_help_window);

  /* 8000 milisecs = 8 secs */
  /* XXX the 'hover-loose-focus' tool below seems to be broken, and
   * if we don't do something, gtt will leave visual turds on the
   * screen indefinitely.  So, in case of turds, hide the hover help
   * after 8 seconds.
   */
  wig->hover_kill_id = gtk_timeout_add (8000, hover_kill_func, wig);
  return 0;
}

/* If the html window looses focus, we've got to hide the flyover help;
 * otherwise it will leave garbage on the screen.
 */
static gboolean
hover_loose_focus (GtkWidget *w, GdkEventFocus *ev, gpointer data)
{
  Wiggy *wig = data;

  if (wig->hover_timeout_id)
    {
      gtk_timeout_remove (wig->hover_timeout_id);
      wig->hover_timeout_id = 0;
      gtk_widget_hide (wig->hover_help_window);
    }
  return 0;
}

static void
html_on_url_cb (GtkHTML *doc, const gchar *url, gpointer data)
{
  Wiggy *wig = data;
  if (NULL == wig->top)
    return;

  /* Create and initialize the hover-help window */
  if (!wig->hover_help_window)
    {
      wig->hover_help_window = gtk_window_new (GTK_WINDOW_POPUP);
      GtkWindow *wino = GTK_WINDOW (wig->hover_help_window);
      gtk_window_set_decorated (wino, FALSE);
      gtk_window_set_destroy_with_parent (wino, TRUE);
      gtk_window_set_transient_for (wino, GTK_WINDOW (wig->top));
      // gtk_window_set_type_hint (wino, GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);
      gtk_window_set_resizable (wino, FALSE); /* FALSE to enable auto-resize */

      /* There must be a better way to draw a line around the box ?? */
      GtkWidget *frame = gtk_frame_new (NULL);
      gtk_container_add (GTK_CONTAINER (wino), frame);
      gtk_container_set_resize_mode (GTK_CONTAINER (frame), GTK_RESIZE_PARENT);
      gtk_widget_show (frame);

      /* There must be a better way to pad the text all around ?? */
      GtkWidget *align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
      // gtk_alignment_set_padding (GTK_ALIGNMENT(align), 6, 6, 6, 6);
      gtk_container_add (GTK_CONTAINER (frame), align);
      gtk_container_set_resize_mode (GTK_CONTAINER (align), GTK_RESIZE_PARENT);
      gtk_widget_show (align);

      GtkWidget *label = gtk_label_new ("xxx");
      wig->hover_label = GTK_LABEL (label);
      gtk_container_add (GTK_CONTAINER (align), label);
      gtk_widget_show (label);

      /* So that we can loose focus later */
      gtk_window_set_focus (GTK_WINDOW (wig->top), GTK_WIDGET (wig->html));

      /* Set up in initial default, so later move works. */
      int px = 0, py = 0, rx = 0, ry = 0;
      gtk_widget_get_pointer (GTK_WIDGET (wig->top), &px, &py);
      gtk_window_get_position (GTK_WINDOW (wig->top), &rx, &ry);
      gtk_window_move (wino, rx + px, ry + py);
    }

  if (url)
    {
      char *msg = get_hover_msg (url);
      gtk_label_set_markup (wig->hover_label, msg);
      gtk_container_resize_children (GTK_CONTAINER (wig->hover_help_window));
      gtk_container_check_resize (GTK_CONTAINER (wig->hover_help_window));
      g_free (msg);
    }

  /* If hovering over a URL, bring up the help popup after one second. */
  if (url)
    {
      /* 600 milliseconds == 0.6 second */
      wig->hover_timeout_id = gtk_timeout_add (600, hover_timer_func, wig);
    }
  else
    {
      if (wig->hover_timeout_id)
        {
          gtk_timeout_remove (wig->hover_timeout_id);
          wig->hover_timeout_id = 0;
          gtk_widget_hide (wig->hover_help_window);
        }
    }
}

/* ============================================================== */
/* HTML form (method=GET, POST) events */

static QofBook *book = NULL;

/* Obtain an SQL query string from the HTML page, and submit
 * it to the query engnine, returning a list of projects.
 *
 * XXX right now, the only kind of queries that are allowed
 * are those that return lists of projects.  This should be fixed.
 * Part of the problem is that we can't currently identify the type
 * of the returned list.  I think we can fix this by returning
 * lists of qof entities, and using that to figure out the returned
 * type.  Need to change GttProject to derive from QofEntity.
 */
static GList *
perform_form_query (KvpFrame *kvpf)
{
  GList *results, *n;

  if (!kvpf)
    return NULL;

  /* Allow the user to enable form debugging by adding the following html:
   * <input type="hidden" name="debug" value="1">
   */
  char *user_debug = kvp_frame_get_string (kvpf, "debug");
  if (user_debug)
    {
      printf ("Debug: HTML Form Input=%s\n", kvp_frame_to_string (kvpf));
    }

  QofSqlQuery *q = qof_sql_query_new ();

  if (!book)
    book = qof_book_new ();
  qof_sql_query_set_book (q, book);
  qof_sql_query_set_kvp (q, kvpf);

  char *query_string = kvp_frame_get_string (kvpf, "query");
  if (!query_string)
    return NULL;
  if (0 == query_string[0])
    return NULL;

  if (user_debug)
    {
      printf ("earliest-end-date = %s\n",
              kvp_frame_get_string (kvpf, "earliest-end-date"));
      printf ("latest-start-date = %s\n",
              kvp_frame_get_string (kvpf, "latest-start-date"));
      printf ("Debug: Will run the query %s\n", query_string);
    }

  /* Run the query */
  results = qof_sql_query_run (q, query_string);

  /* XXX free q after using it */

  if (user_debug)
    {
      printf ("Debug: Query returned the following matching projects:\n");
      /* Print out the results */
      for (n = results; n; n = n->next)
        {
          GttProject *prj = n->data;
          printf ("\t%s\n", gtt_project_get_title (prj));
        }
    }

  return results;
}

static void
submit_clicked_cb (GtkHTML *doc, const gchar *method, const gchar *url,
                   const gchar *encoding, gpointer data)
{
  Wiggy *wig = (Wiggy *)data;
  const char *path;
  KvpFrame *kvpf;
  KvpValue *val;
  GList *qresults;

  if (!wig->prj)
    wig->prj = gtt_projects_tree_get_selected_project (projects_tree);

  kvpf = kvp_frame_new ();
  kvp_frame_add_url_encoding (kvpf, encoding);

  /* If there is a report specified, use that, else use
   * the report specified in the form "action" */
  val = kvp_frame_get_slot (kvpf, "report-path");
  path = kvp_value_get_string (val);
  if (!path)
    path = url;
  path = gtt_ghtml_resolve_path (path, wig->filepath);

  /* Build an ad-hoc query */
  qresults = perform_form_query (kvpf);

  /* Open a new window */
  do_show_report (path, NULL, kvpf, wig->prj, TRUE, qresults);

  /* XXX We cannnot reuse the same window from this callback, we
   * have to let the callback return first, else we get a nasty error.
   * This should be fixed: if the query and the result form is the
   * same, we should re-use the same window.
   */
#if 0
	g_free (wig->filepath);
	wig->filepath = path;
	gtt_ghtml_display (wig->gh, path, wig->prj);
#endif
}

/* ============================================================== */

static void
do_show_report (const char *report, GttPlugin *plg, KvpFrame *kvpf,
                GttProject *prj, gboolean did_query, GList *prjlist)
{
  GtkWidget *jnl_top, *jnl_viewport;
  GladeXML *glxml;
  Wiggy *wig;

  glxml = gtt_glade_xml_new ("glade/journal.glade", "Journal Window");

  jnl_top = glade_xml_get_widget (glxml, "Journal Window");
  jnl_viewport = glade_xml_get_widget (glxml, "Journal ScrollWin");

  wig = g_new0 (Wiggy, 1);
  wig->edit_ivl = NULL;

  wig->top = jnl_top;
  wig->plg = plg;

  if (plg)
    gtk_window_set_title (GTK_WINDOW (jnl_top), plg->name);

  /* Create browser, plug it into the viewport */
  wig->html = GTK_HTML (gtk_html_new ());
  gtk_html_set_editable (wig->html, FALSE);
  gtk_container_add (GTK_CONTAINER (jnl_viewport), GTK_WIDGET (wig->html));

  wig->gh = gtt_ghtml_new ();
  gtt_ghtml_set_stream (wig->gh, wig, wiggy_open, wiggy_write, wiggy_close,
                        wiggy_error);

  /* ---------------------------------------------------- */
  /* Signals for the browser, and the Journal window */

  glade_xml_signal_connect_data (glxml, "on_close_clicked",
                                 GTK_SIGNAL_FUNC (on_close_clicked_cb), wig);

  glade_xml_signal_connect_data (glxml, "on_save_clicked",
                                 GTK_SIGNAL_FUNC (on_save_clicked_cb), wig);

#if LATER
  glade_xml_signal_connect_data (glxml, "on_print_clicked",
                                 GTK_SIGNAL_FUNC (on_print_clicked_cb), wig);
#endif

  glade_xml_signal_connect_data (glxml, "on_publish_clicked",
                                 GTK_SIGNAL_FUNC (on_publish_clicked_cb), wig);

  glade_xml_signal_connect_data (glxml, "on_refresh_clicked",
                                 GTK_SIGNAL_FUNC (on_refresh_clicked_cb), wig);

  g_signal_connect (G_OBJECT (wig->top), "destroy", G_CALLBACK (destroy_cb),
                    wig);

  g_signal_connect (G_OBJECT (wig->html), "link_clicked",
                    G_CALLBACK (html_link_clicked_cb), wig);

  g_signal_connect (G_OBJECT (wig->html), "submit",
                    G_CALLBACK (submit_clicked_cb), wig);

  g_signal_connect (G_OBJECT (wig->html), "url_requested",
                    G_CALLBACK (html_url_requested_cb), wig);

  g_signal_connect (G_OBJECT (wig->html), "on_url",
                    G_CALLBACK (html_on_url_cb), wig);

  g_signal_connect (G_OBJECT (wig->html), "focus_out_event",
                    G_CALLBACK (hover_loose_focus), wig);

  gtk_widget_show (GTK_WIDGET (wig->html));
  gtk_widget_show (jnl_top);

  /* ---------------------------------------------------- */
  /* This is the popup for asking for the user to input an URL. */

  glxml = gtt_glade_xml_new ("glade/journal.glade", "Publish Dialog");
  wig->publish_popup = glade_xml_get_widget (glxml, "Publish Dialog");
  wig->publish_entry = GTK_ENTRY (glade_xml_get_widget (glxml, "url entry"));

  glade_xml_signal_connect_data (glxml, "on_pub_help_clicked",
                                 GTK_SIGNAL_FUNC (gtt_help_popup), NULL);

  glade_xml_signal_connect_data (glxml, "on_pub_cancel_clicked",
                                 GTK_SIGNAL_FUNC (on_pub_cancel_clicked_cb),
                                 wig);

  glade_xml_signal_connect_data (glxml, "on_pub_ok_clicked",
                                 GTK_SIGNAL_FUNC (on_pub_ok_clicked_cb), wig);

  /* ---------------------------------------------------- */
  /* This is the popup menu that says 'edit/delete/merge' */
  /* for intervals */

  glxml = gtt_glade_xml_new ("glade/interval_popup.glade", "Interval Popup");
  wig->interval_popup = glade_xml_get_widget (glxml, "Interval Popup");
  wig->interval_paste = glade_xml_get_widget (glxml, "paste_memo");
  wig->interval_merge_up = glade_xml_get_widget (glxml, "merge_up");
  wig->interval_merge_down = glade_xml_get_widget (glxml, "merge_down");
  wig->interval_move_up = glade_xml_get_widget (glxml, "move_up");
  wig->interval_move_down = glade_xml_get_widget (glxml, "move_down");
  wig->interval = NULL;

  glade_xml_signal_connect_data (glxml, "on_new_interval_activate",
                                 GTK_SIGNAL_FUNC (interval_new_clicked_cb),
                                 wig);

  glade_xml_signal_connect_data (glxml, "on_edit_activate",
                                 GTK_SIGNAL_FUNC (interval_edit_clicked_cb),
                                 wig);

  glade_xml_signal_connect_data (glxml, "on_delete_activate",
                                 GTK_SIGNAL_FUNC (interval_delete_clicked_cb),
                                 wig);

  glade_xml_signal_connect_data (
      glxml, "on_merge_up_activate",
      GTK_SIGNAL_FUNC (interval_merge_up_clicked_cb), wig);

  glade_xml_signal_connect_data (
      glxml, "on_merge_down_activate",
      GTK_SIGNAL_FUNC (interval_merge_down_clicked_cb), wig);

  glade_xml_signal_connect_data (glxml, "on_move_up_activate",
                                 GTK_SIGNAL_FUNC (interval_move_up_clicked_cb),
                                 wig);

  glade_xml_signal_connect_data (
      glxml, "on_move_down_activate",
      GTK_SIGNAL_FUNC (interval_move_down_clicked_cb), wig);

  glade_xml_signal_connect_data (glxml, "on_insert_memo_activate",
                                 GTK_SIGNAL_FUNC (interval_insert_memo_cb),
                                 wig);

  glade_xml_signal_connect_data (glxml, "on_paste_memo_activate",
                                 GTK_SIGNAL_FUNC (interval_paste_memo_cb),
                                 wig);

  /* ---------------------------------------------------- */
  /* This is the popup menu that says 'edit/delete/merge' */
  /* for tasks */

  glxml = gtt_glade_xml_new ("glade/task_popup.glade", "Task Popup");
  wig->task_popup = glade_xml_get_widget (glxml, "Task Popup");
  wig->task_delete_memo = glade_xml_get_widget (glxml, "delete_memo");
  wig->task_paste = glade_xml_get_widget (glxml, "paste");
  wig->task = NULL;

  glade_xml_signal_connect_data (glxml, "on_new_task_activate",
                                 GTK_SIGNAL_FUNC (task_new_task_clicked_cb),
                                 wig);

  glade_xml_signal_connect_data (glxml, "on_edit_task_activate",
                                 GTK_SIGNAL_FUNC (task_edit_task_clicked_cb),
                                 wig);

  glade_xml_signal_connect_data (glxml, "on_delete_memo_activate",
                                 GTK_SIGNAL_FUNC (task_delete_memo_clicked_cb),
                                 wig);

  glade_xml_signal_connect_data (
      glxml, "on_delete_times_activate",
      GTK_SIGNAL_FUNC (task_delete_times_clicked_cb), wig);

  glade_xml_signal_connect_data (glxml, "on_copy_activate",
                                 GTK_SIGNAL_FUNC (task_copy_clicked_cb), wig);

  glade_xml_signal_connect_data (glxml, "on_paste_activate",
                                 GTK_SIGNAL_FUNC (task_paste_clicked_cb), wig);

  glade_xml_signal_connect_data (glxml, "on_new_interval_activate",
                                 GTK_SIGNAL_FUNC (task_new_interval_cb), wig);

  /* ---------------------------------------------------- */
  wig->hover_help_window = NULL;
  wig->hover_timeout_id = 0;

  /* ---------------------------------------------------- */
  /* Finally ... display the actual journal */

  wig->prj = prj;
  wig->filepath = g_strdup (report);
  if (kvpf)
    {
      if (wig->gh->kvp)
        kvp_frame_delete (wig->gh->kvp);
      wig->gh->kvp = kvpf;
    }
  wig->gh->did_query = did_query;
  wig->gh->query_result = prjlist;

  /* XXX should add notifiers for prjlist too ?? Yes we should */
  if (prj)
    gtt_project_add_notifier (prj, redraw, wig);
  gtt_ghtml_display (wig->gh, report, prj);

  /* Can only set editable *after* there's content in the window */
  // gtk_html_set_editable (wig->html, TRUE);
}

/* ============================================================== */

char *
gtt_ghtml_resolve_path (const char *path_frag, const char *reference_path)
{
  const GList *list;
  char buff[PATH_MAX], *path;

  if (!path_frag)
    return NULL;

  /* First, look for path_frag in the reference path. */
  if (reference_path)
    {
      char *p;
      strncpy (buff, reference_path, PATH_MAX);
      p = strrchr (buff, '/');
      if (p)
        {
          p++;
          strncpy (p, path_frag, PATH_MAX - (p - buff));
          if (g_file_test ((buff), G_FILE_TEST_EXISTS))
            return g_strdup (buff);
        }
    }

  /* Next, check each language that the user is willing to look at. */
  list = gnome_i18n_get_language_list ("LC_MESSAGES");
  for (; list; list = list->next)
    {
      const char *lang = list->data;

      /* See if gnotime/ghtml/<lang>/<path_frag> exists. */
      /* Look in the local build dir first (for testing) */

      snprintf (buff, PATH_MAX, "ghtml/%s/%s", lang, path_frag);
      path = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_DATADIR, buff,
                                        TRUE, NULL);
      if (path)
        return path;

      snprintf (buff, PATH_MAX, "gnotime/ghtml/%s/%s", lang, path_frag);
      path = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_DATADIR, buff,
                                        TRUE, NULL);
      if (path)
        return path;

      /* Backwards compat, check the gtt dir, not just the gnotime dir */
      snprintf (buff, PATH_MAX, "gtt/ghtml/%s/%s", lang, path_frag);
      path = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_DATADIR, buff,
                                        TRUE, NULL);
      if (path)
        return path;

      /* some users compile with path settings that gnome
       * cannot find.  In that case we have to supply a full
       * path and check it's existance directly -- we CANNOT
       * use gnome_datadir_file() because it wont work!
       *
       * -warlord 2001-11-29
       */
      snprintf (buff, PATH_MAX, GTTDATADIR "/ghtml/%s/%s", lang, path_frag);
      if (g_file_test ((buff), G_FILE_TEST_EXISTS))
        return g_strdup (buff);
    }
  return g_strdup (path_frag);
}

/* XXX The show_report routine should probably be using data pulled from
 * GConf, in the same way that the user-defined items are obtained.
 * Currently, these are hard-coded in menus.c.
 */

void
show_report (GtkWidget *w, gpointer data)
{
  char *report_file = data;
  GttProject *prj;
  char *path;

  prj = gtt_projects_tree_get_selected_project (projects_tree);

  path = gtt_ghtml_resolve_path (report_file, NULL);
  do_show_report (path, NULL, NULL, prj, FALSE, NULL);
}

void
invoke_report (GtkWidget *widget, gpointer data)
{
  GttProject *prj;
  GttPlugin *plg = data;

  prj = gtt_projects_tree_get_selected_project (projects_tree);

  /* Do not gnome-filepath this, this is for user-defined reports */
  do_show_report (plg->path, plg, NULL, prj, FALSE, NULL);
}

/* ===================== END OF FILE ==============================  */
