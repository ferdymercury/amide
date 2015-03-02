/* mpeg_encode.c - interface to the mpeg encoding library
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2003 Andy Loening
 *
 * Author: Andy Loening <loening@alum.mit.edu>
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

#ifdef AMIDE_LIBFAME_SUPPORT
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "fame.h"
#include "mpeg_encode.h"

#define BUFFER_MULT 8

typedef struct {
  fame_context_t * fame_context;
  fame_parameters_t * fame_parameters;
  gint xsize;
  gint ysize;
  guchar *buffer;
  gint buffer_size; /*xsize*ysize*BUFFER_MULT */
  fame_yuv_t * yuv;
  FILE * output_file;
} context_t;

/* free a mpeg_encode context structure */
static context_t * context_free(context_t * context) {

  gint length;

  if (context == NULL)
    return context;

  if (context->fame_context != NULL) {
    length = fame_close(context->fame_context); /* flush anything left */
    if ((context->buffer != NULL) && (context->output_file != NULL)) /* and finish writing the file */
      fwrite(context->buffer, sizeof(guchar), length, context->output_file);
    context->fame_context = NULL;
  }

  if (context->output_file != NULL) {
    fclose(context->output_file);
    context->output_file = NULL;
  }

  if (context->fame_parameters != NULL) {
    g_free(context->fame_parameters);
    context->fame_parameters = NULL;
  }

  if (context->buffer != NULL) {
    g_free(context->buffer);
    context->buffer = NULL;
  }

  if (context->yuv != NULL) {
    g_free(context->yuv->y);
    g_free(context->yuv);
    context->yuv = NULL;
  }

  g_free(context);

  return NULL;
}






/* setup the mpeg encoding process */
gpointer * mpeg_encode_setup(gchar * output_filename, mpeg_encode_t type, gint xsize, gint ysize) {

  fame_parameters_t default_fame_parameters =  FAME_PARAMETERS_INITIALIZER;
  context_t * context;
  fame_object_t *object;
  int i;

  /* we need x and y to be divisible by 2 for conversion to YUV12 space */
  /* and we need x and y to be divisible by 16 for fame */
  xsize = 16*ceil(xsize/16.0);
  ysize = 16*ceil(ysize/16.0);

  /* alloc space for the mpeg_encoding structure */
  if ((context = g_try_new(context_t,1)) == NULL) {
    g_warning("couldn't allocate space for context_t");
    return NULL;
  }
  context->fame_context = NULL;
  context->fame_parameters = NULL;
  context->buffer = NULL;
  context->output_file = NULL;
  context->xsize = xsize;
  context->ysize = ysize;
  context->buffer_size = xsize*ysize*BUFFER_MULT; /* needs to be able to hold a couple frames */

  if ((context->fame_parameters = g_try_new(fame_parameters_t,1)) == NULL) {
    g_warning("couldn't allocate space for fame parameters");
    context_free(context);
    return NULL;
  }
  memcpy(context->fame_parameters, &default_fame_parameters, sizeof(fame_parameters_t));

  if ((context->buffer = g_try_new(guchar, context->buffer_size)) == NULL) {
    g_warning("Unable to allocate mpeg encoding buffer");
    context_free(context);
    return NULL;
  }


  if ((context->yuv = g_try_new(fame_yuv_t, 1)) == NULL) {
    g_warning("Unable to allocate yuv struct");
    context_free(context);
    return NULL;
  }
  context->yuv->w = xsize;
  context->yuv->h = ysize;
  context->yuv->p = xsize;
  if ((context->yuv->y = g_try_new(unsigned char, xsize*ysize*3/2)) == NULL) {
    g_warning("Unable to allocate yuv buffer");
    context_free(context);
    return NULL;
  }
  context->yuv->u = context->yuv->y + xsize*ysize;
  context->yuv->v = context->yuv->u + xsize*ysize/4;

  /* initialize yuv buffer, as the portion of the buffer that's large then the
     pixbuf's we use is never used*/
  for (i=0; i<xsize*ysize; i++)
    context->yuv->y[i] = 0;
  for (i=0; i<xsize*ysize/4; i++) {
    context->yuv->u[i] = 128;
    context->yuv->v[i] = 128;
  }



  if ((context->output_file = fopen(output_filename, "w")) == NULL) {
    g_warning("unable to open output file for mpeg encoding");
    context_free(context);
    return NULL;
  }

  context->fame_context = fame_open(); /* initalize library */

  /* specify any parameters we want to change from default */
  context->fame_parameters->width = xsize;
  context->fame_parameters->height = ysize;
  context->fame_parameters->frame_rate_num = FRAMES_PER_SECOND;
  context->fame_parameters->frame_rate_den = 1;
  context->fame_parameters->verbose = 0;  /* turn off verbose mode */

  /* specify additional options */
  switch(type) {
  case ENCODE_MPEG4:
    object = fame_get_object(context->fame_context, "profile/mpeg4");
    break;
  case ENCODE_MPEG1:
  default:
    object = fame_get_object(context->fame_context, "profile/mpeg1");
    break;
  }
  fame_register(context->fame_context, "profile", object);

  fame_init(context->fame_context, 
	    context->fame_parameters, 
	    context->buffer, 
	    context->buffer_size);


  return (gpointer) context;
}


#define RGB_TO_Y(pixels, loc) (0.29900 * pixels[loc] + 0.58700 * pixels[loc+1] + 0.11400 * pixels[loc+2])
#define RGB_TO_U(pixels, loc)(-0.16874 * pixels[loc] - 0.33126 * pixels[loc+1] + 0.50000 * pixels[loc+2]+128.0)
#define RGB_TO_V(pixels, loc) (0.50000 * pixels[loc] - 0.41869 * pixels[loc+1] - 0.08131 * pixels[loc+2]+128.0)


static void convert_rgb_pixbuf_to_yuv(fame_yuv_t * yuv, GdkPixbuf * pixbuf) {

  gint x, y, location, location2;
  gint inner_x, inner_y, half_location;
  gfloat cr, cb;
  gint pixbuf_xsize, pixbuf_ysize;
  guchar * pixels;
  gint row_stride;
  gboolean x_odd, y_odd;

  pixbuf_xsize = gdk_pixbuf_get_width(pixbuf);
  pixbuf_ysize = gdk_pixbuf_get_height(pixbuf);
  pixels = gdk_pixbuf_get_pixels(pixbuf);
  row_stride = gdk_pixbuf_get_rowstride(pixbuf);

  y_odd = (pixbuf_ysize & 0x1);
  x_odd = (pixbuf_xsize & 0x1);

  /* note, the Cr and Cb info is subsampled by 2x2 */
  for (y=0; y<pixbuf_ysize-1; y+=2) {
    for (x=0; x<pixbuf_xsize-1; x+=2) {
      cb = 0.0;
      cr = 0.0;
      for (inner_y = y; inner_y < y+2; inner_y++)
	for (inner_x = x; inner_x < x+2; inner_x++) {
	  location = inner_y*row_stride+3*inner_x;
	  yuv->y[inner_x+inner_y*yuv->w] = RGB_TO_Y(pixels, location);
	  cb += RGB_TO_U(pixels, location);
	  cr += RGB_TO_V(pixels, location);
	}
      half_location = x/2 + y*yuv->w/4;
      yuv->u[half_location] = cb/4.0;
      yuv->v[half_location] = cr/4.0;
    }
    if (x_odd) {
      location = y*row_stride+3*x;
      location2 = (y+1)*row_stride+3*x;

      yuv->y[x+y*yuv->w] = RGB_TO_Y(pixels, location);
      yuv->y[x+1+y*yuv->w] = 0;
      yuv->y[x+(y+1)*yuv->w] = RGB_TO_Y(pixels, location2);
      yuv->y[x+1+(y+1)*yuv->w] = 0;

      half_location = x/2 + y*yuv->w/4;
      yuv->u[half_location] = (RGB_TO_U(pixels, location)+RGB_TO_U(pixels, location2)+256)/4.0;
      yuv->v[half_location] = (RGB_TO_V(pixels, location)+RGB_TO_V(pixels, location2)+256)/4.0;
    }
  }
  if (y_odd) {
    for (x=0; x<pixbuf_xsize-1; x+=2) {
      location = y*row_stride+3*x;
      location2 = y*row_stride+3*(x+1);

      yuv->y[x+y*yuv->w] = RGB_TO_Y(pixels, location);
      yuv->y[x+1+y*yuv->w] = RGB_TO_Y(pixels, location2);
      yuv->y[x+(y+1)*yuv->w] = 0;
      yuv->y[x+1+(y+1)*yuv->w] = 0;

      half_location = x/2 + y*yuv->w/4;
      yuv->u[half_location] = (RGB_TO_U(pixels, location)+RGB_TO_U(pixels, location2)+256)/4.0;
      yuv->v[half_location] = (RGB_TO_V(pixels, location)+RGB_TO_V(pixels, location2)+256)/4.0;
    }
    if (x_odd) {
      location = y*row_stride+3*x;

      yuv->y[x+y*yuv->w] = RGB_TO_Y(pixels, location);
      yuv->y[x+1+y*yuv->w] = 0;
      yuv->y[x+(y+1)*yuv->w] = 0;
      yuv->y[x+1+(y+1)*yuv->w] = 0;

      half_location = x/2 + y*yuv->w/4;
      yuv->u[half_location] = (RGB_TO_U(pixels, location)+384)/4.0;
      yuv->v[half_location] = (RGB_TO_V(pixels, location)+384)/4.0;
    }
  }

  return;
}

/* encode a frame of data */
gboolean mpeg_encode_frame(gpointer data, GdkPixbuf * pixbuf) {
  
  context_t * context = data;
  gint length;

  g_return_val_if_fail(gdk_pixbuf_get_colorspace(pixbuf) == GDK_COLORSPACE_RGB, FALSE);


  convert_rgb_pixbuf_to_yuv(context->yuv, pixbuf);

  fame_start_frame(context->fame_context, context->yuv, NULL);

  while((length = fame_encode_slice(context->fame_context)) != 0)
    fwrite(context->buffer, sizeof(guchar), length, context->output_file);

  fame_end_frame(context->fame_context, NULL);
  
  return TRUE;
}



/* close everything up */
gpointer mpeg_encode_close(gpointer data) {
  context_t * context = data;

  context_free(context); /* free context */

  return NULL;
}






#endif /* AMIDE_LIBFAME_SUPPORT */

