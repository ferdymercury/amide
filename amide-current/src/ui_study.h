/* ui_study.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2002 Andy Loening
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

typedef enum {
  HELP_INFO_LINE_1,
  HELP_INFO_LINE_1_SHIFT,
  HELP_INFO_LINE_2,
  HELP_INFO_LINE_2_SHIFT,
  HELP_INFO_LINE_3,
  HELP_INFO_LINE_3_SHIFT,
  HELP_INFO_LINE_3_CTRL,
  HELP_INFO_LINE_BLANK,
  HELP_INFO_LINE_LOCATION1,
  HELP_INFO_LINE_LOCATION2,
  NUM_HELP_INFO_LINES /* corresponds to the location line */
} help_info_line_t;

#define UI_STUDY_DEFAULT_ENTRY_WIDTH 75

#define UI_STUDY_MIN_ROI_WIDTH 1
#define UI_STUDY_MAX_ROI_WIDTH 5

typedef enum {
  DIM_X, DIM_Y, DIM_Z,
  CENTER_X, CENTER_Y, CENTER_Z,
  VOXEL_SIZE_X, VOXEL_SIZE_Y, VOXEL_SIZE_Z,
  SCALING_FACTOR, SCAN_START, FRAME_DURATION,
} which_entry_widget_t;

/* ui_study data structures */
typedef struct ui_study_t {
  GtkWidget * main_table;
  GtkWidget * app; /* pointer to the window managing this study */
  GtkWidget * thickness_spin;
  GtkWidget * zoom_spin;
  GtkWidget * tree[NUM_VIEW_MODES]; /* the tree showing the study data structure info */
  GtkWidget * tree_event_box[NUM_VIEW_MODES];
  GtkWidget * tree_scrolled[NUM_VIEW_MODES];
  pixelpoint_t tree_table_location[NUM_VIEW_MODES][NUM_LAYOUTS];
  GtkWidget * time_dialog;
  GtkWidget * time_button;
  volume_t * active_volume[NUM_VIEW_MODES]; /* which volume to use for actions that are specific for one volume*/
  study_t * study; /* pointer to the study data structure */
  GtkWidget * threshold_dialog; /* pointer to the threshold dialog */
  GtkWidget * preferences_dialog; /* pointer to the preferences dialog */

  /* canvas specific info */
  GtkWidget * center_table;
  GtkWidget * canvas_table[NUM_VIEW_MODES];
  GtkWidget * canvas[NUM_VIEW_MODES][NUM_VIEWS];

  /* help canvas info */
  GnomeCanvas * help_info;
  GnomeCanvasItem * help_legend[NUM_HELP_INFO_LINES];
  GnomeCanvasItem * help_line[NUM_HELP_INFO_LINES];

  /* stuff changed in the preferences dialog */
  layout_t canvas_layout;
  gint roi_width;
  GdkLineStyle line_style;
  gboolean dont_prompt_for_save_on_exit;

  gboolean study_altered;
  gboolean study_virgin;
  view_mode_t view_mode;

  guint reference_count;
} ui_study_t;

/* external functions */
ui_study_t * ui_study_free(ui_study_t * ui_study);
ui_study_t * ui_study_init(void);
volumes_t * ui_study_selected_volumes(ui_study_t * ui_study);
rois_t * ui_study_selected_rois(ui_study_t * ui_study);
void ui_study_make_active_volume(ui_study_t * ui_study, view_mode_t view_mode, volume_t * volume);
align_pt_t * ui_study_add_align_pt(ui_study_t * ui_study, volume_t * volume);
void ui_study_add_volume(ui_study_t * ui_study, volume_t * volume);
void ui_study_add_roi(ui_study_t * ui_study, roi_t * roi);
void ui_study_replace_study(ui_study_t * ui_study, study_t * study);
GtkWidget * ui_study_create(study_t * study, GtkWidget * parent_bin);
void ui_study_update_help_info(ui_study_t * ui_study, help_info_t which_info, 
			       realpoint_t new_point, amide_data_t value);
void ui_study_update_time_button(ui_study_t * ui_study);
void ui_study_update_thickness(ui_study_t * ui_study, floatpoint_t thickness);
void ui_study_update_zoom(ui_study_t * ui_study, floatpoint_t zoom);
void ui_study_update_title(ui_study_t * ui_study);
void ui_study_setup_layout(ui_study_t * ui_study);
void ui_study_setup_widgets(ui_study_t * ui_study);



#endif /* __UI_STUDY_H__ */
