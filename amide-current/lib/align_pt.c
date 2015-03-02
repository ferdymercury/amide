/* align_pt.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
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

#include <sys/stat.h>
#include "config.h"
#include <string.h>
#include <glib.h>
#include "align_pt.h"


/* removes a reference to an alignmen tpoint, frees up the point if no more references */
align_pt_t * align_pt_unref(align_pt_t * align_pt) {

  if (align_pt == NULL) return align_pt;

  /* sanity checks */
  g_return_val_if_fail(align_pt->ref_count > 0, NULL);

  align_pt->ref_count--;

  if (align_pt->ref_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing point: %s\n",align_pt->name);
#endif
    g_free(align_pt->name);
    g_free(align_pt);
    align_pt = NULL;
  }

  return align_pt;
}
     

/* returns an initialized alignment point structure */
align_pt_t * align_pt_init(void) {

  align_pt_t * new_pt;

  if ((new_pt = (align_pt_t *) g_malloc(sizeof(align_pt_t))) == NULL) {
    g_warning("Couldn't allocate memory for the alignment point");
    return new_pt;
  }
  new_pt->ref_count = 1;

  /* put in some sensable values */
  new_pt->name = NULL;
  new_pt->point= zero_rp;

  return new_pt;
}

align_pt_t * align_pt_ref(align_pt_t * align_pt) {
  g_return_val_if_fail(align_pt != NULL, NULL);
  align_pt->ref_count++;
  return align_pt;
}


/* function to write out the information content of an alignment point
   into an xml file.  Returns a string containing the name of the file */
gchar * align_pt_write_xml(align_pt_t * pt, gchar * study_directory, 
			   gchar * volume_name) {

  gchar * pt_xml_filename;
  struct stat file_info;
  guint count;
  xmlDocPtr doc;

  /* make a guess as to our filename */
  count = 1;
  pt_xml_filename = g_strdup_printf("Alignment_pt_%s_on_%s.xml", 
				    pt->name, volume_name);
  /* see if this file already exists */
  while (stat(pt_xml_filename, &file_info) == 0) {
    g_free(pt_xml_filename);
    count++;
    pt_xml_filename = g_strdup_printf("Alignment_pt_%s_%d_on_%s.xml", 
					  pt->name, count, volume_name);
  }

  /* and we now have unique filenames */
#ifdef AMIDE_DEBUG
  g_print("\t- saving alignment point  %s in file %s\n",
	  pt->name, pt_xml_filename);
#endif

  /* write the xml file */
  doc = xmlNewDoc("1.0");
  doc->children = xmlNewDocNode(doc, NULL, "Alignment_point", pt->name);
  xml_save_realpoint(doc->children, "point", pt->point);

  /* and save */
  xmlSaveFile(pt_xml_filename, doc);

  /* and we're done */
  xmlFreeDoc(doc);

  return pt_xml_filename;
}

/* function to load in an alignment point xml file */
align_pt_t * align_pt_load_xml(gchar * pt_xml_filename, const gchar * study_directory) {

  xmlDocPtr doc;
  align_pt_t * new_pt;
  xmlNodePtr nodes;
  gchar * temp_string;

  new_pt = align_pt_init();

  /* parse the xml file */
  if ((doc = xmlParseFile(pt_xml_filename)) == NULL) {
    g_warning("Couldn't Parse AMIDE alignment point xml file %s/%s",
	      study_directory,pt_xml_filename);
    return align_pt_unref(new_pt);
  }

  /* get the root of our document */
  if ((nodes = xmlDocGetRootElement(doc)) == NULL) {
    g_warning("AMIDE alignment point xml file doesn't appear to have a root: %s/%s",
	      study_directory,pt_xml_filename);
    return align_pt_unref(new_pt);
  }

  /* get the name */
  temp_string = xml_get_string(nodes->children, "text");
  if (temp_string != NULL) {
    align_pt_set_name(new_pt, temp_string);
    g_free(temp_string);
  }

  /* get the document tree */
  nodes = nodes->children;

  /* get the point */
  new_pt->point = xml_get_realpoint(nodes, "point");
  /* and we're done */
  xmlFreeDoc(doc);
  
  return new_pt;
}


/* sets the name of an alignment point
   note: new_name is copied rather then just being referenced  */
void align_pt_set_name(align_pt_t * align_pt, gchar * new_name) {
  g_free(align_pt->name); 
  align_pt->name = g_strdup(new_name);
  return;
}

/* makes a new alignment point which is a copy of a previous points info */
align_pt_t * align_pt_copy(align_pt_t * src_align_pt) {

  align_pt_t * dest_align_pt;

  /* sanity checks */
  g_return_val_if_fail(src_align_pt != NULL, NULL);

  dest_align_pt = align_pt_init();

  /* copy the data elements */
  dest_align_pt->point = src_align_pt->point;

  /* make a separate copy in memory of the name */
  align_pt_set_name(dest_align_pt, src_align_pt->name);

  return dest_align_pt;
}


/* free up a list of alignment points */
align_pts_t * align_pts_unref(align_pts_t * align_pts) {
  
  if (align_pts == NULL) return align_pts;

  /* sanity check */
  g_return_val_if_fail(align_pts->ref_count > 0, NULL);

  /* remove a reference count */
  align_pts->ref_count--;

  /* things to do if our reference count is zero */
  if (align_pts->ref_count == 0) {
    /* recursively delete rest of list */
    align_pts->next = align_pts_unref(align_pts->next); 

    align_pts->align_pt = align_pt_unref(align_pts->align_pt);
    g_free(align_pts);
    align_pts = NULL;
  }

  return align_pts;
}

/* returns an initialized alignment point list node structure */
align_pts_t * align_pts_init(align_pt_t * align_pt) {
  
  align_pts_t * new_pts;
  
  if ((new_pts = (align_pts_t *) g_malloc(sizeof(align_pts_t))) == NULL) {
    g_warning("Couldn't allocate memory for the alignment points list");
    return new_pts;
  }
  new_pts->ref_count = 1;
  
  new_pts->align_pt = align_pt_ref(align_pt);
  new_pts->next = NULL;
  
  return new_pts;
}

/* adds one to the reference count of a alignment points list*/
align_pts_t * align_pts_add_ref(align_pts_t * align_pts) {
  g_return_val_if_fail(align_pts != NULL, NULL);
  align_pts->ref_count++;
  return align_pts;
}

/* function to write a list of alignment points as xml data.  Function calls
   align_pt_write_xml to writeout each point, and adds information about the
   alignment point file to the node list */
void align_pts_write_xml(align_pts_t * pts, xmlNodePtr node_list, 
			 gchar * study_directory, gchar * volume_name) {

  gchar * pt_xml_filename;

  if (pts != NULL) {
    pt_xml_filename = align_pt_write_xml(pts->align_pt, study_directory, volume_name);
    xmlNewChild(node_list, NULL, "Alignment_point_file", pt_xml_filename);
    g_free(pt_xml_filename);

    /* and recurse */
    align_pts_write_xml(pts->next, node_list, study_directory, volume_name);
  }
  
  return;
}


/* function to load in a list of alignment point xml nodes */
align_pts_t * align_pts_load_xml(xmlNodePtr node_list, const gchar * study_directory) {

  gchar * file_name;
  align_pts_t * new_pts;
  align_pt_t * new_pt;

  if (node_list != NULL) {
    /* first, recurse on through the list */
    new_pts = align_pts_load_xml(node_list->next, study_directory);

    /* load in this node */
    file_name = xml_get_string(node_list->children, "text");
    new_pt = align_pt_load_xml(file_name,study_directory);
    new_pts = align_pts_add_pt_first(new_pts, new_pt);
    new_pt = align_pt_unref(new_pt);
    g_free(file_name);

  } else
    new_pts = NULL;

  return new_pts;
}


/* function to return a pointer to an alignment point in a points list, returns NULL if not found */
align_pt_t * align_pts_find_pt(align_pts_t * align_pts, align_pt_t * find_pt) {
  
  if (align_pts == NULL) 
    return NULL;
  else if (align_pts->align_pt == find_pt)
    return align_pts->align_pt;
  else 
    return align_pts_find_pt(align_pts->next, find_pt);
} 

/* function to check that an alignment point is in a points list */
gboolean align_pts_includes_pt(align_pts_t * align_pts, align_pt_t * includes_pt) {
  if (align_pts_find_pt(align_pts, includes_pt) != NULL)
    return TRUE;
  else
    return FALSE;
}

/* finds an alignment point in a list by name, returns a pointer if found */
align_pt_t * align_pts_find_pt_by_name(align_pts_t * align_pts, gchar * name) {

  if (align_pts == NULL)
    return NULL;
  else if (strcmp(align_pts->align_pt->name, name) == 0)
    return align_pts->align_pt;
  else
    return align_pts_find_pt_by_name(align_pts->next, name);
}

/* function to check if the name of a point is in a points list or not */
gboolean align_pts_includes_pt_by_name(align_pts_t * align_pts, gchar * name) {
  if (align_pts_find_pt_by_name(align_pts, name) != NULL)
    return TRUE;
  else
    return FALSE;
}


/* function to add an alignment point to the end of an alignment list */
align_pts_t * align_pts_add_pt(align_pts_t * align_pts, align_pt_t * new_pt) {

  align_pts_t * temp_list = align_pts;
  align_pts_t * prev_list = NULL;

  g_return_val_if_fail(new_pt != NULL, align_pts);

  /* make sure the point isn't already in the list */
  if (align_pts_includes_pt(align_pts, new_pt)) return align_pts;

  /* get to the end of the list */
  while (temp_list != NULL) {
    prev_list = temp_list;
    temp_list = temp_list->next;
  }
  
  /* get a new align_pts data structure */
  temp_list = align_pts_init(new_pt);

  if (align_pts == NULL)
    return temp_list;
  else {
    prev_list->next = temp_list;
    return align_pts;
  }
}


/* function to add an alignment point onto a list of points as the first item */
align_pts_t * align_pts_add_pt_first(align_pts_t * align_pts, align_pt_t * new_pt) {

  align_pts_t * temp_list;

  g_return_val_if_fail(new_pt != NULL, align_pts);

  /* make sure the point isn't already in the list */
  if (align_pts_includes_pt(align_pts, new_pt)) return align_pts;

  temp_list = align_pts_add_pt(NULL,new_pt);
  temp_list->next = align_pts;

  return temp_list;
}


/* function to remove a alignment point from a list of points */
align_pts_t * align_pts_remove_pt(align_pts_t * align_pts, align_pt_t * pt) {

  align_pts_t * temp_list = align_pts;
  align_pts_t * prev_list = NULL;

  while (temp_list != NULL) {
    if (temp_list->align_pt == pt) {
      if (prev_list == NULL)
	align_pts = temp_list->next;
      else
	prev_list->next = temp_list->next;
      temp_list->next = NULL;
      temp_list = align_pts_unref(temp_list);
    } else {
      prev_list = temp_list;
      temp_list = temp_list->next;
    }
  }

  return align_pts;
}


/* count the number of points in a list of points */
guint align_pts_count(align_pts_t * list) {
  if (list == NULL) return 0;
  else return (1+align_pts_count(list->next));
}

/* function to count the number of pairs in two lists of poits */
guint align_pts_count_pairs_by_name(align_pts_t * pts1, align_pts_t * pts2) {
  if (pts1 == NULL)
    return 0;
  else if (align_pts_includes_pt_by_name(pts2, pts1->align_pt->name))
    return align_pts_count_pairs_by_name(pts1->next, pts2) + 1;
  else
    return align_pts_count_pairs_by_name(pts1->next, pts2);

}


