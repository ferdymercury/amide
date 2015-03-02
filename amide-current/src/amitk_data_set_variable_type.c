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

/* function to recalcule the max and min values of a data set */
void amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_calc_frame_max_min(AmitkDataSet * data_set) {

  AmitkVoxel i;
  amide_data_t max, min, temp;
  AmitkVoxel initial_voxel;

  
  initial_voxel = zero_voxel;
  for (i.t = 0; i.t < AMITK_DATA_SET_NUM_FRAMES(data_set); i.t++) {
    initial_voxel.t = i.t;
    temp = AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENTS(data_set, initial_voxel);
    if (finite(temp)) max = min = temp;   
    else max = min = 0.0; /* just throw in zero */

    for (i.z = 0; i.z < data_set->raw_data->dim.z; i.z++) 
      for (i.y = 0; i.y < data_set->raw_data->dim.y; i.y++) 
	for (i.x = 0; i.x < data_set->raw_data->dim.x; i.x++) {
	  temp = AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENTS(data_set, i);
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
  data_set->distribution->format = AMITK_FORMAT_FLOAT;
  if ((data_set->distribution->data = amitk_raw_data_get_data_mem(data_set->distribution)) == NULL) {
    g_warning("couldn't allocate memory for data distribution for bar_graph");
    g_object_unref(data_set->distribution);
    data_set->distribution = NULL;
    return;
  }
  
  /* initialize the distribution array */
  amitk_raw_data_FLOAT_initialize_data(data_set->distribution, 0.0);
  
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
	    j.x = scale*(AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENTS(data_set,i)-data_set->global_min);
	    AMITK_RAW_DATA_FLOAT_SET_CONTENT(data_set->distribution,j) += 1.0;
	  }
    }
  }
#if AMIDE_DEBUG
  g_print("\n");
#endif
  
  
  /* do some log scaling so the distribution is more meaningful, and doesn't get
     swamped by outlyers */
  for (j.x = 0; j.x < data_set->distribution->dim.x ; j.x++) 
    AMITK_RAW_DATA_FLOAT_SET_CONTENT(data_set->distribution,j) = 
      log10(*AMITK_RAW_DATA_FLOAT_POINTER(data_set->distribution,j)+1.0);

  return;
}


/* returns a slice  with the appropriate data from the data_set */
AmitkDataSet * amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_get_slice(AmitkDataSet * data_set,
									      const amide_time_t requested_start,
									      const amide_time_t requested_duration,
									      const AmitkPoint  requested_voxel_size,
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
  amide_data_t box_value[8];
  AmitkPoint slice_point, ds_point,diff, nearest_point;
  AmitkSpace * slice_space;
  AmitkSpace * data_set_space;
#if AMIDE_DEBUG
  AmitkPoint center_point;
#endif
  AmitkVoxel ds_voxel;
  amide_data_t weight1, weight2;
  amide_time_t start, end;

  /* ----- figure out what frames of this data set to include ----*/
  start = requested_start;
  start_frame = amitk_data_set_get_frame(data_set, start);
  start_time = amitk_data_set_get_start_time(data_set, start_frame);

  end = requested_start+requested_duration;
  end_frame = amitk_data_set_get_frame(data_set, end);
  end_time = amitk_data_set_get_end_time(data_set, end_frame);

  if (start_frame == end_frame) {
    start = start_time;
    end = end_time;
  } else {
    if (end_time < end)
      end = end_time;
    if (start_time > start)
      start = start_time;
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
  slice->raw_data->dim.x = ceil(fabs(AMITK_VOLUME_X_CORNER(slice_volume))/requested_voxel_size.x);
  slice->raw_data->dim.y = ceil(fabs(AMITK_VOLUME_Y_CORNER(slice_volume))/requested_voxel_size.y);
  slice->raw_data->dim.z = 1;
  slice->raw_data->dim.t = 1;
  slice->raw_data->format = AMITK_FORMAT_FLOAT;
  slice->voxel_size = requested_voxel_size;
  amitk_space_copy_in_place(AMITK_SPACE(slice), AMITK_SPACE(slice_volume));
  amitk_data_set_calc_far_corner(slice);
  corner = AMITK_VOLUME_CORNER(slice);
  slice->scan_start = start;
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
  slice->frame_duration[0] = end-start;
  if ((slice->raw_data->data = amitk_raw_data_get_data_mem(slice->raw_data)) == NULL) {
    g_warning("couldn't allocate space for the slice, wanted %dx%dx%dx%d elements", 
	      slice->raw_data->dim.x, slice->raw_data->dim.y, 
	      slice->raw_data->dim.z, slice->raw_data->dim.t );
    g_object_unref(slice);
    return NULL;
  }


#ifdef AMIDE_DEBUG_COMMENT_OUT
  {
    AmitkPoint real_corner[2];
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

  /* initialize our data set */
  amitk_raw_data_FLOAT_initialize_data(slice->raw_data, 0.0);

  switch(interpolation) {

  case AMITK_INTERPOLATION_TRILINEAR:
    i.t = i.z = 0;
    /* iterate over the frames we'll be incorporating into this slice */
    for (i_frame = start_frame; i_frame <= end_frame; i_frame++) {
      //start_frame+num_frames;i_frame++) {
      
      /* averaging over more then one frame */
      if (end_frame-start_frame > 0) {
	if (i_frame == start_frame)
	  time_weight = (amitk_data_set_get_end_time(data_set, start_frame)-start)/(end-start);
	else if (i_frame == end_frame)
	  time_weight = (end-amitk_data_set_get_start_time(data_set, end_frame))/(end-start);
	else
	  time_weight = data_set->frame_duration[i_frame]/(end-start);
      } else
	time_weight = 1.0;

      /* iterate over the number of planes we'll be compressing into this slice */
      for (z = 0; z < ceil(z_steps*(1.0-SMALL_DISTANCE))*(1.0-SMALL_DISTANCE); z++) {
	
	/* the slices z_coordinate for this iteration's slice voxel */
	slice_point.z = z*voxel_length+voxel_length/2.0;

	/* weight is between 0 and 1, this is used to weight the last voxel  in the slice's z direction */
	if (floor(z_steps) > z)
	  weight = time_weight/z_steps;
	else
	  weight = time_weight*(z_steps-floor(z_steps)) / z_steps;

	/* iterate over the y dimension */
	for (i.y = 0; i.y < slice->raw_data->dim.y; i.y++) {

	  /* the slice y_coordinate of the center of this iteration's slice voxel */
	  slice_point.y = (((amide_real_t) i.y)*corner.y/slice->raw_data->dim.y) + slice->voxel_size.y/2.0;
	  
	  /* the slice x coord of the center of the first slice voxel in this loop */
	  slice_point.x = slice->voxel_size.x/2.0;

	  /* iterate over the x dimension */
	  for (i.x = 0; i.x < slice->raw_data->dim.x; i.x++) {

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
		box_value[l] = AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENTS(data_set, box_voxel[l]);
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

	    AMITK_RAW_DATA_FLOAT_SET_CONTENT(slice->raw_data,i)+=weight*box_value[0];

	    slice_point.x += slice->voxel_size.x; 
	  }
	}
      }
    }
    break;
  case AMITK_INTERPOLATION_NEAREST_NEIGHBOR:
  default:  
    /* figure out what point in the data set we're going to start at */
    alt.x = slice->voxel_size.x/2.0;
    alt.y = slice->voxel_size.y/2.0;
    alt.z = voxel_length/2.0;
    ds_point = amitk_space_s2s(slice_space, data_set_space, alt);

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
	  time_weight = (amitk_data_set_get_end_time(data_set, start_frame)-start)/(end-start);
	else if (i_frame == end_frame)
	  time_weight = (end-amitk_data_set_get_start_time(data_set, end_frame))/(end-start);
	else
	  time_weight = data_set->frame_duration[i_frame]/(end-start);
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
	for (i.y = 0; i.y < slice->raw_data->dim.y; i.y++) {
	  last[AMITK_AXIS_Y] = ds_point;

	  /* and iteratate over x */
	  for (i.x = 0; i.x < slice->raw_data->dim.x; i.x++) {
	    POINT_TO_VOXEL(ds_point, data_set->voxel_size, i_frame, j);
	    if (!amitk_raw_data_includes_voxel(data_set->raw_data,j))
	      AMITK_RAW_DATA_FLOAT_SET_CONTENT(slice->raw_data,i) += weight*EMPTY;
	    else
	      AMITK_RAW_DATA_FLOAT_SET_CONTENT(slice->raw_data,i) += 
		weight*AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENTS(data_set,j);
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


