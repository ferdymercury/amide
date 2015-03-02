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

#include <time.h>
#include "config.h"
#include <gnome.h>
#ifdef AMIDE_LIBMDC_SUPPORT
#include "volume.h"
#include "medcon_import.h"

#undef PACKAGE /* medcon redefines these.... */
#undef VERSION 
#include "medcon.h"
#undef PACKAGE 
#undef VERSION 
#include "config.h"

volume_t * medcon_import(gchar * filename) {

  FILEINFO medcon_file_info;
  gint error;
  struct tm time_structure;
  voxelpoint_t i;
  volume_t * temp_volume;
  gchar * volume_name;
  gchar ** frags=NULL;
  
  /* setup some defaults */
  XMDC_MEDCON = MDC_NO;  /* we're not xmedcon */
  MDC_INFO=MDC_NO;       /* don't print stuff */
  MDC_VERBOSE=MDC_NO;    /* and don't print stuff */
  MDC_ANLZ_SPM=MDC_YES; /* if analyze format, assume SPM */
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
    g_warning("%s: couldn't allocate space for the volume structure to hold medcon data", PACKAGE);
    MdcCleanUpFI(&medcon_file_info);
    return temp_volume;
  }
  if ((temp_volume->data_set = data_set_init()) == NULL) {
    g_warning("%s: couldn't allocate space for the data set structure to hold medcon data", PACKAGE);
    MdcCleanUpFI(&medcon_file_info);
    return volume_free(temp_volume);
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
  temp_volume->data_set->dim.x = medcon_file_info.dim[1];
  temp_volume->data_set->dim.y = medcon_file_info.dim[2];
  temp_volume->data_set->dim.z = medcon_file_info.dim[3];
  temp_volume->data_set->dim.t = medcon_file_info.dim[4];

  /* pick our internal data format */
  switch(medcon_file_info.type) {
    /* note1: which types are supported are determined by what MdcGetImg* functions are available */
    /* note2: floats we have medcon quantify, everything else we try to get the raw data, and store scaling factors */
  case BIT1: /* 1 */
  case BIT8_S: /* 2 */
    g_warning("%s: Importing type %d file through medcon unsupported, trying as unsigned byte",
    	      PACKAGE, medcon_file_info.type);
  case BIT8_U: /* 3 */
    temp_volume->data_set->format = UBYTE;
    MDC_QUANTIFY = MDC_NO; 
    MDC_CALIBRATE = MDC_NO;
    MDC_NORM_OVER_FRAMES = MDC_NO;
    break;
  case BIT16_U: /*  5 */
    g_warning("%s: Importing type %d file through medcon unsupported, trying as signed short",
    	      PACKAGE, medcon_file_info.type);
  case BIT16_S: /* 4 */
    temp_volume->data_set->format = SSHORT;
    MDC_QUANTIFY = MDC_NO;
    MDC_CALIBRATE = MDC_NO;
    MDC_NORM_OVER_FRAMES = MDC_NO;
    break;
  case BIT32_U: /* 7 */
    g_warning("%s: Importing type %d file through medcon unsupported, trying as signed int",
    	      PACKAGE, medcon_file_info.type);
  case BIT32_S: /* 6 */
    temp_volume->data_set->format = SINT;
    MDC_QUANTIFY = MDC_NO;
    MDC_CALIBRATE = MDC_NO;
    MDC_NORM_OVER_FRAMES = MDC_NO;
    break;
  default:
  case BIT64_U: /* 9 */
  case BIT64_S: /* 8 */
  case FLT64: /* 11 */
  case ASCII: /* 12 */
  case VAXFL32: /* 13 */
    g_warning("%s: Importing type %d file through medcon unsupported, trying as float",
    	      PACKAGE, medcon_file_info.type);
  case FLT32: /* 10 */
    temp_volume->data_set->format = FLOAT;
    MDC_QUANTIFY = MDC_YES;
    MDC_CALIBRATE = MDC_YES;
    MDC_NORM_OVER_FRAMES = MDC_YES;
    break;
  }

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
    volume_name = g_strdup(g_basename(filename));
    /* remove the extension of the file */
    g_strreverse(volume_name);
    frags = g_strsplit(volume_name, ".", 2);
    volume_name = frags[1];
    volume_set_name(temp_volume,volume_name);
    g_strreverse(volume_name);
    g_strfreev(frags); /* free up now unused strings */
    g_free(volume_name);
  }

  /* enter in the date the scan was performed */
  time_structure.tm_sec = medcon_file_info.study_time_second;
  time_structure.tm_min = medcon_file_info.study_time_minute;
  time_structure.tm_hour = medcon_file_info.study_time_hour;
  time_structure.tm_mday = medcon_file_info.study_date_day;
  time_structure.tm_mon = medcon_file_info.study_date_month;
  time_structure.tm_year = medcon_file_info.study_date_year-1900;
  time_structure.tm_isdst = -1; /* "-1" is suppose to let the system figure it out, was "daylight"; */
  if (mktime(&time_structure) == -1) { /* do any corrections needed on the time */
    time_t current_time;
    time(&current_time);
    volume_set_scan_date(temp_volume, ctime(&current_time)); /* give up */
    g_strdelimit(temp_volume->scan_date, "\n", ' '); /* turns newlines to white space */
    g_strstrip(temp_volume->scan_date); /* removes trailing and leading white space */
    g_warning("%s: couldn't figure out time of scan from medcon file, setting to %s\n",
	      PACKAGE, temp_volume->scan_date);
  } else {
    volume_set_scan_date(temp_volume, asctime(&time_structure));
    g_strdelimit(temp_volume->scan_date, "\n", ' '); /* turns newlines to white space */
    g_strstrip(temp_volume->scan_date); /* removes trailing and leading white space */
  }

  /* guess the start of the scan is the same as the start of the first frame of data */
  /* note, CTI files specify time as integers in msecs */
  temp_volume->scan_start = medcon_file_info.image[0].frame_start/1000;

#ifdef AMIDE_DEBUG
  g_print("\tscan start time %5.3f\n",temp_volume->scan_start);
#endif


  /* allocate space for the array containing info on the duration of the frames */
  if ((temp_volume->frame_duration = volume_get_frame_duration_mem(temp_volume)) == NULL) {
    g_warning("%s: couldn't allocate space for the frame duration info",PACKAGE);
    MdcCleanUpFI(&medcon_file_info);
    return volume_free(temp_volume);
  }

  /* malloc the space for the volume */
  if ((temp_volume->data_set->data = data_set_get_data_mem(temp_volume->data_set)) == NULL) {
    g_warning("%s: couldn't allocate space for the volume",PACKAGE);
    MdcCleanUpFI(&medcon_file_info);
    return volume_free(temp_volume);
  }

  /* setup the internal scaling factor array for integer types */
  if (temp_volume->data_set->format != FLOAT) {
    temp_volume->internal_scaling->dim.t = temp_volume->data_set->dim.t;
    temp_volume->internal_scaling->dim.z = temp_volume->data_set->dim.z;
    /* malloc the space for the scaling factors */
    g_free(temp_volume->internal_scaling->data); /* get rid of any old scaling factors */
    if ((temp_volume->internal_scaling->data = data_set_get_data_mem(temp_volume->internal_scaling)) == NULL) {
      g_warning("%s: couldn't allocate space for the scaling factors for the (X)medcon data",PACKAGE);
      MdcCleanUpFI(&medcon_file_info);
      return volume_free(temp_volume);
    }
  }

  /* and load in the data */
  for (i.t = 0; i.t < temp_volume->data_set->dim.t; i.t++) {
#ifdef AMIDE_DEBUG
    g_print("\tloading frame %d",i.t);
#endif

    /* set the frame duration, note, medcon/libMDC specifies time as float in msecs */
    temp_volume->frame_duration[i.t] = 
      medcon_file_info.image[i.t*temp_volume->data_set->dim.z].frame_duration/1000;

    /* copy the data into the volume */
    for (i.z = 0 ; i.z < temp_volume->data_set->dim.z; i.z++) {

      switch(temp_volume->data_set->format) {
      case UBYTE:
	{
	  data_set_UBYTE_t * medcon_buffer;

	  /* store the scaling factor... I think this is the right scaling factor... */
	  *DATA_SET_FLOAT_2D_SCALING_POINTER(temp_volume->internal_scaling, i) = 
	    medcon_file_info.image[i.t*temp_volume->data_set->dim.z + i.z].quant_scale
	    * medcon_file_info.image[i.t*temp_volume->data_set->dim.z + i.z].calibr_fctr;

	  /* convert the image to a 8 bit unsigned int to begin with */
	  if ((medcon_buffer = 
	       (data_set_UBYTE_t *) MdcGetImgBIT8_U(&medcon_file_info, 
						    i.z+i.t*temp_volume->data_set->dim.z)) == NULL ){
	    g_warning("%s: medcon couldn't convert to an unsigned byte... out of memory?",PACKAGE);
	    MdcCleanUpFI(&medcon_file_info);
	    g_free(medcon_buffer);
	    return volume_free(temp_volume);
	  }
	  
	  /* transfer over the medcon buffer, compensate for our origin being bottom left */
	  for (i.y = 0; i.y < temp_volume->data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < temp_volume->data_set->dim.x; i.x++)
	      DATA_SET_UBYTE_SET_CONTENT(temp_volume->data_set,i) =
		medcon_buffer[(temp_volume->data_set->dim.x*(temp_volume->data_set->dim.y-i.y-1)+i.x)];
	  
	  /* done with the temporary float buffer */
	  g_free(medcon_buffer);
	}
	break;
      case SSHORT:
	{
	  data_set_SSHORT_t * medcon_buffer;

	  /* store the scaling factor... I think this is the right scaling factor... */
	  *DATA_SET_FLOAT_2D_SCALING_POINTER(temp_volume->internal_scaling, i) = 
	    medcon_file_info.image[i.t*temp_volume->data_set->dim.z + i.z].quant_scale
	    *medcon_file_info.image[i.t*temp_volume->data_set->dim.z + i.z].calibr_fctr;

	  /* convert the image to a 16 bit signed int to begin with */
	  if ((medcon_buffer = 
	       (data_set_SSHORT_t *) MdcGetImgBIT16_S(&medcon_file_info, 
						      i.z+i.t*temp_volume->data_set->dim.z)) == NULL ){
	    g_warning("%s: medcon couldn't convert to a signed short... out of memory?",PACKAGE);
	    MdcCleanUpFI(&medcon_file_info);
	    g_free(medcon_buffer);
	    return volume_free(temp_volume);
	  }
	  
	  /* transfer over the medcon buffer, compensate for our origin being bottom left */
	  for (i.y = 0; i.y < temp_volume->data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < temp_volume->data_set->dim.x; i.x++)
	      DATA_SET_SSHORT_SET_CONTENT(temp_volume->data_set,i) =
		medcon_buffer[(temp_volume->data_set->dim.x*(temp_volume->data_set->dim.y-i.y-1)+i.x)];
	  
	  /* done with the temporary float buffer */
	  g_free(medcon_buffer);
	}
	break;
      case SINT:
	{
	  data_set_SINT_t * medcon_buffer;

	  /* store the scaling factor... I think this is the right scaling factor... */
	  *DATA_SET_FLOAT_2D_SCALING_POINTER(temp_volume->internal_scaling, i) = 
	    medcon_file_info.image[i.t*temp_volume->data_set->dim.z + i.z].quant_scale
	    *medcon_file_info.image[i.t*temp_volume->data_set->dim.z + i.z].calibr_fctr;

	  /* convert the image to a 32 bit signed int to begin with */
	  if ((medcon_buffer = 
	       (data_set_SINT_t *) MdcGetImgBIT32_S(&medcon_file_info, 
						    i.z+i.t*temp_volume->data_set->dim.z)) == NULL ){
	    g_warning("%s: medcon couldn't convert to a signed int... out of memory?",PACKAGE);
	    MdcCleanUpFI(&medcon_file_info);
	    g_free(medcon_buffer);
	    return volume_free(temp_volume);
	  }
	  
	  /* transfer over the medcon buffer, compensate for our origin being bottom left */
	  for (i.y = 0; i.y < temp_volume->data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < temp_volume->data_set->dim.x; i.x++)
	      DATA_SET_SINT_SET_CONTENT(temp_volume->data_set,i) =
		medcon_buffer[(temp_volume->data_set->dim.x*(temp_volume->data_set->dim.y-i.y-1)+i.x)];
	  
	  /* done with the temporary float buffer */
	  g_free(medcon_buffer);
	}
	break;
      case FLOAT: 
	{
	  data_set_FLOAT_t * medcon_buffer;

	  /* convert the image to a 32 bit float to begin with */
	  if ((medcon_buffer = 
	       (data_set_FLOAT_t *) MdcGetImgFLT32(&medcon_file_info, 
						   i.z+i.t*temp_volume->data_set->dim.z)) == NULL){
	    g_warning("%s: medcon couldn't convert to a float... out of memory?",PACKAGE);
	    MdcCleanUpFI(&medcon_file_info);
	    g_free(medcon_buffer);
	    return volume_free(temp_volume);
	  }
	  
	  /* transfer over the medcon buffer, compensate for our origin being bottom left */
	  for (i.y = 0; i.y < temp_volume->data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < temp_volume->data_set->dim.x; i.x++)
	      DATA_SET_FLOAT_SET_CONTENT(temp_volume->data_set,i) =
		medcon_buffer[(temp_volume->data_set->dim.x*(temp_volume->data_set->dim.y-i.y-1)+i.x)];
	  
	  /* done with the temporary float buffer */
	  g_free(medcon_buffer);
	}
	break;
      default:
	g_warning("PACKAGE: unexpected case in __FILE__ at line __LINE__");
	MdcCleanUpFI(&medcon_file_info);
	return volume_free(temp_volume);
	break;
      }
    }
    
#ifdef AMIDE_DEBUG
    g_print("\tduration %5.3f\n",temp_volume->frame_duration[i.t]);
#endif
  }    


  /* and done with the medcon file info structure */
  MdcCleanUpFI(&medcon_file_info);


  /* setup remaining volume parameters */
  volume_set_scaling(temp_volume, 1.0); /* set the external scaling factor */
  volume_recalc_far_corner(temp_volume); /* set the far corner of the volume */
  volume_recalc_max_min(temp_volume); /* set the max/min values in the volume */

  return temp_volume;
}


#endif








