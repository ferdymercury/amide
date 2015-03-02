/* ui_study_volumes.h
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

/* data structures */

typedef struct _ui_study_volume_list_t ui_study_volume_list_t;

struct _ui_study_volume_list_t {
  amide_volume_t * volume;
  GtkWidget * dialog;
  GtkCTree * tree;
  GtkCTreeNode * tree_node;
  ui_threshold_t * threshold;
  ui_study_volume_list_t * next;
};

/* external functions */
void ui_study_volumes_list_free(ui_study_volume_list_t ** pui_study_volumes);
ui_study_volume_list_t * ui_study_volumes_list_init(void);
ui_study_volume_list_t * ui_study_volumes_list_get_volume(ui_study_volume_list_t *list, 
							  amide_volume_t * volume);
gboolean ui_study_volumes_list_includes_volume(ui_study_volume_list_t *list, 
					       amide_volume_t * volume);
void ui_study_volumes_list_add_volume(ui_study_volume_list_t ** plist, 
				      amide_volume_t * volume,
				      GtkCTree * tree,
				      GtkCTreeNode * tree_node);
void ui_study_volumes_list_add_volume_first(ui_study_volume_list_t ** plist, 
					    amide_volume_t * volume);
void ui_study_volumes_list_remove_volume(ui_study_volume_list_t ** plist, amide_volume_t * volume);
floatpoint_t ui_study_volumes_min_dim(ui_study_volume_list_t * ui_study_volumes);
floatpoint_t ui_study_volumes_get_width(ui_study_volume_list_t * ui_study_volumes, 
					const realspace_t view_coord_frame);
floatpoint_t ui_study_volumes_get_height(ui_study_volume_list_t * ui_study_volumes, 
					 const realspace_t view_coord_frame);
floatpoint_t ui_study_volumes_get_length(ui_study_volume_list_t * ui_study_volumes, 
					 const realspace_t view_coord_frame);







