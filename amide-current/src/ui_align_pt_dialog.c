/* ui_align_pt_dialog.c
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
#include "ui_align_pt_dialog_cb.h"
#include "ui_align_pt_dialog.h"
#include "ui_common.h"




/* function that sets up an align point dialog */
GtkWidget * ui_align_pt_dialog_create(ui_study_t * ui_study, align_pt_t * align_pt, volume_t * volume) {
  
  GtkWidget * dialog;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * label;
  GtkWidget * entry;
  GtkWidget * hseparator;
  GtkWidget * axis_indicator;
  guint table_row = 0;
  align_pt_t * new_info = NULL;
  realpoint_t center;

  temp_string = g_strdup_printf("%s: Alignment Point Modification Dialog",PACKAGE);
  dialog = gnome_property_box_new();
  gtk_window_set_title(GTK_WINDOW(dialog), temp_string);
  g_free(temp_string);

  /* create the temporary alignmen tpoint which will store the new information, and then
     can either be applied or cancelled */
  new_info = align_pt_copy(align_pt);

  /* attach it to the dialog */
  gtk_object_set_data(GTK_OBJECT(dialog), "new_info", new_info);
  gtk_object_set_data(GTK_OBJECT(dialog), "old_info", align_pt);
  gtk_object_set_data(GTK_OBJECT(dialog), "volume", volume);

  /* save a pointer to ui_study with the dialog widget so that we can get the view coord frame
     and redraw the alignment point */
  gtk_object_set_data(GTK_OBJECT(dialog), "ui_study", ui_study);


  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(dialog), "close",
		     GTK_SIGNAL_FUNC(ui_align_pt_dialog_cb_close), ui_study);
  gtk_signal_connect(GTK_OBJECT(dialog), "apply",
		     GTK_SIGNAL_FUNC(ui_align_pt_dialog_cb_apply), ui_study);
  gtk_signal_connect(GTK_OBJECT(dialog), "help",
		     GTK_SIGNAL_FUNC(ui_align_pt_dialog_cb_help), NULL);




  /* ---------------------------
     Basic info page 
     --------------------------- */


  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(4,2,FALSE);
  label = gtk_label_new("Basic Info");
  table_row=0;
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(dialog), GTK_WIDGET(packing_table), label);

  /* widgets to change the alignment points's name */
  label = gtk_label_new("alignment point:");
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(label), 0,1,
		   table_row, table_row+1,
		   0, 0,
		   X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), new_info->name);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_align_pt_dialog_cb_change_name), dialog);
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(entry),1,2,
		   table_row, table_row+1,
		   GTK_FILL, 0,
		   X_PADDING, Y_PADDING);
 
  table_row++;

  /* widgets to change the location of the Alignment Point in view space */
  label = gtk_label_new("Center Location (mm from view origin)");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,2,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  center = realspace_alt_coord_to_alt(align_pt->point, 
				      volume->coord_frame,
				      study_coord_frame(ui_study->study));
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
		     GTK_SIGNAL_FUNC(ui_align_pt_dialog_cb_change_entry), dialog);
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
		     GTK_SIGNAL_FUNC(ui_align_pt_dialog_cb_change_entry), dialog);
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
		     GTK_SIGNAL_FUNC(ui_align_pt_dialog_cb_change_entry), dialog);
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


  /* and show all our widgets */
  gtk_widget_show_all(dialog);

  return dialog;
}











