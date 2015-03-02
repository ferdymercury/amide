/* ui_time_dialog.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2002 Andy Loening
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
#include <gtk/gtk.h>
#include "ui_time_dialog.h"
#include "amitk_canvas.h"

#define AMITK_RESPONSE_EXECUTE 1

typedef enum {
  COLUMN_START,
  COLUMN_END,
  COLUMN_FRAME,
  COLUMN_NAME,
  COLUMN_DATA_SET,
  NUM_COLUMNS
} column_type_t;

enum {
  ENTRY_START,
  ENTRY_END,
  NUM_ENTRIES
};
 
static gchar * column_names[] =  {
  "Start (s)",
  "End (s)", 
  "Frame #", 
  "Data Set",
  "error - shouldn't be used"
};

typedef struct ui_time_dialog_t {
  amide_time_t start;
  amide_time_t end;
  gboolean valid;
  GList * data_sets;
} ui_time_dialog_t;


static void selection_for_each_func(GtkTreeModel *model, GtkTreePath *path,
				    GtkTreeIter *iter, gpointer data);
static void selection_changed_cb (GtkTreeSelection *selection, gpointer data);
static void response_cb (GtkDialog * dialog, gint response_id, gpointer data);
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data);
static void change_entry_cb(GtkWidget * widget, gpointer data);
static void update_model(GtkListStore * store, GtkTreeSelection *selection,
			 GList * data_sets, amide_time_t start, amide_time_t end);
static void update_selections(GtkTreeModel * model, GtkTreeSelection *selection,
			      GtkWidget * dialog, amide_time_t start, amide_time_t end);
static void update_entries(GtkWidget * dialog, amide_time_t new_start, amide_time_t new_end);
static void data_set_time_changed_cb(AmitkDataSet * ds, gpointer data);
static void add_data_set(ui_study_t * ui_study, ui_time_dialog_t * new_time, AmitkDataSet * ds);
static void remove_data_set(ui_study_t * ui_study, ui_time_dialog_t * new_time, AmitkDataSet * ds);




static void selection_for_each_func(GtkTreeModel *model, GtkTreePath *path,
				    GtkTreeIter *iter, gpointer data) {

  amide_time_t start, end, old_end;
  ui_time_dialog_t * new_time = data;
  
  gtk_tree_model_get(model, iter, COLUMN_START, &start, COLUMN_END, &end, -1);

  /* save our new times */
  if (new_time->valid) {
    old_end = new_time->end;
    if (start < new_time->start)
      new_time->start = start+EPSILON*fabs(start);
    if (end > old_end)
      new_time->end = end-EPSILON*fabs(end);
    else
      new_time->end = old_end-EPSILON*fabs(old_end);
  } else {
    new_time->valid = TRUE;
    new_time->start = start+EPSILON*fabs(start);
    new_time->end = end-EPSILON*fabs(end);
  }
  
  return;
}

/* reset out start and duration based on what just got selected */
static void selection_changed_cb (GtkTreeSelection *selection, gpointer data) {

  GtkWidget * dialog = data;
  GtkTreeView * tree_view;
  GtkTreeModel * model;
  ui_time_dialog_t * new_time;

  new_time = g_object_get_data(G_OBJECT(dialog), "new_time");
  new_time->valid = FALSE;

  /* run the following function on each selected row */
  gtk_tree_selection_selected_foreach(selection, selection_for_each_func, new_time);

  if (!new_time->valid)
    new_time->valid = TRUE; /* reset selected rows to old values */

  tree_view = gtk_tree_selection_get_tree_view(selection);
  model = gtk_tree_view_get_model(tree_view);

  update_selections(model, selection, dialog, new_time->start, new_time->end);
  update_entries(dialog, new_time->start, new_time->end);

  return;
}


/* function called when we hit the apply button */
static void response_cb(GtkDialog * dialog, gint response_id, gpointer data) {
  
  ui_study_t * ui_study = data;
  ui_time_dialog_t * new_time;
  gint return_val;

  switch(response_id) {
  case AMITK_RESPONSE_EXECUTE:
    /* reset the time */
    new_time = g_object_get_data(G_OBJECT(ui_study->time_dialog), "new_time");
    amitk_study_set_view_start_time(ui_study->study, new_time->start);
    amitk_study_set_view_duration(ui_study->study, new_time->end-new_time->start);

    /* through some new text onto the time popup button */
    ui_study_update_time_button(ui_study);

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

/* callback for the help button */
/*
static void help_cb(GnomePropertyBox *time_dialog, gint page_number, gpointer data) {

  GError *err=NULL;

  gnome_help_display (PACKAGE,"basics.html#TIME-DIALOG-HELP", &err);

  if (err != NULL) {
    g_warning("couldn't open help file, error: %s", err->message);
    g_error_free(err);
  }

  return;
}
*/

/* function called to destroy the time dialog */
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_study_t * ui_study = data;
  ui_time_dialog_t * new_time;

  /* trash collection */
  new_time = g_object_get_data(G_OBJECT(ui_study->time_dialog), "new_time");
  while(new_time->data_sets != NULL) 
    remove_data_set(ui_study, new_time, new_time->data_sets->data);
  g_free(new_time);

  /* make sure the pointer in ui_study do this dialog is nulled */
  ui_study->time_dialog = NULL;

  return FALSE;
}

/* function called when a numerical entry has been changed */
static void change_entry_cb(GtkWidget * widget, gpointer data) {

  gchar * str;
  gint error;
  gdouble temp_val;
  gint which_widget;
  GtkWidget * dialog = data;
  ui_time_dialog_t * new_time;
  GtkTreeView *tree_view;
  GtkTreeSelection *selection;
  GtkTreeModel * model;
  
  which_widget = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "type")); 
  new_time = g_object_get_data(G_OBJECT(dialog), "new_time");
  tree_view = g_object_get_data(G_OBJECT(dialog), "tree_view");
  
  /* get the contents of the name entry box, convert to floating point */
  str = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  error = sscanf(str, "%lf", &temp_val);
  g_free(str);

  if (error == EOF)  /* make sure it's a valid number */
    return;
  
  switch(which_widget) {
  case ENTRY_START:
    if (temp_val-EPSILON*fabs(temp_val) > new_time->end)
      new_time->start = new_time->end-EPSILON*fabs(new_time->end);
    else
      new_time->start = temp_val;
    break;
  case ENTRY_END:
    if (temp_val+EPSILON*fabs(temp_val) < new_time->start)
      new_time->end = new_time->start+EPSILON*fabs(new_time->start);
    else
      new_time->end = temp_val;
    break;
  }

  model = gtk_tree_view_get_model(tree_view);
  selection = gtk_tree_view_get_selection(tree_view);
  update_selections(model, selection, dialog, new_time->start, new_time->end);

  return;
}

static void update_model(GtkListStore * store, GtkTreeSelection *selection,
			 GList * data_sets, amide_time_t start, amide_time_t end) {

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
  frames = g_new(guint,num_sets);
  if ((frames == NULL) && (num_sets !=0)) {
    g_warning("can't count frames or allocate memory!");
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
			COLUMN_END, min+min_ds->frame_duration[frames[min_ds_num]],
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
			      GtkWidget * dialog, amide_time_t start, amide_time_t end) {
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
      if (((start <= iter_start) && ((end) > iter_start)) ||
	  ((start <= (iter_end)) && ((end) > (iter_end))) ||
	  ((start > (iter_start)) &&((end) < (iter_end)))) {
	gtk_tree_selection_select_iter(selection, &iter);
	if (!ds_used) used_sets = g_list_append(used_sets, ds);
      } else if ((!ds_used) && (iter_frame == 0) && (start < iter_start)) {
	/* special case #1
	   this is the first frame in the data set and it's still behind the time, so select it anyway */
	gtk_tree_selection_select_iter(selection, &iter);
	used_sets = g_list_append(used_sets, ds);
      } else if ((!ds_used) && (iter_frame == AMITK_DATA_SET_NUM_FRAMES(ds)-1) && (start > iter_end)) {
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

static void update_entries(GtkWidget * dialog, amide_time_t new_start, amide_time_t new_end) {
  
  GtkWidget * entry;
  gchar * temp_str;

  entry = g_object_get_data(G_OBJECT(dialog), "start_entry");
  g_signal_handlers_block_by_func(G_OBJECT(entry), G_CALLBACK(change_entry_cb), dialog);
  temp_str = g_strdup_printf("%5.2f", new_start);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_str);
  g_free(temp_str);
  g_signal_handlers_unblock_by_func(G_OBJECT(entry), G_CALLBACK(change_entry_cb), dialog);

  entry = g_object_get_data(G_OBJECT(dialog), "end_entry");
  g_signal_handlers_block_by_func(G_OBJECT(entry), G_CALLBACK(change_entry_cb), dialog);
  temp_str = g_strdup_printf("%5.2f", new_end);
  gtk_entry_set_text(GTK_ENTRY(entry), temp_str);
  g_free(temp_str);
  g_signal_handlers_unblock_by_func(G_OBJECT(entry), G_CALLBACK(change_entry_cb), dialog);

  return;
}

static void data_set_time_changed_cb(AmitkDataSet * ds, gpointer data) {
  ui_study_t * ui_study = data;
  ui_time_dialog_set_times(ui_study);
}

static void add_data_set(ui_study_t * ui_study, ui_time_dialog_t * new_time, AmitkDataSet * ds) {
  g_object_ref(ds);
  new_time->data_sets = g_list_append(new_time->data_sets, ds);
  g_signal_connect(G_OBJECT(ds), "time_changed", 
		   G_CALLBACK(data_set_time_changed_cb), ui_study);
  //  g_print("add %s %d\n", AMITK_OBJECT_NAME(ds), G_OBJECT(ds)->ref_count);
  return;
}

static void remove_data_set(ui_study_t * ui_study, ui_time_dialog_t * new_time, AmitkDataSet * ds) {

  g_return_if_fail(g_list_index(new_time->data_sets, ds) >= 0);

  //  g_print("remove %s %d\n", AMITK_OBJECT_NAME(ds), G_OBJECT(ds)->ref_count);
  g_signal_handlers_disconnect_by_func(G_OBJECT(ds), G_CALLBACK(data_set_time_changed_cb), ui_study);
  new_time->data_sets = g_list_remove(new_time->data_sets, ds);
  g_object_unref(ds);
  return;
}

/* function to setup the time combo widget */
void ui_time_dialog_set_times(ui_study_t * ui_study) {

  ui_time_dialog_t * new_time;
  GtkTreeSelection *selection;
  GtkTreeModel * model;
  GtkTreeView * tree_view;
  GList * selected_sets;
  GList * temp_sets;


  if (ui_study->time_dialog == NULL) return;

  tree_view = g_object_get_data(G_OBJECT(ui_study->time_dialog), "tree_view");
  new_time = g_object_get_data(G_OBJECT(ui_study->time_dialog), "new_time");

  while(new_time->data_sets != NULL) 
    remove_data_set(ui_study, new_time, new_time->data_sets->data);

  selected_sets = ui_study_selected_data_sets(ui_study);
  temp_sets = selected_sets;
  while (temp_sets != NULL) {
    add_data_set(ui_study, new_time, temp_sets->data);
    temp_sets = temp_sets->next;
  }
  amitk_objects_unref(selected_sets);

  model = gtk_tree_view_get_model(tree_view);
  selection = gtk_tree_view_get_selection(tree_view);

  update_model(GTK_LIST_STORE(model), selection,new_time->data_sets, 
	       new_time->start, new_time->end);

  update_selections(model, selection, ui_study->time_dialog, 
		    new_time->start, new_time->end);
  update_entries(ui_study->time_dialog, new_time->start, new_time->end);

  return;
}




/* create the time slection dialog */
void ui_time_dialog_create(ui_study_t * ui_study) {

  GtkWidget * time_dialog;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * label;
  GtkWidget * scrolled;
  guint table_row = 0;
  ui_time_dialog_t * new_time;
  GtkListStore * store;
  GtkWidget *tree;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkWidget * hseparator;
  GtkWidget * entry;
  GtkWidget * notebook;
  column_type_t i_column;

  /* sanity checks */
  if (ui_study->time_dialog != NULL)
    return;

  temp_string = g_strdup_printf("%s: Time Dialog",PACKAGE);
  time_dialog = gtk_dialog_new_with_buttons(temp_string,  GTK_WINDOW(ui_study->app),
					    GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
					    GTK_STOCK_EXECUTE, AMITK_RESPONSE_EXECUTE,
					    GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					    NULL);
  g_free(temp_string);
  ui_study->time_dialog = time_dialog; /* save a pointer to the dialog */
  gtk_window_set_resizable(GTK_WINDOW(time_dialog), TRUE);

  /* order is allow shrink, allow grow, autoshrink */
  gtk_window_set_resizable(GTK_WINDOW(time_dialog), TRUE);

  /* make (and save a pointer to) a structure to temporary hold the new time and duration */
  new_time = g_new(ui_time_dialog_t, 1);
  new_time->start = AMITK_STUDY_VIEW_START_TIME(ui_study->study);
  new_time->end = new_time->start+AMITK_STUDY_VIEW_DURATION(ui_study->study);
  new_time->valid = TRUE;
  new_time->data_sets = NULL;
  g_object_set_data(G_OBJECT(time_dialog), "new_time", new_time);
  
  /* setup the callbacks for app */
  g_signal_connect(G_OBJECT(time_dialog), "response", G_CALLBACK(response_cb), ui_study);
  g_signal_connect(G_OBJECT(time_dialog), "delete_event", G_CALLBACK(delete_event_cb), ui_study);


  notebook = gtk_notebook_new();


  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(4,4,FALSE);
  table_row=0;
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(time_dialog)->vbox), packing_table);

  label = gtk_label_new("Start (s)");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    
  entry = gtk_entry_new();
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  g_object_set_data(G_OBJECT(entry), "type", GINT_TO_POINTER(ENTRY_START));
  g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(change_entry_cb), time_dialog);
  g_object_set_data(G_OBJECT(time_dialog), "start_entry", entry);
  gtk_table_attach(GTK_TABLE(packing_table), entry,1,2,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;



  label = gtk_label_new("End (s)");
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    
  entry = gtk_entry_new();
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
  g_object_set_data(G_OBJECT(entry), "type", GINT_TO_POINTER(ENTRY_END));
  g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(change_entry_cb), time_dialog);
  g_object_set_data(G_OBJECT(time_dialog), "end_entry", entry);
  gtk_table_attach(GTK_TABLE(packing_table), entry,1,2,
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
  gtk_widget_set_size_request(scrolled,200,200);
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
  tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref(store);
  g_object_set_data(G_OBJECT(time_dialog), "tree_view", GTK_TREE_VIEW(tree));

  for (i_column=0; i_column<NUM_COLUMNS-1; i_column++) {
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes(column_names[i_column], renderer,
						      "text", i_column, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  }

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (selection_changed_cb), time_dialog);
  gtk_container_add(GTK_CONTAINER(scrolled),tree);

  /* fill in the list */
  ui_time_dialog_set_times(ui_study);

  /* and show all our widgets */
  gtk_widget_show_all(time_dialog);

  return;
}

