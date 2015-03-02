/* amitk_data_set.h
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

#ifndef __AMITK_DATA_SET_H__
#define __AMITK_DATA_SET_H__


#include "amitk_volume.h"
#include "amitk_raw_data.h"
#include "amitk_color_table.h"
#include "amitk_filter.h"

G_BEGIN_DECLS

#define	AMITK_TYPE_DATA_SET		        (amitk_data_set_get_type ())
#define AMITK_DATA_SET(object)		        (G_TYPE_CHECK_INSTANCE_CAST ((object), AMITK_TYPE_DATA_SET, AmitkDataSet))
#define AMITK_DATA_SET_CLASS(klass)	        (G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_DATA_SET, AmitkDataSetClass))
#define AMITK_IS_DATA_SET(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), AMITK_TYPE_DATA_SET))
#define AMITK_IS_DATA_SET_CLASS(klass)	        (G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_DATA_SET))
#define	AMITK_DATA_SET_GET_CLASS(object)	(G_TYPE_CHECK_GET_CLASS ((object), AMITK_TYPE_DATA_SET, AmitkDataSetClass))

#define AMITK_DATA_SET_MODALITY(ds)                (AMITK_DATA_SET(ds)->modality)
#define AMITK_DATA_SET_VOXEL_SIZE(ds)              (AMITK_DATA_SET(ds)->voxel_size)
#define AMITK_DATA_SET_RAW_DATA(ds)                (AMITK_DATA_SET(ds)->raw_data)
#define AMITK_DATA_SET_DIM(ds)                     (AMITK_RAW_DATA_DIM(AMITK_DATA_SET_RAW_DATA(ds)))
#define AMITK_DATA_SET_DIM_X(ds)                   (AMITK_RAW_DATA_DIM_X(AMITK_DATA_SET_RAW_DATA(ds)))
#define AMITK_DATA_SET_DIM_Y(ds)                   (AMITK_RAW_DATA_DIM_Y(AMITK_DATA_SET_RAW_DATA(ds)))
#define AMITK_DATA_SET_DIM_Z(ds)                   (AMITK_RAW_DATA_DIM_Z(AMITK_DATA_SET_RAW_DATA(ds)))
#define AMITK_DATA_SET_DIM_T(ds)                   (AMITK_RAW_DATA_DIM_T(AMITK_DATA_SET_RAW_DATA(ds)))
#define AMITK_DATA_SET_FORMAT(ds)                  (AMITK_RAW_DATA_FORMAT(AMITK_DATA_SET_RAW_DATA(ds)))
#define AMITK_DATA_SET_NUM_FRAMES(ds)              (AMITK_DATA_SET_DIM_T(ds))
#define AMITK_DATA_SET_DISTRIBUTION(ds)            (AMITK_DATA_SET(ds)->distribution)
#define AMITK_DATA_SET_COLOR_TABLE(ds)             (AMITK_DATA_SET(ds)->color_table)
#define AMITK_DATA_SET_INTERPOLATION(ds)           (AMITK_DATA_SET(ds)->interpolation)
#define AMITK_DATA_SET_DYNAMIC(ds)                 (AMITK_DATA_SET(ds)->raw_data->dim.t > 1)
#define AMITK_DATA_SET_THRESHOLDING(ds)            (AMITK_DATA_SET(ds)->thresholding)
#define AMITK_DATA_SET_SLICE_PARENT(ds)            (AMITK_DATA_SET(ds)->slice_parent)
#define AMITK_DATA_SET_SCAN_DATE(ds)               (AMITK_DATA_SET(ds)->scan_date)
#define AMITK_DATA_SET_SCAN_START(ds)              (AMITK_DATA_SET(ds)->scan_start)
#define AMITK_DATA_SET_GLOBAL_MAX(ds)              (AMITK_DATA_SET(ds)->global_max)
#define AMITK_DATA_SET_GLOBAL_MIN(ds)              (AMITK_DATA_SET(ds)->global_min)
#define AMITK_DATA_SET_FRAME_MAX(ds, i)            (AMITK_DATA_SET(ds)->frame_max[(i)])
#define AMITK_DATA_SET_FRAME_MIN(ds, i)            (AMITK_DATA_SET(ds)->frame_min[(i)])
#define AMITK_DATA_SET_THRESHOLD_REF_FRAME(ds,ref_frame) (AMITK_DATA_SET(ds)->threshold_ref_frame[ref_frame])
#define AMITK_DATA_SET_THRESHOLD_MAX(ds, ref_frame)      (AMITK_DATA_SET(ds)->threshold_max[ref_frame])
#define AMITK_DATA_SET_THRESHOLD_MIN(ds, ref_frame)      (AMITK_DATA_SET(ds)->threshold_min[ref_frame])
#define AMITK_DATA_SET_SCALING_TYPE(ds)            (AMITK_DATA_SET(ds)->scaling_type)

#define AMITK_DATA_SET_CONVERSION(ds)              (AMITK_DATA_SET(ds)->conversion)
#define AMITK_DATA_SET_SCALE_FACTOR(ds)            (AMITK_DATA_SET(ds)->scale_factor)
#define AMITK_DATA_SET_INJECTED_DOSE(ds)           (AMITK_DATA_SET(ds)->injected_dose)
#define AMITK_DATA_SET_DISPLAYED_DOSE_UNIT(ds)     (AMITK_DATA_SET(ds)->displayed_dose_unit)
#define AMITK_DATA_SET_SUBJECT_WEIGHT(ds)          (AMITK_DATA_SET(ds)->subject_weight)
#define AMITK_DATA_SET_DISPLAYED_WEIGHT_UNIT(ds)   (AMITK_DATA_SET(ds)->displayed_weight_unit)
#define AMITK_DATA_SET_CYLINDER_FACTOR(ds)         (AMITK_DATA_SET(ds)->cylinder_factor)
#define AMITK_DATA_SET_DISPLAYED_CYLINDER_UNIT(ds) (AMITK_DATA_SET(ds)->displayed_cylinder_unit)

#define AMITK_DATA_SET_DISTRIBUTION_SIZE 256

typedef enum {
  AMITK_INTERPOLATION_NEAREST_NEIGHBOR, 
  AMITK_INTERPOLATION_TRILINEAR, 
  AMITK_INTERPOLATION_NUM
} AmitkInterpolation;

typedef enum {
  AMITK_MODALITY_PET, 
  AMITK_MODALITY_SPECT, 
  AMITK_MODALITY_CT, 
  AMITK_MODALITY_MRI, 
  AMITK_MODALITY_OTHER, 
  AMITK_MODALITY_NUM
} AmitkModality;

typedef enum {
  AMITK_THRESHOLDING_PER_SLICE, 
  AMITK_THRESHOLDING_PER_FRAME, 
  AMITK_THRESHOLDING_INTERPOLATE_FRAMES,
  AMITK_THRESHOLDING_GLOBAL, 
  AMITK_THRESHOLDING_NUM
} AmitkThresholding;

typedef enum {
  AMITK_SCALING_TYPE_0D,
  AMITK_SCALING_TYPE_1D,
  AMITK_SCALING_TYPE_2D,
  AMITK_SCALING_TYPE_NUM
} AmitkScalingType;

typedef enum {
  AMITK_CONVERSION_STRAIGHT,
  AMITK_CONVERSION_PERCENT_ID_PER_G,
  AMITK_CONVERSION_SUV,
  AMITK_CONVERSION_NUM
} AmitkConversion;

typedef enum {
  AMITK_WEIGHT_UNIT_KILOGRAM,
  AMITK_WEIGHT_UNIT_GRAM,
  AMITK_WEIGHT_UNIT_POUND,
  AMITK_WEIGHT_UNIT_OUNCE,
  AMITK_WEIGHT_UNIT_NUM
} AmitkWeightUnit;

typedef enum {
  AMITK_DOSE_UNIT_MEGABECQUEREL,
  AMITK_DOSE_UNIT_MILLICURIE,
  AMITK_DOSE_UNIT_MICROCURIE,
  AMITK_DOSE_UNIT_NANOCURIE,
  AMITK_DOSE_UNIT_NUM
} AmitkDoseUnit;

typedef enum {
  AMITK_CYLINDER_UNIT_MEGABECQUEREL_PER_CC_IMAGE_UNIT,
  AMITK_CYLINDER_UNIT_IMAGE_UNIT_CC_PER_MEGABECQUEREL,
  AMITK_CYLINDER_UNIT_MILLICURIE_PER_CC_IMAGE_UNIT,
  AMITK_CYLINDER_UNIT_IMAGE_UNIT_CC_PER_MILLICURIE,
  AMITK_CYLINDER_UNIT_MICROCURIE_PER_CC_IMAGE_UNIT,
  AMITK_CYLINDER_UNIT_IMAGE_UNIT_CC_PER_MICROCURIE,
  AMITK_CYLINDER_UNIT_NANOCURIE_PER_CC_IMAGE_UNIT,
  AMITK_CYLINDER_UNIT_IMAGE_UNIT_CC_PER_NANOCURIE,
  AMITK_CYLINDER_UNIT_NUM
} AmitkCylinderUnit;
  

/* the skip is for glib-mkenums, it doesn't know how to handle ifdef's */
typedef enum { /*< skip >*/
  AMITK_IMPORT_METHOD_GUESS, 
  AMITK_IMPORT_METHOD_RAW, 
#ifdef AMIDE_LIBECAT_SUPPORT
  AMITK_IMPORT_METHOD_LIBECAT,
#endif
#ifdef AMIDE_LIBMDC_SUPPORT
  AMITK_IMPORT_METHOD_LIBMDC,
#endif
  AMITK_IMPORT_METHOD_NUM
} AmitkImportMethod;

typedef struct _AmitkDataSetClass AmitkDataSetClass;
typedef struct _AmitkDataSet AmitkDataSet;


struct _AmitkDataSet
{
  AmitkVolume parent;

  /* parameters that are saved */
  gchar * scan_date; /* the time/day the image was acquired */
  AmitkModality modality;
  AmitkPoint voxel_size;  /* in mm */
  AmitkRawData * raw_data;
  AmitkScalingType scaling_type; /*  dimensions of internal scaling */
  AmitkRawData * internal_scaling; /* internally (data set) supplied scaling factor */
  amide_time_t scan_start;
  amide_time_t * frame_duration; /* array of the duration of each frame */
  AmitkColorTable color_table; /* the color table to draw this volume in */
  AmitkInterpolation interpolation;

  amide_data_t scale_factor; /* user specified factor to multiply data set by */
  AmitkConversion conversion;
  amide_data_t injected_dose;  /* in MBq */
  AmitkDoseUnit displayed_dose_unit;
  amide_data_t subject_weight; /* in KG */
  AmitkWeightUnit displayed_weight_unit;
  amide_data_t cylinder_factor; /* (MBq/cc)/Image Unit */
  AmitkCylinderUnit displayed_cylinder_unit;


  AmitkThresholding thresholding; /* what sort of thresholding we're using (per slice, global, etc.) */
  amide_data_t threshold_max[2]; /* the thresholds to use for this volume */
  amide_data_t threshold_min[2]; 
  guint threshold_ref_frame[2];
  
  /* parameters calculated as needed or on loading the object */
  /* theoritically, could be recalculated on the fly, but used enough we'll store... */
  AmitkRawData * distribution; /* 1D array of data distribution, used in thresholding */
  amide_data_t global_max;
  amide_data_t global_min;
  amide_data_t * frame_max; 
  amide_data_t * frame_min;
  AmitkRawData * current_scaling; /* external_scaling * internal_scaling[] */

  GList * slice_cache;
  gint max_slice_cache_size;

  /* only used by derived data sets (slices and projections)  */
  /* this is a weak pointer, it should be NULL'ed automatically by gtk on the parent's destruction */
  AmitkDataSet * slice_parent; 

};

struct _AmitkDataSetClass
{
  AmitkVolumeClass parent_class;

  void (* thresholding_changed)    (AmitkDataSet * ds);
  void (* color_table_changed)     (AmitkDataSet * ds);
  void (* interpolation_changed)   (AmitkDataSet * ds);
  void (* conversion_changed)      (AmitkDataSet * ds);
  void (* scale_factor_changed)    (AmitkDataSet * ds);
  void (* modality_changed)        (AmitkDataSet * ds);
  void (* time_changed)            (AmitkDataSet * ds);
  void (* voxel_size_changed)      (AmitkDataSet * ds);
  void (* data_set_changed)        (AmitkDataSet * ds);
  void (* invalidate_slice_cache)  (AmitkDataSet * ds);

};



/* Application-level methods */

GType	        amitk_data_set_get_type	         (void);
AmitkDataSet *  amitk_data_set_new               (void);
AmitkDataSet *  amitk_data_set_new_with_data     (const AmitkFormat format, 
						  const AmitkVoxel dim,
						  const AmitkScalingType scaling_type);
AmitkDataSet * amitk_data_set_import_raw_file    (const gchar * file_name, 
						  AmitkRawFormat raw_format,
						  AmitkVoxel data_dim,
						  guint file_offset,
						  gboolean (*update_func)(), 
						  gpointer update_data);
AmitkDataSet * amitk_data_set_import_file        (AmitkImportMethod import_method, 
						  int submethod,
						  const gchar * import_filename,
						  gboolean (*update_func)(),
						  gpointer update_data);
void           amitk_data_set_set_modality       (AmitkDataSet * ds,
						  const AmitkModality modality);
void           amitk_data_set_set_scan_start     (AmitkDataSet * ds,
						  const amide_time_t start);
void           amitk_data_set_set_frame_duration (AmitkDataSet * ds, 
						  const guint frame,
						  amide_time_t duration);
void           amitk_data_set_set_voxel_size     (AmitkDataSet * ds,
						  const AmitkPoint voxel_size);
void           amitk_data_set_set_thresholding   (AmitkDataSet * ds, 
						  const AmitkThresholding thresholding);
void           amitk_data_set_set_threshold_max  (AmitkDataSet * ds,
						  guint which_reference,
						  amide_data_t value);
void           amitk_data_set_set_threshold_min  (AmitkDataSet * ds,
						  guint which_reference,
						  amide_data_t value);
void           amitk_data_set_set_threshold_ref_frame  (AmitkDataSet * ds,
							guint which_reference,
							guint frame);
void           amitk_data_set_set_color_table    (AmitkDataSet * ds, 
						  const AmitkColorTable new_color_table);
void           amitk_data_set_set_interpolation  (AmitkDataSet * ds,
						  const AmitkInterpolation new_interpolation);
void           amitk_data_set_set_scan_date      (AmitkDataSet * ds, 
						  const gchar * new_date);
void           amitk_data_set_set_conversion     (AmitkDataSet * ds,
						  AmitkConversion new_conversion);
void           amitk_data_set_set_scale_factor   (AmitkDataSet * ds, 
						  amide_data_t new_scale_factor);
void           amitk_data_set_set_injected_dose  (AmitkDataSet * ds,
						  amide_data_t new_injected_dose);
void           amitk_data_set_set_subject_weight (AmitkDataSet * ds,
						  amide_data_t new_subject_weight);
void           amitk_data_set_set_cylinder_factor(AmitkDataSet * ds,
						  amide_data_t new_cylinder_factor);
void           amitk_data_set_set_displayed_dose_unit    (AmitkDataSet * ds,
						          AmitkDoseUnit new_dose_unit);
void           amitk_data_set_set_displayed_weight_unit  (AmitkDataSet * ds,
						          AmitkWeightUnit new_weight_unit);
void           amitk_data_set_set_displayed_cylinder_unit(AmitkDataSet * ds,
						          AmitkCylinderUnit new_cylinder_unit);
amide_time_t   amitk_data_set_get_start_time     (const AmitkDataSet * ds, 
						  const guint frame);
amide_time_t   amitk_data_set_get_end_time       (const AmitkDataSet * ds, 
						  const guint frame);
amide_time_t   amitk_data_set_get_midpt_time     (const AmitkDataSet *ds,
						  const guint frame);
guint          amitk_data_set_get_frame          (const AmitkDataSet * ds, 
						  const amide_time_t time);
amide_time_t   amitk_data_set_get_frame_duration (const AmitkDataSet * ds,
						  guint frame);
amide_time_t   amitk_data_set_get_min_frame_duration (const AmitkDataSet * ds);
void           amitk_data_set_calc_far_corner    (AmitkDataSet * ds);
void           amitk_data_set_calc_max_min       (AmitkDataSet * ds,
						  gboolean (*update_func)(),
						  gpointer update_data);
amide_data_t   amitk_data_set_get_max            (const AmitkDataSet * ds, 
						  const amide_time_t start, 
						  const amide_time_t duration);
amide_data_t   amitk_data_set_get_min            (const AmitkDataSet * ds, 
						  const amide_time_t start, 
						  const amide_time_t duration);
void           amitk_data_set_get_thresholding_max_min(const AmitkDataSet * ds, 
						       const AmitkDataSet * slice,
						       const amide_time_t start,
						       const amide_time_t duration,
						       amide_data_t * max, amide_data_t * min);
void           amitk_data_set_calc_distribution  (AmitkDataSet * ds, 
						  gboolean (*update_func)(), 
						  gpointer update_data);
amide_data_t   amitk_data_set_get_value          (const AmitkDataSet * ds, 
						  const AmitkVoxel i);
void           amitk_data_set_set_value          (AmitkDataSet *ds,
						  const AmitkVoxel i,
						  const amide_data_t value,
						  const gboolean signal_change);
void           amitk_data_set_get_projections    (AmitkDataSet * ds,
						  const guint frame,
						  AmitkDataSet ** projections,
						  gboolean (*update_func)(),
						  gpointer update_data);
AmitkDataSet * amitk_data_set_get_cropped        (const AmitkDataSet * ds,
						  const AmitkVoxel start,
						  const AmitkVoxel end,
						  const AmitkFormat format,
						  const AmitkScalingType scaling_type,
						  gboolean (*update_func)(),
						  gpointer update_data);
AmitkDataSet * amitk_data_set_get_filtered       (const AmitkDataSet * ds,
						  const AmitkFilter filter_type,
						  const gint kernel_size,
						  const amide_real_t fwhm,
						  gboolean (*update_func)(),
						  gpointer update_data);
AmitkDataSet * amitk_data_set_get_slice          (AmitkDataSet * ds,
						  const amide_time_t start,
						  const amide_time_t duration,
						  const amide_real_t pixel_dim,
						  const AmitkVolume * slice_volume,
						  const gboolean need_calc_max_min);





gint           amitk_data_sets_count                 (GList * objects, gboolean recurse);
amide_time_t   amitk_data_sets_get_min_frame_duration(GList * objects);
amide_real_t   amitk_data_sets_get_min_voxel_size    (GList * objects);
amide_real_t   amitk_data_sets_get_max_min_voxel_size(GList * objects);
GList *        amitk_data_sets_get_slices            (GList * objects,
						      GList ** pslice_cache,
						      const gint max_slice_cache_size,
						      const amide_time_t start,
						      const amide_time_t duration,
						      const amide_real_t pixel_dim,
						      const AmitkVolume * view_volume);
AmitkDataSet * amitk_data_sets_find_with_slice_parent(GList * slices, 
						      const AmitkDataSet * slice_parent);
GList *        amitk_data_sets_remove_with_slice_parent(GList * slices,
							const AmitkDataSet * slice_parent);




/* -------- defines ----------- */
#define amitk_data_set_get_frame_duration_mem(ds) (g_try_new(amide_time_t,(ds)->raw_data->dim.t))
#define amitk_data_set_get_frame_max_min_mem(ds) (g_try_new(amide_data_t,(ds)->raw_data->dim.t))
#define amitk_data_set_dynamic(ds) ((ds)->data_set->dim.t > 1)




const gchar *   amitk_scaling_type_get_name       (const AmitkScalingType scaling_type);
const gchar *   amitk_modality_get_name           (const AmitkModality modality);
const gchar *   amitk_interpolation_get_name      (const AmitkInterpolation interpolation);
const gchar *   amitk_thresholding_get_name       (const AmitkThresholding thresholding);
const gchar *   amitk_conversion_get_name         (const AmitkConversion conversion);
const gchar *   amitk_weight_unit_get_name        (const AmitkWeightUnit weight_unit);
const gchar *   amitk_dose_unit_get_name          (const AmitkDoseUnit dose_unit);
const gchar *   amitk_cylinder_unit_get_name      (const AmitkCylinderUnit cylinder_unit);


amide_data_t amitk_weight_unit_convert_to         (const amide_data_t kg, 
						   const AmitkWeightUnit weight_unit);
amide_data_t amitk_weight_unit_convert_from       (const amide_data_t weight, 
						   const AmitkWeightUnit weight_unit);
amide_data_t amitk_dose_unit_convert_to           (const amide_data_t MBq, 
						   const AmitkDoseUnit dose_unit);
amide_data_t amitk_dose_unit_convert_from         (const amide_data_t dose, 
						   const AmitkDoseUnit dose_unit);
amide_data_t amitk_cylinder_unit_convert_to       (const amide_data_t MBq_cc_image_units, 
						   const AmitkCylinderUnit cylinder_unit);
amide_data_t amitk_cylinder_unit_convert_from     (const amide_data_t cylinder_factor,
						   const AmitkCylinderUnit cylinder_unit);


/* external variables */
extern AmitkColorTable amitk_modality_default_color_table[];
extern gchar * amitk_interpolation_explanations[];
extern gchar * amitk_import_menu_names[];
extern gchar * amitk_import_menu_explanations[];
extern gchar * amitk_conversion_names[];
extern gchar * amitk_dose_unit_names[];
extern gchar * amitk_weight_unit_names[];
extern gchar * amitk_cylinder_unit_names[];
extern gchar * amitk_scaling_menu_names[];

G_END_DECLS

#endif /* __AMITK_DATA_SET_H__ */
