/* amitk_simple_canvas.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 *
 * A simple canvas widget that replaces AmitkSimpleCanvas with pure Cairo drawing.
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


#ifndef __AMITK_SIMPLE_CANVAS_H__
#define __AMITK_SIMPLE_CANVAS_H__

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

/* ============ Simple Canvas Types ============ */

/* Forward declarations */
typedef struct _AmitkSimpleCanvas AmitkSimpleCanvas;
typedef struct _AmitkSimpleCanvasClass AmitkSimpleCanvasClass;
typedef struct _AmitkCanvasItem AmitkCanvasItem;
typedef struct _AmitkCanvasPoints AmitkCanvasPoints;

/* Anchor type for text positioning */
typedef enum {
  AMITK_CANVAS_ANCHOR_CENTER,
  AMITK_CANVAS_ANCHOR_NORTH,
  AMITK_CANVAS_ANCHOR_NORTH_WEST,
  AMITK_CANVAS_ANCHOR_NORTH_EAST,
  AMITK_CANVAS_ANCHOR_SOUTH,
  AMITK_CANVAS_ANCHOR_SOUTH_WEST,
  AMITK_CANVAS_ANCHOR_SOUTH_EAST,
  AMITK_CANVAS_ANCHOR_WEST,
  AMITK_CANVAS_ANCHOR_EAST
} AmitkCanvasAnchorType;

/* Item visibility */
typedef enum {
  AMITK_CANVAS_ITEM_VISIBLE,
  AMITK_CANVAS_ITEM_INVISIBLE
} AmitkCanvasItemVisibility;

/* Item types */
typedef enum {
  AMITK_CANVAS_ITEM_TYPE_POLYLINE,
  AMITK_CANVAS_ITEM_TYPE_TEXT,
  AMITK_CANVAS_ITEM_TYPE_IMAGE,
  AMITK_CANVAS_ITEM_TYPE_RECT,
  AMITK_CANVAS_ITEM_TYPE_ELLIPSE
} AmitkCanvasItemType;

/* ============ Points Structure ============ */

struct _AmitkCanvasPoints {
  gint num_points;
  gdouble *coords;  /* x1, y1, x2, y2, ... */
  gint ref_count;
};

AmitkCanvasPoints * amitk_canvas_points_new(gint num_points);
AmitkCanvasPoints * amitk_canvas_points_ref(AmitkCanvasPoints *points);
void amitk_canvas_points_unref(AmitkCanvasPoints *points);

/* ============ Canvas Item ============ */

#define AMITK_TYPE_CANVAS_ITEM            (amitk_canvas_item_get_type())
#define AMITK_CANVAS_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), AMITK_TYPE_CANVAS_ITEM, AmitkCanvasItem))
#define AMITK_IS_CANVAS_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), AMITK_TYPE_CANVAS_ITEM))

typedef struct _AmitkCanvasItemClass AmitkCanvasItemClass;

struct _AmitkCanvasItem {
  GObject parent_instance;

  AmitkCanvasItemType type;
  AmitkCanvasItemVisibility visibility;
  AmitkSimpleCanvas *canvas;
  gdouble x, y;
  gdouble width, height;

  /* Common properties */
  guint32 fill_color_rgba;
  guint32 stroke_color_rgba;
  gdouble line_width;

  /* Type-specific data */
  union {
    struct {
      AmitkCanvasPoints *points;
      gboolean close_path;
      gboolean end_arrow;
      gdouble arrow_length;
      gdouble arrow_tip_length;
      gdouble arrow_width;
    } polyline;

    struct {
      gchar *text;
      AmitkCanvasAnchorType anchor;
      PangoFontDescription *font_desc;
    } text;

    struct {
      GdkPixbuf *pixbuf;
    } image;

    struct {
      gdouble rx, ry;  /* for ellipse: radii; for rect: corner radii */
    } shape;
  } data;

  /* Transform */
  cairo_matrix_t transform;
  gboolean has_transform;
};

struct _AmitkCanvasItemClass {
  GObjectClass parent_class;

  /* Signals */
  gboolean (*button_press_event)   (AmitkCanvasItem *item, AmitkCanvasItem *target, GdkEventButton *event);
  gboolean (*button_release_event) (AmitkCanvasItem *item, AmitkCanvasItem *target, GdkEventButton *event);
  gboolean (*motion_notify_event)  (AmitkCanvasItem *item, AmitkCanvasItem *target, GdkEventMotion *event);
  gboolean (*enter_notify_event)   (AmitkCanvasItem *item, AmitkCanvasItem *target, GdkEventCrossing *event);
  gboolean (*leave_notify_event)   (AmitkCanvasItem *item, AmitkCanvasItem *target, GdkEventCrossing *event);
  gboolean (*scroll_event)         (AmitkCanvasItem *item, AmitkCanvasItem *target, GdkEventScroll *event);
};

GType amitk_canvas_item_get_type(void);

/* Item functions */
void amitk_canvas_item_remove(AmitkCanvasItem *item);
void amitk_canvas_item_set(AmitkCanvasItem *item, const gchar *first_property, ...);
void amitk_canvas_item_translate(AmitkCanvasItem *item, gdouble tx, gdouble ty);
void amitk_canvas_item_set_transform(AmitkCanvasItem *item, const cairo_matrix_t *matrix);
AmitkSimpleCanvas * amitk_canvas_item_get_canvas(AmitkCanvasItem *item);

/* ============ Simple Canvas Widget ============ */

#define AMITK_TYPE_SIMPLE_CANVAS            (amitk_simple_canvas_get_type())
#define AMITK_SIMPLE_CANVAS(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), AMITK_TYPE_SIMPLE_CANVAS, AmitkSimpleCanvas))
#define AMITK_SIMPLE_CANVAS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), AMITK_TYPE_SIMPLE_CANVAS, AmitkSimpleCanvasClass))
#define AMITK_IS_SIMPLE_CANVAS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), AMITK_TYPE_SIMPLE_CANVAS))
#define AMITK_IS_SIMPLE_CANVAS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), AMITK_TYPE_SIMPLE_CANVAS))

struct _AmitkSimpleCanvas {
  GtkDrawingArea parent;

  /* Canvas bounds */
  gdouble x1, y1, x2, y2;
  gdouble scale;

  /* Items on the canvas */
  GList *items;  /* List of AmitkCanvasItem* */

  /* Pointer grab */
  AmitkCanvasItem *grabbed_item;
  guint32 grabbed_time;

  /* For rendering */
  cairo_surface_t *cached_surface;
  gboolean need_redraw;
};

struct _AmitkSimpleCanvasClass {
  GtkDrawingAreaClass parent_class;
};

GType amitk_simple_canvas_get_type(void);
GtkWidget * amitk_simple_canvas_new(void);

/* Canvas configuration */
void amitk_simple_canvas_set_bounds(AmitkSimpleCanvas *canvas,
                                    gdouble x1, gdouble y1,
                                    gdouble x2, gdouble y2);
void amitk_simple_canvas_set_scale(AmitkSimpleCanvas *canvas, gdouble scale);
gdouble amitk_simple_canvas_get_scale(AmitkSimpleCanvas *canvas);

/* Get root item (for compatibility - returns NULL, items are added directly) */
AmitkCanvasItem * amitk_simple_canvas_get_root_item(AmitkSimpleCanvas *canvas);

/* Item creation */
AmitkCanvasItem * amitk_simple_canvas_polyline_new(AmitkSimpleCanvas *canvas,
                                                   gboolean close_path,
                                                   gint num_points,
                                                   ...);
AmitkCanvasItem * amitk_simple_canvas_text_new(AmitkSimpleCanvas *canvas,
                                               const gchar *text,
                                               gdouble x, gdouble y,
                                               gdouble width,
                                               AmitkCanvasAnchorType anchor,
                                               ...);
AmitkCanvasItem * amitk_simple_canvas_image_new(AmitkSimpleCanvas *canvas,
                                                GdkPixbuf *pixbuf,
                                                gdouble x, gdouble y,
                                                ...);
AmitkCanvasItem * amitk_simple_canvas_rect_new(AmitkSimpleCanvas *canvas,
                                               gdouble x, gdouble y,
                                               gdouble width, gdouble height,
                                               ...);
AmitkCanvasItem * amitk_simple_canvas_ellipse_new(AmitkSimpleCanvas *canvas,
                                                  gdouble cx, gdouble cy,
                                                  gdouble rx, gdouble ry,
                                                  ...);

/* Pointer grab */
void amitk_simple_canvas_pointer_grab(AmitkSimpleCanvas *canvas,
                                      AmitkCanvasItem *item,
                                      GdkEventMask event_mask,
                                      GdkCursor *cursor,
                                      guint32 time);
void amitk_simple_canvas_pointer_ungrab(AmitkSimpleCanvas *canvas,
                                        AmitkCanvasItem *item,
                                        guint32 time);

/* Coordinate conversion */
void amitk_simple_canvas_convert_from_item_space(AmitkSimpleCanvas *canvas,
                                                 AmitkCanvasItem *item,
                                                 gdouble *x, gdouble *y);

/* Rendering */
void amitk_simple_canvas_render(AmitkSimpleCanvas *canvas,
                                cairo_t *cr,
                                gdouble x, gdouble y,
                                gdouble width, gdouble height);
void amitk_simple_canvas_request_redraw(AmitkSimpleCanvas *canvas);

/* Utility to get pixbuf from canvas */
GdkPixbuf * amitk_simple_canvas_get_pixbuf(AmitkSimpleCanvas *canvas,
                                           gint xoffset, gint yoffset,
                                           gint width, gint height);

G_END_DECLS

#endif /* __AMITK_SIMPLE_CANVAS_H__ */
