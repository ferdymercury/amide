/* ui_study_dialog_callbacks.c
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
#include "study.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_roi.h"
#include "ui_volume.h"
#include "ui_study.h"
#include "ui_study_dialog.h"
#include "ui_study_dialog_callbacks.h"

/* function called when the name of the study has been changed */
void ui_study_dialog_callbacks_change_name(GtkWidget * widget, gpointer data) {

  study_t * study_new_info = data;
  gchar * new_name;
  GtkWidget * study_dialog;

  /* get the contents of the name entry box and save it */
  new_name = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  study_set_name(study_new_info, new_name);
  g_free(new_name);

  /* tell the study_dialog that we've changed */
  study_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "study_dialog");
  gnome_property_box_changed(GNOME_PROPERTY_BOX(study_dialog));

  return;
}

/* function called when the filename of the study has been changed */
void ui_study_dialog_callbacks_change_filename(GtkWidget * widget, gpointer data) {

  study_t * study_new_info = data;
  gchar * new_name;
  GtkWidget * study_dialog;

  /* get the contents of the name entry box and save it */
  new_name = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  study_set_filename(study_new_info, new_name);
  g_free(new_name);

  /* tell the study_dialog that we've changed */
  study_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "study_dialog");
  gnome_property_box_changed(GNOME_PROPERTY_BOX(study_dialog));

  return;
}



/* function called when rotating the study around an axis */
void ui_study_dialog_callbacks_change_axis(GtkAdjustment * adjustment, gpointer data) {

  ui_study_t * ui_study;
  study_t * study_new_info = data;
  view_t i_view;
  axis_t which_axis;
  floatpoint_t rotation;
  GtkWidget * study_dialog;
  realpoint_t center, temp;

  /* now tell the study_dialog that we've changed */
  study_dialog =  gtk_object_get_data(GTK_OBJECT(adjustment), "study_dialog");
  /* we need the current view_axis so that we know what we're rotating around */
  ui_study = gtk_object_get_data(GTK_OBJECT(study_dialog), "ui_study"); 

  /* saving the center, as we're rotating the study around it's own center */
  center = realspace_alt_coord_to_base(study_view_center(ui_study->study),
				       study_new_info->coord_frame);

  /* figure out which scale widget called me */
  i_view= GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(adjustment),"view"));

  rotation = (adjustment->value/180)*M_PI; /* get rotation in radians */
  which_axis = realspace_get_orthogonal_which_axis(i_view);
  study_set_coord_frame_axis(study_new_info, XAXIS,
			     realspace_rotate_on_axis(study_coord_frame_axis(study_new_info, XAXIS),
						      study_coord_frame_axis(study_new_info, which_axis),
						      rotation));
  study_set_coord_frame_axis(study_new_info, YAXIS,
			     realspace_rotate_on_axis(study_coord_frame_axis(study_new_info, YAXIS),
						      study_coord_frame_axis(study_new_info, which_axis),
						      rotation));
  study_set_coord_frame_axis(study_new_info, ZAXIS,
			     realspace_rotate_on_axis(study_coord_frame_axis(study_new_info, ZAXIS),
						      study_coord_frame_axis(study_new_info, which_axis),
						      rotation));
  realspace_make_orthonormal(study_new_info->coord_frame.axis); /* orthonormalize*/
  
  /* recalculate the offset of this study based on the center we stored */
  study_set_coord_frame_offset(study_new_info, center);
  temp = realspace_alt_coord_to_base(study_view_center(ui_study->study), study_new_info->coord_frame);
  study_set_coord_frame_offset(study_new_info, temp);
			       

  /* return adjustment back to normal */
  adjustment->value = 0.0;
  gtk_adjustment_changed(adjustment);


  /* now tell the study_dialog that we've changed */
  study_dialog =  gtk_object_get_data(GTK_OBJECT(adjustment), "study_dialog");
  ui_study_dialog_set_axis_display(study_dialog);
  gtk_object_set_data(GTK_OBJECT(study_dialog), "axis_changed", GINT_TO_POINTER(TRUE));
  gnome_property_box_changed(GNOME_PROPERTY_BOX(study_dialog));

  return;
}

/* function to reset the study's axis back to the default coords */
void ui_study_dialog_callbacks_reset_axis(GtkWidget* widget, gpointer data) {

  ui_study_t * ui_study;
  study_t * study_new_info = data;
  axis_t i_axis;
  GtkWidget * study_dialog;
  realpoint_t center, temp;

  /* now tell the study_dialog that we've changed */
  study_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "study_dialog");
  /* we need the current view_axis so that we know what we're rotating around */
  ui_study = gtk_object_get_data(GTK_OBJECT(study_dialog), "ui_study"); 
  center = study_view_center(ui_study->study);

  /* reset the axis */
  for (i_axis=0;i_axis<NUM_AXIS;i_axis++) {
    study_new_info->coord_frame.axis[i_axis] = default_axis[i_axis];
  }

  /* recalculate the offset of this study based on the center we stored */
  study_set_coord_frame_offset(study_new_info, center);
  temp = realspace_alt_coord_to_base(study_view_center(ui_study->study), study_new_info->coord_frame);
  study_set_coord_frame_offset(study_new_info, temp);

  ui_study_dialog_set_axis_display(study_dialog);
  gtk_object_set_data(GTK_OBJECT(study_dialog), "axis_changed", GINT_TO_POINTER(TRUE));
  gnome_property_box_changed(GNOME_PROPERTY_BOX(study_dialog));
  
  return;
}





/* function called when we hit the apply button */
void ui_study_dialog_callbacks_apply(GtkWidget* widget, gint page_number, gpointer data) {
  
  ui_study_t * ui_study = data;
  GdkPixmap * pixmap;
  guint8 spacing;
  study_t * study_new_info;
  realpoint_t center;
  
  /* we'll apply all page changes at once */
  if (page_number != -1)
    return;

  /* get the new info for the study */
  study_new_info = gtk_object_get_data(GTK_OBJECT(ui_study->study_dialog),"study_new_info");

  /* save the old center so that we can rotate around it */
  center = realspace_alt_coord_to_base(study_view_center(ui_study->study),
				       study_coord_frame(ui_study->study));

  /* sanity check */
  if (study_new_info == NULL) {
    g_printerr("%s: study_new_info inappropriately null....\n", PACKAGE);
    return;
  }

  /* copy the new info on over */
  study_set_name(ui_study->study, study_name(study_new_info));
  study_set_filename(ui_study->study, study_filename(study_new_info));
  study_set_coord_frame(ui_study->study, study_coord_frame(study_new_info));

  /* copy the view information */
  study_set_view_center(ui_study->study, study_view_center(study_new_info));
  study_set_view_thickness(ui_study->study, study_view_thickness(study_new_info));
  study_set_view_time(ui_study->study, study_view_time(study_new_info));
  study_set_view_duration(ui_study->study, study_view_duration(study_new_info));
  study_set_zoom(ui_study->study, study_zoom(study_new_info));
  study_set_interpolation(ui_study->study, study_interpolation(study_new_info));
  study_set_scaling(ui_study->study, study_scaling(study_new_info));


  /* apply any changes to the name of the widget */
  /* get the current pixmap and spacing in the line of the tree corresponding to the study */
  gtk_ctree_node_get_pixtext(GTK_CTREE(ui_study->tree), ui_study->tree_study, 
			     UI_STUDY_TREE_TEXT_COLUMN, NULL, &spacing, &pixmap, NULL);

  /* reset the text in that tree line */
  gtk_ctree_node_set_pixtext(GTK_CTREE(ui_study->tree), ui_study->tree_study, 
			     UI_STUDY_TREE_TEXT_COLUMN, study_name(ui_study->study), spacing, pixmap, NULL);


  /* redraw the canvas if needed*/
  if (GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(ui_study->study_dialog), "axis_changed"))) {
    gtk_object_set_data(GTK_OBJECT(ui_study->study_dialog), "axis_changed", GINT_TO_POINTER(FALSE));

    /* recalculate the view center */
    study_set_view_center(ui_study->study,
			  realspace_base_coord_to_alt(center, study_coord_frame(ui_study->study)));

    ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_ALL);
  }

  return;
}



/* callback for the help button */
void ui_study_dialog_callbacks_help(GnomePropertyBox *study_dialog, gint page_number, gpointer data) {

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
  g_warning("%s: sorry, no help here yet", PACKAGE);
  
  return;
}

/* function called to destroy the study dialog */
void ui_study_dialog_callbacks_close_event(GtkWidget* widget, gpointer data) {

  ui_study_t * ui_study = data;
  study_t * study_new_info;

  /* trash collection */
  study_new_info = gtk_object_get_data(GTK_OBJECT(widget), "study_new_info");
  study_new_info = study_free(study_new_info);

  /* destroy the widget */
  gtk_widget_destroy(GTK_WIDGET(ui_study->study_dialog));

  /* make sure the pointer in the roi_list_item is nulled */
  ui_study->study_dialog = NULL;

  return;
}


