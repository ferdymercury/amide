/* amitk_study.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2002 Andy Loening
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

#ifndef __AMITK_STUDY_H__
#define __AMITK_STUDY_H__


#include "amitk_object.h"
#include "amitk_data_set.h"
#include "amitk_roi.h"
#include "amitk_fiducial_mark.h"

G_BEGIN_DECLS

#define	AMITK_TYPE_STUDY		(amitk_study_get_type ())
#define AMITK_STUDY(study)		(G_TYPE_CHECK_INSTANCE_CAST ((study), AMITK_TYPE_STUDY, AmitkStudy))
#define AMITK_STUDY_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_STUDY, AmitkStudyClass))
#define AMITK_IS_STUDY(study)		(G_TYPE_CHECK_INSTANCE_TYPE ((study), AMITK_TYPE_STUDY))
#define AMITK_IS_STUDY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_STUDY))
#define	AMITK_STUDY_GET_CLASS(study)	(G_TYPE_CHECK_GET_CLASS ((study), AMITK_TYPE_STUDY, AmitkStudyClass))

#define AMITK_STUDY_VIEW_CENTER(stu)      (amitk_space_s2b(AMITK_SPACE(stu), AMITK_STUDY(stu)->view_center))
#define AMITK_STUDY_VIEW_THICKNESS(stu)   (AMITK_STUDY(stu)->view_thickness)
#define AMITK_STUDY_ZOOM(stu)             (AMITK_STUDY(stu)->zoom)
#define AMITK_STUDY_VIEW_START_TIME(stu)  (AMITK_STUDY(stu)->view_start_time)
#define AMITK_STUDY_VIEW_DURATION(stu)    (AMITK_STUDY(stu)->view_duration)
#define AMITK_STUDY_INTERPOLATION(stu)    (AMITK_STUDY(stu)->interpolation)
#define AMITK_STUDY_CREATION_DATE(stu)    (AMITK_STUDY(stu)->creation_date)
#define AMITK_STUDY_FILENAME(stu)         (AMITK_STUDY(stu)->filename)
#define AMITK_STUDY_VOXEL_DIM(stu)        (AMITK_STUDY(stu)->voxel_dim)
#define AMITK_STUDY_FUSE_TYPE(stu)        (AMITK_STUDY(stu)->fuse_type)

//#define AMIDE_STUDY_FILENAME "study.xml"
#define AMIDE_FILE_VERSION "2.0"


typedef enum {
  AMITK_FUSE_TYPE_BLEND,
  AMITK_FUSE_TYPE_OVERLAY,
  AMITK_FUSE_TYPE_NUM
} AmitkFuseType;

typedef struct _AmitkStudyClass AmitkStudyClass;
typedef struct _AmitkStudy AmitkStudy;


struct _AmitkStudy
{
  AmitkObject parent;

  gchar * creation_date; /* when this study was created */

  /* view parameters */
  AmitkPoint view_center; /* wrt the study coordinate space */
  amide_real_t view_thickness;
  amide_time_t view_start_time;
  amide_time_t view_duration;
  amide_real_t zoom;
  AmitkInterpolation interpolation;
  AmitkFuseType fuse_type;

  /* stuff calculated when file is loaded and stored */
  amide_real_t voxel_dim; /* prefered voxel/pixel dim, canvas wants this info */

  /* stuff that doesn't need to be saved */
  gchar * filename; /* file name of the study */
};

struct _AmitkStudyClass
{
  AmitkObjectClass parent_class;

  void (* study_changed) (AmitkStudy * study);

};



/* Application-level methods */

GType	        amitk_study_get_type	             (void);
AmitkStudy *    amitk_study_new                     (void);
void            amitk_study_set_filename            (AmitkStudy * study, 
						     const gchar * new_filename);
void            amitk_study_set_creation_date       (AmitkStudy * study, 
						     const gchar * new_date); 
void            amitk_study_set_view_thickness      (AmitkStudy * study, 
						     const amide_real_t new_thickness);
void            amitk_study_set_view_center         (AmitkStudy * study,
						     const AmitkPoint new_center);
void            amitk_study_set_view_start_time     (AmitkStudy * study,
						     const amide_time_t new_start);
void            amitk_study_set_view_duration       (AmitkStudy * study,
						     const amide_time_t new_duration);
void            amitk_study_set_interpolation       (AmitkStudy * study,
						     const AmitkInterpolation new_interpolation);
void            amitk_study_set_fuse_type           (AmitkStudy * study,
						     const AmitkFuseType new_fuse_type);
void            amitk_study_set_zoom                (AmitkStudy * study,
						     const amide_real_t new_zoom);
AmitkStudy *    amitk_study_load_xml                (const gchar * study_directory);
gboolean        amitk_study_save_xml                (AmitkStudy * study, 
						     const gchar * study_directory);

const gchar *   amitk_fuse_type_get_name            (const AmitkFuseType fuse_type);


/* external variables */
extern gchar * amitk_fuse_type_explanations[];

G_END_DECLS

#endif /* __AMITK_STUDY_H__ */
