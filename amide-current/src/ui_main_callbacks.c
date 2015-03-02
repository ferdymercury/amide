/* ui_main_callbacks.c
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
#include "ui_main_callbacks.h"

/* function to create a new study widget */
void ui_main_callbacks_new_study(GnomeApp* app, gpointer data) {

  ui_study_create(NULL);

  return;
}
  

/* function which brings up an about box */
void ui_main_callbacks_about(GnomeApp* app, gpointer data) {

  GtkWidget *about;
  const gchar *authors[] = {
    "Andy Loening <loening@ucla.edu>",
    NULL
  };

  about = gnome_about_new(PACKAGE, VERSION, 
			  "(c) 2000 Andy Loening",
			  authors,
			  _("Amide's a Medical Image Data Examiner"),
			  "amide.png");

  gtk_window_set_modal(GTK_WINDOW(about), FALSE);

  gtk_widget_show(about);

  return;
}

/* function ran when closing the main window */
void ui_main_callbacks_delete_event(GnomeApp* app, gpointer data) {
  
  gtk_widget_destroy(GTK_WIDGET(app));

  gtk_main_quit();

}

