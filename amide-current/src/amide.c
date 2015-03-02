/* amide.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2003 Andy Loening
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
#include <locale.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include "amide.h"
#include "amitk_study.h"
#include "ui_study.h"

/* external variables */
gchar * view_names[] = {
  "transverse", 
  "coronal", 
  "sagittal"
};

gchar * object_menu_names[] = {
  "_Study",
  "Selected _Data Sets",
  "Selected _ROIs",
  "Selected _Alignment Points"
};

PangoFontDescription * amitk_fixed_font_desc;

/* internal variables */
static GList * windows = NULL;


gboolean amide_is_xif_directory(const gchar * filename, gboolean * plegacy1, gchar ** pxml_filename) {

  struct stat file_info;
  gchar * temp_str;
  DIR * directory;
  struct dirent * directory_entry;
  gchar *xifname;
  gint length;

  /* remove any trailing directory characters */
  length = strlen(filename);
  if ((length >= 1) && (strcmp(filename+length-1, G_DIR_SEPARATOR_S) == 0))
    length--;
  xifname = g_strndup(filename, length);


  if (stat(xifname, &file_info) != 0) 
    return FALSE; /* file doesn't exist */

  if (!S_ISDIR(file_info.st_mode)) 
    return FALSE;


  /* check for legacy .xif file (< 2.0 version) */
  temp_str = g_strdup_printf("%s%sStudy.xml", xifname, G_DIR_SEPARATOR_S);
  if (stat(temp_str, &file_info) == 0) {
    if (plegacy1 != NULL)  *plegacy1 = TRUE;
    if (pxml_filename != NULL) *pxml_filename = temp_str;
    else g_free(temp_str);

    return TRUE;
  }
  g_free(temp_str);

  /* figure out the name of the study file */
  directory = opendir(xifname);
      
  /* currently, only looks at the first study_*.xml file... there should be only one anyway */
  while (((directory_entry = readdir(directory)) != NULL))
    if (g_pattern_match_simple("study_*.xml", directory_entry->d_name)) {
      if (plegacy1 != NULL) *plegacy1 = FALSE;
      if (pxml_filename != NULL) *pxml_filename = g_strdup(directory_entry->d_name);
      closedir(directory);

      return TRUE;
    }
    
  closedir(directory);

  return FALSE;

}

gboolean amide_is_xif_flat_file(const gchar * filename, guint64 * plocation_le, guint64 *psize_le) {

  struct stat file_info;
  FILE * study_file;
  guint64 location_le, size_le;
  gchar magic[64];

  if (stat(filename, &file_info) != 0)
    return FALSE; /* file doesn't exist */

  if (S_ISDIR(file_info.st_mode)) return FALSE;

  if ((study_file = fopen(filename, "r")) == NULL) 
    return FALSE;

  /* check magic string */
  fread(magic, sizeof(gchar), 64, study_file);
  if (strncmp(magic, AMIDE_FLAT_FILE_MAGIC_STRING, strlen(AMIDE_FLAT_FILE_MAGIC_STRING)) != 0) {
    fclose(study_file);
    return FALSE;
  }

  /* get area of file to read for initial XML data */
  fread(&location_le, sizeof(guint64), 1, study_file);
  fread(&size_le, sizeof(guint64), 1, study_file);
  if (plocation_le != NULL)  *plocation_le = location_le;
  if (psize_le != NULL) *psize_le = size_le;

  fclose(study_file);

  return TRUE;
}



void amide_log_handler(const gchar *log_domain,
		       GLogLevelFlags log_level,
		       const gchar *message,
		       gpointer user_data) {

  gchar * temp_string;
#ifndef AMIDE_DEBUG
  GtkWidget * dialog;
#endif


  temp_string = g_strdup_printf("AMIDE WARNING: %s\n", message);
#ifdef AMIDE_DEBUG
  g_print(temp_string);
#else
  dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
				  GTK_MESSAGE_WARNING,
				  GTK_BUTTONS_OK,
				  temp_string);
#endif
  g_free(temp_string);

#ifndef AMIDE_DEBUG
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
#endif
  
  return;
}



/* little utility function, appends str to pstr,
   handles case of pstr pointing to NULL */
void amitk_append_str(gchar ** pstr, const gchar * format, ...) {

  va_list args;
  gchar * temp_str;
  gchar * error_str;

  if (pstr == NULL) return;

  va_start (args, format);
  error_str = g_strdup_vprintf(format, args);
  va_end (args);

  if (*pstr != NULL) {
    temp_str = g_strdup_printf("%s\n%s", *pstr, error_str);
    g_free(*pstr);
    *pstr = temp_str;
  } else {
    *pstr = g_strdup(error_str);
  }
  g_free(error_str);
}


void font_init(void) {
#ifdef AMIDE_WIN32_HACKS
  amitk_fixed_font_desc = pango_font_description_from_string("Sans 10");
#else
  amitk_fixed_font_desc = pango_font_description_from_string("-*-helvetica-medium-r-normal-*-*-120-*-*-*-*-*-*");
#endif

  return;
}


int main (int argc, char *argv []) {

  gint studies_launched=0;
#ifndef AMIDE_WIN32_HACKS
  struct stat file_info;
  AmitkStudy * study = NULL;
  AmitkStudy * imported_study = NULL;
  const gchar ** input_filenames;
  guint i=0;
  AmitkDataSet * new_ds;
  amide_real_t min_voxel_size;
  poptContext amide_ctx;
  GnomeProgram * program;
#endif

  /* setup i18n */
  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);


#ifndef AMIDE_WIN32_HACKS
  program = gnome_program_init(PACKAGE, VERSION, LIBGNOMEUI_MODULE, argc, argv, 
			       GNOME_PROGRAM_STANDARD_PROPERTIES,
#ifdef AMIDE_OSX_HACKS
			       GNOME_PARAM_ENABLE_SOUND, FALSE,
#endif
			       NULL);
  g_object_get (G_OBJECT (program),GNOME_PARAM_POPT_CONTEXT,&amide_ctx,NULL);

#else /* AMIDE_WIN32_HACKS */
  gtk_init(&argc, &argv);
#endif




#ifdef AMIDE_DEBUG
  /* restore the normal segmentation fault signalling so we can get 
     core dumps and don't get the gnome crash dialog */
  signal(SIGSEGV, SIG_DFL);
#endif

  /* specify my own error handler */
  g_log_set_handler (NULL, G_LOG_LEVEL_WARNING, amide_log_handler, NULL);

  /* and tell gnome-ui to pump it's errors similarly */
  g_log_set_handler ("GnomeUI", G_LOG_LEVEL_WARNING, amide_log_handler, NULL);


  /* startup initializations */
  font_init();

#ifndef AMIDE_WIN32_HACKS
  /* figure out if there was anything else on the command line */
  input_filenames = poptGetArgs(amide_ctx);
  /*  input_filenames is just pointers into the amide_ctx structure,
      and shouldn't be freed */
    
  /* if we specified files on the command line, load it in */
  if (input_filenames != NULL) {
    for (i=0; input_filenames[i] != NULL; i++) {

      /* check to see that the filename exists and it's a directory */
      if (stat(input_filenames[i], &file_info) != 0) {
	g_warning(_("%s does not exist"),input_filenames[i]);
      } else if (amide_is_xif_flat_file(input_filenames[i], NULL, NULL) ||
		 amide_is_xif_directory(input_filenames[i], NULL, NULL)) {
	if ((study=amitk_study_load_xml(input_filenames[i])) == NULL)
	  g_warning(_("Failed to load in as XIF file: %s"), input_filenames[i]);
      } else if (!S_ISDIR(file_info.st_mode)) {
	/* not a directory... maybe an import file? */
	if ((new_ds = amitk_data_set_import_file(AMITK_IMPORT_METHOD_GUESS, 0,
						 input_filenames[i], NULL, NULL)) != NULL) {
	  if (imported_study == NULL) {
	    imported_study = amitk_study_new();
	    amitk_object_set_name(AMITK_OBJECT(imported_study), AMITK_OBJECT_NAME(new_ds));
	    amitk_study_set_view_center(imported_study, amitk_volume_get_center(AMITK_VOLUME(new_ds)));
	  }
	  amitk_object_add_child(AMITK_OBJECT(imported_study), AMITK_OBJECT(new_ds));
	  min_voxel_size = amitk_data_sets_get_min_voxel_size(AMITK_OBJECT_CHILDREN(imported_study));
	  amitk_study_set_view_thickness(imported_study, min_voxel_size);
	  new_ds = amitk_object_unref(new_ds);
	} else 
	  g_warning(_("%s is not an AMIDE study or importable file type"), input_filenames[i]);
      } else {
	  g_warning(_("%s is not an AMIDE XIF Directory"), input_filenames[i]);
      }

      if (study != NULL) {
	/* each whole study gets it's own window */
	ui_study_create(study);
	studies_launched++;
	study = amitk_object_unref(study);
      } 

    }
  } 

  if (imported_study != NULL) {
    /* all imported data sets go into one study */
    ui_study_create(imported_study);
    studies_launched++;
    imported_study = amitk_object_unref(imported_study);
  }

  poptFreeContext(amide_ctx);
#endif 

  /* start up an empty study if we haven't loaded in anything */
  if (studies_launched < 1) ui_study_create(NULL);

  /* the main event loop */
  gtk_main();

  return 0;
}


/* keep track of open windows */
void amide_register_window(gpointer * widget) {

  g_return_if_fail(widget != NULL);
  
  windows = g_list_append(windows, widget);

  return;
}


/* keep track of open windows */
void amide_unregister_window(gpointer * widget) {

  g_return_if_fail(widget != NULL);

  windows = g_list_remove(windows, widget);

  if (windows == NULL) gtk_main_quit();

  return;
}


/* this should cleanly exit the program */
void amide_unregister_all_windows(void) {

  gboolean return_val;
  gint number_to_leave=0;

  while (g_list_nth(windows, number_to_leave) != NULL) {
    /* this works, because each delete event should call amide_unregister_window */
    g_signal_emit_by_name(G_OBJECT(windows->data), "delete_event", NULL, &return_val);
    if (return_val == TRUE) number_to_leave++;
  }

  return;
}
