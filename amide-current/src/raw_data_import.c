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
#include "realspace.h"
#include "color_table.h"
#include "volume.h"
#include <sys/stat.h>
#include "raw_data_import.h"
#include "raw_data_import_callbacks.h"

/* external variables */
gchar * data_format_names[] = {"Unsigned Byte (8 bit)", \
			       "Signed Byte (8 bit)", \
			       "Unsigned Short, Little Endian (16 bit)", \
			       "Signed Short, Little Endian (16 bit)", \
			       "Unsigned Integer, Little Endian (32 bit)", \
			       "Signed Integer, Little Endian (32 bit)", \
			       "Float, Little Endian (32 bit)", \
			       "Double, Little Endian (64 bit)", \
			       "Unsigned Short, Big Endian (16 bit)", \
			       "Signed Short, Big Endian (16 bit)", \
			       "Unsigned Integer, Big Endian (32 bit)", \
			       "Signed Integer, Big Endian (32 bit)", \
			       "Float, Big Endian (32 bit)", \
			       "Double, Big Endian (64 bit)"};

guint data_sizes[] = {1,1,2,2,4,4,4,8,2,2,4,4,4,8};

/* calculate the number of bytes that will be read from the file*/
guint raw_data_calc_num_bytes(raw_data_info_t * raw_data_info) {

  guint num_bytes;
  
  num_bytes = raw_data_info->volume->dim.x* 
    raw_data_info->volume->dim.y* 
    raw_data_info->volume->dim.z* 
    raw_data_info->volume->num_frames*
    data_sizes[raw_data_info->data_format];

  return num_bytes;
}


/* read in the file specified in the raw_data_info structure, and put it in the corresponding volume */
/* notes: 
   -the volume in raw_data_info should be initialized and have the correct dimensional information 
   already filled in 
   -although the function returns nothing, if read is unsuccessful, raw_data_info->volume will be NULL
*/
void raw_data_read_file(raw_data_info_t * raw_data_info) {

  FILE * file_pointer;
  void * file_buffer=NULL;
  size_t bytes_read;
  size_t bytes_to_read;
  volume_data_t max,min,temp;
  guint t;
  voxelpoint_t i;

  /* set the far corner of the volume */
  raw_data_info->volume->corner.x = raw_data_info->volume->dim.x*raw_data_info->volume->voxel_size.x;
  raw_data_info->volume->corner.y = raw_data_info->volume->dim.y*raw_data_info->volume->voxel_size.y;
  raw_data_info->volume->corner.z = raw_data_info->volume->dim.z*raw_data_info->volume->voxel_size.z;
  
  /* malloc the space for the volume data */
  g_free(raw_data_info->volume->data); /* make sure we're doing garbage collection */
  if ((raw_data_info->volume->data = volume_get_data_mem(raw_data_info->volume)) == NULL) {
    g_warning("%s: couldn't allocate space for the volume\n",PACKAGE);
    raw_data_info->volume = volume_free(raw_data_info->volume);
    return;
  }

  /* open the file for reading */
  if ((file_pointer = fopen(raw_data_info->filename, "r")) == NULL) {
    g_warning("%s: couldn't open raw data file %s\n", PACKAGE,raw_data_info->filename);
    raw_data_info->volume = volume_free(raw_data_info->volume);
    return;
  }
  
  /* jump forward by the given offset */
  if (fseek(file_pointer, raw_data_info->offset, SEEK_SET) != 0) {
    g_warning("%s: could not seek forward %d bytes in raw data file:\n\t%s\n",
	      PACKAGE, raw_data_info->offset, raw_data_info->filename);
    raw_data_info->volume = volume_free(raw_data_info->volume);
    fclose(file_pointer);
    return;
  }
    
  /* read in the contents of the file */
  bytes_to_read = raw_data_calc_num_bytes(raw_data_info);
  file_buffer = (void *) g_malloc(bytes_to_read);
  bytes_read = fread(file_buffer, 1, bytes_to_read, file_pointer );
  if (bytes_read != bytes_to_read) {
    g_warning("%s: read wrong number of elements from raw data file:\n\t%s\n\texpected %d\tgot %d\n", 
	      PACKAGE,raw_data_info->filename, bytes_to_read, bytes_read);
    raw_data_info->volume = volume_free(raw_data_info->volume);
    g_free(file_buffer);
    fclose(file_pointer);
    return;
  }
  fclose(file_pointer);
  
    

  /* set the scan start time */
  raw_data_info->volume->scan_start = 0.0;
  
  /* allocate space for the array containing info on the duration of the frames */
  if ((raw_data_info->volume->frame_duration = volume_get_frame_duration_mem(raw_data_info->volume)) == NULL) {
    g_warning("%s: couldn't allocate space for the frame duration info\n",PACKAGE);
    raw_data_info->volume = volume_free(raw_data_info->volume);
    g_free(file_buffer);
    return;
  }
  
  /* iterate over the # of frames */
  for (t = 0; t < raw_data_info->volume->num_frames; t++) {
#ifdef AMIDE_DEBUG
    g_print("\tloading frame %d\n",t);
#endif
    /* set the duration of this frame */
    raw_data_info->volume->frame_duration[t] = 1.0;
    
    /* and convert the data */
    switch (raw_data_info->data_format) {
	
    case DOUBLE_BE: 
      {
	gdouble * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_info->volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_info->volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_info->volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_info->volume,t,i) =
		GUINT64_FROM_BE(data[i.x + 
				    raw_data_info->volume->dim.x*i.y +
				    raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*i.z +
				    raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*raw_data_info->volume->dim.z*t]);
	
      } 
      break;
    case FLOAT_BE:
      {
	gfloat * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_info->volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_info->volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_info->volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_info->volume,t,i) =
		GUINT32_FROM_BE(data[i.x + 
				    raw_data_info->volume->dim.x*i.y +
				    raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*i.z +
				    raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*raw_data_info->volume->dim.z*t]);
	  
      }
      break;
    case SINT_BE:
      {
	gint32 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_info->volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_info->volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_info->volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_info->volume,t,i) =
		GINT32_FROM_BE(data[i.x + 
				   raw_data_info->volume->dim.x*i.y +
				   raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*i.z +
				   raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*raw_data_info->volume->dim.z*t]);
	  
      }
      break;
    case UINT_BE:
      {
	guint32 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_info->volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_info->volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_info->volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_info->volume,t,i) =
		GUINT32_FROM_BE(data[i.x + 
				   raw_data_info->volume->dim.x*i.y +
				   raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*i.z +
				   raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*raw_data_info->volume->dim.z*t]);

      }
      break;
    case SSHORT_BE:
      {
	gint16 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_info->volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_info->volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_info->volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_info->volume,t,i) =
		GINT16_FROM_BE(data[i.x + 
				   raw_data_info->volume->dim.x*i.y +
				   raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*i.z +
				   raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*raw_data_info->volume->dim.z*t]);

      }
      break;
    case USHORT_BE:
      {
	guint16 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_info->volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_info->volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_info->volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_info->volume,t,i) =
		GUINT16_FROM_BE(data[i.x + 
				   raw_data_info->volume->dim.x*i.y +
				   raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*i.z +
				   raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*raw_data_info->volume->dim.z*t]);

      }
      break;
    case DOUBLE_LE:
      {
	gdouble * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_info->volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_info->volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_info->volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_info->volume,t,i) =
		GUINT64_FROM_LE(data[i.x + 
				    raw_data_info->volume->dim.x*i.y +
				    raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*i.z +
				    raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*raw_data_info->volume->dim.z*t]);

      }
      break;
    case FLOAT_LE:
      {
	gfloat * data=file_buffer;
	
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_info->volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_info->volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_info->volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_info->volume,t,i) =
		GUINT32_FROM_LE(data[i.x + 
				    raw_data_info->volume->dim.x*i.y +
				    raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*i.z +
				    raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*raw_data_info->volume->dim.z*t]);

      }
      break;
    case SINT_LE:
      {
	gint32 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_info->volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_info->volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_info->volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_info->volume,t,i) =
		GINT32_FROM_LE(data[i.x + 
				   raw_data_info->volume->dim.x*i.y +
				   raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*i.z +
				   raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*raw_data_info->volume->dim.z*t]);
	  
      }
      break;
    case UINT_LE:
      {
	guint32 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_info->volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_info->volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_info->volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_info->volume,t,i) =
		GUINT32_FROM_LE(data[i.x + 
				    raw_data_info->volume->dim.x*i.y +
				    raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*i.z +
				    raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*raw_data_info->volume->dim.z*t]);

      }
      break;
    case SSHORT_LE:
      {
	gint16 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_info->volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_info->volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_info->volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_info->volume,t,i) =
		GINT16_FROM_LE(data[i.x + 
				   raw_data_info->volume->dim.x*i.y +
				   raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*i.z +
				   raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*raw_data_info->volume->dim.z*t]);

      }
      break;
    case USHORT_LE:
      {
	guint16 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_info->volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_info->volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_info->volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_info->volume,t,i) =
		GUINT16_FROM_LE(data[i.x + 
				    raw_data_info->volume->dim.x*i.y +
				    raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*i.z +
				    raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*raw_data_info->volume->dim.z*t]);

      }
      break;
    case SBYTE:
      {
	gint8 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_info->volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_info->volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_info->volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_info->volume,t,i) =
		data[i.x + 
		    raw_data_info->volume->dim.x*i.y +
		    raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*i.z +
		    raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*raw_data_info->volume->dim.z*t];

      }
      break;
    case UBYTE:
    default:
      {
	guint8 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_info->volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_info->volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_info->volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_info->volume,t,i) =
		data[i.x + 
		    raw_data_info->volume->dim.x*i.y +
		    raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*i.z +
		    raw_data_info->volume->dim.x*raw_data_info->volume->dim.y*raw_data_info->volume->dim.z*t];
	  
      }
      break;
    }
  }      

  /* set the max/min values in the volume */
#ifdef AMIDE_DEBUG
  g_print("\tcalculating max & min");
#endif
  max = 0.0;
  min = 0.0;
  for(t = 0; t < raw_data_info->volume->num_frames; t++) {
    for (i.z = 0; i.z < raw_data_info->volume->dim.z; i.z++) 
      for (i.y = 0; i.y < raw_data_info->volume->dim.y; i.y++) 
	for (i.x = 0; i.x < raw_data_info->volume->dim.x; i.x++) {
	  temp = VOLUME_CONTENTS(raw_data_info->volume, t, i);
	  if (temp > max)
	    max = temp;
	  else if (temp < min)
	    min = temp;
	}
#ifdef AMIDE_DEBUG
    g_print(".");
#endif
  }
  raw_data_info->volume->max = max;
  raw_data_info->volume->min = min;

#ifdef AMIDE_DEBUG
  g_print("\tmax %5.3f min %5.3f\n",max,min);
#endif

  /* garbage collection */
  g_free(file_buffer);
  return;
}

/* calculate the total amount of the file that will be read through */
guint raw_data_ui_num_bytes(raw_data_info_t * raw_data_info) {

  guint num_bytes;
  gchar * temp_string;
  
  num_bytes = raw_data_calc_num_bytes(raw_data_info)+raw_data_info->offset;
;

  if (raw_data_info->num_bytes_label != NULL) {
    temp_string = g_strdup_printf("%d",num_bytes);
    gtk_label_set_text(GTK_LABEL(raw_data_info->num_bytes_label), temp_string);
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
  volume_name = g_basename(raw_data_info->filename);
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
  raw_data_info->num_bytes_label = label;

  table_row++;


  /* what offset in the raw_data file we should start reading at */
  label = gtk_label_new("read offset (bytes)");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
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
  label = gtk_label_new("total bytes to read through:");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 3,4,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  /* how many bytes we're currently reading from the file */
  label = gtk_label_new("0");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 4,5,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  raw_data_info->num_bytes_label = label;

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


  /* display the number of bytes we're set to read */
  raw_data_ui_num_bytes(raw_data_info);

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
    g_warning("%s: couldn't allocate space for raw_data_info structure to load in RAW file\n", PACKAGE);
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
    g_warning("%s: couldn't allocate space for the volume structure to hold RAW data\n", PACKAGE);
    g_free(raw_data_info->filename);
    g_free(raw_data_info);
    return NULL;
  }


  /* create and run the raw_data_import_dialog to acquire needed info from the user */
  raw_data_import_dialog(raw_data_info);

  /* block till the user closes the dialog */
  dialog_reply = gnome_dialog_run(GNOME_DIALOG(raw_data_info->dialog));
  
  temp_volume=raw_data_info->volume;

  /* and start loading in the file if we hit ok*/
  if (dialog_reply == 0) {
    raw_data_read_file(raw_data_info);
  } else /* we hit the cancel button */
    temp_volume = volume_free(temp_volume);

  g_free(raw_data_info->filename);
  g_free(raw_data_info); /* note, we've saved a pointer to raw_data_info->volume in temp_volume */
  return temp_volume; /* and return the new volume structure (it's NULL if we cancelled) */

}










