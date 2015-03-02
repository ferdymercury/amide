/* tb_export_data_set.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2006-2014 Andy Loening
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
#include "amide.h"
#include "amide_gconf.h"
#include "amitk_progress_dialog.h"

#include "tb_export_data_set.h"
#ifdef AMIDE_LIBMDC_SUPPORT
#include "libmdc_interface.h"
#endif
#ifdef AMIDE_LIBDCMDATA_SUPPORT
#include "dcmtk_interface.h"
#endif

#define GCONF_AMIDE_EXPORT "EXPORT"

typedef struct tb_export_t {
  AmitkStudy * study;
  AmitkDataSet * active_ds;
  AmitkPreferences * preferences;

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

    if (tb_export->preferences != NULL) {
      g_object_unref(tb_export->preferences);
      tb_export->preferences = NULL;
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
  tb_export->preferences = NULL;
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


static void read_preferences(gboolean * resliced,
			     gboolean * all_visible,
			     gboolean * inclusive_bounding_box,
			     AmitkExportMethod * method,
			     gint * submethod,
			     AmitkPoint * voxel_size) {

  if (resliced != NULL)
    *resliced = amide_gconf_get_bool(GCONF_AMIDE_EXPORT,"ResliceDataSet");
  if (all_visible != NULL)
    *all_visible = amide_gconf_get_bool(GCONF_AMIDE_EXPORT,"AllVisibleDataSets");
  if (inclusive_bounding_box != NULL)
    *inclusive_bounding_box = amide_gconf_get_bool(GCONF_AMIDE_EXPORT,"InclusiveBoundingBox");
  if (method != NULL)
    *method = amide_gconf_get_int(GCONF_AMIDE_EXPORT,"Method");
  if (submethod != NULL)
    *submethod = amide_gconf_get_int(GCONF_AMIDE_EXPORT,"Submethod");

  if (voxel_size != NULL) {
    (*voxel_size).z = amide_gconf_get_float(GCONF_AMIDE_EXPORT,"VoxelSizeZ");
    if (EQUAL_ZERO((*voxel_size).z))
      (*voxel_size).z =  1.0;
    
    (*voxel_size).y = amide_gconf_get_float(GCONF_AMIDE_EXPORT,"VoxelSizeY");
    if (EQUAL_ZERO((*voxel_size).y)) 
      (*voxel_size).y =  1.0;
    
    (*voxel_size).x = amide_gconf_get_float(GCONF_AMIDE_EXPORT,"VoxelSizeX");
    if (EQUAL_ZERO((*voxel_size).x)) 
      (*voxel_size).x =  1.0;
  }

  return;
}


/* function called when we hit "ok" on the export file dialog */
static gboolean export_data_set(tb_export_t * tb_export, gchar * filename) {

  GList * data_sets;
  AmitkVolume * bounding_box = NULL;
  gboolean resliced;
  gboolean all_visible;
  gboolean inclusive_bounding_box;
  AmitkExportMethod method;
  gint submethod;
  AmitkPoint voxel_size;
  gboolean successful = FALSE;

  g_return_val_if_fail(filename != NULL, FALSE);

  read_preferences(&resliced, &all_visible, &inclusive_bounding_box, &method, &submethod, &voxel_size);

  /* get total bounding box if needed */
  if (inclusive_bounding_box) {
    AmitkCorners corner;
    bounding_box = amitk_volume_new(); /* in base coordinate frame */
    g_return_val_if_fail(bounding_box != NULL, FALSE);
    data_sets = amitk_object_get_children_of_type(AMITK_OBJECT(tb_export->study), 
						  AMITK_OBJECT_TYPE_DATA_SET, TRUE);
    amitk_volumes_get_enclosing_corners(data_sets, AMITK_SPACE(bounding_box), corner);
    amitk_space_set_offset(AMITK_SPACE(bounding_box), corner[0]);
    amitk_volume_set_corner(bounding_box, amitk_space_b2s(AMITK_SPACE(bounding_box), corner[1]));
    amitk_objects_unref(data_sets);
  }


  if (!all_visible) {
    successful = amitk_data_set_export_to_file(tb_export->active_ds, 
					       method, submethod, filename, 
					       AMITK_OBJECT_NAME(tb_export->study),
					       resliced, voxel_size, bounding_box,
					       amitk_progress_dialog_update,
					       tb_export->progress_dialog);
  } else {
    data_sets = amitk_object_get_selected_children_of_type(AMITK_OBJECT(tb_export->study), 
							   AMITK_OBJECT_TYPE_DATA_SET, 
							   AMITK_SELECTION_SELECTED_0, TRUE);
    if (data_sets == NULL) {
      g_warning(_("No Data Sets are current visible"));
    } else {
      successful = amitk_data_sets_export_to_file(data_sets, method, submethod, filename, 
						  AMITK_OBJECT_NAME(tb_export->study),
						  voxel_size, bounding_box,
						  amitk_progress_dialog_update,
						  tb_export->progress_dialog);
      amitk_objects_unref(data_sets);
    }
  }


  if (bounding_box != NULL) 
    bounding_box = amitk_object_unref(bounding_box);

  return successful;
}



static void response_cb (GtkDialog * main_dialog, gint response_id, gpointer data) {
  
  tb_export_t * tb_export = data;
  GtkWidget * file_chooser;
  gint return_val;
  gchar * filename;
  AmitkExportMethod method;
  gboolean close_dialog=FALSE;

  switch(response_id) {
  case AMITK_RESPONSE_EXECUTE:

    /* the rest of this function runs the file selection dialog box */
    file_chooser = gtk_file_chooser_dialog_new(_("Export to File"),
					       GTK_WINDOW(main_dialog), /* parent window */
					       GTK_FILE_CHOOSER_ACTION_SAVE,
					       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					       GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					       NULL);
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(file_chooser), TRUE);
    amitk_preferences_set_file_chooser_directory(tb_export->preferences, file_chooser); /* set the default directory if applicable */

    /* for DCMTK dicom files we don't want to  complain about file existing, as we might be appending */
    read_preferences(NULL, NULL, NULL, &method, NULL, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(file_chooser), 
#ifdef AMIDE_LIBDCMDATA_SUPPORT
						   method!=AMITK_EXPORT_METHOD_DCMTK
#else
						   TRUE
#endif
						   );


    if (gtk_dialog_run(GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT) 
      filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (file_chooser));
    else
      filename = NULL;
    gtk_widget_destroy(file_chooser);

    if (filename == NULL)
      return; /* return to export dialog */
    else { /* export the dataset */
      close_dialog = export_data_set(tb_export, filename); /* close if successful */
      g_free(filename);
    }
    break;

  case GTK_RESPONSE_CANCEL:
    close_dialog = TRUE;
    break;

  default:
    break;
  }

  if (close_dialog) {
    g_signal_emit_by_name(G_OBJECT(main_dialog), "delete_event", NULL, &return_val);
    if (!return_val) gtk_widget_destroy(GTK_WIDGET(main_dialog));
  }

  return;
}







static void write_voxel_size(AmitkPoint voxel_size) {
  amide_gconf_set_float(GCONF_AMIDE_EXPORT,"VoxelSizeZ", voxel_size.z);
  amide_gconf_set_float(GCONF_AMIDE_EXPORT,"VoxelSizeY", voxel_size.y);
  amide_gconf_set_float(GCONF_AMIDE_EXPORT,"VoxelSizeX", voxel_size.x);
  return;
}

static void write_inclusive_bounding_box(gboolean inclusive) {
  amide_gconf_set_bool(GCONF_AMIDE_EXPORT,"InclusiveBoundingBox", inclusive);
  return;
}

static void change_voxel_size_cb(GtkWidget * widget, gpointer data) {

  gdouble size;
  AmitkAxis which_axis;
  gboolean resliced;
  gboolean all_visible;
  gboolean inclusive_bounding_box;
  AmitkExportMethod method;
  gint submethod;
  AmitkPoint voxel_size;

  read_preferences(&resliced, &all_visible, &inclusive_bounding_box, &method, &submethod, &voxel_size);

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


  write_voxel_size(voxel_size);

  return;
}


static void recommend_voxel_size(tb_export_t * tb_export) {
  GList * data_sets;
  AmitkAxis i_axis;
  gboolean resliced;
  gboolean all_visible;
  gboolean inclusive_bounding_box;
  AmitkExportMethod method;
  gint submethod;
  AmitkPoint voxel_size;

  read_preferences(&resliced, &all_visible, &inclusive_bounding_box, &method, &submethod, &voxel_size);

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

  write_voxel_size(voxel_size);

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
  gboolean resliced;
  gboolean all_visible;
  gboolean inclusive_bounding_box;

  resliced = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "resliced"));
  all_visible = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "all_visible"));

  amide_gconf_set_bool(GCONF_AMIDE_EXPORT,"ResliceDataSet", resliced);
  amide_gconf_set_bool(GCONF_AMIDE_EXPORT,"AllVisibleDataSets", all_visible);

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
    write_inclusive_bounding_box(inclusive_bounding_box);
  }

  return;
}

static void bb_radio_buttons_cb(GtkWidget * widget, gpointer data) {

  gboolean inclusive_bounding_box;

  inclusive_bounding_box = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "inclusive_bounding_box"));

  write_inclusive_bounding_box(inclusive_bounding_box);

  return;
}

/* function called when the export type of a data set gets changed */
static void change_export_cb(GtkWidget * widget, gpointer data) {

  AmitkExportMethod method=0;
  gint submethod=0;
  gint counter;
  gint combo_method;
  AmitkImportMethod i_export_method;
#ifdef AMIDE_LIBMDC_SUPPORT
  libmdc_export_t i_libmdc_export;
#endif

  combo_method = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

  counter = 0;
  for (i_export_method = AMITK_EXPORT_METHOD_RAW; i_export_method < AMITK_EXPORT_METHOD_NUM; i_export_method++) {
#ifdef AMIDE_LIBMDC_SUPPORT
    if (i_export_method == AMITK_EXPORT_METHOD_LIBMDC) {
      for (i_libmdc_export=0; i_libmdc_export < LIBMDC_NUM_EXPORT_METHODS; i_libmdc_export++) {
	if (libmdc_supports(libmdc_export_to_format[i_libmdc_export])) {
	  if (counter == combo_method) {
	    method = i_export_method;
	    submethod = libmdc_export_to_format[i_libmdc_export];
	  }
	  counter++;
	}
      }
    } else 
#endif
      {
	if (counter == combo_method) {
	  method = i_export_method;
	  submethod = 0;
	}
	counter++;
      }  
  }

  amide_gconf_set_int(GCONF_AMIDE_EXPORT,"Method", method);
  amide_gconf_set_int(GCONF_AMIDE_EXPORT,"Submethod", submethod);

  return;
}


/* function to setup a dialog to allow us to choice options for rendering */
void tb_export_data_set(AmitkStudy * study, 
			AmitkDataSet * active_ds, 
			AmitkPreferences * preferences,
			GtkWindow * parent) {

  
  gchar * temp_string;
  GtkWidget * table;
  GtkWidget * label;
  guint table_row;
  GtkWidget * radio_button[3];
  GtkWidget * hseparator;
  GtkWidget * export_menu;
  //  GtkObject * adjustment;
  //  GtkWidget * spin_buttons[3];
  gint counter;
  gint current;
  AmitkExportMethod i_export_method;
  tb_export_t * tb_export;
  AmitkAxis i_axis;
#ifdef AMIDE_LIBMDC_SUPPORT
  libmdc_export_t i_libmdc_export;
#endif
  gboolean resliced;
  gboolean all_visible;
  gboolean inclusive_bounding_box;
  AmitkExportMethod method;
  gint submethod;
  AmitkPoint voxel_size;

  read_preferences(&resliced, &all_visible, &inclusive_bounding_box, &method, &submethod, &voxel_size); 


  /* sanity checks */
  g_return_if_fail(AMITK_IS_STUDY(study));
  g_return_if_fail(AMITK_IS_DATA_SET(active_ds));

  tb_export = tb_export_init();
  tb_export->study = AMITK_STUDY(amitk_object_ref(AMITK_OBJECT(study)));
  tb_export->active_ds = amitk_object_ref(active_ds);
  tb_export->preferences = g_object_ref(preferences);

  temp_string = g_strdup_printf(_("%s: Export Data Set Dialog"), PACKAGE);
  tb_export->dialog = gtk_dialog_new_with_buttons (temp_string,  parent,
						   GTK_DIALOG_DESTROY_WITH_PARENT,
						   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
						   GTK_STOCK_EXECUTE, AMITK_RESPONSE_EXECUTE,
						   NULL);
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
  gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  /* select the export type */
  export_menu = gtk_combo_box_new_text();

  counter = 0;
  current = 0;
  for (i_export_method = AMITK_EXPORT_METHOD_RAW; i_export_method < AMITK_EXPORT_METHOD_NUM; i_export_method++) {
#ifdef AMIDE_LIBMDC_SUPPORT
    if (i_export_method == AMITK_EXPORT_METHOD_LIBMDC) {
      for (i_libmdc_export=0; i_libmdc_export < LIBMDC_NUM_EXPORT_METHODS; i_libmdc_export++) {
	if (libmdc_supports(libmdc_export_to_format[i_libmdc_export])) {
	  gtk_combo_box_append_text(GTK_COMBO_BOX(export_menu),
				    libmdc_export_menu_names[i_libmdc_export]);
	  // add tooltips at some point libmdc_export_menu_explanations[i_libmdc_export]
	  if ((method == i_export_method) && (submethod == libmdc_export_to_format[i_libmdc_export]))
	    current = counter;
	  counter++;
	}
      }
    } else 
#endif
      {
	gtk_combo_box_append_text(GTK_COMBO_BOX(export_menu),
				  amitk_export_menu_names[i_export_method]);
	// add tooltips at some point amitk_export_menu_explanations[i_export_method],
	
	if (method == i_export_method)
	  current = counter;
	counter++;
      }  
  }

  g_signal_connect(G_OBJECT(export_menu), "changed", G_CALLBACK(change_export_cb), NULL);
  gtk_combo_box_set_active(GTK_COMBO_BOX(export_menu), current); /* done after signal attachment, in case current never got matched and is still zero */
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
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tb_export->vs_spin_button[i_axis]), 4);
    g_object_set_data(G_OBJECT(tb_export->vs_spin_button[i_axis]), "axis", GINT_TO_POINTER(i_axis));
    g_signal_connect(G_OBJECT(tb_export->vs_spin_button[i_axis]), "value_changed", G_CALLBACK(change_voxel_size_cb), tb_export);

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












