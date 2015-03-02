/* ui_rendering_movie_dialog_callbacks.c
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

#ifdef AMIDE_LIBVOLPACK_SUPPORT
#ifdef AMIDE_MPEG_ENCODE_SUPPORT


#include <gnome.h>
#include <math.h>
#include <sys/stat.h>
#include "amide.h"
#include "rendering.h"
#include "study.h"
#include "image.h"
#include "ui_rendering.h"
#include "ui_rendering_movie_dialog.h"
#include "ui_rendering_movie_dialog_callbacks.h"
#include "ui_study_callbacks.h"

/* function to handle picking an output mpeg file name */
static void ui_rendering_movie_dialog_callbacks_save_as_ok(GtkWidget* widget, gpointer data) {

  GtkWidget * file_selection = data;
  GtkWidget * question;
  ui_rendering_movie_t * ui_rendering_movie;
  gchar * save_filename;
  gchar * temp_string;
  struct stat file_info;

  /* get a pointer to ui_rendering_movie */
  ui_rendering_movie = gtk_object_get_data(GTK_OBJECT(file_selection), "ui_rendering_movie");
  g_print("pointer %p\n", ui_rendering_movie);

  g_print("1 num_frames %d\n",ui_rendering_movie->num_frames);

  /* get the filename */
  save_filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_selection));
  

  /* some sanity checks */
  if ((strcmp(save_filename, ".") == 0) ||
      (strcmp(save_filename, "..") == 0) ||
      (strcmp(save_filename, "") == 0) ||
      (strcmp(save_filename, "/") == 0)) {
    g_warning("%s: Inappropriate filename: %s",PACKAGE, save_filename);
    return;
  }

  g_print("2 num_frames %d\n",ui_rendering_movie->num_frames);

  /* see if the filename already exists, delete if needed */
  if (stat(save_filename, &file_info) == 0) {
    /* check if it's okay to writeover the file */
    temp_string = g_strdup_printf("Overwrite file: %s", save_filename);
    question = gnome_question_dialog_modal_parented(temp_string, NULL, NULL, 
						    GTK_WINDOW(ui_rendering_movie->ui_rendering->app));
    g_free(temp_string);

    /* and wait for the question to return */
    if (gnome_dialog_run_and_close(GNOME_DIALOG(question)) == 1)
      return; /* we don't want to overwrite the file.... */

    /* unlinking the file will occur immediately before writing the new one 
       (i.e. not here ) */
  }

  g_print("3 num_frames %d\n",ui_rendering_movie->num_frames);

  /* allright, generate our movie */
  ui_rendering_movie_dialog_perform(ui_rendering_movie, save_filename);

  /* close the file selection box */
  ui_study_callbacks_file_selection_cancel(widget, file_selection);

  return;
}



/* function to change the number of frames in the movie */
void ui_rendering_movie_dialog_callbacks_change_frames(GtkWidget * widget, gpointer data) {

  ui_rendering_movie_t * ui_rendering_movie = data;
  gchar * str;
  gint error;
  gint temp_val;

  /* get the contents of the name entry box */
  str = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert to a decimal */
  error = sscanf(str, "%d", &temp_val);
  g_free(str);

  if (error == EOF)  /* make sure it's a valid number */
    return;
  if (temp_val < 0)
    return;

  /* set the frames */
  ui_rendering_movie->num_frames = temp_val;

  /* tell the dialog we've changed */
  gnome_property_box_changed(GNOME_PROPERTY_BOX(ui_rendering_movie->dialog));

  return;
}

/* function to change the number of rotations on an axis */
void ui_rendering_movie_dialog_callbacks_change_rotation(GtkWidget * widget, gpointer data) {

  ui_rendering_movie_t * ui_rendering_movie = data;
  gchar * str;
  gint error;
  gdouble temp_val;
  axis_t i_axis;

  /* get the contents of the name entry box */
  str = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert to a decimal */
  error = sscanf(str, "%lf", &temp_val);
  g_free(str);

  if (error == EOF)  /* make sure it's a valid number */
    return;

  /* figure out which axis this is for */
  i_axis = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "which_entry"));

  /* set the rotation */
  ui_rendering_movie->rotation[i_axis] = temp_val;

  /* tell the dialog we've changed */
  gnome_property_box_changed(GNOME_PROPERTY_BOX(ui_rendering_movie->dialog));

  return;
}


/* function called when we hit the apply button */
void ui_rendering_movie_dialog_callbacks_apply(GtkWidget* widget, gint page_number, gpointer data) {
  
  ui_rendering_movie_t * ui_rendering_movie = data;
  GtkWidget * file_selection;
  gchar * temp_string;
  
  /* we'll apply all page changes at once */
  if (page_number != -1)
    return;

  //  g_print("-1 num_frames %d\n",ui_rendering_movie->num_frames);

  /* the rest of this function runs the file selection dialog box */
  file_selection = gtk_file_selection_new(_("Output MPEG As"));

  /* take a guess at the filename */
  temp_string = 
    g_strdup_printf("%s.mpg", 
		    ui_rendering_movie->ui_rendering->contexts->rendering_context->volume->name);
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selection), temp_string);
  g_free(temp_string); 

  /* don't want anything else going on till this window is gone */
  gtk_window_set_modal(GTK_WINDOW(file_selection), TRUE);

  /* save a pointer to the ui_rendering_movie data, so we can use it in the callbacks */
  gtk_object_set_data(GTK_OBJECT(file_selection), "ui_rendering_movie", ui_rendering_movie);
  g_print("pointer %p\n", ui_rendering_movie);

  //  g_print("0 num_frames %d\n",ui_rendering_movie->num_frames);

  /* connect the signals */
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(file_selection)->ok_button),
  		     "clicked",
  		     GTK_SIGNAL_FUNC(ui_rendering_movie_dialog_callbacks_save_as_ok),
  		     file_selection);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(file_selection)->cancel_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(ui_study_callbacks_file_selection_cancel),
		     file_selection);
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(file_selection)->cancel_button),
		     "delete_event",
		     GTK_SIGNAL_FUNC(ui_study_callbacks_file_selection_cancel),
		     file_selection);

  /* set the position of the dialog */
  gtk_window_set_position(GTK_WINDOW(file_selection), GTK_WIN_POS_MOUSE);

  /* run the dialog */
  //gtk_widget_show(file_selection);

  ui_rendering_movie_dialog_perform(ui_rendering_movie, "out.mpg");

  return;
}

/* callback for the help button */
void ui_rendering_movie_dialog_callbacks_help(GnomePropertyBox *rendering_dialog, gint page_number, gpointer data) {

  GnomeHelpMenuEntry help_ref={PACKAGE,"rendering-dialog-help.html#RENDERING-MOVIE-DIALOG-HELP"};
  GnomeHelpMenuEntry help_ref_0 = {PACKAGE,"rendering-dialog-help.html#RENDERING-MOVIE-DIALOG-HELP-MOVIE"};

  switch (page_number) {
  case 0:
    gnome_help_display (0, &help_ref_0);
    break;
  default:
    gnome_help_display (0, &help_ref);
    break;
  }

  return;
}


/* function called to destroy the rendering parameter dialog */
void ui_rendering_movie_dialog_callbacks_close_event(GtkWidget* widget, gpointer data) {

  ui_rendering_movie_t * ui_rendering_movie = data;
  ui_rendering_t * ui_rendering = ui_rendering_movie->ui_rendering;


  /* destroy the widget */
  gtk_widget_destroy(GTK_WIDGET(ui_rendering_movie->dialog));

  /* trash collection */
  ui_rendering->movie = ui_rendering_movie_free(ui_rendering->movie);

  return;
}


#endif /* AMIDE_MPEG_ENCODE_SUPPORT */
#endif /* LIBVOLPACK_SUPPORT */

