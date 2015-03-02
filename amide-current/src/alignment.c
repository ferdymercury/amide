/* alignment.c
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
#ifdef AMIDE_LIBGSL_SUPPORT
#include <sys/stat.h>
#include <string.h>
#include <glib.h>
#include <gsl/gsl_linalg.h>
#include <math.h>
#include "alignment.h"

/* convient functions that gsl doesn't supply */
static gsl_matrix * matrix_mult(gsl_matrix * matrix1, gsl_matrix * matrix2) {

  gsl_matrix * matrix_r = NULL;
  gint i,j,k;
  gdouble temp_value;

  g_return_val_if_fail(matrix1->size2 == matrix2->size1, NULL);

  matrix_r = gsl_matrix_alloc(matrix1->size1,matrix2->size2);
  for (i=0; i<matrix1->size1; i++)
    for (j=0; j<matrix2->size2; j++) {
      temp_value = 0;
      for (k=0; k<matrix1->size2; k++)
	temp_value += gsl_matrix_get(matrix1, i, k) * gsl_matrix_get(matrix2, k, j);
      gsl_matrix_set(matrix_r, i, j, temp_value);
    }

  return matrix_r;
}

static realpoint_t matrix_mult_rp(gsl_matrix * matrix, realpoint_t rp) {

  realpoint_t return_rp;

  g_return_val_if_fail(matrix->size1 = NUM_AXIS, rp);
  g_return_val_if_fail(matrix->size2 = NUM_AXIS, rp);

  return_rp.x = \
    gsl_matrix_get(matrix, 0, 0)*rp.x + 
    gsl_matrix_get(matrix, 0, 1)*rp.y + 
    gsl_matrix_get(matrix, 0, 2)*rp.z;
  return_rp.y = \
    gsl_matrix_get(matrix, 1, 0)*rp.x + 
    gsl_matrix_get(matrix, 1, 1)*rp.y + 
    gsl_matrix_get(matrix, 1, 2)*rp.z;
  return_rp.z = \
    gsl_matrix_get(matrix, 2, 0)*rp.x + 
    gsl_matrix_get(matrix, 2, 1)*rp.y + 
    gsl_matrix_get(matrix, 2, 2)*rp.z;

  return return_rp;
}

/* this algorithm is derived from the review article:
   "Medical Image Registration", 
   Hill, Batchelor, Holden, and Hawkes Phys. Med. Biol 46 (2001) R1-R45
*/

realspace_t * alignment_calculate(volume_t * moving_vol, volume_t * fixed_vol, align_pts_t * pts) {

  realspace_t * coord_frame;
  guint count = 0;
  align_pts_t * moving_pts = NULL;
  align_pts_t * fixed_pts = NULL;
  align_pt_t * temp_pt;
  realpoint_t moving_centroid = zero_rp;
  realpoint_t fixed_centroid = zero_rp;
  realpoint_t temp_rp;
  realpoint_t offset_rp;
  realpoint_t translation_rp;
  realpoint_t axis[NUM_AXIS];
  gsl_matrix * fixed_matrix;
  gsl_matrix * moving_matrixt;
  gsl_matrix * matrix_a;
  gsl_matrix * matrix_v;
  gsl_matrix * matrix_r;
  gsl_matrix * matrix_temp;
  gsl_matrix * matrix_diag;
  gsl_permutation * permutation;
  gsl_vector * vector_s;
  gint i;
  gint signum;
  gdouble det;

  coord_frame = rs_init();

  g_return_val_if_fail(moving_vol != NULL, coord_frame);
  g_return_val_if_fail(fixed_vol != NULL, coord_frame);
  g_return_val_if_fail(pts != NULL, coord_frame);


  /* get the two lists of points */
  while (pts != NULL) {
    if (align_pts_includes_pt_by_name(moving_vol->align_pts, pts->align_pt->name) &&
	align_pts_includes_pt_by_name(fixed_vol->align_pts, pts->align_pt->name)) {
      temp_pt = align_pts_find_pt_by_name(moving_vol->align_pts, pts->align_pt->name);
      moving_pts = align_pts_add_pt(moving_pts, temp_pt);
      moving_centroid = rp_add(moving_centroid, 
			   realspace_alt_coord_to_base(temp_pt->point, moving_vol->coord_frame));

      temp_pt = align_pts_find_pt_by_name(fixed_vol->align_pts, pts->align_pt->name);
      fixed_pts = align_pts_add_pt(fixed_pts, temp_pt);
      fixed_centroid = rp_add(fixed_centroid, 
			  realspace_alt_coord_to_base(temp_pt->point, fixed_vol->coord_frame));
      count++;
    }
    pts = pts->next;
  }

  /* calculate the centroid of each point set */
  moving_centroid = rp_cmult((1.0/(gdouble) count), moving_centroid);
  fixed_centroid = rp_cmult((1.0/(gdouble) count), fixed_centroid);

  /* sanity check */
  if (count < 3) {
    g_warning("%s: Cannot perform an alignment with %d points, need at least 3",PACKAGE, count);
    moving_pts = align_pts_unref(moving_pts);
    fixed_pts = align_pts_unref(fixed_pts);
  }

  /* translate the points into data structures that GSL can handle */
  /* also demean the alignment points so the centroid of each set is the origin */
  /* the matrix are constructed such that each row is a point */
  /* moving_matrixt is the transpose of the moving matrix */
  fixed_matrix = gsl_matrix_alloc(count, NUM_AXIS);
  moving_matrixt = gsl_matrix_alloc(NUM_AXIS, count);
  for (i=0;i<count;i++) {
    temp_rp = rp_sub(realspace_alt_coord_to_base(moving_pts->align_pt->point, moving_vol->coord_frame),
		     moving_centroid);
    gsl_matrix_set(moving_matrixt, XAXIS, i, temp_rp.x);
    gsl_matrix_set(moving_matrixt, YAXIS, i, temp_rp.y);
    gsl_matrix_set(moving_matrixt, ZAXIS, i, temp_rp.z);

    temp_rp = rp_sub(realspace_alt_coord_to_base(fixed_pts->align_pt->point, fixed_vol->coord_frame),
		     fixed_centroid);
    gsl_matrix_set(fixed_matrix, i, XAXIS, temp_rp.x);
    gsl_matrix_set(fixed_matrix, i, YAXIS, temp_rp.y);
    gsl_matrix_set(fixed_matrix, i, ZAXIS, temp_rp.z);
    moving_pts = align_pts_remove_pt(moving_pts, moving_pts->align_pt);
    fixed_pts = align_pts_remove_pt(fixed_pts, fixed_pts->align_pt);
  }

  if ((moving_pts != NULL) || (fixed_pts != NULL)) {
    g_warning("%s: points lists not completely used in %s at %d", PACKAGE, __FILE__, __LINE__);
    gsl_matrix_free(fixed_matrix);
    gsl_matrix_free(moving_matrixt);
    return coord_frame;
  }

  /* calculate the correlation matrix */
  matrix_a = matrix_mult(moving_matrixt, fixed_matrix);
  gsl_matrix_free(fixed_matrix);
  gsl_matrix_free(moving_matrixt);

  /* get the singular value decomposition of the correlation matrix
     matrix_a = U*S*Vt
     note, that the function will place the value of U into matrix_a */
  matrix_v = gsl_matrix_alloc(NUM_AXIS, NUM_AXIS);
  vector_s = gsl_vector_alloc(NUM_AXIS);
  gsl_linalg_SV_decomp_jacobi(matrix_a, matrix_v, vector_s);
  gsl_vector_free(vector_s);

  /* get U transpose */
  gsl_matrix_transpose(matrix_a);

  /* figure out the determinant of V*Ut */
  matrix_temp = matrix_mult(matrix_v, matrix_a);
  permutation = gsl_permutation_alloc(NUM_AXIS);
  gsl_linalg_LU_decomp(matrix_temp, permutation, &signum);
  det = gsl_linalg_LU_det(matrix_temp, signum);
  gsl_permutation_free(permutation);
  gsl_matrix_free(matrix_temp);



  /* compute the rotation
   r = V * delta * Ut, where delta = diag(1,1,det(V,Ut))*/
  matrix_diag = gsl_matrix_alloc(NUM_AXIS, NUM_AXIS);
  gsl_matrix_set_identity (matrix_diag);
  gsl_matrix_set(matrix_diag, NUM_AXIS-1, NUM_AXIS-1, det);
  matrix_temp = matrix_mult(matrix_diag, matrix_a);
  matrix_r = matrix_mult(matrix_v, matrix_temp);

  gsl_matrix_free(matrix_temp);
  gsl_matrix_free(matrix_diag);
  gsl_matrix_free(matrix_a);
  gsl_matrix_free(matrix_v);

#if AMIDE_DEBUG		     
  g_print("determinante is %f\n",det);
  {
    gint z, y;

    g_print("rotation matrix:\n");
    for (z=0;z<NUM_AXIS;z++) {
      g_print("\t");
      for (y=0;y<NUM_AXIS;y++)
	g_print("%f\t",gsl_matrix_get(matrix_r, z, y));
      g_print("\n");
    }
  }
#endif


  /* figure out the new coordinate frame */
  for (i=0;i<NUM_AXIS;i++)
    axis[i]=matrix_mult_rp(matrix_r, rs_specific_axis(moving_vol->coord_frame, i));
  rs_set_axis(coord_frame, axis);

  /* and compute the new offset */
  translation_rp = rp_sub(fixed_centroid, matrix_mult_rp(matrix_r, moving_centroid));
  offset_rp = matrix_mult_rp(matrix_r, rs_offset(moving_vol->coord_frame));
  offset_rp = rp_add(offset_rp, translation_rp);
  rs_set_offset(coord_frame, offset_rp);
  
#if AMIDE_DEBUG		     
  rs_print("old coord frame", moving_vol->coord_frame);
  rs_print("new    coord frame   ", coord_frame);
#endif

  /* garbage collection */
  gsl_matrix_free(matrix_r);

  return coord_frame;
}


#endif /* AMIDE_LIBGSL_SUPPORT */
