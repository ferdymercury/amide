/* ui_volume_dialog.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000 Andy Loening
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
#include "ui_volume_dialog_callbacks.h"
#include "ui_volume_dialog.h"

/* function that sets up the volume dialog */
void ui_volume_dialog_create(ui_study_t * ui_study, amide_volume_t * volume) {
  
  GtkWidget * volume_dialog;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * label;
  GtkWidget * option_menu;
  GtkWidget * menu;
  GtkWidget * menuitem;
  GtkWidget * entry;
  GtkObject * adjustment;
  GtkWidget * dial;
  guint table_row = 0;
  modality_t i_modality;
  view_t i_view;
  ui_study_volume_list_t * volume_list_item;
  amide_volume_t * volume_new_info = NULL;
  realpoint_t center;

  /* figure out the ui_study_volume_list item corresponding to this volume */
  volume_list_item = ui_study->current_volumes;
  if (volume_list_item == NULL) return;

  while (volume_list_item->volume != volume)
    if (volume_list_item->next == NULL) return;
    else volume_list_item = volume_list_item->next;

  /* only want one of these dialogs at a time for a given volume */
  if (volume_list_item->dialog != NULL)
    return;
  
  /* sanity checks */
  g_return_if_fail(volume_list_item != NULL);
  g_return_if_fail(volume_list_item->tree != NULL);
  g_return_if_fail(volume_list_item->tree_node != NULL);
    
  temp_string = g_strdup_printf("%s: Volume Modification Dialog",PACKAGE);
  volume_dialog = gnome_property_box_new();
  gtk_window_set_title(GTK_WINDOW(volume_dialog), temp_string);
  g_free(temp_string);

  /* create the temp volume which will store the new information, and then
     can either be applied or cancelled */
  volume_copy(&volume_new_info, volume);

  /* attach it to the dialog */
  gtk_object_set_data(GTK_OBJECT(volume_dialog), "volume_new_info", volume_new_info);

  /* save a pointer to ui_study with the dialog widget so we can redraw volumes's */ 
  gtk_object_set_data(GTK_OBJECT(volume_dialog), "ui_study", ui_study);

  
  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(volume_dialog), "close",
		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_close_event),
		     volume_list_item);
  gtk_signal_connect(GTK_OBJECT(volume_dialog), "apply",
		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_apply),
		     volume_list_item);
  gtk_signal_connect(GTK_OBJECT(volume_dialog), "help",
		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_help),
		     volume_list_item);

  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(4,2,FALSE);
  label = gtk_label_new("Basic Info");
  table_row=0;
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(volume_dialog), GTK_WIDGET(packing_table), label);
  volume_list_item->dialog = volume_dialog; /* save a pointer to the dialog */

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




  /* the next page of options */
  packing_table = gtk_table_new(4,2,FALSE);
  table_row=0;
  label = gtk_label_new("Center");
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(volume_dialog), GTK_WIDGET(packing_table), label);

  /* widgets to change the location of the volume's center in real space */
  label = gtk_label_new("Center Location (mm from origin)");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,2,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  center = volume_calculate_center(volume);
  table_row++;

  /**************/
  label = gtk_label_new("x");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", center.x);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "volume_dialog", volume_dialog); 
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(CENTER_X));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_entry), 
		     volume_new_info);
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
  free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "volume_dialog", volume_dialog); 
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(CENTER_Y));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_entry), 
		     volume_new_info);
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
  free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "volume_dialog", volume_dialog); 
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(CENTER_Z));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_entry), 
		     volume_new_info);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  /* start making the page to rotate the volume */
  packing_table = gtk_table_new(4,4,FALSE);
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
    gtk_object_set_data(GTK_OBJECT(adjustment), "view", view_names[i_view]); 
    gtk_object_set_data(GTK_OBJECT(adjustment), "volume_dialog", volume_dialog); 
    gtk_object_set_data(GTK_OBJECT(adjustment), "ui_study", ui_study);
    dial = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
    gtk_range_set_update_policy(GTK_RANGE(dial), GTK_UPDATE_DISCONTINUOUS);
    gtk_table_attach(GTK_TABLE(packing_table), 
		     GTK_WIDGET(dial),
		     1,4, table_row, table_row+1,
		     X_PACKING_OPTIONS | GTK_FILL,FALSE, X_PADDING, Y_PADDING);
    gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed", 
		       GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_axis_change), 
		       volume_new_info);
    table_row++;
  }



  /* start making the page to change the threshold/colormap */
  packing_table = gtk_table_new(4,2,FALSE);
  label = gtk_label_new("Colormap/Threshold");
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(volume_dialog), GTK_WIDGET(packing_table), label);
  table_row=0;


  /* and show all our widgets */
  gtk_widget_show_all(volume_dialog);

  return;
}











