/* ui_time_dialog.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2014 Andy Loening
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
#include <gtk/gtk.h>
#include "amide.h"
#include "ui_time_dialog.h"



#define SPIN_BUTTON_X_SIZE 100

typedef enum {
  COLUMN_START,
  COLUMN_END,
  COLUMN_FRAME,
  COLUMN_NAME,
  COLUMN_DATA_SET,
  NUM_COLUMNS
} column_type_t;

static gboolean column_use_my_renderer[NUM_COLUMNS] = {
  TRUE,
  TRUE,
  FALSE,
  FALSE,
  FALSE
};

enum {
  ENTRY_START,
  ENTRY_END,
  NUM_ENTRIES
};
 
static gchar * column_names[] =  {
  N_("Start (s)"),
  N_("End (s)"), 
  N_("Frame #"), 
  N_("Data Set"),
  "error - shouldn't be used"
};

typedef struct ui_time_dialog_t {
  amide_time_t start;
  amide_time_t end;
  gboolean valid;
  AmitkStudy * study;
  GList * data_sets;
  GtkWidget * tree_view;
  GtkWidget * start_spin;
  GtkWidget * end_spin;
} ui_time_dialog_t;


static void selection_for_each_func(GtkTreeModel *model, GtkTreePath *path,
				    GtkTreeIter *iter, gpointer data);
static void selection_changed_cb (GtkTreeSelection *selection, gpointer data);
static gboolean delete_event_cb(GtkWidget* dialog, GdkEvent * event, gpointer data);
static void change_spin_cb(GtkSpinButton * spin_button, gpointer data);
static void update_model(GtkListStore * store, GtkTreeSelection *selection, GList * data_sets);
static void update_selections(GtkTreeModel * model, GtkTreeSelection *selection,
			      GtkWidget * dialog, ui_time_dialog_t * td);
static void update_entries(GtkWidget * dialog, ui_time_dialog_t * td);
static void data_set_time_changed_cb(AmitkDataSet * ds, gpointer dialog);
static void add_data_set(GtkWidget * dialog, AmitkDataSet * ds);
static void remove_data_set(GtkWidget * dialog, AmitkDataSet * ds);




static void selection_for_each_func(GtkTreeModel *model, GtkTreePath *path,
				    GtkTreeIter *iter, gpointer data) {

  amide_time_t start, end, old_end;
  ui_time_dialog_t * td = data;
  
  gtk_tree_model_get(model, iter, COLUMN_START, &start, COLUMN_END, &end, -1);

  /* save our new times */
  if (td->valid) {
    old_end = td->end;
    if (start < td->start)
      td->start = start+EPSILON*fabs(start);
    if (end > old_end)
      td->end = end-EPSILON*fabs(end);
    else
      td->end = old_end-EPSILON*fabs(old_end);
  } else {
    td->valid = TRUE;
    td->start = start+EPSILON*fabs(start);
    td->end = end-EPSILON*fabs(end);
  }
  
  return;
}


/* reset out start and duration based on what just got selected */
static void selection_changed_cb (GtkTreeSelection *selection, gpointer data) {

  GtkWidget * dialog = data;
  GtkTreeView * tree_view;
  GtkTreeModel * model;
  ui_time_dialog_t * td;

  td = g_object_get_data(G_OBJECT(dialog), "td");
  td->valid = FALSE;

  /* run the following function on each selected row */
  gtk_tree_selection_selected_foreach(selection, selection_for_each_func, td);

  if (!td->valid)
    td->valid = TRUE; /* reset selected rows to old values */

  tree_view = gtk_tree_selection_get_tree_view(selection);
  model = gtk_tree_view_get_model(tree_view);

  update_selections(model, selection, dialog, td);
  update_entries(dialog, td);

  amitk_study_set_view_start_time(td->study, td->start);
  amitk_study_set_view_duration(td->study, td->end-td->start);

  return;
}


/* function called to destroy the time dialog */
static gboolean delete_event_cb(GtkWidget* dialog, GdkEvent * event, gpointer data) {

  ui_time_dialog_t * td = data;
  GtkTreeSelection *selection;

  /* explicitly disconnect signals, sometimes GTK throws some of these on delete (after unref'ing study */
  g_signal_handlers_disconnect_by_func(G_OBJECT(td->start_spin), G_CALLBACK(change_spin_cb), dialog);
  g_signal_handlers_disconnect_by_func(G_OBJECT(td->end_spin), G_CALLBACK(change_spin_cb), dialog);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (td->tree_view));
  g_signal_handlers_disconnect_by_func(G_OBJECT(selection), G_CALLBACK(selection_changed_cb), dialog);

  /* trash collection */
  while(td->data_sets != NULL) 
    remove_data_set(dialog, td->data_sets->data);

  if (td->study != NULL) 
    td->study = amitk_object_unref(td->study);

  g_free(td);

  return FALSE;
}

/* function called when a numerical spin button has been changed */
static void change_spin_cb(GtkSpinButton * spin_button, gpointer data) {

  gdouble temp_val;
  gint which_widget;
  GtkWidget * dialog = data;
  ui_time_dialog_t * td;
  GtkTreeSelection *selection;
  GtkTreeModel * model;

  
  which_widget = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(spin_button), "type")); 
  td = g_object_get_data(G_OBJECT(dialog), "td");

  temp_val = gtk_spin_button_get_value(spin_button);

  switch(which_widget) {
  case ENTRY_START:
    if (temp_val-EPSILON*fabs(temp_val) > td->end)
      td->start = td->end-EPSILON*fabs(td->end);
    else
      td->start = temp_val;
    break;
  case ENTRY_END:
    if (temp_val+EPSILON*fabs(temp_val) < td->start)
      td->end = td->start+EPSILON*fabs(td->start);
    else
      td->end = temp_val;
    break;
  }
  
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(td->tree_view));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(td->tree_view));
  update_selections(model, selection, dialog, td);
  update_entries(dialog, td);
  
  amitk_study_set_view_start_time(td->study, td->start);
  amitk_study_set_view_duration(td->study, td->end-td->start);

  return;
}

static void update_model(GtkListStore * store, GtkTreeSelection *selection, GList * data_sets) {

  GList * sets;
  AmitkDataSet * min_ds;
  guint num_sets;
  guint i_ds, min_ds_num;
  guint * frames;
  guint total_frames=0;
  guint current_frames;
  amide_time_t min, temp;
  GtkTreeIter iter;

  gtk_list_store_clear(store);  /* make sure the list is clear */

  /* count the number of data setss */
  sets = data_sets;
  num_sets=0;
  while (sets != NULL) {
    num_sets++;
    total_frames += AMITK_DATA_SET_NUM_FRAMES(sets->data);
    sets = sets->next;
  }

  /* get space for the array that'll take care of which frame of which data set we're looking at*/
  frames = g_try_new(guint,num_sets);
  if ((frames == NULL) && (num_sets !=0)) {
    g_warning(_("can't count frames or allocate memory!"));
    return;
  }

  /* initialize */
  for (i_ds = 0; i_ds<num_sets;i_ds++) {
    frames[i_ds]=0;
  }

  /* start generating our list of options */
  current_frames=0;
  while (current_frames < total_frames) {
    sets = data_sets;

    /* initialize the variables with the first data set on the list */
    i_ds=0;
    while (AMITK_DATA_SET_NUM_FRAMES(sets->data) <= frames[i_ds]) {
      sets = sets->next; /* advancing to a data set that still has frames left */
      i_ds++;
    }
    min_ds = AMITK_DATA_SET(sets->data);
    min_ds_num = i_ds;
    min = amitk_data_set_get_start_time(min_ds,frames[i_ds]);

    sets = sets->next;
    i_ds++;
    while (sets != NULL) {
      if (frames[i_ds] < AMITK_DATA_SET_NUM_FRAMES(sets->data)) {
	temp = amitk_data_set_get_start_time(AMITK_DATA_SET(sets->data), frames[i_ds]);
	if (temp < min) {
	  min_ds = AMITK_DATA_SET(sets->data);
	  min = temp;
	  min_ds_num = i_ds;
	}	
      }
      i_ds++;
      sets = sets->next;
    }
    
    /* we now have the next minimum start time */
    
    /* setup the corresponding list entry */
    gtk_list_store_append (store, &iter);  /* Acquire an iterator */
    gtk_list_store_set (store, &iter,
			COLUMN_START, min,
			COLUMN_END, min+amitk_data_set_get_frame_duration(min_ds,frames[min_ds_num]),
			COLUMN_FRAME,frames[min_ds_num],
			COLUMN_NAME,AMITK_OBJECT_NAME(min_ds), 
			COLUMN_DATA_SET, min_ds,-1);

    frames[min_ds_num]++;
    current_frames++;
  }

  /* freeup our allocated data structures */
  g_free(frames);

  return;
}

static void update_selections(GtkTreeModel * model, GtkTreeSelection *selection, 
			      GtkWidget * dialog, ui_time_dialog_t * td) {

  AmitkDataSet * ds;
  gboolean ds_used;
  GtkTreeIter iter;
  gint iter_frame;
  amide_time_t iter_start, iter_end;
  GList * used_sets=NULL;;

  /* Block signals to this list widget.  I need to do this, as I'll be
     selecting rows, causing the emission of "changed" signals */
  g_signal_handlers_block_by_func(G_OBJECT(selection), G_CALLBACK(selection_changed_cb), dialog);


  if (gtk_tree_model_get_iter_first(model, &iter)) {
    do {
      
      gtk_tree_model_get(model, &iter, 
			 COLUMN_START, &iter_start, COLUMN_END, &iter_end, 
			 COLUMN_FRAME, &iter_frame, COLUMN_DATA_SET, &ds,-1);
      
      /* see if we've already marked a frame in this data set */
      ds_used = (g_list_index(used_sets, ds) >= 0);

      /* figure out if this row is suppose to be selected */
      if (((td->start <= iter_start) && ((td->end) > iter_start)) ||
	  ((td->start <= (iter_end)) && ((td->end) > (iter_end))) ||
	  ((td->start > (iter_start)) &&((td->end) < (iter_end)))) {
	gtk_tree_selection_select_iter(selection, &iter);
	if (!ds_used) used_sets = g_list_append(used_sets, ds);
      } else if ((!ds_used) && (iter_frame == 0) && (td->start < iter_start)) {
	/* special case #1
	   this is the first frame in the data set and it's still behind the time, so select it anyway */
	gtk_tree_selection_select_iter(selection, &iter);
	used_sets = g_list_append(used_sets, ds);
      } else if ((!ds_used) && (iter_frame == AMITK_DATA_SET_NUM_FRAMES(ds)-1) && (td->start > iter_end)) {
	/* special case #2
	   this is the last frame in the data set and no other frame has been selected, 
	   so select this one anyway */
	gtk_tree_selection_select_iter(selection, &iter);
	used_sets = g_list_append(used_sets, ds);
      } else {
	gtk_tree_selection_unselect_iter(selection, &iter);
      }
	  
    } while(gtk_tree_model_iter_next(model, &iter));
  }
  
  g_list_free(used_sets);

  /* done updating the list, we can reconnect signals now */
  g_signal_handlers_unblock_by_func(G_OBJECT(selection), G_CALLBACK(selection_changed_cb), dialog);

  return;
}

static void update_entries(GtkWidget * dialog, ui_time_dialog_t * td) {
  
  g_signal_handlers_block_by_func(G_OBJECT(td->start_spin), G_CALLBACK(change_spin_cb), dialog);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(td->start_spin), td->start);
  g_signal_handlers_unblock_by_func(G_OBJECT(td->start_spin), G_CALLBACK(change_spin_cb), dialog);

  g_signal_handlers_block_by_func(G_OBJECT(td->end_spin), G_CALLBACK(change_spin_cb), dialog);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(td->end_spin), td->end);
  g_signal_handlers_unblock_by_func(G_OBJECT(td->end_spin), G_CALLBACK(change_spin_cb), dialog);

  return;
}

static void data_set_time_changed_cb(AmitkDataSet * ds, gpointer data) {
  GtkWidget * dialog = data;
  ui_time_dialog_set_times(dialog);
}

static void add_data_set(GtkWidget * dialog, AmitkDataSet * ds) {

  ui_time_dialog_t * td;
  td = g_object_get_data(G_OBJECT(dialog), "td");

  td->data_sets = g_list_append(td->data_sets, amitk_object_ref(ds));
  g_signal_connect(G_OBJECT(ds), "time_changed", 
		   G_CALLBACK(data_set_time_changed_cb), dialog);
  return;
}

static void remove_data_set(GtkWidget * dialog, AmitkDataSet * ds) {

  ui_time_dialog_t * td;
  td = g_object_get_data(G_OBJECT(dialog), "td");

  g_return_if_fail(g_list_index(td->data_sets, ds) >= 0);

  g_signal_handlers_disconnect_by_func(G_OBJECT(ds), G_CALLBACK(data_set_time_changed_cb), dialog);
  td->data_sets = g_list_remove(td->data_sets, ds);
  amitk_object_unref(ds);
  return;
}

/* function to setup the time combo widget */
void ui_time_dialog_set_times(GtkWidget * dialog) {

  ui_time_dialog_t * td;
  GtkTreeSelection *selection;
  GtkTreeModel * model;
  GList * selected_sets;
  GList * temp_sets;


  g_return_if_fail(dialog != NULL);

  td = g_object_get_data(G_OBJECT(dialog), "td");

  while(td->data_sets != NULL) 
    remove_data_set(dialog, td->data_sets->data);

  selected_sets = amitk_object_get_selected_children_of_type(AMITK_OBJECT(td->study), 
							     AMITK_OBJECT_TYPE_DATA_SET, 
							     AMITK_SELECTION_ANY, TRUE); 
  temp_sets = selected_sets;
  while (temp_sets != NULL) {
    add_data_set(dialog, temp_sets->data);
    temp_sets = temp_sets->next;
  }
  amitk_objects_unref(selected_sets);

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(td->tree_view));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(td->tree_view));

  update_model(GTK_LIST_STORE(model), selection,td->data_sets);

  update_selections(model, selection, dialog, td);
  update_entries(dialog, td);

  return;
}




/* create the time selection dialog */
GtkWidget * ui_time_dialog_create(AmitkStudy * study, GtkWindow * parent) {

  GtkWidget * dialog;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * label;
  GtkWidget * scrolled;
  guint table_row = 0;
  ui_time_dialog_t * td;
  GtkListStore * store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkWidget * hseparator;
  column_type_t i_column;

  temp_string = g_strdup_printf(_("%s: Time Dialog"),PACKAGE);
  dialog = gtk_dialog_new_with_buttons(temp_string,  parent,
					    GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
					    NULL);
  g_free(temp_string);
  gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);

  /* make (and save a pointer to) a structure to temporary hold the new time and duration */
  td = g_try_new(ui_time_dialog_t, 1);
  g_return_val_if_fail(td != NULL, NULL);
  td->start = AMITK_STUDY_VIEW_START_TIME(study);
  td->end = td->start+AMITK_STUDY_VIEW_DURATION(study);
  td->valid = TRUE;
  td->study = amitk_object_ref(study);
  td->data_sets = NULL;
  g_object_set_data(G_OBJECT(dialog), "td", td);
  
  /* setup the callbacks for the dialog */
  g_signal_connect(G_OBJECT(dialog), "delete_event", G_CALLBACK(delete_event_cb), td);


  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(4,4,FALSE);
  table_row=0;
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), packing_table);

  label = gtk_label_new(_("Start (s)"));
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);

  td->start_spin = gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 1.0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(td->start_spin), FALSE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(td->start_spin), td->start);
  g_object_set_data(G_OBJECT(td->start_spin), "type", GINT_TO_POINTER(ENTRY_START));
  gtk_widget_set_size_request(td->start_spin, SPIN_BUTTON_X_SIZE, -1);
  g_signal_connect(G_OBJECT(td->start_spin), "value_changed",  
		   G_CALLBACK(change_spin_cb), dialog);
  g_signal_connect(G_OBJECT(td->start_spin), "output",
		   G_CALLBACK(amitk_spin_button_scientific_output), NULL);
  gtk_table_attach(GTK_TABLE(packing_table), td->start_spin,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;



  label = gtk_label_new(_("End (s)"));
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    
  td->end_spin = gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 1.0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(td->end_spin), FALSE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(td->end_spin), td->end);
  g_object_set_data(G_OBJECT(td->end_spin), "type", GINT_TO_POINTER(ENTRY_END));
  gtk_widget_set_size_request(td->end_spin, SPIN_BUTTON_X_SIZE, -1);
  g_signal_connect(G_OBJECT(td->end_spin), "value_changed",  
		   G_CALLBACK(change_spin_cb), dialog);
  g_signal_connect(G_OBJECT(td->end_spin), "output",
		   G_CALLBACK(amitk_spin_button_scientific_output), NULL);
  gtk_table_attach(GTK_TABLE(packing_table), td->end_spin,1,2,
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
  store = gtk_list_store_new(NUM_COLUMNS, AMITK_TYPE_TIME, AMITK_TYPE_TIME,
			     G_TYPE_INT, G_TYPE_STRING, G_TYPE_POINTER);
  td->tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref(store);

  for (i_column=0; i_column<NUM_COLUMNS-1; i_column++) {
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes(_(column_names[i_column]), renderer,
						      "text", i_column, NULL);
    if (column_use_my_renderer[i_column]) 
      gtk_tree_view_column_set_cell_data_func(column, renderer,
					      amitk_real_cell_data_func,
					      GINT_TO_POINTER(i_column),NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (td->tree_view), column);
  }

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (td->tree_view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (selection_changed_cb), dialog);
  gtk_container_add(GTK_CONTAINER(scrolled),td->tree_view);

  /* fill in the list */
  ui_time_dialog_set_times(dialog);

  /* and show all our widgets */
  gtk_widget_show_all(dialog);

  return dialog;
}

