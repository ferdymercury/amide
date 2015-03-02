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
#include "study.h"
#include "amitk_coord_frame_edit.h"
#include "ui_study.h"
#include "ui_study_dialog_cb.h"
#include "ui_study_dialog.h"



/* function that sets up the study dialog */
GtkWidget * ui_study_dialog_create(ui_study_t * ui_study) {
  
  GtkWidget * dialog;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * label;
  GtkWidget * entry;
  GtkWidget * cfe;
  guint table_row = 0;
  study_t * new_info = NULL;


  /* sanity checks */
  temp_string = g_strdup_printf("%s: Study Parameter Modification Dialog",PACKAGE);
  dialog = gnome_property_box_new();
  gtk_window_set_title(GTK_WINDOW(dialog), temp_string);
  g_free(temp_string);

  /* create the temp study which will store the new information, and then
     can either be applied or cancelled */
  new_info = study_copy(ui_study->study);

  /* attach it to the dialog */
  gtk_object_set_data(GTK_OBJECT(dialog), "new_info", new_info);

  /* save a pointer to ui_study with the dialog widget */
  gtk_object_set_data(GTK_OBJECT(dialog), "ui_study", ui_study);

  /* and this is used to record if we need to redraw the canvas */
  gtk_object_set_data(GTK_OBJECT(dialog), "axis_changed", GINT_TO_POINTER(FALSE));
  

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(dialog), "close",
		     GTK_SIGNAL_FUNC(ui_study_dialog_cb_close), ui_study);
  gtk_signal_connect(GTK_OBJECT(dialog), "apply",
		     GTK_SIGNAL_FUNC(ui_study_dialog_cb_apply), ui_study);
  gtk_signal_connect(GTK_OBJECT(dialog), "help",
		     GTK_SIGNAL_FUNC(ui_study_dialog_cb_help), NULL);




  /* ---------------------------
     Basic info page 
     --------------------------- */


  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(4,2,FALSE);
  label = gtk_label_new("Basic Info");
  table_row=0;
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(dialog), GTK_WIDGET(packing_table), label);

  /* widgets to change the study's name */
  label = gtk_label_new("name:");
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(label), 0,1,
		   table_row, table_row+1,
		   0, 0,
		   X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), study_name(new_info));
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_study_dialog_cb_change_name), dialog);
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
  gtk_entry_set_text(GTK_ENTRY(entry), study_creation_date(new_info));
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_study_dialog_cb_change_creation_date), new_info);
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(entry),1,2,
		   table_row, table_row+1,
		   GTK_FILL, 0,
		   X_PADDING, Y_PADDING);
 
  table_row++;


  /* ----------------------------------------
     Rotations page
     ---------------------------------------- */
  label = gtk_label_new("Rotate");
  cfe = amitk_coord_frame_edit_new(new_info->coord_frame, 
				   study_view_center(new_info),
				   study_coord_frame(ui_study->study));
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(dialog), cfe, label);
 
  gtk_object_set_data(GTK_OBJECT(dialog), "cfe", cfe);
  gtk_signal_connect(GTK_OBJECT(cfe), "coord_frame_changed",
  		     GTK_SIGNAL_FUNC(ui_study_dialog_cb_coord_frame_changed),
  		     dialog);


  /* and show all our widgets */
  gtk_widget_show_all(dialog);

  return dialog;
}











