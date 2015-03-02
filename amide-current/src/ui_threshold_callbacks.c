/* ui_threshold_callbacks.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
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
#include "color_table.h"
#include "volume.h"
#include "roi.h"
#include "study.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_study_rois.h"
#include "ui_study_volumes.h"
#include "ui_study.h"
#include "ui_threshold2.h"
#include "ui_series2.h"
#include "ui_threshold_callbacks.h"


/* function called when the max triangle is moved 
 * mostly taken from Pennington's fine book */
gint ui_threshold_callbacks_max_arrow(GtkWidget* widget, 
				      GdkEvent * event,
				      gpointer data) {

  ui_study_t * ui_study = data;
  gdouble item_x, item_y;
  GdkCursor * cursor;
  volume_data_t temp;
  amide_volume_t * volume;
  
  /* get the location of the event, and convert it to our coordinate system */
  item_x = event->button.x;
  item_y = event->button.y;
  gnome_canvas_item_w2i(GNOME_CANVAS_ITEM(widget)->parent, &item_x, &item_y);

  volume = ui_study->threshold->volume;

  /* switch on the event which called this */
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      cursor = gdk_cursor_new(GDK_SB_V_DOUBLE_ARROW);
      gnome_canvas_item_grab(GNOME_CANVAS_ITEM(widget),
			     GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			     cursor,
			     event->button.time);
      gdk_cursor_destroy(cursor);
      break;

    case GDK_MOTION_NOTIFY:
      if (event->motion.state & GDK_BUTTON1_MASK) {
	temp = ((UI_THRESHOLD_COLOR_STRIP_HEIGHT - item_y) /
		UI_THRESHOLD_COLOR_STRIP_HEIGHT) *
	  (volume->max - volume->min)+volume->min;
	if (temp <= volume->threshold_min) 
	  temp = volume->threshold_min;
	if (temp < 0.0)
	  temp = 0.0;
	volume->threshold_max = temp;
	ui_threshold_update_canvas(ui_study);
	ui_threshold_update_entries(ui_study->threshold);   /* reset the entry widgets */
      }
      break;

    case GDK_BUTTON_RELEASE:
      gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
      ui_study_update_canvas(ui_study, NUM_VIEWS, REFRESH_IMAGE);
      ui_threshold_update_entries(ui_study->threshold);   /* reset the entry widgets */
      /* and if we have a series up, update that */
      if (ui_study->series != NULL)
	ui_series_update_canvas_image(ui_study);
      break;

    default:
      break;
    }
      
  return FALSE;
}


void ui_threshold_callbacks_max_percentage(GtkWidget* widget, gpointer data) {

  ui_study_t * ui_study = data;
  gchar * string;
  gint error;
  volume_data_t temp;
  amide_volume_t * volume;

  volume = ui_study->threshold->volume;

  /* get the current entry */
  string = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert it to a floating point */
  error = sscanf(string, "%lf", &temp);
  g_free(string);
    
  /* make sure it's a valid floating point */
  if (!(error == EOF)) {
    temp = (volume->max-volume->min) * temp/100.0;
    if ((temp > 0.0) & (temp > volume->threshold_min)) {
      volume->threshold_max = temp;
      ui_threshold_update_canvas(ui_study);
      ui_study_update_canvas(ui_study, NUM_VIEWS, REFRESH_IMAGE);
      /* and if we have a series up, update that */
      if (ui_study->series != NULL)
	ui_series_update_canvas_image(ui_study);
    }
  }      

  /* reset the entry widgets */
  ui_threshold_update_entries(ui_study->threshold);

  return;
}



void ui_threshold_callbacks_max_absolute(GtkWidget* widget, gpointer data) {

  ui_study_t * ui_study = data;
  gchar * string;
  gint error;
  volume_data_t temp;
  amide_volume_t * volume;

  volume = ui_study->threshold->volume;

  /* get the current entry */
  string = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert it to a floating point */
  error = sscanf(string, "%lf", &temp);
  g_free(string);
    
  /* make sure it's a valid floating point */
  if (!(error == EOF)) 
    if ((temp > 0.0) & (temp > volume->threshold_min)) {
      volume->threshold_max = temp;
      ui_threshold_update_canvas(ui_study);
      ui_study_update_canvas(ui_study, NUM_VIEWS, REFRESH_IMAGE);
      /* and if we have a series up, update that */
      if (ui_study->series != NULL)
	ui_series_update_canvas_image(ui_study);
    }
      
  /* reset the entry widgets */
  ui_threshold_update_entries(ui_study->threshold);

  return;
}


/* function called when the min triangle is moved 
 * mostly taken from Pennington's fine book */
gint ui_threshold_callbacks_min_arrow(GtkWidget* widget, 
				      GdkEvent * event,
				      gpointer data) {

  ui_study_t * ui_study = data;
  gdouble item_x, item_y;
  GdkCursor * cursor;
  volume_data_t temp;
  amide_volume_t * volume;

  volume = ui_study->threshold->volume;

  /* get the location of the event, and convert it to our coordinate system */
  item_x = event->button.x;
  item_y = event->button.y;
  gnome_canvas_item_w2i(GNOME_CANVAS_ITEM(widget)->parent, &item_x, &item_y);

  /* switch on the event which called this */
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      cursor = gdk_cursor_new(GDK_SB_V_DOUBLE_ARROW);
      gnome_canvas_item_grab(GNOME_CANVAS_ITEM(widget),
			     GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			     cursor,
			     event->button.time);
      gdk_cursor_destroy(cursor);
      break;

    case GDK_MOTION_NOTIFY:
      if (event->motion.state & GDK_BUTTON1_MASK) {
	temp = ((UI_THRESHOLD_COLOR_STRIP_HEIGHT - item_y) /
		UI_THRESHOLD_COLOR_STRIP_HEIGHT) *
	  (volume->max - volume->min)+volume->min;
	if (temp < volume->min) 
	  temp = volume->min;
	if (temp >= volume->threshold_max)
	  temp = volume->threshold_max;
	if (temp >= volume->max)
	  temp = volume->max;
	volume->threshold_min = temp;
	ui_threshold_update_canvas(ui_study);
      }
      break;

    case GDK_BUTTON_RELEASE:
      gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
      ui_study_update_canvas(ui_study, NUM_VIEWS, REFRESH_IMAGE);
      ui_threshold_update_entries(ui_study->threshold);   /* reset the entry widgets */
      /* and if we have a series up, update that */
      if (ui_study->series != NULL)
	ui_series_update_canvas_image(ui_study);
      break;

    default:
      break;
    }
      
  return FALSE;
}


void ui_threshold_callbacks_min_percentage(GtkWidget* widget, gpointer data) {

  ui_study_t * ui_study = data;
  gchar * string;
  gint error;
  volume_data_t temp;
  amide_volume_t * volume;

  volume = ui_study->threshold->volume;

  /* get the current entry */
  string = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert it to a floating point */
  error = sscanf(string, "%lf", &temp);
  g_free(string);
    
  /* make sure it's a valid floating point */
  if (!(error == EOF)) { 
    temp = ((volume->max-volume->min) * temp/100.0)+volume->min;
    if ((temp < volume->max) & (temp >= volume->min) & (temp < volume->threshold_max)) {
      volume->threshold_min = temp;
      ui_threshold_update_canvas(ui_study);
      ui_study_update_canvas(ui_study, NUM_VIEWS, REFRESH_IMAGE);
      /* and if we have a series up, update that */
      if (ui_study->series != NULL)
	ui_series_update_canvas_image(ui_study);
    }
  }

  /* reset the entry widgets */
  ui_threshold_update_entries(ui_study->threshold);

  return;
}



void ui_threshold_callbacks_min_absolute(GtkWidget* widget, gpointer data) {

  ui_study_t * ui_study = data;
  gchar * string;
  gint error;
  volume_data_t temp;
  amide_volume_t * volume;

  volume = ui_study->threshold->volume;

  /* get the current entry */
  string = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert it to a floating point */
  error = sscanf(string, "%lf", &temp);
  g_free(string);
    
  /* make sure it's a valid floating point */
  if (!(error == EOF))
    if ((temp < volume->max) & (temp >= 0.0) & (temp < volume->threshold_max)) {
      volume->threshold_min = temp;
      ui_threshold_update_canvas(ui_study);
      ui_study_update_canvas(ui_study, NUM_VIEWS, REFRESH_IMAGE);
      /* and if we have a series up, update that */
      if (ui_study->series != NULL)
	ui_series_update_canvas_image(ui_study);
    }

  /* reset the entry widgets */
  ui_threshold_update_entries(ui_study->threshold);

  return;
}





/* function to run for a delete_event */
void ui_threshold_callbacks_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_study_t * ui_study = data;

  /* destroy the widget */
  gtk_widget_destroy(widget);

  /* free the associated data structure */
  ui_threshold_free(&(ui_study->threshold));

  return;
}
