/* study.c
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
#include <glib.h>
#include "amide.h"
#include "realspace.h"
#include "volume.h"
#include "roi.h"
#include "study.h"

void study_free(amide_study_t ** pstudy) {

  
  volume_list_free(&((*pstudy)->volumes)); /*  free volumes */
  roi_list_free(&((*pstudy)->rois)); /* free rois */

  g_free((*pstudy)->name);
  g_free(*pstudy);
  *pstudy = NULL;

  return;
}



amide_study_t * study_init(void) {
  
  amide_study_t * study;

  /* alloc space for the data structure for passing ui info */
  if ((study = (amide_study_t *) g_malloc(sizeof(amide_study_t))) == NULL) {
    g_warning("%s: couldn't allocate space for study_t",PACKAGE);
    return NULL;
  }

  study->name = NULL;
  study->filename = NULL;
  study->volumes = NULL;
  study->rois = NULL;

  return study;
}


