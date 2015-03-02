/* amitk_tree_item.c
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


#include "config.h"
#include "amitk_tree_item.h"
#include "../pixmaps/study.xpm"
#include "../pixmaps/ALIGN_PT.xpm"
#include "study.h"
#include "image.h"

enum {
  HELP_EVENT,
  LAST_SIGNAL
} amitk_tree_item_signals;

static gchar * active_mark="X";
static gchar * nonactive_mark=" ";

static void tree_item_class_init (AmitkTreeItemClass *klass);
static void tree_item_init       (AmitkTreeItem      *tree_item);
static void tree_item_destroy    (GtkObject          *object);
static GdkPixmap * tree_item_load_pixmap(GtkWidget * widget, gpointer object, 
					 object_t type, GdkBitmap ** pgdkbitmap);
static void tree_item_realize_pixmap  (GtkWidget          *box,
				       gpointer            tree_item);
static gboolean event_box_help_cb    (GtkWidget          *event_box, 
				      GdkEventCrossing   *event,
				      gpointer           tree_item);
static void tree_item_construct  (AmitkTreeItem      *tree_item);

static GtkTreeItemClass *parent_class = NULL;
static guint tree_item_signals[LAST_SIGNAL] = { 0 };

GtkType amitk_tree_item_get_type (void) {
  static GtkType tree_item_type = 0;

  if (!tree_item_type)
    {
      static const GtkTypeInfo tree_item_info =
      {
	"AmitkTreeItem",
	sizeof (AmitkTreeItem),
	sizeof (AmitkTreeItemClass),
	(GtkClassInitFunc) tree_item_class_init,
	(GtkObjectInitFunc) tree_item_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      tree_item_type = gtk_type_unique (GTK_TYPE_TREE_ITEM, &tree_item_info);
    }

  return tree_item_type;
}

static void tree_item_class_init (AmitkTreeItemClass *class) {

  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;
  GtkItemClass *item_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;
  item_class = (GtkItemClass*) class;
  container_class = (GtkContainerClass*) class;

  parent_class = gtk_type_class (gtk_tree_item_get_type ());
  
  tree_item_signals[HELP_EVENT] =
    gtk_signal_new ("help_event",
  		    GTK_RUN_FIRST,
		    object_class->type,
  		    GTK_SIGNAL_OFFSET (AmitkTreeItemClass, help_event),
  		    gtk_marshal_NONE__ENUM, 
		    GTK_TYPE_NONE, 1,
		    GTK_TYPE_ENUM);
  gtk_object_class_add_signals (object_class, tree_item_signals, LAST_SIGNAL);

  object_class->destroy = tree_item_destroy;

}



static void tree_item_init (AmitkTreeItem *tree_item) {

  g_return_if_fail (tree_item != NULL);
  g_return_if_fail (AMITK_IS_TREE_ITEM (tree_item));

  tree_item->object = NULL;
  tree_item->parent = NULL;
  tree_item->dialog = NULL;


  return;
}


static void tree_item_destroy (GtkObject *object)
{
  AmitkTreeItem* item;

  g_return_if_fail (object != NULL);
  g_return_if_fail (AMITK_IS_TREE_ITEM (object));

  item = AMITK_TREE_ITEM(object);

  switch(item->type) {
  case STUDY:
    item->object = study_unref(item->object);
    break;
  case ROI:
    item->object = roi_unref(item->object);
    break;
  case VOLUME:
    item->object = volume_unref(item->object);
    break;
  case ALIGN_PT:
    item->object = align_pt_unref(item->object);
    break;
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    break;
  }

  switch(item->parent_type) {
  case STUDY:
    item->parent = study_unref(item->parent);
    break;
  case ROI:
    item->parent = roi_unref(item->parent);
    break;
  case VOLUME:
    item->parent = volume_unref(item->parent);
    break;
  case ALIGN_PT:
    item->parent = align_pt_unref(item->parent);
    break;
  default:
    break;
  }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
  
  return;
}



static GdkPixmap * tree_item_load_pixmap(GtkWidget * widget, gpointer object, 
					 object_t type, GdkBitmap ** pgdkbitmap) {

  GdkPixmap * gdkpixmap;

  /* make the pixmap we'll be putting into the study line */
  switch(type) {
  case STUDY:
    gdkpixmap = gdk_pixmap_create_from_xpm_d(gtk_widget_get_parent_window(widget), 
					     pgdkbitmap,NULL,study_xpm);
    break;
  case VOLUME:
    gdkpixmap = image_get_volume_pixmap(object, gtk_widget_get_parent_window(widget), pgdkbitmap);
    break;
  case ROI:
    gdkpixmap = image_get_roi_pixmap(object, gtk_widget_get_parent_window(widget), pgdkbitmap);
    break;
  default:
  case ALIGN_PT:
    gdkpixmap = gdk_pixmap_create_from_xpm_d(gtk_widget_get_parent_window(widget), 
					     pgdkbitmap,NULL,ALIGN_PT_xpm);
    break;
  }

  return gdkpixmap;
}

static void tree_item_realize_pixmap(GtkWidget * box, gpointer data) {

  GdkPixmap * gdkpixmap;
  GdkBitmap * gdkbitmap;
  AmitkTreeItem * tree_item = data;

  g_return_if_fail(box != NULL);
  g_return_if_fail(AMITK_IS_TREE_ITEM(tree_item));
  gdkpixmap = tree_item_load_pixmap(box, tree_item->object, tree_item->type, &gdkbitmap);
  tree_item->pixmap = gtk_pixmap_new(gdkpixmap,gdkbitmap);
  gtk_box_pack_start(GTK_BOX(box), tree_item->pixmap, FALSE, FALSE, 5);
  gtk_widget_show(tree_item->pixmap);

  return;
}

static gboolean event_box_help_cb(GtkWidget * event_box, GdkEventCrossing * event, gpointer data) {

  AmitkTreeItem * tree_item = data;
  help_info_t help_type;
  g_return_val_if_fail(AMITK_IS_TREE_ITEM(tree_item), FALSE);

  help_type = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(event_box), "which_help"));

  gtk_signal_emit(GTK_OBJECT(tree_item), tree_item_signals[HELP_EVENT], help_type);

  return FALSE;
}


static void tree_item_construct(AmitkTreeItem * tree_item) {

  GtkWidget * hbox;
  GtkWidget * event_box;
  GtkWidget * pix_box;
  volume_t * volume=tree_item->object;
  study_t * study=tree_item->object;
  roi_t * roi=tree_item->object;
  align_pt_t * align_pt=tree_item->object;
  help_info_t help_type;

  switch(tree_item->type) {
  case STUDY:
    tree_item->active_label = NULL;
    tree_item->label = gtk_label_new(study_name(study));
    help_type = HELP_INFO_TREE_STUDY;
    break;
  case ROI:
    tree_item->active_label = gtk_label_new(nonactive_mark);
    gtk_widget_set_usize(tree_item->active_label, 7, -1);
    tree_item->label = gtk_label_new(roi->name);
    help_type = HELP_INFO_TREE_ROI;
    break;
  case VOLUME:
    tree_item->active_label = gtk_label_new(nonactive_mark);
    gtk_widget_set_usize(tree_item->active_label, 7, -1);
    tree_item->label = gtk_label_new(volume->name);
    help_type = HELP_INFO_TREE_VOLUME;
    break;
  case ALIGN_PT:
    tree_item->active_label = gtk_label_new(nonactive_mark);
    gtk_widget_set_usize(tree_item->active_label, 7, -1);
    tree_item->label = gtk_label_new(align_pt->name);
    help_type = HELP_INFO_TREE_ALIGN_PT;
    break;
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    return;
    break;
  }

  pix_box = gtk_hbox_new(FALSE, 0); /*make a box that'll hold the pixmap when it gets realized */
  gtk_signal_connect(GTK_OBJECT(pix_box), "realize",  
		     GTK_SIGNAL_FUNC(tree_item_realize_pixmap), tree_item);

  /* pack the text and icon into a box */
  hbox = gtk_hbox_new(FALSE,0);
  if (tree_item->active_label != NULL) {
    gtk_box_pack_start(GTK_BOX(hbox), tree_item->active_label, FALSE, FALSE, 5);
    gtk_widget_show(tree_item->active_label);
  }
  gtk_box_pack_start(GTK_BOX(hbox), pix_box, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(hbox), tree_item->label, FALSE, FALSE, 5);
  gtk_widget_show(pix_box);
  gtk_widget_show(tree_item->label);
  
  /* make an event box so we can get enter_notify events and display the right help*/
  event_box = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(event_box), hbox);
  gtk_widget_show(hbox);

  gtk_object_set_data(GTK_OBJECT(event_box), "which_help", GINT_TO_POINTER(help_type));
  gtk_signal_connect(GTK_OBJECT(event_box), "enter_notify_event",
  		     GTK_SIGNAL_FUNC(event_box_help_cb), tree_item);
  gtk_signal_connect(GTK_OBJECT(event_box), "leave_notify_event",
  		     GTK_SIGNAL_FUNC(event_box_help_cb), tree_item);


  gtk_container_add(GTK_CONTAINER(tree_item), event_box);
  gtk_widget_show(event_box);
  
  return;
}

GtkWidget* amitk_tree_item_new (gpointer object, object_t type, 
				gpointer parent, object_t parent_type) {

  GtkWidget *item;
  AmitkTreeItem *tree_item;

  item = GTK_WIDGET (gtk_type_new (amitk_tree_item_get_type ()));
  tree_item = AMITK_TREE_ITEM(item);
  
  tree_item->type = type;
  switch(tree_item->type) {
  case STUDY:
    tree_item->object = study_ref(object);
    break;
  case ROI:
    tree_item->object = roi_ref(object);
    break;
  case VOLUME:
    tree_item->object = volume_ref(object);
    break;
  case ALIGN_PT:
    tree_item->object = align_pt_ref(object);
    break;
  default:
    g_error("unexpected case in %s at line %d",__FILE__, __LINE__);
    break;
  }


  tree_item->parent = parent;
  if (tree_item->parent == NULL) {
    tree_item->parent_type = -1;
  } else {
    tree_item->parent_type = parent_type;
    switch(tree_item->parent_type) {
    case STUDY:
      tree_item->parent = study_ref(parent);
      break;
    case ROI:
      tree_item->parent = roi_ref(parent);
      break;
    case VOLUME:
      tree_item->parent = volume_ref(parent);
      break;
    case ALIGN_PT:
      tree_item->parent = align_pt_ref(parent);
      break;
    default:
      g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
      break;
    }
  }

  tree_item_construct(tree_item);

  return item;
}



void amitk_tree_item_set_active_mark(AmitkTreeItem * tree_item, gboolean active) {

  g_return_if_fail(AMITK_IS_TREE_ITEM(tree_item));
  g_return_if_fail(tree_item->active_label != NULL);

  if (active)
    gtk_label_set_text(GTK_LABEL(tree_item->active_label), active_mark);
  else
    gtk_label_set_text(GTK_LABEL(tree_item->active_label), nonactive_mark);

  return;
}

void amitk_tree_item_update_object(AmitkTreeItem * tree_item, gpointer object, object_t type) {

  volume_t * volume = object;
  roi_t * roi = object;
  align_pt_t * align_pt = object;
  study_t * study = object;
  GdkPixmap * gdkpixmap;
  GdkBitmap * gdkbitmap;

  g_return_if_fail(AMITK_IS_TREE_ITEM(tree_item));

  switch(type) {
  case STUDY:
    gtk_label_set_text(GTK_LABEL(tree_item->label), study->name);
    break;
  case ROI:
    gtk_label_set_text(GTK_LABEL(tree_item->label), roi->name);
    gdkpixmap = tree_item_load_pixmap(GTK_WIDGET(tree_item), object, type, &gdkbitmap);
    gtk_pixmap_set(GTK_PIXMAP(tree_item->pixmap), gdkpixmap, gdkbitmap);
    break;
  case VOLUME:
    gtk_label_set_text(GTK_LABEL(tree_item->label), volume->name);
    gdkpixmap = tree_item_load_pixmap(GTK_WIDGET(tree_item), object, type, &gdkbitmap);
    gtk_pixmap_set(GTK_PIXMAP(tree_item->pixmap), gdkpixmap, gdkbitmap);
    break;
  case ALIGN_PT:
    gtk_label_set_text(GTK_LABEL(tree_item->label), align_pt->name);
    break;
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    break;
  }


  return;
}
