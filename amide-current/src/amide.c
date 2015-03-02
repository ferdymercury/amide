/* amide.c
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

#include "amide_config.h" 
//#include <locale.h>
#include <signal.h>
#include <sys/stat.h>
//#include <dirent.h>
//#include <string.h>

#include "amide.h"
#include "amide_gconf.h"
#include "amide_gnome.h"
//#include "amitk_type_builtins.h"
#include "amitk_common.h"
#include "amitk_study.h"
#include "pixmaps.h"
#include "ui_study.h"
#include "ui_common.h"



/* external variables */
gchar * object_menu_names[] = {
  N_("_Study"),
  N_("Selected _Data Sets"),
  N_("Selected _ROIs"),
  N_("Selected _Alignment Points")
};


void amide_log_handler_nopopup(const gchar *log_domain,
			       GLogLevelFlags log_level,
			       const gchar *message,
			       gpointer user_data) {

  AmitkPreferences * preferences = user_data;

  if (AMITK_PREFERENCES_WARNINGS_TO_CONSOLE(preferences)) 
    g_print("AMIDE WARNING: %s\n", message);

  return;
}

void amide_log_handler(const gchar *log_domain,
		       GLogLevelFlags log_level,
		       const gchar *message,
		       gpointer user_data) {

  AmitkPreferences * preferences = user_data;
  GtkWidget * dialog;
  GtkWidget * message_area;
  GtkWidget * scrolled;
  GtkWidget * label;

  if (AMITK_PREFERENCES_WARNINGS_TO_CONSOLE(preferences)) {
    if (log_level == G_LOG_LEVEL_MESSAGE) 
      g_print("AMIDE MESSAGE: %s\n", message);
    else /* G_LOG_LEVEL_WARNING */
      g_print("AMIDE WARNING: %s\n", message);

  } else {

    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
				    (log_level == G_LOG_LEVEL_MESSAGE) ? GTK_MESSAGE_INFO : GTK_MESSAGE_WARNING,
				    GTK_BUTTONS_OK,
				    "AMIDE %s", 
				    (log_level == G_LOG_LEVEL_MESSAGE) ? _("MESSAGE") : _("WARNING"));

    message_area = gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(dialog));

    scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled, -1, 75);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), 
				   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(message_area), scrolled, TRUE, TRUE, Y_PADDING);

    label = gtk_label_new(message);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), label);

    gtk_widget_show_all(message_area);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
  }

  return;
}



void missing_functionality_warning(AmitkPreferences * preferences) {

  gchar * comments;
  gchar * already_warned;

#if (AMIDE_LIBGSL_SUPPORT && AMIDE_LIBMDC_SUPPORT && AMIDE_LIBDCMDATA_SUPPORT && AMIDE_LIBVOLPACK_SUPPORT && (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT))
  return; /* nothing to complain about */
#endif

  already_warned = amide_gconf_get_string_with_default("MISSING_FUNCTIONALITY", "AlreadyWarned", "0.0.0");

  if (g_strcmp0(already_warned, VERSION) == 0)
    return;
  else {
    comments = g_strconcat(_("This version of AMIDE has been compiled without the following functionality:"), 
			   "\n\n",
#ifndef AMIDE_LIBGSL_SUPPORT
			   _("Line profiles, factor analysis, some filtering, and some alignment (missing libgsl)."),")\n",
#endif
#ifndef AMIDE_LIBMDC_SUPPORT
			   _("Reading of most medical imaging formats except raw and DICOM (missing libmdc)."),"\n",
#endif
#ifndef AMIDE_LIBDCMDATA_SUPPORT
			   _("Reading of most DICOM files (missing libdcmdata/dcmtk)."),"\n",
#endif
#ifndef AMIDE_LIBVOLPACK_SUPPORT
			   _("Volume rendering (missing libvolpack)."),")\n",
#endif
#if !(defined(AMIDE_FFMPEG_SUPPORT) || defined(AMIDE_LIBFAME_SUPPORT))
			   _("Saving MPEG output (missing ffmpeg and missing libfame)."),")\n",
#endif
			   NULL);

    g_warning("%s", comments);
    
    g_free(comments);
    amide_gconf_set_string("MISSING_FUNCTIONALITY", "AlreadyWarned", VERSION);
    return;
  }
}



static  gchar **remaining_args = NULL;

static GOptionEntry command_line_entries[] = {
  //  { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Be verbose", NULL },
  { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &remaining_args, "Special option that collects any remaining arguments for us" },
  { NULL }
};


/********************************************* */
int main (int argc, char *argv []) {

  gint studies_launched=0;
  AmitkPreferences * preferences;
  struct stat file_info;
  AmitkStudy * imported_study = NULL;
  AmitkStudy * study = NULL;
  const gchar * input_filename;
  GList * new_data_sets;
  AmitkDataSet * new_ds;
  amide_real_t min_voxel_size;
  gint i;
  gint num_args;
  gchar * studyname=NULL;
  // GOptionContext *context;


  /* setup i18n */
  //  setlocale(LC_ALL, "");
  //  // setlocale(LC_NUMERIC, "POSIX"); /* don't switch radix sign (it's a period not a comma dammit */
  //  bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
  //  textdomain(GETTEXT_PACKAGE);


#if defined (G_PLATFORM_WIN32)
  /* if setlocale is called on win32, we can't seem to reset the locale back to "C"
     to allow correct reading in of text data */
  gtk_disable_setlocale(); /* prevent gtk_init from calling setlocale, etc. */
#endif
  if (!gtk_init_with_args(&argc, &argv, _("[FILE1] [FILE2] ..."),
			  command_line_entries,
			  NULL, NULL)) {
    return 1;
  }
	
  //  context = g_option_context_new ("- analyize medical images");
  //  g_option_context_add_main_entries (context, command_line_entries, GETTEXT_PACKAGE);
  //  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  //  g_option_context_parse (context, &argc, &argv, NULL);
  amide_gconf_init();
  
  /* translations */
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

#ifdef AMIDE_DEBUG
  /* restore the normal segmentation fault signalling so we can get 
     core dumps and don't get the gnome crash dialog */
  signal(SIGSEGV, SIG_DFL);
#endif

  /* load in the default preferences */
  preferences = amitk_preferences_new();

  /* specify my own error handler */
  g_log_set_handler (NULL, G_LOG_LEVEL_WARNING, amide_log_handler, preferences);

  /* specify my message handler */
  g_log_set_handler (NULL, G_LOG_LEVEL_MESSAGE, amide_log_handler, preferences);

  /* specify the default directory */
  ui_common_set_last_path_used(AMITK_PREFERENCES_DEFAULT_DIRECTORY(preferences));

#ifdef OLD_WIN32_HACKS
  /* ignore gdk warnings on win32 */
  /* as of gtk 2.2.4, get "General case not implemented" warnings from gdkproperty-win32.c
     that appear to be unwarrented */
  g_log_set_handler ("Gdk", G_LOG_LEVEL_WARNING, amide_log_handler_nopopup, preferences);

  /* have those annoying UTF-8 error warnings go to a console, instead of distracting the user */
  g_log_set_handler ("Pango", G_LOG_LEVEL_WARNING, amide_log_handler_nopopup, preferences);
#endif
  
  /* startup initializations */
  amitk_common_font_init();
  pixmaps_initialize_icons();

  /* complain about important missing functionality if appropriate */
  missing_functionality_warning(preferences);

  /* if we specified files on the command line, load them in */
  if (remaining_args != NULL) {
    num_args = g_strv_length (remaining_args);

    for (i = 0; i < num_args; ++i) {
      /*  input_filename is just pointers into the amide_ctx structure, and shouldn't be freed */
      input_filename = remaining_args[i];
    
      /* check to see that the filename exists and it's a directory */
      if (stat(input_filename, &file_info) != 0) {
	g_warning(_("%s does not exist"),input_filename);
      } else if (amitk_is_xif_flat_file(input_filename, NULL, NULL) ||
		 amitk_is_xif_directory(input_filename, NULL, NULL)) {
	if ((study=amitk_study_load_xml(input_filename)) == NULL)
	  g_warning(_("Failed to load in as XIF file: %s"), input_filename);
      } else if (!S_ISDIR(file_info.st_mode)) {
	/* not a directory... maybe an import file? */
	if ((new_data_sets = amitk_data_set_import_file(AMITK_IMPORT_METHOD_GUESS, 0, input_filename, 
							&studyname, preferences, NULL, NULL)) != NULL) {

	  while (new_data_sets != NULL) {
	    new_ds = new_data_sets->data;
	    if (imported_study == NULL) {
	      imported_study = amitk_study_new(preferences);

	      if (studyname != NULL) { 
		amitk_study_suggest_name(imported_study, studyname);
		g_free(studyname);
		studyname = NULL;
	      } else if (AMITK_DATA_SET_SUBJECT_NAME(new_ds) != NULL)
		amitk_study_suggest_name(imported_study, AMITK_DATA_SET_SUBJECT_NAME(new_ds));
	      else
		amitk_study_suggest_name(imported_study, AMITK_OBJECT_NAME(new_ds)); 

	    }


	    amitk_object_add_child(AMITK_OBJECT(imported_study), AMITK_OBJECT(new_ds));
	    min_voxel_size = amitk_data_sets_get_min_voxel_size(AMITK_OBJECT_CHILDREN(imported_study));
	    amitk_study_set_view_thickness(imported_study, min_voxel_size);
	    new_data_sets = g_list_remove(new_data_sets, new_ds);
	    new_ds = amitk_object_unref(new_ds);
	  }
	} else 
	  g_warning(_("%s is not an AMIDE study or importable file type"), input_filename);
      } else {
	g_warning(_("%s is not an AMIDE XIF Directory"), input_filename);
      }
      
      if (study != NULL) {
	/* each whole study gets it's own window */
	ui_study_create(study, preferences);
	studies_launched++;
	study = amitk_object_unref(study);
      } 
    }

    g_strfreev (remaining_args);
    remaining_args = NULL;
  }
  

  if (imported_study != NULL) {
    /* all imported data sets go into one study */
    ui_study_create(imported_study, preferences);
    studies_launched++;
    imported_study = amitk_object_unref(imported_study);
  }

  /* start up an empty study if we haven't loaded in anything */
  if (studies_launched < 1) 
    ui_study_create(NULL, preferences);

  /* remove left over references */
  g_object_unref(preferences); 

  /* the main event loop */
  gtk_main(); 
  
  /* clean-up */
  amide_gconf_shutdown();

  return 0;
}



