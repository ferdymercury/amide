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
#include "study.h"
#include "image.h"
#include "ui_common.h"
#include "ui_common_cb.h"
#include "ui_study.h"
#include "ui_study_cb.h"
#include "ui_study_menus.h"
#include "ui_study_toolbar.h"
#include "ui_time_dialog.h"
#include "amitk_tree.h"
#include "amitk_canvas.h"
#include "amitk_threshold.h"

/* external variables */

/* internal variables */
#define HELP_INFO_LINE_X 0.0
#define HELP_INFO_LINE_HEIGHT 13

typedef enum {
  HELP_INFO_LINE_1,
  HELP_INFO_LINE_1_SHIFT,
  HELP_INFO_LINE_2,
  HELP_INFO_LINE_2_SHIFT,
  HELP_INFO_LINE_3,
  HELP_INFO_LINE_3_SHIFT,
  HELP_INFO_LINE_LOCATION,
  HELP_INFO_LINE_LOCATION2,
  NUM_HELP_INFO_LINES
} help_info_line_t;

static gchar * help_info_lines[][NUM_HELP_INFO_LINES] = {
  {"", "", "", "", "", ""}, /* BLANK */
  {"1 move view", "shift-1 horizontal align", "2 move view, min. depth", "shift-2 vertical align", "3 change depth","shift-3 add alignment point"},
  {"1 shift", "", "2 rotate", "", "3 scale", ""}, /*CANVAS_ROI */
  {"1 shift", "", "", "", "", ""}, /*CANVAS_ALIGN_PT */
  {"1 shift", "", "2 erase point", "shift-2 erase large point", "3 start isocontour change", ""}, /*CANVAS_ISOCONTOUR_ROI */
  {"1 draw", "", "", "", "", ""}, /* CANVAS_NEW_ROI */
  {"1 pick isocontour value", "", "", "", "", ""}, /* CANVAS_NEW_ISOCONTOUR_ROI */
  {"1 cancel", "", "2 cancel", "", "3 align", ""}, /*CANVAS ALIGN */
  {"1 select data set", "", "2 make active", "", "3 pop up data set dialog", "shift-3 add alignment point"}, /* TREE_VOLUME */
  {"1 select roi", "", "2 center view on roi", "", "3 pop up roi dialog", ""}, /* TREE_ROI */
  {"1 select point", "", "2 center view on point", "", "3 pop up point dialog", ""}, /* TREE_ALIGN_PT */
  {"", "", "", "", "3 pop up study dialog",""}, /* TREE_STUDY */
  {"", "", "", "", "3 add roi",""} /* TREE_NONE */
};

static gint next_study_num=1;

/* destroy a ui_study data structure */
ui_study_t * ui_study_free(ui_study_t * ui_study) {

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
    ui_study->study = study_unref(ui_study->study);
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

  ui_study->study = NULL;
  ui_study->active_volume = NULL;
  ui_study->threshold_dialog = NULL;
  ui_study->preferences_dialog = NULL;
  ui_study->study_selected = FALSE;
  ui_study->time_dialog = NULL;
  ui_study->thickness_adjustment = NULL;
  ui_study->thickness_spin_button = NULL;

  ui_study->canvas_table = NULL;

  /* load in saved preferences */
  gnome_config_push_prefix("/"PACKAGE"/");
  ui_study->roi_width = gnome_config_get_int("ROI/Width");
  if (ui_study->roi_width == 0) ui_study->roi_width = 2; /* if no config file, put in sane value */

  ui_study->line_style = gnome_config_get_int("ROI/LineStyle"); /* 0 is solid */

  ui_study->canvas_layout = gnome_config_get_int("CANVAS/Layout"); /* 0 is LINEAR_LAYOUT */

  gnome_config_pop_prefix();

  return ui_study;
}


/* if volume is NULL, it removes the active mark */
void ui_study_make_active_volume(ui_study_t * ui_study, volume_t * volume) {

  view_t i_view;
  volumes_t * current_volumes;

  ui_study->active_volume = volume;

  if (volume == NULL) {/* make a guess as to what should be the active volume */
    current_volumes = AMITK_TREE_SELECTED_VOLUMES(ui_study->tree);
    if (current_volumes != NULL)
      ui_study->active_volume = current_volumes->volume;
  }

  /* indicate this is now the active object */
  amitk_tree_set_active_mark(AMITK_TREE(ui_study->tree), ui_study->active_volume, VOLUME);

  if (ui_study->active_volume != NULL)
    for (i_view=0; i_view<NUM_VIEWS; i_view++)
      amitk_canvas_set_color_table(AMITK_CANVAS(ui_study->canvas[i_view]), 
				   ui_study->active_volume->color_table, TRUE);

  /* reset the threshold widget based on the current volume */
  if (ui_study->threshold_dialog != NULL) {
    if (volume == NULL) {
      gtk_widget_destroy(ui_study->threshold_dialog);
      ui_study->threshold_dialog = NULL;
    } else {
      amitk_threshold_dialog_new_volume(AMITK_THRESHOLD_DIALOG(ui_study->threshold_dialog), 
					ui_study->active_volume);
    }
  }
}

/* function for adding an alignment point to the study */
align_pt_t * ui_study_add_align_pt(ui_study_t * ui_study, volume_t * volume) {

  GtkWidget * entry;
  gchar * pt_name = NULL;
  gint entry_return;
  align_pt_t * new_pt=NULL;
  gchar * temp_string;

  g_return_val_if_fail(volume != NULL, NULL);

  temp_string = g_strdup_printf("Adding Alignment Point for Volume: %s\nEnter Alignment Point Name:",
				volume->name);
  entry = gnome_request_dialog(FALSE, temp_string, "", 256, 
			       ui_common_cb_entry_name,
			       &pt_name, 
			       (GNOME_IS_APP(ui_study->app) ? 
				GTK_WINDOW(ui_study->app) : NULL));
  entry_return = gnome_dialog_run_and_close(GNOME_DIALOG(entry));
  g_free(temp_string);
  
  if (entry_return == 0) {
    new_pt = align_pt_init();
    new_pt->point = realspace_alt_coord_to_alt(study_view_center(ui_study->study),
					       study_coord_frame(ui_study->study),
					       volume->coord_frame);
    align_pt_set_name(new_pt, pt_name);
    volume_add_align_pt(volume, new_pt);
    amitk_tree_add_object(AMITK_TREE(ui_study->tree), new_pt, ALIGN_PT, volume, VOLUME, TRUE);
    new_pt = align_pt_unref(new_pt); /* don't want an extra ref */
  }

  return new_pt;
}



/* function to add a new volume into an amide study */
void ui_study_add_volume(ui_study_t * ui_study, volume_t * new_volume) {

  view_t i_view;
  floatpoint_t max_voxel_dim;
  floatpoint_t min_voxel_size;

  /* add the volume to the study structure */
  study_add_volume(ui_study->study, new_volume);
  
  /* and add the volume to the ui_study tree structure */
  amitk_tree_add_object(AMITK_TREE(ui_study->tree), new_volume, VOLUME, ui_study->study, STUDY, TRUE);
  
  /* update the thickness if appropriate*/
  min_voxel_size = volumes_min_voxel_size(study_volumes(ui_study->study));
  if (min_voxel_size > study_view_thickness(ui_study->study))
    study_set_view_thickness(ui_study->study, min_voxel_size);
  ui_study_update_thickness_adjustment(ui_study, study_view_thickness(ui_study->study));
  
  /* update the canvases */
  max_voxel_dim = volumes_max_min_voxel_size(study_volumes(ui_study->study));
  for (i_view=0; i_view< NUM_VIEWS; i_view++) {
    amitk_canvas_set_voxel_dim(AMITK_CANVAS(ui_study->canvas[i_view]), max_voxel_dim, TRUE);
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
  import_volume = volume_unref(import_volume); /* so remove a reference */

  return;
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


/* This function updates the little info box which tells us what the different 
   mouse buttons will do */
void ui_study_update_help_info(ui_study_t * ui_study, help_info_t which_info, 
			       realpoint_t new_point, amide_data_t value) {

  GnomeCanvasItem * button[HELP_INFO_LINE_3_SHIFT-HELP_INFO_LINE_1+1];
  GnomeCanvasItem * location;
  help_info_line_t i_line;
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
					       "text", help_info_lines[which_info][i_line],
					       "x", (gdouble) HELP_INFO_LINE_X,
					       "y", (gdouble) (i_line*HELP_INFO_LINE_HEIGHT),
					       "font", UI_STUDY_HELP_FONT, NULL);
      else /* just need to change the text */
    	gnome_canvas_item_set(button[i_line], "text", help_info_lines[which_info][i_line], NULL);
    
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
				     "x", (gdouble) HELP_INFO_LINE_X,
				     "y", (gdouble) (HELP_INFO_LINE_LOCATION*HELP_INFO_LINE_HEIGHT),
				     "font", UI_STUDY_HELP_FONT, NULL);
  else /* just need to change the text */
    gnome_canvas_item_set(location, "text", location_text, NULL);

  gtk_object_set_data(GTK_OBJECT(ui_study->help_info), "location", location);

  g_free(location_text);

  return;
}



/* updates the settings of the thickness_adjustment, will not change anything about the canvas */
void ui_study_update_thickness_adjustment(ui_study_t * ui_study, floatpoint_t thickness) { 

  floatpoint_t min_voxel_size, max_size;

  if (study_volumes(ui_study->study) == NULL)  return;

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
  if (thickness < min_voxel_size)
    thickness = min_voxel_size;

  GTK_ADJUSTMENT(ui_study->thickness_adjustment)->upper = max_size;
  GTK_ADJUSTMENT(ui_study->thickness_adjustment)->lower = min_voxel_size;
  GTK_ADJUSTMENT(ui_study->thickness_adjustment)->page_size = min_voxel_size;
  GTK_ADJUSTMENT(ui_study->thickness_adjustment)->step_increment = min_voxel_size;
  GTK_ADJUSTMENT(ui_study->thickness_adjustment)->page_increment = min_voxel_size;
  GTK_ADJUSTMENT(ui_study->thickness_adjustment)->value = thickness;
  gtk_adjustment_changed(GTK_ADJUSTMENT(ui_study->thickness_adjustment));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_study->thickness_spin_button), thickness);
  gtk_spin_button_configure(GTK_SPIN_BUTTON(ui_study->thickness_spin_button),
			    GTK_ADJUSTMENT(ui_study->thickness_adjustment), thickness, 2);
  /* and now, reconnect the signal */
  gtk_signal_handler_unblock_by_func(ui_study->thickness_adjustment, 
				     GTK_SIGNAL_FUNC(ui_study_cb_thickness), 
				     ui_study);
  return;
}



void ui_study_setup_canvas(ui_study_t * ui_study) {

  view_t i_view;
  gboolean first_time = FALSE;
  floatpoint_t min_voxel_size;
  

  if (ui_study->canvas_table != NULL) {
    gtk_widget_hide(ui_study->canvas[SAGITTAL]); /* hide, so we get more fluid moving */
    for (i_view = 0; i_view < NUM_VIEWS; i_view++) {
      /* add ref so it's not destroyed when we remove it from the container */
      gtk_object_ref(GTK_OBJECT(ui_study->canvas[i_view]));
      gtk_container_remove(GTK_CONTAINER(ui_study->canvas_table), ui_study->canvas[i_view]);
    }
  } else {/* new canvas */
    first_time = TRUE;
    ui_study->canvas_table = gtk_table_new(3, 2,FALSE);

    if (study_volumes(ui_study->study) != NULL)
      min_voxel_size = volumes_max_min_voxel_size(study_volumes(ui_study->study));
    else
      min_voxel_size = 1;

    for (i_view = 0; i_view < NUM_VIEWS; i_view++) {

      ui_study->canvas[i_view] = amitk_canvas_new(i_view, 
						  ui_study->canvas_layout,
						  study_coord_frame(ui_study->study),
						  realspace_alt_coord_to_base(study_view_center(ui_study->study), 
									      study_coord_frame(ui_study->study)),
						  study_view_thickness(ui_study->study),
						  min_voxel_size,
						  study_zoom(ui_study->study),
						  study_view_time(ui_study->study),
						  study_view_duration(ui_study->study),
						  study_interpolation(ui_study->study),
						  BW_LINEAR,
						  ui_study->line_style,
						  ui_study->roi_width);

      gtk_signal_connect(GTK_OBJECT(ui_study->canvas[i_view]), "help_event",
			 GTK_SIGNAL_FUNC(ui_study_cb_canvas_help_event), ui_study);
      gtk_signal_connect(GTK_OBJECT(ui_study->canvas[i_view]), "view_changing",
			 GTK_SIGNAL_FUNC(ui_study_cb_canvas_view_changing), ui_study);
      gtk_signal_connect(GTK_OBJECT(ui_study->canvas[i_view]), "view_changed",
			 GTK_SIGNAL_FUNC(ui_study_cb_canvas_view_changed), ui_study);
      gtk_signal_connect(GTK_OBJECT(ui_study->canvas[i_view]), "view_z_position_changed",
			 GTK_SIGNAL_FUNC(ui_study_cb_canvas_z_position_changed), ui_study);
      gtk_signal_connect(GTK_OBJECT(ui_study->canvas[i_view]), "volumes_changed",
			 GTK_SIGNAL_FUNC(ui_study_cb_canvas_volumes_changed), ui_study);
      gtk_signal_connect(GTK_OBJECT(ui_study->canvas[i_view]), "roi_changed",
			 GTK_SIGNAL_FUNC(ui_study_cb_canvas_roi_changed), ui_study);
      gtk_signal_connect(GTK_OBJECT(ui_study->canvas[i_view]), "isocontour_3d_changed",
			 GTK_SIGNAL_FUNC(ui_study_cb_canvas_isocontour_3d_changed), ui_study);
      gtk_signal_connect(GTK_OBJECT(ui_study->canvas[i_view]), "new_align_pt",
			 GTK_SIGNAL_FUNC(ui_study_cb_canvas_new_align_pt), ui_study);

    }    
  }

  /* put the labels, canvases, and scroolbars in the table according to the desired layout */
  switch(ui_study->canvas_layout) {
  case ORTHOGONAL_LAYOUT:
    gtk_table_attach(GTK_TABLE(ui_study->canvas_table), ui_study->canvas[TRANSVERSE], 
		     0,1,1,2, FALSE,FALSE, X_PADDING, Y_PADDING);
    gtk_table_attach(GTK_TABLE(ui_study->canvas_table), ui_study->canvas[CORONAL], 
		     0,1,2,3, FALSE,FALSE, X_PADDING, Y_PADDING);
    gtk_table_attach(GTK_TABLE(ui_study->canvas_table), ui_study->canvas[SAGITTAL], 
		     1,2,1,2, FALSE,FALSE, X_PADDING, Y_PADDING);
    break;
  case LINEAR_LAYOUT:
  default:
    for (i_view=0;i_view<NUM_VIEWS;i_view++) {
      gtk_table_attach(GTK_TABLE(ui_study->canvas_table), ui_study->canvas[i_view], 
		       i_view, i_view+1, 0,1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);
    }
    break;
  }

  for (i_view=0; i_view < NUM_VIEWS ;i_view++ )
    amitk_canvas_set_layout(AMITK_CANVAS(ui_study->canvas[i_view]), 
			    ui_study->canvas_layout,
			    study_coord_frame(ui_study->study));

  if (first_time) /* put something into the canvases */
    ; //    ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_ALL); 
  else {
    /* remove the additional reference */
    for (i_view = 0; i_view < NUM_VIEWS; i_view++) {
      gtk_object_unref(GTK_OBJECT(ui_study->canvas[i_view]));
    }
    gtk_widget_show(ui_study->canvas[SAGITTAL]); /* and show */
  }

  return;
}

/* function to setup the widgets inside of the GnomeApp study */
void ui_study_setup_widgets(ui_study_t * ui_study) {

  GtkWidget * main_table;
  GtkWidget * left_table;
  GtkWidget * scrolled;
  //  GtkWidget * viewport;
  GtkWidget * event_box;
  guint main_table_row, main_table_column;
  guint left_table_row;


  /* make and add the main packing table */
  main_table = gtk_table_new(UI_STUDY_MAIN_TABLE_WIDTH, UI_STUDY_MAIN_TABLE_HEIGHT,FALSE);
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

  /* make and populate the tree */
  ui_study->tree = amitk_tree_new();
  amitk_tree_add_object(AMITK_TREE(ui_study->tree), ui_study->study, STUDY, NULL, 0, TRUE);
  gtk_tree_set_selection_mode(GTK_TREE(ui_study->tree), GTK_SELECTION_MULTIPLE);
  gtk_tree_set_view_lines(GTK_TREE(ui_study->tree), FALSE);
  gtk_object_set_data(GTK_OBJECT(ui_study->tree), "active_row", NULL);
  gtk_object_set_data(GTK_OBJECT(ui_study->tree), "study_leaf", NULL);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled),ui_study->tree);
  gtk_signal_connect(GTK_OBJECT(ui_study->tree), "help_event",
		     GTK_SIGNAL_FUNC(ui_study_cb_tree_help_event), ui_study);
  gtk_signal_connect(GTK_OBJECT(ui_study->tree), "select_object", 
  		     GTK_SIGNAL_FUNC(ui_study_cb_tree_select_object), ui_study);
  gtk_signal_connect(GTK_OBJECT(ui_study->tree), "unselect_object", 
  		     GTK_SIGNAL_FUNC(ui_study_cb_tree_unselect_object), ui_study);
  gtk_signal_connect(GTK_OBJECT(ui_study->tree), "make_active_object", 
  		     GTK_SIGNAL_FUNC(ui_study_cb_tree_make_active_object), ui_study);
  gtk_signal_connect(GTK_OBJECT(ui_study->tree), "popup_object", 
  		     GTK_SIGNAL_FUNC(ui_study_cb_tree_popup_object), ui_study);
  gtk_signal_connect(GTK_OBJECT(ui_study->tree), "add_object", 
  		     GTK_SIGNAL_FUNC(ui_study_cb_tree_add_object), ui_study);

  /* the help information canvas */
  //  gtk_widget_push_visual (gdk_rgb_get_visual ());
  //  gtk_widget_push_colormap (gdk_rgb_get_cmap ());
  //  ui_study->help_info = GNOME_CANVAS(gnome_canvas_new_aa ());
  //  gtk_widget_pop_colormap ();
  //  gtk_widget_pop_visual ();
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
		       HELP_INFO_LINE_HEIGHT*NUM_HELP_INFO_LINES);
  gnome_canvas_set_scroll_region(ui_study->help_info, 0.0, 0.0, 150.0, 
				 HELP_INFO_LINE_HEIGHT*NUM_HELP_INFO_LINES);
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

  ui_study_setup_canvas(ui_study);
  //  gtk_container_add(GTK_CONTAINER(viewport), middle_table);
  gtk_table_attach(GTK_TABLE(main_table), ui_study->canvas_table,
		   main_table_column, main_table_column+1, main_table_row, UI_STUDY_MAIN_TABLE_HEIGHT,
  		   X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL,  X_PADDING, Y_PADDING);


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
    ui_study->study->coord_frame = rs_init();
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
  if (GNOME_IS_APP(ui_study->app)) ui_study_menus_create(ui_study);

  /* setup the toolbar */
  if (GNOME_IS_APP(ui_study->app)) ui_study_toolbar_create(ui_study);

  /* setup the rest of the study window */
  ui_study_setup_widgets(ui_study);

  /* get the study window running */
  gtk_widget_show_all(ui_study->app);
  amide_register_window((gpointer) ui_study->app);

  /* set any settings we can */
  ui_study_update_thickness_adjustment(ui_study, study_view_thickness(ui_study->study));

  return ui_study->app;
}
