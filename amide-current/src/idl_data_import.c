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
#include "volume.h"
#include <sys/stat.h>
#include "raw_data_import.h"
#include "idl_data_import.h"



/* function to load in volume data from an IDL data file */
volume_t * idl_data_import(gchar * idl_data_filename) {

  volume_t * idl_volume;
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
  guint file_offset = 35; /* the idl header is 35 bytes */
  data_format_t data_format = UBYTE; /* data type of idl files */

  /* acquire space for the volume structure */
  if ((idl_volume = volume_init()) == NULL) {
    g_warning("%s: couldn't allocate space for the volume structure to hold IDL data", PACKAGE);
    return idl_volume;
  }

  /* we need to read in the first 35 bytes of the IDL data file (the header) */
  if ((file_pointer = fopen(idl_data_filename, "r")) == NULL) {
    g_warning("%s: couldn't open idl data file %s", PACKAGE,idl_data_filename);
    idl_volume = volume_free(idl_volume);
    return idl_volume;
  }
    
  /* read in the header of the file */
  bytes_to_read = file_offset; /* this is the size of the header */
  file_buffer = (void *) g_malloc(bytes_to_read);
  bytes_read = fread(file_buffer, 1, bytes_to_read, file_pointer);
  if (bytes_read != bytes_to_read) {
    g_warning("%s: read wrong number of elements from idl data file:\n\t%s\n\texpected %d\tgot %d", 
	      PACKAGE,idl_data_filename, bytes_to_read, bytes_read);
    idl_volume = volume_free(idl_volume);
    g_free(file_buffer);
    fclose(file_pointer);
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
  g_print("IDL File %s\n",idl_data_filename);
  g_print("\tdimensions %d\n",dimensions);
  g_print("\tdim\tx %d\ty %d\tz %d\tframes %d\n",dim.x,dim.y,dim.z, num_frames);
#endif

  /* set the parameters of the volume structure */
  idl_volume->modality = CT; /* guess CT... */
  idl_volume->dim = dim;
  idl_volume->num_frames = num_frames;
  idl_volume->voxel_size.x = 1.0;
  idl_volume->voxel_size.y = 1.0;
  idl_volume->voxel_size.z = 1.0;
  REALPOINT_MULT(idl_volume->dim, idl_volume->voxel_size, idl_volume->corner);
  
  /* figure out an initial name for the data */
  volume_name = g_basename(idl_data_filename);
  /* remove the extension of the file */
  g_strreverse(volume_name);
  frags = g_strsplit(volume_name, ".", 2);
  g_strreverse(volume_name);
  volume_name = frags[1];
  g_strreverse(volume_name);
  volume_set_name(idl_volume,volume_name);
  g_strfreev(frags); /* free up now unused strings */

  /* now that we've figured out the header info, read in the rest of the idl data file */
  idl_volume = raw_data_read_file(idl_data_filename, idl_volume, data_format, file_offset);

  /* set the axis such that transverse/coronal/sagittal match up correctly */
  /* this is all derived empirically */
  idl_volume->coord_frame.axis[0] = default_axis[0];
  idl_volume->coord_frame.axis[1] = default_axis[2];
  idl_volume->coord_frame.axis[2] = default_axis[1];
  idl_volume->coord_frame.axis[2].x = -idl_volume->coord_frame.axis[2].x;
  idl_volume->coord_frame.axis[2].y = -idl_volume->coord_frame.axis[2].y;
  idl_volume->coord_frame.axis[2].z = -idl_volume->coord_frame.axis[2].z;
  idl_volume->coord_frame.offset.y = idl_volume->dim.z*idl_volume->voxel_size.z;

  /* garbage collection */
  g_free(file_buffer);

  return idl_volume; /* and return the new volume structure (it's NULL if we cancelled) */

}


