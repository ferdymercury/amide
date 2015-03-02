/* ui_rendering_callbacks.c
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

#include <sys/stat.h>
#include <gnome.h>
#include "amide.h"
#include "rendering.h"
#include "study.h"
#include "image.h"
#include "ui_rendering.h"
#include "ui_rendering_callbacks.h"
#include "ui_rendering_dialog.h"
#include "ui_rendering_movie_dialog.h"
#include "ui_study_callbacks.h"


/* function callback for the "render" button */
void ui_rendering_callbacks_render(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;

  ui_rendering_update_canvases(ui_rendering); /* render now */

  return;
}








/* function to switch into immediate rendering mode */
void ui_rendering_callbacks_immediate(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;

  /* should we be rendering immediately */
  ui_rendering->immediate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  /* set the sensitivity of the render button */
  gtk_widget_set_sensitive(GTK_WIDGET(ui_rendering->render_button), !(ui_rendering->immediate));

  if (ui_rendering->immediate)
    ui_rendering_update_canvases(ui_rendering); /* render now */

  return;

}






/* function called when one of the rotate widgets has been hit */
void ui_rendering_callbacks_rotate(GtkAdjustment * adjustment, gpointer data) {

  ui_rendering_t * ui_rendering = data;
  axis_t i_axis;
  floatpoint_t rotation;

  /* figure out which rotate widget called me */
  i_axis = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(adjustment),"axis"));
  rotation = (adjustment->value/180)*M_PI; /* get rotation in radians */

  /* update the rotation values */
  rendering_context_set_rotation(ui_rendering->axis_context, i_axis, rotation);
  rendering_list_set_rotation(ui_rendering->contexts, i_axis, rotation);

  /* render now if appropriate*/
  if (ui_rendering->immediate)
    ui_rendering_update_canvases(ui_rendering); 

  /* return adjustment back to normal */
  adjustment->value = 0.0;
  gtk_adjustment_changed(adjustment);

  return;
}

/* function called to snap the axis back to the default */
void ui_rendering_callbacks_reset_axis_pressed(GtkWidget * widget, gpointer data) {
  ui_rendering_t * ui_rendering = data;

  /* reset the rotations */
  rendering_context_reset_rotation(ui_rendering->axis_context);
  rendering_list_reset_rotation(ui_rendering->contexts);

  /* render now if appropriate*/
  if (ui_rendering->immediate)
    ui_rendering_update_canvases(ui_rendering); 

  return;
}


/* function to handle exporting a view */
static void ui_rendering_callbacks_export_ok(GtkWidget* widget, gpointer data) {

  GtkWidget * file_selection = data;
  GtkWidget * question;
  ui_rendering_t * ui_rendering;
  gchar * save_filename;
  gchar * temp_string;
  struct stat file_info;
  GdkImlibImage * export_image;

  /* get a pointer to ui_rendering */
  ui_rendering = gtk_object_get_data(GTK_OBJECT(file_selection), "ui_rendering");

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

  /* see if the filename already exists */
  if (stat(save_filename, &file_info) == 0) {
    /* check if it's okay to writeover the file */
    temp_string = g_strdup_printf("Overwrite file: %s", save_filename);
    if (GNOME_IS_APP(ui_rendering->app))
      question = gnome_question_dialog_modal_parented(temp_string, NULL, NULL, 
						      GTK_WINDOW(ui_rendering->app));
    else
      question = gnome_question_dialog_modal(temp_string, NULL, NULL);
    g_free(temp_string);

    /* and wait for the question to return */
    if (gnome_dialog_run_and_close(GNOME_DIALOG(question)) == 1)
      return; /* we don't want to overwrite the file.... */
  }


  /* yep, we still need to use imlib for the moment for generating output images,
     maybe gdk_pixbuf will have this functionality at some point */
  export_image = gdk_imlib_create_image_from_data(gdk_pixbuf_get_pixels(ui_rendering->main_image), NULL, 
						  gdk_pixbuf_get_width(ui_rendering->main_image),
						  gdk_pixbuf_get_height(ui_rendering->main_image));
  if (export_image == NULL) {
    g_warning("%s: Failure converting pixbuf to imlib image for exporting image file",PACKAGE);
    return;
  }

  /* allright, export the view */
  if (gdk_imlib_save_image(export_image, save_filename, NULL) == FALSE) {
    g_warning("%s: Failure Saving File: %s",PACKAGE, save_filename);
    gdk_imlib_destroy_image(export_image);
    return;
  }
  gdk_imlib_destroy_image(export_image);

  /* close the file selection box */
  ui_study_callbacks_file_selection_cancel(widget, file_selection);

  return;
}

/* function to save a rendering as an external data format */
void ui_rendering_callbacks_export_pressed(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;
  rendering_list_t * temp_contexts;
  GtkFileSelection * file_selection;
  gchar * temp_string;
  gchar * data_set_names = NULL;
  static guint save_image_num = 0;

  file_selection = GTK_FILE_SELECTION(gtk_file_selection_new(_("Export File")));

  /* take a guess at the filename */
  temp_contexts = ui_rendering->contexts;
  data_set_names = g_strdup(temp_contexts->rendering_context->volume->name);
  temp_contexts = temp_contexts->next;
  while (temp_contexts != NULL) {
    temp_string = g_strdup_printf("%s+%s",data_set_names, temp_contexts->rendering_context->volume->name);
    g_free(data_set_names);
    data_set_names = temp_string;
    temp_contexts = temp_contexts->next;
  }
  temp_string = g_strdup_printf("Rendering_{%s}_%d.jpg", 
				data_set_names,save_image_num);
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selection), temp_string);
  g_free(data_set_names);
  g_free(temp_string); 

  /* save a pointer to ui_rendering */
  gtk_object_set_data(GTK_OBJECT(file_selection), "ui_rendering", ui_rendering);
    
  /* don't want anything else going on till this window is gone */
  //  gtk_window_set_modal(GTK_WINDOW(file_selection), TRUE);
  // for some reason, this modal's the application permanently...

  /* connect the signals */
  gtk_signal_connect(GTK_OBJECT(file_selection->ok_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(ui_rendering_callbacks_export_ok),
		     file_selection);
  gtk_signal_connect(GTK_OBJECT(file_selection->cancel_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(ui_study_callbacks_file_selection_cancel),
		     file_selection);
  gtk_signal_connect(GTK_OBJECT(file_selection->cancel_button),
		     "delete_event",
		     GTK_SIGNAL_FUNC(ui_study_callbacks_file_selection_cancel),
		     file_selection);

  /* set the position of the dialog */
  gtk_window_set_position(GTK_WINDOW(file_selection), GTK_WIN_POS_MOUSE);

  /* run the dialog */
  gtk_widget_show(GTK_WIDGET(file_selection));


  /* increment this number, so each guessed image name is unique */
  save_image_num++;

  return;
}



/* function called when the button to pop up a rendering parameters modification dialog is hit */
void ui_rendering_callbacks_parameters_pressed(GtkWidget * widget, gpointer data) {
  
  ui_rendering_t * ui_rendering = data;

  ui_rendering_dialog_create(ui_rendering);

  return;
}


#ifdef AMIDE_MPEG_ENCODE_SUPPORT
/* function called when the button to pop up a movie generation dialog */
void ui_rendering_callbacks_movie_pressed(GtkWidget * widget, gpointer data) {
  
  ui_rendering_t * ui_rendering = data;

  ui_rendering->movie = ui_rendering_movie_dialog_create(ui_rendering);

  return;
}
#endif

/* function to run for a delete_event */
void ui_rendering_callbacks_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_rendering_t * ui_rendering = data;
#ifdef AMIDE_MPEG_ENCODE_SUPPORT
  ui_rendering_movie_t * ui_rendering_movie = ui_rendering->movie;
#endif

  /* if our parameter modification dialog is up, kill that */
  if (ui_rendering->parameter_dialog  != NULL)
    gtk_signal_emit_by_name(GTK_OBJECT(ui_rendering->parameter_dialog), "delete_event", 
			    NULL, ui_rendering);

#ifdef AMIDE_MPEG_ENCODE_SUPPORT
  /* if the movie dialog is up, kill that */
  if (ui_rendering_movie  != NULL)
    gtk_signal_emit_by_name(GTK_OBJECT(ui_rendering_movie->dialog), "delete_event", 
			    NULL, ui_rendering);
#endif


  /* destroy the widget */
  gtk_widget_destroy(widget);

  /* free the associated data structure */
  ui_rendering = ui_rendering_free(ui_rendering);

  /* quit our app if we've closed all windows */
  number_of_windows--;
  if (number_of_windows == 0)
    gtk_main_quit();

  return;
}

#endif










