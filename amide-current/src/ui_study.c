/* ui_study.c
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

#include "amide_config.h"
#include <string.h>
#include "image.h"
#include "ui_common.h"
#include "ui_study.h"
#include "ui_study_cb.h"
#include "ui_gate_dialog.h"
#include "ui_time_dialog.h"
#include "amitk_tree_view.h"
#include "amitk_canvas.h"
#include "amitk_threshold.h"
#include "amitk_progress_dialog.h"
#include "amitk_common.h"
#include "libmdc_interface.h"


#define HELP_INFO_LINE_HEIGHT 13
#define LEFT_COLUMN_WIDTH 350

/* internal variables */
static gchar * help_info_legends[NUM_HELP_INFO_LINES] = {
  N_("m1"), N_("shift-m1"),
  N_("m2"), N_("shift-m2"),
  N_("m3"), N_("shift-m3"), N_("ctrl-m3"),
  "variable_place_holder"
};

enum {
  HELP_INFO_VARIABLE_LINE_CTRL_X,
  HELP_INFO_VARIABLE_LINE_SHIFT_CTRL_3,
  NUM_HELP_INFO_VARIABLE_LINES
};

static gchar * help_info_variable_legend[NUM_HELP_INFO_VARIABLE_LINES] = {
  N_("ctrl-x"),
  N_("shift-m3")
};

static gchar * help_info_lines[][NUM_HELP_INFO_LINES] = {
  {"",  "", 
   "",  "",    
   "",  "", "",
   ""}, /* BLANK */
  {N_("move view"), N_("shift data set"),
   N_("move view, min. depth"), "", 
   N_("change depth"), N_("rotate data set"),  N_("add fiducial mark"),
   ""}, /* DATA SET */
  {N_("shift"), "", 
   N_("scale"), "", 
   N_("rotate"), "", N_("set data set inside roi to zero"),
   N_("set data set outside roi to zero")}, /*CANVAS_ROI */
  {N_("shift"),  "", 
   "", "", 
   "", "", "",
   ""}, /*CANVAS_FIDUCIAL_MARK */
  {N_("move view"), "",
   N_("move view, min. depth"), "", 
   N_("change depth"), N_("rotate study"), "",
   ""}, /* STUDY  */
  {N_("shift"), "", 
   N_("enter draw mode"), "",
   N_("start isocontour change"), "", N_("set data set inside roi to zero"),
   N_("set data set outside roi to zero")}, /*CANVAS_ISOCONTOUR_ROI */
  {N_("shift"), "", 
   N_("enter draw mode"), "",
   "", "", N_("set data set inside roi to zero"),
   N_("set data set outside roi to zero")}, /*CANVAS_FREEHAND_ROI */
  {N_("draw point"),N_("draw large point"),
   N_("leave draw mode"), "",
   N_("erase point"), N_("erase large point"), "",
   ""}, /* CANVAS_DRAWING_MODE */
  {N_("move line"), "",
   "", "", 
   N_("rotate line"), "", "",
   ""}, /* CANVAS_LINE_PROFILE */
  {N_("draw - edge-to-edge"), "", 
   N_("draw - center out"), "", 
   "", "", "",
   ""}, /* CANVAS_NEW_ROI */
  {N_("pick isocontour start point"), "", 
   "", "", 
   "", "", "",
   ""}, /* CANVAS_NEW_ISOCONTOUR_ROI */
  {N_("pick freehand drawing start point"), "", 
   "", "", 
   "", "", "",
   ""}, /* CANVAS_NEW_FREEHAND_ROI */
  {N_("cancel"), "", 
   "", "", 
   N_("pick new isocontour"), "", "",
   ""}, /*CANVAS CHANGE ISOCONTOUR */
  {N_("cancel"), "", 
   "", "", 
   N_("shift"), "", "",
   ""}, /*CANVAS SHIFT OBJECT */
  {N_("cancel"), "", 
   "", "", 
   N_("rotate"), "", "",
   ""}, /*CANVAS ROTATE OBJECT */
  {N_("select data set"), "", 
   N_("make active"), "", 
   N_("pop up data set dialog"), N_("add roi"), N_("add fiducial mark"),
   N_("delete data set")}, /* TREE_DATA_SET */
  {N_("select roi"), "", 
   N_("center view on roi"), "", 
   N_("pop up roi dialog"), "", "",
   N_("delete roi")}, /* TREE_ROI */
  {N_("select point"), "", 
   N_("center view on point"), "", 
   N_("pop up point dialog"), "", "",
   N_("delete mark")}, /* TREE_FIDUCIAL_MARK */
  {"", "", 
   N_("make active"), "", 
   N_("pop up study dialog"),N_("add roi"), "",
   ""}, /* TREE_STUDY */
  {"", "", 
   "", "", 
   N_("add roi"),"", "",
   ""} /* TREE_NONE */
};

static void update_interpolation_and_rendering(ui_study_t * ui_study);
static void object_selection_changed_cb(AmitkObject * object, gpointer ui_study);
static void study_name_changed_cb(AmitkObject * object, gpointer ui_study);
static void object_add_child_cb(AmitkObject * parent, AmitkObject * child, gpointer ui_study);
static void object_remove_child_cb(AmitkObject * parent, AmitkObject * child, gpointer ui_study);
static void add_object(ui_study_t * ui_study, AmitkObject * object);
static void remove_object(ui_study_t * ui_study, AmitkObject * object);
static void menus_toolbar_create(ui_study_t * ui_study);


/* updates the settings of the interpolation radio button/rendering combo box, will not change canvas */
static void update_interpolation_and_rendering(ui_study_t * ui_study) {

  AmitkInterpolation i_interpolation;
  AmitkInterpolation interpolation;

  
  if (AMITK_IS_STUDY(ui_study->active_object)) {
    for (i_interpolation = 0; i_interpolation < AMITK_INTERPOLATION_NUM; i_interpolation++)
      gtk_action_set_sensitive(ui_study->interpolation_action[i_interpolation], FALSE);
    gtk_widget_set_sensitive(ui_study->rendering_menu, FALSE);
  } else if (AMITK_IS_DATA_SET(ui_study->active_object)) {
    for (i_interpolation = 0; i_interpolation < AMITK_INTERPOLATION_NUM; i_interpolation++)
      gtk_action_set_sensitive(ui_study->interpolation_action[i_interpolation], TRUE);
    gtk_widget_set_sensitive(ui_study->rendering_menu, TRUE);


    /* block signals, as we only want to change the value, it's up to the caller of this
       function to change anything on the actual canvases... 
       we'll unblock at the end of this function */
    for (i_interpolation = 0; i_interpolation < AMITK_INTERPOLATION_NUM; i_interpolation++) 
      g_signal_handlers_block_by_func(G_OBJECT(ui_study->interpolation_action[i_interpolation]),
				      G_CALLBACK(ui_study_cb_interpolation), ui_study);
    g_signal_handlers_block_by_func(G_OBJECT(ui_study->rendering_menu),
				    G_CALLBACK(ui_study_cb_rendering), ui_study);

    interpolation = AMITK_DATA_SET_INTERPOLATION(ui_study->active_object);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(ui_study->interpolation_action[interpolation]),
				 TRUE);
    gtk_combo_box_set_active(GTK_COMBO_BOX(ui_study->rendering_menu), 
			     AMITK_DATA_SET_RENDERING(ui_study->active_object));

    for (i_interpolation = 0; i_interpolation < AMITK_INTERPOLATION_NUM; i_interpolation++)
      g_signal_handlers_unblock_by_func(G_OBJECT(ui_study->interpolation_action[i_interpolation]),
					G_CALLBACK(ui_study_cb_interpolation),  ui_study);
    g_signal_handlers_unblock_by_func(G_OBJECT(ui_study->rendering_menu),
				      G_CALLBACK(ui_study_cb_rendering), ui_study);

  }

}


static void object_selection_changed_cb(AmitkObject * object, gpointer data) {

  ui_study_t * ui_study = data;

  if (AMITK_IS_DATA_SET(object)) {
    if (ui_study->time_dialog != NULL)
      ui_time_dialog_set_times(ui_study->time_dialog);

    if (AMITK_IS_STUDY(ui_study->active_object) &&
	amitk_object_get_selected(object, AMITK_SELECTION_ANY))
      ui_study_make_active_object(ui_study, object);
  }

  return;
}

static void study_name_changed_cb(AmitkObject * object, gpointer data) {

  ui_study_t * ui_study = data;

  if (AMITK_IS_STUDY(object)) {
    ui_study_update_title(ui_study);
  }

  return;
}

static void object_add_child_cb(AmitkObject * parent, AmitkObject * child, gpointer data ) {

  ui_study_t * ui_study = data;
  amide_real_t vox_size;


  g_return_if_fail(AMITK_IS_OBJECT(child));
  add_object(ui_study, child);

  /* reset the view thickness if indicated */
  if (AMITK_IS_DATA_SET(child)) {
    vox_size = amitk_data_sets_get_min_voxel_size(AMITK_OBJECT_CHILDREN(ui_study->study));
    amitk_study_set_view_thickness(ui_study->study, vox_size);
  }
  return;
}

static void object_remove_child_cb(AmitkObject * parent, AmitkObject * child, gpointer data) {

  ui_study_t * ui_study = data;

  g_return_if_fail(AMITK_IS_OBJECT(child));
  remove_object(ui_study, child);

  return;
}
  

static void add_object(ui_study_t * ui_study, AmitkObject * object) {

  GList * children;
  AmitkViewMode i_view_mode;
  AmitkView i_view;

  amitk_object_ref(object); /* add a reference */

  if (AMITK_IS_STUDY(object)) { /* save a ref to a study object */
    if (ui_study->study != NULL) 
      remove_object(ui_study, AMITK_OBJECT(ui_study->study));
    ui_study->study = AMITK_STUDY(object);
    ui_study->active_object = AMITK_OBJECT(ui_study->study);
    ui_study->study_virgin=FALSE;

    /* set any settings we can */
    ui_study_update_thickness(ui_study, AMITK_STUDY_VIEW_THICKNESS(object));
    ui_study_update_zoom(ui_study);
    ui_study_update_fov(ui_study);
    ui_study_update_canvas_target(ui_study);
    ui_study_update_title(ui_study);
    ui_study_update_time_button(ui_study);
    ui_study_update_layout(ui_study);
    ui_study_update_canvas_visible_buttons(ui_study);
    ui_study_update_fuse_type(ui_study);
    ui_study_update_view_mode(ui_study);

    amitk_tree_view_set_study(AMITK_TREE_VIEW(ui_study->tree_view), AMITK_STUDY(object));
    amitk_tree_view_expand_object(AMITK_TREE_VIEW(ui_study->tree_view), object);
    for (i_view_mode = 0; i_view_mode <= AMITK_STUDY_VIEW_MODE(ui_study->study); i_view_mode++) 
      for (i_view=0; i_view< AMITK_VIEW_NUM; i_view++)
	if (AMITK_STUDY_CANVAS_VISIBLE(object, i_view))
	  amitk_canvas_set_study(AMITK_CANVAS(ui_study->canvas[i_view_mode][i_view]), AMITK_STUDY(object));

    g_signal_connect(G_OBJECT(object), "time_changed", G_CALLBACK(ui_study_cb_study_changed), ui_study);
    g_signal_connect(G_OBJECT(object), "filename_changed", G_CALLBACK(study_name_changed_cb), ui_study);
    g_signal_connect(G_OBJECT(object), "thickness_changed",  G_CALLBACK(ui_study_cb_thickness_changed), ui_study);
    g_signal_connect(G_OBJECT(object), "object_name_changed", G_CALLBACK(study_name_changed_cb), ui_study);
    g_signal_connect(G_OBJECT(object), "canvas_visible_changed", G_CALLBACK(ui_study_cb_canvas_layout_changed), ui_study);
    g_signal_connect(G_OBJECT(object), "view_mode_changed", G_CALLBACK(ui_study_cb_canvas_layout_changed), ui_study);
    g_signal_connect(G_OBJECT(object), "canvas_target_changed", G_CALLBACK(ui_study_cb_study_changed), ui_study);
    g_signal_connect(G_OBJECT(object), "canvas_layout_preference_changed", G_CALLBACK(ui_study_cb_canvas_layout_changed), ui_study);
    g_signal_connect(G_OBJECT(object), "panel_layout_preference_changed", G_CALLBACK(ui_study_cb_canvas_layout_changed), ui_study);
    g_signal_connect(G_OBJECT(object), "voxel_dim_or_zoom_changed", G_CALLBACK(ui_study_cb_voxel_dim_or_zoom_changed), ui_study);
    g_signal_connect(G_OBJECT(object), "fov_changed", G_CALLBACK(ui_study_cb_fov_changed), ui_study);

  } else if (AMITK_IS_DATA_SET(object)) {
    amitk_tree_view_expand_object(AMITK_TREE_VIEW(ui_study->tree_view), AMITK_OBJECT_PARENT(object));

    /* see if we should reset the study name */
    if (AMITK_DATA_SET_SUBJECT_NAME(object) != NULL)
      amitk_study_suggest_name(ui_study->study, AMITK_DATA_SET_SUBJECT_NAME(object));
    else
      amitk_study_suggest_name(ui_study->study, AMITK_OBJECT_NAME(object));

    if (ui_study->study_altered != TRUE) {
      ui_study->study_altered=TRUE;
      ui_study->study_virgin=FALSE;
      ui_study_update_title(ui_study);
    }
  }

  g_signal_connect(G_OBJECT(object), "object_selection_changed", G_CALLBACK(object_selection_changed_cb), ui_study);
  g_signal_connect(G_OBJECT(object), "object_add_child", G_CALLBACK(object_add_child_cb), ui_study);
  g_signal_connect(G_OBJECT(object), "object_remove_child", G_CALLBACK(object_remove_child_cb), ui_study);


  /* add children */
  children = AMITK_OBJECT_CHILDREN(object);
  while (children != NULL) {
      add_object(ui_study, children->data);
      children = children->next;
  }

  return;
}

static void remove_object(ui_study_t * ui_study, AmitkObject * object) {

  GList * children;

  /* recursive remove children */
  children = AMITK_OBJECT_CHILDREN(object);
  while (children != NULL) {
    remove_object(ui_study, children->data);
    children = children->next;
  }

  /* disconnect the object's signals */
  if (AMITK_IS_STUDY(object)) {
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), G_CALLBACK(ui_study_cb_study_changed), ui_study);
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), G_CALLBACK(ui_study_cb_thickness_changed), ui_study);
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), G_CALLBACK(ui_study_cb_canvas_layout_changed), ui_study);
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), G_CALLBACK(study_name_changed_cb), ui_study);
  }

  g_signal_handlers_disconnect_by_func(G_OBJECT(object), G_CALLBACK(object_selection_changed_cb), ui_study);
  g_signal_handlers_disconnect_by_func(G_OBJECT(object), G_CALLBACK(object_add_child_cb), ui_study);
  g_signal_handlers_disconnect_by_func(G_OBJECT(object), G_CALLBACK(object_remove_child_cb), ui_study);
  
  /* close down the object's dialog if it's up */
  if (object->dialog != NULL) {
    gtk_widget_destroy(GTK_WIDGET(object->dialog));
    object->dialog = NULL;
  }

  /* and unref */
  amitk_object_unref(object);

  return;
}






static const GtkActionEntry normal_items[] = {
  /* Toplevel */
  { "FileMenu", NULL, N_("_File") },
  { "EditMenu", NULL, N_("_Edit") },
  { "ViewMenu", NULL, N_("_View") },
  { "ToolsMenu", NULL, N_("_Tools") },
  { "HelpMenu", NULL, N_("_Help") },

  /* submenus */
  { "ImportSpecificMenu", NULL, N_("Import File (_specify)")},
  //N_("Import an image data file into this study, specifying the import type"), 
  { "ExportView", NULL, N_("_Export View")},
  //N_("Export one of the views to a picture file")
  { "AddRoi", NULL, N_("Add _ROI")},
  //N_("Add a new ROI"),
#if (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)
  { "FlyThrough",NULL,N_("Generate _Fly Through")},
  //N_("generate an mpeg fly through of the data sets")
#endif
  
  /* FileMenu */
  { "NewStudy",         GTK_STOCK_NEW,     N_("_New Study"), NULL, N_("Create a new study viewer window"), G_CALLBACK(ui_study_cb_new_study)},
  { "OpenXIFFile",      GTK_STOCK_OPEN,    N_("_Open Study"), NULL, N_("Open a previously saved study (XIF file)"), G_CALLBACK(ui_study_cb_open_xif_file)},
  { "SaveAsXIFFile",    GTK_STOCK_SAVE_AS, N_("Save Study As"), NULL, N_("Save current study (as a XIF file)"), G_CALLBACK(ui_study_cb_save_as_xif_file)},
  { "ImportGuess",      NULL,              N_("Import File (guess)"), NULL, N_("Import an image data file into this study, guessing at the file type"),G_CALLBACK(ui_study_cb_import)},
  { "ImportObject",     NULL,              N_("Import _Object from Study"),NULL, N_("Import an object, such as an ROI, from a preexisting study (XIF file)"),G_CALLBACK(ui_study_cb_import_object_from_xif_file)},
  { "ExportDataSet",    NULL,              N_("Export _Data Set"),NULL,N_("Export data set(s) to a medical image format"),G_CALLBACK(ui_study_cb_export_data_set)},
  { "RecoverXIFFile",     NULL,              N_("_Recover Study"),NULL,N_("Try to recover a corrupted XIF file"),G_CALLBACK(ui_study_cb_recover_xif_file)},
  { "OpenXIFDir",       NULL,              N_("Open XIF Directory"), NULL, N_("Open a study stored in XIF directory format"), G_CALLBACK(ui_study_cb_open_xif_dir)},
  { "SaveAsXIFDir",     NULL,              N_("Save As XIF Drectory"), NULL, N_("Save a study in XIF directory format"), G_CALLBACK(ui_study_cb_save_as_xif_dir)},
  { "ImportFromXIFDir", NULL,              N_("Import from XIF Directory"),NULL, N_("Import an object, such as an ROI, from a preexisting XIF directory"),G_CALLBACK(ui_study_cb_import_object_from_xif_dir)},
  { "Close",            GTK_STOCK_CLOSE,   NULL, "<control>W", N_("Close the current study"), G_CALLBACK (ui_study_cb_close)},
  { "Quit",             GTK_STOCK_QUIT,    NULL, "<control>Q", N_("Quit AMIDE"), G_CALLBACK (ui_study_cb_quit)},
  
  /* ExportView Submenu */
  { "ExportViewTransverse", NULL, N_("_Transverse"),NULL,N_("Export the current transaxial view to an image file (JPEG/TIFF/PNG/etc.)"),G_CALLBACK(ui_study_cb_export_view)},
  { "ExportViewCoronal",NULL, N_("_Coronal"),NULL,N_("Export the current coronal view to an image file (JPEG/TIFF/PNG/etc.)"),G_CALLBACK(ui_study_cb_export_view)},
  { "ExportViewSagittal",NULL, N_("_Sagittal"),NULL,N_("Export the current sagittal view to an image file (JPEG/TIFF/PNG/etc.)"),G_CALLBACK(ui_study_cb_export_view)},

  /* EditMenu */
  { "AddFiducial", NULL, N_("Add _Fiducial Mark"),NULL,N_("Add a new fiducial mark to the active data set"),G_CALLBACK(ui_study_cb_add_fiducial_mark)},
  { "Preferences", GTK_STOCK_PREFERENCES,NULL, NULL,NULL,G_CALLBACK(ui_study_cb_preferences)},

  /* ViewMenu */
  { "ViewSeries", NULL, N_("_Series"),NULL,N_("Look at a series of images"), G_CALLBACK(ui_study_cb_series)},
#if AMIDE_LIBVOLPACK_SUPPORT
  { "ViewRendering",NULL,N_("_Volume Rendering"),NULL,N_("perform a volume rendering on the currently selected objects"),G_CALLBACK(ui_study_cb_render)},
#endif

  /* ToolsMenu */
  { "AlignmentWizard",NULL,N_("_Alignment Wizard"),NULL,N_("guides you throw the processing of alignment"),G_CALLBACK(ui_study_cb_alignment_selected)},
  { "CropWizard",NULL,N_("_Crop Active Data Set"),NULL,N_("allows you to crop the active data set"),G_CALLBACK(ui_study_cb_crop_selected)},
  { "DistanceWizard",NULL,N_("Distance Measurements"),NULL,N_("calculate distances between fiducial marks and ROIs"),G_CALLBACK(ui_study_cb_distance_selected)},
  { "FactorAnalysisWizard", NULL,N_("_Factor Analysis"),NULL,N_("allows you to do factor analysis of dynamic data on the active data set"),G_CALLBACK(ui_study_cb_fads_selected)},
  { "FilterWizard",NULL,N_("_Filter Active Data Set"),NULL,N_("allows you to filter the active data set"),G_CALLBACK(ui_study_cb_filter_selected)},
  { "LineProfile",NULL,N_("Generate Line _Profile"),NULL,N_("allows generating a line profile between two fiducial marks"),G_CALLBACK(ui_study_cb_profile_selected)},
  { "MathWizard",NULL,N_("Perform _Math on Data Set(s)"),NULL,N_("perform simple math operations on a data set or between data sets"),G_CALLBACK(ui_study_cb_data_set_math_selected)},
  { "RoiStats",NULL,N_("Calculate _ROI Statistics"),NULL,N_("caculate ROI statistics"),G_CALLBACK(ui_study_cb_roi_statistics)},

  /* Flythrough Submenu */
#if (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)
  { "FlyThroughTransverse",NULL,N_("_Transverse"),NULL,N_("Generate a fly through using transaxial slices"),G_CALLBACK(ui_study_cb_fly_through)},
  { "FlyThroughCoronal",NULL,N_("_Coronal"),NULL,N_("Generate a fly through using coronal slices"),G_CALLBACK(ui_study_cb_fly_through)},
  { "FlyThroughSagittal",NULL,N_("_Sagittal"),NULL,N_("Generate a fly through using sagittal slices"),G_CALLBACK(ui_study_cb_fly_through)},
#endif

  /* Toolbar items */
  { "Thresholding", "amide_icon_thresholding", N_("_Threshold"),NULL,N_("Set the thresholds and colormaps for the active data set"), G_CALLBACK(ui_study_cb_thresholding)},
};


static const GtkRadioActionEntry interpolation_radio_entries[AMITK_INTERPOLATION_NUM] = {
  { "InterpolationNearestNeighbor", "amide_icon_interpolation_nearest_neighbor", N_("Near."), NULL,  N_("interpolate using nearest neighbor (fast)"),AMITK_INTERPOLATION_NEAREST_NEIGHBOR},
  { "InterpolationTrilinear", "amide_icon_interpolation_trilinear", N_("Tri."), NULL, N_("interpolate using trilinear interpolation (slow)"), AMITK_INTERPOLATION_TRILINEAR},
};
 
static const GtkRadioActionEntry fuse_type_radio_entries[AMITK_FUSE_TYPE_NUM] = {
  { "FuseTypeBlend", "amide_icon_fuse_type_blend", N_("Blend"), NULL,  N_("blend all data sets"), AMITK_FUSE_TYPE_BLEND },
  { "FuseTypeOverlay", "amide_icon_fuse_type_overlay", N_("Overlay"), NULL, N_("overlay active data set on blended data sets"),AMITK_FUSE_TYPE_OVERLAY },
};

static const GtkRadioActionEntry view_mode_radio_entries[AMITK_VIEW_MODE_NUM] = {
  { "CanvasViewModeSingle", "amide_icon_view_mode_single", N_("Single"), NULL, N_("All objects are shown in a single view"),AMITK_VIEW_MODE_SINGLE },
  { "CanvasViewModeLinked2Way", "amide_icon_view_mode_linked_2way", N_("2-way"), NULL,N_("Objects are shown between 2 linked views"), AMITK_VIEW_MODE_LINKED_2WAY },
  { "CanvasViewModeLinked3Way", "amide_icon_view_mode_linked_3way", N_("3-way"), NULL,N_("Objects are shown between 3 linked views"), AMITK_VIEW_MODE_LINKED_3WAY },
};

 
/* Toggle items */
static const GtkToggleActionEntry toggle_entries[] = {
  { "CanvasTarget", "amide_icon_canvas_target", N_("Target"), NULL, N_("Leave crosshairs on views"), G_CALLBACK(ui_study_cb_canvas_target), FALSE},
  { "CanvasViewTransverse", "amide_icon_view_transverse", N_("Transverse"), NULL, N_("Enable transverse view"), G_CALLBACK(ui_study_cb_canvas_visible), FALSE},
  { "CanvasViewCoronal", "amide_icon_view_coronal", N_("Coronal"), NULL, N_("Enable coronal view"), G_CALLBACK(ui_study_cb_canvas_visible), FALSE},
  { "CanvasViewSagittal", "amide_icon_view_sagittal", N_("Sagittal"), NULL, N_("Enable sagittal view"), G_CALLBACK(ui_study_cb_canvas_visible), FALSE},
};

static const char *ui_description =
"<ui>"
"  <menubar name='MainMenu'>"
"    <menu action='FileMenu'>"
"      <menuitem action='NewStudy'/>"
"      <menuitem action='OpenXIFFile'/>"
"      <menuitem action='SaveAsXIFFile'/>"
"      <separator/>"
"      <menuitem action='ImportGuess'/>"
"      <menu action='ImportSpecificMenu'>"
  /* filled in the function */
"      </menu>"
"      <menuitem action='ImportObject'/>"
"      <separator/>"
"      <menu action='ExportView'>"
"               <menuitem action='ExportViewTransverse'/>"
"               <menuitem action='ExportViewCoronal'/>"
"               <menuitem action='ExportViewSagittal'/>"
"      </menu>"
"      <menuitem action='ExportDataSet'/>"
"      <separator/>"
"      <menuitem action='RecoverXIFFile'/>"
"      <menuitem action='OpenXIFDir'/>"
"      <menuitem action='SaveAsXIFDir'/>"
"      <menuitem action='ImportFromXIFDir'/>"
"      <separator/>"
"      <menuitem action='Close'/>"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='EditMenu'>"
"      <menu action='AddRoi'>"
  /* filled in the function */
"      </menu>"
"      <menuitem action='AddFiducial'/>"
"      <separator/>"
"      <menuitem action='Preferences'/>"
"    </menu>"
"    <menu action='ViewMenu'>"
"       <menuitem action='ViewSeries'/>"
#if AMIDE_LIBVOLPACK_SUPPORT
"       <menuitem action='ViewRendering'/>"
#endif
"    </menu>"
"    <menu action='ToolsMenu'>"
"       <menuitem action='AlignmentWizard'/>"
"       <menuitem action='CropWizard'/>"
"       <menuitem action='DistanceWizard'/>"
"       <menuitem action='FactorAnalysisWizard'/>"
"       <menuitem action='FilterWizard'/>"
#if (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)
"       <menu action='FlyThrough'>"
"          <menuitem action='FlyThroughTransverse'/>"
"          <menuitem action='FlyThroughCoronal'/>"
"          <menuitem action='FlyThroughSagittal'/>"
"       </menu>"
#endif
"       <menuitem action='LineProfile'/>"
"       <menuitem action='MathWizard'/>"
"       <menuitem action='RoiStats'/>"
"    </menu>"
HELP_MENU_UI_DESCRIPTION
"  </menubar>"
"  <toolbar name='ToolBar'>"
"    <toolitem action='InterpolationNearestNeighbor'/>"
"    <toolitem action='InterpolationTrilinear'/>"
"    <placeholder name='RenderingCombo'/>"
"    <separator/>"
"    <toolitem action='FuseTypeBlend'/>"
"    <toolitem action='FuseTypeOverlay'/>"
"    <separator/>"
"    <toolitem action='CanvasTarget'/>"
"    <separator/>"
"    <toolitem action='CanvasViewTransverse'/>"
"    <toolitem action='CanvasViewCoronal'/>"
"    <toolitem action='CanvasViewSagittal'/>"
"    <separator/>"
"    <toolitem action='CanvasViewModeSingle'/>"
"    <toolitem action='CanvasViewModeLinked2Way'/>"
"    <toolitem action='CanvasViewModeLinked3Way'/>"
"    <separator/>"
"    <toolitem action='Thresholding'/>"
  /*"    <separator/>"  */
  /* "    <placeholder name='Zoom' />" */
"  </toolbar>"
"</ui>";



/* function to setup the menus for the study ui */
static void menus_toolbar_create(ui_study_t * ui_study) {

  GtkWidget *menubar;
  GtkWidget *toolbar;
  GtkActionGroup *action_group;
  GtkUIManager *ui_manager;
  GtkAccelGroup *accel_group;
  GError *error;
  GtkWidget * label;
  AmitkImportMethod i_import_method;
  AmitkRendering i_rendering;
  GtkAction * action;
  GtkWidget * menu;
  GtkWidget * submenu;
  GtkWidget * menu_item;
#ifdef AMIDE_LIBMDC_SUPPORT
  libmdc_import_t i_libmdc_import;
#endif
  AmitkRoiType i_roi_type;
  GtkObject * adjustment;
  GtkWidget * placeholder;
  
  g_assert(ui_study!=NULL); /* sanity check */

  /* create an action group with all the menu actions */
  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE);

  gtk_action_group_add_actions(action_group, normal_items, G_N_ELEMENTS(normal_items),ui_study);
  gtk_action_group_add_actions(action_group, ui_common_help_menu_items, G_N_ELEMENTS(ui_common_help_menu_items),ui_study);
  gtk_action_group_add_toggle_actions(action_group, toggle_entries, G_N_ELEMENTS (toggle_entries), ui_study);
  gtk_action_group_add_radio_actions(action_group, interpolation_radio_entries, G_N_ELEMENTS (interpolation_radio_entries), 
				     0, G_CALLBACK(ui_study_cb_interpolation), ui_study);
  gtk_action_group_add_radio_actions(action_group, fuse_type_radio_entries, G_N_ELEMENTS (fuse_type_radio_entries), 
				     0, G_CALLBACK(ui_study_cb_fuse_type), ui_study);
  gtk_action_group_add_radio_actions(action_group, view_mode_radio_entries, G_N_ELEMENTS (view_mode_radio_entries), 
				     0, G_CALLBACK(ui_study_cb_view_mode), ui_study);

  /* create the ui manager, and add the actions and accel's */
  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (ui_study->window, accel_group);

  /* create the actual menu/toolbar ui */
  error = NULL;
  if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, &error)) {
    g_warning ("%s: building menus failed in %s: %s", PACKAGE, __FILE__, error->message);
    g_error_free (error);
    return;
  }


  /* set additional info so we can tell the menus apart */
  g_object_set_data(G_OBJECT(gtk_action_group_get_action (action_group, "ExportViewTransverse")),
		    "view", GINT_TO_POINTER(AMITK_VIEW_TRANSVERSE));
  g_object_set_data(G_OBJECT(gtk_action_group_get_action (action_group, "ExportViewCoronal")),
		    "view", GINT_TO_POINTER(AMITK_VIEW_CORONAL));
  g_object_set_data(G_OBJECT(gtk_action_group_get_action (action_group, "ExportViewSagittal")),
		    "view", GINT_TO_POINTER(AMITK_VIEW_SAGITTAL));
#if (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)
  g_object_set_data(G_OBJECT(gtk_action_group_get_action (action_group, "FlyThroughTransverse")),
		    "view", GINT_TO_POINTER(AMITK_VIEW_TRANSVERSE));
  g_object_set_data(G_OBJECT(gtk_action_group_get_action (action_group, "FlyThroughCoronal")),
		    "view", GINT_TO_POINTER(AMITK_VIEW_CORONAL));
  g_object_set_data(G_OBJECT(gtk_action_group_get_action (action_group, "FlyThroughSagittal")),
		    "view", GINT_TO_POINTER(AMITK_VIEW_SAGITTAL));
#endif

  /* build the import menu */
  submenu = gtk_menu_new();
  for (i_import_method = AMITK_IMPORT_METHOD_RAW; i_import_method < AMITK_IMPORT_METHOD_NUM; i_import_method++) {
#ifdef AMIDE_LIBMDC_SUPPORT
    if (i_import_method == AMITK_IMPORT_METHOD_LIBMDC) {
      for (i_libmdc_import = 0; i_libmdc_import < LIBMDC_NUM_IMPORT_METHODS; i_libmdc_import++) {
  	if (libmdc_supports(libmdc_import_to_format[i_libmdc_import])) {
	  menu_item = gtk_menu_item_new_with_mnemonic(libmdc_import_menu_names[i_libmdc_import]);
	  /* if tooltip support existed - libmdc_import_menu_explanations[i_libmdc_import] */
	  gtk_menu_shell_append(GTK_MENU_SHELL(submenu), menu_item);
  	  g_object_set_data(G_OBJECT(menu_item), "method", GINT_TO_POINTER(i_import_method));
  	  g_object_set_data(G_OBJECT(menu_item), "submethod", GINT_TO_POINTER(libmdc_import_to_format[i_libmdc_import]));
	  g_signal_connect(G_OBJECT(menu_item), "activate",  G_CALLBACK(ui_study_cb_import), ui_study);
	  gtk_widget_show(menu_item);
	}
      }
    } else 
#endif
      {
	menu_item = gtk_menu_item_new_with_mnemonic(amitk_import_menu_names[i_import_method]);
	/* if tooltip support existed - amitk_import_menu_explanations[i_import_method] */
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), menu_item);
  	g_object_set_data(G_OBJECT(menu_item), "method", GINT_TO_POINTER(i_import_method));
  	g_object_set_data(G_OBJECT(menu_item), "submethod", GINT_TO_POINTER(0));
	g_signal_connect(G_OBJECT(menu_item), "activate",  G_CALLBACK(ui_study_cb_import), ui_study);
	gtk_widget_show(menu_item);
      }
  }
  menu = gtk_ui_manager_get_widget(ui_manager, "ui/MainMenu/FileMenu/ImportSpecificMenu");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu), submenu);
  gtk_widget_show(submenu);
  gtk_widget_show(menu);

  /* build the add roi menu */
  submenu = gtk_menu_new();
  for (i_roi_type=0; i_roi_type<AMITK_ROI_TYPE_NUM; i_roi_type++) {
    menu_item = gtk_menu_item_new_with_mnemonic(_(amitk_roi_menu_names[i_roi_type]));
    /* if tooltip support existed - amitk_roi_menu_explanation[i_roi_type] */
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu), menu_item);
    g_object_set_data(G_OBJECT(menu_item), "roi_type", GINT_TO_POINTER(i_roi_type)); 
    g_signal_connect(G_OBJECT(menu_item), "activate",  G_CALLBACK(ui_study_cb_add_roi), ui_study);
    gtk_widget_show(menu_item);
  }
  menu = gtk_ui_manager_get_widget(ui_manager, "ui/MainMenu/EditMenu/AddRoi");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu), submenu);
  gtk_widget_show(submenu);
  gtk_widget_show(menu);

  /* record the radio/toggle items so that we can change which one is depressed later */
  ui_study->interpolation_action[AMITK_INTERPOLATION_NEAREST_NEIGHBOR] = 
    gtk_action_group_get_action(action_group, "InterpolationNearestNeighbor");
  ui_study->interpolation_action[AMITK_INTERPOLATION_TRILINEAR] = 
    gtk_action_group_get_action(action_group, "InterpolationTrilinear");
  g_assert(AMITK_INTERPOLATION_TRILINEAR+1 == AMITK_INTERPOLATION_NUM); /* make sure we handle all types */

  ui_study->fuse_type_action[AMITK_FUSE_TYPE_BLEND] = 
    gtk_action_group_get_action(action_group, "FuseTypeBlend");
  ui_study->fuse_type_action[AMITK_FUSE_TYPE_OVERLAY] = 
    gtk_action_group_get_action(action_group, "FuseTypeOverlay");
  g_assert(AMITK_FUSE_TYPE_OVERLAY+1 == AMITK_FUSE_TYPE_NUM); /* make sure we handle all types */

  ui_study->view_mode_action[AMITK_VIEW_MODE_SINGLE] = 
    gtk_action_group_get_action(action_group, "CanvasViewModeSingle");
  ui_study->view_mode_action[AMITK_VIEW_MODE_LINKED_2WAY] = 
    gtk_action_group_get_action(action_group, "CanvasViewModeLinked2Way");
  ui_study->view_mode_action[AMITK_VIEW_MODE_LINKED_3WAY] = 
    gtk_action_group_get_action(action_group, "CanvasViewModeLinked3Way");
  g_assert(AMITK_VIEW_MODE_LINKED_3WAY+1 == AMITK_VIEW_MODE_NUM); /* make sure we handle all types */

  ui_study->canvas_target_action = 
    gtk_action_group_get_action(action_group, "CanvasTarget");

  action = gtk_action_group_get_action(action_group, "CanvasViewTransverse");
  ui_study->canvas_visible_action[AMITK_VIEW_TRANSVERSE] = action;
  g_object_set_data(G_OBJECT(action), "view", GINT_TO_POINTER(AMITK_VIEW_TRANSVERSE));
    
  action = gtk_action_group_get_action(action_group, "CanvasViewCoronal");
  ui_study->canvas_visible_action[AMITK_VIEW_CORONAL] = action;
  g_object_set_data(G_OBJECT(action), "view", GINT_TO_POINTER(AMITK_VIEW_CORONAL));
    
  action = gtk_action_group_get_action(action_group, "CanvasViewSagittal");
  ui_study->canvas_visible_action[AMITK_VIEW_SAGITTAL] = action;
  g_object_set_data(G_OBJECT(action), "view", GINT_TO_POINTER(AMITK_VIEW_SAGITTAL));

  /* pack in the menu and toolbar */
  menubar = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
  gtk_box_pack_start (GTK_BOX (ui_study->window_vbox), menubar, FALSE, FALSE, 0);

  toolbar = gtk_ui_manager_get_widget (ui_manager, "/ToolBar");
  gtk_box_pack_start (GTK_BOX (ui_study->window_vbox), toolbar, FALSE, FALSE, 0);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_show_arrow(GTK_TOOLBAR(toolbar), FALSE);

  /* insert the rendering menu into the toolbar */
  placeholder = gtk_ui_manager_get_widget (ui_manager, "/ToolBar/RenderingCombo");
  ui_study->rendering_menu = gtk_combo_box_new_text(); 
  /*  gtk_widget_set_tooltip_text(ui_study->rendering_menu, _(amitk_rendering_explanation)); 
      combo box's (as of 2.24 at least, don't have functioning tool tips */

  for (i_rendering = 0; i_rendering < AMITK_RENDERING_NUM; i_rendering++) 
    gtk_combo_box_append_text(GTK_COMBO_BOX(ui_study->rendering_menu), amitk_rendering_get_name(i_rendering));
  g_signal_connect(G_OBJECT(ui_study->rendering_menu), "changed", G_CALLBACK(ui_study_cb_rendering), ui_study);
  ui_common_toolbar_insert_widget(toolbar, ui_study->rendering_menu, "", 
				  gtk_toolbar_get_item_index(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(placeholder)));

  /* and finish off the rest of the toolbar */
  /* starting with a separator for clarity */
  ui_common_toolbar_append_separator(toolbar);

  /* add the zoom widget to our toolbar */
  label = gtk_label_new(_("zoom:"));
  ui_common_toolbar_append_widget(toolbar, label, NULL);

  ui_study->zoom_spin = gtk_spin_button_new_with_range(AMIDE_LIMIT_ZOOM_LOWER, AMIDE_LIMIT_ZOOM_UPPER, AMIDE_LIMIT_ZOOM_STEP);
  gtk_widget_set_size_request(ui_study->zoom_spin, 50,-1);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(ui_study->zoom_spin), 3);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(ui_study->zoom_spin),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(ui_study->zoom_spin), FALSE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ui_study->zoom_spin), FALSE);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(ui_study->zoom_spin), GTK_UPDATE_ALWAYS);
  g_signal_connect(G_OBJECT(ui_study->zoom_spin), "value_changed",G_CALLBACK(ui_study_cb_zoom), ui_study);
  g_signal_connect(G_OBJECT(ui_study->zoom_spin), "output", G_CALLBACK(amitk_spin_button_scientific_output), NULL);
  g_signal_connect(G_OBJECT(ui_study->zoom_spin), "button_press_event",
		   G_CALLBACK(amitk_spin_button_discard_double_or_triple_click), NULL);
  ui_common_toolbar_append_widget(toolbar,ui_study->zoom_spin,_("specify how much to magnify the images"));

  /* a separator for clarity */
  ui_common_toolbar_append_separator(toolbar);

  /* add the field of view widget to our toolbar */
  label = gtk_label_new(_("fov (%):"));
  ui_common_toolbar_append_widget(toolbar, label, NULL);

  ui_study->fov_spin = gtk_spin_button_new_with_range(AMIDE_LIMIT_FOV_LOWER, AMIDE_LIMIT_FOV_UPPER, AMIDE_LIMIT_FOV_STEP);
  gtk_widget_set_size_request(ui_study->fov_spin, 50,-1);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(ui_study->fov_spin), 0);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(ui_study->fov_spin),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(ui_study->fov_spin), FALSE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ui_study->fov_spin), FALSE);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(ui_study->fov_spin), GTK_UPDATE_ALWAYS);
  g_signal_connect(G_OBJECT(ui_study->fov_spin), "value_changed", G_CALLBACK(ui_study_cb_fov), ui_study);
  g_signal_connect(G_OBJECT(ui_study->fov_spin), "button_press_event",
		   G_CALLBACK(amitk_spin_button_discard_double_or_triple_click), NULL);
  ui_common_toolbar_append_widget(toolbar,ui_study->fov_spin,_("specify how much of the image field of view to display"));

  /* a separator for clarity */
  ui_common_toolbar_append_separator(toolbar);

  /* add the slice thickness selector */
  label = gtk_label_new(_("thickness (mm):"));
  ui_common_toolbar_append_widget(toolbar, label, NULL);

  adjustment = gtk_adjustment_new(1.0, 0.2, G_MAXDOUBLE, 0.2, 0.2, 0.0);
  ui_study->thickness_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment),1.0, 3);
  gtk_widget_set_size_request (ui_study->thickness_spin, 60, -1);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(ui_study->thickness_spin),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(ui_study->thickness_spin), FALSE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ui_study->thickness_spin), FALSE);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(ui_study->thickness_spin), GTK_UPDATE_ALWAYS);
  g_signal_connect(G_OBJECT(ui_study->thickness_spin), "value_changed", G_CALLBACK(ui_study_cb_thickness), ui_study);
  g_signal_connect(G_OBJECT(ui_study->thickness_spin), "output", G_CALLBACK(amitk_spin_button_scientific_output), NULL);
  g_signal_connect(G_OBJECT(ui_study->thickness_spin), "button_press_event",
		   G_CALLBACK(amitk_spin_button_discard_double_or_triple_click), NULL);
  ui_common_toolbar_append_widget(toolbar,ui_study->thickness_spin,_("specify how thick to make the slices (mm)"));

  /* a separator for clarity */
  ui_common_toolbar_append_separator(toolbar);

  /* gate */
  /* note, can't use gtk_tool_button, as no way to set the relief in gtk 2.10, and the default
     is no relief so you can't tell that it's a button.... - */
  label = gtk_label_new(_("gate:"));
  ui_common_toolbar_append_widget(toolbar, label, NULL);

  ui_study->gate_button = gtk_button_new_with_label("?");
  g_signal_connect(G_OBJECT(ui_study->gate_button), "clicked", G_CALLBACK(ui_study_cb_gate), ui_study);
  ui_common_toolbar_append_widget(toolbar, ui_study->gate_button,
				  _("the gate range over which to view the data"));


  /* a separator for clarity */
  ui_common_toolbar_append_separator(toolbar);

  /* frame selector */
  label = gtk_label_new(_("time:"));
  ui_common_toolbar_append_widget(toolbar, label, NULL);

  ui_study->time_button = gtk_button_new_with_label("?"); 
  g_signal_connect(G_OBJECT(ui_study->time_button), "clicked", G_CALLBACK(ui_study_cb_time), ui_study);
  ui_common_toolbar_append_widget(toolbar, ui_study->time_button,
				  _("the time range over which to view the data (s)"));

  return;
}



/* destroy a ui_study data structure */
ui_study_t * ui_study_free(ui_study_t * ui_study) {

  gboolean return_val;

  if (ui_study == NULL)
    return ui_study;

  /* sanity checks */
  g_return_val_if_fail(ui_study->reference_count > 0, NULL);

  /* remove a reference count */
  ui_study->reference_count--;

  /* if we've removed all reference's, free the structure */
  if (ui_study->reference_count == 0) {

    /* these two lines forces any remaining spin button updates, so that we
       don't call any spin button callbacks with invalid data */
    gtk_widget_grab_focus(GTK_WIDGET(ui_study->window));
    while (gtk_events_pending()) gtk_main_iteration();

#ifdef AMIDE_DEBUG
    g_print("freeing ui_study\n");
#endif
    if (ui_study->study != NULL) {
      remove_object(ui_study, AMITK_OBJECT(ui_study->study));
      ui_study->study = NULL;
    }

    if (ui_study->progress_dialog != NULL) {
      g_signal_emit_by_name(G_OBJECT(ui_study->progress_dialog), "delete_event", NULL, &return_val);
      ui_study->progress_dialog = NULL;
    }

    if (ui_study->preferences != NULL) {
      g_object_unref(ui_study->preferences);
      ui_study->preferences = NULL;
    }

    g_free(ui_study);
    ui_study = NULL;
  }
    
  return ui_study;
}

/* allocate and initialize a ui_study data structure */
ui_study_t * ui_study_init(AmitkPreferences * preferences) {

  ui_study_t * ui_study;
  AmitkViewMode i_view_mode;
  AmitkView i_view;
  help_info_line_t i_line;

  /* alloc space for the data structure for passing ui info */
  ui_study = g_try_new(ui_study_t,1);
  g_return_val_if_fail(ui_study != NULL, NULL);

  ui_study->reference_count = 1;

  ui_study->study = NULL;
  ui_study->threshold_dialog = NULL;
  ui_study->gate_dialog = NULL;
  ui_study->time_dialog = NULL;
  ui_study->thickness_spin = NULL;

  ui_study->active_object = NULL;
  for (i_view_mode=0; i_view_mode < AMITK_VIEW_MODE_NUM; i_view_mode++) {
    ui_study->canvas_table[i_view_mode] = NULL;
    for (i_view=0; i_view < AMITK_VIEW_NUM; i_view++) {
      ui_study->canvas[i_view_mode][i_view] = NULL;
    }
  }

  ui_study->study_altered=FALSE;
  ui_study->study_virgin=TRUE;
  
  for (i_line=0 ;i_line < NUM_HELP_INFO_LINES;i_line++) {
    ui_study->help_line[i_line] = NULL;
    ui_study->help_legend[i_line] = NULL;
  }

  ui_study->preferences = g_object_ref(preferences);

  return ui_study;
}



/* if object is NULL, it'll do its best guess */
void ui_study_make_active_object(ui_study_t * ui_study, AmitkObject * object) {

  AmitkView i_view;
  GList * current_objects;
  AmitkViewMode i_view_mode;

  g_return_if_fail(ui_study->study != NULL);
  g_return_if_fail(ui_study->active_object != NULL); 

  if (AMITK_IS_DATA_SET(ui_study->active_object)) {
    g_signal_handlers_disconnect_by_func(G_OBJECT(ui_study->active_object),
					 G_CALLBACK(update_interpolation_and_rendering), ui_study);
    g_signal_handlers_disconnect_by_func(G_OBJECT(ui_study->active_object),
					 G_CALLBACK(ui_study_update_gate_button), ui_study);
  }
  ui_study->active_object = object;

  if (object == NULL) { /* guessing */
    /* find visible data set to make active object */
    for (i_view_mode = 0; 
	 (i_view_mode <= AMITK_STUDY_VIEW_MODE(ui_study->study)) && (ui_study->active_object == NULL);
	 i_view_mode++) {

      current_objects = amitk_object_get_selected_children_of_type(AMITK_OBJECT(ui_study->study),
								   AMITK_OBJECT_TYPE_DATA_SET,
								   AMITK_VIEW_MODE_SINGLE+i_view_mode, 
								   TRUE);

      if (current_objects != NULL) {
	ui_study->active_object = AMITK_OBJECT(current_objects->data);
	amitk_objects_unref(current_objects);
      }

    }
    if (ui_study->active_object == NULL) /* study as backup */
      ui_study->active_object = AMITK_OBJECT(ui_study->study); 
  }

  /* indicate this is now the active object */
  amitk_tree_view_set_active_object(AMITK_TREE_VIEW(ui_study->tree_view), ui_study->active_object);

  /* connect any needed signals */
  if (AMITK_IS_DATA_SET(ui_study->active_object)) {
    g_signal_connect_swapped(G_OBJECT(ui_study->active_object), "interpolation_changed", 
			     G_CALLBACK(update_interpolation_and_rendering), ui_study);
    g_signal_connect_swapped(G_OBJECT(ui_study->active_object), "rendering_changed", 
			     G_CALLBACK(update_interpolation_and_rendering), ui_study);
    g_signal_connect_swapped(G_OBJECT(ui_study->active_object), "view_gates_changed", 
			     G_CALLBACK(ui_study_update_gate_button), ui_study);
  }

  update_interpolation_and_rendering(ui_study);

  for (i_view_mode = 0; i_view_mode <=  AMITK_STUDY_VIEW_MODE(ui_study->study); i_view_mode++) 
    for (i_view=0; i_view< AMITK_VIEW_NUM; i_view++)
      if (ui_study->canvas[i_view_mode][i_view] != NULL)
	amitk_canvas_set_active_object(AMITK_CANVAS(ui_study->canvas[i_view_mode][i_view]), 
				       ui_study->active_object);
  

  /* reset the threshold widget based on the current data set */
  if (ui_study->threshold_dialog != NULL) {
    if (AMITK_IS_STUDY(ui_study->active_object)) {
      gtk_widget_destroy(ui_study->threshold_dialog);
      ui_study->threshold_dialog = NULL;
    } else if (AMITK_IS_DATA_SET(ui_study->active_object)) {
      amitk_threshold_dialog_new_data_set(AMITK_THRESHOLD_DIALOG(ui_study->threshold_dialog), 
					  AMITK_DATA_SET(ui_study->active_object));
    }
  }

  if (ui_study->gate_dialog != NULL) {
    if (AMITK_IS_DATA_SET(ui_study->active_object))
      ui_gate_dialog_set_active_data_set(ui_study->gate_dialog, 
					 AMITK_DATA_SET(ui_study->active_object));
    else
      ui_gate_dialog_set_active_data_set(ui_study->gate_dialog, NULL);
  }

  ui_study_update_gate_button(ui_study);
}

/* function for adding a fiducial mark */
void ui_study_add_fiducial_mark(ui_study_t * ui_study, AmitkObject * parent_object,
				gboolean selected, AmitkPoint position) {

  GtkWidget * dialog;
  gint return_val;
  AmitkFiducialMark * new_pt=NULL;
  gchar * temp_string;
  gchar * return_str=NULL;


  g_return_if_fail(AMITK_IS_OBJECT(parent_object));

  temp_string = g_strdup_printf(_("Adding fiducial mark for data set: %s\nEnter the mark's name:"),
				AMITK_OBJECT_NAME(parent_object));
  dialog = ui_common_entry_dialog(ui_study->window, temp_string, &return_str);
  return_val = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
  g_free(temp_string);


  if (return_val == GTK_RESPONSE_OK) {

    new_pt = amitk_fiducial_mark_new();
    amitk_space_copy_in_place(AMITK_SPACE(new_pt), AMITK_SPACE(parent_object));
    amitk_fiducial_mark_set(new_pt, position);
    amitk_object_set_name(AMITK_OBJECT(new_pt), return_str);
    amitk_object_add_child(AMITK_OBJECT(parent_object), AMITK_OBJECT(new_pt));
    amitk_tree_view_expand_object(AMITK_TREE_VIEW(ui_study->tree_view), AMITK_OBJECT(parent_object));
    amitk_object_unref(new_pt); /* don't want an extra ref */

    if (selected)
      amitk_object_set_selected(AMITK_OBJECT(new_pt), TRUE, AMITK_SELECTION_SELECTED_0);

    if (ui_study->study_altered != TRUE) {
      ui_study->study_virgin=FALSE;
      ui_study->study_altered=TRUE;
      ui_study_update_title(ui_study);
    }
  }

  if (return_str != NULL)
    g_free(return_str);

  return;
}


void ui_study_add_roi(ui_study_t * ui_study, AmitkObject * parent_object, AmitkRoiType roi_type) {

  GtkWidget * dialog;
  gint return_val;
  AmitkRoi * roi;
  gchar * temp_string;
  gchar * return_str=NULL;
  AmitkViewMode i_view_mode;

  g_return_if_fail(AMITK_IS_OBJECT(parent_object));

  temp_string = g_strdup_printf(_("Adding ROI to: %s\nEnter ROI Name:"),
				AMITK_OBJECT_NAME(parent_object));
  dialog = ui_common_entry_dialog(ui_study->window, temp_string, &return_str);
  return_val = gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
  g_free(temp_string);


  if (return_val == GTK_RESPONSE_OK) {

    roi = amitk_roi_new(roi_type);
    amitk_object_set_name(AMITK_OBJECT(roi), return_str);
    amitk_object_add_child(parent_object, AMITK_OBJECT(roi));
    amitk_object_unref(roi); /* don't want an extra ref */

    if (AMITK_ROI_UNDRAWN(roi)) /* undrawn roi's selected to begin with*/
      for (i_view_mode = 0; i_view_mode <=  AMITK_STUDY_VIEW_MODE(ui_study->study); i_view_mode++) 
	amitk_object_set_selected(AMITK_OBJECT(roi), TRUE, i_view_mode);
  
    if (ui_study->study_altered != TRUE) {
      ui_study->study_altered=TRUE;
      ui_study->study_virgin=FALSE;
      ui_study_update_title(ui_study);
    }
  }

  if (return_str != NULL)
    g_free(return_str);

  return;
}


void ui_study_update_canvas_visible_buttons(ui_study_t * ui_study) {

  AmitkView i_view;

  for (i_view=0; i_view < AMITK_VIEW_NUM; i_view++) {
    g_signal_handlers_block_by_func(G_OBJECT(ui_study->canvas_visible_action[i_view]),
				    G_CALLBACK(ui_study_cb_canvas_visible), ui_study);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(ui_study->canvas_visible_action[i_view]),
				 AMITK_STUDY_CANVAS_VISIBLE(ui_study->study, i_view));
    g_signal_handlers_unblock_by_func(G_OBJECT(ui_study->canvas_visible_action[i_view]),
				      G_CALLBACK(ui_study_cb_canvas_visible), ui_study);

  }

  return;
}


/* function to update the text in the gate dialog popup widget */
void ui_study_update_gate_button(ui_study_t * ui_study) {

  gchar * temp_string;
  
  if (AMITK_IS_DATA_SET(ui_study->active_object))
    temp_string = g_strdup_printf(_("%d-%d"),
				  AMITK_DATA_SET_VIEW_START_GATE(ui_study->active_object),
				  AMITK_DATA_SET_VIEW_END_GATE(ui_study->active_object));
  else
    temp_string = g_strdup_printf(_("N/A"));

  gtk_button_set_label(GTK_BUTTON(ui_study->gate_button),temp_string);
  g_free(temp_string);

  return;
}

/* function to update the text in the time dialog popup widget */
void ui_study_update_time_button(ui_study_t * ui_study) {

  gchar * temp_string;
  
  temp_string = g_strdup_printf(_("%g-%g s"),
				AMITK_STUDY_VIEW_START_TIME(ui_study->study),
				AMITK_STUDY_VIEW_START_TIME(ui_study->study)+
				AMITK_STUDY_VIEW_DURATION(ui_study->study));
  gtk_button_set_label(GTK_BUTTON(ui_study->time_button),temp_string);
  g_free(temp_string);

  return;
}


/* This function updates the little info box which tells us what the different 
   mouse buttons will do */
void ui_study_update_help_info(ui_study_t * ui_study, AmitkHelpInfo which_info, 
			       AmitkPoint point, amide_data_t value) {

  help_info_line_t i_line;
  gchar * location_text[2];
  gchar * legend;
  AmitkPoint location_p;
  gchar * help_info_line;

  /* put up the help lines... except for the given info types, in which case
     we leave the last one displayed up */
  if ((which_info != AMITK_HELP_INFO_UPDATE_LOCATION) &&
      (which_info != AMITK_HELP_INFO_UPDATE_SHIFT) &&
      (which_info != AMITK_HELP_INFO_UPDATE_THETA)) {
    for (i_line=0; i_line < HELP_INFO_LINE_BLANK;i_line++) {

      /* the line's legend */
      if (strlen(help_info_lines[which_info][i_line]) > 0) {
	if (i_line == HELP_INFO_LINE_VARIABLE) {
	  if ((which_info == AMITK_HELP_INFO_CANVAS_ROI) ||
	      (which_info == AMITK_HELP_INFO_CANVAS_ISOCONTOUR_ROI) ||
	      (which_info == AMITK_HELP_INFO_CANVAS_FREEHAND_ROI)) {
	    legend = _(help_info_variable_legend[HELP_INFO_VARIABLE_LINE_SHIFT_CTRL_3]);
	  } else {
	    legend = _(help_info_variable_legend[HELP_INFO_VARIABLE_LINE_CTRL_X]);
	  }
	} else {
	  legend = _(help_info_legends[i_line]);
	}
      } else {
	legend = "";
      }
      if (ui_study->help_legend[i_line] == NULL) 
	ui_study->help_legend[i_line] = 
	  gnome_canvas_item_new(gnome_canvas_root(ui_study->help_info),
				gnome_canvas_text_get_type(),
				"justification", GTK_JUSTIFY_RIGHT,
				"anchor", GTK_ANCHOR_NORTH_EAST,
				"text", legend,
				"x", (gdouble) 55.0,
				"y", (gdouble) (i_line*HELP_INFO_LINE_HEIGHT),
				"fill_color", "black", 
				"font_desc", amitk_fixed_font_desc, NULL);
      else /* just need to change the text */
	gnome_canvas_item_set(ui_study->help_legend[i_line], "text", legend, NULL);
      
      /* gettext can't handle "" */
      if (g_strcmp0(help_info_lines[which_info][i_line],"") != 0)
	help_info_line = _(help_info_lines[which_info][i_line]);
      else
	help_info_line = help_info_lines[which_info][i_line];

      /* and the button info */
      if (ui_study->help_line[i_line] == NULL) 
	ui_study->help_line[i_line] = 
	  gnome_canvas_item_new(gnome_canvas_root(ui_study->help_info),
				gnome_canvas_text_get_type(),
				"justification", GTK_JUSTIFY_LEFT,
				"anchor", GTK_ANCHOR_NORTH_WEST,
				"text", help_info_line,
				"x", (gdouble) 65.0,
				"y", (gdouble) (i_line*HELP_INFO_LINE_HEIGHT),
				"fill_color", "black", 
				"font_desc", amitk_fixed_font_desc, NULL);
      else /* just need to change the text */
	gnome_canvas_item_set(ui_study->help_line[i_line], "text", 
			      help_info_line, NULL);
    }
  }



  /* update the location information */
  if ((which_info == AMITK_HELP_INFO_UPDATE_LOCATION) || 
      (which_info == AMITK_HELP_INFO_CANVAS_DRAWING_MODE)) {
    location_text[0] = g_strdup_printf(_("[x,y,z] = [% 5.2f,% 5.2f,% 5.2f] mm"), 
				       point.x, point.y, point.z);
    if (!isnan(value))
      location_text[1] = g_strdup_printf(_("value  = % 5.3g"), value);
    else
      location_text[1] = g_strdup_printf(_("value  = none"));
  } else if (which_info == AMITK_HELP_INFO_UPDATE_SHIFT) {
    location_text[0] = g_strdup_printf(_("shift (x,y,z) ="));
    location_text[1] = g_strdup_printf(_("[% 5.2f,% 5.2f,% 5.2f] mm"), 
				     point.x, point.y, point.z);
  } else if (which_info == AMITK_HELP_INFO_UPDATE_THETA) {
    location_text[0] = g_strdup("");
    location_text[1] = g_strdup_printf(_("theta = % 5.3f degrees"), value);

  } else {
    location_p = AMITK_STUDY_VIEW_CENTER(ui_study->study);
    location_text[0] = g_strdup_printf(_("view center (x,y,z) ="));
    location_text[1] = g_strdup_printf(_("[% 5.2f,% 5.2f,% 5.2f] mm"), 
				       location_p.x, location_p.y, location_p.z);
  }

  /* update the location display */
  for (i_line=HELP_INFO_LINE_LOCATION1; i_line <= HELP_INFO_LINE_LOCATION2;i_line++) {
    if (ui_study->help_line[i_line] == NULL) 
      ui_study->help_line[i_line] =
	gnome_canvas_item_new(gnome_canvas_root(ui_study->help_info),
			      gnome_canvas_text_get_type(),
			      "justification", GTK_JUSTIFY_LEFT,
			      "anchor", GTK_ANCHOR_NORTH_WEST,
			      "text", location_text[i_line-HELP_INFO_LINE_LOCATION1],
			      "x", (gdouble) 2.0,
			      "y", (gdouble) (i_line*HELP_INFO_LINE_HEIGHT),
			      "fill_color", "black",
			      "font_desc", amitk_fixed_font_desc, NULL);

    else /* just need to change the text */
      gnome_canvas_item_set(ui_study->help_line[i_line], "text", 
			    location_text[i_line-HELP_INFO_LINE_LOCATION1],  NULL);

    g_free(location_text[i_line-HELP_INFO_LINE_LOCATION1]);
  }

  return;
}


/* updates the settings of the thickness spin button, will not change anything about the canvas */
void ui_study_update_thickness(ui_study_t * ui_study, amide_real_t thickness) {

  amide_real_t min_voxel_size, max_size;

  /* there's no spin button if we don't create the toolbar at this moment */
  if (ui_study->thickness_spin == NULL) return;

  min_voxel_size = amitk_data_sets_get_min_voxel_size(AMITK_OBJECT_CHILDREN(ui_study->study));
  max_size = amitk_volumes_get_max_size(AMITK_OBJECT_CHILDREN(ui_study->study));
    
  if ((min_voxel_size < 0)  || (max_size < 0)) return; /* no valid objects */
  
  /* block signals to the spin button, as we only want to
     change the value of the spin button, it's up to the caller of this
     function to change anything on the actual canvases... we'll 
     unblock at the end of this function */
  g_signal_handlers_block_by_func(G_OBJECT(ui_study->thickness_spin), 
				  G_CALLBACK(ui_study_cb_thickness), ui_study);
  
  /* set the current thickness if it hasn't already been set or if it's no longer valid*/
  if (thickness < min_voxel_size)
    thickness = min_voxel_size;

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_study->thickness_spin), thickness);
  gtk_spin_button_set_increments(GTK_SPIN_BUTTON(ui_study->thickness_spin), min_voxel_size, min_voxel_size);
  gtk_spin_button_set_range(GTK_SPIN_BUTTON(ui_study->thickness_spin), min_voxel_size, max_size);
  gtk_spin_button_configure(GTK_SPIN_BUTTON(ui_study->thickness_spin),NULL, thickness, 
			    gtk_spin_button_get_digits(GTK_SPIN_BUTTON(ui_study->thickness_spin)));
  /* and now, reconnect the signal */
  g_signal_handlers_unblock_by_func(G_OBJECT(ui_study->thickness_spin),
				    G_CALLBACK(ui_study_cb_thickness), ui_study);

  return;
}

/* updates the settings of the zoom spinbutton, will not change anything about the canvas */
void ui_study_update_zoom(ui_study_t * ui_study) {

  /* block signals to the zoom spin button, as we only want to
     change the value of the spin button, it's up to the caller of this
     function to change anything on the actual canvases... we'll 
     unblock at the end of this function */
  g_signal_handlers_block_by_func(G_OBJECT(ui_study->zoom_spin),
				  G_CALLBACK(ui_study_cb_zoom), ui_study);

  if (ui_study->study != NULL)
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_study->zoom_spin), 
  			      AMITK_STUDY_ZOOM(ui_study->study));
  
  /* and now, reconnect the signal */
  g_signal_handlers_unblock_by_func(G_OBJECT(ui_study->zoom_spin),
				    G_CALLBACK(ui_study_cb_zoom), ui_study);

  return;
}

/* updates the settings of the fov spinbutton, will not change anything about the canvas */
void ui_study_update_fov(ui_study_t * ui_study) {

  /* block signals to the fov spin button, as we only want to
     change the value of the spin button, it's up to the caller of this
     function to change anything on the actual canvases... we'll 
     unblock at the end of this function */
  g_signal_handlers_block_by_func(G_OBJECT(ui_study->fov_spin),
				  G_CALLBACK(ui_study_cb_fov), ui_study);
  
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ui_study->fov_spin), 
			    AMITK_STUDY_FOV(ui_study->study));
  
  /* and now, reconnect the signal */
  g_signal_handlers_unblock_by_func(G_OBJECT(ui_study->fov_spin),
				    G_CALLBACK(ui_study_cb_fov), ui_study);

  return;
}

/* updates the settings of the canvas target button */
void ui_study_update_canvas_target(ui_study_t * ui_study) {

  g_signal_handlers_block_by_func(G_OBJECT(ui_study->canvas_target_action),
				  G_CALLBACK(ui_study_cb_canvas_target), ui_study);
  
  gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(ui_study->canvas_target_action),
			       AMITK_STUDY_CANVAS_TARGET(ui_study->study));
  
  g_signal_handlers_unblock_by_func(G_OBJECT(ui_study->canvas_target_action),
				    G_CALLBACK(ui_study_cb_canvas_target), ui_study);

  return;
}


void ui_study_update_fuse_type(ui_study_t * ui_study) {

  AmitkFuseType i_fuse_type;
  
  g_return_if_fail(ui_study->study != NULL);

  for (i_fuse_type = 0; i_fuse_type < AMITK_FUSE_TYPE_NUM; i_fuse_type++) 
    g_signal_handlers_block_by_func(G_OBJECT(ui_study->fuse_type_action[i_fuse_type]),
				    G_CALLBACK(ui_study_cb_fuse_type), ui_study);

  gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(ui_study->fuse_type_action[AMITK_STUDY_FUSE_TYPE(ui_study->study)]),
			       TRUE);

  for (i_fuse_type = 0; i_fuse_type < AMITK_FUSE_TYPE_NUM; i_fuse_type++)
    g_signal_handlers_unblock_by_func(G_OBJECT(ui_study->fuse_type_action[i_fuse_type]),
				      G_CALLBACK(ui_study_cb_fuse_type),  ui_study);

}

void ui_study_update_view_mode(ui_study_t * ui_study) {

  AmitkViewMode i_view_mode;

  g_return_if_fail(ui_study->study != NULL);

  for (i_view_mode = 0; i_view_mode < AMITK_VIEW_MODE_NUM; i_view_mode++) 
    g_signal_handlers_block_by_func(G_OBJECT(ui_study->view_mode_action[i_view_mode]),
				   G_CALLBACK(ui_study_cb_view_mode), ui_study);

  gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(ui_study->view_mode_action[AMITK_STUDY_VIEW_MODE(ui_study->study)]),
			       TRUE);
  
  for (i_view_mode = 0; i_view_mode < AMITK_VIEW_MODE_NUM; i_view_mode++)
    g_signal_handlers_unblock_by_func(G_OBJECT(ui_study->view_mode_action[i_view_mode]),
				      G_CALLBACK(ui_study_cb_view_mode),ui_study);

}

 
void ui_study_update_title(ui_study_t * ui_study) {
  
  gchar * title;

  if (AMITK_STUDY_FILENAME(ui_study->study) == NULL) {
    title = g_strdup_printf(_("Study: %s %s"), 
			    AMITK_OBJECT_NAME(ui_study->study),
			    ui_study->study_altered ? "*" : "");
  } else {
    title = g_strdup_printf(_("Study: %s (%s) %s"), 
			    AMITK_OBJECT_NAME(ui_study->study),
			    AMITK_STUDY_FILENAME(ui_study->study),
			    ui_study->study_altered ? "*" : "");
  }
  gtk_window_set_title(ui_study->window, title);
  g_free(title);

}

/* taken/modified from gtkhandlebox.c - note, no reattach signal gets called when this is used */
static void handle_box_reattach (GtkHandleBox *hb) {
  GtkWidget *widget = GTK_WIDGET (hb);
  
  if (hb->child_detached) {
    hb->child_detached = FALSE;
    if (GTK_WIDGET_REALIZED (hb)) {
      gdk_window_hide (hb->float_window);
      gdk_window_reparent (hb->bin_window, widget->window, 0, 0);

    }
    hb->float_window_mapped = FALSE;
  }
  gtk_widget_queue_resize (GTK_WIDGET (hb));
}


void ui_study_update_layout(ui_study_t * ui_study) {

  AmitkView i_view;
  AmitkViewMode i_view_mode;
  gint row, column, table_column, table_row;
  // GtkWidget * scrolled;
  
  g_return_if_fail(ui_study->study != NULL);

  /* get rid of visible canvases that are no longer visible */
  for (i_view_mode =  AMITK_STUDY_VIEW_MODE(ui_study->study)+1; 
       i_view_mode < AMITK_VIEW_MODE_NUM; 
       i_view_mode++) {
    if (ui_study->canvas_table[i_view_mode] != NULL) {
      gtk_widget_destroy(ui_study->canvas_table[i_view_mode]);
      ui_study->canvas_table[i_view_mode] = NULL;
      gtk_widget_destroy(ui_study->canvas_handle[i_view_mode]);
      ui_study->canvas_handle[i_view_mode] = NULL;
      for (i_view=0; i_view < AMITK_VIEW_NUM; i_view++)
	ui_study->canvas[i_view_mode][i_view] = NULL;
    }
  }
  for (i_view_mode = 0; i_view_mode <= AMITK_STUDY_VIEW_MODE(ui_study->study); i_view_mode++) {
    for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view++) {
      if ((!AMITK_STUDY_CANVAS_VISIBLE(ui_study->study, i_view)) &&
	  (ui_study->canvas[i_view_mode][i_view] != NULL)) {
	gtk_widget_destroy(ui_study->canvas[i_view_mode][i_view]);
	ui_study->canvas[i_view_mode][i_view] = NULL;
      }
    }
  }


  for (i_view_mode = 0; i_view_mode <= AMITK_STUDY_VIEW_MODE(ui_study->study); i_view_mode++) {

    if (ui_study->canvas_table[i_view_mode] == NULL) {
      ui_study->canvas_table[i_view_mode] = gtk_table_new(3, 2,FALSE);
      
      // scrolled = gtk_scrolled_window_new(NULL, NULL);
      // gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
      // gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), ui_study->canvas_table[i_view_mode]);

      ui_study->canvas_handle[i_view_mode] = gtk_handle_box_new();
      g_object_ref(G_OBJECT(ui_study->canvas_handle[i_view_mode])); /* gets removed below */
      gtk_handle_box_set_shadow_type(GTK_HANDLE_BOX(ui_study->canvas_handle[i_view_mode]), GTK_SHADOW_NONE);
      gtk_handle_box_set_handle_position(GTK_HANDLE_BOX(ui_study->canvas_handle[i_view_mode]), GTK_POS_TOP);
      gtk_container_add(GTK_CONTAINER(ui_study->canvas_handle[i_view_mode]), ui_study->canvas_table[i_view_mode]);
      //      gtk_container_add(GTK_CONTAINER(ui_study->canvas_handle[i_view_mode]), scrolled);

    } else {
      g_return_if_fail(ui_study->canvas_handle[i_view_mode] != NULL);

      /* extra ref so widget not destroyed on container_remove  - gets removed below */
      g_object_ref(G_OBJECT(ui_study->canvas_handle[i_view_mode])); 

      /* note, ui_study->panel_layout doesn't get set till end of function, but first time
	 through this function we never hit this condition */
      if (ui_study->panel_layout != AMITK_STUDY_PANEL_LAYOUT(ui_study->study)) {

	/* hack! to force handle box to reattach.  If we don't force a reattach,
	   the widget doesn't get enough size allocated to it on the gtk_table_attach
	   that follows */
	handle_box_reattach(GTK_HANDLE_BOX(ui_study->canvas_handle[i_view_mode])); 

	gtk_container_remove(GTK_CONTAINER(ui_study->center_table),
			     ui_study->canvas_handle[i_view_mode]);
      }
    }



    for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view++) {

      if (AMITK_STUDY_CANVAS_VISIBLE(ui_study->study, i_view)) {
	if (ui_study->canvas[i_view_mode][i_view] == NULL) { /* new canvas */
	  ui_study->canvas[i_view_mode][i_view] = 
	    amitk_canvas_new(ui_study->study, i_view, i_view_mode, AMITK_CANVAS_TYPE_NORMAL);
	  g_object_ref(G_OBJECT(ui_study->canvas[i_view_mode][i_view])); /* will be removed below */

	  amitk_canvas_set_active_object(AMITK_CANVAS(ui_study->canvas[i_view_mode][i_view]), 
					 ui_study->active_object);
	
	  g_signal_connect(G_OBJECT(ui_study->canvas[i_view_mode][i_view]), "help_event",
			   G_CALLBACK(ui_study_cb_canvas_help_event), ui_study);
	  g_signal_connect(G_OBJECT(ui_study->canvas[i_view_mode][i_view]), "view_changing",
			   G_CALLBACK(ui_study_cb_canvas_view_changing), ui_study);
	  g_signal_connect(G_OBJECT(ui_study->canvas[i_view_mode][i_view]), "view_changed",
			   G_CALLBACK(ui_study_cb_canvas_view_changed), ui_study);
	  g_signal_connect(G_OBJECT(ui_study->canvas[i_view_mode][i_view]), "erase_volume",
			   G_CALLBACK(ui_study_cb_canvas_erase_volume), ui_study);
	  g_signal_connect(G_OBJECT(ui_study->canvas[i_view_mode][i_view]), "new_object",
			   G_CALLBACK(ui_study_cb_canvas_new_object), ui_study);
	
	} else { /* not a new canvas */
	  /* add ref so it's not destroyed when we remove it from the container */
	  g_object_ref(G_OBJECT(ui_study->canvas[i_view_mode][i_view]));

	  /* note, ui_study->canvas_layout doesn't get set till end of function, but first time
	     through this function we never hit this condition */
	  if (ui_study->canvas_layout != AMITK_STUDY_CANVAS_LAYOUT(ui_study->study)) {
		gtk_widget_hide(ui_study->canvas[i_view_mode][i_view]); /* hide, so we get more fluid moving */
		gtk_container_remove(GTK_CONTAINER(ui_study->canvas_table[i_view_mode]), 
				     ui_study->canvas[i_view_mode][i_view]);
	  }
	}
      }
    }
  }

  for (i_view_mode = 0; i_view_mode <= AMITK_STUDY_VIEW_MODE(ui_study->study); i_view_mode++) {

    /* put the canvases in each table/handlebox according to the desired layout */
    switch(AMITK_STUDY_CANVAS_LAYOUT(ui_study->study)) {
    case AMITK_LAYOUT_ORTHOGONAL:
      row = column = 0;
      if (ui_study->canvas[i_view_mode][AMITK_VIEW_TRANSVERSE] != NULL) {
	if (gtk_widget_get_parent(ui_study->canvas[i_view_mode][AMITK_VIEW_TRANSVERSE]) == NULL)
	  gtk_table_attach(GTK_TABLE(ui_study->canvas_table[i_view_mode]), 
			   ui_study->canvas[i_view_mode][AMITK_VIEW_TRANSVERSE], 
			   column,column+1, row,row+1,FALSE,FALSE, X_PADDING, Y_PADDING);
	row++;
      }
      if (ui_study->canvas[i_view_mode][AMITK_VIEW_CORONAL] != NULL) {
	if (gtk_widget_get_parent(ui_study->canvas[i_view_mode][AMITK_VIEW_CORONAL]) == NULL)
	  gtk_table_attach(GTK_TABLE(ui_study->canvas_table[i_view_mode]), 
			   ui_study->canvas[i_view_mode][AMITK_VIEW_CORONAL], 
			   column, column+1, row, row+1, FALSE,FALSE, X_PADDING, Y_PADDING);
      }
      row = 0;
      column++;
      if (ui_study->canvas[i_view_mode][AMITK_VIEW_SAGITTAL] != NULL)
	if (gtk_widget_get_parent(ui_study->canvas[i_view_mode][AMITK_VIEW_SAGITTAL]) == NULL)
	  gtk_table_attach(GTK_TABLE(ui_study->canvas_table[i_view_mode]), 
			   ui_study->canvas[i_view_mode][AMITK_VIEW_SAGITTAL], 
			   column, column+1,row, row+1, FALSE,FALSE, X_PADDING, Y_PADDING);

      break;
    case AMITK_LAYOUT_LINEAR:
    default:
      for (i_view=0;i_view< AMITK_VIEW_NUM;i_view++)
	if (ui_study->canvas[i_view_mode][i_view] != NULL)
	  if (gtk_widget_get_parent(ui_study->canvas[i_view_mode][i_view]) == NULL)
	    gtk_table_attach(GTK_TABLE(ui_study->canvas_table[i_view_mode]), 
			     ui_study->canvas[i_view_mode][i_view], 
			     i_view, i_view+1, 0,1, FALSE, FALSE, X_PADDING, Y_PADDING);

      break;
    }


    /* place the handleboxes */
    if (gtk_widget_get_parent(ui_study->canvas_handle[i_view_mode]) == NULL) {

      switch(AMITK_STUDY_PANEL_LAYOUT(ui_study->study)) {
      case AMITK_PANEL_LAYOUT_LINEAR_X:
	table_column=i_view_mode;
	table_row=0;
	break;
      case AMITK_PANEL_LAYOUT_LINEAR_Y:
	table_row=i_view_mode;
	table_column=0;
	break;
      case AMITK_PANEL_LAYOUT_MIXED:
      default:
	table_column = (i_view_mode % 2);
	table_row = floor((i_view_mode)/2);
	break;
      }

      gtk_table_attach(GTK_TABLE(ui_study->center_table), ui_study->canvas_handle[i_view_mode],
		       table_column, table_column+1, table_row, table_row+1,
		       X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL,  
		       X_PADDING, Y_PADDING);

    }

    /* remove the additional reference */
    for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view++)
      if (ui_study->canvas[i_view_mode][i_view] != NULL)
	g_object_unref(G_OBJECT(ui_study->canvas[i_view_mode][i_view]));
    g_object_unref(ui_study->canvas_handle[i_view_mode]);

    gtk_widget_show_all(ui_study->canvas_handle[i_view_mode]); /* and show */
      
  }

  /* record the layout */
  ui_study->panel_layout = AMITK_STUDY_PANEL_LAYOUT(ui_study->study); 
  ui_study->canvas_layout = AMITK_STUDY_CANVAS_LAYOUT(ui_study->study);

  return;
}

/* function to setup the widgets inside of the study ui */
void ui_study_setup_widgets(ui_study_t * ui_study) {

  GtkWidget * scrolled;
  GtkWidget * left_vbox;
  GtkWidget * hbox;
  GtkWidget * handle_box;


  /* the hbox that'll contain everything in the ui besides the menu and toolbar */
  hbox = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start (GTK_BOX (ui_study->window_vbox), hbox, TRUE, TRUE, 0);

  /* make and add the left packing table */
  left_vbox = gtk_vbox_new(FALSE,0);

  handle_box = gtk_handle_box_new();
  gtk_handle_box_set_shadow_type(GTK_HANDLE_BOX(handle_box), GTK_SHADOW_NONE);
  gtk_handle_box_set_handle_position(GTK_HANDLE_BOX(handle_box), GTK_POS_TOP);
  gtk_container_add(GTK_CONTAINER(handle_box), left_vbox);

  gtk_box_pack_start(GTK_BOX(hbox), handle_box, FALSE, FALSE, X_PADDING);

  /* connect the blank help signal */
  g_object_set_data(G_OBJECT(ui_study->window), "which_help", GINT_TO_POINTER(AMITK_HELP_INFO_BLANK));
  g_signal_connect(G_OBJECT(ui_study->window), "enter_notify_event",
		   G_CALLBACK(ui_study_cb_update_help_info), ui_study);

  ui_study->tree_view = amitk_tree_view_new(AMITK_TREE_VIEW_MODE_MAIN,
					    ui_study->preferences,
					    ui_study->progress_dialog);

  g_signal_connect(G_OBJECT(ui_study->tree_view), "help_event",
		   G_CALLBACK(ui_study_cb_tree_view_help_event), ui_study);
  g_signal_connect(G_OBJECT(ui_study->tree_view), "activate_object", 
		   G_CALLBACK(ui_study_cb_tree_view_activate_object), ui_study);
  g_signal_connect(G_OBJECT(ui_study->tree_view), "popup_object", 
		   G_CALLBACK(ui_study_cb_tree_view_popup_object), ui_study);
  g_signal_connect(G_OBJECT(ui_study->tree_view), "add_object", 
		   G_CALLBACK(ui_study_cb_tree_view_add_object), ui_study);
  g_signal_connect(G_OBJECT(ui_study->tree_view), "delete_object", 
		   G_CALLBACK(ui_study_cb_tree_view_delete_object), ui_study);
      
  /* make a scrolled area for the tree */
  scrolled = gtk_scrolled_window_new(NULL,NULL);  
  gtk_widget_set_size_request(scrolled,LEFT_COLUMN_WIDTH,250);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), 
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), 
					ui_study->tree_view);
  gtk_box_pack_start(GTK_BOX(left_vbox), scrolled, TRUE, TRUE,Y_PADDING);


  /* the help information canvas */
#ifdef AMIDE_LIBGNOMECANVAS_AA
  ui_study->help_info = GNOME_CANVAS(gnome_canvas_new_aa());
#else
  ui_study->help_info = GNOME_CANVAS(gnome_canvas_new());
#endif
  gtk_box_pack_start(GTK_BOX(left_vbox), GTK_WIDGET(ui_study->help_info), FALSE, TRUE, Y_PADDING);
  gtk_widget_set_size_request(GTK_WIDGET(ui_study->help_info), LEFT_COLUMN_WIDTH,
			      HELP_INFO_LINE_HEIGHT*NUM_HELP_INFO_LINES+2);
  gnome_canvas_set_scroll_region(ui_study->help_info, 0.0, 0.0, LEFT_COLUMN_WIDTH, 
				 HELP_INFO_LINE_HEIGHT*NUM_HELP_INFO_LINES+2.0);

  /* make the stuff in the center */
  ui_study->center_table = gtk_table_new(2, 2,FALSE);
  gtk_box_pack_start(GTK_BOX(hbox),ui_study->center_table, TRUE, TRUE, X_PADDING);
  
  return;
}



/* replace what's currently in the ui_study with the specified study */
void ui_study_set_study(ui_study_t * ui_study, AmitkStudy * study) {

  add_object(ui_study, AMITK_OBJECT(study));

  ui_study->study_altered=FALSE;
  ui_study_update_title(ui_study);
  ui_study_make_active_object(ui_study, NULL);

}


/* procedure to set up the study window */
GtkWidget * ui_study_create(AmitkStudy * study, AmitkPreferences * preferences) {

  ui_study_t * ui_study;
  GdkPixbuf * pixbuf;

  g_return_val_if_fail(preferences != NULL, NULL);

  ui_study = ui_study_init(preferences);

  /* setup the study window */
  ui_study->window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
  ui_study->window_vbox = gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER (ui_study->window), ui_study->window_vbox);

  /* set the icon that the window manager knows about */
  pixbuf = gtk_widget_render_icon(GTK_WIDGET(ui_study->window), "amide_icon_logo", -1, 0);
  gtk_window_set_icon(ui_study->window, pixbuf);
  gtk_window_set_default_icon(pixbuf); /* sets it as the default for all additional windows */
  g_object_unref(pixbuf);

  /* disable user resizability, allows the window to autoshrink */  
  gtk_window_set_resizable(ui_study->window, FALSE); 

  /* setup the callbacks for the window */
  g_signal_connect(G_OBJECT(ui_study->window), "delete_event",  
		   G_CALLBACK(ui_study_cb_delete_event), ui_study);

  ui_study->progress_dialog = amitk_progress_dialog_new(ui_study->window);

  /* setup the menu and toolbar */
  menus_toolbar_create(ui_study);

  /* setup the rest of the study window */
  ui_study_setup_widgets(ui_study);


  /* add the study to the ui_study */
  if (study == NULL) {

    study = amitk_study_new(preferences);

    ui_study_set_study(ui_study, study);
    amitk_object_unref(study);
    ui_study->study_virgin=TRUE;

  } else {

    ui_study_set_study(ui_study, study);
    ui_study->study_virgin=FALSE;

  }
  ui_study_update_title(ui_study);

  /* get the study window running */
  gtk_widget_show_all(GTK_WIDGET(ui_study->window));
  amide_register_window((gpointer) ui_study->window);

  return GTK_WIDGET(ui_study->window);
}
 
