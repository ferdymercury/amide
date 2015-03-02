/* ui_study_callbacks.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000 Andy Loening
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
#include "amide.h"
#include "realspace.h"
#include "color_table.h"
#include "volume.h"
#include "roi.h"
#include "study.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_study_rois.h"
#include "ui_study_volumes.h"
#include "ui_study.h"
#include "ui_study_callbacks.h"
#include "ui_threshold2.h"
#include "ui_series2.h"
#include "ui_study_rois2.h"
#include "cti_import.h"
#include "ui_roi_dialog.h"
#include "ui_time_dialog.h"
#include "ui_volume_dialog.h"
#include "raw_data_import.h"


/* function to close the file import widget */
static void ui_study_callbacks_import_cancel(GtkWidget* widget, gpointer data) {

  GtkFileSelection * file_selection = data;

  gtk_grab_remove(GTK_WIDGET(file_selection));
  gtk_widget_destroy(GTK_WIDGET(file_selection));

  return;
}

/* function to start the file importation process */
static void ui_study_callbacks_import_ok(GtkWidget* widget, gpointer data) {

  ui_study_t * ui_study = data;
  gchar * import_filename=NULL;
  gchar * import_filename_base=NULL;
  gchar * import_filename_extension=NULL;
  gchar ** frags=NULL;
  amide_volume_t * import_volume=NULL;

  /* get the filename */
  import_filename = gtk_file_selection_get_filename(ui_study->file_selection);

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



    
    if ((g_strcasecmp(import_filename_extension, "dat")==0) |
	(g_strcasecmp(import_filename_extension, "raw")==0))  
      { /* .dat and .raw are assumed to be raw data */
      
	/* call the corresponding import function */
	import_volume=raw_data_import(import_filename);

      }
#ifdef AMIDE_CTI_SUPPORT      
    else if ((g_strcasecmp(import_filename_extension, "img")==0) |
	     (g_strcasecmp(import_filename_extension, "v")==0))
      { /* if it appears to be a cti file, try importing it */
      
      
	/* and call that import function */
	if ((import_volume=cti_import(import_filename)) == NULL) {
	  g_warning("%s: Could not open file: %s as a CTI File", 
		    PACKAGE, import_filename);
	} 
      }
#endif
     
    else 
      { /* unrecognized file type */
	g_warning("%s: file: %s does not appear to be a supported data file\n", 
		  PACKAGE, import_filename);
      }
    
  } else { /* no filename */
    ; 
  }


  if (import_volume != NULL) {

    /* add the volume to the study structure */
    volume_list_add_volume(&(ui_study->study->volumes), import_volume);
    
    /* and add the volume to the ui_study tree structure */
    ui_study_tree_add_volume(ui_study, import_volume);
    
    /* set the thresholds */
    import_volume->threshold_max = import_volume->max;
    import_volume->threshold_min = (import_volume->min >=0) ? import_volume->min : 0;
    
    /* set the current thickness if it hasn't already been done */
    if (ui_study->current_thickness < 0.0)
      ui_study->current_thickness = REALPOINT_MIN_DIM(import_volume->voxel_size);
    
    /* update the thickness widget */
    ui_study_update_thickness_adjustment(ui_study);
    
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
  ui_study_callbacks_import_cancel(widget, ui_study->file_selection);

  return;
}


/* function to selection which file to import */
void ui_study_callbacks_import(GtkWidget * widget, gpointer data) {
  
  ui_study_t * ui_study = data;

  GtkFileSelection * file_selection;

  file_selection = GTK_FILE_SELECTION(gtk_file_selection_new(_("Import File")));
  ui_study->file_selection=file_selection; /* save for call back use */

  /* don't want anything else going on till this window is gone */
  gtk_window_set_modal(GTK_WINDOW(file_selection), TRUE);

  /* connect the signals */
  gtk_signal_connect(GTK_OBJECT(file_selection->ok_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(ui_study_callbacks_import_ok),
		     ui_study);
  gtk_signal_connect(GTK_OBJECT(file_selection->cancel_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(ui_study_callbacks_import_cancel),
		     file_selection);
  gtk_signal_connect(GTK_OBJECT(file_selection->cancel_button),
		     "delete_event",
		     GTK_SIGNAL_FUNC(ui_study_callbacks_import_cancel),
		     file_selection);

  /* set the position of the dialog */
  gtk_window_set_position(GTK_WINDOW(file_selection), GTK_WIN_POS_MOUSE);

  /* run the dialog */
  gtk_widget_show(GTK_WIDGET(file_selection));
  gtk_grab_add(GTK_WIDGET(file_selection));

  return;
}


/* function called when an event occurs on the image canvas,
   note, events for non-new roi's are handled by
   ui_study_rois_callbacks_roi_event */
gint ui_study_callbacks_canvas_event(GtkWidget* widget, 
				     GdkEvent * event,
				     gpointer data) {

  ui_study_t * ui_study = data;
  realpoint_t real_loc, view_loc;
  view_t i, j;
  axis_t k;
  realpoint_t item, diff;
  amide_volume_t * volume;
  guint32 outline_color;
  GdkCursor * cursor;
  realspace_t view_coord_frame;
  realpoint_t far_corner;
  static realpoint_t picture_center,picture0,picture1;
  static realpoint_t center, p0,p1;
  static GnomeCanvasItem * new_roi = NULL;
  static gboolean dragging = FALSE;
  
  /* sanity checks */
  if (ui_study->study->volumes == NULL)
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
  for (i=0; i<NUM_VIEWS; i++) 
    if (((gpointer) (ui_study->canvas_image[i])) == ((gpointer) widget))
      break;

  /* get the coordinate frame for this view and the far_corner
     the far_corner is in the view_coord_frame */
  view_coord_frame = ui_study_get_coords_current_view(ui_study, i, &far_corner);

  /* convert the event location to view space units */
  view_loc.x = ((item.x-UI_STUDY_TRIANGLE_HEIGHT)
		/ui_study->rgb_image[i]->rgb_width)*far_corner.x;
  view_loc.y = ((item.y-UI_STUDY_TRIANGLE_HEIGHT)
		/ui_study->rgb_image[i]->rgb_height)*far_corner.y;
  view_loc.z = ui_study->current_thickness/2.0;

  /* Convert the event location info to real units */
  real_loc = realspace_alt_coord_to_base(view_loc, view_coord_frame);

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
      gdk_window_set_cursor(gtk_widget_get_parent_window(GTK_WIDGET(ui_study->canvas[i])), cursor);

      break;


     case GDK_LEAVE_NOTIFY:
       /* pop the previous cursor off the stack */
       cursor = g_slist_nth_data(ui_study->cursor_stack, 0);
       ui_study->cursor_stack = g_slist_remove(ui_study->cursor_stack, cursor);
       cursor = g_slist_nth_data(ui_study->cursor_stack, 0);
       gdk_window_set_cursor(gtk_widget_get_parent_window(GTK_WIDGET(ui_study->canvas[i])), cursor);
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
	      
	    case GROUP:
	      ;
	      break;
	    case BOX:
	      new_roi = 
		gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[i]),
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
		gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[i]),
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

	/* some sanity checks */
	if ((real_loc.x < 0.0) | (real_loc.y < 0.0) | (real_loc.z < 0.0) |
	    (real_loc.x > far_corner.x) | (real_loc.y > far_corner.y))
	  // |(real_loc.z > far_corner.z))
	  return TRUE;

	switch (i)
	  {
	  case TRANSVERSE:
	    ui_study->current_axis_p_start.x = view_loc.x;
	    ui_study->current_axis_p_start.y = view_loc.y;
	    ui_study_update_canvas(ui_study, CORONAL, UPDATE_ALL);
	    ui_study_update_canvas(ui_study, SAGITTAL, UPDATE_ALL);
	    ui_study_update_canvas(ui_study, TRANSVERSE, UPDATE_PLANE_ADJUSTMENT);
	    ui_study_update_canvas(ui_study, TRANSVERSE, UPDATE_ARROWS);
	    break;
	  case CORONAL:
	    ui_study->current_axis_p_start.x = view_loc.x;
	    ui_study->current_axis_p_start.z = view_loc.y;
	    ui_study_update_canvas(ui_study, TRANSVERSE, UPDATE_ALL);
	    ui_study_update_canvas(ui_study, SAGITTAL, UPDATE_ALL);
	    ui_study_update_canvas(ui_study, CORONAL, UPDATE_PLANE_ADJUSTMENT);
	    ui_study_update_canvas(ui_study, CORONAL, UPDATE_ARROWS);
	    break;
	  case SAGITTAL:
	    ui_study->current_axis_p_start.y = view_loc.x;
	    ui_study->current_axis_p_start.z = view_loc.y;
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
	    REALSPACE_DIFF(picture_center, item, diff);
	    REALSPACE_SUB(picture_center, diff, picture0);
	    REALSPACE_ADD(picture_center, diff, picture1);
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
	  gdk_window_set_cursor(gtk_widget_get_parent_window(GTK_WIDGET(ui_study->canvas[i])), cursor);
	  dragging = FALSE;
	  
	  REALSPACE_DIFF(center, real_loc, diff);
	  
	  switch (i)
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
	  
	  REALSPACE_SUB(center, diff, p0);
	  REALSPACE_ADD(center, diff, p1);
	  
	  /* let's save the info */
	  gtk_object_destroy(GTK_OBJECT(new_roi));
	  
	  /* we'll save the coord frame and offset of the roi */
	  ui_study->current_roi->coord_frame.offset = p0;
	  for (k=0;k<NUM_AXIS;k++)
	    ui_study->current_roi->coord_frame.axis[k] = view_coord_frame.axis[k];

	  /* and set the far corner of the roi */
	  ui_study->current_roi->corner = 
	    realspace_base_coord_to_alt(p1,ui_study->current_roi->coord_frame);
	  
	  /* and save any roi type specfic info */
	  switch(ui_study->current_roi->type) {
	  case GROUP:
	  case CYLINDER:
	  case BOX:
	  case ELLIPSOID:
	  default:
	    break;
	  }
	  
	  /* update the roi's */
	  for (j=0;j<NUM_VIEWS;j++) {
	    ui_study_rois_update_canvas_rois(ui_study,j);
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
  view_t i;

  /* sanity check */
  if (ui_study->study->volumes == NULL)
    return;
  if (ui_study->current_volumes == NULL)
    return;

  /* figure out which scale widget called me */
  for (i=0;i<NUM_VIEWS;i++) 
    if (gtk_object_get_data(GTK_OBJECT(adjustment),"view") == view_names[i]) {
      switch(i) {
      case TRANSVERSE:
	ui_study->current_axis_p_start.z = adjustment->value;
	ui_study_update_canvas(ui_study,TRANSVERSE,UPDATE_ALL);
	ui_study_update_canvas(ui_study,CORONAL,UPDATE_ARROWS);
	ui_study_update_canvas(ui_study,SAGITTAL,UPDATE_ARROWS);
	break;
      case CORONAL:
	ui_study->current_axis_p_start.y = adjustment->value;
	ui_study_update_canvas(ui_study,CORONAL,UPDATE_ALL);
	ui_study_update_canvas(ui_study,TRANSVERSE,UPDATE_ARROWS);
	ui_study_update_canvas(ui_study,SAGITTAL,UPDATE_ARROWS);
	break;
      case SAGITTAL:
	ui_study->current_axis_p_start.x = adjustment->value;
	ui_study_update_canvas(ui_study,SAGITTAL,UPDATE_ALL);
	ui_study_update_canvas(ui_study,TRANSVERSE,UPDATE_ARROWS);
	ui_study_update_canvas(ui_study,CORONAL,UPDATE_ARROWS);
	break;
      default:
	break;
      }
    }

  return;
}

/* function called when rotating the view coordinates around an axis */
void ui_study_callbacks_axis_change(GtkAdjustment * adjustment, gpointer data) {

  ui_study_t * ui_study = data;
  view_t i_view;
  floatpoint_t rotation;
  floatpoint_t width,height,length;
  floatpoint_t old_width,old_height,old_length;

  /* sanity check */
  if (ui_study->study->volumes == NULL)
    return;
  if (ui_study->current_volumes == NULL)
    return;

  /* get some info we'll need */
  old_width = ui_study_volumes_get_width(ui_study->current_volumes, ui_study->current_coord_frame);
  old_height = ui_study_volumes_get_height(ui_study->current_volumes, ui_study->current_coord_frame);
  old_length = ui_study_volumes_get_length(ui_study->current_volumes, ui_study->current_coord_frame);

  /* figure out which scale widget called me */
  for (i_view=0;i_view<NUM_VIEWS;i_view++) 
    if (gtk_object_get_data(GTK_OBJECT(adjustment),"view") == view_names[i_view]) {
      rotation = (adjustment->value/180)*M_PI; /* get rotation in radians */
      switch(i_view) {
      case TRANSVERSE:
	ui_study->current_coord_frame.axis[XAXIS] = 
	  realspace_rotate_on_axis(&ui_study->current_coord_frame.axis[XAXIS],
				   &ui_study->current_coord_frame.axis[ZAXIS],
				   rotation);
	ui_study->current_coord_frame.axis[YAXIS] = 
	  realspace_rotate_on_axis(&ui_study->current_coord_frame.axis[YAXIS],
				   &ui_study->current_coord_frame.axis[ZAXIS],
				   rotation);
	realspace_make_orthonormal(ui_study->current_coord_frame.axis); /* orthonormalize*/
	/* compensate the current viewing information for the change in size */
	width = ui_study_volumes_get_width(ui_study->current_volumes, ui_study->current_coord_frame);
	height = ui_study_volumes_get_height(ui_study->current_volumes, ui_study->current_coord_frame);
	ui_study->current_axis_p_start.x = ui_study->current_axis_p_start.x +
	  (width-old_width)/2.0;
	ui_study->current_axis_p_start.y = ui_study->current_axis_p_start.y +
	  (height-old_height)/2.0;
	ui_study_update_canvas(ui_study,NUM_VIEWS,UPDATE_ALL);
	break;
      case CORONAL:
	ui_study->current_coord_frame.axis[XAXIS] = 
	  realspace_rotate_on_axis(&ui_study->current_coord_frame.axis[XAXIS],
				   &ui_study->current_coord_frame.axis[YAXIS],
				   rotation);
	ui_study->current_coord_frame.axis[ZAXIS] = 
	  realspace_rotate_on_axis(&ui_study->current_coord_frame.axis[ZAXIS],
				   &ui_study->current_coord_frame.axis[YAXIS],
				   rotation);
	realspace_make_orthonormal(ui_study->current_coord_frame.axis); /* orthonormalize*/
	/* compensate the current viewing information for the change in size */
	width = ui_study_volumes_get_width(ui_study->current_volumes, ui_study->current_coord_frame);
	length =ui_study_volumes_get_length(ui_study->current_volumes, ui_study->current_coord_frame);
	ui_study->current_axis_p_start.x = ui_study->current_axis_p_start.x +
	  (width-old_width)/2.0;
	ui_study->current_axis_p_start.z = ui_study->current_axis_p_start.z +
	  (length-old_length)/2.0;
	ui_study_update_canvas(ui_study,NUM_VIEWS,UPDATE_ALL);
	break;
      case SAGITTAL:
	ui_study->current_coord_frame.axis[YAXIS] = 
	  realspace_rotate_on_axis(&ui_study->current_coord_frame.axis[YAXIS],
				   &ui_study->current_coord_frame.axis[XAXIS],
				   rotation);
	ui_study->current_coord_frame.axis[ZAXIS] = 
	  realspace_rotate_on_axis(&ui_study->current_coord_frame.axis[ZAXIS],
				   &ui_study->current_coord_frame.axis[XAXIS],
				   rotation);
	realspace_make_orthonormal(ui_study->current_coord_frame.axis); /* orthonormalize*/
	/* compensate the current viewing information for the change in size */
	length = ui_study_volumes_get_length(ui_study->current_volumes, ui_study->current_coord_frame);
	height = ui_study_volumes_get_height(ui_study->current_volumes, ui_study->current_coord_frame);
	ui_study->current_axis_p_start.y = ui_study->current_axis_p_start.y +
	  (height-old_height)/2.0;
	ui_study->current_axis_p_start.z = ui_study->current_axis_p_start.z +
	  (length-old_length)/2.0;
	ui_study_update_canvas(ui_study,NUM_VIEWS,UPDATE_ALL);
	break;
      default:
	break;
      }
    }

  /* return adjustment back to normal */
  adjustment->value = 0.0;
  gtk_adjustment_changed(adjustment);

  return;
}


/* function called to reset the axis */
void ui_study_callbacks_reset_axis(GtkWidget * button, gpointer data) {

  ui_study_t * ui_study = data;
  axis_t i;
  floatpoint_t width,height,length;
  floatpoint_t old_width,old_height,old_length;

  /* sanity check */
  if (ui_study->study->volumes == NULL)
    return;
  if (ui_study->current_volumes == NULL)
    return;

  /* get some info we'll need */
  old_width = ui_study_volumes_get_width(ui_study->current_volumes, ui_study->current_coord_frame);
  old_height = ui_study_volumes_get_height(ui_study->current_volumes, ui_study->current_coord_frame);
  old_length = ui_study_volumes_get_length(ui_study->current_volumes, ui_study->current_coord_frame);

  /* reset the axis */
  for (i=0;i<NUM_AXIS;i++) {
    ui_study->current_coord_frame.axis[i] = default_axis[i];
  }
  ui_study->current_coord_frame.offset = realpoint_init;

  /* compensate the current viewing information for the change in size */
  width = ui_study_volumes_get_width(ui_study->current_volumes, ui_study->current_coord_frame);
  height = ui_study_volumes_get_height(ui_study->current_volumes, ui_study->current_coord_frame);
  length = ui_study_volumes_get_length(ui_study->current_volumes, ui_study->current_coord_frame);
  ui_study->current_axis_p_start.x = ui_study->current_axis_p_start.x +
    (width-old_width)/2.0;
  ui_study->current_axis_p_start.y = ui_study->current_axis_p_start.y +
    (height-old_height)/2.0;
  ui_study->current_axis_p_start.z = ui_study->current_axis_p_start.z +
    (length-old_length)/2.0;

  ui_study_update_canvas(ui_study,NUM_VIEWS,UPDATE_ALL);

  return;
}


void ui_study_callbacks_zoom(GtkAdjustment * adjustment, gpointer data) {

  ui_study_t * ui_study = data;

  /* sanity check */
  if (ui_study->study->volumes == NULL)
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
  if (ui_study->study->volumes == NULL)
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

/* function called when hitting the threshold button, pops up a dialog */
void ui_study_callbacks_threshold_pressed(GtkWidget * button, gpointer data) {
  ui_study_t * ui_study = data;
  ui_threshold_create(ui_study);
  return;
}

/* function to switch to normalizing per slice */
void ui_study_callbacks_scaling(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  scaling_t i_scaling;

  /* figure out which scaling menu item called me */
  for (i_scaling=0;i_scaling<NUM_SCALINGS;i_scaling++) 
    if (gtk_object_get_data(GTK_OBJECT(widget),"scaling") == scaling_names[i_scaling])
      /* check if we actually changed values */
      if (ui_study->scaling != i_scaling) {
	/* and inact the changes */
	ui_study->scaling = i_scaling;
	if (ui_study->study->volumes != NULL)
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
  amide_volume_t * volume;

  /* figure out which volume we're dealing with */
  if (ui_study->current_volume == NULL)
    volume = ui_study->study->volumes->volume;
  else
    volume = ui_study->current_volume;

  /* figure out which scaling menu item called me */
  for (i_color_table=0;i_color_table<NUM_COLOR_TABLES;i_color_table++) 
    if (gtk_object_get_data(GTK_OBJECT(widget),"color_table") == color_table_names[i_color_table])
      /* check if we actually changed values */
      if (volume->color_table != i_color_table) {
	/* and inact the changes */
	volume->color_table = i_color_table;
	if (ui_study->study->volumes != NULL) {
	  ui_study_update_canvas(ui_study, NUM_VIEWS, REFRESH_IMAGE);
	  ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_ROIS);
	  if (ui_study->threshold != NULL)
	    ui_threshold_update_canvas(ui_study);
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
  amide_roi_t * roi;

  /* figure out which menu item called me */
  for (i_roi_type=0;i_roi_type<NUM_ROI_TYPES;i_roi_type++)       
    if (gtk_object_get_data(GTK_OBJECT(widget),"roi_type") == roi_type_names[i_roi_type]) {
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
	roi_list_add_roi(&(ui_study->study->rois), roi);
	ui_study_tree_add_roi(ui_study, roi);
      }
    }


  gtk_option_menu_set_history(GTK_OPTION_MENU(ui_study->add_roi_option_menu), 0);
  return;
}

/* function called when another row has been selected in the tree */
void ui_study_callback_tree_select_row(GtkCTree * ctree, GList * node, 
				       gint column, gpointer data) {
  ui_study_t * ui_study = data;
  view_t i;
  gpointer node_pointer;
  GnomeCanvasItem * roi_canvas_item[NUM_VIEWS];
  amide_volume_list_t * volume_list = ui_study->study->volumes;
  amide_roi_list_t * roi_list = ui_study->study->rois;

  /* figure out what this node corresponds to, if anything */
  node_pointer = gtk_ctree_node_get_row_data(ctree, GTK_CTREE_NODE(node));
  if (node_pointer != NULL) {

    while (roi_list != NULL) {

      /* if it's an roi, do the appropriate roi action */
      if (node_pointer == roi_list->roi) {

	/* add the roi to the current list, making sure to only add it once */
	if (!ui_study_rois_list_includes_roi(ui_study->current_rois, 
					     roi_list->roi)) {
	  /* make the canvas graphics */
	  for (i=0;i<NUM_VIEWS;i++)
	    roi_canvas_item[i] = 
	      ui_study_rois_update_canvas_roi(ui_study,i,NULL, roi_list->roi);

	  /* and add this roi to the current_rois list */
	  ui_study_rois_list_add_roi(&(ui_study->current_rois), 
				     roi_list->roi,
				     roi_canvas_item,
				     ctree,
				     GTK_CTREE_NODE(node));
	  
	}
      }

      roi_list = roi_list->next;
    }

    while (volume_list != NULL) {

      /* if it's a volume, do the appropriate volume action */
      if (node_pointer == volume_list->volume) {
	if (!ui_study_volumes_list_includes_volume(ui_study->current_volumes, 
						  volume_list->volume))
	  ui_study_volumes_list_add_volume(&(ui_study->current_volumes),
					  volume_list->volume,
					  ctree,
					  GTK_CTREE_NODE(node));
	ui_study_update_canvas(ui_study,NUM_VIEWS, UPDATE_ALL);
	ui_time_dialog_set_times(ui_study);
      }

      volume_list = volume_list->next;
    }
  }

  return;
}

/* function called when another row has been unselected in the tree */
void ui_study_callback_tree_unselect_row(GtkCTree * ctree, GList * node, 
					 gint column, gpointer data) {
  ui_study_t * ui_study = data;
  gpointer node_pointer;
  amide_volume_list_t * volume_list = ui_study->study->volumes;
  amide_roi_list_t * roi_list = ui_study->study->rois;


  /* figure out what this node corresponds to, if anything */
  node_pointer = gtk_ctree_node_get_row_data(ctree, GTK_CTREE_NODE(node));
  if (node_pointer != NULL) {
    while (roi_list != NULL) {

      /* if it's an roi, do the appropriate roi action */
      if (node_pointer == roi_list->roi) {
	if (ui_study->current_roi == roi_list->roi) 
	  ui_study->current_roi = NULL;
	ui_study_rois_list_remove_roi(&(ui_study->current_rois), roi_list->roi);
      }

      roi_list = roi_list->next;
    }
    while (volume_list != NULL) {

      /* if it's a volume, do the appropriate volume action */
      if (node_pointer == volume_list->volume) {

	ui_study_volumes_list_remove_volume(&(ui_study->current_volumes),
					   volume_list->volume);
      
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

  }

  return;
}

/* function called when a ropw in the tree is clicked on */
gboolean ui_study_callback_tree_click_row(GtkWidget *widget,
					  GdkEventButton *event,
					  gpointer data) {
  ui_study_t * ui_study = data;
  gpointer node_pointer = NULL;
  amide_volume_list_t * volume_list = ui_study->study->volumes;
  amide_roi_list_t * roi_list = ui_study->study->rois;
  realpoint_t center;
  GtkCTree * ctree = GTK_CTREE(widget);
  GtkCTreeNode * node = NULL;
  gint row = -1;
  gint col;
  realpoint_t p0,p1;
 
  /* we're really only interest in double clicks with button 1, or clicks
     with buttons 2 or 3 */
  if (((event->type == GDK_2BUTTON_PRESS) & (event->button == 1)) 
      | (event->button == 3) | (event->button ==2)) {

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
	  if ((event->button == 1) | (event->button == 2)) {
	    ui_study->current_mode = ROI_MODE;
	    ui_study->current_roi = node_pointer;
	    
	    /* center the view on this roi, check first if the roi has been drawn */
	    if ( !roi_undrawn(ui_study->current_roi)) {
	      p0 = ui_study->current_roi->coord_frame.offset;
	      p1 = realspace_alt_coord_to_base(ui_study->current_roi->corner,
					       ui_study->current_roi->coord_frame);
	      REALSPACE_MADD(0.5,p0,0.5,p1,center);
	      ui_study->current_axis_p_start.x = center.x -
		ui_study->current_thickness/2.0;
	      ui_study->current_axis_p_start.y = center.y -
		ui_study->current_thickness/2.0;
	      ui_study->current_axis_p_start.z = center.z -
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

	  if ((event->button == 1) | (event->button == 2)) {
	    ui_study->current_mode = VOLUME_MODE;
	    ui_study->current_volume = node_pointer;
	    /* reset the color_table picker based on the current volume */
	    gtk_option_menu_set_history(GTK_OPTION_MENU(ui_study->color_table_menu),
					ui_study->current_volume->color_table);
	    /* reset the threshold widget based on the current volume */
	    if (ui_study->threshold != NULL)
	      ui_threshold_update(ui_study);

	  } else { /* event.button == 3 */
	    /* start up the volume dialog */
	    ui_volume_dialog_create(ui_study, node_pointer);

	  }
	}

	volume_list = volume_list->next;
      }
      
    }
  }


  return TRUE;
}



/* function called to delete selected items from the tree and the study */
void ui_study_callbacks_delete_item_pressed(GtkWidget * button, gpointer data) {

  ui_study_t * ui_study = data;
  ui_study_volume_list_t * volume_list = ui_study->current_volumes;
  ui_study_roi_list_t * roi_list = ui_study->current_rois;
  amide_volume_t * volume;
  amide_volume_t * current_volume;
  amide_roi_t * roi;
  GtkCTreeNode * node;

  /* delete the selected volumes */
  while (volume_list != NULL) {
    volume = volume_list->volume;
    volume_list = volume_list->next;

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
      current_volume = ui_study->study->volumes->volume;
    else
      current_volume = ui_study->current_volume;
    if (current_volume==volume)
      if (ui_study->threshold != NULL)
	gtk_signal_emit_by_name(GTK_OBJECT(ui_study->threshold->app), "delete_event", NULL, ui_study);

    /* if we're currently displaying this volume in the series widget, delete the series widget */
    if (volume_list_includes_volume(ui_study->series->volumes, current_volume))
      if (ui_study->series != NULL)
	gtk_signal_emit_by_name(GTK_OBJECT(ui_study->series->app), "delete_event", NULL, ui_study);
      

    /* remove the volume from existence */
    ui_study_volumes_list_remove_volume(&(ui_study->current_volumes), volume);
    volume_list_remove_volume(&(ui_study->study->volumes), volume);
    if (ui_study->current_volume == volume)
      ui_study->current_volume = NULL;
    volume_free(&volume);
  }

  /* delete the selected roi's */
  while (roi_list != NULL) {
    roi = roi_list->roi; /* store this so we can free it */

    /*figure out what node in the tree corresponds to this and remove it */
    node = gtk_ctree_find_by_row_data(GTK_CTREE(ui_study->tree),
				      ui_study->tree_rois,
				      roi);
    if (node == ui_study->tree_rois)
      ui_study->tree_rois = GTK_CTREE_NODE_NEXT(node);
    gtk_ctree_remove_node(GTK_CTREE(ui_study->tree), node);

    /* if there is a roi dialog widget up, kill it */
    if (roi_list->dialog != NULL)
      gtk_signal_emit_by_name(GTK_OBJECT(roi_list->dialog), "delete_event", NULL, roi_list);

    /* remove the roi's data structure */
    roi_list = roi_list->next;
    ui_study_rois_list_remove_roi(&(ui_study->current_rois), roi);
    g_assert(roi_list->dialog == NULL);
    if (ui_study->current_roi == roi)
      ui_study->current_roi = NULL;
    roi_list_remove_roi(&(ui_study->study->rois), roi);
    roi_free(&roi);
  }

  return;
}


/* function to switch the interpolation method */
void ui_study_callbacks_interpolation(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  interpolation_t i_interpolation;

  /* figure out which scaling menu item called me */
  for (i_interpolation=0;i_interpolation<NUM_INTERPOLATIONS;i_interpolation++) 
    if (gtk_object_get_data(GTK_OBJECT(widget),"interpolation") == interpolation_names[i_interpolation])
      /* check if we actually changed values */
      if (ui_study->current_interpolation != i_interpolation) {
	/* and inact the changes */
	ui_study->current_interpolation = i_interpolation;
	if (ui_study->study->volumes != NULL) {
	  ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_IMAGE);
	  if (ui_study->threshold != NULL)
	    ui_threshold_update_canvas(ui_study);
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
    gtk_signal_emit_by_name(GTK_OBJECT(ui_study->threshold->app), "delete_event", NULL, ui_study);

  /* destroy the series window if it's open */
  if (ui_study->series != NULL)
    gtk_signal_emit_by_name(GTK_OBJECT(ui_study->series->app), "delete_event", NULL, ui_study);

  /* destroy the widget */
  gtk_widget_destroy(widget);

  /* free our data structure's */
  ui_study_free(&ui_study);

  return;
}

/* function ran when closing a study window */
void ui_study_callbacks_close_event(GtkWidget* widget, gpointer data) {

  ui_study_t * ui_study = data;

  /* run the delete event function */
  gtk_signal_emit_by_name(GTK_OBJECT(ui_study->app), "delete_event", NULL, ui_study);

  return;
}


























