/* amitk_volume.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2014 Andy Loening
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

#ifndef __AMITK_VOLUME_H__
#define __AMITK_VOLUME_H__


#include "amitk_object.h"

G_BEGIN_DECLS

#define	AMITK_TYPE_VOLUME		(amitk_volume_get_type ())
#define AMITK_VOLUME(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), AMITK_TYPE_VOLUME, AmitkVolume))
#define AMITK_VOLUME_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_VOLUME, AmitkVolumeClass))
#define AMITK_IS_VOLUME(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), AMITK_TYPE_VOLUME))
#define AMITK_IS_VOLUME_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_VOLUME))
#define	AMITK_VOLUME_GET_CLASS(object)	(G_TYPE_CHECK_GET_CLASS ((object), AMITK_TYPE_VOLUME, AmitkVolumeClass))

#define AMITK_VOLUME_CORNER(vol)        (AMITK_VOLUME(vol)->corner)
#define AMITK_VOLUME_X_CORNER(vol)      (AMITK_VOLUME(vol)->corner.x)
#define AMITK_VOLUME_Y_CORNER(vol)      (AMITK_VOLUME(vol)->corner.y)
#define AMITK_VOLUME_Z_CORNER(vol)      (AMITK_VOLUME(vol)->corner.z)
#define AMITK_VOLUME_VALID(vol)         (AMITK_VOLUME(vol)->valid)


typedef struct AmitkVolumeClass AmitkVolumeClass;
typedef struct AmitkVolume AmitkVolume;


struct AmitkVolume
{
  AmitkObject parent;
  AmitkPoint corner; /* far corner, in volume's coord space. near corner always 0,0,0 in volume's coord space */
  gboolean valid;         /* if the corner is currently valid */

};

struct AmitkVolumeClass
{
  AmitkObjectClass parent_class;

  void (*volume_corner_changed)         (AmitkVolume * volume,
					 AmitkPoint * new_corner);
  void (* volume_get_center)            (const AmitkVolume * volume,
					 AmitkPoint * center);
  void (* volume_changed)               (AmitkVolume * volume);

};



/* Application-level methods */

GType	        amitk_volume_get_type	             (void);
AmitkVolume *   amitk_volume_new                     (void);
gboolean        amitk_volume_point_in_bounds         (const AmitkVolume * volume,
						      const AmitkPoint point);
AmitkPoint      amitk_volume_place_in_bounds         (const AmitkVolume * volume,
						      const AmitkPoint point);
AmitkPoint      amitk_volume_get_center              (const AmitkVolume * volume);
void            amitk_volume_set_corner              (AmitkVolume * volume, 
						      AmitkPoint corner);
void            amitk_volume_set_z_corner            (AmitkVolume * volume, 
						      amide_real_t z);
void            amitk_volume_set_center              (AmitkVolume * volume,
						      const AmitkPoint center);
void            amitk_volume_get_enclosing_corners   (const AmitkVolume * volume,
						      const AmitkSpace * space,
						      AmitkCorners return_corners);
gboolean       amitk_volumes_get_enclosing_corners   (const GList * volumes,
						      const AmitkSpace * space,
						      AmitkCorners return_corners);
gboolean       amitk_volume_volume_intersection_corners(const AmitkVolume * volume1,
							const AmitkVolume * volume2,
							AmitkCorners return_corners);
amide_real_t   amitk_volumes_get_max_size             (GList * objects);
gboolean       amitk_volumes_calc_display_volume      (const GList * volumes, 
						       const AmitkSpace * space, 
						       const AmitkPoint view_center,
						       const amide_real_t thickness,
						       const amide_real_t fov,
						       AmitkVolume * volume);


G_END_DECLS

#endif /* __AMITK_VOLUME_H__ */
