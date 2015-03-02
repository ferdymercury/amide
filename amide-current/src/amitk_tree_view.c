/* amitk_tree_view.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2002-2014 Andy Loening
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
#include "amitk_tree_view.h"
#include "amitk_study.h"
#include "amitk_marshal.h"
#include "amitk_progress_dialog.h"
#include "image.h"
#include <gdk/gdkkeysyms.h>
#include <string.h>



#define AMITK_TREE_VIEW_DND_OBJECT_TYPE	 "pointer/object"
#define AMITK_DND_URI_LIST_TYPE          "text/uri-list"

enum {
  AMITK_TREE_VIEW_DND_OBJECT,
  AMITK_DND_URI_LIST,
};

static const GtkTargetEntry drag_types[] = {
  { AMITK_TREE_VIEW_DND_OBJECT_TYPE, 0, AMITK_TREE_VIEW_DND_OBJECT }
};

static const GtkTargetEntry drop_types[] = {
  { AMITK_TREE_VIEW_DND_OBJECT_TYPE, 0, AMITK_TREE_VIEW_DND_OBJECT },
  { AMITK_DND_URI_LIST_TYPE,   0, AMITK_DND_URI_LIST }
};


enum {
  HELP_EVENT,
  ACTIVATE_OBJECT,
  POPUP_OBJECT,
  ADD_OBJECT,
  DELETE_OBJECT,
  LAST_SIGNAL
} amitk_tree_view_signals;


/* notes 
   MULTIPLE_SELECTION - is only used in AMITK_TREE_VIEW_MODE_MULTIPLE_SELECTION
   VISIBLE_SINGLE/VISIBLE_LINKED_2WAY/VISIBLE_LINKED_3WAY - are used in
     AMITK_TREE_VIEW_MODE_MAIN, and directly correlate with the related
     AmitkObject parameters

*/
enum {
  COLUMN_MULTIPLE_SELECTION,
  COLUMN_VISIBLE_SINGLE,
  COLUMN_VISIBLE_LINKED_2WAY,
  COLUMN_VISIBLE_LINKED_3WAY,
  COLUMN_EXPANDER,
  COLUMN_ICON,
  COLUMN_NAME,
  COLUMN_OBJECT,
  NUM_COLUMNS
};

typedef struct {
  AmitkObject * object;
  GtkTreeIter iter;
  gboolean found;
} tree_view_find_t;

static void tree_view_class_init (AmitkTreeViewClass *klass);
static void tree_view_init (AmitkTreeView *tree_view);
static void tree_view_destroy (GtkObject *object);
static void tree_view_set_view_mode(AmitkTreeView * tree_view, AmitkViewMode view_mode);
static void tree_view_add_roi_cb(GtkWidget * widget, gpointer data);
static void tree_view_popup_roi_menu(AmitkTreeView * tree_view, AmitkObject * parent_object, 
				     guint button, guint32 activate_time);
static gboolean tree_view_button_press_event(GtkWidget *tree_view,GdkEventButton *event);
static gboolean tree_view_button_release_event(GtkWidget *tree_view, GdkEventButton *event);
static gboolean tree_view_key_press_event(GtkWidget * tree_view, GdkEventKey * event);
static void tree_view_emit_help_signal(AmitkTreeView * tree_view);
static gboolean tree_view_motion_notify_event(GtkWidget *widget, GdkEventMotion *event);
static gboolean tree_view_enter_notify_event(GtkWidget * tree_view,
					     GdkEventCrossing *event);
static GdkPixbuf * tree_view_get_object_pixbuf(AmitkTreeView * tree_view, AmitkObject * object);

//static void tree_view_drop_cb(GtkWidget * menu, gpointer context);

#if WORKING_ON_IT
/* source drag-n-drop */
static void tree_view_drag_begin       (GtkWidget        *widget,
					GdkDragContext   *context);
static void tree_view_drag_end         (GtkWidget        *widget,
					GdkDragContext   *context);
static void tree_view_drag_data_get    (GtkWidget        *widget,
					GdkDragContext   *context,
					GtkSelectionData *selection_data,
					guint             info,
					guint             time);
static void tree_view_drag_data_delete (GtkWidget        *widget,
					GdkDragContext   *context);


/* target drag-n-drop */
static void     tree_view_drag_leave         (GtkWidget        *widget,
					      GdkDragContext   *context,
					      guint             time);
static gboolean tree_view_drag_motion        (GtkWidget        *widget,
					      GdkDragContext   *context,
					      gint              x,
					      gint              y,
					      guint             time);
static gboolean tree_view_drag_drop          (GtkWidget        *widget,
					      GdkDragContext   *context,
					      gint              x,
					      gint              y,
					      guint             time);
static void     tree_view_drag_data_received (GtkWidget        *widget,
					      GdkDragContext   *context,
					      gint              x,
					      gint              y,
					      GtkSelectionData *selection_data,
					      guint             info,
					      guint             time);
#endif

static void tree_view_object_update_cb(AmitkObject * object, gpointer tree_view);
static void tree_view_data_set_color_table_cb(AmitkDataSet * data_set, AmitkViewMode view_mode, gpointer tree_view);
static void tree_view_object_add_child_cb(AmitkObject * parent, AmitkObject * child, gpointer tree_view);
static void tree_view_object_remove_child_cb(AmitkObject * parent, AmitkObject * child, gpointer tree_view);
static gboolean tree_view_find_recurse(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data);
static gboolean tree_view_find_object(AmitkTreeView * tree_view, AmitkObject * object, GtkTreeIter * iter);
static void tree_view_add_object(AmitkTreeView * tree_view, AmitkObject * object);
static void tree_view_remove_object(AmitkTreeView * tree_view, AmitkObject * object);

static GtkTreeViewClass *parent_class;
static guint tree_view_signals[LAST_SIGNAL];


GType amitk_tree_view_get_type (void) {

  static GType tree_view_type = 0;

  if (!tree_view_type)
    {
      static const GTypeInfo tree_view_info =
      {
	sizeof (AmitkTreeViewClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) tree_view_class_init,
	(GClassFinalizeFunc) NULL,
	NULL, /* class data */
	sizeof (AmitkTreeView),
	0, /* # preallocs */
	(GInstanceInitFunc) tree_view_init,
	NULL /* value table */
      };

      tree_view_type = g_type_register_static(GTK_TYPE_TREE_VIEW, "AmitkTreeView", &tree_view_info, 0);
    }

  return tree_view_type;
}

static void tree_view_class_init (AmitkTreeViewClass *klass)
{
  GtkObjectClass *gtkobject_class;
  /*  GtkTreeViewClass *tree_view_class; */
  GtkWidgetClass *widget_class;

  gtkobject_class = (GtkObjectClass*) klass;
  widget_class    = (GtkWidgetClass*) klass;
  /*  tree_view_class    = (GtkTreeViewClass*) klass; */

  parent_class = g_type_class_peek_parent(klass);

  gtkobject_class->destroy = tree_view_destroy;

  widget_class->button_press_event = tree_view_button_press_event;
  widget_class->button_release_event = tree_view_button_release_event;
  widget_class->key_press_event = tree_view_key_press_event;
  widget_class->motion_notify_event = tree_view_motion_notify_event;
  widget_class->enter_notify_event = tree_view_enter_notify_event;

#if WORKING_ON_IT
  widget_class->drag_begin = tree_view_drag_begin;
  widget_class->drag_end = tree_view_drag_end;
  widget_class->drag_data_get = tree_view_drag_data_get;
  widget_class->drag_data_delete = tree_view_drag_data_delete;
  widget_class->drag_leave = tree_view_drag_leave; 
  widget_class->drag_motion = tree_view_drag_motion;
  widget_class->drag_drop = tree_view_drag_drop;
  widget_class->drag_data_received = tree_view_drag_data_received;
#endif

  tree_view_signals[HELP_EVENT] =
    g_signal_new ("help_event",
		  G_TYPE_FROM_CLASS(klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkTreeViewClass, help_event),
		  NULL, NULL,
		  amitk_marshal_NONE__ENUM, 
		  G_TYPE_NONE, 1,
		  AMITK_TYPE_HELP_INFO);
  tree_view_signals[ACTIVATE_OBJECT] =
    g_signal_new ("activate_object",
		  G_TYPE_FROM_CLASS(klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkTreeViewClass, activate_object),
		  NULL, NULL,
		  amitk_marshal_NONE__OBJECT,
		  G_TYPE_NONE, 1,
		  AMITK_TYPE_OBJECT);
  tree_view_signals[POPUP_OBJECT] =
    g_signal_new ("popup_object",
		  G_TYPE_FROM_CLASS(klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkTreeViewClass, popup_object),
		  NULL, NULL,
		  amitk_marshal_NONE__OBJECT,
		  G_TYPE_NONE, 1,
		  AMITK_TYPE_OBJECT);
  tree_view_signals[ADD_OBJECT] =
    g_signal_new ("add_object",
		  G_TYPE_FROM_CLASS(klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkTreeViewClass, add_object),
		  NULL, NULL,
		  amitk_marshal_NONE__OBJECT_ENUM_ENUM, 
		  G_TYPE_NONE,3,
		  AMITK_TYPE_OBJECT,
		  AMITK_TYPE_OBJECT_TYPE,
		  AMITK_TYPE_ROI_TYPE);
  tree_view_signals[DELETE_OBJECT] =
    g_signal_new ("delete_object",
		  G_TYPE_FROM_CLASS(klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkTreeViewClass, delete_object),
		  NULL, NULL,
		  amitk_marshal_NONE__OBJECT, 
		  G_TYPE_NONE,1,
		  AMITK_TYPE_OBJECT);

}

static void tree_view_init (AmitkTreeView * tree_view)
{
  tree_view->study = NULL;
  tree_view->active_object = NULL;
  tree_view->mouse_x = -1;
  tree_view->mouse_y = -1;
  tree_view->current_path = NULL;
  tree_view->prev_view_mode = AMITK_VIEW_MODE_SINGLE;
  tree_view->press_x = -1;
  tree_view->press_y = -1;
  tree_view->drag_begin_possible = FALSE;
  tree_view->src_object = NULL;
  tree_view->dest_object = NULL;
  //  tree_view->drag_list = gtk_target_list_new(drag_types, G_N_ELEMENTS(drag_types));

  /* setup ability to do drag-n-drop */
  //  gtk_drag_dest_set (GTK_WIDGET (tree_view),
  //		     GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT,
  //    		     drag_types, G_N_ELEMENTS(drag_types),
  //		     GDK_ACTION_ASK);

#if WORKING_ON_IT
  gtk_drag_source_set (GTK_WIDGET(tree_view), GDK_BUTTON1_MASK | GDK_BUTTON3_MASK,
  		       drag_types, G_N_ELEMENTS (drag_types),
  		       GDK_ACTION_COPY);

  gtk_drag_dest_set (GTK_WIDGET(tree_view), GTK_DEST_DEFAULT_ALL,
  		     drop_types, G_N_ELEMENTS (drop_types),
		     GDK_ACTION_COPY);
#endif

  
}

static void tree_view_destroy (GtkObject * object) {

  AmitkTreeView * tree_view;

  g_return_if_fail (object != NULL);
  g_return_if_fail (AMITK_IS_TREE_VIEW (object));

  tree_view =  AMITK_TREE_VIEW (object);

  if (tree_view->current_path != NULL) {
    gtk_tree_path_free(tree_view->current_path);
    tree_view->current_path = NULL;
  }

  if (tree_view->active_object != NULL) {
    tree_view->active_object = amitk_object_unref(tree_view->active_object);
  }

  if (tree_view->study != NULL) {
    tree_view_remove_object(tree_view, AMITK_OBJECT(tree_view->study));
    tree_view->study = NULL;
  }

  if (tree_view->preferences != NULL) {
    g_object_unref(tree_view->preferences);
    tree_view->preferences = NULL;
  }

  if (tree_view->progress_dialog != NULL) {
    g_object_unref(tree_view->progress_dialog);
    tree_view->progress_dialog = NULL;
  }

  if (tree_view->drag_list != NULL) {
    gtk_target_list_unref (tree_view->drag_list);
    tree_view->drag_list = NULL;
  }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}



/* function only for AMITK_TREE_VIEW_MODE_MAIN */
static void tree_view_set_view_mode(AmitkTreeView * tree_view, AmitkViewMode view_mode) {

  AmitkViewMode i_view_mode;

  if (tree_view->mode != AMITK_TREE_VIEW_MODE_MAIN) return;

  /* remove all linked columns, we'll add them back below if needed */
  for (i_view_mode = 0; i_view_mode <= tree_view->prev_view_mode; i_view_mode++) {
    g_object_ref(tree_view->select_column[i_view_mode]); /* view_remove_column removes a reference */
    gtk_tree_view_remove_column (GTK_TREE_VIEW (tree_view), tree_view->select_column[i_view_mode]);
  }
  tree_view->prev_view_mode = view_mode;

  /* insert_column doesn't add a reference */
  for (i_view_mode = 0; i_view_mode <= view_mode; i_view_mode++) 
    gtk_tree_view_insert_column (GTK_TREE_VIEW (tree_view), tree_view->select_column[i_view_mode],
				 COLUMN_VISIBLE_SINGLE+i_view_mode);

  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), view_mode != AMITK_VIEW_MODE_SINGLE);

  return;
}

/* callback function for adding an roi */
static void tree_view_add_roi_cb(GtkWidget * widget, gpointer data) {

  AmitkTreeView * tree_view = data;
  AmitkRoiType roi_type;
  AmitkObject * parent_object;

  /* figure out which menu item called me */
  roi_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"roi_type"));
  parent_object = g_object_get_data(G_OBJECT(widget), "parent");
  g_signal_emit(G_OBJECT(tree_view),  tree_view_signals[ADD_OBJECT], 0, 
		parent_object, AMITK_OBJECT_TYPE_ROI, roi_type);

  return;
}


static void tree_view_popup_roi_menu(AmitkTreeView * tree_view, AmitkObject * parent_object, 
				     guint button, guint32 activate_time) {
  GtkWidget * menu;
  GtkWidget * menuitem;
  AmitkRoiType i_roi_type;

  menu = gtk_menu_new();

  for (i_roi_type=0; i_roi_type<AMITK_ROI_TYPE_NUM; i_roi_type++) {
    menuitem = gtk_menu_item_new_with_label(_(amitk_roi_menu_explanation[i_roi_type]));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    g_object_set_data(G_OBJECT(menuitem), "roi_type", GINT_TO_POINTER(i_roi_type)); 
    g_object_set_data(G_OBJECT(menuitem), "parent", parent_object);
    g_signal_connect(G_OBJECT(menuitem), "activate",  G_CALLBACK(tree_view_add_roi_cb), tree_view);
    gtk_widget_show(menuitem);
  }
  gtk_widget_show(menu);
  
  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, activate_time);
  return;
}

static gboolean tree_view_button_press_event (GtkWidget      *widget,
					      GdkEventButton *event) {
  
  gint cell_x, cell_y;
  GtkTreeViewColumn * column;
  gboolean return_value;
  GtkTreeSelection *selection=NULL;
  gboolean valid_row;
  AmitkTreeView * tree_view;

  g_return_val_if_fail (AMITK_IS_TREE_VIEW (widget), FALSE);
  tree_view = AMITK_TREE_VIEW(widget);
  g_return_val_if_fail (event != NULL, FALSE);

  //  g_print("button press\n");

  valid_row = gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree_view), event->x, event->y, 
					    &(tree_view->current_path), 
					    &column, &cell_x, &cell_y);
  
  switch(AMITK_TREE_VIEW(tree_view)->mode) {
    
  case AMITK_TREE_VIEW_MODE_MAIN:
    
    if (event->button == 1) {
      tree_view->drag_begin_possible = TRUE;
      tree_view->press_x = event->x;
      tree_view->press_y = event->y;
    }
    
    /* making the selection mode none reduces some of the flickering caused by us
       manually trying to force which row is selected */
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);
    break;

  default:
    break;
  }
    
  /* run the tree widget's button press event first */
  return_value = GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event);
    


  switch(AMITK_TREE_VIEW(tree_view)->mode) {
    
  case AMITK_TREE_VIEW_MODE_MAIN:
    /* reset what's the active mark */
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
    amitk_tree_view_set_active_object(tree_view, tree_view->active_object);
    
    if ((event->button == 3) && !valid_row)
      tree_view_popup_roi_menu(tree_view, NULL, event->button, event->time);
    break;

  default:
    break;
  }

  return return_value;
}

static gint path_compare(GtkTreePath * path1, GtkTreePath * path2) {

  if ((path1 == NULL) && (path2 == NULL))
    return 0;
  else if ((path1 == NULL) || (path2 == NULL))
    return 1;
  else
    return gtk_tree_path_compare(path1, path2);
}

static gboolean tree_view_button_release_event (GtkWidget      *widget,
						GdkEventButton *event) {
  GtkTreePath * path=NULL;
  gint cell_x, cell_y;
  GtkTreeViewColumn * column;
  AmitkTreeView * tree_view;
  
  g_return_val_if_fail (AMITK_IS_TREE_VIEW (widget), FALSE);
  tree_view = AMITK_TREE_VIEW(widget);
  g_return_val_if_fail (event != NULL, FALSE);

  //  g_print("release button\n");
  tree_view->drag_begin_possible = FALSE;
  //  gtk_drag_finish (tree_view->drag_context, 
  //		   gboolean        success,
  //		   gboolean        del,
  //		   guint32         time)
  //    gtk_grab_remove (widget);
  
  /* figure out if this click was on an item in the tree */
  if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree_view), event->x, event->y, 
				    &path, &column, &cell_x, &cell_y)) {
    
    /* make sure this isn't the expand button, and we're trying to expand,
       also check that we're doing a button release on the object we started
       the button press with */
    if (column != gtk_tree_view_get_expander_column(GTK_TREE_VIEW(tree_view)) &&
	path_compare(path, tree_view->current_path)==0) {
      
      GtkTreeModel * model;
      GtkTreeIter iter;
      gboolean select = FALSE;
      gboolean unselect = FALSE;
      gboolean make_active = FALSE;
      gboolean popup = FALSE;
      gboolean add_object = FALSE;
      gboolean multiple_selection;
      gboolean visible[AMITK_VIEW_MODE_NUM];
      gboolean visible_at_all;
      AmitkObjectType add_type = -1;
      AmitkObject * object=NULL;
      AmitkViewMode view_mode;
      AmitkViewMode i_view_mode;
      
      model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));
      gtk_tree_model_get_iter(model, &iter, path);
      gtk_tree_model_get(model, &iter, COLUMN_OBJECT, &object, 
			 COLUMN_MULTIPLE_SELECTION, &multiple_selection,
			 COLUMN_VISIBLE_SINGLE, &(visible[AMITK_VIEW_MODE_SINGLE]),
			 COLUMN_VISIBLE_LINKED_2WAY, &(visible[AMITK_VIEW_MODE_LINKED_2WAY]),
			 COLUMN_VISIBLE_LINKED_3WAY, &(visible[AMITK_VIEW_MODE_LINKED_3WAY]),
			 -1);
      
      view_mode = AMITK_VIEW_MODE_SINGLE;/* default */
      visible_at_all = visible[AMITK_VIEW_MODE_SINGLE]; 
      
      for (i_view_mode = AMITK_VIEW_MODE_NUM-1; i_view_mode > 0; i_view_mode--) {
	visible_at_all = visible_at_all || visible[i_view_mode];
	if (column == tree_view->select_column[i_view_mode]) {
	  view_mode = i_view_mode;
	}
      }

      switch(tree_view->mode) {
      case AMITK_TREE_VIEW_MODE_MAIN:

	switch (event->button) {
	
	case 1: /* left button */
	  if (!AMITK_IS_STUDY(object)) {
	    if (visible[view_mode])
	      unselect = TRUE;
	    else
	      select = TRUE;
	  }
	  break;
	  
	case 2: /* middle button */
	  make_active = TRUE;
	  if ((!visible_at_all) && (!AMITK_IS_STUDY(object)))
	    select = TRUE;
	  break;
	  
	case 3: /* right button */
	  if (event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) {
	    if (object != NULL) {
	      if (AMITK_IS_DATA_SET(object)) {
		add_object=TRUE;
		if (event->state & GDK_SHIFT_MASK)
		  add_type=AMITK_OBJECT_TYPE_ROI;
		else
		  add_type=AMITK_OBJECT_TYPE_FIDUCIAL_MARK;
	      } else if (AMITK_IS_STUDY(object)) {
		add_object=TRUE;
		add_type=AMITK_OBJECT_TYPE_ROI;
	      }
	    }
	  } else {
	    popup = TRUE;
	  }
	  break;
	  
	default:
	  g_error("unexpected case in %s at line %d",__FILE__, __LINE__);
	  break;
	}
      
	if (select)
	  amitk_object_select(object, view_mode);
	else if (unselect) 
	  amitk_object_unselect(object, view_mode);
	
	if (make_active)
	  g_signal_emit(G_OBJECT(tree_view), tree_view_signals[ACTIVATE_OBJECT], 0,object);
      
	if (popup)
	  g_signal_emit(G_OBJECT(tree_view), tree_view_signals[POPUP_OBJECT], 0, object);
      
	if ((add_object) && (add_type==AMITK_OBJECT_TYPE_FIDUCIAL_MARK))
	  g_signal_emit(G_OBJECT(tree_view),  tree_view_signals[ADD_OBJECT], 0, object, add_type, 0);
      
	if ((add_object) && (add_type==AMITK_OBJECT_TYPE_ROI))
	  tree_view_popup_roi_menu(tree_view, object, event->button, event->time);
	break;

      case AMITK_TREE_VIEW_MODE_SINGLE_SELECTION:
	switch (event->button) {
	case 1: /* left button */
	  g_signal_emit(G_OBJECT(tree_view), tree_view_signals[ACTIVATE_OBJECT],0,object);
	  break;
	}
	break;

      case AMITK_TREE_VIEW_MODE_MULTIPLE_SELECTION:
	switch (event->button) {

	case 1: /* left button */
	  if (multiple_selection)
	    gtk_tree_store_set(GTK_TREE_STORE(model), &iter, COLUMN_MULTIPLE_SELECTION, FALSE, -1);
	  else if (!AMITK_IS_STUDY(object))
	    gtk_tree_store_set(GTK_TREE_STORE(model), &iter, COLUMN_MULTIPLE_SELECTION, TRUE, -1);
	  break;
	
	default:
	  break;
	}
	break;

      default:
	break;
      }
    }


    gtk_tree_path_free(path);
  }

  if (tree_view->current_path != NULL) {
    gtk_tree_path_free(tree_view->current_path);
    tree_view->current_path = NULL;
  }

  return GTK_WIDGET_CLASS (parent_class)->button_release_event (widget, event);
  
}

static gboolean tree_view_key_press_event(GtkWidget * widget,
					  GdkEventKey * event) {

  GtkTreePath * path;
  gint cell_x, cell_y;
  GtkTreeModel * model;
  GtkTreeIter iter;
  AmitkObject * object;
  AmitkTreeView * tree_view;
      

  g_return_val_if_fail(AMITK_IS_TREE_VIEW(widget), FALSE);
  tree_view = AMITK_TREE_VIEW(widget);


  switch (tree_view->mode) {

  case AMITK_TREE_VIEW_MODE_MAIN:
    if (event->state & GDK_CONTROL_MASK) 
      if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree_view), 
					tree_view->mouse_x, 
					tree_view->mouse_y, 
					&path, NULL, &cell_x, &cell_y)) {
	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_path_free(path);
	gtk_tree_model_get(model, &iter, COLUMN_OBJECT, &object, -1);
	
	if ((event->keyval == GDK_x) || (event->keyval == GDK_X))
	  if (!AMITK_IS_STUDY(object))
	    g_signal_emit(G_OBJECT(tree_view), tree_view_signals[DELETE_OBJECT], 0, object);
      }
    break;
  default:
    break;
  }

  return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);
}


static void tree_view_emit_help_signal(AmitkTreeView * tree_view) {

  GtkTreePath * path=NULL;
  gint cell_x, cell_y;
  AmitkHelpInfo help_type;
  GtkTreeModel * model;
  GtkTreeIter iter;
  AmitkObject * object;

  if (tree_view->mode != AMITK_TREE_VIEW_MODE_MAIN) return;

  if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree_view), tree_view->mouse_x, tree_view->mouse_y,
				    &path, NULL, &cell_x, &cell_y)) {

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_path_free(path);
    gtk_tree_model_get(model, &iter, COLUMN_OBJECT, &object, -1);

    if (AMITK_IS_STUDY(object))
      help_type = AMITK_HELP_INFO_TREE_VIEW_STUDY;
    else if (AMITK_IS_DATA_SET(object))
      help_type = AMITK_HELP_INFO_TREE_VIEW_DATA_SET;
    else if (AMITK_IS_ROI(object))
      help_type = AMITK_HELP_INFO_TREE_VIEW_ROI;
    else if (AMITK_IS_FIDUCIAL_MARK(object))
      help_type = AMITK_HELP_INFO_TREE_VIEW_FIDUCIAL_MARK;
    else {
      g_error("unknown object type in %s at %d\n", __FILE__, __LINE__);
      help_type = AMITK_HELP_INFO_TREE_VIEW_NONE;
    }
  } else {
    help_type = AMITK_HELP_INFO_TREE_VIEW_NONE;
  }

  g_signal_emit(G_OBJECT(tree_view), tree_view_signals[HELP_EVENT], 0,help_type);

  return;
}



/* we have to track motion notify for several reasons:
   1. know which entry we're on for button_press events
   2. context sensitive help
   3. are we initiating a drag?
*/
static gboolean tree_view_motion_notify_event(GtkWidget *widget, GdkEventMotion *event) {

  AmitkTreeView * tree_view;

  g_return_val_if_fail(AMITK_IS_TREE_VIEW(widget), FALSE);
  tree_view = AMITK_TREE_VIEW(widget);

  tree_view->mouse_x = event->x;
  tree_view->mouse_y = event->y;

  switch(tree_view->mode) {
  case AMITK_TREE_VIEW_MODE_MAIN:
    tree_view_emit_help_signal(tree_view);
    break;
  default:
    break;
  }

  /* pass the signal on */
  return GTK_WIDGET_CLASS (parent_class)->motion_notify_event (widget, event);
}



static gboolean tree_view_enter_notify_event(GtkWidget * widget,
					     GdkEventCrossing *event) {

  GtkTreeSelection *selection;
  GtkTreeIter iter;
  AmitkTreeView * tree_view;

  g_return_val_if_fail(AMITK_IS_TREE_VIEW(widget), FALSE);
  tree_view = AMITK_TREE_VIEW(widget);

  switch(tree_view->mode) {
  case AMITK_TREE_VIEW_MODE_MAIN:
    tree_view_emit_help_signal(tree_view);

    gtk_widget_grab_focus(widget); /* move the keyboard entry focus into the tree */

    /* double check that the right row is selected - gtk sometimes moves the selection to the 1st row */
    if (tree_view->active_object != NULL) {
      if (tree_view_find_object(tree_view, tree_view->active_object, &iter)) {
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	if (!gtk_tree_selection_iter_is_selected(selection, &iter))
	  gtk_tree_selection_select_iter(selection, &iter);
      }
    }
    break;
  default:
    break;
  }


  /* pass the signal on */
  return (* GTK_WIDGET_CLASS (parent_class)->enter_notify_event) (widget, event);
}


/* note, the contents of GdkPixbuf are shared, and should not be modified */
static GdkPixbuf * tree_view_get_object_pixbuf(AmitkTreeView * tree_view, AmitkObject * object) {

  GdkPixbuf * pixbuf=NULL;

  if (AMITK_IS_ROI(object)) {
    switch (AMITK_ROI_TYPE(object)) {
    case AMITK_ROI_TYPE_ELLIPSOID:
      pixbuf = gtk_widget_render_icon(GTK_WIDGET(tree_view), "amide_icon_roi_ellipsoid", GTK_ICON_SIZE_LARGE_TOOLBAR, 0);
      break;
    case AMITK_ROI_TYPE_CYLINDER:
      pixbuf = gtk_widget_render_icon(GTK_WIDGET(tree_view), "amide_icon_roi_cylinder", GTK_ICON_SIZE_LARGE_TOOLBAR, 0);
      break;
    case AMITK_ROI_TYPE_BOX:
      pixbuf = gtk_widget_render_icon(GTK_WIDGET(tree_view), "amide_icon_roi_box", GTK_ICON_SIZE_LARGE_TOOLBAR, 0);
      break;
    case AMITK_ROI_TYPE_ISOCONTOUR_2D:
      pixbuf = gtk_widget_render_icon(GTK_WIDGET(tree_view), "amide_icon_roi_isocontour_2d", GTK_ICON_SIZE_LARGE_TOOLBAR, 0);
      break;
    case AMITK_ROI_TYPE_ISOCONTOUR_3D:
      pixbuf = gtk_widget_render_icon(GTK_WIDGET(tree_view), "amide_icon_roi_isocontour_3d", GTK_ICON_SIZE_LARGE_TOOLBAR, 0);
      break;
    case AMITK_ROI_TYPE_FREEHAND_2D:
      pixbuf = gtk_widget_render_icon(GTK_WIDGET(tree_view), "amide_icon_roi_freehand_2d", GTK_ICON_SIZE_LARGE_TOOLBAR, 0);
      break;
    case AMITK_ROI_TYPE_FREEHAND_3D:
      pixbuf = gtk_widget_render_icon(GTK_WIDGET(tree_view), "amide_icon_roi_freehand_3d", GTK_ICON_SIZE_LARGE_TOOLBAR, 0);
      break;
    default:
      g_error("Unknown case in %s at %d\n", __FILE__, __LINE__);
      break;
    }

  } else if (AMITK_IS_DATA_SET(object)) {
    pixbuf = image_get_data_set_pixbuf(AMITK_DATA_SET(object));
  } else if (AMITK_IS_STUDY(object)) {
    pixbuf = gtk_widget_render_icon(GTK_WIDGET(tree_view), "amide_icon_study", GTK_ICON_SIZE_LARGE_TOOLBAR, 0);
  } else if (AMITK_IS_FIDUCIAL_MARK(object)) {
    pixbuf = gtk_widget_render_icon(GTK_WIDGET(tree_view), "amide_icon_align_pt", GTK_ICON_SIZE_LARGE_TOOLBAR, 0);
  } else {
    g_error("Unknown case in %s at %d\n", __FILE__, __LINE__);
  }

  return pixbuf;
}


//static void tree_view_drop_cb(GtkWidget * menu, gpointer data) {

//  GdkDragContext * context =data;
//  AmitkTreeView * tree_view;
//  gboolean move;

//  move = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menu), "move"));
//  tree_view = g_object_get_data(G_OBJECT(menu), "tree_view");

//  return;
//}

#if WORKING_ON_IT
static void tree_view_drag_begin (GtkWidget *widget, GdkDragContext *context)
{

  GtkTreePath * path=NULL;
  GtkTreeIter iter;
  //  GdkPixbuf * pixbuf;
  AmitkObject * object;
  gint cell_x, cell_y;
  GtkTreeModel * model;
  AmitkTreeView * tree_view;

  g_return_if_fail(AMITK_IS_TREE_VIEW(widget));
  tree_view = AMITK_TREE_VIEW(widget);
  if (tree_view->mode != AMITK_TREE_VIEW_MODE_MAIN) return;

  g_print("drag begin from %s\n", AMITK_OBJECT_NAME(tree_view->study));

  /* figure out what we're suppose to be dragging */
  if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree_view), 
				    tree_view->mouse_x, tree_view->mouse_y,
				    &path, NULL, &cell_x, &cell_y)) {

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_path_free(path);
    gtk_tree_model_get(model, &iter, COLUMN_OBJECT, &object, -1);
    g_return_if_fail(AMITK_IS_OBJECT(object));


    /* if object is appropriate, start dragging */
    if (!AMITK_IS_STUDY(object)) {
      tree_view->src_object = object;
      g_print("drag begin - figured out source\n");
      //	  context = gtk_drag_begin(widget, tree_view->drag_list, 
      //				   GDK_ACTION_ASK, 1, (GdkEvent*)event);
	  
      //      pixbuf = tree_view_get_object_pixbuf(tree_view, object);
      //      gtk_drag_set_icon_pixbuf(context, pixbuf, -10,-10);
      //      g_object_unref(pixbuf);
    }
  }

}

static void tree_view_drag_end         (GtkWidget        *widget,
					GdkDragContext   *context) {

  AmitkTreeView * tree_view;

  g_return_if_fail(AMITK_IS_TREE_VIEW(widget));
  tree_view = AMITK_TREE_VIEW(widget);
  if (tree_view->mode != AMITK_TREE_VIEW_MODE_MAIN) return;

  g_print("drag end %s\n", AMITK_OBJECT_NAME(tree_view->study));
  tree_view->src_object = NULL;
}

static void tree_view_drag_data_get    (GtkWidget        *widget,
					GdkDragContext   *context,
					GtkSelectionData *selection_data,
					guint             info,
					guint             time) {

  //  char *entry_text;
  AmitkTreeView * tree_view;

  g_return_if_fail(AMITK_IS_TREE_VIEW(widget));
  g_return_if_fail(selection_data != NULL);
  tree_view = AMITK_TREE_VIEW(widget);
  if (tree_view->mode != AMITK_TREE_VIEW_MODE_MAIN) return;

  g_print("drag data get %s    info %d\n", AMITK_OBJECT_NAME(tree_view->study), info);


  switch (info) {
  case AMITK_TREE_VIEW_DND_OBJECT:
    //    gtk_selection_data_set_text(selection_data, AMITK_OBJECT_NAME(tree_view->src_object), -1);
    gtk_selection_data_set (selection_data,
			    selection_data->target,
			    8, (guchar *) AMITK_OBJECT_NAME(tree_view->src_object),
			    strlen (AMITK_OBJECT_NAME(tree_view->src_object))+1);
    break;
  case AMITK_DND_URI_LIST:
  default:
    g_assert_not_reached ();
  }

}

static void tree_view_drag_data_delete (GtkWidget        *widget,
					GdkDragContext   *context) {
  AmitkTreeView * tree_view;

  g_return_if_fail(AMITK_IS_TREE_VIEW(widget));
  tree_view = AMITK_TREE_VIEW(widget);
  if (tree_view->mode != AMITK_TREE_VIEW_MODE_MAIN) return;

  g_print("drag data delete on %s\n", AMITK_OBJECT_NAME(tree_view->study));
}


static void     tree_view_drag_leave         (GtkWidget        *widget,
					      GdkDragContext   *context,
					      guint             time) {

  AmitkTreeView * tree_view;

  g_return_if_fail(AMITK_IS_TREE_VIEW(widget));
  tree_view = AMITK_TREE_VIEW(widget);
  if (tree_view->mode != AMITK_TREE_VIEW_MODE_MAIN) return;

  g_print("target leave on %s\n", AMITK_OBJECT_NAME(tree_view->study));

}

static gboolean tree_view_drag_motion        (GtkWidget        *widget,
					      GdkDragContext   *context,
					      gint              x,
					      gint              y,
					      guint             time) {

  AmitkTreeView * tree_view;

  g_return_val_if_fail(AMITK_IS_TREE_VIEW(widget), FALSE);
  tree_view = AMITK_TREE_VIEW(widget);
  if (tree_view->mode != AMITK_TREE_VIEW_MODE_MAIN) return;

  //  g_print("target motion %d %d on %s\n", x, y, AMITK_OBJECT_NAME(tree_view->study));
  //  gdk_drag_status (context, 0, time);
  // gtk_drag_finish (context, FALSE, FALSE, time);

  //  valid_row = gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree_view), event->x, event->y, 
  //					    &(AMITK_TREE_VIEW(tree_view)->current_path), 
  //					    &column, &cell_x, &cell_y);


  return TRUE;
}

static gboolean tree_view_drag_drop          (GtkWidget        *widget,
					      GdkDragContext   *context,
					      gint              x,
					      gint              y,
					      guint             time) {


  //  GtkWidget * menu;
  //  GtkWidget * menuitem;
  //  GdkAtom target = GDK_NONE;
  AmitkTreeView * tree_view;

  g_return_val_if_fail(AMITK_IS_TREE_VIEW(widget), FALSE);
  tree_view = AMITK_TREE_VIEW(widget);
  if (tree_view->mode != AMITK_TREE_VIEW_MODE_MAIN) return;

  g_print("drop on %d %d %s\n", x, y,  AMITK_OBJECT_NAME(tree_view->study));
  /* check if drop is valid */

  /* record which object we're moving/copying, and to where */
  
  /* see if we want to copy or move */
  //  menu = gtk_menu_new();

  //  menuitem = gtk_menu_item_new_with_label("Move");
  //  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  //  g_object_set_data(G_OBJECT(menuitem), "move", GINT_TO_POINTER(TRUE));
  //  g_object_set(data(G_OBJECT(menuitem), "tree_view", tree_view);
  //  g_signal_connect(G_OBJECT(menuitem), "activate",  G_CALLBACK(tree_view_drop_cb), context);
  //  gtk_widget_show(menuitem);

  //  menuitem = gtk_menu_item_new_with_label("Copy");
  //  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  //  g_object_set_data(G_OBJECT(menuitem), "move", GINT_TO_POINTER(FALSE));
  //  g_object_set(data(G_OBJECT(menuitem), "tree_view", tree_view);
  //  g_signal_connect(G_OBJECT(menuitem), "activate",  G_CALLBACK(tree_view_drop_cb), context);
  //  gtk_widget_show(menuitem);

  //  gtk_widget_show(menu);
  
  //  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 1, time);

  
  //  target = gtk_drag_dest_find_target (widget, context, NULL);

  //  if (target != GDK_NONE) {
  //    g_print("target\n");
  //    gtk_drag_get_data (widget, context, target, time);
  //  } else {
  //    gtk_drag_finish (context, FALSE, FALSE, time);
  //  }
  

  /* Drag and drop didn't happen! */
  //  gtk_drag_finish (context, FALSE, FALSE, time);


  return TRUE;
}


static void tree_view_drag_data_received (GtkWidget        *widget,
					  GdkDragContext   *context,
					  gint              x,
					  gint              y,
					  GtkSelectionData *selection_data,
					  guint             info,
					  guint             time) {

  gchar *str;
  gchar **uris;
  gint uris_index;
  AmitkTreeView * tree_view;
  AmitkDataSet * dropped_ds;
  gchar * import_filename;

  g_return_if_fail(AMITK_IS_TREE_VIEW(widget));
  tree_view = AMITK_TREE_VIEW(widget);
  if (tree_view->mode != AMITK_TREE_VIEW_MODE_MAIN) return;

  g_print("target receive %s\n", AMITK_OBJECT_NAME(tree_view->study));

  if (selection_data->length >= 0)
    g_print("selection data format %d\n", selection_data->format);

  g_print("context->action %d\n", context->action);

  switch (info) {
  case AMITK_TREE_VIEW_DND_OBJECT:
    str = selection_data->data;
    //    str = gtk_selection_data_get_text (selection_data);
    g_print("target receive x %d y %d    info %d suggested action %d\n", x,y,info, context->suggested_action);
    g_print("target receive x %d y %d    info %d suggested action %d selection %s\n", x,y,info, context->suggested_action,str);
    g_free(str);
    break;


  case AMITK_DND_URI_LIST:
    uris = g_strsplit (selection_data->data, "\r\n", 0);
    for (uris_index=0; (uris[uris_index] != NULL && uris[uris_index][0] != '\0'); uris_index++) {

      if ((import_filename = g_filename_from_uri(uris[uris_index], NULL, NULL)) == NULL)
	g_warning("Could not generate filename for URI: %s", uris[uris_index]);
      else {
	dropped_ds = amitk_data_set_import_file(AMITK_IMPORT_METHOD_GUESS, 0, import_filename, NULL,
						tree_view->preferences, amitk_progress_dialog_update, tree_view->progress_dialog);
	g_free(import_filename);
	if (dropped_ds != NULL) {
	  amitk_object_add_child(AMITK_OBJECT(tree_view->study), 
				 AMITK_OBJECT(dropped_ds)); /* this adds a reference to the data set*/
	  amitk_object_unref(dropped_ds); /* so remove a reference */
	}
      }
    }
    g_strfreev (uris);
    break;


  default:
    g_assert_not_reached ();
  }

  gtk_drag_finish (context, FALSE, FALSE, time);
}

/*   if ((data->length >= 0) && (data->format == 8)) */
/*     { */
/*       if (context->action == GDK_ACTION_ASK)  */
/*         { */
/*           GtkWidget *dialog; */
/*           gint response; */
          
/*           dialog = gtk_message_dialog_new (NULL, */
/*                                            GTK_DIALOG_MODAL |  */
/*                                            GTK_DIALOG_DESTROY_WITH_PARENT, */
/*                                            GTK_MESSAGE_INFO, */
/*                                            GTK_BUTTONS_YES_NO, */
/*                                            "Move the data ?\n"); */
/*           response = gtk_dialog_run (GTK_DIALOG (dialog)); */
/*           gtk_widget_destroy (dialog); */
            
/*           if (response == GTK_RESPONSE_YES) */
/*             context->action = GDK_ACTION_MOVE; */
/*           else */
/*             context->action = GDK_ACTION_COPY; */
/*          } */
         
/*       gtk_drag_finish (context, TRUE, FALSE, time); */
/*       return; */
/*     } */
      
/*  } */

#endif


static void tree_view_study_view_mode_cb(AmitkStudy * study, gpointer data) {

  AmitkTreeView * tree_view = data;

  g_return_if_fail(AMITK_IS_TREE_VIEW(tree_view));
  g_return_if_fail(AMITK_IS_STUDY(study));
  if (tree_view->mode != AMITK_TREE_VIEW_MODE_MAIN) return;

  tree_view_set_view_mode(tree_view, AMITK_STUDY_VIEW_MODE(study));

  return;
}

static void tree_view_object_update_cb(AmitkObject * object, gpointer data) {

  AmitkTreeView * tree_view = data;
  GtkTreeIter iter;
  GtkTreeModel * model;
  GdkPixbuf * pixbuf;

  g_return_if_fail(AMITK_IS_TREE_VIEW(tree_view));
  g_return_if_fail(AMITK_IS_OBJECT(object));

  if (tree_view_find_object(tree_view, object, &iter)) {
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));
    pixbuf = tree_view_get_object_pixbuf(tree_view, object);
    gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
		       COLUMN_ICON, pixbuf,
		       COLUMN_NAME, AMITK_OBJECT_NAME(object), 
		       COLUMN_VISIBLE_SINGLE, amitk_object_get_selected(object, AMITK_SELECTION_SELECTED_0),
		       COLUMN_VISIBLE_LINKED_2WAY, amitk_object_get_selected(object, AMITK_SELECTION_SELECTED_1),
		       COLUMN_VISIBLE_LINKED_3WAY, amitk_object_get_selected(object, AMITK_SELECTION_SELECTED_2),
		       -1);
    g_object_unref(pixbuf);
  }

  if (tree_view->mode == AMITK_TREE_VIEW_MODE_MAIN) {

    if (tree_view->active_object == object) {
      if (!amitk_object_get_selected(object, AMITK_SELECTION_ANY)) /* we're unselecting the active object */
	g_signal_emit(G_OBJECT(tree_view), tree_view_signals[ACTIVATE_OBJECT], 0,NULL);
    } else if (tree_view->active_object == NULL) { /* currently no active object */
      if (amitk_object_get_selected(object, AMITK_SELECTION_ANY)) /* we've selected something */
	g_signal_emit(G_OBJECT(tree_view), tree_view_signals[ACTIVATE_OBJECT], 0,object);
    }
  }

  return;
}

static void tree_view_data_set_color_table_cb(AmitkDataSet * data_set, AmitkViewMode view_mode, gpointer data) {

  g_return_if_fail(AMITK_IS_DATA_SET(data_set));
  if (view_mode == AMITK_VIEW_MODE_SINGLE)
    tree_view_object_update_cb(AMITK_OBJECT(data_set), data);

}


static void tree_view_object_add_child_cb(AmitkObject * parent, AmitkObject * child, gpointer data) {

  AmitkTreeView * tree_view = data;

  g_return_if_fail(AMITK_IS_TREE_VIEW(tree_view));
  g_return_if_fail(AMITK_IS_OBJECT(child));

  tree_view_add_object(AMITK_TREE_VIEW(tree_view), child);
  

  return;
}

static void tree_view_object_remove_child_cb(AmitkObject * parent, AmitkObject * child, gpointer data) {
  AmitkTreeView * tree_view = data;

  g_return_if_fail(AMITK_IS_TREE_VIEW(tree_view));
  g_return_if_fail(AMITK_IS_OBJECT(child));

  tree_view_remove_object(AMITK_TREE_VIEW(tree_view), child);

  return;
}

static gboolean tree_view_find_recurse(GtkTreeModel *model, GtkTreePath *path, 
				  GtkTreeIter *iter, gpointer data) {
  tree_view_find_t * tree_view_find=data;
  AmitkObject * object;

  gtk_tree_model_get(model, iter, COLUMN_OBJECT, &object, -1);

  g_return_val_if_fail(object != NULL, FALSE);

  if (object == tree_view_find->object) {
    tree_view_find->iter = *iter;
    tree_view_find->found = TRUE;
    return TRUE;
  } else {
    return FALSE;
  }

}

static gboolean tree_view_find_object(AmitkTreeView * tree_view, AmitkObject * object, GtkTreeIter * iter) {

  GtkTreeModel * model;
  tree_view_find_t tree_view_find;

  g_return_val_if_fail(AMITK_IS_TREE_VIEW(tree_view),FALSE);

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));
  tree_view_find.object = object;
  tree_view_find.found = FALSE;

  gtk_tree_model_foreach(model, tree_view_find_recurse, &tree_view_find);

  if (tree_view_find.found) {
    *iter = tree_view_find.iter;
    return TRUE;
  } else 
    return FALSE;
}


static void tree_view_add_object(AmitkTreeView * tree_view, AmitkObject * object) {

  GList * children;
  GtkTreeIter parent_iter;
  GtkTreeIter iter;
  GtkTreeModel * model;
  GdkPixbuf * pixbuf;

  amitk_object_ref(object); /* add a reference */

  if (AMITK_IS_STUDY(object)) { /* save a pointer to thestudy object */
    if (tree_view->study != NULL) {
      tree_view_remove_object(tree_view, AMITK_OBJECT(tree_view->study));
    }
    tree_view->study = AMITK_STUDY(object);
    tree_view_set_view_mode(tree_view, AMITK_STUDY_VIEW_MODE(object));
  }
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));

  if (AMITK_OBJECT_PARENT(object) == NULL) 
    gtk_tree_store_append (GTK_TREE_STORE(model), &iter, NULL);  /* Acquire a top-level iterator */
  else {
    if (tree_view_find_object(tree_view, AMITK_OBJECT_PARENT(object), &parent_iter))
      gtk_tree_store_append (GTK_TREE_STORE(model), &iter, &parent_iter);
    else
      g_return_if_reached();
  }

  pixbuf = tree_view_get_object_pixbuf(tree_view, object);

  gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
		     COLUMN_MULTIPLE_SELECTION, amitk_object_get_selected(object, AMITK_SELECTION_SELECTED_0), /* default */
		     COLUMN_VISIBLE_SINGLE, amitk_object_get_selected(object, AMITK_SELECTION_SELECTED_0),
		     COLUMN_VISIBLE_LINKED_2WAY, amitk_object_get_selected(object, AMITK_SELECTION_SELECTED_1),
		     COLUMN_VISIBLE_LINKED_3WAY, amitk_object_get_selected(object, AMITK_SELECTION_SELECTED_2),
		     COLUMN_EXPANDER, "*",
		     COLUMN_ICON, pixbuf,
		     COLUMN_NAME, AMITK_OBJECT_NAME(object),
		     COLUMN_OBJECT, object, -1);
  g_object_unref(pixbuf);

  g_signal_connect(G_OBJECT(object), "object_name_changed", G_CALLBACK(tree_view_object_update_cb), tree_view);
  g_signal_connect(G_OBJECT(object), "object_selection_changed", G_CALLBACK(tree_view_object_update_cb), tree_view);
  if (AMITK_IS_DATA_SET(object)) {
    g_signal_connect(G_OBJECT(object), "modality_changed", G_CALLBACK(tree_view_object_update_cb), tree_view);
    g_signal_connect(G_OBJECT(object), "color_table_changed", G_CALLBACK(tree_view_data_set_color_table_cb), tree_view);
  } else if (AMITK_IS_ROI(object)) {
    g_signal_connect(G_OBJECT(object), "roi_type_changed", G_CALLBACK(tree_view_object_update_cb), tree_view);
  } else if (AMITK_IS_STUDY(object)) {
    g_signal_connect(G_OBJECT(object), "view_mode_changed", G_CALLBACK(tree_view_study_view_mode_cb), tree_view);
  }
  g_signal_connect(G_OBJECT(object), "object_add_child", G_CALLBACK(tree_view_object_add_child_cb), tree_view);
  g_signal_connect(G_OBJECT(object), "object_remove_child", G_CALLBACK(tree_view_object_remove_child_cb), tree_view);

  /* add children */
  children = AMITK_OBJECT_CHILDREN(object);
  while (children != NULL) {
      tree_view_add_object(tree_view, children->data);
      children = children->next;
  }


}

static void tree_view_remove_object(AmitkTreeView * tree_view, AmitkObject * object) {

  GList * children;
  GtkTreeIter iter;
  GtkTreeModel * model;

  g_return_if_fail(AMITK_IS_TREE_VIEW(tree_view));
  g_return_if_fail(tree_view_find_object(tree_view, object, &iter)); /* shouldn't fail */

  /* unselect the object */
  if (tree_view->mode == AMITK_TREE_VIEW_MODE_MAIN)
    amitk_object_unselect(object, AMITK_SELECTION_ALL);

  /* recursive remove children */
  children = AMITK_OBJECT_CHILDREN(object);
  while (children != NULL) {
    tree_view_remove_object(tree_view, children->data);
    children = children->next;
  }

  /* disconnect the object's signals */
  g_signal_handlers_disconnect_by_func(G_OBJECT(object), G_CALLBACK(tree_view_object_update_cb), tree_view);
  g_signal_handlers_disconnect_by_func(G_OBJECT(object), G_CALLBACK(tree_view_object_add_child_cb), tree_view);
  g_signal_handlers_disconnect_by_func(G_OBJECT(object), G_CALLBACK(tree_view_object_remove_child_cb), tree_view);

  if (AMITK_IS_STUDY(object)) {
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), G_CALLBACK(tree_view_study_view_mode_cb), tree_view);
  }
  if (AMITK_IS_DATA_SET(object)) {
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), G_CALLBACK(tree_view_data_set_color_table_cb), tree_view);
  }
  
  /* remove the object */
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));
  gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
  
  /* and unref */
  amitk_object_unref(object);

  return;
}


/* preferences and progress_dialog can be NULL, nicer if they're not */
GtkWidget* amitk_tree_view_new (AmitkTreeViewMode tree_mode,
				AmitkPreferences * preferences,
				GtkWidget * progress_dialog) {

  AmitkTreeView * tree_view;
  AmitkViewMode i_view_mode;
  gchar * temp_string;
  GtkTreeStore * store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;

  tree_view = g_object_new(amitk_tree_view_get_type(), NULL);
  tree_view->mode = tree_mode;

  /* if given preferences, save them */
  if (preferences != NULL)
    tree_view->preferences = g_object_ref(preferences);
  else
    tree_view->preferences = NULL;

  /* if given a progress dialog, save it */
  if (progress_dialog != NULL)
    tree_view->progress_dialog = g_object_ref(progress_dialog);
  else
    tree_view->progress_dialog = NULL;


  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), FALSE);

  store = gtk_tree_store_new(NUM_COLUMNS, 
			     G_TYPE_BOOLEAN, /* COLUMN_MULTIPLE_SELECTION */
			     G_TYPE_BOOLEAN, /* COLUMN_VISIBLE_SINGLE */
			     G_TYPE_BOOLEAN,/* COLUMN_VISIBLE_LINKED_2WAY */
			     G_TYPE_BOOLEAN, /* COLUMN_VISIBLE_LINKED_3WAY */
			     G_TYPE_STRING,
			     GDK_TYPE_PIXBUF, 
			     G_TYPE_STRING, 
			     G_TYPE_POINTER);
  gtk_tree_view_set_model (GTK_TREE_VIEW(tree_view), GTK_TREE_MODEL (store));
  g_object_unref(store);

  renderer = gtk_cell_renderer_toggle_new ();
  column = gtk_tree_view_column_new_with_attributes("", renderer, /* "visible" */
						    "active", COLUMN_MULTIPLE_SELECTION, NULL);

  for (i_view_mode = 0; i_view_mode < AMITK_VIEW_MODE_NUM; i_view_mode++) {
    renderer = gtk_cell_renderer_toggle_new ();
    temp_string = g_strdup_printf("%d", i_view_mode+1);
    tree_view->select_column[i_view_mode] = 
      gtk_tree_view_column_new_with_attributes(temp_string, renderer, /* "visible" */
					       "active", COLUMN_VISIBLE_SINGLE+i_view_mode, NULL);
    g_free(temp_string);
  }

  switch(tree_view->mode) {
  case AMITK_TREE_VIEW_MODE_MAIN:
    /* by default, only first select column is shown */
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), tree_view->select_column[AMITK_VIEW_MODE_SINGLE]);
    break;
  case AMITK_TREE_VIEW_MODE_SINGLE_SELECTION:
    break;
  case AMITK_TREE_VIEW_MODE_MULTIPLE_SELECTION:
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
    break;
  default:
    g_error("unexpected case in %s at %d\n", __FILE__, __LINE__);
    break;
  }

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("", renderer, /* "expand "*/
						    "text", COLUMN_EXPANDER, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
  gtk_tree_view_set_expander_column(GTK_TREE_VIEW(tree_view), column);

  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes("", renderer, /* "icon" */
						    "pixbuf", COLUMN_ICON, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("name", renderer,
						    "text", COLUMN_NAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);


  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
  switch (tree_view->mode) {
  case AMITK_TREE_VIEW_MODE_MAIN:
  case AMITK_TREE_VIEW_MODE_SINGLE_SELECTION:
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
    break;
  case AMITK_TREE_VIEW_MODE_MULTIPLE_SELECTION:
  default:
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);
    break;
  }

  return GTK_WIDGET (tree_view);
}


void amitk_tree_view_set_study(AmitkTreeView * tree_view, AmitkStudy * study) {

  g_return_if_fail(AMITK_IS_TREE_VIEW(tree_view));
  g_return_if_fail(AMITK_IS_STUDY(study));

  tree_view_add_object(tree_view, AMITK_OBJECT(study));

  return;
}

void amitk_tree_view_expand_object(AmitkTreeView * tree_view, AmitkObject * object) {

  GtkTreeIter iter;
  GtkTreeModel * model;
  GtkTreePath * path;

  g_return_if_fail(AMITK_IS_TREE_VIEW(tree_view));
  g_return_if_fail(AMITK_IS_OBJECT(object));

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));

  if (tree_view_find_object(tree_view, object, &iter)) {
    path = gtk_tree_model_get_path(model, &iter);
    gtk_tree_view_expand_row(GTK_TREE_VIEW(tree_view), path, FALSE);
    gtk_tree_path_free(path);
  }

  return;
}

/* this function can return NULL */
AmitkObject * amitk_tree_view_get_active_object(AmitkTreeView * tree_view) {

  g_return_val_if_fail(AMITK_IS_TREE_VIEW(tree_view), NULL);
  return tree_view->active_object;
}

void amitk_tree_view_set_active_object(AmitkTreeView * tree_view, AmitkObject * object) {

  GtkTreeSelection *selection;
  GtkTreeIter iter;

  g_return_if_fail(AMITK_IS_TREE_VIEW(tree_view));
  g_return_if_fail((tree_view->mode == AMITK_TREE_VIEW_MODE_MAIN) ||
		   (tree_view->mode == AMITK_TREE_VIEW_MODE_SINGLE_SELECTION));

  if (tree_view->active_object != NULL) {
    amitk_object_unref(tree_view->active_object);
    tree_view->active_object = NULL;
  }

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
  if (object == NULL) {

    /* if any item is marked as active, unselect it */
    if (gtk_tree_selection_get_selected (selection, NULL, &iter))
      gtk_tree_selection_unselect_iter(selection, &iter);

  } else {
    g_return_if_fail(AMITK_IS_OBJECT(object));
    if (tree_view_find_object(tree_view, object, &iter)) {
      
      tree_view->active_object = amitk_object_ref(object);
      if (!gtk_tree_selection_iter_is_selected(selection, &iter))
	gtk_tree_selection_select_iter(selection, &iter);

    }
  }

  return;
}



gboolean multiple_selection_foreach(GtkTreeModel *model,
				    GtkTreePath *path,
				    GtkTreeIter *iter,
				    gpointer data) {

  GList ** pobjects = data;
  gboolean multiple_selection;
  AmitkObject * object;

  gtk_tree_model_get(model, iter, COLUMN_OBJECT, &object, 
		     COLUMN_MULTIPLE_SELECTION, &multiple_selection,
		     -1);

  if (multiple_selection)
    *pobjects = g_list_append(*pobjects, amitk_object_ref(object));

  return FALSE;
}

GList * amitk_tree_view_get_multiple_selection_objects(AmitkTreeView * tree_view) {

  GList * objects=NULL;
  GtkTreeModel *tree_model;

  g_return_val_if_fail(AMITK_IS_TREE_VIEW(tree_view), NULL);
  g_return_val_if_fail(tree_view->mode == AMITK_TREE_VIEW_MODE_MULTIPLE_SELECTION, NULL);

  tree_model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view));
  gtk_tree_model_foreach(tree_model, multiple_selection_foreach, &objects);

  return objects;
}

