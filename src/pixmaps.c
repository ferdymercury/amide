/* pixmaps.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2017 Andy Loening
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
#include "amide-icon-resources.h"
#include "pixmaps.h"


static gboolean icons_initialized=FALSE;

#define AMIDE_ICONS_NUM 44

static struct {
  const gchar * resource_path;
  const gchar * icon_id;
}  amide_icons[AMIDE_ICONS_NUM] = {
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "logo", .icon_id = "amide_icon_logo"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "align_pt", .icon_id = "amide_icon_align_pt"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "fuse_type_blend", .icon_id = "amide_icon_fuse_type_blend"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "fuse_type_overlay", .icon_id = "amide_icon_fuse_type_overlay"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "interpolation_nearest_neighbor", .icon_id = "amide_icon_interpolation_nearest_neighbor"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "interpolation_trilinear", .icon_id = "amide_icon_interpolation_trilinear"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "layout_linear", .icon_id = "amide_icon_layout_linear"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "layout_orthogonal", .icon_id = "amide_icon_layout_orthogonal"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "panels_mixed", .icon_id = "amide_icon_panels_mixed"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "panels_linear_x", .icon_id = "amide_icon_panels_linear_x"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "panels_linear_y", .icon_id = "amide_icon_panels_linear_y"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "roi_box", .icon_id = "amide_icon_roi_box"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "roi_cylinder", .icon_id = "amide_icon_roi_cylinder"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "roi_ellipsoid", .icon_id = "amide_icon_roi_ellipsoid"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "roi_isocontour_2d", .icon_id = "amide_icon_roi_isocontour_2d"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "roi_isocontour_3d", .icon_id = "amide_icon_roi_isocontour_3d"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "roi_freehand_2d", .icon_id = "amide_icon_roi_freehand_2d"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "roi_freehand_3d", .icon_id = "amide_icon_roi_freehand_3d"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "study", .icon_id = "amide_icon_study"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "target", .icon_id = "amide_icon_canvas_target"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "transfer_function", .icon_id = "amide_icon_transfer_function"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "threshold_style_min_max", .icon_id = "amide_icon_threshold_style_min_max"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "threshold_style_center_width", .icon_id = "amide_icon_threshold_style_center_width"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "thresholding", .icon_id = "amide_icon_thresholding"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "thresholding_per_slice", .icon_id = "amide_icon_thresholding_per_slice"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "thresholding_per_frame", .icon_id = "amide_icon_thresholding_per_frame"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "thresholding_interpolate_frames", .icon_id = "amide_icon_thresholding_interpolate_frames"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "thresholding_global", .icon_id = "amide_icon_thresholding_global"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "view_transverse", .icon_id = "amide_icon_view_transverse"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "view_coronal", .icon_id = "amide_icon_view_coronal"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "view_sagittal", .icon_id = "amide_icon_view_sagittal"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "view_single", .icon_id = "amide_icon_view_mode_single"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "view_linked", .icon_id = "amide_icon_view_mode_linked_2way"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "view_linked3", .icon_id = "amide_icon_view_mode_linked_3way"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "window_abdomen", .icon_id = "amide_icon_window_abdomen"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "window_brain", .icon_id = "amide_icon_window_brain"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "window_extremities", .icon_id = "amide_icon_window_extremities"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "window_liver", .icon_id = "amide_icon_window_liver"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "window_lung", .icon_id = "amide_icon_window_lung"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "window_pelvis_soft_tissue", .icon_id = "amide_icon_window_pelvis_soft_tissue"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "window_skull_base", .icon_id = "amide_icon_window_skull_base"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "window_spine_a", .icon_id = "amide_icon_window_spine_a"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "window_spine_b", .icon_id = "amide_icon_window_spine_b"},
  {.resource_path = AMIDE_ICON_GRESOURCE_PREFIX "window_soft_tissue", .icon_id = "amide_icon_window_soft_tissue"},
};

const gchar * windowing_icons[AMITK_WINDOW_NUM] = {
  "amide_icon_window_abdomen",
  "amide_icon_window_brain",
  "amide_icon_window_extremities",
  "amide_icon_window_liver",
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

  GtkIconFactory *icon_factory = NULL;
  GtkIconSet *icon_set = NULL;
  GResource *icon_resource = NULL;
  GdkPixbuf * pixbuf = NULL;
  GError *err = NULL;
  gint i;

  if (icons_initialized) return;

  icon_resource = amide_get_resource();
  g_resources_register(icon_resource);

  /* create amide's group of icons */
  icon_factory = gtk_icon_factory_new();

  for (i=0; i < AMIDE_ICONS_NUM; i++) {
    pixbuf = gdk_pixbuf_new_from_resource(amide_icons[i].resource_path, NULL);
    if (pixbuf == NULL) {
      /*if (err != NULL) {
        fprintf(stderr, "Unable to read load resource: %s\n", err->message);
        g_error_free(err);
      }*/
      fprintf (stderr, "%s : %s\n", amide_icons[i].resource_path, amide_icons[i].icon_id);
    }
    icon_set = gtk_icon_set_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);

    gtk_icon_factory_add (icon_factory, amide_icons[i].icon_id, icon_set);
    gtk_icon_set_unref (icon_set);
  }


  /* add this group of icon's to the list of icon's GTK will search for AMIDE */
  gtk_icon_factory_add_default (icon_factory); 
  g_object_unref (icon_factory);

  icons_initialized=TRUE;
  /* set the app icon now that the resource is working */
  gtk_window_set_default_icon_name(amide_icons[0].icon_id);
  return;
}
