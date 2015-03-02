/* amitk_preferences.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2003-2004 Andy Loening
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

#ifndef AMIDE_WIN32_HACKS
#include <libgnome/libgnome.h>
#endif

#include "amitk_preferences.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"
#include "amitk_data_set.h"


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

#ifndef AMIDE_WIN32_HACKS
  gchar * temp_string;
  gint default_value;
  gfloat temp_float;
  gint temp_int;
#endif
  AmitkLimit i_limit;
  AmitkWindow i_window;
  AmitkModality i_modality;


  /* load in saved preferences */
#ifndef AMIDE_WIN32_HACKS
  gnome_config_push_prefix("/"PACKAGE"/");

  temp_int = gnome_config_get_int_with_default("ROI/Width", &default_value);
  preferences->canvas_roi_width = default_value ? AMITK_PREFERENCES_DEFAULT_CANVAS_ROI_WIDTH : temp_int;

  temp_int = gnome_config_get_int_with_default("ROI/LineStyle", &default_value); 
  preferences->canvas_line_style = default_value ? AMITK_PREFERENCES_DEFAULT_CANVAS_LINE_STYLE : temp_int;

  temp_int = gnome_config_get_int_with_default("ROI/FillIsocontour", &default_value); 
  preferences->canvas_fill_isocontour = default_value ? AMITK_PREFERENCES_DEFAULT_CANVAS_FILL_ISOCONTOUR : temp_int;

  temp_int = gnome_config_get_int_with_default("CANVAS/Layout", &default_value);
  preferences->canvas_layout = default_value ? AMITK_PREFERENCES_DEFAULT_CANVAS_LAYOUT : temp_int;

  temp_int = !gnome_config_get_int_with_default("CANVAS/MinimizeSize", &default_value);
  preferences->canvas_maintain_size = default_value ? AMITK_PREFERENCES_DEFAULT_CANVAS_MAINTAIN_SIZE : temp_int;

  temp_int = gnome_config_get_int_with_default("CANVAS/TargetEmptyArea", &default_value); 
  preferences->canvas_target_empty_area = default_value ? AMITK_PREFERENCES_DEFAULT_CANVAS_TARGET_EMPTY_AREA : temp_int;

  temp_int = gnome_config_get_int_with_default("MISC/WarningsToConsole", &default_value); 
  preferences->warnings_to_console = default_value ? AMITK_PREFERENCES_DEFAULT_WARNINGS_TO_CONSOLE : temp_int;

  temp_int = !gnome_config_get_int_with_default("MISC/DontPromptForSaveOnExit", &default_value); 
  preferences->prompt_for_save_on_exit = default_value ? AMITK_PREFERENCES_DEFAULT_PROMPT_FOR_SAVE_ON_EXIT : temp_int;

  temp_int = gnome_config_get_int_with_default("MISC/SaveXifAsDirectory", &default_value); 
  preferences->save_xif_as_directory = default_value ? AMITK_PREFERENCES_DEFAULT_SAVE_XIF_AS_DIRECTORY : temp_int;

  for (i_modality=0; i_modality<AMITK_MODALITY_NUM; i_modality++) {
    temp_string = g_strdup_printf("DATASETS/DefaultColorTable%s", amitk_modality_get_name(i_modality));
    temp_int = gnome_config_get_int_with_default(temp_string, &default_value);
    g_free(temp_string);
    preferences->default_color_table[i_modality] = 
      default_value ? amitk_modality_default_color_table[i_modality] : temp_int;
  }

  for (i_window = 0; i_window < AMITK_WINDOW_NUM; i_window++) 
    for (i_limit = 0; i_limit < AMITK_LIMIT_NUM; i_limit++) {
      temp_string = g_strdup_printf("WINDOWS/%s-%s", amitk_window_get_name(i_window),
				    amitk_limit_get_name(i_limit));
      temp_float = gnome_config_get_float_with_default(temp_string,&default_value);
      g_free(temp_string);
      preferences->default_window[i_window][i_limit] =
	default_value ? amitk_window_default[i_window][i_limit] : temp_float;
  }

  gnome_config_pop_prefix();

#else /* AMIDE_WIN32_HACKS */

  /* canvas preferences */
  preferences->canvas_roi_width = AMITK_PREFERENCES_DEFAULT_CANVAS_ROI_WIDTH;
  preferences->canvas_line_style = AMITK_PREFERENCES_DEFAULT_CANVAS_LINE_STYLE;
  preferences->canvas_fill_isocontour = AMITK_PREFERENCES_DEFAULT_CANVAS_FILL_ISOCONTOUR;
  preferences->canvas_layout = AMITK_PREFERENCES_DEFAULT_CANVAS_LAYOUT;
  preferences->canvas_maintain_size = AMITK_PREFERENCES_DEFAULT_CANVAS_LAYOUT;
  preferences->canvas_target_empty_area = AMITK_PREFERENCES_DEFAULT_CANVAS_TARGET_EMPTY_AREA; 

  /* debug preferences */
  preferences->warnings_to_console = AMITK_PREFERENCES_DEFAULT_WARNINGS_TO_CONSOLE;

  /* file preferences */
  preferences->prompt_for_save_on_exit = AMITK_PREFERENCES_DEFAULT_PROMPT_FOR_SAVE_ON_EXIT;
  preferences->save_xif_as_directory = AMITK_PREFERENCES_DEFAULT_SAVE_XIF_AS_DIRECTORY;

  /* data set preferences */
  for (i_modality=0; i_modality<AMITK_MODALITY_NUM; i_modality++) {
    preferences->default_color_table[i_modality] = amitk_modality_default_color_table[i_modality];
  }
  for (i_window = 0; i_window < AMITK_WINDOW_NUM; i_window++) 
    for (i_limit = 0; i_limit < AMITK_LIMIT_NUM; i_limit++) 
      preferences->default_window[i_window][i_limit] = amitk_window_default[i_window][i_limit];

#endif /* AMIDE_WIN32_HACKS */


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
#ifndef AMIDE_WIN32_HACKS
    gnome_config_push_prefix("/"PACKAGE"/");
    gnome_config_set_int("ROI/Width", roi_width);
    gnome_config_pop_prefix();
    gnome_config_sync();
#endif
    g_signal_emit(G_OBJECT(preferences), preferences_signals[STUDY_PREFERENCES_CHANGED], 0);
  }


  return;
}

void amitk_preferences_set_canvas_line_style(AmitkPreferences * preferences, GdkLineStyle line_style) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));

  if (AMITK_PREFERENCES_CANVAS_LINE_STYLE(preferences) != line_style) {
    preferences->canvas_line_style = line_style;
#ifndef AMIDE_WIN32_HACKS
    gnome_config_push_prefix("/"PACKAGE"/");
    gnome_config_set_int("ROI/LineStyle",line_style);
    gnome_config_pop_prefix();
    gnome_config_sync();
#endif
    g_signal_emit(G_OBJECT(preferences), preferences_signals[STUDY_PREFERENCES_CHANGED], 0);
  }

  return;
}

void amitk_preferences_set_canvas_fill_isocontour(AmitkPreferences * preferences, gboolean fill_isocontour) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));

  if (AMITK_PREFERENCES_CANVAS_FILL_ISOCONTOUR(preferences) != fill_isocontour) {
    preferences->canvas_fill_isocontour = fill_isocontour;
#ifndef AMIDE_WIN32_HACKS
    gnome_config_push_prefix("/"PACKAGE"/");
    gnome_config_set_int("ROI/FillIsocontour",fill_isocontour);
    gnome_config_pop_prefix();
    gnome_config_sync();
#endif
    g_signal_emit(G_OBJECT(preferences), preferences_signals[STUDY_PREFERENCES_CHANGED], 0);
  }

  return;
}

void amitk_preferences_set_canvas_layout(AmitkPreferences * preferences, 
					 AmitkLayout layout) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));

  if (AMITK_PREFERENCES_CANVAS_LAYOUT(preferences) != layout) {
    preferences->canvas_layout = layout;
#ifndef AMIDE_WIN32_HACKS
    gnome_config_push_prefix("/"PACKAGE"/");
    gnome_config_set_int("CANVAS/Layout", layout);
    gnome_config_pop_prefix();
    gnome_config_sync();
#endif
    g_signal_emit(G_OBJECT(preferences), preferences_signals[STUDY_PREFERENCES_CHANGED], 0);
  }

  return;
}

void amitk_preferences_set_canvas_maintain_size(AmitkPreferences * preferences, 
						gboolean maintain_size) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));

  if (AMITK_PREFERENCES_CANVAS_MAINTAIN_SIZE(preferences) != maintain_size) {
    preferences->canvas_maintain_size = maintain_size;
#ifndef AMIDE_WIN32_HACKS
    gnome_config_push_prefix("/"PACKAGE"/");
    gnome_config_set_int("CANVAS/MinimizeSize",!maintain_size);
    gnome_config_pop_prefix();
    gnome_config_sync();
#endif
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
#ifndef AMIDE_WIN32_HACKS
    gnome_config_push_prefix("/"PACKAGE"/");
    gnome_config_set_int("CANVAS/TargetEmptyArea", target_empty_area);
    gnome_config_pop_prefix();
    gnome_config_sync();
#endif
    g_signal_emit(G_OBJECT(preferences), preferences_signals[STUDY_PREFERENCES_CHANGED], 0);
  }

  return;
}




void amitk_preferences_set_warnings_to_console(AmitkPreferences * preferences, gboolean new_value) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));

  if (AMITK_PREFERENCES_WARNINGS_TO_CONSOLE(preferences) != new_value) {
    preferences->warnings_to_console = new_value;
#ifndef AMIDE_WIN32_HACKS
    gnome_config_push_prefix("/"PACKAGE"/");
    gnome_config_set_int("MISC/WarningsToConsole",new_value);
    gnome_config_pop_prefix();
    gnome_config_sync();
#endif
    g_signal_emit(G_OBJECT(preferences), preferences_signals[MISC_PREFERENCES_CHANGED], 0);
  }
  return;
}

void amitk_preferences_set_prompt_for_save_on_exit(AmitkPreferences * preferences, gboolean new_value) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));

  if (AMITK_PREFERENCES_PROMPT_FOR_SAVE_ON_EXIT(preferences) != new_value) {
    preferences->prompt_for_save_on_exit = new_value;
#ifndef AMIDE_WIN32_HACKS
    gnome_config_push_prefix("/"PACKAGE"/");
    gnome_config_set_int("MISC/DontPromptForSaveOnExit",!new_value);
    gnome_config_pop_prefix();
    gnome_config_sync();
#endif
    g_signal_emit(G_OBJECT(preferences), preferences_signals[MISC_PREFERENCES_CHANGED], 0);
  }
  return;
}

void amitk_preferences_set_xif_as_directory(AmitkPreferences * preferences, gboolean new_value) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));

  if (AMITK_PREFERENCES_SAVE_XIF_AS_DIRECTORY(preferences) != new_value) {
    preferences->save_xif_as_directory = new_value;
#ifndef AMIDE_WIN32_HACKS
    gnome_config_push_prefix("/"PACKAGE"/");
    gnome_config_set_int("MISC/SaveXifAsDirectory", new_value);
    gnome_config_pop_prefix();
    gnome_config_sync();
#endif
    g_signal_emit(G_OBJECT(preferences), preferences_signals[MISC_PREFERENCES_CHANGED], 0);
  }
  return;
}

void amitk_preferences_set_color_table(AmitkPreferences * preferences,
				       AmitkModality modality,
				       AmitkColorTable color_table) {
#ifndef AMIDE_WIN32_HACKS
  gchar * temp_string;
#endif

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));
  g_return_if_fail((modality >= 0) && (modality < AMITK_MODALITY_NUM));
  g_return_if_fail((color_table >= 0) && (color_table < AMITK_COLOR_TABLE_NUM));

  if (AMITK_PREFERENCES_DEFAULT_COLOR_TABLE(preferences,modality) != color_table) {
    preferences->default_color_table[modality] = color_table;

#ifndef AMIDE_WIN32_HACKS
    temp_string = g_strdup_printf("DATASETS/DefaultColorTable%s", 
				  amitk_modality_get_name(modality));
    gnome_config_push_prefix("/"PACKAGE"/");
    gnome_config_set_int(temp_string, color_table);
    gnome_config_pop_prefix();
    g_free(temp_string);
    gnome_config_sync();
#endif
    g_signal_emit(G_OBJECT(preferences), preferences_signals[DATA_SET_PREFERENCES_CHANGED], 0);
  }

  return;
}

void amitk_preferences_set_default_window(AmitkPreferences * preferences,
					  const AmitkWindow window,
					  const AmitkLimit limit,
					  const amide_data_t value) {

#ifndef AMIDE_WIN32_HACKS
  gchar * temp_string;
#endif

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));
  g_return_if_fail((window >= 0) && (window < AMITK_WINDOW_NUM));
  g_return_if_fail((limit >= 0) && (limit < AMITK_LIMIT_NUM));

  if (!REAL_EQUAL(AMITK_PREFERENCES_DEFAULT_WINDOW(preferences, window, limit), value)) {
    preferences->default_window[window][limit] = value;

#ifndef AMIDE_WIN32_HACKS
    temp_string = g_strdup_printf("THRESHOLDS/%s-%s", amitk_window_get_name(window),
				  amitk_limit_get_name(limit));
    gnome_config_push_prefix("/"PACKAGE"/");
    gnome_config_set_float(temp_string,value);
    g_free(temp_string);
    gnome_config_pop_prefix();
    gnome_config_sync();
#endif
    g_signal_emit(G_OBJECT(preferences), preferences_signals[DATA_SET_PREFERENCES_CHANGED], 0);
  }

  return;
}

void amitk_preferences_set_dialog(AmitkPreferences * preferences,
				  GtkWidget * dialog) {

  g_return_if_fail(AMITK_IS_PREFERENCES(preferences));
  preferences->dialog = dialog;

}
