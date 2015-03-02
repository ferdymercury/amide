/* ui_rendering.h
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

#ifdef AMIDE_LIBVOLPACK_SUPPORT

#ifndef __UI_RENDERING_H__
#define __UI_RENDERING_H__

/* header files that are always needed with this file */
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "rendering.h"

/* defines */
#define UI_RENDERING_BLANK_WIDTH 200
#define UI_RENDERING_BLANK_HEIGHT 200
#define BOX_OFFSET 0.2

/* ui_rendering data structures */
typedef struct ui_rendering_t {
  GnomeApp * app; 
  GtkWidget * parameter_dialog;
#ifdef AMIDE_MPEG_ENCODE_SUPPORT
  gpointer movie; /* pointer to type ui_rendering_movie_t */
#endif
  amide_time_t start;
  amide_time_t duration;
  GnomeCanvas * canvas;
  GnomeCanvasItem * canvas_image;
  GdkPixbuf * rgb_image;
  rendering_list_t * contexts;
  GtkWidget * render_button;
  gboolean immediate;
  rendering_quality_t quality;
  pixel_type_t pixel_type;
  gboolean depth_cueing;
  gdouble front_factor;
  gdouble density;
  gdouble zoom;
  interpolation_t interpolation;
  guint reference_count;
} ui_rendering_t;

/* external functions */
ui_rendering_t * ui_rendering_free(ui_rendering_t * ui_rendering);
ui_rendering_t * ui_rendering_init(volume_list_t * volumes, realspace_t coord_frame, 
				   amide_time_t start, amide_time_t duration, interpolation_t interpolation);
void ui_rendering_update_canvases(ui_rendering_t * ui_rendering);
void ui_rendering_create(volume_list_t * volumes, realspace_t coord_frame, 
			 amide_time_t start, amide_time_t duration, interpolation_t interpolation);

#endif /* __UI_RENDERING_H__ */
#endif /* AMIDE_LIBVOLPACK_SUPPORT */



