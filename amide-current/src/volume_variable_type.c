/* volume_variable_type.c - used to generate the different volume_*.c files
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
#include <math.h>
#include <glib.h>
#ifdef AMIDE_DEBUG
#include <stdlib.h>
#endif
#include "volume_`'m4_Variable_Type`'_`'m4_Scale_Dim`'.h"


/* function to recalcule the max and min values of a volume */
void volume_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_recalc_max_min(volume_t * volume) {

  voxelpoint_t i;
  amide_data_t max, min, temp;

#ifdef AMIDE_DEBUG
  div_t x;
  gint divider;
  divider = ((volume->data_set->dim.t/20.0) < 1) ? 1 : (volume->data_set->dim.t/20.0);
  g_print("\tcalculating max & min\t");
#endif

  temp = VOLUME_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENTS(volume, voxelpoint_zero);
  if (finite(temp)) max = min = temp;
  else max = min = 0.0; /* just throw in zero */

  for(i.t = 0; i.t < volume->data_set->dim.t; i.t++) {
#if AMIDE_DEBUG
    x = div(i.t,divider);
    if (x.rem == 0)
      g_print(".");
#endif
    for (i.z = 0; i.z < volume->data_set->dim.z; i.z++) 
      for (i.y = 0; i.y < volume->data_set->dim.y; i.y++) 
	for (i.x = 0; i.x < volume->data_set->dim.x; i.x++) {
	  temp = VOLUME_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENTS(volume, i);
	  if (finite(temp)) {
	    if (temp > max) max = temp;
	    else if (temp < min) min = temp;
	  }
	}
  }
  volume->max = max;
  volume->min = min;

#ifdef AMIDE_DEBUG
  g_print("\tmax %5.3f min %5.3f\n",max,min);
#endif

  return;
}

/* generate the distribution array for a volume */
void volume_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_generate_distribution(volume_t * volume) {

  voxelpoint_t i,j;
  amide_data_t scale;

  if (volume->distribution != NULL)
    return;

#if AMIDE_DEBUG
  g_print("Generating distribution data for volume %s",volume->name);
#endif
  scale = (VOLUME_DISTRIBUTION_SIZE-1)/(volume->max - volume->min);
  
  if ((volume->distribution = data_set_init()) == NULL) {
    g_warning("%s: couldn't allocate space for the data set structure to hold distribution data", PACKAGE);
    return;
  }
  volume->distribution->dim.x = VOLUME_DISTRIBUTION_SIZE;
  volume->distribution->dim.y = 1;
  volume->distribution->dim.z = 1;
  volume->distribution->dim.t = 1;
  volume->distribution->format = FLOAT;
  if ((volume->distribution->data = data_set_get_data_mem(volume->distribution)) == NULL) {
    g_warning("%s: couldn't allocate memory for data distribution for bar_graph",PACKAGE);
    volume->distribution = data_set_free(volume->distribution);
    return;
  }
  
  /* initialize the distribution array */
  data_set_FLOAT_initialize_data(volume->distribution, 0.0);
  
  /* now "bin" the data */
  j.t = j.z = j.y = 0;
  for (i.t = 0; i.t < volume->data_set->dim.t; i.t++) {
#if AMIDE_DEBUG
    div_t x;
    gint divider;
    divider = ((volume->data_set->dim.z/20.0) < 1) ? 1 : (volume->data_set->dim.z/20.0);
    g_print("\n\tframe %d\t",i.t);
#endif
    for ( i.z = 0; i.z < volume->data_set->dim.z; i.z++) {
#if AMIDE_DEBUG
      x = div(i.z,divider);
      if (x.rem == 0)
	g_print(".");
#endif
	for (i.y = 0; i.y < volume->data_set->dim.y; i.y++) 
	  for (i.x = 0; i.x < volume->data_set->dim.x; i.x++) {
	    j.x = scale*(VOLUME_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENTS(volume,i)-volume->min);
	    DATA_SET_FLOAT_SET_CONTENT(volume->distribution,j) += 1.0;
	  }
    }
  }
#if AMIDE_DEBUG
  g_print("\n");
#endif
  
  
  /* do some log scaling so the distribution is more meaningful, and doesn't get
     swamped by outlyers */
  for (j.x = 0; j.x < volume->distribution->dim.x ; j.x++) 
    DATA_SET_FLOAT_SET_CONTENT(volume->distribution,j) = 
      log10(*DATA_SET_FLOAT_POINTER(volume->distribution,j)+1.0);

  return;
}


/* returns a slice  with the appropriate data from the volume */
volume_t * volume_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_get_slice(const volume_t * volume,
								  const amide_time_t start,
								  const amide_time_t duration,
								  const realpoint_t  requested_voxel_size,
								  const realspace_t slice_coord_frame,
								  const realpoint_t far_corner,
								  const interpolation_t interpolation,
								  const gboolean need_calc_max_min) {

  /* zp_start, where on the zp axis to start the slice, zp (z_prime) corresponds
     to the rotated axises, if negative, choose the midpoint */

  volume_t * slice;
  voxelpoint_t i,j;
  intpoint_t z;
  floatpoint_t max_diff, voxel_length, z_steps;
  realpoint_t alt;
  realpoint_t stride[NUM_AXIS], last[NUM_AXIS];
  axis_t i_axis;
  guint l;
  amide_data_t weight;
  intpoint_t start_frame, num_frames, i_frame;
  amide_time_t volume_start, volume_end;
  gchar * temp_string;
  realpoint_t box_rp[8];
  voxelpoint_t box_vp[8];
  amide_data_t box_value[8];
  realpoint_t slice_rp, volume_rp,diff, nearest_rp;
  voxelpoint_t volume_vp;
  amide_data_t weight1, weight2;

  /* ----- figure out what frames of this volume to include ----*/

  /* figure out what frame to start at */
  start_frame = 0; 
  i_frame = 0;
  while (i_frame < volume->data_set->dim.t) {
    volume_start = volume_start_time(volume,i_frame);
    if (start > volume_start-CLOSE)
      start_frame = i_frame;
    i_frame++;
  }
  volume_start = volume_start_time(volume, start_frame);

  /* and figure out what frame to end at */
  num_frames = volume->data_set->dim.t + 1;
  i_frame = start_frame;
  while ((i_frame < volume->data_set->dim.t) && (num_frames > volume->data_set->dim.t)) {
    volume_end = volume_start_time(volume,i_frame)+volume->frame_duration[i_frame];
    if ((start+duration) < volume_end+CLOSE)
      num_frames = i_frame-start_frame+1;
    i_frame++;
  }
  if (num_frames+start_frame > volume->data_set->dim.t)
    num_frames = volume->data_set->dim.t-start_frame;
  volume_end = volume_start_time(volume,start_frame+num_frames-1) + 
    volume->frame_duration[start_frame+num_frames-1];

  /* ------------------------- */


  /* initialize the return slice */
  if ((slice = volume_init()) == NULL) {
    g_warning("%s: couldn't allocate space for the slice volume structure", PACKAGE);
    return slice;
  }
  if ((slice->data_set = data_set_init()) == NULL) {
    g_warning("%s: couldn't allocate space for the slice data set structure", PACKAGE);
    return volume_free(slice);
  }
  slice->data_set->dim.x = ceil(fabs(far_corner.x)/requested_voxel_size.x);
  slice->data_set->dim.y = ceil(fabs(far_corner.y)/requested_voxel_size.y);
  slice->data_set->dim.z = 1;
  slice->data_set->dim.t = 1;
  slice->data_set->format = FLOAT;
  slice->voxel_size = requested_voxel_size;
  slice->coord_frame = slice_coord_frame;
  volume_recalc_far_corner(slice);
  slice->scan_start = volume_start;
  temp_string =  g_strdup_printf("slice from volume %s: size x %5.3f y %5.3f z %5.3f", 
				 volume->name, slice->corner.x, 
				 slice->corner.y, slice->corner.z);
  volume_set_name(slice,temp_string);
  volume_set_scan_date(slice, volume->scan_date);
  g_free(temp_string);

  if ((slice->frame_duration = volume_get_frame_duration_mem(slice)) == NULL) {
    g_warning("%s: couldn't allocate space for the slice frame duration array",PACKAGE);
    return volume_free(slice);
  }
  slice->frame_duration[0] = volume_end-volume_start;
  if ((slice->data_set->data = data_set_get_data_mem(slice->data_set)) == NULL) {
    g_warning("%s: couldn't allocate space for the slice",PACKAGE);
    return volume_free(slice);
  }

  /* ----------------------- */



#ifdef AMIDE_DEBUG
  {
    realpoint_t real_corner[2];
    /* convert to real space */
    real_corner[0] = rs_offset(slice->coord_frame);
    real_corner[1] = realspace_alt_coord_to_base(slice->corner,
						 slice->coord_frame);
    //    g_print("new slice from volume %s\t---------------------\n",volume->name);
    //    g_print("\tdim\t\tx %d\t\ty %d\t\tz %d\n",
    //	    slice->data_set->dim.x, slice->data_set->dim.y, slice->data_set->dim.z);
    //    g_print("\treal corner[0]\tx %5.4f\ty %5.4f\tz %5.4f\n",
    //	    real_corner[0].x,real_corner[0].y,real_corner[0].z);
    //    g_print("\treal corner[1]\tx %5.4f\ty %5.4f\tz %5.4f\n",
    //	    real_corner[1].x,real_corner[1].y,real_corner[1].z);
    //    g_print("\tvolume\t\tstart\t%5.4f\tend\t%5.3f\tframes %d to %d\n",
    //	    volume_start, volume_end,start_frame,start_frame+num_frames-1);
    //    g_print("new_slice from volume %s, frames %d to %d, z offset %5.3f\n",volume->name,
    //    	    start_frame, start_frame+num_frames-1, real_corner[0].z);
  }
#endif


  /* voxel_length is the length of a voxel given the coordinate frame of the slice.
     this is used to figure out how many iterations in the z direction we need to do */
  alt.x = alt.y = 0.0;
  alt.z = 1.0;
  alt = realspace_alt_dim_to_alt(alt, slice->coord_frame, volume->coord_frame);
  alt = rp_mult(alt, volume->voxel_size);
  voxel_length = REALPOINT_MAGNITUDE(alt);
  z_steps = slice->voxel_size.z/voxel_length;

  /* initialize our data set */
  data_set_FLOAT_initialize_data(slice->data_set, 0.0);

  switch(interpolation) {

  case TRILINEAR:
    i.t = i.z = 0;
    /* iterate over the frames we'll be incorporating into this slice */
    for (i_frame = start_frame; i_frame < start_frame+num_frames;i_frame++) {

      /* iterate over the number of planes we'll be compressing into this slice */
      for (z = 0; z < ceil(z_steps-SMALL)-SMALL; z++) {
	
	/* the slices z_coordinate for this iteration's slice voxel */
	slice_rp.z = z*voxel_length+voxel_length/2.0;

	/* weight is between 0 and 1, this is used to weight the last voxel 
	   in the slice's z direction, and in averaging frames */
	if (floor(z_steps) > z)
	  weight = 1.0/(num_frames * z_steps);
	else
	  weight = (z_steps-floor(z_steps)) / (num_frames * z_steps);

	/* iterate over the y dimension */
	for (i.y = 0; i.y < slice->data_set->dim.y; i.y++) {

	  /* the slice y_coordinate of the center of this iteration's slice voxel */
	  slice_rp.y = (((floatpoint_t) i.y)*slice->corner.y/slice->data_set->dim.y) + slice->voxel_size.y/2.0;
	  
	  /* the slice x coord of the center of the first slice voxel in this loop */
	  slice_rp.x = slice->voxel_size.x/2.0;

	  /* iterate over the x dimension */
	  for (i.x = 0; i.x < slice->data_set->dim.x; i.x++) {

	    /* translate the current point in slice space into the volume's coordinate frame */
	    volume_rp = realspace_alt_coord_to_alt(slice_rp, 
						   slice->coord_frame, 
						   volume->coord_frame);

	    /* get the nearest neighbor in the volume to this slice voxel */
	    VOLUME_REALPOINT_TO_VOXEL(volume, volume_rp, i_frame, volume_vp);
	    VOLUME_VOXEL_TO_REALPOINT(volume, volume_vp, nearest_rp);

	    /* figure out which way to go to get the nearest voxels to our slice voxel*/
	    REALPOINT_SUB(volume_rp, nearest_rp, diff);

	    /* figure out which voxels to look at */
	    for (l=0; l<8; l=l+1) {
	      if (diff.x < 0)
		box_vp[l].x = (l & 0x1) ? volume_vp.x-1 : volume_vp.x;
	      else /* diff.x >= 0 */
		box_vp[l].x = (l & 0x1) ? volume_vp.x : volume_vp.x+1;
	      if (diff.y < 0)
		box_vp[l].y = (l & 0x2) ? volume_vp.y-1 : volume_vp.y;
	      else /* diff.y >= 0 */
		box_vp[l].y = (l & 0x2) ? volume_vp.y : volume_vp.y+1;
	      if (diff.z < 0)
		box_vp[l].z = (l & 0x4) ? volume_vp.z-1 : volume_vp.z;
	      else /* diff.z >= 0 */
		box_vp[l].z = (l & 0x4) ? volume_vp.z : volume_vp.z+1;
	      box_vp[l].t = volume_vp.t;

	      VOLUME_VOXEL_TO_REALPOINT(volume, box_vp[l], box_rp[l]);

	      /* get the value of the point on the box */
	      if (data_set_includes_voxel(volume->data_set, box_vp[l]))
		box_value[l] = VOLUME_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENTS(volume, box_vp[l]);
	      else
		box_value[l] = EMPTY;
	    }
	    
	    /* do the x direction linear interpolation of the sets of two points */
	    for (l=0;l<8;l=l+2) {
	      max_diff = box_rp[l+1].x-box_rp[l].x;
	      weight1 = ((max_diff - (volume_rp.x - box_rp[l].x))/max_diff);
	      weight2 = ((max_diff - (box_rp[l+1].x - volume_rp.x))/max_diff);
	      box_value[l] = (box_value[l] * weight1) + (box_value[l+1] * weight2);
	    }

	    /* do the y direction linear interpolation of the sets of two points */
	    for (l=0;l<8;l=l+4) {
	      max_diff = box_rp[l+2].y-box_rp[l].y;
	      weight1 = ((max_diff - (volume_rp.y - box_rp[l].y))/max_diff);
	      weight2 = ((max_diff - (box_rp[l+2].y - volume_rp.y))/max_diff);
	      box_value[l] = (box_value[l] * weight1) + (box_value[l+2] * weight2);
	    }

	    /* do the z direction linear interpolation of the sets of two points */
	    for (l=0;l<8;l=l+8) {
	      max_diff = box_rp[l+4].z-box_rp[l].z;
	      weight1 = ((max_diff - (volume_rp.z - box_rp[l].z))/max_diff);
	      weight2 = ((max_diff - (box_rp[l+4].z - volume_rp.z))/max_diff);
	      box_value[l] = (box_value[l] * weight1) + (box_value[l+4] * weight2);
	    }

	    DATA_SET_FLOAT_SET_CONTENT(slice->data_set,i)+=weight*box_value[0];

	    slice_rp.x += slice->voxel_size.x; 
	  }
	}
      }
    }
    break;
  case NEAREST_NEIGHBOR:
  default:  
    /* figure out what point in the volume we're going to start at */
    alt.x = slice->voxel_size.x/2.0;
    alt.y = slice->voxel_size.y/2.0;
    alt.z = voxel_length/2.0;
    volume_rp = realspace_alt_coord_to_alt(alt, slice->coord_frame, volume->coord_frame);

    /* figure out what stepping one voxel in a given direction in our slice cooresponds to in our volume */
    for (i_axis = 0; i_axis < NUM_AXIS; i_axis++) {
      alt.x = (i_axis == XAXIS) ? slice->voxel_size.x : 0.0;
      alt.y = (i_axis == YAXIS) ? slice->voxel_size.y : 0.0;
      alt.z = (i_axis == ZAXIS) ? voxel_length : 0.0;
      alt = rp_add(rp_sub(realspace_alt_coord_to_base(alt, slice->coord_frame),
			  rs_offset(slice->coord_frame)),
		   rs_offset(volume->coord_frame));
      stride[i_axis] = realspace_base_coord_to_alt(alt, volume->coord_frame);
    }

    i.t = i.z = 0;
    /* iterate over the number of frames we'll be incorporating into this slice */
    for (i_frame = start_frame; i_frame < start_frame+num_frames;i_frame++) {

      /* iterate over the number of planes we'll be compressing into this slice */
      for (z = 0; z < ceil(z_steps-SMALL)-SMALL; z++) { 
	last[ZAXIS] = volume_rp;

	/* weight is between 0 and 1, this is used to weight the last voxel 
	   in the slice's z direction, and in averaging frames */
	if (floor(z_steps) > z)
	  weight = 1.0/(num_frames * z_steps);
	else
	  weight = (z_steps-floor(z_steps)) / (num_frames * z_steps);
	
	/* iterate over the y dimension */
	for (i.y = 0; i.y < slice->data_set->dim.y; i.y++) {
	  last[YAXIS] = volume_rp;

	  /* and iteratate over x */
	  for (i.x = 0; i.x < slice->data_set->dim.x; i.x++) {
	    VOLUME_REALPOINT_TO_VOXEL(volume, volume_rp, i_frame, j);
	    if (!data_set_includes_voxel(volume->data_set,j))
	      DATA_SET_FLOAT_SET_CONTENT(slice->data_set,i) += weight*EMPTY;
	    else
	      DATA_SET_FLOAT_SET_CONTENT(slice->data_set,i) += 
		weight*VOLUME_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENTS(volume,j);
	    REALPOINT_ADD(volume_rp, stride[XAXIS], volume_rp); 
	  }
	  REALPOINT_ADD(last[YAXIS], stride[YAXIS], volume_rp);
	}
	REALPOINT_ADD(last[ZAXIS], stride[ZAXIS], volume_rp); 
      }
    }
    break;
  }

  /* figure out the max and min */
  if (need_calc_max_min)
    volume_recalc_max_min(slice);

  return slice;
}

