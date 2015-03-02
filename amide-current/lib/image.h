/* image.h
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

#ifndef __IMAGE_H__
#define __IMAGE_H__

/* header files that are always needed with this file */
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "study.h"
#include "rendering.h"

#define IMAGE_DISTRIBUTION_WIDTH 100

/* external functions */
GdkPixbuf * image_slice_intersection(const roi_t * roi,
				     const realspace_t * canvas_coord_frame,
				     const realpoint_t canvas_corner,
				     const realpoint_t canvas_voxel_size,
				     rgba_t color,
				     realspace_t ** return_frame,
				     realpoint_t * return_corner);
GdkPixbuf * image_blank(const intpoint_t width, const intpoint_t height, rgba_t image_color);
#ifdef AMIDE_LIBVOLPACK_SUPPORT
GdkPixbuf * image_from_8bit(const guchar * image, const intpoint_t width, const intpoint_t height,
			    const color_table_t color_table);
GdkPixbuf * image_from_contexts(renderings_t * contexts, 
				gint16 image_width, gint16 image_height,
				eye_t eyes, 
				gdouble eye_angle, 
				gint16 eye_width);
#endif
GdkPixbuf * image_of_distribution(volume_t * volume, rgb_t fg, rgb_t bg);
GdkPixbuf * image_from_colortable(const color_table_t color_table,
				  const intpoint_t width, 
				  const intpoint_t height,
				  const amide_data_t min,
				  const amide_data_t max,
				  const amide_data_t volume_min,
				  const amide_data_t volume_max,
				  const gboolean horizontal);
GdkPixbuf * image_from_volumes(volumes_t ** pslices,
			       volumes_t * volumes,
			       const amide_time_t start,
			       const amide_time_t duration,
			       const floatpoint_t thickness,
			       const floatpoint_t voxel_dim,
			       const realspace_t * view_coord_frame,
			       const interpolation_t interpolation);
GdkPixmap * image_get_volume_pixmap(volume_t * volume, 
				    GdkWindow * window, 
				    GdkBitmap ** gdkbitmap);
GdkPixmap * image_get_roi_pixmap(roi_t * roi, 
				 GdkWindow * window, 
				 GdkBitmap ** gdkbitmap);

#endif /*  __IMAGE_H__ */

