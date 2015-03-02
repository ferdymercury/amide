/* roi.h
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

#ifndef __ROI__
#define __ROI__

/* header files that are always needed with this file */
#include "xml.h"
#include "realspace.h"

typedef enum {ELLIPSOID, CYLINDER, BOX, NUM_ROI_TYPES} roi_type_t;
typedef enum {GRAINS_1, GRAINS_8, GRAINS_27, GRAINS_64, NUM_GRAIN_TYPES} roi_grain_t;

typedef struct _roi_t roi_t;
typedef struct _roi_list_t roi_list_t;

/* the amide roi structure */
struct _roi_t {
  gchar * name;
  roi_type_t type;
  roi_grain_t grain; /* how fine a grain to calculate this roi at */
  realspace_t coord_frame;
  realpoint_t corner; /*far corner,near corner is always 0,0,0 in roi coord frame*/
  roi_t * parent;
  roi_list_t * children;

  /* stuff that doesn't need to be saved */
  guint reference_count;
};

/* used to make a linked list of roi_t's */
struct _roi_list_t {
  roi_t * roi;
  guint reference_count;
  roi_list_t * next;
};

/* structure containing the results of an roi analysis */
typedef struct roi_analysis_t {
  amide_time_t time_midpoint;
  amide_data_t mean;
  amide_data_t voxels;
  amide_data_t var;
  amide_data_t min;
  amide_data_t max;
} roi_analysis_t;

/* external functions */
roi_t * roi_free(roi_t * roi);
roi_t * roi_init(void);
gchar * roi_write_xml(roi_t * roi, gchar * study_directory);
roi_t * roi_load_xml(gchar * roi_xml_file_name, const gchar * study_directory);
roi_t * roi_copy(roi_t * src_roi);
roi_t * roi_add_reference(roi_t * roi);
void roi_set_name(roi_t * roi, gchar * new_name);
realpoint_t roi_calculate_center(const roi_t * roi);
roi_list_t * roi_list_free(roi_list_t *roi_list);
roi_list_t * roi_list_init(void);
void roi_list_write_xml(roi_list_t *list, xmlNodePtr node_list, gchar * study_directory);
roi_list_t * roi_list_load_xml(xmlNodePtr node_list, const gchar * study_directory);
gboolean roi_list_includes_roi(roi_list_t *list, roi_t * roi);
roi_list_t * roi_list_add_roi(roi_list_t * list, roi_t * roi);
roi_list_t * roi_list_add_roi_first(roi_list_t * list, roi_t * roi);
roi_list_t * roi_list_remove_roi(roi_list_t * list, roi_t * roi);
roi_list_t * roi_list_copy(roi_list_t * src_roi_list);
void roi_free_points_list(GSList ** plist);
gboolean roi_undrawn(const roi_t * roi);
GSList * roi_get_volume_intersection_points(const volume_t * view_slice,
					    const roi_t * roi);
roi_analysis_t roi_calculate_analysis(roi_t * roi, 
				      const volume_t * volume,
				      roi_grain_t grain,
				      guint frame);

/* internal functions */
void roi_subset_of_volume(roi_t * roi,
			  const volume_t * volume,
			  voxelpoint_t * subset_start,
			  voxelpoint_t * subset_dim);

/* external variables */
extern gchar * roi_type_names[];
extern gchar * roi_grain_names[];




#endif /* __ROI__ */



















