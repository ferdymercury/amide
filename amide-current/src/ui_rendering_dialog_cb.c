/* ui_rendering_dialog_cb.c
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
#include "ui_common.h"
#include "ui_rendering_dialog.h"
#include "ui_rendering_dialog_cb.h"





/* function to change  the rendering quality */
void ui_rendering_dialog_cb_change_quality(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;
  GtkWidget * rendering_dialog;

  ui_rendering->quality = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"quality"));

  /* apply the new quality */
  rendering_list_set_quality(ui_rendering->contexts, ui_rendering->quality);

  /* do updating */
  if (ui_rendering->immediate) {
    ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_rendering->canvas));
    ui_rendering_update_canvases(ui_rendering); 
    ui_common_remove_cursor(GTK_WIDGET(ui_rendering->canvas));
  }  else { /* otherwise, tell the dialog we've changed */
    rendering_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "rendering_dialog");
    gnome_property_box_changed(GNOME_PROPERTY_BOX(rendering_dialog));
  }

  return;
}



/* function to change the return pixel type  */
void ui_rendering_dialog_cb_change_pixel_type(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;
  GtkWidget * rendering_dialog;

  ui_rendering->pixel_type = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"pixel_type"));

  /* apply the new quality */
  rendering_list_set_image(ui_rendering->contexts, 
			   ui_rendering->pixel_type,
			   ui_rendering->zoom);

  /* do updating */
  if (ui_rendering->immediate) {
    ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_rendering->canvas));
    ui_rendering_update_canvases(ui_rendering); 
    ui_common_remove_cursor(GTK_WIDGET(ui_rendering->canvas));
  } else { /* otherwise, tell the dialog we've changed */
    rendering_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "rendering_dialog");
    gnome_property_box_changed(GNOME_PROPERTY_BOX(rendering_dialog));
  }

  return;
}



/* function to change the zoom */
void ui_rendering_dialog_cb_change_zoom(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;
  GtkWidget * rendering_dialog;
  gchar * str;
  gint error;
  gdouble temp_val;

  /* get the contents of the name entry box */
  str = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert to a floating point */
  error = sscanf(str, "%lf", &temp_val);
  g_free(str);

  if (error == EOF)  /* make sure it's a valid number */
    return;
  if (temp_val < 0.1)
    return;
  if (temp_val > 10) /* 10x zoom seems like quite a bit... */
    return;

  /* set the zoom */
  ui_rendering->zoom = temp_val;
  rendering_list_set_image(ui_rendering->contexts, 
			   ui_rendering->pixel_type,
			   ui_rendering->zoom);

  /* do updating */
  if (ui_rendering->immediate) {
    ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_rendering->canvas));
    ui_rendering_update_canvases(ui_rendering); 
    ui_common_remove_cursor(GTK_WIDGET(ui_rendering->canvas));
  } else { /* otherwise, tell the dialog we've changed */
    rendering_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "rendering_dialog");
    gnome_property_box_changed(GNOME_PROPERTY_BOX(rendering_dialog));
  }

  return;
}



/* function to enable/disable depth cueing */
void ui_rendering_dialog_cb_depth_cueing_toggle(GtkWidget * widget, gpointer data) {
  
  ui_rendering_t * ui_rendering = data;
  GtkWidget * rendering_dialog;

  ui_rendering->depth_cueing = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  /* apply the new quality */
  rendering_list_set_depth_cueing(ui_rendering->contexts, ui_rendering->depth_cueing);

  /* do updating */
  if (ui_rendering->immediate) {
    ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_rendering->canvas));
    ui_rendering_update_canvases(ui_rendering); 
    ui_common_remove_cursor(GTK_WIDGET(ui_rendering->canvas));
  } else { /* otherwise, tell the dialog we've changed */
    rendering_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "rendering_dialog");
    gnome_property_box_changed(GNOME_PROPERTY_BOX(rendering_dialog));
  }

  return;
}
  



/* function to change the front factor on depth cueing */
void ui_rendering_dialog_cb_change_front_factor(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;
  GtkWidget * rendering_dialog;
  gchar * str;
  gint error;
  gdouble temp_val;

  /* get the contents of the name entry box */
  str = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert to a floating point */
  error = sscanf(str, "%lf", &temp_val);
  g_free(str);

  if (error == EOF)  /* make sure it's a valid number */
    return;

  /* set the front factor */
  ui_rendering->front_factor = temp_val;
  rendering_list_set_depth_cueing_parameters(ui_rendering->contexts,
					     ui_rendering->front_factor,
					     ui_rendering->density);

  /* do updating */
  if (ui_rendering->immediate) {
    ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_rendering->canvas));
    ui_rendering_update_canvases(ui_rendering); 
    ui_common_remove_cursor(GTK_WIDGET(ui_rendering->canvas));
  } else { /* otherwise, tell the dialog we've changed */
    rendering_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "rendering_dialog");
    gnome_property_box_changed(GNOME_PROPERTY_BOX(rendering_dialog));
  }

  return;
}



/* function to change the density parameter on depth cueing */
void ui_rendering_dialog_cb_change_density(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;
  GtkWidget * rendering_dialog;
  gchar * str;
  gint error;
  gdouble temp_val;

  /* get the contents of the name entry box */
  str = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert to a floating point */
  error = sscanf(str, "%lf", &temp_val);
  g_free(str);

  if (error == EOF)  /* make sure it's a valid number */
    return;

  /* set the density */
  ui_rendering->density = temp_val;
  rendering_list_set_depth_cueing_parameters(ui_rendering->contexts,
					     ui_rendering->front_factor,
					     ui_rendering->density);
  
  /* do updating */
  if (ui_rendering->immediate) {
    ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_rendering->canvas));
    ui_rendering_update_canvases(ui_rendering); 
    ui_common_remove_cursor(GTK_WIDGET(ui_rendering->canvas));
  } else { /* otherwise, tell the dialog we've changed */
    rendering_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "rendering_dialog");
    gnome_property_box_changed(GNOME_PROPERTY_BOX(rendering_dialog));
  }

  return;
}

/* changing the color table of a context */
void ui_rendering_dialog_cb_color_table(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering ;
  GtkWidget * rendering_dialog;
  rendering_t * context = data;
  color_table_t i_color_table;

  ui_rendering = gtk_object_get_data(GTK_OBJECT(widget), "ui_rendering");

  /* figure out which scaling menu item called me */
  i_color_table = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"color_table"));

  /* set the color table */
  context->volume->color_table = i_color_table;
  
  /* do the updating */
  if (ui_rendering->immediate) {
    ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_rendering->canvas));
    ui_rendering_update_canvases(ui_rendering); 
    ui_common_remove_cursor(GTK_WIDGET(ui_rendering->canvas));
  } else { /* otherwise, tell the dialog we've changed */
    rendering_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "rendering_dialog");
    gnome_property_box_changed(GNOME_PROPERTY_BOX(rendering_dialog));
  }

  return;
}

/* function called to change the opacity density or gradient for classification */
void ui_rendering_dialog_cb_opacity_event(GtkWidget * widget, GdkEventButton * event, gpointer data) {
  ui_rendering_dialog_cb_opacity(widget, data);
  return;
}

/* function called to change the opacity density or gradient for classification */
void ui_rendering_dialog_cb_opacity(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering;
  GtkWidget * rendering_dialog;
  GtkWidget * gamma_curve;
  gint i;
  guint num_points;
  rendering_t * context=data;
  curve_type_t curve_type;
  classification_t which_classification;

  g_return_if_fail(context!=NULL);

  /* get some pointers */
  gamma_curve =  gtk_object_get_data(GTK_OBJECT(widget), "gamma_curve");
  which_classification = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(gamma_curve), "classification_type"));
  rendering_dialog =  gtk_object_get_data(GTK_OBJECT(gamma_curve), "rendering_dialog");
  ui_rendering = gtk_object_get_data(GTK_OBJECT(gamma_curve), "ui_rendering");

  /* figure out the curve type */
  switch (GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve)->curve_type) {
  case GTK_CURVE_TYPE_SPLINE:
    curve_type = CURVE_SPLINE;
    num_points = GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve)->num_ctlpoints; 
    break;
  case GTK_CURVE_TYPE_LINEAR:       
  default:
    curve_type = CURVE_LINEAR;
    num_points = GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve)->num_ctlpoints; 
    break;
  }

  /* set the ramp table to what's in the curve widget */
  if (which_classification == DENSITY_CLASSIFICATION)
    gtk_curve_get_vector(GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve), 
			 RENDERING_DENSITY_MAX, context->density_ramp);   
  else /* GRADIENT_CLASSIFICATION */
    gtk_curve_get_vector(GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve), 
			 RENDERING_GRADIENT_MAX, context->gradient_ramp);   
    
  /* store the control points on the widget for later use */
  g_free(context->ramp_x[which_classification]); /* free the old stuff */
  g_free(context->ramp_y[which_classification]); /* free the old stuff */
  context->num_points[which_classification] =  num_points;
  context->curve_type[which_classification] = curve_type;
    
  /* allocate some new memory */
  if ((context->ramp_x[which_classification] = 
       (gint *) g_malloc(context->num_points[which_classification]*sizeof(gint))) == NULL) {
    g_warning("%s: couldn't allocate space for ramp x",PACKAGE);
    return;
  }
  if ((context->ramp_y[which_classification] = 
       (gfloat *) g_malloc(context->num_points[which_classification]*sizeof(gfloat))) == NULL) {
    g_warning("%s: couldn't allocate space for ramp y",PACKAGE);
    return;
  }
    
  /* copy the new ctrl points */
  for (i=0;i<context->num_points[which_classification];i++) {
    context->ramp_x[which_classification][i] = GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve)->ctlpoint[i][0];
    context->ramp_y[which_classification][i] = GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve)->ctlpoint[i][1];
  }

  /* do updating */
  if (ui_rendering->immediate) {
    ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_rendering->canvas));
    ui_rendering_update_canvases(ui_rendering); 
    ui_common_remove_cursor(GTK_WIDGET(ui_rendering->canvas));
  } else  /* otherwise, tell the dialog we've changed */
    gnome_property_box_changed(GNOME_PROPERTY_BOX(rendering_dialog));

  return;
}





/* function called when we hit the apply button */
void ui_rendering_dialog_cb_apply(GtkWidget* widget, gint page_number, gpointer data) {
  
  ui_rendering_t * ui_rendering = data;
  
  /* we'll apply all page changes at once */
  if (page_number != -1)
    return;

  /* and render */
  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_rendering->canvas));
  ui_rendering_update_canvases(ui_rendering); 
  ui_common_remove_cursor(GTK_WIDGET(ui_rendering->canvas));

  return;
}

/* callback for the help button */
void ui_rendering_dialog_cb_help(GnomePropertyBox *rendering_dialog, gint page_number, gpointer data) {

  GnomeHelpMenuEntry help_ref={PACKAGE,"rendering-dialog-help.html#RENDERING-DIALOG-HELP"};
  GnomeHelpMenuEntry help_ref_0 = {PACKAGE,"rendering-dialog-help.html#RENDERING-DIALOG-HELP-BASIC"};
  GnomeHelpMenuEntry help_ref_1 = {PACKAGE,"rendering-dialog-help.html#RENDERING-DIALOG-HELP-DATA-SET"};


  switch (page_number) {
  case 0:
    gnome_help_display (0, &help_ref_0);
    break;
  case 1:
    gnome_help_display (0, &help_ref_1);
    break;
  default:
    gnome_help_display (0, &help_ref);
    break;
  }

  return;
}


/* function called to destroy the rendering parameter dialog */
void ui_rendering_dialog_cb_close(GtkWidget* widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;

  /* destroy the dialog */
  gtk_widget_destroy(widget);
  ui_rendering->parameter_dialog = NULL;

  return;
}

#endif



