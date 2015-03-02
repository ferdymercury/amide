/* amitk_roi.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2004 Andy Loening
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



gchar * amitk_roi_menu_names[] = {
  N_("_Ellipsoid"), 
  N_("Elliptic _Cylinder"), 
  N_("_Box"),
  N_("_2D Isocontour"),
  N_("_3D Isocontour"),
};

gchar * amitk_roi_menu_explanation[] = {
  N_("Add a new elliptical ROI"), 
  N_("Add a new elliptic cylinder ROI"), 
  N_("Add a new box shaped ROI"),
  N_("Add a new 2D Isocontour ROI"),
  N_("Add a new 3D Isocontour ROI"),
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
static void          roi_isocontour_set_voxel_size(AmitkRoi * roi, 
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

  parent_class = g_type_class_peek_parent(class);

  space_class->space_scale = roi_scale;

  object_class->object_copy = roi_copy;
  object_class->object_copy_in_place = roi_copy_in_place;
  object_class->object_write_xml = roi_write_xml;
  object_class->object_read_xml = roi_read_xml;

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

  roi->isocontour = NULL;
  roi->voxel_size = zero_point;
  roi->isocontour_value = 0.0;
  roi->isocontour_inverse = FALSE;
}


static void roi_finalize (GObject *object)
{
  AmitkRoi * roi = AMITK_ROI(object);

  if (roi->isocontour != NULL) {
    g_object_unref(roi->isocontour);
    roi->isocontour = NULL;
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
    if (AMITK_ROI_TYPE_ISOCONTOUR(roi)) {

      voxel_size = AMITK_VOLUME_CORNER(roi);
      voxel_size.x /= roi->isocontour->dim.x;
      voxel_size.y /= roi->isocontour->dim.y;
      voxel_size.z /= roi->isocontour->dim.z;
      roi_isocontour_set_voxel_size(roi, voxel_size);
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

/* doesn't copy the data set used by isocontours, just adds a reference... */
static void roi_copy_in_place (AmitkObject * dest_object, const AmitkObject * src_object) {

  AmitkRoi * src_roi;
  AmitkRoi * dest_roi;

  g_return_if_fail(AMITK_IS_ROI(src_object));
  g_return_if_fail(AMITK_IS_ROI(dest_object));

  src_roi = AMITK_ROI(src_object);
  dest_roi = AMITK_ROI(dest_object);

  amitk_roi_set_type(dest_roi, AMITK_ROI_TYPE(src_object));

  if (src_roi->isocontour != NULL) {
    if (dest_roi->isocontour != NULL)
      g_object_unref(dest_roi->isocontour);
    dest_roi->isocontour = g_object_ref(src_roi->isocontour);
  }

  roi_isocontour_set_voxel_size(dest_roi, AMITK_ROI_VOXEL_SIZE(src_object));
  dest_roi->isocontour_value = AMITK_ROI_ISOCONTOUR_VALUE(src_object);
  dest_roi->isocontour_inverse = AMITK_ROI_ISOCONTOUR_INVERSE(src_object);

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

  /* isocontour specific stuff */
  amitk_point_write_xml(nodes, "voxel_size", AMITK_ROI_VOXEL_SIZE(roi));
  xml_save_real(nodes, "isocontour_value", AMITK_ROI_ISOCONTOUR_VALUE(roi));
  xml_save_boolean(nodes, "isocontour_inverse", AMITK_ROI_ISOCONTOUR_INVERSE(roi));

  if (AMITK_ROI_TYPE_ISOCONTOUR(roi)) {
    name = g_strdup_printf("roi_%s_isocontour", AMITK_OBJECT_NAME(roi));
    amitk_raw_data_write_xml(roi->isocontour, name, study_file, &filename, &location, &size);
    g_free(name);
    if (study_file == NULL) {
      xml_save_string(nodes,"isocontour_file", filename);
      g_free(filename);
    } else {
      xml_save_location_and_size(nodes, "isocontour_location_and_size", location, size);
    }
  }

  return;
}


static gchar * roi_read_xml (AmitkObject * object, xmlNodePtr nodes, FILE * study_file, gchar * error_buf) {

  AmitkRoi * roi;
  AmitkRoiType i_roi_type;
  gchar * temp_string;
  gchar * isocontour_xml_filename=NULL;
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

  /* isocontour specific stuff */
  if (AMITK_ROI_TYPE_ISOCONTOUR(roi)) {
    roi_isocontour_set_voxel_size(roi, amitk_point_read_xml(nodes, "voxel_size", &error_buf));
    roi->isocontour_value = xml_get_real(nodes, "isocontour_value", &error_buf);
    roi->isocontour_inverse = xml_get_boolean(nodes, "isocontour_inverse", &error_buf);

    /* if the ROI's never been drawn, it's possible for this not to exist */
    if (xml_node_exists(nodes, "isocontour_file") || 
	xml_node_exists(nodes, "isocontour_location_and_size")) {
      if (study_file == NULL) 
	isocontour_xml_filename = xml_get_string(nodes, "isocontour_file");
      else
	xml_get_location_and_size(nodes, "isocontour_location_and_size", &location, &size, &error_buf);
      roi->isocontour = amitk_raw_data_read_xml(isocontour_xml_filename, study_file, location, 
						size,&error_buf, NULL, NULL);
      if (isocontour_xml_filename != NULL) g_free(isocontour_xml_filename);
    }
  }

  /* make sure to mark the roi as undrawn if needed */
  if (AMITK_ROI_TYPE_ISOCONTOUR(roi)) {
    if (roi->isocontour == NULL) 
      AMITK_VOLUME(roi)->valid = FALSE;
  } else {
    if (POINT_EQUAL(AMITK_VOLUME_CORNER(roi), zero_point)) {
      AMITK_VOLUME(roi)->valid = FALSE;
    }
  }

  return error_buf;
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
    g_error("roi type %d not implemented!",AMITK_ROI_TYPE(roi));
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


/* this does not recalc the far corner, needs to be done separately */
static void roi_isocontour_set_voxel_size(AmitkRoi * roi, AmitkPoint voxel_size) {

  if (!POINT_EQUAL(AMITK_ROI_VOXEL_SIZE(roi), voxel_size)) {
    roi->voxel_size = voxel_size;
    g_signal_emit(G_OBJECT(roi), roi_signals[ROI_CHANGED], 0);
  }
}

void amitk_roi_isocontour_set_voxel_size(AmitkRoi * roi, AmitkPoint voxel_size) {

  GList * children;
  AmitkPoint ref_point;
  AmitkPoint scaling;
  AmitkPoint old_corner;

  g_return_if_fail(AMITK_IS_ROI(roi));
  g_return_if_fail(AMITK_ROI_TYPE_ISOCONTOUR(roi));

  if (!POINT_EQUAL(AMITK_ROI_VOXEL_SIZE(roi), voxel_size)) {
    old_corner = AMITK_VOLUME_CORNER(roi);
    roi_isocontour_set_voxel_size(roi, voxel_size);
    amitk_roi_isocontour_calc_far_corner(roi);

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
void amitk_roi_isocontour_calc_far_corner(AmitkRoi * roi) {

  AmitkPoint new_point;

  g_return_if_fail(AMITK_IS_ROI(roi));
  g_return_if_fail(AMITK_ROI_TYPE_ISOCONTOUR(roi));

  POINT_MULT(roi->isocontour->dim, roi->voxel_size, new_point);
  amitk_volume_set_corner(AMITK_VOLUME(roi), new_point);

  return;
}


/* returns a slice (in a volume structure) containing a  
   data set defining the edges of the roi in the given space.
   returned data set is in the given coord frame.
*/
AmitkDataSet * amitk_roi_get_intersection_slice(const AmitkRoi * roi, 
						const AmitkVolume * canvas_volume,
						const amide_real_t pixel_dim,
						const gboolean fill_isocontour) {
  
  AmitkDataSet * intersection = NULL;

  g_return_val_if_fail(AMITK_IS_ROI(roi), NULL);

  if (AMITK_ROI_UNDRAWN(roi)) return NULL;

  switch(AMITK_ROI_TYPE(roi)) {
  case AMITK_ROI_TYPE_ISOCONTOUR_2D:
    intersection = 
      amitk_roi_ISOCONTOUR_2D_get_intersection_slice(roi, canvas_volume, pixel_dim, fill_isocontour);
    break;
  case AMITK_ROI_TYPE_ISOCONTOUR_3D:
    intersection = 
      amitk_roi_ISOCONTOUR_3D_get_intersection_slice(roi, canvas_volume, pixel_dim, fill_isocontour);
    break;
  default: 
    g_error("roi type %d not implemented!", AMITK_ROI_TYPE(roi));
    break;
  }


  return intersection;

}


/* sets/resets the isocontour value of an isocontour ROI based on the given volume and voxel 
   note: vol should be a slice for the case of ISOCONTOUR_2D */
void amitk_roi_set_isocontour(AmitkRoi * roi, AmitkDataSet * ds, AmitkVoxel start_voxel, 
			      amide_data_t isocontour_value, gboolean isocontour_inverse) {

  g_return_if_fail(AMITK_ROI_TYPE_ISOCONTOUR(roi));

  switch(AMITK_ROI_TYPE(roi)) {
  case AMITK_ROI_TYPE_ISOCONTOUR_2D:
    amitk_roi_ISOCONTOUR_2D_set_isocontour(roi, ds, start_voxel, isocontour_value, isocontour_inverse);
    break;
  case AMITK_ROI_TYPE_ISOCONTOUR_3D:
  default:
    amitk_roi_ISOCONTOUR_3D_set_isocontour(roi, ds, start_voxel, isocontour_value, isocontour_inverse);
    break;
  }
  
  g_signal_emit(G_OBJECT(roi), roi_signals[ROI_CHANGED], 0);

  return;
}

/* sets an area in the roi to zero (not in the roi) */
void amitk_roi_isocontour_erase_area(AmitkRoi * roi, AmitkVoxel erase_voxel, gint area_size) {

  g_return_if_fail(AMITK_ROI_TYPE_ISOCONTOUR(roi));
  
  switch(AMITK_ROI_TYPE(roi)) {
  case AMITK_ROI_TYPE_ISOCONTOUR_2D:
    amitk_roi_ISOCONTOUR_2D_erase_area(roi, erase_voxel, area_size);
    break;
  case AMITK_ROI_TYPE_ISOCONTOUR_3D:
  default:
    amitk_roi_ISOCONTOUR_3D_erase_area(roi, erase_voxel, area_size);
    break;
  }

  g_signal_emit(G_OBJECT(roi), roi_signals[ROI_CHANGED], 0);

  return;
}


void amitk_roi_set_type(AmitkRoi * roi, AmitkRoiType new_type) {

  g_return_if_fail(AMITK_IS_ROI(roi));
  if ((new_type == AMITK_ROI_TYPE_ISOCONTOUR_2D) ||
      (new_type == AMITK_ROI_TYPE_ISOCONTOUR_3D) ||
      AMITK_ROI_TYPE_ISOCONTOUR(roi))
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
   calculation(AmitkVoxel voxel, amide_data_t value, amide_real_t voxel_fraction, gpointer data) */
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
  default: 
    g_error("roi type %d not implemented!",AMITK_ROI_TYPE(roi));
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
			    gboolean (*update_func)(),
			    gpointer update_data) {

  guint i_frame;
  guint i_gate;

  for (i_frame=0; i_frame<AMITK_DATA_SET_NUM_FRAMES(ds); i_frame++) 
    for (i_gate=0; i_gate<AMITK_DATA_SET_NUM_GATES(ds); i_gate++) 
      amitk_roi_calculate_on_data_set(roi, ds, i_frame, i_gate, outside, FALSE, erase_volume, ds);

  /* recalc max and min */
  amitk_data_set_calc_max_min(ds, update_func, update_data);

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
    if (AMITK_ROI_TYPE_ISOCONTOUR(objects->data)) {
      temp = point_min_dim(AMITK_ROI_VOXEL_SIZE(objects->data));
      if (min_voxel_size < 0.0) min_voxel_size = temp;
      else if (temp > min_voxel_size) min_voxel_size = temp;
    }

  return min_voxel_size;
}


