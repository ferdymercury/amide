/* ui_common.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2014 Andy Loening
 *
 * Author: Andy Loening <loening@alum.mit.edu>
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
#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include "amitk_point.h"

#define HELP_MENU_UI_DESCRIPTION  "<menu action='HelpMenu'> <menuitem action='HelpContents'/> <separator/> <menuitem action='HelpAbout'/> </menu>"

typedef enum {
  UI_CURSOR_DEFAULT,
  UI_CURSOR_ROI_MODE,
  UI_CURSOR_ROI_RESIZE,
  UI_CURSOR_ROI_ROTATE,
  UI_CURSOR_OBJECT_SHIFT,
  UI_CURSOR_ROI_ISOCONTOUR,
  UI_CURSOR_ROI_ERASE,
  UI_CURSOR_ROI_DRAW,
  UI_CURSOR_DATA_SET_MODE, 
  UI_CURSOR_FIDUCIAL_MARK_MODE,
  UI_CURSOR_RENDERING_ROTATE_XY,
  UI_CURSOR_RENDERING_ROTATE_Z, 
  UI_CURSOR_WAIT,
  NUM_CURSORS
} ui_common_cursor_t;

typedef enum {
  UI_COMMON_HELP_MENU_CONTENTS,
  UI_COMMON_HELP_MENU_ABOUT,
  UI_COMMON_HELP_MENU_NUM
} ui_common_help_menu_t;

/* external functions */
gboolean ui_common_check_filename(const gchar * filename);
void ui_common_set_last_path_used(const gchar * last_path_used);
gchar * ui_common_suggest_path(void);
void ui_common_entry_name_cb(gchar * entry_string, gpointer data);
void ui_common_about_cb(GtkWidget * button, gpointer data);
void ui_common_draw_view_axis(GnomeCanvas * canvas, gint row, gint column, 
			      AmitkView view, AmitkLayout layout, 
			      gint axis_width, gint axis_height);

void ui_common_update_sample_roi_item(GnomeCanvasItem * roi_item,
				      gint roi_width,
#ifdef AMIDE_LIBGNOMECANVAS_AA
				      gdouble transparency
#else
				      GdkLineStyle line_style
#endif
				      );

void ui_common_study_preferences_widgets(GtkWidget * packing_table,
					 gint table_row,
					 GtkWidget ** proi_width_spin,
					 GnomeCanvasItem ** proi_item,
#ifdef AMIDE_LIBGNOMECANVAS_AA
					 GtkWidget ** proi_transparency_spin,
#else
					 GtkWidget ** pline_style_menu,
					 GtkWidget ** fill_roi_button,
#endif
					 GtkWidget ** playout_button1,
					 GtkWidget ** playout_button2,
					 GtkWidget ** ppanel_layout_button1,
					 GtkWidget ** ppanel_layout_button2,
					 GtkWidget ** ppanel_layout_button3,
					 GtkWidget ** pmaintain_size_button,
					 GtkWidget ** ptarget_size_spin);
GtkWidget * ui_common_create_view_axis_indicator(AmitkLayout layout);
void ui_common_place_cursor_no_wait(ui_common_cursor_t which_cursor, GtkWidget * widget);
void ui_common_remove_wait_cursor(GtkWidget * widget);
void ui_common_place_cursor(ui_common_cursor_t which_cursor, GtkWidget * widget);
GtkWidget * ui_common_entry_dialog(GtkWindow * parent, gchar * prompt, gchar **return_str_ptr);
void ui_common_init_dialog_response_cb (GtkDialog * dialog, gint response_id, gpointer data);
GList * ui_common_init_dialog_selected_objects(GtkWidget * dialog);
void ui_common_toolbar_insert_widget(GtkWidget * toolbar, GtkWidget * widget, const gchar * tooltip, gint position);
void ui_common_toolbar_append_widget(GtkWidget * toolbar, GtkWidget * widget, const gchar * tooltip);
void ui_common_toolbar_append_separator(GtkWidget * toolbar);

void amide_call_help(const gchar * link_id);
void amide_register_window(gpointer * widget);
void amide_unregister_window(gpointer * widget);
void amide_unregister_all_windows(void);


/* external variables */
extern GtkActionEntry ui_common_help_menu_items[UI_COMMON_HELP_MENU_NUM];
extern GdkCursor * ui_common_cursor[NUM_CURSORS];

