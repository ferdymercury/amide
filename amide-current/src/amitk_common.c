/* amitk_common.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2004-2014 Andy Loening
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
//#include <string.h>
//#include <locale.h>
//#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

#include "amitk_type_builtins.h"
#include "amitk_common.h"


/* external variables */
gchar * amitk_limit_names[AMITK_THRESHOLD_STYLE_NUM][AMITK_LIMIT_NUM] = {
  {
    N_("Min"),
    N_("Max")
  },
  { 
    N_("Center"),
    N_("Width")
  }
};

gchar * amitk_window_names[] = {
  //  N_("Bone"),
  //  N_("Soft Tissue")
  N_("Abdomen"),
  N_("Brain"),
  N_("Extremities"),
  N_("Liver"),
  N_("Lung"),
  N_("Pelvis, soft tissue"),
  N_("Skull Base"),
  N_("Spine A"),
  N_("Spine B"),
  N_("Thorax, soft tissue")
};

/* external variables */
PangoFontDescription * amitk_fixed_font_desc;


void amitk_common_font_init(void) {


#if defined (G_PLATFORM_WIN32)  
  amitk_fixed_font_desc = pango_font_description_from_string("Tahoma 10");
#else
  amitk_fixed_font_desc = pango_font_description_from_string("Monospace 9");
#endif
  /* actually, these fonts aren't fixed width... but it's what I've been using */
  //  amitk_fixed_font_desc = pango_font_description_from_string("Sans 9");
  //  amitk_fixed_font_desc = pango_font_description_from_string("-*-helvetica-medium-r-normal-*-*-120-*-*-*-*-*-*");

  return;
}

/* little utility function, appends str to pstr,
   handles case of pstr pointing to NULL */
void amitk_append_str_with_newline(gchar ** pstr, const gchar * format, ...) {

  va_list args;
  gchar * temp_str;
  gchar * error_str;

  if (pstr == NULL) return;

  va_start (args, format);
  error_str = g_strdup_vprintf(format, args);
  va_end (args);

  if (*pstr != NULL) {
    temp_str = g_strdup_printf("%s\n%s", *pstr, error_str);
    g_free(*pstr);
    *pstr = temp_str;
  } else {
    *pstr = g_strdup(error_str);
  }
  g_free(error_str);
}

void amitk_append_str(gchar ** pstr, const gchar * format, ...) {

  va_list args;
  gchar * temp_str;
  gchar * error_str;

  if (pstr == NULL) return;

  va_start (args, format);
  error_str = g_strdup_vprintf(format, args);
  va_end (args);

  if (*pstr != NULL) {
    temp_str = g_strdup_printf("%s%s", *pstr, error_str);
    g_free(*pstr);
    *pstr = temp_str;
  } else {
    *pstr = g_strdup(error_str);
  }
  g_free(error_str);
}




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

/* this function exists, because double or triple clicking on the arrows in a spin button goes into 
   an endless loop if that spin button is in a toolbar, at least in gtk as of version 2.24.22. */
gint amitk_spin_button_discard_double_or_triple_click(GtkWidget *widget, GdkEventButton *event, gpointer func_data) {
  if ((event->type==GDK_2BUTTON_PRESS || event->type==GDK_3BUTTON_PRESS))
    return TRUE;
  else
    return FALSE;
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
    buf.rect.y1 = yoffset+height;
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



gboolean amitk_is_xif_directory(const gchar * filename, gboolean * plegacy1, gchar ** pxml_filename) {

  struct stat file_info;
  gchar * temp_str;
  DIR * directory;
  struct dirent * directory_entry;
  gchar *xifname;
  gint length;

  /* remove any trailing directory characters */
  length = strlen(filename);
  if ((length >= 1) && (strcmp(filename+length-1, G_DIR_SEPARATOR_S) == 0))
    length--;
  xifname = g_strndup(filename, length);


  if (stat(xifname, &file_info) != 0) 
    return FALSE; /* file doesn't exist */

  if (!S_ISDIR(file_info.st_mode)) 
    return FALSE;


  /* check for legacy .xif file (< 2.0 version) */
  temp_str = g_strdup_printf("%s%sStudy.xml", xifname, G_DIR_SEPARATOR_S);
  if (stat(temp_str, &file_info) == 0) {
    if (plegacy1 != NULL)  *plegacy1 = TRUE;
    if (pxml_filename != NULL) *pxml_filename = temp_str;
    else g_free(temp_str);

    return TRUE;
  }
  g_free(temp_str);

  /* figure out the name of the study file */
  directory = opendir(xifname);
      
  /* currently, only looks at the first study_*.xml file... there should be only one anyway */
  if (directory != NULL) {
    while (((directory_entry = readdir(directory)) != NULL))
      if (g_pattern_match_simple("study_*.xml", directory_entry->d_name)) {
	if (plegacy1 != NULL) *plegacy1 = FALSE;
	if (pxml_filename != NULL) *pxml_filename = g_strdup(directory_entry->d_name);
	closedir(directory);
	
	return TRUE;
      }
  }
    
  closedir(directory);

  return FALSE;

}

gboolean amitk_is_xif_flat_file(const gchar * filename, guint64 * plocation_le, guint64 *psize_le) {

  struct stat file_info;
  FILE * study_file;
  guint64 location_le, size_le;
  gchar magic[64];

  if (stat(filename, &file_info) != 0)
    return FALSE; /* file doesn't exist */

  if (S_ISDIR(file_info.st_mode)) return FALSE;

  /* Note, "rb" is same as "r" on Unix, but not in Windows */
  if ((study_file = fopen(filename, "rb")) == NULL) 
    return FALSE;

  /* check magic string */
  fread(magic, sizeof(gchar), 64, study_file);
  if (strncmp(magic, AMITK_FLAT_FILE_MAGIC_STRING, strlen(AMITK_FLAT_FILE_MAGIC_STRING)) != 0) {
    fclose(study_file);
    return FALSE;
  }

  /* get area of file to read for initial XML data */
  fread(&location_le, sizeof(guint64), 1, study_file);
  fread(&size_le, sizeof(guint64), 1, study_file);
  if (plocation_le != NULL)  *plocation_le = location_le;
  if (psize_le != NULL) *psize_le = size_le;

  fclose(study_file);

  return TRUE;
}





const gchar * amitk_layout_get_name(const AmitkLayout layout) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_LAYOUT);
  enum_value = g_enum_get_value(enum_class, layout);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

const gchar * amitk_panel_layout_get_name(const AmitkPanelLayout panel_layout) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_PANEL_LAYOUT);
  enum_value = g_enum_get_value(enum_class, panel_layout);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

const gchar * amitk_limit_get_name(const AmitkLimit limit) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_LIMIT);
  enum_value = g_enum_get_value(enum_class, limit);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

const gchar * amitk_window_get_name (const AmitkWindow window) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_WINDOW);
  enum_value = g_enum_get_value(enum_class, window);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

const gchar * amitk_modality_get_name(const AmitkModality modality) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_MODALITY);
  enum_value = g_enum_get_value(enum_class, modality);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

