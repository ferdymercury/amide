/* objects.h - functions used for defining objects in 3D volumes
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

#ifndef __OBJECTS_H__
#define __OBJECTS_H__


/* functions */
void objects_place_ellipsoid(volume_t * volume, guint frame, realspace_t in_coord_frame,
			     realpoint_t center, realpoint_t radius, volume_data_t level);
void objects_place_elliptic_cylinder(volume_t * volume, guint frame, realspace_t object_coord_frame,
				     realpoint_t center, realpoint_t radius, floatpoint_t height,
				     volume_data_t level);

#endif /* __OBJECTS_H__ */

