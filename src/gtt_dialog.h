/* GNOME GUI Library
 * Copyright (C) 1995-1998 Jay Painter
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

#ifndef GTT_DIALOG_H
#define GTT_DIALOG_H

#ifndef GTT_DISABLE_DEPRECATED

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include <stdarg.h>

G_BEGIN_DECLS

#define GTT_TYPE_DIALOG (gtt_dialog_get_type ())
#define GTT_DIALOG(obj)                                                       \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTT_TYPE_DIALOG, GttDialog))
#define GTT_DIALOG_CLASS(klass)                                               \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GTT_TYPE_DIALOG, GttDialogClass))
#define GTT_IS_DIALOG(obj)                                                    \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTT_TYPE_DIALOG))
#define GTT_IS_DIALOG_CLASS(klass)                                            \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GTT_TYPE_DIALOG))
#define GTT_DIALOG_GET_CLASS(obj)                                             \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTT_TYPE_DIALOG, GttDialogClass))

typedef struct _GttDialog GttDialog;
typedef struct _GttDialogClass GttDialogClass;

/**
 * GttDialog:
 * @vbox: The middle portion of the dialog box.
 *
 * Client code can pack widgets (for example, text or images) into @vbox.
 */
struct _GttDialog
{
  GtkWindow window;
  /*< public >*/
  GtkWidget *vbox;
  /*< private >*/
  GList *buttons;
  GtkWidget *action_area; /* A button box, not an hbox */

  GtkAccelGroup *accelerators;

  unsigned int click_closes : 1;
  unsigned int just_hide : 1;
};

struct _GttDialogClass
{
  GtkWindowClass parent_class;

  void (*clicked) (GttDialog *dialog, gint button_number);
  gboolean (*close) (GttDialog *dialog);
};

/* GttDialog creates an action area with the buttons of your choice.
   You should pass the button names (possibly GTT_STOCK_BUTTON_*) as
   arguments to gtt_dialog_new(). The buttons are numbered in the
   order you passed them in, starting at 0. These numbers are used
   in other functions, and passed to the "clicked" callback. */

GType gtt_dialog_get_type (void) G_GNUC_CONST;

/* Arguments: Title and button names, then NULL */
GtkWidget *gtt_dialog_new (const gchar *title, ...);
/* Arguments: Title and NULL terminated array of button names. */
GtkWidget *gtt_dialog_newv (const gchar *title, const gchar **buttons);

/* For now this just means the dialog can be centered over
   its parent. */
void gtt_dialog_set_parent (GttDialog *dialog, GtkWindow *parent);

/* Note: it's better to use GttDialog::clicked rather than
   connecting to a button. These are really here in case
   you're lazy. */
/* Connect to the "clicked" signal of a single button */
void gtt_dialog_button_connect (GttDialog *dialog, gint button,
                                GCallback callback, gpointer data);
/* Connect the object to the "clicked" signal of a single button */
void gtt_dialog_button_connect_object (GttDialog *dialog, gint button,
                                       GCallback callback, GtkObject *obj);

/* Run the dialog, return the button # that was pressed or -1 if none.
   (this sets the dialog modal while it blocks)
 */
gint gtt_dialog_run (GttDialog *dialog);
gint gtt_dialog_run_and_close (GttDialog *dialog);

/* Set the default button. - it will have a little highlight,
   and pressing return will activate it. */
void gtt_dialog_set_default (GttDialog *dialog, gint button);

/* Makes the nth button the focused widget in the dialog */
void gtt_dialog_grab_focus (GttDialog *dialog, gint button);
/* Set sensitivity of a button */
void gtt_dialog_set_sensitive (GttDialog *dialog, gint button,
                               gboolean setting);

/* Set the accelerator for a button. Note that there are two
   default accelerators: "Return" will be the same as clicking
   the default button, and "Escape" will emit delete_event.
   (Note: neither of these is in the accelerator table,
   Return is a Gtk default and Escape comes from a key press event
   handler.) */
void gtt_dialog_set_accelerator (GttDialog *dialog, gint button,
                                 const guchar accelerator_key,
                                 guint8 accelerator_mods);

/* Hide and optionally destroy. Destroys by default, use close_hides()
   to change this. */
void gtt_dialog_close (GttDialog *dialog);

/* Make _close just hide, not destroy. */
void gtt_dialog_close_hides (GttDialog *dialog, gboolean just_hide);

/* Whether to close after emitting clicked signal - default is
   FALSE. If clicking *any* button should close the dialog, set it to
   TRUE.  */
void gtt_dialog_set_close (GttDialog *dialog, gboolean click_closes);

/* Normally an editable widget will grab "Return" and keep it from
   activating the dialog's default button. This connects the activate
   signal of the editable to the default button. */
void gtt_dialog_editable_enters (GttDialog *dialog, GtkEditable *editable);

/* Use of append_buttons is discouraged, it's really
   meant for subclasses. */
void gtt_dialog_append_buttons (GttDialog *dialog, const gchar *first, ...);
void gtt_dialog_append_button (GttDialog *dialog, const gchar *button_name);
void gtt_dialog_append_buttonsv (GttDialog *dialog, const gchar **buttons);

/* Add button with arbitrary text and pixmap. */
void gtt_dialog_append_button_with_pixmap (GttDialog *dialog,
                                           const gchar *button_name,
                                           const gchar *pixmap_name);
void gtt_dialog_append_buttons_with_pixmaps (GttDialog *dialog,
                                             const gchar **names,
                                             const gchar **pixmaps);

/* Don't use this either; it's for bindings to languages other
   than C (which makes the varargs kind of lame... feel free to fix)
   You want _new, see above. */
void gtt_dialog_construct (GttDialog *dialog, const gchar *title, va_list ap);
void gtt_dialog_constructv (GttDialog *dialog, const gchar *title,
                            const gchar **buttons);

/* Stock defines for compatibility, not to be used
 * in new applications, please see gtk stock icons
 * and you should use those instead. */
#define GTT_STOCK_BUTTON_OK GTK_STOCK_OK
#define GTT_STOCK_BUTTON_CANCEL GTK_STOCK_CANCEL
#define GTT_STOCK_BUTTON_YES GTK_STOCK_YES
#define GTT_STOCK_BUTTON_NO GTK_STOCK_NO
#define GTT_STOCK_BUTTON_CLOSE GTK_STOCK_CLOSE
#define GTT_STOCK_BUTTON_APPLY GTK_STOCK_APPLY
#define GTT_STOCK_BUTTON_HELP GTK_STOCK_HELP
#define GTT_STOCK_BUTTON_NEXT GTK_STOCK_GO_FORWARD
#define GTT_STOCK_BUTTON_PREV GTK_STOCK_GO_BACK
#define GTT_STOCK_BUTTON_UP GTK_STOCK_GO_UP
#define GTT_STOCK_BUTTON_DOWN GTK_STOCK_GO_DOWN
#define GTT_STOCK_BUTTON_FONT GTK_STOCK_SELECT_FONT

G_END_DECLS

#endif /* GTT_DISABLE_DEPRECATED */

#endif /* GTT_DIALOG_H */
