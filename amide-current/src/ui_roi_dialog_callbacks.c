/* ui_roi_dialog_callbacks.c
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
#include "ui_roi_dialog.h"
#include "ui_roi_dialog_callbacks.h"
#include "ui_study_rois2.h"

/* function called when the name of the roi has been changed */
void ui_roi_dialog_callbacks_change_name(GtkWidget * widget, gpointer data) {

  ui_study_roi_list_t * roi_list_item = data;
  gchar * new_name;
  GdkPixmap * pixmap;
  guint8 spacing;

  /* get the contents of the name entry box */
  new_name = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  g_free(roi_list_item->roi->name); /* free up the old name space */
  roi_list_item->roi->name = new_name; /* and assign the new name */

  /* get the current pixmap and spacing */
  gtk_ctree_node_get_pixtext(roi_list_item->tree,
			     roi_list_item->tree_node,
			     0,
			     NULL,
			     &spacing,
			     &pixmap,
			     NULL);


  /* reset the text in the tree line */
  gtk_ctree_node_set_pixtext(roi_list_item->tree,
			     roi_list_item->tree_node,
			     0, 
			     roi_list_item->roi->name,
			     spacing, pixmap, NULL);

  return;
}

/* function called when a numerical entry of the roi has been changed, 
   used for the coordinate frame offset, axis, and the roi's corner*/
void ui_roi_dialog_callbacks_change_entry(GtkWidget * widget, gpointer data) {

  ui_study_roi_list_t * roi_list_item = data;
  gchar * str;
  gint error;
  gdouble temp_val;
  which_entry_widget_t which_widget;
  ui_study_t * ui_study;
  view_t i_views;

  /* figure out which widget this is */
  which_widget = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "type")); 
  
  ui_study = gtk_object_get_data(GTK_OBJECT(widget), "ui_study"); 

  /* get the contents of the name entry box */
  str = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert to a floating point */
  error = sscanf(str, "%lf", &temp_val);
  g_free(str);

  if (!(error == EOF)) { /* make sure it's a valid number */
    switch(which_widget) {
    case CORNER_X:
      roi_list_item->roi->corner.x = temp_val;
      break;
    case CORNER_Y:
      roi_list_item->roi->corner.y = temp_val;
      break;
    case CORNER_Z:
      roi_list_item->roi->corner.z = temp_val;
      break;
    default:
      break; /* do nothing */
    }
  }

  /* redraw the roi */
  for (i_views=0;i_views<NUM_VIEWS;i_views++) {
    roi_list_item->canvas_roi[i_views] =
      ui_study_rois_update_canvas_roi(ui_study,i_views,
				      roi_list_item->canvas_roi[i_views],
				      roi_list_item->roi);
  }

  return;
}



/* function to change an roi's type */
void ui_roi_dialog_callbacks_change_type(GtkWidget * widget, gpointer data) {

  ui_study_roi_list_t * roi_list_item = data;
  roi_type_t i_roi_type;
  ui_study_t * ui_study;
  view_t i_views;

  /* figure out which menu item called me */
  for (i_roi_type=0;i_roi_type<NUM_ROI_TYPES;i_roi_type++)       
    if (gtk_object_get_data(GTK_OBJECT(widget),"roi_type") == roi_type_names[i_roi_type])
      roi_list_item->roi->type = i_roi_type;  /* reset the roi */

  ui_study = gtk_object_get_data(GTK_OBJECT(widget),"ui_study");

  /* redraw the roi */
  for (i_views=0;i_views<NUM_VIEWS;i_views++) {
    roi_list_item->canvas_roi[i_views] =
      ui_study_rois_update_canvas_roi(ui_study,i_views,
				      roi_list_item->canvas_roi[i_views],
				      roi_list_item->roi);
  }

  return;
}

/* function to change the grain size used to calculate an roi's statistics */
void ui_roi_dialog_callbacks_change_grain(GtkWidget * widget, gpointer data) {

  ui_study_roi_list_t * roi_list_item = data;
  roi_grain_t i_grain;


  /* figure out which menu item called me */
  for (i_grain=0;i_grain<NUM_GRAIN_TYPES;i_grain++)       
    if (gtk_object_get_data(GTK_OBJECT(widget),"grain_size") == roi_grain_names[i_grain])
      roi_list_item->roi->grain = i_grain;  /* set the new grain size */

  return;
}


/* function called to destroy the roi dialog */
void ui_roi_dialog_callbacks_delete_event(GtkWidget* widget, GdkEvent * event,
					  gpointer data) {

  ui_study_roi_list_t * roi_list_item = data;

  /* destroy the widget */
  gtk_widget_destroy(widget);

  /* make sure the pointer in the roi_list_item is nulled */
  roi_list_item->dialog = NULL;

  return;
}








