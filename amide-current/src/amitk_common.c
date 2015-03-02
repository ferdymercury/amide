/* amitk_common.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2004-2006 Andy Loening
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


/* The following function should return a pixbuf representing the currently shown data 
   on the canvas (within the specified height/width at the given offset).  
   -The code is based on gnome_canvas_paint_rect in gnome-canvas.c */
GdkPixbuf * amitk_get_pixbuf_from_canvas(GnomeCanvas * canvas, gint xoffset, gint yoffset,
					 gint width, gint height) {

  GdkPixbuf * pixbuf;

  if (canvas->aa) {
    GnomeCanvasBuf buf;
    GdkColor *color;
    guchar * px;

    px = g_new (guchar, width*height * 3);

    buf.buf = px;
    buf.buf_rowstride = width * 3;
    buf.rect.x0 = xoffset;
    buf.rect.y0 = yoffset;
    buf.rect.x1 = xoffset+width;
    buf.rect.y1 = yoffset+width;
    color = &GTK_WIDGET(canvas)->style->bg[GTK_STATE_NORMAL];
    buf.bg_color = (((color->red & 0xff00) << 8) | (color->green & 0xff00) | (color->blue >> 8));
    buf.is_bg = 1;
    buf.is_buf = 0;
    
    /* render the background */
    if ((* GNOME_CANVAS_GET_CLASS(canvas)->render_background) != NULL)
      (* GNOME_CANVAS_GET_CLASS(canvas)->render_background) (canvas, &buf);
    
    /* render the rest */
    if (canvas->root->object.flags & GNOME_CANVAS_ITEM_VISIBLE)
      (* GNOME_CANVAS_ITEM_GET_CLASS (canvas->root)->render) (canvas->root, &buf);
    
    if (buf.is_bg) {
      g_warning("No code written to implement case buf.is_bg: %s at %d\n", __FILE__, __LINE__);
      pixbuf = NULL;
    } else {
      pixbuf = gdk_pixbuf_new_from_data(buf.buf, GDK_COLORSPACE_RGB, FALSE, 8, 
					width, height,width*3, NULL, NULL);
    }
  } else {
    GdkPixmap * pixmap;

    pixmap = gdk_pixmap_new (canvas->layout.bin_window, width, height,
			     gtk_widget_get_visual (GTK_WIDGET(canvas))->depth);

    /* draw the background */
    (* GNOME_CANVAS_GET_CLASS(canvas)->draw_background)
      (canvas, pixmap, xoffset, yoffset, width, height);

    /* force a draw onto the pixmap */
    (* GNOME_CANVAS_ITEM_GET_CLASS (canvas->root)->draw) 
      (canvas->root, pixmap,xoffset, yoffset, width, height);

    /* transfer to a pixbuf */
    pixbuf = gdk_pixbuf_get_from_drawable (NULL,GDK_DRAWABLE(pixmap),NULL,0,0,0,0,-1,-1);
    g_object_unref(pixmap); 
  }

  return pixbuf;
}
