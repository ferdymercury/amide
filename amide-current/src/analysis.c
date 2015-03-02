/* analysis.c
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
#include "analysis.h"


analysis_frame_t * analysis_frame_free(analysis_frame_t * frame_analysis) {

  if (frame_analysis == NULL)
    return frame_analysis;

  /* sanity checks */
  g_return_val_if_fail(frame_analysis->reference_count > 0, NULL);

  /* remove a reference count */
  frame_analysis->reference_count--;

  /* if we've removed all reference's, free the roi */
  if (frame_analysis->reference_count == 0) {
    /* recursively delete rest of list */
    frame_analysis->next_frame_analysis = analysis_frame_free(frame_analysis->next_frame_analysis);

    g_free(frame_analysis);
    frame_analysis = NULL;
  } 

  return frame_analysis;
}


/* calculate an analysis of several statistical values for an roi on a given volume frame. */
static analysis_frame_t * analysis_frame_init_recurse(roi_t * roi, 
						      volume_t * volume, 
						      guint frame) {

  analysis_frame_t * frame_analysis = NULL;

  if (frame == volume->data_set->dim.t) return NULL; /* check if we're done */
  g_assert(frame < volume->data_set->dim.t); /* sanity check */

  /* and now calculate this frame's data */
#ifdef AMIDE_DEBUG
  g_print("Calculating ROI: %s on Volume: %s Frame %d\n", roi->name, volume->name, frame);
#endif

  switch (roi->type) {
  case ELLIPSOID:
    frame_analysis = analysis_frame_ELLIPSOID_init(roi, volume, frame);
    break;
  case CYLINDER:
    frame_analysis = analysis_frame_CYLINDER_init(roi, volume, frame);
    break;
  case BOX:
    frame_analysis = analysis_frame_BOX_init(roi, volume, frame);
    break;
  case ISOCONTOUR_2D:
    frame_analysis = analysis_frame_ISOCONTOUR_2D_init(roi, volume, frame);
    break;
  case ISOCONTOUR_3D:
    frame_analysis = analysis_frame_ISOCONTOUR_3D_init(roi, volume, frame);
    break;
  default:
    g_warning("%s: roi type %d not implemented!",PACKAGE, roi->type);
    return frame_analysis;
    break;
  }

  /* now let's recurse  */
  frame_analysis->next_frame_analysis = analysis_frame_init_recurse(roi, volume, frame+1);

  return frame_analysis;
}



/* returns a calculated analysis structure of an roi on a frame of a volume */
analysis_frame_t * analysis_frame_init(roi_t * roi, volume_t * volume) {

  /* sanity checks */
  if (roi_undrawn(roi)) {
    g_warning("%s: ROI: %s appears not to have been drawn",PACKAGE, roi->name);
    return NULL;
  }

  return analysis_frame_init_recurse(roi, volume, 0);
}


/* free up an roi analysis over a volume */
analysis_volume_t * analysis_volume_free(analysis_volume_t * volume_analysis) {

  if (volume_analysis == NULL) return volume_analysis;

  /* sanity check */
  g_return_val_if_fail(volume_analysis->reference_count > 0, NULL);

  /* remove a reference count */
  volume_analysis->reference_count--;

  /* stuff to do if reference count is zero */
  if (volume_analysis->reference_count == 0) {
    /* recursively delete rest of list */
    volume_analysis->next_volume_analysis = analysis_volume_free(volume_analysis->next_volume_analysis);

    volume_analysis->frame_analyses = analysis_frame_free(volume_analysis->frame_analyses);
    volume_analysis->volume = volume_free(volume_analysis->volume);
    g_free(volume_analysis);
    volume_analysis = NULL;
  }

  return volume_analysis;
}

/* returns an initialized roi analysis of a list of volumes */
analysis_volume_t * analysis_volume_init(roi_t * roi, volume_list_t * volumes) {
  
  analysis_volume_t * temp_volume_analysis;
  
  if (volumes == NULL)  return NULL;

  if ((temp_volume_analysis =  (analysis_volume_t *) g_malloc(sizeof(analysis_volume_t))) == NULL) {
    g_warning("%s: couldn't allocate space for roi analysis of volumes", PACKAGE);
    return NULL;
  }

  temp_volume_analysis->reference_count = 1;
  temp_volume_analysis->volume = volume_add_reference(volumes->volume);

  /* calculate this one */
  temp_volume_analysis->frame_analyses = analysis_frame_init(roi, temp_volume_analysis->volume);

  /* recurse */
  temp_volume_analysis->next_volume_analysis = analysis_volume_init(roi, volumes->next);

  
  return temp_volume_analysis;
}



/* free up a list of roi analyses */
analysis_roi_t * analysis_roi_free(analysis_roi_t * roi_analysis) {

  if (roi_analysis == NULL) return roi_analysis;

  /* sanity check */
  g_return_val_if_fail(roi_analysis->reference_count > 0, NULL);

  /* remove a reference count */
  roi_analysis->reference_count--;

  /* stuff to do if reference count is zero */
  if (roi_analysis->reference_count == 0) {
    /* recursively free/dereference rest of list */
    roi_analysis->next_roi_analysis = analysis_roi_free(roi_analysis->next_roi_analysis);

    roi_analysis->volume_analyses = analysis_volume_free(roi_analysis->volume_analyses);
    roi_analysis->roi = roi_free(roi_analysis->roi);
    roi_analysis->study = study_free(roi_analysis->study);
    g_free(roi_analysis);
    roi_analysis = NULL;
  }

  return roi_analysis;
}

/* returns an initialized list of roi analyses */
analysis_roi_t * analysis_roi_init(study_t * study, roi_list_t * rois, volume_list_t * volumes) {
  
  analysis_roi_t * temp_roi_analysis;
  
  if (rois == NULL)  return NULL;

  if ((temp_roi_analysis =  (analysis_roi_t *) g_malloc(sizeof(analysis_roi_t))) == NULL) {
    g_warning("%s: couldn't allocate space for roi analyses", PACKAGE);
    return NULL;
  }

  temp_roi_analysis->reference_count = 1;
  temp_roi_analysis->roi = roi_add_reference(rois->roi);
  temp_roi_analysis->study = study_add_reference(study);

  /* calculate this one */
  temp_roi_analysis->volume_analyses = analysis_volume_init(temp_roi_analysis->roi, volumes);

  /* recurse */
  temp_roi_analysis->next_roi_analysis = analysis_roi_init(study, rois->next, volumes);

  
  return temp_roi_analysis;
}



