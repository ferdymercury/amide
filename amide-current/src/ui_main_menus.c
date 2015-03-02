/* ui_main_menus.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001 Andy Loening
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
#include "ui_main_callbacks.h"
#include "ui_main_menus.h"



/* defining the menus for the main ui interface */
static GnomeUIInfo file_menu[] = {
  GNOMEUIINFO_MENU_NEW_ITEM(N_("_New Study"), 
			    N_("Create a new study viewer window"),
			    ui_main_callbacks_new_study, 
			    NULL),
  GNOMEUIINFO_MENU_OPEN_ITEM(ui_main_callbacks_open_study, NULL),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_EXIT_ITEM(ui_main_callbacks_delete_event, NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo help_menu[]= {
  GNOMEUIINFO_HELP(PACKAGE),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_ABOUT_ITEM(ui_main_callbacks_about, NULL), 
  GNOMEUIINFO_END
};

static GnomeUIInfo main_menu[] = {
  GNOMEUIINFO_MENU_FILE_TREE(file_menu),
  GNOMEUIINFO_MENU_HELP_TREE(help_menu),
  GNOMEUIINFO_END
};




/* function to setup the menus for the main ui */
void ui_main_menus_create(GnomeApp* app) {
  
  gnome_app_create_menus(app, main_menu);

  return;

}


