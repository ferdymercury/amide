/* ui_study_cb.h
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



/* external functions */
void ui_study_cb_new_study(GtkWidget * button, gpointer data);
void ui_study_cb_open_study(GtkWidget * button, gpointer data);
void ui_study_cb_save_as(GtkWidget * widget, gpointer data);
void ui_study_cb_import(GtkWidget * widget, gpointer data);
void ui_study_cb_export(GtkWidget * widget, gpointer data);
gboolean ui_study_cb_update_help_info(GtkWidget * widget, GdkEventCrossing * event, gpointer data);
void ui_study_cb_canvas_help_event(GtkWidget * canvas,  help_info_t help_type,
				   realpoint_t *location, gfloat value, gpointer ui_study);
void ui_study_cb_canvas_z_position_changed(GtkWidget * canvas, realpoint_t *position, gpointer ui_study);
void ui_study_cb_canvas_view_changing(GtkWidget * canvas, realpoint_t *position,
				      gfloat thickness, gpointer ui_study);
void ui_study_cb_canvas_view_changed(GtkWidget * canvas, realpoint_t *position,
				     gfloat thickness, gpointer ui_study);
void ui_study_cb_canvas_object_changed(GtkWidget * canvas, gpointer object, object_t type, 
				       gpointer ui_study);
void ui_study_cb_canvas_isocontour_3d_changed(GtkWidget * canvas, roi_t * roi, 
					      realpoint_t *position, gpointer ui_study);
void ui_study_cb_canvas_new_align_pt(GtkWidget * canvas, realpoint_t *position, gpointer ui_study);
void ui_study_cb_tree_select_object(GtkWidget * tree, gpointer object, object_t type, 
				    gpointer parent, object_t parent_type, gpointer ui_study);
void ui_study_cb_tree_unselect_object(GtkWidget * tree, gpointer object, object_t type, 
				      gpointer parent, object_t parent_type, gpointer ui_study);
void ui_study_cb_tree_make_active_object(GtkWidget * tree, gpointer object, object_t type, 
					 gpointer parent, object_t parent_type, gpointer ui_study);
void ui_study_cb_tree_popup_object(GtkWidget * tree, gpointer object, object_t type, 
				   gpointer parent, object_t parent_type, gpointer ui_study);
void ui_study_cb_tree_add_object(GtkWidget * tree, object_t type, gpointer parent, 
				 object_t parent_type, gpointer data);
void ui_study_cb_tree_help_event(GtkWidget * widget, help_info_t help_type, gpointer ui_study);
void ui_study_cb_zoom(GtkObject * adjustment, gpointer ui_study);
void ui_study_cb_thickness(GtkObject * adjustment, gpointer ui_study);
void ui_study_cb_time_pressed(GtkWidget * combo, gpointer data);
void ui_study_cb_series(GtkWidget * widget, gpointer ui_study_p);
#ifdef AMIDE_LIBVOLPACK_SUPPORT
void ui_study_cb_rendering(GtkWidget * widget, gpointer data);
#endif
void ui_study_cb_calculate_all(GtkWidget * widget, gpointer data);
void ui_study_cb_calculate_selected(GtkWidget * widget, gpointer data);
void ui_study_cb_alignment_selected(GtkWidget * widget, gpointer data);
void ui_study_cb_threshold_changed(GtkWidget * widget, gpointer data);
void ui_study_cb_color_changed(GtkWidget * widget, gpointer data);
void ui_study_cb_threshold_pressed(GtkWidget * button, gpointer data);
void ui_study_cb_add_roi(GtkWidget * widget, gpointer data);
void ui_study_cb_add_alignment_point(GtkWidget * widget, gpointer data);
void ui_study_cb_preferences(GtkWidget * widget, gpointer data);
gboolean ui_study_cb_tree_clicked(GtkWidget * leaf, GdkEventButton * event, gpointer data);
void ui_study_cb_edit_objects(GtkWidget * button, gpointer data);
void ui_study_cb_delete_objects(GtkWidget * button, gpointer data);
void ui_study_cb_interpolation(GtkWidget * widget, gpointer ui_study);
void ui_study_cb_view_mode(GtkWidget * widget, gpointer ui_study);
void ui_study_cb_exit(GtkWidget* widget, gpointer data);
void ui_study_cb_close(GtkWidget* widget, gpointer data);
gboolean ui_study_cb_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data);







