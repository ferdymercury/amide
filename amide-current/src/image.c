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
#include <math.h>
#include "amide.h"
#include "image.h"
#include "color_table2.h"

/* callback function used for freeing the pixel data in a gdkpixbuf */
void image_free_rgb_data(guchar * pixels, gpointer data) {
  g_free(pixels);
  return;
}

/* function to return a blank image */
GdkPixbuf * image_blank(const intpoint_t width, const intpoint_t height, rgba_t image_color) {
  
  GdkPixbuf * temp_image;
  intpoint_t i,j;
  guchar * rgb_data;
  
  if ((rgb_data = 
       (guchar *) g_malloc(sizeof(guchar) * 3 * width * height)) == NULL) {
    g_warning("%s: couldn't allocate memory for rgb_data for blank image",PACKAGE);
    return NULL;
  }

  for (i=0 ; i < height; i++)
    for (j=0; j < width; j++) {
      rgb_data[i*width*3+j*3+0] = image_color.r;
      rgb_data[i*width*3+j*3+1] = image_color.g;
      rgb_data[i*width*3+j*3+2] = image_color.b;
    }
  
  temp_image = gdk_pixbuf_new_from_data(rgb_data, GDK_COLORSPACE_RGB,
					FALSE,8,width,height,width*3*sizeof(guchar),
					image_free_rgb_data, NULL);

  return temp_image;
}


#ifdef AMIDE_LIBVOLPACK_SUPPORT
/* function returns a GdkPixbuf from 8 bit data */
GdkPixbuf * image_from_8bit(const guchar * image, const intpoint_t width, const intpoint_t height,
			    const color_table_t color_table) {
  
  GdkPixbuf * temp_image;
  intpoint_t i,j;
  guchar * rgb_data;
  rgba_t rgba_temp;
  guint location;

  if ((rgb_data = 
       (guchar *) g_malloc(sizeof(guchar) * 3 * width * height)) == NULL) {
    g_warning("%s: couldn't allocate memory for image from 8 bit data",PACKAGE);
    return NULL;
  }


  for (i=0 ; i < height; i++)
    for (j=0; j < width; j++) {
      /* note, line below compensates for X's origin being top left, not bottom left */
      rgba_temp = color_table_lookup(image[(height-i-1)*width+j], color_table, 
				     0, RENDERING_DENSITY_MAX);
      location = i*width*3+j*3;
      rgb_data[location+0] = rgba_temp.r;
      rgb_data[location+1] = rgba_temp.r;
      rgb_data[location+2] = rgba_temp.r;
    }

  /* generate the GdkPixbuf from the rgb_data */
  temp_image = gdk_pixbuf_new_from_data(rgb_data, GDK_COLORSPACE_RGB,
					FALSE,8,width,height,width*3*sizeof(guchar),
					image_free_rgb_data, NULL);
  
  return temp_image;
}

/* function returns a GdkPixbuf from a rendering context */
GdkPixbuf * image_from_renderings(rendering_list_t * contexts, gint16 width, gint16 height) {

  rgba_t * rgba_data;
  guchar * rgb_data;
  voxelpoint_t i;
  rgba_t rgba_temp;
  guint location;
  gdouble total_alpha;
  gdouble initial_alpha;
  gdouble new_alpha;
  GdkPixbuf * temp_image;
  gint context_num;

  /* malloc space for a temporary storage buffer */
  if ((rgba_data = (rgba_t *) g_malloc(sizeof(rgba_t) * width * height)) == NULL) {
    g_warning("%s: couldn't allocate memory for rgba_data for transfering rendering to image",PACKAGE);
    return NULL;
  }
  /* and initialize it */
  i.z = 0;
  for (i.y = 0; i.y < height; i.y++) 
    for (i.x = 0; i.x < width; i.x++) {
      location = i.y*width+i.x;
      rgba_data[location].r = 0;
      rgba_data[location].g = 0;
      rgba_data[location].b = 0;
      rgba_data[location].a = 0;
    }

  /* iterate through the contexts, tranfering the image data into the temp storage buffer */
  context_num = 0;
  while (contexts != NULL) {
    context_num++; 

    i.t = i.z = 0;
    for (i.y = 0; i.y < height; i.y++) 
      for (i.x = 0; i.x < width; i.x++) {
	rgba_temp = color_table_lookup(contexts->rendering_context->image[i.x+i.y*width], 
				      contexts->rendering_context->volume->color_table, 
				      0, RENDERING_DENSITY_MAX);
	/* compensate for the fact that X defines the origin as top left, not bottom left */
	location = (height-i.y-1)*width+i.x;
	total_alpha = (context_num-1.0) * rgba_data[location].a + rgba_temp.a;
	if (total_alpha <= CLOSE) {
	  total_alpha = CLOSE;
	  initial_alpha = (context_num-1.0)*CLOSE/context_num;
	  new_alpha = CLOSE/context_num;
	} else {
	  initial_alpha = (context_num-1.0) * rgba_data[location].a;
	  new_alpha = rgba_temp.a;
	}
	rgba_data[location].r = 
	  ((initial_alpha* rgba_data[location].r) +
	   (new_alpha * rgba_temp.r)) /
	  total_alpha;
	rgba_data[location].g = 
	  ((initial_alpha* rgba_data[location].g) +
	   (new_alpha * rgba_temp.g)) /
	  total_alpha;
	rgba_data[location].b = 
	  ((initial_alpha* rgba_data[location].b) +
	   (new_alpha * rgba_temp.b)) /
	  total_alpha;
	rgba_data[location].a = total_alpha;
      }
    
    contexts = contexts->next;
  }

  /* malloc space for the true rgb buffer */
  if ((rgb_data = (guchar *) g_malloc(sizeof(guchar) * 3 * height * width)) == NULL) {
    g_warning("%s: couldn't allocate memory for rgba_data for rendering image",PACKAGE);
    g_free(rgba_data);
    return NULL;
  }

  /* now convert our temp rgb data to real rgb data */
  i.z = 0;
  for (i.y = 0; i.y < height; i.y++) 
    for (i.x = 0; i.x < width; i.x++) {
      location = i.y*width+i.x;
      rgb_data[3*location+0] = rgba_data[location].r;
      rgb_data[3*location+1] = rgba_data[location].g;
      rgb_data[3*location+2] = rgba_data[location].b;
    }

  /* from the rgb_data, generate a GdkPixbuf */
  temp_image = gdk_pixbuf_new_from_data(rgb_data, GDK_COLORSPACE_RGB,
					FALSE,8,width,height,width*3*sizeof(guchar),
					image_free_rgb_data, NULL);

  /* free up any unneeded allocated memory */
  g_free(rgba_data);

  return temp_image;
}
#endif

/* function to make the bar graph to put next to the color_strip image */
GdkPixbuf * image_of_distribution(volume_t * volume) {

  GdkPixbuf * temp_image;
  guchar * rgb_data;
  intpoint_t k,l;
  voxelpoint_t j;
  amide_data_t max, scale;


  /* make sure we have a distribution calculated (won't the first time we're called with a vol)*/
  volume_generate_distribution(volume);

  if ((rgb_data = (guchar *) g_malloc(sizeof(guchar) * 3 * IMAGE_DISTRIBUTION_WIDTH * 
				      volume->distribution->dim.x)) == NULL) {
    g_warning("%s: couldn't allocate memory for rgb_data for bar_graph",PACKAGE);
    return NULL;
  }

  /* figure out the max of the distribution, so we can normalize the distribution to the width */
  max = 0.0;
  j.t = j.z = j.y = 0;
  for (j.x = 0; j.x < volume->distribution->dim.x ; j.x++) 
    if (*DATA_SET_FLOAT_POINTER(volume->distribution,j) > max)
      max = *DATA_SET_FLOAT_POINTER(volume->distribution,j);
  scale = ((double) IMAGE_DISTRIBUTION_WIDTH)/max;

  /* figure out what the rgb data is */
  j.t = j.z = j.y = 0;
  for (l=0 ; l < volume->distribution->dim.x ; l++) {
    j.x = volume->distribution->dim.x-l-1;
    for (k=0; k < floor(((double) IMAGE_DISTRIBUTION_WIDTH)-
			scale*(*DATA_SET_FLOAT_POINTER(volume->distribution,j))) ; k++) {
      rgb_data[l*IMAGE_DISTRIBUTION_WIDTH*3+k*3+0] = 0xFF;
      rgb_data[l*IMAGE_DISTRIBUTION_WIDTH*3+k*3+1] = 0xFF;
      rgb_data[l*IMAGE_DISTRIBUTION_WIDTH*3+k*3+2] = 0xFF;
    }
    for ( ; k < IMAGE_DISTRIBUTION_WIDTH ; k++) {
      rgb_data[l*IMAGE_DISTRIBUTION_WIDTH*3+k*3+0] = 0;
      rgb_data[l*IMAGE_DISTRIBUTION_WIDTH*3+k*3+1] = 0;
      rgb_data[l*IMAGE_DISTRIBUTION_WIDTH*3+k*3+2] = 0;
    }
  }
    
  /* generate the pixbuf image */
  temp_image = gdk_pixbuf_new_from_data(rgb_data, GDK_COLORSPACE_RGB,
					FALSE,8,IMAGE_DISTRIBUTION_WIDTH,
					volume->distribution->dim.x,
					IMAGE_DISTRIBUTION_WIDTH*3*sizeof(guchar),
					image_free_rgb_data, NULL);
  
  if (temp_image == NULL) g_error("NULL Distribution Image created\n");

  return temp_image;
}


/* function to make the color_strip image */
GdkPixbuf * image_from_colortable(const color_table_t color_table,
				  const intpoint_t width, 
				  const intpoint_t height,
				  const amide_data_t min,
				  const amide_data_t max,
				  const amide_data_t volume_min,
				  const amide_data_t volume_max) {

  intpoint_t i,j;
  rgba_t rgba;
  GdkPixbuf * temp_image;
  amide_data_t datum;
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


    rgba = color_table_lookup(datum, color_table, volume_min, volume_max);
    for (i=0; i < width; i++) {
      rgb_data[j*width*3+i*3+0] = rgba.r;
      rgb_data[j*width*3+i*3+1] = rgba.g;
      rgb_data[j*width*3+i*3+2] = rgba.b;
    }
  }

  
  temp_image = gdk_pixbuf_new_from_data(rgb_data, GDK_COLORSPACE_RGB,
					FALSE,8,width,height,width*3*sizeof(guchar),
					image_free_rgb_data, NULL);
  
  return temp_image;
}


GdkPixbuf * image_from_volumes(volume_list_t ** pslices,
			       volume_list_t * volumes,
			       const amide_time_t start,
			       const amide_time_t duration,
			       const floatpoint_t thickness,
			       const realspace_t view_coord_frame,
			       const scaling_t scaling,
			       const floatpoint_t zoom,
			       const interpolation_t interpolation) {

  guint slice_num;
  gdouble total_alpha;
  gdouble initial_alpha;
  gdouble new_alpha;
  guchar * rgb_data;
  rgba_t * rgba_data;
  guint location;
  voxelpoint_t i;
  voxelpoint_t dim;
  amide_data_t max,min;
  GdkPixbuf * temp_image;
  rgba_t rgba_temp;
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
				     zoom, interpolation, TRUE)) == NULL) {
      g_warning("%s: returned slices are NULL?", PACKAGE);
      return NULL;
    } 
    (*pslices) = slices;
  } else {
    slices = (*pslices);
  }

  /* get the dimensions.  since all slices have the same dimensions, we'll just get the first */
  dim = slices->volume->data_set->dim;

  /* malloc space for a temporary storage buffer */
  if ((rgba_data = (rgba_t *) g_malloc(sizeof(rgba_data) * dim.y * dim.x)) == NULL) {
    g_warning("%s: couldn't allocate memory for rgba_data for image",PACKAGE);
    return NULL;
  }
  /* and initialize it */
  i.z = 0;
  for (i.y = 0; i.y < dim.y; i.y++) 
    for (i.x = 0; i.x < dim.x; i.x++) {
      location = i.y*dim.x+i.x;
      rgba_data[location].r = 0;
      rgba_data[location].g = 0;
      rgba_data[location].b = 0;
      rgba_data[location].a = 0;
    }

  /* iterate through all the slices */
  temp_slices = slices;
  temp_volumes = volumes;

  slice_num = 0;
  while (temp_slices != NULL) {
    slice_num++;

    /* get the max/min values for scaling */
    if (scaling == SCALING_GLOBAL) {
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
    i.t = i.z = 0;
    for (i.y = 0; i.y < dim.y; i.y++) 
      for (i.x = 0; i.x < dim.x; i.x++) {
	rgba_temp = color_table_lookup(VOLUME_FLOAT_0D_SCALING_CONTENTS(temp_slices->volume,i), 
				       temp_volumes->volume->color_table, min, max);
	/* compensate for the fact that X defines the origin as top left, not bottom left */
	location = (dim.y - i.y - 1)*dim.x+i.x;
	total_alpha = (slice_num-1.0) * rgba_data[location].a + rgba_temp.a;
	if (total_alpha <= CLOSE) {
	  total_alpha = CLOSE;
	  initial_alpha = (slice_num-1.0)*CLOSE/slice_num;
	  new_alpha = CLOSE/slice_num;
	} else {
	  initial_alpha = (slice_num-1.0) * rgba_data[location].a;
	  new_alpha = rgba_temp.a;
	}
	rgba_data[location].r = 
	  ((initial_alpha* rgba_data[location].r) +
	   (new_alpha * rgba_temp.r)) /
	  total_alpha;
	rgba_data[location].g = 
	  ((initial_alpha* rgba_data[location].g) +
	   (new_alpha * rgba_temp.g)) /
	  total_alpha;
	rgba_data[location].b = 
	  ((initial_alpha* rgba_data[location].b) +
	   (new_alpha * rgba_temp.b)) /
	  total_alpha;
	rgba_data[location].a = total_alpha;
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
      location = i.y*dim.x+i.x;
      rgb_data[3*location+0] = rgba_data[location].r;
      rgb_data[3*location+1] = rgba_data[location].g;
      rgb_data[3*location+2] = rgba_data[location].b;
    }

  

  /* from the rgb_data, generate a GdkPixbuf */
  temp_image = gdk_pixbuf_new_from_data(rgb_data, GDK_COLORSPACE_RGB,
					FALSE,8,dim.x,dim.y,dim.x*3*sizeof(guchar),
					image_free_rgb_data, NULL);

  /* cleanup */
  g_free(rgba_data);

  return temp_image;
}









