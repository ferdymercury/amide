/* amitk_window_edit.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2005-2014 Andy Loening
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
#include "amitk_window_edit.h"
#include "amitk_common.h"

static void window_edit_class_init (AmitkWindowEditClass *class);
static void window_edit_init (AmitkWindowEdit *window_edit);
static void window_edit_destroy(GtkObject * object);
static void window_spin_cb(GtkWidget * widget, gpointer window_edit);
static void insert_window_level_cb  (GtkWidget * widget, gpointer window_edit);
static void window_edit_update_entries(AmitkWindowEdit * window_edit);

static GtkVBoxClass *parent_class;

GType amitk_window_edit_get_type (void) {

  static GType window_edit_type = 0;

  if (!window_edit_type)
    {
      static const GTypeInfo window_edit_info =
      {
	sizeof (AmitkWindowEditClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) window_edit_class_init,
	(GClassFinalizeFunc) NULL,
	NULL, /* class data */
	sizeof (AmitkWindowEdit),
	0, /* # preallocs */
	(GInstanceInitFunc) window_edit_init,
	NULL /* value table */
      };

      window_edit_type = g_type_register_static(GTK_TYPE_VBOX, "AmitkWindowEdit", &window_edit_info, 0);
    }

  return window_edit_type;
}

static void window_edit_class_init (AmitkWindowEditClass *class)
{
  GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS(class);

  parent_class = g_type_class_peek_parent(class);

  gtkobject_class->destroy = window_edit_destroy;

}

static void window_edit_init (AmitkWindowEdit *window_edit)
{

  GtkWidget * table;
  GtkWidget * label;
  AmitkWindow i_window;
  AmitkLimit i_limit;

  guint table_row=0;


  /* initialize some critical stuff */
  window_edit->data_set = NULL;
  window_edit->preferences = NULL;

  table = gtk_table_new(11,4, FALSE);
  gtk_container_add(GTK_CONTAINER(window_edit), table);

  for (i_limit = 0; i_limit < AMITK_LIMIT_NUM; i_limit++) {
    window_edit->limit_label[i_limit]  = gtk_label_new(NULL);
    gtk_table_attach(GTK_TABLE(table), window_edit->limit_label[i_limit], 1+i_limit,2+i_limit, 
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(window_edit->limit_label[i_limit]);
  }
  table_row++;

  for (i_window = 0; i_window < AMITK_WINDOW_NUM; i_window++) {
    label = gtk_label_new(_(amitk_window_names[i_window]));
    gtk_table_attach(GTK_TABLE(table), label, 0,1, 
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);
      
    for (i_limit = 0; i_limit < AMITK_LIMIT_NUM; i_limit++) {
      window_edit->window_spin[i_window][i_limit] = gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 1.0);
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(window_edit->window_spin[i_window][i_limit]), FALSE);
      g_object_set_data(G_OBJECT(window_edit->window_spin[i_window][i_limit]), "which_window", GINT_TO_POINTER(i_window));
      g_object_set_data(G_OBJECT(window_edit->window_spin[i_window][i_limit]), "which_limit", GINT_TO_POINTER(i_limit));
      g_signal_connect(G_OBJECT(window_edit->window_spin[i_window][i_limit]), "output",
		       G_CALLBACK(amitk_spin_button_scientific_output), NULL);
      g_signal_connect(G_OBJECT(window_edit->window_spin[i_window][i_limit]), "value_changed",
		       G_CALLBACK(window_spin_cb), window_edit);
      gtk_table_attach(GTK_TABLE(table), window_edit->window_spin[i_window][i_limit], 1+i_limit,2+i_limit, 
		       table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(window_edit->window_spin[i_window][i_limit]);
    }

    window_edit->insert_button[i_window] = gtk_button_new_with_label("Insert Current Thresholds");
    gtk_table_attach(GTK_TABLE(table), window_edit->insert_button[i_window], 3,4,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    g_object_set_data(G_OBJECT(window_edit->insert_button[i_window]), 
		      "which_window", GINT_TO_POINTER(i_window));
    g_signal_connect(G_OBJECT(window_edit->insert_button[i_window]), "clicked", 
		     G_CALLBACK(insert_window_level_cb), window_edit);

    table_row++;
  }

  gtk_widget_show(table);

}

static void window_edit_destroy (GtkObject * gtkobject) {

  AmitkWindowEdit * window_edit;

  g_return_if_fail (gtkobject != NULL);
  g_return_if_fail (AMITK_IS_WINDOW_EDIT (gtkobject));

  window_edit = AMITK_WINDOW_EDIT(gtkobject);

  if (window_edit->data_set != NULL) {
    g_signal_handlers_disconnect_by_func(G_OBJECT(window_edit->data_set), 
					 window_edit_update_entries, window_edit);
    amitk_object_unref(AMITK_OBJECT(window_edit->data_set));
    window_edit->data_set = NULL;
  } 

  if (window_edit->preferences != NULL) {
    g_signal_handlers_disconnect_by_func(G_OBJECT(window_edit->preferences), 
					 window_edit_update_entries, window_edit);
    g_object_unref(G_OBJECT(window_edit->preferences));
    window_edit->preferences = NULL;
  }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (gtkobject);
}


static void window_spin_cb(GtkWidget * widget, gpointer data) {

  AmitkWindowEdit * window_edit = data;
  AmitkWindow which_window;
  AmitkLimit which_limit;
  amide_data_t value, min, max, center, window;
  AmitkThresholdStyle threshold_style;
  

  g_return_if_fail(AMITK_IS_WINDOW_EDIT(window_edit));

  which_window = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "which_window"));
  which_limit = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "which_limit"));

  value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

  if (window_edit->data_set != NULL) {
    threshold_style = AMITK_DATA_SET_THRESHOLD_STYLE(window_edit->data_set);
    min = AMITK_DATA_SET_THRESHOLD_WINDOW(window_edit->data_set, which_window, AMITK_LIMIT_MIN);
    max = AMITK_DATA_SET_THRESHOLD_WINDOW(window_edit->data_set, which_window, AMITK_LIMIT_MAX);
  } else {/* window_edit->preferences != NULL */
    threshold_style = AMITK_PREFERENCES_THRESHOLD_STYLE(window_edit->preferences);
    min = AMITK_PREFERENCES_WINDOW(window_edit->preferences, which_window, AMITK_LIMIT_MIN);
    max = AMITK_PREFERENCES_WINDOW(window_edit->preferences, which_window, AMITK_LIMIT_MAX);
  }

  /* if needed, translate center/window to min/max */
  if (threshold_style == AMITK_THRESHOLD_STYLE_CENTER_WIDTH) {
    if (which_limit == AMITK_LIMIT_MIN) {
      center = value;
      window = max-min;
    } else { /* which_limit == AMITK_LIMIT_MAX */
      center = (max+min)/2.0;
      window = value;
    }
    min = center-window/2.0;
    max = center+window/2.0;
  }
      
  


  if (window_edit->data_set != NULL) {
    amitk_data_set_set_threshold_window(window_edit->data_set, which_window, AMITK_LIMIT_MIN,min);
    amitk_data_set_set_threshold_window(window_edit->data_set, which_window, AMITK_LIMIT_MAX,max);
  } else {/*  window_edit->preferences != NULL */
    amitk_preferences_set_default_window(window_edit->preferences, which_window, AMITK_LIMIT_MIN, min);
    amitk_preferences_set_default_window(window_edit->preferences, which_window, AMITK_LIMIT_MAX, max);
  }

  return;
}


static void insert_window_level_cb  (GtkWidget * widget, gpointer data) {
  
  AmitkWindowEdit * window_edit = data;
  AmitkWindow window;

  g_return_if_fail(AMITK_IS_WINDOW_EDIT(window_edit));
  g_return_if_fail(AMITK_IS_DATA_SET(window_edit->data_set));

  window = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "which_window"));

  amitk_data_set_set_threshold_window(window_edit->data_set, window, 
				      AMITK_LIMIT_MIN, 
				      AMITK_DATA_SET_THRESHOLD_MIN(window_edit->data_set, 0));
  amitk_data_set_set_threshold_window(window_edit->data_set, window, 
				      AMITK_LIMIT_MAX, 
				      AMITK_DATA_SET_THRESHOLD_MAX(window_edit->data_set, 0));

  return;
}

/* function to update the entry widgets */
static void window_edit_update_entries(AmitkWindowEdit * window_edit) {

  AmitkLimit i_limit;
  AmitkWindow i_window;
  AmitkThresholdStyle threshold_style;
  amide_data_t center;
  amide_data_t window;
  amide_data_t value[AMITK_LIMIT_NUM];

  g_return_if_fail(AMITK_IS_WINDOW_EDIT(window_edit));

  if (window_edit->data_set != NULL)
    threshold_style = AMITK_DATA_SET_THRESHOLD_STYLE(window_edit->data_set);
  else /* window_edit->preferences != NULL */
    threshold_style = AMITK_PREFERENCES_THRESHOLD_STYLE(window_edit->preferences);

  for (i_limit = 0; i_limit < AMITK_LIMIT_NUM; i_limit++) {
    gtk_label_set_text(GTK_LABEL(window_edit->limit_label[i_limit]),
		       amitk_limit_names[threshold_style][i_limit]);
  }


  for (i_window = 0; i_window < AMITK_WINDOW_NUM; i_window++) {

    for (i_limit = 0; i_limit < AMITK_LIMIT_NUM; i_limit++) 
      if (window_edit->data_set != NULL) 
	value[i_limit] = AMITK_DATA_SET_THRESHOLD_WINDOW(window_edit->data_set, i_window, i_limit);
      else  /* window_edit->preferences != NULL */
	value[i_limit] = AMITK_PREFERENCES_WINDOW(window_edit->preferences, i_window, i_limit);

    /* if needed, switch the values to center/window style */
    if (threshold_style == AMITK_THRESHOLD_STYLE_CENTER_WIDTH) {
      center = (value[AMITK_LIMIT_MAX]+value[AMITK_LIMIT_MIN])/2.0;
      window = (value[AMITK_LIMIT_MAX]-value[AMITK_LIMIT_MIN]);
      value[AMITK_LIMIT_MIN] = center;
      value[AMITK_LIMIT_MAX] = window;
    }

    for (i_limit = 0; i_limit < AMITK_LIMIT_NUM; i_limit++) {
      g_signal_handlers_block_by_func(G_OBJECT(window_edit->window_spin[i_window][i_limit]),
				      G_CALLBACK(window_spin_cb), window_edit);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(window_edit->window_spin[i_window][i_limit]),
				value[i_limit]);
      g_signal_handlers_unblock_by_func(G_OBJECT(window_edit->window_spin[i_window][i_limit]),
					G_CALLBACK(window_spin_cb), window_edit);
    }
  }

  return;
}

/* if data_set is specified, this widget causes and reacts to changes
 in the corresponding data set. Otherwise, the preferences object will
 be changed/reacted to.
*/
GtkWidget * amitk_window_edit_new(AmitkDataSet * data_set, AmitkPreferences * preferences) {

  AmitkWindowEdit * window_edit;
  AmitkWindow i_window;

  g_return_val_if_fail(AMITK_IS_DATA_SET(data_set) || AMITK_IS_PREFERENCES(preferences), NULL);

  window_edit = g_object_new(amitk_window_edit_get_type (), NULL);

  if (data_set != NULL) {
    window_edit->data_set = AMITK_DATA_SET(amitk_object_ref(AMITK_OBJECT(data_set)));
    g_signal_connect_swapped(G_OBJECT(window_edit->data_set), "threshold_style_changed", 
			     G_CALLBACK(window_edit_update_entries), window_edit);
    g_signal_connect_swapped(G_OBJECT(window_edit->data_set), "windows_changed", 
			     G_CALLBACK(window_edit_update_entries), window_edit);
    for (i_window=0; i_window < AMITK_WINDOW_NUM; i_window++)
      gtk_widget_show(window_edit->insert_button[i_window]);

  } else { /* preferences != NULL */
    window_edit->preferences = AMITK_PREFERENCES(g_object_ref(G_OBJECT(preferences)));
    g_signal_connect_swapped(G_OBJECT(window_edit->preferences), "data_set_preferences_changed", 
			     G_CALLBACK(window_edit_update_entries), window_edit);

    for (i_window=0; i_window < AMITK_WINDOW_NUM; i_window++)
      gtk_widget_hide(window_edit->insert_button[i_window]);
  }


  window_edit_update_entries(window_edit); 

  return GTK_WIDGET (window_edit);
}


