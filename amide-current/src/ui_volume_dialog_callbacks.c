/* ui_volume_dialog_callbacks.c
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
#include "roi.h"
#include "study.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_study_rois.h"
#include "ui_study_volumes.h"
#include "ui_study.h"
#include "ui_volume_dialog.h"
#include "ui_volume_dialog_callbacks.h"
#include "../pixmaps/PET.xpm"
#include "../pixmaps/SPECT.xpm"
#include "../pixmaps/CT.xpm"
#include "../pixmaps/MRI.xpm"
#include "../pixmaps/OTHER.xpm"


/* function called when the name of the volume has been changed */
void ui_volume_dialog_callbacks_change_name(GtkWidget * widget, gpointer data) {

  amide_volume_t * volume_new_info = data;
  gchar * new_name;
  GtkWidget * volume_dialog;

  /* get the contents of the name entry box and save it */
  new_name = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  volume_set_name(volume_new_info, new_name);
  g_free(new_name);

  /* tell the volume_dialog that we've changed */
  volume_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "volume_dialog");
  gnome_property_box_changed(GNOME_PROPERTY_BOX(volume_dialog));

  return;
}

/* when a numerical entry of the volume has been changed, 
   used for the coordinate frame offset, */
void ui_volume_dialog_callbacks_change_entry(GtkWidget * widget, gpointer data) {

  amide_volume_t * volume_new_info = data;
  gchar * str;
  gint error;
  gdouble temp_val;
  which_entry_widget_t which_widget;
  realpoint_t old_center, new_center, shift;
  GtkWidget * volume_dialog;

  /* initialize the center variables based on the old volume info */
  old_center = new_center = volume_calculate_center(volume_new_info); /* in real coords */

  /* figure out which widget this is */
  which_widget = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "type")); 
  
  /* get the contents of the name entry box */
  str = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert to a floating point */
  error = sscanf(str, "%lf", &temp_val);
  g_free(str);

  if ((error == EOF))  /* make sure it's a valid number */
    return;
  
  /* and save the value until it's applied to the actual volume */
  switch(which_widget) {
  case CENTER_X:
    new_center.x = temp_val;
    break;
  case CENTER_Y:
    new_center.y = temp_val;
    break;
  case CENTER_Z:
    new_center.z = temp_val;
    break;
  default:
    break; /* do nothing */
  }
  
  /* recalculate the volume's offset based on the new center */
  REALSPACE_SUB(new_center, old_center, shift);

  /* and save any changes to the coord frame */
  REALSPACE_ADD(volume_new_info->coord_frame.offset, shift, volume_new_info->coord_frame.offset);

  /* now tell the volume_dialog that we've changed */
  volume_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "volume_dialog");
  gnome_property_box_changed(GNOME_PROPERTY_BOX(volume_dialog));

  return;
}



/* function called when the modality type of the volume gets changed */
void ui_volume_dialog_callbacks_change_modality(GtkWidget * widget, gpointer data) {

  amide_volume_t * volume_new_info = data;
  modality_t i_modality;
  GtkWidget * volume_dialog;

  /* figure out which menu item called me */
  for (i_modality=0;i_modality<NUM_MODALITIES;i_modality++)       
    if (GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"modality")) == i_modality)
      volume_new_info->modality = i_modality;  /* save the new modality until it's applied */

  /* now tell the volume_dialog that we've changed */
  volume_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "volume_dialog");
  gnome_property_box_changed(GNOME_PROPERTY_BOX(volume_dialog));

  return;
}



/* function called when rotating the volume around an axis */
void ui_volume_dialog_callbacks_axis_change(GtkAdjustment * adjustment, gpointer data) {

  ui_study_t * ui_study;
  amide_volume_t * volume_new_info = data;
  view_t i_view;
  floatpoint_t rotation;
  GtkWidget * volume_dialog;
  realpoint_t center, temp;

  /* we need the current view_axis so that we know what we're rotating around */
  ui_study = gtk_object_get_data(GTK_OBJECT(adjustment), "ui_study"); 

  /* saving the center, as we're rotating the volume around it's own center */
  center = volume_calculate_center(volume_new_info); 

  /* figure out which scale widget called me */
  for (i_view=0;i_view<NUM_VIEWS;i_view++) 
    if (gtk_object_get_data(GTK_OBJECT(adjustment),"view") == view_names[i_view]) {
      rotation = (adjustment->value/180)*M_PI; /* get rotation in radians */
      switch(i_view) {
      case TRANSVERSE:
	volume_new_info->coord_frame.axis[XAXIS] = 
	  realspace_rotate_on_axis(&volume_new_info->coord_frame.axis[XAXIS],
				   &ui_study->current_coord_frame.axis[ZAXIS],
				   rotation);
	volume_new_info->coord_frame.axis[YAXIS] = 
	  realspace_rotate_on_axis(&volume_new_info->coord_frame.axis[YAXIS],
				   &ui_study->current_coord_frame.axis[ZAXIS],
				   rotation);
	volume_new_info->coord_frame.axis[ZAXIS] = 
	  realspace_rotate_on_axis(&volume_new_info->coord_frame.axis[ZAXIS],
				   &ui_study->current_coord_frame.axis[ZAXIS],
				   rotation);
	realspace_make_orthonormal(volume_new_info->coord_frame.axis); /* orthonormalize*/
	break;
      case CORONAL:
	volume_new_info->coord_frame.axis[XAXIS] = 
	  realspace_rotate_on_axis(&volume_new_info->coord_frame.axis[XAXIS],
				   &ui_study->current_coord_frame.axis[YAXIS],
				   rotation);
	volume_new_info->coord_frame.axis[YAXIS] = 
	  realspace_rotate_on_axis(&volume_new_info->coord_frame.axis[YAXIS],
				   &ui_study->current_coord_frame.axis[YAXIS],
				   rotation);
	volume_new_info->coord_frame.axis[ZAXIS] = 
	  realspace_rotate_on_axis(&volume_new_info->coord_frame.axis[ZAXIS],
				   &ui_study->current_coord_frame.axis[YAXIS],
				   rotation);
	realspace_make_orthonormal(volume_new_info->coord_frame.axis); /* orthonormalize*/
	break;
      case SAGITTAL:
	volume_new_info->coord_frame.axis[XAXIS] = 
	  realspace_rotate_on_axis(&volume_new_info->coord_frame.axis[XAXIS],
				   &ui_study->current_coord_frame.axis[XAXIS],
				   rotation);
	volume_new_info->coord_frame.axis[YAXIS] = 
	  realspace_rotate_on_axis(&volume_new_info->coord_frame.axis[YAXIS],
				   &ui_study->current_coord_frame.axis[XAXIS],
				   rotation);
	volume_new_info->coord_frame.axis[ZAXIS] = 
	  realspace_rotate_on_axis(&volume_new_info->coord_frame.axis[ZAXIS],
				   &ui_study->current_coord_frame.axis[XAXIS],
				   rotation);
	realspace_make_orthonormal(volume_new_info->coord_frame.axis); /* orthonormalize*/
	break;
      default:
	break;
      }
    }

  
  /* recalculate the offset of this volume based on the center we stored */
  REALSPACE_CMULT(-0.5,volume_new_info->corner,temp);
  volume_new_info->coord_frame.offset = center;
  volume_new_info->coord_frame.offset = realspace_alt_coord_to_base(temp, volume_new_info->coord_frame);

  /* return adjustment back to normal */
  adjustment->value = 0.0;
  gtk_adjustment_changed(adjustment);

  /* now tell the volume_dialog that we've changed */
  volume_dialog =  gtk_object_get_data(GTK_OBJECT(adjustment), "volume_dialog");
  gnome_property_box_changed(GNOME_PROPERTY_BOX(volume_dialog));

  return;
}


/* function called when we hit the apply button */
void ui_volume_dialog_callbacks_apply(GtkWidget* widget, gint page_number, gpointer data) {
  
  ui_study_volume_list_t * volume_list_item = data;
  ui_study_t * ui_study;
  GdkPixmap * pixmap;
  guint8 spacing;
  amide_volume_t * volume_new_info;
  GdkWindow * parent;
  
  /* we'll apply all page changes at once */
  if (page_number != -1)
    return;

  /* get a pointer to ui_study so we can redraw the volume's */
  ui_study = gtk_object_get_data(GTK_OBJECT(volume_list_item->dialog), "ui_study"); 

  /* get the new info for the volume */
  volume_new_info = gtk_object_get_data(GTK_OBJECT(volume_list_item->dialog),"volume_new_info");

  /* sanity check */
  if (volume_new_info == NULL) {
    g_printerr("%s, volume_new_info inappropriately null....\n", PACKAGE);
    return;
  }

  /* copy the new info on over */
  volume_list_item->volume->modality = volume_new_info->modality;
  volume_list_item->volume->conversion = volume_new_info->conversion;
  volume_list_item->volume->color_table = volume_new_info->color_table;
  volume_list_item->volume->coord_frame = volume_new_info->coord_frame;
  volume_set_name(volume_list_item->volume, volume_new_info->name);


  /* apply any changes to the name of the widget */
  /* get the current pixmap and spacing in the line of the tree corresponding to this volume */
  gtk_ctree_node_get_pixtext(volume_list_item->tree, volume_list_item->tree_node, 0,
			     NULL, &spacing, &pixmap, NULL);
  g_free(pixmap);

  parent = gtk_widget_get_parent_window(ui_study->tree);
  
  /* which icon to use */
  switch (volume_list_item->volume->modality) {
  case SPECT:
    pixmap = gdk_pixmap_create_from_xpm_d(parent,NULL,NULL,SPECT_xpm);
    break;
  case MRI:
    pixmap = gdk_pixmap_create_from_xpm_d(parent,NULL,NULL,MRI_xpm);
    break;
  case CT:
    pixmap = gdk_pixmap_create_from_xpm_d(parent,NULL,NULL,CT_xpm);
    break;
  case OTHER:
    pixmap = gdk_pixmap_create_from_xpm_d(parent,NULL,NULL,OTHER_xpm);
    break;
  case PET:
  default:
    pixmap = gdk_pixmap_create_from_xpm_d(parent,NULL,NULL,PET_xpm);
    break;
  }
  /* reset the text in that tree line */
  gtk_ctree_node_set_pixtext(volume_list_item->tree, volume_list_item->tree_node, 0, 
			     volume_list_item->volume->name, spacing, pixmap, NULL);



  /* redraw the volume */
  ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_IMAGE);

  return;
}

/* callback for the help button */
void ui_volume_dialog_callbacks_help(GnomePropertyBox *volume_dialog, gint page_number, gpointer data) {

  g_print("sorry, no help yet......\n");

  return;
}

/* function called to destroy the volume dialog */
void ui_volume_dialog_callbacks_close_event(GtkWidget* widget, gpointer data) {

  ui_study_volume_list_t * volume_list_item = data;
  amide_volume_t * volume_new_info;

  /* trash collection */
  volume_new_info = gtk_object_get_data(GTK_OBJECT(widget), "volume_new_info");
  volume_free(&volume_new_info);

  /* destroy the widget */
  gtk_widget_destroy(GTK_WIDGET(volume_list_item->dialog));

  /* make sure the pointer in the volume_list_item is nulled */
  volume_list_item->dialog = NULL;

  return;
}


