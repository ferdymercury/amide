/* pixmaps.c
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

#include "amitk_data_set.h"
#include "amitk_study.h"

#include "../pixmaps/amide_logo.h"

#include "../pixmaps/interpolation_nearest_neighbor.h"
#include "../pixmaps/interpolation_trilinear.h"
#include "../pixmaps/transfer_function.h"


#include "../pixmaps/fuse_type_blend.h"
#include "../pixmaps/fuse_type_overlay.h"

#include "../pixmaps/target.h"

#include "../pixmaps/view_transverse.h"
#include "../pixmaps/view_coronal.h"
#include "../pixmaps/view_sagittal.h"

#include "../pixmaps/view_single.h"
#include "../pixmaps/view_linked.h"
#include "../pixmaps/view_linked3.h"

#include "../pixmaps/thresholding.h"
#include "../pixmaps/thresholding_per_slice.h"
#include "../pixmaps/thresholding_per_frame.h"
#include "../pixmaps/thresholding_interpolate_frames.h"
#include "../pixmaps/thresholding_global.h"

#include "../pixmaps/threshold_style_min_max.h"
#include "../pixmaps/threshold_style_center_width.h"

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


#include "../pixmaps/roi_box.h"
#include "../pixmaps/roi_cylinder.h"
#include "../pixmaps/roi_ellipsoid.h"
#include "../pixmaps/roi_isocontour_2d.h"
#include "../pixmaps/roi_isocontour_3d.h"
#include "../pixmaps/roi_freehand_2d.h"
#include "../pixmaps/roi_freehand_3d.h"
#include "../pixmaps/align_pt.h"
#include "../pixmaps/study.h"

#include "../pixmaps/layout_linear.h"
#include "../pixmaps/layout_orthogonal.h"

#include "../pixmaps/panels_mixed.h"
#include "../pixmaps/panels_linear_x.h"
#include "../pixmaps/panels_linear_y.h"


static gboolean icons_initialized=FALSE;

static struct {
  const guint8 * pixbuf_inline;
  const gchar * icon_id;
} amide_icons[] = {
  { amide_logo, "amide_icon_logo" },
  { align_pt, "amide_icon_align_pt" },
  { fuse_type_blend, "amide_icon_fuse_type_blend" },
  { fuse_type_overlay, "amide_icon_fuse_type_overlay" },
  { interpolation_nearest_neighbor, "amide_icon_interpolation_nearest_neighbor" },
  { interpolation_trilinear, "amide_icon_interpolation_trilinear" },
  { layout_linear, "amide_icon_layout_linear" },
  { layout_orthogonal, "amide_icon_layout_orthogonal" },
  { panels_mixed, "amide_icon_panels_mixed" },
  { panels_linear_x, "amide_icon_panels_linear_x" },
  { panels_linear_y, "amide_icon_panels_linear_y" },
  { roi_box, "amide_icon_roi_box" },
  { roi_cylinder, "amide_icon_roi_cylinder" },
  { roi_ellipsoid, "amide_icon_roi_ellipsoid" },
  { roi_isocontour_2d, "amide_icon_roi_isocontour_2d"},
  { roi_isocontour_3d, "amide_icon_roi_isocontour_3d"},
  { roi_freehand_2d, "amide_icon_roi_freehand_2d"},
  { roi_freehand_3d, "amide_icon_roi_freehand_3d"},
  { study, "amide_icon_study"},
  { target, "amide_icon_canvas_target" },
  { transfer_function, "amide_icon_transfer_function" },
  { threshold_style_min_max, "amide_icon_threshold_style_min_max" },
  { threshold_style_center_width, "amide_icon_threshold_style_center_width" },
  { thresholding, "amide_icon_thresholding" },  
  { thresholding_per_slice, "amide_icon_thresholding_per_slice" },
  { thresholding_per_frame, "amide_icon_thresholding_per_frame" },
  { thresholding_interpolate_frames, "amide_icon_thresholding_interpolate_frames" },
  { thresholding_global, "amide_icon_thresholding_global" },
  { view_transverse, "amide_icon_view_transverse" },
  { view_coronal, "amide_icon_view_coronal" },
  { view_sagittal, "amide_icon_view_sagittal" },
  { view_single, "amide_icon_view_mode_single" },
  { view_linked, "amide_icon_view_mode_linked_2way" },
  { view_linked3, "amide_icon_view_mode_linked_3way" },
  { window_abdomen, "amide_icon_window_abdomen" },
  { window_brain, "amide_icon_window_brain" },
  { window_extremities,"amide_icon_window_extremities" },
  { window_liver,"amide_icon_window_window_liver" },
  { window_lung,"amide_icon_window_lung" },
  { window_pelvis_soft_tissue,"amide_icon_window_pelvis_soft_tissue" },
  { window_skull_base,"amide_icon_window_skull_base"},
  { window_spine_a,"amide_icon_window_spine_a"},
  { window_spine_b,"amide_icon_window_spine_b"},
  { window_thorax_soft_tissue,"amide_icon_window_soft_tissue"},
};

const gchar * windowing_icons[AMITK_WINDOW_NUM] = {
  "amide_icon_window_abdomen",
  "amide_icon_window_brain",
  "amide_icon_window_extremities",
  "amide_icon_window_window_liver",
  "amide_icon_window_lung",
  "amide_icon_window_pelvis_soft_tissue",
  "amide_icon_window_skull_base",
  "amide_icon_window_spine_a",
  "amide_icon_window_spine_b",
  "amide_icon_window_soft_tissue",
};
  //  amide_icon_window_bone,
  //  amide_icon_window_soft_tissue_icon

void pixmaps_initialize_icons() {

  GtkIconFactory *icon_factory;
  GtkIconSet *icon_set; 
  GdkPixbuf * pixbuf;
  gint i;

  if (icons_initialized) return;

  /* create amide's group of icons */
  icon_factory = gtk_icon_factory_new();

  for (i=0; i < G_N_ELEMENTS(amide_icons); i++) {
    pixbuf = gdk_pixbuf_new_from_inline(-1, amide_icons[i].pixbuf_inline, FALSE, NULL);
    icon_set = gtk_icon_set_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);

    gtk_icon_factory_add (icon_factory, amide_icons[i].icon_id, icon_set);
    gtk_icon_set_unref (icon_set);
  }


  /* add this group of icon's to the list of icon's GTK will search for AMIDE */
  gtk_icon_factory_add_default (icon_factory); 
  g_object_unref (icon_factory);

  icons_initialized=TRUE;
  return;
}
