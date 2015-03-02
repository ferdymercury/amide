/* ui_study_rois.c
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
#include "amide.h"
#include "realspace.h"
#include "color_table.h"
#include "volume.h"
#include "roi.h"
#include "study.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_study_rois.h"
#include "ui_study_volumes.h"
#include "ui_study.h"
#include "ui_study_rois2.h"
#include "ui_study_rois_callbacks.h"

/* free up a ui_study_roi list */
void ui_study_rois_list_free(ui_study_roi_list_t ** pui_study_rois) {

  view_t i;

  if (*pui_study_rois != NULL) {
    roi_free(&((*pui_study_rois)->roi));
    for (i=0;i<NUM_VIEWS;i++)
      if ((*pui_study_rois)->canvas_roi[i] != NULL) 
	gtk_object_destroy(GTK_OBJECT((*pui_study_rois)->canvas_roi[i]));
    if ((*pui_study_rois)->next != NULL)
      ui_study_rois_list_free(&((*pui_study_rois)->next));
    if ((*pui_study_rois)->dialog != NULL)
      gtk_signal_emit_by_name(GTK_OBJECT((*pui_study_rois)->dialog), 
			      "delete_event", NULL, *pui_study_rois);
    g_free(*pui_study_rois);
    *pui_study_rois = NULL;
  }

  return;
}

/* returns an initialized ui_study_roi_list node structure */
ui_study_roi_list_t * ui_study_rois_list_init(void) {
  
  view_t i;
  ui_study_roi_list_t * temp_roi_list;
  
  if ((temp_roi_list = 
       (ui_study_roi_list_t *) g_malloc(sizeof(ui_study_roi_list_t))) == NULL) {
    return NULL;
  }

  for (i=0;i<NUM_VIEWS;i++)
    temp_roi_list->canvas_roi[i] = NULL;
  temp_roi_list->dialog = NULL;
  temp_roi_list->tree = NULL;
  temp_roi_list->tree_node = NULL;
  temp_roi_list->next = NULL;

  return temp_roi_list;
}

/* function to check that an roi is in a ui_study_roi_list */
gboolean ui_study_rois_list_includes_roi(ui_study_roi_list_t *list, 
					 amide_roi_t * roi) {

  while (list != NULL)
    if (list->roi == roi)
      return TRUE;
    else
      list = list->next;

  return FALSE;
}


/* function to add an roi onto a ui_study_roi_list */
void ui_study_rois_list_add_roi(ui_study_roi_list_t ** plist, 
				amide_roi_t * roi,
				GnomeCanvasItem * canvas_roi_item[],
				GtkCTree * tree,
				GtkCTreeNode * tree_node) {

  ui_study_roi_list_t ** ptemp_list = plist;
  view_t i;

  while (*ptemp_list != NULL)
    ptemp_list = &((*ptemp_list)->next);

  (*ptemp_list) = ui_study_rois_list_init();
  (*ptemp_list)->roi = roi;
  for (i=0;i<NUM_VIEWS;i++) 
    (*ptemp_list)->canvas_roi[i] = canvas_roi_item[i];
  (*ptemp_list)->tree = tree;
  (*ptemp_list)->tree_node = tree_node;

  return;
}


/* function to add an roi onto a ui_study_roi_list as the first item*/
void ui_study_rois_list_add_roi_first(ui_study_roi_list_t ** plist, 
				      amide_roi_t * roi,
				      GnomeCanvasItem * canvas_roi_item[]) {

  ui_study_roi_list_t * temp_list;
  view_t i;

  temp_list = ui_study_rois_list_init();
  temp_list->roi = roi;
  for (i=0;i<NUM_VIEWS;i++) 
    temp_list->canvas_roi[i] = canvas_roi_item[i];
  temp_list->next = *plist;
  *plist = temp_list;

  return;
}


/* function to remove an roi from a ui_study_roi_list, does not delete roi */
void ui_study_rois_list_remove_roi(ui_study_roi_list_t ** plist, amide_roi_t * roi) {

  ui_study_roi_list_t * temp_list = *plist;
  ui_study_roi_list_t * prev_list = NULL;

  while (temp_list != NULL) {
    if (temp_list->roi == roi) {
      if (prev_list == NULL)
	*plist = temp_list->next;
      else
	prev_list->next = temp_list->next;
      temp_list->next = NULL;
      temp_list->roi = NULL;
      ui_study_rois_list_free(&temp_list);
    } else {
      prev_list = temp_list;
      temp_list = temp_list->next;
    }
  }

  return;
}


/* function to draw an roi for a canvas */
GnomeCanvasItem *  ui_study_rois_update_canvas_roi(ui_study_t * ui_study, 
						   view_t view, 
						   GnomeCanvasItem * roi_item,
						   amide_roi_t * roi) {

  realpoint_t offset;
  GnomeCanvasPoints * item_points;
  GnomeCanvasItem * item;
  GSList * roi_points, * temp;
  axis_t j;
  guint32 outline_color;
  floatpoint_t width,height;
  amide_volume_t * volume;

  /* sanity check */
  if (ui_study->current_slices[view] == NULL)
    return NULL;

  /* figure out which volume we're dealing with */
  if (ui_study->current_volume == NULL)
    volume = ui_study->study->volumes->volume;
  else
    volume = ui_study->current_volume;
  /* and figure out the outline color from that*/
  outline_color = 
    color_table_outline_color(volume->color_table,
			      ui_study->current_roi == roi);

  /* start by destroying the old object */
  if (roi_item != NULL) 
    gtk_object_destroy(GTK_OBJECT(roi_item));


  /* get the points */
  roi_points = 
    roi_get_volume_intersection_points(ui_study->current_slices[view]->volume, roi);

  /* count the points */
  j=0;
  temp=roi_points;
  while(temp!=NULL) {
    temp=temp->next;
    j++;
  }

  if (j<=1)
    return NULL;


  /* get some needed information */
  width = ui_study->current_slices[view]->volume->dim.x*
    ui_study->current_slices[view]->volume->voxel_size.x;
  height = ui_study->current_slices[view]->volume->dim.y*
    ui_study->current_slices[view]->volume->voxel_size.y;
  offset = 
    realspace_base_coord_to_alt(ui_study->current_slices[view]->volume->coord_frame.offset,
				ui_study->current_slices[view]->volume->coord_frame);
  /* transfer the points list to what we'll be using to construction the figure */
  item_points = gnome_canvas_points_new(j);
  temp=roi_points;
  j=0;
  while(temp!=NULL) {
    item_points->coords[j] = 
      ((((realpoint_t * ) temp->data)->x-offset.x)/width)
      *ui_study->rgb_image[view]->rgb_width + UI_STUDY_TRIANGLE_HEIGHT;
    item_points->coords[j+1] = 
      ((((realpoint_t * ) temp->data)->y-offset.y)/height)
      *ui_study->rgb_image[view]->rgb_height + UI_STUDY_TRIANGLE_HEIGHT;
    temp=temp->next;
    j += 2;
  }

  roi_free_points_list(&roi_points);

  /* create the item */
  item = 
    gnome_canvas_item_new(gnome_canvas_root(ui_study->canvas[view]),
			  gnome_canvas_line_get_type(),
			  "points", item_points,
			  "fill_color_rgba", outline_color,
			  "width_units", 1.0,
			  NULL);
  
  /* attach it's callback */
  gtk_signal_connect(GTK_OBJECT(item), "event",
		     GTK_SIGNAL_FUNC(ui_study_rois_callbacks_roi_event),
		     ui_study);

  /* free up the space used for the item's points */
  gnome_canvas_points_unref(item_points);

  return item;
}


/* function to update all the currently selected rois */
void ui_study_rois_update_canvas_rois(ui_study_t * ui_study, view_t i) {
  
  ui_study_roi_list_t * temp_roi_list=ui_study->current_rois;

  while (temp_roi_list != NULL) {
    temp_roi_list->canvas_roi[i] = 
      ui_study_rois_update_canvas_roi(ui_study,i,
				      temp_roi_list->canvas_roi[i],
				      temp_roi_list->roi);
    temp_roi_list = temp_roi_list->next;
  }

  return;
}


/* analyse the roi's, if All is true, do all roi's, otherwise only
   do selected roi's */
void ui_study_rois_calculate(ui_study_t * ui_study, gboolean all) {

  GnomeApp * app;
  gchar * title;
  gchar * line;
  guint i=0;
  guint j;
  amide_volume_t * volume;
  GtkWidget * packing_table;
  GtkWidget * text;
  amide_roi_list_t * roi_list;
  amide_roi_list_t * generated_roi_list;
  amide_roi_analysis_t analysis;
  ui_study_roi_list_t * current_roi_list;

  /* sanity check */
  if (ui_study->study->volumes == NULL)
    return;
  
  /* figure out which volume we're dealing with */
  if (ui_study->current_volume == NULL)
    volume = ui_study->study->volumes->volume;
  else
    volume = ui_study->current_volume;

  /* get the list of roi's we're going to be calculating over */
  if (all)
    roi_list = ui_study->study->rois;
  else {
    current_roi_list = ui_study->current_rois;
    roi_list = NULL;
    while (current_roi_list != NULL) {
      roi_list_add_roi(&roi_list, current_roi_list->roi);
      current_roi_list = current_roi_list->next;
    }
    generated_roi_list = roi_list;
  }
  

  /* start setting up the widget we'll display the info from */
  title = g_strdup_printf("Roi Analysis: Study %s",ui_study->study->name);
  app = GNOME_APP(gnome_app_new(PACKAGE, title));
  g_free(title);

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(app), "delete_event",
                     GTK_SIGNAL_FUNC(ui_study_rois_callbacks_delete_event),
                     ui_study);
                      

 /* make the widgets for this dialog box */
  packing_table = gtk_table_new(1,1,FALSE);
  gnome_app_set_contents(app, GTK_WIDGET(packing_table));
                          

  /* create the text widget */
  text = gtk_text_new(NULL,NULL);
  gtk_text_set_editable(GTK_TEXT(text), FALSE); /* don't want user to edit */
  gtk_text_set_word_wrap(GTK_TEXT(text), FALSE); 
  gtk_text_set_line_wrap(GTK_TEXT(text), FALSE);
  gtk_widget_set_usize(GTK_WIDGET(text), 700,0);
  
  line = g_strdup_printf("Roi Analysis:\t\tStudy: %s\t\tVolume: %s\n",
			 ui_study->study->name, volume->name);
  gtk_text_insert(GTK_TEXT(text), NULL, NULL, NULL, line, -1);
  g_free(line);

  line = g_strdup_printf("-------------------------------------------------\n");
  gtk_text_insert(GTK_TEXT(text), NULL, NULL, NULL, line, -1);
  g_free(line);

  line = g_strdup_printf("Roi #\tFrame\tVoxels\t\tMean\t\tVar\t\t\tStd E\t\tMin\t\t\tMax\t\t\t\tRoi Name\t\tType\n");
  gtk_text_insert(GTK_TEXT(text), NULL, NULL, NULL, line, -1);
  g_free(line);

  while(roi_list != NULL) {
    for (j=0;j<volume->num_frames;j++) {
      /* calculate the info for this roi */
      i++;
      analysis = roi_calculate_analysis(roi_list->roi,volume,
					roi_list->roi->grain,
					j);
      
      /* print the info for this roi */
      line = 
	g_strdup_printf("%d\t%d\t\t\t%5.3f\t\t%5.3f\t\t%5.3f\t\t%5.3f\t\t%5.3f\t\t%5.3f\t\t%s\t\t%s\n",
			i, 
			j,
			analysis.voxels,
			analysis.mean,
			analysis.var,
			sqrt(analysis.var/analysis.voxels),
			analysis.min,
			analysis.max,
			roi_list->roi->name,
			roi_type_names[roi_list->roi->type]);
      gtk_text_insert(GTK_TEXT(text), NULL, NULL, NULL, line, -1);
      g_free(line);
    }
    roi_list = roi_list->next; /* go to next roi */
  }


  /* and throw the text box into the packing table */
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(text), 0,1,0,1,
		   X_PACKING_OPTIONS | GTK_FILL,
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);

  /* deallocate our roi list if we created it */
  if (!all)
    while(generated_roi_list != NULL)
      roi_list_remove_roi(&generated_roi_list, generated_roi_list->roi);

  /* and show all our widgets */
  gtk_widget_show_all(GTK_WIDGET(app));

  return;
}





