/* amitk_canvas_object.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2002-2014 Andy Loening
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
#include "amitk_marshal.h"
#include "amitk_canvas_object.h"
#include "amitk_type_builtins.h"
#include "amitk_canvas.h"
#include "image.h"

#define FIDUCIAL_MARK_WIDTH 4.0
#define FIDUCIAL_MARK_WIDTH_PIXELS 2

#ifndef AMIDE_LIBGNOMECANVAS_AA
#define FIDUCIAL_MARK_LINE_STYLE GDK_LINE_SOLID
#endif

/* draws the given object on the given canvas.  */
/* if item is NULL, a new canvas item will be created */
/* pixel dim is the current dimensions of the pixels in the canvas */
GnomeCanvasItem * amitk_canvas_object_draw(GnomeCanvas * canvas, 
					   AmitkVolume * canvas_volume,
					   AmitkObject * object,
					   AmitkViewMode view_mode,
					   GnomeCanvasItem * item,
					   amide_real_t pixel_dim,
					   gint width, gint height,
					   gdouble x_offset, 
					   gdouble y_offset,
					   rgba_t roi_color,
					   gint roi_width,
#ifdef AMIDE_LIBGNOMECANVAS_AA
					   gdouble transparency
#else
					   GdkLineStyle line_style,
					   gboolean fill_roi
#endif
					   ) {

  guint32 fill_color_rgba;
  guint32 outline_color_rgba;
  gdouble affine[6];
  gboolean hide_object = FALSE;
  GnomeCanvasPoints * points;

  g_return_val_if_fail(GNOME_IS_CANVAS(canvas), item);
  g_return_val_if_fail(AMITK_IS_VOLUME(canvas_volume), item);
  g_return_val_if_fail(object != NULL, item);


  if (item != NULL) {
    /* make sure to reset any affine translations we've done */
    gnome_canvas_item_i2w_affine(item,affine);
    affine[0] = affine[3] = 1.0;
    affine[1] = affine[2] = affine[4] = affine[5] = 0.0;
    gnome_canvas_item_affine_absolute(item,affine);
  }

  if (AMITK_IS_FIDUCIAL_MARK(object)) {
    /* --------------------- redraw alignment point ----------------------------- */
    AmitkPoint center_point;
    AmitkCanvasPoint center_cpoint;
    rgba_t outline_color;

    if (AMITK_FIDUCIAL_MARK_SPECIFY_COLOR(object)) 
      outline_color = AMITK_FIDUCIAL_MARK_COLOR(object);
    else if (AMITK_IS_DATA_SET(AMITK_OBJECT_PARENT(object)))
      outline_color = 
	amitk_color_table_outline_color(amitk_data_set_get_color_table_to_use(AMITK_DATA_SET(AMITK_OBJECT_PARENT(object)), view_mode), TRUE);
    else
      outline_color = amitk_color_table_outline_color(AMITK_COLOR_TABLE_BW_LINEAR, TRUE);
    outline_color_rgba = amitk_color_table_rgba_to_uint32(outline_color);

    center_point = amitk_space_b2s(AMITK_SPACE(canvas_volume), 
				   AMITK_FIDUCIAL_MARK_GET(object));

    center_cpoint= point_2_canvas_point(AMITK_VOLUME_CORNER(canvas_volume),
					width, height, x_offset, y_offset, center_point);

    points = gnome_canvas_points_new(7);
    points->coords[0] = center_cpoint.x-FIDUCIAL_MARK_WIDTH;
    points->coords[1] = center_cpoint.y;
    points->coords[2] = center_cpoint.x;
    points->coords[3] = center_cpoint.y;
    points->coords[4] = center_cpoint.x;
    points->coords[5] = center_cpoint.y+FIDUCIAL_MARK_WIDTH;
    points->coords[6] = center_cpoint.x;
    points->coords[7] = center_cpoint.y;
    points->coords[8] = center_cpoint.x+FIDUCIAL_MARK_WIDTH;
    points->coords[9] = center_cpoint.y;
    points->coords[10] = center_cpoint.x;
    points->coords[11] = center_cpoint.y;
    points->coords[12] = center_cpoint.x;
    points->coords[13] = center_cpoint.y-FIDUCIAL_MARK_WIDTH;

    if (item == NULL)
      item = gnome_canvas_item_new(gnome_canvas_root(canvas),
				   gnome_canvas_line_get_type(), "points", points,
				   "fill_color_rgba", outline_color_rgba,
				   "width_pixels", FIDUCIAL_MARK_WIDTH_PIXELS, 
#ifndef AMIDE_LIBGNOMECANVAS_AA
    				   "line_style", FIDUCIAL_MARK_LINE_STYLE, 
#endif
				   NULL); 
    else
      gnome_canvas_item_set(item, "points", points,"fill_color_rgba", outline_color_rgba, NULL);
    gnome_canvas_points_unref(points);

    /* make sure the point is on this slice */
    hide_object = ((center_point.x < 0.0) || 
		   (center_point.x > AMITK_VOLUME_X_CORNER(canvas_volume)) ||
		   (center_point.y < 0.0) || 
		   (center_point.y > AMITK_VOLUME_Y_CORNER(canvas_volume)) ||
		   (center_point.z < 0.0) || 
		   (center_point.z > AMITK_VOLUME_Z_CORNER(canvas_volume)));


  } else if (AMITK_IS_ROI(object)) {
    /* --------------------- redraw roi ----------------------------- */
    GSList * roi_points, * temp;
    guint num_points, j;
    AmitkCanvasPoint roi_cpoint;
    GdkPixbuf * pixbuf;
    AmitkCanvasPoint offset_cpoint;
    AmitkCanvasPoint corner_cpoint;
    AmitkRoi * roi = AMITK_ROI(object);
    AmitkPoint offset, corner;
    AmitkPoint * ptemp_rp;
    

    if (AMITK_ROI_UNDRAWN(object)) return item;

    /* overwrite the passed in color if desired */
    if (AMITK_ROI_SPECIFY_COLOR(object))
      roi_color = AMITK_ROI_COLOR(object);

    switch(AMITK_ROI_TYPE(roi)) {
    case AMITK_ROI_TYPE_ISOCONTOUR_2D:
    case AMITK_ROI_TYPE_ISOCONTOUR_3D:
    case AMITK_ROI_TYPE_FREEHAND_2D:
    case AMITK_ROI_TYPE_FREEHAND_3D:

      offset = zero_point;
      corner = one_point;
      pixbuf = image_slice_intersection(roi, canvas_volume, pixel_dim, 
#ifdef AMIDE_LIBGNOMECANVAS_AA
					transparency,
#else
					fill_roi,
#endif
					roi_color,&offset, &corner);
      
      offset_cpoint= point_2_canvas_point(AMITK_VOLUME_CORNER(canvas_volume),
					  width, height, x_offset, y_offset, 
					  amitk_space_b2s(AMITK_SPACE(canvas_volume), offset));
      corner_cpoint= point_2_canvas_point(AMITK_VOLUME_CORNER(canvas_volume),
					  width, height, x_offset, y_offset, 
					  amitk_space_b2s(AMITK_SPACE(canvas_volume), corner));
					  
      /* find the north west corner (in terms of the X reference frame) */
      if (corner_cpoint.y < offset_cpoint.y) offset_cpoint.y = corner_cpoint.y;
      if (corner_cpoint.x < offset_cpoint.x) offset_cpoint.x = corner_cpoint.x;
      
      /* create the item */ 
      if (item == NULL) {
	item =  gnome_canvas_item_new(gnome_canvas_root(canvas),
				      gnome_canvas_pixbuf_get_type(), "pixbuf", pixbuf,
				      "x", (double) offset_cpoint.x, "y", (double) offset_cpoint.y, NULL);
      } else {
	gnome_canvas_item_set(item, "pixbuf", pixbuf, 
			      "x", (double) offset_cpoint.x, "y", (double) offset_cpoint.y, NULL);
      }
      if (pixbuf != NULL)
	g_object_unref(pixbuf);
      break;

    case AMITK_ROI_TYPE_ELLIPSOID:
    case AMITK_ROI_TYPE_CYLINDER:
    case AMITK_ROI_TYPE_BOX:
      roi_points =  
	amitk_roi_get_intersection_line(roi, canvas_volume, pixel_dim);
    
      /* count the points */
      num_points=0;
      temp=roi_points;
      while(temp!=NULL) {
	temp=temp->next;
	num_points++;
      }
    
      /* transfer the points list to what we'll be using to construction the figure */
      if (num_points > 1) {
	points = gnome_canvas_points_new(num_points);
	temp=roi_points;
	j=0;
	while(temp!=NULL) {
	  ptemp_rp = temp->data;
	  roi_cpoint= point_2_canvas_point(AMITK_VOLUME_CORNER(canvas_volume),
					   width, height, x_offset, y_offset, *ptemp_rp);
	  points->coords[j] = roi_cpoint.x;
	  points->coords[j+1] = roi_cpoint.y;
	  temp=temp->next;
	  j += 2;
	}
      } else {
	/* throw in junk we'll hide*/
	hide_object = TRUE;
	points = gnome_canvas_points_new(3);
	points->coords[0] = points->coords[1] = 0;
	points->coords[2] = points->coords[3] = 1;
	points->coords[4] = 0;
	points->coords[5] = 1;
      }
      roi_points = amitk_roi_free_points_list(roi_points);
#ifdef AMIDE_LIBGNOMECANVAS_AA
      outline_color_rgba = amitk_color_table_rgba_to_uint32(roi_color);
      roi_color.a = transparency * 0xFF;
#endif
      fill_color_rgba = amitk_color_table_rgba_to_uint32(roi_color);

      if ((item == NULL)) {   /* create the item */ 
	item =  gnome_canvas_item_new(gnome_canvas_root(canvas),
#ifdef AMIDE_LIBGNOMECANVAS_AA
				      gnome_canvas_polygon_get_type(), 
#else
				      gnome_canvas_line_get_type(), 
#endif
				      "points", points,
				      "fill_color_rgba",fill_color_rgba,
				      "width_pixels", roi_width, 
#ifdef AMIDE_LIBGNOMECANVAS_AA
				      "outline_color_rgba", outline_color_rgba,
#else
				      "line_style", line_style, 
#endif
				      NULL);
      } else {
	/* and reset the line points */
	gnome_canvas_item_set(item, "points", points, 
			      "fill_color_rgba", fill_color_rgba,
			      "width_pixels", roi_width, 
#ifdef AMIDE_LIBGNOMECANVAS_AA
			      "outline_color_rgba", outline_color_rgba,
#else
			      "line_style", line_style,  
#endif
			      NULL);
      }
      gnome_canvas_points_unref(points);
      break;
    default:
      g_error("unexpected case in %s at %d\n", __FILE__, __LINE__);
      break;
    }
  }
 
  /* make sure the point is on this canvas */
  if (hide_object)
    gnome_canvas_item_hide(item);
  else if (item)
    gnome_canvas_item_show(item);

  return item;
}




 
