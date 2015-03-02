/* ui_series_toolbar.c
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
#include "study.h"
#include "rendering.h"
#include "ui_series.h"
#include "ui_series_cb.h"
#include "ui_series_menus.h"
#include "ui_study_menus.h"
#include "../pixmaps/icon_threshold.xpm"
#include "../pixmaps/icon_scaling_per_slice.xpm"
#include "../pixmaps/icon_scaling_global.xpm"


      

/* function to setup the toolbar for the seriesui */
void ui_series_toolbar_create(ui_series_t * ui_series) {

  scaling_t i_scaling;
  GtkWidget * toolbar;
  gchar ** icon_scaling[NUM_SCALINGS] = {icon_scaling_per_slice_xpm,
					icon_scaling_global_xpm};


  /* the toolbar definitions */
  GnomeUIInfo scaling_list[NUM_SCALINGS+1];

  GnomeUIInfo series_main_toolbar[] = {
    GNOMEUIINFO_RADIOLIST(scaling_list),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_ITEM_DATA(NULL,
			  N_("Set the thresholds and colormaps for the data sets in the series view"),
			  ui_series_cb_threshold,
			  ui_series, icon_threshold_xpm),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_END
  };

  /* sanity check */
  g_assert(ui_series!=NULL);

  
  /* start make the scaling toolbar items*/
  for (i_scaling = 0; i_scaling < NUM_SCALINGS; i_scaling++)
    ui_study_menus_fill_in_radioitem(&(scaling_list[i_scaling]),
				     (icon_scaling[i_scaling] == NULL) ? scaling_names[i_scaling] : NULL,
				     scaling_explanations[i_scaling],
				     ui_series_cb_scaling,
				     ui_series, 
				     icon_scaling[i_scaling]);
  ui_study_menus_fill_in_end(&(scaling_list[NUM_SCALINGS]));

  /* make the toolbar */
  toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL,GTK_TOOLBAR_BOTH);
  gnome_app_fill_toolbar(GTK_TOOLBAR(toolbar), series_main_toolbar, NULL);




  /* finish setting up the scaling  items */
  for (i_scaling = 0; i_scaling < NUM_SCALINGS; i_scaling++) {
    gtk_object_set_data(GTK_OBJECT(scaling_list[i_scaling].widget), "scaling", GINT_TO_POINTER(i_scaling));
    gtk_signal_handler_block_by_func(GTK_OBJECT(scaling_list[i_scaling].widget),
				     GTK_SIGNAL_FUNC(ui_series_cb_scaling),
				     ui_series);
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scaling_list[ui_series->scaling].widget),
			       TRUE);
  for (i_scaling = 0; i_scaling < NUM_SCALINGS; i_scaling++)
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(scaling_list[i_scaling].widget),
  				       GTK_SIGNAL_FUNC(ui_series_cb_scaling),
  				       ui_series);




  /* add our toolbar to our app */
  gnome_app_set_toolbar(GNOME_APP(ui_series->app), GTK_TOOLBAR(toolbar));



  return;

}









