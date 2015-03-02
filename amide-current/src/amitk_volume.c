/* amitk_volume.c
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

#include "amide_config.h"

#include "amitk_volume.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"



enum {
  VOLUME_CORNER_CHANGED,
  VOLUME_CHANGED,
  LAST_SIGNAL
};

static void          volume_class_init       (AmitkVolumeClass *klass);
static void          volume_init             (AmitkVolume      *volume);
static void          volume_corner_changed   (AmitkVolume      *volume,
					      AmitkPoint       *new_corner);
static AmitkObject * volume_copy             (const AmitkObject * object);
static void          volume_copy_in_place    (AmitkObject * dest_object, const AmitkObject * src_object);
static void          volume_write_xml        (const AmitkObject * object, xmlNodePtr nodes);
static void          volume_read_xml         (AmitkObject * object, xmlNodePtr nodes);


static AmitkObject* parent_class;
static guint        volume_signals[LAST_SIGNAL];



GType amitk_volume_get_type(void) {

  static GType volume_type = 0;

  if (!volume_type)
    {
      static const GTypeInfo volume_info =
      {
	sizeof (AmitkVolumeClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) volume_class_init,
	(GClassFinalizeFunc) NULL,
	NULL,		/* class_data */
	sizeof (AmitkVolume),
	0,			/* n_preallocs */
	(GInstanceInitFunc) volume_init,
	NULL /* value table */
      };
      
      volume_type = g_type_register_static (AMITK_TYPE_OBJECT, "AmitkVolume", &volume_info, 0);
    }

  return volume_type;
}


static void volume_class_init (AmitkVolumeClass * class) {

  //  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  AmitkObjectClass * object_class = AMITK_OBJECT_CLASS(class);

  parent_class = g_type_class_peek_parent(class);
  
  class->volume_corner_changed = volume_corner_changed;

  object_class->object_copy = volume_copy;
  object_class->object_copy_in_place = volume_copy_in_place;
  object_class->object_write_xml = volume_write_xml;
  object_class->object_read_xml = volume_read_xml;

  volume_signals[VOLUME_CORNER_CHANGED] =
    g_signal_new ("volume_corner_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkVolumeClass, volume_corner_changed),
		  NULL, NULL, amitk_marshal_NONE__BOXED,
		  G_TYPE_NONE,1,
		  AMITK_TYPE_POINT);
  volume_signals[VOLUME_CHANGED] =
    g_signal_new ("volume_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkVolumeClass, volume_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);


}


static void volume_init (AmitkVolume * volume) {

  volume->corner = zero_point;
  volume->valid = FALSE;

  return;
}


static void volume_corner_changed(AmitkVolume *volume, AmitkPoint * new_corner) {

  volume->corner = *new_corner;
  volume->valid = TRUE;

  return;
}

static AmitkObject * volume_copy(const AmitkObject * object) {

  AmitkVolume * copy;

  g_return_val_if_fail(AMITK_IS_VOLUME(object), NULL);

  copy = amitk_volume_new();

  amitk_object_copy_in_place(AMITK_OBJECT(copy), object);

  return AMITK_OBJECT(copy);

}

static void volume_copy_in_place(AmitkObject * dest_object, const AmitkObject * src_object) {

  g_return_if_fail(AMITK_IS_VOLUME(src_object));
  g_return_if_fail(AMITK_IS_VOLUME(dest_object));

  amitk_volume_set_corner(AMITK_VOLUME(dest_object), AMITK_VOLUME_CORNER(src_object));

  AMITK_OBJECT_CLASS (parent_class)->object_copy_in_place (dest_object, src_object);
}




static void volume_write_xml(const AmitkObject * object, xmlNodePtr nodes) {

  AMITK_OBJECT_CLASS(parent_class)->object_write_xml(object, nodes);

  amitk_point_write_xml(nodes, "corner", AMITK_VOLUME_CORNER(object));

  return;
}

static void volume_read_xml(AmitkObject * object, xmlNodePtr nodes) {

  AMITK_OBJECT_CLASS(parent_class)->object_read_xml(object, nodes);
  amitk_volume_set_corner(AMITK_VOLUME(object), amitk_point_read_xml(nodes, "corner"));

  return;
}

AmitkVolume * amitk_volume_new (void) {

  AmitkVolume * volume;

  volume = g_object_new(amitk_volume_get_type(), NULL);

  return volume;
}



AmitkPoint amitk_volume_center(const AmitkVolume * volume) {

  AmitkPoint center;
  AmitkCorners corners;

  g_return_val_if_fail(AMITK_IS_VOLUME(volume), zero_point);
  g_return_val_if_fail(AMITK_VOLUME_VALID(volume), zero_point);

  /* get the far corner, corner[0] is currently always zero */
  corners[0] = amitk_space_b2s(AMITK_SPACE(volume), 
			      AMITK_SPACE_OFFSET(volume));  
  corners[1] = volume->corner;
 
  /* the center in roi coords is then just half the far corner */
  center = point_add(point_cmult(0.5,corners[1]), point_cmult(0.5,corners[0]));

  /* now, translate that into the base coord frame */
  center = amitk_space_s2b(AMITK_SPACE(volume), center);

  return center;
}

void amitk_volume_set_corner(AmitkVolume * volume, AmitkPoint corner) {

  g_return_if_fail(AMITK_IS_VOLUME(volume));

  if (!POINT_EQUAL(AMITK_VOLUME_CORNER(volume), corner)) {
    g_signal_emit(G_OBJECT(volume), volume_signals[VOLUME_CORNER_CHANGED], 0, &corner);
    g_signal_emit(G_OBJECT(volume), volume_signals[VOLUME_CHANGED], 0);
  }
}

void amitk_volume_set_z_corner(AmitkVolume * volume, amide_real_t z) {
  
  AmitkPoint corner;

  g_return_if_fail(AMITK_IS_VOLUME(volume));
  g_return_if_fail(AMITK_VOLUME_VALID(volume));

  if (!REAL_EQUAL(AMITK_VOLUME_Z_CORNER(volume), z)) {
    corner = AMITK_VOLUME_CORNER(volume);
    corner.z = z;
    g_signal_emit(G_OBJECT(volume), volume_signals[VOLUME_CORNER_CHANGED], 0, &corner);
    g_signal_emit(G_OBJECT(volume), volume_signals[VOLUME_CHANGED], 0);
  }
}

/* center should be in the base coordinate frame */
void amitk_volume_set_center(AmitkVolume * volume, const AmitkPoint new_center) {

  AmitkPoint shift;
  AmitkPoint new_offset;

  g_return_if_fail(AMITK_IS_VOLUME(volume));
  g_return_if_fail(AMITK_VOLUME_VALID(volume));
  
  shift = point_sub(new_center, amitk_volume_center(volume));
  if (!POINT_EQUAL(shift, zero_point)) {
    new_offset = point_add(AMITK_SPACE_OFFSET(volume), shift);
    amitk_space_set_offset(AMITK_SPACE(volume), new_offset);
  }
  return;
}

/* takes a volume and a coordinate space, and give the corners of a box with
   sides orthogonal to the given space that totally encompasses the volume */
void amitk_volume_get_enclosing_corners(const AmitkVolume * volume,
					const AmitkSpace * space,
					AmitkCorners return_corners) {

  AmitkCorners corners;

  g_return_if_fail(AMITK_IS_VOLUME(volume));
  g_return_if_fail(AMITK_IS_SPACE(space));
  g_return_if_fail(AMITK_VOLUME_VALID(volume));

  corners[0] = amitk_space_b2s(AMITK_SPACE(volume), AMITK_SPACE_OFFSET(volume));
  corners[1] = volume->corner;

  /* look at all eight corners of our cube, figure out the min and max coords */
  amitk_space_get_enclosing_corners(AMITK_SPACE(volume), corners, space, return_corners);

  return;
}


/* takes a list of volumes and a view coordinate space, give the corners
   necessary to totally encompass the volumes in the given space */
void amitk_volumes_get_enclosing_corners(GList * volumes,
					 const AmitkSpace * space,
					 AmitkCorners return_corners) {

  AmitkCorners temp_corners;
  gboolean valid=FALSE;

  while (volumes != NULL) {

    if (AMITK_IS_VOLUME(volumes->data)) {
      if (AMITK_VOLUME_VALID(volumes->data)) {
	amitk_volume_get_enclosing_corners(AMITK_VOLUME(volumes->data),space,temp_corners);
	if (!valid) {
	  valid = TRUE;
	  return_corners[0] = temp_corners[0];
	  return_corners[1] = temp_corners[1];
	} else {
	  return_corners[0].x = (return_corners[0].x < temp_corners[0].x) ? return_corners[0].x : temp_corners[0].x;
	  return_corners[0].y = (return_corners[0].y < temp_corners[0].y) ? return_corners[0].y : temp_corners[0].y;
	  return_corners[0].z = (return_corners[0].z < temp_corners[0].z) ? return_corners[0].z : temp_corners[0].z;
	  return_corners[1].x = (return_corners[1].x > temp_corners[1].x) ? return_corners[1].x : temp_corners[1].x;
	  return_corners[1].y = (return_corners[1].y > temp_corners[1].y) ? return_corners[1].y : temp_corners[1].y;
	  return_corners[1].z = (return_corners[1].z > temp_corners[1].z) ? return_corners[1].z : temp_corners[1].z;
	}
      }
    }
    volumes = volumes->next;
  }

  if (!valid)
    return_corners[0] = return_corners[1] = zero_point;

  return;
}


/* returns the corners of a box (orthogonal to volume1's coordinate space) that 
   completely encloses the intersection of the two volumes.  Corner points
   are in the space of volume 1.  Returns FALSE if no intersection.
 */
gboolean amitk_volume_volume_intersection_corners(const AmitkVolume * volume1,
					      const AmitkVolume * volume2,
					      AmitkCorners return_corners) {
  
  gint i;

  g_return_val_if_fail(AMITK_IS_VOLUME(volume1), FALSE);
  g_return_val_if_fail(AMITK_IS_VOLUME(volume2), FALSE);
  g_return_val_if_fail(AMITK_VOLUME_VALID(volume1), FALSE);
  g_return_val_if_fail(AMITK_VOLUME_VALID(volume2), FALSE);

  /* get the corners in the volume's space that orthogonally encloses the roi */
  amitk_volume_get_enclosing_corners(volume2, AMITK_SPACE(volume1), return_corners);

  /* and reduce the size of the intersection box */
  for (i=0; i<2; i++) {
    if (return_corners[i].x < 0.0) return_corners[i].x = 0;
    if (return_corners[i].y < 0.0) return_corners[i].y = 0;
    if (return_corners[i].z < 0.0) return_corners[i].z = 0;
    if (return_corners[i].x > volume1->corner.x) return_corners[i].x = volume1->corner.x;
    if (return_corners[i].y > volume1->corner.y) return_corners[i].y = volume1->corner.y;
    if (return_corners[i].z > volume1->corner.z) return_corners[i].z = volume1->corner.z;
  }

  if (REAL_EQUAL(return_corners[0].x, return_corners[1].x) ||
      REAL_EQUAL(return_corners[0].y, return_corners[1].y) ||
      REAL_EQUAL(return_corners[0].z, return_corners[1].z))
    return FALSE;
  else
    return TRUE;
}






/* returns the maximal dimensional size of a list of volumes */
amide_real_t amitk_volumes_get_max_size(GList * objects) {

  amide_real_t temp, max_size;

  if (objects == NULL) return -1.0; /* invalid */

  /* first process the rest of the list */
  max_size = amitk_volumes_get_max_size(objects->next);

  /* now process and compare to the children */
  temp = amitk_volumes_get_max_size(AMITK_OBJECT_CHILDREN(objects->data));
  if (temp > max_size) max_size = temp;

  /* and process this guy */
  if (AMITK_IS_VOLUME(objects->data)) 
    if (AMITK_VOLUME_VALID(objects->data)) {
      temp = point_max_dim(AMITK_VOLUME_CORNER(objects->data));
      if (temp > max_size) max_size = temp;
    }

  return max_size;
}
