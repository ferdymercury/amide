/* color_table.h
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


/* typedef's */
typedef enum {BW_LINEAR, \
	      WB_LINEAR, \
	      RED_TEMPERATURE, \
	      BLUE_TEMPERATURE, \
	      GREEN_TEMPERATURE, \
	      HOT_METAL, \
	      SPECTRUM, \
	      NIH_WHITE, \
	      NIH, \
	      NUM_COLOR_TABLES} color_table_t;

typedef guint8 color_data_t;
typedef gdouble hsv_data_t;

typedef struct color_point_t {
  color_data_t r;
  color_data_t g;
  color_data_t b;
} color_point_t;
  
typedef struct hsv_point_t {
  hsv_data_t h;
  hsv_data_t s;
  hsv_data_t v;
} hsv_point_t;

/* external functions */
guint32 color_table_outline_color(color_table_t which, gboolean highlight);

/* in color_table2.h 
   color_point_t color_table_lookup */

/* external variables */
extern gchar * color_table_names[];


/* internal functions */
color_point_t color_table_hsv_to_rgb(hsv_point_t * hsv);

