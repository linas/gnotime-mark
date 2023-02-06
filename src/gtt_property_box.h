/* gtt-property_box.h - Property dialog box.

   Copyright (C) 1998 Tom Tromey
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

#ifndef GTT_PROPERTY_BOX_H
#define GTT_PROPERTY_BOX_H

#include "gtt_dialog.h"

#ifndef GTT_DISABLE_DEPRECATED

G_BEGIN_DECLS

#define GTT_TYPE_PROPERTY_BOX (gtt_property_box_get_type ())
#define GTT_PROPERTY_BOX(obj)                                                 \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTT_TYPE_PROPERTY_BOX, GttPropertyBox))
#define GTT_PROPERTY_BOX_CLASS(klass)                                         \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GTT_TYPE_PROPERTY_BOX,                   \
                            GttPropertyBoxClass))
#define GTT_IS_PROPERTY_BOX(obj)                                              \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTT_TYPE_PROPERTY_BOX))
#define GTT_IS_PROPERTY_BOX_CLASS(klass)                                      \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GTT_TYPE_PROPERTY_BOX))
#define GTT_PROPERTY_BOX_GET_CLASS(obj)                                       \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTT_TYPE_PROPERTY_BOX,                   \
                              GttPropertyBoxClass))

/*the flag used on the notebook pages to see if a change happened on a certain
 * page or not*/
#define GTT_PROPERTY_BOX_DIRTY "gtt_property_box_dirty"

typedef struct _GttPropertyBox GttPropertyBox;
typedef struct _GttPropertyBoxClass GttPropertyBoxClass;

/**
 * GttPropertyBox:
 *
 * An opaque widget representing a property box. Items should be added to this
 * widget using gtt_property_box_append_page.
 */
struct _GttPropertyBox
{
  /*< private >*/
  GttDialog dialog;

  GtkWidget *notebook;      /* The notebook widget.  */
  GtkWidget *ok_button;     /* OK button.  */
  GtkWidget *apply_button;  /* Apply button.  */
  GtkWidget *cancel_button; /* Cancel/Close button.  */
  GtkWidget *help_button;   /* Help button.  */

  gpointer reserved; /* Reserved for a future private pointer if necessary */
};

struct _GttPropertyBoxClass
{
  GttDialogClass parent_class;

  void (*apply) (GttPropertyBox *propertybox, gint page_num);
  void (*help) (GttPropertyBox *propertybox, gint page_num);
};

GType gtt_property_box_get_type (void) G_GNUC_CONST;
GtkWidget *gtt_property_box_new (void);

/*
 * Call this when the user changes something in the current page of
 * the notebook.
 */
void gtt_property_box_changed (GttPropertyBox *property_box);

void gtt_property_box_set_modified (GttPropertyBox *property_box,
                                    gboolean state);

gint gtt_property_box_append_page (GttPropertyBox *property_box,
                                   GtkWidget *child, GtkWidget *tab_label);

/* Deprecated, use set_modified */
void gtt_property_box_set_state (GttPropertyBox *property_box, gboolean state);

G_END_DECLS

#endif /* GTT_DISABLE_DEPRECATED */

#endif /* GTT_PROPERTY_BOX_H */
