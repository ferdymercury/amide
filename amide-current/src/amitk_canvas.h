/* amitk_canvas.h
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


#ifndef __AMITK_CANVAS_H__
#define __AMITK_CANVAS_H__

/* includes we always need with this widget */
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gnome.h>
#include "volume.h"
#include "roi.h"
#include "align_pt.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define AMITK_TYPE_CANVAS            (amitk_canvas_get_type ())
#define AMITK_CANVAS(obj)            (GTK_CHECK_CAST ((obj), AMITK_TYPE_CANVAS, AmitkCanvas))
#define AMITK_CANVAS_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), AMITK_TYPE_CANVAS, AmitkCanvasClass))
#define AMITK_IS_CANVAS(obj)         (GTK_CHECK_TYPE ((obj), AMITK_TYPE_CANVAS))
#define AMITK_IS_CANVAS_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_CANVAS))

#define AMITK_CANVAS_VIEW(obj)       (AMITK_CANVAS(obj)->view)



typedef enum {
  AMITK_CANVAS_CROSS_HIDE,
  AMITK_CANVAS_CROSS_SHOW,
  NUM_AMITK_CANVAS_CROSS_ACTIONS
} amitk_canvas_cross_action_t;

typedef struct _AmitkCanvas             AmitkCanvas;
typedef struct _AmitkCanvasClass        AmitkCanvasClass;


struct _AmitkCanvas
{
  GtkVBox vbox;

  GtkWidget * canvas;
  GtkWidget * label;
  GtkWidget * scrollbar;
  GtkObject * scrollbar_adjustment;
  GnomeCanvasItem * cross[4];
  GnomeCanvasItem * arrows[4];

  realspace_t * coord_frame;
  realpoint_t corner;
  realpoint_t center; /* in base coord frame */
  floatpoint_t thickness;
  floatpoint_t voxel_dim;
  floatpoint_t zoom;
  amide_time_t start_time;
  amide_time_t duration;
  interpolation_t interpolation;

  view_t view;
  layout_t layout;
  gint roi_width;
  GdkLineStyle line_style;
  color_table_t color_table;

  volumes_t * volumes;
  volumes_t * slices;
  realpoint_t voxel_size;
  GdkPixbuf * pixbuf;
  gint pixbuf_width, pixbuf_height;
  GnomeCanvasItem * image;

  rois_t * rois;
  GList * roi_items;

  align_pts_t * pts;
  GList * pt_items;

};

struct _AmitkCanvasClass
{
  GtkVBoxClass parent_class;
  
  void (* help_event)                (AmitkCanvas *Canvas,
				      help_info_t which_help,
				      realpoint_t *position,
				      gfloat      value);
  void (* view_changing)             (AmitkCanvas *Canvas,
				      realpoint_t *position,
				      gfloat thickness);
  void (* view_changed)              (AmitkCanvas *Canvas,
				      realpoint_t *position,
				      gfloat thickness);
  void (* view_z_position_changed)   (AmitkCanvas *Canvas,
				      realpoint_t *position);
  void (* volumes_changed)           (AmitkCanvas *Canvas);
  void (* roi_changed)               (AmitkCanvas *Canvas,
				      roi_t       *roi);
  void (* isocontour_3d_changed)     (AmitkCanvas *Canvas,
				      roi_t       *roi,
				      realpoint_t *position);
  void (* new_align_pt)              (AmitkCanvas *Canvas,
				      realpoint_t *position);
};  


GtkType       amitk_canvas_get_type           (void);
GtkWidget*    amitk_canvas_new                (view_t view, 
					       layout_t layout,
					       realspace_t * view_coord_frame, 
					       realpoint_t center,
					       floatpoint_t thickness,
					       floatpoint_t voxel_dim,
					       floatpoint_t zoom,
					       amide_time_t start_time,
					       amide_time_t duration,
					       interpolation_t interpolation,
					       color_table_t color_table,
					       GdkLineStyle line_style,
					       gint roi_width);
void          amitk_canvas_set_layout         (AmitkCanvas * canvas, 
					       layout_t new_layout,
					       realspace_t * view_coord_frame);
void          amitk_canvas_set_coord_frame    (AmitkCanvas * canvas, 
					       realspace_t * view_coord_frame,
					       gboolean update_now);
void          amitk_canvas_set_center         (AmitkCanvas * canvas, 
					       realpoint_t center,
					       gboolean update_now);
void          amitk_canvas_set_thickness      (AmitkCanvas * canvas, 
					       floatpoint_t thickness,
					       gboolean update_now);
void          amitk_canvas_set_voxel_dim      (AmitkCanvas * canvas, 
					       floatpoint_t base_voxel_dim,
					       gboolean update_now);
void          amitk_canvas_set_zoom           (AmitkCanvas * canvas, 
					       floatpoint_t zoom,
					       gboolean update_now);
void          amitk_canvas_set_start_time     (AmitkCanvas * canvas, 
					       amide_time_t start_time,
					       gboolean update_now);
void          amitk_canvas_set_duration       (AmitkCanvas * canvas, 
					       amide_time_t duration,
					       gboolean update_now);
void          amitk_canvas_set_interpolation  (AmitkCanvas * canvas, 
					       interpolation_t interpolation,
					       gboolean update_now);
void          amitk_canvas_set_color_table    (AmitkCanvas * canvas, 
					       color_table_t color_table,
					       gboolean update_now);
void          amitk_canvas_set_line_style     (AmitkCanvas * canvas, 
					       GdkLineStyle new_line_style,
					       gboolean update_now);
void          amitk_canvas_set_roi_width      (AmitkCanvas * canvas, 
					       gint new_roi_width,
					       gboolean update_now);
void          amitk_canvas_add_object         (AmitkCanvas * canvas, 
					       gpointer object, 
					       object_t type,
					       gpointer parent_object);
void          amitk_canvas_remove_object      (AmitkCanvas * canvas, 
					       gpointer object, 
					       object_t type,
					       gboolean update_now);
void          amitk_canvas_update_object      (AmitkCanvas * canvas, gpointer object, object_t type);
void          amitk_canvas_threshold_changed  (AmitkCanvas * canvas);
void          amitk_canvas_update_scrollbar   (AmitkCanvas * canvas);
void          amitk_canvas_update_arrows      (AmitkCanvas * canvas);
void          amitk_canvas_update_cross       (AmitkCanvas * canvas, 
					       amitk_canvas_cross_action_t action, 
					       realpoint_t center, 
					       rgba_t outline_color, 
					       floatpoint_t thickness);
void          amitk_canvas_update_rois        (AmitkCanvas * canvas);
void          amitk_canvas_update_volumes     (AmitkCanvas * canvas);
void          amitk_canvas_update_align_pts   (AmitkCanvas * canvas);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __AMITK_CANVAS_H__ */

