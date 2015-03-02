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
#include <string.h>
#include "amide.h"
#include "ui_common.h"
#include "amitk_space.h"
#include "pixmaps.h"
#ifdef AMIDE_LIBGSL_SUPPORT
#include <gsl/gsl_version.h>
#endif
#ifdef AMIDE_LIBVOLPACK_SUPPORT
#include <volpack.h>
#endif
//#ifdef AMIDE_LIBMDC_SUPPORT
//#include <medcon.h>
//#endif
#ifdef AMIDE_LIBFAME_SUPPORT
#include <fame_version.h>
#endif

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
static GList * ui_common_cursor_stack=NULL;

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
      (strcmp(save_filename, "\\") == 0) ||
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
#if (AMIDE_LIBECAT_SUPPORT || AMIDE_LIBGSL_SUPPORT || AMIDE_LIBMDC_SUPPORT || AMIDE_LIBVOLPACK_SUPPORT || AMIDE_LIBFAME_SUPPORT)
		       "Compiled with support for the following libraries:\n",
#endif
#ifdef AMIDE_LIBECAT_SUPPORT
		       "libecat: CTI File library by Merence Sibomona\n",
#endif
#ifdef AMIDE_LIBGSL_SUPPORT
		       "libgsl: GNU Scientific Library by the GSL Team (version ",GSL_VERSION,")\n",
#endif
#ifdef AMIDE_LIBMDC_SUPPORT
		       //		       "libmdc: Medical Imaging File library by Erik Nolf (version ",MDC_VERSION,")\n",
		       "libmdc: Medical Imaging File library by Erik Nolf\n",
#endif
#ifdef AMIDE_LIBVOLPACK_SUPPORT
		       "libvolpack: Volume Rendering library by Philippe Lacroute (version ",VP_VERSION,")\n",
#endif
#ifdef AMIDE_LIBFAME_SUPPORT
		       "libfame: Fast Assembly Mpeg Encoding library by the FAME Team (version ", LIBFAME_VERSION, ")\n",
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

#ifndef AMIDE_OSX_HACKS 

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
			"font_desc", amitk_fixed_font_desc, NULL);

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
			"fill_color", "black", "font_desc", amitk_fixed_font_desc, NULL); 

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
			"fill_color", "black", "font_desc", amitk_fixed_font_desc, NULL); 

#endif

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



/* This data is in X bitmap format, and can be created with the 'bitmap' utility. */
#define small_dot_width 3
#define small_dot_height 3
static unsigned char small_dot_bits[] = {0x00, 0x02, 0x00};
 

/* load in the cursors */
static void ui_common_cursor_init(void) {

  GdkPixmap *source, *mask;
  GdkColor fg = { 0, 0, 0, 0 }; /* black. */
  GdkColor bg = { 0, 0, 0, 0 }; /* black */
  GdkCursor * small_dot;
 
  source = gdk_bitmap_create_from_data(NULL, small_dot_bits, small_dot_width, small_dot_height);
  mask = gdk_bitmap_create_from_data(NULL, small_dot_bits, small_dot_width, small_dot_height);
  small_dot = gdk_cursor_new_from_pixmap (source, mask, &fg, &bg, 2,2);
  gdk_pixmap_unref (source);
  gdk_pixmap_unref (mask);
                                     
  /* load in the cursors */

  ui_common_cursor[UI_CURSOR_DEFAULT] = NULL;
  ui_common_cursor[UI_CURSOR_ROI_MODE] =  gdk_cursor_new(UI_COMMON_ROI_MODE_CURSOR);
  ui_common_cursor[UI_CURSOR_ROI_RESIZE] = small_dot;
  ui_common_cursor[UI_CURSOR_ROI_ROTATE] = small_dot;
  ui_common_cursor[UI_CURSOR_OBJECT_SHIFT] = small_dot;
  ui_common_cursor[UI_CURSOR_ROI_ISOCONTOUR] = gdk_cursor_new(UI_COMMON_ROI_ISOCONTOUR_CURSOR);
  ui_common_cursor[UI_CURSOR_ROI_ERASE] = gdk_cursor_new(UI_COMMON_ROI_ERASE_CURSOR);
  ui_common_cursor[UI_CURSOR_DATA_SET_MODE] = gdk_cursor_new(UI_COMMON_DATA_SET_MODE_CURSOR);
  ui_common_cursor[UI_CURSOR_FIDUCIAL_MARK_MODE] = gdk_cursor_new(UI_COMMON_FIDUCIAL_MARK_MODE_CURSOR);
  ui_common_cursor[UI_CURSOR_RENDERING_ROTATE_XY] = gdk_cursor_new(UI_COMMON_SHIFT_CURSOR);
  ui_common_cursor[UI_CURSOR_RENDERING_ROTATE_Z] = gdk_cursor_new(UI_COMMON_ROTATE_CURSOR);
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

  GList * cursors;
  gboolean reached_spot;
  GdkCursor * cursor;

  /* make sure we have cursors */
  if (!ui_common_cursors_initialized) ui_common_cursor_init();

  /* sanity checks */
  if (widget == NULL) return;
  if (!GTK_WIDGET_REALIZED(widget)) return;
  
  cursor = ui_common_cursor[which_cursor];

  cursors = ui_common_cursor_stack;
  /* if the requested cursor isn't the wait cursor, push req. cursor behind the wait cursors */
  if (which_cursor != UI_CURSOR_WAIT) {
    reached_spot = FALSE;
    while (!reached_spot) {
      if (cursors == NULL)
	reached_spot = TRUE;
      else if (cursors->data != ui_common_cursor[UI_CURSOR_WAIT])
	reached_spot = TRUE;
      else
	cursors = cursors->next;
    }
  }

  /* put the cursor on the stack */
  ui_common_cursor_stack = g_list_insert_before(ui_common_cursor_stack,  cursors, cursor);
  gdk_window_set_cursor(gtk_widget_get_parent_window(widget), cursor);

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
void ui_common_remove_cursor(ui_common_cursor_t which_cursor, GtkWidget * widget) {

  GdkCursor * cursor;

  /* sanity check */
  if (widget == NULL) return;
  if (!GTK_WIDGET_REALIZED(widget)) return;

  /* pop the previous cursor off the stack */
  cursor = ui_common_cursor[which_cursor];
  ui_common_cursor_stack = g_list_remove(ui_common_cursor_stack, cursor);

  /* use the next cursor on the stack */
  cursor =  g_list_nth_data(ui_common_cursor_stack, 0);
  gdk_window_set_cursor(gtk_widget_get_parent_window(widget), cursor);

  return;
}




static void entry_activate(GtkEntry * entry, gpointer data) {

  GtkWidget * dialog = data;
  gchar ** return_str_ptr;

  return_str_ptr = g_object_get_data(G_OBJECT(dialog), "return_str_ptr");
  if(*return_str_ptr != NULL) g_free(*return_str_ptr);
  *return_str_ptr= g_strdup(gtk_entry_get_text(entry));

  gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK,
				    strlen(*return_str_ptr));
  return;
}

static void init_response_cb (GtkDialog * dialog, gint response_id, gpointer data) {
  
  gint return_val;

  switch(response_id) {
  case GTK_RESPONSE_OK:
  case GTK_RESPONSE_CLOSE:
    g_signal_emit_by_name(G_OBJECT(dialog), "delete_event", NULL, &return_val);
    if (!return_val) gtk_widget_destroy(GTK_WIDGET(dialog));
    break;

  default:
    break;
  }

  return;
}


/* function to setup a dialog to allow us to choice options for rendering */
GtkWidget * ui_common_entry_dialog(GtkWindow * parent, gchar * prompt, gchar **return_str_ptr) {
  
  GtkWidget * dialog;
  GtkWidget * table;
  guint table_row;
  GtkWidget * entry;
  GtkWidget * label;
  GtkWidget * image;
  

  dialog = gtk_dialog_new_with_buttons ("Request Dialog",  parent,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CLOSE, 
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					NULL);
  g_object_set_data(G_OBJECT(dialog), "return_str_ptr", return_str_ptr);
  g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(init_response_cb), NULL);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);
  gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK,FALSE);


  table = gtk_table_new(3,2,FALSE);
  table_row=0;
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);


  image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION,GTK_ICON_SIZE_DIALOG);
  gtk_table_attach(GTK_TABLE(table), image, 
		   0,1, table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);

  label = gtk_label_new(prompt);
  gtk_table_attach(GTK_TABLE(table), label, 
		   1,2, table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);
  table_row++;


  entry = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), entry, 
		   1,2, table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(entry_activate), dialog);
  table_row++;


  gtk_widget_show_all(dialog);

  return dialog;
}





