/* amitk_volume.c
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

#include "amide_config.h"

#include "amitk_volume.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"



enum {
  VOLUME_CORNER_CHANGED,
  VOLUME_GET_CENTER,
  VOLUME_CHANGED,
  LAST_SIGNAL
};

static void          volume_class_init       (AmitkVolumeClass  *klass);
static void          volume_init             (AmitkVolume       *volume);
static void          volume_corner_changed   (AmitkVolume       *volume,
					      AmitkPoint        *new_corner);
static void          volume_get_center       (const AmitkVolume *volume,
					      AmitkPoint        *center);
static void          volume_scale            (AmitkSpace * space,
					      AmitkPoint * ref_point,
					      AmitkPoint * scaling);
static AmitkObject * volume_copy             (const AmitkObject * object);
static void          volume_copy_in_place    (AmitkObject * dest_object, const AmitkObject * src_object);
static void          volume_write_xml        (const AmitkObject *object, 
					      xmlNodePtr         nodes, 
					      FILE              *study_file);
static gchar *       volume_read_xml         (AmitkObject       *object, 
					      xmlNodePtr         nodes, 
					      FILE              *study_file,
					      gchar             *error_buf);



static AmitkObjectClass* parent_class;
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
  AmitkSpaceClass * space_class = AMITK_SPACE_CLASS(class);

  parent_class = g_type_class_peek_parent(class);
  
  space_class->space_scale = volume_scale;


  object_class->object_copy = volume_copy;
  object_class->object_copy_in_place = volume_copy_in_place;
  object_class->object_write_xml = volume_write_xml;
  object_class->object_read_xml = volume_read_xml;
  
  class->volume_corner_changed = volume_corner_changed;
  class->volume_get_center = volume_get_center;

  volume_signals[VOLUME_CORNER_CHANGED] =
    g_signal_new ("volume_corner_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkVolumeClass, volume_corner_changed),
		  NULL, NULL, amitk_marshal_NONE__BOXED,
		  G_TYPE_NONE,1,
		  AMITK_TYPE_POINT);
  volume_signals[VOLUME_GET_CENTER] = 
    g_signal_new ("volume_get_center",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkVolumeClass, volume_get_center),
		  NULL, NULL, amitk_marshal_NONE__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);
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

static void volume_get_center(const AmitkVolume *volume, AmitkPoint * pcenter) {

  AmitkCorners corners;

  /* get the far corner, corner[0] is currently always zero */
  corners[0] = amitk_space_b2s(AMITK_SPACE(volume), AMITK_SPACE_OFFSET(volume));  
  corners[1] = volume->corner;
 
  /* the center in roi coords is then just half the far corner */
  (*pcenter) = point_add(point_cmult(0.5,corners[1]), point_cmult(0.5,corners[0]));

  /* now, translate that into the base coord frame */
  (*pcenter) = amitk_space_s2b(AMITK_SPACE(volume), (*pcenter));

  return;
}



static void volume_scale(AmitkSpace * space, AmitkPoint * ref_point, AmitkPoint * scaling) {

  AmitkVolume * volume;
  AmitkPoint new_corner;
  AmitkPoint shift;

  g_return_if_fail(AMITK_IS_VOLUME(space));
  volume = AMITK_VOLUME(space);

  /* readjust the far corner */
  /* note that this is an approximate scaling.  Since we preserve angles,
     we can't correctly scale ROI's if the ROI is not orthogonal to the
     space that's initially being scaled.   */
  new_corner = AMITK_VOLUME_CORNER(volume);
  new_corner = amitk_space_s2b(AMITK_SPACE(volume), new_corner);
  shift = point_sub(new_corner, *ref_point);
  shift = point_mult(*scaling, shift);
  new_corner = point_add(*ref_point, shift);


  /* update the space */
  AMITK_SPACE_CLASS(parent_class)->space_scale (space, ref_point, scaling);

  /* calculate the new corner */
  if (AMITK_VOLUME_VALID(volume)) {
    new_corner = amitk_space_b2s(AMITK_SPACE(volume), new_corner);

    g_return_if_fail(new_corner.x >= 0);
    g_return_if_fail(new_corner.y >= 0);
    g_return_if_fail(new_corner.z >= 0);

    amitk_volume_set_corner(volume, new_corner);
  }

}

static AmitkObject * volume_copy(const AmitkObject * object) {

  AmitkVolume * copy;

  g_return_val_if_fail(AMITK_IS_VOLUME(object), NULL);

  copy = amitk_volume_new();

  amitk_object_copy_in_place(AMITK_OBJECT(copy), AMITK_OBJECT(object));

  return AMITK_OBJECT(copy);

}

static void volume_copy_in_place(AmitkObject * dest_object, const AmitkObject * src_object) {

  g_return_if_fail(AMITK_IS_VOLUME(src_object));
  g_return_if_fail(AMITK_IS_VOLUME(dest_object));

  if (AMITK_VOLUME_VALID(src_object))
    amitk_volume_set_corner(AMITK_VOLUME(dest_object), AMITK_VOLUME_CORNER(src_object));
  else
    AMITK_VOLUME(dest_object)->valid = FALSE;

  AMITK_OBJECT_CLASS (parent_class)->object_copy_in_place (dest_object, src_object);
}


static void volume_write_xml(const AmitkObject * object, xmlNodePtr nodes, FILE * study_file) {

  AMITK_OBJECT_CLASS(parent_class)->object_write_xml(object, nodes, study_file);

  amitk_point_write_xml(nodes, "corner", AMITK_VOLUME_CORNER(object));
  xml_save_boolean(nodes, "valid", AMITK_VOLUME_VALID(object));

  return;
}

static gchar * volume_read_xml(AmitkObject * object, xmlNodePtr nodes, FILE * study_file, gchar * error_buf) {

  AmitkVolume * volume;

  error_buf = AMITK_OBJECT_CLASS(parent_class)->object_read_xml(object, nodes, study_file, error_buf);
  
  volume = AMITK_VOLUME(object);

  volume->corner = amitk_point_read_xml(nodes, "corner", &error_buf);
  volume->valid = xml_get_boolean(nodes, "valid", &error_buf);

  /* legacy, "valid" is a new parameter */
  if (!volume->valid)
    if (!POINT_EQUAL(volume->corner, zero_point))
      volume->valid = TRUE;

  return error_buf;
}

AmitkVolume * amitk_volume_new (void) {

  AmitkVolume * volume;

  volume = g_object_new(amitk_volume_get_type(), NULL);

  return volume;
}


/* point should be in the base coordinate frame */
gboolean amitk_volume_point_in_bounds(const AmitkVolume * volume,
				      const AmitkPoint base_point) {

  AmitkPoint point;

  g_return_val_if_fail(AMITK_IS_VOLUME(volume), FALSE);
  g_return_val_if_fail(AMITK_VOLUME_VALID(volume), FALSE);

  point = amitk_space_b2s(AMITK_SPACE(volume), base_point);

  if ((point.x < 0.0) || (point.y < 0.0) || (point.z < 0.0) ||
      (point.x > AMITK_VOLUME_X_CORNER(volume)) ||
      (point.y > AMITK_VOLUME_Y_CORNER(volume)) ||
      (point.z > AMITK_VOLUME_Z_CORNER(volume)))
    return FALSE;
  else
    return TRUE;
}

AmitkPoint amitk_volume_place_in_bounds(const AmitkVolume * volume,
					const AmitkPoint base_point) {

  AmitkPoint point;

  g_return_val_if_fail(AMITK_IS_VOLUME(volume), zero_point);
  g_return_val_if_fail(AMITK_VOLUME_VALID(volume), zero_point);

  point = amitk_space_b2s(AMITK_SPACE(volume), base_point);

  if (point.x < 0.0)
    point.x = 0.0;
  else if (point.x > AMITK_VOLUME_X_CORNER(volume))
    point.x = AMITK_VOLUME_X_CORNER(volume);

  if (point.y < 0.0)
    point.y = 0.0;
  else if (point.y > AMITK_VOLUME_Y_CORNER(volume))
    point.y = AMITK_VOLUME_Y_CORNER(volume);

  if (point.z < 0.0)
    point.z = 0.0;
  else if (point.z > AMITK_VOLUME_Z_CORNER(volume))
    point.z = AMITK_VOLUME_Z_CORNER(volume);

  point = amitk_space_s2b(AMITK_SPACE(volume), point);

  return point;
}




AmitkPoint amitk_volume_get_center(const AmitkVolume * volume) {

  AmitkPoint center;

  g_return_val_if_fail(AMITK_IS_VOLUME(volume), zero_point);
  g_return_val_if_fail(AMITK_VOLUME_VALID(volume), zero_point);

  g_signal_emit(G_OBJECT(volume), volume_signals[VOLUME_GET_CENTER], 0, &center);

  return center;
}

void amitk_volume_set_corner(AmitkVolume * volume, AmitkPoint corner) {

  g_return_if_fail(AMITK_IS_VOLUME(volume));

  if (!POINT_EQUAL(AMITK_VOLUME_CORNER(volume), corner) || (!AMITK_VOLUME_VALID(volume))) {
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
  
  shift = point_sub(new_center, amitk_volume_get_center(volume));
  if (!POINT_EQUAL(shift, zero_point)) {
    new_offset = point_add(AMITK_SPACE_OFFSET(volume), shift);
    amitk_space_set_offset(AMITK_SPACE(volume), new_offset);
  }
  return;
}

/* takes a volume and a coordinate space, and give the corners of a box with
   sides orthogonal to the given space that totally encompasses the volume.
   The returned corners are in the coordinate space of the "space" variable */
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


/* takes a list of objects and a view coordinate space, give the corners
   necessary to totally encompass the volumes in the given space */
gboolean amitk_volumes_get_enclosing_corners(const GList * objects,
					     const AmitkSpace * space,
					     AmitkCorners return_corners) {

  AmitkCorners temp_corners;
  gboolean valid=FALSE;

  while (objects != NULL) {

    if (AMITK_IS_VOLUME(objects->data)) {
      if (AMITK_VOLUME_VALID(objects->data)) {
	amitk_volume_get_enclosing_corners(AMITK_VOLUME(objects->data),space,temp_corners);
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
    objects = objects->next;
  }

  if (!valid)
    return_corners[0] = return_corners[1] = zero_point;

  return valid;
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


/* given a list of volumes, a view space, a view point, and a
   thickness, calculates an appropriate slab volume in which the
   specified volumes will be containted.  Returns true if the given
   volume structure is changed. view_center should be in the base
   reference frame.  */
/* field of view is in percent */
gboolean amitk_volumes_calc_display_volume(const GList * volumes, 
					   const AmitkSpace * space, 
					   const AmitkPoint view_center,
					   const amide_real_t thickness,
					   const amide_real_t fov,
					   AmitkVolume * volume) {

  AmitkCorners temp_corner;
  AmitkPoint temp_point;
  gboolean changed = FALSE;
  gboolean valid;
  AmitkPoint width;
  amide_real_t max_width;

  if (volumes == NULL) return FALSE;

  /* set the space of our view volume if needed */
  if (!amitk_space_equal(space, AMITK_SPACE(volume))) {
    amitk_space_copy_in_place(AMITK_SPACE(volume), space);
    changed = TRUE;
  }

  /* figure out the corners */
  valid = amitk_volumes_get_enclosing_corners(volumes,space, temp_corner);

  /* update the corners appropriately */
  if (valid) {

    temp_point = amitk_space_b2s(space, view_center);

    /* compensate for field of view */
    width = point_sub(temp_corner[1], temp_corner[0]);
    max_width = (fov/100.0)*POINT_MAX(width);

    if (width.x > max_width) { /* bigger than FOV */
      if ((temp_point.x-max_width/2.0) < temp_corner[0].x)
	temp_corner[1].x = temp_corner[0].x+max_width;
      else if ((temp_point.x+max_width/2.0) > temp_corner[1].x)
	temp_corner[0].x = temp_corner[1].x-max_width;
      else {
	temp_corner[0].x = temp_point.x - max_width/2.0;
	temp_corner[1].x = temp_point.x + max_width/2.0;
      }
    }

    if (width.y > max_width) { 
      if ((temp_point.y-max_width/2.0) < temp_corner[0].y)
	temp_corner[1].y = temp_corner[0].y+max_width;
      else if ((temp_point.y+max_width/2.0) > temp_corner[1].y)
	temp_corner[0].y = temp_corner[1].y-max_width;
      else {
	temp_corner[0].y = temp_point.y - max_width/2.0;
	temp_corner[1].y = temp_point.y + max_width/2.0;
      }
    }

    temp_corner[0].z = temp_point.z - thickness/2.0;
    temp_corner[1].z = temp_point.z + thickness/2.0;
    
    temp_corner[0] = amitk_space_s2b(space, temp_corner[0]);
    temp_corner[1] = amitk_space_s2b(space, temp_corner[1]);
    
    if (!POINT_EQUAL(AMITK_SPACE_OFFSET(volume), temp_corner[0])) {
      amitk_space_set_offset(AMITK_SPACE(volume), temp_corner[0]);
      changed = TRUE;
    }
    
    temp_corner[1] = amitk_space_b2s(AMITK_SPACE(volume), temp_corner[1]);
    if (!POINT_EQUAL(AMITK_VOLUME_CORNER(volume), temp_corner[1])) {
      amitk_volume_set_corner(volume, temp_corner[1]);
      changed = TRUE;
    }
  }

  return changed;
}
