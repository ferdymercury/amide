/* alignment.h
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

#ifdef AMIDE_LIBGSL_SUPPORT

#ifndef __ALIGNMENT_H__
#define __ALIGNMENT_H__

/* header files that are always needed with this file */
#include "volume.h"


/* external functions */
realspace_t alignment_calculate(volume_t * moving_vol, volume_t * fixed_vol, align_pts_t * pts);


#endif /* __ALIGNMENT_H__ */
#endif /* AMIDE_LIBGSL_SUPPORT */
