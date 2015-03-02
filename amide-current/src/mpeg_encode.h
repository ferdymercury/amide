/* mpeg_encode.c - interface to the mpeg_encode application
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

#ifdef AMIDE_MPEG_ENCODE_SUPPORT

#ifndef __MPEG_ENCODE_H__
#define __MPEG_ENCODE_H__

/* header files that are always associated with this header file */
#include <amide.h>

#define FRAMES_PER_SECOND 30

/* functions */
void mpeg_encode(const gchar * temp_dir, GList * file_list, 
		 gchar * output_filename, GTimeVal current_time,
		 gboolean clean_only);

#endif /* __MPEG_ENCODE_H__ */
#endif /* AMIDE_MPEG_ENCODE_SUPPORT */

