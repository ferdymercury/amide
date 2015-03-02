/* ui_study_menus.c
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
#include "ui_study_rois_callbacks.h"
#include "ui_main_callbacks.h"


/* function to fill in a radioitem */
void ui_study_menu_fill_in_radioitem(GnomeUIInfo * item, 
				     gchar * name,
				     gchar * tooltip,
				     gpointer callback_func,
				     gpointer callback_data,
				     gpointer xpm_data) {
  
  item->type = GNOME_APP_UI_ITEM;
  item->label = name;
  item->hint = tooltip;
  item->moreinfo = callback_func;
  item->user_data = callback_data;
  item->unused_data = NULL;
  item->pixmap_type =  GNOME_APP_PIXMAP_DATA;
  item->pixmap_info = xpm_data;
  item->accelerator_key = 0;
  item->ac_mods = (GdkModifierType) 0;
  item->widget = NULL;

  return;
}

/* functionto fill in the end of a menu */
void ui_study_menu_fill_in_end(GnomeUIInfo * item) {

  item->type = GNOME_APP_UI_ENDOFINFO;
  item->label = NULL;
  item->hint = NULL;
  item->moreinfo = NULL;
  item->user_data = NULL;
  item->unused_data = NULL;
  item->pixmap_type =  (GnomeUIPixmapType) 0;
  item->pixmap_info = NULL;
  item->accelerator_key = 0;
  item->ac_mods = (GdkModifierType) 0;
  item->widget = NULL;

  return;
}

/* function to setup the menus for the study ui */
void ui_study_menus_create(ui_study_t * ui_study) {

  
  /* defining the menus for the study ui interface */
  GnomeUIInfo file_menu[] = {
    GNOMEUIINFO_MENU_SAVE_AS_ITEM(ui_study_callbacks_save_as, ui_study),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_ITEM_DATA(N_("_Import File"),
		     N_("Import an image data file into this study"),
		     ui_study_callbacks_import, 
		     ui_study, NULL),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_MENU_CLOSE_ITEM(ui_study_callbacks_close_event, ui_study),
    GNOMEUIINFO_END
  };

  GnomeUIInfo add_roi_menu[] = {
    GNOMEUIINFO_ITEM_DATA(roi_type_names[ELLIPSOID], 
			  N_("Add a new elliptical ROI"),
			  ui_study_callbacks_new_roi_ellipsoid,
			  ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(roi_type_names[CYLINDER], 
			  N_("Add a new elliptic cylinder ROI"),
			  ui_study_callbacks_new_roi_cylinder,
			  ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(roi_type_names[BOX], 
			  N_("Add a new box shaped ROI"),
			  ui_study_callbacks_new_roi_box,
			  ui_study, NULL),
    GNOMEUIINFO_END
  };

  GnomeUIInfo edit_menu[] = {
    GNOMEUIINFO_ITEM_DATA(N_("_Edit"),
			  N_("Edit the selected objects"),
			  ui_study_callbacks_edit_objects,
			  ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(N_("_Delete"),
			  N_("Delete the selected objects"),
			  ui_study_callbacks_delete_objects,
			  ui_study, NULL),
    GNOMEUIINFO_SUBTREE_HINT(N_("_Add ROI"),
			     N_("Add a new ROI"),
			     add_roi_menu),
    GNOMEUIINFO_END
  };

  /* the submenus under the series_type menu */
  GnomeUIInfo series_space_menu[] = {
    GNOMEUIINFO_ITEM_DATA(N_("_Transverse"),
			  N_("Look at a series of transaxial images in a single frame"),
			  ui_study_callbacks_transverse_series_planes, 
			  ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(N_("_Coronal"),
			  N_("Look at a series of coronal images in a single frame"),
			  ui_study_callbacks_coronal_series_planes, 
			  ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(N_("_Sagittal"),
			  N_("Look at a series of sagittal images in a single frame"),
			  ui_study_callbacks_sagittal_series_planes, 
			  ui_study, NULL),
    GNOMEUIINFO_END
  };

  GnomeUIInfo series_time_menu[] = {
    GNOMEUIINFO_ITEM_DATA(N_("_Transverse"),
			  N_("Look at a times series of a transaxial plane"),
			  ui_study_callbacks_transverse_series_frames, 
			  ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(N_("_Coronal"),
			  N_("Look at a time series of a coronal plane"),
			  ui_study_callbacks_coronal_series_frames, 
			  ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(N_("_Sagittal"),
			  N_("Look at a time series of a sagittal plane"),
			  ui_study_callbacks_sagittal_series_frames, 
			  ui_study, NULL),
    GNOMEUIINFO_END
  };

  /* the submenu under the series button */
  GnomeUIInfo series_type_menu[] = {
    GNOMEUIINFO_SUBTREE_HINT(N_("_Space"),
			     N_("Look at a series of images over a spacial dimension"), 
			     series_space_menu),
    GNOMEUIINFO_SUBTREE_HINT(N_("_Time"),
			     N_("Look at a series of images over time"), 
			     series_time_menu),
    GNOMEUIINFO_END
  };


  /* defining the menus for the study ui interface */
  GnomeUIInfo view_menu[] = {
    GNOMEUIINFO_SUBTREE_HINT(N_("_Series"),
			     N_("Look at a series of images"), 
			     series_type_menu),
#if AMIDE_LIBVOLPACK_SUPPORT
    GNOMEUIINFO_ITEM_DATA(N_("_Rendering"),
			  N_("perform volume renderings on the currently selected volumes"),
			  ui_study_callbacks_rendering,
			  ui_study, NULL),
#endif
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
    GNOMEUIINFO_MENU_FILE_TREE(file_menu),
    GNOMEUIINFO_MENU_EDIT_TREE(edit_menu),
    GNOMEUIINFO_MENU_VIEW_TREE(view_menu),
    GNOMEUIINFO_SUBTREE(N_("_Analysis"), analysis_menu),
    GNOMEUIINFO_MENU_HELP_TREE(study_help_menu),
    GNOMEUIINFO_END
  };


  /* sanity check */
  g_assert(ui_study!=NULL);

  /* make the menu */
  gnome_app_create_menus(ui_study->app, study_main_menu);

  return;

}






