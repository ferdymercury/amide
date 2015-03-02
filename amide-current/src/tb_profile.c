/* tb_profile.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2002-2003 Andy Loening
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
#include "amitk_study.h"
#include "amitk_progress_dialog.h"
#include "tb_profile.h"
#include "pixmaps.h"


static const char * wizard_name = N_("Line Profile Wizard");

static const char * error_page_text =
N_("You need at least one data set, and at least"
   "two fiducial marks in the study in order to"
   "generate a line profile");

typedef enum {
  DATA_SET_PAGE,
  FIDUCIAL_MARKS_PAGE,
  OUTPUT_PAGE,
  NUM_PAGES
} which_page_t;


typedef enum {
  COLUMN_DATA_SET_NAME,
  COLUMN_DATA_SET_POINTER,
  NUM_DATA_SET_COLUMNS
} data_set_column_t;

typedef enum {
  COLUMN_FIDUCIAL_MARK_NAME,
  COLUMN_FIDUCIAL_MARK_POINTER,
  NUM_POINT_COLUMNS
} point_column_t;


/* data structures */
typedef struct tb_profile_t {
  GtkWidget * dialog;
  GtkWidget * druid;
  GtkWidget * pages[NUM_PAGES];
  GtkWidget * list_pt1;
  GtkWidget * list_pt2;
  GtkWidget * list_ds;

  AmitkFiducialMark * pt1;
  AmitkFiducialMark * pt2;
  AmitkDataSet * ds;

  GList * data_sets;
  GList * fiducial_marks;

  guint reference_count;
} tb_profile_t;


static void cancel_cb(GtkWidget* widget, gpointer data);
static gboolean delete_event(GtkWidget * widget, GdkEvent * event, gpointer data);
static tb_profile_t * tb_profile_free(tb_profile_t * profile);
static tb_profile_t * tb_profile_init(void);


static void prepare_page_cb(GtkWidget * page, gpointer * druid, gpointer data);
static void data_set_selection_changed_cb(GtkTreeSelection * selection, gpointer data);
static void point_selection_changed_cb(GtkTreeSelection * selection, gpointer data);
static void finish_cb(GtkWidget* widget, gpointer druid, gpointer data);





static void prepare_page_cb(GtkWidget * page, gpointer * druid, gpointer data) {
 
  tb_profile_t * profile = data;
  which_page_t which_page;
  gchar * temp_string;

  which_page = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(page), "which_page"));

  switch(which_page) {
  case DATA_SET_PAGE:
  case FIDUCIAL_MARKS_PAGE:
    break;
  case OUTPUT_PAGE:
    /* calculate the profile */
    // profile->transform_space = amitk_data_set_get_profile(profile->ds,profile->pt1, profile->pt2);

    temp_string = g_strdup_printf(_("Blah blah")); 
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

  tb_profile_t * profile = data;
  AmitkDataSet * ds;
  GtkTreeIter iter;
  GtkTreeModel * model;

  g_print("data set selection cb\n");
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {

    gtk_tree_model_get(model, &iter, COLUMN_DATA_SET_POINTER, &ds, -1);
    g_return_if_fail(AMITK_IS_DATA_SET(ds));

    if (profile->ds != NULL) 
      g_object_unref(profile->ds);
    profile->ds = g_object_ref(ds);
  } else {
    g_print("no pick !\n");
  }

  return;
}

static void point_selection_changed_cb(GtkTreeSelection * selection, gpointer data) {

  tb_profile_t * profile = data;
  AmitkFiducialMark * pt;
  GtkTreeIter iter;
  GtkTreeModel * model;
  gboolean point1;

  point1 = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(selection), "pt1"));

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {

    gtk_tree_model_get(model, &iter, COLUMN_FIDUCIAL_MARK_POINTER, &pt, -1);
    g_return_if_fail(AMITK_IS_FIDUCIAL_MARK(pt));

    if (point1) {
      if (profile->pt1 != NULL) g_object_unref(profile->pt1);
      profile->pt1 = g_object_ref(pt);
    } else { /* point2 */
      if (profile->pt2 != NULL) g_object_unref(profile->pt2);
      profile->pt1 = g_object_ref(pt);
    }
  }

  return;
}


/* function called when the finish button is hit */
static void finish_cb(GtkWidget* widget, gpointer druid, gpointer data) {
  tb_profile_t * profile = data;

  /* sanity check */
  //  g_return_if_fail(profile->transform_space != NULL);

  /* apply the profile transform */
  //  amitk_space_transform(AMITK_SPACE(profile->moving_ds),profile->transform_space);

  /* close the dialog box */
  cancel_cb(widget, data);

  return;
}



/* function called to cancel the dialog */
static void cancel_cb(GtkWidget* widget, gpointer data) {

  tb_profile_t * profile = data;
  GtkWidget * dialog = profile->dialog;
  gboolean return_val;

  /* run the delete event function */
  g_signal_emit_by_name(G_OBJECT(dialog), "delete_event", NULL, &return_val);
  if (!return_val) gtk_widget_destroy(dialog);

  return;
}

/* function called on a delete event */
static gboolean delete_event(GtkWidget * widget, GdkEvent * event, gpointer data) {

  tb_profile_t * profile = data;

  /* trash collection */
  profile = tb_profile_free(profile);

  return FALSE;
}






/* destroy a profile data structure */
static tb_profile_t * tb_profile_free(tb_profile_t * profile) {

  g_return_val_if_fail(profile != NULL, NULL);

  /* sanity checks */
  g_return_val_if_fail(profile->reference_count > 0, NULL);

  /* remove a reference count */
  profile->reference_count--;

  /* things to do if we've removed all reference's */
  if (profile->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing profile\n");
#endif
    
    if (profile->data_sets != NULL)
      profile->data_sets = amitk_objects_unref(profile->data_sets);

    if (profile->fiducial_marks != NULL)
      profile->fiducial_marks = amitk_objects_unref(profile->fiducial_marks);

    if (profile->ds != NULL) {
      g_object_unref(profile->ds);
      profile->ds = NULL;
    }

    if (profile->pt1 != NULL) {
      g_object_unref(profile->pt1);
      profile->pt1 = NULL;
    }

    if (profile->pt2 != NULL) {
      g_object_unref(profile->pt2);
      profile->pt2 = NULL;
    }

    g_free(profile);
    profile = NULL;
  }

  return profile;

}

/* allocate and initialize a profile data structure */
static tb_profile_t * tb_profile_init(void) {

  tb_profile_t * profile;

  /* alloc space for the data structure for passing ui info */
  if ((profile = g_try_new(tb_profile_t,1)) == NULL) {
    g_warning(_("couldn't allocate space for tb_profile_t"));
    return NULL;
  }

  profile->reference_count = 1;
  profile->dialog = NULL;
  profile->druid = NULL;
  profile->ds = NULL;
  profile->pt1 = NULL;
  profile->pt2 = NULL;
  profile->data_sets=NULL;
  profile->fiducial_marks=NULL;

  return profile;
}


/* function that sets up an align point dialog */
void tb_profile(AmitkStudy * study) {

  tb_profile_t * profile;
  GdkPixbuf * logo;
  GtkWidget * table;
  GtkWidget * vseparator;
  GtkListStore * store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GtkTreeModel * model1;
  GtkTreeModel * model2;
  GList * data_sets;
  GList * fiducial_marks;
  gchar * temp_string;

  
  g_return_if_fail(AMITK_IS_STUDY(study));
  
  logo = gdk_pixbuf_new_from_xpm_data(amide_logo_xpm);

  profile = tb_profile_init();
  profile->data_sets = 
    amitk_object_get_children_of_type(AMITK_OBJECT(study), AMITK_OBJECT_TYPE_DATA_SET, TRUE);

  profile->fiducial_marks = 
    amitk_object_get_children_of_type(AMITK_OBJECT(study), AMITK_OBJECT_TYPE_FIDUCIAL_MARK, TRUE);

  profile->dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(G_OBJECT(profile->dialog), "delete_event",
		   G_CALLBACK(delete_event), profile);

  profile->druid = gnome_druid_new();
  gtk_container_add(GTK_CONTAINER(profile->dialog), profile->druid);
  g_signal_connect(G_OBJECT(profile->druid), "cancel", 
		   G_CALLBACK(cancel_cb), profile);


  /* --------------- initial page ------------------ */
  if ((profile->data_sets == NULL) || (profile->fiducial_marks == NULL)) {

    profile->pages[DATA_SET_PAGE]= 
      gnome_druid_page_edge_new_with_vals(GNOME_EDGE_START,TRUE, 
					  wizard_name,
					  error_page_text,
					  logo, NULL, NULL);
    gnome_druid_append_page(GNOME_DRUID(profile->druid), 
			    GNOME_DRUID_PAGE(profile->pages[DATA_SET_PAGE]));
    gnome_druid_set_buttons_sensitive(GNOME_DRUID(profile->druid), FALSE, FALSE, TRUE, TRUE);

  } else {

    /*------------------ pick your data set page ------------------ */
    profile->pages[DATA_SET_PAGE] = 
      gnome_druid_page_standard_new_with_vals("Data Set Selection",logo, NULL);
    gnome_druid_append_page(GNOME_DRUID(profile->druid), 
			    GNOME_DRUID_PAGE(profile->pages[DATA_SET_PAGE]));
    g_object_set_data(G_OBJECT(profile->pages[DATA_SET_PAGE]), 
		      "which_page", GINT_TO_POINTER(DATA_SET_PAGE));
    g_signal_connect(G_OBJECT(profile->pages[DATA_SET_PAGE]), "prepare", 
		     G_CALLBACK(prepare_page_cb), profile);

    table = gtk_table_new(2,2,FALSE);
    gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(profile->pages[DATA_SET_PAGE])->vbox), 
		       table, TRUE, TRUE, 5);
    
    
    /* tree for picking the data set */
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
    profile->list_ds = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
    g_object_unref(store);
    
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes(_("Align Data Set: (moving)"), renderer,
						      "text", COLUMN_DATA_SET_NAME, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (profile->list_ds), column);
    
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (profile->list_ds));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
    g_object_set_data(G_OBJECT(selection), "fixed", GINT_TO_POINTER(FALSE));
    g_signal_connect(G_OBJECT(selection), "changed",
		     G_CALLBACK(data_set_selection_changed_cb), profile);
    
    gtk_table_attach(GTK_TABLE(table),profile->list_ds, 0,1,0,1,
		     GTK_FILL|GTK_EXPAND, GTK_FILL | GTK_EXPAND,X_PADDING, Y_PADDING);
    
    
    /* fill in the table */
    model1 = gtk_tree_view_get_model(GTK_TREE_VIEW(profile->list_ds));
    
    data_sets = profile->data_sets;
    while (data_sets != NULL) {
      gtk_list_store_append (GTK_LIST_STORE(model1), &iter);  /* Acquire an iterator */
      gtk_list_store_set (GTK_LIST_STORE(model1), &iter,
			  COLUMN_DATA_SET_NAME, AMITK_OBJECT_NAME(data_sets->data),
			  COLUMN_DATA_SET_POINTER, data_sets->data, -1);
      data_sets = data_sets->next;
    }
    
    
    
    /*------------------ pick your fiducial marks page ------------------ */
    profile->pages[FIDUCIAL_MARKS_PAGE] = 
      gnome_druid_page_standard_new_with_vals(_("Fiducial Marks Selection"), logo, NULL);
    g_object_set_data(G_OBJECT(profile->pages[FIDUCIAL_MARKS_PAGE]), 
		      "which_page", GINT_TO_POINTER(FIDUCIAL_MARKS_PAGE));
    gnome_druid_append_page(GNOME_DRUID(profile->druid), 
			    GNOME_DRUID_PAGE(profile->pages[FIDUCIAL_MARKS_PAGE]));
    g_signal_connect(G_OBJECT(profile->pages[FIDUCIAL_MARKS_PAGE]), "prepare", 
		     G_CALLBACK(prepare_page_cb), profile);
    
    table = gtk_table_new(3,3,FALSE);
    gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(profile->pages[FIDUCIAL_MARKS_PAGE])->vbox), 
		       table, TRUE, TRUE, 5);

    /* pt1 list */
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
    profile->list_pt1 = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
    g_object_unref(store);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes(_("Point 1"), renderer,
						      "text", COLUMN_FIDUCIAL_MARK_NAME, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (profile->list_pt1), column);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (profile->list_pt1));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
    g_object_set_data(G_OBJECT(selection), "pt1", GINT_TO_POINTER(TRUE));
    g_signal_connect(G_OBJECT(selection), "changed",
		     G_CALLBACK(point_selection_changed_cb), profile);

    gtk_table_attach(GTK_TABLE(table),profile->list_pt1, 0,1,0,1,
		     GTK_FILL|GTK_EXPAND, GTK_FILL | GTK_EXPAND,X_PADDING, Y_PADDING);
    
    vseparator = gtk_vseparator_new();
    gtk_table_attach(GTK_TABLE(table), vseparator, 1,2,0,2, 0, GTK_FILL, X_PADDING, Y_PADDING);

    /* pt2 list */
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
    profile->list_pt2 = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
    g_object_unref(store);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes(_("Point 2"), renderer,
						      "text", COLUMN_FIDUCIAL_MARK_NAME, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (profile->list_pt2), column);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (profile->list_pt2));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
    g_object_set_data(G_OBJECT(selection), "pt1", GINT_TO_POINTER(FALSE));
    g_signal_connect(G_OBJECT(selection), "changed",
		     G_CALLBACK(point_selection_changed_cb), profile);

    gtk_table_attach(GTK_TABLE(table),profile->list_pt2, 2,3,0,1,
		     GTK_FILL|GTK_EXPAND, GTK_FILL | GTK_EXPAND,X_PADDING, Y_PADDING);


    /* fill in the lists */
    fiducial_marks = profile->fiducial_marks;
    model1 = gtk_tree_view_get_model(GTK_TREE_VIEW(profile->list_pt1));
    model2 = gtk_tree_view_get_model(GTK_TREE_VIEW(profile->list_pt1));

    while (fiducial_marks != NULL) {
      if (!AMITK_IS_STUDY(AMITK_OBJECT_PARENT(fiducial_marks->data)))
	temp_string = g_strdup_printf("%s::%s",
				      AMITK_OBJECT_NAME(AMITK_OBJECT_PARENT(fiducial_marks->data)),
				      AMITK_OBJECT_NAME(fiducial_marks->data));
      else 
	temp_string = g_strdup_printf("%s", AMITK_OBJECT_NAME(fiducial_marks->data));
      
      gtk_list_store_append (GTK_LIST_STORE(model1), &iter);  /* Acquire an iterator */
      gtk_list_store_set(GTK_LIST_STORE(model1), &iter,
			 COLUMN_FIDUCIAL_MARK_NAME, temp_string,
			 COLUMN_FIDUCIAL_MARK_POINTER, fiducial_marks->data, -1);
      gtk_list_store_append (GTK_LIST_STORE(model2), &iter);  /* Acquire an iterator */
      gtk_list_store_set(GTK_LIST_STORE(model2), &iter,
			 COLUMN_FIDUCIAL_MARK_NAME, temp_string,
			 COLUMN_FIDUCIAL_MARK_POINTER, fiducial_marks->data, -1);
      g_free(temp_string);
      
      fiducial_marks = fiducial_marks->next;
    }


    
    /* ----------------  conclusion page ---------------------------------- */
    profile->pages[OUTPUT_PAGE] = 
      gnome_druid_page_edge_new_with_vals(GNOME_EDGE_FINISH, TRUE,
					  _("Conclusion"), NULL,logo, NULL, NULL);
    g_object_set_data(G_OBJECT(profile->pages[OUTPUT_PAGE]), 
		      "which_page", GINT_TO_POINTER(OUTPUT_PAGE));
    g_signal_connect(G_OBJECT(profile->pages[OUTPUT_PAGE]), "prepare", 
		     G_CALLBACK(prepare_page_cb), profile);
    g_signal_connect(G_OBJECT(profile->pages[OUTPUT_PAGE]), "finish",
		     G_CALLBACK(finish_cb), profile);
    gnome_druid_append_page(GNOME_DRUID(profile->druid), 
			    GNOME_DRUID_PAGE(profile->pages[OUTPUT_PAGE]));
    
  }

  g_object_unref(logo);
  gtk_widget_show_all(profile->dialog);
  return;
}













