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
#include "amitk_coord_frame_edit.h"
#include "ui_study.h"
#include "ui_roi_dialog_cb.h"
#include "ui_roi_dialog.h"
#include "ui_common.h"



/* function that sets up the roi dialog */
GtkWidget * ui_roi_dialog_create(ui_study_t * ui_study, roi_t * roi) {
  
  GtkWidget * dialog;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * label;
  GtkWidget * option_menu;
  GtkWidget * menu;
  GtkWidget * menuitem;
  GtkWidget * entry;
  GtkWidget * hseparator;
  GtkWidget * axis_indicator;
  GtkWidget * cfe;
  roi_type_t i_roi_type;
  guint table_row = 0;
  roi_t * new_info = NULL;
  realpoint_t center;
  roi_type_t type_start, type_end;

  temp_string = g_strdup_printf("%s: ROI Modification Dialog",PACKAGE);
  dialog = gnome_property_box_new();
  gtk_window_set_title(GTK_WINDOW(dialog), temp_string);
  g_free(temp_string);

  /* create the temp roi which will store the new information, and then
     can either be applied or cancelled */
  new_info = roi_copy(roi);

  /* attach the to the dialog */
  gtk_object_set_data(GTK_OBJECT(dialog), "new_info", new_info);
  gtk_object_set_data(GTK_OBJECT(dialog), "old_info", roi);

  /* save a pointer to ui_study with the dialog widget so we can redraw roi's */ 
  gtk_object_set_data(GTK_OBJECT(dialog), "ui_study", ui_study);


  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(dialog), "close",
		     GTK_SIGNAL_FUNC(ui_roi_dialog_cb_close), ui_study);
  gtk_signal_connect(GTK_OBJECT(dialog), "apply",
		     GTK_SIGNAL_FUNC(ui_roi_dialog_cb_apply), ui_study);
  gtk_signal_connect(GTK_OBJECT(dialog), "help",
		     GTK_SIGNAL_FUNC(ui_roi_dialog_cb_help), NULL);





  /* ---------------------------
     Basic info page 
     --------------------------- */


  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(4,2,FALSE);
  label = gtk_label_new("Basic Info");
  table_row=0;
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(dialog), GTK_WIDGET(packing_table), label);

  /* widgets to change the roi's name */
  label = gtk_label_new("name:");
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(label), 0,1,
		   table_row, table_row+1,
		   0, 0,
		   X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), new_info->name);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_roi_dialog_cb_change_name), dialog);
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
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
     		       GTK_SIGNAL_FUNC(ui_roi_dialog_cb_change_type), dialog);
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
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(dialog), GTK_WIDGET(packing_table), label);

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
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(CENTER_X));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_roi_dialog_cb_change_entry), dialog);
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
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(CENTER_Y));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_roi_dialog_cb_change_entry), dialog);
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
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(CENTER_Z));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_roi_dialog_cb_change_entry), dialog);
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
    gnome_property_box_append_page (GNOME_PROPERTY_BOX(dialog), GTK_WIDGET(packing_table), label);
    
    /* widgets to change the dimensions of the ROI (in roi space) */
    label = gtk_label_new("Dimensions (mm) wrt to ROI");
    gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,2,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    table_row++;
   
    if (roi->type != ISOCONTOUR_2D) {
 
      /**************/
      label = gtk_label_new("x'");
      gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      entry = gtk_entry_new();
      temp_string = g_strdup_printf("%f", new_info->corner.x);
      gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
      g_free(temp_string);
      gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
      gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(DIM_X));
      gtk_signal_connect(GTK_OBJECT(entry), "changed", 
			 GTK_SIGNAL_FUNC(ui_roi_dialog_cb_change_entry), dialog);
      gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		       table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
      table_row++;
      
      /*************/
      label = gtk_label_new("y'");
      gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      entry = gtk_entry_new();
      temp_string = g_strdup_printf("%f", new_info->corner.y);
      gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
      g_free(temp_string);
      gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
      gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(DIM_Y));
      gtk_signal_connect(GTK_OBJECT(entry), "changed", 
			 GTK_SIGNAL_FUNC(ui_roi_dialog_cb_change_entry), dialog);
      gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		       table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
      table_row++;
    }
      
    /*************/
    label = gtk_label_new("z'");
    gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    entry = gtk_entry_new();
    temp_string = g_strdup_printf("%f", new_info->corner.z);
    gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
    g_free(temp_string);
    gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
    gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(DIM_Z));
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		       GTK_SIGNAL_FUNC(ui_roi_dialog_cb_change_entry), dialog);
    gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    table_row++;
      
    }


  /* ----------------------------------------
     Rotations page
     ---------------------------------------- */
  label = gtk_label_new("Rotate");
  cfe = amitk_coord_frame_edit_new(new_info->coord_frame, 
				   roi_calculate_center(new_info),
				   study_coord_frame(ui_study->study));
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(dialog), cfe, label);
 
  gtk_object_set_data(GTK_OBJECT(dialog), "cfe", cfe);
  gtk_signal_connect(GTK_OBJECT(cfe), "coord_frame_changed",
  		     GTK_SIGNAL_FUNC(ui_roi_dialog_cb_coord_frame_changed),
  		     dialog);



  /* and show all our widgets */
  gtk_widget_show_all(dialog);

  return dialog;
}











