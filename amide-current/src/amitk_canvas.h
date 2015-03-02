/* amitk_canvas.h
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


#ifndef __AMITK_CANVAS_H__
#define __AMITK_CANVAS_H__

/* includes we always need with this widget */
//#include <gdk-pixbuf/gdk-pixbuf.h>
//#include <gtk/gtk.h>
//#include <libgnomecanvas/libgnomecanvas.h>
#include "amitk_study.h"

G_BEGIN_DECLS

#define AMITK_TYPE_CANVAS            (amitk_canvas_get_type ())
#define AMITK_CANVAS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), AMITK_TYPE_CANVAS, AmitkCanvas))
#define AMITK_CANVAS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_CANVAS, AmitkCanvasClass))
#define AMITK_IS_CANVAS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AMITK_TYPE_CANVAS))
#define AMITK_IS_CANVAS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_CANVAS))

#define AMITK_CANVAS_VIEW(obj)       (AMITK_CANVAS(obj)->view)
#define AMITK_CANVAS_VIEW_MODE(obj)  (AMITK_CANVAS(obj)->view_mode)

typedef enum {
  AMITK_CANVAS_TYPE_NORMAL,
  AMITK_CANVAS_TYPE_FLY_THROUGH
} AmitkCanvasType;

typedef enum {
  AMITK_CANVAS_TARGET_ACTION_HIDE,
  AMITK_CANVAS_TARGET_ACTION_SHOW,
  AMITK_CANVAS_TARGET_ACTION_LEAVE
} AmitkCanvasTargetAction;

typedef struct _AmitkCanvas             AmitkCanvas;
typedef struct _AmitkCanvasClass        AmitkCanvasClass;


struct _AmitkCanvas
{
  GtkVBox vbox;

  GtkWidget * canvas;
  GtkWidget * label;
  GtkWidget * scrollbar;
  GtkObject * scrollbar_adjustment;
  GnomeCanvasItem * arrows[4];
  GnomeCanvasItem * orientation_label[4];
  AmitkCanvasType type;

  AmitkVolume * volume; /* the volume that this canvas slice displays */
  AmitkPoint center; /* in base coordinate space */

  AmitkView view;
  AmitkViewMode view_mode;
  gint roi_width;
  AmitkObject * active_object;

  GList * slices;
  GList * slice_cache;
  gint max_slice_cache_size;
  gint pixbuf_width, pixbuf_height;
  gdouble border_width;
  GnomeCanvasItem * image;
  GdkPixbuf * pixbuf;

  gboolean time_on_image;
  GnomeCanvasItem * time_label;

  AmitkStudy * study;
  GList * undrawn_rois;
  GList * object_items;

  guint next_update;
  guint idle_handler_id;
  GList * next_update_objects;

  /* profile stuff */
  GnomeCanvasItem * line_profile_item;

  /* target stuff */
  GnomeCanvasItem * target[8];
  AmitkCanvasTargetAction next_target_action;
  AmitkPoint next_target_center;
  amide_real_t next_target_thickness;

};

struct _AmitkCanvasClass
{
  GtkVBoxClass parent_class;
  
  void (* help_event)                (AmitkCanvas *Canvas,
				      AmitkHelpInfo which_help,
				      AmitkPoint *position,
				      amide_data_t value);
  void (* view_changing)             (AmitkCanvas *Canvas,
				      AmitkPoint *position,
				      amide_real_t thickness);
  void (* view_changed)              (AmitkCanvas *Canvas,
				      AmitkPoint *position,
				      amide_real_t thickness);
  void (* erase_volume)              (AmitkCanvas *Canvas,
				      AmitkRoi *roi,
				      gboolean outside);
  void (* new_object)                (AmitkCanvas *Canvas,
				      AmitkObject * parent,
				      AmitkObjectType type,
				      AmitkPoint *position);
};  


GType         amitk_canvas_get_type             (void);
GtkWidget *   amitk_canvas_new                  (AmitkStudy * study,
						 AmitkView view, 
						 AmitkViewMode view_mode,
						 AmitkCanvasType type);
void          amitk_canvas_set_study            (AmitkCanvas * canvas, 
						 AmitkStudy * study);
void          amitk_canvas_set_active_object    (AmitkCanvas * canvas, 
						 AmitkObject * active_object);
void          amitk_canvas_update_target        (AmitkCanvas * canvas, 
						 AmitkCanvasTargetAction action, 
						 AmitkPoint center, 
						 amide_real_t thickness);
void          amitk_canvas_set_time_on_image    (AmitkCanvas * canvas,
						 gboolean time_on_image);
gint          amitk_canvas_get_width            (AmitkCanvas * canvas);
gint          amitk_canvas_get_height           (AmitkCanvas * canvas);

GdkPixbuf *   amitk_canvas_get_pixbuf           (AmitkCanvas * canvas);

G_END_DECLS


#endif /* __AMITK_CANVAS_H__ */

