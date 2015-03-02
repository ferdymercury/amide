/* ui_series.c
 *
 * Part of amide - Amide's a Medical Image Dataset Viewer
 * Copyright (C) 2000 Andy Loening
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
#include "volume.h"
#include "roi.h"
#include "study.h"
#include "color_table.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_study_rois.h"
#include "ui_study.h"
#include "ui_series2.h"
#include "ui_series_callbacks.h"

/* destroy the ui_series slices data */
void ui_series_slices_free(ui_series_t * ui_series) {
  guint i;

  for (i=0; i < ui_series->num_slices; i++) {
    if (ui_series->slices[i] != NULL)
      volume_free(&(ui_series->slices[i]));
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

  return ui_series;
}


/* function to make the adjustments for the scrolling scale */
GtkAdjustment * ui_series_create_scroll_adjustment(ui_study_t * ui_study) { 


  realpoint_t new_axis[NUM_AXIS], view_voxel_size;
  floatpoint_t length, voxel_length, thickness;
  axis_t j;
  GtkAdjustment * new_adjustment;

  /* sanity check */
  if (*(ui_study->series->pvolume) == NULL)
    return NULL;

  for (j=0;j<NUM_AXIS;j++)
    new_axis[j] = realspace_get_orthogonal_axis(ui_study->current_coord_frame.axis,
						ui_study->series->view,j);
  length = volume_get_length(*(ui_study->series->pvolume), new_axis);

  /* get the length of a voxel for the given axis*/
  view_voxel_size = 
    realspace_base_coord_to_alt((*(ui_study->series->pvolume))->voxel_size,
				ui_study->current_coord_frame);
  voxel_length = REALSPACE_DOT_PRODUCT(view_voxel_size,view_voxel_size);

  /* use the most appropriate thickness */
  if (ui_study->current_thickness <= voxel_length)
    thickness = voxel_length;
  else
    thickness = ui_study->current_thickness;

  new_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, length, 
						     thickness, thickness,
						     thickness));
  return new_adjustment;
}

/* funtion to update the canvas iamge */
void ui_series_update_canvas_image(ui_study_t * ui_study) {

  realpoint_t new_axis[NUM_AXIS];
  floatpoint_t zp_start;
  floatpoint_t zp_length, voxel_length;
  floatpoint_t thickness;
  realpoint_t view_voxel_size;
  guint i, start_i;
  gint width, height;
  gdouble x, y;
  axis_t k;
  GtkRequisition requisition;
  GdkCursor * cursor;

  /* sanity check */
  if (*(ui_study->series->pvolume) == NULL)
    return;

  /* push our desired cursor onto the cursor stack */
  cursor = ui_study->cursor[UI_STUDY_WAIT];
  ui_study->cursor_stack = g_slist_prepend(ui_study->cursor_stack,cursor);
  gdk_window_set_cursor(gtk_widget_get_parent_window(GTK_WIDGET(ui_study->series->canvas)), cursor);
  
  /* do any events pending, this allows the cursor to get displayed */
  while (gtk_events_pending()) 
    gtk_main_iteration();


  /* get the view axis to use*/
  for (k=0;k<NUM_AXIS;k++)
    new_axis[k] = realspace_get_orthogonal_axis(ui_study->current_coord_frame.axis,
						ui_study->series->view,k);

  zp_length = volume_get_length(*(ui_study->series->pvolume), new_axis);
  width = 0.8*gdk_screen_width();
  height = 0.8*gdk_screen_height();

  /* get the length of a voxel for the given axis*/
  view_voxel_size = 
    realspace_base_coord_to_alt((*(ui_study->series->pvolume))->voxel_size,
				ui_study->current_coord_frame);
  voxel_length = REALSPACE_DOT_PRODUCT(view_voxel_size,view_voxel_size);

  /* get the total number of images */
  if (ui_study->current_thickness <= voxel_length)
    thickness = voxel_length;
  else
    thickness = ui_study->current_thickness;
  ui_study->series->num_slices = ceil(zp_length/thickness);

  /* allocate space for pointers to our slices if needed */
  if (ui_study->series->slices == NULL) {
    if ((ui_study->series->slices = 
	 (amide_volume_t **) g_malloc(sizeof(amide_volume_t *)
				      *ui_study->series->num_slices)) == NULL) {
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
	 (GdkImlibImage **) g_malloc(sizeof(GdkImlibImage *)
				       *ui_study->series->num_slices)) 
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
  ui_study->series->rgb_images[0] = 
    image_from_volume(&(ui_study->series->slices[0]),
		      *(ui_study->series->pvolume),
		      ui_study->current_frame,
		      0.0,
		      thickness,
		      new_axis,
		      ui_study->scaling,
		      ui_study->color_table,
		      ui_study->current_zoom,
		      ui_study->current_interpolation,
		      ui_study->threshold_min,
		      ui_study->threshold_max);


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
  i = ui_study->series->num_slices*(ui_study->series->zp_start/zp_length);
  if (ui_study->series->num_slices <= 
      ui_study->series->columns*ui_study->series->rows)
    i = 0;
  else if (i > ui_study->series->num_slices - 
	   ui_study->series->columns*ui_study->series->rows)
    i = ui_study->series->num_slices - 
      ui_study->series->columns*ui_study->series->rows;
  if (i < 0)
    i = 0;

  start_i = i;
  x = y = 0.0;

  for (; ((i-start_i) < (ui_study->series->rows*ui_study->series->columns))
	 & (i < ui_study->series->num_slices); i++) {
    zp_start = i*thickness;
    
    if (i != 0) /* we've already done image 0 */
      if (ui_study->series->rgb_images[i] != NULL)
	gnome_canvas_destroy_image(ui_study->series->rgb_images[i]);
    
    if (i != 0) /* we've already done image 0 */
      ui_study->series->rgb_images[i] = 
	image_from_volume(&(ui_study->series->slices[i]),
			  *(ui_study->series->pvolume),
			  ui_study->current_frame,
			  zp_start,
			  thickness,
			  new_axis,
			  ui_study->scaling,
			  ui_study->color_table,
			  ui_study->current_zoom,
			  ui_study->current_interpolation,
			  ui_study->threshold_min,
			  ui_study->threshold_max);
    
    
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
  gdk_window_set_cursor(gtk_widget_get_parent_window(GTK_WIDGET(ui_study->series->canvas)), cursor);

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

  /* sanity checks */
  if (ui_study->study->volumes == NULL)
    return;
  if (ui_study->study->volumes->volume == NULL)
    return;
  if (ui_study->series != NULL)
    return;


  ui_study->series = ui_series_init();
  ui_study->series->view = view;

  /* pick the appropriate volume */
  if (ui_study->current_volume == NULL)
    ui_study->series->pvolume = &(ui_study->study->volumes->volume); 
  else 
    ui_study->series->pvolume = &(ui_study->current_volume);

  title = g_strdup_printf("Series: %s (%s)",ui_study->study->name, 
			  view_names[view]);
  app = GNOME_APP(gnome_app_new(PACKAGE, title));
  free(title);
  ui_study->series->app = app;

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(app), "delete_event",
		     GTK_SIGNAL_FUNC(ui_series_callbacks_delete_event),
		     ui_study);

  /* make the widgets for this dialog box */
  packing_table = gtk_table_new(1,2,FALSE);
  gnome_app_set_contents(app, GTK_WIDGET(packing_table));


  series_canvas = GNOME_CANVAS(gnome_canvas_new());
  ui_study->series->canvas = series_canvas; /* save for future use */
  ui_study->series->zp_start = 0.0;
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









