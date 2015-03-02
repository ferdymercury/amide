/* amitk_tree.c
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

/* adapted from gtkcolorsel.c */

#include "amide_config.h"
#include "amitk_tree.h"
#include "amitk_study.h"
#include "amitk_marshal.h"
#include "image.h"
#include <gdk/gdkkeysyms.h>



#if 0
#define AMITK_TREE_DND_OBJECT_TYPE 	         "pointer/object"

enum {
	AMITK_TREE_DND_OBJECT
};

static const GtkTargetEntry drag_types[] = {
  { AMITK_TREE_DND_OBJECT_TYPE, 0, AMITK_TREE_DND_OBJECT }
};
#endif

enum {
  HELP_EVENT,
  ACTIVATE_OBJECT,
  POPUP_OBJECT,
  ADD_OBJECT,
  DELETE_OBJECT,
  LAST_SIGNAL
} amitk_tree_signals;

enum {
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
} tree_find_t;

static void tree_class_init (AmitkTreeClass *klass);
static void tree_init (AmitkTree *tree);
static void tree_destroy (GtkObject *object);
static void tree_set_view_mode(AmitkTree * tree, AmitkViewMode view_mode);
static void tree_add_roi_cb(GtkWidget * widget, gpointer data);
static void tree_popup_roi_menu(AmitkTree * tree, AmitkObject * parent_object, 
				guint button, guint32 activate_time);
static gboolean tree_button_press_event(GtkWidget *tree,GdkEventButton *event);
static gboolean tree_button_release_event(GtkWidget *tree, GdkEventButton *event);
static gboolean tree_key_press_event(GtkWidget * tree, GdkEventKey * event);
static void tree_emit_help_signal(AmitkTree * tree);
static gboolean tree_motion_notify_event(GtkWidget *widget, GdkEventMotion *event);
static gboolean tree_enter_notify_event(GtkWidget * tree,
					GdkEventCrossing *event);

#if 0
static void tree_drop_cb(GtkWidget * menu, gpointer context);

/* source drag-n-drop */
static void tree_drag_begin       (GtkWidget        *widget,
				   GdkDragContext   *context);
static void tree_drag_end         (GtkWidget        *widget,
				   GdkDragContext   *context);
static void tree_drag_data_get    (GtkWidget        *widget,
				   GdkDragContext   *context,
				   GtkSelectionData *selection_data,
				   guint             info,
				   guint             time);
static void tree_drag_data_delete (GtkWidget        *widget,
				   GdkDragContext   *context);


/* target drag-n-drop */
static void     tree_drag_leave         (GtkWidget        *widget,
					 GdkDragContext   *context,
					 guint             time);
static gboolean tree_drag_motion        (GtkWidget        *widget,
					 GdkDragContext   *context,
					 gint              x,
					 gint              y,
					 guint             time);
static gboolean tree_drag_drop          (GtkWidget        *widget,
					 GdkDragContext   *context,
					 gint              x,
					 gint              y,
					 guint             time);
static void     tree_drag_data_received (GtkWidget        *widget,
					 GdkDragContext   *context,
					 gint              x,
					 gint              y,
					 GtkSelectionData *selection_data,
					 guint             info,
					 guint             time);
#endif

static void tree_object_update_cb(AmitkObject * object, gpointer tree);
static void tree_object_add_child_cb(AmitkObject * parent, AmitkObject * child, gpointer tree);
static void tree_object_remove_child_cb(AmitkObject * parent, AmitkObject * child, gpointer tree);
static gboolean tree_find_recurse(GtkTreeModel *model, GtkTreePath *path, 
				  GtkTreeIter *iter, gpointer data);
static gboolean tree_find_object(AmitkTree * tree, AmitkObject * object, GtkTreeIter * iter);
static void tree_add_object(AmitkTree * tree, AmitkObject * object);
static void tree_remove_object(AmitkTree * tree, AmitkObject * object);

static GtkTreeViewClass *parent_class;
static guint tree_signals[LAST_SIGNAL];


GType amitk_tree_get_type (void) {

  static GType tree_type = 0;

  if (!tree_type)
    {
      static const GTypeInfo tree_info =
      {
	sizeof (AmitkTreeClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) tree_class_init,
	(GClassFinalizeFunc) NULL,
	NULL, /* class data */
	sizeof (AmitkTree),
	0, /* # preallocs */
	(GInstanceInitFunc) tree_init,
	NULL /* value table */
      };

      tree_type = g_type_register_static(GTK_TYPE_TREE_VIEW, "AmitkTree", &tree_info, 0);
    }

  return tree_type;
}

static void tree_class_init (AmitkTreeClass *klass)
{
  GtkObjectClass *gtkobject_class;
  GtkTreeViewClass *tree_class;
  GtkWidgetClass *widget_class;

  gtkobject_class = (GtkObjectClass*) klass;
  widget_class    = (GtkWidgetClass*) klass;
  tree_class    = (GtkTreeViewClass*) klass;

  parent_class = g_type_class_peek_parent(klass);

  gtkobject_class->destroy = tree_destroy;

  widget_class->button_press_event = tree_button_press_event;
  widget_class->button_release_event = tree_button_release_event;
  widget_class->key_press_event = tree_key_press_event;
  widget_class->motion_notify_event = tree_motion_notify_event;
  widget_class->enter_notify_event = tree_enter_notify_event;

#if 0
  widget_class->drag_begin = tree_drag_begin;
  widget_class->drag_end = tree_drag_end;
  widget_class->drag_data_get = tree_drag_data_get;
  widget_class->drag_data_delete = tree_drag_data_delete;
  widget_class->drag_leave = tree_drag_leave; 
  widget_class->drag_motion = tree_drag_motion;
  widget_class->drag_drop = tree_drag_drop;
  widget_class->drag_data_received = tree_drag_data_received;
#endif

  tree_signals[HELP_EVENT] =
    g_signal_new ("help_event",
		  G_TYPE_FROM_CLASS(klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkTreeClass, help_event),
		  NULL, NULL,
		  amitk_marshal_NONE__ENUM, 
		  G_TYPE_NONE, 1,
		  AMITK_TYPE_HELP_INFO);
  tree_signals[ACTIVATE_OBJECT] =
    g_signal_new ("activate_object",
		  G_TYPE_FROM_CLASS(klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkTreeClass, activate_object),
		  NULL, NULL,
		  amitk_marshal_NONE__OBJECT,
		  G_TYPE_NONE, 1,
		  AMITK_TYPE_OBJECT);
  tree_signals[POPUP_OBJECT] =
    g_signal_new ("popup_object",
		  G_TYPE_FROM_CLASS(klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkTreeClass, popup_object),
		  NULL, NULL,
		  amitk_marshal_NONE__OBJECT,
		  G_TYPE_NONE, 1,
		  AMITK_TYPE_OBJECT);
  tree_signals[ADD_OBJECT] =
    g_signal_new ("add_object",
		  G_TYPE_FROM_CLASS(klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkTreeClass, add_object),
		  NULL, NULL,
		  amitk_marshal_NONE__OBJECT_ENUM_ENUM, 
		  G_TYPE_NONE,3,
		  AMITK_TYPE_OBJECT,
		  AMITK_TYPE_OBJECT_TYPE,
		  AMITK_TYPE_ROI_TYPE);
  tree_signals[DELETE_OBJECT] =
    g_signal_new ("delete_object",
		  G_TYPE_FROM_CLASS(klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkTreeClass, delete_object),
		  NULL, NULL,
		  amitk_marshal_NONE__OBJECT, 
		  G_TYPE_NONE,1,
		  AMITK_TYPE_OBJECT);

}

static void tree_init (AmitkTree * tree)
{
  tree->study = NULL;
  tree->active_object = NULL;
  tree->mouse_x = -1;
  tree->mouse_y = -1;
  tree->current_path = NULL;
  tree->prev_view_mode = AMITK_VIEW_MODE_SINGLE;
#if 0
  tree->press_x = -1;
  tree->press_y = -1;
  tree->drag_begin_possible = FALSE;
  tree->src_object = NULL;
  tree->dest_object = NULL;
  tree->drag_list = gtk_target_list_new(drag_types, G_N_ELEMENTS(drag_types));

  /* setup ability to do drag-n-drop */
  gtk_drag_dest_set (GTK_WIDGET (tree),
		     GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT,
    		     drag_types, G_N_ELEMENTS(drag_types),
		     GDK_ACTION_ASK);
#endif
  
}

static void tree_destroy (GtkObject * object) {

  AmitkTree * tree;

  g_return_if_fail (object != NULL);
  g_return_if_fail (AMITK_IS_TREE (object));

  tree =  AMITK_TREE (object);

  if (tree->current_path != NULL) {
    gtk_tree_path_free(tree->current_path);
    tree->current_path = NULL;
  }

  if (tree->active_object != NULL) {
    tree->active_object = amitk_object_unref(tree->active_object);
  }

  if (tree->study != NULL) {
    tree_remove_object(tree, AMITK_OBJECT(tree->study));
    tree->study = NULL;
  }

#if 0
  if (tree->drag_list != NULL) {
    gtk_target_list_unref (tree->drag_list);
    tree->drag_list = NULL;
  }
#endif

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}



static void tree_set_view_mode(AmitkTree * tree, AmitkViewMode view_mode) {

  AmitkViewMode i_view_mode;

  /* remove all linked columns, we'll add them back below if needed */
  for (i_view_mode = 0; i_view_mode <= tree->prev_view_mode; i_view_mode++) {
    g_object_ref(tree->select_column[i_view_mode]); /* view_remove_column removes a reference */
    gtk_tree_view_remove_column (GTK_TREE_VIEW (tree), tree->select_column[i_view_mode]);
  }
  tree->prev_view_mode = view_mode;

  /* insert_column doesn't add a reference */
  for (i_view_mode = 0; i_view_mode <= view_mode; i_view_mode++) 
    gtk_tree_view_insert_column (GTK_TREE_VIEW (tree), tree->select_column[i_view_mode],
				 COLUMN_VISIBLE_SINGLE+i_view_mode);

  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), view_mode != AMITK_VIEW_MODE_SINGLE);

  return;
}

/* callback function for adding an roi */
static void tree_add_roi_cb(GtkWidget * widget, gpointer data) {

  AmitkTree * tree = data;
  AmitkRoiType roi_type;
  AmitkObject * parent_object;

  /* figure out which menu item called me */
  roi_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"roi_type"));
  parent_object = g_object_get_data(G_OBJECT(widget), "parent");
  g_signal_emit(G_OBJECT(tree),  tree_signals[ADD_OBJECT], 0, 
		parent_object, AMITK_OBJECT_TYPE_ROI, roi_type);

  return;
}


static void tree_popup_roi_menu(AmitkTree * tree, AmitkObject * parent_object, 
				guint button, guint32 activate_time) {
  GtkWidget * menu;
  GtkWidget * menuitem;
  AmitkRoiType i_roi_type;

  menu = gtk_menu_new();

  for (i_roi_type=0; i_roi_type<AMITK_ROI_TYPE_NUM; i_roi_type++) {
    menuitem = gtk_menu_item_new_with_label(amitk_roi_menu_explanation[i_roi_type]);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    g_object_set_data(G_OBJECT(menuitem), "roi_type", GINT_TO_POINTER(i_roi_type)); 
    g_object_set_data(G_OBJECT(menuitem), "parent", parent_object);
    g_signal_connect(G_OBJECT(menuitem), "activate",  G_CALLBACK(tree_add_roi_cb), tree);
    gtk_widget_show(menuitem);
  }
  gtk_widget_show(menu);
  
  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, activate_time);
  return;
}

static gboolean tree_button_press_event (GtkWidget      *widget,
					 GdkEventButton *event) {
  
  gint cell_x, cell_y;
  GtkTreeViewColumn * column;
  gboolean return_value;
  GtkTreeSelection *selection;
  gboolean valid_row;
  AmitkTree * tree;

  g_return_val_if_fail (AMITK_IS_TREE (widget), FALSE);
  tree = AMITK_TREE(widget);
  g_return_val_if_fail (event != NULL, FALSE);

#if 0
  g_print("button press\n");

  if (event->button == 1) {
    tree->drag_begin_possible = TRUE;
    tree->press_x = event->x;
    tree->press_y = event->y;
  }
#endif

  valid_row = gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree), event->x, event->y, 
					    &(tree->current_path), 
					    &column, &cell_x, &cell_y);

  /* making the selection mode none reduces some of the flickering caused by us
     manually trying to force which row is selected */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);

  /* run the tree widget's button press event first */
  return_value = GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event);

  /* reset what's the active mark */
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
  amitk_tree_set_active_object(tree, tree->active_object);

  if ((event->button == 3) && !valid_row)
    tree_popup_roi_menu(tree, NULL, event->button, event->time);

  return return_value;
}

static gboolean tree_button_release_event (GtkWidget      *widget,
					   GdkEventButton *event) {
  GtkTreePath * path=NULL;
  gint cell_x, cell_y;
  GtkTreeViewColumn * column;
  AmitkTree * tree;
  
  g_return_val_if_fail (AMITK_IS_TREE (widget), FALSE);
  tree = AMITK_TREE(widget);
  g_return_val_if_fail (event != NULL, FALSE);

#if 0
  g_print("release button\n");
  tree->drag_begin_possible = FALSE;
  //  gtk_drag_finish (tree->drag_context, 
  //		 gboolean        success,
  //		 gboolean        del,
  //		 guint32         time)
  //  gtk_grab_remove (widget);
#endif

  /* figure out if this click was on an item in the tree */
  if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree), event->x, event->y, 
				    &path, &column, &cell_x, &cell_y)) {

    /* make sure this isn't the expand button, and we're trying to expand,
       also check that we're doing a button release on the object we started
       the button press with */
    if (column != gtk_tree_view_get_expander_column(GTK_TREE_VIEW(tree)) &&
	gtk_tree_path_compare(path, tree->current_path)==0) {
      
      GtkTreeModel * model;
      GtkTreeIter iter;
      gboolean select = FALSE;
      gboolean unselect = FALSE;
      gboolean make_active = FALSE;
      gboolean popup = FALSE;
      gboolean add_object = FALSE;
      gboolean visible[AMITK_VIEW_MODE_NUM];
      gboolean visible_at_all;
      AmitkObjectType add_type = -1;
      AmitkObject * object=NULL;
      AmitkViewMode view_mode;
      AmitkViewMode i_view_mode;
      
      model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
      gtk_tree_model_get_iter(model, &iter, path);
      gtk_tree_model_get(model, &iter, COLUMN_OBJECT, &object, 
			 COLUMN_VISIBLE_SINGLE, &(visible[AMITK_VIEW_MODE_SINGLE]),
			 COLUMN_VISIBLE_LINKED_2WAY, &(visible[AMITK_VIEW_MODE_LINKED_2WAY]),
			 COLUMN_VISIBLE_LINKED_3WAY, &(visible[AMITK_VIEW_MODE_LINKED_3WAY]),
			 -1);

      view_mode = AMITK_VIEW_MODE_SINGLE;/* default */
      visible_at_all = visible[AMITK_VIEW_MODE_SINGLE]; 

      for (i_view_mode = AMITK_VIEW_MODE_NUM-1; i_view_mode > 0; i_view_mode--) {
	visible_at_all = visible_at_all || visible[i_view_mode];
	if (column == tree->select_column[i_view_mode]) {
	  view_mode = i_view_mode;
	}
      }

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
	if (object != NULL)
	  if (AMITK_IS_DATA_SET(object)) {
	    add_object=TRUE;
	    if (event->state & GDK_SHIFT_MASK)
	      add_type=AMITK_OBJECT_TYPE_ROI;
	    else
	      add_type=AMITK_OBJECT_TYPE_FIDUCIAL_MARK;
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
	g_signal_emit(G_OBJECT(tree), tree_signals[ACTIVATE_OBJECT], 0,object);
      
      if (popup)
	g_signal_emit(G_OBJECT(tree), tree_signals[POPUP_OBJECT], 0, object);
      
      if ((add_object) && (add_type==AMITK_OBJECT_TYPE_FIDUCIAL_MARK))
	g_signal_emit(G_OBJECT(tree),  tree_signals[ADD_OBJECT], 0, object, add_type, 0);
      
      if ((add_object) && (add_type==AMITK_OBJECT_TYPE_ROI))
	tree_popup_roi_menu(tree, object, event->button, event->time);
    }

    gtk_tree_path_free(path);
  }

  if (tree->current_path != NULL) {
    gtk_tree_path_free(tree->current_path);
    tree->current_path = NULL;
  }

  return GTK_WIDGET_CLASS (parent_class)->button_release_event (widget, event);
  
}

static gboolean tree_key_press_event(GtkWidget * tree,
				     GdkEventKey * event) {

  GtkTreePath * path;
  gint cell_x, cell_y;
  GtkTreeModel * model;
  GtkTreeIter iter;
  AmitkObject * object;
      

  g_return_val_if_fail(AMITK_IS_TREE(tree), FALSE);

  if (event->state & GDK_CONTROL_MASK) 
    if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree), 
				      AMITK_TREE(tree)->mouse_x, 
				      AMITK_TREE(tree)->mouse_y, 
				      &path, NULL, &cell_x, &cell_y)) {

      model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
      gtk_tree_model_get_iter(model, &iter, path);
      gtk_tree_path_free(path);
      gtk_tree_model_get(model, &iter, COLUMN_OBJECT, &object, -1);
      
      if ((event->keyval == GDK_x) || (event->keyval == GDK_X))
	if (!AMITK_IS_STUDY(object))
	  g_signal_emit(G_OBJECT(tree), tree_signals[DELETE_OBJECT], 0, object);
    }

  return GTK_WIDGET_CLASS (parent_class)->key_press_event (tree, event);
}


static void tree_emit_help_signal(AmitkTree * tree) {

  GtkTreePath * path=NULL;
  gint cell_x, cell_y;
  AmitkHelpInfo help_type;
  GtkTreeModel * model;
  GtkTreeIter iter;
  AmitkObject * object;


  if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree), tree->mouse_x, tree->mouse_y,
				    &path, NULL, &cell_x, &cell_y)) {

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_path_free(path);
    gtk_tree_model_get(model, &iter, COLUMN_OBJECT, &object, -1);

    if (AMITK_IS_STUDY(object))
      help_type = AMITK_HELP_INFO_TREE_STUDY;
    else if (AMITK_IS_DATA_SET(object))
      help_type = AMITK_HELP_INFO_TREE_DATA_SET;
    else if (AMITK_IS_ROI(object))
      help_type = AMITK_HELP_INFO_TREE_ROI;
    else if (AMITK_IS_FIDUCIAL_MARK(object))
      help_type = AMITK_HELP_INFO_TREE_FIDUCIAL_MARK;
    else {
      g_error("unknown object type in %s at %d\n", __FILE__, __LINE__);
      help_type = AMITK_HELP_INFO_TREE_NONE;
    }
  } else {
    help_type = AMITK_HELP_INFO_TREE_NONE;
  }

  g_signal_emit(G_OBJECT(tree), tree_signals[HELP_EVENT], 0,help_type);

  return;
}



/* we have to track motion notify for several reasons:
   1. know which entry we're on for button_press events
   2. context sensitive help
   3. are we initiating a drag?
*/
static gboolean tree_motion_notify_event(GtkWidget *widget, GdkEventMotion *event) {

  AmitkTree * tree;
#if 0
  GdkDragContext *context;
  GtkTreePath * path=NULL;
  GtkTreeIter iter;
  GdkPixbuf * pixbuf;
  AmitkObject * object;
  gint cell_x, cell_y;
  GtkTreeModel * model;
#endif

  g_return_val_if_fail(AMITK_IS_TREE(widget), FALSE);
  tree = AMITK_TREE(widget);

  tree->mouse_x = event->x;
  tree->mouse_y = event->y;
  tree_emit_help_signal(tree);

  /* see if we can start a drag */
#if 0
  if (tree->drag_begin_possible) {

    /* see if we've moved the mouse enough */
    if (gtk_drag_check_threshold(widget, tree->press_x, tree->press_y,
				 event->x, event->y)) {
      tree->drag_begin_possible = FALSE;

      /* figure out which object we're manipulating */
      if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree), 
					tree->mouse_x, tree->mouse_y,
					&path, NULL, &cell_x, &cell_y)) {

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_path_free(path);
	gtk_tree_model_get(model, &iter, COLUMN_OBJECT, &object, -1);
	g_return_val_if_fail(AMITK_IS_OBJECT(object), FALSE);


	/* if object is appropriate, start dragging */
	if (!AMITK_IS_STUDY(object)) {
	  tree->src_object = object;
	  g_print("start drag\n");
	  context = gtk_drag_begin(widget, tree->drag_list, 
				   GDK_ACTION_ASK, 1, (GdkEvent*)event);
	  
	  pixbuf = image_get_object_pixbuf(object);
	  gtk_drag_set_icon_pixbuf(context, pixbuf, -10,-10);
	  g_object_unref(pixbuf);
	}
      }
    }
  }
#endif

  /* pass the signal on */
  return GTK_WIDGET_CLASS (parent_class)->motion_notify_event (widget, event);
}



static gboolean tree_enter_notify_event(GtkWidget * tree,
					GdkEventCrossing *event) {

  GtkTreeSelection *selection;
  GtkTreeIter iter;
  AmitkTree * amitk_tree;

  g_return_val_if_fail(AMITK_IS_TREE(tree), FALSE);
  amitk_tree = AMITK_TREE(tree);

  AMITK_TREE(tree)->mouse_x = event->x;
  AMITK_TREE(tree)->mouse_y = event->y;
  tree_emit_help_signal(AMITK_TREE(tree));

  gtk_widget_grab_focus(tree); /* move the keyboard entry focus into the tree */

  /* double check that the right row is selected - gtk sometimes moves the selection to the 1st row */
  if (amitk_tree->active_object != NULL) {
    if (tree_find_object(amitk_tree, amitk_tree->active_object, &iter)) {
      selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
      if (!gtk_tree_selection_iter_is_selected(selection, &iter))
	gtk_tree_selection_select_iter(selection, &iter);
    }
  }

  /* pass the signal on */
  return (* GTK_WIDGET_CLASS (parent_class)->enter_notify_event) (tree, event);
}


#if 0
static void tree_drop_cb(GtkWidget * menu, gpointer data) {

  GdkDragContext * context =data;
  AmitkTree * tree;
  gboolean move;

  move = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menu), "move"));
  tree = g_object_get_data(G_OBJECT(menu), "tree");

  return;
}

static void tree_drag_begin (GtkWidget *widget, GdkDragContext *context)
{
  g_print("drag begin\n");
  /* figure out what we're suppose to be dragging */
}

static void tree_drag_end         (GtkWidget        *widget,
				   GdkDragContext   *context) {
  g_print("drag end\n");
}
static void tree_drag_data_get    (GtkWidget        *widget,
				   GdkDragContext   *context,
				   GtkSelectionData *selection_data,
				   guint             info,
				   guint             time) {

  g_print("drag data get from %s\n", AMITK_OBJECT_NAME(AMITK_TREE(widget)->study));

  gtk_selection_data_set_text (selection_data, "blah blah", -1);
      
  /* record what we're dragging */
}

static void tree_drag_data_delete (GtkWidget        *widget,
				   GdkDragContext   *context) {
  g_print("drag data delete\n");
}


static void     tree_drag_leave         (GtkWidget        *widget,
					 GdkDragContext   *context,
					 guint             time) {

  g_print("target leave\n");

  /* pass the signal on */
}

static gboolean tree_drag_motion        (GtkWidget        *widget,
					 GdkDragContext   *context,
					 gint              x,
					 gint              y,
					 guint             time) {

  g_print("target motion %d %d\n", x, y);
  //  gdk_drag_status (context, 0, time);
  // gtk_drag_finish (context, FALSE, FALSE, time);

  //  valid_row = gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree), event->x, event->y, 
  //					    &(AMITK_TREE(tree)->current_path), 
  //					    &column, &cell_x, &cell_y);


  return TRUE;
}

static gboolean tree_drag_drop          (GtkWidget        *widget,
					 GdkDragContext   *context,
					 gint              x,
					 gint              y,
					 guint             time) {


  //  GtkWidget * menu;
  //  GtkWidget * menuitem;
  GdkAtom target = GDK_NONE;

  g_print("drop on %d %d %s\n", x, y,  AMITK_OBJECT_NAME(AMITK_TREE(widget)->study));
  /* check if drop is valid */

  /* record which object we're moving/copying, and to where */
  
  /* see if we want to copy or move */
  //  menu = gtk_menu_new();

  //  menuitem = gtk_menu_item_new_with_label("Move");
  //  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  //  g_object_set_data(G_OBJECT(menuitem), "move", GINT_TO_POINTER(TRUE));
  //  g_object_set(data(G_OBJECT(menuitem), "tree", tree);
  //  g_signal_connect(G_OBJECT(menuitem), "activate",  G_CALLBACK(tree_drop_cb), context);
  //  gtk_widget_show(menuitem);

  //  menuitem = gtk_menu_item_new_with_label("Copy");
  //  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  //  g_object_set_data(G_OBJECT(menuitem), "move", GINT_TO_POINTER(FALSE));
  //  g_object_set(data(G_OBJECT(menuitem), "tree", tree);
  //  g_signal_connect(G_OBJECT(menuitem), "activate",  G_CALLBACK(tree_drop_cb), context);
  //  gtk_widget_show(menuitem);

  //  gtk_widget_show(menu);
  
  //  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 1, time);

  
  target = gtk_drag_dest_find_target (widget, context, NULL);

  if (target != GDK_NONE) {
    g_print("target\n");
    gtk_drag_get_data (widget, context, target, time);
  } else {
    gtk_drag_finish (context, FALSE, FALSE, time);
  }
  

  /* Drag and drop didn't happen! */
  //  gtk_drag_finish (context, FALSE, FALSE, time);


  return TRUE;
}


static void     tree_drag_data_received (GtkWidget        *widget,
					 GdkDragContext   *context,
					 gint              x,
					 gint              y,
					 GtkSelectionData *selection_data,
					 guint             info,
					 guint             time) {

  gchar *str;

  str = gtk_selection_data_get_text (selection_data);

  g_print("target receive x %d y %d    info %d action %d selection %s\n", x,y,info, context->suggested_action,str);
  g_free(str);
  //  if (context->suggested_action == GDK_ACTION_COPY)  

}

#endif


static void tree_study_view_mode_cb(AmitkStudy * study, gpointer data) {

  AmitkTree * tree = data;

  g_return_if_fail(AMITK_IS_TREE(tree));
  g_return_if_fail(AMITK_IS_STUDY(study));

  tree_set_view_mode(tree, AMITK_STUDY_VIEW_MODE(study));
  return;
}

static void tree_object_update_cb(AmitkObject * object, gpointer data) {

  AmitkTree * tree = data;
  GtkTreeIter iter;
  GtkTreeModel * model;
  GdkPixbuf * pixbuf;

  g_return_if_fail(AMITK_IS_TREE(tree));
  g_return_if_fail(AMITK_IS_OBJECT(object));

  if (tree_find_object(tree, object, &iter)) {
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
    pixbuf = image_get_object_pixbuf(object);
    gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
		       COLUMN_ICON, pixbuf,
		       COLUMN_NAME, AMITK_OBJECT_NAME(object), 
		       COLUMN_VISIBLE_SINGLE, amitk_object_get_selected(object, AMITK_VIEW_MODE_SINGLE),
		       COLUMN_VISIBLE_LINKED_2WAY, amitk_object_get_selected(object, AMITK_VIEW_MODE_LINKED_2WAY),
		       COLUMN_VISIBLE_LINKED_3WAY, amitk_object_get_selected(object, AMITK_VIEW_MODE_LINKED_3WAY),
		       -1);
    g_object_unref(pixbuf);
  }

  if (tree->active_object == object) {
    if (!amitk_object_get_selected(object, AMITK_SELECTION_ANY)) /* we're unselecting the active object */
      g_signal_emit(G_OBJECT(tree), tree_signals[ACTIVATE_OBJECT], 0,NULL);
  } else if (tree->active_object == NULL) { /* currently no active object */
    if (amitk_object_get_selected(object, AMITK_SELECTION_ANY)) /* we've selected something */
      g_signal_emit(G_OBJECT(tree), tree_signals[ACTIVATE_OBJECT], 0,object);
  }

  return;
}

static void tree_object_add_child_cb(AmitkObject * parent, AmitkObject * child, gpointer data) {

  AmitkTree * tree = data;

  g_return_if_fail(AMITK_IS_TREE(tree));
  g_return_if_fail(AMITK_IS_OBJECT(child));

  tree_add_object(AMITK_TREE(tree), child);

  return;
}

static void tree_object_remove_child_cb(AmitkObject * parent, AmitkObject * child, gpointer data) {
  AmitkTree * tree = data;

  g_return_if_fail(AMITK_IS_TREE(tree));
  g_return_if_fail(AMITK_IS_OBJECT(child));

  tree_remove_object(AMITK_TREE(tree), child);

  return;
}

static gboolean tree_find_recurse(GtkTreeModel *model, GtkTreePath *path, 
				  GtkTreeIter *iter, gpointer data) {
  tree_find_t * tree_find=data;
  AmitkObject * object;

  gtk_tree_model_get(model, iter, COLUMN_OBJECT, &object, -1);

  g_return_val_if_fail(object != NULL, FALSE);

  if (object == tree_find->object) {
    tree_find->iter = *iter;
    tree_find->found = TRUE;
    return TRUE;
  } else {
    return FALSE;
  }

}

static gboolean tree_find_object(AmitkTree * tree, AmitkObject * object, GtkTreeIter * iter) {

  GtkTreeModel * model;
  tree_find_t tree_find;

  g_return_val_if_fail(AMITK_IS_TREE(tree),FALSE);

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
  tree_find.object = object;
  tree_find.found = FALSE;

  gtk_tree_model_foreach(model, tree_find_recurse, &tree_find);

  if (tree_find.found) {
    *iter = tree_find.iter;
    return TRUE;
  } else 
    return FALSE;
}


static void tree_add_object(AmitkTree * tree, AmitkObject * object) {

  GList * children;
  GtkTreeIter parent_iter;
  GtkTreeIter iter;
  GtkTreeModel * model;
  GdkPixbuf * pixbuf;

  amitk_object_ref(object); /* add a reference */

  if (AMITK_IS_STUDY(object)) { /* save a pointer to thestudy object */
    if (tree->study != NULL) {
      tree_remove_object(tree, AMITK_OBJECT(tree->study));
    }
    tree->study = AMITK_STUDY(object);
    tree_set_view_mode(tree, AMITK_STUDY_VIEW_MODE(object));
  }
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));

  if (AMITK_OBJECT_PARENT(object) == NULL) 
    gtk_tree_store_append (GTK_TREE_STORE(model), &iter, NULL);  /* Acquire a top-level iterator */
  else {
    if (tree_find_object(tree, AMITK_OBJECT_PARENT(object), &parent_iter))
      gtk_tree_store_append (GTK_TREE_STORE(model), &iter, &parent_iter);
    else
      g_return_if_reached();
  }

  pixbuf = image_get_object_pixbuf(object);

  gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
		     COLUMN_VISIBLE_SINGLE, amitk_object_get_selected(object, AMITK_VIEW_MODE_SINGLE),
		     COLUMN_VISIBLE_LINKED_2WAY, amitk_object_get_selected(object, AMITK_VIEW_MODE_LINKED_2WAY),
		     COLUMN_VISIBLE_LINKED_3WAY, amitk_object_get_selected(object, AMITK_VIEW_MODE_LINKED_3WAY),
		     COLUMN_EXPANDER, "*",
		     COLUMN_ICON, pixbuf,
		     COLUMN_NAME, AMITK_OBJECT_NAME(object),
		     COLUMN_OBJECT, object, -1);
  g_object_unref(pixbuf);

  g_signal_connect(G_OBJECT(object), "object_name_changed", G_CALLBACK(tree_object_update_cb), tree);
  g_signal_connect(G_OBJECT(object), "object_selection_changed", G_CALLBACK(tree_object_update_cb), tree);
  if (AMITK_IS_DATA_SET(object)) {
    g_signal_connect(G_OBJECT(object), "modality_changed", G_CALLBACK(tree_object_update_cb), tree);
    g_signal_connect(G_OBJECT(object), "color_table_changed", G_CALLBACK(tree_object_update_cb), tree);
  } else if (AMITK_IS_ROI(object)) {
    g_signal_connect(G_OBJECT(object), "roi_type_changed", G_CALLBACK(tree_object_update_cb), tree);
  } else if (AMITK_IS_STUDY(object)) {
    g_signal_connect(G_OBJECT(object), "view_mode_changed", G_CALLBACK(tree_study_view_mode_cb), tree);
  }
  g_signal_connect(G_OBJECT(object), "object_add_child", G_CALLBACK(tree_object_add_child_cb), tree);
  g_signal_connect(G_OBJECT(object), "object_remove_child", G_CALLBACK(tree_object_remove_child_cb), tree);

  /* add children */
  children = AMITK_OBJECT_CHILDREN(object);
  while (children != NULL) {
      tree_add_object(tree, children->data);
      children = children->next;
  }


}

static void tree_remove_object(AmitkTree * tree, AmitkObject * object) {

  GList * children;
  GtkTreeIter iter;
  GtkTreeModel * model;

  g_return_if_fail(AMITK_IS_TREE(tree));
  g_return_if_fail(tree_find_object(tree, object, &iter)); /* shouldn't fail */

  /* unselect the object */
  amitk_object_unselect(object, AMITK_SELECTION_ALL);

  /* recursive remove children */
  children = AMITK_OBJECT_CHILDREN(object);
  while (children != NULL) {
    tree_remove_object(tree, children->data);
    children = children->next;
  }

  /* disconnect the object's signals */
  g_signal_handlers_disconnect_by_func(G_OBJECT(object), G_CALLBACK(tree_object_update_cb), tree);
  g_signal_handlers_disconnect_by_func(G_OBJECT(object), G_CALLBACK(tree_object_add_child_cb), tree);
  g_signal_handlers_disconnect_by_func(G_OBJECT(object), G_CALLBACK(tree_object_remove_child_cb), tree);

  if (AMITK_IS_STUDY(object)) {
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), G_CALLBACK(tree_study_view_mode_cb), tree);
  }
  
  /* remove the object */
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
  gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
  
  /* and unref */
  amitk_object_unref(object);

  return;
}


GtkWidget* amitk_tree_new (void) {

  AmitkTree * tree;
  AmitkViewMode i_view_mode;
  gchar * temp_string;
  GtkTreeStore * store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;

  tree = g_object_new(amitk_tree_get_type(), NULL);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);

  store = gtk_tree_store_new(NUM_COLUMNS, 
			     G_TYPE_BOOLEAN, /* COLUMN_VISIBLE_SINGLE */
			     G_TYPE_BOOLEAN,/* COLUMN_VISIBLE_LINKED_2WAY */
			     G_TYPE_BOOLEAN, /* COLUMN_VISIBLE_LINKED_3WAY */
			     G_TYPE_STRING,
			     GDK_TYPE_PIXBUF, 
			     G_TYPE_STRING, 
			     G_TYPE_POINTER);
  gtk_tree_view_set_model (GTK_TREE_VIEW(tree), GTK_TREE_MODEL (store));
  g_object_unref(store);

  for (i_view_mode = 0; i_view_mode < AMITK_VIEW_MODE_NUM; i_view_mode++) {
    renderer = gtk_cell_renderer_toggle_new ();
    temp_string = g_strdup_printf("%d", i_view_mode+1);
    tree->select_column[i_view_mode] = 
      gtk_tree_view_column_new_with_attributes(temp_string, renderer, /* "visible" */
					       "active", COLUMN_VISIBLE_SINGLE+i_view_mode, NULL);
    g_free(temp_string);
  }

  /* by default, only first select column is shown */
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), tree->select_column[AMITK_VIEW_MODE_SINGLE]);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("", renderer, /* "expand "*/
						    "text", COLUMN_EXPANDER, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  gtk_tree_view_set_expander_column(GTK_TREE_VIEW(tree), column);

  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes("", renderer, /* "icon" */
						    "pixbuf", COLUMN_ICON, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes("name", renderer,
						    "text", COLUMN_NAME, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);


  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

  return GTK_WIDGET (tree);
}


void amitk_tree_set_study(AmitkTree * tree, AmitkStudy * study) {

  g_return_if_fail(AMITK_IS_TREE(tree));
  g_return_if_fail(AMITK_IS_STUDY(study));

  tree_add_object(tree, AMITK_OBJECT(study));

  return;
}

void amitk_tree_expand_object(AmitkTree * tree, AmitkObject * object) {

  GtkTreeIter iter;
  GtkTreeModel * model;
  GtkTreePath * path;

  g_return_if_fail(AMITK_IS_TREE(tree));
  g_return_if_fail(AMITK_IS_OBJECT(object));

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));

  if (tree_find_object(tree, object, &iter)) {
    path = gtk_tree_model_get_path(model, &iter);
    gtk_tree_view_expand_row(GTK_TREE_VIEW(tree), path, FALSE);
    gtk_tree_path_free(path);
  }

  return;
}


void amitk_tree_set_active_object(AmitkTree * tree, AmitkObject * object) {

  GtkTreeSelection *selection;
  GtkTreeIter iter;

  g_return_if_fail(AMITK_IS_TREE(tree));

  if (tree->active_object != NULL) {
    amitk_object_unref(tree->active_object);
    tree->active_object = NULL;
  }

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
  if (object == NULL) {

    /* if any item is marked as active, unselect it */
    if (gtk_tree_selection_get_selected (selection, NULL, &iter))
      gtk_tree_selection_unselect_iter(selection, &iter);

  } else {
    g_return_if_fail(AMITK_IS_OBJECT(object));
    if (tree_find_object(tree, object, &iter)) {
      
      tree->active_object = amitk_object_ref(object);
      if (!gtk_tree_selection_iter_is_selected(selection, &iter))
	gtk_tree_selection_select_iter(selection, &iter);

    }
  }

  return;
}



