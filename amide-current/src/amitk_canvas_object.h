/* amitk_canvas_object.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2004-2014 Andy Loening
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


#ifndef __AMITK_CANVAS_OBJECT_H__
#define __AMITK_CANVAS_OBJECT_H__

/* includes we always need with this widget */
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <libgnomecanvas/gnome-canvas.h>
#include "amitk_volume.h"
#include "amitk_color_table.h"

G_BEGIN_DECLS

GnomeCanvasItem * amitk_canvas_object_draw(GnomeCanvas * canvas, 
					   AmitkVolume * canvas_volume,
					   AmitkObject * object,
					   AmitkViewMode view_mode,
					   GnomeCanvasItem * item,
					   amide_real_t pixel_dim,
					   gint width, 
					   gint height,
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
					   );



G_END_DECLS


#endif /* __AMITK_CANVAS_OBJECT_H__ */

