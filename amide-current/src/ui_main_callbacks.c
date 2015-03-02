/* ui_main_callbacks.c
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
#include <gnome.h>
#include <sys/stat.h>
#include "amide.h"
#include "volume.h"
#include "roi.h"
#include "study.h"
#include "rendering.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_roi.h"
#include "ui_volume.h"
#include "ui_study.h"
#include "ui_main_callbacks.h"



/* function to close the "load file" widget */
static void ui_main_callbacks_open_cancel(GtkWidget* widget, gpointer data) {

  GtkWidget * file_selection = data;

  gtk_grab_remove(file_selection);
  gtk_widget_destroy(file_selection);

  return;
}


/* function to handle loading in an AMIDE study */
static void ui_main_callbacks_open_ok(GtkWidget* widget, gpointer data) {

  GtkWidget * file_selection = data;
  gchar * open_filename;
  struct stat file_info;
  study_t * study;

  /* get the filename */
  open_filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_selection));

  /* check to see that the filename exists and it's a directory */
  if (stat(open_filename, &file_info) != 0) {
    g_warning("%s: AMIDE study %s does not exist",PACKAGE, open_filename);
    return;
  }
  if (!S_ISDIR(file_info.st_mode)) {
    g_warning("%s: %s is not a directory/AMIDE study",PACKAGE, open_filename);
    return;
  }

  /* try loading the study into memory */
  if ((study=study_load_xml(open_filename)) == NULL) {
    g_warning("%s: error loading study: %s",PACKAGE, open_filename);
    return;
  }

  /* close the file selection box */
  ui_main_callbacks_open_cancel(widget, file_selection);

  /* and setup the study window */
  ui_study_create(study);

  return;
}


/* function to load a study into a  study widget w*/
void ui_main_callbacks_open_study(GnomeApp* app, gpointer data) {

  GtkWidget * file_selection;

  file_selection = gtk_file_selection_new(_("Open AMIDE File"));

  /* connect the signals */
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(file_selection)->ok_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(ui_main_callbacks_open_ok),
		     file_selection);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(file_selection)->cancel_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(ui_main_callbacks_open_cancel),
		     file_selection);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(file_selection)->cancel_button),
		     "delete_event",
		     GTK_SIGNAL_FUNC(ui_main_callbacks_open_cancel),
		     file_selection);

  /* set the position of the dialog */
  gtk_window_set_position(GTK_WINDOW(file_selection), GTK_WIN_POS_MOUSE);

  /* run the dialog */
  gtk_widget_show(file_selection);
  gtk_grab_add(file_selection);

  return;
}
  

/* function to create a new study widget */
void ui_main_callbacks_new_study(GnomeApp* app, gpointer data) {

  ui_study_create(NULL);

  return;
}

/* function which brings up an about box */
void ui_main_callbacks_about(GnomeApp* app, gpointer data) {

  GtkWidget *about;

  const gchar *authors[] = {
    "Andy Loening <loening@ucla.edu>",
    NULL
  };

  const gchar *contents_base = \
    _("Amide's a Medical Image Data Examiner\n   \n");

  const gchar * contents_compiled = \
    _("compiled with support for the following libraries:");

#ifdef AMIDE_LIBECAT_SUPPORT
  const gchar *contents_libecat = \
    _("\tlibecat: CTI File library by Merence Sibomona");
#endif

#ifdef AMIDE_LIBMDC_SUPPORT
  const gchar *contents_libmdc = \
    _("\tlibmdc: Medical Imaging File library by Erik Nolf");
#endif

#ifdef AMIDE_LIBVOLPACK_SUPPORT
  const gchar *contents_libvolpack = \
    _("\tlibvolpack: Volume Rendering library by Philippe Lacroute");
#endif


  gchar * contents;



  contents = g_strjoin("\n", 
		       contents_base,
#if (AMIDE_LIBECAT_SUPPORT || AMIDE_LIBMDC_SUPPORT || AMIDE_LIBVOLPACK_SUPPORT)
		       contents_compiled,
#endif
#ifdef AMIDE_LIBECAT_SUPPORT
		       contents_libecat,
#endif
#ifdef AMIDE_LIBMDC_SUPPORT
		       contents_libmdc,
#endif
#ifdef AMIDE_LIBVOLPACK_SUPPORT
		       contents_libvolpack,
#endif
		       NULL);


  about = gnome_about_new(PACKAGE, VERSION, 
			  "(c) 2001 Andy Loening",
			  authors,
			  contents,
			  "amide.png");

  gtk_window_set_modal(GTK_WINDOW(about), FALSE);

  gtk_widget_show(about);

  g_free(contents);

  return;
}

/* function ran when closing the main window */
void ui_main_callbacks_delete_event(GnomeApp* app, gpointer data) {
  
  gtk_widget_destroy(GTK_WIDGET(app));

  gtk_main_quit();

}

