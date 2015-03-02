/* ui_fly_through.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2002 Andy Loening
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

#ifdef AMIDE_LIBFAME_SUPPORT

#include <sys/stat.h>
#include <gnome.h>
#include <libgnomecanvas/gnome-canvas-pixbuf.h>
#include "amitk_threshold.h"
#include "mpeg_encode.h"
#include "ui_common.h"
#include "ui_fly_through.h"
#include "amitk_canvas.h"

#define SPIN_BUTTON_DIGITS 3

typedef struct ui_fly_through_t {
  AmitkStudy * study;
  AmitkSpace * space;

  amide_real_t start_z;
  amide_real_t end_z;
  amide_time_t duration;
  gboolean in_generation;

  GtkWidget * dialog;
  GtkWidget * canvas;
  GtkWidget * progress_bar;
  GtkWidget * start_position_button;
  GtkWidget * end_position_button;
  GtkWidget * start_spin_button;
  GtkWidget * end_spin_button;
  GtkWidget * duration_spin_button;
  GtkWidget * position_entry;

  guint reference_count;

} ui_fly_through_t;

static void view_changed_cb(GtkWidget * canvas, AmitkPoint *position,
			    amide_real_t thickness, gpointer data);
static void set_start_pressed_cb(GtkWidget * button, gpointer data);
static void set_end_pressed_cb(GtkWidget * button, gpointer data);
static void change_start_spin_button_cb(GtkWidget * widget, gpointer data);
static void change_end_spin_button_cb(GtkWidget * widget, gpointer data);
static void change_duration_spin_button_cb(GtkWidget * widget, gpointer data);
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data);
static void save_as_ok_cb(GtkWidget* widget, gpointer data);
static void response_cb (GtkDialog * dialog, gint response_id, gpointer data);

static void movie_generate(ui_fly_through_t * ui_fly_through, gchar * output_filename);
static void dialog_update_position_entry(ui_fly_through_t * ui_fly_through);
static void dialog_set_sensitive(ui_fly_through_t * ui_fly_through, gboolean sensitive);
static void dialog_update_entries(ui_fly_through_t * ui_fly_through);
static ui_fly_through_t * ui_fly_through_unref(ui_fly_through_t * ui_fly_through);
static ui_fly_through_t * fly_through_ref(ui_fly_through_t * fly_through);
static ui_fly_through_t * ui_fly_through_init(void);



static void view_changed_cb(GtkWidget * canvas, AmitkPoint *position,
			    amide_real_t thickness, gpointer data) {

  ui_fly_through_t * ui_fly_through = data;

  amitk_study_set_view_center(ui_fly_through->study, *position);
  dialog_update_position_entry(ui_fly_through);

  return;
}


static void set_start_pressed_cb(GtkWidget * button, gpointer data) {

  AmitkPoint temp_point;
  ui_fly_through_t * ui_fly_through = data;

  temp_point = amitk_space_b2s(ui_fly_through->space,
			       AMITK_STUDY_VIEW_CENTER(ui_fly_through->study));

  ui_fly_through->start_z = temp_point.z;

  dialog_update_entries(ui_fly_through);

}

static void set_end_pressed_cb(GtkWidget * button, gpointer data) {

  AmitkPoint temp_point;
  ui_fly_through_t * ui_fly_through = data;

  temp_point = amitk_space_b2s(ui_fly_through->space,
			       AMITK_STUDY_VIEW_CENTER(ui_fly_through->study));

  ui_fly_through->end_z = temp_point.z;

  dialog_update_entries(ui_fly_through);

}


static void change_start_spin_button_cb(GtkWidget * widget, gpointer data) {
  ui_fly_through_t * ui_fly_through = data;
  ui_fly_through->start_z = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  return;
}



static void change_end_spin_button_cb(GtkWidget * widget, gpointer data) {
  ui_fly_through_t * ui_fly_through = data;
  ui_fly_through->end_z = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  return;
}



static void change_duration_spin_button_cb(GtkWidget * widget, gpointer data) {
  ui_fly_through_t * ui_fly_through = data;
  ui_fly_through->duration = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  return;
}

/* function to run for a delete_event */
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_fly_through_t * ui_fly_through = data;

  /* trying to close while we're generating */
  if (ui_fly_through->in_generation) {
    ui_fly_through->in_generation = FALSE; /* signal we need to exit */
    return TRUE;
  }

  /* free the associated data structure */
  ui_fly_through = ui_fly_through_unref(ui_fly_through);

  return FALSE;
}


/* function to handle picking an output mpeg file name */
static void save_as_ok_cb(GtkWidget* widget, gpointer data) {

  GtkWidget * file_selection = data;
  gchar * save_filename;
  ui_fly_through_t * ui_fly_through;

  ui_fly_through = g_object_get_data(G_OBJECT(file_selection), "ui_fly_through");

  if ((save_filename = ui_common_file_selection_get_name(file_selection)) == NULL)
    return; /* inappropriate name or don't want to overwrite */

  /* close the file selection box */
  ui_common_file_selection_cancel_cb(widget, file_selection);

  /* and generate our movie */
  movie_generate(ui_fly_through, save_filename);
  g_free(save_filename);

  return;
}

/* function called when we hit the apply button */
static void response_cb (GtkDialog * dialog, gint response_id, gpointer data) {
  
  ui_fly_through_t * ui_fly_through = data;
  GtkWidget * file_selection;
  gchar * temp_string;
  static guint save_image_num = 0;
  gboolean return_val;
  
  switch(response_id) {
  case AMITK_RESPONSE_EXECUTE:
    /* the rest of this function runs the file selection dialog box */
    file_selection = gtk_file_selection_new(_("Output MPEG As"));

    temp_string = g_strdup_printf("%s_FlyThrough_%d.mpg", 
				  AMITK_OBJECT_NAME(ui_fly_through->study), 
				  save_image_num++);
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selection), temp_string);
    g_free(temp_string); 
    
    /* don't want anything else going on till this window is gone */
    gtk_window_set_modal(GTK_WINDOW(file_selection), TRUE);
    
    /* save a pointer to the ui_rendering_movie data, so we can use it in the callbacks */
    g_object_set_data(G_OBJECT(file_selection), "ui_fly_through", ui_fly_through);
    
    /* connect the signals */
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(file_selection)->ok_button),
		     "clicked", G_CALLBACK(save_as_ok_cb), file_selection);
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(file_selection)->cancel_button),
		     "clicked", G_CALLBACK(ui_common_file_selection_cancel_cb), file_selection);
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(file_selection)->cancel_button),
		     "delete_event", G_CALLBACK(ui_common_file_selection_cancel_cb),  file_selection);
    
    /* set the position of the dialog */
    gtk_window_set_position(GTK_WINDOW(file_selection), GTK_WIN_POS_MOUSE);
    
    /* run the dialog */
    gtk_widget_show(file_selection);

    break;

  case GTK_RESPONSE_CANCEL:
    g_signal_emit_by_name(G_OBJECT(dialog), "delete_event", NULL, &return_val);
    if (!return_val) gtk_widget_destroy(GTK_WIDGET(dialog));
    break;

  default:
    break;
  }

  return;
}



/* perform the movie generation */
static void movie_generate(ui_fly_through_t * ui_fly_through, gchar * output_filename) {

  guint i_frame;
  gint return_val = 1;
  gint num_frames;
  amide_real_t increment_z;
  AmitkPoint current_point;
  gpointer mpeg_encode_context;

  /* gray out anything that could screw up the movie */
  dialog_set_sensitive(ui_fly_through, FALSE);
  ui_fly_through->in_generation = TRUE; /* indicate we're generating */

  num_frames = ceil(ui_fly_through->duration*FRAMES_PER_SECOND);
  if (num_frames > 1)
    increment_z = (ui_fly_through->end_z-ui_fly_through->start_z)/(num_frames-1);
  else
    increment_z = 0; /* erroneous */

  current_point = amitk_space_b2s(AMITK_SPACE(ui_fly_through->space),
				  AMITK_STUDY_VIEW_CENTER(ui_fly_through->study));
  current_point.z = ui_fly_through->start_z;

  mpeg_encode_context = mpeg_encode_setup(output_filename, ENCODE_MPEG1,
					  gdk_pixbuf_get_width(AMITK_CANVAS_PIXBUF(ui_fly_through->canvas)),
					  gdk_pixbuf_get_height(AMITK_CANVAS_PIXBUF(ui_fly_through->canvas)));
  g_return_if_fail(mpeg_encode_context != NULL);

#ifdef AMIDE_DEBUG
  g_print("Total number of movie frames to do: %d\tincrement %f\n",num_frames, increment_z);
#endif
  

  /* start generating the frames, continue while we haven't hit cancel  */
  for (i_frame = 0; 
       (i_frame < num_frames) && ui_fly_through->in_generation && (return_val==1); 
       i_frame++) {

    dialog_update_position_entry(ui_fly_through);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ui_fly_through->progress_bar),
				  (i_frame)/((gdouble) num_frames));

    /* advance the canvas */
    amitk_study_set_view_center(ui_fly_through->study,
				amitk_space_s2b(ui_fly_through->space, current_point));

    /* do any events pending, and make sure the canvas gets updated */
    while (gtk_events_pending() || AMITK_CANVAS(ui_fly_through->canvas)->next_update)
      gtk_main_iteration();
      
    return_val = mpeg_encode_frame(mpeg_encode_context, AMITK_CANVAS_PIXBUF(ui_fly_through->canvas));

    if (return_val != 1) 
      g_warning("encoding of frame %d failed", i_frame);

    current_point.z += increment_z;
  }
  mpeg_encode_close(mpeg_encode_context);
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ui_fly_through->progress_bar),1.0);


  ui_fly_through->in_generation = FALSE; /* done generating */
  dialog_set_sensitive(ui_fly_through, TRUE); /* let user change stuff again */

  return;
}

static void dialog_update_position_entry(ui_fly_through_t * ui_fly_through) {

  gchar * temp_str;
  AmitkPoint temp_point;

  temp_point = amitk_space_b2s(ui_fly_through->space,
			       AMITK_STUDY_VIEW_CENTER(ui_fly_through->study));
  temp_str = g_strdup_printf("%f", temp_point.z);
  gtk_entry_set_text(GTK_ENTRY(ui_fly_through->position_entry), temp_str);
  g_free(temp_str);
  
  return;
}

static void dialog_set_sensitive(ui_fly_through_t * ui_fly_through, gboolean sensitive) {

  gtk_widget_set_sensitive(ui_fly_through->start_position_button, sensitive);
  gtk_widget_set_sensitive(ui_fly_through->end_position_button, sensitive);
  gtk_widget_set_sensitive(ui_fly_through->position_entry, sensitive);
  gtk_widget_set_sensitive(ui_fly_through->start_spin_button, sensitive);
  gtk_widget_set_sensitive(ui_fly_through->end_spin_button, sensitive);
  gtk_widget_set_sensitive(ui_fly_through->duration_spin_button, sensitive);
  gtk_widget_set_sensitive(GTK_WIDGET(ui_fly_through->canvas), sensitive);
  gtk_dialog_set_response_sensitive(GTK_DIALOG(ui_fly_through->dialog), 
				    AMITK_RESPONSE_EXECUTE, sensitive);
}

static void dialog_update_entries(ui_fly_through_t * ui_fly_through) {
  
  g_signal_handlers_block_by_func(G_OBJECT(ui_fly_through->start_spin_button), 
				  G_CALLBACK(change_start_spin_button_cb), ui_fly_through);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_fly_through->start_spin_button), 
			    ui_fly_through->start_z);
  g_signal_handlers_unblock_by_func(G_OBJECT(ui_fly_through->start_spin_button), 
				    G_CALLBACK(change_start_spin_button_cb), 
				    ui_fly_through);

  g_signal_handlers_block_by_func(G_OBJECT(ui_fly_through->end_spin_button), 
				  G_CALLBACK(change_end_spin_button_cb), ui_fly_through);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_fly_through->end_spin_button), 
			    ui_fly_through->end_z);
  g_signal_handlers_unblock_by_func(G_OBJECT(ui_fly_through->end_spin_button), 
				    G_CALLBACK(change_end_spin_button_cb), 
				    ui_fly_through);

  g_signal_handlers_block_by_func(G_OBJECT(ui_fly_through->duration_spin_button), 
				  G_CALLBACK(change_duration_spin_button_cb), ui_fly_through);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_fly_through->duration_spin_button), 
			    ui_fly_through->duration);
  g_signal_handlers_unblock_by_func(G_OBJECT(ui_fly_through->duration_spin_button), 
				    G_CALLBACK(change_duration_spin_button_cb), 
				    ui_fly_through);
  return;
}



static ui_fly_through_t * ui_fly_through_unref(ui_fly_through_t * ui_fly_through) {

  g_return_val_if_fail(ui_fly_through != NULL, NULL);

  /* sanity checks */
  g_return_val_if_fail(ui_fly_through->reference_count > 0, NULL);

  /* remove a reference count */
  ui_fly_through->reference_count--;

  /* things to do if we've removed all reference's */
  if (ui_fly_through->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing ui_fly_through\n");
#endif

    if (ui_fly_through->study != NULL) {
      amitk_canvas_remove_object(AMITK_CANVAS(ui_fly_through->canvas), 
				 AMITK_OBJECT(ui_fly_through->study));
      g_object_unref(ui_fly_through->study);
      ui_fly_through->study = NULL;
    }

    if (ui_fly_through->space != NULL) {
      g_object_unref(ui_fly_through->space);
      ui_fly_through->space = NULL;
    }

    g_free(ui_fly_through);
    ui_fly_through = NULL;
  }

  return ui_fly_through;

}

/* adds one to the reference count  */
static ui_fly_through_t * fly_through_ref(ui_fly_through_t * fly_through) {

  g_return_val_if_fail(fly_through != NULL, NULL);

  fly_through->reference_count++;

  return fly_through;
}

/* allocate and initialize a ui_fly_through data structure */
static ui_fly_through_t * ui_fly_through_init(void) {

  ui_fly_through_t * ui_fly_through;

  /* alloc space for the data structure for passing ui info */
  if ((ui_fly_through = g_try_new(ui_fly_through_t,1)) == NULL) {
    g_warning("couldn't allocate space for ui_fly_through_t");
    return NULL;
  }
  ui_fly_through->reference_count = 1;

  /* set any needed parameters */
  ui_fly_through->study = NULL;
  ui_fly_through->space = NULL;
  ui_fly_through->start_z = 0.0;
  ui_fly_through->end_z = 0.0;
  ui_fly_through->duration = 10.0; /* seconds */
  ui_fly_through->in_generation = FALSE;

  return ui_fly_through;
}



void ui_fly_through_create(GtkWidget * parent_app,
			   AmitkStudy * study,
			   GList * objects,
			   AmitkView view, 
			   AmitkLayout layout) {
 
  ui_fly_through_t * ui_fly_through;
  GtkWidget * packing_table;
  GtkWidget * right_table;
  GtkWidget * label;
  GtkWidget * hseparator;
  gint table_row=0;
  AmitkCorners corners;


  /* sanity checks */
  g_return_if_fail(AMITK_IS_STUDY(study));
  if (amitk_data_sets_count(objects, FALSE) == 0) return;

  ui_fly_through = ui_fly_through_init();
  ui_fly_through->study = AMITK_STUDY(amitk_object_copy(AMITK_OBJECT(study)));
  ui_fly_through->space = amitk_space_get_view_space(view, layout);

  ui_fly_through->dialog = 
    gtk_dialog_new_with_buttons("Fly Through Generation",  GTK_WINDOW(parent_app),
				GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
				GTK_STOCK_EXECUTE, AMITK_RESPONSE_EXECUTE,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				NULL);

  g_signal_connect(G_OBJECT(ui_fly_through->dialog), "delete_event",
		   G_CALLBACK(delete_event_cb), ui_fly_through);
  g_signal_connect(G_OBJECT(ui_fly_through->dialog), "response", 
		   G_CALLBACK(response_cb), ui_fly_through);
  gtk_window_set_resizable(GTK_WINDOW(ui_fly_through->dialog), TRUE);

  /* make the widgets for this dialog box */
  packing_table = gtk_table_new(2,3,FALSE);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(ui_fly_through->dialog)->vbox), packing_table);

  right_table = gtk_table_new(6,2,FALSE);
  gtk_table_attach(GTK_TABLE(packing_table), right_table, 2,3, 0,1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);

  label = gtk_label_new("Current Position (mm):");
  gtk_table_attach(GTK_TABLE(right_table), label, 0,1, table_row,table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  ui_fly_through->position_entry = gtk_entry_new();
  gtk_editable_set_editable(GTK_EDITABLE(ui_fly_through->position_entry), FALSE);
  gtk_table_attach(GTK_TABLE(right_table), ui_fly_through->position_entry,
		   1,2, table_row, table_row+1, 
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  label = gtk_label_new("Start Position (mm):");
  gtk_table_attach(GTK_TABLE(right_table), label, 0,1, table_row,table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  ui_fly_through->start_spin_button = 
    gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 1.0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(ui_fly_through->start_spin_button), 
			     SPIN_BUTTON_DIGITS);
  g_signal_connect(G_OBJECT(ui_fly_through->start_spin_button), "value_changed", 
		   G_CALLBACK(change_start_spin_button_cb), ui_fly_through);
  gtk_table_attach(GTK_TABLE(right_table), ui_fly_through->start_spin_button,
		   1,2, table_row, table_row+1, 
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  label = gtk_label_new("End Position (mm):");
  gtk_table_attach(GTK_TABLE(right_table), label, 0,1, table_row,table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  ui_fly_through->end_spin_button = 
    gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 1.0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(ui_fly_through->end_spin_button), 
			     SPIN_BUTTON_DIGITS);
  g_signal_connect(G_OBJECT(ui_fly_through->end_spin_button), "value_changed", 
		   G_CALLBACK(change_end_spin_button_cb), ui_fly_through);
  gtk_table_attach(GTK_TABLE(right_table), ui_fly_through->end_spin_button,
		   1,2, table_row, table_row+1, 
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  label = gtk_label_new("Movie Duration (sec):");
  gtk_table_attach(GTK_TABLE(right_table), label, 0,1, table_row,table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  ui_fly_through->duration_spin_button = 
    gtk_spin_button_new_with_range(0, G_MAXDOUBLE, 1.0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(ui_fly_through->duration_spin_button), 
			     SPIN_BUTTON_DIGITS);
  g_signal_connect(G_OBJECT(ui_fly_through->duration_spin_button), "value_changed", 
		   G_CALLBACK(change_duration_spin_button_cb), ui_fly_through);
  gtk_table_attach(GTK_TABLE(right_table), ui_fly_through->duration_spin_button,
		   1,2, table_row, table_row+1, 
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(right_table), hseparator,
		   0,2, table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(hseparator);
  table_row++;

  /* the progress bar */
  label = gtk_label_new("Progress:");
  gtk_table_attach(GTK_TABLE(right_table), label, 0,1, table_row,table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  ui_fly_through->progress_bar = gtk_progress_bar_new();
  gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(ui_fly_through->progress_bar), 
				   GTK_PROGRESS_LEFT_TO_RIGHT);
  gtk_table_attach(GTK_TABLE(right_table), ui_fly_through->progress_bar,
		   1,2, table_row, table_row+1, 
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  /* setup the canvas */
  ui_fly_through->canvas = 
    amitk_canvas_new(ui_fly_through->study, view, layout, 0, 0, NULL, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(ui_fly_through->canvas), "view_changed",
		   G_CALLBACK(view_changed_cb), ui_fly_through);
  gtk_table_attach(GTK_TABLE(packing_table), ui_fly_through->canvas, 0,2,0,1,
		   X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);

  ui_fly_through->start_position_button = gtk_button_new_with_label("Set Start Position");
  g_signal_connect(G_OBJECT(ui_fly_through->start_position_button), "pressed",
		   G_CALLBACK(set_start_pressed_cb), ui_fly_through);
  gtk_table_attach(GTK_TABLE(packing_table), ui_fly_through->start_position_button, 
		   0,1,1,2, X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  ui_fly_through->end_position_button = gtk_button_new_with_label("Set End Position");
  g_signal_connect(G_OBJECT(ui_fly_through->end_position_button), "pressed",
		   G_CALLBACK(set_end_pressed_cb), ui_fly_through);
  gtk_table_attach(GTK_TABLE(packing_table), ui_fly_through->end_position_button, 
		   1,2,1,2, X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;



  /* load up the canvases and get some initial info */
  amitk_volumes_get_enclosing_corners(objects, ui_fly_through->space, corners);
  ui_fly_through->start_z = point_get_component(corners[0], AMITK_AXIS_Z);
  ui_fly_through->end_z = point_get_component(corners[1], AMITK_AXIS_Z);

  while (objects != NULL) {
    if (AMITK_IS_DATA_SET(objects->data)) {
      amitk_object_add_child(AMITK_OBJECT(ui_fly_through->study), objects->data);
      amitk_canvas_add_object(AMITK_CANVAS(ui_fly_through->canvas), objects->data);
    }
    objects = objects->next;
  }

  /* update entries */
  dialog_update_position_entry(ui_fly_through);
  dialog_update_entries(ui_fly_through);

  /* and show all our widgets */
  gtk_widget_show_all(ui_fly_through->dialog);
		 
  return;
}



#endif /* AMIDE_LIBFAME_SUPPORT */



