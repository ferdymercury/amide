/* alignment_mutual_information.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2011-2012 Ian Miller
 *
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
#include "amitk_data_set.h"
#include "amitk_data_set_DOUBLE_0D_SCALING.h"
#include "alignment_mutual_information.h"

/* this algorithm will calculate the amount of mutual information between two data sets in their current orientations    */
/* it is a re-write of the original algorithm for purposes of improved speed. the hope is that it won't affect accuracy. */
/* rather than computing mutual information for the whole volume of data, the algorithm computes it for three orthogonal */
/* slices: axial, sagittal, and coronal */
#define NUM_BINS 50
gdouble calculate_mutual_information(AmitkDataSet * fixed_ds, AmitkDataSet * moving_ds, AmitkSpace * new_space, gint granularity,
				     gboolean * pfixed_slices_current, AmitkDataSet ** fixed_slice,
				     AmitkPoint view_center, amide_real_t thickness,
				     amide_time_t view_start_time, amide_time_t view_duration) {

  /* variables related to datasets, their volumes, coordinate spaces, etc */
  amide_data_t value_fixed, value_moving;
  AmitkCanvasPoint pixel_size;
  
  /* variables related to binning and mutual information determination*/
  gdouble bin_width_moving;             // mutual information will be calculated bin-wise
  gdouble bin_width_fixed;              // these values determine the size of the bins
  gint current_bin_number_moving;                       // the number of bins for the first dataset
  gint current_bin_number_fixed;                        // the number of bins for the second dataset
  gint mutual_information_array[NUM_BINS][NUM_BINS] =  {{ 0 }} ;
  gint margin_moving[NUM_BINS] = { 0 };
  gint margin_fixed[NUM_BINS] = { 0 };
  gint margin_total = 0;
  gint mi_nan_count;
  gdouble voxel_probability;            // the probability contribution of a single voxel
  gdouble incremental_mi;
  gdouble mutual_information = 0.0;     // this is the return value; the amount of MI computed in the two data sets; default to zero

  AmitkSpace * temp_space;
  GList * fixed_dss;
  AmitkVolume * view_volume;
  AmitkDataSet * moving_slice;
  AmitkPoint temp_offset;

  /* loop counters */
  AmitkView i_view;
  AmitkVoxel i_voxel;
  gint i, j;                             // temporary counters to iterate through the bins for the two datasets

  amide_data_t moving_global_min;
  amide_data_t fixed_global_min;
  
     
  /* use the range of values present in the data and the number of bins desired in order to determine how wide the bins should be */
  moving_global_min = amitk_data_set_get_global_min(moving_ds);
  fixed_global_min = amitk_data_set_get_global_min(fixed_ds);
  bin_width_moving = (amitk_data_set_get_global_max(moving_ds) - moving_global_min) / NUM_BINS;
  bin_width_fixed  = (amitk_data_set_get_global_max(fixed_ds)  - fixed_global_min)  / NUM_BINS;
    
  /* iterate through the dataset, and build up a frequency matrix for binned values */
  /* granularity determines whether we look at all the voxels, or just a subset. */
  /* we look at every nth voxel, where n is granularity */
  pixel_size.x = pixel_size.y = granularity * point_min_dim(AMITK_DATA_SET_VOXEL_SIZE(fixed_ds));

  /* iterate over the orthogonal directions */
  for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view++) {

    /* create a volume for the slice we're going to work on */
    view_volume = amitk_volume_new();
    temp_space = amitk_space_get_view_space(i_view, AMITK_LAYOUT_LINEAR);

    /* figure out the dimensions of the slice volume */
    fixed_dss = g_list_append(NULL, amitk_object_ref(fixed_ds));
    /* note, we don't bother with FOV, as that's actually computed based on all visible data sets, and we only
       have two of the data sets here. We'll just use 100% FOV */
    amitk_volumes_calc_display_volume(fixed_dss, temp_space, view_center, thickness, 100.0, view_volume);
    amitk_objects_unref(fixed_dss);
    g_object_unref(temp_space);

    /* (re)calculate the fixed slice if we need to */
    if ((fixed_slice[i_view] == NULL) || (*pfixed_slices_current == FALSE)) {
#ifdef AMIDE_DEBUG
      g_print("recompute fixed slice for view %d with pixel size %f\n", i_view, pixel_size.x);
#endif

      if (fixed_slice[i_view] != NULL)
	amitk_object_unref(AMITK_OBJECT(fixed_slice[i_view]));

      /* compute the fixed slices of data */
      fixed_slice[i_view] = amitk_data_set_get_slice(fixed_ds, view_start_time, view_duration, -1, pixel_size, 
						     view_volume);
    }

    /* update the view volume by a transform to take into account the rotations/translations we're doing */
    /* first take care of the axis rotation */
    temp_offset = AMITK_SPACE_OFFSET(view_volume);
    temp_space = amitk_space_calculate_transform(new_space, AMITK_SPACE(moving_ds));
    amitk_space_transform(AMITK_SPACE(view_volume), temp_space);
    g_object_unref(temp_space);

    /* second, take care of adjusting the view volume's offset. */
    temp_offset = amitk_space_b2s(AMITK_SPACE(new_space), temp_offset);
    temp_offset = amitk_space_s2b(AMITK_SPACE(moving_ds), temp_offset);
    amitk_space_set_offset(AMITK_SPACE(view_volume), temp_offset);

    /* now calculate the slice from the moving data set, and calculate the corresponding MI */
    moving_slice = amitk_data_set_get_slice(moving_ds, view_start_time, view_duration, -1, pixel_size, 
					    view_volume);

    /* calculate the mutual information */
    i_voxel = zero_voxel;
    for (i_voxel.y = 0; i_voxel.y < AMITK_DATA_SET_DIM_Y(fixed_slice[i_view]); i_voxel.y++) {
      for (i_voxel.x = 0; i_voxel.x < AMITK_DATA_SET_DIM_X(fixed_slice[i_view]); i_voxel.x++) {

	/* DOUBLE_0D is the type of the slice data sets */
	value_fixed = AMITK_DATA_SET_DOUBLE_0D_SCALING_CONTENT(AMITK_DATA_SET(fixed_slice[i_view]), i_voxel);
	value_moving = AMITK_DATA_SET_DOUBLE_0D_SCALING_CONTENT(AMITK_DATA_SET(moving_slice), i_voxel);

	/* treat any NaN or "out of volume" values as zeros */
	if (isnan(value_fixed)) value_fixed = 0;
	if (isnan(value_moving)) value_moving = 0;
      
	current_bin_number_fixed = floor((value_fixed-fixed_global_min)/bin_width_fixed);
	current_bin_number_moving = floor((value_moving-moving_global_min)/bin_width_moving);

	g_assert(current_bin_number_fixed < NUM_BINS);
	g_assert(current_bin_number_moving < NUM_BINS);
      
	/* Update the count in this particular bin combination by incrementing the counter*/
	mutual_information_array[current_bin_number_fixed][current_bin_number_moving]++;
      
	/* Update the *marginal* count. We could derive this, but we'll need it later, and computing
	   it one time only should improve computation speed */
	margin_fixed[current_bin_number_fixed]++;
	margin_moving[current_bin_number_moving]++;
	margin_total++;
      }
    }

    /* garbage collection */
    amitk_object_unref(AMITK_OBJECT(view_volume));
    amitk_object_unref(AMITK_OBJECT(moving_slice));
  }

  /* mark that we may not need to recompute the fixed slices next time around */
  *pfixed_slices_current = TRUE;


  
  /* calculate the "probability weight" of a single voxel */
  voxel_probability = (1.0 / margin_total);

  /* this is an idea for later: add the MI array to the study as a data set
  if (mi_matrix != NULL) {
    // and add the mutual information matrix to the study
    amitk_object_add_child(AMITK_OBJECT(fixed_ds), AMITK_OBJECT(mi_matrix)); // this adds a reference to the data set
    amitk_object_unref(mi_matrix); // so remove a reference 
  } else  {
    g_warning("Failed to generate filtered data set");
  }
   */

  mutual_information = 0;
   
  /* // print the column headings in order to debug the matrix
  g_print("\t%i", margin_moving[0]);
  for (j = 1; j < NUM_BINS ; j++) {
    g_print("\t%i", margin_moving[j]);
  } */
  
  mi_nan_count=0;
  for (i = 0; i < NUM_BINS ; i++) {
    // g_print("\n%i", margin_fixed[i]);  // print the row headings in order to debug the matrix
    for (j = 0; j < NUM_BINS ; j++) {
      /* when the probability of (x AND y) == 0, then the log (x AND y) is NaN. Therefore, test for this condition, and add a zero in order
         to avoid blowing up the equation
      */
      if (mutual_information_array[i][j] != 0) {
        incremental_mi = (mutual_information_array[i][j]*voxel_probability)*(log2((mutual_information_array[i][j] * voxel_probability)  /( (margin_fixed[i] * voxel_probability)*(margin_moving[j] * voxel_probability))));
      }
      else {
        // how do we deal with zero probability? There is probably a theoretical "best" answer, but this should be practical.
        // we are trying to avoid complete lack of overlap from looking like a "good fit"
        // things I've tried that *don't* work are i_mi=log2(EPSILON) and =log2(voxel_probability)
        incremental_mi = 0;
        mi_nan_count++;
      }
      /* you can choose either of the following g_print commands for debugging the matrix */
      //g_print("\t\%i", mutual_information_array[i][j] );  // for point-wise counts
      // g_print("\t\%4.3f", incremental_mi );               // for point-wise probability
      
      if isinf(incremental_mi) {
        //count it (because lots and lots of zeroes mean bad registration) and go to the next loop
        mi_nan_count++;
        // this is most often a problem when the incemental MI is infinity, due to divide by zero
      } else {
        // add it to the mutual information metric
        mutual_information += (incremental_mi);
      }

/*    // debugging code
      g_print("Current indices are: %i, %i\n", i, j);
      g_print("Current prob(x,y) is: %e\n", mutual_information_array[i][j] * voxel_probability);
      g_print("Current prob(x) is: %e\n", margin_moving[i] * voxel_probability);
      g_print("Current prob(y) is: %e\n", margin_fixed[j] * voxel_probability);
      if (margin_moving[i] == 0 || margin_fixed[j] == 0 ) {
        g_print("there is a zero at i=%i, j=%i!\n", i, j);
      }
      g_print("Incremental mutual information is: %e\n", incremental_mi);
      
      if (margin_moving[i] == 0 || margin_fixed[j] == 0 ) {
        g_print("there is a zero at i=%i, j=%i!\n", i, j);
      }
      g_print("Incremental mutual information is: %e\n", incremental_mi); */

    }
  }
  // g_print("\nThe mutual information sum is %f\n", mutual_information);
  // g_print("the count of NAN in the MI algorithm was\t%d\n", mi_nan_count);


  return mutual_information;
  
}

/* rot_x, y, and z are angles about the respective axes, in radians */
void rotate(AmitkPoint rotation, AmitkSpace * moving_space) {

  // apply the rotation to the data set
  if (rotation.x !=0) amitk_space_rotate_on_vector(AMITK_SPACE(moving_space), base_axes[AMITK_AXIS_X], rotation.x, zero_point );
  if (rotation.y !=0) amitk_space_rotate_on_vector(AMITK_SPACE(moving_space), base_axes[AMITK_AXIS_Y], rotation.y, zero_point );
  if (rotation.z !=0) amitk_space_rotate_on_vector(AMITK_SPACE(moving_space), base_axes[AMITK_AXIS_Z], rotation.z, zero_point );
    
}


/* This is the algorithm responsible for computing the transform which provides the maximum amount of mutual information for coregistration */
AmitkSpace * alignment_mutual_information(AmitkDataSet * moving_ds, 
					  AmitkDataSet * fixed_ds, 
					  AmitkPoint view_center,
					  amide_real_t thickness,
					  amide_time_t view_start_time,
					  amide_time_t view_duration,
					  gdouble * pointer_mutual_information_error,
					  AmitkUpdateFunc update_func,
					  gpointer update_data) {
  
  #define ITERATIONS_PER_LEVEL 3
  #define TRANSLATION_MAX_DISTANCE 10
  #define TRANSLATION_TARGET_PRECISION 0.001
  
  // twenty degrees:
  #define ROTATION_MAX_ANGLE (20*(M_PI/180))
  #define ROTATION_TARGET_PRECISION (0.1*(M_PI/180))
  #define INITIAL_STEP_SIZE 8
  #define MIN_STEP_SIZE 8

  AmitkSpace * transform_space;
  AmitkSpace * new_space;
  AmitkSpace * last_best_space;
  gdouble translation_precision, rotation_precision, step_size;
  //  GRand * random_generator;
  gdouble current_mi = 0, best_mi = 0;
  AmitkPoint best_shift, best_rotation;
  AmitkPoint current_shift, current_rotation;
  gchar * temp_string;
  gboolean continue_work = TRUE;
  AmitkDataSet * fixed_slice[AMITK_VIEW_NUM] = {NULL, NULL, NULL};
  gboolean fixed_slices_current;
  AmitkView i_view;

  //  random_generator = g_rand_new();
  
  /* =======================================================================================*/
  /* determine the translation which maximizes shared information                           */
  /* first pass of pyramidal descent (course alignment followed by fine alignment           */
  /* =======================================================================================*/

  new_space = amitk_space_copy(AMITK_SPACE(moving_ds));
  last_best_space = amitk_space_copy(new_space);

  
  translation_precision = TRANSLATION_MAX_DISTANCE;
  rotation_precision = ROTATION_MAX_ANGLE;
  step_size = INITIAL_STEP_SIZE;
  fixed_slices_current = FALSE;

  /* set baseline characteristics, including baseline space and initial error */
  best_mi = calculate_mutual_information(fixed_ds, moving_ds, new_space, step_size, &fixed_slices_current, fixed_slice,
					 view_center, thickness, view_start_time, view_duration);
#ifdef AMIDE_DEBUG
  g_print("initial mi %f\n", best_mi);
#endif
  
  if (update_func != NULL) {
    temp_string = g_strdup_printf(_("Maximizing the mutual information"));
    continue_work = (*update_func)(update_data, temp_string, (gdouble) 0.0);
    g_free(temp_string);
  }

  
  while (continue_work && ((translation_precision > TRANSLATION_TARGET_PRECISION) || 
			   (rotation_precision > ROTATION_TARGET_PRECISION) || 
			   ( step_size > MIN_STEP_SIZE ))) {
#ifdef AMIDE_DEBUG
    g_print("starting descent\ttranslation precision=\t%4.4f\n\t\t\trotation precision=\t%4.4f\tdegrees\n", 
	    translation_precision, rotation_precision*180/M_PI );
#endif

    if (update_func != NULL) 
      continue_work = (*update_func)(update_data, NULL, (gdouble) -1.0);

    best_shift = zero_point;
          
    /* current_step_size = current_iteration * translation_precision / ITERATIONS_PER_LEVEL;   //
     * current_step_size = g_rand_double_range(random_generator, 0, translation_precision );
     */
    for ( current_shift.z = -translation_precision ; current_shift.z < translation_precision; current_shift.z += translation_precision / ITERATIONS_PER_LEVEL ) { 
      for ( current_shift.y = -translation_precision; current_shift.y < translation_precision; current_shift.y += translation_precision / ITERATIONS_PER_LEVEL ) { 
        for ( current_shift.x = -translation_precision; current_shift.x < translation_precision; current_shift.x += translation_precision / ITERATIONS_PER_LEVEL ) { 
          
          /* first test whether offset increases the mutual information */
	  amitk_space_shift_offset(AMITK_SPACE(new_space), current_shift);
	  current_mi = calculate_mutual_information( fixed_ds, moving_ds, new_space, step_size, &fixed_slices_current, fixed_slice,
						     view_center, thickness, view_start_time, view_duration);
          
          /*if this location gives a better mutual information, then keep it */
          if (current_mi > best_mi ) {
#ifdef AMIDE_DEBUG
            g_print("better translation fit at %4.4f\t%4.4f\t%4.4f with mi=\t%4.4f\n", 
		    current_shift.x, current_shift.y, current_shift.z, current_mi);
#endif
            
            best_mi = current_mi;
            best_shift = current_shift;
          }
          /* and unroll the transformation */
	  g_object_unref(new_space);
	  new_space = amitk_space_copy(last_best_space);
        }
      }
    }
    
    /* apply/store the transform that worked best */
    amitk_space_shift_offset(AMITK_SPACE(new_space), best_shift);
    g_object_unref(last_best_space);
    last_best_space = amitk_space_copy(new_space);

    /* =======================================================================================*/
    /* determine the rotation which maximizes shared information                              */
    /* =======================================================================================*/
    best_rotation = zero_point;
    
    for ( current_rotation.z = -rotation_precision ; current_rotation.z < rotation_precision; current_rotation.z += rotation_precision / ITERATIONS_PER_LEVEL ) { 
      for ( current_rotation.y = -rotation_precision; current_rotation.y < rotation_precision; current_rotation.y += rotation_precision / ITERATIONS_PER_LEVEL ) { 
        for ( current_rotation.x = -rotation_precision; current_rotation.x < rotation_precision; current_rotation.x += rotation_precision / ITERATIONS_PER_LEVEL ) { 
          
          /* first test whether offset increases the mutual information */
          rotate(current_rotation, AMITK_SPACE(new_space));
          current_mi = calculate_mutual_information( fixed_ds, moving_ds, new_space, step_size, &fixed_slices_current, fixed_slice,
						     view_center, thickness, view_start_time, view_duration);

	  /*if this location gives a better mutual information, then keep it */
          if (current_mi > best_mi ) {
#ifdef AMIDE_DEBUG
            g_print("better rotation fit at %4.4f\t%4.4f\t%4.4f\t with mi=\t%4.4f\n", 
		    current_rotation.x, current_rotation.y, current_rotation.z, current_mi);
#endif
            best_mi = current_mi;
	    best_rotation = current_rotation;
          }
          /* and unroll the transformation */
	  g_object_unref(new_space);
	  new_space = amitk_space_copy(last_best_space);
        }
      }      
    }
    
    /* apply/store the transform that worked best */
    rotate(best_rotation, AMITK_SPACE(new_space));
    g_object_unref(last_best_space);
    last_best_space = amitk_space_copy(new_space);

    /* update loop variables for next iteration */
    translation_precision = translation_precision * 0.70;
    rotation_precision = rotation_precision * 0.70;

    if (step_size != MIN_STEP_SIZE) {
      step_size = step_size / 2.0;
      if (step_size < MIN_STEP_SIZE ) 
	step_size = MIN_STEP_SIZE;
      fixed_slices_current = FALSE;
    }
  }
  
  if (update_func != NULL) /* remove progress bar */
    (*update_func)(update_data, NULL, (gdouble) 2.0); 

  /* calculate the transform we'll need to apply */
  transform_space = amitk_space_calculate_transform(AMITK_SPACE(moving_ds), new_space);

  /* garbage collection */
  if (last_best_space != NULL)
    g_object_unref(last_best_space);
  if (new_space != NULL)
    g_object_unref(new_space);
  for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view++) {
    if (fixed_slice[i_view] != NULL)
      amitk_object_unref(AMITK_OBJECT(fixed_slice[i_view]));
  }
    
  *pointer_mutual_information_error = best_mi;
  
  return transform_space;
  
}
