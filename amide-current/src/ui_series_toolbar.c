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


      

/* function to setup the toolbar for the seriesui */
void ui_series_toolbar_create(ui_series_t * ui_series) {

  GtkWidget * toolbar;

  /* the toolbar definitions */
  GnomeUIInfo series_main_toolbar[] = {
    GNOMEUIINFO_ITEM_DATA(NULL,
			  N_("Set the thresholds and colormaps for the data sets in the series view"),
			  ui_series_cb_threshold,
			  ui_series, icon_threshold_xpm),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_END
  };

  /* sanity check */
  g_assert(ui_series!=NULL);

  
  /* make the toolbar */
  toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL,GTK_TOOLBAR_BOTH);
  gnome_app_fill_toolbar(GTK_TOOLBAR(toolbar), series_main_toolbar, NULL);



  /* add our toolbar to our app */
  gnome_app_set_toolbar(GNOME_APP(ui_series->app), GTK_TOOLBAR(toolbar));



  return;

}









