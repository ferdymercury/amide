/* ui_rendering.c
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
#include "ui_rendering.h"
#include "ui_rendering_dialog.h"
#include "ui_rendering_movie_dialog.h"
#include "gtkdial.h"

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


static ui_rendering_t * ui_rendering_init(GList * objects, 
					  amide_time_t start, 
					  amide_time_t duration);
static ui_rendering_t * ui_rendering_free(ui_rendering_t * ui_rendering);
static void update_canvas(ui_rendering_t * ui_rendering);



/* function called when the canvas is hit */

/* function called when an event occurs on the image canvas, 
   notes:
   -events for non-new roi's are handled by ui_study_rois_cb_roi_event 

*/
static gboolean canvas_event_cb(GtkWidget* widget,  GdkEvent * event, gpointer data) {

  ui_rendering_t * ui_rendering = data;
  GnomeCanvas * canvas;
  AmitkPoint temp_point;
  AmitkCanvasPoint temp_cpoint1, temp_cpoint2;
  AmitkCanvasPoint canvas_cpoint, diff_cpoint;
  GnomeCanvasPoints * line_points;
  guint i, j,k;
  rgba_t color;
  static amide_real_t dim;
  static AmitkCanvasPoint last_cpoint, initial_cpoint, center_cpoint;
  static gboolean dragging = FALSE;
  static GnomeCanvasItem * rotation_box[8];
  static AmitkPoint box_point[8];
  static AmitkPoint theta;

  canvas = GNOME_CANVAS(widget);

  /* get the location of the event, and convert it to the canvas coordinates */
  gnome_canvas_w2c_d(canvas, event->button.x, event->button.y, &canvas_cpoint.x, &canvas_cpoint.y);
  //g_print("location %5.3f %5.3f\n",canvas_cpoint.x, canvas_cpoint.y);

  dim = MIN(gdk_pixbuf_get_width(ui_rendering->pixbuf),
	    gdk_pixbuf_get_height(ui_rendering->pixbuf));
  temp_point.x = temp_point.y = temp_point.z = -dim/2.0;
  amitk_space_set_offset(ui_rendering->box_space, temp_point);
  amitk_space_set_axes(ui_rendering->box_space, base_axes, AMITK_SPACE_OFFSET(ui_rendering->box_space));

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
	initial_cpoint = last_cpoint = canvas_cpoint;
	theta = zero_point;
	center_cpoint.x = center_cpoint.y = dim/2.0;

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
	    amitk_color_table_outline_color(ui_rendering->contexts->context->color_table, TRUE);
	  rotation_box[i] = 
	    gnome_canvas_item_new(gnome_canvas_root(ui_rendering->canvas), gnome_canvas_line_get_type(),
				  "points", line_points, 
				  "fill_color_rgba", amitk_color_table_rgba_to_uint32(color),
				  "width_units", 1.0, NULL);
	  gnome_canvas_points_unref(line_points);
	}

	/* translate the 8 vertices back to the base frame */
	for (i=0; i<8; i++)
	  box_point[i] = amitk_space_s2b(ui_rendering->box_space, box_point[i]);

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
	diff_cpoint = canvas_point_sub(initial_cpoint, canvas_cpoint);

	if (event->motion.state & GDK_BUTTON1_MASK) {
	  theta.y = M_PI * diff_cpoint.x / dim;
	  theta.x = M_PI * -diff_cpoint.y / dim; /* compensate for y's origin being top */

	  /* rotate the axis */
	  amitk_space_rotate_on_vector(ui_rendering->box_space,
				       amitk_space_get_axis(ui_rendering->box_space, AMITK_AXIS_X),
				       theta.x, zero_point);
	  amitk_space_rotate_on_vector(ui_rendering->box_space,
				       amitk_space_get_axis(ui_rendering->box_space, AMITK_AXIS_Y),
				       theta.y, zero_point);
	} else {/* button 2 */
	  temp_cpoint1 = canvas_point_sub(initial_cpoint,center_cpoint);
	  temp_cpoint2 = canvas_point_sub(canvas_cpoint,center_cpoint);

	  /* figure out theta.z */
	  theta.z = acos(canvas_point_dot_product(temp_cpoint1, temp_cpoint2) /
			 (canvas_point_mag(temp_cpoint1) * canvas_point_mag(temp_cpoint2)));
	  
	  /* correct for the fact that acos is always positive by using the cross product */
	  if ((temp_cpoint1.x*temp_cpoint2.y-temp_cpoint1.y*temp_cpoint2.x) > 0.0)
	    theta.z = -theta.z;
	  amitk_space_rotate_on_vector(ui_rendering->box_space,
				       amitk_space_get_axis(ui_rendering->box_space, AMITK_AXIS_Z),
				       theta.z, zero_point);
	}
	/* recalculate the offset */
	temp_point.x = temp_point.y = temp_point.z = -dim/2.0;
	amitk_space_set_offset(ui_rendering->box_space, zero_point);
	amitk_space_set_offset(ui_rendering->box_space, 
			       amitk_space_s2b(ui_rendering->box_space, temp_point));

	/* translate the 8 vertices */
	for (i=0; i<8; i++)
	  box_point[i] = amitk_space_b2s(ui_rendering->box_space, box_point[i]);
	
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
	  box_point[i] = amitk_space_s2b(ui_rendering->box_space, box_point[i]);


	last_cpoint = canvas_cpoint;
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
	renderings_set_rotation(ui_rendering->contexts, AMITK_AXIS_X, theta.x);
	renderings_set_rotation(ui_rendering->contexts, AMITK_AXIS_Y, -theta.y); 
	renderings_set_rotation(ui_rendering->contexts, AMITK_AXIS_Z, -theta.z); 

	/* render now if appropriate*/
	ui_rendering_update_canvas(ui_rendering, FALSE); 

      }
      break;
    default:
      break;
    }
  
  return FALSE;
}

/* function callback for the "render" button */
static void render_cb(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;

  ui_rendering_update_canvas(ui_rendering, TRUE); 

  return;
}


/* function to switch into immediate rendering mode */
static void immediate_cb(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;

  /* should we be rendering immediately */
  ui_rendering->immediate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  /* set the sensitivity of the render button */
  gtk_widget_set_sensitive(GTK_WIDGET(ui_rendering->render_button), !(ui_rendering->immediate));

  ui_rendering_update_canvas(ui_rendering, FALSE); 

  return;

}


/* function to switch into stereoscopic rendering mode */
static void stereoscopic_cb(GtkWidget * widget, gpointer data) {
  ui_rendering_t * ui_rendering = data;

  ui_rendering->stereoscopic = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  ui_rendering_update_canvas(ui_rendering, FALSE); 

  return;

}





/* function called when one of the rotate widgets has been hit */
static void rotate_cb(GtkAdjustment * adjustment, gpointer data) {

  ui_rendering_t * ui_rendering = data;
  AmitkAxis i_axis;
  gdouble rot;

  /* figure out which rotate widget called me */
  i_axis = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(adjustment),"axis"));
  rot = (adjustment->value/180.0)*M_PI; /* get rotation in radians */

  /* update the rotation values */
  renderings_set_rotation(ui_rendering->contexts, i_axis, rot);

  /* render now if appropriate*/
  ui_rendering_update_canvas(ui_rendering, FALSE); 

  /* return adjustment back to normal */
  adjustment->value = 0.0;
  gtk_adjustment_changed(adjustment);

  return;
}

/* function called to snap the axis back to the default */
static void reset_axis_pressed_cb(GtkWidget * widget, gpointer data) {
  ui_rendering_t * ui_rendering = data;

  /* reset the rotations */
  renderings_reset_rotation(ui_rendering->contexts);

  ui_rendering_update_canvas(ui_rendering, FALSE); 

  return;
}


/* function to handle exporting a view */
static void export_ok_cb(GtkWidget* widget, gpointer data) {

  GtkWidget * file_selection = data;
  ui_rendering_t * ui_rendering;
  const gchar * save_filename;

  /* get a pointer to ui_rendering */
  ui_rendering = g_object_get_data(G_OBJECT(file_selection), "ui_rendering");

  if ((save_filename = ui_common_file_selection_get_name(file_selection)) == NULL)
    return; /* inappropriate name or don't want to overwrite */

  if (gdk_pixbuf_save (ui_rendering->pixbuf, save_filename, "jpeg", NULL, 
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

  ui_rendering_t * ui_rendering = data;
  renderings_t * temp_contexts;
  GtkFileSelection * file_selection;
  gchar * temp_string;
  gchar * data_set_names = NULL;
  static guint save_image_num = 0;

  g_return_if_fail(ui_rendering->pixbuf != NULL);

  file_selection = GTK_FILE_SELECTION(gtk_file_selection_new(_("Export File")));

  /* take a guess at the filename */
  temp_contexts = ui_rendering->contexts;
  data_set_names = g_strdup(temp_contexts->context->name);
  temp_contexts = temp_contexts->next;
  while (temp_contexts != NULL) {
    temp_string = g_strdup_printf("%s+%s",data_set_names, temp_contexts->context->name);
    g_free(data_set_names);
    data_set_names = temp_string;
    temp_contexts = temp_contexts->next;
  }
  temp_string = g_strdup_printf("Rendering%s{%s}_%d.jpg", 
				ui_rendering->stereoscopic ? "_stereo_" : "_", 
				data_set_names,save_image_num++);
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selection), temp_string);
  g_free(data_set_names);
  g_free(temp_string); 

  /* save a pointer to ui_rendering */
  g_object_set_data(G_OBJECT(file_selection), "ui_rendering", ui_rendering);
    
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
  ui_rendering_t * ui_rendering = data;
  ui_rendering_dialog_create(ui_rendering);
  return;
}


#ifdef AMIDE_LIBFAME_SUPPORT
/* function called when the button to pop up a movie generation dialog */
static void movie_cb(GtkWidget * widget, gpointer data) {
  ui_rendering_t * ui_rendering = data;
  ui_rendering->movie = ui_rendering_movie_dialog_create(ui_rendering);
  return;
}
#endif

/* function to run for a delete_event */
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_rendering_t * ui_rendering = data;
  GtkWidget * app = GTK_WIDGET(ui_rendering->app);
  ui_rendering = ui_rendering_free(ui_rendering); /* free the associated data structure */
  
  amide_unregister_window((gpointer) app); /* tell amide this window is no longer */

  return FALSE;
}

/* function ran when closing the rendering window */
static void close_cb(GtkWidget* widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;
  GtkWidget * app = GTK_WIDGET(ui_rendering->app);
  gboolean return_val;

  /* run the delete event function */
  g_signal_emit_by_name(G_OBJECT(app), "delete_event", NULL, &return_val);
  if (!return_val) gtk_widget_destroy(app);

  return;
}





/* function to setup the menus for the rendering ui */
static void menus_create(ui_rendering_t * ui_rendering) {


  GnomeUIInfo file_menu[] = {
    GNOMEUIINFO_ITEM_DATA(N_("_Export Rendering"),
			  N_("Export the rendered image"),
			  export_cb,
			  ui_rendering, NULL),
#ifdef AMIDE_LIBFAME_SUPPORT
    GNOMEUIINFO_ITEM_DATA(N_("_Create Movie"),
			  N_("Create a movie out of a sequence of renderings"),
			  movie_cb,
			  ui_rendering, NULL),
#endif
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_MENU_CLOSE_ITEM(close_cb, ui_rendering),
    GNOMEUIINFO_END
  };

  GnomeUIInfo edit_menu[] = {
    GNOMEUIINFO_ITEM_DATA(N_("_Rendering Parameters"),
			  N_("Adjust parameters pertinent to the rendered image"),
			  parameters_cb,
			  ui_rendering, NULL),
    GNOMEUIINFO_END
  };

  /* and the main menu definition */
  GnomeUIInfo ui_rendering_main_menu[] = {
    GNOMEUIINFO_MENU_FILE_TREE(file_menu),
    GNOMEUIINFO_MENU_EDIT_TREE(edit_menu),
    GNOMEUIINFO_MENU_HELP_TREE(ui_common_help_menu),
    GNOMEUIINFO_END
  };


  /* sanity check */
  g_assert(ui_rendering !=NULL);

  /* make the menu */
  gnome_app_create_menus(GNOME_APP(ui_rendering->app), ui_rendering_main_menu);

  return;

}




/* destroy a ui_rendering data structure */
static ui_rendering_t * ui_rendering_free(ui_rendering_t * ui_rendering) {

  if (ui_rendering == NULL)
    return ui_rendering;

  /* sanity checks */
  g_return_val_if_fail(ui_rendering->reference_count > 0, NULL);

  ui_rendering->reference_count--;

  /* things to do if we've removed all reference's */
  if (ui_rendering->reference_count == 0) {
    ui_rendering->contexts = renderings_unref(ui_rendering->contexts);
#ifdef AMIDE_DEBUG
    g_print("freeing ui_rendering\n");
#endif

    if (ui_rendering->pixbuf != NULL) {
      g_object_unref(ui_rendering->pixbuf);
      ui_rendering->pixbuf = NULL;
    }

    if (ui_rendering->box_space != NULL) {
      g_object_unref(ui_rendering->box_space);
      ui_rendering->box_space = NULL;
    }

    g_free(ui_rendering);
    ui_rendering = NULL;
  }

  return ui_rendering;

}





/* allocate and initialize a ui_rendering data structure */
static ui_rendering_t * ui_rendering_init(GList * objects, 
					  amide_time_t start, 
					  amide_time_t duration) {

  ui_rendering_t * ui_rendering;

  /* alloc space for the data structure for passing ui info */
  if ((ui_rendering = g_new(ui_rendering_t,1)) == NULL) {
    g_warning("couldn't allocate space for ui_rendering_t");
    return NULL;
  }
  ui_rendering->reference_count = 1;

  /* set any needed parameters */
  ui_rendering->app = NULL;
  ui_rendering->parameter_dialog = NULL;
#ifdef AMIDE_LIBFAME_SUPPORT
  ui_rendering->movie = NULL;
#endif
  ui_rendering->render_button = NULL;
  ui_rendering->immediate = TRUE;
  ui_rendering->stereoscopic = FALSE;
  ui_rendering->contexts = NULL;
  ui_rendering->pixbuf = NULL;
  ui_rendering->canvas_image = NULL;
  ui_rendering->quality = RENDERING_DEFAULT_QUALITY;
  ui_rendering->depth_cueing = RENDERING_DEFAULT_DEPTH_CUEING;
  ui_rendering->front_factor = RENDERING_DEFAULT_FRONT_FACTOR;
  ui_rendering->density = RENDERING_DEFAULT_DENSITY;
  ui_rendering->zoom = RENDERING_DEFAULT_ZOOM;
  ui_rendering->start = start;
  ui_rendering->duration = duration;
  ui_rendering->box_space = amitk_space_new();

  /* load in saved preferences */
  gnome_config_push_prefix("/"PACKAGE"/");

  ui_rendering->stereo_eye_width = gnome_config_get_int("RENDERING/EyeWidth");
  if (ui_rendering->stereo_eye_width == 0)  /* if no config file, put in sane value */
    ui_rendering->stereo_eye_width = 50*gdk_screen_width()/gdk_screen_width_mm(); /* in pixels */

  ui_rendering->stereo_eye_angle = gnome_config_get_float("RENDERING/EyeAngle");
  if ((ui_rendering->stereo_eye_angle <= 0.1) || (ui_rendering->stereo_eye_angle > 45.0))
    ui_rendering->stereo_eye_angle = 2.0; /* degrees */

  gnome_config_pop_prefix();

  /* initialize the rendering contexts */
  ui_rendering->contexts = renderings_init(objects, start, duration);

  return ui_rendering;
}



void ui_rendering_update_canvas(ui_rendering_t * ui_rendering, gboolean override) {

  if (ui_rendering->immediate || override) {
    ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_rendering->canvas));
    update_canvas(ui_rendering); 
    ui_common_remove_cursor(GTK_WIDGET(ui_rendering->canvas));
  }

  return;
}

/* render our objects and place into the canvases */
void update_canvas(ui_rendering_t * ui_rendering) {

  amide_intpoint_t size_dim; 
  AmitkEye eyes;
  gint width, height;

  g_return_if_fail(ui_rendering != NULL);
  g_return_if_fail(ui_rendering->contexts != NULL);

  renderings_reload_objects(ui_rendering->contexts, ui_rendering->start,
			    ui_rendering->duration);

  /* -------- render our objects ------------ */

  if (ui_rendering->stereoscopic) eyes = AMITK_EYE_NUM;
  else eyes = 1;

  if (ui_rendering->pixbuf != NULL) {
    g_object_unref(ui_rendering->pixbuf);
    ui_rendering->pixbuf = NULL;
  }

  /* base the dimensions on the first context in the list.... */
  size_dim = ceil(ui_rendering->zoom*POINT_MAX(ui_rendering->contexts->context->dim));
  ui_rendering->pixbuf = image_from_contexts(ui_rendering->contexts, 
					     size_dim, size_dim, eyes,
					     ui_rendering->stereo_eye_angle, 
					     ui_rendering->stereo_eye_width); 
  
  /* put up the image */
  if (ui_rendering->canvas_image != NULL) 
    gnome_canvas_item_set(ui_rendering->canvas_image,
			  "pixbuf", ui_rendering->pixbuf, NULL);
  else /* time to make a new image */
    ui_rendering->canvas_image = 
      gnome_canvas_item_new(gnome_canvas_root(ui_rendering->canvas),
			    gnome_canvas_pixbuf_get_type(),
			    "pixbuf", ui_rendering->pixbuf,
			    "x", (double) 0.0,
			    "y", (double) 0.0,
			    NULL);

  width = gdk_pixbuf_get_width(ui_rendering->pixbuf);  
  height = gdk_pixbuf_get_height(ui_rendering->pixbuf);  

  /* reset the min size of the widget */
  gnome_canvas_set_scroll_region(ui_rendering->canvas, 0.0, 0.0, width, height);
  gtk_widget_set_size_request(GTK_WIDGET(ui_rendering->canvas), width, height);


  return;
} 



/* function that sets up the rendering dialog */
void ui_rendering_create(GList * objects, amide_time_t start, amide_time_t duration) {
  
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
  ui_rendering_t * ui_rendering;

  /* sanity checks */
  g_return_if_fail(objects != NULL);

  ui_rendering = ui_rendering_init(objects, start, duration);
  ui_rendering->app = GNOME_APP(gnome_app_new(PACKAGE, "Rendering Window"));
  gtk_window_set_resizable(GTK_WINDOW(ui_rendering->app), TRUE);

  /* setup the callbacks for app */
  g_signal_connect(G_OBJECT(ui_rendering->app), "realize", 
		   G_CALLBACK(ui_common_window_realize_cb), NULL);
  g_signal_connect(G_OBJECT(ui_rendering->app), "delete_event",
		   G_CALLBACK(delete_event_cb), ui_rendering);

  /* setup the menus */
  menus_create(ui_rendering);

  /* make the widgets for this dialog box */
  packing_table = gtk_table_new(3,3,FALSE);
  gnome_app_set_contents(ui_rendering->app, packing_table);

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
		    G_CALLBACK(rotate_cb), ui_rendering);


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
		   G_CALLBACK(reset_axis_pressed_cb), ui_rendering);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);



  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), hseparator, FALSE, FALSE, 0);

  /* the Render button, after changing parameters, this is the button you'll
   * wanna hit to rerender the object, unless you have immediate rendering set */
  ui_rendering->render_button = gtk_button_new_with_label("Render");
  gtk_box_pack_start(GTK_BOX(vbox), ui_rendering->render_button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive(ui_rendering->render_button, !(ui_rendering->immediate));
  g_signal_connect(G_OBJECT(ui_rendering->render_button), "clicked", 
		   G_CALLBACK(render_cb), ui_rendering);


  /* the render immediately button.. */
  check_button = gtk_check_button_new_with_label ("on change");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), ui_rendering->immediate);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(immediate_cb), ui_rendering);
  gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

  /* the render immediately button.. */
  check_button = gtk_check_button_new_with_label ("stereoscopic");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), ui_rendering->stereoscopic);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(stereoscopic_cb), ui_rendering);
  gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

  /* setup the main canvas */
  ui_rendering->canvas = GNOME_CANVAS(gnome_canvas_new_aa());
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(ui_rendering->canvas), 1,2,1,2,
		   X_PACKING_OPTIONS | GTK_FILL,
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);
  g_signal_connect(G_OBJECT(ui_rendering->canvas), "event",
		   G_CALLBACK(canvas_event_cb), ui_rendering);
  update_canvas(ui_rendering); /* fill in the canvas */

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
		    G_CALLBACK(rotate_cb), ui_rendering);
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
		   G_CALLBACK(rotate_cb), ui_rendering);

  /* and show all our widgets */
  gtk_widget_show_all(GTK_WIDGET(ui_rendering->app));
  amide_register_window((gpointer) ui_rendering->app);

  return;
}



#endif




















