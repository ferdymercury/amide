/* volume.h
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

#ifndef __VOLUME_H__
#define __VOLUME_H__

/* header files that are always needed with this file */
#include "xml.h"
#include "color_table.h"
#include "data_set.h"

#define VOLUME_DISTRIBUTION_SIZE 300

typedef enum {XDIM, YDIM, ZDIM, TDIM, NUM_DIMS} dimension_t;
typedef enum {NEAREST_NEIGHBOR, 
	      TWO_BY_TWO, 
	      TWO_BY_TWO_BY_TWO, 
	      TRILINEAR, 
	      NUM_INTERPOLATIONS} interpolation_t;
typedef enum {PET, SPECT, CT, MRI, OTHER, NUM_MODALITIES} modality_t;

/* the volume structure */
typedef struct volume_t { 

  /* parameters to save */
  gchar * name;
  gchar * scan_date; /* the time/day the image was acquired */
  modality_t modality;
  realpoint_t voxel_size;  /* in mm */
  data_set_t * data_set;
  amide_data_t external_scaling; /* user specified factor to multiply data set by */
  data_set_t * internal_scaling; /* internally (data set) supplied scaling factor */
  amide_time_t scan_start;
  amide_time_t * frame_duration; /* array of the duration of each frame */
  color_table_t color_table; /* the color table to draw this volume in */
  amide_data_t threshold_max; /* the thresholds to use for this volume */
  amide_data_t threshold_min; 
  realspace_t coord_frame;
  
  /* parameters calculated at run time */
  guint reference_count;
  data_set_t * distribution; /* 1D array of data distribution, used in thresholding */

  /* can be recalculated, but used enough we'll store... */
  amide_data_t max; 
  amide_data_t min;
  data_set_t * current_scaling; /* external_scaling * internal_scaling */
  realpoint_t corner; /* far corner, near corner is 0,0,0 in volume coord space*/ 

} volume_t;


/* a list of volumes */
typedef struct _volume_list_t volume_list_t;
struct _volume_list_t {
  volume_t * volume;
  guint reference_count;
  volume_list_t * next;
};




/* -------- defines ----------- */

/* figure out the real point that corresponds to the voxel coordinates */
#define VOLUME_VOXEL_TO_REALPOINT(vol, vox, real) (((real).x = (((floatpoint_t) (vox).x) + 0.5) * (vol)->voxel_size.x), \
						   ((real).y = (((floatpoint_t) (vox).y) + 0.5) * (vol)->voxel_size.y), \
						   ((real).z = (((floatpoint_t) (vox).z) + 0.5) * (vol)->voxel_size.z))

/* figure out the voxel point that corresponds to the real coordinates */
/* makes use of floats being truncated when converting to int */
#define VOLUME_REALPOINT_TO_VOXEL(vol, real, frame, vox) (((vox).x = ((real).x/(vol)->voxel_size.x)), \
							  ((vox).y = ((real).y/(vol)->voxel_size.y)), \
							  ((vox).z = ((real).z/(vol)->voxel_size.z)), \
							  ((vox).t = (frame)))
     


#define volume_size_frame_duration_mem(vol) ((vol)->data_set->dim.t*sizeof(amide_time_t))
#define volume_get_frame_duration_mem(vol) ((amide_time_t * ) g_malloc(volume_size_frame_duration_mem(vol)))

#define EMPTY 0.0
//#define AXIS_VOLUME_DENSITY 0.1

/* ------------ external functions ---------- */
volume_t * volume_free(volume_t * volume);
volume_t * volume_init(void);
gchar * volume_write_xml(volume_t * volume, gchar * study_directory);
volume_t * volume_load_xml(gchar * volume_xml_filename, const gchar * study_directory);
volume_t * volume_copy(volume_t * src_volume);
volume_t * volume_add_reference(volume_t * volume);
void volume_set_name(volume_t * volume, gchar * new_name);
void volume_set_scan_date(volume_t * volume, gchar * new_date);
void volume_set_scaling(volume_t * volume, amide_data_t new_external_scaling);
realpoint_t volume_calculate_center(const volume_t * volume);
amide_time_t volume_start_time(const volume_t * volume, guint frame);
amide_time_t volume_end_time(const volume_t * volume, guint frame);
guint volume_frame(const volume_t * volume, const amide_time_t time);
amide_time_t volume_min_frame_duration(const volume_t * volume);
void volume_recalc_far_corner(volume_t * volume);
void volume_recalc_max_min(volume_t * volume);
void volume_generate_distribution(volume_t * volume);
amide_data_t volume_value(const volume_t * volume, const voxelpoint_t i);
volume_list_t * volume_list_free(volume_list_t * volume_list);
volume_list_t * volume_list_init(void);
void volume_list_write_xml(volume_list_t *list, xmlNodePtr node_list, gchar * study_directory);
volume_list_t * volume_list_load_xml(xmlNodePtr node_list, const gchar * study_directory);
volume_list_t * volume_list_add_reference(volume_list_t * volume_list_element);
gboolean volume_list_includes_volume(volume_list_t *list, volume_t * vol);
volume_list_t * volume_list_add_volume(volume_list_t *volume_list, volume_t * vol);
volume_list_t * volume_list_add_volume_first(volume_list_t * volume_list, volume_t * vol);
volume_list_t * volume_list_remove_volume(volume_list_t * volume_list, volume_t * vol);
volume_list_t * volume_list_copy(volume_list_t * src_volume_list);
amide_time_t volume_list_start_time(volume_list_t * volume_list);
amide_time_t volume_list_end_time(volume_list_t * volume_list);
amide_time_t volume_list_min_frame_duration(volume_list_t * volume_list);
void volume_get_view_corners(const volume_t * volume,
			     const realspace_t view_coord_frame,
			     realpoint_t corner[]);
void volumes_get_view_corners(volume_list_t * volumes,
			      const realspace_t view_coord_frame,
			      realpoint_t view_corner[]);
floatpoint_t volumes_min_voxel_size(volume_list_t * volumes);
floatpoint_t volumes_max_size(volume_list_t * volumes);
floatpoint_t volumes_max_min_voxel_size(volume_list_t * volumes);
intpoint_t volumes_max_dim(volume_list_t * volumes);
//volume_t * volume_get_axis_volume(guint x_width, guint y_width, guint z_width);
volume_t * volume_get_slice(const volume_t * volume,
			    const amide_time_t start,
			    const amide_time_t duration,
			    const realpoint_t  requested_voxel_size,
			    const realspace_t slice_coord_frame,
			    const realpoint_t far_corner,
			    const interpolation_t interpolation,
			    const gboolean need_calc_max_min);
volume_list_t * volumes_get_slices(volume_list_t * volumes,
				   const amide_time_t start,
				   const amide_time_t duration,
				   const floatpoint_t thickness,
				   const realspace_t view_coord_frame,
				   const floatpoint_t zoom,
				   const interpolation_t interpolation,
				   const gboolean need_calc_max_min);

/* variable type function declarations */
#include "volume_UBYTE_0D_SCALING.h"
#include "volume_UBYTE_1D_SCALING.h"
#include "volume_UBYTE_2D_SCALING.h"
#include "volume_SBYTE_0D_SCALING.h"
#include "volume_SBYTE_1D_SCALING.h"
#include "volume_SBYTE_2D_SCALING.h"
#include "volume_USHORT_0D_SCALING.h"
#include "volume_USHORT_1D_SCALING.h"
#include "volume_USHORT_2D_SCALING.h"
#include "volume_SSHORT_0D_SCALING.h"
#include "volume_SSHORT_1D_SCALING.h"
#include "volume_SSHORT_2D_SCALING.h"
#include "volume_UINT_0D_SCALING.h"
#include "volume_UINT_1D_SCALING.h"
#include "volume_UINT_2D_SCALING.h"
#include "volume_SINT_0D_SCALING.h"
#include "volume_SINT_1D_SCALING.h"
#include "volume_SINT_2D_SCALING.h"
#include "volume_FLOAT_0D_SCALING.h"
#include "volume_FLOAT_1D_SCALING.h"
#include "volume_FLOAT_2D_SCALING.h"
#include "volume_DOUBLE_0D_SCALING.h"
#include "volume_DOUBLE_1D_SCALING.h"
#include "volume_DOUBLE_2D_SCALING.h"
				 
/* external variables */
extern gchar * interpolation_names[];
extern gchar * interpolation_explanations[];
extern gchar * modality_names[];


#endif /* __VOLUME_H__ */
