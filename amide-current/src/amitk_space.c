/* amitk_space.c
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
#include "amitk_space.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"


enum {
  SPACE_SHIFT,
  SPACE_ROTATE,
  SPACE_INVERT,
  SPACE_TRANSFORM,
  SPACE_TRANSFORM_AXES,
  SPACE_SCALE,
  SPACE_CHANGED,
  LAST_SIGNAL
};



/* local functions */
static void          space_class_init          (AmitkSpaceClass *klass);
static void          space_init                (AmitkSpace      *object);
static void          space_shift               (AmitkSpace * space, 
						AmitkPoint * shift);
static void          space_rotate_on_vector    (AmitkSpace * space, 
						AmitkPoint * vector, 
						amide_real_t theta, 
						AmitkPoint * center_of_rotation);
static void          space_invert_axis         (AmitkSpace * space, 
						AmitkAxis which_axis,
						AmitkPoint * center_of_inversion);
static void          space_transform           (AmitkSpace * space, 
						AmitkSpace * transform_space);
static void          space_transform_axes      (AmitkSpace * space,
						AmitkAxes    axes,
						AmitkPoint * center_of_rotation);
static void          space_scale               (AmitkSpace * space, 
						AmitkPoint * ref_point, 
						AmitkPoint * scaling);

static GObjectClass * parent_class;
static guint     space_signals[LAST_SIGNAL];


GType amitk_space_get_type(void) {

  static GType space_type = 0;

  if (!space_type)
    {
      static const GTypeInfo space_info =
      {
	sizeof (AmitkSpaceClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) space_class_init,
	(GClassFinalizeFunc) NULL,
	NULL,		/* class_data */
	sizeof (AmitkSpace),
	0,			/* n_preallocs */
	(GInstanceInitFunc) space_init,
	NULL /* value table */
      };
      
      space_type = g_type_register_static (G_TYPE_OBJECT, "AmitkSpace", &space_info, 0);
    }

  return space_type;
}


static void space_class_init (AmitkSpaceClass * class) {

  //  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent(class);

  class->space_shift = space_shift;
  class->space_rotate = space_rotate_on_vector;
  class->space_invert = space_invert_axis;
  class->space_transform = space_transform;
  class->space_transform_axes = space_transform_axes;
  class->space_scale = space_scale;

  space_signals[SPACE_SHIFT] =
    g_signal_new ("space_shift",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkSpaceClass, space_shift),
		  NULL, NULL, amitk_marshal_NONE__BOXED,
		  G_TYPE_NONE,1,
		  AMITK_TYPE_POINT);
  space_signals[SPACE_ROTATE] =
    g_signal_new ("space_rotate",
  		  G_TYPE_FROM_CLASS(class),
  		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkSpaceClass, space_rotate),
		  NULL, NULL, amitk_marshal_NONE__BOXED_DOUBLE_BOXED,
		  G_TYPE_NONE,3,
		  AMITK_TYPE_POINT,
		  AMITK_TYPE_REAL,
		  AMITK_TYPE_POINT);
  space_signals[SPACE_INVERT] =
    g_signal_new ("space_invert",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkSpaceClass, space_invert),
		  NULL, NULL, amitk_marshal_NONE__ENUM_BOXED,
		  G_TYPE_NONE,2,
		  AMITK_TYPE_AXIS,
		  AMITK_TYPE_POINT);
  space_signals[SPACE_TRANSFORM] =
    g_signal_new ("space_transform",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkSpaceClass, space_transform),
		  NULL, NULL, amitk_marshal_NONE__OBJECT,
		  G_TYPE_NONE,1,
		  AMITK_TYPE_SPACE);
  space_signals[SPACE_TRANSFORM_AXES] =
    g_signal_new ("space_transform_axes",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkSpaceClass, space_transform_axes),
		  NULL, NULL, amitk_marshal_NONE__BOXED_BOXED,
		  G_TYPE_NONE,2,
		  AMITK_TYPE_AXES,
		  AMITK_TYPE_POINT);
  space_signals[SPACE_SCALE] =
    g_signal_new ("space_scale",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkSpaceClass, space_scale),
		  NULL, NULL, amitk_marshal_NONE__BOXED_BOXED,
		  G_TYPE_NONE,2,
		  AMITK_TYPE_POINT,
		  AMITK_TYPE_POINT);
  space_signals[SPACE_CHANGED] =
    g_signal_new ("space_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkSpaceClass, space_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);
}

static void space_init (AmitkSpace * space) {

  AmitkAxis i_axis;

  space->offset = zero_point;
  for (i_axis=0; i_axis<AMITK_AXIS_NUM; i_axis++)
    space->axes[i_axis] = base_axes[i_axis];

}




static void space_shift(AmitkSpace * space, AmitkPoint * shift) {

  space->offset = point_add(space->offset, *shift);

  /* get around some bizarre compiler bug */
  if (isnan(space->offset.x) || isnan(space->offset.y) || isnan(space->offset.z)) {
    g_warning("inappropriate offset, possibly an internalization problem, or a compiler bug from libecat, working around");
    space->offset = zero_point;
  }
    
  return;
}

static void space_invert_axis (AmitkSpace * space, AmitkAxis which_axis, 
			       AmitkPoint * center_of_inversion) {
  AmitkPoint shift;

  shift = amitk_space_b2s(space, *center_of_inversion);

  space->axes[which_axis] = point_neg(space->axes[which_axis]);

  shift = point_sub(*center_of_inversion, amitk_space_s2b(space, shift));
  space->offset = point_add(space->offset, shift);
  return;
}


/*center of rotation should be in base coordinate frame */
static void space_rotate_on_vector(AmitkSpace * space, AmitkPoint * vector, amide_real_t theta, 
				   AmitkPoint * center_of_rotation) {
  AmitkPoint shift;

  shift = amitk_space_b2s(space, *center_of_rotation);

  amitk_axes_rotate_on_vector(space->axes, *vector, theta);
  amitk_axes_make_orthonormal(space->axes); /* make sure the result is orthonomal */

  shift = point_sub(*center_of_rotation, amitk_space_s2b(space, shift));
  space->offset = point_add(space->offset, shift);
  return;
}


static void space_transform(AmitkSpace * space, AmitkSpace * transform_space) {

  space->offset = point_add(space->offset, transform_space->offset);

  amitk_axes_mult(transform_space->axes, space->axes, space->axes);
  amitk_axes_make_orthonormal(space->axes); /* make sure the result is orthonomal */

  /* get around some bizarre compiler bug */
  if (isnan(space->axes[AMITK_AXIS_X].x) || isnan(space->axes[AMITK_AXIS_X].y) || isnan(space->axes[AMITK_AXIS_X].z) ||
      isnan(space->axes[AMITK_AXIS_Y].x) || isnan(space->axes[AMITK_AXIS_Y].y) || isnan(space->axes[AMITK_AXIS_Y].z) ||
      isnan(space->axes[AMITK_AXIS_Z].x) || isnan(space->axes[AMITK_AXIS_Z].y) || isnan(space->axes[AMITK_AXIS_Z].z)) {
    AmitkAxis i_axis;
    g_warning("inappropriate offset: compiler bug? internalization problem? Working around");
    for (i_axis=0;i_axis<AMITK_AXIS_NUM;i_axis++)
      space->axes[i_axis]=base_axes[i_axis];
  }

  return;
}

static void space_transform_axes(AmitkSpace * space, AmitkAxes transform_axes,
				 AmitkPoint * center_of_rotation) {
  AmitkPoint shift;

  shift = amitk_space_b2s(space, *center_of_rotation);

  amitk_axes_mult(transform_axes, space->axes, space->axes);
  amitk_axes_make_orthonormal(space->axes); /* make sure the result is orthonomal */

  shift = point_sub(*center_of_rotation, amitk_space_s2b(space, shift));
  space->offset = point_add(space->offset, shift);

  return;
}


/* scaling and ref_point need to be wrt the base reference frame */
static void space_scale(AmitkSpace * space, AmitkPoint * ref_point, AmitkPoint * scaling) {

  AmitkPoint shift;

  shift = point_sub(AMITK_SPACE_OFFSET(space), *ref_point);
  shift = point_mult(*scaling, shift);
  space->offset = point_add(shift, *ref_point);

  return;
}

AmitkSpace * amitk_space_new (void) {

  AmitkSpace * space;

  space = g_object_new(amitk_space_get_type(), NULL);

  return space;
}



void amitk_space_write_xml(xmlNodePtr node, gchar * descriptor, AmitkSpace * space) {

  gchar * temp_string;
  AmitkAxis i_axis;

  temp_string = g_strdup_printf("%s_offset", descriptor);
  amitk_point_write_xml(node,temp_string, AMITK_SPACE_OFFSET(space));
  g_free(temp_string);

  for (i_axis=0;i_axis<AMITK_AXIS_NUM;i_axis++) {
    temp_string = g_strdup_printf("%s_%s", descriptor, amitk_axis_get_name(i_axis));
    amitk_point_write_xml(node,temp_string, amitk_space_get_axis(space, i_axis));
    g_free(temp_string);
  }

  return;
}

AmitkSpace * amitk_space_read_xml(xmlNodePtr nodes, gchar * descriptor, gchar **perror_buf) {

  gchar * temp_string;
  AmitkAxis i_axis;
  AmitkSpace * new_space;
  AmitkPoint new_axes[AMITK_AXIS_NUM];

  new_space = amitk_space_new();

  temp_string = g_strdup_printf("%s_offset", descriptor);
  amitk_space_set_offset(new_space, amitk_point_read_xml(nodes,temp_string, perror_buf));
  g_free(temp_string);

  for (i_axis=0;i_axis<AMITK_AXIS_NUM; i_axis++) {
    temp_string = g_strdup_printf("%s_%s", descriptor, amitk_axis_get_name(i_axis));

    new_axes[i_axis] = amitk_point_read_xml(nodes,temp_string, perror_buf);
    if (POINT_EQUAL(new_axes[i_axis], zero_point)) 
      new_axes[i_axis] = base_axes[i_axis];
    g_free(temp_string);
  }

  amitk_space_set_axes(new_space, new_axes, AMITK_SPACE_OFFSET(new_space));

  return new_space;
}




void amitk_space_set_offset(AmitkSpace * space, const AmitkPoint new_offset) {

  AmitkPoint shift;

  g_return_if_fail(AMITK_IS_SPACE(space));

  shift = point_sub(new_offset, space->offset);
  amitk_space_shift_offset(space, shift);

  return;
}

void amitk_space_shift_offset(AmitkSpace * space, const AmitkPoint shift ){

  g_return_if_fail(AMITK_IS_SPACE(space));

  if (!POINT_EQUAL(shift, zero_point)) {
    g_signal_emit(G_OBJECT(space), space_signals[SPACE_SHIFT], 0, &shift);
    g_signal_emit(G_OBJECT(space), space_signals[SPACE_CHANGED], 0);
  }
  return;
}


void amitk_space_set_axes(AmitkSpace * space, const AmitkAxes new_axes, 
			  const AmitkPoint center_of_rotation) {

  AmitkAxes transform_axes;

  g_return_if_fail(AMITK_IS_SPACE(space));

  amitk_axes_copy_in_place(transform_axes, space->axes);

  /* since the axes matrix is orthogonal, the transpose is the same as the inverse */
  amitk_axes_transpose(transform_axes);
  amitk_axes_mult(new_axes, transform_axes, transform_axes);
  /* now transform_space->axes incodes the rotation of the original coordinate space
     that will result in the new coordinate space */

  amitk_space_transform_axes(space, transform_axes, center_of_rotation);

  return;
}




/* given two spaces, calculate the transform space that when applied to src_space will 
   make it equal to dest_space. */
AmitkSpace * amitk_space_calculate_transform(const AmitkSpace * dest_space, const AmitkSpace * src_space) {

  AmitkSpace * transform_space;

  g_return_val_if_fail(AMITK_IS_SPACE(src_space), NULL);
  g_return_val_if_fail(AMITK_IS_SPACE(dest_space), NULL);

  /* transform_space will encode the shift and rotation of the coordinate space */
  transform_space = amitk_space_new();
  transform_space->offset = point_sub(src_space->offset, dest_space->offset);

  amitk_axes_copy_in_place(transform_space->axes, dest_space->axes);

  /* since the axes matrix is orthogonal, the transpose is the same as the inverse */
  amitk_axes_transpose(transform_space->axes);
  amitk_axes_mult(src_space->axes, transform_space->axes, transform_space->axes);
  /* now transform_space->axes incodes the rotation of the original coordinate space
     that will result in the new coordinate space */

  return transform_space;
}

void amitk_space_copy_in_place(AmitkSpace * dest_space, const AmitkSpace * src_space) {

  AmitkSpace * transform_space;


  g_return_if_fail(AMITK_IS_SPACE(src_space));
  g_return_if_fail(AMITK_IS_SPACE(dest_space));

  transform_space = amitk_space_calculate_transform(dest_space, src_space);
  g_return_if_fail(transform_space != NULL);
  amitk_space_transform(dest_space, transform_space);
  g_object_unref(transform_space);

  return;
}



void amitk_space_transform(AmitkSpace * space, const AmitkSpace * transform_space) {

  g_return_if_fail(AMITK_IS_SPACE(space));
  g_return_if_fail(AMITK_IS_SPACE(transform_space));

  g_signal_emit(G_OBJECT(space), space_signals[SPACE_TRANSFORM], 0, transform_space);
  g_signal_emit(G_OBJECT(space), space_signals[SPACE_CHANGED], 0);

  return;
}

void amitk_space_transform_axes(AmitkSpace * space, const AmitkAxes transform_axes,
				AmitkPoint center_of_rotation) {

  g_return_if_fail(AMITK_IS_SPACE(space));

  g_signal_emit(G_OBJECT(space), space_signals[SPACE_TRANSFORM_AXES], 
		0, transform_axes, &center_of_rotation);
  g_signal_emit(G_OBJECT(space), space_signals[SPACE_CHANGED], 0);

  return;
}

/* scaling and ref_point need to be wrt the base reference frame */

/* note, this doesn't actually scale the coordinate axis, 
   as we always leave those as orthonormal.
   instead, it shifts the offset of the space appropriately giving the ref_point,
   and leaves further scaling operations to objects that inherit from this class
 */
void amitk_space_scale(AmitkSpace * space, AmitkPoint ref_point, AmitkPoint scaling) {


  g_return_if_fail(AMITK_IS_SPACE(space));

  g_signal_emit(G_OBJECT(space), space_signals[SPACE_SCALE], 0, &ref_point, &scaling);
  g_signal_emit(G_OBJECT(space), space_signals[SPACE_CHANGED], 0);

}


AmitkPoint amitk_space_get_axis(const AmitkSpace * space, const AmitkAxis which_axis) {

  g_return_val_if_fail(AMITK_IS_SPACE(space), one_point);

  return space->axes[which_axis];
}



/* given the ordered corners[] (i.e corner[0].x < corner[1].x, etc.)
   of a box in the input coordinate space, this function will return an ordered set of
   corners for the box in the new coordinate space that completely encloses
   the given box */
void amitk_space_get_enclosing_corners(const AmitkSpace * in_space, const AmitkCorners in_corners, 
				       const AmitkSpace * out_space, AmitkCorners out_corners) {

  AmitkPoint temp_point;
  guint corner;

  g_return_if_fail(AMITK_IS_SPACE(in_space));
  g_return_if_fail(AMITK_IS_SPACE(out_space));

  for (corner=0; corner<8 ; corner++) {
    temp_point.x = in_corners[(corner & 0x1) ? 1 : 0].x;
    temp_point.y = in_corners[(corner & 0x2) ? 1 : 0].y;
    temp_point.z = in_corners[(corner & 0x4) ? 1 : 0].z;
    temp_point = amitk_space_s2s(in_space, out_space, temp_point);

    if (corner==0)
      out_corners[0]=out_corners[1]=temp_point;
    else {
      if (temp_point.x < out_corners[0].x) out_corners[0].x=temp_point.x;
      if (temp_point.x > out_corners[1].x) out_corners[1].x=temp_point.x;
      if (temp_point.y < out_corners[0].y) out_corners[0].y=temp_point.y;
      if (temp_point.y > out_corners[1].y) out_corners[1].y=temp_point.y;
      if (temp_point.z < out_corners[0].z) out_corners[0].z=temp_point.z;
      if (temp_point.z > out_corners[1].z) out_corners[1].z=temp_point.z;
    }
  }

  return;
}


AmitkSpace * amitk_space_copy(const AmitkSpace * space) {

  AmitkSpace * new_space;

  g_return_val_if_fail(AMITK_IS_SPACE(space), NULL);

  new_space = amitk_space_new();
  amitk_space_copy_in_place(new_space, space);

  return new_space;
}

gboolean amitk_space_axes_equal(const AmitkSpace * space1, const AmitkSpace * space2) {

  gboolean inner = TRUE;
  AmitkAxis i_axis;

  g_return_val_if_fail(AMITK_IS_SPACE(space1), FALSE);
  g_return_val_if_fail(AMITK_IS_SPACE(space2), FALSE);

  for (i_axis=0;i_axis<AMITK_AXIS_NUM && inner;i_axis++) 
    inner = inner && POINT_EQUAL(space1->axes[i_axis], space2->axes[i_axis]);

  return inner;
}

gboolean amitk_space_axes_close(const AmitkSpace * space1, const AmitkSpace * space2) {

  gboolean inner = TRUE;
  AmitkAxis i_axis;

  g_return_val_if_fail(AMITK_IS_SPACE(space1), FALSE);
  g_return_val_if_fail(AMITK_IS_SPACE(space2), FALSE);

  for (i_axis=0;i_axis<AMITK_AXIS_NUM && inner;i_axis++) 
    inner = inner && POINT_CLOSE(space1->axes[i_axis], space2->axes[i_axis]);

  return inner;
}


gboolean amitk_space_equal(const AmitkSpace * space1, const AmitkSpace * space2) {

  g_return_val_if_fail(AMITK_IS_SPACE(space1), FALSE);
  g_return_val_if_fail(AMITK_IS_SPACE(space2), FALSE);

  if (POINT_EQUAL(space1->offset, space2->offset)) 
    return amitk_space_axes_equal(space1, space2);
  else
    return FALSE;
}


void amitk_space_invert_axis (AmitkSpace * space, const AmitkAxis which_axis,
			      const AmitkPoint center_of_inversion) {

  g_return_if_fail(AMITK_IS_SPACE(space));

  g_signal_emit(G_OBJECT(space), space_signals[SPACE_INVERT], 0, which_axis, &center_of_inversion);
  g_signal_emit(G_OBJECT(space), space_signals[SPACE_CHANGED], 0);

  return;
}




/* rotate the entire coordinate space around the given vector */
void amitk_space_rotate_on_vector(AmitkSpace * space, const AmitkPoint vector, 
				  const amide_real_t theta, const AmitkPoint center_of_rotation) {

  g_return_if_fail(AMITK_IS_SPACE(space));

  g_signal_emit(G_OBJECT(space), space_signals[SPACE_ROTATE], 0, &vector, theta, &center_of_rotation);
  g_signal_emit(G_OBJECT(space), space_signals[SPACE_CHANGED], 0);

  return;
}



/* given a view, and the layout, return the appropriate coordinate frame */
AmitkSpace * amitk_space_get_view_space(const AmitkView view, 
					const AmitkLayout layout) {

  AmitkSpace * view_space;
  AmitkAxis i_axis;

  view_space = amitk_space_new();

  for (i_axis=0;i_axis<AMITK_AXIS_NUM;i_axis++)
    view_space->axes[i_axis] = amitk_axes_get_orthogonal_axis(base_axes, view, layout, i_axis);

  amitk_axes_make_orthonormal(view_space->axes); /* safety */

  return view_space;
}

/* given a coordinate frame, and a view, and the layout, sets the space to 
   the appropriate coordinate space */
void amitk_space_set_view_space(AmitkSpace * set_space, const AmitkView view, const AmitkLayout layout) {

  AmitkSpace * temp_space;

  g_return_if_fail(AMITK_IS_SPACE(set_space));

  temp_space = amitk_space_get_view_space(view, layout);
  amitk_space_copy_in_place(set_space,  temp_space);
  g_object_unref(temp_space);

  return;
}




/* convert a point from the base coordinate frame to the given coordinate space */
AmitkPoint amitk_space_b2s(const AmitkSpace * space, AmitkPoint in_point) {

  AmitkPoint return_point;

  /* compensate the inpoint for the offset of the new coordinate frame */
  POINT_SUB(in_point, space->offset, in_point);

  /* instead of multiplying by inv(A), we multiple by transpose(A),
     this is the same thing, as A is orthogonal */
  return_point.x = 
    in_point.x * space->axes[AMITK_AXIS_X].x +
    in_point.y * space->axes[AMITK_AXIS_X].y +
    in_point.z * space->axes[AMITK_AXIS_X].z;
  return_point.y = 
    in_point.x * space->axes[AMITK_AXIS_Y].x +
    in_point.y * space->axes[AMITK_AXIS_Y].y +
    in_point.z * space->axes[AMITK_AXIS_Y].z;
  return_point.z = 
    in_point.x * space->axes[AMITK_AXIS_Z].x +
    in_point.y * space->axes[AMITK_AXIS_Z].y +
    in_point.z * space->axes[AMITK_AXIS_Z].z;

  return return_point;

}
/* convert a point from the given coordinate spaace to the base coordinate frame */
AmitkPoint amitk_space_s2b(const AmitkSpace * space, const AmitkPoint in_point) {

  AmitkPoint return_point;

  return_point.x = 
    in_point.x * space->axes[AMITK_AXIS_X].x +
    in_point.y * space->axes[AMITK_AXIS_Y].x +
    in_point.z * space->axes[AMITK_AXIS_Z].x;
  return_point.y = 
    in_point.x * space->axes[AMITK_AXIS_X].y +
    in_point.y * space->axes[AMITK_AXIS_Y].y +
    in_point.z * space->axes[AMITK_AXIS_Z].y;
  return_point.z = 
    in_point.x * space->axes[AMITK_AXIS_X].z +
    in_point.y * space->axes[AMITK_AXIS_Y].z +
    in_point.z * space->axes[AMITK_AXIS_Z].z;

  POINT_ADD(return_point, space->offset, return_point);

  return return_point;
}

/* converts a "dimensional" quantity (i.e. the size of a voxel) from a
   given space to the base coordinate system */
AmitkPoint amitk_space_s2b_dim(const AmitkSpace * space, const AmitkPoint in_point) {

  AmitkPoint return_point;
  AmitkPoint temp[AMITK_AXIS_NUM];
  AmitkPoint temp_point;

  /* all the fabs/point_abs are 'cause dim's should always be positive */
  temp_point.x = fabs(in_point.x); 
  temp_point.y = temp_point.z = 0.0;
  temp[AMITK_AXIS_X]=point_abs(point_sub(amitk_space_s2b(space, temp_point), space->offset));

  temp_point.y = fabs(in_point.y); 
  temp_point.x = temp_point.z = 0.0;
  temp[AMITK_AXIS_Y]=point_abs(point_sub(amitk_space_s2b(space, temp_point), space->offset));

  temp_point.z = fabs(in_point.z); 
  temp_point.x = temp_point.y = 0.0;
  temp[AMITK_AXIS_Z]=point_abs(point_sub(amitk_space_s2b(space, temp_point), space->offset));

  return_point.x = temp[AMITK_AXIS_X].x+temp[AMITK_AXIS_Y].x+temp[AMITK_AXIS_Z].x;
  return_point.y = temp[AMITK_AXIS_X].y+temp[AMITK_AXIS_Y].y+temp[AMITK_AXIS_Z].y;
  return_point.z = temp[AMITK_AXIS_X].z+temp[AMITK_AXIS_Y].z+temp[AMITK_AXIS_Z].z;

  return return_point;
}

/* converts a "dimensional" quantity (i.e. the size of a voxel) from the
   base coordinate space to the given coordinate space */
AmitkPoint amitk_space_b2s_dim(const AmitkSpace * space, const AmitkPoint in_point) {

  AmitkPoint return_point;
  AmitkPoint temp[AMITK_AXIS_NUM];
  AmitkPoint temp_point;
  
  /* all the fabs/point_abs are 'cause dim's should always be positive */
  temp_point.x = fabs(in_point.x); 
  temp_point.y = temp_point.z = 0.0;
  temp[AMITK_AXIS_X]=point_abs(amitk_space_b2s(space, point_add(temp_point, space->offset)));

  temp_point.y = fabs(in_point.y); 
  temp_point.x = temp_point.z = 0.0;
  temp[AMITK_AXIS_Y]=point_abs(amitk_space_b2s(space, point_add(temp_point, space->offset)));

  temp_point.z = fabs(in_point.z); 
  temp_point.x = temp_point.y = 0.0;
  temp[AMITK_AXIS_Z]=point_abs(amitk_space_b2s(space, point_add(temp_point,space->offset)));


  return_point.x = temp[AMITK_AXIS_X].x+temp[AMITK_AXIS_Y].x+temp[AMITK_AXIS_Z].x;
  return_point.y = temp[AMITK_AXIS_X].y+temp[AMITK_AXIS_Y].y+temp[AMITK_AXIS_Z].y;
  return_point.z = temp[AMITK_AXIS_X].z+temp[AMITK_AXIS_Y].z+temp[AMITK_AXIS_Z].z;

  return return_point;
}











/* little utility function for debugging */
void amitk_space_print(AmitkSpace * space, gchar * message) {

  g_print("%s\n", message);
  g_print("\toffset:\t% 5.5f\t% 5.5f\t% 5.5f\n",
	 space->offset.x, space->offset.y, space->offset.z);
  g_print("\taxis x:\t% 5.5f\t% 5.5f\t% 5.5f\n",
	  space->axes[AMITK_AXIS_X].x,space->axes[AMITK_AXIS_X].y, space->axes[AMITK_AXIS_X].z);
  g_print("\taxis y:\t% 5.5f\t% 5.5f\t% 5.5f\n",
	  space->axes[AMITK_AXIS_Y].x,space->axes[AMITK_AXIS_Y].y, space->axes[AMITK_AXIS_Y].z);
  g_print("\taxis z:\t% 5.5f\t% 5.5f\t% 5.5f\n",
	  space->axes[AMITK_AXIS_Z].x,space->axes[AMITK_AXIS_Z].y, space->axes[AMITK_AXIS_Z].z);
  return;
}

