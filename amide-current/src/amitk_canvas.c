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


#include "amide_config.h"
#include <libgnomecanvas/gnome-canvas-pixbuf.h>
#include "amitk_canvas.h"
#include "image.h"
#include "ui_common.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"
#include "amitk_study.h"

#define BOX_SPACING 3
#define CANVAS_BLANK_WIDTH 128
#define CANVAS_BLANK_HEIGHT 128
#define CANVAS_TRIANGLE_WIDTH 13.0
#define CANVAS_TRIANGLE_HEIGHT 8.0
#define CANVAS_FIDUCIAL_MARK_WIDTH 4.0
#define CANVAS_FIDUCIAL_MARK_WIDTH_PIXELS 2
#define CANVAS_FIDUCIAL_MARK_LINE_STYLE GDK_LINE_SOLID


#define UPDATE_NONE 0
#define UPDATE_VIEW 0x1
#define UPDATE_ARROWS 0x2
#define REFRESH_DATA_SETS 0x4
#define UPDATE_DATA_SETS 0x8
#define UPDATE_SCROLLBAR 0x10
#define UPDATE_OBJECT 0x20
#define UPDATE_OBJECTS 0x40
#define UPDATE_CROSS 0x80
#define UPDATE_ALL 0xFF


enum {
  HELP_EVENT,
  VIEW_CHANGING,
  VIEW_CHANGED,
  ISOCONTOUR_3D_CHANGED,
  ERASE_VOLUME,
  NEW_OBJECT,
  LAST_SIGNAL
} amitk_canvas_signals;

typedef enum {
  CANVAS_EVENT_NONE,
  CANVAS_EVENT_ENTER_DATA_SET, 
  CANVAS_EVENT_ENTER_FIDUCIAL_MARK,
  CANVAS_EVENT_ENTER_NEW_NORMAL_ROI,
  CANVAS_EVENT_ENTER_NEW_ISOCONTOUR_ROI,
  CANVAS_EVENT_ENTER_NORMAL_ROI,
  CANVAS_EVENT_ENTER_ISOCONTOUR_ROI,
  CANVAS_EVENT_LEAVE, 
  CANVAS_EVENT_PRESS_MOVE_VIEW,
  CANVAS_EVENT_PRESS_MINIMIZE_VIEW,
  CANVAS_EVENT_PRESS_RESIZE_VIEW, /* 10 */
  CANVAS_EVENT_PRESS_SHIFT_DATA_SET, 
  CANVAS_EVENT_PRESS_ROTATE_DATA_SET,
  CANVAS_EVENT_PRESS_MOVE_FIDUCIAL_MARK,
  CANVAS_EVENT_PRESS_NEW_ROI,
  CANVAS_EVENT_PRESS_SHIFT_ROI,
  CANVAS_EVENT_PRESS_ROTATE_ROI,
  CANVAS_EVENT_PRESS_RESIZE_ROI,
  CANVAS_EVENT_PRESS_ERASE_ISOCONTOUR,
  CANVAS_EVENT_PRESS_LARGE_ERASE_ISOCONTOUR, /* 20 */
  CANVAS_EVENT_PRESS_CHANGE_ISOCONTOUR, 
  CANVAS_EVENT_PRESS_ERASE_VOLUME_INSIDE_ROI, 
  CANVAS_EVENT_PRESS_ERASE_VOLUME_OUTSIDE_ROI, 
  CANVAS_EVENT_PRESS_NEW_OBJECT, 
  CANVAS_EVENT_MOTION, 
  CANVAS_EVENT_MOTION_MOVE_VIEW,
  CANVAS_EVENT_MOTION_MINIMIZE_VIEW,
  CANVAS_EVENT_MOTION_RESIZE_VIEW, 
  CANVAS_EVENT_MOTION_SHIFT_DATA_SET, /* 30 */
  CANVAS_EVENT_MOTION_ROTATE_DATA_SET, 
  CANVAS_EVENT_MOTION_FIDUCIAL_MARK, 
  CANVAS_EVENT_MOTION_NEW_ROI,
  CANVAS_EVENT_MOTION_SHIFT_ROI, 
  CANVAS_EVENT_MOTION_ROTATE_ROI, 
  CANVAS_EVENT_MOTION_RESIZE_ROI, 
  CANVAS_EVENT_MOTION_ERASE_ISOCONTOUR,
  CANVAS_EVENT_MOTION_LARGE_ERASE_ISOCONTOUR,
  CANVAS_EVENT_MOTION_CHANGE_ISOCONTOUR, 
  CANVAS_EVENT_RELEASE_MOVE_VIEW, /* 40 */
  CANVAS_EVENT_RELEASE_MINIMIZE_VIEW, 
  CANVAS_EVENT_RELEASE_RESIZE_VIEW, 
  CANVAS_EVENT_RELEASE_SHIFT_DATA_SET,
  CANVAS_EVENT_RELEASE_ROTATE_DATA_SET,
  CANVAS_EVENT_RELEASE_FIDUCIAL_MARK,
  CANVAS_EVENT_RELEASE_NEW_ROI,
  CANVAS_EVENT_RELEASE_SHIFT_ROI,
  CANVAS_EVENT_RELEASE_ROTATE_ROI,
  CANVAS_EVENT_RELEASE_RESIZE_ROI, 
  CANVAS_EVENT_RELEASE_ERASE_ISOCONTOUR,
  CANVAS_EVENT_RELEASE_LARGE_ERASE_ISOCONTOUR,
  CANVAS_EVENT_RELEASE_CHANGE_ISOCONTOUR,
  CANVAS_EVENT_CANCEL_SHIFT_DATA_SET,
  CANVAS_EVENT_CANCEL_ROTATE_DATA_SET,
  CANVAS_EVENT_ENACT_SHIFT_DATA_SET,
  CANVAS_EVENT_ENACT_ROTATE_DATA_SET,
} canvas_event_t;


static void canvas_class_init (AmitkCanvasClass *klass);
static void canvas_init (AmitkCanvas *canvas);
static void canvas_destroy(GtkObject * object);

static GnomeCanvasItem * canvas_find_item(AmitkCanvas * canvas, AmitkObject * object);
static void common_changed_cb(AmitkCanvas * canvas, AmitkObject * object);
static void canvas_space_changed_cb(AmitkSpace * space, gpointer canvas);
static void canvas_study_changed_cb(AmitkStudy * study, gpointer data);
static void canvas_volume_changed_cb(AmitkVolume * vol, gpointer canvas);
static void canvas_roi_changed_cb(AmitkRoi * roi, gpointer canvas);
static void canvas_data_set_changed_cb(AmitkDataSet * ds, gpointer canvas);
static void canvas_thresholding_changed_cb(AmitkDataSet * ds, gpointer data);
static void canvas_color_table_changed_cb(AmitkDataSet * ds, gpointer data);
static void canvas_fiducial_mark_changed_cb(AmitkFiducialMark * mark, gpointer data);
static gboolean canvas_event_cb(GtkWidget* widget,  GdkEvent * event, gpointer data);
static void canvas_scrollbar_cb(GtkObject * adjustment, gpointer data);

static gboolean canvas_recalc_corners(AmitkCanvas * canvas);
static void canvas_update_scrollbar(AmitkCanvas * canvas, AmitkPoint center, amide_real_t thickness);
static void canvas_update_cross_immediate(AmitkCanvas * canvas, AmitkCanvasCrossAction action, 
					  AmitkPoint center, rgba_t color, amide_real_t thickness);
static void canvas_update_cross(AmitkCanvas * canvas, AmitkCanvasCrossAction action, 
				AmitkPoint center, rgba_t color, amide_real_t thickness);
static void canvas_update_arrows(AmitkCanvas * canvas, AmitkPoint center, amide_real_t thickness);
static void canvas_update_pixbuf(AmitkCanvas * canvas);
static GnomeCanvasItem * canvas_update_object(AmitkCanvas * canvas, 
					      GnomeCanvasItem * item,
					      AmitkObject * object);
static void canvas_update_objects(AmitkCanvas * canvas, gboolean all);
static void canvas_update_setup(AmitkCanvas * canvas);
static void canvas_add_update(AmitkCanvas * canvas, guint update_type);
static gboolean canvas_update_while_idle(gpointer canvas);


static GtkVBoxClass *canvas_parent_class;
static guint canvas_signals[LAST_SIGNAL];


GType amitk_canvas_get_type (void) {

  static GType canvas_type = 0;

  if (!canvas_type)
    {
      static const GTypeInfo canvas_info =
      {
	sizeof (AmitkCanvasClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) canvas_class_init,
	(GClassFinalizeFunc) NULL,
	NULL, /* class data */
	sizeof (AmitkCanvas),
	0,   /* # preallocs */
	(GInstanceInitFunc) canvas_init,
	NULL   /* value table */
      };

      canvas_type = g_type_register_static (GTK_TYPE_VBOX, "AmitkCanvas", &canvas_info, 0);
    }

  return canvas_type;
}

static void canvas_class_init (AmitkCanvasClass *klass)
{
  GObjectClass   *gobject_class;
  GtkObjectClass *gtkobject_class;
  GtkWidgetClass *widget_class;

  gobject_class =   (GObjectClass *) klass;
  gtkobject_class = (GtkObjectClass*) klass;
  widget_class =    (GtkWidgetClass*) klass;

  canvas_parent_class = g_type_class_peek_parent(klass);

  gtkobject_class->destroy = canvas_destroy;

  canvas_signals[HELP_EVENT] =
    g_signal_new ("help_event",
		  G_TYPE_FROM_CLASS(klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkCanvasClass, help_event),
		  NULL,NULL,
		  amitk_marshal_NONE__ENUM_BOXED_DOUBLE,
		  G_TYPE_NONE, 3,
		  AMITK_TYPE_HELP_INFO,
		  AMITK_TYPE_POINT,
		  AMITK_TYPE_DATA);
  canvas_signals[VIEW_CHANGING] =
    g_signal_new ("view_changing",
		  G_TYPE_FROM_CLASS(klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkCanvasClass, view_changing),
		  NULL,NULL,
		  amitk_marshal_NONE__BOXED_DOUBLE,
		  G_TYPE_NONE, 2,
		  AMITK_TYPE_POINT,
		  AMITK_TYPE_REAL);
  canvas_signals[VIEW_CHANGED] =
    g_signal_new ("view_changed",
		  G_TYPE_FROM_CLASS(klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkCanvasClass, view_changed),
		  NULL,NULL,
		  amitk_marshal_NONE__BOXED_DOUBLE,
		  G_TYPE_NONE, 2,
		  AMITK_TYPE_POINT,
		  AMITK_TYPE_REAL);
  canvas_signals[ISOCONTOUR_3D_CHANGED] =
    g_signal_new ("isocontour_3d_changed",
		  G_TYPE_FROM_CLASS(klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkCanvasClass, isocontour_3d_changed),
		  NULL,NULL,
		  amitk_marshal_NONE__OBJECT_BOXED,
		  G_TYPE_NONE, 2,
		  AMITK_TYPE_ROI,
		  AMITK_TYPE_POINT);
  canvas_signals[ERASE_VOLUME] =
    g_signal_new("erase_volume", 
		 G_TYPE_FROM_CLASS(klass),
		 G_SIGNAL_RUN_LAST,
		 G_STRUCT_OFFSET (AmitkCanvasClass, erase_volume),
		 NULL,NULL,
		 amitk_marshal_NONE__OBJECT_BOOLEAN,
		 G_TYPE_NONE, 2,
		 AMITK_TYPE_ROI,
		 G_TYPE_BOOLEAN);
  canvas_signals[NEW_OBJECT] =
    g_signal_new ("new_object",
		  G_TYPE_FROM_CLASS(klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (AmitkCanvasClass, new_object),
		  NULL,NULL,
		  amitk_marshal_NONE__OBJECT_ENUM_BOXED,
		  G_TYPE_NONE, 3,
		  AMITK_TYPE_OBJECT,
		  AMITK_TYPE_OBJECT_TYPE,
		  AMITK_TYPE_POINT);

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
  canvas->with_arrows = TRUE;

  canvas->volume = amitk_volume_new();
  amitk_volume_set_corner(canvas->volume, one_point);

  canvas->center = zero_point;

  canvas->active_ds = NULL;

  canvas->canvas = NULL;
  canvas->slices=NULL;
  canvas->image=NULL;
  canvas->pixbuf=NULL;

  canvas->pixbuf_width = CANVAS_BLANK_WIDTH;
  canvas->pixbuf_height = CANVAS_BLANK_HEIGHT;

  canvas->objects=NULL;
  canvas->object_items=NULL;

  canvas->next_update = 0;
  canvas->idle_handler_id = 0;
  canvas->next_update_items = NULL;

}

static void canvas_destroy (GtkObject * object) {

  AmitkCanvas * canvas;

  g_return_if_fail (object != NULL);
  g_return_if_fail (AMITK_IS_CANVAS (object));

  canvas = AMITK_CANVAS(object);

  while (canvas->objects != NULL)
    if (!amitk_canvas_remove_object(canvas, canvas->objects->data))
      break; /* avoid endless loop in event of error */

  if (canvas->idle_handler_id != 0) {
    gtk_idle_remove(canvas->idle_handler_id);
    canvas->idle_handler_id = 0;
  }

  if (canvas->volume != NULL) {
    g_object_unref(canvas->volume);
    canvas->volume = NULL;
  }

  if (canvas->slices != NULL) {
    amitk_objects_unref(canvas->slices);
    canvas->slices = NULL;
  }

  if (canvas->pixbuf != NULL) {
    g_object_unref(canvas->pixbuf);
    canvas->pixbuf = NULL;
  }

  if (GTK_OBJECT_CLASS (canvas_parent_class)->destroy)
    (* GTK_OBJECT_CLASS (canvas_parent_class)->destroy) (object);
}


static GnomeCanvasItem * canvas_find_item(AmitkCanvas * canvas, AmitkObject * object) {

  GList * items=canvas->object_items;

  while (items != NULL) {
    if (object == g_object_get_data(G_OBJECT(items->data), "object")) 
      return items->data;
    items = items->next;
  }

  return NULL;
}



/* this function converts a gnome canvas event location to a location in the canvas's coordinate space */
static AmitkPoint cp_2_p(AmitkCanvas * canvas, AmitkCanvasPoint canvas_cpoint) {

  AmitkPoint canvas_point;
  AmitkPoint corner;

  corner = AMITK_VOLUME_CORNER(canvas->volume);
  canvas_point.x = ((canvas_cpoint.x-CANVAS_TRIANGLE_HEIGHT)/canvas->pixbuf_width)*corner.x;
  canvas_point.y = ((canvas->pixbuf_height-(canvas_cpoint.y-CANVAS_TRIANGLE_HEIGHT))/
		 canvas->pixbuf_height)*corner.y;
  canvas_point.z = corner.z/2.0;

  /* make sure it's in the canvas */
  if (canvas_point.x < 0.0) canvas_point.x = 0.0;
  if (canvas_point.y < 0.0) canvas_point.y = 0.0;
  if (canvas_point.x > corner.x) canvas_point.x = corner.x;
  if (canvas_point.y > corner.y) canvas_point.y = corner.y;

  return canvas_point;
}


/* this function converts a point in the canvas's coordinate space to a gnome canvas event location */
static AmitkCanvasPoint p_2_cpoint(AmitkCanvas * canvas, AmitkPoint canvas_point) {

  AmitkCanvasPoint canvas_cpoint;
  AmitkPoint corner;

  corner = AMITK_VOLUME_CORNER(canvas->volume);
  canvas_cpoint.x = canvas->pixbuf_width * canvas_point.x/corner.x + CANVAS_TRIANGLE_HEIGHT;
  canvas_cpoint.y = canvas->pixbuf_height * (corner.y - canvas_point.y)/
    corner.y + CANVAS_TRIANGLE_HEIGHT;
  
  return canvas_cpoint;
}


static void common_changed_cb(AmitkCanvas * canvas, AmitkObject * object) {

  GnomeCanvasItem * found_item;

  if (AMITK_IS_DATA_SET(object) || AMITK_IS_STUDY(object)) 
    canvas_add_update(canvas, UPDATE_ALL);
  else {
    found_item = canvas_find_item(canvas, object);
    if (found_item) {
      canvas->next_update_items=g_list_append(canvas->next_update_items, found_item);
      canvas_add_update(canvas, UPDATE_OBJECT);
    } else { /* updating an undrawn roi? */
      if (AMITK_IS_ROI(object))
	if (AMITK_ROI_UNDRAWN(AMITK_ROI(object)))
	  if (g_list_index(canvas->objects, object) >= 0) {
	    amitk_canvas_remove_object(canvas, object);
	    amitk_canvas_add_object(canvas, object);
	  }
    }
  }

  return;
}

static void canvas_space_changed_cb(AmitkSpace * space, gpointer data) {

  AmitkObject * object;
  AmitkCanvas * canvas = data;  

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_OBJECT(space));
  object = AMITK_OBJECT(space);
  common_changed_cb(canvas, object);

  return;
}

static void canvas_study_changed_cb(AmitkStudy * study, gpointer data) {

  AmitkObject * object;
  AmitkCanvas * canvas = data;  
  gboolean changed=FALSE;
  gboolean coords_changed = TRUE;
  AmitkPoint temp[2];

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_OBJECT(study));

  /* make sure we get any changes in the study's view parameters */

  if (!REAL_EQUAL(AMITK_STUDY_VIEW_THICKNESS(study), AMITK_VOLUME_Z_CORNER(canvas->volume))) {
    amitk_volume_set_z_corner(canvas->volume, AMITK_STUDY_VIEW_THICKNESS(study));
    changed = TRUE;
  }

  temp[0] = amitk_space_b2s(AMITK_SPACE(canvas->volume), AMITK_STUDY_VIEW_CENTER(study));
  temp[1] = amitk_space_b2s(AMITK_SPACE(canvas->volume), canvas->center);
  if (!REAL_EQUAL(temp[0].z, temp[1].z))
    changed = TRUE; /* only need to update if z's changed */
  else if (!POINT_EQUAL(temp[0], temp[1]))
    coords_changed = TRUE;
  canvas->center = AMITK_STUDY_VIEW_CENTER(study);

  if (!REAL_EQUAL(canvas->zoom, AMITK_STUDY_ZOOM(study))) {
    canvas->zoom = AMITK_STUDY_ZOOM(study);
    changed = TRUE;
  }

  if (canvas->fuse_type != AMITK_STUDY_FUSE_TYPE(study)) {
    canvas->fuse_type = AMITK_STUDY_FUSE_TYPE(study);
    changed = TRUE;
  }

  if (!REAL_EQUAL(canvas->duration, AMITK_STUDY_VIEW_DURATION(study))) {
    canvas->duration = AMITK_STUDY_VIEW_DURATION(study);
    changed = TRUE;
  }

  if (!REAL_EQUAL(canvas->start_time, AMITK_STUDY_VIEW_START_TIME(study))) {
    canvas->start_time = AMITK_STUDY_VIEW_START_TIME(study);
    changed = TRUE;
  }

  if (!REAL_EQUAL(canvas->voxel_dim, AMITK_STUDY_VOXEL_DIM(study))) {
    canvas->voxel_dim = AMITK_STUDY_VOXEL_DIM(study);
    changed = TRUE;
  }

  if (changed) {
    object = AMITK_OBJECT(study);
    common_changed_cb(canvas, object);
  } else if (coords_changed) {
    canvas_add_update(canvas, UPDATE_ARROWS);
  }

  return;
}

static void canvas_volume_changed_cb(AmitkVolume * vol, gpointer data) {

  AmitkObject * object;
  AmitkCanvas * canvas = data;  

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_VOLUME(vol));
  object = AMITK_OBJECT(vol);
  common_changed_cb(canvas, object);


  return;
}

static void canvas_roi_changed_cb(AmitkRoi * roi, gpointer data) {

  AmitkObject * object;
  AmitkCanvas * canvas = data;  

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_ROI(roi));
  object = AMITK_OBJECT(roi);
  common_changed_cb(canvas, object);

  return;
}

static void canvas_data_set_changed_cb(AmitkDataSet * ds, gpointer data) {

  AmitkObject * object;
  AmitkCanvas * canvas = data;  

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  object = AMITK_OBJECT(ds);
  common_changed_cb(canvas, object);


  return;
}



static void canvas_thresholding_changed_cb(AmitkDataSet * ds, gpointer data) {

  AmitkCanvas * canvas = data;  

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  canvas_add_update(canvas, REFRESH_DATA_SETS);
}

static void canvas_color_table_changed_cb(AmitkDataSet * ds, gpointer data) {

  AmitkCanvas * canvas = data;  

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  canvas_add_update(canvas, REFRESH_DATA_SETS);
  canvas_add_update(canvas, UPDATE_OBJECTS);
}

static void canvas_fiducial_mark_changed_cb(AmitkFiducialMark * mark, gpointer data) {

  AmitkObject * object;
  AmitkCanvas * canvas = data;  

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_OBJECT(mark));
  object = AMITK_OBJECT(mark);
  common_changed_cb(canvas, object);


  return;
}

/* function called when an event occurs on the canvas */
static gboolean canvas_event_cb(GtkWidget* widget,  GdkEvent * event, gpointer data) {

  AmitkCanvas * canvas = data;
  AmitkPoint base_point, canvas_point, diff_point;
  AmitkCanvasPoint canvas_cpoint, diff_cpoint, event_cpoint;
  rgba_t outline_color;
  GnomeCanvasPoints * points;
  canvas_event_t canvas_event_type;
  AmitkObject * object;
  AmitkCanvasPoint temp_cpoint[2];
  AmitkPoint temp_point[2];
  AmitkPoint shift;
  AmitkDataSet * active_slice;
  AmitkVoxel temp_voxel;
  AmitkColorTable color_table;
  GList * objects;
  gboolean undrawn_roi;
  amide_real_t corner;
  AmitkPoint center;
  amide_data_t voxel_value;
  static GnomeCanvasItem * canvas_item = NULL;
  static gboolean grab_on = FALSE;
  static canvas_event_t extended_event_type = CANVAS_EVENT_NONE;
  static AmitkCanvasPoint initial_cpoint;
  static AmitkCanvasPoint previous_cpoint;
  static AmitkPoint initial_base_point;
  static AmitkPoint initial_canvas_point;
  static double theta;
  static AmitkPoint zoom;
  static AmitkPoint radius_point;
  static gboolean in_object=FALSE;

  //  if ((event->type == GDK_BUTTON_PRESS) ||
  //      (event->type == GDK_BUTTON_RELEASE) ||
  //      (event->type == GDK_2BUTTON_PRESS))
  //    g_print("event %d\n", event->type);

  if (canvas->slices == NULL) return FALSE;

  /* check if there are any undrawn roi's */
  object = NULL;
  objects = canvas->objects;
  while (objects != NULL) {
    if (AMITK_IS_ROI(objects->data))
      if (AMITK_ROI_UNDRAWN(AMITK_ROI(objects->data)))
	object = objects->data;
    objects = objects->next;
  }

  /* no undrawn roi's, see if we're on an object */
  if (!object) {
    object = g_object_get_data(G_OBJECT(widget), "object");
    undrawn_roi = FALSE;
  } else
    undrawn_roi = TRUE;

  /* try the active data set, and if not that, try any data set */
  if (object == NULL) {
    if (canvas->active_ds != NULL)
      object = AMITK_OBJECT(canvas->active_ds);
    else  /* just get the first slice's object */
      object = AMITK_OBJECT(AMITK_DATA_SET_SLICE_PARENT(canvas->slices->data)); 
  }
  g_return_val_if_fail(object != NULL, FALSE);
  
  if (canvas->active_ds == NULL)
    color_table = AMITK_COLOR_TABLE_BW_LINEAR;
  else
    color_table = AMITK_DATA_SET_COLOR_TABLE(canvas->active_ds);


  switch(event->type) {

  case GDK_ENTER_NOTIFY:
    event_cpoint.x = event->crossing.x;
    event_cpoint.y = event->crossing.y;
    if (event->crossing.mode != GDK_CROSSING_NORMAL) {
      canvas_event_type = CANVAS_EVENT_NONE; /* ignore grabs */

    } else if (undrawn_roi) {

      if ((AMITK_ROI_TYPE(object) == AMITK_ROI_TYPE_ISOCONTOUR_2D) || 
	  (AMITK_ROI_TYPE(object) == AMITK_ROI_TYPE_ISOCONTOUR_3D)) 
	canvas_event_type = CANVAS_EVENT_ENTER_NEW_ISOCONTOUR_ROI;      
      else canvas_event_type = CANVAS_EVENT_ENTER_NEW_NORMAL_ROI;      
      
    } else if (extended_event_type != CANVAS_EVENT_NONE) {
      canvas_event_type = CANVAS_EVENT_NONE;

    } else if (AMITK_IS_DATA_SET(object)) {
      canvas_event_type = CANVAS_EVENT_ENTER_DATA_SET;
      
    } else if (AMITK_IS_FIDUCIAL_MARK(object)) {
      canvas_event_type = CANVAS_EVENT_ENTER_FIDUCIAL_MARK;

    } else if (AMITK_IS_ROI(object)) { /* sanity check */
      if ((AMITK_ROI_TYPE(object) == AMITK_ROI_TYPE_ISOCONTOUR_2D) || 
	  (AMITK_ROI_TYPE(object) == AMITK_ROI_TYPE_ISOCONTOUR_3D))
	canvas_event_type = CANVAS_EVENT_ENTER_ISOCONTOUR_ROI;
      else  canvas_event_type = CANVAS_EVENT_ENTER_NORMAL_ROI;
      
    } else
      canvas_event_type = CANVAS_EVENT_NONE;
    
    break;
    
  case GDK_LEAVE_NOTIFY:
    event_cpoint.x = event->crossing.x;
    event_cpoint.y = event->crossing.y;
    if (event->crossing.mode != GDK_CROSSING_NORMAL)
      canvas_event_type = CANVAS_EVENT_NONE; /* ignore grabs */
    else if (extended_event_type != CANVAS_EVENT_NONE)
      canvas_event_type = CANVAS_EVENT_NONE; 
    else
      canvas_event_type = CANVAS_EVENT_LEAVE;
    break;

  case GDK_BUTTON_PRESS:
    event_cpoint.x = event->button.x;
    event_cpoint.y = event->button.y;

    if (undrawn_roi) {
      canvas_event_type = CANVAS_EVENT_PRESS_NEW_ROI;
      
    } else if (extended_event_type != CANVAS_EVENT_NONE) {
      canvas_event_type = extended_event_type;
      
    } else if ((!grab_on) && (!in_object) && (AMITK_IS_DATA_SET(object)) && (event->button.button == 1)) {
      if (event->button.state & GDK_SHIFT_MASK) 
	canvas_event_type = CANVAS_EVENT_PRESS_SHIFT_DATA_SET;
      else canvas_event_type = CANVAS_EVENT_PRESS_MOVE_VIEW;
      
    } else if ((!grab_on) && (!in_object) && (AMITK_IS_DATA_SET(object)) && (event->button.button == 2)) {
      if (event->button.state & GDK_SHIFT_MASK) 
	canvas_event_type = CANVAS_EVENT_PRESS_ROTATE_DATA_SET;
      else canvas_event_type = CANVAS_EVENT_PRESS_MINIMIZE_VIEW;
      
    } else if ((!grab_on) && (!in_object) && (AMITK_IS_DATA_SET(object)) && (event->button.button == 3)) {
      if (event->button.state & GDK_CONTROL_MASK)
	canvas_event_type = CANVAS_EVENT_PRESS_NEW_OBJECT;
      else canvas_event_type = CANVAS_EVENT_PRESS_RESIZE_VIEW;
      
    } else if ((!grab_on) && (AMITK_IS_ROI(object)) && (event->button.button == 1)) {
      canvas_event_type = CANVAS_EVENT_PRESS_SHIFT_ROI;
      
    } else if ((!grab_on) && (AMITK_IS_ROI(object)) && (event->button.button == 2)) {
      if ((AMITK_ROI_TYPE(object) != AMITK_ROI_TYPE_ISOCONTOUR_2D) && 
	  (AMITK_ROI_TYPE(object) != AMITK_ROI_TYPE_ISOCONTOUR_3D))
	canvas_event_type = CANVAS_EVENT_PRESS_ROTATE_ROI;
      else if (event->button.state & GDK_SHIFT_MASK)
	canvas_event_type = CANVAS_EVENT_PRESS_LARGE_ERASE_ISOCONTOUR;
      else
	canvas_event_type = CANVAS_EVENT_PRESS_ERASE_ISOCONTOUR;
      
    } else if ((!grab_on) && (AMITK_IS_ROI(object)) && (event->button.button == 3)) {
      if ((event->button.state & GDK_CONTROL_MASK) &&
	  (event->button.state & GDK_SHIFT_MASK))
	canvas_event_type = CANVAS_EVENT_PRESS_ERASE_VOLUME_OUTSIDE_ROI;
      else if (event->button.state & GDK_CONTROL_MASK)
	canvas_event_type = CANVAS_EVENT_PRESS_ERASE_VOLUME_INSIDE_ROI;
      else if ((AMITK_ROI_TYPE(object) != AMITK_ROI_TYPE_ISOCONTOUR_2D) && 
	  (AMITK_ROI_TYPE(object) != AMITK_ROI_TYPE_ISOCONTOUR_3D))
	canvas_event_type = CANVAS_EVENT_PRESS_RESIZE_ROI;
      else canvas_event_type = CANVAS_EVENT_PRESS_CHANGE_ISOCONTOUR;
      
    } else if ((!grab_on) && (AMITK_IS_FIDUCIAL_MARK(object)) && (event->button.button == 1)) {
      canvas_event_type = CANVAS_EVENT_PRESS_MOVE_FIDUCIAL_MARK;

    } else 
      canvas_event_type = CANVAS_EVENT_NONE;
    break;
    
  case GDK_MOTION_NOTIFY:
    event_cpoint.x = event->motion.x;
    event_cpoint.y = event->motion.y;

    if (grab_on && undrawn_roi) {
      canvas_event_type = CANVAS_EVENT_MOTION_NEW_ROI;
      
    } else if (extended_event_type != CANVAS_EVENT_NONE) {

      if (extended_event_type == CANVAS_EVENT_PRESS_ROTATE_DATA_SET)
	canvas_event_type = CANVAS_EVENT_MOTION_ROTATE_DATA_SET;
      else if (extended_event_type == CANVAS_EVENT_PRESS_SHIFT_DATA_SET)
	canvas_event_type = CANVAS_EVENT_MOTION_SHIFT_DATA_SET;
      else {
	canvas_event_type = CANVAS_EVENT_NONE;
	grab_on = FALSE;
	g_error("unexpected case in %s at line %d",  __FILE__, __LINE__);
      }

    } else if (grab_on && (!in_object) && (AMITK_IS_DATA_SET(object)) && (event->motion.state & GDK_BUTTON1_MASK)) {
      canvas_event_type = CANVAS_EVENT_MOTION_MOVE_VIEW;
      
    } else if (grab_on && (!in_object) && (AMITK_IS_DATA_SET(object)) && (event->motion.state & GDK_BUTTON2_MASK)) {
      canvas_event_type = CANVAS_EVENT_MOTION_MINIMIZE_VIEW;
      
    } else if (grab_on && (!in_object) && (AMITK_IS_DATA_SET(object)) && (event->motion.state & GDK_BUTTON3_MASK)) {
      canvas_event_type = CANVAS_EVENT_MOTION_RESIZE_VIEW;
      
    } else if (grab_on && (AMITK_IS_ROI(object)) && (event->motion.state & GDK_BUTTON1_MASK)) {
      canvas_event_type = CANVAS_EVENT_MOTION_SHIFT_ROI;
      
    } else if (grab_on && (AMITK_IS_ROI(object)) && (event->motion.state & GDK_BUTTON2_MASK)) {
      if ((AMITK_ROI_TYPE(object) != AMITK_ROI_TYPE_ISOCONTOUR_2D) && 
	  (AMITK_ROI_TYPE(object) != AMITK_ROI_TYPE_ISOCONTOUR_3D)) 
	canvas_event_type = CANVAS_EVENT_MOTION_ROTATE_ROI;
      else if (event->motion.state & GDK_SHIFT_MASK)
	canvas_event_type = CANVAS_EVENT_MOTION_LARGE_ERASE_ISOCONTOUR;
      else
	canvas_event_type = CANVAS_EVENT_MOTION_ERASE_ISOCONTOUR;
      
    } else if (grab_on && (AMITK_IS_ROI(object)) && (event->motion.state & GDK_BUTTON3_MASK)) {
      if ((AMITK_ROI_TYPE(object) != AMITK_ROI_TYPE_ISOCONTOUR_2D) && 
	  (AMITK_ROI_TYPE(object) != AMITK_ROI_TYPE_ISOCONTOUR_3D))
	canvas_event_type = CANVAS_EVENT_MOTION_RESIZE_ROI;
      else canvas_event_type = CANVAS_EVENT_MOTION_CHANGE_ISOCONTOUR;

    } else if (grab_on && (AMITK_IS_FIDUCIAL_MARK(object)) && (event->motion.state & GDK_BUTTON1_MASK)) {
      canvas_event_type = CANVAS_EVENT_MOTION_FIDUCIAL_MARK;

    } else 
      canvas_event_type = CANVAS_EVENT_MOTION;

    break;
    
  case GDK_BUTTON_RELEASE:
    event_cpoint.x = event->button.x;
    event_cpoint.y = event->button.y;

    if (undrawn_roi) {
      canvas_event_type = CANVAS_EVENT_RELEASE_NEW_ROI;
	
    } else if ((extended_event_type != CANVAS_EVENT_NONE) && (!grab_on) && (event->button.button == 3)) {

      if (extended_event_type == CANVAS_EVENT_PRESS_ROTATE_DATA_SET)
	canvas_event_type = CANVAS_EVENT_ENACT_ROTATE_DATA_SET;
      else if (extended_event_type == CANVAS_EVENT_PRESS_SHIFT_DATA_SET)
	canvas_event_type = CANVAS_EVENT_ENACT_SHIFT_DATA_SET;
      else {
	canvas_event_type = CANVAS_EVENT_NONE;
	grab_on = FALSE;
	g_error("unexpected case in %s at line %d",  __FILE__, __LINE__);
      }

    } else if ((extended_event_type != CANVAS_EVENT_NONE) && (!grab_on))  {

      if (extended_event_type == CANVAS_EVENT_PRESS_ROTATE_DATA_SET)
	canvas_event_type = CANVAS_EVENT_CANCEL_ROTATE_DATA_SET;
      else if (extended_event_type == CANVAS_EVENT_PRESS_SHIFT_DATA_SET)
	canvas_event_type = CANVAS_EVENT_CANCEL_SHIFT_DATA_SET;
      else {
	canvas_event_type = CANVAS_EVENT_NONE;
	grab_on = FALSE;
	g_error("unexpected case in %s at line %d",  __FILE__, __LINE__);
      }

    } else if ((extended_event_type != CANVAS_EVENT_NONE) && (grab_on)) {
      if (extended_event_type == CANVAS_EVENT_PRESS_ROTATE_DATA_SET)
	canvas_event_type = CANVAS_EVENT_RELEASE_ROTATE_DATA_SET;
      else if (extended_event_type == CANVAS_EVENT_PRESS_SHIFT_DATA_SET)
	canvas_event_type = CANVAS_EVENT_RELEASE_SHIFT_DATA_SET;
      else {
	canvas_event_type = CANVAS_EVENT_NONE;
	grab_on = FALSE;
	g_error("unexpected case in %s at line %d",  __FILE__, __LINE__);
      }

    } else if ((!in_object) && (AMITK_IS_DATA_SET(object)) && (event->button.button == 1)) {
      canvas_event_type = CANVAS_EVENT_RELEASE_MOVE_VIEW;
	
    } else if ((!in_object) && (AMITK_IS_DATA_SET(object)) && (event->button.button == 2)) {
      canvas_event_type = CANVAS_EVENT_RELEASE_MINIMIZE_VIEW;
      
    } else if ((!in_object) && (AMITK_IS_DATA_SET(object)) && (event->button.button == 3)) {
      canvas_event_type = CANVAS_EVENT_RELEASE_RESIZE_VIEW;
      
    } else if ((AMITK_IS_ROI(object)) && (event->button.button == 1)) {
      canvas_event_type = CANVAS_EVENT_RELEASE_SHIFT_ROI;
      
    } else if ((AMITK_IS_ROI(object)) && (event->button.button == 2)) {
      if ((AMITK_ROI_TYPE(object) != AMITK_ROI_TYPE_ISOCONTOUR_2D) && 
	  (AMITK_ROI_TYPE(object) != AMITK_ROI_TYPE_ISOCONTOUR_3D))
	canvas_event_type = CANVAS_EVENT_RELEASE_ROTATE_ROI;
      else if (event->button.state & GDK_SHIFT_MASK)
	canvas_event_type = CANVAS_EVENT_RELEASE_LARGE_ERASE_ISOCONTOUR;
      else 
	canvas_event_type = CANVAS_EVENT_RELEASE_ERASE_ISOCONTOUR;      

    } else if ((AMITK_IS_ROI(object)) && (event->button.button == 3)) {
      if ((AMITK_ROI_TYPE(object) != AMITK_ROI_TYPE_ISOCONTOUR_2D) && 
	  (AMITK_ROI_TYPE(object) != AMITK_ROI_TYPE_ISOCONTOUR_3D))
	canvas_event_type = CANVAS_EVENT_RELEASE_RESIZE_ROI;
      else canvas_event_type = CANVAS_EVENT_RELEASE_CHANGE_ISOCONTOUR;

    } else if ((AMITK_IS_FIDUCIAL_MARK(object)) && (event->button.button == 1)) {      
      canvas_event_type = CANVAS_EVENT_RELEASE_FIDUCIAL_MARK;

    } else 
      canvas_event_type = CANVAS_EVENT_NONE;
    
    break;
    
  default: 
    event_cpoint.x = event_cpoint.y = 0;
    /* an event we don't handle */
    canvas_event_type = CANVAS_EVENT_NONE;
    break;
    
  }

  //  if ((canvas_event_type != CANVAS_EVENT_NONE) &&
  //      (canvas_event_type != CANVAS_EVENT_MOTION))
  //    g_print("event %d grab %d gdk %d\n", canvas_event_type, grab_on, event->type);

  /* get the location of the event, and convert it to the canvas coordinates */
  gnome_canvas_w2c_d(GNOME_CANVAS(canvas->canvas), event_cpoint.x, event_cpoint.y, &canvas_cpoint.x, &canvas_cpoint.y);

  /* Convert the event location info to real units */
  canvas_point = cp_2_p(canvas, canvas_cpoint);
  base_point = amitk_space_s2b(AMITK_SPACE(canvas->volume), canvas_point);

  /* get the current location's value */
  g_return_val_if_fail(canvas->slices != NULL, FALSE);
  active_slice = NULL;
  if (canvas->active_ds != NULL) {
    active_slice = amitk_data_sets_find_with_slice_parent(canvas->slices, canvas->active_ds);
  } 
  if (active_slice == NULL) {  /* just use the first slice */
    active_slice = canvas->slices->data;
  }
  g_return_val_if_fail(active_slice != NULL, FALSE);
  temp_point[0] = amitk_space_b2s(AMITK_SPACE(active_slice), base_point);
  POINT_TO_VOXEL(temp_point[0], AMITK_DATA_SET_VOXEL_SIZE(active_slice), 0, temp_voxel);
  voxel_value = amitk_data_set_get_value(active_slice, temp_voxel);
  

  switch (canvas_event_type) {
    
  case CANVAS_EVENT_ENTER_DATA_SET:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_DATA_SET, &base_point, 0.0);
    gtk_widget_grab_focus(GTK_WIDGET(canvas)); /* move the keyboard entry focus into the canvas */
    ui_common_place_cursor(UI_CURSOR_DATA_SET_MODE, GTK_WIDGET(canvas));
    break;

  case CANVAS_EVENT_ENTER_FIDUCIAL_MARK:
    in_object = TRUE;
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_FIDUCIAL_MARK, &base_point, 0.0);
    gtk_widget_grab_focus(GTK_WIDGET(canvas)); /* move the keyboard entry focus into the canvas */
    ui_common_place_cursor(UI_CURSOR_FIDUCIAL_MARK_MODE, GTK_WIDGET(canvas));
    break;
    
  case CANVAS_EVENT_ENTER_NEW_NORMAL_ROI:
    in_object = TRUE;
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_NEW_ROI, &base_point, 0.0);
    gtk_widget_grab_focus(GTK_WIDGET(canvas)); /* move the keyboard entry focus into the canvas */
    ui_common_place_cursor(UI_CURSOR_NEW_ROI_MODE, GTK_WIDGET(canvas));
    break;

    
  case CANVAS_EVENT_ENTER_NEW_ISOCONTOUR_ROI:
    in_object = TRUE;
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_NEW_ISOCONTOUR_ROI, &base_point, 0.0);
    gtk_widget_grab_focus(GTK_WIDGET(canvas)); /* move the keyboard entry focus into the canvas */
    ui_common_place_cursor(UI_CURSOR_NEW_ROI_MODE, GTK_WIDGET(canvas));
    break;

    
  case CANVAS_EVENT_ENTER_NORMAL_ROI:
    in_object = TRUE;
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_ROI, &base_point, 0.0);
    gtk_widget_grab_focus(GTK_WIDGET(canvas)); /* move the keyboard entry focus into the canvas */
    ui_common_place_cursor(UI_CURSOR_OLD_ROI_MODE, GTK_WIDGET(canvas));
    break;

    
  case CANVAS_EVENT_ENTER_ISOCONTOUR_ROI:
    in_object = TRUE;
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_ISOCONTOUR_ROI, &base_point, 0.0);
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, 
		  AMITK_ROI(object)->isocontour_value);
    gtk_widget_grab_focus(GTK_WIDGET(canvas)); /* move the keyboard entry focus into the canvas */
    ui_common_place_cursor(UI_CURSOR_OLD_ROI_MODE, GTK_WIDGET(canvas));
    break;
    
    
  case CANVAS_EVENT_LEAVE:
    ui_common_remove_cursor(GTK_WIDGET(canvas)); 

    /* yes, do it twice if we're leaving the canvas, there's a bug where if the canvas gets 
       covered by a menu, no GDK_LEAVE_NOTIFY is called for the GDK_ENTER_NOTIFY */
    if (AMITK_IS_DATA_SET(object)) ui_common_remove_cursor(GTK_WIDGET(canvas));

    /* yep, we don't seem to get an EVENT_ENTER corresponding to the EVENT_LEAVE for a canvas_item */
    if (in_object) 
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_CANVAS_DATA_SET, &base_point, 0.0);
    in_object = FALSE;
    break;

  case CANVAS_EVENT_PRESS_MINIMIZE_VIEW:
  case CANVAS_EVENT_PRESS_RESIZE_VIEW:
  case CANVAS_EVENT_PRESS_MOVE_VIEW:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_UPDATE_LOCATION, &base_point,voxel_value);
    if (canvas_event_type == CANVAS_EVENT_PRESS_MINIMIZE_VIEW)
      corner = amitk_data_sets_get_min_voxel_size(canvas->objects);
    else
      corner = AMITK_VOLUME_Z_CORNER(canvas->volume);
    grab_on = TRUE;
    outline_color = amitk_color_table_outline_color(color_table, FALSE);
    initial_base_point = base_point;
    canvas_update_cross_immediate(canvas, AMITK_CANVAS_CROSS_ACTION_SHOW, base_point, outline_color, corner);
    gnome_canvas_item_grab(canvas->cross[0], GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
    			   ui_common_cursor[UI_CURSOR_DATA_SET_MODE], event->button.time);
    g_signal_emit(G_OBJECT (canvas), canvas_signals[VIEW_CHANGING], 0, &base_point, corner);
    break;

    
  case CANVAS_EVENT_PRESS_SHIFT_DATA_SET:
  case CANVAS_EVENT_PRESS_ROTATE_DATA_SET:
    if (extended_event_type != CANVAS_EVENT_NONE) {
      grab_on = FALSE; /* do everything on the BUTTON_RELEASE */
    } else {
      GdkPixbuf * pixbuf;

      if (active_slice == NULL) return FALSE;
      grab_on = TRUE;
      extended_event_type = canvas_event_type;
      initial_cpoint = canvas_cpoint;
      initial_base_point = base_point;
      initial_canvas_point = canvas_point;
      previous_cpoint = canvas_cpoint;

      pixbuf = image_from_slice(active_slice);
      canvas_item = gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
					  gnome_canvas_pixbuf_get_type(),
					  "pixbuf",pixbuf,
					  "x", (double) CANVAS_TRIANGLE_HEIGHT,
					  "y", (double) CANVAS_TRIANGLE_HEIGHT,
					  NULL);
      g_signal_connect(G_OBJECT(canvas_item), "event", G_CALLBACK(canvas_event_cb), canvas);
      g_object_unref(pixbuf);
    }
    if (canvas_event_type == CANVAS_EVENT_PRESS_SHIFT_DATA_SET) {
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_CANVAS_SHIFT_DATA_SET, &base_point, 0.0);
    } else { /* ROTATE */
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_CANVAS_ROTATE_DATA_SET, &base_point, 0.0);
    }
    break;


  case CANVAS_EVENT_PRESS_MOVE_FIDUCIAL_MARK:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_FIDUCIAL_MARK, &base_point, 0.0);
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, voxel_value);
    grab_on = TRUE;
    canvas_item = GNOME_CANVAS_ITEM(widget); 
    gnome_canvas_item_grab(canvas_item,
			   GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			   ui_common_cursor[UI_CURSOR_FIDUCIAL_MARK_MODE], event->button.time);
    previous_cpoint = canvas_cpoint;
    initial_base_point = base_point;
    break;
    
  case CANVAS_EVENT_PRESS_NEW_ROI:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_NEW_ROI, &base_point, 0.0);
    outline_color = amitk_color_table_outline_color(color_table, TRUE);
    initial_canvas_point = canvas_point;
    initial_cpoint = canvas_cpoint;
    
    /* create the new roi */
    switch(AMITK_ROI_TYPE(object)) {
    case AMITK_ROI_TYPE_BOX:
    case AMITK_ROI_TYPE_ELLIPSOID:
    case AMITK_ROI_TYPE_CYLINDER:
      canvas_item = 
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
			      (AMITK_ROI_TYPE(object) == AMITK_ROI_TYPE_BOX) ?
			      gnome_canvas_rect_get_type() : gnome_canvas_ellipse_get_type(),
			      "x1",canvas_cpoint.x, "y1", canvas_cpoint.y, 
			      "x2", canvas_cpoint.x, "y2", canvas_cpoint.y,
			      "fill_color", NULL,
			      "outline_color_rgba", amitk_color_table_rgba_to_uint32(outline_color),
			      "width_pixels", canvas->roi_width,
			      NULL);
      g_signal_connect(G_OBJECT(canvas_item), "event", G_CALLBACK(canvas_event_cb), canvas);
      grab_on = TRUE;
      gnome_canvas_item_grab(GNOME_CANVAS_ITEM(canvas_item),
			     GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			     ui_common_cursor[UI_CURSOR_NEW_ROI_MOTION],
			     event->button.time);
      break;
    case AMITK_ROI_TYPE_ISOCONTOUR_2D:
    case AMITK_ROI_TYPE_ISOCONTOUR_3D:
      grab_on = TRUE;
      break;
    default:
      grab_on = FALSE;
      g_error("unexpected case in %s at line %d, roi_type %d", 
	      __FILE__, __LINE__, AMITK_ROI_TYPE(object));
      break;
    }

    break;

  case CANVAS_EVENT_PRESS_SHIFT_ROI:
    previous_cpoint = canvas_cpoint;
  case CANVAS_EVENT_PRESS_ROTATE_ROI:
  case CANVAS_EVENT_PRESS_RESIZE_ROI:
    if ((AMITK_ROI_TYPE(object) == AMITK_ROI_TYPE_ISOCONTOUR_2D) || 
	(AMITK_ROI_TYPE(object) == AMITK_ROI_TYPE_ISOCONTOUR_3D))
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_CANVAS_ISOCONTOUR_ROI, &base_point, 0.0);
    else
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_CANVAS_ROI, &base_point, 0.0);
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, voxel_value);
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
    initial_base_point = base_point;
    initial_cpoint = canvas_cpoint;
    initial_canvas_point = canvas_point;
    theta = 0.0;
    zoom.x = zoom.y = zoom.z = 1.0;
    break;

  case CANVAS_EVENT_PRESS_CHANGE_ISOCONTOUR:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_BLANK, &base_point, 0.0);
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, voxel_value);
    grab_on = TRUE;
    initial_cpoint = canvas_cpoint;

    points = gnome_canvas_points_new(2);
    points->coords[2] = points->coords[0] = canvas_cpoint.x;
    points->coords[3] = points->coords[1] = canvas_cpoint.y;
    outline_color = amitk_color_table_outline_color(color_table, TRUE);
    canvas_item = gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)), 
					gnome_canvas_line_get_type(),
					"points", points, "width_pixels", canvas->roi_width,
					"fill_color_rgba", amitk_color_table_rgba_to_uint32(outline_color),
					NULL);
    g_signal_connect(G_OBJECT(canvas_item), "event", G_CALLBACK(canvas_event_cb), canvas);
    g_object_set_data(G_OBJECT(canvas_item), "object", object);
    gnome_canvas_item_grab(GNOME_CANVAS_ITEM(widget),
			   GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			   ui_common_cursor[UI_CURSOR_OLD_ROI_ISOCONTOUR], event->button.time);
    gnome_canvas_points_unref(points);
    break;

  case CANVAS_EVENT_PRESS_ERASE_VOLUME_OUTSIDE_ROI:
    g_return_val_if_fail(AMITK_IS_ROI(object), FALSE);
    g_signal_emit(G_OBJECT (canvas), canvas_signals[ERASE_VOLUME], 0, AMITK_ROI(object), TRUE);
    break;
  case CANVAS_EVENT_PRESS_ERASE_VOLUME_INSIDE_ROI:
    g_return_val_if_fail(AMITK_IS_ROI(object), FALSE);
    g_signal_emit(G_OBJECT (canvas), canvas_signals[ERASE_VOLUME], 0, AMITK_ROI(object), FALSE);
    break;

  case CANVAS_EVENT_PRESS_NEW_OBJECT:
    if (AMITK_IS_DATA_SET(object))
      g_signal_emit(G_OBJECT (canvas), canvas_signals[NEW_OBJECT], 0,
		    object, AMITK_OBJECT_TYPE_FIDUCIAL_MARK, &base_point);
    break;
    
  case CANVAS_EVENT_MOTION:
    if (AMITK_IS_ROI(object))
      if ((AMITK_ROI_TYPE(object) == AMITK_ROI_TYPE_ISOCONTOUR_2D) || 
	  (AMITK_ROI_TYPE(object) == AMITK_ROI_TYPE_ISOCONTOUR_3D))
	voxel_value = AMITK_ROI(object)->isocontour_value;
  
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, voxel_value);
    
    break;

  case CANVAS_EVENT_MOTION_MOVE_VIEW:
  case CANVAS_EVENT_MOTION_MINIMIZE_VIEW:
  case CANVAS_EVENT_MOTION_RESIZE_VIEW:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, voxel_value);
    if (canvas_event_type == CANVAS_EVENT_MOTION_RESIZE_VIEW) {
      corner = point_max_dim(point_diff(base_point, initial_base_point));
      center = initial_base_point;
    } else if (canvas_event_type == CANVAS_EVENT_MOTION_MOVE_VIEW) {
      corner = AMITK_VOLUME_Z_CORNER(canvas->volume);
      center = base_point;
    } else { /* CANVAS_EVENT_MOTION_MINIMIZE_VIEW */
      corner = amitk_data_sets_get_min_voxel_size(canvas->objects);
      center = base_point;
    }

    outline_color = amitk_color_table_outline_color(color_table, FALSE);
    canvas_update_cross_immediate(canvas, AMITK_CANVAS_CROSS_ACTION_SHOW, center, outline_color,corner);
    g_signal_emit(G_OBJECT (canvas), canvas_signals[VIEW_CHANGING], 0,
		  &center, corner);

    break;

  case CANVAS_EVENT_MOTION_FIDUCIAL_MARK:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, voxel_value);
    diff_cpoint = canvas_point_sub(canvas_cpoint, previous_cpoint);
    gnome_canvas_item_i2w(GNOME_CANVAS_ITEM(canvas_item)->parent, &diff_cpoint.x, &diff_cpoint.y);
    gnome_canvas_item_move(GNOME_CANVAS_ITEM(canvas_item),diff_cpoint.x,diff_cpoint.y); 
    previous_cpoint = canvas_cpoint;
    break;

  case CANVAS_EVENT_PRESS_LARGE_ERASE_ISOCONTOUR:
  case CANVAS_EVENT_PRESS_ERASE_ISOCONTOUR:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_ISOCONTOUR_ROI, &base_point, 0.0);
    grab_on = TRUE;
    canvas_item = GNOME_CANVAS_ITEM(widget);
    gnome_canvas_item_grab(canvas_item,
			   GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			   ui_common_cursor[UI_CURSOR_OLD_ROI_ERASE], event->button.time);
    /* and fall through */
  case CANVAS_EVENT_MOTION_LARGE_ERASE_ISOCONTOUR:
  case CANVAS_EVENT_MOTION_ERASE_ISOCONTOUR:
    g_return_val_if_fail(canvas->slices != NULL, FALSE);
    temp_point[0] = amitk_space_b2s(AMITK_SPACE(object), base_point);
    POINT_TO_VOXEL(temp_point[0], AMITK_ROI(object)->voxel_size, 0, temp_voxel);
    if ((canvas_event_type == CANVAS_EVENT_MOTION_LARGE_ERASE_ISOCONTOUR) ||
	(canvas_event_type == CANVAS_EVENT_PRESS_LARGE_ERASE_ISOCONTOUR))
      amitk_roi_isocontour_erase_area(AMITK_ROI(object), temp_voxel, 2);
    else
      amitk_roi_isocontour_erase_area(AMITK_ROI(object), temp_voxel, 0);

    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, voxel_value);
    break;

  case CANVAS_EVENT_MOTION_CHANGE_ISOCONTOUR:
    points = gnome_canvas_points_new(2);
    points->coords[0] = initial_cpoint.x;
    points->coords[1] = initial_cpoint.y;
    points->coords[2] = canvas_cpoint.x;
    points->coords[3] = canvas_cpoint.y;
    gnome_canvas_item_set(canvas_item, "points", points, NULL);
    gnome_canvas_points_unref(points);

    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, voxel_value);
    break;

  case CANVAS_EVENT_MOTION_NEW_ROI:
    if ((AMITK_ROI_TYPE(object) == AMITK_ROI_TYPE_ISOCONTOUR_2D) || 
	(AMITK_ROI_TYPE(object) == AMITK_ROI_TYPE_ISOCONTOUR_3D))
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_CANVAS_NEW_ISOCONTOUR_ROI, &base_point, 0.0);
    else {
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_CANVAS_NEW_ROI, &base_point, 0.0);
      diff_cpoint = canvas_point_diff(initial_cpoint, canvas_cpoint);
      temp_cpoint[0] = canvas_point_sub(initial_cpoint, diff_cpoint);
      temp_cpoint[1] = canvas_point_add(initial_cpoint, diff_cpoint);
      gnome_canvas_item_set(canvas_item, "x1", temp_cpoint[0].x, "y1", temp_cpoint[0].y,
			    "x2", temp_cpoint[1].x, "y2", temp_cpoint[1].y,NULL);
    }
    break;

  case CANVAS_EVENT_MOTION_SHIFT_DATA_SET:
  case CANVAS_EVENT_MOTION_SHIFT_ROI: 
    if (canvas_event_type == CANVAS_EVENT_MOTION_SHIFT_DATA_SET)
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_UPDATE_SHIFT, &diff_point, theta);
    else
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, voxel_value);

    diff_cpoint = canvas_point_sub(canvas_cpoint, previous_cpoint);
    gnome_canvas_item_i2w(GNOME_CANVAS_ITEM(canvas_item)->parent, &diff_cpoint.x, &diff_cpoint.y);
    gnome_canvas_item_move(GNOME_CANVAS_ITEM(canvas_item),diff_cpoint.x,diff_cpoint.y); 
    previous_cpoint = canvas_cpoint;
    break;


  case CANVAS_EVENT_MOTION_ROTATE_DATA_SET:
  case CANVAS_EVENT_MOTION_ROTATE_ROI: 
    {
      /* rotate button Pressed, we're rotating the object */
      /* note, I'd like to use the function "gnome_canvas_item_rotate"
	 but this isn't defined in the current version of gnome.... 
	 so I'll have to do a whole bunch of shit*/
      double affine[6];
      AmitkCanvasPoint item_center;
      AmitkPoint center;
      
      center = amitk_space_b2s(AMITK_SPACE(canvas->volume), amitk_volume_center(AMITK_VOLUME(object)));
      temp_point[0] = point_sub(initial_canvas_point,center);
      temp_point[1] = point_sub(canvas_point,center);
      
      theta = acos(point_dot_product(temp_point[0],temp_point[1])/(point_mag(temp_point[0]) * point_mag(temp_point[1])));

      //      if (isnan(theta)) theta = 0;
      //      if (canvas->view == AMITK_VIEW_SAGITTAL) theta = -theta; /* sagittal is left-handed */

      /* correct for the fact that acos is always positive by using the cross product */
      if ((temp_point[0].x*temp_point[1].y-temp_point[0].y*temp_point[1].x) < 0.0) theta = -theta;

      /* figure out what the center of the roi is in canvas_item coords */
      /* compensate for x's origin being top left (ours is bottom left) */
      item_center = p_2_cpoint(canvas, center);
      affine[0] = cos(-theta); /* neg cause GDK has Y axis going down, not up */
      affine[1] = sin(-theta);
      affine[2] = -affine[1];
      affine[3] = affine[0];
      affine[4] = (1.0-affine[0])*item_center.x+affine[1]*item_center.y;
      affine[5] = (1.0-affine[3])*item_center.y+affine[2]*item_center.x;
      gnome_canvas_item_affine_absolute(GNOME_CANVAS_ITEM(canvas_item),affine);
    }
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_UPDATE_THETA, &base_point, theta*180.0/M_PI);
    break;


  case CANVAS_EVENT_MOTION_RESIZE_ROI:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, voxel_value);
    {
      /* RESIZE button Pressed, we're scaling the object */
      /* note, I'd like to use the function "gnome_canvas_item_scale"
	 but this isn't defined in the current version of gnome.... 
	 so I'll have to do a whole bunch of shit*/
      /* also, this "zoom" strategy doesn't always work if the object is not
	 aligned with the view.... oh well... good enough for now... */
      double affine[6];
      AmitkCanvasPoint item_center;
      amide_real_t jump_limit, max, temp, dot;
      double cos_r, sin_r, rot;
      AmitkAxis i_axis, axis[AMITK_AXIS_NUM];
      AmitkCanvasPoint radius_cpoint;
      AmitkPoint center, radius, canvas_zoom;
	
      /* calculating the radius and center wrt the canvas */
      radius_point = point_cmult(0.5,AMITK_VOLUME_CORNER(object));
      radius = amitk_space_s2s_dim(AMITK_SPACE(object), AMITK_SPACE(canvas->volume), radius_point);

      center = amitk_space_b2s(AMITK_SPACE(canvas->volume),amitk_volume_center(AMITK_VOLUME(object)));
      temp_point[0] = point_diff(initial_canvas_point, center);
      temp_point[1] = point_diff(canvas_point,center);
      radius_cpoint.x = radius_point.x;
      radius_cpoint.y = radius_point.y;
      jump_limit = canvas_point_mag(radius_cpoint)/3.0;
	
      /* figure out the zoom we're specifying via the canvas */
      if (temp_point[0].x < jump_limit) /* prevent jumping */
	canvas_zoom.x = (radius.x+temp_point[1].x)/(radius.x+temp_point[0].x);
      else
	canvas_zoom.x = temp_point[1].x/temp_point[0].x;
      if (temp_point[0].y < jump_limit) /* prevent jumping */
	canvas_zoom.y = (radius.x+temp_point[1].y)/(radius.x+temp_point[0].y);
      else
	canvas_zoom.y = temp_point[1].y/temp_point[0].y;
      canvas_zoom.z = 1.0;
	
      /* translate the canvas zoom into the ROI's coordinate frame */
      temp_point[0].x = temp_point[0].y = temp_point[0].z = 1.0;
      temp_point[0] = amitk_space_s2s_dim(AMITK_SPACE(canvas->volume), AMITK_SPACE(object), temp_point[0]);
      zoom = amitk_space_s2s_dim(AMITK_SPACE(canvas->volume), AMITK_SPACE(object), canvas_zoom);
      zoom = point_div(zoom, temp_point[0]);
	
      /* first, figure out how much the roi is rotated in the plane of the canvas */
      /* figure out which axis in the ROI is closest to the view's x axis */
      max = 0.0;
      axis[AMITK_AXIS_X]=AMITK_AXIS_X;
      temp_point[0] = 
	amitk_axes_get_orthogonal_axis(AMITK_SPACE_AXES(canvas->volume), 
				       canvas->view, canvas->layout, AMITK_AXIS_X);
      for (i_axis=0;i_axis<AMITK_AXIS_NUM;i_axis++) {
	temp = fabs(point_dot_product(temp_point[0], amitk_space_get_axis(AMITK_SPACE(object), i_axis)));
	if (temp > max) {
	  max=temp;
	  axis[AMITK_AXIS_X]=i_axis;
	}
      }
	
      /* and y axis */
      max = 0.0;
      axis[AMITK_AXIS_Y]= (axis[AMITK_AXIS_X]+1 < AMITK_AXIS_NUM) ? axis[AMITK_AXIS_X]+1 : AMITK_AXIS_X;
      temp_point[0] = amitk_axes_get_orthogonal_axis(AMITK_SPACE_AXES(canvas->volume), 
						     canvas->view, canvas->layout, AMITK_AXIS_Y);
      for (i_axis=0;i_axis<AMITK_AXIS_NUM;i_axis++) {
	temp = fabs(point_dot_product(temp_point[0], amitk_space_get_axis(AMITK_SPACE(object), i_axis)));
	if (temp > max) {
	  if (i_axis != axis[AMITK_AXIS_X]) {
	    max=temp;
	    axis[AMITK_AXIS_Y]=i_axis;
	  }
	}
      }
	
      i_axis = AMITK_AXIS_Z;
      for (i_axis=0;i_axis<AMITK_AXIS_NUM;i_axis++)
	if (i_axis != axis[AMITK_AXIS_X])
	  if (i_axis != axis[AMITK_AXIS_Y])
	    axis[AMITK_AXIS_Z]=i_axis;
	
      rot = 0;
      for (i_axis=0;i_axis<=AMITK_AXIS_X;i_axis++) {
	/* and project that axis onto the x,y plane of the canvas */
	temp_point[0] = amitk_space_get_axis(AMITK_SPACE(object),axis[i_axis]);
	temp_point[0] = amitk_space_b2s(AMITK_SPACE(canvas->volume), temp_point[0]);
	temp_point[0].z = 0.0;
	temp_point[1] = amitk_axes_get_orthogonal_axis(AMITK_SPACE_AXES(canvas->volume),
						       canvas->view, canvas->layout, i_axis);
	temp_point[1] = amitk_space_b2s(AMITK_SPACE(canvas->volume), temp_point[1]);
	temp_point[1].z = 0.0;
	  
	/* and get the angle the projection makes to the x axis */
	dot = point_dot_product(temp_point[0],temp_point[1])/(point_mag(temp_point[0]) * point_mag(temp_point[1]));
	/* correct for the fact that acos is always positive by using the cross product */
	if ((temp_point[0].x*temp_point[1].y-temp_point[0].y*temp_point[1].x) > 0.0)
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
      item_center = p_2_cpoint(canvas, center);
	
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
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, voxel_value);
    gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(canvas->cross[0]), event->button.time);
    grab_on = FALSE;
    canvas_update_cross_immediate(canvas, AMITK_CANVAS_CROSS_ACTION_HIDE, base_point, rgba_black, 0.0);

    if (canvas_event_type == CANVAS_EVENT_RELEASE_RESIZE_VIEW)
      center = initial_base_point;
    else
      center = base_point;

    if (canvas_event_type == CANVAS_EVENT_RELEASE_MINIMIZE_VIEW)
      corner = amitk_data_sets_get_min_voxel_size(canvas->objects);
    else if (canvas_event_type == CANVAS_EVENT_RELEASE_RESIZE_VIEW)
      corner = point_max_dim(point_diff(base_point, initial_base_point));
    else
      corner = AMITK_VOLUME_Z_CORNER(canvas->volume);

    g_signal_emit(G_OBJECT (canvas), canvas_signals[VIEW_CHANGED], 0,
		  &center, corner);
    break;

  case CANVAS_EVENT_RELEASE_SHIFT_DATA_SET:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_SHIFT_DATA_SET, &base_point, 0.0);
    break;

  case CANVAS_EVENT_RELEASE_ROTATE_DATA_SET:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_ROTATE_DATA_SET, &base_point, 0.0);
    break;

  case CANVAS_EVENT_CANCEL_SHIFT_DATA_SET:
  case CANVAS_EVENT_CANCEL_ROTATE_DATA_SET:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_DATA_SET, &base_point, 0.0);
    extended_event_type = CANVAS_EVENT_NONE;
    gtk_object_destroy(GTK_OBJECT(canvas_item));
    break;

  case CANVAS_EVENT_ENACT_SHIFT_DATA_SET:
  case CANVAS_EVENT_ENACT_ROTATE_DATA_SET:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_DATA_SET, &base_point, 0.0);
    gtk_object_destroy(GTK_OBJECT(canvas_item));
    extended_event_type = CANVAS_EVENT_NONE;

    if (canvas->active_ds == NULL) { /* sanity check */
      g_error("inappropriate null active data set, %s at line %d", __FILE__, __LINE__);
    } else if (canvas_event_type == CANVAS_EVENT_ENACT_SHIFT_DATA_SET) { /* shift active data set */
	amitk_space_shift_offset(AMITK_SPACE(canvas->active_ds),
				 point_sub(base_point, initial_base_point));
    } else {/* rotate active data set*/
      if (canvas->view == AMITK_VIEW_SAGITTAL) theta = -theta; /* sagittal is left-handed */

      amitk_space_rotate_on_vector(AMITK_SPACE(canvas->active_ds),
				   amitk_space_get_axis(AMITK_SPACE(canvas->volume), AMITK_AXIS_Z),
				   theta, amitk_volume_center(AMITK_VOLUME(canvas->active_ds)));
    }
    break;




  case CANVAS_EVENT_RELEASE_FIDUCIAL_MARK:
    gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
    grab_on = FALSE;
    /* ------------- apply any shift done -------------- */
    if (!POINT_EQUAL(AMITK_FIDUCIAL_MARK_GET(object), base_point)) {
      shift = point_sub(base_point, initial_base_point);
      amitk_fiducial_mark_set(AMITK_FIDUCIAL_MARK(object), 
			      point_add(shift, AMITK_FIDUCIAL_MARK_GET(object)));

      /* if we were moving the point, reset the view center to where the point is */
      g_signal_emit(G_OBJECT (canvas), canvas_signals[VIEW_CHANGED], 0,
		    &base_point, AMITK_VOLUME_Z_CORNER(canvas->volume));
    }
    break;


  case CANVAS_EVENT_RELEASE_NEW_ROI:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_DATA_SET, &base_point, 0.0);
    grab_on = FALSE;
    switch(AMITK_ROI_TYPE(object)) {
    case AMITK_ROI_TYPE_CYLINDER:
    case AMITK_ROI_TYPE_BOX:
    case AMITK_ROI_TYPE_ELLIPSOID:  
      gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(canvas_item), event->button.time);
      gtk_object_destroy(GTK_OBJECT(canvas_item)); /* get rid of the roi drawn on the canvas */
      
      diff_point = point_diff(initial_canvas_point, canvas_point);
      diff_point.z = AMITK_VOLUME_Z_CORNER(canvas->volume)/2.0;
      temp_point[0] = point_sub(initial_canvas_point, diff_point);
      temp_point[1] = point_add(initial_canvas_point, diff_point);

      /* we'll save the coord frame and offset of the roi */
      amitk_space_copy_in_place(AMITK_SPACE(object), AMITK_SPACE(canvas->volume));
      amitk_space_set_offset(AMITK_SPACE(object),
			     amitk_space_s2b(AMITK_SPACE(canvas->volume), temp_point[0]));
      
      /* and set the far corner of the roi */
      amitk_volume_set_corner(AMITK_VOLUME(object),
			      point_abs(amitk_space_s2s(AMITK_SPACE(canvas->volume), 
							AMITK_SPACE(object), 
							temp_point[1])));
      break;
    case AMITK_ROI_TYPE_ISOCONTOUR_2D: 
    case AMITK_ROI_TYPE_ISOCONTOUR_3D:
      if (AMITK_ROI_TYPE(object) == AMITK_ROI_TYPE_ISOCONTOUR_2D) {

	AmitkObject * parent_ds;
	AmitkDataSet * parent_slice=NULL;
	GList * slices;
	
	parent_ds = amitk_object_get_parent_of_type(object, AMITK_OBJECT_TYPE_DATA_SET);
	if (parent_ds == NULL) { /* if no data set parent, just use the active slice */
	  parent_slice = active_slice;
	} else {
	  slices = canvas->slices;
	  while ((slices != NULL)  && (parent_slice == NULL)){
	    if (AMITK_DATA_SET_SLICE_PARENT(slices->data) == AMITK_DATA_SET(parent_ds))
	      parent_slice = slices->data;
	    slices = slices->next;
	  }
	  if (parent_slice == NULL) {
	    g_warning("Parent of isocontour not currently displayed, can't draw isocontour\n");
	    return FALSE;
	  }
	}
	
	g_return_val_if_fail(parent_slice != NULL, FALSE);
	temp_point[0] = amitk_space_b2s(AMITK_SPACE(parent_slice), base_point);
	POINT_TO_VOXEL(temp_point[0], AMITK_DATA_SET_VOXEL_SIZE(parent_slice),0,temp_voxel);
	ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(canvas));
	if (amitk_raw_data_includes_voxel(parent_slice->raw_data,temp_voxel))
	  amitk_roi_set_isocontour(AMITK_ROI(object),parent_slice, temp_voxel);
	ui_common_remove_cursor(GTK_WIDGET(canvas));
	
      } else { /* ISOCONTOUR_3D */
	g_signal_emit(G_OBJECT (canvas), canvas_signals[ISOCONTOUR_3D_CHANGED], 0,
		      object, &base_point);
      }

      break;
    default:
      g_error("unexpected case in %s at line %d, roi_type %d", 
	      __FILE__, __LINE__, AMITK_ROI_TYPE(object));
      break;
    }

    break;


  case CANVAS_EVENT_RELEASE_SHIFT_ROI:
    gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
    grab_on = FALSE;
    shift = point_sub(base_point, initial_base_point);
    /* ------------- apply any shift done -------------- */
    if (!POINT_EQUAL(shift, zero_point))
      amitk_space_shift_offset(AMITK_SPACE(object), shift);
    break;


  case CANVAS_EVENT_RELEASE_ROTATE_ROI:
    gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
    grab_on = FALSE;
    if (canvas->view == AMITK_VIEW_SAGITTAL) theta = -theta; /* compensate for sagittal being left-handed coord frame */
    
    /* now rotate the roi coordinate space axis */
    amitk_space_rotate_on_vector(AMITK_SPACE(object), 
				 amitk_space_get_axis(AMITK_SPACE(canvas->volume), AMITK_AXIS_Z), 
				 theta, amitk_volume_center(AMITK_VOLUME(object)));
    break;


  case CANVAS_EVENT_RELEASE_RESIZE_ROI:
    gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
    grab_on = FALSE;
    
    radius_point = point_cmult(0.5,AMITK_VOLUME_CORNER(object));
    temp_point[0] = point_mult(zoom, radius_point); /* new radius */
    temp_point[1] = amitk_space_b2s(AMITK_SPACE(object), amitk_volume_center(AMITK_VOLUME(object)));
    temp_point[1] = amitk_space_s2b(AMITK_SPACE(object), point_sub(temp_point[1], temp_point[0]));
    amitk_space_set_offset(AMITK_SPACE(object), temp_point[1]);
	  
    /* and the new upper right corner is simply twice the radius */
    amitk_volume_set_corner(AMITK_VOLUME(object), point_cmult(2.0, temp_point[0]));
    break;

  case CANVAS_EVENT_RELEASE_LARGE_ERASE_ISOCONTOUR:
  case CANVAS_EVENT_RELEASE_ERASE_ISOCONTOUR:
    gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
    grab_on = FALSE;
    break;

  case CANVAS_EVENT_RELEASE_CHANGE_ISOCONTOUR:
    gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
    grab_on = FALSE;
    gtk_object_destroy(GTK_OBJECT(canvas_item));

    if (AMITK_ROI_TYPE(object) == AMITK_ROI_TYPE_ISOCONTOUR_2D) {
      AmitkObject * parent_ds;
      AmitkDataSet * parent_slice=NULL;
      GList * slices;

      parent_ds = amitk_object_get_parent_of_type(object, AMITK_OBJECT_TYPE_DATA_SET);
      if (parent_ds == NULL) { /* if no data set parent, just use the active slice */
	parent_slice = active_slice;
      } else {
	slices = canvas->slices;
	while ((slices != NULL)  && (parent_slice == NULL)){
	  if (AMITK_DATA_SET_SLICE_PARENT(slices->data) == AMITK_DATA_SET(parent_ds))
	    parent_slice = slices->data;
	  slices = slices->next;
	}
	if (parent_slice == NULL) {
	  g_warning("Parent of isocontour not currently displayed, can't draw isocontour\n");
	  return FALSE;
	}
      }
      
      g_return_val_if_fail(parent_slice != NULL, FALSE);
      temp_point[0] = amitk_space_b2s(AMITK_SPACE(parent_slice), base_point);
      POINT_TO_VOXEL(temp_point[0], AMITK_DATA_SET_VOXEL_SIZE(parent_slice),0,temp_voxel);
      ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(canvas));
      if (amitk_raw_data_includes_voxel(parent_slice->raw_data,temp_voxel))
	amitk_roi_set_isocontour(AMITK_ROI(object),parent_slice, temp_voxel);
      ui_common_remove_cursor(GTK_WIDGET(canvas));
    } else { /* ISOCONTOUR_3D */
      g_signal_emit(G_OBJECT (canvas), canvas_signals[ISOCONTOUR_3D_CHANGED], 0,
		    object, &base_point);
    }

    break;

  case CANVAS_EVENT_NONE:
    break;
  default:
    g_error("unexpected case in %s at line %d, event %d", 
	    __FILE__, __LINE__, canvas_event_type);
    break;
  }
  
  return FALSE;
}








/* function called indicating the plane adjustment has changed */
static void canvas_scrollbar_cb(GtkObject * adjustment, gpointer data) {

  AmitkCanvas * canvas = data;
  AmitkPoint canvas_center;

  g_return_if_fail(AMITK_IS_CANVAS(canvas));

  canvas_center = amitk_space_b2s(AMITK_SPACE(canvas->volume), canvas->center);
  canvas_center.z = GTK_ADJUSTMENT(adjustment)->value;
  canvas->center = amitk_space_s2b(AMITK_SPACE(canvas->volume), canvas_center);

  canvas_add_update(canvas, UPDATE_ALL);
  g_signal_emit(G_OBJECT (canvas), canvas_signals[VIEW_CHANGED], 0,
		&(canvas->center), AMITK_VOLUME_Z_CORNER(canvas->volume));

  return;
}


static gboolean canvas_recalc_corners(AmitkCanvas * canvas) {

  AmitkCorners temp_corner;
  AmitkPoint temp_point;
  gboolean changed = FALSE;
  gboolean valid;

  if (canvas->objects == NULL) return FALSE;

  /* figure out the corners */
  valid = amitk_volumes_get_enclosing_corners(canvas->objects, 
					      AMITK_SPACE(canvas->volume), 
					      temp_corner);
  
  /* update the z dimension appropriately */
  if (valid) {
    temp_point = amitk_space_b2s(AMITK_SPACE(canvas->volume), canvas->center);
    temp_corner[0].z = temp_point.z - AMITK_VOLUME_Z_CORNER(canvas->volume)/2.0;
    temp_corner[1].z = temp_point.z + AMITK_VOLUME_Z_CORNER(canvas->volume)/2.0;
    
    temp_corner[0] = amitk_space_s2b(AMITK_SPACE(canvas->volume), temp_corner[0]);
    temp_corner[1] = amitk_space_s2b(AMITK_SPACE(canvas->volume), temp_corner[1]);
    
    if (!POINT_EQUAL(AMITK_SPACE_OFFSET(canvas->volume), temp_corner[0])) {
      amitk_space_set_offset(AMITK_SPACE(canvas->volume), temp_corner[0]);
      changed = TRUE;
    }
    
    temp_corner[1] = amitk_space_b2s(AMITK_SPACE(canvas->volume), temp_corner[1]);
    if (!POINT_EQUAL(AMITK_VOLUME_CORNER(canvas->volume), temp_corner[1])) {
      amitk_volume_set_corner(canvas->volume, temp_corner[1]);
      changed = TRUE;
    }
  }

  return changed;
}


/* function to update the adjustment settings for the scrollbar */
static void canvas_update_scrollbar(AmitkCanvas * canvas, AmitkPoint center, amide_real_t thickness) {

  AmitkCorners view_corner;
  amide_real_t upper, lower;
  amide_real_t min_voxel_size;
  AmitkPoint zp_start;
  
  //  if (canvas->volumes == NULL) {   /* junk values */
  //    min_voxel_size = 1.0;
  //    upper = lower = zp_start.z = 0.0;
  //  } else { /* calculate values */

  /* make valgrind happy */
  view_corner[0]=zero_point;
  view_corner[1]=zero_point;

  amitk_volumes_get_enclosing_corners(canvas->objects, AMITK_SPACE(canvas->volume), view_corner);
  min_voxel_size = amitk_data_sets_get_max_min_voxel_size(canvas->objects);
  
  upper = view_corner[1].z;
  lower = view_corner[0].z;
  
  /* translate the view center point so that the z coordinate corresponds to depth in this view */
  zp_start = amitk_space_b2s(AMITK_SPACE(canvas->volume), center);
  
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
  
  GTK_ADJUSTMENT(canvas->scrollbar_adjustment)->upper = upper;
  GTK_ADJUSTMENT(canvas->scrollbar_adjustment)->lower = lower;
  GTK_ADJUSTMENT(canvas->scrollbar_adjustment)->page_increment = min_voxel_size;
  GTK_ADJUSTMENT(canvas->scrollbar_adjustment)->page_size = thickness;
  GTK_ADJUSTMENT(canvas->scrollbar_adjustment)->value = zp_start.z;
  
  /* allright, we need to update widgets connected to the adjustment without triggering our callback */
  g_signal_handlers_block_by_func(G_OBJECT(canvas->scrollbar_adjustment),
				   G_CALLBACK(canvas_scrollbar_cb), canvas);
  gtk_adjustment_changed(GTK_ADJUSTMENT(canvas->scrollbar_adjustment));  
  g_signal_handlers_unblock_by_func(G_OBJECT(canvas->scrollbar_adjustment), 
				     G_CALLBACK(canvas_scrollbar_cb), canvas);

  return;
}


static void canvas_update_cross_immediate(AmitkCanvas * canvas, AmitkCanvasCrossAction action, 
					  AmitkPoint center, rgba_t color, 
					  amide_real_t thickness) {
  canvas->next_cross_action = action;
  canvas->next_cross_center = center;
  canvas->next_cross_color =  color;
  canvas->next_cross_thickness = thickness;

  canvas_update_cross(canvas, canvas->next_cross_action, canvas->next_cross_center,
		      canvas->next_cross_color, canvas->next_cross_thickness);

  return;
}

/* function to update the target cross on the canvas */
static void canvas_update_cross(AmitkCanvas * canvas, AmitkCanvasCrossAction action, 
				AmitkPoint center, rgba_t color, amide_real_t thickness) {

  GnomeCanvasPoints * points[4];
  AmitkCanvasPoint point0, point1;
  AmitkPoint start, end;
  gint i;

  if ((canvas->slices == NULL) || (action == AMITK_CANVAS_CROSS_ACTION_HIDE)) {
    for (i=0; i < 4 ; i++) 
      if (canvas->cross[i] != NULL) gnome_canvas_item_hide(canvas->cross[i]);
    return;
  }
  
  start = amitk_space_b2s(AMITK_SPACE(canvas->volume), center);
  start.x -= thickness/2.0;
  start.y -= thickness/2.0;
  end = amitk_space_b2s(AMITK_SPACE(canvas->volume), center);
  end.x += thickness/2.0;
  end.y += thickness/2.0;

  /* get the canvas locations corresponding to the start and end coordinates */
  point0 = p_2_cpoint(canvas, start);
  point1 = p_2_cpoint(canvas, end);
    
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
    if (canvas->cross[i]==NULL) {
      canvas->cross[i] =
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
			      gnome_canvas_line_get_type(),
			      "points", points[i], 
			      "fill_color_rgba", amitk_color_table_rgba_to_uint32(color),
			      "width_pixels", 1, NULL);
      g_signal_connect(G_OBJECT(canvas->cross[i]), "event", G_CALLBACK(canvas_event_cb), canvas);
    } else if (action == AMITK_CANVAS_CROSS_ACTION_SHOW)
      gnome_canvas_item_set(canvas->cross[i],"points",points[i], 
			    "fill_color_rgba", amitk_color_table_rgba_to_uint32(color),
			    "width_pixels", 1, NULL);
    else 
      gnome_canvas_item_set(canvas->cross[i],"points",points[i], NULL);
    gnome_canvas_item_show(canvas->cross[i]);
    gnome_canvas_points_unref(points[i]);
  }

  return;
}




/* function to update the arrows on the canvas */
static void canvas_update_arrows(AmitkCanvas * canvas, AmitkPoint center, amide_real_t thickness) {

  GnomeCanvasPoints * points[4];
  AmitkCanvasPoint point0, point1;
  AmitkPoint start, end;
  gint i;

  if (!canvas->with_arrows) return;

  if (canvas->slices == NULL) {
    for (i=0; i<4; i++)
      if (canvas->arrows[i] != NULL)
	gnome_canvas_item_hide(canvas->arrows[i]);
    return;
  }

  /* figure out the dimensions of the view "box" */
  start = amitk_space_b2s(AMITK_SPACE(canvas->volume), center);
  start.x -= thickness/2.0;
  start.y -= thickness/2.0;
  end = amitk_space_b2s(AMITK_SPACE(canvas->volume), center);
  end.x += thickness/2.0;
  end.y += thickness/2.0;
  
  /* get the canvas locations corresponding to the start and end coordinates */
  point0 = p_2_cpoint(canvas, start);
  point1 = p_2_cpoint(canvas, end);

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

  gint old_width, old_height;
  rgba_t blank_rgba;
  GtkStyle * widget_style;
  amide_real_t pixel_dim;
  AmitkPoint corner;
  gint width,height;
  
  old_width = canvas->pixbuf_width;
  old_height = canvas->pixbuf_height;

  /* free the previous pixbuf if possible */
  if (canvas->pixbuf != NULL) {
    g_object_unref(canvas->pixbuf);
    canvas->pixbuf = NULL;
  }

  pixel_dim = (1/canvas->zoom)*canvas->voxel_dim; /* compensate for zoom */

  if (amitk_data_sets_count(canvas->objects, FALSE) == 0) {
    /* just use a blank image */

    /* figure out what color to use */
    widget_style = gtk_widget_get_style(GTK_WIDGET(canvas));
    if (widget_style == NULL) {
      g_warning("Canvas has no style?\n");
      widget_style = gtk_style_new();
    }

    blank_rgba.r = widget_style->bg[GTK_STATE_NORMAL].red >> 8;
    blank_rgba.g = widget_style->bg[GTK_STATE_NORMAL].green >> 8;
    blank_rgba.b = widget_style->bg[GTK_STATE_NORMAL].blue >> 8;
    blank_rgba.a = 0xFF;

    corner = AMITK_VOLUME_CORNER(canvas->volume);

    width = ceil(corner.x/pixel_dim);
    if (width < 1) width = 1;
    height =  ceil(corner.y/pixel_dim);
    if (height < 1) height = 1;
				 
    canvas->pixbuf = image_blank(width, height,blank_rgba);

  } else {
    canvas->pixbuf = image_from_data_sets(&(canvas->slices),
					  canvas->objects,
					  canvas->active_ds,
					  canvas->start_time,
					  canvas->duration,
					  pixel_dim,
					  canvas->volume,
					  canvas->fuse_type);
  }

  if (canvas->pixbuf != NULL) {
    /* record the width and height for future use*/
    canvas->pixbuf_width = gdk_pixbuf_get_width(canvas->pixbuf);
    canvas->pixbuf_height = gdk_pixbuf_get_height(canvas->pixbuf);

    /* reset the min size of the widget and set the scroll region */
    if ((old_width != canvas->pixbuf_width) || 
	(old_height != canvas->pixbuf_height) || (canvas->image == NULL)) {
      gtk_widget_set_size_request(canvas->canvas, 
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
      g_signal_connect(G_OBJECT(canvas->image), "event", G_CALLBACK(canvas_event_cb), canvas);
    } else {
      gnome_canvas_item_set(canvas->image, "pixbuf", canvas->pixbuf, NULL);
    }
    
  }

  return;
}


/* if item is declared, object will be ignored,
   otherwise, object will be used for creating the new canvas_item */
static GnomeCanvasItem * canvas_update_object(AmitkCanvas * canvas, 
					      GnomeCanvasItem * item,
					      AmitkObject * object) {

  rgba_t outline_color;
  gdouble affine[6];
  gboolean new;
  gboolean hide_object = FALSE;
  GnomeCanvasPoints * points;
  guint32 fill_color_rgba;
  if (item != NULL) {
    new=FALSE;
    object = g_object_get_data(G_OBJECT(item), "object");

    /* make sure to reset any affine translations we've done */
    gnome_canvas_item_i2w_affine(GNOME_CANVAS_ITEM(item),affine);
    affine[0] = affine[3] = 1.0;
    affine[1] = affine[2] = affine[4] = affine[5] = affine[6] = 0.0;
    gnome_canvas_item_affine_absolute(GNOME_CANVAS_ITEM(item),affine);
      
  } else {
    new=TRUE;
  }
  g_return_val_if_fail(object != NULL, NULL);



  if (AMITK_IS_FIDUCIAL_MARK(object)) {
    /* --------------------- redraw alignment point ----------------------------- */
    AmitkPoint center_point;
    AmitkCanvasPoint center_cpoint;

    if (AMITK_IS_DATA_SET(AMITK_OBJECT_PARENT(object)))
      outline_color = 
	amitk_color_table_outline_color(AMITK_DATA_SET_COLOR_TABLE(AMITK_OBJECT_PARENT(object)), TRUE);
    else
      outline_color = amitk_color_table_outline_color(AMITK_COLOR_TABLE_BW_LINEAR, TRUE);
    fill_color_rgba = amitk_color_table_rgba_to_uint32(outline_color);

    center_point = amitk_space_b2s(AMITK_SPACE(canvas->volume), 
				   AMITK_FIDUCIAL_MARK_GET(object));

    center_cpoint = p_2_cpoint(canvas, center_point);
    points = gnome_canvas_points_new(7);
    points->coords[0] = center_cpoint.x-CANVAS_FIDUCIAL_MARK_WIDTH;
    points->coords[1] = center_cpoint.y;
    points->coords[2] = center_cpoint.x;
    points->coords[3] = center_cpoint.y;
    points->coords[4] = center_cpoint.x;
    points->coords[5] = center_cpoint.y+CANVAS_FIDUCIAL_MARK_WIDTH;
    points->coords[6] = center_cpoint.x;
    points->coords[7] = center_cpoint.y;
    points->coords[8] = center_cpoint.x+CANVAS_FIDUCIAL_MARK_WIDTH;
    points->coords[9] = center_cpoint.y;
    points->coords[10] = center_cpoint.x;
    points->coords[11] = center_cpoint.y;
    points->coords[12] = center_cpoint.x;
    points->coords[13] = center_cpoint.y-CANVAS_FIDUCIAL_MARK_WIDTH;

    if (new)
      item = gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
				   gnome_canvas_line_get_type(), "points", points,
				   "fill_color_rgba", fill_color_rgba,
				   "width_pixels", CANVAS_FIDUCIAL_MARK_WIDTH_PIXELS,
				   "line_style", CANVAS_FIDUCIAL_MARK_LINE_STYLE, NULL);
    else
      gnome_canvas_item_set(item, "points", points,"fill_color_rgba", fill_color_rgba, NULL);
    gnome_canvas_points_unref(points);

    /* make sure the point is on this slice */
    hide_object = ((center_point.x < 0.0) || 
		   (center_point.x > AMITK_VOLUME_X_CORNER(canvas->volume)) ||
		   (center_point.y < 0.0) || 
		   (center_point.y > AMITK_VOLUME_Y_CORNER(canvas->volume)) ||
		   (center_point.z < 0.0) || 
		   (center_point.z > AMITK_VOLUME_Z_CORNER(canvas->volume)));


  } else if (AMITK_IS_ROI(object)) {
    /* --------------------- redraw roi ----------------------------- */
    GSList * roi_points, * temp;
    guint num_points, j;
    AmitkCanvasPoint roi_cpoint;
    GdkPixbuf * pixbuf;
    AmitkCanvasPoint offset_cpoint;
    AmitkCanvasPoint corner_cpoint;
    AmitkRoi * roi = AMITK_ROI(object);
    amide_real_t pixel_dim;
    AmitkPoint offset, corner;

    pixel_dim = (1/canvas->zoom)*canvas->voxel_dim; /* compensate for zoom */

    if (canvas->active_ds == NULL)
      outline_color = amitk_color_table_outline_color(AMITK_COLOR_TABLE_BW_LINEAR, TRUE);
    else
      outline_color = amitk_color_table_outline_color(AMITK_DATA_SET_COLOR_TABLE(canvas->active_ds), TRUE);
    fill_color_rgba = amitk_color_table_rgba_to_uint32(outline_color);

    switch(AMITK_ROI_TYPE(roi)) {
    case AMITK_ROI_TYPE_ISOCONTOUR_2D:
    case AMITK_ROI_TYPE_ISOCONTOUR_3D:

      offset = zero_point;
      corner = one_point;
      pixbuf = image_slice_intersection(roi, canvas->volume, pixel_dim,
					outline_color,&offset, &corner);
      
      offset_cpoint = p_2_cpoint(canvas, amitk_space_b2s(AMITK_SPACE(canvas->volume), 
							 offset));
      corner_cpoint = p_2_cpoint(canvas, amitk_space_b2s(AMITK_SPACE(canvas->volume), 
							 corner));

      /* find the north west corner (in terms of the X reference frame) */
      if (corner_cpoint.y < offset_cpoint.y) offset_cpoint.y = corner_cpoint.y;
      if (corner_cpoint.x < offset_cpoint.x) offset_cpoint.x = corner_cpoint.x;
      
      /* create the item */ 
      if (new) {
	item =  gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
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
    default:
      roi_points =  
	amitk_roi_get_intersection_line(roi, canvas->volume, pixel_dim);
    
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
	  roi_cpoint = p_2_cpoint(canvas, *((AmitkPoint *) temp->data));
	  points->coords[j] = roi_cpoint.x;
	  points->coords[j+1] = roi_cpoint.y;
	  temp=temp->next;
	  j += 2;
	}
      } else {
	/* throw in junk we'll hide*/
	hide_object = TRUE;
	points = gnome_canvas_points_new(4);
	points->coords[0] = points->coords[1] = 0;
	points->coords[2] = points->coords[3] = 1;
      }
      roi_points = amitk_roi_free_points_list(roi_points);

      if (new) {   /* create the item */ 
	item =  gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
				      gnome_canvas_line_get_type(), "points", points,
				      "fill_color_rgba",fill_color_rgba,
				      "width_pixels", canvas->roi_width,
				      "line_style", canvas->line_style,
				      NULL);
      } else {
	/* and reset the line points */
	gnome_canvas_item_set(item, "points", points, "fill_color_rgba", fill_color_rgba,
			      "width_pixels", canvas->roi_width, "line_style", canvas->line_style, 
			      NULL);
      }
      gnome_canvas_points_unref(points);
      break;
    }
  }
 
  /* make sure the point is on this canvas */
  if (hide_object) // || (canvas->volumes == NULL))
    gnome_canvas_item_hide(item);
  else
    gnome_canvas_item_show(item);

  if (new) {
    g_object_set_data(G_OBJECT(item), "object", object);
    g_signal_connect(G_OBJECT(item), "event", G_CALLBACK(canvas_event_cb), canvas);
  }

  return item;

}


static void canvas_update_objects(AmitkCanvas * canvas, gboolean all) {

  GList * object_items;
  GnomeCanvasItem * item;

  if (all)
    object_items = canvas->object_items;
  else
    object_items = canvas->next_update_items;

  while (object_items != NULL) {
    item = object_items->data;
    canvas_update_object(canvas, item, NULL);
    object_items = object_items->next;
  }
}


static void canvas_update_setup(AmitkCanvas * canvas) {

  gboolean first_time = FALSE;

  if (canvas->canvas != NULL) {
    /* add a ref so they aren't destroyed when removed from the container */
    g_object_ref(G_OBJECT(canvas->label));
    g_object_ref(G_OBJECT(canvas->canvas));
    g_object_ref(G_OBJECT(canvas->scrollbar));

    gtk_container_remove(GTK_CONTAINER(canvas), canvas->label);
    gtk_container_remove(GTK_CONTAINER(canvas), canvas->canvas);
    gtk_container_remove(GTK_CONTAINER(canvas), canvas->scrollbar);

  } else {
    first_time = TRUE;

    canvas->label = gtk_label_new(view_names[canvas->view]);

    //    canvas->canvas = gnome_canvas_new_aa();
    canvas->canvas = gnome_canvas_new();

    canvas->scrollbar_adjustment = gtk_adjustment_new(0.5, 0, 1, 1, 1, 1); /* junk values */
    g_signal_connect(canvas->scrollbar_adjustment, "value_changed", 
    		       G_CALLBACK(canvas_scrollbar_cb), canvas);
    canvas->scrollbar = gtk_hscrollbar_new(GTK_ADJUSTMENT(canvas->scrollbar_adjustment));
    gtk_range_set_update_policy(GTK_RANGE(canvas->scrollbar), GTK_UPDATE_CONTINUOUS);

  }

  /* pack it in based on what the layout is */
  switch(canvas->layout) {


  case AMITK_LAYOUT_ORTHOGONAL:
    switch(canvas->view) {
    case AMITK_VIEW_CORONAL:
      gtk_box_pack_start(GTK_BOX(canvas), canvas->canvas, TRUE, TRUE, BOX_SPACING);
      gtk_box_pack_start(GTK_BOX(canvas), canvas->scrollbar, FALSE, FALSE, BOX_SPACING);
      gtk_box_pack_start(GTK_BOX(canvas), canvas->label, FALSE, FALSE, BOX_SPACING);
      break;
    case AMITK_VIEW_TRANSVERSE:
    case AMITK_VIEW_SAGITTAL:
    default:
      gtk_box_pack_start(GTK_BOX(canvas), canvas->label, FALSE, FALSE, BOX_SPACING);
      gtk_box_pack_start(GTK_BOX(canvas), canvas->scrollbar, FALSE, FALSE, BOX_SPACING);
      gtk_box_pack_start(GTK_BOX(canvas), canvas->canvas, TRUE,TRUE,BOX_SPACING);
      break;
    }
    break;


  case AMITK_LAYOUT_LINEAR:
  default:
    gtk_box_pack_start(GTK_BOX(canvas), canvas->label, FALSE, FALSE, BOX_SPACING);
    gtk_box_pack_start(GTK_BOX(canvas), canvas->canvas, TRUE,TRUE,BOX_SPACING);
    gtk_box_pack_start(GTK_BOX(canvas), canvas->scrollbar, FALSE, FALSE,BOX_SPACING);
    break;
  }



  if (first_time) {
    canvas_add_update(canvas, UPDATE_ALL);
  } else {
    if (canvas->view == AMITK_VIEW_SAGITTAL)
      canvas_add_update(canvas, UPDATE_ALL);
    g_object_unref(G_OBJECT(canvas->label));
    g_object_unref(G_OBJECT(canvas->canvas));
    g_object_unref(G_OBJECT(canvas->scrollbar));
  }


}

static void canvas_add_update(AmitkCanvas * canvas, guint update_type) {

  canvas->next_update = canvas->next_update | update_type;

  if (canvas->idle_handler_id == 0)
    canvas->idle_handler_id = 
      gtk_idle_add_priority(G_PRIORITY_HIGH_IDLE,canvas_update_while_idle, canvas);

  return;
}

static gboolean canvas_update_while_idle(gpointer data) {

  AmitkCanvas * canvas = data;

  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(canvas));

  /* update the coorners */
  if (canvas_recalc_corners(canvas)) /* if corners changed */
    canvas->next_update = canvas->next_update | UPDATE_ALL;

  if (canvas->next_update & UPDATE_DATA_SETS) {
    /* freeing the slices indicates to regenerate them, and fallthrough to refresh */
    amitk_objects_unref(canvas->slices);
    canvas->slices = NULL;
  } 
  
  if ((canvas->next_update & REFRESH_DATA_SETS) ||(canvas->next_update & UPDATE_DATA_SETS)) {
    canvas_update_pixbuf(canvas);
  } 
  
  if (canvas->next_update & UPDATE_ARROWS) {
    canvas_update_arrows(canvas, canvas->center, AMITK_VOLUME_Z_CORNER(canvas->volume));
  }
  
  if (canvas->next_update & UPDATE_SCROLLBAR) {
    canvas_update_scrollbar(canvas, canvas->center, AMITK_VOLUME_Z_CORNER(canvas->volume));
  } 

  if (canvas->next_update & (UPDATE_OBJECTS | UPDATE_OBJECT)) {
    canvas_update_objects(canvas, (canvas->next_update & UPDATE_OBJECTS));

    if (canvas->next_update_items != NULL) {
      g_list_free(canvas->next_update_items);
      canvas->next_update_items = NULL;
    }
  } 


  if (canvas->next_update & UPDATE_CROSS) {
    canvas_update_cross(canvas, canvas->next_cross_action, canvas->next_cross_center,
			canvas->next_cross_color, canvas->next_cross_thickness);
  }

  ui_common_remove_cursor(GTK_WIDGET(canvas));
  canvas->next_update = UPDATE_NONE;

  gtk_idle_remove(canvas->idle_handler_id);
  canvas->idle_handler_id=0;

  return FALSE;
}






GtkWidget * amitk_canvas_new(AmitkStudy * study,
			     AmitkView view, 
			     AmitkLayout layout, 
			     GdkLineStyle line_style,
			     gint roi_width,
			     AmitkDataSet * active_ds,
			     gboolean with_arrows) {

  AmitkCanvas * canvas;

  canvas = g_object_new(amitk_canvas_get_type(), NULL);

  canvas->view = view;
  canvas->layout = layout;
  canvas->line_style = line_style;
  canvas->roi_width = roi_width;
  canvas->with_arrows = with_arrows;

  amitk_space_set_view_space(AMITK_SPACE(canvas->volume), canvas->view, canvas->layout);
  
  canvas_update_setup(canvas);
  amitk_canvas_add_object(canvas, AMITK_OBJECT(study));
  if (active_ds != NULL)
    amitk_canvas_set_active_data_set(canvas, active_ds);

  return GTK_WIDGET (canvas);
}

void amitk_canvas_set_layout(AmitkCanvas * canvas, AmitkLayout new_layout) {

  g_return_if_fail(AMITK_IS_CANVAS(canvas));

  canvas->layout = new_layout;

  /* update the coord frame accordingly */
  amitk_space_set_view_space(AMITK_SPACE(canvas->volume), canvas->view, canvas->layout);
  canvas_recalc_corners(canvas); /* make sure everything's specified correctly */
  canvas_update_setup(canvas);

}



void amitk_canvas_set_active_data_set(AmitkCanvas * canvas, 
				      AmitkDataSet * active_ds) {

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_DATA_SET(active_ds) || (active_ds == NULL));
  if (canvas->active_ds != active_ds) {
    canvas->active_ds = active_ds;
    if (canvas->fuse_type == AMITK_FUSE_TYPE_BLEND) 
      canvas_add_update(canvas, UPDATE_OBJECTS);
    else /* AMITK_FUSE_TYPE_OVERLAY */
      canvas_add_update(canvas, UPDATE_DATA_SETS);
  }
}

void amitk_canvas_set_line_style(AmitkCanvas * canvas, 
				 GdkLineStyle new_line_style) {

  g_return_if_fail(AMITK_IS_CANVAS(canvas));

  if (canvas->line_style != new_line_style) {
    canvas->line_style = new_line_style;
    canvas_add_update(canvas, UPDATE_OBJECTS);
  }
}

void amitk_canvas_set_roi_width(AmitkCanvas * canvas, 
				gint new_roi_width) {

  g_return_if_fail(AMITK_IS_CANVAS(canvas));

  if (canvas->roi_width != new_roi_width) {
    canvas->roi_width = new_roi_width;
    canvas_add_update(canvas, UPDATE_OBJECTS);
  }
}


/* notes: adding a study object doesn't actually add it to a canvas,
   it just changes the view parameters to that of the study's */
void amitk_canvas_add_object(AmitkCanvas * canvas, AmitkObject * object) {

  GnomeCanvasItem * item = NULL;

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_OBJECT(object));

  /* make sure we don't already have it */
  if (g_list_index(canvas->objects, object) >= 0) return;

  canvas->objects = g_list_append(canvas->objects, g_object_ref(object));
  if (AMITK_IS_STUDY(object)) {
    canvas->center = AMITK_STUDY_VIEW_CENTER(object);
    amitk_volume_set_z_corner(canvas->volume, AMITK_STUDY_VIEW_THICKNESS(object));
    canvas->voxel_dim = AMITK_STUDY_VOXEL_DIM(object);
    canvas->zoom = AMITK_STUDY_ZOOM(object);
    canvas->duration = AMITK_STUDY_VIEW_DURATION(object);
    canvas->start_time = AMITK_STUDY_VIEW_START_TIME(object);
    canvas->fuse_type = AMITK_STUDY_FUSE_TYPE(object);

    canvas_add_update(canvas, UPDATE_ALL);

  } else if (AMITK_IS_DATA_SET(object)) {
    canvas_add_update(canvas, UPDATE_ALL);
  } else { /* all other objects */
    item = canvas_update_object(canvas, NULL, object);
    canvas_add_update(canvas, UPDATE_VIEW);
  }

  g_signal_connect(G_OBJECT(object), "space_changed",
		   G_CALLBACK(canvas_space_changed_cb), canvas);
  if (AMITK_IS_STUDY(object))
    g_signal_connect(G_OBJECT(object), "study_changed",
  		     G_CALLBACK(canvas_study_changed_cb), canvas);
  if (AMITK_IS_VOLUME(object))
    g_signal_connect(G_OBJECT(object), "volume_changed",
  		     G_CALLBACK(canvas_volume_changed_cb), canvas);
  if (AMITK_IS_FIDUCIAL_MARK(object))
    g_signal_connect(G_OBJECT(object), "fiducial_mark_changed",
		     G_CALLBACK(canvas_fiducial_mark_changed_cb), canvas);
  if (AMITK_IS_ROI(object))
    g_signal_connect(G_OBJECT(object), "roi_changed",
		     G_CALLBACK(canvas_roi_changed_cb), canvas);
  if (AMITK_IS_DATA_SET(object)) {
    g_signal_connect(G_OBJECT(object), "data_set_changed",
		     G_CALLBACK(canvas_data_set_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "interpolation_changed",
		     G_CALLBACK(canvas_data_set_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "thresholding_changed",
		     G_CALLBACK(canvas_thresholding_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "color_table_changed",
		     G_CALLBACK(canvas_color_table_changed_cb), canvas);
  }

  if (item)
    canvas->object_items = g_list_append(canvas->object_items, item);

  return;
}


gboolean amitk_canvas_remove_object(AmitkCanvas * canvas, 
				    AmitkObject * object) {

  GnomeCanvasItem * found_item;
  GList * objects;

  g_return_val_if_fail(AMITK_IS_CANVAS(canvas), FALSE);
  g_return_val_if_fail(AMITK_IS_OBJECT(object), FALSE);
    
  if (g_list_index(canvas->objects, object) <0) return TRUE; 
  
  /* remove children */
  objects= AMITK_OBJECT_CHILDREN(object);
  while (objects != NULL) {
    amitk_canvas_remove_object(canvas, objects->data);
    objects = objects->next;
  }
  
  /* remove object from canvas */
  canvas->objects = g_list_remove(canvas->objects, object);

  g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_space_changed_cb, canvas);
  if (AMITK_IS_STUDY(object))
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_study_changed_cb, canvas);
  if (AMITK_IS_VOLUME(object))
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_volume_changed_cb, canvas);
  if (AMITK_IS_FIDUCIAL_MARK(object))
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_fiducial_mark_changed_cb, canvas);
  if (AMITK_IS_ROI(object))
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_roi_changed_cb, canvas);
  if (AMITK_IS_DATA_SET(object)) {
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_data_set_changed_cb, canvas);
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_thresholding_changed_cb, canvas);
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_color_table_changed_cb, canvas);
  }
  
  /* find corresponding CanvasItem and destroy */
  found_item = canvas_find_item(canvas, object);
  
  if (found_item) {
    canvas->object_items = g_list_remove(canvas->object_items, found_item);
    gtk_object_destroy(GTK_OBJECT(found_item));
    canvas_add_update(canvas, UPDATE_VIEW); /* needed to check if we need to reset the view slice */
  } else if (AMITK_IS_DATA_SET(object)) {
    canvas_add_update(canvas, UPDATE_ALL);
  }

  g_object_unref(object);
  
  return TRUE;
}


void amitk_canvas_update_cross(AmitkCanvas * canvas, AmitkCanvasCrossAction action, 
			       AmitkPoint center, rgba_t color, amide_real_t thickness) {

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  canvas->next_cross_action = action;
  canvas->next_cross_center = center;
  canvas->next_cross_color =  color;
  canvas->next_cross_thickness = thickness;
  canvas_add_update(canvas, UPDATE_CROSS);
}


 
