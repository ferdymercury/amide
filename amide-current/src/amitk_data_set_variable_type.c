/* amitk_data_set_variable_type.c - used to generate the different amitk_data_set_*.c files
 *
 * Part of amide - Amide's a Medical Image Data Examiner
 * Copyright (C) 2001-2002 Andy Loening
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
#include <glib.h>
#ifdef AMIDE_DEBUG
#include <stdlib.h>
#endif
#include "amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'.h"
#include "amitk_data_set_FLOAT_0D_SCALING.h"

#define DIM_TYPE_`'m4_Scale_Dim`'


/* function to recalcule the max and min values of a data set */
void amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_calc_frame_max_min(AmitkDataSet * data_set) {

  AmitkVoxel i;
  amide_data_t max, min, temp;
  AmitkVoxel initial_voxel;

  
  initial_voxel = zero_voxel;
  for (i.t = 0; i.t < AMITK_DATA_SET_NUM_FRAMES(data_set); i.t++) {
    initial_voxel.t = i.t;
    temp = AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENT(data_set, initial_voxel);
    if (finite(temp)) max = min = temp;   
    else max = min = 0.0; /* just throw in zero */

    for (i.z = 0; i.z < data_set->raw_data->dim.z; i.z++) 
      for (i.y = 0; i.y < data_set->raw_data->dim.y; i.y++) 
	for (i.x = 0; i.x < data_set->raw_data->dim.x; i.x++) {
	  temp = AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENT(data_set, i);
	  if (finite(temp)) {
	    if (temp > max) max = temp;
	    else if (temp < min) min = temp;
	  }
	}
    
    data_set->frame_max[i.t] = max;
    data_set->frame_min[i.t] = min;
    
#ifdef AMIDE_DEBUG
    g_print("\tframe %d max %5.3f frame min %5.3f\n",i.t, data_set->frame_max[i.t],data_set->frame_min[i.t]);
#endif
  }
  
  return;
}

/* generate the distribution array for a data_set */
void amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_calc_distribution(AmitkDataSet * data_set) {

  AmitkVoxel i,j;
  amide_data_t scale;
  AmitkVoxel dim;

  if (data_set->distribution != NULL)
    return;

#if AMIDE_DEBUG
  g_print("Generating distribution data for medical data set %s", AMITK_OBJECT_NAME(data_set));
#endif
  scale = (AMITK_DATA_SET_DISTRIBUTION_SIZE-1)/(data_set->global_max - data_set->global_min);
  
  dim.x = AMITK_DATA_SET_DISTRIBUTION_SIZE;
  dim.y = dim.z = dim.t = 1;
  data_set->distribution = amitk_raw_data_new_with_data(AMITK_FORMAT_DOUBLE, dim);
  if (data_set->distribution == NULL) {
    g_warning("couldn't allocate space for the data set structure to hold distribution data");
    return;
  }
  
  /* initialize the distribution array */
  amitk_raw_data_DOUBLE_initialize_data(data_set->distribution, 0.0);
  
  /* now "bin" the data */
  j.t = j.z = j.y = 0;
  for (i.t = 0; i.t < AMITK_DATA_SET_NUM_FRAMES(data_set); i.t++) {
#if AMIDE_DEBUG
    div_t x;
    gint divider;
    divider = ((data_set->raw_data->dim.z/AMIDE_UPDATE_DIVIDER) < 1) ? 1 : (data_set->raw_data->dim.z/AMIDE_UPDATE_DIVIDER);
    g_print("\n\tframe %d\t",i.t);
#endif
    for ( i.z = 0; i.z < data_set->raw_data->dim.z; i.z++) {
#if AMIDE_DEBUG
      x = div(i.z,divider);
      if (x.rem == 0)
	g_print(".");
#endif
	for (i.y = 0; i.y < data_set->raw_data->dim.y; i.y++) 
	  for (i.x = 0; i.x < data_set->raw_data->dim.x; i.x++) {
	    j.x = scale*(AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENT(data_set,i)-data_set->global_min);
	    AMITK_RAW_DATA_DOUBLE_SET_CONTENT(data_set->distribution,j) += 1.0;
	  }
    }
  }
#if AMIDE_DEBUG
  g_print("\n");
#endif
  
  
  /* do some log scaling so the distribution is more meaningful, and doesn't get
     swamped by outlyers */
  for (j.x = 0; j.x < data_set->distribution->dim.x ; j.x++) 
    AMITK_RAW_DATA_DOUBLE_SET_CONTENT(data_set->distribution,j) = 
      log10(AMITK_RAW_DATA_DOUBLE_CONTENT(data_set->distribution,j)+1.0);

  return;
}


/* return a planar projection of the data set given the associated view */
AmitkDataSet * amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_get_projection(AmitkDataSet * data_set,
										   const AmitkView view,
										   const guint frame) {

  AmitkDataSet * projection;
  AmitkVoxel dim, i, j;
  AmitkPoint voxel_size;
  amide_data_t normalizer;

  dim.z = 1;
  dim.t = 1;
  switch(view) {
  case AMITK_VIEW_CORONAL:
    dim.x = AMITK_DATA_SET_DIM_X(data_set);
    dim.y = AMITK_DATA_SET_DIM_Z(data_set);
    break;
  case AMITK_VIEW_SAGITTAL:
    dim.x = AMITK_DATA_SET_DIM_Y(data_set);
    dim.y = AMITK_DATA_SET_DIM_Z(data_set);
    break;
  case AMITK_VIEW_TRANSVERSE:
  default:
    dim.x = AMITK_DATA_SET_DIM_X(data_set);
    dim.y = AMITK_DATA_SET_DIM_Y(data_set);
    break;
  }


  projection = amitk_data_set_new_with_data(AMITK_FORMAT_DOUBLE, dim, AMITK_SCALING_0D);
  if (projection == NULL) {
    g_warning("couldn't allocate space for the projection, wanted %dx%dx%dx%d elements", 
	      dim.x, dim.y, dim.z, dim.t);
    return NULL;
  }

  

  voxel_size = AMITK_DATA_SET_VOXEL_SIZE(data_set);
  switch(view) {
  case AMITK_VIEW_CORONAL:
    projection->voxel_size.x = voxel_size.x;
    projection->voxel_size.y = voxel_size.z;
    projection->voxel_size.z = AMITK_VOLUME_Y_CORNER(data_set);
    normalizer = voxel_size.y/projection->voxel_size.z;
    break;
  case AMITK_VIEW_SAGITTAL:
    projection->voxel_size.x = voxel_size.y;
    projection->voxel_size.y = voxel_size.z;
    projection->voxel_size.z = AMITK_VOLUME_X_CORNER(data_set);
    normalizer = voxel_size.x/projection->voxel_size.z;
    break;
  case AMITK_VIEW_TRANSVERSE:
  default:
    projection->voxel_size.x = voxel_size.x;
    projection->voxel_size.y = voxel_size.y;
    projection->voxel_size.z = AMITK_VOLUME_Z_CORNER(data_set);
    normalizer = voxel_size.z/projection->voxel_size.z;
    break;
  }

  projection->slice_parent = g_object_ref(data_set);
  amitk_space_copy_in_place(AMITK_SPACE(projection), AMITK_SPACE(data_set));
  amitk_data_set_calc_far_corner(projection);
  projection->scan_start = amitk_data_set_get_start_time(data_set, frame);
  amitk_data_set_set_thresholding(projection, AMITK_THRESHOLDING_GLOBAL);
  amitk_data_set_set_color_table(projection, AMITK_DATA_SET_COLOR_TABLE(data_set));
  amitk_data_set_set_frame_duration(projection, 0, amitk_data_set_get_frame_duration(data_set, frame));

  /* initialize our data set */
  amitk_raw_data_DOUBLE_initialize_data(projection->raw_data, 0.0);

  dim = AMITK_DATA_SET_DIM(data_set);
  i.t = frame;
  j.t = j.z = 0;
  /* and compute the projection */
  switch(view) {
  case AMITK_VIEW_CORONAL:
    for (i.y = 0; i.y < dim.y; i.y++)
      for (i.z = 0, j.y = dim.z-1; i.z < dim.z; i.z++, j.y--)
	for (i.x = j.x = 0; i.x < dim.x; i.x++, j.x++)
	  AMITK_RAW_DATA_DOUBLE_SET_CONTENT(projection->raw_data,j) += 
	    AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENT(data_set,i);
    break;
  case AMITK_VIEW_SAGITTAL:
    for (i.x = 0; i.x < dim.x; i.x++)
      for (i.z = 0, j.y = dim.z-1; i.z < dim.z; i.z++, j.y--)
	for (i.y = j.x = 0; i.y < dim.y; i.y++, j.x++)
	  AMITK_RAW_DATA_DOUBLE_SET_CONTENT(projection->raw_data,j) += 
	    AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENT(data_set,i);
    break;
  case AMITK_VIEW_TRANSVERSE:
  default:
    for (i.z = 0; i.z < dim.z; i.z++)
      for (i.y = j.y = 0; i.y < dim.y; i.y++, j.y++)
	for (i.x = j.x = 0; i.x < dim.x; i.x++, j.x++)
	  AMITK_RAW_DATA_DOUBLE_SET_CONTENT(projection->raw_data,j) += 
	    AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENT(data_set,i);
    break;
  }

  /* normalize for the thickness, we're assuming the previous voxels were
     in some units of blah/mm^3 */
  dim = AMITK_DATA_SET_DIM(projection);
  j.t = j.z = 0;
  for (j.y = 0; j.y < dim.y; j.y++)
    for (j.x = 0; j.x < dim.x; j.x++)
      AMITK_RAW_DATA_DOUBLE_SET_CONTENT(projection->raw_data,j) *= normalizer;

  amitk_data_set_calc_frame_max_min(projection);
  amitk_data_set_calc_global_max_min(projection);
  amitk_data_set_set_threshold_max(projection, 0,
				   AMITK_DATA_SET_GLOBAL_MAX(projection));
  amitk_data_set_set_threshold_min(projection, 0,
				   AMITK_DATA_SET_GLOBAL_MIN(projection));

  return projection;
}

AmitkDataSet * amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_get_cropped(const AmitkDataSet * data_set,
										const AmitkVoxel start,
										const AmitkVoxel end) {

  AmitkDataSet * cropped;
  AmitkVoxel i, j;
  AmitkVoxel dim;
  gchar * temp_string;
  AmitkPoint shift;
  AmitkPoint temp_pt;
  GList * children;

  cropped = AMITK_DATA_SET(amitk_object_copy(AMITK_OBJECT(data_set)));

  /* start by unrefing the info that's only copied by reference by amitk_object_copy */
  if (cropped->raw_data != NULL) {
    g_object_unref(cropped->raw_data);
    cropped->raw_data = NULL;
  }
  
  if (cropped->internal_scaling != NULL) {
    g_object_unref(cropped->internal_scaling);
    cropped->internal_scaling = NULL;
  }

  if (cropped->distribution != NULL) {
    g_object_unref(cropped->distribution);
    cropped->distribution = NULL;
  }

  /* and unref anything that's obviously now incorrect */
  if (cropped->current_scaling != NULL) {
    g_object_unref(cropped->current_scaling);
    cropped->current_scaling = NULL;
  }

  if (cropped->frame_duration != NULL) {
    g_free(cropped->frame_duration);
    cropped->frame_duration = NULL;
  }

  if (cropped->frame_max != NULL) {
    g_free(cropped->frame_max);
    cropped->frame_max = NULL;
  }

  if (cropped->frame_min != NULL) {
    g_free(cropped->frame_min);
    cropped->frame_min = NULL;
  }

  /* set a new name for this guy */
  temp_string = g_strdup_printf("%s, cropped", AMITK_OBJECT_NAME(data_set));
  amitk_object_set_name(AMITK_OBJECT(cropped), temp_string);
  g_free(temp_string);

  /* start the new building process */
  cropped->raw_data = amitk_raw_data_new_with_data(data_set->raw_data->format, 
						   voxel_add(voxel_sub(end, start), one_voxel));
  if (cropped->raw_data == NULL) {
    g_warning("couldn't allocate space for the cropped raw data set structure");
    goto error;
  }
  
  /* copy the raw data on over */
  for (i.t=0, j.t=start.t; j.t <= end.t; i.t++, j.t++) 
    for (i.z=0, j.z=start.z; j.z <= end.z; i.z++, j.z++) 
      for (i.y=0, j.y=start.y; j.y <= end.y; i.y++, j.y++) 
	for (i.x=0, j.x=start.x; j.x <= end.x; i.x++, j.x++) 
	  AMITK_RAW_DATA_`'m4_Variable_Type`'_SET_CONTENT(cropped->raw_data,i) =
	    AMITK_RAW_DATA_`'m4_Variable_Type`'_SET_CONTENT(data_set->raw_data,j);


  dim = one_voxel;
#if defined(DIM_TYPE_1D_SCALING) || defined(DIM_TYPE_2D_SCALING)
  dim.t = end.t-start.t+1;
#endif
#if defined(DIM_TYPE_2D_SCALING)
  dim.z = end.z-start.z+1;
#endif

  /* get space for the scale factors */
  cropped->internal_scaling = 
    amitk_raw_data_new_with_data(AMITK_RAW_DATA_FORMAT(data_set->internal_scaling),dim);
  if (cropped->internal_scaling == NULL) {
    g_warning("couldn't allocate space for the cropped internal scaling structure");
    goto error;
  }

  /* now copy over the scale factors */
  i = j = zero_voxel;
#if defined(DIM_TYPE_1D_SCALING) || defined(DIM_TYPE_2D_SCALING)
  for (i.t=0, j.t=start.t; j.t <= end.t; i.t++, j.t++)
#endif
#if defined (DIM_TYPE_2D_SCALING)
    for (i.z=0, j.z=start.z; j.z <= end.z; i.z++, j.z++)
#endif
      (*AMITK_RAW_DATA_DOUBLE_`'m4_Scale_Dim`'_POINTER(cropped->internal_scaling, i)) =
	*AMITK_RAW_DATA_DOUBLE_`'m4_Scale_Dim`'_POINTER(data_set->internal_scaling, j);



  /* reset the current scaling array */
  amitk_data_set_set_scale_factor(cropped, AMITK_DATA_SET_SCALE_FACTOR(data_set));

  /* setup the frame time array */
  cropped->scan_start = amitk_data_set_get_start_time(data_set, start.t);
  cropped->frame_duration = amitk_data_set_get_frame_duration_mem(data_set);
  for (i.t=0, j.t=start.t; j.t <= end.t; i.t++, j.t++) 
    amitk_data_set_set_frame_duration(cropped, i.t, 
				      amitk_data_set_get_frame_duration(data_set, j.t));

  /* recalc the temporary parameters */
  amitk_data_set_calc_far_corner(cropped);
  amitk_data_set_calc_frame_max_min(cropped);
  amitk_data_set_calc_global_max_min(cropped);

  /* and shift the offset appropriately */
  POINT_MULT(start, AMITK_DATA_SET_VOXEL_SIZE(cropped), temp_pt);
  temp_pt = amitk_space_s2b(AMITK_SPACE(data_set), temp_pt);
  shift = point_sub(temp_pt, AMITK_SPACE_OFFSET(data_set));
  amitk_space_shift_offset(AMITK_SPACE(cropped), shift);

  /* and reshift the children, so they stay in the right spot */
  shift = point_neg(shift);
  children = AMITK_OBJECT_CHILDREN(cropped);
  while (children != NULL) {
    amitk_space_shift_offset(AMITK_SPACE(children->data), shift);
    children = children->next;
  }

  return cropped;

 error:
  g_object_unref(cropped);

  return NULL;
}

/* fills the data set "filtered_ds", with the results of the kernel convolved to data_set */
/* assumptions:
   1- filtered_ds is of type FLOAT
   2- scale of filtered_ds is 1.0
   3- kernel has dimension of 1 in the T direction
   4- kernel is of type DOUBLE

   notes:
   data_set can be the same as filtered_ds;
 */
void amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_apply_kernel(const AmitkDataSet * data_set,
								       AmitkDataSet * filtered_ds,
								       const AmitkRawData * kernel) {

  AmitkVoxel i,j, k;
  AmitkVoxel ds_dim, kernel_dim, mid_dim, output_dim;
  AmitkVoxel inner_start, inner_end, kernel_start;
  AmitkRawData * output_data;
#if AMIDE_DEBUG
  div_t x;
  gint divider;

#endif

  g_return_if_fail(AMITK_IS_DATA_SET(data_set));
  g_return_if_fail(AMITK_IS_DATA_SET(filtered_ds));
  g_return_if_fail(VOXEL_EQUAL(AMITK_DATA_SET_DIM(data_set), AMITK_DATA_SET_DIM(filtered_ds)));

  g_return_if_fail(AMITK_RAW_DATA_FORMAT(AMITK_DATA_SET_RAW_DATA(filtered_ds)) == AMITK_FORMAT_FLOAT);
  g_return_if_fail(REAL_EQUAL(AMITK_DATA_SET_SCALE_FACTOR(filtered_ds), 1.0));
  g_return_if_fail(VOXEL_EQUAL(AMITK_RAW_DATA_DIM(filtered_ds->internal_scaling), one_voxel));
  g_return_if_fail(AMITK_RAW_DATA_DIM_T(kernel) == 1); /* haven't written support yet */
  g_return_if_fail(AMITK_RAW_DATA_FORMAT(kernel) == AMITK_FORMAT_DOUBLE);

  kernel_dim = AMITK_RAW_DATA_DIM(kernel);

  /* check it's odd */
  g_return_if_fail(kernel_dim.x & 0x1);
  g_return_if_fail(kernel_dim.y & 0x1);
  g_return_if_fail(kernel_dim.z & 0x1);

  ds_dim = AMITK_DATA_SET_DIM(filtered_ds);

  mid_dim.t = 0;
  mid_dim.z = kernel_dim.z >> 1;
  mid_dim.y = kernel_dim.y >> 1;
  mid_dim.x = kernel_dim.x >> 1;

  inner_start.t = 0;
  inner_end.t = 0;
  i.t = k.t = 0;

  output_dim = AMITK_DATA_SET_DIM(data_set);
  output_dim.t = 1;
  if ((output_data = amitk_raw_data_new_with_data(AMITK_FORMAT_FLOAT, output_dim)) == NULL) {
    g_warning("couldn't alloc space for FIR filter internal data");
    return;
  }
  amitk_raw_data_FLOAT_initialize_data(output_data, 0.0);

  /* convolve the kernel with the data set */
  for (j.t=0; j.t < AMITK_DATA_SET_NUM_FRAMES(data_set); j.t++) {

#ifdef AMIDE_DEBUG
    divider = ((ds_dim.z/20.0) < 1) ? 1 : (ds_dim.z/20.0);
    g_print("Filtering Frame %d\t", j.t);
#endif

    for (i.z=0; i.z < ds_dim.z; i.z++) {
#ifdef AMIDE_DEBUG
      x = div(i.z,divider);
      if (x.rem == 0) g_print(".");
#endif 

      inner_start.z = i.z-mid_dim.z;
      if (inner_start.z < 0) {
	kernel_start.z = kernel_dim.z-1+inner_start.z;
	inner_start.z = 0;
      } else {
	kernel_start.z = kernel_dim.z-1;
      }
      inner_end.z = i.z+mid_dim.z;
      if (inner_end.z >= ds_dim.z) 
	inner_end.z = ds_dim.z-1;


      for (i.y=0; i.y < ds_dim.y; i.y++) {
	inner_start.y = i.y-mid_dim.y;
	if (inner_start.y < 0) {
	  kernel_start.y = kernel_dim.y-1+inner_start.y;
	  inner_start.y = 0;
	} else {
	  kernel_start.y = kernel_dim.y-1;
	}
	inner_end.y = i.y+mid_dim.y;
	if (inner_end.y >= ds_dim.y) 
	  inner_end.y = ds_dim.y-1;
	
	for (i.x=0; i.x < ds_dim.x; i.x++) {
	  inner_start.x = i.x-mid_dim.x;
	  if (inner_start.x < 0) {
	    kernel_start.x = kernel_dim.x-1+inner_start.x;
	    inner_start.x = 0;
	  } else {
	    kernel_start.x = kernel_dim.x-1;
	  }
	  inner_end.x = i.x+mid_dim.x;
	  if (inner_end.x >= ds_dim.x) 
	    inner_end.x = ds_dim.x-1;

	  /* inner loop */
	  for (j.z=inner_start.z, k.z = kernel_start.z; j.z <= inner_end.z; j.z++, k.z--) {
	    for (j.y=inner_start.y, k.y = kernel_start.y; j.y <= inner_end.y; j.y++, k.y--) {
	      for (j.x=inner_start.x, k.x = kernel_start.x; j.x <= inner_end.x; j.x++, k.x--) {
		AMITK_RAW_DATA_FLOAT_SET_CONTENT(output_data, i) +=
		  AMITK_RAW_DATA_DOUBLE_CONTENT(kernel, k) *
		  AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENT(data_set, j);
	      }
	    }
	  }
	}
      }
    }

    /* copy the output_data over into the filtered_ds */
    for (i.z=0, j.z=0; i.z < output_dim.z; i.z++, j.z++)
      for (i.y=0, j.y=0; i.y < output_dim.y; i.y++, j.y++)
	for (i.x=0, j.x=0; i.x < output_dim.x; i.x++, j.x++)
	  AMITK_RAW_DATA_FLOAT_SET_CONTENT(filtered_ds->raw_data, j) = 
	    AMITK_RAW_DATA_FLOAT_CONTENT(output_data, i);
#ifdef AMIDE_DEBUG
    g_print("\n");
#endif 
  }


  g_object_unref(output_data); /* cleanup */

  return;
}

void amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_filter_gaussian(const AmitkDataSet * data_set,
									  AmitkDataSet * filtered_ds,
									  const gint kernel_size,
									  const amide_real_t fwhm) {

  AmitkRawData * kernel;
  AmitkAxis i_axis;


  /* do the x direction */
  kernel = amitk_filter_calculate_gaussian_kernel(kernel_size, 
						  AMITK_DATA_SET_VOXEL_SIZE(data_set),
						  fwhm, AMITK_AXIS_X);
  amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_apply_kernel(data_set,
								    filtered_ds,
								    kernel);
  g_object_unref(kernel);

  /* and do the rest of the directions */
  for (i_axis=AMITK_AXIS_Y; i_axis < AMITK_AXIS_NUM; i_axis++) {
    kernel = amitk_filter_calculate_gaussian_kernel(kernel_size, 
						    AMITK_DATA_SET_VOXEL_SIZE(data_set),
						    fwhm, i_axis);
    amitk_data_set_FLOAT_0D_SCALING_apply_kernel(filtered_ds,
						 filtered_ds,
						 kernel);
    g_object_unref(kernel);
  }

  return;
}



/* do a (destructive) partial sort of the given data to find median */
/* adapted and modified from Numerical Receipes in C, (who got it from Knuth, Vol 3?) */
/* median size needs to be odd for this to be strictly correct from a statistical stand point*/

#define SWAP(leftv, rightv) temp=(leftv); (leftv)=(rightv); (rightv=temp)
static amide_data_t find_median_by_partial_sort(amide_data_t * partial_sort_data, gint size) {

  gint left, right, mid;
  gint i_left, i_right;
  gint median_point;
  amide_data_t partition_value;
  amide_data_t temp;

  median_point = (size-1) >> 1;  /* not strictly correct for case of even size */
  
  left = 0;
  right = size-1;
  while (TRUE)

    if (right-left <= 2) { /* <= 3 elements left */

      if (right-left == 1) {/* 2 elements left to sort*/
	if (partial_sort_data[left] > partial_sort_data[right]) {
	  SWAP(partial_sort_data[left], partial_sort_data[right]);
	}
      } else if (right-left == 2){ /* 3 elements left to sort */
	if (partial_sort_data[left] > partial_sort_data[right]) {
	  SWAP(partial_sort_data[left], partial_sort_data[right]);
	}
	if (partial_sort_data[left+1] > partial_sort_data[right]) {
	  SWAP(partial_sort_data[left+1], partial_sort_data[right]);
	}
	if (partial_sort_data[left] > partial_sort_data[left+1]) {
	  SWAP(partial_sort_data[left], partial_sort_data[left+1]);
	}
      }
      return partial_sort_data[median_point];

    } else {
      mid = (left+right) >> 1;

      /* do a three way median with the leftmost, rightmost, and mid elements
	 to get a decent guess for a partitioning element.  This partioning element
	 ends up in left+1*/
      SWAP(partial_sort_data[mid], partial_sort_data[left+1]);
      if (partial_sort_data[left] > partial_sort_data[right]) {
	SWAP(partial_sort_data[left], partial_sort_data[right]);
      }
      if (partial_sort_data[left+1] > partial_sort_data[right]) {
	SWAP(partial_sort_data[left+1], partial_sort_data[right]);
      }
      if (partial_sort_data[left] > partial_sort_data[left+1]) {
	SWAP(partial_sort_data[left], partial_sort_data[left+1]);
      }
      
      
      partition_value = partial_sort_data[left+1];
      i_left=left+1;
      i_right = right;

      while (TRUE) {

	/* find an element > partition_value */
	do i_left++; while (partial_sort_data[i_left] < partition_value);

	/* find an element < partition value */
	do i_right--; while (partial_sort_data[i_right] > partition_value);

	if (i_right < i_left) break; /* pointers crossed */

	SWAP(partial_sort_data[i_left], partial_sort_data[i_right]);
      }

      partial_sort_data[left+1] = partial_sort_data[i_right];
      partial_sort_data[i_right] = partition_value;

      if (i_right >= median_point) right = i_right-1;
      if (i_right <= median_point) left = i_left;

    }

}


/* assumptions :
   filtered_ds needs to be of type FLOAT, with 0D scaling

   notes:
   data_set can be the same as filtered_ds
 */	    
void amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_filter_median_3D(const AmitkDataSet * data_set,
									   AmitkDataSet * filtered_ds,
									   const AmitkVoxel kernel_dim) {

  amide_data_t * partial_sort_data;
  AmitkVoxel i,j, mid_dim, output_dim;
  gint loc, median_size;
  AmitkRawData * output_data;
  AmitkVoxel ds_dim;
#if AMIDE_DEBUG
  div_t x;
  gint divider;

#endif

  g_return_if_fail(AMITK_IS_DATA_SET(data_set));
  g_return_if_fail(AMITK_IS_DATA_SET(filtered_ds));
  g_return_if_fail(VOXEL_EQUAL(AMITK_DATA_SET_DIM(data_set), AMITK_DATA_SET_DIM(filtered_ds)));

  g_return_if_fail(AMITK_RAW_DATA_FORMAT(AMITK_DATA_SET_RAW_DATA(filtered_ds)) == AMITK_FORMAT_FLOAT);
  g_return_if_fail(REAL_EQUAL(AMITK_DATA_SET_SCALE_FACTOR(filtered_ds), 1.0));
  g_return_if_fail(VOXEL_EQUAL(AMITK_RAW_DATA_DIM(filtered_ds->internal_scaling), one_voxel));
  g_return_if_fail(kernel_dim.t == 1); /* haven't written support yet */

  /* check it's odd */
  g_return_if_fail(kernel_dim.x & 0x1);
  g_return_if_fail(kernel_dim.y & 0x1);
  g_return_if_fail(kernel_dim.z & 0x1);

  mid_dim.t = 0;
  mid_dim.z = kernel_dim.z >> 1;
  mid_dim.y = kernel_dim.y >> 1;
  mid_dim.x = kernel_dim.x >> 1;

  median_size = kernel_dim.z*kernel_dim.y*kernel_dim.x;

  ds_dim = AMITK_DATA_SET_DIM(data_set);
  output_dim = ds_dim;
  output_dim.t = 1;
  if ((output_data = amitk_raw_data_new_with_data(AMITK_FORMAT_FLOAT, output_dim)) == NULL) {
    g_warning("couldn't allocate space for the internal raw data");
    return;
  }
  amitk_raw_data_FLOAT_initialize_data(output_data, 0.0);
  partial_sort_data = g_try_new(amide_data_t, median_size);
  g_return_if_fail(partial_sort_data != NULL);


  /* iterate over all the voxels in the data_set */
  i.t = 0;
  for (j.t=0; j.t < AMITK_DATA_SET_NUM_FRAMES(data_set); j.t++) {
#ifdef AMIDE_DEBUG
    divider = ((output_dim.z/20.0) < 1) ? 1 : (output_dim.z/20.0);
    g_print("Filtering Frame %d\t", j.t);
#endif
    for (i.z=0; i.z < output_dim.z; i.z++) {
#ifdef AMIDE_DEBUG
      x = div(i.z,divider);
      if (x.rem == 0) g_print(".");
#endif 
      for (i.y=0; i.y < output_dim.y; i.y++) {
	for (i.x=0; i.x < output_dim.x; i.x++) {
	    
	    /* initialize the data for the iteration */
	    loc = 0;
	    for (j.z = i.z-mid_dim.z; j.z <= i.z+mid_dim.z; j.z++) {
	      if ((j.z < 0) || (j.z >= ds_dim.z)) {
		for (j.y=0; j.y < kernel_dim.y; j.y++) {
		  for (j.x=0; j.x < kernel_dim.x; j.x++) {
		    partial_sort_data[loc] = 0.0;
		    loc++;
		  }
		}
	      } else {
		for (j.y = i.y-mid_dim.y; j.y <= i.y+mid_dim.y; j.y++) {
		  if ((j.y < 0) || (j.y >= ds_dim.y)) {
		    for (j.x=0; j.x < kernel_dim.x; j.x++) {
		      partial_sort_data[loc] = 0.0;
		      loc++;
		    }
		  } else {
		    for (j.x = i.x-mid_dim.x; j.x <= i.x+mid_dim.x; j.x++) {
		      if ((j.x < 0) || (j.x >= ds_dim.x)) {
			partial_sort_data[loc] = 0.0;
			loc++;
		      } else {
			partial_sort_data[loc] = 
			  AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENT(data_set, j);
			loc++;
		      }
		    }
		  }
		}
	      }
	    } /* end initializing data */

	    // remove 
	    if (loc != median_size) {
	      g_print("initialize descrepency : %d %d\n", loc, median_size);
	    }
	    
	    /* and store median value */
	    AMITK_RAW_DATA_FLOAT_SET_CONTENT(output_data, i) = 
	      find_median_by_partial_sort(partial_sort_data, median_size);

	  } /* i.x */
	} /* i.y */
      } /* i.z */


    /* copy the output_data over into the filtered_ds */
    for (i.z=0, j.z=0; i.z < output_dim.z; i.z++, j.z++)
      for (i.y=0, j.y=0; i.y < output_dim.y; i.y++, j.y++)
	for (i.x=0, j.x=0; i.x < output_dim.x; i.x++, j.x++)
	  AMITK_RAW_DATA_FLOAT_SET_CONTENT(filtered_ds->raw_data, j) = 
	    AMITK_RAW_DATA_FLOAT_CONTENT(output_data, i);
#ifdef AMIDE_DEBUG
    g_print("\n");
#endif 
  } /* j.t */

  /* garbage collection */
  g_object_unref(output_data); 
  g_free(partial_sort_data);

  return;
}
  

void amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_filter_median_linear(const AmitkDataSet * data_set,
									       AmitkDataSet * filtered_ds,
									       const gint kernel_size) {

  AmitkVoxel kernel_dim;

  kernel_dim.x = kernel_size;
  kernel_dim.y = 1;
  kernel_dim.z = 1;
  kernel_dim.t = 1;
  amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_filter_median_3D(data_set, 
									filtered_ds,
									kernel_dim);
  
  kernel_dim.x = 1;
  kernel_dim.y = kernel_size;
  kernel_dim.z = 1;
  kernel_dim.t = 1;
  amitk_data_set_FLOAT_0D_SCALING_filter_median_3D(filtered_ds,filtered_ds,kernel_dim);
  
  kernel_dim.x = 1;
  kernel_dim.y = 1;
  kernel_dim.z = kernel_size;
  kernel_dim.t = 1;
  amitk_data_set_FLOAT_0D_SCALING_filter_median_3D(filtered_ds,filtered_ds,kernel_dim);

  return;
}


/* returns a slice  with the appropriate data from the data_set */
AmitkDataSet * amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_get_slice(AmitkDataSet * data_set,
									      const amide_time_t requested_start,
									      const amide_time_t requested_duration,
									      const amide_real_t pixel_dim,
									      const AmitkVolume * slice_volume,
									      const gboolean need_calc_max_min) {

  /* zp_start, where on the zp axis to start the slice, zp (z_prime) corresponds
     to the rotated axises, if negative, choose the midpoint */

  AmitkDataSet * slice;
  AmitkVoxel i_voxel,j_voxel;
  amide_intpoint_t z;
  amide_real_t max_diff, voxel_length, z_steps;
  AmitkPoint alt;
  AmitkPoint stride[AMITK_AXIS_NUM], last[AMITK_AXIS_NUM];
  AmitkAxis i_axis;
  guint l;
  amide_data_t weight;
  amide_data_t time_weight;
  amide_intpoint_t start_frame, end_frame, i_frame;
  amide_time_t start_time, end_time;
  gchar * temp_string;
  AmitkPoint box_point[8];
  AmitkVoxel box_voxel[8];
  AmitkVoxel start, end;
  amide_data_t box_value[8];
  AmitkPoint slice_point, ds_point,diff, nearest_point;
  AmitkSpace * slice_space;
  AmitkSpace * data_set_space;
#if AMIDE_DEBUG
  AmitkPoint center_point;
#endif
  AmitkVoxel ds_voxel;
  amide_data_t weight1, weight2;
  amide_data_t empty;
  AmitkCorners intersection_corners;
  AmitkVoxel dim;

  /* ----- figure out what frames of this data set to include ----*/
  start_frame = amitk_data_set_get_frame(data_set, requested_start);
  start_time = amitk_data_set_get_start_time(data_set, start_frame);

  end_frame = amitk_data_set_get_frame(data_set, requested_start+requested_duration);
  end_time = amitk_data_set_get_end_time(data_set, end_frame);

  if (start_frame != end_frame) {
    if (end_time > requested_start+requested_duration)
      end_time = requested_start+requested_duration;
    if (start_time < requested_start)
      start_time = requested_start;
  }

  /* ------------------------- */

  dim.x = ceil(fabs(AMITK_VOLUME_X_CORNER(slice_volume))/pixel_dim);
  dim.y = ceil(fabs(AMITK_VOLUME_Y_CORNER(slice_volume))/pixel_dim);
  dim.z = dim.t = 1;


  /* get the return slice */
  slice = amitk_data_set_new_with_data(AMITK_FORMAT_DOUBLE, dim, AMITK_SCALING_0D);
  if (slice == NULL) {
    g_warning("couldn't allocate space for the slice, wanted %dx%dx%dx%d elements", 
	      dim.x, dim.y, dim.z, dim.t);
    return NULL;
  }


  slice->slice_parent = g_object_ref(data_set);
  slice->voxel_size.x = pixel_dim;
  slice->voxel_size.y = pixel_dim;
  slice->voxel_size.z = AMITK_VOLUME_Z_CORNER(slice_volume);
  amitk_space_copy_in_place(AMITK_SPACE(slice), AMITK_SPACE(slice_volume));
  slice->scan_start = start_time;
  slice->thresholding = data_set->thresholding;
  slice->interpolation = AMITK_DATA_SET_INTERPOLATION(data_set);

  amitk_data_set_calc_far_corner(slice);
  amitk_data_set_set_frame_duration(slice, 0, end_time-start_time);


#if AMIDE_DEBUG
  center_point = amitk_volume_get_center(slice_volume);
  temp_string =  
    g_strdup_printf("slice from data_set %s: @ x %5.3f y %5.3f z %5.3f", AMITK_OBJECT_NAME(data_set), 
		    center_point.x, center_point.y, center_point.z);
  amitk_object_set_name(AMITK_OBJECT(slice),temp_string);
  g_free(temp_string);
#endif
#ifdef AMIDE_DEBUG_COMMENT_OUT
  {
    AmitkCorners real_corner;
    /* convert to real space */
    real_corner[0] = rs_offset(slice->space);
    real_corner[1] = amitk_space_s2b(slice->space, slice->corner);
    g_print("new slice from data_set %s\t---------------------\n",AMITK_OBJECT_NAME(data_set));
    g_print("\tdim\t\tx %d\t\ty %d\t\tz %d\n",
    	    dim.x, dim.y, dim.z);
    g_print("\treal corner[0]\tx %5.4f\ty %5.4f\tz %5.4f\n",
    	    real_corner[0].x,real_corner[0].y,real_corner[0].z);
    g_print("\treal corner[1]\tx %5.4f\ty %5.4f\tz %5.4f\n",
    	    real_corner[1].x,real_corner[1].y,real_corner[1].z);
    g_print("\tdata set\t\tstart\t%5.4f\tend\t%5.3f\tframes %d to %d\n",
    	    start_time, end_time,start_frame,start_frame+num_frames-1);
  }
#endif


  /* get direct pointers to the slice's and data set's spaces for efficiency */
  slice_space = AMITK_SPACE(slice);
  data_set_space = AMITK_SPACE(data_set);

  /* voxel_length is the length of a voxel given the coordinate frame of the slice.
     this is used to figure out how many iterations in the z direction we need to do */
  alt.x = alt.y = 0.0;
  alt.z = 1.0;
  alt = amitk_space_s2s_dim(slice_space, data_set_space, alt);
  alt = point_mult(alt, data_set->voxel_size);
  voxel_length = POINT_MAGNITUDE(alt);
  z_steps = slice->voxel_size.z/voxel_length;

  /* figure out the intersection bounds between the data set and the requested slice volume */
  if (amitk_volume_volume_intersection_corners(slice_volume, 
					       AMITK_VOLUME(data_set), 
					       intersection_corners)) {
    /* translate the intersection into voxel space */
    POINT_TO_VOXEL(intersection_corners[0], slice->voxel_size, 0, start);
    POINT_TO_VOXEL(intersection_corners[1], slice->voxel_size, 0, end);
  } else { /* no intersection */
    start = zero_voxel;
    end = zero_voxel;
  }

  /* make sure we only iterate over the slice we've already mallocs */
  if (start.x < 0) start.x = 0;
  if (start.y < 0) start.y = 0;
  if (end.x >= dim.x) end.x = dim.x-1;
  if (end.y >= dim.y) end.y = dim.y-1;

  /* empty is what we fill voxels with that aren't in the data set*/
  empty = AMITK_DATA_SET_GLOBAL_MIN(data_set);

  /* iterate over those voxels that we didn't cover, and set them to empty */
  i_voxel.t = i_voxel.z = 0;
  for (i_voxel.y = 0; i_voxel.y < start.y; i_voxel.y++) 
    for (i_voxel.x = 0; i_voxel.x < dim.x; i_voxel.x++) 
      AMITK_RAW_DATA_DOUBLE_SET_CONTENT(slice->raw_data,i_voxel) = empty;
  for (i_voxel.y = end.y+1; i_voxel.y < dim.y; i_voxel.y++) 
    for (i_voxel.x = 0; i_voxel.x < dim.x; i_voxel.x++) 
      AMITK_RAW_DATA_DOUBLE_SET_CONTENT(slice->raw_data,i_voxel) = empty;
  for (i_voxel.x = 0; i_voxel.x < start.x; i_voxel.x++) 
    for (i_voxel.y = 0; i_voxel.y < dim.y; i_voxel.y++) 
      AMITK_RAW_DATA_DOUBLE_SET_CONTENT(slice->raw_data,i_voxel) = empty;
  for (i_voxel.x = end.x+1; i_voxel.x < dim.x; i_voxel.x++) 
    for (i_voxel.y = 0; i_voxel.y < dim.y; i_voxel.y++) 
      AMITK_RAW_DATA_DOUBLE_SET_CONTENT(slice->raw_data,i_voxel) = empty;

  /* and initialize the rest of the voxels */
  for (i_voxel.y = start.y; i_voxel.y <= end.y; i_voxel.y++)
    for (i_voxel.x = start.x; i_voxel.x <= end.x; i_voxel.x++)
      AMITK_RAW_DATA_DOUBLE_SET_CONTENT(slice->raw_data,i_voxel) = 0;



  switch(data_set->interpolation) {

  case AMITK_INTERPOLATION_TRILINEAR:
    /* iterate over the frames we'll be incorporating into this slice */
    for (i_frame = start_frame; i_frame <= end_frame; i_frame++) {
      
      /* averaging over more then one frame */
      if (end_frame-start_frame > 0) {
	if (i_frame == start_frame)
	  time_weight = (amitk_data_set_get_end_time(data_set, start_frame)-start_time)/(end_time-start_time);
	else if (i_frame == end_frame)
	  time_weight = (end_time-amitk_data_set_get_start_time(data_set, end_frame))/(end_time-start_time);
	else
	  time_weight = data_set->frame_duration[i_frame]/(end_time-start_time);
      } else
	time_weight = 1.0;

      /* iterate over the number of planes we'll be compressing into this slice */
      for (z = 0; z < ceil(z_steps*(1.0-SMALL_DISTANCE))*(1.0-SMALL_DISTANCE); z++) {
	
	/* the slices z_coordinate for this iteration's slice voxel */
	slice_point.z = (((amide_real_t) z)+0.5)*voxel_length;

	/* weight is between 0 and 1, this is used to weight the last voxel  in the slice's z direction */
	if (floor(z_steps) > z)
	  weight = time_weight/z_steps;
	else
	  weight = time_weight*(z_steps-floor(z_steps)) / z_steps;

	/* iterate over the y dimension */
	for (i_voxel.y = start.y; i_voxel.y <= end.y; i_voxel.y++) {

	  /* the slice y_coordinate of the center of this iteration's slice voxel */
	  slice_point.y = (((amide_real_t) i_voxel.y)+0.5)*slice->voxel_size.y;
	  
	  /* the slice x coord of the center of the first slice voxel in this loop */
	  slice_point.x = (((amide_real_t) start.x)+0.5)*slice->voxel_size.x;

	  /* iterate over the x dimension */
	  for (i_voxel.x = start.x; i_voxel.x <= end.x; i_voxel.x++) {

	    /* translate the current point in slice space into the data set's coordinate frame */
	    ds_point = amitk_space_s2s(slice_space, data_set_space, slice_point);

	    /* get the nearest neighbor in the data set to this slice voxel */
	    POINT_TO_VOXEL(ds_point, data_set->voxel_size, i_frame, ds_voxel);
	    VOXEL_TO_POINT(ds_voxel, data_set->voxel_size, nearest_point);

	    /* figure out which way to go to get the nearest voxels to our slice voxel*/
	    POINT_SUB(ds_point, nearest_point, diff);

	    /* figure out which voxels to look at */
	    for (l=0; l<8; l=l+1) {
	      if (diff.x < 0)
		box_voxel[l].x = (l & 0x1) ? ds_voxel.x-1 : ds_voxel.x;
	      else /* diff.x >= 0 */
		box_voxel[l].x = (l & 0x1) ? ds_voxel.x : ds_voxel.x+1;
	      if (diff.y < 0)
		box_voxel[l].y = (l & 0x2) ? ds_voxel.y-1 : ds_voxel.y;
	      else /* diff.y >= 0 */
		box_voxel[l].y = (l & 0x2) ? ds_voxel.y : ds_voxel.y+1;
	      if (diff.z < 0)
		box_voxel[l].z = (l & 0x4) ? ds_voxel.z-1 : ds_voxel.z;
	      else /* diff.z >= 0 */
		box_voxel[l].z = (l & 0x4) ? ds_voxel.z : ds_voxel.z+1;
	      box_voxel[l].t = ds_voxel.t;

	      VOXEL_TO_POINT(box_voxel[l], data_set->voxel_size, box_point[l]);

	      /* get the value of the point on the box */
	      if (amitk_raw_data_includes_voxel(data_set->raw_data, box_voxel[l]))
		box_value[l] = AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENT(data_set, box_voxel[l]);
	      else
		box_value[l] = empty;
	    }
	    
	    /* do the x direction linear interpolation of the sets of two points */
	    for (l=0;l<8;l=l+2) {
	      max_diff = box_point[l+1].x-box_point[l].x;
	      weight1 = ((max_diff - (ds_point.x - box_point[l].x))/max_diff);
	      weight2 = ((max_diff - (box_point[l+1].x - ds_point.x))/max_diff);
	      box_value[l] = (box_value[l] * weight1) + (box_value[l+1] * weight2);
	    }

	    /* do the y direction linear interpolation of the sets of two points */
	    for (l=0;l<8;l=l+4) {
	      max_diff = box_point[l+2].y-box_point[l].y;
	      weight1 = ((max_diff - (ds_point.y - box_point[l].y))/max_diff);
	      weight2 = ((max_diff - (box_point[l+2].y - ds_point.y))/max_diff);
	      box_value[l] = (box_value[l] * weight1) + (box_value[l+2] * weight2);
	    }

	    /* do the z direction linear interpolation of the sets of two points */
	    for (l=0;l<8;l=l+8) {
	      max_diff = box_point[l+4].z-box_point[l].z;
	      weight1 = ((max_diff - (ds_point.z - box_point[l].z))/max_diff);
	      weight2 = ((max_diff - (box_point[l+4].z - ds_point.z))/max_diff);
	      box_value[l] = (box_value[l] * weight1) + (box_value[l+4] * weight2);
	    }

	    AMITK_RAW_DATA_DOUBLE_SET_CONTENT(slice->raw_data,i_voxel)+=weight*box_value[0];

	    slice_point.x += slice->voxel_size.x; 
	  }
	}
      }
    }
    break;
  case AMITK_INTERPOLATION_NEAREST_NEIGHBOR:
  default:  
    /* figure out what point in the data set we're going to start at */
    ds_point.x = ((amide_real_t) start.x+0.5) * slice->voxel_size.x;
    ds_point.y = ((amide_real_t) start.y+0.5) * slice->voxel_size.y;
    ds_point.z = voxel_length/2.0;
    ds_point = amitk_space_s2s(slice_space, data_set_space, ds_point);

    /* figure out what stepping one voxel in a given direction in our slice cooresponds to in our data set */
    for (i_axis = 0; i_axis < AMITK_AXIS_NUM; i_axis++) {
      alt.x = (i_axis == AMITK_AXIS_X) ? slice->voxel_size.x : 0.0;
      alt.y = (i_axis == AMITK_AXIS_Y) ? slice->voxel_size.y : 0.0;
      alt.z = (i_axis == AMITK_AXIS_Z) ? voxel_length : 0.0;
      alt = point_add(point_sub(amitk_space_s2b(slice_space, alt),
			  AMITK_SPACE_OFFSET(slice_space)),
		      AMITK_SPACE_OFFSET(data_set_space));
      stride[i_axis] = amitk_space_b2s(data_set_space, alt);
    }

    /* iterate over the number of frames we'll be incorporating into this slice */
    for (i_frame = start_frame; i_frame <= end_frame; i_frame++) {

      /* averaging over more then one frame */
      if (end_frame-start_frame > 0) {
	if (i_frame == start_frame)
	  time_weight = (amitk_data_set_get_end_time(data_set, start_frame)-start_time)/(end_time-start_time);
	else if (i_frame == end_frame)
	  time_weight = (end_time-amitk_data_set_get_start_time(data_set, end_frame))/(end_time-start_time);
	else
	  time_weight = data_set->frame_duration[i_frame]/(end_time-start_time);
      } else
	time_weight = 1.0;

      /* iterate over the number of planes we'll be compressing into this slice */
      for (z = 0; z < ceil(z_steps*(1.0-SMALL_DISTANCE))*(1.0-SMALL_DISTANCE); z++) { 
	last[AMITK_AXIS_Z] = ds_point;

	/* weight is between 0 and 1, this is used to weight the last voxel  in the slice's z direction */
	if (floor(z_steps) > z)
	  weight = time_weight/z_steps;
	else
	  weight = time_weight*(z_steps-floor(z_steps)) / z_steps;
	
	/* iterate over the y dimension */
	for (i_voxel.y = start.y; i_voxel.y <= end.y; i_voxel.y++) {
	  last[AMITK_AXIS_Y] = ds_point;

	  /* and iteratate over x */
	  for (i_voxel.x = start.x; i_voxel.x <= end.x; i_voxel.x++) {
	    POINT_TO_VOXEL(ds_point, data_set->voxel_size, i_frame, j_voxel);
	    if (!amitk_raw_data_includes_voxel(data_set->raw_data,j_voxel))
	      AMITK_RAW_DATA_DOUBLE_SET_CONTENT(slice->raw_data,i_voxel) += weight*empty;
	    else
	      AMITK_RAW_DATA_DOUBLE_SET_CONTENT(slice->raw_data,i_voxel) += 
		weight*AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENT(data_set,j_voxel);
	    POINT_ADD(ds_point, stride[AMITK_AXIS_X], ds_point); 
	  }
	  POINT_ADD(last[AMITK_AXIS_Y], stride[AMITK_AXIS_Y], ds_point);
	}
	POINT_ADD(last[AMITK_AXIS_Z], stride[AMITK_AXIS_Z], ds_point); 
      }
    }
    break;
  }


  if (need_calc_max_min) {
    amitk_data_set_calc_frame_max_min(slice);
    amitk_data_set_calc_global_max_min(slice);
  }

  return slice;
}


