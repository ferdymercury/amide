/* fads.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2003 Andy Loening
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

#ifdef AMIDE_LIBGSL_SUPPORT

#ifndef __FADS_H__
#define __FADS_H__

/* header files that are always needed with this file */
#include "amitk_data_set.h"

typedef enum {
  FADS_TYPE_PLS,
  //  FADS_TYPE_TWO_COMPARTMENT,
  NUM_FADS_TYPES
} fads_type_t;

extern gchar * fads_type_name[];
extern gchar * fads_type_explanation[];
extern const char ** fads_type_xpm[];

void fads_svd_factors(AmitkDataSet * data_set, 
		      gint * pnum_factors,
		      gdouble ** pfactors,
		      gdouble ** pcomponents);
void fads_pls(AmitkDataSet * data_set, 
	      gint num_factors, 
	      gint num_iterations,
	      gdouble stopping_criteria,
	      gchar * output_filename,
	      gint num_blood_curve_constraints,
	      gint * blood_curve_constraint_frame,
	      gdouble * blood_curve_constraint_val,
	      gboolean (*update_func)(), 
	      gpointer update_data);
void fads_two_comp(AmitkDataSet * data_set, 
		   gint num_iterations,
		   gdouble stopping_criteria,
		   gchar * output_filename,
		   gint num_blood_curve_constraints,
		   gint * blood_curve_constraint_frame,
		   gdouble * blood_curve_constraint_val,
		   gboolean (*update_func)(), 
		   gpointer update_data);

#endif /* __FADS_H__ */
#endif /* AMIDE_LIBGSL_SUPPORT */
