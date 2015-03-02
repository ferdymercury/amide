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
#include "realspace.h"
#include "color_table.h"
#include "volume.h"
#include "color_table2.h"
#include "roi.h"
#include "study.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_study_rois.h"
#include "ui_study_volumes.h"
#include "ui_study.h"
#include "ui_series2.h"
#include "ui_series_callbacks.h"

/* destroy the ui_series slices data */
void ui_series_slices_free(ui_series_t * ui_series) {
  guint i;

  for (i=0; i < ui_series->num_slices; i++) {
    if (ui_series->slices[i] != NULL)
      volume_list_free(&(ui_series->slices[i]));
    if (ui_series->rgb_images[i] != NULL)
      gnome_canvas_destroy_image(ui_series->rgb_images[i]);
  }
  g_free(ui_series->slices);
  ui_series->slices = NULL;

  return;
}

/* destroy a ui_series data structure */
void ui_series_free(ui_series_t ** pui_series) {

  ui_series_slices_free(*pui_series);
  g_free((*pui_series)->images);
  while ((*pui_series)->volumes != NULL)
    volume_list_remove_volume(&((*pui_series)->volumes), ((*pui_series)->volumes->volume));
  g_free(*pui_series);
  *pui_series = NULL;

  return;
}

/* malloc and initialize a ui_series data structure */
ui_series_t * ui_series_init(void) {

  ui_series_t * ui_series;

  /* alloc space for the data structure for passing ui info */
  if ((ui_series = (ui_series_t *) g_malloc(sizeof(ui_series_t))) == NULL) {
    g_warning("%s: couldn't allocate space for ui_series_t",PACKAGE);
    return NULL;
  }

  /* set any needed parameters */
  ui_series->slices = NULL;
  ui_series->num_slices = 0;
  ui_series->rows = 0;
  ui_series->columns = 0;
  ui_series->images = NULL;
  ui_series->rgb_images = NULL;
  ui_series->volumes = NULL;
  ui_series->interpolation = NEAREST_NEIGHBOR;
  ui_series->time = 0.0;
  ui_series->duration = 1.0;
  ui_series->zoom = 1.0;
  ui_series->thickness = -1.0;
  ui_series->start = realpoint_init;

  return ui_series;
}


/* function to make the adjustments for the scrolling scale */
GtkAdjustment * ui_series_create_scroll_adjustment(ui_study_t * ui_study) { 


  floatpoint_t length, thickness;
  GtkAdjustment * new_adjustment;

  length = volumes_get_length(ui_study->series->volumes, ui_study->series->coord_frame);
  thickness = ui_study->series->thickness;

  new_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(ui_study->series->start.z,
						     0, length, 
						     thickness, thickness,
						     thickness));
  return new_adjustment;
}

/* funtion to update the canvas iamge */
void ui_series_update_canvas_image(ui_study_t * ui_study) {

  floatpoint_t zp_length;
  realpoint_t temp_point;
  guint i, start_i;
  gint width, height;
  gdouble x, y;
  GtkRequisition requisition;
  GdkCursor * cursor;
  GdkWindow * parent=NULL;
  realspace_t view_coord_frame;

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
  width = 0.8*gdk_screen_width();
  height = 0.8*gdk_screen_height();

  /* figure out some parameters */
  zp_length = volumes_get_length(ui_study->series->volumes,ui_study->series->coord_frame);
  ui_study->series->num_slices = ceil(zp_length/ui_study->series->thickness);

  /* allocate space for pointers to our slices if needed */
  if (ui_study->series->slices == NULL) {
    if ((ui_study->series->slices = 
	 (amide_volume_list_t **) g_malloc(sizeof(amide_volume_list_t *) *ui_study->series->num_slices)) == NULL) {
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
  view_coord_frame = ui_study->series->coord_frame;
  view_coord_frame.offset = realpoint_init;

  ui_study->series->rgb_images[0] = 
    image_from_volumes(&(ui_study->series->slices[0]),
		       ui_study->series->volumes,
		       ui_study->series->time,
		       ui_study->series->duration,
		       ui_study->series->thickness,
		       view_coord_frame,
		       ui_study->scaling,
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
	 (GnomeCanvasItem **) g_malloc(sizeof(GnomeCanvasItem *)
				       * ui_study->series->rows
				       * ui_study->series->columns)) 
	== NULL) {
      g_warning("%s: couldn't allocate space for pointers to GnomeCanavasItem's",
		PACKAGE);
      return;
    }
    for (i=0; i < ui_study->series->columns * ui_study->series->rows ; i++)
      ui_study->series->images[i] = NULL;
  }


  /* figure out what's the first image we want to display */
  i = ui_study->series->num_slices*(ui_study->series->start.z/zp_length);
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
    temp_point = ui_study->series->start;
    temp_point.z = i*ui_study->series->thickness;
    view_coord_frame.offset = 
      realspace_alt_coord_to_base(temp_point,
				  ui_study->series->coord_frame);
    
    if (i != 0) /* we've already done image 0 */
      if (ui_study->series->rgb_images[i] != NULL)
	gnome_canvas_destroy_image(ui_study->series->rgb_images[i]);
    
    if (i != 0) /* we've already done image 0 */
      ui_study->series->rgb_images[i] = 
	image_from_volumes(&(ui_study->series->slices[i]),
			   ui_study->series->volumes,
			   ui_study->series->time,
			   ui_study->series->duration,
			   ui_study->series->thickness,
			   view_coord_frame,
			   ui_study->scaling,
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
void ui_series_create(ui_study_t * ui_study, view_t view) {
  
  GnomeApp * app;
  gchar * title = NULL;
  GtkWidget * packing_table;
  GnomeCanvas * series_canvas;
  GtkAdjustment * adjustment;
  GtkWidget * scale;
  ui_study_volume_list_t * ui_volumes;

  /* sanity checks */
  if (ui_study->study->volumes == NULL)
    return;
  if (ui_study->study->volumes->volume == NULL)
    return;
  if (ui_study->series != NULL)
    return;
  if (ui_study->current_volumes == NULL)
    return;

  ui_study->series = ui_series_init();

  title = g_strdup_printf("Series: %s (%s)",ui_study->study->name, view_names[view]);
  app = GNOME_APP(gnome_app_new(PACKAGE, title));
  g_free(title);
  ui_study->series->app = app;

  /* figure out the currrent number of volumes, and make a copy in our series's volume list */
  ui_volumes = ui_study->current_volumes;
  while (ui_volumes != NULL) {
    volume_list_add_volume(&(ui_study->series->volumes),ui_volumes->volume);
    ui_volumes = ui_volumes->next;
  }

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(app), "delete_event",
		     GTK_SIGNAL_FUNC(ui_series_callbacks_delete_event),
		     ui_study);

  /* make the widgets for this dialog box */
  packing_table = gtk_table_new(1,2,FALSE);
  gnome_app_set_contents(app, GTK_WIDGET(packing_table));

  /* save the coord_frame of the series */
  ui_study->series->coord_frame = ui_study->current_view_coord_frame;
  ui_study->series->coord_frame = 
    realspace_get_orthogonal_coord_frame(ui_study->series->coord_frame, view);
  ui_study->series->start = ui_study->current_view_center;

  /* save some parameters */
  ui_study->series->thickness = ui_study->current_thickness;
  ui_study->series->interpolation = ui_study->current_interpolation;
  ui_study->series->time = ui_study->current_time;
  ui_study->series->duration = ui_study->current_duration;
  ui_study->series->zoom = ui_study->current_zoom;

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







