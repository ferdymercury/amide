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

#define EMPTY 0.0
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

  if (data_set->distribution != NULL)
    return;

#if AMIDE_DEBUG
  g_print("Generating distribution data for medical data set %s", AMITK_OBJECT_NAME(data_set));
#endif
  scale = (AMITK_DATA_SET_DISTRIBUTION_SIZE-1)/(data_set->global_max - data_set->global_min);
  
  if ((data_set->distribution = amitk_raw_data_new()) == NULL) {
    g_warning("couldn't allocate space for the data set structure to hold distribution data");
    return;
  }
  data_set->distribution->dim.x = AMITK_DATA_SET_DISTRIBUTION_SIZE;
  data_set->distribution->dim.y = 1;
  data_set->distribution->dim.z = 1;
  data_set->distribution->dim.t = 1;
  data_set->distribution->format = AMITK_FORMAT_DOUBLE;
  if ((data_set->distribution->data = amitk_raw_data_get_data_mem(data_set->distribution)) == NULL) {
    g_warning("couldn't allocate memory for data distribution for bar_graph");
    g_object_unref(data_set->distribution);
    data_set->distribution = NULL;
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
    divider = ((data_set->raw_data->dim.z/20.0) < 1) ? 1 : (data_set->raw_data->dim.z/20.0);
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
      log10(*AMITK_RAW_DATA_DOUBLE_POINTER(data_set->distribution,j)+1.0);

  return;
}


/* return a planar projection of the data set given the associated view */
AmitkDataSet * amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_get_projection(AmitkDataSet * data_set,
									       const AmitkView view,
									       const guint frame) {

  AmitkDataSet * projection;
  AmitkVoxel dim, i, j;
  AmitkPoint voxel_size;

  /* initialize the return projection */
  if ((projection = amitk_data_set_new()) == NULL) {
    g_warning("couldn't allocate space for the projection");
    return NULL;
  }
  if ((projection->raw_data = amitk_raw_data_new()) == NULL) {
    g_warning("couldn't allocate space for the projection's raw data structure");
    g_object_unref(projection);
    return NULL;
  }

  projection->slice_parent = g_object_ref(data_set);
  
  projection->raw_data->dim.z = 1;
  projection->raw_data->dim.t = 1;
  voxel_size = AMITK_DATA_SET_VOXEL_SIZE(data_set);
  switch(view) {
  case AMITK_VIEW_CORONAL:
    projection->raw_data->dim.x = AMITK_DATA_SET_DIM_X(data_set);
    projection->raw_data->dim.y = AMITK_DATA_SET_DIM_Z(data_set);
    projection->voxel_size.x = voxel_size.x;
    projection->voxel_size.y = voxel_size.z;
    projection->voxel_size.z = AMITK_VOLUME_Y_CORNER(data_set);
    break;
  case AMITK_VIEW_SAGITTAL:
    projection->raw_data->dim.x = AMITK_DATA_SET_DIM_Y(data_set);
    projection->raw_data->dim.y = AMITK_DATA_SET_DIM_Z(data_set);
    projection->voxel_size.x = voxel_size.y;
    projection->voxel_size.y = voxel_size.z;
    projection->voxel_size.z = AMITK_VOLUME_X_CORNER(data_set);
    break;
  case AMITK_VIEW_TRANSVERSE:
  default:
    projection->raw_data->dim.x = AMITK_DATA_SET_DIM_X(data_set);
    projection->raw_data->dim.y = AMITK_DATA_SET_DIM_Y(data_set);
    projection->voxel_size.x = voxel_size.x;
    projection->voxel_size.y = voxel_size.y;
    projection->voxel_size.z = AMITK_VOLUME_Z_CORNER(data_set);
    break;
  }

  projection->raw_data->format = AMITK_FORMAT_DOUBLE;
  amitk_space_copy_in_place(AMITK_SPACE(projection), AMITK_SPACE(data_set));
  amitk_data_set_calc_far_corner(projection);
  projection->scan_start = amitk_data_set_get_start_time(data_set, frame);
  amitk_data_set_set_thresholding(projection, AMITK_THRESHOLDING_GLOBAL);
  amitk_data_set_set_color_table(projection, AMITK_DATA_SET_COLOR_TABLE(data_set));

  if ((projection->frame_duration = amitk_data_set_get_frame_duration_mem(projection)) == NULL) {
    g_warning("couldn't allocate space for the projection frame duration array");
    g_object_unref(projection);
    return NULL;
  }
  amitk_data_set_set_frame_duration(projection, 0, amitk_data_set_get_frame_duration(data_set, frame));
  if ((projection->raw_data->data = amitk_raw_data_get_data_mem(projection->raw_data)) == NULL) {
    g_warning("couldn't allocate space for the projection, wanted %dx%dx%dx%d elements", 
	      AMITK_DATA_SET_DIM_X(projection), AMITK_DATA_SET_DIM_Y(projection),
	      AMITK_DATA_SET_DIM_Z(projection), AMITK_DATA_SET_DIM_T(projection));
    g_object_unref(projection);
    return NULL;
  }
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


  amitk_data_set_calc_frame_max_min(projection);
  amitk_data_set_calc_global_max_min(projection);
  amitk_data_set_set_threshold_max(projection, 0,
				   AMITK_DATA_SET_GLOBAL_MAX(projection));
  amitk_data_set_set_threshold_min(projection, 0,
				   AMITK_DATA_SET_GLOBAL_MIN(projection));

  return projection;
}

AmitkDataSet * amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_get_cropped(AmitkDataSet * data_set,
										const AmitkVoxel start,
										const AmitkVoxel end) {

  AmitkDataSet * cropped;
  AmitkVoxel i, j;
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
  temp_string = g_strdup_printf("Cropped %s", AMITK_OBJECT_NAME(data_set));
  amitk_object_set_name(AMITK_OBJECT(cropped), temp_string);
  g_free(temp_string);

  /* start the new building process */
  if ((cropped->raw_data = amitk_raw_data_new()) == NULL) {
    g_warning("couldn't allocate space for the cropped raw data set structure");
    goto error;
  }
  
  cropped->raw_data->format = data_set->raw_data->format;
  /* allocate the space for the data set */
  cropped->raw_data->dim = voxel_add(voxel_sub(end, start), one_voxel);
  if ((cropped->raw_data->data = amitk_raw_data_get_data_mem(cropped->raw_data)) == NULL) {
    g_warning("couldn't allocate space for the cropped raw data");
    goto error;
  }

  /* copy the raw data on over */
  for (i.t=0, j.t=start.t; j.t <= end.t; i.t++, j.t++) 
    for (i.z=0, j.z=start.z; j.z <= end.z; i.z++, j.z++) 
      for (i.y=0, j.y=start.y; j.y <= end.y; i.y++, j.y++) 
	for (i.x=0, j.x=start.x; j.x <= end.x; i.x++, j.x++) 
	  AMITK_RAW_DATA_`'m4_Variable_Type`'_SET_CONTENT(cropped->raw_data,i) =
	    AMITK_RAW_DATA_`'m4_Variable_Type`'_SET_CONTENT(data_set->raw_data,j);


  /* get space for the scale factors */
  if ((cropped->internal_scaling = amitk_raw_data_new()) == NULL) {
    g_warning("couldn't allocate space for the cropped internal scaling structure");
    goto error;
  }

  cropped->internal_scaling->format = AMITK_RAW_DATA_FORMAT(data_set->internal_scaling);
  cropped->internal_scaling->dim = one_voxel;
#if defined(DIM_TYPE_1D_SCALING) || defined(DIM_TYPE_2D_SCALING)
  cropped->internal_scaling->dim.t = end.t-start.t+1;
#endif
#if defined(DIM_TYPE_2D_SCALING)
  cropped->internal_scaling->dim.z = end.z-start.z+1;
#endif

  if ((cropped->internal_scaling->data = amitk_raw_data_get_data_mem(cropped->internal_scaling)) == NULL) {
    g_warning("couldn't allocate space for the cropped internal scale factors");
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

/* returns a slice  with the appropriate data from the data_set */
AmitkDataSet * amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_get_slice(AmitkDataSet * data_set,
									      const amide_time_t requested_start,
									      const amide_time_t requested_duration,
									      const amide_real_t pixel_dim,
									      const AmitkVolume * slice_volume,
									      const AmitkInterpolation interpolation,
									      const gboolean need_calc_max_min) {

  /* zp_start, where on the zp axis to start the slice, zp (z_prime) corresponds
     to the rotated axises, if negative, choose the midpoint */

  AmitkDataSet * slice;
  AmitkVoxel i,j;
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
  AmitkPoint corner;
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
  AmitkCorners intersection_corners;

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


  /* initialize the return slice */
  if ((slice = amitk_data_set_new()) == NULL) {
    g_warning("couldn't allocate space for the slice");
    return NULL;
  }
  if ((slice->raw_data = amitk_raw_data_new()) == NULL) {
    g_warning("couldn't allocate space for the slice's raw data structure");
    g_object_unref(slice);
    return NULL;
  }


  slice->slice_parent = g_object_ref(data_set);
  slice->raw_data->dim.x = ceil(fabs(AMITK_VOLUME_X_CORNER(slice_volume))/pixel_dim);
  slice->raw_data->dim.y = ceil(fabs(AMITK_VOLUME_Y_CORNER(slice_volume))/pixel_dim);
  slice->raw_data->dim.z = 1;
  slice->raw_data->dim.t = 1;
  slice->raw_data->format = AMITK_FORMAT_DOUBLE;
  slice->voxel_size.x = pixel_dim;
  slice->voxel_size.y = pixel_dim;
  slice->voxel_size.z = AMITK_VOLUME_Z_CORNER(slice_volume);
  amitk_space_copy_in_place(AMITK_SPACE(slice), AMITK_SPACE(slice_volume));
  amitk_data_set_calc_far_corner(slice);
  corner = AMITK_VOLUME_CORNER(slice);
  slice->scan_start = start_time;
#if AMIDE_DEBUG
  center_point = amitk_volume_center(slice_volume);
  temp_string =  
    g_strdup_printf("slice from data_set %s: @ x %5.3f y %5.3f z %5.3f", AMITK_OBJECT_NAME(data_set), 
		    center_point.x, center_point.y, center_point.z);
  amitk_object_set_name(AMITK_OBJECT(slice),temp_string);
  g_free(temp_string);
#endif
  slice->thresholding = data_set->thresholding;

  if ((slice->frame_duration = amitk_data_set_get_frame_duration_mem(slice)) == NULL) {
    g_warning("couldn't allocate space for the slice frame duration array");
    g_object_unref(slice);
    return NULL;
  }
  amitk_data_set_set_frame_duration(slice, 0, end_time-start_time);
  if ((slice->raw_data->data = amitk_raw_data_get_data_mem(slice->raw_data)) == NULL) {
    g_warning("couldn't allocate space for the slice, wanted %dx%dx%dx%d elements", 
	      AMITK_DATA_SET_DIM_X(slice), AMITK_DATA_SET_DIM_Y(slice),
	      AMITK_DATA_SET_DIM_Z(slice), AMITK_DATA_SET_DIM_T(slice));
    g_object_unref(slice);
    return NULL;
  }
  /* initialize our data set */
  amitk_raw_data_DOUBLE_initialize_data(slice->raw_data, 0.0);


#ifdef AMIDE_DEBUG_COMMENT_OUT
  {
    AmitkCorners real_corner;
    /* convert to real space */
    real_corner[0] = rs_offset(slice->space);
    real_corner[1] = amitk_space_s2b(slice->space, slice->corner);
    g_print("new slice from data_set %s\t---------------------\n",AMITK_OBJECT_NAME(data_set));
    g_print("\tdim\t\tx %d\t\ty %d\t\tz %d\n",
    	    slice->raw_data->dim.x, slice->raw_data->dim.y, slice->raw_data->dim.z);
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

  if ((end.y > slice->raw_data->dim.y) || (end.x > slice->raw_data->dim.x) ||
      (start.y < 0) || (start.x < 0)) {
    g_object_unref(slice);
    g_warning("inappropriate dimensions for slice: %d %d %d %d, wanted within 0 0 %d %d", 
	      start.x, start.y, end.x, end.y, slice->raw_data->dim.x, slice->raw_data->dim.y);
    return NULL;
  }

  switch(interpolation) {

  case AMITK_INTERPOLATION_TRILINEAR:
    i.t = i.z = 0;
    /* iterate over the frames we'll be incorporating into this slice */
    for (i_frame = start_frame; i_frame <= end_frame; i_frame++) {
      //start_frame+num_frames;i_frame++) {
      
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
	for (i.y = start.y; i.y < end.y; i.y++) {

	  /* the slice y_coordinate of the center of this iteration's slice voxel */
	  slice_point.y = (((amide_real_t) i.y)+0.5)*slice->voxel_size.y;
	  
	  /* the slice x coord of the center of the first slice voxel in this loop */
	  slice_point.x = (((amide_real_t) start.x)+0.5)*slice->voxel_size.x;

	  /* iterate over the x dimension */
	  for (i.x = start.x; i.x <= end.x; i.x++) {

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
		box_value[l] = EMPTY;
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

	    AMITK_RAW_DATA_DOUBLE_SET_CONTENT(slice->raw_data,i)+=weight*box_value[0];

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

    i.t = i.z = 0;
    /* iterate over the number of frames we'll be incorporating into this slice */
    for (i_frame = start_frame; i_frame <= end_frame; i_frame++) {
      //start_frame+num_frames;i_frame++) {

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
	for (i.y = start.y; i.y < end.y; i.y++) {
	  last[AMITK_AXIS_Y] = ds_point;

	  /* and iteratate over x */
	  for (i.x = start.x; i.x <= end.x; i.x++) {
	    POINT_TO_VOXEL(ds_point, data_set->voxel_size, i_frame, j);
	    if (!amitk_raw_data_includes_voxel(data_set->raw_data,j))
	      AMITK_RAW_DATA_DOUBLE_SET_CONTENT(slice->raw_data,i) += weight*EMPTY;
	    else
	      AMITK_RAW_DATA_DOUBLE_SET_CONTENT(slice->raw_data,i) += 
		weight*AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENT(data_set,j);
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


