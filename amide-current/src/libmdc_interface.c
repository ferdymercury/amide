/* libmdc_interface.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2004 Andy Loening
 *
 * Author: Andy Loening <loening@alum.mit.edu>
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

#include "amitk_data_set.h"
#include "amitk_data_set_DOUBLE_0D_SCALING.h"
#include "libmdc_interface.h"
#include <medcon.h>
#include <string.h>

static char * libmdc_unknown = "Unknown";

libmdc_format_t libmdc_import_to_format[LIBMDC_NUM_IMPORT_METHODS] = {
  LIBMDC_NONE,
  LIBMDC_GIF, 
  LIBMDC_ACR, 
  LIBMDC_CONC, 
  LIBMDC_ECAT6, 
  LIBMDC_ECAT7, 
  LIBMDC_INTF, 
  LIBMDC_ANLZ, 
  LIBMDC_DICM
};

gchar * libmdc_import_menu_names[LIBMDC_NUM_IMPORT_METHODS] = {
  N_("(_X)MedCon Guess"),
  N_("_GIF 87a/89a"),
  N_("Acr/_Nema 2.0"),
  N_("_Concorde/microPET"),
  N_("ECAT _6 via (X)MedCon"),
  N_("ECAT _7 via (X)MedCon"),
  N_("_InterFile 3.3"),
  N_("_Analyze (SPM)"),
  N_("_DICOM 3.0"),
};
  
gchar * libmdc_import_menu_explanations[LIBMDC_NUM_IMPORT_METHODS] = {
  N_("let (X)MedCon/libmdc guess file type"),
  N_("Import a file stored as GIF"),
  N_("Import a Acr/Nema 2.0 file"),
  N_("Import a file from the Concorde microPET"),
  N_("Import a CTI/ECAT 6 file through (X)MedCon"),
  N_("Import a CTI/ECAT 7 file through (X)MedCon"),
  N_("Import a InterFile 3.3 file"),
  N_("Import an Analyze file"),
  N_("Import a DICOM 3.0 file")
};

libmdc_format_t libmdc_export_to_format[LIBMDC_NUM_EXPORT_METHODS] = {
  LIBMDC_ACR, 
  LIBMDC_CONC, 
  LIBMDC_ECAT6, 
  LIBMDC_INTF, 
  LIBMDC_ANLZ, 
  LIBMDC_DICM
};

gchar * libmdc_export_menu_names[LIBMDC_NUM_EXPORT_METHODS] = {
  N_("Acr/_Nema 2.0"),
  N_("_Concorde/microPET"),
  N_("ECAT _6 via (X)MedCon"),
  N_("_InterFile 3.3"),
  N_("_Analyze (SPM)"),
  N_("_DICOM 3.0"),
};
  
gchar * libmdc_export_menu_explanations[LIBMDC_NUM_EXPORT_METHODS] = {
  N_("Export a Acr/Nema 2.0 file"),
  N_("Export a Concorde format file"),
  N_("Export a CTI/ECAT 6 file"),
  N_("Export a InterFile 3.3 file"),
  N_("Export an Analyze file"),
  N_("Export a DICOM 3.0 file")
};

gboolean libmdc_supports(libmdc_format_t format) {
  
  switch(format) {
  case LIBMDC_RAW: 
  case LIBMDC_ASCII:
    return TRUE;
    break;
  case LIBMDC_GIF:
    return MDC_INCLUDE_GIF;
    break;
  case LIBMDC_ACR:
    return MDC_INCLUDE_ACR;
    break;
  case LIBMDC_CONC:
    return MDC_INCLUDE_CONC;
    break;
  case LIBMDC_ECAT6:
  case LIBMDC_ECAT7:
    return MDC_INCLUDE_ECAT;
    break;
  case LIBMDC_INTF:
    return MDC_INCLUDE_INTF;
    break;
  case LIBMDC_ANLZ:
    return MDC_INCLUDE_ANLZ;
    break;
  case LIBMDC_DICM:
    return MDC_INCLUDE_DICM;
    break;
  case LIBMDC_NONE:
  default:
    return TRUE;
    break;
  }
}

static gint libmdc_format_number(libmdc_format_t format) {

  switch(format) {
  case LIBMDC_RAW: 
    return MDC_FRMT_RAW;
    break;
  case LIBMDC_ASCII:
    return MDC_FRMT_ASCII;
    break;
  case LIBMDC_GIF:
    return MDC_FRMT_GIF;
    break;
  case LIBMDC_ACR:
    return MDC_FRMT_ACR;
    break;
  case LIBMDC_CONC:
    return MDC_FRMT_CONC;
    break;
  case LIBMDC_ECAT6:
    return MDC_FRMT_ECAT6;
    break;
  case LIBMDC_ECAT7:
    return MDC_FRMT_ECAT7;
    break;
  case LIBMDC_INTF:
    return MDC_FRMT_INTF;
    break;
  case LIBMDC_ANLZ:
    return MDC_FRMT_ANLZ;
    break;
  case LIBMDC_DICM:
    return MDC_FRMT_DICM;
    break;
  case LIBMDC_NONE: /* let libmdc guess */
  default:
    return MDC_FRMT_NONE;
    break;
  }
}

static gint libmdc_type_number(AmitkFormat format) {

  switch(format) {
  case AMITK_FORMAT_UBYTE:
    return BIT8_U;
    break;
  case AMITK_FORMAT_SBYTE:
    return BIT8_S;
    break;
  case AMITK_FORMAT_USHORT:
    return BIT16_U;
    break;
  case AMITK_FORMAT_SSHORT:
    return BIT16_S;
    break;
  case AMITK_FORMAT_UINT:
    return BIT32_U;
    break;
  case AMITK_FORMAT_SINT: 
    return BIT32_S;
    break;
  case AMITK_FORMAT_FLOAT:
    return FLT32;
    break;
  case AMITK_FORMAT_DOUBLE:
    return FLT64;
    break;
  default:
    g_error("unexpected case in %s at line %d",__FILE__, __LINE__);
    return 0;
    break;
  }
}

AmitkDataSet * libmdc_import(const gchar * filename, 
			     libmdc_format_t libmdc_format,
			     AmitkPreferences * preferences,
			     gboolean (*update_func)(),
			     gpointer update_data) {

  FILEINFO libmdc_fi;
  gboolean libmdc_fi_init=FALSE;
  gint error;
  struct tm time_structure;
  AmitkVoxel i;
  gint j;
  guint64 k;
  AmitkDataSet * ds=NULL;
  gchar * name;
  gchar * import_filename=NULL;
  gchar ** frags=NULL;
  AmitkVoxel dim;
  AmitkFormat format;
  AmitkModality modality;
  div_t x;
  gint divider;
  gint total_planes;
  gboolean continue_work=TRUE;
  gboolean invalid_date;
  gchar * temp_string;
  gint image_num;
  gboolean salvage = FALSE;
  gchar * msg;
  gchar * saved_time_locale;
  gchar * saved_numeric_locale;
  
  saved_time_locale = g_strdup(setlocale(LC_TIME,NULL));
  saved_numeric_locale = g_strdup(setlocale(LC_NUMERIC,NULL));
  setlocale(LC_TIME,"C");  
  setlocale(LC_NUMERIC,"C");  

  
  /* setup some defaults */
  MDC_INFO=MDC_NO;       /* don't print stuff */
  MDC_VERBOSE=MDC_NO;    /* and don't print stuff */
  MDC_ANLZ_SPM=MDC_YES; /* if analyze format, assume SPM */
  libmdc_fi.map = MDC_MAP_GRAY; /*default color map*/
  MDC_MAKE_GRAY=MDC_YES;

  /* these are probably no longer important, as I now don't use MdcReadFile, and MdcPixelFiddle
     doesn't get invoked */
  MDC_QUANTIFY=MDC_YES; /* want quantified data */
  MDC_NEGATIVE=MDC_YES; /* allow negative values */
  MDC_NORM_OVER_FRAMES = MDC_NO;

  /* figure out the fallback format */
  if (libmdc_supports(libmdc_format)) 
    MDC_FALLBACK_FRMT = libmdc_format_number(libmdc_format);
  else
    MDC_FALLBACK_FRMT = MDC_FRMT_NONE;

  /* open the file */
  import_filename = g_strdup(filename); /* this gets around the facts that filename is type const */
  if ((error = MdcOpenFile(&libmdc_fi, import_filename)) != MDC_OK) {
    g_warning(_("Can't open file %s with libmdc/(X)MedCon"),filename);
    goto error;
  }
  libmdc_fi_init=TRUE;

  /* read the file */
  if ((error = MdcLoadFile(&libmdc_fi)) != MDC_OK) {
    g_warning(_("Can't read file %s with libmdc/(X)MedCon"),filename);
    goto error;
  }

  /* make sure libmdc returned all dim's as >0 */
  for (j=0; j<MDC_MAX_DIMS; j++) 
    if (libmdc_fi.dim[j] <= 0) 
      libmdc_fi.dim[j] = 1;

  /* start figuring out information */
  dim.x = libmdc_fi.image[0].width;
  dim.y = libmdc_fi.image[0].height;
  dim.z = libmdc_fi.dim[3];
  dim.t = libmdc_fi.dim[4];
  dim.g = libmdc_fi.dim[5];

#ifdef AMIDE_DEBUG
  g_print("libmdc reading file %s\n",filename);
  g_print("\tnum dimensions %d\tx_dim %d\ty_dim %d\tz_dim %d\tgates%d\tframes %d\n",
	  libmdc_fi.dim[0],dim.x, dim.y, dim.z, dim.g, dim.t);
  g_print("\tx size\t%5.3f\ty size\t%5.3f\tz size\t%5.3f\ttime\t%5.3f\n",
	  libmdc_fi.pixdim[1], libmdc_fi.pixdim[2],
	  libmdc_fi.pixdim[3], libmdc_fi.pixdim[4]);
  g_print("\tgates %d\tbeds %d\n",
	  libmdc_fi.dim[5],libmdc_fi.dim[6]);
  g_print("\tbits: %d\ttype: %d\n",libmdc_fi.bits,libmdc_fi.type);
#endif


  /* pick our internal data format */
  switch(libmdc_fi.type) {
  case BIT8_S: /* 2 */
    format = AMITK_FORMAT_SBYTE;
    break;
  case BIT8_U: /* 3 */
    format = AMITK_FORMAT_UBYTE;
    break;
  case BIT16_U: /*  5 */
    format = AMITK_FORMAT_USHORT;
    break;
  case BIT16_S: /* 4 */
    format = AMITK_FORMAT_SSHORT;
    break;
  case BIT32_U: /* 7 */
    format = AMITK_FORMAT_UINT;
    break;
  case BIT32_S: /* 6 */
    format = AMITK_FORMAT_SINT;
    break;
  case FLT32: /* 10 */
    format = AMITK_FORMAT_FLOAT;
    break;
  default:
  case BIT64_U: /* 9 */
  case BIT64_S: /* 8 */
  case FLT64: /* 11 */
  case ASCII: /* 12 */
  case VAXFL32: /* 13 */
  case BIT1: /* 1 */
    g_warning(_("Importing data type %d file through (X)MedCon unsupported in AMIDE, trying anyway"),
    	      libmdc_fi.type);
    format = AMITK_FORMAT_FLOAT;
    salvage = TRUE;
    break;
  }


  if (salvage) {
    /* need to load in all the raw data at once, this is because we'll be using MdcGetImgFLT32,
       which requires having called MdcImagePixelFiddle, which requires all the data */
    for (image_num=0; image_num<libmdc_fi.number; image_num++) 
      if (libmdc_fi.image[image_num].buf == NULL) 
	if ((error = MdcLoadPlane(&libmdc_fi, image_num)) != MDC_OK) {
	  g_warning(_("Couldn't read plane %d in %s with libmdc/(X)MedCon"),image_num, filename);
	  goto error;
	}
    
    msg = MdcImagesPixelFiddle(&libmdc_fi);
    if (msg != NULL) {
      g_warning(_("libmdc returned error: %s"), msg);
      goto error;
    }
  }

  /* guess the modality */
  switch (libmdc_fi.modality) {
  case M_PT:
    modality = AMITK_MODALITY_PET;
    break;
  case M_ST:
  case M_NM:
    modality = AMITK_MODALITY_SPECT;
    break;
  case M_MA:
  case M_MR:
  case M_MS:
    modality = AMITK_MODALITY_MRI;
    break;
  case M_CR:
  case M_CT:
  case M_DF:
  case M_DS:
  case M_DX:
  case M_MG:
  case M_PX:
  case M_RF:
  case M_RG:
  case M_XA:
    modality = AMITK_MODALITY_CT;
    break;
  case M_OT:
  default:
    modality = AMITK_MODALITY_OTHER;
    break;
  }

  ds = amitk_data_set_new_with_data(preferences, modality, format, dim, AMITK_SCALING_TYPE_2D);
  if (ds == NULL) {
    g_warning(_("Couldn't allocate space for the data set structure to hold (X)MedCon data"));
    goto error;
  }


  ds->voxel_size.x = libmdc_fi.pixdim[1];
  ds->voxel_size.y = libmdc_fi.pixdim[2];
  ds->voxel_size.z = libmdc_fi.pixdim[3];
  if (EQUAL_ZERO(ds->voxel_size.x)) {
    g_warning(_("Voxel size X was read as 0, setting to 1 mm.  This may be an internationalization error."));
    ds->voxel_size.x = 1.0;
  }
  if (EQUAL_ZERO(ds->voxel_size.y)) {
    g_warning(_("Voxel size Y was read as 0, setting to 1 mm.  This may be an internationalization error."));
    ds->voxel_size.y = 1.0;
  }
  if (EQUAL_ZERO(ds->voxel_size.z)) {
    g_warning(_("Voxel size Z was read as 0, setting to 1 mm.  This may be an internationalization error."));
    ds->voxel_size.z = 1.0;
  }
  

  /* try figuring out the name, start with the study name */
  name = NULL;
  if (strlen(libmdc_fi.study_id) > 0) 
    if (g_ascii_strcasecmp(libmdc_fi.study_id, libmdc_unknown) != 0)
      name = g_strdup(libmdc_fi.study_id);

  if (name == NULL)
    if (strlen(libmdc_fi.patient_name) > 0)
      if (g_ascii_strcasecmp(libmdc_fi.patient_name, libmdc_unknown) != 0) 
	name = g_strdup(libmdc_fi.patient_name);

  if (name == NULL) {/* no original filename? */
    temp_string = g_path_get_basename(filename);
    /* remove the extension of the file */
    g_strreverse(temp_string);
    frags = g_strsplit(temp_string, ".", 2);
    g_free(temp_string);
    g_strreverse(frags[1]);
    name = g_strdup(frags[1]);
    g_strfreev(frags); /* free up now unused strings */
  }

  /* append the reconstruction method */
  if (strlen(libmdc_fi.recon_method) > 0)
    if (g_ascii_strcasecmp(libmdc_fi.recon_method, libmdc_unknown) != 0) {
      temp_string = name;
      name = g_strdup_printf("%s - %s", temp_string, libmdc_fi.recon_method);
      g_free(temp_string);
    }

  amitk_object_set_name(AMITK_OBJECT(ds),name);
  g_free(name);
  

  /* enter in the date the scan was performed */
  invalid_date = FALSE;

  time_structure.tm_sec = libmdc_fi.study_time_second;
  time_structure.tm_min = libmdc_fi.study_time_minute;
  time_structure.tm_hour = libmdc_fi.study_time_hour;
  time_structure.tm_mday = libmdc_fi.study_date_day;
  if (libmdc_fi.study_date_month == 0) {
    invalid_date = TRUE;
    time_structure.tm_mon=0;
  } else
    time_structure.tm_mon = libmdc_fi.study_date_month-1;
  if (libmdc_fi.study_date_year == 0) {
    invalid_date = TRUE;
    time_structure.tm_year = 0;
  } else
    time_structure.tm_year = libmdc_fi.study_date_year-1900;
  time_structure.tm_isdst = -1; /* "-1" is suppose to let the system figure it out, was "daylight"; */

  if ((mktime(&time_structure) == -1) && invalid_date) { /* do any corrections needed on the time */
    amitk_data_set_set_scan_date(ds, "Unknown"); /* give up */
  } else {
    amitk_data_set_set_scan_date(ds, asctime(&time_structure));
  }

  /* guess the start of the scan is the same as the start of the first frame of data */
  /* note, libmdc specifies time as integers in msecs */
  /* note, libmdc only uses dyndata if there's more than one frame of data,
     if there's only 1 frame, dyndata is NULL! */
  if (libmdc_fi.dyndata != NULL)
    ds->scan_start = libmdc_fi.dyndata[0].time_frame_start/1000.0;
  else {
    g_warning(_("(X)MedCon returned no dynamic data information.  Frame durations will be incorrect"));
    ds->scan_start = 0.0;
  }

#ifdef AMIDE_DEBUG
  g_print("\tscan start time %5.3f\n",ds->scan_start);
#endif

  /* complain if xmedcon is using an affine transformation, this only checks the first image.... */
  if (!EQUAL_ZERO(libmdc_fi.image[0].rescale_intercept))
    g_warning(_("(X)MedCon file has non-zero intercept, which AMIDE is ignoring, quantitation will be off"));

  if (update_func != NULL) {
    temp_string = g_strdup_printf(_("Importing File Through (X)MedCon:\n   %s"), filename);
    continue_work = (*update_func)(update_data, temp_string, (gdouble) 0.0);
    g_free(temp_string);
  }
  total_planes = dim.z*dim.g*dim.t;
  divider = ((total_planes/AMIDE_UPDATE_DIVIDER) < 1) ? 1 : (total_planes/AMIDE_UPDATE_DIVIDER);

  /* and load in the data */
  for (i.t = 0; (i.t < dim.t) && (continue_work); i.t++) {
#ifdef AMIDE_DEBUG
    g_print("\tloading frame %d",i.t);
#endif

    /* set the frame duration, note, medcon/libMDC specifies time as float in msecs */
    if (libmdc_fi.dyndata != NULL)
      amitk_data_set_set_frame_duration(ds, i.t, libmdc_fi.dyndata[i.t].time_frame_duration/1000.0);
    else
      amitk_data_set_set_frame_duration(ds, i.t, 1.0);

    /* make sure it's not zero */
    if (amitk_data_set_get_frame_duration(ds,i.t) < EPSILON) 
      amitk_data_set_set_frame_duration(ds,i.t, EPSILON);

    for (i.g = 0; (i.g < ds->raw_data->dim.g) && (continue_work); i.g++) {

      /* copy the data into the data set */
      for (i.z = 0 ; (i.z < ds->raw_data->dim.z) && (continue_work); i.z++) {

	/* note, libmdc is gates->frames->planes, we're frames->gates->planes */
	image_num = i.z+i.t*dim.z+i.g*dim.z*dim.t;

	if (update_func != NULL) {
	  x = div(image_num,divider);
	  if (x.rem == 0)
	    continue_work = (*update_func)(update_data, NULL, ((gdouble) image_num)/((gdouble) total_planes));
	}

	/* read in the raw plane data if needed */
	if (libmdc_fi.image[image_num].buf == NULL) {
	  if ((error = MdcLoadPlane(&libmdc_fi, image_num)) != MDC_OK) {
	    g_warning(_("Couldn't read plane %d in %s with libmdc/(X)MedCon"),image_num, filename);
	    goto error;
	  }
	}

	/* store the scaling factor... I think this is the right scaling factor... */
	if (salvage)
	  *AMITK_RAW_DATA_DOUBLE_2D_SCALING_POINTER(ds->internal_scaling, i) = 1.0;
	else
	  *AMITK_RAW_DATA_DOUBLE_2D_SCALING_POINTER(ds->internal_scaling, i) = 
	    libmdc_fi.image[image_num].quant_scale*
	    libmdc_fi.image[image_num].calibr_fctr;

	switch(ds->raw_data->format) {
	case AMITK_FORMAT_SBYTE:
	  {
	    amitk_format_SBYTE_t * libmdc_buffer;
	    libmdc_buffer = (amitk_format_SBYTE_t *) (libmdc_fi.image[image_num].buf);
	    
	    /* transfer over the medcon buffer, compensate for our origin being bottom left */
	    for (i.y = 0; i.y < ds->raw_data->dim.y; i.y++) 
	      for (i.x = 0; i.x < ds->raw_data->dim.x; i.x++)
		AMITK_RAW_DATA_SBYTE_SET_CONTENT(ds->raw_data,i) =
		  libmdc_buffer[(ds->raw_data->dim.x*(ds->raw_data->dim.y-i.y-1)+i.x)];
	  }
	  break;
	case AMITK_FORMAT_UBYTE:
	  {
	    amitk_format_UBYTE_t * libmdc_buffer;
	    libmdc_buffer = (amitk_format_UBYTE_t *) (libmdc_fi.image[image_num].buf);
	    
	    /* transfer over the medcon buffer, compensate for our origin being bottom left */
	    for (i.y = 0; i.y < ds->raw_data->dim.y; i.y++) 
	      for (i.x = 0; i.x < ds->raw_data->dim.x; i.x++) 
		AMITK_RAW_DATA_UBYTE_SET_CONTENT(ds->raw_data,i) =
		  libmdc_buffer[(ds->raw_data->dim.x*(ds->raw_data->dim.y-i.y-1)+i.x)];
	  }
	  break;
	case AMITK_FORMAT_SSHORT:
	  {
	    amitk_format_SSHORT_t * libmdc_buffer;
	    if (MdcDoSwap()) {
	      k=0;
	      for (i.y = 0; i.y < ds->raw_data->dim.y; i.y++) 
		for (i.x = 0; i.x < ds->raw_data->dim.x; i.x++,k+=2)
		  MdcSwapBytes(libmdc_fi.image[image_num].buf+k, 2);
	    }
	    libmdc_buffer = (amitk_format_SSHORT_t *) (libmdc_fi.image[image_num].buf);
	    
	    /* transfer over the medcon buffer, compensate for our origin being bottom left */
	    for (i.y = 0; i.y < ds->raw_data->dim.y; i.y++) 
	      for (i.x = 0; i.x < ds->raw_data->dim.x; i.x++)
		AMITK_RAW_DATA_SSHORT_SET_CONTENT(ds->raw_data,i) =
		  libmdc_buffer[(ds->raw_data->dim.x*(ds->raw_data->dim.y-i.y-1)+i.x)];
	  }
	  break;
	case AMITK_FORMAT_USHORT:
	  {
	    amitk_format_USHORT_t * libmdc_buffer;
	    if (MdcDoSwap()) {
	      k=0;
	      for (i.y = 0; i.y < ds->raw_data->dim.y; i.y++) 
		for (i.x = 0; i.x < ds->raw_data->dim.x; i.x++,k+=2)
		  MdcSwapBytes(libmdc_fi.image[image_num].buf+k, 2);
	    }
	    libmdc_buffer = (amitk_format_USHORT_t *) (libmdc_fi.image[image_num].buf);
	    
	    /* transfer over the medcon buffer, compensate for our origin being bottom left */
	    for (i.y = 0; i.y < ds->raw_data->dim.y; i.y++) 
	      for (i.x = 0; i.x < ds->raw_data->dim.x; i.x++)
		AMITK_RAW_DATA_USHORT_SET_CONTENT(ds->raw_data,i) =
		  libmdc_buffer[(ds->raw_data->dim.x*(ds->raw_data->dim.y-i.y-1)+i.x)];
	  }
	  break;
	case AMITK_FORMAT_SINT:
	  {
	    amitk_format_SINT_t * libmdc_buffer;
	    if (MdcDoSwap()) {
	      k=0;
	      for (i.y = 0; i.y < ds->raw_data->dim.y; i.y++) 
		for (i.x = 0; i.x < ds->raw_data->dim.x; i.x++,k+=4)
		  MdcSwapBytes(libmdc_fi.image[image_num].buf+k, 4);
	    }
	    libmdc_buffer = (amitk_format_SINT_t *) (libmdc_fi.image[image_num].buf);
	    
	    /* transfer over the medcon buffer, compensate for our origin being bottom left */
	    for (i.y = 0; i.y < ds->raw_data->dim.y; i.y++) 
	      for (i.x = 0; i.x < ds->raw_data->dim.x; i.x++)
		AMITK_RAW_DATA_SINT_SET_CONTENT(ds->raw_data,i) =
		  libmdc_buffer[(ds->raw_data->dim.x*(ds->raw_data->dim.y-i.y-1)+i.x)];
	  }
	  break;
	case AMITK_FORMAT_UINT:
	  {
	    amitk_format_UINT_t * libmdc_buffer;
	    if (MdcDoSwap()) {
	      k=0;
	      for (i.y = 0; i.y < ds->raw_data->dim.y; i.y++) 
		for (i.x = 0; i.x < ds->raw_data->dim.x; i.x++,k+=4)
		  MdcSwapBytes(libmdc_fi.image[image_num].buf+k, 4);
	    }
	    libmdc_buffer = (amitk_format_UINT_t *) (libmdc_fi.image[image_num].buf);
	    
	    /* transfer over the medcon buffer, compensate for our origin being bottom left */
	    for (i.y = 0; i.y < ds->raw_data->dim.y; i.y++) 
	      for (i.x = 0; i.x < ds->raw_data->dim.x; i.x++)
		AMITK_RAW_DATA_UINT_SET_CONTENT(ds->raw_data,i) =
		  libmdc_buffer[(ds->raw_data->dim.x*(ds->raw_data->dim.y-i.y-1)+i.x)];
	  }
	  break;
	case AMITK_FORMAT_FLOAT: 
	  {
	    amitk_format_FLOAT_t * libmdc_buffer;
	    if (MdcDoSwap()) {
	      k=0;
	      for (i.y = 0; i.y < ds->raw_data->dim.y; i.y++) 
		for (i.x = 0; i.x < ds->raw_data->dim.x; i.x++,k+=4)
		  MdcSwapBytes(libmdc_fi.image[image_num].buf+k, 4);
	    }

	    if (salvage) {
	      /* convert the image to a 32 bit float to begin with */
	      if ((libmdc_buffer = (amitk_format_FLOAT_t *) MdcGetImgFLT32(&libmdc_fi, image_num)) == NULL){
		g_warning(_("(X)MedCon couldn't convert to a float... out of memory?"));
		g_free(libmdc_buffer);
		goto error;
	      }
	    } else {
	      libmdc_buffer = (amitk_format_FLOAT_t *) (libmdc_fi.image[image_num].buf);
	    }
	    
	    /* transfer over the medcon buffer, compensate for our origin being bottom left */
	    for (i.y = 0; i.y < ds->raw_data->dim.y; i.y++) 
	      for (i.x = 0; i.x < ds->raw_data->dim.x; i.x++)
		AMITK_RAW_DATA_FLOAT_SET_CONTENT(ds->raw_data,i) =
		  libmdc_buffer[(ds->raw_data->dim.x*(ds->raw_data->dim.y-i.y-1)+i.x)];

	    /* done with the temporary float buffer */
	    if (salvage) g_free(libmdc_buffer);
	  }
	  break;
	default:
	  g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
	  goto error;
	  break;
	}

	/* free up the buffer data */
	MdcFree(libmdc_fi.image[image_num].buf);
      } /* i.z */
    }
      
#ifdef AMIDE_DEBUG
    g_print("\tduration %5.3f\n", amitk_data_set_get_frame_duration(ds, i.t));
#endif
  }    

  /* setup remaining parameters */
  amitk_data_set_set_injected_dose(ds, libmdc_fi.injected_dose); /* should be in MBq */
  amitk_data_set_set_displayed_dose_unit(ds, AMITK_DOSE_UNIT_MEGABECQUEREL);
  amitk_data_set_set_subject_weight(ds, libmdc_fi.patient_weight); /* should be in Kg */
  amitk_data_set_set_displayed_weight_unit(ds, AMITK_WEIGHT_UNIT_KILOGRAM);
  amitk_data_set_set_scale_factor(ds, 1.0); /* set the external scaling factor */
  amitk_data_set_calc_far_corner(ds); /* set the far corner of the volume */
  amitk_data_set_calc_max_min(ds, update_func, update_data);
  amitk_volume_set_center(AMITK_VOLUME(ds), zero_point);
  goto function_end;





 error:
  if (ds != NULL) 
    ds = amitk_object_unref(ds);


 function_end:

  if (libmdc_fi_init)
    MdcCleanUpFI(&libmdc_fi);

  if (import_filename != NULL)
    g_free(import_filename);

  if (update_func != NULL) /* remove progress bar */
    (*update_func)(update_data, NULL, (gdouble) 2.0); 

  setlocale(LC_NUMERIC, saved_time_locale);
  setlocale(LC_NUMERIC, saved_numeric_locale);
  g_free(saved_time_locale);
  g_free(saved_numeric_locale);
  return ds;
}




void libmdc_export(AmitkDataSet * ds,
		   const gchar * filename, 
		   libmdc_format_t libmdc_format,
		   gboolean resliced,
		   gboolean (*update_func)(),
		   gpointer update_data) {



  FILEINFO fi;
  gboolean fi_init=FALSE;
  IMG_DATA * plane;
  AmitkVoxel i, j;
  AmitkVoxel dim;
  div_t x;
  gint divider;
  gint total_planes;
  gboolean continue_work=TRUE;
  gint image_num;
  gint libmdc_format_num;
  gint bytes_per_row;
  gint bytes_per_plane;
  gchar * err_str; /* note, err_str (if used) will point to a const string in libmdc  */
  gint err_num;
  void * data_ptr;
  gchar * temp_string;
  amide_real_t min_voxel_size=1.0;
  amide_time_t frame_start, frame_duration;
  AmitkPoint corner;
  AmitkCorners corners;
  AmitkVolume * output_volume=NULL;
  AmitkDataSet * slice = NULL;
  AmitkPoint new_offset;
  gfloat * row_data;
  gchar * saved_time_locale;
  gchar * saved_numeric_locale;
  
  saved_time_locale = g_strdup(setlocale(LC_TIME,NULL));
  saved_numeric_locale = g_strdup(setlocale(LC_NUMERIC,NULL));
  setlocale(LC_TIME,"C");  
  setlocale(LC_NUMERIC,"C");  

  
  /* setup some defaults */
  MDC_INFO=MDC_NO;       /* don't print stuff */
  MDC_VERBOSE=MDC_NO;    /* and don't print stuff */
  MDC_ANLZ_SPM=MDC_YES; /* if analyze format, assume SPM */
  fi.map = MDC_MAP_GRAY; /*default color map*/
  MDC_MAKE_GRAY=MDC_YES;
  MDC_QUANTIFY=MDC_YES; /* want quantified data */
  MDC_NEGATIVE=MDC_YES; /* allow negative values */
  MDC_PREFIX_DISABLED=MDC_YES; /* don't add on the m000- stuff */
  MDC_NORM_OVER_FRAMES=MDC_NO;
  MDC_FILE_ENDIAN=MDC_HOST_ENDIAN;
  MDC_FILE_OVERWRITE=MDC_YES;

  /* figure out the fallback format */
  if (libmdc_supports(libmdc_format)) {
    libmdc_format_num = libmdc_format_number(libmdc_format);
  } else {
    g_warning(_("Unsupported export file format: %d\n"), libmdc_format);
    goto cleanup;
  }

  /* initialize the fi info structure */
  MdcInitFI(&fi, "");
  fi_init=TRUE;


  /* set what we can */
  fi.ifname = strdup(filename);
  fi.iformat = MDC_FRMT_RAW;
  if (resliced) {
    fi.type = libmdc_type_number(AMITK_FORMAT_FLOAT);
  } else {
    fi.type = libmdc_type_number(AMITK_DATA_SET_FORMAT(ds));
  }
  fi.bits = MdcType2Bits(fi.type);
  fi.endian = MDC_HOST_ENDIAN;

  dim = AMITK_DATA_SET_DIM(ds);
  if (resliced) {
    output_volume = amitk_volume_new();
    min_voxel_size = point_min_dim(AMITK_DATA_SET_VOXEL_SIZE(ds));
    amitk_volume_get_enclosing_corners(AMITK_VOLUME(ds), AMITK_SPACE(output_volume), corners);
    corner = point_diff(corners[0], corners[1]);
    dim.x = ceil(corner.x/min_voxel_size);
    dim.y = ceil(corner.y/min_voxel_size);
    dim.z = ceil(corner.z/min_voxel_size);
    corner.z = min_voxel_size;
    amitk_space_set_offset(AMITK_SPACE(output_volume), 
			   amitk_space_s2b(AMITK_SPACE(output_volume), corners[0]));
    amitk_volume_set_corner(output_volume, corner);
#ifdef AMIDE_DEBUG
    g_print("output dimensions %d %d %d, voxel size %f\n", dim.x, dim.y, dim.z, min_voxel_size);
#else
    g_warning("dimensions of output data set will be %dx%dx%d, voxel size of %f", dim.x, dim.y, dim.z, min_voxel_size);
#endif
  }

  fi.dim[0]=6;
  fi.dim[1]=dim.x;
  fi.dim[2]=dim.y;
  fi.dim[3]=dim.z; 
  fi.dim[4]=dim.t; 
  fi.dim[5]=dim.g;
  fi.dim[6]=1; /* no beds */
  fi.number = fi.dim[6]*fi.dim[5]*fi.dim[4]*fi.dim[3]; /* total # planes */

  fi.pixdim[0]=3;
  if (resliced) {
    fi.pixdim[1] = fi.pixdim[2] = fi.pixdim[3] = min_voxel_size;
  } else {
    fi.pixdim[1]=AMITK_DATA_SET_VOXEL_SIZE_X(ds);
    fi.pixdim[2]=AMITK_DATA_SET_VOXEL_SIZE_Y(ds);
    fi.pixdim[3]=AMITK_DATA_SET_VOXEL_SIZE_Z(ds);
  }

  switch (AMITK_DATA_SET_MODALITY(ds)) {
  case AMITK_MODALITY_PET:
    fi.modality = M_PT;
    break;
  case AMITK_MODALITY_SPECT:
    fi.modality = M_ST;
    break;
  case AMITK_MODALITY_CT:
    fi.modality = M_CT;
    break;
  case AMITK_MODALITY_MRI:
    fi.modality = M_MR;
    break;
  case AMITK_MODALITY_OTHER:
  default:
    fi.modality = M_OT;
    break;
  }
	
  if (dim.t > 1)
    fi.acquisition_type = MDC_ACQUISITION_DYNAMIC;
  else
    fi.acquisition_type = MDC_ACQUISITION_TOMO;

  if (resliced) {
    bytes_per_row = dim.x*amitk_raw_format_sizes[AMITK_FORMAT_FLOAT];
  } else {
    bytes_per_row = dim.x*amitk_raw_format_sizes[AMITK_DATA_SET_FORMAT(ds)];
  }
  bytes_per_plane  = dim.y*bytes_per_row;

  /* fill in dynamic data struct */
  if (!MdcGetStructDD(&fi,dim.t)) {
    g_warning("couldn't malloc DYNAMIC_DATA structs");
    goto cleanup;
  }


  if (!MdcGetStructID(&fi,fi.number)) {
    g_warning("couldn't malloc img_data structs");
    goto cleanup;
  }


  if (update_func != NULL) {
    temp_string = g_strdup_printf(_("Exporting File Through (X)MedCon:\n   %s"), filename);
    continue_work = (*update_func)(update_data, temp_string, (gdouble) 0.0);
    g_free(temp_string);
  }
  total_planes = dim.z*dim.g*dim.t;
  divider = ((total_planes/AMIDE_UPDATE_DIVIDER) < 1) ? 1 : (total_planes/AMIDE_UPDATE_DIVIDER);


  image_num=0;
  data_ptr = ds->raw_data->data;
  j = zero_voxel;
  for (i.t = 0; (i.t < dim.t) && (continue_work); i.t++) {

    frame_start = amitk_data_set_get_start_time(ds, i.t);
    frame_duration = amitk_data_set_get_frame_duration(ds, i.t);

    fi.dyndata[i.t].nr_of_slices = fi.dim[3];
    /* medcon's in ms */
    fi.dyndata[i.t].time_frame_start = 1000.0*frame_start;
    fi.dyndata[i.t].time_frame_duration = 1000.0*frame_duration;

    for (i.g = 0 ; (i.g < dim.g) && (continue_work); i.g++) {

      for (i.z = 0 ; (i.z < dim.z) && (continue_work); i.z++) {

	/* note, libmdc is gates->frames->planes, we're frames->gates->planes */
	image_num = i.z+i.t*dim.z+i.g*dim.z*dim.t;

	if (update_func != NULL) {
	  x = div(image_num,divider);
	  if (x.rem == 0)
	    continue_work = (*update_func)(update_data, NULL, ((gdouble) image_num)/((gdouble) total_planes));
	}
	
	plane = &(fi.image[image_num]);
	plane->width = fi.dim[1];
	plane->height = fi.dim[2];
	plane->bits = fi.bits;
	plane->type = fi.type;
	plane->pixel_xsize = fi.pixdim[1];
	plane->pixel_ysize = fi.pixdim[2];
	plane->slice_width = fi.pixdim[3];
	if (resliced)
	  plane->quant_scale = 1.0;
	else
	  plane->quant_scale = 
	    AMITK_DATA_SET_SCALE_FACTOR(ds)*
	    amitk_data_set_get_internal_scaling(ds, i);
	plane->calibr_fctr = 1.0;
	
	if ((plane->buf = MdcGetImgBuffer(bytes_per_plane)) == NULL) {
	  g_warning("couldn't alloc %d bytes for plane\n", bytes_per_plane);
	  goto cleanup;
	}
	
	if (resliced) {
	  slice = amitk_data_set_get_slice(ds, frame_start, frame_duration, i.g,
					   min_voxel_size, output_volume);

	  if ((AMITK_DATA_SET_DIM_X(slice) != dim.x) || (AMITK_DATA_SET_DIM_Y(slice) != dim.y)) {
	    g_warning(_("Error in generating resliced data, %dx%d != %dx%d"),
		      AMITK_DATA_SET_DIM_X(slice), AMITK_DATA_SET_DIM_Y(slice),
		      dim.x, dim.y);
	    goto cleanup;
	  }

	  /* advance for next iteration */
	  new_offset = AMITK_SPACE_OFFSET(output_volume);
	  new_offset.z += min_voxel_size;
	  amitk_space_set_offset(AMITK_SPACE(output_volume), new_offset);
	}

	/* flip for (X)MedCon's axis */
	for (i.y=0, j.y=0; i.y < dim.y; i.y++, j.y++) {
	  if (resliced) {
	    row_data = (gfloat *) (plane->buf+bytes_per_row*(dim.y-i.y-1));
	    for (j.x = 0; j.x < dim.x; j.x++) 
	      row_data[j.x] = AMITK_DATA_SET_DOUBLE_0D_SCALING_CONTENT(slice, j);
	  } else {
	    memcpy(plane->buf+bytes_per_row*(dim.y-i.y-1), data_ptr, bytes_per_row);
	    data_ptr += bytes_per_row;
	  }
	}

	if (slice != NULL) slice = amitk_object_unref(slice);
      } /* i.z */
    }
  }

  /* make sure everything's kosher */
  err_str = MdcImagesPixelFiddle(&fi);
  if (err_str != NULL) {
    g_warning("couldn't pixel fiddle, error: %s\n", err_str);
    goto cleanup;
  }

  /* and writeout the file */
  err_num = MdcWriteFile(&fi, libmdc_format_num, 0, NULL);
  if (err_num != MDC_OK) {
    g_warning("couldn't write out file %s, error %d\n", fi.ofname, err_num);
    goto cleanup;
  }



 cleanup:

  if (fi_init)
    MdcCleanUpFI(&fi); /* clean up FILEINFO struct */

  if (output_volume != NULL)
    output_volume = amitk_object_unref(output_volume);

  if (slice != NULL)
    slice = amitk_object_unref(slice);

  if (update_func != NULL) /* remove progress bar */
    (*update_func)(update_data, NULL, (gdouble) 2.0); 

  setlocale(LC_NUMERIC, saved_time_locale);
  setlocale(LC_NUMERIC, saved_numeric_locale);
  g_free(saved_time_locale);
  g_free(saved_numeric_locale);
  return;

}

#endif /* AMIDE_LIBMDC_SUPPORT */





