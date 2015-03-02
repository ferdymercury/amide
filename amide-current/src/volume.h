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
typedef gdouble volume_data_t;
typedef gdouble volume_time_t;

typedef struct _amide_volume_list_t amide_volume_list_t;

/* the amide volume structure */
typedef struct amide_volume_t { 
  gchar * name;
  modality_t modality;
  realpoint_t voxel_size;  /* in mm */
  voxelpoint_t dim;
  volume_data_t * data;
  volume_data_t conversion; /* factor  to translate data into useable units */
  guint num_frames;
  volume_time_t scan_start;
  volume_time_t * frame_duration; /* array of the duration of each frame */
  volume_data_t max; 
  volume_data_t min;
  color_table_t color_table; /* the color table to draw this roi in */
  volume_data_t threshold_max; /* the thresholds to use for this volume */
  volume_data_t threshold_min; 
  realspace_t coord_frame;
  realpoint_t corner; /* far corner, near corner is 0,0,0 in volume coord space*/ 
  volume_data_t * distribution; /* 1D array of data distribution, used in thresholding */
} amide_volume_t;

struct _amide_volume_list_t {
  amide_volume_t * volume;
  amide_volume_list_t * next;
};

/* convenience functions */

/* returns a pointer to the voxel specified by time t and voxelpoint i */
#define VOLUME_POINTER(volume,t,i) (((volume)->data)+((i).x + (i).y * ((volume)->dim.x) + (i).z * ((volume)->dim.x) * ((volume)->dim.y) + (t) * ((volume)->dim.x) * ((volume)->dim.y) * ((volume)->dim.z)))

/* translates to the contents of the voxel specified by time t and voxelpoint i */
#define VOLUME_CONTENTS(volume,t,i) ((volume)->conversion * (*(VOLUME_POINTER((volume),(t),(i)))))

#define VOLUME_SET_CONTENT(volume,t,i) (*(VOLUME_POINTER((volume),(t),(i))))

#define VOLUME_I_TO_R(index,voxel_size) ((index)*(voxel_size)+(voxel_size)/2.0)

#define VOLUME_R_TO_I(real,voxel_size) (rint((real)/(voxel_size)-(voxel_size)/2.0))

#define VOLUME_INDEX_TO_REALPOINT(volume,index,real_p) ( \
                     (real_p).x=VOLUME_I_TO_R((index).x,(volume)->voxel_size.x), \
                     (real_p).y=VOLUME_I_TO_R((index).y,(volume)->voxel_size.y), \
                     (real_p).z=VOLUME_I_TO_R((index).z,(volume)->voxel_size.z))

#define VOLUME_REALPOINT_TO_INDEX(volume,real_p,index) ( \
                     (index).x=VOLUME_R_TO_I((real_p.x),(volume)->voxel_size.x), \
                     (index).y=VOLUME_R_TO_I((real_p.y),(volume)->voxel_size.y), \
                     (index).z=VOLUME_R_TO_I((real_p.z),(volume)->voxel_size.z))


#define EMPTY 0.0

/* external functions */
void volume_free(amide_volume_t ** volume);
amide_volume_t * volume_init(void);
void volume_copy(amide_volume_t ** dest_volume, amide_volume_t * src_volume);
void volume_set_name(amide_volume_t * volume, gchar * new_name);
realpoint_t volume_calculate_center(const amide_volume_t * volume);
volume_time_t volume_start_time(const amide_volume_t * volume, guint frame);
void volume_list_free(amide_volume_list_t ** pvolume_list);
amide_volume_list_t * volume_list_init(void);
gboolean volume_list_includes_volume(amide_volume_list_t *list, 
				     amide_volume_t * vol);
void volume_list_add_volume(amide_volume_list_t ** plist, amide_volume_t * vol);
void volume_list_add_volume_first(amide_volume_list_t ** plist, 
				  amide_volume_t * vol);
void volume_list_remove_volume(amide_volume_list_t ** plist, amide_volume_t * vol);
gboolean volume_includes_voxel(const amide_volume_t * volume,
			       const voxelpoint_t voxel);
void volume_get_view_corners(const amide_volume_t * volume,
			     const realspace_t view_coord_frame,
			     realpoint_t corner[]);
void volumes_get_view_corners(amide_volume_list_t * volumes,
			      const realspace_t view_coord_frame,
			      realpoint_t view_corner[]);
floatpoint_t volumes_min_dim(amide_volume_list_t * volumes);
floatpoint_t volumes_get_width(amide_volume_list_t * volumes, 
			      const realspace_t view_coord_frame);
floatpoint_t volumes_get_height(amide_volume_list_t * volumes, 
			       const realspace_t view_coord_frame);
floatpoint_t volumes_get_length(amide_volume_list_t * volumes, 
			       const realspace_t view_coord_frame);
amide_volume_t * volume_get_slice(const amide_volume_t * volume,
				  const volume_time_t start,
				  const volume_time_t duration,
				  const realpoint_t  requested_voxel_size,
				  const realspace_t slice_coord_frame,
				  const realpoint_t far_corner,
				  const interpolation_t interpolation);
amide_volume_list_t * volumes_get_slices(amide_volume_list_t * volumes,
					 const volume_time_t start,
					 const volume_time_t duration,
					 const floatpoint_t thickness,
					 const realspace_t view_coord_frame,
					 const floatpoint_t zoom,
					 const interpolation_t interpolation);


/* external variables */
extern gchar * interpolation_names[];
extern gchar * modality_names[];

