/* amitk_preferences.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
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

#include "amide_config.h"
#include <string.h>
#include "amide.h"
#include "amide_gconf.h"

#include "amitk_preferences.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"
#include "amitk_data_set.h"

#define GCONF_AMIDE_ROI "ROI"
#define GCONF_AMIDE_CANVAS "CANVAS"
#define GCONF_AMIDE_MISC "MISC"
#define GCONF_AMIDE_DATASETS "DATASETS"
#define GCONF_AMIDE_WINDOWS "WINDOWS"

enum {
  DATA_SET_PREFERENCES_CHANGED,
  STUDY_PREFERENCES_CHANGED,
  MISC_PREFERENCES_CHANGED,
  LAST_SIGNAL
};

static void preferences_class_init          (AmitkPreferencesClass *klass);
static void preferences_init                (AmitkPreferences      *object);
static void preferences_finalize            (GObject           *object);
static GObjectClass * parent_class;
static guint     preferences_signals[LAST_SIGNAL];

/* external variables */
const gchar * amitk_which_default_directory_names[] = {
  N_("None"),
  N_("Specified Directory"),
  N_("Working Directory")
};


GType amitk_preferences_get_type(void) {

  static GType preferences_type = 0;

  if (!preferences_type)
    {
      static const GTypeInfo preferences_info =
      {
	sizeof (AmitkPreferencesClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) preferences_class_init,
	(GClassFinalizeFunc) NULL,
	NULL,		/* class_data */
	sizeof (AmitkPreferences),
	0,			/* n_preallocs */
	(GInstanceInitFunc) preferences_init,
	NULL /* value table */
      };
      
      preferences_type = g_type_register_static (G_TYPE_OBJECT, "AmitkPreferences", &preferences_info, 0);
    }
  
  return preferences_type;
}


static void preferences_class_init (AmitkPreferencesClass * class) {

  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent(class);
  
  gobject_class->finalize = preferences_finalize;

  preferences_signals[DATA_SET_PREFERENCES_CHANGED] =
    g_signal_new ("data_set_preferences_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkPreferencesClass, data_set_preferences_changed),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
  preferences_signals[STUDY_PREFERENCES_CHANGED] =
    g_signal_new ("study_preferences_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkPreferencesClass, study_preferences_changed),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
  preferences_signals[MISC_PREFERENCES_CHANGED] =
    g_signal_new ("misc_preferences_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkPreferencesClass, misc_preferences_changed),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
}

static void preferences_init (AmitkPreferences * preferences) {

  gchar * temp_str;
  AmitkLimit i_limit;
  AmitkWindow i_window;
  AmitkModality i_modality;


  /* load in saved preferences */
  preferences->canvas_roi_width = 
    amide_gconf_get_int_with_default(GCONF_AMIDE_ROI,"Width", AMITK_PREFERENCES_DEFAULT_CANVAS_ROI_WIDTH);

#ifdef AMIDE_LIBGNOMECANVAS_AA
  preferences->canvas_roi_transparency = 
    amide_gconf_get_float_with_default(GCONF_AMIDE_ROI,"Transparency", AMITK_PREFERENCES_DEFAULT_CANVAS_ROI_TRANSPARENCY);

#else
  preferences->canvas_line_style = 
    amide_gconf_get_int_with_default(GCONF_AMIDE_ROI,"LineStyle",AMITK_PREFERENCES_DEFAULT_CANVAS_LINE_STYLE);

  preferences->canvas_fill_roi = 
    amide_gconf_get_bool_with_default(GCONF_AMIDE_ROI,"FillIsocontour", AMITK_PREFERENCES_DEFAULT_CANVAS_FILL_ROI);
#endif

  preferences->canvas_layout = 
    amide_gconf_get_int_with_default(GCONF_AMIDE_CANVAS,"Layout", AMITK_PREFERENCES_DEFAULT_CANVAS_LAYOUT);

  preferences->canvas_maintain_size = 
    amide_gconf_get_bool_with_default(GCONF_AMIDE_CANVAS,"MaintainSize", AMITK_PREFERENCES_DEFAULT_CANVAS_MAINTAIN_SIZE);

  preferences->canvas_target_empty_area = 
    amide_gconf_get_int_with_default(GCONF_AMIDE_CANVAS,"TargetEmptyArea", AMITK_PREFERENCES_DEFAULT_CANVAS_TARGET_EMPTY_AREA);

  preferences->panel_layout = 
    amide_gconf_get_int_with_default(GCONF_AMIDE_CANVAS,"PanelLayout", AMITK_PREFERENCES_DEFAULT_PANEL_LAYOUT);

  preferences->warnings_to_console = 
    amide_gconf_get_bool_with_default(GCONF_AMIDE_MISC,"WarningsToConsole", AMITK_PREFERENCES_DEFAULT_WARNINGS_TO_CONSOLE);

  preferences->prompt_for_save_on_exit = 
    amide_gconf_get_bool_with_default(GCONF_AMIDE_MISC,"PromptForSaveOnExit", AMITK_PREFERENCES_DEFAULT_PROMPT_FOR_SAVE_ON_EXIT);

  preferences->which_default_directory = 
    amide_gconf_get_int_with_default(GCONF_AMIDE_MISC,"WhichDefaultDirectory", AMITK_PREFERENCES_DEFAULT_WHICH_DEFAULT_DIRECTORY);

  preferences->default_directory = 
    amide_gconf_get_string_with_default(GCONF_AMIDE_MISC,"DefaultDirectory", AMITK_PREFERENCES_DEFAULT_DEFAULT_DIRECTORY);

  for (i_modality=0; i_modality<AMITK_MODALITY_NUM; i_modality++) {
    temp_str = g_strdup_printf("DefaultColorTable%s", amitk_modality_get_name(i_modality));
    preferences->color_table[i_modality] = 
      amide_gconf_get_int_with_default(GCONF_AMIDE_DATASETS,temp_str, amitk_modality_default_color_table[i_modality]);
    g_free(temp_str);
  }

  for (i_window = 0; i_window < AMITK_WINDOW_NUM; i_window++) 
    for (i_limit = 0; i_limit < AMITK_LIMIT_NUM; i_limit++) {
      temp_str = g_strdup_printf("%s-%s", amitk_window_get_name(i_window), amitk_limit_get_name(i_limit));
      preferences->window[i_window][i_limit] =
	amide_gconf_get_float_with_default(GCONF_AMIDE_WINDOWS,temp_str, amitk_window_default[i_window][i_limit]);
    }

  preferences->threshold_style = 
    amide_gconf_get_int_with_default(GCONF_AMIDE_DATASETS,"ThresholdStyle", AMITK_PREFERENCES_DEFAULT_THRESHOLD_STYLE);

  preferences->dialog = NULL;

  return;
}


static void preferences_finalize (GObject *object) {

  //  AmitkPreferences * preferences = AMITK_PREFERENCES(object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


AmitkPreferences * amitk_preferences_new (void) {

  AmitkPreferences * preferences;

  preferences = g_object_new(amitk_preferences_get_type(), NULL);

  return preferences;
}


void amitk_preferences_set_canvas_roi_width(AmitkPreferences * preferences, 
					    gint roi_width) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));

  if (roi_width < AMITK_PREFERENCES_MIN_ROI_WIDTH) roi_width = AMITK_PREFERENCES_MIN_ROI_WIDTH;
  if (roi_width > AMITK_PREFERENCES_MAX_ROI_WIDTH) roi_width = AMITK_PREFERENCES_MAX_ROI_WIDTH;

  if (AMITK_PREFERENCES_CANVAS_ROI_WIDTH(preferences) != roi_width) {
    preferences->canvas_roi_width = roi_width;
    amide_gconf_set_int(GCONF_AMIDE_ROI,"Width", roi_width);
    g_signal_emit(G_OBJECT(preferences), preferences_signals[STUDY_PREFERENCES_CHANGED], 0);
  }


  return;
}

#ifdef AMIDE_LIBGNOMECANVAS_AA
void amitk_preferences_set_canvas_roi_transparency(AmitkPreferences * preferences, 
						   gdouble roi_transparency) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));
  g_return_if_fail(roi_transparency >= 0.0);
  g_return_if_fail(roi_transparency <= 1.0);

  if (AMITK_PREFERENCES_CANVAS_ROI_TRANSPARENCY(preferences) != roi_transparency) {
    preferences->canvas_roi_transparency = roi_transparency;
    amide_gconf_set_float(GCONF_AMIDE_ROI,"Transparency", roi_transparency);
    g_signal_emit(G_OBJECT(preferences), preferences_signals[STUDY_PREFERENCES_CHANGED], 0);
  }


  return;
}

#else
void amitk_preferences_set_canvas_line_style(AmitkPreferences * preferences, GdkLineStyle line_style) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));

  if (AMITK_PREFERENCES_CANVAS_LINE_STYLE(preferences) != line_style) {
    preferences->canvas_line_style = line_style;
    amide_gconf_set_int(GCONF_AMIDE_ROI,"LineStyle",line_style);
    g_signal_emit(G_OBJECT(preferences), preferences_signals[STUDY_PREFERENCES_CHANGED], 0);
  }

  return;
}

void amitk_preferences_set_canvas_fill_roi(AmitkPreferences * preferences, gboolean fill_roi) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));

  if (AMITK_PREFERENCES_CANVAS_FILL_ROI(preferences) != fill_roi) {
    preferences->canvas_fill_roi = fill_roi;
    amide_gconf_set_bool(GCONF_AMIDE_ROI,"FillIsocontour",fill_roi);
    g_signal_emit(G_OBJECT(preferences), preferences_signals[STUDY_PREFERENCES_CHANGED], 0);
  }

  return;
}
#endif

void amitk_preferences_set_canvas_layout(AmitkPreferences * preferences, 
					 AmitkLayout layout) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));

  if (AMITK_PREFERENCES_CANVAS_LAYOUT(preferences) != layout) {
    preferences->canvas_layout = layout;
    amide_gconf_set_int(GCONF_AMIDE_CANVAS,"Layout", layout);
    g_signal_emit(G_OBJECT(preferences), preferences_signals[STUDY_PREFERENCES_CHANGED], 0);
  }

  return;
}

void amitk_preferences_set_canvas_maintain_size(AmitkPreferences * preferences, 
						gboolean maintain_size) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));

  if (AMITK_PREFERENCES_CANVAS_MAINTAIN_SIZE(preferences) != maintain_size) {
    preferences->canvas_maintain_size = maintain_size;
    amide_gconf_set_bool(GCONF_AMIDE_CANVAS,"MaintainSize",maintain_size);
    g_signal_emit(G_OBJECT(preferences), preferences_signals[STUDY_PREFERENCES_CHANGED], 0);
  }

  return;
};

void amitk_preferences_set_canvas_target_empty_area(AmitkPreferences * preferences, 
						    gint target_empty_area) {
  
  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));

  /* sanity checks */
  if (target_empty_area < AMITK_PREFERENCES_MIN_TARGET_EMPTY_AREA) 
    target_empty_area = AMITK_PREFERENCES_MIN_TARGET_EMPTY_AREA;
  if (target_empty_area > AMITK_PREFERENCES_MAX_TARGET_EMPTY_AREA) 
    target_empty_area = AMITK_PREFERENCES_MAX_TARGET_EMPTY_AREA;


if (AMITK_PREFERENCES_CANVAS_TARGET_EMPTY_AREA(preferences) != target_empty_area) {
    preferences->canvas_target_empty_area = target_empty_area;
    amide_gconf_set_int(GCONF_AMIDE_CANVAS,"TargetEmptyArea", target_empty_area);
    g_signal_emit(G_OBJECT(preferences), preferences_signals[STUDY_PREFERENCES_CHANGED], 0);
  }

  return;
}


void amitk_preferences_set_panel_layout(AmitkPreferences * preferences, 
					 AmitkPanelLayout panel_layout) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));

  if (AMITK_PREFERENCES_PANEL_LAYOUT(preferences) != panel_layout) {
    preferences->panel_layout = panel_layout;
    amide_gconf_set_int(GCONF_AMIDE_CANVAS,"PanelLayout", panel_layout);
    g_signal_emit(G_OBJECT(preferences), preferences_signals[STUDY_PREFERENCES_CHANGED], 0);
  }

  return;
}



void amitk_preferences_set_warnings_to_console(AmitkPreferences * preferences, gboolean new_value) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));

  if (AMITK_PREFERENCES_WARNINGS_TO_CONSOLE(preferences) != new_value) {
    preferences->warnings_to_console = new_value;
    amide_gconf_set_bool(GCONF_AMIDE_MISC,"WarningsToConsole",new_value);
    g_signal_emit(G_OBJECT(preferences), preferences_signals[MISC_PREFERENCES_CHANGED], 0);
  }
  return;
}

void amitk_preferences_set_prompt_for_save_on_exit(AmitkPreferences * preferences, gboolean new_value) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));

  if (AMITK_PREFERENCES_PROMPT_FOR_SAVE_ON_EXIT(preferences) != new_value) {
    preferences->prompt_for_save_on_exit = new_value;
    amide_gconf_set_bool(GCONF_AMIDE_MISC,"PromptForSaveOnExit",new_value);
    g_signal_emit(G_OBJECT(preferences), preferences_signals[MISC_PREFERENCES_CHANGED], 0);
  }
  return;
}

void amitk_preferences_set_which_default_directory(AmitkPreferences * preferences, AmitkWhichDefaultDirectory new_value) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));

  if (AMITK_PREFERENCES_WHICH_DEFAULT_DIRECTORY(preferences) != new_value) {
    preferences->which_default_directory = new_value;
    amide_gconf_set_int(GCONF_AMIDE_MISC,"WhichDefaultDirectory",new_value);
    g_signal_emit(G_OBJECT(preferences), preferences_signals[MISC_PREFERENCES_CHANGED], 0);
  }
  return;
}



void amitk_preferences_set_default_directory(AmitkPreferences * preferences, const gchar * new_directory) {

  gboolean different=FALSE;
  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));

  if (((AMITK_PREFERENCES_DEFAULT_DIRECTORY(preferences) == NULL) && (new_directory != NULL)) ||
      ((AMITK_PREFERENCES_DEFAULT_DIRECTORY(preferences) != NULL) && (new_directory == NULL)))
    different=TRUE;
  else if ((AMITK_PREFERENCES_DEFAULT_DIRECTORY(preferences) != NULL) && (new_directory != NULL))
    if (strcmp(AMITK_PREFERENCES_DEFAULT_DIRECTORY(preferences), new_directory) != 0)
      different=TRUE;

  if (different) {
    if (preferences->default_directory != NULL)
      g_free(preferences->default_directory);
    if (new_directory != NULL)
      preferences->default_directory = g_strdup(new_directory);
    amide_gconf_set_string(GCONF_AMIDE_MISC,"DefaultDirectory", new_directory);
    g_signal_emit(G_OBJECT(preferences), preferences_signals[MISC_PREFERENCES_CHANGED], 0);
  }
  return;
}

void amitk_preferences_set_color_table(AmitkPreferences * preferences,
				       AmitkModality modality,
				       AmitkColorTable color_table) {
  gchar * temp_string;

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));
  g_return_if_fail((modality >= 0) && (modality < AMITK_MODALITY_NUM));
  g_return_if_fail((color_table >= 0) && (color_table < AMITK_COLOR_TABLE_NUM));

  if (AMITK_PREFERENCES_COLOR_TABLE(preferences,modality) != color_table) {
    preferences->color_table[modality] = color_table;

    temp_string = g_strdup_printf("DefaultColorTable%s", 
				  amitk_modality_get_name(modality));
    amide_gconf_set_int(GCONF_AMIDE_DATASETS,temp_string, color_table);
    g_free(temp_string);
    g_signal_emit(G_OBJECT(preferences), preferences_signals[DATA_SET_PREFERENCES_CHANGED], 0);
  }

  return;
}

void amitk_preferences_set_default_window(AmitkPreferences * preferences,
					  const AmitkWindow window,
					  const AmitkLimit limit,
					  const amide_data_t value) {

  gchar * temp_string;

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));
  g_return_if_fail((window >= 0) && (window < AMITK_WINDOW_NUM));
  g_return_if_fail((limit >= 0) && (limit < AMITK_LIMIT_NUM));

  if (!REAL_EQUAL(AMITK_PREFERENCES_WINDOW(preferences, window, limit), value)) {
    preferences->window[window][limit] = value;

    temp_string = g_strdup_printf("%s-%s", amitk_window_get_name(window),
				  amitk_limit_get_name(limit));
    amide_gconf_set_float(GCONF_AMIDE_WINDOWS,temp_string,value);
    g_free(temp_string);
    g_signal_emit(G_OBJECT(preferences), preferences_signals[DATA_SET_PREFERENCES_CHANGED], 0);
  }

  return;
}

void amitk_preferences_set_threshold_style(AmitkPreferences * preferences,
					   const AmitkThresholdStyle threshold_style) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));

  if (AMITK_PREFERENCES_THRESHOLD_STYLE(preferences) != threshold_style) {
    preferences->threshold_style = threshold_style;
    amide_gconf_set_int(GCONF_AMIDE_DATASETS,"ThresholdStyle", threshold_style);
    g_signal_emit(G_OBJECT(preferences), preferences_signals[DATA_SET_PREFERENCES_CHANGED], 0);
  }
}

void amitk_preferences_set_dialog(AmitkPreferences * preferences,
				  GtkWidget * dialog) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));
  preferences->dialog = dialog;

}

/* conviencence function */
void amitk_preferences_set_file_chooser_directory(AmitkPreferences * preferences,
						  GtkWidget * file_chooser) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));
  g_return_if_fail(GTK_IS_FILE_CHOOSER(file_chooser));

  switch(AMITK_PREFERENCES_WHICH_DEFAULT_DIRECTORY(preferences)) {

  case AMITK_WHICH_DEFAULT_DIRECTORY_SPECIFIED:
    if (AMITK_PREFERENCES_DEFAULT_DIRECTORY(preferences) != NULL)
      gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser), 
					  AMITK_PREFERENCES_DEFAULT_DIRECTORY(preferences));
    break;

  case AMITK_WHICH_DEFAULT_DIRECTORY_WORKING:
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser), "");
    break;

  case AMITK_WHICH_DEFAULT_DIRECTORY_NONE:
  default:
    break;
  }
}


