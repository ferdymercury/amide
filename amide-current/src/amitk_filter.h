/* amitk_filter.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2002-2014 Andy Loening
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

#ifndef __AMITK_FILTER_H__
#define __AMITK_FILTER_H__

#include <glib-object.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* use GNU extensions, i.e. NaN */
#endif
#include <math.h>
#include "amitk_raw_data.h"
#ifdef AMIDE_LIBGSL_SUPPORT
#include <gsl/gsl_fft_complex.h>
#endif

G_BEGIN_DECLS

typedef enum {
  AMITK_FILTER_GAUSSIAN,
  AMITK_FILTER_MEDIAN_LINEAR,
  AMITK_FILTER_MEDIAN_3D,
  AMITK_FILTER_NUM
} AmitkFilter;

#define AMITK_FILTER_FFT_SIZE 64


AmitkRawData * amitk_filter_calculate_gaussian_kernel_complex(const AmitkVoxel kernel_size,
							      const AmitkPoint voxel_size,
							      const amide_real_t fwhm);

#ifdef AMIDE_LIBGSL_SUPPORT
void amitk_filter_3D_FFT(AmitkRawData * data, 
			 gsl_fft_complex_wavetable * wavetable,
			 gsl_fft_complex_workspace * workspace);
void amitk_filter_inverse_3D_FFT(AmitkRawData * data, 
				 gsl_fft_complex_wavetable * wavetable,
				 gsl_fft_complex_workspace * workspace);
void amitk_filter_complex_mult(AmitkRawData * data, AmitkRawData * kernel);
#endif
amide_data_t amitk_filter_find_median_by_partial_sort(amide_data_t * partial_sort_data, gint size);

const gchar * amitk_filter_get_name(const AmitkFilter filter);


G_END_DECLS

#endif /* __AMITK_FILTER_H__ */
