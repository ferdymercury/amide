/* amitk_canvas_compat.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 *
 * Implementation of AmitkSimpleCanvas compatibility layer functions.
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
#include "amitk_canvas_compat.h"
#include <stdarg.h>
#include <string.h>

/* Thread-local canvas context */
__thread AmitkSimpleCanvas *_amitk_current_canvas = NULL;

/* Helper to get canvas from parent parameter */
static AmitkSimpleCanvas * get_canvas_from_parent(AmitkCanvasItem *parent) {
  if (parent != NULL) {
    return amitk_canvas_item_get_canvas(parent);
  }
  return _amitk_current_canvas;
}

/* Helper to parse color from string */
static guint32 parse_color_string(const gchar *color_name) {
  GdkRGBA rgba = {0, 0, 0, 1};
  if (color_name && gdk_rgba_parse(&rgba, color_name)) {
    return ((guint32)(rgba.red * 255) << 24) |
           ((guint32)(rgba.green * 255) << 16) |
           ((guint32)(rgba.blue * 255) << 8) |
           ((guint32)(rgba.alpha * 255));
  }
  return 0x000000FF;  /* Default to black */
}

/* Polyline creation with varargs property parsing */
AmitkCanvasItem * amitk_canvas_polyline_new(AmitkCanvasItem *parent_or_root,
                                        gboolean close_path,
                                        gint num_points,
                                        ...) {
  AmitkSimpleCanvas *canvas = get_canvas_from_parent(parent_or_root);
  g_return_val_if_fail(canvas != NULL, NULL);

  va_list args;
  va_start(args, num_points);

  /* Create the item */
  AmitkCanvasItem *item = amitk_simple_canvas_polyline_new(canvas, close_path, 0, NULL);

  /* Parse properties */
  const gchar *name;
  while ((name = va_arg(args, const gchar*)) != NULL) {
    if (g_str_equal(name, "points")) {
      AmitkCanvasPoints *points = va_arg(args, AmitkCanvasPoints*);
      amitk_canvas_item_set(item, "points", points, NULL);
    }
    else if (g_str_equal(name, "fill-color-rgba")) {
      guint32 rgba = va_arg(args, guint32);
      item->fill_color_rgba = rgba;
    }
    else if (g_str_equal(name, "stroke-color-rgba")) {
      guint32 rgba = va_arg(args, guint32);
      item->stroke_color_rgba = rgba;
    }
    else if (g_str_equal(name, "fill-color")) {
      const gchar *color = va_arg(args, const gchar*);
      item->fill_color_rgba = parse_color_string(color);
    }
    else if (g_str_equal(name, "stroke-color")) {
      const gchar *color = va_arg(args, const gchar*);
      item->stroke_color_rgba = parse_color_string(color);
    }
    else if (g_str_equal(name, "line-width")) {
      item->line_width = va_arg(args, gdouble);
    }
    else if (g_str_equal(name, "end-arrow")) {
      item->data.polyline.end_arrow = va_arg(args, gboolean);
    }
    else if (g_str_equal(name, "arrow-length")) {
      item->data.polyline.arrow_length = va_arg(args, gdouble);
    }
    else if (g_str_equal(name, "arrow-tip-length")) {
      item->data.polyline.arrow_tip_length = va_arg(args, gdouble);
    }
    else if (g_str_equal(name, "arrow-width")) {
      item->data.polyline.arrow_width = va_arg(args, gdouble);
    }
    else {
      /* Skip unknown property value */
      va_arg(args, gpointer);
    }
  }

  va_end(args);
  return item;
}

/* Text creation */
AmitkCanvasItem * amitk_canvas_text_new(AmitkCanvasItem *parent_or_root,
                                    const gchar *text,
                                    gdouble x, gdouble y,
                                    gdouble width,
                                    AmitkCanvasAnchorType anchor,
                                    ...) {
  AmitkSimpleCanvas *canvas = get_canvas_from_parent(parent_or_root);
  g_return_val_if_fail(canvas != NULL, NULL);

  va_list args;
  va_start(args, anchor);

  /* Create the item */
  AmitkCanvasItem *item = amitk_simple_canvas_text_new(canvas, text, x, y, width, anchor, NULL);

  /* Parse properties */
  const gchar *name;
  while ((name = va_arg(args, const gchar*)) != NULL) {
    if (g_str_equal(name, "fill-color-rgba")) {
      guint32 rgba = va_arg(args, guint32);
      item->fill_color_rgba = rgba;
    }
    else if (g_str_equal(name, "fill-color")) {
      const gchar *color = va_arg(args, const gchar*);
      item->fill_color_rgba = parse_color_string(color);
    }
    else if (g_str_equal(name, "font-desc")) {
      PangoFontDescription *fd = va_arg(args, PangoFontDescription*);
      if (item->data.text.font_desc)
        pango_font_description_free(item->data.text.font_desc);
      item->data.text.font_desc = fd ? pango_font_description_copy(fd) : NULL;
    }
    else if (g_str_equal(name, "font")) {
      const gchar *font = va_arg(args, const gchar*);
      if (item->data.text.font_desc)
        pango_font_description_free(item->data.text.font_desc);
      item->data.text.font_desc = pango_font_description_from_string(font);
    }
    else {
      /* Skip unknown property value */
      va_arg(args, gpointer);
    }
  }

  va_end(args);
  return item;
}

/* Image creation */
AmitkCanvasItem * amitk_canvas_image_new(AmitkCanvasItem *parent_or_root,
                                     GdkPixbuf *pixbuf,
                                     gdouble x, gdouble y,
                                     ...) {
  AmitkSimpleCanvas *canvas = get_canvas_from_parent(parent_or_root);
  g_return_val_if_fail(canvas != NULL, NULL);

  va_list args;
  va_start(args, y);

  /* Create the item */
  AmitkCanvasItem *item = amitk_simple_canvas_image_new(canvas, pixbuf, x, y, NULL);

  /* Parse any additional properties (images usually don't have many) */
  const gchar *name;
  while ((name = va_arg(args, const gchar*)) != NULL) {
    /* Skip unknown properties */
    va_arg(args, gpointer);
  }

  va_end(args);
  return item;
}

/* Rectangle creation */
AmitkCanvasItem * amitk_canvas_rect_new(AmitkCanvasItem *parent_or_root,
                                    gdouble x, gdouble y,
                                    gdouble width, gdouble height,
                                    ...) {
  AmitkSimpleCanvas *canvas = get_canvas_from_parent(parent_or_root);
  g_return_val_if_fail(canvas != NULL, NULL);

  va_list args;
  va_start(args, height);

  /* Create the item */
  AmitkCanvasItem *item = amitk_simple_canvas_rect_new(canvas, x, y, width, height, NULL);

  /* Parse properties */
  const gchar *name;
  while ((name = va_arg(args, const gchar*)) != NULL) {
    if (g_str_equal(name, "fill-color-rgba")) {
      guint32 rgba = va_arg(args, guint32);
      item->fill_color_rgba = rgba;
    }
    else if (g_str_equal(name, "stroke-color-rgba")) {
      guint32 rgba = va_arg(args, guint32);
      item->stroke_color_rgba = rgba;
    }
    else if (g_str_equal(name, "fill-color")) {
      const gchar *color = va_arg(args, const gchar*);
      item->fill_color_rgba = parse_color_string(color);
    }
    else if (g_str_equal(name, "stroke-color")) {
      const gchar *color = va_arg(args, const gchar*);
      item->stroke_color_rgba = parse_color_string(color);
    }
    else if (g_str_equal(name, "line-width")) {
      item->line_width = va_arg(args, gdouble);
    }
    else {
      /* Skip unknown property value */
      va_arg(args, gpointer);
    }
  }

  va_end(args);
  return item;
}

/* Ellipse creation */
AmitkCanvasItem * amitk_canvas_ellipse_new(AmitkCanvasItem *parent_or_root,
                                       gdouble cx, gdouble cy,
                                       gdouble rx, gdouble ry,
                                       ...) {
  AmitkSimpleCanvas *canvas = get_canvas_from_parent(parent_or_root);
  g_return_val_if_fail(canvas != NULL, NULL);

  va_list args;
  va_start(args, ry);

  /* Create the item */
  AmitkCanvasItem *item = amitk_simple_canvas_ellipse_new(canvas, cx, cy, rx, ry, NULL);

  /* Parse properties */
  const gchar *name;
  while ((name = va_arg(args, const gchar*)) != NULL) {
    if (g_str_equal(name, "fill-color-rgba")) {
      guint32 rgba = va_arg(args, guint32);
      item->fill_color_rgba = rgba;
    }
    else if (g_str_equal(name, "stroke-color-rgba")) {
      guint32 rgba = va_arg(args, guint32);
      item->stroke_color_rgba = rgba;
    }
    else if (g_str_equal(name, "fill-color")) {
      const gchar *color = va_arg(args, const gchar*);
      item->fill_color_rgba = parse_color_string(color);
    }
    else if (g_str_equal(name, "stroke-color")) {
      const gchar *color = va_arg(args, const gchar*);
      item->stroke_color_rgba = parse_color_string(color);
    }
    else if (g_str_equal(name, "line-width")) {
      item->line_width = va_arg(args, gdouble);
    }
    else {
      /* Skip unknown property value */
      va_arg(args, gpointer);
    }
  }

  va_end(args);
  return item;
}

/* Generic property setter for canvas items - handles g_object_set-style calls */
void amitk_canvas_item_set_properties(AmitkCanvasItem *item, const gchar *first_prop, ...) {
  g_return_if_fail(AMITK_IS_CANVAS_ITEM(item));

  if (first_prop == NULL)
    return;

  va_list args;
  va_start(args, first_prop);

  const gchar *name = first_prop;
  while (name != NULL) {
    if (g_str_equal(name, "points")) {
      AmitkCanvasPoints *points = va_arg(args, AmitkCanvasPoints*);
      if (item->type == AMITK_CANVAS_ITEM_TYPE_POLYLINE && item->data.polyline.points) {
        amitk_canvas_points_unref(item->data.polyline.points);
      }
      if (item->type == AMITK_CANVAS_ITEM_TYPE_POLYLINE) {
        item->data.polyline.points = points ? amitk_canvas_points_ref(points) : NULL;
      }
    }
    else if (g_str_equal(name, "pixbuf")) {
      GdkPixbuf *pixbuf = va_arg(args, GdkPixbuf*);
      if (item->type == AMITK_CANVAS_ITEM_TYPE_IMAGE) {
        if (item->data.image.pixbuf)
          g_object_unref(item->data.image.pixbuf);
        item->data.image.pixbuf = pixbuf ? g_object_ref(pixbuf) : NULL;
      }
    }
    else if (g_str_equal(name, "visibility")) {
      gint vis = va_arg(args, gint);
      item->visibility = (vis == AMITK_CANVAS_ITEM_VISIBLE) ? AMITK_CANVAS_ITEM_VISIBLE : AMITK_CANVAS_ITEM_INVISIBLE;
    }
    else if (g_str_equal(name, "fill-color-rgba")) {
      guint32 rgba = va_arg(args, guint32);
      item->fill_color_rgba = rgba;
    }
    else if (g_str_equal(name, "stroke-color-rgba")) {
      guint32 rgba = va_arg(args, guint32);
      item->stroke_color_rgba = rgba;
    }
    else if (g_str_equal(name, "fill-color")) {
      const gchar *color = va_arg(args, const gchar*);
      item->fill_color_rgba = parse_color_string(color);
    }
    else if (g_str_equal(name, "stroke-color")) {
      const gchar *color = va_arg(args, const gchar*);
      item->stroke_color_rgba = parse_color_string(color);
    }
    else if (g_str_equal(name, "line-width")) {
      item->line_width = va_arg(args, gdouble);
    }
    else if (g_str_equal(name, "x")) {
      gdouble xv = va_arg(args, gdouble);
      item->x = xv;  /* Common x property on item */
    }
    else if (g_str_equal(name, "y")) {
      gdouble yv = va_arg(args, gdouble);
      item->y = yv;  /* Common y property on item */
    }
    else if (g_str_equal(name, "width")) {
      gdouble w = va_arg(args, gdouble);
      item->width = w;
    }
    else if (g_str_equal(name, "height")) {
      gdouble h = va_arg(args, gdouble);
      item->height = h;
    }
    else if (g_str_equal(name, "text")) {
      const gchar *text = va_arg(args, const gchar*);
      if (item->type == AMITK_CANVAS_ITEM_TYPE_TEXT) {
        g_free(item->data.text.text);
        item->data.text.text = g_strdup(text);
      }
    }
    else {
      /* Skip unknown property value */
      va_arg(args, gpointer);
    }

    /* Get next property name */
    name = va_arg(args, const gchar*);
  }

  va_end(args);

  /* Request redraw */
  if (item->canvas) {
    gtk_widget_queue_draw(GTK_WIDGET(item->canvas));
  }
}
