/* roi.c
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
#include "roi.h"

/* external variables */
gchar * roi_type_names[] = {"Group", \
			    "Ellipsoid", \
			    "Elliptic Cylinder", \
			    "Box"};

gchar * roi_grain_names[] = {"1", \
			     "8", \
			     "27", \
			     "64"};

/* free up an roi */
void roi_free(amide_roi_t ** proi) {
  
  if (*proi != NULL) {
    if (((*proi)->children) != NULL)
      roi_list_free(&((*proi)->children));
    g_free((*proi)->name);
    g_free(*proi);
    *proi = NULL;
  }
  
  return;
}

/* free up an roi list */
void roi_list_free(amide_roi_list_t ** proi_list) {
  
  if (*proi_list != NULL) {
    roi_free(&((*proi_list)->roi));
    if ((*proi_list)->next != NULL)
      roi_list_free(&((*proi_list)->next));
    g_free(*proi_list);
    *proi_list = NULL;
  }

  return;
}

/* returns an initialized roi structure */
amide_roi_t * roi_init(void) {
  
  amide_roi_t * temp_roi;
  axis_t i;
  
  if ((temp_roi = 
       (amide_roi_t *) g_malloc(sizeof(amide_roi_t))) == NULL) {
    return NULL;
  }
  
  temp_roi->name = NULL;
  temp_roi->corner = realpoint_init;
  temp_roi->coord_frame.offset = realpoint_init;
  temp_roi->grain = GRAINS_1;
  for (i=0;i<NUM_AXIS;i++)
    temp_roi->coord_frame.axis[i] = default_axis[i];
  temp_roi->parent = NULL;
  temp_roi->children = NULL;
  
  return temp_roi;
}

/* copies the information of one roi into another, if dest roi
   doesn't exist, make it*/
void roi_copy(amide_roi_t ** dest_roi, amide_roi_t * src_roi) {

  /* sanity checks */
  g_assert(src_roi != NULL);
  g_assert(src_roi != *dest_roi);

  /* start over from scratch */
  roi_free(dest_roi);
  *dest_roi = roi_init();

  /* copy the data elements */
  (*dest_roi)->type = src_roi->type;
  (*dest_roi)->coord_frame = src_roi->coord_frame;
  (*dest_roi)->corner = src_roi->corner;
  (*dest_roi)->parent = src_roi->parent;
  (*dest_roi)->grain = src_roi->grain;


  /* make a separate copy in memory of the roi's name */
  roi_set_name(*dest_roi, src_roi->name);

  /* make a separate copy in memory of the roi's children */
  if (src_roi->children != NULL)
    roi_list_copy(&((*dest_roi)->children), src_roi->children);

  return;
}

/* sets the name of an roi
   note: new_name is copied rather then just being referenced by roi */
void roi_set_name(amide_roi_t * roi, gchar * new_name) {

  g_free(roi->name); /* free up the memory used by the old name */
  roi->name = g_strdup(new_name); /* and assign the new name */

  return;
}

/* figure out the center of the roi in real coords */
realpoint_t roi_calculate_center(const amide_roi_t * roi) {

  realpoint_t center;
  realpoint_t corner[2];

  /* get the far corner (in roi coords) */
  corner[0] = realspace_base_coord_to_alt(roi->coord_frame.offset, roi->coord_frame);
  corner[1] = roi->corner;
 
  /* the center in roi coords is then just half the far corner */
  REALSPACE_MADD(0.5,corner[1], 0.5,corner[0], center);
  
  /* now, translate that into real coords */
  center = realspace_alt_coord_to_base(center, roi->coord_frame);

  return center;
}


/* returns an initialized roi list node structure */
amide_roi_list_t * roi_list_init(void) {
  
  amide_roi_list_t * temp_roi_list;
  
  if ((temp_roi_list = 
       (amide_roi_list_t *) g_malloc(sizeof(amide_roi_list_t))) == NULL) {
    return NULL;
  }

  temp_roi_list->next = NULL;
  
  return temp_roi_list;
}

/* function to check that an roi is in an roi list */
gboolean roi_list_includes_roi(amide_roi_list_t *list, amide_roi_t * roi) {

  while (list != NULL)
    if (list->roi == roi)
      return TRUE;
    else
      list = list->next;

  return FALSE;
}

/* function to add an roi onto an roi list */
void roi_list_add_roi(amide_roi_list_t ** plist, amide_roi_t * roi) {

  amide_roi_list_t ** ptemp_list = plist;

  while (*ptemp_list != NULL)
    ptemp_list = &((*ptemp_list)->next);

  (*ptemp_list) = roi_list_init();
  (*ptemp_list)->roi = roi;

  return;
}


/* function to add an roi onto an roi list as the first item*/
void roi_list_add_roi_first(amide_roi_list_t ** plist, amide_roi_t * roi) {

  amide_roi_list_t * temp_list;

  temp_list = roi_list_init();
  temp_list->roi = roi;
  temp_list->next = *plist;
  *plist = temp_list;

  return;
}


/* function to remove an roi from an roi list, does not delete roi */
void roi_list_remove_roi(amide_roi_list_t ** plist, amide_roi_t * roi) {

  amide_roi_list_t * temp_list = *plist;
  amide_roi_list_t * prev_list = NULL;

  while (temp_list != NULL) {
    if (temp_list->roi == roi) {
      if (prev_list == NULL)
	*plist = temp_list->next;
      else
	prev_list->next = temp_list->next;
      temp_list->next = NULL;
      temp_list->roi = NULL;
      roi_list_free(&temp_list);
    } else {
      prev_list = temp_list;
      temp_list = temp_list->next;
    }
  }

  return;
}

/* copies the information of one roi list into another, if dest roi list
   doesn't exist, make it*/
void roi_list_copy(amide_roi_list_t ** dest_roi_list, amide_roi_list_t * src_roi_list) {

  /* sanity check */
  g_assert(src_roi_list != NULL);
  g_assert(src_roi_list != (*dest_roi_list));

  /* if we don't already have a roi_list, allocate the space for one */
  if (*dest_roi_list == NULL)
    *dest_roi_list = roi_list_init();

  /* make a separate copy in memory of the roi this list item points to */
  (*dest_roi_list)->roi = NULL;
  roi_copy(&((*dest_roi_list)->roi), src_roi_list->roi);
    
  /* and make copies of the rest of the elements in this list */
  if (src_roi_list->next != NULL)
    roi_list_copy(&((*dest_roi_list)->next), src_roi_list->next);

  return;
}


void roi_free_points_list(GSList ** plist) {

  if (*plist == NULL)
    return;

  while ((*plist)->next != NULL)
    roi_free_points_list(&((*plist)->next));

  g_free((*plist)->data);

  g_free(*plist);

  *plist = NULL;

  return;
}

/* returns true if we haven't drawn this roi */
gboolean roi_undrawn(const amide_roi_t * roi) {
  
  return 
    REALPOINT_EQUAL(roi->coord_frame.offset,realpoint_init) &&
    REALPOINT_EQUAL(roi->corner,realpoint_init);
}
    

/* returns true if this voxel is in the given box */
gboolean roi_voxel_in_box(const realpoint_t p,
			  const realpoint_t p0,
			  const realpoint_t p1) {


  return (
	  (
	   (
	    ((p.z >= p0.z-CLOSE) && (p.z <= p1.z+CLOSE)) ||
	    ((p.z <= p0.z+CLOSE) && (p.z >= p1.z-CLOSE))) && 
	   (
	    ((p.y >= p0.y-CLOSE) && (p.y <= p1.y+CLOSE)) ||
	    ((p.y <= p0.y+CLOSE) && (p.y >= p1.y-CLOSE))) && 
	   (
	    ((p.x >= p0.x-CLOSE) && (p.x <= p1.x+CLOSE)) ||
	    ((p.x <= p0.x+CLOSE) && (p.x >= p1.x-CLOSE)))));

}


/* returns true if this voxel is in the elliptic cylinder */
gboolean roi_voxel_in_elliptic_cylinder(const realpoint_t p,
					const realpoint_t center,
					const floatpoint_t height,
					const realpoint_t radius) {

  return ((1.0+2*CLOSE >= 
	   (pow((p.x-center.x),2.0)/pow(radius.x,2.0) +
	    pow((p.y-center.y),2.0)/pow(radius.y,2.0)))
	  &&
	  ((p.z > (center.z-height/2.0)-CLOSE) && 
	   (p.z < (center.z+height/2.0)-CLOSE)));
  
}



/* returns true if this voxel is in the ellipsoid */
gboolean roi_voxel_in_ellipsoid(const realpoint_t p,
				const realpoint_t center,
				const realpoint_t radius) {

  	  
  return (1.0 + 3*CLOSE >= 
	  (pow((p.x-center.x),2.0)/pow(radius.x,2.0) +
	   pow((p.y-center.y),2.0)/pow(radius.y,2.0) +
	   pow((p.z-center.z),2.0)/pow(radius.z,2.0)));
}


/* returns a singly linked list of intersection points between the roi
   and the given slice.  returned points are in the slice's coord_frame */
GSList * roi_get_volume_intersection_points(const amide_volume_t * view_slice,
					    const amide_roi_t * roi) {


  GSList * return_points = NULL;
  GSList * new_point, * last_return_point=NULL;
  realpoint_t * ppoint;
  realpoint_t view_p,temp_p, center,radius, slice_corner[2], roi_corner[2];
  voxelpoint_t i;
  floatpoint_t height;
  gboolean voxel_in=FALSE, prev_voxel_intersection, saved=TRUE;

  /* sanity checks */
  g_assert(view_slice->dim.z == 1);

  /* make sure we've already defined this guy */
  if (roi_undrawn(roi))
    return NULL;

#ifdef AMIDE_DEBUG
  g_printerr("roi %s --------------------\n",roi->name);
  g_printerr("\t\toffset\tx %5.3f\ty %5.3f\tz %5.3f\n",
	     roi->coord_frame.offset.x,
	     roi->coord_frame.offset.y,
	     roi->coord_frame.offset.z);
  g_printerr("\t\tcorner\tx %5.3f\ty %5.3f\tz %5.3f\n",
	     roi->corner.x,roi->corner.y,roi->corner.z);
#endif

  switch(roi->type) {
  case GROUP:
    return NULL; 
    break;
  case BOX: /* yes, not the most efficient way to do this, but i didn't
	       feel like figuring out the math.... */
  case CYLINDER:
  case ELLIPSOID: 
  default: /* checking all voxels, looking for those on the edge of the object
	      of interest */

    /* get the roi corners in roi space */
    roi_corner[0] = realspace_base_coord_to_alt(roi->coord_frame.offset, roi->coord_frame);
    roi_corner[1] = roi->corner;

    /* figure out the center of the object in it's space*/
    REALSPACE_MADD(0.5,roi_corner[1], 0.5,roi_corner[0], center);   

    /* figure out the radius in each direction */
    REALSPACE_DIFF(roi_corner[1],roi_corner[0], radius);
    REALSPACE_CMULT(0.5,radius, radius);

    /* figure out the height */
    height = fabs(roi_corner[1].z-roi_corner[0].z);

    /* get the corners of the view slice in view coordinate space */
    slice_corner[0] = realspace_base_coord_to_alt(view_slice->coord_frame.offset,
						  view_slice->coord_frame);
    slice_corner[1] = view_slice->corner;

    /* iterate through the slice, putting all edge points in the list */
    i.z=0;
    view_p.z = (slice_corner[0].z+slice_corner[1].z)/2.0;
    view_p.y = slice_corner[0].y+view_slice->voxel_size.y/2.0;

    for (i.y=0; i.y < view_slice->dim.y ; i.y++) {
      view_p.x = slice_corner[0].x+view_slice->voxel_size.x/2.0;
      prev_voxel_intersection = FALSE;

      for (i.x=0; i.x < view_slice->dim.x ; i.x++) {
	temp_p = realspace_alt_coord_to_alt(view_p, 
					    view_slice->coord_frame,
					    roi->coord_frame);

	switch(roi->type) {
	case BOX: 
	  voxel_in = roi_voxel_in_box(temp_p, roi_corner[0],roi_corner[1]);
	  break;
	case CYLINDER:
	  voxel_in = roi_voxel_in_elliptic_cylinder(temp_p, center, 
						    height, radius);
	  break;
	case ELLIPSOID: 
	  voxel_in = roi_voxel_in_ellipsoid(temp_p,center,radius);
	  break;
	default:
	  g_printerr("roi type %d not fully implemented!\n",roi->type);
	};
	
	/* is it an edge */
	if (voxel_in != prev_voxel_intersection) {

	  /* than save the point */
	  saved = TRUE;
	  new_point = (GSList * ) g_malloc(sizeof(GSList));
	  ppoint = (realpoint_t * ) g_malloc(sizeof(realpoint_t));
	  *ppoint = view_p;
	 
	  if (voxel_in ) {
	    new_point->data = ppoint;
	    new_point->next = NULL;
	    if (return_points == NULL) /* is this the first point? */
	      return_points = new_point;
	    else
	      last_return_point->next = new_point;
	    last_return_point = new_point;
	  } else { /* previous voxel should be returned */
	    ppoint->x -= view_slice->voxel_size.x; /* backup one voxel */
	    new_point->data = ppoint;
	    new_point->next = return_points;
	    if (return_points == NULL) /* is this the first point? */
	      last_return_point = new_point;
	    return_points = new_point;
	  }
	} else {
	  saved = FALSE;
	}

	prev_voxel_intersection = voxel_in;
	view_p.x += view_slice->voxel_size.x; /* advance one voxel */
      }

      /* check if the edge of this row is still in the roi, if it is, add it as
	 a point */
      if ((voxel_in) && !(saved)) {

	/* than save the point */
	new_point = (GSList * ) g_malloc(sizeof(GSList));
	ppoint = (realpoint_t * ) g_malloc(sizeof(realpoint_t));
	*ppoint = view_p;
	ppoint->x -= view_slice->voxel_size.x; /* backup one voxel */
	new_point->data = ppoint;
	new_point->next = return_points;
	if (return_points == NULL) /* is this the first point? */
	  last_return_point = new_point;
	return_points = new_point;
      }

      view_p.y += view_slice->voxel_size.y; /* advance one row */
    }

    /* make sure the two ends meet */
    if (return_points != NULL) {
      new_point = (GSList * ) g_malloc(sizeof(GSList));
      ppoint = (realpoint_t * ) g_malloc(sizeof(realpoint_t));
      *ppoint = *((realpoint_t *) (last_return_point->data));

      new_point->data = ppoint;
      new_point->next = return_points;
      return_points = new_point;
    }

    break;

  }


  return return_points;
}



/* figure out the smallest subset of the given volume structure that the roi is 
   enclosed within */
void roi_subset_of_volume(amide_roi_t * roi,
			  amide_volume_t * volume,
			  voxelpoint_t * subset_start,
			  voxelpoint_t * subset_dim){

  realpoint_t roi_corner[2];
  realpoint_t subset_corner[2];
  voxelpoint_t subset_index[2];
  realpoint_t temp;

  g_assert(volume != NULL);
  g_assert(roi != NULL);

  /* set some values */
  roi_corner[0] = realspace_base_coord_to_alt(roi->coord_frame.offset,
					      roi->coord_frame);
  roi_corner[1] = roi->corner;



  /* look at all eight corners of the roi cube, figure out the max and min dim */

  /* corner 0 */
  temp = realspace_alt_coord_to_alt(roi_corner[0],
				    roi->coord_frame,
				    volume->coord_frame);
  subset_corner[0] = subset_corner[1] = temp;

  /* corner 1 */
  temp.x = roi_corner[0].x;
  temp.y = roi_corner[0].y;
  temp.z = roi_corner[1].z;
  temp = realspace_alt_coord_to_alt(temp,
				    roi->coord_frame,
				    volume->coord_frame);
  if (subset_corner[0].x > temp.x) subset_corner[0].x = temp.x;
  if (subset_corner[1].x < temp.x) subset_corner[1].x = temp.x;
  if (subset_corner[0].y > temp.y) subset_corner[0].y = temp.y;
  if (subset_corner[1].y < temp.y) subset_corner[1].y = temp.y;
  if (subset_corner[0].z > temp.z) subset_corner[0].z = temp.z;
  if (subset_corner[1].z < temp.z) subset_corner[1].z = temp.z;


  /* corner 2 */
  temp.x = roi_corner[0].x;
  temp.y = roi_corner[1].y;
  temp.z = roi_corner[0].z;
  temp = realspace_alt_coord_to_alt(temp,
				    roi->coord_frame,
				    volume->coord_frame);
  if (subset_corner[0].x > temp.x) subset_corner[0].x = temp.x;
  if (subset_corner[1].x < temp.x) subset_corner[1].x = temp.x;
  if (subset_corner[0].y > temp.y) subset_corner[0].y = temp.y;
  if (subset_corner[1].y < temp.y) subset_corner[1].y = temp.y;
  if (subset_corner[0].z > temp.z) subset_corner[0].z = temp.z;
  if (subset_corner[1].z < temp.z) subset_corner[1].z = temp.z;


  /* corner 3 */
  temp.x = roi_corner[0].x;
  temp.y = roi_corner[1].y;
  temp.z = roi_corner[1].z;
  temp = realspace_alt_coord_to_alt(temp,
				    roi->coord_frame,
				    volume->coord_frame);
  if (subset_corner[0].x > temp.x) subset_corner[0].x = temp.x;
  if (subset_corner[1].x < temp.x) subset_corner[1].x = temp.x;
  if (subset_corner[0].y > temp.y) subset_corner[0].y = temp.y;
  if (subset_corner[1].y < temp.y) subset_corner[1].y = temp.y;
  if (subset_corner[0].z > temp.z) subset_corner[0].z = temp.z;
  if (subset_corner[1].z < temp.z) subset_corner[1].z = temp.z;


  /* corner 4 */
  temp.x = roi_corner[1].x;
  temp.y = roi_corner[0].y;
  temp.z = roi_corner[0].z;
  temp = realspace_alt_coord_to_alt(temp,
				    roi->coord_frame,
				    volume->coord_frame);
  if (subset_corner[0].x > temp.x) subset_corner[0].x = temp.x;
  if (subset_corner[1].x < temp.x) subset_corner[1].x = temp.x;
  if (subset_corner[0].y > temp.y) subset_corner[0].y = temp.y;
  if (subset_corner[1].y < temp.y) subset_corner[1].y = temp.y;
  if (subset_corner[0].z > temp.z) subset_corner[0].z = temp.z;
  if (subset_corner[1].z < temp.z) subset_corner[1].z = temp.z;


  /* corner 5 */
  temp.x = roi_corner[1].x;
  temp.y = roi_corner[0].y;
  temp.z = roi_corner[1].z;
  temp = realspace_alt_coord_to_alt(temp,
				    roi->coord_frame,
				    volume->coord_frame);
  if (subset_corner[0].x > temp.x) subset_corner[0].x = temp.x;
  if (subset_corner[1].x < temp.x) subset_corner[1].x = temp.x;
  if (subset_corner[0].y > temp.y) subset_corner[0].y = temp.y;
  if (subset_corner[1].y < temp.y) subset_corner[1].y = temp.y;
  if (subset_corner[0].z > temp.z) subset_corner[0].z = temp.z;
  if (subset_corner[1].z < temp.z) subset_corner[1].z = temp.z;


  /* corner 6 */
  temp.x = roi_corner[1].x;
  temp.y = roi_corner[1].y;
  temp.z = roi_corner[0].z;
  temp = realspace_alt_coord_to_alt(temp,
				    roi->coord_frame,
				    volume->coord_frame);
  if (subset_corner[0].x > temp.x) subset_corner[0].x = temp.x;
  if (subset_corner[1].x < temp.x) subset_corner[1].x = temp.x;
  if (subset_corner[0].y > temp.y) subset_corner[0].y = temp.y;
  if (subset_corner[1].y < temp.y) subset_corner[1].y = temp.y;
  if (subset_corner[0].z > temp.z) subset_corner[0].z = temp.z;
  if (subset_corner[1].z < temp.z) subset_corner[1].z = temp.z;


  /* corner 7 */
  temp.x = roi_corner[1].x;
  temp.y = roi_corner[1].y;
  temp.z = roi_corner[1].z;
  temp = realspace_alt_coord_to_alt(temp,
				    roi->coord_frame,
				    volume->coord_frame);
  if (subset_corner[0].x > temp.x) subset_corner[0].x = temp.x;
  if (subset_corner[1].x < temp.x) subset_corner[1].x = temp.x;
  if (subset_corner[0].y > temp.y) subset_corner[0].y = temp.y;
  if (subset_corner[1].y < temp.y) subset_corner[1].y = temp.y;
  if (subset_corner[0].z > temp.z) subset_corner[0].z = temp.z;
  if (subset_corner[1].z < temp.z) subset_corner[1].z = temp.z;


  /* and convert the subset_corners into indexes */
  VOLUME_REALPOINT_TO_INDEX(volume,subset_corner[0],subset_index[0]);
  VOLUME_REALPOINT_TO_INDEX(volume,subset_corner[1],subset_index[1]);

  /* sanity checks */
  if (subset_index[0].x < 0) subset_index[0].x = 0;
  if (subset_index[0].x > volume->dim.x) subset_index[0].x = volume->dim.x;
  if (subset_index[0].y < 0) subset_index[0].y = 0;
  if (subset_index[0].y > volume->dim.z) subset_index[0].y = volume->dim.y;
  if (subset_index[0].z < 0) subset_index[0].z = 0;
  if (subset_index[0].z > volume->dim.z) subset_index[0].z = volume->dim.z;
  if (subset_index[1].x < 0) subset_index[1].x = 0;
  if (subset_index[1].x > volume->dim.x) subset_index[1].x = volume->dim.x;
  if (subset_index[1].y < 0) subset_index[1].y = 0;
  if (subset_index[1].y > volume->dim.z) subset_index[1].y = volume->dim.y;
  if (subset_index[1].z < 0) subset_index[1].z = 0;
  if (subset_index[1].z > volume->dim.z) subset_index[1].z = volume->dim.z;

  /* and calculate the return values */
  *subset_start = subset_index[0];
  REALSPACE_SUB(subset_index[1],subset_index[0],*subset_dim);

  return;
}


/* calculate an analysis of several statistical values for an roi on
   a given volume.
   note: the "grain" input indicates how many subvoxels we'll divide
   each voxel up into when calculating our statistics.  Setting the grain
   to something more than 1 [GRAIN_1], such as 8 [GRAIN_8] or 64 [GRAIN_64]
   may be useful for getting accurate statistics on small roi's */
amide_roi_analysis_t roi_calculate_analysis(amide_roi_t * roi, 
					    amide_volume_t * volume,
					    roi_grain_t grain,
					    guint frame) {
  
  amide_roi_analysis_t analysis;
  realpoint_t roi_corner[2];
  realpoint_t center, radius,roi_p, volume_p;
  floatpoint_t height;
  intpoint_t voxel_grain_steps;
  volume_data_t temp_data, voxel_grain_size;
  voxelpoint_t i,j;
  voxelpoint_t init,dim;
  gboolean voxel_in;

#ifdef AMIDE_DEBUG
  g_print("Calculating ROI %s on Volume %s using Grain %s\n",
	  roi->name, volume->name, roi_grain_names[roi->grain]);
#endif
  
  /* get the roi corners in roi space */
  roi_corner[0] = realspace_base_coord_to_alt(roi->coord_frame.offset,
					      roi->coord_frame);
  roi_corner[1] = roi->corner;

  /* figure out the center of the object in it's space*/
  REALSPACE_MADD(0.5,roi_corner[1], 0.5,roi_corner[0], center);
  
  /* figure out the radius in each direction */
  REALSPACE_DIFF(roi_corner[1],roi_corner[0], radius);
  REALSPACE_CMULT(0.5,radius, radius);
  
  /* figure out the height */
  height = fabs(roi_corner[1].z-roi_corner[0].z);
  
  /* initialize the analysis data structure based on the center of the
     roi, we always assume at least this voxel is in..... */
  volume_p =
    realspace_alt_coord_to_alt(center,
			       roi->coord_frame,
			       volume->coord_frame);
  VOLUME_REALPOINT_TO_INDEX(volume,volume_p,i);
  temp_data = VOLUME_CONTENTS(volume, frame, i);
  
  analysis.voxels = 1.0; 
  analysis.mean = 0.0;
  analysis.min = analysis.max = temp_data;
  analysis.var = 0;

  /* sanity checks */
  if (roi_undrawn(roi)) {
    g_warning("%s: roi appears not to have been drawn\n",PACKAGE);
    return analysis;
  }

  /* figure out what portion of the volume we'll be iterating over */
  roi_subset_of_volume(roi,volume,&init,&dim);

  /* set some variables based on or grain size */
  switch(grain) {
  case GRAINS_64:
    voxel_grain_size = 1.0/64.0;
    voxel_grain_steps = 4; /* the cube root of 64 */
  break;
  case GRAINS_27:
    voxel_grain_size = 1.0/27.0;
    voxel_grain_steps = 3; /* the cube root of 27 */
  break;
  case GRAINS_8:
    voxel_grain_size = 1.0/8.0;
    voxel_grain_steps = 2; /* the cube root of 8 */
  break;
  case GRAINS_1:
  default:
    voxel_grain_size = 1.0;
    voxel_grain_steps = 1; /* the cube root of 1 */
  break;
  }

  /* alright, iterate through the roi space and calculate our analysis values
   * the index i corresponds to the voxel, the index j corresponds to the 
   * subvoxel within that voxel */
  for (i.z = init.z; i.z<(init.z+dim.z);i.z++) {
    for (j.z = 0 ; j.z < voxel_grain_steps ; j.z++) {
      volume_p.z = i.z*volume->voxel_size.z + 
	(j.z+1)*volume->voxel_size.z/(voxel_grain_steps+1);

      for (i.y = init.y; i.y<(init.y+dim.y);i.y++) {
	for (j.y = 0 ; j.y < voxel_grain_steps ; j.y++) {
	  volume_p.y = i.y*volume->voxel_size.y + 
	    (j.y+1)*volume->voxel_size.y/(voxel_grain_steps+1);

	  for (i.x = init.x; i.x<(init.x+dim.x);i.x++) {
	    for (j.x = 0 ; j.x < voxel_grain_steps ; j.x++) {
	      volume_p.x = i.x*volume->voxel_size.x + 
		(j.x+1)*volume->voxel_size.x/(voxel_grain_steps+1);

	      /* get the corresponding roi point */
	      roi_p = realspace_alt_coord_to_alt(volume_p,
						 volume->coord_frame,
						 roi->coord_frame);

	      /* determine if it's in or not */
	      switch(roi->type) {
	      case BOX: 
		voxel_in = roi_voxel_in_box(roi_p, roi_corner[0],roi_corner[1]);
		break;
	      case CYLINDER:
		voxel_in = roi_voxel_in_elliptic_cylinder(roi_p, center, 
							  height, radius);
		break;
	      case ELLIPSOID: 
		voxel_in = roi_voxel_in_ellipsoid(roi_p,center,radius);
		break;
	      default:
		g_printerr("roi type %d not fully implemented!\n",roi->type);
		return analysis;
		break;
	      };
	      
	      if (voxel_in) {
		temp_data = VOLUME_CONTENTS(volume,frame,i);
	  
		analysis.mean += temp_data*voxel_grain_size;
		analysis.voxels += voxel_grain_size;
		if ((temp_data) < analysis.min)
		  analysis.min = temp_data;
		if ((temp_data) > analysis.max)
		  analysis.max = temp_data;
	      }
	    }
	  }
	}
      }
    }
  }

  /* finish mean calculation */
  analysis.mean /= analysis.voxels;
  
  /* now iterate over the volume again to calculate the variance.... */
  for (i.z = init.z; i.z<(init.z+dim.z);i.z++) {
    for (j.z = 0 ; j.z < voxel_grain_steps ; j.z++) {
      volume_p.z = i.z*volume->voxel_size.z + 
	(j.z+1)*volume->voxel_size.z/(voxel_grain_steps+1);

      for (i.y = init.y; i.y<(init.y+dim.y);i.y++) {
	for (j.y = 0 ; j.y < voxel_grain_steps ; j.y++) {
	  volume_p.y = i.y*volume->voxel_size.y + 
	    (j.y+1)*volume->voxel_size.y/(voxel_grain_steps+1);

	  for (i.x = init.x; i.x<(init.x+dim.x);i.x++) {
	    for (j.x = 0 ; j.x < voxel_grain_steps ; j.x++) {
	      volume_p.x = i.x*volume->voxel_size.x + 
		(j.x+1)*volume->voxel_size.x/(voxel_grain_steps+1);

	      /* get the corresponding roi point */
	      roi_p = realspace_alt_coord_to_alt(volume_p,
						 volume->coord_frame,
						 roi->coord_frame);

	      /* determine if it's in or not */
	      switch(roi->type) {
	      case BOX: 
		voxel_in = roi_voxel_in_box(roi_p, roi_corner[0],roi_corner[1]);
		break;
	      case CYLINDER:
		voxel_in = roi_voxel_in_elliptic_cylinder(roi_p, center, 
							  height, radius);
		break;
	      case ELLIPSOID: 
		voxel_in = roi_voxel_in_ellipsoid(roi_p,center,radius);
		break;
	      default:
		g_printerr("roi type %d not fully implemented!\n",roi->type);
		return analysis;
		break;
	      };
	      
	      if (voxel_in) {
		temp_data = VOLUME_CONTENTS(volume,frame,i);
		
		analysis.var += 
		  voxel_grain_size*pow((temp_data-analysis.mean),2.0);
	      }
	    }
	  }
	}
      }
    }
  }

  /* and divide to get the final var, not I'm using N and not (N-1), cause
     my mean is calculated from the whole data set and not just a sample of it.
     If anyone else with more statistical experience disagrees, please speak
     up */
  analysis.var /= analysis.var/analysis.voxels;

  /* and make sure we don't double count the center voxel if we don't have to */
  if (analysis.voxels >= 1.0+CLOSE)
    analysis.voxels -= 1.0;

  return analysis;
}





