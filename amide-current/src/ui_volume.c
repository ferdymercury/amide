/* ui_volume.c
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
#include "ui_volume.h"


/* free up a ui_volume list */
ui_volume_list_t * ui_volume_list_free(ui_volume_list_t * ui_volume_list) {

  if (ui_volume_list == NULL) return ui_volume_list;

  /* sanity checks */
  g_return_val_if_fail(ui_volume_list->reference_count > 0, NULL);

  ui_volume_list->reference_count--;

  /* things to do if we've removed all reference's */
  if (ui_volume_list->reference_count == 0) {
    ui_volume_list->next = ui_volume_list_free(ui_volume_list->next);
    ui_volume_list->volume = volume_free(ui_volume_list->volume);
    if (ui_volume_list->dialog != NULL) {
      gnome_dialog_close(GNOME_DIALOG(ui_volume_list->dialog));
      ui_volume_list->dialog = NULL;
    }			 
    g_free(ui_volume_list);
    ui_volume_list = NULL;
  }

  return ui_volume_list;

}

/* returns an initialized ui_volume_list node structure */
ui_volume_list_t * ui_volume_list_init(void) {
  
  ui_volume_list_t * temp_volume_list;
  
  if ((temp_volume_list = 
       (ui_volume_list_t *) g_malloc(sizeof(ui_volume_list_t))) == NULL) {
    return NULL;
  }
  temp_volume_list->reference_count = 1;

  temp_volume_list->volume = NULL;
  temp_volume_list->dialog = NULL;
  temp_volume_list->tree_leaf = NULL;
  temp_volume_list->threshold = NULL;
  temp_volume_list->series_list = NULL;
  temp_volume_list->next = NULL;

  return temp_volume_list;
}

/* function to return a pointer to the first volume in the ui_volume_list, returns NULL if none */
volume_t * ui_volume_list_get_first_volume(ui_volume_list_t * ui_volume_list) {
  if (ui_volume_list == NULL)
    return NULL;
  else
    return ui_volume_list->volume;
}


/* function to return a pointer to the list element containing the specified volume */
ui_volume_list_t * ui_volume_list_get_ui_volume(ui_volume_list_t * ui_volume_list, volume_t * volume) {

  if (ui_volume_list == NULL)
    return ui_volume_list;
  else if (ui_volume_list->volume == volume)
    return ui_volume_list;
  else
    return ui_volume_list_get_ui_volume(ui_volume_list->next, volume);
}

/* function to check that a volume is in a ui_volume_list */
gboolean ui_volume_list_includes_volume(ui_volume_list_t * ui_volume_list, volume_t * volume) {

  if (ui_volume_list_get_ui_volume(ui_volume_list,volume) == NULL)
    return FALSE;
  else
    return TRUE;
}


/* function to add a volume onto a ui_volume_list */
ui_volume_list_t * ui_volume_list_add_volume(ui_volume_list_t * ui_volume_list, 
					     volume_t * volume,
					     GtkWidget * tree_leaf) {


  ui_volume_list_t * temp_list = ui_volume_list;
  ui_volume_list_t * prev_list = NULL;

  /* get to the end of the list */
  while (temp_list != NULL) {
    prev_list = temp_list;
    temp_list = temp_list->next;
  }

  /* get a new structure */
  temp_list = ui_volume_list_init();

  /* add the volume and other info to this list structure */
  temp_list->volume = volume_add_reference(volume);
  temp_list->tree_leaf = tree_leaf;

  if  (ui_volume_list == NULL)
    return temp_list;
  else {
    prev_list->next = temp_list;
    return ui_volume_list;
  }
}

/* function to add a volume onto a ui_volume_list as the first item*/
ui_volume_list_t * ui_volume_list_add_volume_first(ui_volume_list_t * ui_volume_list, 
					     volume_t * volume, GtkWidget * tree_leaf) {

  ui_volume_list_t * temp_list;

  temp_list = ui_volume_list_add_volume(NULL,volume,tree_leaf);
  temp_list->next = ui_volume_list;

  return temp_list;
}


/* function to remove a volume from a ui_volume_list */
ui_volume_list_t * ui_volume_list_remove_volume(ui_volume_list_t * ui_volume_list, volume_t * volume) {

  ui_volume_list_t * temp_list = ui_volume_list;
  ui_volume_list_t * prev_list = NULL;

  while (temp_list != NULL) {
    if (temp_list->volume == volume) {
      if (prev_list == NULL)
	ui_volume_list = temp_list->next;
      else
	prev_list->next = temp_list->next;
      temp_list->next = NULL;
      temp_list = ui_volume_list_free(temp_list);
    } else {
      prev_list = temp_list;
      temp_list = temp_list->next;
    }
  }

  return ui_volume_list;
}

/* function to generate a volume_list_t list from a ui_volume_list_t list */
volume_list_t * ui_volume_list_return_volume_list(ui_volume_list_t * ui_volume_list) {
  if (ui_volume_list == NULL) 
    return NULL;
  else 
    return volume_list_add_volume(ui_volume_list_return_volume_list(ui_volume_list->next),
				  ui_volume_list->volume);
}

/* returns the minimum dimensional width of the volume with the largest voxel size */
floatpoint_t ui_volume_list_max_min_voxel_size(ui_volume_list_t * ui_volume_list) {
				      
  volume_list_t * temp_volumes;
  floatpoint_t value;
  
  g_assert(ui_volume_list != NULL);

  /* first, generate a volume_list we can pass to volumes_min_dim */
  temp_volumes = ui_volume_list_return_volume_list(ui_volume_list);
  value=volumes_max_min_voxel_size(temp_volumes); /* now calculate the value */
  temp_volumes = volume_list_free(temp_volumes); /* and delete the volume_list */

  return value;
}




/* returns corners in the view coord frame that completely encompase the volumes */
void ui_volume_list_get_view_corners(ui_volume_list_t * ui_volume_list,
				     const realspace_t view_coord_frame,
				     realpoint_t view_corner[]) {

  volume_list_t * temp_volumes;
  
  g_assert(ui_volume_list != NULL);

  /* first, generate a volume_list we can pass to volumes_get_view_corners */
  temp_volumes = ui_volume_list_return_volume_list(ui_volume_list);
  volumes_get_view_corners(temp_volumes, view_coord_frame, view_corner); 
  temp_volumes = volume_list_free(temp_volumes); /* and delete the volume_list */

  return;
}



