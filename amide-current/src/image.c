/* image.c
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
#include <gnome.h>
#include <matrix.h>
#include <limits.h>
#include <math.h>
#include "amide.h"
#include "realspace.h"
#include "volume.h"
#include "color_table.h"
#include "image.h"

/* external variables */
gchar * scaling_names[] = {"per slice", "global"};



/* function to return a blank image */
GdkImlibImage * image_blank(const gint width, const gint height) {
  
  GdkImlibImage * temp_image;
  gint i,j;
  guchar * rgb_data;
  
  if ((rgb_data = 
       (guchar *) g_malloc(sizeof(guchar) * 3 * width * height)) == NULL) {
    g_warning("%s: couldn't allocate memory for blank image",PACKAGE);
    return NULL;
  }


  for (i=0 ; i < height; i++)
    for (j=0; j < width; j++) {
      rgb_data[i*width*3+j*3+0] = 0x00;
      rgb_data[i*width*3+j*3+1] = 0x00;
      rgb_data[i*width*3+j*3+2] = 0x00;
    }
  
  temp_image = gdk_imlib_create_image_from_data(rgb_data, NULL, width, height);
  
  return temp_image;
}

/* function to make the bar graph to put next to the color_strip image */
GdkImlibImage * image_of_distribution(const amide_volume_t * volume,
				      const gint width, 
				      const gint height) {

  voxelpoint_t i;
  GdkImlibImage * temp_image;
  volume_data_t scale;
  guchar * rgb_data;
  guint frame;
  volume_data_t counter[height];
  guint32 j, k;
  volume_data_t max;


  if ((rgb_data = (guchar *) g_malloc(sizeof(guchar) * 3 * width * height)) == NULL) {
    g_warning("%s: couldn't allocate memory for rgb_data for bar_graph",PACKAGE);
    return NULL;
  }


  scale = height/(volume->max - volume->min);

  /* initialize the counter array */
  for (j = 0 ; j < height ; j++)
    counter[j] = 0.0;

  /* now "bin" the data */
  for (frame = 0; frame < volume->num_frames; frame++)
    for ( i.z = 0; i.z < volume->dim.z; i.z++)
      for (i.y = 0; i.y < volume->dim.y; i.y++) 
	for (i.x = 0; i.x < volume->dim.x; i.x++) {
	  j = rint(scale*VOLUME_CONTENTS(volume,frame,i));
	  counter[j]+=1.0;
	}
  

  /* normalize the counter array to the width */
  max = 0.0;

  /* ignore the lowest two bins */
  for (j = 0; j < height ; j++) 
    if (counter[j] > max)
      max = counter[j];
  
  /* do some scaling */
  for (j = 0; j < height ; j++) 
    counter[j] = log10(counter[j]+1.0);
  max = log10(max+1.0);
  scale = ((double) width)/max;
  for (j = 0 ; j < height ; j++)
    counter[j] = scale*counter[j];

  /* figure out what the rgb data is */
  for (j=0 ; j < height ; j++) {
    for (k=0; k < floor(((double) width)-counter[height-j-1]) ; k++) {
      rgb_data[j*width*3+k*3+0] = 0xFF;
      rgb_data[j*width*3+k*3+1] = 0xFF;
      rgb_data[j*width*3+k*3+2] = 0xFF;
    }
    for ( ; k < width ; k++) {
      rgb_data[j*width*3+k*3+0] = 0;
      rgb_data[j*width*3+k*3+1] = 0;
      rgb_data[j*width*3+k*3+2] = 0;
    }
  }
    
    
  temp_image = gdk_imlib_create_image_from_data(rgb_data, NULL, width, height);
  
  if (temp_image == NULL)
    g_error("NULL Distribution Image created\n");

  /* free up the rgb data */
  g_free(rgb_data);

  return temp_image;
}


/* function to make the color_strip image */
GdkImlibImage * image_from_colortable(const color_table_t color_table,
				      const gint width, 
				      const gint height,
				      const volume_data_t min,
				      const volume_data_t max,
				      const volume_data_t volume_min,
				      const volume_data_t volume_max) {

  guint i,j;
  color_point_t color;
  GdkImlibImage * temp_image;
  volume_data_t datum;
  guchar * rgb_data;

  if ((rgb_data = (guchar *) g_malloc(sizeof(guchar) * 3 * width * height)) == NULL) {
    g_warning("%s: couldn't allocate memory for rgb_data for color_strip",PACKAGE);
    return NULL;
  }


  for (j=0; j < height; j++) {
    datum = (((double) height-j)/height) * volume_max;
    if (datum >= max)
      datum = volume_max;
    else if (datum <= min)
      datum = 0.0;
    else
      datum = (datum-min)/
	((max-min)/(volume_max-volume_min));

    color = color_table_lookup(datum, color_table, volume_min, volume_max);
    for (i=0; i < width; i++) {
      rgb_data[j*width*3+i*3+0] = color.r;
      rgb_data[j*width*3+i*3+1] = color.g;
      rgb_data[j*width*3+i*3+2] = color.b;
    }
  }

  
  temp_image = gdk_imlib_create_image_from_data(rgb_data, NULL, width, height);
  g_free(rgb_data); /* no longer need the data */
  
  return temp_image;
}

GdkImlibImage * image_from_volume(amide_volume_t ** pslice,
				  const amide_volume_t * volume,
				  const guint frame,
				  const floatpoint_t zp_start,
				  const floatpoint_t thickness,
				  const realpoint_t axis[],
				  const scaling_t scaling,
				  const color_table_t color_table,
				  const floatpoint_t zoom,
				  const interpolation_t interpolation,
				  const volume_data_t threshold_min,
				  const volume_data_t threshold_max) {

  guchar * rgb_data;
  voxelpoint_t i;
  volume_data_t max,min,temp;
  GdkImlibImage * temp_image;
  color_point_t rgb_temp;
  amide_volume_t * slice;

  if ((*pslice) == NULL) {
    if ((slice = volume_get_slice(volume, frame, zp_start, thickness,
				  axis, zoom, interpolation)) == NULL) {
      g_warning("%s: slice is NULL?\n", PACKAGE);
      return NULL;
    } 
    *pslice = slice;
  } else 
    slice = (*pslice);

  /* get the max/min values for scaling */
  if (scaling == VOLUME) {
    max = threshold_max;
    min = threshold_min;
  } else {
    /* find the slice's max and min, and then adjust these values to
     * correspond to the current threshold values */
    max = 0.0;
    min = 0.0;
    i.z = 0;
    for (i.y = 0; i.y < (*pslice)->dim.y; i.y++) 
      for (i.x = 0; i.x < (*pslice)->dim.x; i.x++) {
	temp = VOLUME_CONTENTS(slice, frame, i);
	if (temp > max)
	  max = temp;
	else if (temp < min)
	  min = temp;
      }
    max = threshold_max*(max-min)/(volume->max-volume->min);
    min = threshold_min*(max-min)/(volume->max-volume->min);
  }

  if ((rgb_data = (guchar *) g_malloc(sizeof(guchar) * 3 * slice->dim.y *slice->dim.x)) == NULL) {
    g_warning("%s: couldn't allocate memory for rgb_data for image",PACKAGE);
    return NULL;
  }

  /* now convert our slice to rgb data */
  i.z = 0;
  for (i.y = 0; i.y < (slice)->dim.y; i.y++) 
    for (i.x = 0; i.x < (slice)->dim.x; i.x++) {
      rgb_temp = color_table_lookup(VOLUME_CONTENTS((slice),frame,i), 
				    color_table, min, max);
      rgb_data[i.y*(slice)->dim.x*3+i.x*3+0] = rgb_temp.r;
      rgb_data[i.y*(slice)->dim.x*3+i.x*3+1] = rgb_temp.g;
      rgb_data[i.y*(slice)->dim.x*3+i.x*3+2] = rgb_temp.b;
    }

  temp_image = gdk_imlib_create_image_from_data(rgb_data, NULL, 
						(slice)->dim.x, 
						(slice)->dim.y);
  g_free(rgb_data); /* no longer need the data */

  return temp_image;
}









