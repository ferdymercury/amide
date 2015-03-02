/* ui_align_pt.c
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
#include "ui_align_pt.h"

/* free up a ui_align_pts list */
ui_align_pts_t * ui_align_pts_free(ui_align_pts_t * ui_align_pts) {

  view_t i_view;
  ui_align_pts_t * return_list;

  if (ui_align_pts == NULL) return ui_align_pts;

  /* sanity checks */
  g_return_val_if_fail(ui_align_pts->reference_count > 0, NULL);

  ui_align_pts->reference_count--;

  /* things to do if we've removed all reference's */
  if (ui_align_pts->reference_count == 0) {
    return_list = ui_align_pts_free(ui_align_pts->next);
    ui_align_pts->next = NULL;
    ui_align_pts->align_pt = align_pt_free(ui_align_pts->align_pt);
    for (i_view=0;i_view<NUM_VIEWS;i_view++)
      if (ui_align_pts->canvas_pt[i_view] != NULL) {
	gtk_object_destroy(GTK_OBJECT(ui_align_pts->canvas_pt[i_view]));
	ui_align_pts->canvas_pt[i_view] = NULL;
      }
    if (ui_align_pts->dialog != NULL) {
      gnome_dialog_close(GNOME_DIALOG(ui_align_pts->dialog));
      ui_align_pts->dialog = NULL;
    }
    g_free(ui_align_pts);
    ui_align_pts = NULL;
  } else
    return_list = ui_align_pts;

  return return_list;
}

/* returns an initialized ui_align_pts node structure */
ui_align_pts_t * ui_align_pts_init(align_pt_t * align_pt) {

  view_t i_view;
  ui_align_pts_t * temp_pts;
  
  if ((temp_pts = (ui_align_pts_t *) g_malloc(sizeof(ui_align_pts_t))) == NULL) {
    return temp_pts;
  }
  temp_pts->reference_count = 1;

  temp_pts->align_pt = align_pt_add_reference(align_pt);
  for (i_view=0;i_view<NUM_VIEWS;i_view++)
    temp_pts->canvas_pt[i_view] = NULL;
  temp_pts->dialog = NULL;
  temp_pts->tree_leaf = NULL;
  temp_pts->next = NULL;

  return temp_pts;
}

/* return the number of elements in a pts list */
guint ui_align_pts_count(ui_align_pts_t * ui_align_pts) {

  if (ui_align_pts == NULL) return 0;
  else return (1+ui_align_pts_count(ui_align_pts->next));
}

/* function to return a pointer to the list element containing the specified point */
ui_align_pts_t * ui_align_pts_get_ui_pt(ui_align_pts_t * ui_align_pts, align_pt_t * align_pt) {

  if (ui_align_pts == NULL)
    return ui_align_pts;
  else if (ui_align_pts->align_pt == align_pt)
    return ui_align_pts;
  else
    return ui_align_pts_get_ui_pt(ui_align_pts->next, align_pt);
}

/* function to check that an alignment point is in a ui_align_pts list */
gboolean ui_align_pts_includes_pt(ui_align_pts_t * ui_align_pts, align_pt_t * align_pt) {
  if (ui_align_pts_get_ui_pt(ui_align_pts, align_pt) == NULL)
    return FALSE;
  else
    return TRUE;
}


/* function to add an alignment point onto a ui_align_pts list */
ui_align_pts_t * ui_align_pts_add_pt(ui_align_pts_t * ui_align_pts,
				     align_pt_t * align_pt,
				     GnomeCanvasItem * canvas_pt_item[],
				     GtkWidget * tree_leaf) {

  ui_align_pts_t * temp_list = ui_align_pts;
  ui_align_pts_t * prev_list = NULL;
  view_t i_view;

  /* get to the end of the list */
  while (temp_list != NULL) {
    prev_list = temp_list;
    temp_list = temp_list->next;
  }

  /* get a new structure */
  temp_list = ui_align_pts_init(align_pt);

  if (canvas_pt_item != NULL)
    for (i_view=0;i_view<NUM_VIEWS;i_view++) 
      temp_list->canvas_pt[i_view] = canvas_pt_item[i_view];
  temp_list->tree_leaf = tree_leaf;

  if  (ui_align_pts == NULL)
    return temp_list;
  else {
    prev_list->next = temp_list;
    return ui_align_pts;
  }
}


/* function to add an alignment point onto a ui_align_pts list as the first item*/
ui_align_pts_t * ui_align_pts_add_pt_first(ui_align_pts_t * ui_align_pts,
					   align_pt_t * align_pt,
					   GnomeCanvasItem * canvas_pt_item[],
					   GtkWidget * tree_leaf) {

  ui_align_pts_t * temp_list;

  temp_list = ui_align_pts_add_pt(NULL,align_pt,canvas_pt_item,tree_leaf);
  temp_list->next = ui_align_pts;

  return temp_list;
}


/* function to remove an alignment point from a ui_align_pt_list, does not delete pt */
ui_align_pts_t * ui_align_pts_remove_pt(ui_align_pts_t * ui_align_pts, align_pt_t * align_pt) {

  ui_align_pts_t * temp_list = ui_align_pts;
  ui_align_pts_t * prev_list = NULL;

  while (temp_list != NULL) {
    if (temp_list->align_pt == align_pt) {
      if (prev_list == NULL)
	ui_align_pts = temp_list->next;
      else
	prev_list->next = temp_list->next;
      temp_list->next = NULL;
      temp_list = ui_align_pts_free(temp_list);
    } else {
      prev_list = temp_list;
      temp_list = temp_list->next;
    }
  }

  return ui_align_pts;
}

/* function to generate a align_pts list from a ui_align_pts list */
align_pts_t * ui_align_pts_return_align_pts(ui_align_pts_t * ui_align_pts) {
  if (ui_align_pts == NULL) 
    return NULL;
  else 
    return align_pts_add_pt(ui_align_pts_return_align_pts(ui_align_pts->next), 
			    ui_align_pts->align_pt);
}


