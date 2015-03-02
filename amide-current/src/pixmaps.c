/* pixmaps.c
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

#include "amitk_data_set.h"
#include "amitk_study.h"

#include "../pixmaps/amide_logo.xpm"

#include "../pixmaps/icon_interpolation_nearest_neighbor.xpm"
#include "../pixmaps/icon_interpolation_trilinear.xpm"
const char ** icon_interpolation[AMITK_INTERPOLATION_NUM] = {
  icon_interpolation_nearest_neighbor_xpm,
  icon_interpolation_trilinear_xpm
};

#include "../pixmaps/icon_fuse_type_blend.xpm"
#include "../pixmaps/icon_fuse_type_overlay.xpm"
const char ** icon_fuse_type[AMITK_FUSE_TYPE_NUM] = {
  icon_fuse_type_blend_xpm,
  icon_fuse_type_overlay_xpm
};

#include "../pixmaps/icon_view_single.xpm"
#include "../pixmaps/icon_view_linked.xpm"
const char ** icon_view_mode[AMITK_VIEW_MODE_NUM] = {
  icon_view_single_xpm,
  icon_view_linked_xpm
};

#include "../pixmaps/icon_threshold.xpm"
#include "../pixmaps/icon_thresholding_per_slice.xpm"
#include "../pixmaps/icon_thresholding_per_frame.xpm"
#include "../pixmaps/icon_thresholding_interpolate_frames.xpm"
#include "../pixmaps/icon_thresholding_global.xpm"
const char ** icon_thresholding[AMITK_THRESHOLDING_NUM] = {
  icon_thresholding_per_slice_xpm,
  icon_thresholding_per_frame_xpm,
  icon_thresholding_interpolate_frames_xpm,
  icon_thresholding_global_xpm
};

#include "../pixmaps/CYLINDER.xpm"
#include "../pixmaps/BOX.xpm"
#include "../pixmaps/ELLIPSOID.xpm"
#include "../pixmaps/ISOCONTOUR_2D.xpm"
#include "../pixmaps/ISOCONTOUR_3D.xpm"

#include "../pixmaps/study.xpm"
#include "../pixmaps/ALIGN_PT.xpm"

#include "../pixmaps/linear_layout.xpm"
#include "../pixmaps/orthogonal_layout.xpm"
