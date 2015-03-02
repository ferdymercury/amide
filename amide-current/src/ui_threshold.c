/* ui_threshold.c
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
#include "amide.h"
#include "realspace.h"
#include "color_table.h"
#include "volume.h"
#include "roi.h"
#include "study.h"
#include "rendering.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_roi.h"
#include "ui_volume.h"
#include "ui_study.h"
#include "ui_threshold2.h"
#include "ui_threshold_callbacks.h"
#include "ui_study_callbacks.h"

/* destroy a ui_threshold data structure */
ui_threshold_t * ui_threshold_free(ui_threshold_t * ui_threshold) {

  if (ui_threshold == NULL)
    return ui_threshold;

  /* sanity checks */
  g_return_val_if_fail(ui_threshold->reference_count > 0, NULL);

  /* remove a reference count */
  ui_threshold->reference_count--;

  /* things we always do */
  ui_threshold->volume = volume_free(ui_threshold->volume);

  /* if we've removed all reference's,f ree the structure */
  if (ui_threshold->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing ui_threshold\n");
#endif
    g_free(ui_threshold);
  }

  return ui_threshold;
}

/* malloc and initialize a ui_threshold data structure */
ui_threshold_t * ui_threshold_init(void) {

  ui_threshold_t * ui_threshold;

  /* alloc space for the data structure for passing ui info */
  if ((ui_threshold = (ui_threshold_t *) g_malloc(sizeof(ui_threshold_t))) == NULL) {
    g_warning("%s: couldn't allocate space for ui_threshold_t",PACKAGE);
    return NULL;
  }
  ui_threshold->reference_count = 1;

  /* set any needed parameters */
  ui_threshold->color_strip_image = NULL;
  ui_threshold->volume = NULL;

 return ui_threshold;
}

/* function to update the entry widgets */
void ui_threshold_update_entries(ui_threshold_t * ui_threshold) {

  gchar * string;
  volume_t * volume;

  volume =ui_threshold->volume;

  string = g_strdup_printf("%5.2f",(100*(volume->threshold_max-volume->min)/
			   (volume->max- volume->min)));
  gtk_entry_set_text(GTK_ENTRY(ui_threshold->max_percentage), string);
  g_free(string);

  string = g_strdup_printf("%5.3f",volume->threshold_max);
  gtk_entry_set_text(GTK_ENTRY(ui_threshold->max_absolute), string);
  g_free(string);

  string = g_strdup_printf("%5.2f",(100*(volume->threshold_min-volume->min)/
				    (volume->max - volume->min)));
  gtk_entry_set_text(GTK_ENTRY(ui_threshold->min_percentage), string);
  g_free(string);

  string = g_strdup_printf("%5.3f",volume->threshold_min);
  gtk_entry_set_text(GTK_ENTRY(ui_threshold->min_absolute), string);
  g_free(string);

}

/* function called to update the canvas */
void ui_threshold_update_canvas(ui_study_t * ui_study, ui_threshold_t * ui_threshold) {

  GdkImlibImage * temp_image;
  GnomeCanvasPoints * min_points;
  GnomeCanvasPoints * max_points;
  volume_data_t max;
  volume_t * volume;

  volume = ui_threshold->volume;
  
  temp_image = image_from_colortable(volume->color_table,
				     UI_THRESHOLD_COLOR_STRIP_WIDTH,
				     UI_THRESHOLD_COLOR_STRIP_HEIGHT,
				     volume->threshold_min,
				     volume->threshold_max,
				     volume->min,
				     volume->max);


  /* the min arrow */
  min_points = gnome_canvas_points_new(3);
  min_points->coords[0] = UI_THRESHOLD_COLOR_STRIP_WIDTH;
  min_points->coords[1] = UI_THRESHOLD_TRIANGLE_HEIGHT/2 + 
    UI_THRESHOLD_COLOR_STRIP_HEIGHT * 
    (1-(volume->threshold_min-volume->min)/(volume->max-volume->min));
  min_points->coords[2] = UI_THRESHOLD_TRIANGLE_WIDTH + 
    UI_THRESHOLD_COLOR_STRIP_WIDTH;
  min_points->coords[3] = UI_THRESHOLD_TRIANGLE_HEIGHT +
    UI_THRESHOLD_COLOR_STRIP_HEIGHT * 
    (1-(volume->threshold_min-volume->min)/(volume->max-volume->min));
  min_points->coords[4] = UI_THRESHOLD_TRIANGLE_WIDTH + 
    UI_THRESHOLD_COLOR_STRIP_WIDTH;
  min_points->coords[5] = UI_THRESHOLD_COLOR_STRIP_HEIGHT * 
    (1-(volume->threshold_min-volume->min)/(volume->max-volume->min));

  max_points = gnome_canvas_points_new(3);
  if (volume->threshold_max > volume->max) {
    max = volume->max;

    /* upward pointing max arrow */
    max_points->coords[0] = UI_THRESHOLD_COLOR_STRIP_WIDTH+
      UI_THRESHOLD_TRIANGLE_WIDTH/2;
    max_points->coords[1] = UI_THRESHOLD_COLOR_STRIP_HEIGHT * 
      (1-(max-volume->min)/(volume->max-volume->min));
    max_points->coords[2] = UI_THRESHOLD_COLOR_STRIP_WIDTH;
    max_points->coords[3] = UI_THRESHOLD_TRIANGLE_HEIGHT +
      UI_THRESHOLD_COLOR_STRIP_HEIGHT * 
      (1-(max-volume->min)/(volume->max-volume->min));
    max_points->coords[4] = max_points->coords[2]+
      UI_THRESHOLD_TRIANGLE_WIDTH;
    max_points->coords[5] = max_points->coords[3];

  }  else {
    max = volume->threshold_max;

    /* normal max arrow */
    max_points->coords[0] = UI_THRESHOLD_COLOR_STRIP_WIDTH;
    max_points->coords[1] = UI_THRESHOLD_TRIANGLE_HEIGHT/2 +
      UI_THRESHOLD_COLOR_STRIP_HEIGHT * 
      (1-(max-volume->min)/(volume->max-volume->min));
    max_points->coords[2] = UI_THRESHOLD_TRIANGLE_WIDTH + 
      UI_THRESHOLD_COLOR_STRIP_WIDTH;
    max_points->coords[3] = UI_THRESHOLD_TRIANGLE_HEIGHT +
      UI_THRESHOLD_COLOR_STRIP_HEIGHT * 
      (1-(max-volume->min)/(volume->max-volume->min));
    max_points->coords[4] = max_points->coords[2];
    max_points->coords[5] = max_points->coords[3]-UI_THRESHOLD_TRIANGLE_HEIGHT;
  }
  
  gnome_canvas_set_scroll_region(ui_threshold->color_strip, 0.0, 0.0,
				 temp_image->rgb_width + UI_THRESHOLD_TRIANGLE_WIDTH,
				 temp_image->rgb_height + UI_THRESHOLD_TRIANGLE_HEIGHT + 1);


  gtk_widget_set_usize(GTK_WIDGET(ui_threshold->color_strip),
		       temp_image->rgb_width + UI_THRESHOLD_TRIANGLE_WIDTH,
		       temp_image->rgb_height + UI_THRESHOLD_TRIANGLE_HEIGHT + 1);

  if (ui_threshold->color_strip_image != NULL) {
    gnome_canvas_item_set(ui_threshold->color_strip_image,
			  "image", temp_image,
			  "width", (double) temp_image->rgb_width,
			  "height", (double) temp_image->rgb_height,
			  NULL);
    gnome_canvas_item_set(ui_threshold->min_arrow,
			  "points", min_points,
			  NULL);
    gnome_canvas_item_set(ui_threshold->max_arrow,
			  "points", max_points,
			  NULL);
  } else {
    ui_threshold->color_strip_image = 
      gnome_canvas_item_new(gnome_canvas_root(ui_threshold->color_strip),
			    gnome_canvas_image_get_type(),
			    "image", temp_image,
			    "x", 0.0,
			    "y", ((double) UI_THRESHOLD_TRIANGLE_HEIGHT/2),
			    "anchor", GTK_ANCHOR_NORTH_WEST,
			    "width", (double) temp_image->rgb_width,
			    "height", (double) temp_image->rgb_height,
			    NULL);
    ui_threshold->min_arrow = 
      gnome_canvas_item_new(gnome_canvas_root(ui_threshold->color_strip),
			    gnome_canvas_polygon_get_type(),
			    "points", min_points,
			    "fill_color", "white",
			    "outline_color", "black",
			    "width_pixels", 2,
			    NULL);
    gtk_object_set_data(GTK_OBJECT(ui_threshold->min_arrow), "type", GINT_TO_POINTER(MIN_ARROW));
    gtk_object_set_data(GTK_OBJECT(ui_threshold->min_arrow), "ui_study", ui_study);
    gtk_signal_connect(GTK_OBJECT(ui_threshold->min_arrow), "event",
		       GTK_SIGNAL_FUNC(ui_threshold_callbacks_arrow),
		       ui_threshold);
    ui_threshold->max_arrow = 
      gnome_canvas_item_new(gnome_canvas_root(ui_threshold->color_strip),
			    gnome_canvas_polygon_get_type(),
			    "points", max_points,
			    "fill_color", "black",
			    "outline_color", "black",
			    "width_pixels", 2,
			    NULL);
    gtk_object_set_data(GTK_OBJECT(ui_threshold->max_arrow), "type", GINT_TO_POINTER(MAX_ARROW));
    gtk_object_set_data(GTK_OBJECT(ui_threshold->max_arrow), "ui_study", ui_study);
    gtk_signal_connect(GTK_OBJECT(ui_threshold->max_arrow), "event",
		       GTK_SIGNAL_FUNC(ui_threshold_callbacks_arrow),
		       ui_threshold);
  }

  gnome_canvas_points_unref(min_points);
  gnome_canvas_points_unref(max_points);
  gnome_canvas_destroy_image(temp_image);

  /* draw the little triangle things representing min and max */

  return;
} 


/* function to update the whole dialog */
void ui_threshold_dialog_update(ui_study_t * ui_study) {

  volume_t * volume;
  gchar * temp_string;
  GdkImlibImage * temp_image;
    

  /* sanity check */
  if (study_get_volumes(ui_study->study) == NULL)
    return;
  if (ui_study->threshold == NULL) { /* looks like we called the wrong function */
    ui_threshold_dialog_create(ui_study);
    return;
  }

  /* figure out which volume we're dealing with */
  if (ui_study->current_volume == NULL)
    volume = study_get_first_volume(ui_study->study);
  else
    volume = ui_study->current_volume;
  ui_study->threshold->volume = volume_free(ui_study->threshold->volume);
  ui_study->threshold->volume = volume_add_reference(volume);

  /* reset the label which holds the volume name */
  temp_string = g_strdup_printf("data set: %s\n",volume->name);
  gtk_label_set_text(GTK_LABEL(ui_study->threshold->name_label), temp_string);
  g_free(temp_string);


  /* reset the color strip */
  ui_threshold_update_canvas(ui_study, ui_study->threshold); 

  /* reset the distribution image */
  temp_image = image_of_distribution(volume,
				     UI_THRESHOLD_BAR_GRAPH_WIDTH,
				     UI_THRESHOLD_COLOR_STRIP_HEIGHT);
  gnome_canvas_item_set(ui_study->threshold->bar_graph_item,
			"image", temp_image, NULL);
  gnome_canvas_destroy_image(temp_image);



  /* and update what's in the entry widgets */
  ui_threshold_update_entries(ui_study->threshold);

  return;
}


/* function that sets up the thresholding/colormap widgets */
GtkWidget * ui_threshold_create(ui_study_t * ui_study, ui_threshold_t * ui_threshold) {

  GtkWidget * right_table;
  GtkWidget * left_table;
  GtkWidget * main_box;
  guint right_row=0;
  guint left_row=0;
  GnomeCanvas * color_strip;
  GnomeCanvas * bar_graph;
  GdkImlibImage * temp_image;
  GtkWidget * label;
  GtkWidget * entry;
  GtkWidget * option_menu;
  GtkWidget * menu;
  GtkWidget * menuitem;
  color_table_t i_color_table;


  /* we're using two tables packed into a horizontal box */
  main_box = gtk_hbox_new(FALSE,0);



  /*---------------------------------------
    left table 
    --------------------------------------- */

  left_table = gtk_table_new(7,3,FALSE);
  gtk_box_pack_start(GTK_BOX(main_box), left_table, TRUE,TRUE,0);


  label = gtk_label_new("Percent");
  gtk_table_attach(GTK_TABLE(left_table), 
		   GTK_WIDGET(label), 1,2,left_row,left_row+1,
		   X_PACKING_OPTIONS | GTK_FILL,
		   0,
		   X_PADDING, Y_PADDING);

  label = gtk_label_new("Absolute");
  gtk_table_attach(GTK_TABLE(left_table), 
		   GTK_WIDGET(label), 2,3,left_row,left_row+1,
		   X_PACKING_OPTIONS | GTK_FILL,
		   0,
		   X_PADDING, Y_PADDING);
  left_row++;


  label = gtk_label_new("Max Threshold");
  gtk_table_attach(GTK_TABLE(left_table), 
		   GTK_WIDGET(label), 0,1,left_row,left_row+1,
		   X_PACKING_OPTIONS | GTK_FILL,
		   0,
		   X_PADDING, Y_PADDING);


  entry = gtk_entry_new();
  gtk_widget_set_usize(GTK_WIDGET(entry), UI_STUDY_DEFAULT_ENTRY_WIDTH,0);
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(MAX_PERCENT));
  gtk_object_set_data(GTK_OBJECT(entry), "ui_study", ui_study);
  gtk_signal_connect(GTK_OBJECT(entry), "activate", 
  		     GTK_SIGNAL_FUNC(ui_threshold_callbacks_entry), 
  		     ui_threshold);
  gtk_table_attach(GTK_TABLE(left_table), 
		   GTK_WIDGET(entry), 1,2,left_row,left_row+1,
		   X_PACKING_OPTIONS | GTK_FILL,
		   0,
		   X_PADDING, Y_PADDING);
  ui_threshold->max_percentage = entry;

  entry = gtk_entry_new();
  gtk_widget_set_usize(GTK_WIDGET(entry), UI_STUDY_DEFAULT_ENTRY_WIDTH,0);
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(MAX_ABSOLUTE));
  gtk_object_set_data(GTK_OBJECT(entry), "ui_study", ui_study);
  gtk_signal_connect(GTK_OBJECT(entry), "activate", 
  		     GTK_SIGNAL_FUNC(ui_threshold_callbacks_entry), 
  		     ui_threshold);
  gtk_table_attach(GTK_TABLE(left_table), 
		   GTK_WIDGET(entry), 2,3,left_row,left_row+1,
		   X_PACKING_OPTIONS | GTK_FILL,
		   0,
		   X_PADDING, Y_PADDING);
  ui_threshold->max_absolute = entry;

  left_row++;

  /* min threshold stuff */
  label = gtk_label_new("Min Threshold");
  gtk_table_attach(GTK_TABLE(left_table), 
		   GTK_WIDGET(label), 0,1, left_row,left_row+1,
		   X_PACKING_OPTIONS | GTK_FILL,
		   0,
		   X_PADDING, Y_PADDING);

  entry = gtk_entry_new();
  gtk_widget_set_usize(GTK_WIDGET(entry), UI_STUDY_DEFAULT_ENTRY_WIDTH,0);
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(MIN_PERCENT));
  gtk_object_set_data(GTK_OBJECT(entry), "ui_study", ui_study);
  gtk_signal_connect(GTK_OBJECT(entry), "activate", 
  		     GTK_SIGNAL_FUNC(ui_threshold_callbacks_entry),
  		     ui_threshold);
  gtk_table_attach(GTK_TABLE(left_table), 
		   GTK_WIDGET(entry), 1,2, left_row,left_row+1,
		   X_PACKING_OPTIONS | GTK_FILL,
		   0,
		   X_PADDING, Y_PADDING);
  ui_threshold->min_percentage = entry;

  entry = gtk_entry_new();
  gtk_widget_set_usize(GTK_WIDGET(entry), UI_STUDY_DEFAULT_ENTRY_WIDTH,0);
  gtk_object_set_data(GTK_OBJECT(entry), "type", GINT_TO_POINTER(MIN_ABSOLUTE));
  gtk_object_set_data(GTK_OBJECT(entry), "ui_study", ui_study);
  gtk_signal_connect(GTK_OBJECT(entry), "activate", 
  		     GTK_SIGNAL_FUNC(ui_threshold_callbacks_entry),
  		     ui_threshold);
  gtk_table_attach(GTK_TABLE(left_table), 
		   GTK_WIDGET(entry), 2,3, left_row, left_row+1,
		   X_PACKING_OPTIONS | GTK_FILL,
		   0,
		   X_PADDING, Y_PADDING);
  ui_threshold->min_absolute = entry;

  left_row++;

  /* and put valid entries into all the widgets */
  ui_threshold_update_entries(ui_threshold);



  /* color table selector */
  label = gtk_label_new("color table:");
  gtk_table_attach(GTK_TABLE(left_table), 
		   GTK_WIDGET(label), 0,1, 
		   left_row,left_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();

  for (i_color_table=0; i_color_table<NUM_COLOR_TABLES; i_color_table++) {
    menuitem = gtk_menu_item_new_with_label(color_table_names[i_color_table]);
    gtk_menu_append(GTK_MENU(menu), menuitem);
    gtk_object_set_data(GTK_OBJECT(menuitem), "color_table", GINT_TO_POINTER(i_color_table));
    gtk_object_set_data(GTK_OBJECT(menuitem),"threshold", ui_threshold);
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
    		       GTK_SIGNAL_FUNC(ui_study_callbacks_color_table), ui_study);
  }

  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), ui_threshold->volume->color_table);
  ui_threshold->color_table_menu = option_menu;

  gtk_table_attach(GTK_TABLE(left_table), 
		   GTK_WIDGET(option_menu), 1,3, 
		   left_row,left_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);

  left_row++;




  

  /*---------------------------------------
    right table 
    --------------------------------------- */


  right_table = gtk_table_new(7,2,FALSE);
  gtk_box_pack_start(GTK_BOX(main_box), right_table, TRUE,TRUE,0);


  label = gtk_label_new("distribution (log)");
  gtk_table_attach(GTK_TABLE(right_table), 
		   GTK_WIDGET(label), 0,1, right_row, right_row+1,
		   X_PACKING_OPTIONS | GTK_FILL,
		   0,
		   X_PADDING, Y_PADDING);

  right_row++;


  /* the distribution image */
  temp_image = image_of_distribution(ui_threshold->volume,
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

  ui_threshold->bar_graph_item = 
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

  gtk_table_attach(GTK_TABLE(right_table), 
		   GTK_WIDGET(bar_graph), 0,1,right_row,right_row+1,
		   X_PACKING_OPTIONS | GTK_FILL,
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);



  /* the color strip */
  color_strip = GNOME_CANVAS(gnome_canvas_new());
  ui_threshold->color_strip = color_strip; /* save this for future use */
  ui_threshold_update_canvas(ui_study, ui_threshold); /* fill the canvas with the current
							 color map, etc.*/
  gtk_table_attach(GTK_TABLE(right_table), 
		   GTK_WIDGET(color_strip), 1,2,right_row,right_row+1,
		   X_PACKING_OPTIONS | GTK_FILL,
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);


  right_row++;


  return main_box;
}

/* function that sets up the thresholding dialog */
void ui_threshold_dialog_create(ui_study_t * ui_study) {
  
  GnomeApp * app;
  gchar * temp_string = NULL;
  GtkWidget * threshold_widget;
  GtkWidget * vbox;
  GtkWidget * label;
  volume_t * volume;
  ui_volume_list_t * ui_volume_list_item;
  
  /* sanity check */
  if (study_get_volumes(ui_study->study) == NULL)
    return;
  if (ui_study->threshold != NULL)
    return;
  if (ui_study->current_volumes == NULL)
    return;

  /* figure out which volume we're dealing with */
  if (ui_study->current_volume == NULL)
    volume = ui_study->current_volumes->volume;
  else
    volume = ui_study->current_volume;

  /* make sure we don't already have a volume edit dialog up */
  ui_volume_list_item = ui_volume_list_get_ui_volume(ui_study->current_volumes,volume);
  if (ui_volume_list_item != NULL)
    if (ui_volume_list_item->dialog != NULL)
      return;

  ui_study->threshold = ui_threshold_init();

  temp_string = g_strdup_printf("Threshold: %s",study_get_name(ui_study->study));
  app = GNOME_APP(gnome_app_new(PACKAGE, temp_string));
  g_free(temp_string);
  ui_study->threshold->app = app;
  ui_study->threshold->volume = volume_add_reference(volume);

  g_print("threshold for volume %s\n",volume->name);

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(app), "delete_event",
		     GTK_SIGNAL_FUNC(ui_threshold_callbacks_delete_event),
		     ui_study);

  /* setup the vertical packing box */
  vbox = gtk_vbox_new(FALSE,0);
  gnome_app_set_contents(app, GTK_WIDGET(vbox));


  /* make the volume title thingie */
  temp_string = g_strdup_printf("data set: %s\n",volume->name);
  label = gtk_label_new(temp_string);
  g_free(temp_string);
  gtk_box_pack_start(GTK_BOX(vbox), label, TRUE,TRUE,0);
  ui_study->threshold->name_label = label; 


  /* setup the threshold widget */
  threshold_widget = ui_threshold_create(ui_study, ui_study->threshold);
  gtk_box_pack_start(GTK_BOX(vbox), threshold_widget, TRUE,TRUE,0);


  /* and show all our widgets */
  gtk_widget_show_all(GTK_WIDGET(app));

  return;
}


