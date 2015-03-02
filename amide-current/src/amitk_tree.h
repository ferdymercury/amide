/* amitk_tree.h
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

#ifndef __AMITK_TREE_H__
#define __AMITK_TREE_H__

/* includes we always need with this widget */
#include <gnome.h>
#include "amide.h"
#include "amitk_type_builtins.h"
#include "amitk_study.h"

G_BEGIN_DECLS

#define AMITK_TYPE_TREE            (amitk_tree_get_type ())
#define AMITK_TREE(obj)            (GTK_CHECK_CAST ((obj), AMITK_TYPE_TREE, AmitkTree))
#define AMITK_TREE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), AMITK_TYPE_THESHOLD, AmitkTreeClass))
#define AMITK_IS_TREE(obj)         (GTK_CHECK_TYPE ((obj), AMITK_TYPE_TREE))
#define AMITK_IS_TREE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_TREE))

#define AMITK_TREE_STUDY(obj)          (AMITK_TREE(obj)->study);

typedef struct _AmitkTree             AmitkTree;
typedef struct _AmitkTreeClass        AmitkTreeClass;

struct _AmitkTree
{
  GtkTreeView tree;

  AmitkStudy * study;
  AmitkObject * active_object;

  GtkTreeViewColumn * linked_column;
  gint mouse_x; /* the current mouse position */
  gint mouse_y; 
  GtkTreePath * current_path;
  
};

struct _AmitkTreeClass
{
  GtkTreeViewClass parent_class;

  
  void (* help_event)                (AmitkTree *     tree,
				      AmitkHelpInfo   help_type);
  void (* activate_object)           (AmitkTree *     tree,
				      AmitkObject *   object);
  void (* popup_object)              (AmitkTree *     tree,
				      AmitkObject *   object);
  void (* add_object)                (AmitkTree *     tree,
				      AmitkObject *   parent,
				      AmitkObjectType type,
				      AmitkRoiType    roi_type);
  void (* delete_object)             (AmitkTree *     tree,
				      AmitkObject *   object);
};  


GType           amitk_tree_get_type          (void);
GtkWidget*      amitk_tree_new               (void);
void            amitk_tree_set_study         (AmitkTree * tree,
					      AmitkStudy * study);
void            amitk_tree_expand_object     (AmitkTree * tree,
					      AmitkObject * object);
void            amitk_tree_set_active_object (AmitkTree * tree,
					      AmitkObject * object);
void            amitk_tree_set_linked_mode_column_visible (AmitkTree * tree,
							   gboolean visible);


G_END_DECLS

#endif /* __AMITK_TREE_H__ */

