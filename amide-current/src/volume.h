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

typedef enum {XDIM, YDIM, ZDIM, TDIM, NUM_DIMS} dimension_t;
typedef enum {NEAREST_NEIGHBOR, BILINEAR, TRILINEAR, NUM_INTERPOLATIONS} interpolation_t;
typedef enum {PET, SPECT, CT, MRI, OTHER, NUM_MODALITIES} modality_t;

/* setup the types for various internal data formats */
//typedef gdouble volume_data_t;
typedef gfloat volume_data_t;
typedef gdouble volume_time_t;


/* the volume structure */
typedef struct volume_t { 

  /* parameters to save */
  gchar * name;
  modality_t modality;
  realpoint_t voxel_size;  /* in mm */
  voxelpoint_t dim;
  volume_data_t * data;
  volume_data_t conversion; /* factor  to translate data into useable units */
  guint num_frames;
  volume_time_t scan_start;
  volume_time_t * frame_duration; /* array of the duration of each frame */
  color_table_t color_table; /* the color table to draw this roi in */
  volume_data_t max; /* can be recalculated, but used enough we'll store... */
  volume_data_t min;
  volume_data_t threshold_max; /* the thresholds to use for this volume */
  volume_data_t threshold_min; 
  realspace_t coord_frame;
  realpoint_t corner; /* far corner, near corner is 0,0,0 in volume coord space*/ 
  
  /* parameters calculated at run time */
  guint reference_count;
  volume_data_t * distribution; /* 1D array of data distribution, used in thresholding */
} volume_t;


/* a list of volumes */
typedef struct _volume_list_t volume_list_t;
struct _volume_list_t {
  volume_t * volume;
  guint reference_count;
  volume_list_t * next;
};




/* -------- defines ----------- */

/* returns a pointer to the voxel specified by time t and voxelpoint i */
#define VOLUME_POINTER(volume,t,i) (((volume)->data)+((i).x + (i).y * ((volume)->dim.x) + (i).z * ((volume)->dim.x) * ((volume)->dim.y) + (t) * ((volume)->dim.x) * ((volume)->dim.y) * ((volume)->dim.z)))

/* translates to the contents of the voxel specified by time t and voxelpoint i */
#define VOLUME_CONTENTS(volume,t,i) ((volume)->conversion * (*(VOLUME_POINTER((volume),(t),(i)))))

#define VOLUME_SET_CONTENT(volume,t,i) (*(VOLUME_POINTER((volume),(t),(i))))

#define volume_num_voxels_per_frame(vol) ((vol)->dim.x * (vol)->dim.y * (vol)->dim.z)
#define volume_num_voxels(vol) (volume_num_voxels_per_frame(vol) * (vol)->num_frames)
#define volume_size_data_mem(vol) (volume_num_voxels(vol) * sizeof(volume_data_t))
#define volume_get_data_mem(vol) ((volume_data_t * ) g_malloc(volume_size_data_mem(vol)))
#define volume_size_frame_duration_mem(vol) ((vol)->num_frames*sizeof(volume_time_t))
#define volume_get_frame_duration_mem(vol) ((volume_time_t * ) g_malloc(volume_size_frame_duration_mem(vol)))
#define volume_set_dim_x(vol,num) (((vol)->dim.x) = (num))
#define volume_set_dim_y(vol,num) (((vol)->dim.y) = (num))
#define volume_set_dim_z(vol,num) (((vol)->dim.z) = (num))


#define EMPTY 0.0
#define AXIS_VOLUME_DENSITY 0.1

/* ------------ external functions ---------- */
volume_t * volume_free(volume_t * volume);
volume_t * volume_init(void);
volume_t * volume_copy(volume_t * src_volume);
volume_t * volume_add_reference(volume_t * volume);
void volume_set_name(volume_t * volume, gchar * new_name);
realpoint_t volume_calculate_center(const volume_t * volume);
realpoint_t volume_voxel_to_real(const volume_t * volume, const voxelpoint_t i);
voxelpoint_t volume_real_to_voxel(const volume_t * volume, const realpoint_t real);
volume_time_t volume_start_time(const volume_t * volume, guint frame);
volume_list_t * volume_list_free(volume_list_t * volume_list);
volume_list_t * volume_list_init(void);
volume_list_t * volume_list_add_reference(volume_list_t * volume_list_element);
gboolean volume_list_includes_volume(volume_list_t *list, volume_t * vol);
volume_list_t * volume_list_add_volume(volume_list_t *volume_list, volume_t * vol);
volume_list_t * volume_list_add_volume_first(volume_list_t * volume_list, volume_t * vol);
volume_list_t * volume_list_remove_volume(volume_list_t * volume_list, volume_t * vol);
gboolean volume_includes_voxel(const volume_t * volume, const voxelpoint_t voxel);
void volume_get_view_corners(const volume_t * volume,
			     const realspace_t view_coord_frame,
			     realpoint_t corner[]);
void volumes_get_view_corners(volume_list_t * volumes,
			      const realspace_t view_coord_frame,
			      realpoint_t view_corner[]);
floatpoint_t volumes_min_dim(volume_list_t * volumes);
intpoint_t volumes_max_dim(volume_list_t * volumes);
floatpoint_t volumes_get_width(volume_list_t * volumes, const realspace_t view_coord_frame);
floatpoint_t volumes_get_height(volume_list_t * volumes, const realspace_t view_coord_frame);
floatpoint_t volumes_get_length(volume_list_t * volumes, const realspace_t view_coord_frame);
volume_t * volume_get_axis_volume(guint x_width, guint y_width, guint z_width);
volume_t * volume_get_slice(const volume_t * volume,
			    const volume_time_t start,
			    const volume_time_t duration,
			    const realpoint_t  requested_voxel_size,
			    const realspace_t slice_coord_frame,
			    const realpoint_t far_corner,
			    const interpolation_t interpolation);
volume_list_t * volumes_get_slices(volume_list_t * volumes,
				   const volume_time_t start,
				   const volume_time_t duration,
				   const floatpoint_t thickness,
				   const realspace_t view_coord_frame,
				   const floatpoint_t zoom,
				   const interpolation_t interpolation);

				 
/* external variables */
extern gchar * interpolation_names[];
extern gchar * modality_names[];

