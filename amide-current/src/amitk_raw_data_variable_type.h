/* amitk_raw_data_variable_type.h - used to generate the different amitk_raw_data_*.h files
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


#ifndef __AMITK_RAW_DATA_`'m4_Variable_Type`'__
#define __AMITK_RAW_DATA_`'m4_Variable_Type`'__

/* header files that are always needed with this file */
#include "amitk_raw_data.h"

/* defines */
#define AMITK_RAW_DATA_`'m4_Variable_Type`'_0D_SCALING_POINTER(amitk_raw_data,i) \
  (((amitk_format_`'m4_Variable_Type`'_t *) (amitk_raw_data)->data))
#define AMITK_RAW_DATA_`'m4_Variable_Type`'_1D_SCALING_POINTER(amitk_raw_data,i) \
  (((amitk_format_`'m4_Variable_Type`'_t *) (amitk_raw_data)->data)+(i).t)
#define AMITK_RAW_DATA_`'m4_Variable_Type`'_2D_SCALING_POINTER(amitk_raw_data,i) \
  (((amitk_format_`'m4_Variable_Type`'_t *) (amitk_raw_data)->data)+ \
    (i).z + \
    (i).t * ((amitk_raw_data)->dim.z))
#define AMITK_RAW_DATA_`'m4_Variable_Type`'_POINTER(amitk_raw_data,i) \
  (((amitk_format_`'m4_Variable_Type`'_t *) (amitk_raw_data)->data)+ \
   ((i).x +  \
    (i).y * ((amitk_raw_data)->dim.x) +  \
    (i).z * ((amitk_raw_data)->dim.x) * ((amitk_raw_data)->dim.y) +  \
    (i).t * ((amitk_raw_data)->dim.x) * ((amitk_raw_data)->dim.y) * ((amitk_raw_data)->dim.z))) 

#define AMITK_RAW_DATA_`'m4_Variable_Type`'_3D_POINTER(amitk_raw_data,iz,iy,ix) \
  (((amitk_format_`'m4_Variable_Type`'_t *) (amitk_raw_data)->data)+ \
   ((ix) +  \
    (iy) * ((amitk_raw_data)->dim.x) +  \
    (iz) * ((amitk_raw_data)->dim.x) * ((amitk_raw_data)->dim.y)))

#define AMITK_RAW_DATA_`'m4_Variable_Type`'_2D_POINTER(amitk_raw_data,iy,ix) \
  (((amitk_format_`'m4_Variable_Type`'_t *) (amitk_raw_data)->data)+ \
   ((ix) + \
    (iy) * ((amitk_raw_data)->dim.x)))

#define AMITK_RAW_DATA_`'m4_Variable_Type`'_SET_CONTENT(amitk_raw_data,i) \
  (*(AMITK_RAW_DATA_`'m4_Variable_Type`'_POINTER((amitk_raw_data),(i))))

#define AMITK_RAW_DATA_`'m4_Variable_Type`'_CONTENT(amitk_raw_data,i) \
  (*(AMITK_RAW_DATA_`'m4_Variable_Type`'_POINTER((amitk_raw_data),(i))))

//  (*((amitk_format_`'m4_Variable_Type`'_t *) AMITK_RAW_DATA_`'m4_Variable_Type`'_POINTER((amitk_raw_data),(i))))


/* function declarations */
AmitkRawData * amitk_raw_data_`'m4_Variable_Type`'_0D_SCALING_init(amitk_format_`'m4_Variable_Type`'_t init_value);
AmitkRawData * amitk_raw_data_`'m4_Variable_Type`'_2D_init(amitk_format_`'m4_Variable_Type`'_t init_value, 
							   amide_intpoint_t y_dim,
							   amide_intpoint_t x_dim);
AmitkRawData * amitk_raw_data_`'m4_Variable_Type`'_3D_init(amitk_format_`'m4_Variable_Type`'_t init_value, 
							   amide_intpoint_t z_dim,
							   amide_intpoint_t y_dim,
							   amide_intpoint_t x_dim);
void amitk_raw_data_`'m4_Variable_Type`'_initialize_data(AmitkRawData * amitk_raw_data, 
							 amitk_format_`'m4_Variable_Type`'_t init_value);

#endif /* __AMITK_RAW_DATA_`'m4_Variable_Type`'__ */





