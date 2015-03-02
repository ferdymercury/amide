/* tb_roi_analysis.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2014 Andy Loening
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
#include <string.h>

#include "amide.h"
#include "amide_gconf.h"
#include "amitk_common.h"
#include "analysis.h"
#include "tb_roi_analysis.h"
#include "ui_common.h"


#define GCONF_AMIDE_ANALYSIS "ANALYSIS"

#define ROI_STATISTICS_WIDTH 950

/* keep in sync with array below */
typedef enum {
  COLUMN_ROI_NAME,
  COLUMN_DATA_SET_NAME,
  COLUMN_FRAME,
  COLUMN_DURATION,
  COLUMN_TIME_MIDPT,
  COLUMN_GATE,
  COLUMN_GATE_TIME,
  /*  COLUMN_TOTAL, */
  COLUMN_MEDIAN,
  COLUMN_MEAN,
  COLUMN_VAR,
  COLUMN_STD_DEV,
  COLUMN_MIN,
  COLUMN_MAX,
  COLUMN_SIZE,
  COLUMN_FRAC_VOXELS,
  COLUMN_VOXELS,
  NUM_ANALYSIS_COLUMNS,
} column_t;

static gboolean column_use_my_renderer[NUM_ANALYSIS_COLUMNS] = {
  FALSE,
  FALSE,
  FALSE,
  TRUE,
  TRUE,
  FALSE,
  TRUE,
  /*  TRUE, */
  TRUE,
  TRUE,
  TRUE,
  TRUE,
  TRUE,
  TRUE,
  TRUE,
  TRUE,
  FALSE,
};

static gchar * analysis_titles[] = {
  N_("ROI"),
  N_("Data Set"),
  N_("Frame"),
  N_("Duration (s)"),
  N_("Midpt (s)"),
  N_("Gate"),
  N_("Gate Time (s)"),
  /*  N_("Total"), */
  N_("Median"),
  N_("Mean"),
  N_("Var"),
  N_("Std Dev"),
  N_("Min"),
  N_("Max"),
  N_("Size (mm^3)"),
  N_("Frac. Voxels"),
  N_("Voxels")
};

typedef struct tb_roi_analysis_t {
  GtkWidget * dialog;
  AmitkPreferences * preferences;
  analysis_roi_t * roi_analyses;
  guint reference_count;
} tb_roi_analysis_t;
  

static void export_data(tb_roi_analysis_t * tb_roi_analysis, gboolean raw_values);
static void export_analyses(const gchar * save_filename, analysis_roi_t * roi_analyses,
			    gboolean raw_data);
static gchar * analyses_as_string(analysis_roi_t * roi_analyses);
static void response_cb (GtkDialog * dialog, gint response_id, gpointer data);
static void destroy_cb(GtkObject * object, gpointer data);
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * delete_event, gpointer data);
static void add_pages(GtkWidget * notebook, AmitkStudy * study, analysis_roi_t * roi_analyses);
static void read_preferences(gboolean * all_data_sets, 
			     gboolean * all_rois, 
			     analysis_calculation_t * calculation_type,
			     gboolean * accurate,
			     gdouble * subfraction,
			     gdouble * threshold_percentage,
			     gdouble * threshold_value);
static tb_roi_analysis_t * tb_roi_analysis_free(tb_roi_analysis_t * tb_roi_analysis);
static tb_roi_analysis_t * tb_roi_analysis_init(void);



/* function to save the generated roi statistics */
static void export_data(tb_roi_analysis_t * tb_roi_analysis, gboolean raw_data) {  
  analysis_roi_t * temp_analyses = tb_roi_analysis->roi_analyses;
  GtkWidget * file_chooser;
  gchar * temp_string;
  gchar * filename = NULL;

  /* sanity checks */
  g_return_if_fail(tb_roi_analysis->roi_analyses != NULL);

  file_chooser = gtk_file_chooser_dialog_new ((!raw_data) ? _("Export Statistics") : _("Export ROI Raw Data Values"),
					      GTK_WINDOW(tb_roi_analysis->dialog), /* parent window */
					      GTK_FILE_CHOOSER_ACTION_SAVE,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					      NULL);
  gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(file_chooser), TRUE);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (file_chooser), TRUE);
  amitk_preferences_set_file_chooser_directory(tb_roi_analysis->preferences, file_chooser); /* set the default directory if applicable */

  /* take a guess at the filename */
  filename = g_strdup_printf("%s_%s_{%s",
			     AMITK_OBJECT_NAME(tb_roi_analysis->roi_analyses->study), 
			     raw_data ? _("roi_raw_data"): _("analysis"),
			     AMITK_OBJECT_NAME(tb_roi_analysis->roi_analyses->roi));
  
  temp_analyses= tb_roi_analysis->roi_analyses->next_roi_analysis;
  while (temp_analyses != NULL) {
    temp_string = g_strdup_printf("%s+%s",filename,AMITK_OBJECT_NAME(temp_analyses->roi));
    g_free(filename);
    filename = temp_string;
    temp_analyses= temp_analyses->next_roi_analysis;
  }
  temp_string = g_strdup_printf("%s}.tsv",filename);
  g_free(filename);
  filename = temp_string;
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (file_chooser), filename);
  g_free(filename);

  if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT)  {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));
    export_analyses(filename, tb_roi_analysis->roi_analyses, raw_data); /* allright, save the data */
    g_free (filename);
  }
  gtk_widget_destroy (file_chooser);

  return;
}

static void export_analyses(const gchar * save_filename, analysis_roi_t * roi_analyses, gboolean raw_data) {

  FILE * file_pointer;
  time_t current_time;
  analysis_volume_t * volume_analyses;
  analysis_frame_t * frame_analyses;
  analysis_gate_t * gate_analyses;
  guint frame;
  guint gate;
  guint i;
  amide_real_t voxel_volume;
  gboolean title_printed;
  AmitkPoint location;
  analysis_element_t * element;

  /* sanity checks */
  g_return_if_fail(save_filename != NULL);

  if ((file_pointer = fopen(save_filename, "w")) == NULL) {
    g_warning(_("couldn't open: %s for writing roi data"), save_filename);
    return;
  }

  /* intro information */
  time(&current_time);
  fprintf(file_pointer, _("# %s: ROI Analysis File - generated on %s"), PACKAGE, ctime(&current_time));
  fprintf(file_pointer, "#\n");
  fprintf(file_pointer, _("# Study:\t%s\n"), AMITK_OBJECT_NAME(roi_analyses->study));
  fprintf(file_pointer, "#\n");
  
  while (roi_analyses != NULL) {
    fprintf(file_pointer, _("# ROI:\t%s\tType:\t%s"),
	    AMITK_OBJECT_NAME(roi_analyses->roi),
	    amitk_roi_type_get_name(AMITK_ROI_TYPE(roi_analyses->roi)));
    if (AMITK_ROI_TYPE_ISOCONTOUR(roi_analyses->roi)) {
      if (AMITK_ROI_ISOCONTOUR_RANGE(roi_analyses->roi) == AMITK_ROI_ISOCONTOUR_RANGE_ABOVE_MIN) 
	fprintf(file_pointer, _("\tIsocontour Above Value:\t%g"), AMITK_ROI_ISOCONTOUR_MIN_VALUE(roi_analyses->roi));
      else if (AMITK_ROI_ISOCONTOUR_RANGE(roi_analyses->roi) == AMITK_ROI_ISOCONTOUR_RANGE_BELOW_MAX) 
	fprintf(file_pointer, _("\tIsocontour Below Value:\t%g"), AMITK_ROI_ISOCONTOUR_MAX_VALUE(roi_analyses->roi));
      else  /* AMITK_ROI_ISOCONTOUR_RANGE_BETWEEN_MIN_MAX */
	fprintf(file_pointer, _("\tIsocontour Between Values:\t%g %g"), 
		AMITK_ROI_ISOCONTOUR_MIN_VALUE(roi_analyses->roi),
		AMITK_ROI_ISOCONTOUR_MAX_VALUE(roi_analyses->roi));
    }
    fprintf(file_pointer,"\n");

    if (!raw_data) {
      switch(roi_analyses->calculation_type) {
      case ALL_VOXELS:
	fprintf(file_pointer, _("#   Calculation done with all voxels in ROI\n"));
	break;
      case HIGHEST_FRACTION_VOXELS:
	fprintf(file_pointer, _("#   Calculation done on %5.3f percentile of voxels in ROI\n"), roi_analyses->subfraction*100);
	break;
      case VOXELS_NEAR_MAX:
	fprintf(file_pointer, _("#   Calculation done on voxels >= %5.3f percent of maximum value in ROI\n"), roi_analyses->threshold_percentage);
	break;
      case VOXELS_GREATER_THAN_VALUE:
	fprintf(file_pointer, _("#   Calculation done on voxels >= %g in ROI\n"), roi_analyses->threshold_value);
	break;
      default:
	g_error("unexpected case in %s at line %d",__FILE__, __LINE__);
      }
    }

    title_printed = FALSE;

    volume_analyses = roi_analyses->volume_analyses;
    while (volume_analyses != NULL) {

      fprintf(file_pointer, _("#   Data Set:\t%s\tScaling Factor:\t%g\n"),
	      AMITK_OBJECT_NAME(volume_analyses->data_set),
	      AMITK_DATA_SET_SCALE_FACTOR(volume_analyses->data_set));

      switch(AMITK_DATA_SET_CONVERSION(volume_analyses->data_set)) {
      case AMITK_CONVERSION_PERCENT_ID_PER_CC:
      case AMITK_CONVERSION_SUV:
	fprintf(file_pointer, _("#      Output Data Units: %s\n"),
		amitk_conversion_names[AMITK_DATA_SET_CONVERSION(volume_analyses->data_set)]);
	fprintf(file_pointer, _("#         Injected Dose: %g [%s]\n"),
		amitk_dose_unit_convert_to(AMITK_DATA_SET_INJECTED_DOSE(volume_analyses->data_set),
					   AMITK_DATA_SET_DISPLAYED_DOSE_UNIT(volume_analyses->data_set)),
		amitk_dose_unit_names[AMITK_DATA_SET_DISPLAYED_DOSE_UNIT(volume_analyses->data_set)]);
	fprintf(file_pointer, _("#         Cylinder Factor: %g [%s]\n"),
 		amitk_cylinder_unit_convert_to(AMITK_DATA_SET_CYLINDER_FACTOR(volume_analyses->data_set),
					       AMITK_DATA_SET_DISPLAYED_CYLINDER_UNIT(volume_analyses->data_set)),
		amitk_cylinder_unit_names[AMITK_DATA_SET_DISPLAYED_CYLINDER_UNIT(volume_analyses->data_set)]);
	break;
      default:
	break;
      }

      switch(AMITK_DATA_SET_CONVERSION(volume_analyses->data_set)) {
      case AMITK_CONVERSION_SUV:
	fprintf(file_pointer, _("#         Subject Weight: %g [%s]\n"),
 		amitk_weight_unit_convert_to(AMITK_DATA_SET_SUBJECT_WEIGHT(volume_analyses->data_set),
					       AMITK_DATA_SET_DISPLAYED_WEIGHT_UNIT(volume_analyses->data_set)),
		amitk_weight_unit_names[AMITK_DATA_SET_DISPLAYED_WEIGHT_UNIT(volume_analyses->data_set)]);
	break;
      default:
	break;
      }

      if ((!raw_data) && (!title_printed)) {
	fprintf(file_pointer, "#   %s", _(analysis_titles[COLUMN_FRAME]));
	for (i=COLUMN_FRAME+1;i<NUM_ANALYSIS_COLUMNS;i++)
	  fprintf(file_pointer, "\t%12s", _(analysis_titles[i]));
	fprintf(file_pointer, "\n");
	title_printed = TRUE;
      }

      voxel_volume = AMITK_DATA_SET_VOXEL_VOLUME(volume_analyses->data_set); 

      frame_analyses = volume_analyses->frame_analyses;
      frame = 0;
      while (frame_analyses != NULL) {

	gate_analyses = frame_analyses->gate_analyses;
	gate = 0;
	while (gate_analyses != NULL) {
	  if (!raw_data) {
	    fprintf(file_pointer, "    %5d", frame);
	    fprintf(file_pointer, "\t% 12.3f", gate_analyses->duration);
	    fprintf(file_pointer, "\t% 12.3f", gate_analyses->time_midpoint);
	    fprintf(file_pointer, "\t% 12d", gate);
	    fprintf(file_pointer, "\t% 12.3f", gate_analyses->gate_time);
	    /*	  fprintf(file_pointer, "\t% 12g", gate_analyses->total); */
	    fprintf(file_pointer, "\t% 12g", gate_analyses->median);
	    fprintf(file_pointer, "\t% 12g", gate_analyses->mean);
	    fprintf(file_pointer, "\t% 12g", gate_analyses->var);
	    fprintf(file_pointer, "\t% 12g", sqrt(gate_analyses->var));
	    fprintf(file_pointer, "\t% 12g", gate_analyses->min);
	    fprintf(file_pointer, "\t% 12g", gate_analyses->max);
	    fprintf(file_pointer, "\t% 12g", gate_analyses->fractional_voxels*voxel_volume);
	    fprintf(file_pointer, "\t% 12.2f", gate_analyses->fractional_voxels);
	    fprintf(file_pointer, "\t% 12d", gate_analyses->voxels);
	    fprintf(file_pointer, "\n");
	  } else { /* raw data */
	    fprintf(file_pointer, "#   Frame %d, Gate %d, Gate Time %5.3f\n", frame, gate,gate_analyses->gate_time);
	    fprintf(file_pointer, "#      Value\t      Weight\t      X (mm)\t      Y (mm)\t      Z (mm)\n");
	    for (i=0; i < gate_analyses->data_array->len; i++) {
	      element = g_ptr_array_index(gate_analyses->data_array, i);
	      VOXEL_TO_POINT(element->ds_voxel, AMITK_DATA_SET_VOXEL_SIZE(volume_analyses->data_set),location);
	      location = amitk_space_s2b(AMITK_SPACE(volume_analyses->data_set), location);
	      fprintf(file_pointer, "%12g\t%12g\t%12g\t%12g\t%12g\n", element->value, element->weight, location.x, location.y, location.z);
	    }
	  }

	  gate_analyses = gate_analyses->next_gate_analysis;
	  gate++;
	}

	frame_analyses = frame_analyses->next_frame_analysis;
	frame++;
      }
      volume_analyses = volume_analyses->next_volume_analysis;
    }
    roi_analyses = roi_analyses->next_roi_analysis;
    if (roi_analyses != NULL)
      fprintf(file_pointer, "#\n");
  }


  fclose(file_pointer);

  return;
}

static gchar * analyses_as_string(analysis_roi_t * roi_analyses) {

  gchar * roi_stats;
  time_t current_time;
  analysis_volume_t * volume_analyses;
  analysis_frame_t * frame_analyses;
  analysis_gate_t * gate_analyses;
  guint frame;
  guint gate;
  guint i;
  amide_real_t voxel_volume;

  /* intro information */
  time(&current_time);
  roi_stats = g_strdup_printf(_("# Stats for Study: %s\tGenerated on: %s"),
			      AMITK_OBJECT_NAME(roi_analyses->study), ctime(&current_time));
  
  /* print the titles */
  amitk_append_str(&roi_stats,"# %-10s", _(analysis_titles[COLUMN_ROI_NAME]));
  amitk_append_str(&roi_stats,"\t%-12s", _(analysis_titles[COLUMN_DATA_SET_NAME]));
  for (i=COLUMN_DATA_SET_NAME+1;i<NUM_ANALYSIS_COLUMNS;i++)
    amitk_append_str(&roi_stats,"\t%12s", _(analysis_titles[i]));
  amitk_append_str(&roi_stats,"\n");

  /* print the stats */
  while (roi_analyses != NULL) {
    volume_analyses = roi_analyses->volume_analyses;

    while (volume_analyses != NULL) {

      voxel_volume = AMITK_DATA_SET_VOXEL_VOLUME(volume_analyses->data_set); 

      frame_analyses = volume_analyses->frame_analyses;
      frame = 0;

      while (frame_analyses != NULL) {

	gate_analyses = frame_analyses->gate_analyses;
	gate = 0;
	while (gate_analyses != NULL) {
	  amitk_append_str(&roi_stats, "%-12s\t%-12s",
			   AMITK_OBJECT_NAME(roi_analyses->roi),
			   AMITK_OBJECT_NAME(volume_analyses->data_set));

	  amitk_append_str(&roi_stats, "\t% 12d", frame);
	  amitk_append_str(&roi_stats, "\t% 12.3f", gate_analyses->duration);
	  amitk_append_str(&roi_stats, "\t% 12.3f", gate_analyses->time_midpoint);
	  amitk_append_str(&roi_stats, "\t% 12d", gate);
	  amitk_append_str(&roi_stats, "\t% 12.3f", gate_analyses->gate_time);
	  /*	  amitk_append_str(&roi_stats, "\t% 12g", gate_analyses->total); */
	  amitk_append_str(&roi_stats, "\t% 12g", gate_analyses->median);
	  amitk_append_str(&roi_stats, "\t% 12g", gate_analyses->mean);
	  amitk_append_str(&roi_stats, "\t% 12g", gate_analyses->var);
	  amitk_append_str(&roi_stats, "\t% 12g", sqrt(gate_analyses->var));
	  amitk_append_str(&roi_stats, "\t% 12g", gate_analyses->min);
	  amitk_append_str(&roi_stats, "\t% 12g", gate_analyses->max);
	  amitk_append_str(&roi_stats, "\t% 12g", gate_analyses->fractional_voxels*voxel_volume);
	  amitk_append_str(&roi_stats, "\t% 12.2f", gate_analyses->fractional_voxels);
	  amitk_append_str(&roi_stats, "\t% 12d\n", gate_analyses->voxels);

	  gate_analyses = gate_analyses->next_gate_analysis;
	  gate++;
	}

	frame_analyses = frame_analyses->next_frame_analysis;
	frame++;
      }
      volume_analyses = volume_analyses->next_volume_analysis;
    }
    roi_analyses = roi_analyses->next_roi_analysis;
  }


  return roi_stats;
}

static void response_cb (GtkDialog * dialog, gint response_id, gpointer data) {
  
  tb_roi_analysis_t * tb_roi_analysis = data;
  gint return_val;
  GtkClipboard * clipboard;
  gchar * roi_stats;

  switch(response_id) {
  case AMITK_RESPONSE_SAVE_AS:
    export_data(tb_roi_analysis, FALSE);
    break;

  case AMITK_RESPONSE_SAVE_RAW_AS:
    export_data(tb_roi_analysis, TRUE);
    break;

  case AMITK_RESPONSE_COPY:
    roi_stats = analyses_as_string(tb_roi_analysis->roi_analyses);

    /* fill in select/button2 clipboard (X11) */
    clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    gtk_clipboard_set_text(clipboard, roi_stats, -1);

   /* fill in copy/paste clipboard (Win32 and Gnome) */
    clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clipboard, roi_stats, -1);

    g_free(roi_stats);
    break;

  case GTK_RESPONSE_HELP:
    amide_call_help("roi-terms");
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
static void destroy_cb(GtkObject * object, gpointer data) {
  tb_roi_analysis_t * tb_roi_analysis = data;
  tb_roi_analysis = tb_roi_analysis_free(tb_roi_analysis);
  return;
}


static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {
  return FALSE;
}




/* create one page of our notebook */
static void add_pages(GtkWidget * notebook, AmitkStudy * study, analysis_roi_t * roi_analyses) {

  GtkWidget * table;
  GtkWidget * label;
  GtkWidget * entry;
  GtkWidget * list=NULL;
  GtkWidget * scrolled=NULL;
  GtkWidget * hbox;
  analysis_frame_t * frame_analyses;
  analysis_gate_t * gate_analyses;
  guint frame;
  guint gate;
  guint table_row=0;
  amide_real_t voxel_volume;
  GtkListStore * store=NULL;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  column_t i_column;
  gint width;
  analysis_volume_t * volume_analyses;
  analysis_roi_t * temp_roi_analyses;
  gboolean dynamic_data;
  gboolean gated_data;
  gboolean static_tree_created=FALSE;
  gboolean display;
  
  
  /* check if we have dynamic/gated data */
  temp_roi_analyses = roi_analyses;
  dynamic_data = FALSE;
  gated_data=FALSE;
  while (temp_roi_analyses != NULL) {
    volume_analyses = temp_roi_analyses->volume_analyses;
    while (volume_analyses != NULL) {
      if (AMITK_DATA_SET_NUM_FRAMES(volume_analyses->data_set) > 1)
	dynamic_data = TRUE;
      if (AMITK_DATA_SET_NUM_GATES(volume_analyses->data_set) > 1)
	gated_data = TRUE;
      volume_analyses = volume_analyses->next_volume_analysis;
    }
    temp_roi_analyses = temp_roi_analyses->next_roi_analysis;
  }

  while (roi_analyses != NULL) {

    if ((dynamic_data) || (gated_data) || (!static_tree_created)) {

      if ((dynamic_data) || (gated_data))
	label = gtk_label_new(AMITK_OBJECT_NAME(roi_analyses->roi));
      else
	label = gtk_label_new(_("ROI Statistics"));
      table = gtk_table_new(5,3,FALSE);
      gtk_notebook_append_page(GTK_NOTEBOOK(notebook), table, label);

      hbox = gtk_hbox_new(FALSE, 0);
      gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(hbox), 0,5,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      table_row++;
      gtk_widget_show(hbox);

      if ((dynamic_data) || (gated_data)){
	/* tell us the type */
	label = gtk_label_new(_("type:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
	
	entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), amitk_roi_type_get_name(AMITK_ROI_TYPE(roi_analyses->roi)));
	gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 5);
      }
    
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
				 G_TYPE_STRING,
				 G_TYPE_INT,
				 AMITK_TYPE_TIME,
				 AMITK_TYPE_TIME,
				 G_TYPE_INT,
				 AMITK_TYPE_TIME,
				 /*				 AMITK_TYPE_DATA, */
				 AMITK_TYPE_DATA,
				 AMITK_TYPE_DATA,
				 AMITK_TYPE_DATA,
				 AMITK_TYPE_DATA,
				 AMITK_TYPE_DATA,
				 AMITK_TYPE_DATA,
				 AMITK_TYPE_REAL,
				 AMITK_TYPE_REAL,
				 G_TYPE_INT);
      list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
      g_object_unref(store);
    
      for (i_column=0; i_column<NUM_ANALYSIS_COLUMNS; i_column++) {
	display=TRUE;

	if ((dynamic_data) || (gated_data)) 
	  if (i_column == COLUMN_ROI_NAME)
	    display = FALSE;

	if (!dynamic_data) 
	  if ((i_column == COLUMN_FRAME) || 
	      (i_column == COLUMN_TIME_MIDPT) || 
	      (i_column == COLUMN_DURATION)) 
	    display = FALSE;

	if (!gated_data) 
	  if ((i_column == COLUMN_GATE) ||
	      (i_column == COLUMN_GATE_TIME))
	    display = FALSE;

	if (display) {
	  renderer = gtk_cell_renderer_text_new ();
	  column = gtk_tree_view_column_new_with_attributes(_(analysis_titles[i_column]), renderer,
							    "text", i_column, NULL);
	  if (column_use_my_renderer[i_column]) 
	    gtk_tree_view_column_set_cell_data_func(column, renderer,
						    amitk_real_cell_data_func,
						    GINT_TO_POINTER(i_column),NULL);
	  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
	}
      }
    }
    
    /* iterate over the analysis of each volume */
    volume_analyses = roi_analyses->volume_analyses;
    while (volume_analyses != NULL) {
      frame_analyses = volume_analyses->frame_analyses;
      
      voxel_volume = AMITK_DATA_SET_VOXEL_VOLUME(volume_analyses->data_set); 
      
      /* iterate over the frames */
      /* note, use to also include:
   	                    COLUMN_STD_ERR, sqrt(frame_analyses->var/frame_analyses->voxels) 
         but I'm pretty sure this is not strictly corrected for a weighted standard  error...  
	 And most people won't (and shouldn't) be using this value anyway, as it's
	 not the experimental standard error.
      */
      frame = 0;
      while (frame_analyses != NULL) {

	gate_analyses = frame_analyses->gate_analyses;
	gate = 0;
	while (gate_analyses != NULL) {
	  gtk_list_store_append (store, &iter);  /* Acquire an iterator */
	  gtk_list_store_set (store, &iter,
			      COLUMN_ROI_NAME, AMITK_OBJECT_NAME(roi_analyses->roi),
			      COLUMN_DATA_SET_NAME,AMITK_OBJECT_NAME(volume_analyses->data_set),
			      COLUMN_FRAME, frame,
			      COLUMN_DURATION, gate_analyses->duration,
			      COLUMN_TIME_MIDPT, gate_analyses->time_midpoint,
			      COLUMN_GATE, gate,
			      COLUMN_GATE_TIME, gate_analyses->gate_time,
			      /*			      COLUMN_TOTAL, gate_analyses->total, */
			      COLUMN_MEDIAN, gate_analyses->median,
			      COLUMN_MEAN, gate_analyses->mean,
			      COLUMN_VAR, gate_analyses->var,
			      COLUMN_STD_DEV, sqrt(gate_analyses->var),
			      COLUMN_MIN,gate_analyses->min,
			      COLUMN_MAX,gate_analyses->max,
			      COLUMN_SIZE,gate_analyses->fractional_voxels*voxel_volume,
			      COLUMN_FRAC_VOXELS,gate_analyses->fractional_voxels,
			      COLUMN_VOXELS, gate_analyses->voxels,
			      -1);

	  gate_analyses = gate_analyses->next_gate_analysis;
	  gate++;
	}

	frame++;
	frame_analyses = frame_analyses->next_frame_analysis;
      }
      volume_analyses = volume_analyses->next_volume_analysis;
    }


    /* if we made the list on this iteration, place the widget*/
    if ((dynamic_data) || (gated_data) || (!static_tree_created)) {
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
      gtk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);
    
      gtk_container_add(GTK_CONTAINER(scrolled),list); /* and put it in the scrolled widget */
      static_tree_created = TRUE;
    }

    roi_analyses = roi_analyses->next_roi_analysis;
  }

  return;
}


static void read_preferences(gboolean * all_data_sets, 
			     gboolean * all_rois, 
			     analysis_calculation_t * calculation_type,
			     gboolean * accurate,
			     gdouble * subfraction,
			     gdouble * threshold_percentage,
			     gdouble * threshold_value) {

  *all_data_sets = amide_gconf_get_bool(GCONF_AMIDE_ANALYSIS,"CalculateAllDataSets");
  *all_rois = amide_gconf_get_bool(GCONF_AMIDE_ANALYSIS,"CalculateAllRois");
  *calculation_type = amide_gconf_get_int(GCONF_AMIDE_ANALYSIS,"CalculationType");
  *accurate = amide_gconf_get_bool(GCONF_AMIDE_ANALYSIS,"Accurate");
  *subfraction = amide_gconf_get_float(GCONF_AMIDE_ANALYSIS,"SubFraction");
  *threshold_percentage = amide_gconf_get_float(GCONF_AMIDE_ANALYSIS,"ThresholdPercentage");
  *threshold_value = amide_gconf_get_float(GCONF_AMIDE_ANALYSIS,"ThresholdValue");

  return;
}

static tb_roi_analysis_t * tb_roi_analysis_free(tb_roi_analysis_t * tb_roi_analysis) {

  /* sanity checks */
  g_return_val_if_fail(tb_roi_analysis != NULL, NULL);
  g_return_val_if_fail(tb_roi_analysis->reference_count > 0, NULL);

  /* remove a reference count */
  tb_roi_analysis->reference_count--;

  /* things to do if we've removed all reference's */
  if (tb_roi_analysis->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing tb_roi_analysis\n");
#endif

    if (tb_roi_analysis->preferences != NULL) {
      g_object_unref(tb_roi_analysis->preferences);
      tb_roi_analysis->preferences = NULL;
    }

    if (tb_roi_analysis->roi_analyses != NULL) {
      tb_roi_analysis->roi_analyses = analysis_roi_unref(tb_roi_analysis->roi_analyses);
    }

    g_free(tb_roi_analysis);
    tb_roi_analysis = NULL;
  }

  return tb_roi_analysis;
}

static tb_roi_analysis_t * tb_roi_analysis_init(void) {

  tb_roi_analysis_t * tb_roi_analysis;

  /* alloc space for the data structure for passing ui info */
  if ((tb_roi_analysis = g_try_new(tb_roi_analysis_t,1)) == NULL) {
    g_warning(_("couldn't allocate memory space for tb_roi_analysis_t"));
    return NULL;
  }

  tb_roi_analysis->reference_count = 1;
  tb_roi_analysis->dialog = NULL;
  tb_roi_analysis->preferences = NULL;
  tb_roi_analysis->roi_analyses = NULL;

  return tb_roi_analysis;
}


void tb_roi_analysis(AmitkStudy * study, AmitkPreferences * preferences, GtkWindow * parent) {

  tb_roi_analysis_t * tb_roi_analysis;
  GtkWidget * notebook;
  gchar * title;
  GList * rois;
  GList * data_sets;

  gboolean all_data_sets;
  gboolean all_rois;
  analysis_calculation_t calculation_type;
  gboolean accurate;
  gdouble subfraction;
  gdouble threshold_percentage;
  gdouble threshold_value;

  read_preferences(&all_data_sets, &all_rois, &calculation_type, &accurate, &subfraction, 
		   &threshold_percentage, &threshold_value);

  tb_roi_analysis = tb_roi_analysis_init();
  tb_roi_analysis->preferences = g_object_ref(preferences);

  /* figure out which data sets we're dealing with */
  if (all_data_sets)
    data_sets = amitk_object_get_children_of_type(AMITK_OBJECT(study), 
						  AMITK_OBJECT_TYPE_DATA_SET, TRUE);
  else
    data_sets = amitk_object_get_selected_children_of_type(AMITK_OBJECT(study), 
							   AMITK_OBJECT_TYPE_DATA_SET, AMITK_SELECTION_ANY, TRUE);

  if (data_sets == NULL) {
    g_warning(_("No Data Sets selected for calculating analyses"));
    return;
  }

  /* get the list of roi's we're going to be calculating over */
  if (all_rois)
    rois = amitk_object_get_children_of_type(AMITK_OBJECT(study), AMITK_OBJECT_TYPE_ROI, TRUE);
  else 
    rois = amitk_object_get_selected_children_of_type(AMITK_OBJECT(study), 
						      AMITK_OBJECT_TYPE_ROI, AMITK_SELECTION_ANY, TRUE);

  if (rois == NULL) {
    g_warning(_("No ROI's selected for calculating analyses"));
    amitk_objects_unref(data_sets);
    return;
  }

  /* calculate all our data */
  tb_roi_analysis->roi_analyses = analysis_roi_init(study, rois, data_sets, calculation_type, accurate, 
						    subfraction, threshold_percentage, threshold_value);

  rois = amitk_objects_unref(rois);
  data_sets = amitk_objects_unref(data_sets);
  g_return_if_fail(tb_roi_analysis->roi_analyses != NULL);
  
  /* start setting up the widget we'll display the info from */
  title = g_strdup_printf(_("%s Roi Analysis: Study %s"), PACKAGE, 
			  AMITK_OBJECT_NAME(study));
  tb_roi_analysis->dialog = gtk_dialog_new_with_buttons(title, GTK_WINDOW(parent),
							GTK_DIALOG_DESTROY_WITH_PARENT,
							GTK_STOCK_SAVE_AS, AMITK_RESPONSE_SAVE_AS,
							GTK_STOCK_COPY, AMITK_RESPONSE_COPY,
							"Save Raw Values", AMITK_RESPONSE_SAVE_RAW_AS,
							GTK_STOCK_HELP, GTK_RESPONSE_HELP,
							GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
							NULL);
  g_free(title);

  /* setup the callbacks for the dialog */
  g_signal_connect(G_OBJECT(tb_roi_analysis->dialog), "response", G_CALLBACK(response_cb), tb_roi_analysis);
  g_signal_connect(G_OBJECT(tb_roi_analysis->dialog), "delete_event", G_CALLBACK(delete_event_cb), tb_roi_analysis);
  g_signal_connect(G_OBJECT(tb_roi_analysis->dialog), "destroy", G_CALLBACK(destroy_cb), tb_roi_analysis);

  gtk_window_set_resizable(GTK_WINDOW(tb_roi_analysis->dialog), TRUE);


 /* make the widgets for this dialog box */
  notebook = gtk_notebook_new();
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
  gtk_container_set_border_width(GTK_CONTAINER(tb_roi_analysis->dialog), 10);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(tb_roi_analysis->dialog)->vbox), notebook);

  /* add the data pages */
  add_pages(notebook, study, tb_roi_analysis->roi_analyses);

  /* and show all our widgets */
  gtk_widget_show_all(tb_roi_analysis->dialog);

  return;
}




static void radio_buttons_cb(GtkWidget * widget, gpointer data);
static void subfraction_precentage_cb(GtkWidget * widget, gpointer data);
static void threshold_percentage_cb(GtkWidget * widget, gpointer data);
static void threshold_value_cb(GtkWidget * widget, gpointer data);



static void radio_buttons_cb(GtkWidget * widget, gpointer data) {

  gboolean all_data_sets;
  gboolean all_rois;

  all_data_sets = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "all_data_sets"));
  all_rois = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "all_rois"));

  amide_gconf_set_bool(GCONF_AMIDE_ANALYSIS,"CalculateAllDataSets",all_data_sets);
  amide_gconf_set_bool(GCONF_AMIDE_ANALYSIS,"CalculateAllRois",all_rois);

  return;
}

static void calculation_type_cb(GtkWidget * widget, gpointer data) {

  analysis_calculation_t calculation_type;
  GtkWidget * spin_buttons[3];

  calculation_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "calculation_type"));
  spin_buttons[0] = g_object_get_data(G_OBJECT(widget), "spin_button_0");
  spin_buttons[1] = g_object_get_data(G_OBJECT(widget), "spin_button_1");
  spin_buttons[2] = g_object_get_data(G_OBJECT(widget), "spin_button_2");
  gtk_widget_set_sensitive(spin_buttons[0], calculation_type == HIGHEST_FRACTION_VOXELS);
  gtk_widget_set_sensitive(spin_buttons[1], calculation_type == VOXELS_NEAR_MAX);
  gtk_widget_set_sensitive(spin_buttons[2], calculation_type == VOXELS_GREATER_THAN_VALUE);

  amide_gconf_set_int(GCONF_AMIDE_ANALYSIS,"CalculationType", calculation_type);
}

static void accurate_cb(GtkWidget * widget, gpointer data) {
  amide_gconf_set_bool(GCONF_AMIDE_ANALYSIS,"Accurate", 
		       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
  return;
}

static void subfraction_precentage_cb(GtkWidget * widget, gpointer data) {

  gdouble subfraction;

  subfraction = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget))/100.0;

  amide_gconf_set_float(GCONF_AMIDE_ANALYSIS,"SubFraction", subfraction);

  return;
}

static void threshold_percentage_cb(GtkWidget * widget, gpointer data) {

  gdouble threshold_percentage;

  threshold_percentage = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

  amide_gconf_set_float(GCONF_AMIDE_ANALYSIS,"ThresholdPercentage", threshold_percentage);

  return;
}


static void threshold_value_cb(GtkWidget * widget, gpointer data) {

  gdouble threshold_value;

  threshold_value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

  amide_gconf_set_float(GCONF_AMIDE_ANALYSIS,"ThresholdValue", threshold_value);

  return;
}


/* function to setup a dialog to allow us to choice options for rendering */
GtkWidget * tb_roi_analysis_init_dialog(GtkWindow * parent) {
  
  GtkWidget * tb_roi_init_dialog;
  gchar * temp_string;
  GtkWidget * table;
  GtkWidget * label;
  guint table_row;
  GtkWidget * radio_button[4];
  GtkWidget * hseparator;
  GtkObject * adjustment;
  GtkWidget * spin_buttons[3];
  GtkWidget * check_button;
  analysis_calculation_t i_calculation_type;
  gboolean all_data_sets;
  gboolean all_rois;
  analysis_calculation_t calculation_type;
  gboolean accurate;
  gdouble subfraction;
  gdouble threshold_percentage;
  gdouble threshold_value;

  read_preferences(&all_data_sets, &all_rois, &calculation_type, &accurate, 
		   &subfraction, &threshold_percentage, &threshold_value);

  temp_string = g_strdup_printf(_("%s: ROI Analysis Initialization Dialog"), PACKAGE);
  tb_roi_init_dialog = gtk_dialog_new_with_buttons (temp_string,  parent,
						    GTK_DIALOG_DESTROY_WITH_PARENT,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CLOSE, 
						    GTK_STOCK_EXECUTE, AMITK_RESPONSE_EXECUTE,
						    NULL);
  gtk_window_set_title(GTK_WINDOW(tb_roi_init_dialog), temp_string);
  g_free(temp_string);


  /* setup the callbacks for the dialog */
  g_signal_connect(G_OBJECT(tb_roi_init_dialog), "response", G_CALLBACK(ui_common_init_dialog_response_cb), NULL);

  gtk_container_set_border_width(GTK_CONTAINER(tb_roi_init_dialog), 10);

  /* start making the widgets for this dialog box */
  table = gtk_table_new(5,3,FALSE);
  table_row=0;
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(tb_roi_init_dialog)->vbox), table);

  label = gtk_label_new(_("Calculate:"));
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 
		   table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);
  label = gtk_label_new(_("All ROIS:"));
  gtk_table_attach(GTK_TABLE(table), label, 1,2, 
		   table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);
  label = gtk_label_new(_("Selected ROIS:"));
  gtk_table_attach(GTK_TABLE(table), label, 2,3, 
		   table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);
  table_row++;

  label = gtk_label_new(_("On All Data Sets:"));
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


  label = gtk_label_new(_("On Selected Data Sets:"));
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

  g_signal_connect(G_OBJECT(radio_button[0]), "clicked", G_CALLBACK(radio_buttons_cb), NULL);
  g_signal_connect(G_OBJECT(radio_button[1]), "clicked", G_CALLBACK(radio_buttons_cb), NULL);
  g_signal_connect(G_OBJECT(radio_button[2]), "clicked", G_CALLBACK(radio_buttons_cb), NULL);
  g_signal_connect(G_OBJECT(radio_button[3]), "clicked", G_CALLBACK(radio_buttons_cb), NULL);


  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(table), hseparator, 0,3,table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* do we want to calculate over a subfraction */
  label = gtk_label_new(_("Calculate over all voxels (normal):"));
  gtk_table_attach(GTK_TABLE(table), label, 
		   0,1, table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  radio_button[0] = gtk_radio_button_new(NULL);
  gtk_table_attach(GTK_TABLE(table), radio_button[0], 1,2,table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  g_object_set_data(G_OBJECT(radio_button[0]), "calculation_type", GINT_TO_POINTER(ALL_VOXELS));
  table_row++;


  /* do we want to calculate over a subfraction */
  label = gtk_label_new(_("Calculate over % highest voxels:"));
  gtk_table_attach(GTK_TABLE(table), label, 
		   0,1, table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  radio_button[1] = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(radio_button[0]));
  gtk_table_attach(GTK_TABLE(table), radio_button[1], 1,2,table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  g_object_set_data(G_OBJECT(radio_button[1]), "calculation_type", GINT_TO_POINTER(HIGHEST_FRACTION_VOXELS));

  adjustment = gtk_adjustment_new(100.0*subfraction, 0.0, 100.0,1.0, 1.0, 0.0);
  spin_buttons[0] = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 1.0, 0);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_buttons[0]),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(spin_buttons[0]), FALSE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_buttons[0]), FALSE);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin_buttons[0]), GTK_UPDATE_ALWAYS);
  g_signal_connect(G_OBJECT(spin_buttons[0]), "output",
		   G_CALLBACK(amitk_spin_button_scientific_output), NULL);
  g_signal_connect(G_OBJECT(spin_buttons[0]), "value_changed",  G_CALLBACK(subfraction_precentage_cb), NULL);
  gtk_table_attach(GTK_TABLE(table), spin_buttons[0], 2,3, 
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_set_sensitive(spin_buttons[0], calculation_type == HIGHEST_FRACTION_VOXELS);
  table_row++;

  /* do we want to calculate over a percentage of max */
  label = gtk_label_new(_("Calculate for voxels >= % of Max:"));
  gtk_table_attach(GTK_TABLE(table), label, 
		   0,1, table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  radio_button[2] = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(radio_button[0]));
  gtk_table_attach(GTK_TABLE(table), radio_button[2], 1,2,table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  g_object_set_data(G_OBJECT(radio_button[2]), "calculation_type", GINT_TO_POINTER(VOXELS_NEAR_MAX));

  adjustment = gtk_adjustment_new(threshold_percentage, 0.0, 100.0,1.0, 1.0, 0.0);
  spin_buttons[1] = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 1.0, 0);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_buttons[1]),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(spin_buttons[1]), FALSE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_buttons[1]), FALSE);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin_buttons[1]), GTK_UPDATE_ALWAYS);
  g_signal_connect(G_OBJECT(spin_buttons[1]), "output",
		   G_CALLBACK(amitk_spin_button_scientific_output), NULL);
  g_signal_connect(G_OBJECT(spin_buttons[1]), "value_changed",  G_CALLBACK(threshold_percentage_cb), NULL);
  gtk_table_attach(GTK_TABLE(table), spin_buttons[1], 2,3, 
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_set_sensitive(spin_buttons[1], calculation_type == VOXELS_NEAR_MAX);
  table_row++;

  /* do we want to calculate over a percentage of max */
  label = gtk_label_new(_("Calculate for voxels >= Value:"));
  gtk_table_attach(GTK_TABLE(table), label, 
		   0,1, table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  radio_button[3] = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(radio_button[0]));
  gtk_table_attach(GTK_TABLE(table), radio_button[3], 1,2,table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  g_object_set_data(G_OBJECT(radio_button[3]), "calculation_type", GINT_TO_POINTER(VOXELS_GREATER_THAN_VALUE));

  spin_buttons[2] = gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_buttons[2]), threshold_value);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_buttons[2]),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(spin_buttons[2]), FALSE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_buttons[2]), FALSE);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin_buttons[2]), GTK_UPDATE_ALWAYS);
  g_signal_connect(G_OBJECT(spin_buttons[2]), "output",
		   G_CALLBACK(amitk_spin_button_scientific_output), NULL);
  g_signal_connect(G_OBJECT(spin_buttons[2]), "value_changed",  G_CALLBACK(threshold_value_cb), NULL);
  gtk_table_attach(GTK_TABLE(table), spin_buttons[2], 2,3, 
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_set_sensitive(spin_buttons[2], calculation_type == VOXELS_GREATER_THAN_VALUE);
  table_row++;

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button[calculation_type]), TRUE);

  for (i_calculation_type=0; i_calculation_type < NUM_CALCULATION_TYPES; i_calculation_type++) {
    g_object_set_data(G_OBJECT(radio_button[i_calculation_type]), "spin_button_0", spin_buttons[0]);
    g_object_set_data(G_OBJECT(radio_button[i_calculation_type]), "spin_button_1", spin_buttons[1]);
    g_object_set_data(G_OBJECT(radio_button[i_calculation_type]), "spin_button_2", spin_buttons[2]);
    g_signal_connect(G_OBJECT(radio_button[i_calculation_type]), "clicked", 
		     G_CALLBACK(calculation_type_cb), NULL);
  }

  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(table), hseparator, 0,3,table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* do we want more accurate quantitation */
  check_button = gtk_check_button_new_with_label(_("More Accurate Quantitation (Slow)"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), accurate);
  gtk_table_attach(GTK_TABLE(table), check_button, 
		   0,2, table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(accurate_cb), tb_roi_init_dialog);
  table_row++;

  /* and show all our widgets */
  gtk_widget_show_all(tb_roi_init_dialog);

  return tb_roi_init_dialog;
}












