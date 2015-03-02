/* study.h
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

#ifndef __STUDY_H__
#define __STUDY_H__

/* header files that are always needed with this file */
#include "volume.h"
#include "roi.h"

typedef struct study_t {
  gchar * name; /* name of the study */
  gchar * creation_date; /* when this study was created */
  realspace_t * coord_frame;
  volumes_t * volumes; 
  rois_t * rois;

  /* view parameters */
  realpoint_t view_center; /* wrt the study coord_frame */
  floatpoint_t view_thickness;
  amide_time_t view_time;
  amide_time_t view_duration;
  floatpoint_t zoom;
  interpolation_t interpolation;

  /* stuff that doesn't need to be saved */
  guint ref_count;
  gchar * filename; /* file name of the study */

} study_t;

/* defines */

#define STUDY_FILENAME "Study.xml"
#define AMIDE_FILE_VERSION "1.4"
#define study_volumes(study) ((study)->volumes)
#define study_rois(study) ((study)->rois)
#define study_name(study) ((study)->name)
#define study_filename(study) ((study)->filename)
#define study_creation_date(study) ((study)->creation_date)
#define study_first_volume(study) ((study)->volumes->volume)
#define study_view_center(study) ((study)->view_center)
#define study_view_thickness(study) ((study)->view_thickness)
#define study_view_time(study) ((study)->view_time)
#define study_view_duration(study) ((study)->view_duration)
#define study_zoom(study) ((study)->zoom)
#define study_interpolation(study) ((study)->interpolation)
#define study_coord_frame(study) ((study)->coord_frame)
#define study_coord_frame_axis(study) (rs_all_axis((study)->coord_frame))
#define study_set_coord_frame_offset(study, new) (rs_set_offset(((study)->coord_frame),(new)))
#define study_set_view_center(study, new) ((study)->view_center = (new))
#define study_set_view_time(study, new) ((study)->view_time = (new))
#define study_set_view_duration(study, new) ((study)->view_duration = (new))
#define study_set_zoom(study, new) ((study)->zoom = (new))
#define study_set_interpolation(study, new) ((study)->interpolation = (new))

/* external functions */
study_t * study_unref(study_t * study);
study_t * study_init(void);
gboolean study_write_xml(study_t * study, gchar * study_directory);
study_t * study_load_xml(const gchar * study_directory);
study_t * study_copy(study_t * src_study);
study_t * study_ref(study_t * study);
void study_add_volume(study_t * study, volume_t * volume);
void study_remove_volume(study_t * study, volume_t * volume);
void study_add_volumes(study_t * study, volumes_t * volumes);
void study_add_roi(study_t * study, roi_t * roi);
void study_remove_roi(study_t * study, roi_t * roi);
void study_add_rois(study_t * study, rois_t * rois);
void study_set_name(study_t * study, const gchar * new_name);
void study_set_filename(study_t * study, const gchar * new_filename);
void study_set_creation_date(study_t * study, const gchar * new_date);
void study_set_coord_frame(study_t * study, const realspace_t * new_rs);
void study_set_view_thickness(study_t * study, const floatpoint_t new_thickness);
#endif /*__STUDY_H__ */


