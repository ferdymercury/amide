/* amitk_raw_data.h
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

#ifndef __AMITK_RAW_DATA_H__
#define __AMITK_RAW_DATA_H__

/* header files that are always needed with this file */
#include <glib-object.h>
#include "amitk_point.h"

G_BEGIN_DECLS

#define	AMITK_TYPE_RAW_DATA		  (amitk_raw_data_get_type ())
#define AMITK_RAW_DATA(object)		  (G_TYPE_CHECK_INSTANCE_CAST ((object), AMITK_TYPE_RAW_DATA, AmitkRawData))
#define AMITK_RAW_DATA_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_RAW_DATA, AmitkRawDataClass))
#define AMITK_IS_RAW_DATA(object)	  (G_TYPE_CHECK_INSTANCE_TYPE ((object), AMITK_TYPE_RAW_DATA))
#define AMITK_IS_RAW_DATA_CLASS(klass)	  (G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_RAW_DATA))
#define	AMITK_RAW_DATA_GET_CLASS(object)  (G_TYPE_CHECK_GET_CLASS ((object), AMITK_TYPE_RAW_DATA, AmitkRawDataClass))


#define AMITK_RAW_DATA_FORMAT(rd)         (AMITK_RAW_DATA(rd)->format)
#define AMITK_RAW_DATA_DIM(rd)            (AMITK_RAW_DATA(rd)->dim)
#define AMITK_RAW_DATA_DIM_X(rd)          (AMITK_RAW_DATA(rd)->dim.x)
#define AMITK_RAW_DATA_DIM_Y(rd)          (AMITK_RAW_DATA(rd)->dim.y)
#define AMITK_RAW_DATA_DIM_Z(rd)          (AMITK_RAW_DATA(rd)->dim.z)
#define AMITK_RAW_DATA_DIM_T(rd)          (AMITK_RAW_DATA(rd)->dim.t)

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

/* setup the types for various internal data formats */

/*the formats that data can take in memory */
typedef enum {
  AMITK_FORMAT_UBYTE, 
  AMITK_FORMAT_SBYTE, 
  AMITK_FORMAT_USHORT, 
  AMITK_FORMAT_SSHORT, 
  AMITK_FORMAT_UINT, 
  AMITK_FORMAT_SINT, 
  AMITK_FORMAT_FLOAT, 
  AMITK_FORMAT_DOUBLE, 
  AMITK_FORMAT_NUM
} AmitkFormat;

typedef guint8  amitk_format_UBYTE_t;
typedef gint8   amitk_format_SBYTE_t;
typedef guint16 amitk_format_USHORT_t;
typedef gint16  amitk_format_SSHORT_t;
typedef guint32 amitk_format_UINT_t;
typedef gint32  amitk_format_SINT_t;
typedef gfloat  amitk_format_FLOAT_t;
typedef gdouble amitk_format_DOUBLE_t;



/*the formats that data can take on disk */
typedef enum {
  AMITK_RAW_FORMAT_UBYTE_NE, 
  AMITK_RAW_FORMAT_SBYTE_NE, 
  AMITK_RAW_FORMAT_USHORT_LE, 
  AMITK_RAW_FORMAT_SSHORT_LE, 
  AMITK_RAW_FORMAT_UINT_LE, 
  AMITK_RAW_FORMAT_SINT_LE, 
  AMITK_RAW_FORMAT_FLOAT_LE, 
  AMITK_RAW_FORMAT_DOUBLE_LE, 
  AMITK_RAW_FORMAT_USHORT_BE, 
  AMITK_RAW_FORMAT_SSHORT_BE, 
  AMITK_RAW_FORMAT_UINT_BE,
  AMITK_RAW_FORMAT_SINT_BE,
  AMITK_RAW_FORMAT_FLOAT_BE,
  AMITK_RAW_FORMAT_DOUBLE_BE,
  AMITK_RAW_FORMAT_UINT_PDP,
  AMITK_RAW_FORMAT_SINT_PDP,
  AMITK_RAW_FORMAT_FLOAT_PDP, 
  AMITK_RAW_FORMAT_ASCII_NE,
  AMITK_RAW_FORMAT_NUM
} AmitkRawFormat;



typedef struct _AmitkRawDataClass AmitkRawDataClass;
typedef struct _AmitkRawData      AmitkRawData;


struct _AmitkRawData {

  GObject parent;

  AmitkVoxel dim;
  gpointer data;
  AmitkFormat format;
  
};

struct _AmitkRawDataClass
{
  GObjectClass parent_class;

};


/* -------- defines ----------- */
#define amitk_raw_data_includes_voxel(ds, vox) (!(((vox).x < 0) ||  ((vox).y < 0) ||   \
						  ((vox).z < 0) ||  ((vox).t < 0) ||  \
						  ((vox).x >= (ds)->dim.x) ||  \
						  ((vox).y >= (ds)->dim.y) ||  \
						  ((vox).z >= (ds)->dim.z) ||  \
						  ((vox).t >= (ds)->dim.t)))
#define amitk_raw_data_num_voxels(rd) ((rd)->dim.x * (rd)->dim.y * (rd)->dim.z * (rd)->dim.t)
#define amitk_raw_data_size_data_mem(rd) (amitk_raw_data_num_voxels(rd) * amitk_format_sizes[(rd)->format])
#define amitk_raw_data_get_data_mem(rd) (g_try_malloc(amitk_raw_data_size_data_mem(rd)))


/* ------------ external functions ---------- */

GType	        amitk_raw_data_get_type	            (void);
AmitkRawData*   amitk_raw_data_new                  (void);
AmitkRawData*   amitk_raw_data_new_with_data        (AmitkFormat format,
						     AmitkVoxel dim);
AmitkRawData *  amitk_raw_data_import_raw_file      (const gchar * file_name, 
						     AmitkRawFormat raw_format,
						     AmitkVoxel dim,
						     guint file_offset);
gchar *         amitk_raw_data_write_xml            (AmitkRawData  * raw_data, const gchar * name);
AmitkRawData *  amitk_raw_data_read_xml             (gchar * xml_filename);
amide_data_t    amitk_raw_data_get_value            (const AmitkRawData * rd, 
						     const AmitkVoxel i);


AmitkFormat    amitk_raw_format_to_format(AmitkRawFormat raw_format);
AmitkRawFormat amitk_format_to_raw_format(AmitkFormat data_format);

#define amitk_raw_format_calc_num_bytes_per_slice(dim, raw_format) ((dim).x*(dim).y*amitk_raw_format_sizes[raw_format])
#define amitk_raw_format_calc_num_bytes(dim, raw_format) ((dim).z*(dim).t*amitk_raw_format_calc_num_bytes_per_slice(dim,raw_format))


/* external variables */
extern guint amitk_format_sizes[];
extern gchar * amitk_format_names[];
extern guint amitk_raw_format_sizes[];
extern gchar * amitk_raw_format_names[];

/* variable type function declarations */
#include "amitk_raw_data_UBYTE.h"
#include "amitk_raw_data_SBYTE.h"
#include "amitk_raw_data_USHORT.h"
#include "amitk_raw_data_SSHORT.h"
#include "amitk_raw_data_UINT.h"
#include "amitk_raw_data_SINT.h"
#include "amitk_raw_data_FLOAT.h"
#include "amitk_raw_data_DOUBLE.h"

#endif /* __AMITK_RAW_DATA_H__ */

