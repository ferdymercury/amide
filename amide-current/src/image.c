/* image.c
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

#include "config.h"
#include <gnome.h>
#include <matrix.h>
#include <limits.h>
#include <math.h>
#include "amide.h"
#include "color_table.h"
#include "volume.h"
#include "color_table2.h"
#include "rendering.h"
#include "image.h"

/* external variables */
gchar * scaling_names[] = {"per slice", "global"};



/* function to return a blank image */
GdkImlibImage * image_blank(const intpoint_t width, const intpoint_t height) {
  
  GdkImlibImage * temp_image;
  intpoint_t i,j;
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


/* function returns a GdkImlibImage from 8 bit data */
GdkImlibImage * image_from_8bit(const guchar * image, const intpoint_t width, const intpoint_t height,
				const color_table_t color_table) {
  
  GdkImlibImage * temp_image;
  intpoint_t i,j;
  guchar * rgb_data;
  color_point_t rgb_temp;
  guint location;

  if ((rgb_data = 
       (guchar *) g_malloc(sizeof(guchar) * 3 * width * height)) == NULL) {
    g_warning("%s: couldn't allocate memory for image from 8 bit data",PACKAGE);
    return NULL;
  }


  for (i=0 ; i < height; i++)
    for (j=0; j < width; j++) {
      rgb_temp = color_table_lookup(image[i*width+j], color_table, 0, RENDERING_DENSITY_MAX);
      location = i*width*3+j*3;
      rgb_data[location+0] = rgb_temp.r;
      rgb_data[location+1] = rgb_temp.r;
      rgb_data[location+2] = rgb_temp.r;
    }

  /* generate the GdkImlibImage from the rgb_data */
  temp_image = gdk_imlib_create_image_from_data(rgb_data, NULL, width, height);
  
  return temp_image;
}

/* function returns a GdkImlibImage from a rendering context */
GdkImlibImage * image_from_renderings(rendering_list_t * contexts, gint16 width, gint16 height) {

  gint16 * temp_data;
  guchar * rgb_data;
  voxelpoint_t i;
  color_point_t rgb_temp;
  guint location;
  GdkImlibImage * temp_image;

  /* malloc space for a temporary storage buffer */
  if ((temp_data = (guint16 *) g_malloc(sizeof(guint16) * 3 * width * height)) == NULL) {
    g_warning("%s: couldn't allocate memory for temp_data for transfering rendering to image",PACKAGE);
    return NULL;
  }
  /* and initialize it */
  i.z = 0;
  for (i.y = 0; i.y < height; i.y++) 
    for (i.x = 0; i.x < width; i.x++) {
      location = i.y*width*3+i.x*3;
      temp_data[location+0] = 0;
      temp_data[location+1] = 0;
      temp_data[location+2] = 0;
    }

  /* iterate through the contexts, tranfering the image data into the temp storage buffer */
  while (contexts != NULL) {
    i.z = 0;
    for (i.y = 0; i.y < height; i.y++) 
      for (i.x = 0; i.x < width; i.x++) {
	rgb_temp = color_table_lookup(contexts->rendering_context->image[i.x+i.y*width], 
				      contexts->rendering_context->volume->color_table, 
				      0, RENDERING_DENSITY_MAX);
	location = i.y*width*3+i.x*3;
	temp_data[location+0] += rgb_temp.r;
	temp_data[location+1] += rgb_temp.g;
	temp_data[location+2] += rgb_temp.b;
      }

    contexts = contexts->next;
  }

  /* malloc space for the true rgb buffer */
  if ((rgb_data = (guchar *) g_malloc(sizeof(guchar) * 3 * height * width)) == NULL) {
    g_warning("%s: couldn't allocate memory for rgb_data for rendering image",PACKAGE);
    g_free(temp_data);
    return NULL;
  }

  /* now convert our temp rgb data to real rgb data */
  i.z = 0;
  for (i.y = 0; i.y < height; i.y++) 
    for (i.x = 0; i.x < width; i.x++) {
      location = i.y*width*3+i.x*3;
      rgb_data[location+0] = (temp_data[location+0] < 0xFF) ? temp_data[location+0] : 0xFF;
      rgb_data[location+1] = (temp_data[location+1] < 0xFF) ? temp_data[location+1] : 0xFF;
      rgb_data[location+2] = (temp_data[location+2] < 0xFF) ? temp_data[location+2] : 0xFF;
    }

  /* from the rgb_data, generate a GdkImlibImage */
  temp_image = gdk_imlib_create_image_from_data(rgb_data, NULL, width, height);

  /* no longer need the data */
  g_free(rgb_data); 
  g_free(temp_data);

  return temp_image;
}

/* function to make the bar graph to put next to the color_strip image */
GdkImlibImage * image_of_distribution(volume_t * volume,
				      const intpoint_t width, 
				      const intpoint_t height) {

  voxelpoint_t i;
  GdkImlibImage * temp_image;
  volume_data_t scale;
  guchar * rgb_data;
  guint frame;
  intpoint_t j, k;
  volume_data_t max;


  if ((rgb_data = (guchar *) g_malloc(sizeof(guchar) * 3 * width * height)) == NULL) {
    g_warning("%s: couldn't allocate memory for rgb_data for bar_graph",PACKAGE);
    return NULL;
  }



  /* check if we've already calculated the data distribution */
  if (volume->distribution == NULL) {
    /* if not, calculate it now */

    scale = (height-1)/(volume->max - volume->min);

    if ((volume->distribution = 
	 (volume_data_t *) g_malloc(sizeof(volume_data_t) * height)) == NULL) {
      g_warning("%s: couldn't allocate memory for data distribution for bar_graph",PACKAGE);
      return NULL;
    }
    

    /* initialize the distribution array */
    for (j = 0 ; j < height ; j++)
      volume->distribution[j] = 0.0;

    /* now "bin" the data */
    for (frame = 0; frame < volume->num_frames; frame++)
      for ( i.z = 0; i.z < volume->dim.z; i.z++)
	for (i.y = 0; i.y < volume->dim.y; i.y++) 
	  for (i.x = 0; i.x < volume->dim.x; i.x++) {
	    j = rint(scale*(VOLUME_CONTENTS(volume,frame,i)-volume->min));
	    volume->distribution[j]+=1.0;
	  }
  
    /* normalize the distribution array to the width */

    /* figure out the max of the distribution */
    max = 0.0;
    for (j = 0; j < height ; j++) 
      if (volume->distribution[j] > max)
	max = volume->distribution[j];
  
    /* do some log scaling so the distribution is more meaningful, and doesn't get
       swamped by outlyers */
    for (j = 0; j < height ; j++) 
      volume->distribution[j] = log10(volume->distribution[j]+1.0);
    max = log10(max+1.0);
    scale = ((double) width)/max;
    for (j = 0 ; j < height ; j++)
      volume->distribution[j] = scale*volume->distribution[j];
  }

  /* figure out what the rgb data is */
  for (j=0 ; j < height ; j++) {
    for (k=0; k < floor(((double) width)-volume->distribution[height-j-1]) ; k++) {
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
				      const intpoint_t width, 
				      const intpoint_t height,
				      const volume_data_t min,
				      const volume_data_t max,
				      const volume_data_t volume_min,
				      const volume_data_t volume_max) {

  intpoint_t i,j;
  color_point_t color;
  GdkImlibImage * temp_image;
  volume_data_t datum;
  guchar * rgb_data;

  if ((rgb_data = (guchar *) g_malloc(sizeof(guchar) * 3 * width * height)) == NULL) {
    g_warning("%s: couldn't allocate memory for rgb_data for color_strip",PACKAGE);
    return NULL;
  }


  for (j=0; j < height; j++) {
    datum = ((((double) height-j)/height) * (volume_max-volume_min))+volume_min;
    if (datum >= max)
      datum = volume_max;
    else if (datum <= min)
      datum = volume_min;
    else
      datum = (volume_max-volume_min)*(datum-min)/(max-min)+volume_min;


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

GdkImlibImage * image_from_volumes(volume_list_t ** pslices,
				   volume_list_t * volumes,
				   const volume_time_t start,
				   const volume_time_t duration,
				   const floatpoint_t thickness,
				   const realspace_t view_coord_frame,
				   const scaling_t scaling,
				   const floatpoint_t zoom,
				   const interpolation_t interpolation) {

  guchar * rgb_data;
  guint16 * temp_data;
  guint location;
  voxelpoint_t i;
  voxelpoint_t dim;
  volume_data_t max,min;
  GdkImlibImage * temp_image;
  color_point_t rgb_temp;
  volume_list_t * slices;
  volume_list_t * temp_slices;
  volume_list_t * temp_volumes;

  /* sanity checks */
  g_return_val_if_fail(volumes != NULL, NULL);
#ifdef AMIDE_DEBUG
  if ((*pslices) != NULL) {
    temp_slices = *pslices;
    temp_volumes = volumes;
    while ((temp_slices != NULL) && (temp_volumes != NULL)) {
      temp_slices = temp_slices->next;
      temp_volumes = temp_volumes->next;
    }
    /* both should now be null */
    g_assert(temp_slices == NULL);
    g_assert(temp_volumes == NULL);
  }

#endif

  /* generate the slices if we need to */
  if ((*pslices) == NULL) {
    if ((slices = volumes_get_slices(volumes, start, duration, thickness, view_coord_frame, 
				     zoom, interpolation)) == NULL) {
      g_warning("%s: returned slices are NULL?\n", PACKAGE);
      return NULL;
    } 
    (*pslices) = slices;
  } else {
    slices = (*pslices);
  }

  /* get the dimensions.  since all slices have the same dimensions, we'll just get the first */
  dim = slices->volume->dim;

  /* malloc space for a temporary storage buffer */
  if ((temp_data = (guint16 *) g_malloc(sizeof(guint16) * 3 * dim.y * dim.x)) == NULL) {
    g_warning("%s: couldn't allocate memory for rgb_data for image",PACKAGE);
    return NULL;
  }
  /* and initialize it */
  i.z = 0;
  for (i.y = 0; i.y < dim.y; i.y++) 
    for (i.x = 0; i.x < dim.x; i.x++) {
      location = i.y*dim.x*3+i.x*3;
      temp_data[location+0] = 0;
      temp_data[location+1] = 0;
      temp_data[location+2] = 0;
    }

  /* iterate through all the slices */
  temp_slices = slices;
  temp_volumes = volumes;

  while (temp_slices != NULL) {


    /* get the max/min values for scaling */
    if (scaling == VOLUME) {
      max = temp_volumes->volume->threshold_max;
      min = temp_volumes->volume->threshold_min;
    } else {
      /* find the slice's max and min, and then adjust these values to
       * correspond to the current threshold values */
      max = temp_slices->volume->max;
      min = temp_slices->volume->min;
      max = temp_volumes->volume->threshold_max*(max-min)/
	(temp_volumes->volume->max-temp_volumes->volume->min);
      min = temp_volumes->volume->threshold_min*(max-min)/
	(temp_volumes->volume->max-temp_volumes->volume->min);
    }

    /* now add this slice into the rgb data */
    i.z = 0;
    for (i.y = 0; i.y < dim.y; i.y++) 
      for (i.x = 0; i.x < dim.x; i.x++) {
	rgb_temp = color_table_lookup(VOLUME_CONTENTS(temp_slices->volume,0,i), 
				      temp_volumes->volume->color_table, min, max);
	location = i.y*dim.x*3+i.x*3;
	temp_data[location+0] += rgb_temp.r;
	temp_data[location+1] += rgb_temp.g;
	temp_data[location+2] += rgb_temp.b;
      }

    temp_slices = temp_slices->next;
    temp_volumes = temp_volumes->next;
  }


  /* malloc space for the true rgb buffer */
  if ((rgb_data = (guchar *) g_malloc(sizeof(guchar) * 3 * dim.y *dim.x)) == NULL) {
    g_warning("%s: couldn't allocate memory for rgb_data for image",PACKAGE);
    return NULL;
  }

  /* now convert our temp rgb data to real rgb data */
  i.z = 0;
  for (i.y = 0; i.y < dim.y; i.y++) 
    for (i.x = 0; i.x < dim.x; i.x++) {
      location = i.y*dim.x*3+i.x*3;
      rgb_data[location+0] = (temp_data[location+0] < 0xFF) ? temp_data[location+0] : 0xFF;
      rgb_data[location+1] = (temp_data[location+1] < 0xFF) ? temp_data[location+1] : 0xFF;
      rgb_data[location+2] = (temp_data[location+2] < 0xFF) ? temp_data[location+2] : 0xFF;
    }

  

  /* from the rgb_data, generate a GdkImlibImage */
  temp_image = gdk_imlib_create_image_from_data(rgb_data, NULL,dim.x, dim.y);

  /* no longer need the data */
  g_free(rgb_data); 
  g_free(temp_data);

  return temp_image;
}









