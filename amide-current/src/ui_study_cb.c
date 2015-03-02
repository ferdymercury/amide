/* ui_study_cb.c
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
#include <gnome.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include "amitk_dir_sel.h"
#include "amitk_threshold.h"
#include "amitk_canvas.h"
#include "amitk_tree.h"
#include "ui_common.h"
#include "ui_render.h"
#include "ui_series.h"
#include "ui_study.h"
#include "ui_study_cb.h"
#include "ui_preferences_dialog.h"
#include "amitk_object_dialog.h"
#include "amitk_progress_dialog.h"
#include "ui_time_dialog.h"
#include "tb_fly_through.h"
#include "tb_alignment.h"
#include "tb_crop.h"
#include "tb_fads.h"
#include "tb_filter.h"
#include "tb_roi_analysis.h"


/* function to handle loading in an AMIDE study */
static void ui_study_cb_open_ok(GtkWidget* widget, gpointer data) {

  GtkWidget * dir_selection = data;
  const gchar * open_filename;
  struct stat file_info;
  AmitkStudy * study;
  ui_study_t * ui_study;

  /* get the filename */
  open_filename = amitk_dir_selection_get_filename(AMITK_DIR_SELECTION(dir_selection));

  /* get a pointer to ui_study */
  ui_study = g_object_get_data(G_OBJECT(dir_selection), "ui_study");

  /* check to see that the filename exists and it's a directory */
  if (stat(open_filename, &file_info) != 0) {
    g_warning("AMIDE study %s does not exist",open_filename);
    return;
  }
  if (!S_ISDIR(file_info.st_mode)) {
    g_warning("%s is not a directory/AMIDE study",open_filename);
    return;
  }

  /* try loading the study into memory */
  if ((study=amitk_study_load_xml(open_filename)) == NULL) {
    g_warning("error loading study: %s",open_filename);
    return;
  }

  /* close the file selection box */
  ui_common_file_selection_cancel_cb(widget, dir_selection);

  /* setup the study window */
  if (ui_study->study_virgin)
    ui_study_set_study(ui_study, study);
  else
    ui_study_create(study);
  g_object_unref(study);


  return;
}


/* function to load a study into a  study widget w*/
void ui_study_cb_open_study(GtkWidget * button, gpointer data) {

  ui_study_t * ui_study=data;
  GtkWidget * dir_selection;

  dir_selection = amitk_dir_selection_new(_("Open AMIDE File"));

  /* don't want anything else going on till this window is gone */
  gtk_window_set_modal(GTK_WINDOW(dir_selection), TRUE);

  /* save a pointer to ui_study */
  g_object_set_data(G_OBJECT(dir_selection), "ui_study", ui_study);

  /* connect the signals */
  g_signal_connect(G_OBJECT(AMITK_DIR_SELECTION(dir_selection)->ok_button),
		   "clicked", G_CALLBACK(ui_study_cb_open_ok), dir_selection);
  g_signal_connect(G_OBJECT(AMITK_DIR_SELECTION(dir_selection)->cancel_button),
		   "clicked", G_CALLBACK(ui_common_file_selection_cancel_cb),dir_selection);
  g_signal_connect(G_OBJECT(AMITK_DIR_SELECTION(dir_selection)->cancel_button),
		   "delete_event", G_CALLBACK(ui_common_file_selection_cancel_cb), dir_selection);

  /* set the position of the dialog */
  gtk_window_set_position(GTK_WINDOW(dir_selection), GTK_WIN_POS_MOUSE);

  /* run the dialog */
  gtk_widget_show(dir_selection);

  return;
}
  

/* function to create a new study widget */
void ui_study_cb_new_study(GtkWidget * button, gpointer data) {

  ui_study_create(NULL);

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
  gint return_val;

  /* get a pointer to ui_study */
  ui_study = g_object_get_data(G_OBJECT(dir_selection), "ui_study");

  /* get the filename */
  save_filename = g_strdup(amitk_dir_selection_get_filename(AMITK_DIR_SELECTION(dir_selection)));

  /* some sanity checks */
  if ((strcmp(save_filename, ".") == 0) ||
      (strcmp(save_filename, "..") == 0) ||
      (strcmp(save_filename, "") == 0) ||
      (strcmp(save_filename, "/") == 0)) {
    g_warning("Inappropriate filename: %s",save_filename);
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
    question = gtk_message_dialog_new(GTK_WINDOW(ui_study->app),
				      GTK_DIALOG_DESTROY_WITH_PARENT,
				      GTK_MESSAGE_QUESTION,
				      GTK_BUTTONS_OK_CANCEL,
				      "Overwrite file: %s", save_filename);

    /* and wait for the question to return */
    return_val = gtk_dialog_run(GTK_DIALOG(question));

    gtk_widget_destroy(question);
    if (return_val != GTK_RESPONSE_OK)
      return; /* we don't want to overwrite the file.... */

    /* and start deleting everything in the filename/directory */
    if (S_ISDIR(file_info.st_mode)) {
      directory = opendir(save_filename);
      while ((directory_entry = readdir(directory)) != NULL) {
	temp_string = 
	  g_strdup_printf("%s%s%s", save_filename,G_DIR_SEPARATOR_S, directory_entry->d_name);

	if (g_pattern_match_simple("*.xml",directory_entry->d_name))
	  if (unlink(temp_string) != 0)
	    g_warning("Couldn't unlink file: %s",temp_string);
	if (g_pattern_match_simple("*.dat",directory_entry->d_name)) 
	  if (unlink(temp_string) != 0)
	    g_warning("Couldn't unlink file: %s",temp_string);

	g_free(temp_string);
      }

    } else if (S_ISREG(file_info.st_mode)) {
      if (unlink(save_filename) != 0)
	g_warning("Couldn't unlink file: %s",save_filename);

    } else {
      g_warning("Unrecognized file type for file: %s, couldn't delete",save_filename);
      return;
    }

  } else {
    /* make the directory */
    if (mkdir(save_filename, mode) != 0) {
      g_warning("Couldn't create amide directory: %s",save_filename);
      return;
    }
  }

  /* allright, save our study */
  if (amitk_study_save_xml(ui_study->study, save_filename) == FALSE) {
    g_warning("Failure Saving File: %s",save_filename);
    return;
  }

  /* close the file selection box */
  ui_common_file_selection_cancel_cb(widget, dir_selection);

  /* indicate no new changes */
  ui_study->study_altered=FALSE;
  ui_study_update_title(ui_study);

  return;
}

/* function to load in an amide xml file */
void ui_study_cb_save_as(GtkWidget * widget, gpointer data) {
  
  ui_study_t * ui_study = data;
  GtkWidget * dir_selection;
  gchar * temp_string;

  dir_selection = amitk_dir_selection_new(_("Save File"));

  /* take a guess at the filename */
  if (AMITK_STUDY_FILENAME(ui_study->study) == NULL) 
    temp_string = g_strdup_printf("%s.xif",AMITK_OBJECT_NAME(ui_study->study));
  else
    temp_string = g_strdup_printf("%s", AMITK_STUDY_FILENAME(ui_study->study));
  amitk_dir_selection_set_filename(AMITK_DIR_SELECTION(dir_selection), temp_string);
  g_free(temp_string); 

  /* save a pointer to ui_study */
  g_object_set_data(G_OBJECT(dir_selection), "ui_study", ui_study);

  /* don't want anything else going on till this window is gone */
  gtk_window_set_modal(GTK_WINDOW(dir_selection), TRUE);

  /* connect the signals */
  g_signal_connect(G_OBJECT(AMITK_DIR_SELECTION(dir_selection)->ok_button), "clicked",
		   G_CALLBACK(ui_study_cb_save_as_ok), dir_selection);
  g_signal_connect(G_OBJECT(AMITK_DIR_SELECTION(dir_selection)->cancel_button), "clicked",
		   G_CALLBACK(ui_common_file_selection_cancel_cb), dir_selection);
  g_signal_connect(G_OBJECT(AMITK_DIR_SELECTION(dir_selection)->cancel_button), "delete_event",
		   G_CALLBACK(ui_common_file_selection_cancel_cb), dir_selection);

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
  const gchar * import_filename=NULL;
  AmitkImportMethod import_method;
  int submethod = 0;
  AmitkDataSet * import_ds;

  /* get a pointer to ui_study */
  ui_study = g_object_get_data(G_OBJECT(file_selection), "ui_study");

  /* figure out how we want to import */
  import_method = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(file_selection), "method"));

#ifdef AMIDE_LIBMDC_SUPPORT
  /* figure out the submethod if we're loading through libmdc */
  if (import_method == AMITK_IMPORT_METHOD_LIBMDC) 
    submethod =  GPOINTER_TO_INT(g_object_get_data(G_OBJECT(file_selection), "submethod"));
#endif


  /* get the filename and import */
  import_filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_selection));
#ifdef AMIDE_DEBUG
  g_print("file to import: %s\n",import_filename);
#endif

  /* now, what we need to do if we've successfully loaded an image */
  ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);
  if ((import_ds = amitk_data_set_import_file(import_method, submethod, import_filename,
					      amitk_progress_dialog_update, ui_study->progress_dialog)) != NULL) {
#ifdef AMIDE_DEBUG
    g_print("imported data set name %s\n",AMITK_OBJECT_NAME(import_ds));
#endif
    amitk_object_add_child(AMITK_OBJECT(ui_study->study), 
			   AMITK_OBJECT(import_ds)); /* this adds a reference to the data set*/
    g_object_unref(import_ds); /* so remove a reference */
  }

  ui_common_remove_cursor(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);

  /* close the file selection box */
  ui_common_file_selection_cancel_cb(widget, file_selection);
  
  return;
}


/* function to selection which file to import */
void ui_study_cb_import(GtkWidget * widget, gpointer data) {
  
  ui_study_t * ui_study = data;
  GtkFileSelection * file_selection;
  AmitkImportMethod import_method;

  file_selection = GTK_FILE_SELECTION(gtk_file_selection_new(_("Import File")));

  /* save a pointer to ui_study */
  g_object_set_data(G_OBJECT(file_selection), "ui_study", ui_study);
  
  /* and save which method of importing we want to use */
  import_method = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "method"));
  g_object_set_data(G_OBJECT(file_selection), "method", GINT_TO_POINTER(import_method));
#ifdef AMIDE_LIBMDC_SUPPORT
  if (import_method == AMITK_IMPORT_METHOD_LIBMDC)
    g_object_set_data(G_OBJECT(file_selection), "submethod",
		      g_object_get_data(G_OBJECT(widget), "submethod"));
#endif

  /* don't want anything else going on till this window is gone */
  gtk_window_set_modal(GTK_WINDOW(file_selection), TRUE);

  /* connect the signals */
  g_signal_connect(G_OBJECT(file_selection->ok_button), "clicked",
		   G_CALLBACK(ui_study_cb_import_ok), file_selection);
  g_signal_connect(G_OBJECT(file_selection->cancel_button), "clicked",
		   G_CALLBACK(ui_common_file_selection_cancel_cb), file_selection);
  g_signal_connect(G_OBJECT(file_selection->cancel_button),"delete_event",
		   G_CALLBACK(ui_common_file_selection_cancel_cb), file_selection);

  /* set the position of the dialog */
  gtk_window_set_position(GTK_WINDOW(file_selection), GTK_WIN_POS_MOUSE);

  /* run the dialog */
  gtk_widget_show(GTK_WIDGET(file_selection));

  return;
}


/* function to handle exporting a view */
static void ui_study_cb_export_ok(GtkWidget* widget, gpointer data) {

  GtkWidget * file_selection = data;
  ui_study_t * ui_study;
  const gchar * save_filename;
  AmitkView  view;

  /* get a pointer to ui_study */
  ui_study = g_object_get_data(G_OBJECT(file_selection), "ui_study");

  /* figure out which view we're saving */
  view = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(file_selection), "view"));

  /* get the filename */
  if ((save_filename = ui_common_file_selection_get_name(file_selection)) == NULL)
    return; /* inappropriate name or don't want to overwrite */

  if (AMITK_CANVAS_PIXBUF(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][view]) == NULL) {
    g_warning("No data sets selected\n");
    return;
  }

  if (gdk_pixbuf_save (AMITK_CANVAS_PIXBUF(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][view]),
		       save_filename, "jpeg", NULL, 
		       "quality", "100", NULL) == FALSE) {
    g_warning("Failure Saving File: %s",save_filename);
    return;
  }

  /* close the file selection box */
  ui_common_file_selection_cancel_cb(widget, file_selection);

  return;
}

/* function to save a view as an external data format */
void ui_study_cb_export(GtkWidget * widget, gpointer data) {
  
  ui_study_t * ui_study = data;
  GList * current_data_sets;
  GList * temp_sets;
  GtkFileSelection * file_selection;
  gchar * temp_string;
  gchar * data_set_names = NULL;
  AmitkView view;
  amide_real_t upper, lower;
  AmitkPoint temp_p;
  AmitkVolume * canvas_volume;

  current_data_sets = amitk_object_get_selected_children_of_type(AMITK_OBJECT(ui_study->study), 
								 AMITK_OBJECT_TYPE_DATA_SET, 
								 AMITK_VIEW_MODE_SINGLE, TRUE);
  if (current_data_sets == NULL) return;

  file_selection = GTK_FILE_SELECTION(gtk_file_selection_new(_("Export File")));
  view = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "view"));

  if (ui_study->canvas[AMITK_VIEW_MODE_SINGLE][view] == NULL) return;

  canvas_volume = AMITK_CANVAS(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][view])->volume;
  g_return_if_fail(canvas_volume != NULL);

  /* translate the center so that the z coordinate corresponds to depth in this view */
  temp_p = amitk_space_b2s(AMITK_SPACE(canvas_volume), AMITK_STUDY_VIEW_CENTER(ui_study->study));

  /* figure out the top and bottom of this slice */
  upper = temp_p.z + AMITK_STUDY_VIEW_THICKNESS(ui_study->study)/2.0;
  lower = temp_p.z - AMITK_STUDY_VIEW_THICKNESS(ui_study->study)/2.0;

  /* take a guess at the filename */
  data_set_names = g_strdup(AMITK_OBJECT_NAME(current_data_sets->data));
  temp_sets = current_data_sets->next;
  while (temp_sets != NULL) {
    temp_string = g_strdup_printf("%s+%s",data_set_names, 
				  AMITK_OBJECT_NAME(temp_sets->data));
    g_free(data_set_names);
    data_set_names = temp_string;
    temp_sets = temp_sets->next;
  }
  temp_string = g_strdup_printf("%s_%s_%s_%3.1f-%3.1f.jpg", 
				AMITK_OBJECT_NAME(ui_study->study),
				data_set_names,
				view_names[view],
				lower,
				upper);
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selection), temp_string);
  g_free(data_set_names);
  g_free(temp_string); 

  /* save a pointer to ui_study, and record which view we're saving*/
  g_object_set_data(G_OBJECT(file_selection), "ui_study", ui_study);
  g_object_set_data(G_OBJECT(file_selection), "view", GINT_TO_POINTER(view));

  /* don't want anything else going on till this window is gone */
  gtk_window_set_modal(GTK_WINDOW(file_selection), TRUE);

  /* connect the signals */
  g_signal_connect(G_OBJECT(file_selection->ok_button), "clicked",
		   G_CALLBACK(ui_study_cb_export_ok), file_selection);
  g_signal_connect(G_OBJECT(file_selection->cancel_button), "clicked",
		   G_CALLBACK(ui_common_file_selection_cancel_cb), file_selection);
  g_signal_connect(G_OBJECT(file_selection->cancel_button), "delete_event",
		   G_CALLBACK(ui_common_file_selection_cancel_cb), file_selection);

  /* set the position of the dialog */
  gtk_window_set_position(GTK_WINDOW(file_selection), GTK_WIN_POS_MOUSE);

  /* run the dialog */
  gtk_widget_show(GTK_WIDGET(file_selection));

  amitk_objects_unref(current_data_sets);
  return;
}

/* callback generally attached to the entry_notify_event */
gboolean ui_study_cb_update_help_info(GtkWidget * widget, GdkEventCrossing * event, gpointer data) {
  ui_study_t * ui_study = data;
  AmitkHelpInfo which_info;

  which_info = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "which_help"));
  ui_study_update_help_info(ui_study, which_info, AMITK_STUDY_VIEW_CENTER(ui_study->study), 0.0);

  return FALSE;
}



void ui_study_cb_canvas_help_event(GtkWidget * canvas,  AmitkHelpInfo help_type,
				   AmitkPoint *location, amide_data_t value,  gpointer data) {
  ui_study_t * ui_study = data;
  ui_study_update_help_info(ui_study, help_type, *location, value);
  return;
}

void ui_study_cb_canvas_view_changing(GtkWidget * canvas, AmitkPoint *position,
				      amide_real_t thickness, gpointer data) {


  ui_study_t * ui_study = data;
  rgba_t outline_color;
  AmitkView i_view;
  AmitkViewMode i_view_mode;

  g_return_if_fail(ui_study->active_ds != NULL);
  g_return_if_fail(ui_study->study != NULL);

  /* update the other canvases accordingly */
  outline_color = amitk_color_table_outline_color(ui_study->active_ds->color_table, FALSE);
  for (i_view_mode=0; i_view_mode <= AMITK_STUDY_VIEW_MODE(ui_study->study); i_view_mode++)
    for (i_view=0; i_view<AMITK_VIEW_NUM; i_view++)
      if (ui_study->canvas[i_view_mode][i_view] != NULL)
	if (canvas != ui_study->canvas[i_view_mode][i_view])
	  amitk_canvas_update_target(AMITK_CANVAS(ui_study->canvas[i_view_mode][i_view]), 
				     AMITK_CANVAS_TARGET_ACTION_SHOW,*position, outline_color, thickness);

  ui_study_update_thickness(ui_study, thickness);

  return;
}

void ui_study_cb_canvas_view_changed(GtkWidget * canvas, AmitkPoint *position,
				     amide_real_t thickness, gpointer data) {

  ui_study_t * ui_study = data;
  AmitkView i_view;
  AmitkViewMode i_view_mode;

  g_return_if_fail(ui_study->study != NULL);

  amitk_study_set_view_thickness(ui_study->study, thickness);
  amitk_study_set_view_center(ui_study->study, *position);

  /* update the other canvases accordingly */
  for (i_view_mode=0; i_view_mode <= AMITK_STUDY_VIEW_MODE(ui_study->study); i_view_mode++) {
    for (i_view=0; i_view<AMITK_VIEW_NUM; i_view++)
      if (ui_study->canvas[i_view_mode][i_view] != NULL)
	amitk_canvas_update_target(AMITK_CANVAS(ui_study->canvas[i_view_mode][i_view]), 
				   AMITK_CANVAS_TARGET_ACTION_HIDE,*position, rgba_black, thickness);
  }

  return;
}


void ui_study_cb_canvas_isocontour_3d_changed(GtkWidget * canvas, AmitkRoi * roi,
					      AmitkPoint *position, gpointer data) {
  ui_study_t * ui_study = data;
  AmitkPoint temp_point;
  AmitkVoxel temp_voxel;
  GtkWidget * question;
  amide_time_t start_time, end_time;
  gint return_val;
  AmitkObject * parent_ds;

  g_return_if_fail(AMITK_IS_ROI(roi));
  g_return_if_fail(AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_ISOCONTOUR_3D);

  /* figure out the data set we're drawing on */
  parent_ds = amitk_object_get_parent_of_type(AMITK_OBJECT(roi), AMITK_OBJECT_TYPE_DATA_SET);
  if (parent_ds == NULL) {
    g_return_if_fail(ui_study->active_ds != NULL);
    parent_ds = g_object_ref(ui_study->active_ds);
  }

  start_time = AMITK_STUDY_VIEW_START_TIME(ui_study->study);
  end_time = start_time + AMITK_STUDY_VIEW_DURATION(ui_study->study);
  temp_point = amitk_space_b2s(AMITK_SPACE(parent_ds), *position);
  POINT_TO_VOXEL(temp_point, AMITK_DATA_SET_VOXEL_SIZE(parent_ds),
		 amitk_data_set_get_frame(AMITK_DATA_SET(parent_ds), start_time),
		 temp_voxel);

  if (!amitk_raw_data_includes_voxel(AMITK_DATA_SET_RAW_DATA(parent_ds),temp_voxel)) {
    g_warning("designanted voxel not in data set %s\n", AMITK_OBJECT_NAME(parent_ds));
    goto isocontour_3d_changed_end; /* cancel */
  }

  /* complain if more then one frame is currently showing */
  if (temp_voxel.t != amitk_data_set_get_frame(AMITK_DATA_SET(parent_ds), end_time)) {

    question = gtk_message_dialog_new(GTK_WINDOW(ui_study->app),
				      GTK_DIALOG_DESTROY_WITH_PARENT,
				      GTK_MESSAGE_QUESTION,
				      GTK_BUTTONS_OK_CANCEL,
				      "%s: %s\n%s %d",
				      "Multiple data frames are currently being shown from",
				      AMITK_OBJECT_NAME(parent_ds),
				      "The isocontour will only be drawn over frame",
				      temp_voxel.t);

    /* and wait for the question to return */
    return_val = gtk_dialog_run(GTK_DIALOG(question));

    gtk_widget_destroy(question);
    if (return_val != GTK_RESPONSE_OK) { 
      goto isocontour_3d_changed_end; /* cancel */
    }
  }

  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(canvas));
  amitk_roi_set_isocontour(roi, AMITK_DATA_SET(parent_ds), temp_voxel);
  ui_common_remove_cursor(GTK_WIDGET(canvas));

  isocontour_3d_changed_end:

  g_object_unref(parent_ds);

  return;
}


void ui_study_cb_canvas_erase_volume(GtkWidget * canvas, AmitkRoi * roi, 
				     gboolean outside, gpointer data) {
  ui_study_t * ui_study = data;
  GtkWidget * question;
  gint return_val;

  if (ui_study->active_ds == NULL) {
    g_warning("no active data set to erase from");
    return;
  }

  /* make sure we really want to delete */
  question = gtk_message_dialog_new(GTK_WINDOW(ui_study->app),
				    GTK_DIALOG_DESTROY_WITH_PARENT,
				    GTK_MESSAGE_QUESTION,
				    GTK_BUTTONS_OK_CANCEL,
				    "%s %s\n%s: %s\n%s: %s\n%s\n%s: %5.3f\n%s",
				     "Do you really wish to erase the data set",
				    outside ? "exterior" : "interior",
				    "    to the ROI", 
				    AMITK_OBJECT_NAME(roi),
				    "    on the data set: ",
				    AMITK_OBJECT_NAME(ui_study->active_ds),
				    "This step is irreversible",
				    "The minimum threshold value",
				    AMITK_DATA_SET_THRESHOLD_MIN(ui_study->active_ds,0),
				    "    will be used to fill in the volume");
  
  /* and wait for the question to return */
  return_val = gtk_dialog_run(GTK_DIALOG(question));

  gtk_widget_destroy(question);
  if (return_val != GTK_RESPONSE_OK)
    return; /* cancel */

  amitk_roi_erase_volume(roi, ui_study->active_ds, outside,
			 amitk_progress_dialog_update, ui_study->progress_dialog);
  
  return;
}

void ui_study_cb_canvas_new_object(GtkWidget * canvas, AmitkObject * parent, AmitkObjectType type,
				   AmitkPoint *position, gpointer data) {

  ui_study_t * ui_study = data;

  /* only handles fiducial marks currently */
  g_return_if_fail(type == AMITK_OBJECT_TYPE_FIDUCIAL_MARK); 

  ui_study_add_fiducial_mark(ui_study, parent, TRUE, *position);

  return;
}

void ui_study_cb_tree_activate_object(GtkWidget * tree, AmitkObject * object, gpointer data) {

  ui_study_t * ui_study = data;
  AmitkPoint center;

  if (object == NULL) {
    ui_study_make_active_data_set(ui_study, NULL);

  } else if (AMITK_IS_DATA_SET(object)) {
    ui_study_make_active_data_set(ui_study, AMITK_DATA_SET(object));

  } else if (AMITK_IS_ROI(object)) {
    center = amitk_volume_get_center(AMITK_VOLUME(object));
    if (!AMITK_ROI_UNDRAWN(AMITK_ROI(object)) && 
	!POINT_EQUAL(center, AMITK_STUDY_VIEW_CENTER(ui_study->study)))
      amitk_study_set_view_center(ui_study->study, center);
  } else if (AMITK_IS_FIDUCIAL_MARK(object)) {
    center = AMITK_FIDUCIAL_MARK_GET(object);
    if ( !POINT_EQUAL(center, AMITK_STUDY_VIEW_CENTER(ui_study->study)))
      amitk_study_set_view_center(ui_study->study, center);
  }
}


void ui_study_cb_tree_popup_object(GtkWidget * tree, AmitkObject * object, gpointer data) {

  ui_study_t * ui_study = data;
  GtkWidget * dialog;

  if (AMITK_IS_DATA_SET(object)) {
    ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);
    if (ui_study->threshold_dialog != NULL) {
      if (amitk_threshold_dialog_data_set(AMITK_THRESHOLD_DIALOG(ui_study->threshold_dialog)) == 
	  AMITK_DATA_SET(object)) {
	gtk_widget_destroy(ui_study->threshold_dialog);
	ui_study->threshold_dialog = NULL;
      }
    }
  }

  dialog = amitk_object_dialog_new(object, ui_study->canvas_layout);

  if (AMITK_IS_DATA_SET(object))
    ui_common_remove_cursor(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);

  if (dialog != NULL) 
    gtk_widget_show(dialog);
  else /* already up ,pop the window to the top */
    gtk_window_present(GTK_WINDOW(object->dialog));

  return;
}

void ui_study_cb_tree_add_object(GtkWidget * tree, AmitkObject * parent, 
				 AmitkObjectType object_type, AmitkRoiType roi_type, gpointer data) {
  ui_study_t * ui_study = data;

  if (parent == NULL)
    parent = AMITK_OBJECT(ui_study->study);

  if (object_type == AMITK_OBJECT_TYPE_FIDUCIAL_MARK)
    ui_study_add_fiducial_mark(ui_study, parent, TRUE,AMITK_STUDY_VIEW_CENTER(ui_study->study));
  else if (object_type == AMITK_OBJECT_TYPE_ROI)
    ui_study_add_roi(ui_study, parent, roi_type);


  return;
}

void ui_study_cb_tree_delete_object(GtkWidget * tree, AmitkObject * object, gpointer data) {
  ui_study_t * ui_study = data;
  GtkWidget * question;
  gint return_val;

  g_return_if_fail(AMITK_IS_OBJECT(object));

  /* check if it's okay to writeover the file */
  question = gtk_message_dialog_new(GTK_WINDOW(ui_study->app),
				    GTK_DIALOG_DESTROY_WITH_PARENT,
				    GTK_MESSAGE_QUESTION,
				    GTK_BUTTONS_OK_CANCEL,
				    "Do you really want to delete: %s%s",
				    AMITK_OBJECT_NAME(object),
				    AMITK_OBJECT_CHILDREN(object) != NULL ? 
				    "\nand it's children" : "");
  
  /* and wait for the question to return */
  return_val = gtk_dialog_run(GTK_DIALOG(question));

  gtk_widget_destroy(question);
  if (return_val != GTK_RESPONSE_OK)
    return; /* we don't want to overwrite the file.... */

  amitk_object_remove_child(AMITK_OBJECT_PARENT(object), object);

  if (ui_study->study_altered != TRUE) {
    ui_study->study_virgin=FALSE;
    ui_study->study_altered=TRUE;
    ui_study_update_title(ui_study);
  }

  return;
}


void ui_study_cb_tree_help_event(GtkWidget * widget, AmitkHelpInfo help_type, gpointer data) {
  ui_study_t * ui_study = data;
  ui_study_update_help_info(ui_study, help_type, zero_point, 0.0);
  return;
}


void ui_study_cb_zoom(GtkSpinButton * spin_button, gpointer data) {

  ui_study_t * ui_study = data;
  amide_real_t zoom;

  if (AMITK_OBJECT_CHILDREN(ui_study->study)==NULL) return;
  zoom = gtk_spin_button_get_value(spin_button);

  amitk_study_set_zoom(ui_study->study, zoom);
    
  return;
}


void ui_study_cb_thickness(GtkSpinButton * spin_button, gpointer data) {

  ui_study_t * ui_study = data;
  amide_real_t thickness;


  if (AMITK_OBJECT_CHILDREN(ui_study->study)==NULL) return;
  thickness = gtk_spin_button_get_value(spin_button);

  amitk_study_set_view_thickness(ui_study->study, thickness);
  
  return;
}



static gboolean time_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data) {
  ui_study_t * ui_study = data;

  /* just keeping track on whether or not the time widget is up */
  ui_study->time_dialog = NULL;

  return FALSE;
}

void ui_study_cb_time_pressed(GtkWidget * combo, gpointer data) {
  ui_study_t * ui_study = data;

  if (ui_study->time_dialog == NULL) {
    ui_study->time_dialog = ui_time_dialog_create(ui_study->study, GTK_WINDOW(ui_study->app));
    g_signal_connect(G_OBJECT(ui_study->time_dialog), "delete_event",
		     G_CALLBACK(time_delete_event), ui_study);
    gtk_widget_show(ui_study->time_dialog);

  } else /* pop the window to the top */
    gtk_window_present(GTK_WINDOW(ui_study->time_dialog));

  return;
}




/* callbacks for setting up a set of slices in a new window */
void ui_study_cb_series(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  AmitkView view;
  series_t series_type;
  
  view = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "view"));
  series_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "series_type"));

  ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);
  ui_series_create(ui_study->study, 
		   ui_study->active_ds,
		   view, 
		   AMITK_CANVAS(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][view])->volume,
		   ui_study->roi_width, ui_study->line_style,
		   series_type);
  ui_common_remove_cursor(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);

  return;
}

#ifdef AMIDE_LIBFAME_SUPPORT
void ui_study_cb_fly_through(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  AmitkView view;
  
  view = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "view"));

  tb_fly_through(ui_study->study, view,  
		 ui_study->canvas_layout, GTK_WINDOW(ui_study->app));

  return;
}
#endif

#ifdef AMIDE_LIBVOLPACK_SUPPORT
/* callback for starting up volume rendering */
void ui_study_cb_render(GtkWidget * widget, gpointer data) {

  GtkWidget * dialog;
  ui_study_t * ui_study = data;
  gint return_val;

  /* need something to render */
  if (!amitk_object_selected_children(AMITK_OBJECT(ui_study->study),
				      AMITK_VIEW_MODE_SINGLE, TRUE)) 
    return;

  /* let the user input rendering options */
  dialog = ui_render_init_dialog_create(GTK_WINDOW(ui_study->app));

  /* and wait for the question to return */
  return_val = gtk_dialog_run(GTK_DIALOG(dialog));

  gtk_widget_destroy(dialog);
  if (return_val != AMITK_RESPONSE_EXECUTE)
    return; /* we don't want to render */

  ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);
  ui_render_create(ui_study->study);
  ui_common_remove_cursor(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);

  return;
}
#endif


/* do roi calculations */
void ui_study_cb_roi_statistics(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  GtkWidget * dialog;
  gint return_val;

  /* let the user input roi analysis options */
  dialog = tb_roi_analysis_init_dialog(GTK_WINDOW(ui_study->app));

  /* and wait for the question to return */
  return_val = gtk_dialog_run(GTK_DIALOG(dialog));

  gtk_widget_destroy(dialog);
  if (return_val != AMITK_RESPONSE_EXECUTE)
    return; /* we hit cancel */

  ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);
  tb_roi_analysis(ui_study->study, GTK_WINDOW(ui_study->app));
  ui_common_remove_cursor(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);

  return;
}

/* user wants to run the alignment wizard */
void ui_study_cb_alignment_selected(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  tb_alignment(ui_study->study);
  return;
}

/* user wants to run the crop wizard */
void ui_study_cb_crop_selected(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  tb_crop(ui_study->study, ui_study->active_ds);
  return;
}

/* user wants to run the fads wizard */
void ui_study_cb_fads_selected(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  tb_fads(ui_study->active_ds);
  return;
}

/* user wants to run the filter wizard */
void ui_study_cb_filter_selected(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  tb_filter(ui_study->study, ui_study->active_ds);
  return;
}

static gboolean threshold_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data) {
  ui_study_t * ui_study = data;

  /* just keeping track on whether or not the threshold widget is up */
  ui_study->threshold_dialog = NULL;

  return FALSE;
}

/* function called when hitting the threshold button, pops up a dialog */
void ui_study_cb_threshold_pressed(GtkWidget * button, gpointer data) {

  ui_study_t * ui_study = data;

  if (ui_study->active_ds == NULL) return;

  /* make sure we don't already have a data set edit dialog up */
  if ((AMITK_OBJECT(ui_study->active_ds)->dialog == NULL) &&
      (ui_study->threshold_dialog == NULL)) {
    ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);
    ui_study->threshold_dialog = amitk_threshold_dialog_new(ui_study->active_ds,
							    GTK_WINDOW(ui_study->app));
    ui_common_remove_cursor(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);
    g_signal_connect(G_OBJECT(ui_study->threshold_dialog), "delete_event",
		     G_CALLBACK(threshold_delete_event), ui_study);
    gtk_widget_show(GTK_WIDGET(ui_study->threshold_dialog));
  } else {
    /* pop the window to the top */
    if (AMITK_OBJECT(ui_study->active_ds)->dialog)
      gtk_window_present(GTK_WINDOW(AMITK_OBJECT(ui_study->active_ds)->dialog));
    else
      gtk_window_present(GTK_WINDOW(ui_study->threshold_dialog));
  }

  return;
}

/* callback function for adding an roi */
void ui_study_cb_add_roi(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  AmitkRoiType roi_type;

  /* figure out which menu item called me */
  roi_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"roi_type"));
  ui_study_add_roi(ui_study, AMITK_OBJECT(ui_study->study), roi_type);

  return;
}


/* callback function for adding a fiducial mark */
void ui_study_cb_add_fiducial_mark(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  if (ui_study->active_ds == NULL) return;
  ui_study_add_fiducial_mark(ui_study, AMITK_OBJECT(ui_study->active_ds), TRUE,
			     AMITK_STUDY_VIEW_CENTER(ui_study->study));
  return;
}

/* callback function for changing user's preferences */
void ui_study_cb_preferences(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study=data;
  if (ui_study->preferences_dialog == NULL)
    ui_study->preferences_dialog = ui_preferences_dialog_create(ui_study);
  return;
}


/* function to switch the interpolation method */
void ui_study_cb_interpolation(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  AmitkInterpolation interpolation;

  g_return_if_fail(ui_study->active_ds != NULL);

  /* figure out which interpolation menu item called me */
  interpolation = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"interpolation"));

  amitk_data_set_set_interpolation(ui_study->active_ds,  interpolation);
				
  return;
}

void ui_study_cb_study_changed(AmitkStudy * study, gpointer data) {

  ui_study_t * ui_study = data;

  ui_study_update_thickness(ui_study, AMITK_STUDY_VIEW_THICKNESS(ui_study->study));
  ui_study_update_time_button(ui_study->study, ui_study->time_button);
  ui_study_update_canvas_visible_buttons(ui_study);
  ui_study_update_layout(ui_study);
  
  return;
}

void ui_study_cb_study_view_mode_changed(AmitkStudy * study, gpointer data) {

  ui_study_t * ui_study = data;

  ui_study_update_layout(ui_study);

  return;
}

/* function to switch the image fusion type */
void ui_study_cb_fuse_type(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  AmitkFuseType fuse_type;

  /* figure out which fuse_type menu item called me */
  fuse_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"fuse_type"));

  amitk_study_set_fuse_type(ui_study->study,  fuse_type);
				
  return;
}

void ui_study_cb_canvas_visible(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  AmitkView view;

  /* figure out which view item called me */
  view = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"view"));
  amitk_study_set_canvas_visible(ui_study->study, view, 
				 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
  ui_study_update_canvas_visible_buttons(ui_study);


  return;
}

void ui_study_cb_view_mode(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;

  amitk_study_set_view_mode(ui_study->study, 
			    GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"view_mode")));
  return;
}


/* function ran to exit the program */
void ui_study_cb_exit(GtkWidget* widget, gpointer data) {

  ui_study_t * ui_study = data;
  GtkWidget * app = GTK_WIDGET(ui_study->app);
  gboolean return_val;

  /* run the delete event function */
  g_signal_emit_by_name(G_OBJECT(app), "delete_event", NULL, &return_val);

  if (!return_val) { /* if we don't cancel the quit */
    gtk_widget_destroy(app);
    amide_unregister_all_windows(); /* kill everything */
  }

  return;
}

/* function ran when closing a study window */
void ui_study_cb_close(GtkWidget* widget, gpointer data) {

  ui_study_t * ui_study = data;
  GtkWidget * app = GTK_WIDGET(ui_study->app);
  gboolean return_val;

  /* run the delete event function */
  g_signal_emit_by_name(G_OBJECT(app), "delete_event", NULL, &return_val);
  if (!return_val) gtk_widget_destroy(app);

  return;
}

/* function to run for a delete_event */
gboolean ui_study_cb_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_study_t * ui_study = data;
  GtkWidget * app = ui_study->app;
  GtkWidget * exit_dialog;
  gint return_val;

  /* check to see if we need saving */
  if ((ui_study->study_altered == TRUE) && 
      !(ui_study->dont_prompt_for_save_on_exit)) {

    exit_dialog = gtk_message_dialog_new(GTK_WINDOW(ui_study->app),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_OK_CANCEL,
					 "There are unsaved changes to the study.\nAre you sure you wish to quit?");

    /* and wait for the question to return */
    return_val = gtk_dialog_run(GTK_DIALOG(exit_dialog));

    gtk_widget_destroy(exit_dialog);
    if (return_val != GTK_RESPONSE_OK)
      return TRUE; /* cancel */

  }
  
  ui_study = ui_study_free(ui_study); /* free our data structure's */
  amide_unregister_window((gpointer) app); /* tell the rest of the program this window's gone */

  return FALSE;
}

