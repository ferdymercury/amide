/* ui_render.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2017 Andy Loening
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

#include "amitk_canvas_compat.h"

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


static gboolean canvas_event_cb(AmitkCanvasItem * item, AmitkCanvasItem * target,  GdkEvent * event, gpointer data);
static void stereoscopic_cb(GSimpleAction * action, GVariant * state, gpointer data);
static void change_zoom_cb(GtkWidget * widget, gpointer data);
static void rotate_cb(GtkAdjustment * adjustment, gpointer data);
static gboolean button_release_cb(GtkWidget * widget, GdkEvent * event, gpointer data);
static void reset_axis_pressed_cb(GtkWidget * widget, gpointer data);
static void export_cb(GSimpleAction * action, GVariant * param, gpointer data);
static void parameters_cb(GSimpleAction * action, GVariant * param, gpointer data);
static void transfer_function_cb(GSimpleAction * action, GVariant * param, gpointer data);
#if (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)
static void movie_cb(GSimpleAction * action, GVariant * param, gpointer data);
#endif
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data);
static void close_cb(GSimpleAction * action, GVariant * param, gpointer data);


static void read_render_preferences(gboolean * strip_highs, gboolean * optimize_renderings,
				    gboolean * initially_no_gradient_opacity);
static ui_render_t * ui_render_init(GtkWindow * window, GtkWidget *window_vbox, AmitkStudy * study, GList * selected_objects, AmitkPreferences * preferences);
static ui_render_t * ui_render_free(ui_render_t * ui_render);


/* function called when the canvas is hit */

/* function called when an event occurs on the image canvas, 
   notes:
   -events for non-new roi's are handled by ui_study_rois_cb_roi_event 

*/
static gboolean canvas_event_cb(AmitkCanvasItem * item, AmitkCanvasItem * target, GdkEvent * event, gpointer data) {

  ui_render_t * ui_render = data;
  AmitkSimpleCanvas * canvas;
  AmitkCanvasItem * root;
  AmitkPoint temp_point;
  AmitkCanvasPoint temp_cpoint1, temp_cpoint2;
  AmitkCanvasPoint canvas_cpoint, diff_cpoint;
  AmitkCanvasPoints * line_points;
  guint i, j,k;
  rgba_t color;
  gdouble dim;
  static AmitkPoint prev_theta;
  static AmitkPoint theta;
  static AmitkCanvasPoint initial_cpoint, center_cpoint;
  static gboolean dragging = FALSE;
  static AmitkCanvasItem * rotation_box[8];
  static AmitkPoint box_point[8];

  canvas = amitk_canvas_item_get_canvas(item);
  root = amitk_simple_canvas_get_root_item(canvas);

  /* get the location of the event, and convert it to the canvas coordinates */
  canvas_cpoint.x = event->button.x_root;
  canvas_cpoint.y = event->button.y_root;
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
	  line_points = amitk_canvas_points_new(2);
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
            amitk_canvas_polyline_new(root, FALSE, 0, "points", line_points,
                                    "stroke-color-rgba",
                                    amitk_color_table_rgba_to_uint32(color),
                                    "line-width", 1.0, NULL);
	  amitk_canvas_points_unref(line_points);
	}
        g_signal_connect(rotation_box[0], "button-release-event",
                         G_CALLBACK(canvas_event_cb), ui_render);
        g_signal_connect(rotation_box[0], "motion-notify-event",
                         G_CALLBACK(canvas_event_cb), ui_render);

	/* translate the 8 vertices back to the base frame */
	for (i=0; i<8; i++)
	  box_point[i] = amitk_space_s2b(ui_render->box_space, box_point[i]);

	if (event->button.button == 1)
	  amitk_simple_canvas_pointer_grab(canvas, rotation_box[0],
				 GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
				 ui_common_cursor[UI_CURSOR_RENDERING_ROTATE_XY],
				 event->button.time);
	else /* button 2 */
	  amitk_simple_canvas_pointer_grab(canvas, rotation_box[0],
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
	  line_points = amitk_canvas_points_new(2);
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
	  g_object_set(rotation_box[i],  "points", line_points, NULL);
	  amitk_canvas_points_unref(line_points);
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
	amitk_simple_canvas_pointer_ungrab(canvas, rotation_box[0], event->button.time);
	dragging = FALSE;

	/* get rid of the frame */
	for (i=0; i<8; i++)
	  amitk_canvas_item_remove(rotation_box[i]);

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
static void stereoscopic_cb(GSimpleAction * action, GVariant * state, gpointer data) {
  ui_render_t * ui_render = data;
  ui_render->stereoscopic = g_variant_get_int32(state);
  ui_render_add_update(ui_render); 
  g_simple_action_set_state(action, state);
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

/* Compensate the lack of GTK_UPDATE_DISCONTINUOUS in GTK 3 -- the
   sliders are practically unusable without it.  */
static gboolean button_release_cb(GtkWidget * widget, GdkEvent * event, gpointer data) {

  ui_render_t * ui_render = data;
  GtkAdjustment * adjustment;
  AmitkAxis i_axis;
  gdouble val;

  if (AMITK_IS_DIAL(widget))
    adjustment = amitk_dial_get_adjustment(AMITK_DIAL(widget));
  else
    adjustment = gtk_range_get_adjustment(GTK_RANGE(widget));

  i_axis = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(adjustment), "axis"));
  val = gtk_adjustment_get_value(adjustment);

  if (i_axis == AMITK_AXIS_X) {
    if (val != ui_render->x_old) {
      ui_render->x_old = val;
      rotate_cb(adjustment, ui_render);
    } else {
      gtk_adjustment_set_value(adjustment, 0.0);
    }
  } else if (i_axis == AMITK_AXIS_Y) {
    if (val != ui_render->y_old) {
      ui_render->y_old = val;
      rotate_cb(adjustment, ui_render);
    } else {
      gtk_adjustment_set_value(adjustment, 0.0);
    }
  } else {
    if (val != ui_render->z_old) {
      ui_render->z_old = val;
      rotate_cb(adjustment, ui_render);
    } else {
      gtk_adjustment_set_value(adjustment, 0.0);
    }
  }

  return FALSE;
}


/* function called when one of the rotate widgets has been hit */
static void rotate_cb(GtkAdjustment * adjustment, gpointer data) {

  ui_render_t * ui_render = data;
  AmitkAxis i_axis;
  gdouble rot;

  /* figure out which rotate widget called me */
  i_axis = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(adjustment),"axis"));
  rot = (gtk_adjustment_get_value(adjustment)/180.0)*M_PI; /* get rotation in radians */

  /* update the rotation values */
  renderings_set_rotation(ui_render->renderings, i_axis, rot);

  /* render now if appropriate*/
  ui_render_add_update(ui_render); 

  /* return adjustment back to normal */
  gtk_adjustment_set_value(adjustment, 0.0);

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
static void export_cb(GSimpleAction * action, GVariant * param, gpointer data) {

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
					     _("_Cancel"), GTK_RESPONSE_CANCEL,
					     _("_Save"), GTK_RESPONSE_ACCEPT,
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
static void parameters_cb(GSimpleAction * action, GVariant * param, gpointer data) {
  ui_render_t * ui_render = data;
  ui_render_dialog_create_parameters(ui_render);
  return;
}

/* function called when the button to pop up a transfer function dialog is hit */
static void transfer_function_cb(GSimpleAction * action, GVariant * param, gpointer data) {
  ui_render_t * ui_render = data;
  ui_render_dialog_create_transfer_function(ui_render);
  return;
}


#if (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)
/* function called when the button to pop up a movie generation dialog */
static void movie_cb(GSimpleAction * action, GVariant * param, gpointer data) {
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
static void close_cb(GSimpleAction * action, GVariant * param, gpointer data) {

  ui_render_t * ui_render = data;
  GtkWindow * window = ui_render->window;
  gboolean return_val;

  /* run the delete event function */
  g_signal_emit_by_name(G_OBJECT(window), "delete_event", NULL, &return_val);
  if (!return_val) gtk_widget_destroy(GTK_WIDGET(window));

  return;
}



static const GActionEntry normal_items[] = {
  { "export", export_cb },
#if (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)
  { "movie", movie_cb },
#endif
  { "close", close_cb },
  { "parameters", parameters_cb },
  { "transfer", transfer_function_cb }
};

static const GActionEntry stateful_entries[] = {
  { "rendering", NULL, "i", "@i 0", stereoscopic_cb }
};

/* function to setup the menus and toolbars for the rendering ui */
static void menus_toolbar_create(ui_render_t * ui_render) {

  GtkWidget *menubar;
  GtkWidget *toolbar;
  GMenuModel *model;
  GMenu *menu;
  GSimpleActionGroup *action_group;
  GtkBuilder *builder;
  GtkApplication * app;
  GtkWidget * label;
  GtkWidget * spin_button;
  GtkWidget * icon;
  GtkToolItem * tool_item;
  const gchar *close[] = { "<Control>w", NULL };
  const gchar *help[] = { "F1", NULL };

  /* sanity check */
  g_assert(ui_render !=NULL);

  /* create an action group with all the menu actions */
  app = gtk_window_get_application(ui_render->window);
  action_group = g_simple_action_group_new();
  g_action_map_add_action_entries(G_ACTION_MAP(action_group),
                                  normal_items, G_N_ELEMENTS(normal_items),
                                  ui_render);
  g_action_map_add_action_entries(G_ACTION_MAP(app), ui_common_help_menu_items,
                                  G_N_ELEMENTS(ui_common_help_menu_items),
                                  ui_render);
  g_action_map_add_action_entries(G_ACTION_MAP(action_group),
                                  stateful_entries,
                                  G_N_ELEMENTS(stateful_entries), ui_render);
  gtk_widget_insert_action_group(ui_render->window_vbox, "render",
                                 G_ACTION_GROUP(action_group));

  gtk_application_set_accels_for_action(app, "render.close", close);
  gtk_application_set_accels_for_action(app, "app.manual", help);

  /* create the actual menu/toolbar ui */
  builder = gtk_builder_new_from_resource("/com/github/ferdymercury/amide/render.ui");
  gtk_builder_set_translation_domain(builder, GETTEXT_PACKAGE);

  /* pack in the menu and toolbar */
  model = G_MENU_MODEL (gtk_builder_get_object (builder, "menubar"));
  menu = gtk_application_get_menu_by_id (app, "help");
  g_menu_append_submenu (G_MENU (model), _("_Help"), G_MENU_MODEL (menu));
#if !(AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)
  /* Remove "Create Movie" menu item.  */
  menu = G_MENU (gtk_builder_get_object (builder, "file"));
  g_menu_remove (menu, 1);
#endif
  menubar = gtk_menu_bar_new_from_model (model);
  g_object_unref (builder);
  gtk_box_pack_start (GTK_BOX (ui_render->window_vbox), menubar, FALSE, FALSE, 0);

  toolbar = gtk_toolbar_new ();
  gtk_box_pack_start (GTK_BOX (ui_render->window_vbox), toolbar, FALSE, FALSE, 0);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);

  icon = gtk_image_new_from_icon_name("amide_icon_transfer_function",
                                      GTK_ICON_SIZE_LARGE_TOOLBAR);
  tool_item = gtk_tool_button_new(icon, _("X-fer"));
  gtk_tool_item_set_tooltip_text(tool_item,
                                 _("Opacity and density transfer functions"));
  gtk_actionable_set_action_name(GTK_ACTIONABLE(tool_item), "render.transfer");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);

  ui_common_toolbar_append_separator(toolbar);

  tool_item = gtk_radio_tool_button_new(NULL);
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(tool_item), _("Mono"));
  gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tool_item),
                                 "amide_icon_view_mode_single");
  gtk_tool_item_set_tooltip_text(tool_item, _("Monoscopic rendering"));
  gtk_actionable_set_detailed_action_name(GTK_ACTIONABLE(tool_item),
                                          "render.rendering(0)");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);
  tool_item =
    gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(tool_item));
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(tool_item), _("Stereo"));
  gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tool_item),
                                "amide_icon_view_mode_linked_2way");
  gtk_tool_item_set_tooltip_text(tool_item, _("Stereoscopic rendering"));
  gtk_actionable_set_detailed_action_name(GTK_ACTIONABLE(tool_item),
                                          "render.rendering(1)");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);

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
    amide_gconf_get_bool(GCONF_AMIDE_RENDERING,"strip-highs");
  *optimize_renderings = 
    amide_gconf_get_bool(GCONF_AMIDE_RENDERING,"optimize-rendering");
  *initially_no_gradient_opacity = 
    amide_gconf_get_bool(GCONF_AMIDE_RENDERING,"initially-no-gradient-opacity");

  return;
}


/* allocate and initialize a ui_render data structure */
static ui_render_t * ui_render_init(GtkWindow * window,
				    GtkWidget * window_vbox,
				    AmitkStudy * study,
				    GList * selected_objects,
				    AmitkPreferences * preferences) {

  GdkMonitor * monitor;
  GdkRectangle geom;
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
  ui_render->x_old = 0.0;
  ui_render->y_old = 0.0;
  ui_render->z_old = 0.0;
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
    amide_gconf_get_bool(GCONF_AMIDE_RENDERING,"update-without-release");

  ui_render->stereo_eye_width = 
    amide_gconf_get_int(GCONF_AMIDE_RENDERING,"eye-width");
  if (ui_render->stereo_eye_width == 0) { /* if no config file, put in sane value */
    monitor = gdk_display_get_primary_monitor(gdk_display_get_default());
    gdk_monitor_get_geometry(monitor, &geom);
    ui_render->stereo_eye_width = 50*geom.width/gdk_monitor_get_width_mm(monitor); /* in pixels */
  }

  ui_render->stereo_eye_angle = 
    amide_gconf_get_float(GCONF_AMIDE_RENDERING,"eye-angle");
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
  pixbuf = amitk_get_pixbuf_from_canvas(AMITK_SIMPLE_CANVAS(ui_render->canvas), 0.0,0.0,
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
  AmitkCanvasItem * root;
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
  root = amitk_simple_canvas_get_root_item(AMITK_SIMPLE_CANVAS(ui_render->canvas));
  if (ui_render->canvas_image != NULL) 
    canvas_item_set(ui_render->canvas_image,
			  "pixbuf", ui_render->pixbuf, NULL);
  else { /* time to make a new image */
    ui_render->canvas_image = 
      amitk_canvas_image_new(root, ui_render->pixbuf, 0.0, 0.0, NULL);
    g_signal_connect(ui_render->canvas_image, "button-press-event",
                     G_CALLBACK(canvas_event_cb), ui_render);
    g_signal_connect(ui_render->canvas_image, "button-release-event",
                     G_CALLBACK(canvas_event_cb), ui_render);
    g_signal_connect(ui_render->canvas_image, "enter-notify-event",
                     G_CALLBACK(canvas_event_cb), ui_render);
    g_signal_connect(ui_render->canvas_image, "leave-notify-event",
                     G_CALLBACK(canvas_event_cb), ui_render);
    g_signal_connect(ui_render->canvas_image, "motion-notify-event",
                     G_CALLBACK(canvas_event_cb), ui_render);
  }

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
      canvas_item_set(ui_render->canvas_time_label,
			    "text", time_str,
			    "fill-color-rgba", color, NULL);
    else
      ui_render->canvas_time_label = 
        amitk_canvas_text_new(root, time_str, 4.0, ui_render->pixbuf_height-2.0,
                            -1, AMITK_CANVAS_ANCHOR_SOUTH_WEST,
                            "fill-color-rgba", color,
                            "font-desc", amitk_fixed_font_desc, NULL);
    g_free(time_str);
  } else {
    if (ui_render->canvas_time_label != NULL) {
      amitk_canvas_item_remove(ui_render->canvas_time_label);
      ui_render->canvas_time_label = NULL;
    }
  }

  /* reset the min size of the widget */
  amitk_simple_canvas_set_bounds(AMITK_SIMPLE_CANVAS(ui_render->canvas), 0.0, 0.0, ui_render->pixbuf_width, ui_render->pixbuf_height);
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
  gtk_application_add_window(GTK_APPLICATION(g_application_get_default()),
                             GTK_WINDOW(window));
  gtk_window_set_title(window, _("Rendering Window"));
  gtk_window_set_resizable(window, FALSE);
  window_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
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
  packing_table = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(packing_table), Y_PADDING);
  gtk_grid_set_column_spacing(GTK_GRID(packing_table), X_PADDING);
  gtk_box_pack_start (GTK_BOX (ui_render->window_vbox), packing_table, TRUE,TRUE, 0);

  /* setup the main canvas */
  ui_render->canvas = amitk_simple_canvas_new();
  gtk_grid_attach(GTK_GRID(packing_table), ui_render->canvas, 1, 1, 1, 1);
  ui_render_update_immediate(ui_render); /* fill in the canvas */

  /* create the x, y, and z rotation dials */
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_grid_attach(GTK_GRID(packing_table), hbox, 1, 0, 2, 1);
  adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, -90.0, 90.0, 1.0, 1.0, 1.0));
  scale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, adjustment);
  g_object_set_data(G_OBJECT(adjustment), "axis", GINT_TO_POINTER(AMITK_AXIS_Y));
  gtk_box_pack_start(GTK_BOX(hbox), scale, TRUE, TRUE, 0);
  g_signal_connect (scale, "button-release-event",
                    G_CALLBACK(button_release_cb), ui_render);
  label = gtk_label_new(_("y"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

  /* create the z dial */
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_grid_attach(GTK_GRID(packing_table), hbox, 3, 0, 1, 1);
  label = gtk_label_new(_("z"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
  adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, -180.0, 180.0, 1.0, 1.0, 1.0));

  dial = amitk_dial_new(adjustment);
  g_object_set_data(G_OBJECT(adjustment), "axis", GINT_TO_POINTER(AMITK_AXIS_Z));
  gtk_box_pack_start(GTK_BOX(hbox), dial, FALSE, FALSE, 0);
  g_signal_connect (dial, "button-release-event",
                    G_CALLBACK(button_release_cb), ui_render);


  /* the x slider */
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_grid_attach(GTK_GRID(packing_table), vbox, 3, 1, 1, 1);
  label = gtk_label_new(_("x"));
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
  adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, -90.0, 90.0, 1.0, 1.0, 1.0));
  scale = gtk_scale_new(GTK_ORIENTATION_VERTICAL, adjustment);
  g_object_set_data(G_OBJECT(adjustment), "axis", GINT_TO_POINTER(AMITK_AXIS_X));
  gtk_box_pack_start(GTK_BOX(vbox), scale, TRUE, TRUE, 0);
  g_signal_connect(scale, "button-release-event",
                   G_CALLBACK(button_release_cb), ui_render);

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
  amide_gconf_set_bool(GCONF_AMIDE_RENDERING,"strip-highs",
		       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
  return;
}

static void init_optimize_rendering_cb(GtkWidget * widget, gpointer data) {
  amide_gconf_set_bool(GCONF_AMIDE_RENDERING,"optimize-rendering",
		       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
  return;
}

static void init_no_gradient_opacity_cb(GtkWidget * widget, gpointer data) {
  amide_gconf_set_bool(GCONF_AMIDE_RENDERING,"initially-no-gradient-opacity",
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
					_("_Cancel"), GTK_RESPONSE_CLOSE,
					_("_Execute"), AMITK_RESPONSE_EXECUTE,
					NULL);
  gtk_window_set_title(GTK_WINDOW(dialog), temp_string);
  g_free(temp_string);

  /* setup the callbacks for the dialog */
  g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(ui_common_init_dialog_response_cb), NULL);

  gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);

  /* start making the widgets for this dialog box */
  table = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(table), Y_PADDING);
  gtk_grid_set_column_spacing(GTK_GRID(table), X_PADDING);
  table_row=0;
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area
                                  (GTK_DIALOG(dialog))), table);

  tree_view = amitk_tree_view_new(AMITK_TREE_VIEW_MODE_MULTIPLE_SELECTION,NULL, NULL);
  g_object_set_data(G_OBJECT(dialog), "tree_view", tree_view);
  amitk_tree_view_set_study(AMITK_TREE_VIEW(tree_view), study);
  amitk_tree_view_expand_object(AMITK_TREE_VIEW(tree_view), AMITK_OBJECT(study));

  /* make a scrolled area for the tree */
  scrolled = gtk_scrolled_window_new(NULL,NULL);  
  gtk_widget_set_size_request(scrolled,250,250);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), 
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrolled), tree_view);
  gtk_grid_attach(GTK_GRID(table), scrolled, 0, table_row, 2, 1);
  table_row++;

  /* do we want to strip values */
  check_button = gtk_check_button_new_with_label(_("Set values greater than max. threshold to zero?"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), strip_highs);
  gtk_grid_attach(GTK_GRID(table), check_button, 0, table_row, 2, 1);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(init_strip_highs_cb), dialog);
  table_row++;

  /* do we want to converse memory */
  check_button = gtk_check_button_new_with_label(_("Accelerate Rendering?  Increases memory use ~3x"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),optimize_rendering);
  gtk_grid_attach(GTK_GRID(table), check_button, 0, table_row, 2, 1);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(init_optimize_rendering_cb), dialog);
  table_row++;

  /* do we want the initial opacities to be only density dependent */
  check_button = gtk_check_button_new_with_label(_("Initial opacity functions only density dependent?"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),initially_no_gradient_opacity);
  gtk_grid_attach(GTK_GRID(table), check_button, 0, table_row, 2, 1);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(init_no_gradient_opacity_cb), dialog);
  table_row++;


  /* and show all our widgets */
  gtk_widget_show_all(dialog);

  return dialog;
}








#endif




















