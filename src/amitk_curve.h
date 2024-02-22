/* This is the GTK 2 GtkCurve widget, ported to GTK 3.
   Incorporated in AMIDE by Yavor Doganov <yavor@gnu.org>, 2024.  */

/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#ifndef __AMITK_CURVE_H__
#define __AMITK_CURVE_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define AMITK_TYPE_CURVE_TYPE           (amitk_curve_type_get_type ())

#define AMITK_TYPE_CURVE                (amitk_curve_get_type ())
#define AMITK_CURVE(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), AMITK_TYPE_CURVE, AmitkCurve))
#define AMITK_CURVE_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_CURVE, AmitkCurveClass))
#define AMITK_IS_CURVE(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AMITK_TYPE_CURVE))
#define AMITK_IS_CURVE_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_CURVE))
#define AMITK_CURVE_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), AMITK_TYPE_CURVE, AmitkCurveClass))

typedef struct _AmitkCurve      AmitkCurve;
typedef struct _AmitkCurveClass AmitkCurveClass;

typedef enum
{
  AMITK_CURVE_TYPE_LINEAR,     /* linear interpolation */
  AMITK_CURVE_TYPE_SPLINE,     /* spline interpolation */
} AmitkCurveType;

struct _AmitkCurve
{
  GtkDrawingArea graph;

  gint cursor_type;
  gfloat min_x;
  gfloat max_x;
  gfloat min_y;
  gfloat max_y;
  cairo_surface_t *surface;
  AmitkCurveType curve_type;
  gint height;                  /* (cached) graph height in pixels */
  gint grab_point;              /* point currently grabbed */
  gint last;

  /* (cached) curve points: */
  gint num_points;
  GdkPoint *point;

  /* control points: */
  gint num_ctlpoints;           /* number of control points */
  gfloat (*ctlpoint)[2];        /* array of control points */
};

struct _AmitkCurveClass
{
  GtkDrawingAreaClass parent_class;

  void (* curve_type_changed) (AmitkCurve *curve);
};

GType           amitk_curve_type_get_type  (void) G_GNUC_CONST;
GType           amitk_curve_get_type       (void) G_GNUC_CONST;
GtkWidget *     amitk_curve_new            (void);
void            amitk_curve_reset          (AmitkCurve *curve);
void            amitk_curve_set_range      (AmitkCurve *curve,
                                            gfloat min_x, gfloat max_x,
                                            gfloat min_y, gfloat max_y);
void            amitk_curve_get_vector     (AmitkCurve *curve,
                                            int veclen, gfloat vector[]);
void            amitk_curve_set_curve_type (AmitkCurve *curve,
                                            AmitkCurveType type);

G_END_DECLS

#endif /* __AMITK_CURVE_H__ */

