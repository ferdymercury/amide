/* ui_study_callbacks.c
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
#include "amide.h"
#include "color_table.h"
#include "rendering.h"
#include "medcon_import.h"
#include "cti_import.h"
#include "study.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_roi.h"
#include "ui_volume.h"
#include "ui_study.h"
#include "ui_study_callbacks.h"
#include "ui_threshold2.h"
#include "ui_series2.h"
#include "ui_rendering.h"
#include "ui_roi_dialog.h"
#include "ui_study_dialog.h"
#include "ui_time_dialog.h"
#include "ui_volume_dialog.h"
#include "raw_data_import.h"
#include "idl_data_import.h"


/* function to close the file import widget */
static void ui_study_callbacks_file_selection_cancel(GtkWidget* widget, gpointer data) {

  GtkFileSelection * file_selection = data;

  gtk_grab_remove(GTK_WIDGET(file_selection));
  gtk_widget_destroy(GTK_WIDGET(file_selection));

  return;
}


/* function to handle saving */
static void ui_study_callbacks_save_as_ok(GtkWidget* widget, gpointer data) {

  GtkWidget * file_selection = data;
  GtkWidget * question;
  ui_study_t * ui_study;
  gchar * save_filename;
  gchar * temp_string;
  struct stat file_info;
  DIR * directory;
  mode_t mode = 0766;
  struct dirent * directory_entry;

  /* get a pointer to ui_study */
  ui_study = gtk_object_get_data(GTK_OBJECT(file_selection), "ui_study");

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
    question = gnome_question_dialog_modal_parented(temp_string, NULL, NULL, GTK_WINDOW(ui_study->app));
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
  ui_study_callbacks_file_selection_cancel(widget, file_selection);

  return;
}

/* function to load in an amide xml file */
void ui_study_callbacks_save_as(GtkWidget * widget, gpointer data) {
  
  ui_study_t * ui_study = data;
  GtkFileSelection * file_selection;
  gchar * temp_string;

  file_selection = GTK_FILE_SELECTION(gtk_file_selection_new(_("Save File")));

  /* take a guess at the filename */
  if (study_filename(ui_study->study) == NULL) 
    temp_string = g_strdup_printf("%s.xif", study_name(ui_study->study));
  else
    temp_string = g_strdup_printf("%s", study_filename(ui_study->study));
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selection), temp_string);
  g_free(temp_string); 

  /* save a pointer to ui_study */
  gtk_object_set_data(GTK_OBJECT(file_selection), "ui_study", ui_study);

  /* don't want anything else going on till this window is gone */
  gtk_window_set_modal(GTK_WINDOW(file_selection), TRUE);

  /* connect the signals */
  gtk_signal_connect(GTK_OBJECT(file_selection->ok_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(ui_study_callbacks_save_as_ok),
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
  gtk_grab_add(GTK_WIDGET(file_selection));

  return;
}

/* function to start the file importation process */
static void ui_study_callbacks_import_ok(GtkWidget* widget, gpointer data) {

  GtkWidget * file_selection = data;
  ui_study_t * ui_study;
  gchar * import_filename=NULL;
  gchar * import_filename_base=NULL;
  gchar * import_filename_extension=NULL;
  gchar ** frags=NULL;
  volume_t * import_volume=NULL;

  /* get a pointer to ui_study */
  ui_study = gtk_object_get_data(GTK_OBJECT(file_selection), "ui_study");

  /* get the filename */
  import_filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_selection));

  if (import_filename != NULL) {

#ifdef AMIDE_DEBUG
    g_printerr("file to import: %s\n",import_filename);
#endif


    /* extract the extension of the file */
    import_filename_base = g_basename(import_filename);
    g_strreverse(import_filename_base);
    frags = g_strsplit(import_filename_base, ".", 2);
    import_filename_extension = frags[0];
    g_strreverse(import_filename_base);
    g_strreverse(import_filename_extension);



    
    if ((g_strcasecmp(import_filename_extension, "dat")==0) ||
	(g_strcasecmp(import_filename_extension, "raw")==0))  
      { /* .dat and .raw are assumed to be raw data */
      
	/* call the corresponding import function */
	import_volume=raw_data_import(import_filename);

      }
#ifdef AMIDE_LIBECAT_SUPPORT      
    else if ((g_strcasecmp(import_filename_extension, "img")==0) ||
	     (g_strcasecmp(import_filename_extension, "v")==0))
      { /* if it appears to be a cti file, try importing it */

	/* and call that import function */
	if ((import_volume=cti_import(import_filename)) == NULL) {
	  g_warning("%s: Could not interpret file: %s as a CTI File", 
		    PACKAGE, import_filename);

	} 
      }
#endif
    else if ((g_strcasecmp(import_filename_extension, "idl")==0) ||
	     (g_strcasecmp(import_filename_extension, "ucat")==0))
      { /* IDL format data files */

	/* and call that import function */
	if ((import_volume=idl_data_import(import_filename)) == NULL) {
	  g_warning("%s: Could not interpret file: %s as an IDL File", 
		    PACKAGE, import_filename);

	} 
      }
    else /* fallback methods */

#ifdef AMIDE_LIBMDC_SUPPORT
      { /* try passing it to the libmdc library.... */
	
	/* call that import function */
	if ((import_volume=medcon_import(import_filename)) == NULL) {
	  g_warning("%s: Could not interpret file: %s using libmdc (medcon)",
		    PACKAGE, import_filename);

	} 

      }
#else
      { /* unrecognized file type */
	g_warning("%s: Cannot recognize extension %s on file: %s", 
		  PACKAGE, import_filename_extension, import_filename);
      }
#endif
    
  } else { /* no filename */
    ; 
  }
  if (import_volume != NULL) {

#ifdef AMIDE_DEBUG
    g_print("name %s\n",import_volume->name);
#endif

    /* add the volume to the study structure */
    study_add_volume(ui_study->study, import_volume);
    
    /* and add the volume to the ui_study tree structure */
    ui_study_tree_add_volume(ui_study, import_volume);
    
    /* set the thresholds */
    import_volume->threshold_max = import_volume->max;
    import_volume->threshold_min = (import_volume->min >=0) ? import_volume->min : 0;
    
    /* remove a reference to import_volume */
    import_volume = volume_free(import_volume);

    /* update the thickness widget */
    ui_study_update_thickness_adjustment(ui_study);

    /* make sure we're pointing to something sensible */
    if (study_view_center(ui_study->study).x < 0.0) {
      study_set_view_center(ui_study->study, volume_calculate_center(import_volume));
    }
    
    /* update the ui_study cavases with the new volume */
    ui_study_update_canvas(ui_study,NUM_VIEWS,UPDATE_ALL);

    /* adjust the time dialog if necessary */
    ui_time_dialog_set_times(ui_study);

  }

  /* freeup our strings, note, the other two are just pointers into these strings */
  //  g_free(import_filename); /* this causes a crash for some reason, is import_filename just
  //			    *  a reference into the file_selection box? */
  g_strfreev(frags);

  /* close the file selection box */
  ui_study_callbacks_file_selection_cancel(widget, file_selection);

  return;
}


/* function to selection which file to import */
void ui_study_callbacks_import(GtkWidget * widget, gpointer data) {
  
  ui_study_t * ui_study = data;
  GtkFileSelection * file_selection;

  file_selection = GTK_FILE_SELECTION(gtk_file_selection_new(_("Import File")));

  /* save a pointer to ui_study */
  gtk_object_set_data(GTK_OBJECT(file_selection), "ui_study", ui_study);

  /* don't want anything else going on till this window is gone */
  gtk_window_set_modal(GTK_WINDOW(file_selection), TRUE);

  /* connect the signals */
  gtk_signal_connect(GTK_OBJECT(file_selection->ok_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(ui_study_callbacks_import_ok),
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
  gtk_grab_add(GTK_WIDGET(file_selection));

  return;
}

/* callback generally attached to the entry_notify_event */
gboolean ui_study_callbacks_update_help_info(GtkWidget * widget, GdkEventCrossing * event,
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

  ui_study_update_help_info(ui_study, which_info);

  return FALSE;
}



/* function called when an event occurs on the image canvas, 
   notes:
   -events for non-new roi's are handled by ui_study_rois_callbacks_roi_event 

*/
gboolean ui_study_callbacks_canvas_event(GtkWidget* widget,  GdkEvent * event, gpointer data) {

  ui_study_t * ui_study = data;
  realpoint_t real_loc, view_loc, temp_loc;
  view_t i_view, view;
  axis_t i_axis;
  realpoint_t item, diff;
  volume_t * volume;
  guint32 outline_color;
  realspace_t * canvas_coord_frame[NUM_VIEWS];
  realpoint_t * canvas_far_corner[NUM_VIEWS];
  GdkImlibImage * rgb_image;
  GnomeCanvas * canvas;
  GnomeCanvasItem * canvas_image;
  static realpoint_t picture_center,picture0,picture1;
  static realpoint_t center, p0,p1;
  static GnomeCanvasItem * new_roi = NULL;
  static gboolean dragging = FALSE;
  static gboolean depth_change = FALSE;
  static realpoint_t initial_loc;

  /* sanity checks */
  if (study_volumes(ui_study->study) == NULL)
    return TRUE;
  if (ui_study->current_volume == NULL)
    return TRUE;
  if (ui_study->current_mode == ROI_MODE)
    if (ui_study->current_roi == NULL)
      ui_study->current_mode = VOLUME_MODE; /* switch back to volume mode */

  canvas = GNOME_CANVAS(widget);

  /* figure out which volume we're dealing with */
  volume = ui_study->current_volume;

  /* get the location of the event, and convert it to the gnome image system coordinates */
  item.x = event->button.x;
  item.y = event->button.y;
  canvas_image = gtk_object_get_data(GTK_OBJECT(canvas), "canvas_image");
  gnome_canvas_item_w2i(GNOME_CANVAS_ITEM(canvas_image)->parent, &item.x, &item.y);

  /* figure out which canvas called this (transverse/coronal/sagittal)*/
  view = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(canvas), "view"));

  /* get the coordinate frame and the far corner for each of the canvases, 
     the canvas_corner is in the canvas_coord_frame */
  for (i_view = 0; i_view < NUM_VIEWS; i_view++) {
    canvas_coord_frame[i_view] = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[i_view]), "coord_frame");
    canvas_far_corner[i_view] = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[i_view]), "far_corner");
  }

  /* need the heigth and width of the rgb_image */
  rgb_image = gtk_object_get_data(GTK_OBJECT(canvas), "rgb_image");

  /* convert the event location to view space units */
  temp_loc.x = ((item.x-UI_STUDY_TRIANGLE_HEIGHT)
		/rgb_image->rgb_width)*(*canvas_far_corner[view]).x;
  temp_loc.y = ((item.y-UI_STUDY_TRIANGLE_HEIGHT)
		/rgb_image->rgb_height)*(*canvas_far_corner[view]).y;
  temp_loc.z = study_view_thickness(ui_study->study)/2.0;

  /* Convert the event location info to real units */
  real_loc = realspace_alt_coord_to_base(temp_loc, *canvas_coord_frame[view]); 
  view_loc = realspace_base_coord_to_alt(real_loc, study_coord_frame(ui_study->study));

  /* switch on the event which called this */
  switch (event->type)
    {


    case GDK_ENTER_NOTIFY:
      ui_study_update_location_display(ui_study, real_loc);
      if (ui_study->current_mode == ROI_MODE) {
	/* if we're a new roi, using the drawing cursor */
	if (roi_undrawn(ui_study->current_roi))
	  ui_study_place_cursor(ui_study, UI_STUDY_NEW_ROI_MODE, GTK_WIDGET(canvas));
	else
	  /* cursor will be this until we're actually positioned on an roi */
	  ui_study_place_cursor(ui_study, UI_STUDY_NO_ROI_MODE, GTK_WIDGET(canvas));
      } else  /* VOLUME_MODE */
	ui_study_place_cursor(ui_study, UI_STUDY_VOLUME_MODE, GTK_WIDGET(canvas));
      
      break;


     case GDK_LEAVE_NOTIFY:
       ui_study_update_location_display(ui_study, 
					realspace_alt_coord_to_base(study_view_center(ui_study->study),
								    study_coord_frame(ui_study->study)));
       ui_study_remove_cursor(ui_study, GTK_WIDGET(canvas));
       break;



    case GDK_BUTTON_PRESS:
      ui_study_update_location_display(ui_study, real_loc);
      /* figure out the outline color */
      outline_color = color_table_outline_color(volume->color_table, TRUE);

      if (ui_study->current_mode == ROI_MODE) {
	if ((roi_undrawn(ui_study->current_roi)) && (event->button.button == 1)) {
	  /* only new roi's handled in this function, old ones handled by
	     ui_study_rois_callbacks_roi_event */
	  
	  dragging = TRUE;
	  gnome_canvas_item_grab(GNOME_CANVAS_ITEM(canvas_image),
				 GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
				 ui_study->cursor[UI_STUDY_NEW_ROI_MOTION],
				 event->button.time);
	  
	  
	  /* save the center of our object in static variables for future use */
	  center = real_loc;
	  picture_center = picture0 = picture1 = item;
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
				      "width_units", 1.0,
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
				      "width_units", 1.0,
				      NULL);
	      break;
	    }
	}
      } else { /* VOLUME_MODE */

	dragging = TRUE;
	initial_loc = view_loc;

	/* if we clicked with button 2, minimize the thickness */
	if (event->button.button == 2) {
	  depth_change = TRUE;
	  study_set_view_thickness(ui_study->study, 0);
	  ui_study_update_thickness_adjustment(ui_study);
	}

	/* make them targets */
	ui_study_update_targets(ui_study, TARGET_CREATE, view_loc, outline_color);

      }
      break;

    case GDK_MOTION_NOTIFY:
      ui_study_update_location_display(ui_study, real_loc);
      if (ui_study->current_mode == ROI_MODE) {
	if (dragging && (event->motion.state & GDK_BUTTON1_MASK)) {
	  if ( roi_undrawn(ui_study->current_roi)) {
	    /* only new roi's handled in this function, old ones handled by
	       ui_study_rois_callbacks_roi_event */
	    REALPOINT_DIFF(picture_center, item, diff);
	    REALPOINT_SUB(picture_center, diff, picture0);
	    REALPOINT_ADD(picture_center, diff, picture1);
	    gnome_canvas_item_set(new_roi, 
				  "x1", picture0.x, "y1", picture0.y,
				  "x2", picture1.x, "y2", picture1.y,NULL);
	  }
	}
      } else { /* VOLUME_MODE */

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
      break;

    case GDK_BUTTON_RELEASE:
      ui_study_update_location_display(ui_study, real_loc);
      if (ui_study->current_mode == ROI_MODE) {
	if (roi_undrawn(ui_study->current_roi) && dragging) {
	  /* only new roi's handled in this function, old ones handled by
	     ui_study_rois_callbacks_roi_event */
	  gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(canvas_image), event->button.time);
	  dragging = FALSE;
	  
	  REALPOINT_DIFF(center, real_loc, diff);
	  
	  switch (view)
	    {
	    case TRANSVERSE:
	      diff.z = study_view_thickness(ui_study->study)/2.0;
	      break;
	    case CORONAL:
	      diff.y = study_view_thickness(ui_study->study)/2.0;
	      break;
	    case SAGITTAL:
	    default:
	      diff.x = study_view_thickness(ui_study->study)/2.0;
	      break;
	    }
	  
	  REALPOINT_SUB(center, diff, p0);
	  REALPOINT_ADD(center, diff, p1);

	  /* get ride of the roi drawn on the canvas */
	  gtk_object_destroy(GTK_OBJECT(new_roi));
	  
	  /* let's save the info */
	  /* we'll save the coord frame and offset of the roi */
	  ui_study->current_roi->coord_frame.offset = p0;
	  for (i_axis=0;i_axis<NUM_AXIS;i_axis++)
	    ui_study->current_roi->coord_frame.axis[i_axis] = (*canvas_coord_frame[view]).axis[i_axis];

	  /* and set the far corner of the roi */
	  ui_study->current_roi->corner = 
	    realspace_base_coord_to_alt(p1,ui_study->current_roi->coord_frame);
	  
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

	dragging = FALSE;

	/* get rid of the "target" lines */
	ui_study_update_targets(ui_study, TARGET_DELETE, realpoint_init, 0);

	/* update the view center if it's been changed */
	if (event->button.button != 3)
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
      break;
    default:
      break;
    }
      
  return FALSE;
}


/* function called indicating the plane adjustment has changed */
void ui_study_callbacks_plane_change(GtkObject * adjustment, gpointer data) {

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

void ui_study_callbacks_zoom(GtkObject * adjustment, gpointer data) {

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


void ui_study_callbacks_time_pressed(GtkWidget * combo, gpointer data) {
  
  ui_study_t * ui_study = data;

  /* sanity check */
  ui_time_dialog_create(ui_study);

  return;

}


void ui_study_callbacks_thickness(GtkObject * adjustment, gpointer data) {

  ui_study_t * ui_study = data;

  /* sanity check */
  if (study_volumes(ui_study->study) == NULL)
    return;

  if (study_view_thickness(ui_study->study) != GTK_ADJUSTMENT(adjustment)->value) {
    study_set_view_thickness(ui_study->study, GTK_ADJUSTMENT(adjustment)->value);
    ui_study_update_canvas(ui_study,NUM_VIEWS, UPDATE_ALL);
  }

  return;
}


/* callbacks for setting up a set of slices in a new window */
void ui_study_callbacks_transverse_series_planes(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  ui_series_create(ui_study, TRANSVERSE, PLANES);
  return;
}
void ui_study_callbacks_coronal_series_planes(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  ui_series_create(ui_study, CORONAL, PLANES);
  return;
}
void ui_study_callbacks_sagittal_series_planes(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  ui_series_create(ui_study, SAGITTAL, PLANES);
  return;
}
void ui_study_callbacks_transverse_series_frames(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  ui_series_create(ui_study, TRANSVERSE, FRAMES);
  return;
}
void ui_study_callbacks_coronal_series_frames(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  ui_series_create(ui_study, CORONAL, FRAMES);
  return;
}
void ui_study_callbacks_sagittal_series_frames(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  ui_series_create(ui_study, SAGITTAL, FRAMES);
  return;
}

#ifdef AMIDE_LIBVOLPACK_SUPPORT
/* callback for starting up volume rendering */
void ui_study_callbacks_rendering(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  volume_list_t * temp_volumes;

  temp_volumes = ui_volume_list_return_volume_list(ui_study->current_volumes);
  ui_rendering_create(temp_volumes, study_coord_frame(ui_study->study),
		      study_view_time(ui_study->study), 
		      study_view_duration(ui_study->study));
  temp_volumes = volume_list_free(temp_volumes);

  return;
}
#endif

/* function called when hitting the threshold button, pops up a dialog */
void ui_study_callbacks_threshold_pressed(GtkWidget * button, gpointer data) {
  ui_study_t * ui_study = data;
  ui_threshold_dialog_create(ui_study);
  return;
}

/* function to switch to normalizing per slice */
void ui_study_callbacks_scaling(GtkWidget * widget, gpointer data) {

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
    if (ui_study->series != NULL)
      ui_series_update_canvas_image(ui_study);
  }
  
  return;
}

/* function to change the color table */
void ui_study_callbacks_color_table(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  color_table_t i_color_table;
  volume_t * volume;
  ui_volume_list_t * ui_volume_list_item;
  ui_threshold_t * ui_threshold;

  /* figure out which volume we're dealing with */
  ui_threshold = gtk_object_get_data(GTK_OBJECT(widget),"threshold");
  if (ui_threshold != NULL)
    volume = ui_threshold->volume;
  else
    if (ui_study->current_volume == NULL)
      return;
    else
      volume = ui_study->current_volume;


  /* figure out which scaling menu item called me */
  i_color_table = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"color_table"));

  /* check if we actually changed values */
  if (volume->color_table != i_color_table) {
    /* and inact the changes */
    volume->color_table = i_color_table;
    if (study_volumes(ui_study->study) != NULL) {
      
      /* update the canvas */
      ui_study_update_canvas(ui_study, NUM_VIEWS, REFRESH_IMAGE);
      ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_ROIS);
      
      /* update the main color table box if needed */
      if (volume == ui_study->current_volume)
	gtk_option_menu_set_history(GTK_OPTION_MENU(ui_study->color_table_menu),
				    volume->color_table);
      
      /* update the threshold dialog box if needed */
      if (ui_study->threshold != NULL) 
	if (ui_study->threshold->volume == volume) {
	  ui_threshold_update_canvas(ui_study, ui_study->threshold);
	  gtk_option_menu_set_history(GTK_OPTION_MENU(ui_study->threshold->color_table_menu),
				      volume->color_table);
	}
      
      /* update the volume dialog box if needed */
      if (volume == ui_study->current_volume) {
	ui_volume_list_item = ui_volume_list_get_ui_volume(ui_study->current_volumes,volume);
	if (ui_volume_list_item != NULL)
	  if (ui_volume_list_item->dialog != NULL)
	    if (ui_volume_list_item->threshold != NULL) {
	      ui_threshold_update_canvas(ui_study, ui_volume_list_item->threshold);
	      gtk_option_menu_set_history(GTK_OPTION_MENU(ui_volume_list_item->threshold->color_table_menu),
					  volume->color_table);
	    }
      }
      
      /* update the series if needed */
      if (ui_study->series != NULL)
	ui_series_update_canvas_image(ui_study);
    }
  }
  
  return;
}

/* function which gets called from the roi name entry dialog */
void ui_study_callbacks_roi_entry_name(gchar * entry_string, gpointer data) {
  gchar ** p_roi_name = data;
  *p_roi_name = entry_string;
  return;
}


/* callback function from the "add roi" pull down menu to add an roi */
void ui_study_callbacks_add_roi_type(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  roi_type_t i_roi_type;
  GtkWidget * entry;
  gchar * roi_name = NULL;
  gint entry_return;
  roi_t * roi;

  /* figure out which menu item called me */
  i_roi_type = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"roi_type"));

  entry = gnome_request_dialog(FALSE, "ROI Name:", "", 256, 
			       ui_study_callbacks_roi_entry_name,
			       &roi_name, GTK_WINDOW(ui_study->app));
  
  entry_return = gnome_dialog_run_and_close(GNOME_DIALOG(entry));
  
  if (entry_return == 0) {
    roi = roi_init();
    roi_set_name(roi,roi_name);
    roi->type = i_roi_type;
    roi->grain = ui_study->default_roi_grain;
    study_add_roi(ui_study->study, roi);
    ui_study_tree_add_roi(ui_study, roi);
    roi = roi_free(roi);
  }

  return;
}


/* callback used when a leaf is clicked on, 
   this handles double-clicks, middle clicks, and right clicks  */
void ui_study_callbacks_tree_leaf_clicked(GtkWidget * leaf, GdkEventButton * event, gpointer data) {

  ui_study_t * ui_study = data;
  ui_study_tree_object_t object_type;
  volume_t * volume;
  roi_t * roi;
  GtkWidget * subtree;
  GtkWidget * study_leaf;
  realpoint_t roi_center;


  if ((event->type == GDK_BUTTON_PRESS) && (event->button == 1))
    return; /* the select callback handles this case */

  object_type = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(leaf), "type"));

  switch(object_type) {

  case UI_STUDY_TREE_VOLUME:
    volume = gtk_object_get_data(GTK_OBJECT(leaf), "object");


    if ((event->button == 1) || (event->button == 2)) {
      ui_study->current_mode = VOLUME_MODE;
      ui_study->current_volume = volume;

      /* make sure it's already selected */
      if ((GTK_WIDGET_STATE(leaf) != GTK_STATE_SELECTED) && (event->type != GDK_2BUTTON_PRESS)) {
	study_leaf = gtk_object_get_data(GTK_OBJECT(ui_study->tree), "study_leaf");
	subtree = GTK_TREE_ITEM_SUBTREE(study_leaf);
	gtk_tree_select_child(GTK_TREE(subtree), leaf); 
      }

      /* reset the threshold widget based on the current volume */
      if (ui_study->threshold != NULL) 
	ui_threshold_dialog_update(ui_study);

      /* reset the color_table picker based on the current volume */
      gtk_option_menu_set_history(GTK_OPTION_MENU(ui_study->color_table_menu), volume->color_table);

      /* indicate this is now the active object */
      ui_study_tree_update_active_leaf(ui_study, leaf);

    } else {/* event.button == 3 */
      ui_volume_dialog_create(ui_study, volume); /* start up the volume dialog */
    }
    break;


  case UI_STUDY_TREE_ROI:
    roi = gtk_object_get_data(GTK_OBJECT(leaf), "object");

    if ((event->button == 1) || (event->button == 2)) {
      ui_study->current_mode = ROI_MODE;
      ui_study->current_roi = roi;

      /* make sure it's already selected */
      if ((GTK_WIDGET_STATE(leaf) != GTK_STATE_SELECTED) && (event->type != GDK_2BUTTON_PRESS)) {
	study_leaf = gtk_object_get_data(GTK_OBJECT(ui_study->tree), "study_leaf");
	subtree = GTK_TREE_ITEM_SUBTREE(study_leaf);
	gtk_tree_select_child(GTK_TREE(subtree), leaf); 
      }

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

    } else { /* button 3 clicked */
      /* start up the dialog to adjust the roi */
      ui_roi_dialog_create(ui_study, roi);
    }

    break;


  case UI_STUDY_TREE_STUDY:
  default:
    if (event->button ==3)
      ui_study_dialog_create(ui_study);
    break;
  }

  return;
}

/* callback function used when a  row in the tree is selected */
void ui_study_callbacks_tree_select(GtkTree * tree, GtkWidget * leaf, gpointer data) {

  ui_study_t * ui_study = data;
  ui_study_tree_object_t object_type;
  volume_t * volume;
  roi_t * roi;
  GnomeCanvasItem * roi_canvas_item[NUM_VIEWS];
  view_t i_view;

  object_type = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(leaf), "type"));

  switch(object_type) {



  case UI_STUDY_TREE_VOLUME:
    volume = gtk_object_get_data(GTK_OBJECT(leaf), "object");

    /* --------------------- select ----------------- */
    if (GTK_WIDGET_STATE(leaf) == GTK_STATE_SELECTED) {

      if (!ui_volume_list_includes_volume(ui_study->current_volumes, volume))
	ui_study->current_volumes = ui_volume_list_add_volume(ui_study->current_volumes, 
							      volume, leaf);

      if (ui_study->current_volume == NULL) {
	/* make this the current volume if we don't have one yet */
	ui_study->current_volume = volume;
	/* reset the color_table picker based on the current volume */
	gtk_option_menu_set_history(GTK_OPTION_MENU(ui_study->color_table_menu),
				    ui_study->current_volume->color_table);
	/* indicate this is now the active if we're not pointing at anything else leaf */
	if (ui_study->current_mode == VOLUME_MODE)
	  ui_study_tree_update_active_leaf(ui_study, leaf);
      }	  


      /*---------------------- unselect --------------- */
    } else { 
      ui_study->current_volumes =  /* remove the volume from the selection list */
	ui_volume_list_remove_volume(ui_study->current_volumes, volume);
      
      /* if it's the currently active volume... */
      if (ui_study->current_volume == volume) {
	/* reset the currently active volume */
	ui_study->current_volume = ui_volume_list_get_first_volume(ui_study->current_volumes);
	/* reset the color_table picker based on the current volume */
	if (ui_study->current_volume != NULL)
	  gtk_option_menu_set_history(GTK_OPTION_MENU(ui_study->color_table_menu),
				      ui_study->current_volume->color_table);
	/* update which row in the tree has the active symbol */
	if (ui_study->current_mode == VOLUME_MODE)
	  ui_study_tree_update_active_leaf(ui_study, NULL); 
      } 
      
      /* destroy the threshold window if it's open and looking at this volume */
      if (ui_study->threshold != NULL)
	if (ui_study->threshold->volume== volume)
	  gtk_signal_emit_by_name(GTK_OBJECT(ui_study->threshold->app), "delete_event", NULL, ui_study);
      
    }

    /* we'll need to redraw the canvas */
    ui_study_update_canvas(ui_study,NUM_VIEWS,UPDATE_ALL);
    ui_time_dialog_set_times(ui_study);
    break;




  case UI_STUDY_TREE_ROI:
    roi = gtk_object_get_data(GTK_OBJECT(leaf), "object");

    /* --------------------- select ----------------- */
    if ((GTK_WIDGET_STATE(leaf) == GTK_STATE_SELECTED) && 
	(!ui_roi_list_includes_roi(ui_study->current_rois, roi))) {
      /* make the canvas graphics */
      for (i_view=0;i_view<NUM_VIEWS;i_view++)
	roi_canvas_item[i_view] = ui_study_update_canvas_roi(ui_study,i_view,NULL, roi);
      /* and add this roi to the current_rois list */
      ui_study->current_rois =  ui_roi_list_add_roi(ui_study->current_rois, roi,
						    roi_canvas_item, leaf);
      

      /*---------------------- unselect --------------- */
    } else { 

      if (ui_study->current_roi == roi) {
	ui_study->current_roi = NULL;
	if (ui_study->current_mode == ROI_MODE) {
	  ui_study->current_mode = VOLUME_MODE;
	  ui_study_tree_update_active_leaf(ui_study, NULL); 
	}
      }
      ui_study->current_rois = ui_roi_list_remove_roi(ui_study->current_rois, roi);
    }
    break;





  case UI_STUDY_TREE_STUDY:
  default:
    ; 
    break;
  }

  return;
}




/* callback used when the background of the tree is clicked on */
void ui_study_callbacks_tree_clicked(GtkWidget * leaf, GdkEventButton * event, gpointer data) {

  ui_study_t * ui_study = data;
  GtkWidget * menu;
  GtkWidget * menuitem;
  roi_type_t i_roi_type;
  gchar * temp_string;

  if (event->type != GDK_BUTTON_PRESS)
    return;

  if (event->button == 3) {
    menu = gtk_menu_new();

    for (i_roi_type=0; i_roi_type<NUM_ROI_TYPES; i_roi_type++) {
      temp_string = g_strdup_printf("add new %s",roi_type_names[i_roi_type]);
      menuitem = gtk_menu_item_new_with_label(temp_string);
      g_free(temp_string);
      gtk_menu_append(GTK_MENU(menu), menuitem);
      gtk_object_set_data(GTK_OBJECT(menuitem), "roi_type", GINT_TO_POINTER(i_roi_type)); 
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
			 GTK_SIGNAL_FUNC(ui_study_callbacks_add_roi_type), ui_study);
      gtk_widget_show(menuitem);
    }
    gtk_widget_show(menu);
    
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
  }

  return;
}

/* callback functions for the add_roi menu items */
void ui_study_callbacks_new_roi_ellipsoid(GtkWidget * widget, gpointer data) {
  
  gtk_object_set_data(GTK_OBJECT(widget), "roi_type", GINT_TO_POINTER(ELLIPSOID));
  ui_study_callbacks_add_roi_type(widget, data);
  
  return;
}
/* callback functions for the add_roi menu items */
void ui_study_callbacks_new_roi_cylinder(GtkWidget * widget, gpointer data) {
  
  gtk_object_set_data(GTK_OBJECT(widget), "roi_type", GINT_TO_POINTER(CYLINDER));
  ui_study_callbacks_add_roi_type(widget, data);
  
  return;
}
/* callback functions for the add_roi menu items */
void ui_study_callbacks_new_roi_box(GtkWidget * widget, gpointer data) {
  
  gtk_object_set_data(GTK_OBJECT(widget), "roi_type", GINT_TO_POINTER(BOX));
  ui_study_callbacks_add_roi_type(widget, data);
  
  return;
}

/* function called to edit selected objects in the study */
void ui_study_callbacks_edit_objects(GtkWidget * button, gpointer data) {

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
void ui_study_callbacks_delete_objects(GtkWidget * button, gpointer data) {

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
  question = gnome_ok_cancel_dialog_modal_parented(message, NULL, NULL, GTK_WINDOW(ui_study->app));
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
      if (ui_study->threshold != NULL)
	gtk_signal_emit_by_name(GTK_OBJECT(ui_study->threshold->app), "delete_event", NULL, ui_study);
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
void ui_study_callbacks_interpolation(GtkWidget * widget, gpointer data) {

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


/* function to run for a delete_event */
void ui_study_callbacks_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_study_t * ui_study = data;

  /* check to see if we need saving */

  /* destroy the threshold window if it's open */
  if (ui_study->threshold != NULL)
    gtk_signal_emit_by_name(GTK_OBJECT(ui_study->threshold->app), "delete_event", 
			    NULL, ui_study);

  /* destroy the series window if it's open */
  if (ui_study->series != NULL)
    gtk_signal_emit_by_name(GTK_OBJECT(ui_study->series->app), "delete_event", NULL, ui_study);

  /* destroy the time window if it's open */
  if (ui_study->time_dialog != NULL)
    gtk_signal_emit_by_name(GTK_OBJECT(ui_study->time_dialog), "close", ui_study);

  /* destroy the widget */
  gtk_widget_destroy(widget);

  /* free our data structure's */
  ui_study = ui_study_free(ui_study);

  return;
}

/* function ran when closing a study window */
void ui_study_callbacks_close_event(GtkWidget* widget, gpointer data) {

  ui_study_t * ui_study = data;

  /* run the delete event function */
  gtk_signal_emit_by_name(GTK_OBJECT(ui_study->app), "delete_event", NULL, ui_study);

  return;
}


























