/* amitk_roi.c
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

#include "amitk_roi.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"

/* variable type function declarations */
#include "amitk_roi_ELLIPSOID.h"
#include "amitk_roi_CYLINDER.h"
#include "amitk_roi_BOX.h"
#include "amitk_roi_ISOCONTOUR_2D.h"
#include "amitk_roi_ISOCONTOUR_3D.h"
#include "amitk_roi_FREEHAND_2D.h"
#include "amitk_roi_FREEHAND_3D.h"



gchar * amitk_roi_menu_names[] = {
  N_("_Ellipsoid"), 
  N_("Elliptic _Cylinder"), 
  N_("_Box"),
  N_("_2D Isocontour"),
  N_("_3D Isocontour"),
  N_("_2D Freehand"),
  N_("_3D Freehand")
};

gchar * amitk_roi_menu_explanation[] = {
  N_("Add a new elliptical ROI"), 
  N_("Add a new elliptic cylinder ROI"), 
  N_("Add a new box shaped ROI"),
  N_("Add a new 2D Isocontour ROI"),
  N_("Add a new 3D Isocontour ROI"),
  N_("Add a new 2D Freehand ROI"),
  N_("Add a new 3D Freehand ROI")
};


enum {
  ROI_CHANGED,
  ROI_TYPE_CHANGED,
  LAST_SIGNAL
};

static void          roi_class_init          (AmitkRoiClass *klass);
static void          roi_init                (AmitkRoi      *roi);
static void          roi_finalize            (GObject          *object);
static void          roi_scale               (AmitkSpace        *space,
					      AmitkPoint        *ref_point,
					      AmitkPoint        *scaling);
static AmitkObject * roi_copy                (const AmitkObject * object);
static void          roi_copy_in_place       (AmitkObject * dest_object, const AmitkObject * src_object);
static void          roi_write_xml           (const AmitkObject  *object, 
					      xmlNodePtr          nodes, 
					      FILE               *study_file);
static gchar *       roi_read_xml            (AmitkObject        *object, 
					      xmlNodePtr          nodes, 
					      FILE               *study_file, 
					      gchar              *error_buf);
static void          roi_get_center          (const AmitkVolume *volume,
					      AmitkPoint        *center);
static void          roi_set_voxel_size      (AmitkRoi * roi, 
					      AmitkPoint voxel_size);

static AmitkVolumeClass * parent_class;
static guint        roi_signals[LAST_SIGNAL];



GType amitk_roi_get_type(void) {

  static GType roi_type = 0;

  if (!roi_type)
    {
      static const GTypeInfo roi_info =
      {
	sizeof (AmitkRoiClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) roi_class_init,
	(GClassFinalizeFunc) NULL,
	NULL,		/* class_data */
	sizeof (AmitkRoi),
	0,			/* n_preallocs */
	(GInstanceInitFunc) roi_init,
	NULL /* value table */
      };
      
      roi_type = g_type_register_static (AMITK_TYPE_VOLUME, "AmitkRoi", &roi_info, 0);
    }

  return roi_type;
}


static void roi_class_init (AmitkRoiClass * class) {

  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  AmitkObjectClass * object_class = AMITK_OBJECT_CLASS(class);
  AmitkSpaceClass * space_class = AMITK_SPACE_CLASS(class);
  AmitkVolumeClass * volume_class = AMITK_VOLUME_CLASS(class);

  parent_class = g_type_class_peek_parent(class);

  space_class->space_scale = roi_scale;

  object_class->object_copy = roi_copy;
  object_class->object_copy_in_place = roi_copy_in_place;
  object_class->object_write_xml = roi_write_xml;
  object_class->object_read_xml = roi_read_xml;

  volume_class->volume_get_center = roi_get_center;

  gobject_class->finalize = roi_finalize;

  roi_signals[ROI_CHANGED] =
    g_signal_new ("roi_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkRoiClass, roi_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);
  roi_signals[ROI_TYPE_CHANGED] =
    g_signal_new ("roi_type_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkRoiClass, roi_type_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);

}


static void roi_init (AmitkRoi * roi) {

  roi->specify_color = FALSE;
  roi->color = amitk_color_table_uint32_to_rgba(AMITK_OBJECT_DEFAULT_COLOR);

  roi->voxel_size = zero_point;
  roi->map_data = NULL;
  roi->center_of_mass_calculated=FALSE;
  roi->center_of_mass=zero_point;

  roi->isocontour_min_value = 0.0;
  roi->isocontour_max_value = 0.0;
  roi->isocontour_range = AMITK_ROI_ISOCONTOUR_RANGE_ABOVE_MIN;
}


static void roi_finalize (GObject *object)
{
  AmitkRoi * roi = AMITK_ROI(object);

  if (roi->map_data != NULL) {
    g_object_unref(roi->map_data);
    roi->map_data = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void roi_scale(AmitkSpace *space, AmitkPoint *ref_point, AmitkPoint *scaling) {

  AmitkRoi * roi;
  AmitkPoint voxel_size;

  g_return_if_fail(AMITK_IS_ROI(space));
  roi = AMITK_ROI(space);

  /* first, pass the signal on, this gets the volume corner value readjusted */
  AMITK_SPACE_CLASS(parent_class)->space_scale (space, ref_point, scaling);

  /* and readjust the voxel size based on the new corner */
  if (AMITK_VOLUME_VALID(roi)) {
    if (AMITK_ROI_TYPE_ISOCONTOUR(roi) || AMITK_ROI_TYPE_FREEHAND(roi)) {

      voxel_size = AMITK_VOLUME_CORNER(roi);
      voxel_size.x /= roi->map_data->dim.x;
      voxel_size.y /= roi->map_data->dim.y;
      voxel_size.z /= roi->map_data->dim.z;
      roi_set_voxel_size(roi, voxel_size);
    }
  }
}


static AmitkObject * roi_copy (const AmitkObject * object) {

  AmitkRoi * copy;

  g_return_val_if_fail(AMITK_IS_ROI(object), NULL);

  copy = amitk_roi_new(AMITK_ROI_TYPE(object));
  amitk_object_copy_in_place(AMITK_OBJECT(copy), object);

  return AMITK_OBJECT(copy);
}

/* doesn't copy the data set used by isocontours and freehands, just adds a reference... */
static void roi_copy_in_place (AmitkObject * dest_object, const AmitkObject * src_object) {

  AmitkRoi * src_roi;
  AmitkRoi * dest_roi;

  g_return_if_fail(AMITK_IS_ROI(src_object));
  g_return_if_fail(AMITK_IS_ROI(dest_object));

  src_roi = AMITK_ROI(src_object);
  dest_roi = AMITK_ROI(dest_object);

  amitk_roi_set_type(dest_roi, AMITK_ROI_TYPE(src_object));

  dest_roi->specify_color = AMITK_ROI_SPECIFY_COLOR(src_roi);
  dest_roi->color = AMITK_ROI_COLOR(src_roi);

  if (src_roi->map_data != NULL) {
    if (dest_roi->map_data != NULL)
      g_object_unref(dest_roi->map_data);
    dest_roi->map_data = g_object_ref(src_roi->map_data);
  }

  dest_roi->center_of_mass_calculated = src_roi->center_of_mass_calculated;
  dest_roi->center_of_mass = src_roi->center_of_mass;
  roi_set_voxel_size(dest_roi, AMITK_ROI_VOXEL_SIZE(src_object));

  dest_roi->isocontour_min_value = AMITK_ROI_ISOCONTOUR_MIN_VALUE(src_object);
  dest_roi->isocontour_max_value = AMITK_ROI_ISOCONTOUR_MAX_VALUE(src_object);
  dest_roi->isocontour_range = AMITK_ROI_ISOCONTOUR_RANGE(src_object);

  AMITK_OBJECT_CLASS (parent_class)->object_copy_in_place (dest_object, src_object);
}


static void roi_write_xml (const AmitkObject * object, xmlNodePtr nodes, FILE * study_file) {

  AmitkRoi * roi;
  gchar * name;
  gchar * filename;
  guint64 location, size;

  AMITK_OBJECT_CLASS(parent_class)->object_write_xml(object, nodes, study_file);

  roi = AMITK_ROI(object);

  xml_save_string(nodes, "type", amitk_roi_type_get_name(AMITK_ROI_TYPE(roi)));

  xml_save_boolean(nodes, "specify_color", AMITK_ROI_SPECIFY_COLOR(roi));
  xml_save_uint(nodes, "color", amitk_color_table_rgba_to_uint32(AMITK_ROI_COLOR(roi)));

  /* freehand and isocontour specific stuff */
  amitk_point_write_xml(nodes, "voxel_size", AMITK_ROI_VOXEL_SIZE(roi));
  xml_save_boolean(nodes, "center_of_mass_calculated", AMITK_ROI(roi)->center_of_mass_calculated);
  amitk_point_write_xml(nodes, "center_of_mass", AMITK_ROI(roi)->center_of_mass);

  if (AMITK_ROI_TYPE_ISOCONTOUR(roi) || AMITK_ROI_TYPE_FREEHAND(roi)) {
    name = g_strdup_printf("roi_%s_map_data", AMITK_OBJECT_NAME(roi));
    amitk_raw_data_write_xml(roi->map_data, name, study_file, &filename, &location, &size);
    g_free(name);
    if (study_file == NULL) {
      xml_save_string(nodes,"map_file", filename);
      g_free(filename);
    } else {
      xml_save_location_and_size(nodes, "map_location_and_size", location, size);
    }
  }

  /* isocontour specific stuff */
  xml_save_real(nodes, "isocontour_min_value", AMITK_ROI_ISOCONTOUR_MIN_VALUE(roi));
  xml_save_real(nodes, "isocontour_max_value", AMITK_ROI_ISOCONTOUR_MAX_VALUE(roi));
  xml_save_int(nodes, "isocontour_range", AMITK_ROI_ISOCONTOUR_RANGE(roi));

  return;
}


static gchar * roi_read_xml (AmitkObject * object, xmlNodePtr nodes, FILE * study_file, gchar * error_buf) {

  AmitkRoi * roi;
  AmitkRoiType i_roi_type;
  gchar * temp_string;
  gchar * map_xml_filename=NULL;
  guint64 location, size;

  error_buf = AMITK_OBJECT_CLASS(parent_class)->object_read_xml(object, nodes, study_file, error_buf);

  roi = AMITK_ROI(object);

  /* figure out the type */
  temp_string = xml_get_string(nodes, "type");
  if (temp_string != NULL)
    for (i_roi_type=0; i_roi_type < AMITK_ROI_TYPE_NUM; i_roi_type++) 
      if (g_ascii_strcasecmp(temp_string, amitk_roi_type_get_name(i_roi_type)) == 0)
	roi->type = i_roi_type;
  g_free(temp_string);

  amitk_roi_set_specify_color(roi, xml_get_boolean_with_default(nodes, "specify_color",
								AMITK_ROI_SPECIFY_COLOR(roi)));
  amitk_roi_set_color(roi, amitk_color_table_uint32_to_rgba(xml_get_uint_with_default(nodes, "color", AMITK_OBJECT_DEFAULT_COLOR)));

  /* freehand and isocontour specific stuff */
  if (AMITK_ROI_TYPE_ISOCONTOUR(roi) | AMITK_ROI_TYPE_FREEHAND(roi)) {
    roi_set_voxel_size(roi, amitk_point_read_xml(nodes, "voxel_size", &error_buf));

    /* check for old style entries */
    if (xml_node_exists(nodes, "isocontour_center_of_mass_calculated"))
	roi->center_of_mass_calculated = xml_get_boolean(nodes, "isocontour_center_of_mass_calculated", &error_buf);
    else
	roi->center_of_mass_calculated = xml_get_boolean(nodes, "center_of_mass_calculated", &error_buf);

    /* check for old style entries */
    if (xml_node_exists(nodes, "isocontour_center_of_mass"))
      roi->center_of_mass = amitk_point_read_xml(nodes, "isocontour_center_of_mass", &error_buf);
    else
      roi->center_of_mass = amitk_point_read_xml(nodes, "center_of_mass", &error_buf);

    /* check for old style entries */
    if (xml_node_exists(nodes, "isocontour_file") || 
	xml_node_exists(nodes, "isocontour_location_and_size")) {
      if (study_file == NULL) 
	map_xml_filename = xml_get_string(nodes, "isocontour_file");
      else
	xml_get_location_and_size(nodes, "isocontour_location_and_size", &location, &size, &error_buf);
      roi->map_data = amitk_raw_data_read_xml(map_xml_filename, study_file, location, 
						size,&error_buf, NULL, NULL);

      /* if the ROI's never been drawn, it's possible for this not to exist */
    } else if (xml_node_exists(nodes, "map_file") || 
	       xml_node_exists(nodes, "map_location_and_size")) {

      if (study_file == NULL) 
	map_xml_filename = xml_get_string(nodes, "map_file");
      else
	xml_get_location_and_size(nodes, "map_location_and_size", &location, &size, &error_buf);
      roi->map_data = amitk_raw_data_read_xml(map_xml_filename, study_file, location, 
					      size,&error_buf, NULL, NULL);
    }

    if (map_xml_filename != NULL) g_free(map_xml_filename);
  }

  /* isocontour specific stuff */
  if (AMITK_ROI_TYPE_ISOCONTOUR(roi)) {
    /* check first if we're using the old isocontour_value and isocontour_inverse entries */
    if (xml_node_exists(nodes, "isocontour_value")) {
      /* works, cause inverse=TRUE=1=AMITK_ROI_ISOCONTOUR_RANGE_BELOW_MAX */
      roi->isocontour_range = xml_get_boolean(nodes, "isocontour_inverse", &error_buf);
      if (roi->isocontour_range == AMITK_ROI_ISOCONTOUR_RANGE_BELOW_MAX)
	roi->isocontour_min_value = xml_get_real(nodes, "isocontour_value", &error_buf);
      else
	roi->isocontour_max_value = xml_get_real(nodes, "isocontour_value", &error_buf);
    } else {
      /* how we currently store isocontour information */
      roi->isocontour_range = xml_get_int(nodes, "isocontour_range", &error_buf);
      roi->isocontour_min_value = xml_get_real(nodes, "isocontour_min_value", &error_buf);
      roi->isocontour_max_value = xml_get_real(nodes, "isocontour_max_value", &error_buf);
    }
  }

  /* make sure to mark the roi as undrawn if needed */
  if (AMITK_ROI_TYPE_ISOCONTOUR(roi)) {
    if (roi->map_data == NULL) 
      AMITK_VOLUME(roi)->valid = FALSE;
  } else {
    if (POINT_EQUAL(AMITK_VOLUME_CORNER(roi), zero_point)) {
      AMITK_VOLUME(roi)->valid = FALSE;
    }
  }

  return error_buf;
}


static void roi_get_center(const AmitkVolume *volume, AmitkPoint * pcenter) {

  if (AMITK_ROI_TYPE_ISOCONTOUR(volume) || AMITK_ROI_TYPE_FREEHAND(volume)) {
    *pcenter = amitk_roi_get_center_of_mass(AMITK_ROI(volume));

  } else {   /* if geometric, just use the standard volume function */
    AMITK_VOLUME_CLASS(parent_class)->volume_get_center(volume,pcenter);
  }

  return;
}


AmitkRoi * amitk_roi_new (AmitkRoiType type) {

  AmitkRoi * roi;

  roi = g_object_new(amitk_roi_get_type(), NULL);
  roi->type = type;

  return roi;
}





/* returns a singly linked list of intersection points between the roi
   and the given canvas slice.  returned points are in the canvas's coordinate space.
   note: use this function for ELLIPSOID, CYLINDER, and BOX 
*/
GSList * amitk_roi_get_intersection_line(const AmitkRoi * roi, 
					 const AmitkVolume * canvas_slice,
					 const amide_real_t pixel_dim) {
  GSList * return_points = NULL;

  g_return_val_if_fail(AMITK_IS_ROI(roi), NULL);

  if (AMITK_ROI_UNDRAWN(roi)) return NULL;

  switch(AMITK_ROI_TYPE(roi)) {
  case AMITK_ROI_TYPE_ELLIPSOID:
    return_points = amitk_roi_ELLIPSOID_get_intersection_line(roi, canvas_slice, pixel_dim);
    break;
  case AMITK_ROI_TYPE_CYLINDER:
    return_points = amitk_roi_CYLINDER_get_intersection_line(roi, canvas_slice, pixel_dim);
    break;
  case AMITK_ROI_TYPE_BOX:
    return_points = amitk_roi_BOX_get_intersection_line(roi, canvas_slice, pixel_dim);
    break;
  default: 
    g_error("roi type %d not implemented! file %s line %d", AMITK_ROI_TYPE(roi), __FILE__, __LINE__);
    break;
  }


  return return_points;
}

GSList * amitk_roi_free_points_list(GSList * list) {

  AmitkPoint * ppoint;

  if (list == NULL) return list;

  list->next = amitk_roi_free_points_list(list->next); /* recurse */

  ppoint = (AmitkPoint *) list->data;

  list = g_slist_remove(list, ppoint);

  g_free(ppoint);

  return list;
}

/* whether we want to use the specified color or have the program choose a decent color */
void amitk_roi_set_specify_color(AmitkRoi * roi, gboolean specify_color) {

  g_return_if_fail(AMITK_IS_ROI(roi));

  if (specify_color != AMITK_ROI_SPECIFY_COLOR(roi)) {
    roi->specify_color = specify_color;
    g_signal_emit(G_OBJECT(roi), roi_signals[ROI_CHANGED], 0);
  }
  return;
}

/* color to draw the roi in, if we choose specify_color */
void amitk_roi_set_color(AmitkRoi * roi, rgba_t new_color) {

  rgba_t old_color;

  g_return_if_fail(AMITK_IS_ROI(roi));
  old_color = AMITK_ROI_COLOR(roi);

  if ((old_color.r != new_color.r) ||
      (old_color.g != new_color.g) ||
      (old_color.b != new_color.b) ||
      (old_color.a != new_color.a)) {
    roi->color = new_color;
    g_signal_emit(G_OBJECT(roi), roi_signals[ROI_CHANGED], 0);
  }
  return;
}


/* this does not recalc the far corner, needs to be done separately */
static void roi_set_voxel_size(AmitkRoi * roi, AmitkPoint voxel_size) {

  if (!POINT_EQUAL(AMITK_ROI_VOXEL_SIZE(roi), voxel_size)) {
    roi->voxel_size = voxel_size;
    roi->center_of_mass_calculated = FALSE;
    g_signal_emit(G_OBJECT(roi), roi_signals[ROI_CHANGED], 0);
  }
}

/* only for isocontour and freehand roi's */
void amitk_roi_set_voxel_size(AmitkRoi * roi, AmitkPoint voxel_size) {

  GList * children;
  AmitkPoint ref_point;
  AmitkPoint scaling;
  AmitkPoint old_corner;

  g_return_if_fail(AMITK_IS_ROI(roi));
  g_return_if_fail(AMITK_ROI_TYPE_ISOCONTOUR(roi) || AMITK_ROI_TYPE_FREEHAND(roi));

  if (!POINT_EQUAL(AMITK_ROI_VOXEL_SIZE(roi), voxel_size)) {
    old_corner = AMITK_VOLUME_CORNER(roi);
    roi_set_voxel_size(roi, voxel_size);
    if (roi->map_data != NULL)
      amitk_roi_calc_far_corner(roi);

    scaling = point_div(AMITK_VOLUME_CORNER(roi), old_corner);
    scaling = amitk_space_s2b_dim(AMITK_SPACE(roi), scaling);
    ref_point = AMITK_SPACE_OFFSET(roi);

    /* propagate this scaling operation to the children */
    children = AMITK_OBJECT_CHILDREN(roi);
    while (children != NULL) {
      amitk_space_scale(children->data, ref_point, scaling);
      children = children->next;
    }
  }
}



/* function to recalculate the far corner of an isocontour roi */
/* only for isocontour and freehand rois */
void amitk_roi_calc_far_corner(AmitkRoi * roi) {

  AmitkPoint new_point;

  g_return_if_fail(AMITK_IS_ROI(roi));
  g_return_if_fail(AMITK_ROI_TYPE_ISOCONTOUR(roi) || AMITK_ROI_TYPE_FREEHAND(roi));

  POINT_MULT(roi->map_data->dim, roi->voxel_size, new_point);
  amitk_volume_set_corner(AMITK_VOLUME(roi), new_point);

  return;
}


/* returns a slice (in a volume structure) containing a  
   data set defining the edges of the roi in the given space.
   returned data set is in the given coord frame.
*/
AmitkDataSet * amitk_roi_get_intersection_slice(const AmitkRoi * roi, 
						const AmitkVolume * canvas_volume,
						const amide_real_t pixel_dim
#ifndef AMIDE_LIBGNOMECANVAS_AA
						,const gboolean fill_map_roi
#endif
						) {
  
  AmitkDataSet * intersection = NULL;

  g_return_val_if_fail(AMITK_IS_ROI(roi), NULL);

  if (AMITK_ROI_UNDRAWN(roi)) return NULL;

  switch(AMITK_ROI_TYPE(roi)) {
  case AMITK_ROI_TYPE_ISOCONTOUR_2D:
    intersection = 
      amitk_roi_ISOCONTOUR_2D_get_intersection_slice(roi, canvas_volume, pixel_dim 
#ifndef AMIDE_LIBGNOMECANVAS_AA
						     , fill_map_roi
#endif
						     );
    break;
  case AMITK_ROI_TYPE_ISOCONTOUR_3D:
    intersection = 
      amitk_roi_ISOCONTOUR_3D_get_intersection_slice(roi, canvas_volume, pixel_dim 
#ifndef AMIDE_LIBGNOMECANVAS_AA
						     , fill_map_roi
#endif
						     );
    break;
  case AMITK_ROI_TYPE_FREEHAND_2D:
    intersection =
      amitk_roi_FREEHAND_2D_get_intersection_slice(roi, canvas_volume, pixel_dim 
#ifndef AMIDE_LIBGNOMECANVAS_AA
						   , fill_map_roi
#endif
						   );
    break;
  case AMITK_ROI_TYPE_FREEHAND_3D:
    intersection =
      amitk_roi_FREEHAND_3D_get_intersection_slice(roi, canvas_volume, pixel_dim 
#ifndef AMIDE_LIBGNOMECANVAS_AA
						   , fill_map_roi
#endif
						   );
    break;
  default: 
    g_error("roi type %d not implemented! file %s line %d",AMITK_ROI_TYPE(roi), __FILE__, __LINE__);
    break;
  }


  return intersection;

}


/* sets/resets the isocontour value of an isocontour ROI based on the given volume and voxel 
   note: vol should be a slice for the case of ISOCONTOUR_2D/FREEHAND_2D */
void amitk_roi_set_isocontour(AmitkRoi * roi, AmitkDataSet * ds, AmitkVoxel start_voxel, 
			      amide_data_t isocontour_min_value, amide_data_t isocontour_max_value,
			      AmitkRoiIsocontourRange isocontour_range) {

  g_return_if_fail(AMITK_ROI_TYPE_ISOCONTOUR(roi));

  switch(AMITK_ROI_TYPE(roi)) {
  case AMITK_ROI_TYPE_ISOCONTOUR_2D:
    amitk_roi_ISOCONTOUR_2D_set_isocontour(roi, ds, start_voxel, isocontour_min_value, isocontour_max_value, isocontour_range);
    break;
  case AMITK_ROI_TYPE_ISOCONTOUR_3D:
  default:
    amitk_roi_ISOCONTOUR_3D_set_isocontour(roi, ds, start_voxel, isocontour_min_value, isocontour_max_value, isocontour_range);
    break;
  }
  roi->center_of_mass_calculated = FALSE;
  
  g_signal_emit(G_OBJECT(roi), roi_signals[ROI_CHANGED], 0);

  return;
}

/* sets an area in the roi to zero (if erase is TRUE) or in (if erase if FALSE) */
/* only works for isocontour and freehand roi's */
void amitk_roi_manipulate_area(AmitkRoi * roi, gboolean erase, AmitkVoxel voxel, gint area_size) {

  g_return_if_fail(AMITK_ROI_TYPE_ISOCONTOUR(roi) || AMITK_ROI_TYPE_FREEHAND(roi));
  
  /* if we're drawing a single point, do a quick check to see if we're already done */
  if (!AMITK_ROI_UNDRAWN(roi) && (area_size == 0)) {
    if (erase) {
      if (amitk_raw_data_includes_voxel(roi->map_data, voxel)) {
	if (AMITK_RAW_DATA_UBYTE_SET_CONTENT(roi->map_data, voxel)==0) {
	  return;
	}
      }
    } else {
      if (amitk_raw_data_includes_voxel(roi->map_data, voxel)) {
	if (AMITK_RAW_DATA_UBYTE_SET_CONTENT(roi->map_data, voxel)) {
	  return;
	}
      }
    }
  }

  switch(AMITK_ROI_TYPE(roi)) {
  case AMITK_ROI_TYPE_ISOCONTOUR_2D:
    amitk_roi_ISOCONTOUR_2D_manipulate_area(roi, erase, voxel, area_size);
    break;
  case AMITK_ROI_TYPE_ISOCONTOUR_3D:
    amitk_roi_ISOCONTOUR_3D_manipulate_area(roi, erase, voxel, area_size);
    break;
  case AMITK_ROI_TYPE_FREEHAND_2D:
    amitk_roi_FREEHAND_2D_manipulate_area(roi, erase, voxel, area_size);
    break;
  case AMITK_ROI_TYPE_FREEHAND_3D:
    amitk_roi_FREEHAND_3D_manipulate_area(roi, erase, voxel, area_size);
    break;
  default:
    g_error("unexpected case in %s at line %d\n", __FILE__, __LINE__);
    break;
  }
  roi->center_of_mass_calculated = FALSE;

  g_signal_emit(G_OBJECT(roi), roi_signals[ROI_CHANGED], 0);

  return;
}

/* only for isocontour and freehand rois */
AmitkPoint amitk_roi_get_center_of_mass (AmitkRoi * roi) {

  g_return_val_if_fail(AMITK_IS_ROI(roi), zero_point);
  g_return_val_if_fail((AMITK_ROI_TYPE_ISOCONTOUR(roi) || AMITK_ROI_TYPE_FREEHAND(roi)), zero_point);

  if (!roi->center_of_mass_calculated) {
    switch(AMITK_ROI_TYPE(roi)) {
    case AMITK_ROI_TYPE_ISOCONTOUR_2D:
      amitk_roi_ISOCONTOUR_2D_calc_center_of_mass(roi);
      break;
    case AMITK_ROI_TYPE_ISOCONTOUR_3D:
      amitk_roi_ISOCONTOUR_3D_calc_center_of_mass(roi);
      break;
    case AMITK_ROI_TYPE_FREEHAND_2D:
      amitk_roi_FREEHAND_2D_calc_center_of_mass(roi);
      break;
    case AMITK_ROI_TYPE_FREEHAND_3D:
      amitk_roi_FREEHAND_3D_calc_center_of_mass(roi);
      break;
    default:
      g_error("unexpected case in %s at line %d\n", __FILE__, __LINE__);
      break;
    }
  }

  
  return amitk_space_s2b(AMITK_SPACE(roi), roi->center_of_mass);
}

void amitk_roi_set_type(AmitkRoi * roi, AmitkRoiType new_type) {

  g_return_if_fail(AMITK_IS_ROI(roi));
  if ((new_type == AMITK_ROI_TYPE_ISOCONTOUR_2D) ||
      (new_type == AMITK_ROI_TYPE_ISOCONTOUR_3D) ||
      AMITK_ROI_TYPE_ISOCONTOUR(roi) ||
      (new_type == AMITK_ROI_TYPE_FREEHAND_2D) ||
      (new_type == AMITK_ROI_TYPE_FREEHAND_3D) ||
      AMITK_ROI_TYPE_FREEHAND(roi))
    g_return_if_fail(new_type == AMITK_ROI_TYPE(roi));

  if (AMITK_ROI_TYPE(roi) != new_type) {
    roi->type = new_type;
    g_signal_emit(G_OBJECT(roi), roi_signals[ROI_TYPE_CHANGED], 0);
    g_signal_emit(G_OBJECT(roi), roi_signals[ROI_CHANGED], 0);
  }
  return;
}

/* iterates over the voxels in the given data set that are inside the given roi,
   and performs the specified calculation function for those points */
/* if inverse is true, the calculation is done for the portion of the data set not in the roi */
/* if accurate is true, uses much slower but more accurate calculation */
/* calulation should be a function taking the following arguments:
   calculation(AmitkVoxel dataset_voxel, amide_data_t value, amide_real_t voxel_fraction, gpointer data) */
void amitk_roi_calculate_on_data_set(const AmitkRoi * roi,  
				     const AmitkDataSet * ds, 
				     const guint frame,
				     const guint gate,
				     const gboolean inverse,
				     const gboolean accurate,
				     void (*calculation)(),
				     gpointer data) {

  g_return_if_fail(AMITK_IS_ROI(roi));
  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  
  if (AMITK_ROI_UNDRAWN(roi)) return;

  switch(AMITK_ROI_TYPE(roi)) {
  case AMITK_ROI_TYPE_ELLIPSOID:
    if (accurate)
      amitk_roi_ELLIPSOID_calculate_on_data_set_accurate(roi, ds, frame, gate, inverse, calculation, data);
    else
      amitk_roi_ELLIPSOID_calculate_on_data_set_fast(roi, ds, frame, gate, inverse, calculation, data);
    break;
  case AMITK_ROI_TYPE_CYLINDER:
    if (accurate)
      amitk_roi_CYLINDER_calculate_on_data_set_accurate(roi, ds, frame, gate, inverse, calculation, data);
    else
      amitk_roi_CYLINDER_calculate_on_data_set_fast(roi, ds, frame, gate, inverse, calculation, data);
    break;
  case AMITK_ROI_TYPE_BOX:
    if (accurate)
      amitk_roi_BOX_calculate_on_data_set_accurate(roi, ds, frame, gate, inverse, calculation, data);
    else
      amitk_roi_BOX_calculate_on_data_set_fast(roi, ds, frame, gate, inverse, calculation, data);
    break;
  case AMITK_ROI_TYPE_ISOCONTOUR_2D:
    if (accurate)
      amitk_roi_ISOCONTOUR_2D_calculate_on_data_set_accurate(roi, ds, frame, gate, inverse, calculation, data);
    else
      amitk_roi_ISOCONTOUR_2D_calculate_on_data_set_fast(roi, ds, frame, gate, inverse, calculation, data);
    break;
  case AMITK_ROI_TYPE_ISOCONTOUR_3D:
    if (accurate)
      amitk_roi_ISOCONTOUR_3D_calculate_on_data_set_accurate(roi, ds, frame, gate, inverse, calculation, data);
    else
      amitk_roi_ISOCONTOUR_3D_calculate_on_data_set_fast(roi, ds, frame, gate, inverse, calculation, data);
    break;
  case AMITK_ROI_TYPE_FREEHAND_2D:
    if (accurate)
      amitk_roi_FREEHAND_2D_calculate_on_data_set_accurate(roi, ds, frame, gate, inverse, calculation, data);
    else
      amitk_roi_FREEHAND_2D_calculate_on_data_set_fast(roi, ds, frame, gate, inverse, calculation, data);
    break;
  case AMITK_ROI_TYPE_FREEHAND_3D:
    if (accurate)
      amitk_roi_FREEHAND_3D_calculate_on_data_set_accurate(roi, ds, frame, gate, inverse, calculation, data);
    else
      amitk_roi_FREEHAND_3D_calculate_on_data_set_fast(roi, ds, frame, gate, inverse, calculation, data);
    break;
  default: 
    g_error("roi type %d not implemented! file %s line %d",AMITK_ROI_TYPE(roi), __FILE__, __LINE__);
    break;
  }

  return;
}

static void erase_volume(AmitkVoxel voxel, 
			 amide_data_t value, 
			 amide_real_t voxel_fraction, 
			 gpointer ds) {

  amitk_data_set_set_value(AMITK_DATA_SET(ds), voxel, 
			   (value*(1.0-voxel_fraction)+
			    AMITK_DATA_SET_THRESHOLD_MIN(ds, 0)*voxel_fraction),
			   FALSE);
  return;
}


/* sets the volume inside/or outside of the given data set that is enclosed by roi equal to zero */
void amitk_roi_erase_volume(const AmitkRoi * roi, 
			    AmitkDataSet * ds, 
			    const gboolean outside,
			    AmitkUpdateFunc update_func,
			    gpointer update_data) {

  guint i_frame;
  guint i_gate;

  for (i_frame=0; i_frame<AMITK_DATA_SET_NUM_FRAMES(ds); i_frame++) 
    for (i_gate=0; i_gate<AMITK_DATA_SET_NUM_GATES(ds); i_gate++) 
      amitk_roi_calculate_on_data_set(roi, ds, i_frame, i_gate, outside, FALSE, erase_volume, ds);

  /* recalc max and min */
  amitk_data_set_calc_min_max(ds, update_func, update_data);

  /* mark the distribution data as invalid */
  if (AMITK_DATA_SET_DISTRIBUTION(ds) != NULL) {
    g_object_unref(AMITK_DATA_SET_DISTRIBUTION(ds));
    ds->distribution = NULL;
  }

  /* this is a no-op to get a data_set_changed signal */
  amitk_data_set_set_value(AMITK_DATA_SET(ds), zero_voxel,
			   amitk_data_set_get_value(AMITK_DATA_SET(ds), zero_voxel),
			   TRUE);

  return;
}


const gchar * amitk_roi_type_get_name(const AmitkRoiType roi_type) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_ROI_TYPE);
  enum_value = g_enum_get_value(enum_class, roi_type);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}



/* returns the minimum dimensional width of the roi with the largest voxel size */
/* only operates on ISOCONTOUR roi's */
amide_real_t amitk_rois_get_max_min_voxel_size(GList * objects) {

  amide_real_t min_voxel_size, temp;

  if (objects == NULL) return -1.0; /* invalid */

  /* first process the rest of the list */
  min_voxel_size = amitk_rois_get_max_min_voxel_size(objects->next);

  /* now process and compare to the children */
  temp = amitk_rois_get_max_min_voxel_size(AMITK_OBJECT_CHILDREN(objects->data));
  if (temp > 0) {
    if (min_voxel_size < 0.0) min_voxel_size = temp;
    else if (temp > min_voxel_size) min_voxel_size = temp;
  }

  /* and process this guy */
  if (AMITK_IS_ROI(objects->data))
    if (AMITK_ROI_TYPE_ISOCONTOUR(objects->data) || AMITK_ROI_TYPE_FREEHAND(objects->data)) {
      temp = point_min_dim(AMITK_ROI_VOXEL_SIZE(objects->data));
      if (min_voxel_size < 0.0) min_voxel_size = temp;
      else if (temp > min_voxel_size) min_voxel_size = temp;
    }

  return min_voxel_size;
}


