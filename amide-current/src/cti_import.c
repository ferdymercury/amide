/* cti_import.c
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
#include "amide.h"
#ifdef AMIDE_CTI_SUPPORT
#include <matrix.h>
#include "realspace.h"
#include "volume.h"
#include "cti_import.h"

amide_volume_t * cti_import(gchar * cti_filename) {

  MatrixFile * cti_file;
  MatrixData * cti_subheader;
  MatrixData * cti_slice;
  voxelpoint_t i;
  gint matnum;
  amide_volume_t * temp_volume;
  guint t;
  volume_data_t max,min,temp;
  gchar * volume_name;
  gchar ** frags;

  if (!(cti_file = matrix_open(cti_filename, MAT_READ_ONLY, MAT_UNKNOWN_FTYPE))) {
    g_warning("%s: can't open file %s", PACKAGE, cti_filename);
    return NULL;
  }

  /* we always start at the first iamge */
  matnum = mat_numcod(1,1,1,0,0); /* frame, plane, gate, data, bed */

  if (!(cti_subheader = matrix_read(cti_file, matnum, MAT_SUB_HEADER))) {
    g_warning("%s: can't get header info at matrix %x in file %s\n",\
	      PACKAGE, matnum, cti_filename);
    return NULL;
  }

  /* start acquiring some useful information */
  if ((temp_volume = volume_init()) == NULL) {
    g_warning("%s: couldn't allocate space for the volume structure\n", PACKAGE);
    return NULL;
  }

  temp_volume->dim.x = cti_subheader->xdim;
  temp_volume->dim.y = cti_subheader->ydim;
  if (cti_subheader->zdim != 1)
    temp_volume->dim.z = cti_subheader->zdim;
  else
    temp_volume->dim.z = cti_file->mhptr->num_planes;
  temp_volume->num_frames = cti_file->mhptr->num_frames;
  
  /* malloc the space for the volume */
  temp_volume->data = 
    (volume_data_t *) g_malloc(temp_volume->num_frames *
			     temp_volume->dim.z*
			     temp_volume->dim.y*
			     temp_volume->dim.x*sizeof(volume_data_t));
  if (temp_volume->data == NULL) {
    g_warning("%s: couldn't allocate space for the volume\n",PACKAGE);
    volume_free(&temp_volume);
    return NULL;
  }

  /* it's a CTI File, we'll guess it's PET data */
  temp_volume->modality = PET;

  /* try figuring out the name */
  if (cti_file->mhptr->original_file_name != NULL)
    volume_name = cti_file->mhptr->original_file_name;
  else /* no original filename? */
    volume_name = g_basename(cti_filename);
  /* remove the extension of the file */
  g_strreverse(volume_name);
  frags = g_strsplit(volume_name, ".", 2);
  volume_name = frags[1];
  g_strreverse(volume_name);

  temp_volume->name = g_strdup_printf("%s",volume_name); /* allocates memory */

  switch(cti_file->mhptr->file_type) {
  case PetImage:

    /* set some more parameters */
        temp_volume->voxel_size.x = 10*
          ((Image_subheader*)cti_subheader->shptr)->x_pixel_size;
        temp_volume->voxel_size.y = 10*
          ((Image_subheader*)cti_subheader->shptr)->y_pixel_size;
        temp_volume->voxel_size.z = 10*
          ((Image_subheader*)cti_subheader->shptr)->z_pixel_size;

    /* and load in the data */
    for (t = 0; t < temp_volume->num_frames; t++) {
      for (i.z = 0; i.z < temp_volume->dim.z ; i.z++) {
	matnum=mat_numcod(t+1,i.z+1,1,0,0);/* frame, plane, gate, data, bed */

	if ((cti_slice = matrix_read(cti_file, matnum, 0)) == NULL) {
	  g_warning("%s: can't get image matrix %x in file %s\n",\
		    PACKAGE, matnum, cti_filename);
	  volume_free(&temp_volume);
	  return NULL;
	}

	/* copy the data into the volume */
	for (i.y = 0; i.y < temp_volume->dim.y; i.y++) 
	  for (i.x = 0; i.x < temp_volume->dim.x; i.x++)
	    VOLUME_SET_CONTENT(temp_volume,t,i) =
	      cti_slice->scale_factor *
	      *(((guint16 *) cti_slice->data_ptr) + 
	       (temp_volume->dim.x*i.y+i.x));
	/* that guint16 might have to be changed according to the data.... */
	free_matrix_data(cti_slice);
      }
    }
    break;
  case InterfileImage:
  case PetVolume:
  case ByteImage:
  case ByteVolume:
  case Sinogram:
  default:
    g_warning("%s: can't open this CTI file type", PACKAGE);
    volume_free(&temp_volume);
    return NULL;
  }

  /* set the far corner of the volume */
  temp_volume->corner.x = temp_volume->dim.x*temp_volume->voxel_size.x;
  temp_volume->corner.y = temp_volume->dim.y*temp_volume->voxel_size.y;
  temp_volume->corner.z = temp_volume->dim.z*temp_volume->voxel_size.z;

  /* set the max/min values in the volume */
  max = 0.0;
  min = 0.0;
  for(t = 0; t < temp_volume->num_frames; t++) 
    for (i.z = 0; i.z < temp_volume->dim.z; i.z++) 
      for (i.y = 0; i.y < temp_volume->dim.y; i.y++) 
	for (i.x = 0; i.x < temp_volume->dim.x; i.x++) {
	  temp = VOLUME_CONTENTS(temp_volume, t, i);
	  if (temp > max)
	    max = temp;
	  else if (temp < min)
	    min = temp;
	}
  temp_volume->max = max;
  temp_volume->min = min;

  free_matrix_data(cti_subheader);
  matrix_close(cti_file);

#ifdef AMIDE_DEBUG
  g_printerr("\tmax %5.3f min %5.3f\n",max,min);
#endif

  return temp_volume;
}


#endif








