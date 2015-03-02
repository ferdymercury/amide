/* amitk_roi.h
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

#ifndef __AMITK_ROI_H__
#define __AMITK_ROI_H__


#include "amitk_volume.h"
#include "amitk_data_set.h"

G_BEGIN_DECLS

#define	AMITK_TYPE_ROI		        (amitk_roi_get_type ())
#define AMITK_ROI(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), AMITK_TYPE_ROI, AmitkRoi))
#define AMITK_ROI_CLASS(klass)	        (G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_ROI, AmitkRoiClass))
#define AMITK_IS_ROI(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), AMITK_TYPE_ROI))
#define AMITK_IS_ROI_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_ROI))
#define	AMITK_ROI_GET_CLASS(object)	(G_TYPE_CHECK_GET_CLASS ((object), AMITK_TYPE_ROI, AmitkRoiClass))

#define AMITK_ROI_TYPE(roi)                  (AMITK_ROI(roi)->type)
#define AMITK_ROI_SPECIFY_COLOR(roi)         (AMITK_ROI(roi)->specify_color)
#define AMITK_ROI_COLOR(roi)                 (AMITK_ROI(roi)->color)
#define AMITK_ROI_ISOCONTOUR_MIN_VALUE(roi)  (AMITK_ROI(roi)->isocontour_min_value)
#define AMITK_ROI_ISOCONTOUR_MAX_VALUE(roi)  (AMITK_ROI(roi)->isocontour_max_value)
#define AMITK_ROI_ISOCONTOUR_RANGE(roi)      (AMITK_ROI(roi)->isocontour_range)
#define AMITK_ROI_VOXEL_SIZE(roi)            (AMITK_ROI(roi)->voxel_size)
#define AMITK_ROI_UNDRAWN(roi)               (!AMITK_VOLUME_VALID(roi))
#define AMITK_ROI_TYPE_ISOCONTOUR(roi)       ((AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_ISOCONTOUR_2D) || \
					      (AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_ISOCONTOUR_3D))
#define AMITK_ROI_TYPE_FREEHAND(roi)         ((AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_FREEHAND_2D) || \
					      (AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_FREEHAND_3D))

/* for iterative algorithms, how many subvoxels should we break the problem up into */
#define AMITK_ROI_GRANULARITY 4 /* # subvoxels in one dimension, so 1/64 is grain size */
//#define AMITK_ROI_GRANULARITY 10 - takes way to long

typedef enum {
  AMITK_ROI_TYPE_ELLIPSOID, 
  AMITK_ROI_TYPE_CYLINDER, 
  AMITK_ROI_TYPE_BOX, 
  AMITK_ROI_TYPE_ISOCONTOUR_2D, 
  AMITK_ROI_TYPE_ISOCONTOUR_3D, 
  AMITK_ROI_TYPE_FREEHAND_2D,
  AMITK_ROI_TYPE_FREEHAND_3D,
  AMITK_ROI_TYPE_NUM
} AmitkRoiType;

typedef enum {
  AMITK_ROI_ISOCONTOUR_RANGE_ABOVE_MIN,
  AMITK_ROI_ISOCONTOUR_RANGE_BELOW_MAX,
  AMITK_ROI_ISOCONTOUR_RANGE_BETWEEN_MIN_MAX,
  AMITK_ROI_ISOCONTOUR_RANGE_NUM
} AmitkRoiIsocontourRange;


typedef struct _AmitkRoiClass AmitkRoiClass;
typedef struct _AmitkRoi AmitkRoi;


struct _AmitkRoi
{
  AmitkVolume parent;

  AmitkRoiType type;

  gboolean specify_color; /* if false, program guesses a good color to use */
  rgba_t color;

  /* isocontour and freehand specific stuff */
  AmitkPoint voxel_size;
  AmitkRawData * map_data; /* raw data */
  gboolean center_of_mass_calculated;
  AmitkPoint center_of_mass;

  /* isocontour specific stuff */
  amide_data_t isocontour_min_value; /* note, min and max are what were specified for the isocontour */
  amide_data_t isocontour_max_value; /* what the user draws may lie outside of this range */
  AmitkRoiIsocontourRange isocontour_range;

};

struct _AmitkRoiClass
{
  AmitkVolumeClass parent_class;

  void (* roi_changed)      (AmitkRoi * roi);
  void (* roi_type_changed) (AmitkRoi * roi);

};



/* Application-level methods */

GType	        amitk_roi_get_type	          (void);
AmitkRoi *      amitk_roi_new                     (AmitkRoiType type);
GSList *        amitk_roi_get_intersection_line   (const AmitkRoi * roi, 
						   const AmitkVolume * canvas_slice,
						   const amide_real_t pixel_dim);
GSList *        amitk_roi_free_points_list        (GSList * list);
AmitkDataSet *  amitk_roi_get_intersection_slice  (const AmitkRoi * roi, 
						   const AmitkVolume * canvas_slice,
						   const amide_real_t pixel_dim
#ifndef AMIDE_LIBGNOMECANVAS_AA
						   , const gboolean fill_map_roi
#endif
						   );
void            amitk_roi_set_specify_color       (AmitkRoi * roi,
						   gboolean specify_color);
void            amitk_roi_set_color               (AmitkRoi * roi,
						   rgba_t color);
void            amitk_roi_set_voxel_size          (AmitkRoi * roi, 
						   AmitkPoint voxel_size);
void            amitk_roi_calc_far_corner         (AmitkRoi * roi);
void            amitk_roi_set_isocontour          (AmitkRoi * roi, 
						   AmitkDataSet * ds,
						   AmitkVoxel start_voxel,
						   amide_data_t isocontour_min_value,
						   amide_data_t isocontour_max_value,
						   AmitkRoiIsocontourRange isocontour_range);
void            amitk_roi_manipulate_area         (AmitkRoi * roi, 
						   gboolean erase,
						   AmitkVoxel erase_voxel, 
						   gint area_size);
AmitkPoint      amitk_roi_get_center_of_mass      (AmitkRoi * roi);
void            amitk_roi_set_type                (AmitkRoi * roi, AmitkRoiType new_type);
void            amitk_roi_calculate_on_data_set   (const AmitkRoi * roi,  
						   const AmitkDataSet * ds, 
						   const guint frame,
						   const guint gate,
						   const gboolean inverse,
						   const gboolean accurate,
						   void (* calculation)(),
						   gpointer data);
void            amitk_roi_erase_volume            (const AmitkRoi * roi, 
						   AmitkDataSet * ds,
						   const gboolean outside,
						   AmitkUpdateFunc update_func,
						   gpointer update_data);
const gchar *   amitk_roi_type_get_name           (const AmitkRoiType roi_type);

amide_real_t    amitk_rois_get_max_min_voxel_size (GList * objects);


/* external variables */
extern gchar * amitk_roi_menu_names[];
extern gchar * amitk_roi_menu_explanation[];

G_END_DECLS

#endif /* __AMITK_ROI_H__ */
