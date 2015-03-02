/* ui_volume_dialog.c
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
#include "amitk_coord_frame_edit.h"
#include "amitk_threshold.h" 
#include "ui_study.h"
#include "ui_volume_dialog_cb.h"
#include "ui_volume_dialog.h"
#include "ui_study_cb.h"
#include "ui_common.h"



void ui_volume_dialog_update_entries(GtkWidget * dialog, realpoint_t new_center) {

  GtkWidget * entry;
  gchar * temp_string;

  entry = gtk_object_get_data(GTK_OBJECT(dialog), "center_x");
  temp_string = g_strdup_printf("%5.3f", new_center.x);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);

  entry = gtk_object_get_data(GTK_OBJECT(dialog), "center_y");
  temp_string = g_strdup_printf("%5.3f", new_center.y);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);

  entry = gtk_object_get_data(GTK_OBJECT(dialog), "center_z");
  temp_string = g_strdup_printf("%5.3f", new_center.z);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  
  return;
}

/* function that sets up the volume dialog */
GtkWidget * ui_volume_dialog_create(ui_study_t * ui_study, volume_t * volume) {
  
  GtkWidget * dialog;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * frames_table;
  GtkWidget * label;
  GtkWidget * option_menu;
  GtkWidget * menu;
  GtkWidget * menuitem;
  GtkWidget * entry;
  GtkWidget * check_button;
  GtkWidget * scrolled;
  GtkWidget * hseparator;
  GtkWidget * vseparator;
  GtkWidget * axis_indicator;
  GtkWidget * cfe;
  GtkWidget * threshold;
  guint table_row = 0;
  modality_t i_modality;
  guint i;
  volume_t * new_info;
  realpoint_t center;

  temp_string = g_strdup_printf("%s: Medical Data Set Modification Dialog",PACKAGE);
  dialog = gnome_property_box_new();
  gtk_window_set_title(GTK_WINDOW(dialog), temp_string);
  g_free(temp_string);

  /* create the temp volume which will store the new information, and then
     can either be applied or cancelled */
  new_info = volume_copy(volume);

  /* attach the data to the dialog */
  gtk_object_set_data(GTK_OBJECT(dialog), "new_info", new_info);
  gtk_object_set_data(GTK_OBJECT(dialog), "old_info", volume);

  /* save a pointer to ui_study with the dialog widget so we can redraw volumes's */ 
  gtk_object_set_data(GTK_OBJECT(dialog), "ui_study", ui_study);

  
  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(dialog), "close",
		     GTK_SIGNAL_FUNC(ui_volume_dialog_cb_close), ui_study);
  gtk_signal_connect(GTK_OBJECT(dialog), "apply",
		     GTK_SIGNAL_FUNC(ui_volume_dialog_cb_apply), ui_study);
  gtk_signal_connect(GTK_OBJECT(dialog), "help",
    		     GTK_SIGNAL_FUNC(ui_volume_dialog_cb_help), NULL);

  /* start making the widgets for this dialog box */

  /* ----------------------------------------
     Basic Info Page:
     ---------------------------------------- */

  packing_table = gtk_table_new(4,2,FALSE);
  label = gtk_label_new("Basic Info");
  table_row=0;
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(dialog), packing_table, label);

  /* widgets to change the volume's name */
  label = gtk_label_new("name:");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), new_info->name);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_cb_change_name), dialog);
  gtk_table_attach(GTK_TABLE(packing_table), entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
 
  table_row++;



  /* widgets to change the date of the scan name */
  label = gtk_label_new("scan date:");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), new_info->scan_date);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_cb_change_scan_date), dialog);
  gtk_table_attach(GTK_TABLE(packing_table), entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
 
  table_row++;



  /* widgets to change the object's modality */
  label = gtk_label_new("modality:");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();

  for (i_modality=0; i_modality<NUM_MODALITIES; i_modality++) {
    menuitem = gtk_menu_item_new_with_label(modality_names[i_modality]);
    gtk_menu_append(GTK_MENU(menu), menuitem);
    gtk_object_set_data(GTK_OBJECT(menuitem), "modality", GINT_TO_POINTER(i_modality)); 
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
     		       GTK_SIGNAL_FUNC(ui_volume_dialog_cb_change_modality), dialog);
  }
  
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), volume->modality);
  gtk_table_attach(GTK_TABLE(packing_table), option_menu, 1,2, 
		   table_row,table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* widget to change the scaling factor */
  label = gtk_label_new("Scaling Factor:");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", volume->external_scaling);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(SCALING_FACTOR));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_cb_change_entry), dialog);
  gtk_table_attach(GTK_TABLE(packing_table), entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  /* ----------------------------------------
     Dimensions info page
     ---------------------------------------- */

  /* the next page of options */
  packing_table = gtk_table_new(5,5,FALSE);
  table_row=0;
  label = gtk_label_new("Center/Dimensions");
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(dialog), packing_table, label);

  /* widgets to change the location of the volume's center in real space and dimensions*/
  label = gtk_label_new("Center (mm from view origin)");
  gtk_table_attach(GTK_TABLE(packing_table), label, 1,2,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  center = realspace_base_coord_to_alt(volume_center(volume), 
				       study_coord_frame(ui_study->study));

  /* a separator for clarity */
  vseparator = gtk_vseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), vseparator,2,3,
		   0,5, 0, GTK_FILL, X_PADDING, Y_PADDING);

  label = gtk_label_new("Voxel Size (mm)");
  gtk_table_attach(GTK_TABLE(packing_table), label, 4,5,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  table_row++;

  /**************/
  label = gtk_label_new("x");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", center.x);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(CENTER_X));
  gtk_object_set_data(GTK_OBJECT(dialog), "center_x", entry);
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_cb_change_entry), dialog);
  gtk_table_attach(GTK_TABLE(packing_table), entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  label = gtk_label_new("x");
  gtk_table_attach(GTK_TABLE(packing_table), label, 3,4,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", volume->voxel_size.x);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(dialog), "voxel_size_x", entry); 
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(VOXEL_SIZE_X));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_cb_change_entry), dialog);
  gtk_table_attach(GTK_TABLE(packing_table), entry,4,5,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  table_row++;

  /*************/
  label = gtk_label_new("y");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", center.y);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(CENTER_Y));
  gtk_object_set_data(GTK_OBJECT(dialog), "center_y", entry);
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_cb_change_entry), dialog);
  gtk_table_attach(GTK_TABLE(packing_table), entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  label = gtk_label_new("y");
  gtk_table_attach(GTK_TABLE(packing_table), label, 3,4,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", volume->voxel_size.y);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(dialog), "voxel_size_y", entry); 
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(VOXEL_SIZE_Y));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_cb_change_entry), dialog);
  gtk_table_attach(GTK_TABLE(packing_table), entry,4,5,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  table_row++;

  /*************/
  label = gtk_label_new("z");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", center.z);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(CENTER_Z));
  gtk_object_set_data(GTK_OBJECT(dialog), "center_z", entry);
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_cb_change_entry), dialog);
  gtk_table_attach(GTK_TABLE(packing_table), entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  label = gtk_label_new("z");
  gtk_table_attach(GTK_TABLE(packing_table), label, 3,4,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", volume->voxel_size.z);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(dialog), "voxel_size_z", entry); 
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(VOXEL_SIZE_Z));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_cb_change_entry), dialog);
  gtk_table_attach(GTK_TABLE(packing_table), entry,4,5,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  table_row++;


  /*********/

  check_button = gtk_check_button_new_with_label ("keep aspect ratio");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), TRUE);
  gtk_object_set_data(GTK_OBJECT(dialog), "aspect_ratio", GINT_TO_POINTER(TRUE));
  gtk_signal_connect(GTK_OBJECT(check_button), "toggled", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_cb_aspect_ratio), dialog);
  gtk_table_attach(GTK_TABLE(packing_table), check_button,4,5,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  table_row++;

  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator, 0, 5, table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* a canvas to indicate which way is x, y, and z */
  axis_indicator = ui_common_create_view_axis_indicator(ui_study->canvas_layout);
  gtk_table_attach(GTK_TABLE(packing_table), axis_indicator,0,5,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  table_row++;


  /* ----------------------------------------
     Rotations page
     ---------------------------------------- */
  label = gtk_label_new("Rotate");
  cfe = amitk_coord_frame_edit_new(new_info->coord_frame, 
				   volume_center(new_info),
				   study_coord_frame(ui_study->study));
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(dialog), cfe, label);
 
  gtk_object_set_data(GTK_OBJECT(dialog), "cfe", cfe);
  gtk_signal_connect(GTK_OBJECT(cfe), "coord_frame_changed",
  		     GTK_SIGNAL_FUNC(ui_volume_dialog_cb_coord_frame_changed),
  		     dialog);


  /* ----------------------------------------
     Colormap/threshold page
     ---------------------------------------- */

  label = gtk_label_new("Colormap/Threshold");
  threshold = amitk_threshold_new(volume, AMITK_THRESHOLD_BOX_LAYOUT);
  gtk_object_set_data(GTK_OBJECT(dialog), "threshold", threshold);
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(dialog), threshold, label);

  gtk_signal_connect(GTK_OBJECT(threshold), "threshold_changed",
		     GTK_SIGNAL_FUNC(ui_study_cb_threshold_changed),
		     ui_study);
  gtk_signal_connect(GTK_OBJECT(threshold), "color_changed",
		     GTK_SIGNAL_FUNC(ui_study_cb_color_changed),
		     ui_study);

  /* ----------------------------------------
     Time page
     ---------------------------------------- */

  /* start making the page to adjust time values */
  packing_table = gtk_table_new(4,4,FALSE);
  label = gtk_label_new("Time");
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(dialog), packing_table, label);
  table_row=0;


  /* scan start time..... */
  label = gtk_label_new("Scan Start Time (s)");
  gtk_table_attach(GTK_TABLE(packing_table),label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", volume->scan_start);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(SCAN_START));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_cb_change_entry), dialog);
  gtk_table_attach(GTK_TABLE(packing_table), entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator,0,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;



  /* frame duration(s).... */
  label = gtk_label_new("Frame");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  label = gtk_label_new("Duration (s)");
  gtk_table_attach(GTK_TABLE(packing_table), label, 1,2,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  table_row++;




  /* make a scrolled area for the info */
  scrolled = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				 GTK_POLICY_NEVER,
				 GTK_POLICY_AUTOMATIC);
  frames_table = gtk_table_new(2,2,TRUE);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), frames_table);
  gtk_table_attach(GTK_TABLE(packing_table), scrolled, 0,2,
		   table_row, table_row+1, 0, GTK_FILL|GTK_EXPAND, X_PADDING, Y_PADDING);
		    

  /* iterate throught the frames */
  for (i=0; i<volume->data_set->dim.t; i++) {

    /* this frame's label */
    temp_string = g_strdup_printf("%d", i);
    label = gtk_label_new(temp_string);
    g_free(temp_string);
    gtk_table_attach(GTK_TABLE(frames_table), label, 0,1,
		   i, i+1, 0, 0, X_PADDING, Y_PADDING);

    /* and this frame's entry */
    entry = gtk_entry_new();
    temp_string = g_strdup_printf("%f", volume->frame_duration[i]);
    gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
    g_free(temp_string);
    gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
    gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(FRAME_DURATION));
    gtk_object_set_data(GTK_OBJECT(entry), "frame", GINT_TO_POINTER(i));
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		       GTK_SIGNAL_FUNC(ui_volume_dialog_cb_change_entry), dialog);
    gtk_table_attach(GTK_TABLE(frames_table), entry,1,2,
		   i, i+1, 0, 0, X_PADDING, Y_PADDING);

  }

  table_row++;


  /* ----------------------------------------
     Immutables Page:
     ---------------------------------------- */

  packing_table = gtk_table_new(4,2,FALSE);
  label = gtk_label_new("Immutables");
  table_row=0;
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(dialog), packing_table, label);


  /* widget to tell you the internal data format */
  label = gtk_label_new("Data Format:");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), data_format_names[volume->data_set->format]);
  gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
  gtk_table_attach(GTK_TABLE(packing_table), entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator,0,2,
		   table_row, table_row+1, GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
  table_row++;

  /* widgets to display the data set dimensions */
  label = gtk_label_new("Data Set Dimensions (voxels)");
  gtk_table_attach(GTK_TABLE(packing_table), label, 1,2,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  table_row++;

  /**************/
  label = gtk_label_new("x");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%d", volume->data_set->dim.x);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
  gtk_table_attach(GTK_TABLE(packing_table), entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  table_row++;

  /**************/
  label = gtk_label_new("y");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%d", volume->data_set->dim.y);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
  gtk_table_attach(GTK_TABLE(packing_table), entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  table_row++;

  /**************/
  label = gtk_label_new("z");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%d", volume->data_set->dim.z);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
  gtk_table_attach(GTK_TABLE(packing_table), entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  table_row++;

  /**************/
  label = gtk_label_new("t");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%d", volume->data_set->dim.t);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
  gtk_table_attach(GTK_TABLE(packing_table), entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  table_row++;



  /* and show our dialog */
  gtk_widget_show_all(dialog);

  return dialog;
}


void ui_volume_dialog_update(GtkWidget * dialog, volume_t * volume) {

  volume_t * new_info;
  ui_study_t * ui_study;
  realpoint_t center;
  GtkWidget * cfe;

  cfe = gtk_object_get_data(GTK_OBJECT(dialog), "cfe");
  ui_study = gtk_object_get_data(GTK_OBJECT(dialog), "ui_study");
  new_info = gtk_object_get_data(GTK_OBJECT(dialog), "new_info");

  amitk_coord_frame_edit_change_coords(AMITK_COORD_FRAME_EDIT(cfe), volume->coord_frame);
  amitk_coord_frame_edit_change_center(AMITK_COORD_FRAME_EDIT(cfe), volume_center(new_info));

  center = realspace_base_coord_to_alt(volume_center(volume), 
				       study_coord_frame(ui_study->study));
  ui_volume_dialog_update_entries(dialog, center);

  return;
}









