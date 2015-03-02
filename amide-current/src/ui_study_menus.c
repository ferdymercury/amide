/* ui_study_menus.c
 *
 * Part of amide - Amide's a Medical Image Dataset Viewer
 * Copyright (C) 2000 Andy Loening
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
#include "realspace.h"
#include "volume.h"
#include "roi.h"
#include "study.h"
#include "color_table.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_study_rois.h"
#include "ui_study.h"
#include "ui_study_callbacks.h"
#include "ui_study_menus.h"
#include "ui_study_rois_callbacks.h"
#include "ui_main_callbacks.h"

/* function to setup the menus for the main ui */
void ui_study_menus_create(ui_study_t * ui_study) {

  
  /* defining the menus for the study ui interface */
  GnomeUIInfo file_menu[] = {
#ifdef AMIDE_CTI_SUPPORT /*#ifdef AMIDE_CTI_SUPPORT | AMIDE_ANALYZE_SUPPORT | etc. (get it?) */
    GNOMEUIINFO_ITEM_DATA(N_("_Import File"),
		     N_("Import a non-amide data file"),
		     ui_study_callbacks_import, 
		     ui_study, NULL),
#endif 
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_MENU_CLOSE_ITEM(ui_study_callbacks_close_event, ui_study),
    GNOMEUIINFO_END
  };

  /* the submenu under the slide show button */
  GnomeUIInfo slide_menu[] = {
    GNOMEUIINFO_ITEM_DATA(N_("_Transverse"),
			  N_("Look at a series of transaxial images"),
			  ui_study_callbacks_transverse_series, 
			  ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(N_("_Coronal"),
			  N_("Look at a series of coronal images"),
			  ui_study_callbacks_coronal_series, 
			  ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(N_("_Sagittal"),
			  N_("Look at a series of sagittal images"),
			  ui_study_callbacks_sagittal_series, 
			  ui_study, NULL),
    GNOMEUIINFO_END
  };

  /* defining the menus for the study ui interface */
  GnomeUIInfo view_menu[] = {
    GNOMEUIINFO_SUBTREE_HINT(N_("_Slide Show"),
			     N_("Look at a series of images"), 
			     slide_menu),
    GNOMEUIINFO_END
  };

  /* the submenu under the roi analysis menu item */
  GnomeUIInfo roi_menu[] = {
    GNOMEUIINFO_ITEM_DATA(N_("_All"),
			  N_("caculate values for all ROI's"),
			  ui_study_rois_callbacks_calculate_all,
			  ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(N_("_Selected"),
			  N_("calculate values only for the currently selected ROI's"),
			  ui_study_rois_callbacks_calculate_selected,
			  ui_study, NULL),
    GNOMEUIINFO_END
  };

  /* defining the menus for doing analysis */
  GnomeUIInfo analysis_menu[] = {
    GNOMEUIINFO_SUBTREE_HINT(N_("Calculate _ROI's"),
			     N_("Calculate values over the ROI's"), 
			     roi_menu),
    GNOMEUIINFO_END
  };

  /* our help menu... */
  GnomeUIInfo study_help_menu[]= {
    GNOMEUIINFO_HELP(PACKAGE),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_MENU_ABOUT_ITEM(ui_main_callbacks_about, NULL), 
    GNOMEUIINFO_END
  };

  /* and the main menu definition */
  GnomeUIInfo study_main_menu[] = {
    GNOMEUIINFO_SUBTREE(N_("_File"), file_menu),
    GNOMEUIINFO_SUBTREE(N_("_View"), view_menu),
    GNOMEUIINFO_SUBTREE(N_("_Analysis"), analysis_menu),
    GNOMEUIINFO_SUBTREE(N_("_Help"), study_help_menu),
    GNOMEUIINFO_END
  };


  /* sanity check */
  g_assert(ui_study!=NULL);

  /* and make the menu */
  gnome_app_create_menus(ui_study->app, study_main_menu);


  return;

}





