/* ui_render.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2002 Andy Loening
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

#include "amide_config.h"

#ifdef AMIDE_LIBVOLPACK_SUPPORT

#include <gnome.h>
#include <libgnomecanvas/gnome-canvas-pixbuf.h>
#include "image.h"
#include "ui_common.h"
#include "ui_render.h"
#include "ui_render_dialog.h"
#include "ui_render_movie.h"
#include "gtkdial.h"
#include "amitk_progress_dialog.h"





#define UPDATE_NONE 0
#define UPDATE_RENDERING 0x1



static gboolean canvas_event_cb(GtkWidget* widget,  GdkEvent * event, gpointer data);
static void render_cb(GtkWidget * widget, gpointer data);
static void immediate_cb(GtkWidget * widget, gpointer data);
static void stereoscopic_cb(GtkWidget * widget, gpointer data);
static void rotate_cb(GtkAdjustment * adjustment, gpointer data);
static void reset_axis_pressed_cb(GtkWidget * widget, gpointer data);
static void export_cb(GtkWidget * widget, gpointer data);
static void parameters_cb(GtkWidget * widget, gpointer data);
#ifdef AMIDE_LIBFAME_SUPPORT
static void movie_cb(GtkWidget * widget, gpointer data);
#endif
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data);
static void close_cb(GtkWidget* widget, gpointer data);


static ui_render_t * ui_render_init(GnomeApp * app,
				    AmitkStudy * study,
				    gboolean zero_fill,
				    gboolean conserve_memory);
static ui_render_t * ui_render_free(ui_render_t * ui_render);


/* function called when the canvas is hit */

/* function called when an event occurs on the image canvas, 
   notes:
   -events for non-new roi's are handled by ui_study_rois_cb_roi_event 

*/
static gboolean canvas_event_cb(GtkWidget* widget,  GdkEvent * event, gpointer data) {

  ui_render_t * ui_render = data;
  GnomeCanvas * canvas;
  AmitkPoint temp_point;
  AmitkCanvasPoint temp_cpoint1, temp_cpoint2;
  AmitkCanvasPoint canvas_cpoint, diff_cpoint;
  GnomeCanvasPoints * line_points;
  guint i, j,k;
  rgba_t color;
  gdouble dim;
  gdouble height, width;
  static AmitkPoint prev_theta;
  static AmitkPoint theta;
  static AmitkCanvasPoint initial_cpoint, center_cpoint;
  static gboolean dragging = FALSE;
  static GnomeCanvasItem * rotation_box[8];
  static AmitkPoint box_point[8];

  canvas = GNOME_CANVAS(widget);

  /* get the location of the event, and convert it to the canvas coordinates */
  gnome_canvas_w2c_d(canvas, event->button.x, event->button.y, &canvas_cpoint.x, &canvas_cpoint.y);
  height = gdk_pixbuf_get_height(ui_render->pixbuf);
  width = gdk_pixbuf_get_width(ui_render->pixbuf);
  dim = MIN(width, height);
  temp_point.x = temp_point.y = temp_point.z = -dim/2.0;
  amitk_space_set_offset(ui_render->box_space, temp_point);
  //  amitk_space_set_axes(ui_render->box_space, base_axes, AMITK_SPACE_OFFSET(ui_render->box_space));
  amitk_space_set_axes(ui_render->box_space, base_axes, AMITK_SPACE_OFFSET(ui_render->box_space));


  /* switch on the event which called this */
  switch (event->type)
    {


    case GDK_ENTER_NOTIFY:
      ui_common_place_cursor(UI_CURSOR_DATA_SET_MODE, GTK_WIDGET(canvas));
      break;


     case GDK_LEAVE_NOTIFY:
       ui_common_remove_cursor(GTK_WIDGET(canvas));
       break;



    case GDK_BUTTON_PRESS:
      if ((event->button.button == 1) || (event->button.button == 2)) {
	dragging = TRUE;
	initial_cpoint = canvas_cpoint;
	center_cpoint.x = center_cpoint.y = dim/2.0;
	prev_theta = zero_point;

	/* figure out the eight vertices */
	for (i=0; i<8; i++) {
	  box_point[i].x = (i & 0x1) ? BOX_OFFSET * dim : (1-BOX_OFFSET) * dim;
	  box_point[i].y = (i & 0x2) ? BOX_OFFSET * dim : (1-BOX_OFFSET) * dim;
	  box_point[i].z = (i & 0x4) ? BOX_OFFSET * dim : (1-BOX_OFFSET) * dim;
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
	  line_points->coords[0] = box_point[j].x; /* x1 */
	  line_points->coords[1] = box_point[j].y; /* y1 */
	  line_points->coords[2] = box_point[k].x;  /* x2 */
	  line_points->coords[3] = box_point[k].y; /* y2 */
	  color = 
	    amitk_color_table_outline_color(ui_render->renderings->rendering->color_table, TRUE);
	  rotation_box[i] = 
	    gnome_canvas_item_new(gnome_canvas_root(ui_render->canvas), gnome_canvas_line_get_type(),
				  "points", line_points, 
				  "fill_color_rgba", amitk_color_table_rgba_to_uint32(color),
				  "width_units", 1.0, NULL);
	  gnome_canvas_points_unref(line_points);
	}

	/* translate the 8 vertices back to the base frame */
	for (i=0; i<8; i++)
	  box_point[i] = amitk_space_s2b(ui_render->box_space, box_point[i]);

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

	if (event->motion.state & GDK_BUTTON1_MASK) {
	  diff_cpoint = canvas_point_sub(initial_cpoint, canvas_cpoint);
	  theta.y = M_PI * diff_cpoint.x / dim;
	  theta.x = M_PI * -diff_cpoint.y / dim; 

	  /* rotate the axis */
	  amitk_space_rotate_on_vector(ui_render->box_space,
				       amitk_space_get_axis(ui_render->box_space, AMITK_AXIS_X),
				       theta.x, zero_point);
	  amitk_space_rotate_on_vector(ui_render->box_space,
				       amitk_space_get_axis(ui_render->box_space, AMITK_AXIS_Y),
				       theta.y, zero_point);

	  renderings_set_rotation(ui_render->renderings, AMITK_AXIS_Y, prev_theta.y);
	  renderings_set_rotation(ui_render->renderings, AMITK_AXIS_X, -prev_theta.x);
	  renderings_set_rotation(ui_render->renderings, AMITK_AXIS_X, theta.x);
	  renderings_set_rotation(ui_render->renderings, AMITK_AXIS_Y, -theta.y);
	} else {/* button 2 */
	  temp_cpoint1 = canvas_point_sub(initial_cpoint,center_cpoint);
	  temp_cpoint2 = canvas_point_sub(canvas_cpoint,center_cpoint);

	  /* figure out theta.z */
	  theta.z = acos(canvas_point_dot_product(temp_cpoint1, temp_cpoint2) /
			 (canvas_point_mag(temp_cpoint1) * canvas_point_mag(temp_cpoint2)));
	  
	  /* correct for the fact that acos is always positive by using the cross product */
	  if ((temp_cpoint1.x*temp_cpoint2.y-temp_cpoint1.y*temp_cpoint2.x) < 0.0)
	    theta.z = -theta.z;
	  amitk_space_rotate_on_vector(ui_render->box_space,
				       amitk_space_get_axis(ui_render->box_space, AMITK_AXIS_Z),
				       -theta.z, zero_point);

	  renderings_set_rotation(ui_render->renderings, AMITK_AXIS_Z, -prev_theta.z);
	  renderings_set_rotation(ui_render->renderings, AMITK_AXIS_Z, theta.z);
	}
	/* recalculate the offset */
	temp_point.x = temp_point.y = temp_point.z = -dim/2.0;
	amitk_space_set_offset(ui_render->box_space, zero_point);
	amitk_space_set_offset(ui_render->box_space, 
			       amitk_space_s2b(ui_render->box_space, temp_point));

	/* translate the 8 vertices */
	for (i=0; i<8; i++)
	  box_point[i] = amitk_space_b2s(ui_render->box_space, box_point[i]);
	
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
	  line_points->coords[0] = box_point[j].x; /* x1 */
	  line_points->coords[1] = box_point[j].y; /* y1 */
	  line_points->coords[2] = box_point[k].x;  /* x2 */
	  line_points->coords[3] = box_point[k].y; /* y2 */
	  gnome_canvas_item_set(rotation_box[i],  "points", line_points, NULL);
	  gnome_canvas_points_unref(line_points);
	}
	
	/* translate the 8 vertices back to the base frame */
	for (i=0; i<8; i++)
	  box_point[i] = amitk_space_s2b(ui_render->box_space, box_point[i]);

	if (ui_render->update_without_release) 
	  ui_render_add_update(ui_render); 

	prev_theta = theta;
      }

      break;

    case GDK_BUTTON_RELEASE:
      if (dragging) {
	gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(rotation_box[0]), event->button.time);
	dragging = FALSE;

	/* get rid of the frame */
	for (i=0; i<8; i++)
	  gtk_object_destroy(GTK_OBJECT(rotation_box[i]));

	/* render now if appropriate*/
	if (!ui_render->update_without_release) 
	  ui_render_add_update(ui_render); 

      }
      break;
    default:
      break;
    }
  
  return FALSE;
}

/* function callback for the "render" button */
static void render_cb(GtkWidget * widget, gpointer data) {

  ui_render_t * ui_render = data;

  ui_render_add_update(ui_render); 

  return;
}


/* function to switch into immediate rendering mode */
static void immediate_cb(GtkWidget * widget, gpointer data) {

  ui_render_t * ui_render = data;

  /* should we be rendering immediately */
  ui_render->immediate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  /* set the sensitivity of the render button */
  gtk_widget_set_sensitive(GTK_WIDGET(ui_render->render_button), !(ui_render->immediate));

  ui_render_update_immediate(ui_render); 

  return;

}


/* function to switch into stereoscopic rendering mode */
static void stereoscopic_cb(GtkWidget * widget, gpointer data) {
  ui_render_t * ui_render = data;

  ui_render->stereoscopic = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  ui_render_add_update(ui_render); 

  return;

}





/* function called when one of the rotate widgets has been hit */
static void rotate_cb(GtkAdjustment * adjustment, gpointer data) {

  ui_render_t * ui_render = data;
  AmitkAxis i_axis;
  gdouble rot;

  /* figure out which rotate widget called me */
  i_axis = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(adjustment),"axis"));
  rot = (adjustment->value/180.0)*M_PI; /* get rotation in radians */

  /* update the rotation values */
  renderings_set_rotation(ui_render->renderings, i_axis, rot);

  /* render now if appropriate*/
  ui_render_add_update(ui_render); 

  /* return adjustment back to normal */
  adjustment->value = 0.0;
  gtk_adjustment_changed(adjustment);

  return;
}

/* function called to snap the axis back to the default */
static void reset_axis_pressed_cb(GtkWidget * widget, gpointer data) {
  ui_render_t * ui_render = data;

  /* reset the rotations */
  renderings_reset_rotation(ui_render->renderings);

  ui_render_add_update(ui_render); 

  return;
}


/* function to handle exporting a view */
static void export_ok_cb(GtkWidget* widget, gpointer data) {

  GtkWidget * file_selection = data;
  ui_render_t * ui_render;
  const gchar * save_filename;

  /* get a pointer to ui_render */
  ui_render = g_object_get_data(G_OBJECT(file_selection), "ui_render");

  if ((save_filename = ui_common_file_selection_get_name(file_selection)) == NULL)
    return; /* inappropriate name or don't want to overwrite */

  if (gdk_pixbuf_save (ui_render->pixbuf, save_filename, "jpeg", NULL, 
		       "quality", "100", NULL) == FALSE) {
    g_warning("Failure Saving File: %s",save_filename);
    return;
  }



  /* close the file selection box */
  ui_common_file_selection_cancel_cb(widget, file_selection);

  return;
}

/* function to save a rendering as an external data format */
static void export_cb(GtkWidget * widget, gpointer data) {

  ui_render_t * ui_render = data;
  renderings_t * temp_renderings;
  GtkFileSelection * file_selection;
  gchar * temp_string;
  gchar * data_set_names = NULL;
  static guint save_image_num = 0;

  g_return_if_fail(ui_render->pixbuf != NULL);

  file_selection = GTK_FILE_SELECTION(gtk_file_selection_new(_("Export File")));

  /* take a guess at the filename */
  temp_renderings = ui_render->renderings;
  data_set_names = g_strdup(temp_renderings->rendering->name);
  temp_renderings = temp_renderings->next;
  while (temp_renderings != NULL) {
    temp_string = g_strdup_printf("%s+%s",data_set_names, temp_renderings->rendering->name);
    g_free(data_set_names);
    data_set_names = temp_string;
    temp_renderings = temp_renderings->next;
  }
  temp_string = g_strdup_printf("Rendering%s{%s}_%d.jpg", 
				ui_render->stereoscopic ? "_stereo_" : "_", 
				data_set_names,save_image_num++);
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selection), temp_string);
  g_free(data_set_names);
  g_free(temp_string); 

  /* save a pointer to ui_render */
  g_object_set_data(G_OBJECT(file_selection), "ui_render", ui_render);
    
  /* don't want anything else going on till this window is gone */
  //  gtk_window_set_modal(GTK_WINDOW(file_selection), TRUE);
  // for some reason, this modal's the application permanently...

  /* connect the signals */
  g_signal_connect(G_OBJECT(file_selection->ok_button), "clicked",
		   G_CALLBACK(export_ok_cb), file_selection);
  g_signal_connect(G_OBJECT(file_selection->cancel_button), "clicked",
		   G_CALLBACK(ui_common_file_selection_cancel_cb), file_selection);
  g_signal_connect(G_OBJECT(file_selection->cancel_button), "delete_event",
		   G_CALLBACK(ui_common_file_selection_cancel_cb), file_selection);

  /* set the position of the dialog */
  gtk_window_set_position(GTK_WINDOW(file_selection), GTK_WIN_POS_MOUSE);

  /* run the dialog */
  gtk_widget_show(GTK_WIDGET(file_selection));

  return;
}



/* function called when the button to pop up a rendering parameters modification dialog is hit */
static void parameters_cb(GtkWidget * widget, gpointer data) {
  ui_render_t * ui_render = data;
  ui_render_dialog_create(ui_render);
  return;
}


#ifdef AMIDE_LIBFAME_SUPPORT
/* function called when the button to pop up a movie generation dialog */
static void movie_cb(GtkWidget * widget, gpointer data) {
  ui_render_t * ui_render = data;
  ui_render->movie = ui_render_movie_dialog_create(ui_render);
  return;
}
#endif

/* function to run for a delete_event */
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_render_t * ui_render = data;
  GtkWidget * app = GTK_WIDGET(ui_render->app);

  ui_render = ui_render_free(ui_render); /* free the associated data structure */
  amide_unregister_window((gpointer) app); /* tell amide this window is no longer */

  return FALSE;
}

/* function ran when closing the rendering window */
static void close_cb(GtkWidget* widget, gpointer data) {

  ui_render_t * ui_render = data;
  GtkWidget * app = GTK_WIDGET(ui_render->app);
  gboolean return_val;

  /* run the delete event function */
  g_signal_emit_by_name(G_OBJECT(app), "delete_event", NULL, &return_val);
  if (!return_val) gtk_widget_destroy(app);

  return;
}





/* function to setup the menus for the rendering ui */
static void menus_create(ui_render_t * ui_render) {


  GnomeUIInfo file_menu[] = {
    GNOMEUIINFO_ITEM_DATA(N_("_Export Rendering"),
			  N_("Export the rendered image"),
			  export_cb,
			  ui_render, NULL),
#ifdef AMIDE_LIBFAME_SUPPORT
    GNOMEUIINFO_ITEM_DATA(N_("_Create Movie"),
			  N_("Create a movie out of a sequence of renderings"),
			  movie_cb,
			  ui_render, NULL),
#endif
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_MENU_CLOSE_ITEM(close_cb, ui_render),
    GNOMEUIINFO_END
  };

  GnomeUIInfo edit_menu[] = {
    GNOMEUIINFO_ITEM_DATA(N_("_Rendering Parameters"),
			  N_("Adjust parameters pertinent to the rendered image"),
			  parameters_cb,
			  ui_render, NULL),
    GNOMEUIINFO_END
  };

  /* and the main menu definition */
  GnomeUIInfo ui_render_main_menu[] = {
    GNOMEUIINFO_MENU_FILE_TREE(file_menu),
    GNOMEUIINFO_MENU_EDIT_TREE(edit_menu),
    GNOMEUIINFO_MENU_HELP_TREE(ui_common_help_menu),
    GNOMEUIINFO_END
  };


  /* sanity check */
  g_assert(ui_render !=NULL);

  /* make the menu */
  gnome_app_create_menus(GNOME_APP(ui_render->app), ui_render_main_menu);

  return;

}




/* destroy a ui_render data structure */
static ui_render_t * ui_render_free(ui_render_t * ui_render) {

  gboolean return_val;

  if (ui_render == NULL)
    return ui_render;

  /* sanity checks */
  g_return_val_if_fail(ui_render->reference_count > 0, NULL);

  ui_render->reference_count--;

  /* things to do if we've removed all reference's */
  if (ui_render->reference_count == 0) {
    ui_render->renderings = renderings_unref(ui_render->renderings);
#ifdef AMIDE_DEBUG
    g_print("freeing ui_render\n");
#endif

    if (ui_render->idle_handler_id != 0) {
      gtk_idle_remove(ui_render->idle_handler_id);
      ui_render->idle_handler_id = 0;
    }

    if (ui_render->pixbuf != NULL) {
      g_object_unref(ui_render->pixbuf);
      ui_render->pixbuf = NULL;
    }
    
    if (ui_render->box_space != NULL) {
      g_object_unref(ui_render->box_space);
      ui_render->box_space = NULL;
    }
    
    if (ui_render->progress_dialog != NULL) {
      g_signal_emit_by_name(G_OBJECT(ui_render->progress_dialog), "delete_event", NULL, &return_val);
      ui_render->progress_dialog = NULL;
    }

    g_free(ui_render);
    ui_render = NULL;
  }

  return ui_render;

}



/* allocate and initialize a ui_render data structure */
static ui_render_t * ui_render_init(GnomeApp * app,
				    AmitkStudy * study,
				    gboolean zero_fill,
				    gboolean conserve_memory) {

  ui_render_t * ui_render;
  GList * visible_objects;

  /* alloc space for the data structure for passing ui info */
  if ((ui_render = g_try_new(ui_render_t,1)) == NULL) {
    g_warning("couldn't allocate space for ui_render_t");
    return NULL;
  }
  ui_render->reference_count = 1;

  /* set any needed parameters */
  ui_render->app = app;
  ui_render->parameter_dialog = NULL;
#ifdef AMIDE_LIBFAME_SUPPORT
  ui_render->movie = NULL;
#endif
  ui_render->render_button = NULL;
  ui_render->immediate = TRUE;
  ui_render->stereoscopic = FALSE;
  ui_render->renderings = NULL;
  ui_render->pixbuf = NULL;
  ui_render->canvas_image = NULL;
  ui_render->quality = RENDERING_DEFAULT_QUALITY;
  ui_render->depth_cueing = RENDERING_DEFAULT_DEPTH_CUEING;
  ui_render->front_factor = RENDERING_DEFAULT_FRONT_FACTOR;
  ui_render->density = RENDERING_DEFAULT_DENSITY;
  ui_render->zoom = RENDERING_DEFAULT_ZOOM;
  ui_render->start = AMITK_STUDY_VIEW_START_TIME(study);
  ui_render->duration = AMITK_STUDY_VIEW_DURATION(study);
  ui_render->box_space = amitk_space_new();
  ui_render->progress_dialog = amitk_progress_dialog_new(GTK_WINDOW(ui_render->app));
  ui_render->next_update= UPDATE_NONE;
  ui_render->idle_handler_id = 0;

  /* load in saved preferences */
  gnome_config_push_prefix("/"PACKAGE"/");

  ui_render->update_without_release = gnome_config_get_int("RENDERING/UpdateWithoutRelease");

  ui_render->stereo_eye_width = gnome_config_get_int("RENDERING/EyeWidth");
  if (ui_render->stereo_eye_width == 0)  /* if no config file, put in sane value */
    ui_render->stereo_eye_width = 50*gdk_screen_width()/gdk_screen_width_mm(); /* in pixels */

  ui_render->stereo_eye_angle = gnome_config_get_float("RENDERING/EyeAngle");
  if ((ui_render->stereo_eye_angle <= 0.1) || (ui_render->stereo_eye_angle > 45.0))
    ui_render->stereo_eye_angle = 2.0; /* degrees */

  gnome_config_pop_prefix();

  /* initialize the rendering contexts */
  visible_objects = amitk_object_get_selected_children(AMITK_OBJECT(study), AMITK_VIEW_MODE_SINGLE, TRUE);
  ui_render->renderings = renderings_init(visible_objects, 
					  ui_render->start, 
					  ui_render->duration, 
					  zero_fill, conserve_memory,
					  amitk_progress_dialog_update,
					  ui_render->progress_dialog);
  amitk_objects_unref(visible_objects);


  return ui_render;
}



void ui_render_add_update(ui_render_t * ui_render) {

  if (ui_render->immediate) {
    ui_render->next_update = ui_render->next_update | UPDATE_RENDERING;
    if (ui_render->idle_handler_id == 0)
      ui_render->idle_handler_id = 
	gtk_idle_add_priority(G_PRIORITY_HIGH_IDLE,ui_render_update_immediate, ui_render);
  }

  return;
}

/* render our objects and place into the canvases */
gboolean ui_render_update_immediate(gpointer data) {

  ui_render_t * ui_render = data;
  amide_intpoint_t size_dim; 
  AmitkEye eyes;
  gint width, height;
  gboolean return_val=TRUE;

  g_return_val_if_fail(ui_render != NULL, FALSE);
  g_return_val_if_fail(ui_render->renderings != NULL, FALSE);

  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_render->canvas));

  if (!renderings_reload_objects(ui_render->renderings, ui_render->start,
				 ui_render->duration,
				 amitk_progress_dialog_update, 
				 ui_render->progress_dialog)) {
    return_val=FALSE;
    goto function_end;
  }

  /* -------- render our objects ------------ */

  if (ui_render->stereoscopic) eyes = AMITK_EYE_NUM;
  else eyes = 1;

  if (ui_render->pixbuf != NULL) {
    g_object_unref(ui_render->pixbuf);
    ui_render->pixbuf = NULL;
  }

  /* base the dimensions on the first rendering context in the list.... */
  size_dim = ceil(ui_render->zoom*POINT_MAX(ui_render->renderings->rendering->dim));
  ui_render->pixbuf = image_from_renderings(ui_render->renderings, 
					    size_dim, size_dim, eyes,
					    ui_render->stereo_eye_angle, 
					    ui_render->stereo_eye_width); 
  
  /* put up the image */
  if (ui_render->canvas_image != NULL) 
    gnome_canvas_item_set(ui_render->canvas_image,
			  "pixbuf", ui_render->pixbuf, NULL);
  else /* time to make a new image */
    ui_render->canvas_image = 
      gnome_canvas_item_new(gnome_canvas_root(ui_render->canvas),
			    gnome_canvas_pixbuf_get_type(),
			    "pixbuf", ui_render->pixbuf,
			    "x", (double) 0.0,
			    "y", (double) 0.0,
			    NULL);

  width = gdk_pixbuf_get_width(ui_render->pixbuf);  
  height = gdk_pixbuf_get_height(ui_render->pixbuf);  

  /* reset the min size of the widget */
  gnome_canvas_set_scroll_region(ui_render->canvas, 0.0, 0.0, width, height);
  gtk_widget_set_size_request(GTK_WIDGET(ui_render->canvas), width, height);

 function_end:

  ui_common_remove_cursor(GTK_WIDGET(ui_render->canvas));
  ui_render->next_update = UPDATE_NONE;

  if (ui_render->idle_handler_id != 0) {
    gtk_idle_remove(ui_render->idle_handler_id);
    ui_render->idle_handler_id=0;
  }

  return return_val;
} 



/* function that sets up the rendering dialog */
void ui_render_create(AmitkStudy * study, gboolean zero_fill, gboolean conserve_memory) {
  
  GtkWidget * packing_table;
  GtkWidget * check_button;
  GtkWidget * button;
  GtkAdjustment * adjustment;
  GtkWidget * scale;
  GtkWidget * vbox;
  GtkWidget * hbox;
  GtkWidget * hseparator;
  GtkWidget * dial;
  GtkWidget * label;
  GnomeCanvas * axis_indicator;
  GnomeCanvasPoints * axis_line_points;
  ui_render_t * ui_render;
  GtkWidget * app;
  gboolean return_val;

  /* sanity checks */
  g_return_if_fail(AMITK_IS_STUDY(study));

  app = gnome_app_new(PACKAGE, "Rendering Window");
  gtk_window_set_resizable(GTK_WINDOW(app), TRUE);
  ui_render = ui_render_init(GNOME_APP(app), study, zero_fill,conserve_memory);

  /* check we actually have something */
  if (ui_render->renderings == NULL) {
    /* run the delete event function */
    g_signal_emit_by_name(G_OBJECT(app), "delete_event", NULL, &return_val);
    if (!return_val) gtk_widget_destroy(app);
    return;
  }
    
  /* setup the callbacks for app */
  g_signal_connect(G_OBJECT(app), "realize", 
		   G_CALLBACK(ui_common_window_realize_cb), NULL);
  g_signal_connect(G_OBJECT(app), "delete_event",
		   G_CALLBACK(delete_event_cb), ui_render);

  /* setup the menus */
  menus_create(ui_render);

  /* make the widgets for this dialog box */
  packing_table = gtk_table_new(3,3,FALSE);
  gnome_app_set_contents(ui_render->app, packing_table);

  /* start making those widgets */
  vbox = gtk_vbox_new(FALSE, Y_PADDING);
  gtk_table_attach(GTK_TABLE(packing_table), vbox, 0,1,0,2,
		   X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);

  /* create the z dial */
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new("z");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
  adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, -90.0, 90.0, 1.0, 1.0, 1.0));

  dial = gtk_dial_new(adjustment);
  gtk_dial_set_update_policy (GTK_DIAL(dial), GTK_UPDATE_DISCONTINUOUS);
  g_object_set_data(G_OBJECT(adjustment), "axis", GINT_TO_POINTER(AMITK_AXIS_Z));
  gtk_box_pack_start(GTK_BOX(hbox), dial, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT(adjustment), "value_changed", 
		    G_CALLBACK(rotate_cb), ui_render);


  /* a canvas to indicate which way is x, y, and z */
  axis_indicator = GNOME_CANVAS(gnome_canvas_new_aa());
  gtk_widget_set_size_request(GTK_WIDGET(axis_indicator), 100, 100);
  gnome_canvas_set_scroll_region(axis_indicator, 0.0, 0.0, 100.0, 100.0);
  gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(axis_indicator), TRUE, TRUE, 0);

  /* the x axis */
  axis_line_points = gnome_canvas_points_new(2);
  axis_line_points->coords[0] = 40.0; /* x1 */
  axis_line_points->coords[1] = 60.0; /* y1 */
  axis_line_points->coords[2] = 95.0; /* x2 */
  axis_line_points->coords[3] = 60.0; /* y2 */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_line_get_type(),
			"points", axis_line_points, "fill_color", "black",
			"width_units", 2.0, "last_arrowhead", TRUE, 
			"arrow_shape_a", 8.0,
			"arrow_shape_b", 8.0,
			"arrow_shape_c", 8.0,
			NULL);

  gnome_canvas_points_unref(axis_line_points);
  /* the x label */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator),
			gnome_canvas_text_get_type(),
			"anchor", GTK_ANCHOR_NORTH_EAST,
			"text", "x",
			"x", (gdouble) 80.0,
			"y", (gdouble) 65.0,
			"fill_color", "black",
			"font", "fixed", NULL);

  /* the y axis */
  axis_line_points = gnome_canvas_points_new(2);
  axis_line_points->coords[0] = 40.0; /* x1 */
  axis_line_points->coords[1] = 60.0; /* y1 */
  axis_line_points->coords[2] = 40.0; /* x2 */
  axis_line_points->coords[3] = 5.0; /* y2 */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_line_get_type(),
			"points", axis_line_points, "fill_color", "black",
			"width_units", 2.0, "last_arrowhead", TRUE, 
			"arrow_shape_a", 8.0,
			"arrow_shape_b", 8.0,
			"arrow_shape_c", 8.0,
			NULL);
  gnome_canvas_points_unref(axis_line_points);
  /* the y label */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator),
			gnome_canvas_text_get_type(),
			"anchor", GTK_ANCHOR_NORTH_WEST,
			"text", "y",
			"x", (gdouble) 45.0,
			"y", (gdouble) 20.0,
			"fill_color", "black",
			"font", "fixed", NULL);

  /* the z axis */
  axis_line_points = gnome_canvas_points_new(2);
  axis_line_points->coords[0] = 40.0; /* x1 */
  axis_line_points->coords[1] = 60.0; /* y1 */
  axis_line_points->coords[2] = 5.0; /* x2 */
  axis_line_points->coords[3] = 95.0; /* y2 */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_line_get_type(),
			"points", axis_line_points, "fill_color", "black",
			"width_units", 2.0, "last_arrowhead", TRUE,
			"arrow_shape_a", 8.0,
			"arrow_shape_b", 8.0,
			"arrow_shape_c", 8.0,
			NULL);
  gnome_canvas_points_unref(axis_line_points);
  /* the z label */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator),
			gnome_canvas_text_get_type(),
			"anchor", GTK_ANCHOR_NORTH_WEST,
			"text", "z",
			"x", (gdouble) 20.0,
			"y", (gdouble) 80.0,
			"fill_color", "black",
			"font", "fixed", NULL);

  /* button to reset the axis */
  button = gtk_button_new_with_label("Reset Axis");
  g_signal_connect(G_OBJECT(button), "pressed",
		   G_CALLBACK(reset_axis_pressed_cb), ui_render);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);



  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), hseparator, FALSE, FALSE, 0);

  /* the Render button, after changing parameters, this is the button you'll
   * wanna hit to rerender the object, unless you have immediate rendering set */
  ui_render->render_button = gtk_button_new_with_label("Render");
  gtk_box_pack_start(GTK_BOX(vbox), ui_render->render_button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive(ui_render->render_button, !(ui_render->immediate));
  g_signal_connect(G_OBJECT(ui_render->render_button), "clicked", 
		   G_CALLBACK(render_cb), ui_render);


  /* the render immediately button.. */
  check_button = gtk_check_button_new_with_label ("on change");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), ui_render->immediate);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(immediate_cb), ui_render);
  gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

  /* the render immediately button.. */
  check_button = gtk_check_button_new_with_label ("stereoscopic");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), ui_render->stereoscopic);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(stereoscopic_cb), ui_render);
  gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

  /* setup the main canvas */
  ui_render->canvas = GNOME_CANVAS(gnome_canvas_new_aa());
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(ui_render->canvas), 1,2,1,2,
		   X_PACKING_OPTIONS | GTK_FILL,
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);
  g_signal_connect(G_OBJECT(ui_render->canvas), "event",
		   G_CALLBACK(canvas_event_cb), ui_render);
  ui_render_update_immediate(ui_render); /* fill in the canvas */

  /* create the x, and y rotation dials */
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_table_attach(GTK_TABLE(packing_table), hbox, 1,3,0,1,
		   X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL, 
		   X_PADDING, Y_PADDING);
  adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, -90.0, 90.0, 1.0, 1.0, 1.0));
  scale = gtk_hscale_new(adjustment);
  gtk_range_set_update_policy (GTK_RANGE(scale), GTK_UPDATE_DISCONTINUOUS);
  g_object_set_data(G_OBJECT(adjustment), "axis", GINT_TO_POINTER(AMITK_AXIS_Y));
  gtk_box_pack_start(GTK_BOX(hbox), scale, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT(adjustment), "value_changed", 
		    G_CALLBACK(rotate_cb), ui_render);
  label = gtk_label_new("y");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_table_attach(GTK_TABLE(packing_table), vbox,  3,4,1,2,
		   X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL, 
		   X_PADDING, Y_PADDING);
  label = gtk_label_new("x");
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
  adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, -90.0, 90.0, 1.0, 1.0, 1.0));
  scale = gtk_vscale_new(adjustment);
  gtk_range_set_update_policy (GTK_RANGE(scale), GTK_UPDATE_DISCONTINUOUS);
  g_object_set_data(G_OBJECT(adjustment), "axis", GINT_TO_POINTER(AMITK_AXIS_X));
  gtk_box_pack_start(GTK_BOX(vbox), scale, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(adjustment), "value_changed", 
		   G_CALLBACK(rotate_cb), ui_render);

  /* and show all our widgets */
  gtk_widget_show_all(GTK_WIDGET(ui_render->app));
  amide_register_window((gpointer) ui_render->app);

  return;
}




static void init_optimize_rendering_cb(GtkWidget * widget, gpointer data);
static void init_strip_highs_cb(GtkWidget * widget, gpointer data);
static void init_response_cb (GtkDialog * dialog, gint response_id, gpointer data);




static void init_strip_highs_cb(GtkWidget * widget, gpointer data) {
  GtkWidget * dialog = data;
  gboolean strip_highs;

  strip_highs = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  g_object_set_data(G_OBJECT(dialog), "strip_highs", GINT_TO_POINTER(strip_highs));
  
  gnome_config_push_prefix("/"PACKAGE"/");
  gnome_config_set_int("RENDERING/StripHighs", strip_highs);
  gnome_config_pop_prefix();
  gnome_config_sync();
  return;
}

static void init_optimize_rendering_cb(GtkWidget * widget, gpointer data) {
  GtkWidget * dialog = data;
  gboolean optimize_rendering;

  optimize_rendering =gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  g_object_set_data(G_OBJECT(dialog), "optimize_rendering", GINT_TO_POINTER(optimize_rendering));

  gnome_config_push_prefix("/"PACKAGE"/");
  gnome_config_set_int("RENDERING/OptimizeRendering", optimize_rendering);
  gnome_config_pop_prefix();
  gnome_config_sync();
  return;
}


static void init_response_cb (GtkDialog * dialog, gint response_id, gpointer data) {
  
  gint return_val;

  switch(response_id) {
  case AMITK_RESPONSE_EXECUTE:
  case GTK_RESPONSE_CLOSE:
    g_signal_emit_by_name(G_OBJECT(dialog), "delete_event", NULL, &return_val);
    if (!return_val) gtk_widget_destroy(GTK_WIDGET(dialog));
    break;

  default:
    break;
  }

  return;
}


/* function to setup a dialog to allow us to choice options for rendering */
GtkWidget * ui_render_init_dialog_create(GtkWindow * parent) {
  
  GtkWidget * dialog;
  gchar * temp_string;
  GtkWidget * table;
  GtkWidget * check_button;
  guint table_row;
  gboolean strip_highs;
  gboolean optimize_rendering;

  gnome_config_push_prefix("/"PACKAGE"/");
  strip_highs = gnome_config_get_int("RENDERING/StripHighs");
  optimize_rendering = gnome_config_get_int("RENDERING/OptimizeRendering");
  gnome_config_pop_prefix();

  temp_string = g_strdup_printf("%s: Rendering Initialization Dialog", PACKAGE);
  dialog = gtk_dialog_new_with_buttons (temp_string,  parent,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_EXECUTE, AMITK_RESPONSE_EXECUTE,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CLOSE, NULL);
  gtk_window_set_title(GTK_WINDOW(dialog), temp_string);
  g_free(temp_string);

  /* setup the callbacks for the dialog */
  g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(init_response_cb), NULL);

  gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);

  /* start making the widgets for this dialog box */
  table = gtk_table_new(3,2,FALSE);
  table_row=0;
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);

  /* do we want to strip values */
  check_button = gtk_check_button_new_with_label("Set values greater than max. threshold to zero?");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), strip_highs);
  g_object_set_data(G_OBJECT(dialog), "strip_highs", GINT_TO_POINTER(strip_highs));
  gtk_table_attach(GTK_TABLE(table), check_button, 
		   0,2, table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(init_strip_highs_cb), dialog);
  table_row++;

  /* do we want to converse memory */
  check_button = gtk_check_button_new_with_label("Optimize Rendering?  Increases memory use ~3x");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),optimize_rendering);
  g_object_set_data(G_OBJECT(dialog), "optimize_rendering", GINT_TO_POINTER(optimize_rendering));
  gtk_table_attach(GTK_TABLE(table), check_button, 
		   0,2, table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(init_optimize_rendering_cb), dialog);
  table_row++;

  /* and show all our widgets */
  gtk_widget_show_all(dialog);

  return dialog;
}








#endif




















