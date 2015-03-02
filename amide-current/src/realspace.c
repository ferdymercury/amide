/* realspace.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
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
#include <glib.h>
#include <math.h>
#include "amide.h"
#include "realspace.h"

/* external variables */
const gchar * axis_names[] = {"x", "y", "z"};

const realpoint_t default_normal[NUM_AXIS] = {{0.0,0.0,1.0},
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

/* returns abs(rp1) for realpoint structures */
inline realpoint_t rp_abs(const realpoint_t rp1) {
  realpoint_t temp;
  temp.x = fabs(rp1.x);
  temp.y = fabs(rp1.y);
  temp.z = fabs(rp1.z);
  return temp;
}

/* returns rp1+rp2 for realpoint structures */
inline realpoint_t rp_add(const realpoint_t rp1,const realpoint_t rp2) {
  realpoint_t temp;
  temp.x = rp1.x+rp2.x;
  temp.y = rp1.y+rp2.y;
  temp.z = rp1.z+rp2.z;
  return temp;
}

/* returns rp1-rp2 for realpoint structures */
inline realpoint_t rp_sub(const realpoint_t rp1,const realpoint_t rp2) {
  realpoint_t temp;
  temp.x = rp1.x-rp2.x;
  temp.y = rp1.y-rp2.y;
  temp.z = rp1.z-rp2.z;
  return temp;
}

/* returns rp1.*rp2 for realpoint structures */
inline realpoint_t rp_mult(const realpoint_t rp1,const realpoint_t rp2) {
  realpoint_t temp;
  temp.x = rp1.x*rp2.x;
  temp.y = rp1.y*rp2.y;
  temp.z = rp1.z*rp2.z;
  return temp;
}

/* returns rp_abs(rp1-rp2) for realpoint structures */
inline realpoint_t rp_diff(const realpoint_t rp1,const realpoint_t rp2) {
  realpoint_t temp;
  temp.x = fabs(rp1.x-rp2.x);
  temp.y = fabs(rp1.y-rp2.y);
  temp.z = fabs(rp1.z-rp2.z);
  return temp;
}

/* returns cm*rp1 for realpoint structures */
inline realpoint_t rp_cmult(const floatpoint_t cmult,const realpoint_t rp1) {
  realpoint_t temp;
  temp.x = cmult*rp1.x;
  temp.y = cmult*rp1.y;
  temp.z = cmult*rp1.z;
  return temp;
}


/* returns true if the realpoint is in the given box */
gboolean realpoint_in_box(const realpoint_t p,
			  const realpoint_t p0,
			  const realpoint_t p1) {


  return (
	  (
	   (
	    ((p.z >= p0.z-CLOSE) && (p.z <= p1.z+CLOSE)) ||
	    ((p.z <= p0.z+CLOSE) && (p.z >= p1.z-CLOSE))) && 
	   (
	    ((p.y >= p0.y-CLOSE) && (p.y <= p1.y+CLOSE)) ||
	    ((p.y <= p0.y+CLOSE) && (p.y >= p1.y-CLOSE))) && 
	   (
	    ((p.x >= p0.x-CLOSE) && (p.x <= p1.x+CLOSE)) ||
	    ((p.x <= p0.x+CLOSE) && (p.x >= p1.x-CLOSE)))));

}


/* returns true if the realpoint is in the elliptic cylinder 
   note: height is in the z direction, and radius.z isn't used for anything */
gboolean realpoint_in_elliptic_cylinder(const realpoint_t p,
					const realpoint_t center,
					const floatpoint_t height,
					const realpoint_t radius) {

  return ((1.0+2*CLOSE >= 
	   (pow((p.x-center.x),2.0)/pow(radius.x,2.0) +
	    pow((p.y-center.y),2.0)/pow(radius.y,2.0)))
	  &&
	  ((p.z > (center.z-height/2.0)-CLOSE) && 
	   (p.z < (center.z+height/2.0)-CLOSE)));
  
}



/* returns true if the realpoint is in the ellipsoid */
gboolean realpoint_in_ellipsoid(const realpoint_t p,
				const realpoint_t center,
				const realpoint_t radius) {

  	  
  return (1.0 + 3*CLOSE >= 
	  (pow((p.x-center.x),2.0)/pow(radius.x,2.0) +
	   pow((p.y-center.y),2.0)/pow(radius.y,2.0) +
	   pow((p.z-center.z),2.0)/pow(radius.z,2.0)));
}


/* given the ordered corners[] (i.e corner[0].x < corner[1].x, etc.)
   and the coord_space of those corners, this function will return an
   ordered set of corners in a new coord space that completely enclose
   the given corners */

void realspace_get_enclosing_corners(const realspace_t in_coord_frame, const realpoint_t in_corner[], 
				     const realspace_t out_coord_frame, realpoint_t out_corner[] ) {

  realpoint_t temp;
  guint corner;

  

  for (corner=0; corner<8 ; corner++) {
    temp.x = in_corner[(corner & 0x1) ? 1 : 0].x;
    temp.y = in_corner[(corner & 0x2) ? 1 : 0].y;
    temp.z = in_corner[(corner & 0x4) ? 1 : 0].z;
    temp = realspace_alt_coord_to_alt(temp,
				      in_coord_frame,
				      out_coord_frame);
    if (corner==0)
      out_corner[0]=out_corner[1]=temp;
    else {
      if (temp.x < out_corner[0].x) out_corner[0].x=temp.x;
      if (temp.x > out_corner[1].x) out_corner[1].x=temp.x;
      if (temp.y < out_corner[0].y) out_corner[0].y=temp.y;
      if (temp.y > out_corner[1].y) out_corner[1].y=temp.y;
      if (temp.z < out_corner[0].z) out_corner[0].z=temp.z;
      if (temp.z > out_corner[1].z) out_corner[1].z=temp.z;

    }
  }

  return;
}



/* adjusts the given axis into an orthogonal set */
void realspace_make_orthogonal(realpoint_t axis[]) {

  realpoint_t temp;

  /* make orthogonal via gram-schmidt */

  /* leave the xaxis as is */

  /* make the y axis orthogonal */
  REALPOINT_MADD(1.0,
		 axis[YAXIS],
		 -1.0*REALPOINT_DOT_PRODUCT(axis[XAXIS],axis[YAXIS]) /
		 REALPOINT_DOT_PRODUCT(axis[XAXIS],axis[XAXIS]),
		 axis[XAXIS],
		 axis[YAXIS]);

  /* and make the z axis orthogonal */
  REALPOINT_MADD(1.0,
		 axis[ZAXIS],
		 -1.0*REALPOINT_DOT_PRODUCT(axis[XAXIS],axis[ZAXIS]) /
		 REALPOINT_DOT_PRODUCT(axis[XAXIS],axis[XAXIS]),
		 axis[XAXIS],
		 temp);
  REALPOINT_MADD(1.0,
		 temp,
		 -1.0*REALPOINT_DOT_PRODUCT(axis[YAXIS],axis[ZAXIS]) /
		 REALPOINT_DOT_PRODUCT(axis[YAXIS],axis[YAXIS]),
		 axis[YAXIS],
		 temp);
  return;
}

/* adjusts the given axis into a true orthonormal axis set */
void realspace_make_orthonormal(realpoint_t axis[]) {

  /* make orthogonal via gram-schmidt */
  realspace_make_orthogonal(axis);

  /* now normalize the axis to make it orthonormal */
  REALPOINT_CMULT(1.0/sqrt(REALPOINT_DOT_PRODUCT(axis[XAXIS],axis[XAXIS])),
		  axis[XAXIS],
		  axis[XAXIS]);
  REALPOINT_CMULT(1.0/sqrt(REALPOINT_DOT_PRODUCT(axis[YAXIS],axis[YAXIS])),
		  axis[YAXIS],
		  axis[YAXIS]);
  REALPOINT_CMULT(1.0/sqrt(REALPOINT_DOT_PRODUCT(axis[ZAXIS],axis[ZAXIS])),
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

/* returns the axis_t of the orthogonal axis for a given view */
axis_t realspace_get_orthogonal_which_axis(const view_t view) {

  switch(view) {
  case CORONAL:
    return YAXIS;
    break;
  case SAGITTAL:
    return XAXIS;
    break;
  case TRANSVERSE:
  default:
    return ZAXIS;
    break;
  }
}

/* returns the normal axis for the given view */
realpoint_t realspace_get_orthogonal_normal(const realpoint_t axis[],
					    const view_t view) {

  return axis[realspace_get_orthogonal_which_axis(view)];
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

/* given a coordinate frame, return an orthogonal coordinate frame */
realspace_t realspace_get_orthogonal_coord_frame(const realspace_t in_coord_frame,
						 const view_t view) {

  realspace_t return_coord_frame;
  axis_t i_axis;


  /* getting the axis is easy, just spin the current viewing axis based on which
     view we want */
  for (i_axis=0;i_axis<NUM_AXIS;i_axis++)
    return_coord_frame.axis[i_axis] = 
      realspace_get_orthogonal_axis(in_coord_frame.axis,view,i_axis);

  return_coord_frame.offset = in_coord_frame.offset;

  return return_coord_frame;
}


/* convert a point in an alternative coordinate frame to the base
   coordinate frame */
inline realpoint_t realspace_alt_coord_to_base(const realpoint_t in,
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
inline realpoint_t realspace_base_coord_to_alt(realpoint_t in,
					       const realspace_t out_alt_coord_frame) {

  realpoint_t return_point;
  floatpoint_t detA,detBx,detBy,detBz;

  /* compensate the inpoint for the offset of the new coordinate frame */
  REALPOINT_SUB(in, out_alt_coord_frame.offset, in);
  
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
inline realpoint_t realspace_alt_coord_to_alt(const realpoint_t in,
					      const realspace_t in_alt_coord_frame,
					      const realspace_t out_alt_coord_frame) {

  realpoint_t return_point;

  return_point = realspace_alt_coord_to_base(in,in_alt_coord_frame);
  return_point = realspace_base_coord_to_alt(return_point,out_alt_coord_frame);

  return return_point;
}


/* converts a "dimensional" quantity (i.e. the size of a voxel) from a
   given coordinate system to the base coordinate system */
inline realpoint_t realspace_alt_dim_to_base(const realpoint_t in,
					     const realspace_t in_alt_coord_frame) {

  realpoint_t return_point;

  REALPOINT_ABS(in, return_point);/* dim's should always be positive */

  return_point = realspace_alt_coord_to_base(return_point,in_alt_coord_frame);
  REALPOINT_SUB(return_point,in_alt_coord_frame.offset,return_point);
    
  REALPOINT_ABS(return_point, return_point);/* dim's should always be positive */

  return return_point;
}

/* converts a "dimensional" quantity (i.e. the size of a voxel) from the
   base coordinate system to a given coordinate system */
inline realpoint_t realspace_base_dim_to_alt(const realpoint_t in,
					     const realspace_t out_alt_coord_frame) {

  realpoint_t return_point;
  
  REALPOINT_ABS(in, return_point);/* dim's should always be positive */

  REALPOINT_ADD(return_point,out_alt_coord_frame.offset,return_point);
  return_point = realspace_base_coord_to_alt(return_point,out_alt_coord_frame);
    
  REALPOINT_ABS(return_point, return_point);/* dim's should always be positive */

  return return_point;
}


/* converts a "dimensional" quantity (i.e. the size of a voxel) from a given
   coordinate system to another coordinate system */
inline realpoint_t realspace_alt_dim_to_alt(const realpoint_t in,
					    const realspace_t in_alt_coord_frame,
					    const realspace_t out_alt_coord_frame) {

  realpoint_t return_point;
  
  return_point = realspace_alt_dim_to_base(in,
					   in_alt_coord_frame);
  return_point = realspace_base_dim_to_alt(return_point,
					   out_alt_coord_frame);
  
  return return_point;
}














