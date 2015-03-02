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
#include "ui_volume_dialog.h"
#include "ui_volume_dialog_callbacks.h"
#include "ui_time_dialog.h"
#include "../pixmaps/PET.xpm"
#include "../pixmaps/SPECT.xpm"
#include "../pixmaps/CT.xpm"
#include "../pixmaps/MRI.xpm"
#include "../pixmaps/OTHER.xpm"


/* function called when the name of the volume has been changed */
void ui_volume_dialog_callbacks_change_name(GtkWidget * widget, gpointer data) {

  volume_t * volume_new_info = data;
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

  volume_t * volume_new_info = data;
  gchar * str;
  gint error;
  gdouble temp_val;
  floatpoint_t scale;
  which_entry_widget_t which_widget;
  realpoint_t old_center, new_center, shift;
  GtkWidget * volume_dialog;
  GtkWidget * entry;
  guint i;
  gboolean aspect_ratio=TRUE;
  gboolean update_size_x = FALSE;
  gboolean update_size_y = FALSE;
  gboolean update_size_z = FALSE;
  
  /* initialize the center variables based on the old volume info */
  old_center = new_center = volume_calculate_center(volume_new_info); /* in real coords */

  /* figure out which widget this is */
  which_widget = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "type")); 

  /* get the pointer ot the volume dialog and get any necessary info */
  volume_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "volume_dialog");
  aspect_ratio =  GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(volume_dialog), "aspect_ratio"));


  /* get the contents of the name entry box */
  str = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert to a floating point */
  error = sscanf(str, "%lf", &temp_val);
  g_free(str);

  if ((error == EOF))  /* make sure it's a valid number */
    return;
  
  /* and save the value until it's applied to the actual volume */
  switch(which_widget) {
  case VOXEL_SIZE_X:
    if (temp_val > SMALL) { /* can't be having negative/very small numbers */
      if (aspect_ratio) {
	scale = temp_val/volume_new_info->voxel_size.x;
	volume_new_info->voxel_size.y = scale*volume_new_info->voxel_size.y;
	volume_new_info->voxel_size.z = scale*volume_new_info->voxel_size.z;
	update_size_y = update_size_z = TRUE;
      }
      volume_new_info->voxel_size.x = temp_val;
    }
    break;
  case VOXEL_SIZE_Y:
    if (temp_val > SMALL) { /* can't be having negative/very small numbers */
      if (aspect_ratio) {
	scale = temp_val/volume_new_info->voxel_size.y;
	volume_new_info->voxel_size.x = scale*volume_new_info->voxel_size.x;
	volume_new_info->voxel_size.z = scale*volume_new_info->voxel_size.z;
	update_size_x = update_size_z = TRUE;
      }
      volume_new_info->voxel_size.y = temp_val;
    }
    break;
  case VOXEL_SIZE_Z:
    if (temp_val > SMALL) { /* can't be having negative/very small numbers */
      if (aspect_ratio) {
	scale = temp_val/volume_new_info->voxel_size.z;
	volume_new_info->voxel_size.y = scale*volume_new_info->voxel_size.y;
	volume_new_info->voxel_size.x = scale*volume_new_info->voxel_size.x;
	update_size_x = update_size_y = TRUE;
      }
      volume_new_info->voxel_size.z = temp_val;
    }
    break;
  case CENTER_X:
    new_center.x = temp_val;
    break;
  case CENTER_Y:
    new_center.y = temp_val;
    break;
  case CENTER_Z:
    new_center.z = temp_val;
    break;
  case CONVERSION_FACTOR:
    if (fabs(temp_val) > CLOSE) /* avoid zero... */
      volume_new_info->conversion = temp_val;
    break;
  case SCAN_START:
    volume_new_info->scan_start = temp_val;
    break;
  case FRAME_DURATION:
    if (temp_val > CLOSE) {/* avoid zero and negatives */
      i = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "frame"));
      volume_new_info->frame_duration[i] = temp_val;
    }
    break;
  default:
    break; /* do nothing */
  }
  
  /* recalculate the volume's offset based on the new center */
  REALSPACE_SUB(new_center, old_center, shift);

  /* and save any changes to the coord frame */
  REALSPACE_ADD(volume_new_info->coord_frame.offset, shift, volume_new_info->coord_frame.offset);

  /* update the entry widgets as necessary */
  if (update_size_x) {
    entry =  gtk_object_get_data(GTK_OBJECT(volume_dialog), "voxel_size_x");
    gtk_signal_handler_block_by_func(GTK_OBJECT(entry),
    				     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_entry), 
    				     volume_new_info);
    str = g_strdup_printf("%f", volume_new_info->voxel_size.x);
    gtk_entry_set_text(GTK_ENTRY(entry), str);
    g_free(str);
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(entry), 
    				       GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_entry), 
    				       volume_new_info);
  }
  if (update_size_y) {
    entry =  gtk_object_get_data(GTK_OBJECT(volume_dialog), "voxel_size_y");
    gtk_signal_handler_block_by_func(GTK_OBJECT(entry),
    				     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_entry), 
    				     volume_new_info);
    str = g_strdup_printf("%f", volume_new_info->voxel_size.y);
    gtk_entry_set_text(GTK_ENTRY(entry), str);
    g_free(str);
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(entry), 
    				       GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_entry), 
    				       volume_new_info);
  }
  if (update_size_z) {
    entry =  gtk_object_get_data(GTK_OBJECT(volume_dialog), "voxel_size_z");
    gtk_signal_handler_block_by_func(GTK_OBJECT(entry),
    				     GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_entry), 
    				     volume_new_info);
    str = g_strdup_printf("%f", volume_new_info->voxel_size.z);
    gtk_entry_set_text(GTK_ENTRY(entry), str);
    g_free(str);
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(entry), 
    				       GTK_SIGNAL_FUNC(ui_volume_dialog_callbacks_change_entry), 
    				       volume_new_info);
  }



  /* now tell the volume_dialog that we've changed */
  gnome_property_box_changed(GNOME_PROPERTY_BOX(volume_dialog));

  return;
}


/* function called when the aspect ratio button gets clicked */
void ui_volume_dialog_callbacks_aspect_ratio(GtkWidget * widget, gpointer data) {

  GtkWidget * volume_dialog;
  gboolean state;

  /* get the state of the button */
  state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  /* record the change */
  volume_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "volume_dialog");
  gtk_object_set_data(GTK_OBJECT(volume_dialog), "aspect_ratio", GINT_TO_POINTER(state));

  return;
}

/* function called when the modality type of the volume gets changed */
void ui_volume_dialog_callbacks_change_modality(GtkWidget * widget, gpointer data) {

  volume_t * volume_new_info = data;
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
void ui_volume_dialog_callbacks_change_axis(GtkAdjustment * adjustment, gpointer data) {

  ui_study_t * ui_study;
  volume_t * volume_new_info = data;
  view_t i_view;
  floatpoint_t rotation;
  GtkWidget * volume_dialog;
  realpoint_t center, temp;

  /* we need the current view_axis so that we know what we're rotating around */
  ui_study = gtk_object_get_data(GTK_OBJECT(adjustment), "ui_study"); 

  /* saving the center, as we're rotating the volume around it's own center */
  center = volume_calculate_center(volume_new_info); 

  /* figure out which scale widget called me */
  i_view = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(adjustment),"view"));

  rotation = (adjustment->value/180)*M_PI; /* get rotation in radians */

  switch(i_view) {
  case TRANSVERSE:
    volume_new_info->coord_frame.axis[XAXIS] = 
      realspace_rotate_on_axis(&volume_new_info->coord_frame.axis[XAXIS],
			       &ui_study->current_view_coord_frame.axis[ZAXIS],
			       rotation);
    volume_new_info->coord_frame.axis[YAXIS] = 
      realspace_rotate_on_axis(&volume_new_info->coord_frame.axis[YAXIS],
			       &ui_study->current_view_coord_frame.axis[ZAXIS],
			       rotation);
    volume_new_info->coord_frame.axis[ZAXIS] = 
      realspace_rotate_on_axis(&volume_new_info->coord_frame.axis[ZAXIS],
			       &ui_study->current_view_coord_frame.axis[ZAXIS],
			       rotation);
    realspace_make_orthonormal(volume_new_info->coord_frame.axis); /* orthonormalize*/
    break;
  case CORONAL:
    volume_new_info->coord_frame.axis[XAXIS] = 
      realspace_rotate_on_axis(&volume_new_info->coord_frame.axis[XAXIS],
			       &ui_study->current_view_coord_frame.axis[YAXIS],
			       rotation);
    volume_new_info->coord_frame.axis[YAXIS] = 
      realspace_rotate_on_axis(&volume_new_info->coord_frame.axis[YAXIS],
			       &ui_study->current_view_coord_frame.axis[YAXIS],
			       rotation);
    volume_new_info->coord_frame.axis[ZAXIS] = 
      realspace_rotate_on_axis(&volume_new_info->coord_frame.axis[ZAXIS],
			       &ui_study->current_view_coord_frame.axis[YAXIS],
			       rotation);
    realspace_make_orthonormal(volume_new_info->coord_frame.axis); /* orthonormalize*/
    break;
  case SAGITTAL:
    volume_new_info->coord_frame.axis[XAXIS] = 
      realspace_rotate_on_axis(&volume_new_info->coord_frame.axis[XAXIS],
			       &ui_study->current_view_coord_frame.axis[XAXIS],
			       rotation);
    volume_new_info->coord_frame.axis[YAXIS] = 
      realspace_rotate_on_axis(&volume_new_info->coord_frame.axis[YAXIS],
			       &ui_study->current_view_coord_frame.axis[XAXIS],
			       rotation);
    volume_new_info->coord_frame.axis[ZAXIS] = 
      realspace_rotate_on_axis(&volume_new_info->coord_frame.axis[ZAXIS],
			       &ui_study->current_view_coord_frame.axis[XAXIS],
			       rotation);
    realspace_make_orthonormal(volume_new_info->coord_frame.axis); /* orthonormalize*/
    break;
  default:
    break;
  }

  
  /* recalculate the offset of this volume based on the center we stored */
  REALSPACE_CMULT(-0.5,volume_new_info->corner,temp);
  volume_new_info->coord_frame.offset = center;
  volume_new_info->coord_frame.offset = 
    realspace_alt_coord_to_base(temp, volume_new_info->coord_frame);

  /* return adjustment back to normal */
  adjustment->value = 0.0;
  gtk_adjustment_changed(adjustment);


  /* now tell the volume_dialog that we've changed */
  volume_dialog =  gtk_object_get_data(GTK_OBJECT(adjustment), "volume_dialog");
  ui_volume_dialog_set_axis_display(volume_dialog);
  gnome_property_box_changed(GNOME_PROPERTY_BOX(volume_dialog));

  return;
}

/* function to reset the volume's axis back to the default coords */
void ui_volume_dialog_callbacks_reset_axis(GtkWidget* widget, gpointer data) {

  volume_t * volume_new_info = data;
  axis_t i_axis;
  GtkWidget * volume_dialog;
  realpoint_t center, temp;

  /* saving the center, as we're rotating the volume around it's own center */
  center = volume_calculate_center(volume_new_info); 

  /* reset the axis */
  for (i_axis=0;i_axis<NUM_AXIS;i_axis++) {
    volume_new_info->coord_frame.axis[i_axis] = default_axis[i_axis];
  }

  /* recalculate the offset of this volume based on the center we stored */
  REALSPACE_CMULT(-0.5,volume_new_info->corner,temp);
  volume_new_info->coord_frame.offset = center;
  volume_new_info->coord_frame.offset = realspace_alt_coord_to_base(temp, volume_new_info->coord_frame);

  /* now tell the volume_dialog that we've changed */
  volume_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "volume_dialog");
  ui_volume_dialog_set_axis_display(volume_dialog);
  gnome_property_box_changed(GNOME_PROPERTY_BOX(volume_dialog));
  
  return;
}

/* function called when we hit the apply button */
void ui_volume_dialog_callbacks_apply(GtkWidget* widget, gint page_number, gpointer data) {
  
  ui_volume_list_t * ui_volume_list_item = data;
  ui_study_t * ui_study;
  GdkPixmap * pixmap;
  guint8 spacing;
  volume_t * volume_new_info;
  GdkWindow * parent;
  volume_data_t scale;
  guint i;
  
  /* we'll apply all page changes at once */
  if (page_number != -1)
    return;

  /* get a pointer to ui_study so we can redraw the volume's */
  ui_study = gtk_object_get_data(GTK_OBJECT(ui_volume_list_item->dialog), "ui_study"); 

  /* get the new info for the volume */
  volume_new_info = gtk_object_get_data(GTK_OBJECT(ui_volume_list_item->dialog),"volume_new_info");

  /* sanity check */
  if (volume_new_info == NULL) {
    g_printerr("%s, volume_new_info inappropriately null....\n", PACKAGE);
    return;
  }

  /* copy the new info on over */
  ui_volume_list_item->volume->modality = volume_new_info->modality;
  ui_volume_list_item->volume->color_table = volume_new_info->color_table;
  ui_volume_list_item->volume->coord_frame = volume_new_info->coord_frame;
  ui_volume_list_item->volume->voxel_size = volume_new_info->voxel_size;
  volume_set_name(ui_volume_list_item->volume, volume_new_info->name);

  /* apply any changes in the conversion factor */
  if (!FLOATPOINT_EQUAL(ui_volume_list_item->volume->conversion, volume_new_info->conversion)) {
    scale = (volume_new_info->conversion)/(ui_volume_list_item->volume->conversion);
    ui_volume_list_item->volume->conversion = scale * ui_volume_list_item->volume->conversion;
    ui_volume_list_item->volume->max = scale *  ui_volume_list_item->volume->max;
    ui_volume_list_item->volume->min = scale *  ui_volume_list_item->volume->min;
    ui_volume_list_item->volume->threshold_max = scale *  ui_volume_list_item->volume->threshold_max;
    ui_volume_list_item->volume->threshold_min = scale *  ui_volume_list_item->volume->threshold_min;
    ui_threshold_update_entries(ui_volume_list_item->threshold);
    /* note, the threshold bar graph does not need to be redrawn as it's values
       are all relative anyway */
  }

  /* reset the far corner */
  REALSPACE_MULT(ui_volume_list_item->volume->dim,ui_volume_list_item->volume->voxel_size,
		 ui_volume_list_item->volume->corner);

  /* apply any time changes, and recalculate the frame selection
     widget in case any timing information in this volume has changed */
  ui_volume_list_item->volume->scan_start = volume_new_info->scan_start;
  for (i=0;i<ui_volume_list_item->volume->num_frames;i++)
    ui_volume_list_item->volume->frame_duration[i] = volume_new_info->frame_duration[i];
  ui_time_dialog_set_times(ui_study);


  /* apply any changes to the name of the widget */
  /* get the current pixmap and spacing in the line of the tree corresponding to this volume */
  gtk_ctree_node_get_pixtext(ui_volume_list_item->tree, ui_volume_list_item->tree_node, 0,
			     NULL, &spacing, &pixmap, NULL);
  g_free(pixmap);

  parent = gtk_widget_get_parent_window(ui_study->tree);
  
  /* which icon to use */
  switch (ui_volume_list_item->volume->modality) {
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
  gtk_ctree_node_set_pixtext(ui_volume_list_item->tree, ui_volume_list_item->tree_node, 0, 
			     ui_volume_list_item->volume->name, spacing, pixmap, NULL);



  /* redraw the volume */
  ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_ALL);

  return;
}

/* callback for the help button */
void ui_volume_dialog_callbacks_help(GnomePropertyBox *volume_dialog, gint page_number, gpointer data) {

  GnomeHelpMenuEntry help_ref={PACKAGE,"basics.html#VOLUME-DIALOG-HELP"};
  GnomeHelpMenuEntry help_ref_0 = {PACKAGE,"basics.html#VOLUME-DIALOG-HELP-BASIC"};
  GnomeHelpMenuEntry help_ref_1 = {PACKAGE,"basics.html#VOLUME-DIALOG-HELP-CENTER-DIMENSIONS"};
  GnomeHelpMenuEntry help_ref_2 = {PACKAGE,"basics.html#VOLUME-DIALOG-HELP-ROTATE"};
  GnomeHelpMenuEntry help_ref_3 = {PACKAGE,"basics.html#VOLUME-DIALOG-HELP-COLORMAP_THRESHOLD"};
  GnomeHelpMenuEntry help_ref_4 = {PACKAGE,"basics.html#VOLUME-DIALOG-HELP-TIME"};


  switch (page_number) {
  case 0:
    gnome_help_display (0, &help_ref_0);
    break;
  case 1:
    gnome_help_display (0, &help_ref_1);
    break;
  case 2:
    gnome_help_display (0, &help_ref_2);
    break;
  case 3:
    gnome_help_display (0, &help_ref_3);
    break;
  case 4:
    gnome_help_display (0, &help_ref_4);
    break;
  default:
    gnome_help_display (0, &help_ref);
    break;
  }

  return;
}

/* function called to destroy the volume dialog */
void ui_volume_dialog_callbacks_close_event(GtkWidget* widget, gpointer data) {

  ui_volume_list_t * ui_volume_list_item = data;
  volume_t * volume_new_info;

  /* trash collection */
  volume_new_info = gtk_object_get_data(GTK_OBJECT(widget), "volume_new_info");
  volume_new_info = volume_free(volume_new_info);

  /* destroy the widget */
  gtk_widget_destroy(GTK_WIDGET(ui_volume_list_item->dialog));

  /* make sure the pointer in the ui_volume_list_item is nulled */
  ui_volume_list_item->dialog = NULL;

  return;
}


