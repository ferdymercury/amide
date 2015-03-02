/* ui_series.c
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
#include <gnome.h>
#include <libgnomecanvas/gnome-canvas-pixbuf.h>
#include "amitk_threshold.h"
#include "image.h"
#include "ui_common.h"
#include "ui_series.h"
#include "ui_study_menus.h"
#include "../pixmaps/icon_threshold.xpm"

/* external variables */
static gchar * series_names[] = {"over Space", "over Time"};



static void scroll_change_cb(GtkAdjustment* adjustment, gpointer data);
static void export_cb(GtkWidget * widget, gpointer data);
static void data_set_changed_cb(AmitkDataSet * ds, gpointer data);
static void threshold_cb(GtkWidget * widget, gpointer data);
static void close_cb(GtkWidget* widget, gpointer data);
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data);


static void menus_create(ui_series_t * ui_series);
static void toolbar_create(ui_series_t * ui_series);

static ui_series_t * ui_series_free(ui_series_t * ui_series);
static ui_series_t * ui_series_init(void);
static GtkAdjustment * ui_series_create_scroll_adjustment(ui_series_t * ui_series);
static void update_canvas(ui_series_t * ui_series);


/* function called by the adjustment in charge for scrolling */
static void scroll_change_cb(GtkAdjustment* adjustment, gpointer data) {

  ui_series_t * ui_series = data;

  if (ui_series->type == PLANES)
    ui_series->view_point.z = adjustment->value;
  else
    ui_series->view_frame = adjustment->value;

  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_series->canvas));
  update_canvas(ui_series);
  ui_common_remove_cursor(GTK_WIDGET(ui_series->canvas));

  return;
}

/* function to save the series as an external data format */
static void export_cb(GtkWidget * widget, gpointer data) {
  
  //  ui_series_t * ui_series = data;

  /* this function would require being able to transform a canvas back into
     a single image/pixbuf/etc.... don't know how to do this yet */

  return;
}


/* function called when a data set changed */
static void data_set_changed_cb(AmitkDataSet * ds, gpointer data) {
  ui_series_t * ui_series=data;
  update_canvas(ui_series);
  return;
}



static gboolean thresholds_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data) {
  ui_series_t * ui_series = data;

  /* just keeping track on whether or not the threshold widget is up */
  ui_series->thresholds_dialog = NULL;

  return FALSE;
}

/* function called when we hit the threshold button */
static void threshold_cb(GtkWidget * widget, gpointer data) {

  ui_series_t * ui_series = data;

  if (ui_series->thresholds_dialog != NULL)
    return;


  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_series->canvas));

  ui_series->thresholds_dialog = 
    amitk_thresholds_dialog_new(ui_series->objects, GTK_WINDOW(ui_series->app));
  g_signal_connect(G_OBJECT(ui_series->thresholds_dialog), "delete_event",
		   G_CALLBACK(thresholds_delete_event), ui_series);
  gtk_widget_show(ui_series->thresholds_dialog);

  ui_common_remove_cursor(GTK_WIDGET(ui_series->canvas));

  return;
}



/* function ran when closing a series window */
static void close_cb(GtkWidget* widget, gpointer data) {

  ui_series_t * ui_series = data;
  GtkWidget * app = GTK_WIDGET(ui_series->app);
  gboolean return_val;

  /* run the delete event function */
  g_signal_emit_by_name(G_OBJECT(app), "delete_event", NULL, &return_val);
  if (!return_val) gtk_widget_destroy(app);

  return;
}

/* function to run for a delete_event */
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_series_t * ui_series = data;
  GtkWidget * app = GTK_WIDGET(ui_series->app);

  /* free the associated data structure */
  ui_series = ui_series_free(ui_series);

  /* tell the rest of the program this window is no longer here */
  amide_unregister_window((gpointer) app);

  return FALSE;
}






/* function to setup the menus for the series ui */
static void menus_create(ui_series_t * ui_series) {

  /* defining the menus for the series ui interface */
  GnomeUIInfo file_menu[] = {
    //    GNOMEUIINFO_ITEM_DATA(N_("_Export Series"),
    //			  N_("Export the series view to an image file (JPEG/TIFF/PNG/etc.)"),
    //			  ui_series_cb_export,
    //			  ui_series, NULL),
    //    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_MENU_CLOSE_ITEM(close_cb, ui_series),
    GNOMEUIINFO_END
  };

  /* and the main menu definition */
  GnomeUIInfo series_main_menu[] = {
    GNOMEUIINFO_MENU_FILE_TREE(file_menu),
    GNOMEUIINFO_MENU_HELP_TREE(ui_common_help_menu),
    GNOMEUIINFO_END
  };


  /* sanity check */
  g_assert(ui_series!=NULL);


  /* make the menu */
  gnome_app_create_menus(GNOME_APP(ui_series->app), series_main_menu);

  return;

}



      

/* function to setup the toolbar for the seriesui */
void toolbar_create(ui_series_t * ui_series) {

  GtkWidget * toolbar;

  /* the toolbar definitions */
  GnomeUIInfo series_main_toolbar[] = {
    GNOMEUIINFO_ITEM_DATA(NULL,
			  N_("Set the thresholds and colormaps for the data sets in the series view"),
			  threshold_cb,
			  ui_series, icon_threshold_xpm),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_END
  };

  /* sanity check */
  g_assert(ui_series!=NULL);
  
  /* make the toolbar */
  toolbar = gtk_toolbar_new();
  gnome_app_fill_toolbar(GTK_TOOLBAR(toolbar), series_main_toolbar, NULL);

  /* add our toolbar to our app */
  gnome_app_set_toolbar(GNOME_APP(ui_series->app), GTK_TOOLBAR(toolbar));

  return;
}






/* destroy the ui_series slices data */
static void ui_series_slices_free(ui_series_t * ui_series) {
  guint i;

  if (ui_series->slices != NULL) {
    for (i=0; i < ui_series->num_slices; i++) {
      ui_series->slices[i] = amitk_objects_unref(ui_series->slices[i]);
    }
    g_free(ui_series->slices);
    ui_series->slices = NULL;
  }

  return;
}

/* destroy a ui_series data structure */
static ui_series_t * ui_series_free(ui_series_t * ui_series) {

  g_return_val_if_fail(ui_series != NULL, NULL);

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

    if (ui_series->objects != NULL) {
      amitk_objects_unref(ui_series->objects);
      ui_series->objects = NULL;
    }

    if (ui_series->volume != NULL) {
      g_object_unref(ui_series->volume);
      ui_series->volume = NULL;
    }

    if (ui_series->thresholds_dialog != NULL) {
      gtk_widget_destroy(ui_series->thresholds_dialog);
      ui_series->thresholds_dialog = NULL;
    }

    g_free(ui_series->images);
    g_free(ui_series->captions);
    g_free(ui_series->frame_durations);
    g_free(ui_series);
    ui_series = NULL;
  }

  return ui_series;

}

/* allocate and initialize a ui_series data structure */
static ui_series_t * ui_series_init(void) {

  ui_series_t * ui_series;

  /* alloc space for the data structure for passing ui info */
  if ((ui_series = g_new(ui_series_t,1)) == NULL) {
    g_warning("couldn't allocate space for ui_series_t");
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
  ui_series->objects = NULL;
  ui_series->interpolation = AMITK_INTERPOLATION_NEAREST_NEIGHBOR;
  ui_series->voxel_dim = 1.0;
  ui_series->view_point = zero_point;
  ui_series->volume = NULL;
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
    AmitkCorners view_corners;

    amitk_volumes_get_enclosing_corners(ui_series->objects, 
					AMITK_SPACE(ui_series->volume), 
					view_corners);
    
    return GTK_ADJUSTMENT(gtk_adjustment_new(ui_series->view_point.z,
					     view_corners[0].z,
					     view_corners[1].z,
					     AMITK_VOLUME_Z_CORNER(ui_series->volume),
					     AMITK_VOLUME_Z_CORNER(ui_series->volume),
					     AMITK_VOLUME_Z_CORNER(ui_series->volume)));
  } else { /* FRAMES */
    return GTK_ADJUSTMENT(gtk_adjustment_new(ui_series->view_frame, 0, ui_series->num_slices, 1,1,1));
  }
}


/* funtion to update the canvas */
static void update_canvas(ui_series_t * ui_series) {

  AmitkPoint temp_point;
  amide_time_t temp_time, temp_duration;
  gint i, start_i;
  gint width, height;
  gdouble x, y;
  GtkRequisition requisition;
  AmitkVolume * view_volume;
  gint image_width, image_height;
  gchar * temp_string;
  GdkPixbuf * pixbuf;


  /* get the view axis to use*/
  width = 0.9*gdk_screen_width();
  height = 0.85*gdk_screen_height();

  /* allocate space for pointers to our slices if needed */
  if (ui_series->slices == NULL) {
    if ((ui_series->slices = g_new(GList *,ui_series->num_slices)) == NULL) {
      g_warning("couldn't allocate space for pointers to amide_volume_t's");
      return;
    }
    for (i=0; i < ui_series->num_slices ; i++)
      ui_series->slices[i] = NULL;
  }
    
  /* always do the first image, so that we can get the width and height
     of an image (needed for computing the rows and columns portions
     of the ui_series data structure */
  temp_point = ui_series->view_point;
  if (ui_series->type == PLANES) {
    temp_point.z = ui_series->start_z;
    temp_time =  ui_series->view_time;
    temp_duration = ui_series->view_duration;
  } else {
    temp_time = ui_series->start_time;
    temp_duration = ui_series->frame_durations[0];
  }
  view_volume = AMITK_VOLUME(amitk_object_copy(AMITK_OBJECT(ui_series->volume)));
  amitk_space_set_offset(AMITK_SPACE(view_volume), 
			 amitk_space_s2b(AMITK_SPACE(ui_series->volume), temp_point));
  pixbuf = image_from_data_sets(&(ui_series->slices[0]),
				ui_series->objects,
				temp_time*(1.0+EPSILON),
				temp_duration*(1.0-EPSILON),
				ui_series->voxel_dim,
				view_volume,
				ui_series->interpolation);
  g_object_unref(view_volume);
  image_width = gdk_pixbuf_get_width(pixbuf) + UI_SERIES_R_MARGIN + UI_SERIES_L_MARGIN;
  image_height = gdk_pixbuf_get_height(pixbuf) + UI_SERIES_TOP_MARGIN + UI_SERIES_BOTTOM_MARGIN;


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

    if ((ui_series->images = (GnomeCanvasItem **) g_malloc(sizeof(GnomeCanvasItem *)*ui_series->rows*ui_series->columns)) == NULL) {
      g_warning("couldn't allocate space for pointers to image GnomeCanvasItem's");
      return;
    }
    if ((ui_series->captions = (GnomeCanvasItem **) g_malloc(sizeof(GnomeCanvasItem *)*ui_series->rows*ui_series->columns)) == NULL) {
      g_warning("couldn't allocate space for pointers to caption GnomeCanavasItem's");
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
  else if (start_i > (ui_series->num_slices - ui_series->columns*ui_series->rows))
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
      temp_point.z = i*AMITK_VOLUME_Z_CORNER(ui_series->volume)+ui_series->start_z;
    } else { /* frames */
      temp_time += temp_duration; /* duration from last time through the loop */
      temp_duration = ui_series->frame_durations[i];
    }
    view_volume = AMITK_VOLUME(amitk_object_copy(AMITK_OBJECT(ui_series->volume)));
    amitk_space_set_offset(AMITK_SPACE(view_volume), 
			   amitk_space_s2b(AMITK_SPACE(ui_series->volume), temp_point));
    
    if (i != 0) /* we've already done image 0 */
      pixbuf = image_from_data_sets(&(ui_series->slices[i]),
				    ui_series->objects,
				    temp_time*(1.0+EPSILON),
				    temp_duration*(1.0-EPSILON),
				    ui_series->voxel_dim,
				    view_volume,
				    ui_series->interpolation);
    g_object_unref(view_volume);
    
    /* figure out the next x,y spot to put this guy */
    y = floor((i-start_i)/ui_series->columns)*image_height;
    x = (i-start_i-ui_series->columns*floor((i-start_i)/ui_series->columns))*image_width;
    
    if (ui_series->images[i-start_i] == NULL) 
      ui_series->images[i-start_i] = gnome_canvas_item_new(gnome_canvas_root(ui_series->canvas),
							   gnome_canvas_pixbuf_get_type(),
							   "pixbuf", pixbuf,
							   "x", x+UI_SERIES_L_MARGIN,
							   "y", y+UI_SERIES_TOP_MARGIN,
							   NULL);
    else
      gnome_canvas_item_set(ui_series->images[i-start_i], "pixbuf", pixbuf, NULL);
    g_object_unref(pixbuf);
    
    if (ui_series->type == PLANES)
      temp_string = g_strdup_printf("%2.1f-%2.1f mm", temp_point.z, temp_point.z+AMITK_VOLUME_Z_CORNER(ui_series->volume));
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
			      "fill_color", "black", 
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
    gtk_widget_set_size_request(GTK_WIDGET(ui_series->canvas), width, height);
    /* the requisition thing should work.... I'll use this for now for now... */


  return;
}

/* function that sets up the series dialog */
void ui_series_create(AmitkStudy * study, GList * objects, AmitkView view, 
		      AmitkVolume * canvas_view, series_t series_type) {
 
  ui_series_t * ui_series;
  GnomeApp * app;
  gchar * title = NULL;
  GtkWidget * packing_table;
  GtkAdjustment * adjustment;
  GtkWidget * scale;
  amide_time_t min_duration;
  GList * temp_objects;

  /* sanity checks */
  g_return_if_fail(AMITK_IS_STUDY(study));
  g_return_if_fail(objects != NULL);

  ui_series = ui_series_init();
  ui_series->type = series_type;

  title = g_strdup_printf("Series: %s (%s - %s)", AMITK_OBJECT_NAME(study),
			  view_names[view], series_names[ui_series->type]);
  app = GNOME_APP(gnome_app_new(PACKAGE, title));
  g_free(title);
  ui_series->app = app;
  gtk_window_set_resizable(GTK_WINDOW(ui_series->app), TRUE);

  /* add a reference to the objects sent to this series */
  ui_series->objects = amitk_objects_ref(objects);

  /* setup the callbacks for app */
  g_signal_connect(G_OBJECT(app), "realize", G_CALLBACK(ui_common_window_realize_cb), NULL);
  g_signal_connect(G_OBJECT(app), "delete_event", G_CALLBACK(delete_event_cb), ui_series);

  /* save the coordinate space of the series and some other parameters */
  ui_series->volume = AMITK_VOLUME(amitk_object_copy(AMITK_OBJECT(canvas_view)));

  ui_series->view_point = 
    amitk_space_b2s(AMITK_SPACE(ui_series->volume), AMITK_STUDY_VIEW_CENTER(study));
  ui_series->interpolation = AMITK_STUDY_INTERPOLATION(study);
  ui_series->view_time = AMITK_STUDY_VIEW_START_TIME(study);
  min_duration = amitk_data_sets_get_min_frame_duration(ui_series->objects);
  ui_series->view_duration =  
    (min_duration > AMITK_STUDY_VIEW_DURATION(study)) ?  min_duration : AMITK_STUDY_VIEW_DURATION(study);
  ui_series->voxel_dim = (1/AMITK_STUDY_ZOOM(study)) * 
    amitk_data_sets_get_max_min_voxel_size(ui_series->objects);


  /* do some initial calculations */
  if (ui_series->type == PLANES) {
    AmitkCorners view_corners;

    amitk_volumes_get_enclosing_corners(ui_series->objects, AMITK_SPACE(ui_series->volume), view_corners);
    ui_series->start_z = view_corners[0].z;
    ui_series->end_z = view_corners[1].z;
    ui_series->num_slices = ceil((ui_series->end_z-ui_series->start_z)/AMITK_VOLUME_Z_CORNER(ui_series->volume));

  } else { /* FRAMES */
    amide_time_t current_start = 0.0;
    amide_time_t last_start, current_end, temp_time;
    guint num_data_sets = 0;
    guint * frames;
    guint i;
    guint total_data_set_frames=0;
    guint series_frame = 0;
    gboolean done;
    gboolean valid;

    /* first count num objects */
    temp_objects = ui_series->objects;
    while (temp_objects != NULL) {
      if (AMITK_IS_DATA_SET(temp_objects->data)) {
	num_data_sets++;
	total_data_set_frames += AMITK_DATA_SET_NUM_FRAMES(temp_objects->data);
      }
      temp_objects = temp_objects->next;
    }
    
    /* get space for the array that'll take care of which frame of which data set we're looking at*/
    frames = g_new(guint,num_data_sets);
    if (frames == NULL) {
      g_warning("unable to allocate memory for frames");
      ui_series_free(ui_series);
      return;
    }
    for (i=0;i<num_data_sets;i++) frames[i]=0; /* initialize */
    
    /* get space for the array that'll hold the series' frame durations, overestimate the size */
    ui_series->frame_durations = g_new(amide_time_t,total_data_set_frames+2);
    if (ui_series->frame_durations == NULL) {
      g_warning("unable to allocate memory for frame durations");
      g_free(frames);
      ui_series_free(ui_series);
      return;
    }

    /* do the initial case */
    temp_objects = ui_series->objects;
    i=0;
    valid = FALSE;
    while (temp_objects != NULL) {
      if (AMITK_IS_DATA_SET(temp_objects->data)) {
	if (AMITK_DATA_SET_NUM_FRAMES(temp_objects->data) != frames[i]) {
	  temp_time = amitk_data_set_get_start_time(AMITK_DATA_SET(temp_objects->data), frames[i]);
	  if (!valid) /* first valid data set */ {
	    current_start = temp_time;
	    valid = TRUE;
	  } else if (temp_time < current_start) {
	    current_start = temp_time;
	  }
	}
	i++;
      }
      temp_objects = temp_objects->next;
    }

    ui_series->start_time = current_start;
    /* ignore any frames boundaries close to this one */
    temp_objects = ui_series->objects;
    i=0;
    while (temp_objects != NULL) {
      if (AMITK_IS_DATA_SET(temp_objects->data)) {
	temp_time = amitk_data_set_get_start_time(AMITK_DATA_SET(temp_objects->data), frames[i]);
	if ((ui_series->start_time*(1.0-EPSILON) < temp_time) &&
	    (ui_series->start_time*(1.0+EPSILON) > temp_time))
	  frames[i]++;
	i++;
      }
      temp_objects = temp_objects->next;
    }
    
    done = FALSE;
    last_start = ui_series->start_time;
    while (!done) {

      /* check if we're done */
      temp_objects = ui_series->objects;
      i=0;
      done = TRUE;
      while (temp_objects != NULL) {
	if (AMITK_IS_DATA_SET(temp_objects->data)) {
	  if (frames[i] != AMITK_DATA_SET_NUM_FRAMES(temp_objects->data)) 
	    done = FALSE;
	  i++; 
	}
	temp_objects = temp_objects->next;
      }

      if (!done) {
	/* check for the next earliest start time */
	temp_objects = ui_series->objects;
	i=0;
	valid = FALSE;
	while (temp_objects != NULL) {
	  if (AMITK_IS_DATA_SET(temp_objects->data)) {
	    if (AMITK_DATA_SET_NUM_FRAMES(temp_objects->data) != frames[i]) {
	      temp_time = amitk_data_set_get_start_time(AMITK_DATA_SET(temp_objects->data), frames[i]);
	      if (!valid) /* first valid data set */ {
		current_start = temp_time;
		valid = TRUE;
	      } else if (temp_time < current_start) {
		current_start = temp_time;
	      }
	    }
	    i++;
	  }
	  temp_objects = temp_objects->next;
	}

	/* allright, found the next start time */
	ui_series->frame_durations[series_frame] = current_start-last_start;
	series_frame++;
	last_start = current_start;

	/* and ignore any frames boundaries close to this one */
	temp_objects = ui_series->objects;
	i=0;
	while (temp_objects != NULL) {
	  if (AMITK_IS_DATA_SET(temp_objects->data)) {
	    if (AMITK_DATA_SET_NUM_FRAMES(temp_objects->data) != frames[i]) {
	      temp_time = amitk_data_set_get_start_time(AMITK_DATA_SET(temp_objects->data), frames[i]);
	      if ((current_start*(1.0-EPSILON) < temp_time) &&
		  (current_start*(1.0+EPSILON) > temp_time))
		frames[i]++;
	    }
	    i++;
	  }
	  temp_objects = temp_objects->next;
	}
      }
    }
    
    /* need to get the last frame */
    temp_objects = ui_series->objects;
    i=0;
    current_end = amitk_data_set_get_end_time(temp_objects->data, frames[i]-1);
    temp_objects = temp_objects->next;
    i++;
    while (temp_objects != NULL) {
      if (AMITK_IS_DATA_SET(temp_objects->data)) {
	temp_time = amitk_data_set_get_end_time(AMITK_DATA_SET(temp_objects->data), frames[i]-1);
	if (temp_time > current_end) {
	  current_end = temp_time;
	}
	i++;
      }
      temp_objects = temp_objects->next;
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

  /* connect the thresholding and color table signals */
  temp_objects = ui_series->objects;
  while (temp_objects != NULL) {
    if (AMITK_IS_DATA_SET(temp_objects->data)) {
      g_signal_connect(G_OBJECT(temp_objects->data), "thresholding_changed",
		       G_CALLBACK(data_set_changed_cb), ui_series);
      g_signal_connect(G_OBJECT(temp_objects->data), "color_table_changed",
		       G_CALLBACK(data_set_changed_cb), ui_series);
    }
    temp_objects = temp_objects->next;
  }


  menus_create(ui_series); /* setup the menu */
  toolbar_create(ui_series); /* setup the toolbar */

  /* make the widgets for this dialog box */
  packing_table = gtk_table_new(1,2,FALSE);
  gnome_app_set_contents(app, GTK_WIDGET(packing_table));

  /* setup the canvas */
  ui_series->canvas = GNOME_CANVAS(gnome_canvas_new_aa()); /* save for future use */
  update_canvas(ui_series); /* fill in the canvas */
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
  g_signal_connect(G_OBJECT(adjustment), "value_changed", G_CALLBACK(scroll_change_cb), ui_series);

  /* and show all our widgets */
  gtk_widget_show_all(GTK_WIDGET(app));
  amide_register_window((gpointer) app);

  return;
}







