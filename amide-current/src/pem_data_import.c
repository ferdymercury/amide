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
  realpoint_t new_offset;
  realpoint_t new_axis[NUM_AXIS];
  axis_t i_axis;

  /* acquire space for the volume structure */
  if ((pem_volume = volume_init()) == NULL) {
    g_warning("%s: couldn't allocate space for the volume structure to hold PEM data", PACKAGE);
    return pem_volume;
  }
  if ((pem_volume->data_set = data_set_init()) == NULL) {
    g_warning("%s: couldn't allocate space for the data set structure to hold PEM data", PACKAGE);
    return volume_free(pem_volume);
  }



#ifdef AMIDE_DEBUG
  g_print("PEM Data File %s\n",pem_data_filename);
#endif

  /* set the parameters of the volume structure */
  pem_volume->modality = PET;
  pem_volume->data_set->dim.x = PEM_DATA_X;
  pem_volume->data_set->dim.y = PEM_DATA_Y;
  pem_volume->data_set->dim.z = PEM_DATA_Z;
  pem_volume->data_set->dim.t = PEM_DATA_FRAMES;
  pem_volume->voxel_size.x = PEM_VOXEL_SIZE_X;
  pem_volume->voxel_size.y = PEM_VOXEL_SIZE_Y;
  pem_volume->voxel_size.z = PEM_VOXEL_SIZE_Z;
  volume_recalc_far_corner(pem_volume);
  
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
  pem_volume = raw_data_read_volume(pem_data_filename, pem_volume, 
				    PEM_DATA_FORMAT, PEM_FILE_OFFSET);
  /* correct the pem volume based on the model */
  if (pem_model_filename != NULL) {

#ifdef AMIDE_DEBUG
  g_print("PEM Model File %s\n",pem_model_filename);
#endif

    /* acquire space for the volume structure */
    if ((pem_model = volume_init()) == NULL) {
      g_warning("%s: couldn't allocate space for the volume structure to hold the PEM model", PACKAGE);
      return volume_free(pem_volume);
    }
    if ((pem_model->data_set = data_set_init()) == NULL) {
      g_warning("%s: couldn't allocate space for the data set structure to hold PEM model", PACKAGE);
      pem_model = volume_free(pem_model);
      return volume_free(pem_volume);
    }

    /* set the parameters of the volume structure */
    pem_model->modality = PET;
    pem_model->data_set->dim.x = PEM_DATA_X;
    pem_model->data_set->dim.y = PEM_DATA_Y;
    pem_model->data_set->dim.z = PEM_DATA_Z;
    pem_model->data_set->dim.t = PEM_DATA_FRAMES;
    pem_model->data_set->format = FLOAT;
    pem_model->voxel_size.x = PEM_VOXEL_SIZE_X;
    pem_model->voxel_size.y = PEM_VOXEL_SIZE_Y;
    pem_model->voxel_size.z = PEM_VOXEL_SIZE_Z;
    volume_recalc_far_corner(pem_model);
    
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

    pem_model = raw_data_read_volume(pem_model_filename, pem_model, 
				     PEM_DATA_FORMAT, PEM_FILE_OFFSET);

    for (i.t = 0; i.t < pem_volume->data_set->dim.t; i.t++) 
      for (i.z = 0; i.z < pem_volume->data_set->dim.z ; i.z++) 
	for (i.y = 0; i.y < pem_volume->data_set->dim.y; i.y++) 
	  for (i.x = 0; i.x < pem_volume->data_set->dim.x; i.x++) 
	    if (VOLUME_FLOAT_0D_SCALING_CONTENTS(pem_model, i) > 1.0)
	      DATA_SET_FLOAT_SET_CONTENT(pem_volume->data_set,i) 
		= VOLUME_FLOAT_0D_SCALING_CONTENTS(pem_volume, i) 
		/ VOLUME_FLOAT_0D_SCALING_CONTENTS(pem_model, i);
	    else
	      DATA_SET_FLOAT_SET_CONTENT(pem_volume->data_set,i) = 0.0;

    /* free the model, as we no longer need it */
    pem_model = volume_free(pem_model);

    /* and recalculate the max/min of the volume */
    volume_recalc_max_min(pem_volume);

  }

  /* and set the coordinate axis empirically */
  if (pem_volume != NULL) {
    for (i_axis=0;i_axis<NUM_AXIS;i_axis++)
      new_axis[i_axis] = rs_specific_axis(pem_volume->coord_frame, i_axis);
    new_axis[XAXIS].x = -1.0;
    new_axis[XAXIS].y = 0.0;
    new_axis[XAXIS].z = 0.0;
    new_offset = rs_offset(pem_volume->coord_frame);
    new_offset.x = pem_volume->data_set->dim.x*pem_volume->voxel_size.x;
    rs_set_offset(&pem_volume->coord_frame, new_offset);
    rs_set_axis(&pem_volume->coord_frame, new_axis);
  }

  return pem_volume; 
  /* and return the new volume structure (it's NULL if we cancelled) */

}







