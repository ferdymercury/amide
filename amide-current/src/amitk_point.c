/* amitk_point.c
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
#include <locale.h>
#include "amitk_point.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"


const AmitkAxes base_axes =
  {{1.0,0.0,0.0},
   {0.0,1.0,0.0},
   {0.0,0.0,1.0}};


GType amitk_point_get_type (void) {

  static GType our_type = 0;

  if (our_type == 0)
    our_type = g_boxed_type_register_static ("AmitkPoint",
					     (GBoxedCopyFunc) amitk_point_copy,
					     (GBoxedFreeFunc) amitk_point_free);
  return our_type;
}

AmitkPoint * amitk_point_copy(const AmitkPoint * point) {
  return (AmitkPoint *)g_memdup(point, sizeof(AmitkPoint));
}

void amitk_point_free (AmitkPoint * point) {
  g_free (point);
}



AmitkPoint amitk_point_read_xml(xmlNodePtr nodes, gchar * descriptor, gchar **perror_buf) {

  gchar * temp_str;
  AmitkPoint return_rp;
  gint error=EOF;
  gchar * saved_locale;
  
  saved_locale = g_strdup(setlocale(LC_NUMERIC,NULL));
  setlocale(LC_NUMERIC,"POSIX");

  temp_str = xml_get_string(nodes, descriptor);

  if (temp_str != NULL) {

    xml_convert_radix_to_local(temp_str);

#if (SIZE_OF_AMIDE_REAL_T == 8)
    /* convert to doubles */
    error = sscanf(temp_str, "%lf\t%lf\t%lf", &(return_rp.x), &(return_rp.y), &(return_rp.z));
#elif (SIZE_OF_AMIDE_REAL_T == 4)
    /* convert to float */
    error = sscanf(temp_str, "%f\t%f\t%f", &(return_rp.x), &(return_rp.y), &(return_rp.z));
#else
#error "Unknown size for SIZE_OF_AMIDE_REAL_T"
#endif
    g_free(temp_str);
  }

  if ((temp_str == NULL) || (error == EOF)) {
    return_rp = zero_point;
    amitk_append_str_with_newline(perror_buf,_("Couldn't read value for %s, substituting [%5.3f %5.3f %5.3f]"),
				  descriptor, return_rp.x, return_rp.y, return_rp.z);
  }

  setlocale(LC_NUMERIC, saved_locale);
  g_free(saved_locale);
  return return_rp;

}

void amitk_point_write_xml(xmlNodePtr node, gchar * descriptor, AmitkPoint point) {

#ifdef OLD_WIN32_HACKS
  gchar temp_str[128];
#else
  gchar * temp_str;
#endif
  gchar * saved_locale;

  saved_locale = g_strdup(setlocale(LC_NUMERIC,NULL));
  setlocale(LC_NUMERIC,"POSIX");

#ifdef OLD_WIN32_HACKS
  snprintf(temp_str, 128, "%10.9f\t%10.9f\t%10.9f", point.x,point.y,point.z);
#else
  temp_str = g_strdup_printf("%10.9f\t%10.9f\t%10.9f",point.x, point.y,point.z);
#endif
  xml_save_string(node, descriptor, temp_str);
#ifndef OLD_WIN32_HACKS
  g_free(temp_str);
#endif

  setlocale(LC_NUMERIC, saved_locale);
  g_free(saved_locale);
  return;
}









GType amitk_voxel_get_type (void) {

  static GType our_type = 0;
  
  if (our_type == 0)
    our_type = g_boxed_type_register_static ("AmitkVoxel",
					     (GBoxedCopyFunc) amitk_voxel_copy,
					     (GBoxedFreeFunc) amitk_voxel_free);
  return our_type;
}

AmitkVoxel * amitk_voxel_copy(const AmitkVoxel * voxel) {
  return (AmitkVoxel *)g_memdup(voxel, sizeof(AmitkVoxel));
}

void amitk_voxel_free (AmitkVoxel * voxel) {
  g_free(voxel);
}

AmitkVoxel amitk_voxel_read_xml(xmlNodePtr nodes, gchar * descriptor, gchar **perror_buf) {

  gchar * temp_str;
  AmitkVoxel voxel;
  gint x,y,z,g,t;
  gint error=0;

  voxel = one_voxel; /* initialize */
  temp_str = xml_get_string(nodes, descriptor);

  if (temp_str != NULL) {

    /* convert to a voxel */
    error = sscanf(temp_str,"%d\t%d\t%d\t%d\t%d", &x,&y,&z, &g, &t);
    g_free(temp_str);
    
    voxel.x = x;
    voxel.y = y;
    voxel.z = z;
    voxel.g = g;
    voxel.t = t;

  } 

  if ((temp_str == NULL) || (error == EOF)) {
    voxel = zero_voxel;
    amitk_append_str_with_newline(perror_buf,_("Couldn't read value for %s, substituting [%d %d %d %d %d]"),
				  descriptor, voxel.x, voxel.y,voxel.z,voxel.g, voxel.t);
  }

  if (error < 5) {
    /* note, gate was added later, so if we only read 4, the 4th is most likely frames */
    voxel.t = voxel.g;
    voxel.g = 1;
    amitk_append_str_with_newline(perror_buf, _("Couldn't read gate value for %s, substituting %d"),
				  descriptor, voxel.g);
  } else if (error < 4) {
    voxel.t = 1;
    amitk_append_str_with_newline(perror_buf,_("Couldn't read frame value for %s, substituting %d"),
				  descriptor, voxel.t);
  }

  return voxel;
}


void amitk_voxel_write_xml(xmlNodePtr node, gchar * descriptor, AmitkVoxel voxel) {

  gchar * temp_str;

  temp_str = g_strdup_printf("%d\t%d\t%d\t%d\t%d",voxel.x, voxel.y, voxel.z, voxel.g, voxel.t);
  xml_save_string(node, descriptor, temp_str);
  g_free(temp_str);

  return;
}



GType amitk_pixel_get_type (void) {

  static GType our_type = 0;
  
  if (our_type == 0)
    our_type = g_boxed_type_register_static ("AmitkPixel",
					     (GBoxedCopyFunc) amitk_pixel_copy,
					     (GBoxedFreeFunc) amitk_pixel_free);
  return our_type;
}

AmitkPixel * amitk_pixel_copy(const AmitkPixel * pixel) {
  return (AmitkPixel *)g_memdup(pixel, sizeof(AmitkPixel));
}

void amitk_pixel_free (AmitkPixel * pixel) {
  g_free (pixel);
}

GType amitk_canvas_point_get_type (void) {

  static GType our_type = 0;
  
  if (our_type == 0)
    our_type = g_boxed_type_register_static ("AmitkCanvasPoint",
					     (GBoxedCopyFunc) amitk_canvas_point_copy,
					     (GBoxedFreeFunc) amitk_canvas_point_free);
  return our_type;
}

AmitkCanvasPoint * amitk_canvas_point_copy(const AmitkCanvasPoint * point) {
  return (AmitkCanvasPoint *)g_memdup(point, sizeof(AmitkCanvasPoint));
}

void amitk_canvas_point_free (AmitkCanvasPoint * point) {
  g_free (point);
}





GType amitk_axes_get_type (void) {

  static GType our_type = 0;

  if (our_type == 0)
    our_type = g_boxed_type_register_static ("AmitkAxes",
					     (GBoxedCopyFunc) amitk_axes_copy,
					     (GBoxedFreeFunc) amitk_axes_free);
  return our_type;
}

AmitkAxes * amitk_axes_copy(const AmitkAxes * axes) {
  return (AmitkAxes *)g_memdup(axes, sizeof(AmitkAxes));
}

void amitk_axes_free (AmitkAxes * axes) {
  g_free (axes);
}

void amitk_axes_copy_in_place(AmitkAxes dest_axes, const AmitkAxes src_axes) {

  AmitkAxis i_axis;

  for (i_axis=0; i_axis<AMITK_AXIS_NUM; i_axis++)
    dest_axes[i_axis] = src_axes[i_axis];

  return;
}

void amitk_axes_transpose(AmitkAxes axes) {

  amide_real_t temp_val;


  temp_val = axes[AMITK_AXIS_Y].x;
  axes[AMITK_AXIS_Y].x = axes[AMITK_AXIS_X].y;
  axes[AMITK_AXIS_X].y = temp_val;

  temp_val = axes[AMITK_AXIS_Z].x;
  axes[AMITK_AXIS_Z].x = axes[AMITK_AXIS_X].z;
  axes[AMITK_AXIS_X].z = temp_val;

  temp_val = axes[AMITK_AXIS_Z].y;
  axes[AMITK_AXIS_Z].y = axes[AMITK_AXIS_Y].z;
  axes[AMITK_AXIS_Y].z = temp_val;

  return;
}

void amitk_axes_mult(const AmitkAxes const_axes1, const AmitkAxes const_axes2, AmitkAxes dest_axes) {

  AmitkAxes axes1;
  AmitkAxes axes2;

  amitk_axes_copy_in_place(axes1, const_axes1);
  amitk_axes_copy_in_place(axes2, const_axes2);

  dest_axes[AMITK_AXIS_X].x = 
    axes1[AMITK_AXIS_X].x * axes2[AMITK_AXIS_X].x +
    axes1[AMITK_AXIS_Y].x * axes2[AMITK_AXIS_X].y +
    axes1[AMITK_AXIS_Z].x * axes2[AMITK_AXIS_X].z;
  dest_axes[AMITK_AXIS_Y].x = 
    axes1[AMITK_AXIS_X].x * axes2[AMITK_AXIS_Y].x +
    axes1[AMITK_AXIS_Y].x * axes2[AMITK_AXIS_Y].y +
    axes1[AMITK_AXIS_Z].x * axes2[AMITK_AXIS_Y].z;
  dest_axes[AMITK_AXIS_Z].x = 
    axes1[AMITK_AXIS_X].x * axes2[AMITK_AXIS_Z].x +
    axes1[AMITK_AXIS_Y].x * axes2[AMITK_AXIS_Z].y +
    axes1[AMITK_AXIS_Z].x * axes2[AMITK_AXIS_Z].z;

  dest_axes[AMITK_AXIS_X].y = 
    axes1[AMITK_AXIS_X].y * axes2[AMITK_AXIS_X].x +
    axes1[AMITK_AXIS_Y].y * axes2[AMITK_AXIS_X].y +
    axes1[AMITK_AXIS_Z].y * axes2[AMITK_AXIS_X].z;
  dest_axes[AMITK_AXIS_Y].y = 
    axes1[AMITK_AXIS_X].y * axes2[AMITK_AXIS_Y].x +
    axes1[AMITK_AXIS_Y].y * axes2[AMITK_AXIS_Y].y +
    axes1[AMITK_AXIS_Z].y * axes2[AMITK_AXIS_Y].z;
  dest_axes[AMITK_AXIS_Z].y = 
    axes1[AMITK_AXIS_X].y * axes2[AMITK_AXIS_Z].x +
    axes1[AMITK_AXIS_Y].y * axes2[AMITK_AXIS_Z].y +
    axes1[AMITK_AXIS_Z].y * axes2[AMITK_AXIS_Z].z;

  dest_axes[AMITK_AXIS_X].z = 
    axes1[AMITK_AXIS_X].z * axes2[AMITK_AXIS_X].x +
    axes1[AMITK_AXIS_Y].z * axes2[AMITK_AXIS_X].y +
    axes1[AMITK_AXIS_Z].z * axes2[AMITK_AXIS_X].z;
  dest_axes[AMITK_AXIS_Y].z = 
    axes1[AMITK_AXIS_X].z * axes2[AMITK_AXIS_Y].x +
    axes1[AMITK_AXIS_Y].z * axes2[AMITK_AXIS_Y].y +
    axes1[AMITK_AXIS_Z].z * axes2[AMITK_AXIS_Y].z;
  dest_axes[AMITK_AXIS_Z].z = 
    axes1[AMITK_AXIS_X].z * axes2[AMITK_AXIS_Z].x +
    axes1[AMITK_AXIS_Y].z * axes2[AMITK_AXIS_Z].y +
    axes1[AMITK_AXIS_Z].z * axes2[AMITK_AXIS_Z].z;

  return;
}


/* adjusts the given axis into an orthogonal set via gram-schmidt */
static void make_orthogonal(AmitkAxes axes) {

  AmitkPoint temp;
  amide_real_t scale;

  /* leave the xaxis as is */

  /* make the y axis orthogonal */
  scale = -1.0*POINT_DOT_PRODUCT(axes[AMITK_AXIS_X],axes[AMITK_AXIS_Y]) /
    POINT_DOT_PRODUCT(axes[AMITK_AXIS_X], axes[AMITK_AXIS_X]);
  POINT_MADD(1.0,axes[AMITK_AXIS_Y], scale, axes[AMITK_AXIS_X], axes[AMITK_AXIS_Y]);
  
  /* and make the z axis orthogonal */
  scale = -1.0*POINT_DOT_PRODUCT(axes[AMITK_AXIS_X],axes[AMITK_AXIS_Z]) /
	     POINT_DOT_PRODUCT(axes[AMITK_AXIS_X],axes[AMITK_AXIS_X]),
  POINT_MADD(1.0, axes[AMITK_AXIS_Z], scale, axes[AMITK_AXIS_X], temp);
  scale = -1.0*POINT_DOT_PRODUCT(axes[AMITK_AXIS_Y],axes[AMITK_AXIS_Z]) /
    POINT_DOT_PRODUCT(axes[AMITK_AXIS_Y],axes[AMITK_AXIS_Y]);
  POINT_MADD(1.0, temp, scale, axes[AMITK_AXIS_Y], axes[AMITK_AXIS_Z]);

  return;
}

/* adjusts the given axis into a true orthonormal axis set */
void amitk_axes_make_orthonormal(AmitkAxes axes) {

  amide_real_t scale;
  AmitkAxis i_axis;
  gboolean nonsense=FALSE;

  make_orthogonal(axes); 

  /* now normalize the axis to make it orthonormal */
  scale = 1.0/POINT_MAGNITUDE(axes[AMITK_AXIS_X]);
  POINT_CMULT(scale, axes[AMITK_AXIS_X], axes[AMITK_AXIS_X]);
  scale = 1.0/POINT_MAGNITUDE(axes[AMITK_AXIS_Y]);
  POINT_CMULT(scale, axes[AMITK_AXIS_Y], axes[AMITK_AXIS_Y]);
  scale = 1.0/POINT_MAGNITUDE(axes[AMITK_AXIS_Z]);
  POINT_CMULT(scale, axes[AMITK_AXIS_Z], axes[AMITK_AXIS_Z]);

  /* insure we didn't nan ourselves into oblivion with the divisions */
  for (i_axis=0; (i_axis < AMITK_AXIS_NUM) && !nonsense; i_axis++) {
    if (isnan(axes[i_axis].x) || isnan(axes[i_axis].y) || isnan(axes[i_axis].z)) {
      g_warning("inappropriate axes, division by zero? Setting to identity");
      nonsense=TRUE;
    }
  }

  if (nonsense)
    for (i_axis=0; i_axis < AMITK_AXIS_NUM; i_axis++) 
      axes[i_axis] = base_axes[i_axis];

  return;
}



void amitk_axes_rotate_on_vector(AmitkAxes axes, AmitkPoint vector, amide_real_t theta) {
  AmitkAxis i_axis;
  for (i_axis=0;i_axis<AMITK_AXIS_NUM;i_axis++)
    axes[i_axis] = point_rotate_on_vector(axes[i_axis], vector, theta);
  return;
}




/* returns the axis vector which corresponds to the orthogonal axis (specified
   by ax) for the given set of axes in the given view (i.e. coronal, sagittal, etc.) */
AmitkPoint amitk_axes_get_orthogonal_axis(const AmitkAxes axes,
					  const AmitkView which_view,
					  const AmitkLayout which_layout,
					  const AmitkAxis which_axis) {
  
  switch(which_view) {
  case AMITK_VIEW_CORONAL:
    switch (which_axis) {
    case AMITK_AXIS_X:
      return axes[AMITK_AXIS_X];
      break;
    case AMITK_AXIS_Y:
      return point_neg(axes[AMITK_AXIS_Z]);
      break;
    case AMITK_AXIS_Z:
    default:
      return axes[AMITK_AXIS_Y];
      break;
    }
    break;
  case AMITK_VIEW_SAGITTAL:
    switch (which_axis) {
    case AMITK_AXIS_X:
      if (which_layout == AMITK_LAYOUT_ORTHOGONAL)
	return axes[AMITK_AXIS_Z];
      else /* AMITK_LAYOUT_LINEAR */
	return axes[AMITK_AXIS_Y];
      break;
    case AMITK_AXIS_Y:
      if (which_layout == AMITK_LAYOUT_ORTHOGONAL)
	return axes[AMITK_AXIS_Y];
      else /* AMITK_LAYOUT_LINEAR */
	return point_neg(axes[AMITK_AXIS_Z]);
      break;
    case AMITK_AXIS_Z:
    default:
      return axes[AMITK_AXIS_X];
      break;
    }
  case AMITK_VIEW_TRANSVERSE:
  default:
    switch (which_axis) {
    case AMITK_AXIS_X:
      return axes[AMITK_AXIS_X];
      break;
    case AMITK_AXIS_Y:
      return axes[AMITK_AXIS_Y];
      break;
    case AMITK_AXIS_Z:
    default:
      return axes[AMITK_AXIS_Z];
      break;
    }
    break;
  }

  /* shouldn't get here */
  return axes[AMITK_AXIS_Z];
}

/* returns the normal axis vector for the given view */
AmitkPoint amitk_axes_get_normal_axis(const AmitkAxes axes, const AmitkView which_view) {

  /* don't need layout here, as the AMITK_AXIS_Z isn't determined by the layout */
  return amitk_axes_get_orthogonal_axis(axes, which_view, AMITK_LAYOUT_LINEAR, AMITK_AXIS_Z);
}


GType amitk_corners_get_type (void) {

  static GType our_type = 0;

  if (our_type == 0)
    our_type = g_boxed_type_register_static ("AmitkCorners",
					     (GBoxedCopyFunc) amitk_corners_copy,
					     (GBoxedFreeFunc) amitk_corners_free);
  return our_type;
}

AmitkCorners * amitk_corners_copy(const AmitkCorners * corners) {
  return (AmitkCorners *)g_memdup(corners, sizeof(AmitkCorners));
}

void amitk_corners_free (AmitkCorners * corners) {
  g_free (corners);
}











const AmitkPoint zero_point = {0.0,0.0,0.0};
const AmitkPoint one_point = ONE_POINT;
const AmitkPoint ten_point = {10.0,10.0,10.0};

const AmitkVoxel zero_voxel = {0,0,0,0,0};
const AmitkVoxel one_voxel = ONE_VOXEL;

/* returns abs(point1) for realpoint structures */
AmitkPoint point_abs(const AmitkPoint point1) {
  AmitkPoint temp;
  POINT_ABS(point1, temp);
  return temp;
}
/* returns -point1 for realpoint structures */
AmitkPoint point_neg(const AmitkPoint point1) {
  AmitkPoint temp;
  temp.x = -point1.x;
  temp.y = -point1.y;
  temp.z = -point1.z;
  return temp;
}
/* returns point1+point2 for realpoint structures */
AmitkPoint point_add(const AmitkPoint point1,const AmitkPoint point2) {
  AmitkPoint temp;
  POINT_ADD(point1, point2, temp);
  return temp;
}
/* returns point1-point2 for realpoint structures */
AmitkPoint point_sub(const AmitkPoint point1,const AmitkPoint point2) {
  AmitkPoint temp;
  POINT_SUB(point1, point2, temp);
  return temp;
}
/* returns point1.*point2 for realpoint structures */
AmitkPoint point_mult(const AmitkPoint point1,const AmitkPoint point2) {
  AmitkPoint temp;
  POINT_MULT(point1, point2, temp);
  return temp;
}
/* returns point1./point2 for realpoint structures */
AmitkPoint point_div(const AmitkPoint point1,const AmitkPoint point2) {
  AmitkPoint temp;
  POINT_DIV(point1, point2, temp);
  return temp;
}
/* returns abs(point1-point2) for realpoint structures */
AmitkPoint point_diff(const AmitkPoint point1,const AmitkPoint point2) {
  AmitkPoint temp;
  POINT_DIFF(point1, point2, temp);
  return temp;
}
/* returns cm*point1 for realpoint structures */
AmitkPoint point_cmult(const amide_real_t cmult,const AmitkPoint point1) {
  AmitkPoint temp;
  POINT_CMULT(cmult, point1, temp);
  return temp;
}

/* returns cross product of point1 and point2 for realpoint structures */
AmitkPoint point_cross_product(const AmitkPoint point1, const AmitkPoint point2) {
  AmitkPoint temp;
  POINT_CROSS_PRODUCT(point1, point2, temp);
  return temp;
}

/* returns dot product of point1 and point2 for realpoint structures */
amide_real_t point_dot_product(const AmitkPoint point1, const AmitkPoint point2) {
  return POINT_DOT_PRODUCT(point1, point2);
}


/* returns sqrt(point_dot_product(point1, point1)) for realpoint structures */
amide_real_t point_mag(const AmitkPoint point1) {
  return sqrt(POINT_DOT_PRODUCT(point1, point1));
}

/* returns the minimum dimension of the "box" defined by point1*/
amide_real_t point_min_dim(const AmitkPoint point1) {
  return MIN( MIN(point1.x,point1.y), point1.z);
}

/* returns the maximum dimension of the "box" defined by point1 */
amide_real_t point_max_dim(const AmitkPoint point1) {
  return point_mag(point1);
}



/* returns abs(point1-point2) for canvaspoint structures */
AmitkCanvasPoint canvas_point_diff(const AmitkCanvasPoint point1,const AmitkCanvasPoint point2) {
  AmitkCanvasPoint temp;
  temp.x = fabs(point1.x-point2.x);
  temp.y = fabs(point1.y-point2.y);
  return temp;
}
/* returns point1-point2 for canvaspoint structures */
AmitkCanvasPoint canvas_point_sub(const AmitkCanvasPoint point1,const AmitkCanvasPoint point2) {
  AmitkCanvasPoint temp;
  temp.x = point1.x-point2.x;
  temp.y = point1.y-point2.y;
  return temp;
}
/* returns point1+point2 for canvaspoint structures */
AmitkCanvasPoint canvas_point_add(const AmitkCanvasPoint point1,const AmitkCanvasPoint point2) {
  AmitkCanvasPoint temp;
  temp.x = point1.x+point2.x;
  temp.y = point1.y+point2.y;
  return temp;
}
/* returns cm*point1 for canvaspoint structures */
AmitkCanvasPoint canvas_point_cmult(const amide_real_t cmult,const AmitkCanvasPoint point1) {
  AmitkCanvasPoint temp;
  temp.x = cmult*point1.x;
  temp.y = cmult*point1.y;
  return temp;
}
/* returns dot product of point1 and point2 for canvaspoint structures */
amide_real_t canvas_point_dot_product(const AmitkCanvasPoint point1, const AmitkCanvasPoint point2) {

  return point1.x*point2.x + point1.y*point2.y;

}
/* returns sqrt(canvas_point_dot_product(point1, point1)) for canvaspoint structures */
amide_real_t canvas_point_mag(const AmitkCanvasPoint point1) {
  return sqrt(canvas_point_dot_product(point1, point1));
}



/* converts a gnome canvas point to a realpoint in the canvas coordinate's frame */
/* volume_corner is the corner of the volume the appropriate canvas is defined on */
/* width and height are of the canvas, in pixels */
AmitkPoint canvas_point_2_point(AmitkPoint volume_corner,
				gint width, gint height,
				gdouble x_offset,gdouble y_offset,
				AmitkCanvasPoint canvas_cpoint) {

  AmitkPoint canvas_point;

  canvas_point.x = ((canvas_cpoint.x-x_offset)/width)*volume_corner.x;
  canvas_point.y = ((height-(canvas_cpoint.y-y_offset))/height)*volume_corner.y;
  canvas_point.z = volume_corner.z/2.0;

  /* make sure it's in the given volume */
  if (canvas_point.x < 0.0) canvas_point.x = 0.0;
  if (canvas_point.y < 0.0) canvas_point.y = 0.0;
  if (canvas_point.x > volume_corner.x) canvas_point.x = volume_corner.x;
  if (canvas_point.y > volume_corner.y) canvas_point.y = volume_corner.y;

  return canvas_point;

}

/* converts a point in the canvas's coordinate space to a gnome canvas event location */
AmitkCanvasPoint point_2_canvas_point(AmitkPoint volume_corner,
				      gint width,gint height,
				      gdouble x_offset, gdouble y_offset,
				      AmitkPoint canvas_point) {

  AmitkCanvasPoint canvas_cpoint;

  canvas_cpoint.x = width * canvas_point.x/volume_corner.x + x_offset;
  canvas_cpoint.y = height * (volume_corner.y - canvas_point.y)/volume_corner.y + y_offset;
  
  return canvas_cpoint;
}


/* returns voxel1+voxel2 for voxelpoint structures */
AmitkVoxel voxel_add(const AmitkVoxel voxel1,const AmitkVoxel voxel2) {
  AmitkVoxel temp;
  temp.x = voxel1.x+voxel2.x;
  temp.y = voxel1.y+voxel2.y;
  temp.z = voxel1.z+voxel2.z;
  temp.g = voxel1.g+voxel2.g;
  temp.t = voxel1.t+voxel2.t;
  return temp;
}

/* returns voxel1-voxel2 for voxelpoint structures */
AmitkVoxel voxel_sub(const AmitkVoxel voxel1,const AmitkVoxel voxel2) {
  AmitkVoxel temp;
  temp.x = voxel1.x-voxel2.x;
  temp.y = voxel1.y-voxel2.y;
  temp.z = voxel1.z-voxel2.z;
  temp.g = voxel1.g-voxel2.g;
  temp.t = voxel1.t-voxel2.t;
  return temp;
}

/* returns voxel1 == voxel2 for voxelpoint structures */
gboolean voxel_equal(const AmitkVoxel voxel1, const AmitkVoxel voxel2) {

  return VOXEL_EQUAL(voxel1, voxel2);
}

/* returns the maximum dimension of the "box" defined by voxel1 */
amide_real_t voxel_max_dim(const AmitkVoxel voxel1) {
  AmitkPoint temp_point;
  VOXEL_TO_POINT(voxel1, one_point, temp_point);
  return point_mag(temp_point);
}

/* little utility function for debugging */
void voxel_print(gchar * message, const AmitkVoxel voxel) {
  g_print("%s\t%d\t%d\t%d\t%d\t%d\n",message, voxel.x, voxel.y, voxel.z, voxel.g, voxel.t);
  return;
}


amide_intpoint_t voxel_get_dim(const AmitkVoxel voxel,
			       const AmitkDim which_dim) {

  switch(which_dim) {
  case AMITK_DIM_X:
    return voxel.x;
    break;
  case AMITK_DIM_Y:
    return voxel.y;
    break;
  case AMITK_DIM_Z:
    return voxel.z;
    break;
  case AMITK_DIM_G:
    return voxel.g;
    break;
  case AMITK_DIM_T:
    return voxel.t;
    break;
  default:
    g_error("inappropriate case in %s at %d\n", __FILE__, __LINE__);
    g_return_val_if_reached(0);
  }

}

void voxel_set_dim(AmitkVoxel * voxel,
		   const AmitkDim which_dim,
		   amide_intpoint_t value) {

  switch(which_dim) {
  case AMITK_DIM_X:
    voxel->x = value;
    break;
  case AMITK_DIM_Y:
    voxel->y = value;
    break;
  case AMITK_DIM_Z:
    voxel->z = value;
    break;
  case AMITK_DIM_G:
    voxel->g = value;
    break;
  case AMITK_DIM_T:
    voxel->t = value;
    break;
  default:
    g_error("inappropriate case in %s at %d\n", __FILE__, __LINE__);
    g_return_if_reached();
  }
}


/* returns true if the realpoint is in the given box */
/* box first corner is zero point */
gboolean point_in_box(const AmitkPoint p,
			     const AmitkPoint box_corner) {


  return (((p.z >= 0.0) && (p.z <= box_corner.z)) &&
	  ((p.y >= 0.0) && (p.y <= box_corner.y)) &&
	  ((p.x >= 0.0) && (p.x <= box_corner.x)));
}


/* returns true if the realpoint is in the elliptic cylinder, 
   cylinder must be inline with the coordinate space center is in 
   note: height is in the z direction, and radius.z isn't used for anything 
*/
gboolean point_in_elliptic_cylinder(const AmitkPoint p,
					   const AmitkPoint center,
					   const amide_real_t height,
					   const AmitkPoint radius) {

  AmitkPoint diff;
  diff.x = p.x-center.x;
  diff.y = p.y-center.y;

  return ((1.0 >= 
	   ((diff.x*diff.x)/(radius.x*radius.x) +
	    (diff.y*diff.y)/(radius.y*radius.y)))
	  &&
	  ((p.z >= (center.z-height/2.0)) && 
	   (p.z <= (center.z+height/2.0))));
  
}



/* returns true if the realpoint is in the ellipsoid */
gboolean point_in_ellipsoid(const AmitkPoint p,
				   const AmitkPoint center,
				   const AmitkPoint radius) {
  
  AmitkPoint diff;
  diff = point_sub(p, center);
  	  
  return (1.0 >= 
	  (diff.x*diff.x)/(radius.x*radius.x) +
	  (diff.y*diff.y)/(radius.y*radius.y) +
	  (diff.z*diff.z)/(radius.z*radius.z));
}


/* little utility function for debugging */
void point_print(gchar * message, const AmitkPoint point) {
  g_print("%s\t%5.3f\t%5.3f\t%5.3f\n",message, point.x, point.y, point.z);
  return;
}




/* rotate the vector on the given vector by the given rotation */
AmitkPoint point_rotate_on_vector(const AmitkPoint in,
				  const AmitkPoint vector,
				  const amide_real_t theta) {

  AmitkPoint return_vector;

  return_vector.x = 
    (vector.x*vector.x + cos(theta) * (1.0 - vector.x*vector.x)) * in.x +
    (vector.x*vector.y*(1.0-cos(theta)) - vector.z * sin(theta)) * in.y +
    (vector.z*vector.x*(1.0-cos(theta)) + vector.y * sin(theta)) * in.z;
  return_vector.y = 
    (vector.x*vector.y*(1.0-cos(theta)) + vector.z * sin(theta)) * in.x +
    (vector.y*vector.y + cos(theta) * (1.0 - vector.y*vector.y)) * in.y +
    (vector.y*vector.z*(1.0-cos(theta)) - vector.x * sin(theta)) * in.z;
  return_vector.z = 
    (vector.z*vector.x*(1.0-cos(theta)) - vector.y * sin(theta)) * in.x +
    (vector.y*vector.z*(1.0-cos(theta)) + vector.x * sin(theta)) * in.y +
    (vector.z*vector.z + cos(theta) * (1.0 - vector.z*vector.z)) * in.z;
  
  return return_vector;
}

amide_real_t point_get_component(const AmitkPoint point,
				 const AmitkAxis which_axis) {

  switch(which_axis) {
  case AMITK_AXIS_X:
    return point.x;
    break;
  case AMITK_AXIS_Y:
    return point.y;
    break;
  case AMITK_AXIS_Z:
    return point.z;
    break;
  default:
    g_return_val_if_reached(0.0);
  }

}
void point_set_component(AmitkPoint * point,
			 const AmitkAxis which_axis,
			 const amide_real_t value) {

  switch(which_axis) {
  case AMITK_AXIS_X:
    point->x = value;
    break;
  case AMITK_AXIS_Y:
    point->y = value;
    break;
  case AMITK_AXIS_Z:
    point->z = value;
    break;
  default:
    g_return_if_reached();
  }

  return;
}


const gchar * amitk_view_get_name(const AmitkView view) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_VIEW);
  enum_value = g_enum_get_value(enum_class, view);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

const gchar * amitk_dim_get_name(const AmitkDim dim) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_DIM);
  enum_value = g_enum_get_value(enum_class, dim);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

const gchar * amitk_axis_get_name(const AmitkAxis axis) {
  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_AXIS);
  enum_value = g_enum_get_value(enum_class, axis);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

const gchar * amitk_length_unit_get_name(const AmitkLengthUnit length_unit) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_LENGTH_UNIT);
  enum_value = g_enum_get_value(enum_class, length_unit);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

