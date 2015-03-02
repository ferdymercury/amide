/* ui_study.c
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
#include <gnome.h>
#include <math.h>
#include "amide.h"
#include "realspace.h"
#include "color_table.h"
#include "volume.h"
#include "roi.h"
#include "study.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_study_rois.h"
#include "ui_study_volumes.h"
#include "ui_study.h"
#include "ui_study_callbacks.h"
#include "ui_study_menus.h"
#include "ui_study_rois2.h"

#include "../pixmaps/study.xpm"
#include "../pixmaps/PET.xpm"
#include "../pixmaps/SPECT.xpm"
#include "../pixmaps/CT.xpm"
#include "../pixmaps/MRI.xpm"
#include "../pixmaps/OTHER.xpm"
#include "../pixmaps/ROI.xpm"

/* internal variables */
static gint next_study_num=1;


/* destroy a ui_study data structure */
void ui_study_free(ui_study_t ** pui_study) {
  
  while((*pui_study)->current_volumes != NULL)
    ui_study_volumes_list_remove_volume(&((*pui_study)->current_volumes),
					(*pui_study)->current_volumes->volume);
  while((*pui_study)->current_rois != NULL)
    ui_study_rois_list_remove_roi(&((*pui_study)->current_rois),
				  (*pui_study)->current_rois->roi);


  if ((*pui_study)->study != NULL)
    study_free(&((*pui_study)->study));

  g_free(*pui_study);
  *pui_study = NULL;

  return;
}

/* malloc and initialize a ui_study data structure */
ui_study_t * ui_study_init(void) {

  ui_study_t * ui_study;
  view_t i;

  /* alloc space for the data structure for passing ui info */
  if ((ui_study = (ui_study_t *) g_malloc(sizeof(ui_study_t))) == NULL) {
    g_warning("%s: couldn't allocate space for ui_study_t",PACKAGE);
    return NULL;
  }


  for (i=0; i<NUM_VIEWS;i++) {
    ui_study->current_slices[i] = NULL;
    ui_study->rgb_image[i] = NULL;
    ui_study->canvas_arrow[i][0] = NULL;
    ui_study->current_view_coord_frame.axis[i] = default_axis[i];
  }
  ui_study->current_view_coord_frame.offset = realpoint_init;
  ui_study->tree_studies = NULL;
  ui_study->tree_rois = NULL;
  ui_study->tree_volumes = NULL;
  ui_study->scaling = SLICE;
  ui_study->study = NULL;
  ui_study->current_mode = VOLUME_MODE;
  ui_study->current_volume = NULL;
  ui_study->current_roi = NULL;
  ui_study->current_volumes = NULL;
  ui_study->current_rois = NULL;
  ui_study->current_zoom = 1.0;
  ui_study->current_interpolation = NEAREST_NEIGHBOR;
  ui_study->current_time = 0.0+CLOSE;
  ui_study->current_duration = 1.0-CLOSE;
  ui_study->current_thickness = -1.0;
  ui_study->current_view_center.x =
    ui_study->current_view_center.y =
    ui_study->current_view_center.z = -1.0;
  ui_study->default_roi_grain =  GRAINS_1;
  ui_study->threshold = NULL;
  ui_study->series = NULL;
  ui_study->time_dialog = NULL;

  /* load in the cursors */
  ui_study->cursor[UI_STUDY_DEFAULT] = NULL;
  ui_study->cursor[UI_STUDY_NEW_ROI_MODE] = 
    gdk_cursor_new(UI_STUDY_NEW_ROI_MODE_CURSOR);
  ui_study->cursor[UI_STUDY_NEW_ROI_MOTION] = 
    gdk_cursor_new(UI_STUDY_NEW_ROI_MOTION_CURSOR);
  ui_study->cursor[UI_STUDY_NO_ROI_MODE] = 
    gdk_cursor_new(UI_STUDY_NO_ROI_MODE_CURSOR);
  ui_study->cursor[UI_STUDY_OLD_ROI_MODE] = 
    gdk_cursor_new(UI_STUDY_OLD_ROI_MODE_CURSOR);
  ui_study->cursor[UI_STUDY_OLD_ROI_RESIZE] = 
    gdk_cursor_new(UI_STUDY_OLD_ROI_RESIZE_CURSOR);
  ui_study->cursor[UI_STUDY_OLD_ROI_ROTATE] = 
    gdk_cursor_new(UI_STUDY_OLD_ROI_ROTATE_CURSOR);
  ui_study->cursor[UI_STUDY_OLD_ROI_SHIFT] = 
    gdk_cursor_new(UI_STUDY_OLD_ROI_SHIFT_CURSOR);
  ui_study->cursor[UI_STUDY_VOLUME_MODE] = 
    gdk_cursor_new(UI_STUDY_VOLUME_MODE_CURSOR);
  ui_study->cursor[UI_STUDY_WAIT] = gdk_cursor_new(UI_STUDY_WAIT_CURSOR);
  ui_study->cursor_stack = NULL; /* default cursor is NULL, so this works.*/

 return ui_study;
}

/* function to figure out a hypothetical coord_frame given a set of slices 
   and the viewing position, also fills in pfar_corner with the far corner
   of the viewing frame */
realspace_t ui_study_get_coords_current_view(ui_study_t * ui_study, view_t view, 
					     realpoint_t * pfar_corner) {
  realspace_t view_coord_frame;
  amide_volume_list_t * current_slices;
  realpoint_t temp_corner[2];

  /* get the current slices for the given view */
  current_slices = ui_study->current_slices[view];

  /* set the origin of the view_coord_frame to the current view locations */
  view_coord_frame = ui_study->current_view_coord_frame;
  //  view_coord_frame.offset = ui_study->current_axis_p_start;
  view_coord_frame = realspace_get_orthogonal_coord_frame(view_coord_frame, view);
  
  /* figure out the corners */
  volumes_get_view_corners(current_slices, view_coord_frame, temp_corner);

  /* allright, reset the offset of the view frame to the lower left front corner */
  view_coord_frame.offset = temp_corner[0];

  /* and set the upper right back corner */
  (*pfar_corner) = realspace_base_coord_to_alt(temp_corner[1],view_coord_frame);

  return view_coord_frame;
}

/* function to update the adjustments for the plane scale */
GtkAdjustment * ui_study_update_plane_adjustment(ui_study_t * ui_study, view_t view) {

  realspace_t view_coord_frame;
  floatpoint_t length;
  floatpoint_t min_dim;
  floatpoint_t zp_start;

  if (ui_study->current_volumes != NULL) {

    view_coord_frame = ui_study->current_view_coord_frame;
    view_coord_frame.offset = 
      realspace_alt_coord_to_base(ui_study->current_view_center,view_coord_frame);
    view_coord_frame = realspace_get_orthogonal_coord_frame(view_coord_frame, view);

    length = ui_study_volumes_get_length(ui_study->current_volumes, view_coord_frame);

    min_dim = ui_study_volumes_min_dim(ui_study->current_volumes);

    switch(view) {
      case TRANSVERSE:
	zp_start = ui_study->current_view_center.z;
	break;
      case CORONAL:
	zp_start = ui_study->current_view_center.y;
	break;
      case SAGITTAL:
      default:
	zp_start = ui_study->current_view_center.x;
	break;
      }


    if ((zp_start > length) || (zp_start < 0.0)) {
      zp_start = length/2.0;
      switch(view) {
      case TRANSVERSE:
	ui_study->current_view_center.z = zp_start;
	break;
      case CORONAL:
	ui_study->current_view_center.y = zp_start;
	break;
      case SAGITTAL:
	ui_study->current_view_center.x = zp_start;
	break;
      default:
	break;
      }
    }

    ui_study->plane_adjustment[view]->upper = length;
    ui_study->plane_adjustment[view]->lower = 0.0;
    ui_study->plane_adjustment[view]->page_increment = min_dim;
    ui_study->plane_adjustment[view]->page_size = min_dim;
    ui_study->plane_adjustment[view]->value = zp_start;
    gtk_adjustment_changed(ui_study->plane_adjustment[view]);
    return NULL;
  } else {
    min_dim = 1.0;
    length = 0.0;
    return GTK_ADJUSTMENT(gtk_adjustment_new(length/2,0,length, 
				      min_dim,min_dim,min_dim));

  }
}



/* updates the settings of the thickness_adjustment, will not change anything about the canvas */
void ui_study_update_thickness_adjustment(ui_study_t * ui_study) { 

  floatpoint_t min_dim, max_dim;
  amide_volume_t * volume;

  if (ui_study->study->volumes != NULL) {

    /* figure out which volume we're dealing with */
    if (ui_study->current_volume == NULL)
      volume = ui_study->study->volumes->volume;
    else
      volume = ui_study->current_volume;

    /* block signals to the thickness adjustment, as we only want to
       change the value of the adjustment, it's up to the caller of this
       function to change anything on the actual canvases... we'll 
       unblock at the end of this function */
    gtk_signal_handler_block_by_func(GTK_OBJECT(ui_study->thickness_adjustment),
				     GTK_SIGNAL_FUNC(ui_study_callbacks_thickness), 
				     ui_study);

    
    min_dim = REALPOINT_MIN_DIM(volume->voxel_size);
    max_dim = REALPOINT_MAX_DIM(volume->voxel_size) *
      REALPOINT_MAX(volume->dim);
    /* sanity check, current thickness should already be set */
    if (ui_study->current_thickness < 0.0)
      ui_study->current_thickness = min_dim;

    ui_study->thickness_adjustment->upper = max_dim;
    ui_study->thickness_adjustment->lower = min_dim;
    ui_study->thickness_adjustment->page_size = min_dim;
    ui_study->thickness_adjustment->step_increment = min_dim;
    ui_study->thickness_adjustment->page_increment = min_dim;
    ui_study->thickness_adjustment->value = 
      ui_study->current_thickness;
    gtk_adjustment_changed(ui_study->thickness_adjustment);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_study->thickness_spin_button),
			      ui_study->current_thickness);   
    gtk_spin_button_configure(GTK_SPIN_BUTTON(ui_study->thickness_spin_button),
			      ui_study->thickness_adjustment,
			      ui_study->current_thickness,
			      2);
    /* and now, reconnect the signal */
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(ui_study->thickness_adjustment), 
				       GTK_SIGNAL_FUNC(ui_study_callbacks_thickness), 
				       ui_study);
    return;
  }
}




/* function to update the arrows on the canvas */
void ui_study_update_canvas_arrows(ui_study_t * ui_study, view_t view) {

  GnomeCanvasPoints * points[4];
  floatpoint_t x1,y1,x2,y2;
  floatpoint_t width=0.0, height=0.0;
  realspace_t view_coord_frame;

  points[0] = gnome_canvas_points_new(4);
  points[1] = gnome_canvas_points_new(4);
  points[2] = gnome_canvas_points_new(4);
  points[3] = gnome_canvas_points_new(4);

  if ((ui_study->study->volumes == NULL) || (ui_study->current_volumes == NULL)) {
    x1 = y1 = x2 = y2 = 0.5;
  } else {

    view_coord_frame = ui_study->current_view_coord_frame;
    view_coord_frame.offset = 
      realspace_alt_coord_to_base(ui_study->current_view_center,view_coord_frame);
    view_coord_frame = realspace_get_orthogonal_coord_frame(view_coord_frame, view);
    width = ui_study_volumes_get_width(ui_study->current_volumes, view_coord_frame);
    height = ui_study_volumes_get_height(ui_study->current_volumes, view_coord_frame);
    
    /* figure out the x and y coordiantes we're currently pointing to */
    switch(view) {
    case TRANSVERSE:
      x1 = ui_study->current_view_center.x;
      if (x1 < 0.0) x1 = ui_study->current_view_center.x = width/2.0;
      y1 = ui_study->current_view_center.y;
      if (y1 < 0.0) y1 = ui_study->current_view_center.y = height/2.0;
      break;
    case CORONAL:
      x1 = ui_study->current_view_center.x;
      if (x1 < 0.0) x1 = ui_study->current_view_center.x = width/2.0;
      y1 = ui_study->current_view_center.z;
      if (y1 < 0.0) y1 = ui_study->current_view_center.z = height/2.0;
      break;
    case SAGITTAL:
      x1 = ui_study->current_view_center.y;
      if (x1 < 0.0) x1 = ui_study->current_view_center.y = width/2.0;
      y1 = ui_study->current_view_center.z;
      if (y1 < 0.0) y1 = ui_study->current_view_center.z = height/2.0;
      break;
    default:
      x1 = y1 = 0.5;
      break;
    }
  }

  x2 = x1+ui_study->current_thickness;
  y2 = y1+ui_study->current_thickness;

  /* notes:
     1) even coords are the x coordinate, odd coords are the y
     2) drawing coordinate frame starts from the top left
  */

  /* left arrow */
  points[0]->coords[0] = UI_STUDY_TRIANGLE_HEIGHT-1;
  points[0]->coords[1] = UI_STUDY_TRIANGLE_HEIGHT-1 + 
    ui_study->rgb_image[view]->rgb_height * (y1/height);
  points[0]->coords[2] = UI_STUDY_TRIANGLE_HEIGHT-1;
  points[0]->coords[3] = UI_STUDY_TRIANGLE_HEIGHT-1 + 
    ui_study->rgb_image[view]->rgb_height * (y2/height);
  points[0]->coords[4] = 0;
  points[0]->coords[5] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    UI_STUDY_TRIANGLE_WIDTH/2 +
    ui_study->rgb_image[view]->rgb_height * (y2/height);
  points[0]->coords[6] = 0;
  points[0]->coords[7] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    (-UI_STUDY_TRIANGLE_WIDTH/2) +
    ui_study->rgb_image[view]->rgb_height * (y1/height);

  /* top arrow */
  points[1]->coords[0] = UI_STUDY_TRIANGLE_HEIGHT-1 + 
    ui_study->rgb_image[view]->rgb_width * (x1/width);
  points[1]->coords[1] = UI_STUDY_TRIANGLE_HEIGHT-1;
  points[1]->coords[2] = UI_STUDY_TRIANGLE_HEIGHT-1 + 
    ui_study->rgb_image[view]->rgb_width * (x2/width);
  points[1]->coords[3] = UI_STUDY_TRIANGLE_HEIGHT-1;
  points[1]->coords[4] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    UI_STUDY_TRIANGLE_WIDTH/2 +
    ui_study->rgb_image[view]->rgb_width * (x2/width);
  points[1]->coords[5] = 0;
  points[1]->coords[6] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    (-UI_STUDY_TRIANGLE_WIDTH/2) +
    ui_study->rgb_image[view]->rgb_width * (x1/width);
  points[1]->coords[7] = 0;


  /* right arrow */
  points[2]->coords[0] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    ui_study->rgb_image[view]->rgb_width;
  points[2]->coords[1] = UI_STUDY_TRIANGLE_HEIGHT-1 + 
    ui_study->rgb_image[view]->rgb_height * (y1/height);
  points[2]->coords[2] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    ui_study->rgb_image[view]->rgb_width;
  points[2]->coords[3] = UI_STUDY_TRIANGLE_HEIGHT-1 + 
    ui_study->rgb_image[view]->rgb_height * (y2/height);
  points[2]->coords[4] = 2*UI_STUDY_TRIANGLE_HEIGHT-1 +
    ui_study->rgb_image[view]->rgb_width;
  points[2]->coords[5] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    UI_STUDY_TRIANGLE_WIDTH/2 +
    ui_study->rgb_image[view]->rgb_height * (y2/height);
  points[2]->coords[6] = 2*UI_STUDY_TRIANGLE_HEIGHT-1 +
    ui_study->rgb_image[view]->rgb_width;
  points[2]->coords[7] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    (-UI_STUDY_TRIANGLE_WIDTH/2) +
    ui_study->rgb_image[view]->rgb_height * (y1/height);


  /* bottom arrow */
  points[3]->coords[0] = UI_STUDY_TRIANGLE_HEIGHT-1 + 
    ui_study->rgb_image[view]->rgb_width * (x1/width);
  points[3]->coords[1] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    ui_study->rgb_image[view]->rgb_height;
  points[3]->coords[2] = UI_STUDY_TRIANGLE_HEIGHT-1 + 
    ui_study->rgb_image[view]->rgb_width * (x2/width);
  points[3]->coords[3] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    ui_study->rgb_image[view]->rgb_height;
  points[3]->coords[4] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    UI_STUDY_TRIANGLE_WIDTH/2 +
    ui_study->rgb_image[view]->rgb_width * (x2/width);
  points[3]->coords[5] = 2*UI_STUDY_TRIANGLE_HEIGHT-1 +
    ui_study->rgb_image[view]->rgb_height;
  points[3]->coords[6] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    (-UI_STUDY_TRIANGLE_WIDTH/2) +
    ui_study->rgb_image[view]->rgb_width * (x1/width);
  points[3]->coords[7] =  2*UI_STUDY_TRIANGLE_HEIGHT-1 +
    ui_study->rgb_image[view]->rgb_height;

  if (ui_study->canvas_arrow[view][0] != NULL ) {
    /* update the little arrow thingies */
    gnome_canvas_item_set(ui_study->canvas_arrow[view][0],"points",points[0], NULL);
    gnome_canvas_item_set(ui_study->canvas_arrow[view][1],"points",points[1], NULL);
    gnome_canvas_item_set(ui_study->canvas_arrow[view][2],"points",points[2], NULL);
    gnome_canvas_item_set(ui_study->canvas_arrow[view][3],"points",points[3], NULL);

  } else {
    /* create those little arrow things*/
    ui_study->canvas_arrow[view][0] = 
      gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[view]),
			    gnome_canvas_polygon_get_type(),
			    "points", points[0],"fill_color", "white",
			    "outline_color", "black", "width_pixels", 2,
			    NULL);
    ui_study->canvas_arrow[view][1] = 
      gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[view]),
			    gnome_canvas_polygon_get_type(),
			    "points", points[1],"fill_color", "white",
			    "outline_color", "black", "width_pixels", 2,
			    NULL);
    ui_study->canvas_arrow[view][2] = 
      gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[view]),
			    gnome_canvas_polygon_get_type(),
			    "points", points[2],"fill_color", "white",
			    "outline_color", "black", "width_pixels", 2,
			    NULL);
    ui_study->canvas_arrow[view][3] = 
      gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[view]),
			    gnome_canvas_polygon_get_type(),
			    "points", points[3],"fill_color", "white",
			    "outline_color", "black", "width_pixels", 2,
			    NULL);
  }

  gnome_canvas_points_unref(points[0]);
  gnome_canvas_points_unref(points[1]);
  gnome_canvas_points_unref(points[2]);
  gnome_canvas_points_unref(points[3]);

  return;
}



/* function to update the canvas image*/
void ui_study_update_canvas_image(ui_study_t * ui_study, view_t view) {

  realspace_t view_coord_frame;
  ui_study_volume_list_t * temp_ui_study_volumes;
  amide_volume_list_t * temp_volumes=NULL;

  //  GtkRequisition requisition;

  /* if we don't have anything currently selected, get out of here */
  if (ui_study->current_volumes == NULL)
    return;

  /* figure out our view coordinate frame */
  view_coord_frame = ui_study->current_view_coord_frame;
    view_coord_frame.offset = 
      realspace_alt_coord_to_base(ui_study->current_view_center,view_coord_frame);
  view_coord_frame = realspace_get_orthogonal_coord_frame(view_coord_frame, view);

  if (ui_study->rgb_image[view] != NULL)
    gnome_canvas_destroy_image(ui_study->rgb_image[view]);

  /* first, generate an amide_volume_list we can pass to image_from_volumes */
  temp_ui_study_volumes = ui_study->current_volumes;
  while (temp_ui_study_volumes != NULL) {
    volume_list_add_volume(&temp_volumes, temp_ui_study_volumes->volume);
    temp_ui_study_volumes = temp_ui_study_volumes->next;
  }

  ui_study->rgb_image[view] = image_from_volumes(&(ui_study->current_slices[view]),
						 temp_volumes,
						 ui_study->current_time,
						 ui_study->current_duration,
						 ui_study->current_thickness,
						 view_coord_frame,
						 ui_study->scaling,
						 ui_study->current_zoom,
						 ui_study->current_interpolation);

  /* and delete the amide_volume_list without destroying the volumes */
  temp_ui_study_volumes = ui_study->current_volumes;
  while (temp_ui_study_volumes != NULL) {
    volume_list_remove_volume(&temp_volumes, temp_ui_study_volumes->volume);
    temp_ui_study_volumes = temp_ui_study_volumes->next;
  }

  /* reset the min size of the widget */
  gnome_canvas_set_scroll_region(ui_study->canvas[view], 0.0, 0.0, 
  				 ui_study->rgb_image[view]->rgb_width  
				 + 2 * UI_STUDY_TRIANGLE_HEIGHT,
				 ui_study->rgb_image[view]->rgb_height  
				 + 2 * UI_STUDY_TRIANGLE_HEIGHT);

  //  requisition.width = ui_study->rgb_image[view]->rgb_width;
  //  requisition.height = ui_study->rgb_image[view]->rgb_height;
  //  gtk_widget_size_request(GTK_WIDGET(ui_study->canvas[view]), &requisition);
  gtk_widget_set_usize(GTK_WIDGET(ui_study->canvas[view]), 
		       ui_study->rgb_image[view]->rgb_width 
		       + 2 * UI_STUDY_TRIANGLE_HEIGHT, 
		       ui_study->rgb_image[view]->rgb_height 
		       + 2 * UI_STUDY_TRIANGLE_HEIGHT);

  /* put up the image */
  gnome_canvas_item_set(ui_study->canvas_image[view],
			"image", ui_study->rgb_image[view],
			"width", (double) ui_study->rgb_image[view]->rgb_width,
			"height", (double) ui_study->rgb_image[view]->rgb_height,
			NULL);

  return;
}


/* replaces the current cursor with the watch/hourglass/whatever cursor */
void ui_study_place_wait_cursor(ui_study_t * ui_study) {

  GdkCursor * cursor;

  /* push our desired cursor onto the cursor stack */
  cursor = ui_study->cursor[UI_STUDY_WAIT];
  ui_study->cursor_stack = g_slist_prepend(ui_study->cursor_stack,cursor);
  gdk_window_set_cursor(gtk_widget_get_parent_window(GTK_WIDGET(ui_study->canvas[0])), cursor);


  /* do any events pending, this allows the cursor to get displayed */
  while (gtk_events_pending()) 
    gtk_main_iteration();

  return;
}

/* removes the watch/hourglass/whatever cursor, going back to the previous cursor */
void ui_study_remove_wait_cursor(ui_study_t * ui_study) {

  GdkCursor * cursor;

  /* pop the previous cursor off the stack */
  cursor = g_slist_nth_data(ui_study->cursor_stack, 0);
  ui_study->cursor_stack = g_slist_remove(ui_study->cursor_stack, cursor);
  cursor = g_slist_nth_data(ui_study->cursor_stack, 0);
  gdk_window_set_cursor(gtk_widget_get_parent_window(GTK_WIDGET(ui_study->canvas[0])), cursor);

  return;
}

/* function to update a canvas, if view_it is  NUM_VIEWS, update
   all canvases */
void ui_study_update_canvas(ui_study_t * ui_study, view_t i, 
			    ui_study_update_t update) {

  view_t j,k;
  realpoint_t temp_center;
  realpoint_t view_corner[2];

  if (i==NUM_VIEWS) {
    i=0;
    j=NUM_VIEWS;
  } else
    j=i+1;

  ui_study_place_wait_cursor(ui_study);

  /* make sure the view_coord_frame offset is set correctly, 
     adjust current_view_center if necessary */
  temp_center = realspace_alt_coord_to_base(ui_study->current_view_center,
    					    ui_study->current_view_coord_frame);
  volumes_get_view_corners(ui_study->study->volumes,ui_study->current_view_coord_frame, view_corner);
  ui_study->current_view_coord_frame.offset=view_corner[0];
  ui_study->current_view_center = 
    realspace_base_coord_to_alt(temp_center, ui_study->current_view_coord_frame);


  for (k=i;k<j;k++) {
    switch (update) {
    case REFRESH_IMAGE:
      /* refresh the image, but use the same slice as before */
      ui_study_update_canvas_image(ui_study, k);
      break;
    case UPDATE_IMAGE:
      /* indicates to regenerate the slices we're looking at */      
      volume_list_free(&(ui_study->current_slices[k])); 
      ui_study_update_canvas_image(ui_study, k);       /* refresh the image */
      break;
    case UPDATE_ROIS:
      ui_study_rois_update_canvas_rois(ui_study, k);
      break;
    case UPDATE_ARROWS:
      ui_study_update_canvas_arrows(ui_study,k);
      break;
    case UPDATE_PLANE_ADJUSTMENT:
      ui_study_update_plane_adjustment(ui_study, k);
      break;
    case UPDATE_ALL:
    default:
      /* indicates to regenerate everything */
      volume_list_free(&(ui_study->current_slices[k])); 
      ui_study_update_canvas_image(ui_study, k);
      ui_study_rois_update_canvas_rois(ui_study, k);
      ui_study_update_plane_adjustment(ui_study, k);
      ui_study_update_canvas_arrows(ui_study,k);
      break;
    }

  }

  ui_study_remove_wait_cursor(ui_study);

  return;
}



/* function adds a roi item to the tree */
void ui_study_tree_add_roi(ui_study_t * ui_study, amide_roi_t * roi) {

  GdkPixmap * pixmap;
  GdkWindow * parent;
  gchar * tree_buf[2];

  parent = gtk_widget_get_parent_window(ui_study->tree);

  /* which icon to use */
  switch (roi->type) {
  case ELLIPSOID:
    pixmap = gdk_pixmap_create_from_xpm_d(parent,NULL,NULL,ROI_xpm);
    break;
  case CYLINDER:
    pixmap = gdk_pixmap_create_from_xpm_d(parent,NULL,NULL,ROI_xpm);
    break;
  case BOX:
    pixmap = gdk_pixmap_create_from_xpm_d(parent,NULL,NULL,ROI_xpm);
    break;
  case GROUP:
  default:
    pixmap = gdk_pixmap_create_from_xpm_d(parent,NULL,NULL,ROI_xpm);
    break;
  }

  tree_buf[0] = roi->name;
  tree_buf[1] = NULL;
  ui_study->tree_rois = /* now points to current node */
    gtk_ctree_insert_node(GTK_CTREE(ui_study->tree),
			  ui_study->tree_studies, /* parent */
			  ui_study->tree_rois, /* siblings */
			  tree_buf,
			  5, /* spacing */
			  pixmap, NULL, pixmap, NULL, FALSE, TRUE);

  gtk_ctree_node_set_row_data(GTK_CTREE(ui_study->tree),
			      ui_study->tree_rois,
			      roi);
  return;
}

/* function adds a volume item to the tree */
void ui_study_tree_add_volume(ui_study_t * ui_study, amide_volume_t * volume) {

  GdkPixmap * pixmap;
  GdkWindow * parent;
  gchar * tree_buf[2];

  parent = gtk_widget_get_parent_window(ui_study->tree);

  /* which icon to use */
  switch (volume->modality) {
  case SPECT:
    pixmap = gdk_pixmap_create_from_xpm_d(parent,NULL,NULL,SPECT_xpm);
    break;
  case MRI:
    pixmap = gdk_pixmap_create_from_xpm_d(parent,NULL,NULL,MRI_xpm);
    break;
  case CT:
    pixmap = gdk_pixmap_create_from_xpm_d(parent,NULL,NULL,CT_xpm);
    break;
  case OTHER:
    pixmap = gdk_pixmap_create_from_xpm_d(parent,NULL,NULL,OTHER_xpm);
    break;
  case PET:
  default:
    pixmap = gdk_pixmap_create_from_xpm_d(parent,NULL,NULL,PET_xpm);
    break;
  }

  
  tree_buf[0] = volume->name;
  tree_buf[1] = NULL;
  ui_study->tree_volumes =
    gtk_ctree_insert_node(GTK_CTREE(ui_study->tree),
			  ui_study->tree_studies, /* parent */
			  ui_study->tree_volumes,
			  tree_buf,
			  5, /* spacing */
			  pixmap, NULL, pixmap, NULL, FALSE, TRUE);

  gtk_ctree_node_set_row_data(GTK_CTREE(ui_study->tree),
			      ui_study->tree_volumes,
			      volume);
  return;
}


/* function to update the study tree 
   since we use pixmaps in this function, make sure to realize the tree
   before calling this */
void ui_study_update_tree(ui_study_t * ui_study) {

  GdkPixmap * pixmap;
  gchar * tree_buf[2];
  amide_volume_list_t * volume_list;
  amide_roi_list_t * roi_list;
  
  /* make the primary nodes if needed*/
  if (ui_study->tree_studies == NULL) {
    pixmap = 
      gdk_pixmap_create_from_xpm_d(gtk_widget_get_parent_window(ui_study->tree),
				   NULL,NULL,study_xpm);
    
    /* put the current study into the tree */
    tree_buf[0] = ui_study->study->name;
    tree_buf[1] = NULL;
    ui_study->tree_studies =
      gtk_ctree_insert_node(GTK_CTREE(ui_study->tree),
			    NULL, /* parent */
			    ui_study->tree_studies, /* siblings */
			    tree_buf,
			    5, /* spacing */
			    pixmap, NULL, pixmap, NULL, FALSE, TRUE);

    gtk_ctree_node_set_row_data(GTK_CTREE(ui_study->tree),
				ui_study->tree_studies,
				NULL);


    /* if there are any volumes, place them on in */
    if (ui_study->study->volumes != NULL) {
      volume_list = ui_study->study->volumes;
      while (volume_list != NULL) {
	ui_study_tree_add_volume(ui_study, volume_list->volume);
	volume_list = volume_list->next;
      }
    }

    /* if there are any rois, place them on in */
    if (ui_study->study->rois != NULL) {
      roi_list = ui_study->study->rois;
      while (roi_list != NULL) {
	ui_study_tree_add_roi(ui_study, roi_list->roi);
	roi_list = roi_list->next;
      }
    }
  }

  return;
}

/* function to setup the widgets inside of the GnomeApp study */
void ui_study_setup_widgets(ui_study_t * ui_study) {

  GtkWidget * packing_table;
  GtkWidget * right_table;
  GtkWidget * left_table;
  GtkWidget * label;
  GtkWidget * scale;
  GtkWidget * dial;
  GtkAdjustment * adjustment;
  GtkWidget * option_menu;
  GtkWidget * menu;
  GtkWidget * menuitem;
  GtkWidget * button;
  GtkWidget * spin_button;
  GtkWidget * tree;
  GtkWidget * scrolled;
  view_t i;
  scaling_t i_scaling;
  color_table_t i_color_table;
  interpolation_t i_interpolation;
  roi_type_t i_roi_type;
  guint packing_table_row, packing_table_column;
  guint left_table_row, right_table_row;  
  gchar * temp_string;


  /* make and add the packing table */
  packing_table = gtk_table_new(UI_STUDY_PACKING_TABLE_WIDTH,
				UI_STUDY_PACKING_TABLE_HEIGHT,FALSE);
  packing_table_row = packing_table_column=0;
  gnome_app_set_contents(ui_study->app, GTK_WIDGET(packing_table));

  /* setup the left tree widget */
  left_table = gtk_table_new(2,5, FALSE);
  left_table_row=0;
  gtk_table_attach(GTK_TABLE(packing_table), 
		   left_table, 
		   packing_table_column, packing_table_column+1, 
		   packing_table_row, UI_STUDY_PACKING_TABLE_HEIGHT,
		   X_PACKING_OPTIONS | GTK_FILL, 
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);
  packing_table_column++;

  /* make a scrolled area for the tree */
  scrolled = gtk_scrolled_window_new(NULL,NULL);
  gtk_widget_set_usize(scrolled,150,-1);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				 GTK_POLICY_AUTOMATIC,
				 GTK_POLICY_AUTOMATIC);
  gtk_table_attach(GTK_TABLE(left_table),
		   scrolled,
		   0,2, left_table_row, left_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 
		   Y_PACKING_OPTIONS | GTK_FILL, 
		   X_PADDING, Y_PADDING);
  left_table_row++;

  /* make the tree */
  tree = gtk_ctree_new(1,0);
  gtk_clist_set_row_height(GTK_CLIST(tree),UI_STUDY_SIZE_TREE_PIXMAPS);
  gtk_clist_set_selection_mode(GTK_CLIST(tree), GTK_SELECTION_MULTIPLE);
  gtk_signal_connect(GTK_OBJECT(tree), "tree_select_row",
		     GTK_SIGNAL_FUNC(ui_study_callback_tree_select_row),
		     ui_study);
  gtk_signal_connect(GTK_OBJECT(tree), "tree_unselect_row",
		     GTK_SIGNAL_FUNC(ui_study_callback_tree_unselect_row),
		     ui_study);
  gtk_signal_connect(GTK_OBJECT(tree), "button_press_event",
		     GTK_SIGNAL_FUNC(ui_study_callback_tree_click_row),
		     ui_study);

  gtk_container_add(GTK_CONTAINER(scrolled),tree);
  ui_study->tree = tree;
  gtk_widget_realize(tree); /* realize now so we can add pixmaps to the tree now */

  /* populate the tree */
  ui_study_update_tree(ui_study);


  /* add an roi type selector */
  label = gtk_label_new("add roi:");
  gtk_table_attach(GTK_TABLE(left_table), 
		   GTK_WIDGET(label), 0,1, 
		   left_table_row,left_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();
  ui_study->add_roi_option_menu = option_menu;

  menuitem = gtk_menu_item_new_with_label(""); /* add a blank menu item */
  gtk_menu_append(GTK_MENU(menu), menuitem);
  for (i_roi_type=0; i_roi_type<NUM_ROI_TYPES; i_roi_type++) {
    menuitem = gtk_menu_item_new_with_label(roi_type_names[i_roi_type]);
    gtk_menu_append(GTK_MENU(menu), menuitem);
    gtk_object_set_data(GTK_OBJECT(menuitem), "roi_type", roi_type_names[i_roi_type]); 
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
      		       GTK_SIGNAL_FUNC(ui_study_callbacks_add_roi_type), 
    		       ui_study);
  }
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_table_attach(GTK_TABLE(left_table), 
		   GTK_WIDGET(option_menu), 1,2, 
		   left_table_row,left_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  left_table_row++;


  /* do we want to edit an object in the study */
  button = gtk_button_new_with_label("edit object(s)");
  gtk_table_attach(GTK_TABLE(left_table), 
		   GTK_WIDGET(button), 0,2, 
		   left_table_row,left_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_signal_connect(GTK_OBJECT(button), "pressed",
		     GTK_SIGNAL_FUNC(ui_study_callbacks_edit_object_pressed), 
		     ui_study);
  left_table_row++;



  /* and if we want to delete an object from the study */
  button = gtk_button_new_with_label("delete object(s)");
  gtk_table_attach(GTK_TABLE(left_table), 
		   GTK_WIDGET(button), 0,2, 
		   left_table_row,left_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_signal_connect(GTK_OBJECT(button), "pressed",
		     GTK_SIGNAL_FUNC(ui_study_callbacks_delete_object_pressed), 
		     ui_study);
  left_table_row++;




  /* make the three canvases, scales, dials, etc. */
  for (i=0;i<NUM_VIEWS;i++) {
    packing_table_row=0;

    /* make the label for this column */
    label = gtk_label_new(view_names[i]);
    gtk_table_attach(GTK_TABLE(packing_table), 
		     label, 
		     packing_table_column, packing_table_column+1, 
		     packing_table_row, packing_table_row+1,
		     GTK_FILL, FALSE, X_PADDING, Y_PADDING);
    packing_table_row++;

    /* canvas section */
    //    ui_study->canvas[i] = GNOME_CANVAS(gnome_canvas_new_aa());
    ui_study->canvas[i] = GNOME_CANVAS(gnome_canvas_new());

    /* throw something into the canvas */
    if (ui_study->study->volumes == NULL) {
      GnomeCanvasItem * temp_canvas_image;

      ui_study->rgb_image[i] = image_blank(UI_STUDY_BLANK_WIDTH,
					   UI_STUDY_BLANK_HEIGHT);
      gnome_canvas_set_scroll_region(ui_study->canvas[i], 0.0, 0.0, 
				     ui_study->rgb_image[i]->rgb_width
				     +2*UI_STUDY_TRIANGLE_HEIGHT,
				     ui_study->rgb_image[i]->rgb_height
				     +2*UI_STUDY_TRIANGLE_HEIGHT);
      gtk_widget_set_usize(GTK_WIDGET(ui_study->canvas[i]), 
			   ui_study->rgb_image[i]->rgb_width
			   +2*UI_STUDY_TRIANGLE_HEIGHT, 
			   ui_study->rgb_image[i]->rgb_height
			   +2*UI_STUDY_TRIANGLE_HEIGHT);

      temp_canvas_image = 
	gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[i]),
			      gnome_canvas_image_get_type(),
			      "image", ui_study->rgb_image[i],
			      "x", (double) UI_STUDY_TRIANGLE_HEIGHT,
			      "y", (double) UI_STUDY_TRIANGLE_HEIGHT,
			      "anchor", GTK_ANCHOR_NORTH_WEST,
			      "width",(double) ui_study->rgb_image[i]->rgb_width,
			      "height",(double) ui_study->rgb_image[i]->rgb_height,
			      NULL);
      ui_study->canvas_image[i] = temp_canvas_image;
      gtk_signal_connect(GTK_OBJECT(ui_study->canvas_image[i]), "event",
			 GTK_SIGNAL_FUNC(ui_study_callbacks_canvas_event),
			 ui_study);

      /* put up the arrows */
      ui_study_update_canvas_arrows(ui_study,i);

    } else
      ; /* load in a volume */


    gtk_table_attach(GTK_TABLE(packing_table), 
		     GTK_WIDGET(ui_study->canvas[i]), 
		     packing_table_column, packing_table_column+1,
		     packing_table_row, packing_table_row+1,
		     FALSE,FALSE, X_PADDING, Y_PADDING);
    packing_table_row++;


    /* scale section */
    adjustment = ui_study_update_plane_adjustment(ui_study, i);
    /*so we can figure out which adjustment this is in callbacks */
    gtk_object_set_data(GTK_OBJECT(adjustment), "view", view_names[i]); 
    ui_study->plane_adjustment[i] = adjustment;/*save this, so we can change it*/
    scale = gtk_hscale_new(adjustment);
    gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_DISCONTINUOUS);
    gtk_table_attach(GTK_TABLE(packing_table), 
		     GTK_WIDGET(scale), 
		     packing_table_column, packing_table_column+1,
		     packing_table_row, packing_table_row+1,
		     GTK_FILL,FALSE, X_PADDING, Y_PADDING);
    gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed", 
		       GTK_SIGNAL_FUNC(ui_study_callbacks_plane_change), 
		       ui_study);
    packing_table_row++;


    /* dial section */
    adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0,-45.0,45.5,0.5,0.5,0.5));
    /*so we can figure out which adjustment this is in callbacks */
    gtk_object_set_data(GTK_OBJECT(adjustment), "view", view_names[i]); 
    dial = gtk_hscale_new(adjustment);
    gtk_range_set_update_policy(GTK_RANGE(dial), GTK_UPDATE_DISCONTINUOUS);
    gtk_table_attach(GTK_TABLE(packing_table), 
		     GTK_WIDGET(dial),
		     packing_table_column, packing_table_column+1,
		     packing_table_row, packing_table_row+1,
		     GTK_FILL,FALSE, X_PADDING, Y_PADDING);
    gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed", 
		       GTK_SIGNAL_FUNC(ui_study_callbacks_axis_change), 
		       ui_study);
    packing_table_row++;

    /* I should hook up a entry widget to this to allow more fine settings */

    packing_table_column++;
  }
  packing_table_row=0;

  /* things to put in the right most column */
  right_table = gtk_table_new(UI_STUDY_RIGHT_TABLE_WIDTH,
			      UI_STUDY_RIGHT_TABLE_HEIGHT,FALSE);
  right_table_row=0;

  /* selecting per slice/global file normalization */
  label = gtk_label_new("scaling:");
  gtk_table_attach(GTK_TABLE(right_table), 
		   GTK_WIDGET(label), 0,1, 
		   right_table_row,right_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();

  for (i_scaling=0; i_scaling<NUM_SCALINGS; i_scaling++) {
    menuitem = gtk_menu_item_new_with_label(scaling_names[i_scaling]);
    gtk_menu_append(GTK_MENU(menu), menuitem);
    gtk_object_set_data(GTK_OBJECT(menuitem), "scaling", scaling_names[i_scaling]);
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
		       GTK_SIGNAL_FUNC(ui_study_callbacks_scaling), ui_study);
  }

  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), ui_study->scaling);

  gtk_table_attach(GTK_TABLE(right_table), 
		   GTK_WIDGET(option_menu), 1,2,
		   right_table_row,right_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  right_table_row++;

  /* color table selector */
  label = gtk_label_new("color table:");
  gtk_table_attach(GTK_TABLE(right_table), 
		   GTK_WIDGET(label), 0,1, 
		   right_table_row,right_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();

  for (i_color_table=0; i_color_table<NUM_COLOR_TABLES; i_color_table++) {
    menuitem = gtk_menu_item_new_with_label(color_table_names[i_color_table]);
    gtk_menu_append(GTK_MENU(menu), menuitem);
    gtk_object_set_data(GTK_OBJECT(menuitem), "color_table", color_table_names[i_color_table]);
    gtk_object_set_data(GTK_OBJECT(menuitem),"threshold", NULL);
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
    		       GTK_SIGNAL_FUNC(ui_study_callbacks_color_table), ui_study);
  }

  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), BW_LINEAR);
  ui_study->color_table_menu = option_menu;

  gtk_table_attach(GTK_TABLE(right_table), 
		   GTK_WIDGET(option_menu), 1,2, 
		   right_table_row,right_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  right_table_row++;

  /* interpolation selector */
  label = gtk_label_new("interpolation:");
  gtk_table_attach(GTK_TABLE(right_table), 
		   GTK_WIDGET(label), 0,1, 
		   right_table_row,right_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();

  for (i_interpolation=0; i_interpolation<NUM_INTERPOLATIONS; i_interpolation++) {
    menuitem = gtk_menu_item_new_with_label(interpolation_names[i_interpolation]);
    gtk_menu_append(GTK_MENU(menu), menuitem);
    gtk_object_set_data(GTK_OBJECT(menuitem), "interpolation", interpolation_names[i_interpolation]);
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
    		       GTK_SIGNAL_FUNC(ui_study_callbacks_interpolation), 
		       ui_study);
  }

  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), 
			      ui_study->current_interpolation);
  gtk_table_attach(GTK_TABLE(right_table), 
		   GTK_WIDGET(option_menu), 1,2, 
		   right_table_row,right_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  right_table_row++;

  /* button to get the threshold dialog */
  label = gtk_label_new("threshold:");
  gtk_table_attach(GTK_TABLE(right_table), 
		   GTK_WIDGET(label), 0,1, 
		   right_table_row,right_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  
  button = gtk_button_new_with_label("popup");
  gtk_table_attach(GTK_TABLE(right_table), 
		   GTK_WIDGET(button), 1,2, 
		   right_table_row,right_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_signal_connect(GTK_OBJECT(button), "pressed",
		     GTK_SIGNAL_FUNC(ui_study_callbacks_threshold_pressed), 
		     ui_study);
  right_table_row++;

  /* zoom selector */
  label = gtk_label_new("zoom:");
  gtk_table_attach(GTK_TABLE(right_table), 
		   GTK_WIDGET(label), 0,1, 
		   right_table_row,right_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);

  adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(ui_study->current_zoom,
						 0.2,5,0.2, 0.25, 0.25));
  spin_button = gtk_spin_button_new(adjustment, 0.25, 2);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_button),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(spin_button), FALSE);

  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin_button), 
				    GTK_UPDATE_ALWAYS);

  gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed", 
		     GTK_SIGNAL_FUNC(ui_study_callbacks_zoom), 
		     ui_study);

			      
  gtk_table_attach(GTK_TABLE(right_table), 
		   GTK_WIDGET(spin_button),1,2, 
		   right_table_row,right_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  right_table_row++;

  /* frame selector */
  label = gtk_label_new("time:");
  gtk_table_attach(GTK_TABLE(right_table), 
		   GTK_WIDGET(label), 0,1, 
		   right_table_row,right_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);

  temp_string = g_strdup_printf("%5.1f-%5.1fs",ui_study->current_time, ui_study->current_duration);
  button = gtk_button_new_with_label(temp_string);
  ui_study->time_button = button;
  g_free(temp_string);
  gtk_table_attach(GTK_TABLE(right_table), 
		   GTK_WIDGET(button), 1,2, 
		   right_table_row,right_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_signal_connect(GTK_OBJECT(button), "pressed",
		     GTK_SIGNAL_FUNC(ui_study_callbacks_time_pressed), 
		     ui_study);
  right_table_row++;

  /* width selector */
  label = gtk_label_new("thickness:");
  gtk_table_attach(GTK_TABLE(right_table), 
		   GTK_WIDGET(label), 0,1, 
		   right_table_row,right_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);

  adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(1.0,1.0,1.0,1.0,1.0,1.0));
  ui_study->thickness_adjustment = adjustment;
  ui_study->thickness_spin_button = 
    spin_button = gtk_spin_button_new(adjustment,1.0, 2);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_button),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(spin_button), FALSE);

  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin_button), 
				    GTK_UPDATE_ALWAYS);

  gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed", 
		     GTK_SIGNAL_FUNC(ui_study_callbacks_thickness), 
		     ui_study);

			      
  gtk_table_attach(GTK_TABLE(right_table), 
		   GTK_WIDGET(spin_button), 1,2, 
		   right_table_row,right_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  right_table_row++;


  /* button to reset the axis */
  label = gtk_label_new("axis:");
  gtk_table_attach(GTK_TABLE(right_table), 
		   GTK_WIDGET(label), 0,1, 
		   right_table_row,right_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  
  button = gtk_button_new_with_label("reset");
  gtk_table_attach(GTK_TABLE(right_table), 
		   GTK_WIDGET(button), 1,2, 
		   right_table_row,right_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_signal_connect(GTK_OBJECT(button), "pressed",
		     GTK_SIGNAL_FUNC(ui_study_callbacks_reset_axis), 
		     ui_study);
  right_table_row++;

  /* and add the right column to the main table */
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(right_table), 
		   packing_table_column,packing_table_column+1,
		   packing_table_row, UI_STUDY_PACKING_TABLE_HEIGHT,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);

  return;
}



/* procedure to set up the study window */
void ui_study_create(gchar * study) {

  GnomeApp * app;
  gchar * title=NULL;
  ui_study_t * ui_study;
  gchar * temp_string;


  ui_study = ui_study_init();

  /* setup the study window */
  if (study == NULL) {
    ui_study->study = study_init();
    temp_string = g_strdup_printf("temp_%d",next_study_num++);
    study_set_name(ui_study->study, temp_string);
    title = g_strdup_printf("Study: %s",ui_study->study->name);
    g_free(temp_string);
  } else {
    // load info into ui_study;
    title = g_strdup_printf("Study: %s",ui_study->study->name);
  }

  app=GNOME_APP(gnome_app_new(PACKAGE, title));
  g_free(title);
  ui_study->app = app;

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(app), "delete_event",
		     GTK_SIGNAL_FUNC(ui_study_callbacks_delete_event),
		     ui_study);

  /* setup the study menu */
  ui_study_menus_create(ui_study);

  /* setup the rest of the study window */
  ui_study_setup_widgets(ui_study);

  /* and run the study window */
  gtk_widget_show_all(GTK_WIDGET(app));

  return;
}
