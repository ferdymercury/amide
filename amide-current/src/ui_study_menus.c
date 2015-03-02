/* ui_study_menus.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2003 Andy Loening
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




#include "amide_config.h"
#include <libgnomeui/libgnomeui.h>
#include "ui_common.h"
#include "ui_series.h"
#include "ui_study.h"
#include "ui_study_cb.h"
#include "ui_study_menus.h"
#include "medcon_import.h"
#include "pixmaps.h"
#include "amide_limits.h"

#define N_(String) (String) /* ignore internationalization */

static void fill_in_radioitem(GnomeUIInfo * item,
			      const gchar * name,
			      const gchar * tooltip,
			      gpointer cb_func,
			      gpointer cb_data,
			      gpointer xpm_data);
static void fill_in_toggleitem(GnomeUIInfo * item,
			       const gchar * name,
			       const gchar * tooltip,
			       gpointer cb_func,
			       gpointer cb_data,
			       gpointer xpm_data);
static void fill_in_menuitem(GnomeUIInfo * item, 
			     const gchar * name,
			     const gchar * tooltip,
			     gpointer cb_func,
			     gpointer cb_data);
static void fill_in_submenu(GnomeUIInfo * item, 
			    const gchar * name,
			    const gchar * tooltip,
			    GnomeUIInfo * submenu);
static void fill_in_end(GnomeUIInfo * item);

/* function to fill in a radioitem */
static void fill_in_radioitem(GnomeUIInfo * item, 
			      const gchar * name,
			      const gchar * tooltip,
			      gpointer cb_func,
			      gpointer cb_data,
			      gpointer xpm_data) {
  
  item->type = GNOME_APP_UI_ITEM;
  if (name != NULL)
    item->label = g_strdup(name);
  else
    item->label = NULL;
  item->hint = tooltip;
  item->moreinfo = cb_func;
  item->user_data = cb_data;
  item->unused_data = NULL;
  item->pixmap_type =  GNOME_APP_PIXMAP_DATA;
  item->pixmap_info = xpm_data;
  item->accelerator_key = 0;
  item->ac_mods = (GdkModifierType) 0;
  item->widget = NULL;

  return;
}

static void fill_in_toggleitem(GnomeUIInfo * item, 
			       const gchar * name,
			       const gchar * tooltip,
			       gpointer cb_func,
			       gpointer cb_data,
			       gpointer xpm_data) {
  fill_in_radioitem(item, name, tooltip, cb_func, cb_data, xpm_data);
  item->type = GNOME_APP_UI_TOGGLEITEM;

  return;
}

/* function to fill in a menuitem */
static void fill_in_menuitem(GnomeUIInfo * item, 
			     const gchar * name,
			     const gchar * tooltip,
			     gpointer cb_func,
			     gpointer cb_data) {
  
  fill_in_radioitem(item, name, tooltip, cb_func, cb_data, NULL);

  return;
}

static void fill_in_submenu(GnomeUIInfo * item, 
			    const gchar * name,
			    const gchar * tooltip,
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
static void fill_in_end(GnomeUIInfo * item) {

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
    GNOMEUIINFO_ITEM_DATA("_Transverse",
			  "Export the current transaxial view to an image file (JPEG/TIFF/PNG/etc.)",
			  ui_study_cb_export, ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA("_Coronal",
			  "Export the current coronal view to an image file (JPEG/TIFF/PNG/etc.)",
			  ui_study_cb_export, ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA("_Sagittal",
			  "Export the current sagittal view to an image file (JPEG/TIFF/PNG/etc.)",
			  ui_study_cb_export, ui_study, NULL),
    GNOMEUIINFO_END
  };
  
  /* defining the menus for the study ui interface */
  GnomeUIInfo file_menu[] = {
    GNOMEUIINFO_MENU_NEW_ITEM("_New Study", 
			      "Create a new study viewer window",
			      ui_study_cb_new_study, NULL),
    GNOMEUIINFO_MENU_OPEN_ITEM(ui_study_cb_open_study, ui_study),
    GNOMEUIINFO_MENU_SAVE_AS_ITEM(ui_study_cb_save_as, ui_study),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_ITEM_DATA("_Import File (guess)",
			  "Import an image data file into this study, guessing at the file type",
			  ui_study_cb_import, 
			  ui_study, NULL),
    GNOMEUIINFO_SUBTREE_HINT("Import File (_specify)",
			     "Import an image data file into this study, specifying the import filter", 
			     import_specific_menu),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_SUBTREE_HINT("_Export View",
			     "Export one of the views to a data file",
			     export_view_menu),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_MENU_CLOSE_ITEM(ui_study_cb_close, ui_study),
    GNOMEUIINFO_MENU_EXIT_ITEM(ui_study_cb_exit, ui_study),
    GNOMEUIINFO_END
  };

  GnomeUIInfo add_roi_menu[AMITK_ROI_TYPE_NUM+1];

  GnomeUIInfo edit_menu[] = {
    GNOMEUIINFO_SUBTREE_HINT("Add _ROI",
			     "Add a new ROI",
			     add_roi_menu),
    GNOMEUIINFO_ITEM_DATA("Add _Fiducial Mark",
			  "Add a new fiducial mark to the active data set",
			  ui_study_cb_add_fiducial_mark, ui_study, NULL),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_MENU_PREFERENCES_ITEM(ui_study_cb_preferences, ui_study),
    GNOMEUIINFO_END
  };

  /* the submenus under the series_type menu */
  GnomeUIInfo series_space_menu[] = {
    GNOMEUIINFO_ITEM_DATA("_Transverse",
			  "Look at a series of transaxial views in a single frame",
			  ui_study_cb_series, ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA("_Coronal",
			  "Look at a series of coronal views in a single frame",
			  ui_study_cb_series, ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA("_Sagittal",
			  "Look at a series of sagittal views in a single frame",
			  ui_study_cb_series, ui_study, NULL),
    GNOMEUIINFO_END
  };

  GnomeUIInfo series_time_menu[] = {
    GNOMEUIINFO_ITEM_DATA("_Transverse",
			  "Look at a times series of a single transaxial view",
			  ui_study_cb_series, ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA("_Coronal",
			  "Look at a time series of a single coronal view",
			  ui_study_cb_series, ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA("_Sagittal",
			  "Look at a time series of a a signal sagittal view",
			  ui_study_cb_series, ui_study, NULL),
    GNOMEUIINFO_END
  };

  /* the submenu under the series button */
  GnomeUIInfo series_type_menu[] = {
    GNOMEUIINFO_SUBTREE_HINT("_Space",
			     "Look at a series of images over a spacial dimension", 
			     series_space_menu),
    GNOMEUIINFO_SUBTREE_HINT("_Time",
			     "Look at a series of images over time", 
			     series_time_menu),
    GNOMEUIINFO_END
  };


  /* defining the menus for the study ui interface */
  GnomeUIInfo view_menu[] = {
    GNOMEUIINFO_SUBTREE_HINT("_Series",
			     "Look at a series of images", 
			     series_type_menu),
#if AMIDE_LIBVOLPACK_SUPPORT
    GNOMEUIINFO_ITEM_DATA("_Volume Rendering",
			  "perform a volume rendering on the currently selected objects",
			  ui_study_cb_render,
			  ui_study, NULL),
#endif
    GNOMEUIINFO_END
  };

#if AMIDE_LIBFAME_SUPPORT
  GnomeUIInfo fly_through_menu[] = {
    GNOMEUIINFO_ITEM_DATA("_Transverse",
			  "Generate a fly through using transaxial slices",
			  ui_study_cb_fly_through,  ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA("_Coronal",
			  "Generate a fly through using coronal slices",
			  ui_study_cb_fly_through,  ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA("_Sagittal",
			  "Generate a fly through using sagittal slices",
			  ui_study_cb_fly_through,  ui_study, NULL),
    GNOMEUIINFO_END
  };
#endif

  /* tools for analyzing/etc. the data */
  GnomeUIInfo tools_menu[] = {
    GNOMEUIINFO_ITEM_DATA("_Alignment Wizard",
			  "guides you throw the processing of alignment",
			  ui_study_cb_alignment_selected,
			  ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA("_Crop Active Data Set",
			  "allows you to crop the active data set",
			  ui_study_cb_crop_selected,
			  ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA("_Factor Analysis",
			  "allows you to do factor analysis of dynamic data on the active data set",
			  ui_study_cb_fads_selected,
			  ui_study, NULL),
    GNOMEUIINFO_ITEM_DATA("_Filter Active Data Set",
			  "allows you to filter the active data set",
			  ui_study_cb_filter_selected,
			  ui_study, NULL),
#if AMIDE_LIBFAME_SUPPORT
    GNOMEUIINFO_SUBTREE_HINT("Generate _Fly Through",
			     "generate an mpeg fly through of the data sets", 
			     fly_through_menu),
#endif
    GNOMEUIINFO_ITEM_DATA("Calculate _ROI Statistics",
			  "caculate ROI statistics",
			  ui_study_cb_roi_statistics, ui_study, NULL),
    GNOMEUIINFO_END
  };

  /* and the main menu definition */
  GnomeUIInfo study_main_menu[] = {
    GNOMEUIINFO_MENU_FILE_TREE(file_menu),
    GNOMEUIINFO_MENU_EDIT_TREE(edit_menu),
    GNOMEUIINFO_MENU_VIEW_TREE(view_menu),
    GNOMEUIINFO_SUBTREE("_Tools", tools_menu),
    GNOMEUIINFO_MENU_HELP_TREE(ui_common_help_menu),
    GNOMEUIINFO_END
  };


  /* sanity check */
  g_assert(ui_study!=NULL);


  /* fill in some more menu definitions */
  for (i_roi_type = 0; i_roi_type < AMITK_ROI_TYPE_NUM; i_roi_type++) 
    fill_in_menuitem(&(add_roi_menu[i_roi_type]),
		     amitk_roi_menu_names[i_roi_type], 
		     amitk_roi_menu_explanation[i_roi_type],
		     ui_study_cb_add_roi, ui_study);
  fill_in_end(&(add_roi_menu[AMITK_ROI_TYPE_NUM]));

  counter = 0;
  for (i_method = AMITK_IMPORT_METHOD_RAW; i_method < AMITK_IMPORT_METHOD_NUM; i_method++) {
#ifdef AMIDE_LIBMDC_SUPPORT
    if (i_method == AMITK_IMPORT_METHOD_LIBMDC) {
      for (i_libmdc = LIBMDC_GIF; i_libmdc < LIBMDC_NUM_IMPORT_METHODS; i_libmdc++) {
	if (medcon_import_supports(i_libmdc)) {
	  fill_in_menuitem(&(import_specific_menu[counter]),
			   libmdc_menu_names[i_libmdc],
			   libmdc_menu_explanations[i_libmdc],
			   ui_study_cb_import, ui_study);
	  counter++;
	}
      }
    } else 
#endif
      {
	fill_in_menuitem(&(import_specific_menu[counter]),
			 amitk_import_menu_names[i_method],
			 amitk_import_menu_explanations[i_method],
			 ui_study_cb_import, ui_study);
	counter++;
      }
  }
  fill_in_end(&(import_specific_menu[counter]));


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





/* function to setup the toolbar for the study ui */
/* remember, ui_study->study is usually not set by the time this function is called */
void ui_study_toolbar_create(ui_study_t * ui_study) {

  AmitkInterpolation i_interpolation;
  AmitkFuseType i_fuse_type;
  AmitkView i_view;
  AmitkViewMode i_view_mode;
  GtkWidget * label;
  GtkWidget * toolbar;
  GtkObject * adjustment;

  /* the toolbar definitions */
  GnomeUIInfo interpolation_list[AMITK_INTERPOLATION_NUM+1];
  GnomeUIInfo fuse_type_list[AMITK_FUSE_TYPE_NUM+1];
  GnomeUIInfo canvas_visible_list[AMITK_VIEW_NUM+1];
  GnomeUIInfo view_mode_list[AMITK_VIEW_MODE_NUM+1];
  GnomeUIInfo canvas_target[] = {
    GNOMEUIINFO_TOGGLEITEM_DATA(NULL, "Leave crosshairs on canvases", 
				ui_study_cb_target_pressed, ui_study,
				icon_target_xpm),
    GNOMEUIINFO_END
  };

  GnomeUIInfo study_main_toolbar[] = {
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_RADIOLIST(interpolation_list),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_RADIOLIST(fuse_type_list),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_INCLUDE(canvas_target),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_INCLUDE(canvas_visible_list),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_RADIOLIST(view_mode_list),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_ITEM_DATA(NULL,
			  "Set the thresholds and colormaps for the active data set",
			  ui_study_cb_threshold_pressed,
			  ui_study, icon_threshold_xpm),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_END
  };

  /* sanity check */
  g_assert(ui_study!=NULL);
  
  /* start make the interpolation toolbar items*/
  for (i_interpolation = 0; i_interpolation < AMITK_INTERPOLATION_NUM; i_interpolation++) 
    fill_in_radioitem(&(interpolation_list[i_interpolation]),
		      (icon_interpolation[i_interpolation] == NULL) ? 
		      amitk_interpolation_get_name(i_interpolation) : NULL,
		      amitk_interpolation_explanations[i_interpolation],
		      ui_study_cb_interpolation,
		      ui_study, 
		      icon_interpolation[i_interpolation]);
  fill_in_end(&(interpolation_list[AMITK_INTERPOLATION_NUM]));

  /* start make the fuse_type toolbar items*/
  for (i_fuse_type = 0; i_fuse_type < AMITK_FUSE_TYPE_NUM; i_fuse_type++) 
    fill_in_radioitem(&(fuse_type_list[i_fuse_type]),
		      (icon_fuse_type[i_fuse_type] == NULL) ? 
		      amitk_fuse_type_get_name(i_fuse_type) : NULL,
		      amitk_fuse_type_explanations[i_fuse_type],
		      ui_study_cb_fuse_type,
		      ui_study, 
		      icon_fuse_type[i_fuse_type]);
  fill_in_end(&(fuse_type_list[AMITK_INTERPOLATION_NUM]));

  /* and the views */
  for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view++)
    fill_in_toggleitem(&(canvas_visible_list[i_view]),
		       (icon_view[i_view] == NULL) ? 
		       amitk_view_get_name(i_view) : NULL,
		       amitk_view_get_name(i_view),
		       ui_study_cb_canvas_visible,
		       ui_study,
		       icon_view[i_view]);
  fill_in_end(&(canvas_visible_list[AMITK_VIEW_NUM]));
  
  /* and the view modes */
  for (i_view_mode = 0; i_view_mode < AMITK_VIEW_MODE_NUM; i_view_mode++)
    fill_in_radioitem(&(view_mode_list[i_view_mode]),
		      (icon_view_mode[i_view_mode] == NULL) ? 
		      view_mode_names[i_view_mode] : NULL,
		      view_mode_explanations[i_view_mode],
		      ui_study_cb_view_mode,
		      ui_study, 
		      icon_view_mode[i_view_mode]);
  fill_in_end(&(view_mode_list[AMITK_VIEW_MODE_NUM]));


  /* make the toolbar */
  toolbar = gtk_toolbar_new();
  gnome_app_fill_toolbar(GTK_TOOLBAR(toolbar), study_main_toolbar, NULL);

  ui_study->canvas_target_button = canvas_target[0].widget;


  /* finish setting up the interpolation items */
  for (i_interpolation = 0; i_interpolation < AMITK_INTERPOLATION_NUM; i_interpolation++) {
    ui_study->interpolation_button[i_interpolation] = interpolation_list[i_interpolation].widget;
    g_object_set_data(G_OBJECT(ui_study->interpolation_button[i_interpolation]), 
		      "interpolation", GINT_TO_POINTER(i_interpolation));
    gtk_widget_set_sensitive(ui_study->interpolation_button[i_interpolation], FALSE);
  }
  

  /* finish setting up the fuse types items */
  for (i_fuse_type = 0; i_fuse_type < AMITK_FUSE_TYPE_NUM; i_fuse_type++) {
    ui_study->fuse_type_button[i_fuse_type] = fuse_type_list[i_fuse_type].widget;
    g_object_set_data(G_OBJECT(fuse_type_list[i_fuse_type].widget), 
		      "fuse_type", GINT_TO_POINTER(i_fuse_type));
  }

  /* and the view visible buttons */
  for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view++) {
    ui_study->canvas_visible_button[i_view] = canvas_visible_list[i_view].widget;
    g_object_set_data(G_OBJECT(canvas_visible_list[i_view].widget), 
		      "view", GINT_TO_POINTER(i_view));
  }
  

  /* and the view modes */
  for (i_view_mode = 0; i_view_mode < AMITK_VIEW_MODE_NUM; i_view_mode++) {
    ui_study->view_mode_button[i_view_mode] = view_mode_list[i_view_mode].widget;
    g_object_set_data(G_OBJECT(view_mode_list[i_view_mode].widget), 
		      "view_mode", GINT_TO_POINTER(i_view_mode));
  }
  
  /* add the zoom widget to our toolbar */
  label = gtk_label_new("zoom:");
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), label, NULL, NULL);
  gtk_widget_show(label);

  adjustment = gtk_adjustment_new(1.0,
				  AMIDE_LIMIT_ZOOM_LOWER,
				  AMIDE_LIMIT_ZOOM_UPPER,
				  AMIDE_LIMIT_ZOOM_STEP, 
				  AMIDE_LIMIT_ZOOM_PAGE,
				  AMIDE_LIMIT_ZOOM_PAGE);
  ui_study->zoom_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 0.25, 3);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(ui_study->zoom_spin),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(ui_study->zoom_spin), FALSE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ui_study->zoom_spin), FALSE);
  gtk_widget_set_size_request (ui_study->zoom_spin, 75, -1);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(ui_study->zoom_spin), GTK_UPDATE_ALWAYS);

  g_signal_connect(G_OBJECT(ui_study->zoom_spin), "value_changed",  
		   G_CALLBACK(ui_study_cb_zoom), ui_study);
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), ui_study->zoom_spin, 
  			    "specify how much to magnify the images", NULL);
  gtk_widget_show(ui_study->zoom_spin);
			      
  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));


  /* add the slice thickness selector */
  label = gtk_label_new("thickness (mm):");
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), label, NULL, NULL);
  gtk_widget_show(label);

  adjustment = gtk_adjustment_new(1.0,0.2,5.0,0.2,0.2, 0.2);
  ui_study->thickness_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment),1.0, 3);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(ui_study->thickness_spin),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(ui_study->thickness_spin), FALSE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ui_study->thickness_spin), FALSE);
  gtk_widget_set_size_request (ui_study->thickness_spin, 75, -1);

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



