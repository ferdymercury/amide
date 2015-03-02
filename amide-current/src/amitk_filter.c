/* amitk_filter.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2002 Andy Loening
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

#include "amide_config.h"
#include <math.h>
#include "amitk_filter.h"
#include "amitk_type_builtins.h"


inline amide_real_t gaussian(amide_real_t x, amide_real_t sigma) {
  
  return exp(-(x*x)/(2.0*sigma*sigma))/(sigma*sqrt(2*M_PI));

}

AmitkRawData * amitk_filter_calculate_gaussian_kernel(const gint kernel_size,
						      const AmitkPoint voxel_size,
						      const amide_real_t fwhm,
						      const AmitkAxis axis) {

  AmitkVoxel i_voxel;
  amide_real_t sigma;
  gint i;
  gint middle;
  amide_real_t spacing;
  AmitkRawData * kernel;
  amide_real_t total;
  amide_real_t gaussian_value;

  g_return_val_if_fail((kernel_size & 0x1), NULL); /* needs to be odd */

  if ((kernel = amitk_raw_data_new()) == NULL) {
    g_warning("couldn't allocate space for the kernel structure");
    return NULL;
  }
  kernel->format = AMITK_FORMAT_DOUBLE;

  g_free(kernel->data);
  kernel->dim = one_voxel;

  switch(axis) {
  case AMITK_AXIS_X:
    spacing = voxel_size.x;
    kernel->dim.x = kernel_size;
    break;
  case AMITK_AXIS_Y:
    spacing = voxel_size.y;
    kernel->dim.y = kernel_size;
    break;
  case AMITK_AXIS_Z:
    spacing = voxel_size.z;
    kernel->dim.z = kernel_size;
    break;
  default:
    g_warning("unhandled case in %s at %d", __FILE__, __LINE__);
    spacing = 0.0;
    break;
  }

  if ((kernel->data = amitk_raw_data_get_data_mem(kernel)) == NULL) {
    g_warning("Couldn't allocate space for the kernel data");
    g_object_unref(kernel);
    return NULL;
  }

  sigma = fwhm/sqrt(log(4));
  middle = kernel_size>>1;

  i_voxel.t = 0;
  i = 0;
  total = 0.0;
  for (i_voxel.z = 0; i_voxel.z < kernel->dim.z; i_voxel.z++)
    for (i_voxel.y = 0; i_voxel.y < kernel->dim.y; i_voxel.y++)
      for (i_voxel.x = 0; i_voxel.x < kernel->dim.x; i_voxel.x++) {
	gaussian_value = gaussian(spacing*(i-middle), sigma);
	AMITK_RAW_DATA_DOUBLE_SET_CONTENT(kernel, i_voxel) = gaussian_value;
	total += gaussian_value;
	i++;
      }

  /* renormalize, as the tails are cut, and we've discretized the gaussian */
  for (i_voxel.z = 0; i_voxel.z < kernel->dim.z; i_voxel.z++)
    for (i_voxel.y = 0; i_voxel.y < kernel->dim.y; i_voxel.y++)
      for (i_voxel.x = 0; i_voxel.x < kernel->dim.x; i_voxel.x++)
	AMITK_RAW_DATA_DOUBLE_SET_CONTENT(kernel, i_voxel) = 
	  AMITK_RAW_DATA_DOUBLE_CONTENT(kernel, i_voxel)/total;


  return kernel;
}

const gchar * amitk_filter_get_name(const AmitkFilter filter) {
  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_FILTER);
  enum_value = g_enum_get_value(enum_class, filter);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

