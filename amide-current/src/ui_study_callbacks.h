/* ui_study_callbacks.h
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


/* external functions */
void ui_study_callbacks_save_as(GtkWidget * widget, gpointer data);
void ui_study_callbacks_import(GtkWidget * widget, gpointer data);
gint ui_study_callbacks_canvas_event(GtkWidget* widget, GdkEvent * event,
					 gpointer data);
void ui_study_callbacks_plane_change(GtkAdjustment * adjustment, gpointer data);
void ui_study_callbacks_axis_change(GtkAdjustment * adjustment, gpointer data);
void ui_study_callbacks_reset_axis(GtkWidget * button, gpointer data);
void ui_study_callbacks_zoom(GtkAdjustment * adjustment, gpointer data);
void ui_study_callbacks_time_pressed(GtkWidget * combo, gpointer data);
void ui_study_callbacks_thickness(GtkAdjustment * adjustment, gpointer data);
void ui_study_callbacks_transverse_series(GtkWidget * widget, gpointer data);
void ui_study_callbacks_coronal_series(GtkWidget * widget, gpointer data);
void ui_study_callbacks_sagittal_series(GtkWidget * widget, gpointer data);
#ifdef AMIDE_LIBVOLPACK_SUPPORT
void ui_study_callbacks_rendering(GtkWidget * widget, gpointer data);
#endif
void ui_study_callbacks_threshold_pressed(GtkWidget * button, gpointer data);
void ui_study_callbacks_scaling(GtkWidget * adjustment, gpointer data);
void ui_study_callbacks_color_table(GtkWidget * widget, gpointer data);
void ui_study_callbacks_roi_entry_name(gchar * entry_string, gpointer data);
void ui_study_callbacks_add_roi_type(GtkWidget * widget, gpointer data);
void ui_study_callback_tree_select_row(GtkCTree * ctree, GList * node, 
				       gint column, gpointer data);
void ui_study_callback_tree_unselect_row(GtkCTree * ctree, GList * node, 
					 gint column, gpointer data);
gboolean ui_study_callback_tree_click_row(GtkWidget *widget,
					  GdkEventButton *event,
					  gpointer data);
void ui_study_callbacks_edit_object_pressed(GtkWidget * button, gpointer data);
void ui_study_callbacks_delete_object_pressed(GtkWidget * button, gpointer data);
void ui_study_callbacks_interpolation(GtkWidget * widget, gpointer data);
void ui_study_callbacks_delete_event(GtkWidget* widget, GdkEvent * event, 
				     gpointer data);
void ui_study_callbacks_close_event(GtkWidget* widget, gpointer data);







