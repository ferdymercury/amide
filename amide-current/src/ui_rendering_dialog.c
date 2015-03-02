/* ui_rendering_dialog.c
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

#ifdef AMIDE_LIBVOLPACK_SUPPORT


#include <gnome.h>
#include <math.h>
#include "amide.h"
#include "realspace.h"
#include "color_table.h"
#include "volume.h"
#include "color_table2.h"
#include "rendering.h"
#include "roi.h"
#include "study.h"
#include "image.h"
#include "ui_rendering.h"
#include "ui_rendering_dialog.h"
#include "ui_rendering_dialog_callbacks.h"

/* function to update what's in the density and gradient opacity curves */
void ui_rendering_dialog_update_curves(ui_rendering_t * ui_rendering, guint which) {

  rendering_list_t * temp_list=ui_rendering->contexts;
  rendering_t * context;
  GtkWidget * density_curve;
  GtkWidget * gradient_curve;
  gfloat (*curve_points)[2];
  guint i;

  /* figure out which context */
  if (which == 0) /* we'll just use the first context if we're applying to all */
    context = temp_list->rendering_context;
  else {
    which--;
    while (which != 0) {
      temp_list = temp_list->next;
      which--;
    }
    context = temp_list->rendering_context;
  }

  /* get points to the two curve widgets */
  density_curve = gtk_object_get_data(GTK_OBJECT(ui_rendering->parameter_dialog), "density_curve");
  gradient_curve =gtk_object_get_data(GTK_OBJECT(ui_rendering->parameter_dialog), "gradient_curve");

  /* allocate some memory for the curve we're passing to the curve widget */
  if ((curve_points = 
       g_malloc(context->num_density_points*sizeof(gfloat)*2)) == NULL) {
    g_warning("%s: Failed to Allocate Memory for Density Ramp", PACKAGE);
    return;
  }
  /* copy rampx and rampy into the curve array */
  for (i=0; i< context->num_density_points; i++) {
    curve_points[i][0]=context->density_ramp_x[i];
    curve_points[i][1]=context->density_ramp_y[i];
  }

  /* doing some hackish stuff to get this to work as I'd like it (i.e. saving state) */
  GTK_CURVE(GTK_GAMMA_CURVE(density_curve)->curve)->num_ctlpoints =  context->num_density_points;
  g_free(GTK_CURVE(GTK_GAMMA_CURVE(density_curve)->curve)->ctlpoint);
  GTK_CURVE(GTK_GAMMA_CURVE(density_curve)->curve)->ctlpoint = curve_points;

  /* allocate some memory for the curve we're passing to the curve widget */
  if ((curve_points = 
       g_malloc(context->num_gradient_points*sizeof(gfloat)*2)) == NULL) {
    g_warning("%s: Failed to Allocate Memory for Gradient Ramp", PACKAGE);
    return;
  }
  /* copy rampx and rampy into the curve array */
  for (i=0; i< context->num_gradient_points; i++) {
    curve_points[i][0]=context->gradient_ramp_x[i];
    curve_points[i][1]=context->gradient_ramp_y[i];
  }

  /* doing some hackish stuff to get this to work as I'd like it (i.e. saving state) */
  GTK_CURVE(GTK_GAMMA_CURVE(gradient_curve)->curve)->num_ctlpoints =  context->num_gradient_points;
  g_free(GTK_CURVE(GTK_GAMMA_CURVE(gradient_curve)->curve)->ctlpoint);
  GTK_CURVE(GTK_GAMMA_CURVE(gradient_curve)->curve)->ctlpoint = curve_points;

  /* and get the two curves to redraw themselves */
  //  if (GTK_WIDGET_REALIZED(density_curve))
    gtk_curve_set_curve_type(GTK_CURVE(GTK_GAMMA_CURVE(density_curve)->curve), context->curve_type);
  //  if (GTK_WIDGET_REALIZED(gradient_curve))
    gtk_curve_set_curve_type(GTK_CURVE(GTK_GAMMA_CURVE(gradient_curve)->curve), context->curve_type);

    //    gtk_widget_queue_draw_area(density_curve, 0,0,200,200);
    //    gtk_signal_emit_by_name(GTK_OBJECT(density_curve), "configure_event", NULL);
    //    gtk_widget_queue_draw_area(gradient_curve, 0,0,200,200);
    //    gtk_signal_emit_by_name(GTK_OBJECT(gradient_curve), "configure_event", NULL);

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
  rendering_quality_t i_quality;
  pixel_type_t i_pixel_type;
  guint table_row = 0;
  rendering_list_t * temp_list;
  guint i;
  
  if (ui_rendering->parameter_dialog != NULL)
    return;
    
  temp_string = g_strdup_printf("%s: Rendering Parameters Dialog",PACKAGE);
  rendering_dialog = gnome_property_box_new();
  ui_rendering->parameter_dialog = rendering_dialog;
  gtk_window_set_title(GTK_WINDOW(rendering_dialog), temp_string);
  g_free(temp_string);

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(rendering_dialog), "close",
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_close_event),
		     ui_rendering);
  gtk_signal_connect(GTK_OBJECT(rendering_dialog), "apply",
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_apply),
		     ui_rendering);
  gtk_signal_connect(GTK_OBJECT(rendering_dialog), "help",
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_help),
		     ui_rendering);




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
     		       GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_change_quality), 
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
     		       GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_change_pixel_type), 
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
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_change_zoom), 
		     ui_rendering);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;



  /* ---------------------------
     Depth Cueing parameters
     --------------------------- */
  
  packing_table = gtk_table_new(4,2,FALSE);
  label = gtk_label_new("Depth Cueing");
  table_row=0;
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(rendering_dialog), 
				  GTK_WIDGET(packing_table), label);


  /* the depth cueing enabling button */
  check_button = gtk_check_button_new_with_label ("enable/disable depth cueing");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), ui_rendering->depth_cueing);
  gtk_signal_connect(GTK_OBJECT(check_button), "toggled", 
  		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_depth_cueing_toggle), 
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
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_change_front_factor), 
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
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_change_density), 
		     ui_rendering);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(entry),1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;




  /* ---------------------------
     Classification Parameters
     --------------------------- */
  
  packing_table = gtk_table_new(4,2,FALSE);
  label = gtk_label_new("Classification");
  table_row=0;
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(rendering_dialog), 
				  GTK_WIDGET(packing_table), label);

  /* widgets to change which volume where applying our changes to */
  label = gtk_label_new("Apply Changes To:");
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(label), 0,1,
		   table_row, table_row+1,
		   0, 0,
		   X_PADDING, Y_PADDING);

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();

  menuitem = gtk_menu_item_new_with_label("all rendering contexts");
  gtk_menu_append(GTK_MENU(menu), menuitem);
  i = 0;
  gtk_object_set_data(GTK_OBJECT(menuitem), "which", GINT_TO_POINTER(i)); 
  i++;
  gtk_object_set_data(GTK_OBJECT(menuitem), "rendering_dialog", rendering_dialog);
  gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_change_apply_to), 
		     ui_rendering);
  temp_list = ui_rendering->contexts;
  while (temp_list != NULL) {
    menuitem = gtk_menu_item_new_with_label(temp_list->rendering_context->volume->name);
    gtk_menu_append(GTK_MENU(menu), menuitem);
    gtk_object_set_data(GTK_OBJECT(menuitem), "which", GINT_TO_POINTER(i)); 
    i++;
    gtk_object_set_data(GTK_OBJECT(menuitem), "rendering_dialog", rendering_dialog);
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
     		       GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_change_apply_to), 
    		       ui_rendering);
    temp_list = temp_list->next;
  }
  
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), 0);
  gtk_object_set_data(GTK_OBJECT(rendering_dialog), "apply_to", 0);
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(option_menu), 1,2, 
		   table_row,table_row+1,
		   GTK_EXPAND | GTK_FILL, 0, 
		   X_PADDING, Y_PADDING);
  table_row++;


  /* ------- Density Dependent Opacity ---------- */
  label = gtk_label_new("Density\nDependent\nOpacity");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  
  gamma_curve = gtk_gamma_curve_new();
  gtk_curve_set_range(GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve), 0.0, 
		      RENDERING_DENSITY_MAX, 0.0, 1.0);

  /* set some data */
  gtk_object_set_data(GTK_OBJECT(rendering_dialog), "density_curve", gamma_curve);
  gtk_object_set_data(GTK_OBJECT(gamma_curve), "rendering_dialog", rendering_dialog);
  gtk_object_set_data(GTK_OBJECT(gamma_curve), "gamma_curve", gamma_curve);
  gtk_object_set_data(GTK_OBJECT(gamma_curve), "ui_rendering", ui_rendering);
  gtk_object_set_data(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->curve), 
		      "gamma_curve", gamma_curve);
  gtk_object_set_data(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->button[3]), 
		      "gamma_curve", gamma_curve);
  gtk_object_set_data(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->button[4]), 
		      "gamma_curve", gamma_curve);
    
  /* connect the signals together */
  gtk_signal_connect(GTK_OBJECT(gamma_curve), "button-release-event", 
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_density), 
		     NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->curve), "curve_type_changed", 
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_density), 
		     NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->button[3]), "clicked", 
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_density), 
		     NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->button[4]), "clicked", 
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_density), 
		     NULL);

  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(gamma_curve), 1,2,
		   table_row, table_row+1,
		   GTK_EXPAND | GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
  table_row++;


  /* ------- Gradient Dependent Opacity ---------- */
  label = gtk_label_new("Gradient\nDependent\nOpacity");
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(label), 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  
  gamma_curve = gtk_gamma_curve_new();
  gtk_curve_set_range(GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve), 0.0, 
		      RENDERING_GRADIENT_MAX, 0.0, 1.0);

  /* set some data */
  gtk_object_set_data(GTK_OBJECT(rendering_dialog), "gradient_curve", gamma_curve);
  gtk_object_set_data(GTK_OBJECT(gamma_curve), "rendering_dialog", rendering_dialog);
  gtk_object_set_data(GTK_OBJECT(gamma_curve), "gamma_curve", gamma_curve);
  gtk_object_set_data(GTK_OBJECT(gamma_curve), "ui_rendering", ui_rendering);
  gtk_object_set_data(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->curve), 
		      "gamma_curve", gamma_curve);
  gtk_object_set_data(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->button[3]), 
		      "gamma_curve", gamma_curve);
  gtk_object_set_data(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->button[4]), 
		      "gamma_curve", gamma_curve);
    
  /* connect the signals together */
  gtk_signal_connect(GTK_OBJECT(gamma_curve), "button-release-event", 
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_gradient), 
		     NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->curve), "curve_type_changed", 
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_gradient), 
		     NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->button[3]), "clicked", 
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_gradient), 
		     NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->button[4]), "clicked", 
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_gradient), 
		     NULL);

  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(gamma_curve), 1,2,
		   table_row, table_row+1, 
		   GTK_EXPAND | GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
  table_row++;



  /* and show all our widgets */
  gtk_widget_show_all(rendering_dialog);

  /* setup the density and gradient curves  */
  ui_rendering_dialog_update_curves(ui_rendering, 0);

  return;
}

#endif










