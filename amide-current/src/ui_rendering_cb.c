/* ui_rendering_cb.c
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
#include <math.h>
#include <gnome.h>
#include "ui_common.h"
#include "ui_common_cb.h"
#include "ui_rendering.h"
#include "ui_rendering_cb.h"
#include "ui_rendering_dialog.h"
#include "ui_rendering_movie_dialog.h"


/* function called when the canvas is hit */

/* function called when an event occurs on the image canvas, 
   notes:
   -events for non-new roi's are handled by ui_study_rois_cb_roi_event 

*/
gboolean ui_rendering_cb_canvas_event(GtkWidget* widget,  GdkEvent * event, gpointer data) {

  ui_rendering_t * ui_rendering = data;
  GnomeCanvas * canvas;
  realpoint_t temp_rp;
  canvaspoint_t temp_cp1, temp_cp2;
  canvaspoint_t canvas_cp, diff_cp;
  GnomeCanvasPoints * line_points;
  guint i, j,k;
  rgba_t color;
  static floatpoint_t dim;
  static canvaspoint_t last_cp, initial_cp, center_cp;
  static gboolean dragging = FALSE;
  static GnomeCanvasItem * rotation_box[8];
  static realpoint_t box_rp[8];
  static realspace_t box_coord_frame;
  static realpoint_t theta;

  canvas = GNOME_CANVAS(widget);

  /* get the location of the event, and convert it to the canvas coordinates */
  gnome_canvas_w2c_d(canvas, event->button.x, event->button.y, &canvas_cp.x, &canvas_cp.y);
  //g_print("location %5.3f %5.3f\n",canvas_cp.x, canvas_cp.y);

  dim = MIN(gdk_pixbuf_get_width(ui_rendering->rgb_image),
	    gdk_pixbuf_get_height(ui_rendering->rgb_image));
  temp_rp.x = temp_rp.y = temp_rp.z = -dim/2.0;
  rs_set_offset(&box_coord_frame, temp_rp);
  rs_set_axis(&box_coord_frame, default_axis);

  /* switch on the event which called this */
  switch (event->type)
    {


    case GDK_ENTER_NOTIFY:
      ui_common_place_cursor(UI_CURSOR_VOLUME_MODE, GTK_WIDGET(canvas));
      break;


     case GDK_LEAVE_NOTIFY:
       ui_common_remove_cursor(GTK_WIDGET(canvas));
       break;



    case GDK_BUTTON_PRESS:
      if ((event->button.button == 1) || (event->button.button == 2)) {
	dragging = TRUE;
	initial_cp = last_cp = canvas_cp;
	theta = realpoint_zero;
	center_cp.x = center_cp.y = dim/2.0;

	/* figure out the eight vertices */
	for (i=0; i<8; i++) {
	  box_rp[i].x = (i & 0x1) ? BOX_OFFSET * dim : (1-BOX_OFFSET) * dim;
	  box_rp[i].y = (i & 0x2) ? BOX_OFFSET * dim : (1-BOX_OFFSET) * dim;
	  box_rp[i].z = (i & 0x4) ? BOX_OFFSET * dim : (1-BOX_OFFSET) * dim;
	}

	/* draw the 8 lines we use to represent out cube */
	for (i=0; i<8; i++) {
	  line_points = gnome_canvas_points_new(2);
	  if ((i < 4) && (i & 0x1)){ /* i < 4, evens */
	    j = (i & 0x2) ? i -2 : i -1;
	    k = (i & 0x2) ? i  : i +1;
	  } else if (i < 4) { /* i < 4, odds */
	    j = i;
	    k = j+1;
	  } else { /* i > 4 */
	    j = i;
	    k = j-4;
	  }
	  line_points->coords[0] = box_rp[j].x; /* x1 */
	  line_points->coords[1] = box_rp[j].y; /* y1 */
	  line_points->coords[2] = box_rp[k].x;  /* x2 */
	  line_points->coords[3] = box_rp[k].y; /* y2 */
	  color = 
	    color_table_outline_color(ui_rendering->contexts->rendering_context->volume->color_table,
				      TRUE);
	  rotation_box[i] = 
	    gnome_canvas_item_new(gnome_canvas_root(ui_rendering->canvas), gnome_canvas_line_get_type(),
				  "points", line_points, 
				  "fill_color_rgba", color_table_rgba_to_uint32(color),
				  "width_units", 1.0, NULL);
	  gnome_canvas_points_unref(line_points);
	}

	/* translate the 8 vertices back to the base frame */
	for (i=0; i<8; i++)
	  box_rp[i] = realspace_alt_coord_to_base(box_rp[i], box_coord_frame);

	if (event->button.button == 1)
	  gnome_canvas_item_grab(GNOME_CANVAS_ITEM(rotation_box[0]),
				 GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
				 ui_common_cursor[UI_CURSOR_RENDERING_ROTATE_XY],
				 event->button.time);
	else /* button 2 */
	  gnome_canvas_item_grab(GNOME_CANVAS_ITEM(rotation_box[0]),
				 GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
				 ui_common_cursor[UI_CURSOR_RENDERING_ROTATE_Z],
				 event->button.time);
      }
      break;
    
    case GDK_MOTION_NOTIFY:
      if (dragging && (event->motion.state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK))) {
	diff_cp = cp_sub(initial_cp, canvas_cp);

	if (event->motion.state & GDK_BUTTON1_MASK) {
	  theta.y = M_PI * diff_cp.x / dim;
	  theta.x = M_PI * -diff_cp.y / dim; /* compensate for y's origin being top */

	  /* rotate the axis */
	  realspace_rotate_on_axis(&box_coord_frame,
				   rs_specific_axis(box_coord_frame, XAXIS),
				   theta.x);
	  realspace_rotate_on_axis(&box_coord_frame,
				   rs_specific_axis(box_coord_frame, YAXIS),
				   theta.y);
	} else {/* button 2 */
	  temp_cp1 = cp_sub(initial_cp,center_cp);
	  temp_cp2 = cp_sub(canvas_cp,center_cp);

	  /* figure out theta.z */
	  theta.z = acos(cp_dot_product(temp_cp1, temp_cp2) /
			 (cp_mag(temp_cp1) * cp_mag(temp_cp2)));
	  
	  /* correct for the fact that acos is always positive by using the cross product */
	  if ((temp_cp1.x*temp_cp2.y-temp_cp1.y*temp_cp2.x) > 0.0)
	    theta.z = -theta.z;
	  realspace_rotate_on_axis(&box_coord_frame,
				   rs_specific_axis(box_coord_frame, ZAXIS),
				   theta.z);
	}
	/* recalculate the offset */
	temp_rp.x = temp_rp.y = temp_rp.z = -dim/2.0;
	rs_set_offset(&box_coord_frame, realpoint_zero);
	rs_set_offset(&box_coord_frame, 
		      realspace_alt_coord_to_base(temp_rp, box_coord_frame));

	/* translate the 8 vertices */
	for (i=0; i<8; i++)
	  box_rp[i] = realspace_base_coord_to_alt(box_rp[i], box_coord_frame);
	
	/* draw the 8 lines we use to represent out cube */
	for (i=0; i<8; i++) {
	  line_points = gnome_canvas_points_new(2);
	  if ((i < 4) && (i & 0x1)){ /* i < 4, evens */
	    j = (i & 0x2) ? i -2 : i -1;
	    k = (i & 0x2) ? i  : i +1;
	  } else if (i < 4) { /* i < 4, odds */
	    j = i;
	    k = j+1;
	  } else { /* i > 4 */
	    j = i;
	    k = j-4;
	  }
	  line_points->coords[0] = box_rp[j].x; /* x1 */
	  line_points->coords[1] = box_rp[j].y; /* y1 */
	  line_points->coords[2] = box_rp[k].x;  /* x2 */
	  line_points->coords[3] = box_rp[k].y; /* y2 */
	  gnome_canvas_item_set(rotation_box[i],  "points", line_points, NULL);
	  gnome_canvas_points_unref(line_points);
	}
	
	/* translate the 8 vertices back to the base frame */
	for (i=0; i<8; i++)
	  box_rp[i] = realspace_alt_coord_to_base(box_rp[i], box_coord_frame);


	last_cp = canvas_cp;
      }	

      break;

    case GDK_BUTTON_RELEASE:
      if (dragging) {
	gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(rotation_box[0]), event->button.time);
	dragging = FALSE;

	/* get rid of the frame */
	for (i=0; i<8; i++)
	  gtk_object_destroy(GTK_OBJECT(rotation_box[i]));

	/* update the rotation values */
	rendering_list_set_rotation(ui_rendering->contexts, XAXIS, theta.x);
	rendering_list_set_rotation(ui_rendering->contexts, YAXIS, -theta.y); 
	rendering_list_set_rotation(ui_rendering->contexts, ZAXIS, -theta.z); 

	/* render now if appropriate*/
	if (ui_rendering->immediate) {
	  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_rendering->canvas));
	  ui_rendering_update_canvases(ui_rendering); 
	  ui_common_remove_cursor(GTK_WIDGET(ui_rendering->canvas));
	}

      }
      break;
    default:
      break;
    }
  
  return FALSE;
}

/* function callback for the "render" button */
void ui_rendering_cb_render(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;

  /* render now */
  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_rendering->canvas));
  ui_rendering_update_canvases(ui_rendering); 
  ui_common_remove_cursor(GTK_WIDGET(ui_rendering->canvas));

  return;
}


/* function to switch into immediate rendering mode */
void ui_rendering_cb_immediate(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;

  /* should we be rendering immediately */
  ui_rendering->immediate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  /* set the sensitivity of the render button */
  gtk_widget_set_sensitive(GTK_WIDGET(ui_rendering->render_button), !(ui_rendering->immediate));

  /* render now if appropriate*/
  if (ui_rendering->immediate) {
    ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_rendering->canvas));
    ui_rendering_update_canvases(ui_rendering); 
    ui_common_remove_cursor(GTK_WIDGET(ui_rendering->canvas));
  }

  return;

}






/* function called when one of the rotate widgets has been hit */
void ui_rendering_cb_rotate(GtkAdjustment * adjustment, gpointer data) {

  ui_rendering_t * ui_rendering = data;
  axis_t i_axis;
  floatpoint_t rotation;

  /* figure out which rotate widget called me */
  i_axis = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(adjustment),"axis"));
  rotation = (adjustment->value/180)*M_PI; /* get rotation in radians */

  /* update the rotation values */
  rendering_list_set_rotation(ui_rendering->contexts, i_axis, rotation);

  /* render now if appropriate*/
  if (ui_rendering->immediate) {
    ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_rendering->canvas));
    ui_rendering_update_canvases(ui_rendering); 
    ui_common_remove_cursor(GTK_WIDGET(ui_rendering->canvas));
  }

  /* return adjustment back to normal */
  adjustment->value = 0.0;
  gtk_adjustment_changed(adjustment);

  return;
}

/* function called to snap the axis back to the default */
void ui_rendering_cb_reset_axis_pressed(GtkWidget * widget, gpointer data) {
  ui_rendering_t * ui_rendering = data;

  /* reset the rotations */
  rendering_list_reset_rotation(ui_rendering->contexts);

  /* render now if appropriate*/
  if (ui_rendering->immediate) {
    ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_rendering->canvas));
    ui_rendering_update_canvases(ui_rendering); 
    ui_common_remove_cursor(GTK_WIDGET(ui_rendering->canvas));
  }

  return;
}


/* function to handle exporting a view */
static void ui_rendering_cb_export_ok(GtkWidget* widget, gpointer data) {

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
  export_image = gdk_imlib_create_image_from_data(gdk_pixbuf_get_pixels(ui_rendering->rgb_image), NULL, 
						  gdk_pixbuf_get_width(ui_rendering->rgb_image),
						  gdk_pixbuf_get_height(ui_rendering->rgb_image));
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

/* function to save a rendering as an external data format */
void ui_rendering_cb_export(GtkWidget * widget, gpointer data) {

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
		     GTK_SIGNAL_FUNC(ui_rendering_cb_export_ok),
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


  /* increment this number, so each guessed image name is unique */
  save_image_num++;

  return;
}



/* function called when the button to pop up a rendering parameters modification dialog is hit */
void ui_rendering_cb_parameters(GtkWidget * widget, gpointer data) {
  
  ui_rendering_t * ui_rendering = data;

  ui_rendering_dialog_create(ui_rendering);

  return;
}


#ifdef AMIDE_MPEG_ENCODE_SUPPORT
/* function called when the button to pop up a movie generation dialog */
void ui_rendering_cb_movie(GtkWidget * widget, gpointer data) {
  
  ui_rendering_t * ui_rendering = data;

  ui_rendering->movie = ui_rendering_movie_dialog_create(ui_rendering);

  return;
}
#endif

/* function to run for a delete_event */
gboolean ui_rendering_cb_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_rendering_t * ui_rendering = data;
  GtkWidget * app = GTK_WIDGET(ui_rendering->app);
#ifdef AMIDE_MPEG_ENCODE_SUPPORT
  ui_rendering_movie_t * ui_rendering_movie = ui_rendering->movie;
#endif

  /* if our parameter modification dialog is up, kill that */
  if (ui_rendering->parameter_dialog  != NULL) {
    gnome_dialog_close(GNOME_DIALOG(ui_rendering->parameter_dialog));
    ui_rendering->parameter_dialog = NULL;
  }

#ifdef AMIDE_MPEG_ENCODE_SUPPORT
  /* if the movie dialog is up, kill that */
  if (ui_rendering_movie  != NULL)
    gnome_dialog_close(GNOME_DIALOG(ui_rendering_movie->dialog));
#endif

  ui_rendering = ui_rendering_free(ui_rendering); /* free the associated data structure */
  amide_unregister_window((gpointer) app); /* tell amide this window is no longer */

  return FALSE;
}

/* function ran when closing the rendering window */
void ui_rendering_cb_close(GtkWidget* widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;
  GtkWidget * app = GTK_WIDGET(ui_rendering->app);

  /* run the delete event function */
  ui_rendering_cb_delete_event(app, NULL, ui_rendering);
  gtk_widget_destroy(app);

  return;
}

#endif










