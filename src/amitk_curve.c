/* This is the GTK 2 GtkCurve widget, ported to GTK 3.
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

#include <math.h>
#include "amitk_curve.h"

#define RADIUS          3       /* radius of the control points */
#define MIN_DISTANCE    8       /* min distance between control points */

#define GRAPH_MASK       (GDK_EXPOSURE_MASK |            \
                          GDK_POINTER_MOTION_MASK |      \
                          GDK_POINTER_MOTION_HINT_MASK | \
                          GDK_ENTER_NOTIFY_MASK |        \
                          GDK_BUTTON_PRESS_MASK |        \
                          GDK_BUTTON_RELEASE_MASK |      \
                          GDK_BUTTON1_MOTION_MASK)

enum {
  PROP_0,
  PROP_CURVE_TYPE,
  PROP_MIN_X,
  PROP_MAX_X,
  PROP_MIN_Y,
  PROP_MAX_Y
};

static GtkDrawingAreaClass *parent_class = NULL;
static guint curve_type_changed_signal = 0;

/* forward declarations: */
static void amitk_curve_class_init       (AmitkCurveClass *class);
static void amitk_curve_init             (AmitkCurve      *curve);
static void amitk_curve_get_property     (GObject         *object,
                                          guint            param_id,
                                          GValue          *value,
                                          GParamSpec      *pspec);
static void amitk_curve_set_property     (GObject         *object,
                                          guint            param_id,
                                          const GValue    *value,
                                          GParamSpec      *pspec);
static void amitk_curve_finalize         (GObject         *object);
static gboolean amitk_curve_graph_events (GtkWidget       *widget,
                                          GdkEvent        *event,
                                          AmitkCurve      *c);
static gboolean amitk_curve_draw_real    (GtkWidget       *widget,
                                          cairo_t         *cr,
                                          AmitkCurve      *c);
static void amitk_curve_state_changed    (GtkWidget       *widget,
                                          GtkStateFlags    flags,
                                          AmitkCurve      *c);
static void amitk_curve_size_graph       (AmitkCurve      *curve);

GType
amitk_curve_type_get_type (void)
{
    static GType etype = 0;
    if (G_UNLIKELY(etype == 0))
      {
        static const GEnumValue values[] = {
          { AMITK_CURVE_TYPE_LINEAR, "AMITK_CURVE_TYPE_LINEAR", "linear" },
          { AMITK_CURVE_TYPE_SPLINE, "AMITK_CURVE_TYPE_SPLINE", "spline" },
          { 0, NULL, NULL }
        };
        etype = g_enum_register_static (g_intern_static_string
                                        ("AmikCurveType"), values);
    }
    return etype;
}

GType
amitk_curve_get_type (void)
{
  static GType curve_type = 0;

  if (!curve_type)
    {
      const GTypeInfo curve_info =
        {
          sizeof (AmitkCurveClass),
          NULL,            /* base_init */
          NULL,            /* base_finalize */
          (GClassInitFunc) amitk_curve_class_init,
          NULL,            /* class_finalize */
          NULL,            /* class_data */
          sizeof (AmitkCurve),
          0,               /* n_preallocs */
          (GInstanceInitFunc) amitk_curve_init,
        };

      curve_type = g_type_register_static (GTK_TYPE_DRAWING_AREA, "AmitkCurve",
                                           &curve_info, 0);
    }
  return curve_type;
}

static void
amitk_curve_class_init (AmitkCurveClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  gobject_class->finalize = amitk_curve_finalize;

  gobject_class->set_property = amitk_curve_set_property;
  gobject_class->get_property = amitk_curve_get_property;

  g_object_class_install_property (gobject_class,
                                   PROP_CURVE_TYPE,
                                   g_param_spec_enum ("curve-type",
                                                      "Curve type",
                                                      "Is this curve linear or spline interpolated",
                                                      AMITK_TYPE_CURVE_TYPE,
                                                      AMITK_CURVE_TYPE_SPLINE,
                                                      G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_MIN_X,
                                   g_param_spec_float ("min-x",
                                                       "Minimum X",
                                                       "Minimum possible value for X",
                                                       -G_MAXFLOAT,
                                                       G_MAXFLOAT,
                                                       0.0,
                                                       G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_MAX_X,
                                   g_param_spec_float ("max-x",
                                                       "Maximum X",
                                                       "Maximum possible X value",
                                                       -G_MAXFLOAT,
                                                       G_MAXFLOAT,
                                                       1.0,
                                                       G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_MIN_Y,
                                   g_param_spec_float ("min-y",
                                                       "Minimum Y",
                                                       "Minimum possible value for Y",
                                                       -G_MAXFLOAT,
                                                       G_MAXFLOAT,
                                                       0.0,
                                                       G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class,
                                   PROP_MAX_Y,
                                   g_param_spec_float ("max-y",
                                                       "Maximum Y",
                                                       "Maximum possible value for Y",
                                                       -G_MAXFLOAT,
                                                       G_MAXFLOAT,
                                                       1.0,
                                                       G_PARAM_READWRITE));

  curve_type_changed_signal =
    g_signal_new ("curve-type-changed",
                   G_OBJECT_CLASS_TYPE (gobject_class),
                   G_SIGNAL_RUN_FIRST,
                   G_STRUCT_OFFSET (AmitkCurveClass, curve_type_changed),
                   NULL, NULL,
                   g_cclosure_marshal_VOID__VOID,
                   G_TYPE_NONE, 0);
}

static void
amitk_curve_init (AmitkCurve *curve)
{
  gint old_mask;

  curve->cursor_type = GDK_TOP_LEFT_ARROW;
  curve->surface = NULL;
  curve->curve_type = AMITK_CURVE_TYPE_SPLINE;
  curve->height = 0;
  curve->grab_point = -1;

  curve->num_points = 0;
  curve->point = NULL;

  curve->num_ctlpoints = 0;
  curve->ctlpoint = NULL;

  curve->min_x = 0.0;
  curve->max_x = 1.0;
  curve->min_y = 0.0;
  curve->max_y = 1.0;

  old_mask = gtk_widget_get_events (GTK_WIDGET (curve));
  gtk_widget_set_events (GTK_WIDGET (curve), old_mask | GRAPH_MASK);
  g_signal_connect (curve, "event",
                    G_CALLBACK (amitk_curve_graph_events), curve);
  g_signal_connect (curve, "state-flags-changed",
                    G_CALLBACK (amitk_curve_state_changed), curve);
  g_signal_connect (curve, "draw", G_CALLBACK (amitk_curve_draw_real), curve);
  /* Setting the "hexpand" property has to be done before the first
     size_request call (by amitk_curve_size_graph); the second graph
     is computed with shorter width and there is no way to expand it
     in the GtkGrid.  Setting it afterwards has no effect.  */
  gtk_widget_set_hexpand (GTK_WIDGET (curve), TRUE);
  amitk_curve_size_graph (curve);
}

static void
amitk_curve_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  AmitkCurve *curve = AMITK_CURVE (object);

  switch (prop_id)
    {
    case PROP_CURVE_TYPE:
      amitk_curve_set_curve_type (curve, g_value_get_enum (value));
      break;
    case PROP_MIN_X:
      amitk_curve_set_range (curve, g_value_get_float (value), curve->max_x,
                             curve->min_y, curve->max_y);
      break;
    case PROP_MAX_X:
      amitk_curve_set_range (curve, curve->min_x, g_value_get_float (value),
                             curve->min_y, curve->max_y);
      break;
    case PROP_MIN_Y:
      amitk_curve_set_range (curve, curve->min_x, curve->max_x,
                             g_value_get_float (value), curve->max_y);
      break;
    case PROP_MAX_Y:
      amitk_curve_set_range (curve, curve->min_x, curve->max_x,
                             curve->min_y, g_value_get_float (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
amitk_curve_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  AmitkCurve *curve = AMITK_CURVE (object);

  switch (prop_id)
    {
    case PROP_CURVE_TYPE:
      g_value_set_enum (value, curve->curve_type);
      break;
    case PROP_MIN_X:
      g_value_set_float (value, curve->min_x);
      break;
    case PROP_MAX_X:
      g_value_set_float (value, curve->max_x);
      break;
    case PROP_MIN_Y:
      g_value_set_float (value, curve->min_y);
      break;
    case PROP_MAX_Y:
      g_value_set_float (value, curve->max_y);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static int
project (gfloat value, gfloat min, gfloat max, int norm)
{
  return (norm - 1) * ((value - min) / (max - min)) + 0.5;
}

static gfloat
unproject (gint value, gfloat min, gfloat max, int norm)
{
  return value / (gfloat) (norm - 1) * (max - min) + min;
}

/* Solve the tridiagonal equation system that determines the second
   derivatives for the interpolation points.  (Based on Numerical
   Recipies 2nd Edition.) */
static void
spline_solve (int n, gfloat x[], gfloat y[], gfloat y2[])
{
  gfloat p, sig, *u;
  gint i, k;

  u = g_malloc ((n - 1) * sizeof (u[0]));

  y2[0] = u[0] = 0.0;   /* set lower boundary condition to "natural" */

  for (i = 1; i < n - 1; ++i)
    {
      sig = (x[i] - x[i - 1]) / (x[i + 1] - x[i - 1]);
      p = sig * y2[i - 1] + 2.0;
      y2[i] = (sig - 1.0) / p;
      u[i] = ((y[i + 1] - y[i])
              / (x[i + 1] - x[i]) - (y[i] - y[i - 1]) / (x[i] - x[i - 1]));
      u[i] = (6.0 * u[i] / (x[i + 1] - x[i - 1]) - sig * u[i - 1]) / p;
    }

  y2[n - 1] = 0.0;
  for (k = n - 2; k >= 0; --k)
    y2[k] = y2[k] * y2[k + 1] + u[k];

  g_free (u);
}

static gfloat
spline_eval (int n, gfloat x[], gfloat y[], gfloat y2[], gfloat val)
{
  gint k_lo, k_hi, k;
  gfloat h, b, a;

  /* do a binary search for the right interval: */
  k_lo = 0; k_hi = n - 1;
  while (k_hi - k_lo > 1)
    {
      k = (k_hi + k_lo) / 2;
      if (x[k] > val)
        k_hi = k;
      else
        k_lo = k;
    }

  h = x[k_hi] - x[k_lo];
  g_assert (h > 0.0);

  a = (x[k_hi] - val) / h;
  b = (val - x[k_lo]) / h;
  return a*y[k_lo] + b*y[k_hi] +
    ((a*a*a - a)*y2[k_lo] + (b*b*b - b)*y2[k_hi]) * (h*h)/6.0;
}

static void
amitk_curve_interpolate (AmitkCurve *c, gint width, gint height)
{
  gfloat *vector;
  int i;

  vector = g_malloc (width * sizeof (vector[0]));

  amitk_curve_get_vector (c, width, vector);

  c->height = height;
  if (c->num_points != width)
    {
      c->num_points = width;
      g_free (c->point);
      c->point = g_malloc (c->num_points * sizeof (c->point[0]));
    }

  for (i = 0; i < width; ++i)
    {
      c->point[i].x = RADIUS + i;
      c->point[i].y = RADIUS + height
        - project (vector[i], c->min_y, c->max_y, height);
    }

  g_free (vector);
}

static void
amitk_curve_draw (AmitkCurve *c, gint width, gint height)
{
  GtkStateFlags state;
  GdkRGBA color;
  GtkStyleContext *ctxt;
  GtkWidget *w;
  cairo_t *cr;
  gdouble offset;
  gint i;

  g_return_if_fail (c->surface != NULL);

  if (c->height != height || c->num_points != width)
    amitk_curve_interpolate (c, width, height);

  w = GTK_WIDGET (c);

  /* Get the style context from the toplevel because this widget (and
     apparently its parent) do not have bacgkround set so
     gtk_render_background silently does nothing.  */
  ctxt = gtk_widget_get_style_context (gtk_widget_get_toplevel (w));
  state = gtk_style_context_get_state (ctxt);

  cr = cairo_create (c->surface);

  /* Erase what has been drawn during the last call.  */
  gtk_render_background (ctxt, cr, 0, 0,
                         width + RADIUS * 2, height + RADIUS * 2);

  ctxt = gtk_widget_get_style_context (w);
  state = gtk_style_context_get_state (ctxt);
  gtk_style_context_get_color (ctxt, state, &color);
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_set_line_width (cr, 1.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);

  /* Draw the grid lines.  Orignal code uses w->style->dark_gc[state]
     which makes them a bit lighter than the graph itself.  I guess we
     could pass modified GdkRGBA struct members to achieve the same
     effect but this looks risky with some setups/themes.  */
  for (i = 0; i < 5; i++)
    {
      if (i % 2)
        offset = 0;
      else
        offset = 0.5;

      cairo_move_to (cr, RADIUS, i * (height / 4.0) + RADIUS + offset);
      cairo_line_to (cr, width + RADIUS, i * (height / 4.0) + RADIUS + offset);
      cairo_move_to (cr, i * (width / 4.0) + RADIUS, RADIUS + 0.5);
      cairo_line_to (cr, i * (width / 4.0) + RADIUS, height + RADIUS);
    }
  cairo_stroke (cr);

  for (i = 0; i < c->num_points; i++)
    cairo_rectangle (cr, c->point[i].x, c->point[i].y, 1, 1);
  cairo_fill (cr);

  for (i = 0; i < c->num_ctlpoints; ++i)
    {
      gint x, y;

      if (c->ctlpoint[i][0] < c->min_x)
        continue;

      x = project (c->ctlpoint[i][0], c->min_x, c->max_x, width);
      y = height - project (c->ctlpoint[i][1], c->min_y, c->max_y, height);

      /* Draw the control point (a bullet).  */
      cairo_arc (cr, x + RADIUS, y + RADIUS, RADIUS, 0, 2 * M_PI);
      cairo_fill (cr);
    }

  cairo_surface_flush (c->surface);
  cairo_destroy (cr);
  gtk_widget_queue_draw (w);
}

static void
amitk_curve_state_changed (GtkWidget *widget, GtkStateFlags flags,
                           AmitkCurve *c)
{
  gint width, height;

  width = gtk_widget_get_allocated_width (widget) - RADIUS * 2;
  height = gtk_widget_get_allocated_height (widget) - RADIUS * 2;

  /* Trigger a redraw of the graph so that the widget is consitent
     with the rest of the widgets in the dialog window.  */
  amitk_curve_draw (c, width, height);
}

static gboolean
amitk_curve_draw_real (GtkWidget *widget, cairo_t *cr, AmitkCurve *c)
{
  cairo_set_source_surface (cr, c->surface, 0, 0);
  cairo_paint (cr);

  return FALSE;
}

static gboolean
amitk_curve_graph_events (GtkWidget *widget, GdkEvent *event, AmitkCurve *c)
{
  GdkCursorType new_type = c->cursor_type;
  gint i, src, dst, leftbound, rightbound;
  GdkSeat *seat;
  GtkWidget *w;
  gint tx, ty;
  gint cx, x, y, width, height;
  gint closest_point = 0;
  gfloat rx, ry, min_x;
  guint distance;
  gboolean retval = FALSE;

  w = GTK_WIDGET (c);
  width = gtk_widget_get_allocated_width (w) - RADIUS * 2;
  height = gtk_widget_get_allocated_height (w) - RADIUS * 2;

  if ((width < 0) || (height < 0))
    return FALSE;

  /*  Get the pointer position.  */
  seat = gdk_display_get_default_seat (gtk_widget_get_display (w));
  gdk_window_get_device_position (gtk_widget_get_window (w),
                                  gdk_seat_get_pointer (seat),
                                  &tx, &ty, NULL);
  x = CLAMP ((tx - RADIUS), 0, width - 1);
  y = CLAMP ((ty - RADIUS), 0, height - 1);

  min_x = c->min_x;

  distance = ~0U;
  for (i = 0; i < c->num_ctlpoints; ++i)
    {
      cx = project (c->ctlpoint[i][0], min_x, c->max_x, width);
      if ((guint) abs (x - cx) < distance)
        {
          distance = abs (x - cx);
          closest_point = i;
        }
    }

  switch (event->type)
    {
    case GDK_CONFIGURE:
      if (c->surface)
        cairo_surface_destroy (c->surface);
      c->surface = NULL;
      /* fall through */
    case GDK_EXPOSE:
      if (!c->surface)
        c->surface =
          cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                      gtk_widget_get_allocated_width (w),
                                      gtk_widget_get_allocated_height (w));
      amitk_curve_draw (c, width, height);
      break;

    case GDK_BUTTON_PRESS:
      gtk_grab_add (widget);

      new_type = GDK_TCROSS;

      if (distance > MIN_DISTANCE)
        {
          /* insert a new control point */
          if (c->num_ctlpoints > 0)
            {
              cx = project (c->ctlpoint[closest_point][0], min_x,
                            c->max_x, width);
              if (x > cx)
                ++closest_point;
            }
          ++c->num_ctlpoints;
          c->ctlpoint = g_realloc (c->ctlpoint,
                                   c->num_ctlpoints * sizeof (*c->ctlpoint));
          for (i = c->num_ctlpoints - 1; i > closest_point; --i)
            memcpy (c->ctlpoint + i, c->ctlpoint + i - 1,
                    sizeof (*c->ctlpoint));
        }
      c->grab_point = closest_point;
      c->ctlpoint[c->grab_point][0] =
        unproject (x, min_x, c->max_x, width);
      c->ctlpoint[c->grab_point][1] =
        unproject (height - y, c->min_y, c->max_y, height);

      amitk_curve_interpolate (c, width, height);

      amitk_curve_draw (c, width, height);
      retval = TRUE;
      break;

    case GDK_BUTTON_RELEASE:
      gtk_grab_remove (widget);

      /* delete inactive points: */
      for (src = dst = 0; src < c->num_ctlpoints; ++src)
        {
          if (c->ctlpoint[src][0] >= min_x)
            {
              memcpy (c->ctlpoint + dst, c->ctlpoint + src,
                      sizeof (*c->ctlpoint));
              ++dst;
            }
        }
      if (dst < src)
        {
          c->num_ctlpoints -= (src - dst);
          if (c->num_ctlpoints <= 0)
            {
              c->num_ctlpoints = 1;
              c->ctlpoint[0][0] = min_x;
              c->ctlpoint[0][1] = c->min_y;
              amitk_curve_interpolate (c, width, height);
              amitk_curve_draw (c, width, height);
            }
          c->ctlpoint = g_realloc (c->ctlpoint,
                                   c->num_ctlpoints * sizeof (*c->ctlpoint));
        }
      new_type = GDK_FLEUR;
      c->grab_point = -1;
      retval = TRUE;
      break;

    case GDK_MOTION_NOTIFY:
      if (c->grab_point == -1)
        {
          /* if no point is grabbed...  */
          if (distance <= MIN_DISTANCE)
            new_type = GDK_FLEUR;
          else
            new_type = GDK_TCROSS;
        }
      else
        {
          /* drag the grabbed point  */
          new_type = GDK_TCROSS;

          leftbound = -MIN_DISTANCE;
          if (c->grab_point > 0)
            leftbound = project (c->ctlpoint[c->grab_point - 1][0],
                                 min_x, c->max_x, width);

          rightbound = width + RADIUS * 2 + MIN_DISTANCE;
          if (c->grab_point + 1 < c->num_ctlpoints)
            rightbound = project (c->ctlpoint[c->grab_point + 1][0],
                                  min_x, c->max_x, width);

          if (tx <= leftbound || tx >= rightbound
              || ty > height + RADIUS * 2 + MIN_DISTANCE
              || ty < -MIN_DISTANCE)
            c->ctlpoint[c->grab_point][0] = min_x - 1.0;
          else
            {
              rx = unproject (x, min_x, c->max_x, width);
              ry = unproject (height - y, c->min_y, c->max_y, height);
              c->ctlpoint[c->grab_point][0] = rx;
              c->ctlpoint[c->grab_point][1] = ry;
            }
          amitk_curve_interpolate (c, width, height);
          amitk_curve_draw (c, width, height);
        }

      if (new_type != (GdkCursorType) c->cursor_type)
        {
          GdkCursor *cursor;

          c->cursor_type = new_type;

          cursor = gdk_cursor_new_for_display (gtk_widget_get_display (w),
                                               c->cursor_type);
          gdk_window_set_cursor (gtk_widget_get_window (w), cursor);
          g_object_unref (cursor);
        }
      retval = TRUE;
      break;

    default:
      break;
    }

  return retval;
}

void
amitk_curve_set_curve_type (AmitkCurve *c, AmitkCurveType new_type)
{
  gint width, height;

  if (new_type != c->curve_type)
    {
      width  = gtk_widget_get_allocated_width (GTK_WIDGET (c)) - RADIUS * 2;
      height = gtk_widget_get_allocated_height (GTK_WIDGET (c)) - RADIUS * 2;

      c->curve_type = new_type;
      amitk_curve_interpolate (c, width, height);

      g_signal_emit (c, curve_type_changed_signal, 0);
      g_object_notify (G_OBJECT (c), "curve-type");
      amitk_curve_draw (c, width, height);
    }
}

static void
amitk_curve_size_graph (AmitkCurve *curve)
{
  gint width, height;
  gfloat aspect;
  GdkRectangle geom;
  GdkMonitor *monitor;
  GtkWidget *w = GTK_WIDGET (curve);

  width  = (curve->max_x - curve->min_x) + 1;
  height = (curve->max_y - curve->min_y) + 1;
  aspect = width / (gfloat) height;
  monitor = gdk_display_get_primary_monitor (gdk_display_get_default ());
  gdk_monitor_get_geometry (monitor, &geom);
  if (width > geom.width / 4)
    width  = geom.width / 4;
  if (height > geom.height / 4)
    height = geom.height / 4;

  if (aspect < 1.0)
    width  = height * aspect;
  else
    height = width / aspect;

  gtk_widget_set_size_request (w, width + RADIUS * 2, height + RADIUS * 2);
}

static void
amitk_curve_reset_vector (AmitkCurve *curve)
{
  GtkWidget *w = GTK_WIDGET (curve);

  g_free (curve->ctlpoint);

  curve->num_ctlpoints = 2;
  curve->ctlpoint = g_malloc (2 * sizeof (curve->ctlpoint[0]));
  curve->ctlpoint[0][0] = curve->min_x;
  curve->ctlpoint[0][1] = curve->min_y;
  curve->ctlpoint[1][0] = curve->max_x;
  curve->ctlpoint[1][1] = curve->max_y;

  if (curve->surface)
    {
      gint width, height;

      width = gtk_widget_get_allocated_width (w) - RADIUS * 2;
      height = gtk_widget_get_allocated_height (w) - RADIUS * 2;
      amitk_curve_interpolate (curve, width, height);
      amitk_curve_draw (curve, width, height);
    }
}

void
amitk_curve_reset (AmitkCurve *c)
{
  AmitkCurveType old_type;

  old_type = c->curve_type;
  c->curve_type = AMITK_CURVE_TYPE_SPLINE;
  amitk_curve_reset_vector (c);

  if (old_type != AMITK_CURVE_TYPE_SPLINE)
    {
       g_signal_emit (c, curve_type_changed_signal, 0);
       g_object_notify (G_OBJECT (c), "curve-type");
    }
}

void
amitk_curve_set_range (AmitkCurve *curve,
                       gfloat      min_x,
                       gfloat      max_x,
                       gfloat      min_y,
                       gfloat      max_y)
{
  g_object_freeze_notify (G_OBJECT (curve));
  if (curve->min_x != min_x) {
     curve->min_x = min_x;
     g_object_notify (G_OBJECT (curve), "min-x");
  }
  if (curve->max_x != max_x) {
     curve->max_x = max_x;
     g_object_notify (G_OBJECT (curve), "max-x");
  }
  if (curve->min_y != min_y) {
     curve->min_y = min_y;
     g_object_notify (G_OBJECT (curve), "min-y");
  }
  if (curve->max_y != max_y) {
     curve->max_y = max_y;
     g_object_notify (G_OBJECT (curve), "max-y");
  }
  g_object_thaw_notify (G_OBJECT (curve));

  amitk_curve_size_graph (curve);
  amitk_curve_reset_vector (curve);
}

void
amitk_curve_get_vector (AmitkCurve *c, int veclen, gfloat vector[])
{
  gfloat rx, ry, dx, dy, min_x, delta_x, *mem, *xv, *yv, *y2v, prev;
  gint dst, i, x, next, num_active_ctlpoints = 0, first_active = -1;

  min_x = c->min_x;

  /* count active points: */
  prev = min_x - 1.0;
  for (i = num_active_ctlpoints = 0; i < c->num_ctlpoints; ++i)
    if (c->ctlpoint[i][0] > prev)
      {
        if (first_active < 0)
          first_active = i;
        prev = c->ctlpoint[i][0];
        ++num_active_ctlpoints;
      }

  /* handle degenerate case: */
  if (num_active_ctlpoints < 2)
    {
      if (num_active_ctlpoints > 0)
        ry = c->ctlpoint[first_active][1];
      else
        ry = c->min_y;
      if (ry < c->min_y) ry = c->min_y;
      if (ry > c->max_y) ry = c->max_y;
      for (x = 0; x < veclen; ++x)
        vector[x] = ry;
      return;
    }

  switch (c->curve_type)
    {
    case AMITK_CURVE_TYPE_SPLINE:
      mem = g_malloc (3 * num_active_ctlpoints * sizeof (gfloat));
      xv  = mem;
      yv  = mem + num_active_ctlpoints;
      y2v = mem + 2*num_active_ctlpoints;

      prev = min_x - 1.0;
      for (i = dst = 0; i < c->num_ctlpoints; ++i)
        if (c->ctlpoint[i][0] > prev)
          {
            prev    = c->ctlpoint[i][0];
            xv[dst] = c->ctlpoint[i][0];
            yv[dst] = c->ctlpoint[i][1];
            ++dst;
          }

      spline_solve (num_active_ctlpoints, xv, yv, y2v);

      rx = min_x;
      dx = (c->max_x - min_x) / (veclen - 1);
      for (x = 0; x < veclen; ++x, rx += dx)
        {
          ry = spline_eval (num_active_ctlpoints, xv, yv, y2v, rx);
          if (ry < c->min_y) ry = c->min_y;
          if (ry > c->max_y) ry = c->max_y;
          vector[x] = ry;
        }

      g_free (mem);
      break;

    case AMITK_CURVE_TYPE_LINEAR:
      dx = (c->max_x - min_x) / (veclen - 1);
      rx = min_x;
      ry = c->min_y;
      dy = 0.0;
      i  = first_active;
      for (x = 0; x < veclen; ++x, rx += dx)
        {
          if (rx >= c->ctlpoint[i][0])
            {
              if (rx > c->ctlpoint[i][0])
                ry = c->min_y;
              dy = 0.0;
              next = i + 1;
              while (next < c->num_ctlpoints
                     && c->ctlpoint[next][0] <= c->ctlpoint[i][0])
                ++next;
              if (next < c->num_ctlpoints)
                {
                  delta_x = c->ctlpoint[next][0] - c->ctlpoint[i][0];
                  dy = ((c->ctlpoint[next][1] - c->ctlpoint[i][1])
                        / delta_x);
                  dy *= dx;
                  ry = c->ctlpoint[i][1];
                  i = next;
                }
            }
          vector[x] = ry;
          ry += dy;
        }
      break;
    }
}

GtkWidget *
amitk_curve_new (void)
{
  return g_object_new (AMITK_TYPE_CURVE, NULL);
}

static void
amitk_curve_finalize (GObject *object)
{
  AmitkCurve *curve;

  g_return_if_fail (AMITK_IS_CURVE (object));

  curve = AMITK_CURVE (object);
  if (curve->surface)
    cairo_surface_destroy (curve->surface);
  g_free (curve->point);
  g_free (curve->ctlpoint);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}
