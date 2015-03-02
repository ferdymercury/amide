/* study.c
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
#include <unistd.h>
#include "amide.h"
#include "volume.h"
#include "roi.h"
#include "study.h"


study_t * study_free(study_t * study) {

  if (study == NULL)
    return study;
  
  /* sanity checks */
  g_return_val_if_fail(study->reference_count > 0, NULL);

  /* remove a reference count */
  study->reference_count--;

  /* things we always do */
  study->volumes = volume_list_free(study->volumes); /*  free volumes */
  study->rois = roi_list_free(study->rois); /* free rois */

  /* if we've removed all reference's, free the study */
  if (study->reference_count == 0) {
    g_free(study->name);
    g_free(study);
    study = NULL;
  }

  return study;
}



study_t * study_init(void) {
  
  study_t * study;
  axis_t i_axis;

  /* alloc space for the data structure for passing ui info */
  if ((study = (study_t *) g_malloc(sizeof(study_t))) == NULL) {
    g_warning("%s: couldn't allocate space for study_t",PACKAGE);
    return NULL;
  }
  study->reference_count = 1;

  study->name = NULL;
  study->filename = NULL;
  study->coord_frame.offset=realpoint_init;
  for (i_axis=0;i_axis<NUM_AXIS;i_axis++)
    study->coord_frame.axis[i_axis] = default_axis[i_axis];
  study->volumes = NULL;
  study->rois = NULL;

  return study;
}


/* function to writeout the study to disk in xml format */
gboolean study_write_xml(study_t * study, gchar * directory) {

  xmlDocPtr doc;
  xmlNodePtr volume_nodes, roi_nodes;
  gchar * old_dir;

  /* switch into our new directory */
  old_dir = g_get_current_dir();
  if (chdir(directory) != 0) {
    g_warning("%s: Couldn't change directories in writing study\n",PACKAGE);
    return FALSE;
  }

  /* start creating an xml document */
  doc = xmlNewDoc("1.0");

  /* place our study info into it */
  doc->children = xmlNewDocNode(doc, NULL, "Study", study->name);

  /* record our version */
  xml_save_string(doc->children, "AMIDE_Data_File_Version", "1.0");

  /* put in our coord frame */
  xml_save_realspace(doc->children, "coord_frame", study->coord_frame);

  /* put in our volume info */
  volume_nodes = xmlNewChild(doc->children, NULL, "Volumes", NULL);
  volume_list_write_xml(study->volumes, volume_nodes, directory);

  /* put in our roi info */
  roi_nodes = xmlNewChild(doc->children, NULL, "ROIs", NULL);
  roi_list_write_xml(study->rois, roi_nodes, directory);

  /* and save */
  xmlSaveFile(STUDY_FILE_NAME, doc);

  /* and we're done */
  xmlFreeDoc(doc);

  /* and return to the old directory */
  if (chdir(old_dir) != 0) {
    g_warning("%s: Couldn't return to previous directory in writing study\n",PACKAGE);
    return FALSE;
  }
  g_free(old_dir);

  return TRUE;
}

/* function to load in a study from disk in xml format */
study_t * study_load_xml(gchar * directory) {

  xmlDocPtr doc;
  xmlNodePtr nodes;
  xmlNodePtr volume_nodes, roi_nodes;
  gchar * old_dir;
  gchar * temp_string;
  study_t * new_study;
  
  new_study = study_init();

  /* switch into our new directory */
  old_dir = g_get_current_dir();
  if (chdir(directory) != 0) {
    g_warning("%s: Couldn't change directories in loading study\n",PACKAGE);
    study_free(new_study);
    return new_study;
  }

  /* parse the xml file */
  if ((doc = xmlParseFile(STUDY_FILE_NAME)) == NULL) {
    g_warning("%s: Couldn't Parse AMIDE xml file %s/%s\n",PACKAGE, directory,STUDY_FILE_NAME);
    study_free(new_study);
    return new_study;
  }

  /* get the root of our document */
  if ((nodes = xmlDocGetRootElement(doc)) == NULL) {
    g_warning("%s: AMIDE xml file doesn't appear to have a root: %s/%s\n",
	      PACKAGE, directory,STUDY_FILE_NAME);
    study_free(new_study);
    return new_study;
  }

  /* get the study name */
  temp_string = xml_get_string(nodes->children, "text");
  if (temp_string != NULL) {
    study_set_name(new_study,temp_string);
    g_free(temp_string);
  }

  /* get the document tree */
  nodes = nodes->children;

  /* get our study parameters */
  new_study->coord_frame = xml_get_realspace(nodes, "coord_frame");

  /* load in the volumes */
  volume_nodes = xml_get_node(nodes, "Volumes");
  volume_nodes = volume_nodes->children;
  new_study->volumes = volume_list_load_xml(volume_nodes, directory);
  
  /* and load in the rois */
  roi_nodes = xml_get_node(nodes, "ROIs");
  roi_nodes = roi_nodes->children;
  new_study->rois = roi_list_load_xml(roi_nodes, directory);
    
  /* and we're done */
  xmlFreeDoc(doc);
    
  /* and return to the old directory */
  if (chdir(old_dir) != 0) {
    g_warning("%s: Couldn't return to previous directory in load study\n",PACKAGE);
    g_free(old_dir);
    study_free(new_study);
    return new_study;
  }
  g_free(old_dir);

  /* and remember the name of the directory for convience */
  study_set_filename(new_study, directory);

  return new_study;
}


/* makes a new study item which is a copy of a previous study's information. */
study_t * study_copy(study_t * src_study) {

  study_t * dest_study;

  /* sanity checks */
  g_return_val_if_fail(src_study != NULL, NULL);

  dest_study = study_init();

  /* copy the data elements */
  dest_study->coord_frame = src_study->coord_frame;

  /* make a copy of the study's ROIs and volumes */
  if (src_study->rois != NULL)
    dest_study->rois = roi_list_copy(src_study->rois);
  if (src_study->volumes != NULL)
    dest_study->volumes = volume_list_copy(src_study->volumes);

  /* make a separate copy in memory of the study's name and filename */
  study_set_name(dest_study, src_study->name);
  study_set_filename(dest_study, src_study->filename);

  return dest_study;
}


/* adds one to the reference count of a study */
study_t * study_add_reference(study_t * study) {

  study->reference_count++;

  return study;
}

/* add a volume to a study */
void study_add_volume(study_t * study, volume_t * volume) {

  study->volumes = volume_list_add_volume(study->volumes, volume);

  return;
}

/* remove a volume from a study */
void study_remove_volume(study_t * study, volume_t * volume) {

  study->volumes = volume_list_remove_volume(study->volumes, volume);

  return;
}

/* add an roi to a study */
void study_add_roi(study_t * study, roi_t * roi) {

  study->rois = roi_list_add_roi(study->rois, roi);

  return;
}


/* remove an roi from a study */
void study_remove_roi(study_t * study, roi_t * roi) {

  study->rois = roi_list_remove_roi(study->rois, roi);

  return;
}

/* sets the name of a study
   note: new_name is copied rather then just being referenced by study */
void study_set_name(study_t * study, gchar * new_name) {

  g_free(study->name); /* free up the memory used by the old name */
  study->name = g_strdup(new_name); /* and assign the new name */

  return;
}

/* sets the filename of a study
   note: new_filename is copied rather then just being referenced by study */
void study_set_filename(study_t * study, gchar * new_filename) {

  g_free(study->filename); /* free up the memory used by the old filename */
  study->filename = g_strdup(new_filename); /* and assign the new name */

  return;
}

