/* ui_common.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2002 Andy Loening
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

/* header files that are always needed with this file */
#include "amide.h"
#include <gnome.h>

#define UI_COMMON_NEW_ROI_MODE_CURSOR GDK_DRAFT_SMALL
#define UI_COMMON_NEW_ROI_MOTION_CURSOR GDK_PENCIL
#define UI_COMMON_OLD_ROI_MODE_CURSOR GDK_DRAFT_SMALL
#define UI_COMMON_OLD_ROI_RESIZE_CURSOR GDK_SIZING
#define UI_COMMON_OLD_ROI_ROTATE_CURSOR GDK_EXCHANGE
#define UI_COMMON_OLD_ROI_SHIFT_CURSOR GDK_FLEUR
#define UI_COMMON_OLD_ROI_ISOCONTOUR_CURSOR GDK_DRAFT_SMALL
#define UI_COMMON_OLD_ROI_ERASE_CURSOR GDK_DRAFT_SMALL
#define UI_COMMON_DATA_SET_MODE_CURSOR GDK_CROSSHAIR
#define UI_COMMON_FIDUCIAL_MARK_MODE_CURSOR GDK_DOTBOX
#define UI_COMMON_WAIT_CURSOR GDK_WATCH

typedef enum {
  UI_CURSOR_DEFAULT,
  UI_CURSOR_NEW_ROI_MODE,
  UI_CURSOR_NEW_ROI_MOTION, 
  UI_CURSOR_OLD_ROI_MODE,
  UI_CURSOR_OLD_ROI_RESIZE,
  UI_CURSOR_OLD_ROI_ROTATE,
  UI_CURSOR_OLD_ROI_SHIFT,
  UI_CURSOR_OLD_ROI_ISOCONTOUR,
  UI_CURSOR_OLD_ROI_ERASE,
  UI_CURSOR_DATA_SET_MODE, 
  UI_CURSOR_FIDUCIAL_MARK_MODE,
  UI_CURSOR_WAIT,
  NUM_CURSORS
} ui_common_cursor_t;

#define UI_CURSOR_RENDERING_ROTATE_XY UI_CURSOR_OLD_ROI_SHIFT
#define UI_CURSOR_RENDERING_ROTATE_Z UI_CURSOR_OLD_ROI_ROTATE

/* external functions */
void ui_common_entry_name_cb(gchar * entry_string, gpointer data);
void ui_common_file_selection_cancel_cb(GtkWidget* widget, gpointer data);
void ui_common_about_cb(GtkWidget * button, gpointer data);
GtkWidget * ui_common_create_view_axis_indicator(AmitkLayout layout);
void ui_common_window_realize_cb(GtkWidget * widget, gpointer data);
void ui_common_place_cursor(ui_common_cursor_t which_cursor, GtkWidget * widget);
void ui_common_remove_cursor(GtkWidget * widget);



/* external variables */
extern GnomeUIInfo ui_common_help_menu[];
extern GdkCursor * ui_common_cursor[NUM_CURSORS];
