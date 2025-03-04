/* GNOME GUI Library
 * Copyright (C) 1997, 1998 Jay Painter
 * Copyright (C) 2023      Markus Prasser
 * All rights reserved.
 *
 * This file is part of GnoTime (originally the Gnome Library).
 *
 * GnoTime is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Library General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * GnoTime is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Library General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with the GnoTime; see the file COPYING.LIB.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
/*
  @NOTATION@
*/

#include "config.h"

#include "gtt_dialog.h"

#include <gdk/gdkkeysyms-compat.h>

#include <glib/gi18n.h>

#ifndef GTT_DISABLE_DEPRECATED_SOURCE

enum
{
  CLICKED,
  CLOSE,
  LAST_SIGNAL
};

#define GTT_PAD 8
#define GTT_PAD_SMALL 4

static void gtt_dialog_class_init (GttDialogClass *klass);
static void gtt_dialog_init (GttDialog *dialog);
static void gtt_dialog_init_action_area (GttDialog *dialog);

static void gtt_dialog_button_clicked (GtkWidget *button,
                                       GtkWidget *messagebox);
static gint gtt_dialog_key_pressed (GtkWidget *d, GdkEventKey *e);
static gint gtt_dialog_delete_event (GtkWidget *d, GdkEventAny *e);
static void gtt_dialog_destroy (GtkObject *object);
static void gtt_dialog_finalize (GObject *object);
static void gtt_dialog_show (GtkWidget *d);
static void gtt_dialog_close_real (GttDialog *d);

static gint dialog_signals[LAST_SIGNAL] = { 0, 0 };

G_DEFINE_TYPE (GttDialog, gtt_dialog, GTK_TYPE_WINDOW)

static void
gtt_dialog_class_init (GttDialogClass *klass)
{
  GtkObjectClass *object_class;
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass *)klass;
  gobject_class = (GObjectClass *)klass;
  widget_class = (GtkWidgetClass *)klass;

  dialog_signals[CLOSE] = g_signal_new (
      "close", G_TYPE_FROM_CLASS (gobject_class), G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (GttDialogClass, close), NULL, NULL,
      gtk_marshal_BOOLEAN__VOID, G_TYPE_BOOLEAN, 0);

  dialog_signals[CLICKED] = g_signal_new (
      "clicked", G_TYPE_FROM_CLASS (gobject_class), G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET (GttDialogClass, clicked), NULL, NULL,
      g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);

  klass->clicked = NULL;
  klass->close = NULL;
  object_class->destroy = gtt_dialog_destroy;
  gobject_class->finalize = gtt_dialog_finalize;
  widget_class->key_press_event = gtt_dialog_key_pressed;
  widget_class->delete_event = gtt_dialog_delete_event;
  widget_class->show = gtt_dialog_show;
}

static void
gtt_dialog_init (GttDialog *dialog)
{
  GtkWidget *vbox;

  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTT_IS_DIALOG (dialog));

  dialog->just_hide = FALSE;
  dialog->click_closes = FALSE;
  dialog->buttons = NULL;

  /* FIXME:!!!!!!!!!!!!!! */
  GTK_WINDOW (dialog)->type = 1;
  gtk_window_set_position (GTK_WINDOW (dialog), 1);

  /*
  GTK_WINDOW(dialog)->type = gnome_preferences_get_dialog_type();
  gtk_window_set_position(GTK_WINDOW(dialog),
                      gnome_preferences_get_dialog_position());
                      */

  dialog->accelerators = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (dialog), dialog->accelerators);

  vbox = gtk_vbox_new (FALSE, GTT_PAD);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), GTT_PAD_SMALL);
  gtk_container_add (GTK_CONTAINER (dialog), vbox);
  gtk_widget_show (vbox);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  dialog->vbox = gtk_vbox_new (FALSE, GTT_PAD);
  gtk_box_pack_start (GTK_BOX (vbox), dialog->vbox, TRUE, TRUE, GTT_PAD_SMALL);
  gtk_widget_show (dialog->vbox);
}

static void
gtt_dialog_init_action_area (GttDialog *dialog)
{
  GtkWidget *separator;

  if (dialog->action_area)
    return;

  dialog->action_area = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog->action_area),
                             GTK_BUTTONBOX_END);

  gtk_box_pack_end (GTK_BOX (dialog->vbox), dialog->action_area, FALSE, TRUE,
                    0);
  gtk_widget_show (dialog->action_area);

  separator = gtk_hseparator_new ();
  gtk_box_pack_end (GTK_BOX (dialog->vbox), separator, FALSE, TRUE,
                    GTT_PAD_SMALL);
  gtk_widget_show (separator);
}

/**
 * gtt_dialog_construct: Functionality of gtt_dialog_new() for language
 *wrappers.
 * @dialog: Dialog to construct.
 * @title: Title of the dialog.
 * @ap: va_list of buttons, NULL-terminated.
 *
 * See gtt_dialog_new().
 **/
void
gtt_dialog_construct (GttDialog *dialog, const gchar *title, va_list ap)
{
  gchar *button_name;

  if (title)
    gtk_window_set_title (GTK_WINDOW (dialog), title);

  while (TRUE)
    {
      button_name = va_arg (ap, gchar *);

      if (button_name == NULL)
        {
          break;
        }

      gtt_dialog_append_button (dialog, button_name);
    }

  /* argument list may be null if the user wants to do weird things to the
   * dialog, but we need to make sure this is initialized */
  gtt_dialog_init_action_area (dialog);
}

/**
 * gtt_dialog_constructv: Functionality of gtt_dialog_new(), for language
 *wrappers.
 * @dialog: Dialog to construct.
 * @title: Title of the dialog.
 * @buttons: NULL-terminated array of buttons.
 *
 * See gtt_dialog_new().
 **/
void
gtt_dialog_constructv (GttDialog *dialog, const gchar *title,
                       const gchar **buttons)
{
  const gchar *button_name;

  if (title)
    gtk_window_set_title (GTK_WINDOW (dialog), title);

  if (buttons)
    {
      while (TRUE)
        {

          button_name = *buttons++;

          if (button_name == NULL)
            {
              break;
            }

          gtt_dialog_append_button (dialog, button_name);
        };
    }

  /* argument list may be null if the user wants to do weird things to the
   * dialog, but we need to make sure this is initialized */
  gtt_dialog_init_action_area (dialog);
}

/**
 * gtt_dialog_new: Create a new #GttDialog.
 * @title: The title of the dialog; appears in window titlebar.
 * @...: NULL-terminated varargs list of button names or GTT_STOCK_BUTTON_*
 *defines.
 *
 * Creates a new #GttDialog, with the given title, and any button names
 * in the arg list. Buttons can be simple names, such as _("My Button"),
 * or gnome-stock defines such as %GTT_STOCK_BUTTON_OK, etc. The last
 * argument should be NULL to terminate the list.
 *
 * Buttons passed to this function are numbered from left to right,
 * starting with 0. So the first button in the arglist is button 0,
 * then button 1, etc.  These numbers are used throughout the
 * #GttDialog API.
 *
 * Return value: The new #GttDialog.
 **/
GtkWidget *
gtt_dialog_new (const gchar *title, ...)
{
  va_list ap;
  GttDialog *dialog;

  dialog = g_object_new (GTT_TYPE_DIALOG, NULL);

  va_start (ap, title);

  gtt_dialog_construct (dialog, title, ap);

  va_end (ap);

  return GTK_WIDGET (dialog);
}

/**
 * gtt_dialog_newv: Create a new #GttDialog.
 * @title: Title of the dialog.
 * @buttons: NULL-terminated vector of buttons names.
 *
 * See gtt_dialog_new(), this function is identical but does not use
 * varargs.
 *
 * Return value: The new #GttDialog.
 **/
GtkWidget *
gtt_dialog_newv (const gchar *title, const gchar **buttons)
{
  GttDialog *dialog;

  dialog = g_object_new (GTT_TYPE_DIALOG, NULL);

  gtt_dialog_constructv (dialog, title, buttons);

  return GTK_WIDGET (dialog);
}

/**
 * gtt_dialog_set_parent: Set the logical parent window of a #GttDialog.
 * @dialog: #GttDialog to set the parent of.
 * @parent: Parent #GtkWindow.
 *
 * Dialogs have "parents," usually the main application window which spawned
 * them. This function will let the window manager know about the parent-child
 * relationship. Usually this means the dialog must stay on top of the parent,
 * and will be minimized when the parent is. Gnome also allows users to
 * request dialog placement above the parent window (vs. at the mouse position,
 * or at a default window manger location).
 *
 **/
void
gtt_dialog_set_parent (GttDialog *dialog, GtkWindow *parent)
{
  /* This code is duplicated in gnome-file-entry.c:browse-clicked.  If
   * a change is made here, update it there too. */
  /* Also, It might be good at some point to make the first argument
   * GtkWidget, instead of GttDialog */
  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTT_IS_DIALOG (dialog));
  g_return_if_fail (parent != NULL);
  g_return_if_fail (GTK_IS_WINDOW (parent));
  g_return_if_fail (parent != GTK_WINDOW (dialog));

  gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
}

/**
 * gtt_dialog_append_buttons: Add buttons to a dialog after its initial
 *construction.
 * @dialog: #GttDialog to add buttons to.
 * @first: First button to add.
 * @...: varargs list of additional buttons, NULL-terminated.
 *
 * This function is mostly for internal library use. You should use
 * gtt_dialog_new() instead. See that function for a description of
 * the button arguments.
 *
 **/
void
gtt_dialog_append_buttons (GttDialog *dialog, const gchar *first, ...)
{
  va_list ap;
  const gchar *button_name = first;

  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTT_IS_DIALOG (dialog));

  va_start (ap, first);

  while (button_name != NULL)
    {
      gtt_dialog_append_button (dialog, button_name);
      button_name = va_arg (ap, gchar *);
    }
  va_end (ap);
}

/**
 * gtt_dialog_append_button:
 * @dialog: #GttDialog to add button to.
 * @button_name: Button to add.
 *
 * Add a button to a dialog after its initial construction.
 * This function is mostly for internal library use. You should use
 * gtt_dialog_new() instead. See that function for a description of
 * the button argument.
 *
 **/
void
gtt_dialog_append_button (GttDialog *dialog, const gchar *button_name)
{
  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTT_IS_DIALOG (dialog));

  if (button_name != NULL)
    {
      GtkWidget *button;

      gtt_dialog_init_action_area (dialog);

      button = gtk_button_new_from_stock (button_name);
      GTK_WIDGET_SET_FLAGS (GTK_WIDGET (button), GTK_CAN_DEFAULT);
      gtk_box_pack_start (GTK_BOX (dialog->action_area), button, TRUE, TRUE,
                          0);

      gtk_widget_grab_default (button);
      gtk_widget_show (button);

      g_signal_connect_after (button, "clicked",
                              G_CALLBACK (gtt_dialog_button_clicked), dialog);

      dialog->buttons = g_list_append (dialog->buttons, button);
    }
}

#define GTT_STOCK_BUTTON_PADDING 2
static GtkWidget *
gtt_pixmap_button (GtkWidget *pixmap, const char *text)
{
  GtkWidget *button, *label, *hbox, *w;
  gboolean use_icon, use_label;

  g_return_val_if_fail (text != NULL, NULL);

  button = gtk_button_new ();
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_container_add (GTK_CONTAINER (button), hbox);
  w = hbox;
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (w), hbox, TRUE, FALSE,
                      GTT_STOCK_BUTTON_PADDING);

  use_icon = TRUE;  // FIXME: gnome_config_get_bool
                    // ("/Gnome/Icons/ButtonUseIcons=true");
  use_label = TRUE; // FIXME: gnome_config_get_bool
                    // ("/Gnome/Icons/ButtonUseLabels=true");

  if ((use_label) || (!use_icon) || (!pixmap))
    {
      label = gtk_label_new (_ (text));
      gtk_widget_show (label);
      gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE,
                        GTT_STOCK_BUTTON_PADDING);
    }

  if ((use_icon) && (pixmap))
    {

      gtk_widget_show (pixmap);
      gtk_box_pack_start (GTK_BOX (hbox), pixmap, FALSE, FALSE, 0);
    }
  else
    {
      g_object_unref (pixmap);
    }

  return button;
}

/**
 * gtt_dialog_append_button_with_pixmap:
 * @dialog: #GttDialog to add the button to.
 * @button_name: Name of the button, or stock button #define.
 * @pixmap_name: Stock pixmap name.
 *
 * Add a pixmap button to a dialog.
 * gtt_dialog_new() does not permit custom buttons with pixmaps, so if you
 * want one of those you need to use this function.
 *
 **/
void
gtt_dialog_append_button_with_pixmap (GttDialog *dialog,
                                      const gchar *button_name,
                                      const gchar *pixmap_name)
{
  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTT_IS_DIALOG (dialog));

  if (button_name != NULL)
    {
      GtkWidget *button;

      if (pixmap_name != NULL)
        {
          GtkWidget *pixmap;

          pixmap
              = gtk_image_new_from_stock (pixmap_name, GTK_ICON_SIZE_BUTTON);
          button = gtt_pixmap_button (pixmap, button_name);
        }
      else
        {
          button = gtk_button_new_from_stock (button_name);
        }

      gtt_dialog_init_action_area (dialog);

      GTK_WIDGET_SET_FLAGS (GTK_WIDGET (button), GTK_CAN_DEFAULT);
      gtk_box_pack_start (GTK_BOX (dialog->action_area), button, TRUE, TRUE,
                          0);

      gtk_widget_grab_default (button);
      gtk_widget_show (button);

      g_signal_connect_after (button, "clicked",
                              G_CALLBACK (gtt_dialog_button_clicked), dialog);

      dialog->buttons = g_list_append (dialog->buttons, button);
    }
}

/**
 * gtt_dialog_append_buttonsv: Like gtt_dialog_append_buttons(), but with a
 *vector arg instead of a varargs list.
 * @dialog: #GttDialog to append to.
 * @buttons: NULL-terminated vector of buttons to append.
 *
 * For internal use, language bindings, etc. Use gtt_dialog_new() instead.
 *
 **/
void
gtt_dialog_append_buttonsv (GttDialog *dialog, const gchar **buttons)
{
  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTT_IS_DIALOG (dialog));

  while (*buttons != NULL)
    {
      gtt_dialog_append_button (dialog, *buttons);
      buttons++;
    }
}

/**
 * gtt_dialog_append_buttons_with_pixmaps: Like
 *gtt_dialog_append_button_with_pixmap(), but allows multiple buttons.
 * @dialog: #GttDialog to append to.
 * @names: NULL-terminated vector of button names.
 * @pixmaps: NULL-terminated vector of pixmap names.
 *
 * Simply calls gtt_dialog_append_button_with_pixmap() repeatedly.
 *
 **/
void
gtt_dialog_append_buttons_with_pixmaps (GttDialog *dialog, const gchar **names,
                                        const gchar **pixmaps)
{
  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTT_IS_DIALOG (dialog));

  while (*names != NULL)
    {
      gtt_dialog_append_button_with_pixmap (dialog, *names, *pixmaps);
      names++;
      pixmaps++;
    }
}

struct GttDialogRunInfo
{
  gint button_number;
  gint close_id, clicked_id, destroy_id;
  gboolean destroyed;
  GMainLoop *mainloop;
};

static void
gtt_dialog_shutdown_run (GttDialog *dialog, struct GttDialogRunInfo *runinfo)
{
  if (!runinfo->destroyed)
    {
      g_signal_handler_disconnect (dialog, runinfo->close_id);
      g_signal_handler_disconnect (dialog, runinfo->clicked_id);

      runinfo->close_id = runinfo->clicked_id = -1;
    }

  if (runinfo->mainloop)
    {
      g_main_loop_quit (runinfo->mainloop);
      g_main_loop_unref (runinfo->mainloop);
      runinfo->mainloop = NULL;
    }
}

static void
gtt_dialog_setbutton_callback (GttDialog *dialog, gint button_number,
                               struct GttDialogRunInfo *runinfo)
{
  if (runinfo->close_id < 0)
    return;

  runinfo->button_number = button_number;

  gtt_dialog_shutdown_run (dialog, runinfo);
}

static gboolean
gtt_dialog_quit_run (GttDialog *dialog, struct GttDialogRunInfo *runinfo)
{
  if (runinfo->close_id < 0)
    return FALSE;

  gtt_dialog_shutdown_run (dialog, runinfo);

  return FALSE;
}

static void
gtt_dialog_mark_destroy (GttDialog *dialog, struct GttDialogRunInfo *runinfo)
{
  runinfo->destroyed = TRUE;

  if (runinfo->close_id < 0)
    return;
  else
    gtt_dialog_shutdown_run (dialog, runinfo);
}

static gint
gtt_dialog_run_real (GttDialog *dialog, gboolean close_after)
{
  gboolean was_modal;
  struct GttDialogRunInfo ri = { -1, -1, -1, -1, FALSE, NULL };

  g_return_val_if_fail (dialog != NULL, -1);
  g_return_val_if_fail (GTT_IS_DIALOG (dialog), -1);

  was_modal = GTK_WINDOW (dialog)->modal;
  if (!was_modal)
    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  /* There are several things that could happen to the dialog, and we
     need to handle them all: click, delete_event, close, destroy */

  ri.clicked_id = g_signal_connect (
      dialog, "clicked", G_CALLBACK (gtt_dialog_setbutton_callback), &ri);

  ri.close_id = g_signal_connect (dialog, "close",
                                  G_CALLBACK (gtt_dialog_quit_run), &ri);

  ri.destroy_id = g_signal_connect (dialog, "destroy",
                                    G_CALLBACK (gtt_dialog_mark_destroy), &ri);

  if (!GTK_WIDGET_VISIBLE (GTK_WIDGET (dialog)))
    gtk_widget_show (GTK_WIDGET (dialog));

  ri.mainloop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (ri.mainloop);

  g_assert (ri.mainloop == NULL);

  if (!ri.destroyed)
    {

      g_signal_handler_disconnect (dialog, ri.destroy_id);

      if (!was_modal)
        {
          gtk_window_set_modal (GTK_WINDOW (dialog), FALSE);
        }

      if (ri.close_id >= 0) /* We didn't shut down the run? */
        {
          g_signal_handler_disconnect (dialog, ri.close_id);
          g_signal_handler_disconnect (dialog, ri.clicked_id);
        }

      if (close_after)
        {
          gtt_dialog_close (dialog);
        }
    }

  return ri.button_number;
}

/**
 * gtt_dialog_run: Make the dialog modal and block waiting for user response.
 * @dialog: #GttDialog to use.
 *
 * Blocks until the user clicks a button, or closes the dialog with the
 * window manager's close decoration (or by pressing Escape).
 *
 * You need to set up the dialog to do the right thing when a button
 * is clicked or delete_event is received; you must consider both of
 * those possibilities so that you know the status of the dialog when
 * gtt_dialog_run() returns. A common mistake is to forget about
 * Escape and the window manager close decoration; by default, these
 * call gtt_dialog_close(), which by default destroys the dialog. If
 * your button clicks do not destroy the dialog, you don't know
 * whether the dialog is destroyed when gtt_dialog_run()
 * returns. This is bad.
 *
 * So you should either close the dialog on button clicks as well, or
 * change the gtt_dialog_close() behavior to hide instead of
 * destroy. You can do this with gtt_dialog_close_hides().
 *
 * Return value:  If a button was pressed, the button number is returned. If
 *not, -1 is returned.
 **/
gint
gtt_dialog_run (GttDialog *dialog)
{
  return gtt_dialog_run_real (dialog, FALSE);
}

/**
 * gtt_dialog_run_and_close: Like gtt_dialog_run(), but force-closes the
 *dialog after the run, iff the dialog was not closed already.
 * @dialog: #GttDialog to use.
 *
 * See gtt_dialog_run(). The only difference is that this function calls
 * gtt_dialog_close() before returning, if the dialog was not already closed.
 *
 * Return value: If a button was pressed, the button number. Otherwise -1.
 **/
gint
gtt_dialog_run_and_close (GttDialog *dialog)
{
  return gtt_dialog_run_real (dialog, TRUE);
}

/**
 * gtt_dialog_set_default: Set the default button for the dialog. The Enter
 *key activates the default button.
 * @dialog: #GttDialog to affect.
 * @button: Number of the default button.
 *
 * The default button will be activated if the user just presses return.
 * Usually you should make the least-destructive button the default.
 * Otherwise, the most commonly-used button.
 *
 **/
void
gtt_dialog_set_default (GttDialog *dialog, gint button)
{
  GList *list;

  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTT_IS_DIALOG (dialog));

  list = g_list_nth (dialog->buttons, button);

  if (list && list->data)
    {
      gtk_widget_grab_default (GTK_WIDGET (list->data));
      return;
    }
#ifdef GTT_ENABLE_DEBUG
  /* If we didn't find the button, complain */
  g_warning ("Button number %d does not appear to exist\n", button);
#endif
}

/**
 * gtt_dialog_grab_focus: Makes a button grab the focus. T
 * @dialog: #GttDialog to affect.
 * @button: Number of the default button.
 *
 * The button @button will grab the focus.  Use this for dialogs
 * Where only buttons are displayed and you want to change the
 * default button.
 **/
void
gtt_dialog_grab_focus (GttDialog *dialog, gint button)
{
  GList *list;

  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTT_IS_DIALOG (dialog));

  list = g_list_nth (dialog->buttons, button);

  if (list && list->data)
    {
      gtk_widget_grab_focus (GTK_WIDGET (list->data));
      return;
    }
#ifdef GTT_ENABLE_DEBUG
  /* If we didn't find the button, complain */
  g_warning ("Button number %d does not appear to exist\n", button);
#endif
}

/**
 * gtt_dialog_set_close: Whether to call gtt_dialog_close() when a button
 *is clicked.
 * @dialog: #GttDialog to affect.
 * @click_closes: TRUE if clicking any button should call gtt_dialog_close().
 *
 * This is a convenience function so you don't have to connect callbacks
 * to each button just to close the dialog. By default, #GttDialog
 * has this parameter set the FALSE and it will not close on any click.
 * (This was a design error.) However, almost all the #GttDialog subclasses,
 * such as #GnomeMessageBox and #GnomePropertyBox, have this parameter set to
 * TRUE by default.
 *
 **/
void
gtt_dialog_set_close (GttDialog *dialog, gboolean click_closes)
{
  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTT_IS_DIALOG (dialog));

  dialog->click_closes = click_closes;
}

/**
 * gtt_dialog_close_hides: gtt_dialog_close() can destroy or hide the
 *dialog; toggle this behavior.
 * @dialog: #GttDialog to affect.
 * @just_hide: If TRUE, gtt_dialog_close() calls gtk_widget_hide() instead of
 *gtk_widget_destroy().
 *
 * Some dialogs are expensive to create, so you want to keep them around and
 *just gtk_widget_show() them when they are opened, and gtk_widget_hide() them
 *when they're closed. Other dialogs are expensive to keep around, so you want
 *to gtk_widget_destroy() them when they're closed. It's a judgment call you
 * will need to make for each dialog.
 *
 **/
void
gtt_dialog_close_hides (GttDialog *dialog, gboolean just_hide)
{
  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTT_IS_DIALOG (dialog));

  dialog->just_hide = just_hide;
}

/**
 * gtt_dialog_set_sensitive: Set the sensitivity of a button.
 * @dialog: #GttDialog to affect.
 * @button: Which button to affect.
 * @setting: TRUE means it's sensitive.
 *
 * Calls gtk_widget_set_sensitive() on the specified button number.
 *
 **/
void
gtt_dialog_set_sensitive (GttDialog *dialog, gint button, gboolean setting)
{
  GList *list;

  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTT_IS_DIALOG (dialog));

  list = g_list_nth (dialog->buttons, button);

  if (list && list->data)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (list->data), setting);
      return;
    }
#ifdef GTT_ENABLE_DEBUG
  /* If we didn't find the button, complain */
  g_warning ("Button number %d does not appear to exist\n", button);
#endif
}

/**
 * gtt_dialog_button_connect: Connect a callback to one of the button's
 *"clicked" signals.
 * @dialog: #GttDialog to affect.
 * @button: Button number.
 * @callback: A standard Gtk callback.
 * @data: Callback data.
 *
 * Simply g_signal_connect() to the "clicked" signal of the specified button.
 *
 **/
void
gtt_dialog_button_connect (GttDialog *dialog, gint button, GCallback callback,
                           gpointer data)
{
  GList *list;

  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTT_IS_DIALOG (dialog));

  list = g_list_nth (dialog->buttons, button);

  if (list && list->data)
    {
      g_signal_connect (list->data, "clicked", callback, data);
      return;
    }
#ifdef GTT_ENABLE_DEBUG
  /* If we didn't find the button, complain */
  g_warning ("Button number %d does not appear to exist\n", button);
#endif
}

/**
 * gtt_dialog_button_connect_object: g_signal_connect_swapped() to a button.
 * @dialog: #GttDialog to affect.
 * @button: Button to connect to.
 * @callback: Callback.
 * @obj: As for g_signal_connect_swapped().
 *
 * g_signal_connect_swapped() to the "clicked" signal of the given button.
 *
 **/
void
gtt_dialog_button_connect_object (GttDialog *dialog, gint button,
                                  GCallback callback, GtkObject *obj)
{
  GList *list;

  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTT_IS_DIALOG (dialog));

  list = g_list_nth (dialog->buttons, button);

  if (list && list->data)
    {
      g_signal_connect_swapped (list->data, "clicked", callback, obj);
      return;
    }
#ifdef GTT_ENABLE_DEBUG
  /* If we didn't find the button, complain */
  g_warning ("Button number %d does not appear to exist\n", button);
#endif
}

/**
 * gtt_dialog_set_accelerator:
 * @dialog: #GttDialog to affect.
 * @button: Button number.
 * @accelerator_key: Key for the accelerator.
 * @accelerator_mods: Modifier.
 *
 * Set an accelerator key for a button.
 **/
void
gtt_dialog_set_accelerator (GttDialog *dialog, gint button,
                            const guchar accelerator_key,
                            guint8 accelerator_mods)
{
  GList *list;

  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTT_IS_DIALOG (dialog));

  list = g_list_nth (dialog->buttons, button);

  if (list && list->data)
    {
      /*FIXME*/
      gtk_widget_add_accelerator (GTK_WIDGET (list->data), "clicked",
                                  dialog->accelerators, accelerator_key,
                                  accelerator_mods, GTK_ACCEL_VISIBLE);

      return;
    }
#ifdef GTT_ENABLE_DEBUG
  /* If we didn't find the button, complain */
  g_warning ("Button number %d does not appear to exist\n", button);
#endif
}

/**
 * gtt_dialog_editable_enters: Make the "activate" signal of an editable
 *click the default dialog button.
 * @dialog: #GttDialog to affect.
 * @editable: Editable to affect.
 *
 * Normally if there's an editable widget (such as #GtkEntry) in your
 * dialog, pressing Enter will activate the editable rather than the
 * default dialog button. However, in most cases, the user expects to
 * type something in and then press enter to close the dialog. This
 * function enables that behavior.
 *
 **/
void
gtt_dialog_editable_enters (GttDialog *dialog, GtkEditable *editable)
{
  g_return_if_fail (dialog != NULL);
  g_return_if_fail (editable != NULL);
  g_return_if_fail (GTT_IS_DIALOG (dialog));
  g_return_if_fail (GTK_IS_EDITABLE (editable));

  g_signal_connect_swapped (editable, "activate",
                            G_CALLBACK (gtk_window_activate_default), dialog);
}

static void
gtt_dialog_button_clicked (GtkWidget *button, GtkWidget *dialog)
{
  GList *list;
  int which = 0;

  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTT_IS_DIALOG (dialog));

  list = GTT_DIALOG (dialog)->buttons;

  while (list)
    {
      if (list->data == button)
        {
          gboolean click_closes;

          click_closes = GTT_DIALOG (dialog)->click_closes;

          g_signal_emit (dialog, dialog_signals[CLICKED], 0, which);

          /* The dialog may have been destroyed by the clicked signal, which
             is why we had to save the click_closes flag.  Users should be
             careful not to set click_closes and then destroy the dialog
             themselves too. */

          if (click_closes)
            {
              gtt_dialog_close (GTT_DIALOG (dialog));
            }

          /* Dialog may now be destroyed... */
          break;
        }
      list = list->next;
      ++which;
    }
}

static gint
gtt_dialog_key_pressed (GtkWidget *d, GdkEventKey *e)
{
  g_return_val_if_fail (GTT_IS_DIALOG (d), TRUE);

  if (e->keyval == GDK_Escape)
    {
      gtt_dialog_close (GTT_DIALOG (d));

      return TRUE; /* Stop the event? is this TRUE or FALSE? */
    }

  /* Have to call parent's handler, or the widget wouldn't get any
     key press events. Note that this is NOT done if the dialog
     may have been destroyed. */
  if (GTK_WIDGET_CLASS (gtt_dialog_parent_class)->key_press_event)
    return (*(GTK_WIDGET_CLASS (gtt_dialog_parent_class)->key_press_event)) (
        d, e);
  else
    return FALSE; /* Not handled. */
}

static gint
gtt_dialog_delete_event (GtkWidget *d, GdkEventAny *e)
{
  gtt_dialog_close (GTT_DIALOG (d));
  return TRUE; /* We handled it. */
}

static void
gtt_dialog_destroy (GtkObject *object)
{
  GttDialog *dialog;

  /* remember, destroy can be run multiple times! */

  g_return_if_fail (object != NULL);
  g_return_if_fail (GTT_IS_DIALOG (object));

  dialog = GTT_DIALOG (object);

  if (dialog->buttons)
    g_list_free (dialog->buttons);
  dialog->buttons = NULL;

  if (GTK_OBJECT_CLASS (gtt_dialog_parent_class)->destroy)
    (*(GTK_OBJECT_CLASS (gtt_dialog_parent_class)->destroy)) (object);
}

static void
gtt_dialog_finalize (GObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (GTT_IS_DIALOG (object));

  if (G_OBJECT_CLASS (gtt_dialog_parent_class)->finalize)
    (*(G_OBJECT_CLASS (gtt_dialog_parent_class)->finalize)) (object);
}

static void
gtt_dialog_close_real (GttDialog *dialog)
{
  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTT_IS_DIALOG (dialog));

  gtk_widget_hide (GTK_WIDGET (dialog));

  if (!dialog->just_hide)
    {
      gtk_widget_destroy (GTK_WIDGET (dialog));
    }
}

/**
 * gtt_dialog_close: Close (hide or destroy) the dialog.
 * @dialog: #GttDialog to close.
 *
 * See also gtt_dialog_close_hides(). This function emits the
 * "close" signal, which either hides or destroys the dialog (destroy
 * by default). If you connect to the "close" signal, and your
 * callback returns TRUE, the hide or destroy will be blocked. You can
 * do this to avoid closing the dialog if the user gives invalid
 * input, for example.
 *
 * Using gtt_dialog_close() in place of gtk_widget_hide() or
 * gtk_widget_destroy() allows you to easily catch all sources of
 * dialog closure, including delete_event and button clicks, and
 * handle them in a central location.
 **/
void
gtt_dialog_close (GttDialog *dialog)
{
  gint close_handled = FALSE;

  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTT_IS_DIALOG (dialog));

  g_signal_emit (dialog, dialog_signals[CLOSE], 0, &close_handled);

  if (!close_handled)
    {
      gtt_dialog_close_real (dialog);
    }
}

static void
gtt_dialog_show (GtkWidget *d)
{
  if (GTK_WIDGET_CLASS (gtt_dialog_parent_class)->show)
    (*(GTK_WIDGET_CLASS (gtt_dialog_parent_class)->show)) (d);
}

#endif /* GTT_DISABLE_DEPRECATED_SOURCE */
