/* ui_study_cb.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2014 Andy Loening
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
void ui_study_cb_open_xif_file(GtkAction * action, gpointer ui_study);
void ui_study_cb_open_xif_dir(GtkAction * action, gpointer ui_study);
void ui_study_cb_import_object_from_xif_file(GtkAction * action, gpointer data);
void ui_study_cb_import_object_from_xif_dir(GtkAction * action, gpointer data);
void ui_study_cb_recover_xif_file(GtkAction * action, gpointer data);
void ui_study_cb_new_study(GtkAction * action, gpointer ui_study);
void ui_study_cb_save_as_xif_file(GtkAction * action, gpointer data);
void ui_study_cb_save_as_xif_dir(GtkAction * action, gpointer data);
void ui_study_cb_import(GtkAction * action, gpointer data);
void ui_study_cb_export_view(GtkAction * action, gpointer data);
void ui_study_cb_export_data_set(GtkAction * action, gpointer data);
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
void ui_study_cb_series(GtkAction * action, gpointer ui_study);
#if (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)
void ui_study_cb_fly_through(GtkAction * action, gpointer ui_study);
#endif
#ifdef AMIDE_LIBVOLPACK_SUPPORT
void ui_study_cb_render(GtkAction * action, gpointer data);
#endif
void ui_study_cb_roi_statistics(GtkAction * action, gpointer data);
void ui_study_cb_alignment_selected(GtkAction * action, gpointer data);
void ui_study_cb_crop_selected(GtkAction * action, gpointer data);
void ui_study_cb_distance_selected(GtkAction * action, gpointer data);
void ui_study_cb_fads_selected(GtkAction * action, gpointer data);
void ui_study_cb_filter_selected(GtkAction * action, gpointer data);
void ui_study_cb_profile_selected(GtkAction * action, gpointer data);
void ui_study_cb_data_set_math_selected(GtkAction * action, gpointer data);
void ui_study_cb_canvas_target(GtkToggleAction * action, gpointer data);
void ui_study_cb_thresholding(GtkAction * action, gpointer data);
void ui_study_cb_add_roi(GtkWidget * widget, gpointer data);
void ui_study_cb_add_fiducial_mark(GtkAction * action, gpointer data);
void ui_study_cb_preferences(GtkAction * action, gpointer data);
void ui_study_cb_interpolation(GtkRadioAction * action, GtkRadioAction * current, gpointer data);
void ui_study_cb_rendering(GtkWidget * widget, gpointer data);
void ui_study_cb_study_changed(AmitkStudy * study, gpointer ui_study);
void ui_study_cb_thickness_changed(AmitkStudy * study, gpointer data);
void ui_study_cb_canvas_layout_changed(AmitkStudy * study, gpointer ui_study);
void ui_study_cb_voxel_dim_or_zoom_changed(AmitkStudy * study, gpointer ui_study);
void ui_study_cb_fov_changed(AmitkStudy * study, gpointer ui_study);
void ui_study_cb_fuse_type(GtkRadioAction * action, GtkRadioAction * current, gpointer data);
void ui_study_cb_canvas_visible(GtkToggleAction * action, gpointer ui_study);
void ui_study_cb_view_mode(GtkRadioAction * action, GtkRadioAction * current, gpointer data);
void ui_study_cb_quit(GtkAction* action, gpointer data);
void ui_study_cb_close(GtkAction* action, gpointer data);
gboolean ui_study_cb_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data);







