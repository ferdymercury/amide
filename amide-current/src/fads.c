/* fads.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2003 Andy Loening
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
#ifdef AMIDE_LIBGSL_SUPPORT
#include <time.h>
#include <glib.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_multimin.h>
#include "fads.h"
#include "amitk_data_set_FLOAT_0D_SCALING.h"

gchar * fads_type_name[] = {
  "Principle Component Analysis",
  //  "Factor Analysis - Di Paola et al.",
  "Penalized Least Squares - Sitek, et al.",
  "2 compartment model"
};

gchar * fads_type_explanation[] = {
  "Prinicple Component Analysis",

  //  "Basic factor analysis of dynamic systems with positivity "
  //  "constraints, as described in Di Paola, et al., IEEE Trans. "
  //  "Nuc. Sci., 1982",

  "Principle component analysis with positivity constraints "
  "and a penalized least squares objective, an adaptation "
  "of Sitek, et al., IEEE Trans. Med. Imag., 2002",
  
  "Standard 2 compartment model with a single leak"
};


#include "../pixmaps/two_compartment.xpm"
const char ** fads_type_xpm[NUM_FADS_TYPES] = {
  NULL,
  //  NULL,
  NULL,
  //  two_compartment_xpm
};


static gint perform_svd(gsl_matrix * a, gsl_matrix * v, gsl_vector * s) {

  gint status;
  gsl_vector * w;

  w = gsl_vector_alloc(s->size);
  g_return_val_if_fail(w != NULL, -1);

  status = gsl_linalg_SV_decomp(a, v, s, w);
  gsl_vector_free(w);

  return status;
}

void fads_svd_factors(AmitkDataSet * data_set, 
		      gint * pnum_factors,
		      gdouble ** pfactors) {

  gsl_matrix * matrix_a=NULL;
  gsl_matrix * matrix_v=NULL;
  gsl_vector * vector_s=NULL;
  AmitkVoxel dim, i_voxel;
  gint m,n, i;
  amide_data_t value;
  gdouble * factors;
  gint status;

  g_return_if_fail(AMITK_IS_DATA_SET(data_set));

  dim = AMITK_DATA_SET_DIM(data_set);
  n = dim.t;
  m = dim.x*dim.y*dim.z;

  if (n == 1) {
    g_warning("need dynamic data set in order to perform factor analysis");
    goto ending;
  }

  /* do all the memory allocations upfront */
  if ((matrix_a = gsl_matrix_alloc(m,n)) == NULL) {
    g_warning("Failed to allocate %dx%d array", m,n);
    goto ending;
  }
  
  if ((matrix_v = gsl_matrix_alloc(n,n)) == NULL) {
    g_warning("Failed to allocate %dx%d array", n,n);
    goto ending;
  }

  if ((vector_s = gsl_vector_alloc(n)) == NULL) {
    g_warning("Failed to allocate %d vector", n);
    goto ending;
  }

  /* fill in the a matrix */
  for (i_voxel.t = 0; i_voxel.t < dim.t; i_voxel.t++) {
    i = 0;
    for (i_voxel.z = 0; i_voxel.z < dim.z; i_voxel.z++)
      for (i_voxel.y = 0; i_voxel.y < dim.y; i_voxel.y++)
	for (i_voxel.x = 0; i_voxel.x < dim.x; i_voxel.x++, i++) {
	  value = amitk_data_set_get_value(data_set, i_voxel);
	  gsl_matrix_set(matrix_a, i, i_voxel.t, value);
	}
  }

  /* get the singular value decomposition of the correlation matrix -> matrix_a = U*S*Vt
     notes: 
        the function will place the value of U into matrix_a 
        gsl_linalg_SV_decomp_jacobi will return an unsuitable matrix_v, don't use it 
  */
  status = perform_svd(matrix_a, matrix_v, vector_s);
  if (status != 0) g_warning("SV decomp returned error: %s", gsl_strerror(status));

  /* transfering data */
  if (pnum_factors != NULL)
    *pnum_factors = n;

  if (pfactors != NULL) {
    factors = g_try_new(gdouble, n);
    if (factors == NULL) {
      g_warning("Failed to allocate %d factor array", n);
      goto ending;
    }
    *pfactors=factors;
    for (i=0; i<n; i++) 
      factors[i] = gsl_vector_get(vector_s, i);
  }

 ending:

  /* garbage collection */

  if (matrix_a != NULL) {
    gsl_matrix_free(matrix_a);
    matrix_a = NULL;
  }

  if (matrix_v != NULL) {
    gsl_matrix_free(matrix_v);
    matrix_v = NULL;
  }

  if (vector_s != NULL) {
    gsl_vector_free(vector_s);
    vector_s = NULL;
  }

  return;
}


static void perform_pca(AmitkDataSet * data_set, 
			gint num_factors,
			gsl_matrix ** return_u,
			gsl_vector ** return_s, 
			gsl_matrix ** return_v) {

  AmitkVoxel dim, i_voxel;
  guint num_voxels, num_frames;
  gsl_matrix * u = NULL;
  gsl_matrix * v = NULL;
  gsl_vector * s = NULL;
  gsl_matrix * small_u;
  gsl_matrix * small_v;
  gsl_vector * small_s;
  guint i, f, j;
  gint status;
  gdouble total;

  dim = AMITK_DATA_SET_DIM(data_set);
  num_voxels = dim.x*dim.y*dim.z;
  num_frames = dim.t;

  u = gsl_matrix_alloc(num_voxels, num_frames);
  if (u == NULL) {
    g_warning("failed to alloc matrix, size %dx%d", num_voxels, num_frames);
    goto ending;
  }

  if ((v = gsl_matrix_alloc(num_frames,num_frames)) == NULL) {
    g_warning("Failed to allocate %dx%d array", num_frames, num_frames);
    goto ending;
  }

  if ((s = gsl_vector_alloc(num_frames)) == NULL) {
    g_warning("Failed to allocate %d vector", num_frames);
    goto ending;
  }

  /* copy the info into the matrix */
  for (i_voxel.t = 0; i_voxel.t < num_frames; i_voxel.t++) {
    i = 0;
    for (i_voxel.z = 0; i_voxel.z < dim.z; i_voxel.z++)
      for (i_voxel.y = 0; i_voxel.y < dim.y; i_voxel.y++)
	for (i_voxel.x = 0; i_voxel.x < dim.x; i_voxel.x++, i++) 
	  gsl_matrix_set(u, i, i_voxel.t, amitk_data_set_get_value(data_set, i_voxel));
  }

  /* do Singular Value decomposition */
  status = perform_svd(u, v, s);
  if (status != 0) g_warning("SV decomp returned error: %s", gsl_strerror(status));

  /* do some obvious flipping */
  for (f=0; f<num_factors; f++) {
    total = 0;
    for (j=0; j<num_frames; j++)
      total += gsl_matrix_get(v, j, f);
    if (total < 0) {
      for (j=0; j<num_frames; j++)
	gsl_matrix_set(v, j, f, -1*gsl_matrix_get(v, j, f));
      for (i=0; i<num_voxels; i++)
	gsl_matrix_set(u, i, f, -1*gsl_matrix_get(u, i, f));
    } 
  }

  /* copy the SVD info into smaller matrices */
  if (return_u != NULL) {
    small_u = gsl_matrix_alloc(num_voxels, num_factors);
    if (small_u == NULL) {
      g_warning("failed to alloc matrix size %dx%d", num_voxels, num_factors);
      goto ending;
    }
    for (f=0; f<num_factors; f++)
      for (i=0; i<num_voxels; i++)
	gsl_matrix_set(small_u, i, f, gsl_matrix_get(u, i, f));
    *return_u = small_u;
  }
  gsl_matrix_free(u);
  u = NULL;

  if (return_s != NULL) {
    small_s = gsl_vector_alloc(num_factors);
    if (small_s == NULL) {
      g_warning("failed to alloc vector size %d", num_factors);
      goto ending;
    }
    for (f=0; f<num_factors; f++)
      gsl_vector_set(small_s, f, gsl_vector_get(s, f));
    *return_s = small_s;
  }
  gsl_vector_free(s);
  s = NULL;

  if (return_v != NULL) {
    small_v = gsl_matrix_alloc(num_frames, num_factors);
    if (small_v == NULL) {
      g_warning("failed to alloc matrix size %dx%d", num_frames, num_factors);
      goto ending;
    }
    for (j=0; j<num_frames; j++)
      for (f=0; f<num_factors; f++)
	gsl_matrix_set(small_v, j, f, gsl_matrix_get(v, j, f));
    *return_v = small_v;
  }
  gsl_matrix_free(v);
  v = NULL;
  

 ending:

  if (u != NULL) {
    gsl_matrix_free(u);
    u = NULL;
  }

  if (v != NULL) {
    gsl_matrix_free(v);
    v = NULL;
  }

  if (s != NULL) {
    gsl_vector_free(s);
    s = NULL;
  }

  return;
}

void fads_pca(AmitkDataSet * data_set, 
	      gint num_factors,
	      gchar * output_filename,
	      gboolean (*update_func)(),
	      gpointer update_data) {

  gsl_matrix * u=NULL;
  gsl_vector * s=NULL;
  gsl_matrix * v=NULL;
  AmitkVoxel dim, i_voxel;
  guint num_frames;
  guint i, f, t;
  AmitkDataSet * new_ds;
  gchar * temp_string;
  FILE * file_pointer=NULL;
  time_t current_time;
  amide_time_t time_midpoint;

  dim = AMITK_DATA_SET_DIM(data_set);
  num_frames = dim.t;
  dim.t = 1;

  perform_pca(data_set, num_factors, &u, &s, &v);

  /* copy the data on over */
  for (f=0; f<num_factors; f++) {

    new_ds = amitk_data_set_new_with_data(AMITK_FORMAT_FLOAT, dim, AMITK_SCALING_TYPE_0D);
    if (new_ds == NULL) {
      g_warning("failed to allocate new_ds");
      goto ending;
    }

    i=0;
    i_voxel.t = 0;
    for (i_voxel.z=0; i_voxel.z<dim.z; i_voxel.z++) 
      for (i_voxel.y=0; i_voxel.y<dim.y; i_voxel.y++) 
	for (i_voxel.x=0; i_voxel.x<dim.x; i_voxel.x++, i++) 
	  AMITK_DATA_SET_FLOAT_0D_SCALING_SET_CONTENT(new_ds, i_voxel, 
						      gsl_matrix_get(u, i, f));

    temp_string = g_strdup_printf("component %d", f+1);
    amitk_object_set_name(AMITK_OBJECT(new_ds),temp_string);
    g_free(temp_string);
    amitk_space_copy_in_place(AMITK_SPACE(new_ds), AMITK_SPACE(data_set));
    amitk_data_set_calc_max_min(new_ds, NULL, NULL);
    amitk_data_set_set_voxel_size(new_ds, AMITK_DATA_SET_VOXEL_SIZE(data_set));
    amitk_data_set_set_modality(new_ds, AMITK_DATA_SET_MODALITY(data_set));
    amitk_data_set_calc_far_corner(new_ds);
    amitk_data_set_set_threshold_max(new_ds, 0, AMITK_DATA_SET_GLOBAL_MAX(new_ds));
    amitk_data_set_set_threshold_min(new_ds, 0, AMITK_DATA_SET_GLOBAL_MIN(new_ds));
    amitk_data_set_set_interpolation(new_ds, AMITK_DATA_SET_INTERPOLATION(data_set));
    amitk_object_add_child(AMITK_OBJECT(data_set), AMITK_OBJECT(new_ds));
    g_object_unref(new_ds); /* add_child adds a reference */
  }

  /* and output the curves */
  if ((file_pointer = fopen(output_filename, "w")) == NULL) {
    g_warning("couldn't open: %s for writing PCA analyses", output_filename);
    goto ending;
  }

  time(&current_time);
  fprintf(file_pointer, "# %s: Analysis File for %s\n",PACKAGE, AMITK_OBJECT_NAME(data_set));
  fprintf(file_pointer, "# generated on %s", ctime(&current_time));
  fprintf(file_pointer, "# using %s\n",fads_type_name[FADS_TYPE_PCA]);
  fprintf(file_pointer, "#\n");

  fprintf(file_pointer, "# frame\ttime midpt (s)\tfactor:\n");
  fprintf(file_pointer, "#\t");
  for (f=0; f<num_factors; f++) fprintf(file_pointer, "\t\t%d", f+1);
  fprintf(file_pointer, "\n");

  for (t=0; t<AMITK_DATA_SET_NUM_FRAMES(data_set); t++) {
    time_midpoint = amitk_data_set_get_midpt_time(data_set, t);

    fprintf(file_pointer, "  %d", t);
    fprintf(file_pointer, "\t%g\t", time_midpoint);
    for (f=0; f<num_factors; f++) {
      fprintf(file_pointer, "\t%g", 
	      gsl_vector_get(s, f)* gsl_matrix_get(v, t, f));
    }
    fprintf(file_pointer,"\n");
  }

 ending:

  if (file_pointer != NULL) {
    fclose(file_pointer);
    file_pointer = NULL;
  }

  if (u != NULL) {
    gsl_matrix_free(u);
    u = NULL;
  }

  if (v != NULL) {
    gsl_matrix_free(v);
    v = NULL;
  }

  if (s != NULL) {
    gsl_vector_free(s);
    s = NULL;
  }

  return;
}


/* factor analysis, as explained by Di Paola, et al. */
void fads_fads(AmitkDataSet * data_set,
	       gint num_factors,
	       gint max_iterations,
	       gdouble stopping_criteria,
	       gchar * output_filename,
	       gint num_blood_curve_constraints,
	       gint * blood_curve_constraint_frame,
	       gdouble * blood_curve_constraint_val,
	       gboolean (*update_func)(), 
	       gpointer update_data) {

  
  //  gsl_matrix * F=NULL;


  //  F = gsl_matrix_alloc(AMITK_DATA_SET_NUM_FRAMES(data_set), 
  
  //  ending


}










static gdouble calc_magnitude(AmitkDataSet * ds, gdouble * weight) {

  gdouble magnitude;
  AmitkVoxel i_voxel;
  AmitkVoxel dim;

  dim = AMITK_DATA_SET_DIM(ds);
  magnitude = 0;

  for (i_voxel.t=0; i_voxel.t<dim.t; i_voxel.t++) 
    for (i_voxel.z=0; i_voxel.z<dim.z; i_voxel.z++) 
      for (i_voxel.y=0; i_voxel.y<dim.y; i_voxel.y++) 
	for (i_voxel.x=0; i_voxel.x<dim.x; i_voxel.x++)
	  magnitude += weight[i_voxel.t]*fabs(amitk_data_set_get_value(ds, i_voxel));


  return magnitude;
}


/* returned array needs to be free'd */
gdouble * calc_weights(AmitkDataSet * ds) {

  gdouble * weight;
  gint num_frames, t;
  gdouble total_weight;

  num_frames = AMITK_DATA_SET_NUM_FRAMES(ds);

  weight = g_try_new(gdouble, num_frames);
  g_return_val_if_fail(weight != NULL, NULL); /* make sure we've malloc'd it */

  /* setup the weights, each frame has a weight proportional to the duration */
  total_weight = 0;
  for (t=0; t<num_frames; t++) {
    weight[t] = amitk_data_set_get_frame_duration(ds, t);
    total_weight += weight[t];
  }
  for (t=0; t<num_frames; t++)
    weight[t] *= (num_frames/total_weight);

  return weight;
}












typedef struct pls_params_t {
  AmitkDataSet * data_set;
  AmitkVoxel dim;
  gdouble a;
  gdouble b;
  gdouble c;
  gint num_voxels; /* voxels/frame */
  gint num_frames; 
  gint num_factors;
  gint coef_offset; /* num_factors*num_frames */
  gint num_variables; /* coef_offset+num_voxels*num_factors*/

  gdouble * forward_error; /* our estimated data (the forward problem), subtracted by the actual data */
  gdouble * weight; /* the appropriate weight (frame dependent) */
  gdouble * coef_total; /* sum of the coefficients for each pixel */

  /* normalization factors for the penalization constraint */
  /* and dot products between the different columns */
  gdouble * coef_norm_factor; 
  gdouble * coef_dot_prod; 

  gint num_blood_curve_constraints;
  gint * blood_curve_constraint_frame;
  gdouble * blood_curve_constraint_val;

  gdouble orth;
  gdouble blood;
  gdouble ls;
  gdouble neg;
} pls_params_t;



static void pls_calc_forward_error(pls_params_t * p, const gsl_vector *v) {

  gint i, j, f;
  AmitkVoxel i_voxel;
  gdouble inner;

  for (i_voxel.t=0; i_voxel.t<p->dim.t; i_voxel.t++) {
    i=p->coef_offset; /* what to skip in v to get to the coefficients */
    j=0;
    for (i_voxel.z=0; i_voxel.z<p->dim.z; i_voxel.z++) {
      for (i_voxel.y=0; i_voxel.y<p->dim.y; i_voxel.y++) {
	for (i_voxel.x=0; i_voxel.x<p->dim.x; i_voxel.x++, i+=p->num_factors, j+=p->num_frames) {

	  inner = 0.0;
	  for (f=0; f<p->num_factors; f++) {
	    inner += 
	      gsl_vector_get(v, i+f)*
	      gsl_vector_get(v, f*p->num_frames+i_voxel.t);
	  }
	  p->forward_error[j+i_voxel.t] = inner-amitk_data_set_get_value(p->data_set,i_voxel);
	}
      }
    }
  }

  return;
}

static void pls_calc_norm_factors(pls_params_t * p, const gsl_vector *v) {

  gint i, f;
  gdouble inner, temp;

  for (f=0; f<p->num_factors; f++) {
    inner = 0;
    for (i=p->coef_offset; i<p->num_variables; i+=p->num_factors) {
      temp = gsl_vector_get(v, i+f);
      inner += temp*temp;
    }
    p->coef_norm_factor[f] = sqrt(inner);
  }
  
  return;
}

static void pls_calc_dot_prods(pls_params_t * p, const gsl_vector *v) {

  gdouble inner;
  gint f, q, i;

  for (f=0; f < p->num_factors-1; f++) {
    for (q=f+1; q < p->num_factors; q++) {
      inner = 0;
      for (i=p->coef_offset; i<p->num_variables; i+=p->num_factors)
	inner += fabs(gsl_vector_get(v, i+f))*fabs(gsl_vector_get(v, i+q));
      p->coef_dot_prod[f*p->num_factors+q] = inner;
      p->coef_dot_prod[q*p->num_factors+f] = inner;
    }
  }

  return;
}

static void pls_calc_coef_totals(pls_params_t *p, const gsl_vector *v) {

  gint i,j,f;
  gdouble total;

  for (i=p->coef_offset, j=0; i<p->num_variables; i+=p->num_factors, j++) {
    total=0;
    for (f=0;f<p->num_factors; f++)
      total += fabs(gsl_vector_get(v, i+f));
    p->coef_total[j]=total;
  }

  return;
}

static gdouble pls_calc_function(pls_params_t * p, const gsl_vector *v) {

  gdouble ls_answer=0.0;
  gdouble neg_answer=0.0;
  gdouble orth_answer=0.0;
  gdouble blood_answer=0.0;
  gdouble temp;
  gint i, j, f, q;

  /* the Least Squares objective */
  ls_answer = 0.0;
  for (j=0; j<p->num_frames; j++) {
    for (i=0; i<p->num_voxels; i++) {
      temp = p->forward_error[i*p->num_frames+j];
      ls_answer += p->weight[j]*temp*temp;
    }
  }

  /* the non-negativity objective */
  neg_answer = 0.0;
  for (j=0; j<p->num_frames; j++) {
    for (f=0; f<p->num_factors; f++) {
      temp = gsl_vector_get(v, f*p->num_frames+j);
      if (temp < 0.0)
	neg_answer += p->weight[j]*temp*temp;
    }
  }
  for (i= p->coef_offset; i < p->num_variables; i++) {
    temp = gsl_vector_get(v, i);
    if (temp < 0.0)
      neg_answer += temp*temp;
  }

  /* also include the constraint that coefficient totals are ~= 1.0 */
  for (j=0; j<p->num_voxels; j++) {
    temp = p->coef_total[j];
    if (temp > 1.0) {
      temp -= 1.0;
      neg_answer += temp*temp;
    }
  }
  neg_answer *= p->a;

  /* the orthogonality objective */
  orth_answer = 0.0;
  for (f=0; f < p->num_factors-1; f++) {
    for (q=f+1; q < p->num_factors; q++) {
      temp = p->coef_norm_factor[f]*p->coef_norm_factor[q];
      if (temp > EPSILON) /* guard for division by close to zero */
	orth_answer+=p->coef_dot_prod[f*p->num_factors+q]/temp;
    }
  }
  orth_answer *= p->b; /* weight this objective by b */

  /* blood curve constraints */
  blood_answer = 0;
  for (i=0; i<p->num_blood_curve_constraints; i++) {
    temp = gsl_vector_get(v, p->blood_curve_constraint_frame[i]);
    temp -= p->blood_curve_constraint_val[i];
    blood_answer += temp*temp;
  }
  blood_answer *= p->c;

  /* keep track of the three */
  p->ls = ls_answer;
  p->neg = neg_answer;
  p->orth = orth_answer;
  p->blood = blood_answer;

  return ls_answer+neg_answer+orth_answer+blood_answer;
}

static void pls_calc_derivative(pls_params_t * p, const gsl_vector *v, gsl_vector *df) {

  gdouble ls_answer=0.0;
  gdouble neg_answer=0.0;
  gdouble orth_answer;
  gdouble blood_answer=0.0;
  gdouble coef_value, factor_value;
  gdouble norm;
  gint i, j, k, l, f, q;

  
  /* calculate first for the factor variables */
  for (q= 0; q < p->num_factors; q++) {
    for (j=0; j<p->num_frames; j++) {
      
      factor_value = gsl_vector_get(v, q*p->num_frames+j);
      
      /* the Least Squares objective */
      ls_answer = 0.0;
      /* p->coef_offset is what to skip in v to get to the coefficients */
      for (i=p->coef_offset, k=0; i<p->num_variables; i+=p->num_factors, k+=p->num_frames) 
	ls_answer += gsl_vector_get(v, i+q)*p->forward_error[k+j];
      ls_answer *= 2.0*p->weight[j];

      /* the non-negativity objective */
      if (factor_value < 0.0)
	neg_answer = p->a*p->weight[j]*2*factor_value;
      else
	neg_answer = 0;

      /* blood curve constraints */
      blood_answer = 0;
      if (q == 0)  /* 1st factor is blood curve */
	for (i=0; i<p->num_blood_curve_constraints; i++) 
	  if (p->blood_curve_constraint_frame[i] == j) {
	    blood_answer += (factor_value-p->blood_curve_constraint_val[i]);
	  }
      blood_answer *= 2*p->c;

      gsl_vector_set(df, q*p->num_frames+j, ls_answer+neg_answer+blood_answer);
    }
  }

  /* now calculate for the coefficient variables */
  for (q= 0; q < p->num_factors; q++) {
    /* p->coef_offset is what to skip in v to get to the coefficients */
    for (i=p->coef_offset, k=0, l=0; i < p->num_variables; i+=p->num_factors, k+=p->num_frames, l++) {

      coef_value = gsl_vector_get(v, i+q);

      /* the Least Squares objective */
      ls_answer = 0;
      for (j=0; j<p->num_frames; j++)
	ls_answer += p->weight[j]*gsl_vector_get(v, q*p->num_frames+j)*p->forward_error[k+j];
      ls_answer *= 2.0;

      /* the non-negativity and ~= 1.0 objectives */
      neg_answer=0;
      if (coef_value < 0.0) neg_answer += p->a*2*coef_value;
      else if (p->coef_total[l] > 1.0) 
	neg_answer += p->a*2*(p->coef_total[l]-1.0);
	
      /* the orthogonality objective */
      orth_answer = 0;
      norm = p->coef_norm_factor[q];
      for (f=0; f < p->num_factors; f++)
	if (f!=q) { /* don't want dot product with self */
	  if (norm > EPSILON) { /* guard for division by close to zero  */
	    orth_answer +=
	      (gsl_vector_get(v, i+f) -
	       p->coef_dot_prod[f*p->num_factors+q]*fabs(coef_value)/(norm*norm))/
	      p->coef_norm_factor[f];
	  }
	}
      orth_answer *= p->b/norm;
      
      gsl_vector_set(df, i+q, ls_answer+neg_answer+orth_answer);
    }
  }

  return;
}

/* calculate the penalized least squares objective function */
static double pls_f (const gsl_vector *v, void *params) {

  pls_params_t * p = params;

  pls_calc_forward_error(p, v);
  pls_calc_norm_factors(p, v);
  pls_calc_dot_prods(p,v);
  pls_calc_coef_totals(p,v);

  return pls_calc_function(p, v);
}




/* The gradient of f, df = (df/dCip, df/dFpt). */
static void pls_df (const gsl_vector *v, void *params, gsl_vector *df) {
  
  pls_params_t * p = params;

  pls_calc_forward_error(p, v);
  pls_calc_norm_factors(p, v);
  pls_calc_dot_prods(p,v);
  pls_calc_coef_totals(p,v);

  pls_calc_derivative(p, v, df);

  return;
}



/* Compute both f and df together. */
static void pls_fdf (const gsl_vector *v, void *params,  double *f, gsl_vector *df) {

  pls_params_t * p = params;

  pls_calc_forward_error(p, v);
  pls_calc_norm_factors(p, v);
  pls_calc_dot_prods(p,v);
  pls_calc_coef_totals(p,v);

  *f = pls_calc_function(p,v);
  pls_calc_derivative(p, v, df);
  
  return;
}




/* run the penalized least squares algorithm for factor analysis.

   -this method is described in:
   Sitek, et al., IEEE Trans Med Imaging, 21, 2002, pages 216-225 

   differences in my version:
   1-instead of reweighting the coefficents at the end of the fitting,
   the coefficients are penalized for being greater then 1.
   2-adjusts b value very slow, to try to prevent stalling
   3-weights the least square terms by the sqrt of the respective frame duration


   gsl supports minimizing on only a single vector space, so that
   vector is setup as follows
   
   M = num_voxels;
   N = num_frames
   F = num_factors;

   factors = [F*N]
   coefficients = [M*F]

   x = [factor(1,1) factor(1,2) ...... factor(1,N) 
        factor(2,1) factor(2,2) ...... factor(2,N)
	   ..          ..                 ..  
	factor(F,1) factor(F,2) ...... factor(F,N)
	coef(1,1)   coef(1,2)   ...... coef(1,F)
	coef(2,1)   coef(2,2)   ...... coef(2,F)
	   ..          ..                 ..  
	coef(M,1)   coef(M,2)   ...... coef(M,F)]

  
   forward_error is [M*N]



*/

void fads_pls(AmitkDataSet * data_set, 
	      gint num_factors, 
	      gint max_iterations,
	      gdouble stopping_criteria,
	      gchar * output_filename,
	      gint num_blood_curve_constraints,
	      gint * blood_curve_constraint_frame,
	      gdouble * blood_curve_constraint_val,
	      gboolean (*update_func)(), 
	      gpointer update_data) {

  gsl_multimin_fdfminimizer * multmin_minimizer = NULL;
  gsl_multimin_function_fdf multmin_func;
  AmitkVoxel dim;
  pls_params_t p;
  gint iter=0;
  gsl_vector * initial=NULL;
  gint i, f, j, k;
  gint status;
  FILE * file_pointer=NULL;
  time_t current_time;
  gchar * temp_string;
  gboolean continue_work=TRUE;
  gdouble temp1, temp2, mult;
  amide_time_t time_midpoint;
  gdouble init_value;
  gdouble time_constant;
  gdouble time_start;
  gdouble magnitude;
  AmitkDataSet * new_ds;
  AmitkVoxel i_voxel;
  gdouble b_fraction = 0.1;
  gdouble b_values[100];
  gint b_index=0;
  gdouble new_b;
  gdouble * facts; // remove
  gsl_vector * s;
  gsl_matrix * u;
  gsl_matrix * v;
  

  g_return_if_fail(AMITK_IS_DATA_SET(data_set));
  dim = AMITK_DATA_SET_DIM(data_set);
  g_return_if_fail(num_factors <= dim.t);

  /* initialize our parameter structure */
  p.data_set = data_set;
  p.dim = dim;
  p.a = AMITK_DATA_SET_GLOBAL_MAX(data_set)*1000000.0;
  p.b = 100; 
  p.c = p.a;
  p.ls = 1.0;
  p.neg = 1.0;
  p.orth = 1.0;
  p.blood = 1.0;
  p.num_voxels = dim.z*dim.y*dim.x;
  p.num_frames = dim.t;
  p.num_factors = num_factors;
  p.coef_offset = p.num_factors*p.num_frames;
  p.num_variables = p.coef_offset+p.num_factors*p.num_voxels;
  p.num_blood_curve_constraints = num_blood_curve_constraints;
  p.blood_curve_constraint_frame = blood_curve_constraint_frame;
  p.blood_curve_constraint_val = blood_curve_constraint_val;

  /* more sanity checks */
  for (i=0; i<p.num_blood_curve_constraints; i++) {
    g_return_if_fail(p.blood_curve_constraint_frame[i] < p.dim.t);
  }

  p.coef_norm_factor = g_try_new(gdouble, p.num_factors);
  g_return_if_fail(p.coef_norm_factor != NULL); 

  p.coef_dot_prod = g_try_new(gdouble, p.num_factors*p.num_factors);
  g_return_if_fail(p.coef_dot_prod != NULL);
  /* initialize the diagonal... although it should never get used */
  for (f=0; f<p.num_factors; f++)
    p.coef_dot_prod[f*(1+p.num_factors)] = 0.0;

  p.forward_error = g_try_new(gdouble, p.num_frames*p.num_voxels);
  g_return_if_fail(p.forward_error != NULL); /* make sure we've malloc'd it */

  p.coef_total = g_try_new(gdouble, p.num_voxels);
  g_return_if_fail(p.coef_total != NULL);

  /* calculate the weights and magnitude */
  p.weight = calc_weights(p.data_set);
  g_return_if_fail(p.weight != NULL); /* make sure we've malloc'd it */
  magnitude = calc_magnitude(p.data_set, p.weight);


  /* set up gsl */
  multmin_func.f = pls_f;
  multmin_func.df = pls_df;
  multmin_func.fdf = pls_fdf;
  multmin_func.n = p.num_variables;
  multmin_func.params = &p;

  multmin_minimizer = gsl_multimin_fdfminimizer_alloc (gsl_multimin_fdfminimizer_conjugate_pr,
  						       p.num_variables);
  if (multmin_minimizer == NULL) {
    g_warning("failed to allocate multidimensional minimizer");
    goto ending;
  }


  /* Starting point */
  initial = gsl_vector_alloc (p.num_variables);
  
  /* setting the factors to the principle components */
  perform_pca(p.data_set, p.num_factors, &u, &s, &v);

  /* need to initialize the factors, picking some quasi-exponential curves */
  /* use a time constant of 100th of the study length, as a guess */
  time_constant = amitk_data_set_get_end_time(p.data_set, AMITK_DATA_SET_NUM_FRAMES(p.data_set)-1);
  time_constant /= 100.0;

  /* gotta reference from somewhere */
  time_start = amitk_data_set_get_midpt_time(p.data_set, 0); 

  facts = g_try_new(gdouble, p.num_factors*p.num_frames); // remove
  for (f=0; f<p.num_factors; f+=2) {
    for (j=0; j<p.num_frames; j++) {
      time_midpoint = amitk_data_set_get_midpt_time(p.data_set, j); 

      mult = exp(-(time_midpoint-time_start)/time_constant); 

      temp1 = fabs(gsl_vector_get(s, f)*gsl_matrix_get(v, j, f));
      if (f+1 <p.num_factors) {
	temp2 = gsl_vector_get(s, f+1)*fabs(gsl_matrix_get(v, j, f+1));
	gsl_vector_set(initial, f*p.num_frames+j,
		       temp1*mult+temp2*(1-mult));
	gsl_vector_set(initial, (f+1)*p.num_frames+j,
		       temp1*(1-mult)+temp2*mult);
	facts[f*p.num_frames+j] = gsl_vector_get(initial, f*p.num_frames+j);
	facts[(f+1)*p.num_frames+j] = gsl_vector_get(initial, (f+1)*p.num_frames+j);
      } else {
	gsl_vector_set(initial, f*p.num_frames+j,temp1*mult);
	facts[f*p.num_frames+j] = gsl_vector_get(initial, f*p.num_frames+j);
      }
    }
    if (f & 0x1) /* double every other iteration */
      time_constant *=2.0;
  }

  /* and just set the coefficients to something logical */
  init_value = 1.0/num_factors;
  for (f=0; f<num_factors; f++)
    for (i=p.coef_offset, k=0; i<p.num_variables;i+=p.num_factors,k++)
      gsl_vector_set(initial, i+f, init_value);
  gsl_vector_free(s);
  gsl_matrix_free(u);
  gsl_matrix_free(v);


  /* run the objective function once, so that we can pick the b value */
  pls_f(initial, &p);

  /* adjust the b value */
  if (EQUAL_ZERO(p.orth)) 
    p.b = b_fraction;
  else
    p.b = (b_fraction*p.b*(p.ls))/p.orth;

  for (i=0; i<100; i++)
    b_values[i] = p.b;

  gsl_multimin_fdfminimizer_set(multmin_minimizer, &multmin_func, initial, 0.1, stopping_criteria);

  if (update_func != NULL) {
    temp_string = g_strdup_printf("Calculating Penalized Least Squares Factor Analysis:\n   %s", AMITK_OBJECT_NAME(data_set));
    continue_work = (*update_func)(update_data, temp_string, (gdouble) 0.0);
    g_free(temp_string);
  }


  do {
      iter++;
      status = gsl_multimin_fdfminimizer_iterate (multmin_minimizer);

      if (!status) 
	status = gsl_multimin_test_gradient (multmin_minimizer->gradient, stopping_criteria);

      if (update_func != NULL) 
	continue_work = (*update_func)(update_data, NULL, (gdouble) iter/max_iterations);

#if AMIDE_DEBUG  
      g_print("iter %d\t%g=%5.3g+%5.3g+%5.3g+%5.3g\ta=%g, b=%g\n", iter, 
	      100.0*gsl_multimin_fdfminimizer_minimum(multmin_minimizer)/magnitude,
	      100.0*p.ls/magnitude,
	      100.0*p.neg/magnitude,
	      100.0*p.orth/magnitude,
	      100.0*p.blood/magnitude,
	      p.a/magnitude,
	      p.b/magnitude);
#endif /* AMIDE_DEBUG */

      /* adjust the b value for the first 100 iterations, and every 16th iteration after that */
      //      if ((iter < 100) || ((iter & 0x1F) == 0x1F)) {
	if (!EQUAL_ZERO(p.orth)) {

	  /* adjust slowly, using a slow moving average to avoid stalling the minimization */
	  new_b = (b_fraction*p.b*(p.ls))/p.orth;

	  /* don't let the b value increase, might stall optimization*/
	  if (new_b < b_values[b_index]) {
	    b_values[b_index] = new_b;
	    b_index++;
	    if (b_index == 100) b_index = 0;

	    p.b = 0;
	    for (i=0; i<100; i++)
	      p.b += b_values[i]/100;
	  }
	}
	//      }

  } while ((status == GSL_CONTINUE) && (iter < max_iterations) && continue_work);

#if AMIDE_DEBUG  
  if (status == GSL_SUCCESS) 
    g_print("Minimum found after %d iterations\n", iter);
  else if (!continue_work)
    g_print("terminated minization \n");
  else 
    g_print("No minimum found after %d iterations, exited with: %s\n", iter,gsl_strerror(status));
#endif /* AMIDE_DEBUG */

  if (update_func != NULL) /* remove progress bar */
    continue_work = (*update_func)(update_data, NULL, (gdouble) 2.0); 

  /* add the different coefficients to the tree */
  dim.t = 1;
  for (f=0; f<p.num_factors; f++) {
    new_ds = amitk_data_set_new_with_data(AMITK_FORMAT_FLOAT, dim, AMITK_SCALING_TYPE_0D);
    if (new_ds == NULL) {
      g_warning("failed to allocate new_ds");
      goto ending;
    }

    i=p.coef_offset; /* what to skip in v to get to the coefficients */
    for (i_voxel.z=0; i_voxel.z<p.dim.z; i_voxel.z++) 
      for (i_voxel.y=0; i_voxel.y<p.dim.y; i_voxel.y++) 
	for (i_voxel.x=0; i_voxel.x<p.dim.x; i_voxel.x++, i+=p.num_factors) 
	  AMITK_DATA_SET_FLOAT_0D_SCALING_SET_CONTENT(new_ds, i_voxel, 
						      gsl_vector_get(multmin_minimizer->x, i+f));
    //						      gsl_vector_get(initial, i+f));


    temp_string = g_strdup_printf("factor %d", f+1);
    amitk_object_set_name(AMITK_OBJECT(new_ds),temp_string);
    g_free(temp_string);
    amitk_space_copy_in_place(AMITK_SPACE(new_ds), AMITK_SPACE(p.data_set));
    amitk_data_set_calc_max_min(new_ds, NULL, NULL);
    amitk_data_set_set_voxel_size(new_ds, AMITK_DATA_SET_VOXEL_SIZE(p.data_set));
    amitk_data_set_set_modality(new_ds, AMITK_DATA_SET_MODALITY(p.data_set));
    amitk_data_set_calc_far_corner(new_ds);
    amitk_data_set_set_threshold_max(new_ds, 0, AMITK_DATA_SET_GLOBAL_MAX(new_ds));
    amitk_data_set_set_threshold_min(new_ds, 0, AMITK_DATA_SET_GLOBAL_MIN(new_ds));
    amitk_data_set_set_interpolation(new_ds, AMITK_DATA_SET_INTERPOLATION(p.data_set));
    amitk_object_add_child(AMITK_OBJECT(p.data_set), AMITK_OBJECT(new_ds));
    g_object_unref(new_ds); /* add_child adds a reference */
  }



  /* and writeout the factor file */
  if ((file_pointer = fopen(output_filename, "w")) == NULL) {
    g_warning("couldn't open: %s for writing fads analyses", output_filename);
    goto ending;
  }

  time(&current_time);
  fprintf(file_pointer, "# %s: FADS Analysis File for %s\n",PACKAGE, AMITK_OBJECT_NAME(data_set));
  fprintf(file_pointer, "# generated on %s", ctime(&current_time));
  fprintf(file_pointer, "# using %s\n",fads_type_name[FADS_TYPE_PLS]);
  fprintf(file_pointer, "#\n");
  
  if (status == GSL_SUCCESS)
     fprintf(file_pointer, "# found minimal after %d iterations\n", iter);
  else if (!continue_work)
    fprintf(file_pointer, "# user terminated minization after %d iterations.\n",  iter);
  else {
    fprintf(file_pointer, "# No minimum after %d iterations, exited with:\n", iter);
    fprintf(file_pointer, "#    %s\n", gsl_strerror(status));
  }
  fprintf(file_pointer, "#\n");

  fprintf(file_pointer, "# frame\ttime midpt (s)\tfactor:\n");
  fprintf(file_pointer, "#\t");
  for (f=0; f<p.num_factors; f++)
    fprintf(file_pointer, "\t\t%d", f+1);
  fprintf(file_pointer, "\n");

  for (j=0; j<p.num_frames; j++) {
    time_midpoint = amitk_data_set_get_midpt_time(p.data_set, j);

    fprintf(file_pointer, "  %d", j);
    fprintf(file_pointer, "\t%g\t", time_midpoint);
    for (f=0; f<p.num_factors; f++) {
      fprintf(file_pointer, "\t%g", gsl_vector_get(multmin_minimizer->x, f*p.num_frames+j));
      fprintf(file_pointer, "\t%g", gsl_vector_get(initial, f*p.num_frames+j));

      // start remove
      //      fprintf(file_pointer, "\t%g", facts[f*p.num_frames+j] );
      // end remove
    }
    fprintf(file_pointer,"\n");
  }


 ending:

  if (file_pointer != NULL) {
    fclose(file_pointer);
    file_pointer = NULL;
  }

  if (multmin_minimizer != NULL) {
    gsl_multimin_fdfminimizer_free (multmin_minimizer) ;
    multmin_minimizer = NULL;
  }

  if (initial != NULL) {
    gsl_vector_free(initial);
    initial=NULL;
  }

  if (p.weight != NULL) {
    g_free(p.weight);
    p.weight = NULL;
  }

  if (p.forward_error != NULL) {
    g_free(p.forward_error);
    p.forward_error = NULL;
  }

  if (p.coef_total != NULL) {
    g_free(p.coef_total);
    p.coef_total = NULL;
  }

  if (p.coef_dot_prod != NULL) {
    g_free(p.coef_dot_prod);
    p.coef_dot_prod = NULL;
  }

  if (p.coef_norm_factor != NULL) {
    g_free(p.coef_norm_factor);
    p.coef_norm_factor = NULL;
  }

};












#define k12 0.0015




typedef enum {
  TWO_COMP_K21,
  //  TWO_COMP_K12,
  TWO_COMP_BLOOD_FRACTION,
  //  TWO_COMP_T_OFFSET,
  TWO_COMP_NUM_FACTORS
} two_compartment_factors;

gchar * two_compartment_factor_name[] = {
  "k21",
  //  "k12",
  "blood fraction",
  //  "T"
};

typedef struct two_comp_params_t {
  AmitkDataSet * data_set;
  AmitkVoxel dim;
  gdouble lm; /* lagrange multiplier */
  gdouble c;
  gint num_voxels; /* voxels/frame */
  gint num_frames; 
  gint k21_offset; /* num_frames */
  //  gint k12_offset; /* k21_offset + num_voxels */
  gint bf_offset; /* k12_offset+num_voxels */
  //  gint T_offset; /* bf_offset+num_voxels */
  gint num_variables; /* num_frames+(number factors)*num_voxels */

  gdouble * tc_unscaled; /* tc_unscaled*blood_fraction*k21(i)/k12(i) would be our estimate for compartment 2 (tissue component) */
  gdouble * forward_error; /* our estimated data (the forward problem), subtracted by the actual data */
  gdouble * weight; /* the appropriate weight (frame dependent) */

  gint num_blood_curve_constraints;
  gint * blood_curve_constraint_frame;
  gdouble * blood_curve_constraint_val;

  gdouble blood;
  gdouble ls;
  gdouble neg;
} two_comp_params_t;




static void two_comp_calc_compartment(two_comp_params_t * p, const gsl_vector *v) {

  gint i,j, k;
  gdouble convolution_value, kernel;
  gdouble bc; //, k12;
  amide_time_t k_start, k_end, j_end;

  for (i=0; i<p->num_voxels; i++) {
    //    k12 = gsl_vector_get(v, i+p->k12_offset);
    for (j=0; j<p->num_frames; j++) {
      
      convolution_value = 0;
      j_end = amitk_data_set_get_end_time(p->data_set, j);
      for (k=0; k<=j; k++) {
	k_start = amitk_data_set_get_start_time(p->data_set, k);
	k_end = amitk_data_set_get_end_time(p->data_set, k);

	kernel = exp(-k12*(j_end-k_end))-exp(-k12*(j_end-k_start));
	bc = gsl_vector_get(v, k); /* blood curve value */

	convolution_value += bc*kernel;
      }

      p->tc_unscaled[j*p->num_voxels+i] = convolution_value;
    }
  }


  return;
}

static void two_comp_calc_forward_error(two_comp_params_t * p, const gsl_vector *v) {

  gint i;
  AmitkVoxel i_voxel;
  gdouble bc, bf;
  gdouble k21; //, k12;


  for (i_voxel.t=0; i_voxel.t<p->dim.t; i_voxel.t++) {
    i=0;
    bc = gsl_vector_get(v, i_voxel.t);
    for (i_voxel.z=0; i_voxel.z<p->dim.z; i_voxel.z++) {
      for (i_voxel.y=0; i_voxel.y<p->dim.y; i_voxel.y++) {
	for (i_voxel.x=0; i_voxel.x<p->dim.x; i_voxel.x++, i++) {
	  k21 = gsl_vector_get(v, i+p->k21_offset);
	  //	  k12 = gsl_vector_get(v, i+p->k12_offset);
	  bf = gsl_vector_get(v, i+p->bf_offset);
	  p->forward_error[i+i_voxel.t*p->num_voxels] = 
	    bf*(bc+k21*p->tc_unscaled[i+i_voxel.t*p->num_voxels]/k12) - 
	    amitk_data_set_get_value(p->data_set,i_voxel);
	}
      }
    }
  }

  return;
}


static gdouble two_comp_calc_function(two_comp_params_t * p, const gsl_vector *v) {

  gdouble ls_answer=0.0;
  gdouble neg_answer=0.0;
  gdouble blood_answer=0.0;
  gdouble temp;
  gint i, j;

  /* the Least Squares objective */
  ls_answer = 0.0;
  for (i=0; i<p->num_voxels; i++) {
    for (j=0; j<p->num_frames; j++) {
      temp = p->forward_error[i+j*p->num_voxels];
      ls_answer += p->weight[j]*temp*temp;
    }
  }

  /* the non-negativity objective */
  neg_answer = 0.0;
  for (i= 0; i < p->num_variables; i++) {
    temp = gsl_vector_get(v, i);
    if (temp < 0.0)
      neg_answer += temp*temp;
  }

  /* also include a constraint so that blood fractions are <= 1.0 */
  for (i= p->bf_offset; i < p->bf_offset+p->num_voxels; i++) {
    temp = gsl_vector_get(v, i);
    if (temp > 1.0) {
      temp -= 1.0;
      neg_answer += temp*temp;
    }
  }
  neg_answer *= p->lm;

  /* blood curve constraints */
  blood_answer = 0;
  for (i=0; i<p->num_blood_curve_constraints; i++) {
    temp = gsl_vector_get(v, p->blood_curve_constraint_frame[i]);
    temp -= p->blood_curve_constraint_val[i];
    blood_answer += temp*temp;
  }
  blood_answer *= p->c;

  /* keep track of the three */
  p->ls = ls_answer;
  p->neg = neg_answer;
  p->blood = blood_answer;

  return ls_answer+neg_answer+blood_answer;
}

static void two_comp_calc_derivative(two_comp_params_t * p, const gsl_vector *v, gsl_vector *df) {

  gdouble ls_answer;
  gdouble neg_answer;
  gdouble blood_answer;
  gint i, j, k;
  gdouble k21,  bc, bf, error, inner, kernel, tc_unscaled; //k12,
  amide_time_t k_start, k_end, j_end, j_start;


  /* partial derivative of f wrt to the blood curve */
  for (j=0; j<p->num_frames; j++) {

    bc = gsl_vector_get(v, j);
    j_end = amitk_data_set_get_end_time(p->data_set, j);
    j_start = amitk_data_set_get_start_time(p->data_set, j);

    ls_answer = 0;
    for (i=0; i<p->num_voxels; i++) {
      error = p->forward_error[i+j*p->num_voxels];
      k21 = gsl_vector_get(v, i+p->k21_offset);
      //      k12 = gsl_vector_get(v, i+p->k12_offset);
      bf = gsl_vector_get(v, i+p->bf_offset);

      inner = (1-exp(-k12*(j_end-j_start)));
      ls_answer += error*bf*(1+k21*inner/k12);
    }
    ls_answer *= 2*p->weight[j];

    /* the non-negativity objective */
    if (bc < 0.0) neg_answer = p->lm*2*bc;
    else neg_answer = 0;

    /* blood curve constraints */
    blood_answer = 0;
    for (i=0; i<p->num_blood_curve_constraints; i++) 
      if (p->blood_curve_constraint_frame[i] == j) 
	blood_answer += p->c*2*(bc-p->blood_curve_constraint_val[i]);

    if (j==0)
      g_print("bc  %f %f -  %f\n",ls_answer, neg_answer, bc);

    gsl_vector_set(df, j, ls_answer+neg_answer+blood_answer);
  }


  /* partial derivative of f wrt to k21 */
  for (i=0; i<p->num_voxels; i++) {

    k21 = gsl_vector_get(v, i+p->k21_offset);
    //    k12 = gsl_vector_get(v, i+p->k12_offset);
    bf = gsl_vector_get(v, i+p->bf_offset);

    ls_answer = 0;
    for (j=0; j<p->num_frames; j++) {
      ls_answer += 
	p->weight[j]*
	p->forward_error[i+j*p->num_voxels] *
	p->tc_unscaled[i+j*p->num_voxels];
    }
    ls_answer*=2*(bf/k12);

    /* the non-negativity objective */
    if (k21 < 0.0) neg_answer = p->lm*2*k21;
    else neg_answer = 0;

    if (i==0)
      g_print("k21 %f %f\n",ls_answer, neg_answer);

    gsl_vector_set(df, p->k21_offset+i, ls_answer+neg_answer);
  }


#if 0
  /* partial derivative of f wrt to k12 */
  for (i=0; i<p->num_voxels; i++) {
    
    k21 = gsl_vector_get(v, i+p->k21_offset);
    k12 = gsl_vector_get(v, i+p->k12_offset);
    bf = gsl_vector_get(v, i+p->bf_offset);
    
    ls_answer = 0;
    for (j=0; j<p->num_frames; j++) {
      
      error = p->forward_error[i+j*p->num_voxels];
      tc_unscaled = p->tc_unscaled[i+j*p->num_voxels];
      j_end = amitk_data_set_get_end_time(p->data_set, j);
      
      inner = 0;
      for(k=0; k<=j; k++) {
  	bc = gsl_vector_get(v, k);
  	k_start = amitk_data_set_get_start_time(p->data_set, k);
  	k_end = amitk_data_set_get_end_time(p->data_set, k);
  	kernel = 
  	  (j_end-k_start)*exp(-k12*(j_end-k_start)) - 
  	  (j_end-k_end)*exp(-k12*(j_end-k_end));
  	inner += bc*kernel;
      }
      

      ls_answer += p->weight[j]*error*(-tc_unscaled/k12 + inner);
    }
    ls_answer*=(2*bf*k21/k12);
    
    
    /* the non-negativity objective */
    if (k12 < 0.0)
      neg_answer = p->lm*2*k12;
    else 
      neg_answer = 0;

    if (i==0)
      g_print("k12 %f %f\n",ls_answer, neg_answer);

    gsl_vector_set(df, p->k12_offset+i, ls_answer+neg_answer);
  }
#endif

  /* partial derivative of f wrt to the blood fraction */
  for (i=0; i<p->num_voxels; i++) {

    k21 = gsl_vector_get(v, i+p->k21_offset);
    //    k12 = gsl_vector_get(v, i+p->k12_offset);
    bf = gsl_vector_get(v, i+p->bf_offset);

    ls_answer = 0;
    for (j=0; j<p->num_frames; j++) {

      error = p->forward_error[i+j*p->num_voxels];
      bc = gsl_vector_get(v, j);

      tc_unscaled = p->tc_unscaled[i+j*p->num_voxels];

      ls_answer += p->weight[j]*error*(bc+k21*tc_unscaled/k12);
    }
    ls_answer*=2;


    /* the non-negativity and <= 1.0 objectives */
    if (bf < 0.0) neg_answer = p->lm*2*bf;
    else if (bf > 1.0) neg_answer = p->lm*2*(bf-1.0);
    else neg_answer = 0;

    if (i==0)
      g_print("bf  %f %f\n",ls_answer, neg_answer);

    gsl_vector_set(df, p->bf_offset+i, ls_answer+neg_answer);
  }

  return;
}

/* calculate the two compartment objective function */
static double two_comp_f (const gsl_vector *v, void *params) {

  two_comp_params_t * p = params;

  two_comp_calc_compartment(p,v);
  two_comp_calc_forward_error(p, v);

  return two_comp_calc_function(p, v);
}




/* The gradient of f, df = (df/dQt, df/dKij, df/dTi). */
static void two_comp_df (const gsl_vector *v, void *params, gsl_vector *df) {
  
  two_comp_params_t * p = params;

  two_comp_calc_compartment(p,v);
  two_comp_calc_forward_error(p, v);

  two_comp_calc_derivative(p, v, df);
  return;
}



/* Compute both f and df together. */
static void two_comp_fdf (const gsl_vector *v, void *params,  double *f, gsl_vector *df) {

  two_comp_params_t * p = params;

  two_comp_calc_compartment(p,v);
  two_comp_calc_forward_error(p, v);

  *f = two_comp_calc_function(p,v);
  two_comp_calc_derivative(p, v, df);
  return;
}




/* run a two compartment algorithm for factor analysis

   gsl supports minimizing on only a single vector space, so that
   vector is setup as follows
   
   m = num_voxels;
   t = num_frames
   bf = blood fraction

   x = [q1(1)          q1(2)          ...... q1(t)
        k21(1)         k21(2)         ...... k21(m)
	k12(1)         k12(2)         ...... k12(m)
	bf(1)          bf(2)          ...... bf(m)]

	T will come later
	T(1)           T(2)           ...... T(m)]


   forward_error is [t*m]
   tc_unscaled is [t*m] -  tissue component 

*/

#if 0
void fads_two_comp(AmitkDataSet * data_set, 
		   gint max_iterations,
		   gdouble stopping_criteria,
		   gchar * output_filename,
		   gint num_blood_curve_constraints,
		   gint * blood_curve_constraint_frame,
		   gdouble * blood_curve_constraint_val,
		   gboolean (*update_func)(), 
		   gpointer update_data) {

  gsl_multimin_fdfminimizer * multmin_minimizer = NULL;
  gsl_multimin_function_fdf multmin_func;
  AmitkVoxel dim;
  two_comp_params_t p;
  gint iter=0;
  gsl_vector * v=NULL;
  gint i, f, t;
  gint status;
  FILE * file_pointer=NULL;
  time_t current_time;
  gchar * temp_string;
  gboolean continue_work=TRUE;
  gdouble temp;
  amide_time_t time_midpoint;
  gdouble time_constant;
  gdouble time_start;
  AmitkDataSet * new_ds;
  AmitkVoxel i_voxel;
  gdouble magnitude, k21, bf; //k12, 
  

  g_return_if_fail(AMITK_IS_DATA_SET(data_set));
  dim = AMITK_DATA_SET_DIM(data_set);

  /* initialize our parameter structure */
  p.data_set = data_set;
  p.dim = dim;
  p.lm = AMITK_DATA_SET_GLOBAL_MAX(data_set)*100000.0; /* pick a "large" value for the lagrange multiplier */
  p.c = p.lm; 
  p.ls = 1.0;
  p.neg = 1.0;
  p.blood = 1.0;
  p.num_voxels = dim.z*dim.y*dim.x;
  p.num_frames = dim.t;
  p.k21_offset = dim.t+TWO_COMP_K21*p.num_voxels;
  //  p.k12_offset = dim.t+TWO_COMP_K12*p.num_voxels;
  p.bf_offset = dim.t+TWO_COMP_BLOOD_FRACTION*p.num_voxels;
  p.tc_unscaled = NULL;
  p.forward_error = NULL;
  //  p.T_offset = p.blood_frac_offset+p.num_voxels;
  //  p.num_variables = p.T_offset+p.num_voxels;
  p.num_variables = dim.t + TWO_COMP_NUM_FACTORS*p.num_voxels;

  p.num_blood_curve_constraints = num_blood_curve_constraints;
  p.blood_curve_constraint_frame = blood_curve_constraint_frame;
  p.blood_curve_constraint_val = blood_curve_constraint_val;

  /* more sanity checks */
  for (i=0; i<p.num_blood_curve_constraints; i++) {
    g_return_if_fail(p.blood_curve_constraint_frame[i] < p.dim.t);
  }

  p.tc_unscaled = g_try_new(gdouble, p.num_frames*p.num_voxels);
  if (p.tc_unscaled == NULL) {
    g_warning("failed to allocate intermediate data storage for compartment 2");
    goto ending;
  }

  p.forward_error = g_try_new(gdouble, p.num_frames*p.num_voxels);
  if (p.forward_error == NULL) {
    g_warning("failed to allocate intermediate data storage for forward error");
    goto ending;
  }

  /* calculate the weights and magnitude */
  p.weight = calc_weights(p.data_set);
  g_return_if_fail(p.weight != NULL); /* make sure we've malloc'd it */
  magnitude = calc_magnitude(p.data_set, p.weight);

  
  /* set up gsl */
  multmin_func.f = two_comp_f;
  multmin_func.df = two_comp_df;
  multmin_func.fdf = two_comp_fdf;
  multmin_func.n = p.num_variables;
  multmin_func.params = &p;

  //  multmin_minimizer = gsl_multimin_fdfminimizer_alloc (gsl_multimin_fdfminimizer_steepest_descent,
  multmin_minimizer = gsl_multimin_fdfminimizer_alloc (gsl_multimin_fdfminimizer_conjugate_pr,
						       p.num_variables);
  if (multmin_minimizer == NULL) {
    g_warning("failed to allocate multidimensional minimizer");
    goto ending;
  }


  /* Starting point */
  v = gsl_vector_alloc (p.num_variables);
  

  /* initializing q1 with a monoexponential*frame_max,
        use a time constant of 100th of the study length, as a guess...
     initializing the k's with 1
     initializing the spill over fraction with 0.5
     initializing the T's with 0
  */
  time_constant = amitk_data_set_get_end_time(p.data_set, AMITK_DATA_SET_NUM_FRAMES(p.data_set)-1);
  time_constant /= 100.0;

  /* gotta reference from somewhere */
  time_start = amitk_data_set_get_midpt_time(p.data_set, 0);

  for (t=0; t<p.num_frames; t++) {
    time_midpoint = amitk_data_set_get_midpt_time(p.data_set, t);
    temp = exp(-(time_midpoint-time_start)/time_constant); 
    temp *= AMITK_DATA_SET_FRAME_MAX(p.data_set, t);
    gsl_vector_set(v, t, temp);
  }
  for(i=p.k21_offset; i<p.k21_offset+p.num_voxels; i++) gsl_vector_set(v, i, 0.020); /* k21 */
  //  for(i=p.k12_offset; i<p.k12_offset+p.num_voxels; i++) gsl_vector_set(v, i, 0.0015); /* k12 */
  for(i=p.bf_offset; i<p.bf_offset+p.num_voxels; i++) gsl_vector_set(v,i,0.5); /* blood fraction */
  //  for(i=p.T_offset; i<p.T_offset+p.num_voxels; i++) gsl_vector_set(v, i, 0.0);


  gsl_multimin_fdfminimizer_set (multmin_minimizer, &multmin_func, v, 1/6000.0, stopping_criteria);

  if (update_func != NULL) {
    temp_string = g_strdup_printf("Calculating Two Compartment Factor Analysis:\n   %s", AMITK_OBJECT_NAME(data_set));
    continue_work = (*update_func)(update_data, temp_string, (gdouble) 0.0);
    g_free(temp_string);
  }

  do {
      iter++;
      status = gsl_multimin_fdfminimizer_iterate (multmin_minimizer);

      if (!status) 
      	status = gsl_multimin_test_gradient (multmin_minimizer->gradient, stopping_criteria);

      if (update_func != NULL) 
	continue_work = (*update_func)(update_data, NULL, (gdouble) iter/max_iterations);

#if AMIDE_DEBUG  
      g_print("iter %d\t%g=\t%g+%g+%g\n", iter, 
	      100.0*gsl_multimin_fdfminimizer_minimum(multmin_minimizer)/magnitude,
	      100.0*p.ls/magnitude,
	      100.0*p.neg/magnitude,
	      100.0*p.blood/magnitude);
#endif /* AMIDE_DEBUG */

  } while ((status == GSL_CONTINUE) && (iter < max_iterations) && continue_work);

#if AMIDE_DEBUG  
  if (status == GSL_SUCCESS) 
    g_print("Minimum found after %d iterations\n", iter);
  else if (!continue_work)
    g_print("terminated minization \n");
  else 
    g_print("No minimum found after %d iterations, exited with: %s\n", iter,gsl_strerror(status));
#endif /* AMIDE_DEBUG */

  if (update_func != NULL) /* remove progress bar */
    continue_work = (*update_func)(update_data, NULL, (gdouble) 2.0); 

  if ((file_pointer = fopen(output_filename, "w")) == NULL) {
    g_warning("couldn't open: %s for writing fads analyses", output_filename);
    goto ending;
  }

  /* add the different coefficients to the tree */
  dim.t = 1;
  i_voxel.t = 0;
  for (f=0; f<TWO_COMP_NUM_FACTORS; f++) {
    new_ds = amitk_data_set_new_with_data(AMITK_FORMAT_FLOAT, dim, AMITK_SCALING_TYPE_0D);
    if (new_ds == NULL) {
      g_warning("failed to allocate new_ds");
      goto ending;
    }

    i=p.dim.t+f*p.num_voxels; /* what to skip in v to get to the coefficients */
    for (i_voxel.z=0; i_voxel.z<p.dim.z; i_voxel.z++) 
      for (i_voxel.y=0; i_voxel.y<p.dim.y; i_voxel.y++) 
	for (i_voxel.x=0; i_voxel.x<p.dim.x; i_voxel.x++, i++)
	  AMITK_DATA_SET_FLOAT_0D_SCALING_SET_CONTENT(new_ds, i_voxel, 
						      gsl_vector_get(multmin_minimizer->x, i));


    temp_string = g_strdup_printf("factor %s", two_compartment_factor_name[f]);
    amitk_object_set_name(AMITK_OBJECT(new_ds),temp_string);
    g_free(temp_string);
    amitk_space_copy_in_place(AMITK_SPACE(new_ds), AMITK_SPACE(p.data_set));
    amitk_data_set_calc_max_min(new_ds, NULL, NULL);
    amitk_data_set_set_voxel_size(new_ds, AMITK_DATA_SET_VOXEL_SIZE(p.data_set));
    amitk_data_set_set_modality(new_ds, AMITK_DATA_SET_MODALITY(p.data_set));
    amitk_data_set_calc_far_corner(new_ds);
    amitk_data_set_set_threshold_max(new_ds, 0, AMITK_DATA_SET_GLOBAL_MAX(new_ds));
    amitk_data_set_set_threshold_min(new_ds, 0, AMITK_DATA_SET_GLOBAL_MIN(new_ds));
    amitk_data_set_set_interpolation(new_ds, AMITK_DATA_SET_INTERPOLATION(p.data_set));
    amitk_object_add_child(AMITK_OBJECT(p.data_set), AMITK_OBJECT(new_ds));
    g_object_unref(new_ds); /* add_child adds a reference */
  }



  /* and writeout the factor file */
  time(&current_time);
  fprintf(file_pointer, "# %s: FADS Analysis File for %s\n",PACKAGE, AMITK_OBJECT_NAME(data_set));
  fprintf(file_pointer, "# generated on %s", ctime(&current_time));
  fprintf(file_pointer, "# using %s\n",fads_type_name[FADS_TYPE_TWO_COMPARTMENT]);
  fprintf(file_pointer, "#\n");
  
  if (status == GSL_SUCCESS)
     fprintf(file_pointer, "# found minimal after %d iterations\n", iter);
  else if (!continue_work)
    fprintf(file_pointer, "# user terminated minization after %d iterations.\n",  iter);
  else {
    fprintf(file_pointer, "# No minimum after %d iterations, exited with:\n", iter);
    fprintf(file_pointer, "#    %s\n", gsl_strerror(status));
  }
  fprintf(file_pointer, "#\n");

  fprintf(file_pointer, "# frame\ttime midpt (s)\tblood curve value\tinitial curve value:\n");
  fprintf(file_pointer, "\n");

  for (t=0; t<p.num_frames; t++) {
    time_midpoint = amitk_data_set_get_midpt_time(p.data_set, t); 

    fprintf(file_pointer, "  %d", t);
    fprintf(file_pointer, "\t%g\t", time_midpoint);
    // fprintf(file_pointer, "\t%g\n", gsl_vector_get(multmin_minimizer->x, t));
    fprintf(file_pointer, "\t%g\t", gsl_vector_get(multmin_minimizer->x, t));
    temp = exp(-(time_midpoint-time_start)/time_constant); 
    temp *= AMITK_DATA_SET_FRAME_MAX(p.data_set, t);
    fprintf(file_pointer, "\t%g\t", temp);

    for (i=p.dim.x*p.dim.y/2; i<p.dim.x*p.dim.y/2+p.dim.y; i++) {
      k21 = gsl_vector_get(multmin_minimizer->x, i+p.k21_offset);
      //      k12 = gsl_vector_get(multmin_minimizer->x, i+p.k12_offset);
      bf = gsl_vector_get(multmin_minimizer->x, i+p.bf_offset);
      fprintf(file_pointer, "\t%g\t", bf*k21*p.tc_unscaled[i+t*p.num_voxels]/k12);
    }
    fprintf(file_pointer, "\n");
    
  }


 ending:

  if (file_pointer != NULL) {
    fclose(file_pointer);
    file_pointer = NULL;
  }

  if (multmin_minimizer != NULL) {
    gsl_multimin_fdfminimizer_free (multmin_minimizer) ;
    multmin_minimizer = NULL;
  }

  if (v != NULL) {
    gsl_vector_free(v);
    v=NULL;
  }

  if (p.weight != NULL) {
    g_free(p.weight);
    p.weight = NULL;
  }

  if (p.forward_error != NULL) {
    g_free(p.forward_error);
    p.forward_error = NULL;
  }

  if (p.tc_unscaled != NULL) {
    g_free(p.tc_unscaled);
    p.tc_unscaled = NULL;
  }

};


#endif


#endif /* AMIDE_LIBGSL_SUPPORT */
