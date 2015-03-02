/* study.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000 Andy Loening
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


typedef struct amide_study_t {
  gchar * name; /* name of the study */
  gchar * filename; /* file name of the study */
  amide_volume_list_t * volumes; 
  amide_roi_list_t * rois;
} amide_study_t;

/* external functions */
void study_free(amide_study_t ** pstudy);
amide_study_t * study_init(void);
void study_set_name(amide_study_t * study, gchar * new_name);






