/* tb_alignment.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2003 Andy Loening
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
#include "alignment.h"
#include "tb_alignment.h"
#include "ui_common.h"
#include "pixmaps.h"




#ifdef AMIDE_LIBGSL_SUPPORT

static gchar * data_set_error_page_text = 
"There is only one data set in this study.  There needs "
"to be at least two data sets to perform an alignment";

static gchar * fiducial_marks_error_page_text =
"There must exist at least 3 pairs of objects that "
"can be used as fiducial marks between the two "
"data sets in order to perform an alignment.";

static gchar * end_page_text =
"The alignment has been calculated, press Finish to "
"apply, or Cancel to quit";

static gchar * start_page_text = 
"Welcome to the data set alignment wizard, used for "
"aligning one medical image data set with another. "
"\n"
"Currently, only registration using fiducial marks has "
"been implemented inside of AMIDE.";

#else /* no LIBGSL support */

static gchar * start_page_text = 
"Welcome to the data set alignment wizard, used for "
"aligning one medical image data set with another. "
"\n"
"Currently, only registration using fiducial markers has "
"been implemented inside of AMIDE.  This feature requires, "
"support from the GNU Scientific Library (libgsl).  This "
"copy of AMIDE has not been compiled with support for "
"libgsl, so it cannot perform registration. ";

#endif /* NO LIBGSL SUPPORT */



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
  DATA_SETS_PAGE, 
  FIDUCIAL_MARKS_PAGE, 
  CONCLUSION_PAGE, 
  NO_FIDUCIAL_MARKS_PAGE,
  NUM_PAGES
} which_page_t;

/* data structures */
typedef struct tb_alignment_t {
  GtkWidget * dialog;
  GtkWidget * druid;
  GtkWidget * pages[NUM_PAGES];
  GtkWidget * list_moving_ds;
  GtkWidget * list_fixed_ds;
  GtkWidget * list_points;

  GList * data_sets;
  AmitkDataSet * moving_ds;
  AmitkDataSet * fixed_ds;
  GList * selected_marks;
  AmitkSpace * transform_space; /* the new coordinate space for the moving volume */

  guint reference_count;
} tb_alignment_t;


static void cancel_cb(GtkWidget* widget, gpointer data);
static gboolean delete_event(GtkWidget * widget, GdkEvent * event, gpointer data);
static tb_alignment_t * tb_alignment_free(tb_alignment_t * alignment);
static tb_alignment_t * tb_alignment_init(void);


#ifdef AMIDE_LIBGSL_SUPPORT
static gboolean next_page_cb(GtkWidget * page, gpointer *druid, gpointer data);
static gboolean back_page_cb(GtkWidget * page, gpointer *druid, gpointer data);
static void prepare_page_cb(GtkWidget * page, gpointer * druid, gpointer data);
static void data_set_selection_changed_cb(GtkTreeSelection * selection, gpointer data);
static gboolean points_button_press_event(GtkWidget * list, GdkEventButton * event, gpointer data);
static void finish_cb(GtkWidget* widget, gpointer druid, gpointer data);





static gboolean next_page_cb(GtkWidget * page, gpointer *druid, gpointer data) {

  tb_alignment_t * alignment = data;
  which_page_t which_page;
  gboolean nonlinear = FALSE;
  gint num_pairs;

  which_page = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(page), "which_page"));

  switch(which_page) {
  case DATA_SETS_PAGE:
    num_pairs = amitk_objects_count_pairs_by_name(AMITK_OBJECT_CHILDREN(alignment->fixed_ds),
						  AMITK_OBJECT_CHILDREN(alignment->moving_ds));
    if (num_pairs < 3) {
      nonlinear = TRUE;
      gnome_druid_set_page(GNOME_DRUID(druid), 
			   GNOME_DRUID_PAGE(alignment->pages[NO_FIDUCIAL_MARKS_PAGE]));
    }
    break;
  default:
    g_warning("unexpected case in %s at line %d", __FILE__, __LINE__);
    break;
  }

  return nonlinear;
}

static gboolean back_page_cb(GtkWidget * page, gpointer *druid, gpointer data) {

  tb_alignment_t * alignment = data;
  which_page_t which_page;
  gboolean nonlinear = FALSE;

  which_page = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(page), "which_page"));

  switch(which_page) {
  case NO_FIDUCIAL_MARKS_PAGE:
    nonlinear = TRUE;
    gnome_druid_set_page(GNOME_DRUID(druid), 
			 GNOME_DRUID_PAGE(alignment->pages[DATA_SETS_PAGE]));
    break;
  default:
    g_warning("unexpected case in %s at line %d", __FILE__, __LINE__);
    break;
  }

  return nonlinear;
}

static void data_sets_update_model(tb_alignment_t * alignment) {

  GtkTreeIter iter;
  GtkTreeModel * model;
  GtkTreeSelection *selection;
  GList * data_sets;
  gint count;

  /* update the moving data set */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (alignment->list_moving_ds));
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(alignment->list_moving_ds));
  gtk_list_store_clear(GTK_LIST_STORE(model));  /* make sure the list is clear */

  data_sets = alignment->data_sets;
  count = 0;
  while (data_sets != NULL) {
    gtk_list_store_append (GTK_LIST_STORE(model), &iter);  /* Acquire an iterator */
    gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			COLUMN_DATA_SET_NAME, AMITK_OBJECT_NAME(data_sets->data),
			COLUMN_DATA_SET_POINTER, data_sets->data, -1);
    if ( ((alignment->moving_ds == NULL) && (count == 0))  ||
	 (alignment->moving_ds == data_sets->data))
      gtk_tree_selection_select_iter (selection, &iter);
    count++;
    data_sets = data_sets->next;
  }
  
  /* update the fixed data set */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (alignment->list_fixed_ds));
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(alignment->list_fixed_ds));
  gtk_list_store_clear(GTK_LIST_STORE(model));  /* make sure the list is clear */

  data_sets = alignment->data_sets;
  count = 0;
  while (data_sets != NULL) {
    gtk_list_store_append (GTK_LIST_STORE(model), &iter);  /* Acquire an iterator */
    gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			COLUMN_DATA_SET_NAME, AMITK_OBJECT_NAME(data_sets->data),
			COLUMN_DATA_SET_POINTER, data_sets->data, -1);
    if (((alignment->fixed_ds == NULL) && (alignment->moving_ds != data_sets->data))  ||
	(alignment->fixed_ds == data_sets->data))
    if (count == 1)
      gtk_tree_selection_select_iter (selection, &iter);
    count++;
    data_sets = data_sets->next;
  }


  return;
}

static void points_update_model(tb_alignment_t * alignment) {

  GtkTreeIter iter;
  GList * fiducial_marks;
  GList * selected_marks;
  GtkTreeModel * model;
  gboolean selected;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(alignment->list_points));

  gtk_list_store_clear(GTK_LIST_STORE(model));  /* make sure the list is clear */

  /* put in new pairs of points */
  fiducial_marks = AMITK_OBJECT_CHILDREN(alignment->fixed_ds);

  selected_marks = alignment->selected_marks;
  alignment->selected_marks = NULL;

  while (fiducial_marks != NULL) {
    if (AMITK_IS_FIDUCIAL_MARK(fiducial_marks->data) || (AMITK_IS_VOLUME(fiducial_marks->data))) {
      if (amitk_objects_find_object_by_name(AMITK_OBJECT_CHILDREN(alignment->moving_ds),
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
	  alignment->selected_marks = g_list_append(alignment->selected_marks, g_object_ref(fiducial_marks->data));
	
      }
    }
    fiducial_marks = fiducial_marks->next;
  }
  selected_marks = amitk_objects_unref(selected_marks);

  return;
}


static void prepare_page_cb(GtkWidget * page, gpointer * druid, gpointer data) {
 
  tb_alignment_t * alignment = data;
  which_page_t which_page;
  gboolean can_continue;
  gdouble fre;
  gchar * temp_string;

  which_page = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(page), "which_page"));

  switch(which_page) {
  case DATA_SETS_PAGE:
    data_sets_update_model(alignment);
    can_continue = ((alignment->fixed_ds != NULL) &&
		    (alignment->moving_ds != NULL) &&
		    (alignment->fixed_ds != alignment->moving_ds));
    gnome_druid_set_buttons_sensitive(GNOME_DRUID(druid), TRUE, can_continue, TRUE, TRUE);
    break;
  case FIDUCIAL_MARKS_PAGE:
    points_update_model(alignment); 
    can_continue = (amitk_objects_count(alignment->selected_marks) >= 3);
    gnome_druid_set_buttons_sensitive(GNOME_DRUID(druid), TRUE, can_continue, TRUE, TRUE);
    break;
  case CONCLUSION_PAGE:
    /* calculate the alignment */
    alignment->transform_space = alignment_calculate(alignment->moving_ds, 
							alignment->fixed_ds, 
							alignment->selected_marks,&fre);

    temp_string = g_strdup_printf("%s\n\n%s: %5.5f mm/point",
				  end_page_text,
				  "The calculated fiducial reference error is",
				  fre);
    gnome_druid_page_edge_set_text(GNOME_DRUID_PAGE_EDGE(page), temp_string);
    g_free(temp_string);
    gnome_druid_set_show_finish(GNOME_DRUID(druid), TRUE);
    break;
  case NO_FIDUCIAL_MARKS_PAGE:
    /* haven't been able to figure out how to grayout the next button.... */
    gnome_druid_set_buttons_sensitive(GNOME_DRUID(druid), TRUE, FALSE, TRUE, TRUE);
    break;
  default:
    g_warning("unexpected case in %s at line %d", __FILE__, __LINE__);
    break;
  }

  return;
}

static void data_set_selection_changed_cb(GtkTreeSelection * selection, gpointer data) {

  tb_alignment_t * alignment = data;
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
      if (alignment->fixed_ds != NULL) g_object_unref(alignment->fixed_ds);
      alignment->fixed_ds = g_object_ref(ds);
    } else {
      if (alignment->moving_ds != NULL) g_object_unref(alignment->moving_ds);
      alignment->moving_ds = g_object_ref(ds);
    }
  } else {
    if (fixed_ds) {
      if (alignment->fixed_ds != NULL) g_object_unref(alignment->fixed_ds);
      alignment->fixed_ds = NULL;
    } else {
      if (alignment->moving_ds != NULL) g_object_unref(alignment->moving_ds);
      alignment->moving_ds = NULL;
    }
  }
  
  can_continue = ((alignment->fixed_ds != NULL) &&
		  (alignment->moving_ds != NULL) &&
		  (alignment->fixed_ds != alignment->moving_ds));
  gnome_druid_set_buttons_sensitive(GNOME_DRUID(alignment->druid), TRUE, can_continue, TRUE, TRUE);

  return;
}
static gboolean points_button_press_event(GtkWidget * list, GdkEventButton * event, gpointer data) {

  tb_alignment_t * alignment = data;
  GtkTreePath * path=NULL;
  AmitkObject * point;
  guint count;
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
	alignment->selected_marks = g_list_append(alignment->selected_marks, g_object_ref(point));
      } else {
	alignment->selected_marks = g_list_remove(alignment->selected_marks, point);
	g_object_unref(point);
      }
    }
    break;

  default: 
    /* do nothing */
    break;

  }

  count = amitk_objects_count(alignment->selected_marks);
  gnome_druid_set_buttons_sensitive(GNOME_DRUID(alignment->druid), TRUE, (count >= 3), TRUE, TRUE);

  return FALSE;
}


/* function called when the finish button is hit */
static void finish_cb(GtkWidget* widget, gpointer druid, gpointer data) {
  tb_alignment_t * alignment = data;

  /* sanity check */
  g_return_if_fail(alignment->transform_space != NULL);

  /* apply the alignment transform */
  amitk_space_transform(AMITK_SPACE(alignment->moving_ds),alignment->transform_space);

  /* close the dialog box */
  cancel_cb(widget, data);

  return;
}


#endif /* AMIDE_LIBGSL_SUPPORT */






/* function called to cancel the dialog */
static void cancel_cb(GtkWidget* widget, gpointer data) {

  tb_alignment_t * alignment = data;
  GtkWidget * dialog = alignment->dialog;
  gboolean return_val;

  /* run the delete event function */
  g_signal_emit_by_name(G_OBJECT(dialog), "delete_event", NULL, &return_val);
  if (!return_val) gtk_widget_destroy(dialog);

  return;
}

/* function called on a delete event */
static gboolean delete_event(GtkWidget * widget, GdkEvent * event, gpointer data) {

  tb_alignment_t * alignment = data;

  /* trash collection */
  alignment = tb_alignment_free(alignment);

  return FALSE;
}






/* destroy a alignment data structure */
static tb_alignment_t * tb_alignment_free(tb_alignment_t * alignment) {

  g_return_val_if_fail(alignment != NULL, NULL);

  /* sanity checks */
  g_return_val_if_fail(alignment->reference_count > 0, NULL);

  /* remove a reference count */
  alignment->reference_count--;

  /* things to do if we've removed all reference's */
  if (alignment->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing alignment\n");
#endif
    
    if (alignment->data_sets != NULL)
      alignment->data_sets = amitk_objects_unref(alignment->data_sets);

    if (alignment->moving_ds != NULL) {
      g_object_unref(alignment->moving_ds);
      alignment->moving_ds = NULL;
    }

    if (alignment->fixed_ds != NULL) {
      g_object_unref(alignment->fixed_ds);
      alignment->fixed_ds = NULL;
    }

    if (alignment->selected_marks != NULL)
      alignment->selected_marks = amitk_objects_unref(alignment->selected_marks);

    if (alignment->transform_space != NULL) {
      g_object_unref(alignment->transform_space);
      alignment->transform_space = NULL;
    }

    g_free(alignment);
    alignment = NULL;
  }

  return alignment;

}

/* allocate and initialize a alignment data structure */
static tb_alignment_t * tb_alignment_init(void) {

  tb_alignment_t * alignment;

  /* alloc space for the data structure for passing ui info */
  if ((alignment = g_try_new(tb_alignment_t,1)) == NULL) {
    g_warning("couldn't allocate space for tb_alignment_t");
    return NULL;
  }

  alignment->reference_count = 1;
  alignment->dialog = NULL;
  alignment->druid = NULL;
  alignment->moving_ds = NULL;
  alignment->fixed_ds = NULL;
  alignment->selected_marks = NULL;
  alignment->transform_space = NULL;

  return alignment;
}


/* function that sets up an align point dialog */
void tb_alignment(AmitkStudy * study) {

  tb_alignment_t * alignment;
  GdkPixbuf * logo;
#ifdef AMIDE_LIBGSL_SUPPORT
  guint count;
  GtkWidget * table;
  GtkWidget * vseparator;
  GtkListStore * store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
#endif /* AMIDE_LIBGSL_SUPPORT */
  
  g_return_if_fail(AMITK_IS_STUDY(study));
  
  logo = gdk_pixbuf_new_from_xpm_data(amide_logo_xpm);

  alignment = tb_alignment_init();
  alignment->data_sets = 
    amitk_object_get_children_of_type(AMITK_OBJECT(study), AMITK_OBJECT_TYPE_DATA_SET, TRUE);

  alignment->dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(G_OBJECT(alignment->dialog), "delete_event",
		   G_CALLBACK(delete_event), alignment);

  alignment->druid = gnome_druid_new();
  gtk_container_add(GTK_CONTAINER(alignment->dialog), alignment->druid);
  g_signal_connect(G_OBJECT(alignment->druid), "cancel", 
		   G_CALLBACK(cancel_cb), alignment);


  /* --------------- initial page ------------------ */
#ifndef AMIDE_LIBGSL_SUPPORT
  alignment->pages[INTRO_PAGE]= 
    gnome_druid_page_edge_new_with_vals(GNOME_EDGE_START, TRUE,
					"Data Set Alignment Wizard",
					start_page_text, logo, NULL, NULL);
  gnome_druid_append_page(GNOME_DRUID(alignment->druid), 
			  GNOME_DRUID_PAGE(alignment->pages[INTRO_PAGE]));
  gnome_druid_set_buttons_sensitive(GNOME_DRUID(alignment->druid), FALSE, FALSE, TRUE, TRUE);


#else /* #ifdef AMIDE_LIBGSL_SUPPORT */
  /* figure out how many data sets there are */
  count = amitk_data_sets_count(alignment->data_sets, TRUE);

  alignment->pages[INTRO_PAGE]= 
    gnome_druid_page_edge_new_with_vals(GNOME_EDGE_START,TRUE, 
					"Data Set Alignment Wizard",
					 (count >= 2) ? start_page_text : data_set_error_page_text,
					 logo, NULL, NULL);
  gnome_druid_append_page(GNOME_DRUID(alignment->druid), 
			  GNOME_DRUID_PAGE(alignment->pages[INTRO_PAGE]));
  if (count < 2) gnome_druid_set_buttons_sensitive(GNOME_DRUID(alignment->druid), FALSE, FALSE, TRUE, TRUE);
  else gnome_druid_set_buttons_sensitive(GNOME_DRUID(alignment->druid), FALSE, TRUE, TRUE, TRUE);


  /*------------------ pick your data set page ------------------ */
  alignment->pages[DATA_SETS_PAGE] = 
    gnome_druid_page_standard_new_with_vals("Data Set Selection",logo, NULL);
  gnome_druid_append_page(GNOME_DRUID(alignment->druid), 
			  GNOME_DRUID_PAGE(alignment->pages[DATA_SETS_PAGE]));
  g_object_set_data(G_OBJECT(alignment->pages[DATA_SETS_PAGE]), 
		    "which_page", GINT_TO_POINTER(DATA_SETS_PAGE));
  g_signal_connect(G_OBJECT(alignment->pages[DATA_SETS_PAGE]), "next", 
		   G_CALLBACK(next_page_cb), alignment);
  g_signal_connect(G_OBJECT(alignment->pages[DATA_SETS_PAGE]), "prepare", 
		   G_CALLBACK(prepare_page_cb), alignment);

  table = gtk_table_new(3,3,FALSE);
  gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(alignment->pages[DATA_SETS_PAGE])->vbox), 
		     table, TRUE, TRUE, 5);
    
    
  /* the moving data set */
  store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  alignment->list_moving_ds = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref(store);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("Align Data Set: (moving)", renderer,
						    "text", COLUMN_DATA_SET_NAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (alignment->list_moving_ds), column);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (alignment->list_moving_ds));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_object_set_data(G_OBJECT(selection), "fixed", GINT_TO_POINTER(FALSE));
  g_signal_connect(G_OBJECT(selection), "changed",
		   G_CALLBACK(data_set_selection_changed_cb), alignment);

  gtk_table_attach(GTK_TABLE(table),alignment->list_moving_ds, 0,1,0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL | GTK_EXPAND,X_PADDING, Y_PADDING);


  vseparator = gtk_vseparator_new();
  gtk_table_attach(GTK_TABLE(table), vseparator, 1,2,0,2, 0, GTK_FILL, X_PADDING, Y_PADDING);
  
  /* the fixed data set */
  store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
  alignment->list_fixed_ds = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref(store);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("With Data Set: (fixed)", renderer,
						    "text", COLUMN_DATA_SET_NAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (alignment->list_fixed_ds), column);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (alignment->list_fixed_ds));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_object_set_data(G_OBJECT(selection), "fixed", GINT_TO_POINTER(TRUE));
  g_signal_connect(G_OBJECT(selection), "changed",
		   G_CALLBACK(data_set_selection_changed_cb), alignment);

  gtk_table_attach(GTK_TABLE(table),alignment->list_fixed_ds, 2,3,0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL | GTK_EXPAND,X_PADDING, Y_PADDING);


  /*------------------ pick your alignment points page ------------------ */
  alignment->pages[FIDUCIAL_MARKS_PAGE] = 
    gnome_druid_page_standard_new_with_vals("Fiducial Marks Selection", logo, NULL);
  g_object_set_data(G_OBJECT(alignment->pages[FIDUCIAL_MARKS_PAGE]), 
		    "which_page", GINT_TO_POINTER(FIDUCIAL_MARKS_PAGE));
  gnome_druid_append_page(GNOME_DRUID(alignment->druid), 
			  GNOME_DRUID_PAGE(alignment->pages[FIDUCIAL_MARKS_PAGE]));
  g_signal_connect(G_OBJECT(alignment->pages[FIDUCIAL_MARKS_PAGE]), "prepare", 
		   G_CALLBACK(prepare_page_cb), alignment);

  table = gtk_table_new(2,2,FALSE);
  gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(alignment->pages[FIDUCIAL_MARKS_PAGE])->vbox), 
		     table, TRUE, TRUE, 5);
    
    
  store = gtk_list_store_new(3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_POINTER);
  alignment->list_points = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref(store);
  g_signal_connect(G_OBJECT(alignment->list_points), "button_press_event",
		   G_CALLBACK(points_button_press_event), alignment);


  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes("Select", renderer,
						    "active", COLUMN_POINT_SELECTED, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (alignment->list_points), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("Points for Alignment", renderer,
						    "text", COLUMN_POINT_NAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (alignment->list_points), column);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (alignment->list_points));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);
  gtk_table_attach(GTK_TABLE(table),alignment->list_points, 0,1,0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL | GTK_EXPAND,X_PADDING, Y_PADDING);
  

  /* ----------------  conclusion page ---------------------------------- */
  alignment->pages[CONCLUSION_PAGE] = 
    gnome_druid_page_edge_new_with_vals(GNOME_EDGE_FINISH, TRUE,
					"Conclusion", NULL,logo, NULL, NULL);
  g_object_set_data(G_OBJECT(alignment->pages[CONCLUSION_PAGE]), 
		    "which_page", GINT_TO_POINTER(CONCLUSION_PAGE));
  g_signal_connect(G_OBJECT(alignment->pages[CONCLUSION_PAGE]), "prepare", 
		   G_CALLBACK(prepare_page_cb), alignment);
  g_signal_connect(G_OBJECT(alignment->pages[CONCLUSION_PAGE]), "finish",
		   G_CALLBACK(finish_cb), alignment);
  gnome_druid_append_page(GNOME_DRUID(alignment->druid), 
			  GNOME_DRUID_PAGE(alignment->pages[CONCLUSION_PAGE]));

  /* --------------- page shown if no alignment points ------------------ */
  alignment->pages[NO_FIDUCIAL_MARKS_PAGE] =
    gnome_druid_page_edge_new_with_vals(GNOME_EDGE_OTHER, TRUE,
					"Alignment Error", fiducial_marks_error_page_text, 
					logo, NULL, NULL);
  g_object_set_data(G_OBJECT(alignment->pages[NO_FIDUCIAL_MARKS_PAGE]), 
		    "which_page", GINT_TO_POINTER(NO_FIDUCIAL_MARKS_PAGE));
  g_signal_connect(G_OBJECT(alignment->pages[NO_FIDUCIAL_MARKS_PAGE]), "prepare", 
		   G_CALLBACK(prepare_page_cb), alignment);
  g_signal_connect(G_OBJECT(alignment->pages[NO_FIDUCIAL_MARKS_PAGE]), "back", 
		   G_CALLBACK(back_page_cb), alignment);
  gnome_druid_append_page(GNOME_DRUID(alignment->druid), 
			  GNOME_DRUID_PAGE(alignment->pages[NO_FIDUCIAL_MARKS_PAGE]));
#endif /* AMIDE_LIBGSL_SUPPORT */

  g_object_unref(logo);
  gtk_widget_show_all(alignment->dialog);
  return;
}











