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
#include "rendering.h"
#include "study.h"
#include "image.h"
#include "ui_rendering.h"
#include "ui_rendering_dialog.h"
#include "ui_rendering_dialog_callbacks.h"


/* function to update what's in the density and gradient opacity curves */
void ui_rendering_dialog_update_curves(ui_rendering_t * ui_rendering, guint which_context) {

  rendering_list_t * temp_list=ui_rendering->contexts;
  rendering_t * context;
  GtkWidget * density_curve;
  GtkWidget * gradient_curve;
  gfloat (*curve_points)[2];
  guint i;

  /* figure out which context */
  if (which_context == 0) /* we'll just use the first context if we're applying to all */
    context = temp_list->rendering_context;
  else {
    which_context--;
    while (which_context != 0) {
      temp_list = temp_list->next;
      which_context--;
    }
    context = temp_list->rendering_context;
  }

  /* allocate some memory for the curve we're passing to the curve widget */
  if ((curve_points = g_malloc(context->num_density_points*sizeof(gfloat)*2)) == NULL) {
    g_warning("%s: Failed to Allocate Memory for Density Ramp", PACKAGE);
    return;
  }

  /* copy rampx and rampy into the curve array for density */
  for (i=0; i< context->num_density_points; i++) {
    curve_points[i][0]=context->density_ramp_x[i];
    curve_points[i][1]=context->density_ramp_y[i];
  }

  /* doing some hackish stuff to get this to work as I'd like it (i.e. saving state) */
  density_curve = gtk_object_get_data(GTK_OBJECT(ui_rendering->parameter_dialog), "density_curve");
  GTK_CURVE(GTK_GAMMA_CURVE(density_curve)->curve)->num_ctlpoints =  context->num_density_points;
  g_free(GTK_CURVE(GTK_GAMMA_CURVE(density_curve)->curve)->ctlpoint);
  GTK_CURVE(GTK_GAMMA_CURVE(density_curve)->curve)->ctlpoint = curve_points;

  /* don't want to inadvertently trigger the following functions */
  gtk_signal_handler_block_by_func(GTK_OBJECT(density_curve), 
				   GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_density), NULL);
  gtk_signal_handler_block_by_func(GTK_OBJECT(GTK_GAMMA_CURVE(density_curve)->curve), 
  				   GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_density), NULL);

  /* get the curve to redraw itself */
  switch (context->density_curve_type) {
  case CURVE_LINEAR:
    gtk_curve_set_curve_type(GTK_CURVE(GTK_GAMMA_CURVE(density_curve)->curve), GTK_CURVE_TYPE_LINEAR);
    break;
  case CURVE_SPLINE:
    gtk_curve_set_curve_type(GTK_CURVE(GTK_GAMMA_CURVE(density_curve)->curve), GTK_CURVE_TYPE_SPLINE);
    break;
  case CURVE_FREE:    
  default:
    gtk_curve_set_curve_type(GTK_CURVE(GTK_GAMMA_CURVE(density_curve)->curve), GTK_CURVE_TYPE_FREE);
    break;
  }


  /* and yes, it takes all of this to get the gamma curves to redraw correctly */
  gtk_signal_emit_by_name(GTK_OBJECT(GTK_GAMMA_CURVE(density_curve)->curve), "curve_type_changed", NULL); 
  gtk_signal_emit_by_name(GTK_OBJECT(GTK_GAMMA_CURVE(density_curve)->curve), "configure_event", NULL);
  gtk_widget_queue_draw(GTK_WIDGET(density_curve));

  /* and unblock the following functions */
  gtk_signal_handler_unblock_by_func(GTK_OBJECT(density_curve), 
				     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_density), NULL);
  gtk_signal_handler_unblock_by_func(GTK_OBJECT(GTK_GAMMA_CURVE(density_curve)->curve), 
  				     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_density), NULL);





  /* allocate some memory for the curve we're passing to the curve widget */
  if ((curve_points =  g_malloc(context->num_gradient_points*sizeof(gfloat)*2)) == NULL) {
    g_warning("%s: Failed to Allocate Memory for Gradient Ramp", PACKAGE);
    return;
  }
  /* copy rampx and rampy into the curve array for gradient */
  for (i=0; i< context->num_gradient_points; i++) {
    curve_points[i][0]=context->gradient_ramp_x[i];
    curve_points[i][1]=context->gradient_ramp_y[i];
  }

  /* doing some hackish stuff to get this to work as I'd like it (i.e. saving state) */
  gradient_curve = gtk_object_get_data(GTK_OBJECT(ui_rendering->parameter_dialog), "gradient_curve");
  GTK_CURVE(GTK_GAMMA_CURVE(gradient_curve)->curve)->num_ctlpoints =  context->num_gradient_points;
  g_free(GTK_CURVE(GTK_GAMMA_CURVE(gradient_curve)->curve)->ctlpoint);
  GTK_CURVE(GTK_GAMMA_CURVE(gradient_curve)->curve)->ctlpoint = curve_points;

  /* don't want to inadvertently trigger the following functions */
  gtk_signal_handler_block_by_func(GTK_OBJECT(gradient_curve), 
				   GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_gradient), NULL);
  gtk_signal_handler_block_by_func(GTK_OBJECT(GTK_GAMMA_CURVE(gradient_curve)->curve), 
				   GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_gradient), NULL);

  /* get the curve to redraw itself */
  switch (context->gradient_curve_type) {
  case CURVE_LINEAR:
    gtk_curve_set_curve_type(GTK_CURVE(GTK_GAMMA_CURVE(gradient_curve)->curve), GTK_CURVE_TYPE_LINEAR);
    break;
  case CURVE_SPLINE:
    gtk_curve_set_curve_type(GTK_CURVE(GTK_GAMMA_CURVE(gradient_curve)->curve), GTK_CURVE_TYPE_SPLINE);
    break;
  case CURVE_FREE:    
  default:
    gtk_curve_set_curve_type(GTK_CURVE(GTK_GAMMA_CURVE(gradient_curve)->curve), GTK_CURVE_TYPE_FREE);
    break;
  }

  /* and yes, it takes all of this to get the gamma curves to redraw correctly */
  gtk_signal_emit_by_name(GTK_OBJECT(GTK_GAMMA_CURVE(gradient_curve)->curve), "curve_type_changed", NULL); 
  gtk_signal_emit_by_name(GTK_OBJECT(GTK_GAMMA_CURVE(gradient_curve)->curve), "configure_event", NULL);
  gtk_widget_queue_draw(GTK_WIDGET(gradient_curve));

  /* and unblock the following functions */
  gtk_signal_handler_unblock_by_func(GTK_OBJECT(gradient_curve), 
				     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_gradient), NULL);
  gtk_signal_handler_unblock_by_func(GTK_OBJECT(GTK_GAMMA_CURVE(gradient_curve)->curve), 
				     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_gradient), NULL);

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

  /* not operating as a true property box dialog, i.e. no cancel option */
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
  gtk_object_set_data(GTK_OBJECT(menuitem), "which_context", GINT_TO_POINTER(i)); 
  i++;
  gtk_object_set_data(GTK_OBJECT(menuitem), "rendering_dialog", rendering_dialog);
  gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_change_apply_to), 
		     ui_rendering);
  temp_list = ui_rendering->contexts;
  while (temp_list != NULL) {
    menuitem = gtk_menu_item_new_with_label(temp_list->rendering_context->volume->name);
    gtk_menu_append(GTK_MENU(menu), menuitem);
    gtk_object_set_data(GTK_OBJECT(menuitem), "which_context", GINT_TO_POINTER(i)); 
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
  gtk_object_set_data(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->curve), "gamma_curve", gamma_curve);
    
  /* connect the signals together */
  gtk_signal_connect(GTK_OBJECT(gamma_curve), "button-release-event", 
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_density), NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->curve), "curve_type_changed", 
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_density), NULL);
  /* disable the gamma button */
  gtk_signal_handlers_destroy (GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->button[3]));

  /* and attach */
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
  gtk_object_set_data(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->curve), "gamma_curve", gamma_curve);
    
  /* connect the signals together */
  gtk_signal_connect(GTK_OBJECT(gamma_curve), "button-release-event", 
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_gradient), NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->curve), "curve_type_changed", 
		     GTK_SIGNAL_FUNC(ui_rendering_dialog_callbacks_opacity_gradient), NULL);

  /* disable the gamma button */
  gtk_signal_handlers_destroy (GTK_OBJECT(GTK_GAMMA_CURVE(gamma_curve)->button[3]));

  /* and attach */
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(gamma_curve), 1,2,
		   table_row, table_row+1, 
		   GTK_EXPAND | GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
  gtk_widget_show(gamma_curve);
  table_row++;



  /* and show all our widgets */
  gtk_widget_show_all(rendering_dialog);

  /* setup the density and gradient curves  */
  ui_rendering_dialog_update_curves(ui_rendering, 0);

  return;
}

#endif










