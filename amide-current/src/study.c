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
#include "amide.h"
#include "realspace.h"
#include "color_table.h"
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

  /* if we've removed all reference's, free the study */
  if (study->reference_count == 0) {
    study->volumes = volume_list_free(study->volumes); /*  free volumes */
    study->rois = roi_list_free(study->rois); /* free rois */
    g_free(study->name);
    g_free(study);
    study = NULL;
  }

  return study;
}



study_t * study_init(void) {
  
  study_t * study;

  /* alloc space for the data structure for passing ui info */
  if ((study = (study_t *) g_malloc(sizeof(study_t))) == NULL) {
    g_warning("%s: couldn't allocate space for study_t",PACKAGE);
    return NULL;
  }
  study->reference_count = 1;

  study->name = NULL;
  study->filename = NULL;
  study->volumes = NULL;
  study->rois = NULL;

  return study;
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

