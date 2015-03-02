/* ui_study_toolbar.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2002 Andy Loening
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




#include "amide_config.h"
#include <gnome.h>
#include "ui_study.h"
#include "ui_study_cb.h"
#include "ui_study_menus.h"
#include "ui_study_toolbar.h"
#include "pixmaps.h"
#include "amide_limits.h"


/* function to setup the toolbar for the study ui */
/* takes a separate study parameter, as the ui_study->study variable 
   isn't usually set when these function is called */
void ui_study_toolbar_create(ui_study_t * ui_study, AmitkStudy * study) {

  AmitkInterpolation i_interpolation;
  AmitkFuseType i_fuse_type;
  AmitkViewMode i_view_mode;
  GtkWidget * label;
  GtkWidget * toolbar;
  GtkObject * adjustment;

  /* the toolbar definitions */
  GnomeUIInfo interpolation_list[AMITK_INTERPOLATION_NUM+1];
  GnomeUIInfo fuse_type_list[AMITK_FUSE_TYPE_NUM+1];
  GnomeUIInfo view_mode_list[AMITK_VIEW_MODE_NUM+1];

  GnomeUIInfo study_main_toolbar[] = {
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_RADIOLIST(interpolation_list),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_RADIOLIST(fuse_type_list),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_RADIOLIST(view_mode_list),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_ITEM_DATA(NULL,
			  N_("Set the thresholds and colormaps for the active data set"),
			  ui_study_cb_threshold_pressed,
			  ui_study, icon_threshold_xpm),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_END
  };

  /* sanity check */
  g_assert(ui_study!=NULL);


  
  /* start make the interpolation toolbar items*/
  for (i_interpolation = 0; i_interpolation < AMITK_INTERPOLATION_NUM; i_interpolation++) 
    ui_study_menus_fill_in_radioitem(&(interpolation_list[i_interpolation]),
				     (icon_interpolation[i_interpolation] == NULL) ? 
				     amitk_interpolation_get_name(i_interpolation) : NULL,
				     amitk_interpolation_explanations[i_interpolation],
				     ui_study_cb_interpolation,
				     ui_study, 
				     icon_interpolation[i_interpolation]);
  ui_study_menus_fill_in_end(&(interpolation_list[AMITK_INTERPOLATION_NUM]));

  /* start make the fuse_type toolbar items*/
  for (i_fuse_type = 0; i_fuse_type < AMITK_FUSE_TYPE_NUM; i_fuse_type++) 
    ui_study_menus_fill_in_radioitem(&(fuse_type_list[i_fuse_type]),
				     (icon_fuse_type[i_fuse_type] == NULL) ? 
				     amitk_fuse_type_get_name(i_fuse_type) : NULL,
				     amitk_fuse_type_explanations[i_fuse_type],
				     ui_study_cb_fuse_type,
				     ui_study, 
				     icon_fuse_type[i_fuse_type]);
  ui_study_menus_fill_in_end(&(fuse_type_list[AMITK_INTERPOLATION_NUM]));

  /* and the view modes */
  for (i_view_mode = 0; i_view_mode < AMITK_VIEW_MODE_NUM; i_view_mode++)
    ui_study_menus_fill_in_radioitem(&(view_mode_list[i_view_mode]),
				     (icon_view_mode[i_view_mode] == NULL) ? 
				     view_mode_names[i_view_mode] : NULL,
				     view_mode_explanations[i_view_mode],
				     ui_study_cb_view_mode,
				     ui_study, 
				     icon_view_mode[i_view_mode]);
  ui_study_menus_fill_in_end(&(view_mode_list[AMITK_VIEW_MODE_NUM]));


  /* make the toolbar */
  toolbar = gtk_toolbar_new();
  gnome_app_fill_toolbar(GTK_TOOLBAR(toolbar), study_main_toolbar, NULL);



  /* finish setting up the interpolation items */
  for (i_interpolation = 0; i_interpolation < AMITK_INTERPOLATION_NUM; i_interpolation++) {
    ui_study->interpolation_button[i_interpolation] = interpolation_list[i_interpolation].widget;
    g_object_set_data(G_OBJECT(ui_study->interpolation_button[i_interpolation]), 
		      "interpolation", GINT_TO_POINTER(i_interpolation));
    gtk_widget_set_sensitive(ui_study->interpolation_button[i_interpolation], FALSE);
  }
  

  /* finish setting up the fuse types items */
  for (i_fuse_type = 0; i_fuse_type < AMITK_FUSE_TYPE_NUM; i_fuse_type++) {
    g_object_set_data(G_OBJECT(fuse_type_list[i_fuse_type].widget), 
		      "fuse_type", GINT_TO_POINTER(i_fuse_type));
    g_signal_handlers_block_by_func(G_OBJECT(fuse_type_list[i_fuse_type].widget),
				    G_CALLBACK(ui_study_cb_fuse_type), ui_study);
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fuse_type_list[AMITK_STUDY_FUSE_TYPE(study)].widget),
			       TRUE);
  for (i_fuse_type = 0; i_fuse_type < AMITK_FUSE_TYPE_NUM; i_fuse_type++)
    g_signal_handlers_unblock_by_func(G_OBJECT(fuse_type_list[i_fuse_type].widget),
				      G_CALLBACK(ui_study_cb_fuse_type),  ui_study);
  
  /* and the view modes */
  for (i_view_mode = 0; i_view_mode < AMITK_VIEW_MODE_NUM; i_view_mode++) {
    g_object_set_data(G_OBJECT(view_mode_list[i_view_mode].widget), 
		      "view_mode", GINT_TO_POINTER(i_view_mode));
    g_signal_handlers_block_by_func(G_OBJECT(view_mode_list[i_view_mode].widget),
				   G_CALLBACK(ui_study_cb_view_mode), ui_study);
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(view_mode_list[ui_study->view_mode].widget),
			       TRUE);
  for (i_view_mode = 0; i_view_mode < AMITK_VIEW_MODE_NUM; i_view_mode++)
    g_signal_handlers_unblock_by_func(G_OBJECT(view_mode_list[i_view_mode].widget),
				      G_CALLBACK(ui_study_cb_view_mode),ui_study);
  

  /* add the zoom widget to our toolbar */
  label = gtk_label_new("zoom:");
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), label, NULL, NULL);
  gtk_widget_show(label);

  adjustment = gtk_adjustment_new(AMITK_STUDY_ZOOM(study),
				  AMIDE_LIMIT_ZOOM_LOWER,
				  AMIDE_LIMIT_ZOOM_UPPER,
				  AMIDE_LIMIT_ZOOM_STEP, 
				  AMIDE_LIMIT_ZOOM_PAGE,
				  AMIDE_LIMIT_ZOOM_PAGE);
  ui_study->zoom_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 0.25, 2);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(ui_study->zoom_spin),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(ui_study->zoom_spin), FALSE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ui_study->zoom_spin), TRUE);
  gtk_widget_set_size_request (ui_study->zoom_spin, 50, -1);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(ui_study->zoom_spin), GTK_UPDATE_ALWAYS);

  g_signal_connect(G_OBJECT(ui_study->zoom_spin), "value_changed",  
		   G_CALLBACK(ui_study_cb_zoom), ui_study);
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), ui_study->zoom_spin, 
  			    "specify how much to magnify the images", NULL);
  gtk_widget_show(ui_study->zoom_spin);
			      
  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));


  /* add the slice thickness selector */
  label = gtk_label_new("thickness:");
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), label, NULL, NULL);
  gtk_widget_show(label);

  adjustment = gtk_adjustment_new(1.0,0.2,5.0,0.2,0.2, 0.2);
  ui_study->thickness_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment),1.0, 2);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(ui_study->thickness_spin),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(ui_study->thickness_spin), FALSE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ui_study->thickness_spin), TRUE);
  gtk_widget_set_size_request (ui_study->thickness_spin, 50, -1);

  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(ui_study->thickness_spin), 
				    GTK_UPDATE_IF_VALID);

  g_signal_connect(G_OBJECT(ui_study->thickness_spin), "value_changed", 
  		   G_CALLBACK(ui_study_cb_thickness), ui_study);
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), ui_study->thickness_spin, 
			    "specify how thick to make the slices (mm)", NULL);
  gtk_widget_show(ui_study->thickness_spin);

  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

  /* frame selector */
  label = gtk_label_new("time:");
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), label, NULL, NULL);
  gtk_widget_show(label);

  ui_study->time_button = gtk_button_new_with_label(""); 

  g_signal_connect(G_OBJECT(ui_study->time_button), "pressed",
		   G_CALLBACK(ui_study_cb_time_pressed), 
		   ui_study);
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), 
  			    ui_study->time_button, "the time range over which to view the data (s)", NULL);
  gtk_widget_show(ui_study->time_button);






  /* add our toolbar to our app */
  gnome_app_set_toolbar(GNOME_APP(ui_study->app), GTK_TOOLBAR(toolbar));



  return;

}









