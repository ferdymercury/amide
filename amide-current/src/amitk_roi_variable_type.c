/* amitk_roi_variable_type.c - used to generate the different amitk_roi_*.c files
 *
 * Part of amide - Amide's a Medical Image Data Examiner
 * Copyright (C) 2001-2014 Andy Loening
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
#include <sys/types.h>
#include <sys/time.h>
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
    g_warning(_("Out of Memory"));
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
    g_warning(_("Out of Memory"));
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
  i.t = i.g = i.z=0;
  view_point.z = (slice_corners[0].z+slice_corners[1].z)/2.0;
  view_point.y = slice_corners[0].y+pixel_dim/2.0;

  canvas_dim.t = 0;
  canvas_dim.z = ceil((canvas_corner.z)/AMITK_VOLUME_Z_CORNER(canvas_slice));
  canvas_dim.y = ceil((canvas_corner.y)/pixel_dim);
  canvas_dim.x = ceil((canvas_corner.x)/pixel_dim);
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


#if defined(ROI_TYPE_ISOCONTOUR_2D) || defined(ROI_TYPE_ISOCONTOUR_3D) || defined(ROI_TYPE_FREEHAND_2D) || defined(ROI_TYPE_FREEHAND_3D)


/* returns 0 for something not in the roi, returns 1 for an edge, and 2 for something in the roi */
static amitk_format_UBYTE_t map_roi_edge(AmitkRawData * map_roi_ds, AmitkVoxel voxel) {

  amitk_format_UBYTE_t edge_value=0;
  AmitkVoxel new_voxel;
#if defined(ROI_TYPE_ISOCONTOUR_3D) || defined(ROI_TYPE_FREEHAND_3D)
  gint z;
#endif

#if defined(ROI_TYPE_ISOCONTOUR_3D) || defined(ROI_TYPE_FREEHAND_3D)
  for (z=-1; z<=1; z++) {
#endif

    new_voxel = voxel;
#if defined(ROI_TYPE_ISOCONTOUR_3D) || defined(ROI_TYPE_FREEHAND_3D)
    new_voxel.z += z;
    if (z != 0) /* don't reconsider the original point */
      if (amitk_raw_data_includes_voxel(map_roi_ds, new_voxel))
	if (AMITK_RAW_DATA_UBYTE_CONTENT(map_roi_ds,new_voxel) != 0)
	  edge_value++;
#endif
    new_voxel.x--;
    if (amitk_raw_data_includes_voxel(map_roi_ds, new_voxel))
      if (AMITK_RAW_DATA_UBYTE_CONTENT(map_roi_ds,new_voxel) != 0)
	edge_value++;

    new_voxel.y--;
    if (amitk_raw_data_includes_voxel(map_roi_ds, new_voxel))
      if (AMITK_RAW_DATA_UBYTE_CONTENT(map_roi_ds,new_voxel) != 0)
	edge_value++;

    new_voxel.x++;
    if (amitk_raw_data_includes_voxel(map_roi_ds, new_voxel))
      if (AMITK_RAW_DATA_UBYTE_CONTENT(map_roi_ds,new_voxel) != 0)
	edge_value++;

    new_voxel.x++;
    if (amitk_raw_data_includes_voxel(map_roi_ds, new_voxel))
      if (AMITK_RAW_DATA_UBYTE_CONTENT(map_roi_ds,new_voxel) != 0)
	edge_value++;

    new_voxel.y++;
    if (amitk_raw_data_includes_voxel(map_roi_ds, new_voxel))
      if (AMITK_RAW_DATA_UBYTE_CONTENT(map_roi_ds,new_voxel) != 0)
	edge_value++;

    new_voxel.y++;
    if (amitk_raw_data_includes_voxel(map_roi_ds, new_voxel))
      if (AMITK_RAW_DATA_UBYTE_CONTENT(map_roi_ds,new_voxel) != 0)
	edge_value++;

    new_voxel.x--;
    if (amitk_raw_data_includes_voxel(map_roi_ds, new_voxel))
      if (AMITK_RAW_DATA_UBYTE_CONTENT(map_roi_ds,new_voxel) != 0)
	edge_value++;

    new_voxel.x--;
    if (amitk_raw_data_includes_voxel(map_roi_ds, new_voxel))
      if (AMITK_RAW_DATA_UBYTE_CONTENT(map_roi_ds,new_voxel) != 0)
	edge_value++;

#if defined(ROI_TYPE_ISOCONTOUR_3D) || defined(ROI_TYPE_FREEHAND_3D)
  }
#endif

#if defined(ROI_TYPE_ISOCONTOUR_3D) || defined(ROI_TYPE_FREEHAND_3D)
  if (edge_value == 26 )
    edge_value = 2;
  else
    edge_value = 1;
#else /* ROI_TYPE_ISOCONTOUR_2D or ROI_TYPE_FREEHAND_2D */
  if (edge_value == 8 )
    edge_value = 2;
  else
    edge_value = 1;
#endif

  return edge_value;
}



#define FAST_INTERSECTION_SLICE 1

/* intersection data is returned in the form of a volume slice */
AmitkDataSet * amitk_roi_`'m4_Variable_Type`'_get_intersection_slice(const AmitkRoi * roi,
								     const AmitkVolume * canvas_slice,
								     const amide_real_t pixel_dim
#ifndef AMIDE_LIBGNOMECANVAS_AA
								     ,const gboolean fill_map_roi
#endif
								     ) {



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
#if FAST_INTERSECTION_SLICE
  AmitkPoint alt, start_point;
  AmitkPoint stride[AMITK_AXIS_NUM], last_point;
  AmitkAxis i_axis;
#endif

  g_return_val_if_fail(!AMITK_ROI_UNDRAWN(roi), NULL);

  /* figure out the intersection between the canvas slice and the roi */
  if (!amitk_volume_volume_intersection_corners(canvas_slice, 
						AMITK_VOLUME(roi), 
						intersection_corners))
    return NULL; /* no intersection */

  /* translate the intersection into voxel space */
  canvas_voxel_size.x = canvas_voxel_size.y = pixel_dim;
  canvas_voxel_size.z = AMITK_VOLUME_Z_CORNER(canvas_slice);
  POINT_TO_VOXEL(intersection_corners[0], canvas_voxel_size, 0, 0, start);
  POINT_TO_VOXEL(intersection_corners[1], canvas_voxel_size, 0, 0, end);
  dim = voxel_add(voxel_sub(end, start), one_voxel);

  intersection = amitk_data_set_new(NULL, -1);
  intersection->raw_data = amitk_raw_data_new_2D_with_data0(AMITK_FORMAT_UBYTE,dim.y, dim.x);

  /* get the corners of the view slice in view coordinate space */
  slice_corners[0] = amitk_space_b2s(AMITK_SPACE(canvas_slice), 
				     AMITK_SPACE_OFFSET(canvas_slice));
  slice_corners[1] = AMITK_VOLUME_CORNER(canvas_slice);

  view_point.z = (slice_corners[0].z+slice_corners[1].z)/2.0;
  view_point.y = slice_corners[0].y+((double) start.y + 0.5)*pixel_dim;
#if FAST_INTERSECTION_SLICE
  view_point.x = slice_corners[0].x+((double) start.x + 0.5)*pixel_dim;

  /* figure out what point in the roi we're going to start at */
  start_point = amitk_space_s2s(AMITK_SPACE(canvas_slice),AMITK_SPACE(roi), view_point);

  /* figure out what stepping one voxel in a given direction in our slice coresponds to in our roi */
  for (i_axis = 0; i_axis <= AMITK_AXIS_Y; i_axis++) {
    alt.x = (i_axis == AMITK_AXIS_X) ? pixel_dim : 0.0;
    alt.y = (i_axis == AMITK_AXIS_Y) ? pixel_dim : 0.0;
    alt.z = 0.0;
    alt = point_add(point_sub(amitk_space_s2b(AMITK_SPACE(canvas_slice), alt),
			      AMITK_SPACE_OFFSET(canvas_slice)),
		    AMITK_SPACE_OFFSET(roi));
    stride[i_axis] = amitk_space_b2s(AMITK_SPACE(roi), alt);
  }


  roi_point = start_point;
#endif
  i_voxel.z = i_voxel.g = i_voxel.t = 0;
  for (i_voxel.y=0; i_voxel.y<dim.y; i_voxel.y++) {
#if FAST_INTERSECTION_SLICE
    last_point = roi_point;
#else
    view_point.x = slice_corners[0].x+((double)start.x+0.5)*pixel_dim;
#endif

    for (i_voxel.x=0; i_voxel.x<dim.x; i_voxel.x++) {
#if FAST_INTERSECTION_SLICE

#else
      roi_point = amitk_space_s2s(AMITK_SPACE(canvas_slice), AMITK_SPACE(roi), view_point);
#endif
      POINT_TO_VOXEL(roi_point, roi->voxel_size, 0, 0, roi_voxel);
      if (amitk_raw_data_includes_voxel(roi->map_data, roi_voxel)) {
	value = AMITK_RAW_DATA_UBYTE_CONTENT(roi->map_data, roi_voxel);
#if defined(ROI_TYPE_ISOCONTOUR_2D) || defined(ROI_TYPE_FREEHAND_2D)
	if (value > 0)
	  AMITK_RAW_DATA_UBYTE_SET_CONTENT(intersection->raw_data, i_voxel) = 1;
#else /* ISOCONTOUR_3D or FREEHAND_3D */

#ifdef AMIDE_LIBGNOMECANVAS_AA
        if (value > 0)
	  AMITK_RAW_DATA_UBYTE_SET_CONTENT(intersection->raw_data, i_voxel) = value;
#else
	if ((value == 1) || ((value > 0) && fill_map_roi))
	  AMITK_RAW_DATA_UBYTE_SET_CONTENT(intersection->raw_data, i_voxel) = 1;
#endif
#endif
      }
#if FAST_INTERSECTION_SLICE
      POINT_ADD(roi_point, stride[AMITK_AXIS_X], roi_point);
#else
      view_point.x += pixel_dim;
#endif
    }
#if FAST_INTERSECTION_SLICE
    POINT_ADD(last_point, stride[AMITK_AXIS_Y], roi_point);
#else
    view_point.y += pixel_dim;
#endif

  }

#if defined(ROI_TYPE_ISOCONTOUR_2D) || defined(ROI_TYPE_FREEHAND_2D)
#ifndef AMIDE_LIBGNOMECANVAS_AA
  if (!fill_map_roi) 
#endif
    {
      /* mark the edges as such on the 2D isocontour or freehand slices */
      i_voxel.z = i_voxel.g = i_voxel.t = 0;
      for (i_voxel.y=0; i_voxel.y<dim.y; i_voxel.y++) 
	for (i_voxel.x=0; i_voxel.x<dim.x; i_voxel.x++) 
	  if (AMITK_RAW_DATA_UBYTE_CONTENT(intersection->raw_data, i_voxel)) 
	    AMITK_RAW_DATA_UBYTE_SET_CONTENT(intersection->raw_data, i_voxel) =
	      map_roi_edge(intersection->raw_data, i_voxel);
    }
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



#if defined(ROI_TYPE_ISOCONTOUR_2D) || defined(ROI_TYPE_ISOCONTOUR_3D) 
/* the data in temp_rd is setup as follows:
   bit 1 -> is the voxel in the isocontour
   bit 2 -> has the voxel been checked 
   bit 3 -> increment x for backup
   bit 4 -> decrement x for backup
   bit 5 -> increment y for backup
   bit 6 -> decrement y for backup
   bit 7 -> increment z for backup
   bit 8 -> decrement z for backup
*/
static void isocontour_consider(const AmitkDataSet * ds,
				const AmitkRawData * temp_rd, 
				AmitkVoxel ds_voxel, 
				const amide_data_t iso_min_value,
				const amide_data_t iso_max_value,
				const AmitkRoiIsocontourRange iso_range) {


  AmitkVoxel i_voxel;
  AmitkVoxel roi_voxel;
  gboolean found;
  gboolean done;
  amide_data_t voxel_value;

  roi_voxel = ds_voxel;
  roi_voxel.t = roi_voxel.g = 0;
  i_voxel =roi_voxel;
  done=FALSE;

  /* the starting point is in by definition */
  AMITK_RAW_DATA_UBYTE_SET_CONTENT(temp_rd, roi_voxel) |= 0x03;

  /* while we still have neighbor voxels to check */
  while (!done) {

    found = FALSE;

    /* find a neighbor to check, consider the 8 adjoining voxels, or 26 in the case of 3D */
#ifdef ROI_TYPE_ISOCONTOUR_3D
    for (i_voxel.z = (roi_voxel.z >= 1) ? roi_voxel.z-1 : 0;
	 (i_voxel.z < temp_rd->dim.z) && (i_voxel.z <= roi_voxel.z+1) && (!found);
	 i_voxel.z++)
#endif
      for (i_voxel.y = (roi_voxel.y >= 1) ? roi_voxel.y-1 : 0;
	   (i_voxel.y < temp_rd->dim.y) && (i_voxel.y <= roi_voxel.y+1) && (!found);
	   i_voxel.y++)
	for (i_voxel.x = (roi_voxel.x >= 1) ? roi_voxel.x-1 : 0;
	     (i_voxel.x < temp_rd->dim.x) && (i_voxel.x <= roi_voxel.x+1) && (!found);
	     i_voxel.x++) 
	  if (!(AMITK_RAW_DATA_UBYTE_CONTENT(temp_rd, i_voxel) & 0x02)) { /* don't recheck something we've already checked */
	    AMITK_RAW_DATA_UBYTE_SET_CONTENT(temp_rd,i_voxel) |= 0x02; /* it's been checked */

	    ds_voxel.z = i_voxel.z;
	    ds_voxel.y = i_voxel.y;
	    ds_voxel.x = i_voxel.x;

	    voxel_value = amitk_data_set_get_value(ds, ds_voxel);
	    if (((iso_range == AMITK_ROI_ISOCONTOUR_RANGE_ABOVE_MIN) && (voxel_value >= iso_min_value)) ||
		((iso_range == AMITK_ROI_ISOCONTOUR_RANGE_BELOW_MAX) && (voxel_value <= iso_max_value)) ||
		((iso_range == AMITK_ROI_ISOCONTOUR_RANGE_BETWEEN_MIN_MAX) && (voxel_value >= iso_min_value) && (voxel_value <= iso_max_value))) {

	      AMITK_RAW_DATA_UBYTE_SET_CONTENT(temp_rd,i_voxel) |= 0x01; /* it's in */

	      /* store backup info */
#ifdef ROI_TYPE_ISOCONTOUR_3D
	      if (i_voxel.z > roi_voxel.z)
		AMITK_RAW_DATA_UBYTE_SET_CONTENT(temp_rd,i_voxel) |= 0x40;
	      else if (i_voxel.z < roi_voxel.z)
		AMITK_RAW_DATA_UBYTE_SET_CONTENT(temp_rd,i_voxel) |= 0x80;
#endif
	      if (i_voxel.y > roi_voxel.y)
		AMITK_RAW_DATA_UBYTE_SET_CONTENT(temp_rd,i_voxel) |= 0x10;
	      else if (i_voxel.y < roi_voxel.y)
		AMITK_RAW_DATA_UBYTE_SET_CONTENT(temp_rd,i_voxel) |= 0x20;
	      if (i_voxel.x > roi_voxel.x)
		AMITK_RAW_DATA_UBYTE_SET_CONTENT(temp_rd,i_voxel) |= 0x04;
	      else if (i_voxel.x < roi_voxel.x)
		AMITK_RAW_DATA_UBYTE_SET_CONTENT(temp_rd,i_voxel) |= 0x08;
	      
	      /* start next iteration at this voxel */
	      roi_voxel = i_voxel; 
	      found = TRUE;
	    }
	  }

        
    if (!found) { /* all neighbors exhaustively checked, backup */

      /* backup to previous voxel */
      i_voxel = roi_voxel;
#ifdef ROI_TYPE_ISOCONTOUR_3D
      if (AMITK_RAW_DATA_UBYTE_CONTENT(temp_rd, roi_voxel) & 0x80)
	i_voxel.z++;
      else if (AMITK_RAW_DATA_UBYTE_CONTENT(temp_rd, roi_voxel) & 0x40)
	i_voxel.z--;
#endif

      if (AMITK_RAW_DATA_UBYTE_CONTENT(temp_rd, roi_voxel) & 0x20)
	i_voxel.y++;
      else if (AMITK_RAW_DATA_UBYTE_CONTENT(temp_rd, roi_voxel) & 0x10)
	i_voxel.y--;

      if (AMITK_RAW_DATA_UBYTE_CONTENT(temp_rd, roi_voxel) & 0x08)
	i_voxel.x++;
      else if (AMITK_RAW_DATA_UBYTE_CONTENT(temp_rd, roi_voxel) & 0x04)
	i_voxel.x--;

      /* the inital voxel will backup to itself, which is why we get out of the loop */
      if (VOXEL_EQUAL(roi_voxel, i_voxel)) 
	done = TRUE;
      else
	roi_voxel = i_voxel;
    }
  }

  return;
}
  



void amitk_roi_`'m4_Variable_Type`'_set_isocontour(AmitkRoi * roi, AmitkDataSet * ds, 
						   AmitkVoxel iso_voxel,
						   amide_data_t iso_min_value,
						   amide_data_t iso_max_value,
						   AmitkRoiIsocontourRange iso_range) {

  AmitkRawData * temp_rd;
  AmitkPoint temp_point;
  AmitkVoxel min_voxel, max_voxel, i_voxel;
  amide_data_t temp_min_value, temp_max_value;

  g_return_if_fail(roi->type == AMITK_ROI_TYPE_`'m4_Variable_Type`');

  /* what we're setting the isocontour too */
  roi->isocontour_min_value = iso_min_value; 
  roi->isocontour_max_value = iso_max_value; 
  roi->isocontour_range = iso_range; 

  /* we first make a raw data set the size of the data set to record the in/out values */
#if defined(ROI_TYPE_ISOCONTOUR_2D)
  temp_rd = amitk_raw_data_new_2D_with_data0(AMITK_FORMAT_UBYTE, ds->raw_data->dim.y, ds->raw_data->dim.x);
#elif defined(ROI_TYPE_ISOCONTOUR_3D)
  temp_rd = amitk_raw_data_new_3D_with_data0(AMITK_FORMAT_UBYTE, ds->raw_data->dim.z, ds->raw_data->dim.y, ds->raw_data->dim.x);
#endif

  /* epsilon guards for floating point rounding */
  temp_min_value = roi->isocontour_min_value-EPSILON*fabs(roi->isocontour_min_value); 
  temp_max_value = roi->isocontour_max_value+EPSILON*fabs(roi->isocontour_max_value); 

  /* fill in the data set */
  isocontour_consider(ds, temp_rd, iso_voxel, temp_min_value, temp_max_value, iso_range);
  
  /* figure out the min and max dimensions */
  min_voxel = max_voxel = iso_voxel;
  
  i_voxel.t = i_voxel.g = 0;
  for (i_voxel.z=0; i_voxel.z < temp_rd->dim.z; i_voxel.z++) {
    for (i_voxel.y=0; i_voxel.y < temp_rd->dim.y; i_voxel.y++) {
      for (i_voxel.x=0; i_voxel.x < temp_rd->dim.x; i_voxel.x++) {
	if (AMITK_RAW_DATA_UBYTE_CONTENT(temp_rd, i_voxel) & 0x1) {
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
  if (roi->map_data != NULL)
    g_object_unref(roi->map_data);
#if defined(ROI_TYPE_ISOCONTOUR_2D)
  roi->map_data = amitk_raw_data_new_2D_with_data0(AMITK_FORMAT_UBYTE, max_voxel.y-min_voxel.y+1, max_voxel.x-min_voxel.x+1);
#elif defined(ROI_TYPE_ISOCONTOUR_3D)
  roi->map_data = amitk_raw_data_new_3D_with_data0(AMITK_FORMAT_UBYTE,
						   max_voxel.z-min_voxel.z+1,
						   max_voxel.y-min_voxel.y+1, 
						   max_voxel.x-min_voxel.x+1);
#endif

  i_voxel.t = i_voxel.g = 0;
  for (i_voxel.z=0; i_voxel.z<roi->map_data->dim.z; i_voxel.z++)
    for (i_voxel.y=0; i_voxel.y<roi->map_data->dim.y; i_voxel.y++)
      for (i_voxel.x=0; i_voxel.x<roi->map_data->dim.x; i_voxel.x++) {
#if defined(ROI_TYPE_ISOCONTOUR_2D)
	AMITK_RAW_DATA_UBYTE_SET_CONTENT(roi->map_data, i_voxel) = 
	  0x01 & AMITK_RAW_DATA_UBYTE_2D_CONTENT(temp_rd, i_voxel.y+min_voxel.y, i_voxel.x+min_voxel.x);
#elif defined(ROI_TYPE_ISOCONTOUR_3D)
	AMITK_RAW_DATA_UBYTE_SET_CONTENT(roi->map_data, i_voxel) =
	  0x01 & AMITK_RAW_DATA_UBYTE_3D_CONTENT(temp_rd, i_voxel.z+min_voxel.z,i_voxel.y+min_voxel.y, i_voxel.x+min_voxel.x);
#endif
      }

  g_object_unref(temp_rd);

  /* mark the edges as such */
  i_voxel.t = i_voxel.g = 0;
  for (i_voxel.z=0; i_voxel.z<roi->map_data->dim.z; i_voxel.z++)
    for (i_voxel.y=0; i_voxel.y<roi->map_data->dim.y; i_voxel.y++) 
      for (i_voxel.x=0; i_voxel.x<roi->map_data->dim.x; i_voxel.x++) 
	if (AMITK_RAW_DATA_UBYTE_CONTENT(roi->map_data, i_voxel)) 
	  AMITK_RAW_DATA_UBYTE_SET_CONTENT(roi->map_data, i_voxel) =
	    map_roi_edge(roi->map_data, i_voxel);

  /* and set the rest of the important info for the data set */
  amitk_space_copy_in_place(AMITK_SPACE(roi), AMITK_SPACE(ds));
  roi->voxel_size = ds->voxel_size;

  POINT_MULT(min_voxel, ds->voxel_size, temp_point);
  temp_point = amitk_space_s2b(AMITK_SPACE(ds), temp_point);
  amitk_space_set_offset(AMITK_SPACE(roi), temp_point);

  amitk_roi_calc_far_corner(roi);

  return;

}

#endif





void amitk_roi_`'m4_Variable_Type`'_manipulate_area(AmitkRoi * roi, gboolean erase, AmitkVoxel voxel, gint area_size) {

  AmitkVoxel i_voxel, j_voxel;
  AmitkVoxel new_dim;
  AmitkVoxel offset;
  AmitkVoxel map_data_dim;
  AmitkPoint new_offset;
  AmitkRawData * temp_rd;
  gboolean dim_changed=FALSE;

#if defined(ROI_TYPE_ISOCONTOUR_2D) || defined(ROI_TYPE_ISOCONTOUR_3D)
  g_return_if_fail(roi->map_data != NULL);
#endif

  j_voxel = zero_voxel;
  i_voxel = zero_voxel;

  /* check if we need to increase the size of the roi */
  if (!erase || (roi->map_data == NULL)) { /* never need to do for an erase, unless map_data hasn't yet been allocated */
    if (roi->map_data != NULL) {
      new_dim = roi->map_data->dim;
      map_data_dim = roi->map_data->dim;
    } else {
      new_dim = zero_voxel;
      map_data_dim = zero_voxel;
    }
    offset = zero_voxel;

#if defined(ROI_TYPE_ISOCONTOUR_3D) || defined(ROI_TYPE_FREEHAND_3D)
    if ((voxel.z-area_size) < 0) {
      offset.z = -(voxel.z-area_size);
      new_dim.z += offset.z;
      dim_changed = TRUE;
    }
    if ((voxel.z+area_size) > (map_data_dim.z-1)) {
      new_dim.z += (voxel.z-(map_data_dim.z-1))+area_size;
      dim_changed = TRUE;
    }
#endif
    if ((voxel.y-area_size) < 0) {
      offset.y = -(voxel.y-area_size);
      new_dim.y += offset.y;
      dim_changed = TRUE;
    }
    if ((voxel.y+area_size) > (map_data_dim.y-1)) {
      new_dim.y += (voxel.y-(map_data_dim.y-1))+area_size;
      dim_changed = TRUE;
    }
    if ((voxel.x-area_size) < 0) {
      offset.x = -(voxel.x-area_size);
      new_dim.x += offset.x;
      dim_changed = TRUE;
    }
    if ((voxel.x+area_size) > (map_data_dim.x-1)) {
      new_dim.x += (voxel.x-(map_data_dim.x-1))+area_size;
      dim_changed = TRUE;
    }

if (dim_changed) {
#if defined(ROI_TYPE_ISOCONTOUR_2D) || defined(ROI_TYPE_FREEHAND_2D)
      temp_rd = amitk_raw_data_new_2D_with_data0(AMITK_FORMAT_UBYTE, new_dim.y, new_dim.x);
#else /* ROI_TYPE_ISOCONTOUR_3D or ROI_TYPE_FREEHAND_3D */
      temp_rd = amitk_raw_data_new_3D_with_data0(AMITK_FORMAT_UBYTE, new_dim.z, new_dim.y, new_dim.x);
#endif
      
      /* copy the old map data over */
      if (roi->map_data != NULL) {
#if defined(ROI_TYPE_ISOCONTOUR_3D) || defined(ROI_TYPE_FREEHAND_3D)
	for (i_voxel.z = 0, j_voxel.z=offset.z; i_voxel.z < map_data_dim.z; i_voxel.z++, j_voxel.z++) 
#endif
	  for (i_voxel.y = 0, j_voxel.y=offset.y; i_voxel.y < map_data_dim.y; i_voxel.y++, j_voxel.y++) 
	    for (i_voxel.x = 0, j_voxel.x=offset.x; i_voxel.x < map_data_dim.x; i_voxel.x++, j_voxel.x++) 
	      AMITK_RAW_DATA_UBYTE_SET_CONTENT(temp_rd,j_voxel)=
		AMITK_RAW_DATA_UBYTE_CONTENT(roi->map_data,i_voxel);
      
	g_object_unref(roi->map_data);
      }
      roi->map_data = temp_rd;
      
      /* shift the offset to account for the large ROI */
      POINT_MULT(offset, roi->voxel_size, new_offset);
      new_offset = point_cmult(-(1.0+EPSILON), new_offset);
      new_offset = amitk_space_s2b(AMITK_SPACE(roi), new_offset);
      amitk_space_set_offset(AMITK_SPACE(roi), new_offset);
      amitk_roi_calc_far_corner(roi);
    }
  }

  /* sanity check */
  g_return_if_fail(roi->map_data != NULL);

  /* do the erase or drawing */
#if defined(ROI_TYPE_ISOCONTOUR_3D) || defined(ROI_TYPE_FREEHAND_3D)
  for (i_voxel.z = voxel.z-area_size; i_voxel.z <= voxel.z+area_size; i_voxel.z++) 
#endif
    for (i_voxel.y = voxel.y-area_size; i_voxel.y <= voxel.y+area_size; i_voxel.y++) 
      for (i_voxel.x = voxel.x-area_size; i_voxel.x <= voxel.x+area_size; i_voxel.x++) 
	if (erase) {
	  if (amitk_raw_data_includes_voxel(roi->map_data, i_voxel))
	    AMITK_RAW_DATA_UBYTE_SET_CONTENT(roi->map_data,i_voxel)=0;
	} else {
	  if (amitk_raw_data_includes_voxel(roi->map_data, i_voxel))
	    AMITK_RAW_DATA_UBYTE_SET_CONTENT(roi->map_data,i_voxel)=1;
	}

  /* re edge the neighboring points */
#if defined(ROI_TYPE_ISOCONTOUR_3D) || defined(ROI_TYPE_FREEHAND_3D)
  for (i_voxel.z = voxel.z-1-area_size; i_voxel.z <= voxel.z+1+area_size; i_voxel.z++) 
#endif
    for (i_voxel.y = voxel.y-1-area_size; i_voxel.y <= voxel.y+1+area_size; i_voxel.y++) 
      for (i_voxel.x = voxel.x-1-area_size; i_voxel.x <= voxel.x+1+area_size; i_voxel.x++) 
	if (amitk_raw_data_includes_voxel(roi->map_data, i_voxel)) 
  	  if (AMITK_RAW_DATA_UBYTE_CONTENT(roi->map_data, i_voxel)) 
	    AMITK_RAW_DATA_UBYTE_SET_CONTENT(roi->map_data, i_voxel) =
	      map_roi_edge(roi->map_data, i_voxel);

  return;
}


void amitk_roi_`'m4_Variable_Type`'_calc_center_of_mass(AmitkRoi * roi) {

  AmitkVoxel i_voxel;
  guint voxels=0;
  AmitkPoint center_of_mass;
  AmitkPoint roi_voxel_size;
  AmitkPoint current_point;

  roi_voxel_size = AMITK_ROI_VOXEL_SIZE(roi);
  center_of_mass = zero_point;

  i_voxel.t = i_voxel.g = 0;
  for (i_voxel.z=0; i_voxel.z<roi->map_data->dim.z; i_voxel.z++)
    for (i_voxel.y=0; i_voxel.y<roi->map_data->dim.y; i_voxel.y++) 
      for (i_voxel.x=0; i_voxel.x<roi->map_data->dim.x; i_voxel.x++) 
	if (AMITK_RAW_DATA_UBYTE_CONTENT(roi->map_data, i_voxel)) {
	  voxels++;
	  VOXEL_TO_POINT(i_voxel, roi_voxel_size, current_point);
	  POINT_ADD(current_point, center_of_mass, center_of_mass);
	}

  roi->center_of_mass = point_cmult(1.0/((gdouble) voxels), center_of_mass);
  roi->center_of_mass_calculated=TRUE;

}

#endif








/* iterates over the voxels in the given data set that are inside the given roi,
   and performs the specified calculation function for those points */
/* calulation should be a function taking the following arguments:
   calculation(AmitkVoxel dataset_voxel, amide_data_t value, amide_real_t voxel_fraction, gpointer data) */
void amitk_roi_`'m4_Variable_Type`'_calculate_on_data_set_fast(const AmitkRoi * roi,  
							       const AmitkDataSet * ds, 
							       const guint frame,
							       const guint gate,
							       const gboolean inverse,
							       void (* calculation)(),
							       gpointer data) {

  AmitkPoint roi_pt_corner, roi_pt_center, fine_roi_pt;
  AmitkPoint fine_ds_pt, far_ds_pt, center_ds_pt;
  amide_data_t value;
  amide_real_t voxel_fraction;
  AmitkVoxel i,j, k;
  AmitkVoxel start, dim, ds_dim;
  gboolean corner_in, center_in;
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

#if defined(ROI_TYPE_ISOCONTOUR_2D) || defined(ROI_TYPE_ISOCONTOUR_3D) || defined(ROI_TYPE_FREEHAND_2D) || defined(ROI_TYPE_FREEHAND_3D)
  AmitkPoint roi_voxel_size;
  AmitkVoxel roi_voxel;

  roi_voxel_size = AMITK_ROI_VOXEL_SIZE(roi);
#endif

  ds_voxel_size = AMITK_DATA_SET_VOXEL_SIZE(ds);
  sub_voxel_size = point_cmult(1.0/AMITK_ROI_GRANULARITY, ds_voxel_size);
  ds_dim = AMITK_DATA_SET_DIM(ds);

  grain_size = 1.0/(AMITK_ROI_GRANULARITY*AMITK_ROI_GRANULARITY*AMITK_ROI_GRANULARITY);

  /* figure out the intersection between the data set and the roi */
  if (inverse) {
    start = zero_voxel;
    dim = ds_dim;
  } else {
    if (!amitk_volume_volume_intersection_corners(AMITK_VOLUME(ds),  AMITK_VOLUME(roi), 
						  intersection_corners)) {
      dim = zero_voxel; /* no intersection */
      start = zero_voxel;
    } else {
      /* translate the intersection into voxel space */
      POINT_TO_VOXEL(intersection_corners[0], ds_voxel_size, 0, 0, start);
      POINT_TO_VOXEL(intersection_corners[1], ds_voxel_size, 0, 0, dim);
      dim = voxel_add(voxel_sub(dim, start), one_voxel);
    }
  }

  /* check if we're done already */
  if ((dim.x == 0) || (dim.y == 0) || (dim.z == 0)) {
    return;
  }

  /* if we have any small dimensions, make sure we always iterate over sub voxels */
  if ((dim.x == 1) || (dim.y == 1) || (dim.z == 1) ||
      (ds_dim.x == 1) || (ds_dim.y == 1) || (ds_dim.z == 1))
    small_dimensions = TRUE;
  else
    small_dimensions = FALSE;
  
  /* over-iterate, as our initial edges will always be considered out of the roi */
  start.x -= 1;
  start.y -= 1;
  start.z -= 1;
  dim.x += 1;
  dim.y += 1;
  dim.z += 1;

  /* check all dimensions */
  if (start.x < 0) start.x = 0;
  if (start.y < 0) start.y = 0;
  if (start.z < 0) start.z = 0;
  if (dim.x+start.x > ds_dim.x) dim.x = ds_dim.x-start.x;
  if (dim.y+start.y > ds_dim.y) dim.y = ds_dim.y-start.y;
  if (dim.z+start.z > ds_dim.z) dim.z = ds_dim.z-start.z;

  /* start and dim specify (in the data set's voxel space) the voxels in 
     the volume we should be iterating over */

  /* these two arrays are used to store whether the voxel vertices are in the ROI or not */
  next_plane_in = amitk_raw_data_new_2D_with_data0(AMITK_FORMAT_UBYTE, dim.y+1, dim.x+1);
  curr_plane_in = amitk_raw_data_new_2D_with_data0(AMITK_FORMAT_UBYTE, dim.y+1, dim.x+1);

  j.t = frame;
  j.g = gate;
  i.t = k.t = i.g = k.g = 0;
  for (i.z = 0; i.z < dim.z; i.z++) {
    j.z = i.z+start.z;
    far_ds_pt.z = (j.z+1)*ds_voxel_size.z;
    center_ds_pt.z = (j.z+0.5)*ds_voxel_size.z;
    
    for (i.y = 0; i.y < dim.y; i.y++) {
      j.y = i.y+start.y;
      far_ds_pt.y = (j.y+1)*ds_voxel_size.y;
      center_ds_pt.y = (j.y+0.5)*ds_voxel_size.y;
      
      for (i.x = 0; i.x < dim.x; i.x++) {
	j.x = i.x+start.x;
	far_ds_pt.x = (j.x+1)*ds_voxel_size.x;
	center_ds_pt.x = (j.x+0.5)*ds_voxel_size.x;
	
	/* figure out if the center and the next far corner is in the roi or not */
	/* get the corresponding roi points */
	roi_pt_corner = amitk_space_s2s(AMITK_SPACE(ds),  AMITK_SPACE(roi), far_ds_pt);
	roi_pt_center = amitk_space_s2s(AMITK_SPACE(ds),  AMITK_SPACE(roi), center_ds_pt);

	/* calculate the one corner of the voxel "box" to determine if it's in or not */
	/* along with the center of the voxel */
#if defined (ROI_TYPE_BOX)
	corner_in = point_in_box(roi_pt_corner, box_corner);
	center_in = point_in_box(roi_pt_center, box_corner);
#endif
#if defined(ROI_TYPE_CYLINDER)
	corner_in = point_in_elliptic_cylinder(roi_pt_corner, center, height, radius);
	center_in = point_in_elliptic_cylinder(roi_pt_center, center, height, radius);
#endif
#if defined(ROI_TYPE_ELLIPSOID)
	corner_in = point_in_ellipsoid(roi_pt_corner,center,radius);
	center_in = point_in_ellipsoid(roi_pt_center,center,radius);
#endif
#if defined(ROI_TYPE_ISOCONTOUR_2D) || defined(ROI_TYPE_ISOCONTOUR_3D) || defined(ROI_TYPE_FREEHAND_2D) || defined(ROI_TYPE_FREEHAND_3D)
	POINT_TO_VOXEL(roi_pt_corner, roi_voxel_size, 0, 0,roi_voxel);
	corner_in = (amitk_raw_data_includes_voxel(roi->map_data, roi_voxel) &&
		     (AMITK_RAW_DATA_UBYTE_CONTENT(roi->map_data, roi_voxel) != 0));
	POINT_TO_VOXEL(roi_pt_center, roi_voxel_size, 0, 0,roi_voxel);
	center_in = (amitk_raw_data_includes_voxel(roi->map_data, roi_voxel) &&
		     (AMITK_RAW_DATA_UBYTE_CONTENT(roi->map_data, roi_voxel) != 0));
#endif

	AMITK_RAW_DATA_UBYTE_2D_SET_CONTENT(next_plane_in,i.y+1,i.x+1)=corner_in;

	if (AMITK_RAW_DATA_UBYTE_2D_CONTENT(curr_plane_in,i.y,i.x) &&
	    AMITK_RAW_DATA_UBYTE_2D_CONTENT(curr_plane_in,i.y,i.x+1) &&
	    AMITK_RAW_DATA_UBYTE_2D_CONTENT(curr_plane_in,i.y+1,i.x) &&
	    AMITK_RAW_DATA_UBYTE_2D_CONTENT(curr_plane_in,i.y+1,i.x+1) &&
	    AMITK_RAW_DATA_UBYTE_2D_CONTENT(next_plane_in,i.y,i.x) &&
	    AMITK_RAW_DATA_UBYTE_2D_CONTENT(next_plane_in,i.y,i.x+1) &&
	    AMITK_RAW_DATA_UBYTE_2D_CONTENT(next_plane_in,i.y+1,i.x) &&
	    AMITK_RAW_DATA_UBYTE_2D_CONTENT(next_plane_in,i.y+1,i.x+1) &&
	    center_in) {
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
		   center_in ||
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
		
		fine_roi_pt = amitk_space_s2s(AMITK_SPACE(ds), AMITK_SPACE(roi), fine_ds_pt);

		/* calculate the one corner of the voxel "box" to determine if it's in or not */
#if defined (ROI_TYPE_BOX)
		if (point_in_box(fine_roi_pt, box_corner)) voxel_fraction += grain_size;
#endif
#if defined(ROI_TYPE_CYLINDER)
		if (point_in_elliptic_cylinder(fine_roi_pt, center, height, radius)) voxel_fraction += grain_size;
#endif
#if defined(ROI_TYPE_ELLIPSOID)
		if (point_in_ellipsoid(fine_roi_pt,center,radius)) voxel_fraction += grain_size;
#endif
#if defined(ROI_TYPE_ISOCONTOUR_2D) || defined(ROI_TYPE_ISOCONTOUR_3D) || defined(ROI_TYPE_FREEHAND_2D) || defined(ROI_TYPE_FREEHAND_3D)
		POINT_TO_VOXEL(fine_roi_pt, roi_voxel_size, 0, 0, roi_voxel);
		if (amitk_raw_data_includes_voxel(roi->map_data, roi_voxel) &&
		    (AMITK_RAW_DATA_UBYTE_CONTENT(roi->map_data, roi_voxel) != 0))
		  voxel_fraction += grain_size;
#endif
	      } /* k.x loop */
	    } /* k.y loop */
	  } /* k.z loop */

	  if (!inverse) {
	    (*calculation)(j, value, voxel_fraction, data);
	  } else {
	    (*calculation)(j, value, 1.0-voxel_fraction, data);
	  }

	} else { /* this voxel is outside the ROI */
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


/* iterates over the voxels in the given data set that are inside the given roi,
   and performs the specified calculation function for those points */
/* calulation should be a function taking the following arguments:
   calculation(AmitkVoxel dataset_voxel, amide_data_t value, amide_real_t voxel_fraction, gpointer data) */
void amitk_roi_`'m4_Variable_Type`'_calculate_on_data_set_accurate(const AmitkRoi * roi,  
								   const AmitkDataSet * ds, 
								   const guint frame,
								   const guint gate,
								   const gboolean inverse,
								   void (* calculation)(),
								   gpointer data) {

  AmitkPoint fine_roi_pt, fine_ds_pt;
  amide_data_t value;
  amide_real_t voxel_fraction;
  AmitkVoxel j, k;
  AmitkVoxel start, end, ds_dim;
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

#if defined(ROI_TYPE_ISOCONTOUR_2D) || defined(ROI_TYPE_ISOCONTOUR_3D) || defined(ROI_TYPE_FREEHAND_2D) || defined(ROI_TYPE_FREEHAND_3D)
  AmitkPoint roi_voxel_size;
  AmitkVoxel roi_voxel;

  roi_voxel_size = AMITK_ROI_VOXEL_SIZE(roi);
#endif

  ds_voxel_size = AMITK_DATA_SET_VOXEL_SIZE(ds);
  sub_voxel_size = point_cmult(1.0/AMITK_ROI_GRANULARITY, ds_voxel_size);
  ds_dim = AMITK_DATA_SET_DIM(ds);

  grain_size = 1.0/(AMITK_ROI_GRANULARITY*AMITK_ROI_GRANULARITY*AMITK_ROI_GRANULARITY);

  /* figure out the intersection between the data set and the roi */
  if (inverse) {
    start = zero_voxel;
    end = voxel_sub(ds_dim, one_voxel);
  } else {
    if (!amitk_volume_volume_intersection_corners(AMITK_VOLUME(ds),  AMITK_VOLUME(roi), 
						  intersection_corners)) {
      end = zero_voxel; /* no intersection */
      start = one_voxel;
    } else {
      /* translate the intersection into voxel space */
      POINT_TO_VOXEL(intersection_corners[0], ds_voxel_size, 0, 0, start);
      POINT_TO_VOXEL(intersection_corners[1], ds_voxel_size, 0, 0, end);
    }
  }

  /* check if we're done already */
  if ((start.x > end.x) || (start.y > end.y) || (start.z > end.z)) 
    return;

  /* check all dimensions */
  if (start.x < 0) start.x = 0;
  if (start.y < 0) start.y = 0;
  if (start.z < 0) start.z = 0;
  if (end.x >= ds_dim.x) end.x = ds_dim.x-1;
  if (end.y >= ds_dim.y) end.y = ds_dim.y-1;
  if (end.z >= ds_dim.z) end.z = ds_dim.z-1;

  /* start and end specify (in the data set's voxel space) the voxels in 
     the volume we should be iterating over */

  j.t = frame;
  j.g = gate;
  k.t = k.g = 0;

  for (j.z = start.z; j.z <= end.z; j.z++) {
    for (j.y = start.y; j.y <= end.y; j.y++) {
      for (j.x = start.x; j.x <= end.x; j.x++) {

	value = amitk_data_set_get_value(ds,j);
	voxel_fraction=0;

	for (k.z = 0;k.z<AMITK_ROI_GRANULARITY;k.z++) {
	  fine_ds_pt.z = j.z*ds_voxel_size.z+ (k.z+0.5)*sub_voxel_size.z;

	  for (k.y = 0;k.y<AMITK_ROI_GRANULARITY;k.y++) {
	    fine_ds_pt.y = j.y*ds_voxel_size.y+ (k.y+0.5)*sub_voxel_size.y;

	    /* fine_ds_pt.x gets advanced at bottom of loop */
	    fine_ds_pt.x = j.x*ds_voxel_size.x+0.5*sub_voxel_size.x;

	    for (k.x = 0;k.x<AMITK_ROI_GRANULARITY;k.x++) {
	      fine_roi_pt = amitk_space_s2s(AMITK_SPACE(ds), AMITK_SPACE(roi), fine_ds_pt);

	      /* is this point in */
#if defined (ROI_TYPE_BOX)
	      if (point_in_box(fine_roi_pt, box_corner)) voxel_fraction+=grain_size;
#endif
#if defined(ROI_TYPE_CYLINDER)
	      if (point_in_elliptic_cylinder(fine_roi_pt, center, height, radius)) voxel_fraction+=grain_size;
#endif
#if defined(ROI_TYPE_ELLIPSOID)
	      if (point_in_ellipsoid(fine_roi_pt,center,radius)) voxel_fraction+=grain_size;
#endif
#if defined(ROI_TYPE_ISOCONTOUR_2D) || defined(ROI_TYPE_ISOCONTOUR_3D) || defined(ROI_TYPE_FREEHAND_2D) || defined(ROI_TYPE_FREEHAND_3D)
	      POINT_TO_VOXEL(fine_roi_pt, roi_voxel_size, 0, 0, roi_voxel);
	      if (amitk_raw_data_includes_voxel(roi->map_data, roi_voxel) &&
		  (AMITK_RAW_DATA_UBYTE_CONTENT(roi->map_data, roi_voxel) != 0)) 
		voxel_fraction+=grain_size;
#endif
	      fine_ds_pt.x += sub_voxel_size.x;
	    } /* k.x loop */
	  } /* k.y loop */
	} /* k.z loop */

	if (!inverse) {
	  if (voxel_fraction > 0.0) {
	    (*calculation)(j, value, voxel_fraction, data);
	  }
	} else {
	  if (voxel_fraction < 1.0) {
	    (*calculation)(j, value, 1.0-voxel_fraction, data);
	  }
	}
      } /* i.x loop */
    } /* i.y loop */
  } /* i.z loop */

  return;
}




