/* amitk_data_set_variable_type.c - used to generate the different amitk_data_set_*.c files
 *
 * Part of amide - Amide's a Medical Image Data Examiner
 * Copyright (C) 2001-2014 Andy Loening
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
#include "amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'.h"
#include "amitk_data_set_FLOAT_0D_SCALING.h"

#ifdef AMIDE_DEBUG
#include <stdlib.h>
#endif

#define DIM_TYPE_`'m4_Scale_Dim`'
#define DATA_TYPE_`'m4_Variable_Type`'


/* function to calculate the max/min values of a slice within a data set */
void amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_`'m4_Intercept`'calc_slice_min_max(AmitkDataSet * data_set,
											     const amide_intpoint_t frame,
											     const amide_intpoint_t gate,
											     const amide_intpoint_t z,
											     amitk_format_DOUBLE_t * pmin,
											     amitk_format_DOUBLE_t * pmax) {

  AmitkVoxel i;
  amide_data_t max, min, temp;
  AmitkVoxel dim;
  
  dim = AMITK_DATA_SET_DIM(data_set);

  i.t = frame;
  i.g = gate;
  i.z = z;
  i.y = i.x = 0;

  temp = AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_`'m4_Intercept`'CONTENT(data_set, i);
  if (finite(temp)) max = min = temp;   
  else max = min = 0.0; /* just throw in zero */

  for (i.y = 0; i.y < dim.y; i.y++) 
    for (i.x = 0; i.x < dim.x; i.x++) {
      temp = AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_`'m4_Intercept`'CONTENT(data_set, i);
      if (finite(temp)) {
	if (temp > max) max = temp;
	else if (temp < min) min = temp;
      }
    }

  if (pmin != NULL)
    *pmin = min;

  if (pmax != NULL)
    *pmax = max;
  
  return;
}

/* generate the distribution array for a data_set */
void amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_`'m4_Intercept`'calc_distribution(AmitkDataSet * data_set,
									    AmitkUpdateFunc update_func,
									    gpointer update_data) {

  AmitkVoxel i,j;
  amide_data_t scale, diff;
  AmitkVoxel distribution_dim;
  AmitkVoxel data_set_dim;
  gchar * temp_string;
  div_t x;
  gint divider;
  gint total_planes;
  gint i_plane;
  gboolean continue_work=TRUE;
  AmitkRawData * distribution;

  if (data_set->distribution != NULL)
    return;

  data_set_dim = AMITK_DATA_SET_DIM(data_set);
  diff = amitk_data_set_get_global_max(data_set) - amitk_data_set_get_global_min(data_set);
  if (diff == 0.0)
    scale = 0.0;
  else
    scale = (AMITK_DATA_SET_DISTRIBUTION_SIZE-1)/diff;
  
  distribution_dim.x = AMITK_DATA_SET_DISTRIBUTION_SIZE;
  distribution_dim.y = distribution_dim.z = distribution_dim.g = distribution_dim.t = 1;
  distribution = amitk_raw_data_new_with_data(AMITK_FORMAT_DOUBLE, distribution_dim);
  if (distribution == NULL) {
    g_warning(_("couldn't allocate memory space for the data set structure to hold distribution data"));
    return;
  }

  /* initialize the distribution array */
  amitk_raw_data_DOUBLE_initialize_data(distribution, 0.0);
  
  if (update_func != NULL) {
    temp_string = g_strdup_printf(_("Generating distribution data for:\n   %s"), AMITK_OBJECT_NAME(data_set));
    continue_work = (*update_func)(update_data, temp_string, (gdouble) 0.0);
    g_free(temp_string);
  }
  total_planes = AMITK_DATA_SET_TOTAL_PLANES(data_set);
  divider = ((total_planes/AMITK_UPDATE_DIVIDER) < 1) ? 1 : (total_planes/AMITK_UPDATE_DIVIDER);

  /* now "bin" the data */
  j = zero_voxel;
  i_plane=0;
  for (i.t = 0; (i.t < data_set_dim.t) && continue_work; i.t++) {
    for (i.g = 0; (i.g < data_set_dim.g) && continue_work; i.g++) {
      for ( i.z = 0; (i.z < data_set_dim.z) && continue_work; i.z++, i_plane++) {
	if (update_func != NULL) {
	  x = div(i_plane,divider);
	  if (x.rem == 0)
	    continue_work = (*update_func)(update_data, NULL, ((gdouble) i_plane)/((gdouble) total_planes));
	}

	for (i.y = 0; i.y < data_set_dim.y; i.y++) 
	  for (i.x = 0; i.x < data_set_dim.x; i.x++) {
	    j.x = scale*(AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_`'m4_Intercept`'CONTENT(data_set,i)-amitk_data_set_get_global_min(data_set));
	    AMITK_RAW_DATA_DOUBLE_SET_CONTENT(distribution,j) += 1.0;
	  }
      }
    }
  }

  if (update_func != NULL) /* remove progress bar */
    continue_work = (*update_func)(update_data, NULL, (gdouble) 2.0); 

  if (!continue_work) {   /* if we quit, get out of here */
    g_object_unref(distribution);
    return;
  }
  
  /* do some log scaling so the distribution is more meaningful, and doesn't get
     swamped by outlyers */
  for (j.x = 0; j.x < distribution_dim.x ; j.x++) 
    AMITK_RAW_DATA_DOUBLE_SET_CONTENT(distribution,j) = 
      log10(AMITK_RAW_DATA_DOUBLE_CONTENT(distribution,j)+1.0);

  /* and store the distribution with the data set */
  data_set->distribution = distribution;
  return;
}




/* returns a slice  with the appropriate data from the data_set */
AmitkDataSet * amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_`'m4_Intercept`'get_slice(AmitkDataSet * data_set,
											      const amide_time_t start_time,
											      const amide_time_t duration,
											      const amide_intpoint_t gate,
											      const AmitkCanvasPoint pixel_size,
											      const AmitkVolume * slice_volume) {

  /* zp_start, where on the zp axis to start the slice, zp (z_prime) corresponds
     to the rotated axises, if negative, choose the midpoint */

  AmitkDataSet * slice = NULL;
  AmitkVoxel i_voxel;
  amide_intpoint_t z;
  amide_real_t max_diff, voxel_length, z_steps;
  AmitkPoint alt;
  AmitkPoint stride[AMITK_AXIS_NUM], last[AMITK_AXIS_NUM];
  AmitkAxis i_axis;
  guint k, l;
  amide_data_t weight;
  amide_data_t time_weight;
  amide_intpoint_t start_frame, end_frame;
  amide_intpoint_t i_gate;
  amide_time_t end_time;
  AmitkPoint box_point[8];
  AmitkVoxel box_voxel[8];
  AmitkVoxel start, end;
  amide_data_t box_value[8];
  AmitkPoint slice_point, ds_point,start_point,diff, nearest_point;
  AmitkSpace * slice_space;
  AmitkSpace * data_set_space;
#if AMIDE_DEBUG
  gchar * temp_string;
  AmitkPoint center_point;
#endif
  AmitkVoxel ds_voxel;
  amide_data_t weight1, weight2;
  amide_data_t * weights=NULL;
  amide_data_t * intermediate_data=NULL;
  AmitkCorners intersection_corners;
  AmitkVoxel dim;
  gint num_gates;
  gboolean empties=FALSE;

  /* ----- figure out what frames of this data set to include ----*/
  end_time = start_time+duration;
  start_frame = amitk_data_set_get_frame(data_set, start_time+EPSILON);
  end_frame = amitk_data_set_get_frame(data_set, end_time-EPSILON);

  /* the number of gates we'll be looking at */
  if (gate < 0)
    num_gates = AMITK_DATA_SET_NUM_VIEW_GATES(data_set);
  else
    num_gates = 1;

  /* ------------------------- */

  dim.x = ceil(fabs(AMITK_VOLUME_X_CORNER(slice_volume))/pixel_size.x);
  dim.y = ceil(fabs(AMITK_VOLUME_Y_CORNER(slice_volume))/pixel_size.y);
  dim.z = dim.g = dim.t = 1;

  /* if we need it, get the weighting matrix */
  if (data_set->rendering == AMITK_RENDERING_MPR) {
    if ((weights = g_try_malloc0(sizeof(amide_data_t)*dim.x*dim.y)) == NULL) {
      g_warning(_("couldn't allocate memory space for the weights, wanted %dx%d elements"), dim.x, dim.y);
      goto error;
    }
  }

  /* get an intermediate data matrix to speed things up */
  if ((intermediate_data = g_try_malloc0(sizeof(amide_data_t)*dim.x*dim.y)) == NULL) {
    g_warning(_("couldn't allocate memory space for the intermediate_data, wanted %dx%d elements"), dim.x, dim.y);
    goto error;
  }

  /* get the return slice */
  slice = amitk_data_set_new_with_data(NULL, AMITK_DATA_SET_MODALITY(data_set), 
				       AMITK_FORMAT_DOUBLE, dim, AMITK_SCALING_TYPE_0D);
  if (slice == NULL) {
    g_warning(_("couldn't allocate memory space for the slice, wanted %dx%dx%d elements"), 
	      dim.x, dim.y, dim.z);
    goto error;
  }

  slice->slice_parent = data_set;
  g_object_add_weak_pointer(G_OBJECT(data_set), 
			    (gpointer *) &(slice->slice_parent));
  slice->voxel_size.x = pixel_size.x;
  slice->voxel_size.y = pixel_size.y;
  slice->voxel_size.z = AMITK_VOLUME_Z_CORNER(slice_volume);
  amitk_space_copy_in_place(AMITK_SPACE(slice), AMITK_SPACE(slice_volume));
  slice->scan_start = start_time;
  slice->thresholding = data_set->thresholding;
  slice->interpolation = AMITK_DATA_SET_INTERPOLATION(data_set);
  slice->rendering = AMITK_DATA_SET_RENDERING(data_set);
  if (gate < 0) {
    slice->view_start_gate = AMITK_DATA_SET_VIEW_START_GATE(data_set);
    slice->view_end_gate = AMITK_DATA_SET_VIEW_END_GATE(data_set);
  } else {
    slice->view_start_gate = gate;
    slice->view_end_gate = gate;
  }

  amitk_data_set_calc_far_corner(slice);
  amitk_data_set_set_frame_duration(slice, 0, duration);

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
    real_corner[0] = AMITK_SPACE_OFFSET(slice);
    real_corner[1] = amitk_space_s2b(AMITK_SPACE(slice), AMITK_VOLUME_CORNER(slice));
    g_print("new slice from data_set %s\t---------------------\n",AMITK_OBJECT_NAME(data_set));
    g_print("\tdim\t\tx %d\t\ty %d\t\tz %d\n",
    	    dim.x, dim.y, dim.z);
    g_print("\treal corner[0]\tx %5.4f\ty %5.4f\tz %5.4f\n",
    	    real_corner[0].x,real_corner[0].y,real_corner[0].z);
    g_print("\treal corner[1]\tx %5.4f\ty %5.4f\tz %5.4f\n",
    	    real_corner[1].x,real_corner[1].y,real_corner[1].z);
    g_print("\tdata set\t\tstart\t%5.4f\tend\t%5.3f\tframes %d to %d\n",
    	    start_time, end_time,start_frame,end_frame);
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
  z_steps = slice->voxel_size.z/voxel_length; /* non-integer */

  /* figure out the intersection bounds between the data set and the requested slice volume */
  if (amitk_volume_volume_intersection_corners(slice_volume, 
					       AMITK_VOLUME(data_set), 
					       intersection_corners)) {
    /* translate the intersection into voxel space */
    POINT_TO_VOXEL(intersection_corners[0], slice->voxel_size, 0, 0, start);
    POINT_TO_VOXEL(intersection_corners[1], slice->voxel_size, 0, 0, end);
  } else { /* no intersection */
    start = zero_voxel;
    end = zero_voxel;
  }

  /* make sure we only iterate over the slice we've already malloc'ed */
  if (start.x < 0) start.x = 0;
  if (start.y < 0) start.y = 0;
  if (end.x >= dim.x) end.x = dim.x-1;
  if (end.y >= dim.y) end.y = dim.y-1;

  /* iterate over those voxels that we won't be covering, and mark them as NAN */
  i_voxel.t = i_voxel.g = i_voxel.z = 0;
  for (i_voxel.y = 0; i_voxel.y < start.y; i_voxel.y++) 
    for (i_voxel.x = 0; i_voxel.x < dim.x; i_voxel.x++) 
      AMITK_RAW_DATA_DOUBLE_SET_CONTENT(slice->raw_data,i_voxel) = NAN;
  for (i_voxel.y = end.y+1; i_voxel.y < dim.y; i_voxel.y++) 
    for (i_voxel.x = 0; i_voxel.x < dim.x; i_voxel.x++) 
      AMITK_RAW_DATA_DOUBLE_SET_CONTENT(slice->raw_data,i_voxel) = NAN;
  for (i_voxel.x = 0; i_voxel.x < start.x; i_voxel.x++) 
    for (i_voxel.y = 0; i_voxel.y < dim.y; i_voxel.y++) 
      AMITK_RAW_DATA_DOUBLE_SET_CONTENT(slice->raw_data,i_voxel) = NAN;
  for (i_voxel.x = end.x+1; i_voxel.x < dim.x; i_voxel.x++) 
    for (i_voxel.y = 0; i_voxel.y < dim.y; i_voxel.y++) 
      AMITK_RAW_DATA_DOUBLE_SET_CONTENT(slice->raw_data,i_voxel) = NAN;


  switch(data_set->interpolation) {
    
  case AMITK_INTERPOLATION_TRILINEAR:

    /* iterate over the frames we'll be incorporating into this slice */
    for (ds_voxel.t = start_frame; ds_voxel.t <= end_frame; ds_voxel.t++) {
      
      /* averaging over more then one frame */
      if (end_frame-start_frame > 0) {
	if (ds_voxel.t == start_frame)
	  time_weight = (amitk_data_set_get_end_time(data_set, start_frame)-start_time)/(duration*num_gates);
	else if (ds_voxel.t == end_frame)
	  time_weight = (end_time-amitk_data_set_get_start_time(data_set, end_frame))/(duration*num_gates);
	else
	  time_weight = amitk_data_set_get_frame_duration(data_set, ds_voxel.t)/(duration*num_gates);
      } else
	time_weight = 1.0/((gdouble) num_gates);
      
      for (i_gate=0; i_gate < num_gates; i_gate++) {
	if (gate < 0)
	  ds_voxel.g = i_gate+AMITK_DATA_SET_VIEW_START_GATE(data_set);
	else
	  ds_voxel.g = i_gate+gate;
	
	if (ds_voxel.g >= AMITK_DATA_SET_NUM_GATES(data_set))
	  ds_voxel.g -= AMITK_DATA_SET_NUM_GATES(data_set);

	/* initialize the .t/.g components of box_voxel */
	for (l=0; l<8; l=l+1) {
	  box_voxel[l].t = ds_voxel.t;
	  box_voxel[l].g = ds_voxel.g;
	}

	/* iterate over the number of planes we'll be compressing into this slice */
	for (z = 0; z < ceil(z_steps); z++) {
	  
	  /* the slices z_coordinate for this iteration's slice voxel */
	  if (ceil(z_steps) > 1.0)
	    slice_point.z = (z+0.5)*voxel_length;
	  else
	    slice_point.z = (0.5)*slice->voxel_size.z; /* only one iteration in z */
	  
	  /* weight is between 0 and 1, this is used to weight the last voxel in the slice's z direction */
	  if (floor(z_steps) > z)
	    weight = time_weight/z_steps;
	  else
	    weight = time_weight*(z_steps-floor(z_steps)) / z_steps;
	  
	  /* iterate over the y dimension */
	  for (i_voxel.y = start.y,k=0; i_voxel.y <= end.y; i_voxel.y++) {
	    
	    /* the slice y_coordinate of the center of this iteration's slice voxel */
	    slice_point.y = (((amide_real_t) i_voxel.y)+0.5)*slice->voxel_size.y;
	    
	    /* the slice x coord of the center of the first slice voxel in this loop */
	    slice_point.x = (((amide_real_t) start.x)+0.5)*slice->voxel_size.x;
	    
	    /* iterate over the x dimension */
	    for (i_voxel.x = start.x; i_voxel.x <= end.x; i_voxel.x++,k++) {
	      
	      /* translate the current point in slice space into the data set's coordinate frame */
	      ds_point = amitk_space_s2s(slice_space, data_set_space, slice_point);
	      
	      /* get the nearest neighbor in the data set to this slice voxel */
	      POINT_TO_VOXEL_COORDS_ONLY(ds_point, data_set->voxel_size, ds_voxel);
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
		
		VOXEL_TO_POINT(box_voxel[l], data_set->voxel_size, box_point[l]);
		
		/* get the value of the point on the box */
		if (amitk_raw_data_includes_voxel(data_set->raw_data, box_voxel[l]))
		  box_value[l] = AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_`'m4_Intercept`'CONTENT(data_set, box_voxel[l]);
		else {
		  box_value[l] = NAN;
		  empties = TRUE;
		}
	      }
	      
	      if (empties) { /* slow algorithm - checking for empties */
		/* reset value */
		empties = FALSE; 

		/* do the x direction linear interpolation of the sets of two points */
		for (l=0;l<8;l=l+2) {
		  max_diff = box_point[l+1].x-box_point[l].x;
		  weight1 = ((max_diff - (ds_point.x - box_point[l].x))/max_diff);
		  weight2 = ((max_diff - (box_point[l+1].x - ds_point.x))/max_diff);
		  if (isnan(box_value[l])) {
		    if (weight2 >= weight1)
		      box_value[l] = box_value[l+1];
		    /* else box_value[l] left as is (NAN/empty) */
		  } else if (isnan(box_value[l+1])) {
		    if (weight1 < weight2)
		      box_value[l] = NAN;
		    /* else box_value[l] left as is */
		  } else
		    box_value[l] = (box_value[l] * weight1) + (box_value[l+1] * weight2);
		}
		
		/* do the y direction linear interpolation of the sets of two points */
		for (l=0;l<8;l=l+4) {
		  max_diff = box_point[l+2].y-box_point[l].y;
		  weight1 = ((max_diff - (ds_point.y - box_point[l].y))/max_diff);
		  weight2 = ((max_diff - (box_point[l+2].y - ds_point.y))/max_diff);
		  if (isnan(box_value[l])) {
		    if (weight2 >= weight1)
		      box_value[l] = box_value[l+2];
		    /* else box_value[l] left as is (NAN/empty) */
		  } else if (isnan(box_value[l+2])) {
		    if (weight1 < weight2)
		      box_value[l] = NAN;
		    /* else box_value[l] left as is */
		  } else
		    box_value[l] = (box_value[l] * weight1) + (box_value[l+2] * weight2);
		}
		
		/* do the z direction linear interpolation of the sets of two points */
		for (l=0;l<8;l=l+8) {
		  max_diff = box_point[l+4].z-box_point[l].z;
		  weight1 = ((max_diff - (ds_point.z - box_point[l].z))/max_diff);
		  weight2 = ((max_diff - (box_point[l+4].z - ds_point.z))/max_diff);
		  if (isnan(box_value[l])) {
		    if (weight2 >= weight1)
		      box_value[l] = box_value[l+4];
		    /* else box_value[l] left as is (NAN/empty) */
		  } else if (isnan(box_value[l+4])) {
		    if (weight1 < weight2)
		      box_value[l] = NAN;
		    /* else box_value[l] left as is */
		  } else
		    box_value[l] = (box_value[l] * weight1) + (box_value[l+4] * weight2);
		}

		/* separate into MPR/MIP/minIP algorithms */
		if (data_set->rendering == AMITK_RENDERING_MPR) { /* MPR */
		  if (!isnan(box_value[0])) {
		    intermediate_data[k] += weight*box_value[0];
		    weights[k] += weight;
		  }
		} else { /* MIP or MINIP */
		  if ((z == 0) && (ds_voxel.t == start_frame) && (i_gate == 0)) 
		    intermediate_data[k]=box_value[0];
		  else if (data_set->rendering == AMITK_RENDERING_MIP)  /* MIP */
		    intermediate_data[k] = MAX(box_value[0], intermediate_data[k]);
		  else  /* MINIP */
		    intermediate_data[k] = MIN(box_value[0], intermediate_data[k]);
		}

	      } else { /* faster */
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

		/* separate into MPR/MIP/minIP algorithms */
		if (data_set->rendering == AMITK_RENDERING_MPR) { /* MPR */
		  intermediate_data[k] += weight*box_value[0];
		  weights[k] += weight;
		} else { /* MIP or MINIP */
		  if ((z == 0) && (ds_voxel.t == start_frame) && (i_gate == 0)) 
		    intermediate_data[k]=box_value[0];
		  else if (data_set->rendering == AMITK_RENDERING_MIP)  /* MIP */
		    intermediate_data[k] = MAX(intermediate_data[k], box_value[0]);
		  else  /* MINIP */
		    intermediate_data[k] = MIN(intermediate_data[k], box_value[0]);
		}
	      } /* slow (empties) vs fast algorithm */
	      
	      slice_point.x += slice->voxel_size.x; 
	    }
	  }
	}
      }
    }
    break;

  case AMITK_INTERPOLATION_NEAREST_NEIGHBOR:
  default:  
    /* figure out what point in the data set we're going to start at */
    start_point.x = ((amide_real_t) start.x+0.5) * slice->voxel_size.x;
    start_point.y = ((amide_real_t) start.y+0.5) * slice->voxel_size.y;
    if (ceil(z_steps) > 1.0)
      start_point.z = voxel_length/2.0;
    else
      start_point.z = slice->voxel_size.z/2.0; /* only one iteration in z */
    start_point = amitk_space_s2s(slice_space, data_set_space, start_point);

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
    for (ds_voxel.t = start_frame; ds_voxel.t <= end_frame; ds_voxel.t++) {

      /* averaging over more then one frame */
      if (end_frame-start_frame > 0) {
	if (ds_voxel.t == start_frame)
	  time_weight = (amitk_data_set_get_end_time(data_set, start_frame)-start_time)/(duration*num_gates);
	else if (ds_voxel.t == end_frame)
	  time_weight = (end_time-amitk_data_set_get_start_time(data_set, end_frame))/(duration*num_gates);
	else
	  time_weight = amitk_data_set_get_frame_duration(data_set, ds_voxel.t)/(duration*num_gates);
      } else
	time_weight = 1.0/((gdouble) num_gates);

      /* iterate over gates */
      for (i_gate=0; i_gate < num_gates; i_gate++) {
	if (gate < 0)
	  ds_voxel.g = i_gate+AMITK_DATA_SET_VIEW_START_GATE(data_set);
	else
	  ds_voxel.g = i_gate+gate;

	if (ds_voxel.g >= AMITK_DATA_SET_NUM_GATES(data_set))
	  ds_voxel.g -= AMITK_DATA_SET_NUM_GATES(data_set);

	ds_point = start_point;

	/* separate into MPR and MIP/MINIP algorithms. A fair amount
	   of code is duplicated within the algorithms. The reason
	   they aren't combined is to keep the MPR vs MIP/MINIP branch
	   point out of the loop and speed things up slightly for the
	   most commonly used selection (MPR) */

	switch(data_set->rendering) {

	case AMITK_RENDERING_MPR:
	  /* iterate over the number of planes we'll be compressing into this slice */
	  for (z = 0; z < ceil(z_steps); z++) { 
	    last[AMITK_AXIS_Z] = ds_point;
	  
	    /* weight is between 0 and 1, this is used to weight the last voxel  in the slice's z direction */
	    if (floor(z_steps) > z)
	      weight = time_weight/z_steps;
	    else
	      weight = time_weight*(z_steps-floor(z_steps)) / z_steps;
	  
	    /* iterate over x and y */
	    for (i_voxel.y = start.y, k=0; i_voxel.y <= end.y; i_voxel.y++) { 
	      last[AMITK_AXIS_Y] = ds_point;
	      for (i_voxel.x = start.x; i_voxel.x <= end.x; i_voxel.x++, k++) { 
		POINT_TO_VOXEL_COORDS_ONLY(ds_point, data_set->voxel_size, ds_voxel);
		if (amitk_raw_data_includes_voxel(data_set->raw_data,ds_voxel)) {
		  intermediate_data[k] +=
		    weight*AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_`'m4_Intercept`'CONTENT(data_set,ds_voxel);
		  weights[k] += weight;
		}
		POINT_ADD(ds_point, stride[AMITK_AXIS_X], ds_point); 
	      } /* x */
	      POINT_ADD(last[AMITK_AXIS_Y], stride[AMITK_AXIS_Y], ds_point);
	    } /* y */
	    
	    POINT_ADD(last[AMITK_AXIS_Z], stride[AMITK_AXIS_Z], ds_point); 
	  } /* z */
	  break;

	case AMITK_RENDERING_MIP:
	case AMITK_RENDERING_MINIP:

	  /* iterate over the number of planes we'll be compressing into this slice */
	  for (z = 0; z < ceil(z_steps); z++) { 
	    last[AMITK_AXIS_Z] = ds_point;

	    /* need to initialize based on the first plane we encounter */
	    if ((z == 0) && (ds_voxel.t == start_frame) && (i_gate == 0)) {
	      /* iterate over x and y */
	      for (i_voxel.y = start.y,k=0; i_voxel.y <= end.y; i_voxel.y++) {
		last[AMITK_AXIS_Y] = ds_point;
		for (i_voxel.x = start.x; i_voxel.x <= end.x; i_voxel.x++,k++) {
		  POINT_TO_VOXEL_COORDS_ONLY(ds_point, data_set->voxel_size, ds_voxel);
		  if (!amitk_raw_data_includes_voxel(data_set->raw_data,ds_voxel)) 
		    intermediate_data[k] = NAN;
		  else
		    intermediate_data[k] =
		      AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_`'m4_Intercept`'CONTENT(data_set,ds_voxel);
		  POINT_ADD(ds_point, stride[AMITK_AXIS_X], ds_point); 
		} /* x */
		POINT_ADD(last[AMITK_AXIS_Y], stride[AMITK_AXIS_Y], ds_point);
	      } /* y */

	    } else { /* iterate over everything that's not the first plane */

	      if (data_set->rendering == AMITK_RENDERING_MIP) {
		/* iterate over x and y */
		for (i_voxel.y = start.y,k=0; i_voxel.y <= end.y; i_voxel.y++) { 
		  last[AMITK_AXIS_Y] = ds_point;
		  for (i_voxel.x = start.x; i_voxel.x <= end.x; i_voxel.x++,k++) { 
		    POINT_TO_VOXEL_COORDS_ONLY(ds_point, data_set->voxel_size, ds_voxel);
		    if (amitk_raw_data_includes_voxel(data_set->raw_data,ds_voxel)) 
		      intermediate_data[k] = 
			MAX(intermediate_data[k],
			    AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_`'m4_Intercept`'CONTENT(data_set,ds_voxel));
		    POINT_ADD(ds_point, stride[AMITK_AXIS_X], ds_point); 
		  } /* x */
		  POINT_ADD(last[AMITK_AXIS_Y], stride[AMITK_AXIS_Y], ds_point);
		} /* y */ 
	      } else { /* AMITK_RENDERING_MINIP */
		/* iterate over x and y */
		for (i_voxel.y = start.y,k=0; i_voxel.y <= end.y; i_voxel.y++) { 
		  last[AMITK_AXIS_Y] = ds_point;
		  for (i_voxel.x = start.x; i_voxel.x <= end.x; i_voxel.x++,k++) { 
		    POINT_TO_VOXEL_COORDS_ONLY(ds_point, data_set->voxel_size, ds_voxel);
		    if (amitk_raw_data_includes_voxel(data_set->raw_data,ds_voxel)) 
		      intermediate_data[k] = 
			MIN(intermediate_data[k],
			    AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_`'m4_Intercept`'CONTENT(data_set,ds_voxel));
		    POINT_ADD(ds_point, stride[AMITK_AXIS_X], ds_point); 
		  } /* x */
		  POINT_ADD(last[AMITK_AXIS_Y], stride[AMITK_AXIS_Y], ds_point);
		} /* y */ 
	      } /* end else, MIP vs MINIP */
	    } /* end else */
	      
	    POINT_ADD(last[AMITK_AXIS_Z], stride[AMITK_AXIS_Z], ds_point); 
	  } /* z */
	  break;

	default:
	  break;
	} /* MIP vs NON-MIP */

      } /* iterating over gates */
    } /* iterating over frames */
    break;
  }

  /* fill in data/normalize if needed */
  i_voxel.t = i_voxel.g = i_voxel.z = 0;
  if (data_set->rendering == AMITK_RENDERING_MPR) {
    for (i_voxel.y = start.y,k=0; i_voxel.y <= end.y; i_voxel.y++) 
      for (i_voxel.x = start.x; i_voxel.x <= end.x; i_voxel.x++,k++) 
	if (weights[k] > 0)
	  AMITK_RAW_DATA_DOUBLE_SET_CONTENT(slice->raw_data,i_voxel) = intermediate_data[k]/weights[k];
	else
	  AMITK_RAW_DATA_DOUBLE_SET_CONTENT(slice->raw_data,i_voxel) = NAN;
  } else { /* MIP or MINIP */
    for (i_voxel.y = start.y,k=0; i_voxel.y <= end.y; i_voxel.y++) 
      for (i_voxel.x = start.x; i_voxel.x <= end.x; i_voxel.x++,k++) 
	AMITK_RAW_DATA_DOUBLE_SET_CONTENT(slice->raw_data,i_voxel) = intermediate_data[k];
  }
    
 error:

  if (weights != NULL) g_free(weights);
  if (intermediate_data != NULL) g_free(intermediate_data);

  return slice;
}


