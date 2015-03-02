/* ui_preferences_dialog.c
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
#include "study.h"
#include "ui_study.h"
#include "ui_preferences_dialog_cb.h"
#include "ui_preferences_dialog.h"
#include "../pixmaps/linear_layout.xpm"
#include "../pixmaps/orthogonal_layout.xpm"


static gchar * line_style_names[] = {
  "Solid",
  "On/Off",
  "Double Dash"
};


/* function that sets up the preferences dialog */
GtkWidget * ui_preferences_dialog_create(ui_study_t * ui_study) {
  
  GtkWidget * preferences_dialog;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * label;
  GtkObject * adjustment;
  GtkWidget * spin_button;
  GtkWidget * option_menu;
  GtkWidget * menu;
  GtkWidget * menuitem;
  GtkWidget * gtkpixmap;
  GtkWidget * radio_button1;
  GtkWidget * radio_button2;
  GdkPixmap * gdkpixmap;
  GdkBitmap * gdkbitmap;
  GdkColormap * colormap;

  GnomeCanvas * roi_indicator;
  GnomeCanvasItem * roi_item;
  guint table_row;
  GdkLineStyle i_line_style;
  GnomeCanvasPoints * roi_line_points;
  rgba_t outline_color;

  /* sanity checks */
  g_return_val_if_fail(ui_study != NULL, NULL);
  g_return_val_if_fail(ui_study->preferences_dialog == NULL, NULL);
    
  temp_string = g_strdup_printf("%s: Preferences Dialog",PACKAGE);
  preferences_dialog = gnome_property_box_new();
  gtk_window_set_title(GTK_WINDOW(preferences_dialog), temp_string);
  g_free(temp_string);

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(preferences_dialog), "close",
		     GTK_SIGNAL_FUNC(ui_preferences_dialog_cb_close),
		     ui_study);
  gtk_signal_connect(GTK_OBJECT(preferences_dialog), "apply",
		     GTK_SIGNAL_FUNC(ui_preferences_dialog_cb_apply),
		     ui_study);
  gtk_signal_connect(GTK_OBJECT(preferences_dialog), "help",
		     GTK_SIGNAL_FUNC(ui_preferences_dialog_cb_help),
		     ui_study);




  /* ---------------------------
     ROI Drawing page 
     --------------------------- */


  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(4,5,FALSE);
  label = gtk_label_new("ROI Drawing");
  table_row=0;
  gnome_property_box_append_page(GNOME_PROPERTY_BOX(preferences_dialog), packing_table, label);

  /* widgets to change the roi's size */
  label = gtk_label_new("Width (pixels)");
  gtk_table_attach(GTK_TABLE(packing_table), label, 
		   0,1, table_row, table_row+1,
		   0, 0, X_PADDING, Y_PADDING);

  adjustment = gtk_adjustment_new(ui_study->roi_width,
				  UI_STUDY_MIN_ROI_WIDTH,
				  UI_STUDY_MAX_ROI_WIDTH,1.0, 1.0, 1.0);
  spin_button = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 1.0, 0);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_button),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(spin_button), TRUE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_button), TRUE);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin_button), GTK_UPDATE_ALWAYS);

  /* this is where we'll store the new roi width */
  gtk_object_set_data(GTK_OBJECT(preferences_dialog), "new_roi_width", GINT_TO_POINTER(ui_study->roi_width));

  gtk_signal_connect(GTK_OBJECT(spin_button), "changed",  
		     GTK_SIGNAL_FUNC(ui_preferences_dialog_cb_roi_width), ui_study);
  gtk_table_attach(GTK_TABLE(packing_table), spin_button, 1,2, 
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* widgets to change the roi's line style */
  label = gtk_label_new("Line Style:");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();

  for (i_line_style=0; i_line_style<=GDK_LINE_DOUBLE_DASH; i_line_style++) {
    menuitem = gtk_menu_item_new_with_label(line_style_names[i_line_style]);
    gtk_menu_append(GTK_MENU(menu), menuitem);
    gtk_object_set_data(GTK_OBJECT(menuitem), "line_style", GINT_TO_POINTER(i_line_style)); 
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
     		       GTK_SIGNAL_FUNC(ui_preferences_dialog_cb_line_style), ui_study);
  }
  
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), ui_study->line_style);
  gtk_object_set_data(GTK_OBJECT(preferences_dialog), "new_line_style", 
		      GINT_TO_POINTER(ui_study->line_style));
  gtk_widget_set_usize (option_menu, 125, -2);
  gtk_table_attach(GTK_TABLE(packing_table),  option_menu, 1,2, 
		   table_row,table_row+1, GTK_FILL, 0,  X_PADDING, Y_PADDING);
  table_row++;


  /* a little canvas indicator thingie to show the user who the new preferences will look */
  roi_indicator = GNOME_CANVAS(gnome_canvas_new());
  gtk_widget_set_usize(GTK_WIDGET(roi_indicator), 100, 100);
  gnome_canvas_set_scroll_region(roi_indicator, 0.0, 0.0, 100.0, 100.0);
  gtk_table_attach(GTK_TABLE(packing_table),  GTK_WIDGET(roi_indicator), 
		   2,4,0,2,
		   GTK_FILL, 0,  X_PADDING, Y_PADDING);
  /* the x axis */
  roi_line_points = gnome_canvas_points_new(5);
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

  if (ui_study->active_volume != NULL)
    outline_color = color_table_outline_color(ui_study->active_volume->color_table, TRUE);
  else
    outline_color = color_table_outline_color(BW_LINEAR, TRUE);
  roi_item = gnome_canvas_item_new(gnome_canvas_root(roi_indicator), 
				   gnome_canvas_line_get_type(),
				   "points", roi_line_points, 
				   "fill_color_rgba", color_table_rgba_to_uint32(outline_color), 
  				   "width_pixels", ui_study->roi_width,
    				   "line_style", ui_study->line_style, NULL);
  gnome_canvas_points_unref(roi_line_points);
  gtk_object_set_data(GTK_OBJECT(preferences_dialog), "roi_item", roi_item);

  /* ---------------------------
     Canvas Setup
     --------------------------- */


  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(4,5,FALSE);
  label = gtk_label_new("Canvas Setup");
  table_row=0;
  gnome_property_box_append_page(GNOME_PROPERTY_BOX(preferences_dialog), packing_table, label);

  /* widgets to change the roi's size */
  label = gtk_label_new("Layout:");
  gtk_table_attach(GTK_TABLE(packing_table), label, 
		   0,1, table_row, table_row+1,
		   0, 0, X_PADDING, Y_PADDING);

  /* the radio buttons */
  radio_button1 = gtk_radio_button_new(NULL);
  colormap = gtk_widget_get_colormap(preferences_dialog);
  gdkpixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL,colormap,&gdkbitmap,NULL,linear_layout_xpm);
  gtkpixmap = gtk_pixmap_new(gdkpixmap, gdkbitmap);
  gtk_container_add(GTK_CONTAINER(radio_button1), gtkpixmap);
  gtk_table_attach(GTK_TABLE(packing_table), radio_button1,
  		   1,2, table_row, table_row+1,
  		   0, 0, X_PADDING, Y_PADDING);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button1), 
			       (ui_study->canvas_layout == LINEAR_LAYOUT));
  gtk_object_set_data(GTK_OBJECT(radio_button1), "layout", GINT_TO_POINTER(LINEAR_LAYOUT));
  gtk_object_set_data(GTK_OBJECT(preferences_dialog), "new_layout", 
		      GINT_TO_POINTER(ui_study->canvas_layout));

  radio_button2 = gtk_radio_button_new(NULL);
  gtk_radio_button_set_group(GTK_RADIO_BUTTON(radio_button2), 
			     gtk_radio_button_group(GTK_RADIO_BUTTON(radio_button1)));
  gdkpixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL,colormap,&gdkbitmap,NULL,orthogonal_layout_xpm);
  gtkpixmap = gtk_pixmap_new(gdkpixmap, gdkbitmap);
  gtk_container_add(GTK_CONTAINER(radio_button2), gtkpixmap);
  gtk_table_attach(GTK_TABLE(packing_table), radio_button2,
  		   2,3, table_row, table_row+1,
  		   0, 0, X_PADDING, Y_PADDING);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button2), 
			       (ui_study->canvas_layout == ORTHOGONAL_LAYOUT));
  gtk_object_set_data(GTK_OBJECT(radio_button2), "layout", GINT_TO_POINTER(ORTHOGONAL_LAYOUT));

  gtk_signal_connect(GTK_OBJECT(radio_button1), "clicked",  
		     GTK_SIGNAL_FUNC(ui_preferences_dialog_cb_layout), ui_study);
  gtk_signal_connect(GTK_OBJECT(radio_button2), "clicked",  
		     GTK_SIGNAL_FUNC(ui_preferences_dialog_cb_layout), ui_study);

  table_row++;



  /* and show all our widgets */
  gtk_widget_show_all(preferences_dialog);

  return preferences_dialog;
}











