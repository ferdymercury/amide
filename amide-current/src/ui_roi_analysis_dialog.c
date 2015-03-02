/* ui_roi_analysis_dialog.c
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
#include <math.h>
#include "ui_roi_analysis_dialog.h"
#include "ui_roi_analysis_dialog_cb.h"



/* internal defines */
#define NUM_ANALYSIS_COLUMNS 12

static gchar * analysis_titles[] = {
  "Volume",
  "Frame",
  "Duration (s)",
  "Time Midpt (s)",
  "Mean",
  "Var",
  "Std Dev",
  "Std Err",
  "Min",
  "Max",
  "Size (mm^3)",
  "Voxels"
};


void ui_roi_analysis_dialog_export(gchar * save_filename, analysis_roi_t * roi_analyses) {

  FILE * file_pointer;
  time_t current_time;
  analysis_volume_t * volume_analyses;
  analysis_frame_t * frame_analyses;
  guint frame;
  guint i;
  floatpoint_t voxel_volume;
  gboolean title_printed;

  if ((file_pointer = fopen(save_filename, "w")) == NULL) {
    g_warning("%s: couldn't open: %s for writing roi analyses", PACKAGE, save_filename);
    return;
  }

  /* intro information */
  time(&current_time);
  fprintf(file_pointer, "%s: ROI Analysis File - generated on %s", PACKAGE, ctime(&current_time));
  fprintf(file_pointer, "\n");
  fprintf(file_pointer, "Study:\t%s\n",study_name(roi_analyses->study));
  fprintf(file_pointer, "\n");
  
  while (roi_analyses != NULL) {
    fprintf(file_pointer, "ROI:\t%s\tType:\t%s\n",
	    roi_analyses->roi->name,
	    roi_type_names[roi_analyses->roi->type]);
    title_printed = FALSE;

    volume_analyses = roi_analyses->volume_analyses;
    while (volume_analyses != NULL) {
      fprintf(file_pointer, "\tVolume:\t%s\tscaling factor:\t%g\n",
	      volume_analyses->volume->name,
	      volume_analyses->volume->external_scaling);

      if (!title_printed) {
	fprintf(file_pointer, "\t\t%s", analysis_titles[1]);
	for (i=2;i<NUM_ANALYSIS_COLUMNS;i++)
	  fprintf(file_pointer, "\t%12s", analysis_titles[i]);
	fprintf(file_pointer, "\n");
	title_printed = TRUE;
      }

      voxel_volume = volume_analyses->volume->voxel_size.x*
	volume_analyses->volume->voxel_size.y*
	volume_analyses->volume->voxel_size.z;

      frame_analyses = volume_analyses->frame_analyses;
      frame = 0;
      while (frame_analyses != NULL) {
	fprintf(file_pointer, "\t\t%d", frame);
	fprintf(file_pointer, "\t% 12.3f", frame_analyses->duration);
	fprintf(file_pointer, "\t% 12.3f", frame_analyses->time_midpoint);
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
    fprintf(file_pointer, "\n");
    roi_analyses = roi_analyses->next_roi_analysis;
  }


    

  fclose(file_pointer);

  return;
}

/* create one page of our notebook */
static void ui_roi_analysis_dialog_add_page(GtkWidget * notebook, study_t * study,
					    roi_t * roi, analysis_volume_t * volume_analyses) {

  GtkWidget * table;
  gchar * line[NUM_ANALYSIS_COLUMNS];
  GtkWidget * label;
  GtkWidget * entry;
  GtkWidget * clist;
  GtkWidget * scrolled;
  GtkWidget * hbox;
  analysis_frame_t * frame_analyses;
  guint frame;
  guint table_row=0;
  guint i;
  floatpoint_t voxel_volume;
  

  label = gtk_label_new(roi->name);
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
  gtk_entry_set_text(GTK_ENTRY(entry), roi_type_names[roi->type]);
  gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 5);
 
  /* the scroll widget which the list will go into */
  scrolled = gtk_scrolled_window_new(NULL,NULL);
  gtk_widget_set_usize(scrolled,-1,250);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				 GTK_POLICY_NEVER,
				 GTK_POLICY_AUTOMATIC);
  /* and throw the clist into the packing table */
  gtk_table_attach(GTK_TABLE(table), GTK_WIDGET(scrolled), 0,5,table_row, table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL,
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);
  table_row++;

  /* put in the CLIST */
  clist = gtk_clist_new_with_titles(NUM_ANALYSIS_COLUMNS, analysis_titles);

  /* iterate over the analysis of each volume */
  while (volume_analyses != NULL) {
    frame_analyses = volume_analyses->frame_analyses;

    voxel_volume = volume_analyses->volume->voxel_size.x*
      volume_analyses->volume->voxel_size.y*
      volume_analyses->volume->voxel_size.z;

    /* iterate over the frames */
    frame = 0;
    while (frame_analyses != NULL) {
      line[0] = g_strdup(volume_analyses->volume->name);
      line[1] = g_strdup_printf("%d",frame);
      line[2] = g_strdup_printf("%5.4f",frame_analyses->duration);
      line[3] = g_strdup_printf("%5.4f",frame_analyses->time_midpoint);
      line[4] = g_strdup_printf("%5.4g",frame_analyses->mean);
      line[5] = g_strdup_printf("%5.4g",frame_analyses->var);
      line[6] = g_strdup_printf("%5.4g",sqrt(frame_analyses->var));
      line[7] = g_strdup_printf("%5.4g",sqrt(frame_analyses->var/frame_analyses->voxels));
      line[8] = g_strdup_printf("%5.4g",frame_analyses->min);
      line[9] = g_strdup_printf("%5.4g",frame_analyses->max);
      line[10] = g_strdup_printf("%5.4g", frame_analyses->voxels*voxel_volume);
      line[11] = g_strdup_printf("%5.1f",frame_analyses->voxels);

      gtk_clist_append(GTK_CLIST(clist), line);

      for (i=0;i<NUM_ANALYSIS_COLUMNS;i++)
	g_free(line[i]); /* garbage collection */

      frame++;
      frame_analyses = frame_analyses->next_frame_analysis;
    }
    volume_analyses = volume_analyses->next_volume_analysis;
  }


  /* make sure all the data is legible */
  for (i=0;i<NUM_ANALYSIS_COLUMNS;i++) 
    gtk_clist_set_column_width(GTK_CLIST(clist), i, 
			       gtk_clist_optimal_column_width(GTK_CLIST(clist), i));

  gtk_container_add(GTK_CONTAINER(scrolled),clist); /* and put it in the scrolled widget */
  table_row++;

  return;
}

/* function that sets up the roi dialog */
void ui_roi_analysis_dialog_create(ui_study_t * ui_study, gboolean all) {

  GtkWidget * dialog;
  GtkWidget * notebook;
  gchar * title;
  roi_list_t * rois;
  volume_list_t * volumes;
  analysis_roi_t * roi_analyses;
  analysis_roi_t * temp_analyses;
  guint button_num;

  /* figure out which volume we're dealing with */
  volumes = ui_volume_list_return_volume_list(ui_study->current_volumes);

  /* get the list of roi's we're going to be calculating over */
  if (all)
    rois = roi_list_add_reference(study_rois(ui_study->study));
  else 
    rois = ui_roi_list_return_roi_list(ui_study->current_rois);

  if (rois == NULL) {
    g_warning("%s: No ROI's selected for calculating analyses",PACKAGE);
    rois = roi_list_free(rois);
    volumes = volume_list_free(volumes);
    return;
  }

  /* calculate all our data */
  roi_analyses = analysis_roi_init(ui_study->study, rois, volumes);
  g_return_if_fail(roi_analyses != NULL);
  
  /* start setting up the widget we'll display the info from */
  title = g_strdup_printf("%s Roi Analysis: Study %s", PACKAGE, study_name(ui_study->study));
  dialog = gnome_dialog_new(title, "Save Analysis", GNOME_STOCK_BUTTON_CLOSE, NULL);
  gnome_dialog_set_close(GNOME_DIALOG(dialog), FALSE);

  /* order is allow shrink, allow grow, autoshrink */
  gtk_window_set_policy(GTK_WINDOW(dialog), FALSE, TRUE, FALSE); 

  gnome_dialog_button_connect(GNOME_DIALOG(dialog), 1,  /* 1 corresponds to the close button */
			      GTK_SIGNAL_FUNC(ui_roi_analysis_dialog_cb_close_button),dialog);
  gtk_signal_connect(GTK_OBJECT(dialog), "close",
		     GTK_SIGNAL_FUNC(ui_roi_analysis_dialog_cb_close),
		     roi_analyses);
  g_free(title);

  /* connect the buttons */
  button_num=0;
  gnome_dialog_button_connect(GNOME_DIALOG(dialog), button_num, 
  			      GTK_SIGNAL_FUNC(ui_roi_analysis_dialog_cb_export), roi_analyses);
  button_num++;

 /* make the widgets for this dialog box */
  notebook = gtk_notebook_new();
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);
  gtk_container_add(GTK_CONTAINER(GNOME_DIALOG(dialog)->vbox), notebook);

  /* add the data pages */
  temp_analyses = roi_analyses;
  while (temp_analyses != NULL) {
    ui_roi_analysis_dialog_add_page(notebook, ui_study->study, 
				    temp_analyses->roi, temp_analyses->volume_analyses);
    temp_analyses=temp_analyses->next_roi_analysis;
  }

  /* deallocate our lists we created */
  rois = roi_list_free(rois);
  volumes = volume_list_free(volumes);

  /* and show all our widgets */
  gtk_widget_show_all(dialog);

  return;
}











