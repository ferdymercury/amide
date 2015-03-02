/* ui_rendering_movie_dialog.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2002 Andy Loening
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
#ifdef AMIDE_MPEG_ENCODE_SUPPORT

/* header files that are always needed with this file */
#include "ui_rendering.h"

#define MOVIE_DEFAULT_FRAMES 300

typedef enum {OVER_TIME, OVER_FRAMES, DYNAMIC_TYPES} dynamic_t;

/* data structures */
typedef struct ui_rendering_movie_t {
  GtkWidget * dialog;
  GtkWidget * progress;
  guint num_frames;
  gdouble rotation[NUM_AXIS];
  dynamic_t type;
  amide_time_t start_time;
  amide_time_t end_time;
  guint start_frame;
  guint end_frame;
  ui_rendering_t * ui_rendering; /* a pointer back to the main rendering ui */
  guint reference_count;

  GtkWidget * start_time_label;
  GtkWidget * end_time_label;
  GtkWidget * start_frame_label;
  GtkWidget * end_frame_label;
  GtkWidget * start_time_entry;
  GtkWidget * end_time_entry;
  GtkWidget * start_frame_entry;
  GtkWidget * end_frame_entry;
} ui_rendering_movie_t;


/* external functions */
ui_rendering_movie_t * ui_rendering_movie_free(ui_rendering_movie_t * ui_rendering_movie);
ui_rendering_movie_t * ui_rendering_movie_init(void);
ui_rendering_movie_t * ui_rendering_movie_add_reference(ui_rendering_movie_t * movie);
void ui_rendering_movie_dialog_perform(ui_rendering_movie_t * ui_rendering_movie, char * output_file_name);
ui_rendering_movie_t * ui_rendering_movie_dialog_create(ui_rendering_t * ui_rendering);

#endif /* AMIDE_MPEG_ENCODE_SUPPORT */
#endif /* LIBVOLPACK_SUPPORT */






