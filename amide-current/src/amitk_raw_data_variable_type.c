/* amitk_raw_data_variable_type.c - used to generate the different amitk_raw_data_*.c files
 *
 * Part of amide - Amide's a Medical Image Data Examiner
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
#include <glib.h>
#include "amitk_raw_data_`'m4_Variable_Type`'.h"

AmitkRawData * amitk_raw_data_`'m4_Variable_Type`'_0D_SCALING_init(amitk_format_`'m4_Variable_Type`'_t init_value) {

  AmitkRawData * temp_amitk_raw_data;
  AmitkVoxel i;

  temp_amitk_raw_data = amitk_raw_data_new_with_data(AMITK_FORMAT_`'m4_Variable_Type`', one_voxel);
  g_return_val_if_fail(temp_amitk_raw_data != NULL, NULL);
  i = zero_voxel;

  (*AMITK_RAW_DATA_`'m4_Variable_Type`'_0D_SCALING_POINTER(temp_amitk_raw_data, i)) = init_value;

  return temp_amitk_raw_data;
}

void amitk_raw_data_`'m4_Variable_Type`'_initialize_data(AmitkRawData * amitk_raw_data, 
						   amitk_format_`'m4_Variable_Type`'_t init_value) {

  AmitkVoxel i;

  for (i.t = 0; i.t < amitk_raw_data->dim.t; i.t++)
    for (i.g = 0; i.g < amitk_raw_data->dim.g; i.g++) 
      for (i.z = 0; i.z < amitk_raw_data->dim.z; i.z++) 
	for (i.y = 0; i.y < amitk_raw_data->dim.y; i.y++) 
	  for (i.x = 0; i.x < amitk_raw_data->dim.x; i.x++) 
	    AMITK_RAW_DATA_`'m4_Variable_Type`'_SET_CONTENT(amitk_raw_data,i)=init_value;

  return;
}
