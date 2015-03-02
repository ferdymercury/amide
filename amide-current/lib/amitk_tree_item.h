/* amitk_tree_item.h
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

#ifndef __AMITK_TREE_ITEM_H__
#define __AMITK_TREE_ITEM_H__


#include <gnome.h>
#include "amide.h"
#include "amitk_tree.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define AMITK_TYPE_TREE_ITEM              (amitk_tree_item_get_type ())
#define AMITK_TREE_ITEM(obj)              (GTK_CHECK_CAST ((obj), AMITK_TYPE_TREE_ITEM, AmitkTreeItem))
#define AMITK_TREE_ITEM_CLASS(klass)      (GTK_CHECK_CLASS_CAST ((klass), AMITK_TYPE_TREE_ITEM, AmitkTreeItemClass))
#define AMITK_IS_TREE_ITEM(obj)           (GTK_CHECK_TYPE ((obj), AMITK_TYPE_TREE_ITEM))
#define AMITK_IS_TREE_ITEM_CLASS(klass)   (GTK_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_TREE_ITEM))

#define AMITK_TREE_ITEM_SUBTREE(obj)      (GTK_TREE_ITEM(obj)->subtree)
#define AMITK_TREE_ITEM_DIALOG(obj)       (AMITK_TREE_ITEM(obj)->dialog)


typedef struct _AmitkTreeItem       AmitkTreeItem;
typedef struct _AmitkTreeItemClass  AmitkTreeItemClass;

struct _AmitkTreeItem
{
  GtkTreeItem item;

  object_t type;
  gpointer object;

  object_t parent_type;
  gpointer parent;

  GtkWidget * active_label;
  GtkWidget * label;
  GtkWidget * pixmap;

  GtkWidget * dialog;
};

struct _AmitkTreeItemClass
{
  GtkTreeItemClass parent_class;

  void (* help_event)                (AmitkTreeItem *tree_item,
				      help_info_t   help_type);
};


GtkType    amitk_tree_item_get_type       (void);
GtkWidget* amitk_tree_item_new            (gpointer object,
					   object_t type,
					   gpointer parent,
					   object_t parent_type);
void       amitk_tree_item_set_active_mark(AmitkTreeItem * tree_item,
					   gboolean active);
void       amitk_tree_item_update_object  (AmitkTreeItem * tree_item,
					   gpointer object,
					   object_t type);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_TREE_ITEM_H__ */
