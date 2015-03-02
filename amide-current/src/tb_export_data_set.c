/* tb_export_data_set.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2006-2007 Andy Loening
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
#include <gtk/gtk.h>
#include "amitk_progress_dialog.h"

#ifndef AMIDE_WIN32_HACKS
#include <libgnome/libgnome.h>
#endif
#include "tb_export_data_set.h"
#include "ui_common.h"
#ifdef AMIDE_LIBMDC_SUPPORT
#include "libmdc_interface.h"
#endif

#ifdef AMIDE_WIN32_HACKS
static gboolean resliced=FALSE;
static gboolean all_visible=FALSE;
static gboolean inclusive_bounding_box=FALSE;
static AmitkExportMethod method=0;
static gint submethod=0;
static AmitkPoint voxel_size=ONE_POINT;
#endif

typedef struct tb_export_t {
  AmitkStudy * study;
  AmitkDataSet * active_ds;

  GtkWidget * dialog;
  GtkWidget * progress_dialog;
  GtkWidget * vs_spin_button[AMITK_AXIS_NUM];
  GtkWidget * bb_radio_button[2];

  guint reference_count;

} tb_export_t;




static tb_export_t * tb_export_unref(tb_export_t * tb_export) {

  g_return_val_if_fail(tb_export != NULL, NULL);
  gboolean return_val;

  /* sanity checks */
  g_return_val_if_fail(tb_export->reference_count > 0, NULL);

  /* remove a reference count */
  tb_export->reference_count--;

  /* things to do if we've removed all reference's */
  if (tb_export->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing tb_export\n");
#endif

    if (tb_export->study != NULL) {
      amitk_object_unref(tb_export->study);
      tb_export->study = NULL;
    }

    if (tb_export->active_ds != NULL) {
      amitk_object_unref(tb_export->active_ds);
      tb_export->active_ds = NULL;
    }

    if (tb_export->progress_dialog != NULL) {
      g_signal_emit_by_name(G_OBJECT(tb_export->progress_dialog), "delete_event", NULL, &return_val);
      tb_export->progress_dialog = NULL;
    }

    g_free(tb_export);
    tb_export = NULL;
  }

  return tb_export;

}

/* allocate and initialize a tb_export data structure */
static tb_export_t * tb_export_init(void) {

  tb_export_t * tb_export;

  /* alloc space for the data structure for passing ui info */
  if ((tb_export = g_try_new(tb_export_t,1)) == NULL) {
    g_warning(_("couldn't allocate memory space for tb_export_t"));
    return NULL;
  }
  tb_export->reference_count = 1;

  /* set any needed parameters */
  tb_export->study = NULL;
  tb_export->active_ds = NULL;
  tb_export->progress_dialog = NULL;

  return tb_export;
}


static void destroy_cb(GtkObject * object, gpointer data) {
  tb_export_t * tb_export = data;
  tb_export = tb_export_unref(tb_export); /* free the associated data structure */
}

static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {
  //  tb_export_t * tb_export = data;
  return FALSE;
}


#ifndef AMIDE_WIN32_HACKS
static void read_preferences(gboolean * resliced,
			     gboolean * all_visible,
			     gboolean * inclusive_bounding_box,
			     AmitkExportMethod * method,
			     gint * submethod,
			     AmitkPoint * voxel_size) {

  gfloat temp_float;
  gint default_value;

  gnome_config_push_prefix("/"PACKAGE"/");

  *resliced = gnome_config_get_int("EXPORT/ResliceDataSet");
  *all_visible = gnome_config_get_int("EXPORT/AllVisibleDataSets");
  *inclusive_bounding_box = gnome_config_get_int("EXPORT/InclusiveBoundingBox");
  *method = gnome_config_get_int("EXPORT/Method");
  *submethod = gnome_config_get_int("EXPORT/Submethod");

  temp_float = gnome_config_get_float_with_default("EXPORT/VoxelSizeZ",&default_value);
  (*voxel_size).z =  default_value ? 1.0 : temp_float;
  temp_float = gnome_config_get_float_with_default("EXPORT/VoxelSizeY",&default_value);
  (*voxel_size).y =  default_value ? 1.0 : temp_float;
  temp_float = gnome_config_get_float_with_default("EXPORT/VoxelSizeX",&default_value);
  (*voxel_size).x =  default_value ? 1.0 : temp_float;

  //  *subfraction = gnome_config_get_float("ANALYSIS/SubFraction");

  gnome_config_pop_prefix();

  return;
}
#endif


/* function called when we hit "ok" on the export file dialog */
static void export_data_set_ok(GtkWidget* widget, gpointer data) {

  GtkWidget * file_selection = data;
  gchar * filename;
  GList * data_sets;
  tb_export_t * tb_export;
  AmitkVolume * bounding_box = NULL;
#ifndef AMIDE_WIN32_HACKS
  gboolean resliced;
  gboolean all_visible;
  gboolean inclusive_bounding_box;
  AmitkExportMethod method;
  gint submethod;
  AmitkPoint voxel_size;

  read_preferences(&resliced, &all_visible, &inclusive_bounding_box, &method, &submethod, &voxel_size);
#endif

  /* save a pointer to the export_data_set data, so we can use it in the callbacks */
  tb_export = g_object_get_data(G_OBJECT(file_selection), "tb_export");

  /* get the filename and import */
  filename = ui_common_file_selection_get_save_name(file_selection);
  if (filename == NULL) return;

  /* get total bounding box if needed */
  if (inclusive_bounding_box) {
    AmitkCorners corner;
    bounding_box = amitk_volume_new(); /* in base coordinate frame */
    g_return_if_fail(bounding_box != NULL);
    data_sets = amitk_object_get_children_of_type(AMITK_OBJECT(tb_export->study), 
						  AMITK_OBJECT_TYPE_DATA_SET, TRUE);
    amitk_volumes_get_enclosing_corners(data_sets, AMITK_SPACE(bounding_box), corner);
    amitk_space_set_offset(AMITK_SPACE(bounding_box), corner[0]);
    amitk_volume_set_corner(bounding_box, amitk_space_b2s(AMITK_SPACE(bounding_box), corner[1]));
    amitk_objects_unref(data_sets);
  }


  if (!all_visible) {
    amitk_data_set_export_to_file(tb_export->active_ds, 
				  method, submethod, filename, resliced,
				  voxel_size, bounding_box,
				  amitk_progress_dialog_update,
				  tb_export->progress_dialog);
  } else {
    data_sets = amitk_object_get_selected_children_of_type(AMITK_OBJECT(tb_export->study), 
							   AMITK_OBJECT_TYPE_DATA_SET, 
							   AMITK_SELECTION_SELECTED_0, TRUE);
    if (data_sets == NULL) {
      g_warning(_("No Data Sets are current visible"));
    } else {
      amitk_data_sets_export_to_file(data_sets, method, submethod, filename, 
				     voxel_size, bounding_box,
				     amitk_progress_dialog_update,
				     tb_export->progress_dialog);
      amitk_objects_unref(data_sets);
    }
  }

  /* close the file selection box and cleanup*/
  ui_common_file_selection_cancel_cb(widget, file_selection);
  g_free(filename);

  if (bounding_box != NULL) 
    bounding_box = amitk_object_unref(bounding_box);

  return;
}


static void response_cb (GtkDialog * dialog, gint response_id, gpointer data) {
  
  tb_export_t * tb_export = data;
  GtkWidget * file_selection;
  gint return_val;
  gboolean end_dialog=FALSE;
  
  switch(response_id) {
  case AMITK_RESPONSE_EXECUTE:
    /* the rest of this function runs the file selection dialog box */
    file_selection = gtk_file_selection_new(_("Export to File"));
    ui_common_file_selection_set_filename(file_selection, NULL);
    
    /* don't want anything else going on till this window is gone */
    gtk_window_set_modal(GTK_WINDOW(file_selection), TRUE);
    
    /* save a pointer to the export_data_set data, so we can use it in the callbacks */
    g_object_set_data(G_OBJECT(file_selection), "tb_export", tb_export);
    
    /* connect the signals */
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(file_selection)->ok_button), "clicked",
		     G_CALLBACK(export_data_set_ok), file_selection);
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(file_selection)->cancel_button), "clicked",
		     G_CALLBACK(ui_common_file_selection_cancel_cb), file_selection);
    g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(file_selection)->cancel_button),"delete_event",
		     G_CALLBACK(ui_common_file_selection_cancel_cb), file_selection);
    /* set the position of the dialog */
    gtk_window_set_position(GTK_WINDOW(file_selection), GTK_WIN_POS_MOUSE);
    
    /* run the dialog */
    return_val = gtk_dialog_run(GTK_DIALOG(file_selection));
    if (return_val != GTK_RESPONSE_CANCEL) end_dialog=TRUE;

    break;

  case GTK_RESPONSE_CANCEL:
    end_dialog = TRUE;
    break;

  default:
    break;
  }

  if (end_dialog) {
    g_signal_emit_by_name(G_OBJECT(dialog), "delete_event", NULL, &return_val);
    if (!return_val) gtk_widget_destroy(GTK_WIDGET(dialog));
  }

  return;
}







#ifndef AMIDE_WIN32_HACKS
static void write_voxel_size(AmitkPoint voxel_size) {
  gnome_config_push_prefix("/"PACKAGE"/");
  gnome_config_set_float("EXPORT/VoxelSizeZ", voxel_size.z);
  gnome_config_set_float("EXPORT/VoxelSizeY", voxel_size.y);
  gnome_config_set_float("EXPORT/VoxelSizeX", voxel_size.x);
  gnome_config_pop_prefix();
  gnome_config_sync();
  return;
}

static void write_inclusive_bounding_box(gboolean inclusive) {
  gnome_config_push_prefix("/"PACKAGE"/");
  gnome_config_set_int("EXPORT/InclusiveBoundingBox", inclusive);
  gnome_config_pop_prefix();
  gnome_config_sync();
  return;
}
#endif

static void change_voxel_size_cb(GtkWidget * widget, gpointer data) {

  gdouble size;
  AmitkAxis which_axis;
#ifndef AMIDE_WIN32_HACKS
  gboolean resliced;
  gboolean all_visible;
  gboolean inclusive_bounding_box;
  AmitkExportMethod method;
  gint submethod;
  AmitkPoint voxel_size;

  read_preferences(&resliced, &all_visible, &inclusive_bounding_box, &method, &submethod, &voxel_size);
#endif

  /* figure out which widget this is */
  which_axis = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "axis")); 

  size = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

  switch(which_axis) {
  case (AMITK_AXIS_X):
    voxel_size.x = size;
    break;
  case (AMITK_AXIS_Y):
    voxel_size.y = size;
    break;
  case (AMITK_AXIS_Z):
    voxel_size.z = size;
    break;
  default:
    break; /* do nothing */
  }

#ifndef AMIDE_WIN32_HACKS
  write_voxel_size(voxel_size);
#endif

  return;
}


static void recommend_voxel_size(tb_export_t * tb_export) {
  GList * data_sets;
  AmitkAxis i_axis;
#ifndef AMIDE_WIN32_HACKS
  gboolean resliced;
  gboolean all_visible;
  gboolean inclusive_bounding_box;
  AmitkExportMethod method;
  gint submethod;
  AmitkPoint voxel_size;

  read_preferences(&resliced, &all_visible, &inclusive_bounding_box, &method, &submethod, &voxel_size);
#endif

  if (all_visible) {
    data_sets = amitk_object_get_selected_children_of_type(AMITK_OBJECT(tb_export->study), 
							   AMITK_OBJECT_TYPE_DATA_SET, 
							   AMITK_SELECTION_SELECTED_0, TRUE);
    if (data_sets == NULL) {
      g_warning(_("No Data Sets are current visible"));
    } else {
      /* for all visible datasets */
      voxel_size.z = voxel_size.y = voxel_size.x = 
	amitk_data_sets_get_min_voxel_size(data_sets);
      amitk_objects_unref(data_sets);
    }

  } else if (resliced) {
    voxel_size.z = voxel_size.y = voxel_size.x = 
      point_min_dim(AMITK_DATA_SET_VOXEL_SIZE(tb_export->active_ds));

  } else {/* if not resliced */
    voxel_size = AMITK_DATA_SET_VOXEL_SIZE(tb_export->active_ds);
  }

#ifndef AMIDE_WIN32_HACKS
  write_voxel_size(voxel_size);
#endif

  /* update entries */
  for (i_axis=0; i_axis<AMITK_AXIS_NUM; i_axis++) {
    g_return_if_fail(GTK_IS_SPIN_BUTTON(tb_export->vs_spin_button[i_axis]));
    g_signal_handlers_block_by_func(G_OBJECT(tb_export->vs_spin_button[i_axis]), change_voxel_size_cb, tb_export);
				  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_export->vs_spin_button[i_axis]), point_get_component(voxel_size, i_axis));
    g_signal_handlers_unblock_by_func(G_OBJECT(tb_export->vs_spin_button[i_axis]), change_voxel_size_cb, tb_export);
  }
  
  return;
}

static void reslice_radio_buttons_cb(GtkWidget * widget, gpointer data) {

  tb_export_t * tb_export = data;
  AmitkAxis i_axis;
#ifndef AMIDE_WIN32_HACKS
  gboolean resliced;
  gboolean all_visible;
  gboolean inclusive_bounding_box;
#endif

  resliced = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "resliced"));
  all_visible = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "all_visible"));

#ifndef AMIDE_WIN32_HACKS
  gnome_config_push_prefix("/"PACKAGE"/");
  gnome_config_set_int("EXPORT/ResliceDataSet", resliced);
  gnome_config_set_int("EXPORT/AllVisibleDataSets", all_visible);

  gnome_config_pop_prefix();
  gnome_config_sync();
#endif

  /* recalculate voxel sizes */
  recommend_voxel_size(tb_export);

  /* whether or not we can change the voxel sizes */
  for (i_axis=0; i_axis<AMITK_AXIS_NUM; i_axis++) {
    gtk_widget_set_sensitive(GTK_WIDGET(tb_export->vs_spin_button[i_axis]), resliced || all_visible);
  }

  /* whether or not we can change the bounding box type */
  gtk_widget_set_sensitive(tb_export->bb_radio_button[0], resliced || all_visible);
  gtk_widget_set_sensitive(tb_export->bb_radio_button[1], resliced || all_visible);
  if (!resliced && !all_visible) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_export->bb_radio_button[0]), TRUE);
    inclusive_bounding_box=FALSE;
#ifndef AMIDE_WIN32_HACKS
    write_inclusive_bounding_box(inclusive_bounding_box);
#endif
  }

  return;
}

static void bb_radio_buttons_cb(GtkWidget * widget, gpointer data) {

#ifndef AMIDE_WIN32_HACKS
  gboolean inclusive_bounding_box;
#endif

  inclusive_bounding_box = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "inclusive_bounding_box"));

#ifndef AMIDE_WIN32_HACKS
  write_inclusive_bounding_box(inclusive_bounding_box);
#endif

  return;
}

/* function called when the export type of a data set gets changed */
static void change_export_cb(GtkWidget * widget, gpointer data) {

#ifndef AMIDE_WIN32_HACKS
  AmitkExportMethod method=0;
  gint submethod=0;
#endif
  gint counter;
  gint history;
  AmitkImportMethod i_export_method;
#ifdef AMIDE_LIBMDC_SUPPORT
  libmdc_export_t i_libmdc_export;
#endif

  /* figure out which menu item called me */
  history = gtk_option_menu_get_history (GTK_OPTION_MENU(widget));

  counter = 0;
  for (i_export_method = AMITK_EXPORT_METHOD_RAW; i_export_method < AMITK_EXPORT_METHOD_NUM; i_export_method++) {
#ifdef AMIDE_LIBMDC_SUPPORT
    if (i_export_method == AMITK_EXPORT_METHOD_LIBMDC) {
      for (i_libmdc_export=0; i_libmdc_export < LIBMDC_NUM_EXPORT_METHODS; i_libmdc_export++) {
	if (libmdc_supports(libmdc_export_to_format[i_libmdc_export])) {
	  if (counter == history) {
	    method = i_export_method;
	    submethod = libmdc_export_to_format[i_libmdc_export];
	  }
	  counter++;
	}
      }
    } else 
#endif
      {
	if (counter == history) {
	  method = i_export_method;
	  submethod = 0;
	}
	counter++;
      }  
  }

#ifndef AMIDE_WIN32_HACKS
  gnome_config_push_prefix("/"PACKAGE"/");
  gnome_config_set_int("EXPORT/Method", method);
  gnome_config_set_int("EXPORT/Submethod", submethod);

  gnome_config_pop_prefix();
  gnome_config_sync();
#endif

  return;
}


/* function to setup a dialog to allow us to choice options for rendering */
void tb_export_data_set(AmitkStudy * study, AmitkDataSet * active_ds, 
			GtkWindow * parent) {

  
  gchar * temp_string;
  GtkWidget * table;
  GtkWidget * label;
  guint table_row;
  GtkWidget * radio_button[3];
  GtkWidget * hseparator;
  GtkWidget * export_menu;
  //  GtkObject * adjustment;
#if 1
  GtkWidget * menu;
  GtkWidget * menuitem;
#endif
  //  GtkWidget * spin_buttons[3];
  gint counter;
  gint current;
  AmitkExportMethod i_export_method;
  tb_export_t * tb_export;
  AmitkAxis i_axis;
#ifdef AMIDE_LIBMDC_SUPPORT
  libmdc_export_t i_libmdc_export;
#endif
#ifndef AMIDE_WIN32_HACKS
  gboolean resliced;
  gboolean all_visible;
  gboolean inclusive_bounding_box;
  AmitkExportMethod method;
  gint submethod;
  AmitkPoint voxel_size;

  read_preferences(&resliced, &all_visible, &inclusive_bounding_box, &method, &submethod, &voxel_size); 
#endif


  /* sanity checks */
  g_return_if_fail(AMITK_IS_STUDY(study));
  g_return_if_fail(AMITK_IS_DATA_SET(active_ds));

  tb_export = tb_export_init();
  tb_export->study = AMITK_STUDY(amitk_object_ref(AMITK_OBJECT(study)));
  tb_export->active_ds = amitk_object_ref(active_ds);

  temp_string = g_strdup_printf(_("%s: Export Data Set Dialog"), PACKAGE);
  tb_export->dialog = gtk_dialog_new_with_buttons (temp_string,  parent,
							    GTK_DIALOG_DESTROY_WITH_PARENT,
							    GTK_STOCK_EXECUTE, AMITK_RESPONSE_EXECUTE,
							    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
  gtk_window_set_title(GTK_WINDOW(tb_export->dialog), temp_string);
  g_free(temp_string);

  /* setup the callbacks for the dialog */
  g_signal_connect(G_OBJECT(tb_export->dialog), "delete_event",
		   G_CALLBACK(delete_event_cb), tb_export);
  g_signal_connect(G_OBJECT(tb_export->dialog), "destroy",
		   G_CALLBACK(destroy_cb), tb_export);
  g_signal_connect(G_OBJECT(tb_export->dialog), "response", 
		   G_CALLBACK(response_cb), tb_export);

  gtk_container_set_border_width(GTK_CONTAINER(tb_export->dialog), 10);

  /* create the progress dialog */
  tb_export->progress_dialog = amitk_progress_dialog_new(GTK_WINDOW(tb_export->dialog));
  amitk_progress_dialog_set_text(AMITK_PROGRESS_DIALOG(tb_export->progress_dialog),
				 _("Exporting Data Sets"));

  /* start making the widgets for this dialog box */
  table = gtk_table_new(5,4,FALSE);
  table_row=0;
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(tb_export->dialog)->vbox), table);

  label = gtk_label_new(_("Export:"));
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 
		   table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);

  // tooltip N_("Export the data set in its original orientation (unresliced)")
  temp_string = g_strdup_printf(_("Original Orientation - %s"), AMITK_OBJECT_NAME(tb_export->active_ds));
  radio_button[0] = gtk_radio_button_new_with_label(NULL, temp_string);
  g_free(temp_string);
						    
  gtk_table_attach(GTK_TABLE(table), radio_button[0], 1,4, 
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  g_object_set_data(G_OBJECT(radio_button[0]), "resliced", GINT_TO_POINTER(FALSE));
  g_object_set_data(G_OBJECT(radio_button[0]), "all_visible", GINT_TO_POINTER(FALSE));
  table_row++;

  // tooltip N_("Export the data set in its current orientation (resliced)")
  temp_string = g_strdup_printf(_("Resliced Orientation - %s"), AMITK_OBJECT_NAME(tb_export->active_ds));
  radio_button[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_button[0])),
						    temp_string);
  g_free(temp_string);
  gtk_table_attach(GTK_TABLE(table), radio_button[1], 1,4, 
		   table_row, table_row+1,  GTK_FILL, 0, X_PADDING, Y_PADDING);
  g_object_set_data(G_OBJECT(radio_button[1]), "resliced", GINT_TO_POINTER(TRUE));
  g_object_set_data(G_OBJECT(radio_button[1]), "all_visible", GINT_TO_POINTER(FALSE));
  table_row++;

  // tooltip N_("Export all the visible data sets into a single file")
  radio_button[2] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_button[0])),
						    _("All Visible Data Sets (resliced)"));
  gtk_table_attach(GTK_TABLE(table), radio_button[2], 1,4, 
		   table_row, table_row+1,  GTK_FILL, 0, X_PADDING, Y_PADDING);
  g_object_set_data(G_OBJECT(radio_button[2]), "resliced", GINT_TO_POINTER(TRUE));
  g_object_set_data(G_OBJECT(radio_button[2]), "all_visible", GINT_TO_POINTER(TRUE));
  table_row++;

  if (!resliced && !all_visible)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button[0]), TRUE);
  else if (resliced && !all_visible)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button[1]), TRUE);
  else // all_visible
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button[2]), TRUE);

  g_signal_connect(G_OBJECT(radio_button[0]), "clicked", G_CALLBACK(reslice_radio_buttons_cb), tb_export);
  g_signal_connect(G_OBJECT(radio_button[1]), "clicked", G_CALLBACK(reslice_radio_buttons_cb), tb_export);
  g_signal_connect(G_OBJECT(radio_button[2]), "clicked", G_CALLBACK(reslice_radio_buttons_cb), tb_export);

  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(table), hseparator, 0,4,table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  label = gtk_label_new(_("export format:"));
  /* select the export type */
#if 1
  menu = gtk_menu_new();
#else
  FIXME_FOR_GTK24;
  // dialog->modality_menu = gtk_combo_box_new_text();
#endif

  counter = 0;
  current = 0;
  for (i_export_method = AMITK_EXPORT_METHOD_RAW; i_export_method < AMITK_EXPORT_METHOD_NUM; i_export_method++) {
#ifdef AMIDE_LIBMDC_SUPPORT
    if (i_export_method == AMITK_EXPORT_METHOD_LIBMDC) {
      for (i_libmdc_export=0; i_libmdc_export < LIBMDC_NUM_EXPORT_METHODS; i_libmdc_export++) {
	if (libmdc_supports(libmdc_export_to_format[i_libmdc_export])) {
#if 1
	  menuitem = gtk_menu_item_new_with_label(libmdc_export_menu_names[i_libmdc_export]);
	  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	  gtk_widget_show(menuitem);
	  // add tooltips at some point libmdc_export_menu_explanations[i_libmdc_export]
#else
	  FIXME_FOR_GTK24;
	  //	  gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->modality_menu),
	  //				    libmdc_export_menu_names[i_libmdc_export]);
#endif
	  if ((method == i_export_method) && (submethod == libmdc_export_to_format[i_libmdc_export]))
	    current = counter;
	  counter++;
	}
      }
    } else 
#endif
      {
#if 1
	menuitem = gtk_menu_item_new_with_label(amitk_export_menu_names[i_export_method]);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_show(menuitem);
	// add tooltips at some point amitk_export_menu_explanations[i_export_method],
#else
	FIXME_FOR_GTK24;
	//	gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->modality_menu),
	//				  amitk_modality_get_name(i_modality));
#endif
	
	if (method == i_export_method)
	  current = counter;
	counter++;
      }  
  }

#if 1    
  export_menu = gtk_option_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(export_menu), menu);
  gtk_widget_show(menu);
#endif
  g_signal_connect(G_OBJECT(export_menu), "changed", G_CALLBACK(change_export_cb), NULL);
  gtk_option_menu_set_history(GTK_OPTION_MENU(export_menu), current); /* done after signal attachment, in case current never got matched and is still zero */
  gtk_table_attach(GTK_TABLE(table), export_menu, 1,4, 
		   table_row,table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(export_menu);
  table_row++;
    

  /* widgets to change the voxel size of the data set */
  label = gtk_label_new(_("voxel size (mm) [x,y,z]:"));
  gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);


  for (i_axis=0; i_axis<AMITK_AXIS_NUM; i_axis++) {
    tb_export->vs_spin_button[i_axis] = 
      gtk_spin_button_new_with_range(0.0, G_MAXDOUBLE, 0.2);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tb_export->vs_spin_button[i_axis]), FALSE);
    g_object_set_data(G_OBJECT(tb_export->vs_spin_button[i_axis]), "axis", GINT_TO_POINTER(i_axis));
    g_signal_connect(G_OBJECT(tb_export->vs_spin_button[i_axis]), "changed", G_CALLBACK(change_voxel_size_cb), tb_export);

    gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(tb_export->vs_spin_button[i_axis]),i_axis+1,i_axis+2,
		     table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);
    gtk_widget_set_sensitive(GTK_WIDGET(tb_export->vs_spin_button[i_axis]), resliced || all_visible);
  }
  recommend_voxel_size(tb_export); /* updates voxel size guestimate, and updates the entry boxes */
  table_row++;

  //  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_buttons[0]),FALSE);
  //  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(spin_buttons[0]), FALSE);
  //  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin_buttons[0]), GTK_UPDATE_ALWAYS);


  label = gtk_label_new(_("bounding box:"));
  gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  // tooltip N_("Export the data set in its original orientation (unresliced)")
  tb_export->bb_radio_button[0] = gtk_radio_button_new_with_label(NULL, "tight");
						    
  gtk_table_attach(GTK_TABLE(table), tb_export->bb_radio_button[0], 1,2, 
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  g_object_set_data(G_OBJECT(tb_export->bb_radio_button[0]), "inclusive_bounding_box", GINT_TO_POINTER(FALSE));

  // tooltip N_("Export the data set in its current orientation (resliced)")
  tb_export->bb_radio_button[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(tb_export->bb_radio_button[0])),
						    "inclusive (of all data sets)");
  gtk_table_attach(GTK_TABLE(table), tb_export->bb_radio_button[1], 2,4, 
		   table_row, table_row+1,  GTK_FILL, 0, X_PADDING, Y_PADDING);
  g_object_set_data(G_OBJECT(tb_export->bb_radio_button[1]), "inclusive_bounding_box", GINT_TO_POINTER(TRUE));
  table_row++;

  if (!inclusive_bounding_box)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_export->bb_radio_button[0]), TRUE);
  else 
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_export->bb_radio_button[1]), TRUE);

  g_signal_connect(G_OBJECT(tb_export->bb_radio_button[0]), "clicked", G_CALLBACK(bb_radio_buttons_cb), NULL);
  g_signal_connect(G_OBJECT(tb_export->bb_radio_button[1]), "clicked", G_CALLBACK(bb_radio_buttons_cb), NULL);

  gtk_widget_set_sensitive(tb_export->bb_radio_button[0], resliced || all_visible);
  gtk_widget_set_sensitive(tb_export->bb_radio_button[1], resliced || all_visible);

  /* and show all our widgets */
  gtk_widget_show_all(tb_export->dialog);

  return;
}












