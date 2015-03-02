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


#include <time.h>
#include "config.h"
#include <gnome.h>
#include "amide.h"
#ifdef AMIDE_LIBECAT_SUPPORT
#include <matrix.h>
#include "volume.h"
#include "cti_import.h"

static char * cti_data_types[] = {
        "Unknown Data Type", 
	"Byte", 
	"Short (16 bit), Little Endian", 
	"Integer (32 bit), Little Endian",
	"VAX Float (32 bit)",
	"IEEE Float (32 bit)",
	"Short (16 bit), Big Endian",
	"Integer (32 bit), Big Endian"};

volume_t * cti_import(gchar * cti_filename) {

  MatrixFile * cti_file;
  MatrixData * cti_subheader;
  MatrixData * cti_slice;
  voxelpoint_t i, j;
  gint matnum;
  volume_t * temp_volume;
  guint slice, num_slices;
  gchar * volume_name;
  gchar ** frags=NULL;
  time_t scan_time;
  gboolean two_dim_scale;

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
    matrix_close(cti_file);
    return NULL;
  }

  /* make sure we know how to process this */
  if (!((cti_subheader->data_type == VAX_Ix2) || (cti_subheader->data_type == SunShort))) {
    g_warning("%s: no support for importing CTI files with data type of: %s", PACKAGE, 
	      cti_data_types[((cti_subheader->data_type) < 8) ? cti_subheader->data_type : 0]);
    matrix_close(cti_file);
    free_matrix_data(cti_subheader);
    return NULL;
  }

  /* init our data structures */
  if ((temp_volume = volume_init()) == NULL) {
    g_warning("%s: couldn't allocate space for the volume structure to hold CTI data", PACKAGE);
    matrix_close(cti_file);
    free_matrix_data(cti_subheader);
    return temp_volume;
  }
  if ((temp_volume->data_set = data_set_init()) == NULL) {
    g_warning("%s: couldn't allocate space for the data set structure to hold CTI data", PACKAGE);
    matrix_close(cti_file);
    free_matrix_data(cti_subheader);
    return volume_free(temp_volume);
  }

  /* start acquiring some useful information */
  temp_volume->data_set->format = SSHORT;
  temp_volume->data_set->dim.x = cti_subheader->xdim;
  temp_volume->data_set->dim.y = cti_subheader->ydim;
  temp_volume->data_set->dim.t = cti_file->mhptr->num_frames;
  num_slices = cti_file->mhptr->num_planes/cti_subheader->zdim;
  if (cti_subheader->zdim != 1) {
    temp_volume->data_set->dim.z = cti_subheader->zdim;
    /* volume image, so we'll need a 1D array of scaling factors */
    temp_volume->internal_scaling->dim.t = temp_volume->data_set->dim.t;
    two_dim_scale = FALSE;
  } else {
    temp_volume->data_set->dim.z = cti_file->mhptr->num_planes;
    /* slice image, so we'll need a 2D array of scaling factors */
    temp_volume->internal_scaling->dim.t = temp_volume->data_set->dim.t;
    temp_volume->internal_scaling->dim.z = temp_volume->data_set->dim.z;
    two_dim_scale = TRUE;
  }

  /* malloc the space for the scaling factors */
  g_free(temp_volume->internal_scaling->data); /* get rid of any old scaling factors */
  if ((temp_volume->internal_scaling->data = data_set_get_data_mem(temp_volume->internal_scaling)) == NULL) {
    g_warning("%s: couldn't allocate space for the scaling factors for the CTI data",PACKAGE);
    matrix_close(cti_file);
    free_matrix_data(cti_subheader);
    return volume_free(temp_volume);
  }
  
  /* malloc the space for the volume */
  if ((temp_volume->data_set->data = data_set_get_data_mem(temp_volume->data_set)) == NULL) {
    g_warning("%s: couldn't allocate space for the data set to hold CTI data",PACKAGE);
    matrix_close(cti_file);
    free_matrix_data(cti_subheader);
    return volume_free(temp_volume);
  }


  /* it's a CTI File, we'll guess it's PET data */
  temp_volume->modality = PET;

  /* try figuring out the name */
  if (cti_file->mhptr->original_file_name != NULL) {
    volume_name = cti_file->mhptr->original_file_name;
    volume_set_name(temp_volume,volume_name);
  } else {/* no original filename? */
    volume_name = g_strdup(g_basename(cti_filename));
    /* remove the extension of the file */
    g_strreverse(volume_name);
    frags = g_strsplit(volume_name, ".", 2);
    volume_name = frags[1];
    volume_set_name(temp_volume,volume_name);
    g_strreverse(volume_name);
    g_strfreev(frags); /* free up now unused strings */
    g_free(volume_name);
  }
#ifdef AMIDE_DEBUG
  g_print("volume name %s\n",temp_volume->name);
  g_print("\tx size %d\ty size %d\tz size %d\tframes %d\n", 
	  temp_volume->data_set->dim.x, temp_volume->data_set->dim.y, 
	  temp_volume->data_set->dim.z, temp_volume->data_set->dim.t);
  g_print("\tslices/volume %d\n",num_slices);
	  
#endif
  
  /* try to enter in a scan date */
  scan_time = cti_file->mhptr->scan_start_time;
  volume_set_scan_date(temp_volume, ctime(&scan_time));
  g_strdelimit(temp_volume->scan_date, "\n", ' '); /* turns newlines to white space */
  g_strstrip(temp_volume->scan_date); /* removes trailing and leading white space */

  switch(cti_file->mhptr->file_type) {
  case PetImage: /* i.e. CTI 6.4 */
  case PetVolume: /* i.e. CTI 7.0 */

    /* set some more parameters */
    temp_volume->voxel_size.x = 10*((Image_subheader*)cti_subheader->shptr)->x_pixel_size;
    temp_volume->voxel_size.y = 10*((Image_subheader*)cti_subheader->shptr)->y_pixel_size;
    temp_volume->voxel_size.z = 10*((Image_subheader*)cti_subheader->shptr)->z_pixel_size;



    /* guess the start of the scan is the same as the start of the first frame of data */
    /* note, CTI files specify time as integers in msecs */
    temp_volume->scan_start =  (((Image_subheader*)cti_subheader->shptr)->frame_start_time)/1000;
#ifdef AMIDE_DEBUG
    g_print("\tscan start time %5.3f\n",temp_volume->scan_start);
#endif


    /* allocate space for the array containing info on the duration of the frames */
    if ((temp_volume->frame_duration = volume_get_frame_duration_mem(temp_volume)) == NULL) {
      g_warning("%s: couldn't allocate space for the frame duration info",PACKAGE);
      matrix_close(cti_file);
      free_matrix_data(cti_subheader);
      return volume_free(temp_volume);
    }

    /* and load in the data */
    for (i.t = 0; i.t < temp_volume->data_set->dim.t; i.t++) {
#ifdef AMIDE_DEBUG
      g_print("\tloading frame:\t%d",i.t);
#endif
      for (slice=0; slice < num_slices ; slice++) {
	matnum=mat_numcod(i.t+1,slice+1,1,0,0);/* frame, plane, gate, data, bed */

	/* read in the corresponding cti slice */
	if ((cti_slice = matrix_read(cti_file, matnum, 0)) == NULL) {
	  g_warning("%s: can't get image matrix %x in file %s",\
		    PACKAGE, matnum, cti_filename);
	  matrix_close(cti_file);
	  free_matrix_data(cti_subheader);
	  return volume_free(temp_volume);
	}

	/* set the frame duration, note, CTI files specify time as integers in msecs */
	temp_volume->frame_duration[i.t] = (((Image_subheader*)cti_subheader->shptr)->frame_duration)/1000;

	/* save the scale factor */
	j.x = j.y = 0;
	j.z = slice;
	j.t = i.t;
	if (two_dim_scale)
	  *DATA_SET_FLOAT_2D_SCALING_POINTER(temp_volume->internal_scaling, j) = cti_slice->scale_factor;
	else if (slice == 0)
	  *DATA_SET_FLOAT_1D_SCALING_POINTER(temp_volume->internal_scaling, j) = cti_slice->scale_factor;

	/* copy the data into the volume */
	/* note, we compensate here for the fact that we define our origin as the bottom left,
	   not top left like the CTI file */
	for (i.z = slice*(temp_volume->data_set->dim.z/num_slices); 
	     i.z < temp_volume->data_set->dim.z/num_slices + slice*(temp_volume->data_set->dim.z/num_slices) ; 
	     i.z++) {
	  for (i.y = 0; i.y < temp_volume->data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < temp_volume->data_set->dim.x; i.x++)
	      DATA_SET_SSHORT_SET_CONTENT(temp_volume->data_set,i) =
		*(((data_set_SSHORT_t *) cti_slice->data_ptr) + 
		  (temp_volume->data_set->dim.y*temp_volume->data_set->dim.x*
		   (i.z-slice*(temp_volume->data_set->dim.z/num_slices))
		   +temp_volume->data_set->dim.x*(temp_volume->data_set->dim.y-i.y-1)
		   +i.x));
	}
	/* that giint16 might have to be changed according to the data.... */
	free_matrix_data(cti_slice);
      }
#ifdef AMIDE_DEBUG
      g_print("\tduration:\t%5.3f\n",temp_volume->frame_duration[i.t]);
#endif
    }
    break;
  case InterfileImage:
  case ByteImage:
  case ByteVolume:
  case Sinogram:
  default:
    g_warning("%s: can't open this CTI file type", PACKAGE);
    matrix_close(cti_file);
    free_matrix_data(cti_subheader);
    return volume_free(temp_volume);
  }

  /* garbage collection */
  free_matrix_data(cti_subheader);
  matrix_close(cti_file);

  /* setup remaining volume parameters */
  volume_set_scaling(temp_volume, 1.0); /* set the external scaling factor */
  volume_recalc_far_corner(temp_volume); /* set the far corner of the volume */
  volume_recalc_max_min(temp_volume); /* set the max/min values in the volume */

  return temp_volume;
}


#endif








