/* amitk_threshold.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2014 Andy Loening
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

/* adapated from gtkcolorsel.h */

#ifndef __AMITK_THRESHOLD_H__
#define __AMITK_THRESHOLD_H__

/* includes we always need with this widget */
#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include "amitk_data_set.h"
#include "amitk_study.h"

G_BEGIN_DECLS

/* ------------- Threshold---------- */

#define AMITK_TYPE_THRESHOLD            (amitk_threshold_get_type ())
#define AMITK_THRESHOLD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), AMITK_TYPE_THRESHOLD, AmitkThreshold))
#define AMITK_THRESHOLD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_THESHOLD, AmitkThresholdClass))
#define AMITK_IS_THRESHOLD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AMITK_TYPE_THRESHOLD))
#define AMITK_IS_THRESHOLD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_THRESHOLD))

typedef struct _AmitkThreshold             AmitkThreshold;
typedef struct _AmitkThresholdClass        AmitkThresholdClass;


typedef enum {
  AMITK_THRESHOLD_SCALE_FULL,
  AMITK_THRESHOLD_SCALE_SCALED,
  AMITK_THRESHOLD_SCALE_NUM_SCALES
} AmitkThresholdScale;

typedef enum {
  AMITK_THRESHOLD_ARROW_FULL_MIN,
  AMITK_THRESHOLD_ARROW_FULL_CENTER,
  AMITK_THRESHOLD_ARROW_FULL_MAX,
  AMITK_THRESHOLD_ARROW_SCALED_MIN,
  AMITK_THRESHOLD_ARROW_SCALED_CENTER,
  AMITK_THRESHOLD_ARROW_SCALED_MAX,
  AMITK_THRESHOLD_ARROW_NUM_ARROWS
} AmitkThresholdArrow;

typedef enum {
  AMITK_THRESHOLD_ENTRY_MAX_ABSOLUTE,
  AMITK_THRESHOLD_ENTRY_MAX_PERCENT,
  AMITK_THRESHOLD_ENTRY_MIN_ABSOLUTE,
  AMITK_THRESHOLD_ENTRY_MIN_PERCENT,
  AMITK_THRESHOLD_ENTRY_NUM_ENTRIES
} AmitkThresholdEntry;

typedef enum {
  AMITK_THRESHOLD_LINE_MAX,
  AMITK_THRESHOLD_LINE_CENTER,
  AMITK_THRESHOLD_LINE_MIN,
  AMITK_THRESHOLD_LINE_NUM_LINES
} AmitkThresholdLine;

typedef enum {
    AMITK_THRESHOLD_BOX_LAYOUT,
    AMITK_THRESHOLD_LINEAR_LAYOUT
} AmitkThresholdLayout;

struct _AmitkThreshold
{
  GtkVBox vbox;

  gboolean minimal; /* true if we just want the color table menu and the color scale */
  AmitkViewMode view_mode;
  GtkWidget * color_scales[2];
  GtkWidget * histogram;
  GtkWidget * histogram_label;
  GnomeCanvasItem * color_scale_image[2][AMITK_THRESHOLD_SCALE_NUM_SCALES];
  GnomeCanvasItem * histogram_image;
  GnomeCanvasItem * arrow[2][AMITK_THRESHOLD_ARROW_NUM_ARROWS];
  GnomeCanvasItem * connector_line[2][AMITK_THRESHOLD_LINE_NUM_LINES];
  GtkWidget * spin_button[2][AMITK_THRESHOLD_ENTRY_NUM_ENTRIES];
  GtkWidget * color_table_label[AMITK_VIEW_MODE_NUM];
  GtkWidget * color_table_hbox[AMITK_VIEW_MODE_NUM];
  GtkWidget * color_table_menu[AMITK_VIEW_MODE_NUM];
  GtkWidget * color_table_independent[AMITK_VIEW_MODE_NUM];
  GtkWidget * threshold_ref_frame_menu[2];
  GtkWidget * percent_label[2];
  GtkWidget * absolute_label[2];
  GtkWidget * ref_frame_label[2];
  GtkWidget * full_label[2];
  GtkWidget * scaled_label[2];
  GtkWidget * type_button[AMITK_THRESHOLDING_NUM];
  GtkWidget * window_label;
  GtkWidget * window_vbox;
  GtkWidget * style_button[AMITK_THRESHOLD_STYLE_NUM];
  GtkWidget * min_max_label[AMITK_LIMIT_NUM];


  gdouble initial_y[2];
  amide_data_t initial_min[2];
  amide_data_t initial_max[2];

  amide_data_t threshold_max[2]; 
  amide_data_t threshold_min[2]; 

  GtkWidget * progress_dialog;

  AmitkDataSet * data_set; /* what data set this threshold corresponds to */
};

struct _AmitkThresholdClass
{
  GtkVBoxClass parent_class;
};  


GType      amitk_threshold_get_type          (void);
GtkWidget* amitk_threshold_new               (AmitkDataSet * data_set,
					      AmitkThresholdLayout layout,
					      GtkWindow * parent,
					      gboolean minimal);
void       amitk_threshold_new_data_set      (AmitkThreshold * threshold, 
					      AmitkDataSet * new_data_set);
void       amitk_threshold_style_widgets     (GtkWidget ** radio_buttons, 
					      GtkWidget * hbox);

/* ---------- ThresholdDialog------------- */

#define AMITK_TYPE_THRESHOLD_DIALOG     (amitk_threshold_dialog_get_type ())
#define AMITK_THRESHOLD_DIALOG(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), AMITK_TYPE_THRESHOLD_DIALOG, AmitkThresholdDialog))
#define AMITK_THRESHOLD_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_THESHOLD_DIALOG, AmitkThresholdDialogClass))
#define AMITK_IS_THRESHOLD_DIALOG(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AMITK_TYPE_THRESHOLD_DIALOG))
#define AMITK_IS_THRESHOLD_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_THRESHOLD_DIALOG))

typedef struct _AmitkThresholdDialog       AmitkThresholdDialog;
typedef struct _AmitkThresholdDialogClass  AmitkThresholdDialogClass;

struct _AmitkThresholdDialog
{
  GtkDialog dialog;

  GtkWidget *data_set_label;
  GtkWidget *vbox;
  GtkWidget *threshold;
};

struct _AmitkThresholdDialogClass
{
  GtkDialogClass parent_class;

};



GType          amitk_threshold_dialog_get_type     (void);
GtkWidget*     amitk_threshold_dialog_new          (AmitkDataSet * data_set, GtkWindow * parent);
void           amitk_threshold_dialog_new_data_set (AmitkThresholdDialog * threshold_dialog, 
						    AmitkDataSet * new_data_set);
AmitkDataSet * amitk_threshold_dialog_data_set     (AmitkThresholdDialog * threshold_dialog);

/*---------- ThresholdDialogs--------------- */

#define AMITK_TYPE_THRESHOLDS_DIALOG     (amitk_thresholds_dialog_get_type ())
#define AMITK_THRESHOLDS_DIALOG(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), AMITK_TYPE_THRESHOLDS_DIALOG, AmitkThresholdsDialog))
#define AMITK_THRESHOLDS_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_THESHOLDS_DIALOG, AmitkThresholdsDialogClass))
#define AMITK_IS_THRESHOLDS_DIALOG(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AMITK_TYPE_THRESHOLDS_DIALOG))
#define AMITK_IS_THRESHOLDS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_THRESHOLDS_DIALOG))

typedef struct _AmitkThresholdsDialog       AmitkThresholdsDialog;
typedef struct _AmitkThresholdsDialogClass  AmitkThresholdsDialogClass;


struct _AmitkThresholdsDialog
{
  GtkDialog dialog;

  GtkWidget *notebook;
  GList * thresholds;
};

struct _AmitkThresholdsDialogClass
{
  GtkDialogClass parent_class;

};


GType      amitk_thresholds_dialog_get_type   (void);
GtkWidget* amitk_thresholds_dialog_new        (GList * objects, GtkWindow * parent);



G_END_DECLS

#endif /* __AMITK_THRESHOLD_H__ */

