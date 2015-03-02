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

typedef enum {GROUP, ELLIPSOID, CYLINDER, BOX, NUM_ROI_TYPES} roi_type_t;
typedef enum {GRAINS_1, GRAINS_8, GRAINS_27, GRAINS_64, NUM_GRAIN_TYPES} roi_grain_t;

typedef struct _amide_roi_t amide_roi_t;
typedef struct _amide_roi_list_t amide_roi_list_t;

/* the amide roi structure */
struct _amide_roi_t {
  gchar * name;
  roi_type_t type;
  realspace_t coord_frame;
  realpoint_t corner; /*far corner,near corner is always 0,0,0 in roi coord frame*/
  amide_roi_t * parent;
  amide_roi_list_t * children;
  roi_grain_t grain; /* how fine a grain to calculate this roi at */
};

/* used to make a linked list of amide_roi_t's */
struct _amide_roi_list_t {
  amide_roi_t * roi;
  amide_roi_list_t * next;
};

/* structure containing the results of an roi analysis */
typedef struct amide_roi_analysis_t {
  volume_data_t mean;
  volume_data_t voxels;
  volume_data_t var;
  volume_data_t min;
  volume_data_t max;
} amide_roi_analysis_t;

/* external functions */
void roi_free(amide_roi_t ** proi);
void roi_list_free(amide_roi_list_t ** proi_list);
amide_roi_t * roi_init(void);
void roi_copy(amide_roi_t ** dest_roi, amide_roi_t * src_roi);
void roi_set_name(amide_roi_t * roi, gchar * new_name);
realpoint_t roi_calculate_center(const amide_roi_t * roi);
amide_roi_list_t * roi_list_init(void);
gboolean roi_list_includes_roi(amide_roi_list_t *list, amide_roi_t * roi);
void roi_list_add_roi(amide_roi_list_t ** plist, amide_roi_t * roi);
void roi_list_add_roi_first(amide_roi_list_t ** plist, amide_roi_t * roi);
void roi_list_remove_roi(amide_roi_list_t ** plist, amide_roi_t * roi);
void roi_list_copy(amide_roi_list_t ** dest_roi_list, amide_roi_list_t * src_roi_list);
void roi_free_points_list(GSList ** plist);
gboolean roi_undrawn(const amide_roi_t * roi);
GSList * roi_get_volume_intersection_points(const amide_volume_t * view_slice,
					    const amide_roi_t * roi);
amide_roi_analysis_t roi_calculate_analysis(amide_roi_t * roi, 
					    amide_volume_t * volume,
					    roi_grain_t grain,
					    guint frame);


/* internal functions */
gboolean roi_voxel_in_box(const realpoint_t p,
			  const realpoint_t p0,
			  const realpoint_t p1);
gboolean roi_voxel_in_elliptic_cylinder(const realpoint_t p,
					const realpoint_t center,
					const floatpoint_t height,
					const realpoint_t radius);
gboolean roi_voxel_in_ellipsoid(const realpoint_t p,
				const realpoint_t center,
				const realpoint_t radius);
void roi_subset_of_volume(amide_roi_t * roi,
			  amide_volume_t * volume,
			  voxelpoint_t * subset_start,
			  voxelpoint_t * subset_dim);

/* external variables */
extern gchar * roi_type_names[];
extern gchar * roi_grain_names[];






















