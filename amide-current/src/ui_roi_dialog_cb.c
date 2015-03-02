/* ui_roi_dialog_cb.c
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
#include "amitk_coord_frame_edit.h"
#include "amitk_canvas.h"
#include "image.h"
#include "ui_study.h"
#include "ui_roi_dialog.h"
#include "ui_roi_dialog_cb.h"
#include "amitk_tree.h"
#include "amitk_tree_item.h"

/* function called when the name of the roi has been changed */
void ui_roi_dialog_cb_change_name(GtkWidget * widget, gpointer data) {

  roi_t * new_info;
  gchar * new_name;
  GtkWidget * dialog = data;

  new_info = gtk_object_get_data(GTK_OBJECT(dialog), "new_info");

  /* get the contents of the name entry box and save it */
  new_name = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  roi_set_name(new_info, new_name);
  g_free(new_name);

  /* tell the roi_dialog that we've changed */
  gnome_property_box_changed(GNOME_PROPERTY_BOX(dialog));

  return;
}

/* function called when a numerical entry of the roi has been changed, 
   used for the coordinate frame offset, axis, and the roi's corner*/
void ui_roi_dialog_cb_change_entry(GtkWidget * widget, gpointer data) {

  roi_t * new_info;
  ui_study_t * ui_study;
  gchar * str;
  gint error;
  gdouble temp_val;
  which_entry_widget_t which_widget;
  realpoint_t shift;
  realpoint_t old_center;
  realpoint_t new_center;
  realpoint_t new_corner;
  realpoint_t old_corner;
  realpoint_t new_offset;
  GtkWidget * dialog=data;
  GtkWidget * cfe;

  new_info = gtk_object_get_data(GTK_OBJECT(dialog), "new_info");

  ui_study = gtk_object_get_data(GTK_OBJECT(dialog), "ui_study"); 
  cfe = gtk_object_get_data(GTK_OBJECT(dialog), "cfe");

  /* initialize the center and dimension variables based on the old roi info */
  old_center = roi_calculate_center(new_info); /* in base coords */
  new_center = realspace_base_coord_to_alt(old_center, study_coord_frame(ui_study->study));
  old_corner = new_info->corner; /* in roi's coords */
  new_corner = old_corner;

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
    new_center.x = temp_val;
    break;
  case CENTER_Y:
    new_center.y = temp_val;
    break;
  case CENTER_Z:
    new_center.z = temp_val;
    break;
  case DIM_X:
    new_corner.x = fabs(temp_val);
    break;
  case DIM_Y:
    new_corner.y = fabs(temp_val);
    break;
  case DIM_Z:
    new_corner.z = fabs(temp_val);
    break;
  default:
    return;
    break; /* do nothing */
  }

  /* recalculate the roi's offset based on the new dimensions/center/and axis */
  new_center = realspace_alt_coord_to_base(new_center, study_coord_frame(ui_study->study));
  shift = rp_sub(new_center, old_center);
  new_offset = rp_add(rs_offset(new_info->coord_frame), shift);

  /* reset the far corner */
  new_info->corner = new_corner;
  old_corner = realspace_alt_coord_to_base(old_corner, new_info->coord_frame);
  new_corner = realspace_alt_coord_to_base(new_corner, new_info->coord_frame);
  shift = rp_cmult(-0.5, rp_sub(new_corner, old_corner));
  new_offset = rp_add(new_offset, shift);

  /* apply changes to the coord frame */
  amitk_coord_frame_edit_change_center(AMITK_COORD_FRAME_EDIT(cfe), new_center);
  amitk_coord_frame_edit_change_offset(AMITK_COORD_FRAME_EDIT(cfe), new_offset);

  /* adjust the voxel size if appropriate */
  if (new_info->type == ISOCONTOUR_2D)
    new_info->voxel_size.z = new_corner.z;

  gnome_property_box_changed(GNOME_PROPERTY_BOX(dialog));

  return;
}



/* function to change an roi's type */
void ui_roi_dialog_cb_change_type(GtkWidget * widget, gpointer data) {

  roi_t * new_info;
  roi_type_t i_roi_type;
  GtkWidget * dialog = data;

  new_info = gtk_object_get_data(GTK_OBJECT(dialog), "new_info");

  /* figure out which menu item called me */
  i_roi_type = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"roi_type"));
  new_info->type = i_roi_type;  /* save the new roi_type until it's applied */

  gnome_property_box_changed(GNOME_PROPERTY_BOX(dialog));

  return;
}


/* function called when the coord frame has been changed */
void ui_roi_dialog_cb_coord_frame_changed(GtkWidget * widget, gpointer data) {
  GtkWidget * dialog = data;
  gnome_property_box_changed(GNOME_PROPERTY_BOX(dialog)); /* tell the dialog that we've changed */
  return;
}



/* function called when we hit the apply button */
void ui_roi_dialog_cb_apply(GtkWidget* dialog, gint page_number, gpointer data) {

  ui_study_t * ui_study=data;
  roi_t * new_info;
  roi_t * old_info;
  view_t i_view;
  
  /* we'll apply all page changes at once */
  if (page_number != -1) return;

  g_return_if_fail(ui_study != NULL);
  new_info = gtk_object_get_data(GTK_OBJECT(dialog),"new_info");
  g_return_if_fail(new_info != NULL);
  old_info = gtk_object_get_data(GTK_OBJECT(dialog),"old_info");
  g_return_if_fail(old_info != NULL);

  /* copy the new info on over */
  roi_set_name(old_info, new_info->name);
  old_info->type = new_info->type;
  rs_set_coord_frame(old_info->coord_frame, new_info->coord_frame);
  old_info->corner = new_info->corner;
  old_info->voxel_size = new_info->voxel_size;
  old_info->parent = roi_unref(old_info->parent);
  if (new_info->parent != NULL) 
    old_info->parent = roi_copy(new_info->parent);
  old_info->children = rois_unref(old_info->children);
  if (new_info->children != NULL) 
    old_info->children = rois_copy(new_info->children);

  /* apply any changes to the name of the roi and the modality*/
  amitk_tree_update_object(AMITK_TREE(ui_study->tree), old_info, ROI);

  /* redraw the roi */
  for (i_view=0;i_view<NUM_VIEWS;i_view++)
    amitk_canvas_update_object(AMITK_CANVAS(ui_study->canvas[i_view]), old_info, ROI);

  return;
}

/* callback for the help button */
void ui_roi_dialog_cb_help(GnomePropertyBox *dialog, gint page_number, gpointer data) {

  GnomeHelpMenuEntry help_ref={PACKAGE,"basics.html#ROI-DIALOG-HELP"};
  GnomeHelpMenuEntry help_ref_0 = {PACKAGE,"basics.html#ROI-DIALOG-HELP-BASIC"};
  GnomeHelpMenuEntry help_ref_1 = {PACKAGE,"basics.html#ROI-DIALOG-HELP-CENTER"};
  GnomeHelpMenuEntry help_ref_2 = {PACKAGE,"basics.html#ROI-DIALOG-HELP-DIMENSIONS"};
  GnomeHelpMenuEntry help_ref_3 = {PACKAGE,"basics.html#ROI-DIALOG-HELP-ROTATE"};


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
  default:
    gnome_help_display (0, &help_ref);
    break;
  }

  return;
}

/* function called to destroy the roi dialog */
gboolean ui_roi_dialog_cb_close(GtkWidget* dialog, gpointer data) {

  ui_study_t * ui_study=data;
  GtkWidget * tree_item;
  roi_t * new_info;
  roi_t * old_info;

  /* trash collection */
  new_info = gtk_object_get_data(GTK_OBJECT(dialog), "new_info");
  g_return_val_if_fail(new_info != NULL, FALSE);

  old_info = gtk_object_get_data(GTK_OBJECT(dialog), "old_info");
  g_return_val_if_fail(old_info != NULL, FALSE);

#if AMIDE_DEBUG
  { /* little something to make the debugging messages make sense */
    gchar * temp_string;
    temp_string = g_strdup_printf("Copy of %s",new_info->name);
    roi_set_name(new_info, temp_string);
    g_free(temp_string);

  }
#endif
  new_info = roi_unref(new_info);

  /* record that the dialog is no longer up */
  tree_item = amitk_tree_find_object(AMITK_TREE(ui_study->tree), old_info, ROI);
  g_return_val_if_fail(AMITK_IS_TREE_ITEM(tree_item), FALSE);
  AMITK_TREE_ITEM_DIALOG(tree_item) = NULL;

  return FALSE;
}


