/* amitk_roi_variable_type.c - used to generate the different amitk_roi_*.c files
 *
 * Part of amide - Amide's a Medical Image Data Examiner
 * Copyright (C) 2001-2002 Andy Loening
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
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <glib.h>
#include "amitk_roi_`'m4_Variable_Type`'.h"


#define ROI_TYPE_`'m4_Variable_Type`'






#if defined(ROI_TYPE_CYLINDER) || defined(ROI_TYPE_ELLIPSOID) || defined (ROI_TYPE_BOX)
static GSList * append_intersection_point(GSList * points_list, AmitkPoint new_point);
static GSList * prepend_intersection_point(GSList * points_list, AmitkPoint new_point);






static GSList * append_intersection_point(GSList * points_list, AmitkPoint new_point) {
  AmitkPoint * ppoint;

  ppoint = g_try_new(AmitkPoint,1);
  if (ppoint != NULL) {
    *ppoint = new_point;
    return  g_slist_append(points_list, ppoint);
  } else {
    g_warning("Out of Memory");
    return points_list;
  }
}
static GSList * prepend_intersection_point(GSList * points_list, AmitkPoint new_point) {
  AmitkPoint * ppoint;

  ppoint = g_try_new(AmitkPoint,1);
  if (ppoint != NULL) {
    *ppoint = new_point;
    return  g_slist_prepend(points_list, ppoint);
  } else {
    g_warning("Out of Memory");
    return points_list;
  }
}

/* returns a singly linked list of intersection points between the roi
   and the given canvas slice.  returned points are in the canvas's coordinate space.
*/
GSList * amitk_roi_`'m4_Variable_Type`'_get_intersection_line(const AmitkRoi * roi, 
							      const AmitkVolume * canvas_slice,
							      const amide_real_t pixel_dim) {


  GSList * return_points = NULL;
  AmitkPoint view_point,temp_point;
  AmitkCorners slice_corners;
  AmitkPoint canvas_corner;
  AmitkPoint * temp_pointp;
  AmitkVoxel i;
  AmitkVoxel canvas_dim;
  gboolean voxel_in=FALSE, prev_voxel_intersection, saved=TRUE;
#if defined(ROI_TYPE_ELLIPSOID) || defined(ROI_TYPE_CYLINDER)
  AmitkPoint center, radius;
#endif
#if defined(ROI_TYPE_CYLINDER)
  amide_real_t height;
#endif

  /* make sure we've already defined this guy */
  g_return_val_if_fail(!AMITK_ROI_UNDRAWN(roi), NULL);

#ifdef AMIDE_DEBUG
  //  g_print("roi %s --------------------\n",AMITK_OBJECT_NAME(roi));
  //  point_print("\t\tcorner", AMITK_VOLUME_CORNER(roi));
#endif

#if defined(ROI_TYPE_ELLIPSOID) || defined(ROI_TYPE_CYLINDER)
  radius = point_cmult(0.5, AMITK_VOLUME_CORNER(roi));
  center = amitk_space_b2s(AMITK_SPACE(roi), amitk_volume_get_center(AMITK_VOLUME(roi)));
#endif
#ifdef ROI_TYPE_CYLINDER
  height = AMITK_VOLUME_Z_CORNER(roi);
#endif

  /* get the corners of the canvas slice */
  canvas_corner = AMITK_VOLUME_CORNER(canvas_slice);
  slice_corners[0] = amitk_space_b2s(AMITK_SPACE(canvas_slice), 
				     AMITK_SPACE_OFFSET(canvas_slice));
  slice_corners[1] = canvas_corner;

  /* iterate through the slice, putting all edge points in the list */
  i.t = i.z=0;
  view_point.z = (slice_corners[0].z+slice_corners[1].z)/2.0;
  view_point.y = slice_corners[0].y+pixel_dim/2.0;

  canvas_dim.t = 0;
  canvas_dim.z = ceil(canvas_corner.z/AMITK_VOLUME_Z_CORNER(canvas_slice));
  canvas_dim.y = ceil(canvas_corner.y/pixel_dim);
  canvas_dim.x = ceil(canvas_corner.x/pixel_dim);
  g_return_val_if_fail(canvas_dim.z == 1, NULL);

  for (i.y=0; i.y < canvas_dim.y ; i.y++) {

    view_point.x = slice_corners[0].x+pixel_dim/2.0;
    prev_voxel_intersection = FALSE;

    for (i.x=0; i.x < canvas_dim.x ; i.x++) {
      temp_point = amitk_space_s2s(AMITK_SPACE(canvas_slice), AMITK_SPACE(roi), view_point);
      
#ifdef ROI_TYPE_BOX
      voxel_in = point_in_box(temp_point, AMITK_VOLUME_CORNER(roi));
#endif
#ifdef ROI_TYPE_CYLINDER
      voxel_in = point_in_elliptic_cylinder(temp_point, center, height, radius);
#endif
#ifdef ROI_TYPE_ELLIPSOID
      voxel_in = point_in_ellipsoid(temp_point,center,radius);
#endif
	
      /* is it an edge */
      if (voxel_in != prev_voxel_intersection) {

	/* than save the point */
	saved = TRUE;

	temp_point = view_point;
	if (voxel_in ) {
	  return_points = prepend_intersection_point(return_points, temp_point);
	} else { /* previous voxel should be returned */
	  temp_point.x = view_point.x - pixel_dim; /* backup one voxel */
	  return_points = append_intersection_point(return_points, temp_point);
	}
      } else {
	saved = FALSE;
      }
      
      prev_voxel_intersection = voxel_in;
      view_point.x += pixel_dim; /* advance one voxel */
    }
    
    /* check if the edge of this row is still in the roi, if it is, add it as
       a point */
    if ((voxel_in) && !(saved))
      return_points = append_intersection_point(return_points, view_point);

    view_point.y += pixel_dim; /* advance one row */
  }
  
  /* make sure the two ends meet */
  temp_pointp = g_slist_nth_data(return_points, 0);
  if (return_points != NULL)
    return_points = append_intersection_point(return_points, *temp_pointp);
					      
  
  return return_points;
}

#endif


#if defined(ROI_TYPE_ISOCONTOUR_2D) || defined(ROI_TYPE_ISOCONTOUR_3D)
static amitk_format_UBYTE_t isocontour_edge(AmitkRawData * isocontour_ds, AmitkVoxel voxel);
static void isocontour_consider(const AmitkDataSet * ds,
				const AmitkRawData * temp_rd, 
				AmitkVoxel * p_ds_voxel, 
				const amide_data_t iso_value);




/* we return the intersection data in the form of a volume slice because it's convenient */
AmitkDataSet * amitk_roi_`'m4_Variable_Type`'_get_intersection_slice(const AmitkRoi * roi,
								     const AmitkVolume * canvas_slice,
								     const amide_real_t pixel_dim) {



  AmitkVoxel i_voxel;
  AmitkVoxel roi_voxel;
  AmitkVoxel start, end, dim;
  AmitkPoint view_point;
  AmitkPoint roi_point;
  AmitkCorners slice_corners;
  AmitkCorners intersection_corners;
  AmitkDataSet * intersection;
  AmitkPoint temp_point;
  AmitkPoint canvas_voxel_size;
  amitk_format_UBYTE_t value;

  g_return_val_if_fail(!AMITK_ROI_UNDRAWN(roi), NULL);

  /* figure out the intersection between the canvas slice and the roi */
  if (!amitk_volume_volume_intersection_corners(canvas_slice, 
						AMITK_VOLUME(roi), 
						intersection_corners))
    return NULL; /* no intersection */

  /* translate the intersection into voxel space */
  canvas_voxel_size.x = canvas_voxel_size.y = pixel_dim;
  canvas_voxel_size.z = AMITK_VOLUME_Z_CORNER(canvas_slice);
  POINT_TO_VOXEL(intersection_corners[0], canvas_voxel_size, 0, start);
  POINT_TO_VOXEL(intersection_corners[1], canvas_voxel_size, 0, end);
  dim = voxel_add(voxel_sub(end, start), one_voxel);

  intersection = amitk_data_set_new();
  intersection->raw_data = amitk_raw_data_UBYTE_2D_init(0, dim.y, dim.x);

  /* get the corners of the view slice in view coordinate space */
  slice_corners[0] = amitk_space_b2s(AMITK_SPACE(canvas_slice), 
				     AMITK_SPACE_OFFSET(canvas_slice));
  slice_corners[1] = AMITK_VOLUME_CORNER(canvas_slice);

  i_voxel.z = i_voxel.t = 0;
  view_point.z = (slice_corners[0].z+slice_corners[1].z)/2.0;
  view_point.y = slice_corners[0].y+((double) start.y + 0.5)*pixel_dim;

  for (i_voxel.y=0; i_voxel.y<dim.y; i_voxel.y++) {
    view_point.x = slice_corners[0].x+((double)start.x+0.5)*pixel_dim;

    for (i_voxel.x=0; i_voxel.x<dim.x; i_voxel.x++) {
      roi_point = amitk_space_s2s(AMITK_SPACE(canvas_slice), AMITK_SPACE(roi), view_point);
      POINT_TO_VOXEL(roi_point, roi->voxel_size, 0, roi_voxel);
      if (amitk_raw_data_includes_voxel(roi->isocontour, roi_voxel)) {
	value = AMITK_RAW_DATA_UBYTE_CONTENT(roi->isocontour, roi_voxel);
#ifdef ROI_TYPE_ISOCONTOUR_2D
	if (value > 0)
#else
	if ( value == 1)
#endif
	  AMITK_RAW_DATA_UBYTE_SET_CONTENT(intersection->raw_data, i_voxel) = 1;
      }

      view_point.x += pixel_dim;
    }
    view_point.y += pixel_dim;
  }

#ifdef ROI_TYPE_ISOCONTOUR_2D
  /* mark the edges as such on the 2D isocontour slices */
  i_voxel.z = i_voxel.t = 0;
  for (i_voxel.y=0; i_voxel.y<dim.y; i_voxel.y++) 
    for (i_voxel.x=0; i_voxel.x<dim.x; i_voxel.x++) 
      if (AMITK_RAW_DATA_UBYTE_CONTENT(intersection->raw_data, i_voxel)) 
	AMITK_RAW_DATA_UBYTE_SET_CONTENT(intersection->raw_data, i_voxel) =
	  isocontour_edge(intersection->raw_data, i_voxel);
#endif

  amitk_space_copy_in_place(AMITK_SPACE(intersection), AMITK_SPACE(canvas_slice));
  intersection->voxel_size = canvas_voxel_size;

  POINT_MULT(start, intersection->voxel_size, temp_point);
  amitk_space_set_offset(AMITK_SPACE(intersection),
			 amitk_space_s2b(AMITK_SPACE(canvas_slice), temp_point));

  temp_point = AMITK_SPACE_OFFSET(intersection);

  POINT_MULT(intersection->raw_data->dim, intersection->voxel_size, 
	     AMITK_VOLUME_CORNER(intersection));
  
  return intersection;
}


/* returns 0 for something not in the roi, returns 1 for an edge, and 2 for something in the roi */
static amitk_format_UBYTE_t isocontour_edge(AmitkRawData * isocontour_ds, AmitkVoxel voxel) {

  amitk_format_UBYTE_t edge_value=0;
  AmitkVoxel new_voxel;
#ifdef ROI_TYPE_ISOCONTOUR_3D
  gint z;
#endif

#ifdef ROI_TYPE_ISOCONTOUR_3D
  for (z=-1; z<=1; z++) {
#endif

    new_voxel = voxel;
#ifdef ROI_TYPE_ISOCONTOUR_3D
    new_voxel.z += z;
    if (z != 0) /* don't reconsider the original point */
      if (amitk_raw_data_includes_voxel(isocontour_ds, new_voxel))
	if (AMITK_RAW_DATA_UBYTE_CONTENT(isocontour_ds,new_voxel) != 0)
	  edge_value++;
#endif
    new_voxel.x--;
    if (amitk_raw_data_includes_voxel(isocontour_ds, new_voxel))
      if (AMITK_RAW_DATA_UBYTE_CONTENT(isocontour_ds,new_voxel) != 0)
	edge_value++;

    new_voxel.y--;
    if (amitk_raw_data_includes_voxel(isocontour_ds, new_voxel))
      if (AMITK_RAW_DATA_UBYTE_CONTENT(isocontour_ds,new_voxel) != 0)
	edge_value++;

    new_voxel.x++;
    if (amitk_raw_data_includes_voxel(isocontour_ds, new_voxel))
      if (AMITK_RAW_DATA_UBYTE_CONTENT(isocontour_ds,new_voxel) != 0)
	edge_value++;

    new_voxel.x++;
    if (amitk_raw_data_includes_voxel(isocontour_ds, new_voxel))
      if (AMITK_RAW_DATA_UBYTE_CONTENT(isocontour_ds,new_voxel) != 0)
	edge_value++;

    new_voxel.y++;
    if (amitk_raw_data_includes_voxel(isocontour_ds, new_voxel))
      if (AMITK_RAW_DATA_UBYTE_CONTENT(isocontour_ds,new_voxel) != 0)
	edge_value++;

    new_voxel.y++;
    if (amitk_raw_data_includes_voxel(isocontour_ds, new_voxel))
      if (AMITK_RAW_DATA_UBYTE_CONTENT(isocontour_ds,new_voxel) != 0)
	edge_value++;

    new_voxel.x--;
    if (amitk_raw_data_includes_voxel(isocontour_ds, new_voxel))
      if (AMITK_RAW_DATA_UBYTE_CONTENT(isocontour_ds,new_voxel) != 0)
	edge_value++;

    new_voxel.x--;
    if (amitk_raw_data_includes_voxel(isocontour_ds, new_voxel))
      if (AMITK_RAW_DATA_UBYTE_CONTENT(isocontour_ds,new_voxel) != 0)
	edge_value++;

#ifdef ROI_TYPE_ISOCONTOUR_3D
  }
#endif

#ifdef ROI_TYPE_ISOCONTOUR_3D
  if (edge_value == 26 )
    edge_value = 2;
  else
    edge_value = 1;
#else /* ROI_TYPE_ISOCONTOUR_2D */
  if (edge_value == 8 )
    edge_value = 2;
  else
    edge_value = 1;
#endif

  return edge_value;
}

/* note, using pointers for ds_voxel and iso_value to try to reduce
   stack size */
static void isocontour_consider(const AmitkDataSet * ds,
				const AmitkRawData * temp_rd, 
				AmitkVoxel * p_ds_voxel, 
				const amide_data_t iso_value) {
#ifdef ROI_TYPE_ISOCONTOUR_3D
  gint z;
#endif

  { /* don't want roi_voxel allocated on the stack */
    AmitkVoxel roi_voxel;

    roi_voxel = *p_ds_voxel;
    roi_voxel.t = 0;
    if (!amitk_raw_data_includes_voxel(temp_rd, roi_voxel))
      return; /* make sure we're still in the data set */

    if (AMITK_RAW_DATA_UBYTE_CONTENT(temp_rd,roi_voxel) == 1)
      return; /* have we already considered this voxel */
  
    if ((amitk_data_set_get_value(ds, *p_ds_voxel) < iso_value)) return;
    else AMITK_RAW_DATA_UBYTE_SET_CONTENT(temp_rd,roi_voxel)=1; /* it's in */
  }

  /* consider the 8 adjoining voxels, or 26 in the case of 3D */
#ifdef ROI_TYPE_ISOCONTOUR_3D
  for (z=-1; z<=1; z++) { 
    p_ds_voxel->z += z;
    if (z != 0) /* don't reconsider the original point */
      isocontour_consider(ds, temp_rd, p_ds_voxel, iso_value);
#endif
    p_ds_voxel->x--;
    isocontour_consider(ds, temp_rd, p_ds_voxel, iso_value);
    p_ds_voxel->y--;
    isocontour_consider(ds, temp_rd, p_ds_voxel, iso_value);
    p_ds_voxel->x++;
    isocontour_consider(ds, temp_rd, p_ds_voxel, iso_value);
    p_ds_voxel->x++;
    isocontour_consider(ds, temp_rd, p_ds_voxel, iso_value);
    p_ds_voxel->y++;
    isocontour_consider(ds, temp_rd, p_ds_voxel, iso_value);
    p_ds_voxel->y++;
    isocontour_consider(ds, temp_rd, p_ds_voxel, iso_value);
    p_ds_voxel->x--;
    isocontour_consider(ds, temp_rd, p_ds_voxel, iso_value);
    p_ds_voxel->x--;
    isocontour_consider(ds, temp_rd, p_ds_voxel, iso_value);
    p_ds_voxel->y--;
    p_ds_voxel->x++;
#ifdef ROI_TYPE_ISOCONTOUR_3D
    p_ds_voxel->z -= z;
  }
#endif

  return;
}

void amitk_roi_`'m4_Variable_Type`'_set_isocontour(AmitkRoi * roi, AmitkDataSet * ds, 
						   AmitkVoxel iso_voxel) {

  AmitkRawData * temp_rd;
  AmitkPoint temp_point;
  AmitkVoxel min_voxel, max_voxel, i_voxel, roi_voxel;
  rlim_t prev_stack_limit;
  struct rlimit rlim;
  amide_data_t isocontour_value;

  g_return_if_fail(roi->type == AMITK_ROI_TYPE_`'m4_Variable_Type`');

  roi->isocontour_value = amitk_data_set_get_value(ds, iso_voxel); /* what we're setting the isocontour too */

  /* we first make a raw data set the size of the data set to record the in/out values */
#if defined(ROI_TYPE_ISOCONTOUR_2D)
  temp_rd = amitk_raw_data_UBYTE_2D_init(0, ds->raw_data->dim.y, ds->raw_data->dim.x);
#elif defined(ROI_TYPE_ISOCONTOUR_3D)
  temp_rd = amitk_raw_data_UBYTE_3D_init(0, ds->raw_data->dim.z, ds->raw_data->dim.y, ds->raw_data->dim.x);
#endif

  /* remove any limitation to the stack size, this is so we can recurse deeply without
     seg faulting */
  getrlimit(RLIMIT_STACK, &rlim); 
  prev_stack_limit = rlim.rlim_cur;
  if (rlim.rlim_cur != rlim.rlim_max) {
    rlim.rlim_cur = rlim.rlim_max;
    setrlimit(RLIMIT_STACK, &rlim);
  }

  /* fill in the data set */
  isocontour_value = roi->isocontour_value-EPSILON*fabs(roi->isocontour_value); /* epsilon guards for floating point rounding */
  isocontour_consider(ds, temp_rd, &iso_voxel, isocontour_value);
  
  /* the selected point is automatically in */
  roi_voxel = iso_voxel;
  roi_voxel.t = 0;
  if (AMITK_RAW_DATA_UBYTE_CONTENT(temp_rd, roi_voxel) != 1) {
    g_warning("floating point round error detected in %s at %d", __FILE__, __LINE__);
    AMITK_RAW_DATA_UBYTE_SET_CONTENT(temp_rd,roi_voxel)=1; /* have at least one voxel picked */
  }


  /* figure out the min and max dimensions */
  min_voxel = max_voxel = iso_voxel;
  
  i_voxel.t = 0;
  for (i_voxel.z=0; i_voxel.z < temp_rd->dim.z; i_voxel.z++) {
    for (i_voxel.y=0; i_voxel.y < temp_rd->dim.y; i_voxel.y++) {
      for (i_voxel.x=0; i_voxel.x < temp_rd->dim.x; i_voxel.x++) {
	if (AMITK_RAW_DATA_UBYTE_CONTENT(temp_rd, i_voxel)==1) {
	  if (min_voxel.x > i_voxel.x) min_voxel.x = i_voxel.x;
	  if (max_voxel.x < i_voxel.x) max_voxel.x = i_voxel.x;
	  if (min_voxel.y > i_voxel.y) min_voxel.y = i_voxel.y;
	  if (max_voxel.y < i_voxel.y) max_voxel.y = i_voxel.y;
#ifdef ROI_TYPE_ISOCONTOUR_3D
	  if (min_voxel.z > i_voxel.z) min_voxel.z = i_voxel.z;
	  if (max_voxel.z < i_voxel.z) max_voxel.z = i_voxel.z;
#endif
	}
      }
    }
  }

  /* transfer the subset of the data set that contains positive information */
  if (roi->isocontour != NULL)
    g_object_unref(roi->isocontour);
#if defined(ROI_TYPE_ISOCONTOUR_2D)
  roi->isocontour = amitk_raw_data_UBYTE_2D_init(0, max_voxel.y-min_voxel.y+1, max_voxel.x-min_voxel.x+1);
#elif defined(ROI_TYPE_ISOCONTOUR_3D)
  roi->isocontour = amitk_raw_data_UBYTE_3D_init(0,max_voxel.z-min_voxel.z+1,
						 max_voxel.y-min_voxel.y+1, 
						 max_voxel.x-min_voxel.x+1);
#endif

  i_voxel.t = 0;
  for (i_voxel.z=0; i_voxel.z<roi->isocontour->dim.z; i_voxel.z++)
    for (i_voxel.y=0; i_voxel.y<roi->isocontour->dim.y; i_voxel.y++)
      for (i_voxel.x=0; i_voxel.x<roi->isocontour->dim.x; i_voxel.x++) {
#if defined(ROI_TYPE_ISOCONTOUR_2D)
	AMITK_RAW_DATA_UBYTE_SET_CONTENT(roi->isocontour, i_voxel) =
	  AMITK_RAW_DATA_UBYTE_2D_CONTENT(temp_rd, i_voxel.y+min_voxel.y, i_voxel.x+min_voxel.x);
#elif defined(ROI_TYPE_ISOCONTOUR_3D)
	AMITK_RAW_DATA_UBYTE_SET_CONTENT(roi->isocontour, i_voxel) =
	  AMITK_RAW_DATA_UBYTE_3D_CONTENT(temp_rd, i_voxel.z+min_voxel.z,i_voxel.y+min_voxel.y, i_voxel.x+min_voxel.x);
#endif
      }

  g_object_unref(temp_rd);

  /* mark the edges as such */
  i_voxel.t = 0;
  for (i_voxel.z=0; i_voxel.z<roi->isocontour->dim.z; i_voxel.z++)
    for (i_voxel.y=0; i_voxel.y<roi->isocontour->dim.y; i_voxel.y++) 
      for (i_voxel.x=0; i_voxel.x<roi->isocontour->dim.x; i_voxel.x++) 
	if (AMITK_RAW_DATA_UBYTE_CONTENT(roi->isocontour, i_voxel)) 
	  AMITK_RAW_DATA_UBYTE_SET_CONTENT(roi->isocontour, i_voxel) =
	    isocontour_edge(roi->isocontour, i_voxel);

  /* and set the rest of the important info for the data set */
  amitk_space_copy_in_place(AMITK_SPACE(roi), AMITK_SPACE(ds));
  roi->voxel_size = ds->voxel_size;

  POINT_MULT(min_voxel, ds->voxel_size, temp_point);
  temp_point = amitk_space_s2b(AMITK_SPACE(ds), temp_point);
  amitk_space_set_offset(AMITK_SPACE(roi), temp_point);

  POINT_MULT(roi->isocontour->dim, roi->voxel_size, temp_point);
  amitk_volume_set_corner(AMITK_VOLUME(roi), temp_point);

  /* reset our previous stack limit */
  if (prev_stack_limit != rlim.rlim_cur) {
    rlim.rlim_cur = prev_stack_limit;
    setrlimit(RLIMIT_STACK, &rlim);
  }

  return;

}

void amitk_roi_`'m4_Variable_Type`'_erase_area(AmitkRoi * roi, AmitkVoxel erase_voxel, gint area_size) {

  AmitkVoxel i_voxel;

#ifdef ROI_TYPE_ISOCONTOUR_3D
  for (i_voxel.z = erase_voxel.z-area_size; i_voxel.z <= erase_voxel.z+area_size; i_voxel.z++) 
#endif
    for (i_voxel.y = erase_voxel.y-area_size; i_voxel.y <= erase_voxel.y+area_size; i_voxel.y++) 
      for (i_voxel.x = erase_voxel.x-area_size; i_voxel.x <= erase_voxel.x+area_size; i_voxel.x++) 
	if (amitk_raw_data_includes_voxel(roi->isocontour, i_voxel))
	  AMITK_RAW_DATA_UBYTE_SET_CONTENT(roi->isocontour,i_voxel)=0;

  /* re edge the neighboring points */
  i_voxel.t = i_voxel.z = 0;
#ifdef ROI_TYPE_ISOCONTOUR_3D
  for (i_voxel.z = erase_voxel.z-1-area_size; i_voxel.z <= erase_voxel.z+1+area_size; i_voxel.z++) 
#endif
    for (i_voxel.y = erase_voxel.y-1-area_size; i_voxel.y <= erase_voxel.y+1+area_size; i_voxel.y++) 
      for (i_voxel.x = erase_voxel.x-1-area_size; i_voxel.x <= erase_voxel.x+1+area_size; i_voxel.x++) 
	if (amitk_raw_data_includes_voxel(roi->isocontour, i_voxel)) 
	  if (AMITK_RAW_DATA_UBYTE_CONTENT(roi->isocontour, i_voxel)) 
	    AMITK_RAW_DATA_UBYTE_SET_CONTENT(roi->isocontour, i_voxel) =
	      isocontour_edge(roi->isocontour, i_voxel);

  return;
}
#endif








/* iterates over the voxels in the given data set that are inside the given roi,
   and performs the specified calculation function for those points */
/* calulation should be a function taking the following arguments:
   calculation(AmitkVoxel voxel, amide_data_t value, amide_real_t voxel_fraction, gpointer data) */
void amitk_roi_`'m4_Variable_Type`'_calculate_on_data_set(const AmitkRoi * roi,  
							  const AmitkDataSet * ds, 
							  const guint frame,
							  const gboolean inverse,
							  void (* calculation)(),
							  gpointer data) {
  AmitkPoint roi_pt, fine_ds_pt, far_ds_pt;
  amide_data_t value;
  amide_real_t voxel_fraction;
  AmitkVoxel i,j, k;
  AmitkVoxel start, dim;
  gboolean voxel_in;
  AmitkRawData * next_plane_in;
  AmitkRawData * curr_plane_in;
  gboolean small_dimensions;
  AmitkCorners intersection_corners;
  AmitkPoint ds_voxel_size;
  AmitkPoint sub_voxel_size;
  amide_real_t grain_size;

#if defined (ROI_TYPE_BOX)
  AmitkPoint box_corner;
  box_corner = AMITK_VOLUME_CORNER(roi);
#endif

#if defined(ROI_TYPE_ELLIPSOID) || defined(ROI_TYPE_CYLINDER)
  AmitkPoint center;
  AmitkPoint radius;
#if defined(ROI_TYPE_CYLINDER)
  amide_real_t height;
  height = AMITK_VOLUME_Z_CORNER(roi);
#endif  
  center = amitk_space_b2s(AMITK_SPACE(roi), amitk_volume_get_center(AMITK_VOLUME(roi)));
  radius = point_cmult(0.5, AMITK_VOLUME_CORNER(roi));
#endif

#if defined(ROI_TYPE_ISOCONTOUR_2D) || defined(ROI_TYPE_ISOCONTOUR_3D)
  AmitkPoint roi_voxel_size;
  AmitkVoxel roi_voxel;

  roi_voxel_size = AMITK_ROI_VOXEL_SIZE(roi);
#endif

  ds_voxel_size = AMITK_DATA_SET_VOXEL_SIZE(ds);
  sub_voxel_size = point_cmult(1.0/AMITK_ROI_GRANULARITY, ds_voxel_size);

  grain_size = 1.0/(AMITK_ROI_GRANULARITY*AMITK_ROI_GRANULARITY*AMITK_ROI_GRANULARITY);

  /* figure out the intersection between the data set and the roi */
  if (inverse) {
    start = zero_voxel;
    dim = AMITK_DATA_SET_DIM(ds);
  } else {
    if (!amitk_volume_volume_intersection_corners(AMITK_VOLUME(ds),  AMITK_VOLUME(roi), 
						  intersection_corners)) {
      dim = zero_voxel; /* no intersection */
      start = zero_voxel;
    } else {
      /* translate the intersection into voxel space */
      POINT_TO_VOXEL(intersection_corners[0], ds_voxel_size, 0, start);
      POINT_TO_VOXEL(intersection_corners[1], ds_voxel_size, 0, dim);
      dim = voxel_add(voxel_sub(dim, start), one_voxel);
    }
  }

  /* check if we're done already */
  if ((dim.x == 0) || (dim.y == 0) || (dim.z == 0)) {
    return;
  }

  /* if we have any small dimensions, make sure we always iterate over sub voxels */
  small_dimensions = FALSE;
  if ((dim.x == 1) || (dim.y == 1) || (dim.z == 1))
    small_dimensions = TRUE;

  /* over-iterate, as our initial edges will always be considered out of the roi */
  start.x -= 1;
  start.y -= 1;
  start.z -= 1;
  dim.x += 1;
  dim.y += 1;
  dim.z += 1;

  /* start and dim specify (in the data set's voxel space) the voxels in 
     the volume we should be iterating over */

  /* these two arrays are used to store whether the voxel vertices are in the ROI or not */
  next_plane_in = amitk_raw_data_UBYTE_2D_init(0, dim.y+1, dim.x+1);
  curr_plane_in = amitk_raw_data_UBYTE_2D_init(0, dim.y+1, dim.x+1);

  j.t = frame;
  i.t = k.t = 0;
  for (i.z = 0; i.z < dim.z; i.z++) {
    j.z = i.z+start.z;
    far_ds_pt.z = (j.z+1)*ds_voxel_size.z;
    
    for (i.y = 0; i.y < dim.y; i.y++) {
      j.y = i.y+start.y;
      far_ds_pt.y = (j.y+1)*ds_voxel_size.y;
      
      for (i.x = 0; i.x < dim.x; i.x++) {
	j.x = i.x+start.x;
	far_ds_pt.x = (j.x+1)*ds_voxel_size.x;
	
	/* figure out if the next far corner is in the roi or not */
	/* get the corresponding roi point */
	roi_pt = amitk_space_s2s(AMITK_SPACE(ds),  AMITK_SPACE(roi), far_ds_pt);

	/* calculate the one corner of the voxel "box" to determine if it's in or not */
#if defined (ROI_TYPE_BOX)
	voxel_in = point_in_box(roi_pt, box_corner);
#endif
#if defined(ROI_TYPE_CYLINDER)
	voxel_in = point_in_elliptic_cylinder(roi_pt, center, height, radius);
#endif
#if defined(ROI_TYPE_ELLIPSOID)
	voxel_in = point_in_ellipsoid(roi_pt,center,radius);
#endif
#if defined(ROI_TYPE_ISOCONTOUR_2D) || defined(ROI_TYPE_ISOCONTOUR_3D)
	POINT_TO_VOXEL(roi_pt, roi_voxel_size, 0, roi_voxel);
	if (!amitk_raw_data_includes_voxel(roi->isocontour, roi_voxel)) voxel_in = FALSE;
	else if (AMITK_RAW_DATA_UBYTE_CONTENT(roi->isocontour, roi_voxel) == 0) voxel_in = FALSE;
	else voxel_in = TRUE;
#endif

	AMITK_RAW_DATA_UBYTE_2D_SET_CONTENT(next_plane_in,i.y+1,i.x+1)=voxel_in;

	if (AMITK_RAW_DATA_UBYTE_2D_CONTENT(curr_plane_in,i.y,i.x) &&
	    AMITK_RAW_DATA_UBYTE_2D_CONTENT(curr_plane_in,i.y,i.x+1) &&
	    AMITK_RAW_DATA_UBYTE_2D_CONTENT(curr_plane_in,i.y+1,i.x) &&
	    AMITK_RAW_DATA_UBYTE_2D_CONTENT(curr_plane_in,i.y+1,i.x+1) &&
	    AMITK_RAW_DATA_UBYTE_2D_CONTENT(next_plane_in,i.y,i.x) &&
	    AMITK_RAW_DATA_UBYTE_2D_CONTENT(next_plane_in,i.y,i.x+1) &&
	    AMITK_RAW_DATA_UBYTE_2D_CONTENT(next_plane_in,i.y+1,i.x) &&
	    AMITK_RAW_DATA_UBYTE_2D_CONTENT(next_plane_in,i.y+1,i.x+1)) {
	  /* this voxel is entirely in the ROI */

	  if (!inverse) {
	    value = amitk_data_set_get_value(ds,j);
	    (*calculation)(j, value, 1.0, data);
	  }

	} else if (AMITK_RAW_DATA_UBYTE_2D_CONTENT(curr_plane_in,i.y,i.x) ||
		   AMITK_RAW_DATA_UBYTE_2D_CONTENT(curr_plane_in,i.y,i.x+1) ||
		   AMITK_RAW_DATA_UBYTE_2D_CONTENT(curr_plane_in,i.y+1,i.x) ||
		   AMITK_RAW_DATA_UBYTE_2D_CONTENT(curr_plane_in,i.y+1,i.x+1) ||
		   AMITK_RAW_DATA_UBYTE_2D_CONTENT(next_plane_in,i.y,i.x) ||
		   AMITK_RAW_DATA_UBYTE_2D_CONTENT(next_plane_in,i.y,i.x+1) ||
		   AMITK_RAW_DATA_UBYTE_2D_CONTENT(next_plane_in,i.y+1,i.x) ||
		   AMITK_RAW_DATA_UBYTE_2D_CONTENT(next_plane_in,i.y+1,i.x+1) ||
		   small_dimensions) {
	  /* this voxel is partially in the ROI, will need to do subvoxel analysis */

	  value = amitk_data_set_get_value(ds,j);
	  voxel_fraction=0;

	  for (k.z = 0;k.z<AMITK_ROI_GRANULARITY;k.z++) {
	    fine_ds_pt.z = j.z*ds_voxel_size.z+ (k.z+0.5)*sub_voxel_size.z;

	    for (k.y = 0;k.y<AMITK_ROI_GRANULARITY;k.y++) {
	      fine_ds_pt.y = j.y*ds_voxel_size.y+ (k.y+0.5)*sub_voxel_size.y;

	      for (k.x = 0;k.x<AMITK_ROI_GRANULARITY;k.x++) {
		fine_ds_pt.x = j.x*ds_voxel_size.x+(k.x+0.5)*sub_voxel_size.x;
		
		roi_pt = amitk_space_s2s(AMITK_SPACE(ds), AMITK_SPACE(roi), fine_ds_pt);

		/* calculate the one corner of the voxel "box" to determine if it's in or not */
#if defined (ROI_TYPE_BOX)
		voxel_in = point_in_box(roi_pt, box_corner);
#endif
#if defined(ROI_TYPE_CYLINDER)
		voxel_in = point_in_elliptic_cylinder(roi_pt, center, height, radius);
#endif
#if defined(ROI_TYPE_ELLIPSOID)
		voxel_in = point_in_ellipsoid(roi_pt,center,radius);
#endif
#if defined(ROI_TYPE_ISOCONTOUR_2D) || defined(ROI_TYPE_ISOCONTOUR_3D)
		POINT_TO_VOXEL(roi_pt, roi_voxel_size, 0, roi_voxel);
		if (!amitk_raw_data_includes_voxel(roi->isocontour, roi_voxel)) voxel_in = FALSE;
		else if (AMITK_RAW_DATA_UBYTE_CONTENT(roi->isocontour, roi_voxel) == 0) voxel_in = FALSE;
		else voxel_in = TRUE;
#endif
		if (voxel_in) {
		  voxel_fraction += grain_size;
		} /* voxel_in */
	      } /* k.x loop */
	    } /* k.y loop */
	  } /* k.z loop */

	  if (!inverse) {
	    (*calculation)(j, value, voxel_fraction, data);
	  } else {
	    (*calculation)(j, value, 1.0-voxel_fraction, data);
	  }

	} else { /* this voxel is outside the ROI, do nothing*/
	  if (inverse) {
	    value = amitk_data_set_get_value(ds,j);
	    (*calculation)(j, value, 1.0, data);
	  }
	}
      } /* i.x loop */
    } /* i.y loop */
    
    /* need to copy over the info on which voxel corners are in the roi */
    for (k.y=0;k.y < next_plane_in->dim.y; k.y++)
      for (k.x=0;k.x < next_plane_in->dim.x; k.x++)
	AMITK_RAW_DATA_UBYTE_2D_SET_CONTENT(curr_plane_in,k.y,k.x) = 
	  AMITK_RAW_DATA_UBYTE_2D_CONTENT(next_plane_in,k.y,k.x);
  } /* i.z loop */

  /* trash collection */
  g_object_unref(curr_plane_in);
  g_object_unref(next_plane_in);


  return;
}



