/* This is the GTK 2 GtkGammaCurve widget, ported to GTK 3.
   Incorporated in AMIDE by Yavor Doganov <yavor@gnu.org>, 2024.  */

/* GTK - The GIMP Toolkit
 * Copyright (C) 1997 David Mosberger
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __AMITK_GAMMA_CURVE_H__
#define __AMITK_GAMMA_CURVE_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define AMITK_TYPE_GAMMA_CURVE            (amitk_gamma_curve_get_type ())
#define AMITK_GAMMA_CURVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), AMITK_TYPE_GAMMA_CURVE, AmitkGammaCurve))
#define AMITK_GAMMA_CURVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_GAMMA_CURVE, AmitkGammaCurveClass))
#define AMITK_IS_GAMMA_CURVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AMITK_TYPE_GAMMA_CURVE))
#define AMITK_IS_GAMMA_CURVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_GAMMA_CURVE))
#define AMITK_GAMMA_CURVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), AMITK_TYPE_GAMMA_CURVE, AmitkGammaCurveClass))

typedef struct _AmitkGammaCurve      AmitkGammaCurve;
typedef struct _AmitkGammaCurveClass AmitkGammaCurveClass;


struct _AmitkGammaCurve
{
  GtkBox vbox;

  GtkWidget *grid;
  GtkWidget *curve;
  GtkWidget *button[3]; /* spline, linear, reset */
};

struct _AmitkGammaCurveClass
{
  GtkBoxClass parent_class;
};

GType      amitk_gamma_curve_get_type (void) G_GNUC_CONST;
GtkWidget* amitk_gamma_curve_new      (void);

G_END_DECLS

#endif /* __AMITK_GAMMA_CURVE_H__ */
