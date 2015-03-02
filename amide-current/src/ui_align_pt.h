/* ui_align_pt.h
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

/* header files always needed with this file */
#include "align_pt.h"

/* data structures */
typedef struct _ui_align_pts_t ui_align_pts_t;

struct _ui_align_pts_t {
  align_pt_t * align_pt;
  GnomeCanvasItem * canvas_pt[NUM_VIEWS];
  GtkWidget * dialog;
  GtkWidget * tree_leaf;
  guint reference_count;
  ui_align_pts_t * next;
};

/* external functions */
ui_align_pts_t * ui_align_pts_free(ui_align_pts_t * ui_align_pts);
ui_align_pts_t * ui_align_pts_init(align_pt_t * align_pt);
guint ui_align_pts_count(ui_align_pts_t * ui_align_pts_list);
ui_align_pts_t * ui_align_pts_get_ui_pt(ui_align_pts_t * ui_align_pts, align_pt_t * align_pt);
gboolean ui_align_pts_includes_pt(ui_align_pts_t * ui_align_pts, align_pt_t * align_pt);
ui_align_pts_t * ui_align_pts_add_pt(ui_align_pts_t * ui_align_pts,
				     align_pt_t * align_pt,
				     GnomeCanvasItem * canvas_pt_item[],
				     GtkWidget * tree_leaf);
ui_align_pts_t * ui_align_pts_add_pt_first(ui_align_pts_t * ui_align_pts,
					   align_pt_t * align_pt,
					   GnomeCanvasItem * canvas_pt_item[],
					   GtkWidget * tree_leaf);
ui_align_pts_t * ui_align_pts_remove_pt(ui_align_pts_t * ui_align_pts, align_pt_t * align_pt);
align_pts_t * ui_align_pts_return_align_pts(ui_align_pts_t * ui_align_pts);








