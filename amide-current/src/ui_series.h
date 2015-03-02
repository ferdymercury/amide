/* ui_series.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2002 Andy Loening
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

#ifndef __UI_SERIES_H__
#define __UI_SERIES_H__

/* header files that are always needed with this file */
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "amitk_study.h"


#define UI_SERIES_L_MARGIN 2.0
#define UI_SERIES_R_MARGIN UI_SERIES_L_MARGIN
#define UI_SERIES_TOP_MARGIN 2.0
#define UI_SERIES_BOTTOM_MARGIN 15.0
#define UI_SERIES_CAPTION_FONT "fixed"

typedef enum {PLANES, FRAMES} series_t;

/* ui_series data structures */
typedef struct ui_series_t {
  GnomeApp * app; 
  GList ** slices;
  GList * objects;
  AmitkDataSet * active_ds;
  GnomeCanvas * canvas;
  GnomeCanvasItem ** images;
  GnomeCanvasItem ** captions;
  GtkWidget * thresholds_dialog;
  guint num_slices, rows, columns;
  AmitkVolume * volume;
  amide_time_t view_time;
  AmitkFuseType fuse_type;
  amide_real_t voxel_dim;
  series_t type;

  /* for "PLANES" series */
  amide_time_t view_duration;
  amide_real_t start_z;
  amide_real_t z_point; /* current slice offset z component*/
  amide_real_t end_z;

  /* for "FRAMES" series */
  guint view_frame;
  amide_time_t start_time;
  amide_time_t * frame_durations; /* an array of frame durations */

  guint reference_count;
} ui_series_t;

/* external functions */
void ui_series_create(AmitkStudy * study,GList * objects, AmitkDataSet * active_ds,
		      AmitkView view, AmitkVolume * canvas_view, series_t series_type);



#endif /* UI_SERIES_H */



