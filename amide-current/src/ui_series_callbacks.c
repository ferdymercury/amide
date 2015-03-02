/* ui_series_callbacks.c
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
#include "study.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_roi.h"
#include "ui_volume.h"
#include "ui_study.h"
#include "ui_series2.h"
#include "ui_threshold2.h"
#include "ui_series_callbacks.h"

/* function called by the adjustment in charge for scrolling */
void ui_series_callbacks_scroll_change(GtkAdjustment* adjustment, gpointer data) {

  ui_study_t * ui_study = data;

  if (ui_study->series->type == PLANES)
    ui_study->series->view_point.z = adjustment->value;
  else
    ui_study->series->view_time = adjustment->value;

  ui_series_update_canvas_image(ui_study);

  return;
}

/* function to run for a delete_event */
void ui_series_callbacks_delete_event(GtkWidget* widget, GdkEvent * event, 
				      gpointer data) {

  ui_study_t * ui_study = data;

  /* free the associated data structure */
  ui_study->series = ui_series_free(ui_study->series);

  /* destroy the widget */
  gtk_widget_destroy(widget);

  return;
}






