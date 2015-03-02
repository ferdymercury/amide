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


typedef struct study_t {
  gchar * name; /* name of the study */
  gchar * filename; /* file name of the study */
  volume_list_t * volumes; 
  roi_list_t * rois;

  /* stuff that doesn't need to be saved */
  guint reference_count;
} study_t;

/* defines */

#define study_get_volumes(study) ((study)->volumes)
#define study_get_rois(study) ((study)->rois)
#define study_get_name(study) ((study)->name)
#define study_get_first_volume(study) ((study)->volumes->volume)

/* external functions */
study_t * study_free(study_t * study);
study_t * study_init(void);
study_t * study_add_reference(study_t * study);
void study_add_volume(study_t * study, volume_t * volume);
void study_remove_volume(study_t * study, volume_t * volume);
void study_add_roi(study_t * study, roi_t * roi);
void study_remove_roi(study_t * study, roi_t * roi);
void study_set_name(study_t * study, gchar * new_name);






