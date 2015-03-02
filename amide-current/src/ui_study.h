/* ui_study.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2014 Andy Loening
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

#ifndef __UI_STUDY_H__
#define __UI_STUDY_H__

/* header files that are always needed with this file */
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "amitk_study.h"
#include <libgnomecanvas/libgnomecanvas.h>

#define AMIDE_LIMIT_ZOOM_UPPER 10.0
#define AMIDE_LIMIT_ZOOM_LOWER 0.2
#define AMIDE_LIMIT_ZOOM_STEP 0.2

/* in percent */
#define AMIDE_LIMIT_FOV_UPPER 100.0
#define AMIDE_LIMIT_FOV_LOWER 10.0
#define AMIDE_LIMIT_FOV_STEP 10.0

typedef enum {
  HELP_INFO_LINE_1,
  HELP_INFO_LINE_1_SHIFT,
  HELP_INFO_LINE_2,
  HELP_INFO_LINE_2_SHIFT,
  HELP_INFO_LINE_3,
  HELP_INFO_LINE_3_SHIFT,
  HELP_INFO_LINE_3_CTRL,
  HELP_INFO_LINE_VARIABLE,
  HELP_INFO_LINE_BLANK,
  HELP_INFO_LINE_LOCATION1,
  HELP_INFO_LINE_LOCATION2,
  NUM_HELP_INFO_LINES /* corresponds to the location line */
} help_info_line_t;

#define UI_STUDY_DEFAULT_ENTRY_WIDTH 75

/* ui_study data structures */
typedef struct ui_study_t {
  GtkWindow * window; /* pointer to the window managing this study */
  GtkWidget * window_vbox;

  GtkWidget * thickness_spin;
  GtkWidget * zoom_spin;
  GtkWidget * fov_spin;
  GtkAction * interpolation_action[AMITK_INTERPOLATION_NUM];
  GtkWidget * rendering_menu;
  GtkAction * canvas_target_action;
  GtkAction * canvas_visible_action[AMITK_VIEW_NUM];
  GtkAction * view_mode_action[AMITK_VIEW_MODE_NUM];
  GtkAction * fuse_type_action[AMITK_FUSE_TYPE_NUM];
  GtkWidget * tree_view; /* the tree showing the study data structure info */
  GtkWidget * gate_dialog;
  GtkWidget * gate_button;
  GtkWidget * time_dialog;
  GtkWidget * time_button;
  AmitkObject * active_object; /* which object to use for actions that are for one object */
  AmitkStudy * study; /* pointer to the study data structure */
  GtkWidget * threshold_dialog; /* pointer to the threshold dialog */
  GtkWidget * progress_dialog;

  /* canvas specific info */
  GtkWidget * center_table;
  GtkWidget * canvas_table[AMITK_VIEW_MODE_NUM];
  GtkWidget * canvas_handle[AMITK_VIEW_MODE_NUM];
  GtkWidget * canvas[AMITK_VIEW_MODE_NUM][AMITK_VIEW_NUM];
  AmitkPanelLayout panel_layout;
  AmitkLayout  canvas_layout;

  /* help canvas info */
  GnomeCanvas * help_info;
  GnomeCanvasItem * help_legend[NUM_HELP_INFO_LINES];
  GnomeCanvasItem * help_line[NUM_HELP_INFO_LINES];

  /* preferences */
  AmitkPreferences * preferences;

  gboolean study_altered;
  gboolean study_virgin;

  guint reference_count;
} ui_study_t;

/* external functions */
ui_study_t * ui_study_free(ui_study_t * ui_study);
ui_study_t * ui_study_init(AmitkPreferences * preferences);
void ui_study_make_active_object(ui_study_t * ui_study, AmitkObject * object);
void ui_study_add_fiducial_mark(ui_study_t * ui_study, AmitkObject * parent_object,
				gboolean selected, AmitkPoint position);
void ui_study_add_roi(ui_study_t * ui_study, AmitkObject * parent_object, AmitkRoiType roi_type);
void ui_study_set_study(ui_study_t * ui_study, AmitkStudy * study);
GtkWidget * ui_study_create(AmitkStudy * study, AmitkPreferences * preferences);
void ui_study_update_help_info(ui_study_t * ui_study, AmitkHelpInfo which_info, 
			       AmitkPoint point, amide_data_t value);
void ui_study_update_canvas_visible_buttons(ui_study_t * ui_study);
void ui_study_update_gate_button(ui_study_t * ui_study);
void ui_study_update_time_button(ui_study_t * ui_study);
void ui_study_update_thickness(ui_study_t * ui_study, amide_real_t thickness);
void ui_study_update_zoom(ui_study_t * ui_study);
void ui_study_update_fov(ui_study_t * ui_study);
void ui_study_update_canvas_target(ui_study_t * ui_study);
void ui_study_update_fuse_type(ui_study_t * ui_study);
void ui_study_update_view_mode(ui_study_t * ui_study);
void ui_study_update_title(ui_study_t * ui_study);
void ui_study_update_layout(ui_study_t * ui_study);
void ui_study_setup_widgets(ui_study_t * ui_study);



#endif /* __UI_STUDY_H__ */
