/* amitk_study.h
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

#ifndef __AMITK_STUDY_H__
#define __AMITK_STUDY_H__


#include "amitk_object.h"
#include "amitk_data_set.h"
#include "amitk_roi.h"
#include "amitk_fiducial_mark.h"
#include "amitk_preferences.h"
#include "amitk_line_profile.h"

G_BEGIN_DECLS

#define	AMITK_TYPE_STUDY		(amitk_study_get_type ())
#define AMITK_STUDY(study)		(G_TYPE_CHECK_INSTANCE_CAST ((study), AMITK_TYPE_STUDY, AmitkStudy))
#define AMITK_STUDY_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_STUDY, AmitkStudyClass))
#define AMITK_IS_STUDY(study)		(G_TYPE_CHECK_INSTANCE_TYPE ((study), AMITK_TYPE_STUDY))
#define AMITK_IS_STUDY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_STUDY))
#define	AMITK_STUDY_GET_CLASS(study)	(G_TYPE_CHECK_GET_CLASS ((study), AMITK_TYPE_STUDY, AmitkStudyClass))

#define AMITK_STUDY_VIEW_CENTER(stu)              (amitk_space_s2b(AMITK_SPACE(stu), AMITK_STUDY(stu)->view_center))
#define AMITK_STUDY_VIEW_THICKNESS(stu)           (AMITK_STUDY(stu)->view_thickness)
#define AMITK_STUDY_ZOOM(stu)                     (AMITK_STUDY(stu)->zoom)
#define AMITK_STUDY_FOV(stu)                      (AMITK_STUDY(stu)->fov)
#define AMITK_STUDY_VIEW_START_TIME(stu)          (AMITK_STUDY(stu)->view_start_time)
#define AMITK_STUDY_VIEW_DURATION(stu)            (AMITK_STUDY(stu)->view_duration)
#define AMITK_STUDY_CREATION_DATE(stu)            (AMITK_STUDY(stu)->creation_date)
#define AMITK_STUDY_FILENAME(stu)                 (AMITK_STUDY(stu)->filename)
#define AMITK_STUDY_VOXEL_DIM(stu)                (AMITK_STUDY(stu)->voxel_dim)
#define AMITK_STUDY_VOXEL_DIM_VALID(stu)          (AMITK_STUDY(stu)->voxel_dim_valid)
#define AMITK_STUDY_FUSE_TYPE(stu)                (AMITK_STUDY(stu)->fuse_type)
#define AMITK_STUDY_VIEW_MODE(stu)                (AMITK_STUDY(stu)->view_mode)
#define AMITK_STUDY_CANVAS_VISIBLE(stu, canvas)   (AMITK_STUDY(stu)->canvas_visible[canvas])
#define AMITK_STUDY_CANVAS_TARGET(stu)            (AMITK_STUDY(stu)->canvas_target)

#define AMITK_STUDY_CANVAS_ROI_WIDTH(stu)         (AMITK_STUDY(stu)->canvas_roi_width)
#ifdef AMIDE_LIBGNOMECANVAS_AA
#define AMITK_STUDY_CANVAS_ROI_TRANSPARENCY(stu)  (AMITK_STUDY(stu)->canvas_roi_transparency)
#else
#define AMITK_STUDY_CANVAS_LINE_STYLE(stu)        (AMITK_STUDY(stu)->canvas_line_style)
#define AMITK_STUDY_CANVAS_FILL_ROI(stu)          (AMITK_STUDY(stu)->canvas_fill_roi)
#endif
#define AMITK_STUDY_CANVAS_LAYOUT(stu)            (AMITK_STUDY(stu)->canvas_layout)
#define AMITK_STUDY_CANVAS_MAINTAIN_SIZE(stu)     (AMITK_STUDY(stu)->canvas_maintain_size)
#define AMITK_STUDY_CANVAS_TARGET_EMPTY_AREA(stu) (AMITK_STUDY(stu)->canvas_target_empty_area)
#define AMITK_STUDY_PANEL_LAYOUT(stu)             (AMITK_STUDY(stu)->panel_layout)

#define AMITK_STUDY_LINE_PROFILE(stu)             (AMITK_STUDY(stu)->line_profile)

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

  /* canvas view parameters */
  AmitkPoint view_center; /* wrt the study coordinate space */
  amide_real_t view_thickness;
  amide_time_t view_start_time;
  amide_time_t view_duration;
  amide_real_t zoom;
  amide_real_t fov; /* field of view, in percent */
  AmitkFuseType fuse_type;
  AmitkViewMode view_mode;
  gboolean canvas_visible[AMITK_VIEW_NUM];
  gboolean canvas_target; /* target on/off */

  /* canvas preferences */
  gint canvas_roi_width;
#ifdef AMIDE_LIBGNOMECANVAS_AA
  gdouble canvas_roi_transparency;
#else
  GdkLineStyle canvas_line_style;
  gboolean canvas_fill_roi;
#endif
  AmitkLayout canvas_layout;
  gboolean canvas_maintain_size;
  gint canvas_target_empty_area; /* in pixels */
  AmitkPanelLayout panel_layout;

  /* stuff calculated when file is loaded and stored */
  amide_real_t voxel_dim; /* prefered voxel/pixel dim, canvas wants this info */
  gboolean voxel_dim_valid;

  /* stuff that doesn't need to be saved */
  AmitkLineProfile * line_profile;
  gchar * filename; /* file name of the study */
};

struct _AmitkStudyClass
{
  AmitkObjectClass parent_class;

  void (* filename_changed) (AmitkStudy * study);
  void (* thickness_changed) (AmitkStudy * study);
  void (* time_changed) (AmitkStudy * study);
  void (* canvas_visible_changed) (AmitkStudy * study);
  void (* view_mode_changed) (AmitkStudy * study);
  void (* canvas_target_changed) (AmitkStudy * study);
  void (* voxel_dim_or_zoom_changed) (AmitkStudy * study);
  void (* fov_changed) (AmitkStudy * study);
  void (* fuse_type_changed) (AmitkStudy * study);
  void (* view_center_changed) (AmitkStudy * study);
  void (* canvas_roi_preference_changed) (AmitkStudy * study);
  void (* canvas_general_preference_changed) (AmitkStudy * study);
  void (* canvas_target_preference_changed) (AmitkStudy * study);
  void (* canvas_layout_preference_changed) (AmitkStudy * study);
  void (* panel_layout_preference_changed) (AmitkStudy * study);

};



/* Application-level methods */

GType	        amitk_study_get_type	             (void);
AmitkStudy *    amitk_study_new                     (AmitkPreferences * preferences);
void            amitk_study_set_filename            (AmitkStudy * study, 
						     const gchar * new_filename);
void            amitk_study_suggest_name            (AmitkStudy * study,
						     const gchar * suggested_name);
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
void            amitk_study_set_fuse_type           (AmitkStudy * study,
						     const AmitkFuseType new_fuse_type);
void            amitk_study_set_view_mode           (AmitkStudy * study,
						     const AmitkViewMode new_view_mode);
void            amitk_study_set_canvas_visible      (AmitkStudy * study,
						     const AmitkView view,
						     const gboolean visible);
void            amitk_study_set_zoom                (AmitkStudy * study,
						     const amide_real_t new_zoom);
void            amitk_study_set_fov                 (AmitkStudy * study,
						     const amide_real_t new_fov);
void            amitk_study_set_canvas_target       (AmitkStudy * study,
						     const gboolean always_on);
void            amitk_study_set_canvas_roi_width    (AmitkStudy * study,
						     gint roi_width);
#ifdef AMIDE_LIBGNOMECANVAS_AA
void            amitk_study_set_canvas_roi_transparency(AmitkStudy * study,
							const gdouble transparency);
#else
void            amitk_study_set_canvas_line_style   (AmitkStudy * study,
						     const GdkLineStyle line_style);
void            amitk_study_set_canvas_fill_roi     (AmitkStudy * study,
						     const gboolean fill_roi);
#endif
void            amitk_study_set_canvas_layout       (AmitkStudy * study,
						     const AmitkLayout layout);
void            amitk_study_set_canvas_maintain_size(AmitkStudy * study,
						     const gboolean maintain_size);
void            amitk_study_set_canvas_target_empty_area(AmitkStudy * study,
							 gint target_empty_area);
void            amitk_study_set_panel_layout        (AmitkStudy * study,
						     const AmitkPanelLayout panel_layout);
AmitkStudy *    amitk_study_recover_xml             (const gchar * study_filename, 
						     AmitkPreferences * preferences);
AmitkStudy *    amitk_study_load_xml                (const gchar * study_filename);
gboolean        amitk_study_save_xml                (AmitkStudy * study, 
						     const gchar * study_filename,
						     const gboolean save_as_directory);

const gchar *   amitk_fuse_type_get_name            (const AmitkFuseType fuse_type);
const gchar *   amitk_view_mode_get_name            (const AmitkViewMode view_mode);


G_END_DECLS

#endif /* __AMITK_STUDY_H__ */
