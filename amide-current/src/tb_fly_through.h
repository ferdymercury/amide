/* tb_fly_through.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2002-2014 Andy Loening
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

#ifndef __TB_FLY_THROUGH_H__
#define __TB_FLY_THROUGH_H__

/* header files that are always needed with this file */
#include "amitk_study.h"

/* external functions */
void tb_fly_through(AmitkStudy * study,
		    AmitkView view, 
		    AmitkPreferences * preferences,
		    GtkWindow * parent);



#endif /* TB_FLY_THROUGH_H */
#endif /* AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT */


