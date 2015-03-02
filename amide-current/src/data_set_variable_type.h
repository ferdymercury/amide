/* data_set_variable_type.h - used to generate the different data_set_*.h files
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


#ifndef __DATA_SET_`'m4_Internal_Data_Format`'__
#define __DATA_SET_`'m4_Internal_Data_Format`'__

/* header files that are always needed with this file */
#include "data_set.h"

/* defines */
#define DATA_SET_`'m4_Internal_Data_Format`'_0D_SCALING_POINTER(data_set,i) \
  (((data_set_`'m4_Internal_Data_Format`'_t *) (data_set)->data))
#define DATA_SET_`'m4_Internal_Data_Format`'_1D_SCALING_POINTER(data_set,i) \
  (((data_set_`'m4_Internal_Data_Format`'_t *) (data_set)->data)+(i).t)
#define DATA_SET_`'m4_Internal_Data_Format`'_2D_SCALING_POINTER(data_set,i) \
  (((data_set_`'m4_Internal_Data_Format`'_t *) (data_set)->data)+ \
    (i).z + \
    (i).t * ((data_set)->dim.z))
#define DATA_SET_`'m4_Internal_Data_Format`'_POINTER(data_set,i) \
  (((data_set_`'m4_Internal_Data_Format`'_t *) (data_set)->data)+ \
   ((i).x +  \
    (i).y * ((data_set)->dim.x) +  \
    (i).z * ((data_set)->dim.x) * ((data_set)->dim.y) +  \
    (i).t * ((data_set)->dim.x) * ((data_set)->dim.y) * ((data_set)->dim.z))) 

#define DATA_SET_`'m4_Internal_Data_Format`'_SET_CONTENT(data_set,i) \
  (*((data_set_`'m4_Internal_Data_Format`'_t *) DATA_SET_`'m4_Internal_Data_Format`'_POINTER((data_set),(i))))


/* function declarations */
data_set_t * data_set_`'m4_Internal_Data_Format`'_0D_SCALING_init(amide_data_t init_value);
void data_set_`'m4_Internal_Data_Format`'_initialize_data(data_set_t * data_set, 
							  data_set_`'m4_Internal_Data_Format`'_t init_value);

#endif /* __DATA_SET_`'m4_Internal_Data_Format`'__ */
