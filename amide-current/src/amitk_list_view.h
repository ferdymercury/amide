/* amitk_list_view.h
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

#ifndef __AMITK_LIST_VIEW_H__
#define __AMITK_LIST_VIEW_H__

/* includes we always need with this widget */
#include <gtk/gtk.h>
#include "amide.h"
#include "amitk_type_builtins.h"
#include "amitk_study.h"

G_BEGIN_DECLS

#define AMITK_TYPE_LIST_VIEW            (amitk_list_view_get_type ())
#define AMITK_LIST_VIEW(obj)            (GTK_CHECK_CAST ((obj), AMITK_TYPE_LIST_VIEW, AmitkListView))
#define AMITK_LIST_VIEW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), AMITK_TYPE_THESHOLD, AmitklistViewClass))
#define AMITK_IS_LIST_VIEW(obj)         (GTK_CHECK_TYPE ((obj), AMITK_TYPE_LIST_VIEW))
#define AMITK_IS_LIST_VIEW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_LIST_VIEW))

typedef struct _AmitkListView             AmitkListView;
typedef struct _AmitkListViewClass        AmitkListViewClass;

struct _AmitkListView
{
  GtkTreeView tree;

  AmitkStudy * study;

};

struct _AmitkListViewClass
{
  GtkTreeViewClass parent_class;

  
  void (* select_object)             (AmitkListView * list,
				      AmitkObject   * object);
};  


GType           amitk_list_view_get_type          (void);
GtkWidget*      amitk_list_view_new               (void);
void            amitk_list_view_set_study         (AmitkListView * list_view,
						   AmitkStudy * study);

G_END_DECLS

#endif /* __AMITK_LIST_VIEW_H__ */

