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
#include "amitk_study.h"
#include <gnome.h>

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

#define UI_STUDY_MIN_ROI_WIDTH 1
#define UI_STUDY_MAX_ROI_WIDTH 5
#define UI_STUDY_MAX_TARGET_EMPTY_AREA 25


/* ui_study data structures */
typedef struct ui_study_t {
  GtkWidget * main_table;
  GtkWidget * app; /* pointer to the window managing this study */
  GtkWidget * thickness_spin;
  GtkWidget * zoom_spin;
  GtkWidget * interpolation_button[AMITK_INTERPOLATION_NUM];
  GtkWidget * tree; /* the tree showing the study data structure info */
  GtkWidget * time_dialog;
  GtkWidget * time_button;
  AmitkDataSet * active_ds; /* which data set to use for actions that are specific for one data set*/
  AmitkStudy * study; /* pointer to the study data structure */
  GtkWidget * threshold_dialog; /* pointer to the threshold dialog */
  GtkWidget * preferences_dialog; /* pointer to the preferences dialog */

  /* canvas specific info */
  GtkWidget * center_table;
  GtkWidget * canvas_table[AMITK_VIEW_MODE_NUM];
  GtkWidget * canvas[AMITK_VIEW_MODE_NUM][AMITK_VIEW_NUM];

  /* help canvas info */
  GnomeCanvas * help_info;
  GnomeCanvasItem * help_legend[NUM_HELP_INFO_LINES];
  GnomeCanvasItem * help_line[NUM_HELP_INFO_LINES];

  /* stuff changed in the preferences dialog */
  AmitkLayout canvas_layout;
  gint roi_width;
  GdkLineStyle line_style;
  gboolean canvas_leave_target;
  gint canvas_target_empty_area;
  gboolean dont_prompt_for_save_on_exit;

  gboolean study_altered;
  gboolean study_virgin;
  AmitkViewMode view_mode;

  guint reference_count;
} ui_study_t;

/* external functions */
ui_study_t * ui_study_free(ui_study_t * ui_study);
ui_study_t * ui_study_init(void);
GList * ui_study_selected_data_sets(ui_study_t * ui_study);
GList * ui_study_selected_rois(ui_study_t * ui_study);
void ui_study_make_active_data_set(ui_study_t * ui_study, AmitkDataSet * ds);
void ui_study_add_fiducial_mark(ui_study_t * ui_study, AmitkObject * parent_object,
				gboolean selected, AmitkPoint position);
void ui_study_add_roi(ui_study_t * ui_study, AmitkObject * parent_object, AmitkRoiType roi_type);
void ui_study_add_data_set(ui_study_t * ui_study, AmitkDataSet * data_set);
void ui_study_replace_study(ui_study_t * ui_study, AmitkStudy * study);
GtkWidget * ui_study_create(AmitkStudy * study);
void ui_study_update_help_info(ui_study_t * ui_study, AmitkHelpInfo which_info, 
			       AmitkPoint new_point, amide_data_t value);
void ui_study_update_time_button(ui_study_t * ui_study);
void ui_study_update_thickness(ui_study_t * ui_study, amide_real_t thickness);
void ui_study_update_zoom(ui_study_t * ui_study);
void ui_study_update_interpolation(ui_study_t * ui_study);
void ui_study_update_title(ui_study_t * ui_study);
void ui_study_setup_layout(ui_study_t * ui_study);
void ui_study_setup_widgets(ui_study_t * ui_study);



#endif /* __UI_STUDY_H__ */
