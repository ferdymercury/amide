/* amitk_space_edit.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2002-2014 Andy Loening
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
#include "amide.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"
#include "amitk_space_edit.h"
#include "amitk_study.h"

#define DEFAULT_ENTRY_WIDTH 100


static void space_edit_class_init (AmitkSpaceEditClass *klass);
static void space_edit_init (AmitkSpaceEdit *space_edit);
static void space_edit_destroy(GtkObject * object);
static void space_edit_update_entries(AmitkSpaceEdit * space_edit);
static void space_edit_rotate_axis(GtkAdjustment * adjustment, gpointer data);
static gboolean space_edit_prompt(AmitkSpaceEdit * space_edit, const gchar * message);
static void space_edit_apply_entries(GtkWidget * button, gpointer space_edit);
static void space_edit_reset_axis(GtkWidget * button, gpointer data);
static void space_edit_invert_axis(GtkWidget * button, gpointer data);
#if 0
static void space_edit_apply_air(GtkWidget * button, gpointer data);
static void space_edit_export_air(GtkWidget * button, gpointer data);
#endif

static GtkVBoxClass *parent_class;

GType amitk_space_edit_get_type (void) {

  static GType space_edit_type = 0;

  if (!space_edit_type)
    {
      static const GTypeInfo space_edit_info =
      {
	sizeof (AmitkSpaceEditClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) space_edit_class_init,
	(GClassFinalizeFunc) NULL,
	NULL, /* class data */
	sizeof (AmitkSpaceEdit),
	0, /* # preallocs */
	(GInstanceInitFunc) space_edit_init,
	NULL /* value table */
      };

      space_edit_type = g_type_register_static(GTK_TYPE_VBOX, "AmitkSpaceEdit", &space_edit_info, 0);
    }

  return space_edit_type;
}

static void space_edit_class_init (AmitkSpaceEditClass *klass)
{
  /*  GObjectClass   *gobject_class;
      GtkWidgetClass *widget_class; */
  GtkObjectClass *gtkobject_class;

  /*  gobject_class =   (GObjectClass *) klass;
      widget_class =    (GtkWidgetClass*) klass; */
  gtkobject_class = (GtkObjectClass*) klass;

  parent_class = g_type_class_peek_parent(klass);

  gtkobject_class->destroy = space_edit_destroy;

}

static void space_edit_init (AmitkSpaceEdit *space_edit)
{

  GtkWidget * table;
  GtkWidget * slider;
  GtkWidget * label;
  GtkWidget * button;
  GtkWidget * hseparator;
  guint row=0;
  AmitkView i_view;
  GtkObject * adjustment;
  AmitkAxis i_axis, j_axis;

  /* initialize some critical stuff */
  space_edit->object = NULL;

  table = gtk_table_new(14,5, FALSE);
  gtk_container_add(GTK_CONTAINER(space_edit), table);
  gtk_widget_show(table);


  /* the sliders to spin on a view */
  for (i_view=0;i_view< AMITK_VIEW_NUM;i_view++) {

    /* label */
    label = gtk_label_new(amitk_view_get_name(i_view));
    gtk_table_attach(GTK_TABLE(table), label, 0,1, row, row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    /* the slider */
    adjustment = gtk_adjustment_new(0,-45.0,45.5,0.5,0.5,0.5);
    g_object_set_data(G_OBJECT(adjustment), "view", GINT_TO_POINTER(i_view));
    slider = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
    gtk_range_set_update_policy(GTK_RANGE(slider), GTK_UPDATE_DISCONTINUOUS);
    gtk_table_attach(GTK_TABLE(table), slider, 1,5, row, row+1,
		     X_PACKING_OPTIONS | GTK_FILL,FALSE, X_PADDING, Y_PADDING);
    g_signal_connect(G_OBJECT(adjustment), "value_changed", 
		     G_CALLBACK(space_edit_rotate_axis), space_edit);
    gtk_widget_show(slider);
    row++;
  }


  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(table), hseparator, 0, 5, row, row+1,GTK_FILL, 0, X_PADDING, Y_PADDING);
  row++;
  gtk_widget_show(hseparator);

  /* and a display of the current axis */
  label = gtk_label_new(_("i"));
  gtk_table_attach(GTK_TABLE(table), label, 1,2, row, row+1, 0, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(label);
  label = gtk_label_new(_("j"));
  gtk_table_attach(GTK_TABLE(table), label, 2,3, row, row+1, 0, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(label);
  label = gtk_label_new(_("k"));
  gtk_table_attach(GTK_TABLE(table), label, 3,4, row, row+1, 0, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(label);
  row++;


  for (i_axis=0;i_axis<AMITK_AXIS_NUM;i_axis++) {

    /* the row label */
    label = gtk_label_new(amitk_axis_get_name(i_axis));
    gtk_table_attach(GTK_TABLE(table), label, 0,1, row, row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    for (j_axis=0;j_axis<AMITK_AXIS_NUM;j_axis++) {

      space_edit->entry[i_axis][j_axis] = gtk_entry_new();
      gtk_widget_set_size_request(space_edit->entry[i_axis][j_axis], DEFAULT_ENTRY_WIDTH,-1);
      gtk_editable_set_editable(GTK_EDITABLE(space_edit->entry[i_axis][j_axis]), TRUE);
      gtk_table_attach(GTK_TABLE(table), space_edit->entry[i_axis][j_axis],j_axis+1, j_axis+2,
		       row, row+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(space_edit->entry[i_axis][j_axis]);
    }
    
    /* the invert button */
    button = gtk_button_new_with_label("invert axis");
    gtk_table_attach(GTK_TABLE(table), button, 4,5, row, row+1, 0, 0, X_PADDING, Y_PADDING);
    g_object_set_data(G_OBJECT(button), "axis", GINT_TO_POINTER(i_axis));
    g_signal_connect(G_OBJECT(button), "pressed",
		     G_CALLBACK(space_edit_invert_axis), space_edit);
    gtk_widget_show(button);
    

    row++;
  }

  label = gtk_label_new(_("data set axis:"));
  gtk_table_attach(GTK_TABLE(table), label, 0,1, row, row+1,
  		   0, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(label);
  
  /* button to apply entries */
  button = gtk_button_new_with_label(_("apply manual entries"));
  gtk_table_attach(GTK_TABLE(table), button, 1,2, 
		   row, row+1, 0, 0, X_PADDING, Y_PADDING);
  g_signal_connect(G_OBJECT(button), "pressed",
		   G_CALLBACK(space_edit_apply_entries), space_edit);
  gtk_widget_show(button);

  /* button to reset the axis */
  button = gtk_button_new_with_label(_("reset to identity"));
  gtk_table_attach(GTK_TABLE(table), button, 2,3, 
		   row, row+1, 0, 0, X_PADDING, Y_PADDING);
  g_signal_connect(G_OBJECT(button), "pressed",
		   G_CALLBACK(space_edit_reset_axis), space_edit);
  gtk_widget_show(button);
  row++;

#if 0
  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(table), hseparator, 0, 5, row, row+1,GTK_FILL, 0, X_PADDING, Y_PADDING);
  row++;
  gtk_widget_show(hseparator);

  /* our AIR buttons */
  label = gtk_label_new(_("AIR Files:"));
  gtk_table_attach(GTK_TABLE(table), label, 0,1, row, row+1,
		   0, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(label);
  
  button = gtk_button_new_with_label(_("Apply .AIR"));
  gtk_table_attach(GTK_TABLE(table), button, 1,2, 
		   row, row+1, 0, 0, X_PADDING, Y_PADDING);
  g_signal_connect(G_OBJECT(button), "pressed",
		   G_CALLBACK(space_edit_apply_air), space_edit);
  gtk_widget_show(button);

  button = gtk_button_new_with_label(_("Export .AIR"));
  gtk_table_attach(GTK_TABLE(table), button, 2,3, 
		   row, row+1, 0, 0, X_PADDING, Y_PADDING);
  g_signal_connect(G_OBJECT(button), "pressed",
		   G_CALLBACK(space_edit_export_air), space_edit);
  gtk_widget_show(button);

  row++;
#endif
}

static void space_edit_destroy (GtkObject * gtkobject) {

  AmitkSpaceEdit * space_edit;

  g_return_if_fail (gtkobject != NULL);
  g_return_if_fail (AMITK_IS_SPACE_EDIT (gtkobject));

  space_edit = AMITK_SPACE_EDIT(gtkobject);

  if (space_edit->object != NULL) {
    g_signal_handlers_disconnect_by_func(G_OBJECT(space_edit->object), 
					 space_edit_update_entries, space_edit);
    amitk_object_unref(space_edit->object);
    space_edit->object = NULL;
  }



  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (gtkobject);
}



/* function to update the entry widgets */
static void space_edit_update_entries(AmitkSpaceEdit * space_edit) {

  AmitkAxis i_axis, j_axis;
  gchar * temp_string;
  AmitkPoint one_axis;

  g_return_if_fail(AMITK_IS_SPACE_EDIT(space_edit));

  for (i_axis=0;i_axis<AMITK_AXIS_NUM;i_axis++) {
    one_axis = amitk_space_get_axis(AMITK_SPACE(space_edit->object), i_axis);

    for (j_axis=0; j_axis<AMITK_AXIS_NUM;j_axis++) {
      temp_string = g_strdup_printf("%g", point_get_component(one_axis, j_axis));
      gtk_entry_set_text(GTK_ENTRY(space_edit->entry[i_axis][j_axis]), temp_string);
      g_free(temp_string);
    }
  }

  return;
}


static void space_edit_rotate_axis(GtkAdjustment * adjustment, gpointer data) {

  AmitkSpaceEdit * space_edit = data;
  AmitkView i_view;
  amide_real_t rotation;
  AmitkPoint center;

  /* figure out which scale widget called me */
  i_view = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(adjustment),"view"));

  rotation = (adjustment->value/180)*M_PI; /* get rotation in radians */

  /* compensate for sagittal being a left-handed coordinate frame */
  if (i_view == AMITK_VIEW_SAGITTAL) rotation = -rotation; 

  if (AMITK_IS_VOLUME(space_edit->object))
    center = amitk_volume_get_center(AMITK_VOLUME(space_edit->object));
  else if (AMITK_IS_STUDY(space_edit->object))
    center = AMITK_STUDY_VIEW_CENTER(space_edit->object);
  else
    center = AMITK_SPACE_OFFSET(space_edit->object);

  /* rotate the axis */
  amitk_space_rotate_on_vector(AMITK_SPACE(space_edit->object),
			       amitk_axes_get_normal_axis(base_axes, i_view),
			       rotation, center);

  /* return adjustment back to normal */
  adjustment->value = 0.0;
  gtk_adjustment_changed(adjustment);

  return;
}


static gboolean space_edit_prompt(AmitkSpaceEdit * space_edit, const gchar * message) {

  GtkWidget * question;
  gint return_val;
  GtkWidget *toplevel;
  GtkWindow *window;

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET(space_edit));
  if (toplevel != NULL) 
    window = GTK_WINDOW(toplevel);
  else
    window = NULL;

  question = gtk_message_dialog_new(window,
  				    GTK_DIALOG_DESTROY_WITH_PARENT,
  				    GTK_MESSAGE_QUESTION,
  				    GTK_BUTTONS_OK_CANCEL,
  				    "%s", message);
  return_val = gtk_dialog_run(GTK_DIALOG(question));
  
  gtk_widget_destroy(question);
  if (return_val != GTK_RESPONSE_OK)
    return FALSE; 
  else
    return TRUE;
}




static void space_edit_apply_entries(GtkWidget * entry, gpointer data) {

  AmitkSpaceEdit * space_edit = data;
  AmitkAxis i_axis, j_axis;
  const gchar * text;
  AmitkAxes axes;
  gdouble temp_real;
  AmitkPoint center;
  gint error;

  /* first double check that we really want to do this */
  if (!space_edit_prompt(space_edit, _("Do you really wish to manual set the axis?\nThis may flip left/right relationships")))
    return;

  for (i_axis=0;i_axis<AMITK_AXIS_NUM;i_axis++) {
    for (j_axis=0; j_axis<AMITK_AXIS_NUM;j_axis++) {
      text = gtk_entry_get_text(GTK_ENTRY(space_edit->entry[i_axis][j_axis]));
      error = sscanf(text, "%lf", &temp_real);
      if (error != EOF)
	point_set_component(&(axes[i_axis]), j_axis, temp_real);
    }
  }

  amitk_axes_make_orthonormal(axes);

  if (AMITK_IS_VOLUME(space_edit->object))
    center = amitk_volume_get_center(AMITK_VOLUME(space_edit->object));
  else
    center = AMITK_SPACE_OFFSET(space_edit->object);
  amitk_space_set_axes(AMITK_SPACE(space_edit->object), axes, center);
 
  return;
}


static void space_edit_reset_axis(GtkWidget * button, gpointer data) {

  AmitkSpaceEdit * space_edit = data;
  AmitkPoint center;

  /* first double check that we really want to do this */
  if (!space_edit_prompt(space_edit, _("Do you really wish to reset the axis to identity?\nThis may flip left/right relationships")))
    return;

  /* shift the coord frame to the center of rotation, and save the old offset */
  if (AMITK_IS_VOLUME(space_edit->object))
    center = amitk_volume_get_center(AMITK_VOLUME(space_edit->object));
  else if (AMITK_IS_STUDY(space_edit->object))
    center = AMITK_STUDY_VIEW_CENTER(space_edit->object);
  else
    center = AMITK_SPACE_OFFSET(space_edit->object);

  /* reset the axis */
  amitk_space_set_axes(AMITK_SPACE(space_edit->object), base_axes, center);

  return;
}

static void space_edit_invert_axis(GtkWidget * button, gpointer data) {

  AmitkSpaceEdit * space_edit = data;
  AmitkAxis i_axis;
  AmitkPoint center;

  /* first double check that we really want to do this */
  if (!space_edit_prompt(space_edit, _("Do you really wish to invert?\nThis will flip left/right relationships")))
    return;
      
  i_axis = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "axis"));

  /* shift the coord frame to the center of rotation, and save the old offset */
  if (AMITK_IS_VOLUME(space_edit->object))
    center = amitk_volume_get_center(AMITK_VOLUME(space_edit->object));
  else if (AMITK_IS_STUDY(space_edit->object))
    center = AMITK_STUDY_VIEW_CENTER(space_edit->object);
  else
    center = AMITK_SPACE_OFFSET(space_edit->object);

  /* invert the axis */
  amitk_space_invert_axis(AMITK_SPACE(space_edit->object), i_axis, center);

  return;
}

#if 0
static void space_edit_apply_air(GtkWidget * button, gpointer data) {

  AmitkSpaceEdit * space_edit = data;

  g_warning("not yet implemented\n");

  return;
}


static void space_edit_export_air(GtkWidget * button, gpointer data) {

  AmitkSpaceEdit * space_edit = data;

  g_warning("not yet implemented\n");

  return;
}
#endif




GtkWidget * amitk_space_edit_new(AmitkObject * object) {
  AmitkSpaceEdit * space_edit;

  g_return_val_if_fail(AMITK_IS_OBJECT(object), NULL);

  space_edit = g_object_new(amitk_space_edit_get_type (), NULL);

  space_edit->object = amitk_object_ref(object);
  g_signal_connect_swapped(G_OBJECT(object), "space_changed", 
			   G_CALLBACK(space_edit_update_entries), space_edit);

  space_edit_update_entries(space_edit); 

  return GTK_WIDGET (space_edit);
}


