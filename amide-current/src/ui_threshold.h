/* ui_threshold.h
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

#define UI_THRESHOLD_COLOR_STRIP_WIDTH 16
#define UI_THRESHOLD_COLOR_STRIP_HEIGHT 256
#define UI_THRESHOLD_BAR_GRAPH_WIDTH 128
#define UI_THRESHOLD_TRIANGLE_WIDTH 15
#define UI_THRESHOLD_TRIANGLE_HEIGHT 15
#define UI_THRESHOLD_DEFAULT_ENTRY_WIDTH 60

/* ui_threshold data structures */
typedef struct ui_threshold_t {
  GnomeApp * app; /* pointer to the threshold window for this study */
  GnomeCanvas * color_strip;
  GnomeCanvasItem * color_strip_image;
  GnomeCanvasItem * min_arrow;
  GnomeCanvasItem * max_arrow;
  GtkWidget * max_percentage;
  GtkWidget * max_absolute;
  GtkWidget * min_percentage;
  GtkWidget * min_absolute;
} ui_threshold_t;

/* external functions */
void ui_threshold_free(ui_threshold_t ** pui_threshold);
ui_threshold_t * ui_threshold_init(void);
