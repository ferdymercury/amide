/* amitk_dial.c, adapated from gtkdial.c */

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
#include <math.h>
#include <stdio.h>
#include "amitk_dial.h"

#define DIAL_DEFAULT_SIZE 50

/* Forward declarations */

static void amitk_dial_class_init               (AmitkDialClass    *klass);
static void amitk_dial_init                     (AmitkDial         *dial);
static void amitk_dial_destroy                  (GtkWidget         *object);
static void amitk_dial_realize                  (GtkWidget        *widget);
static void amitk_dial_size_request             (GtkWidget      *widget,
                                                 gint           *minimal,
                                                 gint           *natural);
static void amitk_dial_size_allocate            (GtkWidget     *widget,
					       GtkAllocation *allocation);
static gint amitk_dial_expose                   (GtkWidget        *widget,
						cairo_t           *cr);
static gint amitk_dial_button_press             (GtkWidget        *widget,
						GdkEventButton   *event);
static gint amitk_dial_button_release           (GtkWidget        *widget,
						GdkEventButton   *event);
static gint amitk_dial_motion_notify            (GtkWidget        *widget,
						GdkEventMotion   *event);

static void amitk_dial_update_mouse             (AmitkDial *dial, gint x, gint y);
static void amitk_dial_update                   (AmitkDial *dial);
static void amitk_dial_adjustment_changed       (GtkAdjustment    *adjustment,
						gpointer          data);
static void amitk_dial_adjustment_value_changed (GtkAdjustment    *adjustment,
						gpointer          data);

/* Local data */

static GtkWidgetClass *parent_class = NULL;

GType
amitk_dial_get_type ()
{
  static GType dial_type = 0;

  if (!dial_type)
    {
      static const GTypeInfo dial_info =
      {
	sizeof (AmitkDialClass),
	NULL,
	NULL,
	(GClassInitFunc) amitk_dial_class_init,
	NULL,
	NULL,
	sizeof (AmitkDial),
        0,
	(GInstanceInitFunc) amitk_dial_init,
      };

      dial_type = g_type_register_static (GTK_TYPE_WIDGET, "AmitkDial", &dial_info, 0);
    }

  return dial_type;
}

static void
amitk_dial_class_init (AmitkDialClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = (GtkWidgetClass*) class;

  parent_class = g_type_class_peek_parent (class);

  widget_class->destroy = amitk_dial_destroy;

  widget_class->realize = amitk_dial_realize;
  widget_class->draw = amitk_dial_expose;
  widget_class->get_preferred_width = amitk_dial_size_request;
  widget_class->get_preferred_height = amitk_dial_size_request;
  widget_class->size_allocate = amitk_dial_size_allocate;
  widget_class->button_press_event = amitk_dial_button_press;
  widget_class->button_release_event = amitk_dial_button_release;
  widget_class->motion_notify_event = amitk_dial_motion_notify;
}

static void
amitk_dial_init (AmitkDial *dial)
{
  dial->button = 0;
  dial->radius = 0;
  dial->pointer_width = 0;
  dial->angle = 0.0;
  dial->old_value = 0.0;
  dial->old_lower = 0.0;
  dial->old_upper = 0.0;
  dial->adjustment = NULL;
}

GtkWidget*
amitk_dial_new (GtkAdjustment *adjustment)
{
  AmitkDial *dial;

  dial = g_object_new (amitk_dial_get_type (), NULL);

  if (!adjustment)
    adjustment = (GtkAdjustment*) gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  amitk_dial_set_adjustment (dial, adjustment);

  return GTK_WIDGET (dial);
}

static void
amitk_dial_destroy (GtkWidget *object)
{
  AmitkDial *dial;

  g_return_if_fail (object != NULL);
  g_return_if_fail (AMITK_IS_DIAL (object));

  dial = AMITK_DIAL (object);

  if (dial->adjustment) {
    g_object_unref (G_OBJECT (dial->adjustment));
    dial->adjustment = NULL;
  }

  (* GTK_WIDGET_CLASS (parent_class)->destroy) (object);
}

GtkAdjustment*
amitk_dial_get_adjustment (AmitkDial *dial)
{
  g_return_val_if_fail (dial != NULL, NULL);
  g_return_val_if_fail (AMITK_IS_DIAL (dial), NULL);

  return dial->adjustment;
}

void
amitk_dial_set_adjustment (AmitkDial      *dial,
			  GtkAdjustment *adjustment)
{
  g_return_if_fail (dial != NULL);
  g_return_if_fail (AMITK_IS_DIAL (dial));

  if (dial->adjustment)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (dial->adjustment), NULL, (gpointer) dial);
      g_object_unref (G_OBJECT (dial->adjustment));
    }

  dial->adjustment = adjustment;
  g_object_ref (G_OBJECT (dial->adjustment));

  g_signal_connect (G_OBJECT (adjustment), "changed",
		    G_CALLBACK (amitk_dial_adjustment_changed),
		    (gpointer) dial);
  g_signal_connect (G_OBJECT (adjustment), "value_changed",
		    G_CALLBACK (amitk_dial_adjustment_value_changed),
		    (gpointer) dial);

  dial->old_value = gtk_adjustment_get_value(adjustment);
  dial->old_lower = gtk_adjustment_get_lower(adjustment);
  dial->old_upper = gtk_adjustment_get_upper(adjustment);

  amitk_dial_update (dial);
}

static void
amitk_dial_realize (GtkWidget *widget)
{
  /*  AmitkDial *dial; */
  GdkWindowAttr attributes;
  GtkAllocation allocation;
  gint attributes_mask;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (AMITK_IS_DIAL (widget));

  gtk_widget_set_realized (widget, TRUE);
  /* dial = AMITK_DIAL (widget); */

  gtk_widget_get_allocation (widget, &allocation);
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (widget) | 
    GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | 
    GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
    GDK_POINTER_MOTION_HINT_MASK;
  attributes.visual = gtk_widget_get_visual (widget);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;
  gtk_widget_set_window (widget,
                         gdk_window_new (gtk_widget_get_parent_window (widget),
                                         &attributes, attributes_mask));

  gtk_widget_register_window (widget, gtk_widget_get_window (widget));
}

static void 
amitk_dial_size_request (GtkWidget      *widget,
                         gint           *minimal,
                         gint           *natural)
{
  *minimal = *natural = DIAL_DEFAULT_SIZE;
}

static void
amitk_dial_size_allocate (GtkWidget     *widget,
			GtkAllocation *allocation)
{
  AmitkDial *dial;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (AMITK_IS_DIAL (widget));
  g_return_if_fail (allocation != NULL);

  gtk_widget_set_allocation (widget, allocation);
  dial = AMITK_DIAL (widget);

  if (gtk_widget_get_realized (widget))
    {

      gdk_window_move_resize (gtk_widget_get_window (widget),
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);

    }
  dial->radius = MIN (allocation->width, allocation->height) * 0.45;
  dial->pointer_width = dial->radius / 2;
}

static gboolean
amitk_dial_expose (GtkWidget      *widget,
                   cairo_t        *cr)
{
  AmitkDial *dial;
  GtkStyleContext *ctxt;
  cairo_pattern_t *pat;
  cairo_rectangle_t points[6];
  GdkRGBA color;
  gdouble s,c;
  gint xc, yc;
  gint i;
  /*  gint upper, lower; */

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (AMITK_IS_DIAL (widget), FALSE);

  dial = AMITK_DIAL (widget);

/*  gdk_window_clear_area (widget->window,
			 0, 0,
			 widget->allocation.width,
			 widget->allocation.height);
*/
  xc = gtk_widget_get_allocated_width (widget) / 2;
  yc = gtk_widget_get_allocated_height (widget) / 2;

  /*  upper = dial->adjustment->upper; */
  /*  lower = dial->adjustment->lower; */

  ctxt = gtk_widget_get_style_context (widget);
  gtk_style_context_get_color (ctxt, GTK_STATE_FLAG_NORMAL, &color);
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_set_line_width (cr, 1.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);

  /* draw circle */
  cairo_arc (cr, xc, yc, dial->radius, 0, 2*M_PI);
  cairo_new_sub_path (cr);

  /* draw circle */
  cairo_arc (cr, xc - 0.05*dial->radius, yc - 0.05*dial->radius,
             0.9*dial->radius, 0, 2*M_PI);
  cairo_stroke (cr);

  /* Draw pointer */

  s = sin (dial->angle);
  c = cos (dial->angle);

  points[0].x = xc + 0.9*s*dial->pointer_width/2;
  points[0].y = yc + 0.9*c*dial->pointer_width/2;
  points[1].x = xc + 0.9*c*dial->radius;
  points[1].y = yc - 0.9*s*dial->radius;
  points[2].x = xc - 0.9*s*dial->pointer_width/2;
  points[2].y = yc - 0.9*c*dial->pointer_width/2;
  points[3].x = xc - 0.9*c*dial->radius/10;
  points[3].y = yc + 0.9*s*dial->radius/10;
  points[4].x = points[0].x;
  points[4].y = points[0].y;


  for (i = 0; i < 5; i++)
    cairo_line_to (cr, points[i].x, points[i].y);
  cairo_close_path (cr);

  /* I find it difficult to immitate GTK_SHADOW_IN with cairo, so at
     least resort to a simple gradient (otherwise the pointer looks
     rather ugly whether filled or not).  */
  pat = cairo_pattern_create_linear (points[0].x, points[0].y,
                                     points[3].x, points[3].y);
  cairo_pattern_add_color_stop_rgba (pat, 1, color.red, color.green,
                                     color.blue, color.alpha);
  cairo_pattern_add_color_stop_rgba (pat, 0, 1, 1, 1, 1);
  cairo_set_source (cr, pat);
  cairo_fill_preserve (cr);
  cairo_pattern_destroy (pat);
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_stroke (cr);

  return FALSE;
}

static gint
amitk_dial_button_press (GtkWidget      *widget,
		       GdkEventButton *event)
{
  AmitkDial *dial;
  gint dx, dy;
  double s, c;
  double d_parallel;
  double d_perpendicular;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (AMITK_IS_DIAL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  dial = AMITK_DIAL (widget);

  /* Determine if button press was within pointer region - we 
     do this by computing the parallel and perpendicular distance of
     the point where the mouse was pressed from the line passing through
     the pointer */
  
  dx = event->x - gtk_widget_get_allocated_width (widget) / 2;
  dy = gtk_widget_get_allocated_height (widget) / 2 - event->y;
  
  s = sin (dial->angle);
  c = cos (dial->angle);
  
  d_parallel = s*dy + c*dx;
  d_perpendicular = fabs (s*dx - c*dy);
  
  if (!dial->button &&
      (d_perpendicular < dial->pointer_width/2) &&
      (d_parallel > - dial->pointer_width))
    {
      gtk_grab_add (widget);

      dial->button = event->button;

      amitk_dial_update_mouse (dial, event->x, event->y);
    }

  return FALSE;
}

static gint
amitk_dial_button_release (GtkWidget      *widget,
			  GdkEventButton *event)
{
  AmitkDial *dial;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (AMITK_IS_DIAL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  dial = AMITK_DIAL (widget);

  if (dial->button == event->button)
    {
      gtk_grab_remove (widget);

      dial->button = 0;

      if (dial->old_value != gtk_adjustment_get_value (dial->adjustment))
	g_signal_emit_by_name (G_OBJECT (dial->adjustment), "value_changed");
    }

  return FALSE;
}

static gint
amitk_dial_motion_notify (GtkWidget      *widget,
			 GdkEventMotion *event)
{
  AmitkDial *dial;
  gint x, y, mask;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (AMITK_IS_DIAL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  dial = AMITK_DIAL (widget);

  if (dial->button != 0)
    {
      x = event->x;
      y = event->y;

      if (event->is_hint || (event->window != gtk_widget_get_window (widget)))
        gdk_event_request_motions(event);

      switch (dial->button)
	{
	case 1:
	  mask = GDK_BUTTON1_MASK;
	  break;
	case 2:
	  mask = GDK_BUTTON2_MASK;
	  break;
	case 3:
	  mask = GDK_BUTTON3_MASK;
	  break;
	default:
	  mask = 0;
	  break;
	}

      if (mask)
	amitk_dial_update_mouse (dial, x,y);
    }

  return FALSE;
}

static void
amitk_dial_update_mouse (AmitkDial *dial, gint x, gint y)
{
  gint xc, yc;

  g_return_if_fail (dial != NULL);
  g_return_if_fail (AMITK_IS_DIAL (dial));

  xc = gtk_widget_get_allocated_width (GTK_WIDGET(dial)) / 2;
  yc = gtk_widget_get_allocated_height (GTK_WIDGET(dial)) / 2;

  dial->old_value = gtk_adjustment_get_value (dial->adjustment);
  dial->angle = atan2(yc-y, x-xc);

  if (dial->angle < M_PI/2.0)
    dial->angle += 2*M_PI;
  if (dial->angle > 4*M_PI/3.0)
    dial->angle -= 2*M_PI;

  gtk_adjustment_set_value (dial->adjustment,
                            gtk_adjustment_get_upper (dial->adjustment)
                            - (dial->angle+M_PI/2.0)*
                            (gtk_adjustment_get_upper (dial->adjustment)
                             - gtk_adjustment_get_lower (dial->adjustment))
                            / (2*M_PI));

  if (gtk_adjustment_get_value (dial->adjustment) != dial->old_value)
    gtk_widget_queue_draw (GTK_WIDGET (dial));
}

static void
amitk_dial_update (AmitkDial *dial)
{
  gdouble new_value;
  
  g_return_if_fail (dial != NULL);
  g_return_if_fail (AMITK_IS_DIAL (dial));

  new_value = gtk_adjustment_get_value (dial->adjustment);
  
  if (new_value < gtk_adjustment_get_lower (dial->adjustment))
    new_value = gtk_adjustment_get_lower (dial->adjustment);

  if (new_value > gtk_adjustment_get_upper (dial->adjustment))
    new_value = gtk_adjustment_get_upper (dial->adjustment);

  if (new_value != gtk_adjustment_get_value (dial->adjustment))
    {
      gtk_adjustment_set_value (dial->adjustment, new_value);
    }

  dial->angle = -M_PI/2.0 + 2.0*M_PI
    * (new_value - gtk_adjustment_get_lower (dial->adjustment))
    / (gtk_adjustment_get_upper (dial->adjustment)
       - gtk_adjustment_get_lower (dial->adjustment));

  gtk_widget_queue_draw (GTK_WIDGET (dial));
}

static void
amitk_dial_adjustment_changed (GtkAdjustment *adjustment,
			      gpointer       data)
{
  AmitkDial *dial;

  g_return_if_fail (adjustment != NULL);
  g_return_if_fail (data != NULL);

  dial = AMITK_DIAL (data);

  if ((dial->old_value != gtk_adjustment_get_value (adjustment)) ||
      (dial->old_lower != gtk_adjustment_get_lower (adjustment)) ||
      (dial->old_upper != gtk_adjustment_get_upper (adjustment)))
    {
      amitk_dial_update (dial);

      dial->old_value = gtk_adjustment_get_value (adjustment);
      dial->old_lower = gtk_adjustment_get_lower (adjustment);
      dial->old_upper = gtk_adjustment_get_upper (adjustment);
    }
}

static void
amitk_dial_adjustment_value_changed (GtkAdjustment *adjustment,
				    gpointer       data)
{
  AmitkDial *dial;

  g_return_if_fail (adjustment != NULL);
  g_return_if_fail (data != NULL);

  dial = AMITK_DIAL (data);

  if (dial->button)
    return;

  if (dial->old_value != gtk_adjustment_get_value (adjustment))
    {
      amitk_dial_update (dial);

      dial->old_value = gtk_adjustment_get_value (adjustment);
    }
}
