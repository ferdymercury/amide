/* amitk_spin_button.h - cludge to get around bad gtk_spin_button behavior
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2002 Andy Loening
 *
 * Author: Andy Loening <loening@ucla.edu>
 */

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
  02111-1307, USA.
*/

#include <math.h>
#include "amitk_spin_button.h"

#define ARROW_SIZE                         11
#define EPSILON                            1e-5

static void amitk_spin_button_class_init     (AmitkSpinButtonClass *klass);
static void amitk_spin_button_init           (AmitkSpinButton      *spin_button);
static gint spin_button_button_press         (GtkWidget          *widget,
					      GdkEventButton     *event);
static gint spin_button_button_release       (GtkWidget          *widget,
					      GdkEventButton     *event);
static GtkShadowType spin_button_get_shadow_type (GtkSpinButton *spin_button);
static void spin_button_draw_arrow (GtkSpinButton *spin_button, 
				    guint          arrow);
static void spin_button_real_spin (GtkSpinButton *spin_button,
				   gfloat         increment);


static AmitkSpinButtonClass * parent_class = NULL;


GtkType amitk_spin_button_get_type (void)
{
  static guint spin_button_type = 0;

  if (!spin_button_type)
    {
      static const GtkTypeInfo spin_button_info =
      {
	"AmitkSpinButton",
	sizeof (AmitkSpinButton),
	sizeof (AmitkSpinButtonClass),
	(GtkClassInitFunc) amitk_spin_button_class_init,
	(GtkObjectInitFunc) amitk_spin_button_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      spin_button_type = gtk_type_unique (GTK_TYPE_SPIN_BUTTON, &spin_button_info);
    }
  return spin_button_type;
}

static void amitk_spin_button_class_init (AmitkSpinButtonClass *class)
{
  GtkObjectClass   *object_class;
  GtkWidgetClass   *widget_class;

  object_class   = (GtkObjectClass*)   class;
  widget_class   = (GtkWidgetClass*)   class;

  parent_class = gtk_type_class (GTK_TYPE_SPIN_BUTTON);

  widget_class->button_press_event = spin_button_button_press;
  widget_class->button_release_event = spin_button_button_release;
}

static void amitk_spin_button_init (AmitkSpinButton *spin_button)
{
  return;
}

static gint spin_button_button_press (GtkWidget      *widget,
				      GdkEventButton *event)
{
  GtkSpinButton *spin;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_SPIN_BUTTON (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  spin = GTK_SPIN_BUTTON (widget);

  if (!spin->button)
    {
      if (event->window == spin->panel)
	{
	  if (!GTK_WIDGET_HAS_FOCUS (widget))
	    gtk_widget_grab_focus (widget);
	  gtk_grab_add (widget);
	  spin->button = event->button;
	  
	  if (GTK_EDITABLE (widget)->editable)
	    gtk_spin_button_update (spin);
	  
	  if (event->y <= widget->requisition.height / 2)
	    {
	      spin->click_child = GTK_ARROW_UP;
	      if (event->button == 1)
		spin_button_real_spin (spin, spin->adjustment->step_increment);
	      else if (event->button == 2)
		spin_button_real_spin (spin, spin->adjustment->page_increment);
	      spin_button_draw_arrow (spin, GTK_ARROW_UP);
	    }
	  else 
	    {
	      spin->click_child = GTK_ARROW_DOWN;
	      if (event->button == 1)
		spin_button_real_spin (spin, -spin->adjustment->step_increment);
	      else if (event->button == 2)
		spin_button_real_spin (spin, -spin->adjustment->page_increment);
	      spin_button_draw_arrow (spin, GTK_ARROW_DOWN);
	    }
	}
      else
	GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event);
    }
  return FALSE;
}


static GtkShadowType spin_button_get_shadow_type (GtkSpinButton *spin_button)
{
  GtkWidget *widget = GTK_WIDGET (spin_button);
  
  GtkShadowType shadow_type =
    gtk_style_get_prop_experimental (widget->style,
				     "GtkSpinButton::shadow_type", -1);

  if (shadow_type != (GtkShadowType)-1)
    return shadow_type;
  else
    return spin_button->shadow_type;
}




static void spin_button_draw_arrow (GtkSpinButton *spin_button, 
				    guint          arrow)
{
  GtkStateType state_type;
  GtkShadowType shadow_type;
  GtkShadowType spin_shadow_type;
  GtkWidget *widget;
  gint x;
  gint y;

  g_return_if_fail (spin_button != NULL);
  g_return_if_fail (GTK_IS_SPIN_BUTTON (spin_button));
  
  widget = GTK_WIDGET (spin_button);

  spin_shadow_type = spin_button_get_shadow_type (spin_button);

  if (GTK_WIDGET_DRAWABLE (spin_button))
    {
      if (!spin_button->wrap &&
	  (((arrow == GTK_ARROW_UP &&
	  (spin_button->adjustment->upper - spin_button->adjustment->value
	   <= EPSILON))) ||
	  ((arrow == GTK_ARROW_DOWN &&
	  (spin_button->adjustment->value - spin_button->adjustment->lower
	   <= EPSILON)))))
	{
	  shadow_type = GTK_SHADOW_ETCHED_IN;
	  state_type = GTK_STATE_NORMAL;
	}
      else
	{
	  if (spin_button->in_child == arrow)
	    {
	      if (spin_button->click_child == arrow)
		state_type = GTK_STATE_ACTIVE;
	      else
		state_type = GTK_STATE_PRELIGHT;
	    }
	  else
	    state_type = GTK_STATE_NORMAL;
	  
	  if (spin_button->click_child == arrow)
	    shadow_type = GTK_SHADOW_IN;
	  else
	    shadow_type = GTK_SHADOW_OUT;
	}
      if (arrow == GTK_ARROW_UP)
	{
	  if (spin_shadow_type != GTK_SHADOW_NONE)
	    {
	      x = widget->style->klass->xthickness;
	      y = widget->style->klass->ythickness;
	    }
	  else
	    {
	      x = widget->style->klass->xthickness - 1;
	      y = widget->style->klass->ythickness - 1;
	    }
	  gtk_paint_arrow (widget->style, spin_button->panel,
			   state_type, shadow_type, 
			   NULL, widget, "spinbutton",
			   arrow, TRUE, 
			   x, y, ARROW_SIZE, widget->requisition.height / 2 
			   - widget->style->klass->ythickness);
	}
      else
	{
	  if (spin_shadow_type != GTK_SHADOW_NONE)
	    {
	      x = widget->style->klass->xthickness;
	      y = widget->requisition.height / 2;
	    }
	  else
	    {
	      x = widget->style->klass->xthickness - 1;
	      y = widget->requisition.height / 2 + 1;
	    }
	  gtk_paint_arrow (widget->style, spin_button->panel,
			   state_type, shadow_type, 
			   NULL, widget, "spinbutton",
			   arrow, TRUE, 
			   x, y, ARROW_SIZE, widget->requisition.height / 2 
			   - widget->style->klass->ythickness);
	}
    }
}



static gint spin_button_button_release (GtkWidget      *widget,
					GdkEventButton *event)
{
  GtkSpinButton *spin;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_SPIN_BUTTON (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  spin = GTK_SPIN_BUTTON (widget);

  if (event->button == spin->button)
    {
      guint click_child;

      if (event->button == 3)
	{
	  if (event->y >= 0 && event->x >= 0 && 
	      event->y <= widget->requisition.height &&
	      event->x <= ARROW_SIZE + 2 * widget->style->klass->xthickness)
	    {
	      if (spin->click_child == GTK_ARROW_UP &&
		  event->y <= widget->requisition.height / 2)
		{
		  gfloat diff;

		  diff = spin->adjustment->upper - spin->adjustment->value;
		  if (diff > EPSILON)
		    spin_button_real_spin (spin, diff);
		}
	      else if (spin->click_child == GTK_ARROW_DOWN &&
		       event->y > widget->requisition.height / 2)
		{
		  gfloat diff;

		  diff = spin->adjustment->value - spin->adjustment->lower;
		  if (diff > EPSILON)
		    spin_button_real_spin (spin, -diff);
		}
	    }
	}		  
      gtk_grab_remove (widget);
      click_child = spin->click_child;
      spin->click_child = 2;
      spin->button = 0;
      spin_button_draw_arrow (spin, click_child);
    }
  else
    GTK_WIDGET_CLASS (parent_class)->button_release_event (widget, event);

  return FALSE;
}



static void spin_button_real_spin (GtkSpinButton *spin_button,
				   gfloat         increment)
{
  GtkAdjustment *adj;
  gfloat new_value = 0.0;

  g_return_if_fail (spin_button != NULL);
  g_return_if_fail (GTK_IS_SPIN_BUTTON (spin_button));
  
  adj = spin_button->adjustment;

  new_value = adj->value + increment;

  if (increment > 0)
    {
      if (spin_button->wrap)
	{
	  if (fabs (adj->value - adj->upper) < EPSILON)
	    new_value = adj->lower;
	  else if (new_value > adj->upper)
	    new_value = adj->upper;
	}
      else
	new_value = MIN (new_value, adj->upper);
    }
  else if (increment < 0) 
    {
      if (spin_button->wrap)
	{
	  if (fabs (adj->value - adj->lower) < EPSILON)
	    new_value = adj->upper;
	  else if (new_value < adj->lower)
	    new_value = adj->lower;
	}
      else
	new_value = MAX (new_value, adj->lower);
    }

  if (fabs (new_value - adj->value) > EPSILON)
    gtk_adjustment_set_value (adj, new_value);
}



/***********************************************************
 ***********************************************************
 ***                  Public interface                   ***
 ***********************************************************
 ***********************************************************/


GtkWidget * amitk_spin_button_new (GtkAdjustment *adjustment,
				   gfloat         climb_rate,
				   guint          digits)
{
  AmitkSpinButton *spin;

  if (adjustment)
    g_return_val_if_fail (GTK_IS_ADJUSTMENT (adjustment), NULL);
  g_return_val_if_fail (digits < 6, NULL);

  spin = gtk_type_new (AMITK_TYPE_SPIN_BUTTON);

  gtk_spin_button_configure(GTK_SPIN_BUTTON(spin), adjustment, climb_rate, digits);

  return GTK_WIDGET (spin);
}
