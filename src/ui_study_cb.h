/* ui_study_cb.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2017 Andy Loening
 *
 * Author: Andy Loening <loening@alum.mit.edu>
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
void ui_study_cb_open_xif_file(GSimpleAction * action, GVariant * param, gpointer ui_study);
void ui_study_cb_open_xif_dir(GSimpleAction * action, GVariant * param, gpointer ui_study);
void ui_study_cb_import_object_from_xif_file(GSimpleAction * action, GVariant * param, gpointer data);
void ui_study_cb_import_object_from_xif_dir(GSimpleAction * action, GVariant * param, gpointer data);
void ui_study_cb_recover_xif_file(GSimpleAction * action, GVariant * param, gpointer data);
void ui_study_cb_new_study(GSimpleAction * action, GVariant * param, gpointer ui_study);
void ui_study_cb_save_as_xif_file(GSimpleAction * action, GVariant * param, gpointer data);
void ui_study_cb_save_as_xif_dir(GSimpleAction * action, GVariant * param, gpointer data);
void ui_study_cb_import(GSimpleAction * action, GVariant * param, gpointer data);
void ui_study_cb_export_view(GSimpleAction * action, GVariant * param, gpointer data);
void ui_study_cb_export_data_set(GSimpleAction * action, GVariant * param, gpointer data);
gboolean ui_study_cb_update_help_info(GtkWidget * widget, GdkEventCrossing * event, gpointer data);
void ui_study_cb_canvas_help_event(GtkWidget * canvas,  AmitkHelpInfo help_type,
				   AmitkPoint *location, amide_data_t value, gpointer ui_study);
void ui_study_cb_canvas_view_changing(GtkWidget * canvas, AmitkPoint *position,
				      amide_real_t thickness, gpointer ui_study);
void ui_study_cb_canvas_view_changed(GtkWidget * canvas, AmitkPoint *position,
				     amide_real_t thickness, gpointer ui_study);
void ui_study_cb_canvas_erase_volume(GtkWidget * canvas, AmitkRoi * roi, 
				     gboolean outside, gpointer ui_study);
void ui_study_cb_canvas_new_object(GtkWidget * canvas, AmitkObject * parent, AmitkObjectType type, 
				   AmitkPoint *position, gpointer ui_study);
void ui_study_cb_tree_view_activate_object(GtkWidget * tree_view, 
					   AmitkObject * object, 
					   gpointer ui_study);
void ui_study_cb_tree_view_popup_object(GtkWidget * tree_view, AmitkObject * object, gpointer ui_study);
void ui_study_cb_tree_view_add_object(GtkWidget * tree_view, AmitkObject * parent, 
				      AmitkObjectType object_type, AmitkRoiType roi_type, gpointer ui_study);
void ui_study_cb_tree_view_delete_object(GtkWidget * tree_view,AmitkObject * object, gpointer ui_study);
void ui_study_cb_tree_view_help_event(GtkWidget * widget, AmitkHelpInfo help_type, gpointer ui_study);
void ui_study_cb_zoom(GtkSpinButton * spin_button, gpointer ui_study);
void ui_study_cb_fov(GtkSpinButton * spin_button, gpointer ui_study);
void ui_study_cb_thickness(GtkSpinButton * spin_button, gpointer ui_study);
void ui_study_cb_gate(GtkWidget * button, gpointer data);
void ui_study_cb_time(GtkWidget * button, gpointer data);
void ui_study_cb_series(GSimpleAction * action, GVariant * param, gpointer ui_study);
#if (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)
void ui_study_cb_fly_through(GSimpleAction * action, GVariant * param, gpointer ui_study);
#endif
#ifdef AMIDE_LIBVOLPACK_SUPPORT
void ui_study_cb_render(GSimpleAction * action, GVariant * param, gpointer data);
#endif
void ui_study_cb_roi_statistics(GSimpleAction * action, GVariant * param, gpointer data);
void ui_study_cb_alignment_selected(GSimpleAction * action, GVariant * param, gpointer data);
void ui_study_cb_crop_selected(GSimpleAction * action, GVariant * param, gpointer data);
void ui_study_cb_distance_selected(GSimpleAction * action, GVariant * param, gpointer data);
void ui_study_cb_fads_selected(GSimpleAction * action, GVariant * param, gpointer data);
void ui_study_cb_filter_selected(GSimpleAction * action, GVariant * param, gpointer data);
void ui_study_cb_profile_selected(GSimpleAction * action, GVariant * param, gpointer data);
void ui_study_cb_data_set_math_selected(GSimpleAction * action, GVariant * param, gpointer data);
void ui_study_cb_canvas_target(GSimpleAction * action, GVariant * state, gpointer data);
void ui_study_cb_thresholding(GSimpleAction * action, GVariant * param, gpointer data);
void ui_study_cb_add_roi(GSimpleAction * widget, GVariant * param, gpointer data);
void ui_study_cb_add_fiducial_mark(GSimpleAction * action, GVariant * param, gpointer data);
void ui_study_cb_preferences(GSimpleAction * action, GVariant * param, gpointer data);
void ui_study_cb_interpolation(GSimpleAction * action, GVariant * state, gpointer data);
void ui_study_cb_rendering(GtkWidget * widget, gpointer data);
void ui_study_cb_study_changed(AmitkStudy * study, gpointer ui_study);
void ui_study_cb_thickness_changed(AmitkStudy * study, gpointer data);
void ui_study_cb_canvas_layout_changed(AmitkStudy * study, gpointer ui_study);
void ui_study_cb_voxel_dim_or_zoom_changed(AmitkStudy * study, gpointer ui_study);
void ui_study_cb_fov_changed(AmitkStudy * study, gpointer ui_study);
void ui_study_cb_fuse_type(GSimpleAction * action, GVariant * state, gpointer data);
void ui_study_cb_canvas_visible(GSimpleAction * action, GVariant * state, gpointer ui_study);
void ui_study_cb_view_mode(GSimpleAction * action, GVariant * state, gpointer data);
void ui_study_cb_quit(GSimpleAction* action, GVariant * param, gpointer data);
void ui_study_cb_close(GSimpleAction* action, GVariant * parram, gpointer data);
gboolean ui_study_cb_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data);







