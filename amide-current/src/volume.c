/* volume.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
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

#include "config.h"
#include <glib.h>
#include <math.h>
#include "amide.h"
#include "realspace.h"
#include "color_table.h"
#include "volume.h"

/* external variables */
gchar * interpolation_names[] = {"Nearest Neighbhor", \
				 "Bilinear", \
				 "Trilinear"};

gchar * modality_names[] = {"PET", \
			    "SPECT", \
			    "CT", \
			    "MRI", \
			    "Other"};

/* free up a volume */
void volume_free(amide_volume_t ** pvolume) {

  if (*pvolume != NULL) {

    if((*pvolume)->data != NULL)
      g_free((*pvolume)->data);

    if((*pvolume)->frame_duration != NULL)
      g_free((*pvolume)->frame_duration);

    if((*pvolume)->distribution != NULL)
      g_free((*pvolume)->distribution);

    g_free((*pvolume)->name);

    g_free(*pvolume);
    *pvolume = NULL;
  }

  return;
}

/* free up a list of volumes */
void volume_list_free(amide_volume_list_t ** pvolume_list) {
  
  if (*pvolume_list != NULL) {
    volume_free(&((*pvolume_list)->volume));
    if ((*pvolume_list)->next != NULL)
      volume_list_free(&((*pvolume_list)->next));
    g_free(*pvolume_list);
    *pvolume_list = NULL;
  }

  return;
}


/* returns an initialized volume structure */
amide_volume_t * volume_init(void) {

  amide_volume_t * temp_volume;
  axis_t i;

  if ((temp_volume = 
       (amide_volume_t *) g_malloc(sizeof(amide_volume_t))) == NULL) {
    return NULL;
  }

  temp_volume->name = NULL;
  temp_volume->modality = PET;
  temp_volume->voxel_size = realpoint_init;
  temp_volume->dim = voxelpoint_init;
  temp_volume->num_frames = 0;
  temp_volume->data = NULL;
  temp_volume->conversion = 1.0;
  temp_volume->scan_start = 0.0;
  temp_volume->frame_duration = NULL;
  temp_volume->color_table = BW_LINEAR;
  temp_volume->distribution = NULL;
  for (i=0;i<NUM_VIEWS;i++)
    temp_volume->coord_frame.axis[i] = default_axis[i];
  temp_volume->corner = realpoint_init;

  return temp_volume;
}

/* copies the information of one volume into another, if dest volume
   doesn't exist, make it. Does not copy data*/
void volume_copy(amide_volume_t ** dest_volume, amide_volume_t * src_volume) {

  /* sanity checks */
  g_assert(src_volume != NULL);
  g_assert(src_volume != *dest_volume);

  /* start over from scratch */
  volume_free(dest_volume);
  *dest_volume = volume_init();

  /* copy the data elements */
  (*dest_volume)->modality = src_volume->modality;
  (*dest_volume)->voxel_size = src_volume->voxel_size;
  (*dest_volume)->dim = src_volume->dim;
  (*dest_volume)->conversion = src_volume->conversion;
  (*dest_volume)->num_frames = src_volume->num_frames;
  (*dest_volume)->max = src_volume->max;
  (*dest_volume)->min = src_volume->min;
  (*dest_volume)->color_table= src_volume->color_table;
  (*dest_volume)->threshold_max = src_volume->threshold_max;
  (*dest_volume)->threshold_min = src_volume->threshold_min;
  (*dest_volume)->coord_frame = src_volume->coord_frame;
  (*dest_volume)->corner = src_volume->corner;

  /* make a separate copy in memory of the volume's name */
  volume_set_name(*dest_volume, src_volume->name);

  return;
}

/* sets the name of a volume
   note: new_name is copied rather then just being referenced by volume */
void volume_set_name(amide_volume_t * volume, gchar * new_name) {

  g_free(volume->name); /* free up the memory used by the old name */
  volume->name = g_strdup(new_name); /* and assign the new name */

  return;
}


/* figure out the center of the volume in real coords */
realpoint_t volume_calculate_center(const amide_volume_t * volume) {

  realpoint_t center;
  realpoint_t corner[2];

  /* get the far corner (in volume coords) */
  corner[0] = realspace_base_coord_to_alt(volume->coord_frame.offset, volume->coord_frame);
  corner[1] = volume->corner;
 
  /* the center in volume coords is then just half the far corner */
  REALSPACE_MADD(0.5,corner[1], 0.5,corner[0], center);
  
  /* now, translate that into real coords */
  center = realspace_alt_coord_to_base(center, volume->coord_frame);

  return center;
}

/* returns the start time of the given frame */
volume_time_t volume_start_time(const amide_volume_t * volume, guint frame) {

  volume_time_t time;
  guint i_frame;

  g_assert(frame < volume->num_frames);

  time = volume->scan_start;

  for(i_frame=0;i_frame<frame;i_frame++)
    time += volume->frame_duration[i_frame];

  return time;
}

/* returns an initialized volume list node structure */
amide_volume_list_t * volume_list_init(void) {
  
  amide_volume_list_t * temp_volume_list;
  
  if ((temp_volume_list = 
       (amide_volume_list_t *) g_malloc(sizeof(amide_volume_list_t))) == NULL) {
    return NULL;
  }
  
  temp_volume_list->next = NULL;
  
  return temp_volume_list;
}

/* function to check that a volume is in a volume list */
gboolean volume_list_includes_volume(amide_volume_list_t *list, 
				     amide_volume_t * volume) {

  while (list != NULL)
    if (list->volume == volume)
      return TRUE;
    else
      list = list->next;

  return FALSE;
}


/* function to add a volume onto a volume list list */
void volume_list_add_volume(amide_volume_list_t ** plist, amide_volume_t * vol) {

  amide_volume_list_t ** ptemp_list = plist;

  while (*ptemp_list != NULL)
    ptemp_list = &((*ptemp_list)->next);
  
  (*ptemp_list) = volume_list_init();
  (*ptemp_list)->volume = vol;

  return;
}


/* function to add a volume onto a volume list as the first item*/
void volume_list_add_volume_first(amide_volume_list_t ** plist, 
				  amide_volume_t * vol) {

  amide_volume_list_t * temp_list;

  temp_list = volume_list_init();
  temp_list->volume=vol;
  temp_list->next = *plist;
  *plist = temp_list;

  return;
}


/* function to remove a volume from a volume list, does not delete volume */
void volume_list_remove_volume(amide_volume_list_t ** plist, 
			       amide_volume_t * vol) {

  amide_volume_list_t * temp_list = *plist;
  amide_volume_list_t * prev_list = NULL;

  while (temp_list != NULL) {
    if (temp_list->volume == vol) {
      if (prev_list == NULL)
	*plist = temp_list->next;
      else
	prev_list->next = temp_list->next;
      temp_list->next = NULL;
      temp_list->volume = NULL;
      volume_list_free(&temp_list);
    } else {
      prev_list = temp_list;
      temp_list = temp_list->next;
    }
  }

  return;
}

gboolean volume_includes_voxel(const amide_volume_t * volume,
			       const voxelpoint_t voxel) {

  if ( (voxel.x < 0) | (voxel.y < 0) | (voxel.z < 0) |
       (voxel.x >= volume->dim.x) | (voxel.y >= volume->dim.y) |
       (voxel.z >= volume->dim.z))
    return FALSE;
  else
    return TRUE;
}

/* takes a volume and a view_axis, and gives the corners necessary for
   the view to totally encompass the volume in the base coord frame */
void volume_get_view_corners(const amide_volume_t * volume,
			     const realspace_t view_coord_frame,
			     realpoint_t view_corner[]) {

  realpoint_t corner, new_view_corner;
  realpoint_t volume_corner[2];

  g_assert(volume!=NULL);

  volume_corner[0] = realspace_base_coord_to_alt(volume->coord_frame.offset,
						 volume->coord_frame);
  volume_corner[1] = volume->corner;

  

  //  g_print("volume->coord_frame\n\t%5.3f %5.3f %5.3f\n\t%5.3f %5.3f %5.3f\n\t%5.3f %5.3f %5.3f\n",
  //	  volume->coord_frame.axis[0].x,volume->coord_frame.axis[0].y, volume->coord_frame.axis[0].z,
  //	  volume->coord_frame.axis[1].x,volume->coord_frame.axis[1].y, volume->coord_frame.axis[1].z,
  //	  volume->coord_frame.axis[2].x,volume->coord_frame.axis[2].y, volume->coord_frame.axis[2].z);

  //  g_print("view_coord_frame\n\t%5.3f %5.3f %5.3f\n\t%5.3f %5.3f %5.3f\n\t%5.3f %5.3f %5.3f\n",
  //	  view_coord_frame.axis[0].x,view_coord_frame.axis[0].y, view_coord_frame.axis[0].z,
  //	  view_coord_frame.axis[1].x,view_coord_frame.axis[1].y, view_coord_frame.axis[1].z,
  //	  view_coord_frame.axis[2].x,view_coord_frame.axis[2].y, view_coord_frame.axis[2].z);

  /* look at all eight corners of our cube, figure out the min and max coords */

  /* corner 0 */
  corner = volume_corner[0];
  new_view_corner = realspace_alt_coord_to_alt(corner, volume->coord_frame,view_coord_frame);
  view_corner[0] = view_corner[1] = new_view_corner;

  /* corner 1 */
  corner.x = volume_corner[0].x;
  corner.y = volume_corner[0].y;
  corner.z = volume_corner[1].z;
  new_view_corner = realspace_alt_coord_to_alt(corner, volume->coord_frame,view_coord_frame);
  if (view_corner[0].x > new_view_corner.x) view_corner[0].x = new_view_corner.x;
  if (view_corner[1].x < new_view_corner.x) view_corner[1].x = new_view_corner.x;
  if (view_corner[0].y > new_view_corner.y) view_corner[0].y = new_view_corner.y;
  if (view_corner[1].y < new_view_corner.y) view_corner[1].y = new_view_corner.y;
  if (view_corner[0].z > new_view_corner.z) view_corner[0].z = new_view_corner.z;
  if (view_corner[1].z < new_view_corner.z) view_corner[1].z = new_view_corner.z;

  /* corner 2 */
  corner.x = volume_corner[0].x;
  corner.y = volume_corner[1].y;
  corner.z = volume_corner[0].z;
  new_view_corner = realspace_alt_coord_to_alt(corner, volume->coord_frame,view_coord_frame);
  if (view_corner[0].x > new_view_corner.x) view_corner[0].x = new_view_corner.x;
  if (view_corner[1].x < new_view_corner.x) view_corner[1].x = new_view_corner.x;
  if (view_corner[0].y > new_view_corner.y) view_corner[0].y = new_view_corner.y;
  if (view_corner[1].y < new_view_corner.y) view_corner[1].y = new_view_corner.y;
  if (view_corner[0].z > new_view_corner.z) view_corner[0].z = new_view_corner.z;
  if (view_corner[1].z < new_view_corner.z) view_corner[1].z = new_view_corner.z;

  /* corner 3 */
  corner.x = volume_corner[0].x;
  corner.y = volume_corner[1].y;
  corner.z = volume_corner[1].z;
  new_view_corner = realspace_alt_coord_to_alt(corner, volume->coord_frame,view_coord_frame);
  if (view_corner[0].x > new_view_corner.x) view_corner[0].x = new_view_corner.x;
  if (view_corner[1].x < new_view_corner.x) view_corner[1].x = new_view_corner.x;
  if (view_corner[0].y > new_view_corner.y) view_corner[0].y = new_view_corner.y;
  if (view_corner[1].y < new_view_corner.y) view_corner[1].y = new_view_corner.y;
  if (view_corner[0].z > new_view_corner.z) view_corner[0].z = new_view_corner.z;
  if (view_corner[1].z < new_view_corner.z) view_corner[1].z = new_view_corner.z;

  /* corner 4 */
  corner.x = volume_corner[1].x;
  corner.y = volume_corner[0].y;
  corner.z = volume_corner[0].z;
  new_view_corner = realspace_alt_coord_to_alt(corner, volume->coord_frame,view_coord_frame);
  if (view_corner[0].x > new_view_corner.x) view_corner[0].x = new_view_corner.x;
  if (view_corner[1].x < new_view_corner.x) view_corner[1].x = new_view_corner.x;
  if (view_corner[0].y > new_view_corner.y) view_corner[0].y = new_view_corner.y;
  if (view_corner[1].y < new_view_corner.y) view_corner[1].y = new_view_corner.y;
  if (view_corner[0].z > new_view_corner.z) view_corner[0].z = new_view_corner.z;
  if (view_corner[1].z < new_view_corner.z) view_corner[1].z = new_view_corner.z;

  /* corner 5 */
  corner.x = volume_corner[1].x;
  corner.y = volume_corner[0].y;
  corner.z = volume_corner[1].z;
  new_view_corner = realspace_alt_coord_to_alt(corner, volume->coord_frame,view_coord_frame);
  if (view_corner[0].x > new_view_corner.x) view_corner[0].x = new_view_corner.x;
  if (view_corner[1].x < new_view_corner.x) view_corner[1].x = new_view_corner.x;
  if (view_corner[0].y > new_view_corner.y) view_corner[0].y = new_view_corner.y;
  if (view_corner[1].y < new_view_corner.y) view_corner[1].y = new_view_corner.y;
  if (view_corner[0].z > new_view_corner.z) view_corner[0].z = new_view_corner.z;
  if (view_corner[1].z < new_view_corner.z) view_corner[1].z = new_view_corner.z;

  /* corner 6 */
  corner.x = volume_corner[1].x;
  corner.y = volume_corner[1].y;
  corner.z = volume_corner[0].z;
  new_view_corner = realspace_alt_coord_to_alt(corner, volume->coord_frame,view_coord_frame);
  if (view_corner[0].x > new_view_corner.x) view_corner[0].x = new_view_corner.x;
  if (view_corner[1].x < new_view_corner.x) view_corner[1].x = new_view_corner.x;
  if (view_corner[0].y > new_view_corner.y) view_corner[0].y = new_view_corner.y;
  if (view_corner[1].y < new_view_corner.y) view_corner[1].y = new_view_corner.y;
  if (view_corner[0].z > new_view_corner.z) view_corner[0].z = new_view_corner.z;
  if (view_corner[1].z < new_view_corner.z) view_corner[1].z = new_view_corner.z;

  /* corner 7 */
  corner.x = volume_corner[1].x;
  corner.y = volume_corner[1].y;
  corner.z = volume_corner[1].z;
  new_view_corner = realspace_alt_coord_to_alt(corner, volume->coord_frame,view_coord_frame);
  if (view_corner[0].x > new_view_corner.x) view_corner[0].x = new_view_corner.x;
  if (view_corner[1].x < new_view_corner.x) view_corner[1].x = new_view_corner.x;
  if (view_corner[0].y > new_view_corner.y) view_corner[0].y = new_view_corner.y;
  if (view_corner[1].y < new_view_corner.y) view_corner[1].y = new_view_corner.y;
  if (view_corner[0].z > new_view_corner.z) view_corner[0].z = new_view_corner.z;
  if (view_corner[1].z < new_view_corner.z) view_corner[1].z = new_view_corner.z;

  /* and return the corners in the base coord frame */
  view_corner[0] = realspace_alt_coord_to_base(view_corner[0], view_coord_frame);
  view_corner[1] = realspace_alt_coord_to_base(view_corner[1], view_coord_frame);
  return;
}


/* takes a list of volumes and a view axis, and give the corners
   necessary to totally encompass the volume in the base coord frame */
void volumes_get_view_corners(amide_volume_list_t * volumes,
			      const realspace_t view_coord_frame,
			      realpoint_t view_corner[]) {

  amide_volume_list_t * temp_volumes;
  realpoint_t temp_corner[2];

  temp_volumes = volumes;
  volume_get_view_corners(temp_volumes->volume,view_coord_frame,view_corner);
  view_corner[0] = realspace_base_coord_to_alt(view_corner[0], view_coord_frame);
  view_corner[1] = realspace_base_coord_to_alt(view_corner[1], view_coord_frame);
  temp_volumes = volumes->next;
  //  g_print("corner [0] %5.3f %5.3f %5.3f [1] %5.3f %5.3f %5.3f\n",
  //	  view_corner[0].x,view_corner[0].y,view_corner[0].z,
  //	  view_corner[1].x,view_corner[1].y,view_corner[1].z);
  while (temp_volumes != NULL) {
    volume_get_view_corners(temp_volumes->volume,view_coord_frame,temp_corner);
    //  g_print("corner [0] %5.3f %5.3f %5.3f [1] %5.3f %5.3f %5.3f\n",
    //	  temp_corner[0].x,temp_corner[0].y,temp_corner[0].z,
    //	  temp_corner[1].x,temp_corner[1].y,temp_corner[1].z);
    temp_corner[0] = realspace_base_coord_to_alt(temp_corner[0], view_coord_frame);
    temp_corner[1] = realspace_base_coord_to_alt(temp_corner[1], view_coord_frame);
    view_corner[0].x = (view_corner[0].x < temp_corner[0].x) ? view_corner[0].x : temp_corner[0].x;
    view_corner[0].y = (view_corner[0].y < temp_corner[0].y) ? view_corner[0].y : temp_corner[0].y;
    view_corner[0].z = (view_corner[0].z < temp_corner[0].z) ? view_corner[0].z : temp_corner[0].z;
    view_corner[1].x = (view_corner[1].x > temp_corner[1].x) ? view_corner[1].x : temp_corner[1].x;
    view_corner[1].y = (view_corner[1].y > temp_corner[1].y) ? view_corner[1].y : temp_corner[1].y;
    view_corner[1].z = (view_corner[1].z > temp_corner[1].z) ? view_corner[1].z : temp_corner[1].z;
    temp_volumes = temp_volumes->next;
  }
  view_corner[0] = realspace_alt_coord_to_base(view_corner[0], view_coord_frame);
  view_corner[1] = realspace_alt_coord_to_base(view_corner[1], view_coord_frame);

  return;
}


/* returns the minimum dimensional width of the volume with the largest voxel size */
floatpoint_t volumes_min_dim(amide_volume_list_t * volumes) {

  amide_volume_list_t * temp_volumes;
  floatpoint_t min_dim, temp;

  g_assert(volumes!=NULL);

  /* figure out what our voxel size is going to be for our returned
     slices.  I'm going to base this on the volume with the largest
     minimum voxel dimension, this way the user doesn't suddenly get a
     huge image when adding in a study with small voxels (i.e. a CT
     scan).  The user can always increase the zoom later*/
  /* figure out out thickness */
  temp_volumes = volumes;
  min_dim = 0.0;
  while (temp_volumes != NULL) {
    temp = REALPOINT_MIN_DIM(temp_volumes->volume->voxel_size);
    if (temp > min_dim) min_dim = temp;
    temp_volumes = temp_volumes->next;
  }

  return min_dim;
}

/* returns the length of the assembly of volumes wrt the given axis */
floatpoint_t volumes_get_width(amide_volume_list_t * volumes, 
			       const realspace_t view_coord_frame) {
  amide_volume_list_t * temp_volumes;
  realpoint_t view_corner[2];
  realpoint_t temp_corner[2];

  g_assert(volumes!=NULL);

  temp_volumes = volumes;
  volume_get_view_corners(temp_volumes->volume,view_coord_frame,view_corner);
  temp_volumes = volumes->next;
  while (temp_volumes != NULL) {
    volume_get_view_corners(temp_volumes->volume,view_coord_frame,temp_corner);
    view_corner[0].x = (view_corner[0].x < temp_corner[0].x) ? view_corner[0].x : temp_corner[0].x;
    view_corner[0].y = (view_corner[0].y < temp_corner[0].y) ? view_corner[0].y : temp_corner[0].y;
    view_corner[0].z = (view_corner[0].z < temp_corner[0].z) ? view_corner[0].z : temp_corner[0].z;
    view_corner[1].x = (view_corner[1].x > temp_corner[1].x) ? view_corner[1].x : temp_corner[1].x;
    view_corner[1].y = (view_corner[1].y > temp_corner[1].y) ? view_corner[1].y : temp_corner[1].y;
    view_corner[1].z = (view_corner[1].z > temp_corner[1].z) ? view_corner[1].z : temp_corner[1].z;
    temp_volumes = temp_volumes->next;
  }

  view_corner[0] = realspace_base_coord_to_alt(view_corner[0],view_coord_frame);
  view_corner[1] = realspace_base_coord_to_alt(view_corner[1],view_coord_frame);
  return fabs(view_corner[1].x-view_corner[0].x);
}

/* returns the length of the assembly of volumes wrt the given axis */
floatpoint_t volumes_get_height(amide_volume_list_t * volumes, 
				const realspace_t view_coord_frame) {
  amide_volume_list_t * temp_volumes;
  realpoint_t view_corner[2];
  realpoint_t temp_corner[2];

  g_assert(volumes!=NULL);

  temp_volumes = volumes;
  volume_get_view_corners(temp_volumes->volume,view_coord_frame,view_corner);
  temp_volumes = volumes->next;
  while (temp_volumes != NULL) {
    volume_get_view_corners(temp_volumes->volume,view_coord_frame,temp_corner);
    view_corner[0].x = (view_corner[0].x < temp_corner[0].x) ? view_corner[0].x : temp_corner[0].x;
    view_corner[0].y = (view_corner[0].y < temp_corner[0].y) ? view_corner[0].y : temp_corner[0].y;
    view_corner[0].z = (view_corner[0].z < temp_corner[0].z) ? view_corner[0].z : temp_corner[0].z;
    view_corner[1].x = (view_corner[1].x > temp_corner[1].x) ? view_corner[1].x : temp_corner[1].x;
    view_corner[1].y = (view_corner[1].y > temp_corner[1].y) ? view_corner[1].y : temp_corner[1].y;
    view_corner[1].z = (view_corner[1].z > temp_corner[1].z) ? view_corner[1].z : temp_corner[1].z;
    temp_volumes = temp_volumes->next;
  }

  view_corner[0] = realspace_base_coord_to_alt(view_corner[0],view_coord_frame);
  view_corner[1] = realspace_base_coord_to_alt(view_corner[1],view_coord_frame);
  return fabs(view_corner[1].y-view_corner[0].y);
}

/* returns the length of the assembly of volumes wrt the given axis */
floatpoint_t volumes_get_length(amide_volume_list_t * volumes, 
			       const realspace_t view_coord_frame) {
  amide_volume_list_t * temp_volumes;
  realpoint_t view_corner[2];
  realpoint_t temp_corner[2];

  g_assert(volumes!=NULL);

  temp_volumes = volumes;
  volume_get_view_corners(temp_volumes->volume,view_coord_frame,view_corner);
  temp_volumes = volumes->next;
  while (temp_volumes != NULL) {
    volume_get_view_corners(temp_volumes->volume,view_coord_frame,temp_corner);
    view_corner[0].x = (view_corner[0].x < temp_corner[0].x) ? view_corner[0].x : temp_corner[0].x;
    view_corner[0].y = (view_corner[0].y < temp_corner[0].y) ? view_corner[0].y : temp_corner[0].y;
    view_corner[0].z = (view_corner[0].z < temp_corner[0].z) ? view_corner[0].z : temp_corner[0].z;
    view_corner[1].x = (view_corner[1].x > temp_corner[1].x) ? view_corner[1].x : temp_corner[1].x;
    view_corner[1].y = (view_corner[1].y > temp_corner[1].y) ? view_corner[1].y : temp_corner[1].y;
    view_corner[1].z = (view_corner[1].z > temp_corner[1].z) ? view_corner[1].z : temp_corner[1].z;
    temp_volumes = temp_volumes->next;
  }

  view_corner[0] = realspace_base_coord_to_alt(view_corner[0],view_coord_frame);
  view_corner[1] = realspace_base_coord_to_alt(view_corner[1],view_coord_frame);
  return fabs(view_corner[1].z-view_corner[0].z);
}


/* returns a "2D" slice from a volume */
amide_volume_t * volume_get_slice(const amide_volume_t * volume,
				  const volume_time_t start,
				  const volume_time_t duration,
				  const realpoint_t  requested_voxel_size,
				  const realspace_t slice_coord_frame,
				  const realpoint_t far_corner,
				  const interpolation_t interpolation) {

  /* zp_start, where on the zp axis to start the slice, zp (z_prime) corresponds
     to the rotated axises, if negative, choose the midpoint */

  floatpoint_t thickness = requested_voxel_size.z;
  amide_volume_t * return_slice;
  voxelpoint_t i,j;
  intpoint_t z;
  floatpoint_t volume_min_dim, view_min_dim, max_diff, voxel_length;
  floatpoint_t real_value[8];
  guint dimx=0, dimy=0, dimz=0;
  realpoint_t slice_corner;
  realpoint_t real_corner[2];
  realpoint_t volume_p,view_p, alt;
  realpoint_t box_corner_view[8], box_corner_real[8];
  guint l;
  volume_data_t z_weight, max, min, temp;
  guint start_frame, num_frames, i_frame;
  volume_time_t volume_start, volume_end;

  /* sanity check */
  g_assert(volume != NULL);

  /* get some important dimensional information */
  volume_min_dim = REALPOINT_MIN_DIM(volume->voxel_size);

  /* not quite optimal, you may miss voxels in the z direction in special cases (extended thickness
     slices where thickness is still the minimal dimension ) */
  alt = requested_voxel_size;
  alt.z = alt.x; /* we're ignoring the z dimension when getting the minimal voxel size */
  view_min_dim = REALPOINT_MIN_DIM(alt);

  /* calculate some pertinent values */
  dimx = ceil(fabs(far_corner.x)/view_min_dim);
  dimy = ceil(fabs(far_corner.y)/view_min_dim);
  dimz = 1;

  /* figure out what frames of this volume to include */
  start_frame = volume->num_frames + 1;
  i_frame = 0;
  while (i_frame < volume->num_frames) {
    volume_start = volume_start_time(volume,i_frame);
    if (start>volume_start)
      start_frame = i_frame;
    i_frame++;
  }
  if (start_frame >= volume->num_frames)
    start_frame = volume->num_frames-1;

  num_frames = volume->num_frames + 1;
  i_frame = start_frame;
  while ((i_frame < volume->num_frames) & (num_frames > volume->num_frames)) {
    volume_end = volume_start_time(volume,i_frame)+volume->frame_duration[i_frame];
    if ((start+duration) < volume_end)
      num_frames = i_frame-start_frame+1;
    i_frame++;
  }
  if (num_frames+start_frame > volume->num_frames)
    num_frames = volume->num_frames-start_frame;

  volume_start = volume_start_time(volume, start_frame);
  volume_end = volume_start_time(volume,start_frame+num_frames-1)
    +volume->frame_duration[start_frame+num_frames-1];

#ifdef AMIDE_DEBUG
  g_print("new slice from volume %s\t---------------------\n",volume->name);
  g_print("\tdim\t\tx %d\t\ty %d\t\tz %d\n",dimx,dimy,dimz);
#endif

  g_print("start %5.3f duration %5.3f\n",start, duration);

  /* make sure the slice thickness is set correctly */
  slice_corner = far_corner;
  slice_corner.z = thickness;

  /* convert to real space */
  real_corner[0] = slice_coord_frame.offset;
  real_corner[1] = realspace_alt_coord_to_base(slice_corner,
					       slice_coord_frame);
  
#ifdef AMIDE_DEBUG
  g_print("\treal corner[0]\tx %5.4f\ty %5.4f\tz %5.4f\n",
	  real_corner[0].x,real_corner[0].y,real_corner[0].z);
  g_print("\treal corner[1]\tx %5.4f\ty %5.4f\tz %5.4f\n",
	  real_corner[1].x,real_corner[1].y,real_corner[1].z);
  g_print("\tvolume\tstart\t%5.4f\tend\t%5.3f\tframes %d-%d\n",
	  volume_start, volume_end,start_frame,start_frame+num_frames);
#endif
  return_slice = volume_init();
  return_slice->num_frames = 1;
  return_slice->dim.x = dimx;
  return_slice->dim.y = dimy;
  return_slice->dim.z = dimz;
  return_slice->voxel_size = requested_voxel_size;
  return_slice->coord_frame = slice_coord_frame;
  return_slice->corner = slice_corner;
  return_slice->scan_start = volume_start;
  return_slice->frame_duration = 
    (volume_time_t *) g_malloc(1*sizeof(volume_time_t));
  if (return_slice->frame_duration == NULL) {
    g_warning("%s: couldn't allocate space for the slice frame duration array\n",PACKAGE);
    volume_free(&return_slice);
    return NULL;
  }
  return_slice->frame_duration[0] = volume_end-volume_start;
  return_slice->data = 
    (volume_data_t *) g_malloc(return_slice->dim.z* 
			       return_slice->dim.y* 
			       return_slice->dim.x*
			       sizeof(volume_data_t));
  if (return_slice->data == NULL) {
    g_warning("%s: couldn't allocate space for the slice\n",PACKAGE);
    volume_free(&return_slice);
    return NULL;
  }

  /* get the length of a voxel for the given view axis*/
  alt.x = alt.y = 0.0;
  alt.z = 1.0;
  alt = realspace_alt_dim_to_alt(alt, slice_coord_frame, volume->coord_frame);
  voxel_length = REALSPACE_DOT_PRODUCT(alt,volume->voxel_size);


  /* initialize our volume */
  i.z = 0;
  for (i.y = 0; i.y < dimy; i.y++) 
    for (i.x = 0; i.x < dimx; i.x++) 
      VOLUME_SET_CONTENT(return_slice,0,i)=0.0;

  switch(interpolation) {
  case BILINEAR:
    for (i_frame = start_frame; i_frame < start_frame+num_frames;i_frame++)
      for (z = 0; z < ceil(thickness/voxel_length); z++) {
	
	/* the view z_coordinate for this iteration's view voxel */
	view_p.z = z*voxel_length+view_min_dim/2.0;
	
	/* z_weight is between 0 and 1, this is used to weight the last voxel 
	   in the view z direction */
	z_weight = floor(thickness/voxel_length) > z*voxel_length ? 1.0 : 
	  thickness/voxel_length-floor(thickness/voxel_length);
	
	for (i.y = 0; i.y < dimy; i.y++) {

	  /* the view y_coordinate of the center of this iteration's view voxel */
	  view_p.y = (((floatpoint_t) i.y)/dimy) * slice_corner.y + view_min_dim/2.0;
	  
	  /* the view x coord of the center of the first view voxel in this loop */
	  view_p.x = view_min_dim/2.0;
	  for (i.x = 0; i.x < dimx; i.x++) {
	    
	    /* get the locations and values of the four voxels in the real volume
	       which are closest to our view voxel */
	    
	    /* the four corners we're interested in (view space) */
	    box_corner_view[0].x = view_p.x-volume_min_dim/2.0;
	    box_corner_view[0].y = view_p.y-volume_min_dim/2.0;
	    box_corner_view[0].z = view_p.z;
	    box_corner_view[1].x = view_p.x+volume_min_dim/2.0;
	    box_corner_view[1].y = view_p.y-volume_min_dim/2.0;
	    box_corner_view[1].z = view_p.z;
	    box_corner_view[2].x = view_p.x-volume_min_dim/2.0;
	    box_corner_view[2].y = view_p.y+volume_min_dim/2.0;
	    box_corner_view[2].z = view_p.z;
	    box_corner_view[3].x = view_p.x+volume_min_dim/2.0;
	    box_corner_view[3].y = view_p.y+volume_min_dim/2.0;
	    box_corner_view[3].z = view_p.z;
	    
	    /* find the closest voxels in real space and their values*/
	    for (l=0; l<4; l++) {
	      volume_p= realspace_alt_coord_to_alt(box_corner_view[l],
						   slice_coord_frame,
						   volume->coord_frame);
	      j.x = floor((volume_p.x/volume->corner.x)*volume->dim.x);
	      j.y = floor((volume_p.y/volume->corner.y)*volume->dim.y);
	      j.z = floor((volume_p.z/volume->corner.z)*volume->dim.z);
	      volume_p.x = (j.x*volume->corner.x)/volume->dim.x + volume->voxel_size.x/2.0;
	      volume_p.y = (j.y*volume->corner.y)/volume->dim.y + volume->voxel_size.y/2.0;
	      volume_p.z = (j.z*volume->corner.z)/volume->dim.z + volume->voxel_size.z/2.0;
	      box_corner_real[l] = realspace_alt_coord_to_alt(volume_p,
							      volume->coord_frame,
							      slice_coord_frame);
	      /* get the value of the closest voxel in real space */
	      if (!volume_includes_voxel(volume,j))
		real_value[l] = EMPTY;
	      else
		real_value[l] = VOLUME_CONTENTS(volume,i_frame,j);
	    }
	    
	    /* do the x direction linear interpolation of the sets of two points */
	    for (l=0;l<4;l=l+2) {
	      if ((view_p.x > box_corner_real[l].x) & 
		  (view_p.x > box_corner_real[l+1].x))
		real_value[l] = box_corner_real[l].x > box_corner_real[l+1].x ? 
		  real_value[l] : real_value[l+1];
	      else if ((view_p.x<box_corner_real[l].x) & 
		       (view_p.x<box_corner_real[l+1].x))
		real_value[l] = box_corner_real[l].x < box_corner_real[l+1].x ? 
		  real_value[l] : real_value[l+1];
	      else {
		max_diff = fabs(box_corner_real[l].x-box_corner_real[l+1].x);
		if (max_diff > 0.0+CLOSE) {
		  real_value[l] =
		    + (max_diff - fabs(box_corner_real[l].x-view_p.x))
		    *real_value[l]
		    + (max_diff - fabs(box_corner_real[l+1].x-view_p.x))
		    *real_value[l+1];
		  real_value[l] = real_value[l]/max_diff;
		}
	      }
	    }

	    /* do the y direction interpolations of the pairs of x interpolations */
	    for (l=0;l<4;l=l+4) {
	      if ((view_p.y > box_corner_real[l].y) & 
		  (view_p.y > box_corner_real[l+2].y))
		real_value[l] = box_corner_real[l].y > box_corner_real[l+2].y ? 
		  real_value[l] : real_value[l+2];
	      else if ((view_p.y<box_corner_real[l].y) & 
		       (view_p.y<box_corner_real[l+2].y))
		real_value[l] = box_corner_real[l].y < box_corner_real[l+2].y ? 
		  real_value[l] : real_value[l+2];
	      else {
		max_diff = fabs(box_corner_real[l].y-box_corner_real[l+2].y);
		if (max_diff > 0.0+CLOSE) {
		  real_value[l] = 
		    + (max_diff - fabs(box_corner_real[l].y-view_p.y)) 
		    * real_value[l]
		    + (max_diff - fabs(box_corner_real[l+2].y-view_p.y))
		    *real_value[l+2];
		  real_value[l] = real_value[l]/max_diff;
		}
	      }
	    }		
	    
	    VOLUME_SET_CONTENT(return_slice,0,i)+=z_weight*real_value[0];
	    
	    /* jump forward one voxel in the view x direction*/
	    view_p.x += return_slice->voxel_size.x; 
	  }
	}
      }
    break;
  case TRILINEAR:
    for (i_frame = start_frame; i_frame < start_frame+num_frames;i_frame++)
      for (z = 0; z < ceil(thickness/voxel_length); z++) {

	/* the view z_coordinate for this iteration's view voxel */
	view_p.z = z*voxel_length + view_min_dim/2.0;

	/* z_weight is between 0 and 1, this is used to weight the last voxel 
	   in the view z direction */
	z_weight = floor(thickness/voxel_length) > z*voxel_length ? 1.0 : 
	  thickness/voxel_length-floor(thickness/voxel_length);

	for (i.y = 0; i.y < dimy; i.y++) {

	  /* the view y_coordinate of the center of this iteration's view voxel */
	  view_p.y = (((floatpoint_t) i.y)/dimy) * slice_corner.y + view_min_dim/2.0;

	  /* the view x coord of the center of the first view voxel in this loop */
	  view_p.x = view_min_dim/2.0;
	  for (i.x = 0; i.x < dimx; i.x++) {

	    /* get the locations and values of the eight voxels in the real volume
	       which are closest to our view voxel */

	    /* the four corners we're interested in (view space) */
	    box_corner_view[0].x = view_p.x-volume_min_dim/2.0;
	    box_corner_view[0].y = view_p.y-volume_min_dim/2.0;
	    box_corner_view[0].z = view_p.z-volume_min_dim/2.0;
	    box_corner_view[1].x = view_p.x+volume_min_dim/2.0;
	    box_corner_view[1].y = view_p.y-volume_min_dim/2.0;
	    box_corner_view[1].z = view_p.z-volume_min_dim/2.0;
	    box_corner_view[2].x = view_p.x-volume_min_dim/2.0;
	    box_corner_view[2].y = view_p.y+volume_min_dim/2.0;
	    box_corner_view[2].z = view_p.z-volume_min_dim/2.0;
	    box_corner_view[3].x = view_p.x+volume_min_dim/2.0;
	    box_corner_view[3].y = view_p.y+volume_min_dim/2.0;
	    box_corner_view[3].z = view_p.z-volume_min_dim/2.0;
	    box_corner_view[4].x = view_p.x-volume_min_dim/2.0;
	    box_corner_view[4].y = view_p.y-volume_min_dim/2.0;
	    box_corner_view[4].z = view_p.z+volume_min_dim/2.0;
	    box_corner_view[5].x = view_p.x+volume_min_dim/2.0;
	    box_corner_view[5].y = view_p.y-volume_min_dim/2.0;
	    box_corner_view[5].z = view_p.z+volume_min_dim/2.0;
	    box_corner_view[6].x = view_p.x-volume_min_dim/2.0;
	    box_corner_view[6].y = view_p.y+volume_min_dim/2.0;
	    box_corner_view[6].z = view_p.z+volume_min_dim/2.0;
	    box_corner_view[7].x = view_p.x+volume_min_dim/2.0;
	    box_corner_view[7].y = view_p.y+volume_min_dim/2.0;
	    box_corner_view[7].z = view_p.z+volume_min_dim/2.0;

	    /* find the closest voxels in real space and their values*/
	    for (l=0; l<8; l++) {
	      volume_p= realspace_alt_coord_to_alt(box_corner_view[l],
						   slice_coord_frame,
						   volume->coord_frame);
	      j.x = floor(((volume_p.x)/volume->corner.x)*volume->dim.x);
	      j.y = floor(((volume_p.y)/volume->corner.y)*volume->dim.y);
	      j.z = floor(((volume_p.z)/volume->corner.z)*volume->dim.z);
	      volume_p.x = (j.x*volume->corner.x)/volume->dim.x + volume->voxel_size.x/2.0;
	      volume_p.y = (j.y*volume->corner.y)/volume->dim.y + volume->voxel_size.y/2.0;
	      volume_p.z = (j.z*volume->corner.z)/volume->dim.z + volume->voxel_size.z/2.0;
	      box_corner_real[l] = realspace_alt_coord_to_alt(volume_p,
							      volume->coord_frame,
							      slice_coord_frame);
	      if (!volume_includes_voxel(volume,j))
		real_value[l] = EMPTY;
	      else
		real_value[l] = VOLUME_CONTENTS(volume,i_frame,j);
	    }

	    /* do the x direction linear interpolation of the sets of two points */
	    for (l=0;l<8;l=l+2) {
	      if ((view_p.x > box_corner_real[l].x) 
		  & (view_p.x > box_corner_real[l+1].x))
		real_value[l] = box_corner_real[l].x > box_corner_real[l+1].x ? 
		  real_value[l] : real_value[l+1];
	      else if ((view_p.x<box_corner_real[l].x) 
		       & (view_p.x<box_corner_real[l+1].x))
		real_value[l] = box_corner_real[l].x < box_corner_real[l+1].x ? 
		  real_value[l] : real_value[l+1];
	      else {
		max_diff = fabs(box_corner_real[l].x-box_corner_real[l+1].x);
		if (max_diff > 0.0+CLOSE) {
		  real_value[l] =
		    + (max_diff - fabs(box_corner_real[l].x-view_p.x)) 
		    * real_value[l]
		    + (max_diff - fabs(box_corner_real[l+1].x-view_p.x))
		    *real_value[l+1];
		  real_value[l] = real_value[l]/max_diff;
		}
	      }
	    }

	    /* do the y direction interpolations of the pairs of x interpolations */
	    for (l=0;l<8;l=l+4) {
	      if ((view_p.y > box_corner_real[l].y) 
		  & (view_p.y > box_corner_real[l+2].y))
		real_value[l] = box_corner_real[l].y > box_corner_real[l+2].y ? 
		  real_value[l] : real_value[l+2];
	      else if ((view_p.y<box_corner_real[l].y) 
		       & (view_p.y<box_corner_real[l+2].y))
		real_value[l] = box_corner_real[l].y < box_corner_real[l+2].y ? 
		  real_value[l] : real_value[l+2];
	      else {
		max_diff = fabs(box_corner_real[l].y-box_corner_real[l+2].y);
		if (max_diff > 0.0+CLOSE) {
		  real_value[l] =
		    + (max_diff - fabs(box_corner_real[l].y-view_p.y)) 
		    * real_value[l]
		    + (max_diff - fabs(box_corner_real[l+2].y-view_p.y))
		    * real_value[l+2];
		  real_value[l] = real_value[l]/max_diff;
		}
	      }
	    }

	    /* do the z direction interpolations of the pairs of y interpolations */
	    for (l=0;l<8;l=l+8) {
	      if ((view_p.z > box_corner_real[l].z) 
		  & (view_p.z > box_corner_real[l+4].z))
		real_value[l] = box_corner_real[l].z > box_corner_real[l+4].z ? 
		  real_value[l] : real_value[l+4];
	      else if ((view_p.z<box_corner_real[l].z) 
		       & (view_p.z<box_corner_real[l+4].z))
		real_value[l] = box_corner_real[l].z < box_corner_real[l+4].z ? 
		  real_value[l] : real_value[l+4];
	      else {
		max_diff = fabs(box_corner_real[l].z-box_corner_real[l+4].z);
		if (max_diff > 0.0+CLOSE) {
		  real_value[l] =
		    + (max_diff - fabs(box_corner_real[l].z-view_p.z)) 
		    * real_value[l]
		    + (max_diff - fabs(box_corner_real[l+4].z-view_p.z))
		    * real_value[l+4];
		  real_value[l] = real_value[l]/max_diff;
		}
	      }
	    }

	    VOLUME_SET_CONTENT(return_slice,0,i)+=z_weight*real_value[0];

	    /* jump forward one voxel in the view x direction*/
	    view_p.x += return_slice->voxel_size.x; 
	  }
	}
      }
    break;
  case NEAREST_NEIGHBOR:
  default:
    /* nearest neighbor algorithm */
    for (i_frame = start_frame; i_frame < start_frame+num_frames;i_frame++)
      for (z = 0; z < ceil(thickness/voxel_length); z++) {

	view_p.z = z*voxel_length+view_min_dim/2.0;
	z_weight = floor(thickness/voxel_length) > z*voxel_length ? 1.0 : 
	  thickness/voxel_length-floor(thickness/voxel_length);

	for (i.y = 0; i.y < dimy; i.y++) {

	  view_p.y = (((floatpoint_t) i.y)/dimy) * slice_corner.y + view_min_dim/2.0;
	  view_p.x = view_min_dim/2.0;

	  for (i.x = 0; i.x < dimx; i.x++) {

	    volume_p= realspace_alt_coord_to_alt(view_p,
						 slice_coord_frame,
						 volume->coord_frame);
	    j.x = floor((volume_p.x/volume->corner.x)*volume->dim.x);
	    j.y = floor((volume_p.y/volume->corner.y)*volume->dim.y);
	    j.z = floor((volume_p.z/volume->corner.z)*volume->dim.z);

	    if (!volume_includes_voxel(volume,j))
	      VOLUME_SET_CONTENT(return_slice,0,i) += z_weight*EMPTY;
	    else
	      VOLUME_SET_CONTENT(return_slice,0,i) += 
		z_weight*VOLUME_CONTENTS(volume,i_frame,j);
	    view_p.x += view_min_dim; /* jump forward one pixel */
	  }
	}
      }
    break;
  }

  /* correct our volume for it's thickness and the number of frames*/
  i.z = 0;
  for (i.y = 0; i.y < dimy; i.y++) 
    for (i.x = 0; i.x < dimx; i.x++) 
      VOLUME_SET_CONTENT(return_slice,0,i) /= (num_frames*(thickness/voxel_length));


  /* and figure out the max and min */
  max = 0.0;
  min = 0.0;
  i.z = 0;
  for (i.y = 0; i.y < return_slice->dim.y; i.y++) 
    for (i.x = 0; i.x < return_slice->dim.x; i.x++) {
      temp = VOLUME_CONTENTS(return_slice, 0, i);
      if (temp > max)
	max = temp;
      else if (temp < min)
	min = temp;
    }
  return_slice->max = max;
  return_slice->min = min;

  return return_slice;
}



/* give a list ov volumes, returns a list of slices of equal size and orientation
   intersecting these volumes */
amide_volume_list_t * volumes_get_slices(amide_volume_list_t * volumes,
					 const volume_time_t start,
					 const volume_time_t duration,
					 const floatpoint_t thickness,
					 const realspace_t view_coord_frame,
					 const floatpoint_t zoom,
					 const interpolation_t interpolation) {


  axis_t i_axis;
  realpoint_t voxel_size;
  realspace_t slice_coord_frame;
  realpoint_t view_corner[2];
  realpoint_t slice_corner;
  amide_volume_list_t * temp_volumes;
  amide_volume_list_t * temp_slices;
  amide_volume_list_t * slices=NULL;
  volume_data_t min_dim;

  /* thickness (of slice) in mm, negative indicates set this value
     according to voxel size */

  /* sanity check */
  g_assert(volumes != NULL);

  /* get the minimum voxel dimension of this list of volumes */
  min_dim = volumes_min_dim(volumes);
  g_return_val_if_fail(min_dim > 0.0, NULL);

  /* compensate for zoom */
  min_dim = (1/zoom) * min_dim;
  voxel_size.x = voxel_size.y = min_dim;
  voxel_size.z = (thickness > 0) ? thickness : min_dim;

  /* figure out all encompasing corners for the slices based on our viewing axis */
  volumes_get_view_corners(volumes, view_coord_frame, view_corner);


  /* adjust the z' dimension of the view's corners */
  view_corner[0] = realspace_base_coord_to_alt(view_corner[0],view_coord_frame);
  view_corner[0].z=0;
  view_corner[0] = realspace_alt_coord_to_base(view_corner[0],view_coord_frame);

  view_corner[1] = realspace_base_coord_to_alt(view_corner[1],view_coord_frame);
  view_corner[1].z=voxel_size.z;
  view_corner[1] = realspace_alt_coord_to_base(view_corner[1],view_coord_frame);

  /* figure out the coordinate frame for the slices based on the corners returned */
  for(i_axis=0;i_axis<NUM_AXIS;i_axis++) 
    slice_coord_frame.axis[i_axis] = view_coord_frame.axis[i_axis];
  slice_coord_frame.offset = view_corner[0];
  slice_corner = realspace_base_coord_to_alt(view_corner[1],slice_coord_frame);

  /* and get the slices */
  temp_volumes = volumes;
  temp_slices = volume_list_init();
  slices = temp_slices;
  temp_slices->volume = volume_get_slice(temp_volumes->volume, start, duration, voxel_size, 
					 slice_coord_frame, slice_corner, interpolation);
  temp_volumes = temp_volumes->next;
  while (temp_volumes != NULL) {
    temp_slices->next = volume_list_init();
    temp_slices = temp_slices->next;
    temp_slices->volume = volume_get_slice(temp_volumes->volume, start, duration, voxel_size, 
					   slice_coord_frame, slice_corner, interpolation);
    temp_volumes = temp_volumes->next;
  }

  return slices;
}

















