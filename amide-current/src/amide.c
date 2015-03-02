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
#include "study.h"
#include "ui_study.h"

/* external variables */
gchar * view_names[] = {"transverse", "coronal", "sagittal"};
gint number_of_windows = 0;

   

int main (int argc, char *argv []) {

  struct stat file_info;
  study_t * study;
  const gchar ** input_filenames;
  poptContext amide_ctx;
  guint i=0;
  gint studies_launched=0;

  struct poptOption amide_popt_options[] = {
    {"input_study",
     'i',
     POPT_ARG_NONE,
     NULL,
     0,
     N_("Depreciated"),
     N_("DEPRECIATED")
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

  /* uncomment these whenever we get around to using i18n */
  //  bindtextdomain(PACKAGE, GNOMELOCALEDIR);
  //  textdomain(PACKAGE);

  gnome_init_with_popt_table(PACKAGE, VERSION, argc, argv,
			     amide_popt_options, 0, &amide_ctx);

  /* restore the normal segmentation fault signalling so we can get 
     core dumps and don't get the gnome crash dialog */
  signal(SIGSEGV, SIG_DFL);

  gdk_rgb_init(); /* needed for the rgb graphics usage stuff */

  /* figure out if there was anything else on the command line */
  input_filenames = poptGetArgs(amide_ctx);
  /*  input_filenames is just pointers into the amide_ctx structure,
      and shouldn't be freed */
    
  /* if we specified files on the command line, load it in */
  if (input_filenames != NULL) {
    for (i=0; input_filenames[i] != NULL; i++) {
      /* check to see that the filename exists and it's a directory */
      if (stat(input_filenames[i], &file_info) != 0) 
	g_warning("%s: AMIDE study %s does not exist",PACKAGE, input_filenames[i]);
      else if (!S_ISDIR(file_info.st_mode))
	g_warning("%s: %s is not a directory/AMIDE study",PACKAGE, input_filenames[i]);
      else if ((study=study_load_xml(input_filenames[i])) == NULL)
	/* try loading the study into memory */
	g_warning("%s: error loading study: %s",PACKAGE, input_filenames[i]);
      else {
	/* setup the study window */
	ui_study_create(study, NULL);
	studies_launched++;
      }
    }
  } 

  /* start up an empty study if we haven't loaded in anything */
  if (studies_launched < 1)
    ui_study_create(NULL,NULL);

  poptFreeContext(amide_ctx);

  /* the main event loop */
  gtk_main();

  return 0;

}



