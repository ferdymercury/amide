/* amitk_tree.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2002 Andy Loening
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

/* adapted from gtkcolorsel.c */


#include "config.h"
#include "amitk_tree.h"
#include "amitk_tree_item.h"
#include "study.h"

enum {
  HELP_EVENT,
  SELECT_OBJECT,
  UNSELECT_OBJECT,
  MAKE_ACTIVE_OBJECT,
  POPUP_OBJECT,
  ADD_OBJECT,
  LAST_SIGNAL
} amitk_tree_signals;

static void tree_class_init (AmitkTreeClass *klass);
static void tree_init (AmitkTree *tree);
static void tree_destroy (GtkObject *object);
static gint tree_button_press_event(GtkWidget *widget,
				    GdkEventButton *event);
static gint tree_button_release_event(GtkWidget *widget,
				      GdkEventButton *event);
static void tree_item_help_cb        (GtkWidget *tree_item, 
				      help_info_t help_type, 
				      gpointer tree);

static GtkTree *tree_parent_class = NULL;
static guint tree_signals[LAST_SIGNAL] = {0};

typedef void (*AmitkSignal_NONE__POINTER_ENUM_POINTER_ENUM) (GtkObject * object,
							     gpointer arg1,
							     gint arg2, 
							     gpointer arg3,
							     gint arg4, 
							     gpointer user_data);
void amitk_marshal_NONE__POINTER_ENUM_POINTER_ENUM (GtkObject * object,
						    GtkSignalFunc func, 
						    gpointer func_data, 
						    GtkArg * args) {
  AmitkSignal_NONE__POINTER_ENUM_POINTER_ENUM rfunc;
  rfunc = (AmitkSignal_NONE__POINTER_ENUM_POINTER_ENUM) func;
  (*rfunc) (object, 
	    GTK_VALUE_POINTER(args[0]),
	    GTK_VALUE_ENUM(args[1]),
	    GTK_VALUE_POINTER(args[2]),
	    GTK_VALUE_ENUM(args[3]),
	    func_data);
}

typedef void (*AmitkSignal_NONE__ENUM_POINTER_ENUM) (GtkObject * object,
						     gint arg1, 
						     gpointer arg2,
						     gint arg3, 
						     gpointer user_data);
void amitk_marshal_NONE__ENUM_POINTER_ENUM (GtkObject * object,
					    GtkSignalFunc func, 
					    gpointer func_data, 
					    GtkArg * args) {
  AmitkSignal_NONE__ENUM_POINTER_ENUM rfunc;
  rfunc = (AmitkSignal_NONE__ENUM_POINTER_ENUM) func;
  (*rfunc) (object, 
	    GTK_VALUE_ENUM(args[0]),
	    GTK_VALUE_POINTER(args[1]),
	    GTK_VALUE_ENUM(args[2]),
	    func_data);
}


GtkType amitk_tree_get_type (void) {

  static GtkType tree_type = 0;

  if (!tree_type)
    {
      static const GtkTypeInfo tree_info =
      {
	"AmitkTree",
	sizeof (AmitkTree),
	sizeof (AmitkTreeClass),
	(GtkClassInitFunc) tree_class_init,
	(GtkObjectInitFunc) tree_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      tree_type = gtk_type_unique (GTK_TYPE_TREE, &tree_info);
    }

  return tree_type;
}

static void tree_class_init (AmitkTreeClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) klass;
  widget_class = (GtkWidgetClass*) klass;

  tree_parent_class = gtk_type_class (GTK_TYPE_TREE);

  tree_signals[HELP_EVENT] =
    gtk_signal_new ("help_event",
  		    GTK_RUN_FIRST,
		    object_class->type,
  		    GTK_SIGNAL_OFFSET (AmitkTreeClass, help_event),
  		    gtk_marshal_NONE__ENUM, 
		    GTK_TYPE_NONE, 1,
		    GTK_TYPE_ENUM);
  tree_signals[SELECT_OBJECT] =
    gtk_signal_new ("select_object",
  		    GTK_RUN_FIRST,
		    object_class->type,
  		    GTK_SIGNAL_OFFSET (AmitkTreeClass, select_object),
  		    amitk_marshal_NONE__POINTER_ENUM_POINTER_ENUM, 
		    GTK_TYPE_NONE, 4,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_ENUM,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_ENUM);
  tree_signals[UNSELECT_OBJECT] =
    gtk_signal_new ("unselect_object",
  		    GTK_RUN_FIRST,
		    object_class->type,
  		    GTK_SIGNAL_OFFSET (AmitkTreeClass, unselect_object),
  		    amitk_marshal_NONE__POINTER_ENUM_POINTER_ENUM, 
		    GTK_TYPE_NONE, 4,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_ENUM,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_ENUM);
  tree_signals[MAKE_ACTIVE_OBJECT] =
    gtk_signal_new ("make_active_object",
  		    GTK_RUN_FIRST,
		    object_class->type,
  		    GTK_SIGNAL_OFFSET (AmitkTreeClass, make_active_object),
  		    amitk_marshal_NONE__POINTER_ENUM_POINTER_ENUM, 
		    GTK_TYPE_NONE, 4,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_ENUM,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_ENUM);
  tree_signals[POPUP_OBJECT] =
    gtk_signal_new ("popup_object",
  		    GTK_RUN_FIRST,
		    object_class->type,
  		    GTK_SIGNAL_OFFSET (AmitkTreeClass, popup_object),
  		    amitk_marshal_NONE__POINTER_ENUM_POINTER_ENUM, 
		    GTK_TYPE_NONE, 4,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_ENUM,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_ENUM);
  tree_signals[ADD_OBJECT] =
    gtk_signal_new ("add_object",
  		    GTK_RUN_FIRST,
		    object_class->type,
  		    GTK_SIGNAL_OFFSET (AmitkTreeClass, popup_object),
  		    amitk_marshal_NONE__ENUM_POINTER_ENUM, 
		    GTK_TYPE_NONE, 3,
		    GTK_TYPE_ENUM,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_ENUM);
  gtk_object_class_add_signals (object_class, tree_signals, LAST_SIGNAL);

  object_class->destroy = tree_destroy;

  widget_class->button_press_event = tree_button_press_event;
  widget_class->button_release_event = tree_button_release_event;

}

static void tree_init (AmitkTree * tree)
{

  tree->active_leaf = NULL;

  tree->selected_volumes = NULL;
  tree->selected_rois = NULL;

  tree->clicked=FALSE;
}

static void tree_destroy (GtkObject * object) {

  AmitkTree * tree;

  g_return_if_fail (object != NULL);
  g_return_if_fail (AMITK_IS_TREE (object));

  tree =  AMITK_TREE (object);

  tree->selected_volumes = volumes_unref(tree->selected_volumes);
  tree->selected_rois = rois_unref(tree->selected_rois);

  if (GTK_OBJECT_CLASS (tree_parent_class)->destroy)
    (* GTK_OBJECT_CLASS (tree_parent_class)->destroy) (object);
}




static gint tree_button_press_event (GtkWidget      *widget,
				     GdkEventButton *event)
{
  AmitkTree *tree;
  GtkWidget *item;
  AmitkTreeItem *tree_item;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (AMITK_IS_TREE (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  

  tree = AMITK_TREE (widget);
  tree->clicked=TRUE;

  item = gtk_get_event_widget ((GdkEvent*) event);

  while (item && !AMITK_IS_TREE_ITEM (item))
    item = item->parent;
  
  if (!item || (item->parent != widget))
    return FALSE;
  tree_item = AMITK_TREE_ITEM(item);
  
  //  switch(event->button) 
  //    {
  //    case 1:
  //      gtk_tree_select_child (tree, item);
  //      break;
  //    case 2:
  //      if(GTK_TREE_ITEM(item)->subtree) gtk_tree_item_expand(GTK_TREE_ITEM(item));
  //      break;
  //    case 3:
  //      if(GTK_TREE_ITEM(item)->subtree) gtk_tree_item_collapse(GTK_TREE_ITEM(item));
  //      break;
  //    }
  
  return TRUE;
}



static gint tree_button_release_event (GtkWidget * widget,
				       GdkEventButton *event) {

  AmitkTree *tree;
  GtkWidget *item;
  AmitkTreeItem *tree_item;
  gboolean select = FALSE;
  gboolean unselect = FALSE;
  gboolean make_active = FALSE;
  gboolean popup = FALSE;
  gboolean add_object = FALSE;
  object_t add_type = -1;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (AMITK_IS_TREE (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  tree = AMITK_TREE (widget);

  /* make sure to avoid spurious events, yes, this is ugly */
  if (!tree->clicked)
    return FALSE;
  tree->clicked=FALSE;

  item = gtk_get_event_widget ((GdkEvent*) event);

  while (item && !AMITK_IS_TREE_ITEM (item))
    item = item->parent;
  
  if (!item || (item->parent != widget))
    return FALSE;
  
  tree_item = AMITK_TREE_ITEM(item);

  switch (event->button) {

  case 1: /* left button */
    if (GTK_WIDGET_STATE(item) == GTK_STATE_SELECTED) 
      unselect = TRUE;
    else
      select = TRUE;
    break;

  case 2:
    make_active = TRUE;
    if (GTK_WIDGET_STATE(item) != GTK_STATE_SELECTED) 
      select = TRUE;
    break;

  case 3:
    if (event->state & GDK_SHIFT_MASK) {
      switch(tree_item->type) {
      case VOLUME:
	add_object=TRUE;
	add_type=ALIGN_PT;
	break;
      default:
	break;
      }
    } else {
      popup = TRUE;
    }
    break;

  default:
    g_error("unexpected case in %s at line %d",__FILE__, __LINE__);
    break;
  }

  if (tree_item->type == STUDY) 
    select = unselect = make_active = FALSE;

  if (select) {
    amitk_tree_select_object(tree, tree_item->object, tree_item->type);
  } else if (unselect) {
    amitk_tree_unselect_object(tree, tree_item->object, tree_item->type);
  } 

  if (make_active) {
    gtk_signal_emit(GTK_OBJECT(GTK_TREE(tree)->root_tree), 
		    tree_signals[MAKE_ACTIVE_OBJECT], 
		    tree_item->object, tree_item->type,
		    tree_item->parent, tree_item->parent_type);
  }

  if (popup) {
    gtk_signal_emit(GTK_OBJECT(GTK_TREE(tree)->root_tree), 
		    tree_signals[POPUP_OBJECT], 
		    tree_item->object, tree_item->type,
		    tree_item->parent, tree_item->parent_type);
  }

  if (add_object) {
    gtk_signal_emit(GTK_OBJECT(GTK_TREE(tree)->root_tree), 
		    tree_signals[ADD_OBJECT], add_type,
		    tree_item->object, tree_item->type);
  }

  return FALSE;
}

static void tree_item_help_cb(GtkWidget *widget, help_info_t help_type, gpointer data) {

  AmitkTree * tree = data;
  gtk_signal_emit(GTK_OBJECT(tree), tree_signals[HELP_EVENT], help_type);

  return;
}



GtkWidget* amitk_tree_new (view_mode_t view_mode) {
  AmitkTree * tree;

  tree = gtk_type_new (amitk_tree_get_type ());
  tree->view_mode = view_mode;

  return GTK_WIDGET (tree);
}

void amitk_tree_add_object(AmitkTree * root_tree, 
			   gpointer object, object_t type, 
			   gpointer parent, object_t parent_type,
			   gboolean expand_parent) {
  GtkWidget * leaf;
  GtkWidget * parent_leaf;
  AmitkTreeItem * tree_item;
  volumes_t * volumes=NULL;
  rois_t * rois=NULL;
  align_pts_t * align_pts = NULL;
  GtkWidget * subtree;
  volume_t * volume;
  study_t * study;

  g_return_if_fail(AMITK_IS_TREE(root_tree));

  leaf = amitk_tree_item_new(object, type, parent, parent_type);
  parent_leaf = amitk_tree_find_object(root_tree, parent, parent_type);
  if (parent_leaf != NULL) {
    subtree = AMITK_TREE_ITEM_SUBTREE(parent_leaf);
    if (subtree == NULL) {
      subtree = amitk_tree_new(AMITK_TREE_VIEW_MODE(root_tree));
      gtk_tree_item_set_subtree(GTK_TREE_ITEM(parent_leaf), subtree);
      if (!GTK_WIDGET_REALIZED(parent_leaf))
	  GTK_TREE_ITEM(parent_leaf)->expanded = FALSE;
      else
	gtk_tree_item_collapse(GTK_TREE_ITEM(parent_leaf)); 
    }
    if (expand_parent)
      gtk_tree_item_expand(GTK_TREE_ITEM(parent_leaf)); 
    gtk_tree_append(GTK_TREE(subtree), leaf);
  } else {
    gtk_tree_append(GTK_TREE(root_tree), leaf);
  }
  gtk_signal_connect(GTK_OBJECT(leaf), "help_event",
		     GTK_SIGNAL_FUNC(tree_item_help_cb), root_tree);
  tree_item = AMITK_TREE_ITEM(leaf);
  gtk_widget_show(leaf);

  switch(type) {
  case STUDY:
    study = object;
    volumes = study_volumes(study);
    rois = study_rois(study);
    break;
  case VOLUME:
    volume = object;
    align_pts = volume->align_pts;
    break;
  default:
    break;
  }

  if ((volumes != NULL) || (align_pts != NULL) || (rois != NULL)) {
    while (volumes != NULL) {
      amitk_tree_add_object(AMITK_TREE(root_tree), volumes->volume, VOLUME, 
			    tree_item->object, tree_item->type, TRUE);
      volumes = volumes->next;
    }
    while (rois != NULL) {
      amitk_tree_add_object(AMITK_TREE(root_tree), rois->roi, ROI, 
			    tree_item->object, tree_item->type, TRUE);
      rois = rois->next;
    }
    while (align_pts != NULL) {
      amitk_tree_add_object(AMITK_TREE(root_tree), align_pts->align_pt, ALIGN_PT, 
			    tree_item->object, tree_item->type, FALSE);
      align_pts = align_pts->next;
    }
  }

  return;
}

void amitk_tree_remove_object(AmitkTree * tree, gpointer object, object_t type) {

  GtkWidget * root_leaf;
  GtkWidget * parent_leaf;
  AmitkTreeItem * child_leaf;
  GtkWidget * subtree;
  GList * leaves;

  g_return_if_fail(AMITK_IS_TREE(tree));

  root_leaf = amitk_tree_find_object(tree, object, type);

  if (root_leaf != NULL) {
    g_return_if_fail(AMITK_IS_TREE_ITEM(root_leaf));

    /* recursive remove child objet */
    subtree = AMITK_TREE_ITEM_SUBTREE(AMITK_TREE_ITEM(root_leaf));
    
    if (subtree != NULL) {
      leaves = GTK_TREE(subtree)->children;
      
      while (leaves != NULL) {
	child_leaf = leaves->data;
	amitk_tree_remove_object(tree, child_leaf->object, child_leaf->type);
	leaves = GTK_TREE(subtree)->children;
      }
    }

    /* make sure it and it's children are unselected */
    amitk_tree_unselect_object(tree, 
			       AMITK_TREE_ITEM(root_leaf)->object, 
			       AMITK_TREE_ITEM(root_leaf)->type);

    /* and remove - need to know the tree item's parent leaf  */
    parent_leaf = amitk_tree_find_object(tree, 
					 AMITK_TREE_ITEM(root_leaf)->parent, 
					 AMITK_TREE_ITEM(root_leaf)->parent_type);
    if (parent_leaf != NULL)
      tree = AMITK_TREE(AMITK_TREE_ITEM_SUBTREE(AMITK_TREE_ITEM(parent_leaf)));
    gtk_tree_remove_item(GTK_TREE(tree), root_leaf);
  }

  return;

}

GtkWidget * amitk_tree_find_object(AmitkTree * tree, gpointer object, object_t type) {

  GList * leaves;
  GtkWidget * leaf;  
  GtkWidget * subtree;
  GtkWidget * subleaf;

  g_return_val_if_fail(AMITK_IS_TREE(tree), NULL);

  /* find the leaf that corresponds to the given object */
  leaves = GTK_TREE(tree)->children;

  while (leaves != NULL) {
    leaf = leaves->data;

    if (AMITK_TREE_ITEM(leaf)->object == object)
      return leaf;
    else {
      subtree = AMITK_TREE_ITEM_SUBTREE(leaf);
      if (subtree != NULL) {
	subleaf = amitk_tree_find_object(AMITK_TREE(subtree), object, type);
	if (subleaf != NULL)
	  return subleaf;
      }
    }

    leaves = leaves->next;
  }

  return NULL;
}


void amitk_tree_set_active_mark(AmitkTree * tree, gpointer object,object_t type) {

  g_return_if_fail(AMITK_IS_TREE(tree));

  if (tree->active_leaf != NULL)
    amitk_tree_item_set_active_mark(AMITK_TREE_ITEM(tree->active_leaf), FALSE);

  tree->active_leaf = amitk_tree_find_object(tree, object, type);

  if (tree->active_leaf != NULL)
    amitk_tree_item_set_active_mark(AMITK_TREE_ITEM(tree->active_leaf), TRUE);

  return;
}


void amitk_tree_select_object (AmitkTree * tree,gpointer object,object_t type) {

  GtkWidget * leaf;
  AmitkTree * root_tree;

  g_return_if_fail(AMITK_IS_TREE(tree));
  g_return_if_fail(AMITK_IS_TREE(GTK_TREE(tree)->root_tree));
  root_tree = AMITK_TREE(GTK_TREE(tree)->root_tree);

  leaf = amitk_tree_find_object(tree, object, type);

  if (leaf != NULL) {
    switch(AMITK_TREE_ITEM(leaf)->type) {
    case VOLUME:
      root_tree->selected_volumes = 
	volumes_add_volume(root_tree->selected_volumes, AMITK_TREE_ITEM(leaf)->object);
      break;
    case ROI:
      root_tree->selected_rois = 
	rois_add_roi(root_tree->selected_rois, AMITK_TREE_ITEM(leaf)->object);
      break;
    default:
      break;
    }
    gtk_tree_select_child(GTK_TREE(tree), leaf); 
    gtk_signal_emit(GTK_OBJECT(root_tree), 
		    tree_signals[SELECT_OBJECT], 
		    AMITK_TREE_ITEM(leaf)->object, 
		    AMITK_TREE_ITEM(leaf)->type,
		    AMITK_TREE_ITEM(leaf)->parent, 
		    AMITK_TREE_ITEM(leaf)->parent_type);
  }

  return;
}

void amitk_tree_unselect_object(AmitkTree * tree, gpointer object, object_t type) {

  GList * leaves;
  GtkWidget * root_leaf;
  AmitkTreeItem * child_leaf;
  GtkWidget * subtree;
  AmitkTree * root_tree;

  g_return_if_fail(AMITK_IS_TREE(tree));
  g_return_if_fail(AMITK_IS_TREE(GTK_TREE(tree)->root_tree));
  root_tree = AMITK_TREE(GTK_TREE(tree)->root_tree);

  root_leaf = amitk_tree_find_object(tree, object, type);

  if (root_leaf != NULL) {
    subtree = AMITK_TREE_ITEM_SUBTREE(AMITK_TREE_ITEM(root_leaf));

    if (subtree != NULL) {
      leaves = GTK_TREE(subtree)->children;

      while (leaves != NULL) {
	child_leaf = leaves->data;
	g_return_if_fail(AMITK_IS_TREE_ITEM(child_leaf));
	
	if (GTK_WIDGET_STATE(child_leaf) == GTK_STATE_SELECTED) 
	  amitk_tree_unselect_object(tree, child_leaf->object, child_leaf->type);
	
	leaves = leaves->next;
      }
    }
    if (GTK_WIDGET_STATE(root_leaf) == GTK_STATE_SELECTED) {
      switch(AMITK_TREE_ITEM(root_leaf)->type) {
      case VOLUME:
	root_tree->selected_volumes = 
	  volumes_remove_volume(root_tree->selected_volumes, AMITK_TREE_ITEM(root_leaf)->object);
	break;
      case ROI:
	root_tree->selected_rois = 
	  rois_remove_roi(root_tree->selected_rois, AMITK_TREE_ITEM(root_leaf)->object);
	break;
      default:
	break;
      }
      gtk_tree_unselect_child (GTK_TREE(tree), root_leaf);
      gtk_signal_emit(GTK_OBJECT(root_tree), 
		      tree_signals[UNSELECT_OBJECT], 
		      AMITK_TREE_ITEM(root_leaf)->object, 
		      AMITK_TREE_ITEM(root_leaf)->type,
		      AMITK_TREE_ITEM(root_leaf)->parent, 
		      AMITK_TREE_ITEM(root_leaf)->parent_type);
    }
  }
  return;
}

void amitk_tree_update_object(AmitkTree * tree, gpointer object, object_t type) {

  GtkWidget * leaf;
  AmitkTree * root_tree;

  g_return_if_fail(AMITK_IS_TREE(tree));
  g_return_if_fail(AMITK_IS_TREE(GTK_TREE(tree)->root_tree));
  root_tree = AMITK_TREE(GTK_TREE(tree)->root_tree);

  leaf = amitk_tree_find_object(tree, object, type);

  if (leaf != NULL) {
    amitk_tree_item_update_object(AMITK_TREE_ITEM(leaf), object, type);
    //    gtk_signal_emit(GTK_OBJECT(root_tree), 
    //		    tree_signals[UPDATE_OBJECT], 
    //		    AMITK_TREE_ITEM(leaf)->object, 
    //		    AMITK_TREE_ITEM(leaf)->type,
    //		    AMITK_TREE_ITEM(leaf)->parent, 
    //		    AMITK_TREE_ITEM(leaf)->parent_type);
  }


}


/* returns the selected alignment points for a given volume */
align_pts_t * amitk_tree_selected_pts(AmitkTree * tree, gpointer parent, object_t parent_type) {

  GtkWidget * parent_leaf;
  GList * leaves;
  AmitkTreeItem * child_leaf;
  GtkWidget * subtree;
  align_pts_t * pts=NULL;

  g_return_val_if_fail(AMITK_IS_TREE(tree), NULL);

  parent_leaf = amitk_tree_find_object(tree, parent, parent_type);
  if (parent_leaf != NULL) {
    subtree = AMITK_TREE_ITEM_SUBTREE(AMITK_TREE_ITEM(parent_leaf));
    
    if (subtree != NULL) {
      leaves = GTK_TREE(subtree)->children;

      while (leaves != NULL) {
	if (AMITK_IS_TREE_ITEM(leaves->data)) {
	  child_leaf = leaves->data;

	  if (GTK_WIDGET_STATE(child_leaf) == GTK_STATE_SELECTED) 
	    if (child_leaf->type == ALIGN_PT)
	      pts = align_pts_add_pt(pts, child_leaf->object);
	}
	leaves = leaves->next;
      }
    }
  }

  return pts;
}
