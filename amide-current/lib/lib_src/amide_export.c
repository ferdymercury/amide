/* amide_export.c
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

#include "config.h"
#include <gnome.h>
#include <math.h>
#include "amide.h"
#include "amide_export.h"
#include "color_table.h"
#include "study.h"
#include "image.h"
#include "ui_series.h"
#include "ui_study.h"
#include "ui_study_cb.h"

/* exported procedure to set the name of the study */
void amide_study_set_name(GtkWidget * amide_widget,  gchar * new_name) {
  ui_study_t * ui_study;
  GtkWidget * study_leaf;
  GtkWidget * label;

  ui_study = gtk_object_get_data(GTK_OBJECT(amide_widget), "ui_study");
  study_set_name(ui_study->study, new_name);

  /* apply any changes to the name to the study list widget */
  study_leaf = gtk_object_get_data(GTK_OBJECT(ui_study->tree), "study_leaf");
  label = gtk_object_get_data(GTK_OBJECT(study_leaf), "text_label");
  gtk_label_set_text(GTK_LABEL(label), study_name(ui_study->study));

  return;
}

/* exported procedure to set the subjet's age in a study */
void amide_study_set_subject_age(GtkWidget * amide_widget, gchar * new_age) {
  ui_study_t * ui_study;

  ui_study = gtk_object_get_data(GTK_OBJECT(amide_widget), "ui_study");
  g_warning("%s: age not currently implemented",PACKAGE);
  //  study_set_age(ui_study->study, new_age);

  return;
}

/* legacy function, just sets things trilinear */
void amide_study_set_interpolation_bilinear(GtkWidget * amide_widget) {

  g_warning("bilinear no longer supported, switch to trilinear\n");
  amide_study_set_interpolation_trilinear(amide_widget);
  return;
}

/* exported procedure to set the interpolation used for the study to trilinear */
void amide_study_set_interpolation_trilinear(GtkWidget * amide_widget) {
  ui_study_t * ui_study;

  ui_study = gtk_object_get_data(GTK_OBJECT(amide_widget), "ui_study");
  study_set_interpolation(ui_study->study, TRILINEAR);

  return;
}

/* exported proocedure to set the scaling of a study to global */
void amide_study_set_scaling_global(GtkWidget * amide_widget) {
  ui_study_t * ui_study;

  ui_study = gtk_object_get_data(GTK_OBJECT(amide_widget), "ui_study");
  study_set_scaling(ui_study->study, SCALING_GLOBAL);

  return;
}


/* exported procedure to set the zoom of the canvas widgets */
void amide_study_set_zoom(GtkWidget * amide_widget, gfloat zoom) {
  ui_study_t * ui_study;

  ui_study = gtk_object_get_data(GTK_OBJECT(amide_widget), "ui_study");
  study_set_zoom(ui_study->study, zoom);

  return;
}


/* exported procedure to save the current study */
void amide_study_save(GtkWidget * amide_widget) {

  ui_study_t * ui_study;

  ui_study = gtk_object_get_data(GTK_OBJECT(amide_widget), "ui_study");
  ui_study_cb_save_as(amide_widget, ui_study);

  return;
}

/* exported procedure to fire-up the threshold widget */
void amide_study_threshold(GtkWidget * amide_widget) {

  ui_study_t * ui_study;

  ui_study = gtk_object_get_data(GTK_OBJECT(amide_widget), "ui_study");
  ui_study_cb_threshold_pressed(amide_widget, ui_study);

  return;
}

/* exported procedure to import a pem file into a amide study widget */
void amide_study_import_pem(GtkWidget * amide_widget, 
			    gchar * pem_data_filename, 
			    gchar * pem_model_filename) {
  
  ui_study_t * ui_study;

  ui_study = gtk_object_get_data(GTK_OBJECT(amide_widget), "ui_study");

  /* import the file */
  ui_study_import_file(ui_study, PEM_DATA, 0, pem_data_filename, pem_model_filename);

  return;
}

/* procedure to set up a blank study window */
GtkWidget * amide_study_new(GtkWidget * parent_bin) {
  return ui_study_create(NULL, parent_bin);
}


