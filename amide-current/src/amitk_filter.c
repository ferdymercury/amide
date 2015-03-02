/* amitk_filter.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2014 Andy Loening
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
#include <math.h>
#include "amitk_filter.h"
#include "amitk_type_builtins.h"
#ifdef AMIDE_LIBGSL_SUPPORT
#include <gsl/gsl_complex_math.h>
#endif

static inline amide_real_t gaussian(amide_real_t x, amide_real_t sigma) {
  return exp(-(x*x)/(2.0*sigma*sigma))/(sigma*sqrt(2*M_PI));
}

/* gaussian kernel is returned in a packad array (alternating real and imaginary parts 
   since the filter is entirely real, all imaginary parts are zero */
AmitkRawData * amitk_filter_calculate_gaussian_kernel_complex(const AmitkVoxel kernel_size,
							      const AmitkPoint voxel_size,
							      const amide_real_t fwhm) {

  AmitkVoxel i_voxel;
  AmitkPoint location;
  amide_real_t sigma;
  AmitkVoxel half;
  AmitkRawData * kernel;
  amide_real_t total;
  amide_real_t gaussian_value;

  g_return_val_if_fail((kernel_size.t == 1), NULL); /* can't filter over time */
  g_return_val_if_fail((kernel_size.g == 1), NULL); /* can't filter over gates */
  g_return_val_if_fail((kernel_size.z & 0x1), NULL); /* needs to be odd */
  g_return_val_if_fail((kernel_size.y & 0x1), NULL); 
  g_return_val_if_fail((kernel_size.x & 0x1), NULL); 

  if ((kernel = amitk_raw_data_new()) == NULL) {
    g_warning(_("couldn't allocate memory space for the kernel structure"));
    return NULL;
  }
  kernel->format = AMITK_FORMAT_DOUBLE;

  g_free(kernel->data);
  kernel->dim.t = kernel->dim.g = 1;
  kernel->dim.z = kernel->dim.y = AMITK_FILTER_FFT_SIZE;
  kernel->dim.x = 2*AMITK_FILTER_FFT_SIZE; /* real and complex */

  /* get mem for the kernel, initialized to 0 */
  if ((kernel->data = amitk_raw_data_get_data_mem0(kernel)) == NULL) {
    g_warning(_("Couldn't allocate memory space for the kernel data"));
    amitk_object_unref(kernel);
    return NULL;
  }

  sigma = fwhm/SIGMA_TO_FWHM;
  half.t = half.g = 0;
  half.z = kernel_size.z>>1;
  half.y = kernel_size.y>>1;
  half.x = kernel_size.x>>1;

  total = 0.0;

  i_voxel.t = i_voxel.g = 0;
  for (i_voxel.z = 0; i_voxel.z < kernel_size.z; i_voxel.z++) {
    location.z = voxel_size.z*(i_voxel.z-half.z);
    for (i_voxel.y = 0; i_voxel.y < kernel_size.y; i_voxel.y++) {
      location.y = voxel_size.y*(i_voxel.y-half.y);
      for (i_voxel.x = 0; i_voxel.x < 2*kernel_size.x; i_voxel.x+=2) {
	location.x = voxel_size.x*(i_voxel.x/2-half.x);
	gaussian_value = gaussian(point_mag(location), sigma);

	AMITK_RAW_DATA_DOUBLE_SET_CONTENT(kernel, i_voxel) = gaussian_value;
	total += gaussian_value;
      }
    }
  }

  /* renormalize, as the tails are cut, and we've discretized the gaussian */
  for (i_voxel.z = 0; i_voxel.z < kernel_size.z; i_voxel.z++) 
    for (i_voxel.y = 0; i_voxel.y < kernel_size.y; i_voxel.y++) 
      for (i_voxel.x = 0; i_voxel.x < 2*kernel_size.x; i_voxel.x+=2) 
	AMITK_RAW_DATA_DOUBLE_SET_CONTENT(kernel, i_voxel) /= total; 

  return kernel;
}

#ifdef AMIDE_LIBGSL_SUPPORT
void amitk_filter_3D_FFT(AmitkRawData * data, 
			   gsl_fft_complex_wavetable * wavetable,
			   gsl_fft_complex_workspace * workspace) {

  AmitkVoxel i_voxel;

  
  g_return_if_fail(AMITK_RAW_DATA_FORMAT(data) == AMITK_FORMAT_DOUBLE);
  g_return_if_fail(AMITK_RAW_DATA_DIM_T(data) == 1);
  g_return_if_fail(AMITK_RAW_DATA_DIM_G(data) == 1);
  g_return_if_fail(AMITK_RAW_DATA_DIM_Z(data) == AMITK_FILTER_FFT_SIZE);
  g_return_if_fail(AMITK_RAW_DATA_DIM_Y(data) == AMITK_FILTER_FFT_SIZE);
  g_return_if_fail(AMITK_RAW_DATA_DIM_X(data) == 2*AMITK_FILTER_FFT_SIZE);

  /* FFT in the X direction */
  i_voxel.t = i_voxel.g = i_voxel.x = 0;
  for (i_voxel.z=0; i_voxel.z < AMITK_RAW_DATA_DIM_Z(data); i_voxel.z++) 
    for (i_voxel.y=0; i_voxel.y < AMITK_RAW_DATA_DIM_Y(data); i_voxel.y++) 
      gsl_fft_complex_forward(AMITK_RAW_DATA_DOUBLE_POINTER(data,i_voxel),
			      1,
			      AMITK_FILTER_FFT_SIZE,
			      wavetable, workspace);

  /* FFT in the Y direction */
  i_voxel.t = i_voxel.g = i_voxel.y = 0;
  for (i_voxel.z=0; i_voxel.z < AMITK_RAW_DATA_DIM_Z(data); i_voxel.z++) 
    for (i_voxel.x=0; i_voxel.x < AMITK_RAW_DATA_DIM_X(data); i_voxel.x+=2) 
      gsl_fft_complex_forward(AMITK_RAW_DATA_DOUBLE_POINTER(data,i_voxel),
			      AMITK_FILTER_FFT_SIZE,
			      AMITK_FILTER_FFT_SIZE,
			     wavetable, workspace);

  /* FFT in the Z direction */
  i_voxel.t = i_voxel.g = i_voxel.z = 0;
    for (i_voxel.y=0; i_voxel.y < AMITK_RAW_DATA_DIM_Y(data); i_voxel.y++) 
      for (i_voxel.x=0; i_voxel.x < AMITK_RAW_DATA_DIM_X(data); i_voxel.x+=2) 
      gsl_fft_complex_forward(AMITK_RAW_DATA_DOUBLE_POINTER(data,i_voxel),
			      AMITK_FILTER_FFT_SIZE*AMITK_FILTER_FFT_SIZE,
			      AMITK_FILTER_FFT_SIZE,
			      wavetable, workspace);

  return;
}

void amitk_filter_inverse_3D_FFT(AmitkRawData * data, 
				   gsl_fft_complex_wavetable * wavetable,
				   gsl_fft_complex_workspace * workspace) {

  AmitkVoxel i_voxel;

  g_return_if_fail(AMITK_RAW_DATA_FORMAT(data) == AMITK_FORMAT_DOUBLE);
  g_return_if_fail(AMITK_RAW_DATA_DIM_T(data) == 1);
  g_return_if_fail(AMITK_RAW_DATA_DIM_G(data) == 1);
  g_return_if_fail(AMITK_RAW_DATA_DIM_Z(data) == AMITK_FILTER_FFT_SIZE);
  g_return_if_fail(AMITK_RAW_DATA_DIM_Y(data) == AMITK_FILTER_FFT_SIZE);
  g_return_if_fail(AMITK_RAW_DATA_DIM_X(data) == 2*AMITK_FILTER_FFT_SIZE);

  /* inverse FFT in the X direction */
  i_voxel.t = i_voxel.g = i_voxel.x = 0;
  for (i_voxel.z=0; i_voxel.z < AMITK_RAW_DATA_DIM_Z(data); i_voxel.z++) 
    for (i_voxel.y=0; i_voxel.y < AMITK_RAW_DATA_DIM_Y(data); i_voxel.y++) 
      gsl_fft_complex_inverse(AMITK_RAW_DATA_DOUBLE_POINTER(data,i_voxel),
			      1,
			      AMITK_FILTER_FFT_SIZE,
			      wavetable, workspace);

  /* inverse FFT in the Y direction */
  i_voxel.t = i_voxel.g = i_voxel.y = 0;
  for (i_voxel.z=0; i_voxel.z < AMITK_RAW_DATA_DIM_Z(data); i_voxel.z++) 
    for (i_voxel.x=0; i_voxel.x < AMITK_RAW_DATA_DIM_X(data); i_voxel.x+=2) 
      gsl_fft_complex_inverse(AMITK_RAW_DATA_DOUBLE_POINTER(data,i_voxel),
			      AMITK_FILTER_FFT_SIZE,
			      AMITK_FILTER_FFT_SIZE,
			      wavetable, workspace);

  /* inverse FFT in the Z direction */
  i_voxel.t = i_voxel.g = i_voxel.z = 0;
    for (i_voxel.y=0; i_voxel.y < AMITK_RAW_DATA_DIM_Y(data); i_voxel.y++) 
      for (i_voxel.x=0; i_voxel.x < AMITK_RAW_DATA_DIM_X(data); i_voxel.x+=2) 
	gsl_fft_complex_inverse(AMITK_RAW_DATA_DOUBLE_POINTER(data,i_voxel),
				AMITK_FILTER_FFT_SIZE*AMITK_FILTER_FFT_SIZE,
				AMITK_FILTER_FFT_SIZE,
				wavetable, workspace);
  return;
}

/* multiples the data by the kernel, both of which are in packed complex form */
void amitk_filter_complex_mult(AmitkRawData * data, AmitkRawData * kernel) {

  AmitkVoxel i_voxel;
  AmitkVoxel j_voxel;
  gsl_complex a,b;
  

  g_return_if_fail(AMITK_RAW_DATA_FORMAT(data) == AMITK_FORMAT_DOUBLE);
  g_return_if_fail(AMITK_RAW_DATA_FORMAT(kernel) == AMITK_FORMAT_DOUBLE);
  g_return_if_fail(AMITK_RAW_DATA_DIM_T(data) == AMITK_RAW_DATA_DIM_T(kernel));
  g_return_if_fail(AMITK_RAW_DATA_DIM_Z(data) == AMITK_RAW_DATA_DIM_Z(kernel));
  g_return_if_fail(AMITK_RAW_DATA_DIM_Y(data) == AMITK_RAW_DATA_DIM_Y(kernel));
  g_return_if_fail(AMITK_RAW_DATA_DIM_X(data) == AMITK_RAW_DATA_DIM_X(kernel));


  i_voxel.t = i_voxel.g = j_voxel.t = j_voxel.g = 0;
  for (i_voxel.z = 0, j_voxel.z=0; i_voxel.z < AMITK_RAW_DATA_DIM_Z(data); i_voxel.z++, j_voxel.z++)
    for (i_voxel.y = 0, j_voxel.y=0; i_voxel.y < AMITK_RAW_DATA_DIM_Y(data); i_voxel.y++, j_voxel.y++)
      for (i_voxel.x = 0, j_voxel.x=1; i_voxel.x < AMITK_RAW_DATA_DIM_X(data); i_voxel.x+=2, j_voxel.x+=2) {
	a.dat[0] = AMITK_RAW_DATA_DOUBLE_CONTENT(data, i_voxel);
	a.dat[1] = AMITK_RAW_DATA_DOUBLE_CONTENT(data, j_voxel);
	b.dat[0] = AMITK_RAW_DATA_DOUBLE_CONTENT(kernel, i_voxel);
	b.dat[1] = AMITK_RAW_DATA_DOUBLE_CONTENT(kernel, j_voxel);
	a = gsl_complex_mul(a,b);
	AMITK_RAW_DATA_DOUBLE_SET_CONTENT(data, i_voxel) = a.dat[0];
	AMITK_RAW_DATA_DOUBLE_SET_CONTENT(data, j_voxel) = a.dat[1];
      }


  return;
}

#endif


/* do a (destructive) partial sort of the given data to find median */
/* adapted and modified from Numerical Receipes in C, (who got it from Knuth, Vol 3?) */
/* median size needs to be odd for this to be strictly correct from a statistical stand point*/

#define SWAP(leftv, rightv) temp=(leftv); (leftv)=(rightv); (rightv=temp)
amide_data_t amitk_filter_find_median_by_partial_sort(amide_data_t * partial_sort_data, gint size) {

  gint left, right, mid;
  gint i_left, i_right;
  gint median_point;
  amide_data_t partition_value;
  amide_data_t temp;

  median_point = (size-1) >> 1;  /* not strictly correct for case of even size */
  
  left = 0;
  right = size-1;
  while (TRUE)

    if (right-left <= 2) { /* <= 3 elements left */

      if (right-left == 1) {/* 2 elements left to sort*/
	if (partial_sort_data[left] > partial_sort_data[right]) {
	  SWAP(partial_sort_data[left], partial_sort_data[right]);
	}
      } else if (right-left == 2){ /* 3 elements left to sort */
	if (partial_sort_data[left] > partial_sort_data[right]) {
	  SWAP(partial_sort_data[left], partial_sort_data[right]);
	}
	if (partial_sort_data[left+1] > partial_sort_data[right]) {
	  SWAP(partial_sort_data[left+1], partial_sort_data[right]);
	}
	if (partial_sort_data[left] > partial_sort_data[left+1]) {
	  SWAP(partial_sort_data[left], partial_sort_data[left+1]);
	}
      }
      return partial_sort_data[median_point];

    } else {
      mid = (left+right) >> 1;

      /* do a three way median with the leftmost, rightmost, and mid elements
	 to get a decent guess for a partitioning element.  This partioning element
	 ends up in left+1*/
      SWAP(partial_sort_data[mid], partial_sort_data[left+1]);
      if (partial_sort_data[left] > partial_sort_data[right]) {
	SWAP(partial_sort_data[left], partial_sort_data[right]);
      }
      if (partial_sort_data[left+1] > partial_sort_data[right]) {
	SWAP(partial_sort_data[left+1], partial_sort_data[right]);
      }
      if (partial_sort_data[left] > partial_sort_data[left+1]) {
	SWAP(partial_sort_data[left], partial_sort_data[left+1]);
      }
      
      
      partition_value = partial_sort_data[left+1];
      i_left=left+1;
      i_right = right;

      while (TRUE) {

	/* find an element > partition_value */
	do i_left++; while (partial_sort_data[i_left] < partition_value);

	/* find an element < partition value */
	do i_right--; while (partial_sort_data[i_right] > partition_value);

	if (i_right < i_left) break; /* pointers crossed */

	SWAP(partial_sort_data[i_left], partial_sort_data[i_right]);
      }

      partial_sort_data[left+1] = partial_sort_data[i_right];
      partial_sort_data[i_right] = partition_value;

      if (i_right >= median_point) right = i_right-1;
      if (i_right <= median_point) left = i_left;

    }

}



const gchar * amitk_filter_get_name(const AmitkFilter filter) {
  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_FILTER);
  enum_value = g_enum_get_value(enum_class, filter);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

