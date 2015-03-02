/* amitk_coord_frame_edit.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2002 Andy Loening
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
#include "amitk_coord_frame_edit.h"

#define DEFAULT_ENTRY_WIDTH 75

enum {
  COORD_FRAME_CHANGED,
  LAST_SIGNAL
} amitk_coord_frame_edit_signals;

static void cfe_class_init (AmitkCoordFrameEditClass *klass);
static void cfe_init (AmitkCoordFrameEdit *cfe);
static void cfe_destroy(GtkObject * object);
static void cfe_update_entries(AmitkCoordFrameEdit * cfe);
static void cfe_rotate_axis(GtkAdjustment * adjustment, gpointer data);
static void cfe_reset_axis(GtkWidget * button, gpointer data);
static void cfe_invert_axis(GtkWidget * button, gpointer data);

static GtkVBox *cfe_parent_class = NULL;
static guint cfe_signals[LAST_SIGNAL] = {0};

GtkType amitk_coord_frame_edit_get_type (void) {

  static GtkType coord_frame_edit_type = 0;

  if (!coord_frame_edit_type)
    {
      static const GtkTypeInfo coord_frame_edit_info =
      {
	"AmitkCoordFrameEdit",
	sizeof (AmitkCoordFrameEdit),
	sizeof (AmitkCoordFrameEditClass),
	(GtkClassInitFunc) cfe_class_init,
	(GtkObjectInitFunc) cfe_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      coord_frame_edit_type = gtk_type_unique (GTK_TYPE_VBOX, &coord_frame_edit_info);
    }

  return coord_frame_edit_type;
}

static void cfe_class_init (AmitkCoordFrameEditClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) klass;
  widget_class = (GtkWidgetClass*) klass;

  cfe_parent_class = gtk_type_class (GTK_TYPE_VBOX);

  cfe_signals[COORD_FRAME_CHANGED] =
    gtk_signal_new ("coord_frame_changed",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (AmitkCoordFrameEditClass, coord_frame_changed),
		    gtk_marshal_NONE__NONE, GTK_TYPE_NONE, 0);
  gtk_object_class_add_signals(object_class, cfe_signals, LAST_SIGNAL);

  object_class->destroy = cfe_destroy;

}

static void cfe_init (AmitkCoordFrameEdit *cfe)
{

  GtkWidget * table;
  GtkWidget * slider;
  GtkWidget * label;
  GtkWidget * button;
  GtkWidget * hseparator;
  guint row=0;
  view_t i_view;
  GtkObject * adjustment;
  axis_t i_axis, j_axis;

  /* initialize some critical stuff */
  cfe->coord_frame = NULL;

  /* we're using two tables packed into a horizontal box */
  table = gtk_table_new(9,5, FALSE);
  gtk_container_add(GTK_CONTAINER(cfe), table);

  /* the sliders to spin on a view */
  for (i_view=0;i_view<NUM_VIEWS;i_view++) {

    /* label */
    label = gtk_label_new(view_names[i_view]);
    gtk_table_attach(GTK_TABLE(table), label, 0,1, row, row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    /* the slider */
    adjustment = gtk_adjustment_new(0,-45.0,45.5,0.5,0.5,0.5);
    gtk_object_set_data(GTK_OBJECT(adjustment), "view", GINT_TO_POINTER(i_view));
    slider = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
    gtk_range_set_update_policy(GTK_RANGE(slider), GTK_UPDATE_DISCONTINUOUS);
    gtk_table_attach(GTK_TABLE(table), slider, 1,5, row, row+1,
		     X_PACKING_OPTIONS | GTK_FILL,FALSE, X_PADDING, Y_PADDING);
    gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed", 
		       GTK_SIGNAL_FUNC(cfe_rotate_axis), cfe);
    gtk_widget_show(slider);
    row++;
  }


  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(table), hseparator, 0, 5, row, row+1,GTK_FILL, 0, X_PADDING, Y_PADDING);
  row++;
  gtk_widget_show(hseparator);

  /* and a display of the current axis */
  label = gtk_label_new("i");
  gtk_table_attach(GTK_TABLE(table), label, 1,2, row, row+1, 0, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(label);
  label = gtk_label_new("j");
  gtk_table_attach(GTK_TABLE(table), label, 2,3, row, row+1, 0, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(label);
  label = gtk_label_new("k");
  gtk_table_attach(GTK_TABLE(table), label, 3,4, row, row+1, 0, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(label);
  row++;


  for (i_axis=0;i_axis<NUM_AXIS;i_axis++) {

    /* the row label */
    label = gtk_label_new(axis_names[i_axis]);
    gtk_table_attach(GTK_TABLE(table), label, 0,1, row, row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    for (j_axis=0;j_axis<NUM_AXIS;j_axis++) {

      cfe->entry[i_axis][j_axis] = gtk_entry_new();
      gtk_widget_set_usize(cfe->entry[i_axis][j_axis], DEFAULT_ENTRY_WIDTH,0);
      gtk_editable_set_editable(GTK_EDITABLE(cfe->entry[i_axis][j_axis]), FALSE);
      gtk_table_attach(GTK_TABLE(table), cfe->entry[i_axis][j_axis],j_axis+1, j_axis+2,
		       row, row+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(cfe->entry[i_axis][j_axis]);
    }
    
    /* the invert button */
    button = gtk_button_new_with_label("invert axis");
    gtk_table_attach(GTK_TABLE(table), button, 4,5, row, row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_object_set_data(GTK_OBJECT(button), "axis", GINT_TO_POINTER(i_axis));
    gtk_signal_connect(GTK_OBJECT(button), "pressed",
		       GTK_SIGNAL_FUNC(cfe_invert_axis), cfe);
    gtk_widget_show(button);
    

    row++;
  }

  /* button to reset the axis */
  label = gtk_label_new("data set axis:");
  gtk_table_attach(GTK_TABLE(table), label, 0,1, row, row+1,
		   0, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(label);
  
  button = gtk_button_new_with_label("reset to default");
  gtk_table_attach(GTK_TABLE(table), button, 1,2, 
		   row, row+1, 0, 0, X_PADDING, Y_PADDING);
  gtk_signal_connect(GTK_OBJECT(button), "pressed",
		     GTK_SIGNAL_FUNC(cfe_reset_axis), cfe);
  gtk_widget_show(button);
  row++;

  gtk_widget_show(table);

}

static void cfe_destroy (GtkObject * object) {

  AmitkCoordFrameEdit * cfe;

  g_return_if_fail (object != NULL);
  g_return_if_fail (AMITK_IS_COORD_FRAME_EDIT (object));

  cfe = AMITK_COORD_FRAME_EDIT(object);

  cfe->coord_frame = rs_unref(cfe->coord_frame);
  cfe->view_coord_frame = rs_unref(cfe->view_coord_frame);

  if (GTK_OBJECT_CLASS (cfe_parent_class)->destroy)
    (* GTK_OBJECT_CLASS (cfe_parent_class)->destroy) (object);
}



/* function to update the entry widgets */
static void cfe_update_entries(AmitkCoordFrameEdit * cfe) {

  axis_t i_axis;
  gchar * temp_string;
  realpoint_t one_axis;

  for (i_axis=0;i_axis<NUM_AXIS;i_axis++) {
    one_axis = rs_specific_axis(cfe->coord_frame, i_axis);

    temp_string = g_strdup_printf("%f", one_axis.x);
    gtk_entry_set_text(GTK_ENTRY(cfe->entry[i_axis][XAXIS]), temp_string);
    g_free(temp_string);

    temp_string = g_strdup_printf("%f", one_axis.y);
    gtk_entry_set_text(GTK_ENTRY(cfe->entry[i_axis][YAXIS]), temp_string);
    g_free(temp_string);

    temp_string = g_strdup_printf("%f", one_axis.z);
    gtk_entry_set_text(GTK_ENTRY(cfe->entry[i_axis][ZAXIS]), temp_string);
    g_free(temp_string);

  }

  return;
}


static void cfe_rotate_axis(GtkAdjustment * adjustment, gpointer data) {

  AmitkCoordFrameEdit * cfe = data;
  view_t i_view;
  floatpoint_t rotation;
  realpoint_t temp_rp;

  /* figure out which scale widget called me */
  i_view = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(adjustment),"view"));

  rotation = (adjustment->value/180)*M_PI; /* get rotation in radians */

  /* compensate for sagittal being a left-handed coordinate frame */
  if (i_view == SAGITTAL) rotation = -rotation; 

  /* shift the coord frame to the center of rotation, and save the old offset */
  temp_rp = rs_offset(cfe->coord_frame);
  rs_set_offset(cfe->coord_frame, cfe->center_of_rotation);
  temp_rp = realspace_base_coord_to_alt(temp_rp, cfe->coord_frame);

  /* rotate the axis */
  realspace_rotate_on_axis(cfe->coord_frame,
			   rs_get_view_normal(cfe->view_coord_frame, i_view),
			   rotation);

  /* recalculate the offset of this volume based on the center we stored */
  temp_rp = realspace_alt_coord_to_base(temp_rp, cfe->coord_frame);
  rs_set_offset(cfe->coord_frame, temp_rp);

  /* return adjustment back to normal */
  adjustment->value = 0.0;
  gtk_adjustment_changed(adjustment);

  cfe_update_entries(cfe); 
  gtk_signal_emit (GTK_OBJECT (cfe), cfe_signals[COORD_FRAME_CHANGED]);

  return;
}


static void cfe_reset_axis(GtkWidget * button, gpointer data) {

  AmitkCoordFrameEdit * cfe = data;
  realpoint_t temp_rp;

  /* shift the coord frame to the center of rotation, and save the old offset */
  temp_rp = rs_offset(cfe->coord_frame);
  rs_set_offset(cfe->coord_frame, cfe->center_of_rotation);
  temp_rp = realspace_base_coord_to_alt(temp_rp, cfe->coord_frame);

  /* reset the axis */
  rs_set_axis(cfe->coord_frame, default_axis);

  /* reset the offset */
  temp_rp = realspace_alt_coord_to_base(temp_rp, cfe->coord_frame);
  rs_set_offset(cfe->coord_frame, temp_rp);

  /* notify the appropriate stuff that we've changed */
  cfe_update_entries(cfe); 
  gtk_signal_emit (GTK_OBJECT (cfe), cfe_signals[COORD_FRAME_CHANGED]);

  return;
}

static void cfe_invert_axis(GtkWidget * button, gpointer data) {

  AmitkCoordFrameEdit * cfe = data;
  realpoint_t temp_rp;
  axis_t i_axis;

  i_axis = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(button), "axis"));

  /* shift the coord frame to the center of rotation, and save the old offset */
  temp_rp = rs_offset(cfe->coord_frame);
  rs_set_offset(cfe->coord_frame, cfe->center_of_rotation);
  temp_rp = realspace_base_coord_to_alt(temp_rp, cfe->coord_frame);

  /* invert the axis */
  rs_invert_axis(cfe->coord_frame, i_axis);

  /* reset the offset */
  temp_rp = realspace_alt_coord_to_base(temp_rp, cfe->coord_frame);
  rs_set_offset(cfe->coord_frame, temp_rp);
  
  /* notify the appropriate stuff that we've changed */
  cfe_update_entries(cfe); 
  gtk_signal_emit (GTK_OBJECT (cfe), cfe_signals[COORD_FRAME_CHANGED]);

  return;
}



GtkWidget * amitk_coord_frame_edit_new(realspace_t * coord_frame, 
				       realpoint_t center_of_rotation,
				       realspace_t * view_coord_frame) {
  AmitkCoordFrameEdit * cfe;

  g_return_val_if_fail(coord_frame !=NULL, NULL);
  g_return_val_if_fail(view_coord_frame != NULL, NULL);

  cfe = gtk_type_new (amitk_coord_frame_edit_get_type ());

  cfe->coord_frame = rs_ref(coord_frame);
  cfe->center_of_rotation = center_of_rotation;
  cfe->view_coord_frame = rs_ref(view_coord_frame);

  cfe_update_entries(cfe); 

  return GTK_WIDGET (cfe);
}


void amitk_coord_frame_edit_change_center (AmitkCoordFrameEdit * cfe,
					   realpoint_t center_of_rotation) {
  g_return_if_fail(cfe != NULL);

  cfe->center_of_rotation = center_of_rotation;

  return;
}

void amitk_coord_frame_edit_change_offset (AmitkCoordFrameEdit * cfe,
					   realpoint_t offset) {
  g_return_if_fail(cfe != NULL);

  rs_set_offset((cfe->coord_frame), offset);

  return;
}

