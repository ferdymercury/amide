/* cti_import.c
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
#ifdef AMIDE_LIBECAT_SUPPORT
#include <matrix.h>
#include "volume.h"
#include "cti_import.h"

volume_t * cti_import(gchar * cti_filename) {

  MatrixFile * cti_file;
  MatrixData * cti_subheader;
  MatrixData * cti_slice;
  voxelpoint_t i;
  gint matnum;
  volume_t * temp_volume;
  guint t, slice, num_slices;
  volume_data_t max,min,temp;
  gchar * volume_name;
  gchar ** frags=NULL;

  if (!(cti_file = matrix_open(cti_filename, MAT_READ_ONLY, MAT_UNKNOWN_FTYPE))) {
    g_warning("%s: can't open file %s", PACKAGE, cti_filename);
    return NULL;
  }

  /* we always start at the first iamge */
  //  matnum = mat_numcod(1,1,1,0,0); /* frame, plane, gate, data, bed */
  matnum = cti_file->dirlist->first->matnum;

  if (!(cti_subheader = matrix_read(cti_file, matnum, MAT_SUB_HEADER))) {
    g_warning("%s: can't get header info at matrix %x in file %s",\
	      PACKAGE, matnum, cti_filename);
    return NULL;
  }

  /* start acquiring some useful information */
  if ((temp_volume = volume_init()) == NULL) {
    g_warning("%s: couldn't allocate space for the volume structure", PACKAGE);
    return NULL;
  }

  volume_set_dim_x(temp_volume,cti_subheader->xdim);
  volume_set_dim_y(temp_volume,cti_subheader->ydim);
  if (cti_subheader->zdim != 1)
    volume_set_dim_z(temp_volume,cti_subheader->zdim);
  else
    volume_set_dim_z(temp_volume,cti_file->mhptr->num_planes);
  temp_volume->num_frames = cti_file->mhptr->num_frames;
  num_slices = cti_file->mhptr->num_planes/cti_subheader->zdim;
  
  /* malloc the space for the volume */
  if ((temp_volume->data = volume_get_data_mem(temp_volume)) == NULL) {
    g_warning("%s: couldn't allocate space for the volume",PACKAGE);
    temp_volume = volume_free(temp_volume);
    return temp_volume;
  }


  /* it's a CTI File, we'll guess it's PET data */
  temp_volume->modality = PET;

  /* try figuring out the name */
  if (cti_file->mhptr->original_file_name != NULL) {
    volume_name = cti_file->mhptr->original_file_name;
    volume_set_name(temp_volume,volume_name);
  } else {/* no original filename? */
    volume_name = g_basename(cti_filename);
    /* remove the extension of the file */
    g_strreverse(volume_name);
    frags = g_strsplit(volume_name, ".", 2);
    volume_name = frags[1];
    volume_set_name(temp_volume,volume_name);
    g_strreverse(volume_name);
    g_strfreev(frags); /* free up now unused strings */
  }
#ifdef AMIDE_DEBUG
  g_print("volume name %s\n",volume_name);
  g_print("\tx size %d\ty size %d\tz size %d\tframes %d\n", 
	  temp_volume->dim.x, temp_volume->dim.y, temp_volume->dim.z, temp_volume->num_frames);
  g_print("\tslices/volume %d\n",num_slices);
	  
#endif

  switch(cti_file->mhptr->file_type) {
  case PetImage: /* i.e. CTI 6.4 */
  case PetVolume: /* i.e. CTI 7.0 */

    /* set some more parameters */
    temp_volume->voxel_size.x = 10*
      ((Image_subheader*)cti_subheader->shptr)->x_pixel_size;
    temp_volume->voxel_size.y = 10*
      ((Image_subheader*)cti_subheader->shptr)->y_pixel_size;
    temp_volume->voxel_size.z = 10*
      ((Image_subheader*)cti_subheader->shptr)->z_pixel_size;



    /* guess the start of the scan is the same as the start of the first frame of data */
    /* note, CTI files specify time as integers in msecs */
    temp_volume->scan_start = 
      (((Image_subheader*)cti_subheader->shptr)->frame_start_time)/1000;
#ifdef AMIDE_DEBUG
    g_print("\tscan start time %5.3f\n",temp_volume->scan_start);
#endif


    /* allocate space for the array containing info on the duration of the frames */
    if ((temp_volume->frame_duration = volume_get_frame_duration_mem(temp_volume)) == NULL) {
      g_warning("%s: couldn't allocate space for the frame duration info",PACKAGE);
      temp_volume = volume_free(temp_volume);
      return temp_volume;
    }

    /* and load in the data */
    for (t = 0; t < temp_volume->num_frames; t++) {
#ifdef AMIDE_DEBUG
      g_print("\tloading frame:\t%d",t);
#endif
      for (slice=0; slice < num_slices ; slice++) {
	matnum=mat_numcod(t+1,slice+1,1,0,0);/* frame, plane, gate, data, bed */

	/* read in the corresponding cti slice */
	if ((cti_slice = matrix_read(cti_file, matnum, 0)) == NULL) {
	  g_warning("%s: can't get image matrix %x in file %s",\
		    PACKAGE, matnum, cti_filename);
	  temp_volume = volume_free(temp_volume);
	  return temp_volume;
	}

	/* set the frame duration, note, CTI files specify time as integers in msecs */
	temp_volume->frame_duration[t] = 
	  (((Image_subheader*)cti_subheader->shptr)->frame_duration)/1000;

	/* copy the data into the volume */
	for (i.z = slice*(temp_volume->dim.z/num_slices); 
	     i.z < temp_volume->dim.z/num_slices + slice*(temp_volume->dim.z/num_slices) ; 
	     i.z++)
	  for (i.y = 0; i.y < temp_volume->dim.y; i.y++) 
	    for (i.x = 0; i.x < temp_volume->dim.x; i.x++)
	      VOLUME_SET_CONTENT(temp_volume,t,i) =
		cti_slice->scale_factor *
		*(((gint16 *) cti_slice->data_ptr) + 
		  (temp_volume->dim.y*temp_volume->dim.x*(i.z-slice*(temp_volume->dim.z/num_slices))
		   +temp_volume->dim.x*i.y+i.x));
	/* that guint16 might have to be changed according to the data.... */
	free_matrix_data(cti_slice);
      }
#ifdef AMIDE_DEBUG
      g_print("\tduration:\t%5.3f\n",temp_volume->frame_duration[t]);
#endif
    }
    break;
  case InterfileImage:
  case ByteImage:
  case ByteVolume:
  case Sinogram:
  default:
    g_warning("%s: can't open this CTI file type", PACKAGE);
    temp_volume = volume_free(temp_volume);
    return temp_volume;
  }

  /* set the far corner of the volume */
  REALPOINT_MULT(temp_volume->dim, temp_volume->voxel_size, temp_volume->corner);

  /* set the max/min values in the volume */
#ifdef AMIDE_DEBUG
  g_print("\tcalculating max & min");
#endif
  max = 0.0;
  min = 0.0;
  for(t = 0; t < temp_volume->num_frames; t++) {
    for (i.z = 0; i.z < temp_volume->dim.z; i.z++) 
      for (i.y = 0; i.y < temp_volume->dim.y; i.y++) 
	for (i.x = 0; i.x < temp_volume->dim.x; i.x++) {
	  temp = VOLUME_CONTENTS(temp_volume, t, i);
	  if (temp > max)
	    max = temp;
	  else if (temp < min)
	    min = temp;
	}
#ifdef AMIDE_DEBUG
  g_print(".");
#endif
  }
  temp_volume->max = max;
  temp_volume->min = min;

#ifdef AMIDE_DEBUG
  g_print("\tmax %5.3f min %5.3f\n",max,min);
#endif

  free_matrix_data(cti_subheader);
  matrix_close(cti_file);


  return temp_volume;
}


#endif








