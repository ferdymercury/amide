/* analysis_variable_type.c - used to generate the different analysis_*.c files
 *
 * Part of amide - Amide's a Medical Image Data Examiner
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
#define _GNU_SOURCE /* use GNU extensions, i.e. NaN */
#include <math.h>
#include <glib.h>
#include "analysis_`'m4_Variable_Type`'.h"

/* make sure we have NAN defined to at least something */
#ifndef NAN
#define NAN 0
#endif

#define ANALYSIS_`'m4_Variable_Type`'_TYPE

analysis_frame_t * analysis_frame_`'m4_Variable_Type`'_init(roi_t * roi, volume_t * volume, guint frame) {

  analysis_frame_t * frame_analysis;
  realpoint_t roi_p, fine_volume_p, far_volume_p;
  amide_data_t temp_data, total, total_var, total_correction, temp;
  voxelpoint_t i,j, k;
  voxelpoint_t start, dim;
  gboolean voxel_in;
  data_set_t * next_plane_in;
  data_set_t * curr_plane_in;
  gboolean small_dimensions;
  realpoint_t roi_corner[2];
  realpoint_t center;
#if defined(ANALYSIS_ELLIPSOID_TYPE) || defined(ANALYSIS_CYLINDER_TYPE)
  realpoint_t radius;
#endif
#if defined(ANALYSIS_CYLINDER_TYPE)
  floatpoint_t height;
#endif
#if defined(ANALYSIS_ISOCONTOUR_2D_TYPE) || defined(ANALYSIS_ISOCONTOUR_3D_TYPE)
  voxelpoint_t roi_vp;
#endif

  /* get the roi corners in roi space */
  roi_corner[0] = realspace_base_coord_to_alt(rs_offset(roi->coord_frame), roi->coord_frame);
  roi_corner[1] = roi->corner;

  center = rp_add(rp_cmult(0.5,roi_corner[1]), rp_cmult(0.5,roi_corner[0])); 

#if defined(ANALYSIS_ELLIPSOID_TYPE) || defined(ANALYSIS_CYLINDER_TYPE)
  radius = rp_cmult(0.5, rp_diff(roi_corner[1],roi_corner[0])); 
#endif


#if defined(ANALYSIS_CYLINDER_TYPE)
  height = fabs(roi_corner[1].z-roi_corner[0].z);
#endif  

  /* get memory first */
  if ((frame_analysis =  (analysis_frame_t *) g_malloc(sizeof(analysis_frame_t))) == NULL) {
    g_warning("%s: couldn't allocate space for roi analysis of frame %d", PACKAGE, frame);
    return frame_analysis;
  }
  frame_analysis->reference_count = 1;

  /* calculate the time midpoint of the data */
  frame_analysis->time_midpoint = (volume_end_time(volume, frame) + volume_start_time(volume, frame))/2.0;

  /* initialize values */
  frame_analysis->voxels = 0.0; 
  total = total_var = total_correction = 0.0;

  /* figure out what portion of the volume we'll be iterating over */
  roi_subset_of_volume(roi,volume,frame, &start,&dim);

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
  if (!data_set_includes_voxel(volume->data_set, i)) {
    g_warning("%s: Error generating statistics on: %s, frame:%d, min and max values may be incorrect for roi: %s",
	      PACKAGE, volume->name, frame, roi->name);
    temp_data = EMPTY;
  } else
    temp_data = volume_value(volume,i);
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

  /* start and end specify (in the volume's voxel space) the voxels in 
     the volume we should be iterating over */

  /* these two arrays are used to store whether the voxel vertices are
     in the ROI or not */
  next_plane_in = data_set_UBYTE_2D_init(0, dim.y+1, dim.x+1);
  curr_plane_in = data_set_UBYTE_2D_init(0, dim.y+1, dim.x+1);

  j.t = i.t = frame;
  k.t = 0;
  for (i.z = 0; i.z < dim.z; i.z++) {
    j.z = i.z+start.z;
    far_volume_p.z = (j.z+1)*volume->voxel_size.z;
    
    for (i.y = 0; i.y < dim.y; i.y++) {
      j.y = i.y+start.y;
      far_volume_p.y = (j.y+1)*volume->voxel_size.y;
      
      for (i.x = 0; i.x < dim.x; i.x++) {
	j.x = i.x+start.x;
	far_volume_p.x = (j.x+1)*volume->voxel_size.x;
	
	/* figure out if the next far corner is in the roi or not */
	/* get the corresponding roi point */
	roi_p = realspace_alt_coord_to_alt(far_volume_p, volume->coord_frame,  roi->coord_frame);

	/* calculate the one corner of the voxel "box" to determine if it's in or not */
#ifdef ANALYSIS_BOX_TYPE
	voxel_in = realpoint_in_box(roi_p, roi_corner[0],roi_corner[1]);
#endif
#ifdef ANALYSIS_CYLINDER_TYPE
	voxel_in = realpoint_in_elliptic_cylinder(roi_p, center, height, radius);
#endif
#ifdef ANALYSIS_ELLIPSOID_TYPE
	voxel_in = realpoint_in_ellipsoid(roi_p,center,radius);
#endif
#if defined(ANALYSIS_ISOCONTOUR_2D_TYPE) || defined(ANALYSIS_ISOCONTOUR_3D_TYPE)
	ROI_REALPOINT_TO_VOXEL(roi, roi_p, roi_vp);
	if (!data_set_includes_voxel(roi->isocontour, roi_vp)) voxel_in = FALSE;
	else if (*DATA_SET_UBYTE_POINTER(roi->isocontour, roi_vp) == 0) voxel_in = FALSE;
	else voxel_in = TRUE;
#endif

	*DATA_SET_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x+1)=voxel_in;

	if (*DATA_SET_UBYTE_2D_POINTER(curr_plane_in,i.y,i.x) &&
	    *DATA_SET_UBYTE_2D_POINTER(curr_plane_in,i.y,i.x+1) &&
	    *DATA_SET_UBYTE_2D_POINTER(curr_plane_in,i.y+1,i.x) &&
	    *DATA_SET_UBYTE_2D_POINTER(curr_plane_in,i.y+1,i.x+1) &&
	    *DATA_SET_UBYTE_2D_POINTER(next_plane_in,i.y,i.x) &&
	    *DATA_SET_UBYTE_2D_POINTER(next_plane_in,i.y,i.x+1) &&
	    *DATA_SET_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x) &&
	    *DATA_SET_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x+1)) {
	  /* this voxel is entirely in the ROI */

	  temp_data = volume_value(volume,j);

	  total += temp_data;
	  frame_analysis->voxels += 1.0;
	  if ((temp_data) < frame_analysis->min) frame_analysis->min = temp_data;
	  if ((temp_data) > frame_analysis->max) frame_analysis->max = temp_data;

	} else if (*DATA_SET_UBYTE_2D_POINTER(curr_plane_in,i.y,i.x) ||
		   *DATA_SET_UBYTE_2D_POINTER(curr_plane_in,i.y,i.x+1) ||
		   *DATA_SET_UBYTE_2D_POINTER(curr_plane_in,i.y+1,i.x) ||
		   *DATA_SET_UBYTE_2D_POINTER(curr_plane_in,i.y+1,i.x+1) ||
		   *DATA_SET_UBYTE_2D_POINTER(next_plane_in,i.y,i.x) ||
		   *DATA_SET_UBYTE_2D_POINTER(next_plane_in,i.y,i.x+1) ||
		   *DATA_SET_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x) ||
		   *DATA_SET_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x+1) ||
		   small_dimensions) {
	  /* this voxel is partially in the ROI, will need to do subvoxel analysis */

	  temp_data = volume_value(volume,j);

	  for (k.z = 0;k.z<ANALYSIS_GRANULARITY;k.z++) {
	    fine_volume_p.z = j.z*volume->voxel_size.z+
	      (k.z+1)*volume->voxel_size.z/(ANALYSIS_GRANULARITY+1);

	    for (k.y = 0;k.y<ANALYSIS_GRANULARITY;k.y++) {
	      fine_volume_p.y = j.y*volume->voxel_size.y+
		(k.y+1)*volume->voxel_size.y/(ANALYSIS_GRANULARITY+1);

	      for (k.x = 0;k.x<ANALYSIS_GRANULARITY;k.x++) {
		fine_volume_p.x = j.x*volume->voxel_size.x+
		  (k.x+1)*volume->voxel_size.x/(ANALYSIS_GRANULARITY+1);
		
		roi_p = realspace_alt_coord_to_alt(fine_volume_p,
						   volume->coord_frame,
						   roi->coord_frame);

		/* calculate the one corner of the voxel "box" to determine if it's in or not */
#ifdef ANALYSIS_BOX_TYPE
		voxel_in = realpoint_in_box(roi_p, roi_corner[0],roi_corner[1]);
#endif
#ifdef ANALYSIS_CYLINDER_TYPE
		voxel_in = realpoint_in_elliptic_cylinder(roi_p, center, height, radius);
#endif
#ifdef ANALYSIS_ELLIPSOID_TYPE
		voxel_in = realpoint_in_ellipsoid(roi_p,center,radius);
#endif
#if defined(ANALYSIS_ISOCONTOUR_2D_TYPE) || defined(ANALYSIS_ISOCONTOUR_3D_TYPE)
		ROI_REALPOINT_TO_VOXEL(roi, roi_p, roi_vp);
		if (!data_set_includes_voxel(roi->isocontour, roi_vp)) voxel_in = FALSE;
		else if (*DATA_SET_UBYTE_POINTER(roi->isocontour, roi_vp) == 0) voxel_in = FALSE;
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
	*DATA_SET_UBYTE_2D_POINTER(curr_plane_in,k.y,k.x) = 
	  *DATA_SET_UBYTE_2D_POINTER(next_plane_in,k.y,k.x);
  } /* i.z loop */

  /* calculate the mean */
  frame_analysis->mean = total/frame_analysis->voxels;



  /* go through the whole loop again, to calculate variance */
  data_set_UBYTE_initialize_data(next_plane_in,0);
  data_set_UBYTE_initialize_data(curr_plane_in,0);
  for (i.z = 0; i.z < dim.z; i.z++) {
    j.z = i.z+start.z;
    far_volume_p.z = (j.z+1)*volume->voxel_size.z;
    
    for (i.y = 0; i.y < dim.y; i.y++) {
      j.y = i.y+start.y;
      far_volume_p.y = (j.y+1)*volume->voxel_size.y;
      
      for (i.x = 0; i.x < dim.x; i.x++) {
	j.x = i.x+start.x;
	far_volume_p.x = (j.x+1)*volume->voxel_size.x;
	
	/* figure out if the next far corner is in the roi or not */
	/* get the corresponding roi point */
	roi_p = realspace_alt_coord_to_alt(far_volume_p, volume->coord_frame,  roi->coord_frame);

	/* calculate the one corner of the voxel "box" to determine if it's in or not */
#ifdef ANALYSIS_BOX_TYPE
	voxel_in = realpoint_in_box(roi_p, roi_corner[0],roi_corner[1]);
#endif
#ifdef ANALYSIS_CYLINDER_TYPE
	voxel_in = realpoint_in_elliptic_cylinder(roi_p, center, height, radius);
#endif
#ifdef ANALYSIS_ELLIPSOID_TYPE
	voxel_in = realpoint_in_ellipsoid(roi_p,center,radius);
#endif
#if defined(ANALYSIS_ISOCONTOUR_2D_TYPE) || defined(ANALYSIS_ISOCONTOUR_3D_TYPE)
	ROI_REALPOINT_TO_VOXEL(roi, roi_p, roi_vp);
	if (!data_set_includes_voxel(roi->isocontour, roi_vp)) voxel_in = FALSE;
	else if (*DATA_SET_UBYTE_POINTER(roi->isocontour, roi_vp) == 0) voxel_in = FALSE;
	else voxel_in = TRUE;
#endif

	*DATA_SET_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x+1)=voxel_in;

	if (*DATA_SET_UBYTE_2D_POINTER(curr_plane_in,i.y,i.x) &&
	    *DATA_SET_UBYTE_2D_POINTER(curr_plane_in,i.y,i.x+1) &&
	    *DATA_SET_UBYTE_2D_POINTER(curr_plane_in,i.y+1,i.x) &&
	    *DATA_SET_UBYTE_2D_POINTER(curr_plane_in,i.y+1,i.x+1) &&
	    *DATA_SET_UBYTE_2D_POINTER(next_plane_in,i.y,i.x) &&
	    *DATA_SET_UBYTE_2D_POINTER(next_plane_in,i.y,i.x+1) &&
	    *DATA_SET_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x) &&
	    *DATA_SET_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x+1)) {
	  /* this voxel is entirely in the ROI */

	  temp_data = volume_value(volume,j);

	  temp = (temp_data-frame_analysis->mean);
	  total_correction += temp;
	  total_var +=  temp*temp;

	} else if (*DATA_SET_UBYTE_2D_POINTER(curr_plane_in,i.y,i.x) ||
		   *DATA_SET_UBYTE_2D_POINTER(curr_plane_in,i.y,i.x+1) ||
		   *DATA_SET_UBYTE_2D_POINTER(curr_plane_in,i.y+1,i.x) ||
		   *DATA_SET_UBYTE_2D_POINTER(curr_plane_in,i.y+1,i.x+1) ||
		   *DATA_SET_UBYTE_2D_POINTER(next_plane_in,i.y,i.x) ||
		   *DATA_SET_UBYTE_2D_POINTER(next_plane_in,i.y,i.x+1) ||
		   *DATA_SET_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x) ||
		   *DATA_SET_UBYTE_2D_POINTER(next_plane_in,i.y+1,i.x+1) ||
		   small_dimensions) {
	  /* this voxel is partially in the ROI, will need to do subvoxel analysis */

	  temp_data = volume_value(volume,j);

	  for (k.z = 0;k.z<ANALYSIS_GRANULARITY;k.z++) {
	    fine_volume_p.z = j.z*volume->voxel_size.z+
	      (k.z+1)*volume->voxel_size.z/(ANALYSIS_GRANULARITY+1);

	    for (k.y = 0;k.y<ANALYSIS_GRANULARITY;k.y++) {
	      fine_volume_p.y = j.y*volume->voxel_size.y+
		(k.y+1)*volume->voxel_size.y/(ANALYSIS_GRANULARITY+1);

	      for (k.x = 0;k.x<ANALYSIS_GRANULARITY;k.x++) {
		fine_volume_p.x = j.x*volume->voxel_size.x+
		  (k.x+1)*volume->voxel_size.x/(ANALYSIS_GRANULARITY+1);
		
		roi_p = realspace_alt_coord_to_alt(fine_volume_p,
						   volume->coord_frame,
						   roi->coord_frame);

		/* calculate the one corner of the voxel "box" to determine if it's in or not */
#ifdef ANALYSIS_BOX_TYPE
		voxel_in = realpoint_in_box(roi_p, roi_corner[0],roi_corner[1]);
#endif
#ifdef ANALYSIS_CYLINDER_TYPE
		voxel_in = realpoint_in_elliptic_cylinder(roi_p, center, height, radius);
#endif
#ifdef ANALYSIS_ELLIPSOID_TYPE
		voxel_in = realpoint_in_ellipsoid(roi_p,center,radius);
#endif
#if defined(ANALYSIS_ISOCONTOUR_2D_TYPE) || defined(ANALYSIS_ISOCONTOUR_3D_TYPE)
		ROI_REALPOINT_TO_VOXEL(roi, roi_p, roi_vp);
		if (!data_set_includes_voxel(roi->isocontour, roi_vp)) voxel_in = FALSE;
		else if (*DATA_SET_UBYTE_POINTER(roi->isocontour, roi_vp) == 0) voxel_in = FALSE;
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
	*DATA_SET_UBYTE_2D_POINTER(curr_plane_in,k.y,k.x) = 
	  *DATA_SET_UBYTE_2D_POINTER(next_plane_in,k.y,k.x);
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
  curr_plane_in = data_set_free(curr_plane_in);
  next_plane_in = data_set_free(next_plane_in);


  return frame_analysis;
}
