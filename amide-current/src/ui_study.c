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
#include "color_table.h"
#include "study.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_roi.h"
#include "ui_volume.h"
#include "ui_study.h"
#include "ui_study_callbacks.h"
#include "ui_study_menus.h"
#include "ui_study_rois_callbacks.h"

#include "../pixmaps/study.xpm"
#include "../pixmaps/PET.xpm"
#include "../pixmaps/SPECT.xpm"
#include "../pixmaps/CT.xpm"
#include "../pixmaps/MRI.xpm"
#include "../pixmaps/OTHER.xpm"
#include "../pixmaps/ROI.xpm"

/* external variables */

/* internal variables */
static gchar * ui_study_help_info_lines[][NUM_HELP_INFO_LINES] = {
  {"", "", "", "", "", ""}, /* BLANK */
  {"1-Move View", "", "2-Move View, Min. Depth", "", "3-Change Depth",""},
  {"1-Shift", "", "2-Rotate", "", "3-Scale", ""}, /*CANVAS_ROI */
  {"1-select", "", "2-make active", "", "3-pop up dialog", ""}, /* TREE_VOLUME */
  {"1-select", "", "2-make active", "", "3-pop up dialog", ""}, /* TREE_ROI */
  {"", "", "", "", "3-pop up dialog",""}, /* TREE_STUDY */
  {"1-select", "", "2-make active", "", "3-pop up dialog",""} /* TREE_NONE */
};
static gchar * ui_study_tree_active_mark = "*";
static gint next_study_num=1;
static gchar * ui_study_tree_column_titles[] = {"*", "object"};

/* destroy a ui_study data structure */
ui_study_t * ui_study_free(ui_study_t * ui_study) {
  
  if (ui_study == NULL)
    return ui_study;

  /* sanity checks */
  g_return_val_if_fail(ui_study->reference_count > 0, NULL);

  /* remove a reference count */
  ui_study->reference_count--;

  /* things we always do */
  ui_study->current_rois = ui_roi_list_free(ui_study->current_rois);
  ui_study->current_volumes = ui_volume_list_free(ui_study->current_volumes);
  ui_study->study = study_free(ui_study->study);

  /* if we've removed all reference's,f ree the structure */
  if (ui_study->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing ui_study\n");
#endif
    g_free(ui_study);
    ui_study = NULL;
  }
    
  return ui_study;
}

/* malloc and initialize a ui_study data structure */
ui_study_t * ui_study_init(void) {

  ui_study_t * ui_study;

  /* alloc space for the data structure for passing ui info */
  if ((ui_study = (ui_study_t *) g_malloc(sizeof(ui_study_t))) == NULL) {
    g_warning("%s: couldn't allocate space for ui_study_t",PACKAGE);
    return NULL;
  }
  ui_study->reference_count = 1;

  ui_study->tree_study = NULL;
  ui_study->tree_rois = NULL;
  ui_study->tree_volumes = NULL;
  ui_study->study = NULL;
  ui_study->current_mode = VOLUME_MODE;
  ui_study->current_volume = NULL;
  ui_study->current_roi = NULL;
  ui_study->current_volumes = NULL;
  ui_study->current_rois = NULL;
  ui_study->default_roi_grain =  GRAINS_1;
  ui_study->threshold = NULL;
  ui_study->series = NULL;
  ui_study->study_dialog = NULL;
  ui_study->study_selected = FALSE;
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

/* function to update the coordinate frame associated with one of our canvases.
   this is a coord_frame encompasing all the slices associated with one of the orthogonal views */
void ui_study_update_coords_current_view(ui_study_t * ui_study, view_t view) {

  realspace_t * canvas_coord_frame;
  realpoint_t * far_corner;

  volume_list_t * current_slices;
  realpoint_t temp_corner[2];

  /* get the current slices for the given view */
  current_slices = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "slices");

  /* get the pointers to the view's coordinate frame and far corner structures */
  canvas_coord_frame = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "coord_frame");
  far_corner = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "far_corner");

  /* set the origin of the canvas_coord_frame to the current view locations */
  *canvas_coord_frame = study_coord_frame(ui_study->study);
  *canvas_coord_frame = realspace_get_orthogonal_coord_frame(*canvas_coord_frame, view);
  
  /* figure out the corners */
  volumes_get_view_corners(current_slices, *canvas_coord_frame, temp_corner);

  /* allright, reset the offset of the view frame to the lower left front corner */
  (*canvas_coord_frame).offset = temp_corner[0];

  /* and set the upper right back corner */
  *far_corner = realspace_base_coord_to_alt(temp_corner[1],*canvas_coord_frame);

  return;
}

/* This function updates the little info box which tells us what the different 
   mouse buttons will do */
void ui_study_update_help_info(ui_study_t * ui_study, ui_study_help_info_t which_info) {
  GnomeCanvasItem * button[NUM_HELP_INFO_LINES];
  ui_study_help_info_line_t i_line;

  button[HELP_INFO_LINE_1] = gtk_object_get_data(GTK_OBJECT(ui_study->help_info), "button_1_info");
  button[HELP_INFO_LINE_1_SHIFT] = gtk_object_get_data(GTK_OBJECT(ui_study->help_info), "button_1s_info");
  button[HELP_INFO_LINE_2] = gtk_object_get_data(GTK_OBJECT(ui_study->help_info), "button_2_info");
  button[HELP_INFO_LINE_2_SHIFT] = gtk_object_get_data(GTK_OBJECT(ui_study->help_info), "button_2s_info");
  button[HELP_INFO_LINE_3] = gtk_object_get_data(GTK_OBJECT(ui_study->help_info), "button_3_info");
  button[HELP_INFO_LINE_3_SHIFT] = gtk_object_get_data(GTK_OBJECT(ui_study->help_info), "button_3s_info");

  for (i_line=0;i_line<NUM_HELP_INFO_LINES;i_line++) 
    if (button[i_line] == NULL) 
      button[i_line] = gnome_canvas_item_new(gnome_canvas_root(ui_study->help_info),
					     gnome_canvas_text_get_type(),
					     "justification", GTK_JUSTIFY_LEFT,
					     "anchor", GTK_ANCHOR_NORTH_WEST,
					     "text", ui_study_help_info_lines[which_info][i_line],
					     "x", (gdouble) UI_STUDY_HELP_INFO_LINE_X,
					     "y", (gdouble) (i_line*UI_STUDY_HELP_INFO_LINE_HEIGHT),
					     "font", UI_STUDY_HELP_FONT, NULL);
    else /* just need to change the text */
      gnome_canvas_item_set(button[i_line], "text", ui_study_help_info_lines[which_info][i_line], NULL);
  
  gtk_object_set_data(GTK_OBJECT(ui_study->help_info), "button_1_info", button[HELP_INFO_LINE_1]);
  gtk_object_set_data(GTK_OBJECT(ui_study->help_info), "button_1s_info", button[HELP_INFO_LINE_1_SHIFT]);
  gtk_object_set_data(GTK_OBJECT(ui_study->help_info), "button_2_info", button[HELP_INFO_LINE_2]);
  gtk_object_set_data(GTK_OBJECT(ui_study->help_info), "button_2s_info", button[HELP_INFO_LINE_2_SHIFT]);
  gtk_object_set_data(GTK_OBJECT(ui_study->help_info), "button_3_info", button[HELP_INFO_LINE_3]);
  gtk_object_set_data(GTK_OBJECT(ui_study->help_info), "button_3s_info", button[HELP_INFO_LINE_3_SHIFT]);

  return;
}

/* This function updates the little boxes which display the current location */
void ui_study_update_location_display(ui_study_t * ui_study, realpoint_t new_point) {

  gchar * temp_string;

  temp_string = g_strdup_printf("% 5.2f", new_point.x);
  gtk_entry_set_text(GTK_ENTRY(ui_study->location[XAXIS]), temp_string);
  g_free(temp_string);

  temp_string = g_strdup_printf("% 5.2f", new_point.y);
  gtk_entry_set_text(GTK_ENTRY(ui_study->location[YAXIS]), temp_string);
  g_free(temp_string);

  temp_string = g_strdup_printf("% 5.2f", new_point.z);
  gtk_entry_set_text(GTK_ENTRY(ui_study->location[ZAXIS]), temp_string);
  g_free(temp_string);

  return;
}

/* this function is used to draw/update/remove the target lines on the canvases */
void ui_study_update_targets(ui_study_t * ui_study, ui_study_target_action_t action, 
			     realpoint_t center, guint32 outline_color) {

  view_t i_view;
  guint i;
  GnomeCanvasPoints * target_line_points;
  GnomeCanvasItem ** target_lines;
  realpoint_t point0, point1, offset;
  realpoint_t start, end;
  realspace_t * canvas_coord_frame;
  realpoint_t * canvas_far_corner;
  GdkImlibImage * rgb_image;

  /* get rid of the old target lines if they still exist and we're not just moving the target */
  if ((action == TARGET_DELETE) || (action == TARGET_CREATE))
    for (i_view = 0 ; i_view < NUM_VIEWS ; i_view++) {
      target_lines = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[i_view]), "target_lines");
      gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "target_lines", NULL);
      if (target_lines != NULL) {
	for (i=0; i < 4 ; i++) 
	  gtk_object_destroy(GTK_OBJECT(target_lines[i]));
	g_free(target_lines);
      }
    }

  if (action == TARGET_DELETE)
    return;
  
  /* figure out some info */
  start.x = center.x-study_view_thickness(ui_study->study)/2.0;
  start.y = center.y-study_view_thickness(ui_study->study)/2.0;
  start.z = center.z-study_view_thickness(ui_study->study)/2.0;
  end.x = start.x+study_view_thickness(ui_study->study);
  end.y = start.y+study_view_thickness(ui_study->study);
  end.z = start.z+study_view_thickness(ui_study->study);
  
  for (i_view = 0 ; i_view < NUM_VIEWS ; i_view++) {

    /* get the coordinate frame and the far corner for this canvas,
       the canvas_corner is in the canvas_coord_frame.  Also need rgb_image
       for height and width of the canvas */
    canvas_coord_frame = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[i_view]), "coord_frame");
    canvas_far_corner = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[i_view]), "far_corner");
    rgb_image = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[i_view]), "rgb_image");

    /* if we're creating, allocate needed memory, otherwise just get a pointer to the object */
    if (action == TARGET_CREATE) {
      if ((target_lines = (GnomeCanvasItem **) g_malloc(sizeof(GnomeCanvasItem *)*4)) == NULL) {
	g_warning("%s: couldn't allocate space for target lines",PACKAGE);
	return;
      }
      gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "target_lines", target_lines);
    } else { /* TARGET_UPDATE */
      target_lines = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[i_view]), "target_lines");
      g_return_if_fail(target_lines != NULL);
    }
    
    offset = realspace_base_coord_to_alt((*canvas_coord_frame).offset,
					 study_coord_frame(ui_study->study));
    offset = realspace_coord_to_orthogonal_view(offset, i_view);
    
    
    point0 = realspace_coord_to_orthogonal_view(start, i_view);
    point0.x = (point0.x-offset.x)/(*canvas_far_corner).x
      * rgb_image->rgb_width  + UI_STUDY_TRIANGLE_HEIGHT;
    point0.y = (point0.y-offset.y)/(*canvas_far_corner).y 
      * rgb_image->rgb_height + UI_STUDY_TRIANGLE_HEIGHT;
    
    point1 = realspace_coord_to_orthogonal_view(end, i_view);
    point1.x = (point1.x-offset.x)/(*canvas_far_corner).x 
      * rgb_image->rgb_width + UI_STUDY_TRIANGLE_HEIGHT;
    point1.y = (point1.y-offset.y)/(*canvas_far_corner).y 
      * rgb_image->rgb_height + UI_STUDY_TRIANGLE_HEIGHT;
    
    /* line segment 0 */
    target_line_points = gnome_canvas_points_new(3);
    target_line_points->coords[0] = (double) UI_STUDY_TRIANGLE_HEIGHT;
    target_line_points->coords[1] = point0.y;
    target_line_points->coords[2] = point0.x;
    target_line_points->coords[3] = point0.y;
    target_line_points->coords[4] = point0.x;
    target_line_points->coords[5] = (double) UI_STUDY_TRIANGLE_HEIGHT;
    if (action == TARGET_CREATE) 
      target_lines[0] =
	gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[i_view]), gnome_canvas_line_get_type(),
			      "points", target_line_points, "fill_color_rgba", outline_color,
			      "width_units", 1.0, NULL);
    else /* action == TARGET_UPDATE */
      gnome_canvas_item_set(target_lines[0],"points",target_line_points, NULL);
    gnome_canvas_points_unref(target_line_points);
    
    /* line segment 1 */
    target_line_points = gnome_canvas_points_new(3);
    target_line_points->coords[0] = point1.x;
    target_line_points->coords[1] = (double) UI_STUDY_TRIANGLE_HEIGHT;
    target_line_points->coords[2] = point1.x;
    target_line_points->coords[3] = point0.y;
    target_line_points->coords[4] = (double) (rgb_image->rgb_width+UI_STUDY_TRIANGLE_HEIGHT);
    target_line_points->coords[5] = point0.y;
    if (action == TARGET_CREATE)
      target_lines[1] =
	gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[i_view]), gnome_canvas_line_get_type(),
			      "points", target_line_points, "fill_color_rgba", outline_color,
			      "width_units", 1.0, NULL);
    else /* action == TARGET_UPDATE */
      gnome_canvas_item_set(target_lines[1],"points",target_line_points, NULL);
    gnome_canvas_points_unref(target_line_points);
    

    /* line segment 2 */
    target_line_points = gnome_canvas_points_new(3);
    target_line_points->coords[0] = (double) UI_STUDY_TRIANGLE_HEIGHT;
    target_line_points->coords[1] = point1.y;
    target_line_points->coords[2] = point0.x;
    target_line_points->coords[3] = point1.y;
    target_line_points->coords[4] = point0.x;
    target_line_points->coords[5] = (double) (rgb_image->rgb_height+UI_STUDY_TRIANGLE_HEIGHT);
    if (action == TARGET_CREATE)
      target_lines[2] =
	gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[i_view]), gnome_canvas_line_get_type(),
			      "points", target_line_points, "fill_color_rgba", outline_color,
			      "width_units", 1.0, NULL);
    else /* action == TARGET_UPDATE */
      gnome_canvas_item_set(target_lines[2],"points",target_line_points, NULL);
    gnome_canvas_points_unref(target_line_points);


    /* line segment 3 */
    target_line_points = gnome_canvas_points_new(3);
    target_line_points->coords[0] = point1.x;
    target_line_points->coords[1] = (double) (rgb_image->rgb_height+UI_STUDY_TRIANGLE_HEIGHT);
    target_line_points->coords[2] = point1.x;
    target_line_points->coords[3] = point1.y;
    target_line_points->coords[4] = (double) (rgb_image->rgb_width+UI_STUDY_TRIANGLE_HEIGHT);
    target_line_points->coords[5] = point1.y;
    if (action == TARGET_CREATE) 
      target_lines[3] =
	gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[i_view]), gnome_canvas_line_get_type(),
			      "points", target_line_points, "fill_color_rgba", outline_color,
			      "width_units", 1.0, NULL);
    else /* action == TARGET_UPDATE */
      gnome_canvas_item_set(target_lines[3],"points",target_line_points, NULL);
    gnome_canvas_points_unref(target_line_points);


  }

  return;
}

/* function to update the adjustments for the plane scrollbar */
GtkAdjustment * ui_study_update_plane_adjustment(ui_study_t * ui_study, view_t view) {

  realspace_t view_coord_frame;
  realpoint_t view_corner[2];
  realpoint_t view_center;
  floatpoint_t upper, lower;
  floatpoint_t min_voxel_size;
  realpoint_t zp_start;
  GtkAdjustment * adjustment;

  /* which adjustment */
  adjustment = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "plane_adjustment");

  if (ui_study->current_volumes == NULL) {   /* junk values */
    min_voxel_size = 1.0;
    upper = lower = zp_start.z = 0.0;
  } else { /* calculate values */

    view_coord_frame = 
      realspace_get_orthogonal_coord_frame(study_coord_frame(ui_study->study), view);
    ui_volume_list_get_view_corners(ui_study->current_volumes, view_coord_frame, view_corner);
    min_voxel_size = ui_volume_list_max_min_voxel_size(ui_study->current_volumes);

    view_corner[1] = realspace_base_coord_to_alt(view_corner[1], view_coord_frame);
    view_corner[0] = realspace_base_coord_to_alt(view_corner[0], view_coord_frame);
    
    upper = view_corner[1].z;
    lower = view_corner[0].z;
    view_center = study_view_center(ui_study->study);

    /* translate the point so that the z coordinate corresponds to depth in this view */
    zp_start = realspace_coord_to_orthogonal_view(view_center, view);
    
    /* make sure our view center makes sense */
    if (zp_start.z < lower) {

      if (zp_start.z < lower-study_view_thickness(ui_study->study))
	zp_start.z = (upper-lower)/2.0+lower;
      else
	zp_start.z = lower;
      view_center = realspace_coord_from_orthogonal_view(zp_start, view);
      study_set_view_center(ui_study->study, view_center); /* save the updated view coords */

    } else if (zp_start.z > upper) {

      if (zp_start.z > lower+study_view_thickness(ui_study->study))
	zp_start.z = (upper-lower)/2.0+lower;
      else
	zp_start.z = upper;
      view_center = realspace_coord_from_orthogonal_view(zp_start, view);
      study_set_view_center(ui_study->study, view_center); /* save the updated view coords */

    }
  }
  

  /* if we haven't yet made the adjustment, make it */
  if (adjustment == NULL) {
    adjustment = 
      GTK_ADJUSTMENT(gtk_adjustment_new(zp_start.z, lower, upper,
					min_voxel_size,min_voxel_size,
					study_view_thickness(ui_study->study)));
    /*save this, so we can change it later*/
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[view]), "plane_adjustment", adjustment); 
  } else {
    adjustment->upper = upper;
    adjustment->lower = lower;
    adjustment->page_increment = min_voxel_size;
    adjustment->page_size = study_view_thickness(ui_study->study);
    adjustment->value = zp_start.z;

    /* allright, we need to update widgets connected to the adjustment without triggering our callback */
    gtk_signal_handler_block_by_func(GTK_OBJECT(adjustment),
				     GTK_SIGNAL_FUNC(ui_study_callbacks_plane_change),
				     ui_study);
    gtk_adjustment_changed(adjustment);  
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(adjustment), 
				       GTK_SIGNAL_FUNC(ui_study_callbacks_plane_change),
				       ui_study);

  }

  return adjustment;
}



/* updates the settings of the thickness_adjustment, will not change anything about the canvas */
void ui_study_update_thickness_adjustment(ui_study_t * ui_study) { 

  floatpoint_t min_voxel_size, max_size;

  if (study_volumes(ui_study->study) == NULL)
    return;

  /* block signals to the thickness adjustment, as we only want to
     change the value of the adjustment, it's up to the caller of this
     function to change anything on the actual canvases... we'll 
     unblock at the end of this function */
  gtk_signal_handler_block_by_func(GTK_OBJECT(ui_study->thickness_adjustment),
				   GTK_SIGNAL_FUNC(ui_study_callbacks_thickness), 
				     ui_study);

    
  min_voxel_size = volumes_min_voxel_size(study_volumes(ui_study->study));
  max_size = volumes_max_size(study_volumes(ui_study->study));

  /* set the current thickness if it hasn't already been set or if it's no longer valid*/
  if (study_view_thickness(ui_study->study) < min_voxel_size)
    study_view_thickness(ui_study->study) = min_voxel_size;

  ui_study->thickness_adjustment->upper = max_size;
  ui_study->thickness_adjustment->lower = min_voxel_size;
  ui_study->thickness_adjustment->page_size = min_voxel_size;
  ui_study->thickness_adjustment->step_increment = min_voxel_size;
  ui_study->thickness_adjustment->page_increment = min_voxel_size;
  ui_study->thickness_adjustment->value = study_view_thickness(ui_study->study);
  gtk_adjustment_changed(ui_study->thickness_adjustment);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_study->thickness_spin_button),
			    study_view_thickness(ui_study->study));   
  gtk_spin_button_configure(GTK_SPIN_BUTTON(ui_study->thickness_spin_button),
			    ui_study->thickness_adjustment,
			    study_view_thickness(ui_study->study),
			    2);
  /* and now, reconnect the signal */
  gtk_signal_handler_unblock_by_func(GTK_OBJECT(ui_study->thickness_adjustment), 
				     GTK_SIGNAL_FUNC(ui_study_callbacks_thickness), 
				     ui_study);
  return;
}




/* function to update the arrows on the canvas */
void ui_study_update_canvas_arrows(ui_study_t * ui_study, view_t view) {

  GnomeCanvasPoints * points[4];
  floatpoint_t x1,y1,x2,y2;
  floatpoint_t width=0.0, height=0.0;
  realspace_t * canvas_coord_frame;
  realpoint_t view_center, start, temp_p;
  realpoint_t offset;
  GdkImlibImage * rgb_image;
  GnomeCanvasItem * canvas_arrow[4];

  points[0] = gnome_canvas_points_new(4);
  points[1] = gnome_canvas_points_new(4);
  points[2] = gnome_canvas_points_new(4);
  points[3] = gnome_canvas_points_new(4);

  if ((study_volumes(ui_study->study) == NULL) || (ui_study->current_volumes == NULL)) {
    x1 = y1 = x2 = y2 = 0.5;
  } else {

    canvas_coord_frame = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "coord_frame");
    width = ui_volume_list_get_width(ui_study->current_volumes, *canvas_coord_frame);
    height = ui_volume_list_get_height(ui_study->current_volumes, *canvas_coord_frame);
    offset = realspace_base_coord_to_alt((*canvas_coord_frame).offset, study_coord_frame(ui_study->study));

    /* figure out the x and y coordiantes we're currently pointing to */
    view_center = study_view_center(ui_study->study);
    temp_p.x = temp_p.y = temp_p.z = 0.5*study_view_thickness(ui_study->study);
    start = rp_sub(view_center, temp_p);
    switch(view) {
    case TRANSVERSE:
      x1 = start.x-offset.x;
      y1 = start.y-offset.y;
      break;
    case CORONAL:
      x1 = start.x-offset.x;
      y1 = start.z-offset.z;
      break;
    case SAGITTAL:
    default:
      x1 = start.y-offset.y;
      y1 = start.z-offset.z;
      break;
    }
  }

  x2 = x1+study_view_thickness(ui_study->study);
  y2 = y1+study_view_thickness(ui_study->study);

  /* notes:
     1) even coords are the x coordinate, odd coords are the y
     2) drawing coordinate frame starts from the top left
  */

  /* get points to the previous arrows (if they exist */
  canvas_arrow[0] = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "canvas_arrow0");
  canvas_arrow[1] = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "canvas_arrow1");
  canvas_arrow[2] = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "canvas_arrow2");
  canvas_arrow[3] = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "canvas_arrow3");

  /* need to get rgb_image for the height and width, and canvas_arrows obviously */
  rgb_image = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "rgb_image");


  /* left arrow */
  points[0]->coords[0] = UI_STUDY_TRIANGLE_HEIGHT-1;
  points[0]->coords[1] = UI_STUDY_TRIANGLE_HEIGHT-1 + 
    rgb_image->rgb_height * (y1/height);
  points[0]->coords[2] = UI_STUDY_TRIANGLE_HEIGHT-1;
  points[0]->coords[3] = UI_STUDY_TRIANGLE_HEIGHT-1 + 
    rgb_image->rgb_height * (y2/height);
  points[0]->coords[4] = 0;
  points[0]->coords[5] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    UI_STUDY_TRIANGLE_WIDTH/2 +
    rgb_image->rgb_height * (y2/height);
  points[0]->coords[6] = 0;
  points[0]->coords[7] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    (-UI_STUDY_TRIANGLE_WIDTH/2) +
    rgb_image->rgb_height * (y1/height);

  /* top arrow */
  points[1]->coords[0] = UI_STUDY_TRIANGLE_HEIGHT-1 + 
    rgb_image->rgb_width * (x1/width);
  points[1]->coords[1] = UI_STUDY_TRIANGLE_HEIGHT-1;
  points[1]->coords[2] = UI_STUDY_TRIANGLE_HEIGHT-1 + 
    rgb_image->rgb_width * (x2/width);
  points[1]->coords[3] = UI_STUDY_TRIANGLE_HEIGHT-1;
  points[1]->coords[4] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    UI_STUDY_TRIANGLE_WIDTH/2 +
    rgb_image->rgb_width * (x2/width);
  points[1]->coords[5] = 0;
  points[1]->coords[6] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    (-UI_STUDY_TRIANGLE_WIDTH/2) +
    rgb_image->rgb_width * (x1/width);
  points[1]->coords[7] = 0;


  /* right arrow */
  points[2]->coords[0] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    rgb_image->rgb_width;
  points[2]->coords[1] = UI_STUDY_TRIANGLE_HEIGHT-1 + 
    rgb_image->rgb_height * (y1/height);
  points[2]->coords[2] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    rgb_image->rgb_width;
  points[2]->coords[3] = UI_STUDY_TRIANGLE_HEIGHT-1 + 
    rgb_image->rgb_height * (y2/height);
  points[2]->coords[4] = 2*UI_STUDY_TRIANGLE_HEIGHT-1 +
    rgb_image->rgb_width;
  points[2]->coords[5] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    UI_STUDY_TRIANGLE_WIDTH/2 +
    rgb_image->rgb_height * (y2/height);
  points[2]->coords[6] = 2*UI_STUDY_TRIANGLE_HEIGHT-1 +
    rgb_image->rgb_width;
  points[2]->coords[7] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    (-UI_STUDY_TRIANGLE_WIDTH/2) +
    rgb_image->rgb_height * (y1/height);


  /* bottom arrow */
  points[3]->coords[0] = UI_STUDY_TRIANGLE_HEIGHT-1 + 
    rgb_image->rgb_width * (x1/width);
  points[3]->coords[1] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    rgb_image->rgb_height;
  points[3]->coords[2] = UI_STUDY_TRIANGLE_HEIGHT-1 + 
    rgb_image->rgb_width * (x2/width);
  points[3]->coords[3] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    rgb_image->rgb_height;
  points[3]->coords[4] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    UI_STUDY_TRIANGLE_WIDTH/2 +
    rgb_image->rgb_width * (x2/width);
  points[3]->coords[5] = 2*UI_STUDY_TRIANGLE_HEIGHT-1 +
    rgb_image->rgb_height;
  points[3]->coords[6] = UI_STUDY_TRIANGLE_HEIGHT-1 +
    (-UI_STUDY_TRIANGLE_WIDTH/2) +
    rgb_image->rgb_width * (x1/width);
  points[3]->coords[7] =  2*UI_STUDY_TRIANGLE_HEIGHT-1 +
    rgb_image->rgb_height;

  if (canvas_arrow[0] != NULL ) {
    /* update the little arrow thingies */
    gnome_canvas_item_set(canvas_arrow[0],"points",points[0], NULL);
    gnome_canvas_item_set(canvas_arrow[1],"points",points[1], NULL);
    gnome_canvas_item_set(canvas_arrow[2],"points",points[2], NULL);
    gnome_canvas_item_set(canvas_arrow[3],"points",points[3], NULL);

  } else {
    /* create those little arrow things and save pointers to them*/
    canvas_arrow[0] = 
      gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[view]),
			    gnome_canvas_polygon_get_type(),
			    "points", points[0],"fill_color", "white",
			    "outline_color", "black", "width_pixels", 2,
			    NULL);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[view]), "canvas_arrow0", canvas_arrow[0]);
    canvas_arrow[1] = 
      gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[view]),
			    gnome_canvas_polygon_get_type(),
			    "points", points[1],"fill_color", "white",
			    "outline_color", "black", "width_pixels", 2,
			    NULL);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[view]), "canvas_arrow1", canvas_arrow[1]);
    canvas_arrow[2] = 
      gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[view]),
			    gnome_canvas_polygon_get_type(),
			    "points", points[2],"fill_color", "white",
			    "outline_color", "black", "width_pixels", 2,
			    NULL);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[view]), "canvas_arrow2", canvas_arrow[2]);
    canvas_arrow[3] = 
      gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[view]),
			    gnome_canvas_polygon_get_type(),
			    "points", points[3],"fill_color", "white",
			    "outline_color", "black", "width_pixels", 2,
			    NULL);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[view]), "canvas_arrow3", canvas_arrow[3]);
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
  volume_list_t * temp_volumes=NULL;
  volume_list_t * current_slices;
  realpoint_t temp_p;
  gint width, height;
  GnomeCanvasItem * canvas_image;
  GdkImlibImage * rgb_image;
  color_point_t blank_color;
  GtkStyle * widget_style;
  //GtkRequisition requisition;

  /* get points to the canvas image and rgb image associated with the current canvas */
  canvas_image = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "canvas_image");
  rgb_image = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "rgb_image");

  if (rgb_image != NULL) {
    width = rgb_image->rgb_width;
    height = rgb_image->rgb_height;
    gnome_canvas_destroy_image(rgb_image);
  } else {
    width = UI_STUDY_BLANK_WIDTH;
    height = UI_STUDY_BLANK_HEIGHT;
  }

  if (ui_study->current_volumes == NULL) {
    /* just use a blank image */

    /* figure out what color to use */
    widget_style = gtk_widget_get_style(GTK_WIDGET(ui_study->canvas[view]));
    if (widget_style == NULL) {
      g_warning("%s: Canvas has no style?\n",PACKAGE);
      widget_style = gtk_style_new();
    }

    blank_color.r = widget_style->bg[GTK_STATE_NORMAL].red >> 8;
    blank_color.g = widget_style->bg[GTK_STATE_NORMAL].green >> 8;
    blank_color.b = widget_style->bg[GTK_STATE_NORMAL].blue >> 8;

    rgb_image = image_blank(width,height, blank_color);

  } else {
    /* figure out our view coordinate frame */
    temp_p.x = temp_p.y = temp_p.z = 0.5*study_view_thickness(ui_study->study);
    temp_p = rp_sub(study_view_center(ui_study->study), temp_p);
    view_coord_frame = study_coord_frame(ui_study->study);
    view_coord_frame.offset = 
      realspace_alt_coord_to_base(temp_p,view_coord_frame);
    view_coord_frame = realspace_get_orthogonal_coord_frame(view_coord_frame, view);
    
    /* first, generate a volume_list we can pass to image_from_volumes */
    temp_volumes = ui_volume_list_return_volume_list(ui_study->current_volumes);
    
    /* get the pointer to the canvas's slices (if any) */
    current_slices = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "slices");

    rgb_image = image_from_volumes(&(current_slices),
				   temp_volumes,
				   study_view_time(ui_study->study),
				   study_view_duration(ui_study->study),
				   study_view_thickness(ui_study->study),
				   view_coord_frame,
				   study_scaling(ui_study->study),
				   study_zoom(ui_study->study),
				   study_interpolation(ui_study->study));

    /* save the pointer to the current slice list */
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[view]), "slices", current_slices);

    
    /* and delete the volume_list */
    temp_volumes = volume_list_free(temp_volumes);

    /* regenerate the canvas coordinate frames */
    ui_study_update_coords_current_view(ui_study, view);
  }

  /* reset the min size of the widget */
  if ((width != rgb_image->rgb_width) || (height != rgb_image->rgb_height) || (canvas_image == NULL)) {
    gtk_widget_set_usize(GTK_WIDGET(ui_study->canvas[view]), 
			 rgb_image->rgb_width + 2 * UI_STUDY_TRIANGLE_HEIGHT, 
			 rgb_image->rgb_height + 2 * UI_STUDY_TRIANGLE_HEIGHT);
    //requisition.width = rgb_image->rgb_width;
    //    requisition.height = rgb_image->rgb_height;
    //gtk_widget_size_request(GTK_WIDGET(ui_study->canvas[view]), &requisition);
    //gtk_widget_queue_resize(GTK_WIDGET(ui_study->app));
  }

  /* set the scroll region */
  gnome_canvas_set_scroll_region(ui_study->canvas[view], 0.0, 0.0, 
    				 rgb_image->rgb_width + 2 * UI_STUDY_TRIANGLE_HEIGHT,
  				 rgb_image->rgb_height + 2 * UI_STUDY_TRIANGLE_HEIGHT);
  
  /* put the rgb_image on the canvas_image */
  if (canvas_image != NULL) {
    gnome_canvas_item_set(canvas_image,
			  "image", rgb_image, 
			  "width", (double) rgb_image->rgb_width,
			  "height", (double) rgb_image->rgb_height,
			  NULL);
  } else {
    /* time to make a new image */
    canvas_image =
      gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[view]),
			    gnome_canvas_image_get_type(),
			    "image", rgb_image,
			    "x", (double) UI_STUDY_TRIANGLE_HEIGHT,
			    "y", (double) UI_STUDY_TRIANGLE_HEIGHT,
			    "anchor", GTK_ANCHOR_NORTH_WEST,
			    "width",(double) rgb_image->rgb_width,
			    "height",(double) rgb_image->rgb_height,
			    NULL);
  }

  /* save pointers to the canvas image and rgb image as needed */
  gtk_object_set_data(GTK_OBJECT(ui_study->canvas[view]), "canvas_image", canvas_image);
  gtk_object_set_data(GTK_OBJECT(ui_study->canvas[view]), "rgb_image", rgb_image);

  return;
}


/* replaces the current cursor with the specified cursor */
void ui_study_place_cursor(ui_study_t * ui_study, ui_study_cursor_t which_cursor,
			   GtkWidget * widget) {

  GdkCursor * cursor;

  /* push our desired cursor onto the cursor stack */
  cursor = ui_study->cursor[which_cursor];
  ui_study->cursor_stack = g_slist_prepend(ui_study->cursor_stack,cursor);
  gdk_window_set_cursor(gtk_widget_get_parent_window(widget), cursor);

  /* do any events pending, this allows the cursor to get displayed */
  while (gtk_events_pending()) 
    gtk_main_iteration();

  return;
}

/* removes the currsor cursor, goign back to the previous cursor (or default cursor if no previous */
void ui_study_remove_cursor(ui_study_t * ui_study, GtkWidget * widget) {

  GdkCursor * cursor;

  /* pop the previous cursor off the stack */
  cursor = g_slist_nth_data(ui_study->cursor_stack, 0);
  ui_study->cursor_stack = g_slist_remove(ui_study->cursor_stack, cursor);
  cursor = g_slist_nth_data(ui_study->cursor_stack, 0);
  gdk_window_set_cursor(gtk_widget_get_parent_window(widget), cursor);

  /* do any events pending, this allows the cursor to get displayed */
  while (gtk_events_pending()) 
    gtk_main_iteration();

  return;
}




/* function to draw an roi for a canvas */
GnomeCanvasItem *  ui_study_update_canvas_roi(ui_study_t * ui_study, 
					      view_t view, 
					      GnomeCanvasItem * roi_item,
					      roi_t * roi) {
  
  realpoint_t offset;
  GnomeCanvasPoints * item_points;
  GnomeCanvasItem * item;
  GSList * roi_points, * temp;
  axis_t j;
  guint32 outline_color;
  floatpoint_t width,height;
  volume_t * volume;
  volume_list_t * current_slices;
  GdkImlibImage * rgb_image;
  double affine[6];

  /* get a pointer to the canvas's slices and the rgb_image */
  current_slices = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "slices");
  rgb_image = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "rgb_image");

  /* sanity check */
  if (current_slices == NULL) {
    if (roi_item != NULL) /* if no slices, destroy the old roi_item */
      gtk_object_destroy(GTK_OBJECT(roi_item));
    return NULL;
  }

  /* figure out which volume we're dealing with */
  if (ui_study->current_volume == NULL)
    return NULL;
  else
    volume = ui_study->current_volume;
  /* and figure out the outline color from that*/
  outline_color = 
    color_table_outline_color(volume->color_table,
			      ui_study->current_roi == roi);

  /* get the points */
  roi_points =  roi_get_volume_intersection_points(current_slices->volume, roi);

  /* count the points */
  j=0;
  temp=roi_points;
  while(temp!=NULL) {
    temp=temp->next;
    j++;
  }

  if (j<=1) {
    if (roi_item != NULL) /* if no points, destroy the old roi_item */
      gtk_object_destroy(GTK_OBJECT(roi_item));
    return NULL;
  }


  /* get some needed information */
  width = current_slices->volume->dim.x* current_slices->volume->voxel_size.x;
  height = current_slices->volume->dim.y* current_slices->volume->voxel_size.y;
  offset =  realspace_base_coord_to_alt(current_slices->volume->coord_frame.offset,
					current_slices->volume->coord_frame);
  /* transfer the points list to what we'll be using to construction the figure */
  item_points = gnome_canvas_points_new(j);
  temp=roi_points;
  j=0;
  while(temp!=NULL) {
    item_points->coords[j] = 
      ((((realpoint_t * ) temp->data)->x-offset.x)/width) 
      *rgb_image->rgb_width + UI_STUDY_TRIANGLE_HEIGHT;
    item_points->coords[j+1] = 
      ((((realpoint_t * ) temp->data)->y-offset.y)/height)
      *rgb_image->rgb_height + UI_STUDY_TRIANGLE_HEIGHT;
    temp=temp->next;
    j += 2;
  }

  roi_free_points_list(&roi_points);

  /* create the item */ 
  if (roi_item != NULL) {
    /* make sure to reset any affine translations we've done */
    gnome_canvas_item_i2w_affine(GNOME_CANVAS_ITEM(roi_item),affine);
    affine[0] = affine[3] = 1.0;
    affine[1] = affine[2] = affine[4] = affine[5] = affine[6] = 0.0;
    gnome_canvas_item_affine_absolute(GNOME_CANVAS_ITEM(roi_item),affine);

    /* and reset the line points */
    gnome_canvas_item_set(roi_item, "points", item_points, NULL);
    return roi_item;
  } else {
    item =  gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[view]),
				  gnome_canvas_line_get_type(),
				  "points", item_points,
				  "fill_color_rgba", outline_color,
				  "width_units", 1.0,
				  NULL);
    
    /* attach it's callback */
    gtk_signal_connect(GTK_OBJECT(item), "event",
		       GTK_SIGNAL_FUNC(ui_study_rois_callbacks_roi_event),
		       ui_study);
    gtk_object_set_data(GTK_OBJECT(item), "view", GINT_TO_POINTER(view));
    gtk_object_set_data(GTK_OBJECT(item), "roi", roi);
    
    /* free up the space used for the item's points */
    gnome_canvas_points_unref(item_points);
    
    return item;
  }
}



/* function to update all the currently selected rois */
void ui_study_update_canvas_rois(ui_study_t * ui_study, view_t view) {
  
  ui_roi_list_t * temp_roi_list=ui_study->current_rois;

  while (temp_roi_list != NULL) {
    temp_roi_list->canvas_roi[view] = 
      ui_study_update_canvas_roi(ui_study,view,
				 temp_roi_list->canvas_roi[view],
				 temp_roi_list->roi);
    temp_roi_list = temp_roi_list->next;
  }
  
  return;
}

/* function to update a canvas, if view_it is  NUM_VIEWS, update
   all canvases */
void ui_study_update_canvas(ui_study_t * ui_study, view_t i_view, 
			    ui_study_update_t update) {

  view_t j_view,k_view;
  realpoint_t temp_center;
  realpoint_t view_corner[2];
  volume_list_t * current_slices;

  if (i_view==NUM_VIEWS) {
    i_view=0;
    j_view=NUM_VIEWS;
  } else
    j_view=i_view+1;

  ui_study_place_cursor(ui_study, UI_STUDY_WAIT, GTK_WIDGET(ui_study->canvas[i_view]));

  /* make sure the study coord_frame offset is set correctly, 
     adjust current_view_center if necessary */
  temp_center = realspace_alt_coord_to_base(study_view_center(ui_study->study),
    					    study_coord_frame(ui_study->study));
  volumes_get_view_corners(study_volumes(ui_study->study),
			   study_coord_frame(ui_study->study), view_corner);
  study_set_coord_frame_offset(ui_study->study, view_corner[0]);
  study_set_view_center(ui_study->study, 
			realspace_base_coord_to_alt(temp_center, study_coord_frame(ui_study->study)));
  ui_study_update_location_display(ui_study, temp_center);

  for (k_view=i_view;k_view<j_view;k_view++) {
    switch (update) {
    case REFRESH_IMAGE:
      /* refresh the image, but use the same slice as before */
      ui_study_update_canvas_image(ui_study, k_view);
      break;
    case UPDATE_IMAGE:
      /* freeing the slices indicates to regenerate them */
      current_slices = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[k_view]), "slices");
      current_slices=volume_list_free(current_slices); 
      gtk_object_set_data(GTK_OBJECT(ui_study->canvas[k_view]), "slices", current_slices);
      ui_study_update_canvas_image(ui_study, k_view);       /* refresh the image */
      break;
    case UPDATE_ROIS:
      ui_study_update_canvas_rois(ui_study, k_view);
      break;
    case UPDATE_ARROWS:
      ui_study_update_canvas_arrows(ui_study,k_view);
      break;
    case UPDATE_PLANE_ADJUSTMENT:
      ui_study_update_plane_adjustment(ui_study, k_view);
      break;
    case UPDATE_ALL:
    default:
      /* freeing the slices indicates to regenerate them */
      current_slices = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[k_view]), "slices");
      current_slices=volume_list_free(current_slices); 
      gtk_object_set_data(GTK_OBJECT(ui_study->canvas[k_view]), "slices", current_slices);
      ui_study_update_canvas_image(ui_study, k_view);
      ui_study_update_canvas_rois(ui_study, k_view);
      ui_study_update_canvas_arrows(ui_study,k_view);
      ui_study_update_plane_adjustment(ui_study, k_view);
      break;
    }

  }

  ui_study_remove_cursor(ui_study, GTK_WIDGET(ui_study->canvas[i_view]));

  return;
}


/* function to update which row has the active symbol */
void ui_study_tree_update_active_row(ui_study_t * ui_study, gint row) {

  GtkCTree * ctree = GTK_CTREE(ui_study->tree);
  GtkCTreeNode * active_node = NULL;
  GtkCTreeNode * prev_active_node;
  

  /* remove the previous active symbol */
  prev_active_node = gtk_object_get_data(GTK_OBJECT(ctree), "active_row");
  if (prev_active_node != NULL)
    gtk_ctree_node_set_text(ctree, prev_active_node, UI_STUDY_TREE_ACTIVE_COLUMN, NULL);

  /* set the current active symbol */
  if (row != -1) {
    active_node = gtk_ctree_node_nth(ctree, row);
    gtk_ctree_node_set_text(ctree, active_node, UI_STUDY_TREE_ACTIVE_COLUMN, ui_study_tree_active_mark); 
  } else if (ui_study->current_volume != NULL) {
    /* gotta figure out which one to mark */
    active_node = gtk_ctree_find_by_row_data(ctree, NULL, ui_study->current_volume);
    gtk_ctree_node_set_text(ctree, active_node, UI_STUDY_TREE_ACTIVE_COLUMN, ui_study_tree_active_mark); 
  }

  /* save info as to which row has the active symbol */
  gtk_object_set_data(GTK_OBJECT(ctree), "active_row", active_node);
  
  return;
}

/* function adds a roi item to the tree */
void ui_study_tree_add_roi(ui_study_t * ui_study, roi_t * roi) {

  GdkPixmap * pixmap;
  GdkWindow * parent;
  gchar * tree_buf[UI_STUDY_TREE_NUM_COLUMNS];

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
  default:
    pixmap = gdk_pixmap_create_from_xpm_d(parent,NULL,NULL,ROI_xpm);
    break;
  }

  tree_buf[UI_STUDY_TREE_ACTIVE_COLUMN] = NULL;
  tree_buf[UI_STUDY_TREE_TEXT_COLUMN] = roi->name;

  ui_study->tree_rois = /* now points to current node */
    gtk_ctree_insert_node(GTK_CTREE(ui_study->tree),
			  ui_study->tree_study, /* parent */
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
void ui_study_tree_add_volume(ui_study_t * ui_study, volume_t * volume) {

  GdkPixmap * pixmap;
  GdkWindow * parent;
  gchar * tree_buf[UI_STUDY_TREE_NUM_COLUMNS];

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

  
  tree_buf[UI_STUDY_TREE_ACTIVE_COLUMN] = NULL;
  tree_buf[UI_STUDY_TREE_TEXT_COLUMN] = volume->name;
  ui_study->tree_volumes =
    gtk_ctree_insert_node(GTK_CTREE(ui_study->tree),
			  ui_study->tree_study, /* parent */
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
  gchar * tree_buf[UI_STUDY_TREE_NUM_COLUMNS];
  volume_list_t * volume_list;
  roi_list_t * roi_list;
  
  /* make the primary nodes if needed*/
  if (ui_study->tree_study == NULL) {
    pixmap = gdk_pixmap_create_from_xpm_d(gtk_widget_get_parent_window(ui_study->tree),
					  NULL,NULL,study_xpm);
    
    /* put the current study into the tree */
    tree_buf[UI_STUDY_TREE_ACTIVE_COLUMN] = NULL;
    tree_buf[UI_STUDY_TREE_TEXT_COLUMN] = study_name(ui_study->study);

    ui_study->tree_study =
      gtk_ctree_insert_node(GTK_CTREE(ui_study->tree),
			    NULL, /* parent */
			    ui_study->tree_study, /* siblings */
			    tree_buf,
			    5, /* spacing */
			    pixmap, NULL, pixmap, NULL, FALSE, TRUE);
    gtk_ctree_node_set_row_data(GTK_CTREE(ui_study->tree), ui_study->tree_study, ui_study->study);

    /* if there are any volumes, place them on in */
    if (study_volumes(ui_study->study) != NULL) {
      volume_list = study_volumes(ui_study->study);
      while (volume_list != NULL) {
	ui_study_tree_add_volume(ui_study, volume_list->volume);
	volume_list = volume_list->next;
      }
    }

    /* if there are any rois, place them on in */
    if (study_rois(ui_study->study)!= NULL) {
      roi_list = study_rois(ui_study->study);
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

  GtkWidget * main_table;
  GtkWidget * right_table;
  GtkWidget * left_table;
  GtkWidget * middle_table;
  GtkWidget * label;
  GtkWidget * scrollbar;
  GtkAdjustment * adjustment;
  GtkWidget * option_menu;
  GtkWidget * menu;
  GtkWidget * menuitem;
  GtkWidget * button;
  GtkWidget * spin_button;
  GtkWidget * tree;
  GtkWidget * scrolled;
  GtkWidget * entry;
  GtkWidget * event_box;
  view_t i_view;
  axis_t i_axis;
  scaling_t i_scaling;
  color_table_t i_color_table;
  interpolation_t i_interpolation;
  roi_type_t i_roi_type;
  realpoint_t * far_corner;
  realspace_t * canvas_coord_frame;
  guint main_table_row, main_table_column;
  guint left_table_row, right_table_row;
  gint middle_table_row, middle_table_column;
  gchar * temp_string;


  /* make and add the main packing table */
  main_table = gtk_table_new(UI_STUDY_MAIN_TABLE_WIDTH,
			     UI_STUDY_MAIN_TABLE_HEIGHT,FALSE);
  main_table_row = main_table_column=0;
  gnome_app_set_contents(ui_study->app, GTK_WIDGET(main_table));

  /* connect the blank help signal */
  gtk_object_set_data(GTK_OBJECT(ui_study->app), "which_help", GINT_TO_POINTER(HELP_INFO_BLANK));
  gtk_signal_connect(GTK_OBJECT(ui_study->app), "enter_notify_event",
  		     GTK_SIGNAL_FUNC(ui_study_callbacks_update_help_info), ui_study);

  /* setup the left tree widget */
  left_table = gtk_table_new(2,5, FALSE);
  left_table_row=0;
  gtk_table_attach(GTK_TABLE(main_table), 
		   left_table, 
		   main_table_column, main_table_column+1, 
		   main_table_row, UI_STUDY_MAIN_TABLE_HEIGHT,
		   X_PACKING_OPTIONS | GTK_FILL, 
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);
  main_table_column++;
  main_table_row = 0;

  /* make an event box, this allows us to catch events for the tree */
  event_box = gtk_event_box_new();
  gtk_table_attach(GTK_TABLE(left_table), event_box,
		   0,2, left_table_row, left_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 
		   Y_PACKING_OPTIONS | GTK_FILL, 
		   X_PADDING, Y_PADDING);
  left_table_row++;
  gtk_object_set_data(GTK_OBJECT(event_box), "which_help", GINT_TO_POINTER(HELP_INFO_TREE_NONE));
  gtk_signal_connect(GTK_OBJECT(event_box), "enter_notify_event",
  		     GTK_SIGNAL_FUNC(ui_study_callbacks_update_help_info), ui_study);


  /* make a scrolled area for the tree */
  scrolled = gtk_scrolled_window_new(NULL,NULL);
  gtk_widget_set_usize(scrolled,200,200);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(event_box),scrolled);
  //  gtk_table_attach(GTK_TABLE(left_table), scrolled,
  //		   0,2, left_table_row, left_table_row+1,
  //		   X_PACKING_OPTIONS | GTK_FILL, 
  //		   Y_PACKING_OPTIONS | GTK_FILL, 
  //		   X_PADDING, Y_PADDING);


  /* make the tree */
  tree = gtk_ctree_new_with_titles(UI_STUDY_TREE_NUM_COLUMNS,
				   UI_STUDY_TREE_TREE_COLUMN,
				   ui_study_tree_column_titles);
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
  gtk_object_set_data(GTK_OBJECT(tree), "active_row", NULL);


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
    gtk_object_set_data(GTK_OBJECT(menuitem), "roi_type", GINT_TO_POINTER(i_roi_type)); 
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

  /* the help information canvas */
  ui_study->help_info = GNOME_CANVAS(gnome_canvas_new());
  gtk_table_attach(GTK_TABLE(left_table), GTK_WIDGET(ui_study->help_info), 
		   0,2, left_table_row, left_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_object_set_data(GTK_OBJECT(ui_study->help_info), "button_1_info", NULL);
  gtk_object_set_data(GTK_OBJECT(ui_study->help_info), "button_1s_info", NULL);
  gtk_object_set_data(GTK_OBJECT(ui_study->help_info), "button_2_info", NULL);
  gtk_object_set_data(GTK_OBJECT(ui_study->help_info), "button_2s_info", NULL);
  gtk_object_set_data(GTK_OBJECT(ui_study->help_info), "button_3_info", NULL);
  gtk_object_set_data(GTK_OBJECT(ui_study->help_info), "button_3s_info", NULL);
  gtk_widget_set_usize(GTK_WIDGET(ui_study->help_info), 150,
		       UI_STUDY_HELP_INFO_LINE_HEIGHT*NUM_HELP_INFO_LINES);
  gnome_canvas_set_scroll_region(ui_study->help_info, 0.0, 0.0, 150.0, 100.0);
  left_table_row++;

  /* make the stuff in the middle, we'll have the entire middle table in a scrolled window  */
  //  scrolled = gtk_scrolled_window_new(NULL,NULL);
  //  gtk_widget_set_usize(scrolled,-1,-1);
  //  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
  //				 GTK_POLICY_AUTOMATIC,
  //				 GTK_POLICY_AUTOMATIC);
  //  gtk_table_attach(GTK_TABLE(main_table), scrolled,
  //		   main_table_column, main_table_column+1, 
  //		   main_table_row, UI_STUDY_MAIN_TABLE_HEIGHT,
  //		   X_PACKING_OPTIONS | GTK_FILL, 
  //		   Y_PACKING_OPTIONS | GTK_FILL, 
  //		   X_PADDING, Y_PADDING);
  //  main_table_column++;
  //  main_table_row = 0;

  middle_table = gtk_table_new(UI_STUDY_MIDDLE_TABLE_WIDTH, UI_STUDY_MIDDLE_TABLE_HEIGHT,FALSE);
  middle_table_row=middle_table_column = 0;
  //  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled),middle_table);
  gtk_table_attach(GTK_TABLE(main_table), middle_table,
		   main_table_column, main_table_column+1, 
		   main_table_row, UI_STUDY_MAIN_TABLE_HEIGHT,
		   X_PACKING_OPTIONS | GTK_FILL, 
		   Y_PACKING_OPTIONS | GTK_FILL, 
		   X_PADDING, Y_PADDING);
  main_table_column++;
  main_table_row = 0;


  /* make the three canvases, scrollbars, dials, etc. */
  for (i_view=0;i_view<NUM_VIEWS;i_view++) {
    middle_table_row=0;

    /* make the label for this column */
    label = gtk_label_new(view_names[i_view]);
    gtk_table_attach(GTK_TABLE(middle_table), 
		     label, 
		     middle_table_column, middle_table_column+1, 
		     middle_table_row, middle_table_row+1,
		     GTK_FILL, FALSE, X_PADDING, Y_PADDING);
    middle_table_row++;

    /* canvas section */
    //    ui_study->canvas[i] = GNOME_CANVAS(gnome_canvas_new_aa());
    ui_study->canvas[i_view] = GNOME_CANVAS(gnome_canvas_new());
    gtk_table_attach(GTK_TABLE(middle_table), 
		     GTK_WIDGET(ui_study->canvas[i_view]), 
		     middle_table_column, middle_table_column+1,
		     middle_table_row, middle_table_row+1,
		     FALSE,FALSE, X_PADDING, Y_PADDING);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "target_lines", NULL);
    g_return_if_fail((far_corner = (realpoint_t *) g_malloc(sizeof(realpoint_t))) != NULL);
    *far_corner = realpoint_init;
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "far_corner", far_corner);
    g_return_if_fail((canvas_coord_frame = (realspace_t *) g_malloc(sizeof(realspace_t))) != NULL);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "coord_frame", canvas_coord_frame);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "slices", NULL);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "canvas_image", NULL);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "rgb_image", NULL);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "canvas_arrow0", NULL);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "canvas_arrow1", NULL);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "canvas_arrow2", NULL);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "canvas_arrow3", NULL);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "plane_adjustment", NULL);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "view", GINT_TO_POINTER(i_view));

    /* and put something into the canvas */
    ui_study_update_canvas_image(ui_study, i_view); 

    /* hook up the callbacks */
    gtk_signal_connect(GTK_OBJECT(ui_study->canvas[i_view]), "event",
		       GTK_SIGNAL_FUNC(ui_study_callbacks_canvas_event),
		       ui_study);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "which_help", 
			GINT_TO_POINTER(HELP_INFO_CANVAS_VOLUME));
    gtk_signal_connect(GTK_OBJECT(ui_study->canvas[i_view]), "enter_notify_event",
		       GTK_SIGNAL_FUNC(ui_study_callbacks_update_help_info), ui_study);

    middle_table_row++;


    /* scrollbar section */
    adjustment = ui_study_update_plane_adjustment(ui_study, i_view);
    /*so we can figure out which adjustment this is in callbacks */
    gtk_object_set_data(GTK_OBJECT(adjustment), "view", GINT_TO_POINTER(i_view));
    scrollbar = gtk_hscrollbar_new(adjustment);
    gtk_range_set_update_policy(GTK_RANGE(scrollbar), GTK_UPDATE_DISCONTINUOUS);
    gtk_table_attach(GTK_TABLE(middle_table), 
		     GTK_WIDGET(scrollbar), 
		     middle_table_column, middle_table_column+1,
		     middle_table_row, middle_table_row+1,
		     GTK_FILL,FALSE, X_PADDING, Y_PADDING);
    gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed", 
		       GTK_SIGNAL_FUNC(ui_study_callbacks_plane_change), 
		       ui_study);
    middle_table_row++;

    /* I should hook up a entry widget to this to allow more fine settings */

    middle_table_column++;
  }
  main_table_row=0;

  /* things to put in the right most column */
  right_table = gtk_table_new(UI_STUDY_RIGHT_TABLE_WIDTH,
			      UI_STUDY_RIGHT_TABLE_HEIGHT,FALSE);
  right_table_row=0;

  /* displaying the current location */
  label = gtk_label_new("location:");
  gtk_table_attach(GTK_TABLE(right_table), GTK_WIDGET(label), 0,1, 
		   right_table_row,right_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  right_table_row++;
		   
  for (i_axis = 0; i_axis < NUM_AXIS; i_axis++) {
    label = gtk_label_new(axis_names[i_axis]);
    gtk_table_attach(GTK_TABLE(right_table), GTK_WIDGET(label), 0,1, 
		     right_table_row,right_table_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    entry = gtk_entry_new();
    gtk_widget_set_usize(GTK_WIDGET(entry), UI_STUDY_DEFAULT_ENTRY_WIDTH,0);
    gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
    gtk_table_attach(GTK_TABLE(right_table), GTK_WIDGET(entry), 1, 2,
		     right_table_row, right_table_row+1, 0, 0, X_PADDING, Y_PADDING);
    ui_study->location[i_axis] = entry;
    right_table_row++;
  }
  ui_study_update_location_display(ui_study, 
				   realspace_alt_coord_to_base(study_view_center(ui_study->study),
							       study_coord_frame(ui_study->study)));

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
    gtk_object_set_data(GTK_OBJECT(menuitem), "scaling", GINT_TO_POINTER(i_scaling));
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
		       GTK_SIGNAL_FUNC(ui_study_callbacks_scaling), ui_study);
  }

  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), study_scaling(ui_study->study));

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
    gtk_object_set_data(GTK_OBJECT(menuitem), "color_table", GINT_TO_POINTER(i_color_table));
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
    gtk_object_set_data(GTK_OBJECT(menuitem), "interpolation", GINT_TO_POINTER(i_interpolation));
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
    		       GTK_SIGNAL_FUNC(ui_study_callbacks_interpolation), 
		       ui_study);
  }

  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), 
			      study_interpolation(ui_study->study));
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

  adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(study_zoom(ui_study->study),
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

  temp_string = g_strdup_printf("%5.1f-%5.1fs",
				study_view_time(ui_study->study),
				study_view_duration(ui_study->study));
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


  /* and add the right column to the main table */
  gtk_table_attach(GTK_TABLE(main_table), 
		   GTK_WIDGET(right_table), 
		   main_table_column,main_table_column+1,
		   main_table_row, UI_STUDY_MAIN_TABLE_HEIGHT,
		   //		   X_PACKING_OPTIONS | GTK_FILL, 
		   0, 
		   0, 
		   X_PADDING, Y_PADDING);

  return;
}



/* procedure to set up the study window */
GnomeApp * ui_study_create(study_t * study) {

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
    title = g_strdup_printf("Study: %s",study_name(ui_study->study));
    g_free(temp_string);
  } else {
    ui_study->study = study;
    title = g_strdup_printf("Study: %s",study_name(ui_study->study));
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

  /* get the study window running */
  gtk_widget_show_all(GTK_WIDGET(app));

  /* and set any settings we can */
  ui_study_update_thickness_adjustment(ui_study);

  return app;
}
