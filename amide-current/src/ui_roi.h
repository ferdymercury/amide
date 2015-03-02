/* ui_roi.h
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

typedef struct _ui_roi_list_t ui_roi_list_t;

struct _ui_roi_list_t {
  roi_t * roi;
  GnomeCanvasItem * canvas_roi[NUM_VIEWS];
  GtkWidget * dialog;
  GtkCTree * tree;
  GtkCTreeNode * tree_node;
  guint reference_count;
  ui_roi_list_t * next;
};

/* external functions */
ui_roi_list_t * ui_roi_list_free(ui_roi_list_t * ui_roi_list);
ui_roi_list_t * ui_roi_list_init(void);
gboolean ui_roi_list_includes_roi(ui_roi_list_t * ui_roilist, roi_t * roi);
ui_roi_list_t * ui_roi_list_add_roi(ui_roi_list_t * ui_roi_list, 
				    roi_t * roi,
				    GnomeCanvasItem * canvas_roi_item[],
				    GtkCTree * tree,
				    GtkCTreeNode * tree_node);
ui_roi_list_t * ui_roi_list_add_roi_first(ui_roi_list_t * ui_roi_list,
					  roi_t * roi,
					  GnomeCanvasItem * canvas_roi_item[],
					  GtkCTree * tree,
					  GtkCTreeNode * tree_node);
ui_roi_list_t * ui_roi_list_remove_roi(ui_roi_list_t * ui_roi_list, roi_t * roi);







