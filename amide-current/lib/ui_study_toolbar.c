/* ui_study_toolbar.c
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
#include "amide.h"
#include "study.h"
#include "rendering.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_roi.h"
#include "ui_volume.h"
#include "ui_study.h"
#include "ui_study_callbacks.h"
#include "ui_study_menus.h"
#include "ui_study_toolbar.h"
#include "../pixmaps/icon_threshold.xpm"
#include "../pixmaps/icon_scaling_per_slice.xpm"
#include "../pixmaps/icon_scaling_global.xpm"
#include "../pixmaps/icon_interpolation_nearest_neighbor.xpm"
#include "../pixmaps/icon_interpolation_2x2x1.xpm"
#include "../pixmaps/icon_interpolation_2x2x2.xpm"
#include "../pixmaps/icon_interpolation_bilinear.xpm"
#include "../pixmaps/icon_interpolation_trilinear.xpm"


      

/* function to setup the toolbar for the study ui */
void ui_study_toolbar_create(ui_study_t * ui_study) {

  scaling_t i_scaling;
  interpolation_t i_interpolation;
  GtkWidget * label;
  GtkWidget * toolbar;
  GtkObject * adjustment;
  GtkWidget * spin_button;
  GtkWidget * button;
  gchar * temp_string;
  gchar ** icon_scaling[NUM_SCALINGS] = {icon_scaling_per_slice_xpm,
					icon_scaling_global_xpm};

  gchar ** icon_interpolation[NUM_INTERPOLATIONS] = {icon_interpolation_nearest_neighbor_xpm,
						    icon_interpolation_2x2x1_xpm,
						    icon_interpolation_2x2x2_xpm,
						    icon_interpolation_bilinear_xpm,
						    icon_interpolation_trilinear_xpm};

  /* the toolbar definitions */
  GnomeUIInfo scaling_list[NUM_SCALINGS+1];
  GnomeUIInfo interpolation_list[NUM_INTERPOLATIONS+1];

  GnomeUIInfo study_main_toolbar[] = {
    GNOMEUIINFO_RADIOLIST(scaling_list),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_RADIOLIST(interpolation_list),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_ITEM_DATA(NULL,
			  N_("Set the thresholds and colormaps for the active data set"),
			  ui_study_callbacks_threshold_pressed,
			  ui_study, icon_threshold_xpm),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_END
  };

  /* sanity check */
  g_assert(ui_study!=NULL);


  
  /* start make the scaling toolbar items*/
  for (i_scaling = 0; i_scaling < NUM_SCALINGS; i_scaling++)
    ui_study_menu_fill_in_radioitem(&(scaling_list[i_scaling]),
				    (icon_scaling[i_scaling] == NULL) ? scaling_names[i_scaling] : NULL,
				    scaling_explanations[i_scaling],
				    ui_study_callbacks_scaling,
				    ui_study, 
				    icon_scaling[i_scaling]);
  ui_study_menu_fill_in_end(&(scaling_list[NUM_SCALINGS]));

  /* start make the interpolation toolbar items*/
  for (i_interpolation = 0; i_interpolation < NUM_INTERPOLATIONS; i_interpolation++)
    ui_study_menu_fill_in_radioitem(&(interpolation_list[i_interpolation]),
				    (icon_interpolation[i_interpolation] == NULL) ? 
				    interpolation_names[i_interpolation] : NULL,
				    interpolation_explanations[i_interpolation],
				    ui_study_callbacks_interpolation,
				    ui_study, 
				    icon_interpolation[i_interpolation]);
  ui_study_menu_fill_in_end(&(interpolation_list[NUM_INTERPOLATIONS]));



  /* make the toolbar */
  toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL,GTK_TOOLBAR_BOTH);
  gnome_app_fill_toolbar(GTK_TOOLBAR(toolbar), study_main_toolbar, NULL);




  /* finish setting up the scaling  items */
  for (i_scaling = 0; i_scaling < NUM_SCALINGS; i_scaling++) {
    gtk_object_set_data(GTK_OBJECT(scaling_list[i_scaling].widget), "scaling", GINT_TO_POINTER(i_scaling));
    gtk_signal_handler_block_by_func(GTK_OBJECT(scaling_list[i_scaling].widget),
				     GTK_SIGNAL_FUNC(ui_study_callbacks_scaling),
				     ui_study);
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scaling_list[study_scaling(ui_study->study)].widget),
			       TRUE);
  for (i_scaling = 0; i_scaling < NUM_SCALINGS; i_scaling++)
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(scaling_list[i_scaling].widget),
  				       GTK_SIGNAL_FUNC(ui_study_callbacks_scaling),
  				       ui_study);

  /* finish setting up the interpolation items */
  for (i_interpolation = 0; i_interpolation < NUM_INTERPOLATIONS; i_interpolation++) {
    gtk_object_set_data(GTK_OBJECT(interpolation_list[i_interpolation].widget), 
			"interpolation", GINT_TO_POINTER(i_interpolation));
    gtk_signal_handler_block_by_func(GTK_OBJECT(interpolation_list[i_interpolation].widget),
				     GTK_SIGNAL_FUNC(ui_study_callbacks_interpolation),
				     ui_study);
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(interpolation_list[study_interpolation(ui_study->study)].widget),
			       TRUE);
  for (i_interpolation = 0; i_interpolation < NUM_INTERPOLATIONS; i_interpolation++)
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(interpolation_list[i_interpolation].widget),
				       GTK_SIGNAL_FUNC(ui_study_callbacks_interpolation),
				       ui_study);


  /* add the zoom widget to our toolbar */
  label = gtk_label_new("zoom:");
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), label, NULL, NULL);
  gtk_widget_show(label);

  adjustment = gtk_adjustment_new(study_zoom(ui_study->study), 0.2,5,0.2, 0.25, 0.25);
  spin_button = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 0.25, 2);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_button),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(spin_button), FALSE);
  gtk_widget_set_usize (spin_button, 50, 0);

  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin_button), GTK_UPDATE_ALWAYS);

  gtk_signal_connect(adjustment, "value_changed",  
		     GTK_SIGNAL_FUNC(ui_study_callbacks_zoom), 
		     ui_study);
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), 
  			    spin_button, "specify how much to magnify the images", NULL);
  gtk_widget_show(spin_button);
			      
  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));


  /* add the slice thickness selector */
  label = gtk_label_new("thickness:");
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), label, NULL, NULL);
  gtk_widget_show(label);

  adjustment = gtk_adjustment_new(1.0,1.0,1.0,1.0,1.0,1.0);
  ui_study->thickness_adjustment = adjustment;
  ui_study->thickness_spin_button = spin_button = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment),1.0, 2);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_button),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(spin_button), FALSE);
  gtk_widget_set_usize (spin_button, 50, 0);

  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin_button), 
				    GTK_UPDATE_ALWAYS);

  gtk_signal_connect(adjustment, "value_changed", 
		     GTK_SIGNAL_FUNC(ui_study_callbacks_thickness), 
		     ui_study);
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), 
  			    spin_button, "specify how thick to make the slices (mm)", NULL);
  gtk_widget_show(spin_button);

  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

  /* frame selector */
  label = gtk_label_new("time:");
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), label, NULL, NULL);
  gtk_widget_show(label);

  temp_string = g_strdup_printf("%5.1f -%5.1fs",
				study_view_time(ui_study->study),
				study_view_duration(ui_study->study));
  button = gtk_button_new_with_label(temp_string);
  ui_study->time_button = button;
  g_free(temp_string);
  gtk_signal_connect(GTK_OBJECT(button), "pressed",
		     GTK_SIGNAL_FUNC(ui_study_callbacks_time_pressed), 
		     ui_study);
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), 
  			    button, "the time range over which to view the data (s)", NULL);
  gtk_widget_show(button);






  /* add our toolbar to our app */
  gnome_app_set_toolbar(GNOME_APP(ui_study->app), GTK_TOOLBAR(toolbar));



  return;

}









