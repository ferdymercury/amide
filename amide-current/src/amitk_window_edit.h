/* amitk_window_edit.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2005-2014 Andy Loening
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


#ifndef __AMITK_WINDOW_EDIT_H__
#define __AMITK_WINDOW_EDIT_H__

/* includes we always need with this widget */
#include <gtk/gtk.h>
#include <amitk_data_set.h>

G_BEGIN_DECLS

#define AMITK_TYPE_WINDOW_EDIT            (amitk_window_edit_get_type ())
#define AMITK_WINDOW_EDIT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), AMITK_TYPE_WINDOW_EDIT, AmitkWindowEdit))
#define AMITK_WINDOW_EDIT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_WINDOW_EDIT, AmitkWindowEditClass))
#define AMITK_IS_WINDOW_EDIT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AMITK_TYPE_WINDOW_EDIT))
#define AMITK_IS_WINDOW_EDIT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_WINDOW_EDIT))

typedef struct _AmitkWindowEdit             AmitkWindowEdit;
typedef struct _AmitkWindowEditClass        AmitkWindowEditClass;


struct _AmitkWindowEdit
{

  GtkTable table;

  
  GtkWidget * limit_label[AMITK_LIMIT_NUM];
  GtkWidget * window_spin[AMITK_WINDOW_NUM][AMITK_LIMIT_NUM];

  /* used if a data_set is specified */
  AmitkDataSet * data_set;
  GtkWidget * insert_button[AMITK_WINDOW_NUM];

  /* only used if no data_set specified */
  AmitkPreferences * preferences;

};

struct _AmitkWindowEditClass
{
  GtkVBoxClass parent_class;

};  


GType         amitk_window_edit_get_type           (void);
GtkWidget*    amitk_window_edit_new                (AmitkDataSet * data_set,
						    AmitkPreferences * preferences);


G_END_DECLS

#endif /* __AMITK_WINDOW_EDIT_H__ */

