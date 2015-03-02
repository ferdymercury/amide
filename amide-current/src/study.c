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

#include <time.h>
#include <sys/stat.h>
#include "config.h"
#include <glib.h>
#include <unistd.h>
#include "amide.h"
#include "study.h"


/* external variables */
gchar * scaling_names[] = {"per slice", "global"};
gchar * scaling_explanations[] = {
  "scale the images based on the max and min values in the current slice",
  "scale the images based on the max and min values of the entire data set",
};

/* functions */



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
    g_free(study->filename);
    g_free(study->creation_date);
    g_free(study->name);
    g_free(study);
    study = NULL;
  }

  return study;
}


/* initialize a study structure with semi-sensible values */
study_t * study_init(void) {
  
  study_t * study;
  axis_t i_axis;
  realpoint_t init;
  time_t current_time;

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

  /* view parameters */
  init.x = init.y = init.z = -1.0;
  study_set_view_center(study, init);
  study_set_view_thickness(study, -1.0);
  study_set_view_time(study, 0.0+CLOSE);
  study_set_view_duration(study, 1.0-CLOSE);
  study_set_zoom(study, 1.0);
  study_set_interpolation(study, NEAREST_NEIGHBOR);
  study_set_scaling(study, SCALING_GLOBAL);

  /* set the creation date as today */
  study->creation_date = NULL;
  time(&current_time);
  study_set_creation_date(study, ctime(&current_time));
  g_strdelimit(study->creation_date, "\n", ' '); /* turns newlines to white space */
  g_strstrip(study->creation_date); /* removes trailing and leading white space */
      
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
    g_warning("%s: Couldn't change directories in writing study",PACKAGE);
    return FALSE;
  }

#ifdef AMIDE_DEBUG
  g_print("Saving Study %s in %s\n",study_name(study), directory);
#endif

  /* start creating an xml document */
  doc = xmlNewDoc("1.0");

  /* place our study info into it */
  doc->children = xmlNewDocNode(doc, NULL, "Study", study_name(study));

  /* record our version */
  xml_save_string(doc->children, "AMIDE_Data_File_Version", AMIDE_FILE_VERSION);

  /* record the creation date */
  xml_save_string(doc->children, "creation_date", study_creation_date(study));

  /* put in our coord frame */
  xml_save_realspace(doc->children, "coord_frame", study_coord_frame(study));

  /* put in our volume info */
  volume_nodes = xmlNewChild(doc->children, NULL, "Volumes", NULL);
  volume_list_write_xml(study_volumes(study), volume_nodes, directory);

  /* put in our roi info */
  roi_nodes = xmlNewChild(doc->children, NULL, "ROIs", NULL);
  roi_list_write_xml(study_rois(study), roi_nodes, directory);

  /* record our viewing parameters */
  xml_save_realpoint(doc->children, "view_center", study_view_center(study));
  xml_save_floatpoint(doc->children, "view_thickness", study_view_thickness(study));
  xml_save_time(doc->children, "view_time", study_view_time(study));
  xml_save_time(doc->children, "view_duration", study_view_duration(study));
  xml_save_string(doc->children, "interpolation", interpolation_names[study_interpolation(study)]);  
  xml_save_floatpoint(doc->children, "zoom", study_zoom(study));
  xml_save_string(doc->children, "scaling", scaling_names[study_scaling(study)]);


  /* and save */
  xmlSaveFile(STUDY_FILE_NAME, doc);

  /* and we're done */
  xmlFreeDoc(doc);

  /* and return to the old directory */
  if (chdir(old_dir) != 0) {
    g_warning("%s: Couldn't return to previous directory in writing study",PACKAGE);
    return FALSE;
  }
  g_free(old_dir);

  /* remember the name of the directory of this study */
  study_set_filename(study, directory);

  return TRUE;
}

/* function to load in a study from disk in xml format */
study_t * study_load_xml(const gchar * directory) {

  xmlDocPtr doc;
  xmlNodePtr nodes;
  xmlNodePtr volume_nodes, roi_nodes;
  gchar * old_dir;
  gchar * temp_string;
  gchar * file_version;
  gchar * creation_date;
  study_t * new_study;
  volume_list_t * new_volumes;
  roi_list_t * new_rois;
  interpolation_t i_interpolation;
  scaling_t i_scaling;
  time_t modification_time;
  realspace_t study_coord_frame;
  struct stat file_info;
  
  new_study = study_init();

  /* switch into our new directory */
  old_dir = g_get_current_dir();
  if (chdir(directory) != 0) {
    g_warning("%s: Couldn't change directories in loading study",PACKAGE);
    study_free(new_study);
    return new_study;
  }

  /* parse the xml file */
  if ((doc = xmlParseFile(STUDY_FILE_NAME)) == NULL) {
    g_warning("%s: Couldn't Parse AMIDE xml file:\n\t%s/%s",
	      PACKAGE, directory,STUDY_FILE_NAME);
    /* and return to the old directory */
    if (chdir(old_dir) != 0) {
      g_warning("%s: Couldn't return to previous directory in load study",PACKAGE);
      g_free(old_dir);
    }
    study_free(new_study);
    return new_study;
  }

  /* get the root of our document */
  if ((nodes = xmlDocGetRootElement(doc)) == NULL) {
    g_warning("%s: AMIDE xml file doesn't appear to have a root:\n\t%s/%s",
	      PACKAGE, directory,STUDY_FILE_NAME);
    /* and return to the old directory */
    if (chdir(old_dir) != 0) {
      g_warning("%s: Couldn't return to previous directory in load study",PACKAGE);
      g_free(old_dir);
    }
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

  /* get the version of the data file */
  file_version = xml_get_string(nodes, "AMIDE_Data_File_Version");

  /* warn if this is an old file version */
  if (file_version == NULL)
    g_warning("%s: No file version for the data file.... is this an AMIDE data file?",PACKAGE);
  else if (g_strcasecmp(file_version, AMIDE_FILE_VERSION) != 0)
    g_warning("%s: xif data file versions don't match (expected %s but got %s).  Will try anyway",
	      PACKAGE, AMIDE_FILE_VERSION, file_version);

  /* get the creation date of the study */
  creation_date = xml_get_string(nodes, "creation_date");

  /* put in the last time the study file was modified,
     if no creation date was specified */
  if (creation_date == NULL) {
    stat(STUDY_FILE_NAME, &file_info);
    modification_time = file_info.st_mtime;
    creation_date = g_strdup(ctime(&modification_time));
    g_strdelimit(creation_date, "\n", ' '); /* turns newlines to white space */
    g_strstrip(creation_date); /* removes trailing and leading white space */
    g_warning("%s: no creation date found in study, educated guess is %s", 
	      PACKAGE, creation_date);
  }
  study_set_creation_date(new_study, creation_date);
  g_free(creation_date);


  /* get our study parameters */
  study_coord_frame = xml_get_realspace(nodes, "coord_frame");
  study_set_coord_frame(new_study, study_coord_frame);


  /* load in the volumes */
  volume_nodes = xml_get_node(nodes, "Volumes");
  volume_nodes = volume_nodes->children;
  new_volumes = volume_list_load_xml(volume_nodes, directory);
  study_add_volumes(new_study, new_volumes);
  volume_list_free(new_volumes);
  
  /* load in the rois */
  roi_nodes = xml_get_node(nodes, "ROIs");
  roi_nodes = roi_nodes->children;
  new_rois = roi_list_load_xml(roi_nodes, directory);
  study_add_rois(new_study, new_rois);
  roi_list_free(new_rois);

  /* get our view parameters */
  study_set_view_center(new_study, xml_get_realpoint(nodes, "view_center"));
  study_set_view_thickness(new_study, xml_get_floatpoint(nodes, "view_thickness"));
  study_set_view_time(new_study, xml_get_time(nodes, "view_time"));
  study_set_view_duration(new_study, xml_get_time(nodes, "view_duration"));
  study_set_zoom(new_study, xml_get_floatpoint(nodes, "zoom"));
 
  /* sanity check */
  if (study_zoom(new_study) < SMALL) {
    g_warning("%s: inappropriate zoom (%5.3f) for study, reseting to 1.0",PACKAGE, study_zoom(new_study));
    study_set_zoom(new_study, 1.0);
  }

  /* figure out the interpolation */
  temp_string = xml_get_string(nodes, "interpolation");
  if (temp_string != NULL)
    for (i_interpolation=0; i_interpolation < NUM_INTERPOLATIONS; i_interpolation++) 
      if (g_strcasecmp(temp_string, interpolation_names[i_interpolation]) == 0)
	study_set_interpolation(new_study, i_interpolation);
  g_free(temp_string);

  /* figure out the scaling */
  temp_string = xml_get_string(nodes, "scaling");
  if (temp_string != NULL)
    for (i_scaling=0; i_scaling < NUM_SCALINGS; i_scaling++) 
      if (g_strcasecmp(temp_string, scaling_names[i_scaling]) == 0)
	study_set_scaling(new_study, i_scaling);
  g_free(temp_string);

  /* and we're done */
  xmlFreeDoc(doc);
    
  /* legacy cruft, rip out at some point in the future */
  /* compensate for errors in old versions of amide */
  if (g_strcasecmp(file_version, "1.3") < 0) {
    volume_list_t * volumes;
    volume_t * volume;
    roi_list_t * rois;
    roi_t * roi;
    realpoint_t view_center;

    g_warning("%s: detected file version previous to 1.3, compensating for coordinate errors",
	      PACKAGE);

    view_center = study_view_center(new_study);
    view_center.y = -view_center.y;
    study_set_view_center(new_study,view_center);

    volumes = study_volumes(new_study);
    while (volumes != NULL) {
      volume = volumes->volume;
      volume->coord_frame.axis[XAXIS].y = -volume->coord_frame.axis[XAXIS].y;
      volume->coord_frame.axis[YAXIS].y = -volume->coord_frame.axis[YAXIS].y;
      volume->coord_frame.axis[ZAXIS].y = -volume->coord_frame.axis[ZAXIS].y;
      volume->coord_frame.offset.y = -volume->coord_frame.offset.y;
      volumes = volumes->next;
    }

    rois = study_rois(new_study);
    while (rois != NULL) {
      roi = rois->roi;
      roi->coord_frame.axis[XAXIS].y = -roi->coord_frame.axis[XAXIS].y;
      roi->coord_frame.axis[YAXIS].y = -roi->coord_frame.axis[YAXIS].y;
      roi->coord_frame.axis[ZAXIS].y = -roi->coord_frame.axis[ZAXIS].y;
      roi->coord_frame.offset.y = -roi->coord_frame.offset.y;
      rois = rois->next;
    }
  }

  /* freeing up anything we haven't freed yet */
  g_free(file_version);

  /* and return to the old directory */
  if (chdir(old_dir) != 0) {
    g_warning("%s: Couldn't return to previous directory in load study",PACKAGE);
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
  study_set_coord_frame(dest_study, study_coord_frame(src_study));

  /* copy the view information */
  study_set_view_center(dest_study, study_view_center(src_study));
  study_set_view_thickness(dest_study, study_view_thickness(src_study));
  study_set_view_time(dest_study, study_view_time(src_study));
  study_set_view_duration(dest_study, study_view_duration(src_study));
  study_set_zoom(dest_study, study_zoom(src_study));
  study_set_interpolation(dest_study, study_interpolation(src_study));
  study_set_scaling(dest_study, study_scaling(src_study));

  /* make a copy of the study's ROIs and volumes */
  if (study_rois(src_study) != NULL)
    dest_study->rois = roi_list_copy(study_rois(src_study));
  if (study_volumes(src_study) != NULL)
    dest_study->volumes = volume_list_copy(study_volumes(src_study));

  /* make a separate copy in memory of the study's name and filename */
  study_set_name(dest_study, study_name(src_study));
  study_set_filename(dest_study, study_filename(src_study));
  study_set_creation_date(dest_study, study_creation_date(src_study));

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

/* add a list of volumes to a study */
void study_add_volumes(study_t * study, volume_list_t * volumes) {

  while (volumes != NULL) {
    study_add_volume(study, volumes->volume);
    volumes = volumes->next;
  }

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


/* add a list of rois to a study */
void study_add_rois(study_t * study, roi_list_t * rois) {

  while (rois != NULL) {
    study_add_roi(study, rois->roi);
    rois = rois->next;
  }

  return;
}

/* sets the name of a study
   note: new_name is copied rather then just being referenced by study */
void study_set_name(study_t * study, const gchar * new_name) {

  g_free(study->name); /* free up the memory used by the old name */
  study->name = g_strdup(new_name); /* and assign the new name */

  return;
}

/* sets the filename of a study
   note: new_filename is copied rather then just being referenced by study */
void study_set_filename(study_t * study, const gchar * new_filename) {

  g_free(study->filename); /* free up the memory used by the old filename */
  study->filename = g_strdup(new_filename); /* and assign the new name */

  return;
}

/* sets the date of a study
   note: new_date is copied rather then just being referenced by study,
   note: no error checking of the date is currently done. */
void study_set_creation_date(study_t * study, const gchar * new_date) {

  g_free(study->creation_date); /* free up the memory used by the old creation date */
  study->creation_date = g_strdup(new_date); /* and assign the new name */

  return;
}

