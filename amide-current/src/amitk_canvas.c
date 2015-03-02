/* amitk_canvas.c
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
#include "amitk_study.h"
#include "amide.h"
#include "amitk_canvas.h"
#include "amitk_canvas_object.h"
#include "image.h"
#include "ui_common.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"

#define BOX_SPACING 3
#define DEFAULT_CANVAS_TRIANGLE_WIDTH 8.0
#define DEFAULT_CANVAS_TRIANGLE_HEIGHT 8.0
#define DEFAULT_CANVAS_BORDER_WIDTH 12.0
#define DEFAULT_CANVAS_TRIANGLE_SEPARATION 2.0 /* distance between triangle and canvas */
#define DEFAULT_CANVAS_TRIANGLE_BORDER 2.0 /* should be DEFAULT_CANVAS_BORDER_WIDTH-DEFAULT_CANVAS_TRIANGLE_HEIGHT-DEFAULT_CANVAS_TRIANGLE_SEPARATION */

#define UPDATE_NONE 0x00
#define UPDATE_VIEW 0x01
#define UPDATE_ARROWS 0x02
#define UPDATE_LINE_PROFILE 0x04
#define UPDATE_DATA_SETS 0x08
#define UPDATE_SCROLLBAR 0x10
#define UPDATE_OBJECT 0x20
#define UPDATE_OBJECTS 0x40
#define UPDATE_TARGET 0x80
#define UPDATE_TIME 0x100
#define UPDATE_SUBJECT_ORIENTATION 0x200
#define UPDATE_ALL 0x2FF

#define cp_2_p(canvas, canvas_cpoint) (canvas_point_2_point(AMITK_VOLUME_CORNER((canvas)->volume),\
							    (canvas)->pixbuf_width, \
							    (canvas)->pixbuf_height,\
							    (canvas)->border_width, \
							    (canvas)->border_width, \
							    (canvas_cpoint)))

#define p_2_cp(canvas, canvas_cpoint) (point_2_canvas_point(AMITK_VOLUME_CORNER((canvas)->volume),\
							    (canvas)->pixbuf_width, \
							    (canvas)->pixbuf_height,\
							    (canvas)->border_width, \
							    (canvas)->border_width, \
							    (canvas_cpoint)))

gchar * orientation_label[6] = {
  N_("A"), N_("P"), N_("L"), N_("R"), N_("S"), N_("I")
};

enum {
  ANTERIOR, 
  POSTERIOR, 
  LEFT, 
  RIGHT, 
  SUPERIOR, 
  INFERIOR
};


enum {
  HELP_EVENT,
  VIEW_CHANGING,
  VIEW_CHANGED,
  ERASE_VOLUME,
  NEW_OBJECT,
  LAST_SIGNAL
} amitk_canvas_signals;

typedef enum {
  CANVAS_EVENT_NONE,
  CANVAS_EVENT_ENTER_OBJECT,
  CANVAS_EVENT_LEAVE, 
  CANVAS_EVENT_PRESS_MOVE_VIEW,
  CANVAS_EVENT_PRESS_MINIMIZE_VIEW,
  CANVAS_EVENT_PRESS_RESIZE_VIEW, 
  CANVAS_EVENT_PRESS_SHIFT_OBJECT, /* currently data set only */
  CANVAS_EVENT_PRESS_ROTATE_OBJECT, /* currently study or data set only */
  CANVAS_EVENT_PRESS_NEW_ROI,
  CANVAS_EVENT_PRESS_SHIFT_OBJECT_IMMEDIATE,  /* currently roi/line_profile/fiducial only */
  CANVAS_EVENT_PRESS_ROTATE_OBJECT_IMMEDIATE, /* currently roi/line_profile only */ /* 10 */
  CANVAS_EVENT_PRESS_RESIZE_ROI,
  CANVAS_EVENT_PRESS_ENTER_DRAWING_MODE,
  CANVAS_EVENT_PRESS_LEAVE_DRAWING_MODE,
  CANVAS_EVENT_PRESS_DRAW_POINT,
  CANVAS_EVENT_PRESS_DRAW_LARGE_POINT,
  CANVAS_EVENT_PRESS_ERASE_POINT,
  CANVAS_EVENT_PRESS_ERASE_LARGE_POINT,
  CANVAS_EVENT_PRESS_CHANGE_ISOCONTOUR, 
  CANVAS_EVENT_PRESS_ERASE_VOLUME_INSIDE_ROI, 
  CANVAS_EVENT_PRESS_ERASE_VOLUME_OUTSIDE_ROI,
  CANVAS_EVENT_PRESS_NEW_OBJECT, 
  CANVAS_EVENT_MOTION, 
  CANVAS_EVENT_MOTION_MOVE_VIEW, 
  CANVAS_EVENT_MOTION_MINIMIZE_VIEW, 
  CANVAS_EVENT_MOTION_RESIZE_VIEW, 
  CANVAS_EVENT_MOTION_SHIFT_OBJECT,
  CANVAS_EVENT_MOTION_ROTATE_OBJECT,
  CANVAS_EVENT_MOTION_NEW_ROI,
  CANVAS_EVENT_MOTION_RESIZE_ROI, 
  CANVAS_EVENT_MOTION_DRAW_POINT, 
  CANVAS_EVENT_MOTION_DRAW_LARGE_POINT, 
  CANVAS_EVENT_MOTION_ERASE_POINT,
  CANVAS_EVENT_MOTION_ERASE_LARGE_POINT,
  CANVAS_EVENT_MOTION_CHANGE_ISOCONTOUR,  
  CANVAS_EVENT_RELEASE_MOVE_VIEW,  
  CANVAS_EVENT_RELEASE_MINIMIZE_VIEW, 
  CANVAS_EVENT_RELEASE_RESIZE_VIEW, 
  CANVAS_EVENT_RELEASE_SHIFT_OBJECT,
  CANVAS_EVENT_RELEASE_ROTATE_OBJECT,
  CANVAS_EVENT_RELEASE_NEW_ROI,
  CANVAS_EVENT_RELEASE_SHIFT_OBJECT_IMMEDIATE, /* currently roi/line_profie/fiducial only */
  CANVAS_EVENT_RELEASE_ROTATE_OBJECT_IMMEDIATE, 
  CANVAS_EVENT_RELEASE_RESIZE_ROI, 
  CANVAS_EVENT_RELEASE_DRAW_POINT,
  CANVAS_EVENT_RELEASE_DRAW_LARGE_POINT,
  CANVAS_EVENT_RELEASE_ERASE_POINT,
  CANVAS_EVENT_RELEASE_ERASE_LARGE_POINT,
  CANVAS_EVENT_RELEASE_CHANGE_ISOCONTOUR, 
  CANVAS_EVENT_CANCEL_SHIFT_OBJECT, 
  CANVAS_EVENT_CANCEL_ROTATE_OBJECT,
  CANVAS_EVENT_CANCEL_CHANGE_ISOCONTOUR,
  CANVAS_EVENT_ENACT_SHIFT_OBJECT,  
  CANVAS_EVENT_ENACT_ROTATE_OBJECT,
  CANVAS_EVENT_ENACT_CHANGE_ISOCONTOUR,
  CANVAS_EVENT_SCROLL_UP,
  CANVAS_EVENT_SCROLL_DOWN,
} canvas_event_t;


static void canvas_class_init (AmitkCanvasClass *klass);
static void canvas_init (AmitkCanvas *canvas);
static void canvas_destroy(GtkObject * object);

static GnomeCanvasItem * canvas_find_item(AmitkCanvas * canvas, AmitkObject * object);
static GList * canvas_add_current_objects(AmitkCanvas * canvas, GList * objects);
static void canvas_space_changed_cb(AmitkSpace * space, gpointer canvas);
static void canvas_object_selection_changed_cb(AmitkObject * object, gpointer canvas);
static void canvas_object_add_child_cb(AmitkObject * parent, AmitkObject * child, gpointer data);
static void canvas_object_remove_child_cb(AmitkObject * parent, AmitkObject * child, gpointer data);
static void canvas_view_changed_cb(AmitkStudy * study, gpointer canvas);
static void canvas_roi_preference_changed_cb(AmitkStudy * study, gpointer canvas);
static void canvas_general_preference_changed_cb(AmitkStudy * study, gpointer canvas);
static void canvas_line_profile_changed_cb(AmitkLineProfile * line_profile, gpointer data);
static void canvas_target_preference_changed_cb(AmitkStudy * study, gpointer canvas);
static void canvas_layout_preference_changed_cb(AmitkStudy * study, gpointer canvas);
static void canvas_study_changed_cb(AmitkStudy * study, gpointer canvas);
static void canvas_target_changed_cb(AmitkStudy * study, gpointer canvas);
static void canvas_time_changed_cb(AmitkStudy * study, gpointer canvas);
static void canvas_volume_changed_cb(AmitkVolume * vol, gpointer canvas);
static void canvas_roi_changed_cb(AmitkRoi * roi, gpointer canvas);
static void canvas_fiducial_mark_changed_cb(AmitkFiducialMark * fm, gpointer canvas);
static void canvas_data_set_invalidate_slice_cache(AmitkDataSet * ds, gpointer data);
static void data_set_changed_cb(AmitkDataSet * ds, gpointer canvas);
static void data_set_subject_orientation_changed_cb(AmitkDataSet * ds, gpointer canvas);
static void data_set_thresholding_changed_cb(AmitkDataSet * ds, gpointer data);
static void data_set_color_table_changed_cb(AmitkDataSet * ds, AmitkViewMode view_mode, gpointer data);
static amide_real_t canvas_check_z_dimension(AmitkCanvas * canvas, amide_real_t z);
static void canvas_create_isocontour_roi(AmitkCanvas * canvas, AmitkRoi * roi, 
					 AmitkPoint position, AmitkDataSet * active_slice);
static gboolean canvas_create_freehand_roi(AmitkCanvas * canvas, AmitkRoi * roi, 
					   AmitkPoint position, AmitkDataSet * active_slice);
static gboolean canvas_event_cb(GtkWidget* widget,  GdkEvent * event, gpointer data);
static void canvas_scrollbar_adjustment_cb(GtkObject * adjustment, gpointer data);

static gboolean canvas_recalc_corners(AmitkCanvas * canvas);
static void canvas_update_scrollbar(AmitkCanvas * canvas, AmitkPoint center, amide_real_t thickness);
static void canvas_update_target(AmitkCanvas * canvas, AmitkCanvasTargetAction action, 
				 AmitkPoint center, amide_real_t thickness);
static void canvas_update_arrows(AmitkCanvas * canvas);
static void canvas_update_line_profile(AmitkCanvas * canvas);
static void canvas_update_time_on_image(AmitkCanvas * canvas);
static void canvas_update_subject_orientation(AmitkCanvas * canvas);
static void canvas_update_pixbuf(AmitkCanvas * canvas);
static void canvas_update_object(AmitkCanvas * canvas, AmitkObject * object);
static void canvas_update_objects(AmitkCanvas * canvas, gboolean all);
static void canvas_update_setup(AmitkCanvas * canvas);
static void canvas_add_object_update(AmitkCanvas * canvas, AmitkObject * object);
static void canvas_add_update(AmitkCanvas * canvas, guint update_type);
static gboolean canvas_update_while_idle(gpointer canvas);
static void canvas_add_object(AmitkCanvas * canvas, AmitkObject * object);
static void canvas_remove_object(AmitkCanvas * canvas, AmitkObject * object);


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
  GtkObjectClass *gtkobject_class;
  /*  GtkWidgetClass *widget_class;
  GObjectClass   *gobject_class; */

  gtkobject_class = (GtkObjectClass*) klass;
  /*  widget_class =    (GtkWidgetClass*) klass;
      gobject_class =   (GObjectClass *) klass; */

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
  for (i=0;i<4;i++) canvas->arrows[i]=NULL;
  for (i=0;i<8;i++) canvas->target[i]=NULL;
  for (i=0;i<4;i++) canvas->orientation_label[i]=NULL;
  canvas->line_profile_item = NULL;
  canvas->type = AMITK_CANVAS_TYPE_NORMAL;

  canvas->volume = amitk_volume_new();
  amitk_volume_set_corner(canvas->volume, one_point);

  canvas->center = zero_point;

  canvas->active_object = NULL;

  canvas->canvas = NULL;
  canvas->slice_cache = NULL;
  canvas->max_slice_cache_size = 15;
  canvas->slices=NULL;
  canvas->image=NULL;
  canvas->pixbuf=NULL;

  canvas->time_on_image=FALSE;
  canvas->time_label=NULL;

  canvas->pixbuf_width = 128;
  canvas->pixbuf_height = 128;

  canvas->study = NULL;
  canvas->undrawn_rois = NULL;
  canvas->object_items=NULL;

  canvas->next_update = 0;
  canvas->idle_handler_id = 0;
  canvas->next_update_objects = NULL;

}

static void canvas_destroy (GtkObject * object) {

  AmitkCanvas * canvas;

  g_return_if_fail (object != NULL);
  g_return_if_fail (AMITK_IS_CANVAS (object));

  canvas = AMITK_CANVAS(object);

  if (canvas->idle_handler_id != 0) {
    g_source_remove(canvas->idle_handler_id);
    canvas->idle_handler_id = 0;
  }

  if (canvas->next_update_objects != NULL) {
    canvas->next_update_objects = amitk_objects_unref(canvas->next_update_objects);
  }

  if (canvas->volume != NULL) 
    canvas->volume = amitk_object_unref(canvas->volume);

  if (canvas->slice_cache != NULL) {
    canvas->slice_cache = amitk_objects_unref(canvas->slice_cache);
  }

  if (canvas->slices != NULL) {
    canvas->slices = amitk_objects_unref(canvas->slices);
  }

  if (canvas->pixbuf != NULL) {
    g_object_unref(canvas->pixbuf);
    canvas->pixbuf = NULL;
  }

  if (canvas->undrawn_rois != NULL) {
    canvas->undrawn_rois = amitk_objects_unref(canvas->undrawn_rois);
  }

  if (canvas->study != NULL) {
    canvas_remove_object(canvas, AMITK_OBJECT(canvas->study));
    canvas->study = NULL;
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

/* returns a referenced list of all the objects that currently have items on display.
   If objects != NULL, the returned list will be the concatenation of "objects" and 
   currently appearing items not in "objects" */
static GList * canvas_add_current_objects(AmitkCanvas * canvas, GList * objects) {

  AmitkObject * object;
  GList * items=canvas->object_items;

  while (items != NULL) {
    object = g_object_get_data(G_OBJECT(items->data), "object");

    if (object != NULL)
      if (g_list_index(objects, object) < 0) 
	objects = g_list_append(objects, amitk_object_ref(object)); 

    items = items->next;
  }

  return objects;
}


AmitkColorTable canvas_get_color_table(AmitkCanvas * canvas) {

  if (canvas->active_object != NULL)
    if (AMITK_IS_DATA_SET(canvas->active_object))
      return amitk_data_set_get_color_table_to_use(AMITK_DATA_SET(canvas->active_object), AMITK_CANVAS_VIEW_MODE(canvas));

  return AMITK_COLOR_TABLE_BW_LINEAR; /* default */
}





static void canvas_space_changed_cb(AmitkSpace * space, gpointer data) {

  AmitkObject * object;
  AmitkCanvas * canvas = data;  

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_OBJECT(space));
  object = AMITK_OBJECT(space);

  canvas_add_object_update(canvas, object);

  return;
}


static void canvas_object_selection_changed_cb(AmitkObject * object, gpointer data) {

  AmitkCanvas * canvas = data;
  GnomeCanvasItem * item;
  gboolean undrawn_roi=FALSE;

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_OBJECT(object));

  if (AMITK_IS_DATA_SET(object)) { /* update on select or unselect */
    canvas_add_update(canvas, UPDATE_ALL);

  } else {
    if (AMITK_IS_ROI(object))
      undrawn_roi = AMITK_ROI_UNDRAWN(object);

    if (amitk_object_get_selected(object, canvas->view_mode)) {

      if (undrawn_roi) {
	if (g_list_index(canvas->undrawn_rois, object) < 0)  /* not yet in list */
	  canvas->undrawn_rois = g_list_prepend(canvas->undrawn_rois, amitk_object_ref(object));
      } else {
	canvas_add_object_update(canvas, object);
      }

    } else {

      if (undrawn_roi) {
	if (g_list_index(canvas->undrawn_rois, object) >= 0) {
	  canvas->undrawn_rois = g_list_remove(canvas->undrawn_rois, object);
	  amitk_object_unref(object);
	}
      } else if ((item = canvas_find_item(canvas, object)) != NULL) { /* an unselect */
	canvas->object_items = g_list_remove(canvas->object_items, item);
	gtk_object_destroy(GTK_OBJECT(item));
	canvas_add_update(canvas, UPDATE_VIEW); /* needed to check if we need to reset the view slice */
      }
    }
  }

  return;
}

static void canvas_object_add_child_cb(AmitkObject * parent, AmitkObject * child, gpointer data) {

  AmitkCanvas * canvas = data;

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_OBJECT(child));

  canvas_add_object(AMITK_CANVAS(canvas), child);

  return;
}

static void canvas_object_remove_child_cb(AmitkObject * parent, AmitkObject * child, gpointer data) {

  AmitkCanvas * canvas = data;

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_OBJECT(child));
  canvas_remove_object(AMITK_CANVAS(canvas), child);

  return;
}

static void canvas_view_changed_cb(AmitkStudy * study, gpointer data) {

  AmitkCanvas * canvas = data;  
  gboolean changed=FALSE;
  gboolean coords_changed = FALSE;
  AmitkPoint temp[2];

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_OBJECT(study));

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

  if (changed) {
    canvas_add_update(canvas, UPDATE_ALL);
  } else if (coords_changed) {
    canvas_add_update(canvas, UPDATE_ARROWS);
    canvas_add_update(canvas, UPDATE_TARGET);
    canvas_add_update(canvas, UPDATE_LINE_PROFILE);
  }

  return;
}

static void canvas_roi_preference_changed_cb(AmitkStudy * study, gpointer data) {
  AmitkCanvas * canvas = data;
  canvas_add_update(canvas, UPDATE_OBJECTS); /* line_style, transparency, roi_width, fill_roi */
  return;
}

static void canvas_general_preference_changed_cb(AmitkStudy * study, gpointer data) {
  AmitkCanvas * canvas = data;
  canvas_add_update(canvas, UPDATE_ALL); /* maintain_size */
  return;
}

static void canvas_target_preference_changed_cb(AmitkStudy * study, gpointer data) {
  AmitkCanvas * canvas = data;
  canvas_add_update(canvas, UPDATE_TARGET); /* target_empty_area */
  return;
}

static void canvas_layout_preference_changed_cb(AmitkStudy * study, gpointer data) {
  AmitkCanvas * canvas = data;
  amitk_space_set_view_space(AMITK_SPACE(canvas->volume), canvas->view, 
			     AMITK_STUDY_CANVAS_LAYOUT(canvas->study));
  canvas_recalc_corners(canvas); /* make sure everything's specified correctly */
  canvas_update_setup(canvas);
  return;
}

static void canvas_line_profile_changed_cb(AmitkLineProfile * line_profile, gpointer data) {
  AmitkCanvas * canvas = data;
  canvas_add_update(canvas, UPDATE_LINE_PROFILE);
  return;
}

static void canvas_study_changed_cb(AmitkStudy * study, gpointer data) {
  AmitkCanvas * canvas = data;
  canvas_add_update(canvas, UPDATE_ALL);
  return;
}

static void canvas_target_changed_cb(AmitkStudy * study, gpointer data) {

  AmitkCanvas * canvas = data;  

  if (AMITK_STUDY_CANVAS_TARGET(study))
    canvas->next_target_action = AMITK_CANVAS_TARGET_ACTION_LEAVE;
  else
    canvas->next_target_action = AMITK_CANVAS_TARGET_ACTION_HIDE;
  canvas_add_update(canvas, UPDATE_TARGET);

  return;
}

static void canvas_time_changed_cb(AmitkStudy * study, gpointer data) {
  AmitkCanvas * canvas = data;  
  canvas_add_update(canvas, UPDATE_ALL);
}

static void canvas_volume_changed_cb(AmitkVolume * vol, gpointer data) {

  AmitkCanvas * canvas = data;  

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_VOLUME(vol));

  /* check if we need to remove the roi from the undrawn list */
  if (AMITK_IS_ROI(vol))
    if (!AMITK_ROI_UNDRAWN(vol)) 
      if (g_list_index(canvas->undrawn_rois, vol) >= 0) {
	canvas->undrawn_rois = g_list_remove(canvas->undrawn_rois, vol);
	amitk_object_unref(vol);
      }
  canvas_add_object_update(canvas, AMITK_OBJECT(vol));

  return;
}

static void canvas_roi_changed_cb(AmitkRoi * roi, gpointer data) {

  AmitkCanvas * canvas = data;  

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_ROI(roi));
  canvas_add_object_update(canvas, AMITK_OBJECT(roi));

  return;
}

static void canvas_fiducial_mark_changed_cb(AmitkFiducialMark * fm, gpointer data) {

  AmitkCanvas * canvas = data;  

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_FIDUCIAL_MARK(fm));
  canvas_add_object_update(canvas, AMITK_OBJECT(fm));

  return;
}

static void canvas_data_set_invalidate_slice_cache(AmitkDataSet * ds, gpointer data) {

  AmitkCanvas * canvas = data;  

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_DATA_SET(ds));

  canvas->slice_cache = amitk_data_sets_remove_with_slice_parent(canvas->slice_cache, ds);

}

static void data_set_changed_cb(AmitkDataSet * ds, gpointer data) {

  AmitkCanvas * canvas = data;  

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  canvas_add_object_update(canvas, AMITK_OBJECT(ds));

  return;
}

static void data_set_subject_orientation_changed_cb(AmitkDataSet * ds, gpointer data) {

  AmitkCanvas * canvas = data;  

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_DATA_SET(ds));

  if (ds == AMITK_DATA_SET(canvas->active_object))
    canvas_add_update(canvas, UPDATE_SUBJECT_ORIENTATION);

  return;
}


static void data_set_thresholding_changed_cb(AmitkDataSet * ds, gpointer data) {

  AmitkCanvas * canvas = data;  

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  canvas_add_update(canvas, UPDATE_DATA_SETS);
}

static void data_set_color_table_changed_cb(AmitkDataSet * ds, AmitkViewMode view_mode, gpointer data) {

  AmitkCanvas * canvas = data;  

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_DATA_SET(ds));

  if (view_mode == AMITK_CANVAS_VIEW_MODE(canvas)) {
    canvas_add_update(canvas, UPDATE_DATA_SETS);
    canvas_add_update(canvas, UPDATE_OBJECTS);
  }
}

static void value_spin_cb(GtkWidget * widget, gpointer data) {
  gdouble * value = data;
  *value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  return;
}

static amide_real_t canvas_check_z_dimension(AmitkCanvas * canvas, amide_real_t z) {

  GtkWindow * window;
  GtkWidget * toplevel;
  GtkWidget * dialog;
  GtkWidget * table;
  GtkWidget * label;
  GtkWidget * spin_button;
  GtkWidget * image;

  g_return_val_if_fail(AMITK_IS_CANVAS(canvas), z);

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET(canvas));
  if (toplevel != NULL) window = GTK_WINDOW(toplevel);
  else window = NULL;
      
  dialog = gtk_dialog_new_with_buttons (_("ROI Depth Selection"),
					window,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

  table = gtk_table_new(1,3,FALSE);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);

  /* pick depth of roi */
  image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION,GTK_ICON_SIZE_DIALOG);
  gtk_table_attach(GTK_TABLE(table), image, 0,1, 0,1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);

  label = gtk_label_new(_("Please pick depth of ROI (mm):"));
  gtk_table_attach(GTK_TABLE(table), label, 1,2, 0,1, 0, 0, X_PADDING, Y_PADDING);

  spin_button = gtk_spin_button_new_with_range(0.0, G_MAXDOUBLE, MAX(z,EPSILON));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), z);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_button),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(spin_button), FALSE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_button), FALSE);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin_button), GTK_UPDATE_ALWAYS);
  gtk_entry_set_activates_default(GTK_ENTRY(spin_button), TRUE);
  g_signal_connect(G_OBJECT(spin_button), "value_changed",  G_CALLBACK(value_spin_cb), &z);
  g_signal_connect(G_OBJECT(spin_button), "output", G_CALLBACK(amitk_spin_button_scientific_output), NULL);
  gtk_table_attach(GTK_TABLE(table), spin_button, 2,3, 0,1, GTK_FILL, 0, X_PADDING, Y_PADDING);

  gtk_widget_show_all(dialog);
  gtk_dialog_run(GTK_DIALOG(dialog)); /* let the user input */
  gtk_widget_destroy(dialog);

  return z;
}

static void isocontour_gray_spins(AmitkRoiIsocontourRange isocontour_range, 
				  GtkWidget * min_spin_button,
				  GtkWidget * max_spin_button) {

  switch (isocontour_range) {
  case AMITK_ROI_ISOCONTOUR_RANGE_BETWEEN_MIN_MAX:
    gtk_widget_set_sensitive(min_spin_button, TRUE);
    gtk_widget_set_sensitive(max_spin_button, TRUE);
    break;
  case AMITK_ROI_ISOCONTOUR_RANGE_BELOW_MAX:
    gtk_widget_set_sensitive(min_spin_button, FALSE);
    gtk_widget_set_sensitive(max_spin_button, TRUE);
    break;
  case AMITK_ROI_ISOCONTOUR_RANGE_ABOVE_MIN:
  default:
    gtk_widget_set_sensitive(min_spin_button, TRUE);
    gtk_widget_set_sensitive(max_spin_button, FALSE);
    break;
  }

  return;
}

static void isocontour_range_cb(GtkWidget * widget, gpointer data) {

  AmitkRoiIsocontourRange *pisocontour_range = data;
  GtkWidget * min_spin_button;
  GtkWidget * max_spin_button;

  /* get the state of the button */
  *pisocontour_range = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "isocontour_range"));
  min_spin_button = g_object_get_data(G_OBJECT(widget), "min_spin_button");
  max_spin_button = g_object_get_data(G_OBJECT(widget), "max_spin_button");

  /* appropriately gray them out */
  isocontour_gray_spins(*pisocontour_range, min_spin_button, max_spin_button);

  return;
}

static void canvas_create_isocontour_roi(AmitkCanvas * canvas, AmitkRoi * roi, 
					 AmitkPoint position, AmitkDataSet * active_slice) {

  AmitkPoint temp_point;
  AmitkVoxel temp_voxel;
  amide_time_t start_time, end_time;
  AmitkObject * parent_ds;
  AmitkDataSet * draw_on_ds=NULL;
  GtkWindow * window;
  GtkWidget * toplevel;
  amide_intpoint_t gate;
  GtkWidget * hbox;
  GtkWidget * dialog;
  GtkWidget * table;
  GtkWidget * label;
  GtkWidget * min_spin_button;
  GtkWidget * max_spin_button;
  GtkWidget * image;
  GtkWidget * radio_button[3];
  amide_data_t isocontour_min_value;
  amide_data_t isocontour_max_value;
  AmitkRoiIsocontourRange isocontour_range;
  AmitkRoiIsocontourRange i_range;
  gint table_row;
  gchar * temp_str;
  gint return_val;

	
  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_ROI(roi));
  g_return_if_fail(AMITK_ROI_TYPE_ISOCONTOUR(roi));

  /* figure out the data set we're drawing on */
  parent_ds = amitk_object_get_parent_of_type(AMITK_OBJECT(roi), AMITK_OBJECT_TYPE_DATA_SET);


  if (AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_ISOCONTOUR_2D) {
    if (parent_ds != NULL) {
      draw_on_ds = amitk_data_sets_find_with_slice_parent(canvas->slices,
							  AMITK_DATA_SET(parent_ds));
      if (draw_on_ds == NULL) {
	g_warning(_("Parent of isocontour not currently displayed, can't draw isocontour"));
	return;
      }
    } else if (active_slice != NULL) { /* if no data set parent, just use the active slice */
      draw_on_ds = active_slice;
    }


  } else { /* ISOCONTOUR_3D */
    if (parent_ds != NULL) 
      draw_on_ds = AMITK_DATA_SET(parent_ds);
    else if (canvas->active_object != NULL) 
      if (AMITK_IS_DATA_SET(canvas->active_object))
	draw_on_ds = AMITK_DATA_SET(canvas->active_object);
  }

  if (draw_on_ds == NULL) {
    g_warning(_("No data set found to draw isocontour on"));
    return;
  }


  start_time = AMITK_STUDY_VIEW_START_TIME(canvas->study);
  temp_point = amitk_space_b2s(AMITK_SPACE(draw_on_ds), position);
  if (AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_ISOCONTOUR_3D)
    gate = AMITK_DATA_SET_VIEW_START_GATE(draw_on_ds);
  else
    gate = 0;
  POINT_TO_VOXEL(temp_point, AMITK_DATA_SET_VOXEL_SIZE(draw_on_ds),
		 amitk_data_set_get_frame(AMITK_DATA_SET(draw_on_ds), start_time),
		 gate,
		 temp_voxel);

  if (!amitk_raw_data_includes_voxel(AMITK_DATA_SET_RAW_DATA(draw_on_ds),temp_voxel)) {
    g_warning(_("designated voxel not in data set %s"), AMITK_OBJECT_NAME(draw_on_ds));
    return;
  }

  /* what we probably want to set the isocontour too */
  isocontour_min_value = amitk_data_set_get_value(draw_on_ds, temp_voxel); 
  isocontour_max_value = isocontour_min_value;
  isocontour_range = AMITK_ROI_ISOCONTOUR_RANGE(roi);

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET(canvas));
  if (toplevel != NULL) window = GTK_WINDOW(toplevel);
  else window = NULL;

  /* pop up dialog to let user pick isocontour values, etc. and for any warning messages */
  dialog = gtk_dialog_new_with_buttons (_("Isocontour Value Selection"),
					window,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
					GTK_STOCK_OK, GTK_RESPONSE_OK, 
					NULL);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

  table = gtk_table_new(4,3,FALSE);
  table_row=0;
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);

  /* complain if more then one frame or gate is currently showing for ISOCONTOUR_3D */
  if (AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_ISOCONTOUR_3D) {
    end_time = start_time + AMITK_STUDY_VIEW_DURATION(canvas->study);

    /* warning for multiple frames */
    if (temp_voxel.t != amitk_data_set_get_frame(AMITK_DATA_SET(draw_on_ds), end_time)) {
      image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING,GTK_ICON_SIZE_DIALOG);
      gtk_table_attach(GTK_TABLE(table), image, 0,1, table_row,table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);

      temp_str = g_strdup_printf(_("Multiple data frames are currently being shown from: %s\nThe isocontour will only be drawn over frame %d"),AMITK_OBJECT_NAME(draw_on_ds), temp_voxel.t);
      label = gtk_label_new(temp_str);
      g_free(temp_str);
      gtk_table_attach(GTK_TABLE(table), label, 1,3,table_row,table_row+1, 0, 0, X_PADDING, Y_PADDING);
      table_row++;
    }

    /* warning for multiple gates */
    if (AMITK_DATA_SET_VIEW_START_GATE(draw_on_ds) != AMITK_DATA_SET_VIEW_END_GATE(draw_on_ds)) {
      image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING,GTK_ICON_SIZE_DIALOG);
      gtk_table_attach(GTK_TABLE(table), image, 0,1, table_row,table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);

      temp_str = g_strdup_printf(_("Multiple gates are currently being shown from: %s\nThe isocontour will only be drawn over gate %d"),AMITK_OBJECT_NAME(draw_on_ds), temp_voxel.g);
      label = gtk_label_new(temp_str);
      g_free(temp_str);
      gtk_table_attach(GTK_TABLE(table), label, 1,3, table_row,table_row+1, 0, 0, X_PADDING, Y_PADDING);
      table_row++;
    }
  }

  image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION,GTK_ICON_SIZE_DIALOG);
  gtk_table_attach(GTK_TABLE(table), image, 0,1, table_row,table_row+2, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);

  /* box where the radio buttons will go */
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_table_attach(GTK_TABLE(table), hbox,1,3,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(hbox);
  table_row++;


  /* the spin buttons */
  label = gtk_label_new(_("Min:"));
  gtk_table_attach(GTK_TABLE(table), label, 1,2, table_row,table_row+1, 0, 0, X_PADDING, Y_PADDING);

  min_spin_button = gtk_spin_button_new_with_range(-G_MAXDOUBLE, isocontour_min_value, MAX(EPSILON,fabs(isocontour_min_value)/10));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(min_spin_button), isocontour_min_value);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(min_spin_button),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(min_spin_button), FALSE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(min_spin_button), FALSE);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(min_spin_button), GTK_UPDATE_ALWAYS);
  gtk_entry_set_activates_default(GTK_ENTRY(min_spin_button), TRUE);
  g_signal_connect(G_OBJECT(min_spin_button), "value_changed",  G_CALLBACK(value_spin_cb), &isocontour_min_value);
  g_signal_connect(G_OBJECT(min_spin_button), "output", G_CALLBACK(amitk_spin_button_scientific_output), NULL);
  gtk_table_attach(GTK_TABLE(table), min_spin_button, 2,3, table_row,table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  label = gtk_label_new(_("Max:"));
  gtk_table_attach(GTK_TABLE(table), label, 1,2, table_row,table_row+1, 0, 0, X_PADDING, Y_PADDING);

  max_spin_button = gtk_spin_button_new_with_range(isocontour_max_value, G_MAXDOUBLE, MAX(EPSILON,fabs(isocontour_max_value)/10));
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(max_spin_button), isocontour_max_value);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(max_spin_button),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(max_spin_button), FALSE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(max_spin_button), FALSE);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(max_spin_button), GTK_UPDATE_ALWAYS);
  gtk_entry_set_activates_default(GTK_ENTRY(max_spin_button), TRUE);
  g_signal_connect(G_OBJECT(max_spin_button), "value_changed",  G_CALLBACK(value_spin_cb), &isocontour_max_value);
  g_signal_connect(G_OBJECT(max_spin_button), "output", G_CALLBACK(amitk_spin_button_scientific_output), NULL);
  gtk_table_attach(GTK_TABLE(table), max_spin_button, 2,3, table_row,table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* appropriately gray them out */
  isocontour_gray_spins(isocontour_range, min_spin_button, max_spin_button);

  /* radio buttons to choose the isocontour type */
  radio_button[0] = gtk_radio_button_new_with_label(NULL, _("Above Min"));
  radio_button[1] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_button[0]), _("Below Max"));
  radio_button[2] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_button[0]), _("Between Min/Max"));

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button[isocontour_range]), TRUE);

  for (i_range=0; i_range < AMITK_ROI_ISOCONTOUR_RANGE_NUM; i_range++) {
    gtk_box_pack_start(GTK_BOX(hbox), radio_button[i_range], FALSE, FALSE, 3);
    g_object_set_data(G_OBJECT(radio_button[i_range]), "isocontour_range", 
		      GINT_TO_POINTER(i_range));
    g_object_set_data(G_OBJECT(radio_button[i_range]), "min_spin_button", min_spin_button);
    g_object_set_data(G_OBJECT(radio_button[i_range]), "max_spin_button", max_spin_button);
    g_signal_connect(G_OBJECT(radio_button[i_range]), "clicked", 
		     G_CALLBACK(isocontour_range_cb), &isocontour_range);
  }


  /* run the dialog */
  gtk_widget_show_all(dialog);
  return_val = gtk_dialog_run(GTK_DIALOG(dialog)); /* let the user input */
  gtk_widget_destroy(dialog);

  if (return_val != GTK_RESPONSE_OK)
    return; /* cancel */

  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(canvas));
  amitk_roi_set_isocontour(roi, AMITK_DATA_SET(draw_on_ds), temp_voxel, 
			   isocontour_min_value,isocontour_max_value, isocontour_range);
  ui_common_remove_wait_cursor(GTK_WIDGET(canvas));

  return;
}


/* returns true if sucessfully completed */
static gboolean canvas_create_freehand_roi(AmitkCanvas * canvas, AmitkRoi * roi, 
					   AmitkPoint position, AmitkDataSet * active_slice) {

  AmitkObject * parent_ds;
  AmitkDataSet * draw_on_ds=NULL;
  GtkWindow * window;
  GtkWidget * toplevel;
  GtkWidget * dialog;
  GtkWidget * table;
  GtkWidget * label;
  GtkWidget * spin_button[3];
  GtkWidget * image;
  gint table_row;
  gchar * temp_str;
  gint return_val;
  AmitkPoint voxel_size;
  AmitkPoint temp_point;
  AmitkVoxel temp_voxel;
  AmitkAxis i_axis;

	
  g_return_val_if_fail(AMITK_IS_CANVAS(canvas), FALSE);
  g_return_val_if_fail(AMITK_IS_ROI(roi), FALSE);
  g_return_val_if_fail(AMITK_ROI_TYPE_FREEHAND(roi), FALSE);

  /* figure out the data set we're drawing on */
  parent_ds = amitk_object_get_parent_of_type(AMITK_OBJECT(roi), AMITK_OBJECT_TYPE_DATA_SET);

  if (AMITK_ROI_TYPE(roi) == AMITK_ROI_TYPE_FREEHAND_2D) {
    if (parent_ds != NULL) {
      draw_on_ds = amitk_data_sets_find_with_slice_parent(canvas->slices,
							  AMITK_DATA_SET(parent_ds));
      if (draw_on_ds == NULL) {
	g_warning(_("Parent of roi not currently displayed, can't draw freehand roi"));
	return FALSE;
      }
    } else if (active_slice != NULL) { /* if no data set parent, just use the active slice */
      draw_on_ds = active_slice;
    }


  } else { /* FREEHAND_3D */
    if (parent_ds != NULL) 
      draw_on_ds = AMITK_DATA_SET(parent_ds);
    else if (canvas->active_object != NULL) 
      if (AMITK_IS_DATA_SET(canvas->active_object))
	draw_on_ds = AMITK_DATA_SET(canvas->active_object);
  }

  if (draw_on_ds == NULL) {
    g_warning(_("No data set found to draw freehand roi on"));
    return FALSE;
  }


  voxel_size = AMITK_DATA_SET_VOXEL_SIZE(draw_on_ds);

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET(canvas));
  if (toplevel != NULL) window = GTK_WINDOW(toplevel);
  else window = NULL;

  /* pop up dialog to let user pick isocontour values, etc. and for any warning messages */
  dialog = gtk_dialog_new_with_buttons (_("Freehand ROI Parameters"),
					window,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
					GTK_STOCK_OK, GTK_RESPONSE_OK, 
					NULL);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

  table = gtk_table_new(4,3,FALSE);
  table_row=0;
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);

  image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION,GTK_ICON_SIZE_DIALOG);
  gtk_table_attach(GTK_TABLE(table), image, 0,1, table_row,table_row+2, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);

  /* the spin buttons */
  for (i_axis=0; i_axis<AMITK_AXIS_NUM; i_axis++) {

    temp_str = g_strdup_printf(_("Voxel Size %s"), amitk_axis_get_name(i_axis));
    label = gtk_label_new(temp_str);
    g_free(temp_str);
    gtk_table_attach(GTK_TABLE(table), label, 1,2, table_row,table_row+1, 0, 0, X_PADDING, Y_PADDING);

    spin_button[i_axis] = gtk_spin_button_new_with_range(point_get_component(voxel_size, i_axis)/2,
							 point_get_component(voxel_size, i_axis)*3,
							 point_get_component(voxel_size, i_axis)/2);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button[i_axis]), point_get_component(voxel_size, i_axis));
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_button[i_axis]),FALSE);
    gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(spin_button[i_axis]), FALSE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_button[i_axis]), FALSE);
    gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin_button[i_axis]), GTK_UPDATE_ALWAYS);
    gtk_entry_set_activates_default(GTK_ENTRY(spin_button[i_axis]), TRUE);
    gtk_table_attach(GTK_TABLE(table), spin_button[i_axis], 2,3, table_row,table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    table_row++;
  }

  g_signal_connect(G_OBJECT(spin_button[0]), "value_changed",  G_CALLBACK(value_spin_cb), &(voxel_size.x));
  g_signal_connect(G_OBJECT(spin_button[1]), "value_changed",  G_CALLBACK(value_spin_cb), &(voxel_size.y));
  g_signal_connect(G_OBJECT(spin_button[2]), "value_changed",  G_CALLBACK(value_spin_cb), &(voxel_size.z));


  /* run the dialog */
  gtk_widget_show_all(dialog);
  return_val = gtk_dialog_run(GTK_DIALOG(dialog)); /* let the user input */
  gtk_widget_destroy(dialog);

  if (return_val != GTK_RESPONSE_OK)
    return FALSE; /* cancel */

  /* ensure that the position lines up with the voxels in the data set */
  position = amitk_space_b2s(AMITK_SPACE(draw_on_ds), position);
  POINT_TO_VOXEL(position, AMITK_DATA_SET_VOXEL_SIZE(draw_on_ds), 0,0,temp_voxel);
  VOXEL_CORNER(temp_voxel, AMITK_DATA_SET_VOXEL_SIZE(draw_on_ds), position);
  position = amitk_space_s2b(AMITK_SPACE(draw_on_ds), position);
  

  amitk_roi_set_voxel_size(roi, voxel_size);
  amitk_space_copy_in_place(AMITK_SPACE(roi), AMITK_SPACE(draw_on_ds));
  amitk_space_set_offset(AMITK_SPACE(roi), position);
  temp_point = amitk_space_b2s(AMITK_SPACE(roi), position);
  POINT_TO_VOXEL(temp_point, AMITK_ROI_VOXEL_SIZE(roi), 0, 0, temp_voxel);
  amitk_roi_manipulate_area(roi, FALSE, temp_voxel, 0);

  return TRUE;
}



/* function called when an event occurs on the canvas */
static gboolean canvas_event_cb(GtkWidget* widget,  GdkEvent * event, gpointer data) {

  AmitkCanvas * canvas = data;
  AmitkPoint base_point, canvas_point, diff_point;
  AmitkCanvasPoint canvas_cpoint, diff_cpoint, event_cpoint;
  rgba_t outline_color;
  //  GnomeCanvasPoints * points;
  canvas_event_t canvas_event_type;
  AmitkObject * object=NULL;
  AmitkCanvasPoint temp_cpoint[2];
  AmitkPoint temp_point[2];
  AmitkPoint shift;
  AmitkDataSet * active_slice;
  AmitkVoxel temp_voxel;
  amide_real_t corner;
  AmitkPoint center;
  amide_data_t voxel_value;
  AmitkHelpInfo help_info;
  ui_common_cursor_t cursor_type;
  static GnomeCanvasItem * canvas_item = NULL;
  static gboolean grab_on = FALSE;
  static canvas_event_t extended_event_type = CANVAS_EVENT_NONE;
  static AmitkObject * extended_object= NULL;
  static AmitkCanvasPoint initial_cpoint;
  static AmitkCanvasPoint previous_cpoint;
  static AmitkPoint initial_base_point;
  static AmitkPoint initial_canvas_point;
  static double theta;
  static AmitkPoint zoom;
  static AmitkPoint radius_point;
  static gboolean in_object=FALSE;
  static gboolean in_drawing_mode=FALSE;
  static AmitkObject * drawing_object=NULL;
  static gboolean ignore_next_event=FALSE;
  static gboolean enter_drawing_mode=FALSE; /* we want to enter drawing mode */

  //          g_print("event %d widget %p grab_on %d %p\n", event->type, widget, grab_on, 
  //      	    (GNOME_CANVAS(canvas->canvas)->grabbed_item));
  //      if ((event->type == GDK_BUTTON_PRESS) ||
  //          (event->type == GDK_BUTTON_RELEASE) ||
  //          (event->type == GDK_2BUTTON_PRESS))
  //        g_print("event %d widget %p grab_on %d %p\n", event->type, widget, grab_on, 
  //    	    (GNOME_CANVAS(canvas->canvas)->grabbed_item));

  if (canvas->type == AMITK_CANVAS_TYPE_FLY_THROUGH) return FALSE;

  /* sanity checks */
  if (canvas->study == NULL) return FALSE; 
  if (canvas->slices == NULL) return FALSE;
  g_return_val_if_fail(canvas->active_object != NULL, FALSE);

  if (ignore_next_event) {
    ignore_next_event=FALSE;
    return FALSE;
  }

  /* check if we should bail out of drawing mode */
  if (in_drawing_mode) {
    if (drawing_object != NULL) {
      if (!amitk_object_get_selected(drawing_object, AMITK_SELECTION_ANY)) {
	in_drawing_mode = FALSE;
	drawing_object = NULL;
	ui_common_place_cursor(UI_CURSOR_DEFAULT, GTK_WIDGET(canvas));
      }
    }
  }
  

  if (extended_object != NULL)
    object = extended_object;

  if (canvas->undrawn_rois != NULL) /* check if there are any undrawn roi's */
    object = canvas->undrawn_rois->data;
  
  if (object == NULL) /* no undrawn roi's, see if we're on an object */
    object = g_object_get_data(G_OBJECT(widget), "object");
  
  if (object == NULL) 
    object = canvas->active_object; /* try the active object */

  g_return_val_if_fail(object != NULL, FALSE);


  switch(event->type) {

  case GDK_ENTER_NOTIFY:
    event_cpoint.x = event->crossing.x;
    event_cpoint.y = event->crossing.y;

    if (event->crossing.mode != GDK_CROSSING_NORMAL) 
      canvas_event_type = CANVAS_EVENT_NONE; /* ignore grabs */

    else if (enter_drawing_mode) {
      canvas_event_type = CANVAS_EVENT_PRESS_ENTER_DRAWING_MODE;

    } else if ((canvas->undrawn_rois == NULL) && (extended_event_type != CANVAS_EVENT_NONE)) 
      canvas_event_type = CANVAS_EVENT_NONE;

    else 
      canvas_event_type = CANVAS_EVENT_ENTER_OBJECT;

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

    if (enter_drawing_mode) {
      canvas_event_type = CANVAS_EVENT_PRESS_ENTER_DRAWING_MODE;

    } else if (canvas->undrawn_rois != NULL) {
      canvas_event_type = CANVAS_EVENT_PRESS_NEW_ROI;

    } else if (in_drawing_mode) {
      if ((event->button.button == 1) && (event->button.state & GDK_SHIFT_MASK))
	canvas_event_type = CANVAS_EVENT_PRESS_DRAW_LARGE_POINT;
      else if (event->button.button == 1)
	canvas_event_type = CANVAS_EVENT_PRESS_DRAW_POINT;
      else if (event->button.button == 2)
	canvas_event_type = CANVAS_EVENT_PRESS_LEAVE_DRAWING_MODE;
      else if ((event->button.button == 3) && (event->button.state & GDK_SHIFT_MASK))
      	canvas_event_type = CANVAS_EVENT_PRESS_ERASE_LARGE_POINT;
      else if (event->button.button == 3)
      	canvas_event_type = CANVAS_EVENT_PRESS_ERASE_POINT;
      else
	canvas_event_type = CANVAS_EVENT_NONE;

    } else if (extended_event_type != CANVAS_EVENT_NONE) {
      canvas_event_type = extended_event_type;
      
    } else if ((!grab_on) && (!in_object) && (AMITK_IS_DATA_SET(object) || AMITK_IS_STUDY(object)) && (event->button.button == 1)) {
      if ((event->button.state & GDK_SHIFT_MASK) && (AMITK_IS_DATA_SET(object)))
	canvas_event_type = CANVAS_EVENT_PRESS_SHIFT_OBJECT;
      else 
	canvas_event_type = CANVAS_EVENT_PRESS_MOVE_VIEW;
      
    } else if ((!grab_on) && (!in_object) && (AMITK_IS_DATA_SET(object) || AMITK_IS_STUDY(object)) && (event->button.button == 2)) {
      canvas_event_type = CANVAS_EVENT_PRESS_MINIMIZE_VIEW;
      
    } else if ((!grab_on) && (!in_object) && (AMITK_IS_DATA_SET(object) || AMITK_IS_STUDY(object)) && (event->button.button == 3)) {
      if (event->button.state & GDK_CONTROL_MASK)
	canvas_event_type = CANVAS_EVENT_PRESS_NEW_OBJECT;
      else if (event->button.state & GDK_SHIFT_MASK)
	canvas_event_type = CANVAS_EVENT_PRESS_ROTATE_OBJECT;
      else
	canvas_event_type = CANVAS_EVENT_PRESS_RESIZE_VIEW;
      
    } else if ((!grab_on) && (AMITK_IS_ROI(object)) && (event->button.button == 1)) {
      canvas_event_type = CANVAS_EVENT_PRESS_SHIFT_OBJECT_IMMEDIATE;
      
    } else if ((!grab_on) && (AMITK_IS_ROI(object)) && (event->button.button == 2)) {
      if (!AMITK_ROI_TYPE_ISOCONTOUR(object) && !AMITK_ROI_TYPE_FREEHAND(object))
	canvas_event_type = CANVAS_EVENT_PRESS_RESIZE_ROI;
      else 
	canvas_event_type = CANVAS_EVENT_PRESS_ENTER_DRAWING_MODE;

    } else if ((!grab_on) && (AMITK_IS_ROI(object)) && (event->button.button == 3)) {
      if (event->button.state & GDK_SHIFT_MASK)
	canvas_event_type = CANVAS_EVENT_PRESS_ERASE_VOLUME_OUTSIDE_ROI;
      else if (event->button.state & GDK_CONTROL_MASK)
	canvas_event_type = CANVAS_EVENT_PRESS_ERASE_VOLUME_INSIDE_ROI;
      else if (!AMITK_ROI_TYPE_ISOCONTOUR(object) && !AMITK_ROI_TYPE_FREEHAND(object))
	canvas_event_type = CANVAS_EVENT_PRESS_ROTATE_OBJECT_IMMEDIATE;

      else 
	canvas_event_type = CANVAS_EVENT_PRESS_CHANGE_ISOCONTOUR;
      
    } else if ((!grab_on) && (AMITK_IS_FIDUCIAL_MARK(object)) && (event->button.button == 1)) {
      canvas_event_type = CANVAS_EVENT_PRESS_SHIFT_OBJECT_IMMEDIATE;

    } else if ((!grab_on) && (AMITK_IS_LINE_PROFILE(object)) && (event->button.button == 1)) {
      canvas_event_type = CANVAS_EVENT_PRESS_SHIFT_OBJECT_IMMEDIATE;

    } else if ((!grab_on) && (AMITK_IS_LINE_PROFILE(object)) && (event->button.button == 3)) {
      canvas_event_type = CANVAS_EVENT_PRESS_ROTATE_OBJECT_IMMEDIATE;

    } else 
      canvas_event_type = CANVAS_EVENT_NONE;
    break;
    
  case GDK_MOTION_NOTIFY:
    event_cpoint.x = event->motion.x;
    event_cpoint.y = event->motion.y;

    if (grab_on && (canvas->undrawn_rois != NULL))  {
      canvas_event_type = CANVAS_EVENT_MOTION_NEW_ROI;
      
    } else if (enter_drawing_mode) {
      canvas_event_type = CANVAS_EVENT_PRESS_ENTER_DRAWING_MODE;

    } else if (in_drawing_mode) {
      if ((event->motion.state & GDK_BUTTON1_MASK) && (event->motion.state & GDK_SHIFT_MASK))
	canvas_event_type = CANVAS_EVENT_MOTION_DRAW_LARGE_POINT;
      else if (event->motion.state & GDK_BUTTON1_MASK)
	canvas_event_type = CANVAS_EVENT_MOTION_DRAW_POINT;
      else if ((event->motion.state & GDK_BUTTON3_MASK) && (event->motion.state & GDK_SHIFT_MASK))
      	canvas_event_type = CANVAS_EVENT_MOTION_ERASE_LARGE_POINT;
      else if (event->motion.state & GDK_BUTTON3_MASK)
	canvas_event_type = CANVAS_EVENT_MOTION_ERASE_POINT;
      else
	canvas_event_type = CANVAS_EVENT_MOTION;

    } else if (extended_event_type != CANVAS_EVENT_NONE) {

      if (extended_event_type == CANVAS_EVENT_PRESS_ROTATE_OBJECT)
	canvas_event_type = CANVAS_EVENT_MOTION_ROTATE_OBJECT;
      else if (extended_event_type == CANVAS_EVENT_PRESS_SHIFT_OBJECT)
	canvas_event_type = CANVAS_EVENT_MOTION_SHIFT_OBJECT;
      else if (extended_event_type == CANVAS_EVENT_PRESS_CHANGE_ISOCONTOUR)
	canvas_event_type = CANVAS_EVENT_MOTION_CHANGE_ISOCONTOUR;
      else {
	canvas_event_type = CANVAS_EVENT_NONE;
	g_warning("unexpected case in %s at line %d",  __FILE__, __LINE__);
      }

    } else if (grab_on && (!in_object) && (AMITK_IS_DATA_SET(object) || AMITK_IS_STUDY(object)) && (event->motion.state & GDK_BUTTON1_MASK)) {
      canvas_event_type = CANVAS_EVENT_MOTION_MOVE_VIEW;
      
    } else if (grab_on && (!in_object) && (AMITK_IS_DATA_SET(object) || AMITK_IS_STUDY(object)) && (event->motion.state & GDK_BUTTON2_MASK)) {
      canvas_event_type = CANVAS_EVENT_MOTION_MINIMIZE_VIEW;
      
    } else if (grab_on && (!in_object) && (AMITK_IS_DATA_SET(object) || AMITK_IS_STUDY(object)) && (event->motion.state & GDK_BUTTON3_MASK)) {
      canvas_event_type = CANVAS_EVENT_MOTION_RESIZE_VIEW;
      
    } else if (grab_on && (AMITK_IS_ROI(object)) && (event->motion.state & GDK_BUTTON1_MASK)) {
      canvas_event_type = CANVAS_EVENT_MOTION_SHIFT_OBJECT;
      
    } else if (grab_on && (AMITK_IS_ROI(object)) && (event->motion.state & GDK_BUTTON2_MASK)) {
      if (!AMITK_ROI_TYPE_ISOCONTOUR(object) && !AMITK_ROI_TYPE_FREEHAND(object))
	canvas_event_type = CANVAS_EVENT_MOTION_RESIZE_ROI;
      else { 
	canvas_event_type = CANVAS_EVENT_NONE;
	g_warning("unexpected case in %s at line %d",  __FILE__, __LINE__);
      }

    } else if (grab_on && (AMITK_IS_ROI(object)) && (event->motion.state & GDK_BUTTON3_MASK)) {
      if (!AMITK_ROI_TYPE_ISOCONTOUR(object) && !AMITK_ROI_TYPE_FREEHAND(object))
	canvas_event_type = CANVAS_EVENT_MOTION_ROTATE_OBJECT;
      else canvas_event_type = CANVAS_EVENT_MOTION_CHANGE_ISOCONTOUR;

    } else if (grab_on && (AMITK_IS_FIDUCIAL_MARK(object)) && (event->motion.state & GDK_BUTTON1_MASK)) {
      canvas_event_type = CANVAS_EVENT_MOTION_SHIFT_OBJECT;

    } else if (grab_on && (AMITK_IS_LINE_PROFILE(object)) && (event->motion.state & GDK_BUTTON1_MASK)) {
      canvas_event_type = CANVAS_EVENT_MOTION_SHIFT_OBJECT;

    } else if (grab_on && (AMITK_IS_LINE_PROFILE(object)) && (event->motion.state & GDK_BUTTON3_MASK)) {
      canvas_event_type = CANVAS_EVENT_MOTION_ROTATE_OBJECT;

    } else 
      canvas_event_type = CANVAS_EVENT_MOTION;

    break;
    
  case GDK_BUTTON_RELEASE:
    event_cpoint.x = event->button.x;
    event_cpoint.y = event->button.y;

    if (canvas->undrawn_rois != NULL) {
      canvas_event_type = CANVAS_EVENT_RELEASE_NEW_ROI;
	
    } else if (in_drawing_mode) {


      if ((event->button.button == 1) && (event->button.state & GDK_SHIFT_MASK))
	canvas_event_type = CANVAS_EVENT_RELEASE_DRAW_LARGE_POINT;
      else if (event->button.button == 1)
	canvas_event_type = CANVAS_EVENT_RELEASE_DRAW_POINT;
      else if ((event->button.button == 3) && (event->button.state & GDK_SHIFT_MASK))
      	canvas_event_type = CANVAS_EVENT_RELEASE_ERASE_LARGE_POINT;
      else if (event->button.button == 3)
      	canvas_event_type = CANVAS_EVENT_RELEASE_ERASE_POINT;
      else
	canvas_event_type = CANVAS_EVENT_NONE;

    } else if ((extended_event_type != CANVAS_EVENT_NONE) && (!grab_on) && (event->button.button == 3)) {

      if (extended_event_type == CANVAS_EVENT_PRESS_ROTATE_OBJECT)
	canvas_event_type = CANVAS_EVENT_ENACT_ROTATE_OBJECT;
      else if (extended_event_type == CANVAS_EVENT_PRESS_SHIFT_OBJECT)
	canvas_event_type = CANVAS_EVENT_ENACT_SHIFT_OBJECT;
      else if (extended_event_type == CANVAS_EVENT_PRESS_CHANGE_ISOCONTOUR)
	canvas_event_type = CANVAS_EVENT_ENACT_CHANGE_ISOCONTOUR;
      else {
	canvas_event_type = CANVAS_EVENT_NONE;
	g_warning("unexpected case in %s at line %d",  __FILE__, __LINE__);
      }

    } else if ((extended_event_type != CANVAS_EVENT_NONE) && (!grab_on))  {

      if (extended_event_type == CANVAS_EVENT_PRESS_ROTATE_OBJECT)
	canvas_event_type = CANVAS_EVENT_CANCEL_ROTATE_OBJECT;
      else if (extended_event_type == CANVAS_EVENT_PRESS_SHIFT_OBJECT)
	canvas_event_type = CANVAS_EVENT_CANCEL_SHIFT_OBJECT;
      else if (extended_event_type == CANVAS_EVENT_PRESS_CHANGE_ISOCONTOUR)
	canvas_event_type = CANVAS_EVENT_CANCEL_CHANGE_ISOCONTOUR;
      else {
	canvas_event_type = CANVAS_EVENT_NONE;
	g_warning("unexpected case in %s at line %d",  __FILE__, __LINE__);
      }

    } else if ((extended_event_type != CANVAS_EVENT_NONE) && (grab_on)) {
      if (extended_event_type == CANVAS_EVENT_PRESS_ROTATE_OBJECT)
	canvas_event_type = CANVAS_EVENT_RELEASE_ROTATE_OBJECT;
      else if (extended_event_type == CANVAS_EVENT_PRESS_SHIFT_OBJECT)
	canvas_event_type = CANVAS_EVENT_RELEASE_SHIFT_OBJECT;
      else if (extended_event_type == CANVAS_EVENT_PRESS_CHANGE_ISOCONTOUR)
	canvas_event_type = CANVAS_EVENT_RELEASE_CHANGE_ISOCONTOUR;
      else {
	canvas_event_type = CANVAS_EVENT_NONE;
	g_warning("unexpected case in %s at line %d",  __FILE__, __LINE__);
      }

    } else if ((!in_object) && (AMITK_IS_DATA_SET(object) || AMITK_IS_STUDY(object)) && (event->button.button == 1)) {
      canvas_event_type = CANVAS_EVENT_RELEASE_MOVE_VIEW;
	
    } else if ((!in_object) && (AMITK_IS_DATA_SET(object) || AMITK_IS_STUDY(object)) && (event->button.button == 2)) {
      canvas_event_type = CANVAS_EVENT_RELEASE_MINIMIZE_VIEW;
      
    } else if ((!in_object) && (AMITK_IS_DATA_SET(object) || AMITK_IS_STUDY(object)) && (event->button.button == 3)) {
      canvas_event_type = CANVAS_EVENT_RELEASE_RESIZE_VIEW;
      
    } else if ((AMITK_IS_ROI(object)) && (event->button.button == 1)) {
      canvas_event_type = CANVAS_EVENT_RELEASE_SHIFT_OBJECT_IMMEDIATE;
      
    } else if (grab_on && (AMITK_IS_ROI(object)) && (event->button.button == 2)) {
      if (!AMITK_ROI_TYPE_ISOCONTOUR(object) && !AMITK_ROI_TYPE_FREEHAND(object))
	canvas_event_type = CANVAS_EVENT_RELEASE_RESIZE_ROI; 
      else {
	canvas_event_type = CANVAS_EVENT_NONE;
	g_warning("unexpected case in %s at line %d",  __FILE__, __LINE__);
      }

    } else if ((AMITK_IS_ROI(object)) && (event->button.button == 3)) {
      if (!AMITK_ROI_TYPE_ISOCONTOUR(object) && !AMITK_ROI_TYPE_FREEHAND(object))
	canvas_event_type = CANVAS_EVENT_RELEASE_ROTATE_OBJECT_IMMEDIATE;
      else canvas_event_type = CANVAS_EVENT_RELEASE_CHANGE_ISOCONTOUR;

    } else if ((AMITK_IS_FIDUCIAL_MARK(object)) && (event->button.button == 1)) {      
      canvas_event_type = CANVAS_EVENT_RELEASE_SHIFT_OBJECT_IMMEDIATE;

    } else if ((AMITK_IS_LINE_PROFILE(object)) && (event->button.button == 1)) {      
      canvas_event_type = CANVAS_EVENT_RELEASE_SHIFT_OBJECT_IMMEDIATE;

    } else if ((AMITK_IS_LINE_PROFILE(object)) && (event->button.button == 3)) {      
      canvas_event_type = CANVAS_EVENT_RELEASE_ROTATE_OBJECT_IMMEDIATE;

    } else 
      canvas_event_type = CANVAS_EVENT_NONE;
    
    break;

  case GDK_SCROLL: /* scroll wheel event */
    event_cpoint.x = event_cpoint.y = 0;
    if (event->scroll.direction == GDK_SCROLL_UP)
      canvas_event_type = CANVAS_EVENT_SCROLL_UP;
    else if (event->scroll.direction == GDK_SCROLL_DOWN)
      canvas_event_type = CANVAS_EVENT_SCROLL_DOWN;
    else
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
  //    g_print("%s event %d grab %d gdk %d in_object %d\n", AMITK_IS_OBJECT(object) ? AMITK_OBJECT_NAME(object) : "line_profile", 
  //	    canvas_event_type, grab_on, event->type, in_object);

  /* get the location of the event, and convert it to the canvas coordinates */
  gnome_canvas_window_to_world(GNOME_CANVAS(canvas->canvas), event_cpoint.x, event_cpoint.y, &canvas_cpoint.x, &canvas_cpoint.y);
  gnome_canvas_w2c_d(GNOME_CANVAS(canvas->canvas), canvas_cpoint.x, canvas_cpoint.y, &canvas_cpoint.x, &canvas_cpoint.y);

  /* Convert the event location info to real units */
  canvas_point = cp_2_p(canvas, canvas_cpoint);
  base_point = amitk_space_s2b(AMITK_SPACE(canvas->volume), canvas_point);

  /* get the current location's value */
  g_return_val_if_fail(canvas->slices != NULL, FALSE);
  active_slice = NULL;
  if (AMITK_IS_DATA_SET(canvas->active_object)) 
    active_slice = amitk_data_sets_find_with_slice_parent(canvas->slices,  AMITK_DATA_SET(canvas->active_object));

  if (active_slice != NULL) {
    temp_point[0] = amitk_space_b2s(AMITK_SPACE(active_slice), base_point);
    POINT_TO_VOXEL(temp_point[0], AMITK_DATA_SET_VOXEL_SIZE(active_slice), 0,0,temp_voxel);
    voxel_value = amitk_data_set_get_value(active_slice, temp_voxel);
  } else {
    voxel_value = NAN;
  }
  
  switch (canvas_event_type) {
    
  case CANVAS_EVENT_ENTER_OBJECT:

    if (in_drawing_mode) {
      help_info = AMITK_HELP_INFO_CANVAS_DRAWING_MODE;
      cursor_type = UI_CURSOR_ROI_DRAW;
    } else if (AMITK_IS_DATA_SET(object)) {
      help_info = AMITK_HELP_INFO_CANVAS_DATA_SET;
      cursor_type =UI_CURSOR_DATA_SET_MODE;
      
    } else if (AMITK_IS_STUDY(object)) {
      help_info = AMITK_HELP_INFO_CANVAS_STUDY;
      cursor_type = UI_CURSOR_DATA_SET_MODE;
      
    } else if (AMITK_IS_FIDUCIAL_MARK(object)) {
      in_object = TRUE;
      help_info = AMITK_HELP_INFO_CANVAS_FIDUCIAL_MARK;
      cursor_type =UI_CURSOR_FIDUCIAL_MARK_MODE;
      
    } else if (AMITK_IS_ROI(object)) { 
      in_object = TRUE;
      cursor_type = UI_CURSOR_ROI_MODE;
      
      if (AMITK_ROI_TYPE_ISOCONTOUR(object)) {
	if (AMITK_ROI_UNDRAWN(object))
	  help_info = AMITK_HELP_INFO_CANVAS_NEW_ISOCONTOUR_ROI;
	else {
	  help_info = AMITK_HELP_INFO_CANVAS_ISOCONTOUR_ROI;
	}
	
      } else if (AMITK_ROI_TYPE_FREEHAND(object)) {
	if (AMITK_ROI_UNDRAWN(object))
	  help_info = AMITK_HELP_INFO_CANVAS_NEW_FREEHAND_ROI;
	else {
	  help_info = AMITK_HELP_INFO_CANVAS_FREEHAND_ROI;
	}
	

      } else { /* geometric ROI's */
	if (AMITK_ROI_UNDRAWN(object))
	  help_info = AMITK_HELP_INFO_CANVAS_NEW_ROI;
	else
	  help_info = AMITK_HELP_INFO_CANVAS_ROI;
      }

    } else if (AMITK_IS_LINE_PROFILE(object)) {
      in_object = TRUE;
      help_info = AMITK_HELP_INFO_CANVAS_LINE_PROFILE;
      cursor_type = UI_CURSOR_FIDUCIAL_MARK_MODE;

    } else {
      g_return_val_if_reached(FALSE);
    }

    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,help_info, &base_point, 0.0);
    if (AMITK_IS_ROI(object))
      if (!AMITK_ROI_UNDRAWN(object) && 
	  (AMITK_ROI_TYPE_ISOCONTOUR(object))) {
	if (AMITK_ROI_ISOCONTOUR_RANGE(object) == AMITK_ROI_ISOCONTOUR_RANGE_ABOVE_MIN)
	  g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
			AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, 
			AMITK_ROI(object)->isocontour_min_value);
	else if (AMITK_ROI_ISOCONTOUR_RANGE(object) == AMITK_ROI_ISOCONTOUR_RANGE_BELOW_MAX)
	  g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
			AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, 
			AMITK_ROI(object)->isocontour_max_value);
	else /* AMITK_ROI_ISOCONTOUR_RANGE_BETWEEN_MIN_MAX */
	  g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
			AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, 
			0.5*AMITK_ROI(object)->isocontour_min_value+
			0.5*AMITK_ROI(object)->isocontour_max_value);
      }


    gtk_widget_grab_focus(GTK_WIDGET(canvas)); /* move the keyboard entry focus into the canvas */
    ui_common_place_cursor(cursor_type, GTK_WIDGET(canvas));
    break;

    
    
 case CANVAS_EVENT_LEAVE:
   in_object = FALSE;
   ui_common_place_cursor(UI_CURSOR_DEFAULT, GTK_WIDGET(canvas));
   break;

  case CANVAS_EVENT_PRESS_MINIMIZE_VIEW:
  case CANVAS_EVENT_PRESS_RESIZE_VIEW:
  case CANVAS_EVENT_PRESS_MOVE_VIEW:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_UPDATE_LOCATION, &base_point,voxel_value);
    if (canvas_event_type == CANVAS_EVENT_PRESS_MINIMIZE_VIEW) 
      corner = amitk_data_sets_get_min_voxel_size(AMITK_OBJECT_CHILDREN(canvas->study));
    else
      corner = AMITK_VOLUME_Z_CORNER(canvas->volume);
    grab_on = TRUE;
    initial_base_point = base_point;
    canvas_update_target(canvas, AMITK_CANVAS_TARGET_ACTION_SHOW, base_point, corner);
    /* grabbing the target doesn't quite work, double click events aren't handled correctly */
    gnome_canvas_item_grab(canvas->image, GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK |GDK_BUTTON_PRESS_MASK ,
    			   ui_common_cursor[UI_CURSOR_DATA_SET_MODE], event->button.time);
    g_signal_emit(G_OBJECT (canvas), canvas_signals[VIEW_CHANGING], 0, &base_point, corner);
    break;

    
  case CANVAS_EVENT_PRESS_SHIFT_OBJECT:
  case CANVAS_EVENT_PRESS_ROTATE_OBJECT:
    if (extended_event_type != CANVAS_EVENT_NONE) {
      grab_on = FALSE; /* do everything on the BUTTON_RELEASE */
    } else {
      GdkPixbuf * pixbuf;

      if (AMITK_IS_DATA_SET(object)) {
	if (active_slice == NULL) {
	  g_warning(_("The active data set is not visible"));
	  return FALSE;
	}
	pixbuf = image_from_slice(active_slice, AMITK_CANVAS_VIEW_MODE(canvas));
      } else
	pixbuf = g_object_ref(canvas->pixbuf);

      grab_on = TRUE;
      extended_event_type = canvas_event_type;
      extended_object = object;
      initial_cpoint = canvas_cpoint;
      initial_base_point = base_point;
      initial_canvas_point = canvas_point;
      previous_cpoint = canvas_cpoint;

      canvas_item = gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
					  gnome_canvas_pixbuf_get_type(),
					  "pixbuf",pixbuf,
					  "x", (double) canvas->border_width,
					  "y", (double) canvas->border_width,
					  NULL);
      g_signal_connect(G_OBJECT(canvas_item), "event", G_CALLBACK(canvas_event_cb), canvas);
      g_object_unref(pixbuf);
    }

    if (canvas_event_type == CANVAS_EVENT_PRESS_SHIFT_OBJECT) {
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_CANVAS_SHIFT_OBJECT, &base_point, 0.0);
    } else { /* ROTATE */
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_CANVAS_ROTATE_OBJECT, &base_point, 0.0);
    }
    break;


  case CANVAS_EVENT_PRESS_NEW_ROI:
    outline_color = amitk_color_table_outline_color(canvas_get_color_table(canvas), TRUE);
    initial_canvas_point = canvas_point;
    initial_cpoint = canvas_cpoint;
    
    /* create the new roi */
    switch(AMITK_ROI_TYPE(object)) {
    case AMITK_ROI_TYPE_BOX:
    case AMITK_ROI_TYPE_ELLIPSOID:
    case AMITK_ROI_TYPE_CYLINDER:
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_CANVAS_NEW_ROI, &base_point, 0.0);
      canvas_item = 
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
			      (AMITK_ROI_TYPE(object) == AMITK_ROI_TYPE_BOX) ?
			      gnome_canvas_rect_get_type() : gnome_canvas_ellipse_get_type(),
			      "x1",canvas_cpoint.x, "y1", canvas_cpoint.y, 
			      "x2", canvas_cpoint.x, "y2", canvas_cpoint.y,
			      "fill_color", NULL,
			      "outline_color_rgba", amitk_color_table_rgba_to_uint32(outline_color),
			      "width_pixels", AMITK_STUDY_CANVAS_ROI_WIDTH(canvas->study),
			      NULL);
      g_signal_connect(G_OBJECT(canvas_item), "event", G_CALLBACK(canvas_event_cb), canvas);
      grab_on = TRUE;
      gnome_canvas_item_grab(canvas_item,
			     GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			     ui_common_cursor[UI_CURSOR_ROI_MODE],
			     event->button.time);
      break;
    case AMITK_ROI_TYPE_ISOCONTOUR_2D:
    case AMITK_ROI_TYPE_ISOCONTOUR_3D:
      
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_CANVAS_NEW_ISOCONTOUR_ROI, &base_point, 0.0);
      grab_on = TRUE;
      break;
    case AMITK_ROI_TYPE_FREEHAND_2D:
    case AMITK_ROI_TYPE_FREEHAND_3D:
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_CANVAS_NEW_FREEHAND_ROI, &base_point, 0.0);
      break;
    default:
      grab_on = FALSE;
      g_error("unexpected case in %s at line %d, roi_type %d", 
	      __FILE__, __LINE__, AMITK_ROI_TYPE(object));
      break;
    }

    break;

  case CANVAS_EVENT_PRESS_SHIFT_OBJECT_IMMEDIATE:
    previous_cpoint = canvas_cpoint;
  case CANVAS_EVENT_PRESS_ROTATE_OBJECT_IMMEDIATE:
  case CANVAS_EVENT_PRESS_RESIZE_ROI:

    if (AMITK_IS_LINE_PROFILE(object)) 
      help_info = AMITK_HELP_INFO_CANVAS_LINE_PROFILE;
    else if (AMITK_IS_FIDUCIAL_MARK(object)) 
      help_info = AMITK_HELP_INFO_CANVAS_FIDUCIAL_MARK;
    else if (AMITK_ROI_TYPE_ISOCONTOUR(object)) 
      help_info = AMITK_HELP_INFO_CANVAS_ISOCONTOUR_ROI;
    else if (AMITK_ROI_TYPE_FREEHAND(object))
      help_info = AMITK_HELP_INFO_CANVAS_FREEHAND_ROI;
    else /* normal roi */
      help_info = AMITK_HELP_INFO_CANVAS_ROI;

    if (canvas_event_type == CANVAS_EVENT_PRESS_SHIFT_OBJECT_IMMEDIATE)
      cursor_type = UI_CURSOR_OBJECT_SHIFT;
    else if (canvas_event_type == CANVAS_EVENT_PRESS_ROTATE_OBJECT_IMMEDIATE)
      cursor_type = UI_CURSOR_ROI_ROTATE;
    else /* CANVAS_EVENT_PRESS_RESIZE_ROI */
      cursor_type = UI_CURSOR_ROI_RESIZE;

    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0, help_info, &base_point, 0.0);
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, voxel_value);

    grab_on = TRUE;
    canvas_item = GNOME_CANVAS_ITEM(widget); 
    gnome_canvas_item_grab(canvas_item, GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK, 
			   ui_common_cursor[cursor_type], event->button.time);

    initial_base_point = base_point;
    initial_cpoint = canvas_cpoint;
    initial_canvas_point = canvas_point;
    theta = 0.0;
    zoom.x = zoom.y = zoom.z = 1.0;
    break;

  case CANVAS_EVENT_PRESS_ENTER_DRAWING_MODE:
    in_drawing_mode = TRUE;
    enter_drawing_mode = FALSE;
    if (drawing_object == NULL)
      drawing_object = object;
    ui_common_place_cursor(UI_CURSOR_ROI_DRAW, GTK_WIDGET(canvas));
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_DRAWING_MODE, &base_point, voxel_value);
    break;

  case CANVAS_EVENT_PRESS_LEAVE_DRAWING_MODE:
    in_drawing_mode = FALSE;
    drawing_object = NULL;
    ui_common_place_cursor(UI_CURSOR_DEFAULT, GTK_WIDGET(canvas));
    ignore_next_event=TRUE;
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, voxel_value);
    break;

  case CANVAS_EVENT_PRESS_CHANGE_ISOCONTOUR:
    if (extended_event_type != CANVAS_EVENT_NONE) {
      grab_on = FALSE; /* do everything on the BUTTON_RELEASE */
    } else {
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, voxel_value);
      grab_on = TRUE;
      extended_event_type = canvas_event_type;
      extended_object = object;
    }
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_CHANGE_ISOCONTOUR, &base_point, 0.0);
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
    if (!in_drawing_mode)
      if (AMITK_IS_ROI(object))
	if (!AMITK_ROI_UNDRAWN(object))
	  if (AMITK_ROI_TYPE_ISOCONTOUR(object)) {
	    if (AMITK_ROI_ISOCONTOUR_RANGE(object) == AMITK_ROI_ISOCONTOUR_RANGE_ABOVE_MIN)
	      voxel_value = AMITK_ROI(object)->isocontour_min_value;
	    else if (AMITK_ROI_ISOCONTOUR_RANGE(object) == AMITK_ROI_ISOCONTOUR_RANGE_BELOW_MAX)
	      voxel_value = AMITK_ROI(object)->isocontour_max_value;
	    else /* AMITK_ROI_ISOCONTOUR_RANGE_BETWEEN_MIN_MAX */
	      voxel_value = 0.5*(AMITK_ROI(object)->isocontour_min_value+AMITK_ROI(object)->isocontour_max_value);
	  }
	  
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
      corner = amitk_data_sets_get_min_voxel_size(AMITK_OBJECT_CHILDREN(canvas->study));
      center = base_point;
    }

    canvas_update_target(canvas, AMITK_CANVAS_TARGET_ACTION_SHOW, center, corner);
    g_signal_emit(G_OBJECT (canvas), canvas_signals[VIEW_CHANGING], 0,
		  &center, corner);

    break;


  case CANVAS_EVENT_PRESS_DRAW_POINT:
  case CANVAS_EVENT_PRESS_DRAW_LARGE_POINT:
  case CANVAS_EVENT_PRESS_ERASE_POINT:
  case CANVAS_EVENT_PRESS_ERASE_LARGE_POINT:
    grab_on = TRUE;
    canvas_item = GNOME_CANVAS_ITEM(widget);
    gnome_canvas_item_grab(canvas_item,
    			   GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
    			   ui_common_cursor[UI_CURSOR_ROI_DRAW], event->button.time);
    /* and fall through */
  case CANVAS_EVENT_MOTION_DRAW_POINT:
  case CANVAS_EVENT_MOTION_DRAW_LARGE_POINT:
  case CANVAS_EVENT_MOTION_ERASE_POINT:
  case CANVAS_EVENT_MOTION_ERASE_LARGE_POINT:

    temp_point[0] = amitk_space_b2s(AMITK_SPACE(drawing_object), base_point);
    POINT_TO_VOXEL(temp_point[0], AMITK_ROI_VOXEL_SIZE(drawing_object), 0, 0, temp_voxel);

    amitk_roi_manipulate_area(AMITK_ROI(drawing_object),
			      ((canvas_event_type != CANVAS_EVENT_MOTION_DRAW_POINT) &&
			       (canvas_event_type != CANVAS_EVENT_PRESS_DRAW_POINT) &&
			       (canvas_event_type != CANVAS_EVENT_MOTION_DRAW_LARGE_POINT) &&
			       (canvas_event_type != CANVAS_EVENT_PRESS_DRAW_LARGE_POINT)),
			      temp_voxel,
			      ((canvas_event_type == CANVAS_EVENT_MOTION_DRAW_LARGE_POINT) || 
			       (canvas_event_type == CANVAS_EVENT_PRESS_DRAW_LARGE_POINT) || 
			       (canvas_event_type == CANVAS_EVENT_MOTION_ERASE_LARGE_POINT) || 
			       (canvas_event_type == CANVAS_EVENT_PRESS_ERASE_LARGE_POINT)) ? 2 : 0);
    
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
    		  AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, voxel_value);
    break;

  case CANVAS_EVENT_MOTION_CHANGE_ISOCONTOUR:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, voxel_value);
    break;

  case CANVAS_EVENT_MOTION_NEW_ROI:
    switch(AMITK_ROI_TYPE(object)) {
    case AMITK_ROI_TYPE_BOX:
    case AMITK_ROI_TYPE_ELLIPSOID:
    case AMITK_ROI_TYPE_CYLINDER:
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_CANVAS_NEW_ROI, &base_point, 0.0);
      if (event->motion.state & GDK_BUTTON1_MASK) { /* edge-to-edge button 1 */
	temp_cpoint[0] = initial_cpoint;
	temp_cpoint[1] = canvas_cpoint;
      } else { /* center-out other buttons*/
	diff_cpoint = canvas_point_diff(initial_cpoint, canvas_cpoint);
	temp_cpoint[0] = canvas_point_sub(initial_cpoint, diff_cpoint);
	temp_cpoint[1] = canvas_point_add(initial_cpoint, diff_cpoint);
      }
      gnome_canvas_item_set(canvas_item, "x1", temp_cpoint[0].x, "y1", temp_cpoint[0].y,
			    "x2", temp_cpoint[1].x, "y2", temp_cpoint[1].y,NULL);
      break;
    case AMITK_ROI_TYPE_ISOCONTOUR_2D:
    case AMITK_ROI_TYPE_ISOCONTOUR_3D:
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_CANVAS_NEW_ISOCONTOUR_ROI, &base_point, 0.0);
      break;
    case AMITK_ROI_TYPE_FREEHAND_2D:
    case AMITK_ROI_TYPE_FREEHAND_3D:
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_CANVAS_NEW_FREEHAND_ROI, &base_point, 0.0);
      break;
    default:
      g_error("unexpected case in %s at line %d, roi_type %d", 
	      __FILE__, __LINE__, AMITK_ROI_TYPE(object));
      break;
    }
    break;

  case CANVAS_EVENT_MOTION_SHIFT_OBJECT:
    if (AMITK_IS_DATA_SET(object)) {
      diff_point = point_sub(base_point, initial_base_point);
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_UPDATE_SHIFT, &diff_point, theta);
    } else
      g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		    AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, voxel_value);

    diff_cpoint = canvas_point_sub(canvas_cpoint, previous_cpoint);
    gnome_canvas_item_i2w(canvas_item->parent, &diff_cpoint.x, &diff_cpoint.y);
    gnome_canvas_item_move(canvas_item,diff_cpoint.x,diff_cpoint.y); 
    previous_cpoint = canvas_cpoint;
    break;


  case CANVAS_EVENT_MOTION_ROTATE_OBJECT:
    {
      /* rotate button Pressed, we're rotating the object */
      /* note, I'd like to use the function "gnome_canvas_item_rotate"
	 but this isn't defined in the current version of gnome.... 
	 so I'll have to do a whole bunch of shit*/
      double affine[6];
      AmitkCanvasPoint item_center;
      AmitkPoint center;
      
      if (AMITK_IS_VOLUME(object))
	center = amitk_volume_get_center(AMITK_VOLUME(object));
      else /* STUDY */
	center = canvas->center;
      center = amitk_space_b2s(AMITK_SPACE(canvas->volume), center);
				 
      temp_point[0] = point_sub(initial_canvas_point,center);
      temp_point[1] = point_sub(canvas_point,center);
      
      theta = acos(point_dot_product(temp_point[0],temp_point[1])/(point_mag(temp_point[0]) * point_mag(temp_point[1])));

      /* correct for the fact that acos is always positive by using the cross product */
      if ((temp_point[0].x*temp_point[1].y-temp_point[0].y*temp_point[1].x) < 0.0) theta = -theta;

      /* figure out what the center of the roi is in canvas_item coords */
      /* compensate for x's origin being top left (ours is bottom left) */
      item_center = p_2_cp(canvas, center);
      affine[0] = cos(-theta); /* neg cause GDK has Y axis going down, not up */
      affine[1] = sin(-theta);
      affine[2] = -affine[1];
      affine[3] = affine[0];
      affine[4] = (1.0-affine[0])*item_center.x+affine[1]*item_center.y;
      affine[5] = (1.0-affine[3])*item_center.y+affine[2]*item_center.x;
      gnome_canvas_item_affine_absolute(canvas_item,affine);
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

      center = amitk_space_b2s(AMITK_SPACE(canvas->volume),amitk_volume_get_center(AMITK_VOLUME(object)));
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
				       canvas->view, 
				       AMITK_STUDY_CANVAS_LAYOUT(canvas->study), 
				       AMITK_AXIS_X);
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
						     canvas->view, 
						     AMITK_STUDY_CANVAS_LAYOUT(canvas->study), 
						     AMITK_AXIS_Y);
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
						       canvas->view, 
						       AMITK_STUDY_CANVAS_LAYOUT(canvas->study), 
						       i_axis);
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
      item_center = p_2_cp(canvas, center);
	
      /* do a wild ass affine matrix so that we can scale while preserving angles */
      affine[0] = canvas_zoom.x * cos_r * cos_r + canvas_zoom.y * sin_r * sin_r;
      affine[1] = (canvas_zoom.x-canvas_zoom.y)* cos_r * sin_r;
      affine[2] = affine[1];
      affine[3] = canvas_zoom.x * sin_r * sin_r + canvas_zoom.y * cos_r * cos_r;
      affine[4] = item_center.x - item_center.x*canvas_zoom.x*cos_r*cos_r 
	- item_center.x*canvas_zoom.y*sin_r*sin_r + (canvas_zoom.y-canvas_zoom.x)*item_center.y*cos_r*sin_r;
      affine[5] = item_center.y - item_center.y*canvas_zoom.y*cos_r*cos_r 
	- item_center.y*canvas_zoom.x*sin_r*sin_r + (canvas_zoom.y-canvas_zoom.x)*item_center.x*cos_r*sin_r;
      gnome_canvas_item_affine_absolute(canvas_item,affine);
    }
    break;

  case CANVAS_EVENT_RELEASE_MOVE_VIEW:
  case CANVAS_EVENT_RELEASE_MINIMIZE_VIEW:
  case CANVAS_EVENT_RELEASE_RESIZE_VIEW:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_UPDATE_LOCATION, &base_point, voxel_value);
    grab_on = FALSE;
    gnome_canvas_item_ungrab(canvas->image, event->button.time);
    /* queue target cross redraw */
    amitk_canvas_update_target(canvas, AMITK_CANVAS_TARGET_ACTION_HIDE, base_point, 0.0);

    if (canvas_event_type == CANVAS_EVENT_RELEASE_RESIZE_VIEW)
      center = initial_base_point;
    else
      center = base_point;

    if (canvas_event_type == CANVAS_EVENT_RELEASE_MINIMIZE_VIEW) 
      corner = amitk_data_sets_get_min_voxel_size(AMITK_OBJECT_CHILDREN(canvas->study));
    else if (canvas_event_type == CANVAS_EVENT_RELEASE_RESIZE_VIEW)
      corner = point_max_dim(point_diff(base_point, initial_base_point));
    else
      corner = AMITK_VOLUME_Z_CORNER(canvas->volume);

    g_signal_emit(G_OBJECT (canvas), canvas_signals[VIEW_CHANGED], 0, &center, corner);
    break;

  case CANVAS_EVENT_RELEASE_SHIFT_OBJECT:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_SHIFT_OBJECT, &base_point, 0.0);
    break;

  case CANVAS_EVENT_RELEASE_ROTATE_OBJECT:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_ROTATE_OBJECT, &base_point, 0.0);
    break;

  case CANVAS_EVENT_CANCEL_SHIFT_OBJECT:
  case CANVAS_EVENT_CANCEL_ROTATE_OBJECT:
    if (AMITK_IS_STUDY(object))
      help_info = AMITK_HELP_INFO_CANVAS_STUDY;
    else /* DATA_SET */
      help_info = AMITK_HELP_INFO_CANVAS_DATA_SET;
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,help_info, &base_point, 0.0);
    extended_event_type = CANVAS_EVENT_NONE;
    extended_object = NULL;
    gtk_object_destroy(GTK_OBJECT(canvas_item));
    break;

  case CANVAS_EVENT_CANCEL_CHANGE_ISOCONTOUR:
    help_info = AMITK_HELP_INFO_CANVAS_ISOCONTOUR_ROI;
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,help_info, &base_point, 0.0);
    extended_event_type = CANVAS_EVENT_NONE;
    extended_object = NULL;
    break;

  case CANVAS_EVENT_ENACT_SHIFT_OBJECT:
  case CANVAS_EVENT_ENACT_ROTATE_OBJECT:
    if (AMITK_IS_STUDY(object))
      help_info = AMITK_HELP_INFO_CANVAS_STUDY;
    else /* DATA_SET */
      help_info = AMITK_HELP_INFO_CANVAS_DATA_SET;
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,help_info, &base_point, 0.0);
    gtk_object_destroy(GTK_OBJECT(canvas_item));
    extended_event_type = CANVAS_EVENT_NONE;
    extended_object = NULL;

    if (canvas_event_type == CANVAS_EVENT_ENACT_SHIFT_OBJECT) { /* shift active data set */
      amitk_space_shift_offset(AMITK_SPACE(canvas->active_object),
			       point_sub(base_point, initial_base_point));
    } else {/* rotate active data set*/
      if (canvas->view == AMITK_VIEW_SAGITTAL) theta = -theta; /* sagittal is left-handed */

      if (AMITK_IS_VOLUME(object))
	center = amitk_volume_get_center(AMITK_VOLUME(canvas->active_object));
      else 
	center = canvas->center;

      amitk_space_rotate_on_vector(AMITK_SPACE(canvas->active_object),
				   amitk_space_get_axis(AMITK_SPACE(canvas->volume), AMITK_AXIS_Z),
				   theta, center);
    }

    break;

  case CANVAS_EVENT_ENACT_CHANGE_ISOCONTOUR:
    help_info = AMITK_HELP_INFO_CANVAS_ISOCONTOUR_ROI;
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,help_info, &base_point, 0.0);
    extended_event_type = CANVAS_EVENT_NONE;
    extended_object = NULL;
    if (active_slice == NULL) {
      g_warning(_("The active data set is not visible"));
      return FALSE;
    }
    canvas_create_isocontour_roi(canvas, AMITK_ROI(object), base_point, active_slice);
    break;

  case CANVAS_EVENT_RELEASE_NEW_ROI:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_DATA_SET, &base_point, 0.0);
    grab_on = FALSE;

    switch(AMITK_ROI_TYPE(object)) {
    case AMITK_ROI_TYPE_CYLINDER:
    case AMITK_ROI_TYPE_BOX:
    case AMITK_ROI_TYPE_ELLIPSOID:  
      gnome_canvas_item_ungrab(canvas_item, event->button.time);

      diff_point = point_diff(initial_canvas_point, canvas_point);
      diff_point.z = canvas_check_z_dimension(canvas, AMITK_VOLUME_Z_CORNER(canvas->volume));
      diff_point.z /= 2;
      gtk_object_destroy(GTK_OBJECT(canvas_item)); /* get rid of the roi drawn on the canvas */
      

      if (event->button.button == 1) { /* edge to edge */
	temp_point[0].x = MIN(initial_canvas_point.x, canvas_point.x);
	temp_point[0].y = MIN(initial_canvas_point.y, canvas_point.y);
	temp_point[0].z = canvas_point.z - diff_point.z;
	temp_point[1].x = MAX(initial_canvas_point.x, canvas_point.x);
	temp_point[1].y = MAX(initial_canvas_point.y, canvas_point.y);
	temp_point[1].z = canvas_point.z + diff_point.z;
      } else { /* center to center */
	temp_point[0] = point_sub(initial_canvas_point, diff_point);
	temp_point[1] = point_add(initial_canvas_point, diff_point);
      }

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
      if (active_slice == NULL) {
	g_warning(_("The active data set is not visible"));
	return FALSE;
      }
      canvas_create_isocontour_roi(canvas, AMITK_ROI(object), base_point, active_slice);
      break;
    case AMITK_ROI_TYPE_FREEHAND_2D:
    case AMITK_ROI_TYPE_FREEHAND_3D:
      if (active_slice == NULL) {
	g_warning(_("The active data set is not visible"));
	return FALSE;
      }
      if (canvas_create_freehand_roi(canvas, AMITK_ROI(object), base_point, active_slice)) {
	enter_drawing_mode = TRUE;
	drawing_object = object;
      }
      break;
    default:
      g_error("unexpected case in %s at line %d, roi_type %d", 
	      __FILE__, __LINE__, AMITK_ROI_TYPE(object));
      break;
    }

    
    canvas_add_object_update(canvas, AMITK_OBJECT(object)); /* add this object for updating */

    break;




  case CANVAS_EVENT_RELEASE_SHIFT_OBJECT_IMMEDIATE:
    gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
    grab_on = FALSE;
    shift = point_sub(base_point, initial_base_point);
    
    /* ------------- apply any shift done -------------- */
    if (!POINT_EQUAL(shift, zero_point)) {
      if (AMITK_IS_FIDUCIAL_MARK(object)) {
	amitk_fiducial_mark_set(AMITK_FIDUCIAL_MARK(object), 
				point_add(shift, AMITK_FIDUCIAL_MARK_GET(object)));
	center = AMITK_FIDUCIAL_MARK_GET(object);
      } else if (AMITK_IS_ROI(object)) {
	amitk_space_shift_offset(AMITK_SPACE(object), shift);
	center = amitk_volume_get_center(AMITK_VOLUME(object));
      } else if (AMITK_IS_LINE_PROFILE(object)) {
	center = point_add(shift, canvas->center);
	/* make sure it stays in bounds */
	center = amitk_volume_place_in_bounds(canvas->volume, center);
      } else
	g_return_val_if_reached(FALSE);
      
      g_signal_emit(G_OBJECT (canvas), canvas_signals[VIEW_CHANGED], 0,
		    &center, AMITK_VOLUME_Z_CORNER(canvas->volume));
    }
    break;


  case CANVAS_EVENT_RELEASE_ROTATE_OBJECT_IMMEDIATE:
    gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
    grab_on = FALSE;
    if (canvas->view == AMITK_VIEW_SAGITTAL) theta = -theta; /* sagittal is a left-handed coord frame */
    
    /* now rotate the roi coordinate space axis */
    if (AMITK_IS_ROI(object)) {
      amitk_space_rotate_on_vector(AMITK_SPACE(object), 
				   amitk_space_get_axis(AMITK_SPACE(canvas->volume), AMITK_AXIS_Z), 
				   theta, amitk_volume_get_center(AMITK_VOLUME(object)));
    } else if (AMITK_IS_LINE_PROFILE(object)) {
      amitk_line_profile_set_angle(AMITK_LINE_PROFILE(object), theta+AMITK_LINE_PROFILE_ANGLE(object));

    } else
      g_return_val_if_reached(FALSE); /* shouldn't get here */
    break;


  case CANVAS_EVENT_RELEASE_RESIZE_ROI:
    gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
    grab_on = FALSE;
    
    radius_point = point_cmult(0.5,AMITK_VOLUME_CORNER(object));
    temp_point[0] = point_mult(zoom, radius_point); /* new radius */
    temp_point[1] = amitk_space_b2s(AMITK_SPACE(object), amitk_volume_get_center(AMITK_VOLUME(object)));
    temp_point[1] = amitk_space_s2b(AMITK_SPACE(object), point_sub(temp_point[1], temp_point[0]));
    amitk_space_set_offset(AMITK_SPACE(object), temp_point[1]);
	  
    /* and the new upper right corner is simply twice the radius */
    amitk_volume_set_corner(AMITK_VOLUME(object), point_cmult(2.0, temp_point[0]));
    break;

  case CANVAS_EVENT_RELEASE_DRAW_POINT:
  case CANVAS_EVENT_RELEASE_DRAW_LARGE_POINT:
  case CANVAS_EVENT_RELEASE_ERASE_POINT:
  case CANVAS_EVENT_RELEASE_ERASE_LARGE_POINT:
    gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
    grab_on = FALSE;
    break;

  case CANVAS_EVENT_RELEASE_CHANGE_ISOCONTOUR:
    g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		  AMITK_HELP_INFO_CANVAS_CHANGE_ISOCONTOUR, &base_point, 0.0);
    break;

  case CANVAS_EVENT_SCROLL_UP:
  case CANVAS_EVENT_SCROLL_DOWN:
    /* just pretend this event is like the user pressed the scrollbar */
    if (canvas_event_type == CANVAS_EVENT_SCROLL_UP)
      g_signal_emit_by_name(G_OBJECT(canvas->scrollbar), "move_slider", GTK_SCROLL_PAGE_BACKWARD, canvas);
    else  /* scroll down */
      g_signal_emit_by_name(G_OBJECT(canvas->scrollbar), "move_slider", GTK_SCROLL_PAGE_FORWARD, canvas);

    break;

  case CANVAS_EVENT_NONE:
    break;
  default:
    g_warning("unexpected case in %s at line %d, event %d", 
	      __FILE__, __LINE__, canvas_event_type);
    break;
  }
  
  return FALSE;
}








/* function called indicating the plane adjustment has changed */
static void canvas_scrollbar_adjustment_cb(GtkObject * adjustment, gpointer data) {

  AmitkCanvas * canvas = data;
  AmitkPoint canvas_center;

  g_return_if_fail(AMITK_IS_CANVAS(canvas));

  canvas_center = amitk_space_b2s(AMITK_SPACE(canvas->volume), canvas->center);
  canvas_center.z = GTK_ADJUSTMENT(adjustment)->value;
  canvas->center = amitk_space_s2b(AMITK_SPACE(canvas->volume), canvas_center);

  canvas_add_update(canvas, UPDATE_ALL);
  g_signal_emit(G_OBJECT (canvas), canvas_signals[VIEW_CHANGED], 0,
		&(canvas->center), AMITK_VOLUME_Z_CORNER(canvas->volume));
  g_signal_emit(G_OBJECT (canvas), canvas_signals[HELP_EVENT], 0,
		AMITK_HELP_INFO_BLANK, &(canvas->center), 0.0);
  return;
}


static gboolean canvas_recalc_corners(AmitkCanvas * canvas) {

  GList * volumes;
  gboolean changed;

  /* sanity checks */
  if (canvas->study == NULL) return FALSE; 

  /* what volumes are we looking at - ignore ROI's */
  if (AMITK_STUDY_CANVAS_MAINTAIN_SIZE(canvas->study)) {
    volumes = amitk_object_get_children_of_type(AMITK_OBJECT(canvas->study), 
						AMITK_OBJECT_TYPE_DATA_SET,
						TRUE);
  } else {
    volumes = amitk_object_get_selected_children_of_type(AMITK_OBJECT(canvas->study), 
							 AMITK_OBJECT_TYPE_DATA_SET,
							 canvas->view_mode,
							 TRUE);
  }

  changed = amitk_volumes_calc_display_volume(volumes, 
					      AMITK_SPACE(canvas->volume), 
					      canvas->center, 
					      AMITK_VOLUME_Z_CORNER(canvas->volume),
					      AMITK_STUDY_FOV(canvas->study),
					      canvas->volume);
  amitk_objects_unref(volumes);

  return changed;
}


/* function to update the adjustment settings for the scrollbar */
static void canvas_update_scrollbar(AmitkCanvas * canvas, AmitkPoint center, amide_real_t thickness) {

  AmitkCorners view_corner;
  amide_real_t upper, lower;
  amide_real_t min_voxel_size;
  AmitkPoint zp_start;
  GList * volumes;
  GList * data_sets;

  /* sanity checks */
  if (canvas->study == NULL) return; 

  /* make valgrind happy */
  view_corner[0]=zero_point;
  view_corner[1]=zero_point;

  volumes = amitk_object_get_selected_children_of_type(AMITK_OBJECT(canvas->study), 
						       AMITK_OBJECT_TYPE_VOLUME,
						       canvas->view_mode,
						       TRUE);
  amitk_volumes_get_enclosing_corners(volumes, AMITK_SPACE(canvas->volume), view_corner);
  amitk_objects_unref(volumes);

  data_sets = amitk_object_get_selected_children_of_type(AMITK_OBJECT(canvas->study), 
							 AMITK_OBJECT_TYPE_DATA_SET,
							 canvas->view_mode,
							 TRUE);
  min_voxel_size = amitk_data_sets_get_min_voxel_size(data_sets);
  amitk_objects_unref(data_sets);

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
  GTK_ADJUSTMENT(canvas->scrollbar_adjustment)->step_increment = min_voxel_size;
  GTK_ADJUSTMENT(canvas->scrollbar_adjustment)->page_increment = (thickness/2.0 < min_voxel_size) ? min_voxel_size : thickness/2.0;
  GTK_ADJUSTMENT(canvas->scrollbar_adjustment)->page_size = 0;
  GTK_ADJUSTMENT(canvas->scrollbar_adjustment)->value = zp_start.z;

  /* allright, we need to update widgets connected to the adjustment without triggering our callback */
  g_signal_handlers_block_by_func(G_OBJECT(canvas->scrollbar_adjustment),
				   G_CALLBACK(canvas_scrollbar_adjustment_cb), canvas);
  gtk_adjustment_changed(GTK_ADJUSTMENT(canvas->scrollbar_adjustment));  
  g_signal_handlers_unblock_by_func(G_OBJECT(canvas->scrollbar_adjustment), 
				     G_CALLBACK(canvas_scrollbar_adjustment_cb), canvas);

  return;
}


/* function to update the target cross on the canvas */
static void canvas_update_target(AmitkCanvas * canvas, AmitkCanvasTargetAction action, 
				 AmitkPoint center, amide_real_t thickness) {

  GnomeCanvasPoints * points[8];
  AmitkCanvasPoint point0, point1;
  AmitkPoint start, end;
  gint i;
  gdouble separation;
  rgba_t color;

  if (canvas->type == AMITK_CANVAS_TYPE_FLY_THROUGH) return;

  if ((canvas->slices == NULL) || 
      ((action == AMITK_CANVAS_TARGET_ACTION_HIDE) && 
       (!AMITK_STUDY_CANVAS_TARGET(canvas->study)))) {
    for (i=0; i < 8 ; i++) 
      if (canvas->target[i] != NULL) gnome_canvas_item_hide(canvas->target[i]);
    return;
  }
  if (((action == AMITK_CANVAS_TARGET_ACTION_HIDE) && AMITK_STUDY_CANVAS_TARGET(canvas->study)) ||
      (action == AMITK_CANVAS_TARGET_ACTION_LEAVE)) {
    thickness = AMITK_VOLUME_Z_CORNER(canvas->volume);
    center = canvas->center;
  } 
  color = amitk_color_table_outline_color(canvas_get_color_table(canvas), FALSE);

  start = end = amitk_space_b2s(AMITK_SPACE(canvas->volume), center);
  start.x -= thickness/2.0;
  start.y -= thickness/2.0;
  end.x += thickness/2.0;
  end.y += thickness/2.0;

  /* get the canvas locations corresponding to the start and end coordinates */
  point0 = p_2_cp(canvas, start);
  point1 = p_2_cp(canvas, end);

  separation = (point1.x - point0.x)/2.0;
  if (separation < AMITK_STUDY_CANVAS_TARGET_EMPTY_AREA(canvas->study))
    separation = AMITK_STUDY_CANVAS_TARGET_EMPTY_AREA(canvas->study)-separation;
  else
    separation = 0.0;
    
  points[0] = gnome_canvas_points_new(2);
  points[0]->coords[0] = (gdouble) canvas->border_width;
  points[0]->coords[1] = point1.y;
  points[0]->coords[2] = 
    ((point0.x-separation) > canvas->border_width) ? (point0.x-separation) : canvas->border_width;
  points[0]->coords[3] = point1.y;

  points[1] = gnome_canvas_points_new(2);
  points[1]->coords[0] = point0.x;
  points[1]->coords[1] = 
    ((point1.y-separation) > canvas->border_width) ? (point1.y-separation) : canvas->border_width;
  points[1]->coords[2] = point0.x;
  points[1]->coords[3] = (gdouble) canvas->border_width;
    
  points[2] = gnome_canvas_points_new(2);
  points[2]->coords[0] = point1.x;
  points[2]->coords[1] = (gdouble) canvas->border_width;
  points[2]->coords[2] = point1.x;
  points[2]->coords[3] =
    ((point1.y-separation) > canvas->border_width) ? (point1.y-separation) : canvas->border_width;

  points[3] = gnome_canvas_points_new(2);
  points[3]->coords[0] = 
    ((point1.x+separation) < canvas->pixbuf_width+canvas->border_width) ? 
    (point1.x+separation) : canvas->pixbuf_width+canvas->border_width;
  points[3]->coords[1] = point1.y;
  points[3]->coords[2] = (gdouble) (canvas->pixbuf_width+canvas->border_width);
  points[3]->coords[3] = point1.y;
  
  points[4] = gnome_canvas_points_new(2);
  points[4]->coords[0] = (gdouble) canvas->border_width;
  points[4]->coords[1] = point0.y;
  points[4]->coords[2] = 
    ((point0.x-separation) > canvas->border_width) ? (point0.x-separation) : canvas->border_width;
  points[4]->coords[3] = point0.y;

  points[5] = gnome_canvas_points_new(2);
  points[5]->coords[0] = point0.x;
  points[5]->coords[1] = 
    ((point0.y+separation) < canvas->pixbuf_height+canvas->border_width) ? 
    (point0.y+separation) : canvas->pixbuf_height+canvas->border_width;
  points[5]->coords[2] = point0.x;
  points[5]->coords[3] = (gdouble) (canvas->pixbuf_height+canvas->border_width);
  
  points[6] = gnome_canvas_points_new(2);
  points[6]->coords[0] = point1.x;
  points[6]->coords[1] = (gdouble) (canvas->pixbuf_height+canvas->border_width);
  points[6]->coords[2] = point1.x;
  points[6]->coords[3] = 
    ((point0.y+separation) < canvas->pixbuf_height+canvas->border_width) ? 
    (point0.y+separation) : canvas->pixbuf_height+canvas->border_width;

  points[7] = gnome_canvas_points_new(2);
  points[7]->coords[0] = 
    ((point1.x+separation) < canvas->pixbuf_width+canvas->border_width) ? 
    (point1.x+separation) : canvas->pixbuf_width+canvas->border_width;
  points[7]->coords[1] = point0.y;
  points[7]->coords[2] = (gdouble) (canvas->pixbuf_width+canvas->border_width);
  points[7]->coords[3] = point0.y;

  for (i=0; i<8; i++) {
    if (canvas->target[i]==NULL) {
      canvas->target[i] =
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
			      gnome_canvas_line_get_type(),
			      "points", points[i], 
			      "fill_color_rgba", amitk_color_table_rgba_to_uint32(color),
			      "width_pixels", 1, NULL);
      g_signal_connect(G_OBJECT(canvas->target[i]), "event", G_CALLBACK(canvas_event_cb), canvas);
    } else if (action == AMITK_CANVAS_TARGET_ACTION_SHOW)
      gnome_canvas_item_set(canvas->target[i],"points",points[i], 
			    "fill_color_rgba", amitk_color_table_rgba_to_uint32(color),
			    "width_pixels", 1, NULL);
    else 
      gnome_canvas_item_set(canvas->target[i],"points",points[i], 
			    "fill_color_rgba", amitk_color_table_rgba_to_uint32(color), NULL);
    gnome_canvas_item_show(canvas->target[i]);
    gnome_canvas_points_unref(points[i]);
  }

  return;
}




/* function to update the arrows on the canvas */
static void canvas_update_arrows(AmitkCanvas * canvas) {

  GnomeCanvasPoints * points[4];
  AmitkCanvasPoint point0, point1;
  AmitkPoint start, end;
  gint i;
  amide_real_t thickness;
  

  if (canvas->type == AMITK_CANVAS_TYPE_FLY_THROUGH) return;

  if (canvas->slices == NULL) {
    for (i=0; i<4; i++)
      if (canvas->arrows[i] != NULL)
	gnome_canvas_item_hide(canvas->arrows[i]);
    return;
  }

  /* figure out the dimensions of the view "box" */
  thickness= AMITK_VOLUME_Z_CORNER(canvas->volume);
  start = amitk_space_b2s(AMITK_SPACE(canvas->volume), canvas->center);
  start.x -= thickness/2.0;
  start.y -= thickness/2.0;
  end = amitk_space_b2s(AMITK_SPACE(canvas->volume), canvas->center);
  end.x += thickness/2.0;
  end.y += thickness/2.0;
  
  /* get the canvas locations corresponding to the start and end coordinates */
  point0 = p_2_cp(canvas, start);
  point1 = p_2_cp(canvas, end);

  /* notes:
     1) even coords are the x coordinate, odd coords are the y
     2) drawing coordinate frame starts from the top left
     3) X's origin is top left, ours is bottom left
  */
  /* left arrow */
  points[0] = gnome_canvas_points_new(4);
  points[0]->coords[0] = DEFAULT_CANVAS_BORDER_WIDTH-DEFAULT_CANVAS_TRIANGLE_SEPARATION;
  points[0]->coords[1] = point1.y;
  points[0]->coords[2] = points[0]->coords[0];
  points[0]->coords[3] = point0.y;
  points[0]->coords[4] = DEFAULT_CANVAS_TRIANGLE_BORDER;
  points[0]->coords[5] = point0.y + DEFAULT_CANVAS_TRIANGLE_WIDTH/2.0;
  points[0]->coords[6] = DEFAULT_CANVAS_TRIANGLE_BORDER;
  points[0]->coords[7] = point1.y - DEFAULT_CANVAS_TRIANGLE_WIDTH/2.0;

  /* top arrow */
  points[1] = gnome_canvas_points_new(4);
  points[1]->coords[0] = point0.x;
  points[1]->coords[1] = DEFAULT_CANVAS_BORDER_WIDTH-DEFAULT_CANVAS_TRIANGLE_SEPARATION;
  points[1]->coords[2] = point1.x;
  points[1]->coords[3] = points[1]->coords[1];
  points[1]->coords[4] = point1.x + DEFAULT_CANVAS_TRIANGLE_WIDTH/2.0;
  points[1]->coords[5] = DEFAULT_CANVAS_TRIANGLE_BORDER;
  points[1]->coords[6] = point0.x - DEFAULT_CANVAS_TRIANGLE_WIDTH/2.0;
  points[1]->coords[7] = DEFAULT_CANVAS_TRIANGLE_BORDER;

  /* right arrow */
  points[2] = gnome_canvas_points_new(4);
  points[2]->coords[0] = DEFAULT_CANVAS_BORDER_WIDTH + DEFAULT_CANVAS_TRIANGLE_SEPARATION + canvas->pixbuf_width;
  points[2]->coords[1] = point1.y;
  points[2]->coords[2] = points[2]->coords[0];
  points[2]->coords[3] = point0.y;
  points[2]->coords[4] = DEFAULT_CANVAS_BORDER_WIDTH + DEFAULT_CANVAS_TRIANGLE_HEIGHT + DEFAULT_CANVAS_TRIANGLE_SEPARATION + canvas->pixbuf_width;
  points[2]->coords[5] = point0.y + DEFAULT_CANVAS_TRIANGLE_WIDTH/2;
  points[2]->coords[6] = points[2]->coords[4];
  points[2]->coords[7] = point1.y - DEFAULT_CANVAS_TRIANGLE_WIDTH/2;

  /* bottom arrow */
  points[3] = gnome_canvas_points_new(4);
  points[3]->coords[0] = point0.x;
  points[3]->coords[1] = DEFAULT_CANVAS_BORDER_WIDTH + DEFAULT_CANVAS_TRIANGLE_SEPARATION + canvas->pixbuf_height;
  points[3]->coords[2] = point1.x;
  points[3]->coords[3] = points[3]->coords[1];
  points[3]->coords[4] = point1.x + DEFAULT_CANVAS_TRIANGLE_WIDTH/2;
  points[3]->coords[5] = DEFAULT_CANVAS_BORDER_WIDTH+DEFAULT_CANVAS_TRIANGLE_HEIGHT + DEFAULT_CANVAS_TRIANGLE_SEPARATION + canvas->pixbuf_height;
  points[3]->coords[6] = point0.x - DEFAULT_CANVAS_TRIANGLE_WIDTH/2;
  points[3]->coords[7] = points[3]->coords[5];


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


/* function to update the line profile on the canvas */
static void canvas_update_line_profile(AmitkCanvas * canvas) {

  GnomeCanvasPoints * points;
  AmitkCanvasPoint point0, point1;
  AmitkPoint start, end;
  amide_real_t temp;
  amide_real_t profile_angle;
  gint roi_width;
#ifndef AMIDE_LIBGNOMECANVAS_AA
  GdkLineStyle line_style;
#endif
  guint32 fill_color_rgba;
  rgba_t outline_color;
  AmitkLineProfile * line_profile;
  gdouble affine[6];
  AmitkPoint initial;
  

  if (canvas->type == AMITK_CANVAS_TYPE_FLY_THROUGH) return;
  line_profile = AMITK_STUDY_LINE_PROFILE(canvas->study);

  /* figure out if we need to hide it */
  if ((canvas->view != AMITK_LINE_PROFILE_VIEW(line_profile)) ||
      (!AMITK_LINE_PROFILE_VISIBLE(line_profile)) ||
      !amitk_volume_point_in_bounds(canvas->volume, canvas->center)) {
    /* the !amitk_volume_point_in_bounds gets hit when there's no data set 
       on the given canvas..  only happens if multiple views are up at the same
    time */

    if (canvas->line_profile_item != NULL)
      gnome_canvas_item_hide(canvas->line_profile_item);
    return;
  }


  initial = amitk_space_b2s(AMITK_SPACE(canvas->volume), canvas->center);

  profile_angle = AMITK_LINE_PROFILE_ANGLE(line_profile);
  if (canvas->view == AMITK_VIEW_SAGITTAL) profile_angle = -profile_angle; /* sagittal is a left-handed coord frame */

  /* z location's easy */
  start.z = end.z = initial.z; 

  /* x locations */
  if (REAL_EQUAL(profile_angle, M_PI/2.0)) { /* 90 degrees */
    start.x = end.x = initial.x;
  } else if (REAL_EQUAL(profile_angle, 0) || REAL_EQUAL(profile_angle, M_PI)) { /* 0, 180 degrees */
    start.x = 0;
    end.x = AMITK_VOLUME_X_CORNER(canvas->volume);
  } else { /* everything else */
    start.x = initial.x - initial.y/tan(profile_angle);
    end.x = initial.x + (AMITK_VOLUME_Y_CORNER(canvas->volume)-initial.y)/tan(profile_angle);
  }
  if (start.x < 0.0) 
    start.x = 0.0;
  else if (start.x > AMITK_VOLUME_X_CORNER(canvas->volume)) 
    start.x = AMITK_VOLUME_X_CORNER(canvas->volume);
  if (end.x < 0.0) 
    end.x = 0.0;
  else if (end.x > AMITK_VOLUME_X_CORNER(canvas->volume)) 
    end.x = AMITK_VOLUME_X_CORNER(canvas->volume);
  if (profile_angle < 0.0) {
    temp = start.x;
    start.x = end.x;
    end.x = temp;
  }

  /* y locations */
  if (REAL_EQUAL(profile_angle, M_PI/2.0)) { /* 90 degrees */
    start.y = 0;
    end.y = AMITK_VOLUME_Y_CORNER(canvas->volume);
  } else if (REAL_EQUAL(profile_angle, 0) || REAL_EQUAL(profile_angle, M_PI)) { /* 0, 180 degrees */
    start.y = end.y = initial.y;
  } else { /* everything else */
    start.y = initial.y - initial.x*tan(profile_angle);
    end.y = initial.y + (AMITK_VOLUME_X_CORNER(canvas->volume)-initial.x)*tan(profile_angle);
  }
  if (start.y < 0.0) 
    start.y = 0.0;
  else if (start.y > AMITK_VOLUME_Y_CORNER(canvas->volume)) 
    start.y = AMITK_VOLUME_Y_CORNER(canvas->volume);
  if (end.y < 0.0) 
    end.y = 0.0;
  else if (end.y > AMITK_VOLUME_Y_CORNER(canvas->volume)) 
    end.y = AMITK_VOLUME_Y_CORNER(canvas->volume);
  if (fabs(profile_angle) > M_PI/2) {
    temp = start.y;
    start.y = end.y;
    end.y = temp;
  }
    

  /* get the canvas locations corresponding to the start and end coordinates */
  point0 = p_2_cp(canvas, start);
  point1 = p_2_cp(canvas, end);

  /* record our start and end */
  start = amitk_space_s2b(AMITK_SPACE(canvas->volume), start);
  amitk_line_profile_set_start_point(AMITK_STUDY_LINE_PROFILE(canvas->study), start);
  end = amitk_space_s2b(AMITK_SPACE(canvas->volume), end);
  amitk_line_profile_set_end_point(AMITK_STUDY_LINE_PROFILE(canvas->study), end);

  /* calculate the line */
  points = gnome_canvas_points_new(2);
  points->coords[0] = point0.x;
  points->coords[1] = point0.y;
  points->coords[2] = point1.x;
  points->coords[3] = point1.y;


  roi_width = AMITK_STUDY_CANVAS_ROI_WIDTH(canvas->study);
#ifndef AMIDE_LIBGNOMECANVAS_AA
  line_style = AMITK_STUDY_CANVAS_LINE_STYLE(canvas->study);
#endif
  outline_color = amitk_color_table_outline_color(canvas_get_color_table(canvas), TRUE);
  fill_color_rgba = amitk_color_table_rgba_to_uint32(outline_color);

  if (canvas->line_profile_item != NULL ) {
    /* make sure to reset any affine translations we've done */
    gnome_canvas_item_i2w_affine(canvas->line_profile_item,affine);
    affine[0] = affine[3] = 1.0;
    affine[1] = affine[2] = affine[4] = affine[5] = 0.0;
    gnome_canvas_item_affine_absolute(canvas->line_profile_item,affine);

    gnome_canvas_item_set(canvas->line_profile_item,
			  "points",points, 
			  "fill_color_rgba", fill_color_rgba,
			  "width_pixels", roi_width, 
#ifndef AMIDE_LIBGNOMECANVAS_AA
			  "line_style", line_style,  
#endif
			  NULL);
  } else {
    canvas->line_profile_item = 
      gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
			    gnome_canvas_line_get_type(),
			    "points", points,
			    "fill_color_rgba", fill_color_rgba,
			    "width_pixels", roi_width,
			    "last_arrowhead", TRUE,
			    "arrow_shape_a", (gdouble) 6.0,
			    "arrow_shape_b", (gdouble) 5.0,
			    "arrow_shape_c", (gdouble) 4.0,
#ifndef AMIDE_LIBGNOMECANVAS_AA
			    "line_style", line_style,
#endif
			    NULL);
    g_object_set_data(G_OBJECT(canvas->line_profile_item), "object", line_profile);
    g_signal_connect(G_OBJECT(canvas->line_profile_item), "event", G_CALLBACK(canvas_event_cb), canvas);
  }

  
  gnome_canvas_item_show(canvas->line_profile_item);
  gnome_canvas_points_unref(points);

  return;
}


/* function to update the line profile on the canvas */
static void canvas_update_time_on_image(AmitkCanvas * canvas) {

  amide_time_t midpt_time;
  gint hours, minutes, seconds;
  gchar * time_str;
  rgba_t color;

  /* put up the timer */
  if (canvas->time_on_image) {

    midpt_time =   AMITK_STUDY_VIEW_START_TIME(canvas->study)+
      AMITK_STUDY_VIEW_DURATION(canvas->study)/2.0;
    hours = floor(midpt_time/3600);
    midpt_time -= hours*3600;
    minutes = floor(midpt_time/60);
    midpt_time -= minutes*60;
    seconds = midpt_time;
    time_str = g_strdup_printf("%d:%.2d:%.2d",hours,minutes,seconds);

    color = amitk_color_table_outline_color(canvas_get_color_table(canvas), FALSE);
    if (canvas->time_label != NULL) 
      gnome_canvas_item_set(canvas->time_label,
			    "text", time_str,
			    "fill_color_rgba", color, NULL);
    else
      canvas->time_label = 
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
			      gnome_canvas_text_get_type(),
			      "anchor", GTK_ANCHOR_SOUTH_WEST,
			      "text", time_str,
			      "x", 4.0,
			      "y", canvas->pixbuf_height-2.0,
			      "fill_color_rgba", color,
			      "font_desc", amitk_fixed_font_desc, NULL);
    g_free(time_str);
  } else {
    if (canvas->time_label != NULL) {
      gtk_object_destroy(GTK_OBJECT(canvas->time_label));
      canvas->time_label = NULL;
    }
  }

  return;
}


static void canvas_update_subject_orientation(AmitkCanvas * canvas) {

  gboolean remove = FALSE;
  int i;
  float x[4];
  float y[4];
  gint anchor[4];
  gint which_orientation[4];

  if (canvas->active_object == NULL)
    remove = TRUE;
  else if (!AMITK_IS_DATA_SET(canvas->active_object))
    remove = TRUE;
  else if (AMITK_DATA_SET_SUBJECT_ORIENTATION(canvas->active_object) == 
	   AMITK_SUBJECT_ORIENTATION_UNKNOWN)
    remove = TRUE;

  if (remove) {
    for (i=0; i<4; i++) 
      if (canvas->orientation_label[i] != NULL)
	gnome_canvas_item_hide(canvas->orientation_label[i]);

  } else {

    switch(canvas->view) {

    case AMITK_VIEW_TRANSVERSE:
      switch(AMITK_DATA_SET_SUBJECT_ORIENTATION(canvas->active_object)) {
      case AMITK_SUBJECT_ORIENTATION_SUPINE_HEADFIRST:
	which_orientation[0] = ANTERIOR;
	which_orientation[1] = POSTERIOR;
	which_orientation[2] = RIGHT;
	which_orientation[3] = LEFT;
	break;
      case AMITK_SUBJECT_ORIENTATION_SUPINE_FEETFIRST:
	which_orientation[0] = ANTERIOR;
	which_orientation[1] = POSTERIOR;
	which_orientation[2] = LEFT;
	which_orientation[3] = RIGHT;
	break;
      case AMITK_SUBJECT_ORIENTATION_PRONE_HEADFIRST:
	which_orientation[0] = POSTERIOR;
	which_orientation[1] = ANTERIOR;
	which_orientation[2] = LEFT;
	which_orientation[3] = RIGHT;
	break;
      case AMITK_SUBJECT_ORIENTATION_PRONE_FEETFIRST:
	which_orientation[0] = POSTERIOR;
	which_orientation[1] = ANTERIOR;
	which_orientation[2] = RIGHT;
	which_orientation[3] = LEFT;
	break;
      case AMITK_SUBJECT_ORIENTATION_RIGHT_DECUBITUS_HEADFIRST:
	which_orientation[0] = LEFT;
	which_orientation[1] = RIGHT;
	which_orientation[2] = ANTERIOR;
	which_orientation[3] = POSTERIOR;
	break;
      case AMITK_SUBJECT_ORIENTATION_RIGHT_DECUBITUS_FEETFIRST:
	which_orientation[0] = LEFT;
	which_orientation[1] = RIGHT;
	which_orientation[2] = POSTERIOR;
	which_orientation[3] = ANTERIOR;
	break;
      case AMITK_SUBJECT_ORIENTATION_LEFT_DECUBITUS_HEADFIRST:
	which_orientation[0] = RIGHT;
	which_orientation[1] = LEFT;
	which_orientation[2] = POSTERIOR;
	which_orientation[3] = ANTERIOR;
	break;
      case AMITK_SUBJECT_ORIENTATION_LEFT_DECUBITUS_FEETFIRST:
	which_orientation[0] = RIGHT;
	which_orientation[1] = LEFT;
	which_orientation[2] = ANTERIOR;
	which_orientation[3] = POSTERIOR;
	break;
      default:
	g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
	break;
      }
      break;

    case AMITK_VIEW_CORONAL:
      switch(AMITK_DATA_SET_SUBJECT_ORIENTATION(canvas->active_object)) {
      case AMITK_SUBJECT_ORIENTATION_SUPINE_HEADFIRST:
	which_orientation[0] = SUPERIOR;
	which_orientation[1] = INFERIOR;
	which_orientation[2] = RIGHT;
	which_orientation[3] = LEFT;
	break;
      case AMITK_SUBJECT_ORIENTATION_SUPINE_FEETFIRST:
	which_orientation[0] = INFERIOR;
	which_orientation[1] = SUPERIOR;
	which_orientation[2] = LEFT;
	which_orientation[3] = RIGHT;
	break;
      case AMITK_SUBJECT_ORIENTATION_PRONE_HEADFIRST:
	which_orientation[0] = SUPERIOR;
	which_orientation[1] = INFERIOR;
	which_orientation[2] = LEFT;
	which_orientation[3] = RIGHT;
	break;
      case AMITK_SUBJECT_ORIENTATION_PRONE_FEETFIRST:
	which_orientation[0] = INFERIOR;
	which_orientation[1] = SUPERIOR;
	which_orientation[2] = RIGHT;
	which_orientation[3] = LEFT;
	break;
      case AMITK_SUBJECT_ORIENTATION_RIGHT_DECUBITUS_HEADFIRST:
	which_orientation[0] = SUPERIOR;
	which_orientation[1] = INFERIOR;
	which_orientation[2] = ANTERIOR;
	which_orientation[3] = POSTERIOR;
	break;
      case AMITK_SUBJECT_ORIENTATION_RIGHT_DECUBITUS_FEETFIRST:
	which_orientation[0] = INFERIOR;
	which_orientation[1] = SUPERIOR;
	which_orientation[2] = POSTERIOR;
	which_orientation[3] = ANTERIOR;
	break;
      case AMITK_SUBJECT_ORIENTATION_LEFT_DECUBITUS_HEADFIRST:
	which_orientation[0] = SUPERIOR;
	which_orientation[1] = INFERIOR;
	which_orientation[2] = POSTERIOR;
	which_orientation[3] = ANTERIOR;
	break;
      case AMITK_SUBJECT_ORIENTATION_LEFT_DECUBITUS_FEETFIRST:
	which_orientation[0] = INFERIOR;
	which_orientation[1] = SUPERIOR;
	which_orientation[2] = ANTERIOR;
	which_orientation[3] = POSTERIOR;
	break;
      default:
	g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
	break;
      }
      break;

    case AMITK_VIEW_SAGITTAL:
      switch(AMITK_DATA_SET_SUBJECT_ORIENTATION(canvas->active_object)) {
      case AMITK_SUBJECT_ORIENTATION_SUPINE_HEADFIRST:
	which_orientation[0] = SUPERIOR;
	which_orientation[1] = INFERIOR;
	which_orientation[2] = POSTERIOR;
	which_orientation[3] = ANTERIOR;
	break;
      case AMITK_SUBJECT_ORIENTATION_SUPINE_FEETFIRST:
	which_orientation[0] = INFERIOR;
	which_orientation[1] = SUPERIOR;
	which_orientation[2] = POSTERIOR;
	which_orientation[3] = ANTERIOR;
	break;
      case AMITK_SUBJECT_ORIENTATION_PRONE_HEADFIRST:
	which_orientation[0] = SUPERIOR;
	which_orientation[1] = INFERIOR;
	which_orientation[2] = ANTERIOR;
	which_orientation[3] = POSTERIOR;
	break;
      case AMITK_SUBJECT_ORIENTATION_PRONE_FEETFIRST:
	which_orientation[0] = INFERIOR;
	which_orientation[1] = SUPERIOR;
	which_orientation[2] = ANTERIOR;
	which_orientation[3] = POSTERIOR;
	break;
      case AMITK_SUBJECT_ORIENTATION_RIGHT_DECUBITUS_HEADFIRST:
	which_orientation[0] = SUPERIOR;
	which_orientation[1] = INFERIOR;
	which_orientation[2] = RIGHT;
	which_orientation[3] = LEFT;
	break;
      case AMITK_SUBJECT_ORIENTATION_RIGHT_DECUBITUS_FEETFIRST:
	which_orientation[0] = INFERIOR;
	which_orientation[1] = SUPERIOR;
	which_orientation[2] = RIGHT;
	which_orientation[3] = LEFT;
	break;
      case AMITK_SUBJECT_ORIENTATION_LEFT_DECUBITUS_HEADFIRST:
	which_orientation[0] = SUPERIOR;
	which_orientation[1] = INFERIOR;
	which_orientation[2] = LEFT;
	which_orientation[3] = RIGHT;
	break;
      case AMITK_SUBJECT_ORIENTATION_LEFT_DECUBITUS_FEETFIRST:
	which_orientation[0] = INFERIOR;
	which_orientation[1] = SUPERIOR;
	which_orientation[2] = LEFT;
	which_orientation[3] = RIGHT;
	break;
      default:
	g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
	break;
      }
      break;

    default:
      g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
      break;
    }

    /* text locations */
    x[0] = 0;
    y[0] = canvas->border_width;
    anchor[0] = GTK_ANCHOR_NORTH_WEST;

    x[1] = 0;
    y[1] = canvas->border_width + canvas->pixbuf_height;
    anchor[1] = GTK_ANCHOR_SOUTH_WEST;
    
    x[2] = canvas->border_width;
    y[2] = 2*canvas->border_width + canvas->pixbuf_height;
    anchor[2] = GTK_ANCHOR_SOUTH_WEST;
    
    x[3] = canvas->border_width+canvas->pixbuf_width;
    y[3] = 2*canvas->border_width + canvas->pixbuf_height;
    anchor[3] = GTK_ANCHOR_SOUTH_EAST;

    
    for (i=0; i<4; i++) {
      if (canvas->orientation_label[i] != NULL ) 
	gnome_canvas_item_set(canvas->orientation_label[i],"text", _(orientation_label[which_orientation[i]]), 
			      "x", x[i], "y", y[i], NULL);
      else 
	canvas->orientation_label[i] = 
	  gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
				gnome_canvas_text_get_type(),
				"anchor", anchor[i],
				"text", _(orientation_label[which_orientation[i]]),
				"x", x[i],
				"y", y[i],
				"fill_color", "black",
				"font_desc", amitk_fixed_font_desc, NULL);
      gnome_canvas_item_show(canvas->orientation_label[i]);
    }
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
  GList * data_sets;
  AmitkDataSet * active_ds;


  /* sanity checks */
  g_return_if_fail(canvas->study != NULL);

  old_width = canvas->pixbuf_width;
  old_height = canvas->pixbuf_height;

  /* free the previous pixbuf if possible */
  if (canvas->pixbuf != NULL) {
    g_object_unref(canvas->pixbuf);
    canvas->pixbuf = NULL;
  }

  /* compensate for zoom */
  pixel_dim = (1/AMITK_STUDY_ZOOM(canvas->study))*AMITK_STUDY_VOXEL_DIM(canvas->study); 

  data_sets = amitk_object_get_selected_children_of_type(AMITK_OBJECT(canvas->study),
  							 AMITK_OBJECT_TYPE_DATA_SET,
  							 canvas->view_mode,
  							 TRUE);

  if (data_sets == NULL) {
    /* just use a blank image */

    /* figure out what color to use */
    widget_style = gtk_widget_get_style(GTK_WIDGET(canvas));
    if (widget_style == NULL) {
      g_warning(_("Canvas has no style?\n"));
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
    amitk_objects_unref(canvas->slices);
    canvas->slices = NULL;

  } else {
    if (AMITK_IS_DATA_SET(canvas->active_object))
      active_ds = AMITK_DATA_SET(canvas->active_object);
    else
      active_ds = NULL;
    canvas->pixbuf = image_from_data_sets(&(canvas->slices),
					  &(canvas->slice_cache),
					  canvas->max_slice_cache_size,
					  data_sets,
					  active_ds,
					  AMITK_STUDY_VIEW_START_TIME(canvas->study),
					  AMITK_STUDY_VIEW_DURATION(canvas->study),
					  -1,
					  pixel_dim,
					  canvas->volume,
					  AMITK_STUDY_FUSE_TYPE(canvas->study),
					  AMITK_CANVAS_VIEW_MODE(canvas));
    amitk_objects_unref(data_sets);
  }

  if (canvas->pixbuf != NULL) {
    /* record the width and height for future use*/
    canvas->pixbuf_width = gdk_pixbuf_get_width(canvas->pixbuf);
    canvas->pixbuf_height = gdk_pixbuf_get_height(canvas->pixbuf);

    /* reset the min size of the widget and set the scroll region */
    if ((old_width != canvas->pixbuf_width) || 
	(old_height != canvas->pixbuf_height) || (canvas->image == NULL)) {
      gtk_widget_set_size_request(canvas->canvas, 
				  canvas->pixbuf_width + 2 * canvas->border_width, 
				  canvas->pixbuf_height + 2 * canvas->border_width);
      gnome_canvas_set_scroll_region(GNOME_CANVAS(canvas->canvas), 0.0, 0.0, 
				     canvas->pixbuf_width + 2 * canvas->border_width,
				     canvas->pixbuf_height + 2 * canvas->border_width);
    }
    /* put the canvas rgb image on the canvas_image */
    if (canvas->image == NULL) {/* time to make a new image */
      canvas->image = gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas->canvas)),
					    gnome_canvas_pixbuf_get_type(),
					    "pixbuf", canvas->pixbuf,
					    "x", (double) canvas->border_width,
					    "y", (double) canvas->border_width,
					    NULL);
      g_signal_connect(G_OBJECT(canvas->image), "event", G_CALLBACK(canvas_event_cb), canvas);
    } else {
      gnome_canvas_item_set(canvas->image, "pixbuf", canvas->pixbuf, NULL);
    }
    
  }

  return;
}


static void canvas_update_object(AmitkCanvas * canvas, AmitkObject * object) {

  rgba_t outline_color;
  GnomeCanvasItem * item;
  GnomeCanvasItem * new_item;
  amide_real_t pixel_dim;

  g_return_if_fail(object != NULL);
  g_return_if_fail(canvas->study != NULL);

  item = canvas_find_item(canvas, object);

  if (item != NULL) {
    g_return_if_fail(object == g_object_get_data(G_OBJECT(item), "object"));
  }

  outline_color = amitk_color_table_outline_color(canvas_get_color_table(canvas), TRUE);

  /* compensate for zoom */
  pixel_dim = (1/AMITK_STUDY_ZOOM(canvas->study))*AMITK_STUDY_VOXEL_DIM(canvas->study); 

  new_item = amitk_canvas_object_draw(GNOME_CANVAS(canvas->canvas), 
				      canvas->volume, object, AMITK_CANVAS_VIEW_MODE(canvas),
				      item, pixel_dim,
				      canvas->pixbuf_width, canvas->pixbuf_height,
				      canvas->border_width,canvas->border_width,
				      outline_color, 
				      AMITK_STUDY_CANVAS_ROI_WIDTH(canvas->study),
#ifdef AMIDE_LIBGNOMECANVAS_AA
				      AMITK_STUDY_CANVAS_ROI_TRANSPARENCY(canvas->study)
#else
				      AMITK_STUDY_CANVAS_LINE_STYLE(canvas->study),
				      AMITK_STUDY_CANVAS_FILL_ROI(canvas->study)
#endif
);


  if ((item == NULL) && (new_item != NULL)) {
    g_object_set_data(G_OBJECT(new_item), "object", object);
    g_signal_connect(G_OBJECT(new_item), "event", G_CALLBACK(canvas_event_cb), canvas);
    canvas->object_items = g_list_append(canvas->object_items, new_item);
  }

  return;
}


static void canvas_update_objects(AmitkCanvas * canvas, gboolean all) {

  GList * objects;
  AmitkObject * object;

  if (all) {
    objects = canvas_add_current_objects(canvas, canvas->next_update_objects);
  } else {
    objects = canvas->next_update_objects;
  }
  canvas->next_update_objects = NULL;

  while (objects != NULL) {
    object = AMITK_OBJECT(objects->data);
    canvas_update_object(canvas, object);
    objects = g_list_remove(objects, object);
    amitk_object_unref(object);
  }
}


static void canvas_update_setup(AmitkCanvas * canvas) {

  gboolean first_time = FALSE;
  gchar * temp_str;

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

    temp_str = g_strdup_printf("%s %d\n", amitk_view_get_name(canvas->view), canvas->view_mode+1);
    canvas->label = gtk_label_new(temp_str);
    g_free(temp_str);

#ifdef AMIDE_LIBGNOMECANVAS_AA
    canvas->canvas = gnome_canvas_new_aa();
#else
    canvas->canvas = gnome_canvas_new();
#endif

    canvas->scrollbar_adjustment = gtk_adjustment_new(0.5, 0, 1, 1, 1, 1); /* junk values */
    g_signal_connect(canvas->scrollbar_adjustment, "value_changed", 
		     G_CALLBACK(canvas_scrollbar_adjustment_cb), canvas);
    canvas->scrollbar = gtk_hscrollbar_new(GTK_ADJUSTMENT(canvas->scrollbar_adjustment));
    gtk_range_set_update_policy(GTK_RANGE(canvas->scrollbar), GTK_UPDATE_CONTINUOUS);

  }

  /* pack it in based on what the layout is */
  switch(AMITK_STUDY_CANVAS_LAYOUT(canvas->study)) {


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


static void canvas_add_object_update(AmitkCanvas * canvas, AmitkObject * object) {

  if (AMITK_IS_STUDY(object)) {
    canvas_add_update(canvas, UPDATE_ALL);
  } else if (amitk_object_get_selected(object, canvas->view_mode)) {

    if (AMITK_IS_DATA_SET(object)) {
      canvas_add_update(canvas, UPDATE_ALL);
    } else if (g_list_index(canvas->next_update_objects, object) < 0) {/* not yet in list */
      canvas->next_update_objects=g_list_append(canvas->next_update_objects, amitk_object_ref(object));
      canvas_add_update(canvas, UPDATE_OBJECT);
    }
  }
  return;
}


static void canvas_add_update(AmitkCanvas * canvas, guint update_type) {

  /* reslicing is slow, put up a wait cursor */
  /* don't use ui_common_place_cursor, as this has a gtk_main_iteration call... */
  if ((update_type & UPDATE_DATA_SETS) && !(canvas->next_update & UPDATE_DATA_SETS)) 
    ui_common_place_cursor_no_wait(UI_CURSOR_WAIT,GTK_WIDGET(canvas));

  canvas->next_update = canvas->next_update | update_type;

  /* DEFAULT_IDLE is needed, as this is the first default lower then redraw */
  if (canvas->idle_handler_id == 0)
    canvas->idle_handler_id = 
      g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,canvas_update_while_idle, canvas, NULL);

  return;
}

static gboolean canvas_update_while_idle(gpointer data) {

  AmitkCanvas * canvas = data;

  /* update the corners */
  if (canvas_recalc_corners(canvas)) /* if corners changed */
    canvas->next_update = canvas->next_update | UPDATE_ALL;

  if (canvas->next_update & UPDATE_DATA_SETS) {
    canvas_update_pixbuf(canvas);
  } 
  
  if (canvas->next_update & UPDATE_ARROWS) {
    canvas_update_arrows(canvas);
  }
  
  if (canvas->next_update & UPDATE_SCROLLBAR) {
    canvas_update_scrollbar(canvas, canvas->center, AMITK_VOLUME_Z_CORNER(canvas->volume));
  } 

  if (canvas->next_update & (UPDATE_OBJECTS | UPDATE_OBJECT)) {
    canvas_update_objects(canvas, (canvas->next_update & UPDATE_OBJECTS));
  } 

  if (canvas->next_update & UPDATE_LINE_PROFILE) {
    canvas_update_line_profile(canvas);
  }

  if (canvas->next_update & UPDATE_TIME) {
    canvas_update_time_on_image(canvas);
  }

  if (canvas->next_update & UPDATE_SUBJECT_ORIENTATION) {
    canvas_update_subject_orientation(canvas);
  }


  if (canvas->next_update & UPDATE_TARGET) {
    canvas_update_target(canvas, canvas->next_target_action, canvas->next_target_center,
			 canvas->next_target_thickness);
  }

  canvas->idle_handler_id=0;

  if (canvas->next_update & UPDATE_DATA_SETS) /* remove the cursor on slow updates */
    ui_common_remove_wait_cursor(GTK_WIDGET(canvas));
  canvas->next_update = UPDATE_NONE;

  return FALSE;
}



static void canvas_add_object(AmitkCanvas * canvas, AmitkObject * object) {

  GList * children;

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_OBJECT(object));

  amitk_object_ref(object);

  if (AMITK_IS_STUDY(object)) {
    
    if (canvas->study != NULL) {
      canvas_remove_object(canvas, AMITK_OBJECT(canvas->study));
    }
    canvas->study = AMITK_STUDY(object);

    canvas->center = AMITK_STUDY_VIEW_CENTER(object);
    amitk_volume_set_z_corner(canvas->volume, AMITK_STUDY_VIEW_THICKNESS(object));
  }
  canvas_add_object_update(canvas, object);

  g_signal_connect(G_OBJECT(object), "space_changed", G_CALLBACK(canvas_space_changed_cb), canvas);
  g_signal_connect(G_OBJECT(object), "object_selection_changed", G_CALLBACK(canvas_object_selection_changed_cb), canvas);
  g_signal_connect(G_OBJECT(object), "object_add_child", G_CALLBACK(canvas_object_add_child_cb), canvas);
  g_signal_connect(G_OBJECT(object), "object_remove_child", G_CALLBACK(canvas_object_remove_child_cb), canvas);
  if (AMITK_IS_STUDY(object)) {
    g_signal_connect(G_OBJECT(object), "thickness_changed", G_CALLBACK(canvas_view_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "time_changed", G_CALLBACK(canvas_time_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "canvas_target_changed", G_CALLBACK(canvas_target_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "voxel_dim_or_zoom_changed", G_CALLBACK(canvas_study_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "fov_changed", G_CALLBACK(canvas_study_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "fuse_type_changed", G_CALLBACK(canvas_study_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "view_center_changed", G_CALLBACK(canvas_view_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "canvas_roi_preference_changed", G_CALLBACK(canvas_roi_preference_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "canvas_general_preference_changed", G_CALLBACK(canvas_general_preference_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "canvas_target_preference_changed", G_CALLBACK(canvas_target_preference_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "canvas_layout_preference_changed", G_CALLBACK(canvas_layout_preference_changed_cb), canvas);
    g_signal_connect(G_OBJECT(AMITK_STUDY_LINE_PROFILE(object)), 
		     "line_profile_changed", G_CALLBACK(canvas_line_profile_changed_cb), canvas);
  } 
  if (AMITK_IS_VOLUME(object)) {
    g_signal_connect(G_OBJECT(object), "volume_changed", G_CALLBACK(canvas_volume_changed_cb), canvas);
  } 
  if (AMITK_IS_ROI(object)) {
    g_signal_connect(G_OBJECT(object), "roi_changed", G_CALLBACK(canvas_roi_changed_cb), canvas);
  }
  if (AMITK_IS_FIDUCIAL_MARK(object)) {
    g_signal_connect(G_OBJECT(object), "fiducial_mark_changed", G_CALLBACK(canvas_fiducial_mark_changed_cb), canvas);
  }
  if (AMITK_IS_DATA_SET(object)) {
    g_signal_connect(G_OBJECT(object), "data_set_changed", G_CALLBACK(data_set_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "invalidate_slice_cache", G_CALLBACK(canvas_data_set_invalidate_slice_cache), canvas);
    g_signal_connect(G_OBJECT(object), "interpolation_changed", G_CALLBACK(data_set_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "rendering_changed", G_CALLBACK(data_set_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "thresholding_changed", G_CALLBACK(data_set_thresholding_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "thresholds_changed", G_CALLBACK(data_set_thresholding_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "color_table_changed", G_CALLBACK(data_set_color_table_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "subject_orientation_changed", G_CALLBACK(data_set_subject_orientation_changed_cb), canvas);
    g_signal_connect(G_OBJECT(object), "view_gates_changed", G_CALLBACK(data_set_changed_cb), canvas);
  }

  /* keep track of undrawn rois */
  if (AMITK_IS_ROI(object))
    if (AMITK_ROI_UNDRAWN(object)) 
      if (amitk_object_get_selected(object, canvas->view_mode)) {
	if (g_list_index(canvas->undrawn_rois, object) < 0)  /* not yet in list */
	  canvas->undrawn_rois = g_list_prepend(canvas->undrawn_rois, amitk_object_ref(object));
	amitk_object_ref(object);
      }


  /* recurse */
  children= AMITK_OBJECT_CHILDREN(object);
  while (children != NULL) {
    canvas_add_object(canvas, children->data);
    children = children->next;
  }

  return;
}


static void canvas_remove_object(AmitkCanvas * canvas, AmitkObject * object) {

  GnomeCanvasItem * found_item;
  GList * children;

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_OBJECT(object));
    
  /* remove children */
  children= AMITK_OBJECT_CHILDREN(object);
  while (children != NULL) {
    canvas_remove_object(canvas, children->data);
    children = children->next;
  }

  /* keep track of undrawn rois */
  if (AMITK_IS_ROI(object)) {
    if (g_list_index(canvas->undrawn_rois, object) >= 0) {
      canvas->undrawn_rois = g_list_remove(canvas->undrawn_rois, object);
      amitk_object_unref(object);
    }
  }

  g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_space_changed_cb, canvas);
  g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_object_selection_changed_cb, canvas);
  g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_object_add_child_cb, canvas);
  g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_object_remove_child_cb, canvas);
  if (AMITK_IS_STUDY(object)) {
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_study_changed_cb, canvas);
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_view_changed_cb, canvas);
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_target_changed_cb, canvas);
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_time_changed_cb, canvas);
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_roi_preference_changed_cb, canvas);
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_general_preference_changed_cb, canvas);
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_target_preference_changed_cb, canvas);
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_layout_preference_changed_cb, canvas);
    g_signal_handlers_disconnect_by_func(G_OBJECT(AMITK_STUDY_LINE_PROFILE(object)), 
					 canvas_line_profile_changed_cb, canvas);
  }
  if (AMITK_IS_VOLUME(object)) {
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_volume_changed_cb, canvas);
  }
  if (AMITK_IS_ROI(object)) {
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_roi_changed_cb, canvas);
  }
  if (AMITK_IS_FIDUCIAL_MARK(object)) {
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_fiducial_mark_changed_cb, canvas);
  }
  if (AMITK_IS_DATA_SET(object)) {
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), data_set_changed_cb, canvas);
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), canvas_data_set_invalidate_slice_cache, canvas);
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), data_set_thresholding_changed_cb, canvas);
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), data_set_color_table_changed_cb, canvas);
    g_signal_handlers_disconnect_by_func(G_OBJECT(object), data_set_subject_orientation_changed_cb, canvas);
    canvas->slice_cache = amitk_data_sets_remove_with_slice_parent(canvas->slice_cache, AMITK_DATA_SET(object));
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

  amitk_object_unref(object);
  
  return;
}




GtkWidget * amitk_canvas_new(AmitkStudy * study,
			     AmitkView view, 
			     AmitkViewMode view_mode,
			     AmitkCanvasType type) {

  AmitkCanvas * canvas;

  g_return_val_if_fail(AMITK_IS_STUDY(study), NULL);

  canvas = g_object_new(amitk_canvas_get_type(), NULL);

  canvas->view = view;
  canvas->view_mode = view_mode;

  canvas->type = type;
  switch(type) {
  case AMITK_CANVAS_TYPE_FLY_THROUGH:
    canvas->border_width = 0.0;
    break;
  case AMITK_CANVAS_TYPE_NORMAL:
  default:
    canvas->border_width = DEFAULT_CANVAS_BORDER_WIDTH;
    break;
  }

  amitk_space_set_view_space(AMITK_SPACE(canvas->volume), canvas->view, 
			     AMITK_STUDY_CANVAS_LAYOUT(study));
  
  amitk_canvas_set_study(canvas, study);
  canvas_update_setup(canvas);

  return GTK_WIDGET (canvas);
}

void amitk_canvas_set_study(AmitkCanvas * canvas, AmitkStudy * study) {

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(AMITK_IS_STUDY(study));

  canvas_add_object(canvas, AMITK_OBJECT(study));

  return;
}

void amitk_canvas_set_active_object(AmitkCanvas * canvas, 
				    AmitkObject * active_object) {

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  g_return_if_fail(active_object != NULL);

  if (canvas->active_object != active_object) {
    canvas->active_object = active_object;
    if (AMITK_STUDY_FUSE_TYPE(canvas->study) == AMITK_FUSE_TYPE_BLEND) {
      canvas_add_update(canvas, UPDATE_OBJECTS);
    } else /* AMITK_FUSE_TYPE_OVERLAY */
      canvas_add_update(canvas, UPDATE_DATA_SETS);

    canvas_add_update(canvas, UPDATE_TARGET);
    canvas_add_update(canvas, UPDATE_LINE_PROFILE);
    canvas_add_update(canvas, UPDATE_SUBJECT_ORIENTATION);
  }

  return;
}


void amitk_canvas_update_target(AmitkCanvas * canvas, AmitkCanvasTargetAction action, 
			       AmitkPoint center, amide_real_t thickness) {

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  canvas->next_target_action = action;
  canvas->next_target_center = center;
  canvas->next_target_thickness = thickness;
  canvas_add_update(canvas, UPDATE_TARGET);

  return;
}

void amitk_canvas_set_time_on_image(AmitkCanvas * canvas, gboolean time_on_image) {

  g_return_if_fail(AMITK_IS_CANVAS(canvas));
  if (time_on_image != canvas->time_on_image) {
    canvas->time_on_image = time_on_image;
    canvas_add_update(canvas, UPDATE_TIME);
  }

  return;
}

gint amitk_canvas_get_width(AmitkCanvas * canvas) {

  g_return_val_if_fail(AMITK_IS_CANVAS(canvas), 0);

  return canvas->pixbuf_width + 2*canvas->border_width;
} 

gint amitk_canvas_get_height(AmitkCanvas * canvas) {

  GtkRequisition size;
  gint height;

  g_return_val_if_fail(AMITK_IS_CANVAS(canvas), 0);

  height = canvas->pixbuf_height + 2*canvas->border_width;

  gtk_widget_size_request(canvas->label, &size);
  height+=size.height;

  gtk_widget_size_request(canvas->scrollbar, &size);
  height+=size.height;

  return height;
} 


GdkPixbuf * amitk_canvas_get_pixbuf(AmitkCanvas * canvas) {

  GdkPixbuf * pixbuf;

  pixbuf = amitk_get_pixbuf_from_canvas(GNOME_CANVAS(canvas->canvas), 
					canvas->border_width,canvas->border_width,
					canvas->pixbuf_width, canvas->pixbuf_height);
  

  return pixbuf;
}
