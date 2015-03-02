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
#include "amide.h"
#include "volume.h"
#include "roi.h"

/* external variables */
gchar * roi_type_names[] = {"Ellipsoid", \
			    "Elliptic Cylinder", \
			    "Box"};

gchar * roi_grain_names[] = {"1", \
			     "8", \
			     "27", \
			     "64"};

/* free up an roi */
roi_t * roi_free(roi_t * roi) {
  
  if (roi == NULL)
    return roi;

  /* sanity checks */
  g_return_val_if_fail(roi->reference_count > 0, NULL);

  /* remove a reference count */
  roi->reference_count--;

  /* things we always do */
  roi->children = roi_list_free(roi->children);
  
  /* if we've removed all reference's, free the roi */
  if (roi->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing roi: %s\n",roi->name);
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
  axis_t i;
  
  if ((temp_roi = 
       (roi_t *) g_malloc(sizeof(roi_t))) == NULL) {
    return NULL;
  }
  temp_roi->reference_count = 1;
  
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

/* function to write out the information content of an roi into an xml
   file.  Returns a string containing the name of the file. */
gchar * roi_write_xml(roi_t * roi, gchar * directory) {

  gchar * file_name;
  guint count;
  struct stat file_info;
  xmlDocPtr doc;
  xmlNodePtr roi_nodes;

  /* make a guess as to our filename */
  count = 1;
  file_name = g_strdup_printf("ROI_%s.xml", roi->name);

  /* see if this file already exists */
  while (stat(file_name, &file_info) == 0) {
    g_free(file_name);
    count++;
    file_name = g_strdup_printf("ROI_%s_%d.xml", roi->name, count);
  }

  /* and we now have a unique filename */
#ifdef AMIDE_DEBUG
  g_print("\t- saving roi %s in roi %s\n",roi->name, file_name);
#endif

  /* write the roi xml file */
  doc = xmlNewDoc("1.0");
  doc->children = xmlNewDocNode(doc, NULL, "ROI", roi->name);
  xml_save_string(doc->children, "type", roi_type_names[roi->type]);
  xml_save_string(doc->children, "grain", roi_grain_names[roi->grain]);
  xml_save_realspace(doc->children, "coord_frame", roi->coord_frame);
  xml_save_realpoint(doc->children, "corner", roi->corner);

  /* save our children */
  roi_nodes = xmlNewChild(doc->children, NULL, "Children", NULL);
  roi_list_write_xml(roi->children, roi_nodes, directory);

  /* and save */
  xmlSaveFile(file_name, doc);

  /* and we're done */
  xmlFreeDoc(doc);

  return file_name;
}

/* function to load in an ROI xml file */
roi_t * roi_load_xml(gchar * file_name, gchar * directory) {

  xmlDocPtr doc;
  roi_t * new_roi;
  xmlNodePtr nodes;
  xmlNodePtr children_nodes;
  roi_type_t i_roi_type;
  roi_grain_t i_roi_grain;
  gchar * temp_string;

  new_roi = roi_init();

  /* parse the xml file */
  if ((doc = xmlParseFile(file_name)) == NULL) {
    g_warning("%s: Couldn't Parse AMIDE ROI xml file %s/%s",PACKAGE, directory,file_name);
    roi_free(new_roi);
    return new_roi;
  }

  /* get the root of our document */
  if ((nodes = xmlDocGetRootElement(doc)) == NULL) {
    g_warning("%s: AMIDE ROI xml file doesn't appear to have a root: %s/%s",
	      PACKAGE, directory,file_name);
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

  /* figure out the grain */
  temp_string = xml_get_string(nodes, "grain");
  if (temp_string != NULL)
    for (i_roi_grain=0; i_roi_grain < NUM_GRAIN_TYPES; i_roi_grain++) 
      if (g_strcasecmp(temp_string, roi_grain_names[i_roi_grain]) == 0)
	new_roi->grain = i_roi_grain;
  g_free(temp_string);

  /* and figure out the rest of the parameters */
  new_roi->coord_frame = xml_get_realspace(nodes, "coord_frame");
  new_roi->corner = xml_get_realpoint(nodes, "corner");

  /* and get any children */
  temp_string = xml_get_string(nodes->children, "Children");
  if (temp_string != NULL) {
    children_nodes = xml_get_node(nodes->children, "Children");
    new_roi->children = roi_list_load_xml(children_nodes, directory);
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
  dest_roi->grain = src_roi->grain;

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
  corner[0] = realspace_base_coord_to_alt(roi->coord_frame.offset, roi->coord_frame);
  corner[1] = roi->corner;
 
  /* the center in roi coords is then just half the far corner */
  REALPOINT_MADD(0.5,corner[1], 0.5,corner[0], center);
  
  /* now, translate that into real coords */
  center = realspace_alt_coord_to_base(center, roi->coord_frame);

  return center;
}


/* free up an roi list */
roi_list_t * roi_list_free(roi_list_t * roi_list) {

  if (roi_list == NULL)
    return roi_list;

  /* sanity check */
  g_return_val_if_fail(roi_list->reference_count > 0, NULL);

  /* remove a reference count */
  roi_list->reference_count--;

  /* things we always do */
  roi_list->roi = roi_free(roi_list->roi);

  /* recursively delete rest of list */
  roi_list->next = roi_list_free(roi_list->next);

  /* stuff to do if reference count is zero */
  if (roi_list->reference_count == 0) {
    g_free(roi_list);
    roi_list = NULL;
  }

  return roi_list;
}

/* returns an initialized roi list node structure */
roi_list_t * roi_list_init(void) {
  
  roi_list_t * temp_roi_list;
  
  if ((temp_roi_list = 
       (roi_list_t *) g_malloc(sizeof(roi_list_t))) == NULL) {
    return NULL;
  }
  temp_roi_list->reference_count = 1;

  temp_roi_list->next = NULL;
  temp_roi_list->roi = NULL;
  
  return temp_roi_list;
}


/* function to write a list of rois as xml data.  Function calls
   roi_write_xml to writeout each roi, and adds information about the
   roi file to the node_list. */
void roi_list_write_xml(roi_list_t *list, xmlNodePtr node_list, gchar * directory) {

  gchar * file_name;

  if (list != NULL) { 
    file_name = roi_write_xml(list->roi, directory);
    xmlNewChild(node_list, NULL, "ROI_file", file_name);
    g_free(file_name);
    
    /* and recurse */
    roi_list_write_xml(list->next, node_list, directory);
  }

  return;
}

/* function to load in a list of ROI xml nodes */
roi_list_t * roi_list_load_xml(xmlNodePtr node_list, gchar * directory) {

  gchar * file_name;
  roi_list_t * new_roi_list;
  roi_t * new_roi;

  if (node_list != NULL) {
    /* first, recurse on through the list */
    new_roi_list = roi_list_load_xml(node_list->next, directory);

    /* load in this node */
    file_name = xml_get_string(node_list->children, "text");
    new_roi = roi_load_xml(file_name,directory);
    new_roi_list = roi_list_add_roi_first(new_roi_list, new_roi);
    new_roi = roi_free(new_roi);
    g_free(file_name);

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
  temp_list = roi_list_init();

  /* add the roi to this new list item */
  temp_list->roi = roi_add_reference(roi);

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

  /* sanity check */
  g_return_val_if_fail(src_roi_list != NULL, NULL);

  dest_roi_list = roi_list_init();

  /* make a separate copy in memory of the roi this list item points to */
  dest_roi_list->roi = roi_copy(src_roi_list->roi);
    
  /* and make copies of the rest of the elements in this list */
  if (src_roi_list->next != NULL)
    dest_roi_list->next = roi_list_copy(src_roi_list->next);

  return dest_roi_list;
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
gboolean roi_undrawn(const roi_t * roi) {
  
  return 
    REALPOINT_EQUAL(roi->coord_frame.offset,realpoint_init) &&
    REALPOINT_EQUAL(roi->corner,realpoint_init);
}
    


/* returns a singly linked list of intersection points between the roi
   and the given slice.  returned points are in the slice's coord_frame */
GSList * roi_get_volume_intersection_points(const volume_t * view_slice,
					    const roi_t * roi) {


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
    REALPOINT_MADD(0.5,roi_corner[1], 0.5,roi_corner[0], center);   

    /* figure out the radius in each direction */
    REALPOINT_DIFF(roi_corner[1],roi_corner[0], radius);
    REALPOINT_CMULT(0.5,radius, radius);

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
	  voxel_in = realpoint_in_box(temp_p, roi_corner[0],roi_corner[1]);
	  break;
	case CYLINDER:
	  voxel_in = realpoint_in_elliptic_cylinder(temp_p, center, 
						    height, radius);
	  break;
	case ELLIPSOID: 
	  voxel_in = realpoint_in_ellipsoid(temp_p,center,radius);
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
void roi_subset_of_volume(roi_t * roi,
			  const volume_t * volume,
			  voxelpoint_t * subset_start,
			  voxelpoint_t * subset_dim){

  realpoint_t roi_corner[2];
  realpoint_t subset_corner[2];
  voxelpoint_t subset_index[2];

  g_assert(volume != NULL);
  g_assert(roi != NULL);

  /* set some values */
  roi_corner[0] = realspace_base_coord_to_alt(roi->coord_frame.offset,
					      roi->coord_frame);
  roi_corner[1] = roi->corner;

  /* look at all eight corners of the roi cube, figure out the max and min dim */
  realspace_get_enclosing_corners(roi->coord_frame, roi_corner,
				  volume->coord_frame, subset_corner);
				  

  /* and convert the subset_corners into indexes */
  subset_index[0] = volume_realpoint_to_voxel(volume,subset_corner[0]);
  subset_index[1] = volume_realpoint_to_voxel(volume,subset_corner[1]);

  /* sanity checks */
  if (subset_index[0].x < 0) subset_index[0].x = 0;
  if (subset_index[0].x > volume->dim.x) subset_index[0].x = volume->dim.x;
  if (subset_index[0].y < 0) subset_index[0].y = 0;
  if (subset_index[0].y > volume->dim.y) subset_index[0].y = volume->dim.y;
  if (subset_index[0].z < 0) subset_index[0].z = 0;
  if (subset_index[0].z > volume->dim.z) subset_index[0].z = volume->dim.z;
  if (subset_index[1].x < 0) subset_index[1].x = 0;
  if (subset_index[1].x > volume->dim.x) subset_index[1].x = volume->dim.x;
  if (subset_index[1].y < 0) subset_index[1].y = 0;
  if (subset_index[1].y > volume->dim.y) subset_index[1].y = volume->dim.y;
  if (subset_index[1].z < 0) subset_index[1].z = 0;
  if (subset_index[1].z > volume->dim.z) subset_index[1].z = volume->dim.z;

  /* and calculate the return values */
  *subset_start = subset_index[0];
  REALPOINT_SUB(subset_index[1],subset_index[0],*subset_dim);

  return;
}


/* calculate an analysis of several statistical values for an roi on
   a given volume.
   note: the "grain" input indicates how many subvoxels we'll divide
   each voxel up into when calculating our statistics.  Setting the grain
   to something more than 1 [GRAIN_1], such as 8 [GRAIN_8] or 64 [GRAIN_64]
   may be useful for getting accurate statistics on small roi's */
roi_analysis_t roi_calculate_analysis(roi_t * roi, 
				      const volume_t * volume,
				      roi_grain_t grain,
				      guint frame) {
  
  roi_analysis_t analysis;
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
  REALPOINT_MADD(0.5,roi_corner[1], 0.5,roi_corner[0], center);
  
  /* figure out the radius in each direction */
  REALPOINT_DIFF(roi_corner[1],roi_corner[0], radius);
  REALPOINT_CMULT(0.5,radius, radius);
  
  /* figure out the height */
  height = fabs(roi_corner[1].z-roi_corner[0].z);
  
  /* initialize the analysis data structure based on the center of the roi */
  volume_p =  realspace_alt_coord_to_alt(center, roi->coord_frame, volume->coord_frame);
  i = volume_realpoint_to_voxel(volume,volume_p);
  if (!volume_includes_voxel(volume,i)) temp_data = EMPTY;
  else temp_data = VOLUME_CONTENTS(volume,frame,i);

  /* initialize values */
  analysis.voxels = 0.0; 
  analysis.mean = 0;
  analysis.min = analysis.max = temp_data;
  analysis.var = 0;

  /* sanity checks */
  if (roi_undrawn(roi)) {
    g_warning("%s: roi appears not to have been drawn",PACKAGE);
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
		voxel_in = realpoint_in_box(roi_p, roi_corner[0],roi_corner[1]);
		break;
	      case CYLINDER:
		voxel_in = realpoint_in_elliptic_cylinder(roi_p, center, 
							  height, radius);
		break;
	      case ELLIPSOID: 
		voxel_in = realpoint_in_ellipsoid(roi_p,center,radius);
		break;
	      default:
		g_printerr("roi type %d not fully implemented!\n",roi->type);
		return analysis;
		break;
	      };
	      
	      if (voxel_in) {
		if (!volume_includes_voxel(volume,i)) temp_data = EMPTY;
		else temp_data = VOLUME_CONTENTS(volume,frame,i);
	  
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
  if (analysis.voxels > CLOSE) /* make sure we don't div by 0 */
    analysis.mean /= analysis.voxels;
  else
    analysis.mean /= CLOSE;
  
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
		voxel_in = realpoint_in_box(roi_p, roi_corner[0],roi_corner[1]);
		break;
	      case CYLINDER:
		voxel_in = realpoint_in_elliptic_cylinder(roi_p, center, 
							  height, radius);
		break;
	      case ELLIPSOID: 
		voxel_in = realpoint_in_ellipsoid(roi_p,center,radius);
		break;
	      default:
		g_printerr("roi type %d not fully implemented!\n",roi->type);
		return analysis;
		break;
	      };
	      
	      if (voxel_in) {
		if (!volume_includes_voxel(volume,i)) temp_data = EMPTY;
		else temp_data = VOLUME_CONTENTS(volume,frame,i);
		
		analysis.var += 
		  voxel_grain_size*
		  (temp_data-analysis.mean)*(temp_data-analysis.mean);
	      }
	    }
	  }
	}
      }
    }
  }

  /* and divide to get the final var, note I'm using N and not (N-1), cause
     my mean is calculated from the whole data set and not just a sample of it.
     If anyone else with more statistical experience disagrees, please speak
     up */
  if (analysis.voxels > CLOSE) /* make sure we don't div by 0 */
    analysis.var /= analysis.voxels;
  else
    analysis.var /= CLOSE;

  /* and calculate the time midpoint of the data */
  analysis.time_midpoint = 
    (volume_end_time(volume, frame) + volume_start_time(volume, frame))/2.0;

  return analysis;
}





