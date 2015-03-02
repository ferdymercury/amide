/* ui_study_rois_callbacks.c
 *
 * Part of amide - Amide's a Medical Image Dataset Viewer
 * Copyright (C) 2000 Andy Loening
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

#include "config.h"
#include <gnome.h>
#include <math.h>
#include "amide.h"
#include "realspace.h"
#include "volume.h"
#include "roi.h"
#include "study.h"
#include "color_table.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_study_rois.h"
#include "ui_study.h"
#include "ui_study_rois2.h"
#include "ui_study_rois_callbacks.h"


/* function called when an event occurs on the roi item 
   note, new roi's are handled by ui_study_callbacks_canvas_event*/
gint ui_study_rois_callbacks_roi_event(GtkWidget* widget, 
				       GdkEvent * event,
				       gpointer data) {

  ui_study_t * ui_study = data;
  ui_study_roi_list_t * temp_roi_list;
  realpoint_t real_loc, view_loc;
  view_t j;
  axis_t k;
  floatpoint_t view_width,view_height;
  realpoint_t item, diff;
  amide_volume_t * volume;
  GdkCursor * cursor;
  realpoint_t t[3]; /* temp variables */
  realpoint_t new_center, new_radius, view_center, view_radius;
  static realpoint_t center, radius;
  static view_t i;
  static realpoint_t initial_real_loc;
  static realpoint_t initial_view_loc;
  static realpoint_t last_pic_loc;
  static gboolean dragging = FALSE;
  static ui_study_roi_list_t * current_roi_list_item = NULL;
  static GnomeCanvasItem * roi_item = NULL;
  static double theta;
  static realpoint_t zoom;
  static realpoint_t shift;

  /* sanity checks */
  if (ui_study->current_mode == VOLUME_MODE) 
    return TRUE;
  if (ui_study->study->volumes == NULL)
    return TRUE;
  for (j=0; j<NUM_VIEWS; j++)
    if (ui_study->current_slice[j] == NULL)
      return TRUE;
  
  /* figure out which volume we're dealing with */
  if (ui_study->current_volume == NULL)
    volume = ui_study->study->volumes->volume;
  else
    volume = ui_study->current_volume;
		   
  /* get the location of the event, and convert it to the gnome image system 
     coordinates */
  item.x = event->button.x;
  item.y = event->button.y;
  gnome_canvas_item_w2i(GNOME_CANVAS_ITEM(widget)->parent, &item.x, &item.y);

  
  /* iterate through all the currently drawn roi's to figure out which
     roi this item corresponds to and what canvas it's on */
  temp_roi_list = ui_study->current_rois;
  if (roi_item == NULL)
    while (temp_roi_list != NULL) {
      for (j=0; j<NUM_VIEWS; j++) 
	if (((gpointer) (temp_roi_list->canvas_roi[j])) == ((gpointer) widget)) {
	  i = j;
	  current_roi_list_item = temp_roi_list;
	}
      temp_roi_list = temp_roi_list->next;
    }
      
  /* get some needed information */
  view_width = ui_study->current_slice[i]->voxel_size.x*
    ui_study->current_slice[i]->dim.x;
  view_height = ui_study->current_slice[i]->voxel_size.y*
    ui_study->current_slice[i]->dim.y;

  /* convert the event location to view space units */
  view_loc.x = ((item.x-UI_STUDY_TRIANGLE_HEIGHT)
		/ui_study->rgb_image[i]->rgb_width)*view_width;
  view_loc.y = ((item.y-UI_STUDY_TRIANGLE_HEIGHT)
		/ui_study->rgb_image[i]->rgb_height)*view_height;
  view_loc.z = ui_study->current_thickness/2.0;

  /* Convert the event location info to real units */
  real_loc = realspace_alt_coord_to_base(view_loc,
					 ui_study->current_slice[i]->coord_frame);

  /* switch on the event which called this */
  switch (event->type)
    {

    case GDK_ENTER_NOTIFY:
      
      cursor = ui_study->cursor[UI_STUDY_OLD_ROI_MODE];
      
      /* push our desired cursor onto the cursor stack */
      ui_study->cursor_stack = g_slist_prepend(ui_study->cursor_stack,cursor);
      gdk_window_set_cursor(gtk_widget_get_parent_window(GTK_WIDGET(ui_study->canvas[i])), cursor);
      
      break;


    case GDK_LEAVE_NOTIFY:
      /* pop the previous cursor off the stack */
      cursor = g_slist_nth_data(ui_study->cursor_stack, 0);
      ui_study->cursor_stack = g_slist_remove(ui_study->cursor_stack, cursor);
      cursor = g_slist_nth_data(ui_study->cursor_stack, 0);
      gdk_window_set_cursor(gtk_widget_get_parent_window(GTK_WIDGET(ui_study->canvas[i])), cursor);
      
      break;
      


    case GDK_BUTTON_PRESS:
      dragging = TRUE;

      /* last second sanity check */
      if (current_roi_list_item == NULL) {
	g_printerr("null current_roi_list_item?");
	return TRUE;
      }

      if (event->button.button == 1)
	cursor = ui_study->cursor[UI_STUDY_OLD_ROI_RESIZE];
      else if (event->button.button == 2)
	cursor = ui_study->cursor[UI_STUDY_OLD_ROI_ROTATE];
      else if (event->button.button == 3)
	cursor = ui_study->cursor[UI_STUDY_OLD_ROI_SHIFT];
      else /* what the hell button got pressed? */
	return TRUE;

      ui_study->cursor_stack = g_slist_prepend(ui_study->cursor_stack,cursor);
      gnome_canvas_item_grab(GNOME_CANVAS_ITEM(widget),
			     GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			     cursor,
			     event->button.time);

      /* save the roi we're going to manipulate for future use */
      roi_item = current_roi_list_item->canvas_roi[i];

      /* save some values in static variables for future use */
      initial_real_loc = real_loc;
      initial_view_loc = view_loc;
      last_pic_loc = item;
      theta = 0.0;
      zoom.x = zoom.y = zoom.z = 1.0;
      shift = realpoint_init;

      t[0] = 
	realspace_base_coord_to_alt(current_roi_list_item->roi->coord_frame.offset,
				    current_roi_list_item->roi->coord_frame);
      t[1] = current_roi_list_item->roi->corner;
      REALSPACE_MADD(0.5,t[1],0.5,t[0],center);
      REALSPACE_DIFF(t[1],t[0],radius);
      REALSPACE_CMULT(0.5,radius,radius);

      break;

    case GDK_MOTION_NOTIFY:
      if (dragging && 
	  ((event->motion.state && 
	    (GDK_BUTTON1_MASK || GDK_BUTTON2_MASK || GDK_BUTTON3_MASK)))) {

	/* some sanity checks */
	if ((view_loc.x < 0.0) | (view_loc.y < 0.0) | 
	    (view_loc.x > view_width) |
	    (view_loc.y > view_height))
	  return TRUE;

	/* last second sanity check */
	if (current_roi_list_item == NULL) {
	  g_printerr("null current_roi_list_item?");
	  return TRUE;
	}


	if (event->motion.state & GDK_BUTTON1_MASK) {
	  /* BUTTON1 Pressed, we're scaling the object */
	  /* note, I'd like to use the function "gnome_canvas_item_scale"
	     but this isn't defined in the current version of gnome.... 
	     so I'll have to do a whole bunch of shit*/
	  /* also, this "zoom" strategy won't work if the object is not
	     aligned with the view.... oh well... good enough for now... */
	  double affine[6];
	  realpoint_t item_center;

	  gnome_canvas_item_i2w_affine(GNOME_CANVAS_ITEM(roi_item),affine);

	  /* calculating the radius and center in view units */
	  view_radius = 
	    realspace_alt_dim_to_alt(radius,
				     current_roi_list_item->roi->coord_frame,
				     ui_study->current_slice[i]->coord_frame);
	  view_center = 
	    realspace_alt_coord_to_alt(center, 
				       current_roi_list_item->roi->coord_frame,
				       ui_study->current_slice[i]->coord_frame);
	  REALSPACE_SUB(view_loc,view_center,t[0]);
	  REALSPACE_SUB(initial_view_loc,view_center,t[1]);
	  
	  /* figure out what the center of the roi is in canvas_item coords */

	  item_center.x = ((view_center.x/view_width)
			   *ui_study->rgb_image[i]->rgb_width
			   +UI_STUDY_TRIANGLE_HEIGHT);
	  item_center.y = ((view_center.y/view_height)
			   *ui_study->rgb_image[i]->rgb_height
			   +UI_STUDY_TRIANGLE_HEIGHT);

	  zoom.x = (view_radius.x+t[0].x)/(view_radius.x+t[1].x);
	  zoom.y = (view_radius.x+t[0].y)/(view_radius.x+t[1].y);

  
	  affine[0] = zoom.x;
	  affine[3] = zoom.y;
	  affine[4] = (1.0-zoom.x)*item_center.x;
	  affine[5] = (1.0-zoom.y)*item_center.y;
	  gnome_canvas_item_affine_absolute(GNOME_CANVAS_ITEM(roi_item),affine);

	} else if (event->motion.state & GDK_BUTTON2_MASK) {
	  /* BUTTON2 Pressed, we're rotating the object */
	  /* note, I'd like to use the function "gnome_canvas_item_rotate"
	     but this isn't defined in the current version of gnome.... 
	     so I'll have to do a whole bunch of shit*/
	  double affine[6];
	  realpoint_t item_center;

	  gnome_canvas_item_i2w_affine(GNOME_CANVAS_ITEM(roi_item),affine);
	  view_center = 
	    realspace_alt_coord_to_alt(center,
				       current_roi_list_item->roi->coord_frame,
				       ui_study->current_slice[i]->coord_frame);
	  REALSPACE_SUB(initial_view_loc,view_center,t[0]);
	  REALSPACE_SUB(view_loc,view_center,t[1]);

	  /* figure out theta */
	  theta = acos(REALSPACE_DOT_PRODUCT(t[0],t[1])/
		       (REALPOINT_MAGNITUDE(t[0]) * REALPOINT_MAGNITUDE(t[1])));

	  /* figure out what the center of the roi is in canvas_item coords */
	  item_center.x = ((view_center.x/view_width)
			   *ui_study->rgb_image[i]->rgb_width
			   +UI_STUDY_TRIANGLE_HEIGHT);
	  item_center.y = ((view_center.y/view_height)
			   *ui_study->rgb_image[i]->rgb_height
			   +UI_STUDY_TRIANGLE_HEIGHT);
	  affine[0] = cos(theta);
	  affine[1] = sin(theta);
	  affine[2] = -affine[1];
	  affine[3] = affine[0];
	  affine[4] = (1.0-affine[0])*item_center.x+affine[1]*item_center.y;
	  affine[5] = (1.0-affine[3])*item_center.y+affine[2]*item_center.x;
	  gnome_canvas_item_affine_absolute(GNOME_CANVAS_ITEM(roi_item),affine);

	} else if (event->motion.state & GDK_BUTTON3_MASK) {
	  /* BUTTON3 pressed, we're shifting the object */
	  /* do movement calculations */
	  REALSPACE_SUB(real_loc,initial_real_loc,shift);
	  REALSPACE_SUB(item,last_pic_loc,diff);
	  gnome_canvas_item_i2w(GNOME_CANVAS_ITEM(widget)->parent, 
				&diff.x, &diff.y);
	  gnome_canvas_item_move(GNOME_CANVAS_ITEM(roi_item),diff.x,diff.y); 
	} else {
	  g_printerr("reached unsuspected condition in ui_study_rois_callbacks.c");
	  dragging = FALSE;
	  return TRUE;
	}
	last_pic_loc = item;
      }
      break;
      
    case GDK_BUTTON_RELEASE:
      gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget),
			       event->button.time);
      /* pop the previous cursor off the stack */
      cursor = g_slist_nth_data(ui_study->cursor_stack, 0);
      ui_study->cursor_stack = g_slist_remove(ui_study->cursor_stack, cursor);
      cursor = g_slist_nth_data(ui_study->cursor_stack, 0);
      gdk_window_set_cursor(gtk_widget_get_parent_window(GTK_WIDGET(ui_study->canvas[i])), cursor);
      dragging = FALSE;
      roi_item = NULL;

      /* last second sanity check */
      if (current_roi_list_item == NULL) {
	g_printerr("null current_roi_list_item?");
	return TRUE;
      }

      /* apply our changes to the roi */

      g_print("shift %5.3f %5.3f %5.3f\n",shift.x,shift.y,shift.z);
      g_print("zoom  %5.3f %5.3f %5.3f\n",zoom.x,zoom.y,zoom.z);
      g_print("theta %5.3f\n",theta);

      if (event->button.button == 3) {
	/* ------------- apply any shift done -------------- */
	REALSPACE_ADD(current_roi_list_item->roi->coord_frame.offset,
		      shift,
		      current_roi_list_item->roi->coord_frame.offset);
      } else if (event->button.button == 1) {
	/* ------------- apply any zoom done -------------- */
	
	/* subtracting 1.0 from the zoom, as we only want to translate the
	   change in zoom (i.e. away from 1.0) into the roi's coord frame*/
	t[2].x = t[2].y = t[2].z = 1.0;
	REALSPACE_SUB(zoom,t[2],zoom);
	zoom = /* get the zoom in roi coords */
	  realspace_alt_coord_to_base(zoom,
				      ui_study->current_slice[i]->coord_frame);
	REALSPACE_SUB(zoom, 
		      ui_study->current_slice[i]->coord_frame.offset,
		      zoom);
	REALSPACE_ADD(zoom, 
		      current_roi_list_item->roi->coord_frame.offset,
		      zoom);
	zoom = 
	  realspace_base_coord_to_alt(zoom,
				      current_roi_list_item->roi->coord_frame);
	REALSPACE_ADD(zoom,t[2],zoom);
	g_print("zoom  %5.3f %5.3f %5.3f\n",zoom.x,zoom.y,zoom.z);
	
	REALSPACE_MULT(zoom,radius,new_radius); /* apply the zoom */
	
	/* allright, figure out the new lower left corner */
	REALSPACE_SUB(center, new_radius, t[0]);
	
	/* the new roi offset will be the new lower left corner */
	current_roi_list_item->roi->coord_frame.offset =
	  realspace_alt_coord_to_base(t[0],
				      current_roi_list_item->roi->coord_frame);
	
	/* and the new upper right corner is simply twice the radius */
	REALSPACE_CMULT(2,new_radius,t[1]);
	current_roi_list_item->roi->corner = t[1];
	
      } else if (event->button.button == 2) {
	
	/* ------------- apply any rotation done -------------- */
	new_center = /* get the center in real coords */
	  realspace_alt_coord_to_base(center,
				      current_roi_list_item->roi->coord_frame);
	
	/* reset the offset of the roi coord_frame to the object's center */
	current_roi_list_item->roi->coord_frame.offset = new_center;
	
	/* now rotate the roi coord_frame axis */
	for (k=0;k<NUM_AXIS;k++)
	  current_roi_list_item->roi->coord_frame.axis[k] =
	    realspace_rotate_on_axis(&(current_roi_list_item->roi->coord_frame.axis[k]),
				     &(ui_study->current_slice[i]->coord_frame.axis[ZAXIS]),
				     theta);
	realspace_make_orthonormal(current_roi_list_item->roi->coord_frame.axis);
	
	/* and recaculate the offset */
	new_center = /* get the center in roi coords */
	  realspace_base_coord_to_alt(new_center,
				      current_roi_list_item->roi->coord_frame);
	REALSPACE_SUB(new_center,radius,t[0]);
	t[0] = /* get the new offset in real coords */
	  realspace_alt_coord_to_base(t[0],
				      current_roi_list_item->roi->coord_frame);
	current_roi_list_item->roi->coord_frame.offset = t[0]; 
	
      } else 
	return TRUE; /* shouldn't get here */
	
      /* update the roi */
      for (j=0;j<NUM_VIEWS;j++) {
	current_roi_list_item->canvas_roi[j] =
	  ui_study_rois_update_canvas_roi(ui_study,j,
					  current_roi_list_item->canvas_roi[j],
					  current_roi_list_item->roi);
      }
      
      break;
    default:
      break;
    }
      
  return FALSE;
}





void ui_study_rois_callbacks_calculate_all(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;

  ui_study_rois_calculate(ui_study, TRUE);

  return;

}

void ui_study_rois_callbacks_calculate_selected(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;

  ui_study_rois_calculate(ui_study, FALSE);

  return;
}


void ui_study_rois_callbacks_delete_event(GtkWidget* widget, 
					  GdkEvent * event, 
					  gpointer data) {

  /* ui_study_t * ui_study = data; */

  /* destroy the widget */
  gtk_widget_destroy(widget);

  return;
}







