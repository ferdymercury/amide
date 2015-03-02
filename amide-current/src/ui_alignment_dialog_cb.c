/* ui_alignment_dialog_cb.c
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
#include "alignment.h"
#include "ui_study.h"
#include "ui_alignment_dialog.h"
#include "ui_alignment_dialog_cb.h"
#include "amitk_canvas.h"
#include "amitk_tree.h"


gboolean ui_alignment_dialog_cb_next_page(GtkWidget * page, gpointer *druid, gpointer data) {

  ui_alignment_t * ui_alignment = data;
  which_page_t which_page;
  gboolean nonlinear = FALSE;
  gint num_pairs;

  which_page = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(page), "which_page"));

  switch(which_page) {
  case VOLUME_PAGE:
    num_pairs = align_pts_count_pairs_by_name(ui_alignment->volume_fixed->align_pts,
					      ui_alignment->volume_moving->align_pts);
    if (num_pairs < 4) {
      nonlinear = TRUE;
      gnome_druid_set_page(GNOME_DRUID(druid), 
			   GNOME_DRUID_PAGE(ui_alignment->pages[NO_ALIGN_PTS_PAGE]));
    }
    break;
  default:
    g_warning("%s: unexpected case in %s at line %d", PACKAGE, __FILE__, __LINE__);
    break;
  }

  return nonlinear;
}

gboolean ui_alignment_dialog_cb_back_page(GtkWidget * page, gpointer *druid, gpointer data) {

  ui_alignment_t * ui_alignment = data;
  which_page_t which_page;
  gboolean nonlinear = FALSE;

  which_page = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(page), "which_page"));

  switch(which_page) {
  case NO_ALIGN_PTS_PAGE:
    nonlinear = TRUE;
    gnome_druid_set_page(GNOME_DRUID(druid), 
			 GNOME_DRUID_PAGE(ui_alignment->pages[VOLUME_PAGE]));
    break;
  default:
    g_warning("%s: unexpected case in %s at line %d", PACKAGE, __FILE__, __LINE__);
    break;
  }

  return nonlinear;
}

void ui_alignment_dialog_cb_prepare_page(GtkWidget * page, gpointer * druid, gpointer data) {
 
  ui_alignment_t * ui_alignment = data;
  which_page_t which_page;
  gboolean can_continue;
  align_pts_t * align_pts;
  align_pts_t * selected_pts;
  gchar * temp_strings[1];
  guint count = 0;

  which_page = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(page), "which_page"));

  switch(which_page) {
  case VOLUME_PAGE:
    if (ui_alignment->volume_moving == NULL)
      gtk_clist_select_row(GTK_CLIST(ui_alignment->clist_volume_moving), 0, 0);
    if (ui_alignment->volume_fixed == NULL)
      gtk_clist_select_row(GTK_CLIST(ui_alignment->clist_volume_fixed), 1, 0);
    can_continue = ((ui_alignment->volume_fixed != NULL) &&
		    (ui_alignment->volume_moving != NULL) &&
		    (ui_alignment->volume_fixed != ui_alignment->volume_moving));
    gnome_druid_set_buttons_sensitive(GNOME_DRUID(ui_alignment->druid), TRUE, can_continue, TRUE);
    break;
  case ALIGN_PTS_PAGE:
    gtk_clist_clear(GTK_CLIST(ui_alignment->clist_points)); /* delete what was there before */
    gnome_druid_set_buttons_sensitive(GNOME_DRUID(ui_alignment->druid), TRUE, FALSE, TRUE);

    /* put in new pairs of points */
    align_pts = ui_alignment->volume_fixed->align_pts;
    selected_pts = ui_alignment->align_pts;
    ui_alignment->align_pts = NULL;
    while (align_pts != NULL) {
      if (align_pts_includes_pt_by_name(ui_alignment->volume_moving->align_pts, 
					align_pts->align_pt->name)) {
	temp_strings[0] = g_strdup_printf("%s", align_pts->align_pt->name);
	gtk_clist_append(GTK_CLIST(ui_alignment->clist_points), temp_strings);
	g_free(temp_strings[0]);
	gtk_clist_set_row_data(GTK_CLIST(ui_alignment->clist_points), count, align_pts->align_pt);
	if ((align_pts_includes_pt_by_name(selected_pts, align_pts->align_pt->name)) ||
	    (selected_pts == NULL))
	  gtk_clist_select_row(GTK_CLIST(ui_alignment->clist_points), count, 0);
	count ++;
      }
      align_pts = align_pts->next;
    }
    selected_pts = align_pts_unref(selected_pts);

    break;
  case CONCLUSION_PAGE:
#ifdef AMIDE_LIBGSL_SUPPORT
    /* calculate the alignment */
    ui_alignment->coord_frame = alignment_calculate(ui_alignment->volume_moving, 
						    ui_alignment->volume_fixed, 
						    ui_alignment->align_pts);
#endif
    gnome_druid_set_show_finish(GNOME_DRUID(druid), TRUE);
    break;
  case NO_ALIGN_PTS_PAGE:
    gnome_druid_set_show_finish(GNOME_DRUID(druid), FALSE);
    break;
  default:
    g_warning("%s: unexpected case in %s at line %d", PACKAGE, __FILE__, __LINE__);
    break;
  }

  return;
}

void ui_alignment_dialog_cb_select_volume(GtkCList * clist, gint row, gint column,
					  GdkEventButton * event, gpointer data) {

  ui_alignment_t * ui_alignment = data;
  volume_t * volume;
  gboolean fixed_volume;
  gboolean can_continue;
  
  fixed_volume = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(clist), "fixed"));

  volume = gtk_clist_get_row_data(GTK_CLIST(clist), row);
  g_return_if_fail(volume != NULL);

  if (fixed_volume) 
    ui_alignment->volume_fixed = volume_ref(volume);
  else 
    ui_alignment->volume_moving = volume_ref(volume);
  

  can_continue = ((ui_alignment->volume_fixed != NULL) &&
		  (ui_alignment->volume_moving != NULL) &&
		  (ui_alignment->volume_fixed != ui_alignment->volume_moving));
  gnome_druid_set_buttons_sensitive(GNOME_DRUID(ui_alignment->druid), TRUE, can_continue, TRUE);

  return;
}

void ui_alignment_dialog_cb_unselect_volume(GtkCList * clist, gint row, gint column,
					    GdkEventButton * event, gpointer data) {

  ui_alignment_t * ui_alignment = data;
  volume_t * volume;
  gboolean fixed_volume;
  
  fixed_volume = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(clist), "fixed"));

  volume = gtk_clist_get_row_data(GTK_CLIST(clist), row);
  g_return_if_fail(volume != NULL);

  if (fixed_volume)
    ui_alignment->volume_fixed = volume_unref(ui_alignment->volume_fixed);
  else 
    ui_alignment->volume_moving = volume_unref(ui_alignment->volume_moving);

  gnome_druid_set_buttons_sensitive(GNOME_DRUID(ui_alignment->druid), TRUE, FALSE, TRUE);

  return;
}

void ui_alignment_dialog_cb_select_point(GtkCList * clist, gint row, gint column,
					 GdkEventButton * event, gpointer data) {

  ui_alignment_t * ui_alignment = data;
  align_pt_t * align_pt;
  guint count;
  
  align_pt = gtk_clist_get_row_data(GTK_CLIST(clist), row);
  g_return_if_fail(align_pt != NULL);

  ui_alignment->align_pts = align_pts_add_pt(ui_alignment->align_pts, align_pt);

  count = align_pts_count(ui_alignment->align_pts);

  gnome_druid_set_buttons_sensitive(GNOME_DRUID(ui_alignment->druid), TRUE, 
				    (count >= 4), TRUE);

  return;
}

void ui_alignment_dialog_cb_unselect_point(GtkCList * clist, gint row, gint column,
					   GdkEventButton * event, gpointer data) {

  ui_alignment_t * ui_alignment = data;
  align_pt_t * align_pt;
  guint count;
  
  align_pt = gtk_clist_get_row_data(GTK_CLIST(clist), row);
  g_return_if_fail(align_pt != NULL);

  ui_alignment->align_pts = align_pts_remove_pt(ui_alignment->align_pts, align_pt);

  count = align_pts_count(ui_alignment->align_pts);

  gnome_druid_set_buttons_sensitive(GNOME_DRUID(ui_alignment->druid), TRUE, 
				    (count >= 4), TRUE);

  return;
}

/* function called when the finish button is hit */
void ui_alignment_dialog_cb_finish(GtkWidget* widget, gpointer druid, gpointer data) {
  ui_alignment_t * ui_alignment = data;
  ui_study_t * ui_study;
  view_t i_view;

  /* sanity check */
  g_return_if_fail(ui_alignment->coord_frame != NULL);

  /* apply the alignment transform */
  rs_set_coord_frame(ui_alignment->volume_moving->coord_frame,ui_alignment->coord_frame);

  /* update the display */
  ui_study = gtk_object_get_data(GTK_OBJECT(druid), "ui_study");
  if (volumes_includes_volume(AMITK_TREE_SELECTED_VOLUMES(ui_study->tree), ui_alignment->volume_moving)) {
    for (i_view=0; i_view<NUM_VIEWS; i_view++)
      amitk_canvas_update_volumes(AMITK_CANVAS(ui_study->canvas[i_view]));
  }

  /* close the dialog box */
  ui_alignment_dialog_cb_cancel(widget, data);

  return;
}

/* function called to cancel the dialog */
void ui_alignment_dialog_cb_cancel(GtkWidget* widget, gpointer data) {
  ui_alignment_t * ui_alignment = data;
  GtkWidget * dialog = ui_alignment->dialog;

  ui_alignment_dialog_cb_delete_event(dialog, NULL, ui_alignment);
  gtk_widget_destroy(dialog);

  return;
}

/* function called on a delete event */
gboolean ui_alignment_dialog_cb_delete_event(GtkWidget * widget, GdkEvent * event, gpointer data) {

  ui_alignment_t * ui_alignment = data;

  /* trash collection */
  ui_alignment = ui_alignment_free(ui_alignment);

  return FALSE;
}


