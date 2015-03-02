/* amide.c
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

#include "config.h" 
#include <signal.h>
#include <sys/stat.h>
#include <gnome.h>
#include "amide.h"
#include "study.h"
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

gchar * object_edit_menu_explanation[] = {
  "Edit study parameters",
  "Edit parameters for selected data sets",
  "Edit parameters for selected ROIs",
  "Edit parameters for selected alignment points"
};

gchar * object_delete_menu_explanation[] = {
  "BUG",
  "Delete selected data sets",
  "Delete selected ROIs",
  "Delete selected alignment points"
};

gchar * threshold_type_names[] = {
  "per slice", 
  "per frame", 
  "interpolated between frames",
  "global"
};

gchar * threshold_type_explanations[] = {
  "threshold the images based on the max and min values in the current slice",
  "threshold the images based on the max and min values in the current frame",
  "threshold the images based on max and min values interpolated from the reference frame thresholds",
  "threshold the images based on the max and min values of the entire data set",
};

gchar * view_mode_names[] = {
  "single view",
  "linked view"
};

gchar * view_mode_explanations[] = {
  "All objects are shown in a single view",
  "Objects are shown in 1 of 2 linked views"
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





int main (int argc, char *argv []) {

  struct stat file_info;
  study_t * study=NULL;
  const gchar ** input_filenames;
  poptContext amide_ctx;
  guint i=0;
  gint studies_launched=0;
  volume_t * new_volume;
  gboolean loaded;

  struct poptOption amide_popt_options[] = {
    {"input_study",
     'i',
     POPT_ARG_NONE,
     NULL,
     0,
     N_("Depreciated"),
     N_("DEPRECIATED")
    },
    {NULL,
     '\0',
     0,
     NULL,
     0,
     NULL,
     NULL
    }
  };

  /* uncomment these whenever we get around to using i18n */
  //  bindtextdomain(PACKAGE, GNOMELOCALEDIR);
  //  textdomain(PACKAGE);

  gnome_init_with_popt_table(PACKAGE, VERSION, argc, argv,
			     amide_popt_options, 0, &amide_ctx);

#ifdef AMIDE_DEBUG
  /* restore the normal segmentation fault signalling so we can get 
     core dumps and don't get the gnome crash dialog */
  signal(SIGSEGV, SIG_DFL);
#endif

  /* specify my own error handler */
  g_log_set_handler (NULL, G_LOG_LEVEL_WARNING, amide_log_handler, NULL);



  gdk_rgb_init(); /* needed for the rgb graphics usage stuff */

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
	if ((new_volume = volume_import_file(AMIDE_GUESS, 0,input_filenames[i], NULL)) != NULL) {
	  study = study_init();
	  study->coord_frame = rs_init();
	  study_add_volume(study, new_volume);
	  study_set_name(study, new_volume->name); /* first guess at a name */
	  new_volume = volume_unref(new_volume); /* remove a reference */
	  loaded = TRUE;
	} else
	  g_warning("%s is not an AMIDE study or importable file type ", input_filenames[i]);
      } else if ((study=study_load_xml(input_filenames[i])) == NULL)
	/* try loading the study into memory */
	g_warning("error loading study: %s",input_filenames[i]);
      else 
	loaded = TRUE;

      if (loaded) {
	/* setup the study window */
	ui_study_create(study, NULL);
	studies_launched++;
      } else {
	study = study_unref(study);
      }

    }
  } 

  /* start up an empty study if we haven't loaded in anything */
  if (studies_launched < 1) ui_study_create(NULL,NULL);

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
    gtk_signal_emit_by_name(GTK_OBJECT(windows->data), "delete_event", NULL, &return_val);
    if (return_val == TRUE) number_to_leave++;
  }

  return;
}
