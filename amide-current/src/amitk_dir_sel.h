/* amitk_dir_sel.h - this is gtkfilesel, but slightly modified
   so that it lists .xif files in the file list, and compiles
   cleanly under AMIDE
*/

/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __AMITK_DIRSEL_H__
#define __AMITK_DIRSEL_H__


#include <gnome.h>


G_BEGIN_DECLS

#define AMITK_TYPE_DIR_SELECTION            (amitk_dir_selection_get_type ())
#define AMITK_DIR_SELECTION(obj)            (GTK_CHECK_CAST ((obj), AMITK_TYPE_DIR_SELECTION, AmitkDirSelection))
#define AMITK_DIR_SELECTION_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), AMITK_TYPE_DIR_SELECTION, AmitkDirSelectionClass))
#define AMITK_IS_DIR_SELECTION(obj)         (GTK_CHECK_TYPE ((obj), AMITK_TYPE_DIR_SELECTION))
#define AMITK_IS_DIR_SELECTION_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_DIR_SELECTION))
#define AMITK_DIR_SELECTION_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), AMITK_TYPE_DIR_SELECTION, AmitkDirSelectionClass))


typedef struct _AmitkDirSelection       AmitkDirSelection;
typedef struct _AmitkDirSelectionClass  AmitkDirSelectionClass;

struct _AmitkDirSelection
{
  GtkDialog parent_instance;

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

  GPtrArray *selected_names;
  gchar     *last_selected;
};

struct _AmitkDirSelectionClass
{
  GtkDialogClass parent_class;

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};


GtkType    amitk_dir_selection_get_type            (void) G_GNUC_CONST;
GtkWidget* amitk_dir_selection_new                 (const gchar      *title);
void       amitk_dir_selection_set_filename        (AmitkDirSelection *filesel,
						   const gchar      *filename);
/* This function returns the selected filename in the C runtime's
 * multibyte string encoding, which may or may not be the same as that
 * used by GDK (UTF-8). To convert to UTF-8, call g_filename_to_utf8().
 * The returned string points to a statically allocated buffer and
 * should be copied away.
 */
G_CONST_RETURN gchar* amitk_dir_selection_get_filename        (AmitkDirSelection *filesel);

void	   amitk_dir_selection_complete		  (AmitkDirSelection *filesel,
						   const gchar	    *pattern);
void       amitk_dir_selection_show_fileop_buttons (AmitkDirSelection *filesel);
void       amitk_dir_selection_hide_fileop_buttons (AmitkDirSelection *filesel);

gchar**    amitk_dir_selection_get_selections      (AmitkDirSelection *filesel);

void       amitk_dir_selection_set_select_multiple (AmitkDirSelection *filesel,
						   gboolean          select_multiple);
gboolean   amitk_dir_selection_get_select_multiple (AmitkDirSelection *filesel);

G_END_DECLS

#endif /* __AMITK_DIRSEL_H__ */
