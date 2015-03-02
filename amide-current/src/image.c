/* image.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2002 Andy Loening
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

#include "amide_config.h"
#include <gnome.h>
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
#include "../pixmaps/study.xpm"
#include "../pixmaps/ALIGN_PT.xpm"
#include "amitk_data_set_FLOAT_0D_SCALING.h"
#include "amitk_study.h"

/* callback function used for freeing the pixel data in a gdkpixbuf */
static void image_free_rgb_data(guchar * pixels, gpointer data) {
  g_free(pixels);
  return;
}



GdkPixbuf * image_slice_intersection(const AmitkRoi * roi,
				     const AmitkVolume * canvas_slice,
				     const amide_real_t pixel_dim,
				     rgba_t color,
				     AmitkVolume ** return_volume) {

  GdkPixbuf * temp_image;
  AmitkDataSet * intersection;
  AmitkVoxel i;
  guchar * rgba_data;
  AmitkVoxel dim;

  
  intersection = amitk_roi_get_intersection_slice(roi, canvas_slice, pixel_dim);
  if (intersection == NULL) return NULL;

  dim = AMITK_DATA_SET_RAW_DATA(intersection)->dim;
  if ((dim.x == 0) && (dim.y ==0)) {
    g_object_unref(intersection);
    return NULL;
  }

  if ((rgba_data = g_new(guchar,4*dim.x*dim.y)) == NULL) {
    g_warning("couldn't allocate memory for rgba_data for roi image");
    g_object_unref(intersection);
    return NULL;
  }

  *return_volume = amitk_volume_new();
  amitk_object_copy_in_place(AMITK_OBJECT(*return_volume), AMITK_OBJECT(intersection));

  i.z = i.t = 0;
  for (i.y=0 ; i.y < dim.y; i.y++)
    for (i.x=0 ; i.x < dim.x; i.x++)
      if (*AMITK_RAW_DATA_UBYTE_POINTER(intersection->raw_data, i) == 1) {
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

  g_object_unref(intersection);

  return temp_image;
}

/* function to return a blank image */
GdkPixbuf * image_blank(const amide_intpoint_t width, const amide_intpoint_t height, rgba_t image_color) {
  
  GdkPixbuf * temp_image;
  amide_intpoint_t i,j;
  guchar * rgb_data;
  
  if ((rgb_data = g_new(guchar, 3*width*height)) == NULL) {
    g_warning("couldn't allocate memory for rgb_data for blank image");
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
GdkPixbuf * image_from_8bit(const guchar * image, const amide_intpoint_t width, const amide_intpoint_t height,
			    const AmitkColorTable color_table) {
  
  GdkPixbuf * temp_image;
  amide_intpoint_t i,j;
  guchar * rgb_data;
  rgba_t rgba_temp;
  guint location;

  if ((rgb_data = g_new(guchar,3*width*height)) == NULL) {
    g_warning("couldn't allocate memory for image from 8 bit data");
    return NULL;
  }


  for (i=0 ; i < height; i++)
    for (j=0; j < width; j++) {
      /* note, line below compensates for X's origin being top left, not bottom left */
      rgba_temp = amitk_color_table_lookup(image[(height-i-1)*width+j], color_table, 
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
GdkPixbuf * image_from_contexts(renderings_t * contexts, 
				gint16 image_width, gint16 image_height,
				AmitkEye eyes, 
				gdouble eye_angle, 
				gint16 eye_width) {

  gint image_num;
  guint32 total_alpha;
  rgba16_t * rgba16_data;
  guchar * char_data;
  AmitkVoxel i;
  rgba_t rgba_temp;
  guint location;
  GdkPixbuf * temp_image;
  gint total_width;
  gdouble rot;
  AmitkEye i_eye;

  total_width = image_width+(eyes-1)*eye_width;

  /* allocate space for a temporary storage buffer */
  if ((rgba16_data = g_new(rgba16_t,total_width * image_height)) == NULL) {
    g_warning("couldn't allocate memory for rgba16_data for transfering rendering to image");
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

      rendering_context_set_rotation(contexts->rendering_context, AMITK_AXIS_Y, -rot);
      rendering_context_render(contexts->rendering_context);
      rendering_context_set_rotation(contexts->rendering_context, AMITK_AXIS_Y, rot);

      i.t = i.z = 0;
      for (i.y = 0; i.y < image_height; i.y++) 
	for (i.x = 0; i.x < image_width; i.x++) {
	  rgba_temp = amitk_color_table_lookup(contexts->rendering_context->image[i.x+i.y*image_width], 
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
				       ((gdouble) total_alpha));
	  rgba16_data[location].g = ((rgba16_data[location].g*rgba16_data[location].a 
				      + rgba_temp.g*rgba_temp.a)/
				     ((gdouble) total_alpha));
	  rgba16_data[location].b = ((rgba16_data[location].b*rgba16_data[location].a 
				      + rgba_temp.b*rgba_temp.a)/
				     ((gdouble) total_alpha));
	  rgba16_data[location].a = total_alpha;
	  }
	}
    }      
    contexts = contexts->next;
  }

  /* allocate space for the true rgb buffer */
  if ((char_data = g_new(guchar,3*image_height * total_width)) == NULL) {
    g_warning("couldn't allocate memory for char_data for rendering image");
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
GdkPixbuf * image_of_distribution(AmitkDataSet * ds, rgb_t fg, rgb_t bg) {

  GdkPixbuf * temp_image;
  guchar * rgb_data;
  amide_intpoint_t k,l;
  AmitkVoxel j;
  amide_data_t max, scale;
  AmitkRawData * distribution;


  /* make sure we have a distribution calculated (won't the first time we're called with a vol)*/
  amitk_data_set_calc_distribution(ds);
  distribution = AMITK_DATA_SET_DISTRIBUTION(ds);

  if ((rgb_data = g_new(guchar,3*IMAGE_DISTRIBUTION_WIDTH*distribution->dim.x)) == NULL) {
    g_warning("couldn't allocate memory for rgb_data for bar_graph");
    return NULL;
  }

  /* figure out the max of the distribution, so we can normalize the distribution to the width */
  max = 0.0;
  j.t = j.z = j.y = 0;
  for (j.x = 0; j.x < distribution->dim.x ; j.x++) 
    if (*AMITK_RAW_DATA_FLOAT_POINTER(distribution,j) > max)
      max = *AMITK_RAW_DATA_FLOAT_POINTER(distribution,j);
  scale = ((gdouble) IMAGE_DISTRIBUTION_WIDTH)/max;

  /* figure out what the rgb data is */
  j.t = j.z = j.y = 0;
  for (l=0 ; l < distribution->dim.x ; l++) {
    j.x = distribution->dim.x-l-1;
    for (k=0; k < floor(((gdouble) IMAGE_DISTRIBUTION_WIDTH)-
			scale*(*AMITK_RAW_DATA_FLOAT_POINTER(distribution,j))) ; k++) {
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
					distribution->dim.x,
					IMAGE_DISTRIBUTION_WIDTH*3*sizeof(guchar),
					image_free_rgb_data, NULL);
  
  if (temp_image == NULL) g_error("NULL Distribution Image created\n");

  return temp_image;
}


/* function to make the color_strip image */
GdkPixbuf * image_from_colortable(const AmitkColorTable color_table,
				  const amide_intpoint_t width, 
				  const amide_intpoint_t height,
				  const amide_data_t min,
				  const amide_data_t max,
				  const amide_data_t data_set_min,
				  const amide_data_t data_set_max,
				  const gboolean horizontal) {

  amide_intpoint_t i,j;
  rgba_t rgba;
  GdkPixbuf * temp_image;
  amide_data_t datum;
  guchar * rgb_data;

  if ((rgb_data = g_new(guchar,3*width*height)) == NULL) {
    g_warning("couldn't allocate memory for rgb_data for color_strip");
    return NULL;
  }

  if (horizontal) {
      for (i=0; i < width; i++) {
      datum = ((((gdouble) width-i)/width) * (data_set_max-data_set_min))+data_set_min;
      datum = (data_set_max-data_set_min)*(datum-min)/(max-min)+data_set_min;
      
      rgba = amitk_color_table_lookup(datum, color_table, data_set_min, data_set_max);
      for (j=0; j < height; j++) {
	rgb_data[j*width*3+i*3+0] = rgba.r;
	rgb_data[j*width*3+i*3+1] = rgba.g;
	rgb_data[j*width*3+i*3+2] = rgba.b;
      }
    }
  } else {
    for (j=0; j < height; j++) {
      datum = ((((gdouble) height-j)/height) * (data_set_max-data_set_min))+data_set_min;
      datum = (data_set_max-data_set_min)*(datum-min)/(max-min)+data_set_min;
      
      rgba = amitk_color_table_lookup(datum, color_table, data_set_min, data_set_max);
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


GdkPixbuf * image_from_data_sets(GList ** pslices,
				 GList * objects,
				 const amide_time_t start,
				 const amide_time_t duration,
				 const amide_real_t pixel_dim,
				 const AmitkVolume * view_volume,
				 const AmitkInterpolation interpolation) {

  gint slice_num;
  guint32 total_alpha;
  guchar * rgb_data;
  rgba16_t * rgba16_data;
  guint location;
  AmitkVoxel i;
  AmitkVoxel dim;
  amide_data_t max,min;
  GdkPixbuf * temp_image;
  rgba_t rgba_temp;
  GList * slices;
  GList * temp_slices;
  AmitkDataSet * slice;
  AmitkColorTable color_table;
  

  /* sanity checks */
  g_return_val_if_fail(objects != NULL, NULL);

  /* generate the slices if we need to */
  if ((*pslices) == NULL) {
    if ((slices = amitk_data_sets_get_slices(objects, start, duration, pixel_dim, 
					     view_volume, interpolation, TRUE)) == NULL) {
      g_return_val_if_fail(slices != NULL, NULL);
    } 
    (*pslices) = slices;
  } else {
    slices = (*pslices);
  }

  /* get the dimensions.  since all slices have the same dimensions, we'll just get the first */
  dim = AMITK_DATA_SET_RAW_DATA(slices->data)->dim;

  /* allocate space for a temporary storage buffer */
  if ((rgba16_data = g_new(rgba16_t,dim.y*dim.x)) == NULL) {
    g_warning("couldn't allocate memory for rgba16_data for image");
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
  slice_num = 0;

  while (temp_slices != NULL) {
    slice_num++;
    slice = temp_slices->data;

    amitk_data_set_get_thresholding_max_min(AMITK_DATA_SET_SLICE_PARENT(slice),
					    AMITK_DATA_SET(slice),
					    start, duration, &max, &min);

    
    color_table = AMITK_DATA_SET_COLOR_TABLE(AMITK_DATA_SET_SLICE_PARENT(slice));
    /* now add this slice into the rgba16 data */
    i.t = i.z = 0;
    for (i.y = 0; i.y < dim.y; i.y++) 
      for (i.x = 0; i.x < dim.x; i.x++) {
	rgba_temp = 
	  amitk_color_table_lookup(AMITK_DATA_SET_FLOAT_0D_SCALING_CONTENTS(slice,i), color_table,min, max);

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
				     ((gdouble) total_alpha));
	  rgba16_data[location].g = ((rgba16_data[location].g*rgba16_data[location].a 
				      + rgba_temp.g*rgba_temp.a)/
				     ((gdouble) total_alpha));
	  rgba16_data[location].b = ((rgba16_data[location].b*rgba16_data[location].a 
				      + rgba_temp.b*rgba_temp.a)/
				     ((gdouble) total_alpha));
	  rgba16_data[location].a = total_alpha;
	}
      }
    temp_slices = temp_slices->next;
  }

  /* allocate space for the true rgb buffer */
  if ((rgb_data = g_new(guchar,3*dim.y*dim.x)) == NULL) {
    g_warning("couldn't allocate memory for rgb_data for image");
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
GdkPixbuf * image_get_object_pixbuf(AmitkObject * object) {

  GdkPixbuf * pixbuf;

  g_return_val_if_fail(AMITK_IS_OBJECT(object), NULL);

  if (AMITK_IS_ROI(object)) {
    switch (AMITK_ROI_TYPE(object)) {
    case AMITK_ROI_TYPE_CYLINDER:
      pixbuf = gdk_pixbuf_new_from_xpm_data(CYLINDER_xpm);
      break;
    case AMITK_ROI_TYPE_ELLIPSOID:
      pixbuf = gdk_pixbuf_new_from_xpm_data(ELLIPSOID_xpm);
      break;
    case AMITK_ROI_TYPE_ISOCONTOUR_2D:
      pixbuf = gdk_pixbuf_new_from_xpm_data(ISOCONTOUR_2D_xpm);
      break;
    case AMITK_ROI_TYPE_ISOCONTOUR_3D:
      pixbuf = gdk_pixbuf_new_from_xpm_data(ISOCONTOUR_3D_xpm);
      break;
    case AMITK_ROI_TYPE_BOX:
    default:
      pixbuf = gdk_pixbuf_new_from_xpm_data(BOX_xpm);
      break;
    }
  } else if (AMITK_IS_DATA_SET(object)) {
    switch (AMITK_DATA_SET_MODALITY(object)) {
    case AMITK_MODALITY_SPECT:
      pixbuf = gdk_pixbuf_new_from_xpm_data(SPECT_xpm);
      break;
    case AMITK_MODALITY_MRI:
      pixbuf = gdk_pixbuf_new_from_xpm_data(MRI_xpm);
      break;
    case AMITK_MODALITY_CT:
      pixbuf = gdk_pixbuf_new_from_xpm_data(CT_xpm);
      break;
    case AMITK_MODALITY_PET:
      pixbuf = gdk_pixbuf_new_from_xpm_data(PET_xpm);
      break;
    case AMITK_MODALITY_OTHER:
    default:
      pixbuf = gdk_pixbuf_new_from_xpm_data(OTHER_xpm);
      break;
    }
  } else if (AMITK_IS_STUDY(object)) {
    pixbuf = gdk_pixbuf_new_from_xpm_data(study_xpm);
  } else if (AMITK_IS_FIDUCIAL_MARK(object)) {
    pixbuf = gdk_pixbuf_new_from_xpm_data(ALIGN_PT_xpm);
  } else {
    pixbuf = gdk_pixbuf_new_from_xpm_data(OTHER_xpm);
    g_print("Unknown case in %s at %d\n", __FILE__, __LINE__);
  }

  return pixbuf;

}
