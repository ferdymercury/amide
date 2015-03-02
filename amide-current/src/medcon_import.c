/* medcon_import.c
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

#include <time.h>
#include "amide_config.h"
#ifdef AMIDE_LIBMDC_SUPPORT

#include <gnome.h>
#include "amitk_data_set.h"
#include "medcon_import.h"
#undef PACKAGE /* medcon redefines these.... */
#undef VERSION 
#include <medcon.h>
#undef PACKAGE 
#undef VERSION 

static char * medcon_unknown = "Unknown";

gchar * libmdc_menu_names[] = {
  "(_X)MedCon Guess",
  "_Raw via (X)MedCon",
  "A_SCII via (X)MedCon",
  "_GIF 87a/89a",
  "Acr/_Nema 2.0",
  "IN_W 1.0 (RUG)",
  "_Concorde/microPET",
  "_CTI 6.4 via (X)MedCon",
  "_InterFile 3.3",
  "_Analyze (SPM)",
  "_DICOM 3.0",
};
  
gchar * libmdc_menu_explanations[] = {
  "let (X)MedCon/libmdc guess file type",
  "Import a raw data file through (X)MedCon",
  "Import an ASCII data file through (X)MedCon",
  "Import a file stored as GIF",
  "Import a Acr/Nema 2.0 file",
  "Import a INW 1.0 (RUG) File",
  "Import a file from the Concorde microPET",
  "Import a CTI 6.4 file through (X)MedCon",
  "Import a InterFile 3.3 file"
  "Import an Analyze file"
  "Import a DICOM 3.0 file",
};

gboolean medcon_import_supports(libmdc_import_method_t submethod) {
  
  gboolean return_value;

  switch(submethod) {
  case LIBMDC_RAW: 
  case LIBMDC_ASCII:
    return_value = TRUE;
    break;
  case LIBMDC_GIF:
    return_value = MDC_INCLUDE_GIF;
    break;
  case LIBMDC_ACR:
    return_value = MDC_INCLUDE_ACR;
    break;
  case LIBMDC_INW:
    return_value = MDC_INCLUDE_INW;
    break;
  case LIBMDC_CONC:
    return_value = MDC_INCLUDE_CONC;
    break;
  case LIBMDC_ECAT:
    return_value = MDC_INCLUDE_ECAT;
    break;
  case LIBMDC_INTF:
    return_value = MDC_INCLUDE_INTF;
    break;
  case LIBMDC_ANLZ:
    return_value = MDC_INCLUDE_ANLZ;
    break;
  case LIBMDC_DICM:
    return_value = MDC_INCLUDE_DICM;
    break;
  case LIBMDC_NONE:
  default:
    return_value = TRUE;
    break;
  }

  return return_value;
}

AmitkDataSet * medcon_import(const gchar * filename, 
			     libmdc_import_method_t submethod,
			     gboolean (*update_func)(),
			     gpointer update_data) {

  FILEINFO medcon_file_info;
  gint error;
  struct tm time_structure;
  AmitkVoxel i;
  AmitkDataSet * ds=NULL;
  gchar * name;
  gchar * import_filename;
  gchar ** frags=NULL;
  gboolean found_name=FALSE;
  AmitkVoxel dim;
  AmitkFormat format;
  AmitkScalingType scaling_type;
  div_t x;
  gint divider;
  gint t_times_z;
  gboolean continue_work=TRUE;
  gchar * temp_string;
  
  /* setup some defaults */
  XMDC_MEDCON = MDC_NO;  /* we're not xmedcon */
  MDC_INFO=MDC_NO;       /* don't print stuff */
  MDC_VERBOSE=MDC_NO;    /* and don't print stuff */
  MDC_ANLZ_SPM=MDC_YES; /* if analyze format, assume SPM */
  medcon_file_info.map = MDC_MAP_GRAY; /*default color map*/
  MDC_MAKE_GRAY=MDC_YES;
  MDC_QUANTIFY=MDC_YES; /* want quantified data */
  MDC_NEGATIVE=MDC_YES; /* allow negative values */

  /* figure out the fallback format */
  MDC_FALLBACK_FRMT = MDC_FRMT_NONE;
  if (medcon_import_supports(submethod)) {
    switch(submethod) {
    case LIBMDC_RAW: 
      MDC_FALLBACK_FRMT = MDC_FRMT_RAW;
      break;
    case LIBMDC_ASCII:
      MDC_FALLBACK_FRMT = MDC_FRMT_ASCII;
      break;
    case LIBMDC_GIF:
      MDC_FALLBACK_FRMT = MDC_FRMT_GIF;
      break;
    case LIBMDC_ACR:
      MDC_FALLBACK_FRMT = MDC_FRMT_ACR;
      break;
    case LIBMDC_INW:
      MDC_FALLBACK_FRMT = MDC_FRMT_INW;
      break;
    case LIBMDC_CONC:
      MDC_FALLBACK_FRMT = MDC_FRMT_CONC;
      break;
    case LIBMDC_ECAT:
      MDC_FALLBACK_FRMT = MDC_FRMT_ECAT;
      break;
    case LIBMDC_INTF:
      MDC_FALLBACK_FRMT = MDC_FRMT_INTF;
      break;
    case LIBMDC_ANLZ:
      MDC_FALLBACK_FRMT = MDC_FRMT_ANLZ;
      break;
    case LIBMDC_DICM:
      MDC_FALLBACK_FRMT = MDC_FRMT_DICM;
      break;
    case LIBMDC_NONE:
    default:
      MDC_FALLBACK_FRMT = MDC_FRMT_NONE;
      break;
    }
  }  

  /* open the file */
  import_filename = g_strdup(filename); /* this gets around the facts that filename is type const */
  if ((error = MdcOpenFile(&medcon_file_info, import_filename)) != MDC_OK) {
    g_warning("can't open file %s with libmdc (medcon)",filename);
    g_free(import_filename);
    return NULL;
  }
  g_free(import_filename);
  
  /* read the file */
  if ((error = MdcReadFile(&medcon_file_info, 1)) != MDC_OK) {
    g_warning("can't read file %s with libmdc (medcon)",filename);
    goto error;
  }

  /* start figuring out information */
  dim.x = medcon_file_info.dim[1];
  dim.y = medcon_file_info.dim[2];
  dim.z = medcon_file_info.dim[3];
  dim.t = medcon_file_info.dim[4];

#ifdef AMIDE_DEBUG
  g_print("libmdc reading file %s\n",filename);
  g_print("\tnum dimensions %d\tx_dim %d\ty_dim %d\tz_dim %d\tframes %d\n",
	  medcon_file_info.dim[0],dim.x, dim.y, dim.z, dim.t);
  g_print("\tx size\t%5.3f\ty size\t%5.3f\tz size\t%5.3f\ttime\t%5.3f\n",
	  medcon_file_info.pixdim[1], medcon_file_info.pixdim[2],
	  medcon_file_info.pixdim[3], medcon_file_info.pixdim[4]);
  g_print("\tgates %d\tbeds %d\n",
	  medcon_file_info.dim[5],medcon_file_info.dim[6]);
  g_print("\tbits: %d\ttype: %d\n",medcon_file_info.bits,medcon_file_info.type);
#endif

  /* pick our internal data format */
  switch(medcon_file_info.type) {
    /* note1: which types are supported are determined by what MdcGetImg* functions are available */
    /* note2: floats we have medcon quantify, everything else we try to get the raw data, and store scaling factors */
  case BIT1: /* 1 */
  case BIT8_S: /* 2 */
    g_warning("Importing type %d file through medcon unsupported, trying as unsigned byte",
    	      medcon_file_info.type);
  case BIT8_U: /* 3 */
    format = AMITK_FORMAT_UBYTE;
    MDC_QUANTIFY = MDC_NO; 
    MDC_NORM_OVER_FRAMES = MDC_NO;
    scaling_type = AMITK_SCALING_TYPE_2D;
    break;
  case BIT16_U: /*  5 */
    g_warning("Importing type %d file through medcon unsupported, trying as signed short",
    	      medcon_file_info.type);
  case BIT16_S: /* 4 */
    format = AMITK_FORMAT_SSHORT;
    MDC_QUANTIFY = MDC_NO;
    MDC_NORM_OVER_FRAMES = MDC_NO;
    scaling_type = AMITK_SCALING_TYPE_2D;
    break;
  case BIT32_U: /* 7 */
    g_warning("Importing type %d file through medcon unsupported, trying as signed int",
    	      medcon_file_info.type);
  case BIT32_S: /* 6 */
    format = AMITK_FORMAT_SINT;
    MDC_QUANTIFY = MDC_NO;
    MDC_NORM_OVER_FRAMES = MDC_NO;
    scaling_type = AMITK_SCALING_TYPE_2D;
    break;
  default:
  case BIT64_U: /* 9 */
  case BIT64_S: /* 8 */
  case FLT64: /* 11 */
  case ASCII: /* 12 */
  case VAXFL32: /* 13 */
    g_warning("Importing type %d file through medcon unsupported, trying as float",
    	      medcon_file_info.type);
  case FLT32: /* 10 */
    format = AMITK_FORMAT_FLOAT;
    scaling_type = AMITK_SCALING_TYPE_0D;
    MDC_QUANTIFY = MDC_YES;
    MDC_NORM_OVER_FRAMES = MDC_YES;
    break;
  }

  ds = amitk_data_set_new_with_data(format, dim, scaling_type);
  if (ds == NULL) {
    g_warning("couldn't allocate space for the data set structure to hold medcon data");
    goto error;
  }


  ds->voxel_size.x = medcon_file_info.pixdim[1];
  ds->voxel_size.y = medcon_file_info.pixdim[2];
  ds->voxel_size.z = medcon_file_info.pixdim[3];


  /* guess the modality */
  if (g_strcasecmp(medcon_file_info.image[0].image_mod,"PT") == 0)
    ds->modality = AMITK_MODALITY_PET;
  else
    ds->modality = AMITK_MODALITY_CT;

  /* try figuring out the name, start with the study name */
  if (strlen(medcon_file_info.study_name) > 0) 
    if (g_strcasecmp(medcon_file_info.study_name, medcon_unknown) != 0) {
      amitk_object_set_name(AMITK_OBJECT(ds),medcon_file_info.study_name);
      found_name = TRUE;
    }

  if (!found_name) 
    if (strlen(medcon_file_info.patient_name) > 0)
      if (g_strcasecmp(medcon_file_info.patient_name, medcon_unknown) != 0) {
	amitk_object_set_name(AMITK_OBJECT(ds),medcon_file_info.patient_name);
	found_name = TRUE;
      }

  if (!found_name) {/* no original filename? */
    name = g_strdup(g_basename(filename));
    /* remove the extension of the file */
    g_strreverse(name);
    frags = g_strsplit(name, ".", 2);
    g_free(name);
    g_strreverse(frags[1]);
    amitk_object_set_name(AMITK_OBJECT(ds),frags[1]);
    g_strfreev(frags); /* free up now unused strings */
    found_name=TRUE;
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
    amitk_data_set_set_scan_date(ds, ctime(&current_time)); /* give up */
    g_warning("couldn't figure out time of scan from medcon file, setting to %s\n",
	      AMITK_DATA_SET_SCAN_DATE(ds));
  } else {
    amitk_data_set_set_scan_date(ds, asctime(&time_structure));
  }

  /* guess the start of the scan is the same as the start of the first frame of data */
  /* note, CTI files specify time as integers in msecs */
  ds->scan_start = medcon_file_info.image[0].frame_start/1000.0;

#ifdef AMIDE_DEBUG
  g_print("\tscan start time %5.3f\n",ds->scan_start);
#endif


  /* complain if xmedcon is using an affine transformation, this only checks the first image.... */
  if (!EQUAL_ZERO(medcon_file_info.image[0].rescale_intercept))
    g_warning("XmedCon file has non-zero intercept, which AMIDE is ignoring, quantitation will be off\n");

  if (update_func != NULL) {
    temp_string = g_strdup_printf("Importing File Through (X)MedCon:\n   %s", filename);
    continue_work = (*update_func)(update_data, temp_string, (gdouble) 0.0);
    g_free(temp_string);
  }
  t_times_z = dim.z*dim.t;
  divider = ((t_times_z/AMIDE_UPDATE_DIVIDER) < 1) ? 1 : (t_times_z/AMIDE_UPDATE_DIVIDER);

  /* and load in the data */
  for (i.t = 0; i.t <AMITK_DATA_SET_NUM_FRAMES(ds); i.t++) {
#ifdef AMIDE_DEBUG
    g_print("\tloading frame %d",i.t);
#endif

    /* set the frame duration, note, medcon/libMDC specifies time as float in msecs */
    ds->frame_duration[i.t] = 
      medcon_file_info.image[i.t*ds->raw_data->dim.z].frame_duration/1000.0;

    /* make sure it's not zero */
    if (ds->frame_duration[i.t] < SMALL_TIME) ds->frame_duration[i.t] = SMALL_TIME;

    /* copy the data into the data set */
    for (i.z = 0 ; (i.z < ds->raw_data->dim.z) && (continue_work); i.z++) {

      if (update_func != NULL) {
	x = div(i.t*dim.z+i.z,divider);
	if (x.rem == 0)
	  continue_work = (*update_func)(update_data, NULL, (gdouble) (i.z+i.t*dim.z)/t_times_z);
      }

      switch(ds->raw_data->format) {
      case AMITK_FORMAT_UBYTE:
	{
	  amitk_format_UBYTE_t * medcon_buffer;

	  /* store the scaling factor... I think this is the right scaling factor... */
	  *AMITK_RAW_DATA_DOUBLE_2D_SCALING_POINTER(ds->internal_scaling, i) = 
	    medcon_file_info.image[i.t*ds->raw_data->dim.z + i.z].quant_scale
	    * medcon_file_info.image[i.t*ds->raw_data->dim.z + i.z].rescale_slope;

	  /* convert the image to a 8 bit unsigned int to begin with */
	  if ((medcon_buffer = 
	       (amitk_format_UBYTE_t *) MdcGetImgBIT8_U(&medcon_file_info, 
							i.z+i.t*ds->raw_data->dim.z)) == NULL ){
	    g_warning("medcon couldn't convert to an unsigned byte... out of memory?");
	    g_free(medcon_buffer);
	    goto error;
	  }
	  
	  /* transfer over the medcon buffer, compensate for our origin being bottom left */
	  for (i.y = 0; i.y < ds->raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < ds->raw_data->dim.x; i.x++)
	      AMITK_RAW_DATA_UBYTE_SET_CONTENT(ds->raw_data,i) =
		medcon_buffer[(ds->raw_data->dim.x*(ds->raw_data->dim.y-i.y-1)+i.x)];
	  
	  /* done with the temporary float buffer */
	  g_free(medcon_buffer);
	}
	break;
      case AMITK_FORMAT_SSHORT:
	{
	  amitk_format_SSHORT_t * medcon_buffer;

	  /* store the scaling factor... I think this is the right scaling factor... */
	  *AMITK_RAW_DATA_DOUBLE_2D_SCALING_POINTER(ds->internal_scaling, i) = 
	    medcon_file_info.image[i.t*ds->raw_data->dim.z + i.z].rescale_slope;

	  /* convert the image to a 16 bit signed int to begin with */
	  if ((medcon_buffer = 
	       (amitk_format_SSHORT_t *) MdcGetImgBIT16_S(&medcon_file_info, 
							  i.z+i.t*ds->raw_data->dim.z)) == NULL ){
	    g_warning("medcon couldn't convert to a signed short... out of memory?");
	    g_free(medcon_buffer);
	    goto error;
	  }
	  
	  /* transfer over the medcon buffer, compensate for our origin being bottom left */
	  for (i.y = 0; i.y < ds->raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < ds->raw_data->dim.x; i.x++)
	      AMITK_RAW_DATA_SSHORT_SET_CONTENT(ds->raw_data,i) =
		medcon_buffer[(ds->raw_data->dim.x*(ds->raw_data->dim.y-i.y-1)+i.x)];
	  
	  /* done with the temporary float buffer */
	  g_free(medcon_buffer);
	}
	break;
      case AMITK_FORMAT_SINT:
	{
	  amitk_format_SINT_t * medcon_buffer;

	  /* store the scaling factor... I think this is the right scaling factor... */
	  *AMITK_RAW_DATA_DOUBLE_2D_SCALING_POINTER(ds->internal_scaling, i) = 
	    medcon_file_info.image[i.t*ds->raw_data->dim.z + i.z].rescale_slope;

	  /* convert the image to a 32 bit signed int to begin with */
	  if ((medcon_buffer = 
	       (amitk_format_SINT_t *) MdcGetImgBIT32_S(&medcon_file_info, 
						    i.z+i.t*ds->raw_data->dim.z)) == NULL ){
	    g_warning("medcon couldn't convert to a signed int... out of memory?");
	    g_free(medcon_buffer);
	    goto error;
	  }
	  
	  /* transfer over the medcon buffer, compensate for our origin being bottom left */
	  for (i.y = 0; i.y < ds->raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < ds->raw_data->dim.x; i.x++)
	      AMITK_RAW_DATA_SINT_SET_CONTENT(ds->raw_data,i) =
		medcon_buffer[(ds->raw_data->dim.x*(ds->raw_data->dim.y-i.y-1)+i.x)];
	  
	  /* done with the temporary float buffer */
	  g_free(medcon_buffer);
	}
	break;
      case AMITK_FORMAT_FLOAT: 
	{
	  amitk_format_FLOAT_t * medcon_buffer;

	  /* convert the image to a 32 bit float to begin with */
	  if ((medcon_buffer = 
	       (amitk_format_FLOAT_t *) MdcGetImgFLT32(&medcon_file_info, 
						       i.z+i.t*ds->raw_data->dim.z)) == NULL){
	    g_warning("medcon couldn't convert to a float... out of memory?");
	    g_free(medcon_buffer);
	    goto error;
	  }
	  
	  /* transfer over the medcon buffer, compensate for our origin being bottom left */
	  for (i.y = 0; i.y < ds->raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < ds->raw_data->dim.x; i.x++)
	      AMITK_RAW_DATA_FLOAT_SET_CONTENT(ds->raw_data,i) =
		medcon_buffer[(ds->raw_data->dim.x*(ds->raw_data->dim.y-i.y-1)+i.x)];
	  
	  /* done with the temporary float buffer */
	  g_free(medcon_buffer);
	}
	break;
      default:
	g_warning("unexpected case in %s at line %d", __FILE__, __LINE__);
	goto error;
	break;
      }
    }
    
#ifdef AMIDE_DEBUG
    g_print("\tduration %5.3f\n",ds->frame_duration[i.t]);
#endif
  }    

  /* setup remaining parameters */
  amitk_data_set_set_scale_factor(ds, 1.0); /* set the external scaling factor */
  amitk_data_set_calc_far_corner(ds); /* set the far corner of the volume */
  amitk_data_set_calc_max_min(ds, update_func, update_data);
  amitk_volume_set_center(AMITK_VOLUME(ds), zero_point);
  goto function_end;





 error:
  if (ds != NULL) {
    g_object_unref(ds);
    ds = NULL;
  }




 function_end:

  MdcCleanUpFI(&medcon_file_info);

  if (update_func != NULL) /* remove progress bar */
    (*update_func)(update_data, NULL, (gdouble) 2.0); 

  return ds;
}


#endif








