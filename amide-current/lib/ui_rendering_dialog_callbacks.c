/* ui_rendering_dialog_callbacks.c
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



/* function to change  the rendering quality */
void ui_rendering_dialog_callbacks_change_quality(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;
  GtkWidget * rendering_dialog;

  ui_rendering->quality = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"quality"));

  /* apply the new quality */
  rendering_context_set_quality(ui_rendering->axis_context, ui_rendering->quality);
  rendering_list_set_quality(ui_rendering->contexts, ui_rendering->quality);

  /* do updating */
  if (ui_rendering->immediate)
    ui_rendering_update_canvases(ui_rendering); /* render now */
  else { /* otherwise, tell the dialog we've changed */
    rendering_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "rendering_dialog");
    gnome_property_box_changed(GNOME_PROPERTY_BOX(rendering_dialog));
  }

  return;
}



/* function to change the return pixel type  */
void ui_rendering_dialog_callbacks_change_pixel_type(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;
  GtkWidget * rendering_dialog;

  ui_rendering->pixel_type = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"pixel_type"));

  /* apply the new quality */
  rendering_context_set_image(ui_rendering->axis_context, 
			      ui_rendering->pixel_type,
			      RENDERING_DEFAULT_ZOOM);
  rendering_list_set_image(ui_rendering->contexts, 
			   ui_rendering->pixel_type,
			   ui_rendering->zoom);

  /* do updating */
  if (ui_rendering->immediate)
    ui_rendering_update_canvases(ui_rendering); /* render now */
  else { /* otherwise, tell the dialog we've changed */
    rendering_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "rendering_dialog");
    gnome_property_box_changed(GNOME_PROPERTY_BOX(rendering_dialog));
  }

  return;
}



/* function to change the zoom */
void ui_rendering_dialog_callbacks_change_zoom(GtkWidget * widget, gpointer data) {

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
  if (ui_rendering->immediate)
    ui_rendering_update_canvases(ui_rendering); /* render now */
  else { /* otherwise, tell the dialog we've changed */
    rendering_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "rendering_dialog");
    gnome_property_box_changed(GNOME_PROPERTY_BOX(rendering_dialog));
  }

  return;
}



/* function to enable/disable depth cueing */
void ui_rendering_dialog_callbacks_depth_cueing_toggle(GtkWidget * widget, gpointer data) {
  
  ui_rendering_t * ui_rendering = data;
  GtkWidget * rendering_dialog;

  ui_rendering->depth_cueing = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  /* apply the new quality */
  rendering_context_set_depth_cueing(ui_rendering->axis_context, ui_rendering->depth_cueing);
  rendering_list_set_depth_cueing(ui_rendering->contexts, ui_rendering->depth_cueing);

  /* do updating */
  if (ui_rendering->immediate)
    ui_rendering_update_canvases(ui_rendering); /* render now */
  else { /* otherwise, tell the dialog we've changed */
    rendering_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "rendering_dialog");
    gnome_property_box_changed(GNOME_PROPERTY_BOX(rendering_dialog));
  }

  return;
}
  



/* function to change the front factor on depth cueing */
void ui_rendering_dialog_callbacks_change_front_factor(GtkWidget * widget, gpointer data) {

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
  rendering_context_set_depth_cueing_parameters(ui_rendering->axis_context, 
						ui_rendering->front_factor,
						ui_rendering->density);
  rendering_list_set_depth_cueing_parameters(ui_rendering->contexts,
					     ui_rendering->front_factor,
					     ui_rendering->density);

  /* do updating */
  if (ui_rendering->immediate)
    ui_rendering_update_canvases(ui_rendering); /* render now */
  else { /* otherwise, tell the dialog we've changed */
    rendering_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "rendering_dialog");
    gnome_property_box_changed(GNOME_PROPERTY_BOX(rendering_dialog));
  }

  return;
}



/* function to change the density parameter on depth cueing */
void ui_rendering_dialog_callbacks_change_density(GtkWidget * widget, gpointer data) {

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
  rendering_context_set_depth_cueing_parameters(ui_rendering->axis_context, 
						ui_rendering->front_factor,
						ui_rendering->density);
  rendering_list_set_depth_cueing_parameters(ui_rendering->contexts,
					     ui_rendering->front_factor,
					     ui_rendering->density);
  
  /* do updating */
  if (ui_rendering->immediate)
    ui_rendering_update_canvases(ui_rendering); /* render now */
  else { /* otherwise, tell the dialog we've changed */
    rendering_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "rendering_dialog");
    gnome_property_box_changed(GNOME_PROPERTY_BOX(rendering_dialog));
  }

  return;
}


/* function to change what we're going to apply our classification function changes to */
void ui_rendering_dialog_callbacks_change_apply_to(GtkWidget * widget, gpointer data) {
  
  ui_rendering_t * ui_rendering = data;
  GtkWidget * rendering_dialog;
  guint which_context;

  /* what's our choice */
  which_context = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "which_context"));

  
  /* and save that choice for the opacity callbacks */
  rendering_dialog =  gtk_object_get_data(GTK_OBJECT(widget), "rendering_dialog");
  gtk_object_set_data(GTK_OBJECT(rendering_dialog), "apply_to", GINT_TO_POINTER(which_context));

  /* update the gamma curves */
  ui_rendering_dialog_update_curves(ui_rendering, which_context);


  /* do updating */
  if (ui_rendering->immediate)
    ui_rendering_update_canvases(ui_rendering); /* render now */
  else  /* otherwise, tell the dialog we've changed */
    gnome_property_box_changed(GNOME_PROPERTY_BOX(rendering_dialog));
			  
  return;
}



/* function called to change the opacity density for classification */
void ui_rendering_dialog_callbacks_opacity_density(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering;
  GtkWidget * rendering_dialog;
  GtkWidget * gamma_curve;
  gint i;
  rendering_list_t * temp_list;
  rendering_t * context;
  guint which_context, temp_which_context;
  curve_type_t curve_type;

  /* get some pointers */
  gamma_curve =  gtk_object_get_data(GTK_OBJECT(widget), "gamma_curve");
  rendering_dialog =  gtk_object_get_data(GTK_OBJECT(gamma_curve), "rendering_dialog");
  ui_rendering = gtk_object_get_data(GTK_OBJECT(gamma_curve), "ui_rendering");
  temp_list = ui_rendering->contexts;

  /* figure out the curve type */
  switch (GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve)->curve_type) {
  case GTK_CURVE_TYPE_LINEAR:       
    curve_type = CURVE_LINEAR;
    break;
  case GTK_CURVE_TYPE_SPLINE:
    curve_type = CURVE_SPLINE;
    break;
  case GTK_CURVE_TYPE_FREE:    
  default:
    curve_type = CURVE_FREE;
    break;
  }

  /* ------ first apply the changes to the axis rendering context -------- */
  gtk_curve_get_vector(GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve), 
		       RENDERING_DENSITY_MAX, ui_rendering->axis_context->density_ramp);   
  g_free(ui_rendering->axis_context->density_ramp_x); /* free the old stuff */
  g_free(ui_rendering->axis_context->density_ramp_y); /* free the old stuff */
  ui_rendering->axis_context->num_density_points = 
    GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve)->num_ctlpoints; /* get the # of ctrl points */
  ui_rendering->axis_context->density_curve_type = curve_type;

  if ((ui_rendering->axis_context->density_ramp_x = 
       (gint *) g_malloc(ui_rendering->axis_context->num_density_points*sizeof(gint))) == NULL) {
    g_warning("%s: couldn't allocate space for density ramp x",PACKAGE);
    return;
  }
  if ((ui_rendering->axis_context->density_ramp_y = 
       (gfloat *) g_malloc(ui_rendering->axis_context->num_density_points*sizeof(gfloat))) == NULL) {
    g_warning("%s: couldn't allocate space for density ramp y",PACKAGE);
    return;
  }

  /* copy the new ctrl points */
  for (i=0;i<ui_rendering->axis_context->num_density_points;i++) {
    ui_rendering->axis_context->density_ramp_x[i] = 
      GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve)->ctlpoint[0][i];
    ui_rendering->axis_context->density_ramp_y[i] = 
      GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve)->ctlpoint[1][i];
  }


  /* ----------- now apply the changes to the other context(s) ----- */
  /* what are we applying the changes to */
  which_context = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(rendering_dialog), "apply_to"));

  /* figure out which context */
  if (which_context == 0) /* we'll just use the first context if we're applying to all */
    context = temp_list->rendering_context;
  else {
    temp_which_context = which_context-1;
    while (temp_which_context > 0) {
      temp_list = temp_list->next;
      temp_which_context--;
    }
    context = temp_list->rendering_context;
  }


  /* go through all the contexts we need to, updating their values */
  while (context != NULL) {
    /* set the ramp table to what's in the density curve widget */
    gtk_curve_get_vector(GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve), 
			 RENDERING_DENSITY_MAX, context->density_ramp);   
    
    /* store the control points on the widget for later use */
    g_free(context->density_ramp_x); /* free the old stuff */
    g_free(context->density_ramp_y); /* free the old stuff */
    context->num_density_points =  
      GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve)->num_ctlpoints; /* get the # of ctrl points */
    context->density_curve_type = curve_type;
    
    /* allocate some new memory */
    if ((context->density_ramp_x = 
	 (gint *) g_malloc(context->num_density_points*sizeof(gint))) == NULL) {
      g_warning("%s: couldn't allocate space for density ramp x",PACKAGE);
      return;
    }
    if ((context->density_ramp_y = 
	 (gfloat *) g_malloc(context->num_density_points*sizeof(gfloat))) == NULL) {
      g_warning("%s: couldn't allocate space for density ramp y",PACKAGE);
      return;
    }
    
    /* copy the new ctrl points */
    for (i=0;i<context->num_density_points;i++) {
      context->density_ramp_x[i] = GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve)->ctlpoint[i][0];
      context->density_ramp_y[i] = GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve)->ctlpoint[i][1];
    }
    
    if (which_context > 0)
      context = NULL;
    else {
      temp_list = temp_list->next;
      if (temp_list == NULL)
	context = NULL;
      else
	context = temp_list->rendering_context;
    }
  }
    

  /* do updating */
  if (ui_rendering->immediate)
    ui_rendering_update_canvases(ui_rendering); /* render now */
  else  /* otherwise, tell the dialog we've changed */
    gnome_property_box_changed(GNOME_PROPERTY_BOX(rendering_dialog));

  return;
}



/* function called to change the opacity gradient for classification */
void ui_rendering_dialog_callbacks_opacity_gradient(GtkWidget * widget, gpointer data) {

  ui_rendering_t * ui_rendering;
  GtkWidget * rendering_dialog;
  GtkWidget * gamma_curve;
  gint i;
  rendering_list_t * temp_list;
  rendering_t * context;
  guint which_context, temp_which_context;
  curve_type_t curve_type;

  /* get some pointers */
  gamma_curve =  gtk_object_get_data(GTK_OBJECT(widget), "gamma_curve");
  rendering_dialog =  gtk_object_get_data(GTK_OBJECT(gamma_curve), "rendering_dialog");
  ui_rendering = gtk_object_get_data(GTK_OBJECT(gamma_curve), "ui_rendering");
  temp_list = ui_rendering->contexts;

  /* figure out the curve type */
  switch (GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve)->curve_type) {
  case GTK_CURVE_TYPE_LINEAR:       
    curve_type = CURVE_LINEAR;
    break;
  case GTK_CURVE_TYPE_SPLINE:
    curve_type = CURVE_SPLINE;
    break;
  case GTK_CURVE_TYPE_FREE:    
  default:
    curve_type = CURVE_FREE;
    break;
  }

  /* ------ first apply the changes to the axis rendering context -------- */
  gtk_curve_get_vector(GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve), 
		       RENDERING_GRADIENT_MAX, ui_rendering->axis_context->gradient_ramp);   
  g_free(ui_rendering->axis_context->gradient_ramp_x); /* free the old stuff */
  g_free(ui_rendering->axis_context->gradient_ramp_y); /* free the old stuff */
  ui_rendering->axis_context->num_gradient_points = 
    GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve)->num_ctlpoints; /* get the # of ctrl points */
  ui_rendering->axis_context->gradient_curve_type = curve_type;

  if ((ui_rendering->axis_context->gradient_ramp_x = 
       (gint *) g_malloc(ui_rendering->axis_context->num_gradient_points*sizeof(gint))) == NULL) {
    g_warning("%s: couldn't allocate space for gradient ramp x",PACKAGE);
    return;
  }
  if ((ui_rendering->axis_context->gradient_ramp_y = 
       (gfloat *) g_malloc(ui_rendering->axis_context->num_gradient_points*sizeof(gfloat))) == NULL) {
    g_warning("%s: couldn't allocate space for gradient ramp y",PACKAGE);
    return;
  }

  /* copy the new ctrl points */
  for (i=0;i<ui_rendering->axis_context->num_gradient_points;i++) {
    ui_rendering->axis_context->gradient_ramp_x[i] = 
      GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve)->ctlpoint[0][i];
    ui_rendering->axis_context->gradient_ramp_y[i] = 
      GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve)->ctlpoint[1][i];
  }


  /* ----------- now apply the changes to the other context(s) ----- */
  /* what are we applying the changes to */
  which_context = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(rendering_dialog), "apply_to"));

  /* figure out which context */
  if (which_context == 0) /* we'll just use the first context if we're applying to all */
    context = temp_list->rendering_context;
  else {
    temp_which_context = which_context-1;
    while (temp_which_context > 0) {
      temp_list = temp_list->next;
      temp_which_context--;
    }
    context = temp_list->rendering_context;
  }

  /* go through all the contexts we need to, updating their values */
  while (context != NULL) {
    /* set the ramp table to what's in the gradient curve widget */
    gtk_curve_get_vector(GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve), 
			 RENDERING_GRADIENT_MAX, context->gradient_ramp);   
    
    /* store the control points on the widget for later use */
    g_free(context->gradient_ramp_x); /* free the old stuff */
    g_free(context->gradient_ramp_y); /* free the old stuff */
    context->num_gradient_points =  
      GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve)->num_ctlpoints; /* get the # of ctrl points */
    context->gradient_curve_type = curve_type;
    
    /* allocate some new memory */
    if ((context->gradient_ramp_x = 
	 (gint *) g_malloc(context->num_gradient_points*sizeof(gint))) == NULL) {
      g_warning("%s: couldn't allocate space for gradient ramp x",PACKAGE);
      return;
    }
    if ((context->gradient_ramp_y = 
	 (gfloat *) g_malloc(context->num_gradient_points*sizeof(gfloat))) == NULL) {
      g_warning("%s: couldn't allocate space for gradient ramp y",PACKAGE);
      return;
    }
    
    /* copy the new ctrl points */
    for (i=0;i<context->num_gradient_points;i++) {
      context->gradient_ramp_x[i] = GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve)->ctlpoint[i][0];
      context->gradient_ramp_y[i] = GTK_CURVE(GTK_GAMMA_CURVE(gamma_curve)->curve)->ctlpoint[i][1];
    }
    
    if (which_context > 0)
      context = NULL;
    else {
      temp_list = temp_list->next;
      if (temp_list == NULL)
	context = NULL;
      else
	context = temp_list->rendering_context;
    }
  }
    
  /* do updating */
  if (ui_rendering->immediate)
    ui_rendering_update_canvases(ui_rendering); /* render now */
  else  /* otherwise, tell the dialog we've changed */
    gnome_property_box_changed(GNOME_PROPERTY_BOX(rendering_dialog));

  return;
}



/* function called when we hit the apply button */
void ui_rendering_dialog_callbacks_apply(GtkWidget* widget, gint page_number, gpointer data) {
  
  ui_rendering_t * ui_rendering = data;
  
  /* we'll apply all page changes at once */
  if (page_number != -1)
    return;

  /* and render */
  ui_rendering_update_canvases(ui_rendering); /* render now */

  return;
}

/* callback for the help button */
void ui_rendering_dialog_callbacks_help(GnomePropertyBox *rendering_dialog, gint page_number, gpointer data) {

  GnomeHelpMenuEntry help_ref={PACKAGE,"rendering-dialog-help.html#RENDERING-DIALOG-HELP"};
  GnomeHelpMenuEntry help_ref_0 = {PACKAGE,"rendering-dialog-help.html#RENDERING-DIALOG-HELP-BASIC"};
  GnomeHelpMenuEntry help_ref_1 = {PACKAGE,"rendering-dialog-help.html#RENDERING-DIALOG-HELP-DEPTH-CUEING"};
  GnomeHelpMenuEntry help_ref_2 = {PACKAGE,"rendering-dialog-help.html#RENDERING-DIALOG-HELP-CLASSIFICATION"};


  switch (page_number) {
  case 0:
    gnome_help_display (0, &help_ref_0);
    break;
  case 1:
    gnome_help_display (0, &help_ref_1);
    break;
  case 2:
    gnome_help_display (0, &help_ref_2);
    break;
  default:
    gnome_help_display (0, &help_ref);
    break;
  }

  return;
}

/* function called to destroy the rendering parameter dialog */
void ui_rendering_dialog_callbacks_close_event(GtkWidget* widget, gpointer data) {

  ui_rendering_t * ui_rendering = data;

  /* trash collection */

  /* destroy the widget */
  gtk_widget_destroy(GTK_WIDGET(ui_rendering->parameter_dialog));
  ui_rendering->parameter_dialog = NULL;

  return;
}

#endif
