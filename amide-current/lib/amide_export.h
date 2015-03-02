/* amide_export.h
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

/* typedef's */

/* external functions */
void amide_study_set_name(GtkWidget * amide_widget,  gchar * new_name);
void amide_study_set_subject_age(GtkWidget * amide_widget,  gchar * new_age);
void amide_study_set_interpolation_bilinear(GtkWidget * amide_widget);
void amide_study_set_scaling_global(GtkWidget * amide_widget);
void amide_study_set_zoom(GtkWidget * amide_widget, gfloat zoom);
void amide_study_save(GtkWidget * amide_widget);
void amide_study_threshold(GtkWidget * amide_widget);
void amide_study_import_pem(GtkWidget * amide_widget, 
			    gchar * pem_data_filename, 
			    gchar * pem_model_filename);
GtkWidget * amide_study_new(GtkWidget * parent_bin);

/* external variables */


