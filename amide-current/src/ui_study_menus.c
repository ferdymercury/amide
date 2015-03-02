/* ui_study_menus.c
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




#include "amide_config.h"
#include <gnome.h>
#include "ui_common.h"
#include "ui_study.h"
#include "ui_study_cb.h"
#include "ui_study_menus.h"
#include "ui_series.h"
#include "medcon_import.h"

/* function to fill in a radioitem */
void ui_study_menus_fill_in_radioitem(GnomeUIInfo * item, 
				      const gchar * name,
				      gchar * tooltip,
				      gpointer callback_func,
				      gpointer callback_data,
				      gpointer xpm_data) {
  
  item->type = GNOME_APP_UI_ITEM;
  if (name != NULL)
    item->label = g_strdup(name);
  else
    item->label = NULL;
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

/* function to fill in a menuitem */
void ui_study_menus_fill_in_menuitem(GnomeUIInfo * item, 
				     gchar * name,
				     gchar * tooltip,
				     gpointer callback_func,
				     gpointer callback_data) {

  ui_study_menus_fill_in_radioitem(item, name, tooltip, callback_func, callback_data, NULL);

  return;
}
void ui_study_menus_fill_in_submenu(GnomeUIInfo * item, 
				    gchar * name,
				    gchar * tooltip,
				    GnomeUIInfo * submenu) {
  item->type = GNOME_APP_UI_SUBTREE;
  item->label = name;
  item->hint = tooltip;
  item->moreinfo = submenu;
  item->user_data = NULL;
  item->unused_data = NULL;
  item->pixmap_type = (GnomeUIPixmapType) 0; 
  item->pixmap_info = NULL;
  item->accelerator_key = 0;
  item->ac_mods = (GdkModifierType) 0;
  item->widget = NULL;

  return;
}

/* functionto fill in the end of a menu */
void ui_study_menus_fill_in_end(GnomeUIInfo * item) {

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

  AmitkView i_view;
  AmitkImportMethod i_method;
#ifdef AMIDE_LIBMDC_SUPPORT
  libmdc_import_method_t i_libmdc;
#endif
  gint counter;
  AmitkRoiType i_roi_type;


#ifdef AMIDE_LIBMDC_SUPPORT
  GnomeUIInfo import_specific_menu[AMITK_IMPORT_METHOD_NUM + LIBMDC_NUM_IMPORT_METHODS];
#else
  GnomeUIInfo import_specific_menu[AMITK_IMPORT_METHOD_NUM+1];
#endif

  GnomeUIInfo export_view_menu[] = {
    GNOMEUIINFO_ITEM_DATA(N_("_Transverse"),
			  N_("Export the current transaxial view to an image file (JPEG/TIFF/PNG/etc.)"),
			  ui_study_cb_export, ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(N_("_Coronal"),
			  N_("Export the current coronal view to an image file (JPEG/TIFF/PNG/etc.)"),
			  ui_study_cb_export, ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(N_("_Sagittal"),
			  N_("Export the current sagittal view to an image file (JPEG/TIFF/PNG/etc.)"),
			  ui_study_cb_export, ui_study, NULL),
    GNOMEUIINFO_END
  };
  
  /* defining the menus for the study ui interface */
  GnomeUIInfo file_menu[] = {
    GNOMEUIINFO_MENU_NEW_ITEM(N_("_New Study"), 
			      N_("Create a new study viewer window"),
			      ui_study_cb_new_study, NULL),
    GNOMEUIINFO_MENU_OPEN_ITEM(ui_study_cb_open_study, ui_study),
    GNOMEUIINFO_MENU_SAVE_AS_ITEM(ui_study_cb_save_as, ui_study),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_ITEM_DATA(N_("_Import File (guess)"),
			  N_("Import an image data file into this study, guessing at the file type"),
			  ui_study_cb_import, 
			  ui_study, NULL),
    GNOMEUIINFO_SUBTREE_HINT(N_("Import File (_specify)"),
			     N_("Import an image data file into this study, specifying the import filter"), 
			     import_specific_menu),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_SUBTREE_HINT(N_("_Export View"),
			     N_("Export one of the views to a data file"),
			     export_view_menu),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_MENU_CLOSE_ITEM(ui_study_cb_close, ui_study),
    GNOMEUIINFO_MENU_EXIT_ITEM(ui_study_cb_exit, ui_study),
    GNOMEUIINFO_END
  };

  GnomeUIInfo add_roi_menu[AMITK_ROI_TYPE_NUM+1];

  GnomeUIInfo edit_menu[] = {
    GNOMEUIINFO_SUBTREE_HINT(N_("Add _ROI"),
			     N_("Add a new ROI"),
			     add_roi_menu),
    GNOMEUIINFO_ITEM_DATA(N_("Add _Fiducial Mark"),
			  N_("Add a new fiducial mark to the active data set"),
			  ui_study_cb_add_fiducial_mark, ui_study, NULL),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_MENU_PREFERENCES_ITEM(ui_study_cb_preferences, ui_study),
    GNOMEUIINFO_END
  };

  /* the submenus under the series_type menu */
  GnomeUIInfo series_space_menu[] = {
    GNOMEUIINFO_ITEM_DATA(N_("_Transverse"),
			  N_("Look at a series of transaxial views in a single frame"),
			  ui_study_cb_series, ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(N_("_Coronal"),
			  N_("Look at a series of coronal views in a single frame"),
			  ui_study_cb_series, ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(N_("_Sagittal"),
			  N_("Look at a series of sagittal views in a single frame"),
			  ui_study_cb_series, ui_study, NULL),
    GNOMEUIINFO_END
  };

  GnomeUIInfo series_time_menu[] = {
    GNOMEUIINFO_ITEM_DATA(N_("_Transverse"),
			  N_("Look at a times series of a single transaxial view"),
			  ui_study_cb_series, ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(N_("_Coronal"),
			  N_("Look at a time series of a single coronal view"),
			  ui_study_cb_series, ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(N_("_Sagittal"),
			  N_("Look at a time series of a a signal sagittal view"),
			  ui_study_cb_series, ui_study, NULL),
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
    GNOMEUIINFO_ITEM_DATA(N_("_Volume Rendering"),
			  N_("perform a volume rendering on the currently selected objects"),
			  ui_study_cb_render,
			  ui_study, NULL),
#endif
    GNOMEUIINFO_END
  };

#if AMIDE_LIBFAME_SUPPORT
  GnomeUIInfo fly_through_menu[] = {
    GNOMEUIINFO_ITEM_DATA(N_("_Transverse"),
			  N_("Generate a fly through using transaxial slices"),
			  ui_study_cb_fly_through,  ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(N_("_Coronal"),
			  N_("Generate a fly through using coronal slices"),
			  ui_study_cb_fly_through,  ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(N_("_Sagittal"),
			  N_("Generate a fly through using sagittal slices"),
			  ui_study_cb_fly_through,  ui_study, NULL),
    GNOMEUIINFO_END
  };
#endif

  /* tools for analyzing/etc. the data */
  GnomeUIInfo tools_menu[] = {
    GNOMEUIINFO_ITEM_DATA(N_("_Crop Active Data Set"),
			  N_("allows you to crop the active data set"),
			  ui_study_cb_crop_selected,
			  ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(N_("_Filter Active Data Set"),
			  N_("allows you to filter the active data set"),
			  ui_study_cb_filter_selected,
			  ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA(N_("_Alignment Wizard"),
			  N_("guides you throw the processing of alignment"),
			  ui_study_cb_alignment_selected,
			  ui_study, NULL),
#if AMIDE_LIBFAME_SUPPORT
    GNOMEUIINFO_SUBTREE_HINT(N_("Generate _Fly Through"),
			     N_("generate an mpeg fly through of the data sets"), 
			     fly_through_menu),
#endif
    GNOMEUIINFO_ITEM_DATA(N_("Calculate _ROI Statistics"),
			  N_("caculate ROI statistics"),
			  ui_study_cb_roi_statistics, ui_study, NULL),
    GNOMEUIINFO_END
  };

  /* and the main menu definition */
  GnomeUIInfo study_main_menu[] = {
    GNOMEUIINFO_MENU_FILE_TREE(file_menu),
    GNOMEUIINFO_MENU_EDIT_TREE(edit_menu),
    GNOMEUIINFO_MENU_VIEW_TREE(view_menu),
    GNOMEUIINFO_SUBTREE(N_("_Tools"), tools_menu),
    GNOMEUIINFO_MENU_HELP_TREE(ui_common_help_menu),
    GNOMEUIINFO_END
  };


  /* sanity check */
  g_assert(ui_study!=NULL);


  /* fill in some more menu definitions */
  for (i_roi_type = 0; i_roi_type < AMITK_ROI_TYPE_NUM; i_roi_type++) 
    ui_study_menus_fill_in_menuitem(&(add_roi_menu[i_roi_type]),
				    amitk_roi_menu_names[i_roi_type], 
				    amitk_roi_menu_explanation[i_roi_type],
				    ui_study_cb_add_roi, ui_study);
  ui_study_menus_fill_in_end(&(add_roi_menu[AMITK_ROI_TYPE_NUM]));

  counter = 0;
  for (i_method = AMITK_IMPORT_METHOD_RAW; i_method < AMITK_IMPORT_METHOD_NUM; i_method++) {
#ifdef AMIDE_LIBMDC_SUPPORT
    if (i_method == AMITK_IMPORT_METHOD_LIBMDC) {
      for (i_libmdc = LIBMDC_GIF; i_libmdc < LIBMDC_NUM_IMPORT_METHODS; i_libmdc++) {
	if (medcon_import_supports(i_libmdc)) {
	  ui_study_menus_fill_in_menuitem(&(import_specific_menu[counter]),
					  libmdc_menu_names[i_libmdc],
					  libmdc_menu_explanations[i_libmdc],
					  ui_study_cb_import, ui_study);
	  counter++;
	}
      }
    } else 
#endif
      {
	ui_study_menus_fill_in_menuitem(&(import_specific_menu[counter]),
					amitk_import_menu_names[i_method],
					amitk_import_menu_explanations[i_method],
					ui_study_cb_import, ui_study);
	counter++;
      }
  }
  ui_study_menus_fill_in_end(&(import_specific_menu[counter]));


  /* make the menu */
  gnome_app_create_menus(GNOME_APP(ui_study->app), study_main_menu);

  /* add some info to some of the menus 
     note: the "Importing guess" widget doesn't have data set, NULL == AMIDE_GUESS */ 
  counter = 0;
  for (i_method = AMITK_IMPORT_METHOD_RAW; i_method < AMITK_IMPORT_METHOD_NUM; i_method++) {
#ifdef AMIDE_LIBMDC_SUPPORT
    if (i_method == AMITK_IMPORT_METHOD_LIBMDC) {
      for (i_libmdc = LIBMDC_GIF; i_libmdc < LIBMDC_NUM_IMPORT_METHODS; i_libmdc++) {
	if (medcon_import_supports(i_libmdc)) {
	  g_object_set_data(G_OBJECT(import_specific_menu[counter].widget),
			    "method", GINT_TO_POINTER(i_method));
	  g_object_set_data(G_OBJECT(import_specific_menu[counter].widget),
			    "submethod", GINT_TO_POINTER(i_libmdc));
	  counter++;
	}
      }
    } else 
#endif
      {
	g_object_set_data(G_OBJECT(import_specific_menu[counter].widget),
			  "method", GINT_TO_POINTER(i_method));
	counter++;
      }
  }
  
  for (i_roi_type = 0; i_roi_type < AMITK_ROI_TYPE_NUM; i_roi_type++)
    g_object_set_data(G_OBJECT(add_roi_menu[i_roi_type].widget), "roi_type", 
		      GINT_TO_POINTER(i_roi_type));

  for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view++) {
    g_object_set_data(G_OBJECT(export_view_menu[i_view].widget),
		      "view", GINT_TO_POINTER(i_view));
    g_object_set_data(G_OBJECT(series_space_menu[i_view].widget),
		      "view", GINT_TO_POINTER(i_view));
    g_object_set_data(G_OBJECT(series_space_menu[i_view].widget),
		      "series_type", GINT_TO_POINTER(PLANES));
    g_object_set_data(G_OBJECT(series_time_menu[i_view].widget),
		      "view", GINT_TO_POINTER(i_view));
    g_object_set_data(G_OBJECT(series_time_menu[i_view].widget),
		      "series_type", GINT_TO_POINTER(FRAMES));
#if AMIDE_LIBFAME_SUPPORT
    g_object_set_data(G_OBJECT(fly_through_menu[i_view].widget),
		      "view", GINT_TO_POINTER(i_view));
#endif
  }

  return;

}






