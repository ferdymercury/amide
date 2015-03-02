/* amitk_object_dialog.h
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

/* adapated from gtkcolorsel.h */

#ifndef __AMITK_OBJECT_DIALOG_H__
#define __AMITK_OBJECT_DIALOG_H__

/* includes we always need with this widget */
#include <gtk/gtk.h>
#include <libgnomecanvas/libgnomecanvas.h>
#include "amitk_object.h"
#include "amitk_data_set.h"

G_BEGIN_DECLS

/* ------------- Threshold---------- */

#define AMITK_TYPE_OBJECT_DIALOG            (amitk_object_dialog_get_type ())
#define AMITK_OBJECT_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), AMITK_TYPE_OBJECT_DIALOG, AmitkObjectDialog))
#define AMITK_OBJECT_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_THESHOLD, AmitkObjectDialogClass))
#define AMITK_IS_OBJECT_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AMITK_TYPE_OBJECT_DIALOG))
#define AMITK_IS_OBJECT_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_OBJECT_DIALOG))


typedef struct _AmitkObjectDialog       AmitkObjectDialog;
typedef struct _AmitkObjectDialogClass  AmitkObjectDialogClass;

struct _AmitkObjectDialog
{
  GtkDialog parent;

  AmitkObject * object;
  AmitkObject * original_object;

  gboolean aspect_ratio;
  
  GtkWidget * name_entry;
  GtkWidget * roi_type_menu;
  GtkWidget * scan_date_entry;
  GtkWidget * subject_name_entry;
  GtkWidget * subject_id_entry;
  GtkWidget * subject_dob_entry;
  GtkWidget * modality_menu;
  GtkWidget * subject_orientation_menu;
  GtkWidget * subject_sex_menu;
  GtkWidget * scaling_factor_spin;
  GtkWidget * dose_spin;
  GtkWidget * dose_unit_menu;
  GtkWidget * weight_spin;
  GtkWidget * weight_unit_menu;
  GtkWidget * cylinder_spin;
  GtkWidget * cylinder_unit_menu;
  GtkWidget * creation_date_entry;
  GtkWidget * interpolation_button[AMITK_INTERPOLATION_NUM];
  GtkWidget * rendering_menu;
  GtkWidget * conversion_button[AMITK_CONVERSION_NUM];

  GtkWidget * center_spin[AMITK_AXIS_NUM];
  GtkWidget * voxel_size_spin[AMITK_AXIS_NUM];
  GtkWidget * dimension_spin[AMITK_AXIS_NUM];

  GtkWidget * start_spin;
  GtkWidget * * duration_spins;
  GtkWidget * * gate_spins;

  GtkWidget * isocontour_min_value_entry;
  GtkWidget * isocontour_max_value_entry;
  GtkWidget * voxel_dim_entry;

  /* study preferences */
  GtkWidget * roi_width_spin;
  GnomeCanvasItem * roi_item;
#ifdef AMIDE_LIBGNOMECANVAS_AA
  GtkWidget * roi_transparency_spin;
#else
  GtkWidget * line_style_menu;
  GtkWidget * fill_roi_button;
#endif
  GtkWidget * layout_button1;
  GtkWidget * layout_button2;
  GtkWidget * panel_layout_button1;
  GtkWidget * panel_layout_button2;
  GtkWidget * panel_layout_button3;
  GtkWidget * maintain_size_button;
  GtkWidget * target_size_spin;

};

struct _AmitkObjectDialogClass
{
  GtkDialogClass parent_class;

};



GType      amitk_object_dialog_get_type   (void);
GtkWidget* amitk_object_dialog_new        (AmitkObject * object);


G_END_DECLS

#endif /* __AMITK_OBJECT_DIALOG_H__ */

