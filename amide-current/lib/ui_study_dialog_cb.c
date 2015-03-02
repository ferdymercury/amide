/* ui_study_dialog_cb.c
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
#include "ui_study.h"
#include "ui_study_dialog.h"
#include "ui_study_dialog_cb.h"
#include "amitk_canvas.h"
#include "amitk_tree.h"
#include "amitk_tree_item.h"

/* function called when the name of the study has been changed */
void ui_study_dialog_cb_change_name(GtkWidget * widget, gpointer data) {

  study_t * new_info;
  gchar * new_name;
  GtkWidget * dialog=data;

  new_info = gtk_object_get_data(GTK_OBJECT(dialog), "new_info");
  g_return_if_fail(new_info != NULL);

  /* get the contents of the name entry box and save it */
  new_name = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  study_set_name(new_info, new_name);
  g_free(new_name);

  /* tell the study_dialog that we've changed */
  gnome_property_box_changed(GNOME_PROPERTY_BOX(dialog));

  return;
}

/* function called when the creation date of the study has been changed */
void ui_study_dialog_cb_change_creation_date(GtkWidget * widget, gpointer data) {

  study_t * new_info;
  gchar * new_date;
  GtkWidget * dialog=data;

  new_info = gtk_object_get_data(GTK_OBJECT(dialog), "new_info");
  g_return_if_fail(new_info != NULL);

  /* get the contents of the name entry box and save it */
  new_date = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  study_set_creation_date(new_info, new_date);
  g_free(new_date);

  /* tell the study_dialog that we've changed */
  gnome_property_box_changed(GNOME_PROPERTY_BOX(dialog));

  return;
}

/* function called when the coord frame has been changed */
void ui_study_dialog_cb_coord_frame_changed(GtkWidget * widget, gpointer data) {
  GtkWidget * dialog = data;
  gtk_object_set_data(GTK_OBJECT(dialog), "axis_changed", GINT_TO_POINTER(TRUE));
  gnome_property_box_changed(GNOME_PROPERTY_BOX(dialog)); /* tell the dialog that we've changed */
  return;
}


/* function called when we hit the apply button */
void ui_study_dialog_cb_apply(GtkWidget* dialog, gint page_number, gpointer data) {
  
  ui_study_t * ui_study = data;
  study_t * new_info;
  realpoint_t center;
  view_t i_view;
  view_t i_view_mode;
  
  /* we'll apply all page changes at once */
  if (page_number != -1)  return;

  ui_study = gtk_object_get_data(GTK_OBJECT(dialog), "ui_study"); 
  g_return_if_fail(ui_study != NULL);
  new_info = gtk_object_get_data(GTK_OBJECT(dialog),"new_info");
  g_return_if_fail(new_info != NULL);

  /* save the old center so that we can rotate around it */
  center = realspace_alt_coord_to_base(study_view_center(ui_study->study),
				       study_coord_frame(ui_study->study));

  /* copy the new info on over */
  study_set_name(ui_study->study, study_name(new_info));
  study_set_coord_frame(ui_study->study, study_coord_frame(new_info));

  /* copy the view information */
  study_set_view_center(ui_study->study, study_view_center(new_info));
  study_set_view_thickness(ui_study->study, study_view_thickness(new_info));
  study_set_view_time(ui_study->study, study_view_time(new_info));
  study_set_view_duration(ui_study->study, study_view_duration(new_info));
  study_set_zoom(ui_study->study, study_zoom(new_info));
  study_set_interpolation(ui_study->study, study_interpolation(new_info));


  /* apply any changes to the name of the study */
  for (i_view_mode=0; i_view_mode <= ui_study->view_mode; i_view_mode++)
    amitk_tree_update_object(AMITK_TREE(ui_study->tree[i_view_mode]), ui_study->study, STUDY);
  ui_study_update_title(ui_study);

  /* redraw the canvas if needed*/
  if (GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(dialog), "axis_changed"))) {
    gtk_object_set_data(GTK_OBJECT(dialog), "axis_changed", GINT_TO_POINTER(FALSE));

    /* recalculate the view center */
    study_set_view_center(ui_study->study,
			  realspace_base_coord_to_alt(center, study_coord_frame(ui_study->study)));

    for (i_view_mode=0; i_view_mode <= ui_study->view_mode; i_view_mode++)
      for (i_view=0; i_view<NUM_VIEWS; i_view++) {
	amitk_canvas_set_center(AMITK_CANVAS(ui_study->canvas[i_view_mode][i_view]), center, FALSE);
	amitk_canvas_set_coord_frame(AMITK_CANVAS(ui_study->canvas[i_view_mode][i_view]), 
				     study_coord_frame(ui_study->study), TRUE);
      }
  }
    
  return;
}



/* callback for the help button */
void ui_study_dialog_cb_help(GnomePropertyBox *study_dialog, gint page_number, gpointer data) {

  //  GnomeHelpMenuEntry help_ref={PACKAGE,"basics.html#ROI-DIALOG-HELP"};
  //  GnomeHelpMenuEntry help_ref_0 = {PACKAGE,"basics.html#ROI-DIALOG-HELP-BASIC"};
  //  GnomeHelpMenuEntry help_ref_1 = {PACKAGE,"basics.html#ROI-DIALOG-HELP-CENTER"};
  //  GnomeHelpMenuEntry help_ref_2 = {PACKAGE,"basics.html#ROI-DIALOG-HELP-DIMENSIONS"};
  //  GnomeHelpMenuEntry help_ref_3 = {PACKAGE,"basics.html#ROI-DIALOG-HELP-ROTATE"};


  //  switch (page_number) {
  //  case 0:
  //    gnome_help_display (0, &help_ref_0);
  //    break;
  //  case 1:
  //    gnome_help_display (0, &help_ref_1);
  //    break;
  //  case 2:
  //    gnome_help_display (0, &help_ref_2);
  //    break;
  //  case 3:
  //    gnome_help_display (0, &help_ref_3);
  //    break;
  //  default:
  //    gnome_help_display (0, &help_ref);
  //    break;
  //  }
  g_warning("sorry, no help here yet");
  
  return;
}

/* function called to destroy the study dialog */
gboolean ui_study_dialog_cb_close(GtkWidget* dialog, gpointer data) {

  ui_study_t * ui_study = data;
  study_t * new_info;
  GtkWidget * tree_item;

  /* trash collection */
  new_info = gtk_object_get_data(GTK_OBJECT(dialog), "new_info");
  g_return_val_if_fail(new_info != NULL, FALSE);
  new_info = study_unref(new_info);

  /* record that the dialog is no longer up */
  tree_item = amitk_tree_find_object(AMITK_TREE(ui_study->tree[SINGLE_VIEW]), ui_study->study,STUDY);
  g_return_val_if_fail(AMITK_IS_TREE_ITEM(tree_item), FALSE);
  AMITK_TREE_ITEM_DIALOG(tree_item) = NULL;

  return FALSE;
}


