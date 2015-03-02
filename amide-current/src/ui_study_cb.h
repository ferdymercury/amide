/* ui_study_cb.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2003 Andy Loening
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
void ui_study_cb_canvas_help_event(GtkWidget * canvas,  AmitkHelpInfo help_type,
				   AmitkPoint *location, amide_data_t value, gpointer ui_study);
void ui_study_cb_canvas_view_changing(GtkWidget * canvas, AmitkPoint *position,
				      amide_real_t thickness, gpointer ui_study);
void ui_study_cb_canvas_view_changed(GtkWidget * canvas, AmitkPoint *position,
				     amide_real_t thickness, gpointer ui_study);
void ui_study_cb_canvas_isocontour_3d_changed(GtkWidget * canvas, AmitkRoi * roi,
					      AmitkPoint *position, gpointer ui_study);
void ui_study_cb_canvas_erase_volume(GtkWidget * canvas, AmitkRoi * roi, 
				     gboolean outside, gpointer ui_study);
void ui_study_cb_canvas_new_object(GtkWidget * canvas, AmitkObject * parent, AmitkObjectType type, 
				   AmitkPoint *position, gpointer ui_study);
void ui_study_cb_tree_activate_object(GtkWidget * tree, 
				      AmitkObject * object, 
				      gpointer ui_study);
void ui_study_cb_tree_popup_object(GtkWidget * tree, AmitkObject * object, gpointer ui_study);
void ui_study_cb_tree_add_object(GtkWidget * tree, AmitkObject * parent, 
				 AmitkObjectType object_type, AmitkRoiType roi_type, gpointer ui_study);
void ui_study_cb_tree_delete_object(GtkWidget * tree,AmitkObject * object, gpointer ui_study);
void ui_study_cb_tree_help_event(GtkWidget * widget, AmitkHelpInfo help_type, gpointer ui_study);
void ui_study_cb_zoom(GtkSpinButton * spin_button, gpointer ui_study);
void ui_study_cb_thickness(GtkSpinButton * spin_button, gpointer ui_study);
void ui_study_cb_time_pressed(GtkWidget * combo, gpointer data);
void ui_study_cb_series(GtkWidget * widget, gpointer ui_study);
#ifdef AMIDE_LIBFAME_SUPPORT
void ui_study_cb_fly_through(GtkWidget * widget, gpointer ui_study);
#endif
#ifdef AMIDE_LIBVOLPACK_SUPPORT
void ui_study_cb_render(GtkWidget * widget, gpointer data);
#endif
void ui_study_cb_roi_statistics(GtkWidget * widget, gpointer data);
void ui_study_cb_alignment_selected(GtkWidget * widget, gpointer data);
void ui_study_cb_crop_selected(GtkWidget * widget, gpointer data);
void ui_study_cb_fads_selected(GtkWidget * widget, gpointer data);
void ui_study_cb_filter_selected(GtkWidget * widget, gpointer data);
void ui_study_cb_threshold_pressed(GtkWidget * button, gpointer data);
void ui_study_cb_add_roi(GtkWidget * widget, gpointer data);
void ui_study_cb_add_fiducial_mark(GtkWidget * widget, gpointer data);
void ui_study_cb_preferences(GtkWidget * widget, gpointer data);
void ui_study_cb_interpolation(GtkWidget * widget, gpointer ui_study);
void ui_study_cb_study_changed(AmitkStudy * study, gpointer ui_study);
void ui_study_cb_study_view_mode_changed(AmitkStudy * study, gpointer ui_study);
void ui_study_cb_fuse_type(GtkWidget * widget, gpointer ui_study);
void ui_study_cb_canvas_visible(GtkWidget * widget, gpointer ui_study);
void ui_study_cb_view_mode(GtkWidget * widget, gpointer ui_study);
void ui_study_cb_exit(GtkWidget* widget, gpointer data);
void ui_study_cb_close(GtkWidget* widget, gpointer data);
gboolean ui_study_cb_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data);







