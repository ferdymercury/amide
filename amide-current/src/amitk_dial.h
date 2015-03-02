/* amitk_dial.h, adapted from gtkdial.h */

/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef __AMITK_DIAL_H__
#define __AMITK_DIAL_H__


#include <gtk/gtk.h>


G_BEGIN_DECLS

#define AMITK_DIAL(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, amitk_dial_get_type (), AmitkDial)
#define AMITK_DIAL_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, amitk_dial_get_type (), AmitkDialClass)
#define AMITK_IS_DIAL(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, amitk_dial_get_type ())


typedef struct _AmitkDial        AmitkDial;
typedef struct _AmitkDialClass   AmitkDialClass;

struct _AmitkDial
{
  GtkWidget widget;

  /* update policy (GTK_UPDATE_[CONTINUOUS/DELAYED/DISCONTINUOUS]) */
  guint policy : 2;

  /* Button currently pressed or 0 if none */
  guint8 button;

  /* Dimensions of dial components */
  gint radius;
  gint pointer_width;

  /* ID of update timer, or 0 if none */
  guint32 timer;

  /* Current angle */
  gfloat angle;
  gfloat last_angle;

  /* Old values from adjustment stored so we know when something changes */
  gfloat old_value;
  gfloat old_lower;
  gfloat old_upper;

  /* The adjustment object that stores the data for this dial */
  GtkAdjustment *adjustment;
};

struct _AmitkDialClass
{
  GtkWidgetClass parent_class;
};


GtkWidget*     amitk_dial_new                    (GtkAdjustment *adjustment);
GType          amitk_dial_get_type               (void);
GtkAdjustment* amitk_dial_get_adjustment         (AmitkDial      *dial);
void           amitk_dial_set_update_policy      (AmitkDial      *dial,
						GtkUpdateType  policy);

void           amitk_dial_set_adjustment         (AmitkDial      *dial,
						GtkAdjustment *adjustment);

G_END_DECLS

#endif /* __AMITK_DIAL_H__ */
