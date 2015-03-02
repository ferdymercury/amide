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
#include "amide.h"
#include "volume.h"
#include "rendering.h"
#include "roi.h"
#include "study.h"
#include "image.h"
#include "ui_rendering.h"
#include "ui_rendering_callbacks.h"





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
				   volume_time_t start, volume_time_t duration) {

  ui_rendering_t * ui_rendering;
  volume_t * axis_volume;

  /* alloc space for the data structure for passing ui info */
  if ((ui_rendering = (ui_rendering_t *) g_malloc(sizeof(ui_rendering_t))) == NULL) {
    g_warning("%s: couldn't allocate space for ui_rendering_t",PACKAGE);
    return NULL;
  }
  ui_rendering->reference_count = 1;

  /* set any needed parameters */
  ui_rendering->app = NULL;
  ui_rendering->parameter_dialog = NULL;
  ui_rendering->render_button = NULL;
  ui_rendering->immediate = FALSE;
  ui_rendering->contexts = NULL;
  ui_rendering->axis_image = NULL;
  ui_rendering->main_image = NULL;
  ui_rendering->axis_canvas_image = NULL;
  ui_rendering->main_canvas_image = NULL;
  ui_rendering->quality = RENDERING_DEFAULT_QUALITY;
  ui_rendering->pixel_type = RENDERING_DEFAULT_PIXEL_TYPE;
  ui_rendering->depth_cueing = RENDERING_DEFAULT_DEPTH_CUEING;
  ui_rendering->front_factor = RENDERING_DEFAULT_FRONT_FACTOR;
  ui_rendering->density = RENDERING_DEFAULT_DENSITY;
  ui_rendering->zoom = RENDERING_DEFAULT_ZOOM;


  /* initialize the axis rendering context */
  axis_volume = volume_get_axis_volume(AXIS_VOLUME_X_WIDTH, AXIS_VOLUME_Y_WIDTH, AXIS_VOLUME_Z_WIDTH);
  ui_rendering->axis_context = 
    rendering_context_init(axis_volume, axis_volume->coord_frame,
  			   axis_volume->corner, REALPOINT_MIN_DIM(axis_volume->voxel_size),
  			   REALPOINT_MAX_DIM(axis_volume->corner), start, duration);
  volume_free(axis_volume);

  /* initialize the rendering contexts */
  ui_rendering->contexts = rendering_list_init(volumes, coord_frame, start, duration );

  return ui_rendering;
}




/* render our volumes and place into the canvases */
void ui_rendering_update_canvases(ui_rendering_t * ui_rendering) {

  if (ui_rendering == NULL) {
    g_warning("%s: called ui_rendering_update_canvases with NULL ui_rendering....?",PACKAGE);
    return;
  }

  /* ---------- render our axis volume ------------ */
  if (ui_rendering->axis_image != NULL)
    gnome_canvas_destroy_image(ui_rendering->axis_image);

  if (ui_rendering->axis_context == NULL)
    ui_rendering->axis_image = image_blank(UI_RENDERING_BLANK_WIDTH, UI_RENDERING_BLANK_HEIGHT);
  else {
    rendering_context_render(ui_rendering->axis_context);
    ui_rendering->axis_image = 
      image_from_8bit(ui_rendering->axis_context->image, 
		      ui_rendering->axis_context->image_dim,
		      ui_rendering->axis_context->image_dim,
		      BW_LINEAR);
  }

  /* reset the min size of the widget */
  gnome_canvas_set_scroll_region(ui_rendering->axis_canvas, 0.0, 0.0, 
  				 ui_rendering->axis_image->rgb_width,  
  				 ui_rendering->axis_image->rgb_height);  

  //  requisition.width = ui_study->rgb_image[view]->rgb_width;
  //  requisition.height = ui_study->rgb_image[view]->rgb_height;
  //  gtk_widget_size_request(GTK_WIDGET(ui_study->canvas[view]), &requisition);
  gtk_widget_set_usize(GTK_WIDGET(ui_rendering->axis_canvas), 
		       ui_rendering->axis_image->rgb_width ,
		       ui_rendering->axis_image->rgb_height);  
  
  /* put up the image */
  if (ui_rendering->axis_canvas_image != NULL) {
    gnome_canvas_item_set(ui_rendering->axis_canvas_image,
			  "image", ui_rendering->axis_image,
			  "width", (double) ui_rendering->axis_image->rgb_width ,
			  "height", (double) ui_rendering->axis_image->rgb_height ,
			  NULL);
  } else {
    /* time to make a new image */
    ui_rendering->axis_canvas_image = 
      gnome_canvas_item_new(gnome_canvas_root(ui_rendering->axis_canvas),
			    gnome_canvas_image_get_type(),
			    "image", ui_rendering->axis_image,
			    "x", (double) 0.0,
			    "y", (double) 0.0,
			    "anchor", GTK_ANCHOR_NORTH_WEST,
			    "width",(double) ui_rendering->axis_image->rgb_width,
			    "height",(double) ui_rendering->axis_image->rgb_height,
			    NULL);
  }


  /* -------- render our volumes ------------ */
  if (ui_rendering->main_image != NULL)
    gnome_canvas_destroy_image(ui_rendering->main_image);

  if (ui_rendering->contexts == NULL)
    ui_rendering->main_image = image_blank(UI_RENDERING_BLANK_WIDTH, UI_RENDERING_BLANK_HEIGHT);
  else {
    rendering_list_render(ui_rendering->contexts);
    /* base the dimensions on the first context in the list.... */
    ui_rendering->main_image = 
      image_from_renderings(ui_rendering->contexts, 
			    ceil(ui_rendering->zoom * ui_rendering->contexts->rendering_context->image_dim),
			    ceil(ui_rendering->zoom * ui_rendering->contexts->rendering_context->image_dim));
  }

  /* reset the min size of the widget */
  gnome_canvas_set_scroll_region(ui_rendering->main_canvas, 0.0, 0.0, 
  				 ui_rendering->main_image->rgb_width,  
  				 ui_rendering->main_image->rgb_height);  

  //  requisition.width = ui_study->rgb_image[view]->rgb_width;
  //  requisition.height = ui_study->rgb_image[view]->rgb_height;
  //  gtk_widget_size_request(GTK_WIDGET(ui_study->canvas[view]), &requisition);
  gtk_widget_set_usize(GTK_WIDGET(ui_rendering->main_canvas), 
		       ui_rendering->main_image->rgb_width ,
		       ui_rendering->main_image->rgb_height);  
  
  /* put up the image */
  if (ui_rendering->main_canvas_image != NULL) {
    gnome_canvas_item_set(ui_rendering->main_canvas_image,
			  "image", ui_rendering->main_image,
			  "width", (double) ui_rendering->main_image->rgb_width ,
			  "height", (double) ui_rendering->main_image->rgb_height ,
			  NULL);
  } else {
    /* time to make a new image */
    ui_rendering->main_canvas_image = 
      gnome_canvas_item_new(gnome_canvas_root(ui_rendering->main_canvas),
			    gnome_canvas_image_get_type(),
			    "image", ui_rendering->main_image,
			    "x", (double) 0.0,
			    "y", (double) 0.0,
			    "anchor", GTK_ANCHOR_NORTH_WEST,
			    "width",(double) ui_rendering->main_image->rgb_width,
			    "height",(double) ui_rendering->main_image->rgb_height,
			    NULL);
  }

  return;
} 



/* function that sets up the rendering dialog */
void ui_rendering_create(volume_list_t * volumes, realspace_t coord_frame, 
			 volume_time_t start, volume_time_t duration) {
  
  GtkWidget * packing_table;
  GtkWidget * check_button;
  GtkWidget * button;
  GtkWidget * label;
  GtkAdjustment * adjustment;
  GtkWidget * scale;
  GtkWidget * vbox;
  GtkWidget * hbox;
  axis_t i_axis;
  ui_rendering_t * ui_rendering;
  gchar * temp_string;

  /* sanity checks */
  if (volumes == NULL)
    return;

  ui_rendering = ui_rendering_init(volumes, coord_frame, start, duration);
  ui_rendering->app = GNOME_APP(gnome_app_new(PACKAGE, "Rendering Window"));

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(ui_rendering->app), "delete_event",
		     GTK_SIGNAL_FUNC(ui_rendering_callbacks_delete_event),
		     ui_rendering);

  /* make the widgets for this dialog box */
  packing_table = gtk_table_new(2,2,FALSE);
  gnome_app_set_contents(ui_rendering->app, GTK_WIDGET(packing_table));

  /* save some parameters */
  ui_rendering->coord_frame = coord_frame;

  /* start making those widgets */
  vbox = gtk_vbox_new(FALSE, Y_PADDING);
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(vbox), 0,1,0,2,
		   X_PACKING_OPTIONS | GTK_FILL,
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);
  

  /* setup the canvases */
  ui_rendering->main_canvas = GNOME_CANVAS(gnome_canvas_new());
  ui_rendering->axis_canvas = GNOME_CANVAS(gnome_canvas_new());
  ui_rendering_update_canvases(ui_rendering); /* fill in the main and axis canvas */
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(ui_rendering->main_canvas), 1,2,0,2,
		   X_PACKING_OPTIONS | GTK_FILL,
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);
  gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(ui_rendering->axis_canvas), FALSE, FALSE, 0);
  
  /* the Render button, after changing parameters, this is the button you'll
   * wanna hit to rerender the volume, unless you have immediate rendering set */
  ui_rendering->render_button = gtk_button_new_with_label("Render");
  gtk_box_pack_start(GTK_BOX(vbox), ui_rendering->render_button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(ui_rendering->render_button), "clicked", 
		     GTK_SIGNAL_FUNC(ui_rendering_callbacks_render), ui_rendering);


  /* the render immediately button.. */
  check_button = gtk_check_button_new_with_label ("render on change");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), ui_rendering->immediate);
  gtk_signal_connect(GTK_OBJECT(check_button), "toggled", 
  		     GTK_SIGNAL_FUNC(ui_rendering_callbacks_immediate), 
  		     ui_rendering);
  gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

 
  /* create the x, y, and z rotation dials */
 
  for (i_axis=0; i_axis<NUM_AXIS; i_axis++) {
    hbox = gtk_hbox_new(FALSE, X_PADDING);
    adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, -90.0, 90.0, 1.0, 1.0, 1.0));
    scale = gtk_hscale_new(adjustment);
    gtk_range_set_update_policy (GTK_RANGE(scale), GTK_UPDATE_DISCONTINUOUS);
    gtk_box_pack_start(GTK_BOX(hbox), scale, TRUE, TRUE, 0);
    temp_string = g_strdup_printf("%s axis",axis_names[i_axis]);
    label = gtk_label_new(temp_string);
    g_free(temp_string);
    gtk_object_set_data(GTK_OBJECT(adjustment), "axis", GINT_TO_POINTER(i_axis));
    gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    gtk_signal_connect (GTK_OBJECT(adjustment), "value_changed", 
			GTK_SIGNAL_FUNC (ui_rendering_callbacks_rotate), ui_rendering);
  }

  /* button to get the rendering parameters modification dialog */
  button = gtk_button_new_with_label("rendering parameters");
  gtk_signal_connect(GTK_OBJECT(button), "pressed",
		     GTK_SIGNAL_FUNC(ui_rendering_parameters_pressed),
		     ui_rendering);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);



  /* and show all our widgets */
  gtk_widget_show_all(GTK_WIDGET(ui_rendering->app));

  return;
}



#endif



