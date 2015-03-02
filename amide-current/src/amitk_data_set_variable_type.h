/* amitk_data_set_variable_type.h - used to generate the different amitk_data_set_*.h files
 *
 * Part of amide - Amide's a Medical Image Data Examiner
 * Copyright (C) 2001-2002 Andy Loening
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


#ifndef __AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'__
#define __AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'__

/* header files that are always needed with this file */
#include "amitk_data_set.h"


/* defines */

/* translates to the contents of the voxel specified by voxelpoint i */
#define AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENT(data_set,i) \
 (*(AMITK_RAW_DATA_DOUBLE_`'m4_Scale_Dim`'_POINTER((data_set)->current_scaling, (i))) * \
  ((amide_data_t) (*(AMITK_RAW_DATA_`'m4_Variable_Type`'_POINTER((data_set)->raw_data,(i))))))

#define AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_SET_CONTENT(data_set,i,value) \
 (AMITK_RAW_DATA_`'m4_Variable_Type`'_SET_CONTENT((data_set)->raw_data, (i)) = value/ \
  (*(AMITK_RAW_DATA_DOUBLE_`'m4_Scale_Dim`'_POINTER((data_set)->current_scaling, (i)))))

/* function declarations */
void amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_calc_frame_max_min(AmitkDataSet * data_set);
void amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_calc_distribution(AmitkDataSet * data_set);
AmitkDataSet * amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_get_projection(AmitkDataSet * data_set,
									       const AmitkView view,
									       const guint frame);
AmitkDataSet * amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_get_cropped(const AmitkDataSet * data_set,
										const AmitkVoxel start,
										const AmitkVoxel end);
void amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_apply_kernel(const AmitkDataSet * data_set,
								       AmitkDataSet * filtered_ds,
								       const AmitkRawData * kernel);
void amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_filter_gaussian(const AmitkDataSet * data_set,
									  AmitkDataSet * filtered_ds,
									  const gint kernel_size,
									  const amide_real_t fwhm);
void amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_filter_median_3D(const AmitkDataSet * data_set,
									   AmitkDataSet * filtered_ds,
									   const AmitkVoxel kernel_dim);
void amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_filter_median_linear(const AmitkDataSet * data_set,
									       AmitkDataSet * filtered_ds,
									       const gint kernel_size);
AmitkDataSet * amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_get_slice(AmitkDataSet * data_set,
									      const amide_time_t requested_start,
									      const amide_time_t requested_duration,
									      const amide_real_t pixel_dim,
									      const AmitkVolume * slice_volume,
									      const gboolean need_calc_max_min);


#endif /* __AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'__ */







