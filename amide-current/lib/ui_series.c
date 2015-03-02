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
#include "amide.h"
#include "study.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_roi.h"
#include "ui_volume.h"
#include "ui_study.h"
#include "ui_series2.h"
#include "ui_series_callbacks.h"


/* external variables */
gchar * series_names[] = {"over Space", "over Time"};

/* destroy the ui_series slices data */
void ui_series_slices_free(ui_series_t * ui_series) {
  guint i;

  for (i=0; i < ui_series->num_slices; i++) {
    ui_series->slices[i] = volume_list_free(ui_series->slices[i]);
    if (ui_series->rgb_images[i] != NULL)
      gnome_canvas_destroy_image(ui_series->rgb_images[i]);
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

  /* things we always do */
  ui_series_slices_free(ui_series);
  ui_series->volumes = volume_list_free(ui_series->volumes);

  /* things to do if we've removed all reference's */
  if (ui_series->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing ui_series\n");
#endif
    g_free(ui_series->images);
    g_free(ui_series);
    ui_series = NULL;
  }

  return ui_series;

}

/* malloc and initialize a ui_series data structure */
ui_series_t * ui_series_init(void) {

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
  ui_series->rgb_images = NULL;
  ui_series->volumes = NULL;
  ui_series->interpolation = NEAREST_NEIGHBOR;
  ui_series->view_time = 0.0;
  ui_series->view_duration = 1.0;
  ui_series->zoom = 1.0;
  ui_series->thickness = -1.0;
  ui_series->view_point = realpoint_init;
  ui_series->type = PLANES;

  return ui_series;
}


/* function to make the adjustments for the scrolling scale */
GtkAdjustment * ui_series_create_scroll_adjustment(ui_study_t * ui_study) { 

  if (ui_study->series->type == PLANES) {
    realpoint_t view_corner[2];

    volumes_get_view_corners(ui_study->series->volumes, ui_study->series->coord_frame, view_corner);
    view_corner[0] = realspace_base_coord_to_alt(view_corner[0], ui_study->series->coord_frame);
    view_corner[1] = realspace_base_coord_to_alt(view_corner[1], ui_study->series->coord_frame);
    
    return GTK_ADJUSTMENT(gtk_adjustment_new(ui_study->series->view_point.z,
					     view_corner[0].z,
					     view_corner[1].z,
					     ui_study->series->thickness,
					     ui_study->series->thickness,
					     ui_study->series->thickness));
  } else { /* FRAMES */
    volume_time_t start_time, end_time;
    start_time = volume_list_start_time(ui_study->series->volumes);
    end_time = volume_list_end_time(ui_study->series->volumes);

    return GTK_ADJUSTMENT(gtk_adjustment_new(ui_study->series->view_time,
					     start_time, 
					     end_time,
					     ui_study->series->view_duration, 
					     ui_study->series->view_duration,
					     ui_study->series->view_duration));

  }
}

/* funtion to update the canvas iamge */
void ui_series_update_canvas_image(ui_study_t * ui_study) {

  floatpoint_t view_start, view_end;
  volume_time_t time_start, time_end, temp_time;
  realpoint_t temp_point;
  guint i, start_i;
  gint width, height;
  gdouble x, y;
  GtkRequisition requisition;
  GdkCursor * cursor;
  GdkWindow * parent=NULL;
  realspace_t view_coord_frame;
  realpoint_t view_corner[2];


  /* push our desired cursor onto the cursor stack */
  cursor = ui_study->cursor[UI_STUDY_WAIT];
  ui_study->cursor_stack = g_slist_prepend(ui_study->cursor_stack,cursor);

  if (GTK_WIDGET_REALIZED(GTK_WIDGET(ui_study->series->canvas)))
    parent = gtk_widget_get_parent_window(GTK_WIDGET(ui_study->series->canvas));
  if (parent == NULL)
    parent = gtk_widget_get_parent_window(GTK_WIDGET(ui_study->canvas[0]));
  gdk_window_set_cursor(parent, cursor);
  
  /* do any events pending, this allows the cursor to get displayed */
  while (gtk_events_pending()) 
    gtk_main_iteration();


  /* get the view axis to use*/
  width = 0.9*gdk_screen_width();
  height = 0.9*gdk_screen_height();

  /* figure out some parameters */
  volumes_get_view_corners(ui_study->series->volumes, ui_study->series->coord_frame, view_corner);
  view_corner[0] = realspace_base_coord_to_alt(view_corner[0], ui_study->series->coord_frame);
  view_corner[1] = realspace_base_coord_to_alt(view_corner[1], ui_study->series->coord_frame);
  view_end = view_corner[1].z;
  view_start = view_corner[0].z;
  time_start = volume_list_start_time(ui_study->series->volumes);
  time_end = volume_list_end_time(ui_study->series->volumes);
  g_print("temp_start %5.3f time_end %5.3f duration %5.3f\n",time_start, time_end, 
	  ui_study->series->view_duration);
  if (ui_study->series->type == PLANES)
    ui_study->series->num_slices = ceil((view_end-view_start)/ui_study->series->thickness);
  else /* FRAMES */
    ui_study->series->num_slices = ceil((time_end-time_start)/ui_study->series->view_duration);

  /* allocate space for pointers to our slices if needed */
  if (ui_study->series->slices == NULL) {
    if ((ui_study->series->slices = 
	 (volume_list_t **) g_malloc(sizeof(volume_list_t *) *ui_study->series->num_slices)) == NULL) {
      g_warning("%s: couldn't allocate space for pointers to amide_volume_t's",
		PACKAGE);
      return;
    }
    for (i=0; i < ui_study->series->num_slices ; i++)
      ui_study->series->slices[i] = NULL;
  }
    
  /* allocate space for pointers to our rgb_images if needed */
  if (ui_study->series->rgb_images == NULL) {
    if ((ui_study->series->rgb_images = 
	 (GdkImlibImage **) g_malloc(sizeof(GdkImlibImage *)*ui_study->series->num_slices)) 
	== NULL) {
      g_warning("%s: couldn't allocate space for pointers to GdkImlibImage's",
		PACKAGE);
      return;
    }
    for (i=0; i < ui_study->series->num_slices ; i++)
      ui_study->series->rgb_images[i] = NULL;
  }

  /* always do the first image, so that we can get the width and height
     of an image (needed for computing the rows and columns portions
     of the ui_series data structure */
  if (ui_study->series->rgb_images[0] != NULL) 
    gnome_canvas_destroy_image(ui_study->series->rgb_images[0]);

  temp_point = ui_study->series->view_point;
  temp_time =  ui_study->series->view_time;
  if (ui_study->series->type == PLANES)
    temp_point.z = view_start;
  else /* FRAMES */
    temp_time = time_start;
  view_coord_frame = ui_study->series->coord_frame;
  view_coord_frame.offset = realspace_alt_coord_to_base(temp_point, ui_study->series->coord_frame);
  ui_study->series->rgb_images[0] = 
    image_from_volumes(&(ui_study->series->slices[0]),
		       ui_study->series->volumes,
		       temp_time+CLOSE,
		       ui_study->series->view_duration-CLOSE,
		       ui_study->series->thickness,
		       view_coord_frame,
		       study_scaling(ui_study->study),
		       ui_study->series->zoom,
		       ui_study->series->interpolation);

  /* allocate space for pointers to our canvas_images if needed */
  if (ui_study->series->images == NULL) {
    /* figure out how many images we can display at once */
    ui_study->series->columns = 
      floor(width/ui_study->series->rgb_images[0]->rgb_width);
    if (ui_study->series->columns < 1)
      ui_study->series->columns = 1;
    ui_study->series->rows = 
      floor(height/ui_study->series->rgb_images[0]->rgb_height);
    if (ui_study->series->rows < 1)
      ui_study->series->rows = 1;
    if ((ui_study->series->images = 
	 (GnomeCanvasItem **) g_malloc(sizeof(GnomeCanvasItem *) * ui_study->series->rows
				       * ui_study->series->columns)) == NULL) {
      g_warning("%s: couldn't allocate space for pointers to GnomeCanavasItem's",PACKAGE);
      return;
    }
    for (i=0; i < ui_study->series->columns * ui_study->series->rows ; i++)
      ui_study->series->images[i] = NULL;
  }


  /* figure out what's the first image we want to display */
  if (ui_study->series->type == PLANES) 
    i = ui_study->series->num_slices*((ui_study->series->view_point.z-view_start)/(view_end-view_start));
  else  /* FRAMES */
    i = ui_study->series->num_slices*((ui_study->series->view_time-time_start)/(time_end-time_start));

  if (ui_study->series->num_slices <= ui_study->series->columns*ui_study->series->rows)
    i = 0;
  else if (i > (ui_study->series->num_slices -  ui_study->series->columns*ui_study->series->rows))
    i = ui_study->series->num_slices - ui_study->series->columns*ui_study->series->rows;
  if (i < 0)
    i = 0;
  if (ui_study->series->num_slices <= ui_study->series->columns*ui_study->series->rows)
    i = 0;
  else if (i > (ui_study->series->num_slices -  ui_study->series->columns*ui_study->series->rows))
    i = ui_study->series->num_slices - ui_study->series->columns*ui_study->series->rows;
  if (i < 0)
    i = 0;

  start_i = i;
  x = y = 0.0;

  for (; ((i-start_i) < (ui_study->series->rows*ui_study->series->columns))
	 && (i < ui_study->series->num_slices); i++) {
    temp_point = ui_study->series->view_point;
    temp_time = ui_study->series->view_time;
    if (ui_study->series->type == PLANES) 
      temp_point.z = i*ui_study->series->thickness+view_start;
    else
      temp_time = i*ui_study->series->view_duration + time_start;
    view_coord_frame = ui_study->series->coord_frame;
    view_coord_frame.offset = realspace_alt_coord_to_base(temp_point, ui_study->series->coord_frame);

    g_print("temp time %5.3f duration %5.3f\n",temp_time+CLOSE, ui_study->series->view_duration-CLOSE);
    
    if (i != 0) /* we've already done image 0 */
      if (ui_study->series->rgb_images[i] != NULL)
	gnome_canvas_destroy_image(ui_study->series->rgb_images[i]);
    
    if (i != 0) /* we've already done image 0 */
      ui_study->series->rgb_images[i] = 
	image_from_volumes(&(ui_study->series->slices[i]),
			   ui_study->series->volumes,
			   temp_time+CLOSE,
			   ui_study->series->view_duration-CLOSE,
			   ui_study->series->thickness,
			   view_coord_frame,
			   study_scaling(ui_study->study),
			   ui_study->series->zoom,
			   ui_study->series->interpolation);
    
    /* figure out the next x,y spot to put this guy */
    y = floor((i-start_i)/ui_study->series->columns)
      *ui_study->series->rgb_images[i]->rgb_height;
    x = (i-start_i-ui_study->series->columns
	 *floor((i-start_i)/ui_study->series->columns))*
      ui_study->series->rgb_images[i]->rgb_width;
    
    if (ui_study->series->images[i-start_i] == NULL) 
      ui_study->series->images[i-start_i] =
	gnome_canvas_item_new(gnome_canvas_root(ui_study->series->canvas),
			      gnome_canvas_image_get_type(),
			      "image", ui_study->series->rgb_images[i],
			      "x", x,
			      "y", y,
			      "anchor", GTK_ANCHOR_NORTH_WEST,
			      "width", 
			      (double) ui_study->series->rgb_images[i]->rgb_width,
			      "height", 
			      (double) ui_study->series->rgb_images[i]->rgb_height,
			      NULL);
    else
      gnome_canvas_item_set(ui_study->series->images[i-start_i],
			    "image", ui_study->series->rgb_images[i],
			    NULL);
  }

  /* readjust widith and height for what we really used */
  width = ui_study->series->columns*ui_study->series->rgb_images[0]->rgb_width;
  height =ui_study->series->rows   *ui_study->series->rgb_images[0]->rgb_height;


  /* reset the min size of the widget */
  gnome_canvas_set_scroll_region(ui_study->series->canvas, 0.0, 0.0, 
  				 (double) width, (double) height);

  requisition.width = width;
  requisition.height = height;
  gtk_widget_size_request(GTK_WIDGET(ui_study->series->canvas), &requisition);
  gtk_widget_set_usize(GTK_WIDGET(ui_study->series->canvas), width, height);
  /* the requisition thing should work.... I'll use usize for now... */

  /* pop the previous cursor off the stack */
  cursor = g_slist_nth_data(ui_study->cursor_stack, 0);
  ui_study->cursor_stack = g_slist_remove(ui_study->cursor_stack, cursor);
  cursor = g_slist_nth_data(ui_study->cursor_stack, 0);
  gdk_window_set_cursor(parent, cursor);

  return;
}

/* function that sets up the series dialog */
void ui_series_create(ui_study_t * ui_study, view_t view, series_t series_type) {
  
  GnomeApp * app;
  gchar * title = NULL;
  GtkWidget * packing_table;
  GnomeCanvas * series_canvas;
  GtkAdjustment * adjustment;
  GtkWidget * scale;
  volume_time_t min_duration;

  /* sanity checks */
  if (study_volumes(ui_study->study) == NULL)
    return;
  if (study_first_volume(ui_study->study) == NULL)
    return;
  if (ui_study->series != NULL)
    return;
  if (ui_study->current_volumes == NULL)
    return;

  ui_study->series = ui_series_init();
  ui_study->series->type = series_type;

  title = g_strdup_printf("Series: %s (%s - %s)",
			  study_name(ui_study->study), 
			  view_names[view], 
			  series_names[series_type]);
  app = GNOME_APP(gnome_app_new(PACKAGE, title));
  g_free(title);
  ui_study->series->app = app;

  /* make a copy of the volume list for the series */
  ui_study->series->volumes = ui_volume_list_return_volume_list(ui_study->current_volumes);

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(app), "delete_event",
		     GTK_SIGNAL_FUNC(ui_series_callbacks_delete_event),
		     ui_study);

  /* make the widgets for this dialog box */
  packing_table = gtk_table_new(1,2,FALSE);
  gnome_app_set_contents(app, GTK_WIDGET(packing_table));

  /* save the coord_frame of the series */
  ui_study->series->coord_frame = 
    realspace_get_orthogonal_coord_frame(study_coord_frame(ui_study->study), view);
  ui_study->series->view_point = 
    realspace_alt_coord_to_alt(study_view_center(ui_study->study),
			       study_coord_frame(ui_study->study),
			       ui_study->series->coord_frame);

  /* save some parameters */
  ui_study->series->thickness = study_view_thickness(ui_study->study);
  ui_study->series->interpolation = study_interpolation(ui_study->study);
  ui_study->series->view_time = study_view_time(ui_study->study);
  min_duration = volume_list_min_frame_duration(ui_study->series->volumes);
  ui_study->series->view_duration = 
    (min_duration > study_view_duration(ui_study->study)) ? 
    min_duration : study_view_duration(ui_study->study);
  ui_study->series->zoom = study_zoom(ui_study->study);

  /* setup the canvas */
  series_canvas = GNOME_CANVAS(gnome_canvas_new());
  ui_study->series->canvas = series_canvas; /* save for future use */
  ui_series_update_canvas_image(ui_study); /* fill in the canvas */
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(series_canvas), 0,1,1,2,
		   X_PACKING_OPTIONS | GTK_FILL,
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);

  /* make a nice scroll bar */
  adjustment = ui_series_create_scroll_adjustment(ui_study);
  scale = gtk_hscale_new(adjustment);
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_DISCONTINUOUS);
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(scale), 0,1,0,1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed", 
		     GTK_SIGNAL_FUNC(ui_series_callbacks_scroll_change), 
		     ui_study);

  /* and show all our widgets */
  gtk_widget_show_all(GTK_WIDGET(app));

  return;
}







