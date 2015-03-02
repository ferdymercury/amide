/* pem_data_import.c
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
#include <matrix.h>
#include "volume.h"
#include <sys/stat.h>
#include "raw_data_import.h"
#include "pem_data_import.h"



/* function to load in volume data from a PEM data file */
volume_t * pem_data_import(gchar * pem_data_filename, gchar * pem_model_filename) {

  volume_t * pem_volume;
  volume_t * pem_model;
  gchar * volume_name;
  gchar ** frags;
  voxelpoint_t i;
  guint t;
  volume_data_t max,min,temp;

  /* acquire space for the volume structure */
  if ((pem_volume = volume_init()) == NULL) {
    g_warning("%s: couldn't allocate space for the volume structure to hold PEM data", PACKAGE);
    return pem_volume;
  }



#ifdef AMIDE_DEBUG
  g_print("PEM Data File %s\n",pem_data_filename);
#endif

  /* set the parameters of the volume structure */
  pem_volume->modality = PET;
  pem_volume->conversion = 1.0;
  pem_volume->dim.x = PEM_DATA_X;
  pem_volume->dim.y = PEM_DATA_Y;
  pem_volume->dim.z = PEM_DATA_Z;
  pem_volume->num_frames = PEM_DATA_FRAMES;
  pem_volume->voxel_size.x = PEM_VOXEL_SIZE_X;
  pem_volume->voxel_size.y = PEM_VOXEL_SIZE_Y;
  pem_volume->voxel_size.z = PEM_VOXEL_SIZE_Z;
  REALPOINT_MULT(pem_volume->dim, pem_volume->voxel_size, pem_volume->corner);
  
  /* figure out an initial name for the data */
  volume_name = g_strdup(g_basename(pem_data_filename));
  /* remove the extension of the file */
  g_strreverse(volume_name);
  frags = g_strsplit(volume_name, ".", 2);
  g_strreverse(volume_name);
  volume_name = frags[1];
  g_strreverse(volume_name);
  volume_set_name(pem_volume,volume_name);
  g_strfreev(frags); /* free up now unused strings */
  g_free(volume_name);

  /* now that we've figured out the header info, read in the PEM files */
  pem_volume = raw_data_read_file(pem_data_filename, pem_volume, 
				  PEM_DATA_FORMAT, PEM_FILE_OFFSET);
  /* correct the pem volume based on the model */
  if (pem_model_filename != NULL) {

#ifdef AMIDE_DEBUG
  g_print("PEM Model File %s\n",pem_model_filename);
#endif

    /* acquire space for the volume structure */
    if ((pem_model = volume_init()) == NULL) {
      g_warning("%s: couldn't allocate space for the volume structure to hold the PEM model", PACKAGE);
      pem_volume = volume_free(pem_volume);
      return pem_volume;
    }

    /* set the parameters of the volume structure */
    pem_model->modality = PET;
    pem_model->conversion = 1.0;
    pem_model->dim.x = PEM_DATA_X;
    pem_model->dim.y = PEM_DATA_Y;
    pem_model->dim.z = PEM_DATA_Z;
    pem_model->num_frames = PEM_DATA_FRAMES;
    pem_model->voxel_size.x = PEM_VOXEL_SIZE_X;
    pem_model->voxel_size.y = PEM_VOXEL_SIZE_Y;
    pem_model->voxel_size.z = PEM_VOXEL_SIZE_Z;
    REALPOINT_MULT(pem_model->dim, pem_model->voxel_size, pem_model->corner);
    
    /* figure out an initial name for the model */
    volume_name = g_strdup(g_basename(pem_model_filename));
    /* remove the extension of the file */
    g_strreverse(volume_name);
    frags = g_strsplit(volume_name, ".", 2);
    g_strreverse(volume_name);
    volume_name = frags[1];
    g_strreverse(volume_name);
    volume_set_name(pem_model,volume_name);
    g_strfreev(frags); /* free up now unused strings */
    g_free(volume_name);

    pem_model = raw_data_read_file(pem_model_filename, pem_model, 
				   PEM_DATA_FORMAT, PEM_FILE_OFFSET);

    for (t = 0; t < pem_volume->num_frames; t++) 
      for (i.z = 0; i.z < pem_volume->dim.z ; i.z++) 
	for (i.y = 0; i.y < pem_volume->dim.y; i.y++) 
	  for (i.x = 0; i.x < pem_volume->dim.x; i.x++) 
	    if (VOLUME_CONTENTS(pem_model, t, i) > 1.0)
	      VOLUME_SET_CONTENT(pem_volume,t,i) 
		= VOLUME_CONTENTS(pem_volume, t, i) 
		/ VOLUME_CONTENTS(pem_model, t, i);
	    else
	      VOLUME_SET_CONTENT(pem_volume,t,i) = 0.0;

    /* free the model, as we no longer need it */
    pem_model = volume_free(pem_model);

    /* and recalculate the max/min of the volume */

#ifdef AMIDE_DEBUG
    g_print("\trecalculating max & min");
#endif
    max = 0.0;
    min = 0.0;
    for(t = 0; t < pem_volume->num_frames; t++) {
      for (i.z = 0; i.z < pem_volume->dim.z; i.z++) 
	for (i.y = 0; i.y < pem_volume->dim.y; i.y++) 
	  for (i.x = 0; i.x < pem_volume->dim.x; i.x++) {
	    temp = VOLUME_CONTENTS(pem_volume, t, i);
	    if (temp > max)
	      max = temp;
	    else if (temp < min)
	      min = temp;
	  }
#ifdef AMIDE_DEBUG
      g_print(".");
#endif
  }
    pem_volume->max = max;
    pem_volume->min = min;

#ifdef AMIDE_DEBUG
    g_print("\tnew max %5.3f min %5.3f\n",max,min);
#endif

  }

  /* and set the coordinate axis empirically */
  if (pem_volume != NULL) {
    pem_volume->coord_frame.axis[XAXIS].x = -pem_volume->coord_frame.axis[XAXIS].x;
    pem_volume->coord_frame.axis[YAXIS].y = -pem_volume->coord_frame.axis[YAXIS].y;
  }


  return pem_volume; 
  /* and return the new volume structure (it's NULL if we cancelled) */

}







