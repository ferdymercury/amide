/* amitk_data_set.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2003 Andy Loening
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

#include "amitk_data_set.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"

/* variable type function declarations */
#include "amitk_data_set_UBYTE_0D_SCALING.h"
#include "amitk_data_set_UBYTE_1D_SCALING.h"
#include "amitk_data_set_UBYTE_2D_SCALING.h"
#include "amitk_data_set_SBYTE_0D_SCALING.h"
#include "amitk_data_set_SBYTE_1D_SCALING.h"
#include "amitk_data_set_SBYTE_2D_SCALING.h"
#include "amitk_data_set_USHORT_0D_SCALING.h"
#include "amitk_data_set_USHORT_1D_SCALING.h"
#include "amitk_data_set_USHORT_2D_SCALING.h"
#include "amitk_data_set_SSHORT_0D_SCALING.h"
#include "amitk_data_set_SSHORT_1D_SCALING.h"
#include "amitk_data_set_SSHORT_2D_SCALING.h"
#include "amitk_data_set_UINT_0D_SCALING.h"
#include "amitk_data_set_UINT_1D_SCALING.h"
#include "amitk_data_set_UINT_2D_SCALING.h"
#include "amitk_data_set_SINT_0D_SCALING.h"
#include "amitk_data_set_SINT_1D_SCALING.h"
#include "amitk_data_set_SINT_2D_SCALING.h"
#include "amitk_data_set_FLOAT_0D_SCALING.h"
#include "amitk_data_set_FLOAT_1D_SCALING.h"
#include "amitk_data_set_FLOAT_2D_SCALING.h"
#include "amitk_data_set_DOUBLE_0D_SCALING.h"
#include "amitk_data_set_DOUBLE_1D_SCALING.h"
#include "amitk_data_set_DOUBLE_2D_SCALING.h"

#include <string.h>
#include <sys/time.h>
#include <time.h>
#ifdef AMIDE_DEBUG
#include <sys/timeb.h>
#endif
#include "raw_data_import.h"
#include "cti_import.h"
#include "medcon_import.h"


/* external variables */
AmitkColorTable amitk_modality_default_color_table[AMITK_MODALITY_NUM] = {
  AMITK_COLOR_TABLE_NIH, /* PET */
  AMITK_COLOR_TABLE_HOT_METAL, /* SPECT */
  AMITK_COLOR_TABLE_BW_LINEAR, /* CT */
  AMITK_COLOR_TABLE_BW_LINEAR, /* MRI */
  AMITK_COLOR_TABLE_BW_LINEAR /* OTHER */
};

gchar * amitk_interpolation_explanations[] = {
  N_("interpolate using nearest neighbhor (fast)"), 
  N_("interpolate using trilinear interpolation (slow)")
};


gchar * amitk_import_menu_names[] = {
  "", /* place holder for AMITK_IMPORT_METHOD_GUESS */
  N_("_Raw Data"), 
#ifdef AMIDE_LIBECAT_SUPPORT
  N_("_ECAT 6/7 via libecat"),
#endif
#ifdef AMIDE_LIBMDC_SUPPORT
  "" /* place holder for AMITK_IMPORT_METHOD_LIBMDC */
#endif
};
  

gchar * amitk_import_menu_explanations[] = {
  "",
  N_("Import file as raw data"),
#ifdef AMIDE_LIBECAT_SUPPORT
  N_("Import a CTI 6.4 or 7.0 file using the libecat library"),
#endif
#ifdef AMIDE_LIBMDC_SUPPORT
  N_("Import via the (X)medcon importing library (libmdc)"),
#endif
};

gchar * amitk_conversion_names[] = {
  N_("Direct"),
  N_("%ID/G"),
  N_("SUV")
};

gchar * amitk_dose_unit_names[] = {
  N_("MBq"),
  N_("mCi"),
  N_("uCi"),
  N_("nCi")
};

gchar * amitk_weight_unit_names[] = {
  N_("Kg"),
  N_("g"),
  N_("lbs"),
  N_("ounces"),
};

gchar * amitk_cylinder_unit_names[] = {
  N_("MBq/cc/Image Units"),
  N_("Image Units/(MBq/CC)"),
  N_("mCi/cc/Image Units"),
  N_("Image Units/(mCi/cc)"),
  N_("uCi/cc/Image Units"),
  N_("Image Units/(uCi/cc)"),
  N_("nCi/cc/Image Units"),
  N_("Image Units/(nCi/cc)"),
};

gchar * amitk_scaling_menu_names[] = {
  N_("Single Scale Factor"),
  N_("Per Frame Scale Factor"),
  N_("Per Plane Scale Factor")
};


enum {
  THRESHOLDING_CHANGED,
  COLOR_TABLE_CHANGED,
  INTERPOLATION_CHANGED,
  CONVERSION_CHANGED,
  SCALE_FACTOR_CHANGED,
  MODALITY_CHANGED,
  TIME_CHANGED,
  VOXEL_SIZE_CHANGED,
  DATA_SET_CHANGED,
  INVALIDATE_SLICE_CACHE,
  LAST_SIGNAL
};

static void          data_set_class_init             (AmitkDataSetClass *klass);
static void          data_set_init                   (AmitkDataSet      *data_set);
static void          data_set_finalize               (GObject           *object);
static void          data_set_scale                  (AmitkSpace        *space,
						      AmitkPoint        *ref_point,
						      AmitkPoint        *scaling);
static void          data_set_space_changed          (AmitkSpace        *space);
static AmitkObject * data_set_copy                   (const AmitkObject *object);
static void          data_set_copy_in_place          (AmitkObject * dest_object, const AmitkObject * src_object);
static void          data_set_write_xml              (const AmitkObject *object, 
						      xmlNodePtr         nodes, 
						      FILE              *study_file);
static gchar *       data_set_read_xml               (AmitkObject       *object, 
						      xmlNodePtr         nodes, 
						      FILE              *study_file,
						      gchar             *error_buf);
static void          data_set_invalidate_slice_cache (AmitkDataSet * ds);
static void          data_set_set_voxel_size         (AmitkDataSet * ds, 
						      const AmitkPoint voxel_size);
static AmitkVolumeClass * parent_class;
static guint         data_set_signals[LAST_SIGNAL];


static amide_data_t calculate_scale_factor(AmitkDataSet * ds);

GType amitk_data_set_get_type(void) {

  static GType data_set_type = 0;

  if (!data_set_type)
    {
      static const GTypeInfo data_set_info =
      {
	sizeof (AmitkDataSetClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) data_set_class_init,
	(GClassFinalizeFunc) NULL,
	NULL,		/* class_data */
	sizeof (AmitkDataSet),
	0,			/* n_preallocs */
	(GInstanceInitFunc) data_set_init,
	NULL /* value table */
      };
      
      data_set_type = g_type_register_static (AMITK_TYPE_VOLUME, "AmitkDataSet", &data_set_info, 0);
    }

  return data_set_type;
}


static void data_set_class_init (AmitkDataSetClass * class) {

  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  AmitkObjectClass * object_class = AMITK_OBJECT_CLASS(class);
  AmitkSpaceClass * space_class = AMITK_SPACE_CLASS(class);

  parent_class = g_type_class_peek_parent(class);

  space_class->space_scale = data_set_scale;
  space_class->space_changed = data_set_space_changed;

  object_class->object_copy = data_set_copy;
  object_class->object_copy_in_place = data_set_copy_in_place;
  object_class->object_write_xml = data_set_write_xml;
  object_class->object_read_xml = data_set_read_xml;

  class->invalidate_slice_cache = data_set_invalidate_slice_cache;

  gobject_class->finalize = data_set_finalize;


  data_set_signals[THRESHOLDING_CHANGED] =
    g_signal_new ("thresholding_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkDataSetClass, thresholding_changed),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
  data_set_signals[COLOR_TABLE_CHANGED] =
    g_signal_new ("color_table_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkDataSetClass, color_table_changed),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
  data_set_signals[INTERPOLATION_CHANGED] =
    g_signal_new ("interpolation_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkDataSetClass, interpolation_changed),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
  data_set_signals[CONVERSION_CHANGED] =
    g_signal_new ("conversion_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkDataSetClass, conversion_changed),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
  data_set_signals[SCALE_FACTOR_CHANGED] = 
    g_signal_new ("scale_factor_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkDataSetClass, scale_factor_changed),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
  data_set_signals[MODALITY_CHANGED] = 
    g_signal_new ("modality_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkDataSetClass, modality_changed),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
  data_set_signals[TIME_CHANGED] = 
    g_signal_new ("time_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkDataSetClass, time_changed),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
  data_set_signals[VOXEL_SIZE_CHANGED] = 
    g_signal_new ("voxel_size_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkDataSetClass, voxel_size_changed),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
  data_set_signals[DATA_SET_CHANGED] = 
    g_signal_new ("data_set_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkDataSetClass, data_set_changed),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
  data_set_signals[INVALIDATE_SLICE_CACHE] = 
    g_signal_new ("invalidate_slice_cache",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkDataSetClass, invalidate_slice_cache),
		  NULL, NULL,
		  amitk_marshal_NONE__NONE, G_TYPE_NONE, 0);
}


static void data_set_init (AmitkDataSet * data_set) {

  time_t current_time;
  gint i;

  /* put in some sensable values */
  data_set->raw_data = NULL;
  data_set->current_scaling = NULL;
  data_set->frame_duration = NULL;
  data_set->frame_max = NULL;
  data_set->frame_min = NULL;
  data_set->global_max = 0.0;
  data_set->global_min = 0.0;
  amitk_data_set_set_thresholding(data_set, AMITK_THRESHOLDING_GLOBAL);
  for (i=0; i<2; i++)
    data_set->threshold_ref_frame[i]=0;
  data_set->distribution = NULL;
  data_set->modality = AMITK_MODALITY_PET;
  data_set->voxel_size = zero_point;
  data_set->scaling_type = AMITK_SCALING_TYPE_0D;
  data_set->internal_scaling = amitk_raw_data_DOUBLE_0D_SCALING_init(1.0);
  g_assert(data_set->internal_scaling!=NULL);

  amitk_data_set_set_scale_factor(data_set, 1.0);
  data_set->conversion = AMITK_CONVERSION_STRAIGHT;
  data_set->injected_dose = 1.0;
  data_set->displayed_dose_unit = AMITK_DOSE_UNIT_MEGABECQUEREL;
  data_set->subject_weight = 1.0;
  data_set->displayed_weight_unit = AMITK_WEIGHT_UNIT_KILOGRAM;
  data_set->cylinder_factor = 1.0;
  data_set->displayed_cylinder_unit = AMITK_CYLINDER_UNIT_MEGABECQUEREL_PER_CC_IMAGE_UNIT;
  
  data_set->scan_start = 0.0;
  data_set->color_table = AMITK_COLOR_TABLE_BW_LINEAR;
  data_set->interpolation = AMITK_INTERPOLATION_NEAREST_NEIGHBOR;
  data_set->slice_cache = NULL;
  data_set->max_slice_cache_size = 6;
  data_set->slice_parent = NULL;

  /* set the scan date to the current time, good for an initial guess */
  time(&current_time);
  data_set->scan_date = NULL;
  amitk_data_set_set_scan_date(data_set, ctime(&current_time));

}


static void data_set_finalize (GObject *object)
{
  AmitkDataSet * data_set = AMITK_DATA_SET(object);

  if (data_set->raw_data != NULL) {
#ifdef AMIDE_DEBUG
    if (data_set->raw_data->dim.z != 1) /* avoid slices */
      g_print("\tfreeing data set: %s\n",AMITK_OBJECT_NAME(data_set));
#endif
    g_object_unref(data_set->raw_data);
    data_set->raw_data = NULL;
  }

  if (data_set->internal_scaling != NULL) {
    g_object_unref(data_set->internal_scaling);
    data_set->internal_scaling = NULL;
  }

  if (data_set->current_scaling != NULL) {
    g_object_unref(data_set->current_scaling);
    data_set->current_scaling = NULL;
  }

  if (data_set->distribution != NULL) {
    g_object_unref(data_set->distribution);
    data_set->distribution = NULL;
  }

  if (data_set->frame_duration != NULL) {
    g_free(data_set->frame_duration);
    data_set->frame_duration = NULL;
  }

  if (data_set->frame_max != NULL) {
    g_free(data_set->frame_max);
    data_set->frame_max = NULL;
  }

  if (data_set->frame_min != NULL) {
    g_free(data_set->frame_min);
    data_set->frame_min = NULL;
  }

  if (data_set->scan_date != NULL) {
    g_free(data_set->scan_date);
    data_set->scan_date = NULL;
  }

  if (data_set->slice_cache != NULL) {
    amitk_objects_unref(data_set->slice_cache);
    data_set->slice_cache = NULL;
  }

  if (data_set->slice_parent != NULL) {
    g_object_remove_weak_pointer(G_OBJECT(data_set->slice_parent),
				 (gpointer *) &(data_set->slice_parent));
    data_set->slice_parent = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void data_set_scale(AmitkSpace *space, AmitkPoint *ref_point, AmitkPoint *scaling) {

  AmitkDataSet * data_set;
  AmitkPoint voxel_size;

  g_return_if_fail(AMITK_IS_DATA_SET(space));
  data_set = AMITK_DATA_SET(space);

  /* first, pass the signal on, this gets the volume corner value readjusted */
  AMITK_SPACE_CLASS(parent_class)->space_scale (space, ref_point, scaling);

  /* readjust the voxel size based on the new corner */
  if (AMITK_VOLUME_VALID(data_set)) {
    voxel_size = AMITK_VOLUME_CORNER(data_set);
    voxel_size.x /= AMITK_DATA_SET_DIM_X(data_set);
    voxel_size.y /= AMITK_DATA_SET_DIM_Y(data_set);
    voxel_size.z /= AMITK_DATA_SET_DIM_Z(data_set);
    data_set_set_voxel_size(data_set, voxel_size);
  }

}


static void data_set_space_changed(AmitkSpace * space) {

  AmitkDataSet * data_set;

  g_return_if_fail(AMITK_IS_DATA_SET(space));
  data_set = AMITK_DATA_SET(space);

  g_signal_emit(G_OBJECT (data_set), data_set_signals[INVALIDATE_SLICE_CACHE], 0);

  if (AMITK_SPACE_CLASS(parent_class)->space_changed)
    AMITK_SPACE_CLASS(parent_class)->space_changed (space);
}

static AmitkObject * data_set_copy (const AmitkObject * object) {

  AmitkDataSet * copy;

  g_return_val_if_fail(AMITK_IS_DATA_SET(object), NULL);
  
  copy = amitk_data_set_new();
  amitk_object_copy_in_place(AMITK_OBJECT(copy), object);

  return AMITK_OBJECT(copy);
}

/* Notes: 
   - does not make a copy of the source raw_data, just adds a reference 
   - does not make a copy of the distribution data, just adds a reference
   - does not make a copy of the internal scaling factor, just adds a reference */
static void data_set_copy_in_place (AmitkObject * dest_object, const AmitkObject * src_object) {

  AmitkDataSet * src_ds;
  AmitkDataSet * dest_ds;
  guint i;

  g_return_if_fail(AMITK_IS_DATA_SET(src_object));
  g_return_if_fail(AMITK_IS_DATA_SET(dest_object));

  src_ds = AMITK_DATA_SET(src_object);
  dest_ds = AMITK_DATA_SET(dest_object);


  /* copy the data elements */
  amitk_data_set_set_scan_date(dest_ds, AMITK_DATA_SET_SCAN_DATE(src_object));
  amitk_data_set_set_modality(dest_ds, AMITK_DATA_SET_MODALITY(src_object));
  dest_ds->voxel_size = AMITK_DATA_SET_VOXEL_SIZE(src_object);
  if (src_ds->raw_data != NULL) {
    if (dest_ds->raw_data != NULL)
      g_object_unref(dest_ds->raw_data);
    dest_ds->raw_data = g_object_ref(src_ds->raw_data);
  }

  /* just reference, as internal scaling is never suppose to change */
  dest_ds->scaling_type = src_ds->scaling_type;
  if (src_ds->internal_scaling != NULL) {
    if (dest_ds->internal_scaling != NULL)
      g_object_unref(dest_ds->internal_scaling);
    dest_ds->internal_scaling = g_object_ref(src_ds->internal_scaling);
  }

  dest_ds->scan_start = AMITK_DATA_SET_SCAN_START(src_object);

  if (src_ds->distribution != NULL) {
    if (dest_ds->distribution != NULL)
      g_object_unref(dest_ds->distribution);
    dest_ds->distribution = g_object_ref(src_ds->distribution);
  }
  amitk_data_set_set_scale_factor(dest_ds, AMITK_DATA_SET_SCALE_FACTOR(src_object));
  dest_ds->conversion = AMITK_DATA_SET_CONVERSION(src_object);
  dest_ds->injected_dose = AMITK_DATA_SET_INJECTED_DOSE(src_object);
  dest_ds->displayed_dose_unit = AMITK_DATA_SET_DISPLAYED_DOSE_UNIT(src_object);
  dest_ds->subject_weight = AMITK_DATA_SET_SUBJECT_WEIGHT(src_object);
  dest_ds->displayed_weight_unit = AMITK_DATA_SET_DISPLAYED_WEIGHT_UNIT(src_object);
  dest_ds->cylinder_factor = AMITK_DATA_SET_CYLINDER_FACTOR(src_object);
  dest_ds->displayed_cylinder_unit = AMITK_DATA_SET_DISPLAYED_CYLINDER_UNIT(src_object);

  amitk_data_set_set_color_table(dest_ds, AMITK_DATA_SET_COLOR_TABLE(src_object));
  amitk_data_set_set_interpolation(dest_ds, AMITK_DATA_SET_INTERPOLATION(src_object));
  amitk_data_set_set_thresholding(dest_ds,AMITK_DATA_SET_THRESHOLDING(src_object));
  for (i=0; i<2; i++) {
    dest_ds->threshold_max[i] = AMITK_DATA_SET_THRESHOLD_MAX(src_object, i);
    dest_ds->threshold_min[i] = AMITK_DATA_SET_THRESHOLD_MIN(src_object, i);
    dest_ds->threshold_ref_frame[i] = AMITK_DATA_SET_THRESHOLD_REF_FRAME(src_object, i);
  }
  dest_ds->global_max = AMITK_DATA_SET_GLOBAL_MAX(src_object);
  dest_ds->global_min = AMITK_DATA_SET_GLOBAL_MIN(src_object);

  /* make a separate copy in memory of the volume's frame durations and max/min values */
  if (dest_ds->frame_duration != NULL) 
    g_free(dest_ds->frame_duration);
  dest_ds->frame_duration = amitk_data_set_get_frame_duration_mem(dest_ds);
  g_return_if_fail(dest_ds->frame_duration != NULL);
  for (i=0;i<AMITK_DATA_SET_NUM_FRAMES(dest_ds);i++)
    dest_ds->frame_duration[i] = src_ds->frame_duration[i];

  if (dest_ds->frame_max != NULL)
    g_free(dest_ds->frame_max);
  dest_ds->frame_max = amitk_data_set_get_frame_max_min_mem(dest_ds);
  g_return_if_fail(dest_ds->frame_max != NULL);
  for (i=0;i<AMITK_DATA_SET_NUM_FRAMES(dest_ds);i++) 
    dest_ds->frame_max[i] = src_ds->frame_max[i];

  if (dest_ds->frame_min != NULL)
    g_free(dest_ds->frame_min);
  dest_ds->frame_min = amitk_data_set_get_frame_max_min_mem(dest_ds);
  g_return_if_fail(dest_ds->frame_min != NULL);
  for (i=0;i<AMITK_DATA_SET_NUM_FRAMES(dest_ds);i++)
    dest_ds->frame_min[i] = src_ds->frame_min[i];

  AMITK_OBJECT_CLASS (parent_class)->object_copy_in_place (dest_object, src_object);
}







static void data_set_write_xml(const AmitkObject * object, xmlNodePtr nodes, FILE * study_file) {

  AmitkDataSet * ds;
  gchar * xml_filename;
  gchar * name;
  guint64 location, size;

  AMITK_OBJECT_CLASS(parent_class)->object_write_xml(object, nodes, study_file);

  ds = AMITK_DATA_SET(object);

  xml_save_string(nodes, "scan_date", AMITK_DATA_SET_SCAN_DATE(ds));
  xml_save_string(nodes, "modality", amitk_modality_get_name(AMITK_DATA_SET_MODALITY(ds)));
  amitk_point_write_xml(nodes, "voxel_size", AMITK_DATA_SET_VOXEL_SIZE(ds));

  name = g_strdup_printf("data-set_%s_raw-data",AMITK_OBJECT_NAME(object));
  amitk_raw_data_write_xml(AMITK_DATA_SET_RAW_DATA(ds), name, study_file, &xml_filename, &location, &size);
  g_free(name);
  if (study_file == NULL) {
    xml_save_string(nodes, "raw_data_file", xml_filename);
    g_free(xml_filename);
  } else {
    xml_save_location_and_size(nodes, "raw_data_location_and_size", location, size);
  }

  name = g_strdup_printf("data-set_%s_scaling-factors",AMITK_OBJECT_NAME(ds));
  amitk_raw_data_write_xml(ds->internal_scaling, name, study_file, &xml_filename, &location, &size);
  g_free(name);
  if (study_file == NULL) {
    xml_save_string(nodes, "internal_scaling_file", xml_filename);
    g_free(xml_filename);
  } else {
    xml_save_location_and_size(nodes, "internal_scaling_location_and_size", location, size);
  }

  if (ds->distribution != NULL) {
    name = g_strdup_printf("data-set_%s_distribution",AMITK_OBJECT_NAME(ds));
    amitk_raw_data_write_xml(ds->distribution, name, study_file, &xml_filename, &location, &size);
    g_free(name);
    if (study_file == NULL) {
      xml_save_string(nodes, "distribution_file", xml_filename);
      g_free(xml_filename);
    } else {
      xml_save_location_and_size(nodes, "distribution_location_and_size", location, size);
    }
  }

  xml_save_string(nodes, "scaling_type", amitk_scaling_type_get_name(ds->scaling_type));
  xml_save_data(nodes, "scale_factor", AMITK_DATA_SET_SCALE_FACTOR(ds));
  xml_save_string(nodes, "conversion", amitk_conversion_get_name(ds->conversion));
  xml_save_data(nodes, "injected_dose", AMITK_DATA_SET_INJECTED_DOSE(ds));
  xml_save_string(nodes, "displayed_dose_unit", amitk_dose_unit_get_name(ds->displayed_dose_unit));
  xml_save_data(nodes, "subject_weight", AMITK_DATA_SET_SUBJECT_WEIGHT(ds));
  xml_save_string(nodes, "displayed_weight_unit", amitk_weight_unit_get_name(ds->displayed_weight_unit));
  xml_save_data(nodes, "cylinder_factor", AMITK_DATA_SET_CYLINDER_FACTOR(ds));
  xml_save_string(nodes, "displayed_cylinder_unit", amitk_cylinder_unit_get_name(ds->displayed_cylinder_unit));
		  
  xml_save_time(nodes, "scan_start", AMITK_DATA_SET_SCAN_START(ds));
  xml_save_times(nodes, "frame_duration", ds->frame_duration, AMITK_DATA_SET_NUM_FRAMES(ds));
  xml_save_string(nodes, "color_table", amitk_color_table_get_name(AMITK_DATA_SET_COLOR_TABLE(ds)));
  xml_save_string(nodes, "interpolation", 
		  amitk_interpolation_get_name(AMITK_DATA_SET_INTERPOLATION(ds)));
  xml_save_string(nodes, "thresholding", amitk_thresholding_get_name(AMITK_DATA_SET_THRESHOLDING(ds)));
  xml_save_data(nodes, "threshold_max_0", ds->threshold_max[0]);
  xml_save_data(nodes, "threshold_min_0", ds->threshold_min[0]);
  xml_save_int(nodes, "threshold_ref_frame_0", ds->threshold_ref_frame[0]);
  xml_save_data(nodes, "threshold_max_1", ds->threshold_max[1]);
  xml_save_data(nodes, "threshold_min_1", ds->threshold_min[1]);
  xml_save_int(nodes, "threshold_ref_frame_1", ds->threshold_ref_frame[1]);

  return;
}

static gchar * data_set_read_xml(AmitkObject * object, xmlNodePtr nodes, 
				 FILE * study_file, gchar * error_buf) {

  AmitkDataSet * ds;
  AmitkModality i_modality;
  AmitkColorTable i_color_table;
  AmitkThresholding i_thresholding;
  AmitkInterpolation i_interpolation;
  AmitkScalingType i_scaling_type;
  AmitkConversion i_conversion;
  AmitkDoseUnit i_dose_unit;
  AmitkWeightUnit i_weight_unit;
  AmitkCylinderUnit i_cylinder_unit;
  gchar * temp_string;
  gchar * scan_date;
  gchar * filename=NULL;
  guint64 location, size;

  error_buf = AMITK_OBJECT_CLASS(parent_class)->object_read_xml(object, nodes, study_file, error_buf);

  ds = AMITK_DATA_SET(object);

  scan_date = xml_get_string(nodes, "scan_date");
  amitk_data_set_set_scan_date(ds, scan_date);
  g_free(scan_date);

  temp_string = xml_get_string(nodes, "modality");
  if (temp_string != NULL)
    for (i_modality=0; i_modality < AMITK_MODALITY_NUM; i_modality++) 
      if (g_ascii_strcasecmp(temp_string, amitk_modality_get_name(i_modality)) == 0)
	amitk_data_set_set_modality(ds, i_modality);
  g_free(temp_string);

  ds->voxel_size = amitk_point_read_xml(nodes, "voxel_size", &error_buf);

  if (study_file == NULL) 
    filename = xml_get_string(nodes, "raw_data_file");
  else
    xml_get_location_and_size(nodes, "raw_data_location_and_size", &location, &size, &error_buf);

  ds->raw_data = amitk_raw_data_read_xml(filename, study_file, location, size, &error_buf, NULL, NULL);
  if (filename != NULL) {
    g_free(filename);
    filename = NULL;
  }

  if (study_file == NULL) 
    filename = xml_get_string(nodes, "internal_scaling_file");
  else
    xml_get_location_and_size(nodes, "internal_scaling_location_and_size", &location, &size, &error_buf);

  if (ds->internal_scaling != NULL) g_object_unref(ds->internal_scaling);
  ds->internal_scaling = amitk_raw_data_read_xml(filename, study_file, location, size, &error_buf, NULL, NULL);
  if (filename != NULL) {
    g_free(filename);
    filename = NULL;
  }

  if (xml_node_exists(nodes, "distribution_file") || 
      xml_node_exists(nodes, "distribution_location_and_size")) {
    if (study_file == NULL) 
      filename = xml_get_string(nodes, "distribution_file");
    else
      xml_get_location_and_size(nodes, "distribution_location_and_size", &location, &size, &error_buf);
    if (ds->distribution != NULL) g_object_unref(ds->distribution);
    ds->distribution = amitk_raw_data_read_xml(filename, study_file, location, size, &error_buf, NULL, NULL);
    if (filename != NULL) {
      g_free(filename);
      filename = NULL;
    }
  }

  /* figure out the scaling type */
  temp_string = xml_get_string(nodes, "scaling_type");
  if (temp_string != NULL) {
    for (i_scaling_type=0; i_scaling_type < AMITK_SCALING_TYPE_NUM; i_scaling_type++) 
      if (g_ascii_strcasecmp(temp_string, amitk_scaling_type_get_name(i_scaling_type)) == 0)
	ds->scaling_type = i_scaling_type;
  } else { /* scaling_type is a new entry, circa 0.7.7 */
    if (ds->internal_scaling->dim.z > 1) 
      ds->scaling_type = AMITK_SCALING_TYPE_2D;
    else if (ds->internal_scaling->dim.t > 1)
      ds->scaling_type = AMITK_SCALING_TYPE_1D;
    else 
      ds->scaling_type = AMITK_SCALING_TYPE_0D;
  }
  g_free(temp_string);

  /* a little legacy bit, the type of internal_scaling has been changed to double
     as of amide 0.7.1 */
  if (ds->internal_scaling->format != AMITK_FORMAT_DOUBLE) {
    AmitkRawData * old_scaling;
    AmitkVoxel i;

    amitk_append_str(&error_buf, _("wrong type found on internal scaling, converting to double"));
    old_scaling = ds->internal_scaling;

    ds->internal_scaling = amitk_raw_data_new_with_data(AMITK_FORMAT_DOUBLE, old_scaling->dim);
    if (ds->internal_scaling == NULL) {
      amitk_append_str(&error_buf, _("Couldn't allocate space for the new scaling factors"));
      return error_buf;
    }

    for (i.t=0; i.t<ds->internal_scaling->dim.t; i.t++)
      for (i.z=0; i.z<ds->internal_scaling->dim.z; i.z++)
	for (i.y=0; i.y<ds->internal_scaling->dim.y; i.y++)
	  for (i.x=0; i.x<ds->internal_scaling->dim.x; i.x++)
	    AMITK_RAW_DATA_DOUBLE_SET_CONTENT(ds->internal_scaling,i) = 
	      amitk_raw_data_get_value(old_scaling, i);
    
    g_object_unref(old_scaling);
  }
  /* end legacy cruft */

  amitk_data_set_set_scale_factor(ds, xml_get_data(nodes, "scale_factor", &error_buf));
  ds->injected_dose = xml_get_data(nodes, "injected_dose", &error_buf);
  ds->subject_weight = xml_get_data(nodes, "subject_weight", &error_buf); 
  ds->cylinder_factor = xml_get_data(nodes, "cylinder_factor", &error_buf);

  temp_string = xml_get_string(nodes, "conversion");
  if (temp_string != NULL)
    for (i_conversion=0; i_conversion < AMITK_CONVERSION_NUM; i_conversion++)
      if (g_ascii_strcasecmp(temp_string, amitk_conversion_get_name(i_conversion)) == 0)
	ds->conversion = i_conversion;
  g_free(temp_string);

  temp_string = xml_get_string(nodes, "displayed_dose_unit");
  if (temp_string != NULL)
    for (i_dose_unit=0; i_dose_unit < AMITK_DOSE_UNIT_NUM; i_dose_unit++)
      if (g_ascii_strcasecmp(temp_string, amitk_dose_unit_get_name(i_dose_unit)) == 0)
	ds->displayed_dose_unit = i_dose_unit;
  g_free(temp_string);

  temp_string = xml_get_string(nodes, "displayed_weight_unit");
  if (temp_string != NULL)
    for (i_weight_unit=0; i_weight_unit < AMITK_WEIGHT_UNIT_NUM; i_weight_unit++)
      if (g_ascii_strcasecmp(temp_string, amitk_weight_unit_get_name(i_weight_unit)) == 0)
	ds->displayed_weight_unit = i_weight_unit;
  g_free(temp_string);

  temp_string = xml_get_string(nodes, "displayed_cylinder_unit");
  if (temp_string != NULL)
    for (i_cylinder_unit=0; i_cylinder_unit < AMITK_CYLINDER_UNIT_NUM; i_cylinder_unit++)
      if (g_ascii_strcasecmp(temp_string, amitk_cylinder_unit_get_name(i_cylinder_unit)) == 0)
	ds->displayed_cylinder_unit = i_cylinder_unit;
  g_free(temp_string);


  ds->scan_start = xml_get_time(nodes, "scan_start", &error_buf);
  ds->frame_duration = xml_get_times(nodes, "frame_duration", AMITK_DATA_SET_NUM_FRAMES(ds), &error_buf);

  temp_string = xml_get_string(nodes, "color_table");
  if (temp_string != NULL)
    for (i_color_table=0; i_color_table < AMITK_COLOR_TABLE_NUM; i_color_table++) 
      if (g_ascii_strcasecmp(temp_string, amitk_color_table_get_name(i_color_table)) == 0)
	amitk_data_set_set_color_table(ds, i_color_table);
  g_free(temp_string);

  /* figure out the interpolation */
  temp_string = xml_get_string(nodes, "interpolation");
  if (temp_string != NULL)
    for (i_interpolation=0; i_interpolation < AMITK_INTERPOLATION_NUM; i_interpolation++) 
      if (g_ascii_strcasecmp(temp_string, amitk_interpolation_get_name(i_interpolation)) == 0)
	amitk_data_set_set_interpolation(ds, i_interpolation);
  g_free(temp_string);

  temp_string = xml_get_string(nodes, "thresholding");
  if (temp_string != NULL)
    for (i_thresholding=0; i_thresholding < AMITK_THRESHOLDING_NUM; i_thresholding++) 
      if (g_ascii_strcasecmp(temp_string, amitk_thresholding_get_name(i_thresholding)) == 0)
	amitk_data_set_set_thresholding(ds, i_thresholding);
  g_free(temp_string);

  ds->threshold_max[0] =  xml_get_data(nodes, "threshold_max_0", &error_buf);
  ds->threshold_max[1] =  xml_get_data(nodes, "threshold_max_1", &error_buf);
  ds->threshold_ref_frame[0] = xml_get_int(nodes,"threshold_ref_frame_0", &error_buf);
  ds->threshold_min[0] =  xml_get_data(nodes, "threshold_min_0", &error_buf);
  ds->threshold_min[1] =  xml_get_data(nodes, "threshold_min_1", &error_buf);
  ds->threshold_ref_frame[1] = xml_get_int(nodes,"threshold_ref_frame_1", &error_buf);

  /* recalc the temporary parameters */
  amitk_data_set_calc_far_corner(ds);
  amitk_data_set_calc_max_min(ds, NULL, NULL);

  return error_buf;
}

static void data_set_invalidate_slice_cache(AmitkDataSet * data_set) {

  /* invalidate cache */
  if (data_set->slice_cache != NULL) {
    amitk_objects_unref(data_set->slice_cache);
    data_set->slice_cache = NULL;
  }

  return;
}

/* this does not recalc the far corner, needs to be done separately */
static void data_set_set_voxel_size(AmitkDataSet * ds, const AmitkPoint voxel_size) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  if (!POINT_EQUAL(AMITK_DATA_SET_VOXEL_SIZE(ds), voxel_size)) {
    ds->voxel_size = voxel_size;
    g_signal_emit(G_OBJECT (ds), data_set_signals[VOXEL_SIZE_CHANGED], 0);
    g_signal_emit(G_OBJECT (ds), data_set_signals[INVALIDATE_SLICE_CACHE], 0);
    g_signal_emit(G_OBJECT (ds), data_set_signals[DATA_SET_CHANGED], 0);
  }
}


AmitkDataSet * amitk_data_set_new (void) {

  AmitkDataSet * data_set;

  data_set = g_object_new(amitk_data_set_get_type(), NULL);

  return data_set;
}


AmitkDataSet * amitk_data_set_new_with_data(const AmitkFormat format, 
					    const AmitkVoxel dim,
					    const AmitkScalingType scaling_type) {

  AmitkDataSet * data_set;
  AmitkVoxel scaling_dim;
  gint i;

  data_set = amitk_data_set_new();
  g_return_val_if_fail(data_set != NULL, NULL);

  g_assert(data_set->raw_data == NULL);
  data_set->raw_data = amitk_raw_data_new_with_data(format, dim);
  if (data_set->raw_data == NULL) {
    amitk_object_unref(data_set);
    g_return_val_if_reached(NULL);
  }

  g_assert(data_set->frame_duration == NULL);
  data_set->frame_duration = amitk_data_set_get_frame_duration_mem(data_set);
  if (data_set->frame_duration == NULL) {
    amitk_object_unref(data_set);
    g_return_val_if_reached(NULL);
  }
  for (i=0; i < dim.t; i++) 
    data_set->frame_duration[i] = 0.0;


  scaling_dim = one_voxel;
  data_set->scaling_type = scaling_type;
  switch(scaling_type) {
  case AMITK_SCALING_TYPE_2D:
    g_object_unref(data_set->internal_scaling);
    scaling_dim.t = dim.t;
    scaling_dim.z = dim.z;
    data_set->internal_scaling = amitk_raw_data_new_with_data(AMITK_FORMAT_DOUBLE, scaling_dim);
    break;
  case AMITK_SCALING_TYPE_1D:
    g_object_unref(data_set->internal_scaling);
    scaling_dim.t = dim.t;
    data_set->internal_scaling = amitk_raw_data_new_with_data(AMITK_FORMAT_DOUBLE, scaling_dim);
    break;
  case AMITK_SCALING_TYPE_0D:
  default: 
    break; /* amitk_new returns a 0D scaling factor already */
  }
  if (data_set->internal_scaling == NULL) {
    amitk_object_unref(data_set);
    g_return_val_if_reached(NULL);
  }

  return data_set;
}





/* reads the contents of a raw data file into an amide data set structure,
   note: returned structure will have 1 second frame durations entered
   note: returned structure will have a voxel size of 1x1x1 mm
   note: file_offset is bytes for a binary file, lines for an ascii file
*/
AmitkDataSet * amitk_data_set_import_raw_file(const gchar * file_name, 
					      AmitkRawFormat raw_format,
					      AmitkVoxel data_dim,
					      guint file_offset,
					      gboolean (*update_func)(), 
					      gpointer update_data) {

  guint t;
  AmitkDataSet * ds;

  if ((ds = amitk_data_set_new()) == NULL) {
    g_warning(_("couldn't allocate space for the data set structure to hold data"));
    return NULL;
  }

  /* read in the data set */
  ds->raw_data =  amitk_raw_data_import_raw_file(file_name, NULL, raw_format, data_dim, file_offset,
						 update_func, update_data);
  if (ds->raw_data == NULL) {
    g_warning(_("raw_data_read_file failed returning NULL data set"));
    amitk_object_unref(ds);
    return NULL;
  }

  /* allocate space for the array containing info on the duration of the frames */
  if ((ds->frame_duration = amitk_data_set_get_frame_duration_mem(ds)) == NULL) {
    g_warning(_("couldn't allocate space for the frame duration info"));
    amitk_object_unref(ds);
    return NULL;
  }

  /* put in fake frame durations */
  for (t=0; t < AMITK_DATA_SET_NUM_FRAMES(ds); t++)
    ds->frame_duration[t] = 1.0;

  /* calc max/min values */
  amitk_data_set_calc_max_min(ds, update_func, update_data);

  /* put in fake voxel size values and set the far corner appropriately */
  amitk_data_set_set_modality(ds, AMITK_MODALITY_CT); /* guess CT... */
  ds->voxel_size = one_point;
  amitk_data_set_calc_far_corner(ds);

  /* set any remaining parameters */
  amitk_volume_set_center(AMITK_VOLUME(ds), zero_point);

  return ds;
}



/* function to import a file into a data set */
AmitkDataSet * amitk_data_set_import_file(AmitkImportMethod import_method, 
					  int submethod,
					  const gchar * import_filename,
					  gboolean (*update_func)(),
					  gpointer update_data) {
  AmitkDataSet * import_ds;
  gchar * import_filename_base;
  gchar * import_filename_extension;
  gchar ** frags=NULL;

  g_return_val_if_fail(import_filename != NULL, NULL);

  /* if we're guessing how to import.... */
  if (import_method == AMITK_IMPORT_METHOD_GUESS) {

    /* extract the extension of the file */
    import_filename_base = g_path_get_basename(import_filename);
    g_strreverse(import_filename_base);
    frags = g_strsplit(import_filename_base, ".", 2);
    g_free(import_filename_base);

    import_filename_extension = g_strdup(frags[0]);
    g_strreverse(import_filename_extension);
    g_strfreev(frags);
    
    if ((g_ascii_strcasecmp(import_filename_extension, "dat")==0) ||
	(g_ascii_strcasecmp(import_filename_extension, "raw")==0))  
      /* .dat and .raw are assumed to be raw data */
      import_method = AMITK_IMPORT_METHOD_RAW;
#ifdef AMIDE_LIBECAT_SUPPORT      
    else if ((g_ascii_strcasecmp(import_filename_extension, "img")==0) ||
	     (g_ascii_strcasecmp(import_filename_extension, "v")==0) ||
	     (g_ascii_strcasecmp(import_filename_extension, "atn")==0) ||
	     (g_ascii_strcasecmp(import_filename_extension, "scn")==0))
      /* if it appears to be a cti file */
      import_method = AMITK_IMPORT_METHOD_LIBECAT;
#endif
    else /* fallback methods */
#ifdef AMIDE_LIBMDC_SUPPORT
      /* try passing it to the libmdc library.... */
      import_method = AMITK_IMPORT_METHOD_LIBMDC;
#else
    { /* unrecognized file type */
      g_warning(_("Extension %s not recognized on file: %s\nGuessing File Type"), 
		import_filename_extension, import_filename);
      import_method = AMITK_IMPORT_METHOD_RAW;
    }
#endif

    g_free(import_filename_extension);
  }    

  switch (import_method) {

#ifdef AMIDE_LIBECAT_SUPPORT      
  case AMITK_IMPORT_METHOD_LIBECAT:
    import_ds =cti_import(import_filename, update_func, update_data);
    break;
#endif
#ifdef AMIDE_LIBMDC_SUPPORT
  case AMITK_IMPORT_METHOD_LIBMDC:
    import_ds=medcon_import(import_filename, submethod, update_func, update_data);
    break;
#endif
  case AMITK_IMPORT_METHOD_RAW:
  default:
    import_ds= raw_data_import(import_filename);
    break;
  }

  if (import_ds == NULL) return NULL;

  /* set the thresholds */
  import_ds->threshold_max[0] = import_ds->threshold_max[1] = import_ds->global_max;
  import_ds->threshold_min[0] = import_ds->threshold_min[1] =
    (import_ds->global_min > 0.0) ? import_ds->global_min : 0.0;
  import_ds->threshold_ref_frame[1] = AMITK_DATA_SET_NUM_FRAMES(import_ds)-1;

  return import_ds;
}

void amitk_data_set_set_modality(AmitkDataSet * ds, const AmitkModality modality) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  if ( ds->modality != modality) {
    ds->modality = modality;
    g_signal_emit(G_OBJECT (ds), data_set_signals[MODALITY_CHANGED], 0);
    g_signal_emit(G_OBJECT (ds), data_set_signals[DATA_SET_CHANGED], 0);
  }
}


void amitk_data_set_set_scan_start (AmitkDataSet * ds, const amide_time_t start) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  if (ds->scan_start != start) {
    ds->scan_start = start;
    g_signal_emit(G_OBJECT (ds), data_set_signals[TIME_CHANGED], 0);
    g_signal_emit(G_OBJECT (ds), data_set_signals[DATA_SET_CHANGED], 0);
  }
}


void amitk_data_set_set_frame_duration(AmitkDataSet * ds, const guint frame,
				       amide_time_t duration) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  g_return_if_fail(frame < AMITK_DATA_SET_NUM_FRAMES(ds));
  g_return_if_fail(ds->frame_duration != NULL);

  if (duration < EPSILON)
    duration = EPSILON; /* guard against bad values */

  if (ds->frame_duration[frame] != duration) {
    ds->frame_duration[frame] = duration;
    g_signal_emit(G_OBJECT (ds), data_set_signals[TIME_CHANGED], 0);
    g_signal_emit(G_OBJECT (ds), data_set_signals[DATA_SET_CHANGED], 0);
  }
}

void amitk_data_set_set_voxel_size(AmitkDataSet * ds, const AmitkPoint voxel_size) {

  GList * children;
  AmitkPoint ref_point;
  AmitkPoint scaling;
  AmitkPoint old_corner;

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  if (!POINT_EQUAL(AMITK_DATA_SET_VOXEL_SIZE(ds), voxel_size)) {
    old_corner = AMITK_VOLUME_CORNER(ds);

    data_set_set_voxel_size(ds, voxel_size);
    amitk_data_set_calc_far_corner(ds);

    scaling = point_div(AMITK_VOLUME_CORNER(ds), old_corner);
    scaling = amitk_space_s2b_dim(AMITK_SPACE(ds), scaling);
    ref_point = AMITK_SPACE_OFFSET(ds);


    point_print("scaling", scaling);

    /* propagate this scaling operation to the children */
    children = AMITK_OBJECT_CHILDREN(ds);
    while (children != NULL) {
      amitk_space_scale(children->data, ref_point, scaling);
      children = children->next;
    }
  }
}

void amitk_data_set_set_thresholding(AmitkDataSet * ds, const AmitkThresholding thresholding) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  if (ds->thresholding != thresholding) {
    ds->thresholding = thresholding;
    g_signal_emit(G_OBJECT (ds), data_set_signals[THRESHOLDING_CHANGED], 0);
  }
}

void amitk_data_set_set_threshold_max(AmitkDataSet * ds, guint which_reference, amide_data_t value) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  if ((ds->threshold_max[which_reference] != value) && (value > AMITK_DATA_SET_GLOBAL_MIN(ds))) {
    ds->threshold_max[which_reference] = value;
    if (ds->threshold_max[which_reference] < ds->threshold_min[which_reference])
      ds->threshold_min[which_reference] = 
	ds->threshold_max[which_reference]-EPSILON*fabs(ds->threshold_max[which_reference]);
    g_signal_emit(G_OBJECT (ds), data_set_signals[THRESHOLDING_CHANGED], 0);
  }
}

void amitk_data_set_set_threshold_min(AmitkDataSet * ds, guint which_reference, amide_data_t value) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  if ((ds->threshold_min[which_reference] != value ) && (value < AMITK_DATA_SET_GLOBAL_MAX(ds))) {
    ds->threshold_min[which_reference] = value;
    if (ds->threshold_min[which_reference] > ds->threshold_max[which_reference])
      ds->threshold_max[which_reference] = 
	ds->threshold_min[which_reference]+EPSILON*fabs(ds->threshold_min[which_reference]);
    g_signal_emit(G_OBJECT (ds), data_set_signals[THRESHOLDING_CHANGED], 0);
  }
}

void amitk_data_set_set_threshold_ref_frame(AmitkDataSet * ds, guint which_reference, guint frame) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  if (ds->threshold_ref_frame[which_reference] != frame) {
    ds->threshold_ref_frame[which_reference] = frame;
    g_signal_emit(G_OBJECT (ds), data_set_signals[THRESHOLDING_CHANGED], 0);
  }
}

void amitk_data_set_set_color_table(AmitkDataSet * ds, const AmitkColorTable new_color_table) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  if (new_color_table != ds->color_table) {
    ds->color_table = new_color_table;
    g_signal_emit(G_OBJECT (ds), data_set_signals[COLOR_TABLE_CHANGED], 0);
  }
}

void amitk_data_set_set_interpolation(AmitkDataSet * ds, const AmitkInterpolation new_interpolation) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));

  if (ds->interpolation != new_interpolation) {
    ds->interpolation = new_interpolation;
    g_signal_emit(G_OBJECT (ds), data_set_signals[INTERPOLATION_CHANGED], 0);
  }

  return;
}

/* sets the creation date of a data set
   note: no error checking is done on the date, as of yet */
void amitk_data_set_set_scan_date(AmitkDataSet * ds, const gchar * new_date) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));

  if (ds->scan_date != NULL) {
    g_free(ds->scan_date); 
    ds->scan_date = NULL;
  }

  if (new_date != NULL) {
    ds->scan_date = g_strdup(new_date); 
    g_strdelimit(ds->scan_date, "\n", ' '); /* turns newlines to white space */
    g_strstrip(ds->scan_date); /* removes trailing and leading white space */
  } else {
    ds->scan_date = g_strdup(_("unknown"));
  }

  return;
}




/* used when changing the external scale factor, so that we can keep a pregenerated scaling factor */
void amitk_data_set_set_scale_factor(AmitkDataSet * ds, amide_data_t new_scale_factor) {

  AmitkVoxel i;
  gint j;
  amide_data_t scaling;
  gboolean need_update = FALSE;

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  g_return_if_fail(ds->internal_scaling != NULL);

  if (new_scale_factor <= 0.0) return;

  if ((ds->scale_factor != new_scale_factor) || 
      (ds->current_scaling == NULL) ||
      (ds->current_scaling->dim.x != ds->internal_scaling->dim.x) ||
      (ds->current_scaling->dim.y != ds->internal_scaling->dim.y) ||
      (ds->current_scaling->dim.z != ds->internal_scaling->dim.z) ||
      (ds->current_scaling->dim.t != ds->internal_scaling->dim.t)) {
    need_update = TRUE;
  } else { /* make absolutely sure we don't need an update */
    for(i.t = 0; i.t < ds->current_scaling->dim.t; i.t++) 
      for (i.z = 0; i.z < ds->current_scaling->dim.z; i.z++) 
	for (i.y = 0; i.y < ds->current_scaling->dim.y; i.y++) 
	  for (i.x = 0; i.x < ds->current_scaling->dim.x; i.x++)
	    if (AMITK_RAW_DATA_DOUBLE_SET_CONTENT(ds->current_scaling, i) != 
		ds->scale_factor *
		AMITK_RAW_DATA_DOUBLE_CONTENT(ds->internal_scaling, i))
	      need_update = TRUE;
  }


  if (need_update) {

    if (ds->current_scaling == NULL) /* first time */
      scaling = 1.0;
    else
      scaling = new_scale_factor/ds->scale_factor;

    ds->scale_factor = new_scale_factor;
    if (ds->current_scaling != NULL)
      if ((ds->current_scaling->dim.x != ds->internal_scaling->dim.x) ||
	  (ds->current_scaling->dim.y != ds->internal_scaling->dim.y) ||
	  (ds->current_scaling->dim.z != ds->internal_scaling->dim.z) ||
	  (ds->current_scaling->dim.t != ds->internal_scaling->dim.t)) {
	g_object_unref(ds->current_scaling);
	ds->current_scaling = NULL;
      }
    
    if (ds->current_scaling == NULL) {
      ds->current_scaling = amitk_raw_data_new_with_data(ds->internal_scaling->format,
							 ds->internal_scaling->dim);
							 
      g_return_if_fail(ds->current_scaling != NULL);
    }
    
    for(i.t = 0; i.t < ds->current_scaling->dim.t; i.t++) 
      for (i.z = 0; i.z < ds->current_scaling->dim.z; i.z++) 
	for (i.y = 0; i.y < ds->current_scaling->dim.y; i.y++) 
	  for (i.x = 0; i.x < ds->current_scaling->dim.x; i.x++)
	    AMITK_RAW_DATA_DOUBLE_SET_CONTENT(ds->current_scaling, i) = 
	      ds->scale_factor *
	      AMITK_RAW_DATA_DOUBLE_CONTENT(ds->internal_scaling, i);
    
    /* adjust all thresholds and other variables so they remain constant 
       relative the change in scale factors */
    for (j=0; j<2; j++) {
      ds->threshold_max[j] *= scaling;
      ds->threshold_min[j] *= scaling;
    }
    ds->global_max *= scaling;
    ds->global_min *= scaling;
    if ((AMITK_DATA_SET_RAW_DATA(ds) != NULL) && (ds->frame_max != NULL) && (ds->frame_min != NULL))
      for (j=0; j < AMITK_DATA_SET_NUM_FRAMES(ds); j++) {
	ds->frame_max[j] *= scaling;
	ds->frame_min[j] *= scaling;
      }

    /* and emit the signal */
    g_signal_emit (G_OBJECT (ds), data_set_signals[SCALE_FACTOR_CHANGED], 0);
    g_signal_emit (G_OBJECT (ds), data_set_signals[INVALIDATE_SLICE_CACHE], 0);
    g_signal_emit (G_OBJECT (ds), data_set_signals[DATA_SET_CHANGED], 0);
    g_signal_emit(G_OBJECT (ds), data_set_signals[THRESHOLDING_CHANGED], 0);
  }

  return;
}


static amide_data_t calculate_scale_factor(AmitkDataSet * ds) {

  amide_data_t value;

  switch(ds->conversion) {
  case AMITK_CONVERSION_STRAIGHT:
    value = AMITK_DATA_SET_SCALE_FACTOR(ds);
    break;
  case AMITK_CONVERSION_PERCENT_ID_PER_G:
    if (EQUAL_ZERO(ds->injected_dose))
      value = 1.0;
    else
      value = 100.0*ds->cylinder_factor/ds->injected_dose;
    break;
  case AMITK_CONVERSION_SUV:
    if (EQUAL_ZERO(ds->subject_weight)) 
      value = 1.0;
    else if (EQUAL_ZERO(ds->injected_dose))
      value = 1.0;
    else
      value = ds->cylinder_factor/(ds->injected_dose/ds->subject_weight);
    break;
  default:
    value = 1.0;
    g_error("unexpected case in %s at line %d",__FILE__, __LINE__);
    break;
  }

  return value;
}

void amitk_data_set_set_conversion(AmitkDataSet * ds, AmitkConversion new_conversion) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));

  if (ds->conversion != new_conversion) {
    ds->conversion = new_conversion;
    amitk_data_set_set_scale_factor(ds, calculate_scale_factor(ds));
    g_signal_emit(G_OBJECT (ds), data_set_signals[CONVERSION_CHANGED], 0);
  }
}

void amitk_data_set_set_injected_dose(AmitkDataSet * ds,amide_data_t new_injected_dose) {
  g_return_if_fail(AMITK_IS_DATA_SET(ds));

  if (ds->injected_dose != new_injected_dose) {
    ds->injected_dose = new_injected_dose;
    amitk_data_set_set_scale_factor(ds, calculate_scale_factor(ds));
  }
}

void amitk_data_set_set_subject_weight(AmitkDataSet * ds,amide_data_t new_subject_weight) {
  g_return_if_fail(AMITK_IS_DATA_SET(ds));

  if (ds->subject_weight != new_subject_weight) {
    ds->subject_weight = new_subject_weight;
    amitk_data_set_set_scale_factor(ds, calculate_scale_factor(ds));
  }
}

void amitk_data_set_set_cylinder_factor(AmitkDataSet * ds, amide_data_t new_cylinder_factor) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));

  if (ds->cylinder_factor != new_cylinder_factor) {
    ds->cylinder_factor = new_cylinder_factor;
    amitk_data_set_set_scale_factor(ds, calculate_scale_factor(ds));
  }
}

void amitk_data_set_set_displayed_dose_unit(AmitkDataSet * ds, AmitkDoseUnit new_dose_unit) {
  ds->displayed_dose_unit = new_dose_unit;
}

void amitk_data_set_set_displayed_weight_unit(AmitkDataSet * ds, AmitkWeightUnit new_weight_unit) {
  ds->displayed_weight_unit = new_weight_unit;
}

void amitk_data_set_set_displayed_cylinder_unit(AmitkDataSet * ds, AmitkCylinderUnit new_cylinder_unit) {
  ds->displayed_cylinder_unit = new_cylinder_unit;
}



/* returns the start time of the given frame */
amide_time_t amitk_data_set_get_start_time(const AmitkDataSet * ds, const guint frame) {

  amide_time_t time;
  guint i_frame;
  guint check_frame;

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), 0.0);

  if (frame >= AMITK_DATA_SET_NUM_FRAMES(ds))
    check_frame = AMITK_DATA_SET_NUM_FRAMES(ds)-1;
  else
    check_frame = frame;

  time = ds->scan_start;

  for(i_frame=0;i_frame<check_frame;i_frame++)
    time += ds->frame_duration[i_frame];

  return time;
}

/* returns the end time of a given frame */
amide_time_t amitk_data_set_get_end_time(const AmitkDataSet * ds, const guint frame) {

  guint check_frame;
  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), 1.0);


  if (frame >= AMITK_DATA_SET_NUM_FRAMES(ds))
    check_frame = AMITK_DATA_SET_NUM_FRAMES(ds)-1;
  else
    check_frame = frame;

  return 
    amitk_data_set_get_start_time(ds, check_frame) + 
    ds->frame_duration[check_frame];
}

/* returns the midpoint of a given frame */
amide_time_t amitk_data_set_get_midpt_time(const AmitkDataSet * ds, const guint frame) {

  guint check_frame;
  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), 1.0);


  if (frame >= AMITK_DATA_SET_NUM_FRAMES(ds))
    check_frame = AMITK_DATA_SET_NUM_FRAMES(ds)-1;
  else
    check_frame = frame;

  return 
    amitk_data_set_get_start_time(ds, check_frame) + 
    ds->frame_duration[check_frame]/2.0;
}


/* returns the frame that corresponds to the time */
guint amitk_data_set_get_frame(const AmitkDataSet * ds, const amide_time_t time) {

  amide_time_t start, end;
  guint i_frame;

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), 0);

  start = amitk_data_set_get_start_time(ds, 0);
  if (time <= start)
    return 0;

  for(i_frame=0; i_frame < AMITK_DATA_SET_NUM_FRAMES(ds); i_frame++) {
    start = amitk_data_set_get_start_time(ds, i_frame);
    end = amitk_data_set_get_end_time(ds, i_frame);
    if ((start <= time) && (time <= end))
      return i_frame;
  }

  /* must be past the end */
  return AMITK_DATA_SET_NUM_FRAMES(ds)-1;
}

amide_time_t amitk_data_set_get_frame_duration (const AmitkDataSet * ds, guint frame) {

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), 1.0);

  if (frame >= AMITK_DATA_SET_NUM_FRAMES(ds))
    frame = AMITK_DATA_SET_NUM_FRAMES(ds)-1;

  return ds->frame_duration[frame];
}

/* return the minimal frame duration in this data set */
amide_time_t amitk_data_set_get_min_frame_duration(const AmitkDataSet * ds) {

  amide_time_t min_frame_duration;
  guint i_frame;

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), 1.0);

  min_frame_duration = ds->frame_duration[0];
  
  for(i_frame=1;i_frame<AMITK_DATA_SET_NUM_FRAMES(ds);i_frame++)
    if (ds->frame_duration[i_frame] < min_frame_duration)
      min_frame_duration = ds->frame_duration[i_frame];

  return min_frame_duration;
}

/* function to recalculate the far corner of a data set */
void amitk_data_set_calc_far_corner(AmitkDataSet * ds) {

  AmitkPoint new_point;

  g_return_if_fail(AMITK_IS_DATA_SET(ds));

  POINT_MULT(ds->raw_data->dim, ds->voxel_size, new_point);
  amitk_volume_set_corner(AMITK_VOLUME(ds), new_point);

  return;
}



/* function to calculate the max and min over the data frames */
void amitk_data_set_calc_max_min(AmitkDataSet * ds,
				 gboolean (*update_func)(),
				 gpointer update_data) {

  guint i;

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  g_return_if_fail(ds->raw_data != NULL);

  /* allocate the arrays if we haven't already */
  if (ds->frame_max == NULL) {
    ds->frame_max = amitk_data_set_get_frame_max_min_mem(ds);
    ds->frame_min = amitk_data_set_get_frame_max_min_mem(ds);
  }
  g_return_if_fail(ds->frame_max != NULL);
  g_return_if_fail(ds->frame_min != NULL);

  /* hand everything off to the data type specific function */
  switch(ds->raw_data->format) {
  case AMITK_FORMAT_UBYTE:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D)
      amitk_data_set_UBYTE_2D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      amitk_data_set_UBYTE_1D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    else 
      amitk_data_set_UBYTE_0D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    break;
  case AMITK_FORMAT_SBYTE:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      amitk_data_set_SBYTE_2D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      amitk_data_set_SBYTE_1D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    else 
      amitk_data_set_SBYTE_0D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    break;
  case AMITK_FORMAT_USHORT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      amitk_data_set_USHORT_2D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      amitk_data_set_USHORT_1D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    else 
      amitk_data_set_USHORT_0D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    break;
  case AMITK_FORMAT_SSHORT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      amitk_data_set_SSHORT_2D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      amitk_data_set_SSHORT_1D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    else 
      amitk_data_set_SSHORT_0D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    break;
  case AMITK_FORMAT_UINT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      amitk_data_set_UINT_2D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      amitk_data_set_UINT_1D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    else 
      amitk_data_set_UINT_0D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    break;
  case AMITK_FORMAT_SINT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      amitk_data_set_SINT_2D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      amitk_data_set_SINT_1D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    else 
      amitk_data_set_SINT_0D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    break;
  case AMITK_FORMAT_FLOAT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      amitk_data_set_FLOAT_2D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      amitk_data_set_FLOAT_1D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    else 
      amitk_data_set_FLOAT_0D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    break;
  case AMITK_FORMAT_DOUBLE:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      amitk_data_set_DOUBLE_2D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      amitk_data_set_DOUBLE_1D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    else 
      amitk_data_set_DOUBLE_0D_SCALING_calc_frame_max_min(ds, update_func, update_data);
    break;
  default:
    g_error("unexpected case in %s at line %d",__FILE__, __LINE__);
    break;
  }


  /* calc the global max/min */
  ds->global_max = ds->frame_max[0];
  ds->global_min = ds->frame_min[0];
  for (i=1; i<AMITK_DATA_SET_NUM_FRAMES(ds); i++) {
    if (ds->global_max < ds->frame_max[i]) 
      ds->global_max = ds->frame_max[i];
    if (ds->global_min > ds->frame_min[i])
      ds->global_min = ds->frame_min[i];
  }

#ifdef AMIDE_DEBUG
  if (AMITK_DATA_SET_DIM_Z(ds) > 1) /* don't print for slices */
    g_print("\tglobal max %5.3f global min %5.3f\n",ds->global_max,ds->global_min);
#endif
   
  return;
}


amide_data_t amitk_data_set_get_max(const AmitkDataSet * ds, 
				    const amide_time_t start, 
				    const amide_time_t duration) {

  guint start_frame;
  guint end_frame;
  guint i_frame;
  amide_time_t used_start, used_end;
  amide_time_t ds_start, ds_end;
  amide_data_t max;
  amide_data_t time_weight;

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), 1.0);
  g_return_val_if_fail(duration >= 0.0, 1.0);

  /* figure out what frames of this data set to use */
  used_start = start;
  start_frame = amitk_data_set_get_frame(ds, used_start);
  ds_start = amitk_data_set_get_start_time(ds, start_frame);

  used_end = start+duration;
  end_frame = amitk_data_set_get_frame(ds, used_end);
  ds_end = amitk_data_set_get_end_time(ds, end_frame);

  if (start_frame == end_frame) {
    max = ds->frame_max[start_frame];
  } else {
    if (ds_end < used_end)
      used_end = ds_end;
    if (ds_start > used_start)
      used_start = ds_start;

    max = 0;
    for (i_frame=start_frame; i_frame <= end_frame; i_frame++) {

      if (i_frame == start_frame)
	time_weight = (amitk_data_set_get_end_time(ds, start_frame)-used_start)/(used_end-used_start);
      else if (i_frame == end_frame)
	time_weight = (used_end-amitk_data_set_get_start_time(ds, end_frame))/(used_end-used_start);
      else
	time_weight = ds->frame_duration[i_frame]/(used_end-used_start);

      max += time_weight*ds->frame_max[i_frame];
    }
  }

  return max;
}

amide_data_t amitk_data_set_get_min(const AmitkDataSet * ds, 
				    const amide_time_t start, 
				    const amide_time_t duration) {

  guint start_frame;
  guint end_frame;
  guint i_frame;
  amide_time_t used_start, used_end;
  amide_time_t ds_start, ds_end;
  amide_data_t min;
  amide_data_t time_weight;

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), 0.0);
  g_return_val_if_fail(duration>=0.0, 0.0);

  /* figure out what frames of this data set to use */
  used_start = start;
  start_frame = amitk_data_set_get_frame(ds, used_start);
  ds_start = amitk_data_set_get_start_time(ds, start_frame);

  used_end = start+duration;
  end_frame = amitk_data_set_get_frame(ds, used_end);
  ds_end = amitk_data_set_get_end_time(ds, end_frame);

  if (start_frame == end_frame) {
    min = ds->frame_min[start_frame];
  } else {
    if (ds_end < used_end)
      used_end = ds_end;
    if (ds_start > used_start)
      used_start = ds_start;

    min = 0;
    for (i_frame=start_frame; i_frame <= end_frame; i_frame++) {

      if (i_frame == start_frame)
	time_weight = (amitk_data_set_get_end_time(ds, start_frame)-used_start)/(used_end-used_start);
      else if (i_frame == end_frame)
	time_weight = (used_end-amitk_data_set_get_start_time(ds, end_frame))/(used_end-used_start);
      else
	time_weight = ds->frame_duration[i_frame]/(used_end-used_start);

      min += time_weight*ds->frame_min[i_frame];
    }
  }

  return min;
}

/* figure out the min and max threshold values given a threshold type */
/* slice is only needed for THRESHOLDING_PER_SLICE */
/* valid values for start and duration only needed for THRESHOLDING_PER_FRAME
   and THRESHOLDING_INTERPOLATE_FRAMES */
void amitk_data_set_get_thresholding_max_min(const AmitkDataSet * ds, 
					     const AmitkDataSet * slice,
					     const amide_time_t start,
					     const amide_time_t duration,
					     amide_data_t * max, amide_data_t * min) {


  g_return_if_fail(AMITK_IS_DATA_SET(ds));

  /* get the max/min values for thresholding */
  switch(AMITK_DATA_SET_THRESHOLDING(ds)) {

  case AMITK_THRESHOLDING_PER_SLICE: 
    {
      g_return_if_fail(AMITK_IS_DATA_SET(slice));
      amide_data_t slice_max,slice_min;
      amide_data_t threshold_range;
      
      /* find the slice's max and min, and then adjust these values to
       * correspond to the current threshold values */
      slice_max = slice->global_max;
      slice_min = slice->global_min;
      threshold_range = ds->global_max-ds->global_min;
      *max = ds->threshold_max[0]*(slice_max-slice_min)/threshold_range;
      *min = ds->threshold_min[0]*(slice_max-slice_min)/threshold_range;
    }
    break;
  case AMITK_THRESHOLDING_PER_FRAME:
    {
      amide_data_t frame_max,frame_min;
      amide_data_t threshold_range;

      frame_max = amitk_data_set_get_max(ds, start,duration);
      frame_min = amitk_data_set_get_min(ds, start,duration);
      threshold_range = ds->global_max-ds->global_min;
      *max = ds->threshold_max[0]*(frame_max-frame_min)/threshold_range;
      *min = ds->threshold_min[0]*(frame_max-frame_min)/threshold_range;
    }
    break;
  case AMITK_THRESHOLDING_INTERPOLATE_FRAMES:
    {
      guint middle_frame;
    
    
      if (ds->threshold_ref_frame[1]==ds->threshold_ref_frame[0]) {
	*max = ds->threshold_max[0];
	*min = ds->threshold_min[0];
      } else {
	
	middle_frame = amitk_data_set_get_frame(ds, start+duration/2.0);
	if (middle_frame < ds->threshold_ref_frame[0])
	  middle_frame = ds->threshold_ref_frame[0];
	else if (middle_frame > ds->threshold_ref_frame[1])
	  middle_frame = ds->threshold_ref_frame[1];
	
	*max=
	  (((ds->threshold_ref_frame[1]-middle_frame)*ds->threshold_max[0]+
	    (middle_frame-ds->threshold_ref_frame[0])*ds->threshold_max[1])
	   /(ds->threshold_ref_frame[1]-ds->threshold_ref_frame[0]));
	*min=
	  (((ds->threshold_ref_frame[1]-middle_frame)*ds->threshold_min[0]+
	    (middle_frame-ds->threshold_ref_frame[0])*ds->threshold_min[1])
	   /(ds->threshold_ref_frame[1]-ds->threshold_ref_frame[0]));
      }
    }
    break;
  case AMITK_THRESHOLDING_GLOBAL:
    *max = ds->threshold_max[0];
    *min = ds->threshold_min[0];
    break;
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    break;
  }
 
  return;
}

  




/* generate the distribution array for a data set */
void amitk_data_set_calc_distribution(AmitkDataSet * ds, 
				      gboolean (*update_func)(), 
				      gpointer update_data) {


  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  g_return_if_fail(ds->raw_data != NULL);

  /* check that the distribution is the right size.  This may not be the case
     if we've changed AMITK_DATA_SET_DISTRIBUTION_SIZE, and we've loaded
     in an old file */
  if (ds->distribution != NULL)
    if (AMITK_RAW_DATA_DIM_X(ds->distribution) != AMITK_DATA_SET_DISTRIBUTION_SIZE) {
      g_object_unref(ds->distribution);
      ds->distribution = NULL;
    }


  /* hand everything off to the data type specific function */
  switch(ds->raw_data->format) {
  case AMITK_FORMAT_UBYTE:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      amitk_data_set_UBYTE_2D_SCALING_calc_distribution(ds, update_func, update_data);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      amitk_data_set_UBYTE_1D_SCALING_calc_distribution(ds, update_func, update_data);
    else 
      amitk_data_set_UBYTE_0D_SCALING_calc_distribution(ds, update_func, update_data);
    break;
  case AMITK_FORMAT_SBYTE:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      amitk_data_set_SBYTE_2D_SCALING_calc_distribution(ds, update_func, update_data);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      amitk_data_set_SBYTE_1D_SCALING_calc_distribution(ds, update_func, update_data);
    else 
      amitk_data_set_SBYTE_0D_SCALING_calc_distribution(ds, update_func, update_data);
    break;
  case AMITK_FORMAT_USHORT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      amitk_data_set_USHORT_2D_SCALING_calc_distribution(ds, update_func, update_data);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      amitk_data_set_USHORT_1D_SCALING_calc_distribution(ds, update_func, update_data);
    else 
      amitk_data_set_USHORT_0D_SCALING_calc_distribution(ds, update_func, update_data);
    break;
  case AMITK_FORMAT_SSHORT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      amitk_data_set_SSHORT_2D_SCALING_calc_distribution(ds, update_func, update_data);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      amitk_data_set_SSHORT_1D_SCALING_calc_distribution(ds, update_func, update_data);
    else 
      amitk_data_set_SSHORT_0D_SCALING_calc_distribution(ds, update_func, update_data);
    break;
  case AMITK_FORMAT_UINT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      amitk_data_set_UINT_2D_SCALING_calc_distribution(ds, update_func, update_data);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      amitk_data_set_UINT_1D_SCALING_calc_distribution(ds, update_func, update_data);
    else 
      amitk_data_set_UINT_0D_SCALING_calc_distribution(ds, update_func, update_data);
    break;
  case AMITK_FORMAT_SINT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      amitk_data_set_SINT_2D_SCALING_calc_distribution(ds, update_func, update_data);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      amitk_data_set_SINT_1D_SCALING_calc_distribution(ds, update_func, update_data);
    else 
      amitk_data_set_SINT_0D_SCALING_calc_distribution(ds, update_func, update_data);
    break;
  case AMITK_FORMAT_FLOAT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      amitk_data_set_FLOAT_2D_SCALING_calc_distribution(ds, update_func, update_data);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      amitk_data_set_FLOAT_1D_SCALING_calc_distribution(ds, update_func, update_data);
    else 
      amitk_data_set_FLOAT_0D_SCALING_calc_distribution(ds, update_func, update_data);
    break;
  case AMITK_FORMAT_DOUBLE:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      amitk_data_set_DOUBLE_2D_SCALING_calc_distribution(ds, update_func, update_data);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      amitk_data_set_DOUBLE_1D_SCALING_calc_distribution(ds, update_func, update_data);
    else 
      amitk_data_set_DOUBLE_0D_SCALING_calc_distribution(ds, update_func, update_data);
    break;
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    break;
  }

  return;
}

  
amide_data_t amitk_data_set_get_value(const AmitkDataSet * ds, const AmitkVoxel i) {

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), EMPTY);
  
  if (!amitk_raw_data_includes_voxel(ds->raw_data, i)) return EMPTY;

  /* hand everything off to the data type specific function */
  switch(ds->raw_data->format) {
  case AMITK_FORMAT_UBYTE:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      return AMITK_DATA_SET_UBYTE_2D_SCALING_CONTENT(ds,i);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      return AMITK_DATA_SET_UBYTE_1D_SCALING_CONTENT(ds,i);
    else 
      return AMITK_DATA_SET_UBYTE_0D_SCALING_CONTENT(ds,i);
    break;
  case AMITK_FORMAT_SBYTE:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      return AMITK_DATA_SET_SBYTE_2D_SCALING_CONTENT(ds,i);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      return AMITK_DATA_SET_SBYTE_1D_SCALING_CONTENT(ds,i);
    else 
      return AMITK_DATA_SET_SBYTE_0D_SCALING_CONTENT(ds,i);
    break;
  case AMITK_FORMAT_USHORT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      return AMITK_DATA_SET_USHORT_2D_SCALING_CONTENT(ds,i);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      return AMITK_DATA_SET_USHORT_1D_SCALING_CONTENT(ds,i);
    else 
      return AMITK_DATA_SET_USHORT_0D_SCALING_CONTENT(ds,i);
    break;
  case AMITK_FORMAT_SSHORT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      return AMITK_DATA_SET_SSHORT_2D_SCALING_CONTENT(ds,i);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      return AMITK_DATA_SET_SSHORT_1D_SCALING_CONTENT(ds,i);
    else 
      return AMITK_DATA_SET_SSHORT_0D_SCALING_CONTENT(ds,i);
    break;
  case AMITK_FORMAT_UINT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      return AMITK_DATA_SET_UINT_2D_SCALING_CONTENT(ds,i);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      return AMITK_DATA_SET_UINT_1D_SCALING_CONTENT(ds,i);
    else 
      return AMITK_DATA_SET_UINT_0D_SCALING_CONTENT(ds,i);
    break;
  case AMITK_FORMAT_SINT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      return AMITK_DATA_SET_SINT_2D_SCALING_CONTENT(ds,i);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      return AMITK_DATA_SET_SINT_1D_SCALING_CONTENT(ds,i);
    else 
      return AMITK_DATA_SET_SINT_0D_SCALING_CONTENT(ds,i);
    break;
  case AMITK_FORMAT_FLOAT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      return AMITK_DATA_SET_FLOAT_2D_SCALING_CONTENT(ds,i);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      return AMITK_DATA_SET_FLOAT_1D_SCALING_CONTENT(ds,i);
    else 
      return AMITK_DATA_SET_FLOAT_0D_SCALING_CONTENT(ds,i);
    break;
  case AMITK_FORMAT_DOUBLE:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      return AMITK_DATA_SET_DOUBLE_2D_SCALING_CONTENT(ds,i);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      return AMITK_DATA_SET_DOUBLE_1D_SCALING_CONTENT(ds,i);
    else 
      return AMITK_DATA_SET_DOUBLE_0D_SCALING_CONTENT(ds,i);
    break;
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    return EMPTY;
    break;
  }
}



/* sets the current voxel to the given value.  If value/scaling is
   outside of the range of the data set type (i.e. negative for unsigned type),
   it will be truncated to lie at the limits of the range */
/* note, after using this function, you should
   recalculate the frame's max/min, along with the
   global max/min, and the distribution data */
void amitk_data_set_set_value(AmitkDataSet * ds, 
			      const AmitkVoxel i, 
			      const amide_data_t value,
			      const gboolean signal_change) {

  amide_data_t unscaled_value;

  g_return_if_fail(AMITK_IS_DATA_SET(ds));

  /* figure out what the value is unscaled */
  if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
    unscaled_value = value/(*AMITK_RAW_DATA_DOUBLE_2D_SCALING_POINTER((ds)->current_scaling, (i)));
  else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
    unscaled_value = value/(*AMITK_RAW_DATA_DOUBLE_1D_SCALING_POINTER((ds)->current_scaling, (i)));
  else 
    unscaled_value = value/(*AMITK_RAW_DATA_DOUBLE_0D_SCALING_POINTER((ds)->current_scaling, (i)));

  /* truncated unscaled value to type limits */
  if (unscaled_value < amitk_format_min[ds->raw_data->format])
    unscaled_value = amitk_format_min[ds->raw_data->format];
  else if (unscaled_value > amitk_format_max[ds->raw_data->format])
    unscaled_value = amitk_format_max[ds->raw_data->format];


  switch(ds->raw_data->format) {
  case AMITK_FORMAT_UBYTE:
    AMITK_RAW_DATA_UBYTE_SET_CONTENT((ds)->raw_data, (i)) = rint(unscaled_value);
    break;
  case AMITK_FORMAT_SBYTE:
    AMITK_RAW_DATA_SBYTE_SET_CONTENT((ds)->raw_data, (i)) = rint(unscaled_value);
    break;
  case AMITK_FORMAT_USHORT:
    AMITK_RAW_DATA_USHORT_SET_CONTENT((ds)->raw_data, (i)) = rint(unscaled_value);
    break;
  case AMITK_FORMAT_SSHORT:
    AMITK_RAW_DATA_SSHORT_SET_CONTENT((ds)->raw_data, (i)) = rint(unscaled_value);
    break;
  case AMITK_FORMAT_UINT:
    AMITK_RAW_DATA_UINT_SET_CONTENT((ds)->raw_data, (i)) = rint(unscaled_value);
    break;
  case AMITK_FORMAT_SINT:
    AMITK_RAW_DATA_SINT_SET_CONTENT((ds)->raw_data, (i)) = rint(unscaled_value);
    break;
  case AMITK_FORMAT_FLOAT:
    AMITK_RAW_DATA_FLOAT_SET_CONTENT((ds)->raw_data, (i)) = unscaled_value;
    break;
  case AMITK_FORMAT_DOUBLE:
    AMITK_RAW_DATA_DOUBLE_SET_CONTENT((ds)->raw_data, (i)) = unscaled_value;
    break;
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    break;
  }

  if (signal_change) {
    g_signal_emit (G_OBJECT (ds), data_set_signals[INVALIDATE_SLICE_CACHE], 0);
    g_signal_emit (G_OBJECT (ds), data_set_signals[DATA_SET_CHANGED], 0);
  }
}



/* returns a "2D" slice from a data set */
AmitkDataSet *amitk_data_set_get_slice(AmitkDataSet * ds,
				       const amide_time_t start,
				       const amide_time_t duration,
				       const amide_real_t  pixel_dim,
				       const AmitkVolume * slice_volume,
				       const gboolean need_calc_max_min) {

  AmitkDataSet * slice;

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), NULL);
  g_return_val_if_fail(ds->raw_data != NULL, NULL);

  /* hand everything off to the data type specific function */
  switch(ds->raw_data->format) {
  case AMITK_FORMAT_UBYTE:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      slice = amitk_data_set_UBYTE_2D_SCALING_get_slice(ds, start, duration, pixel_dim,
						slice_volume, need_calc_max_min);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      slice = amitk_data_set_UBYTE_1D_SCALING_get_slice(ds, start, duration, pixel_dim,
						slice_volume, need_calc_max_min);
    else 
      slice = amitk_data_set_UBYTE_0D_SCALING_get_slice(ds, start, duration, pixel_dim,
						slice_volume, need_calc_max_min);
    break;
  case AMITK_FORMAT_SBYTE:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      slice = amitk_data_set_SBYTE_2D_SCALING_get_slice(ds, start, duration, pixel_dim,
						slice_volume, need_calc_max_min);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      slice = amitk_data_set_SBYTE_1D_SCALING_get_slice(ds, start, duration, pixel_dim,
						slice_volume, need_calc_max_min);
    else 
      slice = amitk_data_set_SBYTE_0D_SCALING_get_slice(ds, start, duration, pixel_dim,
						slice_volume, need_calc_max_min);
    break;
  case AMITK_FORMAT_USHORT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      slice = amitk_data_set_USHORT_2D_SCALING_get_slice(ds, start, duration, pixel_dim,
						 slice_volume, need_calc_max_min);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      slice = amitk_data_set_USHORT_1D_SCALING_get_slice(ds, start, duration, pixel_dim,
						 slice_volume, need_calc_max_min);
    else 
      slice = amitk_data_set_USHORT_0D_SCALING_get_slice(ds, start, duration, pixel_dim,
						 slice_volume, need_calc_max_min);
    break;
  case AMITK_FORMAT_SSHORT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      slice = amitk_data_set_SSHORT_2D_SCALING_get_slice(ds, start, duration, pixel_dim,
						 slice_volume, need_calc_max_min);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      slice = amitk_data_set_SSHORT_1D_SCALING_get_slice(ds, start, duration, pixel_dim,
						 slice_volume, need_calc_max_min);
    else 
      slice = amitk_data_set_SSHORT_0D_SCALING_get_slice(ds, start, duration, pixel_dim,
						 slice_volume, need_calc_max_min);
    break;
  case AMITK_FORMAT_UINT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      slice = amitk_data_set_UINT_2D_SCALING_get_slice(ds, start, duration, pixel_dim,
					       slice_volume, need_calc_max_min);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      slice = amitk_data_set_UINT_1D_SCALING_get_slice(ds, start, duration, pixel_dim,
					       slice_volume, need_calc_max_min);
    else 
      slice = amitk_data_set_UINT_0D_SCALING_get_slice(ds, start, duration, pixel_dim,
					       slice_volume, need_calc_max_min);
    break;
  case AMITK_FORMAT_SINT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      slice = amitk_data_set_SINT_2D_SCALING_get_slice(ds, start, duration, pixel_dim,
					       slice_volume, need_calc_max_min);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      slice = amitk_data_set_SINT_1D_SCALING_get_slice(ds, start, duration, pixel_dim,
					       slice_volume, need_calc_max_min);
    else 
      slice = amitk_data_set_SINT_0D_SCALING_get_slice(ds, start, duration, pixel_dim,
					       slice_volume, need_calc_max_min);
    break;
  case AMITK_FORMAT_FLOAT:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      slice = amitk_data_set_FLOAT_2D_SCALING_get_slice(ds, start, duration, pixel_dim,
						slice_volume, need_calc_max_min);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      slice = amitk_data_set_FLOAT_1D_SCALING_get_slice(ds, start, duration, pixel_dim,
						slice_volume, need_calc_max_min);
    else 
      slice = amitk_data_set_FLOAT_0D_SCALING_get_slice(ds, start, duration, pixel_dim,
						slice_volume, need_calc_max_min);
    break;
  case AMITK_FORMAT_DOUBLE:
    if (ds->scaling_type == AMITK_SCALING_TYPE_2D) 
      slice = amitk_data_set_DOUBLE_2D_SCALING_get_slice(ds, start, duration, pixel_dim,
						 slice_volume, need_calc_max_min);
    else if (ds->scaling_type == AMITK_SCALING_TYPE_1D)
      slice = amitk_data_set_DOUBLE_1D_SCALING_get_slice(ds, start, duration, pixel_dim,
						 slice_volume, need_calc_max_min);
    else 
      slice = amitk_data_set_DOUBLE_0D_SCALING_get_slice(ds, start, duration, pixel_dim,
						 slice_volume, need_calc_max_min);
    break;
  default:
    slice = NULL;
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    break;
  }

  return slice;
}

/* return the three planar projections of the data set */
/* projections should be an array of 3 pointers to data sets */
void amitk_data_set_get_projections(AmitkDataSet * ds,
				    const guint frame,
				    AmitkDataSet ** projections,
				    gboolean (*update_func)(),
				    gpointer update_data) {

  AmitkVoxel dim, planar_dim, i;
  AmitkPoint voxel_size;
  amide_data_t normalizers[AMITK_VIEW_NUM];
  div_t x;
  gint divider;
  gboolean continue_work=TRUE;
  gchar * temp_string;
  AmitkView i_view;
  amide_data_t value;

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  g_return_if_fail(ds->raw_data != NULL);
  g_return_if_fail(projections != NULL);


  dim = AMITK_DATA_SET_DIM(ds);
  voxel_size = AMITK_DATA_SET_VOXEL_SIZE(ds);

  /* setup the wait dialog */
  if (update_func != NULL) {
    temp_string = g_strdup_printf(_("Generating projections of:\n   %s"), AMITK_OBJECT_NAME(ds));
    continue_work = (*update_func)(update_data, temp_string, (gdouble) 0.0);
    g_free(temp_string);
  }
  divider = ((dim.z/AMIDE_UPDATE_DIVIDER) < 1) ? 1 : (dim.z/AMIDE_UPDATE_DIVIDER);

  /* initialize the 3 projections */
  for (i_view=0; i_view < AMITK_VIEW_NUM; i_view++) {
    planar_dim.z = 1;
    planar_dim.t = 1;
    
    switch(i_view) {
    case AMITK_VIEW_CORONAL:
      planar_dim.x = dim.x;
      planar_dim.y = dim.z;
      break;
    case AMITK_VIEW_SAGITTAL:
      planar_dim.x = dim.y;
      planar_dim.y = dim.z;
      break;
    case AMITK_VIEW_TRANSVERSE:
    default:
      planar_dim.x = dim.x;
      planar_dim.y = dim.y;
      break;
    }

    projections[i_view] = amitk_data_set_new_with_data(AMITK_FORMAT_DOUBLE, planar_dim, AMITK_SCALING_TYPE_0D);
    if (projections[i_view] == NULL) {
      g_warning(_("couldn't allocate space for the projection, wanted %dx%dx%dx%d elements"), 
		planar_dim.x, planar_dim.y, planar_dim.z, planar_dim.t);
      return;
    }

    switch(i_view) {
    case AMITK_VIEW_CORONAL:
      projections[i_view]->voxel_size.x = voxel_size.x;
      projections[i_view]->voxel_size.y = voxel_size.z;
      projections[i_view]->voxel_size.z = AMITK_VOLUME_Y_CORNER(ds);
      normalizers[i_view] = voxel_size.y/projections[i_view]->voxel_size.z;
      break;
    case AMITK_VIEW_SAGITTAL:
      projections[i_view]->voxel_size.x = voxel_size.y;
      projections[i_view]->voxel_size.y = voxel_size.z;
      projections[i_view]->voxel_size.z = AMITK_VOLUME_X_CORNER(ds);
      normalizers[i_view] = voxel_size.x/projections[i_view]->voxel_size.z;
      break;
    case AMITK_VIEW_TRANSVERSE:
    default:
      projections[i_view]->voxel_size.x = voxel_size.x;
      projections[i_view]->voxel_size.y = voxel_size.y;
      projections[i_view]->voxel_size.z = AMITK_VOLUME_Z_CORNER(ds);
      normalizers[i_view] = voxel_size.z/projections[i_view]->voxel_size.z;
      break;
    }

    projections[i_view]->slice_parent = ds;
    g_object_add_weak_pointer(G_OBJECT(ds), 
			      (gpointer *) &(projections[i_view]->slice_parent));
    amitk_space_copy_in_place(AMITK_SPACE(projections[i_view]), AMITK_SPACE(ds));
    amitk_data_set_calc_far_corner(projections[i_view]);
    projections[i_view]->scan_start = amitk_data_set_get_start_time(ds, frame);
    amitk_data_set_set_thresholding(projections[i_view], AMITK_THRESHOLDING_GLOBAL);
    amitk_data_set_set_color_table(projections[i_view], AMITK_DATA_SET_COLOR_TABLE(ds));
    amitk_data_set_set_frame_duration(projections[i_view], 0, amitk_data_set_get_frame_duration(ds, frame));

    /* initialize our projection */
    amitk_raw_data_DOUBLE_initialize_data(projections[i_view]->raw_data, 0.0);
  }


  /* now iterate through the entire data set, adding up the 3 projections */
  i.t = frame;
  for (i.z = 0; (i.z < dim.z) && continue_work; i.z++) {

    if (update_func != NULL) {
      x = div(i.z,divider);
      if (x.rem == 0)
	continue_work = (*update_func)(update_data, NULL, (gdouble) (i.z)/dim.z);
    }

    for (i.y = 0; i.y < dim.y; i.y++) {
      for (i.x = 0; i.x < dim.x; i.x++) {
	value = amitk_data_set_get_value(ds, i);
	AMITK_RAW_DATA_DOUBLE_2D_SET_CONTENT(projections[AMITK_VIEW_TRANSVERSE]->raw_data,i.y, i.x) += value;
	AMITK_RAW_DATA_DOUBLE_2D_SET_CONTENT(projections[AMITK_VIEW_CORONAL]->raw_data,dim.z-i.z-1, i.x) += value;
	AMITK_RAW_DATA_DOUBLE_2D_SET_CONTENT(projections[AMITK_VIEW_SAGITTAL]->raw_data,dim.z-i.z-1, i.y) += value;
      }
    }
  }

  if (update_func != NULL) /* remove progress bar */
    continue_work = (*update_func)(update_data, NULL, (gdouble) 2.0);

  if (!continue_work) {/* we hit cancel */
    for (i_view=0; i_view<AMITK_VIEW_NUM; i_view++) {
      amitk_object_unref(projections[i_view]);
      projections[i_view] = NULL;
    }
    return;
  }
    

  /* normalize for the thickness, we're assuming the previous voxels were
     in some units of blah/mm^3 */
  /* this bit should be short, as the 3 are planar... not updating progress bar */
  for (i_view=0; i_view<AMITK_VIEW_NUM; i_view++) {
    for (i.y = 0; i.y < AMITK_DATA_SET_DIM_Y(projections[i_view]); i.y++)
      for (i.x = 0; i.x < AMITK_DATA_SET_DIM_X(projections[i_view]); i.x++)
	AMITK_RAW_DATA_DOUBLE_2D_SET_CONTENT(projections[i_view]->raw_data,i.y, i.x) *= normalizers[i_view];
  
    amitk_data_set_calc_max_min(projections[i_view], NULL, NULL); 
    amitk_data_set_set_threshold_max(projections[i_view], 0,
				     AMITK_DATA_SET_GLOBAL_MAX(projections[i_view]));
    amitk_data_set_set_threshold_min(projections[i_view], 0,
				     AMITK_DATA_SET_GLOBAL_MIN(projections[i_view]));
  }

  return;
}



/* returns a cropped version of the given data set */
AmitkDataSet *amitk_data_set_get_cropped(const AmitkDataSet * ds,
					 const AmitkVoxel start,
					 const AmitkVoxel end,
					 const AmitkFormat format,
					 const AmitkScalingType scaling_type,
					 gboolean (*update_func)(),
					 gpointer update_data) {

  AmitkDataSet * cropped;
  AmitkVoxel i, j;
  AmitkVoxel dim;
  AmitkVoxel scaling_dim;
  gchar * temp_string;
  AmitkPoint shift;
  AmitkPoint temp_pt;
  GList * children;
  gboolean same_format_and_scaling;
  gboolean unsigned_type;
  gboolean float_type;
  amide_data_t max, min, value;
  div_t x;
  gint divider;
  gint t_times_z;
  gint total_progress;
  gint scaling_progress;
  gint i_progress;
  gboolean continue_work=TRUE;


  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), NULL);
  g_return_val_if_fail(ds->raw_data != NULL, NULL);

  /* are we converting format and/or scaling? */
  if ((AMITK_DATA_SET_FORMAT(ds) == format) && (AMITK_DATA_SET_SCALING_TYPE(ds) == scaling_type))
    same_format_and_scaling = TRUE;
  else
    same_format_and_scaling = FALSE;

  /* are we converting to an unsigned type? */
  switch(format) {
  case AMITK_FORMAT_UBYTE:
  case AMITK_FORMAT_USHORT:
  case AMITK_FORMAT_UINT:
    unsigned_type = TRUE;
    float_type = FALSE;
    break;
  case AMITK_FORMAT_FLOAT:
  case AMITK_FORMAT_DOUBLE:
    unsigned_type = FALSE;
    float_type = TRUE;
    break;
  default:
    unsigned_type = FALSE;
    float_type = FALSE;
    break;
  }

  /* figure out the dimensions of the output data set and the scaling array */
  dim = voxel_add(voxel_sub(end, start), one_voxel);
  scaling_dim = one_voxel;
  if (scaling_type == AMITK_SCALING_TYPE_2D) {
    scaling_dim.t = dim.t;
    scaling_dim.z = dim.z;
  } else if (scaling_type == AMITK_SCALING_TYPE_1D) {
    scaling_dim.t = dim.t;
  }

  /* multiply t_times_z by two if we also need to iterate to find new scale factors */
  t_times_z = dim.t*dim.z;
  if (!same_format_and_scaling) scaling_progress = t_times_z;
  else scaling_progress = 0;
  total_progress = t_times_z + scaling_progress;
  
  divider = ((total_progress/AMIDE_UPDATE_DIVIDER) < 1) ? 1 : (total_progress/AMIDE_UPDATE_DIVIDER);

  cropped = AMITK_DATA_SET(amitk_object_copy(AMITK_OBJECT(ds)));

  /* start by unrefing the info that's only copied by reference by amitk_object_copy */
  if (cropped->raw_data != NULL) {
    g_object_unref(cropped->raw_data);
    cropped->raw_data = NULL;
  }
  
  if (cropped->internal_scaling != NULL) {
    g_object_unref(cropped->internal_scaling);
    cropped->internal_scaling = NULL;
  }

  if (cropped->distribution != NULL) {
    g_object_unref(cropped->distribution);
    cropped->distribution = NULL;  
  }

  /* and unref anything that's obviously now incorrect */
  if (cropped->current_scaling != NULL) {
    g_object_unref(cropped->current_scaling);
    cropped->current_scaling = NULL;
  }

  if (cropped->frame_duration != NULL) {
    g_free(cropped->frame_duration);
    cropped->frame_duration = NULL;
  }

  if (cropped->frame_max != NULL) {
    g_free(cropped->frame_max);
    cropped->frame_max = NULL;
  }

  if (cropped->frame_min != NULL) {
    g_free(cropped->frame_min);
    cropped->frame_min = NULL;
  }

  /* set a new name for this guy */
  temp_string = g_strdup_printf(_("%s, cropped"), AMITK_OBJECT_NAME(ds));
  amitk_object_set_name(AMITK_OBJECT(cropped), temp_string);
  g_free(temp_string);

  /* set the scale type */
  cropped->scaling_type = scaling_type;


  /* setup the scale factors */
  cropped->internal_scaling =  amitk_raw_data_new_with_data(AMITK_FORMAT_DOUBLE,scaling_dim);
  if (cropped->internal_scaling == NULL) {
    g_warning(_("couldn't allocate space for the cropped internal scaling structure"));
    goto error;
  }

  if (update_func != NULL) {
    temp_string = g_strdup_printf(_("Generating cropped version of:\n   %s"), AMITK_OBJECT_NAME(ds));
    continue_work = (*update_func)(update_data, temp_string, (gdouble) 0.0);
    g_free(temp_string);
  }
  
  /* now generate the scale factors */
  if (same_format_and_scaling) { 
    /* we'll just use the old scaling factors if applicable */
    
    i = zero_voxel;
    j = start;
    if (scaling_type == AMITK_SCALING_TYPE_2D) {
      for (i.t=0, j.t=start.t; j.t <= end.t; i.t++, j.t++)
	for (i.z=0, j.z=start.z; j.z <= end.z; i.z++, j.z++)
	  (*AMITK_RAW_DATA_DOUBLE_2D_SCALING_POINTER(cropped->internal_scaling, i)) =
	    *AMITK_RAW_DATA_DOUBLE_2D_SCALING_POINTER(ds->internal_scaling, j);
    } else if (scaling_type == AMITK_SCALING_TYPE_1D) {
      for (i.t=0, j.t=start.t; j.t <= end.t; i.t++, j.t++)
	(*AMITK_RAW_DATA_DOUBLE_1D_SCALING_POINTER(cropped->internal_scaling, i)) =
	  *AMITK_RAW_DATA_DOUBLE_1D_SCALING_POINTER(ds->internal_scaling, j);
    } else { /* 0D */ 
      (*AMITK_RAW_DATA_DOUBLE_0D_SCALING_POINTER(cropped->internal_scaling, i)) =
	*AMITK_RAW_DATA_DOUBLE_0D_SCALING_POINTER(ds->internal_scaling, j);
    }


  } else if (!float_type) { /* we're changing format/scaling type - generate new scaling factors */
    max = min = 0.0;
    for (i.t=0, j.t=start.t; j.t <= end.t; i.t++, j.t++) {
      for (i.z=0, j.z=start.z; (j.z <= end.z) && continue_work; i.z++, j.z++) {
	if (update_func != NULL) {
	  i_progress = i.t*dim.z+i.z;
	  x = div(i_progress,divider);
	  if (x.rem == 0)
	    continue_work = (*update_func)(update_data, NULL, (gdouble) (i_progress)/total_progress);
	}
	for (i.y=0, j.y=start.y; j.y <= end.y; i.y++, j.y++) {
	  for (i.x=0, j.x=start.x; j.x <= end.x; i.x++, j.x++) {
	    value = amitk_data_set_get_value(ds, i);
	    if (value > max) max = value;
	    else if (value < min) min = value;
	  }
	}

	if (scaling_type == AMITK_SCALING_TYPE_2D) {
	  if (!unsigned_type) max = MAX(fabs(min), max);
	  (*AMITK_RAW_DATA_DOUBLE_2D_SCALING_POINTER(cropped->internal_scaling, i)) = 
	    max/amitk_format_max[format];
	  max = min = 0.0;
	}
      }

      if (scaling_type == AMITK_SCALING_TYPE_1D) {
	if (!unsigned_type) max = MAX(fabs(min), max);
	(*AMITK_RAW_DATA_DOUBLE_1D_SCALING_POINTER(cropped->internal_scaling, i)) = 
	  max/amitk_format_max[format];
	max = min = 0.0;
      }
    }

    if (scaling_type == AMITK_SCALING_TYPE_0D) {
      if (!unsigned_type) max = MAX(fabs(min), max);
      (*AMITK_RAW_DATA_DOUBLE_0D_SCALING_POINTER(cropped->internal_scaling, i)) = 
	max/amitk_format_max[format];
      max = min = 0.0;
    }
  } else { /* floating point type, just set scaling factors to 1 */

    i = zero_voxel;
    if (scaling_type == AMITK_SCALING_TYPE_2D) {
      for (i.t=0; j.t < dim.t; i.t++)
	for (i.z=0; j.z < dim.z; i.z++)
	  (*AMITK_RAW_DATA_DOUBLE_2D_SCALING_POINTER(cropped->internal_scaling, i)) = 1.0;
    } else if (scaling_type == AMITK_SCALING_TYPE_1D) {
      for (i.t=0; i.t < dim.t; j.t++)
	(*AMITK_RAW_DATA_DOUBLE_1D_SCALING_POINTER(cropped->internal_scaling, i)) = 1.0;
    } else { /* 0D */ 
      (*AMITK_RAW_DATA_DOUBLE_0D_SCALING_POINTER(cropped->internal_scaling, i)) = 1.0;
    }

  }
    
  /* reset the current scaling array */
  amitk_data_set_set_scale_factor(cropped, AMITK_DATA_SET_SCALE_FACTOR(ds));



  /* and setup the data */
  cropped->raw_data = amitk_raw_data_new_with_data(format, voxel_add(voxel_sub(end, start), one_voxel));
  if (cropped->raw_data == NULL) {
    g_warning(_("couldn't allocate space for the cropped raw data set structure"));
    goto error;
  }


  /* copy the raw data on over */
  for (i.t=0, j.t=start.t; j.t <= end.t; i.t++, j.t++) {
    for (i.z=0, j.z=start.z; (j.z <= end.z) && continue_work; i.z++, j.z++) {
      if (update_func != NULL) {
	i_progress = i.t*dim.z+i.z+scaling_progress;
	x = div(i_progress,divider);
	if (x.rem == 0)
	  continue_work = (*update_func)(update_data, NULL, (gdouble) (i_progress)/total_progress);
      }
      for (i.y=0, j.y=start.y; j.y <= end.y; i.y++, j.y++) {
	if (same_format_and_scaling) {
	  i.x = 0;
	  j.x = start.x;
	  memcpy(amitk_raw_data_get_pointer(cropped->raw_data, i),
		 amitk_raw_data_get_pointer(ds->raw_data, j),
		 amitk_format_sizes[format]*dim.x);
	} else {
	  for (i.x=0, j.x=start.x; j.x <= end.x; i.x++, j.x++) {
	    value = amitk_data_set_get_value(ds, j);
	    amitk_data_set_set_value(cropped, i, value, FALSE);
	  }
	}
      }
    }
  }

  if (update_func != NULL) /* remove progress bar */
    continue_work = (*update_func)(update_data, NULL, (gdouble) 2.0); 

  if (!continue_work) goto error;



  /* setup the frame time array */
  cropped->scan_start = amitk_data_set_get_start_time(ds, start.t);
  cropped->frame_duration = amitk_data_set_get_frame_duration_mem(ds);
  for (i.t=0, j.t=start.t; j.t <= end.t; i.t++, j.t++) 
    amitk_data_set_set_frame_duration(cropped, i.t, 
				      amitk_data_set_get_frame_duration(ds, j.t));

  /* recalc the temporary parameters */
  amitk_data_set_calc_far_corner(cropped);
  amitk_data_set_calc_max_min(cropped, update_func, update_data);

  /* and shift the offset appropriately */
  POINT_MULT(start, AMITK_DATA_SET_VOXEL_SIZE(cropped), temp_pt);
  temp_pt = amitk_space_s2b(AMITK_SPACE(ds), temp_pt);
  shift = point_sub(temp_pt, AMITK_SPACE_OFFSET(ds));
  amitk_space_shift_offset(AMITK_SPACE(cropped), shift);

  /* and reshift the children, so they stay in the right spot */
  shift = point_neg(shift);
  children = AMITK_OBJECT_CHILDREN(cropped);
  while (children != NULL) {
    amitk_space_shift_offset(AMITK_SPACE(children->data), shift);
    children = children->next;
  }

  return cropped; 

error:
  amitk_object_unref(cropped);

  return NULL;
}


#ifdef AMIDE_LIBGSL_SUPPORT


/* fills the data set "filtered_ds", with the results of the kernel convolved to data_set */
/* assumptions:
   1- filtered_ds is of type FLOAT, 0D scaling
   2- scale of filtered_ds is 1.0
   3- kernel is of type DOUBLE, is in complex packed format, and has odd dimensions in x,y,z,
      and dimension 1 in t

   notes:
   1. kernel is modified (FFT'd) 
   2. don't have a separate function for each data type, as getting the data_set data,
   is a tiny fraction of the computational time, use amitk_data_set_get_value instead
 */
static void filter_fir(const AmitkDataSet * data_set,
		       AmitkDataSet * filtered_ds,
		       AmitkRawData * kernel,
		       AmitkVoxel kernel_size) {


  AmitkVoxel subset_size;
  AmitkVoxel i_outer;
  AmitkVoxel i_inner;
  AmitkVoxel j_inner;
  AmitkVoxel half;
  AmitkRawData * subset=NULL;
  gsl_fft_complex_wavetable * wavetable=NULL;
  gsl_fft_complex_workspace * workspace = NULL;

  g_return_if_fail(kernel_size.t == 1);
  g_return_if_fail((kernel_size.z & 0x1)); /* needs to be odd */
  g_return_if_fail((kernel_size.y & 0x1)); 
  g_return_if_fail((kernel_size.x & 0x1)); 
  g_return_if_fail(2*kernel_size.z < AMITK_FILTER_FFT_SIZE);
  g_return_if_fail(2*kernel_size.y < AMITK_FILTER_FFT_SIZE);
  g_return_if_fail(2*kernel_size.x < AMITK_FILTER_FFT_SIZE);

  /* initialize gsl's FFT stuff */
  wavetable = gsl_fft_complex_wavetable_alloc(AMITK_FILTER_FFT_SIZE);
  workspace = gsl_fft_complex_workspace_alloc (AMITK_FILTER_FFT_SIZE);
  if ((wavetable == NULL) || (workspace == NULL)) {
    g_warning(_("Filtering: Failed to allocate wavetable and workspace"));
    goto exit_strategy;
  }

  /* get space for our data subset*/
  if ((subset = amitk_raw_data_new()) == NULL) {
    g_warning(_("couldn't allocate space for the subset structure"));
    goto exit_strategy;
  }
  subset->format = AMITK_FORMAT_DOUBLE;
  subset->dim.t = 1;
  subset->dim.z = subset->dim.y = AMITK_FILTER_FFT_SIZE;
  subset->dim.x = 2*AMITK_FILTER_FFT_SIZE; /* real and complex parts */

  subset_size.t = 1;
  subset_size.z = AMITK_FILTER_FFT_SIZE-kernel_size.z+1;
  subset_size.y = AMITK_FILTER_FFT_SIZE-kernel_size.y+1;
  subset_size.x = AMITK_FILTER_FFT_SIZE-kernel_size.x+1;

  half.t = 0;
  half.z = kernel_size.z>>1;
  half.y = kernel_size.y>>1;
  half.x = kernel_size.x>>1;

  if ((subset->data = amitk_raw_data_get_data_mem(subset)) == NULL) {
    g_warning(_("Couldn't allocate space for the subset data"));
    goto exit_strategy;
  }

  /* FFT the kernel */
  amitk_filter_3D_FFT(kernel, wavetable, workspace);

  /* start the overlap and add FFT method */
  i_outer.t = i_inner.t = j_inner.t = 0;
  for (i_outer.t = 0; i_outer.t < AMITK_DATA_SET_DIM_T(data_set); i_outer.t++) {
    g_print("Filtering Frame %d\t", i_outer.t);
    for (i_outer.z = 0; i_outer.z < AMITK_DATA_SET_DIM_Z(data_set); i_outer.z+= subset_size.z) {
      for (i_outer.y = 0; i_outer.y < AMITK_DATA_SET_DIM_Y(data_set); i_outer.y+= subset_size.y) {
    	for (i_outer.x = 0; i_outer.x < AMITK_DATA_SET_DIM_X(data_set); i_outer.x+= subset_size.x) {
	  g_print(".");
  
	  /* initialize the subset */
	  for (j_inner.z = 0; j_inner.z < subset->dim.z; j_inner.z++) 
	    for (j_inner.y = 0; j_inner.y < subset->dim.y; j_inner.y++) 
	      for (j_inner.x = 0; j_inner.x < subset->dim.x; j_inner.x++) 
		AMITK_RAW_DATA_DOUBLE_SET_CONTENT(subset, j_inner) = 0.0;
	  
	  /* copy data over from the actual data set */
	  for (i_inner.z = 0, j_inner.z=0; 
	       ((i_inner.z < subset_size.z) && (i_inner.z+i_outer.z) < AMITK_DATA_SET_DIM_Z(data_set)); 
	       i_inner.z++, j_inner.z++) 
	    for (i_inner.y = 0, j_inner.y=0; 
		 ((i_inner.y < subset_size.y) && (i_inner.y+i_outer.y) < AMITK_DATA_SET_DIM_Y(data_set)); 
		 i_inner.y++, j_inner.y++) 
	      for (i_inner.x = 0, j_inner.x=0; 
		   ((i_inner.x < subset_size.x) && (i_inner.x+i_outer.x) < AMITK_DATA_SET_DIM_X(data_set)); 
		   i_inner.x++, j_inner.x+=2) 
		AMITK_RAW_DATA_DOUBLE_SET_CONTENT(subset, j_inner) = 
		  amitk_data_set_get_value(data_set, voxel_add(i_outer, i_inner)); /* should be zero for out of range */
	  
	  
	  /* FFT the data */
	  amitk_filter_3D_FFT(subset, wavetable, workspace);
	  
	  /* multiple the data by the filter */
	  amitk_filter_complex_mult(subset, kernel);
	  
	  /* and inverse FFT */
	  amitk_filter_inverse_3D_FFT(subset, wavetable, workspace);
	  
	  
	  /* and add in, at the same time we're shifting the data set over by half the
	     kernel size */
	  for (((i_inner.z = (i_outer.z == 0) ? 0 : -half.z),
		  (j_inner.z = (i_outer.z == 0) ? half.z : 0)); 
	       ((j_inner.z < AMITK_FILTER_FFT_SIZE) && 
		((i_inner.z+i_outer.z) < AMITK_DATA_SET_DIM_Z(filtered_ds))); 
	       i_inner.z++, j_inner.z++)
	    for (((i_inner.y = (i_outer.y == 0) ? 0 : -half.y),
		  (j_inner.y = (i_outer.y == 0) ? half.y : 0)); 
		 ((j_inner.y < AMITK_FILTER_FFT_SIZE) && ((i_inner.y+i_outer.y) < AMITK_DATA_SET_DIM_Y(filtered_ds))); 
		 i_inner.y++, j_inner.y++) 
	      for (((i_inner.x = (i_outer.x == 0) ? 0 : -half.x),
		    (j_inner.x = (i_outer.x == 0) ? 2*half.x : 0)); 
		   ((j_inner.x < 2*AMITK_FILTER_FFT_SIZE) && ((i_inner.x+i_outer.x) < AMITK_DATA_SET_DIM_X(filtered_ds))); 
		   i_inner.x++, j_inner.x+=2) 
		AMITK_RAW_DATA_FLOAT_SET_CONTENT(filtered_ds->raw_data, voxel_add(i_outer,i_inner)) +=
		  AMITK_RAW_DATA_DOUBLE_CONTENT(subset, j_inner);


	}
      }
    }
#ifdef AMIDE_DEBUG
    g_print("\n");
#endif
  } /* i_outer.t */






 exit_strategy:

  if (wavetable != NULL) {
    gsl_fft_complex_wavetable_free(wavetable);
    wavetable = NULL;
  }

  if (workspace != NULL) {
    gsl_fft_complex_workspace_free(workspace);
    workspace = NULL;
  }
  
  if (subset != NULL) {
    g_object_unref(subset);
    subset = NULL;
  }

  return;

}

#endif

/* assumptions:
   1- filtered_ds is of type FLOAT, 0D scaling
   2- scale of filtered_ds is 1.0
   3- kernel dimensions are odd

   notes:
   1. don't have a separate function for each data type, as getting the data_set data,
   is a tiny fraction of the computational time, use amitk_data_set_get_value instead
   2. data set can be the same as filtered_ds
 */
void filter_median_3D(const AmitkDataSet * data_set, AmitkDataSet * filtered_ds,
		      AmitkVoxel kernel_dim) {

  amide_data_t * partial_sort_data;
  AmitkVoxel i,j, mid_dim, output_dim;
  gint loc, median_size;
  AmitkRawData * output_data;
  AmitkVoxel ds_dim;
#if AMIDE_DEBUG
  div_t x;
  gint divider;

#endif

  g_return_if_fail(AMITK_IS_DATA_SET(data_set));
  g_return_if_fail(AMITK_IS_DATA_SET(filtered_ds));
  g_return_if_fail(VOXEL_EQUAL(AMITK_DATA_SET_DIM(data_set), AMITK_DATA_SET_DIM(filtered_ds)));

  g_return_if_fail(AMITK_RAW_DATA_FORMAT(AMITK_DATA_SET_RAW_DATA(filtered_ds)) == AMITK_FORMAT_FLOAT);
  g_return_if_fail(REAL_EQUAL(AMITK_DATA_SET_SCALE_FACTOR(filtered_ds), 1.0));
  g_return_if_fail(VOXEL_EQUAL(AMITK_RAW_DATA_DIM(filtered_ds->internal_scaling), one_voxel));
  g_return_if_fail(kernel_dim.t == 1); /* haven't written support yet */

  /* check it's odd */
  g_return_if_fail(kernel_dim.x & 0x1);
  g_return_if_fail(kernel_dim.y & 0x1);
  g_return_if_fail(kernel_dim.z & 0x1);

  ds_dim = AMITK_DATA_SET_DIM(data_set);
  if (ds_dim.z < kernel_dim.z) {
    kernel_dim.z = 1;
    g_warning(_("data set z dimension to small for kernel, setting kernel dimension to 1"));
  }
  if (ds_dim.y < kernel_dim.y) {
    kernel_dim.y = 1;
    g_warning(_("data set y dimension to small for kernel, setting kernel dimension to 1"));
  }
  if (ds_dim.x < kernel_dim.x) {
    kernel_dim.x = 1;
    g_warning(_("data set x dimension to small for kernel, setting kernel dimension to 1"));
  }

  mid_dim.t = 0;
  mid_dim.z = kernel_dim.z >> 1;
  mid_dim.y = kernel_dim.y >> 1;
  mid_dim.x = kernel_dim.x >> 1;

  median_size = kernel_dim.z*kernel_dim.y*kernel_dim.x;

  output_dim = ds_dim;
  output_dim.t = 1;
  if ((output_data = amitk_raw_data_new_with_data(AMITK_FORMAT_FLOAT, output_dim)) == NULL) {
    g_warning(_("couldn't allocate space for the internal raw data"));
    return;
  }
  amitk_raw_data_FLOAT_initialize_data(output_data, 0.0);
  partial_sort_data = g_try_new(amide_data_t, median_size);
  g_return_if_fail(partial_sort_data != NULL);


  /* iterate over all the voxels in the data_set */
  i.t = 0;
  for (j.t=0; j.t < AMITK_DATA_SET_NUM_FRAMES(data_set); j.t++) {
#ifdef AMIDE_DEBUG
    divider = ((output_dim.z/20.0) < 1) ? 1 : (output_dim.z/20.0);
    g_print("Filtering Frame %d\t", j.t);
#endif
    for (i.z=0; i.z < output_dim.z; i.z++) {
#ifdef AMIDE_DEBUG
      x = div(i.z,divider);
      if (x.rem == 0) g_print(".");
#endif 
      for (i.y=0; i.y < output_dim.y; i.y++) {
	for (i.x=0; i.x < output_dim.x; i.x++) {
	    
	    /* initialize the data for the iteration */
	    loc = 0;
	    for (j.z = i.z-mid_dim.z; j.z <= i.z+mid_dim.z; j.z++) {
	      if ((j.z < 0) || (j.z >= ds_dim.z)) {
		for (j.y=0; j.y < kernel_dim.y; j.y++) {
		  for (j.x=0; j.x < kernel_dim.x; j.x++) {
		    partial_sort_data[loc] = 0.0;
		    loc++;
		  }
		}
	      } else {
		for (j.y = i.y-mid_dim.y; j.y <= i.y+mid_dim.y; j.y++) {
		  if ((j.y < 0) || (j.y >= ds_dim.y)) {
		    for (j.x=0; j.x < kernel_dim.x; j.x++) {
		      partial_sort_data[loc] = 0.0;
		      loc++;
		    }
		  } else {
		    for (j.x = i.x-mid_dim.x; j.x <= i.x+mid_dim.x; j.x++) {
		      if ((j.x < 0) || (j.x >= ds_dim.x)) {
			partial_sort_data[loc] = 0.0;
			loc++;
		      } else {
			partial_sort_data[loc] = amitk_data_set_get_value(data_set, j);
			loc++;
		      }
		    }
		  }
		}
	      }
	    } /* end initializing data */

	    // remove 
	    if (loc != median_size) {
	      g_print("initialize descrepency : %d %d\n", loc, median_size);
	    }
	    
	    /* and store median value */
	    AMITK_RAW_DATA_FLOAT_SET_CONTENT(output_data, i) = 
	      amitk_filter_find_median_by_partial_sort(partial_sort_data, median_size);

	  } /* i.x */
	} /* i.y */
      } /* i.z */


    /* copy the output_data over into the filtered_ds */
    for (i.z=0, j.z=0; i.z < output_dim.z; i.z++, j.z++)
      for (i.y=0, j.y=0; i.y < output_dim.y; i.y++, j.y++)
	for (i.x=0, j.x=0; i.x < output_dim.x; i.x++, j.x++)
	  AMITK_RAW_DATA_FLOAT_SET_CONTENT(filtered_ds->raw_data, j) = 
	    AMITK_RAW_DATA_FLOAT_CONTENT(output_data, i);
#ifdef AMIDE_DEBUG
    g_print("\n");
#endif 
  } /* j.t */

  /* garbage collection */
  g_object_unref(output_data); 
  g_free(partial_sort_data);

  return;
}
  
/* see notes for filter_median_3D */
void filter_median_linear(const AmitkDataSet * data_set,
			  AmitkDataSet * filtered_ds,
			  const gint kernel_size) {

  AmitkVoxel kernel_dim;

  kernel_dim.x = kernel_size;
  kernel_dim.y = 1;
  kernel_dim.z = 1;
  kernel_dim.t = 1;
  filter_median_3D(data_set, filtered_ds, kernel_dim);
  
  kernel_dim.x = 1;
  kernel_dim.y = kernel_size;
  kernel_dim.z = 1;
  kernel_dim.t = 1;
  filter_median_3D(filtered_ds,filtered_ds,kernel_dim);
  
  kernel_dim.x = 1;
  kernel_dim.y = 1;
  kernel_dim.z = kernel_size;
  kernel_dim.t = 1;
  filter_median_3D(filtered_ds,filtered_ds,kernel_dim);

  return;
}



/* returns a filtered version of the given data set */
AmitkDataSet *amitk_data_set_get_filtered(const AmitkDataSet * ds,
					  const AmitkFilter filter_type,
					  const gint kernel_size,
					  const amide_real_t fwhm, 
					  gboolean (*update_func)(),
					  gpointer update_data) {


  AmitkDataSet * filtered=NULL;
  gchar * temp_string;
  AmitkVoxel kernel_dim;

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), NULL);
  g_return_val_if_fail(ds->raw_data != NULL, NULL);

  filtered = AMITK_DATA_SET(amitk_object_copy(AMITK_OBJECT(ds)));

  /* start by unrefing the info that's only copied by reference by amitk_object_copy */
  if (filtered->raw_data != NULL) {
    g_object_unref(filtered->raw_data);
    filtered->raw_data = NULL;
  }
  
  if (filtered->internal_scaling != NULL) {
    g_object_unref(filtered->internal_scaling);
    filtered->internal_scaling = NULL;
  }

  if (filtered->distribution != NULL) {
    g_object_unref(filtered->distribution);
    filtered->distribution = NULL;
  }

  /* and unref anything that's obviously now incorrect */
  if (filtered->current_scaling != NULL) {
    g_object_unref(filtered->current_scaling);
    filtered->current_scaling = NULL;
  }

  if (filtered->frame_max != NULL) {
    g_free(filtered->frame_max);
    filtered->frame_max = NULL;
  }

  if (filtered->frame_min != NULL) {
    g_free(filtered->frame_min);
    filtered->frame_min = NULL;
  }

  /* set a new name for this guy */
  temp_string = g_strdup_printf(_("%s, %s filtered"), AMITK_OBJECT_NAME(ds),
				amitk_filter_get_name(filter_type));
  amitk_object_set_name(AMITK_OBJECT(filtered), temp_string);
  g_free(temp_string);

  /* start the new building process */
  filtered->raw_data = amitk_raw_data_new_with_data(AMITK_FORMAT_FLOAT, AMITK_DATA_SET_DIM(ds));
  if (filtered->raw_data == NULL) {
    g_warning(_("couldn't allocate space for the filtered raw data set structure"));
    goto error;
  }
  
  /* setup the scaling factor */
  filtered->scaling_type = AMITK_SCALING_TYPE_0D;
  filtered->internal_scaling = amitk_raw_data_DOUBLE_0D_SCALING_init(1.0);
  amitk_data_set_set_scale_factor(filtered, 1.0); /* reset the current scaling array */


  /* hand everything off to the data type specific function */
  switch(filter_type) {

#ifdef AMIDE_LIBGSL_SUPPORT
  case AMITK_FILTER_GAUSSIAN:
    {
      AmitkRawData * kernel;
      AmitkVoxel kernel_size_3D;

      kernel_size_3D.t=1;
      kernel_size_3D.z=kernel_size_3D.y=kernel_size_3D.x=kernel_size;

      kernel = amitk_filter_calculate_gaussian_kernel_complex(kernel_size_3D, 
							      AMITK_DATA_SET_VOXEL_SIZE(ds),
							      fwhm);
      if (kernel == NULL) {
	g_warning(_("failed to calculate 3D gaussian kernel"));
	goto error;
      }
      filter_fir(ds, filtered, kernel, kernel_size_3D);
      g_object_unref(kernel);
    }
    break;
#endif

  case AMITK_FILTER_MEDIAN_LINEAR:
    filter_median_linear(ds, filtered, kernel_size);
    break;


  case AMITK_FILTER_MEDIAN_3D:
    kernel_dim.t = 1;
    kernel_dim.z = kernel_dim.y = kernel_dim.x = kernel_size;
    filter_median_3D(ds, filtered, kernel_dim);
    break;


  default: 
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    goto error;
  }

  /* recalc the temporary parameters */
  amitk_data_set_calc_max_min(filtered, update_func, update_data);

  return filtered;
    
 error:
  if (filtered != NULL) amitk_object_unref(filtered);
  return NULL;
}


gint amitk_data_sets_count(GList * objects, gboolean recurse) {

  gint count;

  if (objects == NULL) 
    return 0;

  if (AMITK_IS_DATA_SET(objects->data))
    count = 1;
  else
    count = 0;

  if (recurse) /* count data sets that are children */
    count += amitk_data_sets_count(AMITK_OBJECT_CHILDREN(objects->data), recurse);

  /* add this count too the counts from the rest of the objects */
  return count+amitk_data_sets_count(objects->next, recurse);
}



/* function to return the minimum frame duration from a list of objects */
amide_time_t amitk_data_sets_get_min_frame_duration(GList * objects) {

  amide_time_t min_duration, temp;

  if (objects == NULL) return -1.0; /* invalid */

  /* first process the rest of the list */
  min_duration = amitk_data_sets_get_min_frame_duration(objects->next);

  /* now process and compare to the children */
  temp = amitk_data_sets_get_min_frame_duration(AMITK_OBJECT_CHILDREN(objects->data));
  if (temp >= 0.0) {
    if (min_duration < 0.0) min_duration = temp;
    else if (temp < min_duration) min_duration = temp;
  }
  
  /* and process this guy */
  if (AMITK_IS_DATA_SET(objects->data)) {
    temp = amitk_data_set_get_min_frame_duration(objects->data);
    if (min_duration < 0.0) min_duration = temp;
    else if (temp < min_duration) min_duration = temp;
  }

  return min_duration;
}



/* returns the minimum voxel dimensions of the list of data sets */
amide_real_t amitk_data_sets_get_min_voxel_size(GList * objects) {

  amide_real_t min_voxel_size, temp;

  if (objects == NULL) return -1.0; /* invalid */

  /* first process the rest of the list */
  min_voxel_size = amitk_data_sets_get_min_voxel_size(objects->next);

  /* now process and compare to the children */
  temp = amitk_data_sets_get_min_voxel_size(AMITK_OBJECT_CHILDREN(objects->data));
  if (temp >= 0.0) {
    if (min_voxel_size < 0.0) min_voxel_size = temp;
    else if (temp < min_voxel_size) min_voxel_size = temp;
  }

  /* and process this guy */
  if (AMITK_IS_DATA_SET(objects->data)) {
    temp = point_min_dim(AMITK_DATA_SET_VOXEL_SIZE(objects->data));
    if (min_voxel_size < 0.0) min_voxel_size = temp;
    else if (temp < min_voxel_size) min_voxel_size = temp;
  }

  return min_voxel_size;
}




/* returns the minimum dimensional width of the data set with the largest voxel size */
/* figure out what our voxel size is going to be for our returned
   slices.  I'm going to base this on the volume with the largest
   minimum voxel dimension, this way the user doesn't suddenly get a
   huge image when adding in a study with small voxels (i.e. a CT
   scan).  The user can always increase the zoom later*/
amide_real_t amitk_data_sets_get_max_min_voxel_size(GList * objects) {

  amide_real_t min_voxel_size, temp;

  if (objects == NULL) return -1.0; /* invalid */

  /* first process the rest of the list */
  min_voxel_size = amitk_data_sets_get_max_min_voxel_size(objects->next);

  /* now process and compare to the children */
  temp = amitk_data_sets_get_max_min_voxel_size(AMITK_OBJECT_CHILDREN(objects->data));
  if (temp >= 0.0) {
    if (min_voxel_size < 0.0) min_voxel_size = temp;
    else if (temp > min_voxel_size) min_voxel_size = temp;
  }

  /* and process this guy */
  if (AMITK_IS_DATA_SET(objects->data)) {
    temp = point_min_dim(AMITK_DATA_SET_VOXEL_SIZE(objects->data));
    if (min_voxel_size < 0.0) min_voxel_size = temp;
    else if (temp > min_voxel_size) min_voxel_size = temp;
  }
  return min_voxel_size;
}

/* returns an unreferenced pointer to a slice in the list with the given parent */
AmitkDataSet * amitk_data_sets_find_with_slice_parent(GList * slices, const AmitkDataSet * slice_parent) {

  AmitkDataSet * slice=NULL;

  if (slice_parent == NULL) return NULL;
  if (slices == NULL) return NULL;

  if (AMITK_IS_DATA_SET(slices->data))
    if (AMITK_DATA_SET(slices->data)->slice_parent == slice_parent)
      slice = AMITK_DATA_SET(slices->data);

  /* check children */
  if (slice == NULL)
    slice = amitk_data_sets_find_with_slice_parent(AMITK_OBJECT_CHILDREN(slices->data), slice_parent);

  /* process the rest of the list */
  if (slice == NULL)
    slice = amitk_data_sets_find_with_slice_parent(slices->next, slice_parent);

  return slice;
}

/* removes from slices the slice with the given slice_parent */
GList * amitk_data_sets_remove_with_slice_parent(GList * slices,const AmitkDataSet * slice_parent) {

  AmitkDataSet * slice;

  if (slice_parent == NULL) return NULL;
  if (slices == NULL) return NULL;

  slice = amitk_data_sets_find_with_slice_parent(slices, slice_parent);

  while (slice != NULL) {
    slices = g_list_remove(slices, slice);
    amitk_object_unref(slice);

    /* may be multiple slices with this parent */
    slice = amitk_data_sets_find_with_slice_parent(slices, slice_parent);
  }

  return slices;
}


/* several things cause slice caches to get invalidated, so they don't need to be
   explicitly checked here

   1. Scale factor changes
   2. The parent data set's space changing
   3. The parent data set's voxel size changing
   4. Any change to the raw data

*/

static AmitkDataSet * slice_cache_find (GList * slice_cache, AmitkDataSet * parent_ds, 
					const amide_time_t start, const amide_time_t duration,
					const amide_real_t pixel_dim, const AmitkVolume * view_volume) {

  AmitkDataSet * slice;
  AmitkVoxel dim;

  if (slice_cache == NULL) {
    return NULL;
  } else {
    slice = slice_cache->data;
    if (AMITK_DATA_SET_SLICE_PARENT(slice) == parent_ds) 
      if (amitk_space_equal(AMITK_SPACE(slice), AMITK_SPACE(view_volume))) 
	if (REAL_EQUAL(slice->scan_start, start)) 
	  if (REAL_EQUAL(slice->frame_duration[0], duration)) 
	    if (REAL_EQUAL(slice->voxel_size.z,AMITK_VOLUME_Z_CORNER(view_volume))) {
	      dim.x = ceil(fabs(AMITK_VOLUME_X_CORNER(view_volume))/pixel_dim);
	      dim.y = ceil(fabs(AMITK_VOLUME_Y_CORNER(view_volume))/pixel_dim);
	      dim.z = dim.t = 1;
	      if (VOXEL_EQUAL(dim, AMITK_DATA_SET_DIM(slice))) 
		if (AMITK_DATA_SET_INTERPOLATION(slice) == AMITK_DATA_SET_INTERPOLATION(parent_ds)) 
		  return slice;
	    }
  }
  

  /* this one's not it, keep looking */
  return slice_cache_find(slice_cache->next, parent_ds, start, duration, pixel_dim, view_volume);
}



/* give a list of data_sets, returns a list of slices of equal size and orientation
   intersecting these data_sets.  The slice_cache is a list of already generated slices,
   if an appropriate slice is found in there, it'll be used */
/* notes
   in real practice, the parent data set's local cache is rarely used,
   as most slices, if there in the local cache, will also be in the passed in cache
 */
GList * amitk_data_sets_get_slices(GList * objects,
				   GList ** pslice_cache,
				   const gint max_slice_cache_size,
				   const amide_time_t start,
				   const amide_time_t duration,
				   const amide_real_t pixel_dim,
				   const AmitkVolume * view_volume) {


  GList * slices=NULL;
  GList * last;
  GList * remove;
  AmitkDataSet * local_slice;
  AmitkDataSet * canvas_slice;
  AmitkDataSet * slice;
  AmitkDataSet * parent_ds;
  gint num_data_sets=0;
  gint cache_size;

#ifdef AMIDE_COMMENT_OUT
  struct timeval tv1;
  struct timeval tv2;
  gdouble time1;
  gdouble time2;

  /* let's do some timing */
  gettimeofday(&tv1, NULL);
#endif

  g_return_val_if_fail(objects != NULL, NULL);

  /* and get the slices */
  while (objects != NULL) {
    if (AMITK_IS_DATA_SET(objects->data)) {
      num_data_sets++;
      parent_ds = AMITK_DATA_SET(objects->data);

      /* try to find it in the caches first */
      canvas_slice = slice_cache_find(*pslice_cache, parent_ds, start,  
				      duration, pixel_dim, view_volume);

      local_slice = slice_cache_find(parent_ds->slice_cache, parent_ds, start, 
				     duration, pixel_dim, view_volume);
      
      if (canvas_slice != NULL) {
	slice = amitk_object_ref(canvas_slice);
      } else if (local_slice != NULL) {
	slice = amitk_object_ref(local_slice);
      } else {/* generate a new one */
	slice = amitk_data_set_get_slice(parent_ds, start, duration,  pixel_dim, view_volume, TRUE);
      }

      g_return_val_if_fail(slice != NULL, slices);

      slices = g_list_prepend(slices, slice);

      if (canvas_slice == NULL)
	*pslice_cache = g_list_prepend(*pslice_cache, amitk_object_ref(slice)); /* most recently used first */
      if (local_slice == NULL) {
	parent_ds->slice_cache = g_list_prepend(parent_ds->slice_cache, amitk_object_ref(slice));

	cache_size = g_list_length(parent_ds->slice_cache);
	if (cache_size > parent_ds->max_slice_cache_size) {
	  last = g_list_nth(parent_ds->slice_cache, parent_ds->max_slice_cache_size-1);
	  remove = last->next;
	  last->next = NULL;
	  remove->prev = NULL;
	  amitk_objects_unref(remove);
	}
      }
    }
    objects = objects->next;
  }

  /* regulate the size of the caches */
  cache_size = g_list_length(*pslice_cache);
  if (cache_size > max_slice_cache_size) {
    last = g_list_nth(*pslice_cache, max_slice_cache_size-1);
    remove = last->next;
    last->next = NULL;
    remove->prev = NULL;
    amitk_objects_unref(remove);
  }

#ifdef AMIDE_COMMENT_OUT
  /* and wrapup our timing */
  gettimeofday(&tv2, NULL);
  time1 = ((double) tv1.tv_sec) + ((double) tv1.tv_usec)/1000000.0;
  time2 = ((double) tv2.tv_sec) + ((double) tv2.tv_usec)/1000000.0;
  g_print("######## Slice Generating Took %5.3f (s) #########\n",time2-time1);
#endif

  return slices;
}




const gchar * amitk_scaling_type_get_name(const AmitkScalingType scaling_type) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_SCALING_TYPE);
  enum_value = g_enum_get_value(enum_class, scaling_type);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}


const gchar * amitk_modality_get_name(const AmitkModality modality) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_MODALITY);
  enum_value = g_enum_get_value(enum_class, modality);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

const gchar * amitk_interpolation_get_name(const AmitkInterpolation interpolation) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_INTERPOLATION);
  enum_value = g_enum_get_value(enum_class, interpolation);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

const gchar * amitk_thresholding_get_name(const AmitkThresholding thresholding) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_THRESHOLDING);
  enum_value = g_enum_get_value(enum_class, thresholding);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

const gchar * amitk_conversion_get_name(const AmitkConversion conversion) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_CONVERSION);
  enum_value = g_enum_get_value(enum_class, conversion);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

const gchar * amitk_weight_unit_get_name(const AmitkWeightUnit weight_unit) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_WEIGHT_UNIT);
  enum_value = g_enum_get_value(enum_class, weight_unit);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}


const gchar * amitk_dose_unit_get_name(const AmitkDoseUnit dose_unit) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_DOSE_UNIT);
  enum_value = g_enum_get_value(enum_class, dose_unit);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}


const gchar * amitk_cylinder_unit_get_name(const AmitkCylinderUnit cylinder_unit) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_CYLINDER_UNIT);
  enum_value = g_enum_get_value(enum_class, cylinder_unit);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}




amide_data_t amitk_weight_unit_convert_to         (const amide_data_t kg, 
						   const AmitkWeightUnit weight_unit) {

  switch(weight_unit) {
  case AMITK_WEIGHT_UNIT_KILOGRAM:
    return kg;
    break;
  case AMITK_WEIGHT_UNIT_GRAM:
    return kg*1000.0;
    break;
  case AMITK_WEIGHT_UNIT_POUND:
    return kg*2.2046226;
    break;
  case AMITK_WEIGHT_UNIT_OUNCE:
    return kg*35.273962;
    break;
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    break;
  }

  return 0.0;
}


amide_data_t amitk_weight_unit_convert_from       (const amide_data_t weight, 
						   const AmitkWeightUnit weight_unit) {
  switch(weight_unit) {
  case AMITK_WEIGHT_UNIT_KILOGRAM:
    return weight;
    break;
  case AMITK_WEIGHT_UNIT_GRAM:
    return weight/1000.0;
    break;
  case AMITK_WEIGHT_UNIT_POUND:
    return weight/2.2046226;
    break;
  case AMITK_WEIGHT_UNIT_OUNCE:
    return weight/35.273962;
    break;
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    break;
  }

  return 0.0;
}

amide_data_t amitk_dose_unit_convert_to           (const amide_data_t MBq, 
						   const AmitkDoseUnit dose_unit) {

  switch(dose_unit) {
  case AMITK_DOSE_UNIT_MEGABECQUEREL:
    return MBq;
    break;
  case AMITK_DOSE_UNIT_MILLICURIE:
    return MBq/37.0;
    break;
  case AMITK_DOSE_UNIT_MICROCURIE:
    return MBq/0.037;
    break;
  case AMITK_DOSE_UNIT_NANOCURIE:
    return MBq/0.000037;
    break;
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    break;
  }

  return 0.0;
}

amide_data_t amitk_dose_unit_convert_from         (const amide_data_t dose, 
						   const AmitkDoseUnit dose_unit) {

  switch(dose_unit) {
  case AMITK_DOSE_UNIT_MEGABECQUEREL:
    return dose;
    break;
  case AMITK_DOSE_UNIT_MILLICURIE:
    return dose*37.0;
    break;
  case AMITK_DOSE_UNIT_MICROCURIE:
    return dose*0.037;
    break;
  case AMITK_DOSE_UNIT_NANOCURIE:
    return dose*0.000037;
    break;
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    break;
  }

  return 0.0;
}

amide_data_t amitk_cylinder_unit_convert_to       (const amide_data_t MBq_cc_image_units, 
						   const AmitkCylinderUnit cylinder_unit) {

  switch(cylinder_unit) {
  case AMITK_CYLINDER_UNIT_MEGABECQUEREL_PER_CC_IMAGE_UNIT:
    return MBq_cc_image_units;
    break;
  case AMITK_CYLINDER_UNIT_IMAGE_UNIT_CC_PER_MEGABECQUEREL:
    return 1.0/MBq_cc_image_units;
  case AMITK_CYLINDER_UNIT_MILLICURIE_PER_CC_IMAGE_UNIT:
    return amitk_dose_unit_convert_to(MBq_cc_image_units, AMITK_DOSE_UNIT_MILLICURIE);
    break;
  case AMITK_CYLINDER_UNIT_IMAGE_UNIT_CC_PER_MILLICURIE:
    return 1.0/amitk_dose_unit_convert_to(MBq_cc_image_units, AMITK_DOSE_UNIT_MILLICURIE);
    break;
  case AMITK_CYLINDER_UNIT_MICROCURIE_PER_CC_IMAGE_UNIT:
    return amitk_dose_unit_convert_to(MBq_cc_image_units, AMITK_DOSE_UNIT_MICROCURIE);
    break;
  case AMITK_CYLINDER_UNIT_IMAGE_UNIT_CC_PER_MICROCURIE:
    return 1.0/amitk_dose_unit_convert_to(MBq_cc_image_units, AMITK_DOSE_UNIT_MICROCURIE);
    break;
  case AMITK_CYLINDER_UNIT_NANOCURIE_PER_CC_IMAGE_UNIT:
    return amitk_dose_unit_convert_to(MBq_cc_image_units, AMITK_DOSE_UNIT_NANOCURIE);
    break;
  case AMITK_CYLINDER_UNIT_IMAGE_UNIT_CC_PER_NANOCURIE:
    return 1.0/amitk_dose_unit_convert_to(MBq_cc_image_units, AMITK_DOSE_UNIT_NANOCURIE);
    break;
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    break;
  }

  return 0.0;
}

amide_data_t amitk_cylinder_unit_convert_from     (const amide_data_t cylinder_factor,
						   const AmitkCylinderUnit cylinder_unit) {

  switch(cylinder_unit) {
  case AMITK_CYLINDER_UNIT_MEGABECQUEREL_PER_CC_IMAGE_UNIT:
    return cylinder_factor;
    break;
  case AMITK_CYLINDER_UNIT_IMAGE_UNIT_CC_PER_MEGABECQUEREL:
    return 1.0/cylinder_factor;
    break;
  case AMITK_CYLINDER_UNIT_MILLICURIE_PER_CC_IMAGE_UNIT:
    return amitk_dose_unit_convert_from(cylinder_factor, AMITK_DOSE_UNIT_MILLICURIE);
    break;
  case AMITK_CYLINDER_UNIT_IMAGE_UNIT_CC_PER_MILLICURIE:
    return amitk_dose_unit_convert_from(1.0/cylinder_factor, AMITK_DOSE_UNIT_MILLICURIE);
    break;
  case AMITK_CYLINDER_UNIT_MICROCURIE_PER_CC_IMAGE_UNIT:
    return amitk_dose_unit_convert_from(cylinder_factor, AMITK_DOSE_UNIT_MICROCURIE);
    break;
  case AMITK_CYLINDER_UNIT_IMAGE_UNIT_CC_PER_MICROCURIE:
    return amitk_dose_unit_convert_from(1.0/cylinder_factor, AMITK_DOSE_UNIT_MICROCURIE);
    break;
  case AMITK_CYLINDER_UNIT_NANOCURIE_PER_CC_IMAGE_UNIT:
    return amitk_dose_unit_convert_from(cylinder_factor, AMITK_DOSE_UNIT_NANOCURIE);
    break;
  case AMITK_CYLINDER_UNIT_IMAGE_UNIT_CC_PER_NANOCURIE:
    return amitk_dose_unit_convert_from(1.0/cylinder_factor, AMITK_DOSE_UNIT_NANOCURIE);
    break;
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    break;
  }

  return 0.0;
}
