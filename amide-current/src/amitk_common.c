/* amitk_common.c
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

#include "amide_config.h"
#include "amide.h"
#include "amitk_common.h"
#include <string.h>



/* this function's use is a bit of a cludge 
   GTK typically uses %f for changing a float to text to display in a table
   Here we overwrite the typical conversion with a %g conversion
 */
void amitk_real_cell_data_func(GtkTreeViewColumn *tree_column,
			       GtkCellRenderer *cell,
			       GtkTreeModel *tree_model,
			       GtkTreeIter *iter,
			       gpointer data) {

  gdouble value;
  gchar *text;
  gint column = GPOINTER_TO_INT(data);

  /* Get the double value from the model. */
  gtk_tree_model_get (tree_model, iter, column, &value, -1);

  /* Now we can format the value ourselves. */
  text = g_strdup_printf ("%g", value);
  g_object_set (cell, "text", text, NULL);
  g_free (text);

  return;
}




gint amitk_spin_button_scientific_output (GtkSpinButton *spin_button, gpointer data) {

  gchar *buf = g_strdup_printf ("%g", spin_button->adjustment->value);

  if (strcmp (buf, gtk_entry_get_text (GTK_ENTRY (spin_button))))
    gtk_entry_set_text (GTK_ENTRY (spin_button), buf);
  g_free (buf);

  return TRUE; /* non-zero forces the default output function not to run */
}
