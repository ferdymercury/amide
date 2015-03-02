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

#ifndef __ROI_H__
#define __ROI_H__

/* header files that are always needed with this file */
#include "volume.h"


typedef enum {ELLIPSOID, CYLINDER, BOX, ISOCONTOUR_2D, ISOCONTOUR_3D, NUM_ROI_TYPES} roi_type_t;

typedef struct _roi_t roi_t;
typedef struct _roi_list_t roi_list_t;

     

/* the amide roi structure */
struct _roi_t {
  gchar * name;
  roi_type_t type;
  realspace_t coord_frame;
  realpoint_t corner; /*far corner,near corner is always 0,0,0 in roi coord frame*/
  roi_t * parent;
  roi_list_t * children;

  /* isocontour specific stuff */
  data_set_t * isocontour;
  realpoint_t voxel_size;
  amide_data_t isocontour_value;

  /* stuff that doesn't need to be saved */
  guint reference_count;
};

/* used to make a linked list of roi_t's */
struct _roi_list_t {
  roi_t * roi;
  guint reference_count;
  roi_list_t * next;
};

/* macros */
#define ROI_REALPOINT_TO_VOXEL(roi, real, vox) (((vox).x = floor((real).x/(roi)->voxel_size.x)), \
						((vox).y = floor((real).y/(roi)->voxel_size.y)), \
						((vox).z = floor((real).z/(roi)->voxel_size.z)), \
						((vox).t = (0)))

/* external functions */
roi_t * roi_free(roi_t * roi);
roi_t * roi_init(void);
gchar * roi_write_xml(roi_t * roi, gchar * study_directory);
roi_t * roi_load_xml(gchar * roi_xml_file_name, const gchar * study_directory);
roi_t * roi_copy(roi_t * src_roi);
roi_t * roi_add_reference(roi_t * roi);
void roi_set_name(roi_t * roi, gchar * new_name);
realpoint_t roi_calculate_center(const roi_t * roi);
void roi_get_view_corners(const roi_t * roi,
			  const realspace_t view_coord_frame,
			  realpoint_t view_corner[]);
roi_list_t * roi_list_free(roi_list_t *roi_list);
roi_list_t * roi_list_init(roi_t * roi);
guint roi_list_count(roi_list_t * list);
void roi_list_write_xml(roi_list_t *list, xmlNodePtr node_list, gchar * study_directory);
roi_list_t * roi_list_load_xml(xmlNodePtr node_list, const gchar * study_directory);
gboolean roi_list_includes_roi(roi_list_t *list, roi_t * roi);
roi_list_t * roi_list_add_roi(roi_list_t * list, roi_t * roi);
roi_list_t * roi_list_add_roi_first(roi_list_t * list, roi_t * roi);
roi_list_t * roi_list_remove_roi(roi_list_t * list, roi_t * roi);
roi_list_t * roi_list_copy(roi_list_t * src_roi_list);
roi_list_t * roi_list_add_reference(roi_list_t * rois);
void rois_get_view_corners(roi_list_t * rois,
			   const realspace_t view_coord_frame,
			   realpoint_t view_corner[]);
GSList * roi_free_points_list(GSList * list);
gboolean roi_undrawn(const roi_t * roi);
floatpoint_t rois_max_min_voxel_size(roi_list_t * rois);
GSList * roi_get_intersection_line(const roi_t * roi, const volume_t * view_slice);
volume_t * roi_get_slice(const roi_t * roi, const volume_t * view_slice);
void roi_subset_of_volume(const roi_t * roi,
			  const volume_t * volume,
			  intpoint_t frame,
			  voxelpoint_t * subset_start,
			  voxelpoint_t * subset_dim);
void roi_set_isocontour(roi_t * roi, volume_t * vol, voxelpoint_t value_vp);
void roi_isocontour_erase_area(roi_t * roi, voxelpoint_t erase_vp, gint area_size);

/* external variables */
extern gchar * roi_type_names[];
extern gchar * roi_menu_names[];
extern gchar * roi_menu_explanation[];

/* variable type function declarations */
#include "roi_ELLIPSOID.h"
#include "roi_CYLINDER.h"
#include "roi_BOX.h"
#include "roi_ISOCONTOUR_2D.h"
#include "roi_ISOCONTOUR_3D.h"



#endif /* __ROI_H_ */



















