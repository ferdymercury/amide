/* amitk_list_view.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2003-2004 Andy Loening
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

/* adapted from gtkcolorsel.c */

#include "amide_config.h"
#include "amitk_list_view.h"
#include "amitk_study.h"
#include "amitk_marshal.h"
#include "image.h"


enum {
  SELECT_OBJECT,
  LAST_SIGNAL
} amitk_list_view_signals;

enum {
  COLUMN_ICON,
  COLUMN_NAME,
  COLUMN_OBJECT,
  NUM_COLUMNS
};

typedef struct {
  AmitkObject * object;
  GtkTreeIter iter;
  gboolean found;
} list_view_find_t;


static void list_view_class_init (AmitkListViewClass *klass);
static void list_view_init (AmitkListView *list_view);
static void list_view_destroy (GtkObject *object);

static void list_view_object_update_cb(AmitkObject * object, gpointer list);
static void list_view_object_add_child_cb(AmitkObject * parent, AmitkObject * child, gpointer list_view);
static void list_view_object_remove_child_cb(AmitkObject * parent, AmitkObject * child, gpointer list_view);
static gboolean list_view_find_recurse(GtkTreeModel *model, GtkTreePath *path, 
				       GtkTreeIter *iter, gpointer data);
static gboolean list_view_find_object(AmitkListView * list_view, AmitkObject * object, GtkTreeIter * iter);
static void list_view_add_object(AmitkListView * list_view, AmitkObject * object);
static void list_view_remove_object(AmitkListView * list_view, AmitkObject * object);

static GtkTreeViewClass *parent_class;
static guint list_view_signals[LAST_SIGNAL];


GType amitk_list_view_get_type (void) {

  static GType list_view_type = 0;

  if (!list_view_type)
    {
      static const GTypeInfo list_view_info =
      {
	sizeof (AmitkListViewClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) list_view_class_init,
	(GClassFinalizeFunc) NULL,
	NULL, /* class data */
	sizeof (AmitkListView),
	0, /* # preallocs */
	(GInstanceInitFunc) list_view_init,
	NULL /* value table */
      };

      list_view_type = g_type_register_static(GTK_TYPE_TREE_VIEW, "AmitkListView", &list_view_info, 0);
    }

  return list_view_type;
}

static void list_view_class_init (AmitkListViewClass *klass)
{
  GtkObjectClass *gtkobject_class;
  GtkTreeViewClass *list_view_class;
  GtkWidgetClass *widget_class;

  gtkobject_class = (GtkObjectClass*) klass;
  widget_class    = (GtkWidgetClass*) klass;
  list_view_class = (GtkTreeViewClass*) klass;

  parent_class = g_type_class_peek_parent(klass);

  gtkobject_class->destroy = list_view_destroy;

  list_view_signals[SELECT_OBJECT] =
    g_signal_new ("select_object",
		  G_TYPE_FROM_CLASS(klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkListViewClass, select_object),
		  NULL, NULL,
		  amitk_marshal_NONE__OBJECT,
		  G_TYPE_NONE, 1,
		  AMITK_TYPE_OBJECT);
}

static void list_view_init (AmitkListView * list_view)
{
  list_view->study = NULL;

}

static void list_view_destroy (GtkObject * object) {

  AmitkListView * list_view;

  g_return_if_fail (object != NULL);
  g_return_if_fail (AMITK_IS_LIST_VIEW (object));

  list_view =  AMITK_LIST_VIEW (object);

  if (list_view->study != NULL) {
    list_view_remove_object(list_view, AMITK_OBJECT(list_view->study));
    list_view->study = NULL;
  }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}




static void list_view_object_update_cb(AmitkObject * object, gpointer data) {

  AmitkListView * list_view = data;
  GtkTreeIter iter;
  GtkTreeModel * model;
  GdkPixbuf * pixbuf;

  g_return_if_fail(AMITK_IS_LIST_VIEW(list_view));
  g_return_if_fail(AMITK_IS_OBJECT(object));

  if (list_view_find_object(list_view, object, &iter)) {
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(list_view));
    pixbuf = image_get_object_pixbuf(object);
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
		       COLUMN_ICON, pixbuf,
		       COLUMN_NAME, AMITK_OBJECT_NAME(object), 
		       -1);
    g_object_unref(pixbuf);
  }

  //      g_signal_emit(G_OBJECT(list_view), list_view_signals[SELECT_OBJECT], 0,object);

  return;
}

static void list_view_object_add_child_cb(AmitkObject * parent, AmitkObject * child, gpointer data) {

  AmitkListView * list_view = data;

  g_return_if_fail(AMITK_IS_LIST_VIEW(list_view));
  g_return_if_fail(AMITK_IS_OBJECT(child));

  list_view_add_object(AMITK_LIST_VIEW(list_view), child);

  return;
}

static void list_view_object_remove_child_cb(AmitkObject * parent, AmitkObject * child, gpointer data) {
  AmitkListView * list_view = data;

  g_return_if_fail(AMITK_IS_LIST_VIEW(list_view));
  g_return_if_fail(AMITK_IS_OBJECT(child));

  list_view_remove_object(AMITK_LIST_VIEW(list_view), child);

  return;
}

static gboolean list_view_find_recurse(GtkTreeModel *model, GtkTreePath *path, 
				       GtkTreeIter *iter, gpointer data) {
  list_view_find_t * list_view_find=data;
  AmitkObject * object;

  gtk_tree_model_get(model, iter, COLUMN_OBJECT, &object, -1);

  g_return_val_if_fail(object != NULL, FALSE);

  if (object == list_view_find->object) {
    list_view_find->iter = *iter;
    list_view_find->found = TRUE;
    return TRUE;
  } else {
    return FALSE;
  }

}

static gboolean list_view_find_object(AmitkListView * list_view, AmitkObject * object, GtkTreeIter * iter) {

  GtkTreeModel * model;
  list_view_find_t list_view_find;

  g_return_val_if_fail(AMITK_IS_LIST_VIEW(list_view),FALSE);

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(list_view));
  list_view_find.object = object;
  list_view_find.found = FALSE;

  gtk_tree_model_foreach(model, list_view_find_recurse, &list_view_find);

  if (list_view_find.found) {
    *iter = list_view_find.iter;
    return TRUE;
  } else 
    return FALSE;
}


static void list_view_add_object(AmitkListView * list_view, AmitkObject * object) {

  GList * children;
  GtkTreeIter parent_iter;
  GtkTreeIter iter;
  GtkTreeModel * model;
  GdkPixbuf * pixbuf;

  amitk_object_ref(object); /* add a reference */

  if (AMITK_IS_STUDY(object)) { /* save a pointer to thestudy object */
    if (list_view->study != NULL) {
      list_view_remove_object(list_view, AMITK_OBJECT(list_view->study));
    }
    list_view->study = AMITK_STUDY(object);
  }
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(list_view));

  if (AMITK_OBJECT_PARENT(object) == NULL) 
    gtk_list_store_append (GTK_LIST_STORE(model), &iter);  /* Acquire a top-level iterator */
  else {
    if (list_view_find_object(list_view, AMITK_OBJECT_PARENT(object), &parent_iter))
      gtk_list_store_insert_after (GTK_LIST_STORE(model), &iter, &parent_iter);
    else
      g_return_if_reached();
  }

  pixbuf = image_get_object_pixbuf(object);

  gtk_list_store_set(GTK_LIST_STORE(model), &iter,
		     COLUMN_ICON, pixbuf,
		     COLUMN_NAME, AMITK_OBJECT_NAME(object),
		     COLUMN_OBJECT, object, -1);
  g_object_unref(pixbuf);

  g_signal_connect(G_OBJECT(object), "object_name_changed", G_CALLBACK(list_view_object_update_cb), list_view);
  g_signal_connect(G_OBJECT(object), "object_selection_changed", G_CALLBACK(list_view_object_update_cb), list_view);
  if (AMITK_IS_DATA_SET(object)) {
    g_signal_connect(G_OBJECT(object), "modality_changed", G_CALLBACK(list_view_object_update_cb), list_view);
    g_signal_connect(G_OBJECT(object), "color_table_changed", G_CALLBACK(list_view_object_update_cb), list_view);
  } else if (AMITK_IS_ROI(object)) {
    g_signal_connect(G_OBJECT(object), "roi_type_changed", G_CALLBACK(list_view_object_update_cb), list_view);
  } 
  g_signal_connect(G_OBJECT(object), "object_add_child", G_CALLBACK(list_view_object_add_child_cb), list_view);
  g_signal_connect(G_OBJECT(object), "object_remove_child", G_CALLBACK(list_view_object_remove_child_cb), list_view);

  /* add children */
  children = AMITK_OBJECT_CHILDREN(object);
  while (children != NULL) {
      list_view_add_object(list_view, children->data);
      children = children->next;
  }


}

static void list_view_remove_object(AmitkListView * list_view, AmitkObject * object) {

  GList * children;
  GtkTreeIter iter;
  GtkTreeModel * model;

  g_return_if_fail(AMITK_IS_LIST_VIEW(list_view));
  g_return_if_fail(list_view_find_object(list_view, object, &iter)); /* shouldn't fail */

  /* unselect the object */
  amitk_object_unselect(object, AMITK_SELECTION_ALL);

  /* recursive remove children */
  children = AMITK_OBJECT_CHILDREN(object);
  while (children != NULL) {
    list_view_remove_object(list_view, children->data);
    children = children->next;
  }

  /* disconnect the object's signals */
  g_signal_handlers_disconnect_by_func(G_OBJECT(object), G_CALLBACK(list_view_object_update_cb), list_view);
  g_signal_handlers_disconnect_by_func(G_OBJECT(object), G_CALLBACK(list_view_object_add_child_cb), list_view);
  g_signal_handlers_disconnect_by_func(G_OBJECT(object), G_CALLBACK(list_view_object_remove_child_cb), list_view);

  /* remove the object */
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(list_view));
  gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
  
  /* and unref */
  amitk_object_unref(object);

  return;
}


GtkWidget* amitk_list_view_new (void) {

  AmitkListView * list_view;
  GtkListStore * store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;

  list_view = g_object_new(amitk_list_view_get_type(), NULL);

  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(list_view), FALSE);

  store = gtk_list_store_new(NUM_COLUMNS, 
			     G_TYPE_BOOLEAN, /* COLUMN_VISIBLE_SINGLE */
			     G_TYPE_BOOLEAN,/* COLUMN_VISIBLE_LINKED_2WAY */
			     G_TYPE_BOOLEAN, /* COLUMN_VISIBLE_LINKED_3WAY */
			     G_TYPE_STRING,
			     GDK_TYPE_PIXBUF, 
			     G_TYPE_STRING, 
			     G_TYPE_POINTER);
  gtk_tree_view_set_model (GTK_TREE_VIEW(list_view), GTK_TREE_MODEL (store));
  g_object_unref(store);

  /* icon */
  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes("", renderer, 
						    "pixbuf", COLUMN_ICON, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);

  /* name */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("name", renderer,
						    "text", COLUMN_NAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list_view), column);


  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

  return GTK_WIDGET (list_view);
}


void amitk_list_view_set_study(AmitkListView * list_view, AmitkStudy * study) {

  g_return_if_fail(AMITK_IS_LIST_VIEW(list_view));
  g_return_if_fail(AMITK_IS_STUDY(study));
  list_view_add_object(list_view, AMITK_OBJECT(study));

  return;
}




