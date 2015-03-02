/* analysis.h
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

#ifndef __ANALYSIS_H__
#define __ANALYSIS_H__

/* header files that are always needed with this file */
#include "study.h"

/* defines */
#define ANALYSIS_GRANULARITY 4
#define ANALYSIS_GRAIN_SIZE 0.015625 /* 1/64 */
/* #define ANALYSIS_GRANULARITY 10 */
/* #define ANALYSIS_GRAIN_SIZE 0.001 */ /* 1/10^3 */

/* typedefs, etc. */

typedef struct _analysis_frame_t analysis_frame_t;
typedef struct _analysis_volume_t analysis_volume_t;
typedef struct _analysis_roi_t analysis_roi_t;
typedef struct _analysis_study_t analysis_study_t;

struct _analysis_frame_t {
  amide_time_t duration;
  amide_time_t time_midpoint;
  amide_data_t mean;
  amide_data_t voxels;
  amide_data_t var;
  amide_data_t min;
  amide_data_t max;
  analysis_frame_t * next_frame_analysis;
  guint ref_count;
};

struct _analysis_volume_t {
  volume_t * volume;
  analysis_frame_t * frame_analyses;
  guint ref_count;
  analysis_volume_t * next_volume_analysis;
};

struct _analysis_roi_t {
  roi_t * roi;
  study_t * study;
  analysis_volume_t * volume_analyses;
  guint ref_count;
  analysis_roi_t * next_roi_analysis;
};

/* external functions */
analysis_frame_t * analysis_frame_unref(analysis_frame_t * frame_analysis);
analysis_frame_t * analysis_frame_init(roi_t * roi, volume_t * volume);

analysis_volume_t * analysis_volume_unref(analysis_volume_t *volume_analysis);
analysis_volume_t * analysis_volume_init(roi_t * roi, volumes_t * volumes);

analysis_roi_t * analysis_roi_unref(analysis_roi_t *roi_analysis);
analysis_roi_t * analysis_roi_init(study_t * study, rois_t * rois, volumes_t * volumes);

/* variable type function declarations */
#include "analysis_ELLIPSOID.h"
#include "analysis_CYLINDER.h"
#include "analysis_BOX.h"
#include "analysis_ISOCONTOUR_2D.h"
#include "analysis_ISOCONTOUR_3D.h"

#endif /* __ANALYSIS_H__ */



















