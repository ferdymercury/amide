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

#include "amide_config.h"
#include <gnome.h>
#include <string.h>
#include "image.h"
#include "ui_common.h"
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
#define UI_STUDY_HELP_FONT "-*-helvetica-medium-r-normal-*-*-120-*-*-*-*-*-*"

#define HELP_INFO_LINE_HEIGHT 13

static gchar * help_info_legends[NUM_HELP_INFO_LINES] = {
  "1", "shift-1",
  "2", "shift-2",
  "3", "shift-3", "ctrl-3",
  "ctrl-x"
};

static gchar * help_info_lines[][NUM_HELP_INFO_LINES] = {
  {"",  "", 
   "",  "",    
   "",  "", "",
   ""}, /* BLANK */
  {"move view", "shift data set",
   "move view, min. depth", "rotate", 
   "change depth", "",  "add fiducial mark",
   ""},
  {"shift", "", 
   "rotate", "", 
   "scale", "", "set data set to zero",
   ""}, /*CANVAS_ROI */
  {"shift",  "", 
   "", "", 
   "", "", "",
   ""}, /*CANVAS_FIDUCIAL_MARK */
  {"shift", "", 
   "erase isocontour point", "erase large point", 
   "start isocontour change", "", "set data set to zero",
   ""}, /*CANVAS_ISOCONTOUR_ROI */
  {"draw", "", 
   "", "", 
   "", "", "",
   ""}, /* CANVAS_NEW_ROI */
  {"pick isocontour value", "", 
   "", "", 
   "", "", "",
   ""}, /* CANVAS_NEW_ISOCONTOUR_ROI */
  {"cancel", "", 
   "cancel", "", 
   "shift", "", "",
   ""}, /*CANVAS SHIFT DATA SET*/
  {"cancel", "", 
   "cancel", "", 
   "rotate", "", "",
   ""}, /*CANVAS ROTATE DATA SET*/
  {"select data set", "", 
   "make active", "", 
   "pop up data set dialog", "add roi", "add fiducial mark",
   "delete data set"}, /* TREE_DATA_SET */
  {"select roi", "", 
   "center view on roi", "", 
   "pop up roi dialog", "", "",
   "delete roi"}, /* TREE_ROI */
  {"select point", "", 
   "center view on point", "", 
   "pop up point dialog", "", "",
   "delete mark"}, /* TREE_FIDUCIAL_MARK */
  {"", "", 
   "", "", 
   "pop up study dialog","", "",
   ""}, /* TREE_STUDY */
  {"", "", 
   "", "", 
   "add roi","", "",
   ""} /* TREE_NONE */
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
    if (ui_study->study != NULL) {
      amitk_tree_remove_object(AMITK_TREE(ui_study->tree), AMITK_OBJECT(ui_study->study));
      g_object_unref(ui_study->study);
      ui_study->study = NULL;
    }
      
    g_free(ui_study);
    ui_study = NULL;
  }
    
  return ui_study;
}

/* allocate and initialize a ui_study data structure */
ui_study_t * ui_study_init(void) {

  ui_study_t * ui_study;
  AmitkViewMode i_view_mode;
  help_info_line_t i_line;

  /* alloc space for the data structure for passing ui info */
  if ((ui_study = g_new(ui_study_t,1)) == NULL) {
    g_warning("couldn't allocate space for ui_study_t");
    return NULL;
  }
  ui_study->reference_count = 1;

  ui_study->study = NULL;
  ui_study->threshold_dialog = NULL;
  ui_study->preferences_dialog = NULL;
  ui_study->time_dialog = NULL;
  ui_study->thickness_spin = NULL;

  for (i_view_mode=0; i_view_mode < AMITK_VIEW_MODE_NUM; i_view_mode++) {
    ui_study->active_ds = NULL;
    ui_study->canvas_table[i_view_mode] = NULL;
  }

  ui_study->study_altered=FALSE;
  ui_study->study_virgin=TRUE;
  ui_study->view_mode=AMITK_VIEW_MODE_SINGLE;
  
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
    gnome_config_get_int("CANVAS/Layout"); /* 0 is AMITK_LAYOUT_LINEAR */
  ui_study->dont_prompt_for_save_on_exit = 
    gnome_config_get_int("MISC/DontPromptForSaveOnExit"); /* 0 is FALSE, so we prompt */

  gnome_config_pop_prefix();

  return ui_study;
}


/* returns a list of the currently selected data sets (from both trees if needed)
   caller is responsible for unrefing the list */
GList * ui_study_selected_data_sets(ui_study_t * ui_study) {

  GList * data_sets=NULL;
  GList * temp_objects;
  AmitkViewMode i_view_mode;

  for (i_view_mode=0; i_view_mode <= ui_study->view_mode; i_view_mode++) {
    temp_objects = AMITK_TREE_VISIBLE_OBJECTS(ui_study->tree,i_view_mode);
    while(temp_objects != NULL) {
      if (AMITK_IS_DATA_SET(temp_objects->data)) {
	if (g_list_index(data_sets, temp_objects->data)<0) /* not yet in list */
	  data_sets = g_list_append(data_sets, g_object_ref(temp_objects->data));
      }
      temp_objects = temp_objects->next;
    }
  }

  return data_sets;
}

/* returns a list of the currently selected rois (from both trees if needed)
   caller is responsible for unrefing each element of the list */
GList * ui_study_selected_rois(ui_study_t * ui_study) {

  GList * visible_objects;
  GList * return_list=NULL;
  AmitkViewMode i_view_mode;

  for (i_view_mode=0; i_view_mode <= ui_study->view_mode; i_view_mode++) {
    visible_objects = AMITK_TREE_VISIBLE_OBJECTS(ui_study->tree,i_view_mode);
    while(visible_objects != NULL) {
      if (AMITK_IS_ROI(visible_objects->data)) {
	if (g_list_index(return_list, visible_objects->data) < 0)
	  return_list = g_list_append(return_list, g_object_ref(visible_objects->data));
      }
      visible_objects = visible_objects->next;
    }
  }

  return return_list;
}


/* if data set is NULL, it removes the active mark */
void ui_study_make_active_data_set(ui_study_t * ui_study, AmitkDataSet * ds) {

  AmitkView i_view;
  GList * current_objects;
  AmitkViewMode i_view_mode;

  /* disconnect anything */
  if (ui_study->active_ds != NULL) {
    g_signal_handlers_disconnect_by_func(G_OBJECT(ui_study->active_ds), 
					 G_CALLBACK(ui_study_update_interpolation), ui_study);
  }

  ui_study->active_ds = ds;

  if (ds == NULL) {/* make a guess as to what should be the active data set */
    current_objects = AMITK_TREE_VISIBLE_OBJECTS(ui_study->tree,AMITK_VIEW_MODE_SINGLE);
    while (current_objects != NULL) {
      if (AMITK_IS_DATA_SET(current_objects->data)) {
	ui_study->active_ds = AMITK_DATA_SET(current_objects->data);
	current_objects = NULL;
      } else {
	current_objects = current_objects->next;
      }
    }
    if (ui_study->active_ds == NULL) /* if still nothing */
      if (ui_study->view_mode == AMITK_VIEW_MODE_LINKED) { /* go through the second column */
	current_objects = AMITK_TREE_VISIBLE_OBJECTS(ui_study->tree,AMITK_VIEW_MODE_LINKED);
	while (current_objects != NULL) {
	  if (AMITK_IS_DATA_SET(current_objects->data)) {
	    ui_study->active_ds = AMITK_DATA_SET(current_objects->data);
	    current_objects = NULL;
	  } else {
	    current_objects = current_objects->next;
	  }
	}
      }
  }

  /* indicate this is now the active object */
  if (ui_study->active_ds != NULL) {
    amitk_tree_set_active_mark(AMITK_TREE(ui_study->tree), 
			       AMITK_OBJECT(ui_study->active_ds));
    /* connect any needed signals */
    g_signal_connect_swapped(G_OBJECT(ui_study->active_ds), "interpolation_changed", 
			     G_CALLBACK(ui_study_update_interpolation), ui_study);
  } else {
    amitk_tree_set_active_mark(AMITK_TREE(ui_study->tree), NULL);
  }
  ui_study_update_interpolation(ui_study);

  

  for (i_view_mode = 0; i_view_mode <= ui_study->view_mode; i_view_mode++) 
    for (i_view=0; i_view< AMITK_VIEW_NUM; i_view++)
      amitk_canvas_set_active_data_set(AMITK_CANVAS(ui_study->canvas[i_view_mode][i_view]), 
				       ui_study->active_ds);
  
  /* reset the threshold widget based on the current data set */
  if (ui_study->threshold_dialog != NULL) {
    if (ds == NULL) {
      gtk_widget_destroy(ui_study->threshold_dialog);
      ui_study->threshold_dialog = NULL;
    } else {
      amitk_threshold_dialog_new_data_set(AMITK_THRESHOLD_DIALOG(ui_study->threshold_dialog), 
					  ui_study->active_ds);
    }
  }
}

/* function for adding a fiducial mark */
void ui_study_add_fiducial_mark(ui_study_t * ui_study, AmitkObject * parent_object,
				gboolean selected, AmitkPoint position) {

  GtkWidget * entry;
  gchar * pt_name = NULL;
  gint entry_return;
  AmitkFiducialMark * new_pt=NULL;
  gchar * temp_string;

  g_return_if_fail(AMITK_IS_OBJECT(parent_object));

  temp_string = g_strdup_printf("Adding fiducial mark for data set: %s\nEnter the mark's name:",
				AMITK_OBJECT_NAME(parent_object));
  entry = gnome_request_dialog(FALSE, temp_string, "", 256, 
			       ui_common_entry_name_cb,
			       &pt_name, 
			       (GNOME_IS_APP(ui_study->app) ? 
				GTK_WINDOW(ui_study->app) : NULL));
  entry_return = gnome_dialog_run_and_close(GNOME_DIALOG(entry));
  g_free(temp_string);
  

  if (entry_return == 1) {

    new_pt = amitk_fiducial_mark_new();
    amitk_space_copy_in_place(AMITK_SPACE(new_pt), AMITK_SPACE(parent_object));
    amitk_fiducial_mark_set(new_pt, position);
    amitk_object_set_name(AMITK_OBJECT(new_pt), pt_name);
    amitk_object_add_child(AMITK_OBJECT(parent_object), AMITK_OBJECT(new_pt));
    amitk_tree_add_object(AMITK_TREE(ui_study->tree), AMITK_OBJECT(new_pt), TRUE);
    g_object_unref(new_pt); /* don't want an extra ref */

    if (selected)
      amitk_tree_select_object(AMITK_TREE(ui_study->tree), AMITK_OBJECT(new_pt), AMITK_VIEW_MODE_SINGLE);

    if (ui_study->study_altered != TRUE) {
      ui_study->study_virgin=FALSE;
      ui_study->study_altered=TRUE;
      ui_study_update_title(ui_study);
    }
  }

  return;
}


void ui_study_add_roi(ui_study_t * ui_study, AmitkObject * parent_object, AmitkRoiType roi_type) {

  GtkWidget * entry;
  gchar * roi_name = NULL;
  gint entry_return;
  AmitkRoi * roi;
  gchar * temp_string;

  g_return_if_fail(AMITK_IS_OBJECT(parent_object));

  temp_string = g_strdup_printf("Adding ROI to: %s\nEnter ROI Name:",
				AMITK_OBJECT_NAME(parent_object));
  entry = gnome_request_dialog(FALSE, temp_string, "", 256, 
			       ui_common_entry_name_cb,
			       &roi_name, 
			       (GNOME_IS_APP(ui_study->app) ? 
				GTK_WINDOW(ui_study->app) : NULL));
  entry_return = gnome_dialog_run_and_close(GNOME_DIALOG(entry));
  g_free(temp_string);
  
  if (entry_return == 1) {

    roi = amitk_roi_new(roi_type);
    amitk_object_set_name(AMITK_OBJECT(roi), roi_name);
    amitk_object_add_child(parent_object, AMITK_OBJECT(roi));
    amitk_tree_add_object(AMITK_TREE(ui_study->tree), AMITK_OBJECT(roi), TRUE);
    g_object_unref(roi); /* don't want an extra ref */

    if (AMITK_ROI_UNDRAWN(roi)) /* undrawn roi's selected to begin with*/
      amitk_tree_select_object(AMITK_TREE(ui_study->tree),AMITK_OBJECT(roi), AMITK_VIEW_MODE_SINGLE);
  
    if (ui_study->study_altered != TRUE) {
      ui_study->study_altered=TRUE;
      ui_study->study_virgin=FALSE;
      ui_study_update_title(ui_study);
    }
  }

  return;
}


/* function to add an object into an amide study */
void ui_study_add_data_set(ui_study_t * ui_study, AmitkDataSet * data_set) {

  amide_real_t min_voxel_size;

  g_return_if_fail(AMITK_IS_DATA_SET(data_set));

  amitk_object_add_child(AMITK_OBJECT(ui_study->study), AMITK_OBJECT(data_set));
  amitk_tree_add_object(AMITK_TREE(ui_study->tree), AMITK_OBJECT(data_set), TRUE);

  /* update the thickness if appropriate*/
  min_voxel_size = amitk_data_sets_get_min_voxel_size(AMITK_OBJECT_CHILDREN(ui_study->study));
  if (min_voxel_size > AMITK_STUDY_VIEW_THICKNESS(ui_study->study)) {
    amitk_study_set_view_thickness(ui_study->study, min_voxel_size);
    ui_study_update_thickness(ui_study, AMITK_STUDY_VIEW_THICKNESS(ui_study->study));
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
				AMITK_STUDY_VIEW_START_TIME(ui_study->study),
				AMITK_STUDY_VIEW_START_TIME(ui_study->study)+
				AMITK_STUDY_VIEW_DURATION(ui_study->study));
  gtk_label_set_text(GTK_LABEL(GTK_BIN(ui_study->time_button)->child),temp_string);
  g_free(temp_string);

  return;
}


/* This function updates the little info box which tells us what the different 
   mouse buttons will do */
void ui_study_update_help_info(ui_study_t * ui_study, AmitkHelpInfo which_info, 
			       AmitkPoint new_point, amide_data_t value) {

  help_info_line_t i_line;
  gchar * location_text[2];
  gchar * legend;
  AmitkPoint location_p;

  if (which_info == AMITK_HELP_INFO_UPDATE_LOCATION) {
    location_text[0] = g_strdup_printf("[x,y,z] = [% 5.2f,% 5.2f,% 5.2f]", 
				       new_point.x, new_point.y, new_point.z);
    location_text[1] = g_strdup_printf("value  = % 5.3f", value);
  } else if (which_info == AMITK_HELP_INFO_UPDATE_SHIFT) {
    location_text[0] = g_strdup_printf("shift (x,y,z) =");
    location_text[1] = g_strdup_printf("[% 5.2f,% 5.2f,% 5.2f]", 
				     new_point.x, new_point.y, new_point.z);
  } else if (which_info == AMITK_HELP_INFO_UPDATE_THETA) {
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

    location_p = AMITK_STUDY_VIEW_CENTER(ui_study->study);
    location_text[0] = g_strdup_printf("view center (x,y,z) =");
    location_text[1] = g_strdup_printf("[% 5.2f,% 5.2f,% 5.2f]", 
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


/* updates the settings of the thickness spin button, will not change anything about the canvas */
void ui_study_update_thickness(ui_study_t * ui_study, amide_real_t thickness) {


  amide_real_t min_voxel_size, max_size;

  /* there's no spin button if we don't create the toolbar at this moment */
  if (ui_study->thickness_spin == NULL) return;

  /* block signals to the spin button, as we only want to
     change the value of the spin button, it's up to the caller of this
     function to change anything on the actual canvases... we'll 
     unblock at the end of this function */
  g_signal_handlers_block_by_func(G_OBJECT(ui_study->thickness_spin), 
				  G_CALLBACK(ui_study_cb_thickness), ui_study);

  min_voxel_size = amitk_data_sets_get_min_voxel_size(AMITK_OBJECT_CHILDREN(ui_study->study));
  max_size = amitk_volumes_get_max_size(AMITK_OBJECT_CHILDREN(ui_study->study));

  if ((min_voxel_size < 0)  || (max_size < 0)) return; /* no valid objects */

  /* set the current thickness if it hasn't already been set or if it's no longer valid*/
  if (thickness < min_voxel_size)
    thickness = min_voxel_size;

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_study->thickness_spin), thickness);
  gtk_spin_button_set_increments(GTK_SPIN_BUTTON(ui_study->thickness_spin), min_voxel_size, min_voxel_size);
  gtk_spin_button_set_range(GTK_SPIN_BUTTON(ui_study->thickness_spin), min_voxel_size, max_size);
  gtk_spin_button_configure(GTK_SPIN_BUTTON(ui_study->thickness_spin),NULL, thickness, 2);
  /* and now, reconnect the signal */
  g_signal_handlers_unblock_by_func(G_OBJECT(ui_study->thickness_spin),
				    G_CALLBACK(ui_study_cb_thickness), 
				    ui_study);

  return;
}

/* updates the settings of the zoom spinbutton, will not change anything about the canvas */
void ui_study_update_zoom(ui_study_t * ui_study) {

  /* there's no spin button if we don't create the toolbar at this moment */
  if (ui_study->zoom_spin == NULL) return;

  /* block signals to the zoom spin button, as we only want to
     change the value of the spin button, it's up to the caller of this
     function to change anything on the actual canvases... we'll 
     unblock at the end of this function */
  g_signal_handlers_block_by_func(G_OBJECT(ui_study->zoom_spin),
				  G_CALLBACK(ui_study_cb_zoom), ui_study);

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_study->zoom_spin), 
			    AMITK_STUDY_ZOOM(ui_study->study));

  /* and now, reconnect the signal */
  g_signal_handlers_unblock_by_func(G_OBJECT(ui_study->zoom_spin),
				    G_CALLBACK(ui_study_cb_zoom), ui_study);
  return;
}

/* updates the settings of the interpolation radio button, will not change canvas */
void ui_study_update_interpolation(ui_study_t * ui_study) {

  AmitkInterpolation i_interpolation;
  AmitkInterpolation interpolation;

  if (ui_study->active_ds == NULL) {
    for (i_interpolation = 0; i_interpolation < AMITK_INTERPOLATION_NUM; i_interpolation++)
      gtk_widget_set_sensitive(ui_study->interpolation_button[i_interpolation], FALSE);
  } else {
    for (i_interpolation = 0; i_interpolation < AMITK_INTERPOLATION_NUM; i_interpolation++)
      gtk_widget_set_sensitive(ui_study->interpolation_button[i_interpolation], TRUE);

    interpolation = AMITK_DATA_SET_INTERPOLATION(ui_study->active_ds);

    /* block signals, as we only want to change the value, it's up to the caller of this
       function to change anything on the actual canvases... 
       we'll unblock at the end of this function */
    for (i_interpolation = 0; i_interpolation < AMITK_INTERPOLATION_NUM; i_interpolation++) 
      g_signal_handlers_block_by_func(G_OBJECT(ui_study->interpolation_button[i_interpolation]),
				      G_CALLBACK(ui_study_cb_interpolation), ui_study);

    /* need the button pressed to get the display to update correctly */
    gtk_button_pressed(GTK_BUTTON(ui_study->interpolation_button[interpolation]));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_study->interpolation_button[interpolation]),
				 TRUE);

    for (i_interpolation = 0; i_interpolation < AMITK_INTERPOLATION_NUM; i_interpolation++)
      g_signal_handlers_unblock_by_func(G_OBJECT(ui_study->interpolation_button[i_interpolation]),
					G_CALLBACK(ui_study_cb_interpolation),  ui_study);
  }

}


void ui_study_update_title(ui_study_t * ui_study) {
  
  gchar * title;

  if (ui_study->study_altered) 
    title = g_strdup_printf("Study: %s *", AMITK_OBJECT_NAME(ui_study->study));
  else
    title = g_strdup_printf("Study: %s",AMITK_OBJECT_NAME(ui_study->study));
  gtk_window_set_title(GTK_WINDOW(ui_study->app), title);
  g_free(title);

}



void ui_study_setup_layout(ui_study_t * ui_study) {

  AmitkView i_view;
  gboolean canvas_first_time;
  AmitkViewMode i_view_mode;
  

  for (i_view_mode = 0; i_view_mode <= ui_study->view_mode; i_view_mode++) {

    if (ui_study->canvas_table[i_view_mode] != NULL) {
      canvas_first_time = FALSE;
      gtk_widget_hide(ui_study->canvas[i_view_mode][AMITK_VIEW_SAGITTAL]); /* hide, so we get more fluid moving */
      /* add ref so it's not destroyed when we remove it from the container */
      for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view++) {
	g_object_ref(G_OBJECT(ui_study->canvas[i_view_mode][i_view]));
	gtk_container_remove(GTK_CONTAINER(ui_study->canvas_table[i_view_mode]), 
			     ui_study->canvas[i_view_mode][i_view]);
      }
      g_object_ref(G_OBJECT(ui_study->canvas_table[i_view_mode]));
      gtk_container_remove(GTK_CONTAINER(ui_study->center_table),
			   ui_study->canvas_table[i_view_mode]);
    } else {/* new canvas */
      canvas_first_time = TRUE;
      ui_study->canvas_table[i_view_mode] = gtk_table_new(3, 2,FALSE);
      
      for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view++) {
	ui_study->canvas[i_view_mode][i_view] = 
	  amitk_canvas_new(ui_study->study, i_view, 
			   ui_study->canvas_layout,
			   ui_study->line_style,
			   ui_study->roi_width,
			   TRUE);
	
	g_signal_connect(G_OBJECT(ui_study->canvas[i_view_mode][i_view]), "help_event",
			 G_CALLBACK(ui_study_cb_canvas_help_event), ui_study);
	g_signal_connect(G_OBJECT(ui_study->canvas[i_view_mode][i_view]), "view_changing",
			 G_CALLBACK(ui_study_cb_canvas_view_changing), ui_study);
	g_signal_connect(G_OBJECT(ui_study->canvas[i_view_mode][i_view]), "view_changed",
			 G_CALLBACK(ui_study_cb_canvas_view_changed), ui_study);
	g_signal_connect(G_OBJECT(ui_study->canvas[i_view_mode][i_view]), "isocontour_3d_changed",
			 G_CALLBACK(ui_study_cb_canvas_isocontour_3d_changed), ui_study);
	g_signal_connect(G_OBJECT(ui_study->canvas[i_view_mode][i_view]), "erase_volume",
			 G_CALLBACK(ui_study_cb_canvas_erase_volume), ui_study);
	g_signal_connect(G_OBJECT(ui_study->canvas[i_view_mode][i_view]), "new_object",
			 G_CALLBACK(ui_study_cb_canvas_new_object), ui_study);
	
      }
    }


    /* put the canvases in the table according to the desired layout */
    switch(ui_study->canvas_layout) {
    case AMITK_LAYOUT_ORTHOGONAL:
      gtk_table_attach(GTK_TABLE(ui_study->canvas_table[i_view_mode]), 
		       ui_study->canvas[i_view_mode][AMITK_VIEW_TRANSVERSE], 
		       0,1,1,2, FALSE,FALSE, X_PADDING, Y_PADDING);
      gtk_table_attach(GTK_TABLE(ui_study->canvas_table[i_view_mode]), 
		       ui_study->canvas[i_view_mode][AMITK_VIEW_CORONAL], 
		       0,1,2,3, FALSE,FALSE, X_PADDING, Y_PADDING);
      gtk_table_attach(GTK_TABLE(ui_study->canvas_table[i_view_mode]), 
		       ui_study->canvas[i_view_mode][AMITK_VIEW_SAGITTAL], 
		       1,2,1,2, FALSE,FALSE, X_PADDING, Y_PADDING);
      gtk_table_attach(GTK_TABLE(ui_study->center_table), ui_study->canvas_table[i_view_mode],
		       i_view_mode, i_view_mode+1, 0,1,
		       X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL,  
		       X_PADDING, Y_PADDING);
      break;
    case AMITK_LAYOUT_LINEAR:
    default:
      for (i_view=0;i_view< AMITK_VIEW_NUM;i_view++) {
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

    for (i_view=0; i_view < AMITK_VIEW_NUM ;i_view++ ) {
      amitk_canvas_set_layout(AMITK_CANVAS(ui_study->canvas[i_view_mode][i_view]), 
			      ui_study->canvas_layout);
    }



    if (!canvas_first_time) {
      /* remove the additional reference */
      for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view++)
	g_object_unref(G_OBJECT(ui_study->canvas[i_view_mode][i_view]));
      g_object_unref(G_OBJECT(ui_study->canvas_table[i_view_mode]));
      gtk_widget_show(ui_study->canvas[i_view_mode][AMITK_VIEW_SAGITTAL]); /* and show */
    } else {
      gtk_widget_show_all(ui_study->canvas_table[i_view_mode]); /* and show */
    }
      
  }



  /* get rid of stuff we're not looking at */
  for (i_view_mode = ui_study->view_mode+1; i_view_mode < AMITK_VIEW_MODE_NUM; i_view_mode++) {
    if (ui_study->canvas_table[i_view_mode] != NULL) {
      gtk_widget_destroy(ui_study->canvas_table[i_view_mode]);
      ui_study->canvas_table[i_view_mode] = NULL;
    }
  }

  return;
}

/* function to setup the widgets inside of the GnomeApp study */
void ui_study_setup_widgets(ui_study_t * ui_study) {

  guint main_table_row=0; 
  guint main_table_column=0;
  GtkWidget * scrolled;

  /* make and add the main packing table */
  ui_study->main_table = gtk_table_new(3,2,FALSE);
  if (GNOME_IS_APP(ui_study->app)) 
    gnome_app_set_contents(GNOME_APP(ui_study->app), ui_study->main_table);
  else
    gtk_container_add(GTK_CONTAINER(ui_study->app), ui_study->main_table);

  /* connect the blank help signal */
  g_object_set_data(G_OBJECT(ui_study->app), "which_help", GINT_TO_POINTER(AMITK_HELP_INFO_BLANK));
  g_signal_connect(G_OBJECT(ui_study->app), "enter_notify_event",
		   G_CALLBACK(ui_study_cb_update_help_info), ui_study);

  ui_study->tree = amitk_tree_new();
  amitk_tree_add_object(AMITK_TREE(ui_study->tree), AMITK_OBJECT(ui_study->study), TRUE);
  g_signal_connect(G_OBJECT(ui_study->tree), "help_event",
		   G_CALLBACK(ui_study_cb_tree_help_event), ui_study);
  g_signal_connect(G_OBJECT(ui_study->tree), "select_object", 
		   G_CALLBACK(ui_study_cb_tree_select_object), ui_study);
  g_signal_connect(G_OBJECT(ui_study->tree), "unselect_object", 
		   G_CALLBACK(ui_study_cb_tree_unselect_object), ui_study);
  g_signal_connect(G_OBJECT(ui_study->tree), "make_active_object", 
		   G_CALLBACK(ui_study_cb_tree_make_active_object), ui_study);
  g_signal_connect(G_OBJECT(ui_study->tree), "popup_object", 
		   G_CALLBACK(ui_study_cb_tree_popup_object), ui_study);
  g_signal_connect(G_OBJECT(ui_study->tree), "add_object", 
		   G_CALLBACK(ui_study_cb_tree_add_object), ui_study);
  g_signal_connect(G_OBJECT(ui_study->tree), "delete_object", 
		   G_CALLBACK(ui_study_cb_tree_delete_object), ui_study);
      
  /* make a scrolled area for the tree */
  scrolled = gtk_scrolled_window_new(NULL,NULL);  
  gtk_widget_set_size_request(scrolled,250,250);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), 
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), 
					ui_study->tree);
  gtk_table_attach(GTK_TABLE(ui_study->main_table), scrolled, 
		   main_table_column, main_table_column+1, main_table_row, main_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL, 
		   X_PADDING, Y_PADDING);

  main_table_row++;




  /* the help information canvas */
  ui_study->help_info = GNOME_CANVAS(gnome_canvas_new_aa());
  gtk_table_attach(GTK_TABLE(ui_study->main_table), GTK_WIDGET(ui_study->help_info), 
		   main_table_column, main_table_column+1, main_table_row, main_table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_set_size_request(GTK_WIDGET(ui_study->help_info), 150,
			      HELP_INFO_LINE_HEIGHT*NUM_HELP_INFO_LINES);
  gnome_canvas_set_scroll_region(ui_study->help_info, 0.0, 0.0, 150.0, 
				 HELP_INFO_LINE_HEIGHT*NUM_HELP_INFO_LINES);
  main_table_column++;
  main_table_row=0;

  /* make the stuff in the center */
  ui_study->center_table = gtk_table_new(2, 2,FALSE);
  gtk_table_attach(GTK_TABLE(ui_study->main_table), ui_study->center_table,
		   main_table_column, main_table_column+1, main_table_row, main_table_row+3,
  		   X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL,  X_PADDING, Y_PADDING);
  main_table_column++;
  main_table_row=0;
  
  /* setup the trees and canvases */
  ui_study_setup_layout(ui_study);

  return;
}



/* replace what's currently in the ui_study with the specified study */
void ui_study_replace_study(ui_study_t * ui_study, AmitkStudy * study) {

  AmitkView i_view;
  AmitkViewMode i_view_mode;

  amitk_tree_remove_object(AMITK_TREE(ui_study->tree), AMITK_OBJECT(ui_study->study));

  g_object_unref(ui_study->study);
  ui_study->study = study;
  ui_study->study_virgin=FALSE;
  ui_study->study_altered=FALSE;

  amitk_tree_add_object(AMITK_TREE(ui_study->tree), AMITK_OBJECT(ui_study->study), TRUE);
  for (i_view_mode=0; i_view_mode <= ui_study->view_mode; i_view_mode++) {
    for (i_view=0; i_view< AMITK_VIEW_NUM; i_view++)
      amitk_canvas_add_object(AMITK_CANVAS(ui_study->canvas[i_view_mode][i_view]), 
  			      AMITK_OBJECT(ui_study->study));
  }

  /* set any settings we can */
  ui_study_update_thickness(ui_study, AMITK_STUDY_VIEW_THICKNESS(ui_study->study));
  ui_study_update_zoom(ui_study);
  ui_study_update_title(ui_study);

  g_signal_connect(G_OBJECT(ui_study->study), "thickness_changed", 
		   G_CALLBACK(ui_study_cb_study_changed), ui_study);

}



/* procedure to set up the study window */
GtkWidget * ui_study_create(AmitkStudy * study) {

  ui_study_t * ui_study;
  gchar * temp_string;


  ui_study = ui_study_init();

  /* setup the study window */
  if (study == NULL) {
    ui_study->study = amitk_study_new();
    temp_string = g_strdup_printf("temp_%d",next_study_num++);
    amitk_object_set_name(AMITK_OBJECT(ui_study->study), temp_string);
    g_free(temp_string);
  } else {
    ui_study->study = g_object_ref(study);
    ui_study->study_virgin=FALSE;
  }

  g_signal_connect(G_OBJECT(ui_study->study), "thickness_changed", 
		   G_CALLBACK(ui_study_cb_study_changed), ui_study);
  ui_study->app=gnome_app_new(PACKAGE, NULL);
  ui_study_update_title(ui_study);

  //  gtk_window_set_policy (GTK_WINDOW(ui_study->app), TRUE, TRUE, TRUE);

  /* disable user resizability, this allows the window to autoshrink */
  gtk_window_set_resizable(GTK_WINDOW(ui_study->app), FALSE);

  g_signal_connect(G_OBJECT(ui_study->app), "realize", 
		   G_CALLBACK(ui_common_window_realize_cb), NULL);
  g_object_set_data(G_OBJECT(ui_study->app), "ui_study", ui_study);

  /* setup the callbacks for app */
  g_signal_connect(G_OBJECT(ui_study->app), "delete_event",
		   G_CALLBACK(ui_study_cb_delete_event),
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
  ui_study_update_thickness(ui_study, AMITK_STUDY_VIEW_THICKNESS(ui_study->study));
  ui_study_update_zoom(ui_study);

  return ui_study->app;
}
