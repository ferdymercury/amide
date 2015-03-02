/* volume.h
 *
 * Part of amide - Amide's a Medical Image Dataset Viewer
 * Copyright (C) 2000 Andy Loening
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
typedef enum {DIM2, DIM3} dimensionality_t;
typedef enum {NEAREST_NEIGHBOR, BILINEAR, TRILINEAR, NUM_INTERPOLATIONS} interpolation_t;
typedef enum {PET, SPECT, CT, MRI, NUM_MODALITIES} modality_t;

/* setup the types for various internal data formats */
typedef gdouble volume_data_t;
typedef gdouble volume_time_t;

typedef struct _amide_volume_list_t amide_volume_list_t;

/* the amide volume structure */
typedef struct amide_volume_t { 
  gchar * name;
  modality_t modality;
  dimensionality_t dimensionality;
  realpoint_t voxel_size;  /* in mm */
  voxelpoint_t dim;
  volume_data_t * data;
  volume_data_t conversion; /* factor  to translate data into useable units */
  guint num_frames;
  volume_time_t * time;
  volume_data_t max;
  volume_data_t min;
  realspace_t coord_frame;
  realpoint_t corner; /* far corner, near corner is 0,0,0 in volume coord space*/ 
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
			     const realpoint_t view_axis[],
			     realpoint_t corner[]);
floatpoint_t volume_get_width(const amide_volume_t * volume, 
			      const realpoint_t view_axis[]);
floatpoint_t volume_get_height(const amide_volume_t * volume, 
			       const realpoint_t view_axis[]);
floatpoint_t volume_get_length(const amide_volume_t * volume, 
			       const realpoint_t view_axis[]);
amide_volume_t * volume_get_slice(const amide_volume_t * volume,
				  const guint frame,
				  const floatpoint_t zp_start,
				  const floatpoint_t thickness,
				  const realpoint_t view_axis[],
				  const floatpoint_t zoom,
				  const interpolation_t interpolation);


/* external variables */
extern gchar * interpolation_names[];























