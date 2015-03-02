/* realspace.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2002 Andy Loening
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
#include "realspace.h"

/* external variables */
const gchar * axis_names[] = {"x", "y", "z"};

const realpoint_t default_axis[NUM_AXIS] = {{1.0,0.0,0.0},
					    {0.0,1.0,0.0},
					    {0.0,0.0,1.0}};
const realspace_t default_coord_frame = {{0.0,0.0,0.0},
					 {{1.0,0.0,0.0},
					  {0.0,1.0,0.0},
					  {0.0,0.0,1.0}}};

const realpoint_t zero_rp = {0.0,0.0,0.0};
const realpoint_t one_rp = {1.0,1.0,1.0};
const realpoint_t ten_rp = {10.0,10.0,10.0};

const voxelpoint_t zero_vp = {0,0,0,0};
const voxelpoint_t one_vp = {1,1,1,1};

/* returns abs(rp1) for realpoint structures */
inline realpoint_t rp_abs(const realpoint_t rp1) {
  realpoint_t temp;
  temp.x = fabs(rp1.x);
  temp.y = fabs(rp1.y);
  temp.z = fabs(rp1.z);
  return temp;
}
/* returns -rp1 for realpoint structures */
inline realpoint_t rp_neg(const realpoint_t rp1) {
  realpoint_t temp;
  temp.x = -rp1.x;
  temp.y = -rp1.y;
  temp.z = -rp1.z;
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
/* returns rp1./rp2 for realpoint structures */
inline realpoint_t rp_div(const realpoint_t rp1,const realpoint_t rp2) {
  realpoint_t temp;
  temp.x = rp1.x/rp2.x;
  temp.y = rp1.y/rp2.y;
  temp.z = rp1.z/rp2.z;
  return temp;
}
/* returns abs(rp1-rp2) for realpoint structures */
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

/* returns cross product of rp1 and rp2 for realpoint structures */
inline realpoint_t rp_cross_product(const realpoint_t rp1, const realpoint_t rp2) {
  realpoint_t temp;
  temp.x = rp1.y*rp2.z-rp1.z*rp2.y;
  temp.y = rp1.z*rp2.x-rp1.x*rp2.z;
  temp.z = rp1.x*rp2.y-rp1.y*rp2.x;
  return temp;
}

/* returns dot product of rp1 and rp2 for realpoint structures */
inline floatpoint_t rp_dot_product(const realpoint_t rp1, const realpoint_t rp2) {

  return rp1.x*rp2.x + rp1.y*rp2.y + rp1.z*rp2.z;

}


/* returns sqrt(rp_dot_product(rp1, rp1)) for realpoint structures */
inline floatpoint_t rp_mag(const realpoint_t rp1) {
  return sqrt(rp_dot_product(rp1, rp1));
}

/* returns the minimum dimension of the "box" defined by rp1*/
inline floatpoint_t rp_min_dim(const realpoint_t rp1) {
  return MIN( MIN(rp1.x,rp1.y), rp1.z);
}

/* returns the maximum dimension of the "box" defined by rp1 */
inline floatpoint_t rp_max_dim(const realpoint_t rp1) {
  return rp_mag(rp1);
}




/* returns abs(cp1-cp2) for canvaspoint structures */
inline canvaspoint_t cp_diff(const canvaspoint_t cp1,const canvaspoint_t cp2) {
  canvaspoint_t temp;
  temp.x = fabs(cp1.x-cp2.x);
  temp.y = fabs(cp1.y-cp2.y);
  return temp;
}
/* returns cp1-cp2 for canvaspoint structures */
inline canvaspoint_t cp_sub(const canvaspoint_t cp1,const canvaspoint_t cp2) {
  canvaspoint_t temp;
  temp.x = cp1.x-cp2.x;
  temp.y = cp1.y-cp2.y;
  return temp;
}
/* returns cp1+cp2 for canvaspoint structures */
inline canvaspoint_t cp_add(const canvaspoint_t cp1,const canvaspoint_t cp2) {
  canvaspoint_t temp;
  temp.x = cp1.x+cp2.x;
  temp.y = cp1.y+cp2.y;
  return temp;
}
/* returns dot product of cp1 and cp2 for canvaspoint structures */
inline floatpoint_t cp_dot_product(const canvaspoint_t cp1, const canvaspoint_t cp2) {

  return cp1.x*cp2.x + cp1.y*cp2.y;

}
/* returns sqrt(cp_dot_product(cp1, cp1)) for canvaspoint structures */
inline floatpoint_t cp_mag(const canvaspoint_t cp1) {
  return sqrt(cp_dot_product(cp1, cp1));
}


/* returns vp1+vp2 for voxelpoint structures */
inline voxelpoint_t vp_add(const voxelpoint_t vp1,const voxelpoint_t vp2) {
  voxelpoint_t temp;
  temp.x = vp1.x+vp2.x;
  temp.y = vp1.y+vp2.y;
  temp.z = vp1.z+vp2.z;
  temp.t = vp1.t+vp2.t;
  return temp;
}

/* returns vp1-vp2 for voxelpoint structures */
inline voxelpoint_t vp_sub(const voxelpoint_t vp1,const voxelpoint_t vp2) {
  voxelpoint_t temp;
  temp.x = vp1.x-vp2.x;
  temp.y = vp1.y-vp2.y;
  temp.z = vp1.z-vp2.z;
  temp.t = vp1.t-vp2.t;
  return temp;
}

/* returns vp1 == vp2 for voxelpoint structures */
inline gboolean vp_equal(const voxelpoint_t vp1, const voxelpoint_t vp2) {

  return ((vp1.x == vp2.x) &&
	  (vp1.y == vp2.y) &&
	  (vp1.z == vp2.z) &&
	  (vp1.t == vp2.t));
}
/* returns the maximum dimension of the "box" defined by vp1 */
inline floatpoint_t vp_max_dim(const voxelpoint_t vp1) {
  realpoint_t temp_rp;
  VOXEL_TO_REALPOINT(vp1, one_rp, temp_rp);
  return rp_mag(temp_rp);
}

/* little utility function for debugging */
void vp_print(gchar * message, const voxelpoint_t vp) {
  g_print("%s\t%d\t%d\t%d\t%d\n",message, vp.x, vp.y, vp.z, vp.t);
  return;
}

/* returns true if the realpoint is in the given box */
gboolean rp_in_box(const realpoint_t p,
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
gboolean rp_in_elliptic_cylinder(const realpoint_t p,
				 const realpoint_t center,
				 const floatpoint_t height,
				 const realpoint_t radius) {

  return ((1.0+2*CLOSE >= 
	   (pow((p.x-center.x),2.0)/pow(radius.x,2.0) +
	    pow((p.y-center.y),2.0)/pow(radius.y,2.0)))
	  &&
	  ((p.z >= (center.z-height/2.0)-CLOSE) && 
	   (p.z <= (center.z+height/2.0)-CLOSE)));
  
}



/* returns true if the realpoint is in the ellipsoid */
gboolean rp_in_ellipsoid(const realpoint_t p,
			 const realpoint_t center,
			 const realpoint_t radius) {

  	  
  return (1.0 + 3*CLOSE >= 
	  (pow((p.x-center.x),2.0)/pow(radius.x,2.0) +
	   pow((p.y-center.y),2.0)/pow(radius.y,2.0) +
	   pow((p.z-center.z),2.0)/pow(radius.z,2.0)));
}


/* little utility function for debugging */
void rp_print(gchar * message, const realpoint_t rp) {
  g_print("%s\t%5.3f\t%5.3f\t%5.3f\n",message, rp.x, rp.y, rp.z);
  return;
}

/* little utility function for debugging */
void rs_print(gchar * message, const realspace_t * coord_frame) {

  g_print("%s\toffset:\t%5.3f\t%5.3f\t%5.3f\n", message, 
	  coord_frame->offset.x, coord_frame->offset.y, coord_frame->offset.z);
  g_print("\taxis x:\t%5.3f\t%5.3f\t%5.3f\n",
	  coord_frame->axis[XAXIS].x,coord_frame->axis[XAXIS].y, coord_frame->axis[XAXIS].z);
  g_print("\taxis y:\t%5.3f\t%5.3f\t%5.3f\n",
	  coord_frame->axis[YAXIS].x,coord_frame->axis[YAXIS].y, coord_frame->axis[YAXIS].z);
  g_print("\taxis z:\t%5.3f\t%5.3f\t%5.3f\n",
	  coord_frame->axis[ZAXIS].x,coord_frame->axis[ZAXIS].y, coord_frame->axis[ZAXIS].z);
  return;
}

/* removes a reference to a coordinate frame struct, frees up the struct if no more references */
realspace_t * rs_unref(realspace_t * rs) {

  if (rs == NULL) return rs;

  /* sanity checks */
  g_return_val_if_fail(rs->ref_count > 0, NULL);
  
  rs->ref_count--;

  /* if we've removed all reference's, free remaining data structures */
  if (rs->ref_count == 0) {
    g_free(rs);
    rs = NULL;
  }

  return rs;
}

realspace_t * rs_init(void) {

  realspace_t * temp_rs;
  axis_t i_axis;

  if ((temp_rs = (realspace_t *) g_malloc(sizeof(realspace_t))) == NULL) {
    g_warning("Couldn't allocate memory for the realspace structure");
    return temp_rs;
  }
  temp_rs->ref_count = 1;

  /* put in some sensable values */
  temp_rs->offset = zero_rp;
  for (i_axis=0; i_axis<NUM_AXIS; i_axis++)
    temp_rs->axis[i_axis] = default_axis[i_axis];

  return temp_rs;
}

/* adds one to the reference count of a realspace */
realspace_t * rs_ref(realspace_t * src_rs) {
  g_return_val_if_fail(src_rs != NULL, NULL);   /* sanity checks */
  src_rs->ref_count++;
  return src_rs;
}

realspace_t * rs_copy(const realspace_t * src_rs) {

  realspace_t * dest_rs;
  axis_t i_axis;

  /* sanity checks */
  g_return_val_if_fail(src_rs != NULL, NULL);

  dest_rs = rs_init();
  dest_rs->offset = src_rs->offset;
  for (i_axis=0;i_axis<NUM_AXIS;i_axis++)
    dest_rs->axis[i_axis] = src_rs->axis[i_axis];

  return dest_rs;
}


/* adjusts the given axis into an orthogonal set */
static void make_orthogonal(realpoint_t axis[]) {

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
		 axis[ZAXIS]);
  return;
}

/* adjusts the given axis into a true orthonormal axis set */
static void make_orthonormal(realpoint_t axis[]) {

  /* make orthogonal via gram-schmidt */
  make_orthogonal(axis);

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

/* sets the offset */
void rs_set_offset(realspace_t * rs, const realpoint_t new_offset) {

  rs->offset = new_offset;

  /* get around some bizarre compiler bug */
  if (isnan(rs->offset.x) || isnan(rs->offset.y) || isnan(rs->offset.z)) {
    g_warning("inappropriate offset, probably a compiler bug, working around");
      rs->offset = default_coord_frame.offset;
  }
    
  return;
}


/* sets the axis, makes sure it's orthonormal */
void rs_set_axis(realspace_t * rs, const realpoint_t new_axis[]) {

  axis_t i_axis;

  /* set the axis of the realspace */
  for (i_axis=0;i_axis<NUM_AXIS;i_axis++)
    rs->axis[i_axis]=new_axis[i_axis];


  /* make sure it's orthonormal */
  make_orthonormal(rs->axis);

  /* get around some bizarre compiler bug */
  if (isnan(rs->axis[XAXIS].x) || isnan(rs->axis[XAXIS].y) || isnan(rs->axis[XAXIS].z) ||
      isnan(rs->axis[YAXIS].x) || isnan(rs->axis[YAXIS].y) || isnan(rs->axis[YAXIS].z) ||
      isnan(rs->axis[ZAXIS].x) || isnan(rs->axis[ZAXIS].y) || isnan(rs->axis[ZAXIS].z)) {
    g_warning("inappropriate axis, probably a compiler bug, working around");
    for (i_axis=0;i_axis<NUM_AXIS;i_axis++)
      rs->axis[i_axis]=default_coord_frame.axis[i_axis];
  }
  return;
}

/* change the settings of a coord frame */
void rs_set_coord_frame(realspace_t * dest_rs, const realspace_t * src_rs) {

  g_assert(dest_rs != NULL);
  g_assert(src_rs != NULL);
  
  rs_set_axis(dest_rs, src_rs->axis);
  rs_set_offset(dest_rs, src_rs->offset);

  return;
}

/* invert an axis in the coord frame */
void rs_invert_axis(realspace_t * rs, const axis_t which_axis) {

  rs->axis[which_axis] = rp_neg(rs->axis[which_axis]);

  return;
}

/* given the ordered corners[] (i.e corner[0].x < corner[1].x, etc.)
   and the coord_space of those corners, this function will return an
   ordered set of corners in a new coord space that completely enclose
   the given corners */

void realspace_get_enclosing_corners(const realspace_t * in_coord_frame, const realpoint_t in_corner[], 
				     const realspace_t * out_coord_frame, realpoint_t out_corner[] ) {

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



/* rotate the vector on the given axis by the given rotation */
realpoint_t realpoint_rotate_on_axis(const realpoint_t in,
				     const realpoint_t axis,
				     const floatpoint_t theta) {

  realpoint_t return_vector;

  return_vector.x = 
    (pow(axis.x,2.0) + cos(theta) * (1.0 - pow(axis.x,2.0))) * in.x +
    (axis.x*axis.y*(1.0-cos(theta)) - axis.z * sin(theta)) * in.y +
    (axis.z*axis.x*(1.0-cos(theta)) + axis.y * sin(theta)) * in.z;
  return_vector.y = 
    (axis.x*axis.y*(1.0-cos(theta)) + axis.z * sin(theta)) * in.x +
    (pow(axis.y,2.0) + cos(theta) * (1.0 - pow(axis.y,2.0))) * in.y +
    (axis.y*axis.z*(1.0-cos(theta)) - axis.x * sin(theta)) * in.z;
  return_vector.z = 
    (axis.z*axis.x*(1.0-cos(theta)) - axis.y * sin(theta)) * in.x +
    (axis.y*axis.z*(1.0-cos(theta)) + axis.x * sin(theta)) * in.y +
    (pow(axis.z,2.0) + cos(theta) * (1.0 - pow(axis.z,2.0))) * in.z;

  return return_vector;
}

/* rotate the entire coordinate frame on the given axis */
void realspace_rotate_on_axis(realspace_t * rs,
			      const realpoint_t axis,
			      const floatpoint_t theta) {
  axis_t i_axis;
  realpoint_t new_axis[NUM_AXIS];

  for (i_axis=0;i_axis<NUM_AXIS;i_axis++)
    new_axis[i_axis] = realpoint_rotate_on_axis(rs_specific_axis(rs, i_axis), axis, theta);

  /* make sure the result is orthonomal */
  make_orthonormal(new_axis);

  /* and setup the returned coordinate frame */
  rs_set_offset(rs, rs_offset(rs));
  rs_set_axis(rs, new_axis);

  return;
}



/* returns an axis vector which corresponds to the orthogonal axis (specified
   by ax) in the given view (i.e. coronal, sagittal, etc.) given the current
   axis */
realpoint_t rs_get_orthogonal_view_axis(const realspace_t * rs,
					const view_t view,
					const layout_t layout,
					const axis_t ax) {
  
  switch(view) {
  case CORONAL:
    switch (ax) {
    case XAXIS:
      return rs->axis[XAXIS];
      break;
    case YAXIS:
      return rp_neg(rs->axis[ZAXIS]);
      break;
    case ZAXIS:
    default:
      return rs->axis[YAXIS];
      break;
    }
    break;
  case SAGITTAL:
    switch (ax) {
    case XAXIS:
      if (layout == ORTHOGONAL_LAYOUT)
	return rs->axis[ZAXIS];
      else /* LINEAR_LAYOUT */
	return rs->axis[YAXIS];
      break;
    case YAXIS:
      if (layout == ORTHOGONAL_LAYOUT)
	return rs->axis[YAXIS];
      else /* LINEAR_LAYOUT */
	return rp_neg(rs->axis[ZAXIS]);
      break;
    case ZAXIS:
    default:
      return rs->axis[XAXIS];
      break;
    }
  case TRANSVERSE:
  default:
    switch (ax) {
    case XAXIS:
      return rs->axis[XAXIS];
      break;
    case YAXIS:
      return rs->axis[YAXIS];
      break;
    case ZAXIS:
    default:
      return rs->axis[ZAXIS];
      break;
    }
    break;
  }
}

/* returns the normal axis vector for the given view */
realpoint_t rs_get_view_normal(const realspace_t * rs,
			       const view_t view) {

  /* don't need layout here, as the ZAXIS isn't determined by the layout */
  return rs_get_orthogonal_view_axis(rs, view, LINEAR_LAYOUT, ZAXIS);
}

/* given a coordinate frame, and a view, and the layout, return the appropriate coordinate frame */
realspace_t * realspace_get_view_coord_frame(const realspace_t * in_coord_frame,
					     const view_t view,
					     const layout_t layout) {

  realspace_t * return_coord_frame;
  axis_t i_axis;

  return_coord_frame = rs_init();

  for (i_axis=0;i_axis<NUM_AXIS;i_axis++)
    return_coord_frame->axis[i_axis] = 
      rs_get_orthogonal_view_axis(in_coord_frame,view,layout,i_axis);

  /* the offset of the coord frame stays the same */
  rs_set_offset(return_coord_frame, rs_offset(in_coord_frame));

  return return_coord_frame;
}

/* convert a point in an alternative coordinate frame to the base
   coordinate frame */
inline realpoint_t realspace_alt_coord_to_base(const realpoint_t in,
					       const realspace_t * in_alt_coord_frame) {

  realpoint_t return_point;

  return_point.x = 
    in.x * in_alt_coord_frame->axis[XAXIS].x +
    in.y * in_alt_coord_frame->axis[YAXIS].x +
    in.z * in_alt_coord_frame->axis[ZAXIS].x +
    in_alt_coord_frame->offset.x;
  return_point.y = 
    in.x * in_alt_coord_frame->axis[XAXIS].y +
    in.y * in_alt_coord_frame->axis[YAXIS].y +
    in.z * in_alt_coord_frame->axis[ZAXIS].y +
    in_alt_coord_frame->offset.y;
  return_point.z = 
    in.x * in_alt_coord_frame->axis[XAXIS].z +
    in.y * in_alt_coord_frame->axis[YAXIS].z +
    in.z * in_alt_coord_frame->axis[ZAXIS].z +
    in_alt_coord_frame->offset.z;

  return return_point;
}

/* convert a point from the base coordinates to the given alternative coordinate frame */
inline realpoint_t realspace_base_coord_to_alt(realpoint_t in,
					       const realspace_t * out_alt_coord_frame) {
  realpoint_t return_point;

  /* compensate the inpoint for the offset of the new coordinate frame */
  REALPOINT_SUB(in, rs_offset(out_alt_coord_frame), in); 

  /* instead of multiplying by inv(A), we multiple by transpose(A),
     this is the same thing, as A is orthogonal */
  return_point.x = 
    in.x * out_alt_coord_frame->axis[XAXIS].x +
    in.y * out_alt_coord_frame->axis[XAXIS].y +
    in.z * out_alt_coord_frame->axis[XAXIS].z;
  return_point.y = 
    in.x * out_alt_coord_frame->axis[YAXIS].x +
    in.y * out_alt_coord_frame->axis[YAXIS].y +
    in.z * out_alt_coord_frame->axis[YAXIS].z;
  return_point.z = 
    in.x * out_alt_coord_frame->axis[ZAXIS].x +
    in.y * out_alt_coord_frame->axis[ZAXIS].y +
    in.z * out_alt_coord_frame->axis[ZAXIS].z;

  return return_point;
}

/* converts a "dimensional" quantity (i.e. the size of a voxel) from a
   given coordinate system to the base coordinate system */
inline realpoint_t realspace_alt_dim_to_base(const realpoint_t in,
					     const realspace_t * in_alt_coord_frame) {

  realpoint_t return_point;
  realpoint_t temp[NUM_AXIS];
  realpoint_t temp_rp;

  /* all the fabs/rp_abs are 'cause dim's should always be positive */
  temp_rp.x = fabs(in.x); 
  temp_rp.y = temp_rp.z = 0.0;
  temp[XAXIS] = rp_abs(rp_sub(realspace_alt_coord_to_base(temp_rp, in_alt_coord_frame),
			      in_alt_coord_frame->offset));
  temp_rp.y = fabs(in.y); 
  temp_rp.x = temp_rp.z = 0.0;
  temp[YAXIS] = rp_abs(rp_sub(realspace_alt_coord_to_base(temp_rp, in_alt_coord_frame),
			      in_alt_coord_frame->offset));
  temp_rp.z = fabs(in.z); 
  temp_rp.x = temp_rp.y = 0.0;
  temp[ZAXIS] = rp_abs(rp_sub(realspace_alt_coord_to_base(temp_rp, in_alt_coord_frame),
			      in_alt_coord_frame->offset));

  return_point.x = temp[XAXIS].x+temp[YAXIS].x+temp[ZAXIS].x;
  return_point.y = temp[XAXIS].y+temp[YAXIS].y+temp[ZAXIS].y;
  return_point.z = temp[XAXIS].z+temp[YAXIS].z+temp[ZAXIS].z;

  return return_point;
}

/* converts a "dimensional" quantity (i.e. the size of a voxel) from the
   base coordinate system to a given coordinate system */
inline realpoint_t realspace_base_dim_to_alt(const realpoint_t in,
					     const realspace_t * out_alt_coord_frame) {

  realpoint_t return_point;
  realpoint_t temp[NUM_AXIS];
  realpoint_t temp_rp;
  
  /* all the fabs/rp_abs are 'cause dim's should always be positive */
  temp_rp.x = fabs(in.x); 
  temp_rp.y = temp_rp.z = 0.0;
  temp[XAXIS]=rp_abs(realspace_base_coord_to_alt(rp_add(temp_rp, out_alt_coord_frame->offset),
						 out_alt_coord_frame));
  temp_rp.y = fabs(in.y); 
  temp_rp.x = temp_rp.z = 0.0;
  temp[YAXIS]=rp_abs(realspace_base_coord_to_alt(rp_add(temp_rp, out_alt_coord_frame->offset),
						 out_alt_coord_frame));
  temp_rp.z = fabs(in.z); 
  temp_rp.x = temp_rp.y = 0.0;
  temp[ZAXIS]=rp_abs(realspace_base_coord_to_alt(rp_add(temp_rp, out_alt_coord_frame->offset),
						 out_alt_coord_frame));

  return_point.x = temp[XAXIS].x+temp[YAXIS].x+temp[ZAXIS].x;
  return_point.y = temp[XAXIS].y+temp[YAXIS].y+temp[ZAXIS].y;
  return_point.z = temp[XAXIS].z+temp[YAXIS].z+temp[ZAXIS].z;

  return return_point;
}


/* converts a "dimensional" quantity (i.e. the size of a voxel) from a given
   coordinate system to another coordinate system */
inline realpoint_t realspace_alt_dim_to_alt(const realpoint_t in,
					    const realspace_t * in_alt_coord_frame,
					    const realspace_t * out_alt_coord_frame) {

  realpoint_t return_point;
  
  return_point = realspace_alt_dim_to_base(in,
					   in_alt_coord_frame);
  return_point = realspace_base_dim_to_alt(return_point,
					   out_alt_coord_frame);
  
  return return_point;
}














