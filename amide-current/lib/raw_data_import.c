/* raw_data_import.c
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
#include "amide.h"
#include <matrix.h>
#include "volume.h"
#include <sys/stat.h>
#include "raw_data_import.h"
#include "raw_data_import_callbacks.h"


/* reset the label for the offset entry */
void raw_data_update_offset_label(raw_data_info_t * raw_data_info) {

  if (raw_data_info->data_format == ASCII)
    gtk_label_set_text(GTK_LABEL(raw_data_info->read_offset_label), 
		       "read offset (entries):");
  else
    gtk_label_set_text(GTK_LABEL(raw_data_info->read_offset_label), 
		       "read offset (bytes):");
}

/* calculate the total amount of the file that will be read through */
guint raw_data_update_num_bytes(raw_data_info_t * raw_data_info) {

  guint num_bytes, num_entries;
  gchar * temp_string;
  
    

  /* how many bytes we're currently reading from the file */
  if (raw_data_info->data_format == ASCII) {
    num_entries = 
      raw_data_info->offset +
      raw_data_calc_num_entries(raw_data_info->volume->dim, 
				raw_data_info->volume->num_frames);
    gtk_label_set_text(GTK_LABEL(raw_data_info->num_bytes_label1), 
		       "total entries to read through:");
    temp_string = g_strdup_printf("%d",num_entries);
    gtk_label_set_text(GTK_LABEL(raw_data_info->num_bytes_label2), temp_string);
    g_free(temp_string);

    /* and figure out a bare minimum for how many bytes might be in the file,
       this would correspond to all single digit numbers, seperated by spaces */
    num_bytes = 2*num_entries; 

  } else {
    num_bytes = 
      raw_data_info->offset +
      raw_data_calc_num_bytes(raw_data_info->volume->dim, 
			      raw_data_info->volume->num_frames,
			      raw_data_info->data_format);
    gtk_label_set_text(GTK_LABEL(raw_data_info->num_bytes_label1), 
		       "total bytes to read through:");
    temp_string = g_strdup_printf("%d",num_bytes);
    gtk_label_set_text(GTK_LABEL(raw_data_info->num_bytes_label2), temp_string);
    g_free(temp_string);
  }

  /* if we think we can load in the file, desensitise the "ok" button */
  if ((num_bytes <= raw_data_info->total_file_size) && (num_bytes > 0))
    gnome_dialog_set_sensitive(GNOME_DIALOG(raw_data_info->dialog), 0, TRUE);
  else
    gnome_dialog_set_sensitive(GNOME_DIALOG(raw_data_info->dialog), 0, FALSE);

  return num_bytes;
}


/* function to bring up the dialog widget to direct our importing of raw data */
void raw_data_import_dialog(raw_data_info_t * raw_data_info) {

  modality_t i_modality;
  dimension_t i_dim;
  data_format_t i_data_format;
  axis_t i_axis;
  gchar * temp_string = NULL;
  gchar * volume_name;
  gchar ** frags;
  GtkWidget * raw_data_dialog;
  GtkWidget * packing_table;
  GtkWidget * label;
  GtkWidget * entry;
  GtkWidget * option_menu;
  GtkWidget * menu;
  GtkWidget * menuitem;
  guint table_row = 0;



  /* start making the import dialog */
  temp_string = g_strdup_printf(_("%s: Raw Data Import Dialog\n"), PACKAGE);
  raw_data_dialog = gnome_dialog_new(temp_string,
				     GNOME_STOCK_BUTTON_OK,
				     GNOME_STOCK_BUTTON_CANCEL,
				     NULL);
  g_free(temp_string);
  raw_data_info->dialog = raw_data_dialog; 

  /* set some dialog default actions */
  gnome_dialog_set_close(GNOME_DIALOG(raw_data_dialog), TRUE);
  gnome_dialog_set_default(GNOME_DIALOG(raw_data_dialog), GNOME_OK);

  /* make the packing table */
  packing_table = gtk_table_new(10,5,FALSE);
  gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(raw_data_dialog)->vbox),
		     packing_table, TRUE, TRUE, 0);


  /* widgets to change the roi's name */
  label = gtk_label_new("name:");
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(label), 0,1,
		   table_row, table_row+1,
		   0, 0,
		   X_PADDING, Y_PADDING);

  /* figure out an initial name for the data */
  volume_name = g_strdup(g_basename(raw_data_info->filename));
  /* remove the extension of the file */
  g_strreverse(volume_name);
  frags = g_strsplit(volume_name, ".", 2);
  g_strreverse(volume_name);
  volume_name = frags[1];
  g_strreverse(volume_name);

  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), volume_name);
  volume_set_name(raw_data_info->volume,volume_name);

  g_strfreev(frags); /* free up now unused strings */
  g_free(volume_name);

  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(raw_data_import_callbacks_change_name), 
		     raw_data_info);
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(entry),1,2,
		   table_row, table_row+1,
		   X_PACKING_OPTIONS, 0,
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
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
     		       GTK_SIGNAL_FUNC(raw_data_import_callbacks_change_modality), 
    		       raw_data_info);
  }
  
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), PET);
  raw_data_info->volume->modality = PET;
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(option_menu), 1,2, 
		   table_row,table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, 
		   X_PADDING, Y_PADDING);
  table_row++;


  /* widgets to change the raw data file's  data format */
  label = gtk_label_new("data format:");
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(label), 0,1,
		   table_row, table_row+1,
		   0, 0,
		   X_PADDING, Y_PADDING);

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();

  for (i_data_format=0; i_data_format<NUM_DATA_FORMATS; i_data_format++) {
    menuitem = gtk_menu_item_new_with_label(data_format_names[i_data_format]);
    gtk_menu_append(GTK_MENU(menu), menuitem);
    gtk_object_set_data(GTK_OBJECT(menuitem), "data_format", GINT_TO_POINTER(i_data_format)); 
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
     		       GTK_SIGNAL_FUNC(raw_data_import_callbacks_change_data_format), 
    		       raw_data_info);
  }
  
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), UBYTE);
  raw_data_info->data_format = UBYTE;
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(option_menu), 1,2, 
		   table_row,table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, 
		   X_PADDING, Y_PADDING);

  /* how many bytes we can read from the file */
  label = gtk_label_new("file size (bytes):");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 3,4,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  /* how many bytes we're currently reading from the file */
  temp_string = g_strdup_printf("%d", raw_data_info->total_file_size);
  label = gtk_label_new(temp_string);
  g_free(temp_string);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 4,5,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  table_row++;


  /* what offset in the raw_data file we should start reading at */
  raw_data_info->read_offset_label = gtk_label_new("");
  raw_data_update_offset_label(raw_data_info);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(raw_data_info->read_offset_label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  raw_data_info->offset = 0;
  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%d", raw_data_info->offset);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(NUM_DIMS+NUM_AXIS));
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(raw_data_import_callbacks_change_entry), 
		     raw_data_info);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		   table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);


  /* how many bytes we're currently reading from the file */
  raw_data_info->num_bytes_label1 = gtk_label_new("");
  raw_data_info->num_bytes_label2 = gtk_label_new("");
  raw_data_update_num_bytes(raw_data_info); /* put something sensible into the label */
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(raw_data_info->num_bytes_label1), 
		   3,4, table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(raw_data_info->num_bytes_label2), 
		   4,5, table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  table_row++;



  /* labels for the x, y, and z components */
  label = gtk_label_new("x");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 1,2,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  label = gtk_label_new("y");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 2,3,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  label = gtk_label_new("z");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 3,4,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  label = gtk_label_new("frames");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 4,5,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  table_row++;


  /* widgets to change the dimensions of the volume */
  label = gtk_label_new("dimensions (# voxels)");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);


  raw_data_info->volume->dim.x = 
    raw_data_info->volume->dim.y = 
    raw_data_info->volume->dim.z = 
    raw_data_info->volume->num_frames = 1;
  for (i_dim=0; i_dim<NUM_DIMS; i_dim++) {
    entry = gtk_entry_new();
    temp_string = g_strdup_printf("%d", 1);
    gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
    g_free(temp_string);
    gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
    gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(i_dim));
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		       GTK_SIGNAL_FUNC(raw_data_import_callbacks_change_entry), 
		       raw_data_info);
    gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),i_dim+1,i_dim+2,
		     table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);
  }
  table_row++;

  /* widgets to change the voxel size of the volume */
  label = gtk_label_new("voxel size (mm)");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);


  raw_data_info->volume->voxel_size.x = 
    raw_data_info->volume->voxel_size.y = 
    raw_data_info->volume->voxel_size.z=1;
  for (i_axis=0; i_axis<NUM_AXIS; i_axis++) {
    entry = gtk_entry_new();
    temp_string = g_strdup_printf("%d", 1);
    gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
    g_free(temp_string);
    gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
    gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(i_axis+NUM_DIMS));
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		       GTK_SIGNAL_FUNC(raw_data_import_callbacks_change_entry), 
		       raw_data_info);
    gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),i_axis+1,i_axis+2,
		     table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);
  }
  table_row++;


  /* conversion factor to apply to the data */
  label = gtk_label_new("conversion factor");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  raw_data_info->volume->conversion = 1.0;
  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%5.3f", raw_data_info->volume->conversion);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(raw_data_import_callbacks_change_conversion), 
		     raw_data_info);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		   table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);
  table_row++;

  gtk_widget_show_all(raw_data_dialog);

  return;
}



/* function to bring up the dialog widget to direct our importing of raw data */
volume_t * raw_data_import(gchar * raw_data_filename) {

  struct stat file_info;
  raw_data_info_t * raw_data_info;
  volume_t * temp_volume;
  gint dialog_reply;

  /* get space for our raw_data_info structure */
  if ((raw_data_info = (raw_data_info_t * ) g_malloc(sizeof(raw_data_info_t))) == NULL) {
    g_warning("%s: couldn't allocate space for raw_data_info structure to load in RAW file", PACKAGE);
    return NULL;
  }
  raw_data_info->volume = NULL;
  raw_data_info->filename = g_strdup(raw_data_filename);

  /* figure out the file size in bytes (file_info.st_size) */
  if (stat(raw_data_info->filename, &file_info) != 0) {
    g_warning("%s: couldn't get stat's on file %s", PACKAGE, raw_data_info->filename);
    g_free(raw_data_info);
    return NULL;
  }
  raw_data_info->total_file_size = file_info.st_size;

  /* acquire space for the volume structure */
  if ((raw_data_info->volume = volume_init()) == NULL) {
    g_warning("%s: couldn't allocate space for the volume structure to hold RAW data", PACKAGE);
    g_free(raw_data_info->filename);
    g_free(raw_data_info);
    return NULL;
  }


  /* create and run the raw_data_import_dialog to acquire needed info from the user */
  raw_data_import_dialog(raw_data_info);

  /* block till the user closes the dialog */
  dialog_reply = gnome_dialog_run(GNOME_DIALOG(raw_data_info->dialog));
  
  /* and start loading in the file if we hit ok*/
  if (dialog_reply == 0) {
    REALPOINT_MULT(raw_data_info->volume->dim, 
		   raw_data_info->volume->voxel_size, 
		   raw_data_info->volume->corner);
    temp_volume = raw_data_read_file(raw_data_info->filename,
				     raw_data_info->volume, 
				     raw_data_info->data_format,
				     raw_data_info->offset);
  } else /* we hit the cancel button */
    temp_volume = volume_free(raw_data_info->volume);

  g_free(raw_data_info->filename);
  g_free(raw_data_info); /* note, we've saved a pointer to raw_data_info->volume in temp_volume */
  return temp_volume; /* and return the new volume structure (it's NULL if we cancelled) */

}










