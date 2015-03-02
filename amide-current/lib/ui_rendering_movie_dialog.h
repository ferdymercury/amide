/* ui_rendering_movie_dialog.h
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
#ifdef AMIDE_MPEG_ENCODE_SUPPORT

#define MOVIE_DEFAULT_FRAMES 300

/* data structures */
typedef struct ui_rendering_movie_t {
  GtkWidget * dialog;
  guint num_frames;
  gdouble rotation[NUM_AXIS];
  ui_rendering_t * ui_rendering; /* a pointer back to the main rendering ui */
  guint reference_count;
} ui_rendering_movie_t;


/* external functions */
ui_rendering_movie_t * ui_rendering_movie_free(ui_rendering_movie_t * ui_rendering_movie);
ui_rendering_movie_t * ui_rendering_movie_init(void);
void ui_rendering_movie_dialog_perform(ui_rendering_movie_t * ui_rendering_movie, char * output_file_name);
ui_rendering_movie_t * ui_rendering_movie_dialog_create(ui_rendering_t * ui_rendering);

#endif /* AMIDE_MPEG_ENCODE_SUPPORT */
#endif /* LIBVOLPACK_SUPPORT */






