/* ui_series.c
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
#include <gnome.h>
#include <math.h>
#include <gdk-pixbuf/gnome-canvas-pixbuf.h>
#include "study.h"
#include "image.h"
#include "ui_common.h"
#include "ui_series.h"
#include "ui_series_cb.h"
#include "ui_series_menus.h"
#include "ui_series_toolbar.h"

/* external variables */
gchar * series_names[] = {"over Space", "over Time"};

/* destroy the ui_series slices data */
static void ui_series_slices_free(ui_series_t * ui_series) {
  guint i;

  for (i=0; i < ui_series->num_slices; i++) {
    ui_series->slices[i] = volume_list_free(ui_series->slices[i]);
    if (ui_series->rgb_images[i] != NULL)
      gdk_pixbuf_unref(ui_series->rgb_images[i]);
  }
  g_free(ui_series->slices);
  ui_series->slices = NULL;

  return;
}

/* destroy a ui_series data structure */
ui_series_t * ui_series_free(ui_series_t * ui_series) {

  if (ui_series == NULL)
    return ui_series;

  /* sanity checks */
  g_return_val_if_fail(ui_series->reference_count > 0, NULL);

  /* remove a reference count */
  ui_series->reference_count--;

  /* things to do if we've removed all reference's */
  if (ui_series->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing ui_series\n");
#endif
    ui_series_slices_free(ui_series);
    ui_series->volumes = volume_list_free(ui_series->volumes);
    g_free(ui_series->rgb_images);
    g_free(ui_series->images);
    g_free(ui_series->captions);
    g_free(ui_series->frame_durations);
    g_free(ui_series);
    ui_series = NULL;
  }

  return ui_series;

}

/* malloc and initialize a ui_series data structure */
static ui_series_t * ui_series_init(void) {

  ui_series_t * ui_series;

  /* alloc space for the data structure for passing ui info */
  if ((ui_series = (ui_series_t *) g_malloc(sizeof(ui_series_t))) == NULL) {
    g_warning("%s: couldn't allocate space for ui_series_t",PACKAGE);
    return NULL;
  }
  ui_series->reference_count = 1;

  /* set any needed parameters */
  ui_series->slices = NULL;
  ui_series->num_slices = 0;
  ui_series->rows = 0;
  ui_series->columns = 0;
  ui_series->images = NULL;
  ui_series->captions = NULL;
  ui_series->rgb_images = NULL;
  ui_series->volumes = NULL;
  ui_series->interpolation = NEAREST_NEIGHBOR;
  ui_series->zoom = 1.0;
  ui_series->thickness = -1.0;
  ui_series->view_point = realpoint_zero;
  ui_series->view_time = 0.0;
  ui_series->type = PLANES;
  ui_series->thresholds_dialog = NULL;

  ui_series->view_duration = 1.0;
  ui_series->start_z = 0.0;
  ui_series->end_z = 0.0;

  ui_series->view_frame = 0;
  ui_series->start_time = 0.0;
  ui_series->frame_durations = NULL;

  return ui_series;
}


/* function to make the adjustments for the scrolling scale */
static GtkAdjustment * ui_series_create_scroll_adjustment(ui_series_t * ui_series) { 

  if (ui_series->type == PLANES) {
    realpoint_t view_corner[2];

    volumes_get_view_corners(ui_series->volumes, ui_series->coord_frame, view_corner);
    
    return GTK_ADJUSTMENT(gtk_adjustment_new(ui_series->view_point.z,
					     view_corner[0].z,
					     view_corner[1].z,
					     ui_series->thickness,
					     ui_series->thickness,
					     ui_series->thickness));
  } else { /* FRAMES */
    return GTK_ADJUSTMENT(gtk_adjustment_new(ui_series->view_frame, 0, ui_series->num_slices, 1,1,1));
  }
}

/* funtion to update the canvas */
void ui_series_update_canvas(ui_series_t * ui_series) {

  realpoint_t temp_point;
  amide_time_t temp_time, temp_duration;
  gint i, start_i;
  gint width, height;
  gdouble x, y;
  GtkRequisition requisition;
  realspace_t view_coord_frame;
  gint image_width, image_height;
  gchar * temp_string;


  /* get the view axis to use*/
  width = 0.9*gdk_screen_width();
  height = 0.9*gdk_screen_height();

  /* allocate space for pointers to our slices if needed */
  if (ui_series->slices == NULL) {
    if ((ui_series->slices = 
	 (volume_list_t **) g_malloc(sizeof(volume_list_t *) *ui_series->num_slices)) == NULL) {
      g_warning("%s: couldn't allocate space for pointers to amide_volume_t's", PACKAGE);
      return;
    }
    for (i=0; i < ui_series->num_slices ; i++)
      ui_series->slices[i] = NULL;
  }
    
  /* allocate space for pointers to our rgb_images if needed */
  if (ui_series->rgb_images == NULL) {
    if ((ui_series->rgb_images = (GdkPixbuf **) g_malloc(sizeof(GdkPixbuf *)*ui_series->num_slices)) == NULL) {
      g_warning("%s: couldn't allocate space for pointers to GdkPixbuf's", PACKAGE);
      return;
    }
    for (i=0; i < ui_series->num_slices ; i++)
      ui_series->rgb_images[i] = NULL;
  }

  /* always do the first image, so that we can get the width and height
     of an image (needed for computing the rows and columns portions
     of the ui_series data structure */
  if (ui_series->rgb_images[0] != NULL) 
    gdk_pixbuf_unref(ui_series->rgb_images[0]);

  temp_point = ui_series->view_point;
  if (ui_series->type == PLANES) {
    temp_point.z = ui_series->start_z;
    temp_time =  ui_series->view_time;
    temp_duration = ui_series->view_duration;
  } else {
    temp_time = ui_series->start_time;
    temp_duration = ui_series->frame_durations[0];
  }
  view_coord_frame = ui_series->coord_frame;
  rs_set_offset(&view_coord_frame, realspace_alt_coord_to_base(temp_point, ui_series->coord_frame));
  ui_series->rgb_images[0] = image_from_volumes(&(ui_series->slices[0]),
						ui_series->volumes,
						temp_time+CLOSE,
						temp_duration-CLOSE,
						ui_series->thickness,
						view_coord_frame,
						ui_series->scaling,
						ui_series->zoom,
						ui_series->interpolation);
  image_width = gdk_pixbuf_get_width(ui_series->rgb_images[0]) + UI_SERIES_R_MARGIN + UI_SERIES_L_MARGIN;
  image_height = gdk_pixbuf_get_height(ui_series->rgb_images[0]) 
    + UI_SERIES_TOP_MARGIN + UI_SERIES_BOTTOM_MARGIN;


  /* allocate space for pointers to our canvas_images and captions, if needed */
  if (ui_series->images == NULL) {
    /* figure out how many images we can display at once */
    ui_series->columns = floor(width/image_width);
    if (ui_series->columns < 1) ui_series->columns = 1;
    ui_series->rows = floor(height/image_height);
    if (ui_series->rows < 1) ui_series->rows = 1;

    /* compensate for cases where we don't need all the rows */
    if ((ui_series->columns * ui_series->rows) > ui_series->num_slices)
      ui_series->rows = ceil((double) ui_series->num_slices/(double) ui_series->columns);

    if ((ui_series->images = (GnomeCanvasItem **) g_malloc(sizeof(GnomeCanvasItem *) * ui_series->rows
							   * ui_series->columns)) == NULL) {
      g_warning("%s: couldn't allocate space for pointers to image GnomeCanvasItem's",PACKAGE);
      return;
    }
    if ((ui_series->captions = (GnomeCanvasItem **) g_malloc(sizeof(GnomeCanvasItem *) * ui_series->rows
							     * ui_series->columns)) == NULL) {
      g_warning("%s: couldn't allocate space for pointers to caption GnomeCanavasItem's",PACKAGE);
      return;
    }
    for (i=0; i < ui_series->columns * ui_series->rows ; i++) {
      ui_series->images[i] = NULL;
      ui_series->captions[i] = NULL;
    }
  }


  /* figure out what's the first image we want to display */
  if (ui_series->type == PLANES) 
    start_i = ui_series->num_slices*((ui_series->view_point.z-ui_series->start_z)/
			       (ui_series->end_z-ui_series->start_z));
  else  /* FRAMES */
    start_i = ui_series->view_frame;

  /* correct i for special cases */
  if (ui_series->num_slices < ui_series->columns*ui_series->rows)
    start_i=0;
  else if (start_i < (ui_series->columns*ui_series->rows/2.0))
    start_i=0;
  else if (start_i > (ui_series->num_slices - ui_series->columns*ui_series->rows/2.0))
    start_i = ui_series->num_slices - ui_series->columns*ui_series->rows;
  else
    start_i = start_i-ui_series->columns*ui_series->rows/2.0;

  temp_time = ui_series->start_time;
  temp_point = ui_series->view_point;
  if (ui_series->type == PLANES) {
    temp_time = ui_series->view_time;
    temp_duration = ui_series->view_duration;
  } else { /* frames */
    for (i=0;i< start_i; i++)
      temp_time += ui_series->frame_durations[i];
    temp_time -= ui_series->frame_durations[start_i];/* well get added back 1st time through loop */
    temp_duration = ui_series->frame_durations[start_i]; 
  }
  x = y = 0.0;

  for (i=start_i; ((i-start_i) < (ui_series->rows*ui_series->columns)) && (i < ui_series->num_slices); i++) {
    if (ui_series->type == PLANES) {
      temp_point.z = i*ui_series->thickness+ui_series->start_z;
    } else { /* frames */
      temp_time += temp_duration; /* duration from last time through the loop */
      temp_duration = ui_series->frame_durations[i];
    }
    view_coord_frame = ui_series->coord_frame;
    rs_set_offset(&view_coord_frame,  realspace_alt_coord_to_base(temp_point, ui_series->coord_frame));
    
    if (i != 0) /* we've already done image 0 */
      if (ui_series->rgb_images[i] != NULL)
	gdk_pixbuf_unref(ui_series->rgb_images[i]);
    
    if (i != 0) /* we've already done image 0 */
      ui_series->rgb_images[i] = image_from_volumes(&(ui_series->slices[i]),
						    ui_series->volumes,
						    temp_time+CLOSE,
						    temp_duration-CLOSE,
						    ui_series->thickness,
						    view_coord_frame,
						    ui_series->scaling,
						    ui_series->zoom,
						    ui_series->interpolation);
    
    /* figure out the next x,y spot to put this guy */
    y = floor((i-start_i)/ui_series->columns)*image_height;
    x = (i-start_i-ui_series->columns*floor((i-start_i)/ui_series->columns))*image_width;
    
    if (ui_series->images[i-start_i] == NULL) 
      ui_series->images[i-start_i] = gnome_canvas_item_new(gnome_canvas_root(ui_series->canvas),
							   gnome_canvas_pixbuf_get_type(),
							   "pixbuf", ui_series->rgb_images[i],
							   "x", x+UI_SERIES_L_MARGIN,
							   "y", y+UI_SERIES_TOP_MARGIN,
							   NULL);
    else
      gnome_canvas_item_set(ui_series->images[i-start_i], "pixbuf", ui_series->rgb_images[i], NULL);
    
    if (ui_series->type == PLANES)
      temp_string = g_strdup_printf("%2.1f-%2.1f mm", temp_point.z, temp_point.z+ui_series->thickness);
    else /* frames */
      temp_string = g_strdup_printf("%2.1f-%2.1f s", temp_time, temp_time+temp_duration);
    if (ui_series->captions[i-start_i] == NULL) 
      ui_series->captions[i-start_i] =
	gnome_canvas_item_new(gnome_canvas_root(ui_series->canvas),
			      gnome_canvas_text_get_type(),
			      "justification", GTK_JUSTIFY_LEFT,
			      "anchor", GTK_ANCHOR_NORTH_WEST,
			      "text", temp_string,
			      "x", x+UI_SERIES_L_MARGIN,
			      "y", y+image_height-UI_SERIES_BOTTOM_MARGIN,
			      "font", UI_SERIES_CAPTION_FONT, 
			      NULL);
    else
      gnome_canvas_item_set(ui_series->captions[i-start_i],
			    "text", temp_string,
			    NULL);
    g_free(temp_string);
  }

  /* readjust widith and height for what we really used */
  width = ui_series->columns*image_width;
  height =ui_series->rows   *image_height;
  
  
  /* reset the min size of the widget */
  gnome_canvas_set_scroll_region(ui_series->canvas, 0.0, 0.0, 
				 (double) width, (double) height);
    requisition.width = width;
    requisition.height = height;
    gtk_widget_size_request(GTK_WIDGET(ui_series->canvas), &requisition);
    gtk_widget_set_usize(GTK_WIDGET(ui_series->canvas), width, height);
    /* the requisition thing should work.... I'll use usize for now... */


  return;
}

/* function that sets up the series dialog */
void ui_series_create(study_t * study, volume_list_t * volumes, view_t view, series_t series_type) {
 
  ui_series_t * ui_series;
  GnomeApp * app;
  gchar * title = NULL;
  GtkWidget * packing_table;
  GtkAdjustment * adjustment;
  GtkWidget * scale;
  amide_time_t min_duration;

  /* sanity checks */
  if (study_volumes(study) == NULL)
    return;
  if (study_first_volume(study) == NULL)
    return;
  if (volumes == NULL)
    return;

  ui_series = ui_series_init();
  ui_series->type = series_type;

  title = g_strdup_printf("Series: %s (%s - %s)",
			  study_name(study), 
			  view_names[view], 
			  series_names[ui_series->type]);
  app = GNOME_APP(gnome_app_new(PACKAGE, title));
  g_free(title);
  ui_series->app = app;
  gtk_window_set_policy (GTK_WINDOW(ui_series->app), TRUE, TRUE, TRUE);

  /* make a copy of the volumes sent to this series */
  ui_series->volumes = volume_list_copy(volumes);

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(app), "realize", GTK_SIGNAL_FUNC(ui_common_window_realize_cb), NULL);
  gtk_signal_connect(GTK_OBJECT(app), "delete_event",
		     GTK_SIGNAL_FUNC(ui_series_cb_delete_event),
		     ui_series);

  /* save the coord_frame of the series and some other parameters */
  ui_series->coord_frame =  realspace_get_view_coord_frame(study_coord_frame(study), view);
  ui_series->view_point = realspace_alt_coord_to_alt(study_view_center(study),
						     study_coord_frame(study),
						     ui_series->coord_frame);
  ui_series->thickness = study_view_thickness(study);
  ui_series->interpolation = study_interpolation(study);
  ui_series->view_time = study_view_time(study);
  min_duration = volume_list_min_frame_duration(ui_series->volumes);
  ui_series->view_duration =  
    (min_duration > study_view_duration(study)) ?  min_duration : study_view_duration(study);
  ui_series->zoom = study_zoom(study);
  ui_series->scaling = study_scaling(study);


  /* do some initial calculations */
  if (ui_series->type == PLANES) {
    realpoint_t view_corner[2];

    volumes_get_view_corners(ui_series->volumes, ui_series->coord_frame, view_corner);
    view_corner[0] = realspace_base_coord_to_alt(view_corner[0], ui_series->coord_frame);
    view_corner[1] = realspace_base_coord_to_alt(view_corner[1], ui_series->coord_frame);
    ui_series->start_z = view_corner[0].z;
    ui_series->end_z = view_corner[1].z;
    ui_series->num_slices = ceil((ui_series->end_z-ui_series->start_z)/ui_series->thickness);

  } else { /* FRAMES */
    amide_time_t current_start = 0.0;
    amide_time_t last_start, current_end, temp_time;
    guint num_volumes = 0;
    guint * frames;
    guint i;
    guint total_volume_frames=0;
    guint series_frame = 0;
    volume_list_t * temp_volumes;
    gboolean done;
    gboolean valid;

    /* first count num volumes */
    temp_volumes = ui_series->volumes;
    while (temp_volumes != NULL) {
      num_volumes++;
      total_volume_frames += temp_volumes->volume->data_set->dim.t;
      temp_volumes = temp_volumes->next;
    }

    /* get space for the array that'll take care of which frame of which volume we're looking at*/
    frames = (guint *) g_malloc(num_volumes*sizeof(guint));
    if (frames == NULL) {
      g_warning("%s: unable to allocate memory for frames",PACKAGE);
      ui_series_free(ui_series);
      return;
    }
    for (i=0;i<num_volumes;i++) frames[i]=0; /* initialize */
    
    /* get space for the array that'll hold the series' frame durations, overestimate the size */
    ui_series->frame_durations = (amide_time_t *) g_malloc((total_volume_frames+2)*sizeof(amide_time_t));
    if (ui_series->frame_durations == NULL) {
      g_warning("%s: unable to allocate memory for frame durations",PACKAGE);
      g_free(frames);
      ui_series_free(ui_series);
      return;
    }

    /* do the initial case */
    temp_volumes = ui_series->volumes;
    i=0;
    valid = FALSE;
    while (temp_volumes != NULL) {
      if (temp_volumes->volume->data_set->dim.t != frames[i]) {
	temp_time = volume_start_time(temp_volumes->volume, frames[i]);
	if (!valid) /* first valid volume */ {
	  current_start = temp_time;
	  valid = TRUE;
	} else if (temp_time < current_start) {
	  current_start = temp_time;
	}
      }
      i++;
      temp_volumes = temp_volumes->next;
    }

    ui_series->start_time = current_start;
    /* ignore any frames boundaries close to this one */
    temp_volumes = ui_series->volumes;
    i=0;
    while (temp_volumes != NULL) {
      temp_time = volume_start_time(temp_volumes->volume, frames[i]);
      if ((ui_series->start_time-CLOSE < temp_time) &&
	  (ui_series->start_time+CLOSE > temp_time))
	frames[i]++;
      i++;
      temp_volumes = temp_volumes->next;
    }
    
    done = FALSE;
    last_start = ui_series->start_time;
    while (!done) {

      /* check if we're done */
      temp_volumes = ui_series->volumes;
      i=0;
      done = TRUE;
      while (temp_volumes != NULL) {
	if (frames[i] != temp_volumes->volume->data_set->dim.t) 
	  done = FALSE;
	i++; 
	temp_volumes = temp_volumes->next;
      }

      if (!done) {
	/* check for the next earliest start time */
	temp_volumes = ui_series->volumes;
	i=0;
	valid = FALSE;
	while (temp_volumes != NULL) {
	  if (temp_volumes->volume->data_set->dim.t != frames[i]) {
	    temp_time = volume_start_time(temp_volumes->volume, frames[i]);
	    if (!valid) /* first valid volume */ {
	      current_start = temp_time;
	      valid = TRUE;
	    } else if (temp_time < current_start) {
	      current_start = temp_time;
	    }
	  }
	  i++;
	  temp_volumes = temp_volumes->next;
	}

	/* allright, found the next start time */
	ui_series->frame_durations[series_frame] = current_start-last_start;
	series_frame++;
	last_start = current_start;

	/* and ignore any frames boundaries close to this one */
	temp_volumes = ui_series->volumes;
	i=0;
	while (temp_volumes != NULL) {
	  if (temp_volumes->volume->data_set->dim.t != frames[i]) {
	    temp_time = volume_start_time(temp_volumes->volume, frames[i]);
	    if ((current_start-CLOSE < temp_time) &&
		(current_start+CLOSE > temp_time))
	      frames[i]++;
	  }
	  i++;
	  temp_volumes = temp_volumes->next;
	}
      }
    }
      
    /* need to get the last frame */
    temp_volumes=ui_series->volumes;
    i=0;
    current_end = volume_end_time(temp_volumes->volume, frames[i]-1);
    temp_volumes=temp_volumes->next;
    i++;
    while (temp_volumes != NULL) {
      temp_time = volume_end_time(temp_volumes->volume, frames[i]-1);
      if (temp_time > current_end) {
	current_end = temp_time;
      }
      temp_volumes=temp_volumes->next;
      i++;
    }
    ui_series->frame_durations[series_frame] = current_end-last_start;
    series_frame++;
    
    /* save how many frames we'll need */
    ui_series->num_slices = series_frame;
    
    /* garbage collection */
    g_free(frames);

    /* figure out the view_frame */
    temp_time = ui_series->start_time;
    for (i=0; i<ui_series->num_slices;i++) {
      if ((temp_time <= ui_series->view_time) && 
	  (ui_series->view_time < temp_time+ui_series->frame_durations[i]))
	ui_series->view_frame = i;
      temp_time += ui_series->frame_durations[i];
    }

  }



  ui_series_menus_create(ui_series); /* setup the study menu */
  ui_series_toolbar_create(ui_series); /* setup the toolbar */

  /* make the widgets for this dialog box */
  packing_table = gtk_table_new(1,2,FALSE);
  gnome_app_set_contents(app, GTK_WIDGET(packing_table));

  /* setup the canvas */
  ui_series->canvas = GNOME_CANVAS(gnome_canvas_new()); /* save for future use */
  ui_series_update_canvas(ui_series); /* fill in the canvas */
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(ui_series->canvas), 0,1,1,2,
		   X_PACKING_OPTIONS | GTK_FILL,
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);

  /* make a nice scroll bar */
  adjustment = ui_series_create_scroll_adjustment(ui_series);
  scale = gtk_hscale_new(adjustment);
  if (ui_series->type == FRAMES) 
    gtk_scale_set_digits(GTK_SCALE(scale), 0); /* want integer for frames */
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_DISCONTINUOUS);
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(scale), 0,1,0,1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed", 
		     GTK_SIGNAL_FUNC(ui_series_cb_scroll_change), 
		     ui_series);

  /* and show all our widgets */
  gtk_widget_show_all(GTK_WIDGET(app));
  amide_register_window((gpointer) app);

  return;
}







