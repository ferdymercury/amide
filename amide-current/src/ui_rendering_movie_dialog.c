/* ui_rendering_movie_dialog.c
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
#ifdef AMIDE_MPEG_ENCODE_SUPPORT

#include <gnome.h>
#include <math.h>
#include <sys/stat.h>
#include "ui_common.h"
#include "ui_rendering_movie_dialog.h"
#include "ui_rendering_movie_dialog_cb.h"


/* destroy a ui_rendering_movie data structure */
ui_rendering_movie_t * ui_rendering_movie_free(ui_rendering_movie_t * ui_rendering_movie) {

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
ui_rendering_movie_t * ui_rendering_movie_add_reference(ui_rendering_movie_t * movie) {

  g_return_val_if_fail(movie != NULL, NULL);

  movie->reference_count++;

  return movie;
}


/* malloc and initialize a ui_rendering_movie data structure */
ui_rendering_movie_t * ui_rendering_movie_init(void) {

  ui_rendering_movie_t * ui_rendering_movie;


  /* alloc space for the data structure */
  if ((ui_rendering_movie = (ui_rendering_movie_t *) g_malloc(sizeof(ui_rendering_movie_t))) == NULL) {
    g_warning("%s: couldn't allocate space for ui_rendering_movie_t",PACKAGE);
    return NULL;
  }

  ui_rendering_movie->reference_count = 1;
  ui_rendering_movie->dialog = NULL;
  ui_rendering_movie->num_frames = MOVIE_DEFAULT_FRAMES;
  ui_rendering_movie->rotation[XAXIS] = 0;
  ui_rendering_movie->rotation[YAXIS] = 1;
  ui_rendering_movie->rotation[ZAXIS] = 0;
  ui_rendering_movie->type = OVER_TIME;
  ui_rendering_movie->ui_rendering = NULL;
  ui_rendering_movie->start_time = 0.0;
  ui_rendering_movie->end_time = 1.0;
  ui_rendering_movie->start_frame = 0;
  ui_rendering_movie->end_frame = 0;

 return ui_rendering_movie;
}




/* perform the movie generation */
void ui_rendering_movie_dialog_perform(ui_rendering_movie_t * ui_rendering_movie, gchar * output_file_name) {

  guint i_frame;
  gdouble rotation_step[NUM_AXIS];
  axis_t i_axis;
  gint return_val = 1;
   FILE * param_file;
  GTimeVal current_time;
  guint i;
  gchar * frame_file_name = NULL;
  gchar * param_file_name = NULL;
  gchar * temp_dir = NULL;
  gchar * temp_string;
  struct stat file_info;
  GList * file_list = NULL;
  GList * temp_list;
  GdkImlibImage * ppm_image;
  amide_time_t initial_start, initial_duration;
  ui_rendering_t * ui_rendering;
  volume_t * most_frames_volume=NULL;
  renderings_t * contexts;
  guint volume_frame=0;

  /* figure out what the temp directory is */
  temp_dir = g_get_tmp_dir();

  /* add a reference, we do this 'cause if there's only one reference left (i.e. this one), we 
     know that the user has hit a cancel button */
  ui_rendering_movie = ui_rendering_movie_add_reference(ui_rendering_movie);

  /* get the current time, this is so we have a, "hopefully" unique file name */
  g_get_current_time(&current_time);

  /* save the initial times so we can set it back later */
  ui_rendering = ui_rendering_movie->ui_rendering;
  initial_start = ui_rendering->start;
  initial_duration = ui_rendering->duration;
  
  /* figure out which volume has the most frames, need this if we're doing a 
     movie over frames */
  contexts = ui_rendering->contexts;
  most_frames_volume = contexts->rendering_context->volume;
  contexts = contexts->next;
  while (contexts != NULL) {
    if (contexts->rendering_context->volume->data_set->dim.t >
	most_frames_volume->data_set->dim.t)
      most_frames_volume = contexts->rendering_context->volume;
    contexts = contexts->next;
  }

  /* figure out each frame's duration, needed if we're doing a movie over time */
  ui_rendering->duration = 
    (ui_rendering_movie->end_time-ui_rendering_movie->start_time)
    /((amide_time_t) ui_rendering_movie->num_frames);

#ifdef AMIDE_DEBUG
  g_print("Total number of movie frames to do:\t%d\n",ui_rendering_movie->num_frames);
  g_print("Frame:\n");
#endif
  

  /* start generating the frames */
  for (i_frame = 0; i_frame < ui_rendering_movie->num_frames; i_frame++) {
    if ((return_val == 1) && (ui_rendering_movie->reference_count > 1)) { 
      /* continue if good so far and we haven't closed the dialog box */

      /* figure out the rotation for this frame */
      for (i_axis = 0; i_axis < NUM_AXIS ; i_axis++) {
	rotation_step[i_axis] = 
	  (((double) i_frame)*2.0*M_PI*ui_rendering_movie->rotation[i_axis])
	  / ui_rendering_movie->num_frames;
	renderings_set_rotation(ui_rendering->contexts, i_axis, rotation_step[i_axis]);
      }

      /* figure out the start interval for this frame */
      if (ui_rendering_movie->type == OVER_TIME) {
	ui_rendering->start = ui_rendering_movie->start_time + i_frame*ui_rendering->duration;
      }  else {
	volume_frame = floor((i_frame/((float) ui_rendering_movie->num_frames)
			      * most_frames_volume->data_set->dim.t));
	ui_rendering->start = volume_start_time(most_frames_volume, volume_frame)+CLOSE;
	ui_rendering->duration = 
	  volume_end_time(most_frames_volume, volume_frame) -ui_rendering->start-CLOSE;
      }
 

#ifdef AMIDE_DEBUG
      if (ui_rendering_movie->type == OVER_TIME)
	g_print("\tRendering\t%d\tstart-end time %5.3f-%5.3f\trotation x %5.3f y %5.3f z %5.3f\n",
		i_frame, ui_rendering->start, ui_rendering->start+ui_rendering->duration,
		rotation_step[XAXIS], rotation_step[YAXIS], rotation_step[ZAXIS]);
      else
	g_print("\tRendering\t%d\tdata frame %d\trotation x %5.3f y %5.3f z %5.3f\n",
		i_frame, volume_frame, rotation_step[XAXIS], rotation_step[YAXIS], rotation_step[ZAXIS]);
#endif
      
      /* render the contexts */
      ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_rendering->canvas));
      ui_rendering_update_canvases(ui_rendering); 
      ui_common_remove_cursor(GTK_WIDGET(ui_rendering->canvas));
      
#ifdef AMIDE_DEBUG
      g_print("\tWriting\t\t%d\n",i_frame);
#endif

      i = 0;
      do {
	if (i > 0) g_free(frame_file_name);
	i++;
	frame_file_name = g_strdup_printf("%s/%ld_%d_amide_rendering_%d.ppm",
					  temp_dir, current_time.tv_sec, i, i_frame);
      } while (stat(frame_file_name, &file_info) == 0);
      
      /* yep, we still need to use imlib to export ppm files, as gdk_pixbuf doesn't seem to have
	 this capability */
      ppm_image = 
	gdk_imlib_create_image_from_data(gdk_pixbuf_get_pixels(ui_rendering->rgb_image), 
					 NULL,
					 gdk_pixbuf_get_width(ui_rendering->rgb_image), 
					 gdk_pixbuf_get_height(ui_rendering->rgb_image));

      return_val = gdk_imlib_save_image_to_ppm(ppm_image, frame_file_name);
      gdk_imlib_destroy_image(ppm_image);

      
      if (return_val != 1) 
	g_warning("%s: saving of following ppm file failed: %s\n\tAborting movie generation",
		  PACKAGE, frame_file_name);
      else
	file_list = g_list_append(file_list, frame_file_name);
      
      /* do any events pending, this allows the canvas to get displayed */
      while (gtk_events_pending()) 
	gtk_main_iteration();
      
      /* and unrotate the rendering contexts so that we can properly rerotate
	 for the next frame */
      for (i_axis = NUM_AXIS; i_axis > 0 ; i_axis--) {
	renderings_set_rotation(ui_rendering->contexts, 
				i_axis-1, -rotation_step[i_axis-1]);
      }
      
      /* update the progress bar */
      gtk_progress_set_percentage(GTK_PROGRESS(ui_rendering_movie->progress),
				  (gfloat) i_frame/((gfloat) ui_rendering_movie->num_frames*2.0));
    }
  }



  /* do the mpeg stuff if no errors so far and we haven't closed the dialog box */
  if ((return_val == 1) && (ui_rendering_movie->reference_count > 1)) { 
#ifdef AMIDE_DEBUG
    g_print("Encoding the MPEG\n");
#endif

    i = 0;
    do { /* get a unique file name */
      if (i > 0) g_free(param_file_name); 
      i++;
      param_file_name = g_strdup_printf("%s/%ld_%d_amide_rendering.param",
					temp_dir, current_time.tv_sec, i);
    } while (stat(param_file_name, &file_info) == 0);
    
    /* open the parameter file for writing */
    if ((param_file = fopen(param_file_name, "w")) != NULL) {
      fprintf(param_file, "PATTERN\tIBBPBBPBBPBBPBBP\n");
      fprintf(param_file, "OUTPUT\t%s\n",output_file_name);
      fprintf(param_file, "INPUT_DIR\t%s\n","");
      fprintf(param_file, "INPUT\n");
      
      temp_list = file_list;
      while (temp_list != NULL) {
	frame_file_name = temp_list->data;
	fprintf(param_file, "%s\n",frame_file_name);
	temp_list = temp_list->next;
      }
      fprintf(param_file, "END_INPUT\n");
      fprintf(param_file, "BASE_FILE_FORMAT\tPPM\n");
      fprintf(param_file, "INPUT_CONVERT\t*\n");
      fprintf(param_file, "GOP_SIZE\t16\n");
      fprintf(param_file, "SLICES_PER_FRAME\t1\n");
      fprintf(param_file, "PIXEL\tHALF\n");
      fprintf(param_file, "RANGE\t10\n");
      fprintf(param_file, "PSEARCH_ALG\tEXHAUSTIVE\n");
      fprintf(param_file, "BSEARCH_ALG\tCROSS2\n");
      fprintf(param_file, "IQSCALE\t8\n");
      fprintf(param_file, "PQSCALE\t10\n");
      fprintf(param_file, "BQSCALE\t25\n");
      fprintf(param_file, "BQSCALE\t25\n");
      fprintf(param_file, "REFERENCE_FRAME\tDECODED\n");
      fprintf(param_file, "FRAME_RATE\t30\n");
      
      file_list = g_list_append(file_list, param_file_name);
      fclose(param_file);

      /* delete the previous .mpg file if it existed */
      if (stat(output_file_name, &file_info) == 0) {
	if (S_ISREG(file_info.st_mode)) {
	  if (unlink(output_file_name) != 0) {
	    g_warning("%s: Couldn't unlink file: %s",PACKAGE, output_file_name);
	    return_val = -1;
	  }
	} else {
	  g_warning("%s: Unrecognized file type for file: %s, couldn't delete",
		    PACKAGE, output_file_name);
	  return_val = -1;
	}
      }
      
      if (return_val != -1) {
	/* and run the mpeg encoding process */
	temp_string = g_strdup_printf("%s -quiet 1 -no_frame_summary %s",AMIDE_MPEG_ENCODE_BIN, param_file_name);
	ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_rendering->canvas));
	return_val = system(temp_string);
	ui_common_remove_cursor(GTK_WIDGET(ui_rendering->canvas));
	g_free(temp_string);
	
	if ((return_val == -1) || (return_val == 127)) {
	  g_warning("%s: executing of %s for creation of mpeg movie was unsucessful", 
		    PACKAGE, AMIDE_MPEG_ENCODE_BIN);
	  return_val = -1;
	}
      }
      
      /* update the progress bar */
      gtk_progress_set_percentage(GTK_PROGRESS(ui_rendering_movie->progress), 1.0);
    }
  }


  /* do some cleanups */
  /* note, temp_dir is just a pointer to a string, not a copy, so don't free it */

  while (file_list != NULL) {
    frame_file_name = file_list->data;
    file_list = g_list_remove(file_list, frame_file_name);
    unlink(frame_file_name);
    g_free(frame_file_name);
  }
    
  /* and rerender one last time to back to the initial rotation and time */
  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_rendering->canvas));
  ui_rendering->start = initial_start;
  ui_rendering->duration = initial_duration;
  ui_rendering_update_canvases(ui_rendering); 
  ui_common_remove_cursor(GTK_WIDGET(ui_rendering->canvas));

  /* and remove the reference we added here*/
  ui_rendering_movie = ui_rendering_movie_free(ui_rendering_movie);

  return;
}

/* function that sets up the rendering options dialog */
ui_rendering_movie_t * ui_rendering_movie_dialog_create(ui_rendering_t * ui_rendering) {
  
  ui_rendering_movie_t * ui_rendering_movie;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * hseparator;
  GtkWidget * label;
  GtkWidget * entry;
  GtkWidget * radio_button1;
  GtkWidget * radio_button2;
  guint table_row = 0;
  axis_t i_axis;
  renderings_t * contexts;
  amide_time_t temp_time;
  
  if (ui_rendering->movie != NULL)
    return ui_rendering->movie;

  ui_rendering_movie = ui_rendering_movie_init();
  ui_rendering->movie = ui_rendering_movie;
  ui_rendering_movie->ui_rendering = ui_rendering;

  contexts = ui_rendering->contexts;

  ui_rendering_movie->start_frame = 0;
  ui_rendering_movie->end_frame = contexts->rendering_context->volume->data_set->dim.t-1;
  ui_rendering_movie->start_time = volume_start_time(contexts->rendering_context->volume,0);
  ui_rendering_movie->end_time = volume_end_time(contexts->rendering_context->volume,
						 contexts->rendering_context->volume->data_set->dim.t-1);
  contexts = contexts->next;
  while (contexts != NULL) {
    if (contexts->rendering_context->volume->data_set->dim.t-1 > ui_rendering_movie->end_frame)
      ui_rendering_movie->end_frame = contexts->rendering_context->volume->data_set->dim.t-1;
    temp_time = volume_start_time(contexts->rendering_context->volume,0);
    if (temp_time < ui_rendering_movie->start_time)
      ui_rendering_movie->start_time = temp_time;
    temp_time = volume_end_time(contexts->rendering_context->volume,
				contexts->rendering_context->volume->data_set->dim.t-1);
    if (temp_time > ui_rendering_movie->end_time)
      ui_rendering_movie->end_time = temp_time;
    contexts = contexts->next;
  }
    
  temp_string = g_strdup_printf("%s: Rendering Movie Generation Dialog",PACKAGE);
  ui_rendering_movie->dialog = gnome_property_box_new();
  gtk_window_set_title(GTK_WINDOW(ui_rendering_movie->dialog), temp_string);
  g_free(temp_string);

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(ui_rendering_movie->dialog), "close",
  		     GTK_SIGNAL_FUNC(ui_rendering_movie_dialog_cb_close),
  		     ui_rendering_movie);
  gtk_signal_connect(GTK_OBJECT(ui_rendering_movie->dialog), "apply",
  		     GTK_SIGNAL_FUNC(ui_rendering_movie_dialog_cb_apply), 
		     ui_rendering_movie);
  gtk_signal_connect(GTK_OBJECT(ui_rendering_movie->dialog), "help",
		     GTK_SIGNAL_FUNC(ui_rendering_movie_dialog_cb_help),
		     ui_rendering_movie);

  /* not operating as a true property box dialog, i.e. no ok option */
  /* having an ok button get's really confusing, as we get a close signal in addition
     to an apply signal with the ok button */
  gtk_widget_destroy(GNOME_PROPERTY_BOX(ui_rendering_movie->dialog)->ok_button);


  /* ---------------------------
     Basic Parameters page 
     --------------------------- */


  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(5,3,FALSE);
  label = gtk_label_new("Movie Parameters");
  table_row=0;
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(ui_rendering_movie->dialog),  packing_table, label);

  /* widgets to specify how many frames */
  label = gtk_label_new("Movie Frames");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%d", ui_rendering_movie->num_frames);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_rendering_movie_dialog_cb_change_frames), 
		     ui_rendering_movie);
  gtk_table_attach(GTK_TABLE(packing_table), entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator, 0,3,
		   table_row, table_row+1,GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* widgets to specify number of rotations on the axis */
  for (i_axis=0;i_axis<NUM_AXIS;i_axis++) {
    temp_string = g_strdup_printf("Rotations on %s", axis_names[i_axis]);
    label = gtk_label_new(temp_string);
    g_free(temp_string);
    gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

    entry = gtk_entry_new();
    temp_string = g_strdup_printf("%f", ui_rendering_movie->rotation[i_axis]);
    gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
    g_free(temp_string);
    gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
    gtk_object_set_data(GTK_OBJECT(entry), "which_entry", GINT_TO_POINTER(i_axis));
    gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		       GTK_SIGNAL_FUNC(ui_rendering_movie_dialog_cb_change_rotation), 
		       ui_rendering_movie);
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

  gtk_object_set_data(GTK_OBJECT(radio_button1), "dynamic_type", GINT_TO_POINTER(OVER_TIME));

  radio_button2 = gtk_radio_button_new_with_label(NULL, "over frames");
  gtk_radio_button_set_group(GTK_RADIO_BUTTON(radio_button2), 
			     gtk_radio_button_group(GTK_RADIO_BUTTON(radio_button1)));
  gtk_table_attach(GTK_TABLE(packing_table), radio_button2,
  		   2,3, table_row, table_row+1,
  		   0, 0, X_PADDING, Y_PADDING);
  gtk_object_set_data(GTK_OBJECT(radio_button2), "dynamic_type", GINT_TO_POINTER(OVER_FRAMES));

  gtk_signal_connect(GTK_OBJECT(radio_button1), "clicked",  
		     GTK_SIGNAL_FUNC(ui_rendering_movie_dialog_cb_dynamic_type), ui_rendering_movie);
  gtk_signal_connect(GTK_OBJECT(radio_button2), "clicked",  
		     GTK_SIGNAL_FUNC(ui_rendering_movie_dialog_cb_dynamic_type), ui_rendering_movie);

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
  gtk_signal_connect(GTK_OBJECT(ui_rendering_movie->start_time_entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_rendering_movie_dialog_cb_change_start_time), 
		     ui_rendering_movie);
  gtk_table_attach(GTK_TABLE(packing_table), ui_rendering_movie->start_time_entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  ui_rendering_movie->start_frame_entry = gtk_entry_new();
  temp_string = g_strdup_printf("%d", ui_rendering_movie->start_frame);
  gtk_entry_set_text(GTK_ENTRY(ui_rendering_movie->start_frame_entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(ui_rendering_movie->start_frame_entry), TRUE);
  gtk_signal_connect(GTK_OBJECT(ui_rendering_movie->start_frame_entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_rendering_movie_dialog_cb_change_start_frame), 
		     ui_rendering_movie);
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
  gtk_signal_connect(GTK_OBJECT(ui_rendering_movie->end_time_entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_rendering_movie_dialog_cb_change_end_time), 
		     ui_rendering_movie);
  gtk_table_attach(GTK_TABLE(packing_table), ui_rendering_movie->end_time_entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  ui_rendering_movie->end_frame_entry = gtk_entry_new();
  temp_string = g_strdup_printf("%d", ui_rendering_movie->end_frame);
  gtk_entry_set_text(GTK_ENTRY(ui_rendering_movie->end_frame_entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(ui_rendering_movie->end_frame_entry), TRUE);
  gtk_signal_connect(GTK_OBJECT(ui_rendering_movie->end_frame_entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_rendering_movie_dialog_cb_change_end_frame), 
		     ui_rendering_movie);
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
  gtk_widget_show_all(ui_rendering_movie->dialog);

  /* and hide the appropriate widgets */
  gtk_widget_hide(ui_rendering_movie->start_frame_label);
  gtk_widget_hide(ui_rendering_movie->start_frame_entry);
  gtk_widget_hide(ui_rendering_movie->end_frame_label);
  gtk_widget_hide(ui_rendering_movie->end_frame_entry);

  /* tell the dialog we've changed */
  gnome_property_box_changed(GNOME_PROPERTY_BOX(ui_rendering_movie->dialog));

  return ui_rendering_movie;
}

#endif /* AMIDE_MPEG_ENCODE_SUPPORT */
#endif /* LIBVOLPACK_SUPPORT */










