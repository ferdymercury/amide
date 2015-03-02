/* tb_fly_through.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2002-2014 Andy Loening
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

#if (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)

#include <sys/stat.h>
#include <libgnomecanvas/gnome-canvas-pixbuf.h>
#include "amide.h"
#include "amitk_threshold.h"
#include "amitk_progress_dialog.h"
#include "mpeg_encode.h"
#include "tb_fly_through.h"
#include "amitk_canvas.h"

typedef enum {
  NOT_DYNAMIC,
  OVER_TIME, 
  OVER_FRAMES, 
  OVER_FRAMES_SMOOTHED, 
  OVER_GATES,
  DYNAMIC_TYPES
} dynamic_t;

typedef struct tb_fly_through_t {
  AmitkStudy * study;
  AmitkSpace * space;
  AmitkPreferences * preferences;


  amide_time_t start_time;
  amide_time_t end_time;
  guint start_frame;
  guint end_frame;
  amide_real_t start_z;
  amide_real_t end_z;
  amide_time_t duration;
  gboolean in_generation;
  gboolean dynamic;
  gboolean gated;
  dynamic_t type;

  GtkWidget * dialog;
  GtkWidget * canvas;
  GtkWidget * start_time_label;
  GtkWidget * end_time_label;
  GtkWidget * start_frame_label;
  GtkWidget * end_frame_label;
  GtkWidget * start_time_spin_button;
  GtkWidget * end_time_spin_button;
  GtkWidget * start_frame_spin_button;
  GtkWidget * end_frame_spin_button;
  GtkWidget * time_on_image_label;
  GtkWidget * time_on_image_button;
  GtkWidget * dynamic_type;
  GtkWidget * start_position_button;
  GtkWidget * end_position_button;
  GtkWidget * start_position_spin;
  GtkWidget * end_position_spin;
  GtkWidget * duration_spin_button;
  GtkWidget * position_entry;
  GtkWidget * progress_dialog;

  guint reference_count;

} tb_fly_through_t;

static void view_changed_cb(GtkWidget * canvas, AmitkPoint *position,
			    amide_real_t thickness, gpointer data);

static void dynamic_type_cb(GtkWidget * widget, gpointer data);
static void change_start_time_cb(GtkWidget * widget, gpointer data);
static void change_start_frame_cb(GtkWidget * widget, gpointer data);
static void change_end_time_cb(GtkWidget * widget, gpointer data);
static void change_end_frame_cb(GtkWidget * widget, gpointer data);
static void time_on_image_cb(GtkWidget * widget, gpointer data);
static void set_start_position_pressed_cb(GtkWidget * button, gpointer data);
static void set_end_position_pressed_cb(GtkWidget * button, gpointer data);
static void change_start_position_spin_cb(GtkWidget * widget, gpointer data);
static void change_end_position_spin_cb(GtkWidget * widget, gpointer data);
static void change_duration_spin_cb(GtkWidget * widget, gpointer data);
static void destroy_cb(GtkObject * object, gpointer data);
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data);
static void response_cb (GtkDialog * dialog, gint response_id, gpointer data);

static void movie_generate(tb_fly_through_t * tb_fly_through, gchar * output_filename);
static void dialog_update_position_entry(tb_fly_through_t * tb_fly_through);
static void dialog_set_sensitive(tb_fly_through_t * tb_fly_through, gboolean sensitive);
static void dialog_update_entries(tb_fly_through_t * tb_fly_through);
static tb_fly_through_t * tb_fly_through_unref(tb_fly_through_t * tb_fly_through);
static tb_fly_through_t * tb_fly_through_init(void);



static void view_changed_cb(GtkWidget * canvas, AmitkPoint *position,
			    amide_real_t thickness, gpointer data) {

  tb_fly_through_t * tb_fly_through = data;

  amitk_study_set_view_center(tb_fly_through->study, *position);
  dialog_update_position_entry(tb_fly_through);

  return;
}


static void dynamic_type_cb(GtkWidget * widget, gpointer data) {

  tb_fly_through_t * tb_fly_through = data;
  dynamic_t type;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "dynamic_type"));

  if (type != tb_fly_through->type) {
    tb_fly_through->type = type;

    if (tb_fly_through->dynamic) {
      if (type == OVER_TIME) {
	gtk_widget_show(tb_fly_through->start_time_label);
	gtk_widget_show(tb_fly_through->start_time_spin_button);
	gtk_widget_show(tb_fly_through->end_time_label);
	gtk_widget_show(tb_fly_through->end_time_spin_button);
      }
      
      if ((type == OVER_FRAMES) || (type == OVER_FRAMES_SMOOTHED)) {
	gtk_widget_show(tb_fly_through->start_frame_label);
	gtk_widget_show(tb_fly_through->start_frame_spin_button);
	gtk_widget_show(tb_fly_through->end_frame_label);
	gtk_widget_show(tb_fly_through->end_frame_spin_button);
      }

      if ((type == OVER_FRAMES) || (type == OVER_FRAMES_SMOOTHED) || (type == OVER_TIME)) {
	gtk_widget_show(tb_fly_through->time_on_image_label);
	gtk_widget_show(tb_fly_through->time_on_image_button);
      } else {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_fly_through->time_on_image_button), FALSE);
	gtk_widget_hide(tb_fly_through->time_on_image_label);
	gtk_widget_hide(tb_fly_through->time_on_image_button);
      }
      
      
      if ((type != OVER_FRAMES) && (type != OVER_FRAMES_SMOOTHED)) {
	gtk_widget_hide(tb_fly_through->start_frame_label);
	gtk_widget_hide(tb_fly_through->start_frame_spin_button);
	gtk_widget_hide(tb_fly_through->end_frame_label);
	gtk_widget_hide(tb_fly_through->end_frame_spin_button);
      }
      
      if (type != OVER_TIME) {
	gtk_widget_hide(tb_fly_through->start_time_label);
	gtk_widget_hide(tb_fly_through->start_time_spin_button);
	gtk_widget_hide(tb_fly_through->end_time_label);
	gtk_widget_hide(tb_fly_through->end_time_spin_button);
      }
    }
  }
  return;
}


/* function to change the start time */
static void change_start_time_cb(GtkWidget * widget, gpointer data) {
  tb_fly_through_t * tb_fly_through = data;
  tb_fly_through->start_time = 
    gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  return;
}

/* function to change the start frame */
static void change_start_frame_cb(GtkWidget * widget, gpointer data) {
  tb_fly_through_t * tb_fly_through = data;
  gint temp_val;
  temp_val = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  if (temp_val >= 0) tb_fly_through->start_frame = temp_val;
  return;
}

/* function to change the end time */
static void change_end_time_cb(GtkWidget * widget, gpointer data) {
  tb_fly_through_t * tb_fly_through = data;
  tb_fly_through->end_time = 
    gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  return;
}

/* function to change the end frame */
static void change_end_frame_cb(GtkWidget * widget, gpointer data) {
  tb_fly_through_t * tb_fly_through = data;
  tb_fly_through->end_frame = 
    gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  return;
}

static void time_on_image_cb(GtkWidget * widget, gpointer data) {
  tb_fly_through_t * tb_fly_through = data;
  amitk_canvas_set_time_on_image(AMITK_CANVAS(tb_fly_through->canvas),
				 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
  return;
}

static void set_start_position_pressed_cb(GtkWidget * button, gpointer data) {

  AmitkPoint temp_point;
  tb_fly_through_t * tb_fly_through = data;

  temp_point = amitk_space_b2s(tb_fly_through->space,
			       AMITK_STUDY_VIEW_CENTER(tb_fly_through->study));

  tb_fly_through->start_z = temp_point.z;

  dialog_update_entries(tb_fly_through);

}

static void set_end_position_pressed_cb(GtkWidget * button, gpointer data) {

  AmitkPoint temp_point;
  tb_fly_through_t * tb_fly_through = data;

  temp_point = amitk_space_b2s(tb_fly_through->space,
			       AMITK_STUDY_VIEW_CENTER(tb_fly_through->study));

  tb_fly_through->end_z = temp_point.z;

  dialog_update_entries(tb_fly_through);

}


static void change_start_position_spin_cb(GtkWidget * widget, gpointer data) {
  tb_fly_through_t * tb_fly_through = data;
  tb_fly_through->start_z = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  return;
}



static void change_end_position_spin_cb(GtkWidget * widget, gpointer data) {
  tb_fly_through_t * tb_fly_through = data;
  tb_fly_through->end_z = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  return;
}



static void change_duration_spin_cb(GtkWidget * widget, gpointer data) {
  tb_fly_through_t * tb_fly_through = data;
  tb_fly_through->duration = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  return;
}

static void destroy_cb(GtkObject * object, gpointer data) {
  tb_fly_through_t * tb_fly_through = data;
  tb_fly_through = tb_fly_through_unref(tb_fly_through); /* free the associated data structure */
}

/* function to run for a delete_event */
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {
  tb_fly_through_t * tb_fly_through = data;

  /* trying to close while we're generating */
  if (tb_fly_through->in_generation) {
    tb_fly_through->in_generation = FALSE; /* signal we need to exit */
    return TRUE;
  }

  return FALSE;
}



/* function called when we hit the apply button */
static void response_cb (GtkDialog * dialog, gint response_id, gpointer data) {
  
  tb_fly_through_t * tb_fly_through = data;
  GtkWidget * file_chooser;
  gchar * filename;
  static guint save_image_num = 0;
  gboolean return_val;
  
  switch(response_id) {
  case AMITK_RESPONSE_EXECUTE:

    /* the rest of this function runs the file selection dialog box */
    file_chooser = gtk_file_chooser_dialog_new(_("Output MPEG As"),
					       GTK_WINDOW(dialog), /* parent window */
					       GTK_FILE_CHOOSER_ACTION_SAVE,
					       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					       GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					       NULL);
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(file_chooser), TRUE);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(file_chooser), TRUE);
    amitk_preferences_set_file_chooser_directory(tb_fly_through->preferences, file_chooser); /* set the default directory if applicable */

    /* take a guess at the filename */
    filename = g_strdup_printf("%s_FlyThrough_%d.mpg", 
				  AMITK_OBJECT_NAME(tb_fly_through->study), 
				  save_image_num++);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(file_chooser), filename);
    g_free(filename);


    if (gtk_dialog_run(GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT) 
      filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (file_chooser));
    else
      filename = NULL;
    gtk_widget_destroy(file_chooser);

    if (filename == NULL)
      return; /* return to flythrough generation dialog */
    else { /* generate the movie */
      movie_generate(tb_fly_through, filename);
      g_free(filename);
    }
    /* and fall through to close out the dialog */

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
static void movie_generate(tb_fly_through_t * tb_fly_through, gchar * output_filename) {

  guint i_frame;
  gint return_val = 1;
  gint num_frames;
  amide_real_t increment_z;
  amide_time_t duration=1.0;
  amide_time_t initial_duration;
  amide_time_t initial_start;
  amide_time_t start_time;
  AmitkPoint current_point;
  gpointer mpeg_encode_context;
  gboolean continue_work=TRUE;
  GList * data_sets;
  GList * temp_sets;
  AmitkDataSet * most_frames_ds=NULL;
  guint ds_frame=0;
  gdouble ds_frame_real;
  gint ds_gate;
  GdkPixbuf * pixbuf;

  /* gray out anything that could screw up the movie */
  dialog_set_sensitive(tb_fly_through, FALSE);
  tb_fly_through->in_generation = TRUE; /* indicate we're generating */

  /* figure out which data set has the most frames, need this if we're doing a movie over frames */
  data_sets = amitk_object_get_children_of_type(AMITK_OBJECT(tb_fly_through->study), 
						AMITK_OBJECT_TYPE_DATA_SET, TRUE);
  temp_sets = data_sets;
  while (temp_sets != NULL) {
    if (most_frames_ds == NULL)
      most_frames_ds = AMITK_DATA_SET(temp_sets->data);
    else if (AMITK_DATA_SET_NUM_FRAMES(most_frames_ds) <
	     AMITK_DATA_SET_NUM_FRAMES(temp_sets->data))
      most_frames_ds = AMITK_DATA_SET(temp_sets->data);
    temp_sets = temp_sets->next;
  }

  num_frames = ceil(tb_fly_through->duration*FRAMES_PER_SECOND);
  if (num_frames > 1)
    increment_z = (tb_fly_through->end_z-tb_fly_through->start_z)/(num_frames-1);
  else
    increment_z = 0; /* erroneous */

  current_point = amitk_space_b2s(AMITK_SPACE(tb_fly_through->space),
				  AMITK_STUDY_VIEW_CENTER(tb_fly_through->study));
  current_point.z = tb_fly_through->start_z;

  pixbuf = amitk_canvas_get_pixbuf(AMITK_CANVAS(tb_fly_through->canvas));
  g_return_if_fail(pixbuf != NULL);
  mpeg_encode_context = mpeg_encode_setup(output_filename, ENCODE_MPEG1,
					  gdk_pixbuf_get_width(pixbuf),
					  gdk_pixbuf_get_height(pixbuf));
  g_object_unref(pixbuf);
  g_return_if_fail(mpeg_encode_context != NULL);

#ifdef AMIDE_DEBUG
  g_print("Total number of movie frames to do: %d\tincrement %f\n",num_frames, increment_z);
#endif

  initial_start = AMITK_STUDY_VIEW_START_TIME(tb_fly_through->study);
  initial_duration = AMITK_STUDY_VIEW_DURATION (tb_fly_through->study);
  if (tb_fly_through->type == OVER_TIME) {
    duration = (tb_fly_through->end_time-tb_fly_through->start_time)/((amide_time_t) num_frames);
    amitk_study_set_view_duration(tb_fly_through->study, duration);
  } 
  

  /* start generating the frames, continue while we haven't hit cancel  */
  for (i_frame = 0; 
       (i_frame < num_frames) && tb_fly_through->in_generation && (return_val==1) && (continue_work); 
       i_frame++) {

    switch (tb_fly_through->type) {
    case OVER_FRAMES:
    case OVER_FRAMES_SMOOTHED:
      ds_frame_real = (i_frame/((gdouble) num_frames)) * AMITK_DATA_SET_NUM_FRAMES(most_frames_ds);
      ds_frame = floor(ds_frame_real);
      start_time = amitk_data_set_get_start_time(most_frames_ds, ds_frame);
      duration = amitk_data_set_get_end_time(most_frames_ds, ds_frame)-start_time;
      start_time = start_time + EPSILON*fabs(start_time) +
	((tb_fly_through->type == OVER_FRAMES_SMOOTHED) ? ((ds_frame_real-ds_frame)*duration) : 0.0);
      duration = duration - EPSILON*fabs(duration);
      amitk_study_set_view_start_time(tb_fly_through->study, start_time);
      amitk_study_set_view_duration(tb_fly_through->study, duration);
      break;
    case OVER_TIME:
      start_time = tb_fly_through->start_time + i_frame*duration;
      amitk_study_set_view_start_time(tb_fly_through->study, start_time);
      break;
    case OVER_GATES:
      temp_sets = data_sets;
      while (temp_sets != NULL) {
	ds_gate = floor((i_frame/((gdouble) num_frames))*AMITK_DATA_SET_NUM_GATES(temp_sets->data));
	amitk_data_set_set_view_start_gate(AMITK_DATA_SET(temp_sets->data), ds_gate);
	amitk_data_set_set_view_end_gate(AMITK_DATA_SET(temp_sets->data), ds_gate);
	temp_sets = temp_sets->next;
      }
      break;
    default:
      /* NOT_DYNAMIC */
      break;
    }

    dialog_update_position_entry(tb_fly_through);
    continue_work = amitk_progress_dialog_set_fraction(AMITK_PROGRESS_DIALOG(tb_fly_through->progress_dialog),
						       (i_frame)/((gdouble) num_frames));

    /* advance the canvas */
    amitk_study_set_view_center(tb_fly_through->study,
				amitk_space_s2b(tb_fly_through->space, current_point));



    /* do any events pending, and make sure the canvas gets updated */
    while (gtk_events_pending() || AMITK_CANVAS(tb_fly_through->canvas)->next_update)
      gtk_main_iteration();
      
    pixbuf = amitk_canvas_get_pixbuf(AMITK_CANVAS(tb_fly_through->canvas));
    g_return_if_fail(pixbuf != NULL);
    return_val = mpeg_encode_frame(mpeg_encode_context, pixbuf);
    g_object_unref(pixbuf);

    if (return_val != 1) 
      g_warning(_("encoding of frame %d failed"), i_frame);

    current_point.z += increment_z;
  }
  mpeg_encode_close(mpeg_encode_context);
  amitk_progress_dialog_set_fraction(AMITK_PROGRESS_DIALOG(tb_fly_through->progress_dialog),2.0);

  /* reset the canvas */
  amitk_study_set_view_start_time(tb_fly_through->study, initial_start);
  amitk_study_set_view_duration(tb_fly_through->study, initial_duration);

  /* free up references */
  amitk_objects_unref(data_sets);

  tb_fly_through->in_generation = FALSE; /* done generating */
  dialog_set_sensitive(tb_fly_through, TRUE); /* let user change stuff again */

  return;
}

static void dialog_update_position_entry(tb_fly_through_t * tb_fly_through) {

  gchar * temp_str;
  AmitkPoint temp_point;

  temp_point = amitk_space_b2s(tb_fly_through->space,
			       AMITK_STUDY_VIEW_CENTER(tb_fly_through->study));
  temp_str = g_strdup_printf("%f", temp_point.z);
  gtk_entry_set_text(GTK_ENTRY(tb_fly_through->position_entry), temp_str);
  g_free(temp_str);
  
  return;
}

static void dialog_set_sensitive(tb_fly_through_t * tb_fly_through, gboolean sensitive) {

  gtk_widget_set_sensitive(tb_fly_through->start_position_button, sensitive);
  gtk_widget_set_sensitive(tb_fly_through->end_position_button, sensitive);
  gtk_widget_set_sensitive(tb_fly_through->position_entry, sensitive);
  gtk_widget_set_sensitive(tb_fly_through->start_position_spin, sensitive);
  gtk_widget_set_sensitive(tb_fly_through->end_position_spin, sensitive);
  gtk_widget_set_sensitive(tb_fly_through->duration_spin_button, sensitive);
  gtk_widget_set_sensitive(GTK_WIDGET(tb_fly_through->canvas), sensitive);

  if (tb_fly_through->dynamic || tb_fly_through->gated) {
    gtk_widget_set_sensitive(tb_fly_through->dynamic_type, sensitive);
  }

  if (tb_fly_through->dynamic) {
    gtk_widget_set_sensitive(tb_fly_through->start_frame_spin_button, sensitive);
    gtk_widget_set_sensitive(tb_fly_through->end_frame_spin_button, sensitive);
    gtk_widget_set_sensitive(tb_fly_through->start_time_spin_button, sensitive);
    gtk_widget_set_sensitive(tb_fly_through->end_time_spin_button, sensitive);
  }

  gtk_dialog_set_response_sensitive(GTK_DIALOG(tb_fly_through->dialog), 
				    AMITK_RESPONSE_EXECUTE, sensitive);
}

static void dialog_update_entries(tb_fly_through_t * tb_fly_through) {
  
  g_signal_handlers_block_by_func(G_OBJECT(tb_fly_through->start_position_spin), 
				  G_CALLBACK(change_start_position_spin_cb), tb_fly_through);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_fly_through->start_position_spin), 
			    tb_fly_through->start_z);
  g_signal_handlers_unblock_by_func(G_OBJECT(tb_fly_through->start_position_spin), 
				    G_CALLBACK(change_start_position_spin_cb), 
				    tb_fly_through);

  g_signal_handlers_block_by_func(G_OBJECT(tb_fly_through->end_position_spin), 
				  G_CALLBACK(change_end_position_spin_cb), tb_fly_through);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_fly_through->end_position_spin), 
			    tb_fly_through->end_z);
  g_signal_handlers_unblock_by_func(G_OBJECT(tb_fly_through->end_position_spin), 
				    G_CALLBACK(change_end_position_spin_cb), 
				    tb_fly_through);

  g_signal_handlers_block_by_func(G_OBJECT(tb_fly_through->duration_spin_button), 
				  G_CALLBACK(change_duration_spin_cb), tb_fly_through);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_fly_through->duration_spin_button), 
			    tb_fly_through->duration);
  g_signal_handlers_unblock_by_func(G_OBJECT(tb_fly_through->duration_spin_button), 
				    G_CALLBACK(change_duration_spin_cb), 
				    tb_fly_through);
  return;
}



static tb_fly_through_t * tb_fly_through_unref(tb_fly_through_t * tb_fly_through) {

  g_return_val_if_fail(tb_fly_through != NULL, NULL);
  gboolean return_val;

  /* sanity checks */
  g_return_val_if_fail(tb_fly_through->reference_count > 0, NULL);

  /* remove a reference count */
  tb_fly_through->reference_count--;

  /* things to do if we've removed all reference's */
  if (tb_fly_through->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing tb_fly_through\n");
#endif

    if (tb_fly_through->study != NULL) {
      amitk_object_unref(tb_fly_through->study);
      tb_fly_through->study = NULL;
    }

    if (tb_fly_through->space != NULL) {
      g_object_unref(tb_fly_through->space);
      tb_fly_through->space = NULL;
    }

    if (tb_fly_through->preferences != NULL) {
      g_object_unref(tb_fly_through->preferences);
      tb_fly_through->preferences = NULL;
    }

    if (tb_fly_through->progress_dialog != NULL) {
      g_signal_emit_by_name(G_OBJECT(tb_fly_through->progress_dialog), "delete_event", NULL, &return_val);
      tb_fly_through->progress_dialog = NULL;
    }

    g_free(tb_fly_through);
    tb_fly_through = NULL;
  }

  return tb_fly_through;

}

/* adds one to the reference count  */
//static tb_fly_through_t * fly_through_ref(tb_fly_through_t * fly_through) {
//
//  g_return_val_if_fail(fly_through != NULL, NULL);
//
//  fly_through->reference_count++;
//
//  return fly_through;
//}

/* allocate and initialize a tb_fly_through data structure */
static tb_fly_through_t * tb_fly_through_init(void) {

  tb_fly_through_t * tb_fly_through;

  /* alloc space for the data structure for passing ui info */
  if ((tb_fly_through = g_try_new(tb_fly_through_t,1)) == NULL) {
    g_warning(_("couldn't allocate memory space for tb_fly_through_t"));
    return NULL;
  }
  tb_fly_through->reference_count = 1;

  /* set any needed parameters */
  tb_fly_through->study = NULL;
  tb_fly_through->space = NULL;
  tb_fly_through->preferences = NULL;
  tb_fly_through->start_z = 0.0;
  tb_fly_through->end_z = 0.0;
  tb_fly_through->duration = 10.0; /* seconds */
  tb_fly_through->in_generation = FALSE;
  tb_fly_through->start_time = 0.0;
  tb_fly_through->end_time = 1.0;
  tb_fly_through->start_frame = 0;
  tb_fly_through->end_frame = 0;
  tb_fly_through->type = NOT_DYNAMIC;

  return tb_fly_through;
}



void tb_fly_through(AmitkStudy * study,
		    AmitkView view, 
		    AmitkPreferences * preferences,
		    GtkWindow * parent) {
 
  tb_fly_through_t * tb_fly_through;
  GtkWidget * packing_table;
  GtkWidget * right_table;
  GtkWidget * label;
  gint table_row=0;
  AmitkCorners corners;
  GList * objects;
  GList * temp_objects;
  gboolean dynamic = FALSE;
  gboolean gated = FALSE;
  gboolean valid;
  gint temp_end_frame;
  amide_time_t temp_end_time;
  amide_time_t temp_start_time;
  GtkWidget * radio_button1;
  GtkWidget * radio_button2=NULL;
  GtkWidget * radio_button3=NULL;
  GtkWidget * radio_button4=NULL;
  GtkWidget * radio_button5=NULL;
  GtkWidget * hbox;
  GtkWidget * hseparator;
  AmitkDataSet * temp_ds;


  /* sanity checks */
  g_return_if_fail(AMITK_IS_STUDY(study));
  objects = amitk_object_get_selected_children(AMITK_OBJECT(study), AMITK_SELECTION_SELECTED_0, TRUE);
  if (amitk_data_sets_count(objects, FALSE) == 0) return;

  tb_fly_through = tb_fly_through_init();
  tb_fly_through->study = AMITK_STUDY(amitk_object_copy(AMITK_OBJECT(study)));
  tb_fly_through->space = amitk_space_get_view_space(view, AMITK_STUDY_CANVAS_LAYOUT(study));
  tb_fly_through->preferences = g_object_ref(preferences);

  /* need to reset the view center, as this gets overwritten in
     amitk_object_copy because study_add_child gets called */
  amitk_study_set_view_center(tb_fly_through->study, AMITK_STUDY_VIEW_CENTER(study));


  tb_fly_through->dialog = 
    gtk_dialog_new_with_buttons(_("Fly Through Generation"),  parent,
				GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				_("_Generate Fly Through"), AMITK_RESPONSE_EXECUTE,
				NULL);

  g_signal_connect(G_OBJECT(tb_fly_through->dialog), "delete_event",
		   G_CALLBACK(delete_event_cb), tb_fly_through);
  g_signal_connect(G_OBJECT(tb_fly_through->dialog), "destroy",
		   G_CALLBACK(destroy_cb), tb_fly_through);
  g_signal_connect(G_OBJECT(tb_fly_through->dialog), "response", 
		   G_CALLBACK(response_cb), tb_fly_through);
  gtk_window_set_resizable(GTK_WINDOW(tb_fly_through->dialog), TRUE);

  /* make the widgets for this dialog box */
  packing_table = gtk_table_new(2,3,FALSE);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(tb_fly_through->dialog)->vbox), packing_table);

  right_table = gtk_table_new(9,2,FALSE);
  gtk_table_attach(GTK_TABLE(packing_table), right_table, 2,3, 0,2,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);

  label = gtk_label_new(_("Current Position (mm):"));
  gtk_table_attach(GTK_TABLE(right_table), label, 0,1, table_row,table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  tb_fly_through->position_entry = gtk_entry_new();
  gtk_editable_set_editable(GTK_EDITABLE(tb_fly_through->position_entry), FALSE);
  gtk_table_attach(GTK_TABLE(right_table), tb_fly_through->position_entry,
		   1,2, table_row, table_row+1, 
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  label = gtk_label_new(_("Start Position (mm):"));
  gtk_table_attach(GTK_TABLE(right_table), label, 0,1, table_row,table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  tb_fly_through->start_position_spin = 
    gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 1.0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tb_fly_through->start_position_spin), FALSE);
  g_signal_connect(G_OBJECT(tb_fly_through->start_position_spin), "value_changed", 
		   G_CALLBACK(change_start_position_spin_cb), tb_fly_through);
  g_signal_connect(G_OBJECT(tb_fly_through->start_position_spin), "output",
		   G_CALLBACK(amitk_spin_button_scientific_output), NULL);
  gtk_table_attach(GTK_TABLE(right_table), tb_fly_through->start_position_spin,
		   1,2, table_row, table_row+1, 
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  label = gtk_label_new(_("End Position (mm):"));
  gtk_table_attach(GTK_TABLE(right_table), label, 0,1, table_row,table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  tb_fly_through->end_position_spin = 
    gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 1.0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tb_fly_through->end_position_spin), FALSE);
  g_signal_connect(G_OBJECT(tb_fly_through->end_position_spin), "value_changed", 
		   G_CALLBACK(change_end_position_spin_cb), tb_fly_through);
  g_signal_connect(G_OBJECT(tb_fly_through->end_position_spin), "output",
		   G_CALLBACK(amitk_spin_button_scientific_output), NULL);
  gtk_table_attach(GTK_TABLE(right_table), tb_fly_through->end_position_spin,
		   1,2, table_row, table_row+1, 
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  label = gtk_label_new(_("Movie Duration (sec):"));
  gtk_table_attach(GTK_TABLE(right_table), label, 0,1, table_row,table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  tb_fly_through->duration_spin_button = 
    gtk_spin_button_new_with_range(0, G_MAXDOUBLE, 1.0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tb_fly_through->duration_spin_button), FALSE);
  g_signal_connect(G_OBJECT(tb_fly_through->duration_spin_button), "value_changed", 
		   G_CALLBACK(change_duration_spin_cb), tb_fly_through);
  g_signal_connect(G_OBJECT(tb_fly_through->duration_spin_button), "output",
		   G_CALLBACK(amitk_spin_button_scientific_output), NULL);
  gtk_table_attach(GTK_TABLE(right_table), tb_fly_through->duration_spin_button,
		   1,2, table_row, table_row+1, 
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* the progress dialog */
  tb_fly_through->progress_dialog = amitk_progress_dialog_new(GTK_WINDOW(tb_fly_through->dialog));
  amitk_progress_dialog_set_text(AMITK_PROGRESS_DIALOG(tb_fly_through->progress_dialog),
				 _("Fly through movie generation"));

  /* setup the canvas */
  tb_fly_through->canvas = amitk_canvas_new(tb_fly_through->study, view, 
					    AMITK_VIEW_MODE_SINGLE, AMITK_CANVAS_TYPE_FLY_THROUGH);
  g_signal_connect(G_OBJECT(tb_fly_through->canvas), "view_changed",
		   G_CALLBACK(view_changed_cb), tb_fly_through);
  gtk_table_attach(GTK_TABLE(packing_table), tb_fly_through->canvas, 0,2,0,1,
		   X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);

  tb_fly_through->start_position_button = gtk_button_new_with_label(_("Set Start Position"));
  g_signal_connect(G_OBJECT(tb_fly_through->start_position_button), "pressed",
		   G_CALLBACK(set_start_position_pressed_cb), tb_fly_through);
  gtk_table_attach(GTK_TABLE(packing_table), tb_fly_through->start_position_button, 
		   0,1,1,2, X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  tb_fly_through->end_position_button = gtk_button_new_with_label(_("Set End Position"));
  g_signal_connect(G_OBJECT(tb_fly_through->end_position_button), "pressed",
		   G_CALLBACK(set_end_position_pressed_cb), tb_fly_through);
  gtk_table_attach(GTK_TABLE(packing_table), tb_fly_through->end_position_button, 
		   1,2,1,2, X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;



  /* load up the canvases and get some initial info */
  amitk_volumes_get_enclosing_corners(objects, tb_fly_through->space, corners);
  tb_fly_through->start_z = point_get_component(corners[0], AMITK_AXIS_Z);
  tb_fly_through->end_z = point_get_component(corners[1], AMITK_AXIS_Z);

  temp_objects = objects;
  valid = FALSE;
  while (temp_objects != NULL) {
    if (AMITK_IS_DATA_SET(temp_objects->data)) {
      temp_ds = AMITK_DATA_SET(temp_objects->data);
      temp_end_frame = AMITK_DATA_SET_NUM_FRAMES(temp_ds)-1;
      temp_start_time = amitk_data_set_get_start_time(temp_ds,0);
      temp_end_time = amitk_data_set_get_end_time(temp_ds, temp_end_frame);
      if (!valid) {
	tb_fly_through->end_frame = temp_end_frame;
	tb_fly_through->start_time = temp_start_time;
	tb_fly_through->end_time = temp_end_time;
      } else {
	if (temp_end_frame > tb_fly_through->end_frame)
	  tb_fly_through->end_frame = temp_end_frame;
	if (temp_start_time < tb_fly_through->start_time)
	  tb_fly_through->start_time = temp_start_time;
	if (temp_end_time > tb_fly_through->end_time)
	  tb_fly_through->end_time = temp_end_time;
      }
      valid = TRUE;

      if (AMITK_DATA_SET_NUM_FRAMES(temp_objects->data) > 1)
	dynamic = TRUE;
      if (AMITK_DATA_SET_NUM_GATES(temp_objects->data) > 1)
	gated = TRUE;
    }
    temp_objects = temp_objects->next;
  }
  tb_fly_through->dynamic=dynamic;
  tb_fly_through->gated=gated;

  /* garbage collection */
  amitk_objects_unref(objects);


  if (tb_fly_through->dynamic || tb_fly_through->gated) {
    /* a separator for clarity */
    hseparator = gtk_hseparator_new();
    gtk_table_attach(GTK_TABLE(right_table), hseparator, 0,2,
		     table_row, table_row+1,GTK_FILL, 0, X_PADDING, Y_PADDING);
    table_row++;
    
    /* do we want to make a movie over time or over frames */
    label = gtk_label_new(_("Dynamic Movie:"));
    gtk_table_attach(GTK_TABLE(right_table), label, 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(right_table), hbox,1,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(hbox);
    table_row++;
    
    /* the radio buttons */
    radio_button1 = gtk_radio_button_new_with_label(NULL, _("No"));
    gtk_box_pack_start(GTK_BOX(hbox), radio_button1, FALSE, FALSE, 3);
    g_object_set_data(G_OBJECT(radio_button1), "dynamic_type", GINT_TO_POINTER(NOT_DYNAMIC));
    tb_fly_through->dynamic_type = radio_button1;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button1), TRUE);
    
    if (tb_fly_through->dynamic) {
      radio_button2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_button1), _("over time"));
      gtk_box_pack_start(GTK_BOX(hbox), radio_button2, FALSE, FALSE, 3);
      g_object_set_data(G_OBJECT(radio_button2), "dynamic_type", GINT_TO_POINTER(OVER_TIME));
    
      radio_button3 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_button1), _("over frames"));
      gtk_box_pack_start(GTK_BOX(hbox), radio_button3, FALSE, FALSE, 3);
      g_object_set_data(G_OBJECT(radio_button3), "dynamic_type", GINT_TO_POINTER(OVER_FRAMES));
    }
    
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(right_table), hbox,1,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(hbox);
    table_row++;

    if (tb_fly_through->dynamic) {
      radio_button4 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_button1), _("over frames smoothed"));
      gtk_box_pack_start(GTK_BOX(hbox), radio_button4, FALSE, FALSE, 3);
      g_object_set_data(G_OBJECT(radio_button4), "dynamic_type", GINT_TO_POINTER(OVER_FRAMES_SMOOTHED));
    }
    
    if (tb_fly_through->gated) {
      radio_button5 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_button1), _("over gates"));
      gtk_box_pack_start(GTK_BOX(hbox), radio_button5, FALSE, FALSE, 3);
      g_object_set_data(G_OBJECT(radio_button5), "dynamic_type", GINT_TO_POINTER(OVER_GATES));
    }
    
    g_signal_connect(G_OBJECT(radio_button1), "clicked", G_CALLBACK(dynamic_type_cb), tb_fly_through);
    if (tb_fly_through->dynamic) {
      g_signal_connect(G_OBJECT(radio_button2), "clicked", G_CALLBACK(dynamic_type_cb), tb_fly_through);
      g_signal_connect(G_OBJECT(radio_button3), "clicked", G_CALLBACK(dynamic_type_cb), tb_fly_through);
      g_signal_connect(G_OBJECT(radio_button4), "clicked", G_CALLBACK(dynamic_type_cb), tb_fly_through);
    }
    if (tb_fly_through->gated) {
      g_signal_connect(G_OBJECT(radio_button5), "clicked", G_CALLBACK(dynamic_type_cb), tb_fly_through);
    }
    
  }

  if (tb_fly_through->dynamic) {
    
    /* widgets to specify the start and end times */
    tb_fly_through->start_time_label = gtk_label_new(_("Start Time (s)"));
    gtk_table_attach(GTK_TABLE(right_table), tb_fly_through->start_time_label, 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    tb_fly_through->start_frame_label = gtk_label_new(_("Start Frame"));
    gtk_table_attach(GTK_TABLE(right_table), tb_fly_through->start_frame_label, 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    
    tb_fly_through->start_time_spin_button = 
      gtk_spin_button_new_with_range(tb_fly_through->start_time, tb_fly_through->end_time, 1.0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tb_fly_through->start_time_spin_button), FALSE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_fly_through->start_time_spin_button),
			      tb_fly_through->start_time);
    g_signal_connect(G_OBJECT(tb_fly_through->start_time_spin_button), "value_changed", 
		     G_CALLBACK(change_start_time_cb), tb_fly_through);
    g_signal_connect(G_OBJECT(tb_fly_through->start_time_spin_button), "output",
		     G_CALLBACK(amitk_spin_button_scientific_output), NULL);
    gtk_table_attach(GTK_TABLE(right_table), tb_fly_through->start_time_spin_button,1,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    
    tb_fly_through->start_frame_spin_button =
      gtk_spin_button_new_with_range(tb_fly_through->start_frame,tb_fly_through->end_frame+0.1, 1.0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tb_fly_through->start_frame_spin_button),0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_fly_through->start_frame_spin_button),
			      tb_fly_through->start_frame);
    g_signal_connect(G_OBJECT(tb_fly_through->start_frame_spin_button), "value_changed", 
		     G_CALLBACK(change_start_frame_cb), tb_fly_through);
    gtk_table_attach(GTK_TABLE(right_table), tb_fly_through->start_frame_spin_button,1,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    table_row++;
    
    tb_fly_through->end_time_label = gtk_label_new(_("End Time (s)"));
    gtk_table_attach(GTK_TABLE(right_table), tb_fly_through->end_time_label, 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    tb_fly_through->end_frame_label = gtk_label_new(_("End Frame"));
    gtk_table_attach(GTK_TABLE(right_table), tb_fly_through->end_frame_label, 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    
    
    tb_fly_through->end_time_spin_button =
      gtk_spin_button_new_with_range(tb_fly_through->start_time, tb_fly_through->end_time, 1.0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tb_fly_through->end_time_spin_button), FALSE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_fly_through->end_time_spin_button),
			      tb_fly_through->end_time);
    g_signal_connect(G_OBJECT(tb_fly_through->end_time_spin_button), "value_changed", 
		     G_CALLBACK(change_end_time_cb), tb_fly_through);
    g_signal_connect(G_OBJECT(tb_fly_through->end_time_spin_button), "output",
		     G_CALLBACK(amitk_spin_button_scientific_output), NULL);
    gtk_table_attach(GTK_TABLE(right_table), tb_fly_through->end_time_spin_button,1,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    
    tb_fly_through->end_frame_spin_button =
      gtk_spin_button_new_with_range(tb_fly_through->start_frame,tb_fly_through->end_frame+0.1, 1.0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tb_fly_through->end_frame_spin_button),0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_fly_through->end_frame_spin_button),
			      tb_fly_through->end_frame);
    g_signal_connect(G_OBJECT(tb_fly_through->end_frame_spin_button), "value_changed", 
		     G_CALLBACK(change_end_frame_cb), tb_fly_through);
    gtk_table_attach(GTK_TABLE(right_table), tb_fly_through->end_frame_spin_button,1,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    table_row++;

    tb_fly_through->time_on_image_label = gtk_label_new(_("Display time on image"));
    gtk_table_attach(GTK_TABLE(right_table), tb_fly_through->time_on_image_label, 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    tb_fly_through->time_on_image_button = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_fly_through->time_on_image_button), FALSE);
    g_signal_connect(G_OBJECT(tb_fly_through->time_on_image_button), "toggled", 
		     G_CALLBACK(time_on_image_cb), tb_fly_through);
    gtk_table_attach(GTK_TABLE(right_table), tb_fly_through->time_on_image_button,1,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    table_row++;
  }
  



  /* update entries */
  dialog_update_position_entry(tb_fly_through);
  dialog_update_entries(tb_fly_through);

  /* and show all our widgets */
  gtk_widget_show_all(tb_fly_through->dialog);
		 
  /* and hide the appropriate widgets */
  if (tb_fly_through->dynamic) {
    gtk_widget_hide(tb_fly_through->start_frame_label);
    gtk_widget_hide(tb_fly_through->start_frame_spin_button);
    gtk_widget_hide(tb_fly_through->end_frame_label);
    gtk_widget_hide(tb_fly_through->end_frame_spin_button);
    gtk_widget_hide(tb_fly_through->start_time_label);
    gtk_widget_hide(tb_fly_through->start_time_spin_button);
    gtk_widget_hide(tb_fly_through->end_time_label);
    gtk_widget_hide(tb_fly_through->end_time_spin_button);
    gtk_widget_hide(tb_fly_through->time_on_image_label);
    gtk_widget_hide(tb_fly_through->time_on_image_button);
  }


  return;
}



#endif /* AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT */



