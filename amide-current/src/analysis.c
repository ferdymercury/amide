/* analysis.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2003 Andy Loening
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
#include <glib.h>
#include <sys/stat.h>
#include "analysis.h"


/* make sure we have NAN defined to at least something */
#ifndef NAN
#define NAN 0.0
#endif

#define EMPTY 0.0

static analysis_frame_t * analysis_frame_unref(analysis_frame_t * frame_analysis);
static analysis_frame_t * analysis_frame_init(AmitkRoi * roi, AmitkDataSet *ds,
					      gdouble subfraction);
static analysis_volume_t * analysis_volume_unref(analysis_volume_t *volume_analysis);
static analysis_volume_t * analysis_volume_init(AmitkRoi * roi, GList * volumes,
						gdouble subfraction);


static analysis_frame_t * analysis_frame_unref(analysis_frame_t * frame_analysis) {

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

typedef struct element_t {
  amide_data_t value;
  amide_real_t weight;
} element_t;

static void record_stats(AmitkVoxel voxel,
			 amide_data_t value,
			 amide_real_t voxel_fraction,
			 gpointer data) {

  GPtrArray * array = data;
  element_t * element;
  

  /* crashes if alloc fails, but I don't want to do error checking in the inner loop... 
     let's not run out of memory */
  element = g_malloc(sizeof(element_t)); 

  element->value = value;
  element->weight = voxel_fraction;
  g_ptr_array_add(array, element);

  return;
}

static gint array_comparison(gconstpointer a, gconstpointer b) {

  const element_t * ea = *((element_t **) a);
  const element_t * eb = *((element_t **) b);

  if (ea->value > eb->value) 
    return -1;
  else if (ea->value < eb->value) 
    return 1;
  else
    return 0;
}

/* calculate an analysis of several statistical values for an roi on a given data set frame. */
static analysis_frame_t * analysis_frame_init_recurse(AmitkRoi * roi, 
						      AmitkDataSet * ds, 
						      gdouble subfraction,
						      guint frame) {

  analysis_frame_t * frame_analysis;
  guint num_frames;
  GPtrArray * data_array;
  guint total_voxels;
  guint subfraction_voxels;
  guint i;
  element_t * element;
  amide_data_t temp;

  num_frames = AMITK_DATA_SET_NUM_FRAMES(ds);
  if (frame == num_frames) return NULL; /* check if we're done */
  g_assert(frame < num_frames); /* sanity check */

  /* and now calculate this frame's data */
#ifdef AMIDE_DEBUG
  g_print("Calculating ROI: %s on Data Set: %s Frame %d\n", 
	  AMITK_OBJECT_NAME(roi), AMITK_OBJECT_NAME(ds), frame);
#endif

  if ((data_array = g_ptr_array_new()) == NULL) {
    g_warning("couldn't allocate space for data array for frame %d", frame);
    return NULL;
  }
  /* fill the array with the appropriate info from the data set */
  /* note, I really only need a partial sort to get the median value, but
     glib doesn't have one (only has a full qsort), so I'll just do that.
     Partial sort would be order(N), qsort is order(NlogN), so it's not that
     much worse.

     If I used a partial sort, I'd have to iterate over the subfraction to find the 
     max and min, and I'd have to do another partial sort to find the median 
  */
  amitk_roi_calculate_on_data_set(roi, ds, frame, FALSE, record_stats, data_array);
  g_ptr_array_sort(data_array, array_comparison);

  total_voxels = data_array->len;
  subfraction_voxels = ceil(subfraction*total_voxels);
  if ((subfraction_voxels == 0) && (total_voxels > 0))
    subfraction_voxels = 1; /* have at least one voxel if the roi is in the data set*/


  /* fill in our frame_analysis structure */
  if ((frame_analysis =  g_try_new(analysis_frame_t,1)) == NULL) {
    g_warning("couldn't allocate space for roi analysis of frame %d", frame);
    return frame_analysis;
  }
  frame_analysis->ref_count = 1;

  /* set values */
  frame_analysis->duration = amitk_data_set_get_frame_duration(ds, frame);
  frame_analysis->time_midpoint = amitk_data_set_get_midpt_time(ds, frame);
  frame_analysis->total = 0.0;
  frame_analysis->median = 0.0;
  frame_analysis->total = 0.0;
  frame_analysis->voxels = 0.0;
  frame_analysis->correction = 0.0;
  frame_analysis->var = 0.0;

  
  if (subfraction_voxels == 0) { /* roi not in data set */
    frame_analysis->max = 0.0;
    frame_analysis->min = 0.0;
    frame_analysis->median = 0.0;
    frame_analysis->mean = 0.0;

  } else { 

    /* max */
    element = g_ptr_array_index(data_array, 0);
    frame_analysis->max = element->value;

    /* min */
    element = g_ptr_array_index(data_array, subfraction_voxels-1);
    frame_analysis->min = element->value;

    /* median */
    if (subfraction_voxels & 0x1) { /* odd */
      element = g_ptr_array_index(data_array, (subfraction_voxels-1)/2);
      frame_analysis->median = element->value;
    } else { /* even */
      element = g_ptr_array_index(data_array, subfraction_voxels/2-1);
      frame_analysis->median = 0.5*element->value;
      element = g_ptr_array_index(data_array, subfraction_voxels/2);
      frame_analysis->median += 0.5*element->value;
    }

    /* total and #voxels */
    for (i=0; i<subfraction_voxels; i++) {
      element = g_ptr_array_index(data_array, i);
      frame_analysis->total += element->weight*element->value;
      frame_analysis->voxels += element->weight;
    }

    /* calculate the mean */
    frame_analysis->mean = frame_analysis->total/frame_analysis->voxels;

    /* calculate variance */
    for (i=0; i<subfraction_voxels; i++) {
      element = g_ptr_array_index(data_array, i);
      temp = (element->value-frame_analysis->mean);
      frame_analysis->correction += element->weight*temp;
      frame_analysis->var += element->weight*temp*temp;
    }
    /* and divide to get the final var, note I'm using N-1, as the mean
       in a sense is being "estimated" from the data set....  If anyone
       else with more statistical experience disagrees, please speak up */
    /* the "total correction" parameter is to correct roundoff error,
       for a discussion, see "the art of computer programming" */
    /* strictly, using frame_analysis->voxels < 2.0 is overly conservative,
       could use subfraction_voxels < 2, but if anyone really needs variance
       for such a small volume, they should probably rethink their study... */
    if (frame_analysis->voxels < 2.0)
      frame_analysis->var = NAN; /* variance is nonsensical */
    else
      frame_analysis->var = 
	(frame_analysis->var - frame_analysis->correction*frame_analysis->correction/frame_analysis->voxels)
	/(frame_analysis->voxels-1.0);
  }

  g_ptr_array_free(data_array, TRUE); /* TRUE frees elements too */

  /* now let's recurse  */
  frame_analysis->next_frame_analysis = analysis_frame_init_recurse(roi, ds, subfraction, frame+1);

  return frame_analysis;
}



/* returns a calculated analysis structure of an roi on a frame of a data set */
static analysis_frame_t * analysis_frame_init(AmitkRoi * roi, AmitkDataSet * ds,
					      gdouble subfraction) {

  /* sanity checks */
  if (AMITK_ROI_UNDRAWN(roi)) {
    g_warning("ROI: %s appears not to have been drawn", AMITK_OBJECT_NAME(roi));
    return NULL;
  }

  return analysis_frame_init_recurse(roi, ds, subfraction, 0);
}


/* free up an roi analysis over a data set */
static analysis_volume_t * analysis_volume_unref(analysis_volume_t * volume_analysis) {

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
static analysis_volume_t * analysis_volume_init(AmitkRoi * roi, GList * data_sets, 
						gdouble subfraction) {
  
  analysis_volume_t * temp_volume_analysis;

  
  if (data_sets == NULL)  return NULL;

  g_return_val_if_fail(AMITK_IS_DATA_SET(data_sets->data), NULL);

  if ((temp_volume_analysis =  g_try_new(analysis_volume_t,1)) == NULL) {
    g_warning("couldn't allocate space for roi analysis of volumes");
    return NULL;
  }

  temp_volume_analysis->ref_count = 1;
  temp_volume_analysis->data_set = g_object_ref(data_sets->data);

  /* calculate this one */
  temp_volume_analysis->frame_analyses = 
    analysis_frame_init(roi, temp_volume_analysis->data_set, subfraction);

  /* recurse */
  temp_volume_analysis->next_volume_analysis = 
    analysis_volume_init(roi, data_sets->next, subfraction);

  
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
analysis_roi_t * analysis_roi_init(AmitkStudy * study, GList * rois, 
				   GList * data_sets, gdouble subfraction) {
  
  analysis_roi_t * temp_roi_analysis;
  
  if (rois == NULL)  return NULL;

  if ((temp_roi_analysis =  g_try_new(analysis_roi_t,1)) == NULL) {
    g_warning("couldn't allocate space for roi analyses");
    return NULL;
  }

  temp_roi_analysis->ref_count = 1;
  temp_roi_analysis->roi = g_object_ref(rois->data);
  temp_roi_analysis->study = g_object_ref(study);
  temp_roi_analysis->subfraction = subfraction;

  /* calculate this one */
  temp_roi_analysis->volume_analyses = 
    analysis_volume_init(temp_roi_analysis->roi, data_sets, subfraction);

  /* recurse */
  temp_roi_analysis->next_roi_analysis = 
    analysis_roi_init(study, rois->next, data_sets, subfraction);

  
  return temp_roi_analysis;
}



