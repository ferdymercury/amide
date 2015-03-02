/* ui_common.c
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
#include "ui_common.h"
#include "ui_common_cb.h"

/* our help menu... */
GnomeUIInfo ui_common_help_menu[]= {
  GNOMEUIINFO_HELP(PACKAGE),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_ABOUT_ITEM(ui_common_cb_about, NULL), 
  GNOMEUIINFO_END
};


/* an array to hold the preloaded cursors */
GdkCursor * ui_common_cursor[NUM_CURSORS];

/* internal variables */
static gboolean ui_common_cursors_initialized = FALSE;
static GSList * ui_common_cursor_stack=NULL;

/* load in the cursors */
static void ui_common_cursor_init(void) {

  /* load in the cursors */
  ui_common_cursor[UI_CURSOR_DEFAULT] = NULL;
  ui_common_cursor[UI_CURSOR_NEW_ROI_MODE] =  gdk_cursor_new(UI_COMMON_NEW_ROI_MODE_CURSOR);
  ui_common_cursor[UI_CURSOR_NEW_ROI_MOTION] = gdk_cursor_new(UI_COMMON_NEW_ROI_MOTION_CURSOR);
  ui_common_cursor[UI_CURSOR_NO_ROI_MODE] = gdk_cursor_new(UI_COMMON_NO_ROI_MODE_CURSOR);
  ui_common_cursor[UI_CURSOR_OLD_ROI_MODE] = gdk_cursor_new(UI_COMMON_OLD_ROI_MODE_CURSOR);
  ui_common_cursor[UI_CURSOR_OLD_ROI_RESIZE] = gdk_cursor_new(UI_COMMON_OLD_ROI_RESIZE_CURSOR);
  ui_common_cursor[UI_CURSOR_OLD_ROI_ROTATE] = gdk_cursor_new(UI_COMMON_OLD_ROI_ROTATE_CURSOR);
  ui_common_cursor[UI_CURSOR_OLD_ROI_SHIFT] = gdk_cursor_new(UI_COMMON_OLD_ROI_SHIFT_CURSOR);
  ui_common_cursor[UI_CURSOR_VOLUME_MODE] = gdk_cursor_new(UI_COMMON_VOLUME_MODE_CURSOR);
  ui_common_cursor[UI_CURSOR_WAIT] = gdk_cursor_new(UI_COMMON_WAIT_CURSOR);

  ui_common_cursors_initialized = TRUE;
  return;
}


/* replaces the current cursor with the specified cursor */
void ui_common_place_cursor(ui_common_cursor_t which_cursor, GtkWidget * widget) {

  GdkCursor * cursor;

  /* make sure we have cursors */
  if (!ui_common_cursors_initialized) ui_common_cursor_init();

  /* sanity check */
  if (!GTK_WIDGET_REALIZED(widget)) return;
  
  /* push our desired cursor onto the cursor stack */
  cursor = ui_common_cursor[which_cursor];
  ui_common_cursor_stack = g_slist_prepend(ui_common_cursor_stack,cursor);
  gdk_window_set_cursor(gtk_widget_get_parent_window(widget), cursor);

  //    g_print("push\n");

  /* do any events pending, this allows the cursor to get displayed */
  while (gtk_events_pending()) 
    gtk_main_iteration();

  return;
}

/* removes the currsor cursor, goign back to the previous cursor (or default cursor if no previous */
void ui_common_remove_cursor(GtkWidget * widget) {

  GdkCursor * cursor;

  /* sanity check */
  //if (!GTK_WIDGET_REALIZED(widget)) return;

  /* pop the previous cursor off the stack */
  cursor = g_slist_nth_data(ui_common_cursor_stack, 0);
  ui_common_cursor_stack = g_slist_remove(ui_common_cursor_stack, cursor);
  cursor = g_slist_nth_data(ui_common_cursor_stack, 0);
  gdk_window_set_cursor(gtk_widget_get_parent_window(widget), cursor);

  //    g_print("pop\tlength%d\n",g_slist_length(ui_common_cursor_stack));

  /* do any events pending, this allows the previous cursor to get displayed */
  //    while (gtk_events_pending()) 
  //      gtk_main_iteration();


  return;
}


