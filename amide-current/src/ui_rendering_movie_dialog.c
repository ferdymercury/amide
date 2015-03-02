/* ui_rendering_movie_dialog.c
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
#ifdef AMIDE_LIBFAME_SUPPORT

#include <sys/stat.h>
#include <gnome.h>
#include <string.h>
#include "ui_common.h"
#include "ui_rendering_movie_dialog.h"
#include "amitk_type_builtins.h"
#include "mpeg_encode.h"


#define MOVIE_DEFAULT_DURATION 10.0
#define AMITK_RESPONSE_EXECUTE 1

static void change_frames_cb(GtkWidget * widget, gpointer data);
static void change_rotation_cb(GtkWidget * widget, gpointer data);
static void dynamic_type_cb(GtkWidget * widget, gpointer data);
static void change_start_time_cb(GtkWidget * widget, gpointer data);
static void change_start_frame_cb(GtkWidget * widget, gpointer data);
static void change_end_time_cb(GtkWidget * widget, gpointer data);
static void change_end_frame_cb(GtkWidget * widget, gpointer data);
static void response_cb (GtkDialog * dialog, gint response_id, gpointer data);
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data);


static ui_rendering_movie_t * movie_unref(ui_rendering_movie_t * ui_rendering_movie);
static ui_rendering_movie_t * movie_init(void);
static ui_rendering_movie_t * movie_ref(ui_rendering_movie_t * movie);
static void movie_generate(ui_rendering_movie_t * ui_rendering_movie, char * output_file_name);


/* function to handle picking an output mpeg file name */
static void save_as_ok_cb(GtkWidget* widget, gpointer data) {

  GtkWidget * file_selection = data;
  ui_rendering_movie_t * ui_rendering_movie;
  gchar * save_filename;

  ui_rendering_movie = g_object_get_data(G_OBJECT(file_selection), "ui_movie");

  if ((save_filename = ui_common_file_selection_get_name(file_selection)) == NULL)
    return; /* inappropriate name or don't want to overwrite */

  /* close the file selection box */
  ui_common_file_selection_cancel_cb(widget, file_selection);

  /* and generate our movie */
  movie_generate(ui_rendering_movie, save_filename);
  g_free(save_filename);

  return;
}



/* function to change the number of frames in the movie */
static void change_frames_cb(GtkWidget * widget, gpointer data) {

  ui_rendering_movie_t * ui_rendering_movie = data;
  gchar * str;
  gint error;
  gdouble temp_val;

  /* get the contents of the name entry box */
  str = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert to a decimal */
  error = sscanf(str, "%lf", &temp_val);
  g_free(str);

  if (error == EOF)  /* make sure it's a valid number */
    return;
  if (temp_val < 0.0)
    return;

  /* set the frames */
  ui_rendering_movie->duration = temp_val;

  return;
}

/* function to change the number of rotations on an axis */
static void change_rotation_cb(GtkWidget * widget, gpointer data) {

  ui_rendering_movie_t * ui_rendering_movie = data;
  gchar * str;
  gint error;
  gdouble temp_val;
  AmitkAxis i_axis;

  /* get the contents of the name entry box */
  str = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert to a decimal */
  error = sscanf(str, "%lf", &temp_val);
  g_free(str);

  if (error == EOF)  /* make sure it's a valid number */
    return;

  /* figure out which axis this is for */
  i_axis = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "which_entry"));

  /* set the rotation */
  ui_rendering_movie->rotation[i_axis] = temp_val;

  return;
}

static void dynamic_type_cb(GtkWidget * widget, gpointer data) {

  ui_rendering_movie_t * ui_rendering_movie = data;
  dynamic_t type;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "dynamic_type"));

  if (type != ui_rendering_movie->type) {
    ui_rendering_movie->type = type;
    if (type == OVER_TIME) {
      gtk_widget_show(ui_rendering_movie->start_time_label);
      gtk_widget_show(ui_rendering_movie->start_time_entry);
      gtk_widget_show(ui_rendering_movie->end_time_label);
      gtk_widget_show(ui_rendering_movie->end_time_entry);
      gtk_widget_hide(ui_rendering_movie->start_frame_label);
      gtk_widget_hide(ui_rendering_movie->start_frame_entry);
      gtk_widget_hide(ui_rendering_movie->end_frame_label);
      gtk_widget_hide(ui_rendering_movie->end_frame_entry);
    } else {
      gtk_widget_hide(ui_rendering_movie->start_time_label);
      gtk_widget_hide(ui_rendering_movie->start_time_entry);
      gtk_widget_hide(ui_rendering_movie->end_time_label);
      gtk_widget_hide(ui_rendering_movie->end_time_entry);
      gtk_widget_show(ui_rendering_movie->start_frame_label);
      gtk_widget_show(ui_rendering_movie->start_frame_entry);
      gtk_widget_show(ui_rendering_movie->end_frame_label);
      gtk_widget_show(ui_rendering_movie->end_frame_entry);
    }
  }
  return;
}


/* function to change the start time */
static void change_start_time_cb(GtkWidget * widget, gpointer data) {

  ui_rendering_movie_t * ui_rendering_movie = data;
  gchar * str;
  gint error;
  gdouble temp_val;

  /* get the contents of the name entry box */
  str = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert to a decimal */
  error = sscanf(str, "%lf", &temp_val);
  g_free(str);

  if (error == EOF)  /* make sure it's a valid number */
    return;

  ui_rendering_movie->start_time = temp_val;

  return;
}

/* function to change the start frame */
static void change_start_frame_cb(GtkWidget * widget, gpointer data) {

  ui_rendering_movie_t * ui_rendering_movie = data;
  gchar * str;
  gint error;
  gint temp_val;

  /* get the contents of the name entry box */
  str = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert to a decimal */
  error = sscanf(str, "%d", &temp_val);
  g_free(str);

  if (error == EOF)  /* make sure it's a valid number */
    return;

  /* set the start frame */
  if (temp_val >= 0)
    ui_rendering_movie->start_frame = temp_val;

  return;
}

/* function to change the end time */
static void change_end_time_cb(GtkWidget * widget, gpointer data) {

  ui_rendering_movie_t * ui_rendering_movie = data;
  gchar * str;
  gint error;
  gdouble temp_val;

  /* get the contents of the name entry box */
  str = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert to a decimal */
  error = sscanf(str, "%lf", &temp_val);
  g_free(str);

  if (error == EOF)  /* make sure it's a valid number */
    return;

  ui_rendering_movie->end_time = temp_val;

  return;
}

/* function to change the end frame */
static void change_end_frame_cb(GtkWidget * widget, gpointer data) {

  ui_rendering_movie_t * ui_rendering_movie = data;
  gchar * str;
  gint error;
  gint temp_val;

  /* get the contents of the name entry box */
  str = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert to a decimal */
  error = sscanf(str, "%d", &temp_val);
  g_free(str);

  if (error == EOF)  /* make sure it's a valid number */
    return;

  /* set the end frame */
  ui_rendering_movie->end_frame = temp_val;

  return;
}

/* function called when we hit the apply button */
static void response_cb (GtkDialog * dialog, gint response_id, gpointer data) {
  
  ui_rendering_movie_t * ui_rendering_movie = data;
  renderings_t * temp_contexts;
  GtkWidget * file_selection;
  gchar * temp_string;
  gchar * data_set_names = NULL;
  static guint save_image_num = 0;
  gboolean return_val;
  
  switch(response_id) {
  case AMITK_RESPONSE_EXECUTE:
    /* the rest of this function runs the file selection dialog box */
    file_selection = gtk_file_selection_new(_("Output MPEG As"));
    
    /* take a guess at the filename */
    temp_contexts = ui_rendering_movie->ui_rendering->contexts;
    data_set_names = g_strdup(temp_contexts->context->name);
    temp_contexts = temp_contexts->next;
    while (temp_contexts != NULL) {
      temp_string = g_strdup_printf("%s+%s",data_set_names, temp_contexts->context->name);
      g_free(data_set_names);
      data_set_names = temp_string;
      temp_contexts = temp_contexts->next;
    }
    temp_string = g_strdup_printf("Rendering%s(%s)_%d.mpg", 
				  ui_rendering_movie->ui_rendering->stereoscopic ? "_stereo_" : "_", 
				  data_set_names,save_image_num++);
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selection), temp_string);
    g_free(temp_string); 
    
    /* don't want anything else going on till this window is gone */
    gtk_window_set_modal(GTK_WINDOW(file_selection), TRUE);
    
    /* save a pointer to the ui_rendering_movie data, so we can use it in the callbacks */
    g_object_set_data(G_OBJECT(file_selection), "ui_movie", ui_rendering_movie);
    
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

  case GTK_RESPONSE_CLOSE:
    g_signal_emit_by_name(G_OBJECT(dialog), "delete_event", NULL, &return_val);
    if (!return_val) gtk_widget_destroy(GTK_WIDGET(dialog));
    break;

  default:
    break;
  }

  return;
}

/* callback for the help button */
/*
static void help_cb(GnomePropertyBox *rendering_dialog, gint page_number, gpointer data) {

  GError *err=NULL;

  switch (page_number) {
  case 0:
    gnome_help_display (PACKAGE,"rendering-dialog-help.html#RENDERING-MOVIE-DIALOG-HELP-MOVIE", &err);
    break;
  default:
    gnome_help_display (PACKAGE, "rendering-dialog-help.html#RENDERING-MOVIE-DIALOG-HELP", &err);
    break;
  }

  if (err != NULL) {
    g_warning("couldn't open help file, error: %s", err->message);
    g_error_free(err);
  }

  return;
}
*/


/* function called to destroy the rendering parameter dialog */
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_rendering_movie_t * ui_rendering_movie = data;
  ui_rendering_t * ui_rendering = ui_rendering_movie->ui_rendering;

  /* trash collection */
  movie_unref(ui_rendering->movie);
  ui_rendering->movie = NULL; /* informs the ui_rendering widget that the movie widget is gone */

  return FALSE;
}





/* destroy a ui_rendering_movie data structure */
static ui_rendering_movie_t * movie_unref(ui_rendering_movie_t * ui_rendering_movie) {

  if (ui_rendering_movie == NULL) return ui_rendering_movie;

  /* sanity checks */
  g_return_val_if_fail(ui_rendering_movie->reference_count > 0, NULL);

  ui_rendering_movie->reference_count--;

  /* things we do if we've removed all references */
  if (ui_rendering_movie->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing ui_rendering_movie\n");
#endif
    g_free(ui_rendering_movie);
    ui_rendering_movie = NULL;
  }
    
  return ui_rendering_movie;

}

/* adds one to the reference count  */
static ui_rendering_movie_t * movie_ref(ui_rendering_movie_t * movie) {

  g_return_val_if_fail(movie != NULL, NULL);

  movie->reference_count++;

  return movie;
}


/* allocate and initialize a ui_rendering_movie data structure */
static ui_rendering_movie_t * movie_init(void) {

  ui_rendering_movie_t * ui_rendering_movie;


  /* alloc space for the data structure */
  if ((ui_rendering_movie = g_new(ui_rendering_movie_t,1)) == NULL) {
    g_warning("couldn't allocate space for ui_rendering_movie_t");
    return NULL;
  }

  ui_rendering_movie->reference_count = 1;
  ui_rendering_movie->duration = MOVIE_DEFAULT_DURATION;
  ui_rendering_movie->rotation[AMITK_AXIS_X] = 0;
  ui_rendering_movie->rotation[AMITK_AXIS_Y] = 1;
  ui_rendering_movie->rotation[AMITK_AXIS_Z] = 0;
  ui_rendering_movie->type = OVER_TIME;
  ui_rendering_movie->ui_rendering = NULL;
  ui_rendering_movie->start_time = 0.0;
  ui_rendering_movie->end_time = 1.0;
  ui_rendering_movie->start_frame = 0;
  ui_rendering_movie->end_frame = 0;

 return ui_rendering_movie;
}




/* perform the movie generation */
static void movie_generate(ui_rendering_movie_t * ui_rendering_movie, gchar * output_filename) {

  guint i_frame;
  gdouble rotation_step[AMITK_AXIS_NUM];
  AmitkAxis i_axis;
  gint return_val = 1;
  amide_time_t initial_start, initial_duration;
  amide_time_t start_time, end_time;
  ui_rendering_t * ui_rendering;
  AmitkDataSet * most_frames_ds=NULL;
  renderings_t * contexts;
  guint ds_frame=0;
  guint num_frames;
  gpointer mpeg_encode_context;

  /* add a reference, we do this 'cause if there's only one reference left (i.e. this one), we 
     know that the user has hit a cancel button */
  ui_rendering_movie = movie_ref(ui_rendering_movie);

  /* save the initial times so we can set it back later */
  ui_rendering = ui_rendering_movie->ui_rendering;
  initial_start = ui_rendering->start;
  initial_duration = ui_rendering->duration;
  
  /* figure out which data set has the most frames, need this if we're doing a movie over frames */
  contexts = ui_rendering->contexts;
  while (contexts != NULL) {
    if (AMITK_IS_DATA_SET(contexts->context->object)) {
      if (most_frames_ds == NULL)
	most_frames_ds = AMITK_DATA_SET(contexts->context->object);
      else if (AMITK_DATA_SET_NUM_FRAMES(most_frames_ds) <
	       AMITK_DATA_SET_NUM_FRAMES(contexts->context->object))
	most_frames_ds = AMITK_DATA_SET(contexts->context->object);
    }
    contexts = contexts->next;
  }

  num_frames = ceil(ui_rendering_movie->duration*FRAMES_PER_SECOND);

  /* figure out each frame's duration, needed if we're doing a movie over time */
  ui_rendering->duration = 
    (ui_rendering_movie->end_time-ui_rendering_movie->start_time)
    /((amide_time_t) num_frames);

  
  mpeg_encode_context = mpeg_encode_setup(output_filename, ENCODE_MPEG1,
					  gdk_pixbuf_get_width(ui_rendering->pixbuf),
					  gdk_pixbuf_get_height(ui_rendering->pixbuf));
  g_return_if_fail(mpeg_encode_context != NULL);

#ifdef AMIDE_DEBUG
  g_print("Total number of movie frames to do:\t%d\n",num_frames);
  g_print("Frame:\n");
#endif
  

  /* start generating the frames */
  for (i_frame = 0; i_frame < num_frames; i_frame++) {
    if ((return_val == 1) && (ui_rendering_movie->reference_count > 1)) { 
      /* continue if good so far and we haven't closed the dialog box */

      /* figure out the rotation for this frame */
      for (i_axis = 0; i_axis < AMITK_AXIS_NUM ; i_axis++) {
	rotation_step[i_axis] = 
	  (((gdouble) i_frame)*2.0*M_PI*ui_rendering_movie->rotation[i_axis])
	  / num_frames;
	renderings_set_rotation(ui_rendering->contexts, i_axis, rotation_step[i_axis]);
      }

      /* figure out the start interval for this frame */
      if (ui_rendering_movie->type == OVER_TIME) {
	ui_rendering->start = ui_rendering_movie->start_time + i_frame*ui_rendering->duration;
      }  else {
	if (most_frames_ds) {
	  ds_frame = floor((i_frame/((gdouble) num_frames)
			    * AMITK_DATA_SET_NUM_FRAMES(most_frames_ds)));
	  start_time = amitk_data_set_get_start_time(most_frames_ds, ds_frame);
	  end_time = amitk_data_set_get_end_time(most_frames_ds, ds_frame);
	  ui_rendering->start = start_time + EPSILON*fabs(start_time);
	  ui_rendering->duration = end_time - EPSILON*fabs(end_time);
	} else { /* just have roi's.... doesn't make much sense if we get here */
	  ui_rendering->start = 0.0;
	  ui_rendering->duration = 1.0;
	}
      }
 

#ifdef AMIDE_DEBUG
      if (ui_rendering_movie->type == OVER_TIME)
	g_print("Rendering %d -- %5.3f-%5.3f s -- rot. x %5.1f y %5.1f z %5.1f\r",
		i_frame, ui_rendering->start, ui_rendering->start+ui_rendering->duration,
		180.0*rotation_step[AMITK_AXIS_X]/M_PI, 
		180.0*rotation_step[AMITK_AXIS_Y]/M_PI, 
		180.0*rotation_step[AMITK_AXIS_Z]/M_PI);
      else
	g_print("Rendering %d -- data frame %d -- rot. x %5.1f y %5.1f z %5.1f\r",
		i_frame, ds_frame, 
		180.0*rotation_step[AMITK_AXIS_X]/M_PI, 
		180.0*rotation_step[AMITK_AXIS_Y]/M_PI, 
		180.0*rotation_step[AMITK_AXIS_Z]/M_PI);
#endif
      
      /* render the contexts */
      ui_rendering_update_canvas(ui_rendering, TRUE); 
      
#ifdef AMIDE_DEBUG
      g_print("Writing   %d\r",i_frame);
#endif

      return_val = mpeg_encode_frame(mpeg_encode_context, ui_rendering->pixbuf);

      if (return_val != 1) 
	g_warning("encoding of frame %d failed", i_frame);
      
      /* do any events pending, this allows the canvas to get displayed */
      while (gtk_events_pending()) 
	gtk_main_iteration();
      
      /* and unrotate the rendering contexts so that we can properly rerotate
	 for the next frame */
      for (i_axis = AMITK_AXIS_NUM; i_axis > 0 ; i_axis--) {
	renderings_set_rotation(ui_rendering->contexts, 
				i_axis-1, -rotation_step[i_axis-1]);
      }
      
      /* update the progress bar */
      gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ui_rendering_movie->progress),
				    (gdouble) i_frame/((gdouble) num_frames));
    }
  }

  mpeg_encode_close(mpeg_encode_context);


  /* update the progress bar */
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ui_rendering_movie->progress), 1.0);

  /* and rerender one last time to back to the initial rotation and time */
  ui_rendering->start = initial_start;
  ui_rendering->duration = initial_duration;
  ui_rendering_update_canvas(ui_rendering, TRUE); 

  /* and remove the reference we added here*/
  ui_rendering_movie = movie_unref(ui_rendering_movie);

  return;
}

/* function that sets up the rendering options dialog */
ui_rendering_movie_t * ui_rendering_movie_dialog_create(ui_rendering_t * ui_rendering) {
  
  ui_rendering_movie_t * ui_rendering_movie;
  gchar * temp_string = NULL;
  GtkWidget * dialog;
  GtkWidget * packing_table;
  GtkWidget * hseparator;
  GtkWidget * label;
  GtkWidget * entry;
  GtkWidget * radio_button1;
  GtkWidget * radio_button2;
  guint table_row = 0;
  AmitkAxis i_axis;
  renderings_t * contexts;
  AmitkDataSet * temp_ds;
  gboolean valid;
  gint temp_end_frame;
  amide_time_t temp_end_time;
  amide_time_t temp_start_time;
  
  if (ui_rendering->movie != NULL)
    return ui_rendering->movie;

  ui_rendering_movie = movie_init();
  ui_rendering->movie = ui_rendering_movie;
  ui_rendering_movie->ui_rendering = ui_rendering;


  ui_rendering_movie->start_frame = 0;

  /* try to get reasonable default values */
  contexts = ui_rendering->contexts;
  valid = FALSE;
  while (contexts != NULL) {
    if (AMITK_IS_DATA_SET(contexts->context->object)) {
      temp_ds = AMITK_DATA_SET(contexts->context->object);
      temp_end_frame = AMITK_DATA_SET_NUM_FRAMES(temp_ds)-1;
      temp_start_time = amitk_data_set_get_start_time(temp_ds,0);
      temp_end_time = amitk_data_set_get_end_time(temp_ds, temp_end_frame);
      if (!valid) {
	ui_rendering_movie->end_frame = temp_end_frame;
	ui_rendering_movie->start_time = temp_start_time;
	ui_rendering_movie->end_time = temp_end_time;
      } else {
	if (temp_end_frame > ui_rendering_movie->end_frame)
	  ui_rendering_movie->end_frame = temp_end_frame;
	if (temp_start_time < ui_rendering_movie->start_time)
	  ui_rendering_movie->start_time = temp_start_time;
	if (temp_end_time > ui_rendering_movie->end_time)
	  ui_rendering_movie->end_time = temp_end_time;
      }
      valid = TRUE;
    }
    contexts = contexts->next;
  }
    
  temp_string = g_strdup_printf("%s: Rendering Movie Generation Dialog",PACKAGE);
  dialog = gtk_dialog_new_with_buttons(temp_string,  GTK_WINDOW(ui_rendering->app),
				       GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
				       GTK_STOCK_EXECUTE, AMITK_RESPONSE_EXECUTE,
				       GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
				       NULL);
  g_free(temp_string);

  /* setup the callbacks for app */
  g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(response_cb), ui_rendering_movie);
  g_signal_connect(G_OBJECT(dialog), "delete_event", G_CALLBACK(delete_event_cb), ui_rendering_movie);


  /* ---------------------------
     Basic Parameters page 
     --------------------------- */


  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(5,3,FALSE);
  table_row=0;
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), packing_table);

  /* widgets to specify how many frames */
  label = gtk_label_new("Movie Duration (sec)");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%5.3f", ui_rendering_movie->duration);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(change_frames_cb),  ui_rendering_movie);
  gtk_table_attach(GTK_TABLE(packing_table), entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator, 0,3,
		   table_row, table_row+1,GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* widgets to specify number of rotations on the axis */
  for (i_axis=0;i_axis<AMITK_AXIS_NUM;i_axis++) {

    temp_string = g_strdup_printf("Rotations on %s", amitk_axis_get_name(i_axis));
    label = gtk_label_new(temp_string);
    g_free(temp_string);
    gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

    entry = gtk_entry_new();
    temp_string = g_strdup_printf("%f", ui_rendering_movie->rotation[i_axis]);
    gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
    g_free(temp_string);
    gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
    g_object_set_data(G_OBJECT(entry), "which_entry", GINT_TO_POINTER(i_axis));
    g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(change_rotation_cb), ui_rendering_movie);
    gtk_table_attach(GTK_TABLE(packing_table), entry,1,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    table_row++;
  }

  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator, 0,3,
		   table_row, table_row+1,GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* do we want to make a movie over time or over frames */
  label = gtk_label_new("Dynamic Movie:");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  /* the radio buttons */
  radio_button1 = gtk_radio_button_new_with_label(NULL, "over time");
  gtk_table_attach(GTK_TABLE(packing_table), radio_button1,
  		   1,2, table_row, table_row+1,
  		   0, 0, X_PADDING, Y_PADDING);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button1), TRUE);

  g_object_set_data(G_OBJECT(radio_button1), "dynamic_type", GINT_TO_POINTER(OVER_TIME));

  radio_button2 = gtk_radio_button_new_with_label(NULL, "over frames");
  gtk_radio_button_set_group(GTK_RADIO_BUTTON(radio_button2), 
			     gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_button1)));
  gtk_table_attach(GTK_TABLE(packing_table), radio_button2,
  		   2,3, table_row, table_row+1,
  		   0, 0, X_PADDING, Y_PADDING);
  g_object_set_data(G_OBJECT(radio_button2), "dynamic_type", GINT_TO_POINTER(OVER_FRAMES));

  g_signal_connect(G_OBJECT(radio_button1), "clicked", G_CALLBACK(dynamic_type_cb), ui_rendering_movie);
  g_signal_connect(G_OBJECT(radio_button2), "clicked", G_CALLBACK(dynamic_type_cb), ui_rendering_movie);

  table_row++;

  /* widgets to specify the start and end times */
  ui_rendering_movie->start_time_label = gtk_label_new("Start Time (s)");
  gtk_table_attach(GTK_TABLE(packing_table), ui_rendering_movie->start_time_label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  ui_rendering_movie->start_frame_label = gtk_label_new("Start Frame");
  gtk_table_attach(GTK_TABLE(packing_table), ui_rendering_movie->start_frame_label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  ui_rendering_movie->start_time_entry = gtk_entry_new();
  temp_string = g_strdup_printf("%5.3f", ui_rendering_movie->start_time);
  gtk_entry_set_text(GTK_ENTRY(ui_rendering_movie->start_time_entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(ui_rendering_movie->start_time_entry), TRUE);
  g_signal_connect(G_OBJECT(ui_rendering_movie->start_time_entry), "changed", 
		   G_CALLBACK(change_start_time_cb), ui_rendering_movie);
  gtk_table_attach(GTK_TABLE(packing_table), ui_rendering_movie->start_time_entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  ui_rendering_movie->start_frame_entry = gtk_entry_new();
  temp_string = g_strdup_printf("%d", ui_rendering_movie->start_frame);
  gtk_entry_set_text(GTK_ENTRY(ui_rendering_movie->start_frame_entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(ui_rendering_movie->start_frame_entry), TRUE);
  g_signal_connect(G_OBJECT(ui_rendering_movie->start_frame_entry), "changed", 
		   G_CALLBACK(change_start_frame_cb), ui_rendering_movie);
  gtk_table_attach(GTK_TABLE(packing_table), ui_rendering_movie->start_frame_entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  ui_rendering_movie->end_time_label = gtk_label_new("End Time (s)");
  gtk_table_attach(GTK_TABLE(packing_table), ui_rendering_movie->end_time_label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  ui_rendering_movie->end_frame_label = gtk_label_new("End Frame");
  gtk_table_attach(GTK_TABLE(packing_table), ui_rendering_movie->end_frame_label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);


  ui_rendering_movie->end_time_entry = gtk_entry_new();
  temp_string = g_strdup_printf("%5.3f", ui_rendering_movie->end_time);
  gtk_entry_set_text(GTK_ENTRY(ui_rendering_movie->end_time_entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(ui_rendering_movie->end_time_entry), TRUE);
  g_signal_connect(G_OBJECT(ui_rendering_movie->end_time_entry), "changed", 
		   G_CALLBACK(change_end_time_cb), ui_rendering_movie);
  gtk_table_attach(GTK_TABLE(packing_table), ui_rendering_movie->end_time_entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  ui_rendering_movie->end_frame_entry = gtk_entry_new();
  temp_string = g_strdup_printf("%d", ui_rendering_movie->end_frame);
  gtk_entry_set_text(GTK_ENTRY(ui_rendering_movie->end_frame_entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(ui_rendering_movie->end_frame_entry), TRUE);
  g_signal_connect(G_OBJECT(ui_rendering_movie->end_frame_entry), "changed", 
		   G_CALLBACK(change_end_frame_cb), ui_rendering_movie);
  gtk_table_attach(GTK_TABLE(packing_table), ui_rendering_movie->end_frame_entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator, 0,3,
		   table_row, table_row+1,GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* progress bar */
  label = gtk_label_new("Progress");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  ui_rendering_movie->progress = gtk_progress_bar_new();
  gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(ui_rendering_movie->progress), GTK_PROGRESS_LEFT_TO_RIGHT);
  gtk_table_attach(GTK_TABLE(packing_table), ui_rendering_movie->progress,1,3,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* show all our widgets */
  gtk_widget_show_all(dialog);

  /* and hide the appropriate widgets */
  gtk_widget_hide(ui_rendering_movie->start_frame_label);
  gtk_widget_hide(ui_rendering_movie->start_frame_entry);
  gtk_widget_hide(ui_rendering_movie->end_frame_label);
  gtk_widget_hide(ui_rendering_movie->end_frame_entry);

  return ui_rendering_movie;
}


#endif /* AMIDE_LIBFAME_SUPPORT */
#endif /* LIBVOLPACK_SUPPORT */
