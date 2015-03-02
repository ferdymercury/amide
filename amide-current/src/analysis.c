/* analysis.c
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

#include "amide_config.h"
#include <glib.h>
#include <sys/stat.h>
#include "analysis.h"


analysis_frame_t * analysis_frame_unref(analysis_frame_t * frame_analysis) {

  analysis_frame_t * return_list;

  if (frame_analysis == NULL)
    return frame_analysis;

  /* sanity checks */
  g_return_val_if_fail(frame_analysis->ref_count > 0, NULL);

  /* remove a reference count */
  frame_analysis->ref_count--;

  /* if we've removed all reference's, free the roi */
  if (frame_analysis->ref_count == 0) {
    /* recursively delete rest of list */
    return_list = analysis_frame_unref(frame_analysis->next_frame_analysis);
    frame_analysis->next_frame_analysis = NULL;
    g_free(frame_analysis);
    frame_analysis = NULL;
  } else
    return_list = frame_analysis;

  return frame_analysis;
}


/* calculate an analysis of several statistical values for an roi on a given data set frame. */
static analysis_frame_t * analysis_frame_init_recurse(AmitkRoi * roi, 
						      AmitkDataSet * ds,
						      guint frame) {

  analysis_frame_t * frame_analysis = NULL;

  if (frame == AMITK_DATA_SET_NUM_FRAMES(ds)) return NULL; /* check if we're done */
  g_assert(frame < AMITK_DATA_SET_NUM_FRAMES(ds)); /* sanity check */

  /* and now calculate this frame's data */
#ifdef AMIDE_DEBUG
  g_print("Calculating ROI: %s on Data Set: %s Frame %d\n", 
	  AMITK_OBJECT_NAME(roi), AMITK_OBJECT_NAME(ds), frame);
#endif

  switch (AMITK_ROI_TYPE(roi)) {
  case AMITK_ROI_TYPE_ELLIPSOID:
    frame_analysis = analysis_frame_ELLIPSOID_init(roi, ds, frame);
    break;
  case AMITK_ROI_TYPE_CYLINDER:
    frame_analysis = analysis_frame_CYLINDER_init(roi, ds, frame);
    break;
  case AMITK_ROI_TYPE_BOX:
    frame_analysis = analysis_frame_BOX_init(roi, ds, frame);
    break;
  case AMITK_ROI_TYPE_ISOCONTOUR_2D:
    frame_analysis = analysis_frame_ISOCONTOUR_2D_init(roi, ds, frame);
    break;
  case AMITK_ROI_TYPE_ISOCONTOUR_3D:
    frame_analysis = analysis_frame_ISOCONTOUR_3D_init(roi, ds, frame);
    break;
  default:
    g_error("roi type %d not implemented!", roi->type);
    return frame_analysis;
    break;
  }

  /* now let's recurse  */
  frame_analysis->next_frame_analysis = analysis_frame_init_recurse(roi, ds, frame+1);

  return frame_analysis;
}



/* returns a calculated analysis structure of an roi on a frame of a data set */
analysis_frame_t * analysis_frame_init(AmitkRoi * roi, AmitkDataSet * ds) {

  /* sanity checks */
  if (amitk_roi_undrawn(roi)) {
    g_warning("ROI: %s appears not to have been drawn", AMITK_OBJECT_NAME(roi));
    return NULL;
  }

  return analysis_frame_init_recurse(roi, ds, 0);
}


/* free up an roi analysis over a data set */
analysis_volume_t * analysis_volume_unref(analysis_volume_t * volume_analysis) {

  analysis_volume_t * return_list;

  if (volume_analysis == NULL) return volume_analysis;

  /* sanity check */
  g_return_val_if_fail(volume_analysis->ref_count > 0, NULL);

  /* remove a reference count */
  volume_analysis->ref_count--;

  /* stuff to do if reference count is zero */
  if (volume_analysis->ref_count == 0) {
    /* recursively delete rest of list */
    return_list = analysis_volume_unref(volume_analysis->next_volume_analysis);
    volume_analysis->next_volume_analysis = NULL;

    volume_analysis->frame_analyses = analysis_frame_unref(volume_analysis->frame_analyses);
    if (volume_analysis->data_set != NULL) {
      g_object_unref(volume_analysis->data_set);
      volume_analysis->data_set=NULL;
    }
    g_free(volume_analysis);
    volume_analysis = NULL;
  } else
    return_list = volume_analysis;

  return volume_analysis;
}

/* returns an initialized roi analysis of a list of volumes */
analysis_volume_t * analysis_volume_init(AmitkRoi * roi, GList * data_sets) {
  
  analysis_volume_t * temp_volume_analysis;

  
  if (data_sets == NULL)  return NULL;

  g_return_val_if_fail(AMITK_IS_DATA_SET(data_sets->data), NULL);

  if ((temp_volume_analysis =  g_new(analysis_volume_t,1)) == NULL) {
    g_warning("couldn't allocate space for roi analysis of volumes");
    return NULL;
  }

  temp_volume_analysis->ref_count = 1;
  temp_volume_analysis->data_set = g_object_ref(data_sets->data);

  /* calculate this one */
  temp_volume_analysis->frame_analyses = analysis_frame_init(roi, temp_volume_analysis->data_set);

  /* recurse */
  temp_volume_analysis->next_volume_analysis = analysis_volume_init(roi, data_sets->next);

  
  return temp_volume_analysis;
}



/* free up a list of roi analyses */
analysis_roi_t * analysis_roi_unref(analysis_roi_t * roi_analysis) {

  analysis_roi_t * return_list;

  if (roi_analysis == NULL) return roi_analysis;

  /* sanity check */
  g_return_val_if_fail(roi_analysis->ref_count > 0, NULL);

  /* remove a reference count */
  roi_analysis->ref_count--;

  /* stuff to do if reference count is zero */
  if (roi_analysis->ref_count == 0) {
    /* recursively free/dereference rest of list */
    return_list = analysis_roi_unref(roi_analysis->next_roi_analysis);
    roi_analysis->next_roi_analysis = NULL;

    roi_analysis->volume_analyses = analysis_volume_unref(roi_analysis->volume_analyses);
    if (roi_analysis->roi != NULL) {
      g_object_unref(roi_analysis->roi);
      roi_analysis->roi = NULL;
    }
    if (roi_analysis->study != NULL) {
      g_object_unref(roi_analysis->study);
      roi_analysis->study = NULL;
    }
    g_free(roi_analysis);
    roi_analysis = NULL;
  } else
    return_list = roi_analysis;

  return return_list;
}

/* returns an initialized list of roi analyses */
analysis_roi_t * analysis_roi_init(AmitkStudy * study, GList * rois, GList * data_sets) {
  
  analysis_roi_t * temp_roi_analysis;
  
  if (rois == NULL)  return NULL;

  if ((temp_roi_analysis =  g_new(analysis_roi_t,1)) == NULL) {
    g_warning("couldn't allocate space for roi analyses");
    return NULL;
  }

  temp_roi_analysis->ref_count = 1;
  temp_roi_analysis->roi = g_object_ref(rois->data);
  temp_roi_analysis->study = g_object_ref(study);

  /* calculate this one */
  temp_roi_analysis->volume_analyses = analysis_volume_init(temp_roi_analysis->roi, data_sets);

  /* recurse */
  temp_roi_analysis->next_roi_analysis = analysis_roi_init(study, rois->next, data_sets);

  
  return temp_roi_analysis;
}



