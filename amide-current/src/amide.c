/* amide.c
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
#include "amide.h"
#include "ui_main.h"

/* external variables */
gchar * view_names[] = {"transverse", "coronal", "sagittal"};

int main (int argc, char *argv []) {

  bindtextdomain(PACKAGE, GNOMELOCALEDIR);
  textdomain(PACKAGE);

  gnome_init(PACKAGE, VERSION, argc, argv);

  gdk_rgb_init(); /* needed for the rgb graphics usage stuff */

  /* initialize the first ui */
  ui_main_create();


  /* the main event loop */
  gtk_main();

  return 0;

}
