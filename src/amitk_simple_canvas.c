/* amitk_simple_canvas.c
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

#include "amide_config.h"
#include "amitk_simple_canvas.h"
#include <stdarg.h>
#include <string.h>
#include <math.h>

/* ============ Points Implementation ============ */

AmitkCanvasPoints * amitk_canvas_points_new(gint num_points) {
  AmitkCanvasPoints *points = g_new0(AmitkCanvasPoints, 1);
  points->num_points = num_points;
  points->coords = g_new0(gdouble, num_points * 2);
  points->ref_count = 1;
  return points;
}

AmitkCanvasPoints * amitk_canvas_points_ref(AmitkCanvasPoints *points) {
  if (points)
    points->ref_count++;
  return points;
}

void amitk_canvas_points_unref(AmitkCanvasPoints *points) {
  if (points) {
    points->ref_count--;
    if (points->ref_count <= 0) {
      g_free(points->coords);
      g_free(points);
    }
  }
}

/* ============ Canvas Item Implementation ============ */

enum {
  ITEM_SIGNAL_BUTTON_PRESS,
  ITEM_SIGNAL_BUTTON_RELEASE,
  ITEM_SIGNAL_MOTION_NOTIFY,
  ITEM_SIGNAL_ENTER_NOTIFY,
  ITEM_SIGNAL_LEAVE_NOTIFY,
  ITEM_SIGNAL_SCROLL,
  ITEM_LAST_SIGNAL
};

static guint item_signals[ITEM_LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(AmitkCanvasItem, amitk_canvas_item, G_TYPE_OBJECT)

static void amitk_canvas_item_finalize(GObject *object) {
  AmitkCanvasItem *item = AMITK_CANVAS_ITEM(object);

  switch (item->type) {
  case AMITK_CANVAS_ITEM_TYPE_POLYLINE:
    if (item->data.polyline.points)
      amitk_canvas_points_unref(item->data.polyline.points);
    break;
  case AMITK_CANVAS_ITEM_TYPE_TEXT:
    g_free(item->data.text.text);
    if (item->data.text.font_desc)
      pango_font_description_free(item->data.text.font_desc);
    break;
  case AMITK_CANVAS_ITEM_TYPE_IMAGE:
    if (item->data.image.pixbuf)
      g_object_unref(item->data.image.pixbuf);
    break;
  default:
    break;
  }

  G_OBJECT_CLASS(amitk_canvas_item_parent_class)->finalize(object);
}

static void amitk_canvas_item_class_init(AmitkCanvasItemClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->finalize = amitk_canvas_item_finalize;

  /* Define signals matching AmitkSimpleCanvas item signals */
  item_signals[ITEM_SIGNAL_BUTTON_PRESS] =
    g_signal_new("button-press-event",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(AmitkCanvasItemClass, button_press_event),
                 NULL, NULL,
                 NULL,
                 G_TYPE_BOOLEAN, 2,
                 AMITK_TYPE_CANVAS_ITEM,
                 GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  item_signals[ITEM_SIGNAL_BUTTON_RELEASE] =
    g_signal_new("button-release-event",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(AmitkCanvasItemClass, button_release_event),
                 NULL, NULL,
                 NULL,
                 G_TYPE_BOOLEAN, 2,
                 AMITK_TYPE_CANVAS_ITEM,
                 GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  item_signals[ITEM_SIGNAL_MOTION_NOTIFY] =
    g_signal_new("motion-notify-event",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(AmitkCanvasItemClass, motion_notify_event),
                 NULL, NULL,
                 NULL,
                 G_TYPE_BOOLEAN, 2,
                 AMITK_TYPE_CANVAS_ITEM,
                 GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  item_signals[ITEM_SIGNAL_ENTER_NOTIFY] =
    g_signal_new("enter-notify-event",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(AmitkCanvasItemClass, enter_notify_event),
                 NULL, NULL,
                 NULL,
                 G_TYPE_BOOLEAN, 2,
                 AMITK_TYPE_CANVAS_ITEM,
                 GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  item_signals[ITEM_SIGNAL_LEAVE_NOTIFY] =
    g_signal_new("leave-notify-event",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(AmitkCanvasItemClass, leave_notify_event),
                 NULL, NULL,
                 NULL,
                 G_TYPE_BOOLEAN, 2,
                 AMITK_TYPE_CANVAS_ITEM,
                 GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  item_signals[ITEM_SIGNAL_SCROLL] =
    g_signal_new("scroll-event",
                 G_TYPE_FROM_CLASS(klass),
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(AmitkCanvasItemClass, scroll_event),
                 NULL, NULL,
                 NULL,
                 G_TYPE_BOOLEAN, 2,
                 AMITK_TYPE_CANVAS_ITEM,
                 GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
}

static void amitk_canvas_item_init(AmitkCanvasItem *item) {
  item->type = AMITK_CANVAS_ITEM_TYPE_POLYLINE;
  item->visibility = AMITK_CANVAS_ITEM_VISIBLE;
  item->canvas = NULL;
  item->line_width = 1.0;
  item->fill_color_rgba = 0x00000000;  /* transparent */
  item->stroke_color_rgba = 0x000000FF;  /* black */
  item->has_transform = FALSE;
  cairo_matrix_init_identity(&item->transform);
}

static AmitkCanvasItem * canvas_item_new(AmitkSimpleCanvas *canvas, AmitkCanvasItemType type) {
  AmitkCanvasItem *item = g_object_new(AMITK_TYPE_CANVAS_ITEM, NULL);
  item->type = type;
  item->canvas = canvas;

  /* Add to canvas */
  canvas->items = g_list_append(canvas->items, g_object_ref(item));
  canvas->need_redraw = TRUE;

  return item;
}

void amitk_canvas_item_remove(AmitkCanvasItem *item) {
  if (!item) return;
  if (item->canvas) {
    item->canvas->items = g_list_remove(item->canvas->items, item);
    item->canvas->need_redraw = TRUE;
    gtk_widget_queue_draw(GTK_WIDGET(item->canvas));
    item->canvas = NULL;
  }
  g_object_unref(item);
}

/* Helper to parse RGBA from uint32 */
static void rgba_from_uint32(guint32 rgba, gdouble *r, gdouble *g, gdouble *b, gdouble *a) {
  *r = ((rgba >> 24) & 0xFF) / 255.0;
  *g = ((rgba >> 16) & 0xFF) / 255.0;
  *b = ((rgba >> 8) & 0xFF) / 255.0;
  *a = (rgba & 0xFF) / 255.0;
}

void amitk_canvas_item_set(AmitkCanvasItem *item, const gchar *first_property, ...) {
  va_list args;
  const gchar *name;

  if (!item) return;

  va_start(args, first_property);
  name = first_property;

  while (name) {
    if (g_str_equal(name, "visibility")) {
      item->visibility = va_arg(args, AmitkCanvasItemVisibility);
    }
    else if (g_str_equal(name, "fill-color-rgba")) {
      item->fill_color_rgba = va_arg(args, guint32);
    }
    else if (g_str_equal(name, "stroke-color-rgba")) {
      item->stroke_color_rgba = va_arg(args, guint32);
    }
    else if (g_str_equal(name, "line-width")) {
      item->line_width = va_arg(args, gdouble);
    }
    else if (g_str_equal(name, "points")) {
      if (item->type == AMITK_CANVAS_ITEM_TYPE_POLYLINE) {
        AmitkCanvasPoints *points = va_arg(args, AmitkCanvasPoints*);
        if (item->data.polyline.points)
          amitk_canvas_points_unref(item->data.polyline.points);
        item->data.polyline.points = amitk_canvas_points_ref(points);
      }
    }
    else if (g_str_equal(name, "pixbuf")) {
      if (item->type == AMITK_CANVAS_ITEM_TYPE_IMAGE) {
        GdkPixbuf *pixbuf = va_arg(args, GdkPixbuf*);
        if (item->data.image.pixbuf)
          g_object_unref(item->data.image.pixbuf);
        item->data.image.pixbuf = pixbuf ? g_object_ref(pixbuf) : NULL;
      }
    }
    else if (g_str_equal(name, "x")) {
      item->x = va_arg(args, gdouble);
    }
    else if (g_str_equal(name, "y")) {
      item->y = va_arg(args, gdouble);
    }
    else if (g_str_equal(name, "text")) {
      if (item->type == AMITK_CANVAS_ITEM_TYPE_TEXT) {
        const gchar *text = va_arg(args, const gchar*);
        g_free(item->data.text.text);
        item->data.text.text = g_strdup(text);
      }
    }
    else {
      /* Skip unknown property and its value */
      va_arg(args, gpointer);
    }

    name = va_arg(args, const gchar*);
  }

  va_end(args);

  if (item->canvas) {
    item->canvas->need_redraw = TRUE;
    gtk_widget_queue_draw(GTK_WIDGET(item->canvas));
  }
}

void amitk_canvas_item_translate(AmitkCanvasItem *item, gdouble tx, gdouble ty) {
  if (!item) return;
  cairo_matrix_translate(&item->transform, tx, ty);
  item->has_transform = TRUE;
  if (item->canvas) {
    item->canvas->need_redraw = TRUE;
    gtk_widget_queue_draw(GTK_WIDGET(item->canvas));
  }
}

void amitk_canvas_item_set_transform(AmitkCanvasItem *item, const cairo_matrix_t *matrix) {
  if (!item) return;
  if (matrix) {
    item->transform = *matrix;
    item->has_transform = TRUE;
  } else {
    cairo_matrix_init_identity(&item->transform);
    item->has_transform = FALSE;
  }
  if (item->canvas) {
    item->canvas->need_redraw = TRUE;
    gtk_widget_queue_draw(GTK_WIDGET(item->canvas));
  }
}

AmitkSimpleCanvas * amitk_canvas_item_get_canvas(AmitkCanvasItem *item) {
  return item ? item->canvas : NULL;
}

/* ============ Simple Canvas Implementation ============ */

G_DEFINE_TYPE(AmitkSimpleCanvas, amitk_simple_canvas, GTK_TYPE_DRAWING_AREA)

static void amitk_simple_canvas_finalize(GObject *object);
static gboolean amitk_simple_canvas_draw(GtkWidget *widget, cairo_t *cr);
static gboolean amitk_simple_canvas_button_press(GtkWidget *widget, GdkEventButton *event);
static gboolean amitk_simple_canvas_button_release(GtkWidget *widget, GdkEventButton *event);
static gboolean amitk_simple_canvas_motion_notify(GtkWidget *widget, GdkEventMotion *event);
static gboolean amitk_simple_canvas_enter_notify(GtkWidget *widget, GdkEventCrossing *event);
static gboolean amitk_simple_canvas_leave_notify(GtkWidget *widget, GdkEventCrossing *event);
static gboolean amitk_simple_canvas_scroll(GtkWidget *widget, GdkEventScroll *event);

static void amitk_simple_canvas_class_init(AmitkSimpleCanvasClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

  object_class->finalize = amitk_simple_canvas_finalize;
  widget_class->draw = amitk_simple_canvas_draw;
  widget_class->button_press_event = amitk_simple_canvas_button_press;
  widget_class->button_release_event = amitk_simple_canvas_button_release;
  widget_class->motion_notify_event = amitk_simple_canvas_motion_notify;
  widget_class->enter_notify_event = amitk_simple_canvas_enter_notify;
  widget_class->leave_notify_event = amitk_simple_canvas_leave_notify;
  widget_class->scroll_event = amitk_simple_canvas_scroll;
}

static void amitk_simple_canvas_init(AmitkSimpleCanvas *canvas) {
  canvas->x1 = 0;
  canvas->y1 = 0;
  canvas->x2 = 100;
  canvas->y2 = 100;
  canvas->scale = 1.0;
  canvas->items = NULL;
  canvas->grabbed_item = NULL;
  canvas->cached_surface = NULL;
  canvas->need_redraw = TRUE;

  gtk_widget_add_events(GTK_WIDGET(canvas),
                        GDK_BUTTON_PRESS_MASK |
                        GDK_BUTTON_RELEASE_MASK |
                        GDK_POINTER_MOTION_MASK |
                        GDK_ENTER_NOTIFY_MASK |
                        GDK_LEAVE_NOTIFY_MASK |
                        GDK_SCROLL_MASK);
}

static void amitk_simple_canvas_finalize(GObject *object) {
  AmitkSimpleCanvas *canvas = AMITK_SIMPLE_CANVAS(object);

  /* Free all items */
  GList *l;
  for (l = canvas->items; l; l = l->next) {
    AmitkCanvasItem *item = l->data;
    item->canvas = NULL;  /* Prevent double removal */
    g_object_unref(item);
  }
  g_list_free(canvas->items);

  if (canvas->cached_surface)
    cairo_surface_destroy(canvas->cached_surface);

  G_OBJECT_CLASS(amitk_simple_canvas_parent_class)->finalize(object);
}

GtkWidget * amitk_simple_canvas_new(void) {
  return g_object_new(AMITK_TYPE_SIMPLE_CANVAS, NULL);
}

void amitk_simple_canvas_set_bounds(AmitkSimpleCanvas *canvas,
                                    gdouble x1, gdouble y1,
                                    gdouble x2, gdouble y2) {
  g_return_if_fail(AMITK_IS_SIMPLE_CANVAS(canvas));
  canvas->x1 = x1;
  canvas->y1 = y1;
  canvas->x2 = x2;
  canvas->y2 = y2;
  canvas->need_redraw = TRUE;
  gtk_widget_queue_draw(GTK_WIDGET(canvas));
}

void amitk_simple_canvas_set_scale(AmitkSimpleCanvas *canvas, gdouble scale) {
  g_return_if_fail(AMITK_IS_SIMPLE_CANVAS(canvas));
  canvas->scale = scale;
  canvas->need_redraw = TRUE;
  gtk_widget_queue_draw(GTK_WIDGET(canvas));
}

gdouble amitk_simple_canvas_get_scale(AmitkSimpleCanvas *canvas) {
  g_return_val_if_fail(AMITK_IS_SIMPLE_CANVAS(canvas), 1.0);
  return canvas->scale;
}

AmitkCanvasItem * amitk_simple_canvas_get_root_item(AmitkSimpleCanvas *canvas) {
  /* For compatibility - we don't use a root item concept */
  return NULL;
}

/* ============ Item Drawing ============ */

static void draw_arrow_head(cairo_t *cr, gdouble x1, gdouble y1, gdouble x2, gdouble y2,
                           gdouble arrow_length, gdouble arrow_width) {
  gdouble dx = x2 - x1;
  gdouble dy = y2 - y1;
  gdouble len = sqrt(dx*dx + dy*dy);
  if (len < 0.001) return;

  /* Normalize */
  dx /= len;
  dy /= len;

  /* Arrow base point */
  gdouble ax = x2 - dx * arrow_length;
  gdouble ay = y2 - dy * arrow_length;

  /* Perpendicular */
  gdouble px = -dy * arrow_width / 2.0;
  gdouble py = dx * arrow_width / 2.0;

  cairo_move_to(cr, x2, y2);
  cairo_line_to(cr, ax + px, ay + py);
  cairo_line_to(cr, ax - px, ay - py);
  cairo_close_path(cr);
  cairo_fill(cr);
}

static void draw_polyline(cairo_t *cr, AmitkCanvasItem *item, gdouble scale) {
  AmitkCanvasPoints *points = item->data.polyline.points;
  if (!points || points->num_points < 2) return;

  gdouble r, g, b, a;

  cairo_save(cr);

  if (item->has_transform)
    cairo_transform(cr, &item->transform);

  cairo_move_to(cr, points->coords[0], points->coords[1]);
  for (gint i = 1; i < points->num_points; i++) {
    cairo_line_to(cr, points->coords[i*2], points->coords[i*2+1]);
  }

  if (item->data.polyline.close_path) {
    cairo_close_path(cr);

    /* Fill */
    rgba_from_uint32(item->fill_color_rgba, &r, &g, &b, &a);
    if (a > 0.001) {
      cairo_set_source_rgba(cr, r, g, b, a);
      cairo_fill_preserve(cr);
    }
  }

  /* Stroke */
  rgba_from_uint32(item->stroke_color_rgba, &r, &g, &b, &a);
  cairo_set_source_rgba(cr, r, g, b, a);
  cairo_set_line_width(cr, item->line_width);
  cairo_stroke(cr);

  /* Draw arrow if needed */
  if (item->data.polyline.end_arrow && points->num_points >= 2) {
    gint last = points->num_points - 1;
    gdouble x1 = points->coords[(last-1)*2];
    gdouble y1 = points->coords[(last-1)*2+1];
    gdouble x2 = points->coords[last*2];
    gdouble y2 = points->coords[last*2+1];

    rgba_from_uint32(item->stroke_color_rgba, &r, &g, &b, &a);
    cairo_set_source_rgba(cr, r, g, b, a);
    draw_arrow_head(cr, x1, y1, x2, y2,
                    item->data.polyline.arrow_length,
                    item->data.polyline.arrow_width);
  }

  cairo_restore(cr);
}

static void draw_text(cairo_t *cr, AmitkCanvasItem *item, gdouble scale) {
  if (!item->data.text.text) return;

  gdouble r, g, b, a;
  PangoLayout *layout;
  gint width, height;
  gdouble x, y;

  cairo_save(cr);

  if (item->has_transform)
    cairo_transform(cr, &item->transform);

  layout = pango_cairo_create_layout(cr);
  pango_layout_set_text(layout, item->data.text.text, -1);

  if (item->data.text.font_desc)
    pango_layout_set_font_description(layout, item->data.text.font_desc);

  if (item->width > 0)
    pango_layout_set_width(layout, (int)(item->width * PANGO_SCALE));

  pango_layout_get_pixel_size(layout, &width, &height);

  /* Calculate position based on anchor */
  x = item->x;
  y = item->y;

  switch (item->data.text.anchor) {
  case AMITK_CANVAS_ANCHOR_CENTER:
    x -= width / 2.0;
    y -= height / 2.0;
    break;
  case AMITK_CANVAS_ANCHOR_NORTH:
    x -= width / 2.0;
    break;
  case AMITK_CANVAS_ANCHOR_NORTH_WEST:
    /* Default - no adjustment */
    break;
  case AMITK_CANVAS_ANCHOR_NORTH_EAST:
    x -= width;
    break;
  case AMITK_CANVAS_ANCHOR_SOUTH:
    x -= width / 2.0;
    y -= height;
    break;
  case AMITK_CANVAS_ANCHOR_SOUTH_WEST:
    y -= height;
    break;
  case AMITK_CANVAS_ANCHOR_SOUTH_EAST:
    x -= width;
    y -= height;
    break;
  case AMITK_CANVAS_ANCHOR_WEST:
    y -= height / 2.0;
    break;
  case AMITK_CANVAS_ANCHOR_EAST:
    x -= width;
    y -= height / 2.0;
    break;
  }

  rgba_from_uint32(item->fill_color_rgba, &r, &g, &b, &a);
  if (a < 0.001) {
    /* If no fill color specified, use stroke color for text */
    rgba_from_uint32(item->stroke_color_rgba, &r, &g, &b, &a);
  }
  cairo_set_source_rgba(cr, r, g, b, a);

  cairo_move_to(cr, x, y);
  pango_cairo_show_layout(cr, layout);

  g_object_unref(layout);
  cairo_restore(cr);
}

static void draw_image(cairo_t *cr, AmitkCanvasItem *item, gdouble scale) {
  if (!item->data.image.pixbuf) return;

  cairo_save(cr);

  if (item->has_transform)
    cairo_transform(cr, &item->transform);

  gdk_cairo_set_source_pixbuf(cr, item->data.image.pixbuf, item->x, item->y);
  cairo_paint(cr);

  cairo_restore(cr);
}

static void draw_rect(cairo_t *cr, AmitkCanvasItem *item, gdouble scale) {
  gdouble r, g, b, a;

  cairo_save(cr);

  if (item->has_transform)
    cairo_transform(cr, &item->transform);

  cairo_rectangle(cr, item->x, item->y, item->width, item->height);

  /* Fill */
  rgba_from_uint32(item->fill_color_rgba, &r, &g, &b, &a);
  if (a > 0.001) {
    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_fill_preserve(cr);
  }

  /* Stroke */
  rgba_from_uint32(item->stroke_color_rgba, &r, &g, &b, &a);
  cairo_set_source_rgba(cr, r, g, b, a);
  cairo_set_line_width(cr, item->line_width);
  cairo_stroke(cr);

  cairo_restore(cr);
}

static void draw_ellipse(cairo_t *cr, AmitkCanvasItem *item, gdouble scale) {
  gdouble r, g, b, a;
  gdouble cx = item->x;
  gdouble cy = item->y;
  gdouble rx = item->data.shape.rx;
  gdouble ry = item->data.shape.ry;

  cairo_save(cr);

  if (item->has_transform)
    cairo_transform(cr, &item->transform);

  /* Draw ellipse using arc and scaling */
  cairo_save(cr);
  cairo_translate(cr, cx, cy);
  cairo_scale(cr, rx, ry);
  cairo_arc(cr, 0, 0, 1.0, 0, 2 * G_PI);
  cairo_restore(cr);

  /* Fill */
  rgba_from_uint32(item->fill_color_rgba, &r, &g, &b, &a);
  if (a > 0.001) {
    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_fill_preserve(cr);
  }

  /* Stroke */
  rgba_from_uint32(item->stroke_color_rgba, &r, &g, &b, &a);
  cairo_set_source_rgba(cr, r, g, b, a);
  cairo_set_line_width(cr, item->line_width);
  cairo_stroke(cr);

  cairo_restore(cr);
}

static void draw_item(cairo_t *cr, AmitkCanvasItem *item, gdouble scale) {
  if (item->visibility != AMITK_CANVAS_ITEM_VISIBLE) return;

  switch (item->type) {
  case AMITK_CANVAS_ITEM_TYPE_POLYLINE:
    draw_polyline(cr, item, scale);
    break;
  case AMITK_CANVAS_ITEM_TYPE_TEXT:
    draw_text(cr, item, scale);
    break;
  case AMITK_CANVAS_ITEM_TYPE_IMAGE:
    draw_image(cr, item, scale);
    break;
  case AMITK_CANVAS_ITEM_TYPE_RECT:
    draw_rect(cr, item, scale);
    break;
  case AMITK_CANVAS_ITEM_TYPE_ELLIPSE:
    draw_ellipse(cr, item, scale);
    break;
  }
}

static gboolean amitk_simple_canvas_draw(GtkWidget *widget, cairo_t *cr) {
  AmitkSimpleCanvas *canvas = AMITK_SIMPLE_CANVAS(widget);
  GList *l;

  /* Apply scale */
  cairo_scale(cr, canvas->scale, canvas->scale);

  /* Draw all items */
  for (l = canvas->items; l; l = l->next) {
    draw_item(cr, (AmitkCanvasItem*)l->data, canvas->scale);
  }

  return FALSE;
}

/* ============ Event Handlers ============ */

/* Find item at position - for all item types */
static AmitkCanvasItem * find_item_at(AmitkSimpleCanvas *canvas, gdouble x, gdouble y) {
  /* Return grabbed item if there is one */
  if (canvas->grabbed_item)
    return canvas->grabbed_item;

  /* Simple hit testing - iterate in reverse order (top to bottom) */
  GList *l;
  for (l = g_list_last(canvas->items); l; l = l->prev) {
    AmitkCanvasItem *item = l->data;
    if (item->visibility != AMITK_CANVAS_ITEM_VISIBLE)
      continue;

    /* Simple bounding box test */
    gdouble ix, iy, iw, ih;
    switch (item->type) {
    case AMITK_CANVAS_ITEM_TYPE_RECT:
      ix = item->x; iy = item->y;
      iw = item->width; ih = item->height;
      if (x >= ix && x <= ix + iw && y >= iy && y <= iy + ih)
        return item;
      break;
    case AMITK_CANVAS_ITEM_TYPE_ELLIPSE:
      ix = item->x - item->data.shape.rx;
      iy = item->y - item->data.shape.ry;
      iw = item->data.shape.rx * 2;
      ih = item->data.shape.ry * 2;
      if (x >= ix && x <= ix + iw && y >= iy && y <= iy + ih)
        return item;
      break;
    case AMITK_CANVAS_ITEM_TYPE_IMAGE:
      if (item->data.image.pixbuf) {
        ix = item->x; iy = item->y;
        iw = gdk_pixbuf_get_width(item->data.image.pixbuf);
        ih = gdk_pixbuf_get_height(item->data.image.pixbuf);
        if (x >= ix && x <= ix + iw && y >= iy && y <= iy + ih)
          return item;
      }
      break;
    case AMITK_CANVAS_ITEM_TYPE_POLYLINE:
      /* For polylines, check if point is within tolerance of any line segment */
      if (item->data.polyline.points && item->data.polyline.points->num_points >= 2) {
        AmitkCanvasPoints *pts = item->data.polyline.points;
        gdouble tolerance = item->line_width + 3.0;
        for (int i = 0; i < pts->num_points - 1; i++) {
          gdouble x1 = pts->coords[i*2], y1 = pts->coords[i*2+1];
          gdouble x2 = pts->coords[(i+1)*2], y2 = pts->coords[(i+1)*2+1];
          /* Point to line segment distance */
          gdouble dx = x2 - x1, dy = y2 - y1;
          gdouble len_sq = dx*dx + dy*dy;
          gdouble t = 0;
          if (len_sq > 0.0001)
            t = CLAMP(((x - x1)*dx + (y - y1)*dy) / len_sq, 0.0, 1.0);
          gdouble px = x1 + t * dx, py = y1 + t * dy;
          gdouble dist = sqrt((x - px)*(x - px) + (y - py)*(y - py));
          if (dist <= tolerance)
            return item;
        }
      }
      break;
    case AMITK_CANVAS_ITEM_TYPE_TEXT:
      /* For text, we'd need to compute the actual bounds - simplified here */
      ix = item->x - 50;  /* rough estimate */
      iy = item->y - 10;
      iw = 100;
      ih = 20;
      if (x >= ix && x <= ix + iw && y >= iy && y <= iy + ih)
        return item;
      break;
    }
  }

  return NULL;
}

/* Get first item with event handlers connected (used as fallback target) */
static AmitkCanvasItem * find_first_connectable_item(AmitkSimpleCanvas *canvas) {
  GList *l;
  for (l = canvas->items; l; l = l->next) {
    AmitkCanvasItem *item = l->data;
    if (item->visibility == AMITK_CANVAS_ITEM_VISIBLE)
      return item;
  }
  return NULL;
}

static gboolean amitk_simple_canvas_button_press(GtkWidget *widget, GdkEventButton *event) {
  AmitkSimpleCanvas *canvas = AMITK_SIMPLE_CANVAS(widget);
  gdouble x = event->x / canvas->scale;
  gdouble y = event->y / canvas->scale;
  gboolean handled = FALSE;

  AmitkCanvasItem *item = find_item_at(canvas, x, y);
  AmitkCanvasItem *target = item;

  /* If no specific item found, try first item as target (for canvas-wide events) */
  if (!item) {
    item = find_first_connectable_item(canvas);
  }

  /* Emit signal on item */
  if (item) {
    g_signal_emit(item, item_signals[ITEM_SIGNAL_BUTTON_PRESS], 0, target, event, &handled);
  }

  return handled;
}

static gboolean amitk_simple_canvas_button_release(GtkWidget *widget, GdkEventButton *event) {
  AmitkSimpleCanvas *canvas = AMITK_SIMPLE_CANVAS(widget);
  gdouble x = event->x / canvas->scale;
  gdouble y = event->y / canvas->scale;
  gboolean handled = FALSE;

  AmitkCanvasItem *item = find_item_at(canvas, x, y);
  AmitkCanvasItem *target = item;

  if (!item) {
    item = find_first_connectable_item(canvas);
  }

  if (item) {
    g_signal_emit(item, item_signals[ITEM_SIGNAL_BUTTON_RELEASE], 0, target, event, &handled);
  }

  return handled;
}

static gboolean amitk_simple_canvas_motion_notify(GtkWidget *widget, GdkEventMotion *event) {
  AmitkSimpleCanvas *canvas = AMITK_SIMPLE_CANVAS(widget);
  gdouble x = event->x / canvas->scale;
  gdouble y = event->y / canvas->scale;
  gboolean handled = FALSE;

  AmitkCanvasItem *item = find_item_at(canvas, x, y);
  AmitkCanvasItem *target = item;

  if (!item) {
    item = find_first_connectable_item(canvas);
  }

  if (item) {
    g_signal_emit(item, item_signals[ITEM_SIGNAL_MOTION_NOTIFY], 0, target, event, &handled);
  }

  return handled;
}

static gboolean amitk_simple_canvas_enter_notify(GtkWidget *widget, GdkEventCrossing *event) {
  AmitkSimpleCanvas *canvas = AMITK_SIMPLE_CANVAS(widget);
  gboolean handled = FALSE;

  AmitkCanvasItem *item = find_first_connectable_item(canvas);

  if (item) {
    g_signal_emit(item, item_signals[ITEM_SIGNAL_ENTER_NOTIFY], 0, NULL, event, &handled);
  }

  return handled;
}

static gboolean amitk_simple_canvas_leave_notify(GtkWidget *widget, GdkEventCrossing *event) {
  AmitkSimpleCanvas *canvas = AMITK_SIMPLE_CANVAS(widget);
  gboolean handled = FALSE;

  AmitkCanvasItem *item = find_first_connectable_item(canvas);

  if (item) {
    g_signal_emit(item, item_signals[ITEM_SIGNAL_LEAVE_NOTIFY], 0, NULL, event, &handled);
  }

  return handled;
}

static gboolean amitk_simple_canvas_scroll(GtkWidget *widget, GdkEventScroll *event) {
  AmitkSimpleCanvas *canvas = AMITK_SIMPLE_CANVAS(widget);
  gdouble x = event->x / canvas->scale;
  gdouble y = event->y / canvas->scale;
  gboolean handled = FALSE;

  AmitkCanvasItem *item = find_item_at(canvas, x, y);
  AmitkCanvasItem *target = item;

  if (!item) {
    item = find_first_connectable_item(canvas);
  }

  if (item) {
    g_signal_emit(item, item_signals[ITEM_SIGNAL_SCROLL], 0, target, event, &handled);
  }

  return handled;
}

/* ============ Item Creation Functions ============ */

/* Helper to parse varargs property list */
static void parse_item_properties(AmitkCanvasItem *item, va_list args) {
  const gchar *name;

  while ((name = va_arg(args, const gchar*)) != NULL) {
    if (g_str_equal(name, "fill-color-rgba")) {
      item->fill_color_rgba = va_arg(args, guint32);
    }
    else if (g_str_equal(name, "stroke-color-rgba")) {
      item->stroke_color_rgba = va_arg(args, guint32);
    }
    else if (g_str_equal(name, "fill-color") || g_str_equal(name, "stroke-color")) {
      const gchar *color_name = va_arg(args, const gchar*);
      GdkRGBA rgba;
      if (gdk_rgba_parse(&rgba, color_name)) {
        guint32 c = ((guint32)(rgba.red * 255) << 24) |
                    ((guint32)(rgba.green * 255) << 16) |
                    ((guint32)(rgba.blue * 255) << 8) |
                    ((guint32)(rgba.alpha * 255));
        if (g_str_equal(name, "fill-color"))
          item->fill_color_rgba = c;
        else
          item->stroke_color_rgba = c;
      }
    }
    else if (g_str_equal(name, "line-width")) {
      item->line_width = va_arg(args, gdouble);
    }
    else if (g_str_equal(name, "points")) {
      if (item->type == AMITK_CANVAS_ITEM_TYPE_POLYLINE) {
        AmitkCanvasPoints *points = va_arg(args, AmitkCanvasPoints*);
        if (item->data.polyline.points)
          amitk_canvas_points_unref(item->data.polyline.points);
        item->data.polyline.points = amitk_canvas_points_ref(points);
      }
    }
    else if (g_str_equal(name, "end-arrow")) {
      if (item->type == AMITK_CANVAS_ITEM_TYPE_POLYLINE) {
        item->data.polyline.end_arrow = va_arg(args, gboolean);
      }
    }
    else if (g_str_equal(name, "arrow-length")) {
      if (item->type == AMITK_CANVAS_ITEM_TYPE_POLYLINE) {
        item->data.polyline.arrow_length = va_arg(args, gdouble);
      }
    }
    else if (g_str_equal(name, "arrow-tip-length")) {
      if (item->type == AMITK_CANVAS_ITEM_TYPE_POLYLINE) {
        item->data.polyline.arrow_tip_length = va_arg(args, gdouble);
      }
    }
    else if (g_str_equal(name, "arrow-width")) {
      if (item->type == AMITK_CANVAS_ITEM_TYPE_POLYLINE) {
        item->data.polyline.arrow_width = va_arg(args, gdouble);
      }
    }
    else if (g_str_equal(name, "font-desc")) {
      if (item->type == AMITK_CANVAS_ITEM_TYPE_TEXT) {
        PangoFontDescription *fd = va_arg(args, PangoFontDescription*);
        if (item->data.text.font_desc)
          pango_font_description_free(item->data.text.font_desc);
        item->data.text.font_desc = fd ? pango_font_description_copy(fd) : NULL;
      }
    }
    else {
      /* Skip unknown property and its value */
      va_arg(args, gpointer);
    }
  }
}

AmitkCanvasItem * amitk_simple_canvas_polyline_new(AmitkSimpleCanvas *canvas,
                                                   gboolean close_path,
                                                   gint num_points,
                                                   ...) {
  g_return_val_if_fail(AMITK_IS_SIMPLE_CANVAS(canvas), NULL);

  AmitkCanvasItem *item = canvas_item_new(canvas, AMITK_CANVAS_ITEM_TYPE_POLYLINE);
  item->data.polyline.close_path = close_path;
  item->data.polyline.points = NULL;
  item->data.polyline.end_arrow = FALSE;
  item->data.polyline.arrow_length = 10.0;
  item->data.polyline.arrow_tip_length = 10.0;
  item->data.polyline.arrow_width = 8.0;

  va_list args;
  va_start(args, num_points);
  parse_item_properties(item, args);
  va_end(args);

  gtk_widget_queue_draw(GTK_WIDGET(canvas));
  return item;
}

AmitkCanvasItem * amitk_simple_canvas_text_new(AmitkSimpleCanvas *canvas,
                                               const gchar *text,
                                               gdouble x, gdouble y,
                                               gdouble width,
                                               AmitkCanvasAnchorType anchor,
                                               ...) {
  g_return_val_if_fail(AMITK_IS_SIMPLE_CANVAS(canvas), NULL);

  AmitkCanvasItem *item = canvas_item_new(canvas, AMITK_CANVAS_ITEM_TYPE_TEXT);
  item->x = x;
  item->y = y;
  item->width = width;
  item->data.text.text = g_strdup(text);
  item->data.text.anchor = anchor;
  item->data.text.font_desc = NULL;

  /* Default text color to black */
  item->fill_color_rgba = 0x000000FF;

  va_list args;
  va_start(args, anchor);
  parse_item_properties(item, args);
  va_end(args);

  gtk_widget_queue_draw(GTK_WIDGET(canvas));
  return item;
}

AmitkCanvasItem * amitk_simple_canvas_image_new(AmitkSimpleCanvas *canvas,
                                                GdkPixbuf *pixbuf,
                                                gdouble x, gdouble y,
                                                ...) {
  g_return_val_if_fail(AMITK_IS_SIMPLE_CANVAS(canvas), NULL);

  AmitkCanvasItem *item = canvas_item_new(canvas, AMITK_CANVAS_ITEM_TYPE_IMAGE);
  item->x = x;
  item->y = y;
  item->data.image.pixbuf = pixbuf ? g_object_ref(pixbuf) : NULL;

  va_list args;
  va_start(args, y);
  parse_item_properties(item, args);
  va_end(args);

  gtk_widget_queue_draw(GTK_WIDGET(canvas));
  return item;
}

AmitkCanvasItem * amitk_simple_canvas_rect_new(AmitkSimpleCanvas *canvas,
                                               gdouble x, gdouble y,
                                               gdouble width, gdouble height,
                                               ...) {
  g_return_val_if_fail(AMITK_IS_SIMPLE_CANVAS(canvas), NULL);

  AmitkCanvasItem *item = canvas_item_new(canvas, AMITK_CANVAS_ITEM_TYPE_RECT);
  item->x = x;
  item->y = y;
  item->width = width;
  item->height = height;

  va_list args;
  va_start(args, height);
  parse_item_properties(item, args);
  va_end(args);

  gtk_widget_queue_draw(GTK_WIDGET(canvas));
  return item;
}

AmitkCanvasItem * amitk_simple_canvas_ellipse_new(AmitkSimpleCanvas *canvas,
                                                  gdouble cx, gdouble cy,
                                                  gdouble rx, gdouble ry,
                                                  ...) {
  g_return_val_if_fail(AMITK_IS_SIMPLE_CANVAS(canvas), NULL);

  AmitkCanvasItem *item = canvas_item_new(canvas, AMITK_CANVAS_ITEM_TYPE_ELLIPSE);
  item->x = cx;
  item->y = cy;
  item->data.shape.rx = rx;
  item->data.shape.ry = ry;

  va_list args;
  va_start(args, ry);
  parse_item_properties(item, args);
  va_end(args);

  gtk_widget_queue_draw(GTK_WIDGET(canvas));
  return item;
}

/* ============ Pointer Grab ============ */

void amitk_simple_canvas_pointer_grab(AmitkSimpleCanvas *canvas,
                                      AmitkCanvasItem *item,
                                      GdkEventMask event_mask,
                                      GdkCursor *cursor,
                                      guint32 time) {
  g_return_if_fail(AMITK_IS_SIMPLE_CANVAS(canvas));

  canvas->grabbed_item = item;
  canvas->grabbed_time = time;

  /* Set cursor if provided */
  if (cursor) {
    GdkWindow *window = gtk_widget_get_window(GTK_WIDGET(canvas));
    if (window)
      gdk_window_set_cursor(window, cursor);
  }
}

void amitk_simple_canvas_pointer_ungrab(AmitkSimpleCanvas *canvas,
                                        AmitkCanvasItem *item,
                                        guint32 time) {
  g_return_if_fail(AMITK_IS_SIMPLE_CANVAS(canvas));

  if (canvas->grabbed_item == item) {
    canvas->grabbed_item = NULL;

    /* Reset cursor */
    GdkWindow *window = gtk_widget_get_window(GTK_WIDGET(canvas));
    if (window)
      gdk_window_set_cursor(window, NULL);
  }
}

/* ============ Coordinate Conversion ============ */

void amitk_simple_canvas_convert_from_item_space(AmitkSimpleCanvas *canvas,
                                                 AmitkCanvasItem *item,
                                                 gdouble *x, gdouble *y) {
  g_return_if_fail(AMITK_IS_SIMPLE_CANVAS(canvas));

  if (item && item->has_transform) {
    cairo_matrix_transform_point(&item->transform, x, y);
  }
}

/* ============ Rendering ============ */

void amitk_simple_canvas_render(AmitkSimpleCanvas *canvas,
                                cairo_t *cr,
                                gdouble x, gdouble y,
                                gdouble width, gdouble height) {
  g_return_if_fail(AMITK_IS_SIMPLE_CANVAS(canvas));

  cairo_save(cr);
  cairo_translate(cr, -x, -y);
  cairo_scale(cr, canvas->scale, canvas->scale);

  GList *l;
  for (l = canvas->items; l; l = l->next) {
    draw_item(cr, (AmitkCanvasItem*)l->data, canvas->scale);
  }

  cairo_restore(cr);
}

void amitk_simple_canvas_request_redraw(AmitkSimpleCanvas *canvas) {
  g_return_if_fail(AMITK_IS_SIMPLE_CANVAS(canvas));
  canvas->need_redraw = TRUE;
  gtk_widget_queue_draw(GTK_WIDGET(canvas));
}

GdkPixbuf * amitk_simple_canvas_get_pixbuf(AmitkSimpleCanvas *canvas,
                                           gint xoffset, gint yoffset,
                                           gint width, gint height) {
  g_return_val_if_fail(AMITK_IS_SIMPLE_CANVAS(canvas), NULL);

  cairo_surface_t *surf = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
  cairo_t *cr = cairo_create(surf);

  /* Get background color from widget style */
  GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(canvas));
  GtkStyleContext *ctxt = gtk_widget_get_style_context(toplevel);
  GdkRGBA *bg;
  gtk_style_context_get(ctxt, GTK_STATE_FLAG_NORMAL,
                        GTK_STYLE_PROPERTY_BACKGROUND_COLOR, &bg, NULL);
  gdk_cairo_set_source_rgba(cr, bg);
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_fill(cr);
  gdk_rgba_free(bg);

  /* Render canvas */
  amitk_simple_canvas_render(canvas, cr, xoffset, yoffset, width, height);

  cairo_surface_flush(surf);
  GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface(surf, 0, 0, width, height);

  cairo_destroy(cr);
  cairo_surface_destroy(surf);

  return pixbuf;
}
