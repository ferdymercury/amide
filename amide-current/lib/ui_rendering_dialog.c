/* ui_rendering_dialog.c
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

#include "config.h"

#ifdef AMIDE_LIBVOLPACK_SUPPORT


#include <gnome.h>
#include <math.h>
#include "ui_rendering_dialog.h"
#include "ui_rendering_dialog_cb.h"


#define GAMMA_CURVE_WIDTH -2 /* sets automatically */
#define GAMMA_CURVE_HEIGHT 100

/* function to set what's in the density or gradient opacity curves when it gets realized */
static void setup_curve(GtkWidget * gamma_curve, gpointer data, classification_t type) {

  GtkWidget * curve;
  rendering_t * context = data;
  gfloat (*curve_ctrl_points)[2];
  guint i;

  curve = GTK_GAMMA_CURVE(gamma_curve)->curve;

  /* allocate some memory for the curve we're passing to the curve widget */
  if ((curve_ctrl_points = g_malloc(context->num_points[type]*sizeof(gfloat)*2)) == NULL) {
    g_warning("Failed to Allocate Memory for Ramp");
    return;
  }
  
  /* copy rampx and rampy into the curve array  */
  for (i=0; i< context->num_points[type]; i++) {
    curve_ctrl_points[i][0]=context->ramp_x[type][i];
    curve_ctrl_points[i][1]=context->ramp_y[type][i];
  }
  
  GTK_CURVE(curve)->num_ctlpoints =  context->num_points[type];
  g_free(GTK_CURVE(curve)->ctlpoint);
  GTK_CURVE(curve)->ctlpoint = curve_ctrl_points;
  
  /* doing some hackish stuff to get this to work as I'd like it (i.e. saving state) */
  switch (context->curve_type[type]) {
  case CURVE_SPLINE:
    GTK_CURVE(curve)->curve_type = GTK_CURVE_TYPE_SPLINE;
    GTK_TOGGLE_BUTTON(GTK_GAMMA_CURVE(gamma_curve)->button[0])->active=TRUE;
    break;
  case CURVE_LINEAR:
  default:
    GTK_CURVE(curve)->curve_type = GTK_CURVE_TYPE_LINEAR;
    GTK_TOGGLE_BUTTON(GTK_GAMMA_CURVE(gamma_curve)->button[1])->active=TRUE;
    break;
  }

  return;
}

/* function that sets up the rendering options dialog */
void ui_rendering_dialog_create(ui_rendering_t * ui_rendering) {
  
  GtkWidget * rendering_dialog;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * label;
  GtkWidget * option_menu;
  GtkWidget * menu;
  GtkWidget * menuitem;
  GtkWidget * check_button;
  GtkWidget * entry;
  GtkWidget * gamma_curve;
  GtkWidget * hseparator;
  rendering_quality_t i_quality;
  pixel_type_t i_pixel_type;
  guint table_row = 0;
  renderings_t * temp_list;
  color_table_t i_color_table;
  
  if (ui_rendering->parameter_dialog != NULL)
    return;
    
  temp_string = g_strdup_printf("%s: Rendering Parameters Dialog",PACKAGE);
  rendering_dialog = gnome_property_box_new();
  ui_rendering->parameter_dialog = rendering_dialog;
  gtk_window_set_title(GTK_WINDOW(rendering_dialog), temp_string);
  g_free(temp_string);

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(rendering_dialog), "close",
  		     GTK_SIGNAL_FUNC(ui_rendering_dialog_cb_close), ui_rendering);
  gtk_signal_connect(GTK_OBJECT(rendering_dialog), "apply",
  		     GTK_SIGNAL_FUNC(ui_rendering_dialog_cb_apply), ui_rendering);
  gtk_signal_connect(GTK_OBJECT(rendering_dialog), "help",
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_cb_help),ui_rendering);

  /* not operating as a true property box dialog, i.e. no ok option */
  gtk_widget_destroy(GNOME_PROPERTY_BOX(rendering_dialog)->ok_button);




  /* ---------------------------
     Basic Parameters page 
     --------------------------- */


  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(4,2,FALSE);
  label = gtk_label_new("Basic Parameters");
  table_row=0;
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(rendering_dialog), GTK_WIDGET(packing_table), label);

  /* widgets to change the quality versus speed of rendering */
  label = gtk_label_new("Speed versus Quality");
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(label), 0,1,
		   table_row, table_row+1,
		   0, 0,
		   X_PADDING, Y_PADDING);

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();

  for (i_quality=0; i_quality<NUM_QUALITIES; i_quality++) {
    menuitem = gtk_menu_item_new_with_label(rendering_quality_names[i_quality]);
    gtk_menu_append(GTK_MENU(menu), menuitem);
    gtk_object_set_data(GTK_OBJECT(menuitem), "quality", GINT_TO_POINTER(i_quality)); 
    gtk_object_set_data(GTK_OBJECT(menuitem), "rendering_dialog", rendering_dialog);
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
     		       GTK_SIGNAL_FUNC(ui_rendering_dialog_cb_change_quality), 
    		       ui_rendering);
  }
  
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), ui_rendering->quality);
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(option_menu), 1,2, 
		   table_row,table_row+1,
		   GTK_EXPAND | GTK_FILL, 0, 
		   X_PADDING, Y_PADDING);
  table_row++;



  /* widgets to change the returned pixel type of the rendering */
  label = gtk_label_new("Return Type");
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(label), 0,1,
		   table_row, table_row+1,
		   0, 0,
		   X_PADDING, Y_PADDING);

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();

  for (i_pixel_type=0; i_pixel_type<NUM_PIXEL_TYPES; i_pixel_type++) {
    menuitem = gtk_menu_item_new_with_label(pixel_type_names[i_pixel_type]);
    gtk_menu_append(GTK_MENU(menu), menuitem);
    gtk_object_set_data(GTK_OBJECT(menuitem), "pixel_type", GINT_TO_POINTER(i_pixel_type)); 
    gtk_object_set_data(GTK_OBJECT(menuitem), "rendering_dialog", rendering_dialog);
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
     		       GTK_SIGNAL_FUNC(ui_rendering_dialog_cb_change_pixel_type), 
    		       ui_rendering);
  }
  
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), ui_rendering->pixel_type);
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(option_menu), 1,2, 
		   table_row,table_row+1,
		   GTK_EXPAND | GTK_FILL, 0, 
		   X_PADDING, Y_PADDING);
  table_row++;

  /* widgets to change the zoom */
  label = gtk_label_new("Zoom");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", ui_rendering->zoom);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "rendering_dialog", rendering_dialog);
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_cb_change_zoom), 
		     ui_rendering);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator,0,2,
		   table_row, table_row+1, GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
  table_row++;

  /* widget for the stereo eye angle */
  label = gtk_label_new("Stereo Angle");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", ui_rendering->stereo_eye_angle);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "rendering_dialog", rendering_dialog);
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_cb_change_eye_angle), 
		     ui_rendering);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* widget for the stereo eye width */
  label = gtk_label_new("Eye Width (mm)");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", 
				gdk_screen_width_mm()*
				ui_rendering->stereo_eye_width/
				((gdouble) gdk_screen_width()));
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "rendering_dialog", rendering_dialog);
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_cb_change_eye_width),
		     ui_rendering);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator,0,2,
		   table_row, table_row+1, GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
  table_row++;

  /* the depth cueing enabling button */
  check_button = gtk_check_button_new_with_label ("enable/disable depth cueing");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), ui_rendering->depth_cueing);
  gtk_signal_connect(GTK_OBJECT(check_button), "toggled", 
  		     GTK_SIGNAL_FUNC(ui_rendering_dialog_cb_depth_cueing_toggle), 
  		     ui_rendering);
  gtk_object_set_data(GTK_OBJECT(check_button), "rendering_dialog", rendering_dialog);
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(check_button), 0,2, 
		   table_row,table_row+1,
		   GTK_FILL, 0, 
		   X_PADDING, Y_PADDING);
  table_row++;


  label = gtk_label_new("Front Factor");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", ui_rendering->front_factor);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "rendering_dialog", rendering_dialog);
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_cb_change_front_factor), 
		     ui_rendering);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  label = gtk_label_new("Density");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  entry = gtk_entry_new();
  temp_string = g_strdup_printf("%f", ui_rendering->density);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
  g_free(temp_string);
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  gtk_object_set_data(GTK_OBJECT(entry), "rendering_dialog", rendering_dialog);
  gtk_signal_connect(GTK_OBJECT(entry), "changed", 
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_cb_change_density), 
		     ui_rendering);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;




  /* ---------------------------
     Volume Specific Stuff
     --------------------------- */

  temp_list = ui_rendering->contexts;
  while (temp_list != NULL) {
    packing_table = gtk_table_new(4,3,FALSE);
    table_row=0;
    label = gtk_label_new(temp_list->rendering_context->name);
    gnome_property_box_append_page (GNOME_PROPERTY_BOX(rendering_dialog), 
				    GTK_WIDGET(packing_table), label);

    /* color table selector */
    label = gtk_label_new("color table:");
    gtk_table_attach(GTK_TABLE(packing_table), label, 0,1, table_row,table_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    option_menu = gtk_option_menu_new();
    menu = gtk_menu_new();
    for (i_color_table=0; i_color_table<NUM_COLOR_TABLES; i_color_table++) {
      menuitem = gtk_menu_item_new_with_label(color_table_names[i_color_table]);
      gtk_menu_append(GTK_MENU(menu), menuitem);
      gtk_object_set_data(GTK_OBJECT(menuitem), "color_table", GINT_TO_POINTER(i_color_table));
      gtk_object_set_data(GTK_OBJECT(menuitem), "rendering_dialog", rendering_dialog);
      gtk_object_set_data(GTK_OBJECT(menuitem), "ui_rendering", ui_rendering);
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
			 GTK_SIGNAL_FUNC(ui_rendering_dialog_cb_color_table), 
			 temp_list->rendering_context);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
    gtk_table_attach(GTK_TABLE(packing_table), option_menu, 1,2, table_row,table_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu),
				temp_list->rendering_context->color_table);
    gtk_widget_show_all(option_menu);

    table_row++;


    /* change the classification parameters */

    /* Density Dependent Opacity */
    label = gtk_label_new("Density\nDependent\nOpacity");
    gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  
    gamma_curve = gtk_gamma_curve_new();
    gtk_widget_set_usize(gamma_curve, GAMMA_CURVE_WIDTH, GAMMA_CURVE_HEIGHT);
    gtk_curve_set_range(GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve), 0.0, RENDERING_DENSITY_MAX, 0.0, 1.0);

    gtk_object_set_data(GTK_OBJECT(gamma_curve), "rendering_dialog", rendering_dialog);
    gtk_object_set_data(GTK_OBJECT(gamma_curve), "gamma_curve", gamma_curve);
    gtk_object_set_data(GTK_OBJECT(gamma_curve), "ui_rendering", ui_rendering);
    gtk_object_set_data(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->curve), "gamma_curve", gamma_curve);
    
    gtk_object_set_data(GTK_OBJECT(gamma_curve), "classification_type", 
			GINT_TO_POINTER(DENSITY_CLASSIFICATION));

    gtk_signal_connect(GTK_OBJECT(gamma_curve), "button-release-event", 
		       GTK_SIGNAL_FUNC(ui_rendering_dialog_cb_opacity_event), 
		       temp_list->rendering_context);
    gtk_signal_connect(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->curve), "curve_type_changed", 
		       GTK_SIGNAL_FUNC(ui_rendering_dialog_cb_opacity), 
		       temp_list->rendering_context);
    setup_curve(gamma_curve, temp_list->rendering_context, DENSITY_CLASSIFICATION);
    /* disable the gamma button and free drawing button */
    gtk_widget_destroy(GTK_GAMMA_CURVE(gamma_curve)->button[3]);
    gtk_widget_destroy(GTK_GAMMA_CURVE(gamma_curve)->button[2]);
    
    /* and attach */
    gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(gamma_curve), 1,3,
		     table_row, table_row+1,
		     GTK_EXPAND | GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);

    table_row++;
    
    
    /* Gradient Dependent Opacity */
    label = gtk_label_new("Gradient\nDependent\nOpacity");
    gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    
    gamma_curve = gtk_gamma_curve_new();
    gtk_widget_set_usize(gamma_curve, GAMMA_CURVE_WIDTH, GAMMA_CURVE_HEIGHT);
    gtk_curve_set_range(GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve), 0.0, RENDERING_GRADIENT_MAX, 0.0, 1.0);
    
    gtk_object_set_data(GTK_OBJECT(gamma_curve), "rendering_dialog", rendering_dialog);
    gtk_object_set_data(GTK_OBJECT(gamma_curve), "gamma_curve", gamma_curve);
    gtk_object_set_data(GTK_OBJECT(gamma_curve), "ui_rendering", ui_rendering);
    gtk_object_set_data(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->curve), "gamma_curve", gamma_curve);
    
    gtk_object_set_data(GTK_OBJECT(gamma_curve), "classification_type", 
			GINT_TO_POINTER(GRADIENT_CLASSIFICATION));

    /* connect the signals together */
    gtk_signal_connect(GTK_OBJECT(gamma_curve), "button-release-event", 
		       GTK_SIGNAL_FUNC(ui_rendering_dialog_cb_opacity_event), 
		       temp_list->rendering_context);
    gtk_signal_connect(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->curve), "curve_type_changed", 
		       GTK_SIGNAL_FUNC(ui_rendering_dialog_cb_opacity), 
		       temp_list->rendering_context);
    setup_curve(gamma_curve, temp_list->rendering_context, GRADIENT_CLASSIFICATION);
    /* disable the gamma button and free drawing button */
    gtk_widget_destroy(GTK_GAMMA_CURVE(gamma_curve)->button[3]);
    gtk_widget_destroy(GTK_GAMMA_CURVE(gamma_curve)->button[2]);
    
    /* and attach */
    gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(gamma_curve), 1,3,
		     table_row, table_row+1, 
		     GTK_EXPAND | GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
    gtk_widget_show(gamma_curve);
    table_row++;

    temp_list=temp_list->next;
  }


  /* and show all our widgets */
  gtk_widget_show_all(rendering_dialog);

  return;
}

#endif










