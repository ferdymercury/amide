/* raw_data.h
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

#ifndef __RAW_DATA_H__
#define __RAW_DATA_H__


/* header files that are always needed with this file */
#include "volume.h"

#define RAW_DATA_CONTENT(data, dim, i) ((data)[(i).x + (dim).x*(i).y + (dim).x*(dim).y*(i).z + (dim).x*(dim).y*(dim).z*(i).t])


/* glib doesn't define these for PDP */
#ifdef G_BIG_ENDIAN
#define GINT32_TO_PDP(val)      ((gint32) GUINT32_SWAP_BE_PDP (val))
#define GUINT32_TO_PDP(val)     ((guint32) GUINT32_SWAP_BE_PDP (val))
#else /* G_LITTLE_ENDIAN */
#define GINT32_TO_PDP(val)      ((gint32) GUINT32_SWAP_LE_PDP (val))
#define GUINT32_TO_PDP(val)     ((guint32) GUINT32_SWAP_LE_PDP (val))
#endif
#define GINT32_FROM_PDP(val)    (GINT32_TO_PDP (val))
#define GUINT32_FROM_PDP(val)   (GUINT32_TO_PDP (val))

/* raw_data_format is the formats that data can take on disk */
typedef enum {UBYTE_NE, SBYTE_NE, 
	      USHORT_LE, SSHORT_LE, 
	      UINT_LE, SINT_LE, 
	      FLOAT_LE, DOUBLE_LE, 
	      USHORT_BE, SSHORT_BE, 
	      UINT_BE, SINT_BE,
	      FLOAT_BE, DOUBLE_BE,
	      UINT_PDP, SINT_PDP,
	      FLOAT_PDP, 
	      ASCII_NE,
	      NUM_RAW_DATA_FORMATS} raw_data_format_t;

/* external functions */
data_format_t raw_data_format_data(raw_data_format_t raw_data_format);
raw_data_format_t raw_data_format_raw(data_format_t data_format);
guint raw_data_calc_num_bytes(voxelpoint_t dim, 
			      raw_data_format_t raw_data_format);
data_set_t * raw_data_read_file(const gchar * file_name, 
				raw_data_format_t raw_data_format,
				voxelpoint_t dim,
				guint file_offset);
volume_t * raw_data_read_volume(const gchar * file_name, 
				raw_data_format_t raw_data_format,
				voxelpoint_t data_dim,
				guint file_offset);

/* external variables */
extern gchar * raw_data_format_names[];
extern guint raw_data_sizes[];

#endif /* __RAW_DATA_H__ */





