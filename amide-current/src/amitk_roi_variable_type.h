/* amitk_roi_variable_type.h - used to generate the different amitk_roi_*.h files
 *
 * Part of amide - Amide's a Medical Image Data Examiner
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


#ifndef __AMITK_ROI_`'m4_Internal_Data_Format`'_H__
#define __AMITK_ROI_`'m4_Variable_Type`'_H__

/* header files that are always needed with this file */
#include "amitk_roi.h"
#include "amitk_data_set.h"


/* function declarations */
#define ROI_TYPE_`'m4_Variable_Type`'

#if defined(ROI_TYPE_ELLIPSOID) || defined(ROI_TYPE_CYLINDER) || defined(ROI_TYPE_BOX)
GSList * amitk_roi_`'m4_Variable_Type`'_get_intersection_line(const AmitkRoi * roi, 
							      const AmitkVolume * canvas_slice,
							      const amide_real_t pixel_dim);
#endif

#if defined(ROI_TYPE_ISOCONTOUR_2D) || defined(ROI_TYPE_ISOCONTOUR_3D) || defined(ROI_TYPE_FREEHAND_2D) || defined(ROI_TYPE_FREEHAND_3D)
AmitkDataSet * amitk_roi_`'m4_Variable_Type`'_get_intersection_slice(const AmitkRoi * roi,
								     const AmitkVolume * canvas_slice,
								     const amide_real_t pixel_dim
#ifndef AMIDE_LIBGNOMECANVAS_AA
								     ,const gboolean fill_roi
#endif
								     );
void amitk_roi_`'m4_Variable_Type`'_set_isocontour(AmitkRoi * roi, 
						   AmitkDataSet * ds, 
						   AmitkVoxel iso_vp,
						   amide_data_t iso_min_value,
						   amide_data_t iso_max_value,
						   AmitkRoiIsocontourRange iso_range);
void amitk_roi_`'m4_Variable_Type`'_manipulate_area(AmitkRoi * roi, gboolean erase, AmitkVoxel voxel, gint area_size);
void amitk_roi_`'m4_Variable_Type`'_calc_center_of_mass(AmitkRoi * roi);
#endif

void amitk_roi_`'m4_Variable_Type`'_calculate_on_data_set_fast(const AmitkRoi * roi,  
							       const AmitkDataSet * ds, 
							       const guint frame,
							       const guint gate,
							       const gboolean inverse,
							       void (*calculation)(),
							       gpointer data);
void amitk_roi_`'m4_Variable_Type`'_calculate_on_data_set_accurate(const AmitkRoi * roi,  
								   const AmitkDataSet * ds, 
								   const guint frame,
								   const guint gate,
								   const gboolean inverse,
								   void (*calculation)(),
								   gpointer data);


#undef ROI_TYPE_`'m4_Variable_Type`'

#endif /* __AMITK_ROI_`'m4_Variable_Type`'_H__ */





