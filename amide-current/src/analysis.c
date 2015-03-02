/* analysis.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2014 Andy Loening
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
#include "analysis.h"
#include <glib.h>
#include <sys/stat.h>

#include <sys/time.h>
#include <time.h>
#ifdef AMIDE_DEBUG
#include <sys/timeb.h>
#endif



/* make sure we have NAN defined to at least something */
//0.8.12 - hopefully all systems have bits/nan.h
//remove in a couple versions
//#ifndef NAN
//#define NAN 0.0
//#endif

#define EMPTY 0.0

static analysis_gate_t * analysis_gate_unref(analysis_gate_t *gate_analysis);
static analysis_gate_t * analysis_gate_init(AmitkRoi * roi, AmitkDataSet *ds,guint frame, 
					    analysis_calculation_t calculation_type,
					    gboolean accurate,
					    gdouble subfraction, 
					    gdouble threshold_percentage, 
					    gdouble threshold_value);
static analysis_frame_t * analysis_frame_unref(analysis_frame_t * frame_analysis);
static analysis_frame_t * analysis_frame_init(AmitkRoi * roi, AmitkDataSet *ds, 
					      analysis_calculation_t calculation_type,
					      gboolean accurate,
					      gdouble subfraction, 
					      gdouble threshold_percentage,
					      gdouble threshold_value);
static analysis_volume_t * analysis_volume_unref(analysis_volume_t *volume_analysis);
static analysis_volume_t * analysis_volume_init(AmitkRoi * roi, GList * volumes, 
						analysis_calculation_t calculation_type,
						gboolean accurate,
						gdouble subfraction, 
						gdouble threshold_percentage, 
						gdouble threshold_value);


void free_array_element(gpointer data, gpointer user_data) {
  analysis_element_t * element = data;
  g_free(element);
}

static analysis_gate_t * analysis_gate_unref(analysis_gate_t * gate_analysis) {

  analysis_gate_t * return_list;

  if (gate_analysis == NULL)
    return gate_analysis;

  /* sanity checks */
  g_return_val_if_fail(gate_analysis->ref_count > 0, NULL);

  /* remove a reference count */
  gate_analysis->ref_count--;

  /* if we've removed all reference's, free the roi */
  if (gate_analysis->ref_count == 0) {

    g_ptr_array_foreach(gate_analysis->data_array, free_array_element, NULL); /* free the elements */
    g_ptr_array_free(gate_analysis->data_array, TRUE); /* TRUE frees the array of pointers to elements as well */

    /* recursively delete rest of list */
    return_list = analysis_gate_unref(gate_analysis->next_gate_analysis);
    gate_analysis->next_gate_analysis = NULL;
    g_free(gate_analysis);
    gate_analysis = NULL;
  } else
    return_list = gate_analysis;

  return return_list;
}


static void record_stats(AmitkVoxel ds_voxel,
			 amide_data_t value,
			 amide_real_t voxel_fraction,
			 gpointer data) {

  GPtrArray * array = data;
  analysis_element_t * element;
  
  /* crashes if alloc fails, but I don't want to do error checking in the inner loop... 
     let's not run out of memory */
  if (voxel_fraction > 0.0) {
    element = g_malloc(sizeof(analysis_element_t)); 
    
    element->value = value;
    element->weight = voxel_fraction;
    element->ds_voxel = ds_voxel;
    g_ptr_array_add(array, element);
  }

  return;
}

static gint array_comparison(gconstpointer a, gconstpointer b) {

  const analysis_element_t * ea = *((analysis_element_t **) a);
  const analysis_element_t * eb = *((analysis_element_t **) b);

  if (ea->value > eb->value) 
    return -1;
  else if (ea->value < eb->value) 
    return 1;
  else
    return 0;
}



/* note, the following function for weight variance calculation is
   derived from statistics/wvariance_source.c from gsl version 1.3.  
   copyright Jim Davies, Brian Gough, released under GPL. */
/* The variance is divided by N-1, since the mean in a sense is being
   "estimated" from the data set....  If anyone else with more
   statistical experience disagrees, please speak up */
static gdouble wvariance (GPtrArray * array, guint num_elements, gdouble wmean)
{
  analysis_element_t * element;
  gdouble wsumofsquares = 0 ;
  gdouble Wa = 0;
  gdouble Wb = 0;
  gdouble wi;
  gdouble delta;
  gdouble factor;
  guint i;

  if (num_elements < 2) return NAN;

  /* find the weight sum of the squares */
  /* computes sum(wi*(valuei-mean))/sum(wi) */
  /* and computes the weighted version of N/(N-1) */
  for (i = 0; i < num_elements; i++) {
    element = g_ptr_array_index(array, i);
    wi = element->weight;

    if (wi > 0) {
      delta = element->value-wmean;
      Wa += wi ;
      Wb += wi*wi;
      wsumofsquares += (delta * delta - wsumofsquares) * (wi / Wa);
    } 
  }

  factor = (Wa*Wa)/((Wa*Wa)-Wb);

  return factor*wsumofsquares;
}



/* calculate an analysis of several statistical values for an roi on a given data set frame/gate. */
static analysis_gate_t * analysis_gate_init_recurse(AmitkRoi * roi, 
						    AmitkDataSet * ds, 
						    guint frame,
						    guint gate,
						    analysis_calculation_t calculation_type,
						    gboolean accurate,
						    gdouble subfraction,
						    gdouble threshold_percentage,
						    gdouble threshold_value) {

  GPtrArray * data_array;
  analysis_gate_t * analysis;
  guint subfraction_voxels;
  guint i;
  analysis_element_t * element;
  gdouble max;
  gboolean done;
#ifdef AMIDE_DEBUG
  struct timeval tv1;
  struct timeval tv2;
  gdouble time1;
  gdouble time2;

  /* let's do some timing */
  gettimeofday(&tv1, NULL);
#endif

  if (gate == AMITK_DATA_SET_NUM_GATES(ds)) return NULL; /* check if we're done */

  /* and now calculate this gate's data */
  if ((data_array = g_ptr_array_new()) == NULL) {
    g_warning(_("couldn't allocate memory space for data array for frame %d/gate %d"), frame, gate);
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
  amitk_roi_calculate_on_data_set(roi, ds, frame, gate,FALSE, accurate, record_stats, data_array);
  g_ptr_array_sort(data_array, array_comparison);
  
  switch(calculation_type) {
  case ALL_VOXELS:
    subfraction_voxels = data_array->len;
    break;
  case HIGHEST_FRACTION_VOXELS:
    subfraction_voxels = ceil(subfraction*data_array->len);

    if ((subfraction_voxels == 0) && (data_array->len > 0))
      subfraction_voxels = 1; /* have at least one voxel if the roi is in the data set*/

    break;
  case VOXELS_NEAR_MAX:
    subfraction_voxels = 0;

    if (data_array->len > 0) {
      element = g_ptr_array_index(data_array, 0);
      max = element->value;
      done = FALSE;
      for (i=0; i<data_array->len && !done; i++) {
	element = g_ptr_array_index(data_array, i);
	if (element->value >= max*threshold_percentage/100.0)
	  subfraction_voxels++;
	else
	  done = TRUE;
      }
    }

    if ((subfraction_voxels == 0) && (data_array->len > 0))
      subfraction_voxels = 1; /* have at least one voxel if the roi is in the data set*/

    break;
  case VOXELS_GREATER_THAN_VALUE:
    subfraction_voxels = 0;

    if (data_array->len > 0) {
      element = g_ptr_array_index(data_array, 0);
      max = element->value;
      done = FALSE;
      for (i=0; i<data_array->len && !done; i++) {
	element = g_ptr_array_index(data_array, i);
	if (element->value >= threshold_value)
	  subfraction_voxels++;
	else
	  done = TRUE;
      }
    }
    break;
  default:
    subfraction_voxels=0;
    g_error("unexpected case in %s at line %d",__FILE__, __LINE__);
  }


  /* fill in our gate_analysis structure */
  if ((analysis =  g_try_new(analysis_gate_t,1)) == NULL) {
    g_warning(_("couldn't allocate memory space for roi analysis of frame %d/gate %d"), frame, gate);
    return analysis;
  }
  analysis->ref_count = 1;

  /* set values */
  analysis->data_array = data_array;
  analysis->duration = amitk_data_set_get_frame_duration(ds, frame);
  analysis->time_midpoint = amitk_data_set_get_midpt_time(ds, frame);
  analysis->gate_time = amitk_data_set_get_gate_time(ds, gate);
  analysis->total = 0.0;
  analysis->median = 0.0;
  analysis->total = 0.0;
  analysis->voxels = subfraction_voxels;
  analysis->fractional_voxels = 0.0;
  analysis->correction = 0.0;
  analysis->var = 0.0;

  
  if (subfraction_voxels == 0) { /* roi not in data set */
    analysis->max = 0.0;
    analysis->min = 0.0;
    analysis->median = 0.0;
    analysis->mean = 0.0;

  } else { 

    /* max */
    element = g_ptr_array_index(data_array, 0);
    analysis->max = element->value;

    /* min */
    element = g_ptr_array_index(data_array, subfraction_voxels-1);
    analysis->min = element->value;

    /* median */
    if (subfraction_voxels & 0x1) { /* odd */
      element = g_ptr_array_index(data_array, (subfraction_voxels-1)/2);
      analysis->median = element->value;
    } else { /* even */
      element = g_ptr_array_index(data_array, subfraction_voxels/2-1);
      analysis->median = 0.5*element->value;
      element = g_ptr_array_index(data_array, subfraction_voxels/2);
      analysis->median += 0.5*element->value;
    }

    /* total and #fractional_voxels */
    for (i=0; i<subfraction_voxels; i++) {
      element = g_ptr_array_index(data_array, i);
      analysis->total += element->weight*element->value;
      analysis->fractional_voxels += element->weight;
    }

    /* calculate the mean */
    analysis->mean = analysis->total/analysis->fractional_voxels;

    /* calculate variance */
    analysis->var = wvariance(data_array, subfraction_voxels, analysis->mean);
  }
  
#ifdef AMIDE_DEBUG
  /* and wrapup our timing */
  gettimeofday(&tv2, NULL);
  time1 = ((double) tv1.tv_sec) + ((double) tv1.tv_usec)/1000000.0;
  time2 = ((double) tv2.tv_sec) + ((double) tv2.tv_usec)/1000000.0;

  g_print("Calculated ROI: %s on Data Set: %s Frame %d Gate %d.  Took %5.3f (s) \n", 
	  AMITK_OBJECT_NAME(roi), AMITK_OBJECT_NAME(ds), frame, gate, time2-time1);
#endif


  /* now let's recurse  */
  analysis->next_gate_analysis = 
    analysis_gate_init_recurse(roi, ds, frame, gate+1, calculation_type, accurate, 
			       subfraction, threshold_percentage, threshold_value);

  return analysis;
}



static analysis_gate_t * analysis_gate_init(AmitkRoi * roi, AmitkDataSet * ds,
					    guint frame, 
					    analysis_calculation_t calculation_type,
					    gboolean accurate,
					    gdouble subfraction,
					    gdouble threshold_percentage,
					    gdouble threshold_value) {

  return analysis_gate_init_recurse(roi, ds, frame, 0, calculation_type, accurate,
				    subfraction, threshold_percentage, threshold_value);
}


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

    frame_analysis->gate_analyses = analysis_gate_unref(frame_analysis->gate_analyses);
    g_free(frame_analysis);
    frame_analysis = NULL;
  } else
    return_list = frame_analysis;

  return return_list;
}


/* returns a calculated analysis structure of an roi on a frame of a data set */
static analysis_frame_t * analysis_frame_init_recurse(AmitkRoi * roi, 
						      AmitkDataSet *ds, 
						      guint frame,
						      analysis_calculation_t calculation_type,
						      gboolean accurate,
						      gdouble subfraction,
						      gdouble threshold_percentage,
						      gdouble threshold_value) {
  
  analysis_frame_t * temp_frame_analysis;
  
  if (frame == AMITK_DATA_SET_NUM_FRAMES(ds)) return NULL; /* check if we're done */

  if ((temp_frame_analysis =  g_try_new(analysis_frame_t,1)) == NULL) {
    g_warning(_("couldn't allocate memory space for roi analysis of frames"));
    return NULL;
  }
  
  temp_frame_analysis->ref_count = 1;

  /* calculate this one */
  temp_frame_analysis->gate_analyses = 
    analysis_gate_init(roi, ds, frame, calculation_type, accurate, subfraction, 
		       threshold_percentage, threshold_value);

  /* recurse */
  temp_frame_analysis->next_frame_analysis = 
    analysis_frame_init_recurse(roi, ds, frame+1, calculation_type, accurate, subfraction, 
				threshold_percentage, threshold_value);

  return temp_frame_analysis;
}


static analysis_frame_t * analysis_frame_init(AmitkRoi * roi, AmitkDataSet *ds, 
					      analysis_calculation_t calculation_type,
					      gboolean accurate,
					      gdouble subfraction,
					      gdouble threshold_percentage,
					      gdouble threshold_value) {

  /* sanity checks */
  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), NULL);

  if (AMITK_ROI_UNDRAWN(roi)) {
    g_warning(_("ROI: %s appears not to have been drawn"), AMITK_OBJECT_NAME(roi));
    return NULL;
  }

  return analysis_frame_init_recurse(roi, ds, 0, calculation_type, accurate, subfraction, 
				     threshold_percentage, threshold_value);
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
    if (volume_analysis->data_set != NULL)
      volume_analysis->data_set=amitk_object_unref(volume_analysis->data_set);
    g_free(volume_analysis);
    volume_analysis = NULL;
  } else
    return_list = volume_analysis;

  return return_list;
}

/* returns an initialized roi analysis of a list of volumes */
static analysis_volume_t * analysis_volume_init(AmitkRoi * roi, GList * data_sets, 
						analysis_calculation_t calculation_type,
						gboolean accurate,
						gdouble subfraction,
						gdouble threshold_percentage,
						gdouble threshold_value) {
  
  analysis_volume_t * temp_volume_analysis;

  
  if (data_sets == NULL)  return NULL;

  g_return_val_if_fail(AMITK_IS_DATA_SET(data_sets->data), NULL);

  if ((temp_volume_analysis =  g_try_new(analysis_volume_t,1)) == NULL) {
    g_warning(_("couldn't allocate memory space for roi analysis of volumes"));
    return NULL;
  }

  temp_volume_analysis->ref_count = 1;
  temp_volume_analysis->data_set = amitk_object_ref(data_sets->data);

  /* calculate this one */
  temp_volume_analysis->frame_analyses = 
    analysis_frame_init(roi, temp_volume_analysis->data_set, calculation_type, accurate,
			subfraction, threshold_percentage, threshold_value);

  /* recurse */
  temp_volume_analysis->next_volume_analysis = 
    analysis_volume_init(roi, data_sets->next, calculation_type, accurate,
			 subfraction, threshold_percentage, threshold_value);

  
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
#ifdef AMIDE_DEBUG
    g_print("freeing roi_analysis\n");
#endif
    /* recursively free/dereference rest of list */
    return_list = analysis_roi_unref(roi_analysis->next_roi_analysis);
    roi_analysis->next_roi_analysis = NULL;

    roi_analysis->volume_analyses = analysis_volume_unref(roi_analysis->volume_analyses);
    if (roi_analysis->roi != NULL) 
      roi_analysis->roi = amitk_object_unref(roi_analysis->roi);
    if (roi_analysis->study != NULL) 
      roi_analysis->study = amitk_object_unref(roi_analysis->study);
    g_free(roi_analysis);
    roi_analysis = NULL;
  } else
    return_list = roi_analysis;

  return return_list;
}

/* returns an initialized list of roi analyses */
analysis_roi_t * analysis_roi_init(AmitkStudy * study, GList * rois, 
				   GList * data_sets, 
				   analysis_calculation_t calculation_type,
				   gboolean accurate,
				   gdouble subfraction, 
				   gdouble threshold_percentage,
				   gdouble threshold_value) {
  
  analysis_roi_t * temp_roi_analysis;
  
  if (rois == NULL)  return NULL;

  if ((temp_roi_analysis =  g_try_new(analysis_roi_t,1)) == NULL) {
    g_warning(_("couldn't allocate memory space for roi analyses"));
    return NULL;
  }

  temp_roi_analysis->ref_count = 1;
  temp_roi_analysis->roi = amitk_object_ref(rois->data);
  temp_roi_analysis->study = amitk_object_ref(study);
  temp_roi_analysis->calculation_type = calculation_type;
  temp_roi_analysis->accurate = accurate;
  temp_roi_analysis->subfraction = subfraction;
  temp_roi_analysis->threshold_percentage = threshold_percentage;
  temp_roi_analysis->threshold_value = threshold_value;

  /* calculate this one */
  temp_roi_analysis->volume_analyses = 
    analysis_volume_init(temp_roi_analysis->roi, data_sets, calculation_type, accurate,
			 subfraction, threshold_percentage, threshold_value);

  /* recurse */
  temp_roi_analysis->next_roi_analysis = 
    analysis_roi_init(study, rois->next, data_sets, calculation_type, accurate,
		      subfraction, threshold_percentage, threshold_value);

  
  return temp_roi_analysis;
}



