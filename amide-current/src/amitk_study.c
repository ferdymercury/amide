/* amitk_study.c
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

#include "amide_config.h"

#include <time.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <glib/gstdio.h> /* for g_mkdir */

#include "amitk_study.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"
#include "legacy.h"


enum {
  FILENAME_CHANGED,
  THICKNESS_CHANGED,
  TIME_CHANGED,
  CANVAS_VISIBLE_CHANGED,
  VIEW_MODE_CHANGED,
  CANVAS_TARGET_CHANGED,
  VOXEL_DIM_OR_ZOOM_CHANGED,
  FOV_CHANGED,
  FUSE_TYPE_CHANGED,
  VIEW_CENTER_CHANGED,
  CANVAS_ROI_PREFERENCE_CHANGED,
  CANVAS_GENERAL_PREFERENCE_CHANGED,
  CANVAS_TARGET_PREFERENCE_CHANGED,
  CANVAS_LAYOUT_PREFERENCE_CHANGED,
  PANEL_LAYOUT_PREFERENCE_CHANGED,
  LAST_SIGNAL
};


static gchar * blank_name = N_("new");


static void          study_class_init          (AmitkStudyClass     *klass);
static void          study_init                (AmitkStudy          *study);
static void          study_finalize            (GObject             *object);
static AmitkObject * study_copy                (const AmitkObject   *object);
static void          study_copy_in_place       (AmitkObject * dest_object, const AmitkObject * src_object);
static void          study_write_xml           (const AmitkObject   *object, 
						xmlNodePtr           nodes,
						FILE                *study_file);
static gchar *       study_read_xml            (AmitkObject         *object,
						xmlNodePtr           nodes,
						FILE                *study_file,
						gchar               *error_buf);
static void          study_recalc_voxel_dim    (AmitkStudy          *study);
static void          study_add_child           (AmitkObject         *object,
						AmitkObject         *child);
static void          study_remove_child        (AmitkObject         *object,
						AmitkObject         *child);

static AmitkObjectClass * parent_class;
static guint         study_signals[LAST_SIGNAL];



GType amitk_study_get_type(void) {

  static GType study_type = 0;

  if (!study_type)
    {
      static const GTypeInfo study_info =
      {
	sizeof (AmitkStudyClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) study_class_init,
	(GClassFinalizeFunc) NULL,
	NULL,		/* class_data */
	sizeof (AmitkStudy),
	0,			/* n_preallocs */
	(GInstanceInitFunc) study_init,
	NULL /* value table */
      };
      
      study_type = g_type_register_static (AMITK_TYPE_OBJECT, "AmitkStudy", &study_info, 0);
    }

  return study_type;
}


static void study_class_init (AmitkStudyClass * class) {

  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  AmitkObjectClass * object_class = AMITK_OBJECT_CLASS(class);

  parent_class = g_type_class_peek_parent(class);

  object_class->object_copy = study_copy;
  object_class->object_copy_in_place = study_copy_in_place;
  object_class->object_write_xml = study_write_xml;
  object_class->object_read_xml = study_read_xml;
  object_class->object_add_child = study_add_child;
  object_class->object_remove_child = study_remove_child;

  gobject_class->finalize = study_finalize;

  study_signals[FILENAME_CHANGED] =
    g_signal_new ("filename_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkStudyClass, filename_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);
  study_signals[THICKNESS_CHANGED] =
    g_signal_new ("thickness_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkStudyClass, thickness_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);
  study_signals[TIME_CHANGED] =
    g_signal_new ("time_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkStudyClass, time_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);
  study_signals[CANVAS_VISIBLE_CHANGED] =
    g_signal_new ("canvas_visible_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkStudyClass, canvas_visible_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);
  study_signals[VIEW_MODE_CHANGED] =
    g_signal_new ("view_mode_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkStudyClass, view_mode_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);
  study_signals[CANVAS_TARGET_CHANGED] =
    g_signal_new ("canvas_target_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkStudyClass, canvas_target_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);
  study_signals[VOXEL_DIM_OR_ZOOM_CHANGED] =
    g_signal_new ("voxel_dim_or_zoom_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkStudyClass, voxel_dim_or_zoom_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);
  study_signals[FOV_CHANGED] =
    g_signal_new ("fov_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkStudyClass, fov_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);
  study_signals[FUSE_TYPE_CHANGED] =
    g_signal_new ("fuse_type_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkStudyClass, fuse_type_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);
  study_signals[VIEW_CENTER_CHANGED] =
    g_signal_new ("view_center_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkStudyClass, view_center_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);
  study_signals[CANVAS_ROI_PREFERENCE_CHANGED] =
    g_signal_new ("canvas_roi_preference_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkStudyClass, canvas_roi_preference_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);
  study_signals[CANVAS_GENERAL_PREFERENCE_CHANGED] =
    g_signal_new ("canvas_general_preference_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkStudyClass, canvas_general_preference_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);
  study_signals[CANVAS_TARGET_PREFERENCE_CHANGED] =
    g_signal_new ("canvas_target_preference_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkStudyClass, canvas_target_preference_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);
  study_signals[CANVAS_LAYOUT_PREFERENCE_CHANGED] =
    g_signal_new ("canvas_layout_preference_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkStudyClass, canvas_layout_preference_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);
  study_signals[PANEL_LAYOUT_PREFERENCE_CHANGED] =
    g_signal_new ("panel_layout_preference_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkStudyClass, panel_layout_preference_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);
}


static void study_init (AmitkStudy * study) {

  time_t current_time;
  AmitkView i_view;

  study->filename = NULL;

  /* view parameters */
  study->view_center = zero_point;
  study->view_thickness = 1.0;
  study->view_start_time = 0.0;
  study->view_duration = 1.0;
  study->zoom = 1.0;
  study->fov = 100.0; 
  study->fuse_type = AMITK_FUSE_TYPE_BLEND;
  study->view_mode = AMITK_VIEW_MODE_SINGLE;
  study->voxel_dim = 1.0;
  study->voxel_dim_valid=FALSE;
  for (i_view=0; i_view<AMITK_VIEW_NUM; i_view++)
    study->canvas_visible[i_view] = TRUE;
  study->canvas_target = FALSE;

  /* preferences for the canvas */
  study->canvas_roi_width = AMITK_PREFERENCES_DEFAULT_CANVAS_ROI_WIDTH;
#ifdef AMIDE_LIBGNOMECANVAS_AA
  study->canvas_roi_transparency = AMITK_PREFERENCES_DEFAULT_CANVAS_ROI_TRANSPARENCY;
#else
  study->canvas_line_style = AMITK_PREFERENCES_DEFAULT_CANVAS_LINE_STYLE;
  study->canvas_fill_roi = AMITK_PREFERENCES_DEFAULT_CANVAS_FILL_ROI;
#endif
  study->canvas_layout = AMITK_PREFERENCES_DEFAULT_CANVAS_LAYOUT;
  study->canvas_maintain_size = AMITK_PREFERENCES_DEFAULT_CANVAS_LAYOUT;
  study->canvas_target_empty_area = AMITK_PREFERENCES_DEFAULT_CANVAS_TARGET_EMPTY_AREA; 
  study->panel_layout = AMITK_PREFERENCES_DEFAULT_PANEL_LAYOUT;

  /* line profile stuff */
  study->line_profile = amitk_line_profile_new();

  /* set the creation date as today */
  study->creation_date = NULL;
  time(&current_time);
  amitk_study_set_creation_date(study, ctime(&current_time));
  g_strdelimit(study->creation_date, "\n", ' '); /* turns newlines to white space */
  g_strstrip(study->creation_date); /* removes trailing and leading white space */
      
 return; 
}


static void study_finalize (GObject * object) {
  AmitkStudy * study = AMITK_STUDY(object);

  if (study->creation_date != NULL) {
    g_free(study->creation_date);
    study->creation_date = NULL;
  }
   
  if (study->filename != NULL) {
    g_free(study->filename);
    study->filename = NULL;
  }

  if (study->line_profile != NULL) {
    g_object_unref(study->line_profile);
    study->line_profile = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static AmitkObject * study_copy (const AmitkObject * object) {

  AmitkStudy * copy;

  g_return_val_if_fail(AMITK_IS_STUDY(object), NULL);

  copy = amitk_study_new(NULL);
  amitk_object_copy_in_place(AMITK_OBJECT(copy), object);

  return AMITK_OBJECT(copy);
}

static void study_copy_in_place(AmitkObject * dest_object, const AmitkObject * src_object) {

  AmitkStudy * dest_study;
  AmitkView i_view;
 
  g_return_if_fail(AMITK_IS_STUDY(src_object));
  g_return_if_fail(AMITK_IS_STUDY(dest_object));

  dest_study = AMITK_STUDY(dest_object);

  amitk_study_set_creation_date(dest_study, AMITK_STUDY_CREATION_DATE(src_object));
  dest_study->view_center = AMITK_STUDY(src_object)->view_center;
  amitk_study_set_view_thickness(dest_study, AMITK_STUDY_VIEW_THICKNESS(src_object));
  dest_study->view_start_time = AMITK_STUDY_VIEW_START_TIME(src_object);
  dest_study->view_duration = AMITK_STUDY_VIEW_DURATION(src_object);
  dest_study->zoom = AMITK_STUDY_ZOOM(src_object);
  dest_study->fov = AMITK_STUDY_FOV(src_object);
  dest_study->fuse_type = AMITK_STUDY_FUSE_TYPE(src_object);
  dest_study->view_mode = AMITK_STUDY_VIEW_MODE(src_object);
  dest_study->canvas_target = AMITK_STUDY_CANVAS_TARGET(src_object);
  for (i_view=0; i_view<AMITK_VIEW_NUM; i_view++)
    dest_study->canvas_visible[i_view] = AMITK_STUDY_CANVAS_VISIBLE(src_object, i_view);

  /* preferences */
  dest_study->canvas_roi_width = AMITK_STUDY_CANVAS_ROI_WIDTH(src_object);
#ifdef AMIDE_LIBGNOMECANVAS_AA
  dest_study->canvas_roi_transparency = AMITK_STUDY_CANVAS_ROI_TRANSPARENCY(src_object);
#else
  dest_study->canvas_line_style = AMITK_STUDY_CANVAS_LINE_STYLE(src_object);
  dest_study->canvas_fill_roi = AMITK_STUDY_CANVAS_FILL_ROI(src_object);
#endif
  dest_study->canvas_layout = AMITK_STUDY_CANVAS_LAYOUT(src_object);
  dest_study->canvas_maintain_size = AMITK_STUDY_CANVAS_MAINTAIN_SIZE(src_object);
  dest_study->canvas_target_empty_area = AMITK_STUDY_CANVAS_TARGET_EMPTY_AREA(src_object);
  dest_study->panel_layout = AMITK_STUDY_PANEL_LAYOUT(src_object);

  /* line profile */
  amitk_line_profile_copy_in_place(dest_study->line_profile, AMITK_STUDY_LINE_PROFILE(src_object));

  /* make a separate copy in memory of the study's name and filename */
  amitk_study_set_filename(dest_study, AMITK_STUDY_FILENAME(src_object));

  AMITK_OBJECT_CLASS (parent_class)->object_copy_in_place (dest_object, src_object);
}



static void study_write_xml(const AmitkObject * object,xmlNodePtr nodes,  FILE *study_file) {


  AmitkStudy * study;
  AmitkView i_view;
  gchar * temp_string;

  AMITK_OBJECT_CLASS(parent_class)->object_write_xml(object, nodes, study_file);

  study = AMITK_STUDY(object);

  xml_save_string(nodes, "creation_date", AMITK_STUDY_CREATION_DATE(study));
  amitk_point_write_xml(nodes, "view_center",  AMITK_STUDY_VIEW_CENTER(study));
  xml_save_real(nodes, "view_thickness", AMITK_STUDY_VIEW_THICKNESS(study));
  xml_save_time(nodes, "view_start_time", AMITK_STUDY_VIEW_START_TIME(study));
  xml_save_time(nodes, "view_duration", AMITK_STUDY_VIEW_DURATION(study));
  xml_save_real(nodes, "zoom", AMITK_STUDY_ZOOM(study));
  xml_save_real(nodes, "fov", AMITK_STUDY_FOV(study));
  xml_save_string(nodes, "fuse_type", amitk_fuse_type_get_name(AMITK_STUDY_FUSE_TYPE(study)));
  xml_save_string(nodes, "view_mode", amitk_view_mode_get_name(AMITK_STUDY_VIEW_MODE(study)));
  for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view++) {
    temp_string = g_strdup_printf("%s_canvas_visible", amitk_view_get_name(i_view));
    xml_save_boolean(nodes, temp_string, AMITK_STUDY_CANVAS_VISIBLE(study, i_view));
    g_free(temp_string);
  }
  xml_save_boolean(nodes, "canvas_target", AMITK_STUDY_CANVAS_TARGET(study));

  /* preferences */
  xml_save_int(nodes, "canvas_roi_width", AMITK_STUDY_CANVAS_ROI_WIDTH(study));
#ifdef AMIDE_LIBGNOMECANVAS_AA
  xml_save_real(nodes, "canvas_roi_transparency", AMITK_STUDY_CANVAS_ROI_TRANSPARENCY(study));
#else
  xml_save_int(nodes, "canvas_line_style", AMITK_STUDY_CANVAS_LINE_STYLE(study));
  xml_save_int(nodes, "canvas_fill_roi", AMITK_STUDY_CANVAS_FILL_ROI(study));
#endif
  xml_save_string(nodes, "canvas_layout", amitk_layout_get_name(AMITK_STUDY_CANVAS_LAYOUT(study)));
  xml_save_boolean(nodes, "canvas_maintain_size", AMITK_STUDY_CANVAS_MAINTAIN_SIZE(study));
  xml_save_int(nodes, "canvas_target_empty_area", AMITK_STUDY_CANVAS_TARGET_EMPTY_AREA(study));
  xml_save_string(nodes, "panel_layout", amitk_panel_layout_get_name(AMITK_STUDY_PANEL_LAYOUT(study)));

  return;
}

static gchar * study_read_xml(AmitkObject * object, xmlNodePtr nodes, 
			      FILE * study_file, gchar * error_buf) {

  AmitkStudy * study;
  gchar * creation_date;
  AmitkFuseType i_fuse_type;
  AmitkViewMode i_view_mode;
  gchar * temp_string;
  AmitkView i_view;
  AmitkLayout i_layout;
  AmitkPanelLayout i_panel_layout;
  gboolean all_false;

  error_buf = AMITK_OBJECT_CLASS(parent_class)->object_read_xml(object, nodes, study_file, error_buf);

  study = AMITK_STUDY(object);

  creation_date = xml_get_string(nodes, "creation_date");
  amitk_study_set_creation_date(study, creation_date);
  g_free(creation_date);

  /* get our view parameters */
  amitk_study_set_view_center(study, amitk_point_read_xml(nodes, "view_center", &error_buf));
  amitk_study_set_view_thickness(study, xml_get_real(nodes, "view_thickness", &error_buf));
  amitk_study_set_view_start_time(study, xml_get_time(nodes, "view_start_time", &error_buf));
  amitk_study_set_view_duration(study, xml_get_time(nodes, "view_duration", &error_buf));
  amitk_study_set_zoom(study, xml_get_real(nodes, "zoom", &error_buf));
  if (xml_node_exists(nodes, "fov"))
    amitk_study_set_fov(study, xml_get_real(nodes, "fov", &error_buf));
  amitk_study_set_canvas_target(study, xml_get_boolean(nodes, "canvas_target", &error_buf));

  /* preferences */
  amitk_study_set_canvas_roi_width(study, 
				   xml_get_int_with_default(nodes, "canvas_roi_width", 
							    AMITK_STUDY_CANVAS_ROI_WIDTH(study)));  

#ifdef AMIDE_LIBGNOMECANVAS_AA
  amitk_study_set_canvas_roi_transparency(study,
					  xml_get_real_with_default(nodes, "canvas_roi_transparency",
								    AMITK_STUDY_CANVAS_ROI_TRANSPARENCY(study)));
#else
  amitk_study_set_canvas_line_style(study, 
				    xml_get_int_with_default(nodes, "canvas_line_style",
							     AMITK_STUDY_CANVAS_LINE_STYLE(study)));  
  if (xml_node_exists(nodes, "canvas_fill_isocontour")) /* legacy */
    amitk_study_set_canvas_fill_roi(study, 
				    xml_get_int_with_default(nodes, "canvas_fill_isocontour",
							     AMITK_STUDY_CANVAS_FILL_ROI(study)));  
  else
    amitk_study_set_canvas_fill_roi(study, 
				    xml_get_int_with_default(nodes, "canvas_fill_roi",
							     AMITK_STUDY_CANVAS_FILL_ROI(study)));  
#endif

  amitk_study_set_canvas_maintain_size(study, 
				       xml_get_boolean_with_default(nodes, "canvas_maintain_size",
								AMITK_STUDY_CANVAS_MAINTAIN_SIZE(study)));  
  amitk_study_set_canvas_target_empty_area(study,
					   xml_get_int_with_default(nodes, "canvas_target_empty_area",
								    AMITK_STUDY_CANVAS_TARGET_EMPTY_AREA(study)));

  /* get layout */
  temp_string = xml_get_string(nodes, "canvas_layout");
  if (temp_string != NULL) {
    for (i_layout=0; i_layout < AMITK_LAYOUT_NUM; i_layout++)
      if (g_ascii_strcasecmp(temp_string, amitk_layout_get_name(i_layout)) == 0)
	amitk_study_set_canvas_layout(study, i_layout);
    g_free(temp_string);
  }

  /* get panel layout */
  temp_string = xml_get_string(nodes, "panel_layout");
  if (temp_string != NULL) {
    for (i_panel_layout=0; i_panel_layout < AMITK_PANEL_LAYOUT_NUM; i_panel_layout++)
      if (g_ascii_strcasecmp(temp_string, amitk_panel_layout_get_name(i_panel_layout)) == 0)
	amitk_study_set_panel_layout(study, i_panel_layout);
    g_free(temp_string);
  }

  /* sanity check */
  if (AMITK_STUDY_ZOOM(study) < EPSILON) {
    amitk_append_str_with_newline(&error_buf,_("inappropriate zoom (%5.3f) for study, reseting to 1.0"),
				  AMITK_STUDY_ZOOM(study));
    amitk_study_set_zoom(study, 1.0);
  }

  /* figure out the fuse type */
  temp_string = xml_get_string(nodes, "fuse_type");
  if (temp_string != NULL) {
    for (i_fuse_type=0; i_fuse_type < AMITK_FUSE_TYPE_NUM; i_fuse_type++) 
      if (g_ascii_strcasecmp(temp_string, amitk_fuse_type_get_name(i_fuse_type)) == 0)
	amitk_study_set_fuse_type(study, i_fuse_type);
    g_free(temp_string);
  }

  /* figure out the view mode */
  temp_string = xml_get_string(nodes, "view_mode");
  if (temp_string != NULL) {
    for (i_view_mode=0; i_view_mode < AMITK_VIEW_MODE_NUM; i_view_mode++) 
      if (g_ascii_strcasecmp(temp_string, amitk_view_mode_get_name(i_view_mode)) == 0)
	amitk_study_set_view_mode(study, i_view_mode);
    g_free(temp_string);
  }

  /* figure out which canvases are visible */
  for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view++) {
    temp_string = g_strdup_printf("%s_canvas_visible", amitk_view_get_name(i_view));
    study->canvas_visible[i_view] = xml_get_boolean(nodes, temp_string, &error_buf);
    g_free(temp_string);
  }
  


  /* ---- legacy cruft ---- */
  
  /* canvas_"view"_visible added previous to amide 0.7.7, set all to TRUE if none set */
  all_false = TRUE;
  for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view++) {
    all_false = all_false && !AMITK_STUDY_CANVAS_VISIBLE(study, i_view);
  }
  if (all_false)
    for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view++) study->canvas_visible[i_view] = TRUE;
  
  return error_buf;
}



static void set_voxel_dim_and_zoom(AmitkStudy * study, amide_real_t voxel_dim, amide_real_t zoom) {

  study->zoom = zoom;
  study->voxel_dim = voxel_dim;
  study->voxel_dim_valid=TRUE;
  g_signal_emit(G_OBJECT(study), study_signals[VOXEL_DIM_OR_ZOOM_CHANGED], 0);
}

static void study_recalc_voxel_dim(AmitkStudy * study) {

  amide_real_t voxel_dim;
  amide_real_t zoom;

  if (AMITK_OBJECT_CHILDREN(study) == NULL)
    return; /* no children, no voxel dim to recalculate */
  voxel_dim = amitk_data_sets_get_max_min_voxel_size(AMITK_OBJECT_CHILDREN(study));

  if (AMITK_STUDY_VOXEL_DIM_VALID(study) && !REAL_EQUAL(voxel_dim,AMITK_STUDY_VOXEL_DIM(study))) {
    /* update the zoom conservatively, so the canvas pixel size stays equal or gets smaller */
    if (voxel_dim < AMITK_STUDY_VOXEL_DIM(study))
      zoom = AMITK_STUDY_ZOOM(study)*voxel_dim/AMITK_STUDY_VOXEL_DIM(study);
    else
      zoom = AMITK_STUDY_ZOOM(study);
    set_voxel_dim_and_zoom(study, voxel_dim, zoom);
  } else {
    set_voxel_dim_and_zoom(study, voxel_dim,AMITK_STUDY_ZOOM(study));
  }


  return;
}

static void study_add_child(AmitkObject *object, AmitkObject *child) {

  if (AMITK_IS_DATA_SET(child)) {
    /* if this is for the first dataset added, get a reasonable view center */
    if (!amitk_objects_has_type(AMITK_OBJECT_CHILDREN(object), AMITK_OBJECT_TYPE_DATA_SET, TRUE)) 
      amitk_study_set_view_center(AMITK_STUDY(object), amitk_volume_get_center(AMITK_VOLUME(child)));
  }

  /* run the parent class function so added child is in the object's child list */
  AMITK_OBJECT_CLASS(parent_class)->object_add_child (object, child);

  /* keep the minimum voxel dimension accurate */
  if (AMITK_IS_DATA_SET(child)) {
    study_recalc_voxel_dim(AMITK_STUDY(object));
    g_signal_connect_swapped(G_OBJECT(child), "voxel_size_changed", G_CALLBACK(study_recalc_voxel_dim), object);

  }


  return;
}

static void study_remove_child(AmitkObject *object, AmitkObject *child) {

  gboolean data_set = FALSE;

  if (AMITK_IS_DATA_SET(child)) {
    data_set = TRUE; /* need to know now, child might get unrefed before recalc */
    g_signal_handlers_disconnect_by_func(G_OBJECT(child), G_CALLBACK(study_recalc_voxel_dim), object);
  }

  AMITK_OBJECT_CLASS(parent_class)->object_remove_child (object, child);

  if (data_set) 
    study_recalc_voxel_dim(AMITK_STUDY(object));

  return;
}



AmitkStudy * amitk_study_new (AmitkPreferences * preferences) {

  AmitkStudy * study;

  study = g_object_new(amitk_study_get_type(), NULL);
  amitk_object_set_name(AMITK_OBJECT(study), blank_name);

  if (preferences != NULL) {
    study->canvas_roi_width = AMITK_PREFERENCES_CANVAS_ROI_WIDTH(preferences);
#ifdef AMIDE_LIBGNOMECANVAS_AA
    study->canvas_roi_transparency = AMITK_PREFERENCES_CANVAS_ROI_TRANSPARENCY(preferences);
#else
    study->canvas_line_style = AMITK_PREFERENCES_CANVAS_LINE_STYLE(preferences);
    study->canvas_fill_roi = AMITK_PREFERENCES_CANVAS_FILL_ROI(preferences);
#endif
    study->canvas_layout = AMITK_PREFERENCES_CANVAS_LAYOUT(preferences);
    study->canvas_maintain_size = AMITK_PREFERENCES_CANVAS_MAINTAIN_SIZE(preferences);
    study->canvas_target_empty_area = AMITK_PREFERENCES_CANVAS_TARGET_EMPTY_AREA(preferences);
    study->panel_layout = AMITK_PREFERENCES_PANEL_LAYOUT(preferences);
  }

  return study;
}


/* sets the filename of a study */
void amitk_study_set_filename(AmitkStudy * study, const gchar * new_filename) {

  gchar * temp_string;

  g_return_if_fail(AMITK_IS_STUDY(study));

  g_free(study->filename); /* free up the memory used by the old filename */
  study->filename = NULL;

  if (new_filename == NULL) return;

  temp_string = g_strdup(new_filename); /* and assign the new name */
  
  /* and remove any trailing slashes */
  g_strreverse(temp_string);
  if (strchr(temp_string, '/') == temp_string) {
    study->filename = g_strdup(temp_string+1);
    g_free(temp_string);
  } else {
    study->filename=temp_string;
  }
  g_strreverse(study->filename);
  g_signal_emit(G_OBJECT(study), study_signals[FILENAME_CHANGED], 0);

  return;
}

void amitk_study_suggest_name(AmitkStudy * study, const gchar * suggested_name) {

  g_return_if_fail(AMITK_IS_STUDY(study));

  if (suggested_name == 0)
    return;
  if (strlen(suggested_name) == 0)
    return;

  if ((AMITK_OBJECT_NAME(study) == NULL)  ||
      (g_strcmp0(AMITK_OBJECT_NAME(study), "") == 0) ||
      (g_ascii_strncasecmp(blank_name, AMITK_OBJECT_NAME(study),
			   strlen(AMITK_OBJECT_NAME(study))) == 0)) 
    amitk_object_set_name(AMITK_OBJECT(study), suggested_name);

  return;
}


/* sets the date of a study
   note: no error checking of the date is currently done. */
void amitk_study_set_creation_date(AmitkStudy * study, const gchar * new_date) {

  g_return_if_fail(AMITK_IS_STUDY(study));

  g_free(study->creation_date); /* free up the memory used by the old creation date */
  study->creation_date = g_strdup(new_date); /* and assign the new name */

  return;
}

/* new_center should be in the base coordinate space */
void amitk_study_set_view_center(AmitkStudy * study, const AmitkPoint new_center) {

  AmitkPoint temp;
  g_return_if_fail(AMITK_IS_STUDY(study));

  temp = amitk_space_b2s(AMITK_SPACE(study), new_center);

  if (!POINT_EQUAL(study->view_center, temp)) {
    study->view_center = temp;
    g_signal_emit(G_OBJECT(study), study_signals[VIEW_CENTER_CHANGED], 0);
  }

  return;
}



/* refuses bad choices for thickness */
void amitk_study_set_view_thickness(AmitkStudy * study, const amide_real_t new_thickness) {
  amide_real_t min_voxel_size;
  amide_real_t temp;
  g_return_if_fail(AMITK_IS_STUDY(study));

  min_voxel_size = amitk_data_sets_get_min_voxel_size(AMITK_OBJECT_CHILDREN(study));

  if (min_voxel_size < 0)
    temp = new_thickness;
  else if (new_thickness < min_voxel_size)
    temp = min_voxel_size;
  else
    temp = new_thickness;

  if (!REAL_EQUAL(study->view_thickness, temp)) {
    study->view_thickness = temp;
    g_signal_emit(G_OBJECT(study), study_signals[THICKNESS_CHANGED], 0);
  }

  return;
}

void amitk_study_set_view_start_time (AmitkStudy * study, const amide_time_t new_start) {

  g_return_if_fail(AMITK_IS_STUDY(study));

  if (!REAL_EQUAL(study->view_start_time, new_start)) {
    study->view_start_time = new_start;
    g_signal_emit(G_OBJECT(study), study_signals[TIME_CHANGED], 0);
  }

  return;
}

void amitk_study_set_view_duration (AmitkStudy * study, const amide_time_t new_duration) {

  g_return_if_fail(AMITK_IS_STUDY(study));

  if (!REAL_EQUAL(study->view_duration, new_duration)) {
    study->view_duration = new_duration;
    g_signal_emit(G_OBJECT(study), study_signals[TIME_CHANGED], 0);
  }

  return;
}


void amitk_study_set_fuse_type(AmitkStudy * study, const AmitkFuseType new_fuse_type) {

  g_return_if_fail(AMITK_IS_STUDY(study));

  if (study->fuse_type != new_fuse_type) {
    study->fuse_type = new_fuse_type;
    g_signal_emit(G_OBJECT(study), study_signals[FUSE_TYPE_CHANGED], 0);
  }

  return;
}

void amitk_study_set_view_mode(AmitkStudy * study, const AmitkViewMode new_view_mode) {

  g_return_if_fail(AMITK_IS_STUDY(study));

  if (study->view_mode != new_view_mode) {
    study->view_mode = new_view_mode;
    g_signal_emit(G_OBJECT(study), study_signals[VIEW_MODE_CHANGED], 0);
  }

  return;
}

/* note, at least one canvas has to remain visible at all times, function won't 
   set visible to FALSE for a canvas if this condition will not be met */
void amitk_study_set_canvas_visible(AmitkStudy * study, const AmitkView view, const gboolean visible) {

  AmitkView i_view;
  gboolean one_visible;

  g_return_if_fail(AMITK_IS_STUDY(study));

  /* make sure one canvas will remain visible */
  one_visible = FALSE;
  for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view ++) {
    if (i_view == view)
      one_visible = one_visible || visible;
    else
      one_visible = one_visible || AMITK_STUDY_CANVAS_VISIBLE(study, i_view);
  }
  if (!one_visible) return; 

  if (AMITK_STUDY_CANVAS_VISIBLE(study, view) != visible) {
    study->canvas_visible[view] = visible;
    g_signal_emit(G_OBJECT(study), study_signals[CANVAS_VISIBLE_CHANGED], 0);
  }

  return;
}

void amitk_study_set_zoom(AmitkStudy * study, const amide_real_t new_zoom) {

  g_return_if_fail(AMITK_IS_STUDY(study));

  if (!REAL_EQUAL(AMITK_STUDY_ZOOM(study), new_zoom)) 
    set_voxel_dim_and_zoom(study, AMITK_STUDY_VOXEL_DIM(study), new_zoom);

  return;

}

void amitk_study_set_fov(AmitkStudy * study, const amide_real_t new_fov) {

  g_return_if_fail(AMITK_IS_STUDY(study));

  if (!REAL_EQUAL(AMITK_STUDY_FOV(study), new_fov)) {
    study->fov = new_fov;
    g_signal_emit(G_OBJECT(study), study_signals[FOV_CHANGED],0);
  }

  return;

}

void amitk_study_set_canvas_target(AmitkStudy * study, const gboolean always_on) {

  g_return_if_fail(AMITK_IS_STUDY(study));
  
  if (AMITK_STUDY_CANVAS_TARGET(study) != always_on) {
    study->canvas_target = always_on;
    g_signal_emit(G_OBJECT(study), study_signals[CANVAS_TARGET_CHANGED], 0);
  }

  return;

}

void amitk_study_set_canvas_roi_width(AmitkStudy * study, gint roi_width) {

  g_return_if_fail(AMITK_IS_STUDY(study));

  if (roi_width < AMITK_PREFERENCES_MIN_ROI_WIDTH) 
    roi_width = AMITK_PREFERENCES_MIN_ROI_WIDTH;
  if (roi_width > AMITK_PREFERENCES_MAX_ROI_WIDTH) 
    roi_width = AMITK_PREFERENCES_MAX_ROI_WIDTH;

  if (AMITK_STUDY_CANVAS_ROI_WIDTH(study) != roi_width) {
    study->canvas_roi_width = roi_width;
    g_signal_emit(G_OBJECT(study), study_signals[CANVAS_ROI_PREFERENCE_CHANGED], 0);
  }

  return;
}

#ifdef AMIDE_LIBGNOMECANVAS_AA
void amitk_study_set_canvas_roi_transparency(AmitkStudy * study, const gdouble transparency) {

  g_return_if_fail(AMITK_IS_STUDY(study));
  g_return_if_fail(transparency >= 0.0);
  g_return_if_fail(transparency <= 1.0);

  if (AMITK_STUDY_CANVAS_ROI_TRANSPARENCY(study) != transparency) {
    study->canvas_roi_transparency = transparency;
    g_signal_emit(G_OBJECT(study), study_signals[CANVAS_ROI_PREFERENCE_CHANGED], 0);
  }

  return;
}


#else
void amitk_study_set_canvas_line_style(AmitkStudy * study, const GdkLineStyle line_style){

  g_return_if_fail(AMITK_IS_STUDY(study));

  if (AMITK_STUDY_CANVAS_LINE_STYLE(study) != line_style) {
    study->canvas_line_style = line_style;
    g_signal_emit(G_OBJECT(study), study_signals[CANVAS_ROI_PREFERENCE_CHANGED], 0);
  }

  return;
}

void amitk_study_set_canvas_fill_roi(AmitkStudy * study, const gboolean fill_roi){

  g_return_if_fail(AMITK_IS_STUDY(study));

  if (AMITK_STUDY_CANVAS_FILL_ROI(study) != fill_roi) {
    study->canvas_fill_roi = fill_roi;
    g_signal_emit(G_OBJECT(study), study_signals[CANVAS_ROI_PREFERENCE_CHANGED], 0);
  }

  return;
}
#endif

void amitk_study_set_canvas_layout(AmitkStudy * study, const AmitkLayout layout) {

  g_return_if_fail(AMITK_IS_STUDY(study));

  if (AMITK_STUDY_CANVAS_LAYOUT(study) != layout) {
    study->canvas_layout = layout;
    g_signal_emit(G_OBJECT(study), study_signals[CANVAS_LAYOUT_PREFERENCE_CHANGED], 0);
  }

  return;
}

void amitk_study_set_canvas_maintain_size(AmitkStudy * study, const gboolean maintain_size){

  g_return_if_fail(AMITK_IS_STUDY(study));

  if (AMITK_STUDY_CANVAS_MAINTAIN_SIZE(study) != maintain_size) {
    study->canvas_maintain_size = maintain_size;
    g_signal_emit(G_OBJECT(study), study_signals[CANVAS_GENERAL_PREFERENCE_CHANGED], 0);
  }

  return;
}

void amitk_study_set_canvas_target_empty_area(AmitkStudy * study, gint target_empty_area){

  g_return_if_fail(AMITK_IS_STUDY(study));

  /* sanity checks */
  if (target_empty_area < AMITK_PREFERENCES_MIN_TARGET_EMPTY_AREA) 
    target_empty_area = AMITK_PREFERENCES_MIN_TARGET_EMPTY_AREA;
  if (target_empty_area > AMITK_PREFERENCES_MAX_TARGET_EMPTY_AREA) 
    target_empty_area = AMITK_PREFERENCES_MAX_TARGET_EMPTY_AREA;

  if (AMITK_STUDY_CANVAS_TARGET_EMPTY_AREA(study) != target_empty_area) {
    study->canvas_target_empty_area = target_empty_area;
    g_signal_emit(G_OBJECT(study), study_signals[CANVAS_TARGET_PREFERENCE_CHANGED], 0);
  }

  return;
}

void amitk_study_set_panel_layout(AmitkStudy * study, const AmitkPanelLayout panel_layout) {

  g_return_if_fail(AMITK_IS_STUDY(study));

  if (AMITK_STUDY_PANEL_LAYOUT(study) != panel_layout) {
    study->panel_layout = panel_layout;
    g_signal_emit(G_OBJECT(study), study_signals[PANEL_LAYOUT_PREFERENCE_CHANGED], 0);
  }

  return;
}


/* try to recover a corrupted file */
AmitkStudy * amitk_study_recover_xml(const gchar * study_filename, AmitkPreferences * preferences) {

  AmitkStudy * study = NULL;
  AmitkObject * recovered_object;
  gchar * error_buf=NULL;
  FILE * study_file=NULL;
  guint64 size, counter, start_location=0, end_location=0;
  gint returned_char;
  gboolean have_start_location;

  if (amitk_is_xif_directory(study_filename, NULL, NULL)) {
    g_warning("Recover function only works with XIF flat files, not XIF directories");
    return NULL;
  }

  /* open her on up */
  if ((study_file = fopen(study_filename, "rb")) == NULL) {
    g_warning(_("Couldn't open file %s\n"), study_filename);
    return NULL;
  }

  have_start_location = FALSE;
  counter = 0;
  while ((returned_char = fgetc(study_file)) != EOF) {

    /* find the start of the amide xml object */
    if (!have_start_location) {
      if (returned_char == amide_data_file_xml_start_tag[counter]) {
	if (counter == 0) start_location = ftell(study_file)-1;
	counter++;
	
	if (counter == strlen(amide_data_file_xml_start_tag)) {
	  /* we've got the start of an object */
	  counter = 0;
	  have_start_location = TRUE;
	}
      } else {
	counter = 0;
      }

      /* work on finding the end of the amide xml object */
    } else {
      if (returned_char == amide_data_file_xml_end_tag[counter]) {
	counter++;
	if (counter == strlen(amide_data_file_xml_end_tag)) {
	  counter = 0;

	  /* back the start location to include the xml tag*/
	  start_location -= strlen(amide_data_file_xml_tag)+3;

	  /* read in object */
	  end_location = ftell(study_file);
	  size = end_location-start_location+1;

	  recovered_object = amitk_object_read_xml(NULL, study_file, start_location, size, &error_buf);
	  if (recovered_object != NULL) {
	    if (!AMITK_IS_STUDY(recovered_object)) {
	      if (study == NULL) study = amitk_study_new(preferences);
	      amitk_object_add_child(AMITK_OBJECT(study), recovered_object);
	    }
	    amitk_object_unref(recovered_object);
	  }

	  /* return to the current spot in the file */
	  fseek(study_file, end_location,SEEK_SET);
	  have_start_location = FALSE;

	}
      } else {
	counter = 0;
      }
    }

  }

  /* display accumulated warning messages */
  if (error_buf != NULL) {
    amitk_append_str_with_newline(&error_buf, "");
    amitk_append_str_with_newline(&error_buf, _("The above warnings may arise because portions of the XIF"));
    amitk_append_str_with_newline(&error_buf, _("file were corrupted."));
    g_warning("%s",error_buf);
    g_free(error_buf);
  }

  /* remember the name of the file/directory for convience */
  if (study != NULL)
    amitk_study_set_filename(study, study_filename);

  return study;
}

/* function to load in a study from disk in xml format */
AmitkStudy * amitk_study_load_xml(const gchar * study_filename) {

  AmitkStudy * study;
  gchar * old_dir=NULL;
  gchar * load_filename=NULL;
  gchar * error_buf=NULL;
  gboolean xif_directory=FALSE;
  gboolean legacy1=FALSE;
  FILE * study_file=NULL;
  guint64 location_le, size_le;
  guint64 location, size;


  /* are we dealing with a xif directory */
  if (amitk_is_xif_directory(study_filename, &legacy1, &load_filename)) {
    xif_directory=TRUE;

    /* switch into our new directory */
    old_dir = g_get_current_dir();
    if (chdir(study_filename) != 0) {
      g_warning(_("Couldn't change directories in loading study"));
      return NULL;
    }

  } else if (amitk_is_xif_flat_file(study_filename, &location_le, &size_le)){ /* flat file */
    if ((study_file = fopen(study_filename, "rb")) == NULL) {
      g_warning(_("Couldn't open file %s\n"), study_filename);
      return NULL;
    }
  } else {
    g_warning("Not a XIF study file: %s\n", study_filename);
    return NULL;
  }
  location = GUINT64_FROM_LE(location_le);
  size = GUINT64_FROM_LE(size_le);

  /* sanity check */
  if ((((location == 0) || (size == 0))) && !xif_directory) {
    g_warning(_("File is corrupt.  The file may have been incompletely saved.  You can attempt recovering the file by using the recover function under the file menu."));
    return NULL;
  }

  /* load in the study */
  if (legacy1)
    study = legacy_load_xml(&error_buf);
  else 
    study = AMITK_STUDY(amitk_object_read_xml(load_filename, study_file, location, size, &error_buf));


  if (load_filename != NULL) g_free(load_filename);
  if (study_file != NULL) fclose(study_file);

  /* display accumulated warning messages */
  if (error_buf != NULL) {
    amitk_append_str_with_newline(&error_buf, "");
    amitk_append_str_with_newline(&error_buf, _("The above warnings most likely indicate changes to the"));
    amitk_append_str_with_newline(&error_buf, _("XIF file format, please resave the data as soon as possible."));
    g_warning("%s",error_buf);
    g_free(error_buf);
  }

  /* remember the name of the file/directory for convience */
  if (study != NULL)
    amitk_study_set_filename(study, study_filename);
    
  if (xif_directory) {
    /* and return to the old directory */
    if (chdir(old_dir) != 0) {
      g_warning(_("Couldn't return to previous directory in load study"));
      if (study != NULL) study = amitk_object_unref(study);
    }
    
    g_free(old_dir);
  }

  return study;
}




/* function to writeout the study to disk in an xif file */
gboolean amitk_study_save_xml(AmitkStudy * study, const gchar * study_filename,
			      gboolean save_as_directory) {

  gchar * old_dir=NULL;
  struct stat file_info;
  gchar * temp_string;
  DIR * directory;
  struct dirent * directory_entry;
  guint64 location, size;
  guint64 location_le, size_le;
  FILE * study_file=NULL;

  /* see if the filename already exists, remove stuff if needed */
  if (stat(study_filename, &file_info) == 0) {

    /* and start deleting everything in the filename/directory */
    if (S_ISDIR(file_info.st_mode)) {
      directory = opendir(study_filename);
      while ((directory_entry = readdir(directory)) != NULL) {
	temp_string = 
	  g_strdup_printf("%s%s%s", study_filename,G_DIR_SEPARATOR_S, directory_entry->d_name);

	if ((g_pattern_match_simple("*.xml",directory_entry->d_name)) ||
	    (g_pattern_match_simple("*.dat",directory_entry->d_name)))
	  if (unlink(temp_string) != 0)
	    g_warning(_("Couldn't unlink file: %s"),temp_string);

	g_free(temp_string);
      }
      if (!save_as_directory) /* get rid of the directory too if we're overwriting with a file*/
	if (rmdir(study_filename) != 0) {
	  g_warning(_("Couldn't remove directory: %s"), study_filename);
	  return FALSE;
	}

    } else if (S_ISREG(file_info.st_mode)) {
      if (unlink(study_filename) != 0) {
	g_warning(_("Couldn't unlink file: %s"),study_filename);
	return FALSE;
      }

    } else {
      g_warning(_("Unrecognized file type for file: %s, couldn't delete"),study_filename);
      return FALSE;
    }
  } 

  if (save_as_directory) {
    if (stat(study_filename, &file_info) != 0) {
      /* make the directory if needed */
      if (g_mkdir(study_filename, 0766) != 0) 
	{
	  g_warning(_("Couldn't create amide directory: %s"),study_filename);
	  return FALSE;
	}
    }
  }

  /* remember the name of the xif file/directory of this study */
  amitk_study_set_filename(study, study_filename);


  /* get into the output directory/create output file */
  if (save_as_directory) {
    old_dir = g_get_current_dir();
    if (chdir(study_filename) != 0) {
      g_warning(_("Couldn't change directories in writing study, study not saved"));
      return FALSE;
    }
  } else { /* flat file */
    if ((study_file = fopen(study_filename, "wb")) == NULL) {
      g_warning(_("Couldn't open file %s\n"), study_filename);
      return FALSE;
    }
    fprintf(study_file, "%s Version %s", 
	    AMITK_FLAT_FILE_MAGIC_STRING,
	    AMITK_FILE_VERSION);
    fseek(study_file, 64+2*sizeof(guint64), SEEK_SET);
  }

  /* save the study */
  amitk_object_write_xml(AMITK_OBJECT(study), study_file, NULL, &location, &size);

  if (save_as_directory) {
    if (chdir(old_dir) != 0) {
      g_warning(_("Couldn't return to previous directory in load study"));
      study = amitk_object_unref(study);
    }
  } else { /* flat file */
    /* record location of study object xml, always little endian */
    fseek(study_file, 64, SEEK_SET);
    location_le = GUINT64_TO_LE(location);
    size_le = GUINT64_TO_LE(size);
    fwrite(&location_le, 1, sizeof(guint64), study_file);
    fwrite(&size_le, 1, sizeof(guint64), study_file);
    fclose(study_file);
  }

  return TRUE;
}


const gchar * amitk_fuse_type_get_name(const AmitkFuseType fuse_type) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_FUSE_TYPE);
  enum_value = g_enum_get_value(enum_class, fuse_type);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

const gchar * amitk_view_mode_get_name(const AmitkViewMode view_mode) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_VIEW_MODE);
  enum_value = g_enum_get_value(enum_class, view_mode);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}
