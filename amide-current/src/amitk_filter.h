/* amitk_filter.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2002 Andy Loening
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

#ifndef __AMITK_FILTER_H__
#define __AMITK_FILTER_H__

#include <glib-object.h>
#define _GNU_SOURCE /* use GNU extensions, i.e. NaN */
#include <math.h>
#include "amide.h"
#include "amitk_raw_data.h"

G_BEGIN_DECLS

typedef enum {
  AMITK_FILTER_GAUSSIAN,
  AMITK_FILTER_NUM
} AmitkFilter;
  //  AMITK_FILTER_MEDIAN,

AmitkRawData * amitk_filter_calculate_gaussian_kernel(const gint kernel_size,
						      const AmitkPoint voxel_size,
						      const amide_real_t fwhm,
						      const AmitkAxis axis);

const gchar * amitk_filter_get_name(const AmitkFilter filter);


G_END_DECLS

#endif /* __AMITK_FILTER_H__ */
