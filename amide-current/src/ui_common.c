/* ui_common.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2003 Andy Loening
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

#include "amide_config.h"
#include <sys/stat.h>
#include <gnome.h>
#include "amide.h"
#include "ui_common.h"
#include "amitk_space.h"
#include "pixmaps.h"

#define AXIS_WIDTH 120
#define AXIS_HEADER 20
#define AXIS_MARGIN 10
#define ORTHOGONAL_AXIS_HEIGHT 100
#define LINEAR_AXIS_HEIGHT 140
#define AXIS_TEXT_MARGIN 10
#define AXIS_ARROW_LENGTH 8
#define AXIS_ARROW_EDGE 7
#define AXIS_ARROW_WIDTH 6

/* our help menu... */
GnomeUIInfo ui_common_help_menu[]= {
  GNOMEUIINFO_HELP(PACKAGE),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_ABOUT_ITEM(ui_common_about_cb, NULL), 
  GNOMEUIINFO_END
};


/* an array to hold the preloaded cursors */
GdkCursor * ui_common_cursor[NUM_CURSORS];

/* internal variables */
static gboolean ui_common_cursors_initialized = FALSE;
static GSList * ui_common_cursor_stack=NULL;
static GSList * ui_common_pending_cursors=NULL;




/* function which gets called from a name entry dialog */
void ui_common_entry_name_cb(gchar * entry_string, gpointer data) {
  gchar ** p_roi_name = data;
  *p_roi_name = entry_string;
  return;
}


gchar * ui_common_file_selection_get_name(GtkWidget * file_selection) {

  const gchar * save_filename;
  struct stat file_info;
  GtkWidget * question;
  gint return_val;

  save_filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_selection));

  /* sanity checks */
  if ((strcmp(save_filename, ".") == 0) ||
      (strcmp(save_filename, "..") == 0) ||
      (strcmp(save_filename, "") == 0) ||
      (strcmp(save_filename, "/") == 0)) {
    g_warning("Inappropriate filename: %s",save_filename);
    return NULL;
  }

  /* check with user if filename already exists */
  if (stat(save_filename, &file_info) == 0) {
    /* check if it's okay to writeover the file */
    question = gtk_message_dialog_new(GTK_WINDOW(file_selection),
				      GTK_DIALOG_DESTROY_WITH_PARENT,
				      GTK_MESSAGE_QUESTION,
				      GTK_BUTTONS_OK_CANCEL,
				      "Overwrite file: %s", save_filename);

    /* and wait for the question to return */
    return_val = gtk_dialog_run(GTK_DIALOG(question));

    gtk_widget_destroy(question);
    if (return_val != GTK_RESPONSE_OK) {
      return NULL; /* we don't want to overwrite the file.... */
    }
    
    /* unlinking the file doesn't occur here */
  }

  return g_strdup(save_filename);
}

/* function to close a file selection widget */
void ui_common_file_selection_cancel_cb(GtkWidget* widget, gpointer data) {

  GtkFileSelection * file_selection = data;

  gtk_widget_destroy(GTK_WIDGET(file_selection));

  return;
}

/* function which brings up an about box */
void ui_common_about_cb(GtkWidget * button, gpointer data) {

  GtkWidget *about;
  GdkPixbuf * amide_logo;

  const gchar *authors[] = {
    "Andy Loening <loening@alum.mit.edu>",
    NULL
  };

  gchar * contents;


  contents = g_strjoin("", 
		       "AMIDE's a Medical Image Data Examiner\n",
		       "\n",
		       "Email bug reports to: ", PACKAGE_BUGREPORT,"\n",
		       "\n",
#if (AMIDE_LIBECAT_SUPPORT || AMIDE_LIBGSL_SUPPORT || AMIDE_LIBMDC_SUPPORT || AMIDE_LIBVOLPACK_SUPPORT)
		       "Compiled with support for the following libraries:\n",
#endif
#ifdef AMIDE_LIBECAT_SUPPORT
		       "libecat: CTI File library by Merence Sibomona\n",
#endif
#ifdef AMIDE_LIBGSL_SUPPORT
		       "libgsl: GNU Scientific Library by the GSL Team\n",
#endif
#ifdef AMIDE_LIBMDC_SUPPORT
		       "libmdc: Medical Imaging File library by Erik Nolf\n",
#endif
#ifdef AMIDE_LIBVOLPACK_SUPPORT
		       "libvolpack: Volume Rendering library by Philippe Lacroute\n",
#endif
		       NULL);

  amide_logo = gdk_pixbuf_new_from_xpm_data(amide_logo_xpm);

  about = gnome_about_new(PACKAGE, VERSION, 
			  "Copyright (c) 2000-2003 Andy Loening",
			  contents,
			  authors, NULL, NULL, amide_logo);
  g_object_unref(amide_logo);

  gtk_window_set_modal(GTK_WINDOW(about), FALSE);

  gtk_widget_show(about);

  g_free(contents);

  return;
}



void ui_common_draw_view_axis(GnomeCanvas * canvas, gint row, gint column, 
			      AmitkView view, AmitkLayout layout, 
			      gint axis_width, gint axis_height) {


  const gchar * x_axis_label;
  gdouble x_axis_label_x_location;
  gdouble x_axis_label_y_location;
  GtkAnchorType x_axis_label_anchor;
  GnomeCanvasPoints * x_axis_line_points;

  const gchar * y_axis_label;
  gdouble y_axis_label_x_location;
  gdouble y_axis_label_y_location;
  GtkAnchorType y_axis_label_anchor;
  GnomeCanvasPoints * y_axis_line_points;

  x_axis_line_points = gnome_canvas_points_new(2);
  x_axis_line_points->coords[0] = column*axis_width + AXIS_MARGIN; /* x1 */

  y_axis_line_points = gnome_canvas_points_new(2);
  y_axis_line_points->coords[0] = column*axis_width + AXIS_MARGIN; /* x1 */

  switch(view) {
  case AMITK_VIEW_CORONAL:
    /* the x axis */
    x_axis_line_points->coords[1] = row*axis_height + AXIS_HEADER; /* y1 */
    x_axis_line_points->coords[2] = column*axis_width + axis_width-AXIS_MARGIN; /* x2 */
    x_axis_line_points->coords[3] = row*axis_height + AXIS_HEADER; /* y2 */

    /* the x label */
    x_axis_label = amitk_axis_get_name(AMITK_AXIS_X);
    x_axis_label_x_location = column*axis_width + axis_width-AXIS_MARGIN-AXIS_TEXT_MARGIN;
    x_axis_label_y_location = row*axis_height + AXIS_HEADER+AXIS_TEXT_MARGIN;
    x_axis_label_anchor = GTK_ANCHOR_NORTH_EAST;
    
    /* the z axis */
    y_axis_line_points->coords[1] = row*axis_height + AXIS_HEADER; /* y1 */
    y_axis_line_points->coords[2] = column*axis_width + AXIS_MARGIN; /* x2 */
    y_axis_line_points->coords[3] = row*axis_height + axis_height-AXIS_MARGIN; /* y2 */
    
    /* the z label */
    y_axis_label = amitk_axis_get_name(AMITK_AXIS_Z);
    y_axis_label_x_location = column*axis_width + AXIS_MARGIN+AXIS_TEXT_MARGIN;
    y_axis_label_y_location = row*axis_height + axis_height-AXIS_MARGIN-AXIS_TEXT_MARGIN;
    y_axis_label_anchor = GTK_ANCHOR_NORTH_WEST;
    break;
  case AMITK_VIEW_SAGITTAL:
    /* the y axis */
      x_axis_line_points->coords[3] = row*axis_height + AXIS_HEADER; /* y2 */
    if (layout == AMITK_LAYOUT_ORTHOGONAL) {
      x_axis_line_points->coords[1] = row*axis_height + axis_height-AXIS_MARGIN; /* y1 */
      x_axis_line_points->coords[2] = column*axis_width + AXIS_MARGIN; /* x2 */
    } else { /* AMITK_LAYOUT_LINEAR */ 
      x_axis_line_points->coords[1] = row*axis_height + AXIS_HEADER; /* y1 */
      x_axis_line_points->coords[2] = column*axis_width + axis_width-AXIS_MARGIN; /* x2 */
    }
    
    /* the y label */
    x_axis_label = amitk_axis_get_name(AMITK_AXIS_Y);
    x_axis_label_y_location = row*axis_height + AXIS_HEADER+AXIS_TEXT_MARGIN;
    if (layout == AMITK_LAYOUT_ORTHOGONAL) {
      x_axis_label_x_location = column*axis_width + AXIS_MARGIN+AXIS_TEXT_MARGIN;
      x_axis_label_anchor = GTK_ANCHOR_NORTH_WEST;
    } else {/* AMITK_LAYOUT_LINEAR */
      x_axis_label_x_location = column*axis_width + axis_width-AXIS_MARGIN-AXIS_TEXT_MARGIN;
      x_axis_label_anchor = GTK_ANCHOR_NORTH_EAST;
    }
    
    /* the z axis */
    y_axis_line_points->coords[3] = row*axis_height + axis_height-AXIS_MARGIN; /* y2 */
    if (layout == AMITK_LAYOUT_ORTHOGONAL) {
      y_axis_line_points->coords[1] = row*axis_height + axis_height-AXIS_MARGIN; /* y1 */
      y_axis_line_points->coords[2] = column*axis_width + axis_width-AXIS_MARGIN; /* x2 */
    } else { /* AMITK_LAYOUT_LINEAR */ 
      y_axis_line_points->coords[1] = row*axis_height + AXIS_HEADER; /* y1 */
      y_axis_line_points->coords[2] = column*axis_width + AXIS_MARGIN; /* x2 */
    }
    
    /* the z label */
    y_axis_label = amitk_axis_get_name(AMITK_AXIS_Z);
    y_axis_label_y_location = row*axis_height + axis_height-AXIS_MARGIN-AXIS_TEXT_MARGIN;
    if (layout == AMITK_LAYOUT_ORTHOGONAL) {
      y_axis_label_x_location = column*axis_width + axis_width-AXIS_MARGIN-AXIS_TEXT_MARGIN;
      y_axis_label_anchor = GTK_ANCHOR_SOUTH_EAST;
    } else {
      y_axis_label_x_location = column*axis_width + AXIS_MARGIN+AXIS_TEXT_MARGIN;
      y_axis_label_anchor = GTK_ANCHOR_NORTH_WEST;
    }
    break;
  case AMITK_VIEW_TRANSVERSE:
  default:
    /* the x axis */
    x_axis_line_points->coords[1] = row*axis_height + axis_height-AXIS_MARGIN; /* y1 */
    x_axis_line_points->coords[2] = column*axis_width + axis_width-AXIS_MARGIN; /* x2 */
    x_axis_line_points->coords[3] = row*axis_height + axis_height-AXIS_MARGIN; /* y2 */
  
    /* the x label */
    x_axis_label = amitk_axis_get_name(AMITK_AXIS_X);
    x_axis_label_x_location = column*axis_width + axis_width-AXIS_MARGIN-AXIS_TEXT_MARGIN;
    x_axis_label_y_location = row*axis_height + axis_height-AXIS_MARGIN-AXIS_TEXT_MARGIN;
    x_axis_label_anchor = GTK_ANCHOR_SOUTH_EAST;

    /* the y axis */
    y_axis_line_points->coords[1] = row*axis_height + axis_height-AXIS_MARGIN; /* y1 */
    y_axis_line_points->coords[2] = column*axis_width + AXIS_MARGIN; /* x2 */
    y_axis_line_points->coords[3] = row*axis_height + AXIS_HEADER; /* y2 */

    /* the y label */
    y_axis_label = amitk_axis_get_name(AMITK_AXIS_Y);
    y_axis_label_x_location = column*axis_width + AXIS_MARGIN+AXIS_TEXT_MARGIN;
    y_axis_label_y_location = row*axis_height + AXIS_HEADER+AXIS_TEXT_MARGIN;
    y_axis_label_anchor = GTK_ANCHOR_NORTH_WEST;

    break;
  }

  /* the view label */
  gnome_canvas_item_new(gnome_canvas_root(canvas), gnome_canvas_text_get_type(),
			"anchor", GTK_ANCHOR_NORTH, "text", view_names[view],
			"x", (gdouble) (column+0.5)*axis_width,
			"y", (gdouble) (row+0.5)*axis_height, 
			"fill_color", "black",
			"font", "fixed", NULL);

  /* the x axis */
  gnome_canvas_item_new(gnome_canvas_root(canvas), gnome_canvas_line_get_type(),
			"points", x_axis_line_points, "fill_color", "black",
			"width_pixels", 3, "last_arrowhead", TRUE, 
			"arrow_shape_a", (gdouble) AXIS_ARROW_LENGTH,
			"arrow_shape_b", (gdouble) AXIS_ARROW_EDGE,
			"arrow_shape_c", (gdouble) AXIS_ARROW_WIDTH,
			NULL);
  gnome_canvas_points_unref(x_axis_line_points);
    

  /* the x label */
  gnome_canvas_item_new(gnome_canvas_root(canvas), gnome_canvas_text_get_type(),
			"anchor", x_axis_label_anchor,"text", x_axis_label,
			"x", x_axis_label_x_location, "y", x_axis_label_y_location,
			"fill_color", "black","font", "fixed", NULL);

  /* the y axis */
  gnome_canvas_item_new(gnome_canvas_root(canvas), gnome_canvas_line_get_type(),
			"points", y_axis_line_points, "fill_color", "black",
			"width_pixels", 3, "last_arrowhead", TRUE, 
			"arrow_shape_a", (gdouble) AXIS_ARROW_LENGTH,
			"arrow_shape_b", (gdouble) AXIS_ARROW_EDGE,
			"arrow_shape_c", (gdouble) AXIS_ARROW_WIDTH,
			NULL);
  gnome_canvas_points_unref(x_axis_line_points);

  /* the y label */
  gnome_canvas_item_new(gnome_canvas_root(canvas),gnome_canvas_text_get_type(),
			"anchor", y_axis_label_anchor, "text", y_axis_label,
			"x", y_axis_label_x_location,"y", y_axis_label_y_location,
			"fill_color", "black", "font", "fixed", NULL);

  return;
}



GtkWidget * ui_common_create_view_axis_indicator(AmitkLayout layout) {

  GtkWidget * axis_indicator;
  AmitkView i_view;

  axis_indicator = gnome_canvas_new_aa();
  switch(layout) {
  case AMITK_LAYOUT_ORTHOGONAL:
    gtk_widget_set_size_request(axis_indicator, 2.0*AXIS_WIDTH, 2.0*ORTHOGONAL_AXIS_HEIGHT);
    gnome_canvas_set_scroll_region(GNOME_CANVAS(axis_indicator), 0.0, 0.0, 
				   2.0*AXIS_WIDTH, 2.0*ORTHOGONAL_AXIS_HEIGHT);
    ui_common_draw_view_axis(GNOME_CANVAS(axis_indicator), 0, 0, AMITK_VIEW_TRANSVERSE, 
			     layout, AXIS_WIDTH, ORTHOGONAL_AXIS_HEIGHT);
    ui_common_draw_view_axis(GNOME_CANVAS(axis_indicator), 1, 0, AMITK_VIEW_CORONAL, 
			     layout, AXIS_WIDTH, ORTHOGONAL_AXIS_HEIGHT);
    ui_common_draw_view_axis(GNOME_CANVAS(axis_indicator), 0, 1, AMITK_VIEW_SAGITTAL, 
			     layout, AXIS_WIDTH, ORTHOGONAL_AXIS_HEIGHT);

    break;
  case AMITK_LAYOUT_LINEAR:
  default:
    gtk_widget_set_size_request(axis_indicator, 3.0*AXIS_WIDTH, LINEAR_AXIS_HEIGHT);
    gnome_canvas_set_scroll_region(GNOME_CANVAS(axis_indicator), 0.0, 0.0, 
				   3.0*AXIS_WIDTH, LINEAR_AXIS_HEIGHT);
    for (i_view=0;i_view< AMITK_VIEW_NUM;i_view++)
      ui_common_draw_view_axis(GNOME_CANVAS(axis_indicator), 0, i_view, i_view,
			       layout, AXIS_WIDTH, LINEAR_AXIS_HEIGHT);
    break;
  }


  return GTK_WIDGET(axis_indicator);
}


/* load in the cursors */
static void ui_common_cursor_init(void) {

  /* load in the cursors */
  ui_common_cursor[UI_CURSOR_DEFAULT] = NULL;
  ui_common_cursor[UI_CURSOR_NEW_ROI_MODE] =  gdk_cursor_new(UI_COMMON_NEW_ROI_MODE_CURSOR);
  ui_common_cursor[UI_CURSOR_NEW_ROI_MOTION] = gdk_cursor_new(UI_COMMON_NEW_ROI_MOTION_CURSOR);
  ui_common_cursor[UI_CURSOR_OLD_ROI_MODE] = gdk_cursor_new(UI_COMMON_OLD_ROI_MODE_CURSOR);
  ui_common_cursor[UI_CURSOR_OLD_ROI_RESIZE] = gdk_cursor_new(UI_COMMON_OLD_ROI_RESIZE_CURSOR);
  ui_common_cursor[UI_CURSOR_OLD_ROI_ROTATE] = gdk_cursor_new(UI_COMMON_OLD_ROI_ROTATE_CURSOR);
  ui_common_cursor[UI_CURSOR_OLD_ROI_SHIFT] = gdk_cursor_new(UI_COMMON_OLD_ROI_SHIFT_CURSOR);
  ui_common_cursor[UI_CURSOR_OLD_ROI_ISOCONTOUR] = gdk_cursor_new(UI_COMMON_OLD_ROI_ISOCONTOUR_CURSOR);
  ui_common_cursor[UI_CURSOR_OLD_ROI_ERASE] = gdk_cursor_new(UI_COMMON_OLD_ROI_ERASE_CURSOR);
  ui_common_cursor[UI_CURSOR_DATA_SET_MODE] = gdk_cursor_new(UI_COMMON_DATA_SET_MODE_CURSOR);
  ui_common_cursor[UI_CURSOR_FIDUCIAL_MARK_MODE] = gdk_cursor_new(UI_COMMON_FIDUCIAL_MARK_MODE_CURSOR);
  ui_common_cursor[UI_CURSOR_WAIT] = gdk_cursor_new(UI_COMMON_WAIT_CURSOR);

  ui_common_cursors_initialized = TRUE;
  return;
}


/* callback to add the window's icon when the window gets realized */
void ui_common_window_realize_cb(GtkWidget * widget, gpointer data) {

  GdkPixbuf * pixbuf;

  g_return_if_fail(GTK_IS_WINDOW(widget));

  pixbuf = gdk_pixbuf_new_from_xpm_data(amide_logo_xpm);
  gtk_window_set_icon(GTK_WINDOW(widget), pixbuf);
  g_object_unref(pixbuf);

  return;
}


/* replaces the current cursor with the specified cursor */
void ui_common_place_cursor_no_wait(ui_common_cursor_t which_cursor, GtkWidget * widget) {

  GdkCursor * cursor;
  GdkCursor * current_cursor;

  /* make sure we have cursors */
  if (!ui_common_cursors_initialized) ui_common_cursor_init();

  /* sanity checks */
  if (widget == NULL) return;
  if (!GTK_WIDGET_REALIZED(widget)) return;
  
  cursor = ui_common_cursor[which_cursor];

  /* check that we don't currently have the wait cursor up */
  current_cursor = g_slist_nth_data(ui_common_cursor_stack, 0);
  if (current_cursor != ui_common_cursor[UI_CURSOR_WAIT]) {
    /* push our desired cursor onto the cursor stack */
    ui_common_cursor_stack = g_slist_prepend(ui_common_cursor_stack,cursor);
    gdk_window_set_cursor(gtk_widget_get_parent_window(widget), cursor);
  } else {
    /* if we're waiting for something, save this cursor for later */
    ui_common_pending_cursors = g_slist_append(ui_common_pending_cursors,cursor);
  }

  return;
}

/* replaces the current cursor with the specified cursor */
void ui_common_place_cursor(ui_common_cursor_t which_cursor, GtkWidget * widget) {

  /* call the actual function */
  ui_common_place_cursor_no_wait(which_cursor, widget);

  /* do any events pending, this allows the cursor to get displayed */
  while (gtk_events_pending()) gtk_main_iteration();

  return;
}

/* removes the cursor, going back to the previous cursor (or default cursor if no previous */
void ui_common_remove_cursor(GtkWidget * widget) {

  GdkCursor * cursor;

  /* sanity check */
  if (widget == NULL) return;
  if (!GTK_WIDGET_REALIZED(widget)) return;

  /* pop the previous cursor off the stack */
  cursor = g_slist_nth_data(ui_common_cursor_stack, 0);
  ui_common_cursor_stack = g_slist_remove(ui_common_cursor_stack, cursor);
  
  /* if we have any pending cursors, add them to the stack */
  while ((cursor = g_slist_nth_data(ui_common_pending_cursors, 0)) != NULL) {
    ui_common_cursor_stack = g_slist_prepend(ui_common_cursor_stack,cursor);
    ui_common_pending_cursors = g_slist_remove(ui_common_pending_cursors, cursor);
  }

  cursor = g_slist_nth_data(ui_common_cursor_stack, 0);
  gdk_window_set_cursor(gtk_widget_get_parent_window(widget), cursor);

  return;
}





