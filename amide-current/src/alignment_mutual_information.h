/* alignment_mutual_information.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2011-2012 Ian Miller
 *
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

#ifndef __ALIGNMENT_MUTUAL_INFORMATION_H__
#define __ALIGNMENT_MUTUAL_INFORMATION_H__

/* header files that are always needed with this file */
#include "amitk_data_set.h"


/* external functions */
/* the space returned is the transform needed to change moving_ds's space to the
   aligned space, incoding an axes rotation, as well as the necessary shift
   with respect to the dataset's center */
AmitkSpace * alignment_mutual_information(AmitkDataSet * moving_ds, 
					  AmitkDataSet * fixed_ds, 
					  AmitkPoint view_center,
					  amide_real_t thickness,
					  amide_time_t view_start_time,
					  amide_time_t view_duration,
					  gdouble * pointer_mutual_information_error,
					  AmitkUpdateFunc update_func,
					  gpointer update_data);


#endif /* __ALIGNMENT_MUTUAL_INFORMATION_H__ */
