/* -*- c-basic-offset: 2;  -*-  
 * vistaio_interface.c
 * 
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2015 Andy Loening
 *
 * Author: Gert Wollny <gw.fossdev@gmail.com>
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
#ifdef AMIDE_VISTAIO_SUPPORT

#include "amitk_data_set.h"
#include "amitk_data_set_DOUBLE_0D_SCALING.h"
#include "vistaio_interface.h"
#include <vistaio.h>
#include <string.h>
#include <locale.h>

gboolean vistaio_test_vista(gchar *filename)
{
  return VistaIOIsVistaFile(filename); 
}


static VistaIOBoolean vistaio_get_pixel_signedness(VistaIOImage image);
static void vistaio_get_3dvector(VistaIOImage image, const gchar* name, AmitkPoint *voxel); 
static AmitkDataSet * do_vistaio_import(const gchar * filename, 
					AmitkPreferences * preferences,
					AmitkUpdateFunc update_func,
					gpointer update_data); 

AmitkDataSet * vistaio_import(const gchar * filename, 
			     AmitkPreferences * preferences,
			     AmitkUpdateFunc update_func,
			     gpointer update_data)
{
  AmitkDataSet *retval = NULL; 
	
  gchar * saved_time_locale;
  gchar * saved_numeric_locale;

  saved_time_locale = g_strdup(setlocale(LC_TIME,NULL));
  saved_numeric_locale = g_strdup(setlocale(LC_NUMERIC,NULL));
  
  setlocale(LC_TIME,"POSIX");  
  setlocale(LC_NUMERIC,"POSIX");

  retval = do_vistaio_import(filename, preferences, update_func, update_data);
    
  setlocale(LC_NUMERIC, saved_time_locale);
  setlocale(LC_NUMERIC, saved_numeric_locale);
  g_free(saved_time_locale);
  g_free(saved_numeric_locale);
  return retval; 
  
}


AmitkDataSet * do_vistaio_import(const gchar * filename, 
				 AmitkPreferences * preferences,
				 AmitkUpdateFunc update_func,
				 gpointer update_data)
{

  AmitkVoxel dim;
  AmitkFormat format;
  AmitkPoint voxel = {1, 1, 1};
  AmitkPoint origin = {0, 0, 0}; 
  AmitkModality modality = AMITK_MODALITY_MRI;
  AmitkDataSet * ds=NULL;
  
  VistaIOImage *images = NULL;
  VistaIOAttrList attr_list = NULL;
  VistaIORepnKind in_pixel_repn = VistaIOUnknownRepn;
  VistaIOBoolean is_pixel_unsigned = FALSE; 
  int nimages = 0; 
  gboolean images_good = TRUE; 
  gboolean convert = FALSE; 

  gint format_size;
  gint bytes_per_image;
  gint i, t;
  void *ds_pointer = NULL; 
  
  /* first read the file */
  FILE *f = fopen(filename, "rb");

  /* This shouldn't happen at this point ...*/
  if (!f)  {
    g_warning(_("Can't open file %s "), filename);
    goto error;
  }

  nimages = VistaIOReadImages(f, &attr_list, &images);
  // a vista file?
  if (!nimages) {
    g_warning(_("File %s doesn't contain vista images"), filename);
    goto error; 
  }

  /* amide data sets don't seem to support images of different sizes
     so here we check that ll images are of the same type and dimensions */
  

  /* get proto type from first image  */
  dim.x = VistaIOImageNColumns(images[0]);
  dim.y = VistaIOImageNRows(images[0]);
  dim.z = VistaIOImageNBands(images[0]);
  in_pixel_repn = VistaIOPixelRepn(images[0]);
  is_pixel_unsigned = vistaio_get_pixel_signedness(images[0]);
  vistaio_get_3dvector(images[0], "voxel", &voxel);
  vistaio_get_3dvector(images[0], "origin3d", &origin); 

  /* If there are more than one images, compare with the proto type */
  images_good = TRUE;
  
  for (i = 1; i < nimages && images_good; ++i) {
    AmitkPoint voxel_i = {1,1,1};
    AmitkPoint origin_i = {0,0,0};
    
    images_good &= dim.x == VistaIOImageNColumns(images[i]);
    images_good &= dim.y == VistaIOImageNRows(images[i]);
    images_good &= dim.z == VistaIOImageNBands(images[i]);
    images_good &= in_pixel_repn == VistaIOPixelRepn(images[i]);
    images_good &= is_pixel_unsigned == vistaio_get_pixel_signedness(images[i]);
    
    vistaio_get_3dvector(images[i], "voxel", &voxel_i);
    vistaio_get_3dvector(images[i], "origin3d", &origin_i); 

    images_good &= voxel.x == voxel_i.x;
    images_good &= voxel.y == voxel_i.y;
    images_good &= voxel.z == voxel_i.z;

    images_good &= origin.x == origin_i.x;
    images_good &= origin.y == origin_i.y;
    images_good &= origin.z == origin_i.z; 
    
  }

  if (!images_good) {
    g_warning("File %s containes more than one image of different type or dimensions, only reading first",
	      filename);
    dim.t = 1; 
  } else {
    dim.t = nimages; 
  }
  dim.g = 1;

  /* Now we can create the images */
    /* pick our internal data format */
  switch(in_pixel_repn) {
  case VistaIOSByteRepn:
    format = AMITK_FORMAT_SBYTE;
    break;
  case VistaIOUByteRepn:
    format = AMITK_FORMAT_UBYTE;
    break;
  case VistaIOShortRepn:
    format = is_pixel_unsigned ? AMITK_FORMAT_USHORT : AMITK_FORMAT_SSHORT; 
    break;
  case VistaIOLongRepn: /* 7 */
    format = is_pixel_unsigned ? AMITK_FORMAT_UINT : AMITK_FORMAT_SINT; 
    break;
  case VistaIOFloatRepn:
    format = AMITK_FORMAT_FLOAT;
    break;
  case VistaIODoubleRepn:
    format = AMITK_FORMAT_FLOAT;
    convert = TRUE;
    break;
  case VistaIOBitRepn:
    format = AMITK_FORMAT_UBYTE;
    break;
    
  default:
    g_warning(_("Importing data type %d file through VistaIO unsupported in AMIDE, discarding"),  
	      in_pixel_repn);
    goto error1;  
  }
  
  format_size = amitk_format_sizes[format];
  ds = amitk_data_set_new_with_data(preferences, modality, format, dim, AMITK_SCALING_TYPE_2D_WITH_INTERCEPT);
  if (ds == NULL) {
    g_warning(_("Couldn't allocate memory space for the data set structure to hold (X)MedCon data"));
    goto error;
  }
  bytes_per_image = format_size * dim.x * dim.y * dim.z;

  ds->voxel_size = voxel;

  /* Just use the file name, we can figure out somethink more sophisticated later*/
  amitk_object_set_name(AMITK_OBJECT(ds),filename);

  /* todo: get the orientation from the file, sometimes it is stored there */
  amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_UNKNOWN);

  amitk_data_set_set_scan_date(ds, "Unknown"); 
  amitk_space_set_offset(AMITK_SPACE(ds), origin);

  ds->scan_start = 0.0;
  
  g_message("Now copying data: %d bytes per %d image(s)", bytes_per_image, dim.t); 
  /* now load the data */
  if (!convert) {
    ds_pointer = (char*)amitk_raw_data_get_pointer(AMITK_DATA_SET_RAW_DATA(ds), zero_voxel);
    for (t = 0; t < dim.t; ++t, ds_pointer += bytes_per_image) {
      memcpy(ds_pointer, VistaIOPixelPtr(images[t],0,0,0), bytes_per_image);
      amitk_data_set_set_frame_duration(ds, t, 1.0);
      g_message("Copied image %d\n", t); 
    }
  } else {
    // conversion is always from double to float
    int pixel_per_image = dim.x * dim.y * dim.z;
    int p; 
    float * ds_float = (float*)amitk_raw_data_get_pointer(AMITK_DATA_SET_RAW_DATA(ds), zero_voxel);
    for (t = 0; t < dim.t; ++t) {
      double *in_ptr = (double* )VistaIOPixelPtr(images[t],0,0,0);
      for (p = 0; p < pixel_per_image; ++p, ++ds_float, ++in_ptr)
	*ds_float = *in_ptr;
      amitk_data_set_set_frame_duration(ds, t, 1.0);
    } 
  }
  
  amitk_data_set_set_scale_factor(ds, 1.0); /* set the external scaling factor */
  amitk_data_set_calc_far_corner(ds); /* set the far corner of the volume */
  amitk_data_set_calc_min_max(ds, update_func, update_data);

  
error1:
  for (i = 0; i < nimages; ++i) {
    VistaIODestroyImage(images[i]); 
  }
  
  VistaIOFree(images);
  VistaIODestroyAttrList (attr_list);
  
error:
  if (f)
    fclose(f);
  g_message("Leaving vista import\n"); 
  return ds; 
}

  
static VistaIOBoolean vistaio_get_pixel_signedness(VistaIOImage image)
{
  VistaIOBoolean result = 0;
  if (VistaIOAttrFound == VistaIOGetAttr (VistaIOImageAttrList(image), "repn-unsigned", NULL, VistaIOBitRepn, 
					  &result))
    return result;
  else
    return 0; 
}

static void vistaio_get_3dvector(VistaIOImage image, const gchar* attribute_name, AmitkPoint *voxel)
{
  VistaIOString voxel_string; 
  if (VistaIOGetAttr (VistaIOImageAttrList(image), attribute_name, NULL, VistaIOStringRepn, 
		      &voxel_string) != VistaIOAttrFound) {
    g_warning("VistaIO: Attribute '%s' not found in image\n", attribute_name); 
    return;
  }

  if (sscanf(voxel_string, "%f %f %f", &voxel->x, &voxel->y, &voxel->z) != 3) {
    g_warning("VistaIO: The string '%s' could not be parsed properly to obtain three float values");
  }
  
}


#endif /* AMIDE_VISTAIO_SUPPORT */
