/* gtkdirsel.h - this is gtkfilesel modified for picking .xif files (directories) */

/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __GTK_DIRSEL_H__
#define __GTK_DIRSEL_H__


#include <gtk/gtk.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_TYPE_DIR_SELECTION            (gtk_dir_selection_get_type ())
#define GTK_DIR_SELECTION(obj)            (GTK_CHECK_CAST ((obj), GTK_TYPE_DIR_SELECTION, GtkDirSelection))
#define GTK_DIR_SELECTION_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_DIR_SELECTION, GtkDirSelectionClass))
#define GTK_IS_DIR_SELECTION(obj)         (GTK_CHECK_TYPE ((obj), GTK_TYPE_DIR_SELECTION))
#define GTK_IS_DIR_SELECTION_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_DIR_SELECTION))


typedef struct _GtkDirSelection       GtkDirSelection;
typedef struct _GtkDirSelectionClass  GtkDirSelectionClass;

struct _GtkDirSelection
{
  GtkWindow window;

  GtkWidget *dir_list;
  GtkWidget *file_list;
  GtkWidget *selection_entry;
  GtkWidget *selection_text;
  GtkWidget *main_vbox;
  GtkWidget *ok_button;
  GtkWidget *cancel_button;
  GtkWidget *help_button;
  GtkWidget *history_pulldown;
  GtkWidget *history_menu;
  GList     *history_list;
  GtkWidget *fileop_dialog;
  GtkWidget *fileop_entry;
  gchar     *fileop_file;
  gpointer   cmpl_state;
  
  GtkWidget *fileop_c_dir;
  GtkWidget *fileop_del_file;
  GtkWidget *fileop_ren_file;
  
  GtkWidget *button_area;
  GtkWidget *action_area;
  
};

struct _GtkDirSelectionClass
{
  GtkWindowClass parent_class;
};


GtkType    gtk_dir_selection_get_type            (void);
GtkWidget* gtk_dir_selection_new                 (const gchar      *title);
void       gtk_dir_selection_set_filename        (GtkDirSelection *dirsel,
						   const gchar      *filename);
gchar*     gtk_dir_selection_get_filename        (GtkDirSelection *dirsel);
void	   gtk_dir_selection_complete		  (GtkDirSelection *dirsel,
						   const gchar	    *pattern);
void       gtk_dir_selection_show_fileop_buttons (GtkDirSelection *dirsel);
void       gtk_dir_selection_hide_fileop_buttons (GtkDirSelection *dirsel);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_DIRSEL_H__ */










