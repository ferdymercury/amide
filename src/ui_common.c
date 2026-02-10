/* ui_common.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2017 Andy Loening
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
#include <string.h>
#include "amide.h"
#include "amide_gnome.h"
#include "ui_common.h"
#include "amitk_space.h"
#include "amitk_color_table.h"
#include "amitk_preferences.h"
#include "amitk_threshold.h"
#include "amitk_tree_view.h"
#ifdef AMIDE_LIBGSL_SUPPORT
#include <gsl/gsl_version.h>
#endif
#ifdef AMIDE_LIBVOLPACK_SUPPORT
#include <volpack.h>
#endif
#ifdef AMIDE_LIBMDC_SUPPORT
#include <medcon.h>
#endif
#ifdef AMIDE_FFMPEG_SUPPORT
#include <libavcodec/avcodec.h>
#endif
#ifdef AMIDE_LIBFAME_SUPPORT
#include <fame_version.h>
#endif
#ifdef AMIDE_LIBDCMDATA_SUPPORT
#include <dcmtk_interface.h>
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

static void manual_cb(GSimpleAction *action, GVariant *param, gpointer data);
static void about_cb(GSimpleAction *action, GVariant *param, gpointer data);

/* our help menu... */
GActionEntry ui_common_help_menu_items[UI_COMMON_HELP_MENU_NUM] = {
  { "manual", manual_cb },
  { "about", about_cb }
};


/* an array to hold the preloaded cursors */
GdkCursor * ui_common_cursor[NUM_CURSORS];

/* internal variables */
static gboolean ui_common_cursors_initialized = FALSE;
static gchar * last_path_used=NULL;
static ui_common_cursor_t current_cursor;





static void manual_cb(GSimpleAction * action, GVariant * param, gpointer data) {
  amide_call_help(NULL);
}


/* the about dialog */
static void about_cb(GSimpleAction * action, GVariant * param, gpointer data) {

  const gchar *authors[] = {
    "Andreas Loening <loening@alum.mit.edu>",
    NULL
  };

  const gchar *translators = {
    "Spanish Manual Translation: Pablo Sau <psau@cadpet.es>\n"
    "Chinese (Simplified) Interface Translation: wormwang@holdfastgroup.com\n"
    "Chinese (Traditional) Interface Translation: William Chao <william.chao@ossii.com.tw>"
  };

  gchar * comments;
  comments = g_strjoin("", 
		       _("AMIDE's a Medical Image Data Examiner\n"),
		       "\n",
		       _("Email bug reports to: "), PACKAGE_BUGREPORT,"\n",
		       "\n",
#if (AMIDE_LIBECAT_SUPPORT || AMIDE_LIBGSL_SUPPORT || AMIDE_LIBMDC_SUPPORT || AMIDE_LIBDCMDATA_SUPPORT || AMIDE_LIBVOLPACK_SUPPORT || AMIDE_FFMPEG_SUPPORT || AMIDE_LIBFAME_SUPPORT)
		       _("Compiled with support for the following libraries:\n"),
#endif
#ifdef AMIDE_LIBECAT_SUPPORT
		       _("libecat: CTI File library by Merence Sibomona\n"),
#endif
#ifdef AMIDE_LIBGSL_SUPPORT
		       _("libgsl: GNU Scientific Library by the GSL Team (version "),GSL_VERSION,")\n",
#endif
#ifdef AMIDE_LIBMDC_SUPPORT
		       _("libmdc: Medical Imaging File library by Erik Nolf (version "),MDC_VERSION,")\n",
#endif
#ifdef AMIDE_LIBDCMDATA_SUPPORT
		       _("libdcmdata: OFFIS DICOM Toolkit DCMTK (C) OFFIS e.V. (version "),dcmtk_version,")\n",
#endif
#ifdef AMIDE_LIBVOLPACK_SUPPORT
		       _("libvolpack: Volume Rendering library by Philippe Lacroute (version "),VP_VERSION,")\n",
#endif
#ifdef AMIDE_FFMPEG_SUPPORT
		       _("libavcodec: media encoding library by the FFMPEG Team (version "),AV_STRINGIFY(LIBAVCODEC_VERSION), ")\n",
#endif
#ifdef AMIDE_LIBFAME_SUPPORT
		       _("libfame: Fast Assembly Mpeg Encoding library by the FAME Team (version "), LIBFAME_VERSION, ")\n",
#endif
		       NULL);

  gtk_show_about_dialog(NULL, 
			"name", PACKAGE,
			"version", VERSION,
			"copyright", "Copyright (c) 2000-2017 Andreas Loening",
			"license", "GNU General Public License, Version 2",
			"authors", authors,
			"comments", comments,
			/* "documenters", documenters, */
			/* "artists", artists, */
			/* "logo", uses default window icon we've already set*/
			"translator-credits", translators,
			/* "translator-credits, _("translator-credits"), */ /* this would mark the string for translation by the translator*/
			"website", "http://amide.sourceforge.net",
			NULL);

  g_free(comments);

  return;
}


/* returns TRUE for OK */
gboolean ui_common_check_filename(const gchar * filename) {

  if ((strcmp(filename, ".") == 0) ||
      (strcmp(filename, "..") == 0) ||
      (strcmp(filename, "") == 0) ||
      (strcmp(filename, "\\") == 0) ||
      (strcmp(filename, "/") == 0)) {
    return FALSE;
  } else
    return TRUE;
}


void ui_common_set_last_path_used(const gchar * path) {

  if (last_path_used != NULL) g_free(last_path_used);
  last_path_used = g_strdup(path);

  return;
}


/* Returns a suggested path to save a file in. Returned path needs to be free'd */
gchar * ui_common_suggest_path(void) {

  gchar * dir_string;

  if (last_path_used != NULL)
    dir_string = g_path_get_dirname(last_path_used);
  else
    dir_string = g_strdup(".");;

  return dir_string;

}

/* function which brings up an about box */
void ui_common_about_cb(GtkWidget * button, gpointer data) {
  about_cb(NULL, NULL, NULL);
}

void ui_common_draw_view_axis(AmitkSimpleCanvas * canvas, gint row, gint column,
			      AmitkView view, AmitkLayout layout, 
			      gint axis_width, gint axis_height) {

  const gchar * x_axis_label;
  gdouble x_axis_label_x_location;
  gdouble x_axis_label_y_location;
  AmitkCanvasAnchorType x_axis_label_anchor;
  AmitkCanvasPoints * x_axis_line_points;

  const gchar * y_axis_label;
  gdouble y_axis_label_x_location;
  gdouble y_axis_label_y_location;
  AmitkCanvasAnchorType y_axis_label_anchor;
  AmitkCanvasPoints * y_axis_line_points;
  AmitkCanvasItem * root;

  x_axis_line_points = amitk_canvas_points_new(2);
  x_axis_line_points->coords[0] = column*axis_width + AXIS_MARGIN; /* x1 */

  y_axis_line_points = amitk_canvas_points_new(2);
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
    x_axis_label_anchor = AMITK_CANVAS_ANCHOR_NORTH_EAST;
    
    /* the z axis */
    y_axis_line_points->coords[1] = row*axis_height + AXIS_HEADER; /* y1 */
    y_axis_line_points->coords[2] = column*axis_width + AXIS_MARGIN; /* x2 */
    y_axis_line_points->coords[3] = row*axis_height + axis_height-AXIS_MARGIN; /* y2 */
    
    /* the z label */
    y_axis_label = amitk_axis_get_name(AMITK_AXIS_Z);
    y_axis_label_x_location = column*axis_width + AXIS_MARGIN+AXIS_TEXT_MARGIN;
    y_axis_label_y_location = row*axis_height + axis_height-AXIS_MARGIN-AXIS_TEXT_MARGIN;
    y_axis_label_anchor = AMITK_CANVAS_ANCHOR_NORTH_WEST;
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
      x_axis_label_anchor = AMITK_CANVAS_ANCHOR_NORTH_WEST;
    } else {/* AMITK_LAYOUT_LINEAR */
      x_axis_label_x_location = column*axis_width + axis_width-AXIS_MARGIN-AXIS_TEXT_MARGIN;
      x_axis_label_anchor = AMITK_CANVAS_ANCHOR_NORTH_EAST;
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
      y_axis_label_anchor = AMITK_CANVAS_ANCHOR_SOUTH_EAST;
    } else {
      y_axis_label_x_location = column*axis_width + AXIS_MARGIN+AXIS_TEXT_MARGIN;
      y_axis_label_anchor = AMITK_CANVAS_ANCHOR_NORTH_WEST;
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
    x_axis_label_anchor = AMITK_CANVAS_ANCHOR_SOUTH_EAST;

    /* the y axis */
    y_axis_line_points->coords[1] = row*axis_height + axis_height-AXIS_MARGIN; /* y1 */
    y_axis_line_points->coords[2] = column*axis_width + AXIS_MARGIN; /* x2 */
    y_axis_line_points->coords[3] = row*axis_height + AXIS_HEADER; /* y2 */

    /* the y label */
    y_axis_label = amitk_axis_get_name(AMITK_AXIS_Y);
    y_axis_label_x_location = column*axis_width + AXIS_MARGIN+AXIS_TEXT_MARGIN;
    y_axis_label_y_location = row*axis_height + AXIS_HEADER+AXIS_TEXT_MARGIN;
    y_axis_label_anchor = AMITK_CANVAS_ANCHOR_NORTH_WEST;

    break;
  }

  /* the view label */
  root = amitk_simple_canvas_get_root_item(canvas);
  amitk_canvas_text_new(root, amitk_view_get_name(view),
                      (gdouble) (column+0.5)*axis_width,
                      (gdouble) (row+0.5)*axis_height, -1,
                      AMITK_CANVAS_ANCHOR_NORTH,
                      "fill-color", "black",
                      "font-desc", amitk_fixed_font_desc, NULL);

  /* the x axis */
  amitk_canvas_polyline_new(root, FALSE, 0, "points", x_axis_line_points,
                          "stroke-color", "black", "line-width", 3.0,
                          "end-arrow", TRUE,
                          "arrow-length", (gdouble) AXIS_ARROW_LENGTH,
                          "arrow-tip-length", (gdouble) AXIS_ARROW_EDGE,
                          "arrow-width", (gdouble) AXIS_ARROW_WIDTH, NULL);

  /* the x label */
  amitk_canvas_text_new(root, x_axis_label, x_axis_label_x_location,
                      x_axis_label_y_location, -1, x_axis_label_anchor,
                      "fill-color", "black",
                      "font-desc", amitk_fixed_font_desc, NULL);

  /* the y axis */
  amitk_canvas_polyline_new(root, FALSE, 0, "points", y_axis_line_points,
                          "stroke-color", "black", "line-width", 3.0,
                          "end-arrow", TRUE,
                          "arrow-length", (gdouble) AXIS_ARROW_LENGTH,
                          "arrow-tip-length", (gdouble) AXIS_ARROW_EDGE,
                          "arrow-width", (gdouble) AXIS_ARROW_WIDTH, NULL);

  amitk_canvas_points_unref(x_axis_line_points);
  amitk_canvas_points_unref(y_axis_line_points);

  /* the y label */
  amitk_canvas_text_new(root, y_axis_label, y_axis_label_x_location,
                      y_axis_label_y_location, -1, y_axis_label_anchor,
                      "fill-color", "black",
                      "font-desc", amitk_fixed_font_desc, NULL);

  return;
}


void ui_common_update_sample_roi_item(AmitkCanvasItem * roi_item,
				      gint roi_width,
				      gdouble transparency
				      ) {

  rgba_t outline_color;
  rgba_t fill_color;

  outline_color = amitk_color_table_outline_color(AMITK_COLOR_TABLE_NIH, TRUE);
  fill_color = outline_color;
  fill_color.a *= transparency;

  g_object_set(roi_item,
               "line-width", (gdouble) roi_width,
               "fill-color-rgba", amitk_color_table_rgba_to_uint32(fill_color),
               "stroke-color-rgba", amitk_color_table_rgba_to_uint32(outline_color),
               NULL);

  return;
}


void ui_common_study_preferences_widgets(GtkWidget * packing_table,
					 gint table_row,
					 GtkWidget ** proi_width_spin,
					 AmitkCanvasItem ** proi_item,
					 GtkWidget ** proi_transparency_spin,
					 GtkWidget ** playout_button1,
					 GtkWidget ** playout_button2,
					 GtkWidget ** ppanel_layout_button1,
					 GtkWidget ** ppanel_layout_button2,
					 GtkWidget ** ppanel_layout_button3,
					 GtkWidget ** pmaintain_size_button,
					 GtkWidget ** ptarget_size_spin) {

  GtkWidget * label;
  GtkAdjustment * adjustment;
  GtkWidget * roi_canvas;
  AmitkCanvasPoints * roi_line_points;
  AmitkCanvasItem * root;
  GtkWidget * image;
  GtkWidget * hseparator;


  /* widgets to change the roi's size */
  label = gtk_label_new(_("ROI Width (pixels)"));
  gtk_grid_attach(GTK_GRID(packing_table), label, 0, table_row, 1, 1);
  gtk_widget_show(label);

  adjustment = gtk_adjustment_new(AMITK_PREFERENCES_MIN_ROI_WIDTH,
				  AMITK_PREFERENCES_MIN_ROI_WIDTH,
				  AMITK_PREFERENCES_MAX_ROI_WIDTH,1.0, 1.0, 0.0);
  *proi_width_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 1.0, 0);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(*proi_width_spin),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(*proi_width_spin), TRUE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(*proi_width_spin), TRUE);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(*proi_width_spin), GTK_UPDATE_ALWAYS);
  gtk_grid_attach(GTK_GRID(packing_table), *proi_width_spin,
                  1, table_row, 1, 1);
  gtk_widget_show(*proi_width_spin);

  /* a little canvas indicator thingie to show the user who the new preferences will look */
  roi_canvas = amitk_simple_canvas_new();
  gtk_widget_set_size_request(roi_canvas, 100, 100);
  amitk_simple_canvas_set_bounds(AMITK_SIMPLE_CANVAS(roi_canvas), 0.0, 0.0, 100.0, 100.0);
  gtk_grid_attach(GTK_GRID(packing_table), roi_canvas, 2, table_row, 1, 2);
  gtk_widget_show(roi_canvas);

  /* the box */
  roi_line_points = amitk_canvas_points_new(5);
  roi_line_points->coords[0] = 25.0; /* x1 */
  roi_line_points->coords[1] = 25.0; /* y1 */
  roi_line_points->coords[2] = 75.0; /* x2 */
  roi_line_points->coords[3] = 25.0; /* y2 */
  roi_line_points->coords[4] = 75.0; /* x3 */
  roi_line_points->coords[5] = 75.0; /* y3 */
  roi_line_points->coords[6] = 25.0; /* x4 */
  roi_line_points->coords[7] = 75.0; /* y4 */
  roi_line_points->coords[8] = 25.0; /* x4 */
  roi_line_points->coords[9] = 25.0; /* y4 */

  root = amitk_simple_canvas_get_root_item(AMITK_SIMPLE_CANVAS(roi_canvas));
  *proi_item = amitk_canvas_polyline_new(root, TRUE, 0,
				     "points", roi_line_points, 
				     NULL);
  amitk_canvas_points_unref(roi_line_points);
  table_row++;


  /* widget to change the transparency level */
  /* only works for anti-aliased canvases */
  /* widgets to change the roi's size */

  label = gtk_label_new(_("ROI Transparency"));
  gtk_grid_attach(GTK_GRID(packing_table), label, 0, table_row, 1, 1);
  gtk_widget_show(label);

  *proi_transparency_spin = gtk_spin_button_new_with_range(0.0,1.0,AMITK_PREFERENCES_DEFAULT_CANVAS_ROI_TRANSPARENCY);
  gtk_spin_button_set_increments(GTK_SPIN_BUTTON(*proi_transparency_spin),0.1,0.1);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(*proi_transparency_spin),FALSE);
  gtk_grid_attach(GTK_GRID(packing_table), *proi_transparency_spin,
                  1, table_row, 1, 1);
  gtk_widget_show(*proi_transparency_spin);
  table_row++;

  hseparator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_grid_attach(GTK_GRID(packing_table), hseparator, 0, table_row, 3, 1);
  table_row++;
  gtk_widget_show(hseparator);


  label = gtk_label_new(_("Canvas Layout:"));
  gtk_grid_attach(GTK_GRID(packing_table), label, 0, table_row, 1, 1);
  gtk_widget_show(label);

  /* the radio buttons */
  *playout_button1 = gtk_radio_button_new(NULL);
  image = gtk_image_new_from_icon_name("amide_icon_layout_linear",GTK_ICON_SIZE_DIALOG);
  gtk_button_set_image(GTK_BUTTON(*playout_button1), image);
  gtk_grid_attach(GTK_GRID(packing_table), *playout_button1,
                  1, table_row, 1, 1);
  g_object_set_data(G_OBJECT(*playout_button1), "layout", GINT_TO_POINTER(AMITK_LAYOUT_LINEAR));
  gtk_widget_show(*playout_button1);

  *playout_button2 = gtk_radio_button_new(NULL);
  gtk_radio_button_set_group(GTK_RADIO_BUTTON(*playout_button2), 
			     gtk_radio_button_get_group(GTK_RADIO_BUTTON(*playout_button1)));
  image = gtk_image_new_from_icon_name("amide_icon_layout_orthogonal",GTK_ICON_SIZE_DIALOG);
  gtk_button_set_image(GTK_BUTTON(*playout_button2), image);
  gtk_grid_attach(GTK_GRID(packing_table), *playout_button2,
                  2, table_row, 1, 1);
  g_object_set_data(G_OBJECT(*playout_button2), "layout", GINT_TO_POINTER(AMITK_LAYOUT_ORTHOGONAL));
  gtk_widget_show(*playout_button2);

  table_row++;


  label = gtk_label_new(_("Multiple Canvases Layout:"));
  gtk_grid_attach(GTK_GRID(packing_table), label, 0, table_row, 1, 1);
  gtk_widget_show(label);

  /* the radio buttons */
  *ppanel_layout_button1 = gtk_radio_button_new(NULL);
  image = gtk_image_new_from_icon_name("amide_icon_panels_mixed", GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_button_set_image(GTK_BUTTON(*ppanel_layout_button1), image);
  gtk_grid_attach(GTK_GRID(packing_table), *ppanel_layout_button1,
                  1, table_row, 1, 1);
  g_object_set_data(G_OBJECT(*ppanel_layout_button1), "panel_layout", GINT_TO_POINTER(AMITK_PANEL_LAYOUT_MIXED));
  gtk_widget_show(*ppanel_layout_button1);

  *ppanel_layout_button2 = gtk_radio_button_new(NULL);
  gtk_radio_button_set_group(GTK_RADIO_BUTTON(*ppanel_layout_button2), 
			     gtk_radio_button_get_group(GTK_RADIO_BUTTON(*ppanel_layout_button1)));
  image = gtk_image_new_from_icon_name("amide_icon_panels_linear_x", GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_button_set_image(GTK_BUTTON(*ppanel_layout_button2), image);
  gtk_grid_attach(GTK_GRID(packing_table), *ppanel_layout_button2,
                  2, table_row, 1, 1);
  g_object_set_data(G_OBJECT(*ppanel_layout_button2), "panel_layout", GINT_TO_POINTER(AMITK_PANEL_LAYOUT_LINEAR_X));
  gtk_widget_show(*ppanel_layout_button2);

  *ppanel_layout_button3 = gtk_radio_button_new(NULL);
  gtk_radio_button_set_group(GTK_RADIO_BUTTON(*ppanel_layout_button3), 
			     gtk_radio_button_get_group(GTK_RADIO_BUTTON(*ppanel_layout_button1)));
  image = gtk_image_new_from_icon_name("amide_icon_panels_linear_y", GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_button_set_image(GTK_BUTTON(*ppanel_layout_button3), image);
  gtk_grid_attach(GTK_GRID(packing_table), *ppanel_layout_button3,
                  3, table_row, 1, 1);
  g_object_set_data(G_OBJECT(*ppanel_layout_button3), "panel_layout", GINT_TO_POINTER(AMITK_PANEL_LAYOUT_LINEAR_Y));
  gtk_widget_show(*ppanel_layout_button3);

  table_row++;


  /* do we want the size of the canvas to not resize */
  label = gtk_label_new(_("Maintain view size constant:"));
  gtk_grid_attach(GTK_GRID(packing_table), label, 0, table_row, 1, 1);
  gtk_widget_show(label);

  *pmaintain_size_button = gtk_check_button_new();
  gtk_widget_set_halign(*pmaintain_size_button, GTK_ALIGN_CENTER);
  gtk_grid_attach(GTK_GRID(packing_table), *pmaintain_size_button,
                  1, table_row, 1, 1);
  gtk_widget_show(*pmaintain_size_button);
  table_row++;


  /* widgets to change the amount of empty space in the center of the target */
  label = gtk_label_new(_("Target Empty Area (pixels)"));
  gtk_grid_attach(GTK_GRID(packing_table), label, 0, table_row, 1, 1);
  gtk_widget_show(label);

  adjustment = gtk_adjustment_new(AMITK_PREFERENCES_MIN_TARGET_EMPTY_AREA, 
				  AMITK_PREFERENCES_MIN_TARGET_EMPTY_AREA, 
				  AMITK_PREFERENCES_MAX_TARGET_EMPTY_AREA, 1.0, 1.0, 0.0);
  *ptarget_size_spin = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 1.0, 0);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(*ptarget_size_spin),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(*ptarget_size_spin), TRUE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(*ptarget_size_spin), TRUE);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(*ptarget_size_spin), GTK_UPDATE_ALWAYS);

  gtk_grid_attach(GTK_GRID(packing_table), *ptarget_size_spin,
                  1, table_row, 1, 1);
  gtk_widget_show(*ptarget_size_spin);


  return;
}

GtkWidget * ui_common_create_view_axis_indicator(AmitkLayout layout) {

  GtkWidget * axis_indicator;
  AmitkView i_view;

  axis_indicator = amitk_simple_canvas_new();
  switch(layout) {
  case AMITK_LAYOUT_ORTHOGONAL:
    gtk_widget_set_size_request(axis_indicator, 2.0*AXIS_WIDTH, 2.0*ORTHOGONAL_AXIS_HEIGHT);
    amitk_simple_canvas_set_bounds(AMITK_SIMPLE_CANVAS(axis_indicator), 0.0, 0.0,
				   2.0*AXIS_WIDTH, 2.0*ORTHOGONAL_AXIS_HEIGHT);
    ui_common_draw_view_axis(AMITK_SIMPLE_CANVAS(axis_indicator), 0, 0, AMITK_VIEW_TRANSVERSE,
			     layout, AXIS_WIDTH, ORTHOGONAL_AXIS_HEIGHT);
    ui_common_draw_view_axis(AMITK_SIMPLE_CANVAS(axis_indicator), 1, 0, AMITK_VIEW_CORONAL,
			     layout, AXIS_WIDTH, ORTHOGONAL_AXIS_HEIGHT);
    ui_common_draw_view_axis(AMITK_SIMPLE_CANVAS(axis_indicator), 0, 1, AMITK_VIEW_SAGITTAL,
			     layout, AXIS_WIDTH, ORTHOGONAL_AXIS_HEIGHT);

    break;
  case AMITK_LAYOUT_LINEAR:
  default:
    gtk_widget_set_size_request(axis_indicator, 3.0*AXIS_WIDTH, LINEAR_AXIS_HEIGHT);
    amitk_simple_canvas_set_bounds(AMITK_SIMPLE_CANVAS(axis_indicator), 0.0, 0.0,
				   3.0*AXIS_WIDTH, LINEAR_AXIS_HEIGHT);
    for (i_view=0;i_view< AMITK_VIEW_NUM;i_view++)
      ui_common_draw_view_axis(AMITK_SIMPLE_CANVAS(axis_indicator), 0, i_view, i_view,
			       layout, AXIS_WIDTH, LINEAR_AXIS_HEIGHT);
    break;
  }


  return GTK_WIDGET(axis_indicator);
}



/* load in the cursors */
static void ui_common_cursor_init(void) {

  cairo_surface_t * surf;
  cairo_t * cr;
  GdkPixbuf * pixbuf;
  GdkCursor * small_dot;
  GdkDisplay * d;
 
  surf = cairo_image_surface_create(CAIRO_FORMAT_A1, 3, 3);
  cr = cairo_create(surf);
  cairo_arc(cr, 0, 0, 1.5, 0, 2 * M_PI);
  cairo_fill(cr);
  cairo_destroy(cr);
  pixbuf = gdk_pixbuf_get_from_surface(surf, 0, 0, 3, 3);
  cairo_surface_destroy(surf);
  d = gdk_display_get_default();
  small_dot = gdk_cursor_new_from_pixbuf(d, pixbuf, 0, 0);
  g_object_unref(pixbuf);
                                     
  /* load in the cursors */

  ui_common_cursor[UI_CURSOR_DEFAULT] = NULL;
  ui_common_cursor[UI_CURSOR_ROI_MODE] =  gdk_cursor_new_for_display(d, GDK_DRAFT_SMALL);
  ui_common_cursor[UI_CURSOR_ROI_RESIZE] = small_dot; /* was GDK_SIZING */
  ui_common_cursor[UI_CURSOR_ROI_ROTATE] = small_dot; /* was GDK_EXCHANGE */
  ui_common_cursor[UI_CURSOR_ROI_DRAW] = gdk_cursor_new_for_display(d, GDK_PENCIL);
  ui_common_cursor[UI_CURSOR_OBJECT_SHIFT] = small_dot; /* was GDK_FLEUR */
  ui_common_cursor[UI_CURSOR_ROI_ISOCONTOUR] = gdk_cursor_new_for_display(d, GDK_DRAFT_SMALL);
  ui_common_cursor[UI_CURSOR_ROI_ERASE] = gdk_cursor_new_for_display(d, GDK_DRAFT_SMALL);
  ui_common_cursor[UI_CURSOR_DATA_SET_MODE] = gdk_cursor_new_for_display(d, GDK_CROSSHAIR);
  ui_common_cursor[UI_CURSOR_FIDUCIAL_MARK_MODE] = gdk_cursor_new_for_display(d, GDK_DRAFT_SMALL);
  ui_common_cursor[UI_CURSOR_RENDERING_ROTATE_XY] = gdk_cursor_new_for_display(d, GDK_FLEUR);
  ui_common_cursor[UI_CURSOR_RENDERING_ROTATE_Z] = gdk_cursor_new_for_display(d, GDK_EXCHANGE);
  ui_common_cursor[UI_CURSOR_WAIT] = gdk_cursor_new_for_display(d, GDK_WATCH);
  

 
  ui_common_cursors_initialized = TRUE;
  return;
}


/* replaces the current cursor with the specified cursor */
void ui_common_place_cursor_no_wait(ui_common_cursor_t which_cursor, GtkWidget * widget) {

  GdkCursor * cursor;

  /* make sure we have cursors */
  if (!ui_common_cursors_initialized) ui_common_cursor_init();

  /* sanity checks */
  if (widget == NULL) return;
  if (!gtk_widget_get_realized(widget)) return;

  if (which_cursor != UI_CURSOR_WAIT)
    current_cursor = which_cursor;
  
  cursor = ui_common_cursor[which_cursor];

  gdk_window_set_cursor(gtk_widget_get_parent_window(widget), cursor);

  return;
}

void ui_common_remove_wait_cursor(GtkWidget * widget) {

  ui_common_place_cursor_no_wait(current_cursor, widget);
}

/* replaces the current cursor with the specified cursor */
void ui_common_place_cursor(ui_common_cursor_t which_cursor, GtkWidget * widget) {

  /* call the actual function */
  ui_common_place_cursor_no_wait(which_cursor, widget);

  /* do any events pending, this allows the cursor to get displayed */
  while (gtk_events_pending()) gtk_main_iteration();

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


/* a simple request dialog */
GtkWidget * ui_common_entry_dialog(GtkWindow * parent, gchar * prompt, gchar **return_str_ptr) {
  
  GtkWidget * dialog;
  GtkWidget * table;
  guint table_row;
  GtkWidget * entry;
  GtkWidget * label;
  GtkWidget * image;
  

  dialog = gtk_dialog_new_with_buttons (_("Request Dialog"),  parent,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					_("_Cancel"), GTK_RESPONSE_CLOSE,
					_("_OK"), GTK_RESPONSE_OK,
					NULL);
  g_object_set_data(G_OBJECT(dialog), "return_str_ptr", return_str_ptr);
  g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(init_response_cb), NULL);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);
  gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK,FALSE);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

  table = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(table), Y_PADDING);
  gtk_grid_set_column_spacing(GTK_GRID(table), X_PADDING);
  table_row=0;
  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area
                                  (GTK_DIALOG(dialog))), table);


  image = gtk_image_new_from_icon_name("dialog-question", GTK_ICON_SIZE_DIALOG);
  gtk_grid_attach(GTK_GRID(table), image, 0, table_row, 1, 1);

  label = gtk_label_new(prompt);
  gtk_grid_attach(GTK_GRID(table), label, 1, table_row, 1, 1);
  table_row++;


  entry = gtk_entry_new();
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
  gtk_grid_attach(GTK_GRID(table), entry, 1, table_row, 1, 1);
  g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(entry_activate), dialog);
  table_row++;


  gtk_widget_show_all(dialog);

  return dialog;
}


void ui_common_init_dialog_response_cb (GtkDialog * dialog, gint response_id, gpointer data) {
  
  gint return_val;

  switch(response_id) {
  case AMITK_RESPONSE_EXECUTE:
  case GTK_RESPONSE_CLOSE:
    g_signal_emit_by_name(G_OBJECT(dialog), "delete_event", NULL, &return_val);
    if (!return_val) gtk_widget_destroy(GTK_WIDGET(dialog));
    break;

  default:
    break;
  }

  return;
}

GList * ui_common_init_dialog_selected_objects(GtkWidget * dialog) {

  GList * objects;
  AmitkTreeView * tree_view;
  
  tree_view = g_object_get_data(G_OBJECT(dialog), "tree_view");
  objects = amitk_tree_view_get_multiple_selection_objects(tree_view);

  return objects;
}


void ui_common_toolbar_insert_widget(GtkWidget * toolbar, GtkWidget * widget, const gchar * tooltip, gint position) {

  GtkToolItem * toolbar_item;

  toolbar_item = gtk_tool_item_new();
  gtk_container_add(GTK_CONTAINER(toolbar_item), widget);
  if (tooltip != NULL)
    gtk_widget_set_tooltip_text(widget, tooltip);
  gtk_tool_item_set_homogeneous(toolbar_item, FALSE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolbar_item, position);

  return;
}

void ui_common_toolbar_append_widget(GtkWidget * toolbar, GtkWidget * widget, const gchar * tooltip) {

  ui_common_toolbar_insert_widget(toolbar, widget, tooltip, -1);
  return;
}

void ui_common_toolbar_append_widget_full(GtkWidget * toolbar, GtkWidget * widget, const gchar * label, const gchar * tooltip, GCallback cb, gpointer data) {

  GtkToolItem * toolbar_item;
  GtkWidget * menu_item;

  toolbar_item = gtk_tool_item_new();
  gtk_container_add(GTK_CONTAINER(toolbar_item), widget);
  if (tooltip != NULL)
    gtk_widget_set_tooltip_text(widget, tooltip);
  if (label != NULL) {
    menu_item = gtk_menu_item_new_with_label(label);
    gtk_tool_item_set_proxy_menu_item(toolbar_item, label, menu_item);
    if (cb != NULL)
      g_signal_connect(menu_item, "activate", cb, data);
  }
  gtk_tool_item_set_homogeneous(toolbar_item, FALSE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolbar_item, -1);
}

void ui_common_toolbar_append_separator(GtkWidget * toolbar) {
  GtkToolItem * toolbar_item;
  toolbar_item =  gtk_separator_tool_item_new();
  gtk_tool_item_set_homogeneous(toolbar_item, FALSE);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolbar_item, -1);
  return;
}




/* internal variables */
static GList * windows = NULL;

/* keep track of open windows */
void amide_register_window(gpointer * widget) {

  g_return_if_fail(widget != NULL);
  
  windows = g_list_append(windows, widget);

  return;
}


/* keep track of open windows */
void amide_unregister_window(gpointer * widget) {
  GtkApplication * app;

  g_return_if_fail(widget != NULL);

  windows = g_list_remove(windows, widget);
  app = gtk_window_get_application(GTK_WINDOW(widget));

  if (windows == NULL) g_application_quit(G_APPLICATION(app));

  return;
}


/* this should cleanly exit the program */
void amide_unregister_all_windows(void) {

  gboolean return_val;
  gint number_to_leave=0;

  while (g_list_nth(windows, number_to_leave) != NULL) {
    /* this works, because each delete event should call amide_unregister_window */
    g_signal_emit_by_name(G_OBJECT(windows->data), "delete_event", NULL, &return_val);
    if (return_val == TRUE) number_to_leave++;
  }

  return;
}


void amide_call_help(const gchar * link_id) {

#ifndef OLD_WIN32_HACKS
  GError *error=NULL;
  amide_gnome_help_display(PACKAGE, link_id, &error);
  if (error != NULL) {
    g_warning("couldn't open help file, error: %s", error->message);
    g_error_free(error);
  }

#else
  g_warning("Help is unavailable in the Windows version. Please see the help documentation online at http://amide.sf.net, or in the AMIDE install folder");
#endif

  return;
}



