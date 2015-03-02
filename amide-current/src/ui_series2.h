/* ui_series2.h
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

/* this is a continuation of ui_series.h, it gets around some data
   dependencies (i.e. some functions use ui_study_t) */

/* external functions */
void ui_series_update_canvas_image(ui_study_t * ui_study);
void ui_series_create(ui_study_t * ui_study, view_t rot);


/* internal functions */
GtkAdjustment * ui_series_create_scroll_adjustment(ui_study_t * ui_study);
