/* amitk_color_table.h
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

#ifndef __COLOR_TABLE_H__
#define __COLOR_TABLE_H__

/* header files that are always associated with this header file */
#include "amitk_type.h"

G_BEGIN_DECLS

/* typedef's */
typedef enum {
  AMITK_COLOR_TABLE_BW_LINEAR, 
  AMITK_COLOR_TABLE_WB_LINEAR, 
  AMITK_COLOR_TABLE_BWB_LINEAR,
  AMITK_COLOR_TABLE_WBW_LINEAR,
  AMITK_COLOR_TABLE_RED_TEMP, 
  AMITK_COLOR_TABLE_INV_RED_TEMP, 
  AMITK_COLOR_TABLE_BLUE_TEMP, 
  AMITK_COLOR_TABLE_INV_BLUE_TEMP, 
  AMITK_COLOR_TABLE_GREEN_TEMP, 
  AMITK_COLOR_TABLE_INV_GREEN_TEMP, 
  AMITK_COLOR_TABLE_HOT_METAL, 
  AMITK_COLOR_TABLE_INV_HOT_METAL, 
  AMITK_COLOR_TABLE_HOT_METAL_CONTOUR,
  AMITK_COLOR_TABLE_INV_HOT_METAL_CONTOUR,
  AMITK_COLOR_TABLE_HOT_BLUE, 
  AMITK_COLOR_TABLE_INV_HOT_BLUE, 
  AMITK_COLOR_TABLE_HOT_GREEN, 
  AMITK_COLOR_TABLE_INV_HOT_GREEN, 
  AMITK_COLOR_TABLE_SPECTRUM, 
  AMITK_COLOR_TABLE_INV_SPECTRUM, 
  AMITK_COLOR_TABLE_NIH_WHITE, 
  AMITK_COLOR_TABLE_INV_NIH_WHITE, 
  AMITK_COLOR_TABLE_NIH, 
  AMITK_COLOR_TABLE_INV_NIH, 
  AMITK_COLOR_TABLE_NUM
} AmitkColorTable; 

#define AMITK_OBJECT_DEFAULT_COLOR 0x808000FF


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
#define amitk_color_table_rgba_to_uint32(rgba) (((rgba).r<<24) | ((rgba).g<<16) | ((rgba).b<<8) | ((rgba).a<<0))

/* external functions */
rgba_t amitk_color_table_uint32_to_rgba(guint32 color_uint32);
rgba_t amitk_color_table_outline_color(AmitkColorTable which, gboolean highlight);
rgba_t amitk_color_table_lookup(amide_data_t datum, AmitkColorTable which,
				amide_data_t min, amide_data_t max);
const gchar * amitk_color_table_get_name(const AmitkColorTable which);
/* external variables */
extern gchar * color_table_menu_names[];

G_END_DECLS

#endif /* __COLOR_TABLE_H__ */
