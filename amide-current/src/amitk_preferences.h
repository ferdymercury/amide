/* amitk_preferences.h
 *
 * Part of amide - Amide's a Medical Image Data Examiner
 * Copyright (C) 2003-2014 Andy Loening
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

#ifndef __AMITK_PREFERENCES_H__
#define __AMITK_PREFERENCES_H__

/* header files that are always needed with this file */
#include <gtk/gtk.h>
#include "amitk_common.h"
#include "amitk_color_table.h"

G_BEGIN_DECLS

#define	AMITK_TYPE_PREFERENCES		  (amitk_preferences_get_type ())
#define AMITK_PREFERENCES(object)	  (G_TYPE_CHECK_INSTANCE_CAST ((object), AMITK_TYPE_PREFERENCES, AmitkPreferences))
#define AMITK_PREFERENCES_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_PREFERENCES, AmitkPreferencesClass))
#define AMITK_IS_PREFERENCES(object)	  (G_TYPE_CHECK_INSTANCE_TYPE ((object), AMITK_TYPE_PREFERENCES))
#define AMITK_IS_PREFERENCES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_PREFERENCES))
#define	AMITK_PREFERENCES_GET_CLASS(object)  (G_TYPE_CHECK_GET_CLASS ((object), AMITK_TYPE_PREFERENCES, AmitkPreferencesClass))

#define AMITK_PREFERENCES_WARNINGS_TO_CONSOLE(object)     (AMITK_PREFERENCES(object)->warnings_to_console)

#define AMITK_PREFERENCES_PROMPT_FOR_SAVE_ON_EXIT(object) (AMITK_PREFERENCES(object)->prompt_for_save_on_exit)
#define AMITK_PREFERENCES_WHICH_DEFAULT_DIRECTORY(object) (AMITK_PREFERENCES(object)->which_default_directory)
#define AMITK_PREFERENCES_DEFAULT_DIRECTORY(object)       (AMITK_PREFERENCES(object)->default_directory)

#define AMITK_PREFERENCES_CANVAS_ROI_WIDTH(pref)                (AMITK_PREFERENCES(pref)->canvas_roi_width)
#ifdef AMIDE_LIBGNOMECANVAS_AA
#define AMITK_PREFERENCES_CANVAS_ROI_TRANSPARENCY(pref)         (AMITK_PREFERENCES(pref)->canvas_roi_transparency)
#else
#define AMITK_PREFERENCES_CANVAS_LINE_STYLE(pref)               (AMITK_PREFERENCES(pref)->canvas_line_style)
#define AMITK_PREFERENCES_CANVAS_FILL_ROI(pref)                 (AMITK_PREFERENCES(pref)->canvas_fill_roi)
#endif
#define AMITK_PREFERENCES_CANVAS_LAYOUT(pref)                   (AMITK_PREFERENCES(pref)->canvas_layout)
#define AMITK_PREFERENCES_CANVAS_MAINTAIN_SIZE(pref)            (AMITK_PREFERENCES(pref)->canvas_maintain_size)
#define AMITK_PREFERENCES_CANVAS_TARGET_EMPTY_AREA(pref)        (AMITK_PREFERENCES(pref)->canvas_target_empty_area)
#define AMITK_PREFERENCES_PANEL_LAYOUT(pref)                    (AMITK_PREFERENCES(pref)->panel_layout)
#define AMITK_PREFERENCES_COLOR_TABLE(pref, modality)           (AMITK_PREFERENCES(pref)->color_table[modality])
#define AMITK_PREFERENCES_WINDOW(pref, which_window, limit)     (AMITK_PREFERENCES(pref)->window[which_window][limit])
#define AMITK_PREFERENCES_THRESHOLD_STYLE(pref)                 (AMITK_PREFERENCES(pref)->threshold_style)
#define AMITK_PREFERENCES_DIALOG(pref)                          (AMITK_PREFERENCES(pref)->dialog)

typedef enum {
  AMITK_WHICH_DEFAULT_DIRECTORY_NONE,
  AMITK_WHICH_DEFAULT_DIRECTORY_SPECIFIED,
  AMITK_WHICH_DEFAULT_DIRECTORY_WORKING,
  AMITK_WHICH_DEFAULT_DIRECTORY_NUM
} AmitkWhichDefaultDirectory;

#define AMITK_PREFERENCES_DEFAULT_CANVAS_ROI_WIDTH 2
#ifdef AMIDE_LIBGNOMECANVAS_AA
#define AMITK_PREFERENCES_DEFAULT_CANVAS_ROI_TRANSPARENCY 0.5
#else
#define AMITK_PREFERENCES_DEFAULT_CANVAS_LINE_STYLE GDK_LINE_SOLID
#define AMITK_PREFERENCES_DEFAULT_CANVAS_FILL_ROI TRUE
#endif
#define AMITK_PREFERENCES_DEFAULT_CANVAS_LAYOUT AMITK_LAYOUT_LINEAR
#define AMITK_PREFERENCES_DEFAULT_CANVAS_MAINTAIN_SIZE TRUE
#define AMITK_PREFERENCES_DEFAULT_CANVAS_TARGET_EMPTY_AREA 5
#define AMITK_PREFERENCES_DEFAULT_PANEL_LAYOUT AMITK_PANEL_LAYOUT_MIXED
#define AMITK_PREFERENCES_DEFAULT_WARNINGS_TO_CONSOLE FALSE
#define AMITK_PREFERENCES_DEFAULT_PROMPT_FOR_SAVE_ON_EXIT TRUE
#define AMITK_PREFERENCES_DEFAULT_SAVE_XIF_AS_DIRECTORY FALSE
#define AMITK_PREFERENCES_DEFAULT_WHICH_DEFAULT_DIRECTORY AMITK_WHICH_DEFAULT_DIRECTORY_NONE
#define AMITK_PREFERENCES_DEFAULT_DEFAULT_DIRECTORY NULL
#define AMITK_PREFERENCES_DEFAULT_THRESHOLD_STYLE AMITK_THRESHOLD_STYLE_MIN_MAX

#define AMITK_PREFERENCES_MIN_ROI_WIDTH 1
#define AMITK_PREFERENCES_MAX_ROI_WIDTH 5
#define AMITK_PREFERENCES_MIN_TARGET_EMPTY_AREA 0
#define AMITK_PREFERENCES_MAX_TARGET_EMPTY_AREA 25



typedef struct _AmitkPreferencesClass AmitkPreferencesClass;
typedef struct _AmitkPreferences      AmitkPreferences;

struct _AmitkPreferences {

  GObject parent;

  /* debug preferences */
  gboolean warnings_to_console;

  /* file saving preferences */
  gboolean prompt_for_save_on_exit;
  gboolean save_xif_as_directory;
  AmitkWhichDefaultDirectory which_default_directory;
  gchar * default_directory;

  /* canvas preferences -> study preferences */
  gint canvas_roi_width;
  gdouble canvas_roi_transparency;
#ifdef AMIDE_LIBGNOMECANVAS_AA
  GdkLineStyle canvas_line_style;
  gboolean canvas_fill_roi;
#endif
  AmitkLayout canvas_layout;
  gboolean canvas_maintain_size;
  gint canvas_target_empty_area; /* in pixels */
  AmitkPanelLayout panel_layout;

  /* data set preferences */
  AmitkColorTable color_table[AMITK_MODALITY_NUM];
  amide_data_t window[AMITK_WINDOW_NUM][AMITK_LIMIT_NUM];
  AmitkThresholdStyle threshold_style;

  /* misc pointers */
  GtkWidget * dialog;

};

struct _AmitkPreferencesClass
{
  GObjectClass parent_class;

  void (* data_set_preferences_changed) (AmitkPreferences * preferences);
  void (* study_preferences_changed)    (AmitkPreferences * preferences);
  void (* misc_preferences_changed)     (AmitkPreferences * preferences);

};


/* ------------ external functions ---------- */

GType	            amitk_preferences_get_type	                 (void);
AmitkPreferences*   amitk_preferences_new                        (void);
void                amitk_preferences_set_canvas_roi_width       (AmitkPreferences * preferences, 
							          gint roi_width);
#ifdef AMIDE_LIBGNOMECANVAS_AA
void                amitk_preferences_set_canvas_roi_transparency(AmitkPreferences * preferences, 
							          gdouble roi_transparency);
#else
void                amitk_preferences_set_canvas_line_style      (AmitkPreferences * preferences, 
							          GdkLineStyle line_style);
void                amitk_preferences_set_canvas_fill_roi        (AmitkPreferences * preferences, 
							          gboolean fill_roi);
#endif
void                amitk_preferences_set_canvas_layout          (AmitkPreferences * preferences, 
							          AmitkLayout layout);
void                amitk_preferences_set_canvas_maintain_size   (AmitkPreferences * preferences, 
							          gboolean maintain_size);
void                amitk_preferences_set_canvas_target_empty_area(AmitkPreferences * preferences, 
								   gint target_empty_area);
void                amitk_preferences_set_panel_layout           (AmitkPreferences * preferences, 
							          AmitkPanelLayout panel_layout);
void                amitk_preferences_set_warnings_to_console    (AmitkPreferences * preferences, 
								  gboolean new_value);
void                amitk_preferences_set_prompt_for_save_on_exit(AmitkPreferences * preferences,
								  gboolean new_value);
void                amitk_preferences_set_xif_as_directory       (AmitkPreferences * preferences,
							          gboolean new_value);
void                amitk_preferences_set_which_default_directory(AmitkPreferences * preferences,
								  const AmitkWhichDefaultDirectory which_default_directory);
void                amitk_preferences_set_default_directory      (AmitkPreferences * preferences,
								  const gchar * directory);
void                amitk_preferences_set_color_table            (AmitkPreferences * preferences,
								  AmitkModality modality,
								  AmitkColorTable color_table);
void                amitk_preferences_set_default_window         (AmitkPreferences * preferences,
								  const AmitkWindow window,
								  const AmitkLimit limit,
								  const amide_data_t value);
void                amitk_preferences_set_threshold_style        (AmitkPreferences * preferences,
								  const AmitkThresholdStyle threshold_style);
void                amitk_preferences_set_dialog                 (AmitkPreferences * preferences,
								  GtkWidget * dialog);
void                amitk_preferences_set_file_chooser_directory (AmitkPreferences * preferences,
								  GtkWidget * file_chooser);

/* external variables */
extern const gchar * amitk_which_default_directory_names[];

G_END_DECLS
#endif /* __AMITK_PREFERENCES_H__ */

