/* ui_render_dialog.c
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

#include "amide_config.h"

#ifdef AMIDE_LIBVOLPACK_SUPPORT

#include "amide.h"
#include "amide_gconf.h"
#include "ui_common.h"
#include "ui_render_dialog.h"
#include "amitk_color_table_menu.h"


#define GAMMA_CURVE_WIDTH -1 /* sets automatically */
#define GAMMA_CURVE_HEIGHT 100

static void change_quality_cb(GtkWidget * widget, gpointer data);
static void change_pixel_type_cb(GtkWidget * widget, gpointer data);
static void change_density_cb(GtkWidget * widget, gpointer data);
static void change_eye_angle_cb(GtkWidget * widget, gpointer data);
static void change_eye_width_cb(GtkWidget * widget, gpointer data);
static void depth_cueing_toggle_cb(GtkWidget * widget, gpointer data);
static void change_front_factor_cb(GtkWidget * widget, gpointer data);
static void color_table_cb(GtkWidget * widget, gpointer data);
static void change_opacity_cb(GtkWidget * widget, gpointer data);
static void p_response_cb (GtkDialog * dialog, gint response_id, gpointer data);
static void tf_response_cb (GtkDialog * dialog, gint response_id, gpointer data);
static gboolean p_delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data);
static gboolean tf_delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data);

static void setup_curve(GtkWidget * gamma_curve, gpointer data, classification_t type);



/* function to change  the rendering quality */
static void change_quality_cb(GtkWidget * widget, gpointer data) {

  ui_render_t * ui_render = data;
  rendering_quality_t new_quality;

  new_quality = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

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

  new_type = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

  if (rendering->pixel_type != new_type) {
    rendering->pixel_type = new_type;

    /* apply the new quality */
    rendering_set_image(rendering, rendering->pixel_type, ui_render->zoom);
    
    /* do updating */
    ui_render_add_update(ui_render);
  }
  
  return;
}



/* function to switch between click & drag versus click, drag, release */
static void update_without_release_toggle_cb(GtkWidget * widget, gpointer data) {
  
  ui_render_t * ui_render = data;
  gboolean update_without_release;

  update_without_release = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  
  if (ui_render->update_without_release != update_without_release) {
    ui_render->update_without_release = update_without_release;

    /* save user preferences */
    amide_gconf_set_bool(GCONF_AMIDE_RENDERING,"UpdateWithoutRelease", 
			 ui_render->update_without_release);
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
    amide_gconf_set_float(GCONF_AMIDE_RENDERING,"EyeAngle", 
			  ui_render->stereo_eye_angle);
    
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
    amide_gconf_set_int(GCONF_AMIDE_RENDERING,"EyeWidth", 
			ui_render->stereo_eye_width);
    
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

  i_color_table = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

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
      g_warning(_("couldn't allocate memory space for ramp x"));
      return;
    }
    if ((rendering->ramp_y[i_classification] = 
	 g_try_new(gfloat,rendering->num_points[i_classification])) == NULL) {
      g_warning(_("couldn't allocate memory space for ramp y"));
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



static void p_response_cb (GtkDialog * dialog, gint response_id, gpointer data) {
  
  gint return_val;

  switch(response_id) {
  case GTK_RESPONSE_HELP:
    amide_call_help("rendering-dialog");
    break;

  case GTK_RESPONSE_CLOSE:
    g_signal_emit_by_name(G_OBJECT(dialog), "delete_event", NULL, &return_val);
    if (!return_val) gtk_widget_destroy(GTK_WIDGET(dialog));
    break;

  default:
    break;
  }

  return;
}

static void tf_response_cb (GtkDialog * dialog, gint response_id, gpointer data) {
  
  gint return_val;

  switch(response_id) {
  case GTK_RESPONSE_HELP:
    amide_call_help("transfer-function-dialog");
    break;

  case GTK_RESPONSE_CLOSE:
    g_signal_emit_by_name(G_OBJECT(dialog), "delete_event", NULL, &return_val);
    if (!return_val) gtk_widget_destroy(GTK_WIDGET(dialog));
    break;

  default:
    break;
  }

  return;
}


static gboolean p_delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {
  ui_render_t * ui_render = data;
  ui_render->parameter_dialog = NULL;
  return FALSE;
}

static gboolean tf_delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {
  ui_render_t * ui_render = data;
  ui_render->transfer_function_dialog = NULL;
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
    g_warning(_("Failed to Allocate Memory for Ramp"));
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
void ui_render_dialog_create_parameters(ui_render_t * ui_render) {
  
  GtkWidget * dialog;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * label;
   GtkWidget * menu;
  GtkWidget * check_button;
  GtkWidget * spin_button;
  GtkWidget * hseparator;
  rendering_quality_t i_quality;
  guint table_row = 0;
  
  if (ui_render->parameter_dialog != NULL)
    return;
  temp_string = g_strdup_printf(_("%s: Rendering Parameters Dialog"),PACKAGE);
  dialog = gtk_dialog_new_with_buttons (temp_string,  ui_render->window,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_HELP, GTK_RESPONSE_HELP,
					GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					NULL);
  g_free(temp_string);
  ui_render->parameter_dialog = dialog;



  /* setup the callbacks for the dialog */
  g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(p_response_cb), ui_render);
  g_signal_connect(G_OBJECT(dialog), "delete_event", G_CALLBACK(p_delete_event_cb), ui_render);
		   


  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(4,2,FALSE);
  table_row=0;
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), packing_table);

  /* widgets to change the quality versus speed of rendering */
  label = gtk_label_new(_("Speed versus Quality"));
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  menu = gtk_combo_box_new_text();
  for (i_quality=0; i_quality<NUM_QUALITIES; i_quality++) 
    gtk_combo_box_append_text(GTK_COMBO_BOX(menu), _(rendering_quality_names[i_quality]));
  gtk_combo_box_set_active(GTK_COMBO_BOX(menu), ui_render->quality);
  g_signal_connect(G_OBJECT(menu), "changed", G_CALLBACK(change_quality_cb), ui_render);
  gtk_table_attach(GTK_TABLE(packing_table), menu, 1,2, 
		   table_row,table_row+1, GTK_EXPAND | GTK_FILL, 0, 
		   X_PADDING, Y_PADDING);
  table_row++;



  /* allow rendering to be click and drag */
  check_button = gtk_check_button_new_with_label (_("update without button release"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), ui_render->update_without_release);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(update_without_release_toggle_cb), ui_render);
  gtk_table_attach(GTK_TABLE(packing_table), check_button, 0,2, 
		   table_row,table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator,0,2,
		   table_row, table_row+1, GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
  table_row++;


  /* widget for the stereo eye angle */
  label = gtk_label_new(_("Stereo Angle"));
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  spin_button = gtk_spin_button_new_with_range(-90.0, 90.0, 0.2);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_button), FALSE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_button), 2);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), ui_render->stereo_eye_angle);
  g_signal_connect(G_OBJECT(spin_button), "value_changed", G_CALLBACK(change_eye_angle_cb), ui_render);
  gtk_table_attach(GTK_TABLE(packing_table), spin_button,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* widget for the stereo eye width */
  label = gtk_label_new(_("Eye Width (mm)"));
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  spin_button = gtk_spin_button_new_with_range(0, G_MAXDOUBLE, 1.0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_button), FALSE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_button), 2);
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
  check_button = gtk_check_button_new_with_label (_("enable/disable depth cueing"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), ui_render->depth_cueing);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(depth_cueing_toggle_cb), ui_render);
  gtk_table_attach(GTK_TABLE(packing_table), check_button, 0,2, 
		   table_row,table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  label = gtk_label_new(_("Front Factor"));
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  spin_button = gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 0.2);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_button), FALSE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_button), 2);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), ui_render->front_factor);
  g_signal_connect(G_OBJECT(spin_button), "value_changed", G_CALLBACK(change_front_factor_cb), ui_render);
  gtk_table_attach(GTK_TABLE(packing_table), spin_button,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  label = gtk_label_new(_("Density"));
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  spin_button = gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 0.2);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_button), FALSE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_button), 2);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), ui_render->density);
  g_signal_connect(G_OBJECT(spin_button), "value_changed", G_CALLBACK(change_density_cb), ui_render);
  gtk_table_attach(GTK_TABLE(packing_table), spin_button,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  /* and show all our widgets */
  gtk_widget_show_all(dialog);

  return;
}


/* function that sets up the rendering options dialog */
void ui_render_dialog_create_transfer_function(ui_render_t * ui_render) {
  
  GtkWidget * dialog;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * label;
  GtkWidget * menu;
  GtkWidget * gamma_curve[2];
  GtkWidget * button;
  GtkWidget * notebook;
  classification_t i_classification;
  GtkWidget * hseparator;
  pixel_type_t i_pixel_type;
  guint table_row = 0;
  renderings_t * temp_list;
  

  if (ui_render->transfer_function_dialog != NULL)
    return;
  temp_string = g_strdup_printf(_("%s: Transfer Function Dialog"),PACKAGE);
  dialog = gtk_dialog_new_with_buttons (temp_string,  ui_render->window,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_HELP, GTK_RESPONSE_HELP,
					GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					NULL);
  g_free(temp_string);
  ui_render->transfer_function_dialog = dialog;

  /* setup the callbacks for the dialog */
  g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(tf_response_cb), ui_render);
  g_signal_connect(G_OBJECT(dialog), "delete_event", G_CALLBACK(tf_delete_event_cb), ui_render);
		   

  notebook = gtk_notebook_new();
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), notebook);


  temp_list = ui_render->renderings;
  while (temp_list != NULL) {
    packing_table = gtk_table_new(4,3,FALSE);
    table_row=0;
    label = gtk_label_new(temp_list->rendering->name);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), packing_table, label);

    /* widgets to change the returned pixel type of the rendering */
    label = gtk_label_new(_("Return Type"));
    gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

    menu = gtk_combo_box_new_text();
    for (i_pixel_type=0; i_pixel_type<NUM_PIXEL_TYPES; i_pixel_type++) 
      gtk_combo_box_append_text(GTK_COMBO_BOX(menu), _(pixel_type_names[i_pixel_type]));
    g_object_set_data(G_OBJECT(menu), "ui_render", ui_render);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu), temp_list->rendering->pixel_type);
    g_signal_connect(G_OBJECT(menu), "changed", G_CALLBACK(change_pixel_type_cb), temp_list->rendering);
    gtk_table_attach(GTK_TABLE(packing_table), menu, 1,2, 
		     table_row,table_row+1, GTK_EXPAND | GTK_FILL, 0, X_PADDING, Y_PADDING);
    table_row++;

    /* color table selector */
    label = gtk_label_new(_("color table:"));
    gtk_table_attach(GTK_TABLE(packing_table), label, 0,1, table_row,table_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    menu = amitk_color_table_menu_new();
    g_object_set_data(G_OBJECT(menu), "ui_render", ui_render);
    gtk_table_attach(GTK_TABLE(packing_table), menu, 1,2, table_row,table_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu),temp_list->rendering->color_table);
    g_signal_connect(G_OBJECT(menu), "changed", G_CALLBACK(color_table_cb), 
		     temp_list->rendering);
    gtk_widget_show(menu);

    table_row++;


    /* a separator for clarity */
    hseparator = gtk_hseparator_new();
    gtk_table_attach(GTK_TABLE(packing_table), hseparator,0,2,
		     table_row, table_row+1, GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
    table_row++;

    /* widgets for changing the density and gradient classification parameters */
    for (i_classification = 0; i_classification < NUM_CLASSIFICATIONS; i_classification++) {

      if (i_classification == DENSITY_CLASSIFICATION)
	label = gtk_label_new(_("Density\nDependent\nOpacity"));
      else /* gradient classification*/
	label = gtk_label_new(_("Gradient\nDependent\nOpacity"));
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
    button = gtk_button_new_with_label(_("Apply Curve Changes"));
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
  gtk_widget_show_all(dialog);

  return;
}

#endif










