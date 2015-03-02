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


#define UI_STUDY_TRIANGLE_WIDTH 15
#define UI_STUDY_TRIANGLE_HEIGHT 7.5
#define UI_STUDY_PACKING_TABLE_HEIGHT 5
#define UI_STUDY_PACKING_TABLE_WIDTH 5
#define UI_STUDY_RIGHT_TABLE_HEIGHT 8
#define UI_STUDY_RIGHT_TABLE_WIDTH 2
#define UI_STUDY_SIZE_TREE_PIXMAPS 24
#define UI_STUDY_NEW_ROI_MODE_CURSOR GDK_DRAFT_SMALL
#define UI_STUDY_NEW_ROI_MOTION_CURSOR GDK_PENCIL
#define UI_STUDY_NO_ROI_MODE_CURSOR GDK_QUESTION_ARROW
#define UI_STUDY_OLD_ROI_MODE_CURSOR GDK_DRAFT_SMALL
#define UI_STUDY_OLD_ROI_RESIZE_CURSOR GDK_X_CURSOR
#define UI_STUDY_OLD_ROI_ROTATE_CURSOR GDK_EXCHANGE
#define UI_STUDY_OLD_ROI_SHIFT_CURSOR GDK_FLEUR
#define UI_STUDY_VOLUME_MODE_CURSOR GDK_CROSSHAIR
#define UI_STUDY_WAIT_CURSOR GDK_WATCH
#define UI_STUDY_BLANK_WIDTH 128
#define UI_STUDY_BLANK_HEIGHT 256
#define UI_STUDY_DEFAULT_ENTRY_WIDTH 75
#define UI_STUDY_TREE_ACTIVE_COLUMN 0
#define UI_STUDY_TREE_TREE_COLUMN 1
#define UI_STUDY_TREE_TEXT_COLUMN 1
#define UI_STUDY_TREE_NUM_COLUMNS 2

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
  UI_STUDY_DEFAULT,
  UI_STUDY_NEW_ROI_MODE,
  UI_STUDY_NEW_ROI_MOTION, 
  UI_STUDY_NO_ROI_MODE,
  UI_STUDY_OLD_ROI_MODE,
  UI_STUDY_OLD_ROI_RESIZE,
  UI_STUDY_OLD_ROI_ROTATE,
  UI_STUDY_OLD_ROI_SHIFT,
  UI_STUDY_VOLUME_MODE, 
  UI_STUDY_WAIT,
  NUM_CURSORS
} ui_study_cursor_t;

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
  AXIS_X_X, AXIS_X_Y, AXIS_X_Z,
  AXIS_Y_X, AXIS_Y_Y, AXIS_Y_Z,
  AXIS_Z_X, AXIS_Z_Y, AXIS_Z_Z,
  CONVERSION_FACTOR, SCAN_START, FRAME_DURATION,
} which_entry_widget_t;

/* ui_study data structures */
typedef struct ui_study_t {
  GnomeApp * app; /* pointer to the window managing this study */
  GnomeCanvas * canvas[NUM_VIEWS];
  GtkAdjustment * thickness_adjustment;
  GdkCursor * cursor[NUM_CURSORS];
  GSList * cursor_stack;
  GtkWidget * thickness_spin_button;
  GtkWidget * color_table_menu;
  GtkWidget * add_roi_option_menu;
  GtkWidget * tree; /* the tree showing the study data structure info */
  GtkCTreeNode * tree_study;
  GtkCTreeNode * tree_volumes;
  GtkCTreeNode * tree_rois;
  GtkWidget * study_dialog;
  gboolean study_selected;
  GtkWidget * time_dialog;
  GtkWidget * time_button;
  ui_study_mode_t current_mode; /* are we currently working on an roi or a volume */
  volume_t * current_volume; /* the last volume double clicked on */
  roi_t * current_roi; /* the last roi double clicked on */
  ui_volume_list_t * current_volumes; /* the currently selected volumes */ 
  ui_roi_list_t * current_rois; /* the currently selected rois */
  roi_grain_t default_roi_grain;
  study_t * study; /* pointer to the study data structure */
  ui_threshold_t * threshold; /* pointer to the threshold widget data structure */
  ui_series_t * series; /* pointer to the series widget data structure */
  guint reference_count;
} ui_study_t;

/* external functions */
GnomeApp * ui_study_create(study_t * study);
void ui_study_update_coords_current_view(ui_study_t * ui_study, view_t view);
void ui_study_update_targets(ui_study_t * ui_study, ui_study_target_action_t action, 
			     realpoint_t center, guint32 outline_color);
GtkAdjustment * ui_study_update_plane_adjustment(ui_study_t * ui_study, view_t view);
void ui_study_update_thickness_adjustment(ui_study_t * ui_study);
void ui_study_place_cursor(ui_study_t * ui_study, ui_study_cursor_t which_cursor, GtkWidget * widget);
void ui_study_remove_cursor(ui_study_t * ui_study, GtkWidget * widget);
GnomeCanvasItem *  ui_study_update_canvas_roi(ui_study_t * ui_study, view_t view, 
					      GnomeCanvasItem * roi_item, roi_t * roi);
void ui_study_update_canvas_rois(ui_study_t * ui_study, view_t view);
void ui_study_update_canvas(ui_study_t * ui_study, view_t i_view, 
			    ui_study_update_t update);
void ui_study_tree_update_active_row(ui_study_t * ui_study, gint row);
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
extern gchar * ui_study_tree_active_mark;
