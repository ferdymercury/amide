/* libecat_interface.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2014 Andy Loening
 *
 * Author: Andy Loening <loening@alum.mit.edu>
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
#include <matrix.h>
#include <locale.h>
#include "libecat_interface.h"

static char * libecat_data_types[] = {
  N_("Unknown Data Type"),  /* UnknownMatDataType */
  N_("Byte"), /* ByteData */
  N_("Short (16 bit), Little Endian"), /* VAX_Ix2 */
  N_("Integer (32 bit), Little Endian"), /* VAX_Ix4 */
  N_("VAX Float (32 bit)"), /* VAX_Rx4 */
  N_("IEEE Float (32 bit)"), /* IeeeFloat */
  N_("Short (16 bit), Big Endian"), /* SunShort */
  N_("Integer (32 bit), Big Endian") /* SunLong */
}; /* NumMatrixDataTypes */

AmitkDataSet * libecat_import(const gchar * libecat_filename, 
			      AmitkPreferences * preferences,
			      AmitkUpdateFunc update_func,
			      gpointer update_data) {

  MatrixFile * libecat_file=NULL;
  MatrixData * matrix_data=NULL;
  MatrixData * matrix_slice=NULL;
  AmitkVoxel i, j;
  gint matnum;
  AmitkDataSet * ds=NULL;
  guint slice, num_slices;
  gchar * name;
  gchar ** frags=NULL;
  time_t scan_time;
  AmitkPoint temp_point;
  AmitkPoint offset_rp;
  AmitkFormat format;
  AmitkVoxel dim;
  AmitkScalingType scaling_type;
  div_t x;
  gint divider;
  gint total_planes, i_plane;
  gboolean continue_work=TRUE;
  gchar * temp_string;
  const gchar * bad_char;
  Image_subheader * ish;
  Scan_subheader * ssh;
  Attn_subheader * ash;
  gchar * saved_time_locale;
  gchar * saved_numeric_locale;
  time_t dob;
  gint num_corrupted_planes = 0;
  gdouble calibration_factor = 1.0;
  
  saved_time_locale = g_strdup(setlocale(LC_TIME,NULL));
  saved_numeric_locale = g_strdup(setlocale(LC_NUMERIC,NULL));
  setlocale(LC_TIME,"POSIX");  
  setlocale(LC_NUMERIC,"POSIX");  


  if (!(libecat_file = matrix_open(libecat_filename, MAT_READ_ONLY, MAT_UNKNOWN_FTYPE))) {
    g_warning(_("Can't open file %s using libecat"), libecat_filename);
    goto error;
  }

  /* check if we can handle the file type */
  switch(libecat_file->mhptr->file_type) {
  case PetImage: /* i.e. CTI 6.4 */
  case PetVolume: /* i.e. CTI 7.0 */
  case AttenCor:
  case Sinogram:
  case Normalization:
  case InterfileImage:
    break;
  case NoData:
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
    g_warning(_("Don't know how to handle this CTI file type: %d"), libecat_file->mhptr->file_type);
    goto error;
    break;
  }

  /* we always start at the first iamge */
  //  matnum = mat_numcod(1,1,1,0,0); /* frame, plane, gate, data, bed */
  matnum = libecat_file->dirlist->first->matnum;

  if (!(matrix_data = matrix_read(libecat_file, matnum, MAT_SUB_HEADER))) {
    g_warning(_("Libecat can't get header info at matrix %x in file %s"), matnum, libecat_filename);
    goto error;
  }

  /* make sure we know how to process the data type */
  /* note that the libecat library handles endian issues */
  switch (matrix_data->data_type) {
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
    g_warning(_("No support for importing CTI files with data type of: %d (%s)"), 
	      matrix_data->data_type,
	      libecat_data_types[((matrix_data->data_type) < NumMatrixDataTypes) ? 
				 matrix_data->data_type : 0]);
    goto error;
    break;
  }

  /* start acquiring some useful information */
  dim.x = matrix_data->xdim;
  dim.y = matrix_data->ydim;
  dim.z = (matrix_data->zdim == 1) ? libecat_file->mhptr->num_planes : matrix_data->zdim;
  dim.g = libecat_file->mhptr->num_gates;
  dim.t = libecat_file->mhptr->num_frames;
  num_slices = libecat_file->mhptr->num_planes/matrix_data->zdim;

  /* should we include the calibration factor in the conversion? */
  if (libecat_file->mhptr->calibration_units == Calibrated)
    calibration_factor = libecat_file->mhptr->calibration_factor;

  /* figure out the size of our factor array */
  if (matrix_data->zdim == 1) {
    /* slice image, non-float data, so we'll need a per/plane (2D) array of scaling factors */
    scaling_type = AMITK_SCALING_TYPE_2D;
  } else { /* volume image, so we'll need a per/frame (1D) array of scaling factors */
    scaling_type = AMITK_SCALING_TYPE_1D;
  }

  /* init our data structures, it's a CTI File, we'll guess it's PET data */
  ds = amitk_data_set_new_with_data(preferences, AMITK_MODALITY_PET,
				    format, dim, scaling_type);
  if (ds == NULL) {
    g_warning(_("Couldn't allocate memory space for the data set structure to hold CTI data"));
    goto error;
  }
  
  /* try figuring out the name */
  if (libecat_file->mhptr->study_name[0] != '\0')
    name = g_strndup(libecat_file->mhptr->study_name, 12);
  else if (libecat_file->mhptr->original_file_name[0] != '\0')
    name = g_strndup(libecat_file->mhptr->original_file_name, 32);
  else {/* no original filename? */
    temp_string = g_path_get_basename(libecat_filename);
    /* remove the extension of the file */
    g_strreverse(temp_string);
    frags = g_strsplit(temp_string, ".", 2);
    g_free(temp_string);
    g_strreverse(frags[1]);
    name = g_strdup(frags[1]);
    g_strfreev(frags); /* free up now unused strings */
  }

  /* validate the name to utf8 */
  if (!g_utf8_validate(name, -1, &bad_char)) {
    gsize invalid_point = bad_char-name;
    name[invalid_point] = '\0';
  }


  /* try adding on the reconstruction method */
  switch(libecat_file->mhptr->file_type) {
  case PetImage: 
  case PetVolume: 
  case InterfileImage:
    ish = (Image_subheader *) matrix_data->shptr;
    temp_string = name;
    if (ish->annotation[0] != '\0')
      name = g_strdup_printf("%s - %s", temp_string, ish->annotation);
    else
      name = g_strdup_printf("%s, %d", temp_string, ish->recon_type);
    g_free(temp_string);
    break;
  case AttenCor:
    ash = (Attn_subheader *) matrix_data->shptr;
    temp_string = name;
    name = g_strdup_printf("%s,%d", temp_string, ash->attenuation_type);
    g_free(temp_string);
    break;
  case Sinogram:
  default:
    break; 
  }


  amitk_object_set_name(AMITK_OBJECT(ds),name);
#ifdef AMIDE_DEBUG
  g_print("data set name %s\n",AMITK_OBJECT_NAME(ds));
  g_print("\tx size %d\ty size %d\tz size %d\tframes %d\n", dim.x, dim.y, dim.z, dim.t);
  g_print("\tslices/volume %d\n",num_slices);
	  
#endif


  
  /* try to enter in a scan date and addition data */
  scan_time = libecat_file->mhptr->scan_start_time;
  amitk_data_set_set_scan_date(ds, ctime(&scan_time));
  amitk_data_set_set_subject_name(ds, libecat_file->mhptr->patient_name);
  amitk_data_set_set_subject_id(ds, libecat_file->mhptr->patient_id);
  dob = libecat_file->mhptr->patient_birth_date;
  amitk_data_set_set_subject_dob(ds, ctime(&(dob)));

  switch(libecat_file->mhptr->patient_orientation) {
  case FeetFirstProne:
    amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_PRONE_FEETFIRST);
    break;
  case HeadFirstProne:
    amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_PRONE_HEADFIRST);
    break;
  case FeetFirstSupine: 
    amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_SUPINE_FEETFIRST);
    break;
  case HeadFirstSupine:
    amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_SUPINE_HEADFIRST);
    break;
  case FeetFirstRight: 
    amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_RIGHT_DECUBITUS_FEETFIRST);
    break;
  case HeadFirstRight:
    amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_RIGHT_DECUBITUS_HEADFIRST);
    break;
  case FeetFirstLeft:
    amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_LEFT_DECUBITUS_FEETFIRST);
    break;
  case HeadFirstLeft:
    amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_LEFT_DECUBITUS_HEADFIRST);
    break;
  case UnknownOrientation:
  default:
    amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_UNKNOWN);
    break;
  }

  /* get the voxel size */
  temp_point.x = 10*matrix_data->pixel_size;
  temp_point.y = 10*matrix_data->y_size;
  temp_point.z = 10*matrix_data->z_size;
  if (isnan(temp_point.x) || isnan(temp_point.y) || isnan(temp_point.z)) {/*handle corrupted cti files */ 
    g_warning(_("Detected corrupted CTI file, will try to continue by guessing voxel_size"));
    ds->voxel_size = one_point;
  } else if (EQUAL_ZERO(temp_point.x) || EQUAL_ZERO(temp_point.y) || EQUAL_ZERO(temp_point.z)) {
    g_warning(_("Detected zero voxel size in CTI file, will try to continue by guessing voxel_size"));
    ds->voxel_size = one_point;
  } else
    ds->voxel_size = temp_point;

  offset_rp.x = 10*matrix_data->x_origin;
  offset_rp.y = 10*matrix_data->y_origin;
  offset_rp.z = 10*matrix_data->z_origin;

  if (isnan(offset_rp.x) || isnan(offset_rp.y) || isnan(offset_rp.z)) {    /*handle corrupted cti files */ 
    g_warning(_("Detected corrupted CTI file, will try to continue by guessing offset"));
    offset_rp = zero_point;
  }

  /* guess the start of the scan is the same as the start of the first frame of data */
  /* note, CTI files specify time as integers in msecs */
  /* check if we can handle the file type */
  switch(libecat_file->mhptr->file_type) {
  case PetImage: 
  case PetVolume: 
  case InterfileImage:
    ish =  (Image_subheader *) matrix_data->shptr;
    ds->scan_start = ish->frame_start_time/1000.0;
    break;
  case AttenCor:
    ds->scan_start = 0.0; /* doesn't mean anything */
    break;
  case Sinogram:
    ssh = (Scan_subheader *) matrix_data->shptr;
    ds->scan_start = ssh->frame_start_time/1000.0;
    break;
  default:
    break; /* should never get here */
  }
#ifdef AMIDE_DEBUG
  g_print("\tscan start time %5.3f\n",ds->scan_start);
#endif
  
  if (update_func != NULL) {
    temp_string = g_strdup_printf(_("Importing CTI File:\n   %s"), libecat_filename);
    continue_work = (*update_func)(update_data, temp_string, (gdouble) 0.0);
    g_free(temp_string);
  }
  total_planes = dim.z*dim.g*dim.t;
  divider = ((total_planes/AMITK_UPDATE_DIVIDER) < 1) ? 1 : (total_planes/AMITK_UPDATE_DIVIDER);


  /* and load in the data */
  for (i.t = 0, i_plane=0; (i.t < AMITK_DATA_SET_NUM_FRAMES(ds)) && (continue_work); i.t++) {
#ifdef AMIDE_DEBUG
    g_print("\tloading frame:\t%d",i.t);
#endif
    for (i.g = 0; (i.g < AMITK_DATA_SET_NUM_GATES(ds)) && (continue_work); i.g++) {
      for (slice=0; (slice < num_slices) && (continue_work) ; slice++) {
	matnum=mat_numcod(i.t+1,slice+1,i.g+1,0,0);/* frame, plane, gate, data, bed */
      
	/* read in the corresponding cti slice */
	if ((matrix_slice = matrix_read(libecat_file, matnum, 0)) == NULL) {
	  num_corrupted_planes+=dim.z/num_slices;
	  /*	  g_warning(_("Libecat can't get image matrix %x in file %s"), matnum, libecat_filename); */
	  /* goto error; */
	} else {
      
	  /* set the frame duration, note, CTI files specify time as integers in msecs */
	  switch(libecat_file->mhptr->file_type) {
	  case PetImage: 
	  case PetVolume: 
	  case InterfileImage:
	    ish = (Image_subheader *) matrix_slice->shptr;
	    ds->frame_duration[i.t] = ish->frame_duration/1000.0;
	    break;
	  case Normalization:
	  case AttenCor:
	    ds->frame_duration[i.t] = 1.0; /* doesn't mean anything */
	    break;
	  case Sinogram:
	    ssh = (Scan_subheader *) matrix_slice->shptr;
	    ds->frame_duration[i.t] = ssh->frame_duration/1000.0;
	    break;
	  default:
	    break; /* should never get here */
	  }
	  
	  for (i.z = slice*(dim.z/num_slices); 
	       i.z < dim.z/num_slices + slice*(dim.z/num_slices); 
	       i.z++, i_plane++) {
	    
	    if (update_func != NULL) {
	      x = div(i_plane,divider);
	      if (x.rem == 0)
		continue_work = (*update_func)(update_data, NULL, ((gdouble) i_plane)/((gdouble)total_planes));
	    }
	  
	    /* save the scale factor */
	    j.x = j.y = 0;
	    j.z = slice;
	    j.g = i.g;
	    j.t = i.t;
	    if (scaling_type == AMITK_SCALING_TYPE_2D) 
	      *AMITK_RAW_DATA_DOUBLE_2D_SCALING_POINTER(ds->internal_scaling_factor, j) = calibration_factor*matrix_slice->scale_factor;
	    else if (i.z == 0)  /* AMITK_SCALING_TYPE_1D */
	      *AMITK_RAW_DATA_DOUBLE_1D_SCALING_POINTER(ds->internal_scaling_factor, j) = calibration_factor*matrix_slice->scale_factor;
	    
	    switch (format) {
	    case AMITK_FORMAT_SSHORT:
	      /* copy the data into the volume */
	      /* note, we compensate here for the fact that we define 
		 our origin as the bottom left, not top left like the CTI file */
	      for (i.y = 0; i.y < dim.y; i.y++) 
		for (i.x = 0; i.x < dim.x; i.x++)
		  AMITK_RAW_DATA_SSHORT_SET_CONTENT(ds->raw_data,i) =
		    *(((amitk_format_SSHORT_t *) matrix_slice->data_ptr) + 
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
		    *(((amitk_format_FLOAT_t *) matrix_slice->data_ptr) + 
		      (dim.y*dim.x*(i.z-slice*(dim.z/num_slices))
		     +dim.x*(dim.y-i.y-1)
		       +i.x));
	      break;
	    default:
	      g_error("unexpected case in %s at %d\n", __FILE__, __LINE__);
	      goto error;
	      break; 
	    } /* format */
	  } /* i.z */
	  free_matrix_data(matrix_slice);
	} /* matrix_slice != NULL */
      } /* slice */
    } /* i.g */
#ifdef AMIDE_DEBUG
    g_print("\tduration:\t%5.3f\n",ds->frame_duration[i.t]);
#endif
  } /* i.t */

  if (num_corrupted_planes > 0) 
    g_warning(_("Libecat returned %d blank planes... corrupted data file?  Use data with caution."), num_corrupted_planes);

  if (!continue_work) goto error;

  /* setup remaining volume parameters */
  amitk_data_set_set_injected_dose(ds, amitk_dose_unit_convert_from(libecat_file->mhptr->dosage, 
								    AMITK_DOSE_UNIT_MILLICURIE));
  amitk_data_set_set_subject_weight(ds, libecat_file->mhptr->patient_weight);
  amitk_data_set_set_scale_factor(ds, 1.0); /* set the external scaling factor */
  amitk_data_set_calc_far_corner(ds); /* set the far corner of the volume */
  amitk_data_set_calc_min_max(ds, update_func, update_data);
  amitk_volume_set_center(AMITK_VOLUME(ds), offset_rp);
  goto function_end;



 error:

  if (ds != NULL) 
    ds = amitk_object_unref(ds);



 function_end:

  if (update_func != NULL) /* remove progress bar */
    (*update_func)(update_data, NULL, (gdouble) 2.0); 

  if (libecat_file != NULL)
    matrix_close(libecat_file);

  if (matrix_data != NULL)
    free_matrix_data(matrix_data);


  setlocale(LC_NUMERIC, saved_time_locale);
  setlocale(LC_NUMERIC, saved_numeric_locale);
  g_free(saved_time_locale);
  g_free(saved_numeric_locale);
  return ds;

}


#endif








