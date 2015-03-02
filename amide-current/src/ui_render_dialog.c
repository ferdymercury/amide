/* ui_render_dialog.c
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

#ifdef AMIDE_LIBVOLPACK_SUPPORT


#include <gnome.h>
#include "ui_common.h"
#include "ui_render_dialog.h"


#define GAMMA_CURVE_WIDTH -1 /* sets automatically */
#define GAMMA_CURVE_HEIGHT 100
#define DIALOG_SPIN_BUTTON_DIGITS 3

static void change_quality_cb(GtkWidget * widget, gpointer data);
static void change_pixel_type_cb(GtkWidget * widget, gpointer data);
static void change_density_cb(GtkWidget * widget, gpointer data);
static void change_zoom_cb(GtkWidget * widget, gpointer data);
static void change_eye_angle_cb(GtkWidget * widget, gpointer data);
static void change_eye_width_cb(GtkWidget * widget, gpointer data);
static void depth_cueing_toggle_cb(GtkWidget * widget, gpointer data);
static void change_front_factor_cb(GtkWidget * widget, gpointer data);
static void color_table_cb(GtkWidget * widget, gpointer data);
static void change_opacity_cb(GtkWidget * widget, gpointer data);
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data);

static void setup_curve(GtkWidget * gamma_curve, gpointer data, classification_t type);



/* function to change  the rendering quality */
static void change_quality_cb(GtkWidget * widget, gpointer data) {

  ui_render_t * ui_render = data;
  rendering_quality_t new_quality;

  new_quality = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));

  if (ui_render->quality != new_quality) {
    ui_render->quality = new_quality;

    /* apply the new quality */
    renderings_set_quality(ui_render->renderings, ui_render->quality);
    
    /* do updating */
    ui_render_add_update(ui_render);
  }

  return;
}



/* function to change the return pixel type  */
static void change_pixel_type_cb(GtkWidget * widget, gpointer data) {

  ui_render_t * ui_render;
  rendering_t * rendering = data;
  pixel_type_t new_type;

  ui_render = g_object_get_data(G_OBJECT(widget), "ui_render");

  new_type = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));

  if (rendering->pixel_type != new_type) {
    rendering->pixel_type = new_type;

    /* apply the new quality */
    rendering_set_image(rendering, rendering->pixel_type, ui_render->zoom);
    
    /* do updating */
    ui_render_add_update(ui_render);
  }
  
  return;
}



/* function to change the zoom */
static void change_zoom_cb(GtkWidget * widget, gpointer data) {

  ui_render_t * ui_render = data;
  gdouble temp_val;

  temp_val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

  if (temp_val < 0.1)
    return;
  if (temp_val > 10) /* 10x zoom seems like quite a bit... */
    return;

  /* set the zoom */
  if (!REAL_EQUAL(ui_render->zoom, temp_val)) {
    ui_render->zoom = temp_val;
    renderings_set_zoom(ui_render->renderings, ui_render->zoom);
    
    /* do updating */
    ui_render_add_update(ui_render); 
  }

  return;
}


/* function to change the stereo eye angle */
static void change_eye_angle_cb(GtkWidget * widget, gpointer data) {

  ui_render_t * ui_render = data;
  gdouble temp_val;

  temp_val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

  if (temp_val > 90) /* 90 degrees seems like quite a bit... */
    return;

  if (!REAL_EQUAL(ui_render->stereo_eye_angle, temp_val)) {
    ui_render->stereo_eye_angle = temp_val;
    
    /* save user preferences */
    gnome_config_push_prefix("/"PACKAGE"/");
    gnome_config_set_float("RENDERING/EyeAngle", ui_render->stereo_eye_angle);
    gnome_config_pop_prefix();
    gnome_config_sync();
    
    /* do updating */
    ui_render_add_update(ui_render); 
  }

  return;
}

/* function to change the distance between stereo image pairs */
static void change_eye_width_cb(GtkWidget * widget, gpointer data) {

  ui_render_t * ui_render = data;
  gdouble temp_val;

  temp_val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

  temp_val = temp_val*gdk_screen_width()/gdk_screen_width_mm();
  if (temp_val < 0) /* weird mutants? */
    return;
  if (temp_val > 1000) /* just plain wrong? */
    return;

  if (!REAL_EQUAL(ui_render->stereo_eye_width, temp_val)) {

    ui_render->stereo_eye_width = temp_val;
    
    /* save user preferences */
    gnome_config_push_prefix("/"PACKAGE"/");
    gnome_config_set_int("RENDERING/EyeWidth", ui_render->stereo_eye_width);
    gnome_config_pop_prefix();
    gnome_config_sync();
    
    /* do updating */
    ui_render_add_update(ui_render); 
  }

  return;
}


/* function to enable/disable depth cueing */
static void depth_cueing_toggle_cb(GtkWidget * widget, gpointer data) {
  
  ui_render_t * ui_render = data;
  gboolean depth_cueing;

  depth_cueing = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  
  if (ui_render->depth_cueing != depth_cueing) {
    ui_render->depth_cueing = depth_cueing;

    /* apply the new quality */
    renderings_set_depth_cueing(ui_render->renderings, ui_render->depth_cueing);
    
    ui_render_add_update(ui_render);
  }

  return;
}
  



/* function to change the front factor on depth cueing */
static void change_front_factor_cb(GtkWidget * widget, gpointer data) {

  ui_render_t * ui_render = data;
  gdouble temp_val;

  temp_val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

  if (!REAL_EQUAL(ui_render->front_factor, temp_val)) {

    /* set the front factor */
    ui_render->front_factor = temp_val;

    renderings_set_depth_cueing_parameters(ui_render->renderings,
					   ui_render->front_factor,
					   ui_render->density);
    
    ui_render_add_update(ui_render); 
  }

  return;
}



/* function to change the density parameter on depth cueing */
static void change_density_cb(GtkWidget * widget, gpointer data) {

  ui_render_t * ui_render = data;
  gdouble temp_val;

  temp_val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

  if (!REAL_EQUAL(ui_render->density, temp_val)) {
  
    ui_render->density = temp_val; /* set the density */

    renderings_set_depth_cueing_parameters(ui_render->renderings,
					   ui_render->front_factor,
					   ui_render->density);
    
    ui_render_add_update(ui_render); 
  }

  return;
}

/* changing the color table of a rendering context */
static void color_table_cb(GtkWidget * widget, gpointer data) {

  ui_render_t * ui_render ;
  rendering_t * rendering = data;
  AmitkColorTable i_color_table;

  ui_render = g_object_get_data(G_OBJECT(widget), "ui_render");

  i_color_table = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));

  if (rendering->color_table != i_color_table) {
    /* set the color table */
    rendering->color_table = i_color_table;
  
    ui_render_add_update(ui_render); 
  }

  return;
}


/* function called to change the opacity density or gradient for classification */
static void change_opacity_cb(GtkWidget * widget, gpointer data) {

  ui_render_t * ui_render;
  GtkWidget * gamma_curve[2];
  gint i;
  guint num_points;
  rendering_t * rendering=data;
  curve_type_t curve_type;
  classification_t i_classification;

  g_return_if_fail(rendering!=NULL);

  /* get some pointers */
  gamma_curve[DENSITY_CLASSIFICATION] =  g_object_get_data(G_OBJECT(widget), "gamma_curve_density");
  gamma_curve[GRADIENT_CLASSIFICATION] =  g_object_get_data(G_OBJECT(widget), "gamma_curve_gradient");
  ui_render = g_object_get_data(G_OBJECT(widget), "ui_render");

  for (i_classification = 0; i_classification < NUM_CLASSIFICATIONS; i_classification++) {
    /* figure out the curve type */
    switch (GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve[i_classification])->curve)->curve_type) {
    case GTK_CURVE_TYPE_SPLINE:
      curve_type = CURVE_SPLINE;
      num_points = GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve[i_classification])->curve)->num_ctlpoints; 
      break;
    case GTK_CURVE_TYPE_LINEAR:       
    default:
      curve_type = CURVE_LINEAR;
      num_points = GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve[i_classification])->curve)->num_ctlpoints; 
      break;
    }

    /* set the ramp table to what's in the curve widget */
    gtk_curve_get_vector(GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve[i_classification])->curve), 
			 (i_classification == DENSITY_CLASSIFICATION) ?  RENDERING_DENSITY_MAX:RENDERING_GRADIENT_MAX,
			 (i_classification == DENSITY_CLASSIFICATION) ? rendering->density_ramp : rendering->gradient_ramp);   
    
    /* store the control points on the widget for later use */
    g_free(rendering->ramp_x[i_classification]); /* free the old stuff */
    g_free(rendering->ramp_y[i_classification]); /* free the old stuff */
    rendering->num_points[i_classification] =  num_points;
    rendering->curve_type[i_classification] = curve_type;
    
    /* allocate some new memory */
    if ((rendering->ramp_x[i_classification] = 
	 g_try_new(gint,rendering->num_points[i_classification])) == NULL) {
      g_warning("couldn't allocate space for ramp x");
      return;
    }
    if ((rendering->ramp_y[i_classification] = 
	 g_try_new(gfloat,rendering->num_points[i_classification])) == NULL) {
      g_warning("couldn't allocate space for ramp y");
      return;
    }
    
    /* copy the new ctrl points */
    for (i=0;i<rendering->num_points[i_classification];i++) {
      rendering->ramp_x[i_classification][i] = GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve[i_classification])->curve)->ctlpoint[i][0];
      rendering->ramp_y[i_classification][i] = GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve[i_classification])->curve)->ctlpoint[i][1];
    }

    rendering->need_reclassify = TRUE;
    rendering->need_rerender = TRUE;
  }

  ui_render_add_update(ui_render); 

  return;
}




/* callback for the help button */
/*
static void help_cb(GnomePropertyBox *rendering_dialog, gint page_number, gpointer data) {

  GError *err=NULL;

  switch (page_number) {
  case 0:
    gnome_help_display (PACKAGE, "rendering-dialog-help.html#RENDERING-DIALOG-HELP-DATA-SET", &err);
    break;
  case 1:
    gnome_help_display (PACKAGE, "rendering-dialog-help.html#RENDERING-DIALOG-HELP-BASIC", &err);
    break;
  default:
    gnome_help_display (PACKAGE, "rendering-dialog-help.html#RENDERING-DIALOG-HELP", &err);
    break;
  }

  if (err != NULL) {
    g_warning("couldn't open help file, error: %s", err->message);
    g_error_free(err);
  }

  return;
}
*/


/* function called to destroy the rendering parameter dialog */
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_render_t * ui_render = data;

  g_print("parameter dialog delete\n");
  ui_render->parameter_dialog = NULL;

  return FALSE;
}




/* function to set what's in the density or gradient opacity curves when it gets realized */
static void setup_curve(GtkWidget * gamma_curve, gpointer data, classification_t type) {

  GtkWidget * curve;
  rendering_t * rendering = data;
  gfloat (*curve_ctrl_points)[2];
  guint i;

  curve = GTK_GAMMA_CURVE(gamma_curve)->curve;

  /* allocate some memory for the curve we're passing to the curve widget */
  if ((curve_ctrl_points = g_try_malloc(rendering->num_points[type]*sizeof(gfloat)*2)) == NULL) {
    g_warning("Failed to Allocate Memory for Ramp");
    return;
  }
  
  /* copy rampx and rampy into the curve array  */
  for (i=0; i< rendering->num_points[type]; i++) {
    curve_ctrl_points[i][0]=rendering->ramp_x[type][i];
    curve_ctrl_points[i][1]=rendering->ramp_y[type][i];
  }
  
  GTK_CURVE(curve)->num_ctlpoints =  rendering->num_points[type];
  g_free(GTK_CURVE(curve)->ctlpoint);
  GTK_CURVE(curve)->ctlpoint = curve_ctrl_points;
  
  /* doing some hackish stuff to get this to work as I'd like it (i.e. saving state) */
  switch (rendering->curve_type[type]) {
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
void ui_render_dialog_create(ui_render_t * ui_render) {
  
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * label;
  GtkWidget * option_menu;
  GtkWidget * menu;
  GtkWidget * menuitem;
  GtkWidget * check_button;
  GtkWidget * spin_button;
  GtkWidget * gamma_curve[2];
  GtkWidget * button;
  GtkWidget * notebook;
  classification_t i_classification;
  GtkWidget * hseparator;
  rendering_quality_t i_quality;
  pixel_type_t i_pixel_type;
  guint table_row = 0;
  renderings_t * temp_list;
  AmitkColorTable i_color_table;
  
  if (ui_render->parameter_dialog != NULL)
    return;
  temp_string = g_strdup_printf("%s: Rendering Parameters Dialog",PACKAGE);
  ui_render->parameter_dialog = 
    gtk_dialog_new_with_buttons (temp_string,  GTK_WINDOW(ui_render->app),
				 GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
				 NULL);
  g_free(temp_string);

  /* setup the callbacks for the dialog */
  g_signal_connect(G_OBJECT(ui_render->parameter_dialog), "delete_event", 
		   G_CALLBACK(delete_event_cb), ui_render);
		   

  notebook = gtk_notebook_new();
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(ui_render->parameter_dialog)->vbox), notebook);



  /* ---------------------------
     Basic Parameters page 
     --------------------------- */


  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(4,2,FALSE);
  label = gtk_label_new("Basic Parameters");
  table_row=0;
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), packing_table, label);

  /* widgets to change the quality versus speed of rendering */
  label = gtk_label_new("Speed versus Quality");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();

  for (i_quality=0; i_quality<NUM_QUALITIES; i_quality++) {
    menuitem = gtk_menu_item_new_with_label(rendering_quality_names[i_quality]);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    g_object_set_data(G_OBJECT(menuitem), "quality", GINT_TO_POINTER(i_quality)); 
  }
  
  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), ui_render->quality);
  g_signal_connect(G_OBJECT(option_menu), "changed", G_CALLBACK(change_quality_cb), ui_render);
  gtk_table_attach(GTK_TABLE(packing_table), option_menu, 1,2, 
		   table_row,table_row+1, GTK_EXPAND | GTK_FILL, 0, 
		   X_PADDING, Y_PADDING);
  table_row++;



  /* widgets to change the zoom */
  label = gtk_label_new("Zoom");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  spin_button = gtk_spin_button_new_with_range(0.1, 10.0, 0.2);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_button), DIALOG_SPIN_BUTTON_DIGITS);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), ui_render->zoom);
  g_signal_connect(G_OBJECT(spin_button), "value_changed", G_CALLBACK(change_zoom_cb), ui_render);
  gtk_table_attach(GTK_TABLE(packing_table), spin_button,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator,0,2,
		   table_row, table_row+1, GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
  table_row++;

  /* widget for the stereo eye angle */
  label = gtk_label_new("Stereo Angle");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  spin_button = gtk_spin_button_new_with_range(-90.0, 90.0, 0.2);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_button), DIALOG_SPIN_BUTTON_DIGITS);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), ui_render->stereo_eye_angle);
  g_signal_connect(G_OBJECT(spin_button), "value_changed", G_CALLBACK(change_eye_angle_cb), ui_render);
  gtk_table_attach(GTK_TABLE(packing_table), spin_button,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* widget for the stereo eye width */
  label = gtk_label_new("Eye Width (mm)");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  spin_button = gtk_spin_button_new_with_range(0, G_MAXDOUBLE, 1.0);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_button), DIALOG_SPIN_BUTTON_DIGITS);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), 
			    gdk_screen_width_mm()*ui_render->stereo_eye_width/
			    ((gdouble) gdk_screen_width()));
  g_signal_connect(G_OBJECT(spin_button), "value_changed", G_CALLBACK(change_eye_width_cb), ui_render);
  gtk_table_attach(GTK_TABLE(packing_table), spin_button,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator,0,2,
		   table_row, table_row+1, GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
  table_row++;

  /* the depth cueing enabling button */
  check_button = gtk_check_button_new_with_label ("enable/disable depth cueing");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), ui_render->depth_cueing);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(depth_cueing_toggle_cb), ui_render);
  gtk_table_attach(GTK_TABLE(packing_table), check_button, 0,2, 
		   table_row,table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  label = gtk_label_new("Front Factor");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  spin_button = gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 0.2);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_button), DIALOG_SPIN_BUTTON_DIGITS);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), ui_render->front_factor);
  g_signal_connect(G_OBJECT(spin_button), "value_changed", G_CALLBACK(change_front_factor_cb), ui_render);
  gtk_table_attach(GTK_TABLE(packing_table), spin_button,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  label = gtk_label_new("Density");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  spin_button = gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 0.2);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_button), DIALOG_SPIN_BUTTON_DIGITS);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), ui_render->density);
  g_signal_connect(G_OBJECT(spin_button), "value_changed", G_CALLBACK(change_density_cb), ui_render);
  gtk_table_attach(GTK_TABLE(packing_table), spin_button,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;




  /* ---------------------------
     Volume Specific Stuff
     --------------------------- */

  temp_list = ui_render->renderings;
  while (temp_list != NULL) {
    packing_table = gtk_table_new(4,3,FALSE);
    table_row=0;
    label = gtk_label_new(temp_list->rendering->name);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), packing_table, label);

    /* widgets to change the returned pixel type of the rendering */
    label = gtk_label_new("Return Type");
    gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

    option_menu = gtk_option_menu_new();
    menu = gtk_menu_new();
    for (i_pixel_type=0; i_pixel_type<NUM_PIXEL_TYPES; i_pixel_type++) {
      menuitem = gtk_menu_item_new_with_label(pixel_type_names[i_pixel_type]);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    }
  
    gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
    g_object_set_data(G_OBJECT(option_menu), "ui_render", ui_render);
    gtk_table_attach(GTK_TABLE(packing_table), option_menu, 1,2, 
		     table_row,table_row+1, GTK_EXPAND | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), temp_list->rendering->pixel_type);
    g_signal_connect(G_OBJECT(option_menu), "changed", G_CALLBACK(change_pixel_type_cb), temp_list->rendering);
    table_row++;

    /* color table selector */
    label = gtk_label_new("color table:");
    gtk_table_attach(GTK_TABLE(packing_table), label, 0,1, table_row,table_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    option_menu = gtk_option_menu_new();
    menu = gtk_menu_new();
    for (i_color_table=0; i_color_table<AMITK_COLOR_TABLE_NUM; i_color_table++) {
      menuitem = gtk_menu_item_new_with_label(color_table_menu_names[i_color_table]);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
    g_object_set_data(G_OBJECT(option_menu), "ui_render", ui_render);
    gtk_table_attach(GTK_TABLE(packing_table), option_menu, 1,2, table_row,table_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu),
				temp_list->rendering->color_table);
    g_signal_connect(G_OBJECT(option_menu), "changed", G_CALLBACK(color_table_cb), 
		     temp_list->rendering);
    gtk_widget_show_all(option_menu);

    table_row++;


    /* a separator for clarity */
    hseparator = gtk_hseparator_new();
    gtk_table_attach(GTK_TABLE(packing_table), hseparator,0,2,
		     table_row, table_row+1, GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
    table_row++;

    /* widgets for changing the density and gradient classification parameters */
    for (i_classification = 0; i_classification < NUM_CLASSIFICATIONS; i_classification++) {

      if (i_classification == DENSITY_CLASSIFICATION)
	label = gtk_label_new("Density\nDependent\nOpacity");
      else /* gradient classification*/
	label = gtk_label_new("Gradient\nDependent\nOpacity");
      gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  
      gamma_curve[i_classification] = gtk_gamma_curve_new();
      gtk_widget_set_size_request(gamma_curve[i_classification], GAMMA_CURVE_WIDTH, GAMMA_CURVE_HEIGHT);
      gtk_curve_set_range(GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve[i_classification])->curve), 0.0, 
			  (i_classification == DENSITY_CLASSIFICATION) ? RENDERING_DENSITY_MAX: RENDERING_GRADIENT_MAX, 
			  0.0, 1.0);

      setup_curve(gamma_curve[i_classification], temp_list->rendering, i_classification);
      /* disable the gamma button and free drawing button */
      gtk_widget_destroy(GTK_GAMMA_CURVE(gamma_curve[i_classification])->button[3]);
      gtk_widget_destroy(GTK_GAMMA_CURVE(gamma_curve[i_classification])->button[2]);
    
      /* and attach */
      gtk_table_attach(GTK_TABLE(packing_table), gamma_curve[i_classification], 1,3,
		       table_row, table_row+1,
		       GTK_EXPAND | GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
      table_row++;
    }

    /* GTK no longer has a way to detect automatically when the GtkCurve has been changed,
       user will now have to explicitly change */
    button = gtk_button_new_with_label("Apply Curve Changes");
    g_object_set_data(G_OBJECT(button), "gamma_curve_density", gamma_curve[DENSITY_CLASSIFICATION]);
    g_object_set_data(G_OBJECT(button), "gamma_curve_gradient", gamma_curve[GRADIENT_CLASSIFICATION]);
    g_object_set_data(G_OBJECT(button), "ui_render", ui_render);
    /* and attach */
    gtk_table_attach(GTK_TABLE(packing_table), button, 1,3, table_row, table_row+1, 
		     GTK_EXPAND | GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
    gtk_widget_show(button);
    table_row++;
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(change_opacity_cb), 
		     temp_list->rendering);

    temp_list=temp_list->next;
  }


  /* and show all our widgets */
  gtk_widget_show_all(ui_render->parameter_dialog);

  return;
}

#endif










