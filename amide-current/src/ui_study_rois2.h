/* ui_study_rois2.h
 *
 * Part of amide - Amide's a Medical Image Dataset Viewer
 * Copyright (C) 2000 Andy Loening
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

/* this is a continuation of ui_study_rois.h, it gets around some data
   dependencies (i.e. some functions use ui_study_t) */

/* external functions */
GnomeCanvasItem *  ui_study_rois_update_canvas_roi(ui_study_t * ui_study, 
						   view_t i, 
						   GnomeCanvasItem * roi_item,
						   amide_roi_t * roi);
void ui_study_rois_update_canvas_rois(ui_study_t * ui_study, view_t i);
void ui_study_rois_calculate(ui_study_t * ui_study, gboolean All);






