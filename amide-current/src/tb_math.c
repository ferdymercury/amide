/* tb_math.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2006-2012 Andy Loening
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
#include "amitk_progress_dialog.h"
#include "tb_math.h"


#define LABEL_WIDTH 375


static gchar * data_set_error_page_text = 
N_("There is only one data set in this study.  There needs "
   "to be at least two data sets to perform mathematical operations");

static gchar * start_page_text = 
N_("Welcome to the data set math wizard, used for "
   "performing mathematical operations between medical "
   "image data sets.\n"
   "\n"
   "Note - you will get more pleasing results if the data "
   "sets in question are set to trilinear interpolation mode.");


typedef enum {
  COLUMN_DATA_SET_NAME,
  COLUMN_DATA_SET_POINTER,
  NUM_DATA_SET_COLUMNS
} data_set_column_t;

typedef enum {
  COLUMN_OPERATION_NAME,
  COLUMN_OPERATION_NUMBER,
  NUM_OPERATION_COLUMNS
} point_column_t;

typedef enum {
  INTRO_PAGE, 
  DATA_SETS_PAGE, 
  OPERATION_PAGE,
  CONCLUSION_PAGE, 
  NUM_PAGES
} which_page_t;

/* data structures */
typedef struct tb_math_t {
  GtkWidget * dialog;
  GtkWidget * page[NUM_PAGES];
  GtkWidget * progress_dialog;
  GtkWidget * list_ds1;
  GtkWidget * list_ds2;
  GtkWidget * list_operation;

  AmitkStudy * study;
  AmitkDataSet * ds1;
  AmitkDataSet * ds2;
  AmitkOperation operation;
  gboolean by_frames;

  guint reference_count;
} tb_math_t;



static void data_sets_update_model(tb_math_t * math);
static void operation_update_model(tb_math_t * math);
static void data_set_selection_changed_cb(GtkTreeSelection * selection, gpointer data);
static void operation_selection_changed_cb(GtkTreeSelection * selection, gpointer data);
static void by_frames_cb(GtkWidget * widget, gpointer data);

static tb_math_t * tb_math_free(tb_math_t * math);
static tb_math_t * tb_math_init(void);

static void prepare_page_cb(GtkAssistant * wizard, GtkWidget * page, gpointer data);
static void apply_cb(GtkAssistant * assistant, gpointer data);
static void close_cb(GtkAssistant * assistant, gpointer data);


static GtkWidget * create_data_sets_page(tb_math_t * tb_math);
static GtkWidget * create_operation_page(tb_math_t * tb_math);


static void data_sets_update_model(tb_math_t * tb_math) {

  GtkTreeIter iter;
  GtkTreeModel * model;
  GtkTreeSelection *selection;
  GList * data_sets;
  GList * temp_data_sets;
  gint count;

  /* update data set 1 */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tb_math->list_ds1));
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tb_math->list_ds1));
  gtk_list_store_clear(GTK_LIST_STORE(model));  /* make sure the list is clear */

  data_sets = amitk_object_get_children_of_type(AMITK_OBJECT(tb_math->study), AMITK_OBJECT_TYPE_DATA_SET, TRUE);

  count = 0;
  temp_data_sets = data_sets;
  while (temp_data_sets != NULL) {
    gtk_list_store_append (GTK_LIST_STORE(model), &iter);  /* Acquire an iterator */
    gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			COLUMN_DATA_SET_NAME, AMITK_OBJECT_NAME(temp_data_sets->data),
			COLUMN_DATA_SET_POINTER, temp_data_sets->data, -1);
    if ( ((tb_math->ds1 == NULL) && (count == 0))  ||
	 (tb_math->ds1 == temp_data_sets->data))
      gtk_tree_selection_select_iter (selection, &iter);
    count++;
    temp_data_sets = temp_data_sets->next;
  }
  
  /* update the data set 2 */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tb_math->list_ds2));
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tb_math->list_ds2));
  gtk_list_store_clear(GTK_LIST_STORE(model));  /* make sure the list is clear */

  temp_data_sets = data_sets;
  count = 0;
  while (temp_data_sets != NULL) {
    gtk_list_store_append (GTK_LIST_STORE(model), &iter);  /* Acquire an iterator */
    gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			COLUMN_DATA_SET_NAME, AMITK_OBJECT_NAME(temp_data_sets->data),
			COLUMN_DATA_SET_POINTER, temp_data_sets->data, -1);
    if (((tb_math->ds2 == NULL) && (tb_math->ds1 != temp_data_sets->data))  ||
	(tb_math->ds2 == temp_data_sets->data))
    if (count == 1)
      gtk_tree_selection_select_iter (selection, &iter);
    count++;
    temp_data_sets = temp_data_sets->next;
  }

  if (data_sets != NULL)
    data_sets = amitk_objects_unref(data_sets);

  return;
}

static void operation_update_model(tb_math_t * tb_math) {

  GtkTreeIter iter;
  GtkTreeModel * model;
  AmitkOperation i_operation;
  GtkTreeSelection *selection;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tb_math->list_operation));
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tb_math->list_operation));

  gtk_list_store_clear(GTK_LIST_STORE(model));  /* make sure the list is clear */

  /* put in the possible operations */
  for (i_operation=0; i_operation < AMITK_OPERATION_NUM; i_operation++) {
    gtk_list_store_append (GTK_LIST_STORE(model), &iter);  /* Acquire an iterator */
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
		       COLUMN_OPERATION_NAME, amitk_operation_get_name(i_operation),
		       COLUMN_OPERATION_NUMBER, i_operation, -1);
    if (i_operation == tb_math->operation)
      gtk_tree_selection_select_iter (selection, &iter);
  }

  return;
}


static void data_set_selection_changed_cb(GtkTreeSelection * selection, gpointer data) {

  tb_math_t * tb_math = data;
  AmitkDataSet * ds;
  gboolean which_ds;
  gboolean can_continue;
  GtkTreeIter iter;
  GtkTreeModel * model;

  which_ds = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(selection), "which_ds"));

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {

    gtk_tree_model_get(model, &iter, COLUMN_DATA_SET_POINTER, &ds, -1);
    g_return_if_fail(AMITK_IS_DATA_SET(ds));

    switch(which_ds) {
    case 0:
      if (tb_math->ds1 != NULL) amitk_object_unref(tb_math->ds1);
      tb_math->ds1 = amitk_object_ref(ds);
      break;
    case 1:
      if (tb_math->ds2 != NULL) amitk_object_unref(tb_math->ds2);
      tb_math->ds2 = amitk_object_ref(ds);
      break;
    default:
      g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
      break;
    }
  }
  
  can_continue = ((tb_math->ds1 != NULL) &&
		  (tb_math->ds2 != NULL) &&
		  (tb_math->ds1 != tb_math->ds2));

  gtk_assistant_set_page_complete(GTK_ASSISTANT(tb_math->dialog),
				  tb_math->page[DATA_SETS_PAGE], 
				  can_continue);

  return;
}

static void operation_selection_changed_cb(GtkTreeSelection * selection, gpointer data) {

  tb_math_t * tb_math = data;
  AmitkOperation operation;
  GtkTreeIter iter;
  GtkTreeModel * model;

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {

    gtk_tree_model_get(model, &iter, COLUMN_OPERATION_NUMBER, &operation, -1);

    g_return_if_fail(operation >= 0);
    g_return_if_fail(operation < AMITK_OPERATION_NUM);
    
    tb_math->operation = operation;

  }

  return;
}

static void by_frames_cb(GtkWidget * widget, gpointer data) {
  tb_math_t * tb_math = data;
  tb_math->by_frames = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  return;
}


static void prepare_page_cb(GtkAssistant * wizard, GtkWidget * page, gpointer data) {
 
  tb_math_t * tb_math = data;
  which_page_t which_page;
  gchar * temp_string;

  which_page = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(page), "which_page"));

  switch(which_page) {
  case DATA_SETS_PAGE:
    data_sets_update_model(tb_math);
    //    gtk_assistant_set_page_complete(GTK_ASSISTANT(tb_math->dialog), page, 
    //				    (tb_math->ds1 != NULL) && (tb_math->ds2 != NULL) && (tb_math->ds1 != tb_math->ds2));
    break;
  case OPERATION_PAGE:
    operation_update_model(tb_math); 
    break;
  case CONCLUSION_PAGE:
    temp_string = g_strdup_printf(_("A new data set will be created with the math operation, press Finish to calculate this data set, or Cancel to quit."));
    gtk_label_set_text(GTK_LABEL(page), temp_string);
    g_free(temp_string);
    break;
  default:
    break;
  }

  return;
}


/* function called when the finish button is hit */
static void apply_cb(GtkAssistant * assistant, gpointer data) {
  tb_math_t * tb_math = data;
  AmitkDataSet * output_ds;

  /* sanity check */
  g_return_if_fail(tb_math->ds1 != NULL);
  g_return_if_fail(tb_math->ds2 != NULL);

  /* apply the math */
  output_ds = amitk_data_sets_math(tb_math->ds1, 
				   tb_math->ds2, 
				   tb_math->operation,
				   tb_math->by_frames,
				   amitk_progress_dialog_update,
				   tb_math->progress_dialog);
  if (output_ds != NULL) {
    amitk_object_add_child(AMITK_OBJECT(tb_math->study), AMITK_OBJECT(output_ds));
    amitk_object_unref(output_ds);
  } else {
    g_warning(_("Math operation failed - results not added to study"));
  }

  return;
}



/* function called to cancel the dialog */
static void close_cb(GtkAssistant * assistant, gpointer data) {

  tb_math_t * tb_math = data;
  GtkWidget * dialog = tb_math->dialog;

  tb_math = tb_math_free(tb_math); /* trash collection */
  gtk_widget_destroy(dialog);

  return;
}




/* destroy a math data structure */
static tb_math_t * tb_math_free(tb_math_t * tb_math) {

  gboolean return_val;

  g_return_val_if_fail(tb_math != NULL, NULL);

  /* sanity checks */
  g_return_val_if_fail(tb_math->reference_count > 0, NULL);

  /* remove a reference count */
  tb_math->reference_count--;

  /* things to do if we've removed all reference's */
  if (tb_math->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing tb_math\n");
#endif
    
    if (tb_math->study != NULL) {
      tb_math->study = amitk_object_unref(tb_math->study);
      tb_math->study = NULL;
    }

    if (tb_math->ds1 != NULL) {
      amitk_object_unref(tb_math->ds1);
      tb_math->ds1 = NULL;
    }

    if (tb_math->ds2 != NULL) {
      amitk_object_unref(tb_math->ds2);
      tb_math->ds2 = NULL;
    }

    if (tb_math->progress_dialog != NULL) {
      g_signal_emit_by_name(G_OBJECT(tb_math->progress_dialog), "delete_event", NULL, &return_val);
      tb_math->progress_dialog = NULL;
    }
    
    g_free(tb_math);
    tb_math = NULL;
  }

  return tb_math;

}

/* allocate and initialize a math data structure */
static tb_math_t * tb_math_init(void) {

  tb_math_t * tb_math;

  /* alloc space for the data structure for passing ui info */
  if ((tb_math = g_try_new(tb_math_t,1)) == NULL) {
    g_warning(_("couldn't allocate memory space for tb_math_t"));
    return NULL;
  }

  tb_math->reference_count = 1;
  tb_math->dialog = NULL;
  tb_math->study = NULL;
  tb_math->ds1 = NULL;
  tb_math->ds2 = NULL;
  tb_math->operation = AMITK_OPERATION_ADD;
  tb_math->by_frames = FALSE;

  return tb_math;
}


static GtkWidget * create_data_sets_page(tb_math_t * tb_math) {

  GtkWidget * table;
  GtkWidget * vseparator;
  GtkListStore * store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;

  table = gtk_table_new(3,3,FALSE);
    
  /* the first data set */
  store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  tb_math->list_ds1 = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref(store);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes(_("Data Set 1:"), renderer,
						    "text", COLUMN_DATA_SET_NAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tb_math->list_ds1), column);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tb_math->list_ds1));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_object_set_data(G_OBJECT(selection), "which_ds", GINT_TO_POINTER(0));
  g_signal_connect(G_OBJECT(selection), "changed",
		   G_CALLBACK(data_set_selection_changed_cb), tb_math);

  gtk_table_attach(GTK_TABLE(table),tb_math->list_ds1, 0,1,0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL | GTK_EXPAND,X_PADDING, Y_PADDING);


  vseparator = gtk_vseparator_new();
  gtk_table_attach(GTK_TABLE(table), vseparator, 1,2,0,2, 0, GTK_FILL, X_PADDING, Y_PADDING);
  
  /* the second data set */
  store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  tb_math->list_ds2 = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref(store);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes(_("Data Set 2:"), renderer,
						    "text", COLUMN_DATA_SET_NAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tb_math->list_ds2), column);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tb_math->list_ds2));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_object_set_data(G_OBJECT(selection), "which_ds", GINT_TO_POINTER(1));
  g_signal_connect(G_OBJECT(selection), "changed",
		   G_CALLBACK(data_set_selection_changed_cb), tb_math);

  gtk_table_attach(GTK_TABLE(table),tb_math->list_ds2, 2,3,0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL | GTK_EXPAND,X_PADDING, Y_PADDING);

  return table;
}

static GtkWidget * create_operation_page(tb_math_t * tb_math) {

  GtkWidget * table;
  GtkListStore * store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkWidget * check_button;

  table = gtk_table_new(3,2,FALSE);
    
  store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
  tb_math->list_operation = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref(store);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes(_("Math Operation"), renderer,
						    "text", COLUMN_OPERATION_NAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tb_math->list_operation), column);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tb_math->list_operation));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect(G_OBJECT(selection), "changed",
		   G_CALLBACK(operation_selection_changed_cb), tb_math);
  gtk_table_attach(GTK_TABLE(table),tb_math->list_operation, 0,1,0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL | GTK_EXPAND,X_PADDING, Y_PADDING);

  check_button = gtk_check_button_new_with_label ("Do operation frame-by-frame (default is by time)");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), tb_math->by_frames);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(by_frames_cb),tb_math);
  gtk_table_attach(GTK_TABLE(table), check_button,0,1,1,2,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(check_button);

  return table;
}



/* function that sets up an image math dialog */
void tb_math(AmitkStudy * study, GtkWindow * parent) {

  tb_math_t * tb_math;
  GdkPixbuf * logo;
  guint count;
  GList * data_sets;
  gint i;
  
  g_return_if_fail(AMITK_IS_STUDY(study));
  
  tb_math = tb_math_init();
  tb_math->study = amitk_object_ref(AMITK_OBJECT(study));

  tb_math->dialog = gtk_assistant_new();
  gtk_window_set_transient_for(GTK_WINDOW(tb_math->dialog), parent);
  gtk_window_set_destroy_with_parent(GTK_WINDOW(tb_math->dialog), TRUE);
  g_signal_connect(G_OBJECT(tb_math->dialog), "cancel", G_CALLBACK(close_cb), tb_math);
  g_signal_connect(G_OBJECT(tb_math->dialog), "close", G_CALLBACK(close_cb), tb_math);
  g_signal_connect(G_OBJECT(tb_math->dialog), "apply", G_CALLBACK(apply_cb), tb_math);
  g_signal_connect(G_OBJECT(tb_math->dialog), "prepare",  G_CALLBACK(prepare_page_cb), tb_math);


  tb_math->progress_dialog = amitk_progress_dialog_new(GTK_WINDOW(tb_math->dialog));

  /* --------------- initial page ------------------ */
  /* figure out how many data sets there are */
  data_sets = amitk_object_get_children_of_type(AMITK_OBJECT(tb_math->study), AMITK_OBJECT_TYPE_DATA_SET, TRUE);
  count = amitk_data_sets_count(data_sets, TRUE);
  if (data_sets != NULL) data_sets = amitk_objects_unref(data_sets);

  tb_math->page[INTRO_PAGE]= gtk_label_new((count >= 2) ? _(start_page_text) : _(data_set_error_page_text));
  gtk_widget_set_size_request(tb_math->page[INTRO_PAGE],LABEL_WIDTH, -1);
  gtk_label_set_line_wrap(GTK_LABEL(tb_math->page[INTRO_PAGE]), TRUE);
  gtk_assistant_append_page(GTK_ASSISTANT(tb_math->dialog), tb_math->page[INTRO_PAGE]);
  gtk_assistant_set_page_title(GTK_ASSISTANT(tb_math->dialog), tb_math->page[INTRO_PAGE], 
			       _("Data Set Math Wizard"));
  gtk_assistant_set_page_type(GTK_ASSISTANT(tb_math->dialog), tb_math->page[INTRO_PAGE],
			      GTK_ASSISTANT_PAGE_INTRO);
  gtk_assistant_set_page_complete(GTK_ASSISTANT(tb_math->dialog), tb_math->page[INTRO_PAGE], count >= 2);


  /*------------------ pick your data set page ------------------ */
  tb_math->page[DATA_SETS_PAGE] = create_data_sets_page(tb_math);
  gtk_assistant_append_page(GTK_ASSISTANT(tb_math->dialog), tb_math->page[DATA_SETS_PAGE]);
  gtk_assistant_set_page_title(GTK_ASSISTANT(tb_math->dialog), tb_math->page[DATA_SETS_PAGE], 
			       _("Data Set Selection"));


  /*------------------ pick your operation page ------------------ */
  tb_math->page[OPERATION_PAGE] = create_operation_page(tb_math);
  gtk_assistant_append_page(GTK_ASSISTANT(tb_math->dialog), tb_math->page[OPERATION_PAGE]);
  gtk_assistant_set_page_title(GTK_ASSISTANT(tb_math->dialog), tb_math->page[OPERATION_PAGE], 
			       _("Operation Selection"));
  gtk_assistant_set_page_complete(GTK_ASSISTANT(tb_math->dialog),tb_math->page[OPERATION_PAGE], TRUE); /* we always have one selected */


  /* ----------------  conclusion page ---------------------------------- */
  tb_math->page[CONCLUSION_PAGE] = gtk_label_new("");
  gtk_widget_set_size_request(tb_math->page[CONCLUSION_PAGE],LABEL_WIDTH, -1);
  gtk_label_set_line_wrap(GTK_LABEL(tb_math->page[CONCLUSION_PAGE]), TRUE);
  gtk_assistant_append_page(GTK_ASSISTANT(tb_math->dialog), tb_math->page[CONCLUSION_PAGE]);
  gtk_assistant_set_page_title(GTK_ASSISTANT(tb_math->dialog), tb_math->page[CONCLUSION_PAGE], 
			       _("Conclusion"));
  gtk_assistant_set_page_type(GTK_ASSISTANT(tb_math->dialog), tb_math->page[CONCLUSION_PAGE],
			      GTK_ASSISTANT_PAGE_CONFIRM);
  gtk_assistant_set_page_complete(GTK_ASSISTANT(tb_math->dialog), tb_math->page[CONCLUSION_PAGE], TRUE); /* always set to complete here */

  logo = gtk_widget_render_icon(GTK_WIDGET(tb_math->dialog), "amide_icon_logo", GTK_ICON_SIZE_DIALOG, 0);
  for (i=0; i<NUM_PAGES; i++) {
    gtk_assistant_set_page_header_image(GTK_ASSISTANT(tb_math->dialog), tb_math->page[i], logo);
    g_object_set_data(G_OBJECT(tb_math->page[i]),"which_page", GINT_TO_POINTER(i));
  }
  g_object_unref(logo);

  gtk_widget_show_all(tb_math->dialog);
  return;
}













