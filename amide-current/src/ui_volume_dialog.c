/* ui_volume_dialog.c
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
#include "study.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_roi.h"
#include "ui_volume.h"
#include "ui_study.h"
#include "ui_volume_dialog_callbacks.h"
#include "ui_volume_dialog.h"
#include "ui_threshold2.h"



/* function to set that volume axis display widgets */
void ui_volume_dialog_set_axis_display(GtkWidget * volume_dialog) {

  GtkWidget * label;
  GtkWidget * entry;
  gchar * temp_string;
  volume_t * volume_new_info;
  axis_t i_axis;

  volume_new_info = gtk_object_get_data(GTK_OBJECT(volume_dialog), "volume_new_info");

  for (i_axis=0;i_axis<NUM_AXIS;i_axis++) {
    label = gtk_object_get_data(GTK_OBJECT(volume_dialog), axis_names[i_axis]);

    entry = gtk_object_get_data(GTK_OBJECT(label), axis_names[XAXIS]);
    temp_string = g_strdup_printf("%f", volume_new_info->coord_frame.axis[i_axis].x);
    gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
    g_free(temp_string);

    entry = gtk_object_get_data(GTK_OBJECT(label), axis_names[YAXIS]);
    temp_string = g_strdup_printf("%f", volume_new_info->coord_frame.axis[i_axis].y);
    gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
    g_free(temp_string);

    entry = gtk_object_get_data(GTK_OBJECT(label), axis_names[ZAXIS]);
    temp_string = g_strdup_printf("%f", volume_new_info->coord_frame.axis[i_axis].z);
    gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
    g_free(temp_string);

  }

  return;
}




/* function that sets up the volume dialog */
void ui_volume_dialog_create(ui_study_t * ui_study, volume_t * volume) {
  
  GtkWidget * volume_dialog;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * frames_table;
  GtkWidget * label;
  GtkWidget * option_menu;
  GtkWidget * menu;
  GtkWidget * menuitem;
  GtkWidget * entry;
  GtkObject * adjustment;
  GtkWidget * dial;
  GtkWidget * button;
  GtkWidget * check_button;
  GtkWidget * threshold_widget;
  GtkWidget * scrolled;
  GtkWidget * hseparator;
  GtkWidget * vseparator;
  guint table_row = 0;
  modality_t i_modality;
  view_t i_view;
  axis_t i_axis, j_axis;
  guint i;
  ui_volume_list_t * ui_volume_list_item;
  volume_t * volume_new_info;
  realpoint_t center;
  ui_threshold_t * volume_threshold;

  /* figure out the ui_study_volume_list item corresponding to this volume */
  ui_volume_list_item = ui_volume_list_get_ui_volume(ui_study->current_volumes, volume);
  if (ui_volume_list_item == NULL) return;

  /* only want one of these dialogs at a time for a given volume */
  if (ui_volume_list_item->dialog != NULL)
    return;

  /* and if the threshold dialog corresponds to this volume, kill it */
  if (ui_study->threshold != NULL)
    if (ui_study->threshold->volume == volume)
      if (ui_study->threshold->app != NULL)
	gtk_signal_emit_by_name(GTK_OBJECT(ui_study->threshold->app), "delete_event", 
				NULL, ui_study);

  
  /* sanity checks */
  g_return_if_fail(ui_volume_list_item != NULL);
  g_return_if_fail(ui_volume_list_item->tree_leaf != NULL);
    
  temp_string = g_strdup_printf("%s: Data Set Modification Dialog",PACKAGE);
  volume_dialog = gnome_property_box_new();
  gtk_window_set_title(GTK_WINDOW(volume_dialog), temp_string);
  g_free(temp_string);

  /* create the temp volume which will store the new information, and then
     can either be applied or cancelled */
  volume_new_info = volume_copy(volume);

  /* attach it to the dialog */
  gtk_object_set_data(GTK_OBJECT(volume_dialog), "volume_new_info", volume_new_info);

  /* save a pointer to ui_study with the dialog widget so we can redraw volumes's */ 
  gtk_object_set_data(GTK_OBJECT(volume_dialog), "ui_study", ui_study);

  
  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(volume_dialog), "close",
		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_close_event),
		     ui_volume_list_item);
  gtk_signal_connect(GTK_OBJECT(volume_dialog), "apply",
		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_apply),
		     ui_volume_list_item);
  gtk_signal_connect(GTK_OBJECT(volume_dialog), "help",
    		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_help),
    		     ui_volume_list_item);

  /* start making the widgets for this dialog box */

  /* ----------------------------------------
     Basic Info Page:
     ---------------------------------------- */

  packing_table = gtk_table_new(4,2,FALSE);
  label = gtk_label_new("Basic Info");
  table_row=0;
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(volume_dialog), GTK_WIDGET(packing_table), label);
  ui_volume_list_item->dialog = volume_dialog; /* save a pointer to the dialog */

  /* widgets to change the volume's name */
  label = gtk_label_new("name:");
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(label), 0,1,
		   table_row, table_row+1,
		   0, 0,
		   X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), volume_new_info->name);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "volume_dialog", volume_dialog); 
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_name), 
		     volume_new_info);
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(entry),1,2,
		   table_row, table_row+1,
		   GTK_FILL, 0,
		   X_PADDING, Y_PADDING);
 
  table_row++;



  /* widgets to change the object's modality */
  label = gtk_label_new("modality:");
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(label), 0,1,
		   table_row, table_row+1,
		   0, 0,
		   X_PADDING, Y_PADDING);

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();

  for (i_modality=0; i_modality<NUM_MODALITIES; i_modality++) {
    menuitem = gtk_menu_item_new_with_label(modality_names[i_modality]);
    gtk_menu_append(GTK_MENU(menu), menuitem);
    gtk_object_set_data(GTK_OBJECT(menuitem), "modality", GINT_TO_POINTER(i_modality)); 
    gtk_object_set_data(GTK_OBJECT(menuitem), "volume_dialog", volume_dialog); 
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
     		       GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_modality), 
    		       volume_new_info);
  }
  
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), volume->modality);
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(option_menu), 1,2, 
		   table_row,table_row+1,
		   GTK_FILL, 0, 
		   X_PADDING, Y_PADDING);
  table_row++;

  /* widget to change the conversion scale factor */
  label = gtk_label_new("Conversion Factor");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", volume->conversion);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "volume_dialog", volume_dialog); 
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(CONVERSION_FACTOR));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_entry), 
		     volume_new_info);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;



  /* ----------------------------------------
     Dimensions info page
     ---------------------------------------- */

  /* the next page of options */
  packing_table = gtk_table_new(5,5,FALSE);
  table_row=0;
  label = gtk_label_new("Center/Dimensions");
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(volume_dialog), GTK_WIDGET(packing_table), label);

  /* widgets to change the location of the volume's center in real space and dimensions*/
  label = gtk_label_new("Center (mm from origin)");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 1,2,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  center = volume_calculate_center(volume);

  /* a separator for clarity */
  vseparator = gtk_vseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(vseparator),2,3,
		   0,5, 0, GTK_FILL, X_PADDING, Y_PADDING);

  label = gtk_label_new("Voxel Size (mm)");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 4,5,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  table_row++;

  /**************/
  label = gtk_label_new("x");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", center.x);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "volume_dialog", volume_dialog); 
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(CENTER_X));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_entry), 
		     volume_new_info);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  label = gtk_label_new("x");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 3,4,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", volume->voxel_size.x);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "volume_dialog", volume_dialog); 
  gtk_object_set_data(GTK_OBJECT(volume_dialog), "voxel_size_x", entry); 
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(VOXEL_SIZE_X));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_entry), 
		     volume_new_info);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),4,5,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  table_row++;

  /*************/
  label = gtk_label_new("y");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", center.y);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "volume_dialog", volume_dialog); 
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(CENTER_Y));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_entry), 
		     volume_new_info);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  label = gtk_label_new("y");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 3,4,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", volume->voxel_size.y);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "volume_dialog", volume_dialog); 
  gtk_object_set_data(GTK_OBJECT(volume_dialog), "voxel_size_y", entry); 
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(VOXEL_SIZE_Y));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_entry), 
		     volume_new_info);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),4,5,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  table_row++;

  /*************/
  label = gtk_label_new("z");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", center.z);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "volume_dialog", volume_dialog); 
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(CENTER_Z));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_entry), 
		     volume_new_info);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  label = gtk_label_new("z");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 3,4,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", volume->voxel_size.z);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "volume_dialog", volume_dialog); 
  gtk_object_set_data(GTK_OBJECT(volume_dialog), "voxel_size_z", entry); 
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(VOXEL_SIZE_Z));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_entry), 
		     volume_new_info);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),4,5,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  table_row++;


  /*********/

  check_button = gtk_check_button_new_with_label ("keep aspect ratio");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), TRUE);
  gtk_object_set_data(GTK_OBJECT(volume_dialog), "aspect_ratio", GINT_TO_POINTER(TRUE));
  gtk_object_set_data(GTK_OBJECT(check_button), "volume_dialog", volume_dialog); 
  gtk_signal_connect(GTK_OBJECT(check_button), "toggled", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_aspect_ratio), 
		     NULL);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(check_button),4,5,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  table_row++;





  /* ----------------------------------------
     Rotations page
     ---------------------------------------- */


  /* start making the page to rotate the volume */
  packing_table = gtk_table_new(9,4,FALSE);
  label = gtk_label_new("Rotate");
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(volume_dialog), GTK_WIDGET(packing_table), label);
  table_row=0;


  /* draw the x/y/z labels */
  label = gtk_label_new("transverse");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  table_row++;
  label = gtk_label_new("coronal");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  table_row++;
  label = gtk_label_new("sagittal");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  
  
  table_row=0;
  /* make the three canvases, scales, dials, etc. */
  for (i_view=0;i_view<NUM_VIEWS;i_view++) {

    /* dial section */
    adjustment = gtk_adjustment_new(0,-45.0,45.5,0.5,0.5,0.5);
    /*so we can figure out which adjustment this is in callbacks */
    gtk_object_set_data(GTK_OBJECT(adjustment), "view", GINT_TO_POINTER(i_view));
    gtk_object_set_data(GTK_OBJECT(adjustment), "volume_dialog", volume_dialog); 
    gtk_object_set_data(GTK_OBJECT(adjustment), "ui_study", ui_study);
    dial = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
    gtk_range_set_update_policy(GTK_RANGE(dial), GTK_UPDATE_DISCONTINUOUS);
    gtk_table_attach(GTK_TABLE(packing_table), 
		     GTK_WIDGET(dial),
		     1,4, table_row, table_row+1,
		     X_PACKING_OPTIONS | GTK_FILL,FALSE, X_PADDING, Y_PADDING);
    gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed", 
		       GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_axis), 
		       volume_new_info);
    table_row++;
  }



  /* button to reset the axis */
  label = gtk_label_new("data set axis:");
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(label), 0,1, 
		   table_row, table_row+1,
		   0, 0, X_PADDING, Y_PADDING);
  
  button = gtk_button_new_with_label("reset to default");
  gtk_object_set_data(GTK_OBJECT(button), "volume_dialog", volume_dialog); 
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(button), 1,2, 
		   table_row, table_row+1,
		   0, 0, X_PADDING, Y_PADDING);
  gtk_signal_connect(GTK_OBJECT(button), "pressed",
		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_reset_axis), 
		     volume_new_info);
  table_row++;

  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(hseparator),
		   table_row, table_row+1,0,4, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  /* and a display of the current axis */
  label = gtk_label_new("i");
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(label), 1,2,
		   table_row, table_row+1,
		   0, 0, X_PADDING, Y_PADDING);

  label = gtk_label_new("j");
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(label), 2,3,
		   table_row, table_row+1,
		   0, 0, X_PADDING, Y_PADDING);

  label = gtk_label_new("k");
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(label), 3,4,
		   table_row, table_row+1,
		   0, 0, X_PADDING, Y_PADDING);
  table_row++;


  for (i_axis=0;i_axis<NUM_AXIS;i_axis++) {

    /* the row label */
    label = gtk_label_new(axis_names[i_axis]);
    gtk_object_set_data(GTK_OBJECT(volume_dialog), axis_names[i_axis], label);
    gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1, 
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    for (j_axis=0;j_axis<NUM_AXIS;j_axis++) {

      entry = gtk_entry_new();
      gtk_widget_set_usize(GTK_WIDGET(entry), UI_STUDY_DEFAULT_ENTRY_WIDTH,0);
      gtk_object_set_data(GTK_OBJECT(label), axis_names[j_axis], entry);
      gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
      gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),j_axis+1, j_axis+2,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    }

    table_row++;
  }

  ui_volume_dialog_set_axis_display(volume_dialog);


  /* ----------------------------------------
     Colormap/threshold page
     ---------------------------------------- */


  /* start making the page to change the threshold/colormap */
  packing_table = gtk_table_new(1,1,FALSE);
  label = gtk_label_new("Colormap/Threshold");
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(volume_dialog), GTK_WIDGET(packing_table), label);
  table_row=0;


  volume_threshold = ui_threshold_init();
  volume_threshold->volume = volume;
  threshold_widget = ui_threshold_create(ui_study, volume_threshold);
  ui_volume_list_item->threshold = volume_threshold;
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(threshold_widget), 0,1,
		   table_row, table_row+1,
		   0, 0, X_PADDING, Y_PADDING);
  table_row++;
  

  /* ----------------------------------------
     Time page
     ---------------------------------------- */

  /* start making the page to adjust time values */
  packing_table = gtk_table_new(4,4,FALSE);
  label = gtk_label_new("Time");
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(volume_dialog), GTK_WIDGET(packing_table), label);
  table_row=0;


  /* scan start time..... */
  label = gtk_label_new("Scan Start Time (s)");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", volume->scan_start);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "volume_dialog", volume_dialog); 
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(SCAN_START));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_entry), 
		     volume_new_info);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(hseparator),0,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;



  /* frame duration(s).... */
  label = gtk_label_new("Frame");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  label = gtk_label_new("Duration (s)");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 1,2,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  table_row++;




  /* make a scrolled area for the info */
  scrolled = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				 GTK_POLICY_NEVER,
				 GTK_POLICY_AUTOMATIC);
  frames_table = gtk_table_new(2,2,TRUE);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), frames_table);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(scrolled), 0,2,
		   table_row, table_row+1, 0, GTK_FILL|GTK_EXPAND, X_PADDING, Y_PADDING);
		    

  /* iterate throught the frames */
  for (i=0; i<volume->num_frames; i++) {

    /* this frame's label */
    temp_string = g_strdup_printf("%d", i);
    label = gtk_label_new(temp_string);
    g_free(temp_string);
    gtk_table_attach(GTK_TABLE(frames_table), GTK_WIDGET(label), 0,1,
		   i, i+1, 0, 0, X_PADDING, Y_PADDING);

    /* and this frame's entry */
    entry = gtk_entry_new();
    temp_string = g_strdup_printf("%f", volume->frame_duration[i]);
    gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
    g_free(temp_string);
    gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
    gtk_object_set_data(GTK_OBJECT(entry), "volume_dialog", volume_dialog); 
    gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(FRAME_DURATION));
    gtk_object_set_data(GTK_OBJECT(entry), "frame", GINT_TO_POINTER(i));
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		       GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_entry), 
		       volume_new_info);
    gtk_table_attach(GTK_TABLE(frames_table), GTK_WIDGET(entry),1,2,
		   i, i+1, 0, 0, X_PADDING, Y_PADDING);

  }

  table_row++;

  /* and show all our widgets */
  gtk_widget_show_all(volume_dialog);

  return;
}











