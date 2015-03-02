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
#include <signal.h>
#include <sys/stat.h>
#include <gnome.h>
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

/* internal variables */
static GList * windows = NULL;



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
  dialog = gnome_warning_dialog(temp_string);
#endif
  g_free(temp_string);

#ifndef AMIDE_DEBUG
  gnome_dialog_run_and_close(GNOME_DIALOG(dialog));
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


int main (int argc, char *argv []) {

  struct stat file_info;
  AmitkStudy * study = NULL;
  AmitkStudy * imported_study = NULL;
  const gchar ** input_filenames;
  poptContext amide_ctx;
  guint i=0;
  gint studies_launched=0;
  AmitkDataSet * new_ds;
  gboolean loaded;
  amide_real_t min_voxel_size;
  GnomeProgram * program;

  /* uncomment these whenever we get around to using i18n */
  //  bindtextdomain(PACKAGE, GNOMELOCALEDIR);
  //  textdomain(PACKAGE);

  program = gnome_program_init(PACKAGE, VERSION, LIBGNOMEUI_MODULE, argc, argv, 
			       GNOME_PROGRAM_STANDARD_PROPERTIES, NULL);
  g_object_get (G_OBJECT (program),GNOME_PARAM_POPT_CONTEXT,&amide_ctx,NULL);
#ifdef AMIDE_DEBUG
  /* restore the normal segmentation fault signalling so we can get 
     core dumps and don't get the gnome crash dialog */
  signal(SIGSEGV, SIG_DFL);
#endif

  /* specify my own error handler */
  g_log_set_handler (NULL, G_LOG_LEVEL_WARNING, amide_log_handler, NULL);

  /* figure out if there was anything else on the command line */
  input_filenames = poptGetArgs(amide_ctx);
  /*  input_filenames is just pointers into the amide_ctx structure,
      and shouldn't be freed */
    
  /* if we specified files on the command line, load it in */
  if (input_filenames != NULL) {
    for (i=0; input_filenames[i] != NULL; i++) {
      loaded = FALSE;

      /* check to see that the filename exists and it's a directory */
      if (stat(input_filenames[i], &file_info) != 0) 
	g_warning("AMIDE study %s does not exist",input_filenames[i]);
      else if (!S_ISDIR(file_info.st_mode)) {
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
	  g_object_unref(new_ds);
	  new_ds = NULL;
	} else
	  g_warning("%s is not an AMIDE study or importable file type ", input_filenames[i]);
      } else if ((study=amitk_study_load_xml(input_filenames[i])) == NULL)
	g_warning("error loading study: %s",input_filenames[i]);

      if (study != NULL) {
	/* each whole study gets it's own window */
	ui_study_create(study);
	studies_launched++;
	g_object_unref(study);
	study = NULL;
      } 

    }
  } 

  if (imported_study != NULL) {
    /* all imported data sets go into one study */
    ui_study_create(imported_study);
    studies_launched++;
    g_object_unref(imported_study);
    imported_study =NULL;
  }

  /* start up an empty study if we haven't loaded in anything */
  if (studies_launched < 1) ui_study_create(NULL);

  poptFreeContext(amide_ctx);


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
