/* ui_roi.c
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
#include "ui_roi.h"

/* free up a ui_roi list */
ui_roi_list_t * ui_roi_list_free(ui_roi_list_t * ui_roi_list) {

  view_t i_view;

  if (ui_roi_list == NULL)
    return ui_roi_list;

  /* sanity checks */
  g_return_val_if_fail(ui_roi_list->reference_count > 0, NULL);

  
  /* remove a reference count */
  ui_roi_list->reference_count--;

  /* things we always do */
  ui_roi_list->next = ui_roi_list_free(ui_roi_list->next);
  ui_roi_list->roi = roi_free(ui_roi_list->roi);


  /* things to do if we've removed all reference's */
  if (ui_roi_list->reference_count == 0) {
    for (i_view=0;i_view<NUM_VIEWS;i_view++)
      if (ui_roi_list->canvas_roi[i_view] != NULL) {
	gtk_object_destroy(GTK_OBJECT(ui_roi_list->canvas_roi[i_view]));
	ui_roi_list->canvas_roi[i_view] = NULL;
      }
    if (ui_roi_list->dialog != NULL)
      gtk_signal_emit_by_name(GTK_OBJECT(ui_roi_list->dialog), 
			      "delete_event", NULL, ui_roi_list);
    g_free(ui_roi_list);
    ui_roi_list = NULL;
  }

  return ui_roi_list;
}

/* returns an initialized ui_roi_list node structure */
ui_roi_list_t * ui_roi_list_init(void) {

  view_t i_view;
  ui_roi_list_t * temp_roi_list;
  
  if ((temp_roi_list = 
       (ui_roi_list_t *) g_malloc(sizeof(ui_roi_list_t))) == NULL) {
    return NULL;
  }
  temp_roi_list->reference_count = 1;

  temp_roi_list->roi = NULL;
  for (i_view=0;i_view<NUM_VIEWS;i_view++)
    temp_roi_list->canvas_roi[i_view] = NULL;
  temp_roi_list->dialog = NULL;
  temp_roi_list->tree_leaf = NULL;
  temp_roi_list->next = NULL;

  return temp_roi_list;
}

/* function to return a pointer to the list element containing the specified roi */
ui_roi_list_t * ui_roi_list_get_ui_roi(ui_roi_list_t * ui_roi_list, roi_t * roi) {

  if (ui_roi_list == NULL)
    return ui_roi_list;
  else if (ui_roi_list->roi == roi)
    return ui_roi_list;
  else
    return ui_roi_list_get_ui_roi(ui_roi_list->next, roi);
}

/* function to check that an roi is in a ui_roi_list */
gboolean ui_roi_list_includes_roi(ui_roi_list_t * ui_roi_list, roi_t * roi) {

  if (ui_roi_list_get_ui_roi(ui_roi_list,roi) == NULL)
    return FALSE;
  else
    return TRUE;
}


/* function to add an roi onto a ui_roi_list */
ui_roi_list_t * ui_roi_list_add_roi(ui_roi_list_t * ui_roi_list, 
				    roi_t * roi,
				    GnomeCanvasItem * canvas_roi_item[],
				    GtkWidget * tree_leaf) {

  ui_roi_list_t * temp_list = ui_roi_list;
  ui_roi_list_t * prev_list = NULL;
  view_t i_view;

  /* get to the end of the list */
  while (temp_list != NULL) {
    prev_list = temp_list;
    temp_list = temp_list->next;
  }

  /* get a new structure */
  temp_list = ui_roi_list_init();

  /* add the roi and other info to this list structure */
  temp_list->roi = roi_add_reference(roi);
  for (i_view=0;i_view<NUM_VIEWS;i_view++) 
    temp_list->canvas_roi[i_view] = canvas_roi_item[i_view];
  temp_list->tree_leaf = tree_leaf;

  if  (ui_roi_list == NULL)
    return temp_list;
  else {
    prev_list->next = temp_list;
    return ui_roi_list;
  }
}


/* function to add an roi onto a ui_roi_list as the first item*/
ui_roi_list_t * ui_roi_list_add_roi_first(ui_roi_list_t * ui_roi_list,
					  roi_t * roi,
					  GnomeCanvasItem * canvas_roi_item[],
					  GtkWidget * tree_leaf) {

  ui_roi_list_t * temp_list;

  temp_list = ui_roi_list_add_roi(NULL,roi,canvas_roi_item,tree_leaf);
  temp_list->next = ui_roi_list;

  return temp_list;
}


/* function to remove an roi from a ui_roi_list, does not delete roi */
ui_roi_list_t * ui_roi_list_remove_roi(ui_roi_list_t * ui_roi_list, roi_t * roi) {

  ui_roi_list_t * temp_list = ui_roi_list;
  ui_roi_list_t * prev_list = NULL;

  while (temp_list != NULL) {
    if (temp_list->roi == roi) {
      if (prev_list == NULL)
	ui_roi_list = temp_list->next;
      else
	prev_list->next = temp_list->next;
      temp_list->next = NULL;
      temp_list = ui_roi_list_free(temp_list);
    } else {
      prev_list = temp_list;
      temp_list = temp_list->next;
    }
  }

  return ui_roi_list;
}



