/* ui_study_cb.c
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
#include <sys/stat.h> /* needed for stat */
#include <sys/types.h> /* needed for dirent.h on mac os */
#include <dirent.h>
#include <string.h>
#include "amide.h"
#include "amitk_threshold.h"
#include "amitk_canvas.h"
#include "amitk_tree_view.h"
#include "ui_common.h"
#include "ui_render.h"
#include "ui_series.h"
#include "ui_study.h"
#include "ui_study_cb.h"
#include "ui_preferences_dialog.h"
#include "amitk_object_dialog.h"
#include "amitk_progress_dialog.h"
#include "ui_gate_dialog.h"
#include "ui_time_dialog.h"
#include "tb_export_data_set.h"
#include "tb_distance.h"
#include "tb_fly_through.h"
#include "tb_alignment.h"
#include "tb_crop.h"
#include "tb_fads.h"
#include "tb_filter.h"
#include "tb_math.h"
#include "tb_profile.h"
#include "tb_roi_analysis.h"


static gchar * no_active_ds = N_("No data set is currently marked as active");
#ifndef AMIDE_LIBGSL_SUPPORT
static gchar * no_gsl = N_("This wizard requires compiled in support from the GNU Scientific Library (libgsl), which this copy of AMIDE does not have.");
#endif


static void object_picker(ui_study_t * ui_study, AmitkStudy * import_study) {

  GtkWidget * dialog;
  gchar * temp_string;
  GtkWidget * table;
  guint table_row;
  GtkWidget * tree_view;
  GtkWidget * scrolled;
  GList * selected_objects;
  GList * temp_objects;
  gint return_val;

  /* unselect all objects in study */
  amitk_object_set_selected(AMITK_OBJECT(import_study), FALSE, AMITK_SELECTION_ALL);

  temp_string = g_strdup_printf(_("%s: Pick Object(s) to Import"), PACKAGE);
  dialog = gtk_dialog_new_with_buttons (temp_string,  ui_study->window,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_EXECUTE, AMITK_RESPONSE_EXECUTE,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CLOSE, NULL);
  gtk_window_set_title(GTK_WINDOW(dialog), temp_string);
  g_free(temp_string);

  /* setup the callbacks for the dialog */
  g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(ui_common_init_dialog_response_cb), NULL);

  gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);

  /* start making the widgets for this dialog box */
  table = gtk_table_new(5,2,FALSE);
  table_row=0;
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);

  tree_view = amitk_tree_view_new(AMITK_TREE_VIEW_MODE_MULTIPLE_SELECTION,NULL, NULL);
  g_object_set_data(G_OBJECT(dialog), "tree_view", tree_view);
  amitk_tree_view_set_study(AMITK_TREE_VIEW(tree_view), import_study);
  amitk_tree_view_expand_object(AMITK_TREE_VIEW(tree_view), AMITK_OBJECT(import_study));

  /* make a scrolled area for the tree */
  scrolled = gtk_scrolled_window_new(NULL,NULL);  
  gtk_widget_set_size_request(scrolled,250,250);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), 
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), 
					tree_view);
  gtk_table_attach(GTK_TABLE(table), scrolled, 0,2,
		   table_row, table_row+1,GTK_FILL, GTK_FILL | GTK_EXPAND, X_PADDING, Y_PADDING);
  table_row++;

  /* and show all our widgets */
  gtk_widget_show_all(dialog);

  /* and wait for the question to return */
  return_val = gtk_dialog_run(GTK_DIALOG(dialog));

  /* get which objects were selected */
  selected_objects = ui_common_init_dialog_selected_objects(dialog);
  gtk_widget_destroy(dialog);

  /* add on the new objects i fwe want */
  if (return_val == AMITK_RESPONSE_EXECUTE) {
    temp_objects = selected_objects;
    while (temp_objects != NULL) {
      amitk_object_remove_child(AMITK_OBJECT(import_study), AMITK_OBJECT(temp_objects->data));
      amitk_object_add_child(AMITK_OBJECT(ui_study->study), AMITK_OBJECT(temp_objects->data));
      temp_objects = temp_objects->next;
    }
  }
  selected_objects = amitk_objects_unref(selected_objects);

}



gboolean xif_files_filter(const GtkFileFilterInfo *filter_info, gpointer data) {

  g_return_val_if_fail(filter_info->filename != NULL, FALSE);

  // amitk_is_xif_directory(filter_info->filename, NULL, NULL)
  return amitk_is_xif_flat_file(filter_info->filename, NULL, NULL);
}

void create_xif_filters(GtkFileChooser * file_chooser) {
  GtkFileFilter * filter;

  /* create the filter for .xif files */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("XIF Files"));
  //	gtk_file_filter_add_pattern (filter, "*");
  gtk_file_filter_add_custom(filter, GTK_FILE_FILTER_FILENAME, xif_files_filter, NULL, NULL);
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(file_chooser), filter);
  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (file_chooser), filter); /* xif filter is default */
  
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All Files"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_chooser), filter);

  return;
}

void read_xif(ui_study_t * ui_study, gboolean import_object, gboolean as_directory, gboolean recovery_mode) {
  GtkWidget * file_chooser;
  AmitkStudy * study;
  gchar * filename;

  /* get the name of the file to import */
  file_chooser = gtk_file_chooser_dialog_new (recovery_mode ? _("Recover AMIDE XIF FILE") : _("Open AMIDE XIF File"),
					      GTK_WINDOW(ui_study->window), /* parent window */
					      as_directory ? GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER : GTK_FILE_CHOOSER_ACTION_OPEN, 
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					      NULL);
  gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(file_chooser), TRUE);
  amitk_preferences_set_file_chooser_directory(ui_study->preferences, file_chooser); /* set the default directory if applicable */

  if (!as_directory) 
    create_xif_filters(GTK_FILE_CHOOSER(file_chooser));  /* only include *.xif in the list by default */

  if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT) 
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));
  else
    filename = NULL;
  gtk_widget_destroy (file_chooser);

  if (filename == NULL) return;
  if (!ui_common_check_filename(filename)) {
    g_warning(_("Inappropriate filename: %s"),filename);
    g_free(filename);
    return;
  }

  ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);

  /* try loading the study into memory */
  if (recovery_mode) {
    study = amitk_study_recover_xml(filename, ui_study->preferences);
    if (study == NULL) g_warning(_("error recovering study: %s"),filename);
  } else {
    study = amitk_study_load_xml(filename);
    if (study == NULL) g_warning(_("error loading study: %s"),filename);
  }

  if (study != NULL) {
    if (!import_object) {
      /* setup the study window */
      if (ui_study->study_virgin)
	ui_study_set_study(ui_study, study);
      else
	ui_study_create(study, ui_study->preferences);
    } else {
      object_picker(ui_study, study);
    }
    amitk_object_unref(study);
  }

  ui_common_remove_wait_cursor(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);
  g_free(filename);
} 

/* function to load a study into the  study widget */
void ui_study_cb_open_xif_file(GtkAction * action, gpointer data) {
  ui_study_t * ui_study=data;
  read_xif(ui_study, FALSE, FALSE, FALSE);
  return;
}
void ui_study_cb_open_xif_dir(GtkAction * action, gpointer data) {
  ui_study_t * ui_study=data;
  read_xif(ui_study, FALSE, TRUE, FALSE);
  return;
}

/* function to load an object from a pre-existing study into the current study widget */
void ui_study_cb_import_object_from_xif_file(GtkAction * action, gpointer data) {
  ui_study_t * ui_study = data;
  read_xif(ui_study, TRUE, FALSE, FALSE);
  return;
}
void ui_study_cb_import_object_from_xif_dir(GtkAction * action, gpointer data) {
  ui_study_t * ui_study = data;
  read_xif(ui_study, TRUE, TRUE, FALSE);
  return;
}

/* try to recover portions of a study file */
void ui_study_cb_recover_xif_file(GtkAction * action, gpointer data) {
  ui_study_t * ui_study=data;
  read_xif(ui_study, FALSE, FALSE, TRUE);
  return;
}


/* function to create a new study widget */
void ui_study_cb_new_study(GtkAction * action, gpointer data) {
  ui_study_t * ui_study = data;
  ui_study_create(NULL, ui_study->preferences);
  return;
}



/* returned save_filename needs to be free'd */
static gchar * verify_save_name(GtkWindow * parent, const gchar * filename) {
  gchar * save_filename;
  gchar * prev_filename;
  gchar ** frags1=NULL;
  gchar ** frags2=NULL;
  struct stat file_info;
  GtkWidget * question;
  gint return_val;


  g_return_val_if_fail(filename != NULL, NULL);

  /* make sure the filename ends with .xif */
  save_filename = g_strdup(filename);
  g_strreverse(save_filename);
  frags1 = g_strsplit(save_filename, ".", 2);
  g_strreverse(save_filename);
  g_strreverse(frags1[0]);
  frags2 = g_strsplit(frags1[0], G_DIR_SEPARATOR_S, -1);
  if (g_ascii_strcasecmp(frags2[0], "xif") != 0) {
    prev_filename = save_filename;
    save_filename = g_strdup_printf("%s%s",prev_filename, ".xif");
    g_free(prev_filename);
  }    
  g_strfreev(frags2);
  g_strfreev(frags1);


  /* check with user if filename already exists */
  if (stat(save_filename, &file_info) == 0) {
    /* check if it's okay to writeover the file */
    question = gtk_message_dialog_new(parent,
				      GTK_DIALOG_DESTROY_WITH_PARENT,
				      GTK_MESSAGE_QUESTION,
				      GTK_BUTTONS_OK_CANCEL,
				      _("Overwrite file: %s"), save_filename);

    /* and wait for the question to return */
    return_val = gtk_dialog_run(GTK_DIALOG(question));
    
    gtk_widget_destroy(question);
    if (return_val != GTK_RESPONSE_OK) {
      return NULL; /* we don't want to overwrite the file.... */
    }
  }
  /* unlinking the file doesn't occur here */

  return save_filename;
}


void save_xif(ui_study_t * ui_study, gboolean as_directory) {
  GtkWidget * file_chooser;
  gchar * initial_filename;
  gchar * final_filename;
  gchar * temp_str;

  /* get the name of the file to save */
  file_chooser = gtk_file_chooser_dialog_new (_("Save AMIDE XIF File"),
					      GTK_WINDOW(ui_study->window), /* parent window */
					      as_directory ? GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER : GTK_FILE_CHOOSER_ACTION_SAVE, 
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					      NULL);
  gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(file_chooser), TRUE);

  if (!as_directory) 
    create_xif_filters(GTK_FILE_CHOOSER(file_chooser)); /* only include *.xif in the list by default */

  /* take a guess at the filename */
  if (AMITK_STUDY_FILENAME(ui_study->study) == NULL) {
    if (AMITK_OBJECT_NAME(ui_study->study) != NULL) {
      initial_filename = g_strdup_printf("%s.xif",AMITK_OBJECT_NAME(ui_study->study));
      gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(file_chooser), initial_filename);
      g_free(initial_filename);
    } else {
      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(file_chooser), "Untitled.xif");
    }

    temp_str = ui_common_suggest_path();
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser), temp_str);
    g_free(temp_str);

  } else { /* don't already have a filename */

    if (!as_directory) {
      gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(file_chooser), AMITK_STUDY_FILENAME(ui_study->study));
    } else { /* as_directory */
      /* have to do all this crap, as gtk_file_chooser_set_filename doesn't play well with GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER */
      temp_str = g_path_get_basename(AMITK_STUDY_FILENAME(ui_study->study));
      gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(file_chooser), temp_str);
      g_free(temp_str);

      temp_str = g_path_get_dirname(AMITK_STUDY_FILENAME(ui_study->study));
      gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser), temp_str);
      g_free(temp_str);

    }
  }  

  if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT) 
    initial_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));
  else
    initial_filename = NULL;
  gtk_widget_destroy (file_chooser);
  if (initial_filename == NULL) return;

  /* sanity checks */
  if (!ui_common_check_filename(initial_filename)) {
    g_warning(_("Inappropriate filename: %s"),initial_filename);
    g_free(initial_filename);
    return;
  }

  /* verify the file name is kosher, and if we really want to overwrite */
  if (!as_directory) {
    final_filename = verify_save_name(GTK_WINDOW(ui_study->window), initial_filename);
    g_free(initial_filename);
    if (final_filename == NULL) return;
  } else {
    /* we don't force the user to end the directory name with .xif
       this is mainly because gtk_file_chooser already creates the directory (annoying) */
    final_filename = initial_filename;
    initial_filename = NULL;
  }

  ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);

  /* allright, save our study */
  if (amitk_study_save_xml(ui_study->study, final_filename, as_directory) == FALSE) {
    g_warning(_("Failure Saving File: %s"),final_filename);
  } else {

    /* indicate no new changes */
    ui_study->study_altered=FALSE;
    ui_study_update_title(ui_study);
  }

  ui_common_remove_wait_cursor(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);
  ui_common_set_last_path_used(final_filename);
  g_free(final_filename);
}

void ui_study_cb_save_as_xif_file(GtkAction * action, gpointer data) {
  ui_study_t * ui_study = data;
  save_xif(ui_study, FALSE);
  return;
}
void ui_study_cb_save_as_xif_dir(GtkAction * action, gpointer data) {
  ui_study_t * ui_study = data;
  save_xif(ui_study, TRUE);
  return;
}

/* function to selection which file to import */
void ui_study_cb_import(GtkAction * action, gpointer data) {

  ui_study_t * ui_study = data;
  GtkWidget * file_chooser;
  AmitkImportMethod method;
  int submethod;
  gchar * filename;
  gchar * studyname = NULL;
  GList * import_data_sets;
  AmitkDataSet * import_ds;

  /* get the name of the file to import */
  file_chooser = gtk_file_chooser_dialog_new (_("Import File"),
					      GTK_WINDOW(ui_study->window), /* parent window */
					      GTK_FILE_CHOOSER_ACTION_OPEN,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					      NULL);
  gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(file_chooser), TRUE);
  amitk_preferences_set_file_chooser_directory(ui_study->preferences, file_chooser); /* set the default directory if applicable */

  if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT) 
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));
  else
    filename = NULL;
  gtk_widget_destroy (file_chooser);

  if (filename == NULL) return;
  if (!ui_common_check_filename(filename)) {
    g_warning(_("Inappropriate Filename: %s"), filename);
    g_free(filename);
    return;
  }

#ifdef AMIDE_DEBUG
  g_print("file to import: %s\n",filename);
#endif

  /* method we're trying to use to read in the file */
  method = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(action), "method"));
  submethod = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(action), "submethod"));
  
  ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);

  /* now, what we need to do if we've successfully gotten an image filename */
  if ((import_data_sets = amitk_data_set_import_file(method, submethod, filename, &studyname,
						     ui_study->preferences, amitk_progress_dialog_update, 
						     ui_study->progress_dialog)) != NULL) {
    
    if (studyname != NULL) {
      amitk_study_suggest_name(ui_study->study, studyname);
      g_free(studyname);
    }


    while (import_data_sets != NULL) {
      import_ds = import_data_sets->data;
#ifdef AMIDE_DEBUG
      g_print("imported data set name %s\n",AMITK_OBJECT_NAME(import_ds));
#endif
      
      amitk_object_add_child(AMITK_OBJECT(ui_study->study), 
			     AMITK_OBJECT(import_ds)); /* this adds a reference to the data set*/
      import_data_sets = g_list_remove(import_data_sets, import_ds);
      amitk_object_unref(import_ds); /* so remove a reference */
    }
  } else {
    g_warning(_("Could not import data sets from file %s\n"), filename);
  }

  ui_common_remove_wait_cursor(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);

  ui_common_set_last_path_used(filename);
  g_free(filename);

  return;
}


    

/* function to selection which file to export to */
void ui_study_cb_export_data_set(GtkAction * action, gpointer data) {
  ui_study_t * ui_study = data;

  if (!AMITK_IS_DATA_SET(ui_study->active_object)) {
    g_warning(_("There's currently no active data set to export"));
    return;
  }

  /* let the user input rendering options */
  tb_export_data_set(ui_study->study,
		     AMITK_DATA_SET(ui_study->active_object),
		     ui_study->preferences,
		     ui_study->window);

  return;
}

/* function to save a view as an external data format */
void ui_study_cb_export_view(GtkAction * action, gpointer data) {
  
  ui_study_t * ui_study = data;
  GList * current_data_sets;
  GList * temp_sets;
  GtkWidget * file_chooser;
  gchar * filename;
  gchar * data_set_names = NULL;
  AmitkView view;
  amide_real_t upper, lower;
  AmitkPoint temp_p;
  AmitkVolume * canvas_volume;
  GdkPixbuf * pixbuf;
  gboolean save_as_png=FALSE;
  gint length;
  const gchar * extension;
  gboolean return_val;

  current_data_sets = amitk_object_get_selected_children_of_type(AMITK_OBJECT(ui_study->study), 
								 AMITK_OBJECT_TYPE_DATA_SET, 
								 AMITK_SELECTION_SELECTED_0, TRUE);
  if (current_data_sets == NULL) return;

  file_chooser = gtk_file_chooser_dialog_new(_("Export to File"),
					     GTK_WINDOW(ui_study->window), /* parent window */
					     GTK_FILE_CHOOSER_ACTION_SAVE,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					     NULL);
  gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(file_chooser), TRUE);
  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER (file_chooser), TRUE);
  amitk_preferences_set_file_chooser_directory(ui_study->preferences, file_chooser); /* set the default directory if applicable */


  view = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(action), "view"));

  if (ui_study->canvas[AMITK_VIEW_MODE_SINGLE][view] == NULL) return;

  canvas_volume = AMITK_CANVAS(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][view])->volume;
  g_return_if_fail(canvas_volume != NULL);

  /* translate the center so that the z coordinate corresponds to depth in this view */
  temp_p = amitk_space_b2s(AMITK_SPACE(canvas_volume), AMITK_STUDY_VIEW_CENTER(ui_study->study));

  /* figure out the top and bottom of this slice */
  upper = temp_p.z + AMITK_STUDY_VIEW_THICKNESS(ui_study->study)/2.0;
  lower = temp_p.z - AMITK_STUDY_VIEW_THICKNESS(ui_study->study)/2.0;

  /* take a guess at the filename */
  data_set_names = g_strdup(AMITK_OBJECT_NAME(current_data_sets->data));
  temp_sets = current_data_sets->next;
  while (temp_sets != NULL) {
    filename = g_strdup_printf("%s+%s",data_set_names, 
				  AMITK_OBJECT_NAME(temp_sets->data));
    g_free(data_set_names);
    data_set_names = filename;
    temp_sets = temp_sets->next;
  }
  filename = g_strdup_printf("%s_%s_%s_%3.1f-%3.1f.jpg", 
			     AMITK_OBJECT_NAME(ui_study->study),
			     data_set_names,
			     amitk_view_get_name(view),
			     lower,
			     upper);
  gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(file_chooser), filename);
  g_free(data_set_names);
  g_free(filename); 
  amitk_objects_unref(current_data_sets);


  /* run the file chooser dialog */
  if (gtk_dialog_run(GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT) 
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (file_chooser));
  else
    filename = NULL;
  gtk_widget_destroy (file_chooser);
  if (filename == NULL) return;
  
  /* check if we want a png */
  length = strlen(filename);
  if (length > 4) {
    extension = filename + length-4;
    if (g_ascii_strncasecmp(extension, ".png", 4)==0) {
      save_as_png = TRUE;
    }
  }

  /* get a pixbuf of the canvas */
  pixbuf = amitk_canvas_get_pixbuf(AMITK_CANVAS(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][view]));
  if (pixbuf == NULL) {
    g_warning(_("Canvas failed to return a valid image\n"));
  } else {

    if (save_as_png)
      return_val = gdk_pixbuf_save (pixbuf, filename, "png", NULL, NULL);
    else
      return_val = gdk_pixbuf_save (pixbuf, filename, "jpeg", NULL, "quality", "100", NULL);

    if (!return_val)
      g_warning(_("Failure Saving File: %s"),filename);

    g_object_unref(pixbuf);
  }

  g_free(filename);
  return;
}




/* callback generally attached to the entry_notify_event */
gboolean ui_study_cb_update_help_info(GtkWidget * widget, GdkEventCrossing * event, gpointer data) {
  ui_study_t * ui_study = data;
  AmitkHelpInfo which_info;

  which_info = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "which_help"));
  ui_study_update_help_info(ui_study, which_info, AMITK_STUDY_VIEW_CENTER(ui_study->study), 0.0);

  return FALSE;
}



void ui_study_cb_canvas_help_event(GtkWidget * canvas,  AmitkHelpInfo help_type,
				   AmitkPoint *location, amide_data_t value,  gpointer data) {
  ui_study_t * ui_study = data;
  ui_study_update_help_info(ui_study, help_type, *location, value);
  return;
}

void ui_study_cb_canvas_view_changing(GtkWidget * canvas, AmitkPoint *position,
				      amide_real_t thickness, gpointer data) {


  ui_study_t * ui_study = data;
  AmitkView i_view;
  AmitkViewMode i_view_mode;

  g_return_if_fail(ui_study->study != NULL);

  /* update the other canvases accordingly */
  for (i_view_mode=0; i_view_mode <= AMITK_STUDY_VIEW_MODE(ui_study->study); i_view_mode++)
    for (i_view=0; i_view<AMITK_VIEW_NUM; i_view++)
      if (ui_study->canvas[i_view_mode][i_view] != NULL)
	if (canvas != ui_study->canvas[i_view_mode][i_view])
	  amitk_canvas_update_target(AMITK_CANVAS(ui_study->canvas[i_view_mode][i_view]), 
				     AMITK_CANVAS_TARGET_ACTION_SHOW,*position, thickness);

  if (!REAL_EQUAL(AMITK_STUDY_VIEW_THICKNESS(ui_study->study), thickness)) 
    ui_study_update_thickness(ui_study, thickness);

  return;
}

void ui_study_cb_canvas_view_changed(GtkWidget * canvas, AmitkPoint *position,
				     amide_real_t thickness, gpointer data) {

  ui_study_t * ui_study = data;
  AmitkView i_view;
  AmitkViewMode i_view_mode;

  g_return_if_fail(ui_study->study != NULL);

  amitk_study_set_view_thickness(ui_study->study, thickness);
  amitk_study_set_view_center(ui_study->study, *position);

  /* update the other canvases accordingly */
  for (i_view_mode=0; i_view_mode <= AMITK_STUDY_VIEW_MODE(ui_study->study); i_view_mode++) {
    for (i_view=0; i_view<AMITK_VIEW_NUM; i_view++)
      if (ui_study->canvas[i_view_mode][i_view] != NULL)
  	amitk_canvas_update_target(AMITK_CANVAS(ui_study->canvas[i_view_mode][i_view]), 
  				   AMITK_CANVAS_TARGET_ACTION_HIDE,*position, thickness);
  }

  return;
}




void ui_study_cb_canvas_erase_volume(GtkWidget * canvas, AmitkRoi * roi, 
				     gboolean outside, gpointer data) {
  ui_study_t * ui_study = data;
  GtkWidget * question;
  gint return_val;

  if (!AMITK_IS_DATA_SET(ui_study->active_object)) {
    g_warning(_("no active data set to erase from"));
    return;
  }

  /* make sure we really want to delete */
  question = gtk_message_dialog_new(ui_study->window,
				    GTK_DIALOG_DESTROY_WITH_PARENT,
				    GTK_MESSAGE_QUESTION,
				    GTK_BUTTONS_OK_CANCEL,
				    _("Do you really wish to erase the data set %s\n    to the ROI: %s\n     on the data set: %s\nThis step is irreversible\nThe minimum threshold value: %5.3f\n    will be used to fill in the volume"),
				    outside ? _("exterior") : _("interior"),
				    AMITK_OBJECT_NAME(roi),
				    AMITK_OBJECT_NAME(ui_study->active_object),
				    AMITK_DATA_SET_THRESHOLD_MIN(ui_study->active_object,0));
  
  /* and wait for the question to return */
  return_val = gtk_dialog_run(GTK_DIALOG(question));

  gtk_widget_destroy(question);
  if (return_val != GTK_RESPONSE_OK)
    return; /* cancel */

  amitk_roi_erase_volume(roi, AMITK_DATA_SET(ui_study->active_object), outside,
			 amitk_progress_dialog_update, ui_study->progress_dialog);
  
  return;
}

void ui_study_cb_canvas_new_object(GtkWidget * canvas, AmitkObject * parent, AmitkObjectType type,
				   AmitkPoint *position, gpointer data) {

  ui_study_t * ui_study = data;

  /* only handles fiducial marks currently */
  g_return_if_fail(type == AMITK_OBJECT_TYPE_FIDUCIAL_MARK); 

  ui_study_add_fiducial_mark(ui_study, parent, TRUE, *position);

  return;
}

void ui_study_cb_tree_view_activate_object(GtkWidget * tree_view, AmitkObject * object, gpointer data) {

  ui_study_t * ui_study = data;
  AmitkPoint center;

  if (object == NULL) {
    ui_study_make_active_object(ui_study, object);

  } else if (AMITK_IS_DATA_SET(object) || AMITK_IS_STUDY(object)) {
    ui_study_make_active_object(ui_study, object);

  } else if (AMITK_IS_ROI(object)) {
    if (!AMITK_ROI_UNDRAWN(AMITK_ROI(object))) {
      center = amitk_volume_get_center(AMITK_VOLUME(object));
      if (!POINT_EQUAL(center, AMITK_STUDY_VIEW_CENTER(ui_study->study)))
	amitk_study_set_view_center(ui_study->study, center);
    }
  } else if (AMITK_IS_FIDUCIAL_MARK(object)) {
    center = AMITK_FIDUCIAL_MARK_GET(object);
    if ( !POINT_EQUAL(center, AMITK_STUDY_VIEW_CENTER(ui_study->study)))
      amitk_study_set_view_center(ui_study->study, center);
  }
}


void ui_study_cb_tree_view_popup_object(GtkWidget * tree_view, AmitkObject * object, gpointer data) {

  ui_study_t * ui_study = data;
  GtkWidget * dialog;

  if (AMITK_IS_DATA_SET(object)) {
    ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);
    if (ui_study->threshold_dialog != NULL) {
      if (amitk_threshold_dialog_data_set(AMITK_THRESHOLD_DIALOG(ui_study->threshold_dialog)) == 
	  AMITK_DATA_SET(object)) {
	gtk_widget_destroy(ui_study->threshold_dialog);
	ui_study->threshold_dialog = NULL;
      }
    }
  }

  dialog = amitk_object_dialog_new(object);

  if (AMITK_IS_DATA_SET(object))
    ui_common_remove_wait_cursor(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);

  if (dialog != NULL) 
    gtk_widget_show(dialog);
  else /* already up ,pop the window to the top */
    gtk_window_present(GTK_WINDOW(object->dialog));

  return;
}

void ui_study_cb_tree_view_add_object(GtkWidget * tree_view, AmitkObject * parent, 
				      AmitkObjectType object_type, AmitkRoiType roi_type, gpointer data) {
  ui_study_t * ui_study = data;

  if (parent == NULL)
    parent = AMITK_OBJECT(ui_study->study);

  if (object_type == AMITK_OBJECT_TYPE_FIDUCIAL_MARK)
    ui_study_add_fiducial_mark(ui_study, parent, TRUE,AMITK_STUDY_VIEW_CENTER(ui_study->study));
  else if (object_type == AMITK_OBJECT_TYPE_ROI)
    ui_study_add_roi(ui_study, parent, roi_type);


  return;
}

void ui_study_cb_tree_view_delete_object(GtkWidget * tree_view, AmitkObject * object, gpointer data) {
  ui_study_t * ui_study = data;
  GtkWidget * question;
  gint return_val;

  g_return_if_fail(AMITK_IS_OBJECT(object));

  question = gtk_message_dialog_new(ui_study->window,
				    GTK_DIALOG_DESTROY_WITH_PARENT,
				    GTK_MESSAGE_QUESTION,
				    GTK_BUTTONS_OK_CANCEL,
				    _("Do you really want to delete: %s%s"),
				    AMITK_OBJECT_NAME(object),
				    AMITK_OBJECT_CHILDREN(object) != NULL ? 
				    _("\nand its children") : "");
  
  /* and wait for the question to return */
  return_val = gtk_dialog_run(GTK_DIALOG(question));

  gtk_widget_destroy(question);
  if (return_val != GTK_RESPONSE_OK)
    return; 

  amitk_object_remove_child(AMITK_OBJECT_PARENT(object), object);

  if (ui_study->study_altered != TRUE) {
    ui_study->study_virgin=FALSE;
    ui_study->study_altered=TRUE;
    ui_study_update_title(ui_study);
  }

  return;
}


void ui_study_cb_tree_view_help_event(GtkWidget * widget, AmitkHelpInfo help_type, gpointer data) {
  ui_study_t * ui_study = data;
  ui_study_update_help_info(ui_study, help_type, zero_point, 0.0);
  return;
}


void ui_study_cb_zoom(GtkSpinButton * spin_button, gpointer data) {

  ui_study_t * ui_study = data;
  amide_real_t zoom;

  if (AMITK_OBJECT_CHILDREN(ui_study->study)==NULL) return;

  zoom = gtk_spin_button_get_value(spin_button);
  amitk_study_set_zoom(ui_study->study, zoom);

  return;
}

void ui_study_cb_fov(GtkSpinButton * spin_button, gpointer data) {

  ui_study_t * ui_study = data;
  amide_real_t fov;

  if (AMITK_OBJECT_CHILDREN(ui_study->study)==NULL) return;

  fov = gtk_spin_button_get_value(spin_button);
  amitk_study_set_fov(ui_study->study, fov);
    
  return;
}


void ui_study_cb_thickness(GtkSpinButton * spin_button, gpointer data) {

  ui_study_t * ui_study = data;
  amide_real_t thickness;

  if (AMITK_OBJECT_CHILDREN(ui_study->study)==NULL) return;

  /* this does the equivalent of hitting "enter", makes sure any rounding error
     that is done on the entry is performed before we get the value */
  g_signal_emit_by_name(G_OBJECT(spin_button), "activate", NULL, NULL);


  thickness = gtk_spin_button_get_value(spin_button);
  amitk_study_set_view_thickness(ui_study->study, thickness);
  
  return;
}



static gboolean gate_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data) {
  ui_study_t * ui_study = data;

  /* just keeping track on whether or not the gate widget is up */
  ui_study->gate_dialog = NULL;

  return FALSE;
}

void ui_study_cb_gate(GtkWidget * button, gpointer data) {
  ui_study_t * ui_study = data;

  if (ui_study->gate_dialog == NULL) {
    if (AMITK_IS_DATA_SET(ui_study->active_object)) {
      ui_study->gate_dialog = ui_gate_dialog_create(AMITK_DATA_SET(ui_study->active_object), 
						    ui_study->window);
      g_signal_connect(G_OBJECT(ui_study->gate_dialog), "delete_event",
		       G_CALLBACK(gate_delete_event), ui_study);
      gtk_widget_show(ui_study->gate_dialog);
    }
  } else /* pop the window to the top */
    gtk_window_present(GTK_WINDOW(ui_study->gate_dialog));

  return;
}


static gboolean time_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data) {
  ui_study_t * ui_study = data;

  /* just keeping track on whether or not the time widget is up */
  ui_study->time_dialog = NULL;

  return FALSE;
}

void ui_study_cb_time(GtkWidget * button, gpointer data) {
  ui_study_t * ui_study = data;

  if (ui_study->time_dialog == NULL) {
    ui_study->time_dialog = ui_time_dialog_create(ui_study->study, ui_study->window);
    g_signal_connect(G_OBJECT(ui_study->time_dialog), "delete_event",
		     G_CALLBACK(time_delete_event), ui_study);
    gtk_widget_show(ui_study->time_dialog);

  } else /* pop the window to the top */
    gtk_window_present(GTK_WINDOW(ui_study->time_dialog));

  return;
}




/* callbacks for setting up a set of slices in a new window */
void ui_study_cb_series(GtkAction * action, gpointer data) {

  GtkWidget * dialog;
  ui_study_t * ui_study = data;
  gint return_val;
  GList * selected_objects;

  /* let the user input rendering options */
  dialog = ui_series_init_dialog_create(ui_study->study, ui_study->window);

  /* and wait for the question to return */
  return_val = gtk_dialog_run(GTK_DIALOG(dialog));
    
  selected_objects = ui_common_init_dialog_selected_objects(dialog);
  gtk_widget_destroy(dialog);

  /* we want to create the series */
  if (return_val == AMITK_RESPONSE_EXECUTE) {
    ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);
    ui_series_create(ui_study->study, 
		     ui_study->active_object,
		     selected_objects,
		     ui_study->preferences);
    ui_common_remove_wait_cursor(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);
  }

  selected_objects = amitk_objects_unref(selected_objects);
  return;
}

#if (AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)
void ui_study_cb_fly_through(GtkAction * action, gpointer data) {

  ui_study_t * ui_study = data;
  AmitkView view;
  
  view = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(action), "view"));

  tb_fly_through(ui_study->study, view, ui_study->preferences, ui_study->window);

  return;
}
#endif

#ifdef AMIDE_LIBVOLPACK_SUPPORT
/* callback for starting up volume rendering */
void ui_study_cb_render(GtkAction * action, gpointer data) {

  GtkWidget * dialog;
  ui_study_t * ui_study = data;
  gint return_val;
  GList * selected_objects;

  /* let the user input rendering options */
  dialog = ui_render_init_dialog_create(ui_study->study, ui_study->window);

  /* and wait for the question to return */
  return_val = gtk_dialog_run(GTK_DIALOG(dialog));

  selected_objects = ui_common_init_dialog_selected_objects(dialog);
  gtk_widget_destroy(dialog);

  /* we want to render */
  if (return_val == AMITK_RESPONSE_EXECUTE) {
    ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);
    ui_render_create(ui_study->study, selected_objects, ui_study->preferences);
    ui_common_remove_wait_cursor(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);
  }

  selected_objects = amitk_objects_unref(selected_objects);
  return;
}
#endif


/* do roi calculations */
void ui_study_cb_roi_statistics(GtkAction * action, gpointer data) {
  ui_study_t * ui_study = data;
  GtkWidget * dialog;
  gint return_val;

  /* let the user input roi analysis options */
  dialog = tb_roi_analysis_init_dialog(ui_study->window);

  /* and wait for the question to return */
  return_val = gtk_dialog_run(GTK_DIALOG(dialog));

  gtk_widget_destroy(dialog);
  if (return_val != AMITK_RESPONSE_EXECUTE)
    return; /* we hit cancel */

  ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);
  tb_roi_analysis(ui_study->study, ui_study->preferences, ui_study->window);
  ui_common_remove_wait_cursor(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);

  return;
}

/* user wants to run the alignment wizard */
void ui_study_cb_alignment_selected(GtkAction * action, gpointer data) {
  ui_study_t * ui_study = data;
  tb_alignment(ui_study->study, ui_study->window);
  return;
}



/* user wants to run the crop wizard */
void ui_study_cb_crop_selected(GtkAction * action, gpointer data) {
  ui_study_t * ui_study = data;

  if (!AMITK_IS_DATA_SET(ui_study->active_object)) 
    g_warning("%s",no_active_ds);
  else
    tb_crop(ui_study->study, AMITK_DATA_SET(ui_study->active_object), ui_study->window);
  return;
}

/* user wants to run the distance wizard */
void ui_study_cb_distance_selected(GtkAction * action, gpointer data) {
  ui_study_t * ui_study = data;

  tb_distance(ui_study->study, ui_study->window);

  return;
}

/* user wants to run the fads wizard */
void ui_study_cb_fads_selected(GtkAction * action, gpointer data) {
  ui_study_t * ui_study = data;

  if (!AMITK_IS_DATA_SET(ui_study->active_object)) 
    g_warning("%s",no_active_ds);
  else {
#ifdef AMIDE_LIBGSL_SUPPORT
    tb_fads(AMITK_DATA_SET(ui_study->active_object), ui_study->preferences, ui_study->window);
#else
    g_warning("%s",no_gsl);
#endif
  }
  return;
}

/* user wants to run the filter wizard */
void ui_study_cb_filter_selected(GtkAction * action, gpointer data) {
  ui_study_t * ui_study = data;

  if (!AMITK_IS_DATA_SET(ui_study->active_object)) 
    g_warning("%s",no_active_ds);
  else 
    tb_filter(ui_study->study, AMITK_DATA_SET(ui_study->active_object), ui_study->window);

  return;
}

/* user wants to run the profile wizard */
void ui_study_cb_profile_selected(GtkAction * action, gpointer data) {
  ui_study_t * ui_study = data;

  tb_profile(ui_study->study, ui_study->preferences, ui_study->window);

  return;
}


/* user wants to run the image math wizard */
void ui_study_cb_data_set_math_selected(GtkAction * action, gpointer data) {
  ui_study_t * ui_study = data;

  tb_math(ui_study->study, ui_study->window);

  return;
}


static gboolean threshold_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data) {
  ui_study_t * ui_study = data;

  /* just keeping track on whether or not the threshold widget is up */
  ui_study->threshold_dialog = NULL;

  return FALSE;
}


/* function called when target button toggled on toolbar */
void ui_study_cb_canvas_target(GtkToggleAction * action, gpointer data) {

  ui_study_t * ui_study = data;
  amitk_study_set_canvas_target(ui_study->study,
				gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)));  
}

/* function called when hitting the threshold button, pops up a dialog */
void ui_study_cb_thresholding(GtkAction * action, gpointer data) {

  ui_study_t * ui_study = data;

  if (!AMITK_IS_DATA_SET(ui_study->active_object)) return;

  /* make sure we don't already have a data set edit dialog up */
  if ((ui_study->active_object->dialog == NULL) &&
      (ui_study->threshold_dialog == NULL)) {
    ui_common_place_cursor(UI_CURSOR_WAIT, ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);
    ui_study->threshold_dialog = amitk_threshold_dialog_new(AMITK_DATA_SET(ui_study->active_object),
							    ui_study->window);
    ui_common_remove_wait_cursor(ui_study->canvas[AMITK_VIEW_MODE_SINGLE][AMITK_VIEW_TRANSVERSE]);
    g_signal_connect(G_OBJECT(ui_study->threshold_dialog), "delete_event",
		     G_CALLBACK(threshold_delete_event), ui_study);
    gtk_widget_show(ui_study->threshold_dialog);
  } else {
    /* pop the window to the top */
    if (ui_study->active_object->dialog)
      gtk_window_present(GTK_WINDOW(ui_study->active_object->dialog));
    else
      gtk_window_present(GTK_WINDOW(ui_study->threshold_dialog));
  }

  return;
}

/* callback function for adding an roi */
void ui_study_cb_add_roi(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  AmitkRoiType roi_type;

  /* figure out which menu item called me */
  roi_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"roi_type"));
  ui_study_add_roi(ui_study, AMITK_OBJECT(ui_study->study), roi_type);

  return;
}


/* callback function for adding a fiducial mark */
void ui_study_cb_add_fiducial_mark(GtkAction * action, gpointer data) {

  ui_study_t * ui_study = data;

  if (AMITK_IS_DATA_SET(ui_study->active_object)) {
    ui_study_add_fiducial_mark(ui_study, ui_study->active_object, TRUE,
			       AMITK_STUDY_VIEW_CENTER(ui_study->study));
  }

  return;
}

/* callback function for changing user's preferences */
void ui_study_cb_preferences(GtkAction * action, gpointer data) {

  ui_study_t * ui_study=data;
  ui_preferences_dialog_create(ui_study);
  return;
}


/* function to switch the interpolation method */
void ui_study_cb_interpolation(GtkRadioAction * action, GtkRadioAction * current, gpointer data) {
  ui_study_t * ui_study = data;
  g_return_if_fail(AMITK_IS_DATA_SET(ui_study->active_object));

  amitk_data_set_set_interpolation(AMITK_DATA_SET(ui_study->active_object),
				   gtk_radio_action_get_current_value((GTK_RADIO_ACTION(current))));	
  return;
}

/* function to switch the rendering method */
void ui_study_cb_rendering(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;
  g_return_if_fail(AMITK_IS_DATA_SET(ui_study->active_object));

  amitk_data_set_set_rendering(AMITK_DATA_SET(ui_study->active_object),
			       gtk_combo_box_get_active(GTK_COMBO_BOX(widget)));
  return;
}

void ui_study_cb_study_changed(AmitkStudy * study, gpointer data) {
  ui_study_t * ui_study = data;
  ui_study_update_time_button(ui_study);
  ui_study_update_canvas_target(ui_study);
  return;
}

void ui_study_cb_thickness_changed(AmitkStudy * study, gpointer data) {
  ui_study_t * ui_study = data;
  ui_study_update_thickness(ui_study, AMITK_STUDY_VIEW_THICKNESS(ui_study->study));
  return;
}

void ui_study_cb_canvas_layout_changed(AmitkStudy * study, gpointer data) {
  ui_study_t * ui_study = data;
  ui_study_update_canvas_visible_buttons(ui_study);
  ui_study_update_layout(ui_study);
  return;
}

void ui_study_cb_voxel_dim_or_zoom_changed(AmitkStudy * study, gpointer data) {
  ui_study_t * ui_study = data;
  ui_study_update_zoom(ui_study);
  return;
}

void ui_study_cb_fov_changed(AmitkStudy * study, gpointer data) {
  ui_study_t * ui_study = data;
  ui_study_update_fov(ui_study);
  return;
}

/* function to switch the image fusion type */
void ui_study_cb_fuse_type(GtkRadioAction * action, GtkRadioAction * current, gpointer data) {

  ui_study_t * ui_study = data;

  amitk_study_set_fuse_type(ui_study->study,  
			    gtk_radio_action_get_current_value((GTK_RADIO_ACTION(current))));	
  ui_study_update_fuse_type(ui_study);
				
  return;
}

void ui_study_cb_canvas_visible(GtkToggleAction * action, gpointer data) {

  ui_study_t * ui_study = data;

  AmitkView view;

  view = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(action),"view"));
  amitk_study_set_canvas_visible(ui_study->study, view, 
				 gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action)));
  ui_study_update_canvas_visible_buttons(ui_study);

  return;
}

void ui_study_cb_view_mode(GtkRadioAction * action, GtkRadioAction * current, gpointer data) {

  ui_study_t * ui_study = data;

  amitk_study_set_view_mode(ui_study->study, 
			    gtk_radio_action_get_current_value((GTK_RADIO_ACTION(current))));	
  ui_study_update_view_mode(ui_study);

  return;
}


/* function ran to exit the program */
void ui_study_cb_quit(GtkAction * action, gpointer data) {

  ui_study_t * ui_study = data;
  GtkWindow * window = ui_study->window;
  gboolean return_val;

  /* run the delete event function */
  g_signal_emit_by_name(G_OBJECT(window), "delete_event", NULL, &return_val);

  if (!return_val) { /* if we don't cancel the quit */
    gtk_widget_destroy(GTK_WIDGET(window));
    amide_unregister_all_windows(); /* kill everything */
  }

  return;
}

/* function ran when closing a study window */
void ui_study_cb_close(GtkAction* action, gpointer data) {

  ui_study_t * ui_study = data;
  GtkWindow * window = ui_study->window;
  gboolean return_val;

  /* run the delete event function */
  g_signal_emit_by_name(G_OBJECT(window), "delete_event", NULL, &return_val);
  if (!return_val) gtk_widget_destroy(GTK_WIDGET(window));

  return;
}

/* function to run for a delete_event */
gboolean ui_study_cb_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_study_t * ui_study = data;
  GtkWindow * window = ui_study->window;
  GtkWidget * exit_dialog;
  gint return_val;

  /* check to see if we need saving */
  if ((ui_study->study_altered == TRUE) && 
      (AMITK_PREFERENCES_PROMPT_FOR_SAVE_ON_EXIT(ui_study->preferences))) {

    exit_dialog = gtk_message_dialog_new(ui_study->window,
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_OK_CANCEL,
					 _("There are unsaved changes to the study.\nAre you sure you wish to quit?"));

    /* and wait for the question to return */
    return_val = gtk_dialog_run(GTK_DIALOG(exit_dialog));

    gtk_widget_destroy(exit_dialog);
    if (return_val != GTK_RESPONSE_OK)
      return TRUE; /* cancel */

  }
  
  ui_study = ui_study_free(ui_study); /* free our data structure's */
  amide_unregister_window((gpointer) window); /* tell the rest of the program this window's gone */

  return FALSE;
}

