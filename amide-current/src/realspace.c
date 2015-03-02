/* realspace.c
 *
 * Part of amide - Amide's a Medical Image Dataset Viewer
 * Copyright (C) 2000 Andy Loening
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
#include <glib.h>
#include <math.h>
#include "amide.h"
#include "realspace.h"


/* convenience definitions */
const realpoint_t default_normal[NUM_VIEWS] = {{0.0,0.0,1.0},
					       {0.0,1.0,0.0},
					       {1.0,0.0,0.0}};

const realpoint_t default_axis[NUM_AXIS] = {{1.0,0.0,0.0},
					    {0.0,1.0,0.0},
					    {0.0,0.0,1.0}};

const realpoint_t default_rotation[NUM_AXIS] = {{0.0,0.0,0.0},
						{-M_PI_2,0.0,0.0},
						{-M_PI_2,-M_PI_2,-M_PI_2}};
const realpoint_t realpoint_init = {0.0,0.0,0.0};

const voxelpoint_t voxelpoint_init = {0,0,0};


/* adjusts the given axis into an orthogonal set */
void realspace_make_orthogonal(realpoint_t axis[]) {

  realpoint_t temp;

  /* make orthogonal via gram-schmidt */

  /* leave the xaxis as is */

  /* make the y axis orthogonal */
  REALSPACE_MADD(1.0,
		 axis[YAXIS],
		 -1.0*REALSPACE_DOT_PRODUCT(axis[XAXIS],axis[YAXIS]) /
		 REALSPACE_DOT_PRODUCT(axis[XAXIS],axis[XAXIS]),
		 axis[XAXIS],
		 axis[YAXIS]);

  /* and make the z axis orthogonal */
  REALSPACE_MADD(1.0,
		 axis[ZAXIS],
		 -1.0*REALSPACE_DOT_PRODUCT(axis[XAXIS],axis[ZAXIS]) /
		 REALSPACE_DOT_PRODUCT(axis[XAXIS],axis[XAXIS]),
		 axis[XAXIS],
		 temp);
  REALSPACE_MADD(1.0,
		 temp,
		 -1.0*REALSPACE_DOT_PRODUCT(axis[YAXIS],axis[ZAXIS]) /
		 REALSPACE_DOT_PRODUCT(axis[YAXIS],axis[YAXIS]),
		 axis[YAXIS],
		 temp);
  return;
}

/* adjusts the given axis into a true orthonormal axis set */
void realspace_make_orthonormal(realpoint_t axis[]) {

  /* make orthogonal via gram-schmidt */
  realspace_make_orthogonal(axis);

  /* now normalize the axis to make it orthonormal */
  REALSPACE_CMULT(1.0/sqrt(REALSPACE_DOT_PRODUCT(axis[XAXIS],axis[XAXIS])),
		  axis[XAXIS],
		  axis[XAXIS]);
  REALSPACE_CMULT(1.0/sqrt(REALSPACE_DOT_PRODUCT(axis[YAXIS],axis[YAXIS])),
		  axis[YAXIS],
		  axis[YAXIS]);
  REALSPACE_CMULT(1.0/sqrt(REALSPACE_DOT_PRODUCT(axis[ZAXIS],axis[ZAXIS])),
		  axis[ZAXIS],
		  axis[ZAXIS]);

  return;
}

/* rotate the vector on the given axis by the given rotation */
realpoint_t realspace_rotate_on_axis(const realpoint_t * in,
				     const realpoint_t * axis,
				     const floatpoint_t theta) {

  realpoint_t return_vector;

  return_vector.x = 
    (pow(axis->x,2.0) + cos(theta) * (1.0 - pow(axis->x,2.0))) * in->x +
    (axis->x*axis->y*(1.0-cos(theta)) - axis->z * sin(theta)) * in->y +
    (axis->z*axis->x*(1.0-cos(theta)) + axis->y * sin(theta)) * in->z;
  return_vector.y = 
    (axis->x*axis->y*(1.0-cos(theta)) + axis->z * sin(theta)) * in->x +
    (pow(axis->y,2.0) + cos(theta) * (1.0 - pow(axis->y,2.0))) * in->y +
    (axis->y*axis->z*(1.0-cos(theta)) - axis->x * sin(theta)) * in->z;
  return_vector.z = 
    (axis->z*axis->x*(1.0-cos(theta)) - axis->y * sin(theta)) * in->x +
    (axis->y*axis->z*(1.0-cos(theta)) + axis->x * sin(theta)) * in->y +
    (pow(axis->z,2.0) + cos(theta) * (1.0 - pow(axis->z,2.0))) * in->z;

  return return_vector;
}

/* returns the normal axis for the given view */
realpoint_t realspace_get_orthogonal_normal(const realpoint_t axis[],
					    const view_t i) {

  switch(i) {
  case CORONAL:
    return axis[YAXIS];
    break;
  case SAGITTAL:
    return axis[XAXIS];
    break;
  case TRANSVERSE:
  default:
    return axis[ZAXIS];
    break;
  }
}

/* returns an axis vector which corresponds to the orthogonal axis (specified
   by ax) in the given view (i.e. coronal, sagittal, etc.) given the current
   axis */
realpoint_t realspace_get_orthogonal_axis(const realpoint_t axis[],
					  const view_t view,
					  const axis_t ax) {
  switch(view) {
  case CORONAL:
    switch (ax) {
    case XAXIS:
      return axis[XAXIS];
      break;
    case YAXIS:
      return axis[ZAXIS];
      break;
    case ZAXIS:
    default:
      return axis[YAXIS];
      break;
    }
    break;
  case SAGITTAL:
    switch (ax) {
    case XAXIS:
      return axis[YAXIS];
      break;
    case YAXIS:
      return axis[ZAXIS];
      break;
    case ZAXIS:
    default:
      return axis[XAXIS];
      break;
    }
    break;
  case TRANSVERSE:
  default:
    switch (ax) {
    case XAXIS:
      return axis[XAXIS];
      break;
    case YAXIS:
      return axis[YAXIS];
      break;
    case ZAXIS:
    default:
      return axis[ZAXIS];
      break;
    }
    break;
  }
}

/* convert a point in an alternative coordinate frame to the base
   coordinate frame */
realpoint_t realspace_alt_coord_to_base(const realpoint_t in,
					const realspace_t in_alt_coord_frame) {

  realpoint_t return_point;

  return_point.x = 
    in.x * in_alt_coord_frame.axis[XAXIS].x +
    in.y * in_alt_coord_frame.axis[YAXIS].x +
    in.z * in_alt_coord_frame.axis[ZAXIS].x +
    in_alt_coord_frame.offset.x;
  return_point.y = 
    in.x * in_alt_coord_frame.axis[XAXIS].y +
    in.y * in_alt_coord_frame.axis[YAXIS].y +
    in.z * in_alt_coord_frame.axis[ZAXIS].y +
    in_alt_coord_frame.offset.y;
  return_point.z = 
    in.x * in_alt_coord_frame.axis[XAXIS].z +
    in.y * in_alt_coord_frame.axis[YAXIS].z +
    in.z * in_alt_coord_frame.axis[ZAXIS].z +
    in_alt_coord_frame.offset.z;

  return return_point;
}

/* convert the given point from base coordinates to the given alternate
   coordinate frame */
realpoint_t realspace_base_coord_to_alt(realpoint_t in,
					const realspace_t out_alt_coord_frame) {

  realpoint_t return_point;
  floatpoint_t detA,detBx,detBy,detBz;

  /* compensate the inpoint for the offset of the new coordinate frame */
  REALSPACE_SUB(in, out_alt_coord_frame.offset, in);
  
  /* figure out the determinate of the view_axis */
  detA = 0.0
    + out_alt_coord_frame.axis[XAXIS].x*out_alt_coord_frame.axis[YAXIS].y*out_alt_coord_frame.axis[ZAXIS].z    
    + out_alt_coord_frame.axis[YAXIS].x*out_alt_coord_frame.axis[ZAXIS].y*out_alt_coord_frame.axis[XAXIS].z    
    + out_alt_coord_frame.axis[ZAXIS].x*out_alt_coord_frame.axis[XAXIS].y*out_alt_coord_frame.axis[YAXIS].z    
    - out_alt_coord_frame.axis[XAXIS].x*out_alt_coord_frame.axis[ZAXIS].y*out_alt_coord_frame.axis[YAXIS].z    
    - out_alt_coord_frame.axis[YAXIS].x*out_alt_coord_frame.axis[XAXIS].y*out_alt_coord_frame.axis[ZAXIS].z    
    - out_alt_coord_frame.axis[ZAXIS].x*out_alt_coord_frame.axis[YAXIS].y*out_alt_coord_frame.axis[XAXIS].z;

  /* and figure out the "determinates" to use for cramer's rule */
  detBx = 0.0
    + in.x*out_alt_coord_frame.axis[YAXIS].y*out_alt_coord_frame.axis[ZAXIS].z 
    + out_alt_coord_frame.axis[YAXIS].x*out_alt_coord_frame.axis[ZAXIS].y*in.z
    + out_alt_coord_frame.axis[ZAXIS].x*in.y*out_alt_coord_frame.axis[YAXIS].z
    - in.x*out_alt_coord_frame.axis[ZAXIS].y*out_alt_coord_frame.axis[YAXIS].z
    - out_alt_coord_frame.axis[YAXIS].x*in.y*out_alt_coord_frame.axis[ZAXIS].z
    - out_alt_coord_frame.axis[ZAXIS].x*out_alt_coord_frame.axis[YAXIS].y*in.z;
  detBy = 0.0
    + out_alt_coord_frame.axis[XAXIS].x*in.y*out_alt_coord_frame.axis[ZAXIS].z 
    + in.x*out_alt_coord_frame.axis[ZAXIS].y*out_alt_coord_frame.axis[XAXIS].z
    + out_alt_coord_frame.axis[ZAXIS].x*out_alt_coord_frame.axis[XAXIS].y*in.z
    - out_alt_coord_frame.axis[XAXIS].x*out_alt_coord_frame.axis[ZAXIS].y*in.z
    - in.x*out_alt_coord_frame.axis[XAXIS].y*out_alt_coord_frame.axis[ZAXIS].z
    - out_alt_coord_frame.axis[ZAXIS].x*in.y*out_alt_coord_frame.axis[XAXIS].z;
  detBz = 0.0 
    + out_alt_coord_frame.axis[XAXIS].x*out_alt_coord_frame.axis[YAXIS].y*in.z 
    + out_alt_coord_frame.axis[YAXIS].x*in.y*out_alt_coord_frame.axis[XAXIS].z
    + in.x*out_alt_coord_frame.axis[XAXIS].y*out_alt_coord_frame.axis[YAXIS].z
    - out_alt_coord_frame.axis[XAXIS].x*in.y*out_alt_coord_frame.axis[YAXIS].z
    - out_alt_coord_frame.axis[YAXIS].x*out_alt_coord_frame.axis[XAXIS].y*in.z
    - in.x*out_alt_coord_frame.axis[YAXIS].y*out_alt_coord_frame.axis[XAXIS].z;

  /* get around some obscure compile bug.... */
  if (isnan(detA)!= 0) detA = 1.0;
  if (isnan(detBx)!= 0) detBx = 0.0;
  if (isnan(detBy)!= 0) detBy = 0.0;
  if (isnan(detBz)!= 0) detBz = 0.0;
  /* try removing these four previous lines when something beyond gcc 2.95.2 
     comes out */

  /* do our inverse via Cramer's Rule */
  return_point.x = detBx/detA;
  return_point.y = detBy/detA;
  return_point.z = detBz/detA;


  return return_point;
}

/* converts a point from one coordinate frame to another */
realpoint_t realspace_alt_coord_to_alt(const realpoint_t in,
				       const realspace_t in_alt_coord_frame,
				       const realspace_t out_alt_coord_frame) {

  realpoint_t return_point;

  return_point = realspace_alt_coord_to_base(in,in_alt_coord_frame);
  return_point = realspace_base_coord_to_alt(return_point,out_alt_coord_frame);

  return return_point;
}


/* converts a "dimensional" quantity (i.e. the size of a voxel) from a
   given coordinate system to the base coordinate system */
realpoint_t realspace_alt_dim_to_base(const realpoint_t in,
				      const realspace_t in_alt_coord_frame) {

  realpoint_t return_point;

  REALSPACE_ABS(in, return_point);/* dim's should always be positive */

  return_point = realspace_alt_coord_to_base(return_point,in_alt_coord_frame);
  REALSPACE_SUB(return_point,in_alt_coord_frame.offset,return_point);
    
  REALSPACE_ABS(return_point, return_point);/* dim's should always be positive */

  return return_point;
}

/* converts a "dimensional" quantity (i.e. the size of a voxel) from the
   base coordinate system to a given coordinate system */
realpoint_t realspace_base_dim_to_alt(const realpoint_t in,
				      const realspace_t out_alt_coord_frame) {

  realpoint_t return_point;
  
  REALSPACE_ABS(in, return_point);/* dim's should always be positive */

  REALSPACE_ADD(return_point,out_alt_coord_frame.offset,return_point);
  return_point = realspace_base_coord_to_alt(return_point,out_alt_coord_frame);
    
  REALSPACE_ABS(return_point, return_point);/* dim's should always be positive */

  return return_point;
}


/* converts a "dimensional" quantity (i.e. the size of a voxel) from a given
   coordinate system to another coordinate system */
realpoint_t realspace_alt_dim_to_alt(const realpoint_t in,
				     const realspace_t in_alt_coord_frame,
				     const realspace_t out_alt_coord_frame) {

  realpoint_t return_point;
  
  return_point = realspace_alt_dim_to_base(in,
					   in_alt_coord_frame);
  return_point = realspace_base_dim_to_alt(return_point,
					   out_alt_coord_frame);
  
  return return_point;
}













