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
#define AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_CONTENTS(data_set,i) \
 (*(AMITK_RAW_DATA_FLOAT_`'m4_Scale_Dim`'_POINTER((data_set)->current_scaling, (i))) * \
  ((amide_data_t) (*(AMITK_RAW_DATA_`'m4_Variable_Type`'_POINTER((data_set)->raw_data,(i))))))


/* function declarations */
void amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_calc_frame_max_min(AmitkDataSet * data_set);
void amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_calc_distribution(AmitkDataSet * data_set);
AmitkDataSet * amitk_data_set_`'m4_Variable_Type`'_`'m4_Scale_Dim`'_get_slice(AmitkDataSet * data_set,
									      const amide_time_t requested_start,
									      const amide_time_t requested_duration,
									      const AmitkPoint  requested_voxel_size,
									      const AmitkVolume * slice_volume,
									      const AmitkInterpolation interpolation,
									      const gboolean need_calc_max_min);


#endif /* __AMITK_DATA_SET_`'m4_Variable_Type`'_`'m4_Scale_Dim`'__ */







