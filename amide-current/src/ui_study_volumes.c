/* ui_study_volumes.c
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

/* free up a ui_study_volume list */
void ui_study_volumes_list_free(ui_study_volume_list_t ** pui_study_volumes) {

  if (*pui_study_volumes != NULL) {
    volume_free(&((*pui_study_volumes)->volume));
    if ((*pui_study_volumes)->next != NULL)
      ui_study_volumes_list_free(&((*pui_study_volumes)->next));
    if ((*pui_study_volumes)->dialog != NULL)
      gtk_signal_emit_by_name(GTK_OBJECT((*pui_study_volumes)->dialog), 
			      "delete_event", NULL, *pui_study_volumes);
    g_free(*pui_study_volumes);
    *pui_study_volumes = NULL;
  }

  return;
}

/* returns an initialized ui_study_volume_list node structure */
ui_study_volume_list_t * ui_study_volumes_list_init(void) {
  
  ui_study_volume_list_t * temp_volume_list;
  
  if ((temp_volume_list = 
       (ui_study_volume_list_t *) g_malloc(sizeof(ui_study_volume_list_t))) == NULL) {
    return NULL;
  }

  temp_volume_list->dialog = NULL;
  temp_volume_list->tree = NULL;
  temp_volume_list->tree_node = NULL;
  temp_volume_list->threshold = NULL;
  temp_volume_list->next = NULL;

  return temp_volume_list;
}

/* function to return a pointer to the list element containing the specified volume */
ui_study_volume_list_t * ui_study_volumes_list_get_volume(ui_study_volume_list_t *list, 
							  amide_volume_t * volume) {

  while (list != NULL)
    if (list->volume == volume)
      return list;
    else
      list = list->next;

  return NULL;
}

/* function to check that a volume is in a ui_study_volume_list */
gboolean ui_study_volumes_list_includes_volume(ui_study_volume_list_t *list, 
					       amide_volume_t * volume) {

  if (ui_study_volumes_list_get_volume(list,volume) == NULL)
    return FALSE;
  else
    return TRUE;
}


/* function to add an volume onto a ui_study_volume_list */
void ui_study_volumes_list_add_volume(ui_study_volume_list_t ** plist, 
				      amide_volume_t * volume,
				      GtkCTree * tree,
				      GtkCTreeNode * tree_node) {

  ui_study_volume_list_t ** ptemp_list = plist;

  while (*ptemp_list != NULL)
    ptemp_list = &((*ptemp_list)->next);

  (*ptemp_list) = ui_study_volumes_list_init();
  (*ptemp_list)->volume = volume;
  (*ptemp_list)->tree = tree;
  (*ptemp_list)->tree_node = tree_node;

  return;
}


/* function to add a volume onto a ui_study_volume_list as the first item*/
void ui_study_volumes_list_add_volume_first(ui_study_volume_list_t ** plist, 
					    amide_volume_t * volume) {

  ui_study_volume_list_t * temp_list;

  temp_list = ui_study_volumes_list_init();
  temp_list->volume = volume;
  temp_list->next = *plist;
  *plist = temp_list;

  return;
}


/* function to remove a volume from a ui_study_volume_list, does not delete volume */
void ui_study_volumes_list_remove_volume(ui_study_volume_list_t ** plist, amide_volume_t * volume) {

  ui_study_volume_list_t * temp_list = *plist;
  ui_study_volume_list_t * prev_list = NULL;

  while (temp_list != NULL) {
    if (temp_list->volume == volume) {
      if (prev_list == NULL)
	*plist = temp_list->next;
      else
	prev_list->next = temp_list->next;
      temp_list->next = NULL;
      temp_list->volume = NULL;
      ui_study_volumes_list_free(&temp_list);
    } else {
      prev_list = temp_list;
      temp_list = temp_list->next;
    }
  }

  return;
}


/* returns the minimum dimensional width of the volume with the largest voxel size */
floatpoint_t ui_study_volumes_min_dim(ui_study_volume_list_t * ui_study_volumes) {
				      
  ui_study_volume_list_t * temp_ui_study_volumes;
  amide_volume_list_t * temp_volumes=NULL;
  floatpoint_t value;
  
  g_assert(ui_study_volumes!=NULL);

  /* first, generate an amide_volume_list we can pass to volumes_min_dim */
  temp_ui_study_volumes = ui_study_volumes;
  while (temp_ui_study_volumes != NULL) {
    volume_list_add_volume(&temp_volumes, temp_ui_study_volumes->volume);
    temp_ui_study_volumes = temp_ui_study_volumes->next;
  }

  /* now calculate the value */
  value=volumes_min_dim(temp_volumes);
  
  /* and delete the amide_volume_list without destroying the volumes */
  temp_ui_study_volumes = ui_study_volumes;
  while (temp_ui_study_volumes != NULL) {
    volume_list_remove_volume(&temp_volumes, temp_ui_study_volumes->volume);
    temp_ui_study_volumes = temp_ui_study_volumes->next;
  }

  return value;
}

/* returns the width of the assembly of volumes wrt the given axis */
floatpoint_t ui_study_volumes_get_width(ui_study_volume_list_t * ui_study_volumes, 
					const realspace_t view_coord_frame) {
  ui_study_volume_list_t * temp_ui_study_volumes;
  amide_volume_list_t * temp_volumes=NULL;
  floatpoint_t value;
  
  g_assert(ui_study_volumes!=NULL);

  /* first, generate an amide_volume_list we can pass to volumes_get_width */
  temp_ui_study_volumes = ui_study_volumes;
  while (temp_ui_study_volumes != NULL) {
    volume_list_add_volume(&temp_volumes, temp_ui_study_volumes->volume);
    temp_ui_study_volumes = temp_ui_study_volumes->next;
  }

  /* now calculate the value */
  value=volumes_get_width(temp_volumes, view_coord_frame);
  
  /* and delete the amide_volume_list without destroying the volumes */
  temp_ui_study_volumes = ui_study_volumes;
  while (temp_ui_study_volumes != NULL) {
    volume_list_remove_volume(&temp_volumes, temp_ui_study_volumes->volume);
    temp_ui_study_volumes = temp_ui_study_volumes->next;
  }

  return value;
}

/* returns the height of the assembly of volumes wrt the given axis */
floatpoint_t ui_study_volumes_get_height(ui_study_volume_list_t * ui_study_volumes, 
					 const realspace_t view_coord_frame) {
  ui_study_volume_list_t * temp_ui_study_volumes;
  amide_volume_list_t * temp_volumes=NULL;
  floatpoint_t value;
  
  g_assert(ui_study_volumes!=NULL);

  /* first, generate an amide_volume_list we can pass to volumes_get_height */
  temp_ui_study_volumes = ui_study_volumes;
  while (temp_ui_study_volumes != NULL) {
    volume_list_add_volume(&temp_volumes, temp_ui_study_volumes->volume);
    temp_ui_study_volumes = temp_ui_study_volumes->next;
  }

  /* now calculate the value */
  value=volumes_get_height(temp_volumes, view_coord_frame);
  
  /* and delete the amide_volume_list without destroying the volumes */
  temp_ui_study_volumes = ui_study_volumes;
  while (temp_ui_study_volumes != NULL) {
    volume_list_remove_volume(&temp_volumes, temp_ui_study_volumes->volume);
    temp_ui_study_volumes = temp_ui_study_volumes->next;
  }

  return value;
}

/* returns the length of the assembly of volumes wrt the given axis */
floatpoint_t ui_study_volumes_get_length(ui_study_volume_list_t * ui_study_volumes, 
					 const realspace_t view_coord_frame) {
  ui_study_volume_list_t * temp_ui_study_volumes;
  amide_volume_list_t * temp_volumes=NULL;
  floatpoint_t value;
  
  g_assert(ui_study_volumes!=NULL);

  /* first, generate an amide_volume_list we can pass to volumes_get_length */
  temp_ui_study_volumes = ui_study_volumes;
  while (temp_ui_study_volumes != NULL) {
    volume_list_add_volume(&temp_volumes, temp_ui_study_volumes->volume);
    temp_ui_study_volumes = temp_ui_study_volumes->next;
  }

  /* now calculate the value */
  value=volumes_get_length(temp_volumes, view_coord_frame);
  
  /* and delete the amide_volume_list without destroying the volumes */
  temp_ui_study_volumes = ui_study_volumes;
  while (temp_ui_study_volumes != NULL) {
    volume_list_remove_volume(&temp_volumes, temp_ui_study_volumes->volume);
    temp_ui_study_volumes = temp_ui_study_volumes->next;
  }

  return value;
}



