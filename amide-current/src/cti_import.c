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


#include "amide_config.h"
#ifdef AMIDE_LIBECAT_SUPPORT
#include <time.h>
#include <gnome.h>
#include <matrix.h>
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

AmitkDataSet * cti_import(const gchar * cti_filename, 
			  gboolean (*update_func)(),
			  gpointer update_data) {

  MatrixFile * cti_file=NULL;
  MatrixData * cti_subheader=NULL;
  MatrixData * cti_slice=NULL;
  AmitkVoxel i, j;
  gint matnum;
  AmitkDataSet * ds=NULL;
  guint slice, num_slices;
  gchar * temp_name;
  gchar ** frags=NULL;
  time_t scan_time;
  AmitkPoint temp_point;
  AmitkFormat format;
  AmitkVoxel dim;
  AmitkScalingType scaling_type;
  div_t x;
  gint divider;
  gint t_times_z;
  gboolean continue_work=TRUE;
  gchar * temp_string;

  if (!(cti_file = matrix_open(cti_filename, MAT_READ_ONLY, MAT_UNKNOWN_FTYPE))) {
    g_warning("can't open file %s", cti_filename);
    goto error;
  }

  /* check if we can handle the file type */
  switch(cti_file->mhptr->file_type) {
  case PetImage: /* i.e. CTI 6.4 */
  case PetVolume: /* i.e. CTI 7.0 */
  case AttenCor:
  case Sinogram:
  case InterfileImage:
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
  case ByteImage:
  case ByteVolume:
  default:
    g_warning("can't open this CTI file type: %d", cti_file->mhptr->file_type);
    goto error;
    break;
  }

  /* we always start at the first iamge */
  //  matnum = mat_numcod(1,1,1,0,0); /* frame, plane, gate, data, bed */
  matnum = cti_file->dirlist->first->matnum;

  if (!(cti_subheader = matrix_read(cti_file, matnum, MAT_SUB_HEADER))) {
    g_warning("can't get header info at matrix %x in file %s", matnum, cti_filename);
    goto error;
  }

  /* make sure we know how to process the data type */
  /* note that the libecat library handles endian issues */
  switch (cti_subheader->data_type) {
  case VAX_Ix2:  /* little endian short */
  case SunShort: /* big endian short */
    format = AMITK_FORMAT_SSHORT;
    break;
  case IeeeFloat: /* big endian float */
  case VAX_Rx4:  /* PDP float */
    format = AMITK_FORMAT_FLOAT;
    break; /* handled data types */
  case UnknownMatDataType:
  case ByteData: 
  case VAX_Ix4: /* little endian int */
  case SunLong: /* big endian int? */
  case ColorData:
  case BitData:
  default:
    g_warning("no support for importing CTI files with data type of: %d (%s)", 
	      cti_subheader->data_type,
	      cti_data_types[((cti_subheader->data_type) < NumMatrixDataTypes) ? 
			     cti_subheader->data_type : 0]);
    goto error;
    break;
  }

  /* start acquiring some useful information */
  dim.x = cti_subheader->xdim;
  dim.y = cti_subheader->ydim;
  dim.z = (cti_subheader->zdim == 1) ? cti_file->mhptr->num_planes : cti_subheader->zdim;
  dim.t = cti_file->mhptr->num_frames;
  num_slices = cti_file->mhptr->num_planes/cti_subheader->zdim;

  /* figure out the size of our factor array */
  if (cti_subheader->zdim == 1) {
    /* slice image, non-float data, so we'll need a per/plane (2D) array of scaling factors */
    scaling_type = AMITK_SCALING_TYPE_2D;
  } else { /* volume image, so we'll need a per/frame (1D) array of scaling factors */
    scaling_type = AMITK_SCALING_TYPE_1D;
  }

  /* init our data structures */
  ds = amitk_data_set_new_with_data(format, dim, scaling_type);
  if (ds == NULL) {
    g_warning("couldn't allocate space for the data set structure to hold CTI data");
    goto error;
  }
  
  /* it's a CTI File, we'll guess it's PET data */
  ds->modality = AMITK_MODALITY_PET;

  /* try figuring out the name */
  if (cti_file->mhptr->original_file_name != NULL) {
    amitk_object_set_name(AMITK_OBJECT(ds),cti_file->mhptr->original_file_name);
  } else {/* no original filename? */
    temp_name = g_strdup(g_basename(cti_filename));
    /* remove the extension of the file */
    g_strreverse(temp_name);
    frags = g_strsplit(temp_name, ".", 2);
    g_free(temp_name);
    g_strreverse(frags[1]);
    amitk_object_set_name(AMITK_OBJECT(ds),frags[1]);
    g_strfreev(frags); /* free up now unused strings */
  }
#ifdef AMIDE_DEBUG
  g_print("data set name %s\n",AMITK_OBJECT_NAME(ds));
  g_print("\tx size %d\ty size %d\tz size %d\tframes %d\n", dim.x, dim.y, dim.z, dim.t);
  g_print("\tslices/volume %d\n",num_slices);
	  
#endif
  
  /* try to enter in a scan date */
  scan_time = cti_file->mhptr->scan_start_time;
  amitk_data_set_set_scan_date(ds, ctime(&scan_time));

  /* get the voxel size */
  temp_point.x = 10*cti_subheader->pixel_size;
  temp_point.y = 10*cti_subheader->y_size;
  temp_point.z = 10*cti_subheader->z_size;
  if (isnan(temp_point.x) || isnan(temp_point.y) || isnan(temp_point.z)) {/*handle corrupted cti files */ 
    g_warning("dectected corrupted CTI file, will try to continue by guessing voxel_size");
    ds->voxel_size = one_point;
  } else if (EQUAL_ZERO(temp_point.x) || EQUAL_ZERO(temp_point.y) || EQUAL_ZERO(temp_point.z)) {
    g_warning("detected zero voxel size in CTI file, will try to continue by guessing voxel_size");
    ds->voxel_size = one_point;
  } else
    ds->voxel_size = temp_point;

  /* get the offset */
  temp_point.x = 10*(((Image_subheader*)cti_subheader->shptr)->x_offset);
  temp_point.y = 10*(((Image_subheader*)cti_subheader->shptr)->y_offset);
  temp_point.z = 10*(((Image_subheader*)cti_subheader->shptr)->z_offset);
  if (isnan(temp_point.x) || isnan(temp_point.y) || isnan(temp_point.z)) {    /*handle corrupted cti files */ 
    g_warning("detected corrupted CTI file, will try to continue by guessing offset");
    temp_point = zero_point;
  }
  amitk_space_set_offset(AMITK_SPACE(ds), temp_point);

  /* guess the start of the scan is the same as the start of the first frame of data */
  /* note, CTI files specify time as integers in msecs */
  /* check if we can handle the file type */
  switch(cti_file->mhptr->file_type) {
  case PetImage: 
  case PetVolume: 
  case InterfileImage:
    ds->scan_start = (((Image_subheader*)cti_subheader->shptr)->frame_start_time)/1000.0;
    break;
  case AttenCor:
    ds->scan_start = 0.0; /* doesn't mean anything */
    break;
  case Sinogram:
    ds->scan_start = (((Scan_subheader*)cti_subheader->shptr)->frame_start_time)/1000.0;
    break;
  default:
    break; /* should never get here */
  }
#ifdef AMIDE_DEBUG
  g_print("\tscan start time %5.3f\n",ds->scan_start);
#endif
  
  if (update_func != NULL) {
    temp_string = g_strdup_printf("Importing CTI File:\n   %s", cti_filename);
    continue_work = (*update_func)(update_data, temp_string, (gdouble) 0.0);
    g_free(temp_string);
  }
  t_times_z = dim.z*dim.t;
  divider = ((t_times_z/AMIDE_UPDATE_DIVIDER) < 1) ? 1 : (t_times_z/AMIDE_UPDATE_DIVIDER);


  /* and load in the data */
  for (i.t = 0; i.t < AMITK_DATA_SET_NUM_FRAMES(ds); i.t++) {
#ifdef AMIDE_DEBUG
    g_print("\tloading frame:\t%d",i.t);
#endif
    for (slice=0; (slice < num_slices) && (continue_work) ; slice++) {
      matnum=mat_numcod(i.t+1,slice+1,1,0,0);/* frame, plane, gate, data, bed */
      
      /* read in the corresponding cti slice */
      if ((cti_slice = matrix_read(cti_file, matnum, 0)) == NULL) {
	g_warning("can't get image matrix %x in file %s", matnum, cti_filename);
	goto error;
      }
      
      /* set the frame duration, note, CTI files specify time as integers in msecs */
      switch(cti_file->mhptr->file_type) {
      case PetImage: 
      case PetVolume: 
      case InterfileImage:
	ds->frame_duration[i.t] = (((Image_subheader*)cti_slice->shptr)->frame_duration)/1000.0;
	break;
      case AttenCor:
	ds->frame_duration[i.t] = 1.0; /* doesn't mean anything */
	break;
      case Sinogram:
	ds->frame_duration[i.t] = (((Scan_subheader*)cti_slice->shptr)->frame_duration)/1000.0;
	break;
      default:
	break; /* should never get here */
      }
      
      for (i.z = slice*(dim.z/num_slices); 
	   i.z < dim.z/num_slices + slice*(dim.z/num_slices); 
	   i.z++) {

	if (update_func != NULL) {
	  x = div(i.t*dim.z+i.z,divider);
	  if (x.rem == 0)
	    continue_work = (*update_func)(update_data, NULL, (gdouble) (i.z+i.t*dim.z)/t_times_z);
	}

	/* save the scale factor */
	j.x = j.y = 0;
	j.z = slice;
	j.t = i.t;
	if (scaling_type == AMITK_SCALING_TYPE_2D)
	  *AMITK_RAW_DATA_DOUBLE_2D_SCALING_POINTER(ds->internal_scaling, j) = cti_slice->scale_factor;
	else if (i.z == 0)  /* AMITK_SCALING_TYPE_1D */
	  *AMITK_RAW_DATA_DOUBLE_1D_SCALING_POINTER(ds->internal_scaling, j) = cti_slice->scale_factor;

	switch (format) {
	case AMITK_FORMAT_SSHORT:
	  /* copy the data into the volume */
	  /* note, we compensate here for the fact that we define 
	     our origin as the bottom left, not top left like the CTI file */
	  for (i.y = 0; i.y < dim.y; i.y++) 
	    for (i.x = 0; i.x < dim.x; i.x++)
	      AMITK_RAW_DATA_SSHORT_SET_CONTENT(ds->raw_data,i) =
		*(((amitk_format_SSHORT_t *) cti_slice->data_ptr) + 
		  (dim.y*dim.x*(i.z-slice*(dim.z/num_slices))
		   +dim.x*(dim.y-i.y-1)
		   +i.x));
	  break;
	case AMITK_FORMAT_FLOAT:
	  /* copy the data into the volume */
	  /* note, we compensate here for the fact that we define 
	     our origin as the bottom left, not top left like the CTI file */
	  for (i.y = 0; i.y < dim.y; i.y++) 
	    for (i.x = 0; i.x < dim.x; i.x++)
	      AMITK_RAW_DATA_FLOAT_SET_CONTENT(ds->raw_data,i) =
		*(((amitk_format_FLOAT_t *) cti_slice->data_ptr) + 
		  (dim.y*dim.x*(i.z-slice*(dim.z/num_slices))
		   +dim.x*(dim.y-i.y-1)
		   +i.x));
	default:
	  g_warning("unexpected case in %s at %d\n", __FILE__, __LINE__);
	  goto error;
	  break; 
	}

      }

      free_matrix_data(cti_slice);
    }
#ifdef AMIDE_DEBUG
    g_print("\tduration:\t%5.3f\n",ds->frame_duration[i.t]);
#endif
  }

  if (!continue_work) goto error;

  /* setup remaining volume parameters */
  amitk_data_set_set_scale_factor(ds, 1.0); /* set the external scaling factor */
  amitk_data_set_calc_far_corner(ds); /* set the far corner of the volume */
  amitk_data_set_calc_max_min(ds, update_func, update_data);
  goto function_end;



 error:

  if (ds != NULL) {
    g_object_unref(ds);
    ds = NULL;
  }



 function_end:

  if (update_func != NULL) /* remove progress bar */
    (*update_func)(update_data, NULL, (gdouble) 2.0); 

  if (cti_file != NULL)
    matrix_close(cti_file);

  if (cti_subheader != NULL)
    free_matrix_data(cti_subheader);



  return ds;

}


#endif








