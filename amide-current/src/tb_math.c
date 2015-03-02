/* tb_math.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2006-2007 Andy Loening
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
#include <libgnomeui/libgnomeui.h>
#include "tb_math.h"
#include "pixmaps.h"




static gchar * data_set_error_page_text = 
N_("There is only one data set in this study.  There needs "
   "to be at least two data sets to perform mathematical operations");

static gchar * start_page_text = 
N_("Welcome to the data set math wizard, used for "
   "performaing mathematical operations between medical "
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
  GtkWidget * druid;
  GtkWidget * pages[NUM_PAGES];
  GtkWidget * list_ds1;
  GtkWidget * list_ds2;
  GtkWidget * list_operation;

  AmitkStudy * study;
  AmitkDataSet * ds1;
  AmitkDataSet * ds2;
  AmitkOperation operation;

  guint reference_count;
} tb_math_t;


static void cancel_cb(GtkWidget* widget, gpointer data);
static void destroy_cb(GtkObject * object, gpointer data);
static gboolean delete_event_cb(GtkWidget * widget, GdkEvent * event, gpointer data);
static tb_math_t * tb_math_free(tb_math_t * math);
static tb_math_t * tb_math_init(void);


static void data_sets_update_model(tb_math_t * math);
static void operation_update_model(tb_math_t * math);
static void prepare_page_cb(GtkWidget * page, gpointer * druid, gpointer data);
static void data_set_selection_changed_cb(GtkTreeSelection * selection, gpointer data);
static void operation_selection_changed_cb(GtkTreeSelection * selection, gpointer data);
static void finish_cb(GtkWidget* widget, gpointer druid, gpointer data);




static void data_sets_update_model(tb_math_t * math) {

  GtkTreeIter iter;
  GtkTreeModel * model;
  GtkTreeSelection *selection;
  GList * data_sets;
  GList * temp_data_sets;
  gint count;

  /* update data set 1 */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (math->list_ds1));
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(math->list_ds1));
  gtk_list_store_clear(GTK_LIST_STORE(model));  /* make sure the list is clear */

  data_sets = amitk_object_get_children_of_type(AMITK_OBJECT(math->study), AMITK_OBJECT_TYPE_DATA_SET, TRUE);

  count = 0;
  temp_data_sets = data_sets;
  while (temp_data_sets != NULL) {
    gtk_list_store_append (GTK_LIST_STORE(model), &iter);  /* Acquire an iterator */
    gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			COLUMN_DATA_SET_NAME, AMITK_OBJECT_NAME(temp_data_sets->data),
			COLUMN_DATA_SET_POINTER, temp_data_sets->data, -1);
    if ( ((math->ds1 == NULL) && (count == 0))  ||
	 (math->ds1 == temp_data_sets->data))
      gtk_tree_selection_select_iter (selection, &iter);
    count++;
    temp_data_sets = temp_data_sets->next;
  }
  
  /* update the data set 2 */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (math->list_ds2));
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(math->list_ds2));
  gtk_list_store_clear(GTK_LIST_STORE(model));  /* make sure the list is clear */

  temp_data_sets = data_sets;
  count = 0;
  while (temp_data_sets != NULL) {
    gtk_list_store_append (GTK_LIST_STORE(model), &iter);  /* Acquire an iterator */
    gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			COLUMN_DATA_SET_NAME, AMITK_OBJECT_NAME(temp_data_sets->data),
			COLUMN_DATA_SET_POINTER, temp_data_sets->data, -1);
    if (((math->ds2 == NULL) && (math->ds1 != temp_data_sets->data))  ||
	(math->ds2 == temp_data_sets->data))
    if (count == 1)
      gtk_tree_selection_select_iter (selection, &iter);
    count++;
    temp_data_sets = temp_data_sets->next;
  }

  if (data_sets != NULL)
    data_sets = amitk_objects_unref(data_sets);

  return;
}

static void operation_update_model(tb_math_t * math) {

  GtkTreeIter iter;
  GtkTreeModel * model;
  AmitkOperation i_operation;
  GtkTreeSelection *selection;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (math->list_operation));
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(math->list_operation));

  gtk_list_store_clear(GTK_LIST_STORE(model));  /* make sure the list is clear */

  /* put in the possible operations */
  for (i_operation=0; i_operation < AMITK_OPERATION_NUM; i_operation++) {
    gtk_list_store_append (GTK_LIST_STORE(model), &iter);  /* Acquire an iterator */
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
		       COLUMN_OPERATION_NAME, amitk_operation_get_name(i_operation),
		       COLUMN_OPERATION_NUMBER, i_operation, -1);
    if (i_operation == math->operation)
      gtk_tree_selection_select_iter (selection, &iter);
  }

  return;
}


static void prepare_page_cb(GtkWidget * page, gpointer * druid, gpointer data) {
 
  tb_math_t * math = data;
  which_page_t which_page;
  gboolean can_continue;
  gchar * temp_string;

  which_page = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(page), "which_page"));

  switch(which_page) {
  case DATA_SETS_PAGE:
    data_sets_update_model(math);
    can_continue = ((math->ds1 != NULL) &&
		    (math->ds2 != NULL) &&
		    (math->ds1 != math->ds2));
    gnome_druid_set_buttons_sensitive(GNOME_DRUID(druid), TRUE, can_continue, TRUE, TRUE);
    break;
  case OPERATION_PAGE:
    operation_update_model(math); 
    gnome_druid_set_buttons_sensitive(GNOME_DRUID(druid), TRUE, TRUE, TRUE, TRUE);
    break;
  case CONCLUSION_PAGE:
    temp_string = g_strdup_printf(_("A new data set will be created with the math operation, press Finish to calculate this data set, or Cancel to quit."));
    gnome_druid_page_edge_set_text(GNOME_DRUID_PAGE_EDGE(page), temp_string);
    g_free(temp_string);
    gnome_druid_set_show_finish(GNOME_DRUID(druid), TRUE);
    break;
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    break;
  }

  return;
}

static void data_set_selection_changed_cb(GtkTreeSelection * selection, gpointer data) {

  tb_math_t * math = data;
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
      if (math->ds1 != NULL) amitk_object_unref(math->ds1);
      math->ds1 = amitk_object_ref(ds);
      break;
    case 1:
      if (math->ds2 != NULL) amitk_object_unref(math->ds2);
      math->ds2 = amitk_object_ref(ds);
      break;
    default:
      g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
      break;
    }
  }
  
  can_continue = ((math->ds1 != NULL) &&
		  (math->ds2 != NULL) &&
		  (math->ds1 != math->ds2));
  gnome_druid_set_buttons_sensitive(GNOME_DRUID(math->druid), TRUE, can_continue, TRUE, TRUE);

  return;
}

static void operation_selection_changed_cb(GtkTreeSelection * selection, gpointer data) {

  tb_math_t * math = data;
  AmitkOperation operation;
  GtkTreeIter iter;
  GtkTreeModel * model;

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {

    gtk_tree_model_get(model, &iter, COLUMN_OPERATION_NUMBER, &operation, -1);

    g_return_if_fail(operation >= 0);
    g_return_if_fail(operation < AMITK_OPERATION_NUM);
    
    math->operation = operation;

  }

  return;
}


/* function called when the finish button is hit */
static void finish_cb(GtkWidget* widget, gpointer druid, gpointer data) {
  tb_math_t * math = data;
  AmitkDataSet * output_ds;

  /* sanity check */
  g_return_if_fail(math->ds1 != NULL);
  g_return_if_fail(math->ds2 != NULL);

  /* apply the math */
  output_ds = amitk_data_sets_math(math->ds1, math->ds2, math->operation);
  if (output_ds != NULL) {
    amitk_object_add_child(AMITK_OBJECT(math->study), AMITK_OBJECT(output_ds));
    amitk_object_unref(output_ds);
  } else {
    g_warning(_("Math operation failed - results not added to study"));
  }

  /* close the dialog box */
  cancel_cb(widget, data);

  return;
}



/* function called to cancel the dialog */
static void cancel_cb(GtkWidget* widget, gpointer data) {

  tb_math_t * math = data;
  GtkWidget * dialog = math->dialog;
  gboolean return_val;

  /* run the delete event function */
  g_signal_emit_by_name(G_OBJECT(dialog), "delete_event", NULL, &return_val);
  if (!return_val) gtk_widget_destroy(dialog);

  return;
}


static void destroy_cb(GtkObject * object, gpointer data) {
  tb_math_t * math = data;
  math = tb_math_free(math); /* trash collection */
  return;
}

/* function called on a delete event */
static gboolean delete_event_cb(GtkWidget * widget, GdkEvent * event, gpointer data) {
  return FALSE;
}






/* destroy a math data structure */
static tb_math_t * tb_math_free(tb_math_t * math) {

  g_return_val_if_fail(math != NULL, NULL);

  /* sanity checks */
  g_return_val_if_fail(math->reference_count > 0, NULL);

  /* remove a reference count */
  math->reference_count--;

  /* things to do if we've removed all reference's */
  if (math->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing math\n");
#endif
    
    if (math->study != NULL) {
      math->study = amitk_object_unref(math->study);
      math->study = NULL;
    }

    if (math->ds1 != NULL) {
      amitk_object_unref(math->ds1);
      math->ds1 = NULL;
    }

    if (math->ds2 != NULL) {
      amitk_object_unref(math->ds2);
      math->ds2 = NULL;
    }

    g_free(math);
    math = NULL;
  }

  return math;

}

/* allocate and initialize a math data structure */
static tb_math_t * tb_math_init(void) {

  tb_math_t * math;

  /* alloc space for the data structure for passing ui info */
  if ((math = g_try_new(tb_math_t,1)) == NULL) {
    g_warning(_("couldn't allocate memory space for tb_math_t"));
    return NULL;
  }

  math->reference_count = 1;
  math->dialog = NULL;
  math->druid = NULL;
  math->study = NULL;
  math->ds1 = NULL;
  math->ds2 = NULL;
  math->operation = AMITK_OPERATION_ADD;

  return math;
}


/* function that sets up an image math dialog */
void tb_math(AmitkStudy * study, GtkWindow * parent) {

  tb_math_t * math;
  GdkPixbuf * logo;
  guint count;
  GtkWidget * table;
  GtkWidget * vseparator;
  GtkListStore * store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GList * data_sets;
  
  g_return_if_fail(AMITK_IS_STUDY(study));
  
  logo = gdk_pixbuf_new_from_inline(-1, amide_logo_small, FALSE, NULL);

  math = tb_math_init();
  math->study = amitk_object_ref(AMITK_OBJECT(study));

  math->dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_transient_for(GTK_WINDOW(math->dialog), parent);
  gtk_window_set_destroy_with_parent(GTK_WINDOW(math->dialog), TRUE);
  g_signal_connect(G_OBJECT(math->dialog), "delete_event", G_CALLBACK(delete_event_cb), math);
  g_signal_connect(G_OBJECT(math->dialog), "destroy", G_CALLBACK(destroy_cb), math);

  math->druid = gnome_druid_new();
  gtk_container_add(GTK_CONTAINER(math->dialog), math->druid);
  g_signal_connect(G_OBJECT(math->druid), "cancel", 
		   G_CALLBACK(cancel_cb), math);


  /* --------------- initial page ------------------ */
  /* figure out how many data sets there are */
  data_sets = amitk_object_get_children_of_type(AMITK_OBJECT(math->study), AMITK_OBJECT_TYPE_DATA_SET, TRUE);
  count = amitk_data_sets_count(data_sets, TRUE);
  if (data_sets != NULL) data_sets = amitk_objects_unref(data_sets);

  math->pages[INTRO_PAGE]= 
    gnome_druid_page_edge_new_with_vals(GNOME_EDGE_START,TRUE, 
					"Data Set Math Wizard",
					 (count >= 2) ? start_page_text : data_set_error_page_text,
					 logo, NULL, NULL);
  gnome_druid_append_page(GNOME_DRUID(math->druid), 
			  GNOME_DRUID_PAGE(math->pages[INTRO_PAGE]));
  if (count < 2) gnome_druid_set_buttons_sensitive(GNOME_DRUID(math->druid), FALSE, FALSE, TRUE, TRUE);
  else gnome_druid_set_buttons_sensitive(GNOME_DRUID(math->druid), FALSE, TRUE, TRUE, TRUE);


  /*------------------ pick your data set page ------------------ */
  math->pages[DATA_SETS_PAGE] = 
    gnome_druid_page_standard_new_with_vals("Data Set Selection",logo, NULL);
  gnome_druid_append_page(GNOME_DRUID(math->druid), 
			  GNOME_DRUID_PAGE(math->pages[DATA_SETS_PAGE]));
  g_object_set_data(G_OBJECT(math->pages[DATA_SETS_PAGE]), 
		    "which_page", GINT_TO_POINTER(DATA_SETS_PAGE));
  g_signal_connect(G_OBJECT(math->pages[DATA_SETS_PAGE]), "prepare", 
		   G_CALLBACK(prepare_page_cb), math);

  table = gtk_table_new(3,3,FALSE);
  gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(math->pages[DATA_SETS_PAGE])->vbox), 
		     table, TRUE, TRUE, 5);
    
    
  /* the first data set */
  store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  math->list_ds1 = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref(store);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes(_("Data Set 1:"), renderer,
						    "text", COLUMN_DATA_SET_NAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (math->list_ds1), column);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (math->list_ds1));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_object_set_data(G_OBJECT(selection), "which_ds", GINT_TO_POINTER(0));
  g_signal_connect(G_OBJECT(selection), "changed",
		   G_CALLBACK(data_set_selection_changed_cb), math);

  gtk_table_attach(GTK_TABLE(table),math->list_ds1, 0,1,0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL | GTK_EXPAND,X_PADDING, Y_PADDING);


  vseparator = gtk_vseparator_new();
  gtk_table_attach(GTK_TABLE(table), vseparator, 1,2,0,2, 0, GTK_FILL, X_PADDING, Y_PADDING);
  
  /* the second data set */
  store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  math->list_ds2 = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref(store);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes(_("Data Set 2:"), renderer,
						    "text", COLUMN_DATA_SET_NAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (math->list_ds2), column);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (math->list_ds2));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_object_set_data(G_OBJECT(selection), "which_ds", GINT_TO_POINTER(1));
  g_signal_connect(G_OBJECT(selection), "changed",
		   G_CALLBACK(data_set_selection_changed_cb), math);

  gtk_table_attach(GTK_TABLE(table),math->list_ds2, 2,3,0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL | GTK_EXPAND,X_PADDING, Y_PADDING);


  /*------------------ pick your operation page ------------------ */
  math->pages[OPERATION_PAGE] = 
    gnome_druid_page_standard_new_with_vals(_("Operation Selection"), logo, NULL);
  g_object_set_data(G_OBJECT(math->pages[OPERATION_PAGE]), 
		    "which_page", GINT_TO_POINTER(OPERATION_PAGE));
  gnome_druid_append_page(GNOME_DRUID(math->druid), 
			  GNOME_DRUID_PAGE(math->pages[OPERATION_PAGE]));
  g_signal_connect(G_OBJECT(math->pages[OPERATION_PAGE]), "prepare", 
		   G_CALLBACK(prepare_page_cb), math);

  table = gtk_table_new(2,2,FALSE);
  gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(math->pages[OPERATION_PAGE])->vbox), 
		     table, TRUE, TRUE, 5);
    
    
  store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
  math->list_operation = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref(store);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes(_("Math Operation"), renderer,
						    "text", COLUMN_OPERATION_NAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (math->list_operation), column);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (math->list_operation));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect(G_OBJECT(selection), "changed",
		   G_CALLBACK(operation_selection_changed_cb), math);
  gtk_table_attach(GTK_TABLE(table),math->list_operation, 0,1,0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL | GTK_EXPAND,X_PADDING, Y_PADDING);
  

  /* ----------------  conclusion page ---------------------------------- */
  math->pages[CONCLUSION_PAGE] = 
    gnome_druid_page_edge_new_with_vals(GNOME_EDGE_FINISH, TRUE,
					_("Conclusion"), NULL,logo, NULL, NULL);
  g_object_set_data(G_OBJECT(math->pages[CONCLUSION_PAGE]), 
		    "which_page", GINT_TO_POINTER(CONCLUSION_PAGE));
  g_signal_connect(G_OBJECT(math->pages[CONCLUSION_PAGE]), "prepare", 
		   G_CALLBACK(prepare_page_cb), math);
  g_signal_connect(G_OBJECT(math->pages[CONCLUSION_PAGE]), "finish",
		   G_CALLBACK(finish_cb), math);
  gnome_druid_append_page(GNOME_DRUID(math->druid), 
			  GNOME_DRUID_PAGE(math->pages[CONCLUSION_PAGE]));


  g_object_unref(logo);
  gtk_widget_show_all(math->dialog);
  return;
}













