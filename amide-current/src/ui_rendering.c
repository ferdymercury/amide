/* ui_rendering.c
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
#include <math.h>
#include <gdk-pixbuf/gnome-canvas-pixbuf.h>
#include "image.h"
#include "ui_rendering.h"
#include "ui_rendering_cb.h"
#include "ui_rendering_menus.h"




/* destroy a ui_rendering data structure */
ui_rendering_t * ui_rendering_free(ui_rendering_t * ui_rendering) {

  if (ui_rendering == NULL)
    return ui_rendering;

  /* sanity checks */
  g_return_val_if_fail(ui_rendering->reference_count > 0, NULL);

  /* remove a reference count */
  ui_rendering->reference_count--;

  /* things we always do */
  ui_rendering->contexts = rendering_list_free(ui_rendering->contexts);

  /* things to do if we've removed all reference's */
  if (ui_rendering->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing ui_rendering\n");
#endif
    g_free(ui_rendering);
    ui_rendering = NULL;
  }

  return ui_rendering;

}





/* malloc and initialize a ui_rendering data structure */
ui_rendering_t * ui_rendering_init(volume_list_t * volumes, realspace_t coord_frame, 
				   amide_time_t start, amide_time_t duration, interpolation_t interpolation) {

  ui_rendering_t * ui_rendering;

  /* alloc space for the data structure for passing ui info */
  if ((ui_rendering = (ui_rendering_t *) g_malloc(sizeof(ui_rendering_t))) == NULL) {
    g_warning("%s: couldn't allocate space for ui_rendering_t",PACKAGE);
    return NULL;
  }
  ui_rendering->reference_count = 1;

  /* set any needed parameters */
  ui_rendering->app = NULL;
  ui_rendering->parameter_dialog = NULL;
#ifdef AMIDE_MPEG_ENCODE_SUPPORT
  ui_rendering->movie = NULL;
#endif
  ui_rendering->render_button = NULL;
  ui_rendering->immediate = TRUE;
  ui_rendering->contexts = NULL;
  ui_rendering->rgb_image = NULL;
  ui_rendering->canvas_image = NULL;
  ui_rendering->quality = RENDERING_DEFAULT_QUALITY;
  ui_rendering->pixel_type = RENDERING_DEFAULT_PIXEL_TYPE;
  ui_rendering->depth_cueing = RENDERING_DEFAULT_DEPTH_CUEING;
  ui_rendering->front_factor = RENDERING_DEFAULT_FRONT_FACTOR;
  ui_rendering->density = RENDERING_DEFAULT_DENSITY;
  ui_rendering->zoom = RENDERING_DEFAULT_ZOOM;
  ui_rendering->interpolation=interpolation;
  ui_rendering->start = start;
  ui_rendering->duration = duration;

  /* initialize the rendering contexts */
  ui_rendering->contexts = rendering_list_init(volumes, coord_frame, start, duration,
					       ui_rendering->interpolation);

  return ui_rendering;
}




/* render our volumes and place into the canvases */
void ui_rendering_update_canvases(ui_rendering_t * ui_rendering) {

  rgba_t blank_rgba;
  intpoint_t size_dim; 

  blank_rgba.r = blank_rgba.g = blank_rgba.b = 0;
  blank_rgba.a = 0xFF;

  if (ui_rendering == NULL) {
    g_warning("%s: called ui_rendering_update_canvases with NULL ui_rendering....?",PACKAGE);
    return;
  }

  /* reload the volumes if the time's changed */
  if (ui_rendering->contexts != NULL)
    rendering_list_reload_volume(ui_rendering->contexts, ui_rendering->start,
				 ui_rendering->duration, ui_rendering->interpolation);

  /* -------- render our volumes ------------ */
  if (ui_rendering->rgb_image != NULL)
    gdk_pixbuf_unref(ui_rendering->rgb_image);

  if (ui_rendering->contexts == NULL)
    ui_rendering->rgb_image = image_blank(UI_RENDERING_BLANK_WIDTH, UI_RENDERING_BLANK_HEIGHT, blank_rgba);
  else {
    rendering_list_render(ui_rendering->contexts);
    /* base the dimensions on the first context in the list.... */
    size_dim = ceil(ui_rendering->zoom*REALPOINT_MAX(ui_rendering->contexts->rendering_context->dim));
    ui_rendering->rgb_image = image_from_renderings(ui_rendering->contexts, size_dim, size_dim); 
  }

  /* reset the min size of the widget */
  gnome_canvas_set_scroll_region(ui_rendering->canvas, 0.0, 0.0, 
  				 gdk_pixbuf_get_width(ui_rendering->rgb_image),  
  				 gdk_pixbuf_get_height(ui_rendering->rgb_image));  

  //  requisition.width = ui_study->rgb_image[view]->rgb_width;
  //  requisition.height = ui_study->rgb_image[view]->rgb_height;
  //  gtk_widget_size_request(GTK_WIDGET(ui_study->canvas[view]), &requisition);
  gtk_widget_set_usize(GTK_WIDGET(ui_rendering->canvas), 
		       gdk_pixbuf_get_width(ui_rendering->rgb_image),
		       gdk_pixbuf_get_height(ui_rendering->rgb_image));  
  
  /* put up the image */
  if (ui_rendering->canvas_image != NULL) {
    gnome_canvas_item_set(ui_rendering->canvas_image,
			  "pixbuf", ui_rendering->rgb_image,
			  NULL);
  } else {
    /* time to make a new image */
    ui_rendering->canvas_image = 
      gnome_canvas_item_new(gnome_canvas_root(ui_rendering->canvas),
			    gnome_canvas_pixbuf_get_type(),
			    "pixbuf", ui_rendering->rgb_image,
			    "x", (double) 0.0,
			    "y", (double) 0.0,
			    NULL);
  }

  return;
} 



/* function that sets up the rendering dialog */
void ui_rendering_create(volume_list_t * volumes, realspace_t coord_frame, 
			 amide_time_t start, amide_time_t duration, interpolation_t interpolation) {
  
  GtkWidget * packing_table;
  GtkWidget * check_button;
  GtkWidget * button;
  GtkAdjustment * adjustment;
  GtkWidget * scale;
  GtkWidget * vbox;
  GtkWidget * hbox;
  GtkWidget * hseparator;
  GtkWidget * dial;
  GtkWidget * label;
  GnomeCanvas * axis_indicator;
  GnomeCanvasPoints * axis_line_points;
  ui_rendering_t * ui_rendering;

  /* sanity checks */
  if (volumes == NULL)
    return;

  ui_rendering = ui_rendering_init(volumes, coord_frame, start, duration, interpolation);
  ui_rendering->app = GNOME_APP(gnome_app_new(PACKAGE, "Rendering Window"));

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(ui_rendering->app), "delete_event",
		     GTK_SIGNAL_FUNC(ui_rendering_cb_delete_event),
		     ui_rendering);

  /* setup the rendering menu */
  ui_rendering_menus_create(ui_rendering);

  /* make the widgets for this dialog box */
  packing_table = gtk_table_new(3,3,FALSE);
  gnome_app_set_contents(ui_rendering->app, GTK_WIDGET(packing_table));

  /* start making those widgets */
  vbox = gtk_vbox_new(FALSE, Y_PADDING);
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(vbox), 0,1,0,2,
		   X_PACKING_OPTIONS | GTK_FILL,
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);

  /* create the z dial */
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new("z");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
  adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, -90.0, 90.0, 1.0, 1.0, 1.0));
  dial = gtk_dial_new(adjustment);
  gtk_dial_set_update_policy (GTK_DIAL(dial), GTK_UPDATE_DISCONTINUOUS);
  gtk_object_set_data(GTK_OBJECT(adjustment), "axis", GINT_TO_POINTER(ZAXIS));
  gtk_box_pack_start(GTK_BOX(hbox), dial, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT(adjustment), "value_changed", 
		      GTK_SIGNAL_FUNC (ui_rendering_cb_rotate), ui_rendering);


  /* a canvas to indicate which way is x, y, and z */
  axis_indicator = GNOME_CANVAS(gnome_canvas_new());
  gtk_widget_set_usize(GTK_WIDGET(axis_indicator), 100, 100);
  gnome_canvas_set_scroll_region(axis_indicator, 0.0, 0.0, 100.0, 100.0);
  gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(axis_indicator), TRUE, TRUE, 0);

  /* the x axis */
  axis_line_points = gnome_canvas_points_new(2);
  axis_line_points->coords[0] = 40.0; /* x1 */
  axis_line_points->coords[1] = 60.0; /* y1 */
  axis_line_points->coords[2] = 95.0; /* x2 */
  axis_line_points->coords[3] = 60.0; /* y2 */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_line_get_type(),
			"points", axis_line_points, "fill_color", "black",
			"width_units", 2.0, "last_arrowhead", TRUE, 
			"arrow_shape_a", 8.0,
			"arrow_shape_b", 8.0,
			"arrow_shape_c", 8.0,
			NULL);

  gnome_canvas_points_unref(axis_line_points);
  /* the x label */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator),
			gnome_canvas_text_get_type(),
			"justification", GTK_JUSTIFY_LEFT,
			"anchor", GTK_ANCHOR_NORTH_EAST,
			"text", "x",
			"x", (gdouble) 80.0,
			"y", (gdouble) 65.0,
			"font", "fixed", NULL);

  /* the y axis */
  axis_line_points = gnome_canvas_points_new(2);
  axis_line_points->coords[0] = 40.0; /* x1 */
  axis_line_points->coords[1] = 60.0; /* y1 */
  axis_line_points->coords[2] = 40.0; /* x2 */
  axis_line_points->coords[3] = 5.0; /* y2 */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_line_get_type(),
			"points", axis_line_points, "fill_color", "black",
			"width_units", 2.0, "last_arrowhead", TRUE, 
			"arrow_shape_a", 8.0,
			"arrow_shape_b", 8.0,
			"arrow_shape_c", 8.0,
			NULL);
  gnome_canvas_points_unref(axis_line_points);
  /* the y label */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator),
			gnome_canvas_text_get_type(),
			"justification", GTK_JUSTIFY_LEFT,
			"anchor", GTK_ANCHOR_NORTH_WEST,
			"text", "y",
			"x", (gdouble) 45.0,
			"y", (gdouble) 20.0,
			"font", "fixed", NULL);

  /* the z axis */
  axis_line_points = gnome_canvas_points_new(2);
  axis_line_points->coords[0] = 40.0; /* x1 */
  axis_line_points->coords[1] = 60.0; /* y1 */
  axis_line_points->coords[2] = 5.0; /* x2 */
  axis_line_points->coords[3] = 95.0; /* y2 */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_line_get_type(),
			"points", axis_line_points, "fill_color", "black",
			"width_units", 2.0, "last_arrowhead", TRUE,
			"arrow_shape_a", 8.0,
			"arrow_shape_b", 8.0,
			"arrow_shape_c", 8.0,
			NULL);
  gnome_canvas_points_unref(axis_line_points);
  /* the z label */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator),
			gnome_canvas_text_get_type(),
			"justification", GTK_JUSTIFY_LEFT,
			"anchor", GTK_ANCHOR_NORTH_WEST,
			"text", "z",
			"x", (gdouble) 20.0,
			"y", (gdouble) 80.0,
			"font", "fixed", NULL);

  /* button to reset the axis */
  button = gtk_button_new_with_label("Reset Axis");
  gtk_signal_connect(GTK_OBJECT(button), "pressed",
		     GTK_SIGNAL_FUNC(ui_rendering_cb_reset_axis_pressed),
		     ui_rendering);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);



  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), hseparator, FALSE, FALSE, 0);

  /* the Render button, after changing parameters, this is the button you'll
   * wanna hit to rerender the volume, unless you have immediate rendering set */
  ui_rendering->render_button = gtk_button_new_with_label("Render");
  gtk_box_pack_start(GTK_BOX(vbox), ui_rendering->render_button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive(GTK_WIDGET(ui_rendering->render_button), !(ui_rendering->immediate));
  gtk_signal_connect(GTK_OBJECT(ui_rendering->render_button), "clicked", 
		     GTK_SIGNAL_FUNC(ui_rendering_cb_render), ui_rendering);


  /* the render immediately button.. */
  check_button = gtk_check_button_new_with_label ("on change");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), ui_rendering->immediate);
  gtk_signal_connect(GTK_OBJECT(check_button), "toggled", 
  		     GTK_SIGNAL_FUNC(ui_rendering_cb_immediate), 
  		     ui_rendering);
  gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

  /* setup the main canvases */
  ui_rendering->canvas = GNOME_CANVAS(gnome_canvas_new());
  ui_rendering_update_canvases(ui_rendering); /* fill in the canvas */
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(ui_rendering->canvas), 1,2,1,2,
		   X_PACKING_OPTIONS | GTK_FILL,
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);
  gtk_signal_connect(GTK_OBJECT(ui_rendering->canvas), "event",
		     GTK_SIGNAL_FUNC(ui_rendering_cb_canvas_event),
		     ui_rendering);
  

  /* create the x, and y rotation dials */
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_table_attach(GTK_TABLE(packing_table), hbox, 1,2,0,1,
		   X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL, 
		   X_PADDING, Y_PADDING);
  adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, -90.0, 90.0, 1.0, 1.0, 1.0));
  scale = gtk_hscale_new(adjustment);
  gtk_range_set_update_policy (GTK_RANGE(scale), GTK_UPDATE_DISCONTINUOUS);
  gtk_object_set_data(GTK_OBJECT(adjustment), "axis", GINT_TO_POINTER(YAXIS));
  gtk_box_pack_start(GTK_BOX(hbox), scale, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT(adjustment), "value_changed", 
		      GTK_SIGNAL_FUNC (ui_rendering_cb_rotate), ui_rendering);
  label = gtk_label_new("y");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_table_attach(GTK_TABLE(packing_table), vbox,  2,3,1,2,
		   X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL, 
		   X_PADDING, Y_PADDING);
  label = gtk_label_new("x");
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
  adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, -90.0, 90.0, 1.0, 1.0, 1.0));
  scale = gtk_vscale_new(adjustment);
  gtk_range_set_update_policy (GTK_RANGE(scale), GTK_UPDATE_DISCONTINUOUS);
  gtk_object_set_data(GTK_OBJECT(adjustment), "axis", GINT_TO_POINTER(XAXIS));
  gtk_box_pack_start(GTK_BOX(vbox), scale, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT(adjustment), "value_changed", 
		      GTK_SIGNAL_FUNC (ui_rendering_cb_rotate), ui_rendering);

  /* and show all our widgets */
  gtk_widget_show_all(GTK_WIDGET(ui_rendering->app));
  number_of_windows++;

  return;
}



#endif



