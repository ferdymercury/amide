/* ui_common_cb.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2002 Andy Loening
 *
 * Author: Andy Loening <loening@ucla.edu>
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


#include "config.h"
#include <gnome.h>

/* function which gets called from a name entry dialog */
void ui_common_cb_entry_name(gchar * entry_string, gpointer data) {
  gchar ** p_roi_name = data;
  *p_roi_name = entry_string;
  return;
}


/* function to close a file selection widget */
void ui_common_cb_file_selection_cancel(GtkWidget* widget, gpointer data) {

  GtkFileSelection * file_selection = data;

  gtk_widget_destroy(GTK_WIDGET(file_selection));

  return;
}


/* function which brings up an about box */
void ui_common_cb_about(GtkWidget * button, gpointer data) {

  GtkWidget *about;

  const gchar *authors[] = {
    "Andy Loening <loening@ucla.edu>",
    NULL
  };

  const gchar *contents_base = \
    _("Amide's a Medical Image Data Examiner\n   \n");

  const gchar * contents_compiled = \
    _("compiled with support for the following libraries:");

#ifdef AMIDE_LIBECAT_SUPPORT
  const gchar *contents_libecat = \
    _("\tlibecat: CTI File library by Merence Sibomona");
#endif

#ifdef AMIDE_LIBGSL_SUPPORT
  const gchar *contents_libgsl = \
    _("\tlibgsl: GNU Scientific Library by the GSL Team");
#endif

#ifdef AMIDE_LIBMDC_SUPPORT
  const gchar *contents_libmdc = \
    _("\tlibmdc: Medical Imaging File library by Erik Nolf");
#endif

#ifdef AMIDE_LIBVOLPACK_SUPPORT
  const gchar *contents_libvolpack = \
    _("\tlibvolpack: Volume Rendering library by Philippe Lacroute");
#endif


  gchar * contents;



  contents = g_strjoin("\n", 
		       contents_base,
#if (AMIDE_LIBECAT_SUPPORT || AMIDE_LIBGSL_SUPPORT || AMIDE_LIBMDC_SUPPORT || AMIDE_LIBVOLPACK_SUPPORT)
		       contents_compiled,
#endif
#ifdef AMIDE_LIBECAT_SUPPORT
		       contents_libecat,
#endif
#ifdef AMIDE_LIBGSL_SUPPORT
		       contents_libgsl,
#endif
#ifdef AMIDE_LIBMDC_SUPPORT
		       contents_libmdc,
#endif
#ifdef AMIDE_LIBVOLPACK_SUPPORT
		       contents_libvolpack,
#endif
		       NULL);


  about = gnome_about_new(PACKAGE, VERSION, 
			  "Copyright (c) 2000-2002 Andy Loening",
			  authors,
			  contents,
			  "amide.png");

  gtk_window_set_modal(GTK_WINDOW(about), FALSE);

  gtk_widget_show(about);

  g_free(contents);

  return;
}























