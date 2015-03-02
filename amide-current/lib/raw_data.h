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

/* typedefs, structs, etc. */

typedef enum {UBYTE, SBYTE, 
	      USHORT_LE, SSHORT_LE, 
	      UINT_LE, SINT_LE, 
	      FLOAT_LE, DOUBLE_LE, 
	      USHORT_BE, SSHORT_BE, 
	      UINT_BE, SINT_BE,
	      FLOAT_BE, DOUBLE_BE,
	      ASCII,
	      NUM_DATA_FORMATS} data_format_t;


/* don't trust that these are going to work... most of them aren't tested */
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
#define AMIDE_FLOAT_FROM_LE(val) ((gfloat) (val))
#define AMIDE_FLOAT_TO_LE(val) (AMIDE_FLOAT_FROM_LE(val))
#define AMIDE_FLOAT_FROM_BE(val) ((gfloat) GUINT32_SWAP_LE_BE_CONSTANT(val))
#define AMIDE_FLOAT_TO_BE(val) (AMIDE_FLOAT_FROM_BE(val))
#define AMIDE_DOUBLE_FROM_LE(val) ((gdouble) (val))
#define AMIDE_DOUBLE_TO_LE(val) (AMIDE_DOUBLE_FROM_LE(val))
#define AMIDE_DOUBLE_FROM_BE(val) ((gdouble) GUINT64_SWAP_LE_BE_CONSTANT(val))
#define AMIDE_DOUBLE_TO_BE(val) (AMIDE_DOUBLE_FROM_BE(val))
#else /* BIG ENDIAN */
#define AMIDE_FLOAT_FROM_LE(val) ((gfloat) GUINT32_SWAP_LE_BE_CONSTANT(val))
#define AMIDE_FLOAT_TO_LE(val) (AMIDE_FLOAT_FROM_LE(val))
#define AMIDE_FLOAT_FROM_BE(val) ((gfloat) (val))
#define AMIDE_FLOAT_TO_BE(val) (AMIDE_FLOAT_FROM_BE(val))
#define AMIDE_DOUBLE_FROM_LE(val) ((gdouble) GUINT64_SWAP_LE_BE_CONSTANT(val))
#define AMIDE_DOUBLE_TO_LE(val) (AMIDE_DOUBLE_FROM_LE(val))
#define AMIDE_DOUBLE_FROM_BE(val) ((gdouble) (val))
#define AMIDE_DOUBLE_TO_BE(val) (AMIDE_DOUBLE_FROM_BE(val))
#endif

#define raw_data_calc_num_entries(dim, frames) ((dim).x * (dim).y * (dim).z * (frames))

/* external functions */
guint raw_data_calc_num_bytes(voxelpoint_t dim, 
			      guint num_frames, 
			      data_format_t data_format);
volume_t * raw_data_read_file(gchar * file_name, 
			      volume_t * raw_data_volume,
			      data_format_t data_format,
			      guint file_offset);

/* external variables */
extern gchar * data_format_names[];
extern guint data_sizes[];

#endif /* __RAW_DATA_H__ */





