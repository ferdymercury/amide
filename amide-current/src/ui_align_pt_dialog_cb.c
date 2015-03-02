/* ui_align_pt_dialog_cb.c
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
#include "ui_align_pt_dialog.h"
#include "ui_align_pt_dialog_cb.h"
#include "amitk_canvas.h"
#include "amitk_tree.h"
#include "amitk_tree_item.h"

/* function called when the name of the alignment point has been changed */
void ui_align_pt_dialog_cb_change_name(GtkWidget * widget, gpointer data) {

  GtkWidget * dialog = data;
  align_pt_t * new_info;
  gchar * new_name;

  new_info = gtk_object_get_data(GTK_OBJECT(dialog),"new_info");
  g_return_if_fail(new_info != NULL);

    /* get the contents of the name entry box and save it */
  new_name = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  align_pt_set_name(new_info, new_name);
  g_free(new_name);

  /* tell the_dialog that we've changed */
  gnome_property_box_changed(GNOME_PROPERTY_BOX(dialog));

  return;
}

/* function called when a numerical entry of the alignment point has been changed */
void ui_align_pt_dialog_cb_change_entry(GtkWidget * widget, gpointer data) {

  GtkWidget * dialog = data;
  align_pt_t * new_info;
  volume_t * volume;
  ui_study_t * ui_study;
  gchar * str;
  gint error;
  gdouble temp_val;
  which_entry_widget_t which_widget;
  realpoint_t temp_center;

  new_info = gtk_object_get_data(GTK_OBJECT(dialog),"new_info");
  g_return_if_fail(new_info != NULL);
  
  /* figure out which widget this is */
  which_widget = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "type")); 

  /* get a pointer to the volume */
  volume = gtk_object_get_data(GTK_OBJECT(dialog), "volume");
  ui_study = gtk_object_get_data(GTK_OBJECT(dialog), "ui_study"); 

  temp_center = realspace_alt_coord_to_alt(new_info->point, 
					   volume->coord_frame,
					   study_coord_frame(ui_study->study));
  
  /* get the contents of the name entry box */
  str = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert to a floating point */
  error = sscanf(str, "%lf", &temp_val);
  g_free(str);

  if (error == EOF)  /* make sure it's a valid number */
    return;
  
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
  default:
    return;
    break; /* do nothing */
  }

  new_info->point = realspace_alt_coord_to_alt(temp_center, 
					       study_coord_frame(ui_study->study),
					       volume->coord_frame);

  /* now tell the dialog that we've changed */
  gnome_property_box_changed(GNOME_PROPERTY_BOX(dialog));

  return;
}




/* function called when we hit the apply button */
void ui_align_pt_dialog_cb_apply(GtkWidget* dialog, gint page_number, gpointer data) {
  
  ui_study_t * ui_study=data;
  align_pt_t * new_info;
  align_pt_t * old_info;
  view_t i_view;
  
  /* we'll apply all page changes at once */
  if (page_number != -1) return;

  /* get the new info for the alignment point */
  new_info = gtk_object_get_data(GTK_OBJECT(dialog),"new_info");
  g_return_if_fail(new_info != NULL);
  old_info = gtk_object_get_data(GTK_OBJECT(dialog),"old_info");
  g_return_if_fail(old_info != NULL);
  
  /* copy the new info on over */
  align_pt_set_name(old_info, new_info->name);
  old_info->point = new_info->point;

  /* apply any changes to the name of the alignment point */
  amitk_tree_update_object(AMITK_TREE(ui_study->tree), old_info, ALIGN_PT);

  /* redraw the alignment point */
  for (i_view=0;i_view<NUM_VIEWS;i_view++)
    amitk_canvas_update_object(AMITK_CANVAS(ui_study->canvas[i_view]), old_info, ALIGN_PT);

  return;
}

/* callback for the help button */
void ui_align_pt_dialog_cb_help(GnomePropertyBox *dialog, gint page_number, gpointer data) {

  GnomeHelpMenuEntry help_ref={PACKAGE,"basics.html#ALIGN-PT-DIALOG-HELP"};
  GnomeHelpMenuEntry help_ref_0 = {PACKAGE,"basics.html#ALIGN-PT-DIALOG-HELP-BASIC"};

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

/* function called to destroy the dialog */
gboolean ui_align_pt_dialog_cb_close(GtkWidget* dialog, gpointer data) {

  ui_study_t * ui_study=data;
  GtkWidget * tree_item;
  align_pt_t * new_info;
  align_pt_t * old_info;


  /* trash collection */
  new_info = gtk_object_get_data(GTK_OBJECT(dialog), "new_info");
  g_return_val_if_fail(new_info != NULL, FALSE);

  old_info = gtk_object_get_data(GTK_OBJECT(dialog), "old_info");
  g_return_val_if_fail(old_info != NULL, FALSE);
#if AMIDE_DEBUG
  { /* little something to make the debugging messages make sense */
    gchar * temp_string;
    temp_string = g_strdup_printf("Copy of %s",new_info->name);
    align_pt_set_name(new_info, temp_string);
    g_free(temp_string);

  }
#endif
  new_info = align_pt_unref(new_info);

  /* record that the dialog is no longer up */
  tree_item = amitk_tree_find_object(AMITK_TREE(ui_study->tree), old_info, VOLUME);
  g_return_val_if_fail(AMITK_IS_TREE_ITEM(tree_item), FALSE);
  AMITK_TREE_ITEM_DIALOG(tree_item) = NULL;

  return FALSE;
}


