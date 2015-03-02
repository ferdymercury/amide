/* ui_study_cb.c
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
#include <dirent.h>
#include <fnmatch.h>
#include "gtkdirsel.h"
#include "rendering.h"
#include "study.h"
#include "amitk_threshold.h"
#include "amitk_canvas.h"
#include "amitk_tree.h"
#include "amitk_tree_item.h"
#include "ui_common.h"
#include "ui_common_cb.h"
#include "ui_rendering.h"
#include "ui_series.h"
#include "ui_study.h"
#include "ui_study_cb.h"
#include "ui_preferences_dialog.h"
#include "ui_roi_dialog.h"
#include "ui_study_dialog.h"
#include "ui_time_dialog.h"
#include "ui_alignment_dialog.h"
#include "ui_volume_dialog.h"
#include "ui_align_pt_dialog.h"
#include "ui_roi_analysis_dialog.h"

/* function to handle loading in an AMIDE study */
static void ui_study_cb_open_ok(GtkWidget* widget, gpointer data) {

  GtkWidget * dir_selection = data;
  gchar * open_filename;
  struct stat file_info;
  study_t * study;

  /* get the filename */
  open_filename = gtk_dir_selection_get_filename(GTK_DIR_SELECTION(dir_selection));

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
  ui_common_cb_file_selection_cancel(widget, dir_selection);

  /* setup the study window */
  ui_study_create(study, NULL);

  return;
}


/* function to load a study into a  study widget w*/
void ui_study_cb_open_study(GtkWidget * button, gpointer data) {

  GtkWidget * dir_selection;

  dir_selection = gtk_dir_selection_new(_("Open AMIDE File"));

  /* don't want anything else going on till this window is gone */
  gtk_window_set_modal(GTK_WINDOW(dir_selection), TRUE);

  /* connect the signals */
  gtk_signal_connect(GTK_OBJECT(GTK_DIR_SELECTION(dir_selection)->ok_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(ui_study_cb_open_ok),
		     dir_selection);
  gtk_signal_connect(GTK_OBJECT(GTK_DIR_SELECTION(dir_selection)->cancel_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(ui_common_cb_file_selection_cancel),
		     dir_selection);
  gtk_signal_connect(GTK_OBJECT(GTK_DIR_SELECTION(dir_selection)->cancel_button),
		     "delete_event",
		     GTK_SIGNAL_FUNC(ui_common_cb_file_selection_cancel),
		     dir_selection);

  /* set the position of the dialog */
  gtk_window_set_position(GTK_WINDOW(dir_selection), GTK_WIN_POS_MOUSE);

  /* run the dialog */
  gtk_widget_show(dir_selection);

  return;
}
  

/* function to create a new study widget */
void ui_study_cb_new_study(GtkWidget * button, gpointer data) {

  ui_study_create(NULL, NULL);

  return;
}

/* function to handle saving */
static void ui_study_cb_save_as_ok(GtkWidget* widget, gpointer data) {

  GtkWidget * dir_selection = data;
  GtkWidget * question;
  ui_study_t * ui_study;
  gchar * save_filename;
  gchar * prev_filename;
  gchar * temp_string;
  struct stat file_info;
  DIR * directory;
  mode_t mode = 0766;
  struct dirent * directory_entry;
  gchar ** frags1=NULL;
  gchar ** frags2=NULL;

  /* get a pointer to ui_study */
  ui_study = gtk_object_get_data(GTK_OBJECT(dir_selection), "ui_study");

  /* get the filename */
  save_filename = gtk_dir_selection_get_filename(GTK_DIR_SELECTION(dir_selection));

  /* some sanity checks */
  if ((strcmp(save_filename, ".") == 0) ||
      (strcmp(save_filename, "..") == 0) ||
      (strcmp(save_filename, "") == 0) ||
      (strcmp(save_filename, "/") == 0)) {
    g_warning("%s: Inappropriate filename: %s",PACKAGE, save_filename);
    return;
  }

  /* make sure the filename ends with .xif */
  g_strreverse(save_filename);
  frags1 = g_strsplit(save_filename, ".", 2);
  g_strreverse(save_filename);
  g_strreverse(frags1[0]);
  frags2 = g_strsplit(frags1[0], "/", -1);
  if (g_strcasecmp(frags2[0], "xif") != 0) {
    prev_filename = save_filename;
    save_filename = g_strdup_printf("%s%s",prev_filename, ".xif");
    g_free(prev_filename);
  }    
  g_strfreev(frags2);
  g_strfreev(frags1);

  /* see if the filename already exists */
  if (stat(save_filename, &file_info) == 0) {
    /* check if it's okay to writeover the file */
    temp_string = g_strdup_printf("Overwrite file: %s", save_filename);
    if (GNOME_IS_APP(ui_study->app))
      question = gnome_question_dialog_modal_parented(temp_string, NULL, NULL, 
						      GTK_WINDOW(ui_study->app));
    else
      question = gnome_question_dialog_modal(temp_string, NULL, NULL);
    g_free(temp_string);

    /* and wait for the question to return */
    if (gnome_dialog_run_and_close(GNOME_DIALOG(question)) == 1)
      return; /* we don't want to overwrite the file.... */

    /* and start deleting everything in the filename/directory */
    if (S_ISDIR(file_info.st_mode)) {
      directory = opendir(save_filename);
      while ((directory_entry = readdir(directory)) != NULL) {
	temp_string = 
	  g_strdup_printf("%s%s%s", save_filename,G_DIR_SEPARATOR_S, directory_entry->d_name);

	if (fnmatch("*.xml",directory_entry->d_name,FNM_NOESCAPE) == 0) 
	  if (unlink(temp_string) != 0)
	    g_warning("%s: Couldn't unlink file: %s",PACKAGE, temp_string);
	if (fnmatch("*.dat",directory_entry->d_name,FNM_NOESCAPE) == 0) 
	  if (unlink(temp_string) != 0)
	    g_warning("%s: Couldn't unlink file: %s",PACKAGE, temp_string);

	g_free(temp_string);
      }

    } else if (S_ISREG(file_info.st_mode)) {
      if (unlink(save_filename) != 0)
	g_warning("%s: Couldn't unlink file: %s",PACKAGE, save_filename);

    } else {
      g_warning("%s: Unrecognized file type for file: %s, couldn't delete",PACKAGE, save_filename);
      return;
    }

  } else {
    /* make the directory */
    if (mkdir(save_filename, mode) != 0) {
      g_warning("%s: Couldn't create amide directory: %s",PACKAGE, save_filename);
      return;
    }
  }

  /* allright, save our study */
  if (study_write_xml(ui_study->study, save_filename) == FALSE) {
    g_warning("%s: Failure Saving File: %s",PACKAGE, save_filename);
    return;
  }

  /* close the file selection box */
  ui_common_cb_file_selection_cancel(widget, dir_selection);

  return;
}

/* function to load in an amide xml file */
void ui_study_cb_save_as(GtkWidget * widget, gpointer data) {
  
  ui_study_t * ui_study = data;
  GtkDirSelection * dir_selection;
  gchar * temp_string;

  dir_selection = GTK_DIR_SELECTION(gtk_dir_selection_new(_("Save File")));

  /* take a guess at the filename */
  if (study_filename(ui_study->study) == NULL) 
    temp_string = g_strdup_printf("%s.xif", study_name(ui_study->study));
  else
    temp_string = g_strdup_printf("%s", study_filename(ui_study->study));
  gtk_dir_selection_set_filename(GTK_DIR_SELECTION(dir_selection), temp_string);
  g_free(temp_string); 

  /* save a pointer to ui_study */
  gtk_object_set_data(GTK_OBJECT(dir_selection), "ui_study", ui_study);

  /* don't want anything else going on till this window is gone */
  gtk_window_set_modal(GTK_WINDOW(dir_selection), TRUE);

  /* connect the signals */
  gtk_signal_connect(GTK_OBJECT(dir_selection->ok_button), "clicked",
		     GTK_SIGNAL_FUNC(ui_study_cb_save_as_ok), dir_selection);
  gtk_signal_connect(GTK_OBJECT(dir_selection->cancel_button), "clicked",
		     GTK_SIGNAL_FUNC(ui_common_cb_file_selection_cancel), dir_selection);
  gtk_signal_connect(GTK_OBJECT(dir_selection->cancel_button), "delete_event",
		     GTK_SIGNAL_FUNC(ui_common_cb_file_selection_cancel), dir_selection);

  /* set the position of the dialog */
  gtk_window_set_position(GTK_WINDOW(dir_selection), GTK_WIN_POS_MOUSE);

  /* run the dialog */
  gtk_widget_show(GTK_WIDGET(dir_selection));

  return;
}

/* function to start the file importation process */
static void ui_study_cb_import_ok(GtkWidget* widget, gpointer data) {

  GtkWidget * file_selection = data;
  ui_study_t * ui_study;
  gchar * import_filename=NULL;
  import_method_t import_method;
  gchar * model_filename=NULL;
  gchar * model_name;
  GtkWidget * entry;
  gint entry_return;
  struct stat file_info;
  int submethod = 0;

  /* get a pointer to ui_study */
  ui_study = gtk_object_get_data(GTK_OBJECT(file_selection), "ui_study");

  /* figure out how we want to import */
  import_method = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(file_selection), "method"));

  /* we need some more info if we're doing a PEM file */
  if (import_method == PEM_DATA) {

    /* get the name of the model data file */
    entry = gnome_request_dialog(FALSE, 
				 "Corresponding Model File, if any?", "", 
				 256,  ui_common_cb_entry_name,
				 &model_name, 
				 (GNOME_IS_APP(ui_study->app) ? 
				  GTK_WINDOW(ui_study->app) : NULL));
    entry_return = gnome_dialog_run_and_close(GNOME_DIALOG(entry));
  
    if (model_name == NULL)
      model_filename = NULL; 
    else if (entry_return != 0) /* did we hit cancel */
      model_filename = NULL;
    else if (strcmp(model_name, "") == 0) /* hit ok, but no entry */
      model_filename = NULL;
    else {
      /* assumes the model is in the same directory as the data file */
      model_filename = 
	g_strconcat(g_dirname(import_filename),"/",model_name, NULL);

      if (stat(model_filename, &file_info) != 0) {
	/* hit ok, but no such file */
	g_warning("%s: no such PEM Model File:\n\t%s",PACKAGE, model_filename);
	model_filename = NULL;
      }
    }
  }

#ifdef AMIDE_LIBMDC_SUPPORT
  /* figure out the submethod if we're loading through libmdc */
  if (import_method == LIBMDC_DATA) 
    submethod =  GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(file_selection), "submethod"));
#endif


  /* get the filename and import */
  import_filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_selection));
  ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[TRANSVERSE]);
  ui_study_import_file(ui_study, import_method, submethod, import_filename, model_filename);
  ui_common_remove_cursor(ui_study->canvas[TRANSVERSE]);

  /* garbage cleanup */
  if (import_method == PEM_DATA) {
    g_free(model_name);
    g_free(model_filename);
  }

  //  g_free(import_filename); /* this causes a crash for some reason, is import_filename just
  //			    *  a reference into the file_selection box? */

  /* close the file selection box */
  ui_common_cb_file_selection_cancel(widget, file_selection);
  
  return;
}


/* function to selection which file to import */
void ui_study_cb_import(GtkWidget * widget, gpointer data) {
  
  ui_study_t * ui_study = data;
  GtkFileSelection * file_selection;
  import_method_t import_method;

  file_selection = GTK_FILE_SELECTION(gtk_file_selection_new(_("Import File")));

  /* save a pointer to ui_study */
  gtk_object_set_data(GTK_OBJECT(file_selection), "ui_study", ui_study);
  
  /* and save which method of importing we want to use */
  import_method = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "method"));
  gtk_object_set_data(GTK_OBJECT(file_selection), "method", GINT_TO_POINTER(import_method));
#ifdef AMIDE_LIBMDC_SUPPORT
  if (import_method == LIBMDC_DATA)
    gtk_object_set_data(GTK_OBJECT(file_selection), "submethod",
			gtk_object_get_data(GTK_OBJECT(widget), "submethod"));
#endif

  /* don't want anything else going on till this window is gone */
  gtk_window_set_modal(GTK_WINDOW(file_selection), TRUE);

  /* connect the signals */
  gtk_signal_connect(GTK_OBJECT(file_selection->ok_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(ui_study_cb_import_ok),
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


/* function to handle exporting a view */
static void ui_study_cb_export_ok(GtkWidget* widget, gpointer data) {

  GtkWidget * file_selection = data;
  GtkWidget * question;
  ui_study_t * ui_study;
  gchar * save_filename;
  gchar * temp_string;
  struct stat file_info;
  GdkImlibImage * export_image;
  guchar * pixels;
  view_t  view;

  /* get a pointer to ui_study */
  ui_study = gtk_object_get_data(GTK_OBJECT(file_selection), "ui_study");

  /* get the filename */
  save_filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_selection));

  /* figure out which view we're saving */
  view = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(file_selection), "view"));

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
    if (GNOME_IS_APP(ui_study->app))
      question = gnome_question_dialog_modal_parented(temp_string, NULL, NULL, 
						      GTK_WINDOW(ui_study->app));
    else
      question = gnome_question_dialog_modal(temp_string, NULL, NULL);
    g_free(temp_string);

    /* and wait for the question to return */
    if (gnome_dialog_run_and_close(GNOME_DIALOG(question)) == 1)
      return; /* we don't want to overwrite the file.... */
  }

  /* need to get a pointer to the pixels that we're going to save */
  pixels = gdk_pixbuf_get_pixels(AMITK_CANVAS(ui_study->canvas[view])->pixbuf);

  /* yep, we still need to use imlib for the moment for generating output images,
     maybe gdk_pixbuf will have this functionality at some point */
  export_image = gdk_imlib_create_image_from_data(pixels, NULL, 
						  gdk_pixbuf_get_width(AMITK_CANVAS(ui_study->canvas[view])->pixbuf),
						  gdk_pixbuf_get_height(AMITK_CANVAS(ui_study->canvas[view])->pixbuf));
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
  ui_common_cb_file_selection_cancel(widget, file_selection);

  return;
}

/* function to save a view as an external data format */
void ui_study_cb_export(GtkWidget * widget, gpointer data) {
  
  ui_study_t * ui_study = data;
  volumes_t * current_volumes;
  GtkFileSelection * file_selection;
  gchar * temp_string;
  gchar * data_set_names = NULL;
  view_t view;
  floatpoint_t upper, lower;
  realpoint_t view_center;
  realpoint_t temp_p;
  realspace_t * canvas_coord_frame;

  /* sanity checks */
  if (study_volumes(ui_study->study) == NULL) return;

  current_volumes = AMITK_TREE_SELECTED_VOLUMES(ui_study->tree);
  if (current_volumes == NULL) return;

  file_selection = GTK_FILE_SELECTION(gtk_file_selection_new(_("Export File")));
  view = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "view"));
  canvas_coord_frame = AMITK_CANVAS(ui_study->canvas[view])->coord_frame;
  g_return_if_fail(canvas_coord_frame != NULL);

  /* get the center */
  view_center = study_view_center(ui_study->study);
  /* translate the point so that the z coordinate corresponds to depth in this view */
  temp_p = realspace_alt_coord_to_alt(view_center,
				      study_coord_frame(ui_study->study),
				      canvas_coord_frame);

  /* figure out the top and bottom of this slice */
  upper = temp_p.z + study_view_thickness(ui_study->study)/2.0;
  lower = temp_p.z - study_view_thickness(ui_study->study)/2.0;

  /* take a guess at the filename */
  data_set_names = g_strdup(current_volumes->volume->name);
  current_volumes = current_volumes->next;
  while (current_volumes != NULL) {
    temp_string = g_strdup_printf("%s+%s",data_set_names, current_volumes->volume->name);
    g_free(data_set_names);
    data_set_names = temp_string;
    current_volumes = current_volumes->next;
  }
  temp_string = g_strdup_printf("%s_{%s}_%s_%3.1f-%3.1f.jpg", 
				study_name(ui_study->study),
				data_set_names,
				view_names[view],
				lower,
				upper);
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selection), temp_string);
  g_free(data_set_names);
  g_free(temp_string); 

  /* save a pointer to ui_study, and record which view we're saving*/
  gtk_object_set_data(GTK_OBJECT(file_selection), "ui_study", ui_study);
  gtk_object_set_data(GTK_OBJECT(file_selection), "view", GINT_TO_POINTER(view));

  /* don't want anything else going on till this window is gone */
  gtk_window_set_modal(GTK_WINDOW(file_selection), TRUE);

  /* connect the signals */
  gtk_signal_connect(GTK_OBJECT(file_selection->ok_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(ui_study_cb_export_ok),
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

/* callback generally attached to the entry_notify_event */
gboolean ui_study_cb_update_help_info(GtkWidget * widget, GdkEventCrossing * event, gpointer data) {
  ui_study_t * ui_study = data;
  help_info_t which_info;

  which_info = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "which_help"));

  /* quick correction so we can handle leaving a tree leaf */
  if (((which_info == HELP_INFO_TREE_VOLUME) ||
       (which_info == HELP_INFO_TREE_ROI) ||
       (which_info == HELP_INFO_TREE_STUDY)) &&
      (event->type == GDK_LEAVE_NOTIFY))
    which_info = HELP_INFO_TREE_NONE;

  ui_study_update_help_info(ui_study, which_info, zero_rp, 0.0);

  return FALSE;
}



void ui_study_cb_canvas_help_event(GtkWidget * canvas,  help_info_t help_type,
				   realpoint_t *location, gfloat value,  gpointer data) {
  ui_study_t * ui_study = data;
  ui_study_update_help_info(ui_study, help_type, *location, value);
  return;
}

void ui_study_cb_canvas_z_position_changed(GtkWidget * canvas, realpoint_t *position, gpointer data) {

  ui_study_t * ui_study = data;
  view_t i_view;

  study_set_view_center(ui_study->study, 
			realspace_base_coord_to_alt(*position, study_coord_frame(ui_study->study)));

  /* update the other canvases accordingly */
  for (i_view=0; i_view<NUM_VIEWS; i_view++)
    if (canvas != ui_study->canvas[i_view])
      amitk_canvas_update_arrows(AMITK_CANVAS(ui_study->canvas[i_view]));

  return;
}

void ui_study_cb_canvas_view_changing(GtkWidget * canvas, realpoint_t *position,
				      gfloat thickness, gpointer data) {


  ui_study_t * ui_study = data;
  rgba_t outline_color;
  view_t i_view;

  outline_color = color_table_outline_color(ui_study->active_volume->color_table, FALSE);

  ui_study_update_thickness_adjustment(ui_study, thickness);

  /* update the other canvases accordingly */
  for (i_view=0; i_view<NUM_VIEWS; i_view++)
    if (canvas != ui_study->canvas[i_view])
      amitk_canvas_update_cross(AMITK_CANVAS(ui_study->canvas[i_view]), AMITK_CANVAS_CROSS_SHOW,
				*position, outline_color, thickness);

  return;
}

void ui_study_cb_canvas_view_changed(GtkWidget * canvas, realpoint_t *position,
				     gfloat thickness, gpointer data) {

  ui_study_t * ui_study = data;
  view_t i_view;

  study_set_view_center(ui_study->study, 
  			realspace_base_coord_to_alt(*position, study_coord_frame(ui_study->study)));
  study_set_view_thickness(ui_study->study, thickness);
  ui_study_update_thickness_adjustment(ui_study, study_view_thickness(ui_study->study));

  /* update the other canvases accordingly */
  for (i_view=0; i_view<NUM_VIEWS; i_view++)
    if (canvas != ui_study->canvas[i_view]) {
      amitk_canvas_update_cross(AMITK_CANVAS(ui_study->canvas[i_view]), AMITK_CANVAS_CROSS_HIDE,
				zero_rp, rgba_black, 0);/* remove targets */
      amitk_canvas_set_thickness(AMITK_CANVAS(ui_study->canvas[i_view]), 
				 study_view_thickness(ui_study->study), FALSE);
      amitk_canvas_set_center(AMITK_CANVAS(ui_study->canvas[i_view]),  *position, TRUE);
    }

  return;
}

void ui_study_cb_canvas_volumes_changed(GtkWidget * canvas, gpointer data) {
  ui_study_t * ui_study = data;
  view_t i_view;

  /* update the other canvases accordingly */
  for (i_view=0; i_view<NUM_VIEWS; i_view++)
    if (canvas != ui_study->canvas[i_view])
      amitk_canvas_update_volumes(AMITK_CANVAS(ui_study->canvas[i_view]));
  /* I really should check to see if a volume dialog is up, and update the rotations.... */

  return;
}

void ui_study_cb_canvas_roi_changed(GtkWidget * canvas, roi_t * roi, gpointer data) {
  ui_study_t * ui_study = data;
  view_t i_view;

  /* update the other canvases accordingly */
  for (i_view=0; i_view<NUM_VIEWS; i_view++)
    if (canvas != ui_study->canvas[i_view])
      amitk_canvas_update_object(AMITK_CANVAS(ui_study->canvas[i_view]), roi, ROI);
  /* I really should check to see if an ROI dialog is up, and update it */

  return;
}

void ui_study_cb_canvas_isocontour_3d_changed(GtkWidget * canvas, roi_t * roi, 
					      realpoint_t *position, gpointer data) {
  ui_study_t * ui_study = data;
  view_t i_view;
  realpoint_t temp_rp;
  voxelpoint_t temp_vp;

  g_return_if_fail(ui_study->active_volume != NULL);

  temp_rp = realspace_base_coord_to_alt(*position, ui_study->active_volume->coord_frame);
  REALPOINT_TO_VOXEL(temp_rp, ui_study->active_volume->voxel_size, 
		     volume_frame(ui_study->active_volume, study_view_time(ui_study->study)), 
		     temp_vp);
  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(canvas));
  if (data_set_includes_voxel(ui_study->active_volume->data_set,temp_vp))
    roi_set_isocontour(roi, ui_study->active_volume, temp_vp);
  ui_common_remove_cursor(GTK_WIDGET(canvas));

  /* update all the canvases accordingly */
  for (i_view=0; i_view<NUM_VIEWS; i_view++)
    amitk_canvas_update_object(AMITK_CANVAS(ui_study->canvas[i_view]), roi, ROI);

}

void ui_study_cb_canvas_new_align_pt(GtkWidget * canvas, realpoint_t *position, gpointer data) {

  ui_study_t * ui_study = data;
  view_t i_view;
  align_pt_t * new_pt;

  g_return_if_fail(ui_study->active_volume != NULL);
  new_pt = ui_study_add_align_pt(ui_study, ui_study->active_volume);
  if (new_pt == NULL) return;

  new_pt->point = realspace_base_coord_to_alt(*position, ui_study->active_volume->coord_frame);
  for (i_view=0; i_view<NUM_VIEWS; i_view++)
    amitk_canvas_add_object(AMITK_CANVAS(ui_study->canvas[i_view]), 
			    new_pt, ALIGN_PT, ui_study->active_volume);

  return;
}

void ui_study_cb_tree_select_object(GtkWidget * tree, gpointer object, object_t type, 
				    gpointer parent, object_t parent_type, gpointer data) {

  ui_study_t * ui_study = data;
  view_t i_view;
  for (i_view=0; i_view<NUM_VIEWS; i_view++)
    amitk_canvas_add_object(AMITK_CANVAS(ui_study->canvas[i_view]), 
  			    object, type, parent);


  switch(type) {
  case VOLUME:
    ui_time_dialog_set_times(ui_study);
    ui_study_make_active_volume(ui_study, object);
    break;
  default:
    break;
  }

  return;
}

void ui_study_cb_tree_unselect_object(GtkWidget * tree, gpointer object, object_t type, 
				      gpointer parent, object_t parent_type, gpointer data) {

  ui_study_t * ui_study = data;
  view_t i_view;

  for (i_view=0; i_view<NUM_VIEWS; i_view++)
    amitk_canvas_remove_object(AMITK_CANVAS(ui_study->canvas[i_view]), 
			       object, type, TRUE);

  switch(type) {
  case VOLUME:
    ui_time_dialog_set_times(ui_study);
    if (ui_study->active_volume == object)
      ui_study_make_active_volume(ui_study, NULL);
    break;
  default:
    break;
  }

  return;
}

void ui_study_cb_tree_make_active_object(GtkWidget * tree, gpointer object, object_t type, 
					 gpointer parent, object_t parent_type, gpointer data) {

  ui_study_t * ui_study = data;
  view_t i_view;
  roi_t * roi=object;
  volume_t * parent_volume = parent;
  align_pt_t * align_pt=object;
  realpoint_t center;

  switch(type) {
  case VOLUME:
    ui_study_make_active_volume(ui_study, object);
    break;
  case ROI:
    center = realspace_base_coord_to_alt(roi_calculate_center(roi), study_coord_frame(ui_study->study));
    if (!roi_undrawn(roi) && !REALPOINT_EQUAL(center, study_view_center(ui_study->study))) {
      study_set_view_center(ui_study->study, center);
      for (i_view=0; i_view<NUM_VIEWS; i_view++)
	amitk_canvas_set_center(AMITK_CANVAS(ui_study->canvas[i_view]), 
				realspace_alt_coord_to_base(study_view_center(ui_study->study),
							    study_coord_frame(ui_study->study)),
				TRUE);
    }
    break;
  case ALIGN_PT:
    center = realspace_alt_coord_to_alt(align_pt->point, 
					parent_volume->coord_frame,
					study_coord_frame(ui_study->study));
    if ( !REALPOINT_EQUAL(center, study_view_center(ui_study->study))) {
      study_set_view_center(ui_study->study, center);
      for (i_view=0; i_view<NUM_VIEWS; i_view++)
	amitk_canvas_set_center(AMITK_CANVAS(ui_study->canvas[i_view]), 
				realspace_alt_coord_to_base(study_view_center(ui_study->study),
							    study_coord_frame(ui_study->study)),
				TRUE);
    }
    break;
  default:
    break;
  }
  return;
}

void ui_study_cb_tree_popup_object(GtkWidget * tree, gpointer object, object_t type, 
				   gpointer parent, object_t parent_type, gpointer data) {

  ui_study_t * ui_study = data;
  GtkWidget * tree_item;

  /* get the tree item */
  tree_item = amitk_tree_find_object(AMITK_TREE(tree), object, type);
  g_return_if_fail(AMITK_IS_TREE_ITEM(tree_item));

  if (!AMITK_TREE_ITEM_DIALOG(tree_item)) {
    switch(type) {
    case VOLUME:
      ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[TRANSVERSE]);
      /* if the threshold dialog corresponds to this volume, kill it */
      if (ui_study->threshold_dialog != NULL)
	if (amitk_threshold_dialog_volume(AMITK_THRESHOLD_DIALOG(ui_study->threshold_dialog)) == object) {
	  gtk_widget_destroy(ui_study->threshold_dialog);
	  ui_study->threshold_dialog = NULL;
	}
      AMITK_TREE_ITEM_DIALOG(tree_item) = 
	ui_volume_dialog_create(ui_study, object); /* start up the volume dialog */
      ui_common_remove_cursor(ui_study->canvas[TRANSVERSE]);
      break;
    case ROI:
      AMITK_TREE_ITEM_DIALOG(tree_item) = 
	ui_roi_dialog_create(ui_study, object);
      break;
    case ALIGN_PT:
      AMITK_TREE_ITEM_DIALOG(tree_item) = 
	ui_align_pt_dialog_create(ui_study, object, parent);
      break;
    case STUDY:
      AMITK_TREE_ITEM_DIALOG(tree_item) = 
	ui_study_dialog_create(ui_study);
      break;
    default:
      break;
    }
  } else {
    /* pop the window to the top */
    ; //GTK 2.0    gtk_window_present(GTK_WINDOW(AMITK_TREE_ITEM_DIALOG(tree_item)));
  }

  return;
}

void ui_study_cb_tree_add_object(GtkWidget * tree, object_t type, gpointer parent, 
				 object_t parent_type, gpointer data) {
  ui_study_t * ui_study = data;

  switch(type) {

  case ALIGN_PT:

    switch(parent_type) {
    case VOLUME:
      ui_study_add_align_pt(ui_study, parent);
      break;
    default:
      break;
    }
    break;

  default:
    break;
  }

  return;
}

void ui_study_cb_tree_help_event(GtkWidget * widget, help_info_t help_type, gpointer data) {
  ui_study_t * ui_study = data;
  ui_study_update_help_info(ui_study, help_type, zero_rp, 0.0);
  return;
}

void ui_study_cb_zoom(GtkObject * adjustment, gpointer data) {

  ui_study_t * ui_study = data;
  view_t i_view;

  /* sanity check */
  if (study_volumes(ui_study->study) == NULL)
    return;

  if (study_zoom(ui_study->study) != GTK_ADJUSTMENT(adjustment)->value) {
      study_set_zoom(ui_study->study, GTK_ADJUSTMENT(adjustment)->value);
      for (i_view=0; i_view<NUM_VIEWS; i_view++)
	amitk_canvas_set_zoom(AMITK_CANVAS(ui_study->canvas[i_view]), 
			      study_zoom(ui_study->study),TRUE);
  }
    
  return;
}


void ui_study_cb_time_pressed(GtkWidget * combo, gpointer data) {
  
  ui_study_t * ui_study = data;

  /* sanity check */
  ui_time_dialog_create(ui_study);

  return;

}


void ui_study_cb_thickness(GtkObject * adjustment, gpointer data) {

  ui_study_t * ui_study = data;
  view_t i_view;

  /* sanity check */
  if (study_volumes(ui_study->study) == NULL)
    return;

  gtk_signal_emit_stop_by_name(GTK_OBJECT(adjustment), "value_changed");
  if (study_view_thickness(ui_study->study) != GTK_ADJUSTMENT(adjustment)->value) {
    study_set_view_thickness(ui_study->study, GTK_ADJUSTMENT(adjustment)->value);
    for (i_view=0; i_view<NUM_VIEWS; i_view++)
      amitk_canvas_set_thickness(AMITK_CANVAS(ui_study->canvas[i_view]), 
				 study_view_thickness(ui_study->study), TRUE);
  }
  
  return;
}


/* callbacks for setting up a set of slices in a new window */
void ui_study_cb_series(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  view_t view;
  series_t series_type;
  
  view = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "view"));
  series_type = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "series_type"));

  ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[TRANSVERSE]);
  ui_series_create(ui_study->study, 
		   AMITK_TREE_SELECTED_VOLUMES(ui_study->tree), 
		   view, ui_study->canvas_layout,series_type);
  ui_common_remove_cursor(ui_study->canvas[TRANSVERSE]);

  return;
}

#ifdef AMIDE_LIBVOLPACK_SUPPORT
/* callback for starting up volume rendering using nearest neighbor interpolation*/
void ui_study_cb_rendering(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  interpolation_t interpolation;


  interpolation = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "interpolation"));

  ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[TRANSVERSE]);
  ui_rendering_create(AMITK_TREE_SELECTED_VOLUMES(ui_study->tree),
		      AMITK_TREE_SELECTED_ROIS(ui_study->tree),
		      study_coord_frame(ui_study->study),
		      study_view_time(ui_study->study), 
		      study_view_duration(ui_study->study),interpolation);
  ui_common_remove_cursor(ui_study->canvas[TRANSVERSE]);

  return;
}
#endif


/* do roi calculations for selected ROI's over selected volumes */
void ui_study_cb_calculate_all(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[TRANSVERSE]);
  ui_roi_analysis_dialog_create(ui_study, TRUE);
  ui_common_remove_cursor(ui_study->canvas[TRANSVERSE]);
  return;

}

/* do roi calculations for all ROI's over selected volumes */
void ui_study_cb_calculate_selected(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[TRANSVERSE]);
  ui_roi_analysis_dialog_create(ui_study, FALSE);
  ui_common_remove_cursor(ui_study->canvas[TRANSVERSE]);
  return;
}

/* user wants to run the alignment wizard */
void ui_study_cb_alignment_selected(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  ui_alignment_dialog_create(ui_study);
  return;
}

/* function called when the threshold is changed */
void ui_study_cb_threshold_changed(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study=data;
  view_t i_view;

  for(i_view=0; i_view<NUM_VIEWS;i_view++)
    amitk_canvas_threshold_changed(AMITK_CANVAS(ui_study->canvas[i_view]));

  return;
}


/* function called when the volume's color is changed */
void ui_study_cb_color_changed(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study=data;
  view_t i_view;

  for(i_view=0; i_view<NUM_VIEWS;i_view++)
    amitk_canvas_threshold_changed(AMITK_CANVAS(ui_study->canvas[i_view]));
  if (ui_study->active_volume != NULL)
    for(i_view=0; i_view<NUM_VIEWS;i_view++)
      amitk_canvas_set_color_table(AMITK_CANVAS(ui_study->canvas[i_view]), 
				   ui_study->active_volume->color_table, TRUE);

  return;
}

static gboolean ui_study_cb_threshold_close(GtkWidget* widget, gpointer data) {

  ui_study_t * ui_study = data;

  /* just keeping track on whether or not the threshold widget is up */
  ui_study->threshold_dialog = NULL;

  return FALSE;
}

/* function called when hitting the threshold button, pops up a dialog */
void ui_study_cb_threshold_pressed(GtkWidget * button, gpointer data) {

  ui_study_t * ui_study = data;
  GtkWidget * tree_item;

  if (ui_study->active_volume == NULL) return;

  /* make sure we don't already have a volume edit dialog up */
  tree_item = amitk_tree_find_object(AMITK_TREE(ui_study->tree), ui_study->active_volume, VOLUME);
  g_return_if_fail(AMITK_IS_TREE_ITEM(tree_item));

  if (!AMITK_TREE_ITEM_DIALOG(tree_item) && (ui_study->threshold_dialog == NULL)) {
    ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[TRANSVERSE]);
    ui_study->threshold_dialog = amitk_threshold_dialog_new(ui_study->active_volume);
    ui_common_remove_cursor(ui_study->canvas[TRANSVERSE]);
    gtk_signal_connect(GTK_OBJECT(ui_study->threshold_dialog), "threshold_changed",
		       GTK_SIGNAL_FUNC(ui_study_cb_threshold_changed),
		       ui_study);
    gtk_signal_connect(GTK_OBJECT(ui_study->threshold_dialog), "color_changed",
		     GTK_SIGNAL_FUNC(ui_study_cb_color_changed),
		       ui_study);
    gtk_signal_connect(GTK_OBJECT(ui_study->threshold_dialog), "close",
		       GTK_SIGNAL_FUNC(ui_study_cb_threshold_close),
		       ui_study);
    gtk_widget_show(GTK_WIDGET(ui_study->threshold_dialog));
  } else {
    /* pop the window to the top */
    ; //    if (!AMITK_TREE_ITEM_DIALOG(tree_item))
    //GTK2.0      gtk_window_present(GTK_WINDOW(AMITK_TREE_ITEM_DIALOG(tree_item)));
    //    else
    //GTK2.0      gtk_window_present(GTK_WINDOW(ui_study->threshold_dialog));
  }

  return;
}

/* callback function for adding an roi */
void ui_study_cb_add_roi(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  roi_type_t i_roi_type;
  GtkWidget * entry;
  gchar * roi_name = NULL;
  gint entry_return;
  roi_t * roi;

  /* figure out which menu item called me */
  i_roi_type = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"roi_type"));

  entry = gnome_request_dialog(FALSE, "ROI Name:", "", 256, 
			       ui_common_cb_entry_name,
			       &roi_name, 
			       (GNOME_IS_APP(ui_study->app) ? 
				GTK_WINDOW(ui_study->app) : NULL));
  
  entry_return = gnome_dialog_run_and_close(GNOME_DIALOG(entry));
  
  if (entry_return == 0) {
    roi = roi_init();
    roi_set_name(roi,roi_name);
    roi->type = i_roi_type;
    study_add_roi(ui_study->study, roi);
    amitk_tree_add_object(AMITK_TREE(ui_study->tree), roi, ROI, ui_study->study, STUDY, TRUE);
    amitk_tree_select_object(AMITK_TREE(ui_study->tree), roi, ROI);
    roi = roi_unref(roi);
  }

  return;
}


/* callback function for adding an alignment point */
void ui_study_cb_add_alignment_point(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  if (ui_study->active_volume == NULL) return;
  ui_study_add_align_pt(ui_study, ui_study->active_volume);
  return;
}

/* callback function for changing user's preferences */
void ui_study_cb_preferences(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study=data;
  if (ui_study->preferences_dialog == NULL)
    ui_study->preferences_dialog = ui_preferences_dialog_create(ui_study);
  return;
}


/* callback used when the background of the tree is clicked on */
gboolean ui_study_cb_tree_clicked(GtkWidget * leaf, GdkEventButton * event, gpointer data) {

  ui_study_t * ui_study = data;
  GtkWidget * menu;
  GtkWidget * menuitem;
  roi_type_t i_roi_type;

  if (event->type != GDK_BUTTON_PRESS)
    return TRUE;

  if (event->button == 3) {
    menu = gtk_menu_new();

    for (i_roi_type=0; i_roi_type<NUM_ROI_TYPES; i_roi_type++) {
      menuitem = gtk_menu_item_new_with_label(roi_menu_explanation[i_roi_type]);
      gtk_menu_append(GTK_MENU(menu), menuitem);
      gtk_object_set_data(GTK_OBJECT(menuitem), "roi_type", GINT_TO_POINTER(i_roi_type)); 
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
			 GTK_SIGNAL_FUNC(ui_study_cb_add_roi), ui_study);
      gtk_widget_show(menuitem);
    }
    gtk_widget_show(menu);
    
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
  }

  return TRUE;
}


/* function called to edit selected objects in the study */
void ui_study_cb_edit_objects(GtkWidget * button, gpointer data) {

  ui_study_t * ui_study = data;
  object_t type;
  volumes_t * current_volumes;
  rois_t * current_rois;
  align_pts_t * current_pts;
  align_pts_t * temp_pts;

  type = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(button), "type"));


  switch(type) {
  case VOLUME:
    current_volumes = AMITK_TREE_SELECTED_VOLUMES(ui_study->tree);
    /* pop up dialogs for the selected volumes */
    while (current_volumes != NULL) {
      ui_volume_dialog_create(ui_study, current_volumes->volume);
      current_volumes = current_volumes->next;
    }
    break;
    
  case ROI:
    current_rois = AMITK_TREE_SELECTED_ROIS(ui_study->tree);
    /* pop up dialogs for the selected rois */
    while (current_rois != NULL) {
      ui_roi_dialog_create(ui_study, current_rois->roi);
      current_rois = current_rois->next;
    }
    break;

  case ALIGN_PT:
    current_volumes = AMITK_TREE_SELECTED_VOLUMES(ui_study->tree);
    /* pop up dialogs for the selected alignment points */
    while (current_volumes != NULL) {
      current_pts = amitk_tree_selected_pts(AMITK_TREE(ui_study->tree), current_volumes->volume, VOLUME);
      temp_pts = current_pts;

      while (temp_pts != NULL) {
	ui_align_pt_dialog_create(ui_study, temp_pts->align_pt, current_volumes->volume);
	temp_pts = temp_pts->next;
      }
      current_pts = align_pts_unref(current_pts);

      current_volumes = current_volumes->next;
    }
    break;

  case STUDY:
    ui_study_dialog_create(ui_study);
    break;

  default:
    g_warning("%s: unexpected case in %s at line %d",PACKAGE, __FILE__, __LINE__);
    break;
  }

  return;
}



/* function called to delete selected objects from the tree and the study */
void ui_study_cb_delete_objects(GtkWidget * button, gpointer data) {

  ui_study_t * ui_study = data;
  object_t type;
  volume_t * volume;
  roi_t * roi;
  GtkWidget * question;
  gchar * message;
  gchar * temp;
  gint question_return;
  volumes_t * current_volumes;
  rois_t * current_rois;
  align_pts_t * current_pts;
  align_pts_t * temp_pts;
  gboolean found_some = FALSE;

  type = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(button), "type"));


  switch(type) {
  case VOLUME:
    current_volumes = AMITK_TREE_SELECTED_VOLUMES(ui_study->tree);
    if (current_volumes == NULL) return;

    message = g_strdup_printf("Do you want to delete:\n");
    while (current_volumes != NULL) {
      temp = message;
      message = g_strdup_printf("%s\tdata set:\t%s\n", temp, current_volumes->volume->name);
      g_free(temp);
      current_volumes = current_volumes->next;
    }
    break;
  case ROI:
    current_rois = AMITK_TREE_SELECTED_ROIS(ui_study->tree);
    if (current_rois == NULL) return;

    message = g_strdup_printf("Do you want to delete:\n");
    while (current_rois != NULL) {
      temp = message;
      message = g_strdup_printf("%s\troi:\t%s\n", temp, current_rois->roi->name);
      g_free(temp);
      current_rois = current_rois->next;
    }

    break;

  case ALIGN_PT:
    current_volumes = AMITK_TREE_SELECTED_VOLUMES(ui_study->tree);
    while ((current_volumes != NULL) && (!found_some)) {
      current_pts = amitk_tree_selected_pts(AMITK_TREE(ui_study->tree), current_volumes->volume, VOLUME);
      if (current_pts != NULL)
	found_some = TRUE;
      current_pts = align_pts_unref(current_pts);
      current_volumes = current_volumes->next;
    }
    if (!found_some) return;

    message = g_strdup_printf("Do you want to delete:\n");
    current_volumes = AMITK_TREE_SELECTED_VOLUMES(ui_study->tree);
    while (current_volumes != NULL) {
      current_pts = amitk_tree_selected_pts(AMITK_TREE(ui_study->tree), current_volumes->volume, VOLUME);
      temp_pts = current_pts;
      while (temp_pts != NULL) {
	temp = message;
	message = g_strdup_printf("%s\talignment point:\t%s - on data set:\t%s\n", temp, 
				  temp_pts->align_pt->name,
				  current_volumes->volume->name);
	g_free(temp);
	temp_pts = temp_pts->next;
      }
      current_pts = align_pts_unref(current_pts);
      current_volumes = current_volumes->next;
    }
    break;

  default:
    g_warning("%s: unexpected case in %s at line %d",PACKAGE, __FILE__, __LINE__);
    return;
    break;
  }

  /* see if we really wanna delete this crap */
  if (GNOME_IS_APP(ui_study->app))
    question = gnome_ok_cancel_dialog_modal_parented(message, NULL, NULL, 
						     GTK_WINDOW(ui_study->app));
  else
    question = gnome_ok_cancel_dialog_modal(message, NULL, NULL);

  g_free(message);
  question_return = gnome_dialog_run_and_close(GNOME_DIALOG(question));

  if (question_return != 0) return; /* didn't hit the "ok" button */
  
  switch(type) {

  case VOLUME:
    /* delete the selected volumes */
    current_volumes = AMITK_TREE_SELECTED_VOLUMES(ui_study->tree);
    while (current_volumes != NULL) {

      volume = volume_ref(current_volumes->volume);
      amitk_tree_remove_object(AMITK_TREE(ui_study->tree), current_volumes->volume, VOLUME);

      /* destroy the threshold window if it's open, and remove the active mark */
      if (ui_study->active_volume==volume)
	ui_study_make_active_volume(ui_study, NULL);
     
      study_remove_volume(ui_study->study, volume);
      volume = volume_unref(volume);
      current_volumes = AMITK_TREE_SELECTED_VOLUMES(ui_study->tree);
    }

    break;

  case ROI:
    /* delete the selected roi's */
    current_rois = AMITK_TREE_SELECTED_ROIS(ui_study->tree);
    while (current_rois != NULL) {

      roi = roi_ref(current_rois->roi);
      amitk_tree_remove_object(AMITK_TREE(ui_study->tree), current_rois->roi, ROI);

      /* remove the roi's data structure */
      study_remove_roi(ui_study->study, roi);
      roi = roi_unref(roi);
      current_rois = AMITK_TREE_SELECTED_ROIS(ui_study->tree);
    }
    break;

  case ALIGN_PT:
    /* delete the selected alignment points */
    current_volumes = AMITK_TREE_SELECTED_VOLUMES(ui_study->tree);
    while (current_volumes != NULL) {
      current_pts = amitk_tree_selected_pts(AMITK_TREE(ui_study->tree), current_volumes->volume, VOLUME);
      temp_pts = current_pts;
      while (temp_pts != NULL) {
	amitk_tree_remove_object(AMITK_TREE(ui_study->tree), temp_pts->align_pt, ALIGN_PT);

	/* remove the point from existence */
	current_volumes->volume->align_pts = 
	  align_pts_remove_pt(current_volumes->volume->align_pts, temp_pts->align_pt);
	temp_pts = temp_pts->next;
      }
      current_pts = align_pts_unref(current_pts);
      current_volumes = current_volumes->next;
    }
    break;

  default:
    g_warning("%s: unexpected case in %s at line %d",PACKAGE, __FILE__, __LINE__);
    return;
    break;
  }

  return;
}


/* function to switch the interpolation method */
void ui_study_cb_interpolation(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  interpolation_t i_interpolation;
  view_t i_view;

  /* figure out which interpolation menu item called me */
  i_interpolation = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"interpolation"));

  /* check if we actually changed values */
  if (study_interpolation(ui_study->study) != i_interpolation) {
    /* and inact the changes */
    study_set_interpolation(ui_study->study, i_interpolation);
    if (study_volumes(ui_study->study) != NULL) 
      for (i_view=0; i_view<NUM_VIEWS; i_view++)
	amitk_canvas_set_interpolation(AMITK_CANVAS(ui_study->canvas[i_view]), 
				       study_interpolation(ui_study->study), TRUE);
  }

  return;
}



/* function ran to exit the program */
void ui_study_cb_exit(GtkWidget* widget, gpointer data) {

  ui_study_t * ui_study = data;
  GtkWidget * app = GTK_WIDGET(ui_study->app);

  /* run the delete event function */
  ui_study_cb_delete_event(app, NULL, ui_study);
  gtk_widget_destroy(app);

  amide_unregister_all_windows(); /* kill everything */

  return;
}

/* function ran when closing a study window */
void ui_study_cb_close(GtkWidget* widget, gpointer data) {

  ui_study_t * ui_study = data;
  GtkWidget * app = GTK_WIDGET(ui_study->app);

  /* run the delete event function */
  ui_study_cb_delete_event(app, NULL, ui_study);
  gtk_widget_destroy(app);

  return;
}

/* function to run for a delete_event */
gboolean ui_study_cb_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_study_t * ui_study = data;
  GtkWidget * app = ui_study->app;

  /* check to see if we need saving */

  /* destroy the threshold window if it's open */
  if (ui_study->threshold_dialog != NULL) {
    gtk_widget_destroy(ui_study->threshold_dialog);
    ui_study->threshold_dialog = NULL;
  }

  /* destroy the preferences dialog if it's open */
  if (ui_study->preferences_dialog != NULL) {
    gnome_dialog_close(GNOME_DIALOG(ui_study->preferences_dialog));
    ui_study->preferences_dialog = NULL;
  }

  /* destroy the time window if it's open */
  if (ui_study->time_dialog != NULL) {
    gnome_dialog_close(GNOME_DIALOG(ui_study->time_dialog));
    ui_study->time_dialog = NULL;
  }

  
  ui_study = ui_study_free(ui_study); /* free our data structure's */
  amide_unregister_window((gpointer) app); /* tell the rest of the program this window's gone */

  return FALSE;
}
































