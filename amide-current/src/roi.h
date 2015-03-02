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
typedef struct _rois_t rois_t;

     

/* the amide roi structure */
struct _roi_t {
  gchar * name;
  roi_type_t type;
  realspace_t * coord_frame;
  realpoint_t corner; /*far corner,near corner is always 0,0,0 in roi coord frame*/
  roi_t * parent;
  rois_t * children;

  /* isocontour specific stuff */
  data_set_t * isocontour;
  realpoint_t voxel_size;
  amide_data_t isocontour_value;

  /* stuff that doesn't need to be saved */
  guint ref_count;
};

/* used to make a linked list of roi_t's */
struct _rois_t {
  roi_t * roi;
  guint ref_count;
  rois_t * next;
};


/* external functions */
roi_t * roi_unref(roi_t * roi);
roi_t * roi_init(void);
gchar * roi_write_xml(roi_t * roi, gchar * study_directory);
roi_t * roi_load_xml(gchar * roi_xml_file_name, const gchar * study_directory);
roi_t * roi_copy(roi_t * src_roi);
roi_t * roi_ref(roi_t * roi);
void roi_set_name(roi_t * roi, gchar * new_name);
realpoint_t roi_calculate_center(const roi_t * roi);
void roi_get_view_corners(const roi_t * roi,
			  const realspace_t * view_coord_frame,
			  realpoint_t view_corner[]);
rois_t * rois_unref(rois_t *rois);
rois_t * rois_init(roi_t * roi);
guint rois_count(rois_t * list);
void rois_write_xml(rois_t *list, xmlNodePtr node_list, gchar * study_directory);
rois_t * rois_load_xml(xmlNodePtr node_list, const gchar * study_directory);
gboolean rois_includes_roi(rois_t *list, roi_t * roi);
rois_t * rois_add_roi(rois_t * list, roi_t * roi);
rois_t * rois_add_roi_first(rois_t * list, roi_t * roi);
rois_t * rois_remove_roi(rois_t * list, roi_t * roi);
rois_t * rois_copy(rois_t * src_roi_list);
rois_t * rois_ref(rois_t * rois);
void rois_get_view_corners(rois_t * rois,
			   const realspace_t * view_coord_frame,
			   realpoint_t view_corner[]);
GSList * roi_free_points_list(GSList * list);
gboolean roi_undrawn(const roi_t * roi);
roi_t * rois_undrawn_roi(const rois_t * rois);
floatpoint_t rois_max_min_voxel_size(rois_t * rois);
GSList * roi_get_intersection_line(const roi_t * roi, 
				   const realpoint_t canvas_voxel_size,
				   const realspace_t * canvas_coord_frame,
				   const realpoint_t canvas_corner);
volume_t * roi_get_intersection_slice(const roi_t * roi, 
				      const realspace_t * canvas_coord_frame,
				      const realpoint_t canvas_corner,
				      const realpoint_t canvas_voxel_size);
void roi_subset_of_space(const roi_t * roi,
			 const realspace_t * coord_frame,
			 const realpoint_t corner,
			 const realpoint_t voxel_size,
			 voxelpoint_t * subset_start,
			 voxelpoint_t * subset_dim);
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



















