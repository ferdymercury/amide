/* ui_study.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2002 Andy Loening
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
#define UI_STUDY_HELP_FONT "-*-helvetica-medium-r-normal-*-*-100-*-*-p-*-*-*"

#define HELP_INFO_LINE_HEIGHT 13

static gchar * help_info_legends[NUM_HELP_INFO_LINES] = {
  "1", "shift-1",
  "2", "shift-2",
  "3", "shift-3", "ctrl-3"
};

static gchar * help_info_lines[][NUM_HELP_INFO_LINES] = {
  {"",  "", 
   "",  "",    
   "",  "", ""}, /* BLANK */
  {"move view", "shift data set",
   "move view, min. depth", "horizontal align", 
   "change depth", "vertical align",  "add alignment point"},
  {"shift", "", 
   "rotate", "", 
   "scale", "", ""}, /*CANVAS_ROI */
  {"shift",  "", 
   "", "", 
   "", "", ""}, /*CANVAS_ALIGN_PT */
  {"shift", "", 
   "erase point", "erase large point", 
   "start isocontour change", "", ""}, /*CANVAS_ISOCONTOUR_ROI */
  {"draw", "", 
   "", "", 
   "", "", ""}, /* CANVAS_NEW_ROI */
  {"pick isocontour value", "", 
   "", "", 
   "", "", ""}, /* CANVAS_NEW_ISOCONTOUR_ROI */
  {"cancel", "", 
   "cancel", "", 
   "shift", "", ""}, /*CANVAS SHIFT */
  {"cancel", "", 
   "cancel", "", 
   "align", "", ""}, /*CANVAS ALIGN */
  {"select data set", "", 
   "make active", "", 
   "pop up data set dialog", "add alignment point", ""}, /* TREE_VOLUME */
  {"select roi", "", 
   "center view on roi", "", 
   "pop up roi dialog", "", ""}, /* TREE_ROI */
  {"select point", "", 
   "center view on point", "", 
   "pop up point dialog", "", ""}, /* TREE_ALIGN_PT */
  {"", "", 
   "", "", 
   "pop up study dialog","", ""}, /* TREE_STUDY */
  {"", "", 
   "", "", 
   "add roi","", ""} /* TREE_NONE */
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
  view_mode_t i_view_mode;
  help_info_line_t i_line;

  /* alloc space for the data structure for passing ui info */
  if ((ui_study = (ui_study_t *) g_malloc(sizeof(ui_study_t))) == NULL) {
    g_warning("couldn't allocate space for ui_study_t");
    return NULL;
  }
  ui_study->reference_count = 1;

  ui_study->study = NULL;
  ui_study->threshold_dialog = NULL;
  ui_study->preferences_dialog = NULL;
  ui_study->time_dialog = NULL;
  ui_study->thickness_spin = NULL;

  for (i_view_mode=0; i_view_mode < NUM_VIEW_MODES; i_view_mode++) {
    ui_study->active_volume[i_view_mode] = NULL;
    ui_study->tree_event_box[i_view_mode] = NULL;
    ui_study->canvas_table[i_view_mode] = NULL;
  }

  ui_study->study_altered=FALSE;
  ui_study->study_virgin=TRUE;
  ui_study->view_mode=SINGLE_VIEW;
  
  for (i_line=0 ;i_line < NUM_HELP_INFO_LINES;i_line++) {
    ui_study->help_line[i_line] = NULL;
    ui_study->help_legend[i_line] = NULL;
  }

  /* load in saved preferences */
  gnome_config_push_prefix("/"PACKAGE"/");
  ui_study->roi_width = 
    gnome_config_get_int("ROI/Width");
  if (ui_study->roi_width == 0) 
    ui_study->roi_width = 2; /* if no config file, put in sane value */
  ui_study->line_style = 
    gnome_config_get_int("ROI/LineStyle"); /* 0 is solid */
  ui_study->canvas_layout = 
    gnome_config_get_int("CANVAS/Layout"); /* 0 is LINEAR_LAYOUT */
  ui_study->dont_prompt_for_save_on_exit = 
    gnome_config_get_int("MISC/DontPromptForSaveOnExit"); /* 0 is FALSE, so we prompt */

  gnome_config_pop_prefix();

  return ui_study;
}


/* returns a list of the currently selected volumes (from both trees if needed)
   caller is responsible for unrefing the list */
volumes_t * ui_study_selected_volumes(ui_study_t * ui_study) {

  volumes_t * volumes=NULL;
  volumes_t * temp_volumes;
  view_mode_t i_view_mode;

  for (i_view_mode=0; i_view_mode <= ui_study->view_mode; i_view_mode++) {
    temp_volumes = AMITK_TREE_SELECTED_VOLUMES(ui_study->tree[i_view_mode]);
    while(temp_volumes != NULL) {
      if (!volumes_includes_volume(volumes, temp_volumes->volume))
	volumes = volumes_add_volume(volumes, temp_volumes->volume);
      temp_volumes = temp_volumes->next;
    }
  }

  return volumes;
}

/* returns a list of the currently selected roiss (from both trees if needed)
   caller is responsible for unrefing the list */
rois_t * ui_study_selected_rois(ui_study_t * ui_study) {

  rois_t * rois=NULL;
  rois_t * temp_rois;
  view_mode_t i_view_mode;

  for (i_view_mode=0; i_view_mode <= ui_study->view_mode; i_view_mode++) {
    temp_rois = AMITK_TREE_SELECTED_ROIS(ui_study->tree[i_view_mode]);
    while(temp_rois != NULL) {
      if (!rois_includes_roi(rois, temp_rois->roi))
	rois = rois_add_roi(rois, temp_rois->roi);
      temp_rois = temp_rois->next;
    }
  }

  return rois;
}


/* if volume is NULL, it removes the active mark */
void ui_study_make_active_volume(ui_study_t * ui_study, view_mode_t view_mode, volume_t * volume) {

  view_t i_view;
  volumes_t * current_volumes;

  ui_study->active_volume[view_mode] = volume;

  if (volume == NULL) {/* make a guess as to what should be the active volume */
    current_volumes = AMITK_TREE_SELECTED_VOLUMES(ui_study->tree[view_mode]);
    if (current_volumes != NULL)
      ui_study->active_volume[view_mode] = current_volumes->volume;
  }

  /* indicate this is now the active object */
  amitk_tree_set_active_mark(AMITK_TREE(ui_study->tree[view_mode]), 
			     ui_study->active_volume[view_mode], VOLUME);

  for (i_view=0; i_view<NUM_VIEWS; i_view++)
    amitk_canvas_set_active_volume(AMITK_CANVAS(ui_study->canvas[view_mode][i_view]), 
				   ui_study->active_volume[view_mode], TRUE);
  
  /* reset the threshold widget based on the current volume */
  if (ui_study->threshold_dialog != NULL) {
    if (volume == NULL) {
      gtk_widget_destroy(ui_study->threshold_dialog);
      ui_study->threshold_dialog = NULL;
    } else {
      amitk_threshold_dialog_new_volume(AMITK_THRESHOLD_DIALOG(ui_study->threshold_dialog), 
					ui_study->active_volume[view_mode]);
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
  view_mode_t i_view_mode;

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
    for (i_view_mode=0; i_view_mode <= ui_study->view_mode; i_view_mode++)
      amitk_tree_add_object(AMITK_TREE(ui_study->tree[i_view_mode]), new_pt, ALIGN_PT, volume, VOLUME, TRUE);
    new_pt = align_pt_unref(new_pt); /* don't want an extra ref */

    if (ui_study->study_altered != TRUE) {
      ui_study->study_virgin=FALSE;
      ui_study->study_altered=TRUE;
      ui_study_update_title(ui_study);
    }
  }

  return new_pt;
}



/* function to add a new volume into an amide study */
void ui_study_add_volume(ui_study_t * ui_study, volume_t * new_volume) {

  view_t i_view;
  view_mode_t i_view_mode;
  floatpoint_t max_voxel_dim;
  floatpoint_t min_voxel_size;

  /* add the volume to the study structure */
  study_add_volume(ui_study->study, new_volume);
  
  /* and add the volume to the ui_study tree structure */
  for (i_view_mode=0; i_view_mode <= ui_study->view_mode; i_view_mode++)
    amitk_tree_add_object(AMITK_TREE(ui_study->tree[i_view_mode]), 
			  new_volume, VOLUME, 
			  ui_study->study, STUDY, TRUE);
  
  /* update the thickness if appropriate*/
  min_voxel_size = volumes_min_voxel_size(study_volumes(ui_study->study));
  if (min_voxel_size > study_view_thickness(ui_study->study))
    study_set_view_thickness(ui_study->study, min_voxel_size);
  ui_study_update_thickness(ui_study, study_view_thickness(ui_study->study));
  
  /* update the canvases */
  max_voxel_dim = volumes_max_min_voxel_size(study_volumes(ui_study->study));
  for (i_view_mode=0; i_view_mode <= ui_study->view_mode; i_view_mode++)
    for (i_view=0; i_view< NUM_VIEWS; i_view++)
      amitk_canvas_set_voxel_dim(AMITK_CANVAS(ui_study->canvas[i_view_mode][i_view]), max_voxel_dim, TRUE);
  
  /* adjust the time dialog if necessary */
  ui_time_dialog_set_times(ui_study);

  if (ui_study->study_altered != TRUE) {
    ui_study->study_altered=TRUE;
    ui_study->study_virgin=FALSE;
    ui_study_update_title(ui_study);
  }

  return;
}

/* function to add an roi into an amide study */
void ui_study_add_roi(ui_study_t * ui_study, roi_t * new_roi) {

  view_mode_t i_view_mode;

  /* add the roi to the study structure */
  study_add_roi(ui_study->study, new_roi);
  
  /* and add the roi to the ui_study tree structure */
  for (i_view_mode=0; i_view_mode <= ui_study->view_mode; i_view_mode++) {
    amitk_tree_add_object(AMITK_TREE(ui_study->tree[i_view_mode]), 
			  new_roi, ROI, ui_study->study, STUDY, TRUE);
    if (roi_undrawn(new_roi)) /* undrawn roi's selected to begin with*/
      amitk_tree_select_object(AMITK_TREE(ui_study->tree[i_view_mode]), new_roi, ROI);
  }
  
  if (ui_study->study_altered != TRUE) {
    ui_study->study_altered=TRUE;
    ui_study->study_virgin=FALSE;
    ui_study_update_title(ui_study);
  }

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

  help_info_line_t i_line;
  gchar * location_text[2];
  gchar * legend;
  realpoint_t location_p;

  if (which_info == HELP_INFO_UPDATE_LOCATION) {
    location_text[0] = g_strdup_printf("location (x,y,z) =");
    location_text[1] = g_strdup_printf("(% 5.2f,% 5.2f,% 5.2f)", 
				     new_point.x, new_point.y, new_point.z);
  } else if (which_info == HELP_INFO_UPDATE_SHIFT) {
    location_text[0] = g_strdup_printf("shift (x,y,z) =");
    location_text[1] = g_strdup_printf("(% 5.2f,% 5.2f,% 5.2f)", 
				     new_point.x, new_point.y, new_point.z);
  } else if (which_info == HELP_INFO_UPDATE_VALUE) {
    location_text[0] = g_strdup("");
    location_text[1] = g_strdup_printf("value = % 5.3f", value);

  } else if (which_info == HELP_INFO_UPDATE_THETA) {
    location_text[0] = g_strdup("");
    location_text[1] = g_strdup_printf("theta = % 5.3f", value);

  } else {

    for (i_line=0; i_line < HELP_INFO_LINE_BLANK;i_line++) {

      /* the line's legend */
      if (strlen(help_info_lines[which_info][i_line]) > 0)
	legend = help_info_legends[i_line];
      else
	legend = "";
      if (ui_study->help_legend[i_line] == NULL) 
	ui_study->help_legend[i_line] = 
	  gnome_canvas_item_new(gnome_canvas_root(ui_study->help_info),
				gnome_canvas_text_get_type(),
				"justification", GTK_JUSTIFY_RIGHT,
				"anchor", GTK_ANCHOR_NORTH_EAST,
				"text", legend,
				"x", (gdouble) 20.0,
				"y", (gdouble) (i_line*HELP_INFO_LINE_HEIGHT),
				"fill_color", "black", 
				"font", UI_STUDY_HELP_FONT, NULL);
      else /* just need to change the text */
	gnome_canvas_item_set(ui_study->help_legend[i_line], "text", legend, NULL);
      
      /* and the button info */
      if (ui_study->help_line[i_line] == NULL) 
	ui_study->help_line[i_line] = 
	  gnome_canvas_item_new(gnome_canvas_root(ui_study->help_info),
				gnome_canvas_text_get_type(),
				"justification", GTK_JUSTIFY_LEFT,
				"anchor", GTK_ANCHOR_NORTH_WEST,
				"text", help_info_lines[which_info][i_line],
				"x", (gdouble) 30.0,
				"y", (gdouble) (i_line*HELP_INFO_LINE_HEIGHT),
				"fill_color", "black", 
				"font", UI_STUDY_HELP_FONT, NULL);
      else /* just need to change the text */
	gnome_canvas_item_set(ui_study->help_line[i_line], "text", 
			      help_info_lines[which_info][i_line], NULL);
    }

    location_p = realspace_alt_coord_to_base(study_view_center(ui_study->study),
					     study_coord_frame(ui_study->study));
    location_text[0] = g_strdup_printf("view center (x,y,z) =");
    location_text[1] = g_strdup_printf("(% 5.2f,% 5.2f,% 5.2f)", 
				     new_point.x, new_point.y, new_point.z);
  }

  /* update the location display */
  for (i_line=HELP_INFO_LINE_LOCATION1; i_line <= HELP_INFO_LINE_LOCATION2;i_line++) {
    if (ui_study->help_line[i_line] == NULL) 
      ui_study->help_line[i_line] =
	gnome_canvas_item_new(gnome_canvas_root(ui_study->help_info),
			      gnome_canvas_text_get_type(),
			      "justification", GTK_JUSTIFY_LEFT,
			      "anchor", GTK_ANCHOR_NORTH_WEST,
			      "text", location_text[i_line-HELP_INFO_LINE_LOCATION1],
			      "x", (gdouble) 0.0,
			      "y", (gdouble) (i_line*HELP_INFO_LINE_HEIGHT),
			      "fill_color", "black",
			      "font", UI_STUDY_HELP_FONT, NULL);
    else /* just need to change the text */
      gnome_canvas_item_set(ui_study->help_line[i_line], "text", 
			    location_text[i_line-HELP_INFO_LINE_LOCATION1],  NULL);

    g_free(location_text[i_line-HELP_INFO_LINE_LOCATION1]);
  }

  return;
}


/* updates the settings of the thickness_adjustment, will not change anything about the canvas */
void ui_study_update_thickness(ui_study_t * ui_study, floatpoint_t thickness) { 

  floatpoint_t min_voxel_size, max_size;
  GtkAdjustment * adjustment;

  if (study_volumes(ui_study->study) == NULL) return;

  /* there's no spin button if we don't create the toolbar at this moment */
  if (ui_study->thickness_spin == NULL) return;

  adjustment = GTK_SPIN_BUTTON(ui_study->thickness_spin)->adjustment;

  /* block signals to the thickness adjustment, as we only want to
     change the value of the adjustment, it's up to the caller of this
     function to change anything on the actual canvases... we'll 
     unblock at the end of this function */
  gtk_signal_handler_block_by_func(GTK_OBJECT(adjustment), 
				   GTK_SIGNAL_FUNC(ui_study_cb_thickness), ui_study);

  min_voxel_size = volumes_min_voxel_size(study_volumes(ui_study->study));
  max_size = volumes_max_size(study_volumes(ui_study->study));

  /* set the current thickness if it hasn't already been set or if it's no longer valid*/
  if (thickness < min_voxel_size)
    thickness = min_voxel_size;

  adjustment->upper = max_size;
  adjustment->lower = min_voxel_size;
  adjustment->page_size = min_voxel_size;
  adjustment->step_increment = min_voxel_size;
  adjustment->page_increment = min_voxel_size;
  adjustment->value = thickness;
  gtk_adjustment_changed(adjustment);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_study->thickness_spin), thickness);
  gtk_spin_button_configure(GTK_SPIN_BUTTON(ui_study->thickness_spin),NULL, thickness, 2);
  /* and now, reconnect the signal */
  gtk_signal_handler_unblock_by_func(GTK_OBJECT(adjustment),
				     GTK_SIGNAL_FUNC(ui_study_cb_thickness), 
				     ui_study);
  return;
}

/* updates the settings of the zoom adjustment, will not change anything about the canvas */
void ui_study_update_zoom(ui_study_t * ui_study, floatpoint_t zoom) { 

  GtkAdjustment * adjustment;

  if (study_volumes(ui_study->study) == NULL) return;

  /* there's no spin button if we don't create the toolbar at this moment */
  if (ui_study->zoom_spin == NULL) return;

  adjustment = GTK_SPIN_BUTTON(ui_study->zoom_spin)->adjustment;

  /* block signals to the zoom adjustment, as we only want to
     change the value of the adjustment, it's up to the caller of this
     function to change anything on the actual canvases... we'll 
     unblock at the end of this function */
  gtk_signal_handler_block_by_func(GTK_OBJECT(adjustment), 
				   GTK_SIGNAL_FUNC(ui_study_cb_zoom), ui_study);

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_study->zoom_spin), zoom);

  /* and now, reconnect the signal */
  gtk_signal_handler_unblock_by_func(GTK_OBJECT(adjustment),
				     GTK_SIGNAL_FUNC(ui_study_cb_zoom), 
				     ui_study);
  return;
}


void ui_study_update_title(ui_study_t * ui_study) {
  
  gchar * title;

  if (ui_study->study_altered) 
    title = g_strdup_printf("Study: %s *",study_name(ui_study->study));
  else
    title = g_strdup_printf("Study: %s",study_name(ui_study->study));

  gtk_window_set_title(GTK_WINDOW(ui_study->app), title);
  g_free(title);

}



void ui_study_setup_layout(ui_study_t * ui_study) {

  view_t i_view;
  gboolean canvas_first_time;
  gboolean tree_first_time;
  floatpoint_t min_voxel_size;
  view_mode_t i_view_mode;
  

  for (i_view_mode = 0; i_view_mode <= ui_study->view_mode; i_view_mode++) {

    if (ui_study->tree_event_box[i_view_mode] != NULL) {
      tree_first_time = FALSE;
      /* add ref so it's not destroyed when we remove it from the container */ 
      gtk_object_ref(GTK_OBJECT(ui_study->tree_event_box[i_view_mode]));
      /* for some reason, removing and adding a tree widget to a container causes the tree's
	 displayed list of items to become duplicated.  The work around here is to only
	 remove the container if it's reallyl being moved (SINGLE_VIEW tree stays in 
	 the same place */
      if (i_view_mode != SINGLE_VIEW) /* this test is a work around, explained above */
	gtk_container_remove(GTK_CONTAINER(ui_study->main_table),
			     ui_study->tree_event_box[i_view_mode]);
    } else { /* new tree */
      tree_first_time = TRUE;

      ui_study->tree[i_view_mode] = amitk_tree_new(i_view_mode);
      amitk_tree_add_object(AMITK_TREE(ui_study->tree[i_view_mode]), ui_study->study, STUDY, NULL, 0, TRUE);
      gtk_tree_set_selection_mode(GTK_TREE(ui_study->tree[i_view_mode]), GTK_SELECTION_MULTIPLE);
      gtk_tree_set_view_lines(GTK_TREE(ui_study->tree[i_view_mode]), FALSE);
      gtk_object_set_data(GTK_OBJECT(ui_study->tree[i_view_mode]), "view_mode", GINT_TO_POINTER(i_view_mode));
      gtk_signal_connect(GTK_OBJECT(ui_study->tree[i_view_mode]), "help_event",
			 GTK_SIGNAL_FUNC(ui_study_cb_tree_help_event), ui_study);
      gtk_signal_connect(GTK_OBJECT(ui_study->tree[i_view_mode]), "select_object", 
			 GTK_SIGNAL_FUNC(ui_study_cb_tree_select_object), ui_study);
      gtk_signal_connect(GTK_OBJECT(ui_study->tree[i_view_mode]), "unselect_object", 
			 GTK_SIGNAL_FUNC(ui_study_cb_tree_unselect_object), ui_study);
      gtk_signal_connect(GTK_OBJECT(ui_study->tree[i_view_mode]), "make_active_object", 
			 GTK_SIGNAL_FUNC(ui_study_cb_tree_make_active_object), ui_study);
      gtk_signal_connect(GTK_OBJECT(ui_study->tree[i_view_mode]), "popup_object", 
			 GTK_SIGNAL_FUNC(ui_study_cb_tree_popup_object), ui_study);
      gtk_signal_connect(GTK_OBJECT(ui_study->tree[i_view_mode]), "add_object", 
			 GTK_SIGNAL_FUNC(ui_study_cb_tree_add_object), ui_study);

      
      /* make a scrolled area for the tree */
      ui_study->tree_scrolled[i_view_mode] = gtk_scrolled_window_new(NULL,NULL);  
      gtk_widget_set_usize(ui_study->tree_scrolled[i_view_mode],200,200);
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ui_study->tree_scrolled[i_view_mode]), 
				     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
      gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(ui_study->tree_scrolled[i_view_mode]), 
      					    ui_study->tree[i_view_mode]);

      
      /* make an event box, this allows us to catch events for the scrolled area */
      ui_study->tree_event_box[i_view_mode] = gtk_event_box_new();

      gtk_object_set_data(GTK_OBJECT(ui_study->tree_event_box[i_view_mode]), "which_help",
			  GINT_TO_POINTER(HELP_INFO_TREE_NONE));
      gtk_signal_connect(GTK_OBJECT(ui_study->tree_event_box[i_view_mode]), "enter_notify_event",
			 GTK_SIGNAL_FUNC(ui_study_cb_update_help_info), ui_study);
      gtk_signal_connect(GTK_OBJECT(ui_study->tree_event_box[i_view_mode]), "button_press_event",
			 GTK_SIGNAL_FUNC(ui_study_cb_tree_clicked), ui_study);
      gtk_container_add(GTK_CONTAINER(ui_study->tree_event_box[i_view_mode]), 
			ui_study->tree_scrolled[i_view_mode]);

    }
    

    if (ui_study->canvas_table[i_view_mode] != NULL) {
      canvas_first_time = FALSE;
      gtk_widget_hide(ui_study->canvas[i_view_mode][SAGITTAL]); /* hide, so we get more fluid moving */
      /* add ref so it's not destroyed when we remove it from the container */
      for (i_view = 0; i_view < NUM_VIEWS; i_view++) {
	gtk_object_ref(GTK_OBJECT(ui_study->canvas[i_view_mode][i_view]));
	gtk_container_remove(GTK_CONTAINER(ui_study->canvas_table[i_view_mode]), 
			     ui_study->canvas[i_view_mode][i_view]);
      }
      gtk_object_ref(GTK_OBJECT(ui_study->canvas_table[i_view_mode]));
      gtk_container_remove(GTK_CONTAINER(ui_study->center_table),
			   ui_study->canvas_table[i_view_mode]);
    } else {/* new canvas */
      canvas_first_time = TRUE;
      ui_study->canvas_table[i_view_mode] = gtk_table_new(3, 2,FALSE);
      
      if (study_volumes(ui_study->study) != NULL)
	min_voxel_size = volumes_max_min_voxel_size(study_volumes(ui_study->study));
      else
	min_voxel_size = 1;

      for (i_view = 0; i_view < NUM_VIEWS; i_view++) {
	ui_study->canvas[i_view_mode][i_view] = 
	  amitk_canvas_new(i_view, 
			   ui_study->canvas_layout,
			   i_view_mode,
			   study_coord_frame(ui_study->study),
			   realspace_alt_coord_to_base(study_view_center(ui_study->study), 
						       study_coord_frame(ui_study->study)),
			   study_view_thickness(ui_study->study),
			   min_voxel_size,
			   study_zoom(ui_study->study),
			   study_view_time(ui_study->study),
			   study_view_duration(ui_study->study),
			   study_interpolation(ui_study->study),
			   ui_study->line_style,
			   ui_study->roi_width);
	
	gtk_signal_connect(GTK_OBJECT(ui_study->canvas[i_view_mode][i_view]), "help_event",
			   GTK_SIGNAL_FUNC(ui_study_cb_canvas_help_event), ui_study);
	gtk_signal_connect(GTK_OBJECT(ui_study->canvas[i_view_mode][i_view]), "view_changing",
			   GTK_SIGNAL_FUNC(ui_study_cb_canvas_view_changing), ui_study);
	gtk_signal_connect(GTK_OBJECT(ui_study->canvas[i_view_mode][i_view]), "view_changed",
			   GTK_SIGNAL_FUNC(ui_study_cb_canvas_view_changed), ui_study);
	gtk_signal_connect(GTK_OBJECT(ui_study->canvas[i_view_mode][i_view]), "view_z_position_changed",
			   GTK_SIGNAL_FUNC(ui_study_cb_canvas_z_position_changed), ui_study);
	gtk_signal_connect(GTK_OBJECT(ui_study->canvas[i_view_mode][i_view]), "object_changed",
			   GTK_SIGNAL_FUNC(ui_study_cb_canvas_object_changed), ui_study);
	gtk_signal_connect(GTK_OBJECT(ui_study->canvas[i_view_mode][i_view]), "isocontour_3d_changed",
			   GTK_SIGNAL_FUNC(ui_study_cb_canvas_isocontour_3d_changed), ui_study);
	gtk_signal_connect(GTK_OBJECT(ui_study->canvas[i_view_mode][i_view]), "new_align_pt",
			   GTK_SIGNAL_FUNC(ui_study_cb_canvas_new_align_pt), ui_study);
	
      }
    }


    /* put the labels, canvases, scrollbars, and trees in the table according to the desired layout */
    if ((i_view_mode != SINGLE_VIEW) || tree_first_time) /* this test is a work around, explained above */
      gtk_table_attach(GTK_TABLE(ui_study->main_table), ui_study->tree_event_box[i_view_mode],
		       ui_study->tree_table_location[i_view_mode][ui_study->canvas_layout].x,
		       ui_study->tree_table_location[i_view_mode][ui_study->canvas_layout].x+1,
		       ui_study->tree_table_location[i_view_mode][ui_study->canvas_layout].y,
		       ui_study->tree_table_location[i_view_mode][ui_study->canvas_layout].y+1,
		       X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL, 
		       X_PADDING, Y_PADDING);

    switch(ui_study->canvas_layout) {
    case ORTHOGONAL_LAYOUT:
      gtk_table_attach(GTK_TABLE(ui_study->canvas_table[i_view_mode]), 
		       ui_study->canvas[i_view_mode][TRANSVERSE], 
		       0,1,1,2, FALSE,FALSE, X_PADDING, Y_PADDING);
      gtk_table_attach(GTK_TABLE(ui_study->canvas_table[i_view_mode]), 
		       ui_study->canvas[i_view_mode][CORONAL], 
		       0,1,2,3, FALSE,FALSE, X_PADDING, Y_PADDING);
      gtk_table_attach(GTK_TABLE(ui_study->canvas_table[i_view_mode]), 
		       ui_study->canvas[i_view_mode][SAGITTAL], 
		       1,2,1,2, FALSE,FALSE, X_PADDING, Y_PADDING);
      gtk_table_attach(GTK_TABLE(ui_study->center_table), ui_study->canvas_table[i_view_mode],
		       i_view_mode, i_view_mode+1, 0,1,
		       X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL,  
		       X_PADDING, Y_PADDING);
      break;
    case LINEAR_LAYOUT:
    default:
      for (i_view=0;i_view<NUM_VIEWS;i_view++) {
	gtk_table_attach(GTK_TABLE(ui_study->canvas_table[i_view_mode]), 
			 ui_study->canvas[i_view_mode][i_view], 
			 i_view, i_view+1, 0,1, FALSE, FALSE, X_PADDING, Y_PADDING);
      }
      gtk_table_attach(GTK_TABLE(ui_study->center_table), ui_study->canvas_table[i_view_mode],
		       0,1,i_view_mode, i_view_mode+1, 
		       X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL,  
		       X_PADDING, Y_PADDING);


      break;
    }

    for (i_view=0; i_view < NUM_VIEWS ;i_view++ ) {
      amitk_canvas_set_layout(AMITK_CANVAS(ui_study->canvas[i_view_mode][i_view]), 
			      ui_study->canvas_layout,
			      study_coord_frame(ui_study->study));
    }


    if (!tree_first_time)
      /* remove the additional reference */
      gtk_object_unref(GTK_OBJECT(ui_study->tree_event_box[i_view_mode]));
    else
      gtk_widget_show_all(ui_study->tree_event_box[i_view_mode]);

    if (!canvas_first_time) {
      /* remove the additional reference */
      for (i_view = 0; i_view < NUM_VIEWS; i_view++)
	gtk_object_unref(GTK_OBJECT(ui_study->canvas[i_view_mode][i_view]));
      gtk_object_unref(GTK_OBJECT(ui_study->canvas_table[i_view_mode]));
      gtk_widget_show(ui_study->canvas[i_view_mode][SAGITTAL]); /* and show */
    } else {
      gtk_widget_show_all(ui_study->canvas_table[i_view_mode]); /* and show */
    }
      
  }



  /* get rid of stuff we're not looking at */
  for (i_view_mode = ui_study->view_mode+1; i_view_mode < NUM_VIEW_MODES; i_view_mode++) {
    if (ui_study->canvas_table[i_view_mode] != NULL) {
      gtk_widget_destroy(ui_study->canvas_table[i_view_mode]);
      ui_study->canvas_table[i_view_mode] = NULL;
    }
    if (ui_study->tree_event_box[i_view_mode] != NULL) {
      gtk_widget_destroy(ui_study->tree_event_box[i_view_mode]);
      ui_study->tree_event_box[i_view_mode] = NULL;
    }
  }

  return;
}

/* function to setup the widgets inside of the GnomeApp study */
void ui_study_setup_widgets(ui_study_t * ui_study) {

  guint main_table_row=0; 
  guint main_table_column=0;

  /* make and add the main packing table */
  ui_study->main_table = gtk_table_new(3,2,FALSE);
  if (GNOME_IS_APP(ui_study->app)) 
    gnome_app_set_contents(GNOME_APP(ui_study->app), ui_study->main_table);
  else
    gtk_container_add(GTK_CONTAINER(ui_study->app), ui_study->main_table);

  /* connect the blank help signal */
  gtk_object_set_data(GTK_OBJECT(ui_study->app), "which_help", GINT_TO_POINTER(HELP_INFO_BLANK));
  gtk_signal_connect(GTK_OBJECT(ui_study->app), "enter_notify_event",
    		     GTK_SIGNAL_FUNC(ui_study_cb_update_help_info), ui_study);

  /* record the location of the 1st tree */
  ui_study->tree_table_location[SINGLE_VIEW][LINEAR_LAYOUT].x = main_table_column;
  ui_study->tree_table_location[SINGLE_VIEW][LINEAR_LAYOUT].y = main_table_row;
  ui_study->tree_table_location[SINGLE_VIEW][ORTHOGONAL_LAYOUT].x = main_table_column;
  ui_study->tree_table_location[SINGLE_VIEW][ORTHOGONAL_LAYOUT].y = main_table_row;
  main_table_row++;


  /* the help information canvas */
  ui_study->help_info = GNOME_CANVAS(gnome_canvas_new_aa());
  gtk_table_attach(GTK_TABLE(ui_study->main_table), GTK_WIDGET(ui_study->help_info), 
		   main_table_column, main_table_column+1, main_table_row, main_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_set_usize(GTK_WIDGET(ui_study->help_info), 150,
		       HELP_INFO_LINE_HEIGHT*NUM_HELP_INFO_LINES);
  gnome_canvas_set_scroll_region(ui_study->help_info, 0.0, 0.0, 150.0, 
				 HELP_INFO_LINE_HEIGHT*NUM_HELP_INFO_LINES);
  main_table_row++;

  /* record the location we'll use for the second tree widget if we're showing it for linear layout */
  g_assert(NUM_LAYOUTS == MAX(ORTHOGONAL_LAYOUT, LINEAR_LAYOUT)+1);
  ui_study->tree_table_location[LINKED_VIEW][LINEAR_LAYOUT].x = main_table_column;
  ui_study->tree_table_location[LINKED_VIEW][LINEAR_LAYOUT].y = main_table_row;

  main_table_row=0;
  main_table_column++;

  /* make the stuff in the center */
  ui_study->center_table = gtk_table_new(2, 2,FALSE);
  gtk_table_attach(GTK_TABLE(ui_study->main_table), ui_study->center_table,
		   main_table_column, main_table_column+1, main_table_row, main_table_row+3,
  		   X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL,  X_PADDING, Y_PADDING);
  main_table_column++;
  main_table_row=0;
  
  /* record the location we'll use for the second tree widget if we're showing it for orthogonal layout */
  ui_study->tree_table_location[LINKED_VIEW][ORTHOGONAL_LAYOUT].x = main_table_column;
  ui_study->tree_table_location[LINKED_VIEW][ORTHOGONAL_LAYOUT].y = main_table_row;

  /* setup the trees and canvases */
  ui_study_setup_layout(ui_study);

  return;
}



/* replace what's currently in the ui_study with the specified study */
void ui_study_replace_study(ui_study_t * ui_study, study_t * study) {

  view_t i_view;
  view_mode_t i_view_mode;

  for (i_view_mode=0; i_view_mode <= ui_study->view_mode; i_view_mode++) {
    amitk_tree_remove_object(AMITK_TREE(ui_study->tree[i_view_mode]), ui_study->study, STUDY);
    for (i_view=0; i_view<NUM_VIEWS; i_view++)
      amitk_canvas_remove_object(AMITK_CANVAS(ui_study->canvas[i_view_mode][i_view]), 
				 ui_study->study, STUDY, TRUE);
  }

  ui_study->study = study_unref(ui_study->study);
  ui_study->study = study;
  ui_study->study_virgin=FALSE;
  ui_study->study_altered=FALSE;

  for (i_view_mode=0; i_view_mode <= ui_study->view_mode; i_view_mode++) {
    amitk_tree_add_object(AMITK_TREE(ui_study->tree[i_view_mode]), ui_study->study, STUDY, NULL, 0, TRUE);
    for (i_view=0; i_view<NUM_VIEWS; i_view++)
      amitk_canvas_add_object(AMITK_CANVAS(ui_study->canvas[i_view_mode][i_view]), 
			      ui_study->study, STUDY, NULL);
  }

  /* set any settings we can */
  ui_study_update_thickness(ui_study, study_view_thickness(ui_study->study));
  ui_study_update_zoom(ui_study, study_zoom(ui_study->study));
  ui_study_update_title(ui_study);

}

/* procedure to set up the study window,
   parent_bin is any container widget that can be cast to a GtkBin,
   if parent_bin is NULL, a new GnomeApp will be created */
GtkWidget * ui_study_create(study_t * study, GtkWidget * parent_bin) {

  ui_study_t * ui_study;
  gchar * temp_string;


  ui_study = ui_study_init();

  /* setup the study window */
  if (study == NULL) {
    ui_study->study = study_init();
    ui_study->study->coord_frame = rs_init();
    temp_string = g_strdup_printf("temp_%d",next_study_num++);
    study_set_name(ui_study->study, temp_string);
    g_free(temp_string);
  } else {
    ui_study->study = study;
    ui_study->study_virgin=FALSE;
  }

  if (parent_bin == NULL) {
    ui_study->app=gnome_app_new(PACKAGE, NULL);
    ui_study_update_title(ui_study);
  } else {
    ui_study->app=gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(ui_study->app), GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(parent_bin), ui_study->app);
  }
  gtk_window_set_policy (GTK_WINDOW(ui_study->app), TRUE, TRUE, TRUE);

  gtk_signal_connect(GTK_OBJECT(ui_study->app), "realize", 
		     GTK_SIGNAL_FUNC(ui_common_window_realize_cb), NULL);
  gtk_object_set_data(GTK_OBJECT(ui_study->app), "ui_study", ui_study);

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
  ui_study_update_thickness(ui_study, study_view_thickness(ui_study->study));
  ui_study_update_zoom(ui_study, study_zoom(ui_study->study));

  return ui_study->app;
}
