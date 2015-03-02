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
#include <signal.h>
#include <sys/stat.h>
#include <gnome.h>
#include "amide.h"
#include "ui_main.h"
#include "study.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_roi.h"
#include "ui_volume.h" 
#include "ui_study.h"

/* external variables */
gchar * view_names[] = {"transverse", "coronal", "sagittal"};

   

int main (int argc, char *argv []) {

  struct stat file_info;
  study_t * study;
  gchar * input_filename = NULL;

  struct poptOption amide_popt_options[] = {
    {"input_study",
     'i',
     POPT_ARG_STRING,
     &input_filename,
     0,
     N_("Load in this amide .xif study at startup"),
     N_("XIF_STUDY")
    },
    {NULL,
     '\0',
     0,
     NULL,
     0,
     NULL,
     NULL
    }
  };

  bindtextdomain(PACKAGE, GNOMELOCALEDIR);
  textdomain(PACKAGE);

  gnome_init_with_popt_table(PACKAGE, VERSION, argc, argv,
			     amide_popt_options, 0, NULL);

  /* restore the normal segmentation fault signalling so we can get 
     core dumps and don't get the gnome crash dialog */
  signal(SIGSEGV, SIG_DFL);

  gdk_rgb_init(); /* needed for the rgb graphics usage stuff */

  /* initialize the main ui */
  ui_main_create();

  /* if we specified a file on the command line, load it in */
  if (input_filename != NULL) {
    /* check to see that the filename exists and it's a directory */
    if (stat(input_filename, &file_info) != 0)
      g_warning("%s: AMIDE study %s does not exist",PACKAGE, input_filename);
    else if (!S_ISDIR(file_info.st_mode))
      g_warning("%s: %s is not a directory/AMIDE study",PACKAGE, input_filename);
    else if ((study=study_load_xml(input_filename)) == NULL)
      /* try loading the study into memory */
      g_warning("%s: error loading study: %s",PACKAGE, input_filename);
    else
      /* setup the study window */
      ui_study_create(study, NULL);
  }

  /* the main event loop */
  gtk_main();

  return 0;

}



