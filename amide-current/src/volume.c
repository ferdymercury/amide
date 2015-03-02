/* volume.c
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

#include "config.h"
#include <glib.h>
#include <math.h>
#include "amide.h"
#include "realspace.h"
#include "volume.h"

/* external variables */
gchar * interpolation_names[] = {"Nearest Neighbhor", \
				 "Bilinear", \
				 "Trilinear"};

/* free up a volume */
void volume_free(amide_volume_t ** pvolume) {

  if (*pvolume != NULL) {

    if((*pvolume)->data != NULL)
      g_free((*pvolume)->data);

    if((*pvolume)->time != NULL)
      g_free((*pvolume)->time);

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
  temp_volume->dimensionality=DIM3;
  temp_volume->voxel_size = realpoint_init;
  temp_volume->dim = voxelpoint_init;
  temp_volume->num_frames = 0;
  temp_volume->data = NULL;
  temp_volume->conversion = 1.0;
  temp_volume->time = NULL;
  for (i=0;i<NUM_VIEWS;i++)
    temp_volume->coord_frame.axis[i] = default_axis[i];
  temp_volume->corner = realpoint_init;

  return temp_volume;
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
			     const realpoint_t view_axis[],
			     realpoint_t view_corner[]) {

  realpoint_t corner, new_view_corner;
  realspace_t view_coord_frame;
  realpoint_t volume_corner[2];
  axis_t i;

  g_assert(volume!=NULL);

  /* set some values */
  view_coord_frame.offset = realpoint_init;
  for (i=0;i<NUM_AXIS;i++)
    view_coord_frame.axis[i] = view_axis[i];
  volume_corner[0] = volume->coord_frame.offset;
  volume_corner[1] = realspace_alt_coord_to_base(volume->corner,
						 volume->coord_frame);

  /* look at all eight corners of our cube, figure out the min and max coords */

  /* corner 0 */
  corner = volume_corner[0];
  new_view_corner = realspace_base_coord_to_alt(corner, view_coord_frame);
  view_corner[0] = view_corner[1] = new_view_corner;

  /* corner 1 */
  corner.x = volume_corner[0].x;
  corner.y = volume_corner[0].y;
  corner.z = volume_corner[1].z;
  new_view_corner = realspace_base_coord_to_alt(corner, view_coord_frame);
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
  new_view_corner = realspace_base_coord_to_alt(corner, view_coord_frame);
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
  new_view_corner = realspace_base_coord_to_alt(corner, view_coord_frame);
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
  new_view_corner = realspace_base_coord_to_alt(corner, view_coord_frame);
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
  new_view_corner = realspace_base_coord_to_alt(corner, view_coord_frame);
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
  new_view_corner = realspace_base_coord_to_alt(corner, view_coord_frame);
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
  new_view_corner = realspace_base_coord_to_alt(corner, view_coord_frame);
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

/* returns the width of the volume wrt the given axis */
floatpoint_t volume_get_width(const amide_volume_t * volume, 
			      const realpoint_t view_axis[]) {
  realpoint_t view_corner[2];
  realspace_t coord_frame;
  axis_t i;

  g_assert(volume!=NULL);
  coord_frame.offset = realpoint_init;
  for (i=0;i<NUM_AXIS;i++)
    coord_frame.axis[i] = view_axis[i];

  g_assert(volume!=NULL);
  volume_get_view_corners(volume,view_axis,view_corner);
  view_corner[0] = realspace_base_coord_to_alt(view_corner[0],coord_frame);
  view_corner[1] = realspace_base_coord_to_alt(view_corner[1],coord_frame);
  return fabs(view_corner[1].x-view_corner[0].x);
}


/* returns the length of the volume wrt the given axis */
floatpoint_t volume_get_height(const amide_volume_t * volume, 
			       const realpoint_t view_axis[]) {
  realpoint_t view_corner[2];
  realspace_t coord_frame;
  axis_t i;

  g_assert(volume!=NULL);
  coord_frame.offset = realpoint_init;
  for (i=0;i<NUM_AXIS;i++)
    coord_frame.axis[i] = view_axis[i];

  g_assert(volume!=NULL);
  volume_get_view_corners(volume,view_axis,view_corner);
  view_corner[0] = realspace_base_coord_to_alt(view_corner[0],coord_frame);
  view_corner[1] = realspace_base_coord_to_alt(view_corner[1],coord_frame);
  return fabs(view_corner[1].y-view_corner[0].y);
}

/* returns the length of the volume wrt the given axis */
floatpoint_t volume_get_length(const amide_volume_t * volume, 
			       const realpoint_t view_axis[]) {
  realpoint_t view_corner[2];
  realspace_t coord_frame;
  axis_t i;

  g_assert(volume!=NULL);
  coord_frame.offset = realpoint_init;
  for (i=0;i<NUM_AXIS;i++)
    coord_frame.axis[i] = view_axis[i];
  volume_get_view_corners(volume,view_axis,view_corner);
  view_corner[0] = realspace_base_coord_to_alt(view_corner[0],coord_frame);
  view_corner[1] = realspace_base_coord_to_alt(view_corner[1],coord_frame);
  return fabs(view_corner[1].z-view_corner[0].z);
}


/* returns a "2D" slice from a volume */
amide_volume_t * volume_get_slice(const amide_volume_t * volume,
				  const guint frame,
				  const floatpoint_t zp_start,
				  const floatpoint_t assigned_thickness,
				  const realpoint_t view_axis[],
				  const floatpoint_t zoom,
				  const interpolation_t interpolation) {

  /* thickness (of slice) in mm, negative indicates set this value
     according to voxel size */
  /* zp_start, where on the zp axis to start the slice, zp (z_prime) corresponds
     to the rotated axises, if negative, choose the midpoint */

  floatpoint_t thickness = assigned_thickness;
  amide_volume_t * return_slice;
  voxelpoint_t i,j;
  axis_t k;
  intpoint_t z;
  floatpoint_t zp;
  floatpoint_t real_min_dim, view_min_dim, view_length, max_diff, voxel_length;
  floatpoint_t real_value[8];
  realpoint_t view_size, real_size;
  guint dimx=0, dimy=0, dimz=0;
  realpoint_t view_corner[2];
  realpoint_t real_corner[2];
  realpoint_t volume_corner[2]; /* in real space */
  realpoint_t real_p,view_p, view_voxel_size,adj;
  realspace_t view_coord_frame;
  realpoint_t box_corner_view[8], box_corner_real[8];
  guint l;
  volume_data_t z_weight;

  /* sanity check */
  g_assert(volume != NULL);


  /* get some important dimensional information */
  real_min_dim = REALPOINT_MIN_DIM(volume->voxel_size);
  volume_corner[0] = volume->coord_frame.offset;
  volume_corner[1] = realspace_alt_coord_to_base(volume->corner,
						 volume->coord_frame);
  REALSPACE_DIFF(volume_corner[0],volume_corner[1],real_size);

  /* the new slice's size */
  volume_get_view_corners(volume,view_axis,view_corner);

  /* adjust the returned view corners into our view coord_frame */
  for (k=0;k<NUM_AXIS;k++)
    view_coord_frame.axis[k] = view_axis[k];
  view_coord_frame.offset = view_corner[0];
  view_corner[0] = realpoint_init;
  view_corner[1] = realspace_base_coord_to_alt(view_corner[1],view_coord_frame);
  
  /* calculate some pertinent values */
  view_size.x = fabs(view_corner[1].x);
  view_size.y = fabs(view_corner[1].y);
  view_length = fabs(view_corner[1].z);
  dimx = ceil(zoom*view_size.x/real_min_dim);
  dimy = ceil(zoom*view_size.y/real_min_dim);
  dimz = 1;

#ifdef AMIDE_DEBUG
  g_printerr("new slice---------------------\n");
  g_printerr("\tdim\t\tx %d\t\ty %d\t\tz %d\n",dimx,dimy,dimz);
#endif

  /* at what point to start the slice at (z prime normal) */
  if ((zp_start < 0.0) | (zp_start > view_length))
    zp = view_length/2.0;
  else
    zp = zp_start;

  /* figure out out thickness */
  if (thickness < 0) /* negative thickness means set according to min_dim */
    thickness = real_min_dim;
  if (thickness > view_length-zp) /* make sure we're not thicker then our vol */
    thickness = view_length - zp;

  /* readjust the coord_frame and corner value as we only want a slice*/
  adj.x = adj.y = 0.0;
  adj.z = zp;
  adj = realspace_alt_coord_to_base(adj,view_coord_frame);
  view_coord_frame.offset = adj;
  view_corner[1].z = thickness;
  view_size.z = fabs(view_corner[1].z);

  /* convert to real space */
  real_corner[0] = realspace_alt_coord_to_base(view_corner[0],
					       view_coord_frame);
  real_corner[1] = realspace_alt_coord_to_base(view_corner[1],
					       view_coord_frame);
  
#ifdef AMIDE_DEBUG
  g_printerr("\treal corner[0]\tx %5.4f\ty %5.4f\tz %5.4f\n",
	  real_corner[0].x,real_corner[0].y,real_corner[0].z);
  g_printerr("\treal corner[1]\tx %5.4f\ty %5.4f\tz %5.4f\n",
	  real_corner[1].x,real_corner[1].y,real_corner[1].z);
#endif

  return_slice = volume_init();
  return_slice->num_frames = 1;
  return_slice->dimensionality = DIM3;
  return_slice->dim.x = dimx;
  return_slice->dim.y = dimy;
  return_slice->dim.z = dimz;
  return_slice->voxel_size.x = view_size.x/dimx;
  return_slice->voxel_size.y = view_size.y/dimy;
  return_slice->voxel_size.z = view_size.z/dimz;
  view_min_dim = REALPOINT_MIN_DIM(return_slice->voxel_size);
  return_slice->coord_frame.offset = view_coord_frame.offset;
  for (k=0;k<NUM_AXIS;k++)
    return_slice->coord_frame.axis[k] = view_axis[k];
  return_slice->corner = view_corner[1];
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
  view_voxel_size.x = view_voxel_size.y = 0.0;
  view_voxel_size.z = 1.0;
  view_voxel_size = realspace_alt_dim_to_alt(view_voxel_size,
					     view_coord_frame,
					     volume->coord_frame);
  voxel_length = REALSPACE_DOT_PRODUCT(view_voxel_size,volume->voxel_size);

  /* initialize our volume */
  i.z = 0;
  for (i.y = 0; i.y < dimy; i.y++) 
    for (i.x = 0; i.x < dimx; i.x++) 
      VOLUME_SET_CONTENT(return_slice,0,i)=0.0;

  switch(interpolation) {
  case BILINEAR:
    for (z = 0; z < ceil(thickness/voxel_length); z++) {

      /* the view z_coordinate for this iteration's view voxel */
      view_p.z = z*voxel_length+view_min_dim/2.0;

      /* z_weight is between 0 and 1, this is used to weight the last voxel 
	 in the view z direction */
      z_weight = floor(thickness/voxel_length) > z*voxel_length ? 1.0 : 
	thickness/voxel_length-floor(thickness/voxel_length);

      for (i.y = 0; i.y < dimy; i.y++) {

	/* the view y_coordinate of the center of this iteration's view voxel */
	view_p.y = (((floatpoint_t) i.y)/dimy) * view_size.y + view_min_dim/2.0;

	/* the view x coord of the center of the first view voxel in this loop */
	view_p.x = view_min_dim/2.0;
	for (i.x = 0; i.x < dimx; i.x++) {

	  /* get the locations and values of the four voxels in the real volume
	     which are closest to our view voxel */

	  /* the four corners we're interested in (view space) */
	  box_corner_view[0].x = view_p.x-real_min_dim/2.0;
	  box_corner_view[0].y = view_p.y-real_min_dim/2.0;
	  box_corner_view[0].z = view_p.z;
	  box_corner_view[1].x = view_p.x+real_min_dim/2.0;
	  box_corner_view[1].y = view_p.y-real_min_dim/2.0;
	  box_corner_view[1].z = view_p.z;
	  box_corner_view[2].x = view_p.x-real_min_dim/2.0;
	  box_corner_view[2].y = view_p.y+real_min_dim/2.0;
	  box_corner_view[2].z = view_p.z;
	  box_corner_view[3].x = view_p.x+real_min_dim/2.0;
	  box_corner_view[3].y = view_p.y+real_min_dim/2.0;
	  box_corner_view[3].z = view_p.z;

	  /* find the closest voxels in real space and their values*/
	  for (l=0; l<4; l++) {
	    real_p= realspace_alt_coord_to_alt(box_corner_view[l],
					       view_coord_frame,
					       volume->coord_frame);
	    j.x = floor(((real_p.x-volume_corner[0].x)/real_size.x)*volume->dim.x);
	    j.y = floor(((real_p.y-volume_corner[0].y)/real_size.y)*volume->dim.y);
	    j.z = floor(((real_p.z-volume_corner[0].z)/real_size.z)*volume->dim.z);
	    real_p.x = (j.x*real_size.x)/volume->dim.x
	      +volume_corner[0].x + volume->voxel_size.x/2.0;
	    real_p.y = (j.y*real_size.y)/volume->dim.y
	      +volume_corner[0].y + volume->voxel_size.y/2.0;
	    real_p.z = (j.z*real_size.z)/volume->dim.z
	      +volume_corner[0].z + volume->voxel_size.z/2.0;
	    box_corner_real[l] = realspace_alt_coord_to_alt(real_p,
							    volume->coord_frame,
							    view_coord_frame);
	    /* get the value of the closest voxel in real space */
	    if (!volume_includes_voxel(volume,j))
	      real_value[l] = EMPTY;
	    else
	      real_value[l] = VOLUME_CONTENTS(volume,frame,j);
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
    for (z = 0; z < ceil(thickness/voxel_length); z++) {

      /* the view z_coordinate for this iteration's view voxel */
      view_p.z = z*voxel_length + view_min_dim/2.0;

      /* z_weight is between 0 and 1, this is used to weight the last voxel 
	 in the view z direction */
      z_weight = floor(thickness/voxel_length) > z*voxel_length ? 1.0 : 
	thickness/voxel_length-floor(thickness/voxel_length);

      for (i.y = 0; i.y < dimy; i.y++) {

	/* the view y_coordinate of the center of this iteration's view voxel */
	view_p.y = (((floatpoint_t) i.y)/dimy) * view_size.y + view_min_dim/2.0;

	/* the view x coord of the center of the first view voxel in this loop */
	view_p.x = view_min_dim/2.0;
	for (i.x = 0; i.x < dimx; i.x++) {

	  /* get the locations and values of the eight voxels in the real volume
	     which are closest to our view voxel */

	  /* the four corners we're interested in (view space) */
	  box_corner_view[0].x = view_p.x-real_min_dim/2.0;
	  box_corner_view[0].y = view_p.y-real_min_dim/2.0;
	  box_corner_view[0].z = view_p.z-real_min_dim/2.0;
	  box_corner_view[1].x = view_p.x+real_min_dim/2.0;
	  box_corner_view[1].y = view_p.y-real_min_dim/2.0;
	  box_corner_view[1].z = view_p.z-real_min_dim/2.0;
	  box_corner_view[2].x = view_p.x-real_min_dim/2.0;
	  box_corner_view[2].y = view_p.y+real_min_dim/2.0;
	  box_corner_view[2].z = view_p.z-real_min_dim/2.0;
	  box_corner_view[3].x = view_p.x+real_min_dim/2.0;
	  box_corner_view[3].y = view_p.y+real_min_dim/2.0;
	  box_corner_view[3].z = view_p.z-real_min_dim/2.0;
	  box_corner_view[4].x = view_p.x-real_min_dim/2.0;
	  box_corner_view[4].y = view_p.y-real_min_dim/2.0;
	  box_corner_view[4].z = view_p.z+real_min_dim/2.0;
	  box_corner_view[5].x = view_p.x+real_min_dim/2.0;
	  box_corner_view[5].y = view_p.y-real_min_dim/2.0;
	  box_corner_view[5].z = view_p.z+real_min_dim/2.0;
	  box_corner_view[6].x = view_p.x-real_min_dim/2.0;
	  box_corner_view[6].y = view_p.y+real_min_dim/2.0;
	  box_corner_view[6].z = view_p.z+real_min_dim/2.0;
	  box_corner_view[7].x = view_p.x+real_min_dim/2.0;
	  box_corner_view[7].y = view_p.y+real_min_dim/2.0;
	  box_corner_view[7].z = view_p.z+real_min_dim/2.0;

	  /* find the closest voxels in real space and their values*/
	  for (l=0; l<8; l++) {
	    real_p= realspace_alt_coord_to_alt(box_corner_view[l],
					       view_coord_frame,
					       volume->coord_frame);
	    j.x = floor(((real_p.x-volume_corner[0].x)/real_size.x)*volume->dim.x);
	    j.y = floor(((real_p.y-volume_corner[0].y)/real_size.y)*volume->dim.y);
	    j.z = floor(((real_p.z-volume_corner[0].z)/real_size.z)*volume->dim.z);
	    real_p.x = (j.x*real_size.x)/volume->dim.x
	      +volume_corner[0].x + volume->voxel_size.x/2.0;
	    real_p.y = (j.y*real_size.y)/volume->dim.y
	      +volume_corner[0].y + volume->voxel_size.y/2.0;
	    real_p.z = (j.z*real_size.z)/volume->dim.z
	      +volume_corner[0].z + volume->voxel_size.z/2.0;
	    box_corner_real[l] = realspace_alt_coord_to_alt(real_p,
							    volume->coord_frame,
							    view_coord_frame);
	    if (!volume_includes_voxel(volume,j))
	      real_value[l] = EMPTY;
	    else
	      real_value[l] = VOLUME_CONTENTS(volume,frame,j);
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
    for (z = 0; z < ceil(thickness/voxel_length); z++) {
      view_p.z = z*voxel_length+view_min_dim/2.0;
      z_weight = floor(thickness/voxel_length) > z*voxel_length ? 1.0 : 
	thickness/voxel_length-floor(thickness/voxel_length);
      for (i.y = 0; i.y < dimy; i.y++) {
	view_p.y = (((floatpoint_t) i.y)/dimy) * view_size.y + view_min_dim/2.0;
	view_p.x = view_min_dim/2.0;
	for (i.x = 0; i.x < dimx; i.x++) {
	  real_p= realspace_alt_coord_to_alt(view_p,
					     view_coord_frame,
					     volume->coord_frame);
	  j.x = floor(((real_p.x-volume_corner[0].x)/real_size.x)*volume->dim.x);
	  j.y = floor(((real_p.y-volume_corner[0].y)/real_size.y)*volume->dim.y);
	  j.z = floor(((real_p.z-volume_corner[0].z)/real_size.z)*volume->dim.z);
	  if (!volume_includes_voxel(volume,j))
	    VOLUME_SET_CONTENT(return_slice,0,i) += z_weight*EMPTY;
	  else
	    VOLUME_SET_CONTENT(return_slice,0,i) += 
	      z_weight*VOLUME_CONTENTS(volume,frame,j);
	  view_p.x += view_min_dim; /* jump forward one pixel */
	}
      }
    }
    break;
  }

  /* correct our volume for it's thickness */
  i.z = 0;
  for (i.y = 0; i.y < dimy; i.y++) 
    for (i.x = 0; i.x < dimx; i.x++) 
      VOLUME_SET_CONTENT(return_slice,0,i) /= (thickness/voxel_length);

  return return_slice;
}

