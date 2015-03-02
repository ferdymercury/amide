/* realspace.h
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

typedef enum {XAXIS, YAXIS, ZAXIS, NUM_AXIS} axis_t;

/* setup the types for various internal data formats */
typedef gdouble floatpoint_t;
typedef gint16 intpoint_t;

/* pixel point is a point in pixel (integer) 2D space */
typedef struct pixelpoint_t {
  intpoint_t x;
  intpoint_t y;
} pixelpoint_t;

/* voxel point is a point in voxel (integer) 3D space */
typedef struct voxelpoint_t {
  intpoint_t x;
  intpoint_t y;
  intpoint_t z;
} voxelpoint_t;

/* realpoint is a point in real (float) 3D space */
typedef struct realpoint_t {
  floatpoint_t x;
  floatpoint_t y;
  floatpoint_t z;
} realpoint_t;

/* a realspace is the description of an alternative coordinate frame 
   wrt to the base coordinate space.  note: offset is in the base
   coordinate frame*/
typedef struct realspace_t {
  realpoint_t offset;
  realpoint_t axis[NUM_AXIS];
} realspace_t;

/* constants */
#define CLOSE 0.00001 /* what's close enough to be equal.... */
#define SMALL 0.01 /* in milimeter's, used as a lower limit on some dimensions */

/* convenience functions */

/* returns the boolean value of fp1==fp2 (within a factor of CLOSE) */
#define FLOATPOINT_EQUAL(fp1,fp2) (((fp1) > ((1.00-CLOSE)*(fp2)-CLOSE)) && \
				   ((fp1) < ((1.00+CLOSE)*(fp2)+CLOSE)))

/* returns the boolean value of rp1==rp2 (within a factor of CLOSE) */
#define REALPOINT_EQUAL(rp1,rp2) (FLOATPOINT_EQUAL((rp1.x),(rp2.x)) && \
				  FLOATPOINT_EQUAL((rp1.y),(rp2.y)) && \
				  FLOATPOINT_EQUAL((rp1.z),(rp2.z)))

#define REALPOINT_MAGNITUDE(rp) (sqrt(pow((rp).x,2.0) + pow((rp).y,2.0) + pow((rp).z,2.0)))
				  
/* returns the minimum dimension of the "box" defined by rp1*/
#define REALPOINT_MIN_DIM(rp1) (MIN( MIN((rp1).x, (rp1).y), (rp1).z))

/* returns the maximum dimension of the "box" defined by rp1 */
#define REALPOINT_MAX_DIM(rp1) (REALPOINT_MAGNITUDE(rp1))

/* returned the maximum of rp1 */
#define REALPOINT_MAX(rp1) (MAX( MAX((rp1).x, (rp1).y), (rp1).z))

/* returns rp1 dot rp2" */
#define REALSPACE_DOT_PRODUCT(rp1,rp2) ((rp1).x*(rp2).x+(rp1).y*(rp2).y+(rp1).z*(rp2).z)

/* returns rp2 = abs(rp1)" */
#define REALSPACE_ABS(rp1,rp2) ((rp2).x = fabs((rp1).x), \
				(rp2).y = fabs((rp1).y), \
				(rp2).z = fabs((rp1).z)) 

/* does rp3=rp1+rp2 for realpoint structures */
#define REALSPACE_ADD(rp1,rp2,rp3) (((rp3).x = (rp1).x+(rp2).x), \
				    ((rp3).y = (rp1).y+(rp2).y), \
		                    ((rp3).z = (rp1).z+(rp2).z))
/* does rp3=rp1-rp2 for realpoint structures */
#define REALSPACE_SUB(rp1,rp2,rp3) (((rp3).x = (rp1).x-(rp2).x), \
				    ((rp3).y = (rp1).y-(rp2).y), \
		                    ((rp3).z = (rp1).z-(rp2).z))

/* does rp3=rp1.*rp2 for realpoint structures */
#define REALSPACE_MULT(rp1,rp2,rp3) (((rp3).x = (rp1).x*(rp2).x), \
				     ((rp3).y = (rp1).y*(rp2).y), \
		                     ((rp3).z = (rp1).z*(rp2).z))

/* does rp3=fabs(rp1-rp2) for realpoint structures */
#define REALSPACE_DIFF(rp1,rp2,rp3) (((rp3).x = fabs((rp1).x-(rp2).x)), \
				     ((rp3).y = fabs((rp1).y-(rp2).y)), \
		                     ((rp3).z = fabs((rp1).z-(rp2).z)))

/* does rp3=cm*rp1 for realpoint structures */
#define REALSPACE_CMULT(cm,rp1,rp3) (((rp3).x = (cm)*(rp1).x), \
				    ((rp3).y = (cm)*(rp1).y), \
				    ((rp3).z = (cm)*(rp1).z))

/* does rp3=cm*rp1+dm*rp2 for realpoint structures */
#define REALSPACE_MADD(cm,rp1,dm,rp2,rp3) (((rp3).x = cm*(rp1).x+dm*(rp2).x), \
					   ((rp3).y = cm*(rp1).y+dm*(rp2).y), \
					   ((rp3).z = cm*(rp1).z+dm*(rp2).z)) 


/* external functions */
void realspace_make_orthogonal(realpoint_t axis[]);
void realspace_make_orthonormal(realpoint_t axis[]);
realpoint_t realspace_rotate_on_axis(const realpoint_t * in,
				     const realpoint_t * axis,
				     const floatpoint_t theta);
realpoint_t realspace_get_orthogonal_normal(const realpoint_t axis[],
					    const view_t i);
realpoint_t realspace_get_orthogonal_axis(const realpoint_t axis[],
					  const view_t view,
					  const axis_t ax);
realspace_t realspace_get_orthogonal_coord_frame(const realspace_t in_coord_frame,
						 const view_t view);
realpoint_t realspace_alt_coord_to_base(const realpoint_t in,
					const realspace_t in_alt_coord_frame);
realpoint_t realspace_base_coord_to_alt(realpoint_t in,
					const realspace_t out_alt_coord_frame);
realpoint_t realspace_alt_coord_to_alt(const realpoint_t in,
				       const realspace_t in_alt_coord_frame,
				       const realspace_t out_alt_coord_frame);
realpoint_t realspace_alt_dim_to_base(const realpoint_t in,
				      const realspace_t in_alt_coord_frame);
realpoint_t realspace_base_dim_to_alt(const realpoint_t in,
				      const realspace_t out_alt_coord_frame);
realpoint_t realspace_alt_dim_to_alt(const realpoint_t in,
				     const realspace_t in_alt_coord_frame,
				     const realspace_t out_alt_coord_frame);

/* external variables */
extern const gchar * axis_names[];
extern const realpoint_t default_normal[NUM_VIEWS];
extern const realpoint_t default_axis[NUM_VIEWS];
extern const realpoint_t default_rotation[NUM_VIEWS];
extern const realpoint_t realpoint_init;
extern const voxelpoint_t voxelpoint_init;









