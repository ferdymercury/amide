/* cti_import.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2002 Andy Loening
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
#ifdef AMIDE_LIBECAT_SUPPORT
#include <time.h>
#include <gnome.h>
#include <math.h>
#include <matrix.h>
#include "volume.h"
#include "cti_import.h"

static char * cti_data_types[] = {
  "Unknown Data Type",  /* UnknownMatDataType */
  "Byte", /* ByteData */
  "Short (16 bit), Little Endian", /* VAX_Ix2 */
  "Integer (32 bit), Little Endian", /* VAX_Ix4 */
  "VAX Float (32 bit)", /* VAX_Rx4 */
  "IEEE Float (32 bit)", /* IeeeFloat */
  "Short (16 bit), Big Endian", /* SunShort */
  "Integer (32 bit), Big Endian" /* SunLong */
}; /* NumMatrixDataTypes */

volume_t * cti_import(const gchar * cti_filename) {

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
  realpoint_t temp_rp;
  data_format_t data_format;

  if (!(cti_file = matrix_open(cti_filename, MAT_READ_ONLY, MAT_UNKNOWN_FTYPE))) {
    g_warning("can't open file %s", cti_filename);
    return NULL;
  }

  /* check if we can handle the file type */
  switch(cti_file->mhptr->file_type) {
  case PetImage: /* i.e. CTI 6.4 */
  case PetVolume: /* i.e. CTI 7.0 */
  case AttenCor:
  case Sinogram:
    break;
  case NoData:
  case Normalization:
  case PolarMap:
  case ByteProjection:
  case PetProjection: 
  case Short3dSinogram: 
  case Byte3dSinogram: 
  case Norm3d:
  case Float3dSinogram:
  case InterfileImage:
  case ByteImage:
  case ByteVolume:
  default:
    g_warning("can't open this CTI file type: %d", cti_file->mhptr->file_type);
    matrix_close(cti_file);
    return NULL;
  }

  /* we always start at the first iamge */
  //  matnum = mat_numcod(1,1,1,0,0); /* frame, plane, gate, data, bed */
  matnum = cti_file->dirlist->first->matnum;

  if (!(cti_subheader = matrix_read(cti_file, matnum, MAT_SUB_HEADER))) {
    g_warning("can't get header info at matrix %x in file %s",\
	      matnum, cti_filename);
    matrix_close(cti_file);
    return NULL;
  }

  /* make sure we know how to process the data type */
  /* note that the libecat library handles endian issues */
  switch (cti_subheader->data_type) {
  case VAX_Ix2:  /* little endian short */
  case SunShort: /* big endian short */
    data_format = SSHORT;
    break;
  case IeeeFloat: /* big endian float */
  case VAX_Rx4:  /* PDP float */
    data_format = FLOAT;
    break; /* handled data types */
  case UnknownMatDataType:
  case ByteData: 
  case VAX_Ix4: /* little endian int */
  case SunLong: /* big endian int? */
  case ColorData:
  case BitData:
  default:
    g_warning("no support for importing CTI files with data type of: %s", 
	      cti_data_types[((cti_subheader->data_type) < NumMatrixDataTypes) ? 
			     cti_subheader->data_type : 0]);
    matrix_close(cti_file);
    free_matrix_data(cti_subheader);
    return NULL;
  }

  /* init our data structures */
  if ((temp_volume = volume_init()) == NULL) {
    g_warning("couldn't allocate space for the volume structure to hold CTI data");
    matrix_close(cti_file);
    free_matrix_data(cti_subheader);
    return temp_volume;
  }
  temp_volume->data_set = data_set_init();
  temp_volume->coord_frame = rs_init();
  if ((temp_volume->data_set == NULL) ||(temp_volume->coord_frame == NULL)) {
    g_warning("couldn't allocate space for the data set structure/coord_frame to hold CTI data");
    matrix_close(cti_file);
    free_matrix_data(cti_subheader);
    return volume_unref(temp_volume);
  }

  /* start acquiring some useful information */
  temp_volume->data_set->format = data_format;
  temp_volume->data_set->dim.x = cti_subheader->xdim;
  temp_volume->data_set->dim.y = cti_subheader->ydim;
  temp_volume->data_set->dim.z = (cti_subheader->zdim == 1) ? 
    cti_file->mhptr->num_planes : cti_subheader->zdim;
  temp_volume->data_set->dim.t = cti_file->mhptr->num_frames;
  num_slices = cti_file->mhptr->num_planes/cti_subheader->zdim;

  /* figure out the size of our factor array */
  if ( data_format == FLOAT) { /* float image, so we don't need scaling factors */
    two_dim_scale = FALSE;
  } else if (cti_subheader->zdim == 1) {
    /* slice image, non-float data, so we'll need a 2D array of scaling factors */
    temp_volume->internal_scaling->dim.t = temp_volume->data_set->dim.t;
    temp_volume->internal_scaling->dim.z = temp_volume->data_set->dim.z;
    two_dim_scale = TRUE;
  } else { /* volume image, so we'll need a 1D array of scaling factors */
    temp_volume->internal_scaling->dim.t = temp_volume->data_set->dim.t;
    two_dim_scale = FALSE;
  }

  /* malloc the space for the scaling factors */
  g_free(temp_volume->internal_scaling->data); /* get rid of any old scaling factors */
  if ((temp_volume->internal_scaling->data = data_set_get_data_mem(temp_volume->internal_scaling)) == NULL) {
    g_warning("couldn't allocate space for the scaling factors for the CTI data");
    matrix_close(cti_file);
    free_matrix_data(cti_subheader);
    return volume_unref(temp_volume);
  }
  
  /* malloc the space for the volume */
  if ((temp_volume->data_set->data = data_set_get_data_mem(temp_volume->data_set)) == NULL) {
    g_warning("couldn't allocate space for the data set to hold CTI data");
    matrix_close(cti_file);
    free_matrix_data(cti_subheader);
    return volume_unref(temp_volume);
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

  /* get the voxel size */
  temp_rp.x = 10*cti_subheader->pixel_size;
  temp_rp.y = 10*cti_subheader->y_size;
  temp_rp.z = 10*cti_subheader->z_size;
  if (isnan(temp_rp.x) || isnan(temp_rp.y) || isnan(temp_rp.z)) {/*handle corrupted cti files */ 
    g_warning("dectected corrupted CTI file, will try to continue by guessing voxel_size");
    temp_volume->voxel_size = one_rp;
  } else if (FLOATPOINT_EQUAL(temp_rp.x, 0.0) || 
	     FLOATPOINT_EQUAL(temp_rp.y, 0.0) || 
	     FLOATPOINT_EQUAL(temp_rp.z, 0.0)) {
    g_warning("detected zero voxel size in CTI file, will try to continue by guessing voxel_size");
    temp_volume->voxel_size = one_rp;
  } else
    temp_volume->voxel_size = temp_rp;

  /* get the offset */
  temp_rp.x = 10*(((Image_subheader*)cti_subheader->shptr)->x_offset);
  temp_rp.y = 10*(((Image_subheader*)cti_subheader->shptr)->y_offset);
  temp_rp.z = 10*(((Image_subheader*)cti_subheader->shptr)->z_offset);
  if (isnan(temp_rp.x) || isnan(temp_rp.y) || isnan(temp_rp.z)) {    /*handle corrupted cti files */ 
    g_warning("detected corrupted CTI file, will try to continue by guessing offset");
    temp_rp = zero_rp;
  }
  volume_set_center(temp_volume, temp_rp);

  /* guess the start of the scan is the same as the start of the first frame of data */
  /* note, CTI files specify time as integers in msecs */
  /* check if we can handle the file type */
  switch(cti_file->mhptr->file_type) {
  case PetImage: 
  case PetVolume: 
    temp_volume->scan_start = (((Image_subheader*)cti_subheader->shptr)->frame_start_time)/1000.0;
    break;
  case AttenCor:
    temp_volume->scan_start = 0.0; /* doesn't mean anything */
    break;
  case Sinogram:
    temp_volume->scan_start = (((Scan_subheader*)cti_subheader->shptr)->frame_start_time)/1000.0;
    break;
  default:
    break; /* should never get here */
  }
#ifdef AMIDE_DEBUG
  g_print("\tscan start time %5.3f\n",temp_volume->scan_start);
#endif
  free_matrix_data(cti_subheader);
  

  /* allocate space for the array containing info on the duration of the frames */
  if ((temp_volume->frame_duration = volume_get_frame_duration_mem(temp_volume)) == NULL) {
    g_warning("couldn't allocate space for the frame duration info");
    matrix_close(cti_file);
    return volume_unref(temp_volume);
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
	g_warning("can't get image matrix %x in file %s",\
		  matnum, cti_filename);
	matrix_close(cti_file);
	return volume_unref(temp_volume);
      }
      
      /* set the frame duration, note, CTI files specify time as integers in msecs */
      switch(cti_file->mhptr->file_type) {
      case PetImage: 
      case PetVolume: 
	temp_volume->frame_duration[i.t] = (((Image_subheader*)cti_slice->shptr)->frame_duration)/1000.0;
	break;
      case AttenCor:
	temp_volume->frame_duration[i.t] = 1.0; /* doesn't mean anything */
	break;
      case Sinogram:
	temp_volume->frame_duration[i.t] = (((Scan_subheader*)cti_slice->shptr)->frame_duration)/1000.0;
	break;
      default:
	break; /* should never get here */
      }
      
      switch (data_format) {
      case SSHORT:
	
	/* save the scale factor */
	j.x = j.y = 0;
	j.z = slice;
	j.t = i.t;
	if (two_dim_scale)
	  *DATA_SET_FLOAT_2D_SCALING_POINTER(temp_volume->internal_scaling, j) = cti_slice->scale_factor;
	else if (slice == 0)
	  *DATA_SET_FLOAT_1D_SCALING_POINTER(temp_volume->internal_scaling, j) = cti_slice->scale_factor;
	
	/* copy the data into the volume */
	/* note, we compensate here for the fact that we define 
	   our origin as the bottom left, not top left like the CTI file */
	for (i.z = slice*(temp_volume->data_set->dim.z/num_slices); 
	     i.z < temp_volume->data_set->dim.z/num_slices + slice*(temp_volume->data_set->dim.z/num_slices); 
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
	break;
      case FLOAT:
	/* floating point data should use no scale factor */
	
	/* copy the data into the volume */
	/* note, we compensate here for the fact that we define 
	   our origin as the bottom left, not top left like the CTI file */
	for (i.z = slice*(temp_volume->data_set->dim.z/num_slices); 
	     i.z < temp_volume->data_set->dim.z/num_slices + slice*(temp_volume->data_set->dim.z/num_slices); 
	     i.z++) {
	  for (i.y = 0; i.y < temp_volume->data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < temp_volume->data_set->dim.x; i.x++)
	      DATA_SET_FLOAT_SET_CONTENT(temp_volume->data_set,i) =
		*(((data_set_FLOAT_t *) cti_slice->data_ptr) + 
		  (temp_volume->data_set->dim.y*temp_volume->data_set->dim.x*
		   (i.z-slice*(temp_volume->data_set->dim.z/num_slices))
		   +temp_volume->data_set->dim.x*(temp_volume->data_set->dim.y-i.y-1)
		   +i.x));
	}
      default:
	break; /* should never get here */
      }

      free_matrix_data(cti_slice);
    }
#ifdef AMIDE_DEBUG
    g_print("\tduration:\t%5.3f\n",temp_volume->frame_duration[i.t]);
#endif
  }

  /* garbage collection */
  matrix_close(cti_file);


  /* setup remaining volume parameters */
  volume_set_scaling(temp_volume, 1.0); /* set the external scaling factor */
  volume_recalc_far_corner(temp_volume); /* set the far corner of the volume */
  volume_calc_frame_max_min(temp_volume);
  volume_calc_global_max_min(temp_volume); 

  return temp_volume;
}


#endif








