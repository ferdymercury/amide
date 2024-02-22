/* ui_study.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2017 Andy Loening
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

  AmitkInterpolation interpolation;

  
  if (AMITK_IS_STUDY(ui_study->active_object)) {
    g_simple_action_set_enabled(G_SIMPLE_ACTION(ui_study->interpolation_action), FALSE);
    gtk_widget_set_sensitive(ui_study->rendering_menu, FALSE);
  } else if (AMITK_IS_DATA_SET(ui_study->active_object)) {
    g_simple_action_set_enabled(G_SIMPLE_ACTION(ui_study->interpolation_action), TRUE);
    gtk_widget_set_sensitive(ui_study->rendering_menu, TRUE);


    /* block signals, as we only want to change the value, it's up to the caller of this
       function to change anything on the actual canvases... 
       we'll unblock at the end of this function */
    g_signal_handlers_block_by_func(ui_study->interpolation_action,
				      G_CALLBACK(ui_study_cb_interpolation), ui_study);
    g_signal_handlers_block_by_func(G_OBJECT(ui_study->rendering_menu),
				    G_CALLBACK(ui_study_cb_rendering), ui_study);

    interpolation = AMITK_DATA_SET_INTERPOLATION(ui_study->active_object);
    g_simple_action_set_state(G_SIMPLE_ACTION(ui_study->interpolation_action),
                              g_variant_new_int32(interpolation));
    gtk_combo_box_set_active(GTK_COMBO_BOX(ui_study->rendering_menu), 
			     AMITK_DATA_SET_RENDERING(ui_study->active_object));

    g_signal_handlers_unblock_by_func(ui_study->interpolation_action,
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






static const GActionEntry normal_items[] = {
  { "new", ui_study_cb_new_study },
  { "open", ui_study_cb_open_xif_file },
  { "save", ui_study_cb_save_as_xif_file },
  { "import", ui_study_cb_import, "(ii)" },
  { "import-object", ui_study_cb_import_object_from_xif_file },
  { "export", ui_study_cb_export_data_set },
  { "recover", ui_study_cb_recover_xif_file },
  { "open-dir", ui_study_cb_open_xif_dir },
  { "save-dir", ui_study_cb_save_as_xif_dir },
  { "import-dir", ui_study_cb_import_object_from_xif_dir },
  { "close", ui_study_cb_close },
  { "quit", ui_study_cb_quit },
  { "export-view", ui_study_cb_export_view, "i" },
  { "add-roi", ui_study_cb_add_roi, "i" },
  { "fiducial", ui_study_cb_add_fiducial_mark },
  { "preferences", ui_study_cb_preferences },
  { "series", ui_study_cb_series },
#if AMIDE_LIBVOLPACK_SUPPORT
  { "rendering", ui_study_cb_render },
#endif
  { "alignment", ui_study_cb_alignment_selected },
  { "crop", ui_study_cb_crop_selected },
  { "distance", ui_study_cb_distance_selected },
  { "fads", ui_study_cb_fads_selected },
  { "filter", ui_study_cb_filter_selected },
  { "profile", ui_study_cb_profile_selected },
  { "math", ui_study_cb_data_set_math_selected },
  { "roi", ui_study_cb_roi_statistics },
#if (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)
  { "fly-through", ui_study_cb_fly_through, "i" },
#endif
  { "thresholding", ui_study_cb_thresholding }
};

static const GActionEntry stateful_entries[] = {
  { "interpolation", NULL, "i", "@i 0", ui_study_cb_interpolation },
  { "fuse-type", NULL, "i", "@i 0", ui_study_cb_fuse_type },
  { "view-mode", NULL, "i", "@i 0", ui_study_cb_view_mode },
  { "target", NULL, NULL, "false", ui_study_cb_canvas_target },
  { "view-transverse", NULL, NULL, "true", ui_study_cb_canvas_visible },
  { "view-coronal", NULL, NULL, "true", ui_study_cb_canvas_visible },
  { "view-sagittal", NULL, NULL, "true", ui_study_cb_canvas_visible }
};



/* function to setup the menus for the study ui */
static void menus_toolbar_create(ui_study_t * ui_study) {

  GtkApplication *app;
  GtkWidget *toolbar;
  GtkWidget * label;
  AmitkImportMethod i_import_method;
  AmitkRendering i_rendering;
  GMenu * menu;
  GMenuItem * menu_item;
  GtkToolItem * tool_item;
  GtkWidget * icon;
#ifdef AMIDE_LIBMDC_SUPPORT
  libmdc_import_t i_libmdc_import;
#endif
  AmitkRoiType i_roi_type;
  GtkAdjustment * adjustment;
  gchar *name;
  const gchar *new[] = { "<Control>n", NULL };
  const gchar *open[] = { "<Control>o", NULL };
  const gchar *close[] = { "<Control>w", NULL };
  const gchar *quit[] = { "<Control>q", NULL };
  const gchar *help[] = { "F1", NULL };
  static gboolean menus_built = FALSE;
  
  g_assert(ui_study!=NULL); /* sanity check */


  app = gtk_window_get_application(ui_study->window);
  g_action_map_add_action_entries(G_ACTION_MAP(ui_study->window),
                                  normal_items, G_N_ELEMENTS(normal_items),
                                  ui_study);
  g_action_map_add_action_entries(G_ACTION_MAP(app),
                                  ui_common_help_menu_items,
                                  G_N_ELEMENTS(ui_common_help_menu_items),
                                  ui_study);
  g_action_map_add_action_entries(G_ACTION_MAP(ui_study->window),
                                  stateful_entries,
                                  G_N_ELEMENTS(stateful_entries), ui_study);

  gtk_application_set_accels_for_action(app, "win.new", new);
  gtk_application_set_accels_for_action(app, "win.open", open);
  gtk_application_set_accels_for_action(app, "win.close", close);
  gtk_application_set_accels_for_action(app, "win.quit", quit);
  gtk_application_set_accels_for_action(app, "app.manual", help);

  /* Because the main (study) menu (src/menus.ui) is so-called
     "automatic resource" loaded by GtkApplication, manipulating it
     should be done only once, for the first study window.  */
  if (!menus_built) {
#if !AMIDE_LIBVOLPACK_SUPPORT
    /* Remove "Volume Rendering" menu item.  */
    menu = gtk_application_get_menu_by_id(app, "view");
    g_menu_remove(menu, 1);
#endif
#if !(AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)
    /* Remove "Generate Fly Through" submenu.  */
    menu = gtk_application_get_menu_by_id(app, "tools");
    g_menu_remove(menu, 5);
#endif

    /* build the import menu */
    menu = gtk_application_get_menu_by_id(app, "import");
    for (i_import_method = AMITK_IMPORT_METHOD_RAW; i_import_method < AMITK_IMPORT_METHOD_NUM; i_import_method++) {
#ifdef AMIDE_LIBMDC_SUPPORT
      if (i_import_method == AMITK_IMPORT_METHOD_LIBMDC) {
        for (i_libmdc_import = 0; i_libmdc_import < LIBMDC_NUM_IMPORT_METHODS; i_libmdc_import++) {
          if (libmdc_supports(libmdc_import_to_format[i_libmdc_import])) {
            name = g_strdup_printf("win.import((%d,%d))", i_import_method,
                                   libmdc_import_to_format[i_libmdc_import]);
            menu_item = g_menu_item_new(libmdc_import_menu_names[i_libmdc_import],
                                        name);
            g_free(name);
            /* if tooltip support existed - libmdc_import_menu_explanations[i_libmdc_import] */
            g_menu_insert_item(menu, i_libmdc_import, menu_item);
            g_object_unref(menu_item);
          }
        }
      } else
#endif
        {
          name = g_strdup_printf("win.import((%d,0))", i_import_method);
          menu_item = g_menu_item_new(amitk_import_menu_names[i_import_method],
                                      name);
          g_free(name);
          /* if tooltip support existed - amitk_import_menu_explanations[i_import_method] */
          g_menu_insert_item(menu, i_import_method, menu_item);
          g_object_unref(menu_item);
        }
    }

    /* build the add roi menu */
    menu = gtk_application_get_menu_by_id(app, "roi");
    for (i_roi_type=0; i_roi_type<AMITK_ROI_TYPE_NUM; i_roi_type++) {
      name = g_strdup_printf("win.add-roi(%d)", i_roi_type);
      menu_item = g_menu_item_new(_(amitk_roi_menu_names[i_roi_type]), name);
      g_free(name);
      /* if tooltip support existed - amitk_roi_menu_explanation[i_roi_type] */
      g_menu_insert_item(menu, i_roi_type, menu_item);
      g_object_unref(menu_item);
    }
    menus_built = TRUE;
  }

  /* record the radio/toggle items so that we can change which one is depressed later */
  ui_study->interpolation_action =
    g_action_map_lookup_action(G_ACTION_MAP(ui_study->window), "interpolation");

  ui_study->fuse_type_action =
    g_action_map_lookup_action(G_ACTION_MAP(ui_study->window), "fuse-type");

  ui_study->view_mode_action =
    g_action_map_lookup_action(G_ACTION_MAP(ui_study->window), "view-mode");

  ui_study->canvas_target_action =
    g_action_map_lookup_action(G_ACTION_MAP(ui_study->window), "target");

  ui_study->canvas_visible_action[AMITK_VIEW_TRANSVERSE] =
    g_action_map_lookup_action(G_ACTION_MAP(ui_study->window), "view-transverse");
  ui_study->canvas_visible_action[AMITK_VIEW_CORONAL] =
    g_action_map_lookup_action(G_ACTION_MAP(ui_study->window), "view-coronal");
  ui_study->canvas_visible_action[AMITK_VIEW_SAGITTAL] =
    g_action_map_lookup_action(G_ACTION_MAP(ui_study->window), "view-sagittal");

  /* Construct the toolbar.  All of this is a bit verbose but easier
     to do than writing an XML description (or doing it in Glade).  */
  toolbar = gtk_toolbar_new();
  gtk_box_pack_start (GTK_BOX (ui_study->window_vbox), toolbar, FALSE, FALSE, 0);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_show_arrow(GTK_TOOLBAR(toolbar), FALSE);

  tool_item = gtk_radio_tool_button_new(NULL);
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(tool_item), _("Near."));
  gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tool_item),
                                "amide_icon_interpolation_nearest_neighbor");
  gtk_tool_item_set_tooltip_text(tool_item,
                                 _("interpolate using nearest neighbor (fast)"));
  gtk_actionable_set_detailed_action_name(GTK_ACTIONABLE(tool_item),
                                          "win.interpolation(0)");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);
  tool_item =
    gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(tool_item));
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(tool_item), _("Tri."));
  gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tool_item),
                                "amide_icon_interpolation_trilinear");
  gtk_tool_item_set_tooltip_text(tool_item,
                                 _("interpolate using trilinear interpolation (slow)"));
  gtk_actionable_set_detailed_action_name(GTK_ACTIONABLE(tool_item),
                                          "win.interpolation(1)");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);

  /* insert the rendering menu into the toolbar */
  ui_study->rendering_menu = gtk_combo_box_text_new();

  for (i_rendering = 0; i_rendering < AMITK_RENDERING_NUM; i_rendering++) 
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(ui_study->rendering_menu), amitk_rendering_get_name(i_rendering));
  g_signal_connect(G_OBJECT(ui_study->rendering_menu), "changed", G_CALLBACK(ui_study_cb_rendering), ui_study);
  ui_common_toolbar_insert_widget(toolbar, ui_study->rendering_menu,
                                  _(amitk_rendering_explanation),
                                  gtk_toolbar_get_n_items(GTK_TOOLBAR(toolbar)));

  ui_common_toolbar_append_separator(toolbar);

  tool_item = gtk_radio_tool_button_new(NULL);
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(tool_item), _("Blend"));
  gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tool_item),
                                "amide_icon_fuse_type_blend");
  gtk_tool_item_set_tooltip_text(tool_item, _("blend all data sets"));
  gtk_actionable_set_detailed_action_name(GTK_ACTIONABLE(tool_item),
                                          "win.fuse-type(0)");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);
  tool_item =
    gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(tool_item));
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(tool_item), _("Overlay"));
  gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tool_item),
                                "amide_icon_fuse_type_overlay");
  gtk_tool_item_set_tooltip_text(tool_item,
                                 _("overlay active data set on blended data sets"));
  gtk_actionable_set_detailed_action_name(GTK_ACTIONABLE(tool_item),
                                          "win.fuse-type(1)");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);

  ui_common_toolbar_append_separator(toolbar);

  tool_item = gtk_toggle_tool_button_new();
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(tool_item), _("Target"));
  gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tool_item),
                                "amide_icon_canvas_target");
  gtk_tool_item_set_tooltip_text(tool_item, _("Leave crosshairs on views"));
  gtk_actionable_set_action_name(GTK_ACTIONABLE(tool_item), "win.target");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);

  ui_common_toolbar_append_separator(toolbar);

  tool_item = gtk_toggle_tool_button_new();
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(tool_item), _("Transverse"));
  gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tool_item),
                                "amide_icon_view_transverse");
  gtk_tool_item_set_tooltip_text(tool_item, _("Enable transverse view"));
  gtk_actionable_set_detailed_action_name(GTK_ACTIONABLE(tool_item),
                                          "win.view-transverse");
  g_object_set_data(G_OBJECT(ui_study->canvas_visible_action[AMITK_VIEW_TRANSVERSE]),
                    "button", tool_item);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);
  tool_item = gtk_toggle_tool_button_new();
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(tool_item), _("Coronal"));
  gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tool_item),
                                "amide_icon_view_coronal");
  gtk_tool_item_set_tooltip_text(tool_item, _("Enable coronal view"));
  gtk_actionable_set_detailed_action_name(GTK_ACTIONABLE(tool_item),
                                          "win.view-coronal");
  g_object_set_data(G_OBJECT(ui_study->canvas_visible_action[AMITK_VIEW_CORONAL]),
                    "button", tool_item);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);
  tool_item = gtk_toggle_tool_button_new();
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(tool_item), _("Sagittal"));
  gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tool_item),
                           "amide_icon_view_sagittal");
  gtk_tool_item_set_tooltip_text(tool_item, _("Enable sagittal view"));
  gtk_actionable_set_detailed_action_name(GTK_ACTIONABLE(tool_item),
                                          "win.view-sagittal");
  g_object_set_data(G_OBJECT(ui_study->canvas_visible_action[AMITK_VIEW_SAGITTAL]),
                    "button", tool_item);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);

  ui_common_toolbar_append_separator(toolbar);

  tool_item = gtk_radio_tool_button_new(NULL);
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(tool_item), _("Single"));
  gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tool_item),
                                 "amide_icon_view_mode_single");
  gtk_tool_item_set_tooltip_text(tool_item,
                                 _("All objects are shown in a single view"));
  gtk_actionable_set_detailed_action_name(GTK_ACTIONABLE(tool_item),
                                          "win.view-mode(0)");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);
  tool_item =
    gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(tool_item));
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(tool_item), _("2-way"));
  gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tool_item),
                                "amide_icon_view_mode_linked_2way");
  gtk_tool_item_set_tooltip_text(tool_item,
                                 _("Objects are shown between 2 linked views"));
  gtk_actionable_set_detailed_action_name(GTK_ACTIONABLE(tool_item),
                                          "win.view-mode(1)");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);
  tool_item =
    gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(tool_item));
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(tool_item), _("3-way"));
  gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(tool_item),
                                "amide_icon_view_mode_linked_3way");
  gtk_tool_item_set_tooltip_text(tool_item,
                                 _("Objects are shown between 3 linked views"));
  gtk_actionable_set_detailed_action_name(GTK_ACTIONABLE(tool_item),
                                          "win.view-mode(2)");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);

  ui_common_toolbar_append_separator(toolbar);

  icon = gtk_image_new_from_icon_name("amide_icon_thresholding",
                                      GTK_ICON_SIZE_LARGE_TOOLBAR);
  tool_item = gtk_tool_button_new(icon, _("_Threshold"));
  gtk_tool_item_set_tooltip_text(tool_item,
                                 _("Set the thresholds and colormaps for the active data set"));
  gtk_actionable_set_action_name(GTK_ACTIONABLE(tool_item), "win.thresholding");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_item, -1);

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

  GVariant * state;
  AmitkView i_view;

  for (i_view=0; i_view < AMITK_VIEW_NUM; i_view++) {
    g_signal_handlers_block_by_func(G_OBJECT(ui_study->canvas_visible_action[i_view]),
				    G_CALLBACK(ui_study_cb_canvas_visible), ui_study);
    state = g_variant_new_boolean(AMITK_STUDY_CANVAS_VISIBLE(ui_study->study,
                                                             i_view));
    g_simple_action_set_state(G_SIMPLE_ACTION
                              (ui_study->canvas_visible_action[i_view]),
                              state);
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

  AmitkCanvasItem * root;
  help_info_line_t i_line;
  gchar * location_text[2];
  gchar * legend;
  AmitkPoint location_p;
  gchar * help_info_line;

  /* put up the help lines... except for the given info types, in which case
     we leave the last one displayed up */
  root = amitk_simple_canvas_get_root_item(AMITK_SIMPLE_CANVAS(ui_study->help_info));
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
          amitk_canvas_text_new(root, legend, 55.0,
                              (gdouble) (i_line*HELP_INFO_LINE_HEIGHT),
                              -1, AMITK_CANVAS_ANCHOR_NORTH_EAST,
                              "alignment", PANGO_ALIGN_RIGHT,
                              "fill-color", "black",
                              "font-desc", amitk_fixed_font_desc, NULL);
      else /* just need to change the text */
	g_object_set(ui_study->help_legend[i_line], "text", legend, NULL);
      
      /* gettext can't handle "" */
      if (g_strcmp0(help_info_lines[which_info][i_line],"") != 0)
	help_info_line = _(help_info_lines[which_info][i_line]);
      else
	help_info_line = help_info_lines[which_info][i_line];

      /* and the button info */
      if (ui_study->help_line[i_line] == NULL) 
	ui_study->help_line[i_line] = 
          amitk_canvas_text_new(root, help_info_line, 65.0,
                              (gdouble) (i_line*HELP_INFO_LINE_HEIGHT),
                              -1, AMITK_CANVAS_ANCHOR_NORTH_WEST,
                              "fill-color", "black",
                              "font-desc", amitk_fixed_font_desc, NULL);
      else /* just need to change the text */
	g_object_set(ui_study->help_line[i_line], "text",
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
        amitk_canvas_text_new(root,
                            location_text[i_line-HELP_INFO_LINE_LOCATION1],
                            2.0, (gdouble) (i_line*HELP_INFO_LINE_HEIGHT),
                            -1, AMITK_CANVAS_ANCHOR_NORTH_WEST,
                            "fill-color", "black",
                            "font-desc", amitk_fixed_font_desc, NULL);

    else /* just need to change the text */
      g_object_set(ui_study->help_line[i_line], "text",
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

  GVariant * state;

  g_signal_handlers_block_by_func(G_OBJECT(ui_study->canvas_target_action),
				  G_CALLBACK(ui_study_cb_canvas_target), ui_study);
  
  state = g_variant_new_boolean(AMITK_STUDY_CANVAS_TARGET(ui_study->study));
  g_simple_action_set_state(G_SIMPLE_ACTION(ui_study->canvas_target_action),
                            state);
  
  g_signal_handlers_unblock_by_func(G_OBJECT(ui_study->canvas_target_action),
				    G_CALLBACK(ui_study_cb_canvas_target), ui_study);

  return;
}


void ui_study_update_fuse_type(ui_study_t * ui_study) {

  GVariant * state;
  
  g_return_if_fail(ui_study->study != NULL);

  g_signal_handlers_block_by_func(ui_study->fuse_type_action,
				    G_CALLBACK(ui_study_cb_fuse_type), ui_study);

  state = g_variant_new_int32(AMITK_STUDY_FUSE_TYPE(ui_study->study));
  g_simple_action_set_state(G_SIMPLE_ACTION(ui_study->fuse_type_action),
                            state);

  g_signal_handlers_unblock_by_func(ui_study->fuse_type_action,
				      G_CALLBACK(ui_study_cb_fuse_type),  ui_study);

}

void ui_study_update_view_mode(ui_study_t * ui_study) {

  GVariant * state;

  g_return_if_fail(ui_study->study != NULL);

  g_signal_handlers_block_by_func(ui_study->view_mode_action,
				   G_CALLBACK(ui_study_cb_view_mode), ui_study);

  state = g_variant_new_int32(AMITK_STUDY_VIEW_MODE(ui_study->study));
  g_simple_action_set_state(G_SIMPLE_ACTION(ui_study->view_mode_action), state);
  
  g_signal_handlers_unblock_by_func(ui_study->view_mode_action,
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
      ui_study->canvas_table[i_view_mode] = gtk_grid_new();
      gtk_grid_set_row_spacing(GTK_GRID(ui_study->canvas_table[i_view_mode]),
                               Y_PADDING);
      gtk_grid_set_column_spacing(GTK_GRID(ui_study->canvas_table[i_view_mode]),
                                  X_PADDING);
      
      // scrolled = gtk_scrolled_window_new(NULL, NULL);
      // gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
      // gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), ui_study->canvas_table[i_view_mode]);
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
        if (gtk_widget_get_parent(ui_study->canvas[i_view_mode][AMITK_VIEW_TRANSVERSE]) == NULL) {
          gtk_widget_set_valign(ui_study->canvas[i_view_mode][AMITK_VIEW_TRANSVERSE],
                                GTK_ALIGN_CENTER);
          gtk_grid_attach(GTK_GRID(ui_study->canvas_table[i_view_mode]),
                          ui_study->canvas[i_view_mode][AMITK_VIEW_TRANSVERSE],
                          column, row, 1, 1);
        }
	row++;
      }
      if (ui_study->canvas[i_view_mode][AMITK_VIEW_CORONAL] != NULL) {
        if (gtk_widget_get_parent(ui_study->canvas[i_view_mode][AMITK_VIEW_CORONAL]) == NULL) {
          gtk_widget_set_valign(ui_study->canvas[i_view_mode][AMITK_VIEW_CORONAL],
                                GTK_ALIGN_CENTER);
          gtk_grid_attach(GTK_GRID(ui_study->canvas_table[i_view_mode]),
                          ui_study->canvas[i_view_mode][AMITK_VIEW_CORONAL],
                          column, row, 1, 1);
        }
      }
      row = 0;
      column++;
      if (ui_study->canvas[i_view_mode][AMITK_VIEW_SAGITTAL] != NULL)
        if (gtk_widget_get_parent(ui_study->canvas[i_view_mode][AMITK_VIEW_SAGITTAL]) == NULL) {
          gtk_widget_set_valign(ui_study->canvas[i_view_mode][AMITK_VIEW_SAGITTAL],
                                GTK_ALIGN_CENTER);
          gtk_grid_attach(GTK_GRID(ui_study->canvas_table[i_view_mode]),
                          ui_study->canvas[i_view_mode][AMITK_VIEW_SAGITTAL],
                          column, row, 1, 1);
        }

      break;
    case AMITK_LAYOUT_LINEAR:
    default:
      for (i_view=0;i_view< AMITK_VIEW_NUM;i_view++)
	if (ui_study->canvas[i_view_mode][i_view] != NULL)
          if (gtk_widget_get_parent(ui_study->canvas[i_view_mode][i_view]) == NULL) {
            gtk_widget_set_valign(ui_study->canvas[i_view_mode][i_view],
                                  GTK_ALIGN_CENTER);
            gtk_grid_attach(GTK_GRID(ui_study->canvas_table[i_view_mode]),
                            ui_study->canvas[i_view_mode][i_view],
                            i_view, 0, 1, 1);
          }

      break;
    }


    if (gtk_widget_get_parent(ui_study->canvas_table[i_view_mode]) == NULL) {

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

      gtk_grid_attach(GTK_GRID(ui_study->center_table),
                      ui_study->canvas_table[i_view_mode],
                      table_column, table_row, 1, 1);

    }

    /* remove the additional reference */
    for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view++)
      if (ui_study->canvas[i_view_mode][i_view] != NULL)
	g_object_unref(G_OBJECT(ui_study->canvas[i_view_mode][i_view]));

    gtk_widget_show_all(ui_study->canvas_table[i_view_mode]); /* and show */
      
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


  /* the hbox that'll contain everything in the ui besides the menu and toolbar */
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
  gtk_box_pack_start (GTK_BOX (ui_study->window_vbox), hbox, TRUE, TRUE, 0);

  /* make and add the left packing table */
  left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  gtk_box_pack_start(GTK_BOX(hbox), left_vbox, FALSE, FALSE, X_PADDING);

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
  gtk_container_add(GTK_CONTAINER(scrolled), ui_study->tree_view);
  gtk_box_pack_start(GTK_BOX(left_vbox), scrolled, TRUE, TRUE,Y_PADDING);


  /* the help information canvas */
  ui_study->help_info = AMITK_SIMPLE_CANVAS(amitk_simple_canvas_new());
  gtk_box_pack_start(GTK_BOX(left_vbox), GTK_WIDGET(ui_study->help_info), FALSE, TRUE, Y_PADDING);
  gtk_widget_set_size_request(GTK_WIDGET(ui_study->help_info), LEFT_COLUMN_WIDTH,
			      HELP_INFO_LINE_HEIGHT*NUM_HELP_INFO_LINES+2);
  amitk_simple_canvas_set_bounds(ui_study->help_info, 0.0, 0.0, LEFT_COLUMN_WIDTH,
				 HELP_INFO_LINE_HEIGHT*NUM_HELP_INFO_LINES+2.0);

  /* make the stuff in the center */
  ui_study->center_table = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(ui_study->center_table), Y_PADDING);
  gtk_grid_set_column_spacing(GTK_GRID(ui_study->center_table), X_PADDING);
  gtk_box_pack_start(GTK_BOX(hbox),ui_study->center_table, TRUE, TRUE, X_PADDING);
  gtk_widget_show(ui_study->center_table);
  
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
  GtkApplication * app;

  g_return_val_if_fail(preferences != NULL, NULL);

  ui_study = ui_study_init(preferences);
  app = GTK_APPLICATION(g_application_get_default());

  /* setup the study window */
  ui_study->window = GTK_WINDOW(gtk_application_window_new(app));
  ui_study->window_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
  gtk_container_add(GTK_CONTAINER (ui_study->window), ui_study->window_vbox);

  /* set the icon that the window manager knows about */
  pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                    "amide_icon_logo", 90, 0, NULL);
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
 
