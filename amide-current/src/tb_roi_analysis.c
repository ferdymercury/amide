/* tb_roi_analysis.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2002 Andy Loening
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
#include <gtk/gtk.h>
#include <string.h>
#include "ui_common.h"
#include "analysis.h"
#include "tb_roi_analysis.h"



#define AMITK_RESPONSE_SAVE_AS 3
#define ROI_STATISTICS_WIDTH 1024

typedef enum {
  COLUMN_NAME,
  COLUMN_FRAME,
  COLUMN_DURATION,
  COLUMN_TIME_MIDPT,
  COLUMN_TOTAL,
  COLUMN_MEAN,
  COLUMN_VAR,
  COLUMN_STD_DEV,
  COLUMN_STD_ERR,
  COLUMN_MIN,
  COLUMN_MAX,
  COLUMN_SIZE,
  COLUMN_VOXELS,
  NUM_ANALYSIS_COLUMNS,
} column_t;

static gchar * analysis_titles[] = {
  "Data Set",
  "Frame",
  "Duration (s)",
  "Time Midpt (s)",
  "Total",
  "Mean",
  "Var",
  "Std Dev",
  "Std Err",
  "Min",
  "Max",
  "Size (mm^3)",
  "Voxels"
};


static void export_ok_cb(GtkWidget* widget, gpointer data);
static void export_data(analysis_roi_t * roi_analyses);
static void export_analyses(const gchar * save_filename, analysis_roi_t * roi_analyses);
static void response_cb (GtkDialog * dialog, gint response_id, gpointer data);
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * delete_event, gpointer data);
static void add_page(GtkWidget * notebook, AmitkStudy * study,
		     AmitkRoi * roi, analysis_volume_t * volume_analyses);


/* function to handle exporting the roi analyses */
static void export_ok_cb(GtkWidget* widget, gpointer data) {

  GtkWidget * file_selection = data;
  analysis_roi_t * roi_analyses;
  const gchar * save_filename;

  /* get a pointer to the analysis */
  roi_analyses = g_object_get_data(G_OBJECT(file_selection), "roi_analyses");

  if ((save_filename = ui_common_file_selection_get_name(file_selection)) == NULL)
    return; /* inappropriate name or don't want to overwrite */

  /* allright, save the data */
  export_analyses(save_filename, roi_analyses);

  /* close the file selection box */
  ui_common_file_selection_cancel_cb(widget, file_selection);

  return;
}

/* function to save the generated roi statistics */
static void export_data(analysis_roi_t * roi_analyses) {
  
  analysis_roi_t * temp_analyses = roi_analyses;
  GtkFileSelection * file_selection;
  gchar * temp_string;
  gchar * analysis_name = NULL;

  /* sanity checks */
  g_return_if_fail(roi_analyses != NULL);

  file_selection = GTK_FILE_SELECTION(gtk_file_selection_new(_("Export Statistics")));

  /* take a guess at the filename */
  analysis_name = g_strdup_printf("%s_analysis_{%s",
				  AMITK_OBJECT_NAME(roi_analyses->study), 
				  AMITK_OBJECT_NAME(roi_analyses->roi));
  temp_analyses= roi_analyses->next_roi_analysis;
  while (temp_analyses != NULL) {
    temp_string = g_strdup_printf("%s+%s",analysis_name,AMITK_OBJECT_NAME(temp_analyses->roi));
    g_free(analysis_name);
    analysis_name = temp_string;
    temp_analyses= temp_analyses->next_roi_analysis;
  }
  temp_string = g_strdup_printf("%s}.csv",analysis_name);
  g_free(analysis_name);
  analysis_name = temp_string;
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selection), analysis_name);
  g_free(analysis_name);

  /* save a pointer to the analyses */
  g_object_set_data(G_OBJECT(file_selection), "roi_analyses", roi_analyses);

  /* don't want anything else going on till this window is gone */
  gtk_window_set_modal(GTK_WINDOW(file_selection), TRUE);

  /* connect the signals */
  g_signal_connect(G_OBJECT(file_selection->ok_button), "clicked",
		   G_CALLBACK(export_ok_cb), file_selection);
  g_signal_connect(G_OBJECT(file_selection->cancel_button), "clicked",
		   G_CALLBACK(ui_common_file_selection_cancel_cb), file_selection);
  g_signal_connect(G_OBJECT(file_selection->cancel_button), "delete_event",
		   G_CALLBACK(ui_common_file_selection_cancel_cb), file_selection);

  /* set the position of the dialog */
  gtk_window_set_position(GTK_WINDOW(file_selection), GTK_WIN_POS_MOUSE);

  /* run the dialog */
  gtk_widget_show(GTK_WIDGET(file_selection));

  return;
}


static void export_analyses(const gchar * save_filename, analysis_roi_t * roi_analyses) {

  FILE * file_pointer;
  time_t current_time;
  analysis_volume_t * volume_analyses;
  analysis_frame_t * frame_analyses;
  guint frame;
  guint i;
  amide_real_t voxel_volume;
  gboolean title_printed;
  AmitkPoint voxel_size;

  if ((file_pointer = fopen(save_filename, "w")) == NULL) {
    g_warning("couldn't open: %s for writing roi analyses", save_filename);
    return;
  }

  /* intro information */
  time(&current_time);
  fprintf(file_pointer, "# %s: ROI Analysis File - generated on %s", PACKAGE, ctime(&current_time));
  fprintf(file_pointer, "#\n");
  fprintf(file_pointer, "# Study:\t%s\n", AMITK_OBJECT_NAME(roi_analyses->study));
  fprintf(file_pointer, "#\n");
  
  while (roi_analyses != NULL) {
    fprintf(file_pointer, "# ROI:\t%s\tType:\t%s\n",
	    AMITK_OBJECT_NAME(roi_analyses->roi),
	    amitk_roi_type_get_name(AMITK_ROI_TYPE(roi_analyses->roi)));
    title_printed = FALSE;

    volume_analyses = roi_analyses->volume_analyses;
    while (volume_analyses != NULL) {

      fprintf(file_pointer, "#\tVolume:\t%s\tscaling factor:\t%g\n",
	      AMITK_OBJECT_NAME(volume_analyses->data_set),
	      AMITK_DATA_SET_SCALE_FACTOR(volume_analyses->data_set));

      if (!title_printed) {
	fprintf(file_pointer, "#\t\t%s", analysis_titles[1]);
	for (i=2;i<NUM_ANALYSIS_COLUMNS;i++)
	  fprintf(file_pointer, "\t%12s", analysis_titles[i]);
	fprintf(file_pointer, "\n");
	title_printed = TRUE;
      }

      voxel_size = AMITK_DATA_SET_VOXEL_SIZE(volume_analyses->data_set);
      voxel_volume = voxel_size.x*voxel_size.y*voxel_size.z;

      frame_analyses = volume_analyses->frame_analyses;
      frame = 0;
      while (frame_analyses != NULL) {
	fprintf(file_pointer, "\t\t%d", frame);
	fprintf(file_pointer, "\t% 12.3f", frame_analyses->duration);
	fprintf(file_pointer, "\t% 12.3f", frame_analyses->time_midpoint);
	fprintf(file_pointer, "\t% 12g", frame_analyses->total);
	fprintf(file_pointer, "\t% 12g", frame_analyses->mean);
	fprintf(file_pointer, "\t% 12g", frame_analyses->var);
	fprintf(file_pointer, "\t% 12g", sqrt(frame_analyses->var));
	fprintf(file_pointer, "\t% 12g", sqrt(frame_analyses->var/frame_analyses->voxels));
	fprintf(file_pointer, "\t% 12g", frame_analyses->min);
	fprintf(file_pointer, "\t% 12g", frame_analyses->max);
	fprintf(file_pointer, "\t% 12g", frame_analyses->voxels*voxel_volume);
	fprintf(file_pointer, "\t% 12.2f", frame_analyses->voxels);
	fprintf(file_pointer, "\n");

	frame_analyses = frame_analyses->next_frame_analysis;
	frame++;
      }
      volume_analyses = volume_analyses->next_volume_analysis;
    }
    fprintf(file_pointer, "#\n");
    roi_analyses = roi_analyses->next_roi_analysis;
  }


    

  fclose(file_pointer);

  return;
}

static void response_cb (GtkDialog * dialog, gint response_id, gpointer data) {
  
  analysis_roi_t * roi_analyses = data;
  gint return_val;

  switch(response_id) {
  case AMITK_RESPONSE_SAVE_AS:
    export_data(roi_analyses);
    break;

  case GTK_RESPONSE_CLOSE:
    g_signal_emit_by_name(G_OBJECT(dialog), "delete_event", NULL, &return_val);
    if (!return_val) gtk_widget_destroy(GTK_WIDGET(dialog));
    break;

  default:
    break;
  }

  return;
}


/* function called to destroy the roi analysis dialog */
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {

  analysis_roi_t * roi_analyses = data;

  /* free the associated data structure */
  roi_analyses = analysis_roi_unref(roi_analyses);

  return FALSE;
}




/* create one page of our notebook */
static void add_page(GtkWidget * notebook, AmitkStudy * study,
		     AmitkRoi * roi, analysis_volume_t * volume_analyses) {

  GtkWidget * table;
  GtkWidget * label;
  GtkWidget * entry;
  GtkWidget * list;
  GtkWidget * scrolled;
  GtkWidget * hbox;
  analysis_frame_t * frame_analyses;
  guint frame;
  guint table_row=0;
  amide_real_t voxel_volume;
  AmitkPoint voxel_size;
  GtkListStore * store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  column_t i_column;
  gint width;
  
  


  label = gtk_label_new(AMITK_OBJECT_NAME(roi));
  table = gtk_table_new(5,3,FALSE);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, label);

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(hbox), 0,5,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  table_row++;
  gtk_widget_show(hbox);

  /* tell use the type */
  label = gtk_label_new("type:");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

  entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(entry), amitk_roi_type_get_name(AMITK_ROI_TYPE(roi)));
  gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 5);

  /* try to get a reasonable estimate for how wide the statistics box should be */
  width = 0.9*gdk_screen_width();
  if (width > ROI_STATISTICS_WIDTH)
    width = ROI_STATISTICS_WIDTH;

  /* the scroll widget which the list will go into */
  scrolled = gtk_scrolled_window_new(NULL,NULL);
  gtk_widget_set_size_request(scrolled,width,250);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				 GTK_POLICY_AUTOMATIC,
				 GTK_POLICY_AUTOMATIC);

  /* and throw the scrolled widget into the packing table */
  gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(scrolled), 0,5,table_row, table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL,
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);
  table_row++;

  /* and the list itself */
  store = gtk_list_store_new(NUM_ANALYSIS_COLUMNS, 
			     G_TYPE_STRING,
			     G_TYPE_INT,
			     AMITK_TYPE_TIME,
			     AMITK_TYPE_TIME,
			     AMITK_TYPE_DATA,
			     AMITK_TYPE_DATA,
			     AMITK_TYPE_DATA,
			     AMITK_TYPE_DATA,
			     AMITK_TYPE_DATA,
			     AMITK_TYPE_DATA,
			     AMITK_TYPE_DATA,
			     AMITK_TYPE_REAL,
			     AMITK_TYPE_REAL);
  list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref(store);

  for (i_column=0; i_column<NUM_ANALYSIS_COLUMNS; i_column++) {
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes(analysis_titles[i_column], renderer,
						      "text", i_column, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
  }

  /* iterate over the analysis of each volume */
  while (volume_analyses != NULL) {
    frame_analyses = volume_analyses->frame_analyses;

    voxel_size = AMITK_DATA_SET_VOXEL_SIZE(volume_analyses->data_set);
    voxel_volume = voxel_size.x*voxel_size.y*voxel_size.z;

    /* iterate over the frames */
    frame = 0;
    while (frame_analyses != NULL) {
      gtk_list_store_append (store, &iter);  /* Acquire an iterator */
      gtk_list_store_set (store, &iter,
			  COLUMN_NAME,AMITK_OBJECT_NAME(volume_analyses->data_set),
			  COLUMN_FRAME, frame,
			  COLUMN_DURATION, frame_analyses->duration,
			  COLUMN_TIME_MIDPT, frame_analyses->time_midpoint,
			  COLUMN_TOTAL, frame_analyses->total,
			  COLUMN_MEAN, frame_analyses->mean,
			  COLUMN_VAR, frame_analyses->var,
			  COLUMN_STD_DEV, sqrt(frame_analyses->var),
			  COLUMN_STD_ERR, sqrt(frame_analyses->var/frame_analyses->voxels),
			  COLUMN_MIN,frame_analyses->min,
			  COLUMN_MAX,frame_analyses->max,
			  COLUMN_SIZE,frame_analyses->voxels*voxel_volume,
			  COLUMN_VOXELS,frame_analyses->voxels,
			  -1);
      frame++;
      frame_analyses = frame_analyses->next_frame_analysis;
    }
    volume_analyses = volume_analyses->next_volume_analysis;
  }

  /* just using the list for display */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);

  gtk_container_add(GTK_CONTAINER(scrolled),list); /* and put it in the scrolled widget */
  table_row++;

  return;
}


void tb_roi_analysis(AmitkStudy * study,
		     gboolean all_data_sets,
		     gboolean all_rois,
		     GtkWindow * parent) {

  GtkWidget * dialog;
  GtkWidget * notebook;
  gchar * title;
  GList * rois;
  GList * data_sets;
  analysis_roi_t * roi_analyses;
  analysis_roi_t * temp_analyses;

  /* figure out which data sets we're dealing with */
  if (all_data_sets)
    data_sets = amitk_object_get_children_of_type(AMITK_OBJECT(study), 
						  AMITK_OBJECT_TYPE_DATA_SET, TRUE);
  else
    data_sets = amitk_object_get_selected_children_of_type(AMITK_OBJECT(study), 
							   AMITK_OBJECT_TYPE_DATA_SET, AMITK_SELECTION_ALL, TRUE);

  if (data_sets == NULL) {
    g_warning("No Data Sets selected for calculating analyses");
    return;
  }

  /* get the list of roi's we're going to be calculating over */
  if (all_rois)
    rois = amitk_object_get_children_of_type(AMITK_OBJECT(study), AMITK_OBJECT_TYPE_ROI, TRUE);
  else 
    rois = amitk_object_get_selected_children_of_type(AMITK_OBJECT(study), 
						      AMITK_OBJECT_TYPE_ROI, AMITK_SELECTION_ALL, TRUE);

  if (rois == NULL) {
    g_warning("No ROI's selected for calculating analyses");
    amitk_objects_unref(data_sets);
    return;
  }

  /* calculate all our data */
  roi_analyses = analysis_roi_init(study, rois, data_sets);

  rois = amitk_objects_unref(rois);
  data_sets = amitk_objects_unref(data_sets);
  g_return_if_fail(roi_analyses != NULL);
  
  /* start setting up the widget we'll display the info from */
  title = g_strdup_printf("%s Roi Analysis: Study %s", PACKAGE, 
			  AMITK_OBJECT_NAME(study));
  dialog = gtk_dialog_new_with_buttons(title, GTK_WINDOW(parent),
				       GTK_DIALOG_DESTROY_WITH_PARENT,
				       GTK_STOCK_SAVE_AS, AMITK_RESPONSE_SAVE_AS,
				       GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
				       NULL);
  g_free(title);

  /* setup the callbacks for app */
  g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(response_cb), roi_analyses);
  g_signal_connect(G_OBJECT(dialog), "delete_event", G_CALLBACK(delete_event_cb), roi_analyses);

  gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);


 /* make the widgets for this dialog box */
  notebook = gtk_notebook_new();
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), notebook);

  /* add the data pages */
  temp_analyses = roi_analyses;
  while (temp_analyses != NULL) {
    add_page(notebook, study, temp_analyses->roi, temp_analyses->volume_analyses);
    temp_analyses=temp_analyses->next_roi_analysis;
  }

  /* and show all our widgets */
  gtk_widget_show_all(dialog);

  return;
}




static void radio_buttons_cb(GtkWidget * widget, gpointer data);
static void init_response_cb (GtkDialog * dialog, gint response_id, gpointer data);



/* function called to change the layout */
static void radio_buttons_cb(GtkWidget * widget, gpointer data) {

  GtkWidget * dialog=data;
  gboolean all_data_sets;
  gboolean all_rois;

  all_data_sets = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "all_data_sets"));
  all_rois = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "all_rois"));

  g_object_set_data(G_OBJECT(dialog), "all_data_sets", GINT_TO_POINTER(all_data_sets));
  g_object_set_data(G_OBJECT(dialog), "all_rois", GINT_TO_POINTER(all_rois));

  gnome_config_push_prefix("/"PACKAGE"/");
  gnome_config_set_int("ANALYSIS/CalculateAllDataSets",all_data_sets);
  gnome_config_set_int("ANALYSIS/CalculateAllRois",all_rois);
  gnome_config_pop_prefix();
  gnome_config_sync();

  return;
}

static void init_response_cb (GtkDialog * dialog, gint response_id, gpointer data) {
  
  gint return_val;

  switch(response_id) {
  case AMITK_RESPONSE_EXECUTE:
  case GTK_RESPONSE_CLOSE:
    g_signal_emit_by_name(G_OBJECT(dialog), "delete_event", NULL, &return_val);
    if (!return_val) gtk_widget_destroy(GTK_WIDGET(dialog));
    break;

  default:
    break;
  }

  return;
}


/* function to setup a dialog to allow us to choice options for rendering */
GtkWidget * tb_roi_analysis_init_dialog(GtkWindow * parent) {
  
  GtkWidget * dialog;
  gchar * temp_string;
  GtkWidget * table;
  GtkWidget * label;
  guint table_row;
  gboolean all_data_sets;
  gboolean all_rois;
  GtkWidget * radio_button[4];

  gnome_config_push_prefix("/"PACKAGE"/");
  all_data_sets = gnome_config_get_int("ANALYSIS/CalculateAllDataSets");
  all_rois = gnome_config_get_int("ANALYSIS/CalculateAllRois");
  gnome_config_pop_prefix();

  temp_string = g_strdup_printf("%s: ROI Analysis Initialization Dialog", PACKAGE);
  dialog = gtk_dialog_new_with_buttons (temp_string,  parent,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_EXECUTE, AMITK_RESPONSE_EXECUTE,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CLOSE, NULL);
  gtk_window_set_title(GTK_WINDOW(dialog), temp_string);
  g_free(temp_string);
  g_object_set_data(G_OBJECT(dialog), "all_data_sets", GINT_TO_POINTER(all_data_sets));
  g_object_set_data(G_OBJECT(dialog), "all_rois", GINT_TO_POINTER(all_rois));


  /* setup the callbacks for the dialog */
  g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(init_response_cb), NULL);

  gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);

  /* start making the widgets for this dialog box */
  table = gtk_table_new(3,3,FALSE);
  table_row=0;
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);

  label = gtk_label_new("Calculate:");
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 
		   table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);
  label = gtk_label_new("All ROIS:");
  gtk_table_attach(GTK_TABLE(table), label, 1,2, 
		   table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);
  label = gtk_label_new("Selected ROIS:");
  gtk_table_attach(GTK_TABLE(table), label, 2,3, 
		   table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);
  table_row++;

  label = gtk_label_new("On All Data Sets:");
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 
		   table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);

  radio_button[0] = gtk_radio_button_new(NULL);
  gtk_table_attach(GTK_TABLE(table), radio_button[0], 1,2, 
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  g_object_set_data(G_OBJECT(radio_button[0]), "all_data_sets", GINT_TO_POINTER(TRUE));
  g_object_set_data(G_OBJECT(radio_button[0]), "all_rois", GINT_TO_POINTER(TRUE));

  radio_button[1] = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(radio_button[0]));
  gtk_table_attach(GTK_TABLE(table), radio_button[1], 2,3, 
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  g_object_set_data(G_OBJECT(radio_button[1]), "all_data_sets", GINT_TO_POINTER(TRUE));
  g_object_set_data(G_OBJECT(radio_button[1]), "all_rois", GINT_TO_POINTER(FALSE));
  table_row++;


  label = gtk_label_new("On Selected Data Sets:");
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 
		   table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);

  radio_button[2] = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(radio_button[0]));
  gtk_table_attach(GTK_TABLE(table), radio_button[2], 1,2, 
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  g_object_set_data(G_OBJECT(radio_button[2]), "all_data_sets", GINT_TO_POINTER(FALSE));
  g_object_set_data(G_OBJECT(radio_button[2]), "all_rois", GINT_TO_POINTER(TRUE));

  radio_button[3] = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(radio_button[0]));
  gtk_table_attach(GTK_TABLE(table), radio_button[3], 2,3, 
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  g_object_set_data(G_OBJECT(radio_button[3]), "all_data_sets", GINT_TO_POINTER(FALSE));
  g_object_set_data(G_OBJECT(radio_button[3]), "all_rois", GINT_TO_POINTER(FALSE));
  table_row++;

  if (all_data_sets && all_rois)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button[0]), TRUE);
  else if (all_data_sets && !all_rois)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button[1]), TRUE);
  else if (!all_data_sets && all_rois)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button[2]), TRUE);
  else /* !all_data_sets && !all_rois */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button[3]), TRUE);

  g_signal_connect(G_OBJECT(radio_button[0]), "clicked", G_CALLBACK(radio_buttons_cb), dialog);
  g_signal_connect(G_OBJECT(radio_button[1]), "clicked", G_CALLBACK(radio_buttons_cb), dialog);
  g_signal_connect(G_OBJECT(radio_button[2]), "clicked", G_CALLBACK(radio_buttons_cb), dialog);
  g_signal_connect(G_OBJECT(radio_button[3]), "clicked", G_CALLBACK(radio_buttons_cb), dialog);


  /* and show all our widgets */
  gtk_widget_show_all(dialog);

  return dialog;
}












