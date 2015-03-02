/* ui_render.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2014 Andy Loening
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

#ifdef AMIDE_LIBVOLPACK_SUPPORT

#ifndef __UI_RENDER_H__
#define __UI_RENDER_H__

/* header files that are always needed with this file */
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "render.h"
#include "amitk_study.h"

#define GCONF_AMIDE_RENDERING "RENDERING"

/* defines */
#define UI_RENDER_BLANK_WIDTH 200
#define UI_RENDER_BLANK_HEIGHT 200
#define BOX_OFFSET 0.2

/* ui_render data structures */
typedef struct ui_render_t {
  GtkWindow * window;
  GtkWidget * window_vbox;
  GtkWidget * parameter_dialog;
  GtkWidget * transfer_function_dialog;
  AmitkPreferences * preferences;
#if (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)
  gpointer movie; /* pointer to type ui_render_movie_t */
#endif
  amide_time_t start;
  amide_time_t duration;
  amide_real_t fov;
  AmitkPoint view_center;
  GtkWidget * canvas;
  GnomeCanvasItem * canvas_image;
  GnomeCanvasItem * canvas_time_label;
  gboolean time_label_on;
  gint pixbuf_width, pixbuf_height;
  GdkPixbuf * pixbuf;
  renderings_t * renderings;
  gboolean update_without_release;
  gboolean stereoscopic;
  gdouble stereo_eye_angle;
  gint stereo_eye_width; /* pixels */
  rendering_quality_t quality;
  gboolean depth_cueing;
  gdouble front_factor;
  gdouble density;
  gdouble zoom;
  AmitkSpace * box_space;

  guint next_update;
  guint idle_handler_id;
  gboolean rendered_successfully;

  GtkWidget * progress_dialog;
  gboolean disable_progress_dialog;
  guint reference_count;
} ui_render_t;

/* external functions */
GdkPixbuf * ui_render_get_pixbuf(ui_render_t * ui_render);
void ui_render_add_update(ui_render_t * ui_render);
gboolean ui_render_update_immediate(gpointer ui_render);
void ui_render_create(AmitkStudy * study, GList * selected_objects, AmitkPreferences * preferences);
GtkWidget * ui_render_init_dialog_create(AmitkStudy * study, GtkWindow * parent);

#endif /* __UI_RENDER_H__ */
#endif /* AMIDE_LIBVOLPACK_SUPPORT */



