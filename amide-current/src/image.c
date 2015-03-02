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
#include "image.h"
#include "../pixmaps/PET.xpm"
#include "../pixmaps/SPECT.xpm"
#include "../pixmaps/CT.xpm"
#include "../pixmaps/MRI.xpm"
#include "../pixmaps/OTHER.xpm"
#include "../pixmaps/CYLINDER.xpm"
#include "../pixmaps/BOX.xpm"
#include "../pixmaps/ELLIPSOID.xpm"
#include "../pixmaps/ISOCONTOUR_2D.xpm"
#include "../pixmaps/ISOCONTOUR_3D.xpm"


/* callback function used for freeing the pixel data in a gdkpixbuf */
static void image_free_rgb_data(guchar * pixels, gpointer data) {
  g_free(pixels);
  return;
}



GdkPixbuf * image_slice_intersection(const roi_t * roi, const volume_t * slice, rgba_t color,
				     realspace_t * return_frame,
				     realpoint_t * return_corner) {

  GdkPixbuf * temp_image;
  volume_t * intersection;
  voxelpoint_t i;
  guchar * rgba_data;
  voxelpoint_t dim;

  
  if ((intersection = roi_get_slice(roi, slice)) == NULL) return NULL;

  dim = intersection->data_set->dim;

  if ((rgba_data = (guchar *) g_malloc(4*sizeof(guchar)*dim.x*dim.y)) == NULL) {
    g_warning("%s: couldn't allocate memory for rgba_data for roi image",PACKAGE);
    return NULL;
  }

  *return_frame = intersection->coord_frame;
  *return_corner = intersection->corner;

  i.z = i.t = 0;
  for (i.y=0 ; i.y < dim.y; i.y++)
    for (i.x=0 ; i.x < dim.x; i.x++)
      if (*DATA_SET_UBYTE_POINTER(intersection->data_set, i) == 1) {
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+0] = color.r;
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+1] = color.g;
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+2] = color.b;
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+3] = 0xFF;
      }	else {
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+0] = 0x00;
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+1] = 0x00;
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+2] = 0x00;
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+3] = 0x00;
      }

  temp_image = gdk_pixbuf_new_from_data(rgba_data, GDK_COLORSPACE_RGB, TRUE,8,
					dim.x,dim.y,dim.x*4*sizeof(guchar),
					image_free_rgb_data, 
					NULL);

  intersection = volume_free(intersection);

  return temp_image;
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
GdkPixbuf * image_from_contexts(rendering_list_t * contexts, 
				gint16 image_width, gint16 image_height,
				eye_t eyes, 
				gdouble eye_angle, 
				gint16 eye_width) {

  gint image_num;
  guint32 total_alpha;
  rgba16_t * rgba16_data;
  guchar * char_data;
  voxelpoint_t i;
  rgba_t rgba_temp;
  guint location;
  GdkPixbuf * temp_image;
  gint total_width;
  gdouble rot;
  eye_t i_eye;

  total_width = image_width+(eyes-1)*eye_width;

  /* malloc space for a temporary storage buffer */
  if ((rgba16_data = (rgba16_t *) g_malloc(sizeof(rgba16_t) * total_width * image_height)) == NULL) {
    g_warning("%s: couldn't allocate memory for rgba16_data for transfering rendering to image",PACKAGE);
    return NULL;
  }
  /* and initialize it */
  i.z = 0;
  for (i.y = 0; i.y < image_height; i.y++) 
    for (i.x = 0; i.x < total_width; i.x++) {
      location = i.y*total_width+i.x;
      rgba16_data[location].r = 0;
      rgba16_data[location].g = 0;
      rgba16_data[location].b = 0;
      rgba16_data[location].a = 0;
    }

  /* iterate through the eyes and contexts, tranfering the image data into the temp storage buffer */
  image_num = 0; 

  while (contexts != NULL) {

    for (i_eye = 0; i_eye < eyes; i_eye ++) {
      image_num++; 

      if (eyes == 1) rot = 0.0;
      else rot = (-0.5 + i_eye*1.0) * eye_angle;
      rot = M_PI*rot/180; /* convert to radians */

      rendering_context_set_rotation(contexts->rendering_context, YAXIS, -rot);
      rendering_context_render(contexts->rendering_context);
      rendering_context_set_rotation(contexts->rendering_context, YAXIS, rot);

      i.t = i.z = 0;
      for (i.y = 0; i.y < image_height; i.y++) 
	for (i.x = 0; i.x < image_width; i.x++) {
	  rgba_temp = color_table_lookup(contexts->rendering_context->image[i.x+i.y*image_width], 
					 contexts->rendering_context->color_table, 
					 0, RENDERING_DENSITY_MAX);
	  /* compensate for the fact that X defines the origin as top left, not bottom left */
	  location = (image_height-i.y-1)*total_width+i.x+i_eye*eye_width;
	  total_alpha = rgba16_data[location].a + rgba_temp.a;
	  if (total_alpha == 0) {
	    rgba16_data[location].r = (((image_num-1)*rgba16_data[location].r + 
					rgba_temp.r)/
				       ((gdouble) image_num));
	    rgba16_data[location].g = (((image_num-1)*rgba16_data[location].g + 
					rgba_temp.g)/
				       ((gdouble) image_num));
	    rgba16_data[location].b = (((image_num-1)*rgba16_data[location].b + 
					rgba_temp.b)/
				       ((gdouble) image_num));
	  } else if (rgba16_data[location].a == 0) {
	    rgba16_data[location].r = rgba_temp.r;
	    rgba16_data[location].g = rgba_temp.g;
	    rgba16_data[location].b = rgba_temp.b;
	    rgba16_data[location].a = rgba_temp.a;
	  } else if (rgba_temp.a != 0) {
	    rgba16_data[location].r = ((rgba16_data[location].r*rgba16_data[location].a 
					+ rgba_temp.r*rgba_temp.a)/
				       ((double) total_alpha));
	  rgba16_data[location].g = ((rgba16_data[location].g*rgba16_data[location].a 
				      + rgba_temp.g*rgba_temp.a)/
				     ((double) total_alpha));
	  rgba16_data[location].b = ((rgba16_data[location].b*rgba16_data[location].a 
				      + rgba_temp.b*rgba_temp.a)/
				     ((double) total_alpha));
	  rgba16_data[location].a = total_alpha;
	  }
	}
    }      
    contexts = contexts->next;
  }

  /* malloc space for the true rgb buffer */
  if ((char_data = (guchar *) g_malloc(3*sizeof(guchar) * image_height * total_width)) == NULL) {
    g_warning("%s: couldn't allocate memory for char_data for rendering image",PACKAGE);
    g_free(rgba16_data);
    return NULL;
  }

  /* now convert our temp rgb data to real rgb data */
  i.z = 0;
  for (i.y = 0; i.y < image_height; i.y++) 
    for (i.x = 0; i.x < total_width; i.x++) {
      location = i.y*total_width+i.x;
      char_data[3*location+0] = rgba16_data[location].r < 0xFF ? rgba16_data[location].r : 0xFF;
      char_data[3*location+1] = rgba16_data[location].g < 0xFF ? rgba16_data[location].g : 0xFF;
      char_data[3*location+2] = rgba16_data[location].b < 0xFF ? rgba16_data[location].b : 0xFF;
    }

  /* from the rgb_data, generate a GdkPixbuf */
  temp_image = gdk_pixbuf_new_from_data(char_data, GDK_COLORSPACE_RGB,FALSE,8,
					total_width,image_height,total_width*3*sizeof(guchar),
					image_free_rgb_data, NULL);
  
  /* free up any unneeded allocated memory */
  g_free(rgba16_data);
  
  return temp_image;
}
#endif

/* function to make the bar graph to put next to the color_strip image */
GdkPixbuf * image_of_distribution(volume_t * volume, rgb_t fg, rgb_t bg) {

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
      rgb_data[l*IMAGE_DISTRIBUTION_WIDTH*3+k*3+0] = bg.r;
      rgb_data[l*IMAGE_DISTRIBUTION_WIDTH*3+k*3+1] = bg.g;
      rgb_data[l*IMAGE_DISTRIBUTION_WIDTH*3+k*3+2] = bg.b;
    }
    for ( ; k < IMAGE_DISTRIBUTION_WIDTH ; k++) {
      rgb_data[l*IMAGE_DISTRIBUTION_WIDTH*3+k*3+0] = fg.r;
      rgb_data[l*IMAGE_DISTRIBUTION_WIDTH*3+k*3+1] = fg.g;
      rgb_data[l*IMAGE_DISTRIBUTION_WIDTH*3+k*3+2] = fg.b;
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
				  const amide_data_t volume_max,
				  const gboolean horizontal) {

  intpoint_t i,j;
  rgba_t rgba;
  GdkPixbuf * temp_image;
  amide_data_t datum;
  guchar * rgb_data;

  if ((rgb_data = (guchar *) g_malloc(sizeof(guchar) * 3 * width * height)) == NULL) {
    g_warning("%s: couldn't allocate memory for rgb_data for color_strip",PACKAGE);
    return NULL;
  }

  if (horizontal) {
      for (i=0; i < width; i++) {
      datum = ((((double) width-i)/width) * (volume_max-volume_min))+volume_min;
      datum = (volume_max-volume_min)*(datum-min)/(max-min)+volume_min;
      
      rgba = color_table_lookup(datum, color_table, volume_min, volume_max);
      for (j=0; j < height; j++) {
	rgb_data[j*width*3+i*3+0] = rgba.r;
	rgb_data[j*width*3+i*3+1] = rgba.g;
	rgb_data[j*width*3+i*3+2] = rgba.b;
      }
    }
  } else {
    for (j=0; j < height; j++) {
      datum = ((((double) height-j)/height) * (volume_max-volume_min))+volume_min;
      datum = (volume_max-volume_min)*(datum-min)/(max-min)+volume_min;
      
      rgba = color_table_lookup(datum, color_table, volume_min, volume_max);
      for (i=0; i < width; i++) {
	rgb_data[j*width*3+i*3+0] = rgba.r;
	rgb_data[j*width*3+i*3+1] = rgba.g;
	rgb_data[j*width*3+i*3+2] = rgba.b;
      }
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
			       const floatpoint_t voxel_dim,
			       const realspace_t view_coord_frame,
			       const scaling_t scaling,
			       const interpolation_t interpolation) {

  gint slice_num;
  guint32 total_alpha;
  guchar * rgb_data;
  rgba16_t * rgba16_data;
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
    if ((slices = volumes_get_slices(volumes, start, duration, thickness, voxel_dim, 
				     view_coord_frame, interpolation, TRUE)) == NULL) {
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
  if ((rgba16_data = (rgba16_t *) g_malloc(sizeof(rgba16_t) * dim.y * dim.x)) == NULL) {
    g_warning("%s: couldn't allocate memory for rgba16_data for image",PACKAGE);
    return NULL;
  }
  /* and initialize it */
  i.z = 0;
  for (i.y = 0; i.y < dim.y; i.y++) 
    for (i.x = 0; i.x < dim.x; i.x++) {
      location = i.y*dim.x+i.x;
      rgba16_data[location].r = 0;
      rgba16_data[location].g = 0;
      rgba16_data[location].b = 0;
      rgba16_data[location].a = 0;
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

    /* now add this slice into the rgba16 data */
    i.t = i.z = 0;
    for (i.y = 0; i.y < dim.y; i.y++) 
      for (i.x = 0; i.x < dim.x; i.x++) {
	rgba_temp = color_table_lookup(VOLUME_FLOAT_0D_SCALING_CONTENTS(temp_slices->volume,i), 
				       temp_volumes->volume->color_table, min, max);
	/* compensate for the fact that X defines the origin as top left, not bottom left */
	location = (dim.y - i.y - 1)*dim.x+i.x;
	total_alpha = rgba16_data[location].a + rgba_temp.a;
	if (total_alpha == 0) {
	  rgba16_data[location].r = (((slice_num-1)*rgba16_data[location].r + 
				      rgba_temp.r)/
				     ((gdouble) slice_num));
	  rgba16_data[location].g = (((slice_num-1)*rgba16_data[location].g + 
				      rgba_temp.g)/
				     ((gdouble) slice_num));
	  rgba16_data[location].b = (((slice_num-1)*rgba16_data[location].b + 
				      rgba_temp.b)/
				     ((gdouble) slice_num));
	} else if (rgba16_data[location].a == 0) {
	  rgba16_data[location].r = rgba_temp.r;
	  rgba16_data[location].g = rgba_temp.g;
	  rgba16_data[location].b = rgba_temp.b;
	  rgba16_data[location].a = rgba_temp.a;
	} else if (rgba_temp.a != 0) {
	  rgba16_data[location].r = ((rgba16_data[location].r*rgba16_data[location].a 
				      + rgba_temp.r*rgba_temp.a)/
				     ((double) total_alpha));
	  rgba16_data[location].g = ((rgba16_data[location].g*rgba16_data[location].a 
				      + rgba_temp.g*rgba_temp.a)/
				     ((double) total_alpha));
	  rgba16_data[location].b = ((rgba16_data[location].b*rgba16_data[location].a 
				      + rgba_temp.b*rgba_temp.a)/
				     ((double) total_alpha));
	  rgba16_data[location].a = total_alpha;
	}
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
      rgb_data[3*location+0] = rgba16_data[location].r < 0xFF ? rgba16_data[location].r : 0xFF;
      rgb_data[3*location+1] = rgba16_data[location].g < 0xFF ? rgba16_data[location].g : 0xFF;
      rgb_data[3*location+2] = rgba16_data[location].b < 0xFF ? rgba16_data[location].b : 0xFF;
    }

  

  /* from the rgb_data, generate a GdkPixbuf */
  temp_image = gdk_pixbuf_new_from_data(rgb_data, GDK_COLORSPACE_RGB,
					FALSE,8,dim.x,dim.y,dim.x*3*sizeof(guchar),
					image_free_rgb_data, NULL);

  /* cleanup */
  g_free(rgba16_data);

  return temp_image;
}








/* get the icon to use for this modality */
GdkPixmap * image_get_volume_pixmap(volume_t * volume, 
				    GdkWindow * window, 
				    GdkBitmap ** gdkbitmap) {

  GdkPixmap * gdkpixmap;
  GdkBitmap * bmp;

  g_return_val_if_fail(window != NULL, NULL);

  /* which icon to use */
  switch (volume->modality) {
  case SPECT:
    gdkpixmap = gdk_pixmap_create_from_xpm_d(window, gdkbitmap,NULL,SPECT_xpm);
    break;
  case MRI:
    gdkpixmap = gdk_pixmap_create_from_xpm_d(window, gdkbitmap,NULL,MRI_xpm);
    break;
  case CT:
    gdkpixmap = gdk_pixmap_create_from_xpm_d(window, gdkbitmap,NULL,CT_xpm);
    break;
  case PET:
    gdkpixmap = gdk_pixmap_create_from_xpm_d(window, gdkbitmap,NULL,PET_xpm);
    break;
  case OTHER:
  default:
    gdkpixmap = gdk_pixmap_create_from_xpm_d(window, gdkbitmap,NULL,OTHER_xpm);
    break;
  }

  if (gdkbitmap != NULL) gdkbitmap = &bmp;

  return gdkpixmap;
}


/* get the icon to use for this modality */
GdkPixmap * image_get_roi_pixmap(roi_t * roi, 
				 GdkWindow * window, 
				 GdkBitmap ** gdkbitmap) {
  GdkPixmap * gdkpixmap;
  GdkBitmap * bmp;

  g_return_val_if_fail(window != NULL, NULL);

  /* which icon to use */
  switch (roi->type) {
  case CYLINDER:
    gdkpixmap = gdk_pixmap_create_from_xpm_d(window, gdkbitmap,NULL,CYLINDER_xpm);
    break;
  case ELLIPSOID:
    gdkpixmap = gdk_pixmap_create_from_xpm_d(window, gdkbitmap,NULL,ELLIPSOID_xpm);
    break;
  case ISOCONTOUR_2D:
    gdkpixmap = gdk_pixmap_create_from_xpm_d(window, gdkbitmap,NULL,ISOCONTOUR_2D_xpm);
    break;
  case ISOCONTOUR_3D:
    gdkpixmap = gdk_pixmap_create_from_xpm_d(window, gdkbitmap,NULL,ISOCONTOUR_3D_xpm);
    break;
  case BOX:
  default:
    gdkpixmap = gdk_pixmap_create_from_xpm_d(window, gdkbitmap,NULL,BOX_xpm);
    break;
  }

  if (gdkbitmap != NULL) gdkbitmap = &bmp;

  return gdkpixmap;

}
