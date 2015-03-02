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

  /* get the filename and import */
  import_filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_selection));
  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_study->canvas[0]));
  ui_study_import_file(ui_study, import_filename, model_filename, import_method);
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

  file_selection = GTK_FILE_SELECTION(gtk_file_selection_new(_("Import File")));

  /* save a pointer to ui_study */
  gtk_object_set_data(GTK_OBJECT(file_selection), "ui_study", ui_study);
  
  /* and save which method of importing we want to use */
  gtk_object_set_data(GTK_OBJECT(file_selection), "method",
		      gtk_object_get_data(GTK_OBJECT(widget), "method"));

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
gboolean ui_study_cb_update_help_info(GtkWidget * widget, GdkEventCrossing * event,
					     gpointer data) {
  ui_study_t * ui_study = data;
  ui_study_help_info_t which_info;

  which_info = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "which_help"));

  /* quick correction so we can handle when the canvas switches to roi mode */
  if (which_info == HELP_INFO_CANVAS_VOLUME)
    if (ui_study->current_mode == ROI_MODE) {
      if (roi_undrawn(ui_study->current_roi))
	which_info = HELP_INFO_CANVAS_NEW_ROI;
      else
	which_info = HELP_INFO_CANVAS_ROI;
    }

  /* quick correction so we can handle leaving a tree leaf */
  if (((which_info == HELP_INFO_TREE_VOLUME) ||
       (which_info == HELP_INFO_TREE_ROI) ||
       (which_info == HELP_INFO_TREE_STUDY)) &&
      (event->type == GDK_LEAVE_NOTIFY))
    which_info = HELP_INFO_TREE_NONE;

  ui_study_update_help_info(ui_study, which_info, realpoint_zero);

  return FALSE;
}



/* function called when an event occurs on the image canvas, 
   notes:
   -events for non-new roi's are handled by ui_study_rois_cb_roi_event 

*/
gboolean ui_study_cb_canvas_event(GtkWidget* widget,  GdkEvent * event, gpointer data) {

  ui_study_t * ui_study = data;
  realpoint_t real_loc, view_loc, canvas_rp, p0, p1, diff_rp;
  view_t i_view, view;
  canvaspoint_t canvas_cp, diff_cp;
  guint32 outline_color;
  GnomeCanvas * canvas;
  GnomeCanvasPoints * align_line_points;
  static canvaspoint_t picture_center,picture0,picture1;
  static realpoint_t center;
  static GnomeCanvasItem * new_roi = NULL;
  static GnomeCanvasItem * align_line = NULL;
  static gboolean dragging = FALSE;
  static gboolean shift = FALSE;
  static gboolean depth_change = FALSE;
  static gboolean align_vertical = FALSE;
  static realpoint_t initial_loc;

  /* sanity checks */
  if (study_volumes(ui_study->study) == NULL) return TRUE;
  if (ui_study->current_volume == NULL) return TRUE;
  if (ui_study->current_mode == ROI_MODE)
    if (ui_study->current_roi == NULL)
      ui_study->current_mode = VOLUME_MODE; /* switch back to volume mode */

  canvas = GNOME_CANVAS(widget);

  /* figure out which canvas called this (transverse/coronal/sagittal)*/
  view = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(canvas), "view"));

  /* get the location of the event, and convert it to the canvas coordinates */
  gnome_canvas_w2c_d(canvas, event->button.x, event->button.y, &canvas_cp.x, &canvas_cp.y);

  /* Convert the event location info to real units */
  canvas_rp = ui_study_cp_2_rp(ui_study, view, canvas_cp);
  real_loc = realspace_alt_coord_to_base(canvas_rp, ui_study->canvas_coord_frame[view]);
  view_loc = realspace_base_coord_to_alt(real_loc, study_coord_frame(ui_study->study));

  /* switch on the event which called this */
  switch (event->type)
    {


    case GDK_ENTER_NOTIFY:
      ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, real_loc);
      if (ui_study->current_mode == ROI_MODE) {
	/* if we're a new roi, using the drawing cursor */
	if (roi_undrawn(ui_study->current_roi))
	  ui_common_place_cursor(UI_CURSOR_NEW_ROI_MODE, GTK_WIDGET(canvas));
	//	else
	/* cursor will be this until we're actually positioned on an roi */
	//	  ui_common_place_cursor(UI_CURSOR_NO_ROI_MODE, GTK_WIDGET(canvas));
	// since we need to double the remove cursor below, it's better not to do
	// this guy, as it starts to look really stupid
      } else  /* VOLUME_MODE */
	ui_common_place_cursor(UI_CURSOR_VOLUME_MODE, GTK_WIDGET(canvas));
      
      break;


    case GDK_LEAVE_NOTIFY:
      /* yes, do it twice, there's a bug where if the canvas gets covered by a menu,
	 no GDK_LEAVE_NOTIFY is called for the GDK_ENTER_NOTIFY */
      ui_common_remove_cursor(GTK_WIDGET(canvas)); 
      ui_common_remove_cursor(GTK_WIDGET(canvas));
      break;



    case GDK_BUTTON_PRESS:
      ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, real_loc);
      /* figure out the outline color */
      outline_color = color_table_outline_color(ui_study->current_volume->color_table, TRUE);

      if (ui_study->current_mode == ROI_MODE) {
	if ((roi_undrawn(ui_study->current_roi)) && (event->button.button == 1)) {

	  /* only new roi's handled in this function, old ones handled by
	     ui_study_rois_cb_roi_event */
	  
	  /* save the center of our object in static variables for future use */
	  center = canvas_rp;
	  picture_center = picture0 = picture1 = canvas_cp;

	  /* create the new roi */
	  switch(ui_study->current_roi->type) 
	    {
	    case BOX:
	      new_roi = 
		gnome_canvas_item_new(gnome_canvas_root(canvas),
				      gnome_canvas_rect_get_type(),
				      "x1",picture0.x, "y1", picture0.y, 
				      "x2", picture1.x, "y2", picture1.y,
				      "fill_color", NULL,
				      "outline_color_rgba", outline_color,
				      "width_pixels", ui_study->roi_width,
				      NULL);
 	      
	      break;
	    case ELLIPSOID:
	    case CYLINDER:
	    default:
	      new_roi = 
		gnome_canvas_item_new(gnome_canvas_root(canvas),
				      gnome_canvas_ellipse_get_type(),
				      "x1", picture0.x, "y1", picture0.y, 
				      "x2", picture1.x, "y2", picture1.y,
				      "fill_color", NULL,
				      "outline_color_rgba", outline_color,
				      "width_pixels", ui_study->roi_width,
				      NULL);
	      break;
	    }

	  /* grab based on the new roi */
	  dragging = TRUE;
	  gnome_canvas_item_grab(GNOME_CANVAS_ITEM(new_roi),
				 GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
				 ui_common_cursor[UI_CURSOR_NEW_ROI_MOTION],
				 event->button.time);
	  
	  
	}
      } else { /* VOLUME_MODE */

	if (shift == TRUE) {
	  dragging = FALSE; /* do everything in BUTTON_RELEASE */
	  ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_ALIGN, real_loc);
	} else if (event->button.state && GDK_SHIFT_MASK) {
	  dragging = TRUE;
	  initial_loc = view_loc;
	  shift = TRUE;
	  picture0 = picture1 = canvas_cp;
	  align_line_points = gnome_canvas_points_new(2);
	  align_line_points->coords[0] = picture0.x;
	  align_line_points->coords[1] = picture0.y;
	  align_line_points->coords[2] = picture1.x;
	  align_line_points->coords[3] = picture1.y;
	  
	  align_line = gnome_canvas_item_new(gnome_canvas_root(canvas),
					     gnome_canvas_line_get_type(),
					     "points", align_line_points,
					     "fill_color_rgba", outline_color,
					     "width_pixels", ui_study->roi_width,
					     NULL);
	  
	  gnome_canvas_points_unref(align_line_points);
	  
	  /* check if we're aligning vertically or horizontally */
	  if (event->button.button == 2) 
	    align_vertical = TRUE;
	  else
	    align_vertical = FALSE;

	  ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_ALIGN, real_loc);
	} else {
	  dragging = TRUE;
	  initial_loc = view_loc;
	  shift = FALSE;

	  /* if we clicked with button 2, minimize the thickness */
	  if (event->button.button == 2) {
	    depth_change = TRUE;
	    study_set_view_thickness(ui_study->study, 0);
	    ui_study_update_thickness_adjustment(ui_study);
	  }

	  /* make them targets */
	  ui_study_update_targets(ui_study, TARGET_CREATE, view_loc, outline_color);
	}
      }
      break;

    case GDK_MOTION_NOTIFY:
      ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, real_loc);
      if (ui_study->current_mode == ROI_MODE) {
	if (dragging && (event->motion.state & GDK_BUTTON1_MASK)) {
	  if ( roi_undrawn(ui_study->current_roi)) {
	    /* only new roi's handled in this function, old ones handled by
	       ui_study_rois_cb_roi_event */
	    diff_cp = cp_diff(picture_center, canvas_cp);
	    picture0 = cp_sub(picture_center, diff_cp);
	    picture1 = cp_add(picture_center, diff_cp);
	    gnome_canvas_item_set(new_roi, 
				  "x1", picture0.x, "y1", picture0.y,
				  "x2", picture1.x, "y2", picture1.y,NULL);
	  }
	}
      } else { /* VOLUME_MODE */

	if ((shift == TRUE) && (dragging == TRUE)) {
	  picture1 = canvas_cp;
	  align_line_points = gnome_canvas_points_new(2);
	  align_line_points->coords[0] = picture0.x;
	  align_line_points->coords[1] = picture0.y;
	  align_line_points->coords[2] = picture1.x;
	  align_line_points->coords[3] = picture1.y;

	  gnome_canvas_item_set(align_line, "points", align_line_points, NULL);
	  
	  gnome_canvas_points_unref(align_line_points);
	  
	  ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_ALIGN, real_loc);
	} else {

	  if (dragging && (event->motion.state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK))) {
	    /* button one or two indicates we want to slide the target without changing the depth */
	    /* with button two, the depth should have already been changed back to the minimum */
	    ui_study_update_targets(ui_study, TARGET_UPDATE, view_loc, 0);
	  } else if (dragging && (event->motion.state & GDK_BUTTON3_MASK)) {
	    /* button three indicates we want to resize the depth */
	    depth_change = TRUE;
	    study_set_view_thickness(ui_study->study,REALPOINT_MAX_DIM(rp_diff(view_loc, initial_loc)));
	    ui_study_update_thickness_adjustment(ui_study);
	    ui_study_update_targets(ui_study, TARGET_UPDATE, initial_loc, 0);
	  }
	}
      }
      break;

    case GDK_BUTTON_RELEASE:
      ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, real_loc);
      if (ui_study->current_mode == ROI_MODE) {
	if (roi_undrawn(ui_study->current_roi) && dragging) {
	  /* only new roi's handled in this function, old ones handled by
	     ui_study_rois_cb_roi_event */
	  gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(new_roi), event->button.time);
	  dragging = FALSE;
	  
	  diff_rp = rp_diff(center, canvas_rp);
	  diff_rp.z = study_view_thickness(ui_study->study)/2.0;

	  p0 = rp_sub(center, diff_rp);
	  p1 = rp_add(center, diff_rp);

	  /* get rid of the roi drawn on the canvas */
	  gtk_object_destroy(GTK_OBJECT(new_roi));
	  
	  /* let's save the info */
	  /* we'll save the coord frame and offset of the roi */
	  rs_set_offset(&ui_study->current_roi->coord_frame,
			realspace_alt_coord_to_base(p0, ui_study->canvas_coord_frame[view]));
	  rs_set_axis(&ui_study->current_roi->coord_frame, 
		      rs_all_axis(ui_study->canvas_coord_frame[view]));

	  /* and set the far corner of the roi */
	  p1 = realspace_alt_coord_to_base(p1, ui_study->canvas_coord_frame[view]);
	  ui_study->current_roi->corner = 
	    rp_abs(realspace_base_coord_to_alt(p1,ui_study->current_roi->coord_frame));
	  
	  
	  /* and save any roi type specfic info */
	  switch(ui_study->current_roi->type) {
	  case CYLINDER:
	  case BOX:
	  case ELLIPSOID:
	  default:
	    break;
	  }
	  
	  /* update the roi's */
	  for (i_view=0;i_view<NUM_VIEWS;i_view++) {
	    ui_study_update_canvas_rois(ui_study,i_view);
	  }
	}
      } else { /* VOLUME_MODE */

	if (shift == TRUE) {
	  if (dragging != TRUE) { /* time to enact changes */

	    shift = FALSE;
	    gtk_object_destroy(GTK_OBJECT(align_line));
	    if (event->button.button == 3) { /* we want to act on our changes */
	      floatpoint_t rotation;
	      ui_volume_list_t * temp_list = ui_study->current_volumes;
	      realpoint_t volume_center, real_center;

	      /* figure out how many degrees we've rotated */
	      diff_cp = cp_sub(picture1, picture0);
	      if (align_vertical)
		rotation = -atan(diff_cp.x/diff_cp.y);
	      else
		rotation = atan(diff_cp.y/diff_cp.x);
	      
	      /* compensate for sagittal being a left-handed coordinate frame */
	      if (view == SAGITTAL) 
		rotation = -rotation; 

	      /* rotate all the currently displayed volumes */
	      while (temp_list != NULL) {

		/* saving the view center wrt to the volume's coord axis, as we're rotating around
		   the view center */
		real_center = realspace_alt_coord_to_base(study_view_center(ui_study->study),
							  study_coord_frame(ui_study->study));
		volume_center = realspace_base_coord_to_alt(real_center,
							    temp_list->volume->coord_frame);
		realspace_rotate_on_axis(&temp_list->volume->coord_frame,
					 realspace_get_view_normal(study_coord_frame_axis(ui_study->study),
								   view),
					 rotation);
  
		/* recalculate the offset of this volume based on the center we stored */
		rs_set_offset(&temp_list->volume->coord_frame, realpoint_zero);
		volume_center = realspace_alt_coord_to_base(volume_center,
							    temp_list->volume->coord_frame);
		rs_set_offset(&temp_list->volume->coord_frame, rp_sub(real_center, volume_center));

		/* I really should check to see if a volume dialog is up, and update
		   the rotations.... */

		temp_list = temp_list->next; /* and go on to the next */
	      }

	      ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_ALL);
	      ui_study_update_help_info(ui_study, HELP_INFO_CANVAS_VOLUME, real_loc);
	    }
	  }
	} else if (dragging == TRUE) { 

	  dragging = FALSE;

	  /* get rid of the "target" lines */
	  ui_study_update_targets(ui_study, TARGET_DELETE, realpoint_zero, 0);

	  /* update the view center */
	  if (event->button.button == 3)
	    study_set_view_center(ui_study->study, initial_loc);
	  else if (event->button.button == 2) {
	    /* correct for the change in depth */
	    p0 = realspace_alt_coord_to_alt(initial_loc, study_coord_frame(ui_study->study),
					    ui_study->canvas_coord_frame[view]);
	    p1 = realspace_alt_coord_to_alt(view_loc, study_coord_frame(ui_study->study),
					    ui_study->canvas_coord_frame[view]);
	    p1.z = p0.z; /* correct for any depth change */
	    view_loc = realspace_alt_coord_to_alt(p1, ui_study->canvas_coord_frame[view],
						  study_coord_frame(ui_study->study));
	    study_set_view_center(ui_study->study, view_loc);
	  } else /* button 1 */
	    study_set_view_center(ui_study->study, view_loc);


	  /* update the canvases */
	  if (depth_change == TRUE) {
	    /* need to update all canvases */
	    ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_ALL);
	  } else { /* depth_change == FALSE */
	    /* don't need to update the canvas we're currently over */
	    switch (view)
	      {
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
	}
      }
      break;
    default:
      break;
    }
  
  return FALSE;
}

/* function called when an event occurs on the roi item 
   notes:
   - new roi's are handled by ui_study_cb_canvas_event
   - widget should generally by GnomeCanvasLine type */
gboolean ui_study_cb_roi_event(GtkWidget* widget, GdkEvent * event, gpointer data) {

  GnomeCanvas * canvas;
  ui_study_t * ui_study = data;
  realpoint_t real_loc, canvas_rp; 
  view_t i_view;
  canvaspoint_t canvas_cp, diff;
  realpoint_t t[2]; /* temp variables */
  realpoint_t new_center, new_radius, roi_center, roi_radius;
  realpoint_t canvas_zoom;
  roi_t * roi;
  gint rgb_width, rgb_height;
  static realpoint_t center, radius;
  static view_t view_static;
  static realpoint_t initial_real_loc;
  static realpoint_t initial_canvas_rp;
  static canvaspoint_t last_pic_cp;
  static gboolean dragging = FALSE;
  static ui_roi_list_t * current_roi_list_item = NULL;
  static GnomeCanvasItem * roi_item = NULL;
  static double theta;
  static realpoint_t roi_zoom;
  static realpoint_t shift;


  /* sanity checks */
  if (study_volumes(ui_study->study) == NULL)  return TRUE;
  if (ui_study->current_volume == NULL) return TRUE; 
  if (ui_study->current_mode == VOLUME_MODE) return TRUE;
		   
  /* if we don't already know what roi_item we're dealing with, find out */
  if (roi_item == NULL) {
    view_static = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "view"));
    roi = gtk_object_get_data(GTK_OBJECT(widget), "roi");
    current_roi_list_item = ui_roi_list_get_ui_roi(ui_study->current_rois, roi);
  }

  canvas = GNOME_CANVAS(ui_study->canvas[view_static]);
      
  /* get the location of the event, and convert it to the canvas coordinates */
  gnome_canvas_w2c_d(canvas, event->button.x, event->button.y, &canvas_cp.x, &canvas_cp.y);
  rgb_width = gdk_pixbuf_get_width(ui_study->canvas_rgb[view_static]);
  rgb_height = gdk_pixbuf_get_height(ui_study->canvas_rgb[view_static]);

  /* Convert the event location info to real units */
  canvas_rp = ui_study_cp_2_rp(ui_study, view_static, canvas_cp);
  real_loc = realspace_alt_coord_to_base(canvas_rp, ui_study->canvas_coord_frame[view_static]);

  /* switch on the event which called this */
  switch (event->type)
    {

    case GDK_ENTER_NOTIFY:
      ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, real_loc);
      ui_common_place_cursor(UI_CURSOR_OLD_ROI_MODE, GTK_WIDGET(canvas));
      break;


    case GDK_LEAVE_NOTIFY:
      /* yes, do it twice, there's a bug where if the canvas gets covered by a menu,
	 no LEAVE_NOTIFY is called for the GDK_ENTER_NOTIFY */
      ui_common_remove_cursor(GTK_WIDGET(canvas));
      ui_common_remove_cursor(GTK_WIDGET(canvas));
      break;
      


    case GDK_BUTTON_PRESS:
      ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, real_loc);
      dragging = TRUE;

      /* last second sanity check */
      if (current_roi_list_item == NULL) {
	g_warning("%s: null current_roi_list_item?", PACKAGE);
	return TRUE;
      }

      if (event->button.button == UI_STUDY_ROIS_SHIFT_BUTTON)
	gnome_canvas_item_grab(GNOME_CANVAS_ITEM(widget),
			       GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			       ui_common_cursor[UI_CURSOR_OLD_ROI_SHIFT], event->button.time);
      else if (event->button.button == UI_STUDY_ROIS_ROTATE_BUTTON)
	gnome_canvas_item_grab(GNOME_CANVAS_ITEM(widget),
			       GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			       ui_common_cursor[UI_CURSOR_OLD_ROI_ROTATE], event->button.time);
      else if (event->button.button == UI_STUDY_ROIS_RESIZE_BUTTON)
	gnome_canvas_item_grab(GNOME_CANVAS_ITEM(widget),
			       GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			       ui_common_cursor[UI_CURSOR_OLD_ROI_RESIZE], event->button.time);
      else /* what the hell button got pressed? */
	return TRUE;


      /* save the roi we're going to manipulate for future use */
      roi_item = current_roi_list_item->canvas_roi[view_static];

      /* save some values in static variables for future use */
      initial_real_loc = real_loc;
      initial_canvas_rp = canvas_rp;
      last_pic_cp = canvas_cp;
      theta = 0.0;
      roi_zoom.x = roi_zoom.y = roi_zoom.z = 1.0;
      shift = realpoint_zero;

      t[0] = realspace_base_coord_to_alt(rs_offset(current_roi_list_item->roi->coord_frame),
					 current_roi_list_item->roi->coord_frame);
      t[1] = current_roi_list_item->roi->corner;
      center = rp_add(rp_cmult(0.5, t[1]), rp_cmult(0.5, t[0]));
      radius=rp_cmult(0.5,rp_diff(t[1], t[0]));
      break;

    case GDK_MOTION_NOTIFY:
      ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, real_loc);
      if (dragging && 
	  ((event->motion.state & 
	    (UI_STUDY_ROIS_SHIFT_MASK | UI_STUDY_ROIS_ROTATE_MASK | UI_STUDY_ROIS_RESIZE_MASK)))) {

	/* last second sanity check */
	if (current_roi_list_item == NULL) {
	  g_warning("%s: null current_roi_list_item?", PACKAGE);
	  return TRUE;
	}


	if (event->motion.state & UI_STUDY_ROIS_RESIZE_MASK) {
	  /* RESIZE button Pressed, we're scaling the object */
	  /* note, I'd like to use the function "gnome_canvas_item_scale"
	     but this isn't defined in the current version of gnome.... 
	     so I'll have to do a whole bunch of shit*/
	  /* also, this "zoom" strategy doesn't always work if the object is not
	     aligned with the view.... oh well... good enough for now... */
	  double affine[6];
	  realpoint_t item_center;
	  floatpoint_t jump_limit, max, temp, dot;
	  double cos_r, sin_r, rot;
	  axis_t i_axis, axis[NUM_AXIS];
	  canvaspoint_t radius_cp;
	  
	  /* calculating the radius and center wrt the canvas */
	  roi_radius = realspace_alt_dim_to_alt(radius, current_roi_list_item->roi->coord_frame,
						ui_study->canvas_coord_frame[view_static]);
	  roi_center = realspace_alt_coord_to_alt(center, current_roi_list_item->roi->coord_frame,
						  ui_study->canvas_coord_frame[view_static]);
	  t[0] = rp_diff(initial_canvas_rp, roi_center);
	  t[1] = rp_diff(canvas_rp,roi_center);
	  radius_cp.x = radius.x;
	  radius_cp.y = radius.y;
	  jump_limit = cp_mag(radius_cp)/3.0;

	  /* figure out the zoom we're specifying via the canvas */
	  if (t[0].x < jump_limit) /* prevent jumping */
	    canvas_zoom.x = (roi_radius.x+t[1].x)/(roi_radius.x+t[0].x);
	  else
	    canvas_zoom.x = t[1].x/t[0].x;
	  if (t[0].y < jump_limit) /* prevent jumping */
	    canvas_zoom.y = (roi_radius.x+t[1].y)/(roi_radius.x+t[0].y);
	  else
	    canvas_zoom.y = t[1].y/t[0].y;
	  
	  canvas_zoom.z = 1.0;

	  //	  g_print("---------------------- view: %d\n", view_static);
	  /* translate the canvas zoom into the ROI's coordinate frame */
	  t[0].x = t[0].y = t[0].z = 1.0;
	  t[0] = realspace_alt_dim_to_alt(t[0], ui_study->canvas_coord_frame[view_static],
					  current_roi_list_item->roi->coord_frame);
	  roi_zoom = realspace_alt_dim_to_alt(canvas_zoom, ui_study->canvas_coord_frame[view_static],
					      current_roi_list_item->roi->coord_frame);
	  roi_zoom = rp_div(roi_zoom, t[0]);

	  /* first, figure out how much the roi is rotated in the plane of the canvas */

	  /* allright, figure out which axis in the ROI is closest to the view's x axis */
	  max = 0.0;
	  axis[XAXIS]=XAXIS;
	  t[0] = realspace_get_orthogonal_view_axis(rs_all_axis(ui_study->canvas_coord_frame[view_static]),
						    view_static, XAXIS);
	  for (i_axis=0;i_axis<NUM_AXIS;i_axis++) {
	    temp = fabs(rp_dot_product(t[0],
  				       rs_specific_axis(current_roi_list_item->roi->coord_frame, i_axis)));
	    if (temp > max) {
	      max=temp;
	      axis[XAXIS]=i_axis;
	    }
	  }

	  /* and y axis */
	  max = 0.0;
	  axis[YAXIS]= (axis[XAXIS]+1 < NUM_AXIS) ? axis[XAXIS]+1 : XAXIS;
	  t[0] = realspace_get_orthogonal_view_axis(rs_all_axis(ui_study->canvas_coord_frame[view_static]), 
						    view_static, YAXIS);
	  for (i_axis=0;i_axis<NUM_AXIS;i_axis++) {
	    temp = fabs(rp_dot_product(t[0],
  				       rs_specific_axis(current_roi_list_item->roi->coord_frame, i_axis)));
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
	    t[0] = rs_specific_axis(current_roi_list_item->roi->coord_frame,axis[i_axis]);
	    	    t[0] = realspace_base_coord_to_alt(t[0], ui_study->canvas_coord_frame[view_static]);
	    t[0].z = 0.0;
	    t[1] = 
	      realspace_get_orthogonal_view_axis(rs_all_axis(ui_study->canvas_coord_frame[view_static]), 
						 view_static, i_axis);
	    t[1] = realspace_base_coord_to_alt(t[1], ui_study->canvas_coord_frame[view_static]);
	    t[1].z = 0.0;
	    //	    g_print("t[0] %5.3f %5.3f %5.3f\nt[1] %5.3f %5.3f %5.3f\n",
	    //		    t[0].x,t[0].y,t[0].z,
	    //		    t[1].x,t[1].y,t[1].z);

	    /* and get the angle the projection makes to the x axis */
	    dot = rp_dot_product(t[0],t[1])/(rp_mag(t[0]) * rp_mag(t[1]));
	    /* correct for the fact that acos is always positive by using the cross product */
	    if ((t[0].x*t[1].y-t[0].y*t[1].x) > 0.0)
	      temp = acos(dot);
	    else
	      temp = -acos(dot);
	    if (isnan(temp)) temp=0; 
	    //	    g_print("rot is %5.3f dot is %5.3f\n",temp*180/M_PI, dot);

	    rot += temp;
	  }
	  //	  rot = rot/2.0;

	  /* precompute cos and sin of rot */
	  cos_r = cos(rot);
	  sin_r = sin(rot);

	  //	  g_print("xaxis %d yaxis %d zaxis %d rot %5.3f\n", axis[0],axis[1], axis[2], 180*rot/M_PI);

	  /* figure out what the center of the roi is in gnome_canvas_item coords */
	  /* compensate for X's origin being top left (not bottom left) */
	  item_center.x = ((roi_center.x/ui_study->canvas_corner[view_static].x)*rgb_width+UI_STUDY_TRIANGLE_HEIGHT);
	  item_center.y =  rgb_height - ((roi_center.y/ui_study->canvas_corner[view_static].y) *rgb_height)
	    +UI_STUDY_TRIANGLE_HEIGHT;

	  /* do a wild ass affine matrix so that we can scale while preserving angles */
	  affine[0] = canvas_zoom.x * cos_r * cos_r + canvas_zoom.y * sin_r * sin_r;
	  affine[1] = (canvas_zoom.x-canvas_zoom.y)* cos_r * sin_r;
	  affine[2] = affine[1];
	  affine[3] = canvas_zoom.x * sin_r * sin_r + canvas_zoom.y * cos_r * cos_r;
	  affine[4] = item_center.x - item_center.x*canvas_zoom.x*cos_r*cos_r 
	    - item_center.x*canvas_zoom.y*sin_r*sin_r + (canvas_zoom.y-canvas_zoom.x)*item_center.y*cos_r*sin_r;
	  affine[5] = item_center.y - item_center.y*canvas_zoom.y*cos_r*cos_r 
	    - item_center.y*canvas_zoom.x*sin_r*sin_r + (canvas_zoom.y-canvas_zoom.x)*item_center.x*cos_r*sin_r;
	  gnome_canvas_item_affine_absolute(GNOME_CANVAS_ITEM(roi_item),affine);

	} else if (event->motion.state & UI_STUDY_ROIS_ROTATE_MASK) {
	  /* rotate button Pressed, we're rotating the object */
	  /* note, I'd like to use the function "gnome_canvas_item_rotate"
	     but this isn't defined in the current version of gnome.... 
	     so I'll have to do a whole bunch of shit*/
	  double affine[6];
	  realpoint_t item_center;

	  //	  gnome_canvas_item_i2w_affine(GNOME_CANVAS_ITEM(roi_item),affine);
	  roi_center = realspace_alt_coord_to_alt(center,
						  current_roi_list_item->roi->coord_frame,
						  ui_study->canvas_coord_frame[view_static]);
	  t[0] = rp_sub(initial_canvas_rp,roi_center);
	  t[1] = rp_sub(canvas_rp,roi_center);

	  /* figure out theta */
	  theta = acos(rp_dot_product(t[0],t[1])/(rp_mag(t[0]) * rp_mag(t[1])));
	  
	  /* correct for the fact that acos is always positive by using the cross product */
	  if ((t[0].x*t[1].y-t[0].y*t[1].x) > 0.0)
	    theta = -theta;

	  /* figure out what the center of the roi is in canvas_item coords */
	  /* compensate for x's origin being top left (ours is bottom left) */
	  item_center.x = 
	    rgb_width * roi_center.x/ui_study->canvas_corner[view_static].x +UI_STUDY_TRIANGLE_HEIGHT;
	  item_center.y = 
	    rgb_height - 
	    rgb_height * roi_center.y/ui_study->canvas_corner[view_static].y +UI_STUDY_TRIANGLE_HEIGHT;
	  affine[0] = cos(theta);
	  affine[1] = sin(theta);
	  affine[2] = -affine[1];
	  affine[3] = affine[0];
	  affine[4] = (1.0-affine[0])*item_center.x+affine[1]*item_center.y;
	  affine[5] = (1.0-affine[3])*item_center.y+affine[2]*item_center.x;
	  gnome_canvas_item_affine_absolute(GNOME_CANVAS_ITEM(roi_item),affine);

	} else if (event->motion.state & UI_STUDY_ROIS_SHIFT_MASK) {
	  /* shift button pressed, we're shifting the object */
	  /* do movement calculations */
	  shift = rp_sub(real_loc, initial_real_loc);
	  diff = cp_sub(canvas_cp, last_pic_cp);
	  /* convert the location back to world coordiantes */
	  gnome_canvas_item_i2w(GNOME_CANVAS_ITEM(roi_item)->parent, &diff.x, &diff.y);
	  gnome_canvas_item_move(GNOME_CANVAS_ITEM(roi_item),diff.x,diff.y); 
	} else {
	  g_warning("%s: reached unsuspected condition in ui_study_rois_cb.c", PACKAGE);
	  dragging = FALSE;
	  return TRUE;
	}
	last_pic_cp = canvas_cp;
      }
      break;
      
    case GDK_BUTTON_RELEASE:
      ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, real_loc);
      gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
      dragging = FALSE;
      roi_item = NULL;

      /* last second sanity check */
      if (current_roi_list_item == NULL) {
	g_warning("%s: null current_roi_list_item?", PACKAGE);
	return TRUE;
      }

      /* compensate for the fact that our origin is left bottom, not X's left top */
      /* also, compensate for sagittal being a left-handed coordinate frame */
      if (view_static != SAGITTAL)
	theta = -theta;

#ifdef AMIDE_DEBUG
      g_print("roi changes\n");
      g_print("\tshift\t%5.3f\t%5.3f\t%5.3f\n",shift.x,shift.y,shift.z);
      g_print("\troi_zoom\t%5.3f\t%5.3f\t%5.3f\n",roi_zoom.x,roi_zoom.y,roi_zoom.z);
      g_print("\ttheta\t%5.3f\n",theta*180/M_PI);
#endif

      /* apply our changes to the roi */

      if (event->button.button == UI_STUDY_ROIS_SHIFT_BUTTON) {
	/* ------------- apply any shift done -------------- */
	rs_set_offset(&current_roi_list_item->roi->coord_frame,
		      rp_add(shift, rs_offset(current_roi_list_item->roi->coord_frame)));
      } else if (event->button.button == UI_STUDY_ROIS_RESIZE_BUTTON) {
	/* ------------- apply any zoom done -------------- */
	new_radius = rp_mult(roi_zoom, radius);
	
	
	t[0] = rp_sub(center, new_radius); /* figure out the new lower left corner */

	/* the new roi offset will be the new lower left corner */
	rs_set_offset(&current_roi_list_item->roi->coord_frame,
		      realspace_alt_coord_to_base(t[0],
						  current_roi_list_item->roi->coord_frame));
	
	/* and the new upper right corner is simply twice the radius */
	t[1] = rp_cmult(2.0, new_radius);
	current_roi_list_item->roi->corner = t[1];
	
      } else if (event->button.button == UI_STUDY_ROIS_ROTATE_BUTTON) {
	
	/* ------------- apply any rotation done -------------- */
	new_center = /* get the center in real coords */
	  realspace_alt_coord_to_base(center,
				      current_roi_list_item->roi->coord_frame);
	
	/* reset the offset of the roi coord_frame to the object's center */
	rs_set_offset(&current_roi_list_item->roi->coord_frame, new_center);

	/* now rotate the roi coord_frame axis */
	realspace_rotate_on_axis(&current_roi_list_item->roi->coord_frame,
				 rs_specific_axis(ui_study->canvas_coord_frame[view_static],  ZAXIS),
				 theta);
	
	/* and recaculate the offset */
	new_center = /* get the center in roi coords */
	  realspace_base_coord_to_alt(new_center,
				      current_roi_list_item->roi->coord_frame);
	t[0] = rp_sub(new_center, radius);
	t[0] = /* get the new offset in real coords */
	  realspace_alt_coord_to_base(t[0],
				      current_roi_list_item->roi->coord_frame);
	rs_set_offset(&current_roi_list_item->roi->coord_frame, t[0]); 
	
      } else 
	return TRUE; /* shouldn't get here */

      if (event->button.button == UI_STUDY_ROIS_SHIFT_BUTTON) {
	/* if we were moving the ROI, reset the view_center to the center of the current roi */
	t[0] = realspace_base_coord_to_alt(rs_offset(current_roi_list_item->roi->coord_frame),
					   current_roi_list_item->roi->coord_frame);
	t[1] = current_roi_list_item->roi->corner;
	center = rp_add(rp_cmult(0.5, t[0]), rp_cmult(0.5, t[1]));
	center = realspace_alt_coord_to_alt(center,
					    current_roi_list_item->roi->coord_frame,
					    study_coord_frame(ui_study->study));
	study_set_view_center(ui_study->study, center);

	/* update everything */
	ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_ALL);
      } else { 
	/* we just need to update the ROI's otherwise */
	/* update the roi's */
	for (i_view=0;i_view<NUM_VIEWS;i_view++) 
	  current_roi_list_item->canvas_roi[i_view] =
	    ui_study_update_canvas_roi(ui_study,i_view,
				       current_roi_list_item->canvas_roi[i_view],
				       current_roi_list_item->roi);
      }
      
      break;
    default:
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
  ui_series_create(ui_study->study, temp_volumes, view, series_type);
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

  if (ui_study->current_mode != VOLUME_MODE) return;
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

  /* do any events pending, this allows the select state of the 
     leaf to get updated correctly with double clicks*/
  while (gtk_events_pending()) 
    gtk_main_iteration();

  /* decode the event, and figure out what we gotta do */
  switch (event->button) {

  case 1: /* left button */
    if (event->type == GDK_2BUTTON_PRESS) {
      make_active = TRUE;
      if (GTK_WIDGET_STATE(leaf) != GTK_STATE_SELECTED) {
      	select = TRUE;
      	study_leaf = gtk_object_get_data(GTK_OBJECT(ui_study->tree), "study_leaf");
      	subtree = GTK_TREE_ITEM_SUBTREE(study_leaf);
      	gtk_tree_select_child(GTK_TREE(subtree), leaf); 
      }
    } else {
      if (GTK_WIDGET_STATE(leaf) != GTK_STATE_SELECTED)
	select = TRUE;
      else
	unselect = TRUE;
    }

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
    if (GTK_WIDGET_STATE(leaf) == GTK_STATE_SELECTED)
      popup_dialog = TRUE;
    else
      popup_dialog = FALSE;
    popup_study_dialog = TRUE;
    break;
  }

  //    g_print("click event %d %d select %d unselect %d make_active %d popup %d\n",event->type, event->button,
  //  	  select, unselect, make_active, popup_dialog);

  /* do the appropriate action for the appropriate object */
  object_type = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(leaf), "type"));

  switch(object_type) {

  case UI_STUDY_TREE_VOLUME:
    volume = gtk_object_get_data(GTK_OBJECT(leaf), "object");

    if (select) {
      ui_study->current_mode = VOLUME_MODE;
      if (!ui_volume_list_includes_volume(ui_study->current_volumes, volume))
	ui_study->current_volumes = ui_volume_list_add_volume(ui_study->current_volumes, volume, leaf);

      /* if no other volume is active, make this guy active */
      if (ui_study->current_volume == NULL)
	make_active = TRUE;

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
	if (ui_study->current_mode == VOLUME_MODE)
	  ui_study_tree_update_active_leaf(ui_study, NULL); 
      } 
      
      /* destroy the threshold window if it's open and looking at this volume */
      if (ui_study->threshold_dialog != NULL)
      	if (amitk_threshold_dialog_volume(AMITK_THRESHOLD_DIALOG(ui_study->threshold_dialog)) == volume)
      	  gtk_signal_emit_by_name(GTK_OBJECT(ui_study->threshold_dialog), "delete_event");
      
      /* we'll need to redraw the canvas */
      ui_study_update_canvas(ui_study,NUM_VIEWS,UPDATE_ALL);
      ui_time_dialog_set_times(ui_study);
      
    }

    if (make_active) {
      ui_study->current_mode = VOLUME_MODE;
      ui_study->current_volume = volume;
      
      /* make sure we don't already have a volume edit dialog up */
      ui_volume_list_item = ui_volume_list_get_ui_volume(ui_study->current_volumes,ui_study->current_volume);
      if (ui_volume_list_item != NULL) {
	if (ui_volume_list_item->dialog != NULL) {
      	  gtk_signal_emit_by_name(GTK_OBJECT(ui_study->threshold_dialog), "delete_event");
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
      if (ui_study->current_roi == roi) {
	ui_study->current_roi = NULL;
	if (ui_study->current_mode == ROI_MODE) {
	  ui_study->current_mode = VOLUME_MODE;
	  ui_study_tree_update_active_leaf(ui_study, NULL); 
	}
      }
      ui_study->current_rois = ui_roi_list_remove_roi(ui_study->current_rois, roi);
    }


    if (make_active) {
      ui_study->current_mode = ROI_MODE;
      ui_study->current_roi = roi;

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
      /* indicate this is now the active leaf */
      ui_study_tree_update_active_leaf(ui_study, leaf);

    }

    if (popup_dialog) {
      ui_roi_dialog_create(ui_study, roi);
    }
    break;




  case UI_STUDY_TREE_STUDY:
  default:
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
      if (ui_study->threshold_dialog != NULL)
	gtk_signal_emit_by_name(GTK_OBJECT(ui_study->threshold_dialog), "delete_event");
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
    if (ui_study->current_roi == roi)
      ui_study->current_roi = NULL;
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



/* function ran when closing a study window */
void ui_study_cb_close(GtkWidget* widget, gpointer data) {

  GtkWidget * app = data;

  /* run the delete event function */
  gtk_signal_emit_by_name(GTK_OBJECT(app), "delete_event");

  return;
}

/* function to run for a delete_event */
gboolean ui_study_cb_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_study_t * ui_study = data;

  /* check to see if we need saving */

  /* destroy the threshold window if it's open */
  if (ui_study->threshold_dialog != NULL)
    gtk_signal_emit_by_name(GTK_OBJECT(ui_study->threshold_dialog), "delete_event");

  /* destroy the preferences dialog if it's open */
  if (ui_study->preferences_dialog != NULL)
    gtk_signal_emit_by_name(GTK_OBJECT(ui_study->preferences_dialog), "close", ui_study);

  /* destroy the time window if it's open */
  if (ui_study->time_dialog != NULL)
    gtk_signal_emit_by_name(GTK_OBJECT(ui_study->time_dialog), "close", ui_study);

  /* free our data structure's */
  ui_study = ui_study_free(ui_study);

  /* destroy the widget last */
  gtk_widget_destroy(widget);

  /* quit our app if we've closed all windows */
  number_of_windows--;
  if (number_of_windows == 0)
    gtk_main_quit();

  return FALSE;
}


























