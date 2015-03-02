/* image.h
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

/* header files that are always needed with this file */
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gnome-canvas-pixbuf.h>
#include "study.h"
#include "rendering.h"

/* external functions */
void image_free_rgb_data(guchar * pixels, gpointer data);
GdkPixbuf * image_blank(const intpoint_t width, const intpoint_t height, color_point_t image_color);
#ifdef AMIDE_LIBVOLPACK_SUPPORT
GdkPixbuf * image_from_8bit(const guchar * image, const intpoint_t width, const intpoint_t height,
			    const color_table_t color_table);
GdkPixbuf * image_from_renderings(rendering_list_t * contexts, 
				  gint16 width, gint16 height);
#endif
GdkPixbuf * image_of_distribution(volume_t * volume,
				  const intpoint_t width, 
				  const intpoint_t height);
GdkPixbuf * image_from_colortable(const color_table_t color_table,
				  const intpoint_t width, 
				  const intpoint_t height,
				  const volume_data_t min,
				  const volume_data_t max,
				  const volume_data_t volume_min,
				  const volume_data_t volume_max);
GdkPixbuf * image_from_volumes(volume_list_t ** pslices,
			       volume_list_t * volumes,
			       const volume_time_t start,
			       const volume_time_t duration,
			       const floatpoint_t thickness,
			       const realspace_t view_coord_frame,
			       const scaling_t scaling,
			       const floatpoint_t zoom,
			       const interpolation_t interpolation);

