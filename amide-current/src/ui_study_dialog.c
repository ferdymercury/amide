/* ui_study_dialog.c
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
#include "ui_study_dialog_callbacks.h"
#include "ui_study_dialog.h"








/* function to set the view display widgets */
void ui_study_dialog_set_axis_display(GtkWidget * study_dialog) {

  GtkWidget * label;
  GtkWidget * entry;
  gchar * temp_string;
  study_t * study_new_info;
  axis_t i_axis;

  study_new_info = gtk_object_get_data(GTK_OBJECT(study_dialog), "study_new_info");

  for (i_axis=0;i_axis<NUM_AXIS;i_axis++) {
    label = gtk_object_get_data(GTK_OBJECT(study_dialog), axis_names[i_axis]);

    entry = gtk_object_get_data(GTK_OBJECT(label), axis_names[XAXIS]);
    temp_string = g_strdup_printf("%f", study_new_info->coord_frame.axis[i_axis].x);
    gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
    g_free(temp_string);

    entry = gtk_object_get_data(GTK_OBJECT(label), axis_names[YAXIS]);
    temp_string = g_strdup_printf("%f", study_new_info->coord_frame.axis[i_axis].y);
    gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
    g_free(temp_string);

    entry = gtk_object_get_data(GTK_OBJECT(label), axis_names[ZAXIS]);
    temp_string = g_strdup_printf("%f", study_new_info->coord_frame.axis[i_axis].z);
    gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
    g_free(temp_string);

  }

  return;
}





/* function that sets up the study dialog */
void ui_study_dialog_create(ui_study_t * ui_study) {
  
  GtkWidget * study_dialog;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * label;
  GtkWidget * entry;
  GtkWidget * dial;
  GtkWidget * hseparator;
  GtkWidget * button;
  GtkObject * adjustment;
  view_t i_view;
  axis_t i_axis, j_axis;
  guint table_row = 0;
  study_t * study_new_info = NULL;


  /* only want one of these dialogs at a time for a given study */
  if (ui_study->study_dialog != NULL)
    return;
  
  /* sanity checks */
  temp_string = g_strdup_printf("%s: Study Parameter Modification Dialog",PACKAGE);
  study_dialog = gnome_property_box_new();
  gtk_window_set_title(GTK_WINDOW(study_dialog), temp_string);
  g_free(temp_string);
  ui_study->study_dialog = study_dialog; /* save a pointer to the dialog */

  /* create the temp study which will store the new information, and then
     can either be applied or cancelled */
  study_new_info = study_copy(ui_study->study);

  /* attach it to the dialog */
  gtk_object_set_data(GTK_OBJECT(study_dialog), "study_new_info", study_new_info);

  /* save a pointer to ui_study with the dialog widget */
  gtk_object_set_data(GTK_OBJECT(study_dialog), "ui_study", ui_study);

  /* and this is used to record if we need to redraw the canvas */
  gtk_object_set_data(GTK_OBJECT(study_dialog), "axis_changed", GINT_TO_POINTER(FALSE));
  

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(study_dialog), "close",
		     GTK_SIGNAL_FUNC(ui_study_dialog_callbacks_close_event),
		     ui_study);
  gtk_signal_connect(GTK_OBJECT(study_dialog), "apply",
		     GTK_SIGNAL_FUNC(ui_study_dialog_callbacks_apply),
		     ui_study);
  gtk_signal_connect(GTK_OBJECT(study_dialog), "help",
		     GTK_SIGNAL_FUNC(ui_study_dialog_callbacks_help),
		     ui_study);




  /* ---------------------------
     Basic info page 
     --------------------------- */


  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(4,2,FALSE);
  label = gtk_label_new("Basic Info");
  table_row=0;
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(study_dialog), GTK_WIDGET(packing_table), label);

  /* widgets to change the study's name */
  label = gtk_label_new("name:");
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(label), 0,1,
		   table_row, table_row+1,
		   0, 0,
		   X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), study_name(study_new_info));
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "study_dialog", study_dialog); 
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_study_dialog_callbacks_change_name), 
		     study_new_info);
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(entry),1,2,
		   table_row, table_row+1,
		   GTK_FILL, 0,
		   X_PADDING, Y_PADDING);
 
  table_row++;

  /* widgets to change the study's creation date */
  label = gtk_label_new("creation date:");
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(label), 0,1,
		   table_row, table_row+1,
		   0, 0,
		   X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), study_creation_date(study_new_info));
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "study_dialog", study_dialog); 
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_study_dialog_callbacks_change_creation_date), 
		     study_new_info);
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(entry),1,2,
		   table_row, table_row+1,
		   GTK_FILL, 0,
		   X_PADDING, Y_PADDING);
 
  table_row++;


  /* ---------------------------
     Rotation adjustment page
     --------------------------- */


  /* start making the page to change the view on the entire study */
  packing_table = gtk_table_new(9,4,FALSE);
  label = gtk_label_new("Rotate");
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(study_dialog), GTK_WIDGET(packing_table), label);
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
    gtk_object_set_data(GTK_OBJECT(adjustment), "study_dialog", study_dialog); 
    gtk_object_set_data(GTK_OBJECT(adjustment), "ui_study", ui_study);
    dial = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
    gtk_range_set_update_policy(GTK_RANGE(dial), GTK_UPDATE_DISCONTINUOUS);
    gtk_table_attach(GTK_TABLE(packing_table), 
		     GTK_WIDGET(dial),
		     1,4, table_row, table_row+1,
		     X_PACKING_OPTIONS | GTK_FILL,FALSE, X_PADDING, Y_PADDING);
    gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed", 
		       GTK_SIGNAL_FUNC(ui_study_dialog_callbacks_change_axis), 
		       study_new_info);
    table_row++;
  }


  /* button to reset the axis */
  label = gtk_label_new("study axis:");
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(label), 0,1, 
		   table_row, table_row+1,
		   0, 0, X_PADDING, Y_PADDING);
  
  button = gtk_button_new_with_label("reset to default");
  gtk_object_set_data(GTK_OBJECT(button), "study_dialog", study_dialog); 
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(button), 1,2, 
		   table_row, table_row+1,
		   0, 0, X_PADDING, Y_PADDING);
  gtk_signal_connect(GTK_OBJECT(button), "pressed",
		     GTK_SIGNAL_FUNC(ui_study_dialog_callbacks_reset_axis), 
		     study_new_info);
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
    gtk_object_set_data(GTK_OBJECT(study_dialog), axis_names[i_axis], label);
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

  ui_study_dialog_set_axis_display(study_dialog);




  /* and show all our widgets */
  gtk_widget_show_all(study_dialog);

  return;
}











