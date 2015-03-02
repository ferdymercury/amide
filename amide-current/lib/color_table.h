/* color_table.h
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

#ifndef __COLOR_TABLE_H__
#define __COLOR_TABLE_H__

/* header files that are always associated with this header file */
#include "amide.h"

/* typedef's */
typedef enum {BW_LINEAR, \
	      WB_LINEAR, \
	      RED_TEMPERATURE, \
	      INV_RED_TEMPERATURE, \
	      BLUE_TEMPERATURE, \
	      INV_BLUE_TEMPERATURE, \
	      GREEN_TEMPERATURE, \
	      INV_GREEN_TEMPERATURE, \
	      HOT_METAL, \
	      INV_HOT_METAL, \
	      HOT_BLUE, \
	      INV_HOT_BLUE, \
	      HOT_GREEN, \
	      INV_HOT_GREEN, \
	      SPECTRUM, \
	      INV_SPECTRUM, \
	      NIH_WHITE, \
	      INV_NIH_WHITE, \
	      NIH, \
	      INV_NIH, \
	      NUM_COLOR_TABLES} color_table_t;

typedef guint8 color_data_t;
typedef guint16 color_data16_t;
typedef gdouble hsv_data_t;

typedef struct rgba_t {
  color_data_t r;
  color_data_t g;
  color_data_t b;
  color_data_t a;
} rgba_t;

typedef struct rgba16_t {
  color_data16_t r;
  color_data16_t g;
  color_data16_t b;
  color_data16_t a;
} rgba16_t;

typedef struct rgb_t {
  color_data_t r;
  color_data_t g;
  color_data_t b;
} rgb_t;
  
typedef struct hsv_t {
  hsv_data_t h;
  hsv_data_t s;
  hsv_data_t v;
} hsv_t;


/* defines */
#define color_table_rgba_to_uint32(rgba) (((rgba).r<<24) | ((rgba).g<<16) | ((rgba).b<<8) | ((rgba).a<<0))

/* external functions */
rgba_t color_table_outline_color(color_table_t which, gboolean highlight);
rgba_t color_table_lookup(amide_data_t datum, color_table_t which,
			  amide_data_t min, amide_data_t max);
/* external variables */
extern gchar * color_table_names[];
extern rgba_t rgba_black;


/* internal functions */
rgb_t color_table_hsv_to_rgb(hsv_t * hsv);


#endif /* __COLOR_TABLE_H__ */
