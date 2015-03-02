/* ui_gate_dialog.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2004-2014 Andy Loening
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
#include "amitk_common.h"
#include "ui_gate_dialog.h"

typedef enum {
  COLUMN_GATE,
  COLUMN_GATE_TIME,
  NUM_COLUMNS
} column_type_t;

static gboolean column_use_my_renderer[NUM_COLUMNS] = {
  FALSE,
  TRUE
};

enum {
  ENTRY_START,
  ENTRY_END,
  NUM_ENTRIES
};
 
static gchar * column_names[] =  {
  N_("Gate #"), 
  N_("Gate Time (s)")
};

typedef struct ui_gate_dialog_t {
  AmitkDataSet * ds;
  GtkWidget * tree_view;
  GtkWidget * start_spin;
  GtkWidget * end_spin;
  GtkWidget * autoplay_check_button;

  guint idle_handler_id;
  gboolean valid;
  gint start_gate;
  gint end_gate;
} ui_gate_dialog_t;


static void selection_for_each_func(GtkTreeModel *model, GtkTreePath *path,
				    GtkTreeIter *iter, gpointer data);
static void autoplay_cb(GtkWidget * widget, gpointer data);
static gboolean autoplay_update_while_idle(gpointer data);
static void selection_changed_cb (GtkTreeSelection *selection, gpointer data);
static gboolean delete_event_cb(GtkWidget* dialog, GdkEvent * event, gpointer data);
static void change_spin_cb(GtkSpinButton * spin_button, gpointer data);
static void update_model(GtkListStore * store, GtkTreeSelection *selection,
			 AmitkDataSet * ds);
static void update_selections(GtkTreeModel * model, GtkTreeSelection *selection,
			      GtkWidget * dialog, ui_gate_dialog_t * gd);
static void update_entries(GtkWidget * dialog);
static void data_set_gate_changed_cb(AmitkDataSet * ds, gpointer dialog);
static void remove_data_set(GtkWidget * dialog);




static void selection_for_each_func(GtkTreeModel *model, GtkTreePath *path,
				    GtkTreeIter *iter, gpointer data) {

  gint i_gate;
  ui_gate_dialog_t * gd = data;
  
  gtk_tree_model_get(model, iter, COLUMN_GATE, &i_gate, -1);
  
  if (!gd->valid) {
    gd->start_gate = i_gate;
    gd->end_gate = i_gate;
    gd->valid = TRUE;
  } else {
    if (i_gate < gd->start_gate)
      gd->start_gate = i_gate;
    else if (i_gate > gd->end_gate)
      gd->end_gate = i_gate;
  }
  

  return;
}


static void autoplay_cb(GtkWidget * widget, gpointer data) {
  ui_gate_dialog_t * gd=data;
  guint interval;
  gboolean autoplay;


  autoplay = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gd->autoplay_check_button));
  if (gd->ds == NULL) {
    autoplay = FALSE;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gd->autoplay_check_button), FALSE);
  }

  if (autoplay) {
    interval = 1000.0 / AMITK_DATA_SET_NUM_GATES(gd->ds); /* try to cycle through in 1000 ms */
    if (interval < 200) interval= 200; /* set 200 ms as lowest repetition rate) */
    if (gd->idle_handler_id == 0)
      gd->idle_handler_id = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE,interval,autoplay_update_while_idle, gd, NULL);
  } else {
    if (gd->idle_handler_id != 0) {
      g_source_remove(gd->idle_handler_id);
      gd->idle_handler_id=0;
    }
  }

  return;
}

static gboolean autoplay_update_while_idle(gpointer data) {
  ui_gate_dialog_t * gd=data;

  if (gd->ds == NULL) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gd->autoplay_check_button), FALSE);
    return FALSE;
  }

  gd->start_gate = 1 + AMITK_DATA_SET_VIEW_START_GATE(gd->ds);
  if (gd->start_gate >= AMITK_DATA_SET_NUM_GATES(gd->ds))
    gd->start_gate = 0;
  gd->end_gate = gd->start_gate; 
  amitk_data_set_set_view_start_gate(gd->ds, gd->start_gate);
  amitk_data_set_set_view_end_gate(gd->ds, gd->end_gate);

  return TRUE;
}

/* reset out start and duration based on what just got selected */
static void selection_changed_cb (GtkTreeSelection *selection, gpointer data) {

  GtkWidget * dialog = data;
  ui_gate_dialog_t * gd;

  gd = g_object_get_data(G_OBJECT(dialog), "gd");
  if (gd->ds == NULL) return;

  /* run the following function on each selected row */
  gd->valid = FALSE;
  gtk_tree_selection_selected_foreach(selection, selection_for_each_func, gd);

  amitk_data_set_set_view_start_gate(gd->ds, gd->start_gate);
  amitk_data_set_set_view_end_gate(gd->ds, gd->end_gate);

  return;
}


/* function called to destroy the gate dialog */
static gboolean delete_event_cb(GtkWidget* dialog, GdkEvent * event, gpointer data) {

  ui_gate_dialog_t * gd = data;
  GtkTreeSelection *selection;

  /* explicitly disconnect signals, sometimes GTK throws some of these on delete (after unref'ing study */
  g_signal_handlers_disconnect_by_func(G_OBJECT(gd->start_spin), G_CALLBACK(change_spin_cb), dialog);
  g_signal_handlers_disconnect_by_func(G_OBJECT(gd->end_spin), G_CALLBACK(change_spin_cb), dialog);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gd->tree_view));
  g_signal_handlers_disconnect_by_func(G_OBJECT(selection), G_CALLBACK(selection_changed_cb), dialog);

  /* trash collection */
  if (gd->idle_handler_id != 0) {
    g_source_remove(gd->idle_handler_id);
    gd->idle_handler_id=0;
  }

  remove_data_set(dialog);
  g_free(gd);

  return FALSE;
}

/* function called when a numerical spin button has been changed */
static void change_spin_cb(GtkSpinButton * spin_button, gpointer data) {

  gint temp_val;
  gint which_widget;
  GtkWidget * dialog = data;
  ui_gate_dialog_t * gd;
  //  GtkTreeSelection *selection;
  //  GtkTreeModel * model;

  
  which_widget = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(spin_button), "type")); 
  gd = g_object_get_data(G_OBJECT(dialog), "gd");

  if (gd->ds == NULL) return;

  temp_val = gtk_spin_button_get_value_as_int(spin_button);

  switch(which_widget) {
  case ENTRY_START:
    amitk_data_set_set_view_start_gate(gd->ds, temp_val);
    break;
  case ENTRY_END:
    amitk_data_set_set_view_end_gate(gd->ds, temp_val);
    break;
  default:
    g_error("unexpected case in %s at line %d",__FILE__, __LINE__);
    break;
  }
  
  //  model = gtk_tree_view_get_model(GTK_TREE_VIEW(gd->tree_view));
  //  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gd->tree_view));
  //  update_selections(model, selection, dialog, gd);
  update_entries(dialog);
  

  return;
}

static void update_model(GtkListStore * store, GtkTreeSelection *selection,
			 AmitkDataSet * ds) {

  GtkTreeIter iter;
  guint i_gate;

  gtk_list_store_clear(store);  /* make sure the list is clear */
  if (ds == NULL) return;

  /* start generating our list of options */
  for (i_gate=0; i_gate < AMITK_DATA_SET_NUM_GATES(ds); i_gate++) {
    /* setup the corresponding list entry */
    gtk_list_store_append (store, &iter);  /* Acquire an iterator */
    gtk_list_store_set(store, &iter, 
		       COLUMN_GATE, i_gate, 
		       COLUMN_GATE_TIME, amitk_data_set_get_gate_time(ds,i_gate), -1);
  }

  return;
}

static void update_selections(GtkTreeModel * model, GtkTreeSelection *selection, 
			      GtkWidget * dialog, ui_gate_dialog_t * gd) {

  GtkTreeIter iter;
  gint iter_gate;
  gboolean select;

  if (gd->ds == NULL) return;

  /* Block signals to this list widget.  I need to do this, as I'll be
     selecting rows, causing the emission of "changed" signals */
  g_signal_handlers_block_by_func(G_OBJECT(selection), G_CALLBACK(selection_changed_cb), dialog);


  if (gtk_tree_model_get_iter_first(model, &iter)) {
    do {
      
      gtk_tree_model_get(model, &iter, COLUMN_GATE, &iter_gate,-1);
      
      /* figure out if this row is suppose to be selected */
      select = FALSE;
      if (AMITK_DATA_SET_VIEW_START_GATE(gd->ds) > AMITK_DATA_SET_VIEW_END_GATE(gd->ds)) {
	if ((iter_gate >= AMITK_DATA_SET_VIEW_START_GATE(gd->ds)) ||
	    (iter_gate <= AMITK_DATA_SET_VIEW_END_GATE(gd->ds)))
	  select = TRUE;
      } else {
	if ((iter_gate >= AMITK_DATA_SET_VIEW_START_GATE(gd->ds)) &&
	    (iter_gate <= AMITK_DATA_SET_VIEW_END_GATE(gd->ds)))
	  select = TRUE;
      }

      if (select)
	gtk_tree_selection_select_iter(selection, &iter);
      else 
	gtk_tree_selection_unselect_iter(selection, &iter);
	  
    } while(gtk_tree_model_iter_next(model, &iter));
  }
  
  /* done updating the list, we can reconnect signals now */
  g_signal_handlers_unblock_by_func(G_OBJECT(selection), G_CALLBACK(selection_changed_cb), dialog);

  return;
}

static void update_entries(GtkWidget * dialog) {
  
  ui_gate_dialog_t * gd;
  gint value;

  gd = g_object_get_data(G_OBJECT(dialog), "gd");

  g_signal_handlers_block_by_func(G_OBJECT(gd->start_spin), G_CALLBACK(change_spin_cb), dialog);
  value = (gd->ds != NULL) ? AMITK_DATA_SET_VIEW_START_GATE(gd->ds) : 0;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(gd->start_spin), value);
  g_signal_handlers_unblock_by_func(G_OBJECT(gd->start_spin), G_CALLBACK(change_spin_cb), dialog);

  g_signal_handlers_block_by_func(G_OBJECT(gd->end_spin), G_CALLBACK(change_spin_cb), dialog);
  value = (gd->ds != NULL) ? AMITK_DATA_SET_VIEW_END_GATE(gd->ds) : 0;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(gd->end_spin), value);
  g_signal_handlers_unblock_by_func(G_OBJECT(gd->end_spin), G_CALLBACK(change_spin_cb), dialog);

  return;
}

static void data_set_gate_changed_cb(AmitkDataSet * ds, gpointer data) {
  GtkWidget * dialog = data;
  ui_gate_dialog_t * gd;
  GtkTreeSelection *selection;
  GtkTreeModel * model;

  gd = g_object_get_data(G_OBJECT(dialog), "gd");

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(gd->tree_view));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gd->tree_view));

  update_selections(model, selection, dialog, gd);
  update_entries(dialog);

}

static void remove_data_set(GtkWidget * dialog) {

  ui_gate_dialog_t * gd;

  gd = g_object_get_data(G_OBJECT(dialog), "gd");

  if (gd->ds == NULL) return;

  g_signal_handlers_disconnect_by_func(G_OBJECT(gd->ds), G_CALLBACK(data_set_gate_changed_cb), dialog);
  amitk_object_unref(gd->ds);
  gd->ds = NULL;

  return;
}


void ui_gate_dialog_set_active_data_set(GtkWidget * dialog, AmitkDataSet * ds) {

  ui_gate_dialog_t * gd;
  GtkTreeSelection *selection;
  GtkTreeModel * model;

  g_return_if_fail(dialog != NULL);

  gd = g_object_get_data(G_OBJECT(dialog), "gd");

  remove_data_set(dialog);

  if (ds != NULL)
    if (AMITK_IS_DATA_SET(ds)) {
      gd->ds = amitk_object_ref(ds);
      g_signal_connect(G_OBJECT(ds), "view_gates_changed", G_CALLBACK(data_set_gate_changed_cb), dialog);
    }

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(gd->tree_view));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gd->tree_view));

  update_model(GTK_LIST_STORE(model), selection,gd->ds);
  update_selections(model, selection, dialog, gd);
  update_entries(dialog);


  return;
}




/* create the gate selection dialog */
GtkWidget * ui_gate_dialog_create(AmitkDataSet * ds, GtkWindow * parent) {

  GtkWidget * dialog;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * label;
  GtkWidget * scrolled;
  guint table_row = 0;
  ui_gate_dialog_t * gd;
  GtkListStore * store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkWidget * hseparator;
  column_type_t i_column;

  temp_string = g_strdup_printf(_("%s: Gate Dialog"),PACKAGE);
  dialog = gtk_dialog_new_with_buttons(temp_string,  parent,
					    GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
					    NULL);
  g_free(temp_string);
  gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);

  gd = g_try_new(ui_gate_dialog_t, 1);
  g_return_val_if_fail(gd != NULL, NULL);
  gd->ds = NULL;
  gd->ds = amitk_object_ref(ds);
  gd->idle_handler_id=0;
  g_object_set_data(G_OBJECT(dialog), "gd", gd);
  
  /* setup the callbacks for the dialog */
  g_signal_connect(G_OBJECT(dialog), "delete_event", G_CALLBACK(delete_event_cb), gd);

  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(5,4,FALSE);
  table_row=0;
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), packing_table);

  label = gtk_label_new(_("Start Gate"));
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  gd->start_spin = gtk_spin_button_new_with_range(0, G_MAXDOUBLE, 1.0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(gd->start_spin), TRUE);
  g_object_set_data(G_OBJECT(gd->start_spin), "type", GINT_TO_POINTER(ENTRY_START));
  g_signal_connect(G_OBJECT(gd->start_spin), "value_changed",  
		   G_CALLBACK(change_spin_cb), dialog);
  g_signal_connect(G_OBJECT(gd->start_spin), "output",
		   G_CALLBACK(amitk_spin_button_scientific_output), NULL);
  gtk_table_attach(GTK_TABLE(packing_table), gd->start_spin,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;



  label = gtk_label_new(_("End Gate"));
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    
  gd->end_spin = gtk_spin_button_new_with_range(0, G_MAXDOUBLE, 1.0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(gd->end_spin), TRUE);
  g_object_set_data(G_OBJECT(gd->end_spin), "type", GINT_TO_POINTER(ENTRY_END));
  g_signal_connect(G_OBJECT(gd->end_spin), "value_changed",  
		   G_CALLBACK(change_spin_cb), dialog);
  g_signal_connect(G_OBJECT(gd->end_spin), "output",
		   G_CALLBACK(amitk_spin_button_scientific_output), NULL);
  gtk_table_attach(GTK_TABLE(packing_table), gd->end_spin,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator, 0, 4, 
		   table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* the scroll widget which the list will go into */
  scrolled = gtk_scrolled_window_new(NULL,NULL);
  gtk_widget_set_size_request(scrolled,350,200);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
  				 GTK_POLICY_AUTOMATIC,
  				 GTK_POLICY_AUTOMATIC);
  gtk_table_attach(GTK_TABLE(packing_table), scrolled, 0,2, 
  		   table_row, table_row+1, 
  		   X_PACKING_OPTIONS | GTK_FILL, 
  		   Y_PACKING_OPTIONS | GTK_FILL, 
  		   X_PADDING, Y_PADDING);
  table_row++;
    


  /* and the list itself */
  store = gtk_list_store_new(NUM_COLUMNS, G_TYPE_INT, AMITK_TYPE_TIME);
  gd->tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref(store);

  for (i_column=0; i_column<NUM_COLUMNS; i_column++) {
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes(_(column_names[i_column]), renderer,
						      "text", i_column, NULL);
    if (column_use_my_renderer[i_column]) 
      gtk_tree_view_column_set_cell_data_func(column, renderer,
					      amitk_real_cell_data_func,
					      GINT_TO_POINTER(i_column),NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (gd->tree_view), column);
  }

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (gd->tree_view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (selection_changed_cb), dialog);
  gtk_container_add(GTK_CONTAINER(scrolled),gd->tree_view);

  /* check button for autoplay */
  gd->autoplay_check_button = gtk_check_button_new_with_label (_("Auto play"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gd->autoplay_check_button), FALSE);
  g_signal_connect(G_OBJECT(gd->autoplay_check_button), "toggled", G_CALLBACK(autoplay_cb),gd);
  gtk_table_attach(GTK_TABLE(packing_table), gd->autoplay_check_button,0,2,table_row,table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  /* fill in the list/update entries */
  ui_gate_dialog_set_active_data_set(dialog, ds);

  /* and show all our widgets */
  gtk_widget_show_all(dialog);

  return dialog;
}

