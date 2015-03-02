/* amide.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001 Andy Loening
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
gchar * view_names[] = {"transverse", "coronal", "sagittal"};

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

/* internal variables */
static GList * windows = NULL;

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

  /* restore the normal segmentation fault signalling so we can get 
     core dumps and don't get the gnome crash dialog */
  signal(SIGSEGV, SIG_DFL);

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
	g_warning("%s: AMIDE study %s does not exist",PACKAGE, input_filenames[i]);
      else if (!S_ISDIR(file_info.st_mode)) {
	/* not a directory... maybe an import file? */
	if ((new_volume = volume_import_file(AMIDE_GUESS, 0,input_filenames[i], NULL)) != NULL) {
	  study = study_init();
	  study_add_volume(study, new_volume);
	  study_set_name(study, new_volume->name); /* first guess at a name */
	  new_volume = volume_free(new_volume); /* remove a reference */
	  loaded = TRUE;
	} else
	  g_warning("%s: %s is not an AMIDE study or importable file type ", PACKAGE, input_filenames[i]);
      } else if ((study=study_load_xml(input_filenames[i])) == NULL)
	/* try loading the study into memory */
	g_warning("%s: error loading study: %s",PACKAGE, input_filenames[i]);
      else 
	loaded = TRUE;

      if (loaded) {
	/* setup the study window */
	ui_study_create(study, NULL);
	studies_launched++;
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

  while (windows != NULL) {
    /* this works, because each delete event should call amide_unregister_window */
    gtk_signal_emit_by_name(GTK_OBJECT(windows->data), "delete_event");
  }

  return;
}
