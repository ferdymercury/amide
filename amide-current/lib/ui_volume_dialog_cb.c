/* ui_volume_dialog_cb.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2002 Andy Loening
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
#include "image.h"
#include "amitk_coord_frame_edit.h"
#include "amitk_threshold.h"
#include "amitk_canvas.h"
#include "ui_study.h"
#include "ui_volume_dialog.h"
#include "ui_volume_dialog_cb.h"
#include "ui_time_dialog.h"
#include "amitk_tree.h"
#include "amitk_tree_item.h"

/* function called when the name of the volume has been changed */
void ui_volume_dialog_cb_change_name(GtkWidget * widget, gpointer data) {

  volume_t * new_info;
  gchar * new_name;
  GtkWidget * dialog=data;

  new_info = gtk_object_get_data(GTK_OBJECT(dialog), "new_info");

  /* get the contents of the name entry box and save it */
  new_name = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  volume_set_name(new_info, new_name);
  g_free(new_name);

  gnome_property_box_changed(GNOME_PROPERTY_BOX(dialog));

  return;
}

/* function called when the scan date of the volume has been changed */
void ui_volume_dialog_cb_change_scan_date(GtkWidget * widget, gpointer data) {

  volume_t * new_info;
  gchar * new_date;
  GtkWidget * dialog=data;

  new_info = gtk_object_get_data(GTK_OBJECT(dialog), "new_info");

  /* get the contents of the name entry box and save it */
  new_date = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  volume_set_scan_date(new_info, new_date);
  g_free(new_date);

  gnome_property_box_changed(GNOME_PROPERTY_BOX(dialog));

  return;
}

/* when a numerical entry of the volume has been changed, 
   used for the coordinate frame offset, */
void ui_volume_dialog_cb_change_entry(GtkWidget * widget, gpointer data) {

  volume_t * new_info;
  ui_study_t * ui_study;
  gchar * str;
  gint error;
  gdouble temp_val;
  floatpoint_t scale;
  which_entry_widget_t which_widget;
  realpoint_t old_center, new_center, shift;
  realpoint_t new_offset;
  GtkWidget * dialog=data;
  GtkWidget * entry;
  GtkWidget * cfe;
  guint i;
  gboolean aspect_ratio=TRUE;
  gboolean update_size_x = FALSE;
  gboolean update_size_y = FALSE;
  gboolean update_size_z = FALSE;

  new_info = gtk_object_get_data(GTK_OBJECT(dialog), "new_info");

  /* get the pointer ot the volume dialog and get any necessary info */
  aspect_ratio =  GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(dialog), "aspect_ratio"));
  ui_study = gtk_object_get_data(GTK_OBJECT(dialog), "ui_study"); 
  cfe = gtk_object_get_data(GTK_OBJECT(dialog), "cfe");

  /* initialize the center variables based on the old volume info */
  old_center = volume_center(new_info); /* in real coords */
  new_center = realspace_base_coord_to_alt(old_center, study_coord_frame(ui_study->study));

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
  case VOXEL_SIZE_X:
    if (temp_val > SMALL) { /* can't be having negative/very small numbers */
      if (aspect_ratio) {
	scale = temp_val/new_info->voxel_size.x;
	new_info->voxel_size.y = scale*new_info->voxel_size.y;
	new_info->voxel_size.z = scale*new_info->voxel_size.z;
	update_size_y = update_size_z = TRUE;
      }
      new_info->voxel_size.x = temp_val;
    }
    break;
  case VOXEL_SIZE_Y:
    if (temp_val > SMALL) { /* can't be having negative/very small numbers */
      if (aspect_ratio) {
	scale = temp_val/new_info->voxel_size.y;
	new_info->voxel_size.x = scale*new_info->voxel_size.x;
	new_info->voxel_size.z = scale*new_info->voxel_size.z;
	update_size_x = update_size_z = TRUE;
      }
      new_info->voxel_size.y = temp_val;
    }
    break;
  case VOXEL_SIZE_Z:
    if (temp_val > SMALL) { /* can't be having negative/very small numbers */
      if (aspect_ratio) {
	scale = temp_val/new_info->voxel_size.z;
	new_info->voxel_size.y = scale*new_info->voxel_size.y;
	new_info->voxel_size.x = scale*new_info->voxel_size.x;
	update_size_x = update_size_y = TRUE;
      }
      new_info->voxel_size.z = temp_val;
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
  case SCALING_FACTOR:
    if (fabs(temp_val) > CLOSE) /* avoid zero... */
      volume_set_scaling(new_info, temp_val);
    break;
  case SCAN_START:
    new_info->scan_start = temp_val;
    break;
  case FRAME_DURATION:
    if (temp_val > CLOSE) {/* avoid zero and negatives */
      i = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "frame"));
      new_info->frame_duration[i] = temp_val;
    }
    break;
  default:
    break; /* do nothing */
  }
  
  /* recalculate the volume's offset based on the new center */
  new_center = realspace_alt_coord_to_base(new_center, study_coord_frame(ui_study->study));
  shift = rp_sub(new_center, old_center);

  /* and save any changes to the coord frame */
  new_offset = rp_add(rs_offset(new_info->coord_frame), shift);
  amitk_coord_frame_edit_change_center(AMITK_COORD_FRAME_EDIT(cfe), new_center);
  amitk_coord_frame_edit_change_offset(AMITK_COORD_FRAME_EDIT(cfe), new_offset);

  /* update the entry widgets as necessary */
  if (update_size_x) {
    entry =  gtk_object_get_data(GTK_OBJECT(dialog), "voxel_size_x");
    gtk_signal_handler_block_by_func(GTK_OBJECT(entry),
				     GTK_SIGNAL_FUNC(ui_volume_dialog_cb_change_entry), dialog);
    str = g_strdup_printf("%f", new_info->voxel_size.x);
    gtk_entry_set_text(GTK_ENTRY(entry), str);
    g_free(str);
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(entry), 
    				       GTK_SIGNAL_FUNC(ui_volume_dialog_cb_change_entry), dialog);
  }
  if (update_size_y) {
    entry =  gtk_object_get_data(GTK_OBJECT(dialog), "voxel_size_y");
    gtk_signal_handler_block_by_func(GTK_OBJECT(entry),
    				     GTK_SIGNAL_FUNC(ui_volume_dialog_cb_change_entry), dialog);
    str = g_strdup_printf("%f", new_info->voxel_size.y);
    gtk_entry_set_text(GTK_ENTRY(entry), str);
    g_free(str);
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(entry), 
    				       GTK_SIGNAL_FUNC(ui_volume_dialog_cb_change_entry), dialog);
  }
  if (update_size_z) {
    entry =  gtk_object_get_data(GTK_OBJECT(dialog), "voxel_size_z");
    gtk_signal_handler_block_by_func(GTK_OBJECT(entry),
    				     GTK_SIGNAL_FUNC(ui_volume_dialog_cb_change_entry), dialog);
    str = g_strdup_printf("%f", new_info->voxel_size.z);
    gtk_entry_set_text(GTK_ENTRY(entry), str);
    g_free(str);
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(entry), 
    				       GTK_SIGNAL_FUNC(ui_volume_dialog_cb_change_entry), dialog);
  }

  /* now tell the volume_dialog that we've changed */
  gnome_property_box_changed(GNOME_PROPERTY_BOX(dialog));

  return;
}


/* function called when the aspect ratio button gets clicked */
void ui_volume_dialog_cb_aspect_ratio(GtkWidget * widget, gpointer data) {

  GtkWidget * dialog=data;
  gboolean state;

  /* get the state of the button */
  state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  /* record the change */
  gtk_object_set_data(GTK_OBJECT(dialog), "aspect_ratio", GINT_TO_POINTER(state));

  return;
}

/* function called when the modality type of the volume gets changed */
void ui_volume_dialog_cb_change_modality(GtkWidget * widget, gpointer data) {

  volume_t * new_info;
  modality_t i_modality;
  GtkWidget * dialog=data;

  new_info = gtk_object_get_data(GTK_OBJECT(dialog), "new_info");

  /* figure out which menu item called me */
  for (i_modality=0;i_modality<NUM_MODALITIES;i_modality++)       
    if (GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"modality")) == i_modality)
      new_info->modality = i_modality;  /* save the new modality until it's applied */

  /* now tell the volume_dialog that we've changed */
  gnome_property_box_changed(GNOME_PROPERTY_BOX(dialog));

  return;
}



/* function called when the coord frame has been changed */
void ui_volume_dialog_cb_coord_frame_changed(GtkWidget * widget, gpointer data) {
  GtkWidget * dialog = data;
  gnome_property_box_changed(GNOME_PROPERTY_BOX(dialog)); /* tell the dialog that we've changed */
  return;
}

/* function called when we hit the apply button */
void ui_volume_dialog_cb_apply(GtkWidget* dialog, gint page_number, gpointer data) {
  
  ui_study_t * ui_study=data;
  volume_t * new_info;
  volume_t * old_info;
  amide_data_t scale;
  guint i_frame;
  view_t i_view;
  view_mode_t i_view_mode;
  GtkWidget * threshold;
  
  /* we'll apply all page changes at once */
  if (page_number != -1) return;

  g_return_if_fail(ui_study != NULL);
  new_info = gtk_object_get_data(GTK_OBJECT(dialog),"new_info");
  g_return_if_fail(new_info != NULL);
  old_info = gtk_object_get_data(GTK_OBJECT(dialog),"old_info");
  g_return_if_fail(old_info != NULL);
  threshold = gtk_object_get_data(GTK_OBJECT(dialog),"threshold");
  g_return_if_fail(threshold != NULL);

  /* copy the needed new info on over */
  old_info->modality = new_info->modality;
  rs_set_coord_frame(old_info->coord_frame,new_info->coord_frame);
  old_info->voxel_size = new_info->voxel_size;
  volume_set_name(old_info,new_info->name);

  /* apply any changes in the scaling factor */
  if (!FLOATPOINT_EQUAL(old_info->external_scaling, new_info->external_scaling)) {
    scale = (new_info->external_scaling)/(old_info->external_scaling);
    volume_set_scaling(old_info, new_info->external_scaling);
    old_info->global_max *= scale;
    old_info->global_min *= scale;
    old_info->threshold_max[0] *= scale;
    old_info->threshold_min[0] *= scale;
    old_info->threshold_max[1] *= scale;
    old_info->threshold_min[1] *= scale;
    for (i_frame=0; i_frame<old_info->data_set->dim.t; i_frame++) {
      old_info->frame_max[i_frame] *= scale;
      old_info->frame_min[i_frame] *= scale;
    }
    amitk_threshold_update(AMITK_THRESHOLD(threshold));
    /* note, the threshold bar graph does not need to be redrawn as it's values
       are all relative anyway */
  }

  /* reset the far corner */
  volume_recalc_far_corner(old_info);

  /* apply any time changes, and recalculate the frame selection
     widget in case any timing information in this volume has changed */
  old_info->scan_start = new_info->scan_start;
  for (i_frame=0;i_frame<old_info->data_set->dim.t;i_frame++)
    old_info->frame_duration[i_frame] = new_info->frame_duration[i_frame];
  ui_time_dialog_set_times(ui_study);



  /* redraw the volume */
  for (i_view_mode=0; i_view_mode <= ui_study->view_mode; i_view_mode++) {

    /* change the modality icon */
    amitk_tree_update_object(AMITK_TREE(ui_study->tree[i_view_mode]), old_info, VOLUME);

    if (volumes_includes_volume(AMITK_TREE_SELECTED_VOLUMES(ui_study->tree[i_view_mode]), old_info)) {
      for (i_view=0; i_view<NUM_VIEWS; i_view++)
	amitk_canvas_update_object(AMITK_CANVAS(ui_study->canvas[i_view_mode][i_view]), old_info, VOLUME);
    }
  }

  return;
}

/* callback for the help button */
void ui_volume_dialog_cb_help(GnomePropertyBox *dialog, gint page_number, gpointer data) {

  GnomeHelpMenuEntry help_ref={PACKAGE,"basics.html#VOLUME-DIALOG-HELP"};
  GnomeHelpMenuEntry help_ref_0 = {PACKAGE,"basics.html#VOLUME-DIALOG-HELP-BASIC"};
  GnomeHelpMenuEntry help_ref_1 = {PACKAGE,"basics.html#VOLUME-DIALOG-HELP-CENTER-DIMENSIONS"};
  GnomeHelpMenuEntry help_ref_2 = {PACKAGE,"basics.html#VOLUME-DIALOG-HELP-ROTATE"};
  GnomeHelpMenuEntry help_ref_3 = {PACKAGE,"basics.html#VOLUME-DIALOG-HELP-COLORMAP_THRESHOLD"};
  GnomeHelpMenuEntry help_ref_4 = {PACKAGE,"basics.html#VOLUME-DIALOG-HELP-TIME"};
  GnomeHelpMenuEntry help_ref_5 = {PACKAGE,"basics.html#VOLUME-DIALOG-HELP-IMMUTABLES"};


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
  case 5:
    gnome_help_display (0, &help_ref_5);
    break;
  default:
    gnome_help_display (0, &help_ref);
    break;
  }

  return;
}

/* function called to destroy the volume dialog */
gboolean ui_volume_dialog_cb_close(GtkWidget* dialog, gpointer data) {

  ui_study_t * ui_study=data;
  GtkWidget * tree_item;
  volume_t * new_info;
  volume_t * old_info;

  /* trash collection */
  new_info = gtk_object_get_data(GTK_OBJECT(dialog), "new_info");
  g_return_val_if_fail(new_info != NULL, FALSE);

  old_info = gtk_object_get_data(GTK_OBJECT(dialog), "old_info");
  g_return_val_if_fail(old_info != NULL, FALSE);

#if AMIDE_DEBUG
  { /* little something to make the debugging messages make sense */
    gchar * temp_string;
    temp_string = g_strdup_printf("Copy of %s",new_info->name);
    volume_set_name(new_info, temp_string);
    g_free(temp_string);

  }
#endif
  new_info = volume_unref(new_info);

  /* record that the dialog is no longer up */
  tree_item = amitk_tree_find_object(AMITK_TREE(ui_study->tree[SINGLE_VIEW]), old_info, VOLUME);
  g_return_val_if_fail(AMITK_IS_TREE_ITEM(tree_item), FALSE);
  AMITK_TREE_ITEM_DIALOG(tree_item) = NULL;

  return FALSE;
}


