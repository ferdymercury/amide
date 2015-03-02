/* idl_data_import.c
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
#include "idl_data_import.h"



/* function to load in volume data from an IDL data file */
amide_volume_t * idl_data_import(gchar * idl_data_filename) {

  raw_data_info_t * idl_data_info;
  amide_volume_t * temp_volume;
  FILE * file_pointer;
  void * file_buffer = NULL;
  size_t bytes_read;
  size_t bytes_to_read;
  guint8 * bytes;
  guint32 * uints;
  guint num_frames;
  guint dimensions;
  voxelpoint_t dim;
  gchar * volume_name;
  gchar ** frags;

  /* get space for our raw_data_info structure */
  if ((idl_data_info = (raw_data_info_t * ) g_malloc(sizeof(raw_data_info_t))) == NULL) {
    g_warning("%s: couldn't allocate space for raw_data_info structure for loading in IDL file\n", PACKAGE);
    return NULL;
  }
  idl_data_info->volume = NULL;
  idl_data_info->filename = g_strdup(idl_data_filename);
  idl_data_info->total_file_size = 0; /* don't need this */
  idl_data_info->data_format = UBYTE;
  idl_data_info->offset = 35; /* the idl header is 35 bytes */

  /* acquire space for the volume structure */
  if ((idl_data_info->volume = volume_init()) == NULL) {
    g_warning("%s: couldn't allocate space for the volume structure to hold IDL data\n", PACKAGE);
    g_free(idl_data_info->filename);
    g_free(idl_data_info);
    return NULL;
  }

  /* we need to read in the first 35 bytes of the IDL data file (the header */
  /* open the file for reading */
  if ((file_pointer = fopen(idl_data_info->filename, "r")) == NULL) {
    g_warning("%s: couldn't open idl data file %s\n", PACKAGE,idl_data_info->filename);
    volume_free(&(idl_data_info->volume));
    g_free(idl_data_info->filename);
    g_free(idl_data_info);
    return NULL;
  }
    
  /* read in the header of the file */
  bytes_to_read = idl_data_info->offset; /* this is the size of the header */
  file_buffer = (void *) g_malloc(bytes_to_read);
  bytes_read = fread(file_buffer, 1, bytes_to_read, file_pointer );
  if (bytes_read != bytes_to_read) {
    g_warning("%s: read wrong number of elements from idl data file:\n\t%s\n\texpected %d\tgot %d\n", 
	      PACKAGE,idl_data_info->filename, bytes_to_read, bytes_read);
    volume_free(&(idl_data_info->volume));
    g_free(file_buffer);
    fclose(file_pointer);
    g_free(idl_data_info->filename);
    g_free(idl_data_info);
    return NULL;
  }
  fclose(file_pointer);

  /* interperate the header, note, it's in big endian format...

     bytes         content
     -------------------------------------
     0             number of dimensions
     1-4           x dimension
     5-8           y dimension
     9-12          z dimension
     13-16         number of frames?
     17-35         ?

  */
  bytes = file_buffer;

  dimensions = bytes[0];
  
  uints = file_buffer+1;

  dim.x = GUINT32_FROM_BE(uints[0]); /* bytes 1-4 */
  dim.y = GUINT32_FROM_BE(uints[1]); /* bytes 5-8 */
  dim.z = GUINT32_FROM_BE(uints[2]); /* bytes 9-12 */
  num_frames = GUINT32_FROM_BE(uints[3]); /* bytes 13-16 */

#ifdef AMIDE_DEBUG
  g_print("IDL File %s\n",idl_data_info->filename);
  g_print("\tdimensions %d\n",dimensions);
  g_print("\tdim\tx %d\ty %d\tz %d\tframes %d\n",dim.x,dim.y,dim.z, num_frames);
#endif

  /* set the parameters of the volume structure */
  idl_data_info->volume->modality = CT; /* guess CT... */
  idl_data_info->volume->dim = dim;
  idl_data_info->volume->num_frames = num_frames;
  idl_data_info->volume->voxel_size.x = 1.0;
  idl_data_info->volume->voxel_size.y = 1.0;
  idl_data_info->volume->voxel_size.z = 1.0;
  
  /* figure out an initial name for the data */
  volume_name = g_basename(idl_data_info->filename);
  /* remove the extension of the file */
  g_strreverse(volume_name);
  frags = g_strsplit(volume_name, ".", 2);
  g_strreverse(volume_name);
  volume_name = frags[1];
  g_strreverse(volume_name);
  volume_set_name(idl_data_info->volume,volume_name);
  g_strfreev(frags); /* free up now unused strings */

  /* now that we've figured out the header info, read in the rest of the idl data file */
  raw_data_read_file(idl_data_info);

  /* set the axis such that transverse/coronal/sagittal match up correctly */
  /* this is all derived empirically */
  idl_data_info->volume->coord_frame.axis[0] = default_axis[0];
  idl_data_info->volume->coord_frame.axis[1] = default_axis[2];
  idl_data_info->volume->coord_frame.axis[2] = default_axis[1];
  idl_data_info->volume->coord_frame.axis[2].x = -idl_data_info->volume->coord_frame.axis[2].x;
  idl_data_info->volume->coord_frame.axis[2].y = -idl_data_info->volume->coord_frame.axis[2].y;
  idl_data_info->volume->coord_frame.axis[2].z = -idl_data_info->volume->coord_frame.axis[2].z;
  idl_data_info->volume->coord_frame.offset.y = idl_data_info->volume->dim.z*idl_data_info->volume->voxel_size.z;

  /* garbage collection */
  temp_volume = idl_data_info->volume;
  g_free(file_buffer);
  g_free(idl_data_info->filename);
  g_free(idl_data_info); /* note, we've saved a pointer to raw_data_info->volume in temp_volume */

  return temp_volume; /* and return the new volume structure (it's NULL if we cancelled) */

}


