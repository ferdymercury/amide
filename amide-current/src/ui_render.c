/* ui_render.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2014 Andy Loening
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

#ifdef AMIDE_LIBVOLPACK_SUPPORT

#include <libgnomecanvas/libgnomecanvas.h>

#include "amide_gconf.h"
#include "image.h"
#include "ui_common.h"
#include "ui_render.h"
#include "ui_render_dialog.h"
#include "ui_render_movie.h"
#include "amitk_dial.h"
#include "amitk_progress_dialog.h"
#include "amitk_tree_view.h"
#include "amitk_common.h"


#define UPDATE_NONE 0
#define UPDATE_RENDERING 0x1


static gboolean canvas_event_cb(GtkWidget* widget,  GdkEvent * event, gpointer data);
static void stereoscopic_cb(GtkRadioAction * action, GtkRadioAction * current, gpointer data);
static void change_zoom_cb(GtkWidget * widget, gpointer data);
static void rotate_cb(GtkAdjustment * adjustment, gpointer data);
static void reset_axis_pressed_cb(GtkWidget * widget, gpointer data);
static void export_cb(GtkAction * action, gpointer data);
static void parameters_cb(GtkAction * action, gpointer data);
static void transfer_function_cb(GtkAction * action, gpointer data);
#if (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)
static void movie_cb(GtkAction * action, gpointer data);
#endif
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data);
static void close_cb(GtkAction * action, gpointer data);


static void read_render_preferences(gboolean * strip_highs, gboolean * optimize_renderings,
				    gboolean * initially_no_gradient_opacity);
static ui_render_t * ui_render_init(GtkWindow * window, GtkWidget *window_vbox, AmitkStudy * study, GList * selected_objects, AmitkPreferences * preferences);
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
  static AmitkPoint prev_theta;
  static AmitkPoint theta;
  static AmitkCanvasPoint initial_cpoint, center_cpoint;
  static gboolean dragging = FALSE;
  static GnomeCanvasItem * rotation_box[8];
  static AmitkPoint box_point[8];

  canvas = GNOME_CANVAS(widget);

  /* get the location of the event, and convert it to the canvas coordinates */
  gnome_canvas_window_to_world(canvas, event->button.x, event->button.y, &canvas_cpoint.x, &canvas_cpoint.y);
  gnome_canvas_w2c_d(canvas, canvas_cpoint.x, canvas_cpoint.y, &canvas_cpoint.x, &canvas_cpoint.y);
  dim = MIN(ui_render->pixbuf_width, ui_render->pixbuf_height);
  temp_point.x = temp_point.y = temp_point.z = -dim/2.0;
  amitk_space_set_offset(ui_render->box_space, temp_point);
  //  amitk_space_set_axes(ui_render->box_space, base_axes, AMITK_SPACE_OFFSET(ui_render->box_space));
  amitk_space_set_axes(ui_render->box_space, base_axes, AMITK_SPACE_OFFSET(ui_render->box_space));


  /* switch on the event which called this */
  switch (event->type) {


    case GDK_ENTER_NOTIFY:
      ui_common_place_cursor(UI_CURSOR_DATA_SET_MODE, GTK_WIDGET(canvas));
      break;


     case GDK_LEAVE_NOTIFY:
       ui_common_place_cursor(UI_CURSOR_DEFAULT, GTK_WIDGET(canvas));
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
	    gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(ui_render->canvas)), 
				  gnome_canvas_line_get_type(),
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
/* function to switch into stereoscopic rendering mode */
static void stereoscopic_cb(GtkRadioAction * action, GtkRadioAction * current, gpointer data) {
  ui_render_t * ui_render = data;
  ui_render->stereoscopic = gtk_radio_action_get_current_value((GTK_RADIO_ACTION(current)));
  ui_render_add_update(ui_render); 
  return;
}

/* function to change the zoom */
static void change_zoom_cb(GtkWidget * widget, gpointer data) {

  ui_render_t * ui_render = data;
  gdouble temp_val;

  temp_val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  
  if (temp_val < 0.1)
    return;
  if (temp_val > 10) /* 10x zoom seems like quite a bit... */
    return;
  
  /* set the zoom */
  if (!REAL_EQUAL(ui_render->zoom, temp_val)) {
    ui_render->zoom = temp_val;
    renderings_set_zoom(ui_render->renderings, ui_render->zoom);
    
    /* do updating */
    ui_render_add_update(ui_render); 
  }

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



/* function to save a rendering as an external data format */
static void export_cb(GtkAction * action, gpointer data) {

  ui_render_t * ui_render = data;
  renderings_t * temp_renderings;
  GtkWidget * file_chooser;
  gchar * data_set_names = NULL;
  gchar * filename;
  static guint save_image_num = 0;
  GdkPixbuf * pixbuf;

  g_return_if_fail(ui_render->pixbuf != NULL);

  file_chooser = gtk_file_chooser_dialog_new(_("Export File"),
					     GTK_WINDOW(ui_render->window), /* parent window */
					     GTK_FILE_CHOOSER_ACTION_SAVE,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					     NULL);
  gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(file_chooser), TRUE);
  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER (file_chooser), TRUE);
  amitk_preferences_set_file_chooser_directory(ui_render->preferences, file_chooser); /* set the default directory if applicable */

  /* take a guess at the filename */
  temp_renderings = ui_render->renderings;
  data_set_names = g_strdup(temp_renderings->rendering->name);
  temp_renderings = temp_renderings->next;
  while (temp_renderings != NULL) {
    filename = g_strdup_printf("%s+%s",data_set_names, temp_renderings->rendering->name);
    g_free(data_set_names);
    data_set_names = filename;
    temp_renderings = temp_renderings->next;
  }
  filename = g_strdup_printf("Rendering%s{%s}_%d.jpg", 
			     ui_render->stereoscopic ? "_stereo_" : "_", 
			     data_set_names,save_image_num++);
  gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER (file_chooser), filename);
  g_free(data_set_names);
  g_free(filename); 

  /* run the dialog */
  if (gtk_dialog_run(GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT) 
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (file_chooser));
  else
    filename = NULL;
  gtk_widget_destroy (file_chooser);

  /* save out the image if we have a filename */
  if (filename != NULL) {
    pixbuf = ui_render_get_pixbuf(ui_render);
    if (pixbuf != NULL) {
      if (gdk_pixbuf_save (pixbuf, filename, "jpeg", NULL, "quality", "100", NULL) == FALSE) 
	g_warning(_("Failure Saving File: %s"), filename);
      g_object_unref(pixbuf);
    } else {
      g_warning(_("Canvas failed to return a valid image\n"));
    }
    g_free(filename);
  }

  return;
}



/* function called when the button to pop up a rendering parameters modification dialog is hit */
static void parameters_cb(GtkAction * action, gpointer data) {
  ui_render_t * ui_render = data;
  ui_render_dialog_create_parameters(ui_render);
  return;
}

/* function called when the button to pop up a transfer function dialog is hit */
static void transfer_function_cb(GtkAction * action, gpointer data) {
  ui_render_t * ui_render = data;
  ui_render_dialog_create_transfer_function(ui_render);
  return;
}


#if (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)
/* function called when the button to pop up a movie generation dialog */
static void movie_cb(GtkAction * action, gpointer data) {
  ui_render_t * ui_render = data;
  ui_render->movie = ui_render_movie_dialog_create(ui_render);
  return;
}
#endif

/* function to run for a delete_event */
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_render_t * ui_render = data;
  GtkWindow * window = ui_render->window;

  ui_render = ui_render_free(ui_render); /* free the associated data structure */
  amide_unregister_window((gpointer) window); /* tell amide this window is no longer */

  return FALSE;
}

/* function ran when closing the rendering window */
static void close_cb(GtkAction* widget, gpointer data) {

  ui_render_t * ui_render = data;
  GtkWindow * window = ui_render->window;
  gboolean return_val;

  /* run the delete event function */
  g_signal_emit_by_name(G_OBJECT(window), "delete_event", NULL, &return_val);
  if (!return_val) gtk_widget_destroy(GTK_WIDGET(window));

  return;
}



static const GtkActionEntry normal_items[] = {
  /* Toplevel */
  { "FileMenu", NULL, N_("_File") },
  { "EditMenu", NULL, N_("_Edit") },
  { "HelpMenu", NULL, N_("_Help") },
  
  /* File menu */
  { "ExportRendering", NULL, N_("_Export Rendering"), NULL, N_("Export the rendered image"), G_CALLBACK(export_cb)},
#if (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)
  { "CreateMovie", NULL, N_("_Create Movie"), NULL, N_("Create a movie out of a sequence of renderings"), G_CALLBACK(movie_cb)},
#endif
  { "Close", GTK_STOCK_CLOSE, NULL, "<control>W", N_("Close the rendering dialog"), G_CALLBACK (close_cb)},
  
  /* Edit menu */
  { "Parameters", NULL, N_("_Rendering Parameters"), NULL, N_("Adjust parameters pertinent to the rendered image"), G_CALLBACK(parameters_cb)},
  
  /* Toolbar items */
  { "TransferFunctions", "amide_icon_transfer_function", N_("X-fer"), NULL, N_("Opacity and density transfer functions"), G_CALLBACK(transfer_function_cb)}
};

static const GtkRadioActionEntry stereoscopic_radio_entries[] = {
  { "MonoscopicRendering", "amide_icon_view_mode_single", N_("Mono"), NULL,  N_("Monoscopic rendering"), 0},
  { "StereoscopicRendering", "amide_icon_view_mode_linked_2way", N_("Stereo"), NULL,  N_("Stereoscopic rendering"), 1},
};

static const char *ui_description =
"<ui>"
"  <menubar name='MainMenu'>"
"    <menu action='FileMenu'>"
"      <menuitem action='ExportRendering'/>"
"      <menuitem action='CreateMovie'/>"
"      <separator/>"
"      <menuitem action='Close'/>"
"    </menu>"
"    <menu action='EditMenu'>"
"      <menuitem action='Parameters'/>"
"    </menu>"
HELP_MENU_UI_DESCRIPTION
"  </menubar>"
"  <toolbar name='ToolBar'>"
"    <toolitem action='TransferFunctions'/>"
"    <separator/>"
"    <toolitem action='MonoscopicRendering'/>"
"    <toolitem action='StereoscopicRendering'/>"
  /* "    <separator/>" */
  /* "    <placeholder name='Zoom' />" */
"  </toolbar>"
"</ui>";

/* function to setup the menus and toolbars for the rendering ui */
static void menus_toolbar_create(ui_render_t * ui_render) {

  GtkWidget *menubar;
  GtkWidget *toolbar;
  GtkActionGroup *action_group;
  GtkUIManager *ui_manager;
  GtkAccelGroup *accel_group;
  GtkWidget * label;
  GtkWidget * spin_button;
  GError * error;

  /* sanity check */
  g_assert(ui_render !=NULL);

  /* create an action group with all the menu actions */
  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions(action_group, normal_items, G_N_ELEMENTS(normal_items),ui_render);
  gtk_action_group_add_actions(action_group, ui_common_help_menu_items, G_N_ELEMENTS(ui_common_help_menu_items),ui_render);
  gtk_action_group_add_radio_actions(action_group, stereoscopic_radio_entries, G_N_ELEMENTS (stereoscopic_radio_entries), 
				     0, G_CALLBACK(stereoscopic_cb), ui_render);

  /* create the ui manager, and add the actions and accel's */
  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (ui_render->window, accel_group);

  /* create the actual menu/toolbar ui */
  error = NULL;
  if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, &error)) {
    g_warning ("%s: building menus failed in %s: %s", PACKAGE, __FILE__, error->message);
    g_error_free (error);
    return;
  }

  /* pack in the menu and toolbar */
  menubar = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
  gtk_box_pack_start (GTK_BOX (ui_render->window_vbox), menubar, FALSE, FALSE, 0);

  toolbar = gtk_ui_manager_get_widget (ui_manager, "/ToolBar");
  gtk_box_pack_start (GTK_BOX (ui_render->window_vbox), toolbar, FALSE, FALSE, 0);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

  /* a separator for clarity */
  ui_common_toolbar_append_separator(toolbar);

  /* add the zoom widget to our toolbar */
  label = gtk_label_new(_("zoom:"));
  ui_common_toolbar_append_widget(toolbar, label, NULL);

  spin_button = gtk_spin_button_new_with_range(0.1, 10.0, 0.2);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_button),FALSE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_button), FALSE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_button), 2);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), ui_render->zoom);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin_button), GTK_UPDATE_ALWAYS);
  gtk_widget_set_size_request (spin_button, 60, -1);
  g_signal_connect(G_OBJECT(spin_button), "value_changed", G_CALLBACK(change_zoom_cb), ui_render);
  g_signal_connect(G_OBJECT(spin_button), "button_press_event",
		   G_CALLBACK(amitk_spin_button_discard_double_or_triple_click), NULL);
  ui_common_toolbar_append_widget(toolbar, spin_button,_("specify how much to magnify the rendering"));
			      
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

    if (ui_render->preferences != NULL) {
      g_object_unref(ui_render->preferences);
      ui_render->preferences = NULL;
    }

    if (ui_render->idle_handler_id != 0) {
      g_source_remove(ui_render->idle_handler_id);
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



static void read_render_preferences(gboolean * strip_highs, gboolean * optimize_renderings,
				    gboolean * initially_no_gradient_opacity) {

  *strip_highs = 
    amide_gconf_get_bool(GCONF_AMIDE_RENDERING,"StripHighs");
  *optimize_renderings = 
    amide_gconf_get_bool(GCONF_AMIDE_RENDERING,"OptimizeRendering");
  *initially_no_gradient_opacity = 
    amide_gconf_get_bool(GCONF_AMIDE_RENDERING,"InitiallyNoGradientOpacity");

  return;
}


/* allocate and initialize a ui_render data structure */
static ui_render_t * ui_render_init(GtkWindow * window,
				    GtkWidget * window_vbox,
				    AmitkStudy * study,
				    GList * selected_objects,
				    AmitkPreferences * preferences) {

  ui_render_t * ui_render;
  gboolean strip_highs;
  gboolean optimize_rendering;
  gboolean initially_no_gradient_opacity;

  read_render_preferences(&strip_highs, &optimize_rendering, &initially_no_gradient_opacity);

  /* alloc space for the data structure for passing ui info */
  if ((ui_render = g_try_new(ui_render_t,1)) == NULL) {
    g_warning(_("couldn't allocate memory space for ui_render_t"));
    return NULL;
  }
  ui_render->reference_count = 1;

  /* set any needed parameters */
  ui_render->window = window;
  ui_render->window_vbox = window_vbox;
  ui_render->parameter_dialog = NULL;
  ui_render->transfer_function_dialog = NULL;
  ui_render->preferences = g_object_ref(preferences);
#if (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)
  ui_render->movie = NULL;
#endif
  ui_render->stereoscopic = FALSE;
  ui_render->renderings = NULL;
  ui_render->pixbuf = NULL;
  ui_render->pixbuf_width = 128;
  ui_render->pixbuf_height = 128;
  ui_render->canvas_image = NULL;
  ui_render->canvas_time_label = NULL;
  ui_render->time_label_on = FALSE;
  ui_render->quality = RENDERING_DEFAULT_QUALITY;
  ui_render->depth_cueing = RENDERING_DEFAULT_DEPTH_CUEING;
  ui_render->front_factor = RENDERING_DEFAULT_FRONT_FACTOR;
  ui_render->density = RENDERING_DEFAULT_DENSITY;
  ui_render->zoom = RENDERING_DEFAULT_ZOOM;
  ui_render->start = AMITK_STUDY_VIEW_START_TIME(study);
  ui_render->duration = AMITK_STUDY_VIEW_DURATION(study);
  ui_render->fov = AMITK_STUDY_FOV(study);
  ui_render->view_center = AMITK_STUDY_VIEW_CENTER(study);
  ui_render->box_space = amitk_space_new();
  ui_render->progress_dialog = amitk_progress_dialog_new(ui_render->window);
  ui_render->disable_progress_dialog=FALSE;
  ui_render->next_update= UPDATE_NONE;
  ui_render->idle_handler_id = 0;
  ui_render->rendered_successfully=FALSE;

  /* load in saved render preferences */
  ui_render->update_without_release = 
    amide_gconf_get_bool(GCONF_AMIDE_RENDERING,"UpdateWithoutRelease");

  ui_render->stereo_eye_width = 
    amide_gconf_get_int(GCONF_AMIDE_RENDERING,"EyeWidth");
  if (ui_render->stereo_eye_width == 0)  /* if no config file, put in sane value */
    ui_render->stereo_eye_width = 50*gdk_screen_width()/gdk_screen_width_mm(); /* in pixels */

  ui_render->stereo_eye_angle = 
    amide_gconf_get_float(GCONF_AMIDE_RENDERING,"EyeAngle");
  if ((ui_render->stereo_eye_angle <= 0.1) || (ui_render->stereo_eye_angle > 45.0))
    ui_render->stereo_eye_angle = 5.0; /* degrees */

  /* initialize the rendering contexts */
  ui_render->renderings = renderings_init(selected_objects, 
					  ui_render->start, 
					  ui_render->duration, 
					  strip_highs, optimize_rendering, initially_no_gradient_opacity,
					  ui_render->fov,
					  ui_render->view_center,
					  ui_render->disable_progress_dialog ? NULL : amitk_progress_dialog_update,
					  ui_render->disable_progress_dialog ? NULL : ui_render->progress_dialog);

  return ui_render;
}


GdkPixbuf * ui_render_get_pixbuf(ui_render_t * ui_render) {

  GdkPixbuf * pixbuf;
  pixbuf = amitk_get_pixbuf_from_canvas(GNOME_CANVAS(ui_render->canvas), 0.0,0.0,
					ui_render->pixbuf_width, ui_render->pixbuf_height);
  return pixbuf;
}


void ui_render_add_update(ui_render_t * ui_render) {


  
  ui_render->next_update = ui_render->next_update | UPDATE_RENDERING;
  if (ui_render->idle_handler_id == 0) {
    ui_common_place_cursor_no_wait(UI_CURSOR_WAIT, ui_render->canvas);
    ui_render->idle_handler_id = 
      g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,ui_render_update_immediate, ui_render, NULL);
  }

  return;
}

/* render our objects and place into the canvases */
gboolean ui_render_update_immediate(gpointer data) {

  ui_render_t * ui_render = data;
  amide_intpoint_t size_dim; 
  AmideEye eyes;
  gboolean return_val=TRUE;
  amide_time_t midpt_time;
  gint hours, minutes, seconds;
  gchar * time_str;
  rgba_t color;

  g_return_val_if_fail(ui_render != NULL, FALSE);
  g_return_val_if_fail(ui_render->renderings != NULL, FALSE);

  ui_render->rendered_successfully=FALSE;
  if (!renderings_reload_objects(ui_render->renderings, ui_render->start,
				 ui_render->duration,
				 ui_render->disable_progress_dialog ? NULL : amitk_progress_dialog_update,
				 ui_render->disable_progress_dialog ? NULL : ui_render->progress_dialog )) {
    return_val=FALSE;
    goto function_end;
  }


  /* -------- render our objects ------------ */

  if (ui_render->stereoscopic) eyes = AMIDE_EYE_NUM;
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
      gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(ui_render->canvas)),
			    gnome_canvas_pixbuf_get_type(),
			    "pixbuf", ui_render->pixbuf,
			    "x", (double) 0.0,
			    "y", (double) 0.0,
			    NULL);

  /* get the height and width */
  ui_render->pixbuf_width = gdk_pixbuf_get_width(ui_render->pixbuf);  
  ui_render->pixbuf_height = gdk_pixbuf_get_height(ui_render->pixbuf);  

  /* put up the timer */
  if (ui_render->time_label_on) {
    midpt_time = ui_render->start+ui_render->duration/2.0;
    hours = floor(midpt_time/3600);
    midpt_time -= hours*3600;
    minutes = floor(midpt_time/60);
    midpt_time -= minutes*60;
    seconds = midpt_time;
    time_str = g_strdup_printf("%d:%.2d:%.2d",hours,minutes,seconds);

    color = amitk_color_table_outline_color(ui_render->renderings->rendering->color_table, FALSE);
    if (ui_render->canvas_time_label != NULL) 
      gnome_canvas_item_set(ui_render->canvas_time_label,
			    "text", time_str,
			    "fill_color_rgba", color, NULL);
    else
      ui_render->canvas_time_label = 
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(ui_render->canvas)),
			      gnome_canvas_text_get_type(),
			      "anchor", GTK_ANCHOR_SOUTH_WEST,
			      "text", time_str,
			      "x", 4.0,
			      "y", ui_render->pixbuf_height-2.0,
			      "fill_color_rgba", color,
			      "font_desc", amitk_fixed_font_desc, NULL);
    g_free(time_str);
  } else {
    if (ui_render->canvas_time_label != NULL) {
      gtk_object_destroy(GTK_OBJECT(ui_render->canvas_time_label));
      ui_render->canvas_time_label = NULL;
    }
  }

  /* reset the min size of the widget */
  gnome_canvas_set_scroll_region(GNOME_CANVAS(ui_render->canvas), 0.0, 0.0, ui_render->pixbuf_width, ui_render->pixbuf_height);
  gtk_widget_set_size_request(ui_render->canvas, ui_render->pixbuf_width, ui_render->pixbuf_height);
  ui_render->rendered_successfully = TRUE;
  return_val = FALSE;

 function_end:

  ui_common_remove_wait_cursor(ui_render->canvas);
  ui_render->next_update = UPDATE_NONE;
  ui_render->idle_handler_id=0;

  return return_val;
} 




/* function that sets up the rendering dialog */
void ui_render_create(AmitkStudy * study, GList * selected_objects, AmitkPreferences * preferences) {
  
  GtkWidget * packing_table;
  GtkWidget * button;
  GtkAdjustment * adjustment;
  GtkWidget * scale;
  GtkWidget * vbox;
  GtkWidget * hbox;
  GtkWidget * dial;
  GtkWidget * label;
  ui_render_t * ui_render;
  GtkWindow * window;
  GtkWidget * window_vbox;
  gboolean return_val;

  /* sanity checks */
  g_return_if_fail(AMITK_IS_STUDY(study));

  /* setup the window with a vbox container in it */
  window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
  gtk_window_set_title(window, _("Rendering Window"));
  gtk_window_set_resizable(window, FALSE);
  window_vbox = gtk_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (window), window_vbox);
  ui_render = ui_render_init(window, window_vbox, study, selected_objects, preferences);

  /* check we actually have something */
  if (ui_render->renderings == NULL) {
    /* run the delete event function */
    g_signal_emit_by_name(G_OBJECT(window), "delete_event", NULL, &return_val);
    if (!return_val) gtk_widget_destroy(GTK_WIDGET(window));
    return;
  }
    
  /* setup the callbacks for the window */
  g_signal_connect(G_OBJECT(ui_render->window), "delete_event",
		   G_CALLBACK(delete_event_cb), ui_render);

  /* setup the menus and toolbar */
  menus_toolbar_create(ui_render);

  /* make the widgets for this dialog box */
  packing_table = gtk_table_new(3,3,FALSE);
  gtk_box_pack_start (GTK_BOX (ui_render->window_vbox), packing_table, TRUE,TRUE, 0);

  /* setup the main canvas */
#ifdef AMIDE_LIBGNOMECANVAS_AA
  ui_render->canvas = gnome_canvas_new_aa();
#else
  ui_render->canvas = gnome_canvas_new();
#endif
  gtk_table_attach(GTK_TABLE(packing_table), 
		   ui_render->canvas, 1,2,1,2,
		   X_PACKING_OPTIONS | GTK_FILL,
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);
  g_signal_connect(G_OBJECT(ui_render->canvas), "event",
		   G_CALLBACK(canvas_event_cb), ui_render);
  ui_render_update_immediate(ui_render); /* fill in the canvas */

  /* create the x, y, and z rotation dials */
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
  label = gtk_label_new(_("y"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

  /* create the z dial */
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_table_attach(GTK_TABLE(packing_table), hbox, 3,4,0,1,
		   X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL, 
		   X_PADDING, Y_PADDING);
  label = gtk_label_new(_("z"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
  adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, -180.0, 180.0, 1.0, 1.0, 1.0));

  dial = amitk_dial_new(adjustment);
  gtk_widget_set_size_request(dial,50,50);
  amitk_dial_set_update_policy (AMITK_DIAL(dial), GTK_UPDATE_DISCONTINUOUS);
  g_object_set_data(G_OBJECT(adjustment), "axis", GINT_TO_POINTER(AMITK_AXIS_Z));
  gtk_box_pack_start(GTK_BOX(hbox), dial, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT(adjustment), "value_changed", 
		    G_CALLBACK(rotate_cb), ui_render);


  /* the x slider */
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_table_attach(GTK_TABLE(packing_table), vbox,  3,4,1,2,
		   X_PACKING_OPTIONS , Y_PACKING_OPTIONS | GTK_FILL, 
		   X_PADDING, Y_PADDING);
  label = gtk_label_new(_("x"));
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
  adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, -90.0, 90.0, 1.0, 1.0, 1.0));
  scale = gtk_vscale_new(adjustment);
  gtk_range_set_update_policy (GTK_RANGE(scale), GTK_UPDATE_DISCONTINUOUS);
  g_object_set_data(G_OBJECT(adjustment), "axis", GINT_TO_POINTER(AMITK_AXIS_X));
  gtk_box_pack_start(GTK_BOX(vbox), scale, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(adjustment), "value_changed", 
		   G_CALLBACK(rotate_cb), ui_render);

  /* button to reset the axis */
  button = gtk_button_new_with_label(_("Reset Axis"));
  g_signal_connect(G_OBJECT(button), "pressed",
		   G_CALLBACK(reset_axis_pressed_cb), ui_render);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

  /* and show all our widgets */
  gtk_widget_show_all(GTK_WIDGET(ui_render->window));
  amide_register_window((gpointer) ui_render->window);

  return;
}




static void init_strip_highs_cb(GtkWidget * widget, gpointer data);
static void init_optimize_rendering_cb(GtkWidget * widget, gpointer data);
static void init_no_gradient_opacity_cb(GtkWidget * widget, gpointer data);



static void init_strip_highs_cb(GtkWidget * widget, gpointer data) {
  amide_gconf_set_bool(GCONF_AMIDE_RENDERING,"StripHighs", 
		       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
  return;
}

static void init_optimize_rendering_cb(GtkWidget * widget, gpointer data) {
  amide_gconf_set_bool(GCONF_AMIDE_RENDERING,"OptimizeRendering", 
		       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
  return;
}

static void init_no_gradient_opacity_cb(GtkWidget * widget, gpointer data) {
  amide_gconf_set_bool(GCONF_AMIDE_RENDERING,"InitiallyNoGradientOpacity", 
		       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
  return;
}


/* function to setup a dialog to allow us to choose options for rendering */
GtkWidget * ui_render_init_dialog_create(AmitkStudy * study, GtkWindow * parent) {
  
  GtkWidget * dialog;
  gchar * temp_string;
  GtkWidget * table;
  GtkWidget * check_button;
  guint table_row;
  GtkWidget * tree_view;
  GtkWidget * scrolled;
  gboolean strip_highs;
  gboolean optimize_rendering;
  gboolean initially_no_gradient_opacity;

  read_render_preferences(&strip_highs, &optimize_rendering, &initially_no_gradient_opacity);

  temp_string = g_strdup_printf(_("%s: Rendering Initialization Dialog"), PACKAGE);
  dialog = gtk_dialog_new_with_buttons (temp_string,  parent,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CLOSE, 
					GTK_STOCK_EXECUTE, AMITK_RESPONSE_EXECUTE,
					NULL);
  gtk_window_set_title(GTK_WINDOW(dialog), temp_string);
  g_free(temp_string);

  /* setup the callbacks for the dialog */
  g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(ui_common_init_dialog_response_cb), NULL);

  gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);

  /* start making the widgets for this dialog box */
  table = gtk_table_new(5,2,FALSE);
  table_row=0;
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);

  tree_view = amitk_tree_view_new(AMITK_TREE_VIEW_MODE_MULTIPLE_SELECTION,NULL, NULL);
  g_object_set_data(G_OBJECT(dialog), "tree_view", tree_view);
  amitk_tree_view_set_study(AMITK_TREE_VIEW(tree_view), study);
  amitk_tree_view_expand_object(AMITK_TREE_VIEW(tree_view), AMITK_OBJECT(study));

  /* make a scrolled area for the tree */
  scrolled = gtk_scrolled_window_new(NULL,NULL);  
  gtk_widget_set_size_request(scrolled,250,250);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), 
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), 
					tree_view);
  gtk_table_attach(GTK_TABLE(table), scrolled, 0,2,
		   table_row, table_row+1,GTK_FILL, GTK_FILL | GTK_EXPAND, X_PADDING, Y_PADDING);
  table_row++;

  /* do we want to strip values */
  check_button = gtk_check_button_new_with_label(_("Set values greater than max. threshold to zero?"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), strip_highs);
  gtk_table_attach(GTK_TABLE(table), check_button, 
		   0,2, table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(init_strip_highs_cb), dialog);
  table_row++;

  /* do we want to converse memory */
  check_button = gtk_check_button_new_with_label(_("Accelerate Rendering?  Increases memory use ~3x"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),optimize_rendering);
  gtk_table_attach(GTK_TABLE(table), check_button, 
		   0,2, table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(init_optimize_rendering_cb), dialog);
  table_row++;

  /* do we want the initial opacities to be only density dependent */
  check_button = gtk_check_button_new_with_label(_("Initial opacity functions only density dependent?"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),initially_no_gradient_opacity);
  gtk_table_attach(GTK_TABLE(table), check_button, 
		   0,2, table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(init_no_gradient_opacity_cb), dialog);
  table_row++;


  /* and show all our widgets */
  gtk_widget_show_all(dialog);

  return dialog;
}








#endif




















