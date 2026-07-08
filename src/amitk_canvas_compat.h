/* amitk_canvas_compat.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 *
 * Compatibility layer for canvas operations.
 * Provides helper functions and the AmitkCanvasBounds struct.
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

#ifndef __AMITK_CANVAS_COMPAT_H__
#define __AMITK_CANVAS_COMPAT_H__

#include "amitk_simple_canvas.h"

G_BEGIN_DECLS

/* Bounds structure (for amitk_canvas_render_to_bounds) */
typedef struct {
  gdouble x1, y1, x2, y2;
} AmitkCanvasBounds;

/* Property setter for canvas items - use instead of g_object_set for canvas items */
void amitk_canvas_item_set_properties(AmitkCanvasItem *item, const gchar *first_prop, ...);

/* Macro for setting canvas item properties (replaces g_object_set for canvas items) */
#define canvas_item_set(item, ...) amitk_canvas_item_set_properties(AMITK_CANVAS_ITEM(item), __VA_ARGS__)

/* For get_parent, return NULL (we don't have parent items) */
static inline AmitkCanvasItem * amitk_canvas_item_get_parent(AmitkCanvasItem *item) {
  (void)item;
  return NULL;
}

/* Rendering with bounds structure */
static inline void amitk_canvas_render_to_bounds(AmitkSimpleCanvas *canvas, cairo_t *cr,
                                                 AmitkCanvasBounds *bounds, gdouble scale) {
  (void)scale;  /* We use canvas's internal scale */
  amitk_simple_canvas_render(canvas, cr,
                             bounds->x1, bounds->y1,
                             bounds->x2 - bounds->x1,
                             bounds->y2 - bounds->y1);
}

/* Thread-local canvas for item creation context */
extern __thread AmitkSimpleCanvas *_amitk_current_canvas;

/* Set the current canvas context (call before creating items) */
static inline void amitk_canvas_set_context(AmitkSimpleCanvas *canvas) {
  _amitk_current_canvas = canvas;
}

/* Item creation functions that take parent_or_root parameter */
AmitkCanvasItem * amitk_canvas_polyline_new(AmitkCanvasItem *parent_or_root,
                                            gboolean close_path,
                                            gint num_points,
                                            ...);

AmitkCanvasItem * amitk_canvas_text_new(AmitkCanvasItem *parent_or_root,
                                        const gchar *text,
                                        gdouble x, gdouble y,
                                        gdouble width,
                                        AmitkCanvasAnchorType anchor,
                                        ...);

AmitkCanvasItem * amitk_canvas_image_new(AmitkCanvasItem *parent_or_root,
                                         GdkPixbuf *pixbuf,
                                         gdouble x, gdouble y,
                                         ...);

AmitkCanvasItem * amitk_canvas_rect_new(AmitkCanvasItem *parent_or_root,
                                        gdouble x, gdouble y,
                                        gdouble width, gdouble height,
                                        ...);

AmitkCanvasItem * amitk_canvas_ellipse_new(AmitkCanvasItem *parent_or_root,
                                           gdouble cx, gdouble cy,
                                           gdouble rx, gdouble ry,
                                           ...);

G_END_DECLS

#endif /* __AMITK_CANVAS_COMPAT_H__ */
