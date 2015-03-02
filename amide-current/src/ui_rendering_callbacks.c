/* ui_rendering_callbacks.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001 Andy Loening
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

#include "config.h"

#ifdef AMIDE_LIBVOLPACK_SUPPORT

#include <gnome.h>
#include "amide.h"
#include "realspace.h"
#include "color_table.h"
#include "volume.h"
#include "rendering.h"
#include "roi.h"
#include "study.h"
#include "image.h"
#include "ui_rendering.h"
#include "ui_rendering_callbacks.h"
#include "ui_rendering_dialog.h"


/* function callback for the "render" button */
void ui_rendering_callbacks_render(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;

  ui_rendering_update_canvases(ui_rendering); /* render now */

  return;
}








/* function to switch into immediate rendering mode */
void ui_rendering_callbacks_immediate(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;

  /* should we be rendering immediately */
  ui_rendering->immediate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  /* set the sensitivity of the render button */
  gtk_widget_set_sensitive(GTK_WIDGET(ui_rendering->render_button), !(ui_rendering->immediate));
    
  if (ui_rendering->immediate)
    ui_rendering_update_canvases(ui_rendering); /* render now */

  return;

}






/* function called when one of the rotate widgets has been hit */
void ui_rendering_callbacks_rotate(GtkAdjustment * adjustment, gpointer data) {

  ui_rendering_t * ui_rendering = data;
  axis_t i_axis;
  floatpoint_t rotation;

  /* figure out which rotate widget called me */
  i_axis = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(adjustment),"axis"));
  rotation = (adjustment->value/180)*M_PI; /* get rotation in radians */

  /* update the rotation values */
  rendering_context_set_rotation(ui_rendering->axis_context, i_axis, rotation);
  rendering_lists_set_rotation(ui_rendering->contexts, i_axis, rotation);

  /* render now if appropriate*/
  if (ui_rendering->immediate)
    ui_rendering_update_canvases(ui_rendering); 

  /* return adjustment back to normal */
  adjustment->value = 0.0;
  gtk_adjustment_changed(adjustment);

  return;
}


/* function called when the button to pop up a rendering parameters modification dialog is hit */
void ui_rendering_parameters_pressed(GtkWidget * widget, gpointer data) {
  
  ui_rendering_t * ui_rendering = data;

  ui_rendering_dialog_create(ui_rendering);

  return;
}


/* function to run for a delete_event */
void ui_rendering_callbacks_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_rendering_t * ui_rendering = data;

  /* if our parameter modification dialog is up, kill that */
  if (ui_rendering->parameter_dialog  != NULL)
    gtk_signal_emit_by_name(GTK_OBJECT(ui_rendering->parameter_dialog), "delete_event", 
			    NULL, ui_rendering);

  /* destroy the widget */
  gtk_widget_destroy(widget);

  /* free the associated data structure */
  ui_rendering = ui_rendering_free(ui_rendering);

  return;
}

#endif










