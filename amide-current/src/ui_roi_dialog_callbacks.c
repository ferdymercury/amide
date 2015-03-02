/* ui_roi_dialog_callbacks.c
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
#include <math.h>
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
#include "ui_roi_dialog.h"
#include "ui_roi_dialog_callbacks.h"
#include "ui_study_rois2.h"

/* function called when the name of the roi has been changed */
void ui_roi_dialog_callbacks_change_name(GtkWidget * widget, gpointer data) {

  amide_roi_t * roi_new_info = data;
  gchar * new_name;
  GtkWidget * roi_dialog;

  /* get the contents of the name entry box and save it */
  new_name = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  roi_set_name(roi_new_info, new_name);
  g_free(new_name);

  /* tell the roi_dialog that we've changed */
  roi_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "roi_dialog");
  gnome_property_box_changed(GNOME_PROPERTY_BOX(roi_dialog));

  return;
}

/* function called when a numerical entry of the roi has been changed, 
   used for the coordinate frame offset, axis, and the roi's corner*/
void ui_roi_dialog_callbacks_change_entry(GtkWidget * widget, gpointer data) {

  amide_roi_t * roi_new_info = data;
  gchar * str;
  gint error;
  gdouble temp_val;
  which_entry_widget_t which_widget;
  realpoint_t temp_center;
  realpoint_t temp_dim;
  realspace_t temp_coord_frame;
  realpoint_t temp;
  axis_t i_axis;
  GtkWidget * roi_dialog;

  /* initialize the center and dimension variables based on the old roi info */
  temp_center = roi_calculate_center(roi_new_info); /* in real coords */
  temp_dim = roi_new_info->corner; /* in roi's coords */
  for (i_axis=0;i_axis<NUM_AXIS;i_axis++)
    temp_coord_frame.axis[i_axis] = roi_new_info->coord_frame.axis[i_axis];
  temp_coord_frame.offset = realpoint_init;

  /* figure out which widget this is */
  which_widget = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "type")); 
  
  /* get the contents of the name entry box */
  str = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert to a floating point */
  error = sscanf(str, "%lf", &temp_val);
  g_free(str);

  if (error == EOF)  /* make sure it's a valid number */
    return;
  
  /* and save the value until it's applied to the actual roi */
  switch(which_widget) {
  case CENTER_X:
    temp_center.x = temp_val;
    break;
  case CENTER_Y:
    temp_center.y = temp_val;
    break;
  case CENTER_Z:
    temp_center.z = temp_val;
    break;
  case DIM_X:
    temp_dim.x = fabs(temp_val);
    break;
  case DIM_Y:
    temp_dim.y = fabs(temp_val);
    break;
  case DIM_Z:
    temp_dim.z = fabs(temp_val);
    break;
  case AXIS_X_X:
    temp_coord_frame.axis[XAXIS].x = temp_val;
    break;
  case AXIS_X_Y:
    temp_coord_frame.axis[XAXIS].y = temp_val;
    break;
  case AXIS_X_Z:
    temp_coord_frame.axis[XAXIS].z = temp_val;
    break;
  case AXIS_Y_X:
    temp_coord_frame.axis[YAXIS].x = temp_val;
    break;
  case AXIS_Y_Y:
    temp_coord_frame.axis[YAXIS].y = temp_val;
    break;
  case AXIS_Y_Z:
    temp_coord_frame.axis[YAXIS].z = temp_val;
    break;
  case AXIS_Z_X:
    temp_coord_frame.axis[ZAXIS].x = temp_val;
    break;
  case AXIS_Z_Y:
    temp_coord_frame.axis[ZAXIS].y = temp_val;
    break;
  case AXIS_Z_Z:
    temp_coord_frame.axis[ZAXIS].z = temp_val;
    break;
  default:
    break; /* do nothing */
  }
  

  /* recalculate the roi's offset based on the new dimensions/center/and axis */
  temp = realspace_alt_dim_to_base(temp_dim, temp_coord_frame);
  REALSPACE_MADD(-0.5,temp,1,temp_center,temp_coord_frame.offset);

  /* reset the far corner based on the new coord frame */
  roi_new_info->corner = realspace_alt_dim_to_alt(temp_dim, 
						  roi_new_info->coord_frame,
						  temp_coord_frame);

  /* and save any changes to the coord frame */
  roi_new_info->coord_frame = temp_coord_frame;

  /* now tell the roi_dialog that we've changed */
  roi_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "roi_dialog");
  gnome_property_box_changed(GNOME_PROPERTY_BOX(roi_dialog));

  return;
}



/* function to change an roi's type */
void ui_roi_dialog_callbacks_change_type(GtkWidget * widget, gpointer data) {

  amide_roi_t * roi_new_info = data;
  roi_type_t i_roi_type;
  GtkWidget * roi_dialog;

  /* figure out which menu item called me */
  for (i_roi_type=0;i_roi_type<NUM_ROI_TYPES;i_roi_type++)       
    if (GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"roi_type")) == i_roi_type)
      roi_new_info->type = i_roi_type;  /* save the new roi_type until it's applied */

  /* now tell the roi_dialog that we've changed */
  roi_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "roi_dialog");
  gnome_property_box_changed(GNOME_PROPERTY_BOX(roi_dialog));

  return;
}

/* function to change the grain size used to calculate an roi's statistics */
void ui_roi_dialog_callbacks_change_grain(GtkWidget * widget, gpointer data) {

  amide_roi_t * roi_new_info = data;
  roi_grain_t i_grain;
  GtkWidget * roi_dialog;

  /* figure out which menu item called me */
  for (i_grain=0;i_grain<NUM_GRAIN_TYPES;i_grain++)       
    if (GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"grain_size")) == i_grain)
      roi_new_info->grain = i_grain;  /* save the new grain size until it's applied */

  /* now tell the roi_dialog that we've changed */
  roi_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "roi_dialog");
  gnome_property_box_changed(GNOME_PROPERTY_BOX(roi_dialog));

  return;
}

/* function called when we hit the apply button */
void ui_roi_dialog_callbacks_apply(GtkWidget* widget, gint page_number, gpointer data) {
  
  ui_study_roi_list_t * roi_list_item = data;
  ui_study_t * ui_study;
  GdkPixmap * pixmap;
  guint8 spacing;
  amide_roi_t * roi_new_info;
  view_t i_views;
  
  /* we'll apply all page changes at once */
  if (page_number != -1)
    return;

  /* get the new info for the roi */
  roi_new_info = gtk_object_get_data(GTK_OBJECT(roi_list_item->dialog),"roi_new_info");

  /* sanity check */
  if (roi_new_info == NULL) {
    g_printerr("%s: roi_new_info inappropriately null....\n", PACKAGE);
    return;
  }

  /* copy the new info on over */
  roi_copy(&(roi_list_item->roi), roi_new_info);

  /* apply any changes to the name of the widget */
  /* get the current pixmap and spacing in the line of the tree corresponding to this roi */
  gtk_ctree_node_get_pixtext(roi_list_item->tree, roi_list_item->tree_node, 0,
			     NULL, &spacing, &pixmap, NULL);
  /* reset the text in that tree line */
  gtk_ctree_node_set_pixtext(roi_list_item->tree, roi_list_item->tree_node, 0, 
			     roi_list_item->roi->name, spacing, pixmap, NULL);


  /* get a pointer to ui_study so we can redraw the roi's */
  ui_study = gtk_object_get_data(GTK_OBJECT(roi_list_item->dialog), "ui_study"); 

  /* redraw the roi */
  for (i_views=0;i_views<NUM_VIEWS;i_views++) {
    roi_list_item->canvas_roi[i_views] =
      ui_study_rois_update_canvas_roi(ui_study,i_views,
				      roi_list_item->canvas_roi[i_views],
				      roi_list_item->roi);
  }

  return;
}

/* callback for the help button */
void ui_roi_dialog_callbacks_help(GnomePropertyBox *roi_dialog, gint page_number, gpointer data) {

  g_print("sorry, no help yet......\n");

  return;
}

/* function called to destroy the roi dialog */
void ui_roi_dialog_callbacks_close_event(GtkWidget* widget, gpointer data) {

  ui_study_roi_list_t * roi_list_item = data;
  amide_roi_t * roi_new_info;

  /* trash collection */
  roi_new_info = gtk_object_get_data(GTK_OBJECT(widget), "roi_new_info");
  roi_free(&roi_new_info);

  /* destroy the widget */
  gtk_widget_destroy(GTK_WIDGET(roi_list_item->dialog));

  /* make sure the pointer in the roi_list_item is nulled */
  roi_list_item->dialog = NULL;

  return;
}


