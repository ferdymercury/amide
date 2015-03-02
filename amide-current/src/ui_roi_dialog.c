/* ui_roi_dialog.c
 *
 * Part of amide - Amide's a Medical Image Dataset Viewer
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
#include "volume.h"
#include "roi.h"
#include "study.h"
#include "color_table.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_study_rois.h"
#include "ui_study.h"
#include "ui_roi_dialog_callbacks.h"
#include "ui_roi_dialog.h"

/* function that sets up the roi dialog */
void ui_roi_dialog_create(ui_study_t * ui_study, amide_roi_t * roi) {
  
  GnomeApp * app;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * label;
  GtkWidget * option_menu;
  GtkWidget * menu;
  GtkWidget * menuitem;
  GtkWidget * entry;
  roi_type_t i_roi_type;
  roi_grain_t i_grain;
  guint table_row = 0;
  ui_study_roi_list_t * roi_list_item;

  /* figure out the ui_study_roi_list item corresponding to this roi */
  roi_list_item = ui_study->current_rois;
  if (roi_list_item == NULL) return;

  while (roi_list_item->roi != roi)
    if (roi_list_item->next == NULL) return;
    else roi_list_item = roi_list_item->next;
  
  /* sanity check */
  if (roi_list_item == NULL)
    return;
  if (roi_list_item->dialog != NULL)
    return;
  g_return_if_fail(roi_list_item->tree != NULL);
  g_return_if_fail(roi_list_item->tree_node != NULL);
    
  temp_string = g_strdup_printf("ROI Modification Dialog");
  app = GNOME_APP(gnome_app_new(PACKAGE, temp_string));
  free(temp_string);

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(app), "delete_event",
		     GTK_SIGNAL_FUNC(ui_roi_dialog_callbacks_delete_event),
		     roi_list_item);

  /* make the widgets for this dialog box */
  packing_table = gtk_table_new(1,2,FALSE);
  gnome_app_set_contents(app, GTK_WIDGET(packing_table));
  roi_list_item->dialog = app; /* save a pointer to the dialog */

  /* widgets to change the roi's name */
  label = gtk_label_new("name:");
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(label), 0,1,
		   table_row, table_row+1,
		   0, 0,
		   X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), roi->name);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_signal_connect(GTK_OBJECT(entry), "activate", 
  //  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_roi_dialog_callbacks_change_name), 
		     roi_list_item);
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(entry),1,2,
		   table_row, table_row+1,
		   X_PACKING_OPTIONS, 0,
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

  for (i_roi_type=0; i_roi_type<NUM_ROI_TYPES; i_roi_type++) {
    menuitem = gtk_menu_item_new_with_label(roi_type_names[i_roi_type]);
    gtk_menu_append(GTK_MENU(menu), menuitem);
    gtk_object_set_data(GTK_OBJECT(menuitem), "roi_type", roi_type_names[i_roi_type]); 
    gtk_object_set_data(GTK_OBJECT(menuitem), "ui_study", ui_study);
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
     		       GTK_SIGNAL_FUNC(ui_roi_dialog_callbacks_change_type), 
    		       roi_list_item);
  }
  
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), roi->type);
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(option_menu), 1,2, 
		   table_row,table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, 
		   X_PADDING, Y_PADDING);
  table_row++;



  /* widgets to change the roi's far corner (i.e. radius) */
  label = gtk_label_new("Far Corner (roi coords):");
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(label), 0,2,
		   table_row, table_row+1,
		   0, 0,
		   X_PADDING, Y_PADDING);
  table_row++;

  /*************/
  label = gtk_label_new("x:");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", roi->corner.x);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(CORNER_X));
  gtk_object_set_data(GTK_OBJECT(entry), "ui_study", ui_study);
  gtk_signal_connect(GTK_OBJECT(entry), "activate", 
		     GTK_SIGNAL_FUNC(ui_roi_dialog_callbacks_change_entry), 
		     roi_list_item);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		   table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);
  table_row++;

  /*************/
  label = gtk_label_new("y:");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", roi->corner.y);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(CORNER_Y));
  gtk_object_set_data(GTK_OBJECT(entry), "ui_study", ui_study);
  gtk_signal_connect(GTK_OBJECT(entry), "activate", 
		     GTK_SIGNAL_FUNC(ui_roi_dialog_callbacks_change_entry), 
		     roi_list_item);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		   table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);
  table_row++;

  /*************/
  label = gtk_label_new("z:");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", roi->corner.z);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(CORNER_Z));
  gtk_object_set_data(GTK_OBJECT(entry), "ui_study", ui_study);
  gtk_signal_connect(GTK_OBJECT(entry), "activate", 
		     GTK_SIGNAL_FUNC(ui_roi_dialog_callbacks_change_entry), 
		     roi_list_item);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		   table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);
  table_row++;


  /* widgets to change the object's grain size */
  label = gtk_label_new("grain:");
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(label), 0,1,
		   table_row, table_row+1,
		   0, 0,
		   X_PADDING, Y_PADDING);

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();

  for (i_grain=0; i_grain<NUM_GRAIN_TYPES; i_grain++) {
    menuitem = gtk_menu_item_new_with_label(roi_grain_names[i_grain]);
    gtk_menu_append(GTK_MENU(menu), menuitem);
    gtk_object_set_data(GTK_OBJECT(menuitem), "grain_size", roi_grain_names[i_grain]); 
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
     		       GTK_SIGNAL_FUNC(ui_roi_dialog_callbacks_change_grain), 
    		       roi_list_item);
  }
  
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), roi->grain);
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(option_menu), 1,2, 
		   table_row,table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, 
		   X_PADDING, Y_PADDING);
  table_row++;


  /* and show all our widgets */
  gtk_widget_show_all(GTK_WIDGET(app));

  return;
}



