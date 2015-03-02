/* alignment.c
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
#ifdef AMIDE_LIBGSL_SUPPORT
#include <sys/stat.h>
#include <string.h>
#include <glib.h>
#include <gsl/gsl_linalg.h>
#include "alignment_procrustes.h"
#include "amitk_fiducial_mark.h"

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

static AmitkPoint matrix_mult_point(gsl_matrix * matrix, AmitkPoint rp) {

  AmitkPoint return_point;

  g_return_val_if_fail(matrix->size1 == AMITK_AXIS_NUM, rp);
  g_return_val_if_fail(matrix->size2 == AMITK_AXIS_NUM, rp);

  return_point.x = \
    gsl_matrix_get(matrix, 0, 0)*rp.x + 
    gsl_matrix_get(matrix, 0, 1)*rp.y + 
    gsl_matrix_get(matrix, 0, 2)*rp.z;
  return_point.y = \
    gsl_matrix_get(matrix, 1, 0)*rp.x + 
    gsl_matrix_get(matrix, 1, 1)*rp.y + 
    gsl_matrix_get(matrix, 1, 2)*rp.z;
  return_point.z = \
    gsl_matrix_get(matrix, 2, 0)*rp.x + 
    gsl_matrix_get(matrix, 2, 1)*rp.y + 
    gsl_matrix_get(matrix, 2, 2)*rp.z;

  return return_point;
}


/* this is the procrustes rigid registration algorithm, derived from the review article:
   "Medical Image Registration", 
   Hill, Batchelor, Holden, and Hawkes Phys. Med. Biol 46 (2001) R1-R45
*/

AmitkSpace * alignment_procrustes(AmitkDataSet * moving_ds, AmitkDataSet * fixed_ds, GList * marks,
				  gdouble *pointer_fiducial_reference_error) {

  AmitkSpace * transform_space=NULL;
  guint count = 0;
  GList * moving_fiducial_marks = NULL;
  GList * fixed_fiducial_marks = NULL;
  GList * moving_temp_list;
  GList * fixed_temp_list;
  AmitkObject * moving_fiducial_mark;
  AmitkObject * fixed_fiducial_mark;
  AmitkFiducialMark * fiducial_mark;
  AmitkPoint moving_centroid = zero_point;
  AmitkPoint fixed_centroid = zero_point;
  AmitkPoint temp_point;
  AmitkPoint shift;
  AmitkPoint offset_shift;
  AmitkAxes axis;
  AmitkPoint moving_center;
  AmitkPoint fixed_center;
  gsl_matrix * fixed_matrix=NULL;
  gsl_matrix * moving_matrixt=NULL;
  gsl_matrix * matrix_a;
  gsl_matrix * matrix_v;
  gsl_matrix * matrix_r=NULL;
  gsl_matrix * matrix_temp;
  gsl_matrix * matrix_diag;
  gsl_permutation * permutation;
  gsl_vector * vector_s;
  gsl_vector * vector_w;
  gint i;
  gint signum;
  gdouble det;
  gdouble fre;
  gint status;
  const char * mark_name;

  g_return_val_if_fail(AMITK_IS_DATA_SET(moving_ds), NULL);
  g_return_val_if_fail(AMITK_IS_DATA_SET(fixed_ds), NULL);
  g_return_val_if_fail(marks != NULL, NULL);


  /* get the two lists of points */
  while (marks != NULL) {
    fiducial_mark = marks->data;
    if (AMITK_IS_FIDUCIAL_MARK(fiducial_mark) || AMITK_IS_VOLUME(fiducial_mark)) {
      mark_name = AMITK_OBJECT_NAME(fiducial_mark);

      moving_fiducial_mark = amitk_objects_find_object_by_name(AMITK_OBJECT_CHILDREN(moving_ds), mark_name);
      fixed_fiducial_mark = amitk_objects_find_object_by_name(AMITK_OBJECT_CHILDREN(fixed_ds), mark_name);
      
      if ((AMITK_IS_FIDUCIAL_MARK(moving_fiducial_mark) || AMITK_IS_VOLUME(moving_fiducial_mark)) &&
	  (AMITK_IS_FIDUCIAL_MARK(fixed_fiducial_mark) || AMITK_IS_VOLUME(moving_fiducial_mark))) {

	moving_fiducial_marks = g_list_append(moving_fiducial_marks, amitk_object_ref(moving_fiducial_mark));
	if (AMITK_IS_FIDUCIAL_MARK(moving_fiducial_mark))
	  moving_center = AMITK_FIDUCIAL_MARK_GET(moving_fiducial_mark);
	else /* (AMITK_IS_VOLUME(moving_fiducial_mark) - maybe someday data sets will be fiducials */
	  moving_center = amitk_volume_get_center(AMITK_VOLUME(moving_fiducial_mark));
	moving_centroid = point_add(moving_centroid, moving_center);
	
	fixed_fiducial_marks = g_list_append(fixed_fiducial_marks, amitk_object_ref(fixed_fiducial_mark));
	if (AMITK_IS_FIDUCIAL_MARK(fixed_fiducial_mark))
	  fixed_center = AMITK_FIDUCIAL_MARK_GET(fixed_fiducial_mark);
	else /* (AMITK_IS_VOLUME(fixed_fiducial_mark) - maybe someday data sets will be fiducials */
	  fixed_center = amitk_volume_get_center(AMITK_VOLUME(fixed_fiducial_mark));
	fixed_centroid = point_add(fixed_centroid, fixed_center);

	count++;
      }
    }
    marks = marks->next;
  }

  /* calculate the centroid of each point set */
  moving_centroid = point_cmult((1.0/(gdouble) count), moving_centroid);
  fixed_centroid = point_cmult((1.0/(gdouble) count), fixed_centroid);

  /* sanity check */
  if (count < 3) {
    g_warning(_("Cannot perform an alignment with %d points, need at least 3"),count);
    goto ending;
  }

  /* translate the points into data structures that GSL can handle */
  /* also demean the alignment points so the centroid of each set is the origin */
  /* the matrix are constructed such that each row is a point */
  /* moving_matrixt is the transpose of the moving matrix */
  moving_matrixt = gsl_matrix_alloc(AMITK_AXIS_NUM, count);
  fixed_matrix = gsl_matrix_alloc(count, AMITK_AXIS_NUM);

  moving_temp_list = moving_fiducial_marks;
  fixed_temp_list = fixed_fiducial_marks;

  for (i=0;i<count;i++) {

    moving_fiducial_mark = moving_temp_list->data;
    moving_temp_list = moving_temp_list->next;

    fixed_fiducial_mark = fixed_temp_list->data;
    fixed_temp_list = fixed_temp_list->next;

    if (AMITK_IS_FIDUCIAL_MARK(moving_fiducial_mark))
      moving_center = AMITK_FIDUCIAL_MARK_GET(moving_fiducial_mark);
    else /* (AMITK_IS_VOLUME(moving_fiducial_mark) - maybe someday data sets will be fiducials */
      moving_center = amitk_volume_get_center(AMITK_VOLUME(moving_fiducial_mark));

    temp_point = point_sub(moving_center, moving_centroid);
    gsl_matrix_set(moving_matrixt, AMITK_AXIS_X, i, temp_point.x);
    gsl_matrix_set(moving_matrixt, AMITK_AXIS_Y, i, temp_point.y);
    gsl_matrix_set(moving_matrixt, AMITK_AXIS_Z, i, temp_point.z);

    if (AMITK_IS_FIDUCIAL_MARK(fixed_fiducial_mark))
      fixed_center = AMITK_FIDUCIAL_MARK_GET(fixed_fiducial_mark);
    else /* (AMITK_IS_VOLUME(fixed_fiducial_mark) - maybe someday data sets will be fiducials */
      fixed_center = amitk_volume_get_center(AMITK_VOLUME(fixed_fiducial_mark));

    temp_point = point_sub(fixed_center,fixed_centroid);
    gsl_matrix_set(fixed_matrix, i, AMITK_AXIS_X, temp_point.x);
    gsl_matrix_set(fixed_matrix, i, AMITK_AXIS_Y, temp_point.y);
    gsl_matrix_set(fixed_matrix, i, AMITK_AXIS_Z, temp_point.z);

  }

  if ((fixed_temp_list != NULL) || (moving_temp_list != NULL)) {
    g_warning(_("points lists not completely used in %s at %d"), __FILE__, __LINE__);
    goto ending;
  }

  /* calculate the correlation matrix */
  matrix_a = matrix_mult(moving_matrixt, fixed_matrix);

  /* get the singular value decomposition of the correlation matrix -> matrix_a = U*S*Vt
     notes: 
        the function will place the value of U into matrix_a 
        gsl_linalg_SV_decomp_jacobi will return an unsuitable matrix_v, don't use it 
  */
  matrix_v = gsl_matrix_alloc(AMITK_AXIS_NUM, AMITK_AXIS_NUM);
  vector_s = gsl_vector_alloc(AMITK_AXIS_NUM);
  vector_w = gsl_vector_alloc(AMITK_AXIS_NUM);
  status = gsl_linalg_SV_decomp(matrix_a, matrix_v, vector_s, vector_w);
  if (status != 0) g_warning(_("SV decomp returned error: %s"), gsl_strerror(status));
  gsl_vector_free(vector_s);
  gsl_vector_free(vector_w);

  /* get U transpose */
  status = gsl_matrix_transpose(matrix_a);
  if (status != 0) g_warning(_("transpose returned error: %s"), gsl_strerror(status));

  /* figure out the determinant of V*Ut */
  matrix_temp = matrix_mult(matrix_v, matrix_a);
  permutation = gsl_permutation_alloc(AMITK_AXIS_NUM);
  status = gsl_linalg_LU_decomp(matrix_temp, permutation, &signum);
  if (status != 0) g_warning(_("LU decomp returned error: %s"), gsl_strerror(status));
  det = gsl_linalg_LU_det(matrix_temp, signum);
  g_return_val_if_fail(fabs(det) > 0.1, NULL); /* needs to be -1 or 1 */
  gsl_permutation_free(permutation);
  gsl_matrix_free(matrix_temp);



  /* compute the rotation
   r = V * delta * Ut, where delta = diag(1,1,det(V,Ut))*/
  matrix_diag = gsl_matrix_alloc(AMITK_AXIS_NUM, AMITK_AXIS_NUM);
  gsl_matrix_set_identity (matrix_diag);
  if (fabs(det) > 0.1) 
    gsl_matrix_set(matrix_diag, AMITK_AXIS_NUM-1, AMITK_AXIS_NUM-1, det);
  matrix_temp = matrix_mult(matrix_diag, matrix_a);
  matrix_r = matrix_mult(matrix_v, matrix_temp);

  gsl_matrix_free(matrix_temp);
  gsl_matrix_free(matrix_diag);
  gsl_matrix_free(matrix_a);
  gsl_matrix_free(matrix_v);

  transform_space = amitk_space_new();

  /* transfer over the rotation matrix */
  for (i=0;i<AMITK_AXIS_NUM;i++){
    axis[i].x = gsl_matrix_get(matrix_r, 0, i);
    axis[i].y = gsl_matrix_get(matrix_r, 1, i);
    axis[i].z = gsl_matrix_get(matrix_r, 2, i);
  }
  amitk_space_set_axes(transform_space, axis, AMITK_SPACE_OFFSET(transform_space));

  /* and figure out the shift: t = fixed_centroid - R*moving_centroid,
     but we also need to compensate because amitk_space_transform will rotate
     relative to the moving_ds's offset, not the fixed_centroid (hence offset_shift) */
  shift = point_sub(fixed_centroid, matrix_mult_point(matrix_r, moving_centroid));
  offset_shift = matrix_mult_point(matrix_r,AMITK_SPACE_OFFSET(moving_ds));
  offset_shift = point_sub(offset_shift, AMITK_SPACE_OFFSET(moving_ds));
  shift = point_add(offset_shift, shift);
  amitk_space_set_offset(transform_space, shift); 

#if AMIDE_DEBUG		     
  g_print("determinante is %f\n",det);
  amitk_space_print(transform_space, "transform space");
#endif

  /* calculate fiducial reference error */
  if (pointer_fiducial_reference_error != NULL) {
    AmitkSpace * new_space = NULL;
    fre = 0.0;

    new_space = amitk_space_copy(AMITK_SPACE(moving_ds));
    amitk_space_transform(new_space, transform_space);

    moving_temp_list = moving_fiducial_marks;
    fixed_temp_list = fixed_fiducial_marks;
    
    for (i=0;i<count;i++) {
      moving_fiducial_mark = moving_temp_list->data;
      moving_temp_list = moving_temp_list->next;
      fixed_fiducial_mark = fixed_temp_list->data;
      fixed_temp_list = fixed_temp_list->next;

      if (AMITK_IS_FIDUCIAL_MARK(moving_fiducial_mark))
	moving_center = AMITK_FIDUCIAL_MARK_GET(moving_fiducial_mark);
      else /* (AMITK_IS_VOLUME(moving_fiducial_mark) - maybe someday data sets will be fiducials */
	moving_center = amitk_volume_get_center(AMITK_VOLUME(moving_fiducial_mark));
      
      if (AMITK_IS_FIDUCIAL_MARK(fixed_fiducial_mark))
	fixed_center = AMITK_FIDUCIAL_MARK_GET(fixed_fiducial_mark);
      else /* (AMITK_IS_VOLUME(fixed_fiducial_mark) - maybe someday data sets will be fiducials */
	fixed_center = amitk_volume_get_center(AMITK_VOLUME(fixed_fiducial_mark));

      moving_center = amitk_space_b2s(AMITK_SPACE(moving_ds), moving_center);
      moving_center = amitk_space_s2b(new_space,moving_center);
      fre += point_mag(point_sub(moving_center, fixed_center));
    }

    fre = fre/((gdouble) count);
    *pointer_fiducial_reference_error = fre;

    if (new_space != NULL) g_object_unref(new_space);
  }
 ending:

  /* garbage collection */

  if (matrix_r != NULL)
    gsl_matrix_free(matrix_r);

  if (moving_fiducial_marks != NULL)
    amitk_objects_unref(moving_fiducial_marks);

  if (fixed_fiducial_marks != NULL)
    amitk_objects_unref(fixed_fiducial_marks);

  if (fixed_matrix != NULL)
    gsl_matrix_free(fixed_matrix);

  if (moving_matrixt != NULL)
    gsl_matrix_free(moving_matrixt);



  return transform_space;
}


#endif /* AMIDE_LIBGSL_SUPPORT */
