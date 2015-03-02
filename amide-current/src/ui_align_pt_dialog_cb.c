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

/* function called when the name of the alignment point has been changed */
void ui_align_pt_dialog_cb_change_name(GtkWidget * widget, gpointer data) {

  align_pt_t * align_pt_new_info = data;
  gchar * new_name;
  GtkWidget * dialog;

  /* get the contents of the name entry box and save it */
  new_name = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  align_pt_set_name(align_pt_new_info, new_name);
  g_free(new_name);

  /* tell the_dialog that we've changed */
  dialog =  gtk_object_get_data(GTK_OBJECT(widget), "dialog");
  gnome_property_box_changed(GNOME_PROPERTY_BOX(dialog));

  return;
}

/* function called when a numerical entry of the alignment point has been changed */
void ui_align_pt_dialog_cb_change_entry(GtkWidget * widget, gpointer data) {

  align_pt_t * align_pt_new_info = data;
  volume_t * volume;
  ui_study_t * ui_study;
  gchar * str;
  gint error;
  gdouble temp_val;
  which_entry_widget_t which_widget;
  realpoint_t temp_center;
  GtkWidget * dialog;


  /* figure out which widget this is */
  which_widget = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "type")); 

  /* get a pointer to the volume */
  dialog =  gtk_object_get_data(GTK_OBJECT(widget), "dialog");
  volume = gtk_object_get_data(GTK_OBJECT(dialog), "volume");
  ui_study = gtk_object_get_data(GTK_OBJECT(dialog), "ui_study"); 

  temp_center = realspace_alt_coord_to_alt(align_pt_new_info->point, 
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

  align_pt_new_info->point = realspace_alt_coord_to_alt(temp_center, 
							study_coord_frame(ui_study->study),
							volume->coord_frame);

  /* now tell the dialog that we've changed */
  gnome_property_box_changed(GNOME_PROPERTY_BOX(dialog));

  return;
}




/* function called when we hit the apply button */
void ui_align_pt_dialog_cb_apply(GtkWidget* widget, gint page_number, gpointer data) {
  
  ui_align_pts_t * ui_align_pts_item = data;
  ui_study_t * ui_study;
  align_pt_t * align_pt_new_info;
  volume_t * volume;
  view_t i_views;
  GtkWidget * label;
  
  /* we'll apply all page changes at once */
  if (page_number != -1)
    return;

  /* get the new info for the alignment point */
  align_pt_new_info = gtk_object_get_data(GTK_OBJECT(ui_align_pts_item->dialog),"new_info");

  /* sanity check */
  if (align_pt_new_info == NULL) {
    g_warning("%s: align_pt_new_info inappropriately null....", PACKAGE);
    return;
  }

  /* copy the new info on over */
  align_pt_set_name(ui_align_pts_item->align_pt, align_pt_new_info->name);
  ui_align_pts_item->align_pt->point = align_pt_new_info->point;

  /* apply any changes to the name of the alignment point */
  label = gtk_object_get_data(GTK_OBJECT(ui_align_pts_item->tree_leaf), "text_label");
  gtk_label_set_text(GTK_LABEL(label), ui_align_pts_item->align_pt->name);

  /* get a pointer to ui_study so we can redraw the alignment points's */
  ui_study = gtk_object_get_data(GTK_OBJECT(ui_align_pts_item->dialog), "ui_study"); 
  volume = gtk_object_get_data(GTK_OBJECT(ui_align_pts_item->dialog), "volume");

  /* redraw the alignment point */
  for (i_views=0;i_views<NUM_VIEWS;i_views++)
    ui_align_pts_item->canvas_pt[i_views] =
      ui_study_update_canvas_align_pt(ui_study,i_views,
				      ui_align_pts_item->canvas_pt[i_views],
				      ui_align_pts_item->align_pt,
				      volume);

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
gboolean ui_align_pt_dialog_cb_close(GtkWidget* widget, gpointer data) {

  ui_align_pts_t * ui_align_pts_item = data;
  align_pt_t * align_pt_new_info;

  /* trash collection */
  align_pt_new_info = gtk_object_get_data(GTK_OBJECT(widget), "new_info");
#if AMIDE_DEBUG
  { /* little something to make the debugging messages make sense */
    gchar * temp_string;
    temp_string = g_strdup_printf("Copy of %s",align_pt_new_info->name);
    align_pt_set_name(align_pt_new_info, temp_string);
    g_free(temp_string);

  }
#endif
  align_pt_new_info = align_pt_free(align_pt_new_info);

  /* make sure the pointer in the align_pts_item is nulled */
  ui_align_pts_item->dialog = NULL;

  return FALSE;
}


