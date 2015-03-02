/* ui_roi_analysis_dialog_cb.c
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
#include <math.h>
#include <sys/stat.h>
#include "ui_common_cb.h"
#include "ui_roi_analysis_dialog.h"
#include "ui_roi_analysis_dialog_cb.h"


/* function to handle exporting the roi analyses */
static void ui_roi_analysis_dialog_cb_export_ok(GtkWidget* widget, gpointer data) {

  GtkWidget * file_selection = data;
  GtkWidget * question;
  analysis_roi_t * roi_analyses;
  gchar * save_filename;
  gchar * temp_string;
  struct stat file_info;

  /* get a pointer to the analysis */
  roi_analyses = gtk_object_get_data(GTK_OBJECT(file_selection), "roi_analyses");

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
    question = gnome_question_dialog_modal(temp_string, NULL, NULL);
    g_free(temp_string);

    /* and wait for the question to return */
    if (gnome_dialog_run_and_close(GNOME_DIALOG(question)) == 1)
      return; /* we don't want to overwrite the file.... */
  }


  /* allright, save the data */
  ui_roi_analysis_dialog_export(save_filename, roi_analyses);

  /* close the file selection box */
  ui_common_cb_file_selection_cancel(widget, file_selection);

  return;
}

/* function to save the generated roi statistics */
void ui_roi_analysis_dialog_cb_export(GtkWidget * widget, gpointer data) {
  
  analysis_roi_t * roi_analyses = data;
  analysis_roi_t * temp_analyses = data;
  GtkFileSelection * file_selection;
  gchar * temp_string;
  gchar * analysis_name = NULL;

  /* sanity checks */
  g_return_if_fail(roi_analyses != NULL);

  file_selection = GTK_FILE_SELECTION(gtk_file_selection_new(_("Export Statistics")));

  /* take a guess at the filename */
  analysis_name = g_strdup_printf("%s_analysis:%s",study_name(roi_analyses->study), roi_analyses->roi->name);
  temp_analyses= roi_analyses->next_roi_analysis;
  while (temp_analyses != NULL) {
    temp_string = g_strdup_printf("%s+%s",analysis_name, temp_analyses->roi->name);
    g_free(analysis_name);
    analysis_name = temp_string;
    temp_analyses= temp_analyses->next_roi_analysis;
  }
  temp_string = g_strdup_printf("%s.txt",analysis_name);
  g_free(analysis_name);
  analysis_name = temp_string;
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selection), analysis_name);
  g_free(analysis_name);

  /* save a pointer to the analyses */
  gtk_object_set_data(GTK_OBJECT(file_selection), "roi_analyses", roi_analyses);

  /* don't want anything else going on till this window is gone */
  gtk_window_set_modal(GTK_WINDOW(file_selection), TRUE);

  /* connect the signals */
  gtk_signal_connect(GTK_OBJECT(file_selection->ok_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(ui_roi_analysis_dialog_cb_export_ok),
		     file_selection);
  gtk_signal_connect(GTK_OBJECT(file_selection->cancel_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(ui_common_cb_file_selection_cancel),
		     file_selection);
  gtk_signal_connect(GTK_OBJECT(file_selection->cancel_button),
		     "delete_event",
		     GTK_SIGNAL_FUNC(ui_common_cb_file_selection_cancel),
		     file_selection);

  /* set the position of the dialog */
  gtk_window_set_position(GTK_WINDOW(file_selection), GTK_WIN_POS_MOUSE);

  /* run the dialog */
  gtk_widget_show(GTK_WIDGET(file_selection));

  return;
}

/* callback for the close button*/
void ui_roi_analysis_dialog_cb_close_button(GtkWidget* widget, gpointer data) {
  
  GtkWidget * dialog = data;
  gnome_dialog_close(GNOME_DIALOG(dialog));
  return;
}

/* function called to destroy the roi dialog */
gboolean ui_roi_analysis_dialog_cb_close(GtkWidget* widget, gpointer data) {

  analysis_roi_t * roi_analyses = data;

  /* free the associated data structure */
  roi_analyses = analysis_roi_free(roi_analyses);

  return FALSE;
}


