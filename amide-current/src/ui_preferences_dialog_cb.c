/* ui_preferences_dialog_cb.c
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
#include "study.h"
#include "ui_study.h"
#include "ui_preferences_dialog.h"
#include "ui_preferences_dialog_cb.h"


/* function called when the roi width has been changed */
void ui_preferences_dialog_cb_roi_width(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  gint new_roi_width;
  GnomeCanvasItem * roi_item;

  new_roi_width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));

  /* this is where we'll store the new roi width */
  gtk_object_set_data(GTK_OBJECT(ui_study->preferences_dialog), 
		      "new_roi_width", GINT_TO_POINTER(new_roi_width));

  gnome_property_box_changed(GNOME_PROPERTY_BOX(ui_study->preferences_dialog));

  /* update the roi indicator */
  roi_item = gtk_object_get_data(GTK_OBJECT(ui_study->preferences_dialog), "roi_item");
  gnome_canvas_item_set(roi_item, "width_pixels", new_roi_width, NULL);

  return;
}


/* function to change the line style */
void ui_preferences_dialog_cb_line_style(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study=data;
  GdkLineStyle new_line_style;
  GnomeCanvasItem * roi_item;

  /* figure out which menu item called me */
  new_line_style = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"line_style"));

  /* this is where we'll store the new line style */
  gtk_object_set_data(GTK_OBJECT(ui_study->preferences_dialog), 
		      "new_line_style", GINT_TO_POINTER(new_line_style));

  gnome_property_box_changed(GNOME_PROPERTY_BOX(ui_study->preferences_dialog));

  /* update the roi indicator */
  roi_item = gtk_object_get_data(GTK_OBJECT(ui_study->preferences_dialog), "roi_item");
  gnome_canvas_item_set(roi_item, "line_style", new_line_style, NULL);
  return;
}


/* function called when we hit the apply button */
void ui_preferences_dialog_cb_apply(GtkWidget* widget, gint page_number, gpointer data) {
  
  ui_study_t * ui_study = data;
  gint new_roi_width;
  GdkLineStyle new_line_style;
  
  /* we'll apply all page changes at once */
  if (page_number != -1)  return;


  new_roi_width = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(ui_study->preferences_dialog), 
						      "new_roi_width"));
  new_line_style = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(ui_study->preferences_dialog), 
						       "new_line_style"));

  /* sanity checks */
  if (new_roi_width < UI_STUDY_MIN_ROI_WIDTH) new_roi_width = UI_STUDY_MIN_ROI_WIDTH;
  if (new_roi_width > UI_STUDY_MAX_ROI_WIDTH) new_roi_width = UI_STUDY_MAX_ROI_WIDTH;

  /* copy the new info on over */
  ui_study->roi_width = new_roi_width;
  ui_study->line_style = new_line_style;

  /* and save user preferences */
  gnome_config_push_prefix("/"PACKAGE"/");
  gnome_config_set_int("ROI/Width",ui_study->roi_width);
  gnome_config_set_int("ROI/LineStyle",ui_study->line_style);
  gnome_config_pop_prefix();
  gnome_config_sync();

  ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_ROIS);
  return;
}

/* callback for the help button */
void ui_preferences_dialog_cb_help(GnomePropertyBox *preferences_dialog, gint page_number, gpointer data) {

  GnomeHelpMenuEntry help_ref={PACKAGE,"basics.html#PREFERENCES-DIALOG-HELP"};
  GnomeHelpMenuEntry help_ref_0 = {PACKAGE,"basics.html#PREFERENCES-DIALOG-HELP-ROI"};

  switch (page_number) {
  case 0:
    gnome_help_display (0, &help_ref_0);
    break;
  default:
    gnome_help_display (0, &help_ref);
    break;
  }

  return;
}

/* function called to destroy the preferences dialog */
void ui_preferences_dialog_cb_close(GtkWidget* widget, gpointer data) {

  ui_study_t * ui_study = data;

  /* destroy the widget */
  gtk_widget_destroy(ui_study->preferences_dialog);

  ui_study->preferences_dialog = NULL;

  return;
}


