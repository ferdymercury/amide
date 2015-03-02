/* amitk_roi.c
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
  "_Ellipsoid", 
  "Elliptic _Cylinder", 
  "_Box",
  "_2D Isocontour",
  "_3D Isocontour",
};

gchar * amitk_roi_menu_explanation[] = {
  "Add a new elliptical ROI", 
  "Add a new elliptic cylinder ROI", 
  "Add a new box shaped ROI",
  "Add a new 2D Isocontour ROI",
  "Add a new 3D Isocontour ROI",
};


enum {
  ROI_CHANGED,
  ROI_TYPE_CHANGED,
  LAST_SIGNAL
};

static void          roi_class_init          (AmitkRoiClass *klass);
static void          roi_init                (AmitkRoi      *roi);
static void          roi_finalize            (GObject          *object);
static AmitkObject * roi_copy                (const AmitkObject * object);
static void          roi_copy_in_place       (AmitkObject * dest_object, const AmitkObject * src_object);
static void          roi_write_xml           (const AmitkObject * object, xmlNodePtr nodes);
static void          roi_read_xml            (AmitkObject * object, xmlNodePtr nodes);
static AmitkVolume * parent_class;
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

  parent_class = g_type_class_peek_parent(class);

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

  amitk_roi_set_voxel_size(dest_roi, AMITK_ROI_VOXEL_SIZE(src_object));
  dest_roi->isocontour_value = AMITK_ROI_ISOCONTOUR_VALUE(src_object);

  AMITK_OBJECT_CLASS (parent_class)->object_copy_in_place (dest_object, src_object);
}


static void roi_write_xml (const AmitkObject * object, xmlNodePtr nodes) {

  AmitkRoi * roi;
  gchar * name;
  gchar * filename;

  AMITK_OBJECT_CLASS(parent_class)->object_write_xml(object, nodes);

  roi = AMITK_ROI(object);

  xml_save_string(nodes, "type", amitk_roi_type_get_name(AMITK_ROI_TYPE(roi)));

  /* isocontour specific stuff */
  amitk_point_write_xml(nodes, "voxel_size", AMITK_ROI_VOXEL_SIZE(roi));
  xml_save_real(nodes, "isocontour_value", AMITK_ROI_ISOCONTOUR_VALUE(roi));

  if ((AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_ISOCONTOUR_2D) || 
      (AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_ISOCONTOUR_3D)) {
    name = g_strdup_printf("roi_%s_isocontour", AMITK_OBJECT_NAME(roi));
    filename = amitk_raw_data_write_xml(roi->isocontour, name);
    g_free(name);
    xml_save_string(nodes,"isocontour_file", filename);
    g_free(filename);
  } else {
    xml_save_string(nodes,"isocontour_file", NULL);
  }

  return;
}


static void roi_read_xml (AmitkObject * object, xmlNodePtr nodes) {

  AmitkRoi * roi;
  AmitkRoiType i_roi_type;
  gchar * temp_string;
  gchar * isocontour_xml_filename;

  AMITK_OBJECT_CLASS(parent_class)->object_read_xml(object, nodes);

  roi = AMITK_ROI(object);

  /* figure out the type */
  temp_string = xml_get_string(nodes, "type");
  if (temp_string != NULL)
    for (i_roi_type=0; i_roi_type < AMITK_ROI_TYPE_NUM; i_roi_type++) 
      if (g_strcasecmp(temp_string, amitk_roi_type_get_name(i_roi_type)) == 0)
	roi->type = i_roi_type;
  g_free(temp_string);

  /* isocontour specific stuff */
  if ((AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_ISOCONTOUR_2D) || 
      (AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_ISOCONTOUR_3D)) {
    amitk_roi_set_voxel_size(roi, amitk_point_read_xml(nodes, "voxel_size"));
    roi->isocontour_value = xml_get_real(nodes, "isocontour_value");

    isocontour_xml_filename = xml_get_string(nodes, "isocontour_file");
    if (isocontour_xml_filename != NULL)
      roi->isocontour = amitk_raw_data_read_xml(isocontour_xml_filename);
  }

  /* make sure to mark the roi as undrawn if needed */
  if ((AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_ISOCONTOUR_2D) || 
      (AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_ISOCONTOUR_3D)) {
    if (roi->isocontour == NULL) 
      AMITK_VOLUME(roi)->valid = FALSE;
  } else {
    if (POINT_EQUAL(AMITK_VOLUME_CORNER(roi), zero_point)) {
      AMITK_VOLUME(roi)->valid = FALSE;
    }
  }

  return;
}


AmitkRoi * amitk_roi_new (AmitkRoiType type) {

  AmitkRoi * roi;

  roi = g_object_new(amitk_roi_get_type(), NULL);
  roi->type = type;

  return roi;
}


/* figure out the smallest subset of the specified volume in space that completely  encloses the roi */
void amitk_roi_volume_intersection(const AmitkRoi * roi,
				   const AmitkVolume * volume,
				   const AmitkPoint voxel_size,
				   AmitkVoxel * subset_start,
				   AmitkVoxel * subset_dim) {
  
  AmitkCorners roi_corners;
  AmitkCorners subset_corners;
  AmitkVoxel subset_indexes[2];

  g_return_if_fail(AMITK_IS_ROI(roi));

  /* get the corners in the volume's space that orthogonally encloses the roi */
  amitk_volume_get_enclosing_corners(AMITK_VOLUME(roi), AMITK_SPACE(volume), subset_corners);

  /* look at all eight corners of the roi cube, figure out the max and min dim */
  amitk_space_get_enclosing_corners(AMITK_SPACE(roi), roi_corners, AMITK_SPACE(volume), subset_corners);

  /* and convert the subset_corners into indexes */
  POINT_TO_VOXEL(subset_corners[0], voxel_size, 0, subset_indexes[0]);
  POINT_TO_VOXEL(subset_corners[1], voxel_size, 0, subset_indexes[1]);

  /* and calculate the return values */
  *subset_start = subset_indexes[0];
  *subset_dim = voxel_add(voxel_sub(subset_indexes[1], subset_indexes[0]), one_voxel);

  return;
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


/* returns a slice (in a volume structure) containing a  
   data set defining the edges of the roi in the given space.
   returned data set is in the given coord frame.
*/
AmitkDataSet * amitk_roi_get_intersection_slice(const AmitkRoi * roi, 
						const AmitkVolume * canvas_volume,
						const amide_real_t pixel_dim) {
  
  AmitkDataSet * intersection = NULL;

  g_return_val_if_fail(AMITK_IS_ROI(roi), NULL);

  if (AMITK_ROI_UNDRAWN(roi)) return NULL;

  switch(AMITK_ROI_TYPE(roi)) {
  case AMITK_ROI_TYPE_ISOCONTOUR_2D:
    intersection = 
      amitk_roi_ISOCONTOUR_2D_get_intersection_slice(roi, canvas_volume, pixel_dim);
    break;
  case AMITK_ROI_TYPE_ISOCONTOUR_3D:
    intersection = 
      amitk_roi_ISOCONTOUR_3D_get_intersection_slice(roi, canvas_volume, pixel_dim);
    break;
  default: 
    g_error("roi type %d not implemented!", AMITK_ROI_TYPE(roi));
    break;
  }


  return intersection;

}


/* sets/resets the isocontour value of an isocontour ROI based on the given volume and voxel 
   note: vol should be a slice for the case of ISOCONTOUR_2D */
void amitk_roi_set_isocontour(AmitkRoi * roi, AmitkDataSet * ds, AmitkVoxel value_voxel) {

  g_return_if_fail((AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_ISOCONTOUR_2D) || 
		   (AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_ISOCONTOUR_3D));

  
  switch(AMITK_ROI_TYPE(roi)) {
  case AMITK_ROI_TYPE_ISOCONTOUR_2D:
    amitk_roi_ISOCONTOUR_2D_set_isocontour(roi, ds, value_voxel);
    break;
  case AMITK_ROI_TYPE_ISOCONTOUR_3D:
  default:
    amitk_roi_ISOCONTOUR_3D_set_isocontour(roi, ds, value_voxel);
    break;
  }
  
  g_signal_emit(G_OBJECT(roi), roi_signals[ROI_CHANGED], 0);

  return;
}

/* sets an area in the roi to zero (not in the roi) */
void amitk_roi_isocontour_erase_area(AmitkRoi * roi, AmitkVoxel erase_voxel, gint area_size) {

  g_return_if_fail((AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_ISOCONTOUR_2D) || 
		   (AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_ISOCONTOUR_3D));
  
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
      (AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_ISOCONTOUR_2D) ||
      (AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_ISOCONTOUR_3D))
    g_return_if_fail(new_type == AMITK_ROI_TYPE(roi));

  if (AMITK_ROI_TYPE(roi) != new_type) {
    roi->type = new_type;
    g_signal_emit(G_OBJECT(roi), roi_signals[ROI_TYPE_CHANGED], 0);
    g_signal_emit(G_OBJECT(roi), roi_signals[ROI_CHANGED], 0);
  }
  return;
}

void amitk_roi_set_voxel_size(AmitkRoi * roi, AmitkPoint voxel_size) {

  g_return_if_fail(AMITK_IS_ROI(roi));

  if (!POINT_EQUAL(AMITK_ROI_VOXEL_SIZE(roi), voxel_size)) {
    roi->voxel_size = voxel_size;
    g_signal_emit(G_OBJECT(roi), roi_signals[ROI_CHANGED], 0);
  }
}


/* iterates over the voxels in the given data set that are inside the given roi,
   and performs the specified calculation function for those points */
/* calulation should be a function taking the following arguments:
   calculation(AmitkVoxel voxel, amide_data_t value, amide_real_t voxel_fraction, gpointer data) */
void amitk_roi_calculate_on_data_set(const AmitkRoi * roi,  
				     const AmitkDataSet * ds, 
				     const guint frame,
				     void (*calculation)(),
				     gpointer data) {

  g_return_if_fail(AMITK_IS_ROI(roi));
  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  
  if (AMITK_ROI_UNDRAWN(roi)) return;

  switch(AMITK_ROI_TYPE(roi)) {
  case AMITK_ROI_TYPE_ELLIPSOID:
    amitk_roi_ELLIPSOID_calculate_on_data_set(roi, ds, frame, calculation, data);
    break;
  case AMITK_ROI_TYPE_CYLINDER:
    amitk_roi_CYLINDER_calculate_on_data_set(roi, ds, frame, calculation, data);
    break;
  case AMITK_ROI_TYPE_BOX:
    amitk_roi_BOX_calculate_on_data_set(roi, ds, frame, calculation, data);
    break;
  case AMITK_ROI_TYPE_ISOCONTOUR_2D:
    amitk_roi_ISOCONTOUR_2D_calculate_on_data_set(roi, ds, frame, calculation, data);
    break;
  case AMITK_ROI_TYPE_ISOCONTOUR_3D:
    amitk_roi_ISOCONTOUR_3D_calculate_on_data_set(roi, ds, frame, calculation, data);
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


/* sets the volume inside of the given data set that is enclosed by roi equal to zero */
void amitk_roi_erase_volume(const AmitkRoi * roi, AmitkDataSet * ds) {

  guint i_frame;

  for (i_frame=0; i_frame<AMITK_DATA_SET_NUM_FRAMES(ds); i_frame++)
    amitk_roi_calculate_on_data_set(roi, ds, i_frame, erase_volume, ds);

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
    if ((AMITK_ROI_TYPE(objects->data) == AMITK_ROI_TYPE_ISOCONTOUR_2D) ||
	(AMITK_ROI_TYPE(objects->data) == AMITK_ROI_TYPE_ISOCONTOUR_3D)) {
      temp = point_min_dim(AMITK_ROI_VOXEL_SIZE(objects->data));
      if (min_voxel_size < 0.0) min_voxel_size = temp;
      else if (temp > min_voxel_size) min_voxel_size = temp;
    }

  return min_voxel_size;
}


