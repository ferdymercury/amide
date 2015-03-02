/* ui_main.c
 *
 * Part of amide - Amide's a Medical Image Dataset Viewer
 * Copyright (C) 2000 Andy Loening
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
#include "ui_main.h"
#include "ui_main_callbacks.h"
#include "ui_main_menus.h"
#include "../pixmaps/amide.xpm"


/* procedure to initialize the user interface  */
void ui_main_create(void) {


  GnomeApp * app;
  gchar * title;
  GnomePixmap * logo;

  /* create the main window */
  title = g_strdup_printf ("%s %s",PACKAGE, VERSION);
  app = GNOME_APP(gnome_app_new(PACKAGE, title));
  free(title);

  /* setup the main menu */
  ui_main_menus_create(app);

  /* add in a cheasy logo thing */
  logo = GNOME_PIXMAP(gnome_pixmap_new_from_xpm_d(amide_xpm));
  gnome_app_set_contents(app, GTK_WIDGET(logo));


  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(app), "delete_event",
		     GTK_SIGNAL_FUNC(ui_main_callbacks_delete_event),
		     NULL);

  /* and show */
  gtk_widget_show_all(GTK_WIDGET(app));

  return;
}



