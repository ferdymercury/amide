/* mpeg_encode.c - interface to the mpeg encoding library
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

#if (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)

#ifndef __MPEG_ENCODE_H__
#define __MPEG_ENCODE_H__

/* header files that are always associated with this header file */
#include <gdk-pixbuf/gdk-pixbuf.h>

#define FRAMES_PER_SECOND 30

typedef enum {
  ENCODE_MPEG1,
  ENCODE_MPEG4
} mpeg_encode_t;
/* functions */


gpointer mpeg_encode_setup(gchar * output_filename, mpeg_encode_t type, gint xsize, gint ysize);
gboolean mpeg_encode_frame(gpointer mpeg_encode_context, GdkPixbuf * pixbuf);
gpointer mpeg_encode_close(gpointer mpeg_encode_context);

#endif /* __MPEG_ENCODE_H__ */
#endif /* AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT */

