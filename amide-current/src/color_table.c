/* color_table.c
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
#include <limits.h>
#include <math.h>
#include "amide.h"
#include "realspace.h"
#include "volume.h"
#include "color_table.h"

/* external variables */
gchar * color_table_names[] = {"black/white linear", \
			       "white/black linear", \
			       "red temperature", \
			       "blue temperature", \
			       "green temperature", \
			       "spectrum", \
			       "NIH + white", \
			       "NIH"};

/* this algorithm is derived from "Computer Graphics: principles and practice" */
/* hue = [0 360], s and v are in [0,1] */
color_point_t color_table_hsv_to_rgb(hsv_point_t * hsv) {
  
  color_point_t rgb;
  hsv_data_t fraction, p,q,t;
  gint whole;

  if (hsv->s == 0.0) /* saturation is zero (on black white line)n */
    rgb.r = rgb.g = rgb.b = UCHAR_MAX * hsv->v; /* achromatic case */
  else {
    if (hsv->h == 360.0)
      hsv->h = 0.0; /* from [0 360] to [0 360) */
    
    whole = floor(hsv->h/60.0);
    fraction = (hsv->h/60.0) - whole;

    p = hsv->v*(1.0-hsv->s);
    q = hsv->v*(1.0-(hsv->s*fraction));
    t = hsv->v*(1.0-(hsv->s*(1.0-fraction)));

    switch(whole) {
    case 0:
      rgb.r = UCHAR_MAX * hsv->v;
      rgb.g = UCHAR_MAX * t;
      rgb.b = UCHAR_MAX * p;
      break;
    case 1:
      rgb.r = UCHAR_MAX * q;
      rgb.g = UCHAR_MAX * hsv->v;
      rgb.b = UCHAR_MAX * p;
      break;
    case 2:
      rgb.r = UCHAR_MAX * p;
      rgb.g = UCHAR_MAX * hsv->v;
      rgb.b = UCHAR_MAX * t;
      break;
    case 3:
      rgb.r = UCHAR_MAX * p;
      rgb.g = UCHAR_MAX * q;
      rgb.b = UCHAR_MAX * hsv->v;
      break;
    case 4:
      rgb.r = UCHAR_MAX * t;
      rgb.g = UCHAR_MAX * p;
      rgb.b = UCHAR_MAX * hsv->v;
      break;
    case 5:
      rgb.r = UCHAR_MAX * hsv->v;
      rgb.g = UCHAR_MAX * p;
      rgb.b = UCHAR_MAX * q;
    }
  }    

  return rgb;
}

color_point_t color_table_lookup(volume_data_t datum, color_table_t which,
				 volume_data_t min, volume_data_t max) {

  color_point_t rgb;
  hsv_point_t hsv;
  floatpoint_t scale;
  volume_data_t temp;

  switch(which) {
  case RED_TEMPERATURE:
    /* this may not be exactly right.... */
    scale = 0xFF/(max-min);
    temp = ((datum-min)/(max-min));
    if (temp > 1.0)
      rgb.r = rgb.g = rgb.b = 0xFF;
    else if (temp < 0.0)
      rgb.r = rgb.g = rgb.b = 0;
    else {
      rgb.r = temp >= 0.70 ? 0xFF : scale*(datum-min)/0.70;
      rgb.g = temp >= 0.50 ? 2*scale*(datum-max/2.0-min) : 0;
      rgb.b = temp >= 0.50 ? 2*scale*(datum-max/2.0-min) : 0;
    }
    break;
  case BLUE_TEMPERATURE:
    /* this may not be exactly right.... */
    scale = 0xFF/(max-min);
    temp = ((datum-min)/(max-min));
    if (temp > 1.0)
      rgb.r = rgb.g = rgb.b = 0xFF;
    else if (temp < 0.0)
      rgb.r = rgb.g = rgb.b = 0;
    else {
      rgb.r = temp >= 0.50 ? 2*scale*(datum-max/2.0-min) : 0;
      rgb.g = temp >= 0.50 ? 2*scale*(datum-max/2.0-min) : 0;
      rgb.b = temp >= 0.70 ? 0xFF : scale*(datum-min)/0.70;
    }
    break;
  case GREEN_TEMPERATURE:
    /* this may not be exactly right.... */
    scale = 0xFF/(max-min);
    temp = ((datum-min)/(max-min));
    if (temp > 1.0)
      rgb.r = rgb.g = rgb.b = 0xFF;
    else if (temp < 0.0)
      rgb.r = rgb.g = rgb.b = 0;
    else {
      rgb.r = temp >= 0.50 ? 2*scale*(datum-max/2.0-min) : 0;
      rgb.g = temp >= 0.70 ? 0xFF : scale*(datum-min)/0.70;
      rgb.b = temp >= 0.50 ? 2*scale*(datum-max/2.0-min) : 0;
    }
    break;
  case SPECTRUM:
    temp = ((datum-min)/(max-min));
    hsv.s = 1.0;
    hsv.v = 1.0;

    if (temp > 1.0)
      hsv.h = 360.0;
    else if (temp < 0.0)
      hsv.h = 0.0;
    else
      hsv.h = 360.0*temp; 

    rgb = color_table_hsv_to_rgb(&hsv);
    break;
  case NIH:
    /* this algorithm is a complete guess, don't trust it */
    temp = ((datum-min)/(max-min));
    hsv.s = 1.0;

    if (temp < 0.0)
      hsv.v = 0.0;
    else if (temp > 0.2)
      hsv.v = 1.0;
    else
      hsv.v = 5*temp;

    if (temp > 0.99)
      hsv.h = 0.0;
    else if (temp < 0.0)
      hsv.h = 300.0;
    else
      hsv.h = (1.0/0.99) * 300.0*(1.0-temp); 

    rgb = color_table_hsv_to_rgb(&hsv);
    break;
  case NIH_WHITE:
    /* this algorithm is a complete guess, don't trust it */
    temp = ((datum-min)/(max-min));
    hsv.s = 1.0;

    if (temp < 0.0)
      hsv.v = 0.0;
    else if (temp > 0.2)
      hsv.v = 1.0;
    else
      hsv.v = 5*temp;

    if (temp > 0.99)
      hsv.s = hsv.h = 0.0;
    else if (temp < 0.0)
      hsv.h = 300.0;
    else
      hsv.h = (1.0/0.99) * 300.0*(1.0-temp); 

    rgb = color_table_hsv_to_rgb(&hsv);
    break;
  case WB_LINEAR:
    temp = 0xFF-((datum-min) * 0xFF/(max-min));
    if (temp > 255)
      temp = 255;
    else if (temp < 0)
      temp = 0;
    rgb.r = temp;
    rgb.g = temp;
    rgb.b = temp;
    break;
  case BW_LINEAR:
  default:
    temp = (datum-min) * 0xFF/(max-min);
    if (temp > 255)
      temp = 255;
    else if (temp < 0)
      temp = 0;
    rgb.r = temp;
    rgb.g = temp;
    rgb.b = temp;
    break;
  }

  return rgb;
}


guint32 color_table_outline_color(color_table_t which, gboolean highlight) {

  color_point_t color, normal_color, highlight_color;
  guint32 outline_color;

  switch(which) {
  case RED_TEMPERATURE:
    normal_color = color_table_lookup(1.0, which, 0.0,1.0);
    highlight_color.r = 0;
    highlight_color.g = 0xFF;
    highlight_color.b = 0xFF;
    break;
  case BLUE_TEMPERATURE:
    normal_color = color_table_lookup(1.0, which, 0.0,1.0);
    highlight_color.r = 0xFF;
    highlight_color.g = 0xFF;
    highlight_color.b = 0;
    break;
  case GREEN_TEMPERATURE:
    normal_color = color_table_lookup(1.0, which, 0.0,1.0);
    highlight_color.r = 0xFF;
    highlight_color.g = 0;
    highlight_color.b = 0xFF;
    break;
  case SPECTRUM:
    normal_color.r = normal_color.g = normal_color.b = 0xFF;
    highlight_color.r = highlight_color.g = highlight_color.b = 0x00;
    break;
  case NIH:
    normal_color.r = normal_color.g = normal_color.b = 0xFF;
    highlight_color.r = 0xFF;
    highlight_color.g = 0x80;
    highlight_color.b = 0x00;
    break;
  case NIH_WHITE:
    normal_color = color_table_lookup(1.0, which, 0.0,1.0);
    highlight_color.r = 0xFF;
    highlight_color.g = 0x80;
    highlight_color.b = 0x00;
    break;
  case WB_LINEAR:
    normal_color = color_table_lookup(1.0, which, 0.0,1.0);
    highlight_color.r = 0xFF;
    highlight_color.g = 0x00;
    highlight_color.b = 0x00;
    break;
  case BW_LINEAR:
  default:
    normal_color = color_table_lookup(1.0, which, 0.0,1.0);
    highlight_color.r = 0xFF;
    highlight_color.g = 0xFF;
    highlight_color.b = 0x00;
    break;
  }

  if (highlight)
    color = highlight_color;
  else
    color = normal_color;

  outline_color = color.r <<24 | color.g <<16 | color.b << 8 | 0xFF <<0;

  return outline_color;
}
