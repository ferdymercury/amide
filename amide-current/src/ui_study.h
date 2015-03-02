/* ui_study.h
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

#ifndef __UI_STUDY_H__
#define __UI_STUDY_H__

/* header files that are always needed with this file */
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "study.h"
#include "ui_roi.h"
#include "ui_volume.h"

#define UI_STUDY_TRIANGLE_WIDTH 13.0
#define UI_STUDY_TRIANGLE_HEIGHT 8.0
#define UI_STUDY_MAIN_TABLE_HEIGHT 1
#define UI_STUDY_MAIN_TABLE_WIDTH 2
#define UI_STUDY_MIDDLE_TABLE_HEIGHT 3
#define UI_STUDY_MIDDLE_TABLE_WIDTH 3
#define UI_STUDY_SIZE_TREE_PIXMAPS 24
#define UI_STUDY_BLANK_WIDTH 128
#define UI_STUDY_BLANK_HEIGHT 128
#define UI_STUDY_DEFAULT_ENTRY_WIDTH 75
#define UI_STUDY_TREE_ACTIVE_COLUMN 0
#define UI_STUDY_TREE_TREE_COLUMN 1
#define UI_STUDY_TREE_TEXT_COLUMN 1
#define UI_STUDY_TREE_NUM_COLUMNS 2
#define UI_STUDY_HELP_FONT "fixed"
#define UI_STUDY_HELP_INFO_LINE_X 20.0
#define UI_STUDY_HELP_INFO_LINE_HEIGHT 13

#define UI_STUDY_MIN_ROI_WIDTH 1
#define UI_STUDY_MAX_ROI_WIDTH 5

typedef enum {
  VOLUME_MODE, ROI_MODE, NUM_MODES
} ui_study_mode_t;

typedef enum {
  UPDATE_ARROWS, 
  REFRESH_IMAGE,
  UPDATE_IMAGE, 
  UPDATE_PLANE_ADJUSTMENT, 
  UPDATE_ROIS,
  UPDATE_ALL
} ui_study_update_t;


typedef enum {
  HELP_INFO_BLANK,
  HELP_INFO_CANVAS_VOLUME,
  HELP_INFO_CANVAS_ROI,
  HELP_INFO_CANVAS_NEW_ROI,
  HELP_INFO_CANVAS_ALIGN,
  HELP_INFO_TREE_VOLUME,
  HELP_INFO_TREE_ROI,
  HELP_INFO_TREE_STUDY,
  HELP_INFO_TREE_NONE,
  HELP_INFO_UPDATE_LOCATION,
  NUM_HELP_INFOS
} ui_study_help_info_t;

typedef enum {
  HELP_INFO_LINE_1,
  HELP_INFO_LINE_1_SHIFT,
  HELP_INFO_LINE_2,
  HELP_INFO_LINE_2_SHIFT,
  HELP_INFO_LINE_3,
  HELP_INFO_LINE_3_SHIFT,
  HELP_INFO_LINE_LOCATION,
  HELP_INFO_LINE_LOCATION2,
  NUM_HELP_INFO_LINES
} ui_study_help_info_line_t;

typedef enum {
  TARGET_DELETE,
  TARGET_CREATE,
  TARGET_UPDATE,
  NUM_TARGET_ACTIONS
} ui_study_target_action_t;

typedef enum {
  DIM_X, DIM_Y, DIM_Z,
  CENTER_X, CENTER_Y, CENTER_Z,
  VOXEL_SIZE_X, VOXEL_SIZE_Y, VOXEL_SIZE_Z,
  SCALING_FACTOR, SCAN_START, FRAME_DURATION,
} which_entry_widget_t;

typedef enum {
  UI_STUDY_TREE_STUDY,
  UI_STUDY_TREE_VOLUME,
  UI_STUDY_TREE_ROI,
  NUM_UI_STUDY_TREE_TYPES
} ui_study_tree_object_t;

/* ui_study data structures */
typedef struct ui_study_t {
  GtkWidget * app; /* pointer to the window managing this study */
  GtkObject * thickness_adjustment;
  GtkWidget * location[NUM_VIEWS];
  GnomeCanvas * help_info;
  GtkWidget * thickness_spin_button;
  GtkWidget * tree; /* the tree showing the study data structure info */
  GtkWidget * study_dialog;
  gboolean study_selected;
  GtkWidget * time_dialog;
  GtkWidget * time_button;
  ui_study_mode_t current_mode; /* are we currently working on an roi or a volume */
  volume_t * current_volume; /* the last volume double clicked on */
  roi_t * current_roi; /* the last roi double clicked on */
  ui_volume_list_t * current_volumes; /* the currently selected volumes */ 
  ui_roi_list_t * current_rois; /* the currently selected rois */
  study_t * study; /* pointer to the study data structure */
  GtkWidget * threshold_dialog; /* pointer to the threshold dialog */
  GtkWidget * preferences_dialog; /* pointer to the preferences dialog */

  /* canvas specific info */
  GnomeCanvas * canvas[NUM_VIEWS];
  GnomeCanvasItem * canvas_image[NUM_VIEWS];
  realspace_t canvas_coord_frame[NUM_VIEWS];
  realpoint_t canvas_corner[NUM_VIEWS];
  GdkPixbuf * canvas_rgb[NUM_VIEWS];
  volume_list_t * current_slices[NUM_VIEWS];

  /* stuff changed in the preferences dialog */
  gint roi_width;
  GdkLineStyle line_style;

  guint reference_count;
} ui_study_t;

/* external functions */
void ui_study_add_volume(ui_study_t * ui_study, volume_t * new_volume);
void ui_study_import_file(ui_study_t * ui_study, gchar * import_filename, 
			  gchar * model_filename, import_method_t import_method);
GtkWidget * ui_study_create(study_t * study, GtkWidget * parent_bin);
void ui_study_update_help_info(ui_study_t * ui_study, ui_study_help_info_t which_info, realpoint_t new_point);
realpoint_t ui_study_cp_2_rp(ui_study_t * ui_study, view_t view, canvaspoint_t canvas_cp);
canvaspoint_t ui_study_rp_2_cp(ui_study_t * ui_study, view_t view, realpoint_t canvas_rp);
void ui_study_update_time_button(ui_study_t * ui_study);
void ui_study_update_coords_current_view(ui_study_t * ui_study, view_t view);
void ui_study_update_targets(ui_study_t * ui_study, ui_study_target_action_t action,
			     realpoint_t center, guint32 outline_color);
GtkObject * ui_study_update_plane_adjustment(ui_study_t * ui_study, view_t view);
void ui_study_update_thickness_adjustment(ui_study_t * ui_study);
GnomeCanvasItem *  ui_study_update_canvas_roi(ui_study_t * ui_study, view_t view, 
					      GnomeCanvasItem * roi_item, roi_t * roi);
void ui_study_update_canvas_rois(ui_study_t * ui_study, view_t view);
void ui_study_update_canvas(ui_study_t * ui_study, view_t i_view,  ui_study_update_t update);
void ui_study_tree_update_active_leaf(ui_study_t * ui_study, GtkWidget * leaf);
void ui_study_tree_add_roi(ui_study_t * ui_study, roi_t * roi);
void ui_study_tree_add_volume(ui_study_t * ui_study, volume_t * volume);

/* internal functions */
void ui_study_update_canvas_arrows(ui_study_t * ui_study, view_t view);
void ui_study_update_canvas_image(ui_study_t * ui_study, view_t view);
void ui_study_update_tree(ui_study_t * ui_study);
void ui_study_setup_widgets(ui_study_t * ui_study);
ui_study_t * ui_study_free(ui_study_t * ui_study);
ui_study_t * ui_study_init(void);



/* external variables */



#endif /* __UI_STUDY_H__ */
