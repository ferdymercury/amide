/* ui_threshold.c
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
#include "ui_threshold2.h"
#include "ui_threshold_callbacks.h"


/* destroy a ui_threshold data structure */
void ui_threshold_free(ui_threshold_t ** pui_threshold) {

  g_free(*pui_threshold);
  *pui_threshold = NULL;

  return;
}

/* malloc and initialize a ui_threshold data structure */
ui_threshold_t * ui_threshold_init(void) {

  ui_threshold_t * ui_threshold;

  /* alloc space for the data structure for passing ui info */
  if ((ui_threshold = (ui_threshold_t *) g_malloc(sizeof(ui_threshold_t))) == NULL) {
    g_warning("%s: couldn't allocate space for ui_threshold_t",PACKAGE);
    return NULL;
  }

  /* set any needed parameters */
  ui_threshold->color_strip_image = NULL;

 return ui_threshold;
}

/* function to update the entry widgets */
void ui_threshold_update_entries(ui_study_t * ui_study) {

  gchar * string;
  amide_volume_t * volume;

  /* figure out which volume we're dealing with */
  /* we're just going to work with the first volume in the list */
  if (ui_study->current_volumes == NULL)
    volume = ui_study->study->volumes->volume;
  else
    volume = ui_study->current_volumes->volume;

  string = g_strdup_printf("%5.2f",(100*ui_study->threshold_max/
			   (volume->max- volume->min)));
  gtk_entry_set_text(GTK_ENTRY(ui_study->threshold->max_percentage), string);
  g_free(string);

  string = g_strdup_printf("%5.3f",ui_study->threshold_max);
  gtk_entry_set_text(GTK_ENTRY(ui_study->threshold->max_absolute), string);
  g_free(string);

  string = g_strdup_printf("%5.2f",(100*ui_study->threshold_min/
				    (volume->max - volume->min)));
  gtk_entry_set_text(GTK_ENTRY(ui_study->threshold->min_percentage), string);
  g_free(string);

  string = g_strdup_printf("%5.3f",ui_study->threshold_min);
  gtk_entry_set_text(GTK_ENTRY(ui_study->threshold->min_absolute), string);
  g_free(string);

}

/* function called to update the canvas */
void ui_threshold_update_canvas(ui_study_t * ui_study) {

  GdkImlibImage * temp_image;
  GnomeCanvasPoints * min_points;
  GnomeCanvasPoints * max_points;
  volume_data_t max;
  amide_volume_t * volume;

  /* figure out which volume we're dealing with */
  /* we're just going to work with the first volume in the list */
  if (ui_study->current_volumes == NULL)
    volume = ui_study->study->volumes->volume;
  else
    volume = ui_study->current_volumes->volume;
  
  temp_image = image_from_colortable(ui_study->color_table,
				     UI_THRESHOLD_COLOR_STRIP_WIDTH,
				     UI_THRESHOLD_COLOR_STRIP_HEIGHT,
				     ui_study->threshold_min,
				     ui_study->threshold_max,
				     volume->min,
				     volume->max);

  min_points = gnome_canvas_points_new(3);
  min_points->coords[0] = UI_THRESHOLD_COLOR_STRIP_WIDTH;
  min_points->coords[1] = UI_THRESHOLD_TRIANGLE_HEIGHT/2 + 
    UI_THRESHOLD_COLOR_STRIP_HEIGHT * 
    (1-ui_study->threshold_min/(volume->max-volume->min));
  min_points->coords[2] = UI_THRESHOLD_TRIANGLE_WIDTH + 
    UI_THRESHOLD_COLOR_STRIP_WIDTH;
  min_points->coords[3] = UI_THRESHOLD_TRIANGLE_HEIGHT +
    UI_THRESHOLD_COLOR_STRIP_HEIGHT * (1-ui_study->threshold_min/
     (volume->max-volume->min));
  min_points->coords[4] = UI_THRESHOLD_TRIANGLE_WIDTH + 
    UI_THRESHOLD_COLOR_STRIP_WIDTH;
  min_points->coords[5] = UI_THRESHOLD_COLOR_STRIP_HEIGHT * 
    (1-ui_study->threshold_min/
     (volume->max-volume->min));

  max_points = gnome_canvas_points_new(3);
  if (ui_study->threshold_max > volume->max) {
    max = volume->max;

    /* upward pointing max arrow */
    max_points->coords[0] = UI_THRESHOLD_COLOR_STRIP_WIDTH+
      UI_THRESHOLD_TRIANGLE_WIDTH/2;
    max_points->coords[1] = UI_THRESHOLD_COLOR_STRIP_HEIGHT * 
      (1-max/(volume->max-volume->min));
    max_points->coords[2] = UI_THRESHOLD_COLOR_STRIP_WIDTH;
    max_points->coords[3] = UI_THRESHOLD_TRIANGLE_HEIGHT +
      UI_THRESHOLD_COLOR_STRIP_HEIGHT * 
      (1-max/(volume->max-volume->min));
    max_points->coords[4] = max_points->coords[2]+
      UI_THRESHOLD_TRIANGLE_WIDTH;
    max_points->coords[5] = max_points->coords[3];

  }  else {
    max = ui_study->threshold_max;

    /* normal max arrow */
    max_points->coords[0] = UI_THRESHOLD_COLOR_STRIP_WIDTH;
    max_points->coords[1] = UI_THRESHOLD_TRIANGLE_HEIGHT/2 +
      UI_THRESHOLD_COLOR_STRIP_HEIGHT * 
      (1-max/(volume->max-volume->min));
    max_points->coords[2] = UI_THRESHOLD_TRIANGLE_WIDTH + 
      UI_THRESHOLD_COLOR_STRIP_WIDTH;
    max_points->coords[3] = UI_THRESHOLD_TRIANGLE_HEIGHT +
      UI_THRESHOLD_COLOR_STRIP_HEIGHT * 
      (1-max/(volume->max-volume->min));
    max_points->coords[4] = max_points->coords[2];
    max_points->coords[5] = max_points->coords[3]-UI_THRESHOLD_TRIANGLE_HEIGHT;
  }
  
  gnome_canvas_set_scroll_region(ui_study->threshold->color_strip, 0.0, 0.0,
				 temp_image->rgb_width + UI_THRESHOLD_TRIANGLE_WIDTH,
				 temp_image->rgb_height + UI_THRESHOLD_TRIANGLE_HEIGHT + 1);


  gtk_widget_set_usize(GTK_WIDGET(ui_study->threshold->color_strip),
		       temp_image->rgb_width + UI_THRESHOLD_TRIANGLE_WIDTH,
		       temp_image->rgb_height + UI_THRESHOLD_TRIANGLE_HEIGHT + 1);

  if (ui_study->threshold->color_strip_image != NULL) {
    gnome_canvas_item_set(ui_study->threshold->color_strip_image,
			  "image", temp_image,
			  "width", (double) temp_image->rgb_width,
			  "height", (double) temp_image->rgb_height,
			  NULL);
    gnome_canvas_item_set(ui_study->threshold->min_arrow,
			  "points", min_points,
			  NULL);
    gnome_canvas_item_set(ui_study->threshold->max_arrow,
			  "points", max_points,
			  NULL);
  } else {
    ui_study->threshold->color_strip_image = 
      gnome_canvas_item_new(gnome_canvas_root(ui_study->threshold->color_strip),
			    gnome_canvas_image_get_type(),
			    "image", temp_image,
			    "x", 0.0,
			    "y", ((double) UI_THRESHOLD_TRIANGLE_HEIGHT/2),
			    "anchor", GTK_ANCHOR_NORTH_WEST,
			    "width", (double) temp_image->rgb_width,
			    "height", (double) temp_image->rgb_height,
			    NULL);
    ui_study->threshold->min_arrow = 
      gnome_canvas_item_new(gnome_canvas_root(ui_study->threshold->color_strip),
			    gnome_canvas_polygon_get_type(),
			    "points", min_points,
			    "fill_color", "white",
			    "outline_color", "black",
			    "width_pixels", 2,
			    NULL);
    gtk_signal_connect(GTK_OBJECT(ui_study->threshold->min_arrow), "event",
		       GTK_SIGNAL_FUNC(ui_threshold_callbacks_min_arrow),
		       ui_study);
    ui_study->threshold->max_arrow = 
      gnome_canvas_item_new(gnome_canvas_root(ui_study->threshold->color_strip),
			    gnome_canvas_polygon_get_type(),
			    "points", max_points,
			    "fill_color", "black",
			    "outline_color", "black",
			    "width_pixels", 2,
			    NULL);
    gtk_signal_connect(GTK_OBJECT(ui_study->threshold->max_arrow), "event",
		       GTK_SIGNAL_FUNC(ui_threshold_callbacks_max_arrow),
		       ui_study);
  }

  gnome_canvas_points_unref(min_points);
  gnome_canvas_points_unref(max_points);
  gnome_canvas_destroy_image(temp_image);

  /* draw the little triangle things representing min and max */

  return;
} 

/* function that sets up the thresholding dialog */
void ui_threshold_create(ui_study_t * ui_study) {
  
  GnomeApp * app;
  gchar * title = NULL;
  GtkWidget * packing_table;
  GnomeCanvas * color_strip;
  GnomeCanvas * bar_graph;
  GdkImlibImage * temp_image;
  GtkWidget * label;
  GtkWidget * entry;
  amide_volume_t * volume;
  
  /* sanity check */
  if (ui_study->study->volumes == NULL)
    return;
  if (ui_study->threshold != NULL)
    return;

  ui_study->threshold = ui_threshold_init();

  title = g_strdup_printf("Threshold: %s",ui_study->study->name);
  app = GNOME_APP(gnome_app_new(PACKAGE, title));
  free(title);
  ui_study->threshold->app = app;

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(app), "delete_event",
		     GTK_SIGNAL_FUNC(ui_threshold_callbacks_delete_event),
		     ui_study);

  /* make the widgets for this dialog box */
  packing_table = gtk_table_new(5,5,FALSE);
  gnome_app_set_contents(app, GTK_WIDGET(packing_table));

  color_strip = GNOME_CANVAS(gnome_canvas_new());
  ui_study->threshold->color_strip = color_strip; /* save this for future use */
  ui_threshold_update_canvas(ui_study); /* fill the canvas with the current
					      color map, etc.*/
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(color_strip), 0,1,0,1,
		   X_PACKING_OPTIONS | GTK_FILL,
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);


  /* figure out which volume we're dealing with */
  if (ui_study->current_volume == NULL)
    volume = ui_study->study->volumes->volume;
  else
    volume = ui_study->current_volume;

  temp_image = image_of_distribution(volume,
				     UI_THRESHOLD_BAR_GRAPH_WIDTH,
				     UI_THRESHOLD_COLOR_STRIP_HEIGHT);
  bar_graph = GNOME_CANVAS(gnome_canvas_new());
  gnome_canvas_set_scroll_region(bar_graph, 
				 0.0, UI_THRESHOLD_TRIANGLE_WIDTH/2,
				 (gdouble) temp_image->rgb_width,
				 UI_THRESHOLD_TRIANGLE_WIDTH/2.0+ (gdouble) temp_image->rgb_height);
  
  gtk_widget_set_usize(GTK_WIDGET(bar_graph),
		       temp_image->rgb_width,
		       temp_image->rgb_height + UI_THRESHOLD_TRIANGLE_HEIGHT/2);

  gnome_canvas_item_new(gnome_canvas_root(bar_graph),
			gnome_canvas_image_get_type(),
			"image", temp_image,
			"x", 0.0,
			"y", ((double) UI_THRESHOLD_TRIANGLE_HEIGHT/2),
			"anchor", GTK_ANCHOR_NORTH_WEST,
			"width", (double) temp_image->rgb_width,
			"height", (double) temp_image->rgb_height,
			NULL);

  gnome_canvas_destroy_image(temp_image);

  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(bar_graph), 1,3,0,1,
		   X_PACKING_OPTIONS | GTK_FILL,
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);

  /* max threshold stuff */
  label = gtk_label_new("Percent");
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(label), 1,2,1,2,
		   X_PACKING_OPTIONS | GTK_FILL,
		   0,
		   X_PADDING, Y_PADDING);

  label = gtk_label_new("Absolute");
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(label), 2,3,1,2,
		   X_PACKING_OPTIONS | GTK_FILL,
		   0,
		   X_PADDING, Y_PADDING);

  label = gtk_label_new("Max Threshold");
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(label), 0,1,2,3,
		   X_PACKING_OPTIONS | GTK_FILL,
		   0,
		   X_PADDING, Y_PADDING);


  entry = gtk_entry_new();
  gtk_widget_set_usize(GTK_WIDGET(entry), UI_THRESHOLD_DEFAULT_ENTRY_WIDTH,0);
  gtk_signal_connect(GTK_OBJECT(entry), "activate", 
  		     GTK_SIGNAL_FUNC(ui_threshold_callbacks_max_percentage), 
  		     ui_study);
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(entry), 1,2,2,3,
		   X_PACKING_OPTIONS | GTK_FILL,
		   0,
		   X_PADDING, Y_PADDING);
  ui_study->threshold->max_percentage = entry;

  entry = gtk_entry_new();
  gtk_widget_set_usize(GTK_WIDGET(entry), UI_THRESHOLD_DEFAULT_ENTRY_WIDTH,0);
  gtk_signal_connect(GTK_OBJECT(entry), "activate", 
  		     GTK_SIGNAL_FUNC(ui_threshold_callbacks_max_absolute), 
  		     ui_study);
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(entry), 2,3,2,3,
		   X_PACKING_OPTIONS | GTK_FILL,
		   0,
		   X_PADDING, Y_PADDING);
  ui_study->threshold->max_absolute = entry;




  /* min threshold stuff */
  label = gtk_label_new("Min Threshold");
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(label), 0,1,3,4,
		   X_PACKING_OPTIONS | GTK_FILL,
		   0,
		   X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  gtk_widget_set_usize(GTK_WIDGET(entry), UI_THRESHOLD_DEFAULT_ENTRY_WIDTH,0);
  gtk_signal_connect(GTK_OBJECT(entry), "activate", 
  		     GTK_SIGNAL_FUNC(ui_threshold_callbacks_min_percentage), 
  		     ui_study);
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(entry), 1,2,3,4,
		   X_PACKING_OPTIONS | GTK_FILL,
		   0,
		   X_PADDING, Y_PADDING);
  ui_study->threshold->min_percentage = entry;

  entry = gtk_entry_new();
  gtk_widget_set_usize(GTK_WIDGET(entry), UI_THRESHOLD_DEFAULT_ENTRY_WIDTH,0);
  gtk_signal_connect(GTK_OBJECT(entry), "activate", 
  		     GTK_SIGNAL_FUNC(ui_threshold_callbacks_min_absolute), 
  		     ui_study);
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(entry), 2,3,3,4,
		   X_PACKING_OPTIONS | GTK_FILL,
		   0,
		   X_PADDING, Y_PADDING);
  ui_study->threshold->min_absolute = entry;

  /* and put valid entries into all the widgets */
  ui_threshold_update_entries(ui_study);

  /* and show all our widgets */
  gtk_widget_show_all(GTK_WIDGET(app));

  return;
}


