/* medcon_import.c
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
#ifdef AMIDE_LIBMDC_SUPPORT
#include "realspace.h"
#include "color_table.h"
#include "volume.h"
#include "medcon_import.h"

#undef PACKAGE /* medcon redefines these.... */
#undef VERSION 
#include "medcon.h"
#undef PACKAGE 
#undef VERSION 
#include "config.h"

#define SIZEOF_SHORT 2
#define SIZEOF_INT 4

volume_t * medcon_import(gchar * filename) {

  FILEINFO medcon_file_info;
  gint error;


  voxelpoint_t i;
  volume_t * temp_volume;
  guint t;
  volume_data_t max,min,temp;
  gchar * volume_name;
  gchar ** frags=NULL;
  gfloat * medcon_buffer;

  
  /* setup some defaults */
  medcon_file_info.map = MDC_MAP_GRAY; /*default color map*/


  /* open the file */
  if ((error = MdcOpenFile(&medcon_file_info, filename)) != MDC_OK) {
    g_warning("%s: can't open file %s with libmdc (medcon)",PACKAGE, filename);
    return NULL;
  }

  /* read the file */
  if ((error = MdcReadFile(&medcon_file_info, 1)) != MDC_OK) {
    g_warning("%s: can't read file %s with libmdc (medcon)",PACKAGE, filename);
    MdcCleanUpFI(&medcon_file_info);
    return NULL;
  }

  /* start acquiring some useful information */
  if ((temp_volume = volume_init()) == NULL) {
    g_warning("%s: couldn't allocate space for the volume structure\n", PACKAGE);
    MdcCleanUpFI(&medcon_file_info);
    return temp_volume;
  }

  /* start figuring out information */
#ifdef AMIDE_DEBUG
  g_print("libmdc reading file %s\n",filename);
  g_print("\tnum dimensions %d\tx_dim %d\ty_dim %d\tz_dim %d\tframes %d\n",
	  medcon_file_info.dim[0],medcon_file_info.dim[1],medcon_file_info.dim[2],
	  medcon_file_info.dim[3],medcon_file_info.dim[4]);
  g_print("\tx size\t%5.3f\ty size\t%5.3f\tz size\t%5.3f\ttime\t%5.3f\n",
	  medcon_file_info.pixdim[1], medcon_file_info.pixdim[2],
	  medcon_file_info.pixdim[3], medcon_file_info.pixdim[4]);
  g_print("\tgates %d\tbeds %d\n",
	  medcon_file_info.dim[5],medcon_file_info.dim[6]);
  g_print("\tbits: %d\ttype: %d\n",medcon_file_info.bits,medcon_file_info.type);
#endif
  temp_volume->dim.x = medcon_file_info.dim[1];
  temp_volume->dim.y = medcon_file_info.dim[2];
  temp_volume->dim.z = medcon_file_info.dim[3];
  temp_volume->num_frames = medcon_file_info.dim[4];
  temp_volume->voxel_size.x = medcon_file_info.pixdim[1];
  temp_volume->voxel_size.y = medcon_file_info.pixdim[2];
  temp_volume->voxel_size.z = medcon_file_info.pixdim[3];


  /* guess the modality */
  if (g_strcasecmp(medcon_file_info.image[0].image_mod,"PT") == 0)
    temp_volume->modality = PET;
  else
    temp_volume->modality = CT;

  /* try figuring out the name */
  if (medcon_file_info.study_name != NULL) {
    volume_name = medcon_file_info.study_name;
    volume_set_name(temp_volume,volume_name);
  } else {/* no original filename? */
    volume_name = g_basename(filename);
    /* remove the extension of the file */
    g_strreverse(volume_name);
    frags = g_strsplit(volume_name, ".", 2);
    volume_name = frags[1];
    volume_set_name(temp_volume,volume_name);
    g_strreverse(volume_name);
    g_strfreev(frags); /* free up now unused strings */
  }

  /* guess the start of the scan is the same as the start of the first frame of data */
  /* note, CTI files specify time as integers in msecs */
  temp_volume->scan_start = medcon_file_info.image[0].frame_start/1000;

#ifdef AMIDE_DEBUG
  g_print("\tscan start time %5.3f\n",temp_volume->scan_start);
#endif


  /* allocate space for the array containing info on the duration of the frames */
  if ((temp_volume->frame_duration = volume_get_frame_duration_mem(temp_volume)) == NULL) {
    g_warning("%s: couldn't allocate space for the frame duration info\n",PACKAGE);
    MdcCleanUpFI(&medcon_file_info);
    temp_volume = volume_free(temp_volume);
    return temp_volume;
  }

  /* malloc the space for the volume */
  if ((temp_volume->data = volume_get_data_mem(temp_volume)) == NULL) {
    g_warning("%s: couldn't allocate space for the volume\n",PACKAGE);
    MdcCleanUpFI(&medcon_file_info);
    temp_volume = volume_free(temp_volume);
    return temp_volume;
  }

  /* and load in the data */
  for (t = 0; t < temp_volume->num_frames; t++) {
#ifdef AMIDE_DEBUG
    g_print("\tloading frame %d",t);
#endif

    /* set the frame duration, note, medcon/libMDC specifies time as float in msecs */
    temp_volume->frame_duration[t] = 
      medcon_file_info.image[t*temp_volume->dim.z].frame_duration/1000;

    /* copy the data into the volume */
    for (i.z = 0 ; i.z < temp_volume->dim.z; i.z++) {

      /* convert the image to a 32 bit float to begin with */
      if ((medcon_buffer = (gfloat *) MdcGetImgFLT32(&medcon_file_info, i.z+t*temp_volume->dim.z)) == NULL) {
	g_warning("%s: medcon couldn't convert to a float... out of memory?\n",PACKAGE);
	temp_volume = volume_free(temp_volume);
	MdcCleanUpFI(&medcon_file_info);
	g_free(medcon_buffer);
	return temp_volume;
      }

      for (i.y = 0; i.y < temp_volume->dim.y; i.y++) 
	for (i.x = 0; i.x < temp_volume->dim.x; i.x++)
	  VOLUME_SET_CONTENT(temp_volume,t,i) =
	    medcon_buffer[(temp_volume->dim.x*i.y+i.x)];
    
      /* done with the temporary float buffer */
      g_free(medcon_buffer);
    }

#ifdef AMIDE_DEBUG
    g_print("\tduration %5.3f\n",temp_volume->frame_duration[t]);
#endif
  }    


  /* and done with the medcon file info structure */
  MdcCleanUpFI(&medcon_file_info);


  /* set the far corner of the volume */
  temp_volume->corner.x = temp_volume->dim.x*temp_volume->voxel_size.x;
  temp_volume->corner.y = temp_volume->dim.y*temp_volume->voxel_size.y;
  temp_volume->corner.z = temp_volume->dim.z*temp_volume->voxel_size.z;
  
  /* set the max/min values in the volume */
#ifdef AMIDE_DEBUG
  g_print("\tcalculating max & min");
#endif
  max = 0.0;
  min = 0.0;
  for(t = 0; t < temp_volume->num_frames; t++) {
    for (i.z = 0; i.z < temp_volume->dim.z; i.z++) 
      for (i.y = 0; i.y < temp_volume->dim.y; i.y++) 
	for (i.x = 0; i.x < temp_volume->dim.x; i.x++) {
	  temp = VOLUME_CONTENTS(temp_volume, t, i);
	  if (temp > max)
	    max = temp;
	  else if (temp < min)
	    min = temp;
	}
#ifdef AMIDE_DEBUG
  g_print(".");
#endif
  }
  temp_volume->max = max;
  temp_volume->min = min;

#ifdef AMIDE_DEBUG
  g_print("\tmax %5.3f min %5.3f\n",max,min);
#endif





  return temp_volume;
}


#endif








