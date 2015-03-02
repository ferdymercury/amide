/* raw_data.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
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

#ifndef __RAW_DATA_H__
#define __RAW_DATA_H__


/* header files that are always needed with this file */
#include "data_set.h"
#include "volume.h"

/* raw_data_format is the formats that data can take on disk */
typedef enum {UBYTE_NE, SBYTE_NE, 
	      USHORT_LE, SSHORT_LE, 
	      UINT_LE, SINT_LE, 
	      FLOAT_LE, DOUBLE_LE, 
	      USHORT_BE, SSHORT_BE, 
	      UINT_BE, SINT_BE,
	      FLOAT_BE, DOUBLE_BE,
	      ASCII_NE,
	      NUM_RAW_DATA_FORMATS} raw_data_format_t;

/* external functions */
data_format_t raw_data_format_data(raw_data_format_t raw_data_format);
raw_data_format_t raw_data_format_raw(data_format_t data_format);
guint raw_data_calc_num_bytes(voxelpoint_t dim, 
			      raw_data_format_t raw_data_format);
data_set_t * raw_data_read_file(gchar * file_name, 
				data_set_t * raw_data_set,
				raw_data_format_t raw_data_format,
				guint file_offset);
volume_t * raw_data_read_volume(gchar * file_name, 
				volume_t * raw_data_volume,
				raw_data_format_t raw_data_format,
				guint file_offset);

/* external variables */
extern gchar * raw_data_format_names[];
extern guint raw_data_sizes[];

#endif /* __RAW_DATA_H__ */





