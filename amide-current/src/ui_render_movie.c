/* ui_render_movie_dialog.c
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
#if (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)

#include "amide.h"
#include <sys/stat.h>
#include <string.h>
#include "ui_common.h"
#include "ui_render_movie.h"
#include "amitk_type_builtins.h"
#include "amitk_progress_dialog.h"
#include "mpeg_encode.h"


#define MOVIE_DEFAULT_DURATION 10.0

typedef enum {
  NOT_DYNAMIC,
  OVER_TIME, 
  OVER_FRAMES, 
  OVER_FRAMES_SMOOTHED,
  OVER_GATES,
  DYNAMIC_TYPES
} dynamic_t;


/* data structures */
typedef struct ui_render_movie_t {
  ui_render_t * ui_render; /* a pointer back to the main rendering ui */

  amide_time_t start_time;
  amide_time_t end_time;
  amide_time_t duration; /* movie duration */
  guint start_frame;
  guint end_frame;
  gdouble rotation[AMITK_AXIS_NUM];
  dynamic_t type;
  gboolean in_generation;
  gboolean quit_generation;

  GtkWidget * dialog;
  GtkWidget * progress_dialog;
  GtkWidget * start_time_label;
  GtkWidget * end_time_label;
  GtkWidget * start_frame_label;
  GtkWidget * end_frame_label;
  GtkWidget * start_time_spin_button;
  GtkWidget * end_time_spin_button;
  GtkWidget * start_frame_spin_button;
  GtkWidget * end_frame_spin_button;
  GtkWidget * duration_spin_button;
  GtkWidget * time_on_image_label;
  GtkWidget * time_on_image_button;
  GtkWidget * dynamic_type;
  GtkWidget * axis_spin_button[AMITK_AXIS_NUM];

  guint reference_count;

} ui_render_movie_t;



static void change_frames_cb(GtkWidget * widget, gpointer data);
static void change_rotation_cb(GtkWidget * widget, gpointer data);
static void dynamic_type_cb(GtkWidget * widget, gpointer data);
static void change_start_time_cb(GtkWidget * widget, gpointer data);
static void change_start_frame_cb(GtkWidget * widget, gpointer data);
static void change_end_time_cb(GtkWidget * widget, gpointer data);
static void change_end_frame_cb(GtkWidget * widget, gpointer data);
static void time_label_on_cb(GtkWidget * widget, gpointer data);
static void response_cb (GtkDialog * dialog, gint response_id, gpointer data);
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data);
static void dialog_set_sensitive(ui_render_movie_t * ui_render_movie, gboolean sensitive);


static ui_render_movie_t * movie_unref(ui_render_movie_t * ui_render_movie);
static ui_render_movie_t * movie_init(void);
static void movie_generate(ui_render_movie_t * ui_render_movie, char * output_file_name);



/* function to change the number of frames in the movie */
static void change_frames_cb(GtkWidget * widget, gpointer data) {
  ui_render_movie_t * ui_render_movie = data;
  ui_render_movie->duration = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  return;
}

/* function to change the number of rotations on an axis */
static void change_rotation_cb(GtkWidget * widget, gpointer data) {
  ui_render_movie_t * ui_render_movie = data;
  AmitkAxis i_axis;

  /* figure out which axis this is for */
  i_axis = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "which_entry"));

  /* set the rotation */
  ui_render_movie->rotation[i_axis] = 
    gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

  return;
}

static void dynamic_type_cb(GtkWidget * widget, gpointer data) {

  ui_render_movie_t * ui_render_movie = data;
  dynamic_t type;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "dynamic_type"));

  if (type != ui_render_movie->type) {
    ui_render_movie->type = type;
    if (type == OVER_TIME) {
      gtk_widget_show(ui_render_movie->start_time_label);
      gtk_widget_show(ui_render_movie->start_time_spin_button);
      gtk_widget_show(ui_render_movie->end_time_label);
      gtk_widget_show(ui_render_movie->end_time_spin_button);
    }

    if ((type == OVER_FRAMES) || (type == OVER_FRAMES_SMOOTHED)) {
      gtk_widget_show(ui_render_movie->start_frame_label);
      gtk_widget_show(ui_render_movie->start_frame_spin_button);
      gtk_widget_show(ui_render_movie->end_frame_label);
      gtk_widget_show(ui_render_movie->end_frame_spin_button);
    }

    if ((type == NOT_DYNAMIC) || (type == OVER_GATES)) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_render_movie->time_on_image_button), FALSE);
      gtk_widget_hide(ui_render_movie->time_on_image_label);
      gtk_widget_hide(ui_render_movie->time_on_image_button);
    } else {
      gtk_widget_show(ui_render_movie->time_on_image_label);
      gtk_widget_show(ui_render_movie->time_on_image_button);
    }


    if ((type != OVER_FRAMES) && (type != OVER_FRAMES_SMOOTHED)) {
      gtk_widget_hide(ui_render_movie->start_frame_label);
      gtk_widget_hide(ui_render_movie->start_frame_spin_button);
      gtk_widget_hide(ui_render_movie->end_frame_label);
      gtk_widget_hide(ui_render_movie->end_frame_spin_button);
    }

    if (type != OVER_TIME) {
      gtk_widget_hide(ui_render_movie->start_time_label);
      gtk_widget_hide(ui_render_movie->start_time_spin_button);
      gtk_widget_hide(ui_render_movie->end_time_label);
      gtk_widget_hide(ui_render_movie->end_time_spin_button);
    }
  }
  return;
}


/* function to change the start time */
static void change_start_time_cb(GtkWidget * widget, gpointer data) {
  ui_render_movie_t * ui_render_movie = data;
  ui_render_movie->start_time = 
    gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  return;
}

/* function to change the start frame */
static void change_start_frame_cb(GtkWidget * widget, gpointer data) {
  ui_render_movie_t * ui_render_movie = data;
  gint temp_val;
  temp_val = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  if (temp_val >= 0) ui_render_movie->start_frame = temp_val;
  return;
}

/* function to change the end time */
static void change_end_time_cb(GtkWidget * widget, gpointer data) {
  ui_render_movie_t * ui_render_movie = data;
  ui_render_movie->end_time = 
    gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  return;
}

/* function to change the end frame */
static void change_end_frame_cb(GtkWidget * widget, gpointer data) {
  ui_render_movie_t * ui_render_movie = data;
  ui_render_movie->end_frame = 
    gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  return;
}

static void time_label_on_cb(GtkWidget * widget, gpointer data) {
  ui_render_movie_t * ui_render_movie = data;
  ui_render_movie->ui_render->time_label_on = 
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  return;
}

/* function called when we hit the apply button */
static void response_cb (GtkDialog * dialog, gint response_id, gpointer data) {
  
  ui_render_movie_t * ui_render_movie = data;
  renderings_t * temp_renderings;
  GtkWidget * file_chooser;
  gchar * filename;
  gchar * data_set_names = NULL;
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
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER (file_chooser), TRUE);
    amitk_preferences_set_file_chooser_directory(ui_render_movie->ui_render->preferences, file_chooser); /* set the default directory if applicable */

    /* take a guess at the filename */
    temp_renderings = ui_render_movie->ui_render->renderings;
    data_set_names = g_strdup(temp_renderings->rendering->name);
    temp_renderings = temp_renderings->next;
    while (temp_renderings != NULL) {
      filename = g_strdup_printf("%s+%s",data_set_names, temp_renderings->rendering->name);
      g_free(data_set_names);
      data_set_names = filename;
      temp_renderings = temp_renderings->next;
    }
    filename = g_strdup_printf("Rendering%s%s_%d.mpg", 
				  ui_render_movie->ui_render->stereoscopic ? "_stereo_" : "_", 
				  data_set_names,save_image_num++);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER (file_chooser), filename);
    g_free(filename); 


    /* run the dialog */
    if (gtk_dialog_run(GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT) 
      filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (file_chooser));
    else
      filename = NULL;
    gtk_widget_destroy (file_chooser);

    if (filename == NULL) 
      return; /* return to movie generate dialog */
    else {  /* generate the movie */
      movie_generate(ui_render_movie, filename);
      g_free(filename);
    } 
    /* and fall through to close out the dialog */

  case GTK_RESPONSE_CANCEL:
    g_signal_emit_by_name(G_OBJECT(dialog), "delete_event", NULL, &return_val);
    if (!return_val) gtk_widget_destroy(GTK_WIDGET(dialog));
    break;

  case GTK_RESPONSE_HELP:
    amide_call_help("rendering-movie-dialog");
    break;



  default:
    break;
  }

  return;
}



/* function called to destroy the  dialog */
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_render_movie_t * ui_render_movie = data;
  ui_render_t * ui_render = ui_render_movie->ui_render;

  /* trying to close while we're generating */
  if (ui_render_movie->in_generation) {
    ui_render_movie->quit_generation=TRUE; /* signal we need to exit rendering */
    return TRUE;
  }

  /* trash collection */
  /* also informs the ui_render widget that the movie widget is gone */
  ui_render->movie = movie_unref(ui_render->movie); 

  return FALSE;
}

static void dialog_set_sensitive(ui_render_movie_t * ui_render_movie, gboolean sensitive) {

  AmitkAxis i_axis;

  gtk_widget_set_sensitive(ui_render_movie->duration_spin_button, sensitive);
  for (i_axis=0; i_axis<AMITK_AXIS_NUM; i_axis++) 
    gtk_widget_set_sensitive(ui_render_movie->axis_spin_button[i_axis], sensitive);
  gtk_widget_set_sensitive(ui_render_movie->start_time_spin_button, sensitive);
  gtk_widget_set_sensitive(ui_render_movie->end_time_spin_button, sensitive);
  gtk_widget_set_sensitive(ui_render_movie->start_frame_spin_button, sensitive);
  gtk_widget_set_sensitive(ui_render_movie->end_frame_spin_button, sensitive);
  gtk_widget_set_sensitive(ui_render_movie->dynamic_type, sensitive);
  gtk_dialog_set_response_sensitive(GTK_DIALOG(ui_render_movie->dialog), 
				    AMITK_RESPONSE_EXECUTE, sensitive);
}






/* destroy a ui_render_movie data structure */
static ui_render_movie_t * movie_unref(ui_render_movie_t * ui_render_movie) {

  gboolean return_val;

  if (ui_render_movie == NULL) return ui_render_movie;

  /* sanity checks */
  g_return_val_if_fail(ui_render_movie->reference_count > 0, NULL);

  ui_render_movie->reference_count--;

  /* things we do if we've removed all references */
  if (ui_render_movie->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing ui_render_movie\n");
#endif

    if (ui_render_movie->progress_dialog != NULL) {
      g_signal_emit_by_name(G_OBJECT(ui_render_movie->progress_dialog), 
			    "delete_event", NULL, &return_val);
      ui_render_movie->progress_dialog = NULL;
    }
    ui_render_movie->ui_render->time_label_on=FALSE;

    g_free(ui_render_movie);
    ui_render_movie = NULL;
  }
    
  return ui_render_movie;

}


/* allocate and initialize a ui_render_movie data structure */
static ui_render_movie_t * movie_init(void) {

  ui_render_movie_t * ui_render_movie;


  /* alloc space for the data structure */
  if ((ui_render_movie = g_try_new(ui_render_movie_t,1)) == NULL) {
    g_warning(_("couldn't allocate memory space for ui_render_movie_t"));
    return NULL;
  }

  ui_render_movie->reference_count = 1;
  ui_render_movie->duration = MOVIE_DEFAULT_DURATION;
  ui_render_movie->rotation[AMITK_AXIS_X] = 0;
  ui_render_movie->rotation[AMITK_AXIS_Y] = 1;
  ui_render_movie->rotation[AMITK_AXIS_Z] = 0;
  ui_render_movie->type = NOT_DYNAMIC;
  ui_render_movie->ui_render = NULL;
  ui_render_movie->start_time = 0.0;
  ui_render_movie->end_time = 1.0;
  ui_render_movie->start_frame = 0;
  ui_render_movie->end_frame = 0;
  ui_render_movie->in_generation=FALSE;
  ui_render_movie->quit_generation=FALSE;
  ui_render_movie->progress_dialog = NULL;

 return ui_render_movie;
}



/* perform the movie generation */
static void movie_generate(ui_render_movie_t * ui_render_movie, gchar * output_filename) {

  guint i_frame;
  gdouble rotation_step[AMITK_AXIS_NUM];
  AmitkAxis i_axis;
  gint return_val = TRUE;
  amide_time_t initial_start, initial_duration;
  amide_time_t start_time, duration;
  ui_render_t * ui_render;
  AmitkDataSet * most_frames_ds=NULL;
  renderings_t * renderings;
  guint ds_frame=0;
  gdouble ds_frame_real;
  guint num_frames;
  gpointer mpeg_encode_context;
  gboolean continue_work=TRUE;
  gint ds_gate;
  GdkPixbuf * pixbuf;

  /* gray out anything that could screw up the movie */
  dialog_set_sensitive(ui_render_movie, FALSE);
  ui_render_movie->in_generation=TRUE; /* indicate we're generating */

  /* save the initial times so we can set it back later */
  ui_render = ui_render_movie->ui_render;
  initial_start = ui_render->start;
  initial_duration = ui_render->duration;

  /* disable the normal rendering progress dialog */
  ui_render->disable_progress_dialog = TRUE;
  
  /* figure out which data set has the most frames, need this if we're doing a movie over frames */
  renderings = ui_render->renderings;
  while (renderings != NULL) {
    if (AMITK_IS_DATA_SET(renderings->rendering->object)) {
      if (most_frames_ds == NULL)
	most_frames_ds = AMITK_DATA_SET(renderings->rendering->object);
      else if (AMITK_DATA_SET_NUM_FRAMES(most_frames_ds) <
	       AMITK_DATA_SET_NUM_FRAMES(renderings->rendering->object))
	most_frames_ds = AMITK_DATA_SET(renderings->rendering->object);
    }
    renderings = renderings->next;
  }

  num_frames = ceil(ui_render_movie->duration*FRAMES_PER_SECOND);

  /* figure out each frame's duration, needed if we're doing a movie over time */
  if ((ui_render_movie->type == OVER_FRAMES) || 
      (ui_render_movie->type == OVER_FRAMES_SMOOTHED) ||
      (ui_render_movie->type == OVER_TIME)) {
    ui_render->duration = 
      (ui_render_movie->end_time-ui_render_movie->start_time) /((amide_time_t) num_frames);
  }

  
  mpeg_encode_context = mpeg_encode_setup(output_filename, ENCODE_MPEG1,
					  ui_render->pixbuf_width,
					  ui_render->pixbuf_height);
  g_return_if_fail(mpeg_encode_context != NULL);

  /* start generating the frames, continue while we haven't hit cancel */
  for (i_frame = 0; 
       ((i_frame < num_frames) && !ui_render_movie->quit_generation && return_val && continue_work);
       i_frame++) {

    /* update the progress bar */
    continue_work = amitk_progress_dialog_set_fraction(AMITK_PROGRESS_DIALOG(ui_render_movie->progress_dialog),
						       (gdouble) i_frame/((gdouble) num_frames));

    /* figure out the rotation for this frame */
    for (i_axis = 0; i_axis < AMITK_AXIS_NUM ; i_axis++) {
      rotation_step[i_axis] = 
	(((gdouble) i_frame)*2.0*M_PI*ui_render_movie->rotation[i_axis])
	/ num_frames;
      renderings_set_rotation(ui_render->renderings, i_axis, rotation_step[i_axis]);
    }

    /* figure out the start interval for this frame */
    switch (ui_render_movie->type) {
    case OVER_TIME:
      ui_render->start = ui_render_movie->start_time + i_frame*ui_render->duration;
      break;
    case OVER_FRAMES_SMOOTHED:
    case OVER_FRAMES:
      if (most_frames_ds) {
	ds_frame_real = (i_frame/((gdouble) num_frames)) * AMITK_DATA_SET_NUM_FRAMES(most_frames_ds);
	ds_frame = floor(ds_frame_real);
	start_time = amitk_data_set_get_start_time(most_frames_ds, ds_frame);
	duration = amitk_data_set_get_end_time(most_frames_ds, ds_frame) - start_time;
	ui_render->start = start_time + EPSILON*fabs(start_time) + 
	  ((ui_render_movie->type == OVER_FRAMES_SMOOTHED) ? ((ds_frame_real-ds_frame)*duration) : 0.0);
	ui_render->duration = duration - EPSILON*fabs(duration);
      } else { /* just have roi's.... doesn't make much sense if we get here */
	ui_render->start = 0.0;
	ui_render->duration = 1.0;
      }
      break;
    case OVER_GATES:
      renderings = ui_render->renderings;
      while (renderings != NULL) {
	if (AMITK_IS_DATA_SET(renderings->rendering->object)) {
	  ds_gate = floor((i_frame/((gdouble) num_frames))*AMITK_DATA_SET_NUM_GATES(renderings->rendering->object));
	  amitk_data_set_set_view_start_gate(AMITK_DATA_SET(renderings->rendering->object), ds_gate);
	  amitk_data_set_set_view_end_gate(AMITK_DATA_SET(renderings->rendering->object), ds_gate);
	}
	renderings = renderings->next;
      }
      break;
    default:
      /* NOT_DYNAMIC */
      break;
    }


 
    /* render the contexts */
    ui_render_update_immediate(ui_render);
    
    if (ui_render->rendered_successfully) {/* if we rendered correct, encode the mpeg frame */
      pixbuf = ui_render_get_pixbuf(ui_render);
      if (pixbuf == NULL) {
	g_warning(_("Canvas failed to return a valid image\n"));
	break;
      }
      return_val = mpeg_encode_frame(mpeg_encode_context, pixbuf);
      g_object_unref(pixbuf);
    } else
      return_val = FALSE;
      
    /* do any events pending, this allows the canvas to get displayed */
    while (gtk_events_pending()) 
      gtk_main_iteration();
      
    /* and unrotate the rendering contexts so that we can properly rerotate
       for the next frame */
    for (i_axis = AMITK_AXIS_NUM; i_axis > 0 ; i_axis--) {
      renderings_set_rotation(ui_render->renderings, 
			      i_axis-1, -rotation_step[i_axis-1]);
    }
  }

  mpeg_encode_close(mpeg_encode_context);
  amitk_progress_dialog_set_fraction(AMITK_PROGRESS_DIALOG(ui_render_movie->progress_dialog),2.0);

  /* and rerender one last time to back to the initial rotation and time */
  ui_render->start = initial_start;
  ui_render->duration = initial_duration;
  ui_render->disable_progress_dialog = FALSE;
  ui_render_add_update(ui_render); 

  ui_render_movie->in_generation = FALSE; /* done generating */
  ui_render_movie->quit_generation = FALSE;
  dialog_set_sensitive(ui_render_movie, TRUE); /* let user change stuff again */

  return;
}

/* function that sets up the rendering options dialog */
gpointer * ui_render_movie_dialog_create(ui_render_t * ui_render) {
  
  ui_render_movie_t * ui_render_movie;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * hseparator;
  GtkWidget * label;
  GtkWidget * radio_button1;
  GtkWidget * radio_button2;
  GtkWidget * radio_button3;
  GtkWidget * radio_button4;
  GtkWidget * radio_button5;
  GtkWidget * hbox;
  guint table_row = 0;
  AmitkAxis i_axis;
  renderings_t * renderings;
  AmitkDataSet * temp_ds;
  gboolean valid;
  gint temp_end_frame;
  amide_time_t temp_end_time;
  amide_time_t temp_start_time;
  
  if (ui_render->movie != NULL)
    return ui_render->movie;

  ui_render_movie = movie_init();
  ui_render->movie = ui_render_movie;
  ui_render_movie->ui_render = ui_render;


  ui_render_movie->start_frame = 0;

  /* try to get reasonable default values */
  renderings = ui_render->renderings;
  valid = FALSE;
  while (renderings != NULL) {
    if (AMITK_IS_DATA_SET(renderings->rendering->object)) {
      temp_ds = AMITK_DATA_SET(renderings->rendering->object);
      temp_end_frame = AMITK_DATA_SET_NUM_FRAMES(temp_ds)-1;
      temp_start_time = amitk_data_set_get_start_time(temp_ds,0);
      temp_end_time = amitk_data_set_get_end_time(temp_ds, temp_end_frame);
      if (!valid) {
	ui_render_movie->end_frame = temp_end_frame;
	ui_render_movie->start_time = temp_start_time;
	ui_render_movie->end_time = temp_end_time;
      } else {
	if (temp_end_frame > ui_render_movie->end_frame)
	  ui_render_movie->end_frame = temp_end_frame;
	if (temp_start_time < ui_render_movie->start_time)
	  ui_render_movie->start_time = temp_start_time;
	if (temp_end_time > ui_render_movie->end_time)
	  ui_render_movie->end_time = temp_end_time;
      }
      valid = TRUE;
    }
    renderings = renderings->next;
  }
    
  temp_string = g_strdup_printf(_("%s: Rendering Movie Generation Dialog"),PACKAGE);
  ui_render_movie->dialog = 
    gtk_dialog_new_with_buttons(temp_string, ui_render->window,
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_HELP, GTK_RESPONSE_HELP,				
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				_("_Generate Movie"), AMITK_RESPONSE_EXECUTE,
				NULL);
  g_free(temp_string);

  /* setup the callbacks for the dialog */
  g_signal_connect(G_OBJECT(ui_render_movie->dialog), 
		   "response", G_CALLBACK(response_cb), ui_render_movie);
  g_signal_connect(G_OBJECT(ui_render_movie->dialog), 
		   "delete_event", G_CALLBACK(delete_event_cb), ui_render_movie);


  /* ---------------------------
     Basic Parameters page 
     --------------------------- */


  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(5,3,FALSE);
  table_row=0;
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(ui_render_movie->dialog)->vbox), packing_table);

  /* widgets to specify how many frames */
  label = gtk_label_new(_("Movie Duration (sec)"));
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  ui_render_movie->duration_spin_button  = 
    gtk_spin_button_new_with_range(0, G_MAXDOUBLE, 1.0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ui_render_movie->duration_spin_button), FALSE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_render_movie->duration_spin_button),
			    ui_render_movie->duration);
  g_signal_connect(G_OBJECT(ui_render_movie->duration_spin_button), "value_changed", 
		   G_CALLBACK(change_frames_cb),  ui_render_movie);
  g_signal_connect(G_OBJECT(ui_render_movie->duration_spin_button), "output",
		   G_CALLBACK(amitk_spin_button_scientific_output), NULL);
  gtk_table_attach(GTK_TABLE(packing_table), ui_render_movie->duration_spin_button,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator, 0,3,
		   table_row, table_row+1,GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* widgets to specify number of rotations on the axis */
  for (i_axis=0;i_axis<AMITK_AXIS_NUM;i_axis++) {

    temp_string = g_strdup_printf(_("Rotations on %s"), amitk_axis_get_name(i_axis));
    label = gtk_label_new(temp_string);
    g_free(temp_string);
    gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

    ui_render_movie->axis_spin_button[i_axis] = 
      gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 1.0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ui_render_movie->axis_spin_button[i_axis]), FALSE);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(ui_render_movie->axis_spin_button[i_axis]), 2);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_render_movie->axis_spin_button[i_axis]),
			      ui_render_movie->rotation[i_axis]);
    g_object_set_data(G_OBJECT(ui_render_movie->axis_spin_button[i_axis]), 
		      "which_entry", GINT_TO_POINTER(i_axis));
    g_signal_connect(G_OBJECT(ui_render_movie->axis_spin_button[i_axis]), 
		     "value_changed", G_CALLBACK(change_rotation_cb), ui_render_movie);
    gtk_table_attach(GTK_TABLE(packing_table), ui_render_movie->axis_spin_button[i_axis],1,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    table_row++;
  }

  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator, 0,3,
		   table_row, table_row+1,GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* do we want to make a movie over time or over frames */
  label = gtk_label_new(_("Dynamic Movie:"));
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_table_attach(GTK_TABLE(packing_table), hbox,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(hbox);
  table_row++;

  /* the radio buttons */
  radio_button1 = gtk_radio_button_new_with_label(NULL, _("No"));
  gtk_box_pack_start(GTK_BOX(hbox), radio_button1, FALSE, FALSE, 3);
  g_object_set_data(G_OBJECT(radio_button1), "dynamic_type", GINT_TO_POINTER(NOT_DYNAMIC));
  ui_render_movie->dynamic_type = radio_button1;

  radio_button2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_button1), _("over time"));
  gtk_box_pack_start(GTK_BOX(hbox), radio_button2, FALSE, FALSE, 3);
  g_object_set_data(G_OBJECT(radio_button2), "dynamic_type", GINT_TO_POINTER(OVER_TIME));

  radio_button3 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_button1), _("over frames"));
  gtk_box_pack_start(GTK_BOX(hbox), radio_button3, FALSE, FALSE, 3);
  g_object_set_data(G_OBJECT(radio_button3), "dynamic_type", GINT_TO_POINTER(OVER_FRAMES));

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_table_attach(GTK_TABLE(packing_table), hbox,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(hbox);
  table_row++;

  radio_button4 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_button1), _("over frames smoothed"));
  gtk_box_pack_start(GTK_BOX(hbox), radio_button4, FALSE, FALSE, 3);
  g_object_set_data(G_OBJECT(radio_button4), "dynamic_type", GINT_TO_POINTER(OVER_FRAMES_SMOOTHED));

  radio_button5 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_button1), _("over gates"));
  gtk_box_pack_start(GTK_BOX(hbox), radio_button5, FALSE, FALSE, 3);
  g_object_set_data(G_OBJECT(radio_button5), "dynamic_type", GINT_TO_POINTER(OVER_GATES));

  g_signal_connect(G_OBJECT(radio_button1), "clicked", G_CALLBACK(dynamic_type_cb), ui_render_movie);
  g_signal_connect(G_OBJECT(radio_button2), "clicked", G_CALLBACK(dynamic_type_cb), ui_render_movie);
  g_signal_connect(G_OBJECT(radio_button3), "clicked", G_CALLBACK(dynamic_type_cb), ui_render_movie);
  g_signal_connect(G_OBJECT(radio_button4), "clicked", G_CALLBACK(dynamic_type_cb), ui_render_movie);
  g_signal_connect(G_OBJECT(radio_button5), "clicked", G_CALLBACK(dynamic_type_cb), ui_render_movie);


  /* widgets to specify the start and end times */
  ui_render_movie->start_time_label = gtk_label_new(_("Start Time (s)"));
  gtk_table_attach(GTK_TABLE(packing_table), ui_render_movie->start_time_label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  ui_render_movie->start_frame_label = gtk_label_new(_("Start Frame"));
  gtk_table_attach(GTK_TABLE(packing_table), ui_render_movie->start_frame_label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  ui_render_movie->start_time_spin_button = 
    gtk_spin_button_new_with_range(ui_render_movie->start_time, ui_render_movie->end_time, 1.0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ui_render_movie->start_time_spin_button), FALSE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_render_movie->start_time_spin_button),
			    ui_render_movie->start_time);
  g_signal_connect(G_OBJECT(ui_render_movie->start_time_spin_button), "value_changed", 
		   G_CALLBACK(change_start_time_cb), ui_render_movie);
  g_signal_connect(G_OBJECT(ui_render_movie->start_time_spin_button), "output",
		   G_CALLBACK(amitk_spin_button_scientific_output), NULL);
  gtk_table_attach(GTK_TABLE(packing_table), ui_render_movie->start_time_spin_button,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  ui_render_movie->start_frame_spin_button =
    gtk_spin_button_new_with_range(ui_render_movie->start_frame,ui_render_movie->end_frame+0.1, 1.0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(ui_render_movie->start_frame_spin_button),0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_render_movie->start_frame_spin_button),
			    ui_render_movie->start_frame);
  g_signal_connect(G_OBJECT(ui_render_movie->start_frame_spin_button), "value_changed", 
		   G_CALLBACK(change_start_frame_cb), ui_render_movie);
  gtk_table_attach(GTK_TABLE(packing_table), ui_render_movie->start_frame_spin_button,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  ui_render_movie->end_time_label = gtk_label_new(_("End Time (s)"));
  gtk_table_attach(GTK_TABLE(packing_table), ui_render_movie->end_time_label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  ui_render_movie->end_frame_label = gtk_label_new(_("End Frame"));
  gtk_table_attach(GTK_TABLE(packing_table), ui_render_movie->end_frame_label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);


  ui_render_movie->end_time_spin_button =
    gtk_spin_button_new_with_range(ui_render_movie->start_time, ui_render_movie->end_time, 1.0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ui_render_movie->end_time_spin_button), FALSE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_render_movie->end_time_spin_button),
			    ui_render_movie->end_time);
  g_signal_connect(G_OBJECT(ui_render_movie->end_time_spin_button), "value_changed", 
		   G_CALLBACK(change_end_time_cb), ui_render_movie);
  g_signal_connect(G_OBJECT(ui_render_movie->end_time_spin_button), "output",
		   G_CALLBACK(amitk_spin_button_scientific_output), NULL);
  gtk_table_attach(GTK_TABLE(packing_table), ui_render_movie->end_time_spin_button,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  ui_render_movie->end_frame_spin_button =
    gtk_spin_button_new_with_range(ui_render_movie->start_frame,ui_render_movie->end_frame+0.1, 1.0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(ui_render_movie->end_frame_spin_button),0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_render_movie->end_frame_spin_button),
			    ui_render_movie->end_frame);
  g_signal_connect(G_OBJECT(ui_render_movie->end_frame_spin_button), "value_changed", 
		   G_CALLBACK(change_end_frame_cb), ui_render_movie);
  gtk_table_attach(GTK_TABLE(packing_table), ui_render_movie->end_frame_spin_button,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  ui_render_movie->time_on_image_label = gtk_label_new(_("Display time on image"));
  gtk_table_attach(GTK_TABLE(packing_table), ui_render_movie->time_on_image_label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  ui_render_movie->time_on_image_button = gtk_check_button_new();
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui_render_movie->time_on_image_button), 
			       ui_render_movie->ui_render->time_label_on);
  g_signal_connect(G_OBJECT(ui_render_movie->time_on_image_button), "toggled", 
		   G_CALLBACK(time_label_on_cb), ui_render_movie);
  gtk_table_attach(GTK_TABLE(packing_table), ui_render_movie->time_on_image_button,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* progress dialog */
  ui_render_movie->progress_dialog = amitk_progress_dialog_new(GTK_WINDOW(ui_render_movie->dialog));
  amitk_progress_dialog_set_text(AMITK_PROGRESS_DIALOG(ui_render_movie->progress_dialog),
				 _("Rendered Movie Progress"));

  /* show all our widgets */
  gtk_widget_show_all(ui_render_movie->dialog);

  /* and hide the appropriate widgets */
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button1), TRUE);
  gtk_widget_hide(ui_render_movie->start_frame_label);
  gtk_widget_hide(ui_render_movie->start_frame_spin_button);
  gtk_widget_hide(ui_render_movie->end_frame_label);
  gtk_widget_hide(ui_render_movie->end_frame_spin_button);
  gtk_widget_hide(ui_render_movie->start_time_label);
  gtk_widget_hide(ui_render_movie->start_time_spin_button);
  gtk_widget_hide(ui_render_movie->end_time_label);
  gtk_widget_hide(ui_render_movie->end_time_spin_button);
  gtk_widget_hide(ui_render_movie->time_on_image_label);
  gtk_widget_hide(ui_render_movie->time_on_image_button);

  return (gpointer) ui_render_movie;
}


#endif /* AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT */
#endif /* LIBVOLPACK_SUPPORT */
