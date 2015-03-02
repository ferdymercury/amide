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
#include <gdk-pixbuf/gnome-canvas-pixbuf.h>
#include "study.h"
#include "image.h"
#include "ui_common.h"
#include "ui_study.h"
#include "ui_study_cb.h"
#include "ui_study_menus.h"
#include "ui_study_toolbar.h"
#include "ui_time_dialog.h"

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
  {"1 move view", "shift-1 horizontal align", "2 move view, min. depth", "shift-2 vertical align", "3 change depth",""},
  {"1 shift", "", "2 rotate", "", "3 scale", ""}, /*CANVAS_ROI */
  {"1 shift", "", "", "", "3 start isocontour change", ""}, /*CANVAS_ISOCONTOUR_ROI */
  {"1 draw", "", "", "", "", ""}, /* CANVAS_NEW_ROI */
  {"1 pick isocontour value", "", "", "", "", ""}, /* CANVAS_NEW_ISOCONTOUR_ROI */
  {"1 cancel", "", "2 cancel", "", "3 align", ""}, /*CANVAS ALIGN */
  {"1 select data set", "", "2 make active", "", "3 pop up data set dialog", ""}, /* TREE_VOLUME */
  {"1 select roi", "", "2 make active", "", "3 pop up roi dialog", ""}, /* TREE_ROI */
  {"", "", "", "", "3 pop up study dialog",""}, /* TREE_STUDY */
  {"", "", "", "", "3 add roi",""} /* TREE_NONE */
};

static gchar * ui_study_tree_active_mark = "X";
static gchar * ui_study_tree_non_active_mark = " ";
static gint next_study_num=1;

/* destroy a ui_study data structure */
ui_study_t * ui_study_free(ui_study_t * ui_study) {

  view_t i_view;
  
  if (ui_study == NULL)
    return ui_study;

  /* sanity checks */
  g_return_val_if_fail(ui_study->reference_count > 0, NULL);

  /* remove a reference count */
  ui_study->reference_count--;

  /* if we've removed all reference's, free the structure */
  if (ui_study->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing ui_study\n");
#endif
    ui_study->current_rois = ui_roi_list_free(ui_study->current_rois);
    ui_study->current_volumes = ui_volume_list_free(ui_study->current_volumes);
    ui_study->study = study_free(ui_study->study);
    for (i_view=0;i_view<NUM_VIEWS;i_view++) {
      ui_study->current_slices[i_view] = volume_list_free(ui_study->current_slices[i_view]);
    }
    g_free(ui_study);
    ui_study = NULL;
  }
    
  return ui_study;
}

/* malloc and initialize a ui_study data structure */
ui_study_t * ui_study_init(void) {

  ui_study_t * ui_study;
  view_t i_view;

  /* alloc space for the data structure for passing ui info */
  if ((ui_study = (ui_study_t *) g_malloc(sizeof(ui_study_t))) == NULL) {
    g_warning("%s: couldn't allocate space for ui_study_t",PACKAGE);
    return NULL;
  }
  ui_study->reference_count = 1;

  ui_study->study = NULL;
  ui_study->current_volume = NULL;
  ui_study->current_volumes = NULL;
  ui_study->current_rois = NULL;
  ui_study->threshold_dialog = NULL;
  ui_study->preferences_dialog = NULL;
  ui_study->study_dialog = NULL;
  ui_study->study_selected = FALSE;
  ui_study->undrawn_roi = NULL;
  ui_study->time_dialog = NULL;
  ui_study->thickness_adjustment = NULL;
  ui_study->thickness_spin_button = NULL;

  for (i_view = 0;i_view<NUM_VIEWS;i_view++) {
    ui_study->canvas_rgb[i_view]=NULL;
    ui_study->current_slices[i_view]=NULL;
    ui_study->canvas_image[i_view]=NULL;
  }

  /* load in saved preferences */
  gnome_config_push_prefix("/"PACKAGE"/");
  ui_study->roi_width = gnome_config_get_int("ROI/Width");
  if (ui_study->roi_width == 0) ui_study->roi_width = 2; /* if no config file, put in sane value */

  ui_study->line_style = gnome_config_get_int("ROI/LineStyle"); /* 0 is solid */
  gnome_config_pop_prefix();

 return ui_study;
}


/* function to add a new volume into an amide study */
void ui_study_add_volume(ui_study_t * ui_study, volume_t * new_volume) {

  /* add the volume to the study structure */
  study_add_volume(ui_study->study, new_volume);
  
  /* and add the volume to the ui_study tree structure */
  ui_study_tree_add_volume(ui_study, new_volume);
  
  /* update the thickness widget */
  ui_study_update_thickness_adjustment(ui_study);
  
  /* make sure we're pointing to something sensible */
  if (study_view_center(ui_study->study).x < 0.0) {
    study_set_view_center(ui_study->study, volume_calculate_center(new_volume));
  }
  
  /* adjust the time dialog if necessary */
  ui_time_dialog_set_times(ui_study);

  return;
}


/* function to load in a non-xif file into the study and update the widgets
   note: model_name is a secondary file needed by PEM, it's usually
   going to be set to NULL */
void ui_study_import_file(ui_study_t * ui_study, import_method_t import_method,
			  int submethod, gchar * import_filename, gchar * model_filename) {

  volume_t * import_volume;

#ifdef AMIDE_DEBUG
  g_print("file to import: %s\n",import_filename);
#endif

  
  /* now, what we need to do if we've successfully loaded an image */
  if ((import_volume = volume_import_file(import_method, submethod,
					  import_filename, model_filename)) == NULL)
    return;

#ifdef AMIDE_DEBUG
  g_print("imported volume name %s\n",import_volume->name);
#endif

  ui_study_add_volume(ui_study, import_volume); /* this adds a reference to the volume*/
  import_volume = volume_free(import_volume); /* so remove a reference */

  /* update the ui_study cavases with the new volume */
  ui_study_update_canvas(ui_study,NUM_VIEWS,UPDATE_ALL);

  return;
}



/* this function converts a gnome canvas event location to a location in the canvas's coord frame */
realpoint_t ui_study_cp_2_rp(ui_study_t * ui_study, view_t view, canvaspoint_t canvas_cp) {

  realpoint_t canvas_rp;
  gint rgb_width, rgb_height;

  rgb_width = gdk_pixbuf_get_width(ui_study->canvas_rgb[view]);
  rgb_height = gdk_pixbuf_get_height(ui_study->canvas_rgb[view]);

  /* and convert */
  canvas_rp.x = 
    ((canvas_cp.x-UI_STUDY_TRIANGLE_HEIGHT)/rgb_width)*ui_study->canvas_corner[view].x;
  canvas_rp.y = 
    ((rgb_height-(canvas_cp.y-UI_STUDY_TRIANGLE_HEIGHT))/rgb_height)*ui_study->canvas_corner[view].y;
  canvas_rp.z = 
    study_view_thickness(ui_study->study)/2.0;

  return canvas_rp;
}

/* this function converts a point in the canvas's coord frame to a gnome canvas event location */
canvaspoint_t ui_study_rp_2_cp(ui_study_t * ui_study, view_t view, realpoint_t canvas_rp) {

  canvaspoint_t canvas_cp;
  gint rgb_width, rgb_height;

  rgb_width = gdk_pixbuf_get_width(ui_study->canvas_rgb[view]);
  rgb_height = gdk_pixbuf_get_height(ui_study->canvas_rgb[view]);

  /* and convert */
  canvas_cp.x = 
    rgb_width * canvas_rp.x/ui_study->canvas_corner[view].x + UI_STUDY_TRIANGLE_HEIGHT;
  canvas_cp.y = 
    rgb_height * (ui_study->canvas_corner[view].y - canvas_rp.y)/ui_study->canvas_corner[view].y 
    + UI_STUDY_TRIANGLE_HEIGHT;

  return canvas_cp;
}

/* function to update the text in the time dialog popup widget */
void ui_study_update_time_button(ui_study_t * ui_study) {

  gchar * temp_string;

  temp_string = g_strdup_printf("%5.1f-%5.1f s",
				study_view_time(ui_study->study),
				study_view_time(ui_study->study)+study_view_duration(ui_study->study));
  gtk_label_set_text(GTK_LABEL(GTK_BIN(ui_study->time_button)->child),temp_string);
  g_free(temp_string);

  return;
}

/* function to update the coordinate frame associated with one of our canvases.
   this is a coord_frame encompasing all the slices associated with one of the orthogonal views */
void ui_study_update_coords_current_view(ui_study_t * ui_study, view_t view) {

  realpoint_t temp_corner[2];

  /* sanity checks */
  if (ui_study->current_slices[view] == NULL) return;

  /* set the origin of the canvas_coord_frame to the current view locations */
  ui_study->canvas_coord_frame[view] = 
    realspace_get_view_coord_frame(study_coord_frame(ui_study->study), view);
  
  /* figure out the corners */
  volumes_get_view_corners(ui_study->current_slices[view], 
			   ui_study->canvas_coord_frame[view], 
			   temp_corner);

  /* allright, reset the offset of the view frame to the lower left front corner and the far corner*/
  temp_corner[1] = realspace_alt_coord_to_base(temp_corner[1], 
					       ui_study->canvas_coord_frame[view]);
  temp_corner[0] = realspace_alt_coord_to_base(temp_corner[0], ui_study->canvas_coord_frame[view]);
  rs_set_offset(&ui_study->canvas_coord_frame[view], temp_corner[0]);

  ui_study->canvas_corner[view] = realspace_base_coord_to_alt(temp_corner[1], 
							      ui_study->canvas_coord_frame[view]);

  return;
}


/* This function updates the little info box which tells us what the different 
   mouse buttons will do */
void ui_study_update_help_info(ui_study_t * ui_study, ui_study_help_info_t which_info, 
			       realpoint_t new_point, amide_data_t value) {

  GnomeCanvasItem * button[HELP_INFO_LINE_3_SHIFT-HELP_INFO_LINE_1+1];
  GnomeCanvasItem * location;
  ui_study_help_info_line_t i_line;
  gchar * location_text;
  realpoint_t location_p;

  if (which_info == HELP_INFO_UPDATE_LOCATION) {
    location_text = g_strdup_printf("location (x,y,z) = \n (% 5.2f,% 5.2f,% 5.2f)", 
				    new_point.x, new_point.y, new_point.z);
  } else if (which_info == HELP_INFO_UPDATE_VALUE) {
    location_text = g_strdup_printf("value = % 5.3f", value);
  } else {
    button[HELP_INFO_LINE_1] = gtk_object_get_data(GTK_OBJECT(ui_study->help_info), "button_1_info");
    button[HELP_INFO_LINE_1_SHIFT] = gtk_object_get_data(GTK_OBJECT(ui_study->help_info), "button_1s_info");
    button[HELP_INFO_LINE_2] = gtk_object_get_data(GTK_OBJECT(ui_study->help_info), "button_2_info");
    button[HELP_INFO_LINE_2_SHIFT] = gtk_object_get_data(GTK_OBJECT(ui_study->help_info), "button_2s_info");
    button[HELP_INFO_LINE_3] = gtk_object_get_data(GTK_OBJECT(ui_study->help_info), "button_3_info");
    button[HELP_INFO_LINE_3_SHIFT] = gtk_object_get_data(GTK_OBJECT(ui_study->help_info), "button_3s_info");
    
    for (i_line=HELP_INFO_LINE_1 ;i_line <= HELP_INFO_LINE_3_SHIFT;i_line++) 
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

    location_p = realspace_alt_coord_to_base(study_view_center(ui_study->study),
					     study_coord_frame(ui_study->study));
    location_text = g_strdup_printf("view center (x,y,z) = \n (% 5.2f,% 5.2f,% 5.2f)", 
				    location_p.x, location_p.y, location_p.z);
  } 
  /* update the location display */
  location = gtk_object_get_data(GTK_OBJECT(ui_study->help_info), "location");
  if (location == NULL) 
    location = gnome_canvas_item_new(gnome_canvas_root(ui_study->help_info),
				     gnome_canvas_text_get_type(),
				     "justification", GTK_JUSTIFY_LEFT,
				     "anchor", GTK_ANCHOR_NORTH_WEST,
				     "text", location_text,
				     "x", (gdouble) 0,
				     "y", (gdouble) (HELP_INFO_LINE_LOCATION*UI_STUDY_HELP_INFO_LINE_HEIGHT),
				     "font", UI_STUDY_HELP_FONT, NULL);
  else /* just need to change the text */
    gnome_canvas_item_set(location, "text", location_text, NULL);

  gtk_object_set_data(GTK_OBJECT(ui_study->help_info), "location", location);

  g_free(location_text);

  return;
}


/* this function is used to draw/update/remove the target lines on the canvases */
void ui_study_update_targets(ui_study_t * ui_study, ui_study_target_action_t action, 
			     realpoint_t center, rgba_t outline_color) {

  view_t i_view;
  guint i;
  GnomeCanvasPoints * target_line_points;
  GnomeCanvasItem ** target_lines;
  canvaspoint_t point0, point1;
  realpoint_t start, end;
  gint rgb_width, rgb_height;

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
  
  for (i_view = 0 ; i_view < NUM_VIEWS ; i_view++) {

    rgb_width = gdk_pixbuf_get_width(ui_study->canvas_rgb[i_view]);
    rgb_height = gdk_pixbuf_get_height(ui_study->canvas_rgb[i_view]);

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
    
    /* figure out some info */
    start = realspace_alt_coord_to_alt(center, study_coord_frame(ui_study->study),
				       ui_study->canvas_coord_frame[i_view]);
    start.x -= study_view_thickness(ui_study->study)/2.0;
    start.y -= study_view_thickness(ui_study->study)/2.0;
    end = realspace_alt_coord_to_alt(center, study_coord_frame(ui_study->study),
				     ui_study->canvas_coord_frame[i_view]);
    end.x += study_view_thickness(ui_study->study)/2.0;
    end.y += study_view_thickness(ui_study->study)/2.0;

    /* get the canvas locations corresponding to the start and end coordinates */
    point0 = ui_study_rp_2_cp(ui_study, i_view, start);
    point1 = ui_study_rp_2_cp(ui_study, i_view, end);
    
    /* line segment 0 */
    target_line_points = gnome_canvas_points_new(3);
    target_line_points->coords[0] = (double) UI_STUDY_TRIANGLE_HEIGHT;
    target_line_points->coords[1] = point1.y;
    target_line_points->coords[2] = point0.x;
    target_line_points->coords[3] = point1.y;
    target_line_points->coords[4] = point0.x;
    target_line_points->coords[5] = (double) UI_STUDY_TRIANGLE_HEIGHT;
    if (action == TARGET_CREATE) 
      target_lines[0] =
	gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[i_view]), gnome_canvas_line_get_type(),
			      "points", target_line_points, 
			      "fill_color_rgba", color_table_rgba_to_uint32(outline_color),
			      "width_pixels", 1, NULL);
    else /* action == TARGET_UPDATE */
      gnome_canvas_item_set(target_lines[0],"points",target_line_points, NULL);
    gnome_canvas_points_unref(target_line_points);
    
    /* line segment 1 */
    target_line_points = gnome_canvas_points_new(3);
    target_line_points->coords[0] = point1.x;
    target_line_points->coords[1] = (double) UI_STUDY_TRIANGLE_HEIGHT;
    target_line_points->coords[2] = point1.x;
    target_line_points->coords[3] = point1.y;
    target_line_points->coords[4] = (double) (rgb_width+UI_STUDY_TRIANGLE_HEIGHT);
    target_line_points->coords[5] = point1.y;
    if (action == TARGET_CREATE)
      target_lines[1] =
	gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[i_view]), gnome_canvas_line_get_type(),
			      "points", target_line_points, 
			      "fill_color_rgba", color_table_rgba_to_uint32(outline_color),
			      "width_pixels", 1, NULL);
    else /* action == TARGET_UPDATE */
      gnome_canvas_item_set(target_lines[1],"points",target_line_points, NULL);
    gnome_canvas_points_unref(target_line_points);
    

    /* line segment 2 */
    target_line_points = gnome_canvas_points_new(3);
    target_line_points->coords[0] = (double) UI_STUDY_TRIANGLE_HEIGHT;
    target_line_points->coords[1] = point0.y;
    target_line_points->coords[2] = point0.x;
    target_line_points->coords[3] = point0.y;
    target_line_points->coords[4] = point0.x;
    target_line_points->coords[5] = (double) (rgb_height+UI_STUDY_TRIANGLE_HEIGHT);
    if (action == TARGET_CREATE)
      target_lines[2] =
	gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[i_view]), gnome_canvas_line_get_type(),
			      "points", target_line_points, 
			      "fill_color_rgba", color_table_rgba_to_uint32(outline_color),
			      "width_pixels", 1, NULL);
    else /* action == TARGET_UPDATE */
      gnome_canvas_item_set(target_lines[2],"points",target_line_points, NULL);
    gnome_canvas_points_unref(target_line_points);


    /* line segment 3 */
    target_line_points = gnome_canvas_points_new(3);
    target_line_points->coords[0] = point1.x;
    target_line_points->coords[1] = (double) (rgb_height+UI_STUDY_TRIANGLE_HEIGHT);
    target_line_points->coords[2] = point1.x;
    target_line_points->coords[3] = point0.y;
    target_line_points->coords[4] = (double) (rgb_width+UI_STUDY_TRIANGLE_HEIGHT);
    target_line_points->coords[5] = point0.y;
    if (action == TARGET_CREATE) 
      target_lines[3] =
	gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[i_view]), gnome_canvas_line_get_type(),
			      "points", target_line_points, 
			      "fill_color_rgba", color_table_rgba_to_uint32(outline_color),
			      "width_pixels", 1, NULL);
    else /* action == TARGET_UPDATE */
      gnome_canvas_item_set(target_lines[3],"points",target_line_points, NULL);
    gnome_canvas_points_unref(target_line_points);


  }

  return;
}

/* function to update the adjustments for the plane scrollbar */
GtkObject * ui_study_update_plane_adjustment(ui_study_t * ui_study, view_t view) {

  realspace_t view_coord_frame;
  realpoint_t view_corner[2];
  realpoint_t view_center;
  floatpoint_t upper, lower;
  floatpoint_t min_voxel_size;
  realpoint_t zp_start;
  GtkObject * adjustment;

  /* which adjustment and coord frame */
  adjustment = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "plane_adjustment");

  if (ui_study->current_volumes == NULL) {   /* junk values */
    min_voxel_size = 1.0;
    upper = lower = zp_start.z = 0.0;
  } else { /* calculate values */

    view_coord_frame = 
      realspace_get_view_coord_frame(study_coord_frame(ui_study->study), view);
    ui_volume_list_get_view_corners(ui_study->current_volumes, view_coord_frame, view_corner);
    min_voxel_size = ui_volume_list_max_min_voxel_size(ui_study->current_volumes);

    upper = view_corner[1].z;
    lower = view_corner[0].z;
    view_center = study_view_center(ui_study->study);

    /* translate the view center point so that the z coordinate corresponds to depth in this view */
    zp_start = realspace_alt_coord_to_alt(view_center,
					  study_coord_frame(ui_study->study),
					  view_coord_frame);
    
    /* make sure our view center makes sense */
    if (zp_start.z < lower) {

      if (zp_start.z < lower-study_view_thickness(ui_study->study))
	zp_start.z = (upper-lower)/2.0+lower;
      else
	zp_start.z = lower;

      view_center = realspace_alt_coord_to_alt(zp_start,
					       view_coord_frame,
					       study_coord_frame(ui_study->study));
      study_set_view_center(ui_study->study, view_center); /* save the updated view coords */

    } else if (zp_start.z > upper) {

      if (zp_start.z > lower+study_view_thickness(ui_study->study))
	zp_start.z = (upper-lower)/2.0+lower;
      else
	zp_start.z = upper;
      view_center = realspace_alt_coord_to_alt(zp_start,
					       view_coord_frame,
					       study_coord_frame(ui_study->study));
      study_set_view_center(ui_study->study, view_center); /* save the updated view coords */

    }
  }
  

  /* if we haven't yet made the adjustment, make it */
  if (adjustment == NULL) {
    adjustment = gtk_adjustment_new(zp_start.z, lower, upper,
				    min_voxel_size,min_voxel_size,
				    study_view_thickness(ui_study->study));
    /*save this, so we can change it later*/
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[view]), "plane_adjustment", adjustment); 
  } else {
    GTK_ADJUSTMENT(adjustment)->upper = upper;
    GTK_ADJUSTMENT(adjustment)->lower = lower;
    GTK_ADJUSTMENT(adjustment)->page_increment = min_voxel_size;
    GTK_ADJUSTMENT(adjustment)->page_size = study_view_thickness(ui_study->study);
    GTK_ADJUSTMENT(adjustment)->value = zp_start.z;

    /* allright, we need to update widgets connected to the adjustment without triggering our callback */
    gtk_signal_handler_block_by_func(adjustment, GTK_SIGNAL_FUNC(ui_study_cb_plane_change),
				     ui_study);
    gtk_adjustment_changed(GTK_ADJUSTMENT(adjustment));  
    gtk_signal_handler_unblock_by_func(adjustment, GTK_SIGNAL_FUNC(ui_study_cb_plane_change),
				       ui_study);

  }

  return adjustment;
}



/* updates the settings of the thickness_adjustment, will not change anything about the canvas */
void ui_study_update_thickness_adjustment(ui_study_t * ui_study) { 

  floatpoint_t min_voxel_size, max_size;

  if (study_volumes(ui_study->study) == NULL)
    return;

  /* there's no thickness adjustment if we don't create the toolbar at this moment */
  if (ui_study->thickness_adjustment == NULL)
    return;

  /* block signals to the thickness adjustment, as we only want to
     change the value of the adjustment, it's up to the caller of this
     function to change anything on the actual canvases... we'll 
     unblock at the end of this function */
  gtk_signal_handler_block_by_func(ui_study->thickness_adjustment,
				   GTK_SIGNAL_FUNC(ui_study_cb_thickness), 
				   ui_study);


  min_voxel_size = volumes_min_voxel_size(study_volumes(ui_study->study));
  max_size = volumes_max_size(study_volumes(ui_study->study));

  /* set the current thickness if it hasn't already been set or if it's no longer valid*/
  if (study_view_thickness(ui_study->study) < min_voxel_size)
    study_view_thickness(ui_study->study) = min_voxel_size;

  GTK_ADJUSTMENT(ui_study->thickness_adjustment)->upper = max_size;
  GTK_ADJUSTMENT(ui_study->thickness_adjustment)->lower = min_voxel_size;
  GTK_ADJUSTMENT(ui_study->thickness_adjustment)->page_size = min_voxel_size;
  GTK_ADJUSTMENT(ui_study->thickness_adjustment)->step_increment = min_voxel_size;
  GTK_ADJUSTMENT(ui_study->thickness_adjustment)->page_increment = min_voxel_size;
  GTK_ADJUSTMENT(ui_study->thickness_adjustment)->value = study_view_thickness(ui_study->study);
  gtk_adjustment_changed(GTK_ADJUSTMENT(ui_study->thickness_adjustment));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_study->thickness_spin_button),
			    study_view_thickness(ui_study->study));   
  gtk_spin_button_configure(GTK_SPIN_BUTTON(ui_study->thickness_spin_button),
			    GTK_ADJUSTMENT(ui_study->thickness_adjustment),
			    study_view_thickness(ui_study->study),
			    2);
  /* and now, reconnect the signal */
  gtk_signal_handler_unblock_by_func(ui_study->thickness_adjustment, 
				     GTK_SIGNAL_FUNC(ui_study_cb_thickness), 
				     ui_study);
  return;
}




/* function to update the arrows on the canvas */
void ui_study_update_canvas_arrows(ui_study_t * ui_study, view_t view) {

  GnomeCanvasPoints * points[4];
  canvaspoint_t point0, point1;
  realpoint_t start, end;
  realpoint_t view_center;
  gint rgb_width, rgb_height;
  GnomeCanvasItem * canvas_arrow[4];

  /* get points to the previous arrows (if they exist */
  canvas_arrow[0] = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "canvas_arrow0");
  canvas_arrow[1] = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "canvas_arrow1");
  canvas_arrow[2] = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "canvas_arrow2");
  canvas_arrow[3] = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view]), "canvas_arrow3");

  /* if no images, destroy the arrows */
  if ((study_volumes(ui_study->study) == NULL) || (ui_study->current_volumes == NULL)) {
    if (canvas_arrow[0] != NULL)
      gtk_object_destroy(GTK_OBJECT(canvas_arrow[0]));
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[view]), "canvas_arrow0", NULL);
    if (canvas_arrow[1] != NULL)
      gtk_object_destroy(GTK_OBJECT(canvas_arrow[1]));
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[view]), "canvas_arrow1", NULL);
    if (canvas_arrow[2] != NULL)
      gtk_object_destroy(GTK_OBJECT(canvas_arrow[2]));
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[view]), "canvas_arrow2", NULL);
    if (canvas_arrow[3] != NULL)
      gtk_object_destroy(GTK_OBJECT(canvas_arrow[3]));
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[view]), "canvas_arrow3", NULL);
    return;
  }   /* otherwise, create/redraw the arrows */

  /* figure out the dimensions of the view "box" */
  view_center = study_view_center(ui_study->study);
  start = realspace_alt_coord_to_alt(view_center, study_coord_frame(ui_study->study),
				     ui_study->canvas_coord_frame[view]);
  start.x -= study_view_thickness(ui_study->study)/2.0;
  start.y -= study_view_thickness(ui_study->study)/2.0;
  end = realspace_alt_coord_to_alt(view_center, study_coord_frame(ui_study->study),
				   ui_study->canvas_coord_frame[view]);
  end.x += study_view_thickness(ui_study->study)/2.0;
  end.y += study_view_thickness(ui_study->study)/2.0;
  
  /* get the canvas locations corresponding to the start and end coordinates */
  point0 = ui_study_rp_2_cp(ui_study,view, start);
  point1 = ui_study_rp_2_cp(ui_study,view, end);

  /* need to get the height and width of the canvas's rgb image*/
  rgb_width = gdk_pixbuf_get_width(ui_study->canvas_rgb[view]);
  rgb_height = gdk_pixbuf_get_height(ui_study->canvas_rgb[view]);


  /* notes:
     1) even coords are the x coordinate, odd coords are the y
     2) drawing coordinate frame starts from the top left
     3) X's origin is top left, ours is bottom left
  */

  points[0] = gnome_canvas_points_new(4);
  points[1] = gnome_canvas_points_new(4);
  points[2] = gnome_canvas_points_new(4);
  points[3] = gnome_canvas_points_new(4);

  /* left arrow */
  points[0]->coords[0] = UI_STUDY_TRIANGLE_HEIGHT-1.0;
  points[0]->coords[1] = point1.y;
  points[0]->coords[2] = UI_STUDY_TRIANGLE_HEIGHT-1.0;
  points[0]->coords[3] = point0.y;
  points[0]->coords[4] = 0;
  points[0]->coords[5] = point0.y + UI_STUDY_TRIANGLE_WIDTH/2.0;
  points[0]->coords[6] = 0;
  points[0]->coords[7] = point1.y - UI_STUDY_TRIANGLE_WIDTH/2.0;

  /* top arrow */
  points[1]->coords[0] = point0.x;
  points[1]->coords[1] = UI_STUDY_TRIANGLE_HEIGHT-1.0;
  points[1]->coords[2] = point1.x;
  points[1]->coords[3] = UI_STUDY_TRIANGLE_HEIGHT-1.0;
  points[1]->coords[4] = point1.x + UI_STUDY_TRIANGLE_WIDTH/2.0;
  points[1]->coords[5] = 0;
  points[1]->coords[6] = point0.x - UI_STUDY_TRIANGLE_WIDTH/2.0;
  points[1]->coords[7] = 0;


  /* right arrow */
  points[2]->coords[0] = UI_STUDY_TRIANGLE_HEIGHT + rgb_width;
  points[2]->coords[1] = point1.y;
  points[2]->coords[2] = UI_STUDY_TRIANGLE_HEIGHT + rgb_width;
  points[2]->coords[3] = point0.y;
  points[2]->coords[4] = 2*(UI_STUDY_TRIANGLE_HEIGHT)-1.0 + rgb_width;
  points[2]->coords[5] = point0.y + UI_STUDY_TRIANGLE_WIDTH/2;
  points[2]->coords[6] = 2*(UI_STUDY_TRIANGLE_HEIGHT)-1.0 + rgb_width;
  points[2]->coords[7] = point1.y - UI_STUDY_TRIANGLE_WIDTH/2;

  /* bottom arrow */
  points[3]->coords[0] = point0.x;
  points[3]->coords[1] = UI_STUDY_TRIANGLE_HEIGHT + rgb_height;
  points[3]->coords[2] = point1.x;
  points[3]->coords[3] = UI_STUDY_TRIANGLE_HEIGHT + rgb_height;
  points[3]->coords[4] = point1.x + UI_STUDY_TRIANGLE_WIDTH/2;
  points[3]->coords[5] = 2*(UI_STUDY_TRIANGLE_HEIGHT)-1.0 + rgb_height;
  points[3]->coords[6] = point0.x - UI_STUDY_TRIANGLE_WIDTH/2;
  points[3]->coords[7] =  2*(UI_STUDY_TRIANGLE_HEIGHT)-1.0 + rgb_height;

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
  realpoint_t temp_rp;
  gint width, height;
  gint rgb_width, rgb_height;
  rgba_t blank_rgba;
  GtkStyle * widget_style;

  if (ui_study->canvas_rgb[view] != NULL) {
    width = gdk_pixbuf_get_width(ui_study->canvas_rgb[view]);
    height = gdk_pixbuf_get_height(ui_study->canvas_rgb[view]);
    gdk_pixbuf_unref(ui_study->canvas_rgb[view]);
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

    blank_rgba.r = widget_style->bg[GTK_STATE_NORMAL].red >> 8;
    blank_rgba.g = widget_style->bg[GTK_STATE_NORMAL].green >> 8;
    blank_rgba.b = widget_style->bg[GTK_STATE_NORMAL].blue >> 8;
    blank_rgba.a = 0xFF;

    ui_study->canvas_rgb[view] = image_blank(width,height, blank_rgba);

  } else {
    /* figure out our view coordinate frame */
    temp_rp.x = temp_rp.y = temp_rp.z = 0.5*study_view_thickness(ui_study->study);
    temp_rp = rp_sub(study_view_center(ui_study->study), temp_rp);
    view_coord_frame = study_coord_frame(ui_study->study);
    rs_set_offset(&view_coord_frame, realspace_alt_coord_to_base(temp_rp,view_coord_frame));
    view_coord_frame = realspace_get_view_coord_frame(view_coord_frame, view);
    
    /* first, generate a volume_list we can pass to image_from_volumes */
    temp_volumes = ui_volume_list_return_volume_list(ui_study->current_volumes);
    
    ui_study->canvas_rgb[view] = image_from_volumes(&(ui_study->current_slices[view]),
						    temp_volumes,
						    study_view_time(ui_study->study),
						    study_view_duration(ui_study->study),
						    study_view_thickness(ui_study->study),
						    view_coord_frame,
						    study_scaling(ui_study->study),
						    study_zoom(ui_study->study),
						    study_interpolation(ui_study->study));

    /* and delete the volume_list */
    temp_volumes = volume_list_free(temp_volumes);

    /* regenerate the canvas coordinate frames */
    ui_study_update_coords_current_view(ui_study, view);
  }

  rgb_width = gdk_pixbuf_get_width(ui_study->canvas_rgb[view]);
  rgb_height = gdk_pixbuf_get_height(ui_study->canvas_rgb[view]);

  /* reset the min size of the widget and set the scroll region */
  if ((width != rgb_width) || (height != rgb_height) || 
      (ui_study->canvas_image[view] == NULL)) {
    gtk_widget_set_usize(GTK_WIDGET(ui_study->canvas[view]), 
			 rgb_width + 2 * UI_STUDY_TRIANGLE_HEIGHT, 
			 rgb_height + 2 * UI_STUDY_TRIANGLE_HEIGHT);
    gnome_canvas_set_scroll_region(ui_study->canvas[view], 0.0, 0.0, 
				   rgb_width + 2 * UI_STUDY_TRIANGLE_HEIGHT,
				   rgb_height + 2 * UI_STUDY_TRIANGLE_HEIGHT);
  }
	
  /* put the canvas rgb image on the canvas_image */
  if (ui_study->canvas_image[view] != NULL)
    gnome_canvas_item_set(ui_study->canvas_image[view], "pixbuf", ui_study->canvas_rgb[view],  NULL);
  else
    /* time to make a new image */
    ui_study->canvas_image[view] =
      gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[view]),
			    gnome_canvas_pixbuf_get_type(),
			    "pixbuf", ui_study->canvas_rgb[view],
			    "x", (double) UI_STUDY_TRIANGLE_HEIGHT,
			    "y", (double) UI_STUDY_TRIANGLE_HEIGHT,
			    NULL);

  return;
}


/* function to draw an roi for a canvas, for ELLIPSOID, BOX, and CYLINDER */
GnomeCanvasItem *  ui_study_update_canvas_roi_line(ui_study_t * ui_study, 
						   view_t view, 
						   GnomeCanvasItem * roi_item,
						   roi_t * roi) {
  
  GnomeCanvasPoints * item_points;
  GnomeCanvasItem * item;
  GSList * roi_points, * temp;
  guint j;
  rgba_t outline_color;
  double affine[6];
  canvaspoint_t roi_cp;

  /* sanity check */
  if ((ui_study->current_volume == NULL) || (ui_study->current_slices[view] == NULL)) {
    if (roi_item != NULL) /* if no slices, destroy the old roi_item */
      gtk_object_destroy(GTK_OBJECT(roi_item));
    return NULL;
  }

  /* and figure out the outline color from that*/
  outline_color = color_table_outline_color(ui_study->current_volume->color_table, TRUE);

  /* get the points */
  roi_points =  roi_get_slice_intersection_line(roi, ui_study->current_slices[view]->volume);

  /* count the points */
  j=0;
  temp=roi_points;
  while(temp!=NULL) {
    temp=temp->next;
    j++;
  }

  /* if not enough points for a line, destroy the old roi_item */
  if (j<=1) {
    if (roi_item != NULL) 
      gtk_object_destroy(GTK_OBJECT(roi_item));
    return NULL;
  }


  /* transfer the points list to what we'll be using to construction the figure */
  item_points = gnome_canvas_points_new(j);
  temp=roi_points;
  j=0;
  while(temp!=NULL) {
    roi_cp = ui_study_rp_2_cp(ui_study, view, *((realpoint_t *) temp->data));
    item_points->coords[j] = roi_cp.x;
    item_points->coords[j+1] = roi_cp.y;
    temp=temp->next;
    j += 2;
  }

  roi_points = roi_free_points_list(roi_points);

  /* create the item */ 
  if (roi_item != NULL) {
    /* make sure to reset any affine translations we've done */
    gnome_canvas_item_i2w_affine(GNOME_CANVAS_ITEM(roi_item),affine);
    affine[0] = affine[3] = 1.0;
    affine[1] = affine[2] = affine[4] = affine[5] = affine[6] = 0.0;
    gnome_canvas_item_affine_absolute(GNOME_CANVAS_ITEM(roi_item),affine);

    /* and reset the line points */
    gnome_canvas_item_set(roi_item, 
			  "points", item_points, 
			  "fill_color_rgba", color_table_rgba_to_uint32(outline_color),
			  "width_pixels", ui_study->roi_width,
			  "line_style", ui_study->line_style,
			  NULL);
    item = roi_item;

  } else {
    item =  gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[view]),
    				  gnome_canvas_line_get_type(),
    				  "points", item_points,
    				  "fill_color_rgba", color_table_rgba_to_uint32(outline_color),
    				  "width_pixels", ui_study->roi_width,
    				  "line_style", ui_study->line_style,
    				  NULL);
    
    /* attach it's callback */
    gtk_signal_connect(GTK_OBJECT(item), "event",
    		       GTK_SIGNAL_FUNC(ui_study_cb_canvas_event), ui_study);
    gtk_object_set_data(GTK_OBJECT(item), "view", GINT_TO_POINTER(view));
    gtk_object_set_data(GTK_OBJECT(item), "roi", roi);
    gtk_object_set_data(GTK_OBJECT(item), "object_type", GINT_TO_POINTER(OBJECT_ROI_TYPE));
    
  }

  gnome_canvas_points_unref(item_points);
  return item;

}


/* function to draw an roi for a canvas as an image, for ISOCONTOUR's, */
GnomeCanvasItem *  ui_study_update_canvas_roi_image(ui_study_t * ui_study, 
						    view_t view, 
						    GnomeCanvasItem * roi_item,
						    roi_t * roi) {
  
  GnomeCanvasItem * item;
  rgba_t outline_color;
  double affine[6];
  GdkPixbuf * rgb_image;
  canvaspoint_t offset_cp;
  realpoint_t corner_rp;
  canvaspoint_t corner_cp;
  realspace_t coord_frame;

  /* sanity check */
  if ((ui_study->current_volume == NULL) || (ui_study->current_slices[view] == NULL)) {
    if (roi_item != NULL) {/* if no slices, destroy the old roi_item */
      rgb_image = gtk_object_get_data(GTK_OBJECT(roi_item), "rgb_image");
      gdk_pixbuf_unref(rgb_image);
      gtk_object_destroy(GTK_OBJECT(roi_item));
    }
    return NULL;
  }
  outline_color = color_table_outline_color(ui_study->current_volume->color_table, TRUE);

  /* get the image */
  if (roi_item != NULL) {
    rgb_image = gtk_object_get_data(GTK_OBJECT(roi_item), "rgb_image");
    gdk_pixbuf_unref(rgb_image);
  }
  if ((rgb_image = image_slice_intersection(roi, 
					    ui_study->current_slices[view]->volume, 
					    outline_color,
					    &coord_frame,
					    &corner_rp)) == NULL)
    return roi_item;

  offset_cp = ui_study_rp_2_cp(ui_study, view, 
			       realspace_base_coord_to_alt(rs_offset(coord_frame),
							   ui_study->canvas_coord_frame[view]));
  corner_cp = ui_study_rp_2_cp(ui_study, view, 
			       realspace_alt_coord_to_alt(corner_rp, 
							  coord_frame, 
							  ui_study->canvas_coord_frame[view]));

  /* find the north west corner (in terms of the X reference frame) */
  if (corner_cp.y < offset_cp.y) offset_cp.y = corner_cp.y;
  if (corner_cp.x < offset_cp.x) offset_cp.x = corner_cp.x;

  /* create the item */ 
  if (roi_item != NULL) {
    /* make sure to reset any affine translations we've done */
    gnome_canvas_item_i2w_affine(GNOME_CANVAS_ITEM(roi_item),affine);
    affine[0] = affine[3] = 1.0;
    affine[1] = affine[2] = affine[4] = affine[5] = affine[6] = 0.0;
    gnome_canvas_item_affine_absolute(GNOME_CANVAS_ITEM(roi_item),affine);

    gnome_canvas_item_set(roi_item, "pixbuf", rgb_image, 
			  "x", (double) offset_cp.x,
			  "y", (double) offset_cp.y,
			  NULL);
    item = roi_item;
  } else {
    item =  gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[view]),
				  gnome_canvas_pixbuf_get_type(),
				  "pixbuf", rgb_image,
				  "x", (double) offset_cp.x,
				  "y", (double) offset_cp.y,
				  NULL);
    
    /* attach it's callback */
    gtk_signal_connect(GTK_OBJECT(item), "event",
		       GTK_SIGNAL_FUNC(ui_study_cb_canvas_event), ui_study);
    gtk_object_set_data(GTK_OBJECT(item), "view", GINT_TO_POINTER(view));
    gtk_object_set_data(GTK_OBJECT(item), "roi", roi);
    gtk_object_set_data(GTK_OBJECT(item), "object_type", GINT_TO_POINTER(OBJECT_ROI_TYPE));
  }

  gtk_object_set_data(GTK_OBJECT(item), "rgb_image", rgb_image);

  return item;
}


/* function to update a single roi */
GnomeCanvasItem *  ui_study_update_canvas_roi(ui_study_t * ui_study, 
					      view_t view, 
					      GnomeCanvasItem * roi_item,
					      roi_t * roi) {

  GnomeCanvasItem * return_item;

  if ((roi->type == ISOCONTOUR_2D) || (roi->type == ISOCONTOUR_3D))
    return_item = ui_study_update_canvas_roi_image(ui_study,view, roi_item, roi);
  else {
    return_item = ui_study_update_canvas_roi_line(ui_study,view, roi_item, roi);
  }

  return return_item;
}

/* function to update all the currently selected rois */
void ui_study_update_canvas_rois(ui_study_t * ui_study, view_t view) {
  
  ui_roi_list_t * temp_roi_list=ui_study->current_rois;
  GnomeCanvasItem * item;

  while (temp_roi_list != NULL) {
    item = ui_study_update_canvas_roi(ui_study, view,
				     temp_roi_list->canvas_roi[view],
				     temp_roi_list->roi);
    temp_roi_list->canvas_roi[view] = item;
    temp_roi_list = temp_roi_list->next;
  }
  
  return;
}

/* function to update a canvas, if view_it is  NUM_VIEWS, update
   all canvases */
void ui_study_update_canvas(ui_study_t * ui_study, view_t i_view, ui_study_update_t update) {

  view_t j_view,k_view;
  realpoint_t temp_center;
  realpoint_t view_corner[2];

  if (i_view==NUM_VIEWS) {
    i_view=0;
    j_view=NUM_VIEWS;
  } else
    j_view=i_view+1;

  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_study->canvas[i_view]));

  /* make sure the study coord_frame offset is set correctly, 
     adjust current_view_center if necessary */
  temp_center = realspace_alt_coord_to_base(study_view_center(ui_study->study),
  					    study_coord_frame(ui_study->study));
  if (study_volumes(ui_study->study) != NULL) {
    volumes_get_view_corners(study_volumes(ui_study->study), 
			     study_coord_frame(ui_study->study), view_corner);
    view_corner[0] = realspace_alt_coord_to_base(view_corner[0], study_coord_frame(ui_study->study));
    study_set_coord_frame_offset(ui_study->study, view_corner[0]);
    study_set_view_center(ui_study->study, 
			  realspace_base_coord_to_alt(temp_center, study_coord_frame(ui_study->study)));
  };

  for (k_view=i_view;k_view<j_view;k_view++) {
    switch (update) {
    case REFRESH_IMAGE:
      /* refresh the image, but use the same slice as before */
      ui_study_update_canvas_image(ui_study, k_view);
      break;
    case UPDATE_IMAGE:
      /* freeing the slices indicates to regenerate them */
      ui_study->current_slices[k_view]=volume_list_free(ui_study->current_slices[k_view]); 
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
      ui_study->current_slices[k_view]=volume_list_free(ui_study->current_slices[k_view]); 
      ui_study_update_canvas_image(ui_study, k_view);
      ui_study_update_canvas_rois(ui_study, k_view);
      ui_study_update_canvas_arrows(ui_study,k_view);
      ui_study_update_plane_adjustment(ui_study, k_view);
      break;
    }

  }

  ui_common_remove_cursor(GTK_WIDGET(ui_study->canvas[i_view]));

  return;
}


/* function to update which leaf has the active symbol */
void ui_study_tree_update_active_leaf(ui_study_t * ui_study, GtkWidget * leaf) {

  GtkWidget * prev_active_leaf;
  GtkWidget * label;
  ui_volume_list_t * ui_volume_item;
  
  /* remove the previous active symbol */
  prev_active_leaf = gtk_object_get_data(GTK_OBJECT(ui_study->tree), "active_row");

  if (prev_active_leaf != NULL) {
    label = gtk_object_get_data(GTK_OBJECT(prev_active_leaf), "active_label");
    gtk_label_set_text(GTK_LABEL(label), ui_study_tree_non_active_mark);
  }

  /* set the current active symbol */
  if (leaf != NULL) {
    label = gtk_object_get_data(GTK_OBJECT(leaf), "active_label");
    gtk_label_set_text(GTK_LABEL(label), ui_study_tree_active_mark);
  } else if (ui_study->current_volume != NULL) {
    /* gotta figure out which one to mark */
    ui_volume_item = ui_volume_list_get_ui_volume(ui_study->current_volumes, ui_study->current_volume);
    if (ui_volume_item != NULL) {
      leaf = ui_volume_item->tree_leaf;
      label = gtk_object_get_data(GTK_OBJECT(leaf), "active_label");
      gtk_label_set_text(GTK_LABEL(label), ui_study_tree_active_mark);
    }
  }

  /* save info as to which leaf has the active symbol */
  gtk_object_set_data(GTK_OBJECT(ui_study->tree), "active_row", leaf);
  
  return;
}

/* function adds a roi item to the tree */
void ui_study_tree_add_roi(ui_study_t * ui_study, roi_t * roi) {

  GdkPixmap * icon;
  GtkWidget * leaf;
  GtkWidget * text_label;
  GtkWidget * active_label;
  GtkWidget * pixmap;
  GtkWidget * hbox;
  GtkWidget * event_box;
  GtkWidget * subtree;
  GtkWidget * study_leaf;

  /* which icon to use */
  switch (roi->type) {
  case ELLIPSOID:
    icon = gdk_pixmap_create_from_xpm_d(gtk_widget_get_parent_window(ui_study->tree),NULL,NULL,ROI_xpm);
    break;
  case CYLINDER:
    icon = gdk_pixmap_create_from_xpm_d(gtk_widget_get_parent_window(ui_study->tree),NULL,NULL,ROI_xpm);
    break;
  case BOX:
    icon = gdk_pixmap_create_from_xpm_d(gtk_widget_get_parent_window(ui_study->tree),NULL,NULL,ROI_xpm);
    break;
  default:
    icon = gdk_pixmap_create_from_xpm_d(gtk_widget_get_parent_window(ui_study->tree),NULL,NULL,ROI_xpm);
    break;
  }
  pixmap = gtk_pixmap_new(icon, NULL);

  text_label = gtk_label_new(roi->name);
  active_label = gtk_label_new(ui_study_tree_non_active_mark);
  gtk_widget_set_usize(active_label, 7, -1);

  /* pack the text and icon into a box */
  hbox = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(hbox), active_label, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(hbox), text_label, FALSE, FALSE, 5);
  gtk_widget_show(text_label);
  gtk_widget_show(active_label);
  gtk_widget_show(pixmap);

  /* make an event box so we can get enter_notify events and display the right help*/
  event_box = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(event_box), hbox);
  gtk_object_set_data(GTK_OBJECT(event_box), "which_help", GINT_TO_POINTER(HELP_INFO_TREE_ROI));
  gtk_signal_connect(GTK_OBJECT(event_box), "enter_notify_event",
		     GTK_SIGNAL_FUNC(ui_study_cb_update_help_info), ui_study);
  gtk_signal_connect(GTK_OBJECT(event_box), "leave_notify_event",
		     GTK_SIGNAL_FUNC(ui_study_cb_update_help_info), ui_study);
  gtk_widget_show(hbox);
  
  /* and make the leaf and put the hbox into the leaf */
  leaf = gtk_tree_item_new();
  gtk_container_add(GTK_CONTAINER(leaf), event_box);
  gtk_widget_show(event_box);

  /* save pertinent values */
  gtk_object_set_data(GTK_OBJECT(leaf), "type", GINT_TO_POINTER(UI_STUDY_TREE_ROI));
  gtk_object_set_data(GTK_OBJECT(leaf), "text_label", text_label);
  gtk_object_set_data(GTK_OBJECT(leaf), "active_label", active_label);
  gtk_object_set_data(GTK_OBJECT(leaf), "pixmap", pixmap);
  gtk_object_set_data(GTK_OBJECT(leaf), "object", roi);

  /* attach the callback */
  gtk_signal_connect(GTK_OBJECT(leaf), "button_press_event",
		     GTK_SIGNAL_FUNC(ui_study_cb_tree_leaf_clicked), ui_study);

  /* and add this leaf to the study_leaf's tree */
  study_leaf = gtk_object_get_data(GTK_OBJECT(ui_study->tree), "study_leaf");
  subtree = GTK_TREE_ITEM_SUBTREE(study_leaf);
  if (roi_undrawn(roi)) {
    gtk_tree_select_child(GTK_TREE(subtree), leaf);  /* new roi's are selected when added */
    ui_study->current_rois = ui_roi_list_add_roi(ui_study->current_rois, roi, NULL, leaf);
  }
  gtk_tree_append(GTK_TREE(subtree), leaf);
  gtk_widget_show(leaf);

  return;
}

/* function adds a volume item to the tree */
void ui_study_tree_add_volume(ui_study_t * ui_study, volume_t * volume) {

  GdkPixmap * icon;
  GtkWidget * leaf;
  GtkWidget * text_label;
  GtkWidget * active_label;
  GtkWidget * pixmap;
  GtkWidget * hbox;
  GtkWidget * event_box;
  GtkWidget * subtree;
  GtkWidget * study_leaf;

  /* which icon to use */
  switch (volume->modality) {
  case SPECT:
    icon = gdk_pixmap_create_from_xpm_d(gtk_widget_get_parent_window(ui_study->tree),
					NULL,NULL,SPECT_xpm);
    break;
  case MRI:
    icon = gdk_pixmap_create_from_xpm_d(gtk_widget_get_parent_window(ui_study->tree),
					NULL,NULL,MRI_xpm);
    break;
  case CT:
    icon = gdk_pixmap_create_from_xpm_d(gtk_widget_get_parent_window(ui_study->tree),
					NULL,NULL,CT_xpm);
    break;
  case OTHER:
    icon = gdk_pixmap_create_from_xpm_d(gtk_widget_get_parent_window(ui_study->tree),
					NULL,NULL,OTHER_xpm);
    break;
  case PET:
  default:
    icon = gdk_pixmap_create_from_xpm_d(gtk_widget_get_parent_window(ui_study->tree),
					NULL,NULL,PET_xpm);
    break;
  }
  pixmap = gtk_pixmap_new(icon, NULL);

  text_label = gtk_label_new(volume->name);
  active_label = gtk_label_new(ui_study_tree_non_active_mark);
  gtk_widget_set_usize(active_label, 7, -1);

  /* pack the text and icon into a box */
  hbox = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(hbox), active_label, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(hbox), text_label, FALSE, FALSE, 5);
  gtk_widget_show(text_label);
  gtk_widget_show(active_label);
  gtk_widget_show(pixmap);

  /* make an event box so we can get enter_notify events and display the right help*/
  event_box = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(event_box), hbox);
  gtk_object_set_data(GTK_OBJECT(event_box), "which_help", GINT_TO_POINTER(HELP_INFO_TREE_VOLUME));
  gtk_signal_connect(GTK_OBJECT(event_box), "enter_notify_event",
		     GTK_SIGNAL_FUNC(ui_study_cb_update_help_info), ui_study);
  gtk_signal_connect(GTK_OBJECT(event_box), "leave_notify_event",
		     GTK_SIGNAL_FUNC(ui_study_cb_update_help_info), ui_study);
  gtk_widget_show(hbox);
  
  /* and make the leaf and put the hbox into the leaf */
  leaf = gtk_tree_item_new();
  gtk_container_add(GTK_CONTAINER(leaf), event_box);
  gtk_widget_show(event_box);

  /* save pertinent values */
  gtk_object_set_data(GTK_OBJECT(leaf), "type", GINT_TO_POINTER(UI_STUDY_TREE_VOLUME));
  gtk_object_set_data(GTK_OBJECT(leaf), "text_label", text_label);
  gtk_object_set_data(GTK_OBJECT(leaf), "active_label", active_label);
  gtk_object_set_data(GTK_OBJECT(leaf), "pixmap", pixmap);
  gtk_object_set_data(GTK_OBJECT(leaf), "object", volume);

  /* attach the callback */
  gtk_signal_connect(GTK_OBJECT(leaf), "button_press_event",
		     GTK_SIGNAL_FUNC(ui_study_cb_tree_leaf_clicked), ui_study);

  /* and add this leaf to the study_leaf's tree */
  study_leaf = gtk_object_get_data(GTK_OBJECT(ui_study->tree), "study_leaf");
  subtree = GTK_TREE_ITEM_SUBTREE(study_leaf);
  gtk_tree_append(GTK_TREE(subtree), leaf);
  gtk_widget_show(leaf);

  return;
}


/* function to update the study tree 
   since we use pixmaps in this function, make sure to realize the tree
   before calling this  */
void ui_study_update_tree(ui_study_t * ui_study) {

  GdkPixmap * icon;
  volume_list_t * volume_list;
  roi_list_t * roi_list;
  GtkWidget * leaf;
  GtkWidget * label;
  GtkWidget * pixmap;
  GtkWidget * hbox;
  GtkWidget * event_box;
  GtkWidget * subtree;

  /* sanity check */
  leaf = gtk_object_get_data(GTK_OBJECT(ui_study->tree), "study_leaf");
  if (leaf != NULL)
    return;

  /* make the pixmap we'll be putting into the study line */
  icon = gdk_pixmap_create_from_xpm_d(gtk_widget_get_parent_window(ui_study->tree), NULL,NULL,study_xpm);
  pixmap = gtk_pixmap_new(icon, NULL);
  
  /* make the text label for putting into the study line */
  label = gtk_label_new(study_name(ui_study->study));
  
  /* pack the text and icon into a box */
  hbox = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
  
  /* make an event box so we can get enter_notify events and display the right help*/
  event_box = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(event_box), hbox);
  gtk_object_set_data(GTK_OBJECT(event_box), "which_help", GINT_TO_POINTER(HELP_INFO_TREE_STUDY));
  gtk_signal_connect(GTK_OBJECT(event_box), "enter_notify_event",
		     GTK_SIGNAL_FUNC(ui_study_cb_update_help_info), ui_study);
  gtk_signal_connect(GTK_OBJECT(event_box), "leave_notify_event",
		     GTK_SIGNAL_FUNC(ui_study_cb_update_help_info), ui_study);

  /* and make the leaf and put the hbox into the leaf */
  leaf = gtk_tree_item_new();
  gtk_container_add(GTK_CONTAINER(leaf), event_box);
  
  /* save pertinent values */
  gtk_object_set_data(GTK_OBJECT(leaf), "type", GINT_TO_POINTER(UI_STUDY_TREE_STUDY));
  gtk_object_set_data(GTK_OBJECT(leaf), "text_label", label);
  gtk_object_set_data(GTK_OBJECT(leaf), "pixmap", pixmap);
  
  /* make it so we get the right help info */
  
  /* attach the callback, using "_after" has the side effect of not being able to
     select this leaf, which we desire */
  gtk_signal_connect(GTK_OBJECT(leaf), "button_press_event",
		     GTK_SIGNAL_FUNC(ui_study_cb_tree_leaf_clicked), ui_study);
  
  gtk_tree_append(GTK_TREE(ui_study->tree), leaf);
  gtk_object_set_data(GTK_OBJECT(ui_study->tree), "study_leaf", leaf); /* save a pointer to it */
  
  /*  make the subtree that will hold the volume and roi nodes */
  subtree = gtk_tree_new();
  gtk_tree_item_set_subtree(GTK_TREE_ITEM(leaf), subtree);
  
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
  
  gtk_tree_item_expand(GTK_TREE_ITEM(leaf)); /* have the study leaf expanded by default */
  
  return;
}

/* function to setup the widgets inside of the GnomeApp study */
void ui_study_setup_widgets(ui_study_t * ui_study) {

  GtkWidget * main_table;
  GtkWidget * left_table;
  GtkWidget * middle_table;
  GtkWidget * label;
  GtkWidget * scrollbar;
  GtkObject * adjustment;
  GtkWidget * tree;
  GtkWidget * scrolled;
  //  GtkWidget * viewport;
  GtkWidget * event_box;
  view_t i_view;
  guint main_table_row, main_table_column;
  guint left_table_row;
  gint middle_table_row, middle_table_column;


  /* make and add the main packing table */
  main_table = gtk_table_new(UI_STUDY_MAIN_TABLE_WIDTH,
			     UI_STUDY_MAIN_TABLE_HEIGHT,FALSE);
  main_table_row = main_table_column=0;
  if (GNOME_IS_APP(ui_study->app))
    gnome_app_set_contents(GNOME_APP(ui_study->app), main_table);
  else
    gtk_container_add(GTK_CONTAINER(ui_study->app), main_table);

  /* connect the blank help signal */
  gtk_object_set_data(GTK_OBJECT(ui_study->app), "which_help", GINT_TO_POINTER(HELP_INFO_BLANK));
  gtk_signal_connect(GTK_OBJECT(ui_study->app), "enter_notify_event",
  		     GTK_SIGNAL_FUNC(ui_study_cb_update_help_info), ui_study);

  /* setup the left tree widget */
  left_table = gtk_table_new(2,5, FALSE);
  left_table_row=0;
  gtk_table_attach(GTK_TABLE(main_table), 
		   left_table, 
		   main_table_column, main_table_column+1, 
		   main_table_row, UI_STUDY_MAIN_TABLE_HEIGHT,
		   0, //X_PACKING_OPTIONS | GTK_FILL, 
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);
  main_table_column++;
  main_table_row = 0;

  /* make an event box, this allows us to catch events for the scrolled area */
  event_box = gtk_event_box_new();
  gtk_table_attach(GTK_TABLE(left_table), event_box,
  		   0,2, left_table_row, left_table_row+1,
  		   X_PACKING_OPTIONS | GTK_FILL, 
  		   Y_PACKING_OPTIONS | GTK_FILL, 
  		   X_PADDING, Y_PADDING);
  left_table_row++;
  gtk_object_set_data(GTK_OBJECT(event_box), "which_help", GINT_TO_POINTER(HELP_INFO_TREE_NONE));
  gtk_signal_connect(GTK_OBJECT(event_box), "enter_notify_event",
      		     GTK_SIGNAL_FUNC(ui_study_cb_update_help_info), ui_study);
  gtk_signal_connect(GTK_OBJECT(event_box), "button_press_event",
		     GTK_SIGNAL_FUNC(ui_study_cb_tree_clicked), ui_study);

  /* make a scrolled area for the tree */
  scrolled = gtk_scrolled_window_new(NULL,NULL);
  gtk_widget_set_usize(scrolled,200,200);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), 
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(event_box),scrolled);

  /* make the tree */
  tree = gtk_tree_new();
  gtk_tree_set_selection_mode(GTK_TREE(tree), GTK_SELECTION_MULTIPLE);
  gtk_tree_set_view_lines(GTK_TREE(tree), FALSE);
  gtk_object_set_data(GTK_OBJECT(tree), "active_row", NULL);
  gtk_object_set_data(GTK_OBJECT(tree), "study_leaf", NULL);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled),tree);
  ui_study->tree = tree;
  gtk_widget_realize(tree); /* realize now so we can add pixmaps to the tree now */

  /* populate the tree */
  ui_study_update_tree(ui_study);

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
  gtk_object_set_data(GTK_OBJECT(ui_study->help_info), "location", NULL);
  gtk_widget_set_usize(GTK_WIDGET(ui_study->help_info), 150,
		       UI_STUDY_HELP_INFO_LINE_HEIGHT*NUM_HELP_INFO_LINES);
  gnome_canvas_set_scroll_region(ui_study->help_info, 0.0, 0.0, 150.0, 
				 UI_STUDY_HELP_INFO_LINE_HEIGHT*NUM_HELP_INFO_LINES);
  left_table_row++;

  /* make the stuff in the middle, we'll have the entire middle table in a scrolled window  */
  //  scrolled = gtk_scrolled_window_new(NULL,NULL);
  //  gtk_widget_set_usize(scrolled,200,200);
  //  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
  //  				 GTK_POLICY_AUTOMATIC,
  //  				 GTK_POLICY_AUTOMATIC);
  //  gtk_table_attach(GTK_TABLE(main_table), scrolled,
  //  		   main_table_column, main_table_column+1, 
  //  		   main_table_row, UI_STUDY_MAIN_TABLE_HEIGHT,
  //  		   X_PACKING_OPTIONS | GTK_FILL, 
  //  		   Y_PACKING_OPTIONS | GTK_FILL, 
  //  		   X_PADDING, Y_PADDING);

  //  viewport = gtk_viewport_new(NULL, NULL);
  //  gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport), GTK_SHADOW_NONE);
  //  gtk_container_add(GTK_CONTAINER(scrolled), viewport);


  middle_table = gtk_table_new(UI_STUDY_MIDDLE_TABLE_WIDTH, UI_STUDY_MIDDLE_TABLE_HEIGHT,FALSE);
  //  gtk_container_add(GTK_CONTAINER(viewport), middle_table);
  middle_table_row=middle_table_column = 0;
  gtk_table_attach(GTK_TABLE(main_table), middle_table, 
		   main_table_column, main_table_column+1, main_table_row, UI_STUDY_MAIN_TABLE_HEIGHT,
  		   X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL,  X_PADDING, Y_PADDING);


  /* make the three canvases, scrollbars, dials, etc. */
  for (i_view=0;i_view<NUM_VIEWS;i_view++) {
    middle_table_row=0;

    /* make the label for this column */
    label = gtk_label_new(view_names[i_view]);
    gtk_table_attach(GTK_TABLE(middle_table), label, 
		     middle_table_column, middle_table_column+1, middle_table_row, middle_table_row+1,
		     GTK_FILL, FALSE, X_PADDING, Y_PADDING);
    middle_table_row++;

    /* canvas section */
    //ui_study->canvas[i_view] = GNOME_CANVAS(gnome_canvas_new_aa());
    ui_study->canvas[i_view] = GNOME_CANVAS(gnome_canvas_new());

    gtk_table_attach(GTK_TABLE(middle_table), 
		     GTK_WIDGET(ui_study->canvas[i_view]), 
		     middle_table_column, middle_table_column+1,
		     middle_table_row, middle_table_row+1,
		     FALSE,FALSE, X_PADDING, Y_PADDING);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "target_lines", NULL);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "canvas_arrow0", NULL);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "canvas_arrow1", NULL);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "canvas_arrow2", NULL);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "canvas_arrow3", NULL);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "plane_adjustment", NULL);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "view", GINT_TO_POINTER(i_view));
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), 
			"object_type", GINT_TO_POINTER(OBJECT_VOLUME_TYPE));

    /* and put something into the canvas */
    ui_study_update_canvas_image(ui_study, i_view); 

    /* hook up the callbacks */
    gtk_signal_connect(GTK_OBJECT(ui_study->canvas[i_view]), "event",
		       GTK_SIGNAL_FUNC(ui_study_cb_canvas_event),
		       ui_study);
    gtk_object_set_data(GTK_OBJECT(ui_study->canvas[i_view]), "which_help", 
			GINT_TO_POINTER(HELP_INFO_CANVAS_VOLUME));
    gtk_signal_connect(GTK_OBJECT(ui_study->canvas[i_view]), "button_press_event",
		       GTK_SIGNAL_FUNC(ui_study_cb_update_help_info), ui_study);

    middle_table_row++;


    /* scrollbar section */
    adjustment = ui_study_update_plane_adjustment(ui_study, i_view);
    /*so we can figure out which adjustment this is in callbacks */
    gtk_object_set_data(adjustment, "view", GINT_TO_POINTER(i_view));
    scrollbar = gtk_hscrollbar_new(GTK_ADJUSTMENT(adjustment));
    gtk_range_set_update_policy(GTK_RANGE(scrollbar), GTK_UPDATE_DISCONTINUOUS);
    gtk_table_attach(GTK_TABLE(middle_table), 
		     GTK_WIDGET(scrollbar), 
		     middle_table_column, middle_table_column+1,
		     middle_table_row, middle_table_row+1,
		     GTK_FILL,FALSE, X_PADDING, Y_PADDING);
    gtk_signal_connect(adjustment, "value_changed", 
		       GTK_SIGNAL_FUNC(ui_study_cb_plane_change), 
		       ui_study);
    middle_table_row++;

    /* I should hook up a entry widget to this to allow more fine settings */

    middle_table_column++;
  }

  return;
}



/* procedure to set up the study window,
   parent_bin is any container widget that can be cast to a GtkBin,
   if parent_bin is NULL, a new GnomeApp will be created */
GtkWidget * ui_study_create(study_t * study, GtkWidget * parent_bin) {

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

  if (parent_bin == NULL) 
    ui_study->app=gnome_app_new(PACKAGE, title);
  else {
    ui_study->app=gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(ui_study->app), GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(parent_bin), ui_study->app);
  }
  gtk_window_set_policy (GTK_WINDOW(ui_study->app), TRUE, TRUE, TRUE);

  gtk_signal_connect(GTK_OBJECT(ui_study->app), "realize", 
		     GTK_SIGNAL_FUNC(ui_common_window_realize_cb), NULL);
  gtk_object_set_data(GTK_OBJECT(ui_study->app), "ui_study", ui_study);
  g_free(title);

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(ui_study->app), "delete_event",
		     GTK_SIGNAL_FUNC(ui_study_cb_delete_event),
		     ui_study);

  /* setup the study menu */
  if (GNOME_IS_APP(ui_study->app))
    ui_study_menus_create(ui_study);


  /* setup the toolbar */
  if (GNOME_IS_APP(ui_study->app))
    ui_study_toolbar_create(ui_study);

  /* setup the rest of the study window */
  ui_study_setup_widgets(ui_study);

  /* get the study window running */
  gtk_widget_show_all(ui_study->app);
  amide_register_window((gpointer) ui_study->app);

  /* set any settings we can */
  ui_study_update_thickness_adjustment(ui_study);

  /* make sure we're pointing to something sensible */
  if (study_view_center(ui_study->study).x < 0.0) {
    if (study_volumes(ui_study->study) != NULL)
      study_set_view_center(ui_study->study, 
			    volume_calculate_center(study_volumes(ui_study->study)->volume));
  }

  return ui_study->app;
}
