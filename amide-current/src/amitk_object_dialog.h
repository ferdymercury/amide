/* amitk_object_dialog.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2002 Andy Loening
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

#ifndef __AMITK_OBJECT_DIALOG_H__
#define __AMITK_OBJECT_DIALOG_H__

/* includes we always need with this widget */
#include <gtk/gtk.h>
#include "amitk_object.h"
#include "amitk_data_set.h"

G_BEGIN_DECLS

/* ------------- Threshold---------- */

#define AMITK_TYPE_OBJECT_DIALOG            (amitk_object_dialog_get_type ())
#define AMITK_OBJECT_DIALOG(obj)            (GTK_CHECK_CAST ((obj), AMITK_TYPE_OBJECT_DIALOG, AmitkObjectDialog))
#define AMITK_OBJECT_DIALOG_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), AMITK_TYPE_THESHOLD, AmitkObjectDialogClass))
#define AMITK_IS_OBJECT_DIALOG(obj)         (GTK_CHECK_TYPE ((obj), AMITK_TYPE_OBJECT_DIALOG))
#define AMITK_IS_OBJECT_DIALOG_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_OBJECT_DIALOG))


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
  GtkWidget * modality_menu;
  GtkWidget * scaling_factor_entry;
  GtkWidget * creation_date_entry;
  GtkWidget * interpolation_button[AMITK_INTERPOLATION_NUM];

  GtkWidget * center_spinner[AMITK_AXIS_NUM];
  GtkWidget * voxel_size_spinner[AMITK_AXIS_NUM];
  GtkWidget * dimension_spinner[AMITK_AXIS_NUM];

  GtkWidget * start_entry;
  GtkWidget * * duration_entries;

  GtkWidget * isocontour_value_entry;
  GtkWidget * voxel_dim_entry;
};

struct _AmitkObjectDialogClass
{
  GtkDialogClass parent_class;

};



GType      amitk_object_dialog_get_type   (void);
GtkWidget* amitk_object_dialog_new        (AmitkObject * object,
					   AmitkLayout layout);


G_END_DECLS

#endif /* __AMITK_OBJECT_DIALOG_H__ */

