/* fads.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2003-2014 Andy Loening
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

#ifdef AMIDE_LIBGSL_SUPPORT

#ifndef __FADS_H__
#define __FADS_H__

/* header files that are always needed with this file */
#include "amitk_data_set.h"

typedef enum {
  FADS_TYPE_PCA,
  FADS_TYPE_PLS,
  FADS_TYPE_TWO_COMPARTMENT,
  NUM_FADS_TYPES
} fads_type_t;

typedef enum {
  /*  FADS_MINIMIZER_STEEPEST_DESCENT, */
  FADS_MINIMIZER_CONJUGATE_FR,
  FADS_MINIMIZER_CONJUGATE_PR,
  /*  FADS_MINIMIZER_VECTOR_BFGS, */
  NUM_FADS_MINIMIZERS
} fads_minimizer_algorithm_t;


extern gchar * fads_minimizer_algorithm_name[];
extern gchar * fads_type_name[];
extern gchar * fads_type_explanation[];
extern const guint8 * fads_type_icon[];

void fads_svd_factors(AmitkDataSet * data_set, 
		      gint * pnum_factors,
		      gdouble ** pfactors);
void fads_pca(AmitkDataSet * data_set, 
	      gint num_factors,
	      gchar * output_filename,
	      AmitkUpdateFunc update_func,
	      gpointer update_data);
void fads_pls(AmitkDataSet * data_set, 
	      gint num_factors, 
	      fads_minimizer_algorithm_t minimizer_algorithm,
	      gint max_iterations,
	      gdouble stopping_criteria,
	      gboolean sum_factors_equal_one,
	      gdouble beta,
	      gchar * output_filename,
	      gint num_blood_curve_constraints,
	      gint * blood_curve_constraint_frame,
	      gdouble * blood_curve_constraint_val,
	      GArray * initial_curves,
	      AmitkUpdateFunc update_func,
	      gpointer update_data);
void fads_two_comp(AmitkDataSet * data_set, 
		   fads_minimizer_algorithm_t minimizer_algorithm,
		   gint max_iterations,
		   gint tissue_types,
		   gdouble k12,
		   gdouble k21,
		   gdouble stopping_criteria,
		   gboolean sum_factors_equal_one,
		   gchar * output_filename,
		   gint num_blood_curve_constraints,
		   gint * blood_curve_constraint_frame,
		   gdouble * blood_curve_constraint_val,
		   AmitkUpdateFunc update_func,
		   gpointer update_data);

#endif /* __FADS_H__ */
#endif /* AMIDE_LIBGSL_SUPPORT */
