/* raw_data.c
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

guint data_sizes[] = {1,1,1,2,2,4,4,4,8,2,2,4,4,4,8};



/* calculate the number of bytes that will be read from the file*/
guint raw_data_calc_num_bytes(voxelpoint_t dim, guint num_frames, data_format_t data_format) {

  return (dim.x * dim.y * dim.z * num_frames *  data_sizes[data_format]);

}

/* reads the contents of a raw data file into an amide volume data structure,
   note: raw_data_volume should be pre initialized with the appropriate dim 
   note: if raw_data_volume->frame_duration is null, bs values will be entered*/
volume_t * raw_data_read_file(gchar * file_name, 
			      volume_t * raw_data_volume,
			      data_format_t data_format,
			      guint file_offset) {

  FILE * file_pointer;
  void * file_buffer=NULL;
  size_t bytes_read;
  size_t bytes_to_read;
  volume_data_t max,min,temp;
  guint t;
  voxelpoint_t i;
  gboolean bullshit_frame_durations;

  if (raw_data_volume == NULL) {
    g_warning("%s: raw_data_read called with NULL volume\n",PACKAGE);
    return raw_data_volume;
  }
  
  /* malloc the space for the volume data */
  g_free(raw_data_volume->data); /* just in case */
  if ((raw_data_volume->data = volume_get_data_mem(raw_data_volume)) == NULL) {
    g_warning("%s: couldn't allocate space for the raw data volume\n",PACKAGE);
    raw_data_volume = volume_free(raw_data_volume);
    return raw_data_volume;
  }

  /* open the raw data file for reading */
  if ((file_pointer = fopen(file_name, "r")) == NULL) {
    g_warning("%s: couldn't open raw data file %s\n", PACKAGE,file_name);
    raw_data_volume = volume_free(raw_data_volume);
    return raw_data_volume;
  }
  
  /* jump forward by the given offset */
  if (fseek(file_pointer, file_offset, SEEK_SET) != 0) {
    g_warning("%s: could not seek forward %d bytes in raw data file:\n\t%s\n",
	      PACKAGE, file_offset, file_name);
    raw_data_volume = volume_free(raw_data_volume);
    fclose(file_pointer);
    return raw_data_volume;
  }
    
  /* read in the contents of the file */
  bytes_to_read = raw_data_calc_num_bytes(raw_data_volume->dim, 
					  raw_data_volume->num_frames, 
					  data_format);
  file_buffer = (void *) g_malloc(bytes_to_read);
  bytes_read = fread(file_buffer, 1, bytes_to_read, file_pointer );
  if (bytes_read != bytes_to_read) {
    g_warning("%s: read wrong number of elements from raw data file:\n\t%s\n\texpected %d\tgot %d\n", 
	      PACKAGE,file_name, bytes_to_read, bytes_read);
    raw_data_volume = volume_free(raw_data_volume);
    g_free(file_buffer);
    fclose(file_pointer);
    return raw_data_volume;
  }
  fclose(file_pointer);
  
  if (raw_data_volume->frame_duration == NULL) {
    bullshit_frame_durations = TRUE;
  
    /* allocate space for the array containing info on the duration of the frames */
    if ((raw_data_volume->frame_duration = volume_get_frame_duration_mem(raw_data_volume)) == NULL) {
      g_warning("%s: couldn't allocate space for the frame duration info\n",PACKAGE);
      raw_data_volume = volume_free(raw_data_volume);
      g_free(file_buffer);
      return raw_data_volume;
    }
  } else
    bullshit_frame_durations = FALSE;
  
  /* iterate over the # of frames */
  for (t = 0; t < raw_data_volume->num_frames; t++) {
#ifdef AMIDE_DEBUG
    g_print("\tloading frame %d\n",t);
#endif

    /* set the duration of this frame if needed */
    if (bullshit_frame_durations)
      raw_data_volume->frame_duration[t] = 1.0;
    
    /* and convert the data */
    switch (data_format) {
	
    case DOUBLE_BE: 
      {
	gdouble * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_volume,t,i) = 
		AMIDE_DOUBLE_FROM_BE(data[i.x + 
				    raw_data_volume->dim.x*i.y +
				    raw_data_volume->dim.x*raw_data_volume->dim.y*i.z +
				    raw_data_volume->dim.x*raw_data_volume->dim.y*raw_data_volume->dim.z*t]);
	
      } 
      break;
    case FLOAT_BE:
      {
	gfloat * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_volume,t,i) = 
		AMIDE_FLOAT_FROM_BE(data[i.x + 
				    raw_data_volume->dim.x*i.y +
				    raw_data_volume->dim.x*raw_data_volume->dim.y*i.z +
				    raw_data_volume->dim.x*raw_data_volume->dim.y*raw_data_volume->dim.z*t]);
	  
      }
      break;
    case SINT_BE:
      {
	gint32 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_volume,t,i) =
		GINT32_FROM_BE(data[i.x + 
				   raw_data_volume->dim.x*i.y +
				   raw_data_volume->dim.x*raw_data_volume->dim.y*i.z +
				   raw_data_volume->dim.x*raw_data_volume->dim.y*raw_data_volume->dim.z*t]);
	  
      }
      break;
    case UINT_BE:
      {
	guint32 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_volume,t,i) =
		GUINT32_FROM_BE(data[i.x + 
				   raw_data_volume->dim.x*i.y +
				   raw_data_volume->dim.x*raw_data_volume->dim.y*i.z +
				   raw_data_volume->dim.x*raw_data_volume->dim.y*raw_data_volume->dim.z*t]);

      }
      break;
    case SSHORT_BE:
      {
	gint16 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_volume,t,i) =
		GINT16_FROM_BE(data[i.x + 
				   raw_data_volume->dim.x*i.y +
				   raw_data_volume->dim.x*raw_data_volume->dim.y*i.z +
				   raw_data_volume->dim.x*raw_data_volume->dim.y*raw_data_volume->dim.z*t]);

      }
      break;
    case USHORT_BE:
      {
	guint16 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_volume,t,i) =
		GUINT16_FROM_BE(data[i.x + 
				   raw_data_volume->dim.x*i.y +
				   raw_data_volume->dim.x*raw_data_volume->dim.y*i.z +
				   raw_data_volume->dim.x*raw_data_volume->dim.y*raw_data_volume->dim.z*t]);

      }
      break;
    case DOUBLE_LE:
      {
	gdouble * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_volume,t,i) = 
		AMIDE_DOUBLE_FROM_LE(data[i.x + 
					 raw_data_volume->dim.x*i.y +
					 raw_data_volume->dim.x*raw_data_volume->dim.y*i.z +
					 raw_data_volume->dim.x*raw_data_volume->dim.y*raw_data_volume->dim.z*t]);

      }
      break;
    case FLOAT_LE:
      {
	gfloat * data=file_buffer;
	
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_volume,t,i) = 
		AMIDE_FLOAT_FROM_LE(data[i.x + 
					raw_data_volume->dim.x*i.y +
					raw_data_volume->dim.x*raw_data_volume->dim.y*i.z +
					raw_data_volume->dim.x*raw_data_volume->dim.y*raw_data_volume->dim.z*t]);
	
      }
      break;
    case SINT_LE:
      {
	gint32 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_volume,t,i) =
		GINT32_FROM_LE(data[i.x + 
				   raw_data_volume->dim.x*i.y +
				   raw_data_volume->dim.x*raw_data_volume->dim.y*i.z +
				   raw_data_volume->dim.x*raw_data_volume->dim.y*raw_data_volume->dim.z*t]);
	  
      }
      break;
    case UINT_LE:
      {
	guint32 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_volume,t,i) =
		GUINT32_FROM_LE(data[i.x + 
				    raw_data_volume->dim.x*i.y +
				    raw_data_volume->dim.x*raw_data_volume->dim.y*i.z +
				    raw_data_volume->dim.x*raw_data_volume->dim.y*raw_data_volume->dim.z*t]);

      }
      break;
    case SSHORT_LE:
      {
	gint16 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_volume,t,i) =
		GINT16_FROM_LE(data[i.x + 
				   raw_data_volume->dim.x*i.y +
				   raw_data_volume->dim.x*raw_data_volume->dim.y*i.z +
				   raw_data_volume->dim.x*raw_data_volume->dim.y*raw_data_volume->dim.z*t]);

      }
      break;
    case USHORT_LE:
      {
	guint16 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_volume,t,i) =
		GUINT16_FROM_LE(data[i.x + 
				    raw_data_volume->dim.x*i.y +
				    raw_data_volume->dim.x*raw_data_volume->dim.y*i.z +
				    raw_data_volume->dim.x*raw_data_volume->dim.y*raw_data_volume->dim.z*t]);

      }
      break;
    case SBYTE:
      {
	gint8 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_volume,t,i) =
		data[i.x + 
		    raw_data_volume->dim.x*i.y +
		    raw_data_volume->dim.x*raw_data_volume->dim.y*i.z +
		    raw_data_volume->dim.x*raw_data_volume->dim.y*raw_data_volume->dim.z*t];

      }
      break;
    case UBYTE:
    default:
      {
	guint8 * data=file_buffer;
	  
	/* copy this frame into the volume */
	for (i.z = 0; i.z < raw_data_volume->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(raw_data_volume,t,i) =
		data[i.x + 
		    raw_data_volume->dim.x*i.y +
		    raw_data_volume->dim.x*raw_data_volume->dim.y*i.z +
		    raw_data_volume->dim.x*raw_data_volume->dim.y*raw_data_volume->dim.z*t];
	  
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
  for(t = 0; t < raw_data_volume->num_frames; t++) {
    for (i.z = 0; i.z < raw_data_volume->dim.z; i.z++) 
      for (i.y = 0; i.y < raw_data_volume->dim.y; i.y++) 
	for (i.x = 0; i.x < raw_data_volume->dim.x; i.x++) {
	  temp = VOLUME_CONTENTS(raw_data_volume, t, i);
	  if (temp > max)
	    max = temp;
	  else if (temp < min)
	    min = temp;
	}
#ifdef AMIDE_DEBUG
    g_print(".");
#endif
  }
  raw_data_volume->max = max;
  raw_data_volume->min = min;

#ifdef AMIDE_DEBUG
  g_print("\tmax %5.3f min %5.3f\n",max,min);
#endif

  /* garbage collection */
  g_free(file_buffer);

  return raw_data_volume;
}
