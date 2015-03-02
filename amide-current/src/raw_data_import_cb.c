/* raw_data_import_cb.c
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
#include "volume.h"
#include "raw_data_import.h"
#include "raw_data_import_cb.h"


/* function called when the name of the volume has been changed */
void raw_data_import_cb_change_name(GtkWidget * widget, gpointer data) {

  raw_data_info_t * raw_data_info = data;
  gchar * new_name;

  /* get the contents of the name entry box and save it */
  new_name = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  volume_set_name(raw_data_info->volume, new_name);
  g_free(new_name);

  return;
}

/* function called to change the scaling factor */
void raw_data_import_cb_change_scaling(GtkWidget * widget, gpointer data) {

  gchar * str;
  gint error;
  gdouble temp_real;
  raw_data_info_t * raw_data_info = data;

  /* get the contents of the name entry box */
  str = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert to the correct number */
  error = sscanf(str, "%lf", &temp_real);
  if (error == EOF)
    return; /* make sure it's a valid number */
  g_free(str);

  /* and save the value */
  volume_set_scaling(raw_data_info->volume, temp_real);

  return;
}
      

/* function called when a numerical entry of the volume has been changed, 
   used for the dimensions and voxel_size */
void raw_data_import_cb_change_entry(GtkWidget * widget, gpointer data) {

  gchar * str;
  gint error;
  gdouble temp_real=0.0;
  gint temp_int=0;
  guint which_widget;
  raw_data_info_t * raw_data_info = data;

  /* figure out which widget this is */
  which_widget = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "type")); 

  /* get the contents of the name entry box */
  str = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);


  /* convert to the correct number */
  if ((which_widget < NUM_DIMS) || (which_widget == NUM_DIMS+NUM_AXIS)) {
    error = sscanf(str, "%d", &temp_int);
    if (error == EOF)
      return; /* make sure it's a valid number */
    if (temp_int < 0)
      return;
  } else { /* one of the voxel_size widgets */
    error = sscanf(str, "%lf", &temp_real);
    if (error == EOF) /* make sure it's a valid number */
      return;
    if (temp_real < 0.0)
      return;
  }
  g_free(str);

  /* and save the value in our temporary volume structure */
  switch(which_widget) {
  case XDIM:
    raw_data_info->volume->data_set->dim.x = temp_int;
    break;
  case YDIM:
    raw_data_info->volume->data_set->dim.y = temp_int;
    break;
  case ZDIM:
    raw_data_info->volume->data_set->dim.z = temp_int;
    break;
  case TDIM:
    raw_data_info->volume->data_set->dim.t = temp_int;
    break;
  case (NUM_DIMS+XAXIS):
    raw_data_info->volume->voxel_size.x = temp_real;
    break;
  case (NUM_DIMS+YAXIS):
    raw_data_info->volume->voxel_size.y = temp_real;
    break;
  case (NUM_DIMS+ZAXIS):
    raw_data_info->volume->voxel_size.z = temp_real;
    break;
  case (NUM_DIMS+NUM_AXIS):
    raw_data_info->offset = temp_int;
    break;
  default:
    break; /* do nothing */
  }
  
  /* recalculate the total number of bytes to be read and have it displayed */
  raw_data_update_num_bytes(raw_data_info);

  return;
}
      
      

/* function to change the modality */
void raw_data_import_cb_change_modality(GtkWidget * widget, gpointer data) {
  
  raw_data_info_t * raw_data_info = data;
  modality_t i_modality;

  /* figure out which menu item called me */
  for (i_modality=0;i_modality<NUM_MODALITIES;i_modality++)       
    if (GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"modality")) == i_modality)
      raw_data_info->volume->modality = i_modality;  /* save the new modality */

  return;
}

/* function to change the raw data file's data format */
void raw_data_import_cb_change_raw_data_format(GtkWidget * widget, gpointer data) {

  raw_data_info_t * raw_data_info = data;
  raw_data_format_t i_raw_data_format;

  /* figure out which menu item called me */
  for (i_raw_data_format=0;i_raw_data_format<NUM_RAW_DATA_FORMATS;i_raw_data_format++)       
    if (GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"raw_data_format")) == i_raw_data_format)
      raw_data_info->raw_data_format = i_raw_data_format; /* save the new data format */

  /* recalculate the total number of bytes to be read and have it displayed*/
  raw_data_update_num_bytes(raw_data_info);

  /* update the offset label so it makes sense */
  raw_data_update_offset_label(raw_data_info);

  return;
}





