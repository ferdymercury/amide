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
#include "roi.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define AMITK_TYPE_TREE            (amitk_tree_get_type ())
#define AMITK_TREE(obj)            (GTK_CHECK_CAST ((obj), AMITK_TYPE_TREE, AmitkTree))
#define AMITK_TREE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), AMITK_TYPE_THESHOLD, AmitkTreeClass))
#define AMITK_IS_TREE(obj)         (GTK_CHECK_TYPE ((obj), AMITK_TYPE_TREE))
#define AMITK_IS_TREE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_TREE))

#define AMITK_TREE_SELECTED_VOLUMES(obj)   (AMITK_TREE(obj)->selected_volumes)
#define AMITK_TREE_SELECTED_ROIS(obj)      (AMITK_TREE(obj)->selected_rois)

typedef struct _AmitkTree             AmitkTree;
typedef struct _AmitkTreeClass        AmitkTreeClass;

struct _AmitkTree
{
  GtkTree tree;

  GtkWidget * active_leaf; /* which leaf carries the active symbol */

  volumes_t * selected_volumes;
  rois_t * selected_rois;
  
  gboolean clicked;

};

struct _AmitkTreeClass
{
  GtkTreeClass parent_class;

  
  void (* help_event)                (AmitkTree    *tree,
				      help_info_t  help_type);
  void (* select_object)             (AmitkTree    *tree,
				      gpointer     object,
				      object_t     type,
				      gpointer     parent,
				      object_t     parent_type);
  void (* unselect_object)           (AmitkTree    *tree,
				      gpointer     object,
				      object_t     type,
				      gpointer     parent,
				      object_t     parent_type);
  void (* make_active_object)        (AmitkTree    *tree,
				      gpointer     object,
				      object_t     type,
				      gpointer     parent,
				      object_t     parent_type);
  void (* popup_object)              (AmitkTree    *tree,
				      object_t     type,
				      gpointer     parent,
				      object_t     parent_type);
  void (* add_object)                (AmitkTree    *tree,
				      object_t     type,
				      gpointer     parent,
				      object_t     parent_type);
};  


GtkType         amitk_tree_get_type          (void);
GtkWidget*      amitk_tree_new               (void);
void            amitk_tree_add_object        (AmitkTree * tree, 
					      gpointer object, 
					      object_t type, 
					      gpointer parent,
					      object_t parent_type,
					      gboolean expand_parent);
void            amitk_tree_remove_object     (AmitkTree * tree, 
					      gpointer object, 
					      object_t type);
GtkWidget *     amitk_tree_find_object       (AmitkTree * tree, 
					      gpointer object, 
					      object_t type);
void            amitk_tree_set_active_mark   (AmitkTree * tree,
					      gpointer object,
					      object_t type);
void            amitk_tree_select_object     (AmitkTree * tree,
					      gpointer object,
					      object_t type);
void            amitk_tree_unselect_object   (AmitkTree * tree, 
					      gpointer object, 
					      object_t type);
void            amitk_tree_update_object     (AmitkTree * tree, 
					      gpointer object, 
					      object_t type);
align_pts_t *   amitk_tree_selected_pts      (AmitkTree * tree,
					      gpointer parent,
					      object_t parent_type);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __AMITK_TREE_H__ */

