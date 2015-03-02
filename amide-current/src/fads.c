/* fads.c
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

#include "amide_config.h"
#ifdef AMIDE_LIBGSL_SUPPORT
#include <time.h>
#include <glib.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_multimin.h>
#include "fads.h"
#include "amitk_data_set_FLOAT_0D_SCALING.h"


#define FINAL_MU 1e-07

gchar * fads_minimizer_algorithm_name[NUM_FADS_MINIMIZERS] = {
  //  N_("Steepest Descent"),
  N_("Fletcher-Reeves conjugate gradient"),
  N_("Polak-Ribiere conjugate gradient"),
  //  N_("vector BFGS conjugate gradient")
};


gchar * fads_type_name[] = {
  N_("Principle Component Analysis"),
  N_("Penalized Least Squares Factor Analysis"),
  N_("2 compartment model")
};

gchar * fads_type_explanation[] = {
  N_("Priniciple Component Analysis based on singular value "
     "decomposition."),

  N_("Principle component analysis with positivity constraints "
     "and a penalized least squares objective, an adaptation "
     "of Sitek, et al., IEEE Trans. Med. Imag., 2002.  If beta "
     "is set to zero, this is normal factor analysis, similar to "
     "Di Paola, et al., IEEE Trans. Nuc. Sci., 1982"),
  
  N_("Standard 2 compartment model")

};

#include "../pixmaps/two_compartment.h"

const guint8 * fads_type_icon[NUM_FADS_TYPES] = {
  NULL,
  NULL,
  two_compartment
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
  m = dim.x*dim.y*dim.z*dim.g;

  if (n == 1) {
    g_warning(_("need dynamic data set in order to perform factor analysis"));
    goto ending;
  }

  /* do all the memory allocations upfront */
  if ((matrix_a = gsl_matrix_alloc(m,n)) == NULL) {
    g_warning(_("Failed to allocate %dx%d array"), m,n);
    goto ending;
  }
  
  if ((matrix_v = gsl_matrix_alloc(n,n)) == NULL) {
    g_warning(_("Failed to allocate %dx%d array"), n,n);
    goto ending;
  }

  if ((vector_s = gsl_vector_alloc(n)) == NULL) {
    g_warning(_("Failed to allocate %d vector"), n);
    goto ending;
  }

  /* fill in the a matrix */
  for (i_voxel.t = 0; i_voxel.t < dim.t; i_voxel.t++) {
    i = 0;
    for (i_voxel.g = 0; i_voxel.g < dim.g; i_voxel.g++) {
      for (i_voxel.z = 0; i_voxel.z < dim.z; i_voxel.z++)
	for (i_voxel.y = 0; i_voxel.y < dim.y; i_voxel.y++)
	  for (i_voxel.x = 0; i_voxel.x < dim.x; i_voxel.x++, i++) {
	    value = amitk_data_set_get_value(data_set, i_voxel);
	    gsl_matrix_set(matrix_a, i, i_voxel.t, value);
	  }
    }
  }

  /* get the singular value decomposition of the correlation matrix -> matrix_a = U*S*Vt
     notes: 
        the function will place the value of U into matrix_a 
        gsl_linalg_SV_decomp_jacobi will return an unsuitable matrix_v, don't use it 
  */
  status = perform_svd(matrix_a, matrix_v, vector_s);
  if (status != 0) g_warning(_("SV decomp returned error: %s"), gsl_strerror(status));

  /* transferring data */
  if (pnum_factors != NULL)
    *pnum_factors = n;

  if (pfactors != NULL) {
    factors = g_try_new(gdouble, n);
    if (factors == NULL) {
      g_warning(_("Failed to allocate %d factor array"), n);
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

static void write_header(FILE * file_pointer, gint status, fads_type_t type, 
			 AmitkDataSet * ds, gint iter) {

  time_t current_time;

  time(&current_time);
  fprintf(file_pointer, "# %s: FADS Analysis File for %s\n",PACKAGE, AMITK_OBJECT_NAME(ds));
  fprintf(file_pointer, "# generated on %s", ctime(&current_time));
  fprintf(file_pointer, "# using %s\n",fads_type_name[type]);
  fprintf(file_pointer, "#\n");
  
  if (iter >= 0) {
    if (status == GSL_SUCCESS)
      fprintf(file_pointer, "# found minimal after %d iterations\n", iter);
    else if (status == GSL_CONTINUE)
      fprintf(file_pointer, "# user terminated minization after %d iterations.\n",  iter);
    else  {
      fprintf(file_pointer, "# No minimum after %d iterations, exited with:\n", iter);
      fprintf(file_pointer, "#    %s\n", gsl_strerror(status));
    }
    fprintf(file_pointer, "#\n");
  }

  return;
}


static gsl_multimin_fdfminimizer * alloc_fdfminimizer(fads_minimizer_algorithm_t minimizer_algorithm,
						      gint num_variables) {

  switch(minimizer_algorithm) {
    //  case FADS_MINIMIZER_STEEPEST_DESCENT:
    //    return gsl_multimin_fdfminimizer_alloc(gsl_multimin_fdfminimizer_steepest_descent,
    //					   num_variables);
    //    break;
  case FADS_MINIMIZER_CONJUGATE_FR:
    return gsl_multimin_fdfminimizer_alloc(gsl_multimin_fdfminimizer_conjugate_fr,
					   num_variables);
    break;
  case FADS_MINIMIZER_CONJUGATE_PR:
    return gsl_multimin_fdfminimizer_alloc(gsl_multimin_fdfminimizer_conjugate_pr,
					   num_variables);
    break;
    //  case FADS_MINIMIZER_VECTOR_BFGS:
    //    return gsl_multimin_fdfminimizer_alloc(gsl_multimin_fdfminimizer_vector_bfgs,
    //					   num_variables);
    //    break;
  default:
    g_error("no such minimizer algorithm %d\n", minimizer_algorithm);
    return NULL;
    break;
  }

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
  num_voxels = dim.x*dim.y*dim.z*dim.g;
  num_frames = dim.t;

  u = gsl_matrix_alloc(num_voxels, num_frames);
  if (u == NULL) {
    g_warning(_("failed to alloc matrix, size %dx%d"), num_voxels, num_frames);
    goto ending;
  }

  if ((v = gsl_matrix_alloc(num_frames,num_frames)) == NULL) {
    g_warning(_("Failed to allocate %dx%d array"), num_frames, num_frames);
    goto ending;
  }

  if ((s = gsl_vector_alloc(num_frames)) == NULL) {
    g_warning(_("Failed to allocate %d vector"), num_frames);
    goto ending;
  }

  /* copy the info into the matrix */
  for (i_voxel.t = 0; i_voxel.t < num_frames; i_voxel.t++) {
    i = 0;
    for (i_voxel.g = 0; i_voxel.g < dim.g; i_voxel.g++)
      for (i_voxel.z = 0; i_voxel.z < dim.z; i_voxel.z++)
	for (i_voxel.y = 0; i_voxel.y < dim.y; i_voxel.y++)
	  for (i_voxel.x = 0; i_voxel.x < dim.x; i_voxel.x++, i++) 
	    gsl_matrix_set(u, i, i_voxel.t, amitk_data_set_get_value(data_set, i_voxel));
  }

  /* do Singular Value decomposition */
  status = perform_svd(u, v, s);
  if (status != 0) g_warning(_("SV decomp returned error: %s"), gsl_strerror(status));

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
      g_warning(_("failed to alloc matrix size %dx%d"), num_voxels, num_factors);
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
      g_warning(_("failed to alloc vector size %d"), num_factors);
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
      g_warning(_("failed to alloc matrix size %dx%d"), num_frames, num_factors);
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
	      AmitkUpdateFunc update_func,
	      gpointer update_data) {

  gsl_matrix * u=NULL;
  gsl_vector * s=NULL;
  gsl_matrix * v=NULL;
  AmitkVoxel dim, i_voxel;
  guint f, i, j;
  AmitkDataSet * new_ds;
  gchar * temp_string;
  FILE * file_pointer=NULL;
  amide_time_t frame_midpoint, frame_duration;
  AmitkViewMode i_view_mode;

  dim = AMITK_DATA_SET_DIM(data_set);
  dim.t = 1;


  /* note, since we're using a gsl function for the bulk of the calculation (in perform_pca), 
     there's no way to update the progress bar as we're going along. We'll still throw up the
     dialog so people will at least know we're thinking */
  if (update_func != NULL) {
    temp_string = g_strdup_printf(_("Calculating Principle Component Analysis on:\n   %s"), 
				  AMITK_OBJECT_NAME(data_set));
    (*update_func)(update_data, temp_string, (gdouble) 0.1); /* can't cancel this */
    g_free(temp_string);
  }

  perform_pca(data_set, num_factors, &u, &s, &v);



  /* copy the data on over */
  for (f=0; f<num_factors; f++) {

    new_ds = amitk_data_set_new_with_data(NULL, AMITK_DATA_SET_MODALITY(data_set),
					  AMITK_FORMAT_FLOAT, dim, AMITK_SCALING_TYPE_0D);
    if (new_ds == NULL) {
      g_warning(_("failed to allocate new_ds"));
      goto ending;
    }
    for (i_view_mode=0; i_view_mode < AMITK_VIEW_MODE_NUM; i_view_mode++)
      amitk_data_set_set_color_table(new_ds, i_view_mode, AMITK_DATA_SET_COLOR_TABLE(data_set, i_view_mode));

    i=0;
    i_voxel.t = 0;
    for (i_voxel.g=0; i_voxel.g<dim.g; i_voxel.g++) 
      for (i_voxel.z=0; i_voxel.z<dim.z; i_voxel.z++) 
	for (i_voxel.y=0; i_voxel.y<dim.y; i_voxel.y++) 
	  for (i_voxel.x=0; i_voxel.x<dim.x; i_voxel.x++, i++) 
	    AMITK_DATA_SET_FLOAT_0D_SCALING_SET_CONTENT(new_ds, i_voxel, 
							gsl_matrix_get(u, i, f));

    temp_string = g_strdup_printf("component %d", f+1);
    amitk_object_set_name(AMITK_OBJECT(new_ds),temp_string);
    g_free(temp_string);
    amitk_space_copy_in_place(AMITK_SPACE(new_ds), AMITK_SPACE(data_set));
    amitk_data_set_set_voxel_size(new_ds, AMITK_DATA_SET_VOXEL_SIZE(data_set));
    amitk_data_set_set_modality(new_ds, AMITK_DATA_SET_MODALITY(data_set));
    amitk_data_set_calc_far_corner(new_ds);
    amitk_data_set_set_threshold_max(new_ds, 0, amitk_data_set_get_global_max(new_ds));
    amitk_data_set_set_threshold_min(new_ds, 0, amitk_data_set_get_global_min(new_ds));
    amitk_data_set_set_interpolation(new_ds, AMITK_DATA_SET_INTERPOLATION(data_set));
    amitk_object_add_child(AMITK_OBJECT(data_set), AMITK_OBJECT(new_ds));
    amitk_object_unref(new_ds); /* add_child adds a reference */
  }

  /* and output the curves */
  if ((file_pointer = fopen(output_filename, "w")) == NULL) {
    g_warning(_("couldn't open: %s for writing PCA analyses"), output_filename);
    goto ending;
  }

  write_header(file_pointer, 0, FADS_TYPE_PCA, data_set, -1);

  fprintf(file_pointer, "# frame\tduration (s)\ttime midpt (s)\tfactor:\n");
  fprintf(file_pointer, "#\t");
  for (f=0; f<num_factors; f++) fprintf(file_pointer, "\t\t%d", f+1);
  fprintf(file_pointer, "\n");

  for (j=0; j<AMITK_DATA_SET_NUM_FRAMES(data_set); j++) {
    frame_midpoint = amitk_data_set_get_midpt_time(data_set, j);
    frame_duration = amitk_data_set_get_frame_duration(data_set, j);

    fprintf(file_pointer, "  %d", j);
    fprintf(file_pointer, "\t%g\t%g\t", frame_duration, frame_midpoint);
    for (f=0; f<num_factors; f++) {
      fprintf(file_pointer, "\t%g", 
	      gsl_vector_get(s, f)* gsl_matrix_get(v, j, f));
    }
    fprintf(file_pointer,"\n");
  }

 ending:

  if (update_func != NULL) /* remove progress bar */
    (*update_func)(update_data, NULL, (gdouble) 2.0); 

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

static gdouble calc_magnitude(AmitkDataSet * ds, gdouble * weight) {

  gdouble magnitude;
  AmitkVoxel i_voxel;
  AmitkVoxel dim;
  amide_data_t data;

  dim = AMITK_DATA_SET_DIM(ds);
  magnitude = 0;

  for (i_voxel.t=0; i_voxel.t<dim.t; i_voxel.t++) 
    for (i_voxel.g=0; i_voxel.g<dim.g; i_voxel.g++) 
      for (i_voxel.z=0; i_voxel.z<dim.z; i_voxel.z++) 
	for (i_voxel.y=0; i_voxel.y<dim.y; i_voxel.y++) 
	  for (i_voxel.x=0; i_voxel.x<dim.x; i_voxel.x++) {
	    data = amitk_data_set_get_value(ds, i_voxel);
	    magnitude += weight[i_voxel.t]*data*data;
	  }


  return sqrt(magnitude);
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
  gdouble mu;
  gdouble b;
  gint num_voxels; /* voxels/frame */
  gint num_frames; 
  gint num_factors;
  gint alpha_offset; /* num_factors*num_frames */
  gint num_variables; /* alpha_offset+num_voxels*num_factors*/

  gdouble * forward_error; /* our estimated data (the forward problem), subtracted by the actual data */
  gdouble * weight; /* the appropriate weight (frame dependent) */
  gdouble * ec_a; /* used for sum alpha == 1.0 */
  gdouble * ec_bc;
  
  /* lagrange multipliers - equality */
  gdouble * lme_a; /* used for sum alpha == 1.0 */
  gdouble * lme_bc;

  /* lagrange multipliers - inequality */
  gdouble * lmi_a; /* 0 <= alpha <= 1.0 */
  gdouble * lmi_f;

  /* variable constraints */
  gboolean sum_factors_equal_one; /* whether to constrain by sum alpha == 1.0 */
  gint num_blood_curve_constraints;
  gint * blood_curve_constraint_frame;
  gdouble * blood_curve_constraint_val;


  gdouble orth;
  gdouble blood;
  gdouble ls;
  gdouble neg;
} pls_params_t;



static void pls_calc_constraints(pls_params_t * p, const gsl_vector *v) {

  gint f, i, k;
  gdouble total, bc;


  /* total alpha == 1 constraints */
  if (p->sum_factors_equal_one) {
    for (k=0, i=p->alpha_offset; k < p->num_voxels; k++, i+=p->num_factors) {
      total = -1.0;
      for (f=0; f<p->num_factors; f++)
	total += gsl_vector_get(v, i+f);
      p->ec_a[k] = total;
    }
  }


  /* blood curve constraints */
  for (i=0; i<p->num_blood_curve_constraints; i++) {
    bc = gsl_vector_get(v, p->blood_curve_constraint_frame[i]);
    p->ec_bc[i] = bc - p->blood_curve_constraint_val[i];
  }

}

static void pls_calc_forward_error(pls_params_t * p, const gsl_vector *v) {

  gint i, k, f;
  AmitkVoxel i_voxel;
  gdouble inner;
  gdouble alpha;
  gdouble factor;

  for (i_voxel.t=0; i_voxel.t<p->dim.t; i_voxel.t++) {
    i=p->alpha_offset; /* what to skip in v to get to the coefficients */
    k=0;
    for (i_voxel.g=0; i_voxel.g<p->dim.g; i_voxel.g++) {
      for (i_voxel.z=0; i_voxel.z<p->dim.z; i_voxel.z++) {
	for (i_voxel.y=0; i_voxel.y<p->dim.y; i_voxel.y++) {
	  for (i_voxel.x=0; i_voxel.x<p->dim.x; i_voxel.x++, i+=p->num_factors, k+=p->num_frames) {
	    inner = 0.0;
	    for (f=0;  f< p->num_factors; f++) {
	      alpha = gsl_vector_get(v, i+f);
	      factor = gsl_vector_get(v, f*p->num_frames+i_voxel.t);
	      inner += alpha*factor;
	    }
	    p->forward_error[k+i_voxel.t] = inner - amitk_data_set_get_value(p->data_set,i_voxel);
	  }
	}
      }
    }
  }

  return;
}


static gdouble pls_calc_function(pls_params_t * p, const gsl_vector *v) {

  gdouble ls_answer=0.0;
  gdouble neg_answer=0.0;
  gdouble orth_answer=0.0;
  gdouble blood_answer=0.0;
  gdouble temp, lambda, factor, alpha;
  gint i, j, f, l;

  /* the Least Squares objective */
  ls_answer = 0.0;
  for (j=0; j<p->num_frames; j++) {
    for (i=0; i<p->num_voxels; i++) {
      temp = p->forward_error[i*p->num_frames+j];
      ls_answer += p->weight[j]*temp*temp;
    }
  }
  p->ls = ls_answer;

  /* the non-negativity constraints */
  neg_answer = 0.0;
  for (f=0; f<p->num_factors; f++) {
    for (j=0; j<p->num_frames; j++) {
      factor = gsl_vector_get(v, f*p->num_frames+j);
      lambda = p->lmi_f[f*p->num_frames+j];
      if ((factor-p->mu*lambda) < 0.0)
	neg_answer += factor*(factor/(2.0*p->mu) - lambda);
      else
	neg_answer -= lambda*lambda*p->mu/2.0;
    }
  }
  
  for (l=0, i=p->alpha_offset; l<p->num_voxels; l++, i+=p->num_factors) {
    for (f=0; f<p->num_factors; f++) {
      alpha = gsl_vector_get(v, i+f);
      lambda = p->lmi_a[l*p->num_factors+f];
      if ((alpha-p->mu*lambda) < 0.0)
	neg_answer += alpha*(alpha/(2.0*p->mu) - lambda);
      else
	neg_answer -= lambda*lambda*p->mu/2.0;
    }
  }

  /* the sum of alpha's == 1 constraint */
  if (p->sum_factors_equal_one) {
    for (i=0; i< p->num_voxels; i++)
      neg_answer += p->ec_a[i]*(p->ec_a[i]/(2.0*p->mu) - p->lme_a[i]);
  }

  p->neg = neg_answer;

  /* the orthogonality objective */
  orth_answer = 0.0;
#if 0
  for (l=0; l<p->num_voxels; l++) 
    if ((p->coef_total[l] < 1.0) && (p->coef_total[l] > 0))
      orth_answer += (2*p->coef_total[l]-p->coef_squared_total[l]-p->coef_total[l]*p->coef_total[l]);
  orth_answer *= 0.5;
  p->orth = orth_answer;
  orth_answer *= 0.5*p->b; /* weight this objective by b */
#endif

  /* blood curve constraints */
  blood_answer = 0;
  for (i=0; i<p->num_blood_curve_constraints; i++) 
    blood_answer += p->ec_bc[i]*(p->ec_bc[i]/(2.0*p->mu) - p->lme_bc[i]);
  p->blood = blood_answer;

  return ls_answer+neg_answer+orth_answer+blood_answer;
}

static void pls_calc_derivative(pls_params_t * p, const gsl_vector *v, gsl_vector *df) {

  gdouble ls_answer=0.0;
  gdouble neg_answer=0.0;
  gdouble orth_answer;
  gdouble blood_answer=0.0;
  gdouble factor, alpha, lambda;
  gint i, j, k, l, q; //f


  /* calculate first for the factor variables */
  for (q= 0; q < p->num_factors; q++) {
    for (j=0; j<p->num_frames; j++) {
      factor = gsl_vector_get(v, q*p->num_frames+j);
      
      /* the Least Squares objective */
      ls_answer = 0.0;
      /* p->coef_offset is what to skip in v to get to the coefficients */
      for (i=p->alpha_offset, k=0; k < p->num_frames*p->num_voxels; i+=p->num_factors, k+=p->num_frames) {
	alpha = gsl_vector_get(v, i+q);
	ls_answer += alpha*p->forward_error[k+j];
      }
      ls_answer *= 2.0*p->weight[j];

      /* the non-negativity objective */
      lambda = p->lmi_f[q*p->num_frames+j];
      if ((factor - p->mu*lambda) < 0.0)
	neg_answer = factor/p->mu-lambda;
      else
	neg_answer = 0;

      /* blood curve constraints */
      blood_answer = 0;
      if (q == 0)  /* 1st factor is blood curve */
	for (i=0; i<p->num_blood_curve_constraints; i++) 
	  if (p->blood_curve_constraint_frame[i] == j) 
	    blood_answer += p->ec_bc[i]/p->mu - p->lme_bc[i];

      gsl_vector_set(df, q*p->num_frames+j, ls_answer+neg_answer+blood_answer);
    }
  }

  /* now calculate for the coefficient variables */
  for (q= 0; q < p->num_factors; q++) {

    for (i=p->alpha_offset, k=0, l=0; i < p->num_variables; i+=p->num_factors, k+=p->num_frames, l++) {
      alpha = gsl_vector_get(v, i+q);

      /* the Least Squares objective */
      ls_answer = 0;
      for (j=0; j<p->num_frames; j++) {
	factor = gsl_vector_get(v, q*p->num_frames+j);
	ls_answer += p->weight[j]*p->forward_error[k+j]*factor;
      }
      ls_answer *= 2.0;

      /* the non-negativity and <= 1 objective */
      lambda = p->lmi_a[l*p->num_factors+q];
      if ((alpha-p->mu*lambda) < 0.0)
	neg_answer = alpha/p->mu-lambda;
      else
	neg_answer = 0;

      /* the sum of alpha's == 1 constraint */
      if (p->sum_factors_equal_one) {
	neg_answer += p->ec_a[l]/p->mu - p->lme_a[l];
      }
	
      /* the orthogonality objective */
#if 0
      if ((p->coef_total[l] < 1.0) && (p->coef_total[l] > 0))
	orth_answer = p->b*(1.0-alpha-p->coef_total[l]);
      else
#endif
	orth_answer = 0;
      
      gsl_vector_set(df, i+q, ls_answer+neg_answer+orth_answer);
    }
  }

  return;
}

/* calculate the penalized least squares objective function */
static double pls_f (const gsl_vector *v, void *params) {

  pls_params_t * p = params;

  pls_calc_constraints(p,v);
  pls_calc_forward_error(p, v);

  return pls_calc_function(p, v);
}




/* The gradient of f, df = (df/dCip, df/dFpt). */
static void pls_df (const gsl_vector *v, void *params, gsl_vector *df) {
  
  pls_params_t * p = params;

  pls_calc_constraints(p,v);
  pls_calc_forward_error(p, v);

  pls_calc_derivative(p, v, df);

  return;
}



/* Compute both f and df together. */
static void pls_fdf (const gsl_vector *v, void *params,  double *f, gsl_vector *df) {

  pls_params_t * p = params;

  pls_calc_constraints(p,v);
  pls_calc_forward_error(p, v);

  *f = pls_calc_function(p,v);
  pls_calc_derivative(p, v, df);
  
  return;
}




/* run the penalized least squares algorithm for factor analysis.

   -this method is described in:
   Sitek, et al., IEEE Trans Med Imaging, 21, 2002, pages 216-225 

   differences in my version:
   1-instead of reweighting the coefficents at the end of the fitting,
   the coefficients are penalized for being greater then 1, and their total
   for each voxel has to equal 1.  
   2-weights the least square terms by their respective frame duration
   
   Constrained optimization is done by using an augmented lagrangian method, 
   see Chapter 17.4 of "Numerical Optimization" - Nocedal & Wright.

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
	factor(F,1  factor(F,2) ...... factor(F,N)
	alpha(1,1)  alpha(1,2)  ...... alpha(1,F)
	alpha(2,1)  alpha(2,2)  ...... alpha(2,F)
	   ..          ..                 ..  
	alpha(M,1)  alpha(M,2)  ...... alpha(M,F)]

  
   forward_error is [M*N]



*/

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
	      gpointer update_data) {

  gsl_multimin_fdfminimizer * multimin_minimizer = NULL;
  gsl_multimin_function_fdf multimin_func;
  AmitkVoxel dim;
  pls_params_t p;
  gint inner_iter=0;
  gint outer_iter=0;
  gsl_vector * initial=NULL;
  gint i, f, j;
  gint status;
  FILE * file_pointer=NULL;
  gchar * temp_string;
  gboolean continue_work=TRUE;
  gdouble alpha, factor;
  amide_time_t frame_midpoint, frame_duration;
  gdouble init_value;
  gdouble magnitude;
  AmitkDataSet * new_ds;
  AmitkVoxel i_voxel;
  gdouble current_beta=0.0;
  AmitkViewMode i_view_mode;
  GTimer * timer=NULL;
  gboolean new_outer;
#if AMIDE_DEBUG
  div_t x;
#endif

  g_return_if_fail(AMITK_IS_DATA_SET(data_set));
  dim = AMITK_DATA_SET_DIM(data_set);
  g_return_if_fail(num_factors <= dim.t);

  /* initialize our parameter structure */
  p.data_set = data_set;
  p.dim = dim;
  p.mu = 1000;
  p.b = 0.0;
  p.ls = 0.0;
  p.neg = 0.0;
  p.orth = 0.0;
  p.blood = 0.0;
  p.num_voxels = dim.g*dim.z*dim.y*dim.x;
  p.num_frames = dim.t;
  p.num_factors = num_factors;
  p.alpha_offset = p.num_factors*p.num_frames;
  p.num_variables = p.alpha_offset+p.num_factors*p.num_voxels;

  p.sum_factors_equal_one = sum_factors_equal_one;
  p.num_blood_curve_constraints = num_blood_curve_constraints;
  p.blood_curve_constraint_frame = blood_curve_constraint_frame;
  p.blood_curve_constraint_val = blood_curve_constraint_val;
  p.forward_error = NULL;
  p.weight = NULL;
  p.ec_a = NULL;
  p.ec_bc = NULL;
  p.lme_a = NULL;
  p.lme_bc = NULL;
  p.lmi_a = NULL;
  p.lmi_f = NULL;

  /* more sanity checks */
  for (i=0; i<p.num_blood_curve_constraints; i++) {
    g_return_if_fail(p.blood_curve_constraint_frame[i] < p.dim.t);
  }

  if (initial_curves != NULL) {
    if (initial_curves->len < p.num_frames*p.num_factors) {
      g_warning(_("Supplied initial curves have %d elements, need %d x %d = %d elements"), 
		initial_curves->len, p.num_frames, p.num_factors, p.num_frames*p.num_factors);
      goto ending;
    }
  }

  p.forward_error = g_try_new(gdouble, p.num_frames*p.num_voxels);
  if (p.forward_error == NULL) {
    g_warning(_("failed forward error malloc"));
    goto ending;
  }

  /* calculate the weights and magnitude */
  p.weight = calc_weights(p.data_set);
  if (p.weight == NULL) {
    g_warning(_("failed weight malloc"));
    goto ending;
  }
  magnitude = calc_magnitude(p.data_set, p.weight);

  if (p.sum_factors_equal_one) {
    p.ec_a = g_try_new(gdouble, p.num_voxels);
    if (p.ec_a == NULL) {
      g_warning(_("failed equality constraint alpha malloc"));
      goto ending;
    }
  }

  p.ec_bc = g_try_new(gdouble, p.num_blood_curve_constraints);
  if ((p.ec_bc == NULL) && (p.num_blood_curve_constraints > 0)) {
    g_warning(_("failed equality constraint blood curve malloc"));
    goto ending;
  }

  if (p.sum_factors_equal_one) {
    p.lme_a = g_try_new(gdouble, p.num_voxels);
    if (p.lme_a == NULL) {
      g_warning(_("failed malloc for equality lagrange multiplier - alpha"));
      goto ending;
    }
    for (i=0; i<p.num_voxels; i++)
      p.lme_a[i] = 0.0;
  }

  p.lme_bc = g_try_new(gdouble, p.num_blood_curve_constraints);
  if ((p.lme_bc == NULL) && (p.num_blood_curve_constraints > 0)) {
    g_warning(_("failed malloc for equality lagrange multiplier - blood curve"));
    goto ending;
  }
  for (i=0; i<p.num_blood_curve_constraints; i++)
    p.lme_bc[i] = 0.0;

  p.lmi_a = g_try_new(gdouble, p.num_voxels*p.num_factors);
  if (p.lmi_a == NULL) {
    g_warning(_("failed malloc for inequality lagrange multiplier - alpha"));
    goto ending;
  }
  for (i=0; i<p.num_voxels; i++)
    for (f=0; f<p.num_factors; f++)
      p.lmi_a[i*p.num_factors+f] = 0.0;

  p.lmi_f = g_try_new(gdouble, p.num_frames*p.num_factors);
  if (p.lmi_f == NULL) {
    g_warning(_("failed malloc for inequality lagrange multiplier - factors"));
    goto ending;
  }
  for (f=0; f<p.num_factors; f++)
    for (j=0; j<p.num_frames; j++)
      p.lmi_f[f*p.num_frames+j] = 0.0;
  

  /* set up gsl */
  multimin_func.f = pls_f;
  multimin_func.df = pls_df;
  multimin_func.fdf = pls_fdf;
  multimin_func.n = p.num_variables;
  multimin_func.params = &p;

  multimin_minimizer = alloc_fdfminimizer(minimizer_algorithm, p.num_variables);
  if (multimin_minimizer == NULL) {
    g_warning(_("failed to allocate multidimensional minimizer"));
    goto ending;
  }


  /* Starting point */
  initial = gsl_vector_alloc (p.num_variables);
  
    /* we've been passed a GArray with the initial curves */
  if (initial_curves != NULL) {

    for (f=0; f < p.num_factors; f++) {
      for (j=0; j < p.num_frames; j++) {
	gsl_vector_set(initial, f*p.num_frames+j,
		       g_array_index(initial_curves, gdouble, 
				     j*p.num_factors + f));
      }
    }

  } else {
    /* if not given initial curves, try to come up with something reasonable as the initial condition */
    gdouble time_constant;
    gdouble time_start;
    gsl_vector * s;
    gsl_matrix * u;
    gsl_matrix * v;
    gdouble temp1, temp2, mult;


    /* setting the factors to the principle components */
    perform_pca(p.data_set, p.num_factors, &u, &s, &v);
    
    /* need to initialize the factors, picking some quasi-exponential curves */
    /* use a time constant of 100th of the study length, as a guess */
    time_constant = amitk_data_set_get_end_time(p.data_set, AMITK_DATA_SET_NUM_FRAMES(p.data_set)-1);
    time_constant /= 100.0;

    /* gotta reference from somewhere */
    time_start = amitk_data_set_get_midpt_time(p.data_set, 0); 

#if 0
    time_constant = 1.0;
    for (f=0; f<p.num_factors; f++) {
      if (f & 0x1) /* odd */
	for (j=0; j<p.num_frames; j++) 
	  gsl_vector_set(initial, f*p.num_frames+j, (j/((gdouble)p.num_frames*time_constant)));
      else
	for (j=0; j<p.num_frames; j++) 
	  gsl_vector_set(initial, f*p.num_frames+j, ((gdouble) p.num_frames-j-1)/((gdouble)p.num_frames*time_constant));
      if (f & 0x1) /* double every other iteration */
	time_constant *=2.0;
    }
    
    for (j=0; j<p.num_frames; j++) {
      gsl_vector_set(initial, j, (j/((gdouble)p.num_frames)));
      gsl_vector_set(initial, p.num_frames+j, ((gdouble) p.num_frames-j-1)/((gdouble)p.num_frames));
    }
#endif

    for (f=0; f<p.num_factors; f+=2) {
      for (j=0; j<p.num_frames; j++) {
	frame_midpoint = amitk_data_set_get_midpt_time(p.data_set, j); 
	
	mult = exp(-(frame_midpoint-time_start)/time_constant); 
	
	temp1 = fabs(gsl_vector_get(s, f)*gsl_matrix_get(v, j, f));
	if (f+1 < p.num_factors) {
	  temp2 = gsl_vector_get(s, f+1)*fabs(gsl_matrix_get(v, j, f+1));
	  gsl_vector_set(initial, f*p.num_frames+j, 
			 temp1*mult+temp2*(1-mult));
	  gsl_vector_set(initial, (f+1)*p.num_frames+j,
			 temp1*(1-mult)+temp2*mult);
	} else {
	  gsl_vector_set(initial, f*p.num_frames+j,temp1*mult);
	}
      }
      if (f & 0x1) /* double every other iteration */
	time_constant *=2.0;
    }
    gsl_vector_free(s);
    gsl_matrix_free(u);
    gsl_matrix_free(v);

  
  }


#if AMIDE_DEBUG
  g_print("frame\tinitial curves\n");
  for (j=0; j<p.num_frames; j++) {
    g_print("%d",j);
    for (f=0; f<p.num_factors; f++) 
      g_print("\t%5.3f",gsl_vector_get(initial,f*p.num_frames+j));
    g_print("\n");
  }
#endif
  

  /* and just set the coefficients to something logical */
  init_value = 1.0/num_factors;
  for (f=0; f<p.num_factors; f++) 
    for (i=p.alpha_offset; i<p.num_variables; i+=p.num_factors)
      gsl_vector_set(initial, i+f, init_value);

  if (update_func != NULL) {
    timer = g_timer_new();
    temp_string = g_strdup_printf(_("Calculating Penalized Least Squares Factor Analysis:\n   %s"), 
				  AMITK_OBJECT_NAME(data_set));
    continue_work = (*update_func)(update_data, temp_string, (gdouble) 0.0);
    g_free(temp_string);
  }

#if AMIDE_DEBUG
  if (timer != NULL)
    timer = g_timer_new();

  g_print("iteration\t%10s = %5s + %5s + %5s + %5s\tmu\tbeta\n", "total err", "lsq", "noneg", "ortho", "blood");
#endif


  do { /* outer loop */
    outer_iter++;
    new_outer=TRUE;

    /* (re)set the minimizer */
    gsl_multimin_fdfminimizer_set(multimin_minimizer, &multimin_func, 
				  initial, 0.0001, stopping_criteria);

    do { /* inner loop */
      inner_iter++;
      
      status = gsl_multimin_fdfminimizer_iterate (multimin_minimizer);
      
      if (!status) 
	status = gsl_multimin_test_gradient (multimin_minimizer->gradient, stopping_criteria);

      
      if (timer != NULL) {
	/* we only update things if at least 1 second has passed or we're at the start of a new loop */
	if ((g_timer_elapsed(timer,NULL) > 1.0) || (new_outer)) {
	  new_outer = FALSE;

	  if (update_func != NULL) 
	    continue_work = (*update_func)(update_data, NULL, (gdouble) inner_iter/max_iterations);
      
	  g_timer_start(timer); /* reset the timer */
#if AMIDE_DEBUG  
	  g_print("iter %d-%d\t%10.9g = %5.3g + %5.3g + %5.3g + %5.3g\t%g\t%g     \r",
		  outer_iter, inner_iter, 
		  100.0*gsl_multimin_fdfminimizer_minimum(multimin_minimizer)/magnitude,
		  100.0*p.ls/magnitude,
		  100.0*p.neg/magnitude,
		  100.0*p.b*p.orth/magnitude,
		  100.0*p.blood/magnitude,
		  p.mu, (beta > 0) ? 100*(current_beta/beta) : 0.0);
#endif /* AMIDE_DEBUG */
	}
#if AMIDE_DEBUG
	x = div(inner_iter,1000);
	if (x.rem == 0)
	  g_print("\n");
#endif /* AMIDE_DEBUG */
      }

      if (inner_iter >= max_iterations)
	  status = GSL_EMAXITER;

      if (isnan(gsl_multimin_fdfminimizer_minimum(multimin_minimizer)))
	status = GSL_ERUNAWAY;
      else
	gsl_vector_memcpy(initial, multimin_minimizer->x);
      
    } while ((status == GSL_CONTINUE) && continue_work); /* inner loop */

#if AMIDE_DEBUG  
    g_print("\n");
#endif /* AMIDE_DEBUG */
    
    
    /* adjust lagrangian conditions */
    if (((status == GSL_SUCCESS) || (status == GSL_ENOPROG) || (status == GSL_ERUNAWAY)) && 
	continue_work && ((p.mu > FINAL_MU) || (current_beta < beta))) {

#if AMIDE_DEBUG
      if (status == GSL_ENOPROG)
	g_print("--- previous iteration was not making progress towards a solution ---\n");
      else if (status == GSL_ERUNAWAY)
	g_print("--- previous iteration ran away, reseting to last good value  ---\n");
#endif
      if (status == GSL_ERUNAWAY) {
	/* need to recompute ec's */
	pls_calc_constraints(&p, initial);
      }

      /* adjust lagrangian's */
      for (i=0; i<p.num_voxels; i++) {

	if (p.sum_factors_equal_one) {
	  p.lme_a[i] -= p.ec_a[i]/p.mu;
	}

	for (f=0; f<p.num_factors; f++) {
	  alpha = gsl_vector_get(initial, p.alpha_offset + i*p.num_factors+f);
	  p.lmi_a[i*p.num_factors+f] -= alpha/p.mu;
	  if (p.lmi_a[i*p.num_factors+f] < 0.0) 
	    p.lmi_a[i*p.num_factors+f] = 0.0;
	}
      }

      for (i=0; i<p.num_blood_curve_constraints; i++) 
	p.lme_bc[i] -= p.ec_bc[i]/p.mu;

      for (f=0; f<p.num_factors; f++)
	for (j=0; j<p.num_frames; j++) {
	  factor = gsl_vector_get(initial, f*p.num_frames+j);
	  p.lmi_f[f*p.num_frames+j] -= factor/p.mu;
	  if (p.lmi_f[f*p.num_frames+j] < 0.0) 
	    p.lmi_f[f*p.num_frames+j] = 0.0;
	}

      /* adjust current beta */
      if (current_beta == 0)
	current_beta = beta/100;
      else if (current_beta < beta)
	current_beta *= 1.1;

      /* adjust mu */
      if (p.mu > FINAL_MU)
	p.mu *= 0.7;

      status = GSL_CONTINUE;
    }
    
  } while ((status == GSL_CONTINUE) && continue_work); /* outer loop */




#if AMIDE_DEBUG  
  if (status == GSL_SUCCESS) 
    g_print("Minimum found after %d iterations\n", inner_iter);
  else if (status == GSL_CONTINUE)
    g_print("terminated minization \n");
  else 
    g_print("No minimum found after %d iterations, exited with: %s\n", 
	    inner_iter,gsl_strerror(status));
#endif /* AMIDE_DEBUG */

  if (update_func != NULL) /* remove progress bar */
    continue_work = (*update_func)(update_data, NULL, (gdouble) 2.0); 

  /* add the different coefficients to the tree */
  dim.t = 1;
  i_voxel.t = 0;
  for (f=0; f<p.num_factors; f++) {
    new_ds = amitk_data_set_new_with_data(NULL, AMITK_DATA_SET_MODALITY(data_set),
					  AMITK_FORMAT_FLOAT, dim, AMITK_SCALING_TYPE_0D);
    if (new_ds == NULL) {
      g_warning(_("failed to allocate new_ds"));
      goto ending;
    }
    for (i_view_mode=0; i_view_mode < AMITK_VIEW_MODE_NUM; i_view_mode++)
      amitk_data_set_set_color_table(new_ds, i_view_mode, AMITK_DATA_SET_COLOR_TABLE(data_set, i_view_mode));

    i=p.alpha_offset; /* what to skip in v to get to the coefficients */
    for (i_voxel.g=0; i_voxel.g<p.dim.g; i_voxel.g++) 
      for (i_voxel.z=0; i_voxel.z<p.dim.z; i_voxel.z++) 
	for (i_voxel.y=0; i_voxel.y<p.dim.y; i_voxel.y++) 
	  for (i_voxel.x=0; i_voxel.x<p.dim.x; i_voxel.x++, i+= p.num_factors) {
	    alpha = gsl_vector_get(initial, i+f);
	    AMITK_DATA_SET_FLOAT_0D_SCALING_SET_CONTENT(new_ds, i_voxel, alpha);
	  }
  

    temp_string = g_strdup_printf(_("factor %d"), f+1);
    amitk_object_set_name(AMITK_OBJECT(new_ds),temp_string);
    g_free(temp_string);
    amitk_space_copy_in_place(AMITK_SPACE(new_ds), AMITK_SPACE(p.data_set));
    amitk_data_set_set_voxel_size(new_ds, AMITK_DATA_SET_VOXEL_SIZE(p.data_set));
    amitk_data_set_set_modality(new_ds, AMITK_DATA_SET_MODALITY(p.data_set));
    amitk_data_set_calc_far_corner(new_ds);
    amitk_data_set_set_threshold_max(new_ds, 0, amitk_data_set_get_global_max(new_ds));
    amitk_data_set_set_threshold_min(new_ds, 0, amitk_data_set_get_global_min(new_ds));
    amitk_data_set_set_interpolation(new_ds, AMITK_DATA_SET_INTERPOLATION(p.data_set));
    amitk_object_add_child(AMITK_OBJECT(p.data_set), AMITK_OBJECT(new_ds));
    amitk_object_unref(new_ds); /* add_child adds a reference */
  }



  /* and writeout the factor file */
  if ((file_pointer = fopen(output_filename, "w")) == NULL) {
    g_warning(_("couldn't open: %s for writing fads analyses"), output_filename);
    goto ending;
  }

  write_header(file_pointer, status, FADS_TYPE_PLS, data_set, inner_iter);

  fprintf(file_pointer, "# frame\tduration (s)\ttime midpt (s)\tfactor:\n");
  fprintf(file_pointer, "#\t");
  for (f=0; f<p.num_factors; f++)
    fprintf(file_pointer, "\t\t%d", f+1);
  fprintf(file_pointer, "\n");

  for (j=0; j<p.num_frames; j++) {
    frame_midpoint = amitk_data_set_get_midpt_time(p.data_set, j);
    frame_duration = amitk_data_set_get_frame_duration(p.data_set, j);

    fprintf(file_pointer, "  %d", j);
    fprintf(file_pointer, "\t%12.3f\t%12.3f\t", frame_duration, frame_midpoint);
    for (f=0; f<p.num_factors; f++) {
      fprintf(file_pointer, "\t% 12g", gsl_vector_get(initial, f*p.num_frames+j));
    }
    fprintf(file_pointer,"\n");
  }


 ending:

  if (file_pointer != NULL) {
    fclose(file_pointer);
    file_pointer = NULL;
  }

  if (multimin_minimizer != NULL) {
    gsl_multimin_fdfminimizer_free (multimin_minimizer) ;
    multimin_minimizer = NULL;
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

  if (p.ec_a != NULL) {
    g_free(p.ec_a);
    p.ec_a = NULL;
  }

  if (p.ec_bc != NULL) {
    g_free(p.ec_bc);
    p.ec_bc = NULL;
  }

  if (p.lme_a != NULL) {
    g_free(p.lme_a);
    p.lme_a = NULL;
  }

  if (p.lme_bc != NULL) {
    g_free(p.lme_bc);
    p.lme_bc = NULL;
  }

  if (p.lmi_a != NULL) {
    g_free(p.lmi_a);
    p.lmi_a = NULL;
  }

  if (p.lmi_f != NULL) {
    g_free(p.lmi_f);
    p.lmi_f = NULL;
  }

  if (timer != NULL) {
    g_timer_destroy(timer);
    timer = NULL;
  }
};








typedef struct two_comp_params_t {
  AmitkDataSet * data_set;
  AmitkVoxel dim;
  gdouble mu;
  gint num_voxels; /* voxels/frame */
  gint num_frames; 
  gint num_factors;
  gint num_tissues; /* num_factors-1 */
  gint k12_offset; /* 0 */
  gint k21_offset; /* k12_offset+num_tissues */
  gint bc_offset; /* k21_offset+num_tissues */
  gint alpha_offset; /* bc_offset+num_frames */
  gint num_variables; /* alpha_offset + num_factors*num_voxels */

  /* tc_unscaled[f]*k21(f) would be our estimate for compartment 2 (tissue component) */
  gdouble * tc_unscaled; 

  gdouble * forward_error; /* our estimated data (the forward problem), subtracted by the actual data */
  gdouble * weight; /* the appropriate weight (frame dependent) */
  gdouble * start; /* start time of each frame */
  gdouble * end; /* end time of each frame */
  gdouble * midpt; /* midpt of each frame */
  gdouble * ec_a; /* used for sum alpha == 1.0 */
  gdouble * ec_bc;

  /* lagrange multipliers - equality */
  gdouble * lme_a; /* used for sum alpha == 1.0 */
  gdouble * lme_bc;

  /* lagrange multipliers - inequality */
  gdouble * lmi_a; /* 0 <= alpha <= 1.0 */
  gdouble * lmi_bc;
  gdouble * lmi_k12;
  gdouble * lmi_k21;

  /* variable constraints */
  gboolean sum_factors_equal_one; /* whether to constrain by sum alpha == 1.0 */
  gint num_blood_curve_constraints;
  gint * blood_curve_constraint_frame;
  gdouble * blood_curve_constraint_val;

  gdouble blood;
  gdouble ls;
  gdouble neg;
} two_comp_params_t;



static void two_comp_calc_constraints(two_comp_params_t * p, const gsl_vector *v) {

  gint f, i, k;
  gdouble total, bc;


  /* total alpha == 1 constraints */
  if (p->sum_factors_equal_one) {
    for (k=0, i=p->alpha_offset; k < p->num_voxels; k++, i+=p->num_factors) {
      total = -1.0;
      for (f=0; f<p->num_factors; f++)
	total += gsl_vector_get(v, i+f);
      p->ec_a[k] = total;
    }
  }

  /* blood curve constraints */
  for (i=0; i<p->num_blood_curve_constraints; i++) {
    bc = gsl_vector_get(v, p->blood_curve_constraint_frame[i]);
    p->ec_bc[i] = bc-p->blood_curve_constraint_val[i];;
  }

}

static void two_comp_calc_compartments(two_comp_params_t * p, const gsl_vector *v) {

  gint j, k, t;
  gdouble convolution_value, kernel;
  gdouble k12, bc;


  for (t=0; t<p->num_tissues; t++) {
    k12 = gsl_vector_get(v, p->k12_offset+t);

    for (j=0; j<p->num_frames; j++) {

      convolution_value=0;
      for (k=0; k < j; k++) { 
	bc = gsl_vector_get(v, p->bc_offset+k);
	if (fabs(k12) < EPSILON) 
	  kernel = p->end[k]-p->start[k];
	else 
	  kernel = (exp(-k12*(p->midpt[j]-p->end[k]))-exp(-k12*(p->midpt[j]-p->start[k])))/k12;
	convolution_value += bc*kernel;
      }
      /* k == j */
      bc = gsl_vector_get(v, p->bc_offset+j);
      if (fabs(k12) < EPSILON) 
	kernel = p->midpt[j]-p->start[j];
      else 
	kernel = (1-exp(-k12*(p->midpt[j]-p->start[j])))/k12;
      convolution_value += bc*kernel;


      p->tc_unscaled[j*p->num_tissues+t] = convolution_value;
    }
  }

  return;
}


static void two_comp_calc_forward_error(two_comp_params_t * p, const gsl_vector *v) {

  gint i, k, t;
  AmitkVoxel i_voxel;
  gdouble bc;
  gdouble k21;
  gdouble inner, alpha;


  for (i_voxel.t=0; i_voxel.t<p->dim.t; i_voxel.t++) {
    i=p->alpha_offset;
    k=0;
    bc = gsl_vector_get(v, p->bc_offset+i_voxel.t);
    for (i_voxel.g=0; i_voxel.g<p->dim.g; i_voxel.g++) {
      for (i_voxel.z=0; i_voxel.z<p->dim.z; i_voxel.z++) {
	for (i_voxel.y=0; i_voxel.y<p->dim.y; i_voxel.y++) {
	  for (i_voxel.x=0; i_voxel.x<p->dim.x; i_voxel.x++, k++, i+=p->num_factors) {
	    
	    inner=0;
	    for (t=0; t < p->num_tissues; t++) {
	      k21 = gsl_vector_get(v, p->k21_offset+t);
	      alpha = gsl_vector_get(v, i+t);
	      inner += alpha*k21*p->tc_unscaled[i_voxel.t*p->num_tissues+t];
	    }
	    
	    alpha = gsl_vector_get(v, i+p->num_tissues);
	    p->forward_error[k*p->num_frames+i_voxel.t] = 
	      alpha*bc+inner-amitk_data_set_get_value(p->data_set,i_voxel);
	    
	  }
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
  gdouble temp, bc, k12, k21, alpha, lambda;
  gint i, k, j, f, t;

  /* the Least Squares objective */
  ls_answer = 0.0;
  for (i=0; i<p->num_voxels; i++) {
    for (j=0; j<p->num_frames; j++) {
      temp = p->forward_error[i*p->num_frames+j];
      ls_answer += p->weight[j]*temp*temp;
    }
  }


  neg_answer = 0.0;

  /* non negativity for the k12's */
  for (t=0; t<p->num_tissues; t++) {
    k12 = gsl_vector_get(v, p->k12_offset+t);
    lambda = p->lmi_k12[t];
    if ((k12-p->mu*lambda) < 0.0)
      neg_answer += k12*(k12/(2.0*p->mu)-lambda);
    else
      neg_answer -= lambda*lambda*p->mu/2.0;
  }

  /* non negativity for the k21's */
  for (t=0; t<p->num_tissues; t++) {
    k21 = gsl_vector_get(v, p->k21_offset+t);
    lambda = p->lmi_k21[t];
    if ((k21-p->mu*lambda) < 0.0)
      neg_answer += k21*(k21/(2.0*p->mu)-lambda);
    else
      neg_answer -= lambda*lambda*p->mu/2.0;
  }

  /* non-negativity for the blood curve */
  for (j=0; j<p->num_frames; j++) {
    bc = gsl_vector_get(v, p->bc_offset+j);
    lambda = p->lmi_bc[j];
    if ((bc-p->mu*lambda) < 0.0)
      neg_answer += bc*(bc/(2.0*p->mu) - lambda);
    else
      neg_answer -= lambda*lambda*p->mu/2.0;
  }

  /* non-negativity for the alpha's */
  for (k=0, i=p->alpha_offset; k<p->num_voxels; k++, i+=p->num_factors) {
    for (f=0; f<p->num_factors; f++) {
      alpha = gsl_vector_get(v, i+f);
      lambda = p->lmi_a[k*p->num_factors+f];
      if ((alpha-p->mu*lambda) < 0.0)
	neg_answer += alpha*(alpha/(2.0*p->mu) - lambda);
      else
	neg_answer -= lambda*lambda*p->mu/2.0;
    }
  }

  /* the sum of alpha's == 1 constraint */
  if (p->sum_factors_equal_one) {
    for (k=0; k< p->num_voxels; k++)
      neg_answer += p->ec_a[k]*(p->ec_a[k]/(2.0*p->mu) - p->lme_a[k]);
  }

  /* blood curve constraints */
  blood_answer = 0;
  for (i=0; i<p->num_blood_curve_constraints; i++) 
    blood_answer += p->ec_bc[i]*(p->ec_bc[i]/(2.0*p->mu) - p->lme_bc[i]);

  /* keep track of the three */
  p->ls = ls_answer;
  p->neg = neg_answer;
  p->blood = blood_answer;

  return ls_answer+neg_answer+blood_answer;
}

static void two_comp_calc_derivative(two_comp_params_t * p, const gsl_vector *v, gsl_vector *df) {

  gdouble ls_answer, neg_answer, blood_answer;
  gint f, i, j, k, t;
  gdouble k12, k21,  bc, inner, kernel, alpha;
  gdouble delta1, delta2, lambda;
  

  /* partial derivative of f wrt to the k12's */
  for (t=0; t<p->num_tissues; t++) {
    k12 = gsl_vector_get(v, p->k12_offset+t);
    k21 = gsl_vector_get(v, p->k21_offset+t);

    ls_answer=0;
    for (i=0; i<p->num_voxels; i++) {
      for (j=0; j<p->num_frames; j++) {

	inner = 0;
	for (k=0; k<j ; k++) {
	  delta1 = p->midpt[j]-p->end[k];
	  delta2 = p->midpt[j]-p->start[k];
	  if (fabs(k12) < EPSILON) 
	    kernel = 0.5*(delta1*delta1-delta2*delta2);
	  else
	    kernel = (-delta1*exp(-k12*delta1)+delta2*exp(-k12*delta2))/k12;
	  bc = gsl_vector_get(v, p->bc_offset+k);
	  inner += bc*kernel;
	}

	/* k == j */
	delta2 = p->midpt[j]-p->start[j];
	if (fabs(k12) < EPSILON) 
	  kernel = -0.5*(delta2*delta2);
	else
	  kernel = (delta2*exp(-k12*delta2))/k12;
	bc = gsl_vector_get(v, p->bc_offset+j);
	inner += bc*kernel;

	if (fabs(k12) > EPSILON) 
	  inner -= p->tc_unscaled[j*p->num_tissues+t]/k12;

	alpha = gsl_vector_get(v, p->alpha_offset+i*p->num_factors+t);
	ls_answer += p->weight[j]*p->forward_error[i*p->num_frames+j] * alpha*k21*inner;
      }
    }
    ls_answer *=2;

    /* the non-negatvity objective */
    lambda = p->lmi_k12[t];
    if ((k12 - p->mu*lambda) < 0.0)
      neg_answer = k12/p->mu - lambda;
    else
      neg_answer = 0.0;

    gsl_vector_set(df, p->k12_offset+t, ls_answer+neg_answer);
  }

  /* partial derivative of f wrt to the k21's */
  for (t=0; t<p->num_tissues; t++) {
    k21 = gsl_vector_get(v, p->k21_offset+t);

    ls_answer=0;
    for (i=0; i<p->num_voxels; i++) {
      alpha = gsl_vector_get(v, p->alpha_offset+i*p->num_factors +t);
      for (j=0; j<p->num_frames; j++) {
	ls_answer += 
	  p->weight[j]*
	  p->forward_error[i*p->num_frames+j] *
	  alpha*
	  p->tc_unscaled[j*p->num_tissues+t];
      }
    }
    ls_answer *=2;

    /* the non-negatvity objective */
    lambda = p->lmi_k21[t];
    if ((k21 - p->mu*lambda) < 0.0)
      neg_answer = k21/p->mu - lambda;
    else
      neg_answer = 0.0;

    gsl_vector_set(df, p->k21_offset+t, ls_answer+neg_answer);
  }

  /* partial derivative of f wrt to the blood curve */
  for (j=0; j<p->num_frames; j++) {
    bc = gsl_vector_get(v, p->bc_offset+j);

    ls_answer=0;
    for (i=0; i<p->num_voxels; i++) {

      /* k == j */
      inner = gsl_vector_get(v, p->alpha_offset+i*p->num_factors+p->num_tissues);
      for (t=0; t< p->num_tissues; t++) {
	alpha = gsl_vector_get(v, p->alpha_offset+i*p->num_factors+t);
	k12 = gsl_vector_get(v, p->k12_offset+t);
	k21 = gsl_vector_get(v, p->k21_offset+t);
	if (fabs(k12) < EPSILON) {
	  kernel = p->midpt[j]-p->start[j];
	} else {
	  kernel = (1-exp(-k12*(p->midpt[j]-p->start[j])))/k12;
	}
	inner += alpha*k21*kernel;
      }
      ls_answer += p->weight[j]*p->forward_error[i*p->num_frames+j]*inner;


      /* k > j */
      for (k=j+1; k<p->num_frames; k++) {
	
	inner = 0;
	for (t=0; t< p->num_tissues; t++) {
	  alpha = gsl_vector_get(v, p->alpha_offset+i*p->num_factors+t);
	  k12 = gsl_vector_get(v, p->k12_offset+t);
	  k21 = gsl_vector_get(v, p->k21_offset+t);
	  if (fabs(k12) < EPSILON) {
	    kernel = p->end[j]-p->start[j];
	  } else {
	    kernel = (exp(-k12*(p->midpt[k]-p->end[j]))-exp(-k12*(p->midpt[k]-p->start[j])))/k12;
	  }
	  inner += alpha*k21*kernel;
	}
	ls_answer += p->weight[k]*p->forward_error[i*p->num_frames+k]*inner;
      }
    }
    ls_answer *= 2;

    /* the non-negatvity objective */
    lambda = p->lmi_bc[j];
    if ((bc - p->mu*lambda) < 0.0)
      neg_answer = bc/p->mu - lambda;
    else
      neg_answer = 0.0;

    /* blood curve constraints */
    blood_answer = 0;
    for (i=0; i<p->num_blood_curve_constraints; i++) 
      if (p->blood_curve_constraint_frame[i] == j) 
	blood_answer += p->ec_bc[i]/p->mu - p->lme_bc[i];

    gsl_vector_set(df, p->bc_offset+j, ls_answer+neg_answer+blood_answer);
  }


  /* partial derivative of f wrt to the alpha's */
  for (i=0; i<p->num_voxels; i++) {
    for (f=0; f< p->num_factors; f++) {
      alpha = gsl_vector_get(v, p->alpha_offset+i*p->num_factors+f);

      ls_answer = 0;
      for (j=0; j< p->num_frames; j++) {
	if (f < p->num_tissues) {
	  k21 = gsl_vector_get(v, p->k21_offset+f);
	  inner = k21*p->tc_unscaled[j*p->num_tissues+f];
	} else {
	  bc = gsl_vector_get(v, p->bc_offset+j);
	  inner = bc;
	}
	ls_answer += p->weight[j]*p->forward_error[i*p->num_frames+j]*inner;
      }
      ls_answer *=2;

      /* the non-negatvity and <= 1 objective */
      lambda = p->lmi_a[i*p->num_factors+f];
      if ((alpha - p->mu*lambda) < 0.0)
	neg_answer = alpha/p->mu - lambda;
      else
	neg_answer = 0.0;

      /* the sum of alpha's == 1 constraint */
      if (p->sum_factors_equal_one) {
	neg_answer += p->ec_a[i]/p->mu - p->lme_a[i];
      }

      gsl_vector_set(df, p->alpha_offset+i*p->num_factors+f, ls_answer + neg_answer);
    }
  }

  return;
}

/* calculate the two compartment objective function */
static double two_comp_f (const gsl_vector *v, void *params) {

  two_comp_params_t * p = params;

  two_comp_calc_constraints(p, v);
  two_comp_calc_compartments(p,v);
  two_comp_calc_forward_error(p, v);

  return two_comp_calc_function(p, v);
}




/* The gradient of f, df = (df/dQt, df/dKij, df/dTi). */
static void two_comp_df (const gsl_vector *v, void *params, gsl_vector *df) {
  
  two_comp_params_t * p = params;

  two_comp_calc_constraints(p, v);
  two_comp_calc_compartments(p,v);
  two_comp_calc_forward_error(p, v);

  two_comp_calc_derivative(p, v, df);
  return;
}



/* Compute both f and df together. */
static void two_comp_fdf (const gsl_vector *v, void *params,  double *f, gsl_vector *df) {

  two_comp_params_t * p = params;

  two_comp_calc_constraints(p, v);
  two_comp_calc_compartments(p,v);
  two_comp_calc_forward_error(p, v);

  *f = two_comp_calc_function(p,v);
  two_comp_calc_derivative(p, v, df);
  return;
}




/* run a two compartment algorithm for factor analysis

   gsl supports minimizing on only a single vector space, so that
   vector is setup as follows
   
   M = num_voxels;
   N = num_frames
   F = num_factors (the last factor is the blood curve)
   bc = blood curve
   alpha = coefficent's

   x = [k12(1)           k12(2)        ...... k12(F-1)
        k21(1)           k21(2)        ...... k21(F-1)
        bc(1)            bc(2)         ...... bc(N)
        alpha(1,1)       alpha(1,2)    ...... alpha(1, F)
	alpha(2,1)       alpha(2,2)    ...... alpha(2, F)
                                       ......
	alpha(M,1)       alpha(M,2)    ...... alpha(M, F) ]
	

   forward_error is [M*N]
   tc_unscaled is [N*F]

*/

void fads_two_comp(AmitkDataSet * data_set, 
		   fads_minimizer_algorithm_t minimizer_algorithm,
		   gint max_iterations,
		   gint tissue_types,
		   gdouble supplied_k12,
		   gdouble supplied_k21,
		   gdouble stopping_criteria,
		   gboolean sum_factors_equal_one,
		   gchar * output_filename,
		   gint num_blood_curve_constraints,
		   gint * blood_curve_constraint_frame,
		   gdouble * blood_curve_constraint_val,
		   AmitkUpdateFunc update_func,
		   gpointer update_data) {

  gsl_multimin_fdfminimizer * multimin_minimizer = NULL;
  gsl_multimin_function_fdf multimin_func;
  AmitkVoxel dim;
  two_comp_params_t p;
  gint outer_iter=0;
  gint inner_iter=0;
  gsl_vector * initial=NULL;
  gint i, f, j, t;
  gint status;
  FILE * file_pointer=NULL;
  gchar * temp_string;
  gboolean continue_work=TRUE;
  gdouble temp, bc;
  amide_time_t frame_midpoint, frame_duration;
  amide_time_t time_constant;
  amide_time_t time_start;
  AmitkDataSet * new_ds;
  AmitkVoxel i_voxel;
  gdouble magnitude, k12, k21;
  gdouble init_value, alpha;
  AmitkViewMode i_view_mode;
  GTimer * timer=NULL;
  gboolean new_outer;
#if AMIDE_DEBUG
  div_t x;
#endif

  g_return_if_fail(AMITK_IS_DATA_SET(data_set));
  dim = AMITK_DATA_SET_DIM(data_set);

  g_return_if_fail(tissue_types >= 1);

  /* initialize our parameter structure */
  p.data_set = data_set;
  p.dim = dim;
  p.mu = 1000.0;
  p.ls = 0.0;
  p.neg = 0.0;
  p.blood = 0.0;
  p.num_voxels = dim.g*dim.z*dim.y*dim.x;
  p.num_frames = dim.t;
  p.num_factors = tissue_types+1;
  p.num_tissues = tissue_types;
  p.k12_offset = 0;
  p.k21_offset = p.k12_offset + p.num_tissues;
  p.bc_offset = p.k21_offset + p.num_tissues;
  p.alpha_offset = p.bc_offset+p.num_frames;
  p.num_variables = p.alpha_offset + p.num_factors*p.num_voxels;
  p.tc_unscaled = NULL;
  p.forward_error = NULL;
  p.start = NULL;
  p.end = NULL;
  p.ec_a = NULL;
  p.ec_bc = NULL;
  p.lme_a = NULL;
  p.lme_bc = NULL;
  p.lmi_a = NULL;
  p.lmi_bc = NULL;
  p.lmi_k12 = NULL;
  p.lmi_k12 = NULL;

  p.sum_factors_equal_one = sum_factors_equal_one;
  p.num_blood_curve_constraints = num_blood_curve_constraints;
  p.blood_curve_constraint_frame = blood_curve_constraint_frame;
  p.blood_curve_constraint_val = blood_curve_constraint_val;

  /* more sanity checks */
  for (i=0; i<p.num_blood_curve_constraints; i++) {
    g_return_if_fail(p.blood_curve_constraint_frame[i] < p.dim.t);
  }

  p.tc_unscaled = g_try_new(gdouble, p.num_tissues*p.num_frames);
  if (p.tc_unscaled == NULL) {
    g_warning(_("failed to allocate intermediate data storage for tissue components"));
    goto ending;
  }

  p.forward_error = g_try_new(gdouble, p.num_frames*p.num_voxels);
  if (p.forward_error == NULL) {
    g_warning(_("failed to allocate intermediate data storage for forward error"));
    goto ending;
  }
  
  p.start = g_try_new(gdouble, p.num_frames);
  if (p.start == NULL) {
    g_warning(_("failed to allocate frame start array"));
    goto ending;
  }

  p.end = g_try_new(gdouble, p.num_frames);
  if (p.end == NULL) {
    g_warning(_("failed to allocate frame end array"));
    goto ending;
  }
  
  p.midpt = g_try_new(gdouble, p.num_frames);
  if (p.midpt == NULL) {
    g_warning(_("failed to allocate frame midpt array"));
    goto ending;
  }

  for (j=0; j<p.num_frames; j++) {
    p.start[j] = amitk_data_set_get_start_time(p.data_set, j);
    p.end[j] = amitk_data_set_get_end_time(p.data_set, j);
    p.midpt[j] = amitk_data_set_get_midpt_time(p.data_set, j);
  }

  if (p.sum_factors_equal_one) {
    p.ec_a = g_try_new(gdouble, p.num_voxels);
    if (p.ec_a == NULL) {
      g_warning(_("failed malloc for equality constraint on alpha"));
      goto ending;
    }
  }

  p.ec_bc = g_try_new(gdouble, p.num_blood_curve_constraints);
  if ((p.ec_bc == NULL) && (p.num_blood_curve_constraints > 0)) {
    g_warning(_("failed malloc for equality constraint on blood curve"));
    goto ending;
  }

  if (p.sum_factors_equal_one) {
    p.lme_a = g_try_new(gdouble, p.num_voxels);
    if (p.lme_a == NULL) {
      g_warning(_("failed malloc for equality lagrange multiplier - alpha"));
      goto ending;
    }
    for (i=0; i<p.num_voxels; i++)
      p.lme_a[i] = 0.0;
  }

  p.lme_bc = g_try_new(gdouble, p.num_blood_curve_constraints);
  if ((p.lme_bc == NULL) && (p.num_blood_curve_constraints > 0)) {
    g_warning(_("failed malloc for equality lagrange multiplier - blood curve"));
    goto ending;
  }
  for (i=0; i<p.num_blood_curve_constraints; i++)
    p.lme_bc[i] = 0.0;

  p.lmi_a = g_try_new(gdouble, p.num_voxels*p.num_factors);
  if (p.lmi_a == NULL) {
    g_warning(_("failed malloc for inequality lagrange multiplier - alpha"));
    goto ending;
  }
  for (i=0; i<p.num_voxels*p.num_factors; i++)
    p.lmi_a[i] = 0.0;

  p.lmi_bc = g_try_new(gdouble, p.num_frames);
  if (p.lmi_bc == NULL) {
    g_warning(_("failed malloc for inequailty lagrange multiplier - blood curve"));
    goto ending;
  }
  for (j=0; j<p.num_frames; j++)
    p.lmi_bc[j] = 0.0;

  p.lmi_k12 = g_try_new(gdouble, p.num_tissues);
  if (p.lmi_k12 == NULL) {
    g_warning(_("failed malloc for inequailty lagrange multiplier - k12"));
    goto ending;
  }
  for (t=0; t<p.num_tissues; t++)
    p.lmi_k12[t] = 0.0;

  p.lmi_k21 = g_try_new(gdouble, p.num_tissues);
  if (p.lmi_k21 == NULL) {
    g_warning(_("failed malloc for inequailty lagrange multiplier - k21"));
    goto ending;
  }
  for (t=0; t<p.num_tissues; t++)
    p.lmi_k21[t] = 0.0;

  

  /* calculate the weights and magnitude */
  p.weight = calc_weights(p.data_set);
  g_return_if_fail(p.weight != NULL); /* make sure we've malloc'd it */
  magnitude = calc_magnitude(p.data_set, p.weight);

  
  /* set up gsl */
  multimin_func.f = two_comp_f;
  multimin_func.df = two_comp_df;
  multimin_func.fdf = two_comp_fdf;
  multimin_func.n = p.num_variables;
  multimin_func.params = &p;

  multimin_minimizer = alloc_fdfminimizer(minimizer_algorithm,p.num_variables);
  if (multimin_minimizer == NULL) {
    g_warning(_("failed to allocate multidimensional minimizer"));
    goto ending;
  }


  /* Starting point */
  initial = gsl_vector_alloc (p.num_variables);
  if (initial == NULL) {
    g_warning(_("failed to allocate initial vector"));
    goto ending;
  }
  
  /* initialize k's with unique values
     initializing bc with a monoexponential*frame_max,
        use a time constant of 100th of the study length, as a guess...
     initialize the alpha's to something reasonable
  */
  for (t=0; t< p.num_tissues; t++) {
    gsl_vector_set(initial, p.k12_offset+t, supplied_k12/(t+1));
    gsl_vector_set(initial, p.k21_offset+t, 2*supplied_k21/(t+2));
  }

  /* gotta reference from somewhere */
  time_start = p.midpt[0];
  time_constant = p.end[p.num_frames-1]/10;

  for (j=0; j<p.num_frames; j++) {
    temp = exp(-(p.midpt[j]-time_start)/time_constant); 
    temp *= 3.0*amitk_data_set_get_frame_max(p.data_set, j);
    gsl_vector_set(initial, p.bc_offset+j, temp);
  }

  init_value = 1/p.num_factors;
  for (i=p.alpha_offset; i<p.num_variables; i+=p.num_factors)
    for (f=0; f<p.num_factors; f++)
      gsl_vector_set(initial, i+f, init_value);

  if (update_func != NULL) {
    timer = g_timer_new();
    temp_string = g_strdup_printf(_("Calculating Two Compartment Factor Analysis:\n   %s"), AMITK_OBJECT_NAME(data_set));
    continue_work = (*update_func)(update_data, temp_string, (gdouble) 0.0);
    g_free(temp_string);
  }

#if AMIDE_DEBUG
  if (timer != NULL)
    timer = g_timer_new();
#endif

  do { /* outer loop */
    outer_iter++;
    new_outer=TRUE;

    /* (re)set the minimizer */
    gsl_multimin_fdfminimizer_set(multimin_minimizer, &multimin_func, 
				  initial, 0.0001, stopping_criteria);

    do { /* inner loop */
      inner_iter++;

      status = gsl_multimin_fdfminimizer_iterate (multimin_minimizer);

      if (!status) 
      	status = gsl_multimin_test_gradient (multimin_minimizer->gradient, stopping_criteria);

      if (timer != NULL) {
	/* we only update things if at least 1 second has passed or we're at the start of a new loop */
	if ((g_timer_elapsed(timer,NULL) > 1.0) || (new_outer)) {
	  new_outer = FALSE;

	  if (update_func != NULL) 
	    continue_work = (*update_func)(update_data, NULL, (gdouble) inner_iter/max_iterations);

	  g_timer_start(timer); /* reset the timer */
#if AMIDE_DEBUG  
	  g_print("iter %d %d   %10.9g=%5.3g+%5.3g+%5.3g   mu=%g k12=%g k21=%g     \r",
		  outer_iter, inner_iter,
		  100.0*gsl_multimin_fdfminimizer_minimum(multimin_minimizer)/magnitude,
		  100.0*p.ls/magnitude,
		  100.0*p.neg/magnitude,
		  100.0*p.blood/magnitude, 
		  p.mu, 
		  gsl_vector_get(multimin_minimizer->x, p.k12_offset),
		  gsl_vector_get(multimin_minimizer->x, p.k21_offset));
	  //      g_print("bc %g %g   tc %g %g      alpha %g %g\n", 
	  //	      gsl_vector_get(multimin_minimizer->x, p.bc_offset),
	  //	      gsl_vector_get(multimin_minimizer->x, p.bc_offset+1),
	  //	      p.tc_unscaled[0], p.tc_unscaled[1],
	  //	      gsl_vector_get(multimin_minimizer->x, p.alpha_offset),
	  //	      gsl_vector_get(multimin_minimizer->x, p.alpha_offset+1));
#endif /* AMIDE_DEBUG */
	}
#if AMIDE_DEBUG
	x = div(inner_iter,1000);
	if (x.rem == 0)
	  g_print("\n");
#endif /* AMIDE_DEBUG */
      }


      if (inner_iter >= max_iterations)
	  status = GSL_EMAXITER;

      if (isnan(gsl_multimin_fdfminimizer_minimum(multimin_minimizer)))
	status = GSL_ERUNAWAY;
      else
	gsl_vector_memcpy(initial, multimin_minimizer->x);


    } while ((status == GSL_CONTINUE) && continue_work); /* inner loop */

#if AMIDE_DEBUG
    g_print("\n");
#endif

    /* adjust lagrangian conditions */
    if (((status == GSL_SUCCESS) || (status == GSL_ENOPROG) || (status == GSL_ERUNAWAY)) && 
	continue_work && (p.mu > FINAL_MU)) {

#if AMIDE_DEBUG
      if (status == GSL_ENOPROG)
	g_print("--- previous iteration was not making progress towards a solution ---\n");
      else if (status == GSL_ERUNAWAY)
	g_print("--- previous iteration ran away, reseting to last good value  ---\n");
#endif
      if (status == GSL_ERUNAWAY) {
	/* need to recompute ec's */
	two_comp_calc_constraints(&p, initial);
      }

      /* adjust lagrangian's */
      for (i=0; i<p.num_voxels; i++) {

	if (p.sum_factors_equal_one) {
	  p.lme_a[i] -= p.ec_a[i]/p.mu;
	}

	for (f=0; f<p.num_factors; f++) {
	  alpha = gsl_vector_get(initial, p.alpha_offset + i*p.num_factors+f);
	  p.lmi_a[i*p.num_factors+f] -= alpha/p.mu;
	  if (p.lmi_a[i*p.num_factors+f] < 0.0) 
	    p.lmi_a[i*p.num_factors+f] = 0.0;
	}
      }

      for (i=0; i<p.num_blood_curve_constraints; i++) 
	p.lme_bc[i] -= p.ec_bc[i]/p.mu;

      for (j=0; j<p.num_frames; j++) {
	bc = gsl_vector_get(initial, p.bc_offset+j);
	p.lmi_bc[j] -= bc/p.mu;
	if (p.lmi_bc[j] < 0.0) 
	  p.lmi_bc[j] = 0.0;
      }

      for (t=0; t<p.num_tissues; t++) {
	k12 = gsl_vector_get(initial, p.k12_offset+t);
	p.lmi_k12[t] -= k12/p.mu;
	if (p.lmi_k12[t] < 0.0)
	  p.lmi_k12[t] = 0.0;
	
	k21 = gsl_vector_get(initial, p.k12_offset+t);
	p.lmi_k21[t] -= k21/p.mu;
	if (p.lmi_k21[t] < 0.0)
	  p.lmi_k21[t] = 0.0;
      }
      
      /* adjust mu */
      if (p.mu > FINAL_MU) {
	p.mu *= 0.7;
      }
      
      status = GSL_CONTINUE;
    }
    


  } while ((status == GSL_CONTINUE) && continue_work); /* outer loop */


#if AMIDE_DEBUG  
  if (status == GSL_SUCCESS) 
    g_print(_("Minimum found after %d iterations\n"), inner_iter);
  else if (status == GSL_CONTINUE)
    g_print(_("terminated minization \n"));
  else 
    g_print(_("No minimum found after %d iterations, exited with: %s\n"), 
	    inner_iter,gsl_strerror(status));
#endif /* AMIDE_DEBUG */

  if (update_func != NULL) /* remove progress bar */
    continue_work = (*update_func)(update_data, NULL, (gdouble) 2.0); 


  /* add the different coefficients to the tree */
  dim.t = 1;
  i_voxel.t = 0;
  for (f=0; f < p.num_factors; f++) {
    new_ds = amitk_data_set_new_with_data(NULL, AMITK_DATA_SET_MODALITY(data_set),
					  AMITK_FORMAT_FLOAT, dim, AMITK_SCALING_TYPE_0D);
    if (new_ds == NULL) {
      g_warning(_("failed to allocate new_ds"));
      goto ending;
    }

    for (i_view_mode = 0; i_view_mode < AMITK_VIEW_MODE_NUM; i_view_mode++)
      amitk_data_set_set_color_table(new_ds, i_view_mode, AMITK_DATA_SET_COLOR_TABLE(data_set, i_view_mode));

    i=p.alpha_offset; /* what to skip in v to get to the coefficients */
    for (i_voxel.g=0; i_voxel.g<p.dim.g; i_voxel.g++) 
      for (i_voxel.z=0; i_voxel.z<p.dim.z; i_voxel.z++) 
	for (i_voxel.y=0; i_voxel.y<p.dim.y; i_voxel.y++) 
	  for (i_voxel.x=0; i_voxel.x<p.dim.x; i_voxel.x++, i+=p.num_factors) {
	    alpha = gsl_vector_get(initial, i+f);
	    AMITK_DATA_SET_FLOAT_0D_SCALING_SET_CONTENT(new_ds, i_voxel, alpha);
	  }

    if (f < p.num_tissues) {
      k12 = gsl_vector_get(initial, p.k12_offset+f);
      k21 = gsl_vector_get(initial, p.k21_offset+f);
      temp_string = g_strdup_printf(_("tissue type %d: k12 %g k21 %g"), f+1, k12,k21);
    } else
      temp_string = g_strdup_printf(_("blood fraction"));
    amitk_object_set_name(AMITK_OBJECT(new_ds),temp_string);
    g_free(temp_string);
    amitk_space_copy_in_place(AMITK_SPACE(new_ds), AMITK_SPACE(p.data_set));
    amitk_data_set_set_voxel_size(new_ds, AMITK_DATA_SET_VOXEL_SIZE(p.data_set));
    amitk_data_set_set_modality(new_ds, AMITK_DATA_SET_MODALITY(p.data_set));
    amitk_data_set_calc_far_corner(new_ds);
    amitk_data_set_set_threshold_max(new_ds, 0, amitk_data_set_get_global_max(new_ds));
    amitk_data_set_set_threshold_min(new_ds, 0, amitk_data_set_get_global_min(new_ds));
    amitk_data_set_set_interpolation(new_ds, AMITK_DATA_SET_INTERPOLATION(p.data_set));
    amitk_object_add_child(AMITK_OBJECT(p.data_set), AMITK_OBJECT(new_ds));
    amitk_object_unref(new_ds); /* add_child adds a reference */
  }


  /* and writeout the tissue curves file */
  if ((file_pointer = fopen(output_filename, "w")) == NULL) {
    g_warning(_("couldn't open: %s for writing fads analyses"), output_filename);
    goto ending;
  }

  write_header(file_pointer, status, FADS_TYPE_TWO_COMPARTMENT, data_set, inner_iter);

  fprintf(file_pointer, "# frame\tduration (s)\ttime midpt (s)\tblood curve");
  for (t=0; t<p.num_tissues; t++)
    fprintf(file_pointer, "\ttissue curve %d",t);
  fprintf(file_pointer, "\n");

  fprintf(file_pointer, "# k12  \t              \t           ");
  for (t=0; t< p.num_tissues; t++)
    fprintf(file_pointer, "\t%f",gsl_vector_get(initial, p.k12_offset+t));
  fprintf(file_pointer, "\n");

  fprintf(file_pointer, "# k21  \t              \t           ");
  for (t=0; t<p.num_tissues; t++)
    fprintf(file_pointer, "\t%f",gsl_vector_get(initial, p.k21_offset+t));
  fprintf(file_pointer, "\n");

  for (j=0; j<p.num_frames; j++) {
    frame_midpoint = amitk_data_set_get_midpt_time(p.data_set, j);
    frame_duration = amitk_data_set_get_frame_duration(p.data_set, j);

    fprintf(file_pointer, "  %d", j);
    fprintf(file_pointer, "\t%g\t%g\t", frame_duration, frame_midpoint);
    fprintf(file_pointer, "\t%g", gsl_vector_get(initial, p.bc_offset+j));
    for (t=0; t<p.num_tissues; t++) {
      k21 = gsl_vector_get(initial, p.k21_offset+t);
      fprintf(file_pointer, "\t%g", k21*p.tc_unscaled[j*p.num_tissues+t]);
    }
    fprintf(file_pointer, "\n");
    
  }

 ending:

  if (file_pointer != NULL) {
    fclose(file_pointer);
    file_pointer = NULL;
  }

  if (multimin_minimizer != NULL) {
    gsl_multimin_fdfminimizer_free (multimin_minimizer) ;
    multimin_minimizer = NULL;
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

  if (p.tc_unscaled != NULL) {
    g_free(p.tc_unscaled);
    p.tc_unscaled = NULL;
  }

  if (p.start != NULL) {
    g_free(p.start);
    p.start = NULL;
  }

  if (p.end != NULL) {
    g_free(p.end);
    p.end = NULL;
  }

  if (p.midpt != NULL) {
    g_free(p.midpt);
    p.midpt = NULL;
  }
  
  if (p.ec_a != NULL) {
    g_free(p.ec_a);
    p.ec_a = NULL;
  }

  if (p.ec_bc != NULL) {
    g_free(p.ec_bc);
    p.ec_bc = NULL;
  }

  if (p.lme_a != NULL) {
    g_free(p.lme_a);
    p.lme_a = NULL;
  }

  if (p.lme_bc != NULL) {
    g_free(p.lme_bc);
    p.lme_bc = NULL;
  }

  if (p.lmi_a != NULL) {
    g_free(p.lmi_a);
    p.lmi_a = NULL;
  }

  if (p.lmi_bc != NULL) {
    g_free(p.lmi_bc);
    p.lmi_bc = NULL;
  }

  if (p.lmi_k12 != NULL) {
    g_free(p.lmi_k12);
    p.lmi_k12 = NULL;
  }

  if (p.lmi_k21 != NULL) {
    g_free(p.lmi_k21);
    p.lmi_k21 = NULL;
  }

  if (timer != NULL) {
    g_timer_destroy(timer);
    timer = NULL;
  }

};

 
#endif /* AMIDE_LIBGSL_SUPPORT */
