/* tb_alignment.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2014 Andy Loening
 * except mutual information addition Copyright (C) 2011-2012 Ian Miller
 *
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
#include "alignment_mutual_information.h"
#include "alignment_procrustes.h"
#include "tb_alignment.h"
#include "ui_common.h"


#define LABEL_WIDTH 375

static gchar * data_set_error_page_text = 
N_("There is only one data set in this study.  There needs "
   "to be at least two data sets to perform an alignment");

static gchar * fiducial_marks_error_page_text =
N_("In order to perform an alignment, each data set must "
   "have at least three objects with the same name as "
   "the corresponding three objects in the other data set."
   "\n\n"
   "Please see the help documentation for a longer "
   "explanation as to how alignments can be done.");

static gchar * start_page_text = 
N_("Welcome to the data set alignment wizard, used for "
   "aligning one medical image data set with another. "
   "\n\n"
#ifdef AMIDE_LIBGSL_SUPPORT
   "Currently, only rigid body registration using either "
   "fiducial marks, or maximization of mutual information "
   "has been implemented inside of AMIDE. "
#else
   "This program was built without libgsl support, as such "
   "registration utilizing fiducial marks is not supported."
#endif
   "\n\n"
   "The mutual information algorithm is run on the "
   "currently displayed slices, not the whole data "
   "sets.");


typedef enum {
  COLUMN_DATA_SET_NAME,
  COLUMN_DATA_SET_POINTER,
  NUM_DATA_SET_COLUMNS
} data_set_column_t;

typedef enum {
  COLUMN_POINT_SELECTED,
  COLUMN_POINT_NAME,
  COLUMN_POINT_POINTER,
  NUM_POINT_COLUMNS
} point_column_t;

typedef enum {
  INTRO_PAGE, 
  ALIGNMENT_TYPE_PAGE,  
  DATA_SETS_PAGE,
  FIDUCIAL_MARKS_PAGE,
  NO_FIDUCIAL_MARKS_PAGE,
  CONCLUSION_PAGE, 
  NUM_PAGES
} which_page_t;

typedef enum {
#ifdef AMIDE_LIBGSL_SUPPORT
  PROCRUSTES,
#endif
  MUTUAL_INFORMATION,
  NUM_ALIGNMENT_TYPES
} which_alignment_t;

static gchar * alignment_names[] = {
#ifdef AMIDE_LIBGSL_SUPPORT
  N_("Fiducial Markers"),
#endif
  N_("Mutual Information")
};

/* data structures */
typedef struct tb_alignment_t {
  GtkWidget * dialog;
  GtkWidget * page[NUM_PAGES];
  GtkWidget * list_moving_ds;
  GtkWidget * list_fixed_ds;
  GtkWidget * list_points;
  GtkWidget * progress_dialog;

  GList * data_sets;
  AmitkDataSet * moving_ds;
  AmitkDataSet * fixed_ds;
  which_alignment_t alignment_type;
  GList * selected_marks;
  AmitkSpace * transform_space; /* the new coordinate space for the moving volume */
  amide_time_t view_start_time;
  amide_time_t view_duration;
  AmitkPoint view_center;
  amide_real_t view_thickness;
  

  guint reference_count;
} tb_alignment_t;


static tb_alignment_t * tb_alignment_free(tb_alignment_t * alignment);
static tb_alignment_t * tb_alignment_init(void);

static void apply_cb(GtkAssistant * assistant, gpointer data);
static void close_cb(GtkAssistant * assistant, gpointer data);

static void data_sets_update_model(tb_alignment_t * alignment);
static void points_update_model(tb_alignment_t * alignment);
static void alignment_type_changed_cb(GtkRadioButton * clicked_button, gpointer data );
static void data_set_selection_changed_cb(GtkTreeSelection * selection, gpointer data);
static gboolean points_button_press_event(GtkWidget * list, GdkEventButton * event, gpointer data);

static GtkWidget * create_alignment_type_page(tb_alignment_t * tb_alignment);
static GtkWidget * create_data_sets_page(tb_alignment_t * tb_alignment);
static GtkWidget * create_fiducial_marks_page(tb_alignment_t * tb_alignment);

static gint forward_page_function (gint current_page, gpointer data);
static void prepare_page_cb(GtkAssistant * wizard, GtkWidget * page, gpointer data);


/* destroy a alignment data structure */
static tb_alignment_t * tb_alignment_free(tb_alignment_t * tb_alignment) {

  gboolean return_val;

  g_return_val_if_fail(tb_alignment != NULL, NULL);

  /* sanity checks */
  g_return_val_if_fail(tb_alignment->reference_count > 0, NULL);

  /* remove a reference count */
  tb_alignment->reference_count--;

  /* things to do if we've removed all reference's */
  if (tb_alignment->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing tb_alignment\n");
#endif
    
    if (tb_alignment->data_sets != NULL)
      tb_alignment->data_sets = amitk_objects_unref(tb_alignment->data_sets);

    if (tb_alignment->moving_ds != NULL) {
      amitk_object_unref(tb_alignment->moving_ds);
      tb_alignment->moving_ds = NULL;
    }

    if (tb_alignment->fixed_ds != NULL) {
      amitk_object_unref(tb_alignment->fixed_ds);
      tb_alignment->fixed_ds = NULL;
    }

    if (tb_alignment->selected_marks != NULL)
      tb_alignment->selected_marks = amitk_objects_unref(tb_alignment->selected_marks);

    if (tb_alignment->transform_space != NULL) {
      g_object_unref(tb_alignment->transform_space);
      tb_alignment->transform_space = NULL;
    }

    if (tb_alignment->progress_dialog != NULL) {
      g_signal_emit_by_name(G_OBJECT(tb_alignment->progress_dialog), "delete_event", NULL, &return_val);
      tb_alignment->progress_dialog = NULL;
    }


    g_free(tb_alignment);
    tb_alignment = NULL;
  }

  return tb_alignment;

}

/* allocate and initialize an alignment data structure */
static tb_alignment_t * tb_alignment_init(void) {

  tb_alignment_t * tb_alignment;

  /* alloc space for the data structure for passing ui info */
  if ((tb_alignment = g_try_new(tb_alignment_t,1)) == NULL) {
    g_warning(_("couldn't allocate memory space for tb_alignment_t"));
    return NULL;
  }

  tb_alignment->reference_count = 1;
  tb_alignment->dialog = NULL;
  tb_alignment->progress_dialog = NULL;
  tb_alignment->moving_ds = NULL;
  tb_alignment->fixed_ds = NULL;
  tb_alignment->alignment_type = 0; /* PROCRUSTES if with GSL support */
  tb_alignment->selected_marks = NULL;
  tb_alignment->transform_space = NULL;

  return tb_alignment;
}

/* function called when the finish button is hit */
static void apply_cb(GtkAssistant * assistant, gpointer data) {
  tb_alignment_t * tb_alignment = data;

  /* sanity check */
  g_return_if_fail(tb_alignment->transform_space != NULL);

  /* apply the alignment transform */
  amitk_space_transform(AMITK_SPACE(tb_alignment->moving_ds), tb_alignment->transform_space);
  
  return;
}



/* function called to cancel the dialog */
static void close_cb(GtkAssistant * assistant, gpointer data) {
  tb_alignment_t * tb_alignment = data;
  GtkWidget * dialog = tb_alignment->dialog;

  tb_alignment = tb_alignment_free(tb_alignment); /* trash collection */
  gtk_widget_destroy(dialog);

  return;
}


static void data_sets_update_model(tb_alignment_t * tb_alignment) {

  GtkTreeIter iter;
  GtkTreeModel * model;
  GtkTreeSelection *selection;
  GList * data_sets;
  gint count;

  /* update the moving data set */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tb_alignment->list_moving_ds));
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tb_alignment->list_moving_ds));
  gtk_list_store_clear(GTK_LIST_STORE(model));  /* make sure the list is clear */

  data_sets = tb_alignment->data_sets;
  count = 0;
  while (data_sets != NULL) {
    gtk_list_store_append (GTK_LIST_STORE(model), &iter);  /* Acquire an iterator */
    gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			COLUMN_DATA_SET_NAME, AMITK_OBJECT_NAME(data_sets->data),
			COLUMN_DATA_SET_POINTER, data_sets->data, -1);
    if ( ((tb_alignment->moving_ds == NULL) && (count == 0))  ||
	 (tb_alignment->moving_ds == data_sets->data))
      gtk_tree_selection_select_iter (selection, &iter);
    count++;
    data_sets = data_sets->next;
  }
  
  /* update the fixed data set */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tb_alignment->list_fixed_ds));
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tb_alignment->list_fixed_ds));
  gtk_list_store_clear(GTK_LIST_STORE(model));  /* make sure the list is clear */

  data_sets = tb_alignment->data_sets;
  count = 0;
  while (data_sets != NULL) {
    gtk_list_store_append (GTK_LIST_STORE(model), &iter);  /* Acquire an iterator */
    gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			COLUMN_DATA_SET_NAME, AMITK_OBJECT_NAME(data_sets->data),
			COLUMN_DATA_SET_POINTER, data_sets->data, -1);
    if (((tb_alignment->fixed_ds == NULL) && (tb_alignment->moving_ds != data_sets->data))  ||
	(tb_alignment->fixed_ds == data_sets->data))
    if (count == 1)
      gtk_tree_selection_select_iter (selection, &iter);
    count++;
    data_sets = data_sets->next;
  }


  return;
}

static void points_update_model(tb_alignment_t * tb_alignment) {

  GtkTreeIter iter;
  GList * fiducial_marks;
  GList * selected_marks;
  GtkTreeModel * model;
  gboolean selected;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tb_alignment->list_points));

  gtk_list_store_clear(GTK_LIST_STORE(model));  /* make sure the list is clear */

  /* put in new pairs of points */
  fiducial_marks = AMITK_OBJECT_CHILDREN(tb_alignment->fixed_ds);

  selected_marks = tb_alignment->selected_marks;
  tb_alignment->selected_marks = NULL;

  while (fiducial_marks != NULL) {
    if (AMITK_IS_FIDUCIAL_MARK(fiducial_marks->data) || (AMITK_IS_VOLUME(fiducial_marks->data))) {
      if (amitk_objects_find_object_by_name(AMITK_OBJECT_CHILDREN(tb_alignment->moving_ds),
					    AMITK_OBJECT_NAME(fiducial_marks->data))) {
	if ((amitk_objects_find_object_by_name(selected_marks, AMITK_OBJECT_NAME(fiducial_marks->data))) ||
	    (selected_marks == NULL))
	  selected = TRUE;
	else 
	  selected = FALSE;
	
	gtk_list_store_append (GTK_LIST_STORE(model), &iter);  /* Acquire an iterator */
	gtk_list_store_set(GTK_LIST_STORE(model), &iter,
			   COLUMN_POINT_SELECTED, selected,
			   COLUMN_POINT_NAME, AMITK_OBJECT_NAME(fiducial_marks->data), 
			   COLUMN_POINT_POINTER, fiducial_marks->data, -1);
	if (selected)
	  tb_alignment->selected_marks = g_list_append(tb_alignment->selected_marks, amitk_object_ref(fiducial_marks->data));
	
      }
    }
    fiducial_marks = fiducial_marks->next;
  }
  selected_marks = amitk_objects_unref(selected_marks);

  return;
}

static void alignment_type_changed_cb(GtkRadioButton * rb, gpointer data ) {
  
  tb_alignment_t * tb_alignment = data;

  g_return_if_fail(tb_alignment != NULL);

  tb_alignment->alignment_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(rb), "alignment_type"));

  return;
}

static void data_set_selection_changed_cb(GtkTreeSelection * selection, gpointer data) {

  tb_alignment_t * tb_alignment = data;
  AmitkDataSet * ds;
  gboolean fixed_ds;
  gboolean can_continue;
  GtkTreeIter iter;
  GtkTreeModel * model;

  fixed_ds = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(selection), "fixed"));

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {

    gtk_tree_model_get(model, &iter, COLUMN_DATA_SET_POINTER, &ds, -1);
    g_return_if_fail(AMITK_IS_DATA_SET(ds));

    if (fixed_ds) {
      if (tb_alignment->fixed_ds != NULL) amitk_object_unref(tb_alignment->fixed_ds);
      tb_alignment->fixed_ds = amitk_object_ref(ds);
    } else {
      if (tb_alignment->moving_ds != NULL) amitk_object_unref(tb_alignment->moving_ds);
      tb_alignment->moving_ds = amitk_object_ref(ds);
    }
  } else {
    if (fixed_ds) {
      if (tb_alignment->fixed_ds != NULL) amitk_object_unref(tb_alignment->fixed_ds);
      tb_alignment->fixed_ds = NULL;
    } else {
      if (tb_alignment->moving_ds != NULL) amitk_object_unref(tb_alignment->moving_ds);
      tb_alignment->moving_ds = NULL;
    }
  }
  
  can_continue = ((tb_alignment->fixed_ds != NULL) &&
		  (tb_alignment->moving_ds != NULL) &&
		  (tb_alignment->fixed_ds != tb_alignment->moving_ds));

  gtk_assistant_set_page_complete(GTK_ASSISTANT(tb_alignment->dialog),
				  tb_alignment->page[DATA_SETS_PAGE], 
				  can_continue);
  return;
}


static gboolean points_button_press_event(GtkWidget * list, GdkEventButton * event, gpointer data) {

  tb_alignment_t * tb_alignment = data;
  GtkTreePath * path=NULL;
  AmitkObject * point;
  gboolean can_continue;
  GtkTreeIter iter;
  GtkTreeModel * model;
  gboolean toggled;
  gint cell_x, cell_y;

  switch(event->button) {
  case 1: /* left button */
    if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(list), event->x, event->y, 
				      &path, NULL, &cell_x, &cell_y)) {
      model = gtk_tree_view_get_model(GTK_TREE_VIEW(list));
      gtk_tree_model_get_iter(model, &iter, path);
      gtk_tree_path_free(path);
      gtk_tree_model_get(model, &iter, COLUMN_POINT_SELECTED, &toggled, 
			 COLUMN_POINT_POINTER, &point, -1);
      g_return_val_if_fail(AMITK_IS_FIDUCIAL_MARK(point) || AMITK_IS_VOLUME(point), FALSE);

      toggled = !toggled;
      gtk_list_store_set(GTK_LIST_STORE(model), &iter, COLUMN_POINT_SELECTED, toggled, -1);
      if (toggled) {
	tb_alignment->selected_marks = g_list_append(tb_alignment->selected_marks, amitk_object_ref(point));
      } else {
	tb_alignment->selected_marks = g_list_remove(tb_alignment->selected_marks, point);
	amitk_object_unref(point);
      }
    }
    break;

  default: 
    /* do nothing */
    break;

  }

  can_continue = (amitk_objects_count(tb_alignment->selected_marks) >= 3);

  gtk_assistant_set_page_complete(GTK_ASSISTANT(tb_alignment->dialog),
				  tb_alignment->page[FIDUCIAL_MARKS_PAGE], 
				  can_continue);

  return FALSE;
}


/* Ian Miller addition */
/* this sets up the user interface for the "Alignment type" page/section of the wizard */
static GtkWidget * create_alignment_type_page(tb_alignment_t * tb_alignment) {
  GtkWidget * vbox;
  GtkWidget * rb[NUM_ALIGNMENT_TYPES];
  which_alignment_t i_alignment;

  vbox = gtk_vbox_new (TRUE, 2);
  for (i_alignment = 0; i_alignment < NUM_ALIGNMENT_TYPES; i_alignment ++) {
    if (i_alignment == 0)
      rb[i_alignment] =  gtk_radio_button_new_with_label(NULL, alignment_names[i_alignment]);
    else
      rb[i_alignment] =  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb[0]), alignment_names[i_alignment]);
    gtk_box_pack_start (GTK_BOX (vbox), rb[i_alignment], TRUE, TRUE, 2);
    g_object_set_data(G_OBJECT(rb[i_alignment]), "alignment_type", GINT_TO_POINTER(i_alignment));
  }

  /* set which toggle button is pressed in before connecting signals */
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb[tb_alignment->alignment_type]), TRUE);

  for (i_alignment = 0; i_alignment < NUM_ALIGNMENT_TYPES; i_alignment ++) {
    g_signal_connect(G_OBJECT(rb[i_alignment]), "clicked", G_CALLBACK(alignment_type_changed_cb), tb_alignment);
  }
  
  return vbox;
}

static GtkWidget * create_data_sets_page(tb_alignment_t * tb_alignment) {
  GtkWidget * table;
  GtkListStore * store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkWidget * vseparator;

  table = gtk_table_new(3,3,FALSE);
    
  /* the moving data set */
  store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  tb_alignment->list_moving_ds = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref(store);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes(_("Align Data Set: (moving)"), renderer,
						    "text", COLUMN_DATA_SET_NAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tb_alignment->list_moving_ds), column);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tb_alignment->list_moving_ds));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_object_set_data(G_OBJECT(selection), "fixed", GINT_TO_POINTER(FALSE));
  g_signal_connect(G_OBJECT(selection), "changed",
		   G_CALLBACK(data_set_selection_changed_cb), tb_alignment);

  gtk_table_attach(GTK_TABLE(table),tb_alignment->list_moving_ds, 0,1,0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL | GTK_EXPAND,X_PADDING, Y_PADDING);


  vseparator = gtk_vseparator_new();
  gtk_table_attach(GTK_TABLE(table), vseparator, 1,2,0,2, 0, GTK_FILL, X_PADDING, Y_PADDING);
  
  /* the fixed data set */
  store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  tb_alignment->list_fixed_ds = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref(store);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes(_("Align Data Set: (fixed)"), renderer,
						    "text", COLUMN_DATA_SET_NAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tb_alignment->list_fixed_ds), column);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tb_alignment->list_fixed_ds));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_object_set_data(G_OBJECT(selection), "fixed", GINT_TO_POINTER(TRUE));
  g_signal_connect(G_OBJECT(selection), "changed",
		   G_CALLBACK(data_set_selection_changed_cb), tb_alignment);

  gtk_table_attach(GTK_TABLE(table),tb_alignment->list_fixed_ds, 2,3,0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL | GTK_EXPAND,X_PADDING, Y_PADDING);

  return table;
}

static GtkWidget * create_fiducial_marks_page(tb_alignment_t * tb_alignment) {

  GtkWidget * table;
  GtkListStore * store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  
  table = gtk_table_new(2,2,FALSE);

  store = gtk_list_store_new(3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_POINTER);
  tb_alignment->list_points = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref(store);
  g_signal_connect(G_OBJECT(tb_alignment->list_points), "button_press_event",
		   G_CALLBACK(points_button_press_event), tb_alignment);


  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes("Select", renderer,
						    "active", COLUMN_POINT_SELECTED, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tb_alignment->list_points), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes(_("Points for Alignment"), renderer,
						    "text", COLUMN_POINT_NAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tb_alignment->list_points), column);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tb_alignment->list_points));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);
  gtk_table_attach(GTK_TABLE(table),tb_alignment->list_points, 0,1,0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL | GTK_EXPAND,X_PADDING, Y_PADDING);

  return table;
}

static gint forward_page_function (gint current_page, gpointer data) {

  tb_alignment_t * tb_alignment=data;
  gint num_pairs=0;

  switch (current_page) {
  case ALIGNMENT_TYPE_PAGE:
    return DATA_SETS_PAGE;
    break;
  case DATA_SETS_PAGE:
    if (tb_alignment->alignment_type == MUTUAL_INFORMATION) return CONCLUSION_PAGE;
    if ((tb_alignment->fixed_ds != NULL) && (tb_alignment->moving_ds != NULL)) 
      num_pairs = amitk_objects_count_pairs_by_name(AMITK_OBJECT_CHILDREN(tb_alignment->fixed_ds),
						    AMITK_OBJECT_CHILDREN(tb_alignment->moving_ds));
    if (num_pairs < 3) 
      return NO_FIDUCIAL_MARKS_PAGE;
    else
      return FIDUCIAL_MARKS_PAGE;
    break;
  case FIDUCIAL_MARKS_PAGE:
    return CONCLUSION_PAGE;
    break;
  default:
    return current_page+1;
    break;
  }
}



static void prepare_page_cb(GtkAssistant * wizard, GtkWidget * page, gpointer data) {
 
  tb_alignment_t * tb_alignment = data;
  which_page_t which_page;
  which_alignment_t which_alignment;
  gdouble performance_metric;
  gchar * temp_string;

  which_page = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(page), "which_page"));
  which_alignment = tb_alignment->alignment_type;

  switch(which_page) {
  case DATA_SETS_PAGE:
    data_sets_update_model(tb_alignment);
    gtk_assistant_set_page_complete(GTK_ASSISTANT(tb_alignment->dialog), page, 
				    (tb_alignment->fixed_ds != NULL) && (tb_alignment->moving_ds != NULL) && (tb_alignment->fixed_ds != tb_alignment->moving_ds));
    break;
  case FIDUCIAL_MARKS_PAGE:
    points_update_model(tb_alignment); 
    gtk_assistant_set_page_complete(GTK_ASSISTANT(tb_alignment->dialog),
				    tb_alignment->page[FIDUCIAL_MARKS_PAGE], 
				    amitk_objects_count(tb_alignment->selected_marks) >= 3);
    break;
  case CONCLUSION_PAGE:
    /* calculate the alignment */
    switch(which_alignment) {
#ifdef AMIDE_LIBGSL_SUPPORT
    case PROCRUSTES:
      tb_alignment->transform_space = alignment_procrustes(tb_alignment->moving_ds, 
							   tb_alignment->fixed_ds, 
							   tb_alignment->selected_marks,
							   &performance_metric);
      temp_string = g_strdup_printf(_("The alignment has been calculated, press Finish to apply, or Cancel to quit.\n\nThe calculated fiducial reference error is:\n\t %5.2f mm/point"), 
				    performance_metric);
      break;
#endif
    case MUTUAL_INFORMATION:
      tb_alignment->transform_space = alignment_mutual_information(tb_alignment->moving_ds, 
								   tb_alignment->fixed_ds,
								   tb_alignment->view_center,
								   tb_alignment->view_thickness,
								   tb_alignment->view_start_time,
								   tb_alignment->view_duration,
								   &performance_metric,
      								   amitk_progress_dialog_update,
      								   tb_alignment->progress_dialog);
      temp_string = g_strdup_printf(_("The alignment has been calculated, press Finish to apply, or Cancel to quit.\n\nThe calculated mutual information metric is:\n\t %5.2f"),
				    performance_metric);
      break;
    default:
      g_return_if_reached();
      break;
    }
    gtk_label_set_text(GTK_LABEL(page), temp_string);
    g_free(temp_string);
    gtk_assistant_set_page_complete(GTK_ASSISTANT(tb_alignment->dialog), page, TRUE);
    break;
  case NO_FIDUCIAL_MARKS_PAGE:
  default:
    break;
  }

  return;
}



/* function that sets up an align point dialog */
void tb_alignment(AmitkStudy * study, GtkWindow * parent) {

  tb_alignment_t * tb_alignment;
  GdkPixbuf * logo;
  guint count;
  gint i;
  
  g_return_if_fail(AMITK_IS_STUDY(study));

  tb_alignment = tb_alignment_init();
  tb_alignment->data_sets = 
    amitk_object_get_children_of_type(AMITK_OBJECT(study), AMITK_OBJECT_TYPE_DATA_SET, TRUE);

  tb_alignment->view_start_time = AMITK_STUDY_VIEW_START_TIME(study);
  tb_alignment->view_duration = AMITK_STUDY_VIEW_DURATION(study);
  tb_alignment->view_center = AMITK_STUDY_VIEW_CENTER(study);
  tb_alignment->view_thickness = AMITK_STUDY_VIEW_THICKNESS(study);

  tb_alignment->dialog = gtk_assistant_new();
  gtk_window_set_transient_for(GTK_WINDOW(tb_alignment->dialog), parent);
  gtk_window_set_destroy_with_parent(GTK_WINDOW(tb_alignment->dialog), TRUE);
  g_signal_connect(G_OBJECT(tb_alignment->dialog), "cancel", G_CALLBACK(close_cb), tb_alignment);
  g_signal_connect(G_OBJECT(tb_alignment->dialog), "close", G_CALLBACK(close_cb), tb_alignment);
  g_signal_connect(G_OBJECT(tb_alignment->dialog), "apply", G_CALLBACK(apply_cb), tb_alignment);
  g_signal_connect(G_OBJECT(tb_alignment->dialog), "prepare",  G_CALLBACK(prepare_page_cb), tb_alignment);
  gtk_assistant_set_forward_page_func(GTK_ASSISTANT(tb_alignment->dialog),
				      forward_page_function,
				      tb_alignment, NULL);
  
  tb_alignment->progress_dialog = amitk_progress_dialog_new(GTK_WINDOW(tb_alignment->dialog));

  /* --------------- initial page ------------------ */
  count = amitk_data_sets_count(tb_alignment->data_sets, TRUE);
  tb_alignment->page[INTRO_PAGE] = gtk_label_new((count >= 2) ? _(start_page_text) : _(data_set_error_page_text));
  gtk_widget_set_size_request(tb_alignment->page[INTRO_PAGE],LABEL_WIDTH, -1);
  gtk_label_set_line_wrap(GTK_LABEL(tb_alignment->page[INTRO_PAGE]), TRUE);
  gtk_assistant_append_page(GTK_ASSISTANT(tb_alignment->dialog), tb_alignment->page[INTRO_PAGE]);
  gtk_assistant_set_page_title(GTK_ASSISTANT(tb_alignment->dialog), tb_alignment->page[INTRO_PAGE], 
			       _("Data Set Alignment Wizard"));
  gtk_assistant_set_page_type(GTK_ASSISTANT(tb_alignment->dialog), tb_alignment->page[INTRO_PAGE],
			      GTK_ASSISTANT_PAGE_INTRO);
  gtk_assistant_set_page_complete(GTK_ASSISTANT(tb_alignment->dialog), tb_alignment->page[INTRO_PAGE], count >= 2);

  /*------------------ pick your alignment type page ------------------ */
  tb_alignment->page[ALIGNMENT_TYPE_PAGE] = create_alignment_type_page(tb_alignment);
  gtk_assistant_append_page(GTK_ASSISTANT(tb_alignment->dialog), tb_alignment->page[ALIGNMENT_TYPE_PAGE]);
  gtk_assistant_set_page_title(GTK_ASSISTANT(tb_alignment->dialog), tb_alignment->page[ALIGNMENT_TYPE_PAGE], 
			       _("Alignment Type Selection"));
  gtk_assistant_set_page_complete(GTK_ASSISTANT(tb_alignment->dialog), tb_alignment->page[ALIGNMENT_TYPE_PAGE], TRUE);


  /*------------------ pick your data set page ------------------ */
  tb_alignment->page[DATA_SETS_PAGE] = create_data_sets_page(tb_alignment);
  gtk_assistant_append_page(GTK_ASSISTANT(tb_alignment->dialog), tb_alignment->page[DATA_SETS_PAGE]);
  gtk_assistant_set_page_title(GTK_ASSISTANT(tb_alignment->dialog), tb_alignment->page[DATA_SETS_PAGE], 
			       _("Data Set Selection"));

  /*------------------ pick your fiducial marks page ------------------ */
  tb_alignment->page[FIDUCIAL_MARKS_PAGE] = create_fiducial_marks_page(tb_alignment);
  gtk_assistant_append_page(GTK_ASSISTANT(tb_alignment->dialog), tb_alignment->page[FIDUCIAL_MARKS_PAGE]);
  gtk_assistant_set_page_title(GTK_ASSISTANT(tb_alignment->dialog), tb_alignment->page[FIDUCIAL_MARKS_PAGE], 
			       _("Fiducial Marks Selection"));

  /* --------------- page shown if no fiducial marks ------------------ */
  tb_alignment->page[NO_FIDUCIAL_MARKS_PAGE] = gtk_label_new(_(fiducial_marks_error_page_text));
  gtk_widget_set_size_request(tb_alignment->page[NO_FIDUCIAL_MARKS_PAGE],LABEL_WIDTH, -1);
  gtk_label_set_line_wrap(GTK_LABEL(tb_alignment->page[NO_FIDUCIAL_MARKS_PAGE]), TRUE);
  gtk_assistant_append_page(GTK_ASSISTANT(tb_alignment->dialog), tb_alignment->page[NO_FIDUCIAL_MARKS_PAGE]);
  gtk_assistant_set_page_title(GTK_ASSISTANT(tb_alignment->dialog), tb_alignment->page[NO_FIDUCIAL_MARKS_PAGE], 
			       _("Alignment Error"));

  /* ----------------  conclusion page ---------------------------------- */
  tb_alignment->page[CONCLUSION_PAGE] = gtk_label_new("");
  gtk_widget_set_size_request(tb_alignment->page[CONCLUSION_PAGE],LABEL_WIDTH, -1);
  gtk_label_set_line_wrap(GTK_LABEL(tb_alignment->page[CONCLUSION_PAGE]), TRUE);
  gtk_assistant_append_page(GTK_ASSISTANT(tb_alignment->dialog), tb_alignment->page[CONCLUSION_PAGE]);
  gtk_assistant_set_page_title(GTK_ASSISTANT(tb_alignment->dialog), tb_alignment->page[CONCLUSION_PAGE], 
			       _("Conclusion"));
  gtk_assistant_set_page_type(GTK_ASSISTANT(tb_alignment->dialog), tb_alignment->page[CONCLUSION_PAGE],
			      GTK_ASSISTANT_PAGE_CONFIRM);

  /* things for all page */
  logo = gtk_widget_render_icon(GTK_WIDGET(tb_alignment->dialog), "amide_icon_logo", GTK_ICON_SIZE_DIALOG, 0);
  for (i=0; i<NUM_PAGES; i++) {
    gtk_assistant_set_page_header_image(GTK_ASSISTANT(tb_alignment->dialog), tb_alignment->page[i], logo);
    g_object_set_data(G_OBJECT(tb_alignment->page[i]),"which_page", GINT_TO_POINTER(i));
  }
  g_object_unref(logo);

  gtk_widget_show_all(tb_alignment->dialog);

  return;
}







