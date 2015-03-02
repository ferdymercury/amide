/* ui_common.c
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

#include "amide_config.h"
#include <gnome.h>
#include "amide.h"
#include "ui_common.h"
#include "../pixmaps/icon_amide.xpm"

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
    "Andy Loening <loening@ucla.edu>",
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

  amide_logo = gdk_pixbuf_new_from_xpm_data((const char **) icon_amide_xpm);

  about = gnome_about_new(PACKAGE, VERSION, 
			  "Copyright (c) 2000-2002 Andy Loening",
			  contents,
			  authors, NULL, NULL, amide_logo);
  g_object_unref(amide_logo);

  gtk_window_set_modal(GTK_WINDOW(about), FALSE);

  gtk_widget_show(about);

  g_free(contents);

  return;
}



GtkWidget * ui_common_create_view_axis_indicator(AmitkLayout layout) {

  GnomeCanvas * axis_indicator;
  GnomeCanvasPoints * axis_line_points;
  gint column[AMITK_VIEW_NUM];
  gint row[AMITK_VIEW_NUM];
  AmitkView i_view;
  gint axis_height;

  axis_indicator = GNOME_CANVAS(gnome_canvas_new_aa());
  switch(layout) {
  case AMITK_LAYOUT_ORTHOGONAL:
    axis_height = ORTHOGONAL_AXIS_HEIGHT;
    gtk_widget_set_size_request(GTK_WIDGET(axis_indicator), 2.0*AXIS_WIDTH, 2.0*axis_height);
    gnome_canvas_set_scroll_region(axis_indicator, 0.0, 0.0, 2.0*AXIS_WIDTH, 2.0*axis_height);
    column[AMITK_VIEW_TRANSVERSE] = 0;
    column[AMITK_VIEW_CORONAL] = 0;
    column[AMITK_VIEW_SAGITTAL] = 1;
    row[AMITK_VIEW_TRANSVERSE] = 0;
    row[AMITK_VIEW_CORONAL] = 1;
    row[AMITK_VIEW_SAGITTAL] = 0;
    break;
  case AMITK_LAYOUT_LINEAR:
  default:
    axis_height = LINEAR_AXIS_HEIGHT;
    gtk_widget_set_size_request(GTK_WIDGET(axis_indicator), 3.0*AXIS_WIDTH, axis_height);
    gnome_canvas_set_scroll_region(axis_indicator, 0.0, 0.0, 3.0*AXIS_WIDTH, axis_height);
    for (i_view=0;i_view< AMITK_VIEW_NUM;i_view++) {
      column[i_view] = i_view;
      row[i_view] = 0;
    }
    break;
  }

  /*----------- transverse ------------*/
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_text_get_type(),
			"anchor", GTK_ANCHOR_NORTH, "text", view_names[AMITK_VIEW_TRANSVERSE],
			"x", (gdouble) (column[AMITK_VIEW_TRANSVERSE]+0.5)*AXIS_WIDTH,
			"y", (gdouble) (row[AMITK_VIEW_TRANSVERSE]+0.5)*axis_height, 
			"fill_color", "black",
			"font", "fixed", NULL);

  /* the x axis */
  axis_line_points = gnome_canvas_points_new(2);
  axis_line_points->coords[0] = column[AMITK_VIEW_TRANSVERSE]*AXIS_WIDTH + AXIS_MARGIN; /* x1 */
  axis_line_points->coords[1] = row[AMITK_VIEW_TRANSVERSE]*axis_height + axis_height-AXIS_MARGIN; /* y1 */
  axis_line_points->coords[2] = column[AMITK_VIEW_TRANSVERSE]*AXIS_WIDTH + AXIS_WIDTH-AXIS_MARGIN; /* x2 */
  axis_line_points->coords[3] = row[AMITK_VIEW_TRANSVERSE]*axis_height + axis_height-AXIS_MARGIN; /* y2 */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_line_get_type(),
			"points", axis_line_points, "fill_color", "black",
			"width_pixels", 3, "last_arrowhead", TRUE, 
			"arrow_shape_a", (gdouble) AXIS_ARROW_LENGTH,
			"arrow_shape_b", (gdouble) AXIS_ARROW_EDGE,
			"arrow_shape_c", (gdouble) AXIS_ARROW_WIDTH,
			NULL);
  gnome_canvas_points_unref(axis_line_points);

  /* the x label */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_text_get_type(),
			"anchor", GTK_ANCHOR_SOUTH_EAST,"text", "x",
			"x", (gdouble) column[AMITK_VIEW_TRANSVERSE]*AXIS_WIDTH + AXIS_WIDTH-AXIS_MARGIN-AXIS_TEXT_MARGIN,
			"y", (gdouble) row[AMITK_VIEW_TRANSVERSE]*axis_height + axis_height-AXIS_MARGIN-AXIS_TEXT_MARGIN,
			"fill_color", "black",
			"font", "fixed", NULL);

  /* the y axis */
  axis_line_points = gnome_canvas_points_new(2);
  axis_line_points->coords[0] = column[AMITK_VIEW_TRANSVERSE]*AXIS_WIDTH + AXIS_MARGIN; /* x1 */
  axis_line_points->coords[1] = row[AMITK_VIEW_TRANSVERSE]*axis_height + axis_height-AXIS_MARGIN; /* y1 */
  axis_line_points->coords[2] = column[AMITK_VIEW_TRANSVERSE]*AXIS_WIDTH + AXIS_MARGIN; /* x2 */
  axis_line_points->coords[3] = row[AMITK_VIEW_TRANSVERSE]*axis_height + AXIS_HEADER; /* y2 */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_line_get_type(),
			"points", axis_line_points, "fill_color", "black",
			"width_pixels", 3, "last_arrowhead", TRUE, 
			"arrow_shape_a", (gdouble) AXIS_ARROW_LENGTH,
			"arrow_shape_b", (gdouble) AXIS_ARROW_EDGE,
			"arrow_shape_c", (gdouble) AXIS_ARROW_WIDTH,
			NULL);
  gnome_canvas_points_unref(axis_line_points);

  /* the y label */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator),gnome_canvas_text_get_type(),
			"anchor", GTK_ANCHOR_NORTH_WEST, "text", "y",
			"x", (gdouble) column[AMITK_VIEW_TRANSVERSE]*AXIS_WIDTH + AXIS_MARGIN+AXIS_TEXT_MARGIN,
			"y", (gdouble) row[AMITK_VIEW_TRANSVERSE]*axis_height + AXIS_HEADER+AXIS_TEXT_MARGIN,
			"fill_color", "black",
			"font", "fixed", NULL);


  /*----------- coronal ------------*/
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_text_get_type(),
			"anchor", GTK_ANCHOR_NORTH, "text", view_names[AMITK_VIEW_CORONAL],
			"x", (gdouble) (column[AMITK_VIEW_CORONAL]+0.5)*AXIS_WIDTH,
			"y", (gdouble) (row[AMITK_VIEW_CORONAL]+0.5)*axis_height,
			"fill_color", "black",
			"font", "fixed", NULL);

  /* the x axis */
  axis_line_points = gnome_canvas_points_new(2);
  axis_line_points->coords[0] = column[AMITK_VIEW_CORONAL]*AXIS_WIDTH + AXIS_MARGIN; /* x1 */
  axis_line_points->coords[1] = row[AMITK_VIEW_CORONAL]*axis_height + AXIS_HEADER; /* y1 */
  axis_line_points->coords[2] = column[AMITK_VIEW_CORONAL]*AXIS_WIDTH + AXIS_WIDTH-AXIS_MARGIN; /* x2 */
  axis_line_points->coords[3] = row[AMITK_VIEW_CORONAL]*axis_height + AXIS_HEADER; /* y2 */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_line_get_type(),
			"points", axis_line_points, "fill_color", "black",
			"width_pixels", 3, "last_arrowhead", TRUE, 
			"arrow_shape_a", (gdouble) AXIS_ARROW_LENGTH,
			"arrow_shape_b", (gdouble) AXIS_ARROW_EDGE,
			"arrow_shape_c", (gdouble) AXIS_ARROW_WIDTH,
			NULL);
  gnome_canvas_points_unref(axis_line_points);

  /* the x label */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_text_get_type(),
			"anchor", GTK_ANCHOR_NORTH_EAST, "text", "x",
			"x", (gdouble) column[AMITK_VIEW_CORONAL]*AXIS_WIDTH + AXIS_WIDTH-AXIS_MARGIN-AXIS_TEXT_MARGIN,
			"y", (gdouble) row[AMITK_VIEW_CORONAL]*axis_height + AXIS_HEADER+AXIS_TEXT_MARGIN,
			"fill_color", "black",
			"font", "fixed", NULL);

  /* the z axis */
  axis_line_points = gnome_canvas_points_new(2);
  axis_line_points->coords[0] = column[AMITK_VIEW_CORONAL]*AXIS_WIDTH + AXIS_MARGIN; /* x1 */
  axis_line_points->coords[1] = row[AMITK_VIEW_CORONAL]*axis_height + AXIS_HEADER; /* y1 */
  axis_line_points->coords[2] = column[AMITK_VIEW_CORONAL]*AXIS_WIDTH + AXIS_MARGIN; /* x2 */
  axis_line_points->coords[3] = row[AMITK_VIEW_CORONAL]*axis_height + axis_height-AXIS_MARGIN; /* y2 */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_line_get_type(),
			"points", axis_line_points, "fill_color", "black",
			"width_pixels", 3, "last_arrowhead", TRUE, 
			"arrow_shape_a", (gdouble) AXIS_ARROW_LENGTH,
			"arrow_shape_b", (gdouble) AXIS_ARROW_EDGE,
			"arrow_shape_c", (gdouble) AXIS_ARROW_WIDTH,
			NULL);
  gnome_canvas_points_unref(axis_line_points);

  /* the z label */
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_text_get_type(),
			"anchor", GTK_ANCHOR_NORTH_WEST, "text", "z",
			"x", (gdouble) column[AMITK_VIEW_CORONAL]*AXIS_WIDTH + AXIS_MARGIN+AXIS_TEXT_MARGIN,
			"y", (gdouble) row[AMITK_VIEW_CORONAL]*axis_height + axis_height-AXIS_MARGIN-AXIS_TEXT_MARGIN,
			"fill_color", "black",
			"font", "fixed", NULL);

  /*------------ sagittal --------------*/
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_text_get_type(),
			"anchor", GTK_ANCHOR_NORTH, "text", view_names[AMITK_VIEW_SAGITTAL],
			"x", (gdouble) (column[AMITK_VIEW_SAGITTAL]+0.5)*AXIS_WIDTH,
			"y", (gdouble) (row[AMITK_VIEW_SAGITTAL]+0.5)*axis_height,
			"fill_color", "black",
			"font", "fixed", NULL);

  /* the y axis */
  axis_line_points = gnome_canvas_points_new(2);
  axis_line_points->coords[0] = column[AMITK_VIEW_SAGITTAL]*AXIS_WIDTH + AXIS_MARGIN; /* x1 */
  axis_line_points->coords[3] = row[AMITK_VIEW_SAGITTAL]*axis_height + AXIS_HEADER; /* y2 */
  if (layout == AMITK_LAYOUT_ORTHOGONAL) {
    axis_line_points->coords[1] = row[AMITK_VIEW_SAGITTAL]*axis_height + axis_height-AXIS_MARGIN; /* y1 */
    axis_line_points->coords[2] = column[AMITK_VIEW_SAGITTAL]*AXIS_WIDTH + AXIS_MARGIN; /* x2 */
  } else { /* AMITK_LAYOUT_LINEAR */ 
    axis_line_points->coords[1] = row[AMITK_VIEW_SAGITTAL]*axis_height + AXIS_HEADER; /* y1 */
    axis_line_points->coords[2] = column[AMITK_VIEW_SAGITTAL]*AXIS_WIDTH + AXIS_WIDTH-AXIS_MARGIN; /* x2 */
  }
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_line_get_type(),
			"points", axis_line_points, "fill_color", "black",
			"width_pixels", 3, "last_arrowhead", TRUE, 
			"arrow_shape_a", (gdouble) AXIS_ARROW_LENGTH,
			"arrow_shape_b", (gdouble) AXIS_ARROW_EDGE,
			"arrow_shape_c", (gdouble) AXIS_ARROW_WIDTH,
			NULL);
  gnome_canvas_points_unref(axis_line_points);

  /* the y label */
  if (layout == AMITK_LAYOUT_ORTHOGONAL) 
    gnome_canvas_item_new(gnome_canvas_root(axis_indicator),gnome_canvas_text_get_type(),
			  "anchor", GTK_ANCHOR_NORTH_WEST, "text", "y",
			  "x", (gdouble) column[AMITK_VIEW_SAGITTAL]*AXIS_WIDTH + AXIS_MARGIN+AXIS_TEXT_MARGIN,
			  "y", (gdouble) row[AMITK_VIEW_SAGITTAL]*axis_height + AXIS_HEADER+AXIS_TEXT_MARGIN,
			  "fill_color", "black",
			  "font", "fixed", NULL);
  else /* AMITK_LAYOUT_LINEAR */
    gnome_canvas_item_new(gnome_canvas_root(axis_indicator),gnome_canvas_text_get_type(),
			  "anchor", GTK_ANCHOR_NORTH_EAST, "text", "y",
			  "x", (gdouble) column[AMITK_VIEW_SAGITTAL]*AXIS_WIDTH + AXIS_WIDTH-AXIS_MARGIN-AXIS_TEXT_MARGIN,
			  "y", (gdouble) row[AMITK_VIEW_SAGITTAL]*axis_height + AXIS_HEADER+AXIS_TEXT_MARGIN,
			  "fill_color", "black",
			  "font", "fixed", NULL);

  /* the z axis */
  axis_line_points = gnome_canvas_points_new(2);
  axis_line_points->coords[0] = column[AMITK_VIEW_SAGITTAL]*AXIS_WIDTH + AXIS_MARGIN; /* x1 */
  axis_line_points->coords[3] = row[AMITK_VIEW_SAGITTAL]*axis_height + axis_height-AXIS_MARGIN; /* y2 */
  if (layout == AMITK_LAYOUT_ORTHOGONAL) {
    axis_line_points->coords[1] = row[AMITK_VIEW_SAGITTAL]*axis_height + axis_height-AXIS_MARGIN; /* y1 */
    axis_line_points->coords[2] = column[AMITK_VIEW_SAGITTAL]*AXIS_WIDTH + AXIS_WIDTH-AXIS_MARGIN; /* x2 */
  } else { /* AMITK_LAYOUT_LINEAR */ 
    axis_line_points->coords[1] = row[AMITK_VIEW_SAGITTAL]*axis_height + AXIS_HEADER; /* y1 */
    axis_line_points->coords[2] = column[AMITK_VIEW_SAGITTAL]*AXIS_WIDTH + AXIS_MARGIN; /* x2 */
  }
  gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_line_get_type(),
			"points", axis_line_points, "fill_color", "black",
			"width_pixels", 3, "last_arrowhead", TRUE, 
			"arrow_shape_a", (gdouble) AXIS_ARROW_LENGTH,
			"arrow_shape_b", (gdouble) AXIS_ARROW_EDGE,
			"arrow_shape_c", (gdouble) AXIS_ARROW_WIDTH,
			NULL);
  gnome_canvas_points_unref(axis_line_points);

  /* the z label */
  if (layout == AMITK_LAYOUT_ORTHOGONAL) 
    gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_text_get_type(),
			  "anchor", GTK_ANCHOR_SOUTH_EAST, "text", "z",
			  "x", (gdouble) column[AMITK_VIEW_SAGITTAL]*AXIS_WIDTH + AXIS_WIDTH-AXIS_MARGIN-AXIS_TEXT_MARGIN,
			  "y", (gdouble) row[AMITK_VIEW_SAGITTAL]*axis_height + axis_height-AXIS_MARGIN-AXIS_TEXT_MARGIN,
			  "fill_color", "black",
			  "font", "fixed", NULL);
  else  /* AMITK_LAYOUT_LINEAR */ 
    gnome_canvas_item_new(gnome_canvas_root(axis_indicator), gnome_canvas_text_get_type(),
			  "anchor", GTK_ANCHOR_NORTH_WEST, "text", "z",
			  "x", (gdouble) column[AMITK_VIEW_SAGITTAL]*AXIS_WIDTH + AXIS_MARGIN+AXIS_TEXT_MARGIN,
			  "y", (gdouble) row[AMITK_VIEW_SAGITTAL]*axis_height + axis_height-AXIS_MARGIN-AXIS_TEXT_MARGIN,
			  "fill_color", "black",
			  "font", "fixed", NULL);

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
  GdkPixmap *pm;
  GdkBitmap *bm;

  g_return_if_fail(widget != NULL);

  /* setup the threshold's icon */
  pm = gdk_pixmap_create_from_xpm_d(widget->window, &bm,NULL,icon_amide_xpm),
  gdk_window_set_icon(widget->window, NULL, pm, bm);

  return;
}


/* replaces the current cursor with the specified cursor */
void ui_common_place_cursor(ui_common_cursor_t which_cursor, GtkWidget * widget) {

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


