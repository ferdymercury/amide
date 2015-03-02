/* ui_roi_dialog.c
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
#include "ui_study.h"
#include "ui_roi_dialog_cb.h"
#include "ui_roi_dialog.h"
#include "ui_common.h"



/* function to set that roi axis display widgets */
void ui_roi_dialog_set_axis_display(GtkWidget * roi_dialog) {

  GtkWidget * label;
  GtkWidget * entry;
  gchar * temp_string;
  roi_t * roi_new_info;
  axis_t i_axis;
  realpoint_t one_axis;

  roi_new_info = gtk_object_get_data(GTK_OBJECT(roi_dialog), "roi_new_info");
  

  for (i_axis=0;i_axis<NUM_AXIS;i_axis++) {
    one_axis = rs_specific_axis(roi_new_info->coord_frame, i_axis);

    label = gtk_object_get_data(GTK_OBJECT(roi_dialog), axis_names[i_axis]);

    entry = gtk_object_get_data(GTK_OBJECT(label), axis_names[XAXIS]);
    temp_string = g_strdup_printf("%f", one_axis.x);
    gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
    g_free(temp_string);

    entry = gtk_object_get_data(GTK_OBJECT(label), axis_names[YAXIS]);
    temp_string = g_strdup_printf("%f", one_axis.y);
    gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
    g_free(temp_string);

    entry = gtk_object_get_data(GTK_OBJECT(label), axis_names[ZAXIS]);
    temp_string = g_strdup_printf("%f", one_axis.z);
    gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
    g_free(temp_string);

  }

  return;
}





/* function that sets up the roi dialog */
void ui_roi_dialog_create(ui_study_t * ui_study, roi_t * roi) {
  
  GtkWidget * roi_dialog;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * label;
  GtkWidget * option_menu;
  GtkWidget * menu;
  GtkWidget * menuitem;
  GtkWidget * entry;
  GtkWidget * dial;
  GtkWidget * hseparator;
  GtkWidget * button;
  GtkWidget * axis_indicator;
  GtkObject * adjustment;
  view_t i_view;
  axis_t i_axis, j_axis;
  roi_type_t i_roi_type;
  guint table_row = 0;
  ui_roi_list_t * ui_roi_list_item;
  roi_t * roi_new_info = NULL;
  realpoint_t center;
  roi_type_t type_start, type_end;

  /* figure out the ui_study_roi_list item corresponding to this roi */
  ui_roi_list_item = ui_roi_list_get_ui_roi(ui_study->current_rois, roi);

  /* sanity checks */
  g_return_if_fail(ui_roi_list_item != NULL);
  g_return_if_fail(ui_roi_list_item->tree_leaf != NULL);
    
  /* only want one of these dialogs at a time for a given roi */
  if (ui_roi_list_item->dialog != NULL) return;
  
  temp_string = g_strdup_printf("%s: ROI Modification Dialog",PACKAGE);
  roi_dialog = gnome_property_box_new();
  gtk_window_set_title(GTK_WINDOW(roi_dialog), temp_string);
  g_free(temp_string);
  ui_roi_list_item->dialog = roi_dialog; /* save a pointer to the dialog */

  /* create the temp roi which will store the new information, and then
     can either be applied or cancelled */
  roi_new_info = roi_copy(roi);

  /* attach it to the dialog */
  gtk_object_set_data(GTK_OBJECT(roi_dialog), "roi_new_info", roi_new_info);

  /* save a pointer to ui_study with the dialog widget so we can redraw roi's */ 
  gtk_object_set_data(GTK_OBJECT(roi_dialog), "ui_study", ui_study);


  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(roi_dialog), "close",
		     GTK_SIGNAL_FUNC(ui_roi_dialog_cb_close),
		     ui_roi_list_item);
  gtk_signal_connect(GTK_OBJECT(roi_dialog), "apply",
		     GTK_SIGNAL_FUNC(ui_roi_dialog_cb_apply),
		     ui_roi_list_item);
  gtk_signal_connect(GTK_OBJECT(roi_dialog), "help",
		     GTK_SIGNAL_FUNC(ui_roi_dialog_cb_help),
		     ui_roi_list_item);




  /* ---------------------------
     Basic info page 
     --------------------------- */


  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(4,2,FALSE);
  label = gtk_label_new("Basic Info");
  table_row=0;
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(roi_dialog), GTK_WIDGET(packing_table), label);

  /* widgets to change the roi's name */
  label = gtk_label_new("name:");
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(label), 0,1,
		   table_row, table_row+1,
		   0, 0,
		   X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), roi_new_info->name);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "roi_dialog", roi_dialog); 
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_roi_dialog_cb_change_name), 
		     roi_new_info);
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(entry),1,2,
		   table_row, table_row+1,
		   GTK_FILL, 0,
		   X_PADDING, Y_PADDING);
 
  table_row++;



  /* widgets to change the object's type */
  label = gtk_label_new("type:");
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(label), 0,1,
		   table_row, table_row+1,
		   0, 0,
		   X_PADDING, Y_PADDING);

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();


  switch(roi->type) {
  case ELLIPSOID:
  case CYLINDER:
  case BOX:
    type_start = 0;
    type_end = BOX;
    break;
  case ISOCONTOUR_2D:
  case ISOCONTOUR_3D:
  default:
    type_start = type_end = roi->type;
    break;
  }

  for (i_roi_type=type_start; i_roi_type<=type_end; i_roi_type++) {
    menuitem = gtk_menu_item_new_with_label(roi_type_names[i_roi_type]);
    gtk_menu_append(GTK_MENU(menu), menuitem);
    gtk_object_set_data(GTK_OBJECT(menuitem), "roi_type", GINT_TO_POINTER(i_roi_type)); 
  gtk_object_set_data(GTK_OBJECT(menuitem), "roi_dialog", roi_dialog); 
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
     		       GTK_SIGNAL_FUNC(ui_roi_dialog_cb_change_type), 
    		       roi_new_info);
  }
  
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), roi->type);
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(option_menu), 1,2, 
		   table_row,table_row+1,
		   GTK_FILL, 0, 
		   X_PADDING, Y_PADDING);
  table_row++;

  if ((roi->type == ISOCONTOUR_2D) || (roi->type == ISOCONTOUR_3D)) {
    label = gtk_label_new("isocontour value");
    gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    entry = gtk_entry_new();
    temp_string = g_strdup_printf("%f", roi->isocontour_value);
    gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
    g_free(temp_string);
    gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
    gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    table_row++;
  }

  /* ---------------------------
     Center adjustment page
     --------------------------- */


  /* the next page of options */
  packing_table = gtk_table_new(4,2,FALSE);
  table_row=0;
  label = gtk_label_new("Center");
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(roi_dialog), GTK_WIDGET(packing_table), label);

  /* widgets to change the location of the ROI's center in real space */
  label = gtk_label_new("Center Location (mm from view origin)");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,2,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  center = realspace_base_coord_to_alt(roi_calculate_center(roi), study_coord_frame(ui_study->study));
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
  gtk_object_set_data(GTK_OBJECT(entry), "roi_dialog", roi_dialog); 
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(CENTER_X));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_roi_dialog_cb_change_entry), 
		     roi_new_info);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
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
  gtk_object_set_data(GTK_OBJECT(entry), "roi_dialog", roi_dialog); 
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(CENTER_Y));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_roi_dialog_cb_change_entry), 
		     roi_new_info);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
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
  gtk_object_set_data(GTK_OBJECT(entry), "roi_dialog", roi_dialog); 
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(CENTER_Z));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_roi_dialog_cb_change_entry), 
		     roi_new_info);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator, 0, 4, table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* a canvas to indicate which way is x, y, and z */
  axis_indicator = ui_common_create_view_axis_indicator(ui_study->canvas_layout);
  gtk_table_attach(GTK_TABLE(packing_table), axis_indicator,0,4,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  table_row++;



  /* ---------------------------
     Dimension adjustment page
     --------------------------- */

  if (roi->type != ISOCONTOUR_3D) {

    /* the next page of options */
    packing_table = gtk_table_new(4,2,FALSE);
    table_row=0;
    label = gtk_label_new("Dimensions");
    gnome_property_box_append_page (GNOME_PROPERTY_BOX(roi_dialog), GTK_WIDGET(packing_table), label);
    
    /* widgets to change the dimensions of the ROI (in roi space) */
    label = gtk_label_new("Dimensions (mm from center of roi)");
    gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,2,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    table_row++;
   
    if (roi->type != ISOCONTOUR_2D) {
 
      /**************/
      label = gtk_label_new("x'");
      gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      entry = gtk_entry_new();
      temp_string = g_strdup_printf("%f", roi_new_info->corner.x);
      gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
      g_free(temp_string);
      gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
      gtk_object_set_data(GTK_OBJECT(entry), "roi_dialog", roi_dialog); 
      gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(DIM_X));
      gtk_signal_connect(GTK_OBJECT(entry), "changed", 
			 GTK_SIGNAL_FUNC(ui_roi_dialog_cb_change_entry), 
			 roi_new_info);
      gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		       table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
      table_row++;
      
      /*************/
      label = gtk_label_new("y'");
      gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      entry = gtk_entry_new();
      temp_string = g_strdup_printf("%f", roi_new_info->corner.y);
      gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
      g_free(temp_string);
      gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
      gtk_object_set_data(GTK_OBJECT(entry), "roi_dialog", roi_dialog); 
      gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(DIM_Y));
      gtk_signal_connect(GTK_OBJECT(entry), "changed", 
			 GTK_SIGNAL_FUNC(ui_roi_dialog_cb_change_entry), 
			 roi_new_info);
      gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		       table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
      table_row++;
    }
      
    /*************/
    label = gtk_label_new("z'");
    gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    entry = gtk_entry_new();
    temp_string = g_strdup_printf("%f", roi_new_info->corner.z);
    gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
    g_free(temp_string);
    gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
    gtk_object_set_data(GTK_OBJECT(entry), "roi_dialog", roi_dialog); 
    gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(DIM_Z));
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		       GTK_SIGNAL_FUNC(ui_roi_dialog_cb_change_entry), 
		       roi_new_info);
    gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    table_row++;
      
    }


  /* ---------------------------
     Rotation adjustment page
     --------------------------- */


  /* start making the page to rotate the roi */
  packing_table = gtk_table_new(9,4,FALSE);
  label = gtk_label_new("Rotate");
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(roi_dialog), GTK_WIDGET(packing_table), label);
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
    gtk_object_set_data(GTK_OBJECT(adjustment), "roi_dialog", roi_dialog); 
    gtk_object_set_data(GTK_OBJECT(adjustment), "ui_study", ui_study);
    dial = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
    gtk_range_set_update_policy(GTK_RANGE(dial), GTK_UPDATE_DISCONTINUOUS);
    gtk_table_attach(GTK_TABLE(packing_table), 
		     GTK_WIDGET(dial),
		     1,4, table_row, table_row+1,
		     X_PACKING_OPTIONS | GTK_FILL,FALSE, X_PADDING, Y_PADDING);
    gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed", 
		       GTK_SIGNAL_FUNC(ui_roi_dialog_cb_change_axis), 
		       roi_new_info);
    table_row++;
  }


  /* button to reset the axis */
  label = gtk_label_new("roi axis:");
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(label), 0,1, 
		   table_row, table_row+1,
		   0, 0, X_PADDING, Y_PADDING);
  
  button = gtk_button_new_with_label("reset to default");
  gtk_object_set_data(GTK_OBJECT(button), "roi_dialog", roi_dialog); 
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(button), 1,2, 
		   table_row, table_row+1,
		   0, 0, X_PADDING, Y_PADDING);
  gtk_signal_connect(GTK_OBJECT(button), "pressed",
		     GTK_SIGNAL_FUNC(ui_roi_dialog_cb_reset_axis), 
		     roi_new_info);
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
    gtk_object_set_data(GTK_OBJECT(roi_dialog), axis_names[i_axis], label);
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

  ui_roi_dialog_set_axis_display(roi_dialog);







  /* and show all our widgets */
  gtk_widget_show_all(roi_dialog);

  return;
}











