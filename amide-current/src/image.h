/* image.h
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

#ifndef __IMAGE_H__
#define __IMAGE_H__

/* header files that are always needed with this file */
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "amide.h"
#include "amitk_common.h"
#include "amitk_study.h"
#include "amitk_roi.h"
#include "render.h"

#define IMAGE_DISTRIBUTION_WIDTH 100

/* external functions */
GdkPixbuf * image_slice_intersection(const AmitkRoi * roi,
				     const AmitkVolume * canvas_slice,
				     const amide_real_t pixel_size,
#ifdef AMIDE_LIBGNOMECANVAS_AA
				     const gdouble transparency,
#else
				     const gboolean fill_roi,
#endif
				     rgba_t color,
				     AmitkPoint * return_offset,
				     AmitkPoint * return_corner);
GdkPixbuf * image_blank(const amide_intpoint_t width, 
			const amide_intpoint_t height, 
			rgba_t image_color);
GdkPixbuf * image_from_8bit(const guchar * image, 
			    const amide_intpoint_t width, 
			    const amide_intpoint_t height,
			    const AmitkColorTable color_table);
#ifdef AMIDE_LIBVOLPACK_SUPPORT
GdkPixbuf * image_from_renderings(renderings_t * renderings, 
				  gint16 image_width, gint16 image_height,
				  AmideEye eyes, 
				  gdouble eye_angle, 
				  gint16 eye_width);
#endif
GdkPixbuf * image_of_distribution(AmitkDataSet * ds, rgb_t fg,
				  AmitkUpdateFunc update_func,
				  gpointer update_data);
GdkPixbuf * image_from_colortable(const AmitkColorTable color_table,
				  const amide_intpoint_t width, 
				  const amide_intpoint_t height,
				  const amide_data_t min,
				  const amide_data_t max,
				  const amide_data_t data_set_min,
				  const amide_data_t data_set_max,
				  const gboolean horizontal);
GdkPixbuf * image_from_projection(AmitkDataSet * projection);
GdkPixbuf * image_from_slice(AmitkDataSet * slice,
			     AmitkViewMode view_mode);
GdkPixbuf * image_from_data_sets(GList ** pdisp_slices,
				 GList ** pslice_cache,
				 const gint max_slice_cache_size,
				 GList * objects,
				 const AmitkDataSet * active_ds,
				 const amide_time_t start,
				 const amide_time_t duration,
				 const amide_intpoint_t gate,
				 const amide_real_t pixel_size,
				 const AmitkVolume * view_volume,
				 const AmitkFuseType fuse_type,
				 const AmitkViewMode view_mode);
GdkPixbuf * image_get_data_set_pixbuf(AmitkDataSet * ds);

#endif /*  __IMAGE_H__ */

