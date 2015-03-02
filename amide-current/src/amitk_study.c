/* amitk_study.c
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

#include <time.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "amitk_study.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"
#include "legacy.h"


/* external variables */
gchar * amitk_fuse_type_explanations[] = {
  "blend all data sets",
  "overlay active data set on blended data sets"
};

gchar * view_mode_names[] = {
  "single view",
  "linked view, 2 way",
  "linked view, 3 way"
};

gchar * view_mode_explanations[] = {
  "All objects are shown in a single view",
  "Objects are shown between 2 linked views",
  "Objects are shown between 3 linked views",
};




enum {
  STUDY_CHANGED,
  THICKNESS_CHANGED,
  TIME_CHANGED,
  CANVAS_VISIBLE_CHANGED,
  VIEW_MODE_CHANGED,
  LAST_SIGNAL
};


static void          study_class_init          (AmitkStudyClass     *klass);
static void          study_init                (AmitkStudy          *study);
static void          study_finalize            (GObject             *object);
static AmitkObject * study_copy                (const AmitkObject   *object);
static void          study_copy_in_place       (AmitkObject * dest_object, const AmitkObject * src_object);
static void          study_write_xml           (const AmitkObject   *object, 
						xmlNodePtr           nodes);
static gchar *       study_read_xml            (AmitkObject         *object,
						xmlNodePtr           nodes,
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

  study_signals[STUDY_CHANGED] =
    g_signal_new ("study_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkStudyClass, study_changed),
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

}


static void study_init (AmitkStudy * study) {

  time_t current_time;
  AmitkView i_view;

  study->filename = NULL;

  /* view parameters */
  study->view_center = zero_point;
  study->view_thickness = 1.0;
  study->view_start_time = SMALL_TIME;
  study->view_duration = 1.0-SMALL_TIME;
  study->zoom = 1.0;
  study->fuse_type = AMITK_FUSE_TYPE_BLEND;
  study->view_mode = AMITK_VIEW_MODE_SINGLE;
  for (i_view=0; i_view<AMITK_VIEW_NUM; i_view++)
    study->canvas_visible[i_view] = TRUE;
  study->voxel_dim = 1.0;

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

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static AmitkObject * study_copy (const AmitkObject * object) {

  AmitkStudy * copy;

  g_return_val_if_fail(AMITK_IS_STUDY(object), NULL);

  copy = amitk_study_new();
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
  dest_study->fuse_type = AMITK_STUDY_FUSE_TYPE(src_object);
  dest_study->view_mode = AMITK_STUDY_VIEW_MODE(src_object);
  for (i_view=0; i_view<AMITK_VIEW_NUM; i_view++)
    dest_study->canvas_visible[i_view] = AMITK_STUDY_CANVAS_VISIBLE(src_object, i_view);

  /* make a separate copy in memory of the study's name and filename */
  amitk_study_set_filename(dest_study, AMITK_STUDY_FILENAME(src_object));

  AMITK_OBJECT_CLASS (parent_class)->object_copy_in_place (dest_object, src_object);
}



static void study_write_xml(const AmitkObject * object, xmlNodePtr nodes) {

  AmitkStudy * study;
  AmitkView i_view;
  gchar * temp_string;

  AMITK_OBJECT_CLASS(parent_class)->object_write_xml(object, nodes);

  study = AMITK_STUDY(object);

  xml_save_string(nodes, "creation_date", AMITK_STUDY_CREATION_DATE(study));
  amitk_point_write_xml(nodes, "view_center",  AMITK_STUDY_VIEW_CENTER(study));
  xml_save_real(nodes, "view_thickness", AMITK_STUDY_VIEW_THICKNESS(study));
  xml_save_time(nodes, "view_start_time", AMITK_STUDY_VIEW_START_TIME(study));
  xml_save_time(nodes, "view_duration", AMITK_STUDY_VIEW_DURATION(study));
  xml_save_string(nodes, "fuse_type",
		  amitk_fuse_type_get_name(AMITK_STUDY_FUSE_TYPE(study)));
  xml_save_string(nodes, "view_mode",
		  amitk_view_mode_get_name(AMITK_STUDY_VIEW_MODE(study)));
  for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view++) {
    temp_string = g_strdup_printf("%s_canvas_visible", amitk_view_get_name(i_view));
    xml_save_boolean(nodes, temp_string, AMITK_STUDY_CANVAS_VISIBLE(study, i_view));
    g_free(temp_string);
  }
  xml_save_real(nodes, "zoom", AMITK_STUDY_ZOOM(study));

  return;
}

static gchar * study_read_xml(AmitkObject * object, xmlNodePtr nodes, gchar * error_buf) {

  AmitkStudy * study;
  gchar * creation_date;
  AmitkFuseType i_fuse_type;
  AmitkViewMode i_view_mode;
  gchar * temp_string;
  AmitkView i_view;
  gboolean all_false;

  error_buf = AMITK_OBJECT_CLASS(parent_class)->object_read_xml(object, nodes, error_buf);

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
 
  /* sanity check */
  if (AMITK_STUDY_ZOOM(study) < EPSILON) {
    amitk_append_str(&error_buf,"inappropriate zoom (%5.3f) for study, reseting to 1.0",AMITK_STUDY_ZOOM(study));
    amitk_study_set_zoom(study, 1.0);
  }

  /* figure out the fuse type */
  temp_string = xml_get_string(nodes, "fuse_type");
  if (temp_string != NULL)
    for (i_fuse_type=0; i_fuse_type < AMITK_FUSE_TYPE_NUM; i_fuse_type++) 
      if (g_strcasecmp(temp_string, amitk_fuse_type_get_name(i_fuse_type)) == 0)
	amitk_study_set_fuse_type(study, i_fuse_type);
  g_free(temp_string);

  /* figure out the view mode */
  temp_string = xml_get_string(nodes, "view_mode");
  if (temp_string != NULL)
    for (i_view_mode=0; i_view_mode < AMITK_VIEW_MODE_NUM; i_view_mode++) 
      if (g_strcasecmp(temp_string, amitk_view_mode_get_name(i_view_mode)) == 0)
	amitk_study_set_view_mode(study, i_view_mode);
  g_free(temp_string);

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


static void study_recalc_voxel_dim(AmitkStudy * study) {

  amide_real_t voxel_dim;

  voxel_dim = amitk_data_sets_get_max_min_voxel_size(AMITK_OBJECT_CHILDREN(study));
  if (!REAL_EQUAL(voxel_dim,AMITK_STUDY_VOXEL_DIM(study))) {
    study->voxel_dim = voxel_dim;
    g_signal_emit(G_OBJECT(study), study_signals[STUDY_CHANGED], 0);
  }

  return;
}

static void study_add_child(AmitkObject *object, AmitkObject *child) {

  AMITK_OBJECT_CLASS(parent_class)->object_add_child (object, child);

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



AmitkStudy * amitk_study_new (void) {

  AmitkStudy * study;

  study = g_object_new(amitk_study_get_type(), NULL);

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
    g_signal_emit(G_OBJECT(study), study_signals[STUDY_CHANGED], 0);
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
    g_signal_emit(G_OBJECT(study), study_signals[STUDY_CHANGED], 0);
    g_signal_emit(G_OBJECT(study), study_signals[THICKNESS_CHANGED], 0);
  }

  return;
}

void amitk_study_set_view_start_time (AmitkStudy * study, const amide_time_t new_start) {

  g_return_if_fail(AMITK_IS_STUDY(study));

  if (!REAL_EQUAL(study->view_start_time, new_start)) {
    study->view_start_time = new_start;
    g_signal_emit(G_OBJECT(study), study_signals[STUDY_CHANGED], 0);
    g_signal_emit(G_OBJECT(study), study_signals[TIME_CHANGED], 0);
  }

  return;
}

void amitk_study_set_view_duration (AmitkStudy * study, const amide_time_t new_duration) {

  g_return_if_fail(AMITK_IS_STUDY(study));

  if (!REAL_EQUAL(study->view_duration, new_duration)) {
    study->view_duration = new_duration;
    g_signal_emit(G_OBJECT(study), study_signals[STUDY_CHANGED], 0);
    g_signal_emit(G_OBJECT(study), study_signals[TIME_CHANGED], 0);
  }

  return;
}


void amitk_study_set_fuse_type(AmitkStudy * study, const AmitkFuseType new_fuse_type) {

  g_return_if_fail(AMITK_IS_STUDY(study));

  if (study->fuse_type != new_fuse_type) {
    study->fuse_type = new_fuse_type;
    g_signal_emit(G_OBJECT(study), study_signals[STUDY_CHANGED], 0);
  }

  return;
}

void amitk_study_set_view_mode(AmitkStudy * study, const AmitkViewMode new_view_mode) {

  g_return_if_fail(AMITK_IS_STUDY(study));

  if (study->view_mode != new_view_mode) {
    study->view_mode = new_view_mode;
    g_signal_emit(G_OBJECT(study), study_signals[STUDY_CHANGED], 0);
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

  if (!REAL_EQUAL(study->zoom, new_zoom)) {
    study->zoom = new_zoom;
    g_signal_emit(G_OBJECT(study), study_signals[STUDY_CHANGED], 0);
  }

  return;

}



/* function to load in a study from disk in xml format */
AmitkStudy * amitk_study_load_xml(const gchar * study_directory) {

  AmitkStudy * study;
  gchar * old_dir;
  gchar * cur_dir;
  gchar * xml_filename=NULL;
  struct stat file_info;
  DIR * directory;
  struct dirent * directory_entry;
  gchar * error_buf=NULL;
  
  
  /* switch into our new directory */
  old_dir = g_get_current_dir();
  if (chdir(study_directory) != 0) {
    g_warning("Couldn't change directories in loading study");
    return NULL;
  }

  /* check for legacy .xif file (< 2.0 version) */
  if (stat("Study.xml", &file_info) == 0) {
    study = legacy_load_xml(&error_buf);
  } else {

    /* figure out the name of the study file */
    cur_dir = g_get_current_dir();
    directory = opendir(cur_dir);
    g_free(cur_dir);
    
    /* currently, only looks at the first study_*.xml file... there should be only one anyway */
    while (((directory_entry = readdir(directory)) != NULL) && (xml_filename == NULL))
      if (g_pattern_match_simple("study_*.xml", directory_entry->d_name))
	xml_filename = g_strdup(directory_entry->d_name);
    
    closedir(directory);
    
    /* load in the study */
    study = AMITK_STUDY(amitk_object_read_xml(xml_filename, &error_buf));
    g_free(xml_filename);
  }

  /* display accumulated warning messages */
  if (error_buf != NULL) {
    g_warning(error_buf);
    g_free(error_buf);
  }

  /* remember the name of the directory for convience */
  amitk_study_set_filename(study, study_directory);

  /* and return to the old directory */
  if (chdir(old_dir) != 0) {
    g_warning("Couldn't return to previous directory in load study");
    g_object_unref(study);
    study = NULL;
  }

  g_free(old_dir);
  return study;
}




/* function to writeout the study to disk in an xif file */
gboolean amitk_study_save_xml(AmitkStudy * study, const gchar * study_directory) {

  gchar * old_dir;

  /* switch into our new directory */
  old_dir = g_get_current_dir();
  if (chdir(study_directory) != 0) {
    g_warning("Couldn't change directories in writing study, study not saved");
    return FALSE;
  }

  amitk_object_write_xml(AMITK_OBJECT(study));

  /* and return to the old directory */
  if (chdir(old_dir) != 0) {
    g_warning("Couldn't return to previous directory in writing study");
    return FALSE;
  }
  g_free(old_dir);

  /* remember the name of the directory of this study */
  amitk_study_set_filename(study, study_directory);

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
