/* color_table.c
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
#include <glib.h>
#include <limits.h>
#include <math.h>
#include "amide.h"
#include "color_table.h"
#include "volume.h"
#include "color_table2.h"

/* external variables */
gchar * color_table_names[] = {"black/white linear", \
			       "white/black linear", \
			       "red temperature", \
			       "inverse red temp.", \
			       "blue temperature", \
			       "inv blue temp.", \
			       "green temperature", \
			       "inverse green temp.", \
			       "hot metal", \
			       "inverse hot metal", \
			       "hot blue", \
			       "inverse hot blue", \
			       "hot green", \
			       "inverse hot green", \
			       "spectrum", \
			       "inverse spectrum", \
			       "NIH + white", \
			       "inverse NIH + white", \
			       "NIH", \
			       "inverse NIH"};

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
    temp = (datum-min)/(max-min);
    if (temp > 1.0)
      rgb.r = rgb.g = rgb.b = 0xFF;
    else if (temp < 0.0)
      rgb.r = rgb.g = rgb.b = 0;
    else {
      rgb.r = temp >= 0.70 ? 0xFF : scale*(datum-min)/0.70;
      rgb.g = temp >= 0.50 ? 2*scale*(datum-min/2.0-max/2.0) : 0;
      rgb.b = temp >= 0.50 ? 2*scale*(datum-min/2.0-max/2.0) : 0;
    }
    break;
  case INV_RED_TEMPERATURE:
    rgb = color_table_lookup((max-datum)+min, RED_TEMPERATURE, min, max);
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
      rgb.r = temp >= 0.50 ? 2*scale*(datum-max/2.0-min/2.0) : 0;
      rgb.g = temp >= 0.50 ? 2*scale*(datum-max/2.0-min/2.0) : 0;
      rgb.b = temp >= 0.70 ? 0xFF : scale*(datum-min)/0.70;
    }
    break;
  case INV_BLUE_TEMPERATURE:
    rgb = color_table_lookup((max-datum)+min, BLUE_TEMPERATURE, min, max);
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
      rgb.r = temp >= 0.50 ? 2*scale*(datum-max/2.0-min/2.0) : 0;
      rgb.g = temp >= 0.70 ? 0xFF : scale*(datum-min)/0.70;
      rgb.b = temp >= 0.50 ? 2*scale*(datum-max/2.0-min/2.0) : 0;
    }
    break;
  case INV_GREEN_TEMPERATURE:
    rgb = color_table_lookup((max-datum)+min, GREEN_TEMPERATURE, min, max);
    break;
  case HOT_METAL:
    /* derived from code in xmedcon (by Erik Nolf) */

    temp = ((datum-min)/(max-min)); /* between 0.0 and 1.0 */
    if (temp > 1.0)
      rgb.r = rgb.g = rgb.b = 0xFF;
    else if (temp < 0.0)
      rgb.r = rgb.g = rgb.b = 0;
    else {
      /* several "magic" numbers are used, i.e. I have no idea how they're derived, 
	 but they work.....
	 red: distributed between 0-181 (out of 255)
	 green: distributed between 128-218 (out of 255)
	 blue: distributed between 192-255 (out of 255)
      */
      rgb.r = (temp >= (182.0/255.0)) ? 0xFF : 0xFF*temp*(255.0/182); 
      rgb.g = 
	(temp <  (128.0/255.0)) ? 0x00 : 
	((temp >= (219.0/255.0)) ? 0xFF : 0xFF*(temp-128.0/255.0)*(255.0/91.0));
      rgb.b = (temp >= (192.0/255.0)) ? 0xFF*(temp-192.0/255)*(255.0/63.0) : 0x00 ;
      
    }
    break;
  case INV_HOT_METAL:
    rgb = color_table_lookup((max-datum)+min, HOT_METAL, min, max);
    break;
  case HOT_BLUE:
    /* derived from code in xmedcon (by Erik Nolf) */

    temp = ((datum-min)/(max-min)); /* between 0.0 and 1.0 */
    if (temp > 1.0)
      rgb.r = rgb.g = rgb.b = 0xFF;
    else if (temp < 0.0)
      rgb.r = rgb.g = rgb.b = 0;
    else {
      /* several "magic" numbers are used, i.e. I have no idea how they're derived, 
	 but they work.....
	 green: distributed between 0-181 (out of 255)
	 red: distributed between 128-218 (out of 255)
	 blue: distributed between 192-255 (out of 255)
      */
      rgb.b = (temp >= (182.0/255.0)) ? 0xFF : 0xFF*temp*(255.0/182); 
      rgb.g = 
	(temp <  (128.0/255.0)) ? 0x00 : 
	((temp >= (219.0/255.0)) ? 0xFF : 0xFF*(temp-128.0/255.0)*(255.0/91.0));
      rgb.r = (temp >= (192.0/255.0)) ? 0xFF*(temp-192.0/255)*(255.0/63.0) : 0x00 ;
      
    }
    break;
  case INV_HOT_BLUE:
    rgb = color_table_lookup((max-datum)+min, HOT_BLUE, min, max);
    break;
  case HOT_GREEN:
    /* derived from code in xmedcon (by Erik Nolf) */

    temp = ((datum-min)/(max-min)); /* between 0.0 and 1.0 */
    if (temp > 1.0)
      rgb.r = rgb.g = rgb.b = 0xFF;
    else if (temp < 0.0)
      rgb.r = rgb.g = rgb.b = 0;
    else {
      /* several "magic" numbers are used, i.e. I have no idea how they're derived, 
	 but they work.....
	 green: distributed between 0-181 (out of 255)
	 red: distributed between 128-218 (out of 255)
	 blue: distributed between 192-255 (out of 255)
      */
      rgb.g = (temp >= (182.0/255.0)) ? 0xFF : 0xFF*temp*(255.0/182); 
      rgb.r = 
	(temp <  (128.0/255.0)) ? 0x00 : 
	((temp >= (219.0/255.0)) ? 0xFF : 0xFF*(temp-128.0/255.0)*(255.0/91.0));
      rgb.b = (temp >= (192.0/255.0)) ? 0xFF*(temp-192.0/255)*(255.0/63.0) : 0x00 ;
      
    }
    break;
  case INV_HOT_GREEN:
    rgb = color_table_lookup((max-datum)+min, HOT_GREEN, min, max);
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
  case INV_SPECTRUM:
    rgb = color_table_lookup((max-datum)+min, SPECTRUM, min, max);
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
  case INV_NIH:
    rgb = color_table_lookup((max-datum)+min, NIH, min, max);
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
  case INV_NIH_WHITE:
    rgb = color_table_lookup((max-datum)+min, NIH_WHITE, min, max);
    break;
    //  case WB_LINEAR:
  case WB_LINEAR:
    rgb = color_table_lookup((max-datum)+min, BW_LINEAR, min, max);
    break;

    //    temp = 0xFF-((datum-min) * 0xFF/(max-min));
    //    if (temp > 255)
    //      temp = 255;
    //    else if (temp < 0)
    //      temp = 0;
    //    rgb.r = temp;
    //    rgb.g = temp;
    //    rgb.b = temp;
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
  case INV_RED_TEMPERATURE:
    normal_color = color_table_lookup(1.0, which, 0.0,1.0);
    highlight_color.r = 0x00;
    highlight_color.g = 0xFF;
    highlight_color.b = 0xFF;
    break;
  case BLUE_TEMPERATURE:
  case INV_BLUE_TEMPERATURE:
    normal_color = color_table_lookup(1.0, which, 0.0,1.0);
    highlight_color.r = 0xFF;
    highlight_color.g = 0xFF;
    highlight_color.b = 0x00;
    break;
  case GREEN_TEMPERATURE:
  case INV_GREEN_TEMPERATURE:
    normal_color = color_table_lookup(1.0, which, 0.0,1.0);
    highlight_color.r = 0xFF;
    highlight_color.g = 0x00;
    highlight_color.b = 0xFF;
    break;
  case HOT_METAL:
  case INV_HOT_METAL:
    normal_color = color_table_lookup(1.0, which, 0.0,1.0);
    highlight_color.r = 0x00;
    highlight_color.g = 0xFF;
    highlight_color.b = 0xFF;
    break;
  case HOT_BLUE:
  case INV_HOT_BLUE:
    normal_color = color_table_lookup(1.0, which, 0.0,1.0);
    highlight_color.r = 0xFF;
    highlight_color.g = 0x00;
    highlight_color.b = 0xFF;
    break;
  case HOT_GREEN:
  case INV_HOT_GREEN:
    normal_color = color_table_lookup(1.0, which, 0.0,1.0);
    highlight_color.r = 0xFF;
    highlight_color.g = 0xFF;
    highlight_color.b = 0x00;
    break;
  case SPECTRUM:
  case INV_SPECTRUM:
    normal_color.r = normal_color.g = normal_color.b = 0xFF;
    highlight_color.r = highlight_color.g = highlight_color.b = 0x00;
    break;
  case NIH:
  case INV_NIH:
    normal_color.r = normal_color.g = normal_color.b = 0xFF;
    highlight_color.r = 0xFF;
    highlight_color.g = 0x80;
    highlight_color.b = 0x00;
    break;
  case NIH_WHITE:
  case INV_NIH_WHITE:
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
