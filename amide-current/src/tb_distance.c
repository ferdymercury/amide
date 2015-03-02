/* tb_distance.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2012-2014 Andy Loening
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
#include "amitk_tree_view.h"



gchar * explanation_text = 
  N_("To make a distance measurement, pick two objects (fiducial marks or ROIs). "
     "For ROIs, the distance measurement will be with respect to the center of "
     "the ROI.");


typedef struct tb_distance_t {
  GtkWidget * dialog;

  GtkTextBuffer * result_text_buffer;
  GtkWidget * tree_view1;
  GtkWidget * tree_view2;

  AmitkObject * object1;
  AmitkObject * object2;

  guint reference_count;
} tb_distance_t;


static tb_distance_t * tb_distance_free(tb_distance_t * tb_distance);
static tb_distance_t * tb_distance_init(void);

static void corner_changed_cb(gpointer unused1, AmitkPoint * unused2, gpointer data);
static void space_changed_cb(gpointer unused, gpointer data);
static void update_result_text(tb_distance_t * tb_distance);
static void update_objects(tb_distance_t * tb_distance);
static void tree_view_activate_object(GtkWidget * tree_view, AmitkObject * object, gpointer data);
static void destroy_cb(GtkObject * object, gpointer data);
static void response_cb (GtkDialog * dialog, gint response_id, gpointer data);

static tb_distance_t * tb_distance_free(tb_distance_t * tb_distance) {

  /* sanity checks */
  g_return_val_if_fail(tb_distance != NULL, NULL);
  g_return_val_if_fail(tb_distance->reference_count > 0, NULL);

  /* remove a reference count */
  tb_distance->reference_count--;

  /* things to do if we've removed all references */
  if (tb_distance->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing tb_distance\n");
#endif

    if (tb_distance->object1 != NULL) {
      if (AMITK_IS_ROI(tb_distance->object1))
	g_signal_handlers_disconnect_by_func(G_OBJECT(tb_distance->object1), corner_changed_cb, tb_distance);
      g_signal_handlers_disconnect_by_func(G_OBJECT(tb_distance->object1), space_changed_cb, tb_distance);
      amitk_object_unref(tb_distance->object1);
      tb_distance->object1 = NULL;
    }

    if (tb_distance->object2 != NULL) {
      if (AMITK_IS_ROI(tb_distance->object2))
	  g_signal_handlers_disconnect_by_func(G_OBJECT(tb_distance->object2), corner_changed_cb, tb_distance);
      g_signal_handlers_disconnect_by_func(G_OBJECT(tb_distance->object2), space_changed_cb, tb_distance);
      amitk_object_unref(tb_distance->object2);
      tb_distance->object2 = NULL;
    }

    g_free(tb_distance);
    tb_distance = NULL;
  }

  return tb_distance;
}

static tb_distance_t * tb_distance_init(void) {

  tb_distance_t * tb_distance;

  if ((tb_distance = g_try_new(tb_distance_t,1)) == NULL) {
    g_warning(_("couldn't allocate memory space for tb_distance_t"));
    return NULL;
  }

  tb_distance->reference_count=1;
  tb_distance->object1 = NULL;
  tb_distance->object2 = NULL;

  return tb_distance;
}

static void corner_changed_cb(gpointer unused1, AmitkPoint * unused2, gpointer data) {
  tb_distance_t * tb_distance = data;
  update_result_text(tb_distance);
  return;
}

static void space_changed_cb(gpointer unused, gpointer data) {
  tb_distance_t * tb_distance = data;
  update_result_text(tb_distance);
  return;
}

static void update_result_text(tb_distance_t * tb_distance) {
  AmitkPoint point1;
  AmitkPoint point2;
  amide_real_t distance;
  gchar * temp_string;

  if ((tb_distance->object1 == NULL) || 
      (tb_distance->object2 == NULL) || 
      (tb_distance->object1 == tb_distance->object2)) {
    gtk_text_buffer_set_text (tb_distance->result_text_buffer, _(explanation_text), -1);
  } else {

    /* allright, have two good objects, let's calculate distance */
    if (AMITK_IS_ROI(tb_distance->object1))
      point1 = amitk_volume_get_center(AMITK_VOLUME(tb_distance->object1));
    else /* is fiducial */
      point1 = AMITK_FIDUCIAL_MARK_GET(tb_distance->object1);

    if (AMITK_IS_ROI(tb_distance->object2))
      point2 = amitk_volume_get_center(AMITK_VOLUME(tb_distance->object2));
    else /* is fiducial */
      point2 = AMITK_FIDUCIAL_MARK_GET(tb_distance->object2);

    distance = point_mag(point_sub(point1, point2));

    temp_string = g_strdup_printf("Point1: %s\nPoint2: %s\nDistance %5.3f (mm)", 
				  AMITK_OBJECT_NAME(tb_distance->object1), 
				  AMITK_OBJECT_NAME(tb_distance->object2), 
				  distance);
    gtk_text_buffer_set_text(tb_distance->result_text_buffer, temp_string, -1);
    g_free(temp_string);
  }
}


static void update_objects(tb_distance_t * tb_distance) {
  AmitkObject * object;
  
  /* unref prior objects */
  if (tb_distance->object1 != NULL) {
    if (AMITK_IS_ROI(tb_distance->object1))
      g_signal_handlers_disconnect_by_func(G_OBJECT(tb_distance->object1), corner_changed_cb, tb_distance);
    g_signal_handlers_disconnect_by_func(G_OBJECT(tb_distance->object1), space_changed_cb, tb_distance);
    tb_distance->object1=amitk_object_unref(tb_distance->object1);
  }
  if (tb_distance->object2 != NULL) {
    if (AMITK_IS_ROI(tb_distance->object2))
      g_signal_handlers_disconnect_by_func(G_OBJECT(tb_distance->object2), corner_changed_cb, tb_distance);
    g_signal_handlers_disconnect_by_func(G_OBJECT(tb_distance->object2), space_changed_cb, tb_distance);
    tb_distance->object2=amitk_object_unref(tb_distance->object2);
  }

  /* reference the new objects */
  object = amitk_tree_view_get_active_object(AMITK_TREE_VIEW(tb_distance->tree_view1));
  if (object != NULL)
    if ((AMITK_IS_ROI(object) || AMITK_IS_FIDUCIAL_MARK(object))) {
      tb_distance->object1=amitk_object_ref(object);
      if (AMITK_IS_VOLUME(object))
	g_signal_connect_after(G_OBJECT(object), "volume_corner_changed", G_CALLBACK(corner_changed_cb), tb_distance);
      g_signal_connect_after(G_OBJECT(object), "space_changed", G_CALLBACK(space_changed_cb), tb_distance);
    } 

  object = amitk_tree_view_get_active_object(AMITK_TREE_VIEW(tb_distance->tree_view2));
  if (object != NULL)
    if ((AMITK_IS_ROI(object) || AMITK_IS_FIDUCIAL_MARK(object))) {
      tb_distance->object2=amitk_object_ref(object);
      if (AMITK_IS_VOLUME(object))
	g_signal_connect_after(G_OBJECT(object), "volume_corner_changed", G_CALLBACK(corner_changed_cb), tb_distance);
      g_signal_connect_after(G_OBJECT(object), "space_changed", G_CALLBACK(space_changed_cb), tb_distance);
    } 

  update_result_text(tb_distance);

  return;
}

static void tree_view_activate_object(GtkWidget * tree_view, AmitkObject * object, gpointer data) {

  tb_distance_t * tb_distance = data;

  if (AMITK_IS_ROI(object) || AMITK_IS_FIDUCIAL_MARK(object)) 
    amitk_tree_view_set_active_object(AMITK_TREE_VIEW(tree_view), object);
  else
    amitk_tree_view_set_active_object(AMITK_TREE_VIEW(tree_view), NULL);

  update_objects(tb_distance);
}

static void destroy_cb(GtkObject * object, gpointer data) {
  tb_distance_t * tb_distance = data;
  tb_distance = tb_distance_free(tb_distance);
  return;
}


static void response_cb (GtkDialog * dialog, gint response_id, gpointer data) {

  switch(response_id) {
  case GTK_RESPONSE_CANCEL:
    gtk_widget_destroy(GTK_WIDGET(dialog));
    break;

  default:
    break;
  }

  return;
}

void tb_distance(AmitkStudy * study, GtkWindow * parent_window) {

  GtkWidget * table;
  GtkWidget * label;
  GtkWidget * scrolled;
  GtkWidget * text_view;
  guint table_row=0;
  tb_distance_t * tb_distance;


  tb_distance = tb_distance_init();

  tb_distance->dialog = gtk_dialog_new_with_buttons(_("Distance Measurement Tool"), parent_window,
				       GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
				       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				       NULL);
  

  g_signal_connect(G_OBJECT(tb_distance->dialog), "destroy", G_CALLBACK(destroy_cb), tb_distance);
  g_signal_connect(G_OBJECT(tb_distance->dialog), "response", G_CALLBACK(response_cb), tb_distance);
  gtk_window_set_resizable(GTK_WINDOW(tb_distance->dialog), TRUE);

  /* make the widgets for this dialog box */
  table = gtk_table_new(2,3,FALSE);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(tb_distance->dialog)->vbox), table);


  label = gtk_label_new(_("Starting Point"));
  gtk_table_attach(GTK_TABLE(table), label, 0,1, table_row,table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  label = gtk_label_new(_("Ending Point"));
  gtk_table_attach(GTK_TABLE(table), label, 1,2, table_row,table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  /* make the starting point tree view */
  tb_distance->tree_view1 = amitk_tree_view_new(AMITK_TREE_VIEW_MODE_SINGLE_SELECTION,NULL, NULL);
  amitk_tree_view_set_study(AMITK_TREE_VIEW(tb_distance->tree_view1), study);
  amitk_tree_view_expand_object(AMITK_TREE_VIEW(tb_distance->tree_view1), AMITK_OBJECT(study));
  g_signal_connect(G_OBJECT(tb_distance->tree_view1), "activate_object", G_CALLBACK(tree_view_activate_object), tb_distance);

  /* and a scrolled area for the tree */
  scrolled = gtk_scrolled_window_new(NULL,NULL);  
  gtk_widget_set_size_request(scrolled,250,250);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), tb_distance->tree_view1);
  gtk_table_attach(GTK_TABLE(table), scrolled, 0,1, table_row, table_row+1,GTK_FILL, GTK_FILL | GTK_EXPAND, X_PADDING, Y_PADDING);


  /* and make the ending point tree view */
  tb_distance->tree_view2 = amitk_tree_view_new(AMITK_TREE_VIEW_MODE_SINGLE_SELECTION,NULL, NULL);
  amitk_tree_view_set_study(AMITK_TREE_VIEW(tb_distance->tree_view2), study);
  amitk_tree_view_expand_object(AMITK_TREE_VIEW(tb_distance->tree_view2), AMITK_OBJECT(study));
  g_signal_connect(G_OBJECT(tb_distance->tree_view2), "activate_object", G_CALLBACK(tree_view_activate_object), tb_distance);

  /* and a scrolled area for the tree */
  scrolled = gtk_scrolled_window_new(NULL,NULL);  
  gtk_widget_set_size_request(scrolled,250,250);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), tb_distance->tree_view2);
  gtk_table_attach(GTK_TABLE(table), scrolled, 1,2, table_row, table_row+1,GTK_FILL, GTK_FILL | GTK_EXPAND, X_PADDING, Y_PADDING);

  table_row++;


  /* and finally, the result buffer */
  text_view = gtk_text_view_new();
  tb_distance->result_text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
  update_result_text(tb_distance);

  gtk_table_attach(GTK_TABLE(table), text_view, 0,2, table_row,table_row+1, FALSE,FALSE, X_PADDING, Y_PADDING);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
  gtk_widget_set_size_request(text_view,400,-1);
  table_row++;


  gtk_widget_show_all(GTK_WIDGET(tb_distance->dialog));


  /* needed, as the process of showing selects an object in the tree view */
  amitk_tree_view_set_active_object(AMITK_TREE_VIEW(tb_distance->tree_view1), NULL); /* make sure nothing selected */
  amitk_tree_view_set_active_object(AMITK_TREE_VIEW(tb_distance->tree_view2), NULL); /* make sure nothing selected */
  return;

}











