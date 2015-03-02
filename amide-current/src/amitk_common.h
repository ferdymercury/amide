/* amitk_common.h
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

#ifndef __AMITK_COMMON_H__
#define __AMITK_COMMON_H__

/* header files that are always needed with this file */
#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include "amide_intl.h"

G_BEGIN_DECLS

#define AMITK_RESPONSE_EXECUTE 1
#define AMITK_RESPONSE_COPY 2
#define AMITK_RESPONSE_SAVE_AS 3
#define AMITK_RESPONSE_SAVE_RAW_AS 4

/* defines how many times we want the progress bar to be updated over the course of an action */
#define AMITK_UPDATE_DIVIDER 40.0 /* must be float point */

/* file info.  magic string needs to be < 64 bytes */
#define AMITK_FILE_VERSION (xmlChar *) "2.0"
#define AMITK_FLAT_FILE_MAGIC_STRING "AMIDE XML Image Format Flat File"

/* typedef's */
/* layout of the three views in a canvas */
typedef enum {
  AMITK_LAYOUT_LINEAR, 
  AMITK_LAYOUT_ORTHOGONAL,
  AMITK_LAYOUT_NUM
} AmitkLayout;

/* layout of multiple canvases */
typedef enum {
  AMITK_PANEL_LAYOUT_MIXED,
  AMITK_PANEL_LAYOUT_LINEAR_X,
  AMITK_PANEL_LAYOUT_LINEAR_Y,
  AMITK_PANEL_LAYOUT_NUM
} AmitkPanelLayout;

typedef enum {
  AMITK_VIEW_MODE_SINGLE,
  AMITK_VIEW_MODE_LINKED_2WAY,
  AMITK_VIEW_MODE_LINKED_3WAY,
  AMITK_VIEW_MODE_NUM
} AmitkViewMode;

typedef enum {
  AMITK_MODALITY_PET, 
  AMITK_MODALITY_SPECT, 
  AMITK_MODALITY_CT, 
  AMITK_MODALITY_MRI, 
  AMITK_MODALITY_OTHER, 
  AMITK_MODALITY_NUM
} AmitkModality;

typedef enum {
  AMITK_LIMIT_MIN,
  AMITK_LIMIT_MAX,
  AMITK_LIMIT_NUM
} AmitkLimit;

  //  AMITK_WINDOW_BONE,
  //  AMITK_WINDOW_SOFT_TISSUE,
typedef enum {
  AMITK_WINDOW_ABDOMEN,
  AMITK_WINDOW_BRAIN,
  AMITK_WINDOW_EXTREMITIES,
  AMITK_WINDOW_LIVER,
  AMITK_WINDOW_LUNG,
  AMITK_WINDOW_PELVIS_SOFT_TISSUE,
  AMITK_WINDOW_SKULL_BASE,
  AMITK_WINDOW_SPINE_A,
  AMITK_WINDOW_SPINE_B,
  AMITK_WINDOW_THORAX_SOFT_TISSUE,
  AMITK_WINDOW_NUM
} AmitkWindow;

typedef enum {
  AMITK_THRESHOLD_STYLE_MIN_MAX,
  AMITK_THRESHOLD_STYLE_CENTER_WIDTH,
  AMITK_THRESHOLD_STYLE_NUM
} AmitkThresholdStyle;

typedef enum {
  AMITK_HELP_INFO_BLANK,
  AMITK_HELP_INFO_CANVAS_DATA_SET,
  AMITK_HELP_INFO_CANVAS_ROI,
  AMITK_HELP_INFO_CANVAS_FIDUCIAL_MARK,
  AMITK_HELP_INFO_CANVAS_STUDY,
  AMITK_HELP_INFO_CANVAS_ISOCONTOUR_ROI,
  AMITK_HELP_INFO_CANVAS_FREEHAND_ROI,
  AMITK_HELP_INFO_CANVAS_DRAWING_MODE,
  AMITK_HELP_INFO_CANVAS_LINE_PROFILE,
  AMITK_HELP_INFO_CANVAS_NEW_ROI,
  AMITK_HELP_INFO_CANVAS_NEW_ISOCONTOUR_ROI,
  AMITK_HELP_INFO_CANVAS_NEW_FREEHAND_ROI,
  AMITK_HELP_INFO_CANVAS_CHANGE_ISOCONTOUR,
  AMITK_HELP_INFO_CANVAS_SHIFT_OBJECT,
  AMITK_HELP_INFO_CANVAS_ROTATE_OBJECT,
  AMITK_HELP_INFO_TREE_VIEW_DATA_SET,
  AMITK_HELP_INFO_TREE_VIEW_ROI,
  AMITK_HELP_INFO_TREE_VIEW_FIDUCIAL_MARK,
  AMITK_HELP_INFO_TREE_VIEW_STUDY,
  AMITK_HELP_INFO_TREE_VIEW_NONE,
  AMITK_HELP_INFO_UPDATE_LOCATION,
  AMITK_HELP_INFO_UPDATE_THETA,
  AMITK_HELP_INFO_UPDATE_SHIFT,
  AMITK_HELP_INFO_NUM
} AmitkHelpInfo;


/* external variables */
extern gchar * amitk_limit_names[AMITK_THRESHOLD_STYLE_NUM][AMITK_LIMIT_NUM];
extern gchar * amitk_window_names[AMITK_WINDOW_NUM];
extern PangoFontDescription * amitk_fixed_font_desc;

/* external functions */
void amitk_common_font_init(void);

void amitk_append_str_with_newline(gchar ** pstr, const gchar * format, ...);
void amitk_append_str(gchar ** pstr, const gchar * format, ...);

void amitk_real_cell_data_func(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
			       GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data);
gint amitk_spin_button_scientific_output (GtkSpinButton *spin_button, gpointer data);
gint amitk_spin_button_discard_double_or_triple_click(GtkWidget *widget, GdkEventButton *event, gpointer func_data);
GdkPixbuf * amitk_get_pixbuf_from_canvas(GnomeCanvas * canvas, gint xoffset, gint yoffset,
					 gint width, gint height);

gboolean amitk_is_xif_directory(const gchar * filename, gboolean * plegacy, gchar ** pxml_filename);
gboolean amitk_is_xif_flat_file(const gchar * filename, guint64 * plocation_le, guint64 *psize_le);


/* built in type functions */
const gchar *   amitk_layout_get_name             (const AmitkLayout layout);
const gchar *   amitk_panel_layout_get_name       (const AmitkPanelLayout panel_layout);
const gchar *   amitk_limit_get_name              (const AmitkLimit limit);
const gchar *   amitk_window_get_name             (const AmitkWindow window);
const gchar *   amitk_modality_get_name           (const AmitkModality modality);

G_END_DECLS

#endif /* __AMITK_COMMON_H__ */
