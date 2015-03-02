/* amitk_canvas.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2002 Andy Loening
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
#include <gdk-pixbuf/gnome-canvas-pixbuf.h>
#include <math.h>
#include "amitk_canvas.h"
#include "image.h"
#include "ui_common.h"

#define BOX_SPACING 3
#define CANVAS_BLANK_WIDTH 128
#define CANVAS_BLANK_HEIGHT 128
#define CANVAS_TRIANGLE_WIDTH 13.0
#define CANVAS_TRIANGLE_HEIGHT 8.0
#define CANVAS_ALIGN_PT_WIDTH 4.0
#define CANVAS_ALIGN_PT_WIDTH_PIXELS 2
#define CANVAS_ALIGN_PT_LINE_STYLE GDK_LINE_SOLID

typedef enum {
  UPDATE_ARROWS, 
  REFRESH_VOLUMES,
  UPDATE_VOLUMES, 
  UPDATE_SCROLLBAR, 
  UPDATE_ROIS,
  UPDATE_ALIGN_PTS,
  UPDATE_ALL
} update_t;


enum {
  HELP_EVENT,
  VIEW_CHANGING,
  VIEW_CHANGED,
  VIEW_Z_POSITION_CHANGED,
  VOLUMES_CHANGED,
  ROI_CHANGED,
  ISOCONTOUR_3D_CHANGED,
  NEW_ALIGN_PT,
  LAST_SIGNAL
} amitk_canvas_signals;

typedef enum {
  CANVAS_EVENT_ENTER_VOLUME, 
  CANVAS_EVENT_ENTER_ALIGN_PT,
  CANVAS_EVENT_ENTER_NEW_NORMAL_ROI,
  CANVAS_EVENT_ENTER_NEW_ISOCONTOUR_ROI,
  CANVAS_EVENT_ENTER_NORMAL_ROI,
  CANVAS_EVENT_ENTER_ISOCONTOUR_ROI,
  CANVAS_EVENT_LEAVE, 
  CANVAS_EVENT_PRESS_MOVE_VIEW,
  CANVAS_EVENT_PRESS_MINIMIZE_VIEW,
  CANVAS_EVENT_PRESS_RESIZE_VIEW,
  CANVAS_EVENT_PRESS_ALIGN_HORIZONTAL, /* 10 */
  CANVAS_EVENT_PRESS_ALIGN_VERTICAL,
  CANVAS_EVENT_PRESS_MOVE_ALIGN_PT,
  CANVAS_EVENT_PRESS_NEW_ROI,
  CANVAS_EVENT_PRESS_SHIFT_ROI,
  CANVAS_EVENT_PRESS_ROTATE_ROI,
  CANVAS_EVENT_PRESS_RESIZE_ROI,
  CANVAS_EVENT_PRESS_ERASE_ISOCONTOUR,
  CANVAS_EVENT_PRESS_LARGE_ERASE_ISOCONTOUR,
  CANVAS_EVENT_PRESS_CHANGE_ISOCONTOUR,
  CANVAS_EVENT_PRESS_NEW_ALIGN_PT, /* 20 */
  CANVAS_EVENT_MOTION, 
  CANVAS_EVENT_MOTION_MOVE_VIEW,
  CANVAS_EVENT_MOTION_MINIMIZE_VIEW,
  CANVAS_EVENT_MOTION_RESIZE_VIEW,
  CANVAS_EVENT_MOTION_ALIGN_HORIZONTAL,
  CANVAS_EVENT_MOTION_ALIGN_VERTICAL, 
  CANVAS_EVENT_MOTION_ALIGN_PT,
  CANVAS_EVENT_MOTION_NEW_ROI,
  CANVAS_EVENT_MOTION_SHIFT_ROI,
  CANVAS_EVENT_MOTION_ROTATE_ROI, /* 30 */
  CANVAS_EVENT_MOTION_RESIZE_ROI, 
  CANVAS_EVENT_MOTION_ERASE_ISOCONTOUR,
  CANVAS_EVENT_MOTION_LARGE_ERASE_ISOCONTOUR,
  CANVAS_EVENT_MOTION_CHANGE_ISOCONTOUR, 
  CANVAS_EVENT_RELEASE_MOVE_VIEW,
  CANVAS_EVENT_RELEASE_MINIMIZE_VIEW,
  CANVAS_EVENT_RELEASE_RESIZE_VIEW,
  CANVAS_EVENT_RELEASE_ALIGN_HORIZONTAL,
  CANVAS_EVENT_RELEASE_ALIGN_VERTICAL, 
  CANVAS_EVENT_RELEASE_ALIGN_PT,
  CANVAS_EVENT_RELEASE_NEW_ROI,
  CANVAS_EVENT_RELEASE_SHIFT_ROI,
  CANVAS_EVENT_RELEASE_ROTATE_ROI,
  CANVAS_EVENT_RELEASE_RESIZE_ROI, 
  CANVAS_EVENT_RELEASE_ERASE_ISOCONTOUR,
  CANVAS_EVENT_RELEASE_LARGE_ERASE_ISOCONTOUR,
  CANVAS_EVENT_RELEASE_CHANGE_ISOCONTOUR,
  CANVAS_EVENT_CANCEL_ALIGN_HORIZONTAL,
  CANVAS_EVENT_CANCEL_ALIGN_VERTICAL,
  CANVAS_EVENT_ENACT_ALIGN_HORIZONTAL,
  CANVAS_EVENT_ENACT_ALIGN_VERTICAL,
  CANVAS_EVENT_DO_NOTHING
} canvas_event_t;


typedef void (*AmitkSignal_NONE__ENUM_POINTER_FLOAT) (GtkObject * object,
						      gint arg1, 
						      gpointer arg2,
						      gfloat arg3, 
						      gpointer user_data);
void amitk_marshal_NONE__ENUM_POINTER_FLOAT (GtkObject * object,
					    GtkSignalFunc func, 
					    gpointer func_data, 
					    GtkArg * args) {
  AmitkSignal_NONE__ENUM_POINTER_FLOAT rfunc;
  rfunc = (AmitkSignal_NONE__ENUM_POINTER_FLOAT) func;
  (*rfunc) (object, 
	    GTK_VALUE_ENUM (args[0]), 
	    GTK_VALUE_POINTER(args[1]),
	    GTK_VALUE_FLOAT(args[2]),
	    func_data);
}

typedef void (*AmitkSignal_NONE__FLOAT) (GtkObject * object,
					 gfloat arg1, 
					 gpointer user_data);
void amitk_marshal_NONE__FLOAT (GtkObject * object,
				GtkSignalFunc func, 
				gpointer func_data, 
				GtkArg * args) {
  AmitkSignal_NONE__FLOAT rfunc;
  rfunc = (AmitkSignal_NONE__FLOAT) func;
  (*rfunc) (object, 
	    GTK_VALUE_FLOAT(args[0]),
	    func_data);
}

typedef void (*AmitkSignal_NONE__POINTER_FLOAT) (GtkObject * object,
						 gpointer arg1,
						 gfloat arg2, 
						 gpointer user_data);
void amitk_marshal_NONE__POINTER_FLOAT (GtkObject * object,
					GtkSignalFunc func, 
					gpointer func_data, 
					GtkArg * args) {
  AmitkSignal_NONE__POINTER_FLOAT rfunc;
  rfunc = (AmitkSignal_NONE__POINTER_FLOAT) func;
  (*rfunc) (object, 
	    GTK_VALUE_POINTER(args[0]),
	    GTK_VALUE_FLOAT(args[1]),
	    func_data);
}


static void canvas_class_init (AmitkCanvasClass *klass);
static void canvas_init (AmitkCanvas *canvas);
static void canvas_destroy(GtkObject * object);

static gboolean canvas_event_cb(GtkWidget* widget,  GdkEvent * event, gpointer data);
static void canvas_scrollbar_cb(GtkObject * adjustment, gpointer data);

static void canvas_update_scrollbar(AmitkCanvas * canvas, realpoint_t center, floatpoint_t thickness);
static void canvas_update_cross(AmitkCanvas * canvas, amitk_canvas_cross_action_t action, 
				realpoint_t center, rgba_t outline_color, floatpoint_t thickness);
static void canvas_update_arrows(AmitkCanvas * canvas, realpoint_t center, floatpoint_t thickness);
static void canvas_update_pixbuf(AmitkCanvas * canvas);
static GnomeCanvasItem * canvas_update_align_pt(AmitkCanvas * canvas, 
						GnomeCanvasItem * pt_item,
						align_pt_t * align_pt,
						volume_t * parent_vol);
static void canvas_update_align_pts(AmitkCanvas * canvas);
static GnomeCanvasItem * canvas_update_roi(AmitkCanvas * canvas, 
					   GnomeCanvasItem * roi_item,
					   roi_t * roi);
static void canvas_update_rois(AmitkCanvas * canvas);
static void canvas_update_setup(AmitkCanvas * canvas);
static void canvas_update(AmitkCanvas * canvas, update_t update);


static GtkVBox *canvas_parent_class = NULL;
static guint canvas_signals[LAST_SIGNAL] = {0};


GtkType amitk_canvas_get_type (void) {

  static GtkType canvas_type = 0;

  if (!canvas_type)
    {
      static const GtkTypeInfo canvas_info =
      {
	"AmitkCanvas",
	sizeof (AmitkCanvas),
	sizeof (AmitkCanvasClass),
	(GtkClassInitFunc) canvas_class_init,
	(GtkObjectInitFunc) canvas_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      canvas_type = gtk_type_unique (GTK_TYPE_VBOX, &canvas_info);
    }

  return canvas_type;
}

static void canvas_class_init (AmitkCanvasClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) klass;
  widget_class = (GtkWidgetClass*) klass;

  canvas_parent_class = gtk_type_class (GTK_TYPE_VBOX);

  canvas_signals[HELP_EVENT] =
    gtk_signal_new ("help_event",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (AmitkCanvasClass, help_event),
		    amitk_marshal_NONE__ENUM_POINTER_FLOAT,
		    GTK_TYPE_NONE, 3,
		    GTK_TYPE_ENUM,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_FLOAT);
  canvas_signals[VIEW_CHANGING] =
    gtk_signal_new ("view_changing",
  		    GTK_RUN_FIRST,
  		    object_class->type,
  		    GTK_SIGNAL_OFFSET (AmitkCanvasClass, view_changing),
  		    amitk_marshal_NONE__POINTER_FLOAT, 
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_FLOAT);
  canvas_signals[VIEW_CHANGED] =
    gtk_signal_new ("view_changed",
  		    GTK_RUN_FIRST,
  		    object_class->type,
  		    GTK_SIGNAL_OFFSET (AmitkCanvasClass, view_changed),
  		    amitk_marshal_NONE__POINTER_FLOAT, 
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_FLOAT);
  canvas_signals[VIEW_Z_POSITION_CHANGED] =
    gtk_signal_new ("view_z_position_changed",
  		    GTK_RUN_FIRST,
  		    object_class->type,
  		    GTK_SIGNAL_OFFSET (AmitkCanvasClass, view_z_position_changed),
  		    gtk_marshal_NONE__POINTER, 
		    GTK_TYPE_NONE, 1,
		    GTK_TYPE_POINTER);
  canvas_signals[VOLUMES_CHANGED] =
    gtk_signal_new ("volumes_changed",
  		    GTK_RUN_FIRST,
  		    object_class->type,
  		    GTK_SIGNAL_OFFSET (AmitkCanvasClass, volumes_changed),
  		    gtk_marshal_NONE__NONE, 
		    GTK_TYPE_NONE, 0);
  canvas_signals[ROI_CHANGED] =
    gtk_signal_new ("roi_changed",
  		    GTK_RUN_FIRST,
  		    object_class->type,
  		    GTK_SIGNAL_OFFSET (AmitkCanvasClass, roi_changed),
  		    gtk_marshal_NONE__POINTER, 
		    GTK_TYPE_NONE, 1,
		    GTK_TYPE_POINTER);
  canvas_signals[ISOCONTOUR_3D_CHANGED] =
    gtk_signal_new ("isocontour_3d_changed",
  		    GTK_RUN_FIRST,
  		    object_class->type,
  		    GTK_SIGNAL_OFFSET (AmitkCanvasClass, isocontour_3d_changed),
  		    gtk_marshal_NONE__POINTER_POINTER, 
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER);
  canvas_signals[NEW_ALIGN_PT] =
    gtk_signal_new ("new_align_pt",
  		    GTK_RUN_FIRST,
  		    object_class->type,
  		    GTK_SIGNAL_OFFSET (AmitkCanvasClass, new_align_pt),
  		    gtk_marshal_NONE__POINTER, 
		    GTK_TYPE_NONE, 1,
		    GTK_TYPE_POINTER);
  gtk_object_class_add_signals(object_class, canvas_signals, LAST_SIGNAL);

  object_class->destroy = canvas_destroy;

}

static void canvas_init (AmitkCanvas *canvas)
{
  gint i;

  gtk_box_set_homogeneous(GTK_BOX(canvas), FALSE);

  /* initialize some critical stuff */
  for (i=0;i<4;i++) {
    canvas->cross[i]=NULL;
    canvas->arrows[i]=NULL;
  }

  canvas->coord_frame=NULL;
  canvas->color_table = BW_LINEAR;

  canvas->canvas = NULL;
  canvas->volumes=NULL;
  canvas->slices=NULL;
  canvas->voxel_size = one_rp;
  canvas->corner = one_rp;
  canvas->pixbuf=NULL;
  canvas->image=NULL;

  canvas->rois=NULL;
  canvas->roi_items=NULL;

  canvas->pts=NULL;
  canvas->pt_items=NULL;

}

static void canvas_destroy (GtkObject * object) {

  AmitkCanvas * canvas;

  g_return_if_fail (object != NULL);
  g_return_if_fail (AMITK_IS_CANVAS (object));

  canvas = AMITK_CANVAS(object);

  canvas->coord_frame = rs_unref(canvas->coord_frame);

  canvas->volumes=volumes_unref(canvas->volumes);
  canvas->slices=volumes_unref(canvas->slices);
  canvas->rois=rois_unref(canvas->rois);
  canvas->pts=align_pts_unref(canvas->pts);
  

  if (GTK_OBJECT_CLASS (canvas_parent_class)->destroy)
    (* GTK_OBJECT_CLASS (canvas_parent_class)->destroy) (object);
}





/* this function converts a gnome canvas event location to a location in the canvas's coord frame */
static realpoint_t cp_2_rp(AmitkCanvas * canvas, canvaspoint_t canvas_cp) {

  realpoint_t canvas_rp;

  canvas_rp.x = ((canvas_cp.x-CANVAS_TRIANGLE_HEIGHT)/canvas->pixbuf_width)*canvas->corner.x;
  canvas_rp.y = ((canvas->pixbuf_height-(canvas_cp.y-CANVAS_TRIANGLE_HEIGHT))/
		 canvas->pixbuf_height)*canvas->corner.y;
  canvas_rp.z = canvas->thickness/2.0;

  return canvas_rp;
}


/* this function converts a point in the canvas's coord frame to a gnome canvas event location */
static canvaspoint_t rp_2_cp(AmitkCanvas * canvas, realpoint_t canvas_rp) {

  canvaspoint_t canvas_cp;

  canvas_cp.x = canvas->pixbuf_width * canvas_rp.x/canvas->corner.x + CANVAS_TRIANGLE_HEIGHT;
  canvas_cp.y = canvas->pixbuf_height * (canvas->corner.y - canvas_rp.y)/
    canvas->corner.y + CANVAS_TRIANGLE_HEIGHT;
  
  return canvas_cp;
}




/* function called when an event occurs on the canvas */
static gboolean canvas_event_cb(GtkWidget* widget,  GdkEvent * event, gpointer data) {

  AmitkCanvas * canvas = data;
  realpoint_t base_rp, canvas_rp, diff_rp;
  canvaspoint_t canvas_cp, diff_cp, event_cp;
  rgba_t outline_color;
  GnomeCanvasPoints * points;
  canvas_event_t canvas_event_type;
  object_t type;
  roi_t * roi;
  volume_t * volume;
  align_pt_t * align_pt;
  canvaspoint_t temp_cp[2];
  realpoint_t temp_rp[2];
  realpoint_t shift;
  volumes_t * volumes;
  volume_t * temp_volume;
  voxelpoint_t temp_vp;
  roi_t * undrawn_roi;
  static GnomeCanvasItem * canvas_item = NULL;
  static gboolean grab_on = FALSE;
  static gboolean extended_mode = FALSE;
  static gboolean align_vertical = FALSE;
  static canvaspoint_t initial_cp;
  static canvaspoint_t previous_cp;
  static realpoint_t initial_base_rp;
  static realpoint_t initial_canvas_rp;
  static double theta;
  static realpoint_t roi_zoom;
  static realpoint_t roi_center_rp;
  static realpoint_t roi_radius_rp;
  static gboolean in_object=FALSE;

  /* sanity checks */
  if (canvas->volumes == NULL) return TRUE;

  /* figure out which type of object */ 
  type = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "type"));

  switch(type) {
  case ROI:
    roi = gtk_object_get_data(GTK_OBJECT(widget), "object");
    volume = NULL;
    align_pt = NULL;
    break;
  case VOLUME:
    roi = NULL;
    volume = NULL;
    align_pt = NULL;
    break;
  case ALIGN_PT:
    roi = NULL;
    align_pt = gtk_object_get_data(GTK_OBJECT(widget), "object");
    volume = gtk_object_get_data(GTK_OBJECT(widget), "parent_object");
    break;
  default:
    roi = NULL;
    align_pt = NULL;
    volume = NULL;
    g_warning("%s: unexpected case in %s at line %d", PACKAGE, __FILE__, __LINE__);
    break;
  }

  undrawn_roi = rois_undrawn_roi(canvas->rois);

  switch(event->type) {

  case GDK_ENTER_NOTIFY:
    event_cp.x = event->crossing.x;
    event_cp.y = event->crossing.y;
    if (event->crossing.mode != GDK_CROSSING_NORMAL) {
      canvas_event_type = CANVAS_EVENT_DO_NOTHING; /* ignore grabs */

    } else if (undrawn_roi != NULL) {

      if ((undrawn_roi->type == ISOCONTOUR_2D) || (undrawn_roi->type == ISOCONTOUR_3D)) 
	canvas_event_type = CANVAS_EVENT_ENTER_NEW_ISOCONTOUR_ROI;      
      else canvas_event_type = CANVAS_EVENT_ENTER_NEW_NORMAL_ROI;      
      
    } else if (extended_mode) {
      canvas_event_type = CANVAS_EVENT_DO_NOTHING;

    } else if (type == VOLUME) {
      canvas_event_type = CANVAS_EVENT_ENTER_VOLUME;
      
    } else if (type == ALIGN_PT) {
      canvas_event_type = CANVAS_EVENT_ENTER_ALIGN_PT;

    } else if (roi != NULL) { /* sanity check */
      if ((roi->type == ISOCONTOUR_2D) || (roi->type == ISOCONTOUR_3D))
	canvas_event_type = CANVAS_EVENT_ENTER_ISOCONTOUR_ROI;
      else  canvas_event_type = CANVAS_EVENT_ENTER_NORMAL_ROI;
      
    } else
      canvas_event_type = CANVAS_EVENT_DO_NOTHING;
    
    break;
    
  case GDK_LEAVE_NOTIFY:
    event_cp.x = event->crossing.x;
    event_cp.y = event->crossing.y;
    if (event->crossing.mode != GDK_CROSSING_NORMAL)
      canvas_event_type = CANVAS_EVENT_DO_NOTHING; /* ignore grabs */
    else if (extended_mode)
      canvas_event_type = CANVAS_EVENT_DO_NOTHING; 
    else
      canvas_event_type = CANVAS_EVENT_LEAVE;
    break;
    
  case GDK_BUTTON_PRESS:
    event_cp.x = event->button.x;
    event_cp.y = event->button.y;

    if (undrawn_roi != NULL) {
      canvas_event_type = CANVAS_EVENT_PRESS_NEW_ROI;
      
    } else if (extended_mode) {
      if (align_vertical) canvas_event_type = CANVAS_EVENT_PRESS_ALIGN_VERTICAL;
      else canvas_event_type = CANVAS_EVENT_PRESS_ALIGN_HORIZONTAL;
      
    } else if ((!grab_on) && (!in_object) && (type == VOLUME) && (event->button.button == 1)) {
      if (event->button.state & GDK_SHIFT_MASK) 
	canvas_event_type = CANVAS_EVENT_PRESS_ALIGN_HORIZONTAL;
      else canvas_event_type = CANVAS_EVENT_PRESS_MOVE_VIEW;
      
    } else if ((!grab_on) && (!in_object) && (type == VOLUME) && (event->button.button == 2)) {
      if (event->button.state & GDK_SHIFT_MASK) 
	canvas_event_type = CANVAS_EVENT_PRESS_ALIGN_VERTICAL;
      else canvas_event_type = CANVAS_EVENT_PRESS_MINIMIZE_VIEW;
      
    } else if ((!grab_on) && (!in_object) && (type == VOLUME) && (event->button.button == 3)) {
      if (event->button.state & GDK_SHIFT_MASK) 
	canvas_event_type = CANVAS_EVENT_PRESS_NEW_ALIGN_PT;
      else canvas_event_type = CANVAS_EVENT_PRESS_RESIZE_VIEW;
      
    } else if ((!grab_on) && (type == ROI) && (event->button.button == 1)) {
      canvas_event_type = CANVAS_EVENT_PRESS_SHIFT_ROI;
      
    } else if ((!grab_on) && (type == ROI) && (event->button.button == 2)) {
      if ((roi->type != ISOCONTOUR_2D) && (roi->type != ISOCONTOUR_3D))
	canvas_event_type = CANVAS_EVENT_PRESS_ROTATE_ROI;
      else if (event->button.state & GDK_SHIFT_MASK)
	canvas_event_type = CANVAS_EVENT_PRESS_LARGE_ERASE_ISOCONTOUR;
      else
	canvas_event_type = CANVAS_EVENT_PRESS_ERASE_ISOCONTOUR;
      
    } else if ((!grab_on) && (type == ROI) && (event->button.button == 3)) {
      if ((roi->type != ISOCONTOUR_2D) && (roi->type != ISOCONTOUR_3D))
	canvas_event_type = CANVAS_EVENT_PRESS_RESIZE_ROI;
      else canvas_event_type = CANVAS_EVENT_PRESS_CHANGE_ISOCONTOUR;
      
    } else if ((!grab_on) && (type == ALIGN_PT) && (event->button.button == 1)) {
      canvas_event_type = CANVAS_EVENT_PRESS_MOVE_ALIGN_PT;

    } else 
      canvas_event_type = CANVAS_EVENT_DO_NOTHING;
    
    break;
    
  case GDK_MOTION_NOTIFY:
    event_cp.x = event->motion.x;
    event_cp.y = event->motion.y;

    if (grab_on && (undrawn_roi != NULL)) {
      canvas_event_type = CANVAS_EVENT_MOTION_NEW_ROI;
      
    } else if (extended_mode ) {
      if (align_vertical) canvas_event_type = CANVAS_EVENT_MOTION_ALIGN_VERTICAL;
      else canvas_event_type = CANVAS_EVENT_MOTION_ALIGN_HORIZONTAL;
      
    } else if (grab_on && (!in_object) && (type == VOLUME) && (event->motion.state & GDK_BUTTON1_MASK)) {
      canvas_event_type = CANVAS_EVENT_MOTION_MOVE_VIEW;
      
    } else if (grab_on && (!in_object) && (type == VOLUME) && (event->motion.state & GDK_BUTTON2_MASK)) {
      canvas_event_type = CANVAS_EVENT_MOTION_MINIMIZE_VIEW;
      
    } else if (grab_on && (!in_object) && (type == VOLUME) && (event->motion.state & GDK_BUTTON3_MASK)) {
      canvas_event_type = CANVAS_EVENT_MOTION_RESIZE_VIEW;
      
    } else if (grab_on && (type == ROI) && (event->motion.state & GDK_BUTTON1_MASK)) {
      canvas_event_type = CANVAS_EVENT_MOTION_SHIFT_ROI;
      
    } else if (grab_on && (type == ROI) && (event->motion.state & GDK_BUTTON2_MASK)) {
      if ((roi->type != ISOCONTOUR_2D) && (roi->type != ISOCONTOUR_3D)) 
	canvas_event_type = CANVAS_EVENT_MOTION_ROTATE_ROI;
      else if (event->motion.state & GDK_SHIFT_MASK)
	canvas_event_type = CANVAS_EVENT_MOTION_LARGE_ERASE_ISOCONTOUR;
      else
	canvas_event_type = CANVAS_EVENT_MOTION_ERASE_ISOCONTOUR;
      
    } else if (grab_on && (type == ROI) && (event->motion.state & GDK_BUTTON3_MASK)) {
      if ((roi->type != ISOCONTOUR_2D) && (roi->type != ISOCONTOUR_3D))
	canvas_event_type = CANVAS_EVENT_MOTION_RESIZE_ROI;
      else canvas_event_type = CANVAS_EVENT_MOTION_CHANGE_ISOCONTOUR;

    } else if (grab_on && (type == ALIGN_PT) && (event->motion.state & GDK_BUTTON1_MASK)) {
      canvas_event_type = CANVAS_EVENT_MOTION_ALIGN_PT;

    } else 
      canvas_event_type = CANVAS_EVENT_MOTION;

    break;
    
  case GDK_BUTTON_RELEASE:
    event_cp.x = event->button.x;
    event_cp.y = event->button.y;

    if (undrawn_roi != NULL) {
      canvas_event_type = CANVAS_EVENT_RELEASE_NEW_ROI;
	
    } else if (extended_mode && (!grab_on) && (event->button.button == 3)) {
      if (align_vertical) canvas_event_type = CANVAS_EVENT_ENACT_ALIGN_VERTICAL;
      else canvas_event_type = CANVAS_EVENT_ENACT_ALIGN_HORIZONTAL;
      
    } else if (extended_mode && (!grab_on))  {
      if (align_vertical) canvas_event_type = CANVAS_EVENT_CANCEL_ALIGN_VERTICAL;
      else canvas_event_type = CANVAS_EVENT_CANCEL_ALIGN_HORIZONTAL;

    } else if (extended_mode && (grab_on)) {
      if (align_vertical) canvas_event_type = CANVAS_EVENT_RELEASE_ALIGN_VERTICAL;
      else canvas_event_type = CANVAS_EVENT_RELEASE_ALIGN_HORIZONTAL;
      
    } else if ((!in_object) && (type == VOLUME) && (event->button.button == 1)) {
      canvas_event_type = CANVAS_EVENT_RELEASE_MOVE_VIEW;
	
    } else if ((!in_object) && (type == VOLUME) && (event->button.button == 2)) {
      canvas_event_type = CANVAS_EVENT_RELEASE_MINIMIZE_VIEW;
      
    } else if ((!in_object) && (type == VOLUME) && (event->button.button == 3)) {
      canvas_event_type = CANVAS_EVENT_RELEASE_RESIZE_VIEW;
      
    } else if ((type == ROI) && (event->button.button == 1)) {
      canvas_event_type = CANVAS_EVENT_RELEASE_SHIFT_ROI;
      
    } else if ((type == ROI) && (event->button.button == 2)) {
      if ((roi->type != ISOCONTOUR_2D) && (roi->type != ISOCONTOUR_3D))
	canvas_event_type = CANVAS_EVENT_RELEASE_ROTATE_ROI;
      else if (event->button.state & GDK_SHIFT_MASK)
	canvas_event_type = CANVAS_EVENT_RELEASE_LARGE_ERASE_ISOCONTOUR;
      else 
	canvas_event_type = CANVAS_EVENT_RELEASE_ERASE_ISOCONTOUR;      

    } else if ((type == ROI) && (event->button.button == 3)) {
      if ((roi->type != ISOCONTOUR_2D) && (roi->type != ISOCONTOUR_3D))
	canvas_event_type = CANVAS_EVENT_RELEASE_RESIZE_ROI;
      else canvas_event_type = CANVAS_EVENT_RELEASE_CHANGE_ISOCONTOUR;

    } else if ((type == ALIGN_PT) && (event->button.button == 1)) {      
      canvas_event_type = CANVAS_EVENT_RELEASE_ALIGN_PT;

    } else 
      canvas_event_type = CANVAS_EVENT_DO_NOTHING;
    
    break;
    
  default: 
    event_cp.x = event_cp.y = 0;
    /* an event we don't handle */
    canvas_event_type = CANVAS_EVENT_DO_NOTHING;
    break;
    
  }

  /* get the location of the event, and convert it to the canvas coordinates */
  gnome_canvas_w2c_d(GNOME_CANVAS(canvas->canvas), event_cp.x, event_cp.y, &canvas_cp.x, &canvas_cp.y);

  /* Convert the event location info to real units */
  canvas_rp = cp_2_rp(canvas, canvas_cp);
  base_rp = realspace_alt_coord_to_base(canvas_rp, canvas->coord_frame);


  //  if ((canvas_event_type == CANVAS_EVENT_DO_NOTHING) ||
  //      (canvas_event_type == CANVAS_EVENT_MOTION))
  //    ;
  //  else
  //    g_print("gdk %2d canvas %2d grab_on %d button %d mode %d widget %p extended %d, undrawn_roi %c\n",
  //  	    event->type, canvas_event_type, grab_on, event->button.button, type, 
  //	    widget, extended_mode, (undrawn_roi == NULL) ? 'n' : 'y' );


  switch (canvas_event_type) {
    
  case CANVAS_EVENT_ENTER_VOLUME:
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_CANVAS_VOLUME, &base_rp, 0.0);
    ui_common_place_cursor(UI_CURSOR_VOLUME_MODE, GTK_WIDGET(canvas));
    break;

  case CANVAS_EVENT_ENTER_ALIGN_PT:
    in_object = TRUE;
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_CANVAS_ALIGN_PT, &base_rp, 0.0);
    ui_common_place_cursor(UI_CURSOR_ALIGN_PT_MODE, GTK_WIDGET(canvas));
    break;
    
  case CANVAS_EVENT_ENTER_NEW_NORMAL_ROI:
    in_object = TRUE;
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_CANVAS_NEW_ROI, &base_rp, 0.0);
    ui_common_place_cursor(UI_CURSOR_NEW_ROI_MODE, GTK_WIDGET(canvas));
    break;

    
  case CANVAS_EVENT_ENTER_NEW_ISOCONTOUR_ROI:
    in_object = TRUE;
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_CANVAS_NEW_ISOCONTOUR_ROI, &base_rp, 0.0);
    ui_common_place_cursor(UI_CURSOR_NEW_ROI_MODE, GTK_WIDGET(canvas));
    break;

    
  case CANVAS_EVENT_ENTER_NORMAL_ROI:
    in_object = TRUE;
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_CANVAS_ROI, &base_rp, 0.0);
    ui_common_place_cursor(UI_CURSOR_OLD_ROI_MODE, GTK_WIDGET(canvas));
    break;

    
  case CANVAS_EVENT_ENTER_ISOCONTOUR_ROI:
    in_object = TRUE;
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_CANVAS_ISOCONTOUR_ROI, &base_rp, 0.0);
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_UPDATE_VALUE, &base_rp, roi->isocontour_value);
    ui_common_place_cursor(UI_CURSOR_OLD_ROI_MODE, GTK_WIDGET(canvas));
    break;
    
    
  case CANVAS_EVENT_LEAVE:
    ui_common_remove_cursor(GTK_WIDGET(canvas)); 

    /* yes, do it twice if we're leaving the canvas, there's a bug where if the canvas gets 
       covered by a menu, no GDK_LEAVE_NOTIFY is called for the GDK_ENTER_NOTIFY */
    if (type == VOLUME) ui_common_remove_cursor(GTK_WIDGET(canvas));

    /* yep, we don't seem to get an EVENT_ENTER corresponding to the EVENT_LEAVE for a canvas_item */
    if (in_object) 
      gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		      HELP_INFO_CANVAS_VOLUME, &base_rp, 0.0);
    in_object = FALSE;
    break;

  case CANVAS_EVENT_PRESS_MINIMIZE_VIEW:
    canvas->thickness = volumes_min_voxel_size(canvas->volumes);
    /* fall through */
  case CANVAS_EVENT_PRESS_RESIZE_VIEW:
  case CANVAS_EVENT_PRESS_MOVE_VIEW:
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_UPDATE_LOCATION, &base_rp, 0.0);
    grab_on = TRUE;
    canvas->center = base_rp;
    outline_color = color_table_outline_color(canvas->color_table, FALSE);
    canvas_update_cross(canvas, AMITK_CANVAS_CROSS_SHOW, canvas->center, outline_color,canvas->thickness);
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[VIEW_CHANGING], 
		    &(canvas->center), canvas->thickness);
    break;

    
  case CANVAS_EVENT_PRESS_ALIGN_VERTICAL:
  case CANVAS_EVENT_PRESS_ALIGN_HORIZONTAL:
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_CANVAS_ALIGN, &base_rp, 0.0);
    if (extended_mode) {
      grab_on = FALSE; /* do everything on the BUTTON_RELEASE */
    } else {
      grab_on = TRUE;
      extended_mode = TRUE;
      initial_cp = canvas_cp;

      points = gnome_canvas_points_new(2);
      points->coords[2] = points->coords[0] = canvas_cp.x;
      points->coords[3] = points->coords[1] = canvas_cp.y;
      outline_color = color_table_outline_color(canvas->color_table, TRUE);
      canvas_item = gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)), 
					  gnome_canvas_line_get_type(),
					  "points", points, "width_pixels", canvas->roi_width,
					  "fill_color_rgba", color_table_rgba_to_uint32(outline_color),
					  NULL);
      gnome_canvas_points_unref(points);

      align_vertical = (canvas_event_type == CANVAS_EVENT_PRESS_ALIGN_VERTICAL);
    }
    break;


  case CANVAS_EVENT_PRESS_MOVE_ALIGN_PT:
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_CANVAS_ALIGN_PT, &base_rp, 0.0);
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_UPDATE_LOCATION, &base_rp, 0.0);
    grab_on = TRUE;
    canvas_item = GNOME_CANVAS_ITEM(widget); 
    gnome_canvas_item_grab(canvas_item,
			   GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			   ui_common_cursor[UI_CURSOR_ALIGN_PT_MODE], event->button.time);
    previous_cp = canvas_cp;
    break;
    
  case CANVAS_EVENT_PRESS_NEW_ROI:
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_CANVAS_NEW_ROI, &base_rp, 0.0);
    outline_color = color_table_outline_color(canvas->color_table, TRUE);
    initial_canvas_rp = canvas_rp;
    initial_cp = canvas_cp;
    
    /* create the new roi */
    switch(undrawn_roi->type) {
    case BOX:
      canvas_item = 
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
			      gnome_canvas_rect_get_type(),
			      "x1",canvas_cp.x, "y1", canvas_cp.y, 
			      "x2", canvas_cp.x, "y2", canvas_cp.y,
			      "fill_color", NULL,
			      "outline_color_rgba", color_table_rgba_to_uint32(outline_color),
			      "width_pixels", canvas->roi_width,
			      NULL);
      grab_on = TRUE;
      gnome_canvas_item_grab(GNOME_CANVAS_ITEM(canvas_item),
			     GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			     ui_common_cursor[UI_CURSOR_NEW_ROI_MOTION],
			     event->button.time);
      break;
    case ELLIPSOID:
    case CYLINDER:
      canvas_item = 
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
			      gnome_canvas_ellipse_get_type(),
			      "x1", canvas_cp.x, "y1", canvas_cp.y, 
			      "x2", canvas_cp.x, "y2", canvas_cp.y,
			      "fill_color", NULL,
			      "outline_color_rgba", color_table_rgba_to_uint32(outline_color),
			      "width_pixels", canvas->roi_width,
			      NULL);
      grab_on = TRUE;
      gnome_canvas_item_grab(GNOME_CANVAS_ITEM(canvas_item),
			     GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			     ui_common_cursor[UI_CURSOR_NEW_ROI_MOTION],
			     event->button.time);
      break;
    case ISOCONTOUR_2D:
    case ISOCONTOUR_3D:
      grab_on = TRUE;
      break;
    default:
      grab_on = FALSE;
      g_warning("%s: unexpected case in %s at line %d, roi_type %d", 
		PACKAGE, __FILE__, __LINE__, undrawn_roi->type);
      break;
    }
    break;

  case CANVAS_EVENT_PRESS_SHIFT_ROI:
    previous_cp = canvas_cp;
  case CANVAS_EVENT_PRESS_ROTATE_ROI:
  case CANVAS_EVENT_PRESS_RESIZE_ROI:
    if ((roi->type == ISOCONTOUR_2D) || (roi->type == ISOCONTOUR_3D))
      gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		      HELP_INFO_CANVAS_ISOCONTOUR_ROI, &base_rp, 0.0);
    else
      gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		      HELP_INFO_CANVAS_ROI, &base_rp, 0.0);
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_UPDATE_LOCATION, &base_rp, 0.0);
    grab_on = TRUE;
    canvas_item = GNOME_CANVAS_ITEM(widget); 
    if (canvas_event_type == CANVAS_EVENT_PRESS_SHIFT_ROI)
      gnome_canvas_item_grab(canvas_item,
			     GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			     ui_common_cursor[UI_CURSOR_OLD_ROI_SHIFT], event->button.time);
    else if (canvas_event_type == CANVAS_EVENT_PRESS_ROTATE_ROI)
      gnome_canvas_item_grab(canvas_item,
			     GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			     ui_common_cursor[UI_CURSOR_OLD_ROI_ROTATE], event->button.time);
    else /* CANVAS_EVENT_PRESS_RESIZE_ROI */
      gnome_canvas_item_grab(canvas_item,
			     GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			     ui_common_cursor[UI_CURSOR_OLD_ROI_RESIZE], event->button.time);
    initial_base_rp = base_rp;
    initial_cp = canvas_cp;
    initial_canvas_rp = canvas_rp;
    theta = 0.0;
    roi_zoom.x = roi_zoom.y = roi_zoom.z = 1.0;
    temp_rp[0] = realspace_base_coord_to_alt(rs_offset(roi->coord_frame), roi->coord_frame);
    roi_center_rp = rp_add(rp_cmult(0.5, temp_rp[0]), rp_cmult(0.5, roi->corner));
    roi_radius_rp = rp_cmult(0.5,rp_diff(roi->corner, temp_rp[0]));
    break;

  case CANVAS_EVENT_PRESS_CHANGE_ISOCONTOUR:
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_CANVAS_ISOCONTOUR_ROI, &base_rp, 0.0);
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_UPDATE_LOCATION, &base_rp, 0.0);
    grab_on = TRUE;
    initial_cp = canvas_cp;

    points = gnome_canvas_points_new(2);
    points->coords[2] = points->coords[0] = canvas_cp.x;
    points->coords[3] = points->coords[1] = canvas_cp.y;
    outline_color = color_table_outline_color(canvas->color_table, TRUE);
    canvas_item = gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)), 
					gnome_canvas_line_get_type(),
					"points", points, "width_pixels", canvas->roi_width,
					"fill_color_rgba", color_table_rgba_to_uint32(outline_color),
					NULL);
    gnome_canvas_item_grab(GNOME_CANVAS_ITEM(widget),
			   GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			   ui_common_cursor[UI_CURSOR_OLD_ROI_ISOCONTOUR], event->button.time);
    gtk_object_set_data(GTK_OBJECT(canvas_item), "object", roi);
    gtk_object_set_data(GTK_OBJECT(canvas_item), "type", GINT_TO_POINTER(ROI));
    gnome_canvas_points_unref(points);
    break;

  case CANVAS_EVENT_PRESS_NEW_ALIGN_PT:
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[NEW_ALIGN_PT], &base_rp);
    break;

  case CANVAS_EVENT_MOTION:
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_UPDATE_LOCATION, &base_rp, 0.0);
    break;

  case CANVAS_EVENT_MOTION_MOVE_VIEW:
  case CANVAS_EVENT_MOTION_MINIMIZE_VIEW:
  case CANVAS_EVENT_MOTION_RESIZE_VIEW:
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_UPDATE_LOCATION, &base_rp, 0.0);
    if (canvas_event_type == CANVAS_EVENT_MOTION_RESIZE_VIEW)
      canvas->thickness = rp_max_dim(rp_diff(base_rp, canvas->center));
    else
      canvas->center = base_rp;
    outline_color = color_table_outline_color(canvas->color_table, FALSE);
    canvas_update_cross(canvas, AMITK_CANVAS_CROSS_SHOW, canvas->center, outline_color,canvas->thickness);
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[VIEW_CHANGING], 
		    &(canvas->center), canvas->thickness);
    break;

  case CANVAS_EVENT_MOTION_ALIGN_PT:
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_UPDATE_LOCATION, &base_rp, 0.0);
    diff_cp = cp_sub(canvas_cp, previous_cp);
    gnome_canvas_item_i2w(GNOME_CANVAS_ITEM(canvas_item)->parent, &diff_cp.x, &diff_cp.y);
    gnome_canvas_item_move(GNOME_CANVAS_ITEM(canvas_item),diff_cp.x,diff_cp.y); 
    previous_cp = canvas_cp;
    break;

  case CANVAS_EVENT_PRESS_LARGE_ERASE_ISOCONTOUR:
  case CANVAS_EVENT_PRESS_ERASE_ISOCONTOUR:
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_CANVAS_ISOCONTOUR_ROI, &base_rp, 0.0);
    grab_on = TRUE;
    canvas_item = GNOME_CANVAS_ITEM(widget);
    gnome_canvas_item_grab(canvas_item,
			   GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			   ui_common_cursor[UI_CURSOR_OLD_ROI_ERASE], event->button.time);
    /* and fall through */
  case CANVAS_EVENT_MOTION_LARGE_ERASE_ISOCONTOUR:
  case CANVAS_EVENT_MOTION_ERASE_ISOCONTOUR:
    g_return_val_if_fail(canvas->slices != NULL, FALSE);
    temp_rp[0] = realspace_base_coord_to_alt(base_rp, roi->coord_frame);
    REALPOINT_TO_VOXEL(temp_rp[0], roi->voxel_size, 0, temp_vp);
    if ((canvas_event_type == CANVAS_EVENT_MOTION_LARGE_ERASE_ISOCONTOUR) ||
	(canvas_event_type == CANVAS_EVENT_PRESS_LARGE_ERASE_ISOCONTOUR))
      roi_isocontour_erase_area(roi, temp_vp, 2);
    else
      roi_isocontour_erase_area(roi, temp_vp, 0);
    temp_volume = canvas->slices->volume; /* just use the first slice for now... not always right */
    temp_rp[0] = realspace_base_coord_to_alt(base_rp, temp_volume->coord_frame);
    REALPOINT_TO_VOXEL(temp_rp[0], temp_volume->voxel_size, 0, temp_vp);
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_UPDATE_VALUE, &base_rp, volume_value(temp_volume, temp_vp));
    break;

  case CANVAS_EVENT_MOTION_CHANGE_ISOCONTOUR:
    g_return_val_if_fail(canvas->slices != NULL, FALSE);
    temp_volume = canvas->slices->volume; /* just use the first slice for now... not always right */
    temp_rp[0] = realspace_base_coord_to_alt(base_rp, temp_volume->coord_frame);
    REALPOINT_TO_VOXEL(temp_rp[0], temp_volume->voxel_size, 0, temp_vp);
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_UPDATE_VALUE, &base_rp, volume_value(temp_volume, temp_vp));
    /* and fall through */
  case CANVAS_EVENT_MOTION_ALIGN_HORIZONTAL:
  case CANVAS_EVENT_MOTION_ALIGN_VERTICAL:
    points = gnome_canvas_points_new(2);
    points->coords[0] = initial_cp.x;
    points->coords[1] = initial_cp.y;
    points->coords[2] = canvas_cp.x;
    points->coords[3] = canvas_cp.y;
    gnome_canvas_item_set(canvas_item, "points", points, NULL);
    gnome_canvas_points_unref(points);
    break;

  case CANVAS_EVENT_MOTION_NEW_ROI:
    if ((undrawn_roi->type == ISOCONTOUR_2D) || (undrawn_roi->type == ISOCONTOUR_3D))
      gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		      HELP_INFO_CANVAS_NEW_ISOCONTOUR_ROI, &base_rp, 0.0);
    else {
      gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		      HELP_INFO_CANVAS_NEW_ROI, &base_rp, 0.0);
      diff_cp = cp_diff(initial_cp, canvas_cp);
      temp_cp[0] = cp_sub(initial_cp, diff_cp);
      temp_cp[1] = cp_add(initial_cp, diff_cp);
      gnome_canvas_item_set(canvas_item, "x1", temp_cp[0].x, "y1", temp_cp[0].y,
			    "x2", temp_cp[1].x, "y2", temp_cp[1].y,NULL);
    }
    break;

  case CANVAS_EVENT_MOTION_SHIFT_ROI:
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
    		    HELP_INFO_UPDATE_LOCATION, &base_rp, 0.0);
    diff_cp = cp_sub(canvas_cp, previous_cp);
    gnome_canvas_item_i2w(GNOME_CANVAS_ITEM(canvas_item)->parent, &diff_cp.x, &diff_cp.y);
    gnome_canvas_item_move(GNOME_CANVAS_ITEM(canvas_item),diff_cp.x,diff_cp.y); 
    previous_cp = canvas_cp;
    break;


  case CANVAS_EVENT_MOTION_ROTATE_ROI: 
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_UPDATE_LOCATION, &base_rp, 0.0);
    {
      /* rotate button Pressed, we're rotating the object */
      /* note, I'd like to use the function "gnome_canvas_item_rotate"
	 but this isn't defined in the current version of gnome.... 
	 so I'll have to do a whole bunch of shit*/
      double affine[6];
      canvaspoint_t item_center;
      realpoint_t center;
      
      center = realspace_alt_coord_to_alt(roi_center_rp, roi->coord_frame, canvas->coord_frame);
      temp_rp[0] = rp_sub(initial_canvas_rp,center);
      temp_rp[1] = rp_sub(canvas_rp,center);
      
      theta = acos(rp_dot_product(temp_rp[0],temp_rp[1])/(rp_mag(temp_rp[0]) * rp_mag(temp_rp[1])));

      /* correct for the fact that acos is always positive by using the cross product */
      if ((temp_rp[0].x*temp_rp[1].y-temp_rp[0].y*temp_rp[1].x) < 0.0) theta = -theta;

      /* figure out what the center of the roi is in canvas_item coords */
      /* compensate for x's origin being top left (ours is bottom left) */
      item_center = rp_2_cp(canvas, center);
      affine[0] = cos(-theta); /* neg cause GDK has Y axis going down, not up */
      affine[1] = sin(-theta);
      affine[2] = -affine[1];
      affine[3] = affine[0];
      affine[4] = (1.0-affine[0])*item_center.x+affine[1]*item_center.y;
      affine[5] = (1.0-affine[3])*item_center.y+affine[2]*item_center.x;
      gnome_canvas_item_affine_absolute(GNOME_CANVAS_ITEM(canvas_item),affine);
    }
    break;


  case CANVAS_EVENT_MOTION_RESIZE_ROI:
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_UPDATE_LOCATION, &base_rp, 0.0);
    {
      /* RESIZE button Pressed, we're scaling the object */
      /* note, I'd like to use the function "gnome_canvas_item_scale"
	 but this isn't defined in the current version of gnome.... 
	 so I'll have to do a whole bunch of shit*/
      /* also, this "zoom" strategy doesn't always work if the object is not
	 aligned with the view.... oh well... good enough for now... */
      double affine[6];
      canvaspoint_t item_center;
      floatpoint_t jump_limit, max, temp, dot;
      double cos_r, sin_r, rot;
      axis_t i_axis, axis[NUM_AXIS];
      canvaspoint_t radius_cp;
      realpoint_t center, radius, canvas_zoom;
	
      /* calculating the radius and center wrt the canvas */
      radius = realspace_alt_dim_to_alt(roi_radius_rp, roi->coord_frame, canvas->coord_frame);
      center = realspace_alt_coord_to_alt(roi_center_rp, roi->coord_frame, canvas->coord_frame);
      temp_rp[0] = rp_diff(initial_canvas_rp, center);
      temp_rp[1] = rp_diff(canvas_rp,center);
      radius_cp.x = roi_radius_rp.x;
      radius_cp.y = roi_radius_rp.y;
      jump_limit = cp_mag(radius_cp)/3.0;
	
      /* figure out the zoom we're specifying via the canvas */
      if (temp_rp[0].x < jump_limit) /* prevent jumping */
	canvas_zoom.x = (radius.x+temp_rp[1].x)/(radius.x+temp_rp[0].x);
      else
	canvas_zoom.x = temp_rp[1].x/temp_rp[0].x;
      if (temp_rp[0].y < jump_limit) /* prevent jumping */
	canvas_zoom.y = (radius.x+temp_rp[1].y)/(radius.x+temp_rp[0].y);
      else
	canvas_zoom.y = temp_rp[1].y/temp_rp[0].y;
      canvas_zoom.z = 1.0;
	
      /* translate the canvas zoom into the ROI's coordinate frame */
      temp_rp[0].x = temp_rp[0].y = temp_rp[0].z = 1.0;
      temp_rp[0] = realspace_alt_dim_to_alt(temp_rp[0], canvas->coord_frame,
					    roi->coord_frame);
      roi_zoom = realspace_alt_dim_to_alt(canvas_zoom, canvas->coord_frame,
					  roi->coord_frame);
      roi_zoom = rp_div(roi_zoom, temp_rp[0]);
	
      /* first, figure out how much the roi is rotated in the plane of the canvas */
      /* figure out which axis in the ROI is closest to the view's x axis */
      max = 0.0;
      axis[XAXIS]=XAXIS;
      temp_rp[0] = rs_get_orthogonal_view_axis(canvas->coord_frame, 
					       canvas->view, canvas->layout, XAXIS);
      for (i_axis=0;i_axis<NUM_AXIS;i_axis++) {
	temp = fabs(rp_dot_product(temp_rp[0], rs_specific_axis(roi->coord_frame, i_axis)));
	if (temp > max) {
	  max=temp;
	  axis[XAXIS]=i_axis;
	}
      }
	
      /* and y axis */
      max = 0.0;
      axis[YAXIS]= (axis[XAXIS]+1 < NUM_AXIS) ? axis[XAXIS]+1 : XAXIS;
      temp_rp[0] = rs_get_orthogonal_view_axis(canvas->coord_frame, 
					       canvas->view, canvas->layout, YAXIS);
      for (i_axis=0;i_axis<NUM_AXIS;i_axis++) {
	temp = fabs(rp_dot_product(temp_rp[0], rs_specific_axis(roi->coord_frame, i_axis)));
	if (temp > max) {
	  if (i_axis != axis[XAXIS]) {
	    max=temp;
	    axis[YAXIS]=i_axis;
	  }
	}
      }
	
      i_axis = ZAXIS;
      for (i_axis=0;i_axis<NUM_AXIS;i_axis++)
	if (i_axis != axis[XAXIS])
	  if (i_axis != axis[YAXIS])
	    axis[ZAXIS]=i_axis;
	
      rot = 0;
      for (i_axis=0;i_axis<=XAXIS;i_axis++) {
	/* and project that axis onto the x,y plane of the canvas */
	temp_rp[0] = rs_specific_axis(roi->coord_frame,axis[i_axis]);
	temp_rp[0] = realspace_base_coord_to_alt(temp_rp[0], canvas->coord_frame);
	temp_rp[0].z = 0.0;
	temp_rp[1] = rs_get_orthogonal_view_axis(canvas->coord_frame, 
						 canvas->view, canvas->layout, i_axis);
	temp_rp[1] = realspace_base_coord_to_alt(temp_rp[1], canvas->coord_frame);
	temp_rp[1].z = 0.0;
	  
	/* and get the angle the projection makes to the x axis */
	dot = rp_dot_product(temp_rp[0],temp_rp[1])/(rp_mag(temp_rp[0]) * rp_mag(temp_rp[1]));
	/* correct for the fact that acos is always positive by using the cross product */
	if ((temp_rp[0].x*temp_rp[1].y-temp_rp[0].y*temp_rp[1].x) > 0.0)
	  temp = acos(dot);
	else
	  temp = -acos(dot);
	if (isnan(temp)) temp=0; 
	rot += temp;
      }

      cos_r = cos(rot);       /* precompute cos and sin of rot */
      sin_r = sin(rot);
	
      /* figure out what the center of the roi is in gnome_canvas_item coords */
      /* compensate for X's origin being top left (not bottom left) */
      item_center = rp_2_cp(canvas, center);
	
      /* do a wild ass affine matrix so that we can scale while preserving angles */
      affine[0] = canvas_zoom.x * cos_r * cos_r + canvas_zoom.y * sin_r * sin_r;
      affine[1] = (canvas_zoom.x-canvas_zoom.y)* cos_r * sin_r;
      affine[2] = affine[1];
      affine[3] = canvas_zoom.x * sin_r * sin_r + canvas_zoom.y * cos_r * cos_r;
      affine[4] = item_center.x - item_center.x*canvas_zoom.x*cos_r*cos_r 
	- item_center.x*canvas_zoom.y*sin_r*sin_r + (canvas_zoom.y-canvas_zoom.x)*item_center.y*cos_r*sin_r;
      affine[5] = item_center.y - item_center.y*canvas_zoom.y*cos_r*cos_r 
	- item_center.y*canvas_zoom.x*sin_r*sin_r + (canvas_zoom.y-canvas_zoom.x)*item_center.x*cos_r*sin_r;
      gnome_canvas_item_affine_absolute(GNOME_CANVAS_ITEM(canvas_item),affine);
    }
    break;

  case CANVAS_EVENT_RELEASE_MOVE_VIEW:
  case CANVAS_EVENT_RELEASE_MINIMIZE_VIEW:
  case CANVAS_EVENT_RELEASE_RESIZE_VIEW:
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_UPDATE_LOCATION, &base_rp, 0.0);
    grab_on = FALSE;
    canvas_update_cross(canvas, AMITK_CANVAS_CROSS_HIDE, base_rp, rgba_black, 0.0);
    canvas->center = base_rp;
    if (canvas_event_type == CANVAS_EVENT_RELEASE_MOVE_VIEW)
      canvas_update_arrows(canvas, canvas->center, canvas->thickness);
    else
      canvas_update(canvas, UPDATE_ALL);
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[VIEW_CHANGED], 
		    &(canvas->center), canvas->thickness);
    break;

  case CANVAS_EVENT_RELEASE_ALIGN_HORIZONTAL:
  case CANVAS_EVENT_RELEASE_ALIGN_VERTICAL:
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_CANVAS_ALIGN, &base_rp, 0.0);
    break;

  case CANVAS_EVENT_CANCEL_ALIGN_HORIZONTAL:
  case CANVAS_EVENT_CANCEL_ALIGN_VERTICAL:
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_CANVAS_VOLUME, &base_rp, 0.0);
    extended_mode = FALSE;
    gtk_object_destroy(GTK_OBJECT(canvas_item));
    break;

  case CANVAS_EVENT_ENACT_ALIGN_HORIZONTAL:
  case CANVAS_EVENT_ENACT_ALIGN_VERTICAL:
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_CANVAS_VOLUME, &base_rp, 0.0);
    extended_mode = FALSE;
    gtk_object_destroy(GTK_OBJECT(canvas_item));

    /* figure out how many degrees we've rotated */
    diff_cp = cp_sub(canvas_cp, initial_cp);
    if (align_vertical) theta = -atan(diff_cp.x/diff_cp.y);
    else theta = atan(diff_cp.y/diff_cp.x);
    
    if (isnan(theta)) theta = 0;
    
    if (canvas->view == SAGITTAL) theta = -theta; /* sagittal is left-handed */
    
    /* rotate all the currently displayed volumes */
    volumes = canvas->volumes;
    while (volumes != NULL) {
      /* saving the center wrt to the volume's coord axis, as we're rotating around the center */
      temp_rp[0] = realspace_base_coord_to_alt(canvas->center, volumes->volume->coord_frame);
      realspace_rotate_on_axis(volumes->volume->coord_frame,
			       rs_specific_axis(canvas->coord_frame, ZAXIS),
			       theta);
	
      /* recalculate the offset of this volume based on the center we stored */
      rs_set_offset(volumes->volume->coord_frame, zero_rp);
      temp_rp[0] = realspace_alt_coord_to_base(temp_rp[0], volumes->volume->coord_frame);
      rs_set_offset(volumes->volume->coord_frame, rp_sub(canvas->center, temp_rp[0]));
      volumes = volumes->next;
    }      
     
    canvas_update(canvas, UPDATE_ALL);
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[VOLUMES_CHANGED]);

    break;




  case CANVAS_EVENT_RELEASE_ALIGN_PT:
    gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
    grab_on = FALSE;
    temp_rp[0] = realspace_base_coord_to_alt(base_rp, volume->coord_frame);
    /* ------------- apply any shift done -------------- */
    if (!REALPOINT_EQUAL(align_pt->point, temp_rp[0])) {
      align_pt->point = temp_rp[0];

      /* if we were moving the point, reset the view center to where the point is */
      canvas->center = base_rp;
      canvas_update_arrows(canvas, canvas->center, canvas->thickness);
      gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[VIEW_CHANGED], 
		      &(canvas->center), canvas->thickness);
    }
    break;


  case CANVAS_EVENT_RELEASE_NEW_ROI:
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[HELP_EVENT], 
		    HELP_INFO_CANVAS_VOLUME, &base_rp, 0.0);
    grab_on = FALSE;
    switch(undrawn_roi->type) {
    case CYLINDER:
    case BOX:
    case ELLIPSOID:  
      gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(canvas_item), event->button.time);
      gtk_object_destroy(GTK_OBJECT(canvas_item)); /* get rid of the roi drawn on the canvas */
      
      diff_rp = rp_diff(initial_canvas_rp, canvas_rp);
      diff_rp.z = canvas->thickness/2.0;
      temp_rp[0] = rp_sub(initial_canvas_rp, diff_rp);
      temp_rp[1] = rp_add(initial_canvas_rp, diff_rp);

      /* we'll save the coord frame and offset of the roi */
      undrawn_roi->coord_frame = rs_copy(canvas->coord_frame);
      rs_set_offset(undrawn_roi->coord_frame,
		    realspace_alt_coord_to_base(temp_rp[0], canvas->coord_frame));
      
      /* and set the far corner of the roi */
      undrawn_roi->corner = 
	rp_abs(realspace_alt_coord_to_alt(temp_rp[1], canvas->coord_frame,
					  undrawn_roi->coord_frame));
      amitk_canvas_update_object(canvas, undrawn_roi, ROI);
      gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[ROI_CHANGED], undrawn_roi);
      break;
    case ISOCONTOUR_2D: 
    case ISOCONTOUR_3D:
      if (undrawn_roi->type == ISOCONTOUR_2D) {
	g_return_val_if_fail(canvas->slices != NULL, FALSE);
	temp_volume = canvas->slices->volume; /* just use the first slice for now... not always right */
	temp_rp[0] = realspace_base_coord_to_alt(base_rp, temp_volume->coord_frame);
	REALPOINT_TO_VOXEL(temp_rp[0], temp_volume->voxel_size, 
			   volume_frame(temp_volume, canvas->start_time), temp_vp);
	ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(canvas));
	if (data_set_includes_voxel(temp_volume->data_set,temp_vp))
	  roi_set_isocontour(undrawn_roi, temp_volume, temp_vp);
	ui_common_remove_cursor(GTK_WIDGET(canvas));
	
	amitk_canvas_update_object(canvas, undrawn_roi, ROI);
	gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[ROI_CHANGED], undrawn_roi);
      } else { /* ISOCONTOUR_3D */
	gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[ISOCONTOUR_3D_CHANGED], undrawn_roi, &base_rp);
      }
      break;
    default:
      g_warning("%s: unexpected case in %s at line %d, roi_type %d", 
		PACKAGE, __FILE__, __LINE__, undrawn_roi->type);
      break;
    }

    break;


  case CANVAS_EVENT_RELEASE_SHIFT_ROI:
    gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
    grab_on = FALSE;
    shift = rp_sub(base_rp, initial_base_rp);
    /* ------------- apply any shift done -------------- */
    if (!REALPOINT_EQUAL(shift, zero_rp)) {
      rs_set_offset(roi->coord_frame, rp_add(shift, rs_offset(roi->coord_frame)));
      /* reset the view_center to the center of the current roi */
      canvas->center = roi_calculate_center(roi);
      canvas_update(canvas, UPDATE_ALL);
      gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[VIEW_CHANGED], 
		      &(canvas->center), canvas->thickness);
    }
    break;


  case CANVAS_EVENT_RELEASE_ROTATE_ROI:
    gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
    grab_on = FALSE;
    if (canvas->view == SAGITTAL) theta = -theta; /* compensate for sagittal being left-handed coord frame */
    
    temp_rp[0] = realspace_alt_coord_to_base(roi_center_rp, roi->coord_frame); /* new center */
    rs_set_offset(roi->coord_frame, temp_rp[0]);
    
    /* now rotate the roi coord_frame axis */
    realspace_rotate_on_axis(roi->coord_frame,
			     rs_specific_axis(canvas->coord_frame,  ZAXIS),
			     theta);
    
    /* and recaculate the offset */
    temp_rp[0] = rp_sub(realspace_base_coord_to_alt(temp_rp[0], roi->coord_frame), roi_radius_rp);
    temp_rp[0] = realspace_alt_coord_to_base(temp_rp[0],roi->coord_frame);
    rs_set_offset(roi->coord_frame, temp_rp[0]); 

    canvas_update_roi(canvas, GNOME_CANVAS_ITEM(widget), NULL);
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[ROI_CHANGED], roi);
    break;


  case CANVAS_EVENT_RELEASE_RESIZE_ROI:
    gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
    grab_on = FALSE;
    
    temp_rp[0] = rp_mult(roi_zoom, roi_radius_rp); /* new radius */
    rs_set_offset(roi->coord_frame, realspace_alt_coord_to_base(rp_sub(roi_center_rp, temp_rp[0]), 
								roi->coord_frame));
	  
    /* and the new upper right corner is simply twice the radius */
    roi->corner = rp_cmult(2.0, temp_rp[0]);

    canvas_update_roi(canvas, GNOME_CANVAS_ITEM(widget), NULL);
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[ROI_CHANGED], roi);
    break;

  case CANVAS_EVENT_RELEASE_LARGE_ERASE_ISOCONTOUR:
  case CANVAS_EVENT_RELEASE_ERASE_ISOCONTOUR:
    gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
    grab_on = FALSE;

    canvas_update_roi(canvas, GNOME_CANVAS_ITEM(widget), NULL);
    gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[ROI_CHANGED], roi);
    break;

  case CANVAS_EVENT_RELEASE_CHANGE_ISOCONTOUR:
    gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
    grab_on = FALSE;
    gtk_object_destroy(GTK_OBJECT(canvas_item));

    if (roi->type == ISOCONTOUR_2D) {
      g_return_val_if_fail(canvas->slices != NULL, FALSE);
      temp_volume = canvas->slices->volume; /* just use the first slice for now... not always right */
      temp_rp[0] = realspace_base_coord_to_alt(base_rp, temp_volume->coord_frame);
      REALPOINT_TO_VOXEL(temp_rp[0], temp_volume->voxel_size, 
			 volume_frame(temp_volume, canvas->start_time), temp_vp);
      ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(canvas));
      if (data_set_includes_voxel(temp_volume->data_set,temp_vp))
	roi_set_isocontour(roi, temp_volume, temp_vp);
      ui_common_remove_cursor(GTK_WIDGET(canvas));
      
      canvas_update_roi(canvas, GNOME_CANVAS_ITEM(widget), NULL);
      gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[ROI_CHANGED], roi);
    } else { /* ISOCONTOUR_3D */
      gtk_signal_emit(GTK_OBJECT (canvas), canvas_signals[ISOCONTOUR_3D_CHANGED], roi, &base_rp);
    }

    break;

  case CANVAS_EVENT_DO_NOTHING:
    break;
  default:
    g_warning("%s: unexpected case in %s at line %d, event %d", PACKAGE, __FILE__, __LINE__, canvas_event_type);
    break;
  }
  
  return FALSE;
}








/* function called indicating the plane adjustment has changed */
static void canvas_scrollbar_cb(GtkObject * adjustment, gpointer data) {

  AmitkCanvas * canvas = data;
  realpoint_t canvas_center;

  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));

  if (canvas->volumes == NULL) return;

  canvas_center = realspace_base_coord_to_alt(canvas->center,
					   canvas->coord_frame);
  canvas_center.z = GTK_ADJUSTMENT(adjustment)->value;
  canvas->center = realspace_alt_coord_to_base(canvas_center, canvas->coord_frame);

  canvas_update(canvas, UPDATE_ALL);
  gtk_signal_emit (GTK_OBJECT (canvas), canvas_signals[VIEW_Z_POSITION_CHANGED], &(canvas->center));

  return;
}





/* function to update the adjustment settings for the scrollbar */
static void canvas_update_scrollbar(AmitkCanvas * canvas, realpoint_t center, floatpoint_t thickness) {

  realpoint_t view_corner[2];
  floatpoint_t upper, lower;
  floatpoint_t min_voxel_size;
  realpoint_t zp_start;

  if (canvas->volumes == NULL) {   /* junk values */
    min_voxel_size = 1.0;
    upper = lower = zp_start.z = 0.0;
  } else { /* calculate values */
    volumes_get_view_corners(canvas->volumes, canvas->coord_frame, view_corner);
    min_voxel_size = volumes_max_min_voxel_size(canvas->volumes);

    upper = view_corner[1].z;
    lower = view_corner[0].z;

    /* translate the view center point so that the z coordinate corresponds to depth in this view */
    zp_start = realspace_base_coord_to_alt(center, canvas->coord_frame);

    /* make sure our view center makes sense */
    if (zp_start.z < lower) {

      if (zp_start.z < lower-thickness)
	zp_start.z = (upper-lower)/2.0+lower;
      else
	zp_start.z = lower;

    } else if (zp_start.z > upper) {

      if (zp_start.z > lower+thickness)
	zp_start.z = (upper-lower)/2.0+lower;
      else
	zp_start.z = upper;
    }
  }
  
  GTK_ADJUSTMENT(canvas->scrollbar_adjustment)->upper = upper;
  GTK_ADJUSTMENT(canvas->scrollbar_adjustment)->lower = lower;
  GTK_ADJUSTMENT(canvas->scrollbar_adjustment)->page_increment = min_voxel_size;
  GTK_ADJUSTMENT(canvas->scrollbar_adjustment)->page_size = thickness;
  GTK_ADJUSTMENT(canvas->scrollbar_adjustment)->value = zp_start.z;
  
  /* allright, we need to update widgets connected to the adjustment without triggering our callback */
  gtk_signal_handler_block_by_func(canvas->scrollbar_adjustment, 
				   GTK_SIGNAL_FUNC(canvas_scrollbar_cb), canvas);
  gtk_adjustment_changed(GTK_ADJUSTMENT(canvas->scrollbar_adjustment));  
  gtk_signal_handler_unblock_by_func(canvas->scrollbar_adjustment, 
				     GTK_SIGNAL_FUNC(canvas_scrollbar_cb), canvas);

  return;
}



/* function to update the target cross on the canvas */
static void canvas_update_cross(AmitkCanvas * canvas, amitk_canvas_cross_action_t action, 
				realpoint_t center, rgba_t outline_color, floatpoint_t thickness) {

  GnomeCanvasPoints * points[4];
  canvaspoint_t point0, point1;
  realpoint_t start, end;
  gint i;

  if (action == AMITK_CANVAS_CROSS_HIDE) {
    for (i=0; i < 4 ; i++) 
      if (canvas->cross[i] != NULL)
	gnome_canvas_item_hide(canvas->cross[i]);
    return;
  }
  
  /* figure out some info */
  start = realspace_base_coord_to_alt(center, canvas->coord_frame);
  start.x -= thickness/2.0;
  start.y -= thickness/2.0;
  end = realspace_base_coord_to_alt(center, canvas->coord_frame);
  end.x += thickness/2.0;
  end.y += thickness/2.0;

  /* get the canvas locations corresponding to the start and end coordinates */
  point0 = rp_2_cp(canvas, start);
  point1 = rp_2_cp(canvas, end);
    
  points[0] = gnome_canvas_points_new(3);
  points[0]->coords[0] = (double) CANVAS_TRIANGLE_HEIGHT;
  points[0]->coords[1] = point1.y;
  points[0]->coords[2] = point0.x;
  points[0]->coords[3] = point1.y;
  points[0]->coords[4] = point0.x;
  points[0]->coords[5] = (double) CANVAS_TRIANGLE_HEIGHT;
    
  points[1] = gnome_canvas_points_new(3);
  points[1]->coords[0] = point1.x;
  points[1]->coords[1] = (double) CANVAS_TRIANGLE_HEIGHT;
  points[1]->coords[2] = point1.x;
  points[1]->coords[3] = point1.y;
  points[1]->coords[4] = (double) (canvas->pixbuf_width+CANVAS_TRIANGLE_HEIGHT);
  points[1]->coords[5] = point1.y;
  
  points[2] = gnome_canvas_points_new(3);
  points[2]->coords[0] = (double) CANVAS_TRIANGLE_HEIGHT;
  points[2]->coords[1] = point0.y;
  points[2]->coords[2] = point0.x;
  points[2]->coords[3] = point0.y;
  points[2]->coords[4] = point0.x;
  points[2]->coords[5] = (double) (canvas->pixbuf_height+CANVAS_TRIANGLE_HEIGHT);
  
  points[3] = gnome_canvas_points_new(3);
  points[3]->coords[0] = point1.x;
  points[3]->coords[1] = (double) (canvas->pixbuf_height+CANVAS_TRIANGLE_HEIGHT);
  points[3]->coords[2] = point1.x;
  points[3]->coords[3] = point0.y;
  points[3]->coords[4] = (double) (canvas->pixbuf_width+CANVAS_TRIANGLE_HEIGHT);
  points[3]->coords[5] = point0.y;

  for (i=0; i<4; i++) {
    if (canvas->cross[i]==NULL)
      canvas->cross[i] =
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
			      gnome_canvas_line_get_type(),
			      "points", points[i], 
			      "fill_color_rgba", color_table_rgba_to_uint32(outline_color),
			      "width_pixels", 1, NULL);
    else if (action == AMITK_CANVAS_CROSS_SHOW)
      gnome_canvas_item_set(canvas->cross[i],"points",points[i], 
			    "fill_color_rgba", color_table_rgba_to_uint32(outline_color),
			    "width_pixels", 1, NULL);
    else 
      gnome_canvas_item_set(canvas->cross[i],"points",points[i], NULL);
    gnome_canvas_item_show(canvas->cross[i]);
    gnome_canvas_points_unref(points[i]);
  }

  return;
}




/* function to update the arrows on the canvas */
static void canvas_update_arrows(AmitkCanvas * canvas, realpoint_t center, floatpoint_t thickness) {

  GnomeCanvasPoints * points[4];
  canvaspoint_t point0, point1;
  realpoint_t start, end;
  gint i;

  if (canvas->volumes == NULL) {
    for (i=0; i<4; i++)
      if (canvas->arrows[i] != NULL)
	gnome_canvas_item_hide(canvas->arrows[i]);
    return;
  }

  /* figure out the dimensions of the view "box" */
  start = realspace_base_coord_to_alt(center, canvas->coord_frame);
  start.x -= thickness/2.0;
  start.y -= thickness/2.0;
  end = realspace_base_coord_to_alt(center, canvas->coord_frame);
  end.x += thickness/2.0;
  end.y += thickness/2.0;
  
  /* get the canvas locations corresponding to the start and end coordinates */
  point0 = rp_2_cp(canvas, start);
  point1 = rp_2_cp(canvas, end);

  /* notes:
     1) even coords are the x coordinate, odd coords are the y
     2) drawing coordinate frame starts from the top left
     3) X's origin is top left, ours is bottom left
  */

  /* left arrow */
  points[0] = gnome_canvas_points_new(4);
  points[0]->coords[0] = CANVAS_TRIANGLE_HEIGHT-1.0;
  points[0]->coords[1] = point1.y;
  points[0]->coords[2] = CANVAS_TRIANGLE_HEIGHT-1.0;
  points[0]->coords[3] = point0.y;
  points[0]->coords[4] = 0;
  points[0]->coords[5] = point0.y + CANVAS_TRIANGLE_WIDTH/2.0;
  points[0]->coords[6] = 0;
  points[0]->coords[7] = point1.y - CANVAS_TRIANGLE_WIDTH/2.0;

  /* top arrow */
  points[1] = gnome_canvas_points_new(4);
  points[1]->coords[0] = point0.x;
  points[1]->coords[1] = CANVAS_TRIANGLE_HEIGHT-1.0;
  points[1]->coords[2] = point1.x;
  points[1]->coords[3] = CANVAS_TRIANGLE_HEIGHT-1.0;
  points[1]->coords[4] = point1.x + CANVAS_TRIANGLE_WIDTH/2.0;
  points[1]->coords[5] = 0;
  points[1]->coords[6] = point0.x - CANVAS_TRIANGLE_WIDTH/2.0;
  points[1]->coords[7] = 0;

  /* right arrow */
  points[2] = gnome_canvas_points_new(4);
  points[2]->coords[0] = CANVAS_TRIANGLE_HEIGHT + canvas->pixbuf_width;
  points[2]->coords[1] = point1.y;
  points[2]->coords[2] = CANVAS_TRIANGLE_HEIGHT + canvas->pixbuf_width;
  points[2]->coords[3] = point0.y;
  points[2]->coords[4] = 2*(CANVAS_TRIANGLE_HEIGHT)-1.0 + canvas->pixbuf_width;
  points[2]->coords[5] = point0.y + CANVAS_TRIANGLE_WIDTH/2;
  points[2]->coords[6] = 2*(CANVAS_TRIANGLE_HEIGHT)-1.0 + canvas->pixbuf_width;
  points[2]->coords[7] = point1.y - CANVAS_TRIANGLE_WIDTH/2;

  /* bottom arrow */
  points[3] = gnome_canvas_points_new(4);
  points[3]->coords[0] = point0.x;
  points[3]->coords[1] = CANVAS_TRIANGLE_HEIGHT + canvas->pixbuf_height;
  points[3]->coords[2] = point1.x;
  points[3]->coords[3] = CANVAS_TRIANGLE_HEIGHT + canvas->pixbuf_height;
  points[3]->coords[4] = point1.x + CANVAS_TRIANGLE_WIDTH/2;
  points[3]->coords[5] = 2*(CANVAS_TRIANGLE_HEIGHT)-1.0 + canvas->pixbuf_height;
  points[3]->coords[6] = point0.x - CANVAS_TRIANGLE_WIDTH/2;
  points[3]->coords[7] =  2*(CANVAS_TRIANGLE_HEIGHT)-1.0 + canvas->pixbuf_height;


  for (i=0; i<4; i++) {
    if (canvas->arrows[i] != NULL ) 
      gnome_canvas_item_set(canvas->arrows[i],"points",points[i], NULL);
    else 
      canvas->arrows[i] =
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
			      gnome_canvas_polygon_get_type(),
			      "points", points[i],"fill_color", "white",
			      "outline_color", "black", "width_pixels", 2,
			      NULL);
    gnome_canvas_item_show(canvas->arrows[i]);
    gnome_canvas_points_unref(points[i]);
  }

  return;
}








static void canvas_update_pixbuf(AmitkCanvas * canvas) {

  realpoint_t temp_rp;
  gint old_width, old_height;
  rgba_t blank_rgba;
  GtkStyle * widget_style;
  realpoint_t temp_corner[2];
  floatpoint_t voxel_dim;
  
  if (canvas->pixbuf != NULL) {
    old_width = canvas->pixbuf_width;
    old_height = canvas->pixbuf_height;
    gdk_pixbuf_unref(canvas->pixbuf);
  } else {
    old_width = CANVAS_BLANK_WIDTH;
    old_height = CANVAS_BLANK_HEIGHT;
  }

  if (canvas->volumes == NULL) {
    /* just use a blank image */

    /* figure out what color to use */
    widget_style = gtk_widget_get_style(GTK_WIDGET(canvas));
    if (widget_style == NULL) {
      g_warning("%s: Canvas has no style?\n",PACKAGE);
      widget_style = gtk_style_new();
    }

    blank_rgba.r = widget_style->bg[GTK_STATE_NORMAL].red >> 8;
    blank_rgba.g = widget_style->bg[GTK_STATE_NORMAL].green >> 8;
    blank_rgba.b = widget_style->bg[GTK_STATE_NORMAL].blue >> 8;
    blank_rgba.a = 0xFF;

    canvas->pixbuf = image_blank(old_width,old_height, blank_rgba);

  } else {
    /* update the coord frame appropriate */
    temp_rp.x = temp_rp.y = temp_rp.z = 0.5*canvas->thickness;
    temp_rp = rp_sub(canvas->center, temp_rp);
    rs_set_offset(canvas->coord_frame, temp_rp);

    /* figure out what voxel size we want to use */
    voxel_dim = (1/canvas->zoom) * canvas->voxel_dim; /* compensate for zoom */
    
    canvas->pixbuf = image_from_volumes(&(canvas->slices),
					canvas->volumes,
					canvas->start_time,
					canvas->duration,
					canvas->thickness,
					voxel_dim,
					canvas->coord_frame,
					canvas->interpolation);

    canvas->voxel_size.x = canvas->voxel_size.y = voxel_dim;
    canvas->voxel_size.z = canvas->thickness;

    /* figure out the corners */
    volumes_get_view_corners(canvas->slices, canvas->coord_frame, temp_corner);

    /* allright, reset the offset of the canvas coord frame to the 
       lower left front corner and the far corner*/
    temp_corner[1] = realspace_alt_coord_to_base(temp_corner[1], canvas->coord_frame);
    temp_corner[0] = realspace_alt_coord_to_base(temp_corner[0], canvas->coord_frame);

    rs_set_offset(canvas->coord_frame, temp_corner[0]);
    canvas->corner = realspace_base_coord_to_alt(temp_corner[1], canvas->coord_frame);
  }

  /* record the width and height for future use*/
  canvas->pixbuf_width = gdk_pixbuf_get_width(canvas->pixbuf);
  canvas->pixbuf_height = gdk_pixbuf_get_height(canvas->pixbuf);

  /* reset the min size of the widget and set the scroll region */
  if ((old_width != canvas->pixbuf_width) || 
      (old_height != canvas->pixbuf_height) || (canvas->image == NULL)) {
    gtk_widget_set_usize(canvas->canvas, 
			 canvas->pixbuf_width + 2 * CANVAS_TRIANGLE_HEIGHT, 
			 canvas->pixbuf_height + 2 * CANVAS_TRIANGLE_HEIGHT);
    gnome_canvas_set_scroll_region(GNOME_CANVAS(canvas->canvas), 0.0, 0.0, 
				   canvas->pixbuf_width + 2 * CANVAS_TRIANGLE_HEIGHT,
				   canvas->pixbuf_height + 2 * CANVAS_TRIANGLE_HEIGHT);
  }
	
  /* put the canvas rgb image on the canvas_image */
  if (canvas->image == NULL) {/* time to make a new image */
    canvas->image = gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
					  gnome_canvas_pixbuf_get_type(),
					  "pixbuf", canvas->pixbuf,
					  "x", (double) CANVAS_TRIANGLE_HEIGHT,
					  "y", (double) CANVAS_TRIANGLE_HEIGHT,
					  NULL);

  } else {
    gnome_canvas_item_set(canvas->image, "pixbuf", canvas->pixbuf,  NULL);
  }

  return;
}


/* if pt_item is declared, align_pt and parent_vol will be ignored,
   otherwise, align_pt and parent_vol will be used for creating a new pt_item */
static GnomeCanvasItem * canvas_update_align_pt(AmitkCanvas * canvas, 
						GnomeCanvasItem * pt_item,
						align_pt_t * align_pt,
						volume_t * parent_vol) {

  rgba_t outline_color;
  GnomeCanvasPoints * points;
  gdouble affine[6];
  realpoint_t center_rp;
  canvaspoint_t center_cp;

  if (pt_item != NULL)
    align_pt = gtk_object_get_data(GTK_OBJECT(pt_item), "object");
  g_return_val_if_fail(align_pt != NULL, NULL);

  if (pt_item != NULL)
    parent_vol = gtk_object_get_data(GTK_OBJECT(pt_item), "parent_object");
  g_return_val_if_fail(parent_vol != NULL, NULL);

  outline_color = color_table_outline_color(parent_vol->color_table, TRUE);

  center_rp = realspace_alt_coord_to_alt(align_pt->point, 
					 parent_vol->coord_frame,
					 canvas->coord_frame);

  center_cp = rp_2_cp(canvas, center_rp);
  points = gnome_canvas_points_new(7);
  points->coords[0] = center_cp.x-CANVAS_ALIGN_PT_WIDTH;
  points->coords[1] = center_cp.y;
  points->coords[2] = center_cp.x;
  points->coords[3] = center_cp.y;
  points->coords[4] = center_cp.x;
  points->coords[5] = center_cp.y+CANVAS_ALIGN_PT_WIDTH;
  points->coords[6] = center_cp.x;
  points->coords[7] = center_cp.y;
  points->coords[8] = center_cp.x+CANVAS_ALIGN_PT_WIDTH;
  points->coords[9] = center_cp.y;
  points->coords[10] = center_cp.x;
  points->coords[11] = center_cp.y;
  points->coords[12] = center_cp.x;
  points->coords[13] = center_cp.y-CANVAS_ALIGN_PT_WIDTH;

  if (pt_item == NULL) {
    pt_item = gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
				    gnome_canvas_line_get_type(), "points", points,
				    "fill_color_rgba", color_table_rgba_to_uint32(outline_color),
				    "width_pixels", CANVAS_ALIGN_PT_WIDTH_PIXELS,
				    "line_style", CANVAS_ALIGN_PT_LINE_STYLE, NULL);
    gtk_object_set_data(GTK_OBJECT(pt_item), "parent_object", parent_vol);
    gtk_object_set_data(GTK_OBJECT(pt_item), "object", align_pt);
    gtk_object_set_data(GTK_OBJECT(pt_item), "type", GINT_TO_POINTER(ALIGN_PT));
    gtk_signal_connect(GTK_OBJECT(pt_item), "event", GTK_SIGNAL_FUNC(canvas_event_cb), canvas);
  } else {
    /* make sure to reset any affine translations we've done */
    gnome_canvas_item_i2w_affine(pt_item,affine);
    affine[0] = affine[3] = 1.0;
    affine[1] = affine[2] = affine[4] = affine[5] = affine[6] = 0.0;
    gnome_canvas_item_affine_absolute(pt_item,affine);
    
    gnome_canvas_item_set(pt_item, "points", points,
			  "fill_color_rgba", color_table_rgba_to_uint32(outline_color), NULL);
  }

  gnome_canvas_points_unref(points);

  /* make sure the point is on this slice */
  if ((center_rp.z < 0.0) || (center_rp.z > canvas->corner.z) || (canvas->volumes == NULL))
    gnome_canvas_item_hide(pt_item);
  else
    gnome_canvas_item_show(pt_item);

  return pt_item;
}

static void canvas_update_align_pts(AmitkCanvas * canvas) {

  GList * pt_items;
  GnomeCanvasItem * pt_item;

  pt_items = canvas->pt_items;

  while (pt_items != NULL) {
    pt_item = pt_items->data;
    canvas_update_align_pt(canvas, pt_item, NULL, NULL);
    pt_items = pt_items->next;
  }
}


/* if roi_item is declared, roi will be ignored,
   otherwise, roi will be used for creating the new roi_item */
static GnomeCanvasItem * canvas_update_roi(AmitkCanvas * canvas, 
					   GnomeCanvasItem * roi_item,
					   roi_t * roi) {

  rgba_t outline_color;
  gdouble affine[6];
  GnomeCanvasPoints * points;
  GSList * roi_points, * temp;
  guint num_points, j;
  canvaspoint_t roi_cp;
  GdkPixbuf * rgb_image;
  canvaspoint_t offset_cp;
  realpoint_t corner_rp;
  canvaspoint_t corner_cp;
  realspace_t * coord_frame;
  gboolean hide_roi = FALSE;


  if (roi_item != NULL)
    roi = gtk_object_get_data(GTK_OBJECT(roi_item), "object");
  g_return_val_if_fail(roi != NULL, NULL);

  outline_color = color_table_outline_color(canvas->color_table, TRUE);

  switch(roi->type) {
  case ISOCONTOUR_2D:
  case ISOCONTOUR_3D:

    if (roi_item != NULL) {
      rgb_image = gtk_object_get_data(GTK_OBJECT(roi_item), "rgb_image");
      gdk_pixbuf_unref(rgb_image);
    }
    
    rgb_image = image_slice_intersection(roi, 
					 canvas->coord_frame,
					 canvas->corner,
					 canvas->voxel_size,
					 outline_color,
					 &coord_frame,
					 &corner_rp);

    offset_cp = 
      rp_2_cp(canvas, realspace_base_coord_to_alt(rs_offset(coord_frame), canvas->coord_frame));
    corner_cp = 
      rp_2_cp(canvas, realspace_alt_coord_to_alt(corner_rp, coord_frame, canvas->coord_frame));
    coord_frame = rs_unref(coord_frame);

    /* find the north west corner (in terms of the X reference frame) */
    if (corner_cp.y < offset_cp.y) offset_cp.y = corner_cp.y;
    if (corner_cp.x < offset_cp.x) offset_cp.x = corner_cp.x;

    /* create the item */ 
    if (roi_item == NULL) {
      roi_item =  gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
					gnome_canvas_pixbuf_get_type(),
					"pixbuf", rgb_image,
					"x", (double) offset_cp.x,
					"y", (double) offset_cp.y,
					NULL);

      gtk_object_set_data(GTK_OBJECT(roi_item), "object", roi);
      gtk_object_set_data(GTK_OBJECT(roi_item), "type", GINT_TO_POINTER(ROI));
      gtk_signal_connect(GTK_OBJECT(roi_item), "event", GTK_SIGNAL_FUNC(canvas_event_cb), canvas);
    } else {
      /* make sure to reset any affine translations we've done */
      gnome_canvas_item_i2w_affine(GNOME_CANVAS_ITEM(roi_item),affine);
      affine[0] = affine[3] = 1.0;
      affine[1] = affine[2] = affine[4] = affine[5] = affine[6] = 0.0;
      gnome_canvas_item_affine_absolute(GNOME_CANVAS_ITEM(roi_item),affine);

      gnome_canvas_item_set(roi_item, "pixbuf", rgb_image, 
			    "x", (double) offset_cp.x,
			    "y", (double) offset_cp.y,
			    NULL);
    }

    gtk_object_set_data(GTK_OBJECT(roi_item), "rgb_image", rgb_image);

    break;
  case ELLIPSOID:
  case CYLINDER:
  case BOX:
  default:
    /* get the points */
    roi_points =  roi_get_intersection_line(roi, canvas->voxel_size,
					    canvas->coord_frame, canvas->corner);
    
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
	roi_cp = rp_2_cp(canvas, *((realpoint_t *) temp->data));
	points->coords[j] = roi_cp.x;
	points->coords[j+1] = roi_cp.y;
	temp=temp->next;
	j += 2;
      }
    } else {
      /* throw in junk we'll hide*/
      hide_roi = TRUE;
      points = gnome_canvas_points_new(4);
      points->coords[0] = points->coords[1] = 0;
      points->coords[2] = points->coords[3] = 1;
    }
    roi_points = roi_free_points_list(roi_points);

    if (roi_item == NULL) {   /* create the item */ 
      roi_item =  gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
					gnome_canvas_line_get_type(),
					"points", points,
					"fill_color_rgba", color_table_rgba_to_uint32(outline_color),
					"width_pixels", canvas->roi_width,
					"line_style", canvas->line_style,
					NULL);
      
      gtk_object_set_data(GTK_OBJECT(roi_item), "object", roi);
      gtk_object_set_data(GTK_OBJECT(roi_item), "type", GINT_TO_POINTER(ROI));
      gtk_signal_connect(GTK_OBJECT(roi_item), "event", GTK_SIGNAL_FUNC(canvas_event_cb), canvas);
    } else {
      /* make sure to reset any affine translations we've done */
      gnome_canvas_item_i2w_affine(GNOME_CANVAS_ITEM(roi_item),affine);
      affine[0] = affine[3] = 1.0;
      affine[1] = affine[2] = affine[4] = affine[5] = affine[6] = 0.0;
      gnome_canvas_item_affine_absolute(GNOME_CANVAS_ITEM(roi_item),affine);
      
      /* and reset the line points */
      gnome_canvas_item_set(roi_item, 
			    "points", points, 
			    "fill_color_rgba", color_table_rgba_to_uint32(outline_color),
			    "width_pixels", canvas->roi_width,
			    "line_style", canvas->line_style,
			    NULL);
    }
    gnome_canvas_points_unref(points);
    
    break;
  }
 
  /* make sure the point is on this canvas */
  if (hide_roi || (canvas->volumes == NULL))
    gnome_canvas_item_hide(roi_item);
  else
    gnome_canvas_item_show(roi_item);
    


  return roi_item;

}



static void canvas_update_rois(AmitkCanvas * canvas) {

  GList * roi_items;
  GnomeCanvasItem * roi_item;

  roi_items = canvas->roi_items;

  while (roi_items != NULL) {
    roi_item = roi_items->data;
    canvas_update_roi(canvas, roi_item, NULL);
    roi_items = roi_items->next;
  }
}



static void canvas_update_setup(AmitkCanvas * canvas) {

  gboolean first_time = FALSE;

  if (canvas->canvas != NULL) {
    /* add a ref so they aren't destroyed when removed from the container */
    gtk_object_ref(GTK_OBJECT(canvas->label));
    gtk_object_ref(GTK_OBJECT(canvas->canvas));
    gtk_object_ref(GTK_OBJECT(canvas->scrollbar));

    gtk_container_remove(GTK_CONTAINER(canvas), canvas->label);
    gtk_container_remove(GTK_CONTAINER(canvas), canvas->canvas);
    gtk_container_remove(GTK_CONTAINER(canvas), canvas->scrollbar);

  } else {
    first_time = TRUE;

    canvas->label = gtk_label_new(view_names[canvas->view]);

    //gtk_widget_push_visual (gdk_rgb_get_visual ());
    //gtk_widget_push_colormap (gdk_rgb_get_cmap ());
    //    canvas->canvas = gnome_canvas_new_aa ();
    //gtk_widget_pop_colormap ();
    //gtk_widget_pop_visual ();
    canvas->canvas = gnome_canvas_new();
    gtk_object_set_data(GTK_OBJECT(canvas->canvas), "type", GINT_TO_POINTER(VOLUME));
    gtk_signal_connect(GTK_OBJECT(canvas->canvas), "event", GTK_SIGNAL_FUNC(canvas_event_cb), canvas);

    canvas->scrollbar_adjustment = gtk_adjustment_new(0.5, 0, 1, 1, 1, 1); /* junk values */
    gtk_signal_connect(canvas->scrollbar_adjustment, "value_changed", 
    		       GTK_SIGNAL_FUNC(canvas_scrollbar_cb), canvas);
    canvas->scrollbar = gtk_hscrollbar_new(GTK_ADJUSTMENT(canvas->scrollbar_adjustment));
    gtk_range_set_update_policy(GTK_RANGE(canvas->scrollbar), GTK_UPDATE_CONTINUOUS);

  }

  /* pack it in based on what the layout is */
  switch(canvas->layout) {


  case ORTHOGONAL_LAYOUT:
    switch(canvas->view) {
    case CORONAL:
      gtk_box_pack_start(GTK_BOX(canvas), canvas->canvas, TRUE, TRUE, BOX_SPACING);
      gtk_box_pack_start(GTK_BOX(canvas), canvas->scrollbar, FALSE, FALSE, BOX_SPACING);
      gtk_box_pack_start(GTK_BOX(canvas), canvas->label, FALSE, FALSE, BOX_SPACING);
      break;
    case TRANSVERSE:
    case SAGITTAL:
    default:
      gtk_box_pack_start(GTK_BOX(canvas), canvas->label, FALSE, FALSE, BOX_SPACING);
      gtk_box_pack_start(GTK_BOX(canvas), canvas->scrollbar, FALSE, FALSE, BOX_SPACING);
      gtk_box_pack_start(GTK_BOX(canvas), canvas->canvas, TRUE,TRUE,BOX_SPACING);
      break;
    }
    break;


  case LINEAR_LAYOUT:
  default:
    gtk_box_pack_start(GTK_BOX(canvas), canvas->label, FALSE, FALSE, BOX_SPACING);
    gtk_box_pack_start(GTK_BOX(canvas), canvas->canvas, TRUE,TRUE,BOX_SPACING);
    gtk_box_pack_start(GTK_BOX(canvas), canvas->scrollbar, FALSE, FALSE,BOX_SPACING);
    break;
  }



  if (first_time) {
    canvas_update(canvas, UPDATE_ALL);
  } else {
    if (canvas->view == SAGITTAL)
      canvas_update(canvas, UPDATE_ALL);
    gtk_object_unref(GTK_OBJECT(canvas->label));
    gtk_object_unref(GTK_OBJECT(canvas->canvas));
    gtk_object_unref(GTK_OBJECT(canvas->scrollbar));
  }


}


static void canvas_update(AmitkCanvas * canvas, update_t update) {

  //  realpoint_t view_corner[2];
  //  realpoint_t temp_center;

  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(canvas));

  /* make sure the study coord_frame offset is set correctly, 
     adjust current_view_center if necessary */
  //  if (canvas->volumes != NULL) {
  //    temp_center = realspace_alt_coord_to_base(study_view_center(canvas->study),
  //  					      study_coord_frame(canvas->study));
  //    volumes_get_view_corners(study_volumes(canvas->study), 
  //    			     study_coord_frame(canvas->study), view_corner);
  //    view_corner[0] = realspace_alt_coord_to_base(view_corner[0], study_coord_frame(canvas->study));
  //    study_set_coord_frame_offset(canvas->study, view_corner[0]);
  //    study_set_view_center(canvas->study, 
  //  			  realspace_base_coord_to_alt(temp_center, 
  //  						      study_coord_frame(canvas->study)));
  //  };

  switch (update) {
  case UPDATE_VOLUMES:
    /* freeing the slices indicates to regenerate them, and fallthrough to refresh */
    canvas->slices = volumes_unref(canvas->slices);
  case REFRESH_VOLUMES:
    /* refresh the image, but use the same slice as before */
    canvas_update_pixbuf(canvas);
    break;
  case UPDATE_ROIS: 
    canvas_update_rois(canvas);
    break;
  case UPDATE_ALIGN_PTS:
    canvas_update_align_pts(canvas);
    break;
  case UPDATE_ARROWS:
    canvas_update_arrows(canvas, canvas->center, canvas->thickness);
    break;
  case UPDATE_SCROLLBAR:
    canvas_update_scrollbar(canvas, canvas->center, canvas->thickness);
    break;
  case UPDATE_ALL:
  default:
    /* freeing the slices indicates to regenerate them */
    canvas->slices=volumes_unref(canvas->slices);
    canvas_update_pixbuf(canvas);
    canvas_update_scrollbar(canvas, canvas->center, canvas->thickness);
    canvas_update_arrows(canvas, canvas->center, canvas->thickness);
    canvas_update_rois(canvas);
    canvas_update_align_pts(canvas);
    break;
  }

  ui_common_remove_cursor(GTK_WIDGET(canvas));

  return;

}






GtkWidget * amitk_canvas_new(view_t view, 
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
			     gint roi_width) {

  AmitkCanvas * canvas;

  canvas = gtk_type_new (amitk_canvas_get_type ());

  canvas->view = view;
  canvas->center = center;
  canvas->thickness = thickness;
  canvas->voxel_dim = voxel_dim;
  canvas->zoom = zoom;
  canvas->start_time = start_time;
  canvas->duration = duration;
  canvas->interpolation = interpolation;
  canvas->line_style = line_style;
  canvas->roi_width = roi_width;
  canvas->color_table = color_table;
  canvas->layout = layout;
  canvas->coord_frame = realspace_get_view_coord_frame(view_coord_frame,view, layout);
  canvas_update_setup(canvas);

  return GTK_WIDGET (canvas);
}

void amitk_canvas_set_layout(AmitkCanvas * canvas, 
			     layout_t new_layout, 
			     realspace_t * view_coord_frame) {

  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));

  canvas->layout = new_layout;

  /* update the coord frame accordingly */
  canvas->coord_frame = rs_unref(canvas->coord_frame);
  canvas->coord_frame = realspace_get_view_coord_frame(view_coord_frame, canvas->view, canvas->layout);
  canvas_update_setup(canvas);

}

void amitk_canvas_set_coord_frame(AmitkCanvas * canvas, 
				  realspace_t * view_coord_frame,
				  gboolean update_now) {

  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));

  /* update the coord frame accordingly */
  canvas->coord_frame = rs_unref(canvas->coord_frame);
  canvas->coord_frame = realspace_get_view_coord_frame(view_coord_frame, canvas->view, canvas->layout);
  if (update_now) canvas_update(canvas, UPDATE_ALL);

}

void amitk_canvas_set_center(AmitkCanvas * canvas, 
			     realpoint_t center,
			     gboolean update_now) {

  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  canvas->center = center;
  if (update_now) canvas_update(canvas, UPDATE_ALL);
}

void amitk_canvas_set_thickness(AmitkCanvas * canvas, 
				floatpoint_t thickness,
				gboolean update_now) {

  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  canvas->thickness = thickness;
  if (update_now) canvas_update(canvas, UPDATE_ALL);
}

void amitk_canvas_set_voxel_dim(AmitkCanvas * canvas, 
				floatpoint_t base_voxel_dim,
				gboolean update_now) {

  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  canvas->voxel_dim = base_voxel_dim;
  if (update_now) canvas_update(canvas, UPDATE_ALL);
}

void amitk_canvas_set_zoom(AmitkCanvas * canvas, 
			   floatpoint_t zoom,
			   gboolean update_now) {

  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  canvas->zoom = zoom;
  if (update_now) canvas_update(canvas, UPDATE_ALL);
}

void amitk_canvas_set_start_time(AmitkCanvas * canvas, 
				 amide_time_t start_time,
				 gboolean update_now) {

  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  canvas->start_time = start_time;
  if (update_now) canvas_update(canvas, UPDATE_VOLUMES);
}

void amitk_canvas_set_duration(AmitkCanvas * canvas, 
			       amide_time_t duration,
			       gboolean update_now) {

  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  canvas->duration = duration;
  if (update_now) canvas_update(canvas, UPDATE_VOLUMES);
}

void amitk_canvas_set_interpolation(AmitkCanvas * canvas, 
				    interpolation_t interpolation,
				    gboolean update_now) {

  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  canvas->interpolation = interpolation;
  if (update_now) canvas_update(canvas, UPDATE_VOLUMES);
}

void amitk_canvas_set_color_table(AmitkCanvas * canvas, 
				  color_table_t color_table,
				  gboolean update_now) {

  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  canvas->color_table = color_table;
  if (update_now) canvas_update(canvas, UPDATE_ROIS);

}

void amitk_canvas_set_line_style(AmitkCanvas * canvas, 
				 GdkLineStyle new_line_style,
				 gboolean update_now) {

  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));

  canvas->line_style = new_line_style;
  if (update_now) canvas_update(canvas, UPDATE_ROIS);

}

void amitk_canvas_set_roi_width(AmitkCanvas * canvas, 
				gint new_roi_width,
				gboolean update_now) {

  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));

  canvas->roi_width = new_roi_width;
  if (update_now) canvas_update(canvas, UPDATE_ROIS);

}


void amitk_canvas_add_object(AmitkCanvas * canvas, 
			     gpointer object, 
			     object_t type, 
			     gpointer parent_object) {

  GnomeCanvasItem * item;

  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));

  switch(type) {
  case VOLUME:
    if (!volumes_includes_volume(canvas->volumes, object)) {
      canvas->volumes = volumes_add_volume(canvas->volumes, object);
      canvas_update(canvas, UPDATE_ALL);
    }
    break;
  case ALIGN_PT: 
    if (!align_pts_includes_pt(canvas->pts, object)) {
      item = canvas_update_align_pt(canvas, NULL, object, parent_object);
      canvas->pts = align_pts_add_pt(canvas->pts, object);
      canvas->pt_items = g_list_append(canvas->pt_items, item);
    }
    break;
  case ROI:
    if (!rois_includes_roi(canvas->rois, object)) {
      if (!roi_undrawn(object)) {
	item = canvas_update_roi(canvas, NULL, object);
	canvas->roi_items = g_list_append(canvas->roi_items, item);
      }
      canvas->rois = rois_add_roi(canvas->rois, object);
    }
    break;
  default:
    g_warning("%s: unexpected case in %s at line %d",PACKAGE, __FILE__, __LINE__);
    break;
  }

}

void amitk_canvas_remove_object(AmitkCanvas * canvas, 
				gpointer object, 
				object_t type,
				gboolean update_now) {

  GnomeCanvasItem * item;
  GnomeCanvasItem * found_item = NULL;

  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));

  switch(type) {
  case VOLUME: 
    {
      volume_t * volume = object;
      align_pts_t * align_pts = volume->align_pts;
      while (align_pts != NULL) {
	amitk_canvas_remove_object(canvas, align_pts->align_pt, ALIGN_PT, FALSE);
	align_pts = align_pts->next;
      }
      canvas->volumes = volumes_remove_volume(canvas->volumes, volume);
      if (update_now) canvas_update(canvas, UPDATE_ALL);
    }
    break;
  case ALIGN_PT:
    {
      GList * pt_items=canvas->pt_items;
      align_pt_t * align_pt;

      while (pt_items != NULL) {
	item = pt_items->data;
	align_pt = gtk_object_get_data(GTK_OBJECT(item), "object");
	if (align_pt == object)
	  found_item = item;
	pt_items = pt_items->next;
      }
      if (found_item != NULL) {
	gtk_object_destroy(GTK_OBJECT(found_item));
	canvas->pt_items = g_list_remove(canvas->pt_items, found_item);
      }

      canvas->pts = align_pts_remove_pt(canvas->pts, object);
    }
    break;
  case ROI:
    {
      GList * roi_items=canvas->roi_items;
      roi_t * roi;

      while (roi_items != NULL) {
	item = roi_items->data;
	roi = gtk_object_get_data(GTK_OBJECT(item), "object");
	if (roi == object)
	  found_item = item;
	roi_items = roi_items->next;
      }
      if (found_item != NULL) {
	gtk_object_destroy(GTK_OBJECT(found_item));
	canvas->roi_items = g_list_remove(canvas->roi_items, found_item);
      }

      canvas->rois = rois_remove_roi(canvas->rois, object);
    }
    break;
  default:
    g_warning("%s: unexpected case in %s at line %d",PACKAGE, __FILE__, __LINE__);
    break;
  }

}

void amitk_canvas_update_object(AmitkCanvas * canvas, gpointer object, object_t type) {

  GnomeCanvasItem * item;
  GList * items;
  gboolean found;

  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));

  switch(type) {
  case VOLUME: 
    /* yeah.... just have to update everything */
    canvas_update(canvas, UPDATE_ALL);
    break;
  case ROI:
    {
      roi_t * roi;

      items=canvas->roi_items;
      found=FALSE;
      while (items != NULL) {
	item = items->data;
	roi = gtk_object_get_data(GTK_OBJECT(item), "object");
	if (roi == object) {
	  found=TRUE;
	  canvas_update_roi(canvas, item, NULL);
	}
       items = items->next;
      }
      if (!found) /* updating an undrawn roi? */
	if (rois_includes_roi(canvas->rois, object)) {
	  amitk_canvas_remove_object(canvas, object,type, FALSE);
	  amitk_canvas_add_object(canvas, object,type, NULL);
	}
    }
    break;
  case ALIGN_PT:
    {
      align_pt_t * align_pt;

      items=canvas->pt_items;
      while (items != NULL) {
	item = items->data;
	align_pt = gtk_object_get_data(GTK_OBJECT(item), "object");
	if (align_pt == object)
	  canvas_update_align_pt(canvas, item, NULL, NULL);
	items = items->next;
      }
    }
    break;
  default:
    g_warning("%s: unexpected case in %s at line %d",PACKAGE, __FILE__, __LINE__);
    break;
  }

}




void amitk_canvas_threshold_changed(AmitkCanvas * canvas) {

  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  canvas_update(canvas, REFRESH_VOLUMES);
}


void amitk_canvas_update_scrollbar(AmitkCanvas * canvas) {
  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  canvas_update(canvas, UPDATE_SCROLLBAR);
}

void amitk_canvas_update_arrows(AmitkCanvas * canvas) {
  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  canvas_update(canvas, UPDATE_ARROWS);
}

void amitk_canvas_update_cross(AmitkCanvas * canvas, amitk_canvas_cross_action_t action, 
			       realpoint_t center, rgba_t outline_color, floatpoint_t thickness) {
  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  canvas_update_cross(canvas, action, center, outline_color, thickness);
}

void amitk_canvas_update_align_pts(AmitkCanvas * canvas) {
  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  canvas_update(canvas, UPDATE_ALIGN_PTS);
}

void amitk_canvas_update_rois(AmitkCanvas * canvas) {
  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  canvas_update(canvas, UPDATE_ROIS);
}

void amitk_canvas_update_volumes(AmitkCanvas * canvas) {
  g_return_if_fail(canvas != NULL);
  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  canvas_update(canvas, UPDATE_ALL);
}


