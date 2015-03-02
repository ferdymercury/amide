/* amitk_color_table.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2014 Andy Loening
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
#include <limits.h>
#include <math.h>
#include "amitk_common.h"
#include "amitk_color_table.h"
#include "amitk_type_builtins.h"

/* external variables */

/* don't change these, as legacy.c needs them for reading in files */
gchar * color_table_menu_names[] = {
  N_("black/white linear"), 
  N_("white/black linear"), 
  N_("black/white/black"),
  N_("white/black/white"),
  N_("red temperature"), 
  N_("inverse red temp."), 
  N_("blue temperature"), 
  N_("inv. blue temp."), 
  N_("green temperature"), 
  N_("inv. green temp."), 
  N_("hot metal"), 
  N_("inv. hot metal"), 
  N_("hot metal contour"),
  N_("inv. hot metal c."),
  N_("hot blue"), 
  N_("inverse hot blue"), 
  N_("hot green"), 
  N_("inverse hot green"), 
  N_("spectrum"), 
  N_("inverse spectrum"), 
  N_("NIH + white"), 
  N_("inv. NIH + white"), 
  N_("NIH"),
  N_("inverse NIH")
};

/* internal functions */
static rgb_t hsv_to_rgb(hsv_t * hsv);




/* this algorithm is derived from "Computer Graphics: principles and practice" */
/* hue = [0 360], s and v are in [0,1] */
static rgb_t hsv_to_rgb(hsv_t * hsv) {
  
  rgb_t rgb;
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
      break;
    default:
      /* should never get here */
      rgb.r = rgb.g = rgb.b = 0;
      break;
    }
  }    

  return rgb;
}

rgba_t amitk_color_table_lookup(amide_data_t datum, AmitkColorTable which,
				amide_data_t min, amide_data_t max) {

  rgba_t temp_rgba;
  rgba_t rgba;
  rgb_t rgb;
  hsv_t hsv;
  amide_data_t scale;
  amide_data_t temp;
  gboolean not_a_number=FALSE;

  /* if not a number, computer as min value, and then set to transparent */
  if (isnan(datum)) {
    datum = min;
    not_a_number = TRUE;
  }

  switch(which) {
  case AMITK_COLOR_TABLE_BWB_LINEAR:
    temp = 2*(datum-min) * 0xFF/(max-min);
    if (temp > 255)
      temp = 511-temp;
    if (temp < 0)
      temp = 0;
    rgba.r = temp;
    rgba.g = temp;
    rgba.b = temp;
    rgba.a = temp;
    break;
  case AMITK_COLOR_TABLE_WBW_LINEAR:
    rgba = amitk_color_table_lookup(datum, AMITK_COLOR_TABLE_BWB_LINEAR, min, max);
    rgba.r = 0xFF-rgba.r;
    rgba.g = 0xFF-rgba.g;
    rgba.b = 0xFF-rgba.b;
    break;
  case AMITK_COLOR_TABLE_RED_TEMP:
    /* this may not be exactly right.... */
    scale = 0xFF/(max-min);
    temp = (datum-min)/(max-min);
    if (temp > 1.0)
      rgba.r = rgba.g = rgba.b = rgba.a = 0xFF;
    else if (temp < 0.0)
      rgba.r = rgba.g = rgba.b = rgba.a = 0;
    else {
      rgba.r = temp >= 0.70 ? 0xFF : scale*(datum-min)/0.70;
      rgba.g = temp >= 0.50 ? 2*scale*(datum-min/2.0-max/2.0) : 0;
      rgba.b = temp >= 0.50 ? 2*scale*(datum-min/2.0-max/2.0) : 0;
      rgba.a = 0xFF*temp;
    }
    break;
  case AMITK_COLOR_TABLE_INV_RED_TEMP:
    rgba = amitk_color_table_lookup((max-datum)+min, AMITK_COLOR_TABLE_RED_TEMP, min, max);
    rgba.a = 0xFF-rgba.a; 
    break;
  case AMITK_COLOR_TABLE_BLUE_TEMP:
    temp_rgba = amitk_color_table_lookup(datum, AMITK_COLOR_TABLE_RED_TEMP, min, max);
    rgba.r = temp_rgba.b;
    rgba.g = temp_rgba.g;
    rgba.b = temp_rgba.r;
    rgba.a = temp_rgba.a;
    break;
  case AMITK_COLOR_TABLE_INV_BLUE_TEMP:
    rgba = amitk_color_table_lookup((max-datum)+min, AMITK_COLOR_TABLE_BLUE_TEMP, min, max);
    rgba.a = 0xFF-rgba.a; 
    break;
  case AMITK_COLOR_TABLE_GREEN_TEMP:
    temp_rgba = amitk_color_table_lookup(datum, AMITK_COLOR_TABLE_RED_TEMP, min, max);
    rgba.r = temp_rgba.g;
    rgba.g = temp_rgba.r;
    rgba.b = temp_rgba.b;
    rgba.a = temp_rgba.a;
    break;
  case AMITK_COLOR_TABLE_INV_GREEN_TEMP:
    rgba = amitk_color_table_lookup((max-datum)+min, AMITK_COLOR_TABLE_GREEN_TEMP, min, max);
    rgba.a = 0xFF-rgba.a; 
    break;
  case AMITK_COLOR_TABLE_HOT_METAL:
    /* derived from code in xmedcon (by Erik Nolf) */

    temp = ((datum-min)/(max-min)); /* between 0.0 and 1.0 */
    if (temp > 1.0)
      rgba.r = rgba.g = rgba.b = rgba.a = 0xFF;
    else if (temp < 0.0)
      rgba.r = rgba.g = rgba.b = rgba.a = 0;
    else {
      /* several "magic" numbers are used, i.e. I have no idea how they're derived, 
	 but they work.....
	 red: distributed between 0-181 (out of 255)
	 green: distributed between 128-218 (out of 255)
	 blue: distributed between 192-255 (out of 255)
      */
      rgba.r = (temp >= (182.0/255.0)) ? 0xFF : 0xFF*temp*(255.0/182); 
      rgba.g = 
	(temp <  (128.0/255.0)) ? 0x00 : 
	((temp >= (219.0/255.0)) ? 0xFF : 0xFF*(temp-128.0/255.0)*(255.0/91.0));
      rgba.b = (temp >= (192.0/255.0)) ? 0xFF*(temp-192.0/255)*(255.0/63.0) : 0x00 ;
      rgba.a = 0xFF*temp;
    }
    break;
  case AMITK_COLOR_TABLE_INV_HOT_METAL:
    rgba = amitk_color_table_lookup((max-datum)+min, AMITK_COLOR_TABLE_HOT_METAL, min, max);
    rgba.a = 0xFF-rgba.a; 
    break;
  case AMITK_COLOR_TABLE_HOT_METAL_CONTOUR:
    temp = 2*(datum-min) * 0xFF/(max-min);
    if (temp > 255)
      temp = 511-temp;
    if (temp < 0)
      temp = 0;
    rgba = amitk_color_table_lookup(temp, AMITK_COLOR_TABLE_HOT_METAL, 0, 255);
    break;
  case AMITK_COLOR_TABLE_INV_HOT_METAL_CONTOUR:
    temp = 2*(datum-min) * 0xFF/(max-min);
    if (temp > 255)
      temp = 511-temp;
    if (temp < 0)
      temp = 0;
    rgba = amitk_color_table_lookup(temp, AMITK_COLOR_TABLE_INV_HOT_METAL, 0, 255);
    break;
  case AMITK_COLOR_TABLE_HOT_BLUE:
    temp_rgba = amitk_color_table_lookup(datum, AMITK_COLOR_TABLE_HOT_METAL, min, max);
    rgba.r = temp_rgba.b;
    rgba.g = temp_rgba.g;
    rgba.b = temp_rgba.r;
    rgba.a = temp_rgba.a;
    break;
  case AMITK_COLOR_TABLE_INV_HOT_BLUE:
    rgba = amitk_color_table_lookup((max-datum)+min, AMITK_COLOR_TABLE_HOT_BLUE, min, max);
    rgba.a = 0xFF-rgba.a; 
    break;
  case AMITK_COLOR_TABLE_HOT_GREEN:
    temp_rgba = amitk_color_table_lookup(datum, AMITK_COLOR_TABLE_HOT_METAL, min, max);
    rgba.r = temp_rgba.g;
    rgba.g = temp_rgba.r;
    rgba.b = temp_rgba.b;
    rgba.a = temp_rgba.a;
    break;
  case AMITK_COLOR_TABLE_INV_HOT_GREEN:
    rgba = amitk_color_table_lookup((max-datum)+min, AMITK_COLOR_TABLE_HOT_GREEN, min, max);
    rgba.a = 0xFF-rgba.a; 
    break;
  case AMITK_COLOR_TABLE_SPECTRUM:
    temp = ((datum-min)/(max-min));
    hsv.s = 1.0;
    hsv.v = 1.0;

    if (temp > 1.0) {
      hsv.h = 360.0;
      rgba.a = 0xFF;
    } else if (temp < 0.0) {
      hsv.h = 0.0;
      rgba.a = 0;
    } else {
      hsv.h = 360.0*temp; 
      rgba.a = 0xFF*temp;
    }

    rgb = hsv_to_rgb(&hsv);
    rgba.r = rgb.r;
    rgba.g = rgb.g;
    rgba.b = rgb.b;
    break;
  case AMITK_COLOR_TABLE_INV_SPECTRUM:
    rgba = amitk_color_table_lookup((max-datum)+min, AMITK_COLOR_TABLE_SPECTRUM, min, max);
    rgba.a = 0xFF-rgba.a; 
    break;
  case AMITK_COLOR_TABLE_NIH:
    /* this algorithm is a complete guess, don't trust it */
    temp = ((datum-min)/(max-min));
    hsv.s = 1.0;

    if (temp < 0.0)
      hsv.v = 0.0;
    else if (temp > 0.2)
      hsv.v = 1.0;
    else
      hsv.v = 5*temp;

    if (temp > 1.0) {
      hsv.h = 0.0;
      rgba.a = 0xFF;
    } else if (temp < 0.0) {
      hsv.h = 300.0;
      rgba.a = 0;
    } else {
      hsv.h = 300.0*(1.0-temp); 
      rgba.a = 0xFF*temp;
    }

    rgb = hsv_to_rgb(&hsv);
    rgba.r = rgb.r;
    rgba.g = rgb.g;
    rgba.b = rgb.b;
    break;
  case AMITK_COLOR_TABLE_INV_NIH:
    rgba = amitk_color_table_lookup((max-datum)+min, AMITK_COLOR_TABLE_NIH, min, max);
    rgba.a = 0xFF-rgba.a; 
    break;
  case AMITK_COLOR_TABLE_NIH_WHITE:
    /* this algorithm is a complete guess, don't trust it */
    temp = ((datum-min)/(max-min));
    hsv.s = 1.0;

    if (temp < 0.0) 
      hsv.v = 0.0;
    else if (temp > 0.2)
      hsv.v = 1.0;
    else
      hsv.v = 5*temp;

    if (temp > 1.0) { 
      hsv.s = hsv.h = 0.0;
      rgba.a = 0xFF;
    } else if (temp < 0.0) {
      hsv.h = 300.0;
      rgba.a = 0;
    } else {
      hsv.h = 300.0*(1.0-temp); 
      rgba.a = 0xFF * temp;
    }

    rgb = hsv_to_rgb(&hsv);
    rgba.r = rgb.r;
    rgba.g = rgb.g;
    rgba.b = rgb.b;
    break;
  case AMITK_COLOR_TABLE_INV_NIH_WHITE:
    rgba = amitk_color_table_lookup((max-datum)+min, AMITK_COLOR_TABLE_NIH_WHITE, min, max);
    rgba.a = 0xFF-rgba.a; 
    break;
  case AMITK_COLOR_TABLE_WB_LINEAR:
    rgba = amitk_color_table_lookup((max-datum)+min, AMITK_COLOR_TABLE_BW_LINEAR, min, max);
    rgba.a = 0xFF-rgba.a; 
    break;
  case AMITK_COLOR_TABLE_BW_LINEAR:
  default:
    temp = (datum-min) * 0xFF/(max-min);
    if (temp > 255)
      temp = 255;
    else if (temp < 0)
      temp = 0;
    rgba.r = temp;
    rgba.g = temp;
    rgba.b = temp;
    rgba.a = temp;
    break;
  }

  if (not_a_number) rgba.a = 0;

  return rgba;
}

rgba_t amitk_color_table_uint32_to_rgba(guint32 color_uint32) {
  rgba_t rgba;

  rgba.r = (color_uint32 >> 24) & 0x000000FF;
  rgba.g = (color_uint32 >> 16) & 0x000000FF;
  rgba.b = (color_uint32 >> 8) & 0x000000FF;
  rgba.a = (color_uint32 >> 0) & 0x000000FF;

  return rgba;
}


rgba_t amitk_color_table_outline_color(AmitkColorTable which, gboolean highlight) {

  rgba_t rgba, normal_rgba, highlight_rgba;

  switch(which) {
  case AMITK_COLOR_TABLE_RED_TEMP:
  case AMITK_COLOR_TABLE_INV_RED_TEMP:
    normal_rgba = amitk_color_table_lookup(1.0, which, 0.0,1.0);
    normal_rgba.r = 0x00;
    normal_rgba.g = 0xFF;
    normal_rgba.b = 0x80;
    highlight_rgba.r = 0x00;
    highlight_rgba.g = 0xFF;
    highlight_rgba.b = 0xFF;
    highlight_rgba.a = normal_rgba.a;
    break;
  case AMITK_COLOR_TABLE_BLUE_TEMP:
  case AMITK_COLOR_TABLE_INV_BLUE_TEMP:
    normal_rgba = amitk_color_table_lookup(1.0, which, 0.0,1.0);
    normal_rgba.r = 0xFF;
    normal_rgba.g = 0x80;
    normal_rgba.b = 0x00;
    highlight_rgba.r = 0xFF;
    highlight_rgba.g = 0xFF;
    highlight_rgba.b = 0x00;
    highlight_rgba.a = normal_rgba.a;
    break;
  case AMITK_COLOR_TABLE_GREEN_TEMP:
  case AMITK_COLOR_TABLE_INV_GREEN_TEMP:
    normal_rgba = amitk_color_table_lookup(1.0, which, 0.0,1.0);
    normal_rgba.r = 0x00;
    normal_rgba.g = 0x80;
    normal_rgba.b = 0xFF;
    highlight_rgba.r = 0xFF;
    highlight_rgba.g = 0x00;
    highlight_rgba.b = 0xFF;
    highlight_rgba.a = normal_rgba.a;
    break;
  case AMITK_COLOR_TABLE_HOT_METAL:
  case AMITK_COLOR_TABLE_INV_HOT_METAL:
    normal_rgba = amitk_color_table_lookup(1.0, which, 0.0,1.0);
    normal_rgba.r = 0x00;
    normal_rgba.g = 0xFF;
    normal_rgba.b = 0x80;
    highlight_rgba.r = 0x00;
    highlight_rgba.g = 0xFF;
    highlight_rgba.b = 0xFF;
    highlight_rgba.a = normal_rgba.a;
    break;
  case AMITK_COLOR_TABLE_HOT_METAL_CONTOUR:
  case AMITK_COLOR_TABLE_INV_HOT_METAL_CONTOUR:
    normal_rgba = amitk_color_table_lookup(0.5, which, 0.0,1.0);
    normal_rgba.r = 0x00;
    normal_rgba.g = 0xFF;
    normal_rgba.b = 0x80;
    highlight_rgba.r = 0x00;
    highlight_rgba.g = 0xFF;
    highlight_rgba.b = 0xFF;
    highlight_rgba.a = normal_rgba.a;
    break;
  case AMITK_COLOR_TABLE_HOT_BLUE:
  case AMITK_COLOR_TABLE_INV_HOT_BLUE:
    normal_rgba = amitk_color_table_lookup(1.0, which, 0.0,1.0);
    normal_rgba.r = 0xFF;
    normal_rgba.g = 0x80;
    normal_rgba.b = 0x00;
    highlight_rgba.r = 0xFF;
    highlight_rgba.g = 0xFF;
    highlight_rgba.b = 0x00;
    highlight_rgba.a = normal_rgba.a;
    break;
  case AMITK_COLOR_TABLE_HOT_GREEN:
  case AMITK_COLOR_TABLE_INV_HOT_GREEN:
    normal_rgba = amitk_color_table_lookup(1.0, which, 0.0,1.0);
    normal_rgba.r = 0x80;
    normal_rgba.g = 0x00;
    normal_rgba.b = 0xFF;
    highlight_rgba.r = 0xFF;
    highlight_rgba.g = 0x00;
    highlight_rgba.b = 0xFF;
    highlight_rgba.a = normal_rgba.a;
    break;
  case AMITK_COLOR_TABLE_SPECTRUM:
  case AMITK_COLOR_TABLE_INV_SPECTRUM:
    normal_rgba.r = normal_rgba.g = normal_rgba.b = normal_rgba.a = 0xFF;
    highlight_rgba.r = highlight_rgba.g = highlight_rgba.b = 0x00;
    highlight_rgba.a = normal_rgba.a;
    break;
  case AMITK_COLOR_TABLE_NIH:
  case AMITK_COLOR_TABLE_INV_NIH:
    normal_rgba.r = normal_rgba.g = normal_rgba.b = normal_rgba.a = 0xFF;
    highlight_rgba.r = 0xFF;
    highlight_rgba.g = 0x80;
    highlight_rgba.b = 0x00;
    highlight_rgba.a = normal_rgba.a;
    break;
  case AMITK_COLOR_TABLE_NIH_WHITE:
  case AMITK_COLOR_TABLE_INV_NIH_WHITE:
    normal_rgba = amitk_color_table_lookup(1.0, which, 0.0,1.0);
    highlight_rgba.r = 0xFF;
    highlight_rgba.g = 0x80;
    highlight_rgba.b = 0x00;
    highlight_rgba.a = normal_rgba.a;
    break;
  case AMITK_COLOR_TABLE_WBW_LINEAR:
    normal_rgba = amitk_color_table_lookup(0.5, which, 0.0,1.0);
    normal_rgba.r = 0xFF;
    normal_rgba.g = 0x00;
    normal_rgba.b = 0x00;
    highlight_rgba.r = 0xF0;
    highlight_rgba.g = 0xF0;
    highlight_rgba.b = 0x00;
    highlight_rgba.a = normal_rgba.a;
    break;
  case AMITK_COLOR_TABLE_WB_LINEAR:
    normal_rgba = amitk_color_table_lookup(1.0, which, 0.0,1.0);
    normal_rgba.r = 0xFF;
    normal_rgba.g = 0x00;
    normal_rgba.b = 0x00;
    highlight_rgba.r = 0xF0;
    highlight_rgba.g = 0xF0;
    highlight_rgba.b = 0x00;
    highlight_rgba.a = normal_rgba.a;
    break;
  case AMITK_COLOR_TABLE_BWB_LINEAR:
    normal_rgba = amitk_color_table_lookup(0.5, which, 0.0,1.0);
    normal_rgba.r = 0xFF;
    normal_rgba.g = 0x00;
    normal_rgba.b = 0x00;
    highlight_rgba.r = 0xFF;
    highlight_rgba.g = 0xFF;
    highlight_rgba.b = 0x00;
    highlight_rgba.a = normal_rgba.a;
    break;
  case AMITK_COLOR_TABLE_BW_LINEAR:
  default:
    normal_rgba = amitk_color_table_lookup(1.0, which, 0.0,1.0);
    normal_rgba.r = 0xFF;
    normal_rgba.g = 0x00;
    normal_rgba.b = 0x00;
    highlight_rgba.r = 0xFF;
    highlight_rgba.g = 0xFF;
    highlight_rgba.b = 0x00;
    highlight_rgba.a = normal_rgba.a;
    break;
  }

  if (highlight)
    rgba = highlight_rgba;
  else
    rgba = normal_rgba;

  return rgba;
}

const gchar * amitk_color_table_get_name(const AmitkColorTable which) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_COLOR_TABLE);
  enum_value = g_enum_get_value(enum_class, which);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}
