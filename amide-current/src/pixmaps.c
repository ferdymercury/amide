/* pixmaps.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2006 Andy Loening
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

#include "amitk_data_set.h"
#include "amitk_study.h"
#include "fads.h"

#include "../pixmaps/amide_logo.h"
#include "../pixmaps/amide_logo_small.h"

#include "../pixmaps/icon_transfer_function.xpm"

#include "../pixmaps/icon_interpolation_nearest_neighbor.xpm"
#include "../pixmaps/icon_interpolation_trilinear.xpm"
const char ** interpolation_icon[AMITK_INTERPOLATION_NUM] = {
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
#include "../pixmaps/icon_view_linked3.xpm"
const char ** icon_view_mode[AMITK_VIEW_MODE_NUM] = {
  icon_view_single_xpm,
  icon_view_linked_xpm,
  icon_view_linked3_xpm
};

#include "../pixmaps/icon_target.xpm"

#include "../pixmaps/icon_view_transverse.xpm"
#include "../pixmaps/icon_view_coronal.xpm"
#include "../pixmaps/icon_view_sagittal.xpm"

const char ** icon_view[AMITK_VIEW_NUM] = {
  icon_view_transverse_xpm,
  icon_view_coronal_xpm,
  icon_view_sagittal_xpm,
};


#include "../pixmaps/icon_threshold.xpm"
#include "../pixmaps/thresholding_per_slice_icon.h"
#include "../pixmaps/thresholding_per_frame_icon.h"
#include "../pixmaps/thresholding_interpolate_frames_icon.h"
#include "../pixmaps/thresholding_global_icon.h"
const guint8 * thresholding_icons[AMITK_THRESHOLDING_NUM] = {
  thresholding_per_slice_icon,
  thresholding_per_frame_icon,
  thresholding_interpolate_frames_icon,
  thresholding_global_icon
};

#include "../pixmaps/min_max_icon.h"
#include "../pixmaps/center_width_icon.h"
const guint8 * threshold_style_icons[AMITK_THRESHOLD_STYLE_NUM] = {
  min_max_icon,
  center_width_icon
};

/* #include "../pixmaps/window_bone_icon.h" */
/* #include "../pixmaps/window_soft_tissue_icon.h" */
#include "../pixmaps/window_abdomen.h"
#include "../pixmaps/window_brain.h"
#include "../pixmaps/window_extremities.h"
#include "../pixmaps/window_liver.h"
#include "../pixmaps/window_lung.h"
#include "../pixmaps/window_pelvis_soft_tissue.h"
#include "../pixmaps/window_skull_base.h"
#include "../pixmaps/window_spine_a.h"
#include "../pixmaps/window_spine_b.h"
#include "../pixmaps/window_thorax_soft_tissue.h"
const guint8 * windowing_icons[AMITK_WINDOW_NUM] = {
  window_abdomen,
  window_brain,
  window_extremities,
  window_liver,
  window_lung,
  window_pelvis_soft_tissue,
  window_skull_base,
  window_spine_a,
  window_spine_b,
  window_thorax_soft_tissue
};
  //  window_bone_icon,
  //  window_soft_tissue_icon


#include "../pixmaps/box_icon.h"
#include "../pixmaps/cylinder_icon.h"
#include "../pixmaps/ellipsoid_icon.h"
#include "../pixmaps/isocontour_2d_icon.h"
#include "../pixmaps/isocontour_3d_icon.h"
#include "../pixmaps/freehand_2d_icon.h"
#include "../pixmaps/freehand_3d_icon.h"
const guint8 * roi_icons[AMITK_ROI_TYPE_NUM] = {
  ellipsoid_icon,
  cylinder_icon,
  box_icon,
  isocontour_2d_icon,
  isocontour_3d_icon,
  freehand_2d_icon,
  freehand_3d_icon
};
#include "../pixmaps/align_pt_icon.h"
#include "../pixmaps/study_icon.h"

#include "../pixmaps/linear_layout.xpm"
#include "../pixmaps/orthogonal_layout.xpm"

#include "../pixmaps/panels_mixed.h"
#include "../pixmaps/panels_linear_x.h"
#include "../pixmaps/panels_linear_y.h"
