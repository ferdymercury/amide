/* amitk_color_table_menu.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2003-2014 Andy Loening
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


#ifndef __AMITK_COLOR_TABLE_MENU__
#define __AMITK_COLOR_TABLE_MENU_H__

/* includes we always need with this widget */
#include <gtk/gtk.h>
#include "amitk_color_table.h"

G_BEGIN_DECLS

#define AMITK_TYPE_COLOR_TABLE_MENU             (amitk_color_table_menu_get_type ())
#define AMITK_COLOR_TABLE_MENU(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), AMITK_TYPE_COLOR_TABLE_MENU, AmitkColorTableMenu))
#define AMITK_COLOR_TABLE_MENU_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_COLOR_TABLE_MENU, AmitkColorTableMenuClass))
#define AMITK_IS_COLOR_TABLE_MENU(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AMITK_TYPE_COLOR_TABLE_MENU))
#define AMITK_IS_COLOR_TABLE_MENU_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_COLOR_TABLE_MENU))
#define AMITK_COLOR_TABLE_MENU_COLOR_TABLE(obj) (AMITK_COLOR_TABLE_MENU(obj)->color_table)

typedef struct _AmitkColorTableMenu             AmitkColorTableMenu;
typedef struct _AmitkColorTableMenuClass        AmitkColorTableMenuClass;


struct _AmitkColorTableMenu {
  GtkComboBox combo_box;
};

struct _AmitkColorTableMenuClass {
  GtkComboBoxClass parent_class;
};  


GType         amitk_color_table_menu_get_type          (void);
GtkWidget*    amitk_color_table_menu_new               ();


G_END_DECLS

#endif /* __AMITK_COLOR_TABLE_MENU_H__ */

