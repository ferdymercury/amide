/* volume.h
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

#ifndef __VOLUME_H__
#define __VOLUME_H__

/* header files that are always needed with this file */
#include "xml.h"
#include "color_table.h"
#include "data_set.h"
#include "align_pt.h"

#define VOLUME_DISTRIBUTION_SIZE 256

typedef enum {XDIM, YDIM, ZDIM, TDIM, NUM_DIMS} dimension_t;
typedef enum {NEAREST_NEIGHBOR, TRILINEAR, NUM_INTERPOLATIONS} interpolation_t;
typedef enum {PET, SPECT, CT, MRI, OTHER, NUM_MODALITIES} modality_t;

typedef enum {
  AMIDE_GUESS, RAW_DATA, PEM_DATA, IDL_DATA,
#ifdef AMIDE_LIBECAT_SUPPORT
  LIBECAT_DATA, 
#endif
#ifdef AMIDE_LIBMDC_SUPPORT
  LIBMDC_DATA,
#endif
  NUM_IMPORT_METHODS
} import_method_t;
       

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
  realspace_t * coord_frame;
  align_pts_t * align_pts;

  threshold_t threshold_type; /* what sort of thresholding we're using (per slice, global, etc.) */
  amide_data_t threshold_max[2]; /* the thresholds to use for this volume */
  amide_data_t threshold_min[2]; 
  guint threshold_ref_frame[2];
  
  /* parameters calculated at run time */
  guint ref_count;
  data_set_t * distribution; /* 1D array of data distribution, used in thresholding */

  /* can be recalculated on the fly, but used enough we'll store... */
  amide_data_t global_max;
  amide_data_t global_min;
  amide_data_t * frame_max; 
  amide_data_t * frame_min;
  data_set_t * current_scaling; /* external_scaling * internal_scaling */
  realpoint_t corner; /* far corner, near corner is 0,0,0 in volume coord space*/ 

} volume_t;


/* a list of volumes */
typedef struct _volumes_t volumes_t;
struct _volumes_t {
  volume_t * volume;
  guint ref_count;
  volumes_t * next;
};




/* -------- defines ----------- */


#define volume_size_frame_duration_mem(vol) ((vol)->data_set->dim.t*sizeof(amide_time_t))
#define volume_get_frame_duration_mem(vol) ((amide_time_t * ) g_malloc(volume_size_frame_duration_mem(vol)))
#define volume_size_frame_max_min_mem(vol) ((vol)->data_set->dim.t*sizeof(amide_data_t))
#define volume_get_frame_max_min_mem(vol) ((amide_data_t * ) g_malloc(volume_size_frame_max_min_mem(vol)))
#define volume_dynamic(vol) ((vol)->data_set->dim.t > 1)

#define EMPTY 0.0
//#define AXIS_VOLUME_DENSITY 0.1

/* ------------ external functions ---------- */
volume_t * volume_unref(volume_t * volume);
volume_t * volume_init(void);
gchar * volume_write_xml(volume_t * volume, gchar * study_directory);
volume_t * volume_load_xml(gchar * volume_xml_filename, const gchar * study_directory);
volume_t * volume_import_file(import_method_t import_method, int submethod,
			      const gchar * import_filename, gchar * model_filename);
volume_t * volume_copy(volume_t * src_volume);
volume_t * volume_ref(volume_t * volume);
void volume_add_align_pt(volume_t * volume, align_pt_t * new_pt);
void volume_set_name(volume_t * volume, gchar * new_name);
void volume_set_scan_date(volume_t * volume, gchar * new_date);
void volume_set_scaling(volume_t * volume, amide_data_t new_external_scaling);
realpoint_t volume_center(const volume_t * volume);
void volume_set_center(volume_t * volume, realpoint_t center);
amide_time_t volume_start_time(const volume_t * volume, guint frame);
amide_time_t volume_end_time(const volume_t * volume, guint frame);
guint volume_frame(const volume_t * volume, const amide_time_t time);
amide_time_t volume_min_frame_duration(const volume_t * volume);
void volume_recalc_far_corner(volume_t * volume);
void volume_calc_global_max_min(volume_t * volume);
void volume_calc_frame_max_min(volume_t * volume);
amide_data_t volume_max(volume_t * volume, amide_time_t start, amide_time_t duration);
amide_data_t volume_min(volume_t * volume, amide_time_t start, amide_time_t duration);
void volume_generate_distribution(volume_t * volume);
amide_data_t volume_value(const volume_t * volume, const voxelpoint_t i);
volumes_t * volumes_unref(volumes_t * volumes);
volumes_t * volumes_init(volume_t * volume);
guint volumes_count(volumes_t * list);
void volumes_write_xml(volumes_t *list, xmlNodePtr node_list, gchar * study_directory);
volumes_t * volumes_load_xml(xmlNodePtr node_list, const gchar * study_directory);
volumes_t * volumes_copy(volumes_t * src_volumes);
volumes_t * volumes_ref(volumes_t * volumes);
gboolean volumes_includes_volume(volumes_t *list, volume_t * vol);
volumes_t * volumes_add_volume(volumes_t *volumes, volume_t * vol);
volumes_t * volumes_add_volume_first(volumes_t * volumes, volume_t * vol);
volumes_t * volumes_remove_volume(volumes_t * volumes, volume_t * vol);
amide_time_t volumes_start_time(volumes_t * volumes);
amide_time_t volumes_end_time(volumes_t * volumes);
amide_time_t volumes_min_frame_duration(volumes_t * volumes);
void volume_get_view_corners(const volume_t * volume,
			     const realspace_t * view_coord_frame,
			     realpoint_t corner[]);
void volumes_get_view_corners(volumes_t * volumes,
			      const realspace_t * view_coord_frame,
			      realpoint_t view_corner[]);
floatpoint_t volumes_min_voxel_size(volumes_t * volumes);
floatpoint_t volumes_max_size(volumes_t * volumes);
floatpoint_t volumes_max_min_voxel_size(volumes_t * volumes);
intpoint_t volumes_max_dim(volumes_t * volumes);
volume_t * volume_get_slice(const volume_t * volume,
			    const amide_time_t start,
			    const amide_time_t duration,
			    const realpoint_t  requested_voxel_size,
			    realspace_t * slice_coord_frame,
			    const realpoint_t far_corner,
			    const interpolation_t interpolation,
			    const gboolean need_calc_max_min);
volumes_t * volumes_get_slices(volumes_t * volumes,
			       const amide_time_t start,
			       const amide_time_t duration,
			       const floatpoint_t thickness,
			       const floatpoint_t voxel_dim,
			       const realspace_t * view_coord_frame,
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
extern gchar * import_menu_names[];
extern gchar * import_menu_explanations[];


#endif /* __VOLUME_H__ */
