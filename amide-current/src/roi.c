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
#include <sys/stat.h>
#include "volume.h"
#include "roi.h"

/* external variables */
gchar * roi_type_names[] = {
  "Ellipsoid", 
  "Elliptic Cylinder", 
  "Box",
  "2D Isocontour",
  "3D Isocontour"
};

gchar * roi_menu_names[] = {
  "_Ellipsoid", 
  "Elliptic _Cylinder", 
  "_Box",
  "_2D Isocontour",
  "_3D Isocontour",
};
gchar * roi_menu_explanation[] = {
  "Add a new elliptical ROI", 
  "Add a new elliptic cylinder ROI", 
  "Add a new box shaped ROI",
  "Add a new 2D Isocontour ROI",
  "Add a new 3D Isocontour ROI",
};

/* free up an roi */
roi_t * roi_free(roi_t * roi) {
  
  if (roi == NULL)
    return roi;

  /* sanity checks */
  g_return_val_if_fail(roi->reference_count > 0, NULL);

  roi->reference_count--; /* remove a reference count */

  /* if we've removed all reference's, free the roi */
  if (roi->reference_count == 0) {
    roi->children = roi_list_free(roi->children);
    roi->isocontour = data_set_free(roi->isocontour);
#ifdef AMIDE_DEBUG
    //    g_print("freeing roi: %s\n",roi->name);
#endif
    g_free(roi->name);
    g_free(roi);
    roi = NULL;
  }

  return roi;
}

/* returns an initialized roi structure */
roi_t * roi_init(void) {
  
  roi_t * temp_roi;
  
  if ((temp_roi = 
       (roi_t *) g_malloc(sizeof(roi_t))) == NULL) {
    return NULL;
  }
  temp_roi->reference_count = 1;
  
  temp_roi->name = NULL;
  temp_roi->corner = zero_rp;
  rs_set_offset(&temp_roi->coord_frame, zero_rp);
  rs_set_axis(&temp_roi->coord_frame, default_axis);
  temp_roi->parent = NULL;
  temp_roi->children = NULL;

  temp_roi->isocontour = NULL;
  temp_roi->voxel_size = zero_rp;
  temp_roi->isocontour_value = EMPTY;
  
  return temp_roi;
}

/* function to write out the information content of an roi into an xml
   file.  Returns a string containing the name of the file. */
gchar * roi_write_xml(roi_t * roi, gchar * study_directory) {

  gchar * roi_xml_filename;
  guint count;
  struct stat file_info;
  xmlDocPtr doc;
  xmlNodePtr roi_nodes;
  gchar * isocontour_name;
  gchar * isocontour_xml_filename;

  /* make a guess as to our filename */
  count = 1;
  roi_xml_filename = g_strdup_printf("ROI_%s.xml", roi->name);

  /* see if this file already exists */
  while (stat(roi_xml_filename, &file_info) == 0) {
    g_free(roi_xml_filename);
    count++;
    roi_xml_filename = g_strdup_printf("ROI_%s_%d.xml", roi->name, count);
  }

  /* and we now have a unique filename */
#ifdef AMIDE_DEBUG
  g_print("\t- saving roi %s in roi %s\n",roi->name, roi_xml_filename);
#endif

  /* write the roi xml file */
  doc = xmlNewDoc("1.0");
  doc->children = xmlNewDocNode(doc, NULL, "ROI", roi->name);
  xml_save_string(doc->children, "type", roi_type_names[roi->type]);
  xml_save_realspace(doc->children, "coord_frame", roi->coord_frame);
  xml_save_realpoint(doc->children, "corner", roi->corner);
  xml_save_realpoint(doc->children, "voxel_size", roi->voxel_size);
  xml_save_floatpoint(doc->children, "isocontour_value", roi->isocontour_value);

  if ((roi->type == ISOCONTOUR_2D) || (roi->type == ISOCONTOUR_3D)) {
    isocontour_name = g_strdup_printf("Isocontour_%s", roi->name);
    isocontour_xml_filename = data_set_write_xml(roi->isocontour, study_directory, isocontour_name);
    g_free(isocontour_name);
    xml_save_string(doc->children,"isocontour_file", isocontour_xml_filename);
    g_free(isocontour_xml_filename);
  } else {
    xml_save_string(doc->children,"isocontour_file", NULL);
  }

  /* save our children */
  roi_nodes = xmlNewChild(doc->children, NULL, "Children", NULL);
  roi_list_write_xml(roi->children, roi_nodes, study_directory);

  /* and save */
  xmlSaveFile(roi_xml_filename, doc);

  /* and we're done */
  xmlFreeDoc(doc);

  return roi_xml_filename;
}

/* function to load in an ROI xml file */
roi_t * roi_load_xml(gchar * roi_xml_filename, const gchar * study_directory) {

  xmlDocPtr doc;
  roi_t * new_roi;
  xmlNodePtr nodes;
  xmlNodePtr children_nodes;
  roi_type_t i_roi_type;
  gchar * temp_string;
  gchar * isocontour_xml_filename;

  new_roi = roi_init();

  /* parse the xml file */
  if ((doc = xmlParseFile(roi_xml_filename)) == NULL) {
    g_warning("%s: Couldn't Parse AMIDE ROI xml file %s/%s",PACKAGE, study_directory,roi_xml_filename);
    roi_free(new_roi);
    return new_roi;
  }

  /* get the root of our document */
  if ((nodes = xmlDocGetRootElement(doc)) == NULL) {
    g_warning("%s: AMIDE ROI xml file doesn't appear to have a root: %s/%s",
	      PACKAGE, study_directory,roi_xml_filename);
    roi_free(new_roi);
    return new_roi;
  }

  /* get the roi name */
  temp_string = xml_get_string(nodes->children, "text");
  if (temp_string != NULL) {
    roi_set_name(new_roi,temp_string);
    g_free(temp_string);
  }

  /* get the document tree */
  nodes = nodes->children;

  /* figure out the type */
  temp_string = xml_get_string(nodes, "type");
  if (temp_string != NULL)
    for (i_roi_type=0; i_roi_type < NUM_ROI_TYPES; i_roi_type++) 
      if (g_strcasecmp(temp_string, roi_type_names[i_roi_type]) == 0)
	new_roi->type = i_roi_type;
  g_free(temp_string);

  /* and figure out the rest of the parameters */
  new_roi->coord_frame = xml_get_realspace(nodes, "coord_frame");
  new_roi->corner = xml_get_realpoint(nodes, "corner");

  /* isocontour specific stuff */
  if ((new_roi->type == ISOCONTOUR_2D) || (new_roi->type == ISOCONTOUR_3D)) {
    new_roi->voxel_size = xml_get_realpoint(nodes, "voxel_size");
    new_roi->isocontour_value = xml_get_floatpoint(nodes, "isocontour_value");

    isocontour_xml_filename = xml_get_string(nodes, "isocontour_file");
    if (isocontour_xml_filename != NULL)
      new_roi->isocontour = data_set_load_xml(isocontour_xml_filename, study_directory);
  }

  /* and get any children */
  temp_string = xml_get_string(nodes->children, "Children");
  if (temp_string != NULL) {
    children_nodes = xml_get_node(nodes->children, "Children");
    new_roi->children = roi_list_load_xml(children_nodes, study_directory);
  }
  g_free(temp_string);
   
  /* and we're done */
  xmlFreeDoc(doc);


  return new_roi;
}


/* makes a new roi item which is a copy of a previous roi's information. */
roi_t * roi_copy(roi_t * src_roi) {

  roi_t * dest_roi;

  /* sanity checks */
  g_return_val_if_fail(src_roi != NULL, NULL);

  dest_roi = roi_init();

  /* copy the data elements */
  dest_roi->type = src_roi->type;
  dest_roi->coord_frame = src_roi->coord_frame;
  dest_roi->corner = src_roi->corner;
  dest_roi->parent = src_roi->parent;
  dest_roi->voxel_size = src_roi->voxel_size;
  dest_roi->isocontour_value = src_roi->isocontour_value;
  dest_roi->isocontour = data_set_add_reference(src_roi->isocontour);
  
  /* make a separate copy in memory of the roi's children */
  if (src_roi->children != NULL)
    dest_roi->children = roi_list_copy(src_roi->children);

  /* make a separate copy in memory of the roi's name */
  roi_set_name(dest_roi, src_roi->name);

  return dest_roi;
}

/* adds one to the reference count of an roi */
roi_t * roi_add_reference(roi_t * roi) {

  roi->reference_count++;

  return roi;
}

/* sets the name of an roi
   note: new_name is copied rather then just being referenced by roi */
void roi_set_name(roi_t * roi, gchar * new_name) {

  g_free(roi->name); /* free up the memory used by the old name */
  roi->name = g_strdup(new_name); /* and assign the new name */

  return;
}

/* figure out the center of the roi in real coords */
realpoint_t roi_calculate_center(const roi_t * roi) {

  realpoint_t center;
  realpoint_t corner[2];

  /* get the far corner (in roi coords) */
  corner[0] = realspace_base_coord_to_alt(rs_offset(roi->coord_frame), roi->coord_frame);
  corner[1] = roi->corner;
 
  /* the center in roi coords is then just half the far corner */
  center = rp_add(rp_cmult(0.5,corner[1]), rp_cmult(0.5,corner[0]));
  
  /* now, translate that into real coords */
  center = realspace_alt_coord_to_base(center, roi->coord_frame);

  return center;
}

/* takes an roi and a view_axis, and gives the corners necessary for 
   the view to totally encompass the roi in the view coord frame */
void roi_get_view_corners(const roi_t * roi,
			  const realspace_t view_coord_frame,
			  realpoint_t view_corner[]) {

  realpoint_t corner[2];

  g_assert(roi!=NULL);

  corner[0] = realspace_base_coord_to_alt(rs_offset(roi->coord_frame),roi->coord_frame);
  corner[1] = roi->corner;

  /* look at all eight corners of our cube, figure out the min and max coords */
  realspace_get_enclosing_corners(roi->coord_frame, corner, view_coord_frame, view_corner);
  return;
}


/* free up an roi list */
roi_list_t * roi_list_free(roi_list_t * roi_list) {

  roi_list_t * return_list;

  if (roi_list == NULL) return roi_list;

  /* sanity check */
  g_return_val_if_fail(roi_list->reference_count > 0, NULL);

  /* remove a reference count */
  roi_list->reference_count--;

  /* stuff to do if reference count is zero */
  if (roi_list->reference_count == 0) {
    /* recursively delete rest of list */
    return_list = roi_list_free(roi_list->next);
    roi_list->next = NULL;
    roi_list->roi = roi_free(roi_list->roi);
    g_free(roi_list);
    roi_list = NULL;
  } else
    return_list = roi_list;

  return return_list;
}

/* returns an initialized roi list node structure */
roi_list_t * roi_list_init(roi_t * roi) {
  
  roi_list_t * temp_roi_list;
  
  if ((temp_roi_list = 
       (roi_list_t *) g_malloc(sizeof(roi_list_t))) == NULL) {
    return NULL;
  }
  temp_roi_list->reference_count = 1;

  temp_roi_list->next = NULL;
  temp_roi_list->roi = roi_add_reference(roi);
  
  return temp_roi_list;
}

/* count the number of roi's in the roi list */
guint roi_list_count(roi_list_t * list) {
  if (list == NULL) return 0;
  else return (1+roi_list_count(list->next)+roi_list_count(list->roi->children));
}

/* function to write a list of rois as xml data.  Function calls
   roi_write_xml to writeout each roi, and adds information about the
   roi file to the node_list. */
void roi_list_write_xml(roi_list_t *list, xmlNodePtr node_list, gchar * study_directory) {

  gchar * roi_xml_filename;

  if (list != NULL) { 
    roi_xml_filename = roi_write_xml(list->roi, study_directory);
    xmlNewChild(node_list, NULL, "ROI_file", roi_xml_filename);
    g_free(roi_xml_filename);
    
    /* and recurse */
    roi_list_write_xml(list->next, node_list, study_directory);
  }

  return;
}

/* function to load in a list of ROI xml nodes */
roi_list_t * roi_list_load_xml(xmlNodePtr node_list, const gchar * study_directory) {

  gchar * roi_xml_filename;
  roi_list_t * new_roi_list;
  roi_t * new_roi;

  if (node_list != NULL) {
    /* first, recurse on through the list */
    new_roi_list = roi_list_load_xml(node_list->next, study_directory);

    /* load in this node */
    roi_xml_filename = xml_get_string(node_list->children, "text");
    new_roi = roi_load_xml(roi_xml_filename,study_directory);
    new_roi_list = roi_list_add_roi_first(new_roi_list, new_roi);
    new_roi = roi_free(new_roi);
    g_free(roi_xml_filename);

  } else
    new_roi_list = NULL;

  return new_roi_list;
}

/* function to check that an roi is in an roi list */
gboolean roi_list_includes_roi(roi_list_t *list, roi_t * roi) {

  while (list != NULL)
    if (list->roi == roi)
      return TRUE;
    else
      list = list->next;

  return FALSE;
}

/* function to add an roi onto an roi list */
roi_list_t * roi_list_add_roi(roi_list_t * roi_list, roi_t * roi) {

  roi_list_t * temp_list = roi_list;
  roi_list_t * prev_list = NULL;


  /* get to the end of the list */
  while (temp_list != NULL) {
    prev_list = temp_list;
    temp_list = temp_list->next;
  }
  
  /* get a new roi_list data structure */
  temp_list = roi_list_init(roi);

  if (roi_list == NULL)
    return temp_list;
  else {
    prev_list->next = temp_list;
    return roi_list;
  }
}


/* function to add an roi onto an roi list as the first item*/
roi_list_t * roi_list_add_roi_first(roi_list_t * roi_list, roi_t * roi) {

  roi_list_t * temp_list;

  temp_list = roi_list_add_roi(NULL,roi);
  temp_list->next = roi_list;

  return temp_list;
}


/* function to remove an roi from an roi list */
roi_list_t * roi_list_remove_roi(roi_list_t * roi_list, roi_t * roi) {

  roi_list_t * temp_list = roi_list;
  roi_list_t * prev_list = NULL;

  while (temp_list != NULL) {
    if (temp_list->roi == roi) {
      if (prev_list == NULL)
	roi_list = temp_list->next;
      else
	prev_list->next = temp_list->next;
      temp_list->next = NULL;
      temp_list = roi_list_free(temp_list);
    } else {
      prev_list = temp_list;
      temp_list = temp_list->next;
    }
  }

  return roi_list;
}

/* makes a new roi_list which is a copy of a previous roi_list */
roi_list_t * roi_list_copy(roi_list_t * src_roi_list) {

  roi_list_t * dest_roi_list;
  roi_t * temp_roi;

  /* sanity check */
  g_return_val_if_fail(src_roi_list != NULL, NULL);

  /* make a separate copy in memory of the roi this list item points to */
  temp_roi = roi_copy(src_roi_list->roi); 

  dest_roi_list = roi_list_init(temp_roi); 
  temp_roi = roi_free(temp_roi); /* no longer need a reference outside of the list's reference */

  /* and make copies of the rest of the elements in this list */
  if (src_roi_list->next != NULL)
    dest_roi_list->next = roi_list_copy(src_roi_list->next);

  return dest_roi_list;
}

/* adds one to the reference count of an roi list*/
roi_list_t * roi_list_add_reference(roi_list_t * rois) {

  rois->reference_count++;

  return rois;
}

/* takes a list of rois and a view axis, and give the corners
   necessary to totally encompass the roi in the view coord frame */
void rois_get_view_corners(roi_list_t * rois,
			   const realspace_t view_coord_frame,
			   realpoint_t view_corner[]) {

  roi_list_t * temp_rois;
  realpoint_t temp_corner[2];

  g_assert(rois!=NULL);

  temp_rois = rois;
  roi_get_view_corners(temp_rois->roi,view_coord_frame,view_corner);
  temp_rois = rois->next;

  while (temp_rois != NULL) {
    roi_get_view_corners(temp_rois->roi,view_coord_frame,temp_corner);
    view_corner[0].x = (view_corner[0].x < temp_corner[0].x) ? view_corner[0].x : temp_corner[0].x;
    view_corner[0].y = (view_corner[0].y < temp_corner[0].y) ? view_corner[0].y : temp_corner[0].y;
    view_corner[0].z = (view_corner[0].z < temp_corner[0].z) ? view_corner[0].z : temp_corner[0].z;
    view_corner[1].x = (view_corner[1].x > temp_corner[1].x) ? view_corner[1].x : temp_corner[1].x;
    view_corner[1].y = (view_corner[1].y > temp_corner[1].y) ? view_corner[1].y : temp_corner[1].y;
    view_corner[1].z = (view_corner[1].z > temp_corner[1].z) ? view_corner[1].z : temp_corner[1].z;
    temp_rois = temp_rois->next;
  }

  return;
}


GSList * roi_free_points_list(GSList * list) {

  realpoint_t * ppoint;

  if (list == NULL) return list;

  list->next = roi_free_points_list(list->next); /* recurse */

  ppoint = (realpoint_t *) list->data;

  list = g_slist_remove(list, ppoint);

  g_free(ppoint);

  return list;
}

/* returns true if we haven't drawn this roi */
gboolean roi_undrawn(const roi_t * roi) {
  
  if ((roi->type == ISOCONTOUR_2D) ||
      (roi->type == ISOCONTOUR_3D))
    return (roi->isocontour == NULL);
  else
    return 
      REALPOINT_EQUAL(rs_offset(roi->coord_frame),zero_rp) &&
      REALPOINT_EQUAL(roi->corner,zero_rp);
}
    
/* returns the maximal minimum voxel dimensions of a list of rois */
floatpoint_t rois_max_min_voxel_size(roi_list_t * rois) {

  floatpoint_t current_min_voxel_size, next_min_voxel_size;

  g_assert(rois!=NULL);
  g_assert(rois->roi!=NULL);

  if ((rois->roi->type == ISOCONTOUR_2D) || (rois->roi->type == ISOCONTOUR_3D)) {
    current_min_voxel_size = rp_min_dim(rois->roi->voxel_size);
    if (2.0*current_min_voxel_size > rp_min_dim(rois->roi->corner)) /* thin roi correction */
      current_min_voxel_size = rp_min_dim(rois->roi->corner)/2.0;
  } else
    current_min_voxel_size = rp_min_dim(rois->roi->corner)/50.0;

  if (rois->next == NULL) 
    next_min_voxel_size = current_min_voxel_size;
  else  
    next_min_voxel_size = rois_max_min_voxel_size(rois->next);

  if (next_min_voxel_size > current_min_voxel_size) 
    current_min_voxel_size = next_min_voxel_size;

  return current_min_voxel_size;
}


/* returns a singly linked list of intersection points between the roi
   and the given slice.  returned points are in the slice's coord_frame.
   note: use this function for ELLIPSOID, CYLINDER, and BOX 
*/
GSList * roi_get_intersection_line(const roi_t * roi,  const volume_t * view_slice) {

  GSList * return_points = NULL;

  switch(roi->type) {
  case ELLIPSOID:
    return_points = roi_ELLIPSOID_get_intersection_line(roi, view_slice);
    break;
  case CYLINDER:
    return_points = roi_CYLINDER_get_intersection_line(roi, view_slice);
    break;
  case BOX:
    return_points = roi_BOX_get_intersection_line(roi, view_slice);
    break;
  default: 
    g_warning("%s: roi type %d not implemented!",PACKAGE, roi->type);
    break;
  }


  return return_points;
}

/* returns a slice (in a volume structure) containing a  
   data set defining the edges of the roi in the given slice.
   returned data set is in the slice's coord frame.
*/
volume_t * roi_get_slice(const roi_t * roi, const volume_t * view_slice) {
  
  volume_t * intersection = NULL;

  switch(roi->type) {
  case ISOCONTOUR_2D:
    intersection = roi_ISOCONTOUR_2D_get_slice(roi, view_slice);
    break;
  case ISOCONTOUR_3D:
    intersection = roi_ISOCONTOUR_3D_get_slice(roi, view_slice);
    break;
  default: 
    g_warning("%s: roi type %d not implemented!",PACKAGE, roi->type);
    break;
  }


  return intersection;

}


/* figure out the smallest subset of the given volume structure that the roi is 
   enclosed within */
void roi_subset_of_volume(const roi_t * roi,
			  const volume_t * volume,
			  intpoint_t frame,
			  voxelpoint_t * subset_start,
			  voxelpoint_t * subset_dim){

  realpoint_t roi_corner[2];
  realpoint_t subset_corner[2];
  voxelpoint_t subset_index[2];

  g_assert(volume != NULL);
  g_assert(roi != NULL);

  /* set some values */
  roi_corner[0] = realspace_base_coord_to_alt(rs_offset(roi->coord_frame),
					      roi->coord_frame);
  roi_corner[1] = roi->corner;

  /* look at all eight corners of the roi cube, figure out the max and min dim */
  realspace_get_enclosing_corners(roi->coord_frame, roi_corner,
				  volume->coord_frame, subset_corner);

  /* and convert the subset_corners into indexes */
  VOLUME_REALPOINT_TO_VOXEL(volume, subset_corner[0], 0, subset_index[0]);
  VOLUME_REALPOINT_TO_VOXEL(volume, subset_corner[1], 0, subset_index[1]);

  /* and add one, as we want to completely enclose the roi */
  subset_index[1].x += 1;
  subset_index[1].y += 1;
  subset_index[1].z += 1;
  

  /* reduce in size if possible */
  if (volume->data_set != NULL) {
    if (subset_index[0].x < 0) subset_index[0].x = 0;
    if (subset_index[0].x > volume->data_set->dim.x) subset_index[0].x = volume->data_set->dim.x;
    if (subset_index[0].y < 0) subset_index[0].y = 0;
    if (subset_index[0].y > volume->data_set->dim.y) subset_index[0].y = volume->data_set->dim.y;
    if (subset_index[0].z < 0) subset_index[0].z = 0;
    if (subset_index[0].z > volume->data_set->dim.z) subset_index[0].z = volume->data_set->dim.z;
    if (subset_index[1].x < 0) subset_index[1].x = 0;
    if (subset_index[1].x > volume->data_set->dim.x) subset_index[1].x = volume->data_set->dim.x;
    if (subset_index[1].y < 0) subset_index[1].y = 0;
    if (subset_index[1].y > volume->data_set->dim.y) subset_index[1].y = volume->data_set->dim.y;
    if (subset_index[1].z < 0) subset_index[1].z = 0;
    if (subset_index[1].z > volume->data_set->dim.z) subset_index[1].z = volume->data_set->dim.z;
  }

  /* and calculate the return values */
  *subset_start = subset_index[0];
  subset_start->t = frame;
  REALPOINT_SUB(subset_index[1], subset_index[0], *subset_dim);
  subset_dim->t=1;

  return;
}



/* sets/resets the isocontour value of an isocontour ROI based on the given volume and voxel 
   note: vol should be a slice for the case of ISOCONTOUR_2D */
void roi_set_isocontour(roi_t * roi, volume_t * vol, voxelpoint_t value_vp) {

  g_return_if_fail((roi->type == ISOCONTOUR_2D) || (roi->type == ISOCONTOUR_3D));

  
  switch(roi->type) {
  case ISOCONTOUR_2D:
    roi_ISOCONTOUR_2D_set_isocontour(roi, vol, value_vp);
    break;
  case ISOCONTOUR_3D:
  default:
    roi_ISOCONTOUR_3D_set_isocontour(roi, vol, value_vp);
    break;
  }

  return;
}

/* sets an area in the roi to zero (not in the roi) */
void roi_isocontour_erase_area(roi_t * roi, voxelpoint_t erase_vp, gint area_size) {

  g_return_if_fail((roi->type == ISOCONTOUR_2D) || (roi->type == ISOCONTOUR_3D));
  
  switch(roi->type) {
  case ISOCONTOUR_2D:
    roi_ISOCONTOUR_2D_erase_area(roi, erase_vp, area_size);
    break;
  case ISOCONTOUR_3D:
  default:
    roi_ISOCONTOUR_3D_erase_area(roi, erase_vp, area_size);
    break;
  }

  return;
}





