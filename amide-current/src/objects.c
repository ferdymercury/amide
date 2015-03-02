/* objects.c - functions used for defining objects in 3D volumes
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
#include "color_table.h"
#include "volume.h"

/* this function places a 3D ellipse on a volume 
 
   notes: 

   This function could be speeded up by moving some of the tests done
   for each voxel out of the inner loop so that they are only done for
   voxels which could possibly be on the surface... eh, fast enough
   for now.

   center is wrt the volume's coord frame
   radius is wrt to the object_coord_frame
   currently, the offset of object_coord_frame is ignored

*/


void objects_place_ellipsoid(volume_t * volume, guint frame, realspace_t in_coord_frame,
			     realpoint_t center, realpoint_t radius, volume_data_t level) {

  voxelpoint_t i, j, low, high;
  realpoint_t real_p, temp;
  volume_data_t weight;
  floatpoint_t min_dim;
  realpoint_t object_corner[2];
  realpoint_t volume_corner[2];
  guint corner;
  realspace_t object_coord_frame;


  /* figure out a reasonable coord frame for the object 
     the object is centered in the coord frame */
  object_coord_frame = in_coord_frame;
  temp = realspace_alt_coord_to_alt(center, volume->coord_frame, object_coord_frame);
  REALSPACE_SUB(temp, radius, temp);
  temp = realspace_alt_coord_to_base(temp, object_coord_frame);
  object_coord_frame.offset = temp;
  
  min_dim = REALPOINT_MIN_DIM(volume->voxel_size);

  object_corner[0].x = -min_dim;
  object_corner[0].y = -min_dim;
  object_corner[0].z = -min_dim;

  object_corner[1].x = 2.0 * radius.x + min_dim;
  object_corner[1].y = 2.0 * radius.y + min_dim;
  object_corner[1].z = 2.0 * radius.z + min_dim;

  /* get center wrt the object coord frame */
  //  REALSPACE_CMULT(0.5, object_corner[1], center);
  center = radius;
  
  realspace_get_enclosing_corners(object_coord_frame, object_corner,
				  volume->coord_frame, volume_corner);
				  

  /* figure out the limits on x, y, and z that we need to worry about */
  if (volume_corner[0].x < 0.0)
    volume_corner[0].x = 0.0;
  if (volume_corner[1].x < 0.0)
    volume_corner[1].x = 0.0;
  if (volume_corner[0].y < 0.0)
    volume_corner[0].y = 0.0;
  if (volume_corner[1].y < 0.0)
    volume_corner[1].y = 0.0;
  if (volume_corner[0].z < 0.0)
    volume_corner[0].z = 0.0;
  if (volume_corner[1].z < 0.0)
    volume_corner[1].z = 0.0;

  if (volume_corner[0].x > volume->corner.x)
    volume_corner[0].x = volume->corner.x;
  if (volume_corner[1].x > volume->corner.x)
    volume_corner[1].x = volume->corner.x;
  if (volume_corner[0].y > volume->corner.y)
    volume_corner[0].y = volume->corner.y;
  if (volume_corner[1].y > volume->corner.y)
    volume_corner[1].y = volume->corner.y;
  if (volume_corner[0].z > volume->corner.z)
    volume_corner[0].z = volume->corner.z;
  if (volume_corner[1].z > volume->corner.z)
    volume_corner[1].z = volume->corner.z;

  low = volume_real_to_voxel(volume, volume_corner[0]);
  high = volume_real_to_voxel(volume, volume_corner[1]);

  /* iterate around the dimensions */
  for (i.x = low.x; i.x < high.x ; i.x++) 
    for (i.y = low.y; i.y < high.y ; i.y++) 
      for (i.z = low.z; i.z < high.z ; i.z++) {

	/* iterative over the eight corners */
	weight = 0.0;
	for (corner=0; corner<8; corner++) {
	  j.x = (corner & 0x1) ? i.x+1 : i.x;
	  j.y = (corner & 0x2) ? i.y+1 : i.y;
	  j.z = (corner & 0x4) ? i.z+1 : i.z;
	  real_p = volume_voxel_to_real(volume,j);
	  real_p = realspace_alt_coord_to_alt(real_p, volume->coord_frame, object_coord_frame);
	  if (realpoint_in_ellipsoid(real_p,center,radius)) {
	    weight += 0.125;
	  }
	}
	VOLUME_SET_CONTENT(volume,frame,i) += weight*level;
      }

  return;
}



/* this function places a cylinder on a volume */
void objects_place_elliptic_cylinder(volume_t * volume, guint frame, realspace_t in_coord_frame,
				     realpoint_t center, realpoint_t radius, floatpoint_t height,
				     volume_data_t level) {

  voxelpoint_t i, j, low, high;
  volume_data_t weight;
  floatpoint_t min_dim;
  realpoint_t real_p, temp;
  realpoint_t object_corner[2];
  realpoint_t volume_corner[2];
  guint corner;
  realspace_t object_coord_frame;


  /* figure out a reasonable coord frame for the object 
     the object is centered in the coord frame */
  object_coord_frame = in_coord_frame;
  temp = realspace_alt_coord_to_alt(center, volume->coord_frame, object_coord_frame);
  temp.x = temp.x-radius.x;
  temp.y = temp.y-radius.y;
  temp.z = temp.z-height/2.0;
  temp = realspace_alt_coord_to_base(temp, object_coord_frame);
  object_coord_frame.offset = temp;
  
  min_dim = REALPOINT_MIN_DIM(volume->voxel_size);

  object_corner[0].x = -min_dim;
  object_corner[0].y = -min_dim;
  object_corner[0].z = -min_dim;

  object_corner[1].x = 2.0 * radius.x + min_dim;
  object_corner[1].y = 2.0 * radius.y + min_dim;
  object_corner[1].z = height + min_dim;

  /* get the center of the object wrt object coord frame */
  //  REALSPACE_CMULT(0.5, object_corner[1], center);
  center.x = radius.x;
  center.y = radius.y;
  center.z = height/2.0;


  realspace_get_enclosing_corners(object_coord_frame, object_corner,
				  volume->coord_frame, volume_corner);
				  

  /* figure out the limits on x, y, and z that we need to worry about */
  if (volume_corner[0].x < 0.0)
    volume_corner[0].x = 0.0;
  if (volume_corner[1].x < 0.0)
    volume_corner[1].x = 0.0;
  if (volume_corner[0].y < 0.0)
    volume_corner[0].y = 0.0;
  if (volume_corner[1].y < 0.0)
    volume_corner[1].y = 0.0;
  if (volume_corner[0].z < 0.0)
    volume_corner[0].z = 0.0;
  if (volume_corner[1].z < 0.0)
    volume_corner[1].z = 0.0;

  if (volume_corner[0].x > volume->corner.x)
    volume_corner[0].x = volume->corner.x;
  if (volume_corner[1].x > volume->corner.x)
    volume_corner[1].x = volume->corner.x;
  if (volume_corner[0].y > volume->corner.y)
    volume_corner[0].y = volume->corner.y;
  if (volume_corner[1].y > volume->corner.y)
    volume_corner[1].y = volume->corner.y;
  if (volume_corner[0].z > volume->corner.z)
    volume_corner[0].z = volume->corner.z;
  if (volume_corner[1].z > volume->corner.z)
    volume_corner[1].z = volume->corner.z;

  low = volume_real_to_voxel(volume, volume_corner[0]);
  high = volume_real_to_voxel(volume, volume_corner[1]);

  /* iterate around the dimensions */
  for (i.x = low.x; i.x < high.x ; i.x++) 
    for (i.y = low.y; i.y < high.y ; i.y++) 
      for (i.z = low.z; i.z < high.z ; i.z++) {

	/* iterative over the eight corners */
	weight = 0.0;
	for (corner=0; corner<8; corner++) {
	  j.x = (corner & 0x1) ? i.x+1 : i.x;
	  j.y = (corner & 0x2) ? i.y+1 : i.y;
	  j.z = (corner & 0x4) ? i.z+1 : i.z;
	  real_p = volume_voxel_to_real(volume,j);
	  real_p = realspace_alt_coord_to_alt(real_p, volume->coord_frame, object_coord_frame);
	  if (realpoint_in_elliptic_cylinder(real_p,center,height, radius))
	    weight += 0.125;
	}
	VOLUME_SET_CONTENT(volume,frame,i) += weight*level;
      }

  return;
}

