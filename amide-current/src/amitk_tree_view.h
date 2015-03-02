/* amitk_tree_view.h
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

#ifndef __AMITK_TREE_VIEW_H__
#define __AMITK_TREE_VIEW_H__

/* includes we always need with this widget */
#include <gtk/gtk.h>
#include "amitk_type_builtins.h"
#include "amitk_study.h"

G_BEGIN_DECLS

#define AMITK_TYPE_TREE_VIEW            (amitk_tree_view_get_type ())
#define AMITK_TREE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), AMITK_TYPE_TREE_VIEW, AmitkTreeView))
#define AMITK_TREE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_THESHOLD, AmitkTreeViewClass))
#define AMITK_IS_TREE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AMITK_TYPE_TREE_VIEW))
#define AMITK_IS_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_TREE_VIEW))

typedef enum {
  AMITK_TREE_VIEW_MODE_MAIN,
  AMITK_TREE_VIEW_MODE_SINGLE_SELECTION,
  AMITK_TREE_VIEW_MODE_MULTIPLE_SELECTION,
  AMITK_TREE_VIEW_MODE_NUM
} AmitkTreeViewMode;

typedef struct _AmitkTreeView             AmitkTreeView;
typedef struct _AmitkTreeViewClass        AmitkTreeViewClass;

struct _AmitkTreeView
{
  GtkTreeView tree_view;
  AmitkTreeViewMode mode;

  AmitkStudy * study;
  AmitkObject * active_object;
  AmitkPreferences * preferences;
  GtkWidget * progress_dialog;

  GtkTreeViewColumn * select_column[AMITK_VIEW_MODE_NUM];
  AmitkViewMode prev_view_mode;
  gint mouse_x; /* the current mouse position */
  gint mouse_y; 
  GtkTreePath * current_path;

  /* drag-n-drop info */
  gboolean drag_begin_possible;
  gint press_x;
  gint press_y;
  AmitkObject * src_object; /* not referenced */
  AmitkObject * dest_object; /* not referenced */
  GtkTargetList * drag_list;
  
};

struct _AmitkTreeViewClass
{
  GtkTreeViewClass parent_class;

  
  void (* help_event)                (AmitkTreeView * tree_view,
				      AmitkHelpInfo   help_type);
  void (* activate_object)           (AmitkTreeView * tree,
				      AmitkObject   * object);
  void (* popup_object)              (AmitkTreeView * tree_view,
				      AmitkObject   * object);
  void (* add_object)                (AmitkTreeView * tree_view,
				      AmitkObject   * parent,
				      AmitkObjectType type,
				      AmitkRoiType    roi_type);
  void (* delete_object)             (AmitkTreeView * tree_view,
				      AmitkObject   * object);
};  


GType           amitk_tree_view_get_type          (void);
GtkWidget*      amitk_tree_view_new               (AmitkTreeViewMode tree_mode,
						   AmitkPreferences * preferences,
						   GtkWidget * progress_dialog);
void            amitk_tree_view_set_study         (AmitkTreeView * tree_view,
					           AmitkStudy * study);
void            amitk_tree_view_expand_object     (AmitkTreeView * tree_view,
						   AmitkObject * object);
AmitkObject *   amitk_tree_view_get_active_object (AmitkTreeView * tree_view);
void            amitk_tree_view_set_active_object (AmitkTreeView * tree_view,
						   AmitkObject * object);
GList *         amitk_tree_view_get_multiple_selection_objects(AmitkTreeView * tree_view);

G_END_DECLS

#endif /* __AMITK_TREE_VIEW_H__ */

