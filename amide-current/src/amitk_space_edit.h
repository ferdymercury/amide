/* amitk_space_edit.h
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


#ifndef __AMITK_SPACE_EDIT_H__
#define __AMITK_SPACE_EDIT_H__

/* includes we always need with this widget */
#include <gtk/gtk.h>
#include "amitk_object.h"

G_BEGIN_DECLS

#define AMITK_TYPE_SPACE_EDIT            (amitk_space_edit_get_type ())
#define AMITK_SPACE_EDIT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), AMITK_TYPE_SPACE_EDIT, AmitkSpaceEdit))
#define AMITK_SPACE_EDIT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_SPACE_EDIT, AmitkSpaceEditClass))
#define AMITK_IS_SPACE_EDIT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AMITK_TYPE_SPACE_EDIT))
#define AMITK_IS_SPACE_EDIT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_SPACE_EDIT))

typedef struct _AmitkSpaceEdit             AmitkSpaceEdit;
typedef struct _AmitkSpaceEditClass        AmitkSpaceEditClass;


struct _AmitkSpaceEdit
{
  GtkVBox vbox;

  GtkWidget * entry[AMITK_AXIS_NUM][AMITK_AXIS_NUM];

  AmitkObject * object;

};

struct _AmitkSpaceEditClass
{
  GtkVBoxClass parent_class;

};  


GType         amitk_space_edit_get_type          (void);
GtkWidget*    amitk_space_edit_new               (AmitkObject * object);


G_END_DECLS

#endif /* __AMITK_SPACE_EDIT_H__ */

