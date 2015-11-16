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
#include <assert.h>



gboolean vistaio_test_vista(gchar *filename)
{
  return VistaIOIsVistaFile(filename); 
}


static VistaIOBoolean vistaio_get_pixel_signedness(VistaIOImage image);
static VistaIOBoolean vistaio_get_3dvector(VistaIOImage image, const gchar* name, AmitkPoint *voxel);
static VistaIOBoolean vistaio_get_rotation(VistaIOImage image, AmitkAxes matrix); 
static VistaIOBoolean vistaio_get_voxelsize_from2d(VistaIOImage  image, AmitkPoint *voxel); 

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


typedef struct _IOUpdate IOUpdate; 
struct _IOUpdate {
  AmitkUpdateFunc update_func; 
  gpointer update_data;
  gboolean cont;
  gchar *message; 
};



static void AmitkIOShowProgress(int pos, int length, void *data) 
{
  IOUpdate *update; 
  assert(data);
  update = (IOUpdate *)data;
  if (update->update_func) {
    update->cont = (*update->update_func)(update->update_data, update->message, ((gdouble) pos)/((gdouble) length));
  }
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
  AmitkAxes new_axes; 
  
  VistaIOImage *images = NULL;
  VistaIOAttrList attr_list = NULL;
  VistaIORepnKind in_pixel_repn = VistaIOUnknownRepn;
  VistaIOBoolean is_pixel_unsigned = FALSE;

  VistaIOBoolean origin_found = FALSE;
  VistaIOBoolean voxelsize_found = FALSE;
  VistaIOBoolean rotation_found = FALSE;
  
  int nimages = 0; 
  gboolean images_good = TRUE; 
  gboolean convert = FALSE; 

  gint format_size;
  gint bytes_per_image;
  gint i, t;
  void *ds_pointer = NULL;
  IOUpdate update; 
  
  /* first read the file */
  FILE *f = fopen(filename, "rb");

  if (!f)  {
    g_warning(_("Can't open file %s "), filename);
    goto error;
  }

  update.update_func = update_func; 
  update.update_data = update_data;
  update.cont = TRUE;

  if (update_func != NULL) {
    update.message = g_strdup_printf(_("Loading file %s"), filename);
  }

  
  VistaIOSetProgressIndicator(AmitkIOShowProgress, AmitkIOShowProgress, &update); 
  
  nimages = VistaIOReadImages(f, &attr_list, &images);
  
  if (update_func != NULL) {
    g_free(update.message);
    (*update_func)(update_data, NULL, (gdouble) 2.0); 
  }
  
  if (!update.cont) {
    g_debug("User interrupt");
    goto error1; 
  }
  
  // a vista file?
  if (!nimages) {
    g_warning(_("File %s doesn't contain vista image(s)"), filename);
    goto error; 
  }

  /* amide data sets don't seem to support images of different sizes
     so here we check that all images are of the same type and dimensions */

  /* get proto type from first image  */
  dim.x = VistaIOImageNColumns(images[0]);
  dim.y = VistaIOImageNRows(images[0]);
  dim.z = VistaIOImageNBands(images[0]);
  in_pixel_repn = VistaIOPixelRepn(images[0]);
  is_pixel_unsigned = vistaio_get_pixel_signedness(images[0]);

  voxelsize_found = vistaio_get_3dvector(images[0], "voxel", &voxel);

  
  if (!voxelsize_found) {
    voxelsize_found = vistaio_get_voxelsize_from2d(images[0], &voxel);
  }
  
  origin_found = vistaio_get_3dvector(images[0], "origin3d", &origin); 
  if (!origin_found) 
    origin_found = vistaio_get_3dvector(images[0], "position", &origin);

  
  rotation_found = vistaio_get_rotation(images[0], new_axes);
  
  g_debug("Prototype image is: %dx%dx%d, repn:%d(%d), origin: (%f %f %f), voxelsize (%f %f %f)",
	  dim.x, dim.y, dim.z,
	  in_pixel_repn, is_pixel_unsigned,
	  origin.x, origin.y, origin.z, 
	  voxel.x, voxel.y, voxel.z); 
  
  /* If there are more than one images, compare with the proto type */
  images_good = TRUE;
  
  for (i = 1; i < nimages && images_good; ++i) {
    AmitkPoint voxel_i = {1,1,1};
    AmitkPoint origin_i = {0,0,0};
    VistaIOBoolean origin_found_i = FALSE;
        
    images_good &= dim.x == VistaIOImageNColumns(images[i]);
    images_good &= dim.y == VistaIOImageNRows(images[i]);
    images_good &= dim.z == VistaIOImageNBands(images[i]);
    images_good &= in_pixel_repn == VistaIOPixelRepn(images[i]);
    images_good &= is_pixel_unsigned == vistaio_get_pixel_signedness(images[i]);
    
    vistaio_get_3dvector(images[i], "voxel", &voxel_i);
    origin_found_i = vistaio_get_3dvector(images[i], "origin3d", &origin_i);
    if (!origin_found_i)
      origin_found_i = vistaio_get_3dvector(images[i], "position", &origin_i);
    
    images_good &= voxel.x == voxel_i.x;
    images_good &= voxel.y == voxel_i.y;
    images_good &= voxel.z == voxel_i.z;

    images_good &= origin.x == origin_i.x;
    images_good &= origin.y == origin_i.y;
    images_good &= origin.z == origin_i.z; 
    
  }

  if (!images_good) {
    g_warning("File %s containes more than one image of different type, scaling, or dimensions, only reading first",
	      filename);
    dim.t = 1; 
  } else {
    dim.t = nimages; 
  }
  dim.g = 1;

	  /* the origin is read directly from DICOM therefore we emulate what the dcmtk reader is doing */
  origin.y *= -1.0; /* DICOM specifies y axis in wrong direction */
  origin.z *= -1.0; /* DICOM specifies z axis in wrong direction */


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
    /* Double valued images need to be converted to plain float*/
  case VistaIODoubleRepn:
    format = AMITK_FORMAT_FLOAT;
    convert = TRUE;
    break;
    /* Binary images are read as ubyte, since vista also stores them like this in memory */
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
    g_warning(_("Couldn't allocate memory space for the data set structure to hold Vista data"));
    goto error;
  }
  bytes_per_image = format_size * dim.x * dim.y * dim.z;

  /* Just use the file name, we can figure out somethink more sophisticated later*/
  amitk_object_set_name(AMITK_OBJECT(ds),filename);

  if (rotation_found) 
    amitk_space_set_axes(AMITK_SPACE(ds), new_axes, zero_point);

  /* This is just informal information */
  amitk_data_set_set_subject_orientation(ds, AMITK_SUBJECT_ORIENTATION_UNKNOWN);
  amitk_data_set_set_scan_date(ds, "Unknown"); 



  amitk_space_set_offset(AMITK_SPACE(ds), origin);
  amitk_data_set_set_voxel_size(ds, voxel);

  
  
  ds->scan_start = 0.0;
  
  g_debug("Now copying data: %d bytes per %d image(s)", bytes_per_image, dim.t); 
  /* now load the data */
  if (!convert) {
    ds_pointer = (char*)amitk_raw_data_get_pointer(AMITK_DATA_SET_RAW_DATA(ds), zero_voxel);
    for (t = 0; t < dim.t; ++t, ds_pointer += bytes_per_image) {
      memcpy(ds_pointer, VistaIOPixelPtr(images[t],0,0,0), bytes_per_image);
      amitk_data_set_set_frame_duration(ds, t, 1.0);
      g_debug("Copied image %d", t); 
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

static VistaIOBoolean vistaio_get_3dvector(VistaIOImage image, const gchar* attribute_name, AmitkPoint *voxel)
{
  VistaIOString voxel_string;
  float x,y,z;
  
  if (VistaIOGetAttr (VistaIOImageAttrList(image), attribute_name, NULL, VistaIOStringRepn, 
		      &voxel_string) != VistaIOAttrFound) {
    g_debug("VistaIO: Attribute '%s' not found in image", attribute_name); 
    return FALSE;
  }

  if (sscanf(voxel_string, "%f %f %f", &x, &y, &z) != 3) {
    g_debug("VistaIO: The string '%s' could not be parsed properly to obtain three float values", voxel_string);
    return FALSE;
  }else {
    voxel->x = x;
    voxel->y = y;
    voxel->z = z; 
  }
  return TRUE;
}

static VistaIOBoolean vistaio_get_voxelsize_from2d(VistaIOImage  image, AmitkPoint *voxel)
{
  VistaIOString pixel_string;
  float x, y;
  VistaIOFloat slice_thickness, spacing;
  VistaIOBoolean thickness_found = FALSE; 
  VistaIOBoolean spacing_found = FALSE; 
  
  if (VistaIOGetAttr (VistaIOImageAttrList(image), "pixel", NULL, VistaIOStringRepn, 
		      &pixel_string) != VistaIOAttrFound) {
    g_debug("VistaIO: Attribute 'pixel' not found in image"); 
    return FALSE;
  }
  
  if (sscanf(pixel_string, "%f %f", &x, &y) != 2) {
    g_debug("VistaIO: The string '%s' could not be parsed properly to obtain two float values", pixel_string);
    return FALSE;
  }else {
    voxel->x = x;
    voxel->y = y;
  }

  thickness_found = VistaIOGetAttr (VistaIOImageAttrList(image), "SliceThickness", NULL, VistaIOFloatRepn, 
				    &slice_thickness);
  
  spacing_found = VistaIOGetAttr (VistaIOImageAttrList(image), "SpacingBetweenSlices", NULL, VistaIOFloatRepn, 
				  &spacing); 
  
  if (spacing_found) {
    voxel->z = spacing; 
    if (thickness_found) {
      if (slice_thickness != spacing)
	g_debug("Slice thickness %f != slice spacing %f, use spacing", slice_thickness, spacing); 
    }
  } else if (thickness_found) {
    voxel->z = slice_thickness; 
  } else {
    return FALSE; 
  }
  return TRUE; 
}

static VistaIOBoolean vistaio_get_rotation(VistaIOImage image, AmitkAxes matrix)
{
  int name_length = 0; 
  char *splitname = NULL; 
  VistaIOString voxel_string; 
  if (VistaIOGetAttr (VistaIOImageAttrList(image), "rotation3d", NULL, VistaIOStringRepn, 
		      &voxel_string) != VistaIOAttrFound) {
    g_debug("VistaIO: Attribute 'rotation3d' not found in image"); 
    return FALSE;
  }

  /* Split the input data */
  splitname = strchr(voxel_string, '=');
  if (!splitname) {
    if (strcmp(voxel_string, "rot-identity")) {
      matrix[AMITK_AXIS_X].x = 1;
      matrix[AMITK_AXIS_X].y = 0;
      matrix[AMITK_AXIS_X].z = 0;

      matrix[AMITK_AXIS_Y].x = 0;
      matrix[AMITK_AXIS_Y].y = 1;
      matrix[AMITK_AXIS_Y].z = 0;

      matrix[AMITK_AXIS_Z].x = 0;
      matrix[AMITK_AXIS_Z].y = 0;
      matrix[AMITK_AXIS_Z].z = 1;
      return TRUE; 
    }else{
      g_debug("VistaIO: Unexpected 'rotation3d' attribute '%s'",  voxel_string); 
      return FALSE; 
    }
  }
  
  name_length = splitname - voxel_string;
  ++splitname; 

  g_debug("Found 'rotation3d' as '%s' witgh indicator length %d", voxel_string, name_length); 
  
  if (!strncmp(voxel_string, "rot-matrix", name_length)) {
    float xx, xy,xz, yx, yy, yz, zx, zy, zz;
    if (sscanf(splitname,"%f,%f,%f;%f,%f,%f;%f,%f,%f",
	       &xx, &xy, &xz, &yx, &yy, &yz, &zx, &zy, &zz) != 9) {
      g_debug("VistaIO: The string '%s' could not be parsed properly "
	      "to obtain nine float values for the 'rotation3d' matrix", splitname);
      return FALSE;
    }
    matrix[AMITK_AXIS_X].x = xx;
    matrix[AMITK_AXIS_X].y = -xy;
    matrix[AMITK_AXIS_X].z = -xz;
    
    matrix[AMITK_AXIS_Y].x = yx;
    matrix[AMITK_AXIS_Y].y = -yy;
    matrix[AMITK_AXIS_Y].z = -yz;
    
    matrix[AMITK_AXIS_Z].x = zx;
    matrix[AMITK_AXIS_Z].y = -zy;
    matrix[AMITK_AXIS_Z].z = -zz;
    return TRUE;
    
  } else if  (!strncmp(voxel_string, "rot-quaternion", name_length)) {
    float x, y, z, w;
    double a2, b2, c2, d2, bc, ad, bd, ac, cd, ab; 
    
    if (sscanf(splitname,"%f,%f,%f,%f",
	       &w, &x, &y, &z) != 4) {
      g_debug("VistaIO: The string '%s' could not be parsed properly "
	      "to obtain four float values for the 'rotation3d' quaternion", splitname);
      return FALSE; 
    }
    /* Evaluate the matrix from the quaternion */

    a2 = w * w;
    b2 = x * x;
    c2 = y * y;
    d2 = z * z;
    
    bc = 2.0 * x * y; 
    ad = 2.0 * w * z; 
    bd = 2.0 * x * z; 
    ac = 2.0 * w * y; 
    cd = 2.0 * y * z; 
    ab = 2.0 * w * x; 

    
    matrix[AMITK_AXIS_X].x = a2 + b2 - c2 - d2;
    matrix[AMITK_AXIS_X].y = -(bc - ad);
    matrix[AMITK_AXIS_X].z = -(bd + ac);
    
    matrix[AMITK_AXIS_Y].x = bc + ad;
    matrix[AMITK_AXIS_Y].y = -(a2 - b2 + c2 - d2);
    matrix[AMITK_AXIS_Y].z = -(cd - ab);
    
    matrix[AMITK_AXIS_Z].x = bd - ac;
    matrix[AMITK_AXIS_Z].y = -(cd + ab);
    matrix[AMITK_AXIS_Z].z = -(a2 - b2 - c2 + d2);
    return TRUE;
    
  } else {
    g_debug("VistaIO: Unexpected 'rotation3d' attribute '%s'",  voxel_string);
    return FALSE; 
  }
  
  
}


#endif /* AMIDE_VISTAIO_SUPPORT */
