/* amitk_threshold.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001 Andy Loening
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

/* adapated from gtkcolorsel.h */

#ifndef __AMITK_THRESHOLD_H__
#define __AMITK_THRESHOLD_H__

/* includes we always need with this widget */
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gnome.h>
#include "volume.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ------------- Threshold---------- */

#define AMITK_TYPE_THRESHOLD            (amitk_threshold_get_type ())
#define AMITK_THRESHOLD(obj)            (GTK_CHECK_CAST ((obj), AMITK_TYPE_THRESHOLD, AmitkThreshold))
#define AMITK_THRESHOLD_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), AMITK_TYPE_THESHOLD, AmitkThresholdClass))
#define AMITK_IS_THRESHOLD(obj)         (GTK_CHECK_TYPE ((obj), AMITK_TYPE_THRESHOLD))
#define AMITK_IS_THRESHOLD_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_THRESHOLD))

typedef struct _AmitkThreshold             AmitkThreshold;
typedef struct _AmitkThresholdClass        AmitkThresholdClass;

typedef enum {
  MAX_ARROW, MIN_ARROW, NUM_THRESHOLD_ARROWS
} which_threshold_arrow_t;

typedef enum {
  MAX_ABSOLUTE, MAX_PERCENT, MIN_ABSOLUTE, MIN_PERCENT, NUM_THRESHOLD_ENTRIES
} which_threshold_entry_t;

struct _AmitkThreshold
{
  GtkVBox vbox;

  GtkWidget * color_strip;
  GtkWidget * bar_graph;
  GnomeCanvasItem * color_strip_image;
  GnomeCanvasItem * bar_graph_item;
  GnomeCanvasItem * arrow[NUM_THRESHOLD_ARROWS];
  GdkPixbuf * color_strip_rgb_image;
  GdkPixbuf * bar_graph_rgb_image;
  GtkWidget * entry[NUM_THRESHOLD_ENTRIES];
  GtkWidget * color_table_menu;

  gdouble initial_y;
  amide_data_t initial_min;
  amide_data_t initial_max;

  volume_t * volume; /* what volume this threshold corresponds to */
};

struct _AmitkThresholdClass
{
  GtkVBoxClass parent_class;

  void (* threshold_changed) (AmitkThreshold *threshold);
  void (* color_changed) (AmitkThreshold *threshold);
};  


GtkType    amitk_threshold_get_type          (void);
GtkWidget* amitk_threshold_new               (volume_t * volume);
void       amitk_threshold_new_volume        (AmitkThreshold * threshold, volume_t * new_volume);
void       amitk_threshold_update            (AmitkThreshold * threshold);

/* ---------- ThresholdDialog------------- */

#define AMITK_TYPE_THRESHOLD_DIALOG     (amitk_threshold_dialog_get_type ())
#define AMITK_THRESHOLD_DIALOG(obj)     (GTK_CHECK_CAST ((obj), AMITK_TYPE_THRESHOLD_DIALOG, AmitkThresholdDialog))
#define AMITK_THRESHOLD_DIALOG_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), AMITK_TYPE_THESHOLD_DIALOG, AmitkThresholdDialogClass))
#define AMITK_IS_THRESHOLD_DIALOG(obj)  (GTK_CHECK_TYPE ((obj), AMITK_TYPE_THRESHOLD_DIALOG))
#define AMITK_IS_THRESHOLD_DIALOG_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_THRESHOLD_DIALOG))

typedef struct _AmitkThresholdDialog       AmitkThresholdDialog;
typedef struct _AmitkThresholdDialogClass  AmitkThresholdDialogClass;

struct _AmitkThresholdDialog
{
  GnomeDialog dialog;

  GtkWidget *volume_label;
  GtkWidget *frame;
  GtkWidget *threshold;
};

struct _AmitkThresholdDialogClass
{
  GnomeDialogClass parent_class;

  void (* threshold_changed) (AmitkThresholdDialog * dialog);
  void (* color_changed) (AmitkThresholdDialog * dialog);
};



GtkType    amitk_threshold_dialog_get_type   (void);
GtkWidget* amitk_threshold_dialog_new        (volume_t * volume);
void       amitk_threshold_dialog_new_volume (AmitkThresholdDialog * threshold_dialog, volume_t * new_volume);
volume_t * amitk_threshold_dialog_volume     (AmitkThresholdDialog * threshold_dialog);

/*---------- ThresholdDialogs--------------- */

#define AMITK_TYPE_THRESHOLDS_DIALOG     (amitk_thresholds_dialog_get_type ())
#define AMITK_THRESHOLDS_DIALOG(obj)     (GTK_CHECK_CAST ((obj), AMITK_TYPE_THRESHOLDS_DIALOG, AmitkThresholdsDialog))
#define AMITK_THRESHOLDS_DIALOG_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), AMITK_TYPE_THESHOLDS_DIALOG, AmitkThresholdsDialogClass))
#define AMITK_IS_THRESHOLDS_DIALOG(obj)  (GTK_CHECK_TYPE ((obj), AMITK_TYPE_THRESHOLDS_DIALOG))
#define AMITK_IS_THRESHOLDS_DIALOG_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_THRESHOLDS_DIALOG))

typedef struct _AmitkThresholdsDialog       AmitkThresholdsDialog;
typedef struct _AmitkThresholdsDialogClass  AmitkThresholdsDialogClass;


struct _AmitkThresholdsDialog
{
  GnomeDialog dialog;

  GtkWidget *notebook;
  GList * thresholds;
};

struct _AmitkThresholdsDialogClass
{
  GnomeDialogClass parent_class;

  void (* threshold_changed) (AmitkThresholdsDialog * dialog);
  void (* color_changed) (AmitkThresholdsDialog * dialog);
};


GtkType    amitk_thresholds_dialog_get_type   (void);
GtkWidget* amitk_thresholds_dialog_new        (volume_list_t * volumes);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __AMITK_THRESHOLD_H__ */

