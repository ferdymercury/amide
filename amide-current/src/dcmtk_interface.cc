/* dcmtk_interface.cc
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2005-2014 Andy Loening
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

#include "amide_config.h"
#ifdef AMIDE_LIBDCMDATA_SUPPORT
#ifdef HAVE_STRPTIME
#define __USE_XOPEN_EXTENDED
#endif
#ifdef OLD_WIN32_HACKS
#include <unistd.h>
#endif
#include "dcmtk_interface.h" 
#include <dirent.h>
#include <sys/stat.h>
#include "amitk_data_set_DOUBLE_0D_SCALING.h"
#include <glib/gstdio.h> /* make sure we get g_mkdir on mingw32 */

/* dcmtk redefines a lot of things that they shouldn't... */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#define HAVE_CONFIG_H /* the autoconf compiled version of DCMTK has a config header file */
#include <dcmtk/dcmdata/dcddirif.h>     /* for class DicomDirInterface */
#include <dcmtk/dcmdata/dctk.h>

const gchar * dcmtk_version = OFFIS_DCMTK_VERSION;

/* based on dcmftest.cc - part of dcmtk */
gboolean dcmtk_test_dicom(const gchar * filename) {

  FILE * file;
  gchar signature[4];
  gboolean is_dicom;

  g_return_val_if_fail(filename != NULL, FALSE);

  if ((file = fopen(filename, "rb")) == NULL)
    return FALSE;

  if ((fseek(file, DCM_PreambleLen, SEEK_SET) < 0) || 
      (fread(signature, 1, DCM_MagicLen, file) != DCM_MagicLen)) {
    is_dicom = FALSE;
  } else if (strncmp(signature, DCM_Magic, DCM_MagicLen) != 0) {
    is_dicom = FALSE;
  } else {
    is_dicom = TRUE;
  }
  fclose(file);

  return is_dicom;
}



static AmitkDataSet * read_dicom_file(const gchar * filename,
				      gchar ** pstudyname,
				      AmitkPreferences * preferences,
				      gint *pnum_frames,
				      gint *pnum_gates,
				      gint *pnum_slices,
				      AmitkUpdateFunc update_func,
				      gpointer update_data,
				      gchar **perror_buf) {

  DcmFileFormat dcm_format;
  //  DcmMetaInfo * dcm_metainfo;
  DcmDataset * dcm_dataset;
  OFCondition result;
  Uint16 return_uint16=0;
  Sint32 return_sint32=0;
  Sint16 return_sint16=0;
  Float64 return_float64=0.0;
  Uint16 bits_allocated;
  Uint16 pixel_representation;
  const char * return_str=NULL;
  const char * scan_date=NULL;
  const char * scan_time=NULL;
  gchar * temp_str;
  gboolean valid;

  AmitkPoint voxel_size = one_point;
  AmitkDataSet * ds=NULL;
  AmitkModality modality;
  AmitkVoxel dim;
  AmitkVoxel i;
  AmitkFormat format;
  gboolean continue_work=TRUE;
  gboolean found_value;
  const void * buffer=NULL;
  void * ds_pointer;
  /*  long unsigned num_bytes; */
  gint format_size;
  AmitkPoint new_offset;
  AmitkAxes new_axes;
  AmitkPoint direction;
  amide_time_t acquisition_start_time=-1.0;
  amide_time_t dose_time=-1.0;
  amide_time_t decay_time=-1.0;
  gdouble decay_value;
  gdouble half_life=-1.0;
  gint hours, minutes, seconds;
  struct tm time_structure;

  /* note - dcmtk always uses POSIX locale - look to setlocale stuff in libmdc_interface.c if this ever comes up*/

  result = dcm_format.loadFile(filename);
  if (result.bad()) {
    g_warning("could not read DICOM file %s, dcmtk returned %s",filename, result.text());
    goto error;
  }

  //  dcm_metainfo = dcm_format.getMetaInfo();
  //  if (dcm_metainfo == NULL) {
  //    g_warning("could not find metainfo in DICOM file %s\n", filename);
  //  }

  dcm_dataset = dcm_format.getDataset();
  //  dcm_dataset = &(dcm_dir.getDataset());
  if (dcm_dataset == NULL) {
    g_warning(_("could not find dataset in DICOM file %s\n"), filename);
    goto error;
  }

  modality = AMITK_MODALITY_OTHER;
  if (dcm_dataset->findAndGetString(DCM_Modality, return_str).good()) {
    if (return_str != NULL) {
      
      /* first, process modalities we know we don't read */
      if ((g_ascii_strcasecmp(return_str, "PR") == 0)) {
	amitk_append_str_with_newline(perror_buf, _("Modality %s is not understood.  Ignoring File %s"), return_str, filename);
	goto error;
      }
      
      /* now to modalities we think we understand */
      if ((g_ascii_strcasecmp(return_str, "CR") == 0) ||
	  (g_ascii_strcasecmp(return_str, "CT") == 0) ||
	  (g_ascii_strcasecmp(return_str, "DF") == 0) ||
	  (g_ascii_strcasecmp(return_str, "DS") == 0) ||
	  (g_ascii_strcasecmp(return_str, "DX") == 0) ||
	  (g_ascii_strcasecmp(return_str, "MG") == 0) ||
	  (g_ascii_strcasecmp(return_str, "PX") == 0) ||
	  (g_ascii_strcasecmp(return_str, "RF") == 0) ||
	  (g_ascii_strcasecmp(return_str, "RG") == 0) ||
	  (g_ascii_strcasecmp(return_str, "XA") == 0) ||
	  (g_ascii_strcasecmp(return_str, "XA") == 0))
	modality = AMITK_MODALITY_CT;
      else if ((g_ascii_strcasecmp(return_str, "MA") == 0) ||
	       (g_ascii_strcasecmp(return_str, "MR") == 0) ||
	       (g_ascii_strcasecmp(return_str, "MS") == 0)) 
	modality = AMITK_MODALITY_MRI;
      else if ((g_ascii_strcasecmp(return_str, "ST") == 0) ||
	       (g_ascii_strcasecmp(return_str, "NM") == 0))
	modality = AMITK_MODALITY_SPECT;
      else if ((g_ascii_strcasecmp(return_str, "PT") == 0))
	modality = AMITK_MODALITY_PET;
    }
  }


  /* get basic data */
  if (dcm_dataset->findAndGetUint16(DCM_Columns, return_uint16).bad()) {
    g_warning(_("could not find # of columns - Failed to load file %s\n"), filename);
    goto error;
  }
  dim.x = return_uint16;

  if (dcm_dataset->findAndGetUint16(DCM_Rows, return_uint16).bad()) {
    g_warning(_("could not find # of rows - Failed to load file %s\n"), filename);
    goto error;
  }
  dim.y = return_uint16;

  /* number of slices */
  /* note, number of "Frames" is the number of frames of data within the single DICOM file that do not have
     repeated header information, it does not refer necessarily to the number of time frames as is the 
     convention for nuclear medicine.
     The DICOM tag NumberOfSlices is the number of slices in the study, not the number in the particular DICOM 
     file, so it's not what we want to be using.
  */
  if (dcm_dataset->findAndGetString(DCM_NumberOfFrames, return_str, OFTrue).bad()) {
    /* note, I've never actually seen a DICOM file that uses the DCM_Planes tag, it's now retired */
    if (dcm_dataset->findAndGetUint16(DCM_RETIRED_Planes, return_uint16).bad()) 
      dim.z = 1;
    else
      dim.z = return_uint16;
  } else {
    if (return_str != NULL) {
      if (sscanf(return_str, "%d", &(return_sint32)) != 1)
	g_warning("could not process NumberOfFrames - Failed to load file %s\n", filename);
      dim.z = return_sint32;
      if (dim.z < 1)
	g_warning("in correct NumberOf Frames (%d) - Failed to load file %s\n", dim.z, filename);
    }
  }

  dim.t = 1; /* no support for multiple time frame of data in a single file */

  dim.g = 1; /* no support for multiple gates in a single file */

  /* TimeSlices can also indicate # of bed positions, so only use for number of frames
     if this is explicitly a DYNAMIC data set */
  if (dcm_dataset->findAndGetString(DCM_SeriesType, return_str, OFTrue).good()) 
    if (return_str != NULL)
      if (strstr(return_str, "DYNAMIC") != NULL)
	if (dcm_dataset->findAndGetUint16(DCM_NumberOfTimeSlices, return_uint16).good()) 
	  *pnum_frames = return_uint16;

  
  if (dcm_dataset->findAndGetString(DCM_SeriesType, return_str, OFTrue).good()) 
    if (return_str != NULL) 
      if (strstr(return_str, "GATED\\IMAGE") != NULL)
	if (dcm_dataset->findAndGetUint16(DCM_NumberOfTimeSlots, return_uint16).good()) 
	  *pnum_gates = return_uint16;

  /* some files use NumberOfTemporalPositions */
  if (*pnum_gates == 1)
    if (dcm_dataset->findAndGetSint32(DCM_NumberOfTemporalPositions, return_sint32).good()) 
      if (return_sint32 > 0)
	*pnum_gates = return_sint32;

  /* record how many slices in each volume. Used if there is an inconsistent between the 
     number of gates and total number of images */
  /* 0x0021,0x104f corresponds to LocationsInAcquisition */
  if (dcm_dataset->findAndGetSint16(DcmTagKey(0x0021, 0x104f), return_sint16).good())
    if (return_sint16 > 0)
      *pnum_slices = return_sint16;

  if (dcm_dataset->findAndGetUint16(DCM_BitsAllocated, return_uint16).bad()) {
    g_warning(_("could not find # of bits allocated - Failed to load file %s\n"), filename);
    goto error;
  }
  bits_allocated = return_uint16;

  if (dcm_dataset->findAndGetUint16(DCM_PixelRepresentation, return_uint16).bad()) {
    g_warning(_("could not find pixel representation - Failed to load file %s\n"), filename);
    goto error;
  }
  pixel_representation = return_uint16;

  switch(bits_allocated) {
  case 8:
    if (pixel_representation) format = AMITK_FORMAT_SBYTE;
    else format = AMITK_FORMAT_UBYTE;
    break;
  case 16:
    if (pixel_representation) format = AMITK_FORMAT_SSHORT;
    else format = AMITK_FORMAT_USHORT;
    break;
  case 32:
    /* DICOM doesn't actually support anything but 8 and 16bit */
    if (pixel_representation) format = AMITK_FORMAT_SINT;
    else format = AMITK_FORMAT_UINT;
    break;
  default:
    g_warning(_("unsupported # of bits allocated (%d) - Failed to load file %s\n"), bits_allocated, filename);
    goto error;
  }

  /* malloc data set */
  ds = amitk_data_set_new_with_data(preferences, modality, format, dim, AMITK_SCALING_TYPE_0D_WITH_INTERCEPT);
  if (ds == NULL) {
    g_warning(_("Couldn't allocate space for the data set structure to hold DCMTK data - Failed to load file %s"), filename);
    goto error;
  }

  /* get the series number */
  if (dcm_dataset->findAndGetSint32(DCM_SeriesNumber, return_sint32).good())
    amitk_data_set_set_series_number(ds, return_sint32);

  /* get the dicom image type, we don't do anything with this, but we store
     it in case we're going to export this data set as DICOM data */
  if (dcm_dataset->findAndGetString(DCM_ImageType, return_str).good())
    amitk_data_set_set_dicom_image_type(ds, return_str); /* function can handle NULL strings */

  /* get the voxel size */
  if (dcm_dataset->findAndGetFloat64(DCM_PixelSpacing, return_float64,0).good()) {
    voxel_size.y = return_float64;
    if (dcm_dataset->findAndGetFloat64(DCM_PixelSpacing, return_float64,1).good())
      voxel_size.x = return_float64;
    else {
      voxel_size.x = voxel_size.y;
    }
  } else {
    amitk_append_str_with_newline(perror_buf, _("Could not find the pixel size, setting to 1 mm for File %s"), filename);
  }

  /* try to figure out the correct z thickness */
  voxel_size.z = -1;
  if (dcm_dataset->findAndGetFloat64(DCM_SpacingBetweenSlices, return_float64).good()) 
    voxel_size.z = return_float64;

  if (voxel_size.z <= 0.0) {
    if (dcm_dataset->findAndGetFloat64(DCM_SliceThickness, return_float64).good()) {
      voxel_size.z = return_float64;
    } else {
      amitk_append_str_with_newline(perror_buf, _("Could not find the slice thickness, setting to 1 mm for File %s"), filename);
      voxel_size.z = 1;
    }
  }

  /* store this number, occasionally used in sorting */
  if (dcm_dataset->findAndGetSint32(DCM_InstanceNumber, return_sint32).good())
    ds->instance_number = return_sint32;

  /* DICOM coordinates are LPH+, meaning that x increases toward patient left, y increases towards
     patient posterior, and z increases toward patient head. AMIDE is LAF+ */
  /* ImageOrientationPatient contains a vector along the row direction, as well as the column direction,
     note though that the column direction in DICOM is in the wrong direction - it's from top to bottom,
     the z axis is off as well */
  new_axes[AMITK_AXIS_X] = base_axes[AMITK_AXIS_X];
  new_axes[AMITK_AXIS_Y] = base_axes[AMITK_AXIS_Y];
  new_axes[AMITK_AXIS_Z] = base_axes[AMITK_AXIS_Z];
  if (dcm_dataset->findAndGetFloat64(DCM_ImageOrientationPatient, return_float64, 0).good()) {
    new_axes[AMITK_AXIS_X].x = return_float64;
    if (dcm_dataset->findAndGetFloat64(DCM_ImageOrientationPatient, return_float64, 1).good()) {
      new_axes[AMITK_AXIS_X].y = return_float64;
      if (dcm_dataset->findAndGetFloat64(DCM_ImageOrientationPatient, return_float64, 2).good()) {
	new_axes[AMITK_AXIS_X].z = return_float64;
	if (dcm_dataset->findAndGetFloat64(DCM_ImageOrientationPatient, return_float64, 3).good()) {
	  new_axes[AMITK_AXIS_Y].x = return_float64;
	  if (dcm_dataset->findAndGetFloat64(DCM_ImageOrientationPatient, return_float64, 4).good()) {
	    new_axes[AMITK_AXIS_Y].y = return_float64;
	    if (dcm_dataset->findAndGetFloat64(DCM_ImageOrientationPatient, return_float64, 5).good()) {
	      new_axes[AMITK_AXIS_Y].z = return_float64;
	    }
	  }
	}
      }
    }
  }
  new_axes[AMITK_AXIS_X].y *= -1.0;
  new_axes[AMITK_AXIS_X].z *= -1.0;
  new_axes[AMITK_AXIS_Y].y *= -1.0;
  new_axes[AMITK_AXIS_Y].z *= -1.0;
  POINT_CROSS_PRODUCT(new_axes[AMITK_AXIS_X], new_axes[AMITK_AXIS_Y], new_axes[AMITK_AXIS_Z]);
  amitk_space_set_axes(AMITK_SPACE(ds), new_axes, zero_point);

  /* note, ImagePositionPatient is suppose to be defined as the center of the upper left hand voxel 
     in Patient space, not necessarily with respect to the Gantry. At least for GE scanners, 
     the offset is on corner of this voxel */
  found_value=FALSE;
  if (dcm_dataset->findAndGetFloat64(DCM_ImagePositionPatient, return_float64, 0).good()) {
    new_offset.x = return_float64;
    if (dcm_dataset->findAndGetFloat64(DCM_ImagePositionPatient, return_float64, 1).good()) {
      new_offset.y = return_float64;
      if (dcm_dataset->findAndGetFloat64(DCM_ImagePositionPatient, return_float64, 2).good()) {
      	new_offset.z = return_float64;

	/* not doing the half voxel correction... values seem more correct without, at least for GE.
	   If this was not the case, the code would be:
	      new_offset.x -= 0.5*voxel_size.x;
	      new_offset.y -= 0.5*voxel_size.y;
	      new_offset.z -= 0.5*voxel_size.z;
	*/

	/* Offset seems to be in base coordinate space, otherwise the code would be
	   amitk_space_set_offset(AMITK_SPACE(ds), amitk_space_s2b(AMITK_SPACE(ds), new_offset));
	*/
	new_offset.y = -1.0*new_offset.y; /* DICOM specifies y axis in wrong direction */
	new_offset.z = -1.0*new_offset.z; /* DICOM specifies z axis in wrong direction */

	amitk_space_set_offset(AMITK_SPACE(ds), new_offset);
	found_value=TRUE;
      }
    }
  }
  if (!found_value) { /* see if we can get it otherwise */
    if (dcm_dataset->findAndGetFloat64(DCM_SliceLocation, return_float64).good()) {
      /* if no ImagePositionPatient, try using SliceLocation */
      new_offset.x = new_offset.y = 0.0;
      //this looks wrong 2007.11.21 - doing what's done in the DCM_ImagePositionPatient case
      // new_offset.z = -1.0*(return_float64+voxel_size.z*dim.z); /* DICOM specifies z axis in wrong direction */
      new_offset.z = -1.0*return_float64; /* DICOM specifies z axis in wrong direction */
      amitk_space_set_offset(AMITK_SPACE(ds), new_offset);
    }
  }


  amitk_data_set_set_voxel_size(ds, voxel_size);
  amitk_data_set_calc_far_corner(ds);

  if (dcm_dataset->findAndGetString(DCM_PatientID, return_str, OFTrue).good()) 
    amitk_data_set_set_subject_id(ds, return_str);

  if (dcm_dataset->findAndGetFloat64(DCM_PatientWeight, return_float64).good()) {
    amitk_data_set_set_subject_weight(ds, return_float64);
    amitk_data_set_set_displayed_weight_unit(ds, AMITK_WEIGHT_UNIT_KILOGRAM);
  }

  /* any MRI parameters we may need */
  if (dcm_dataset->findAndGetFloat64(DCM_InversionTime, return_float64).good())
    amitk_data_set_set_inversion_time(ds, return_float64);
  if (dcm_dataset->findAndGetFloat64(DCM_EchoTime, return_float64).good())
    amitk_data_set_set_echo_time(ds, return_float64);

  if (dcm_dataset->findAndGetFloat64(DCM_DiffusionBValue, return_float64, 0, OFTrue).good()) {
    amitk_data_set_set_diffusion_b_value(ds, return_float64);
  } else if (dcm_dataset->findAndGetSint32(DcmTagKey(0x0043, 0x1039), return_sint32, 0, OFTrue).good()) { /* GE */
    amitk_data_set_set_diffusion_b_value(ds, return_sint32);
  }

  /* diffusion directions */
  direction = zero_point;

  if (dcm_dataset->findAndGetFloat64(DCM_DiffusionGradientOrientation, return_float64, 0, OFTrue).good())
    direction.x = return_float64;
  if (dcm_dataset->findAndGetFloat64(DCM_DiffusionGradientOrientation, return_float64, 1, OFTrue).good())
    direction.y = return_float64;
  if (dcm_dataset->findAndGetFloat64(DCM_DiffusionGradientOrientation, return_float64, 2, OFTrue).good())
    direction.z = return_float64;

  /* following works for GE.... */
  if (POINT_EQUAL(direction, zero_point)) {
    if (dcm_dataset->findAndGetFloat64(DcmTagKey(0x0019, 0x10bb), return_float64, 0, OFTrue).good())
      direction.x = return_float64;
    if (dcm_dataset->findAndGetFloat64(DcmTagKey(0x0019, 0x10bc), return_float64, 0, OFTrue).good())
      direction.y = return_float64;
    if (dcm_dataset->findAndGetFloat64(DcmTagKey(0x0019, 0x10bd), return_float64, 0, OFTrue).good())
      direction.z = return_float64;
  }

  amitk_data_set_set_diffusion_direction(ds, direction);
  
  /* store the gate time if possible, DICOM stores this as milliseconds */
  if (dcm_dataset->findAndGetFloat64(DCM_TriggerTime, return_float64).good())
    amitk_data_set_set_gate_time(ds, 0, return_float64/1000.0);
  else
    amitk_data_set_set_gate_time(ds, 0, 0.0);

  /* and get the Temporal position, DICOM uses this with gated MR images */
  if (dcm_dataset->findAndGetSint32(DCM_TemporalPositionIdentifier, return_sint32).good())
    ds->gate_num=return_sint32;
  else
    ds->gate_num=-1;

  /* find the most relevant start time */
  return_str = NULL;
  if (!dcm_dataset->findAndGetString(DCM_SeriesTime, return_str, OFTrue).good()) 
    if (!dcm_dataset->findAndGetString(DCM_AcquisitionTime, return_str, OFTrue).good()) 
      dcm_dataset->findAndGetString(DCM_StudyTime, return_str, OFTrue);
  if (return_str != NULL) {
    if (sscanf(return_str, "%2d%2d%2d\n", &hours, &minutes, &seconds)==3) {
      acquisition_start_time = ((hours*60)+minutes)*60+seconds;
    }
  }


  if (dcm_dataset->findAndGetString(DCM_RadiopharmaceuticalStartTime, return_str, OFTrue).good()) 
    if (return_str != NULL) 
      if (sscanf(return_str, "%2d%2d%2d\n", &hours, &minutes, &seconds)==3) 
	dose_time = ((hours*60)+minutes)*60+seconds;

  if ((dose_time >= 0.0) && (acquisition_start_time >= 0.0)) {
    /* correct for if the dose was measured the previous day... note, DICOM can't seem to handle
       dose measured more than 24hours in the past */
    if (acquisition_start_time > dose_time)
      decay_time = acquisition_start_time-dose_time;
    else
      decay_time = acquisition_start_time+24*60*60-dose_time; 
  }

  if (dcm_dataset->findAndGetFloat64(DCM_RadionuclideHalfLife, return_float64, 0, OFTrue).good()) 
    half_life = return_float64;

  if (dcm_dataset->findAndGetFloat64(DCM_RadionuclideTotalDose, return_float64, 0, OFTrue).good()) {
    /* note, for NM data sets, total dose is in MBq. For PET data sets total dose is
       suppose to be in Bq.  However, it seems that the value is sometimes in the 
       wrong units, at least for GE data. */
    decay_value = return_float64;
    if (decay_value > 10000.0) decay_value /= 1E6; /* if more than 10000, assume its in Bq, and convert */
    if ((decay_time > 0.0) && (half_life > 0.0))
      decay_value *= pow(0.5, decay_time/half_life);
    amitk_data_set_set_injected_dose(ds, decay_value);
    amitk_data_set_set_displayed_dose_unit(ds, AMITK_DOSE_UNIT_MEGABECQUEREL);
  }

  if (dcm_dataset->findAndGetString(DCM_Units, return_str, OFTrue).good()) {
    if (return_str != NULL) {
      /* BQML - voxel values are in Bq per ml */
      if (strcmp(return_str, "BQML")==0) {
	amitk_data_set_set_cylinder_factor(ds,1.0/1.0E6);
	amitk_data_set_set_displayed_cylinder_unit(ds, AMITK_CYLINDER_UNIT_MEGABECQUEREL_PER_CC_IMAGE_UNIT);
      }
    }
  }

  if (dcm_dataset->findAndGetString(DCM_StudyDescription, return_str, OFTrue).good()) {
    amitk_object_set_name(AMITK_OBJECT(ds), return_str);
    if (pstudyname != NULL)
      *pstudyname = g_strdup(return_str);
  }

  if (dcm_dataset->findAndGetString(DCM_SeriesDescription, return_str, OFTrue).good()) {
    /* note, this overwrites the name set immediately above based on StudyDescription */
    if (return_str != NULL) 
      amitk_object_set_name(AMITK_OBJECT(ds), return_str);
  }

  if (dcm_dataset->findAndGetString(DCM_PatientName, return_str, OFTrue).good()) {
      if (AMITK_OBJECT_NAME(ds) == NULL)
	amitk_object_set_name(AMITK_OBJECT(ds), return_str);
      amitk_data_set_set_subject_name(ds, return_str);
  }

  if (dcm_dataset->findAndGetString(DCM_SeriesDate, return_str, OFTrue).good()) 
    scan_date = return_str;
  else if (dcm_dataset->findAndGetString(DCM_AcquisitionDate, return_str, OFTrue).good()) 
    scan_date = return_str;
  else if (dcm_dataset->findAndGetString(DCM_StudyDate, return_str, OFTrue).good()) 
    scan_date = return_str;

  if (dcm_dataset->findAndGetString(DCM_SeriesTime, return_str, OFTrue).good()) 
    scan_time = return_str;
  else if (dcm_dataset->findAndGetString(DCM_AcquisitionTime, return_str, OFTrue).good()) 
    scan_time = return_str;
  else if (dcm_dataset->findAndGetString(DCM_StudyTime, return_str, OFTrue).good()) 
    scan_time = return_str;

  valid=FALSE;
  if (scan_date != NULL) {
    if (3 == sscanf(scan_date, "%4d%2d%2d", 
		    &(time_structure.tm_year),
		    &(time_structure.tm_mon),
		    &(time_structure.tm_mday))) {
      time_structure.tm_year -= 1900; /* years start from 1900 */
      if (time_structure.tm_mon > 0)
	time_structure.tm_mon -= 1; /* now from 0-11 */

      if (scan_time != NULL) 
	if (3 == sscanf(scan_time, "%2d%2d%2d",
			&(time_structure.tm_hour),
			&(time_structure.tm_min),
			&(time_structure.tm_sec)))
	  valid = TRUE;

      if (!valid) {
	time_structure.tm_hour = 0;
	time_structure.tm_min = 0;
	time_structure.tm_sec = 0;
      }
    }

    time_structure.tm_isdst = -1; /* "-1" is suppose to let the system figure it out */

    if (mktime(&time_structure) != -1) {
      amitk_data_set_set_scan_date(ds, asctime(&time_structure));
      valid = TRUE;
    } else {
      valid = FALSE;
    }
  }

  if (!valid) {
    if ((scan_date != NULL) && (scan_time != NULL)) {
      temp_str = g_strdup_printf("%s %s\n",scan_date,scan_time);
      amitk_data_set_set_scan_date(ds,temp_str);
      g_free(temp_str);
    } else if (scan_date != NULL) {
      amitk_data_set_set_scan_date(ds,scan_date);
    } else if (scan_time != NULL) {
      amitk_data_set_set_scan_date(ds,scan_time);
    }
  }

  if (dcm_dataset->findAndGetString(DCM_PatientBirthDate, return_str, OFTrue).good()) 
    amitk_data_set_set_subject_dob(ds, return_str);

  if (dcm_dataset->findAndGetString(DCM_PatientSex, return_str, OFTrue).good())
    if (return_str != NULL) {
      if (return_str[0] == 'M')
	amitk_data_set_set_subject_sex(ds, AMITK_SUBJECT_SEX_MALE);
      else if (return_str[0] == 'F')
	amitk_data_set_set_subject_sex(ds, AMITK_SUBJECT_SEX_FEMALE);
      /* default is unknown */
    }

  /* because of how the dicom coordinates are setup, after reading in the patient slices, 
     they should all be oriented as SUPINE_HEADFIRST in AMIDE */
  /* 20071121 - renabled following code, not sure if above statement is true */
  //  amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_SUPINE_HEADFIRST);
  if (dcm_dataset->findAndGetString(DCM_PatientPosition, return_str, OFTrue).good()) {
    if (return_str != NULL) {
      if (g_ascii_strcasecmp(return_str, "HFS")==0)
  	amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_SUPINE_HEADFIRST);
      else if (g_ascii_strcasecmp(return_str, "FFS")==0)
  	amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_SUPINE_FEETFIRST);
      else if (g_ascii_strcasecmp(return_str, "HFP")==0)
  	amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_PRONE_HEADFIRST);
      else if (g_ascii_strcasecmp(return_str, "FFP")==0)
  	amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_PRONE_FEETFIRST);
      else if (g_ascii_strcasecmp(return_str, "HFDR")==0)
  	amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_RIGHT_DECUBITUS_HEADFIRST);
      else if (g_ascii_strcasecmp(return_str, "FFDR")==0)
  	amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_RIGHT_DECUBITUS_FEETFIRST);
      else if (g_ascii_strcasecmp(return_str, "HFDL")==0)
  	amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_LEFT_DECUBITUS_HEADFIRST);
      else if (g_ascii_strcasecmp(return_str, "FFDL")==0)
  	amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_LEFT_DECUBITUS_FEETFIRST);
    }
  }

  format_size = amitk_format_sizes[format];
  /*  num_bytes = amitk_raw_format_calc_num_bytes(dim, amitk_format_to_raw_format(format)); */

  /* a "GetSint16Array" function is also provided, but for some reason I get an error
     when using it.  I'll just use GetUint16Array even for signed stuff */
  switch(format) {
  case AMITK_FORMAT_SBYTE:
  case AMITK_FORMAT_UBYTE:
    {
      const Uint8 * temp_buffer;
      result = dcm_dataset->findAndGetUint8Array(DCM_PixelData, temp_buffer);
      buffer = (void *) temp_buffer;
      break;
    }
  case AMITK_FORMAT_SSHORT:
  case AMITK_FORMAT_USHORT:
    {
      const Uint16 * temp_buffer;
      result = dcm_dataset->findAndGetUint16Array(DCM_PixelData, temp_buffer);
      buffer = (void *) temp_buffer;
      break;
    }
  case AMITK_FORMAT_SINT:
  case AMITK_FORMAT_UINT:
    {
      const Uint32 * temp_buffer;
      result = dcm_dataset->findAndGetUint32Array(DCM_PixelData, temp_buffer);
      //      result = dcm_dataset->findAndGetUint16Array(DCM_PixelData, temp_buffer);
      buffer = (void *) temp_buffer;
      break;
    }
  default:
    g_warning("unsupported data format in %s at %d\n", __FILE__, __LINE__);
    goto error;
    break;
  }

  if (result.bad()) {
    g_warning(_("error reading in pixel data -  DCMTK error: %s - Failed to read file %s"),result.text(), filename);
    goto error;
  }

  i = zero_voxel;

  /* store the scaling factor... if there is one */
  if (dcm_dataset->findAndGetFloat64(DCM_RescaleSlope, return_float64,0).good())
    *AMITK_RAW_DATA_DOUBLE_2D_SCALING_POINTER(ds->internal_scaling_factor, i) = return_float64;

  /* same for the offset */
  if (dcm_dataset->findAndGetFloat64(DCM_RescaleIntercept, return_float64).good()) 
    *AMITK_RAW_DATA_DOUBLE_2D_SCALING_POINTER(ds->internal_scaling_intercept, i) = return_float64;

  if (dcm_dataset->findAndGetFloat64(DCM_FrameReferenceTime, return_float64).good()) 
    amitk_data_set_set_scan_start(ds,return_float64/1000.0);

  /* and load in the data - plane by plane */
  for (i.t = 0; (i.t < dim.t) && (continue_work); i.t++) {

    /* note ... doesn't seem to be a way to encode different frame durations within one dicom file */
    if (dcm_dataset->findAndGetSint32(DCM_ActualFrameDuration, return_sint32).good()) {
      amitk_data_set_set_frame_duration(ds,i.t, ((gdouble) return_sint32)/1000.0);
      /* make sure it's not zero */
      if (amitk_data_set_get_frame_duration(ds,i.t) < EPSILON) 
	amitk_data_set_set_frame_duration(ds,i.t, EPSILON);
    }

    for (i.g = 0; (i.g < ds->raw_data->dim.g) && (continue_work); i.g++) {

      /* note, we've already flipped the coordinate axis, so reading in the data straight is correct */
      ds_pointer = amitk_raw_data_get_pointer(AMITK_DATA_SET_RAW_DATA(ds), i);
      memcpy(ds_pointer, (guchar *) buffer, format_size*ds->raw_data->dim.x*ds->raw_data->dim.y*ds->raw_data->dim.z);

    }
  }


  amitk_data_set_set_scale_factor(ds, 1.0); /* set the external scaling factor */
  amitk_data_set_calc_far_corner(ds); /* set the far corner of the volume */
  amitk_data_set_calc_min_max(ds, update_func, update_data);

  goto function_end;


 error:

  if (ds != NULL) {
    ds = AMITK_DATA_SET(amitk_object_unref(ds));
    ds = NULL;
  }


 function_end:
  return ds;
}

void transfer_slice(AmitkDataSet * ds, AmitkDataSet * slice_ds, AmitkVoxel i) {

  size_t num_bytes_to_transfer;
  void * ds_pointer;
  void * slice_pointer;

  num_bytes_to_transfer =  amitk_raw_format_calc_num_bytes(AMITK_DATA_SET_DIM(slice_ds), AMITK_DATA_SET_FORMAT(slice_ds));

  ds_pointer = amitk_raw_data_get_pointer(AMITK_DATA_SET_RAW_DATA(ds), i);
  g_return_if_fail(ds_pointer != NULL);
  slice_pointer = amitk_raw_data_get_pointer(AMITK_DATA_SET_RAW_DATA(slice_ds), zero_voxel);
  g_return_if_fail(slice_pointer != NULL);
  memcpy(ds_pointer, slice_pointer, num_bytes_to_transfer);
	  
  /* copy the scaling factors */
  *AMITK_RAW_DATA_DOUBLE_2D_SCALING_POINTER(ds->internal_scaling_factor, i) = 
    amitk_data_set_get_internal_scaling_factor(slice_ds, zero_voxel);
  *AMITK_RAW_DATA_DOUBLE_2D_SCALING_POINTER(ds->internal_scaling_intercept, i) = 
    amitk_data_set_get_scaling_intercept(slice_ds, zero_voxel);

  return;
}

/* sort by location */
static gint sort_slices_func(gconstpointer a, gconstpointer b) {
  AmitkDataSet * slice_a = (AmitkDataSet *) a;
  AmitkDataSet * slice_b = (AmitkDataSet *) b;
  AmitkPoint offset_b;

  g_return_val_if_fail(AMITK_IS_DATA_SET(slice_a), -1);
  g_return_val_if_fail(AMITK_IS_DATA_SET(slice_b), 1);

  offset_b = amitk_space_b2s(AMITK_SPACE(slice_a), AMITK_SPACE_OFFSET(slice_b));

  if (offset_b.z > 0.0)
    return -1;
  else if (offset_b.z < 0.0) 
    return 1;
  else
    return 0;
}

/* sort by time, then by location */
static gint sort_slices_func_with_time(gconstpointer a, gconstpointer b) {
  AmitkDataSet * slice_a = (AmitkDataSet *) a;
  AmitkDataSet * slice_b = (AmitkDataSet *) b;
  amide_time_t scan_start_a, scan_start_b;

  g_return_val_if_fail(AMITK_IS_DATA_SET(slice_a), -1);
  g_return_val_if_fail(AMITK_IS_DATA_SET(slice_b), 1);

  /* first sort by time */
  scan_start_a = AMITK_DATA_SET_SCAN_START(slice_a);
  scan_start_b = AMITK_DATA_SET_SCAN_START(slice_b);
  if (!REAL_EQUAL(scan_start_a, scan_start_b)) {
    if (scan_start_a < scan_start_b)
      return -1;
    else
      return 1;
  }

  /* then sort by location */
  return sort_slices_func(a,b);
}

/* sort by gate, then by location */
static gint sort_slices_func_with_gate(gconstpointer a, gconstpointer b) {
  AmitkDataSet * slice_a = (AmitkDataSet *) a;
  AmitkDataSet * slice_b = (AmitkDataSet *) b;
  amide_time_t gate_time_a, gate_time_b;

  g_return_val_if_fail(AMITK_IS_DATA_SET(slice_a), -1);
  g_return_val_if_fail(AMITK_IS_DATA_SET(slice_b), 1);

  /* first sort by gate if we can. Note, we set gate_num to -1 if
     there's no entry for it in the DICOM file */
  if ((slice_a->gate_num >= 0) && (slice_b->gate_num >=0)) {
    if (slice_a->gate_num < slice_b->gate_num)
      return -1;
    else if (slice_a->gate_num > slice_b->gate_num)
      return 1;
  }

  /* if we don't have gate_num entries, sort by gate time */
  gate_time_a = amitk_data_set_get_gate_time(slice_a, 0);
  gate_time_b = amitk_data_set_get_gate_time(slice_b, 0);
  if (!REAL_EQUAL(gate_time_a, gate_time_b)) {
    if (gate_time_a < gate_time_b)
      return -1;
    else
      return 1;
  }

  /* then sort by location */
  return sort_slices_func(a,b);
}

static GList * find_slices_that_match(GList ** pslices) {

  AmitkDataSet * initial_ds;
  AmitkDataSet * comparison_ds;
  GList * matching_slices=NULL;
  gboolean match;
  GList * current_slices;

  g_return_val_if_fail(*pslices != NULL, NULL);

  /* put the first slice into the matching set */
  initial_ds = AMITK_DATA_SET(g_list_nth_data(*pslices, 0));
  *pslices = g_list_remove(*pslices, initial_ds);
  matching_slices = g_list_append(matching_slices, initial_ds);
  current_slices = *pslices;

  /* go through the rest of the slices and find what matches */
  while (current_slices != NULL) {
    comparison_ds = AMITK_DATA_SET(current_slices->data);

    /* check dimensions are equal */
    match = (VOXEL_EQUAL(AMITK_DATA_SET_DIM(initial_ds), AMITK_DATA_SET_DIM(comparison_ds)));

    /* check voxel sizes are equal */
    if (match)
      match = (POINT_EQUAL(AMITK_DATA_SET_VOXEL_SIZE(initial_ds), AMITK_DATA_SET_VOXEL_SIZE(comparison_ds)));

    /* check that the orientation of the slices are equal. Note, we use _close instead of _equal (CLOSE vs EPSILON) because there values
       are coming from character strings in the DICOM header, and may be a little imprecise. */
    if (match)
      match = amitk_space_axes_close(AMITK_SPACE(initial_ds), AMITK_SPACE(comparison_ds));

    /* check that the image type tags are the same. g_strcmp0 handles NULL pointers */
    if (match)
      match = (g_strcmp0(AMITK_DATA_SET_DICOM_IMAGE_TYPE(initial_ds), AMITK_DATA_SET_DICOM_IMAGE_TYPE(comparison_ds)) == 0);

    /* if MRI, check inversion, echo times, and b-value are equal */
    if (AMITK_DATA_SET_MODALITY(initial_ds) == AMITK_MODALITY_MRI)  {
      if (match)
	if (!isnan (AMITK_DATA_SET_INVERSION_TIME(initial_ds)) && !isnan(AMITK_DATA_SET_INVERSION_TIME(comparison_ds)))
	  match = REAL_EQUAL(AMITK_DATA_SET_INVERSION_TIME(initial_ds), AMITK_DATA_SET_INVERSION_TIME(comparison_ds));
      if (match)
	if (!isnan (AMITK_DATA_SET_ECHO_TIME(initial_ds)) && !isnan(AMITK_DATA_SET_ECHO_TIME(comparison_ds)))
	  match = REAL_EQUAL(AMITK_DATA_SET_ECHO_TIME(initial_ds), AMITK_DATA_SET_ECHO_TIME(comparison_ds));
      if (match)
	if (!isnan (AMITK_DATA_SET_DIFFUSION_B_VALUE(initial_ds)) && !isnan(AMITK_DATA_SET_DIFFUSION_B_VALUE(comparison_ds))) {
	  match = REAL_EQUAL(AMITK_DATA_SET_DIFFUSION_B_VALUE(initial_ds), AMITK_DATA_SET_DIFFUSION_B_VALUE(comparison_ds));
	  if (match)
	    match = POINT_EQUAL(AMITK_DATA_SET_DIFFUSION_DIRECTION(initial_ds), AMITK_DATA_SET_DIFFUSION_DIRECTION(comparison_ds));
	}
    }

    if (match) 
      matching_slices = g_list_append(matching_slices, comparison_ds);

    current_slices = current_slices->next;
  }

  current_slices = matching_slices;
  while (current_slices != NULL) {
    *pslices = g_list_remove(*pslices, current_slices->data);
    current_slices = current_slices->next;
  }

  return matching_slices;
}

static GList * separate_duplicate_slices(GList * slices_to_combine, GList ** premaining_slices) {

  GList * current_slices;
  AmitkDataSet * previous_ds;
  AmitkDataSet * current_ds;
  AmitkDataSet * discard_ds;

  g_return_val_if_fail(slices_to_combine != NULL, NULL);
  g_return_val_if_fail(premaining_slices != NULL, NULL);

  current_slices = g_list_first(slices_to_combine);
  previous_ds = AMITK_DATA_SET(current_slices->data);

  /* get a list of the duplicates */
  current_slices = current_slices->next;
  while (current_slices != NULL) {
    current_ds = AMITK_DATA_SET(current_slices->data);
    current_slices = current_slices->next;

    if (POINT_EQUAL(AMITK_SPACE_OFFSET(previous_ds),
		    AMITK_SPACE_OFFSET(current_ds))) {
      if (current_ds->instance_number < previous_ds->instance_number) {
	discard_ds = previous_ds;
	previous_ds = current_ds; /* for next iteration */
      } else {
	discard_ds = current_ds;
	/* previous_ds stays the same */
      }

      *premaining_slices = g_list_append(*premaining_slices, discard_ds);
      slices_to_combine = g_list_remove(slices_to_combine, discard_ds);
    } else {
      previous_ds = current_ds;
    }
  }

  return slices_to_combine;
}

static AmitkDataSet * import_slices_as_dataset(GList * slices, 
					       gint num_frames, 
					       gint num_gates,
					       gint num_slices,
					       AmitkUpdateFunc update_func,
					       gpointer update_data,
					       gchar **perror_buf) {

  AmitkDataSet * ds=NULL;
  gint num_files;
  gint i_file;
  AmitkDataSet * slice_ds=NULL;
  AmitkVoxel dim, scaling_dim;
  div_t x;
  AmitkVoxel i;
  AmitkPoint offset, initial_offset, diff;
  gboolean screwed_up_timing;
  gboolean screwed_up_thickness;
  amide_real_t true_thickness=0.0;
  amide_real_t old_thickness=0.0;
  AmitkPoint voxel_size;
  gboolean figured_out_dimz=FALSE;

  g_return_val_if_fail(slices != NULL, NULL);

  screwed_up_timing=FALSE;
  screwed_up_thickness=FALSE;
  num_files = g_list_length(slices);

  
  /* special stuff for 1st slice */
  slice_ds = (AmitkDataSet *) g_list_nth_data(slices,0);

  dim = AMITK_DATA_SET_DIM(slice_ds);

  if ((dim.z != 1) && (num_files != 1)) {
    g_warning("Don't know how to deal with multiple files containing multiple slices");
    goto error;
  }

  if (dim.z == 1) 
    dim.z = num_files;

  /* dealing with dynamic or gated data */
  if ((num_frames > 1) || (num_gates > 1)) {

    if (num_frames > 1) 
      x = div(dim.z, num_frames);
    else /* (num_gates > 1) */
      x = div(dim.z, num_gates);

    if (x.rem == 0) {
      /* ideal case, things make sense */
      if (num_frames > 1) 
	dim.t = num_frames;
      else /* (num_gates > 1) */
	dim.g = num_gates;
      dim.z = x.quot;
      figured_out_dimz = TRUE;
    }


    if ((!figured_out_dimz) && (num_slices > 1)) {
      /* inconsistency in our data set.... let's assume num_gates or num frames off, and 
	 see if we can use the num_slices parameter to recover */
      x = div(dim.z, num_slices);

      if (x.rem == 0) {
	/* try to recover based on what the DICOM file thinks is the number of slices per frame */
	amitk_append_str_with_newline(perror_buf, 
				      _("Cannot evenly divide the number of slices (%d) by the number of reported %s (%d) for data set %s - will try with %d %s"), 
				      dim.z,
				      num_frames > 1 ? _("frames") : _("gates"), 
				      num_frames > 1 ? num_frames : num_gates,
				      AMITK_OBJECT_NAME(slice_ds),
				      x.quot,
				      num_frames > 1 ? _("frames") : _("gates"));
	if (num_frames > 1) 
	  dim.t = x.quot;
	else /* (num_gates > 1) */
	  dim.g = x.quot;
	dim.z = num_slices;
	figured_out_dimz = TRUE;
      }
    }

    if (!figured_out_dimz) {
      /* still have inconsistency in our data set....  */

      /* if we have a num_slices parameter, we'll end up using that */
      if (num_slices > 1)
	x.quot = num_slices;

      amitk_append_str_with_newline(perror_buf, 
				    _("Cannot evenly divide the number of slices (%d) by the number of %s (%d) for data set %s - will load first %d slices"), 
				    dim.z,
				    num_frames > 1 ? _("frames") : _("gates"), 
				    num_frames > 1 ? num_frames : num_gates,
				    AMITK_OBJECT_NAME(slice_ds),
				    (x.quot > 0) ? x.quot : dim.z);
      
      /* failure */
      if (num_frames > 1) 
	dim.t = num_frames = 1;
      else /* (num_gates > 1) */
	dim.g = num_gates = 1;
      if (x.quot > 0)
	dim.z = x.quot;
      figured_out_dimz = TRUE;
    }
  } /* dynamic/gated data */


  ds = AMITK_DATA_SET(amitk_object_copy(AMITK_OBJECT(slice_ds)));
      
  /* unref and remalloc what we need */
  if (ds->raw_data != NULL) g_object_unref(ds->raw_data);
  ds->raw_data = amitk_raw_data_new_with_data(AMITK_DATA_SET_FORMAT(slice_ds), dim);
  if (ds->raw_data == NULL) {
    g_warning("Could not allocate memory for raw data with dimensions %d x %d x %d x %d x %d", dim.g, dim.t, dim.z, dim.y, dim.x);
    goto error;
  }

  if (ds->distribution != NULL) g_object_unref(ds->distribution);
  ds->distribution = NULL;
  
  /* if we're loading up multiple files, each file has a separate scale factor, so we'll use
     per slice (2D) scaling. If we're loading up a single file with multiple data frames in it,
     there's only one scaling factor, so per frame (3D) scaling */
  if (num_files > 1)
    ds->scaling_type = AMITK_SCALING_TYPE_2D_WITH_INTERCEPT;
  else
    ds->scaling_type = AMITK_SCALING_TYPE_1D_WITH_INTERCEPT;
  scaling_dim=one_voxel;
  scaling_dim.z = (num_files > 1) ? dim.z : 1;
  scaling_dim.g = dim.g;
  scaling_dim.t = dim.t;
  
  if (ds->internal_scaling_factor != NULL) g_object_unref(ds->internal_scaling_factor);
  ds->internal_scaling_factor = amitk_raw_data_new_with_data(AMITK_FORMAT_DOUBLE, scaling_dim);
  if (ds->internal_scaling_intercept != NULL) g_object_unref(ds->internal_scaling_intercept);
  ds->internal_scaling_intercept = amitk_raw_data_new_with_data(AMITK_FORMAT_DOUBLE, scaling_dim);
  if (ds->current_scaling_factor != NULL) g_object_unref(ds->current_scaling_factor);
  ds->current_scaling_factor=NULL;
  
  if (ds->frame_max != NULL)
    g_free(ds->frame_max);
  ds->frame_max = amitk_data_set_get_frame_min_max_mem(ds);
  if (ds->frame_min != NULL)
    g_free(ds->frame_min);
  ds->frame_min = amitk_data_set_get_frame_min_max_mem(ds);
  
      
  if (ds->frame_duration != NULL)
    g_free(ds->frame_duration);
  if ((ds->frame_duration = amitk_data_set_get_frame_duration_mem(ds)) == NULL) {
    g_warning(_("couldn't allocate space for the frame duration info"));
    goto error;
  }

  if (ds->gate_time != NULL)
    g_free(ds->gate_time);
  if ((ds->gate_time = amitk_data_set_get_gate_time_mem(ds)) == NULL) {
    g_warning(_("couldn't allocate space for the gate time info"));
    goto error;
  }
  
      
  initial_offset = AMITK_SPACE_OFFSET(slice_ds);

  /* and process all the images */
  for (i_file=0; i_file < num_files; i_file++) {
    slice_ds = (AmitkDataSet *) g_list_nth_data(slices,i_file);

    x = div(i_file, dim.z);
    i=zero_voxel;
    if (num_gates > 1)
      i.g = x.quot;
    else
      i.t = x.quot;
    i.z = x.rem;
    transfer_slice(ds, slice_ds, i);

    /* record frame/gate duration if needed */
    if (i.z == 0) {
      amitk_data_set_set_frame_duration(ds, i.t, amitk_data_set_get_frame_duration(slice_ds, 0));
      amitk_data_set_set_gate_time(ds, i.g, amitk_data_set_get_gate_time(slice_ds, 0));
    }


    if (i_file != 0) {
	  
      /* figure out which direction the slices are going */
      if (i_file == 1) {
	
	offset = AMITK_SPACE_OFFSET(slice_ds);

	POINT_SUB(offset, initial_offset, diff);
	true_thickness = POINT_MAGNITUDE(diff);
	old_thickness = AMITK_DATA_SET_VOXEL_SIZE_Z(ds);
	if (true_thickness > EPSILON)

	  /* correct for differences in thickness */
	  if (!REAL_EQUAL(true_thickness, old_thickness)) {
	    voxel_size = AMITK_DATA_SET_VOXEL_SIZE(ds);
	    voxel_size.z = true_thickness;
	    amitk_data_set_set_voxel_size(ds, voxel_size);

	    /* whether to bother complaining */
	    if (!REAL_CLOSE(true_thickness, old_thickness))
	      screwed_up_thickness = TRUE;
	  }
      }

      
      //	  if (i_file != 0) {
      
      /* check if we've crossed a frame boundary */
      //	    if (!REAL_EQUAL(last_scan_start, AMITK_DATA_SET_SCAN_START(slice_ds))) {
      
      //	      if (is_dynamic) {
      //		num_frames++;
      //	    if (num_frames > 1) {
      
      //	      if (!REAL_EQUAL(AMITK_DATA_SET_SCAN_START(slice_ds)-last_scan_start, duration)) 
      //		screwed_up_timing=TRUE;
      
      //		duration = amitk_data_set_get_frame_duration(slice_ds, 0);
      //		g_array_append_val(durations, duration);
      
      //		last_scan_start = AMITK_DATA_SET_SCAN_START(slice_ds);
      //	      }
      //	    }
      //	  }


      /* check if the curtains match the rug */
      if ((i.z == 0) && (i.t > 0)) {
	if (!REAL_CLOSE(AMITK_DATA_SET_SCAN_START(slice_ds)-amitk_data_set_get_start_time(ds, i.t-1), 
			amitk_data_set_get_frame_duration(ds, i.t-1))) 
	  screwed_up_timing=TRUE;
      }
    }

    //    last_offset = AMITK_SPACE_OFFSET(slice_ds);

  } /* i_file loop */

  if (screwed_up_timing) 
    amitk_append_str_with_newline(perror_buf, _("Detected discontinous frames in data set %s - frame start times will be incorrect"), AMITK_OBJECT_NAME(ds));
  
  if (screwed_up_thickness)
    amitk_append_str_with_newline(perror_buf, _("Slice thickness (%5.3f mm) not equal to slice spacing (%5.3f mm) in data set %s - will use slice spacing for thickness"), old_thickness, true_thickness, AMITK_OBJECT_NAME(ds));
  
  /* detected a dynamic data set, massage the data appropriately */
  //	if (num_frames > 1) {
  //	  x = div(dim.z, num_frames);
  //	  if (x.rem != 0) {
  //	    g_warning(_("Cannot evenly divide the number of slices by the number of frames for data set %s - ignoring dynamic data"), AMITK_OBJECT_NAME(ds));
  //	  } else {
	
  
  /* hack the data set sizes */
  //	    ds->raw_data->dim.t = num_frames;
  //	    ds->raw_data->dim.z = x.quot;
  //
  //	    ds->internal_scaling_factor->dim.t = num_frames;
  //	    ds->internal_scaling_factor->dim.z = x.quot;
  //
  //	    ds->internal_scaling_intercept->dim.t = num_frames;
  //	    ds->internal_scaling_intercept->dim.z = x.quot;
  
  /* free the frame max/min arrays - will be realloc's by set_calc_min_max */
  //	    g_free(ds->frame_max);
  //	    ds->frame_max=NULL;
  //	    g_free(ds->frame_min);
  //	    ds->frame_min=NULL;
  
  /* and set the frame durations */
  //	    if (ds->frame_duration != NULL)
  //	      g_free(ds->frame_duration);
  //	    if ((ds->frame_duration = amitk_data_set_get_frame_duration_mem(ds)) == NULL) {
  //	      g_warning(_("couldn't allocate space for the frame duration info"));
  //	      amitk_object_unref(ds);
  //	      return NULL;
  //	    }
  //
  //	    for (j=0; j < AMITK_DATA_SET_NUM_FRAMES(ds); j++) 
  //	      ds->frame_duration[j] = g_array_index(durations,amide_time_t, j);
  //	  }
  //	}
  //	g_array_free(durations, TRUE);
  
  /* correct for a datset where the z direction is reveresed */
  //	if (reversed_z) {
  //	  void * buffer;
  //	  amide_data_t temp_val;
  
  //	  buffer = g_try_malloc(num_bytes_per_slice);
  //	  if (buffer == NULL) {
  //	    g_warning("could not malloc temp buffer");
  //	    goto error;
  //	  }
  
  
  //	  k = zero_voxel;
  //	  for (i.t = 0,k.t=0; i.t < AMITK_DATA_SET_DIM_T(ds); i.t++,k.t++) {
  //	    for (i.g = 0,k.g=0; i.g < AMITK_DATA_SET_DIM_G(ds); i.g++,k.g++) {
  //	      for (i.z = 0,k.z=AMITK_DATA_SET_DIM_Z(ds)-1; i.z < AMITK_DATA_SET_DIM_Z(ds); i.z++,k.z--) {
  
  /* exchange the slices*/
  //		ds_pointer = amitk_raw_data_get_pointer(AMITK_DATA_SET_RAW_DATA(ds), i);
  //		slice_pointer = amitk_raw_data_get_pointer(AMITK_DATA_SET_RAW_DATA(ds), k);
  //		memcpy(buffer, ds_pointer, num_bytes_per_slice);
  //		memcpy(ds_pointer, slice_pointer, num_bytes_per_slice);
  //		memcpy(slice_pointer, buffer, num_bytes_per_slice);
  //
  /* exchange the scale factors */
  //		temp_val = AMITK_RAW_DATA_DOUBLE_CONTENT(ds->internal_scaling_factor, i);
  //		AMITK_RAW_DATA_DOUBLE_SET_CONTENT(ds->internal_scaling_factor, i) =
  //		  AMITK_RAW_DATA_DOUBLE_CONTENT(ds->internal_scaling_factor, k);
  //		AMITK_RAW_DATA_DOUBLE_SET_CONTENT(ds->internal_scaling_factor, k) =
  //		  temp_val;
  
  /* exchange the scale offsets */
  //		temp_val = AMITK_RAW_DATA_DOUBLE_CONTENT(ds->internal_scaling_intercept, i);
  //		AMITK_RAW_DATA_DOUBLE_SET_CONTENT(ds->internal_scaling_intercept, i) =
  //		  AMITK_RAW_DATA_DOUBLE_CONTENT(ds->internal_scaling_intercept, k);
  //		AMITK_RAW_DATA_DOUBLE_SET_CONTENT(ds->internal_scaling_intercept, k) =
  //		  temp_val;
  //		
  //	      }
  //	    }
  //	  }
  //
  //	  g_free(buffer);
  //	}
  
  
  /* make sure remaining values have been calculated */
  amitk_data_set_set_scale_factor(ds, 1.0); /* set the external scaling factor */
  amitk_data_set_calc_far_corner(ds); /* set the far corner of the volume */
  amitk_data_set_calc_min_max(ds, update_func, update_data);
  
  goto end;

 error:
  if (ds != NULL) {
    amitk_object_unref(ds);
    ds = NULL;
  }

 end:

  return ds;
}



static GList * free_slices(GList * slices) {
  AmitkDataSet * slice_ds;
  
  while (slices != NULL) {
    slice_ds = AMITK_DATA_SET(g_list_nth_data(slices,0));
    slices = g_list_remove(slices, slice_ds);
    amitk_object_unref(slice_ds);
  }

  return slices;
}


 

static GList * organize_and_import_slices_as_datasets(GList ** premaining_slices, 
						      gint num_frames, 
						      gint num_gates,
						      gint num_slices,
						      AmitkUpdateFunc update_func,
						      gpointer update_data,
						      gchar **perror_buf) {

  GList * slices_to_combine = NULL;
  GList * returned_sets = NULL;
  GList * additional_sets = NULL;
  AmitkDataSet * ds=NULL;

  /* find all the slices that appear to match into one dataset */
  slices_to_combine = find_slices_that_match(premaining_slices);

  /* sort list based on the slice's time and z position */
  if (num_frames > 1)
    slices_to_combine = g_list_sort(slices_to_combine, sort_slices_func_with_time);
  else if (num_gates > 1)
    slices_to_combine = g_list_sort(slices_to_combine, sort_slices_func_with_gate);
  else
    slices_to_combine = g_list_sort(slices_to_combine, sort_slices_func);

  /* throw out any slices that are duplicated in terms of orientation */
  slices_to_combine = separate_duplicate_slices(slices_to_combine, premaining_slices);

  /* load in the primary data set */
  ds = import_slices_as_dataset(slices_to_combine, num_frames, num_gates, num_slices, update_func, update_data, perror_buf);
  if (ds != NULL)
    returned_sets = g_list_append(returned_sets, ds);
  free_slices(slices_to_combine);
  
  /* and recurse to try loading in any remaining data sets */
  if (*premaining_slices != NULL) {
    additional_sets = organize_and_import_slices_as_datasets(premaining_slices, num_frames, num_gates, num_slices, update_func, update_data, perror_buf);
    if (additional_sets != NULL)
      returned_sets = g_list_concat(returned_sets, additional_sets);
  }

  return returned_sets;
}

static GList * import_files_as_datasets(GList * image_files, 
					gchar ** pstudyname,
					AmitkPreferences * preferences, 
					AmitkUpdateFunc update_func,
					gpointer update_data,
					gchar **perror_buf) {


  GList * returned_sets=NULL;
  AmitkDataSet * slice_ds=NULL;
  div_t x;
  gchar * slice_name;
  gint image;
  gint num_frames=1;
  gint num_gates=1;
  gint num_slices=-1;
  gint num_files;
  GList * slices=NULL;
  gboolean continue_work=TRUE;
  gint divider;

  num_files = g_list_length(image_files);
  g_return_val_if_fail(num_files != 0, NULL);

  if (update_func != NULL) 
    continue_work = (*update_func)(update_data, _("Importing File(s) Through DCMTK"), (gdouble) 0.0);
  divider = (num_files/AMITK_UPDATE_DIVIDER < 1.0) ? 1 : (gint) rint(num_files/AMITK_UPDATE_DIVIDER);

  for (image=0; (image < num_files) && (continue_work); image++) {
    
    if (update_func != NULL) {
      x = div(image,divider);
      if (x.rem == 0)
	continue_work = (*update_func)(update_data, NULL, ((gdouble) image)/((gdouble) num_files));
    }


    slice_name = (gchar *) g_list_nth_data(image_files,image);

    slice_ds = read_dicom_file(slice_name, pstudyname,preferences, 
			       &num_frames, &num_gates, &num_slices, NULL, NULL, perror_buf);
    if (slice_ds == NULL) {
      goto cleanup;
    } else if ((AMITK_DATA_SET_DIM_Z(slice_ds) != 1) && (num_files > 1)) {
      /* can handle multiple dicom files each with a single slice, or one dicom file with multiple slices,
	 can't handle multiple files each with multiple slices */
      g_warning(_("no support for multislice files within DICOM directory format"));
      amitk_object_unref(slice_ds);
      slice_ds = NULL;
      goto cleanup;
    } 
    slices = g_list_append(slices, slice_ds);
  }
  if (!continue_work) goto cleanup;

  if ((num_frames > 1) && (num_gates > 1)) 
    g_warning("Don't know how to deal with multi-gate and multi-frame data, results will be undefined");


  returned_sets = organize_and_import_slices_as_datasets(&slices, num_frames, num_gates, num_slices, update_func, update_data, perror_buf);


 cleanup:
  if (update_func != NULL) /* remove progress bar */
    (*update_func) (update_data, NULL, (gdouble) 2.0); 

  slices = free_slices(slices);

  return returned_sets;
}

typedef struct slice_info_t {
  gchar * filename;
  gchar * series_instance_uid;
  gchar * modality;
  gchar * series_description;
  gchar * patient_id;
  gchar * patient_name;
  gint series_number;
} slice_info_t;

static void free_slice_info(slice_info_t * info) {

  if (info->filename != NULL)
    g_free(info->filename);

  if (info->series_instance_uid != NULL)
    g_free(info->series_instance_uid);

  if (info->modality != NULL)
    g_free(info->modality);
  
  if (info->series_description != NULL)
    g_free(info->series_description);

  if (info->patient_id != NULL)
    g_free(info->patient_id);

  if (info->patient_name != NULL)
    g_free(info->patient_name);

  g_free(info);

  return;
}

static slice_info_t * slice_info_new(void) {

  slice_info_t * info;

  info = g_try_new(slice_info_t, 1);
  info->filename=NULL;
  info->series_instance_uid=NULL;
  info->modality=NULL;
  info->series_description=NULL;
  info->patient_id=NULL;
  info->patient_name=NULL;
  info->series_number = -1;

  return info;
}


static slice_info_t * get_slice_info(const gchar * filename) {

  OFCondition result;
  DcmFileFormat dcm_format;
  DcmDataset * dcm_dataset;
  const char * return_str=NULL;
  slice_info_t * info=NULL;
  Sint32 return_sint32;

  result = dcm_format.loadFile(filename);
  if (result.bad()) return NULL;

  dcm_dataset = dcm_format.getDataset();
  if (dcm_dataset == NULL) return NULL;

  info = slice_info_new();
  info->filename = g_strdup(filename);

  dcm_dataset->findAndGetString(DCM_SeriesInstanceUID, return_str, OFTrue);
  if (return_str != NULL)
      info->series_instance_uid = g_strdup(return_str);

  dcm_dataset->findAndGetString(DCM_Modality, return_str, OFTrue);
  if (return_str != NULL)
      info->modality = g_strdup(return_str);

  dcm_dataset->findAndGetString(DCM_SeriesDescription, return_str, OFTrue);
  if (return_str != NULL) 
      info->series_description = g_strdup(return_str);

  dcm_dataset->findAndGetString(DCM_PatientID, return_str, OFTrue);
  if (return_str != NULL) 
      info->patient_id = g_strdup(return_str);

  dcm_dataset->findAndGetString(DCM_PatientName, return_str, OFTrue);
  if (return_str != NULL) 
      info->patient_name = g_strdup(return_str);

  if (dcm_dataset->findAndGetSint32(DCM_SeriesNumber, return_sint32).good())
    info->series_number = return_sint32;

  return info;
}

static gboolean check_str(gchar * str1, gchar * str2) {

  if ((str1 == NULL) && (str2 == NULL))
    return TRUE;
  if ((str1 == NULL) || (str2 == NULL))
    return FALSE;
  if (strcmp(str1, str2) == 0)
    return TRUE;

  return FALSE;
}

static gboolean check_same(slice_info_t * slice1, slice_info_t * slice2) {

  gint j, count;
  gchar ** slice1_uid_split=NULL;
  gchar ** slice2_uid_split=NULL;

  gboolean same_dataset;


  if ((slice1->series_instance_uid == NULL) || (slice2->series_instance_uid == NULL)) 
    return FALSE;
  else if ((slice1->series_instance_uid != NULL) && (slice2->series_instance_uid != NULL)) {
    slice1_uid_split = g_strsplit(slice1->series_instance_uid, ".", -1);
    slice2_uid_split = g_strsplit(slice2->series_instance_uid, ".", -1); 

    count=0;
    while (slice1_uid_split[count] != NULL) count++;

    /* check all but last number in series instance uid */
    same_dataset=TRUE;
    for (j=0; (j<(count-1) && same_dataset); j++) {
      if (slice2_uid_split[j] == NULL)
	same_dataset=FALSE;
      else if (strcmp(slice1_uid_split[j], slice2_uid_split[j]) != 0)
	same_dataset=FALSE;
    }

    if (slice1_uid_split != NULL) g_strfreev(slice1_uid_split);
    if (slice2_uid_split != NULL) g_strfreev(slice2_uid_split);

    if (!same_dataset) return FALSE;
  }


  if (!check_str(slice1->series_description, slice2->series_description))
    return FALSE;

  if (slice1->series_number != slice2->series_number)
    return FALSE;

  if (!check_str(slice1->modality, slice2->modality))
    return FALSE;

  if (!check_str(slice1->patient_id, slice2->patient_id)) {
    return FALSE;
  }

  if (!check_str(slice1->patient_name, slice2->patient_name)) {
    return FALSE;
  }

  return TRUE;
}

static gint sort_raw_info(gconstpointer a, gconstpointer b) {
  const slice_info_t * slicea = (slice_info_t *) a;
  const slice_info_t * sliceb = (slice_info_t *) b;

  /* sort by series if we've got the data */
  if ((slicea->series_number >=0) && (sliceb->series_number >=0)) {
    if (slicea->series_number > sliceb->series_number)
      return 1;
    else if (slicea->series_number < sliceb->series_number)
      return -1;
  }

  return strcmp(slicea->filename, sliceb->filename);
}

static gint sort_datasets_by_dicom_params(gconstpointer a, gconstpointer b) {

  g_return_val_if_fail(a != NULL, 0);
  g_return_val_if_fail(b != NULL, 0);

  /* first sort by series number */
  if (AMITK_DATA_SET_SERIES_NUMBER(a) > AMITK_DATA_SET_SERIES_NUMBER(b))
    return 1;
  else if (AMITK_DATA_SET_SERIES_NUMBER(a) < AMITK_DATA_SET_SERIES_NUMBER(b))
    return -1;

  /* then sort by echo time if we have that parameter */
  if (!isnan(AMITK_DATA_SET_ECHO_TIME(a)) && !isnan(AMITK_DATA_SET_ECHO_TIME(b))) {
    if (AMITK_DATA_SET_ECHO_TIME(a) > AMITK_DATA_SET_ECHO_TIME(b))
      return 1;
    else if (AMITK_DATA_SET_ECHO_TIME(a) < AMITK_DATA_SET_ECHO_TIME(b))
      return -1;
  }

  return 0;
}

static GList * import_files(const gchar * filename,
			    gchar ** pstudyname,
			    AmitkPreferences * preferences,
			    AmitkUpdateFunc update_func,
			    gpointer update_data) {


  AmitkDataSet * ds;
  GList * data_sets = NULL;
  GList * returned_sets = NULL;
  GList * image_files=NULL;
  GList * raw_info=NULL;
  GList * all_slices=NULL; /* list of lists */
  GList * temp_all_slices;
  GList * sorted_slices=NULL; /* pointer to a list in all_slices */
  gchar * image_name;
  gchar * error_buf=NULL;
  slice_info_t * info=NULL;
  slice_info_t * current_info=NULL;
  gchar * dirname=NULL;
  gchar * basename=NULL;
  gchar * regularized_filename;
  gchar * new_filename;
  DIR* dir;
  struct dirent* entry;
  gboolean continue_work=TRUE;
  gboolean all_datasets;
  gboolean use_this_one;
  GtkWidget * question;
  gint return_val;

  /* note, I generate a "regularized_filename" rather than just using filename, to insure that 
   when the filenames get sorted alphabetically, they're all of the same "./filename" form. */
  dirname = g_path_get_dirname(filename);
  basename = g_path_get_basename(filename);
  if (dirname == NULL)
    regularized_filename = g_strdup_printf("%s", filename);
  else
    regularized_filename = g_strdup_printf("%s%s%s", dirname, G_DIR_SEPARATOR_S,basename);

  /* get the intially requested file */
  info = get_slice_info(regularized_filename);
  if (info == NULL) {
    g_warning(_("could not find dataset in DICOM file %s\n"), regularized_filename);
    return NULL;
  }
  raw_info = g_list_append(raw_info, info); 

  /* ------- find all dicom files in the directory ------------ */
  if (update_func != NULL) 
    continue_work = (*update_func)(update_data, _("Scanning Files to find additional DICOM Slices"), (gdouble) 0.0);
    
  if ((dir = opendir(dirname))!=NULL) {
    while (((entry = readdir(dir)) != NULL) && (continue_work)) {

      if (update_func != NULL)
	continue_work = (*update_func)(update_data, NULL, -1.0);

      if (dirname == NULL)
	new_filename = g_strdup_printf("%s", entry->d_name);
      else
	new_filename = g_strdup_printf("%s%s%s", dirname, G_DIR_SEPARATOR_S,entry->d_name);

      if (strcmp(basename, entry->d_name) != 0) { /* we've already got the initial filename */
	if (dcmtk_test_dicom(new_filename)) {
	  info = get_slice_info(new_filename);
	  if (info != NULL) 
	    raw_info = g_list_append(raw_info, info); /* we have a match */
	}
      }

      g_free(new_filename);
    }
    if (dir != NULL) closedir(dir);
  }
  if (dirname != NULL) g_free(dirname);
  if (basename != NULL) g_free(basename);

  if (update_func != NULL) /* remove progress bar */
    (*update_func) (update_data, NULL, (gdouble) 2.0); 

  /* sort by series number, then filename */
  raw_info = g_list_sort(raw_info, sort_raw_info);

  /* ------------ sort the files ---------------- */
  while (raw_info != NULL) {
    current_info = (slice_info_t *) raw_info->data;
    raw_info = g_list_remove(raw_info, current_info);

    /* go through the list of slice lists, find one that matches */
    temp_all_slices = all_slices; 
    while ((temp_all_slices != NULL) && (current_info != NULL)){
      sorted_slices = (GList *) temp_all_slices->data;
      g_assert(sorted_slices->data != NULL);
      info = (slice_info_t *) sorted_slices->data;

      /* current slice matches the first slice in the current list, add it to this list */
      if (check_same(current_info, info)) {
	sorted_slices = g_list_append(sorted_slices, current_info);
	current_info = NULL;
      }

      temp_all_slices = temp_all_slices->next;
    }

    /* current info doesn't match anything, add it on as a new list */
    if (current_info != NULL) 
      all_slices = g_list_append(all_slices, g_list_append(NULL, current_info));
  }


  g_assert(all_slices != NULL);

  /* check if we want to load in everything or not */
  all_datasets=FALSE;
  if (g_list_length(all_slices) > 1) {
    /* make sure we really want to delete */
    question = gtk_message_dialog_new(NULL,
				      GTK_DIALOG_DESTROY_WITH_PARENT,
				      GTK_MESSAGE_QUESTION,
				      GTK_BUTTONS_YES_NO,
				      _("Multiple data sets were found in the same directory as the specified file. In addition to the dataset corresponding to the specified file, would you like to load in these additional datasets?"));
  
    /* and wait for the question to return */
    return_val = gtk_dialog_run(GTK_DIALOG(question));

    gtk_widget_destroy(question);
    if (return_val == GTK_RESPONSE_YES)
      all_datasets=TRUE;
  }

  while (all_slices != NULL) {
    sorted_slices = (GList *) all_slices->data;
    all_slices = g_list_remove(all_slices, sorted_slices);
    use_this_one = FALSE;

    /* transfer file names */
    image_files = NULL;
    while (sorted_slices != NULL) {
      info = (slice_info_t *) sorted_slices->data;

      /* see if this group of filenames contains the filename we initially started with */
      if (strcmp(regularized_filename, info->filename) == 0)
	use_this_one = TRUE;

      sorted_slices = g_list_remove(sorted_slices, info);
      image_files = g_list_append(image_files, info->filename);
      info->filename=NULL;
      free_slice_info(info);
    }

    if (all_datasets || (use_this_one)) {
      returned_sets = import_files_as_datasets(image_files, pstudyname, preferences, update_func, update_data, &error_buf);

      while (returned_sets != NULL) {
	ds = AMITK_DATA_SET(returned_sets->data);
	data_sets = g_list_append(data_sets, ds);
	returned_sets = g_list_remove(returned_sets, ds);
      }
    }

    /* cleanup */
    while (image_files != NULL) {
      image_name = (gchar *) g_list_nth_data(image_files,0);
      image_files = g_list_remove(image_files, image_name); 
      g_free(image_name);
    }
  }

  /* and sort datasets by series number, echo time, etc.*/
  data_sets = g_list_sort(data_sets, sort_datasets_by_dicom_params);

  /* cleanup */
  if (error_buf != NULL) {
    g_warning("%s",error_buf);
    g_free(error_buf);
  }

  if (regularized_filename != NULL)
    g_free(regularized_filename);

  return data_sets;
}




/* read in a DICOMDIR type file */
static GList * import_dir(const gchar * filename, 
			  gchar ** pstudyname,
			  AmitkPreferences * preferences,
			  AmitkUpdateFunc update_func,
			  gpointer update_data) {

  GList * data_sets=NULL;
  GList * returned_sets=NULL;
  DcmDirectoryRecord * dcm_root_record;
  DcmDirectoryRecord * patient_record=NULL;
  DcmDirectoryRecord * study_record=NULL;
  DcmDirectoryRecord * series_record=NULL;
  DcmDirectoryRecord * image_record=NULL;
  OFCondition result;

  AmitkDataSet * ds=NULL;
  const char * return_str=NULL;
  const char * record_name[3];
  const char * patient_id;
  gchar * dirname=NULL;
  gchar * object_name=NULL;
  gchar * temp_name;
  gchar * temp_name2;
  GList * image_files=NULL;
  gint j;

  gchar * error_buf=NULL;
  gchar * image_name1;
  gchar * image_name2;
  gchar * lowercase_image_name1=NULL;;
  gchar * lowercase_image_name2=NULL;;
  gboolean valid_filename;
  gboolean ignore_missing_files = FALSE;
  gboolean use_alternative;
  gboolean always_use_alternative = FALSE;

  GtkWidget * question;
  gint return_val;


  /* first try loading it as a DIRFILE */
  DcmDicomDir dcm_dir(filename);
  /*  dcm_dir.print(cout); */
  dirname = g_path_get_dirname(filename);

  dcm_root_record = &(dcm_dir.getRootRecord());
  /*  dcm_root_record->print(cout);*/

  /* go through the whole directory */
  while ((patient_record = dcm_root_record->nextSub(patient_record)) != NULL) {
    patient_record->findAndGetString(DCM_PatientName, record_name[0]);
    patient_record->findAndGetString(DCM_PatientID, patient_id, OFTrue);


    while ((study_record= patient_record->nextSub(study_record)) != NULL) {
      study_record->findAndGetString(DCM_StudyDescription, record_name[1]);

      while ((series_record = study_record->nextSub(series_record)) != NULL) {
	series_record->findAndGetString(DCM_SeriesDescription, record_name[2]);

	/* concat the names that aren't NULL */
	for (j=0; j<3; j++) {
	  if (record_name[j] != NULL) {
	    temp_name2 = g_strdup(record_name[j]);
	    g_strstrip(temp_name2);
	    if (object_name != NULL) {
	      temp_name = object_name;
	      object_name = g_strdup_printf("%s - %s", temp_name, temp_name2);
	      g_free(temp_name);
	    } else
	      object_name = g_strdup(temp_name2);
	    g_free(temp_name2);
	  }
	}

	while ((image_record=series_record->nextSub(image_record)) != NULL) {
	  if (image_record->findAndGetString(DCM_ReferencedFileID, return_str).good()) {
	    image_name1 = g_strdup(return_str);
	    g_strdelimit(image_name1, "\\", G_DIR_SEPARATOR); /* remove separators */
	    image_name2 = g_strdup_printf("%s%s%s", dirname, G_DIR_SEPARATOR_S,image_name1);

	    /* check if there's a upper vs lower case issue, seems to happen with
	       GE exported DICOMDIR files, likely due to some filesystems not being
	       case aware */
	    valid_filename = dcmtk_test_dicom(image_name2);
	    if ((!valid_filename) && (!ignore_missing_files)) {
	      use_alternative=FALSE;
	      lowercase_image_name1 = g_ascii_strdown(image_name1,-1);
	      lowercase_image_name2 = g_strdup_printf("%s%s%s", dirname, G_DIR_SEPARATOR_S,
						     lowercase_image_name1);
	      g_free(lowercase_image_name1);

	      if (!dcmtk_test_dicom(lowercase_image_name2)) {
		question = 
		  gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, 
					 GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
					 _("For series: %s\n\nListed in DICOMDIR: %s\n\nCould not read DICOM file: %s"),
					 object_name, filename, image_name2);
		gtk_dialog_add_button(GTK_DIALOG(question), GTK_STOCK_OK, GTK_RESPONSE_OK);
		gtk_dialog_add_button(GTK_DIALOG(question), 
				      _("OK, ignore similar errors"),1);
		return_val = gtk_dialog_run(GTK_DIALOG(question));
		gtk_widget_destroy(question);
		if (return_val == 1) ignore_missing_files = TRUE;
	      } else if (!always_use_alternative) {
		question = 
		  gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, 
					 GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
					 _("For series: %s\n\nListed in DICOMDIR: %s\n\nCould not read DICOM file: %s\n\nFound possible alternative file: %s"),
					 object_name, filename, image_name2, lowercase_image_name2);
		gtk_dialog_add_button(GTK_DIALOG(question), GTK_STOCK_OK, GTK_RESPONSE_OK);
		gtk_dialog_add_button(GTK_DIALOG(question), 
				      _("OK, ignore similar errors"), 1);
		gtk_dialog_add_button(GTK_DIALOG(question), 
				      _("Use alternative"), 2);
		gtk_dialog_add_button(GTK_DIALOG(question), 
				      _("Always use alternative"), 3);
		return_val = gtk_dialog_run(GTK_DIALOG(question));
		gtk_widget_destroy(question);
		
		if (return_val == 1) ignore_missing_files = TRUE;
		if (return_val == 2) use_alternative=TRUE;
		if (return_val == 3) always_use_alternative = TRUE;
	      }

	      if (use_alternative || always_use_alternative) {
		g_free(image_name2);
		image_name2=lowercase_image_name2;
		valid_filename=TRUE;
	      } else {
		g_free(lowercase_image_name2);
	      }
	    }

	    g_free(image_name1);
	    if (valid_filename)
	      image_files = g_list_append(image_files, image_name2);
	  }
	}

#if AMIDE_DEBUG
	g_print("object name: %s    with %d files (images)\n", object_name, g_list_length(image_files));
#endif

	/* read in slices as dataset(s) */
	if (g_list_length(image_files) != 0)
	  returned_sets = import_files_as_datasets(image_files, pstudyname, preferences, 
						   update_func, update_data, &error_buf);

	while (returned_sets != NULL) {
	  ds = AMITK_DATA_SET(returned_sets->data);
	  /* If the actual dicom file didn't have these entries, we try using the 
	     corresponding enteries from the DICOMDIR file */
	  if (AMITK_OBJECT_NAME(ds) == NULL) {
	    if (record_name[2] != NULL)
	      amitk_object_set_name(AMITK_OBJECT(ds), record_name[2]);
	    else 
	      amitk_object_set_name(AMITK_OBJECT(ds), object_name);
	  }

	  if (AMITK_DATA_SET_SUBJECT_NAME(ds) == NULL) {
	    if (record_name[0] != NULL)
	      amitk_data_set_set_subject_name(ds, record_name[0]);
	  }
	  if (AMITK_DATA_SET_SUBJECT_ID(ds) == NULL) {
	    if (patient_id != NULL)
	      amitk_data_set_set_subject_id(ds, patient_id);
	  }

	  data_sets = g_list_append(data_sets, ds);
	  returned_sets = g_list_remove(returned_sets, ds);
	}


	/* cleanup */
	while (image_files != NULL) {
	  image_name2 = (gchar *) g_list_nth_data(image_files,0);
	  image_files = g_list_remove(image_files, image_name2);
	  g_free(image_name2);
	}

	g_free(object_name);
	object_name=NULL;

      }
    }
  }

  /* and sort datasets by series number, echo time, etc.*/
  data_sets = g_list_sort(data_sets, sort_datasets_by_dicom_params);

  /* more cleanup */
  if (dirname != NULL)
    g_free(dirname);

  if (error_buf != NULL) {
    g_warning("%s",error_buf);
    g_free(error_buf);
  }

  return data_sets;
}

GList * dcmtk_import(const gchar * filename,
		     gchar ** pstudyname,
		     AmitkPreferences * preferences,
		     AmitkUpdateFunc update_func,
		     gpointer update_data) {

  GList * data_sets=NULL;
  gboolean is_dir=FALSE;
  DcmFileFormat dcm_format;
  DcmMetaInfo * dcm_metainfo;
  OFCondition result;
  const char * return_str=NULL;

  if (!dcmDataDict.isDictionaryLoaded()) {
    g_warning(_("Could not find a DICOM Data Dictionary.  Reading may fail.  Consider defining the environmental variable DCMDICTPATH to the dicom.dic file."));
  }

  result = dcm_format.loadFile(filename);
  if (result.bad()) {
    g_warning(_("could not read DICOM file %s, dcmtk returned %s"),filename, result.text());
    return NULL;
  }

  dcm_metainfo = dcm_format.getMetaInfo();
  if (dcm_metainfo == NULL) {
    g_warning("could not find metainfo for DICOM file %s\n", filename);
    return NULL;
  }

  dcm_metainfo->findAndGetString(DCM_MediaStorageSOPClassUID, return_str, OFTrue);
  if (return_str != NULL) {
    if (strcmp(return_str, UID_MediaStorageDirectoryStorage) == 0)
      is_dir= TRUE; /* we've got a DICOMDIR file */
  }


  if (is_dir) {
    data_sets = import_dir(filename, pstudyname, preferences, update_func, update_data);
  } else {
    data_sets = import_files(filename, pstudyname, preferences, update_func, update_data);
  }

  return data_sets;
}


static void insert_str(DcmItem * dcm_item, const DcmTag& tag, const gchar * str) {
  OFCondition status;

  status = dcm_item->putAndInsertString(tag, str);

  if (status.bad()) 
    g_error("Error %s: failed writing tag of type %s\n",
	    status.text(), tag.getVRName());
  return;
}

static void insert_int_str(DcmItem * dcm_item, const DcmTag& tag, gint value) {
  gchar * temp_str;

  temp_str = g_strdup_printf("%d",value);
  insert_str(dcm_item, tag, temp_str);
  g_free(temp_str);

  return;
}

static void insert_double_str(DcmItem * dcm_item, const DcmTag& tag, gdouble value) {
  gchar * temp_str;

  temp_str = g_strdup_printf("%f",value);
  insert_str(dcm_item,tag,temp_str);
  g_free(temp_str);

  return;
}

static void insert_double2_str(DcmItem * dcm_item, const DcmTag& tag, gdouble value1, gdouble value2) {
  gchar * temp_str;

  temp_str = g_strdup_printf("%f\\%f",value1,value2);
  insert_str(dcm_item,tag,temp_str);
  g_free(temp_str);

  return;
}

static void insert_double3_str(DcmItem * dcm_item, const DcmTag& tag, 
			       gdouble value1, gdouble value2, gdouble value3) {
  gchar * temp_str;

  temp_str = g_strdup_printf("%f\\%f\\%f",value1,value2,value3);
  insert_str(dcm_item,tag,temp_str);
  g_free(temp_str);

  return;
}

static void insert_double6_str(DcmItem * dcm_item, const DcmTag& tag, 
			       gdouble value1, gdouble value2, gdouble value3,
			       gdouble value4, gdouble value5, gdouble value6) {
  gchar * temp_str;

  temp_str = g_strdup_printf("%f\\%f\\%f\\%f\\%f\\%f",
			     value1,value2,value3,value4,value5,value6);
  insert_str(dcm_item,tag,temp_str);
  g_free(temp_str);

  return;
}

/* take a stab at converting the study date and frame information into strings appropriate 
   for the dicom header. date_str and time_str will have to be free'd */
static void generate_dicom_date_and_time(const gchar * date, const amide_time_t frame_offset, 
					 gchar ** pdate_str, gchar ** ptime_str) {

  struct tm time_structure;
  time_t time_in_seconds;
  gboolean valid = FALSE;

  if(date != NULL) {
#ifdef HAVE_STRPTIME
    valid = (strptime(date,"%c",&time_structure) != NULL);
    if (!valid) /* try a less locale specific format string */
      valid = (strptime(date,"%a %b %d %T %Y",&time_structure) != NULL);
#endif
  }

  if (!valid) { 
    time_structure.tm_year = 0; /* 1900 */
    time_structure.tm_mon = 0; /* January */
    time_structure.tm_mday = 1; /* first */
    time_structure.tm_hour = 0;
    time_structure.tm_min = 0;
    time_structure.tm_sec = 0;
  }

  time_structure.tm_isdst = -1; /* make mktime figure out daylight savings times issues */

  time_in_seconds = mktime(&time_structure); /* convert the dates to seconds since 1970 */
  time_in_seconds += frame_offset; /* add the frame offset time */

#if !defined(G_PLATFORM_WIN32)
  localtime_r(&time_in_seconds, &time_structure);
#else /* win32 doesn't have localtime_r */
  {
    struct tm *ptm = localtime (&time_in_seconds);

    if (ptm != NULL) 
      memcpy ((void *) &time_structure, (void *) ptm, sizeof(struct tm));
    else
      valid = FALSE;
  }
#endif
  


  *ptime_str = g_strdup_printf("%02d%02d%02d",
			       time_structure.tm_hour,
			       time_structure.tm_min,
			       time_structure.tm_sec);
  
  if (valid) 
    *pdate_str = g_strdup_printf("%04d%02d%02d",
				 time_structure.tm_year+1900,
				 time_structure.tm_mon+1,
				 time_structure.tm_mday);
    
  else if (date != NULL)
    *pdate_str = g_strdup(date); /* if we couldn't figure out the date string, just propogate it */
  else
    *pdate_str = g_strdup("19000101");

  return;
}



/* dirname is usually the name of the directory that the dicomdir file will go into,
   it can also be the name of a preexisting dicomdir file, in which case the
   exported data will be appended */
gboolean dcmtk_export(AmitkDataSet * ds, 
		      const gchar * dir_or_filename,
		      const gchar * studyname,
		      const gboolean resliced,
		      const AmitkPoint resliced_voxel_size,
		      const AmitkVolume * bounding_box,
		      AmitkUpdateFunc update_func,
		      gpointer update_data) {

  DicomDirInterface dcm_dir;
  DcmDataset * dcm_ds;
  DcmFileFormat dcm_format;
  DcmMetaInfo * dcm_metainfo;
  DcmItem * dcm_item;
  char uid[100];
  OFCondition status;
  struct stat file_info;
  gchar * subdirname=NULL;
  gchar * full_subdirname=NULL;
  gchar * filename=NULL;
  gchar * dirname=NULL;
  gchar * dcmdir_filename=NULL;
  gchar * full_filename=NULL;
  gchar * temp_str=NULL;
  gint i;
  AmitkVolume * output_volume=NULL;
  AmitkDataSet * slice = NULL;
  AmitkVoxel dim;
  AmitkPoint corner;
  AmitkVoxel i_voxel, j_voxel;
  gboolean format_changing;
  gboolean format_size_short;
  gpointer buffer = NULL;
  amitk_format_SSHORT_t * sshort_buffer;
  gdouble min,max;
  gint buffer_size;
  div_t x;
  gint divider;
  gint total_planes;
  gboolean continue_work=TRUE;
  AmitkPoint output_start_pt;
  AmitkCanvasPoint pixel_size;
  AmitkPoint new_offset;
  amide_data_t value;
  gint image_num;
  gchar * saved_time_locale;
  gchar * saved_numeric_locale;
  AmitkAxes axes;
  AmitkPoint dicom_offset;
  AmitkPoint voxel_size;
  gboolean successful = FALSE;
  gchar * date_str;
  gchar * time_str;

  saved_time_locale = g_strdup(setlocale(LC_TIME,NULL));
  saved_numeric_locale = g_strdup(setlocale(LC_NUMERIC,NULL));
  setlocale(LC_TIME,"POSIX");  
  setlocale(LC_NUMERIC,"POSIX");  

  /* figure out if we've been given a directory, or a DICOMDIR filename */
  /* very simplistic, we assume the directory file will be called DICOMDIR
     if it's anything else, we assume we've been given a directory to create a new DICOMDIR file in */
  dcmdir_filename = g_path_get_basename(dir_or_filename);
  if (g_strcmp0(dcmdir_filename, "DICOMDIR") == 0) 
    if (dcmtk_test_dicom(dir_or_filename)) 
      dirname = g_path_get_dirname(dir_or_filename);

  /* otherwise */
  if (dirname == NULL) {
    dirname = g_strdup(dir_or_filename);
  }

  if (dcmdir_filename != NULL) g_free(dcmdir_filename);
  dcmdir_filename = g_strdup_printf("%s/DICOMDIR",dirname);

  /* make the directory we'll output the dicom data in if it doesn't already exist */
  if (stat(dirname, &file_info) == 0) {
    if (!S_ISDIR(file_info.st_mode)) {
      g_warning(_("File already exists with name: %s"), dirname);
      goto cleanup;
    }

  } else {
    if (g_mkdir(dirname,0766) != 0) {
      g_warning(_("Couldn't create directory: %s"),dirname);
      goto cleanup;
    }
  }


  /* create the DICOMDIR, unless already made in which case we'll append to it */
  if (stat(dcmdir_filename,&file_info) == 0) {
    if (!S_ISREG(file_info.st_mode)) {
      g_warning(_("Existing DICOMDIR file %s is not a regular file"),dcmdir_filename);
      goto cleanup;
    }
    status = dcm_dir.appendToDicomDir(DicomDirInterface::AP_Default,dcmdir_filename);
    if (status.bad()) {
      g_warning(_("Existing DICOMDIR file %s exists but is not appendable, error %s"),dcmdir_filename, status.text());
      goto cleanup;
    }
  } else {
    status = dcm_dir.createNewDicomDir(DicomDirInterface::AP_Default,dcmdir_filename);
    if (status.bad()) {
      g_warning(_("Could not create DICOMDIR file %s, error %s"),dcmdir_filename, status.text());
      goto cleanup;
    }
  }



  /* find a unique directory name */
  i=0;
  temp_str = NULL;
  do {
    if (subdirname != NULL) g_free(subdirname);
    if (full_subdirname != NULL) g_free(full_subdirname);
    subdirname = g_strdup_printf("DCM%03d", i);
    full_subdirname = g_strdup_printf("%s%s%s",dirname,G_DIR_SEPARATOR_S,subdirname);
    i++;
  } while (stat(full_subdirname, &file_info) == 0);

#if AMIDE_DEBUG
  g_print("selected subdirname %s\n", subdirname);
#endif

  if (g_mkdir(full_subdirname,0766) != 0) {
    g_warning(_("Couldn't create directory: %s"),full_subdirname);
  }


  /* figure out our dimensions */
  dim = AMITK_DATA_SET_DIM(ds);
  if (resliced) {
    if (bounding_box != NULL) 
      output_volume = AMITK_VOLUME(amitk_object_copy(AMITK_OBJECT(bounding_box)));
    else
      output_volume = amitk_volume_new();
    if (output_volume == NULL) goto cleanup;

    if (bounding_box != NULL) {
      corner = AMITK_VOLUME_CORNER(output_volume);
    } else {
      AmitkCorners corners;
      amitk_volume_get_enclosing_corners(AMITK_VOLUME(ds), AMITK_SPACE(output_volume), corners);
      corner = point_diff(corners[0], corners[1]);
      amitk_space_set_offset(AMITK_SPACE(output_volume), 
			     amitk_space_s2b(AMITK_SPACE(output_volume), corners[0]));
    }

    voxel_size = resliced_voxel_size;
    pixel_size.x = voxel_size.x;
    pixel_size.y = voxel_size.y;
    dim.x = (amide_intpoint_t) ceil(corner.x/voxel_size.x);
    dim.y = (amide_intpoint_t) ceil(corner.y/voxel_size.y);
    dim.z = (amide_intpoint_t) ceil(corner.z/voxel_size.z);
    corner.z = voxel_size.z;
#ifdef AMIDE_DEBUG
    g_print("output dimensions %d %d %d, voxel size %f %f %f\n", dim.x, dim.y, dim.z, voxel_size.x, voxel_size.y, voxel_size.z);
#else
    g_warning(_("dimensions of output data set will be %dx%dx%d, voxel size of %fx%fx%f"), dim.x, dim.y, dim.z, voxel_size.x, voxel_size.y, voxel_size.z);
#endif
  } else {
    output_volume = amitk_volume_new();
    amitk_space_copy_in_place(AMITK_SPACE(output_volume), AMITK_SPACE(ds));
    voxel_size = AMITK_DATA_SET_VOXEL_SIZE(ds);
    corner = AMITK_VOLUME_CORNER(ds);
  }
  amitk_volume_set_corner(output_volume, corner);
  output_start_pt = AMITK_SPACE_OFFSET(output_volume);


  /* create the dicom data structure */
  dcm_ds = dcm_format.getDataset();
  dcm_metainfo = dcm_format.getMetaInfo();
  dcm_metainfo->putAndInsertString(DCM_SOPClassUID, UID_SecondaryCaptureImageStorage);

  /* required dicom entries */
  insert_str(dcm_ds,DCM_SeriesInstanceUID,dcmGenerateUniqueIdentifier(uid, SITE_INSTANCE_UID_ROOT));

  /* if we're appending to a DICOMDIR, it would be a little better to copy the studyinstanceuid 
     and frameofreferenceuid from whatever DICOM files are already stored... but it's simpler to 
     just regenerate these values */
  insert_str(dcm_ds,DCM_StudyInstanceUID, dcmGenerateUniqueIdentifier(uid, SITE_INSTANCE_UID_ROOT));
  insert_str(dcm_ds,DCM_FrameOfReferenceUID, dcmGenerateUniqueIdentifier(uid, SITE_INSTANCE_UID_ROOT));
	     
  insert_str(dcm_ds, DCM_StudyID,"0");
  if (AMITK_DATA_SET_SERIES_NUMBER(dcm_ds) > 0)
    insert_int_str(dcm_ds, DCM_SeriesNumber, AMITK_DATA_SET_SERIES_NUMBER(dcm_ds));
  else
    insert_str(dcm_ds, DCM_SeriesNumber,"0");

  /* other stuff */
  if (AMITK_DATA_SET_SUBJECT_NAME(ds) != NULL) 
    insert_str(dcm_ds,DCM_PatientName, AMITK_DATA_SET_SUBJECT_NAME(ds));
  if (AMITK_DATA_SET_SUBJECT_ID(ds) != NULL) 
    insert_str(dcm_ds,DCM_PatientID, AMITK_DATA_SET_SUBJECT_ID(ds));
  if (AMITK_DATA_SET_SUBJECT_DOB(ds) != NULL) 
    insert_str(dcm_ds,DCM_PatientBirthDate, AMITK_DATA_SET_SUBJECT_DOB(ds));

  /* set StudyDescription */
  if (studyname != NULL)
    insert_str(dcm_ds,DCM_StudyDescription, studyname);
  else
    insert_str(dcm_ds,DCM_StudyDescription, AMITK_OBJECT_NAME(ds));

  /* set SeriesDescription */
  if (AMITK_OBJECT_NAME(ds) != NULL) 
    insert_str(dcm_ds,DCM_SeriesDescription, AMITK_OBJECT_NAME(ds));
  insert_double_str(dcm_ds,DCM_PatientWeight, AMITK_DATA_SET_SUBJECT_WEIGHT(ds));


  switch(AMITK_DATA_SET_SUBJECT_SEX(ds)) {
  case AMITK_SUBJECT_SEX_MALE:
    insert_str(dcm_ds, DCM_PatientSex, "M");
    break;
  case AMITK_SUBJECT_SEX_FEMALE:
    insert_str(dcm_ds, DCM_PatientSex, "F");
    break;
  case AMITK_SUBJECT_SEX_UNKNOWN:
  default:
    insert_str(dcm_ds, DCM_PatientSex, "O");
    break;
  }

  switch(AMITK_DATA_SET_SUBJECT_ORIENTATION(ds)) {
  case AMITK_SUBJECT_ORIENTATION_SUPINE_HEADFIRST:
    insert_str(dcm_ds,DCM_PatientPosition, "HFS"); 
    break;
  case AMITK_SUBJECT_ORIENTATION_SUPINE_FEETFIRST:
    insert_str(dcm_ds,DCM_PatientPosition, "FFS"); 
    break;
  case AMITK_SUBJECT_ORIENTATION_PRONE_HEADFIRST:
    insert_str(dcm_ds,DCM_PatientPosition, "HFP"); 
    break;
  case AMITK_SUBJECT_ORIENTATION_PRONE_FEETFIRST:
    insert_str(dcm_ds,DCM_PatientPosition, "FFP"); 
    break;
  case AMITK_SUBJECT_ORIENTATION_RIGHT_DECUBITUS_HEADFIRST:
    insert_str(dcm_ds,DCM_PatientPosition, "HFDR"); 
    break;
  case AMITK_SUBJECT_ORIENTATION_RIGHT_DECUBITUS_FEETFIRST:
    insert_str(dcm_ds,DCM_PatientPosition, "FFDR"); 
    break;
  case AMITK_SUBJECT_ORIENTATION_LEFT_DECUBITUS_HEADFIRST:
    insert_str(dcm_ds,DCM_PatientPosition, "HFDL"); 
    break;
  case AMITK_SUBJECT_ORIENTATION_LEFT_DECUBITUS_FEETFIRST:
    insert_str(dcm_ds,DCM_PatientPosition, "FFDL"); 
    break;
  case AMITK_SUBJECT_ORIENTATION_UNKNOWN:
  default:
    break;
  }

  switch(AMITK_DATA_SET_MODALITY(ds)) {
  case AMITK_MODALITY_PET:
    insert_str(dcm_ds,DCM_Modality, "PT");
    dcm_metainfo->putAndInsertString(DCM_MediaStorageSOPClassUID,
				     UID_PositronEmissionTomographyImageStorage);
    break;
  case AMITK_MODALITY_SPECT:
    insert_str(dcm_ds,DCM_Modality, "ST");
    dcm_metainfo->putAndInsertString(DCM_MediaStorageSOPClassUID,
				     UID_NuclearMedicineImageStorage);
    break;
  case AMITK_MODALITY_CT:
    insert_str(dcm_ds,DCM_Modality, "CT");
    dcm_metainfo->putAndInsertString(DCM_MediaStorageSOPClassUID,
				     UID_CTImageStorage);
    break;
  case AMITK_MODALITY_MRI:
    insert_str(dcm_ds,DCM_Modality, "MR");
    dcm_metainfo->putAndInsertString(DCM_MediaStorageSOPClassUID,
				     UID_MRImageStorage);
    break;
  case AMITK_MODALITY_OTHER:
  default:
    insert_str(dcm_ds,DCM_Modality, "OT");
    dcm_metainfo->putAndInsertString(DCM_MediaStorageSOPClassUID,
				     UID_SecondaryCaptureImageStorage);
    break;
  }


  /* modality specific items */
  switch(AMITK_DATA_SET_MODALITY(ds)) {
  case AMITK_MODALITY_PET:
  case AMITK_MODALITY_SPECT:
    /* create a radiopharmaceutical information sequence if we have the data */
    if (!isnan(AMITK_DATA_SET_INJECTED_DOSE(ds))) {
      if (dcm_ds->findOrCreateSequenceItem(DCM_RadiopharmaceuticalInformationSequence, dcm_item, 0).good()) {
	insert_double_str(dcm_item, DCM_RadionuclideTotalDose, AMITK_DATA_SET_INJECTED_DOSE(ds));
      }
    }
    insert_str(dcm_ds, DCM_DecayCorrection, "START"); /* assume things were decay corrected from the start of the scan */
    break;
  case AMITK_MODALITY_MRI:
    if (!isnan(AMITK_DATA_SET_INVERSION_TIME(ds))) 
      insert_double_str(dcm_ds, DCM_InversionTime, AMITK_DATA_SET_INVERSION_TIME(ds));
    if (!isnan(AMITK_DATA_SET_ECHO_TIME(ds))) 
      insert_double_str(dcm_ds, DCM_EchoTime, AMITK_DATA_SET_ECHO_TIME(ds));
    if (!isnan(AMITK_DATA_SET_DIFFUSION_B_VALUE(ds))) {
      insert_double_str(dcm_ds, DCM_DiffusionBValue, AMITK_DATA_SET_DIFFUSION_B_VALUE(ds));
      dcm_ds->putAndInsertFloat64(DCM_DiffusionGradientOrientation, 
				  AMITK_DATA_SET_DIFFUSION_DIRECTION(ds).x, 0, OFTrue);
      dcm_ds->putAndInsertFloat64(DCM_DiffusionGradientOrientation, 
				  AMITK_DATA_SET_DIFFUSION_DIRECTION(ds).y, 1, OFTrue);
      dcm_ds->putAndInsertFloat64(DCM_DiffusionGradientOrientation, 
				  AMITK_DATA_SET_DIFFUSION_DIRECTION(ds).z, 2, OFTrue);
    }
    break;
  default:
    break;
  }

  dcm_ds->putAndInsertUint16(DCM_Columns, dim.x);
  dcm_ds->putAndInsertUint16(DCM_Rows, dim.y);
  dcm_ds->putAndInsertUint16(DCM_NumberOfSlices, dim.z); /* number of planes per frame, not per file */
  /* retired entry  dcm_ds->putAndInsertUint16(DCM_Planes, 1); */ /* we save one plane per file */

  /* past in the image type if we have that info*/
  if (AMITK_DATA_SET_DICOM_IMAGE_TYPE(ds) != NULL) {
    dcm_metainfo->putAndInsertString(DCM_ImageType,
  				     AMITK_DATA_SET_DICOM_IMAGE_TYPE(ds));
  } else {
    /* technically, ImageType is a required entry, so we should probably guess something */
    dcm_metainfo->putAndInsertString(DCM_ImageType,
  				     "DERIVED\\SECONDARY");
  }

  /* tag required for PET data, but we don't currently keep track of the data set underlying units */
  dcm_ds->putAndInsertString(DCM_Units, "NONE");

  /* add some dicom required stuff */
  dcm_ds->putAndInsertString(DCM_PhotometricInterpretation, "MONOCHROME2");
  dcm_ds->putAndInsertUint16(DCM_SamplesPerPixel, 1); /* all our data is monochrome, 3 would be rgb */

  /* if gated study, put in the gate information */
  if (AMITK_DATA_SET_DIM_G(ds) > 1) {
    insert_str(dcm_ds,DCM_SeriesType, "GATED\\IMAGE");
    dcm_ds->putAndInsertUint16(DCM_NumberOfTimeSlots, AMITK_DATA_SET_DIM_G(ds)); /* used for PET */
    dcm_ds->putAndInsertSint32(DCM_NumberOfTemporalPositions, AMITK_DATA_SET_DIM_G(ds)); /* used for MR */
  }
  
  if (AMITK_DATA_SET_DIM_T(ds) > 1)
    insert_str(dcm_ds,DCM_SeriesType, "DYNAMIC");
  dcm_ds->putAndInsertUint16(DCM_NumberOfTimeSlices, AMITK_DATA_SET_DIM_T(ds));

  /* figure out the format we'll be saving in */
  if (!resliced) {
    switch (AMITK_DATA_SET_FORMAT(ds)) {
    case AMITK_FORMAT_SBYTE:
    case AMITK_FORMAT_UBYTE:
      format_changing=FALSE;
      format_size_short=FALSE;
      break;
    case AMITK_FORMAT_SSHORT:
    case AMITK_FORMAT_USHORT:
      format_changing=FALSE;
      format_size_short=TRUE;
      break;
    case AMITK_FORMAT_FLOAT:
    case AMITK_FORMAT_DOUBLE:
    case AMITK_FORMAT_SINT:
    case AMITK_FORMAT_UINT:
      format_changing=TRUE; /* well save as signed shorts */
      format_size_short=TRUE;
      break;
    default:
      format_changing=TRUE; 
      format_size_short=TRUE;
      g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
      break;
    }
  } else {
    format_changing=TRUE; 
    format_size_short=TRUE;
  }

  dcm_ds->putAndInsertUint16(DCM_BitsAllocated,
			     format_changing ? 
			     8*amitk_format_sizes[AMITK_FORMAT_SSHORT] :
			     8*amitk_format_sizes[AMITK_DATA_SET_FORMAT(ds)]);
  dcm_ds->putAndInsertUint16(DCM_BitsStored,
			     format_changing ? 
			     8*amitk_format_sizes[AMITK_FORMAT_SSHORT] :
			     8*amitk_format_sizes[AMITK_DATA_SET_FORMAT(ds)]);
  dcm_ds->putAndInsertUint16(DCM_HighBit,
			     format_changing ? 
			     8*amitk_format_sizes[AMITK_FORMAT_SSHORT]-1 :
			     8*amitk_format_sizes[AMITK_DATA_SET_FORMAT(ds)]-1);
  dcm_ds->putAndInsertUint16(DCM_PixelRepresentation,
			     format_changing ?
			     amitk_format_signed[AMITK_FORMAT_SSHORT] :
			     amitk_format_signed[AMITK_DATA_SET_FORMAT(ds)]);
  buffer_size = dim.y*dim.x*(format_changing ? 
			     amitk_format_sizes[AMITK_FORMAT_SSHORT] :
			     amitk_format_sizes[AMITK_DATA_SET_FORMAT(ds)]);

  insert_double2_str(dcm_ds, DCM_PixelSpacing,voxel_size.y,voxel_size.x);

  insert_double_str(dcm_ds, DCM_SpacingBetweenSlices, voxel_size.z);
  insert_double_str(dcm_ds, DCM_SliceThickness, voxel_size.z);

  /* take a stab at converting the study date and putting it into the dicom header */
  generate_dicom_date_and_time(AMITK_DATA_SET_SCAN_DATE(ds), 0.0, &date_str, &time_str);
  insert_str(dcm_ds, DCM_StudyDate, date_str);
  insert_str(dcm_ds, DCM_SeriesDate, date_str);
  insert_str(dcm_ds, DCM_AcquisitionDate, date_str);
  g_free(date_str);
      
  insert_str(dcm_ds, DCM_StudyTime, time_str);
  insert_str(dcm_ds, DCM_SeriesTime, time_str);
  insert_str(dcm_ds, DCM_AcquisitionTime, time_str);
  insert_str(dcm_ds, DCM_RadiopharmaceuticalStartTime,time_str); /* dose already corrected to start time */
  g_free(time_str);

  /* put in orientation - see note in read_dicom_file about AMIDE versus DICOM orientation */
  /* do flips for dicom convention */
  amitk_axes_copy_in_place(axes, AMITK_SPACE_AXES(output_volume));
  axes[AMITK_AXIS_X].y *= -1.0;
  axes[AMITK_AXIS_X].z *= -1.0;
  axes[AMITK_AXIS_Y].y *= -1.0;
  axes[AMITK_AXIS_Y].z *= -1.0;
  insert_double6_str(dcm_ds, DCM_ImageOrientationPatient,
		     axes[AMITK_AXIS_X].x,axes[AMITK_AXIS_X].y,axes[AMITK_AXIS_X].z,
		     axes[AMITK_AXIS_Y].x,axes[AMITK_AXIS_Y].y,axes[AMITK_AXIS_Y].z);


  /* and finally, get to the real work */
  if (update_func != NULL) {
    temp_str = g_strdup_printf(_("Exporting File Through DCMTK:\n   %s"), dirname);
    continue_work = (*update_func)(update_data, temp_str, (gdouble) 0.0);
    g_free(temp_str);
  }
  total_planes = dim.z*dim.g*dim.t;
  divider = ((total_planes/AMITK_UPDATE_DIVIDER) < 1) ? 1 : (gint) rint(total_planes/AMITK_UPDATE_DIVIDER);

  image_num=0;
  j_voxel = zero_voxel;
  i_voxel = zero_voxel;
  for (i_voxel.t = 0; (i_voxel.t < dim.t) && (continue_work); i_voxel.t++) {

    insert_int_str(dcm_ds, DCM_ActualFrameDuration,
	       (int) (1000*amitk_data_set_get_frame_duration(ds,i_voxel.t))); /* into ms */
    insert_double_str(dcm_ds,DCM_FrameReferenceTime,
		      1000.0*amitk_data_set_get_start_time(ds,i_voxel.t));

    /* ContentDate/Time is actually when the pixel data is generated, not when the acquisition is started... so
       technically the below is wrong. Only including this because ContentDate/Time is apparently a Type 1 (required)
       parameter. */
    generate_dicom_date_and_time(AMITK_DATA_SET_SCAN_DATE(ds), amitk_data_set_get_start_time(ds, i_voxel.t), &date_str, &time_str);
    insert_str(dcm_ds, DCM_ContentDate, date_str);
    insert_str(dcm_ds, DCM_ContentTime, time_str);
    g_free(date_str);
    g_free(time_str);

    for (i_voxel.g = 0 ; (i_voxel.g < dim.g) && (continue_work); i_voxel.g++) {

      /* if gated study, write in gate info */
      if (AMITK_DATA_SET_DIM_G(ds) > 1) {
	insert_int_str(dcm_ds, DCM_TemporalPositionIdentifier, i_voxel.g+1); /* starts at 1, not 0 */
	insert_double_str(dcm_ds,DCM_TriggerTime,
			  1000.0*amitk_data_set_get_gate_time(ds,i_voxel.g)); /* into ms */
      }

      /* reset the output slice */
      amitk_space_set_offset(AMITK_SPACE(output_volume), output_start_pt);

      for (i_voxel.z = 0 ; (i_voxel.z < dim.z) && (continue_work); i_voxel.z++, image_num++) {

	if (update_func != NULL) {
	  x = div(image_num,divider);
	  if (x.rem == 0)
	    continue_work = (*update_func)(update_data, NULL, ((gdouble) image_num)/((gdouble) total_planes));

	}
	
	/* get our transfer buffer */
	buffer = g_try_malloc0(buffer_size);
	if (buffer == NULL) {
	  g_warning(_("Could not malloc transfer buffer"));
	  goto cleanup;
	}

	/* set things up if resliced or format changing */
	if (resliced) {
	  slice = amitk_data_set_get_slice(ds, 
					   amitk_data_set_get_start_time(ds,i_voxel.t), 
					   amitk_data_set_get_frame_duration(ds,i_voxel.t), 
					   i_voxel.g, pixel_size, output_volume);

	  if ((AMITK_DATA_SET_DIM_X(slice) != dim.x) || (AMITK_DATA_SET_DIM_Y(slice) != dim.y)) {
	    g_warning(_("Error in generating resliced data, %dx%d != %dx%d"),
		      AMITK_DATA_SET_DIM_X(slice), AMITK_DATA_SET_DIM_Y(slice),
		      dim.x, dim.y);
	    goto cleanup;
	  }

	  min = amitk_data_set_get_global_min(slice);
	  max = amitk_data_set_get_global_max(slice);

	} else if (format_changing) { 
	  amitk_data_set_slice_calc_min_max(ds,
					    i_voxel.t,
					    i_voxel.g,
					    i_voxel.z,
					    &min, &max);
	} 

	/* transfer into the new buffer */
	if (format_changing) {
	  max = MAX(fabs(min), max);
	  sshort_buffer = (amitk_format_SSHORT_t *) buffer;
	  i=0;

	  for (i_voxel.y=0, j_voxel.y=0; i_voxel.y < dim.y; i_voxel.y++, j_voxel.y++)
	    for (i_voxel.x=0, j_voxel.x = 0; i_voxel.x < dim.x; i_voxel.x++, j_voxel.x++,i++) {
	      if (resliced) 
		value = AMITK_DATA_SET_DOUBLE_0D_SCALING_CONTENT(slice, j_voxel);
	      else
		value = amitk_data_set_get_value(ds, i_voxel);
	      sshort_buffer[i] = (amitk_format_SSHORT_t) rint(amitk_format_max[AMITK_FORMAT_SSHORT]*value/max);
	    }

	} else { /* short or char type */
	  i_voxel.y = 0;
	  i_voxel.x = 0;
	  memcpy(buffer, amitk_raw_data_get_pointer(AMITK_DATA_SET_RAW_DATA(ds), i_voxel), buffer_size);
	}

	/* convert to little endian */
	/* note, ushort conversion is exactly the same as sshort, so the below works */
	if (format_size_short) {
	  i=0;
	  sshort_buffer = (amitk_format_SSHORT_t *) buffer;
	  for (i_voxel.y=0; i_voxel.y < dim.y; i_voxel.y++)
	    for (i_voxel.x=0; i_voxel.x < dim.x; i_voxel.x++,i++) 
	      sshort_buffer[i] = GINT16_TO_LE(sshort_buffer[i]);
	}

	/* and store */
	dcm_ds->putAndInsertUint8Array(DCM_PixelData, (Uint8*) buffer, buffer_size);

	dcm_ds->putAndInsertUint16(DCM_ImageIndex, image_num);

	/* store the scaling factor and offset */
	insert_double_str(dcm_ds,DCM_RescaleSlope, 
			  (resliced || format_changing) ? 
			  max/amitk_format_max[AMITK_FORMAT_SSHORT] :
			  amitk_data_set_get_scaling_factor(ds,i_voxel));

	insert_double_str(dcm_ds,DCM_RescaleIntercept, 
			  (resliced || format_changing) ? 0.0 :
			  amitk_data_set_get_scaling_intercept(ds,i_voxel));
	
	/* and get it an identifier */
	dcmGenerateUniqueIdentifier(uid, SITE_INSTANCE_UID_ROOT);
	insert_str(dcm_ds,DCM_SOPInstanceUID,uid);
	
	/* set the meta info as well */
	dcm_metainfo->putAndInsertString(DCM_MediaStorageSOPInstanceUID,uid);

	/* and some other data */
	insert_int_str(dcm_ds,DCM_InstanceNumber, image_num);

	/* and save it's position in space - 
	   see note in dicom_read_file about DICOM versus AMIDE conventions */
	dicom_offset = AMITK_SPACE_OFFSET(output_volume);
	dicom_offset.y = -1.0*dicom_offset.y; /* DICOM specifies y axis in wrong direction */
	dicom_offset.z = -1.0*dicom_offset.z; /* DICOM specifies z axis in wrong direction */
	insert_double3_str(dcm_ds, DCM_ImagePositionPatient,
			   dicom_offset.x,dicom_offset.y, dicom_offset.z);
	insert_double_str(dcm_ds, DCM_SliceLocation, dicom_offset.z);

	/* and write it on out */
	filename = g_strdup_printf("%s%sIMG%05d",subdirname,G_DIR_SEPARATOR_S,image_num);
	full_filename = g_strdup_printf("%s%s%s",dirname, G_DIR_SEPARATOR_S,filename);
	status = dcm_format.saveFile(full_filename, EXS_LittleEndianExplicit);
	if (status.bad()) {
	  g_warning(_("couldn't write out file %s, error %s"), full_filename, status.text());
	  goto cleanup;
	}

	/* add it to the DICOMDIR file */
	status = dcm_dir.addDicomFile(filename,dirname);
	if (status.bad()) {
	  g_warning(_("couldn't append file %s to DICOMDIR %s, error %s"), filename, dirname, status.text());
	  goto cleanup;
	}


	/* cleanups */
	if (filename != NULL) {
	  g_free(filename);
	  filename = NULL;
	}

	if (full_filename != NULL) {
	  g_free(full_filename);
	  full_filename = NULL;
	}

	if (buffer != NULL) {
	  g_free(buffer);
	  buffer = NULL;
	}

	if (slice != NULL) 
	  slice = AMITK_DATA_SET(amitk_object_unref(slice));

	/* advance for next iteration */
	new_offset = zero_point;
	new_offset.z += voxel_size.z;
	amitk_space_set_offset(AMITK_SPACE(output_volume), 
			       amitk_space_s2b(AMITK_SPACE(output_volume), new_offset));

      } /* i_voxel.z */
    } /* i_voxel.g */
  } /* i_voxel.t */

  status = dcm_dir.writeDicomDir();
  if (status.bad()) {
    g_warning(_("Failed to write DICOMDIR file, error %s\n"), status.text());
    goto cleanup;
  }

  successful = TRUE; /* whew! made it */

 cleanup:

  if (output_volume != NULL)
    output_volume = AMITK_VOLUME(amitk_object_unref(output_volume));

  if (slice != NULL)
    slice = AMITK_DATA_SET(amitk_object_unref(slice));
  
  if (buffer != NULL)
    g_free(buffer);

  if (filename != NULL)
    g_free(filename);

  if (full_filename != NULL)
    g_free(full_filename);

  if (subdirname != NULL)
    g_free(subdirname);

  if (full_subdirname != NULL)
    g_free(full_subdirname);

  if (dirname != NULL)
    g_free(dirname);

  if (dcmdir_filename != NULL)
    g_free(dcmdir_filename);

  if (update_func != NULL) /* remove progress bar */
    (*update_func)(update_data, NULL, (gdouble) 2.0); 

  setlocale(LC_NUMERIC, saved_time_locale);
  setlocale(LC_NUMERIC, saved_numeric_locale);
  g_free(saved_time_locale);
  g_free(saved_numeric_locale);

  return successful;
}

#endif /* AMIDE_LIBDCMDATA_SUPPORT */
