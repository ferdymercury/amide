/* amitk_data_set.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2014 Andy Loening
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
#include "amitk_preferences.h"

G_BEGIN_DECLS

#define	AMITK_TYPE_DATA_SET		        (amitk_data_set_get_type ())
#define AMITK_DATA_SET(object)		        (G_TYPE_CHECK_INSTANCE_CAST ((object), AMITK_TYPE_DATA_SET, AmitkDataSet))
#define AMITK_DATA_SET_CLASS(klass)	        (G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_DATA_SET, AmitkDataSetClass))
#define AMITK_IS_DATA_SET(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), AMITK_TYPE_DATA_SET))
#define AMITK_IS_DATA_SET_CLASS(klass)	        (G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_DATA_SET))
#define	AMITK_DATA_SET_GET_CLASS(object)	(G_TYPE_CHECK_GET_CLASS ((object), AMITK_TYPE_DATA_SET, AmitkDataSetClass))

#define AMITK_DATA_SET_MODALITY(ds)                (AMITK_DATA_SET(ds)->modality)
#define AMITK_DATA_SET_VOXEL_SIZE(ds)              (AMITK_DATA_SET(ds)->voxel_size)
#define AMITK_DATA_SET_VOXEL_SIZE_X(ds)            (AMITK_DATA_SET(ds)->voxel_size.x)
#define AMITK_DATA_SET_VOXEL_SIZE_Y(ds)            (AMITK_DATA_SET(ds)->voxel_size.y)
#define AMITK_DATA_SET_VOXEL_SIZE_Z(ds)            (AMITK_DATA_SET(ds)->voxel_size.z)
#define AMITK_DATA_SET_VOXEL_VOLUME(ds)            (AMITK_DATA_SET(ds)->voxel_size.z*AMITK_DATA_SET(ds)->voxel_size.y*AMITK_DATA_SET(ds)->voxel_size.x)
#define AMITK_DATA_SET_RAW_DATA(ds)                (AMITK_DATA_SET(ds)->raw_data)
#define AMITK_DATA_SET_DIM(ds)                     (AMITK_RAW_DATA_DIM(AMITK_DATA_SET_RAW_DATA(ds)))
#define AMITK_DATA_SET_DIM_X(ds)                   (AMITK_RAW_DATA_DIM_X(AMITK_DATA_SET_RAW_DATA(ds)))
#define AMITK_DATA_SET_DIM_Y(ds)                   (AMITK_RAW_DATA_DIM_Y(AMITK_DATA_SET_RAW_DATA(ds)))
#define AMITK_DATA_SET_DIM_Z(ds)                   (AMITK_RAW_DATA_DIM_Z(AMITK_DATA_SET_RAW_DATA(ds)))
#define AMITK_DATA_SET_DIM_G(ds)                   (AMITK_RAW_DATA_DIM_G(AMITK_DATA_SET_RAW_DATA(ds)))
#define AMITK_DATA_SET_DIM_T(ds)                   (AMITK_RAW_DATA_DIM_T(AMITK_DATA_SET_RAW_DATA(ds)))
#define AMITK_DATA_SET_FORMAT(ds)                  (AMITK_RAW_DATA_FORMAT(AMITK_DATA_SET_RAW_DATA(ds)))
#define AMITK_DATA_SET_NUM_GATES(ds)               (AMITK_DATA_SET_DIM_G(ds))
#define AMITK_DATA_SET_NUM_FRAMES(ds)              (AMITK_DATA_SET_DIM_T(ds))
#define AMITK_DATA_SET_TOTAL_PLANES(ds)            (AMITK_DATA_SET_DIM_Z(ds)*AMITK_DATA_SET_DIM_G(ds)*AMITK_DATA_SET_DIM_T(ds))
#define AMITK_DATA_SET_DISTRIBUTION(ds)            (AMITK_DATA_SET(ds)->distribution)
#define AMITK_DATA_SET_COLOR_TABLE(ds, view_mode)  (AMITK_DATA_SET(ds)->color_table[view_mode])
#define AMITK_DATA_SET_COLOR_TABLE_INDEPENDENT(ds, view_mode) (AMITK_DATA_SET(ds)->color_table_independent[view_mode])
#define AMITK_DATA_SET_INTERPOLATION(ds)           (AMITK_DATA_SET(ds)->interpolation)
#define AMITK_DATA_SET_RENDERING(ds)               (AMITK_DATA_SET(ds)->rendering)
#define AMITK_DATA_SET_DYNAMIC(ds)                 (AMITK_DATA_SET_NUM_FRAMES(ds) > 1)
#define AMITK_DATA_SET_GATED(ds)                   (AMITK_DATA_SET_NUM_GATES(ds) > 1)
#define AMITK_DATA_SET_THRESHOLDING(ds)            (AMITK_DATA_SET(ds)->thresholding)
#define AMITK_DATA_SET_THRESHOLD_STYLE(ds)         (AMITK_DATA_SET(ds)->threshold_style)
#define AMITK_DATA_SET_SLICE_PARENT(ds)            (AMITK_DATA_SET(ds)->slice_parent)
#define AMITK_DATA_SET_SCAN_DATE(ds)               (AMITK_DATA_SET(ds)->scan_date)
#define AMITK_DATA_SET_SUBJECT_NAME(ds)            (AMITK_DATA_SET(ds)->subject_name)
#define AMITK_DATA_SET_SUBJECT_ID(ds)              (AMITK_DATA_SET(ds)->subject_id)
#define AMITK_DATA_SET_SUBJECT_DOB(ds)             (AMITK_DATA_SET(ds)->subject_dob)
#define AMITK_DATA_SET_SERIES_NUMBER(ds)           (AMITK_DATA_SET(ds)->series_number)
#define AMITK_DATA_SET_DICOM_IMAGE_TYPE(ds)        (AMITK_DATA_SET(ds)->dicom_image_type)
#define AMITK_DATA_SET_SCAN_START(ds)              (AMITK_DATA_SET(ds)->scan_start)
#define AMITK_DATA_SET_THRESHOLD_REF_FRAME(ds,ref_frame) (AMITK_DATA_SET(ds)->threshold_ref_frame[ref_frame])
#define AMITK_DATA_SET_THRESHOLD_MAX(ds, ref_frame)      (AMITK_DATA_SET(ds)->threshold_max[ref_frame])
#define AMITK_DATA_SET_THRESHOLD_MIN(ds, ref_frame)      (AMITK_DATA_SET(ds)->threshold_min[ref_frame])
#define AMITK_DATA_SET_SCALING_TYPE(ds)            (AMITK_DATA_SET(ds)->scaling_type)
#define AMITK_DATA_SET_SCALING_HAS_INTERCEPT(ds)   ((AMITK_DATA_SET(ds)->scaling_type == AMITK_SCALING_TYPE_0D_WITH_INTERCEPT) || (AMITK_DATA_SET(ds)->scaling_type == AMITK_SCALING_TYPE_1D_WITH_INTERCEPT) || (AMITK_DATA_SET(ds)->scaling_type == AMITK_SCALING_TYPE_2D_WITH_INTERCEPT)) 
#define AMITK_DATA_SET_SUBJECT_ORIENTATION(ds)     (AMITK_DATA_SET(ds)->subject_orientation)
#define AMITK_DATA_SET_SUBJECT_SEX(ds)             (AMITK_DATA_SET(ds)->subject_sex)

#define AMITK_DATA_SET_CONVERSION(ds)              (AMITK_DATA_SET(ds)->conversion)
#define AMITK_DATA_SET_SCALE_FACTOR(ds)            (AMITK_DATA_SET(ds)->scale_factor)
#define AMITK_DATA_SET_INJECTED_DOSE(ds)           (AMITK_DATA_SET(ds)->injected_dose)
#define AMITK_DATA_SET_DISPLAYED_DOSE_UNIT(ds)     (AMITK_DATA_SET(ds)->displayed_dose_unit)
#define AMITK_DATA_SET_SUBJECT_WEIGHT(ds)          (AMITK_DATA_SET(ds)->subject_weight)
#define AMITK_DATA_SET_DISPLAYED_WEIGHT_UNIT(ds)   (AMITK_DATA_SET(ds)->displayed_weight_unit)
#define AMITK_DATA_SET_CYLINDER_FACTOR(ds)         (AMITK_DATA_SET(ds)->cylinder_factor)
#define AMITK_DATA_SET_DISPLAYED_CYLINDER_UNIT(ds) (AMITK_DATA_SET(ds)->displayed_cylinder_unit)
#define AMITK_DATA_SET_INVERSION_TIME(ds)          (AMITK_DATA_SET(ds)->inversion_time)
#define AMITK_DATA_SET_ECHO_TIME(ds)               (AMITK_DATA_SET(ds)->echo_time)
#define AMITK_DATA_SET_DIFFUSION_B_VALUE(ds)       (AMITK_DATA_SET(ds)->diffusion_b_value)
#define AMITK_DATA_SET_DIFFUSION_DIRECTION(ds)     (AMITK_DATA_SET(ds)->diffusion_direction)
#define AMITK_DATA_SET_THRESHOLD_WINDOW(ds, i_win, limit) (AMITK_DATA_SET(ds)->threshold_window[i_win][limit])
#define AMITK_DATA_SET_VIEW_START_GATE(ds)         (AMITK_DATA_SET(ds)->view_start_gate)
#define AMITK_DATA_SET_VIEW_END_GATE(ds)           (AMITK_DATA_SET(ds)->view_end_gate)
#define AMITK_DATA_SET_NUM_VIEW_GATES(ds)          (AMITK_DATA_SET(ds)->num_view_gates)

#define AMITK_DATA_SET_DISTRIBUTION_SIZE 256

typedef enum {
  AMITK_OPERATION_UNARY_RESCALE,
  AMITK_OPERATION_UNARY_REMOVE_NEGATIVES,
  AMITK_OPERATION_UNARY_NUM
} AmitkOperationUnary;

typedef enum {
  AMITK_OPERATION_BINARY_ADD,
  AMITK_OPERATION_BINARY_SUB,
  AMITK_OPERATION_BINARY_MULTIPLY,
  AMITK_OPERATION_BINARY_DIVISION,
  AMITK_OPERATION_BINARY_T2STAR,
  AMITK_OPERATION_BINARY_NUM
} AmitkOperationBinary;


typedef enum {
  AMITK_INTERPOLATION_NEAREST_NEIGHBOR, 
  AMITK_INTERPOLATION_TRILINEAR, 
  AMITK_INTERPOLATION_NUM
} AmitkInterpolation;

typedef enum {
  AMITK_RENDERING_MPR,
  AMITK_RENDERING_MIP,
  AMITK_RENDERING_MINIP,
  AMITK_RENDERING_NUM
} AmitkRendering;

typedef enum {
  AMITK_THRESHOLDING_PER_SLICE, 
  AMITK_THRESHOLDING_PER_FRAME, 
  AMITK_THRESHOLDING_INTERPOLATE_FRAMES,
  AMITK_THRESHOLDING_GLOBAL, 
  AMITK_THRESHOLDING_NUM
} AmitkThresholding;

/* 2D is per plane scaling */
/* 1D is per frame/gate scaling */
/* 0D is global scaling */
typedef enum {
  AMITK_SCALING_TYPE_0D,
  AMITK_SCALING_TYPE_1D,
  AMITK_SCALING_TYPE_2D,
  AMITK_SCALING_TYPE_0D_WITH_INTERCEPT,
  AMITK_SCALING_TYPE_1D_WITH_INTERCEPT,
  AMITK_SCALING_TYPE_2D_WITH_INTERCEPT,
  AMITK_SCALING_TYPE_NUM
} AmitkScalingType;

typedef enum {
  AMITK_CONVERSION_STRAIGHT,
  AMITK_CONVERSION_PERCENT_ID_PER_CC,
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
  AMITK_CYLINDER_UNIT_MILLICURIE_PER_CC_IMAGE_UNIT,
  AMITK_CYLINDER_UNIT_MICROCURIE_PER_CC_IMAGE_UNIT,
  AMITK_CYLINDER_UNIT_NANOCURIE_PER_CC_IMAGE_UNIT,
  AMITK_CYLINDER_UNIT_IMAGE_UNIT_CC_PER_MEGABECQUEREL,
  AMITK_CYLINDER_UNIT_IMAGE_UNIT_CC_PER_MILLICURIE,
  AMITK_CYLINDER_UNIT_IMAGE_UNIT_CC_PER_MICROCURIE,
  AMITK_CYLINDER_UNIT_IMAGE_UNIT_CC_PER_NANOCURIE,
  AMITK_CYLINDER_UNIT_NUM
} AmitkCylinderUnit;

typedef enum {
  AMITK_SUBJECT_ORIENTATION_UNKNOWN,
  AMITK_SUBJECT_ORIENTATION_SUPINE_HEADFIRST,
  AMITK_SUBJECT_ORIENTATION_SUPINE_FEETFIRST,
  AMITK_SUBJECT_ORIENTATION_PRONE_HEADFIRST,
  AMITK_SUBJECT_ORIENTATION_PRONE_FEETFIRST,
  AMITK_SUBJECT_ORIENTATION_RIGHT_DECUBITUS_HEADFIRST,
  AMITK_SUBJECT_ORIENTATION_RIGHT_DECUBITUS_FEETFIRST,
  AMITK_SUBJECT_ORIENTATION_LEFT_DECUBITUS_HEADFIRST,
  AMITK_SUBJECT_ORIENTATION_LEFT_DECUBITUS_FEETFIRST,
  AMITK_SUBJECT_ORIENTATION_NUM
} AmitkSubjectOrientation;

typedef enum {
  AMITK_SUBJECT_SEX_UNKNOWN,
  AMITK_SUBJECT_SEX_MALE,
  AMITK_SUBJECT_SEX_FEMALE,
  AMITK_SUBJECT_SEX_NUM
} AmitkSubjectSex;

/* the skip is for glib-mkenums, it doesn't know how to handle ifdef's */
typedef enum { /*< skip >*/
  AMITK_IMPORT_METHOD_GUESS, 
  AMITK_IMPORT_METHOD_RAW, 
#ifdef AMIDE_LIBDCMDATA_SUPPORT
  AMITK_IMPORT_METHOD_DCMTK,
#endif
#ifdef AMIDE_LIBECAT_SUPPORT
  AMITK_IMPORT_METHOD_LIBECAT,
#endif
#ifdef AMIDE_LIBMDC_SUPPORT
  AMITK_IMPORT_METHOD_LIBMDC,
#endif
  AMITK_IMPORT_METHOD_NUM
} AmitkImportMethod;

/* the skip is for glib-mkenums, it doesn't know how to handle ifdef's */
typedef enum { /*< skip >*/
  AMITK_EXPORT_METHOD_RAW, 
#ifdef AMIDE_LIBDCMDATA_SUPPORT
  AMITK_EXPORT_METHOD_DCMTK,
#endif
#ifdef AMIDE_LIBMDC_SUPPORT
  AMITK_EXPORT_METHOD_LIBMDC,
#endif
  AMITK_EXPORT_METHOD_NUM
} AmitkExportMethod;

typedef struct _AmitkDataSetClass AmitkDataSetClass;
typedef struct _AmitkDataSet AmitkDataSet;


struct _AmitkDataSet
{
  AmitkVolume parent;

  /* parameters that are saved */
  gchar * scan_date; /* the time/day the image was acquired */
  gchar * subject_name; /* name of the subject */
  gchar * subject_id; /* id of the subject */
  gchar * subject_dob; /* date of birth of the subject */
  gint series_number; /* series number, used by dicom */
  gchar * dicom_image_type; /* dicom specific image type designator */
  AmitkModality modality;
  AmitkPoint voxel_size;  /* in mm */
  AmitkRawData * raw_data;
  AmitkScalingType scaling_type; /*  dimensions of internal scaling */
  AmitkRawData * internal_scaling_factor; /* internally (data set) supplied scaling factor */
  AmitkRawData * internal_scaling_intercept; /* internally (data set) supplied scaling intercept */
  amide_time_t * gate_time; /* array of the trigger times of each gate */
  amide_time_t scan_start;
  amide_time_t * frame_duration; /* array of the duration of each frame */
  AmitkColorTable color_table[AMITK_VIEW_MODE_NUM]; /* the color table to draw this volume in */
  gboolean color_table_independent[AMITK_VIEW_MODE_NUM]; /* whether to use the independent color tables for 2-way or 3-way linked modes*/
  AmitkInterpolation interpolation;
  AmitkRendering rendering; /* rendering mode - MPR/MIP/MINIP */
  AmitkSubjectOrientation subject_orientation; /* orientation of subject in scanner */
  AmitkSubjectSex subject_sex;

  amide_data_t scale_factor; /* user specified factor to multiply data set by */
  AmitkConversion conversion;
  amide_data_t injected_dose;  /* in MBq */
  AmitkDoseUnit displayed_dose_unit;
  amide_data_t subject_weight; /* in KG */
  AmitkWeightUnit displayed_weight_unit;
  amide_data_t cylinder_factor; /* (MBq/cc)/Image Unit */
  AmitkCylinderUnit displayed_cylinder_unit;

  /* MRI parameters */
  amide_time_t inversion_time; /* in milliseconds */
  amide_time_t echo_time; /* in milliseconds */
  gdouble diffusion_b_value;
  AmitkPoint diffusion_direction;

  /* Thresholding */
  AmitkThresholding thresholding; /* what sort of thresholding we're using (per slice, global, etc.) */
  AmitkThresholdStyle threshold_style; /* min/max or center/width */
  amide_data_t threshold_max[2]; /* the thresholds to use for this volume */
  amide_data_t threshold_min[2]; 
  guint threshold_ref_frame[2];
  amide_data_t threshold_window[AMITK_WINDOW_NUM][AMITK_LIMIT_NUM];

  /* which gates we're showing */
  amide_intpoint_t view_start_gate;
  amide_intpoint_t view_end_gate;
  
  /* parameters calculated as needed or on loading the object */
  /* in theory, could be recalculated on the fly, but used enough we'll store... */
  AmitkRawData * distribution; /* 1D array of data distribution, used in thresholding */
  gboolean min_max_calculated; /* the min/max values can be calculated on demand */
  amide_data_t global_max;
  amide_data_t global_min;
  amide_data_t * frame_max; 
  amide_data_t * frame_min;
  AmitkRawData * current_scaling_factor; /* external_scaling * internal_scaling_factor[] */
  amide_intpoint_t num_view_gates;

  GList * slice_cache;

  /* only used by derived data sets (slices and projections)  */
  /* this is a weak pointer, it should be NULL'ed automatically by gtk on the parent's destruction */
  AmitkDataSet * slice_parent; 

  /* misc data items - not saved in .xif file */
  gint instance_number; /* used by dcmtk_interface.cc occasionally for sorting */
  gint gate_num; /* used by dcmtk_interface.cc occasionally for sorting */

};

struct _AmitkDataSetClass
{
  AmitkVolumeClass parent_class;

  void (* thresholding_changed)         (AmitkDataSet * ds);
  void (* threshold_style_changed)      (AmitkDataSet * ds);
  void (* thresholds_changed)           (AmitkDataSet * ds);
  void (* windows_changed)              (AmitkDataSet * ds);
  void (* color_table_changed)          (AmitkDataSet * ds,
					 AmitkViewMode * view_mode);
  void (* color_table_independent_changed)(AmitkDataSet * ds,
					   AmitkViewMode * view_mode);
  void (* interpolation_changed)        (AmitkDataSet * ds);
  void (* rendering_changed)            (AmitkDataSet * ds);
  void (* subject_orientation_changed)  (AmitkDataSet * ds);
  void (* subject_sex_changed)          (AmitkDataSet * ds);
  void (* conversion_changed)           (AmitkDataSet * ds);
  void (* scale_factor_changed)         (AmitkDataSet * ds);
  void (* modality_changed)             (AmitkDataSet * ds);
  void (* time_changed)                 (AmitkDataSet * ds);
  void (* voxel_size_changed)           (AmitkDataSet * ds);
  void (* data_set_changed)             (AmitkDataSet * ds);
  void (* invalidate_slice_cache)       (AmitkDataSet * ds);
  void (* view_gates_changed)           (AmitkDataSet * ds);

};

/* Application-level methods */

GType	        amitk_data_set_get_type	         (void);
AmitkDataSet *  amitk_data_set_new               (AmitkPreferences * preferences,
						  const AmitkModality modality);
AmitkDataSet *  amitk_data_set_new_with_data     (AmitkPreferences * preferences,
						  const AmitkModality modality,
						  const AmitkFormat format, 
						  const AmitkVoxel dim,
						  const AmitkScalingType scaling_type);
AmitkDataSet * amitk_data_set_import_raw_file    (const gchar * file_name, 
						  const AmitkRawFormat raw_format,
						  const AmitkVoxel data_dim,
						  guint file_offset,
						  AmitkPreferences * preferences,
						  const AmitkModality modality,
						  const gchar * data_set_name,
						  const AmitkPoint voxel_size,
						  const amide_data_t scale_factor,
						  AmitkUpdateFunc update_func,
						  gpointer update_data);
GList *        amitk_data_set_import_file        (AmitkImportMethod method, 
						  int submethod,
						  const gchar * filename,
						  gchar ** pstudyname,
						  AmitkPreferences * preferences,
						  AmitkUpdateFunc update_func,
						  gpointer update_data);
gboolean       amitk_data_set_export_to_file     (AmitkDataSet * ds,
						  const AmitkExportMethod method, 
						  const int submethod,
						  const gchar * filename,
						  const gchar * studyname,
						  const gboolean resliced,
						  const AmitkPoint voxel_size,
						  const AmitkVolume * bounding_box,
						  AmitkUpdateFunc update_func,
						  gpointer update_data);
gboolean       amitk_data_sets_export_to_file    (GList * data_sets,
						  const AmitkExportMethod method, 
						  const int submethod,
						  const gchar * filename,
						  const gchar * studyname,
						  const AmitkPoint voxel_size,
						  const AmitkVolume * bounding_box,
						  AmitkUpdateFunc update_func,
						  gpointer update_data);
amide_data_t   amitk_data_set_get_global_max     (AmitkDataSet * ds);
amide_data_t   amitk_data_set_get_global_min     (AmitkDataSet * ds);
amide_data_t   amitk_data_set_get_frame_max      (AmitkDataSet * ds,
						  const guint frame);
amide_data_t   amitk_data_set_get_frame_min      (AmitkDataSet * ds,
						  const guint frame);
AmitkColorTable amitk_data_set_get_color_table_to_use(AmitkDataSet * ds, 
						      const AmitkViewMode view_mode);
void           amitk_data_set_set_modality       (AmitkDataSet * ds,
						  const AmitkModality modality);
void           amitk_data_set_set_scan_start     (AmitkDataSet * ds,
						  const amide_time_t start);
void           amitk_data_set_set_frame_duration (AmitkDataSet * ds, 
						  const guint frame,
						  amide_time_t duration);
void           amitk_data_set_set_voxel_size     (AmitkDataSet * ds,
						  const AmitkPoint voxel_size);
void           amitk_data_set_set_thresholding  (AmitkDataSet * ds, 
						 const AmitkThresholding thresholding);
void           amitk_data_set_set_threshold_style(AmitkDataSet * ds,
						  const AmitkThresholdStyle threshold_style);
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
						  const AmitkViewMode view_mode,
						  const AmitkColorTable new_color_table);
void           amitk_data_set_set_color_table_independent(AmitkDataSet * ds,
							  const AmitkViewMode view_mode,
							  const gboolean independent);
void           amitk_data_set_set_interpolation  (AmitkDataSet * ds,
						  const AmitkInterpolation new_interpolation);
void           amitk_data_set_set_rendering      (AmitkDataSet * ds,
						  const AmitkRendering new_rendering);
void           amitk_data_set_set_subject_orientation    (AmitkDataSet * ds,
							  const AmitkSubjectOrientation subject_orientation);
void           amitk_data_set_set_subject_sex    (AmitkDataSet * ds,
						  const AmitkSubjectSex subject_sex);
void           amitk_data_set_set_scan_date      (AmitkDataSet * ds, 
						  const gchar * new_date);
void           amitk_data_set_set_subject_name   (AmitkDataSet * ds, 
						  const gchar * new_name);
void           amitk_data_set_set_subject_id     (AmitkDataSet * ds, 
						  const gchar * new_id);
void           amitk_data_set_set_subject_dob    (AmitkDataSet * ds, 
						  const gchar * new_dob);
void           amitk_data_set_set_series_number  (AmitkDataSet * ds,
						  const gint new_series_number);
void           amitk_data_set_set_dicom_image_type(AmitkDataSet * ds,
						   const gchar * image_type);
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
void           amitk_data_set_set_inversion_time   (AmitkDataSet * ds,
						    amide_time_t new_inversion_time);
void           amitk_data_set_set_echo_time        (AmitkDataSet * ds,
						    amide_time_t new_echo_time);
void           amitk_data_set_set_diffusion_b_value(AmitkDataSet * ds,
						    gdouble b_value);
void           amitk_data_set_set_diffusion_direction(AmitkDataSet * ds,
						      AmitkPoint direction);
void           amitk_data_set_set_threshold_window (AmitkDataSet * ds,
						    const AmitkWindow window,
						    const AmitkLimit limit,
						    const amide_data_t value);
void           amitk_data_set_set_view_start_gate(AmitkDataSet * ds,
						  amide_intpoint_t start_gate);
void           amitk_data_set_set_view_end_gate  (AmitkDataSet * ds,
						  amide_intpoint_t end_gate);
void           amitk_data_set_set_gate_time      (AmitkDataSet * ds,
						  const guint gate,
						  amide_time_t time);
amide_time_t   amitk_data_set_get_gate_time      (const AmitkDataSet * ds,
						  const guint gate);
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

/* note: calling any of the get_*_max or get_*_min functions will automatically
   call calc_min_max if needed.  The main reason to call this function independently
   is if you know the min/max values will be needed later, and you'd like to put up a 
   progress dialog. */
void           amitk_data_set_calc_min_max       (AmitkDataSet * ds,
						  AmitkUpdateFunc update_func,
						  gpointer update_data);
void           amitk_data_set_calc_min_max_if_needed(AmitkDataSet * ds,
						     AmitkUpdateFunc update_func,
						     gpointer update_data);
void           amitk_data_set_slice_calc_min_max (AmitkDataSet * ds,
						  const amide_intpoint_t frame,
						  const amide_intpoint_t gate,
						  const amide_intpoint_t z,
						  amitk_format_DOUBLE_t * pmin,
						  amitk_format_DOUBLE_t * pmax);
amide_data_t   amitk_data_set_get_max            (AmitkDataSet * ds, 
						  const amide_time_t start, 
						  const amide_time_t duration);
amide_data_t   amitk_data_set_get_min            (AmitkDataSet * ds, 
						  const amide_time_t start, 
						  const amide_time_t duration);
void           amitk_data_set_get_thresholding_min_max(AmitkDataSet * ds, 
						       AmitkDataSet * slice,
						       const amide_time_t start,
						       const amide_time_t duration,
						       amide_data_t * min, amide_data_t * max);
void           amitk_data_set_calc_distribution   (AmitkDataSet * ds, 
						   AmitkUpdateFunc update_func,
						   gpointer update_data);
amide_data_t   amitk_data_set_get_internal_value  (const AmitkDataSet * ds, 
						   const AmitkVoxel i);
amide_data_t   amitk_data_set_get_value           (const AmitkDataSet * ds, 
						   const AmitkVoxel i);
amide_data_t   amitk_data_set_get_internal_scaling_factor(const AmitkDataSet * ds, 
							  const AmitkVoxel i);
amide_data_t   amitk_data_set_get_scaling_factor  (const AmitkDataSet * ds,
						   const AmitkVoxel i);
amide_data_t   amitk_data_set_get_scaling_intercept(const AmitkDataSet * ds, 
						    const AmitkVoxel i);
void           amitk_data_set_set_value           (AmitkDataSet *ds,
						   const AmitkVoxel i,
						   const amide_data_t value,
						   const gboolean signal_change);
void           amitk_data_set_set_internal_value  (AmitkDataSet *ds,
						   const AmitkVoxel i,
						   const amide_data_t internal_value,
						   const gboolean signal_change);
void           amitk_data_set_get_projections     (AmitkDataSet * ds,
						   const guint frame,
						   const guint gate,
						   AmitkDataSet ** projections,
						   AmitkUpdateFunc update_func,
						   gpointer update_data);
AmitkDataSet * amitk_data_set_get_cropped         (const AmitkDataSet * ds,
						   const AmitkVoxel start,
						   const AmitkVoxel end,
						   const AmitkFormat format,
						   const AmitkScalingType scaling_type,
						   AmitkUpdateFunc update_func,
						   gpointer update_data);
AmitkDataSet * amitk_data_set_get_filtered        (const AmitkDataSet * ds,
						   const AmitkFilter filter_type,
						   const gint kernel_size,
						   const amide_real_t fwhm,
						   AmitkUpdateFunc update_func,
						   gpointer update_data);
AmitkDataSet * amitk_data_set_get_slice           (AmitkDataSet * ds,
						   const amide_time_t start,
						   const amide_time_t duration,
						   const amide_intpoint_t gate,
						   const AmitkCanvasPoint pixel_size,
						   const AmitkVolume * slice_volume);
void           amitk_data_set_get_line_profile    (AmitkDataSet * ds,
						   const amide_time_t start,
						   const amide_time_t duration,
						   const AmitkPoint start_point,
						   const AmitkPoint end_point,
						   GPtrArray ** preturn_data);




gint           amitk_data_sets_count                 (GList * objects, gboolean recurse);
amide_time_t   amitk_data_sets_get_min_frame_duration(GList * objects);
amide_real_t   amitk_data_sets_get_min_voxel_size    (GList * objects);
amide_real_t   amitk_data_sets_get_max_min_voxel_size(GList * objects);
GList *        amitk_data_sets_get_slices            (GList * objects,
						      GList ** pslice_cache,
						      const gint max_slice_cache_size,
						      const amide_time_t start,
						      const amide_time_t duration,
						      const amide_intpoint_t gate,
						      const AmitkCanvasPoint pixel_size,
						      const AmitkVolume * view_volume);
AmitkDataSet * amitk_data_sets_find_with_slice_parent(GList * slices, 
						      const AmitkDataSet * slice_parent);
GList *        amitk_data_sets_remove_with_slice_parent(GList * slices,
							const AmitkDataSet * slice_parent);
AmitkDataSet * amitk_data_sets_math_unary            (AmitkDataSet * ds1, 
						      AmitkOperationUnary operation,
						      amide_data_t parameter0,
						      amide_data_t parameter1,
						      AmitkUpdateFunc update_func,
						      gpointer update_data);
AmitkDataSet * amitk_data_sets_math_binary           (AmitkDataSet * ds1, 
						      AmitkDataSet * ds2, 
						      AmitkOperationBinary operation,
						      amide_data_t parameter0,
						      amide_data_t parameter1,
						      gboolean by_frame,
						      gboolean maintain_ds1_dim,
						      AmitkUpdateFunc update_func,
						      gpointer update_data);




/* -------- defines ----------- */
#define amitk_data_set_get_gate_time_mem(ds) (g_try_new0(amide_time_t,(ds)->raw_data->dim.g))
#define amitk_data_set_get_frame_duration_mem(ds) (g_try_new0(amide_time_t,(ds)->raw_data->dim.t))
#define amitk_data_set_get_frame_min_max_mem(ds) (g_try_new0(amide_data_t,(ds)->raw_data->dim.t))


const gchar *   amitk_scaling_type_get_name       (const AmitkScalingType scaling_type);
const gchar *   amitk_operation_unary_get_name    (const AmitkOperationUnary operation);
const gchar *   amitk_operation_binary_get_name   (const AmitkOperationBinary operation);
const gchar *   amitk_interpolation_get_name      (const AmitkInterpolation interpolation);
const gchar *   amitk_rendering_get_name          (const AmitkRendering rendering);
const gchar *   amitk_subject_orientation_get_name(const AmitkSubjectOrientation subject_orientation);
const gchar *   amitk_subject_sex_get_name        (const AmitkSubjectSex subject_sex);
const gchar *   amitk_thresholding_get_name       (const AmitkThresholding thresholding);
const gchar *   amitk_threshold_style_get_name    (const AmitkThresholdStyle threshold_style);
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
extern const gchar * amitk_interpolation_explanations[];
extern const gchar * amitk_rendering_explanation;
extern const gchar * amitk_import_menu_names[];
extern const gchar * amitk_import_menu_explanations[];
extern const gchar * amitk_export_menu_names[];
extern const gchar * amitk_export_menu_explanations[];
extern const gchar * amitk_conversion_names[];
extern const gchar * amitk_dose_unit_names[];
extern const gchar * amitk_weight_unit_names[];
extern const gchar * amitk_cylinder_unit_names[];
extern const gchar * amitk_scaling_menu_names[];
extern amide_data_t amitk_window_default[AMITK_WINDOW_NUM][AMITK_LIMIT_NUM];


G_END_DECLS

#endif /* __AMITK_DATA_SET_H__ */
