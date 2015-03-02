/* ui_series_menus.c
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
#include "study.h"
#include "rendering.h"
#include "ui_common.h"
#include "ui_series.h"
#include "ui_series_cb.h"
#include "ui_series_menus.h"


/* function to setup the menus for the series ui */
void ui_series_menus_create(ui_series_t * ui_series) {

  /* defining the menus for the series ui interface */
  GnomeUIInfo file_menu[] = {
    //    GNOMEUIINFO_ITEM_DATA(N_("_Export Series"),
    //			  N_("Export the series view to an image file (JPEG/TIFF/PNG/etc.)"),
    //			  ui_series_cb_export,
    //			  ui_series, NULL),
    //    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_MENU_CLOSE_ITEM(ui_series_cb_close, ui_series),
    GNOMEUIINFO_END
  };

  /* and the main menu definition */
  GnomeUIInfo series_main_menu[] = {
    GNOMEUIINFO_MENU_FILE_TREE(file_menu),
    GNOMEUIINFO_MENU_HELP_TREE(ui_common_help_menu),
    GNOMEUIINFO_END
  };


  /* sanity check */
  g_assert(ui_series!=NULL);


  /* make the menu */
  gnome_app_create_menus(GNOME_APP(ui_series->app), series_main_menu);

  return;

}






