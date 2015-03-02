/* data_set.h
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

#ifndef __DATA_SET_H__
#define __DATA_SET_H__

/* header files that are always needed with this file */
#include "xml.h"

/* typedefs, structs, etc. */

/* setup the types for various internal data formats */
/* data format is the formats that data can take in memory */
typedef enum {UBYTE, SBYTE, 
	      USHORT, SSHORT, 
	      UINT, SINT, 
	      FLOAT, DOUBLE, 
	      NUM_DATA_FORMATS} data_format_t;

typedef guint8 data_set_UBYTE_t;
typedef gint8 data_set_SBYTE_t;
typedef guint16 data_set_USHORT_t;
typedef gint16 data_set_SSHORT_t;
typedef guint32 data_set_UINT_t;
typedef gint32 data_set_SINT_t;
typedef gfloat data_set_FLOAT_t;
typedef gdouble data_set_DOUBLE_t;


/* the data_set structure */
typedef struct data_set_t { 

  /* parameters to save */
  voxelpoint_t dim;
  gpointer data;
  data_format_t format;
  
  /* parameters calculated at run time */
  guint reference_count;
} data_set_t;


/* -------- defines ----------- */
#define data_set_includes_voxel(ds, vox) (!(((vox).x < 0) || \
					    ((vox).y < 0) || \
					    ((vox).z < 0) || \
					    ((vox).t < 0) || \
					    ((vox).x >= (ds)->dim.x) || \
					    ((vox).y >= (ds)->dim.y) || \
					    ((vox).z >= (ds)->dim.z) || \
					    ((vox).t >= (ds)->dim.t)))
#define data_set_num_voxels(data_set) \
  ((data_set)->dim.x * (data_set)->dim.y * (data_set)->dim.z * (data_set)->dim.t)
#define data_set_size_data_mem(data_set) (data_set_num_voxels(data_set) * data_format_sizes[(data_set)->format])
#define data_set_get_data_mem(data_set) (g_malloc(data_set_size_data_mem(data_set)))


/* ------------ external functions ---------- */
data_set_t * data_set_free(data_set_t * data_set);
data_set_t * data_set_init(void);
gchar * data_set_write_xml(data_set_t * data_set, gchar * study_directory, gchar * data_set_name);
data_set_t * data_set_load_xml(gchar * data_set_xml_filename, const gchar * study_directory);
data_set_t * data_set_add_reference(data_set_t * data_set);

/* external variables */
extern guint data_format_sizes[];
extern gchar * data_format_names[];

/* variable type function declarations */
#include "data_set_UBYTE.h"
#include "data_set_SBYTE.h"
#include "data_set_USHORT.h"
#include "data_set_SSHORT.h"
#include "data_set_UINT.h"
#include "data_set_SINT.h"
#include "data_set_FLOAT.h"
#include "data_set_DOUBLE.h"

#endif /* __DATA_SET_H__ */

