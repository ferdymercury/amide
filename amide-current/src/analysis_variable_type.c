/* analysis_variable_type.c - used to generate the different analysis_*.c files
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
#include "analysis_`'m4_Variable_Type`'.h"

/* make sure we have NAN defined to at least something */
#ifndef NAN
#define NAN 0.0
#endif

#define EMPTY 0.0

#define ANALYSIS_`'m4_Variable_Type`'_TYPE

analysis_frame_t * analysis_frame_`'m4_Variable_Type`'_init(AmitkRoi * roi, AmitkDataSet * ds, guint frame) {

  analysis_frame_t * frame_analysis;
  AmitkPoint roi_pt, fine_ds_pt, far_ds_pt;
  amide_data_t temp_data, total, total_var, total_correction, temp;
  AmitkVoxel i,j, k;
  AmitkVoxel start, dim;
  gboolean voxel_in;
  AmitkRawData * next_plane_in;
  AmitkRawData * curr_plane_in;
  gboolean small_dimensions;
  AmitkCorners intersection_corners;
  AmitkPoint ds_voxel_size;
  AmitkPoint roi_voxel_size;

#if defined(ANALYSIS_BOX_TYPE)
  AmitkPoint box_corner;
  box_corner = AMITK_VOLUME_CORNER(roi);
#endif

#if defined(ANALYSIS_ELLIPSOID_TYPE) || defined(ANALYSIS_CYLINDER_TYPE)
  AmitkPoint center;
  AmitkPoint radius;
#if defined(ANALYSIS_CYLINDER_TYPE)
  amide_real_t height;
  height = AMITK_VOLUME_Z_CORNER(roi);
#endif  
  center = amitk_space_b2s(AMITK_SPACE(roi), amitk_volume_center(AMITK_VOLUME(roi)));
  radius = point_cmult(0.5, AMITK_VOLUME_CORNER(roi));
#endif

#if defined(ANALYSIS_ISOCONTOUR_2D_TYPE) || defined(ANALYSIS_ISOCONTOUR_3D_TYPE)
  AmitkVoxel roi_voxel;
#endif


  /* get memory first */
  if ((frame_analysis =  g_new(analysis_frame_t,1)) == NULL) {
    g_warning("couldn't allocate space for roi analysis of frame %d", frame);
    return frame_analysis;
  }
  frame_analysis->ref_count = 1;

  /* note the frame duation */
  frame_analysis->duration = amitk_data_set_get_frame_duration(ds, frame);

  /* calculate the time midpoint of the data */
  frame_analysis->time_midpoint = (amitk_data_set_get_end_time(ds, frame) + 
				   amitk_data_set_get_start_time(ds, frame))/2.0;

  /* initialize values */
  frame_analysis->voxels = 0.0; 
  total = total_var = total_correction = 0.0;
  ds_voxel_size = AMITK_DATA_SET_VOXEL_SIZE(ds);
  roi_voxel_size = AMITK_ROI_VOXEL_SIZE(roi);

  /* figure out the intersection between the data set and the roi */
  if (!amitk_volume_volume_intersection_corners(AMITK_VOLUME(ds), 
						AMITK_VOLUME(roi), 
						intersection_corners)) {
    dim = zero_voxel; /* no intersection */
  } else {
    /* translate the intersection into voxel space */
    POINT_TO_VOXEL(intersection_corners[0], ds_voxel_size, 0, start);
    POINT_TO_VOXEL(intersection_corners[1], ds_voxel_size, 0, dim);
    dim = voxel_add(voxel_sub(dim, start), one_voxel);
  }

  /* check if we're done already */
  if ((dim.x == 0) || (dim.y == 0) || (dim.z == 0)) {
    frame_analysis->mean = frame_analysis->var = frame_analysis->min = frame_analysis->max =  NAN;
    return frame_analysis;
  }

  /* initialize max and min based on a voxel that's inside the ROI */
  i.x = start.x+dim.x/2;
  i.y = start.y+dim.y/2;
  i.z = start.z+dim.z/2;
  i.t = frame;
  if (!amitk_raw_data_includes_voxel(AMITK_DATA_SET_RAW_DATA(ds), i)) {
    g_warning("Error generating statistics on: %s, frame:%d, min and max values may be incorrect for roi: %s",
	      AMITK_OBJECT_NAME(ds), frame, AMITK_OBJECT_NAME(roi));
    temp_data = EMPTY;
  } else
    temp_data = amitk_data_set_get_value(ds,i);
  frame_analysis->min = frame_analysis->max = temp_data;

  /* if we have any small dimensions, make sure we always iterate over sub voxels */
  small_dimensions = FALSE;
  if ((dim.x == 1) || (dim.y == 1) || (dim.z == 1))
    small_dimensions = TRUE;

  /* over-iterate, as our initial edges will always be considered out of the roi */
  start.x -= 1;
  start.y -= 1;
  start.z -= 1;
  dim.x += 1;
  dim.y += 1;
  dim.z += 1;

  /* start and end specify (in the data set's voxel space) the voxels in 
     the volume we should be iterating over */

  /* these two arrays are used to store whether the voxel vertices are in the ROI or not */
  next_plane_in = amitk_raw_data_UBYTE_2D_init(0, dim.y+1, dim.x+1);
  curr_plane_in = amitk_raw_data_UBYTE_2D_init(0, dim.y+1, dim.x+1);

  j.t = frame;
  i.t = k.t = 0;
  for (i.z = 0; i.z < dim.z; i.z++) {
    j.z = i.z+start.z;
    far_ds_pt.z = (j.z+1)*ds_voxel_size.z;
    
    for (i.y = 0; i.y < dim.y; i.y++) {
      j.y = i.y+start.y;
      far_ds_pt.y = (j.y+1)*ds_voxel_size.y;
      
      for (i.x = 0; i.x < dim.x; i.x++) {
	j.x = i.x+start.x;
	far_ds_pt.x = (j.x+1)*ds_voxel_size.x;
	
	/* figure out if the next far corner is in the roi or not */
	/* get the corresponding roi point */
	roi_pt = amitk_space_s2s(AMITK_SPACE(ds),  AMITK_SPACE(roi), far_ds_pt);

	/* calculate the one corner of the voxel "box" to determine if it's in or not */
#ifdef ANALYSIS_BOX_TYPE
	voxel_in = point_in_box(roi_pt, box_corner);
#endif
#ifdef ANALYSIS_CYLINDER_TYPE
	voxel_in = point_in_elliptic_cylinder(roi_pt, center, height, radius);
#endif
#ifdef ANALYSIS_ELLIPSOID_TYPE
	voxel_in = point_in_ellipsoid(roi_pt,center,radius);
#endif
#if defined(ANALYSIS_ISOCONTOUR_2D_TYPE) || defined(ANALYSIS_ISOCONTOUR_3D_TYPE)
	POINT_TO_VOXEL(roi_pt, roi_voxel_size, 0, roi_voxel);
	if (!amitk_raw_data_includes_voxel(roi->isocontour, roi_voxel)) voxel_in = FALSE;
	else if (*AMITK_RAW_DATA_UBYTE_POINTER(roi->isocontour, roi_voxel) == 0) voxel_in = FALSE;
	else voxel_in = TRUE;
#endif

	*AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x+1)=voxel_in;

	if (*AMITK_RAW_DATA_UBYTE_2D_POINTER(curr_plane_in,i.y,i.x) &&
	    *AMITK_RAW_DATA_UBYTE_2D_POINTER(curr_plane_in,i.y,i.x+1) &&
	    *AMITK_RAW_DATA_UBYTE_2D_POINTER(curr_plane_in,i.y+1,i.x) &&
	    *AMITK_RAW_DATA_UBYTE_2D_POINTER(curr_plane_in,i.y+1,i.x+1) &&
	    *AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,i.y,i.x) &&
	    *AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,i.y,i.x+1) &&
	    *AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x) &&
	    *AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x+1)) {
	  /* this voxel is entirely in the ROI */

	  temp_data = amitk_data_set_get_value(ds,j);

	  total += temp_data;
	  frame_analysis->voxels += 1.0;
	  if ((temp_data) < frame_analysis->min) frame_analysis->min = temp_data;
	  if ((temp_data) > frame_analysis->max) frame_analysis->max = temp_data;

	} else if (*AMITK_RAW_DATA_UBYTE_2D_POINTER(curr_plane_in,i.y,i.x) ||
		   *AMITK_RAW_DATA_UBYTE_2D_POINTER(curr_plane_in,i.y,i.x+1) ||
		   *AMITK_RAW_DATA_UBYTE_2D_POINTER(curr_plane_in,i.y+1,i.x) ||
		   *AMITK_RAW_DATA_UBYTE_2D_POINTER(curr_plane_in,i.y+1,i.x+1) ||
		   *AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,i.y,i.x) ||
		   *AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,i.y,i.x+1) ||
		   *AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x) ||
		   *AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x+1) ||
		   small_dimensions) {
	  /* this voxel is partially in the ROI, will need to do subvoxel analysis */

	  temp_data = amitk_data_set_get_value(ds,j);

	  for (k.z = 0;k.z<ANALYSIS_GRANULARITY;k.z++) {
	    fine_ds_pt.z = j.z*ds_voxel_size.z+
	      (((amide_real_t) k.z)+0.5)*ds_voxel_size.z/ANALYSIS_GRANULARITY;

	    for (k.y = 0;k.y<ANALYSIS_GRANULARITY;k.y++) {
	      fine_ds_pt.y = j.y*ds_voxel_size.y+
		(((amide_real_t) k.y)+0.5)*ds_voxel_size.y/ANALYSIS_GRANULARITY;

	      for (k.x = 0;k.x<ANALYSIS_GRANULARITY;k.x++) {
		fine_ds_pt.x = j.x*ds_voxel_size.x+
		  (((amide_real_t) k.x)+0.5)*ds_voxel_size.x/ANALYSIS_GRANULARITY;
		
		roi_pt = amitk_space_s2s(AMITK_SPACE(ds), AMITK_SPACE(roi), fine_ds_pt);

		/* calculate the one corner of the voxel "box" to determine if it's in or not */
#ifdef ANALYSIS_BOX_TYPE
		voxel_in = point_in_box(roi_pt, box_corner);
#endif
#ifdef ANALYSIS_CYLINDER_TYPE
		voxel_in = point_in_elliptic_cylinder(roi_pt, center, height, radius);
#endif
#ifdef ANALYSIS_ELLIPSOID_TYPE
		voxel_in = point_in_ellipsoid(roi_pt,center,radius);
#endif
#if defined(ANALYSIS_ISOCONTOUR_2D_TYPE) || defined(ANALYSIS_ISOCONTOUR_3D_TYPE)
		POINT_TO_VOXEL(roi_pt, roi_voxel_size, 0, roi_voxel);
		if (!amitk_raw_data_includes_voxel(roi->isocontour, roi_voxel)) voxel_in = FALSE;
		else if (*AMITK_RAW_DATA_UBYTE_POINTER(roi->isocontour, roi_voxel) == 0) voxel_in = FALSE;
		else voxel_in = TRUE;
#endif
		if (voxel_in) {
		  total += temp_data*ANALYSIS_GRAIN_SIZE;
		  frame_analysis->voxels += ANALYSIS_GRAIN_SIZE;
		  if ((temp_data) < frame_analysis->min) frame_analysis->min = temp_data;
		  if ((temp_data) > frame_analysis->max) frame_analysis->max = temp_data;
		} /* voxel_in */
	      } /* k.x loop */
	    } /* k.y loop */
	  } /* k.z loop */
	} /* this voxel is outside the ROI, do nothing*/
      } /* i.x loop */
    } /* i.y loop */
    
    /* need to copy over the info on which voxel corners are in the roi */
    for (k.y=0;k.y < next_plane_in->dim.y; k.y++)
      for (k.x=0;k.x < next_plane_in->dim.x; k.x++)
	*AMITK_RAW_DATA_UBYTE_2D_POINTER(curr_plane_in,k.y,k.x) = 
	  *AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,k.y,k.x);
  } /* i.z loop */

  /* calculate the mean */
  frame_analysis->total = total;
  frame_analysis->mean = total/frame_analysis->voxels;



  /* go through the whole loop again, to calculate variance */
  amitk_raw_data_UBYTE_initialize_data(next_plane_in,0);
  amitk_raw_data_UBYTE_initialize_data(curr_plane_in,0);
  for (i.z = 0; i.z < dim.z; i.z++) {
    j.z = i.z+start.z;
    far_ds_pt.z = (j.z+1)*ds_voxel_size.z;
    
    for (i.y = 0; i.y < dim.y; i.y++) {
      j.y = i.y+start.y;
      far_ds_pt.y = (j.y+1)*ds_voxel_size.y;
      
      for (i.x = 0; i.x < dim.x; i.x++) {
	j.x = i.x+start.x;
	far_ds_pt.x = (j.x+1)*ds_voxel_size.x;
	
	/* figure out if the next far corner is in the roi or not */
	/* get the corresponding roi point */
	roi_pt = amitk_space_s2s(AMITK_SPACE(ds),  AMITK_SPACE(roi), far_ds_pt);

	/* calculate the one corner of the voxel "box" to determine if it's in or not */
#ifdef ANALYSIS_BOX_TYPE
	voxel_in = point_in_box(roi_pt, box_corner);
#endif
#ifdef ANALYSIS_CYLINDER_TYPE
	voxel_in = point_in_elliptic_cylinder(roi_pt, center, height, radius);
#endif
#ifdef ANALYSIS_ELLIPSOID_TYPE
	voxel_in = point_in_ellipsoid(roi_pt,center,radius);
#endif
#if defined(ANALYSIS_ISOCONTOUR_2D_TYPE) || defined(ANALYSIS_ISOCONTOUR_3D_TYPE)
	POINT_TO_VOXEL(roi_pt, roi_voxel_size, 0, roi_voxel);
	if (!amitk_raw_data_includes_voxel(roi->isocontour, roi_voxel)) voxel_in = FALSE;
	else if (*AMITK_RAW_DATA_UBYTE_POINTER(roi->isocontour, roi_voxel) == 0) voxel_in = FALSE;
	else voxel_in = TRUE;
#endif

	*AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x+1)=voxel_in;

	if (*AMITK_RAW_DATA_UBYTE_2D_POINTER(curr_plane_in,i.y,i.x) &&
	    *AMITK_RAW_DATA_UBYTE_2D_POINTER(curr_plane_in,i.y,i.x+1) &&
	    *AMITK_RAW_DATA_UBYTE_2D_POINTER(curr_plane_in,i.y+1,i.x) &&
	    *AMITK_RAW_DATA_UBYTE_2D_POINTER(curr_plane_in,i.y+1,i.x+1) &&
	    *AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,i.y,i.x) &&
	    *AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,i.y,i.x+1) &&
	    *AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x) &&
	    *AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x+1)) {
	  /* this voxel is entirely in the ROI */

	  temp_data = amitk_data_set_get_value(ds,j);

	  temp = (temp_data-frame_analysis->mean);
	  total_correction += temp;
	  total_var +=  temp*temp;

	} else if (*AMITK_RAW_DATA_UBYTE_2D_POINTER(curr_plane_in,i.y,i.x) ||
		   *AMITK_RAW_DATA_UBYTE_2D_POINTER(curr_plane_in,i.y,i.x+1) ||
		   *AMITK_RAW_DATA_UBYTE_2D_POINTER(curr_plane_in,i.y+1,i.x) ||
		   *AMITK_RAW_DATA_UBYTE_2D_POINTER(curr_plane_in,i.y+1,i.x+1) ||
		   *AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,i.y,i.x) ||
		   *AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,i.y,i.x+1) ||
		   *AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x) ||
		   *AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x+1) ||
		   small_dimensions) {
	  /* this voxel is partially in the ROI, will need to do subvoxel analysis */

	  temp_data = amitk_data_set_get_value(ds,j);

	  for (k.z = 0;k.z<ANALYSIS_GRANULARITY;k.z++) {
	    fine_ds_pt.z = j.z*ds_voxel_size.z+
	      (((amide_real_t) k.z)+0.5)*ds_voxel_size.z/ANALYSIS_GRANULARITY;

	    for (k.y = 0;k.y<ANALYSIS_GRANULARITY;k.y++) {
	      fine_ds_pt.y = j.y*ds_voxel_size.y+
		(((amide_real_t) k.y)+0.5)*ds_voxel_size.y/ANALYSIS_GRANULARITY;


	      for (k.x = 0;k.x<ANALYSIS_GRANULARITY;k.x++) {
		fine_ds_pt.x = j.x*ds_voxel_size.x+
		  (((amide_real_t) k.x)+0.5)*ds_voxel_size.x/ANALYSIS_GRANULARITY;
		
		roi_pt = amitk_space_s2s(AMITK_SPACE(ds), AMITK_SPACE(roi), fine_ds_pt);

		/* calculate the one corner of the voxel "box" to determine if it's in or not */
#ifdef ANALYSIS_BOX_TYPE
		voxel_in = point_in_box(roi_pt, box_corner);
#endif
#ifdef ANALYSIS_CYLINDER_TYPE
		voxel_in = point_in_elliptic_cylinder(roi_pt, center, height, radius);
#endif
#ifdef ANALYSIS_ELLIPSOID_TYPE
		voxel_in = point_in_ellipsoid(roi_pt,center,radius);
#endif
#if defined(ANALYSIS_ISOCONTOUR_2D_TYPE) || defined(ANALYSIS_ISOCONTOUR_3D_TYPE)
		POINT_TO_VOXEL(roi_pt, roi_voxel_size, 0, roi_voxel);
		if (!amitk_raw_data_includes_voxel(roi->isocontour, roi_voxel)) voxel_in = FALSE;
		else if (*AMITK_RAW_DATA_UBYTE_POINTER(roi->isocontour, roi_voxel) == 0) voxel_in = FALSE;
		else voxel_in = TRUE;
#endif
		if (voxel_in) {
		  temp = (temp_data-frame_analysis->mean);
		  total_correction += ANALYSIS_GRAIN_SIZE*temp;
		  total_var +=  ANALYSIS_GRAIN_SIZE*temp*temp;
		} /* voxel_in */
	      } /* k.x loop */
	    } /* k.y loop */
	  } /* k.z loop */
	} /* this voxel is outside the ROI, do nothing*/
      } /* i.x loop */
    } /* i.y loop */
    
    /* need to copy over the info on which voxel corners are in the roi */
    for (k.y=0;k.y < next_plane_in->dim.y; k.y++)
      for (k.x=0;k.x < next_plane_in->dim.x; k.x++)
	*AMITK_RAW_DATA_UBYTE_2D_POINTER(curr_plane_in,k.y,k.x) = 
	  *AMITK_RAW_DATA_UBYTE_2D_POINTER(next_plane_in,k.y,k.x);
  } /* i.z loop */

  /* and divide to get the final var, note I'm using N-1, as the mean
     in a sense is being "estimated" from the data set....  If anyone
     else with more statistical experience disagrees, please speak up */
  /* the "total correction" parameter is to correct roundoff error,
     for a discussion, see "the art of computer programming" */

  if (frame_analysis->voxels < 2.0)
    frame_analysis->var = 0; /* variance is non-sensible */
  else
    frame_analysis->var = (total_var-total_correction*total_correction/frame_analysis->voxels)
      /(frame_analysis->voxels-1.0);

  /* trash collection */
  g_object_unref(curr_plane_in);
  g_object_unref(next_plane_in);


  return frame_analysis;
}
