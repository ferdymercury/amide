/* ui_rendering_menus.c
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

#ifdef AMIDE_LIBVOLPACK_SUPPORT

#include <gnome.h>
#include "ui_common.h"
#include "ui_rendering_menus.h"
#include "ui_rendering_cb.h"

/* function to setup the menus for the rendering ui */
void ui_rendering_menus_create(ui_rendering_t * ui_rendering) {


  GnomeUIInfo file_menu[] = {
    GNOMEUIINFO_ITEM_DATA(N_("_Export Rendering"),
			  N_("Export the rendered image"),
			  ui_rendering_cb_export,
			  ui_rendering, NULL),
#ifdef AMIDE_MPEG_ENCODE_SUPPORT
    GNOMEUIINFO_ITEM_DATA(N_("_Create Movie"),
			  N_("Create a movie out of a sequence of renderings"),
			  ui_rendering_cb_movie,
			  ui_rendering, NULL),
#endif
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_MENU_CLOSE_ITEM(ui_rendering_cb_close, ui_rendering),
    GNOMEUIINFO_END
  };

  GnomeUIInfo edit_menu[] = {
    GNOMEUIINFO_ITEM_DATA(N_("_Rendering Parameters"),
			  N_("Adjust parameters pertinent to the rendered image"),
			  ui_rendering_cb_parameters,
			  ui_rendering, NULL),
    GNOMEUIINFO_END
  };

  /* and the main menu definition */
  GnomeUIInfo ui_rendering_main_menu[] = {
    GNOMEUIINFO_MENU_FILE_TREE(file_menu),
    GNOMEUIINFO_MENU_EDIT_TREE(edit_menu),
    GNOMEUIINFO_MENU_HELP_TREE(ui_common_help_menu),
    GNOMEUIINFO_END
  };


  /* sanity check */
  g_assert(ui_rendering !=NULL);


  /* make the menu */
  gnome_app_create_menus(GNOME_APP(ui_rendering->app), ui_rendering_main_menu);

  return;

}



#endif



