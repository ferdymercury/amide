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
#include "volume.h"
#include "rendering.h"
#include "medcon_import.h"
#include "cti_import.h"
#include "roi.h"
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
    g_warning("%s: Inappropriate filename: %s\n",PACKAGE, save_filename);
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

    /* and start deleting everything in the filename/directory */
    if (S_ISDIR(file_info.st_mode)) {
      directory = opendir(save_filename);
      while ((directory_entry = readdir(directory)) != NULL) {
	temp_string = 
	  g_strdup_printf("%s%s%s", save_filename,G_DIR_SEPARATOR_S, directory_entry->d_name);

	if (fnmatch("*.xml",directory_entry->d_name,FNM_NOESCAPE) == 0) 
	  if (unlink(temp_string) != 0)
	    g_warning("%s: Couldn't unlink file: %s\n",PACKAGE, temp_string);
	if (fnmatch("*.dat",directory_entry->d_name,FNM_NOESCAPE) == 0) 
	  if (unlink(temp_string) != 0)
	    g_warning("%s: Couldn't unlink file: %s\n",PACKAGE, temp_string);

	g_free(temp_string);
      }

    } else if (S_ISREG(file_info.st_mode)) {
      if (unlink(save_filename) != 0)
	g_warning("%s: Couldn't unlink file: %s\n",PACKAGE, save_filename);

    } else {
      g_warning("%s: Unrecognized file type for file: %s, couldn't delete\n",PACKAGE, save_filename);
      return;
    }

  } else {
    /* make the directory */
    if (mkdir(save_filename, mode) != 0) {
      g_warning("%s: Couldn't create amide directory: %s\n",PACKAGE, save_filename);
      return;
    }
  }

  /* allright, save our study */
  if (study_write_xml(ui_study->study, save_filename) == FALSE) {
    g_warning("%s: Failure Saving File: %s\n",PACKAGE, save_filename);
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
  if (study_get_filename(ui_study->study) == NULL) 
    temp_string = g_strdup_printf("%s.xif", study_get_name(ui_study->study));
  else
    temp_string = g_strdup_printf("%s", study_get_filename(ui_study->study));
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
	g_warning("%s: Cannot recognize extension %s on file: %s\n", 
		  PACKAGE, import_filename_extension, import_filename);
      }
#endif
    
  } else { /* no filename */
    ; 
  }

  g_print("name %s\n",import_volume->name);
  if (import_volume != NULL) {

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
    if (ui_study->current_view_center.x < 0.0) {
      ui_study->current_view_center = volume_calculate_center(import_volume);
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


/* function called when an event occurs on the image canvas, note, events for 
   non-new roi's are handled by ui_study_rois_callbacks_roi_event */
gint ui_study_callbacks_canvas_event(GtkWidget* widget,  GdkEvent * event, gpointer data) {

  ui_study_t * ui_study = data;
  realpoint_t real_loc, view_loc, temp_loc;
  view_t i_view, view;
  axis_t i_axis;
  realpoint_t item, diff;
  volume_t * volume;
  guint32 outline_color;
  GdkCursor * cursor;
  GnomeCanvas * canvas;
  realspace_t view_coord_frame;
  realpoint_t far_corner;
  static realpoint_t picture_center,picture0,picture1;
  static realpoint_t center, p0,p1;
  static GnomeCanvasItem * new_roi = NULL;
  static gboolean dragging = FALSE;
  
  /* sanity checks */
  if (study_get_volumes(ui_study->study) == NULL)
    return TRUE;
  if (ui_study->current_volumes == NULL)
    return TRUE;
  if (ui_study->current_mode == ROI_MODE)
    if (ui_study->current_roi == NULL)
      return TRUE;

  /* figure out which volume we're dealing with */
  if (ui_study->current_volume == NULL)
    volume = ui_study->current_volumes->volume;
  else
    volume = ui_study->current_volume;

  /* get the location of the event, and convert it to the gnome image system 
     coordinates */
  item.x = event->button.x;
  item.y = event->button.y;
  gnome_canvas_item_w2i(GNOME_CANVAS_ITEM(widget)->parent, &item.x, &item.y);

  /* figure out which canvas called this (transverse/coronal/sagittal)*/
  view = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "view"));
  canvas = ui_study->canvas[view];

  /* get the coordinate frame for this view and the far_corner
     the far_corner is in the view_coord_frame */
  view_coord_frame = ui_study_get_coords_current_view(ui_study, view, &far_corner);

  /* convert the event location to view space units */
  temp_loc.x = ((item.x-UI_STUDY_TRIANGLE_HEIGHT)
		/ui_study->rgb_image[view]->rgb_width)*far_corner.x;
  temp_loc.y = ((item.y-UI_STUDY_TRIANGLE_HEIGHT)
		/ui_study->rgb_image[view]->rgb_height)*far_corner.y;
  temp_loc.z = ui_study->current_thickness/2.0;

  /* Convert the event location info to real units */
  real_loc = realspace_alt_coord_to_base(temp_loc, view_coord_frame);
  view_loc = realspace_base_coord_to_alt(real_loc, study_get_coord_frame(ui_study->study));

  /* switch on the event which called this */
  switch (event->type)
    {


    case GDK_ENTER_NOTIFY:
      
      if (ui_study->current_mode == ROI_MODE)
	/* if we're a new roi, using the drawing cursor */
	if (roi_undrawn(ui_study->current_roi))
	  cursor = ui_study->cursor[UI_STUDY_NEW_ROI_MODE];
	else
	  /* cursor will be this until we're actually positioned on an roi */
	  cursor = ui_study->cursor[UI_STUDY_NO_ROI_MODE];
      else  /* VOLUME_MODE */
	cursor = ui_study->cursor[UI_STUDY_VOLUME_MODE];
      
      /* push our desired cursor onto the cursor stack */
      ui_study->cursor_stack = g_slist_prepend(ui_study->cursor_stack,cursor);
      gdk_window_set_cursor(gtk_widget_get_parent_window(GTK_WIDGET(canvas)), cursor);

      break;


     case GDK_LEAVE_NOTIFY:
       /* pop the previous cursor off the stack */
       cursor = g_slist_nth_data(ui_study->cursor_stack, 0);
       ui_study->cursor_stack = g_slist_remove(ui_study->cursor_stack, cursor);
       cursor = g_slist_nth_data(ui_study->cursor_stack, 0);
       gdk_window_set_cursor(gtk_widget_get_parent_window(GTK_WIDGET(canvas)), cursor);
       break;



    case GDK_BUTTON_PRESS:
      if (ui_study->current_mode == ROI_MODE) {
	if ( roi_undrawn(ui_study->current_roi)) {
	  /* only new roi's handled in this function, old ones handled by
	     ui_study_rois_callbacks_roi_event */
	  
	  dragging = TRUE;
	  cursor = ui_study->cursor[UI_STUDY_NEW_ROI_MOTION];
	  ui_study->cursor_stack = g_slist_prepend(ui_study->cursor_stack,cursor);
	  gnome_canvas_item_grab(GNOME_CANVAS_ITEM(widget),
				 GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
				 cursor,
				 event->button.time);
	  
	  /* figure out the outline color */
	  outline_color = color_table_outline_color(volume->color_table, TRUE);
	  
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

	switch (view)
	  {
	  case TRANSVERSE:
	    ui_study->current_view_center.x = view_loc.x;
	    ui_study->current_view_center.y = view_loc.y;
	    ui_study_update_canvas(ui_study, CORONAL, UPDATE_ALL);
	    ui_study_update_canvas(ui_study, SAGITTAL, UPDATE_ALL);
	    ui_study_update_canvas(ui_study, TRANSVERSE, UPDATE_PLANE_ADJUSTMENT);
	    ui_study_update_canvas(ui_study, TRANSVERSE, UPDATE_ARROWS);
	    break;
	  case CORONAL:
	    ui_study->current_view_center.x = view_loc.x;
	    ui_study->current_view_center.z = view_loc.z;
	    ui_study_update_canvas(ui_study, TRANSVERSE, UPDATE_ALL);
	    ui_study_update_canvas(ui_study, SAGITTAL, UPDATE_ALL);
	    ui_study_update_canvas(ui_study, CORONAL, UPDATE_PLANE_ADJUSTMENT);
	    ui_study_update_canvas(ui_study, CORONAL, UPDATE_ARROWS);
	    break;
	  case SAGITTAL:
	    ui_study->current_view_center.y = view_loc.y;
	    ui_study->current_view_center.z = view_loc.z;
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

    case GDK_MOTION_NOTIFY:
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
	;
      }

      break;

    case GDK_BUTTON_RELEASE:
      if (ui_study->current_mode == ROI_MODE) {
	if ( roi_undrawn(ui_study->current_roi)) {
	  /* only new roi's handled in this function, old ones handled by
	     ui_study_rois_callbacks_roi_event */
	  gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget),
				   event->button.time);
	  /* pop the previous cursor off the stack */
	  cursor = g_slist_nth_data(ui_study->cursor_stack, 0);
	  ui_study->cursor_stack = g_slist_remove(ui_study->cursor_stack, cursor);
	  cursor = g_slist_nth_data(ui_study->cursor_stack, 0);
	  gdk_window_set_cursor(gtk_widget_get_parent_window(GTK_WIDGET(canvas)), cursor);
	  dragging = FALSE;
	  
	  REALPOINT_DIFF(center, real_loc, diff);
	  
	  switch (view)
	    {
	    case TRANSVERSE:
	      diff.z = ui_study->current_thickness/2.0;
	      break;
	    case CORONAL:
	      diff.y = ui_study->current_thickness/2.0;
	      break;
	    case SAGITTAL:
	    default:
	      diff.x = ui_study->current_thickness/2.0;
	      break;
	    }
	  
	  REALPOINT_SUB(center, diff, p0);
	  REALPOINT_ADD(center, diff, p1);
	  
	  /* let's save the info */
	  gtk_object_destroy(GTK_OBJECT(new_roi));
	  
	  /* we'll save the coord frame and offset of the roi */
	  ui_study->current_roi->coord_frame.offset = p0;
	  for (i_axis=0;i_axis<NUM_AXIS;i_axis++)
	    ui_study->current_roi->coord_frame.axis[i_axis] = view_coord_frame.axis[i_axis];

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
	;
      }
      break;
    default:
      break;
    }
      
  return FALSE;
}


/* function called indicating the plane adjustment has changed */
void ui_study_callbacks_plane_change(GtkAdjustment * adjustment, gpointer data) {

  ui_study_t * ui_study = data;
  view_t i_view;

  /* sanity check */
  if (study_get_volumes(ui_study->study) == NULL)
    return;
  if (ui_study->current_volumes == NULL)
    return;

  /* figure out which scale widget called me */
  i_view = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(adjustment),"view"));

  switch(i_view) {
  case TRANSVERSE:
    ui_study->current_view_center.z = adjustment->value;
    ui_study_update_canvas(ui_study,TRANSVERSE,UPDATE_ALL);
    ui_study_update_canvas(ui_study,CORONAL,UPDATE_ARROWS);
    ui_study_update_canvas(ui_study,SAGITTAL,UPDATE_ARROWS);
    break;
  case CORONAL:
    ui_study->current_view_center.y = adjustment->value;
    ui_study_update_canvas(ui_study,CORONAL,UPDATE_ALL);
    ui_study_update_canvas(ui_study,TRANSVERSE,UPDATE_ARROWS);
    ui_study_update_canvas(ui_study,SAGITTAL,UPDATE_ARROWS);
    break;
  case SAGITTAL:
    ui_study->current_view_center.x = adjustment->value;
    ui_study_update_canvas(ui_study,SAGITTAL,UPDATE_ALL);
    ui_study_update_canvas(ui_study,TRANSVERSE,UPDATE_ARROWS);
    ui_study_update_canvas(ui_study,CORONAL,UPDATE_ARROWS);
    break;
  default:
    break;
  }

  return;
}

void ui_study_callbacks_zoom(GtkAdjustment * adjustment, gpointer data) {

  ui_study_t * ui_study = data;

  /* sanity check */
  if (study_get_volumes(ui_study->study) == NULL)
    return;

  if (ui_study->current_zoom != adjustment->value) {
    ui_study->current_zoom = adjustment->value;
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


void ui_study_callbacks_thickness(GtkAdjustment * adjustment, gpointer data) {

  ui_study_t * ui_study = data;

  /* sanity check */
  if (study_get_volumes(ui_study->study) == NULL)
    return;

  if (ui_study->current_thickness != adjustment->value) {
    ui_study->current_thickness = adjustment->value;
    ui_study_update_canvas(ui_study,NUM_VIEWS, UPDATE_ALL);
  }

  return;
}


/* callbacks for setting up a set of slices in a new window */
void ui_study_callbacks_transverse_series(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  ui_series_create(ui_study, TRANSVERSE);
  return;
}
void ui_study_callbacks_coronal_series(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  ui_series_create(ui_study, CORONAL);
  return;
}
void ui_study_callbacks_sagittal_series(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  ui_series_create(ui_study, SAGITTAL);
  return;
}

#ifdef AMIDE_LIBVOLPACK_SUPPORT
/* callback for starting up volume rendering */
void ui_study_callbacks_rendering(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  volume_list_t * temp_volumes;

  temp_volumes = ui_volume_list_return_volume_list(ui_study->current_volumes);
  ui_rendering_create(temp_volumes, study_get_coord_frame(ui_study->study),
		      ui_study->current_time, ui_study->current_duration);
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

  /* check if we actually changed values */
  if (ui_study->scaling != i_scaling) {
    /* and inact the changes */
    ui_study->scaling = i_scaling;
    if (study_get_volumes(ui_study->study) != NULL)
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
      volume = study_get_first_volume(ui_study->study);
    else
      volume = ui_study->current_volume;


  /* figure out which scaling menu item called me */
  i_color_table = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"color_table"));

  /* check if we actually changed values */
  if (volume->color_table != i_color_table) {
    /* and inact the changes */
    volume->color_table = i_color_table;
    if (study_get_volumes(ui_study->study) != NULL) {
      
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

  entry = 
    gnome_request_dialog(FALSE, "ROI Name:", "", 256, 
			 ui_study_callbacks_roi_entry_name,
			 &roi_name, 
			 GTK_WINDOW(ui_study->app));
  
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

  gtk_option_menu_set_history(GTK_OPTION_MENU(ui_study->add_roi_option_menu), 0);
  return;
}

/* function called when another row has been selected in the tree */
void ui_study_callback_tree_select_row(GtkCTree * ctree, GList * node, 
				       gint column, gpointer data) {
  ui_study_t * ui_study = data;
  view_t i_view;
  gpointer node_pointer;
  GnomeCanvasItem * roi_canvas_item[NUM_VIEWS];
  volume_list_t * volume_list;
  roi_list_t * roi_list;

  volume_list = study_get_volumes(ui_study->study);
  roi_list = study_get_rois(ui_study->study);

  /* figure out what this node corresponds to, if anything */
  node_pointer = gtk_ctree_node_get_row_data(ctree, GTK_CTREE_NODE(node));
  if (node_pointer != NULL) {

    while (roi_list != NULL) {

      /* if it's an roi, do the appropriate roi action */
      if (node_pointer == roi_list->roi) {

	/* add the roi to the current list, making sure to only add it once */
	if (!ui_roi_list_includes_roi(ui_study->current_rois, roi_list->roi)) {
	  /* make the canvas graphics */
	  for (i_view=0;i_view<NUM_VIEWS;i_view++)
	    roi_canvas_item[i_view] = 
	      ui_study_update_canvas_roi(ui_study,i_view,NULL, roi_list->roi);

	  /* and add this roi to the current_rois list */
	  ui_study->current_rois = 
	    ui_roi_list_add_roi(ui_study->current_rois, roi_list->roi,
				roi_canvas_item, ctree, GTK_CTREE_NODE(node));
	  
	}
      }

      roi_list = roi_list->next;
    }

    while (volume_list != NULL) {

      /* if it's a volume, do the appropriate volume action */
      if (node_pointer == volume_list->volume) {
	if (!ui_volume_list_includes_volume(ui_study->current_volumes, volume_list->volume))
	  ui_study->current_volumes =
	    ui_volume_list_add_volume(ui_study->current_volumes, volume_list->volume,
				      ctree, GTK_CTREE_NODE(node));
	ui_study_update_canvas(ui_study,NUM_VIEWS, UPDATE_ALL);
	ui_time_dialog_set_times(ui_study);
      }

      volume_list = volume_list->next;
    }

    /* check if we were selecting the study */
    if (node_pointer == ui_study->study) {
      ui_study->study_selected = TRUE;
    }

  }

  return;
}

/* function called when another row has been unselected in the tree */
void ui_study_callback_tree_unselect_row(GtkCTree * ctree, GList * node, 
					 gint column, gpointer data) {
  ui_study_t * ui_study = data;
  gpointer node_pointer;
  volume_list_t * volume_list;
  roi_list_t * roi_list;

  volume_list = study_get_volumes(ui_study->study);
  roi_list = study_get_rois(ui_study->study);

  /* figure out what this node corresponds to, if anything */
  node_pointer = gtk_ctree_node_get_row_data(ctree, GTK_CTREE_NODE(node));
  if (node_pointer != NULL) {
    while (roi_list != NULL) {

      /* if it's an roi, do the appropriate roi action */
      if (node_pointer == roi_list->roi) {
	if (ui_study->current_roi == roi_list->roi) 
	  ui_study->current_roi = NULL;
	ui_study->current_rois = ui_roi_list_remove_roi(ui_study->current_rois, roi_list->roi);
      }

      roi_list = roi_list->next;
    }
    while (volume_list != NULL) {

      /* if it's a volume, do the appropriate volume action */
      if (node_pointer == volume_list->volume) {

	ui_study->current_volumes = 
	  ui_volume_list_remove_volume(ui_study->current_volumes, volume_list->volume);
      
	/* destroy the threshold window if it's open and looking at this volume */
	if (ui_study->threshold != NULL)
	  if (ui_study->threshold->volume==volume_list->volume)
	    gtk_signal_emit_by_name(GTK_OBJECT(ui_study->threshold->app), "delete_event", 
				    NULL, ui_study);

	/* we'll need to redraw the canvas */
	ui_study_update_canvas(ui_study,NUM_VIEWS,UPDATE_ALL);
	ui_time_dialog_set_times(ui_study);
      }

      volume_list = volume_list->next;
    }

    /* check if we were unselecting the study */
    if (node_pointer == ui_study->study) {
      if (ui_study->study_dialog != NULL)
	gtk_signal_emit_by_name(GTK_OBJECT(ui_study->study_dialog), 
			      "delete_event", NULL, ui_study);
      ui_study->study_selected = FALSE;
    }

  }

  return;
}

/* function called when a row in the tree is clicked on */
gboolean ui_study_callback_tree_click_row(GtkWidget *widget,
					  GdkEventButton *event,
					  gpointer data) {
  ui_study_t * ui_study = data;
  gpointer node_pointer = NULL;
  volume_list_t * volume_list; 
  roi_list_t * roi_list;
  realpoint_t center;
  GtkCTree * ctree = GTK_CTREE(widget);
  GtkCTreeNode * node = NULL;
  gint row = -1;
  gint col;

  volume_list = study_get_volumes(ui_study->study);
  roi_list = study_get_rois(ui_study->study);

  /* we're really only interest in double clicks with button 1, or clicks
     with buttons 2 or 3 */
  if (((event->type == GDK_2BUTTON_PRESS) && (event->button == 1)) 
      || (event->button == 3) || (event->button ==2)) {

    /* do some fancy stuff to figure out what node in the tree just got selected*/
    gtk_clist_get_selection_info (GTK_CLIST(ctree), event->x, event->y, 
				  &row, &col);
    if (row != -1) 
      node = gtk_ctree_node_nth(ctree, (guint)row);
                    

    /* figure out what this node corresponds to, if anything, and take
       appropriate corresponding action */
    if (node != NULL)
      node_pointer = gtk_ctree_node_get_row_data(ctree, node);
    if (node_pointer != NULL) {
      while (roi_list != NULL) {

	/* if it's an roi, do the appropriate roi action */
	if (node_pointer == roi_list->roi) {
	  if ((event->button == 1) || (event->button == 2)) {
	    ui_study->current_mode = ROI_MODE;
	    ui_study->current_roi = node_pointer;
	    
	    /* center the view on this roi, check first if the roi has been drawn */
	    if ( !roi_undrawn(ui_study->current_roi)) {
	      center = roi_calculate_center(ui_study->current_roi);
	      center = realspace_base_coord_to_alt(center, study_get_coord_frame(ui_study->study));
	      ui_study->current_view_center.x = center.x -
		ui_study->current_thickness/2.0;
	      ui_study->current_view_center.y = center.y -
		ui_study->current_thickness/2.0;
	      ui_study->current_view_center.z = center.z -
		ui_study->current_thickness/2.0;
	      ui_study_update_canvas(ui_study,NUM_VIEWS, UPDATE_ALL);
	    } else {
	      /* if this roi hasn't yet been drawn, at least update 
		 which roi is highlighted */
	      ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_ROIS);
	    }
	  } else { /* button 3 clicked */
	    /* start up the dialog to adjust the roi */
	    ui_roi_dialog_create(ui_study, node_pointer);
	  }
	}


	roi_list = roi_list->next;
      }


      while (volume_list != NULL) {

	/* if it's a volume, display that volume */
	if (node_pointer == volume_list->volume) {

	  if ((event->button == 1) || (event->button == 2)) {
	    ui_study->current_mode = VOLUME_MODE;
	    ui_study->current_volume = node_pointer;
	    /* reset the color_table picker based on the current volume */
	    gtk_option_menu_set_history(GTK_OPTION_MENU(ui_study->color_table_menu),
					ui_study->current_volume->color_table);
	    /* reset the threshold widget based on the current volume */
	    if (ui_study->threshold != NULL)
	      ui_threshold_dialog_update(ui_study);

	  } else { /* event.button == 3 */
	    /* start up the volume dialog */
	    ui_volume_dialog_create(ui_study, node_pointer);

	  }
	}

	volume_list = volume_list->next;
      }
      
      /* check if we were clicking on the study */
      if (node_pointer == ui_study->study) {
	if ((event->button == 1 ) || (event->button == 2))
	  ; /* don't do anything for this */
	else  /*event.button == 3 */
	  ui_study_dialog_create(ui_study);
      }

    }
  }


  return TRUE;
}


/* function called to edit selected objects in the study */
void ui_study_callbacks_edit_object_pressed(GtkWidget * button, gpointer data) {

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

  /* pop up the study parameter dialog if selected */
  if (ui_study->study_selected)
    ui_study_dialog_create(ui_study);

  return;
}



/* function called to delete selected objects from the tree and the study */
void ui_study_callbacks_delete_object_pressed(GtkWidget * button, gpointer data) {

  ui_study_t * ui_study = data;
  volume_t * current_volume;
  volume_t * volume;
  roi_t * roi;
  GtkCTreeNode * node;

  /* delete the selected volumes */
  while (ui_study->current_volumes != NULL) {
    volume = volume_add_reference(ui_study->current_volumes->volume);
    
    /*figure out what node in the tree corresponds to this and remove it */
    node = gtk_ctree_find_by_row_data(GTK_CTREE(ui_study->tree),
				      ui_study->tree_volumes,
				      volume);
    if (node == ui_study->tree_volumes)
      ui_study->tree_volumes = GTK_CTREE_NODE_NEXT(node);
    gtk_ctree_remove_node(GTK_CTREE(ui_study->tree), node);
    
    /* destroy the threshold window if it's open and looking at this volume */
    /* figure out what's the current active volume */
    if (ui_study->current_volume == NULL)
      current_volume = study_get_first_volume(ui_study->study);
    else
      current_volume = ui_study->current_volume;
    if (current_volume==volume)
      if (ui_study->threshold != NULL)
	gtk_signal_emit_by_name(GTK_OBJECT(ui_study->threshold->app), "delete_event", NULL, ui_study);
     
    /* remove the volume from existence */
    if (ui_study->current_volume == volume)
      ui_study->current_volume = volume_free(ui_study->current_volume);
    study_remove_volume(ui_study->study, volume);
    ui_study->current_volumes = ui_volume_list_remove_volume(ui_study->current_volumes, volume);
    volume = volume_free(volume);
  }

  /* delete the selected roi's */
  while (ui_study->current_rois != NULL) {
    roi = roi_add_reference(ui_study->current_rois->roi);

    /*figure out what node in the tree corresponds to this and remove it */
    node = gtk_ctree_find_by_row_data(GTK_CTREE(ui_study->tree),
				      ui_study->tree_rois,
				      roi);
    if (node == ui_study->tree_rois)
      ui_study->tree_rois = GTK_CTREE_NODE_NEXT(node);
    gtk_ctree_remove_node(GTK_CTREE(ui_study->tree), node);

    /* remove the roi's data structure */
    if (ui_study->current_roi == roi)
      ui_study->current_roi = roi_free(ui_study->current_roi);
    study_remove_roi(ui_study->study, roi);
    ui_study->current_rois = ui_roi_list_remove_roi(ui_study->current_rois, roi);
    roi = roi_free(roi);
  }

  return;
}


/* function to switch the interpolation method */
void ui_study_callbacks_interpolation(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  interpolation_t i_interpolation;

  /* figure out which scaling menu item called me */
  i_interpolation = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"interpolation"));

  /* check if we actually changed values */
  if (ui_study->current_interpolation != i_interpolation) {
    /* and inact the changes */
    ui_study->current_interpolation = i_interpolation;
	if (study_get_volumes(ui_study->study) != NULL) {
	  ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_IMAGE);
	  //	  if (ui_study->threshold != NULL)
	  //	    ui_threshold_update_canvas(ui_study, ui_study->threshold);
	}
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


























