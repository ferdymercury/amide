/* data_set_variable_type.c - used to generate the different data_set_*.c files
 *
 * Part of amide - Amide's a Medical Image Data Examiner
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
#include <math.h>
#include <glib.h>
#include "data_set_`'m4_Variable_Type`'.h"

data_set_t * data_set_`'m4_Variable_Type`'_0D_SCALING_init(amide_data_t init_value) {

  data_set_t * temp_data_set;
  voxelpoint_t i;

  temp_data_set = data_set_init();
  i = voxelpoint_zero;

  /* put in our 0D values */
  temp_data_set->dim.x = temp_data_set->dim.y = temp_data_set->dim.z = temp_data_set->dim.t = 1;
  temp_data_set->format = `'m4_Variable_Type`';
  temp_data_set->data = data_set_get_data_mem(temp_data_set);
  (*DATA_SET_`'m4_Variable_Type`'_0D_SCALING_POINTER(temp_data_set, i)) = init_value;

  return temp_data_set;
}

data_set_t * data_set_`'m4_Variable_Type`'_2D_init(amide_data_t init_value, 
						   intpoint_t y_dim,
						   intpoint_t x_dim) {

  data_set_t * temp_data_set;

  temp_data_set = data_set_init();

  /* put in our 0D values */
  temp_data_set->dim.x = x_dim;
  temp_data_set->dim.y = y_dim;
  temp_data_set->dim.z = temp_data_set->dim.t = 1;
  temp_data_set->format = `'m4_Variable_Type`';
  temp_data_set->data = data_set_get_data_mem(temp_data_set);

  data_set_`'m4_Variable_Type`'_initialize_data(temp_data_set, init_value);

  return temp_data_set;
}


void data_set_`'m4_Variable_Type`'_initialize_data(data_set_t * data_set, 
						   data_set_`'m4_Variable_Type`'_t init_value) {

  voxelpoint_t i;

  for (i.t = 0; i.t < data_set->dim.t; i.t++)
    for (i.z = 0; i.z < data_set->dim.z; i.z++) 
      for (i.y = 0; i.y < data_set->dim.y; i.y++) 
	for (i.x = 0; i.x < data_set->dim.x; i.x++) 
	  DATA_SET_`'m4_Variable_Type`'_SET_CONTENT(data_set,i)=0.0;

  return;
}
