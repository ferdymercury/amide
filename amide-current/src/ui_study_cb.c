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
#include "ui_volume_dialog.h"
#include "ui_roi_analysis_dialog.h"

/* function which gets called from a name entry dialog */
static void ui_study_cb_entry_name(gchar * entry_string, gpointer data) {
  gchar ** p_roi_name = data;
  *p_roi_name = entry_string;
  return;
}

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
  import_method = 
    GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(file_selection), "method"));

  /* we need some more info if we're doing a PEM file */
  if (import_method == PEM_DATA) {

    /* get the name of the model data file */
    entry = gnome_request_dialog(FALSE, 
				 "Corresponding Model File, if any?", "", 
				 256,  ui_study_cb_entry_name,
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
  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_study->canvas[0]));
  ui_study_import_file(ui_study, import_method, submethod, import_filename, model_filename);
  ui_common_remove_cursor(GTK_WIDGET(ui_study->canvas[0]));

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
  if (import_method == LIBMDC_DATA)
    gtk_object_set_data(GTK_OBJECT(file_selection), "submethod",
			gtk_object_get_data(GTK_OBJECT(widget), "submethod"));

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
  pixels = gdk_pixbuf_get_pixels(ui_study->canvas_rgb[view]);

  /* yep, we still need to use imlib for the moment for generating output images,
     maybe gdk_pixbuf will have this functionality at some point */
  export_image = gdk_imlib_create_image_from_data(pixels, NULL, 
						  gdk_pixbuf_get_width(ui_study->canvas_rgb[view]),
						  gdk_pixbuf_get_height(ui_study->canvas_rgb[view]));
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
  ui_volume_list_t * current_volumes;
  GtkFileSelection * file_selection;
  gchar * temp_string;
  gchar * data_set_names = NULL;
  view_t view;
  floatpoint_t upper, lower;
  realpoint_t view_center;
  realpoint_t temp_p;

  /* sanity checks */
  if (study_volumes(ui_study->study) == NULL)
    return;
  if (ui_study->current_volume == NULL)
    return;
  if (ui_study->current_volumes == NULL)
    return;

  file_selection = GTK_FILE_SELECTION(gtk_file_selection_new(_("Export File")));
  view = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "view"));

  /* get the center */
  view_center = study_view_center(ui_study->study);
  /* translate the point so that the z coordinate corresponds to depth in this view */
  temp_p = realspace_alt_coord_to_alt(view_center,
				      study_coord_frame(ui_study->study),
				      ui_study->canvas_coord_frame[view]);

  /* figure out the top and bottom of this slice */
  upper = temp_p.z + study_view_thickness(ui_study->study)/2.0;
  lower = temp_p.z - study_view_thickness(ui_study->study)/2.0;

  /* take a guess at the filename */
  current_volumes = ui_study->current_volumes;
  data_set_names = g_strdup(current_volumes->volume->name);
  current_volumes = current_volumes->next;
  while (current_volumes != NULL) {
    temp_string = g_strdup_printf("%s+%s",data_set_names, current_volumes->volume->name);
    g_free(data_set_names);
    data_set_names = temp_string;
    current_volumes = current_volumes->next;
  }
  temp_string = g_strdup_printf("%s_{%s}_%s:%3.1f-%3.1f.jpg", 
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
  ui_study_help_info_t which_info;

  which_info = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "which_help"));

  /* quick correction so we can handle leaving a tree leaf */
  if (((which_info == HELP_INFO_TREE_VOLUME) ||
       (which_info == HELP_INFO_TREE_ROI) ||
       (which_info == HELP_INFO_TREE_STUDY)) &&
      (event->type == GDK_LEAVE_NOTIFY))
    which_info = HELP_INFO_TREE_NONE;

  ui_study_update_help_info(ui_study, which_info, realpoint_zero, 0.0);

  return FALSE;
}



/* function called when an event occurs on the image canvas, 
   notes:
   -events for non-new roi's are handled by ui_study_rois_cb_roi_event 

*/
gboolean ui_study_cb_canvas_event(GtkWidget* widget,  GdkEvent * event, gpointer data) {

  ui_study_t * ui_study = data;
  GnomeCanvas * canvas;
  realpoint_t base_rp, view_rp, canvas_rp, diff_rp;
  view_t view;
  canvaspoint_t canvas_cp, diff_cp, event_cp;
  rgba_t outline_color;
  GnomeCanvasPoints * points;
  ui_study_canvas_event_t canvas_event_type;
  ui_study_canvas_object_t object_type;
  roi_t * roi;
  canvaspoint_t temp_cp[2];
  realpoint_t temp_rp[2];
  static GnomeCanvasItem * canvas_item = NULL;
  static gboolean grab_on = FALSE;
  static gboolean extended_mode = FALSE;
  static gboolean depth_change = FALSE;
  static gboolean align_vertical = FALSE;
  static canvaspoint_t initial_cp;
  static canvaspoint_t previous_cp;
  static realpoint_t initial_view_rp;
  static realpoint_t initial_base_rp;
  static realpoint_t initial_canvas_rp;
  static double theta;
  static realpoint_t roi_zoom;
  static realpoint_t shift;
  static realpoint_t roi_center_rp;
  static realpoint_t roi_radius_rp;
  static gboolean in_roi=FALSE;

  /* sanity checks */
  if (study_volumes(ui_study->study) == NULL) return TRUE;
  if (ui_study->current_volume == NULL) return TRUE;

  /* figure out which view we're on, get our canvas, and type of object */ 
  view = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "view"));
  canvas = ui_study->canvas[view];
  object_type = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "object_type"));

  /* get the roi if appropriate */
  if (object_type != OBJECT_ROI_TYPE) roi = NULL;
  else roi = gtk_object_get_data(GTK_OBJECT(widget), "roi");

  switch(event->type) {
    
  case GDK_ENTER_NOTIFY:
    event_cp.x = event->crossing.x;
    event_cp.y = event->crossing.y;
    if (event->crossing.mode != GDK_CROSSING_NORMAL)
      canvas_event_type = UI_STUDY_EVENT_DO_NOTHING; /* ignore grabs */

    else if (ui_study->undrawn_roi != NULL) {
      if ((ui_study->undrawn_roi->type == ISOCONTOUR_2D) || (ui_study->undrawn_roi->type == ISOCONTOUR_3D)) 
	canvas_event_type = UI_STUDY_EVENT_ENTER_NEW_ISOCONTOUR_ROI;      
      else canvas_event_type = UI_STUDY_EVENT_ENTER_NEW_NORMAL_ROI;      
      
    } else if (object_type == OBJECT_VOLUME_TYPE) {
      canvas_event_type = UI_STUDY_EVENT_ENTER_VOLUME;
      
    } else if (roi != NULL) { /* sanity check */
      if ((roi->type == ISOCONTOUR_2D) || (roi->type == ISOCONTOUR_3D))
	canvas_event_type = UI_STUDY_EVENT_ENTER_ISOCONTOUR_ROI;
      else  canvas_event_type = UI_STUDY_EVENT_ENTER_NORMAL_ROI;
      
    } else
      canvas_event_type = UI_STUDY_EVENT_DO_NOTHING;
    
    break;
    
  case GDK_LEAVE_NOTIFY:
    event_cp.x = event->crossing.x;
    event_cp.y = event->crossing.y;
    if (event->crossing.mode != GDK_CROSSING_NORMAL)
      canvas_event_type = UI_STUDY_EVENT_DO_NOTHING; /* ignore grabs */
    else
      canvas_event_type = UI_STUDY_EVENT_LEAVE;
    break;
    
  case GDK_BUTTON_PRESS:
    event_cp.x = event->button.x;
    event_cp.y = event->button.y;
    if (ui_study->undrawn_roi != NULL) {
      canvas_event_type = UI_STUDY_EVENT_PRESS_NEW_ROI;
      
    } else if (extended_mode) {
      if (align_vertical) canvas_event_type = UI_STUDY_EVENT_PRESS_ALIGN_VERTICAL;
      else canvas_event_type = UI_STUDY_EVENT_PRESS_ALIGN_HORIZONTAL;

    } else if ((!grab_on) && (!in_roi) && (object_type == OBJECT_VOLUME_TYPE) && (event->button.button == 1)) {
      if (event->button.state & GDK_SHIFT_MASK) canvas_event_type = UI_STUDY_EVENT_PRESS_ALIGN_HORIZONTAL;
      else canvas_event_type = UI_STUDY_EVENT_PRESS_MOVE_VIEW;
      
    } else if ((!grab_on) && (!in_roi) && (object_type == OBJECT_VOLUME_TYPE) && (event->button.button == 2)) {
      if (event->button.state & GDK_SHIFT_MASK) canvas_event_type = UI_STUDY_EVENT_PRESS_ALIGN_VERTICAL;
      else canvas_event_type = UI_STUDY_EVENT_PRESS_MINIMIZE_VIEW;
      
    } else if ((!grab_on) && (!in_roi) && (object_type == OBJECT_VOLUME_TYPE) && (event->button.button == 3)) {
      if (event->button.state & GDK_SHIFT_MASK) canvas_event_type = UI_STUDY_EVENT_DO_NOTHING;
      else canvas_event_type = UI_STUDY_EVENT_PRESS_RESIZE_VIEW;
      
    } else if ((!grab_on) && (object_type == OBJECT_ROI_TYPE) && (event->button.button == 1)) {
      canvas_event_type = UI_STUDY_EVENT_PRESS_SHIFT_ROI;
      
    } else if ((!grab_on) && (object_type == OBJECT_ROI_TYPE) && (event->button.button == 2)) {
      if ((roi->type != ISOCONTOUR_2D) && (roi->type != ISOCONTOUR_3D))
	canvas_event_type = UI_STUDY_EVENT_PRESS_ROTATE_ROI;
      else if (event->button.state & GDK_SHIFT_MASK)
	canvas_event_type = UI_STUDY_EVENT_PRESS_LARGE_ERASE_ISOCONTOUR;
      else
	canvas_event_type = UI_STUDY_EVENT_PRESS_ERASE_ISOCONTOUR;
      
    } else if ((!grab_on) && (object_type == OBJECT_ROI_TYPE) && (event->button.button == 3)) {
      if ((roi->type != ISOCONTOUR_2D) && (roi->type != ISOCONTOUR_3D))
	canvas_event_type = UI_STUDY_EVENT_PRESS_RESIZE_ROI;
      else canvas_event_type = UI_STUDY_EVENT_PRESS_CHANGE_ISOCONTOUR;
      
    } else 
      canvas_event_type = UI_STUDY_EVENT_DO_NOTHING;
    
    break;
    
  case GDK_MOTION_NOTIFY:
    event_cp.x = event->motion.x;
    event_cp.y = event->motion.y;
    if (grab_on && (ui_study->undrawn_roi != NULL)) {
      canvas_event_type = UI_STUDY_EVENT_MOTION_NEW_ROI;
      
    } else if (extended_mode ) {
      if (align_vertical) canvas_event_type = UI_STUDY_EVENT_MOTION_ALIGN_VERTICAL;
      else canvas_event_type = UI_STUDY_EVENT_MOTION_ALIGN_HORIZONTAL;
      
    } else if (grab_on && (!in_roi) && (object_type == OBJECT_VOLUME_TYPE) && (event->motion.state & GDK_BUTTON1_MASK)) {
      canvas_event_type = UI_STUDY_EVENT_MOTION_MOVE_VIEW;
      
    } else if (grab_on && (!in_roi) && (object_type == OBJECT_VOLUME_TYPE) && (event->motion.state & GDK_BUTTON2_MASK)) {
      canvas_event_type = UI_STUDY_EVENT_MOTION_MINIMIZE_VIEW;
      
    } else if (grab_on && (!in_roi) && (object_type == OBJECT_VOLUME_TYPE) && (event->motion.state & GDK_BUTTON3_MASK)) {
      canvas_event_type = UI_STUDY_EVENT_MOTION_RESIZE_VIEW;
      
    } else if (grab_on && (object_type == OBJECT_ROI_TYPE) && (event->motion.state & GDK_BUTTON1_MASK)) {
      canvas_event_type = UI_STUDY_EVENT_MOTION_SHIFT_ROI;
      
    } else if (grab_on && (object_type == OBJECT_ROI_TYPE) && (event->motion.state & GDK_BUTTON2_MASK)) {
      if ((roi->type != ISOCONTOUR_2D) && (roi->type != ISOCONTOUR_3D)) 
	canvas_event_type = UI_STUDY_EVENT_MOTION_ROTATE_ROI;
      else if (event->motion.state & GDK_SHIFT_MASK)
	canvas_event_type = UI_STUDY_EVENT_MOTION_LARGE_ERASE_ISOCONTOUR;
      else
	canvas_event_type = UI_STUDY_EVENT_MOTION_ERASE_ISOCONTOUR;
      
    } else if (grab_on && (object_type == OBJECT_ROI_TYPE) && (event->motion.state & GDK_BUTTON3_MASK)) {
      if ((roi->type != ISOCONTOUR_2D) && (roi->type != ISOCONTOUR_3D))
	canvas_event_type = UI_STUDY_EVENT_MOTION_RESIZE_ROI;
      else canvas_event_type = UI_STUDY_EVENT_MOTION_CHANGE_ISOCONTOUR;
      
    } else if (!in_roi) {
      canvas_event_type = UI_STUDY_EVENT_MOTION;
    
    } else
      canvas_event_type = UI_STUDY_EVENT_DO_NOTHING;

    break;
    
  case GDK_BUTTON_RELEASE:
    event_cp.x = event->button.x;
    event_cp.y = event->button.y;
    if (ui_study->undrawn_roi != NULL) {
	canvas_event_type = UI_STUDY_EVENT_RELEASE_NEW_ROI;
	
    } else if (extended_mode && (!grab_on) && (event->button.button == 3)) {
      if (align_vertical) canvas_event_type = UI_STUDY_EVENT_ENACT_ALIGN_VERTICAL;
      else canvas_event_type = UI_STUDY_EVENT_ENACT_ALIGN_HORIZONTAL;
      
    } else if (extended_mode && (!grab_on))  {
      if (align_vertical) canvas_event_type = UI_STUDY_EVENT_CANCEL_ALIGN_VERTICAL;
      else canvas_event_type = UI_STUDY_EVENT_CANCEL_ALIGN_HORIZONTAL;

    } else if (extended_mode && (grab_on)) {
      if (align_vertical) canvas_event_type = UI_STUDY_EVENT_RELEASE_ALIGN_VERTICAL;
      else canvas_event_type = UI_STUDY_EVENT_RELEASE_ALIGN_HORIZONTAL;
      
    } else if ((!in_roi) && (object_type == OBJECT_VOLUME_TYPE) && (event->button.button == 1)) {
      canvas_event_type = UI_STUDY_EVENT_RELEASE_MOVE_VIEW;
	
    } else if ((!in_roi) && (object_type == OBJECT_VOLUME_TYPE) && (event->button.button == 2)) {
      canvas_event_type = UI_STUDY_EVENT_RELEASE_MINIMIZE_VIEW;
      
    } else if ((!in_roi) && (object_type == OBJECT_VOLUME_TYPE) && (event->button.button == 3)) {
      canvas_event_type = UI_STUDY_EVENT_RELEASE_RESIZE_VIEW;
      
    } else if ((object_type == OBJECT_ROI_TYPE) && (event->button.button == 1)) {
      canvas_event_type = UI_STUDY_EVENT_RELEASE_SHIFT_ROI;
      
    } else if ((object_type == OBJECT_ROI_TYPE) && (event->button.button == 2)) {
      if ((roi->type != ISOCONTOUR_2D) && (roi->type != ISOCONTOUR_3D))
	canvas_event_type = UI_STUDY_EVENT_RELEASE_ROTATE_ROI;
      else if (event->button.state & GDK_SHIFT_MASK)
	canvas_event_type = UI_STUDY_EVENT_RELEASE_LARGE_ERASE_ISOCONTOUR;
      else 
	canvas_event_type = UI_STUDY_EVENT_RELEASE_ERASE_ISOCONTOUR;      

    } else if ((object_type == OBJECT_ROI_TYPE) && (event->button.button == 3)) {
      if ((roi->type != ISOCONTOUR_2D) && (roi->type != ISOCONTOUR_3D))
	canvas_event_type = UI_STUDY_EVENT_RELEASE_RESIZE_ROI;
      else canvas_event_type = UI_STUDY_EVENT_RELEASE_CHANGE_ISOCONTOUR;
      
    } else 
      canvas_event_type = UI_STUDY_EVENT_DO_NOTHING;
    
    break;
    
  default: 
    event_cp.x = event_cp.y = 0;
    /* an event we don't handle */
    canvas_event_type = UI_STUDY_EVENT_DO_NOTHING;
    break;
    
  }

  /* get the location of the event, and convert it to the canvas coordinates */
  gnome_canvas_w2c_d(canvas, event_cp.x, event_cp.y, &canvas_cp.x, &canvas_cp.y);

  /* Convert the event location info to real units */
  canvas_rp = ui_study_cp_2_rp(ui_study, view, canvas_cp);
  base_rp = realspace_alt_coord_to_base(canvas_rp, ui_study->canvas_coord_frame[view]);
  view_rp = realspace_base_coord_to_alt(base_rp, study_coord_frame(ui_study->study));


  //  if ((event->type == 3) && (canvas_event_type == 40))
  //    ;
  //  else
  //    g_print("gdk %2d canvas %2d grab_on %d button %d mode %d in_roi %d widget %p extended %d\n",
  //	    event->type, canvas_event_type, grab_on, event->button.button, object_type, in_roi,
  //	  widget, extended_mode );


  switch (canvas_event_type) {
    
  case UI_STUDY_EVENT_ENTER_VOLUME:
    ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_VOLUME, base_rp, 0.0);
    ui_common_place_cursor(UI_CURSOR_VOLUME_MODE, GTK_WIDGET(canvas));
    break;

    
  case UI_STUDY_EVENT_ENTER_NEW_NORMAL_ROI:
    in_roi = TRUE;
    ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_NEW_ROI, base_rp, 0.0);
    ui_common_place_cursor(UI_CURSOR_NEW_ROI_MODE, GTK_WIDGET(canvas));
    break;

    
  case UI_STUDY_EVENT_ENTER_NEW_ISOCONTOUR_ROI:
    in_roi = TRUE;
    ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_NEW_ISOCONTOUR_ROI, base_rp, 0.0);
    ui_common_place_cursor(UI_CURSOR_NEW_ROI_MODE, GTK_WIDGET(canvas));
    break;

    
  case UI_STUDY_EVENT_ENTER_NORMAL_ROI:
    in_roi = TRUE;
    ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_ROI, base_rp, 0.0);
    ui_common_place_cursor(UI_CURSOR_OLD_ROI_MODE, GTK_WIDGET(canvas));
    break;

    
  case UI_STUDY_EVENT_ENTER_ISOCONTOUR_ROI:
    in_roi = TRUE;
    ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_ISOCONTOUR_ROI, base_rp, 0.0);
    ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_VALUE, base_rp, roi->isocontour_value);
    ui_common_place_cursor(UI_CURSOR_OLD_ROI_MODE, GTK_WIDGET(canvas));
    break;
    
    
  case UI_STUDY_EVENT_LEAVE:
    ui_common_remove_cursor(GTK_WIDGET(canvas)); 

    /* yes, do it twice if we're leaving the canvas, there's a bug where if the canvas gets 
       covered by a menu, no GDK_LEAVE_NOTIFY is called for the GDK_ENTER_NOTIFY */
    if (object_type == OBJECT_VOLUME_TYPE) ui_common_remove_cursor(GTK_WIDGET(canvas));

    /* yep, we don't seem to get an EVENT_ENTER corresponding to the EVENT_LEAVE for a canvas_item */
    if (in_roi) ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_VOLUME, base_rp, 0.0);
    in_roi = FALSE;
    break;

    
  case UI_STUDY_EVENT_PRESS_MINIMIZE_VIEW:
    depth_change = TRUE;
    study_set_view_thickness(ui_study->study, 0);
    ui_study_update_thickness_adjustment(ui_study);
  case UI_STUDY_EVENT_PRESS_MOVE_VIEW:
  case UI_STUDY_EVENT_PRESS_RESIZE_VIEW:
    ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, base_rp, 0.0);
    grab_on = TRUE;
    initial_view_rp = view_rp;
    outline_color = color_table_outline_color(ui_study->current_volume->color_table, FALSE);
    ui_study_update_targets(ui_study, TARGET_CREATE, view_rp, outline_color);
    break;

    
  case UI_STUDY_EVENT_PRESS_ALIGN_VERTICAL:
  case UI_STUDY_EVENT_PRESS_ALIGN_HORIZONTAL:
    ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_ALIGN, base_rp, 0.0);
    if (extended_mode) {
      grab_on = FALSE; /* do everything on the BUTTON_RELEASE */
    } else {
      grab_on = TRUE;
      initial_view_rp = view_rp;
      extended_mode = TRUE;
      initial_cp = canvas_cp;

      points = gnome_canvas_points_new(2);
      points->coords[2] = points->coords[0] = canvas_cp.x;
      points->coords[3] = points->coords[1] = canvas_cp.y;
      outline_color = color_table_outline_color(ui_study->current_volume->color_table, TRUE);
      canvas_item = gnome_canvas_item_new(gnome_canvas_root(canvas), gnome_canvas_line_get_type(),
					  "points", points, "width_pixels", ui_study->roi_width,
					  "fill_color_rgba", color_table_rgba_to_uint32(outline_color),
					  NULL);
      gnome_canvas_points_unref(points);

      align_vertical = (canvas_event_type == UI_STUDY_EVENT_PRESS_ALIGN_VERTICAL);
    }
    break;

    
  case UI_STUDY_EVENT_PRESS_NEW_ROI:
    ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_NEW_ROI, base_rp, 0.0);
    outline_color = color_table_outline_color(ui_study->current_volume->color_table, TRUE);
    initial_canvas_rp = canvas_rp;
    initial_cp = canvas_cp;
    
    /* create the new roi */
    switch(ui_study->undrawn_roi->type) {
    case BOX:
      canvas_item = 
	gnome_canvas_item_new(gnome_canvas_root(canvas),
			      gnome_canvas_rect_get_type(),
			      "x1",canvas_cp.x, "y1", canvas_cp.y, 
			      "x2", canvas_cp.x, "y2", canvas_cp.y,
			      "fill_color", NULL,
			      "outline_color_rgba", color_table_rgba_to_uint32(outline_color),
			      "width_pixels", ui_study->roi_width,
			      NULL);
      grab_on = TRUE;
      gnome_canvas_item_grab(GNOME_CANVAS_ITEM(canvas_item),
			     GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			     ui_common_cursor[UI_CURSOR_NEW_ROI_MOTION],
			     event->button.time);
      break;
    case ELLIPSOID:
    case CYLINDER:
      canvas_item = 
	gnome_canvas_item_new(gnome_canvas_root(canvas),
			      gnome_canvas_ellipse_get_type(),
			      "x1", canvas_cp.x, "y1", canvas_cp.y, 
			      "x2", canvas_cp.x, "y2", canvas_cp.y,
			      "fill_color", NULL,
			      "outline_color_rgba", color_table_rgba_to_uint32(outline_color),
			      "width_pixels", ui_study->roi_width,
			      NULL);
      grab_on = TRUE;
      gnome_canvas_item_grab(GNOME_CANVAS_ITEM(canvas_item),
			     GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			     ui_common_cursor[UI_CURSOR_NEW_ROI_MOTION],
			     event->button.time);
      break;
    case ISOCONTOUR_2D:
    case ISOCONTOUR_3D:
      grab_on = TRUE;
      break;
    default:
      grab_on = FALSE;
      g_warning("%s: unexpected case in %s at line %d, roi_type %d", 
		PACKAGE, __FILE__, __LINE__, ui_study->undrawn_roi->type);
      break;
    }
    break;


  case UI_STUDY_EVENT_PRESS_SHIFT_ROI:
    previous_cp = canvas_cp;
  case UI_STUDY_EVENT_PRESS_ROTATE_ROI:
  case UI_STUDY_EVENT_PRESS_RESIZE_ROI:
    if ((roi->type == ISOCONTOUR_2D) || (roi->type == ISOCONTOUR_3D))
      ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_ISOCONTOUR_ROI, base_rp, 0.0);
    else
      ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_ROI, base_rp, 0.0);
    grab_on = TRUE;
    canvas_item = GNOME_CANVAS_ITEM(widget); 
    if (canvas_event_type == UI_STUDY_EVENT_PRESS_SHIFT_ROI)
      gnome_canvas_item_grab(canvas_item,
			     GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			     ui_common_cursor[UI_CURSOR_OLD_ROI_SHIFT], event->button.time);
    else if (canvas_event_type == UI_STUDY_EVENT_PRESS_ROTATE_ROI)
      gnome_canvas_item_grab(canvas_item,
			     GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			     ui_common_cursor[UI_CURSOR_OLD_ROI_ROTATE], event->button.time);
    else /* UI_STUDY_EVENT_PRESS_RESIZE_ROI */
      gnome_canvas_item_grab(canvas_item,
			     GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			     ui_common_cursor[UI_CURSOR_OLD_ROI_RESIZE], event->button.time);
    initial_base_rp = base_rp;
    initial_cp = canvas_cp;
    initial_canvas_rp = canvas_rp;
    theta = 0.0;
    roi_zoom.x = roi_zoom.y = roi_zoom.z = 1.0;
    shift = realpoint_zero;
    temp_rp[0] = realspace_base_coord_to_alt(rs_offset(roi->coord_frame), roi->coord_frame);
    roi_center_rp = rp_add(rp_cmult(0.5, temp_rp[0]), rp_cmult(0.5, roi->corner));
    roi_radius_rp = rp_cmult(0.5,rp_diff(roi->corner, temp_rp[0]));
    break;

  case UI_STUDY_EVENT_PRESS_CHANGE_ISOCONTOUR:
    ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_ISOCONTOUR_ROI, base_rp, 0.0);
    grab_on = TRUE;
    initial_cp = canvas_cp;

    points = gnome_canvas_points_new(2);
    points->coords[2] = points->coords[0] = canvas_cp.x;
    points->coords[3] = points->coords[1] = canvas_cp.y;
    outline_color = color_table_outline_color(ui_study->current_volume->color_table, TRUE);
    canvas_item = gnome_canvas_item_new(gnome_canvas_root(canvas), gnome_canvas_line_get_type(),
					"points", points, "width_pixels", ui_study->roi_width,
					"fill_color_rgba", color_table_rgba_to_uint32(outline_color),
					NULL);
    gnome_canvas_item_grab(GNOME_CANVAS_ITEM(widget),
			   GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			   ui_common_cursor[UI_CURSOR_OLD_ROI_ISOCONTOUR], event->button.time);
    gtk_object_set_data(GTK_OBJECT(canvas_item), "roi", roi);
    gtk_object_set_data(GTK_OBJECT(canvas_item), "object_type", GINT_TO_POINTER(OBJECT_ROI_TYPE));
    gnome_canvas_points_unref(points);
    break;

  case UI_STUDY_EVENT_MOTION:
    ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, base_rp, 0.0);
    break;

  case UI_STUDY_EVENT_MOTION_MOVE_VIEW:
  case UI_STUDY_EVENT_MOTION_MINIMIZE_VIEW:
    ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, base_rp, 0.0);
    ui_study_update_targets(ui_study, TARGET_UPDATE, view_rp, rgba_black);
    break;


  case UI_STUDY_EVENT_MOTION_RESIZE_VIEW:
    ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, base_rp, 0.0);
    depth_change = TRUE;
    study_set_view_thickness(ui_study->study,REALPOINT_MAX_DIM(rp_diff(view_rp, initial_view_rp)));
    ui_study_update_thickness_adjustment(ui_study);
    ui_study_update_targets(ui_study, TARGET_UPDATE, initial_view_rp, rgba_black);
    break;


  case UI_STUDY_EVENT_PRESS_LARGE_ERASE_ISOCONTOUR:
  case UI_STUDY_EVENT_PRESS_ERASE_ISOCONTOUR:
    ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_ISOCONTOUR_ROI, base_rp, 0.0);
    grab_on = TRUE;
    canvas_item = GNOME_CANVAS_ITEM(widget);
    gnome_canvas_item_grab(canvas_item,
			   GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			   ui_common_cursor[UI_CURSOR_OLD_ROI_ERASE], event->button.time);
  case UI_STUDY_EVENT_MOTION_LARGE_ERASE_ISOCONTOUR:
  case UI_STUDY_EVENT_MOTION_ERASE_ISOCONTOUR:
    {
      voxelpoint_t temp_vp;
      intpoint_t temp_frame;
      volume_t * temp_vol;
      
      if (roi->type == ISOCONTOUR_2D)
	temp_vol = ui_study->current_slices[view]->volume; /* just use the first slice for now */
      else /* ISOCONTOUR_3D */
	temp_vol = ui_study->current_volume;
      temp_frame = volume_frame(temp_vol, study_view_time(ui_study->study));
      temp_rp[0] = realspace_base_coord_to_alt(base_rp, roi->coord_frame);
      ROI_REALPOINT_TO_VOXEL(roi, temp_rp[0], temp_vp);
      if ((canvas_event_type == UI_STUDY_EVENT_MOTION_LARGE_ERASE_ISOCONTOUR) ||
	  (canvas_event_type == UI_STUDY_EVENT_PRESS_LARGE_ERASE_ISOCONTOUR))
	roi_isocontour_erase_area(roi, temp_vp, 2);
      else
	roi_isocontour_erase_area(roi, temp_vp, 0);
      temp_rp[0] = realspace_base_coord_to_alt(base_rp, temp_vol->coord_frame);
      VOLUME_REALPOINT_TO_VOXEL(temp_vol, temp_rp[0], temp_frame, temp_vp);
      ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_VALUE, base_rp, 
				volume_value(temp_vol, temp_vp));
    }
    break;


  case UI_STUDY_EVENT_MOTION_CHANGE_ISOCONTOUR:
    {
      voxelpoint_t temp_vp;
      intpoint_t temp_frame;
      volume_t * temp_vol;

      if (roi->type == ISOCONTOUR_2D)
	temp_vol = ui_study->current_slices[view]->volume; /* just use the first slice for now */
      else /* ISOCONTOUR_3D */
	temp_vol = ui_study->current_volume;
      temp_frame = volume_frame(temp_vol, study_view_time(ui_study->study));
      temp_rp[0] = realspace_base_coord_to_alt(base_rp, temp_vol->coord_frame);
      VOLUME_REALPOINT_TO_VOXEL(temp_vol, temp_rp[0], temp_frame, temp_vp);
      ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_VALUE, base_rp, 
				volume_value(temp_vol, temp_vp));
    }
  case UI_STUDY_EVENT_MOTION_ALIGN_HORIZONTAL:
  case UI_STUDY_EVENT_MOTION_ALIGN_VERTICAL:
    points = gnome_canvas_points_new(2);
    points->coords[0] = initial_cp.x;
    points->coords[1] = initial_cp.y;
    points->coords[2] = canvas_cp.x;
    points->coords[3] = canvas_cp.y;
    gnome_canvas_item_set(canvas_item, "points", points, NULL);
    gnome_canvas_points_unref(points);
    break;


  case UI_STUDY_EVENT_MOTION_NEW_ROI:
    if ((ui_study->undrawn_roi->type == ISOCONTOUR_2D) || (ui_study->undrawn_roi->type == ISOCONTOUR_3D))
      ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_NEW_ISOCONTOUR_ROI, base_rp, 0.0);
    else {
      ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_NEW_ROI, base_rp, 0.0);
      diff_cp = cp_diff(initial_cp, canvas_cp);
      temp_cp[0] = cp_sub(initial_cp, diff_cp);
      temp_cp[1] = cp_add(initial_cp, diff_cp);
      gnome_canvas_item_set(canvas_item, "x1", temp_cp[0].x, "y1", temp_cp[0].y,
			    "x2", temp_cp[1].x, "y2", temp_cp[1].y,NULL);
    }
    break;

  case UI_STUDY_EVENT_MOTION_SHIFT_ROI:
    ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, base_rp, 0.0);
    shift = rp_sub(base_rp, initial_base_rp);
    diff_cp = cp_sub(canvas_cp, previous_cp);
    gnome_canvas_item_i2w(GNOME_CANVAS_ITEM(canvas_item)->parent, &diff_cp.x, &diff_cp.y);
    gnome_canvas_item_move(GNOME_CANVAS_ITEM(canvas_item),diff_cp.x,diff_cp.y); 
    previous_cp = canvas_cp;
    break;


  case UI_STUDY_EVENT_MOTION_ROTATE_ROI: 
    ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, base_rp, 0.0);
    {
      /* rotate button Pressed, we're rotating the object */
      /* note, I'd like to use the function "gnome_canvas_item_rotate"
	 but this isn't defined in the current version of gnome.... 
	 so I'll have to do a whole bunch of shit*/
      double affine[6];
      canvaspoint_t item_center;
      realpoint_t center;
      
      center = realspace_alt_coord_to_alt(roi_center_rp, roi->coord_frame, ui_study->canvas_coord_frame[view]);
      temp_rp[0] = rp_sub(initial_canvas_rp,center);
      temp_rp[1] = rp_sub(canvas_rp,center);
      
      theta = acos(rp_dot_product(temp_rp[0],temp_rp[1])/(rp_mag(temp_rp[0]) * rp_mag(temp_rp[1])));

      /* correct for the fact that acos is always positive by using the cross product */
      if ((temp_rp[0].x*temp_rp[1].y-temp_rp[0].y*temp_rp[1].x) < 0.0) theta = -theta;

      /* figure out what the center of the roi is in canvas_item coords */
      /* compensate for x's origin being top left (ours is bottom left) */
      item_center = ui_study_rp_2_cp(ui_study, view, center);
      affine[0] = cos(-theta); /* neg cause GDK has Y axis going down, not up */
      affine[1] = sin(-theta);
      affine[2] = -affine[1];
      affine[3] = affine[0];
      affine[4] = (1.0-affine[0])*item_center.x+affine[1]*item_center.y;
      affine[5] = (1.0-affine[3])*item_center.y+affine[2]*item_center.x;
      gnome_canvas_item_affine_absolute(GNOME_CANVAS_ITEM(canvas_item),affine);
    }
    break;


  case UI_STUDY_EVENT_MOTION_RESIZE_ROI:
    ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, base_rp, 0.0);
    {
      /* RESIZE button Pressed, we're scaling the object */
      /* note, I'd like to use the function "gnome_canvas_item_scale"
	 but this isn't defined in the current version of gnome.... 
	 so I'll have to do a whole bunch of shit*/
      /* also, this "zoom" strategy doesn't always work if the object is not
	 aligned with the view.... oh well... good enough for now... */
      double affine[6];
      canvaspoint_t item_center;
      floatpoint_t jump_limit, max, temp, dot;
      double cos_r, sin_r, rot;
      axis_t i_axis, axis[NUM_AXIS];
      canvaspoint_t radius_cp;
      realpoint_t center, radius, canvas_zoom;
	
      /* calculating the radius and center wrt the canvas */
      radius = realspace_alt_dim_to_alt(roi_radius_rp, roi->coord_frame, ui_study->canvas_coord_frame[view]);
      center = realspace_alt_coord_to_alt(roi_center_rp, roi->coord_frame, ui_study->canvas_coord_frame[view]);
      temp_rp[0] = rp_diff(initial_canvas_rp, center);
      temp_rp[1] = rp_diff(canvas_rp,center);
      radius_cp.x = roi_radius_rp.x;
      radius_cp.y = roi_radius_rp.y;
      jump_limit = cp_mag(radius_cp)/3.0;
	
      /* figure out the zoom we're specifying via the canvas */
      if (temp_rp[0].x < jump_limit) /* prevent jumping */
	canvas_zoom.x = (radius.x+temp_rp[1].x)/(radius.x+temp_rp[0].x);
      else
	canvas_zoom.x = temp_rp[1].x/temp_rp[0].x;
      if (temp_rp[0].y < jump_limit) /* prevent jumping */
	canvas_zoom.y = (radius.x+temp_rp[1].y)/(radius.x+temp_rp[0].y);
      else
	canvas_zoom.y = temp_rp[1].y/temp_rp[0].y;
      canvas_zoom.z = 1.0;
	
      /* translate the canvas zoom into the ROI's coordinate frame */
      temp_rp[0].x = temp_rp[0].y = temp_rp[0].z = 1.0;
      temp_rp[0] = realspace_alt_dim_to_alt(temp_rp[0], ui_study->canvas_coord_frame[view],
					    roi->coord_frame);
      roi_zoom = realspace_alt_dim_to_alt(canvas_zoom, ui_study->canvas_coord_frame[view],
					  roi->coord_frame);
      roi_zoom = rp_div(roi_zoom, temp_rp[0]);
	
      /* first, figure out how much the roi is rotated in the plane of the canvas */
      /* figure out which axis in the ROI is closest to the view's x axis */
      max = 0.0;
      axis[XAXIS]=XAXIS;
      temp_rp[0] = realspace_get_orthogonal_view_axis(rs_all_axis(ui_study->canvas_coord_frame[view]), 
						      view, ui_study->canvas_layout, XAXIS);
      for (i_axis=0;i_axis<NUM_AXIS;i_axis++) {
	temp = fabs(rp_dot_product(temp_rp[0], rs_specific_axis(roi->coord_frame, i_axis)));
	if (temp > max) {
	  max=temp;
	  axis[XAXIS]=i_axis;
	}
      }
	
      /* and y axis */
      max = 0.0;
      axis[YAXIS]= (axis[XAXIS]+1 < NUM_AXIS) ? axis[XAXIS]+1 : XAXIS;
      temp_rp[0] = realspace_get_orthogonal_view_axis(rs_all_axis(ui_study->canvas_coord_frame[view]), 
						      view, ui_study->canvas_layout, YAXIS);
      for (i_axis=0;i_axis<NUM_AXIS;i_axis++) {
	temp = fabs(rp_dot_product(temp_rp[0], rs_specific_axis(roi->coord_frame, i_axis)));
	if (temp > max) {
	  if (i_axis != axis[XAXIS]) {
	    max=temp;
	    axis[YAXIS]=i_axis;
	  }
	}
      }
	
      i_axis = ZAXIS;
      for (i_axis=0;i_axis<NUM_AXIS;i_axis++)
	if (i_axis != axis[XAXIS])
	  if (i_axis != axis[YAXIS])
	    axis[ZAXIS]=i_axis;
	
      rot = 0;
      for (i_axis=0;i_axis<=XAXIS;i_axis++) {
	/* and project that axis onto the x,y plane of the canvas */
	temp_rp[0] = rs_specific_axis(roi->coord_frame,axis[i_axis]);
	temp_rp[0] = realspace_base_coord_to_alt(temp_rp[0], ui_study->canvas_coord_frame[view]);
	temp_rp[0].z = 0.0;
	temp_rp[1] = realspace_get_orthogonal_view_axis(rs_all_axis(ui_study->canvas_coord_frame[view]), 
							view, ui_study->canvas_layout, i_axis);
	temp_rp[1] = realspace_base_coord_to_alt(temp_rp[1], ui_study->canvas_coord_frame[view]);
	temp_rp[1].z = 0.0;
	  
	/* and get the angle the projection makes to the x axis */
	dot = rp_dot_product(temp_rp[0],temp_rp[1])/(rp_mag(temp_rp[0]) * rp_mag(temp_rp[1]));
	/* correct for the fact that acos is always positive by using the cross product */
	if ((temp_rp[0].x*temp_rp[1].y-temp_rp[0].y*temp_rp[1].x) > 0.0)
	  temp = acos(dot);
	else
	  temp = -acos(dot);
	if (isnan(temp)) temp=0; 
	rot += temp;
      }

      cos_r = cos(rot);       /* precompute cos and sin of rot */
      sin_r = sin(rot);
	
      /* figure out what the center of the roi is in gnome_canvas_item coords */
      /* compensate for X's origin being top left (not bottom left) */
      item_center = ui_study_rp_2_cp(ui_study, view, center);
	
      /* do a wild ass affine matrix so that we can scale while preserving angles */
      affine[0] = canvas_zoom.x * cos_r * cos_r + canvas_zoom.y * sin_r * sin_r;
      affine[1] = (canvas_zoom.x-canvas_zoom.y)* cos_r * sin_r;
      affine[2] = affine[1];
      affine[3] = canvas_zoom.x * sin_r * sin_r + canvas_zoom.y * cos_r * cos_r;
      affine[4] = item_center.x - item_center.x*canvas_zoom.x*cos_r*cos_r 
	- item_center.x*canvas_zoom.y*sin_r*sin_r + (canvas_zoom.y-canvas_zoom.x)*item_center.y*cos_r*sin_r;
      affine[5] = item_center.y - item_center.y*canvas_zoom.y*cos_r*cos_r 
	- item_center.y*canvas_zoom.x*sin_r*sin_r + (canvas_zoom.y-canvas_zoom.x)*item_center.x*cos_r*sin_r;
      gnome_canvas_item_affine_absolute(GNOME_CANVAS_ITEM(canvas_item),affine);
    }
    break;


  case UI_STUDY_EVENT_RELEASE_MOVE_VIEW:
  case UI_STUDY_EVENT_RELEASE_MINIMIZE_VIEW:
  case UI_STUDY_EVENT_RELEASE_RESIZE_VIEW:
    ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, base_rp, 0.0);
    grab_on = FALSE;
    ui_study_update_targets(ui_study, TARGET_DELETE, realpoint_zero, rgba_black); /* remove targets */

    if (canvas_event_type == UI_STUDY_EVENT_RELEASE_MOVE_VIEW) 
      study_set_view_center(ui_study->study, view_rp);
    else if (canvas_event_type == UI_STUDY_EVENT_RELEASE_RESIZE_VIEW)
      study_set_view_center(ui_study->study, initial_view_rp);
    else { /* UI_STUDY_EVENT_RELASE_MINIMIZE_VIEW */
      /* correct for the change in depth */
      temp_rp[0] = realspace_alt_coord_to_alt(initial_view_rp, study_coord_frame(ui_study->study),
					      ui_study->canvas_coord_frame[view]);
      temp_rp[1] = realspace_alt_coord_to_alt(view_rp, study_coord_frame(ui_study->study),
      					      ui_study->canvas_coord_frame[view]);
      temp_rp[0].z = temp_rp[1].z; /* correct for any depth change */
      view_rp = realspace_alt_coord_to_alt(temp_rp[0], ui_study->canvas_coord_frame[view],
					   study_coord_frame(ui_study->study));
      study_set_view_center(ui_study->study, view_rp);
    }

    /* update the canvases */
    if (depth_change == TRUE) {
      depth_change = FALSE; /* reset */
      ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_ALL); /* need to update all canvases */
    } else { /* depth_change == FALSE ,don't need to update the canvas we're currently over */
      switch (view) {
      case TRANSVERSE:
	ui_study_update_canvas(ui_study, CORONAL, UPDATE_ALL);
	ui_study_update_canvas(ui_study, SAGITTAL, UPDATE_ALL);
	ui_study_update_canvas(ui_study, TRANSVERSE, UPDATE_PLANE_ADJUSTMENT);
	ui_study_update_canvas(ui_study, TRANSVERSE, UPDATE_ARROWS);
	break;
      case CORONAL:
	ui_study_update_canvas(ui_study, TRANSVERSE, UPDATE_ALL);
	ui_study_update_canvas(ui_study, SAGITTAL, UPDATE_ALL);
	ui_study_update_canvas(ui_study, CORONAL, UPDATE_PLANE_ADJUSTMENT);
	ui_study_update_canvas(ui_study, CORONAL, UPDATE_ARROWS);
	break;
      case SAGITTAL:
	ui_study_update_canvas(ui_study, TRANSVERSE, UPDATE_ALL);
	ui_study_update_canvas(ui_study, CORONAL, UPDATE_ALL);
	ui_study_update_canvas(ui_study, SAGITTAL, UPDATE_PLANE_ADJUSTMENT);
	ui_study_update_canvas(ui_study, SAGITTAL, UPDATE_ARROWS);
	break;
      default:
	break;
      }
    }
    break;


  case UI_STUDY_EVENT_RELEASE_ALIGN_HORIZONTAL:
  case UI_STUDY_EVENT_RELEASE_ALIGN_VERTICAL:
    ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_ALIGN, base_rp, 0.0);
    break;

  case UI_STUDY_EVENT_CANCEL_ALIGN_HORIZONTAL:
  case UI_STUDY_EVENT_CANCEL_ALIGN_VERTICAL:
    ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_VOLUME, base_rp, 0.0);
    extended_mode = FALSE;
    gtk_object_destroy(GTK_OBJECT(canvas_item));
    break;


  case UI_STUDY_EVENT_ENACT_ALIGN_HORIZONTAL:
  case UI_STUDY_EVENT_ENACT_ALIGN_VERTICAL:
    ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_VOLUME, base_rp, 0.0);
    {
      floatpoint_t temp_rot;
      volume_t * temp_vol;
      ui_volume_list_t * temp_ui_vols;
      realpoint_t volume_center, real_center;

      extended_mode = FALSE;
      gtk_object_destroy(GTK_OBJECT(canvas_item));

      /* figure out how many degrees we've rotated */
      diff_cp = cp_sub(canvas_cp, initial_cp);
      if (align_vertical) temp_rot = -atan(diff_cp.x/diff_cp.y);
      else temp_rot = atan(diff_cp.y/diff_cp.x);

      if (isnan(temp_rot)) temp_rot = 0;
      
      if (view == SAGITTAL) temp_rot = -temp_rot; /* sagittal is left-handed */
      
      /* rotate all the currently displayed volumes */
      temp_ui_vols = ui_study->current_volumes;
      while (temp_ui_vols != NULL) {
	temp_vol = temp_ui_vols->volume;
	
	/* saving the view center wrt to the volume's coord axis, as we're rotating around the view center */
	real_center = realspace_alt_coord_to_base(study_view_center(ui_study->study),
						  study_coord_frame(ui_study->study));
	volume_center = realspace_base_coord_to_alt(real_center, temp_vol->coord_frame);
	realspace_rotate_on_axis(&temp_vol->coord_frame,
				 rs_specific_axis(ui_study->canvas_coord_frame[view], ZAXIS),
				 temp_rot);
	
	/* recalculate the offset of this volume based on the center we stored */
	rs_set_offset(&temp_vol->coord_frame, realpoint_zero);
	volume_center = realspace_alt_coord_to_base(volume_center, temp_vol->coord_frame);
	rs_set_offset(&temp_vol->coord_frame, rp_sub(real_center, volume_center));
	/* I really should check to see if a volume dialog is up, and update the rotations.... */
	temp_ui_vols = temp_ui_vols->next; /* and go on to the next */
      }
      
      ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_ALL);
    }
    break;


  case UI_STUDY_EVENT_RELEASE_NEW_ROI:
    ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_VOLUME, base_rp, 0.0);
    grab_on = FALSE;
    switch(ui_study->undrawn_roi->type) {
    case CYLINDER:
    case BOX:
    case ELLIPSOID:  
      gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(canvas_item), event->button.time);
      gtk_object_destroy(GTK_OBJECT(canvas_item)); /* get rid of the roi drawn on the canvas */
      
      diff_rp = rp_diff(initial_canvas_rp, canvas_rp);
      diff_rp.z = study_view_thickness(ui_study->study)/2.0;
      temp_rp[0] = rp_sub(initial_canvas_rp, diff_rp);
      temp_rp[1] = rp_add(initial_canvas_rp, diff_rp);

      /* we'll save the coord frame and offset of the roi */
      rs_set_offset(&ui_study->undrawn_roi->coord_frame,
		    realspace_alt_coord_to_base(temp_rp[0], ui_study->canvas_coord_frame[view]));
      rs_set_axis(&ui_study->undrawn_roi->coord_frame, rs_all_axis(ui_study->canvas_coord_frame[view]));
      
      /* and set the far corner of the roi */
      ui_study->undrawn_roi->corner = 
	rp_abs(realspace_alt_coord_to_alt(temp_rp[1], ui_study->canvas_coord_frame[view],
					  ui_study->undrawn_roi->coord_frame));
      break;
    case ISOCONTOUR_2D: 
    case ISOCONTOUR_3D:
      {
	voxelpoint_t temp_vp;
	intpoint_t temp_frame;
	volume_t * temp_vol;

	if (ui_study->undrawn_roi->type == ISOCONTOUR_2D) {
	  temp_vol = ui_study->current_slices[view]->volume; /* just use the first slice for now */
	  temp_frame = 0;
	} else { /* ISOCONTOUR_3D */
	  temp_vol = ui_study->current_volume;
	  temp_frame = volume_frame(temp_vol, study_view_time(ui_study->study));
	}
	temp_rp[0] = realspace_base_coord_to_alt(base_rp, temp_vol->coord_frame);
	VOLUME_REALPOINT_TO_VOXEL(temp_vol, temp_rp[0], temp_frame, temp_vp);
	ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(canvas));
	if (data_set_includes_voxel(temp_vol->data_set,temp_vp))
	  roi_set_isocontour(ui_study->undrawn_roi, temp_vol, temp_vp);
	ui_common_remove_cursor(GTK_WIDGET(canvas));
      }
      break;
    default:
      g_warning("%s: unexpected case in %s at line %d, roi_type %d", 
		PACKAGE, __FILE__, __LINE__, ui_study->undrawn_roi->type);
      break;
    }
    ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_ROIS);
    roi_free(ui_study->undrawn_roi); 
    ui_study->undrawn_roi = NULL;
    break;


  case UI_STUDY_EVENT_RELEASE_SHIFT_ROI:
    gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
    grab_on = FALSE;
    /* ------------- apply any shift done -------------- */
    if (!REALPOINT_EQUAL(shift, realpoint_zero)) {
      rs_set_offset(&roi->coord_frame, rp_add(shift, rs_offset(roi->coord_frame)));
      /* if we were moving the ROI, reset the view_center to the center of the current roi */
      temp_rp[0] = rp_add(rp_cmult(0.5, realspace_base_coord_to_alt(rs_offset(roi->coord_frame), 
								    roi->coord_frame)),
			  rp_cmult(0.5, roi->corner));
      temp_rp[0] = realspace_alt_coord_to_alt(temp_rp[0],
					      roi->coord_frame, 
					      study_coord_frame(ui_study->study));
      study_set_view_center(ui_study->study, temp_rp[0]);
      ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_ALL);
    }
    break;


  case UI_STUDY_EVENT_RELEASE_ROTATE_ROI:
    {
      ui_roi_list_t * ui_roi_item;
      view_t i_view;

      gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
      grab_on = FALSE;
      if (view == SAGITTAL) theta = -theta; /* compensate for sagittal being left-handed coord frame */
      
      temp_rp[0] = realspace_alt_coord_to_base(roi_center_rp, roi->coord_frame); /* new center */
      rs_set_offset(&roi->coord_frame, temp_rp[0]);
      
      /* now rotate the roi coord_frame axis */
      realspace_rotate_on_axis(&roi->coord_frame,
			       rs_specific_axis(ui_study->canvas_coord_frame[view],  ZAXIS),
			       theta);

      /* and recaculate the offset */
      temp_rp[0] = rp_sub(realspace_base_coord_to_alt(temp_rp[0], roi->coord_frame), roi_radius_rp);
      temp_rp[0] = realspace_alt_coord_to_base(temp_rp[0],roi->coord_frame);
      rs_set_offset(&roi->coord_frame, temp_rp[0]); 
      
      ui_roi_item = ui_roi_list_get_ui_roi(ui_study->current_rois, roi);
      for (i_view=0;i_view<NUM_VIEWS;i_view++) 
	ui_roi_item->canvas_roi[i_view] =
	  ui_study_update_canvas_roi(ui_study,i_view, ui_roi_item->canvas_roi[i_view], ui_roi_item->roi);
    }
    break;


  case UI_STUDY_EVENT_RELEASE_RESIZE_ROI:
    {
      ui_roi_list_t * ui_roi_item;
      view_t i_view;

      gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
      grab_on = FALSE;

      temp_rp[0] = rp_mult(roi_zoom, roi_radius_rp); /* new radius */
      rs_set_offset(&roi->coord_frame, realspace_alt_coord_to_base(rp_sub(roi_center_rp, temp_rp[0]), 
								   roi->coord_frame));
	  
      /* and the new upper right corner is simply twice the radius */
      roi->corner = rp_cmult(2.0, temp_rp[0]);
	  
      ui_roi_item = ui_roi_list_get_ui_roi(ui_study->current_rois, roi);
      for (i_view=0;i_view<NUM_VIEWS;i_view++) 
	ui_roi_item->canvas_roi[i_view] =
	  ui_study_update_canvas_roi(ui_study,i_view, ui_roi_item->canvas_roi[i_view], ui_roi_item->roi);
    }
    break;

  case UI_STUDY_EVENT_RELEASE_LARGE_ERASE_ISOCONTOUR:
  case UI_STUDY_EVENT_RELEASE_ERASE_ISOCONTOUR:
    {
      ui_roi_list_t * ui_roi_item;
      view_t i_view;

      gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
      grab_on = FALSE;

      ui_roi_item = ui_roi_list_get_ui_roi(ui_study->current_rois, roi);
      for (i_view=0;i_view<NUM_VIEWS;i_view++) 
	ui_roi_item->canvas_roi[i_view] =
	  ui_study_update_canvas_roi(ui_study,i_view, ui_roi_item->canvas_roi[i_view], ui_roi_item->roi);
    }
    break;

  case UI_STUDY_EVENT_RELEASE_CHANGE_ISOCONTOUR:
    gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
    grab_on = FALSE;
    gtk_object_destroy(GTK_OBJECT(canvas_item));
    {
      voxelpoint_t temp_vp;
      intpoint_t temp_frame;
      volume_t * temp_vol;
      ui_roi_list_t * ui_roi_item;
      view_t i_view;

      if (roi->type == ISOCONTOUR_2D) {
	temp_vol = ui_study->current_slices[view]->volume; /* just use the first slice for now */
	temp_frame = 0;
      } else { /* ISOCONTOUR_3D */
	temp_vol = ui_study->current_volume;
	temp_frame = volume_frame(temp_vol, study_view_time(ui_study->study));
      }
      temp_rp[0] = realspace_base_coord_to_alt(base_rp, temp_vol->coord_frame);
      VOLUME_REALPOINT_TO_VOXEL(temp_vol, temp_rp[0], temp_frame, temp_vp);
      ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(canvas));
      if (data_set_includes_voxel(temp_vol->data_set,temp_vp))
	roi_set_isocontour(roi, temp_vol, temp_vp);
      ui_common_remove_cursor(GTK_WIDGET(canvas));

      ui_roi_item = ui_roi_list_get_ui_roi(ui_study->current_rois, roi);
      for (i_view=0;i_view<NUM_VIEWS;i_view++) 
	ui_roi_item->canvas_roi[i_view] =
	  ui_study_update_canvas_roi(ui_study,i_view, ui_roi_item->canvas_roi[i_view], ui_roi_item->roi);
      break;
    }

  case UI_STUDY_EVENT_DO_NOTHING:
    break;
  default:
    g_warning("%s: unexpected case in %s at line %d, event %d", PACKAGE, __FILE__, __LINE__, canvas_event_type);
    break;
  }
  
  return FALSE;
}



/* function called indicating the plane adjustment has changed */
void ui_study_cb_plane_change(GtkObject * adjustment, gpointer data) {

  ui_study_t * ui_study = data;
  view_t i_view;
  realpoint_t view_center;

  /* sanity check */
  if (study_volumes(ui_study->study) == NULL)
    return;
  if (ui_study->current_volumes == NULL)
    return;

  /* figure out which scale widget called me */
  i_view = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(adjustment),"view"));

  switch(i_view) {
  case TRANSVERSE:
    view_center = study_view_center(ui_study->study);
    view_center.z = GTK_ADJUSTMENT(adjustment)->value;
    study_set_view_center(ui_study->study, view_center);
    ui_study_update_canvas(ui_study,TRANSVERSE,UPDATE_ALL);
    ui_study_update_canvas(ui_study,CORONAL,UPDATE_ARROWS);
    ui_study_update_canvas(ui_study,SAGITTAL,UPDATE_ARROWS);
    break;
  case CORONAL:
    view_center = study_view_center(ui_study->study);
    view_center.y = GTK_ADJUSTMENT(adjustment)->value;
    study_set_view_center(ui_study->study, view_center);
    ui_study_update_canvas(ui_study,CORONAL,UPDATE_ALL);
    ui_study_update_canvas(ui_study,TRANSVERSE,UPDATE_ARROWS);
    ui_study_update_canvas(ui_study,SAGITTAL,UPDATE_ARROWS);
    break;
  case SAGITTAL:
    view_center = study_view_center(ui_study->study);
    view_center.x = GTK_ADJUSTMENT(adjustment)->value;
    study_set_view_center(ui_study->study, view_center);
    ui_study_update_canvas(ui_study,SAGITTAL,UPDATE_ALL);
    ui_study_update_canvas(ui_study,TRANSVERSE,UPDATE_ARROWS);
    ui_study_update_canvas(ui_study,CORONAL,UPDATE_ARROWS);
    break;
  default:
    break;
  }

  return;
}

void ui_study_cb_zoom(GtkObject * adjustment, gpointer data) {

  ui_study_t * ui_study = data;

  /* sanity check */
  if (study_volumes(ui_study->study) == NULL)
    return;

  if (study_zoom(ui_study->study) != GTK_ADJUSTMENT(adjustment)->value) {
      study_set_zoom(ui_study->study, GTK_ADJUSTMENT(adjustment)->value);
      ui_study_update_canvas(ui_study,NUM_VIEWS, UPDATE_ALL);
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

  /* sanity check */
  if (study_volumes(ui_study->study) == NULL)
    return;

  gtk_signal_emit_stop_by_name(GTK_OBJECT(adjustment), "value_changed");
  if (study_view_thickness(ui_study->study) != GTK_ADJUSTMENT(adjustment)->value) {
    study_set_view_thickness(ui_study->study, GTK_ADJUSTMENT(adjustment)->value);
    ui_study_update_canvas(ui_study,NUM_VIEWS, UPDATE_ALL);
  }
  
  return;
}


/* callbacks for setting up a set of slices in a new window */
void ui_study_cb_series(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  volume_list_t * temp_volumes;
  view_t view;
  series_t series_type;
  
  view = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "view"));
  series_type = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "series_type"));

  temp_volumes = ui_volume_list_return_volume_list(ui_study->current_volumes);
  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_study->canvas[0]));
  ui_series_create(ui_study->study, temp_volumes, view, ui_study->canvas_layout,series_type);
  ui_common_remove_cursor(GTK_WIDGET(ui_study->canvas[0]));
  temp_volumes = volume_list_free(temp_volumes); /* and delete the volume_list */

  return;
}

#ifdef AMIDE_LIBVOLPACK_SUPPORT
/* callback for starting up volume rendering using nearest neighbor interpolation*/
void ui_study_cb_rendering_nearest_neighbor(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  volume_list_t * temp_volumes;


  temp_volumes = ui_volume_list_return_volume_list(ui_study->current_volumes);
  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_study->canvas[0]));
  ui_rendering_create(temp_volumes, study_coord_frame(ui_study->study),
		      study_view_time(ui_study->study), 
		      study_view_duration(ui_study->study),
		      NEAREST_NEIGHBOR);
  ui_common_remove_cursor(GTK_WIDGET(ui_study->canvas[0]));
  temp_volumes = volume_list_free(temp_volumes);

  return;
}
/* callback for starting up volume rendering using trilinear interpolation*/
void ui_study_cb_rendering_trilinear(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  volume_list_t * temp_volumes;


  temp_volumes = ui_volume_list_return_volume_list(ui_study->current_volumes);
  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_study->canvas[0]));
  ui_rendering_create(temp_volumes, study_coord_frame(ui_study->study),
		      study_view_time(ui_study->study), 
		      study_view_duration(ui_study->study),
		      TRILINEAR);
  ui_common_remove_cursor(GTK_WIDGET(ui_study->canvas[0]));
  temp_volumes = volume_list_free(temp_volumes);

  return;
}
#endif


/* do roi calculations for selected ROI's over selected volumes */
void ui_study_cb_calculate_all(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_study->canvas[0]));
  ui_roi_analysis_dialog_create(ui_study, TRUE);
  ui_common_remove_cursor(GTK_WIDGET(ui_study->canvas[0]));
  return;

}

/* do roi calculations for all ROI's over selected volumes */
void ui_study_cb_calculate_selected(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_study->canvas[0]));
  ui_roi_analysis_dialog_create(ui_study, FALSE);
  ui_common_remove_cursor(GTK_WIDGET(ui_study->canvas[0]));
  return;
}


/* function called when the threshold is changed */
void ui_study_cb_threshold_changed(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study=data;

  /* update the ui_study cavases  */
  ui_study_update_canvas(ui_study,NUM_VIEWS,REFRESH_IMAGE);

  return;
}

/* function called when the volume's color is changed */
void ui_study_cb_color_changed(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study=data;

  /* update the ui_study cavases  */
  ui_study_update_canvas(ui_study,NUM_VIEWS,REFRESH_IMAGE);
  ui_study_update_canvas(ui_study,NUM_VIEWS,UPDATE_ROIS);

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
  ui_volume_list_t * ui_volume_list_item;

  /* make sure we don't already have a volume edit dialog up */
  ui_volume_list_item = ui_volume_list_get_ui_volume(ui_study->current_volumes,ui_study->current_volume);
  if (ui_volume_list_item != NULL)
    if (ui_volume_list_item->dialog != NULL)
      return;
              
  if (ui_study->current_volume == NULL) return;
  if (ui_study->threshold_dialog != NULL) return;

  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_study->canvas[0]));
  ui_study->threshold_dialog = amitk_threshold_dialog_new(ui_study->current_volume);
  ui_common_remove_cursor(GTK_WIDGET(ui_study->canvas[0]));
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

  return;
}

/* function to switch to normalizing per slice */
void ui_study_cb_scaling(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  scaling_t i_scaling;

  /* figure out which scaling menu item called me */
  i_scaling = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"scaling"));

  /* check if we actually changed vapplues */
  if (study_scaling(ui_study->study) != i_scaling) {
    /* and inact the changes */
    study_set_scaling(ui_study->study, i_scaling);
    if (study_volumes(ui_study->study) != NULL)
      ui_study_update_canvas(ui_study,NUM_VIEWS, REFRESH_IMAGE);
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
			       ui_study_cb_entry_name,
			       &roi_name, 
			       (GNOME_IS_APP(ui_study->app) ? 
				GTK_WINDOW(ui_study->app) : NULL));
  
  entry_return = gnome_dialog_run_and_close(GNOME_DIALOG(entry));
  
  if (entry_return == 0) {
    roi = roi_init();
    roi_set_name(roi,roi_name);
    roi->type = i_roi_type;
    study_add_roi(ui_study->study, roi);
    ui_study_tree_add_roi(ui_study, roi);
    ui_study->undrawn_roi = roi_add_reference(roi);
    roi = roi_free(roi);
  }

  return;
}

/* callback function for adding an alignment point */
void ui_study_cb_add_alignment_point(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  GtkWidget * entry;
  gchar * pt_name = NULL;
  gint entry_return;
  //  align_pt_t new_point;

  //  if (ui_study->current_mode != VOLUME_MODE) return;
  if (ui_study->current_volume == NULL) return;

  entry = gnome_request_dialog(FALSE, "Alignment Point Name:", "", 256, 
			       ui_study_cb_entry_name,
			       &pt_name, 
			       (GNOME_IS_APP(ui_study->app) ? 
				GTK_WINDOW(ui_study->app) : NULL));
  entry_return = gnome_dialog_run_and_close(GNOME_DIALOG(entry));
  
  if (entry_return == 0) {
    //    new_pt = align_pt_init();
    //    align_pt_set_name(new_pt, pt_name);
    //    study_add_alignment(ui_study->study, ui_study->current_volume, new_pt);
    //    ui_study_tree_add_align_pt(ui_study, ui_study->current_volume, new_pt);
    //    new_pt = alignt_pt_free(new_pt);
  }

  return;
}

/* callback function for changing user's preferences */
void ui_study_cb_preferences(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study=data;
  if (ui_study->preferences_dialog == NULL)
    ui_study->preferences_dialog = ui_preferences_dialog_create(ui_study);

}

/* callback used when a leaf is clicked on, this handles all clicks */
void ui_study_cb_tree_leaf_clicked(GtkWidget * leaf, GdkEventButton * event, gpointer data) {

  ui_study_t * ui_study = data;
  ui_study_tree_object_t object_type;
  volume_t * volume;
  roi_t * roi;
  GtkWidget * subtree;
  GtkWidget * study_leaf;
  realpoint_t roi_center;
  gboolean make_active = FALSE;
  gboolean unselect = FALSE;
  gboolean select = FALSE;
  gboolean popup_dialog = FALSE;
  gboolean popup_study_dialog = FALSE;
  view_t i_view;
  GnomeCanvasItem * roi_canvas_item[NUM_VIEWS];
  ui_volume_list_t * ui_volume_list_item;

  /* do any events pending, this allows the select state of the leaf
     to get updated correctly with double clicks*/
  while (gtk_events_pending())  gtk_main_iteration();

  /* decode the event, and figure out what we gotta do */
  switch (event->button) {

  case 1: /* left button */
    if (GTK_WIDGET_STATE(leaf) == GTK_STATE_SELECTED)
      select = TRUE;
    else
      unselect = TRUE;
    break;


  case 2: /* middle button */
    if (GTK_WIDGET_STATE(leaf) != GTK_STATE_SELECTED) {
      /* next 3 lines are because GTK only allows button 1 to select in the tree, so this works around */
      study_leaf = gtk_object_get_data(GTK_OBJECT(ui_study->tree), "study_leaf");
      subtree = GTK_TREE_ITEM_SUBTREE(study_leaf);
      gtk_tree_select_child(GTK_TREE(subtree), leaf); 
    }
    select = TRUE;
    make_active = TRUE;
    break;


  case 3: /* right button */
  default:
    popup_dialog = (GTK_WIDGET_STATE(leaf) == GTK_STATE_SELECTED);
    popup_study_dialog = TRUE;
    break;
  }

  /* do the appropriate action for the appropriate object */
  object_type = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(leaf), "type"));

  switch(object_type) {

  case UI_STUDY_TREE_VOLUME:
    volume = gtk_object_get_data(GTK_OBJECT(leaf), "object");

    if (select) {
      if (!ui_volume_list_includes_volume(ui_study->current_volumes, volume))
	ui_study->current_volumes = ui_volume_list_add_volume(ui_study->current_volumes, volume, leaf);

      /* if no other volume is active, make this guy active */
      if (ui_study->current_volume == NULL) {
	ui_study->current_volume = volume;
	make_active = TRUE;
      }

      /* we'll need to redraw the canvas */
      ui_study_update_canvas(ui_study,NUM_VIEWS,UPDATE_ALL);
      ui_time_dialog_set_times(ui_study);

    }


    if (unselect) {
      /* remove the volume from the selection list */
      ui_study->current_volumes =  ui_volume_list_remove_volume(ui_study->current_volumes, volume);
      
      /* if it's the currently active volume... */
      if (ui_study->current_volume == volume) {
	/* reset the currently active volume */
	ui_study->current_volume = ui_volume_list_get_first_volume(ui_study->current_volumes);
	/* update which row in the tree has the active symbol */
	ui_study_tree_update_active_leaf(ui_study, NULL); 
      } 
      
      /* destroy the threshold window if it's open and looking at this volume */
      if (ui_study->threshold_dialog != NULL)
      	if (amitk_threshold_dialog_volume(AMITK_THRESHOLD_DIALOG(ui_study->threshold_dialog)) == volume) {
	  gtk_widget_destroy(ui_study->threshold_dialog);
	  ui_study->threshold_dialog = NULL;
	}
      
      /* we'll need to redraw the canvas */
      ui_study_update_canvas(ui_study,NUM_VIEWS,UPDATE_ALL);
      ui_time_dialog_set_times(ui_study);
      
    }

    if (make_active) {
      ui_study->current_volume = volume;
      
      /* make sure we don't already have a volume edit dialog up */
      ui_volume_list_item = ui_volume_list_get_ui_volume(ui_study->current_volumes,ui_study->current_volume);
      if (ui_volume_list_item != NULL) {
	if (ui_volume_list_item->dialog != NULL) {
	  gtk_widget_destroy(ui_study->threshold_dialog);
	  ui_study->threshold_dialog = NULL;
	} else {
	  /* reset the threshold widget based on the current volume */
	  if (ui_study->threshold_dialog != NULL)
	    amitk_threshold_dialog_new_volume(AMITK_THRESHOLD_DIALOG(ui_study->threshold_dialog), volume);
	}
      }

      /* indicate this is now the active object */
      ui_study_tree_update_active_leaf(ui_study, leaf);
      
    }
    
    if (popup_dialog) {
      ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_study->canvas[0]));
      ui_volume_dialog_create(ui_study, volume); /* start up the volume dialog */
      ui_common_remove_cursor(GTK_WIDGET(ui_study->canvas[0]));
    }
    
    break;


  case UI_STUDY_TREE_ROI:
    roi = gtk_object_get_data(GTK_OBJECT(leaf), "object");


    if (select) {
      /* and add this roi to the current_rois list */
      if (!ui_roi_list_includes_roi(ui_study->current_rois, roi)) {
	/* make the canvas graphics */
	for (i_view=0;i_view<NUM_VIEWS;i_view++)
	  roi_canvas_item[i_view] = ui_study_update_canvas_roi(ui_study,i_view,NULL, roi);
	ui_study->current_rois =  ui_roi_list_add_roi(ui_study->current_rois, roi, roi_canvas_item, leaf);
      }
    }


    if (unselect) {
      ui_study->current_rois = ui_roi_list_remove_roi(ui_study->current_rois, roi);
    }


    if (make_active) {
      /* center the view on this roi, check first if the roi has been drawn, and if we're
	 already centered */
      roi_center = roi_calculate_center(roi);
      roi_center = realspace_base_coord_to_alt(roi_center, study_coord_frame(ui_study->study));
      if ( !roi_undrawn(roi) && !REALPOINT_EQUAL(roi_center, study_view_center(ui_study->study))) {
	study_set_view_center(ui_study->study, roi_center);
	ui_study_update_canvas(ui_study,NUM_VIEWS, UPDATE_ALL);
      } else {
	/* if this roi hasn't yet been drawn, at least update 
	   which roi is highlighted */
	ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_ROIS);
      }
    }

    if ((popup_dialog) && (!roi_undrawn(roi))) {
      ui_roi_dialog_create(ui_study, roi);
    }
    break;




  case UI_STUDY_TREE_STUDY:
  default:
    study_leaf = gtk_object_get_data(GTK_OBJECT(ui_study->tree), "study_leaf");
    gtk_tree_item_expand(GTK_TREE_ITEM(study_leaf));
    if (popup_study_dialog) 
      ui_study_dialog_create(ui_study);
    break;
  }

  return;
}

/* callback used when the background of the tree is clicked on */
void ui_study_cb_tree_clicked(GtkWidget * leaf, GdkEventButton * event, gpointer data) {

  ui_study_t * ui_study = data;
  GtkWidget * menu;
  GtkWidget * menuitem;
  roi_type_t i_roi_type;

  if (event->type != GDK_BUTTON_PRESS)
    return;

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

  return;
}

/* function called to edit selected objects in the study */
void ui_study_cb_edit_objects(GtkWidget * button, gpointer data) {

  ui_study_t * ui_study = data;
  ui_volume_list_t * volume_list = ui_study->current_volumes;
  ui_roi_list_t * roi_list = ui_study->current_rois;

  /* pop up dialogs for the selected volumes */
  while (volume_list != NULL) {
    ui_volume_dialog_create(ui_study, volume_list->volume);
    volume_list = volume_list->next;
  }

  /* pop up dialogs for the selected rois */
  while (roi_list != NULL) {
    ui_roi_dialog_create(ui_study, roi_list->roi);
    roi_list = roi_list->next;
  }

  return;
}



/* function called to delete selected objects from the tree and the study */
void ui_study_cb_delete_objects(GtkWidget * button, gpointer data) {

  ui_study_t * ui_study = data;
  volume_t * volume;
  roi_t * roi;
  GtkWidget * study_leaf;
  GtkWidget * subtree;
  GtkWidget * question;
  gchar * message;
  gchar * temp;
  gint question_return;
  ui_volume_list_t * current_volumes;
  ui_roi_list_t * current_rois;

  /* sanity check */
  if ((ui_study->current_volumes == NULL) && (ui_study->current_rois == NULL))
    return;

  /* formulate a message to prompt to the user */
  message = g_strdup_printf("Do you want to delete:\n");
  current_volumes = ui_study->current_volumes;
  while (current_volumes != NULL) {
    temp = message;
    message = g_strdup_printf("%s\tvolume:\t%s\n", temp, current_volumes->volume->name);
    g_free(temp);
    current_volumes = current_volumes->next;
  }
  current_rois = ui_study->current_rois;
  while (current_rois != NULL) {
    temp = message;
    message = g_strdup_printf("%s\troi:\t%s\n", temp, current_rois->roi->name);
    g_free(temp);
    current_rois = current_rois->next;
  }
  
  /* see if we really wanna delete crap */
  if (GNOME_IS_APP(ui_study->app))
    question = gnome_ok_cancel_dialog_modal_parented(message, NULL, NULL, 
						     GTK_WINDOW(ui_study->app));
  else
    question = gnome_ok_cancel_dialog_modal(message, NULL, NULL);

  g_free(message);
  question_return = gnome_dialog_run_and_close(GNOME_DIALOG(question));

  if (question_return != 0) 
    return; /* didn't hit the "ok" button */

  study_leaf = gtk_object_get_data(GTK_OBJECT(ui_study->tree), "study_leaf");
  subtree = GTK_TREE_ITEM_SUBTREE(study_leaf);

  /* delete the selected volumes */
  while (ui_study->current_volumes != NULL) {
    volume = volume_add_reference(ui_study->current_volumes->volume);

    /* remove the leaf */
    gtk_tree_remove_item(GTK_TREE(subtree), ui_study->current_volumes->tree_leaf);
    
    /* destroy the threshold window if it's open, and remove the active mark */
    if (ui_study->current_volume==volume) {
      ui_study->current_volume = NULL;
      if (ui_study->threshold_dialog != NULL) {
	gtk_widget_destroy(ui_study->threshold_dialog);
	ui_study->threshold_dialog = NULL;
      }
    }
     
    /* remove the volume from existence */
    ui_study->current_volumes = ui_volume_list_remove_volume(ui_study->current_volumes, volume);
    study_remove_volume(ui_study->study, volume);
    volume = volume_free(volume);
  }

  /* delete the selected roi's */
  while (ui_study->current_rois != NULL) {
    roi = roi_add_reference(ui_study->current_rois->roi);

    /* remove the leaf */
    gtk_tree_remove_item(GTK_TREE(subtree), ui_study->current_rois->tree_leaf);

    /* remove the roi's data structure */
    study_remove_roi(ui_study->study, roi);
    ui_study->current_rois = ui_roi_list_remove_roi(ui_study->current_rois, roi);
    roi = roi_free(roi);
  }

  /* save info as to which leaf has the active symbol */
  gtk_object_set_data(GTK_OBJECT(ui_study->tree), "active_row", NULL);

  /* redraw the screen */
  ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_ALL);

  return;
}


/* function to switch the interpolation method */
void ui_study_cb_interpolation(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  interpolation_t i_interpolation;

  /* figure out which scaling menu item called me */
  i_interpolation = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"interpolation"));

  /* check if we actually changed values */
  if (study_interpolation(ui_study->study) != i_interpolation) {
    /* and inact the changes */
    study_set_interpolation(ui_study->study, i_interpolation);
    if (study_volumes(ui_study->study) != NULL) 
      ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_IMAGE);
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
































