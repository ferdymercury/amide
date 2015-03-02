/* amitk_common.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2004 Andy Loening
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

/* header files that are always needed with this file */
#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>

void amitk_real_cell_data_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
			       GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data);
gint amitk_spin_button_scientific_output (GtkSpinButton *spin_button, gpointer data);
GdkPixbuf * amitk_get_pixbuf_from_canvas(GnomeCanvas * canvas, gint xoffset, gint yoffset,
					 gint width, gint height);
