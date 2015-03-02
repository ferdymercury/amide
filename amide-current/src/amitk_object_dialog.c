/* amitk_object_dialog.c
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

/* adapted from gtkcolorsel.c */


#include "amide_config.h"
#include "amide.h"
#include "amitk_marshal.h"
#include "amitk_object_dialog.h"
#include "amitk_study.h"
#include "amitk_space_edit.h"
#include "amitk_threshold.h"
#include "amitk_window_edit.h"
#include "amitk_common.h"
#include "ui_common.h"


#define AMITK_RESPONSE_REVERT 2
#define DIMENSION_STEP 0.2

static void object_dialog_class_init (AmitkObjectDialogClass *class);
static void object_dialog_init (AmitkObjectDialog *object_dialog);
static void object_dialog_destroy (GtkObject * object);
static void object_dialog_construct(AmitkObjectDialog * dialog, 
				    AmitkObject * object,
				    AmitkLayout layout);
static void dialog_update_entries  (AmitkObjectDialog * dialog);
static void dialog_update_interpolation(AmitkObjectDialog * dialog);
static void dialog_update_rendering(AmitkObjectDialog * dialog);
static void dialog_update_conversion(AmitkObjectDialog * dialog);
static void dialog_update_roi_sample_item(AmitkObjectDialog * dialog);

static void dialog_response_cb        (GtkDialog * dialog,
				       gint        response_id,
				       gpointer    data);
static void dialog_change_interpolation_cb   (GtkWidget * widget, gpointer data);
static void dialog_change_rendering_cb       (GtkWidget * widget, gpointer data);
static void dialog_conversion_cb             (GtkWidget * widget, gpointer data);
static void dialog_set_view_center_to_origin_cb(GtkWidget * widget, gpointer data);
static void dialog_aspect_ratio_cb           (GtkWidget * widget, gpointer data);
static void dialog_change_name_cb            (GtkWidget * widget, gpointer data);
static void dialog_change_creation_date_cb   (GtkWidget * widget, gpointer data);
static void dialog_change_scan_date_cb       (GtkWidget * widget, gpointer data);
static void dialog_change_subject_name_cb    (GtkWidget * widget, gpointer data);
static void dialog_change_subject_id_cb      (GtkWidget * widget, gpointer data);
static void dialog_change_subject_dob_cb     (GtkWidget * widget, gpointer data);
static void dialog_change_center_cb          (GtkWidget * widget, gpointer data);
static void dialog_change_dim_cb             (GtkWidget * widget, gpointer data);
static void dialog_change_voxel_size_cb      (GtkWidget * widget, gpointer data);
static void dialog_change_scale_factor_cb    (GtkWidget * widget, gpointer data);
static void dialog_change_dose_cb            (GtkWidget * widget, gpointer data);
static void dialog_change_weight_cb          (GtkWidget * widget, gpointer data);
static void dialog_change_cylinder_cb        (GtkWidget * widget, gpointer data);
static void dialog_change_scan_start_cb      (GtkWidget * widget, gpointer data);
static void dialog_change_frame_duration_cb  (GtkWidget * widget, gpointer data);
static void dialog_change_gate_time_cb       (GtkWidget * widget, gpointer data);
static void dialog_change_roi_type_cb      (GtkWidget * widget, gpointer data);
static void dialog_change_modality_cb      (GtkWidget * widget, gpointer data);
static void dialog_change_subject_orientation_cb(GtkWidget * widget, gpointer data);
static void dialog_change_subject_sex_cb   (GtkWidget * widget, gpointer data);
static void dialog_change_dose_unit_cb     (GtkWidget * widget, gpointer data);
static void dialog_change_weight_unit_cb   (GtkWidget * widget, gpointer data);
static void dialog_change_cylinder_unit_cb (GtkWidget * widget, gpointer data);

static void dialog_change_roi_width_cb           (GtkWidget * widget, gpointer data);
#ifdef AMIDE_LIBGNOMECANVAS_AA
static void dialog_change_roi_transparency_cb    (GtkWidget * widget, gpointer data);
#else
static void dialog_change_line_style_cb          (GtkWidget * widget, gpointer data);
static void dialog_change_fill_roi_cb            (GtkWidget * widget, gpointer data);
#endif
static void dialog_change_layout_cb              (GtkWidget * widget, gpointer data);
static void dialog_change_panel_layout_cb        (GtkWidget * widget, gpointer data);
static void dialog_change_maintain_size_cb       (GtkWidget * widget, gpointer data);
static void dialog_change_target_empty_area_cb   (GtkWidget * widget, gpointer data);
static void dialog_specify_color_cb              (GtkWidget * widget, gpointer data);
static void dialog_change_color_cb               (GtkWidget * widget, gpointer data);

static GtkDialogClass *object_dialog_parent_class;


GType amitk_object_dialog_get_type (void)
{
  static GType object_dialog_type = 0;

  if (!object_dialog_type)
    {
      GTypeInfo object_dialog_info =
      {
	sizeof (AmitkObjectDialogClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) object_dialog_class_init,
	(GClassFinalizeFunc) NULL,
	NULL, /* class data */
	sizeof (AmitkObjectDialog),
	0,   /* # preallocs */
	(GInstanceInitFunc) object_dialog_init,
	NULL   /* value table */
      };

      object_dialog_type = g_type_register_static(GTK_TYPE_DIALOG, "AmitkObjectDialog", 
						  &object_dialog_info, 0);
    }

  return object_dialog_type;
}

static void object_dialog_class_init (AmitkObjectDialogClass *klass)
{
  GtkObjectClass *gtkobject_class;

  gtkobject_class = (GtkObjectClass*) klass;

  object_dialog_parent_class = g_type_class_peek_parent(klass);

  gtkobject_class->destroy = object_dialog_destroy;

}

static void object_dialog_destroy (GtkObject * object) {

  AmitkObjectDialog * dialog;

  g_return_if_fail (object != NULL);
  g_return_if_fail (AMITK_IS_OBJECT_DIALOG (object));

  dialog = AMITK_OBJECT_DIALOG(object);

  if (dialog->object != NULL) {
    g_signal_handlers_disconnect_by_func(G_OBJECT(dialog->object), dialog_update_entries, dialog);
    g_signal_handlers_disconnect_by_func(G_OBJECT(dialog->object), dialog_update_interpolation, dialog);
    g_signal_handlers_disconnect_by_func(G_OBJECT(dialog->object), dialog_update_rendering, dialog);
    g_signal_handlers_disconnect_by_func(G_OBJECT(dialog->object), dialog_update_conversion, dialog);
    dialog->object->dialog = NULL;
    dialog->object = amitk_object_unref(dialog->object);
  }

  if (dialog->original_object != NULL) {
#if AMIDE_DEBUG
    {
      gchar * temp_string;
      temp_string = g_strdup_printf(_("Copy of %s"), AMITK_OBJECT_NAME(dialog->original_object));
      amitk_object_set_name(dialog->original_object,temp_string);
      g_free(temp_string);
    }
#endif
    dialog->original_object = amitk_object_unref(dialog->original_object);
  }

  if (dialog->duration_spins != NULL) {
    g_free(dialog->duration_spins);
    dialog->duration_spins = NULL;
  }

  if (GTK_OBJECT_CLASS (object_dialog_parent_class)->destroy)
    (* GTK_OBJECT_CLASS (object_dialog_parent_class)->destroy) (object);
}


static void object_dialog_init (AmitkObjectDialog * dialog) {

  dialog->object = NULL;
  dialog->original_object = NULL;
  dialog->aspect_ratio = TRUE;
  dialog->duration_spins = NULL;

  return;
}


static void object_dialog_construct(AmitkObjectDialog * dialog, 
				    AmitkObject * object,
				    AmitkLayout layout) {

  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * label;
  GtkWidget * hseparator;
  GtkWidget * vseparator;
  GtkWidget * axis_indicator;
  GtkWidget * check_button;
  GtkWidget * notebook;
  GtkWidget * space_edit;
  GtkWidget * hbox;
  GtkWidget * image;
  GtkWidget * button;
  gint table_row;
  gint inner_table_row;
  gint table_column;
  AmitkAxis i_axis;
  guint i;
  gboolean immutables;

  gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
  gtk_dialog_add_buttons(GTK_DIALOG(dialog),
			 GTK_STOCK_REVERT_TO_SAVED, AMITK_RESPONSE_REVERT,
			 GTK_STOCK_HELP, GTK_RESPONSE_HELP,
			 GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, 
			 NULL);

  /* create the temp object which will store the old info if we want revert */
  dialog->original_object = amitk_object_copy(object);
  dialog->object = amitk_object_ref(object);

  /* setup the callbacks for the dialog */
  g_signal_connect(G_OBJECT(dialog), "response",
  		   G_CALLBACK(dialog_response_cb), NULL);

  notebook = gtk_notebook_new();
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), notebook);
  gtk_widget_show(notebook);

  /* ---------------------------
     Basic info page 
     --------------------------- */

  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(14,4,FALSE);
  label = gtk_label_new(_("Basic Info"));
  table_row=0;
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), packing_table, label);
  gtk_widget_show(label);
  gtk_widget_show(packing_table);

  /* widgets to change the object's name */
  if (AMITK_IS_DATA_SET(object) ) 
    label = gtk_label_new(_("Data Set Name:"));
  else 
    label = gtk_label_new(_("Name:"));
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1, 
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(label);

  dialog->name_entry = gtk_entry_new();
  gtk_editable_set_editable(GTK_EDITABLE(dialog->name_entry), TRUE);
  g_signal_connect(G_OBJECT(dialog->name_entry), "changed", G_CALLBACK(dialog_change_name_cb), dialog);
  gtk_table_attach(GTK_TABLE(packing_table),dialog->name_entry,1,4, 
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(dialog->name_entry);
  table_row++;



  if (AMITK_IS_ROI(object)) {
    AmitkRoiType i_roi_type;
    AmitkRoiType type_start, type_end;

    /* widgets to change the object's type */
    label = gtk_label_new(_("Type:"));
    gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);
    
    dialog->roi_type_menu = gtk_combo_box_new_text();

    switch(AMITK_ROI_TYPE(object)) {
    case AMITK_ROI_TYPE_ELLIPSOID:
    case AMITK_ROI_TYPE_CYLINDER:
    case AMITK_ROI_TYPE_BOX:
      type_start = 0;
      type_end = AMITK_ROI_TYPE_BOX;
      break;
    case AMITK_ROI_TYPE_ISOCONTOUR_2D:
    case AMITK_ROI_TYPE_ISOCONTOUR_3D:
    case AMITK_ROI_TYPE_FREEHAND_2D:
    case AMITK_ROI_TYPE_FREEHAND_3D:
      type_start = type_end = AMITK_ROI_TYPE(object);
      break;
    default:
      type_start = type_end = 0;
      g_error("unexpected case in %s at line %d\n", __FILE__, __LINE__);
      break;
    }

    for (i_roi_type=type_start; i_roi_type<=type_end; i_roi_type++) 
      gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->roi_type_menu),
    				amitk_roi_type_get_name(i_roi_type));
    if (type_start != type_end)
      g_signal_connect(G_OBJECT(dialog->roi_type_menu), "changed", G_CALLBACK(dialog_change_roi_type_cb), dialog);
    gtk_table_attach(GTK_TABLE(packing_table), dialog->roi_type_menu, 1,2, 
		     table_row,table_row+1, GTK_FILL, 0,  X_PADDING, Y_PADDING);
    gtk_widget_show(dialog->roi_type_menu);
    table_row++;

    gtk_widget_show(packing_table);


  } else if (AMITK_IS_DATA_SET(object)) {
    AmitkInterpolation i_interpolation;
    AmitkRendering i_rendering;
    AmitkConversion i_conversion;
    AmitkModality i_modality;
    AmitkDoseUnit i_dose_unit;
    AmitkWeightUnit i_weight_unit;
    AmitkCylinderUnit i_cylinder_unit;
    AmitkSubjectOrientation i_subject_orientation;
    AmitkSubjectSex i_subject_sex;


    /* widgets to change the date of the scan name */
    label = gtk_label_new(_("Scan Date:"));
    gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    dialog->scan_date_entry = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(dialog->scan_date_entry), TRUE);
    g_signal_connect(G_OBJECT(dialog->scan_date_entry), "changed", G_CALLBACK(dialog_change_scan_date_cb), dialog);
    gtk_table_attach(GTK_TABLE(packing_table), dialog->scan_date_entry,1,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(dialog->scan_date_entry);

    /* widgets to change the object's modality */
    label = gtk_label_new(_("Modality:"));
    gtk_table_attach(GTK_TABLE(packing_table), label, 2,3,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    dialog->modality_menu = gtk_combo_box_new_text();
    for (i_modality=0; i_modality<AMITK_MODALITY_NUM; i_modality++) 
      gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->modality_menu),
				amitk_modality_get_name(i_modality));
    g_signal_connect(G_OBJECT(dialog->modality_menu), "changed", G_CALLBACK(dialog_change_modality_cb), dialog);
    gtk_table_attach(GTK_TABLE(packing_table), dialog->modality_menu, 3,4, 
		     table_row,table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(dialog->modality_menu);
    table_row++;
    
    /* widget to change the interpolation */
    label = gtk_label_new(_("Interpolation Type:"));
    gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);
    
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(packing_table), hbox,1,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(hbox);

    for (i_interpolation = 0; i_interpolation < AMITK_INTERPOLATION_NUM; i_interpolation++) {

      if (i_interpolation == 0)
	dialog->interpolation_button[0] = gtk_radio_button_new(NULL);
      else
	dialog->interpolation_button[i_interpolation] = 
	  gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(dialog->interpolation_button[0]));

      switch (i_interpolation) {
      case AMITK_INTERPOLATION_NEAREST_NEIGHBOR:
	image = gtk_image_new_from_stock("amide_icon_interpolation_nearest_neighbor",GTK_ICON_SIZE_LARGE_TOOLBAR);
	break;
      case AMITK_INTERPOLATION_TRILINEAR:
	image = gtk_image_new_from_stock("amide_icon_interpolation_trilinear",GTK_ICON_SIZE_LARGE_TOOLBAR);
	break;
      default:
	g_error("unexpected case in %s at line %d",__FILE__, __LINE__);
	break;
      }
      gtk_button_set_image(GTK_BUTTON(dialog->interpolation_button[i_interpolation]), image);

      
      gtk_box_pack_start(GTK_BOX(hbox), dialog->interpolation_button[i_interpolation], FALSE, FALSE, 3);
      gtk_widget_show(dialog->interpolation_button[i_interpolation]);
      gtk_widget_set_tooltip_text(dialog->interpolation_button[i_interpolation],
				  amitk_interpolation_explanations[i_interpolation]);
      
      g_object_set_data(G_OBJECT(dialog->interpolation_button[i_interpolation]), "interpolation", 
			GINT_TO_POINTER(i_interpolation));
      
      g_signal_connect(G_OBJECT(dialog->interpolation_button[i_interpolation]), "clicked",  
		       G_CALLBACK(dialog_change_interpolation_cb), dialog);
    }


    /* widget to change the rendering */
    label = gtk_label_new(_("Rendering Type:"));
    gtk_table_attach(GTK_TABLE(packing_table), label, 2,3,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);
    

    dialog->rendering_menu = gtk_combo_box_new_text();
    /*  gtk_widget_set_tooltip_text(dialog->rendering_menu, _(amitk_rendering_explanation)); 
	combo box's (as of 2.24 at least, don't have functioning tool tips */
    for (i_rendering = 0; i_rendering < AMITK_RENDERING_NUM; i_rendering++)
      gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->rendering_menu),
				amitk_rendering_get_name(i_rendering));
    g_signal_connect(G_OBJECT(dialog->rendering_menu), "changed", G_CALLBACK(dialog_change_rendering_cb), dialog);
    gtk_table_attach(GTK_TABLE(packing_table), dialog->rendering_menu, 3,4,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(dialog->rendering_menu);
    table_row++;


    /* a separator for clarity */
    hseparator = gtk_hseparator_new();
    gtk_table_attach(GTK_TABLE(packing_table), hseparator, 0, 5, table_row, table_row+1,
		     GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(hseparator);
    table_row++;
  

    table_column=0;


    /* widgets to change the subject name associated with the data */
    label = gtk_label_new(_("Subject Name:"));
    gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    dialog->subject_name_entry = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(dialog->subject_name_entry), TRUE);
    g_signal_connect(G_OBJECT(dialog->subject_name_entry), "changed", G_CALLBACK(dialog_change_subject_name_cb), dialog);
    gtk_table_attach(GTK_TABLE(packing_table), dialog->subject_name_entry,1,4,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(dialog->subject_name_entry);
    table_row++;

    /* widgets to change the id associated with the data */
    label = gtk_label_new(_("Subject ID:"));
    gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    dialog->subject_id_entry = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(dialog->subject_id_entry), TRUE);
    g_signal_connect(G_OBJECT(dialog->subject_id_entry), "changed", G_CALLBACK(dialog_change_subject_id_cb), dialog);
    gtk_table_attach(GTK_TABLE(packing_table), dialog->subject_id_entry,1,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(dialog->subject_id_entry);

    /* widgets to change the subject's date of birth */
    label = gtk_label_new(_("Subject DOB:"));
    gtk_table_attach(GTK_TABLE(packing_table), label, 2,3,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    dialog->subject_dob_entry = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(dialog->subject_dob_entry), TRUE);
    g_signal_connect(G_OBJECT(dialog->subject_dob_entry), "changed", G_CALLBACK(dialog_change_subject_dob_cb), dialog);
    gtk_table_attach(GTK_TABLE(packing_table), dialog->subject_dob_entry,3,4,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(dialog->subject_dob_entry);

    table_row++;

    /* widgets to change the subject's orientation */
    label = gtk_label_new(_("Subject Orientation:"));
    gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    dialog->subject_orientation_menu = gtk_combo_box_new_text();
    for (i_subject_orientation=0; i_subject_orientation<AMITK_SUBJECT_ORIENTATION_NUM; i_subject_orientation++) 
      gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->subject_orientation_menu),
				amitk_subject_orientation_get_name(i_subject_orientation));
    g_signal_connect(G_OBJECT(dialog->subject_orientation_menu), "changed", G_CALLBACK(dialog_change_subject_orientation_cb), dialog);
    gtk_table_attach(GTK_TABLE(packing_table), dialog->subject_orientation_menu, 1,2, 
		     table_row,table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(dialog->subject_orientation_menu);

    /* widgets to change the subject's sex (much easier in the virtual world than in real life) */
    label = gtk_label_new(_("Subject Sex:"));
    gtk_table_attach(GTK_TABLE(packing_table), label, 2,3,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    dialog->subject_sex_menu = gtk_combo_box_new_text();
    for (i_subject_sex=0; i_subject_sex<AMITK_SUBJECT_SEX_NUM; i_subject_sex++) 
      gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->subject_sex_menu),
				amitk_subject_sex_get_name(i_subject_sex));
    g_signal_connect(G_OBJECT(dialog->subject_sex_menu), "changed", G_CALLBACK(dialog_change_subject_sex_cb), dialog);
    gtk_table_attach(GTK_TABLE(packing_table), dialog->subject_sex_menu, 3,4,
		     table_row,table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(dialog->subject_sex_menu);

    table_row++;
    
    /* a separator for clarity */
    hseparator = gtk_hseparator_new();
    gtk_table_attach(GTK_TABLE(packing_table), hseparator, 0, 5, table_row, table_row+1,
		     GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(hseparator);
    table_row++;
  

    table_column=0;


    /* widget to change the scaling factor */
    label = gtk_label_new(_("Conversion Type:"));
    gtk_table_attach(GTK_TABLE(packing_table), label, table_column,table_column+1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);


    inner_table_row = table_row+1;
    for (i_conversion=0; i_conversion < AMITK_CONVERSION_NUM; i_conversion++) {
      if (i_conversion == 0)
	dialog->conversion_button[0] = 
	  gtk_radio_button_new_with_label(NULL, amitk_conversion_names[i_conversion]);
      else
	dialog->conversion_button[i_conversion] = 
	  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(dialog->conversion_button[0]),
						      amitk_conversion_names[i_conversion]);
      
      g_object_set_data(G_OBJECT(dialog->conversion_button[i_conversion]), "conversion", 
			GINT_TO_POINTER(i_conversion));
      gtk_table_attach(GTK_TABLE(packing_table), dialog->conversion_button[i_conversion], 
		       table_column,table_column+1, inner_table_row, inner_table_row+1,
		       0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(dialog->conversion_button[i_conversion]);
      g_signal_connect(G_OBJECT(dialog->conversion_button[i_conversion]), "clicked",  
		       G_CALLBACK(dialog_conversion_cb), dialog);
      if (i_conversion == 0) inner_table_row+=2;
      else inner_table_row++;

    }

    table_row++;
    table_column++;



    label = gtk_label_new(_("Scaling Factor:"));
    gtk_table_attach(GTK_TABLE(packing_table), label, table_column,table_column+1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    dialog->scaling_factor_spin = gtk_spin_button_new_with_range(0.0, G_MAXDOUBLE, 1.0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->scaling_factor_spin), FALSE);
    g_signal_connect(G_OBJECT(dialog->scaling_factor_spin), "value_changed", 
		     G_CALLBACK(dialog_change_scale_factor_cb), dialog);
    g_signal_connect(G_OBJECT(dialog->scaling_factor_spin), "output",
		     G_CALLBACK(amitk_spin_button_scientific_output), NULL);
    gtk_table_attach(GTK_TABLE(packing_table), dialog->scaling_factor_spin,
		     table_column+1,table_column+2,table_row, table_row+1, 
		     GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(dialog->scaling_factor_spin);
    table_row++;


    /* a separator for clarity */
    hseparator = gtk_hseparator_new();
    gtk_table_attach(GTK_TABLE(packing_table), hseparator, 1, 5, table_row, table_row+1,
		     GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(hseparator);
    table_row++;
  

    /* injected dose */
    label = gtk_label_new(_("Injected Dose:"));
    gtk_table_attach(GTK_TABLE(packing_table), label, table_column,table_column+1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    dialog->dose_spin = gtk_spin_button_new_with_range(0.0, G_MAXDOUBLE, 1.0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->dose_spin), FALSE);
    g_signal_connect(G_OBJECT(dialog->dose_spin), "value_changed", 
		     G_CALLBACK(dialog_change_dose_cb), dialog);
    g_signal_connect(G_OBJECT(dialog->dose_spin), "output",
		     G_CALLBACK(amitk_spin_button_scientific_output), NULL);
    gtk_table_attach(GTK_TABLE(packing_table), dialog->dose_spin,
		     table_column+1,table_column+2,table_row, table_row+1, 
		     GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(dialog->dose_spin);

    /* injected dose units */
    dialog->dose_unit_menu = gtk_combo_box_new_text();
    for (i_dose_unit=0; i_dose_unit<AMITK_DOSE_UNIT_NUM; i_dose_unit++) 
      gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->dose_unit_menu),
				amitk_dose_unit_names[i_dose_unit]);				
    g_signal_connect(G_OBJECT(dialog->dose_unit_menu), "changed", 
		     G_CALLBACK(dialog_change_dose_unit_cb), dialog);
    gtk_table_attach(GTK_TABLE(packing_table), dialog->dose_unit_menu, 
		     table_column+2, table_column+3, table_row,table_row+1, 
		     GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(dialog->dose_unit_menu);
    table_row++;

    

    /* subject weight */
    label = gtk_label_new(_("Subject Weight:"));
    gtk_table_attach(GTK_TABLE(packing_table), label, table_column,table_column+1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    dialog->weight_spin = gtk_spin_button_new_with_range(0.0, G_MAXDOUBLE, 1.0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->weight_spin), FALSE);
    g_signal_connect(G_OBJECT(dialog->weight_spin), "value_changed", 
		     G_CALLBACK(dialog_change_weight_cb), dialog);
    g_signal_connect(G_OBJECT(dialog->weight_spin), "output",
		     G_CALLBACK(amitk_spin_button_scientific_output), NULL);
    gtk_table_attach(GTK_TABLE(packing_table), dialog->weight_spin,
		     table_column+1,table_column+2,table_row, table_row+1, 
		     GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(dialog->weight_spin);

    /* subject weight units */
    dialog->weight_unit_menu = gtk_combo_box_new_text();
    for (i_weight_unit=0; i_weight_unit<AMITK_WEIGHT_UNIT_NUM; i_weight_unit++) 
      gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->weight_unit_menu),
				amitk_weight_unit_names[i_weight_unit]);
    g_signal_connect(G_OBJECT(dialog->weight_unit_menu), "changed", 
		     G_CALLBACK(dialog_change_weight_unit_cb), dialog);
    gtk_table_attach(GTK_TABLE(packing_table), dialog->weight_unit_menu, 
		     table_column+2, table_column+3, table_row,table_row+1, 
		     GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(dialog->weight_unit_menu);
    table_row++;
    


    /* cylinder factor */
    label = gtk_label_new(_("Cylinder Factor:"));
    gtk_table_attach(GTK_TABLE(packing_table), label, table_column,table_column+1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    dialog->cylinder_spin = gtk_spin_button_new_with_range(0.0, G_MAXDOUBLE, 1.0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->cylinder_spin), FALSE);
    g_signal_connect(G_OBJECT(dialog->cylinder_spin), "value_changed", 
		     G_CALLBACK(dialog_change_cylinder_cb), dialog);
    g_signal_connect(G_OBJECT(dialog->cylinder_spin), "output",
		     G_CALLBACK(amitk_spin_button_scientific_output), NULL);
    gtk_table_attach(GTK_TABLE(packing_table), dialog->cylinder_spin,
		     table_column+1,table_column+2,table_row, table_row+1, 
		     GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(dialog->cylinder_spin);

    /* cylinder factor units */
    dialog->cylinder_unit_menu = gtk_combo_box_new_text();
    for (i_cylinder_unit=0; i_cylinder_unit<AMITK_CYLINDER_UNIT_NUM; i_cylinder_unit++) 
      gtk_combo_box_append_text(GTK_COMBO_BOX(dialog->cylinder_unit_menu),
				amitk_cylinder_unit_names[i_cylinder_unit]);				
    g_signal_connect(G_OBJECT(dialog->cylinder_unit_menu), "changed", 
		     G_CALLBACK(dialog_change_cylinder_unit_cb), dialog);
    gtk_table_attach(GTK_TABLE(packing_table), dialog->cylinder_unit_menu, 
		     table_column+2, table_column+3, table_row,table_row+1, 
		     GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(dialog->cylinder_unit_menu);
    table_row++;
    



    dialog_update_interpolation(dialog);
    dialog_update_rendering(dialog);
    dialog_update_conversion(dialog);
    
  } else if (AMITK_IS_STUDY(object)) {
    /* widgets to change the study's creation date */
    label = gtk_label_new(_("Creation Date:"));
    gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    dialog->creation_date_entry = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(dialog->creation_date_entry), TRUE);
    g_signal_connect(G_OBJECT(dialog->creation_date_entry), "changed", G_CALLBACK(dialog_change_creation_date_cb), dialog);
    gtk_table_attach(GTK_TABLE(packing_table), dialog->creation_date_entry,1,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(dialog->creation_date_entry);
    table_row++;
 
  }
  

  /* ---------------------------
     Center adjustment page
     --------------------------- 
     
     notes:

     - shifting the center of an entire study doesn't mean anything,
     instead, the option is to change the view center

  */
  
  /* keep this on page 1 for fiducial points */
  if (AMITK_IS_FIDUCIAL_MARK(object)) {
    /* a separator for clarity */
    hseparator = gtk_hseparator_new();
    gtk_table_attach(GTK_TABLE(packing_table), hseparator, 0, 4, 
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(hseparator);
    table_row++;
  } else {
    /* the next page of options */
    packing_table = gtk_table_new(7,7,FALSE);
    table_row=0;
    if (AMITK_IS_STUDY(object))
      label = gtk_label_new(_("View Center"));
    else
      label = gtk_label_new(_("Center"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), packing_table, label);
    gtk_widget_show(label);
  }
  

  if (AMITK_IS_STUDY(object))
    label = gtk_label_new(_("View Center (mm from origin)"));
  else
    label = gtk_label_new(_("Center Location (mm from origin)"));
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,2,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(label);
  table_row++;

  /* location, and dimensions for data set's */
  for (i_axis=0; i_axis<AMITK_AXIS_NUM; i_axis++) {
    label = gtk_label_new(amitk_axis_get_name(i_axis));
    gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);
    
    dialog->center_spin[i_axis] = 
      gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, DIMENSION_STEP);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->center_spin[i_axis]), FALSE);
    g_object_set_data(G_OBJECT(dialog->center_spin[i_axis]), "axis", GINT_TO_POINTER(i_axis));
    g_signal_connect(G_OBJECT(dialog->center_spin[i_axis]), "value_changed", 
		     G_CALLBACK(dialog_change_center_cb), dialog);
    g_signal_connect(G_OBJECT(dialog->center_spin[i_axis]), "output",
		     G_CALLBACK(amitk_spin_button_scientific_output), NULL);
    gtk_table_attach(GTK_TABLE(packing_table), dialog->center_spin[i_axis],1,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(dialog->center_spin[i_axis]);
    
    table_row++;
  }

  if (AMITK_IS_STUDY(object)) {
    button = gtk_button_new_with_label("Shift all objects so view center is origin");
    gtk_table_attach(GTK_TABLE(packing_table), button, 0,3,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(dialog_set_view_center_to_origin_cb), object);
    gtk_widget_show(button);
    table_row++;

  }
  
  /* a separator for clarity */
  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator, 0, 5, table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(hseparator);
  table_row++;

  /* a canvas to indicate which way is x, y, and z */
  axis_indicator = ui_common_create_view_axis_indicator(layout);
  gtk_table_attach(GTK_TABLE(packing_table), axis_indicator,0,5,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(axis_indicator);
  
  table_row++;
  
  gtk_widget_show(packing_table);


  /* ---------------------------
     Voxel Size Page for Data Set's
     --------------------------- */

  if (AMITK_IS_DATA_SET(object)) {

    /* the next page of options */
    packing_table = gtk_table_new(4,2,FALSE);
    table_row=0;
    label = gtk_label_new(_("Voxel Size"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), packing_table, label);
    gtk_widget_show(label);

    label = gtk_label_new(_("Voxel Size (mm)"));
    gtk_table_attach(GTK_TABLE(packing_table), label, 0,2,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);
    table_row++;
      
    for (i_axis=0; i_axis<AMITK_AXIS_NUM; i_axis++) {
      label = gtk_label_new(amitk_axis_get_name(i_axis));
      gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(label);

      dialog->voxel_size_spin[i_axis] = 
	gtk_spin_button_new_with_range(0.0, G_MAXDOUBLE, DIMENSION_STEP);
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->voxel_size_spin[i_axis]), FALSE);
      g_object_set_data(G_OBJECT(dialog->voxel_size_spin[i_axis]), "axis", 
			GINT_TO_POINTER(i_axis));
      g_signal_connect(G_OBJECT(dialog->voxel_size_spin[i_axis]), "value_changed", 
		       G_CALLBACK(dialog_change_voxel_size_cb), dialog);
      g_signal_connect(G_OBJECT(dialog->voxel_size_spin[i_axis]), "output",
		       G_CALLBACK(amitk_spin_button_scientific_output), NULL);
      gtk_table_attach(GTK_TABLE(packing_table), dialog->voxel_size_spin[i_axis],1,2,
		       table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(dialog->voxel_size_spin[i_axis]);
      table_row++;
    }	
    
    check_button = gtk_check_button_new_with_label ("keep aspect ratio");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), 
				 dialog->aspect_ratio);
    g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(dialog_aspect_ratio_cb), dialog);
    gtk_table_attach(GTK_TABLE(packing_table), check_button,1,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(check_button);
    table_row++;

    gtk_widget_show(packing_table);
  }

  /* ---------------------------
     Dimension adjustment page for ROI's

     notes:
     3D Isocontours/Freehands have no adjustable dimensions
     2D Isocontours/Freehands can have their z dimension adjusted
     --------------------------- */
  
  if (AMITK_IS_ROI(object)) {
    if ((AMITK_ROI_TYPE(object) != AMITK_ROI_TYPE_ISOCONTOUR_3D) &&
	(AMITK_ROI_TYPE(object) != AMITK_ROI_TYPE_FREEHAND_3D)) {
      
      /* the next page of options */
      packing_table = gtk_table_new(4,2,FALSE);
      table_row=0;
      label = gtk_label_new(_("Dimensions"));
      gtk_notebook_append_page(GTK_NOTEBOOK(notebook), packing_table, label);
      gtk_widget_show(label);

      /* widgets to change the dimensions of the objects (in object's space) */
      label = gtk_label_new(_("Dimensions (mm) wrt to ROI"));
      gtk_table_attach(GTK_TABLE(packing_table), label, 0,2,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(label);
      table_row++;
      
      for (i_axis=0; i_axis<AMITK_AXIS_NUM; i_axis++) {
	if (((AMITK_ROI_TYPE(object) != AMITK_ROI_TYPE_ISOCONTOUR_2D) &&
	     (AMITK_ROI_TYPE(object) != AMITK_ROI_TYPE_FREEHAND_2D)) || 
	    (i_axis == AMITK_AXIS_Z)) {
	
	  /**************/
	  label = gtk_label_new(amitk_axis_get_name(i_axis));
	  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
			   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
	  gtk_widget_show(label);
	
	  dialog->dimension_spin[i_axis] = 
	    gtk_spin_button_new_with_range(0.0, G_MAXDOUBLE, DIMENSION_STEP);
	  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->dimension_spin[i_axis]), FALSE);
	  g_object_set_data(G_OBJECT(dialog->dimension_spin[i_axis]), "axis", GINT_TO_POINTER(i_axis));
	  g_signal_connect(G_OBJECT(dialog->dimension_spin[i_axis]), "value_changed", 
			   G_CALLBACK(dialog_change_dim_cb), dialog);
	  g_signal_connect(G_OBJECT(dialog->dimension_spin[i_axis]), "output",
			   G_CALLBACK(amitk_spin_button_scientific_output), NULL);
	  gtk_table_attach(GTK_TABLE(packing_table), dialog->dimension_spin[i_axis],1,2,
			   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
	  gtk_widget_show(dialog->dimension_spin[i_axis]);
	  table_row++;
	}
      }

      gtk_widget_show(packing_table);
    }
  }
    
  /* ----------------------------------------
     Rotations page
     ---------------------------------------- 
     notes: 

     -fiducial points don't need to be rotated,as they're 0 dimensional 
  */

  if (!AMITK_IS_FIDUCIAL_MARK(object)) {
    label = gtk_label_new(_("Rotate"));
    space_edit = amitk_space_edit_new(object);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), space_edit, label);
    gtk_widget_show(space_edit);
    gtk_widget_show(label);

  }

  /* ----------------------------------------
     Colormap/threshold page
     ---------------------------------------- */

  if (AMITK_IS_DATA_SET(object)) {
    GtkWidget * threshold;

    label = gtk_label_new(_("Colormap/Threshold"));
    threshold = amitk_threshold_new(AMITK_DATA_SET(object), AMITK_THRESHOLD_BOX_LAYOUT, 
				    GTK_WINDOW(dialog), FALSE);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), threshold, label);
    gtk_widget_show(label);
    gtk_widget_show(threshold);
  }

  /* ----------------------------------------
     Time page
     ---------------------------------------- */

  if (AMITK_IS_DATA_SET(object)) {
    GtkWidget * secondary_table;
    GtkWidget * scrolled;

    /* start making the page to adjust time values */
    label = gtk_label_new(_("Time/Gate"));
    packing_table = gtk_table_new(4,5,FALSE);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), packing_table, label);
    table_row=0;
    gtk_widget_show(label);


    /* scan start time..... */
    label = gtk_label_new(_("Scan Start Time (s)"));
    gtk_table_attach(GTK_TABLE(packing_table),label, 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    dialog->start_spin = gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 1.0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->start_spin), FALSE);
    g_signal_connect(GTK_OBJECT(dialog->start_spin), "value_changed", 
		     G_CALLBACK(dialog_change_scan_start_cb), dialog);
    g_signal_connect(G_OBJECT(dialog->start_spin), "output",
		     G_CALLBACK(amitk_spin_button_scientific_output), NULL);
    gtk_table_attach(GTK_TABLE(packing_table), dialog->start_spin,1,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(dialog->start_spin);
    table_row++;


    /* a separator for clarity */
    hseparator = gtk_hseparator_new();
    gtk_table_attach(GTK_TABLE(packing_table), hseparator,0,2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(hseparator);
    table_row++;


    /* frame duration(s).... */
    label = gtk_label_new(_("Frame"));
    gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    label = gtk_label_new(_("Duration (s)"));
    gtk_table_attach(GTK_TABLE(packing_table), label, 1,2,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);
    table_row++;

    /* make a scrolled area for the info */
    scrolled = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				   GTK_POLICY_NEVER,
				   GTK_POLICY_AUTOMATIC);
    secondary_table = gtk_table_new(AMITK_DATA_SET_NUM_FRAMES(object),2,TRUE);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), secondary_table);
    gtk_widget_show(secondary_table);

    gtk_table_attach(GTK_TABLE(packing_table), scrolled, 0,2,
		     table_row, table_row+1, 0, GTK_FILL|GTK_EXPAND, X_PADDING, Y_PADDING);

    /* get memory for the spin buttons */
    dialog->duration_spins = g_try_new(GtkWidget *,AMITK_DATA_SET_NUM_FRAMES(object));

    /* iterate throught the frames */
    for (i=0; i< AMITK_DATA_SET_NUM_FRAMES(object); i++) {
      
      /* this frame's label */
      temp_string = g_strdup_printf("%d", i);
      label = gtk_label_new(temp_string);
      g_free(temp_string);
      gtk_table_attach(GTK_TABLE(secondary_table), label, 0,1, i, i+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(label);
      
      /* and this frame's spin_button */

      dialog->duration_spins[i] = gtk_spin_button_new_with_range(0.0, G_MAXDOUBLE, 1.0);
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->duration_spins[i]), FALSE);
      gtk_editable_set_editable(GTK_EDITABLE(dialog->duration_spins[i]), TRUE);
      g_object_set_data(G_OBJECT(dialog->duration_spins[i]), "frame", GINT_TO_POINTER(i));
      g_signal_connect(G_OBJECT(dialog->duration_spins[i]), "value_changed", 
		       G_CALLBACK(dialog_change_frame_duration_cb), dialog);
      g_signal_connect(G_OBJECT(dialog->duration_spins[i]), "output",
		       G_CALLBACK(amitk_spin_button_scientific_output), NULL);
      gtk_table_attach(GTK_TABLE(secondary_table), dialog->duration_spins[i],1,2,
		       i, i+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(dialog->duration_spins[i]);
      
    }
    gtk_widget_show(scrolled);



    /* a separator for clarity */
    vseparator = gtk_vseparator_new();
    gtk_table_attach(GTK_TABLE(packing_table), vseparator,2,3,
		     0, table_row+1, GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
    gtk_widget_show(vseparator);
    table_row=0;


    /* skip two rows to make things line up well */
    table_row +=2;
    
    /* gate time(s).... */
    label = gtk_label_new(_("Gate"));
    gtk_table_attach(GTK_TABLE(packing_table), label, 3,4,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    label = gtk_label_new(_("Trigger Time (s)"));
    gtk_table_attach(GTK_TABLE(packing_table), label, 4,5,
		     table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);
    table_row++;

    /* make a scrolled area for the info */
    scrolled = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				   GTK_POLICY_NEVER,
				   GTK_POLICY_AUTOMATIC);
    secondary_table = gtk_table_new(AMITK_DATA_SET_NUM_GATES(object),2,TRUE);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), secondary_table);
    gtk_widget_show(secondary_table);

    gtk_table_attach(GTK_TABLE(packing_table), scrolled, 3,5,
		     table_row, table_row+1, 0, GTK_FILL|GTK_EXPAND, X_PADDING, Y_PADDING);

    /* get memory for the spin buttons */
    dialog->gate_spins = g_try_new(GtkWidget *,AMITK_DATA_SET_NUM_GATES(object));

    /* iterate throught the gates */
    for (i=0; i< AMITK_DATA_SET_NUM_GATES(object); i++) {
      
      /* this frame's label */
      temp_string = g_strdup_printf("%d", i);
      label = gtk_label_new(temp_string);
      g_free(temp_string);
      gtk_table_attach(GTK_TABLE(secondary_table), label, 0,1, i, i+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(label);
      
      /* and this gates spin_button */
      dialog->gate_spins[i] = gtk_spin_button_new_with_range(0.0, G_MAXDOUBLE, 1.0);
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(dialog->gate_spins[i]), FALSE);
      gtk_editable_set_editable(GTK_EDITABLE(dialog->gate_spins[i]), TRUE);
      g_object_set_data(G_OBJECT(dialog->gate_spins[i]), "gate", GINT_TO_POINTER(i));
      g_signal_connect(G_OBJECT(dialog->gate_spins[i]), "value_changed", 
		       G_CALLBACK(dialog_change_gate_time_cb), dialog);
      g_signal_connect(G_OBJECT(dialog->gate_spins[i]), "output",
		       G_CALLBACK(amitk_spin_button_scientific_output), NULL);
      gtk_table_attach(GTK_TABLE(secondary_table), dialog->gate_spins[i],1,2,
		       i, i+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(dialog->gate_spins[i]);
      
    }
    gtk_widget_show(scrolled);
    table_row++;

    gtk_widget_show(packing_table);
  }

  /* ----------------------------------------
     Misc. Preferences Page
     ---------------------------------------- */
  if (AMITK_IS_STUDY(object) || AMITK_IS_DATA_SET(object)) {

    packing_table = gtk_table_new(4,2,FALSE);
    if (AMITK_IS_STUDY(object))
      label = gtk_label_new(_("ROI/View Preferences"));
    else /* AMITK_IS_DATA_SET */
      label = gtk_label_new(_("Windowing Prefs"));
    table_row=0;
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), packing_table, label);
    gtk_widget_show(label);

    if (AMITK_IS_STUDY(object)) {

      ui_common_study_preferences_widgets(packing_table, table_row,
					  &(dialog->roi_width_spin),
					  &(dialog->roi_item), 
#ifdef AMIDE_LIBGNOMECANVAS_AA
					  &(dialog->roi_transparency_spin),
#else
					  &(dialog->line_style_menu),
					  &(dialog->fill_roi_button),
#endif
					  &(dialog->layout_button1), 
					  &(dialog->layout_button2), 
					  &(dialog->panel_layout_button1), 
					  &(dialog->panel_layout_button2), 
					  &(dialog->panel_layout_button3), 
					  &(dialog->maintain_size_button),
					  &(dialog->target_size_spin));


      g_signal_connect(G_OBJECT(dialog->roi_width_spin), "value_changed",  
		       G_CALLBACK(dialog_change_roi_width_cb), dialog);
#ifdef AMIDE_LIBGNOMECANVAS_AA
      g_signal_connect(G_OBJECT(dialog->roi_transparency_spin), "value_changed",  
		       G_CALLBACK(dialog_change_roi_transparency_cb), dialog);
#else
      g_signal_connect(G_OBJECT(dialog->line_style_menu), "changed", 
		       G_CALLBACK(dialog_change_line_style_cb), dialog);
      g_signal_connect(G_OBJECT(dialog->fill_roi_button), "toggled", 
		       G_CALLBACK(dialog_change_fill_roi_cb), dialog);
#endif
      g_signal_connect(G_OBJECT(dialog->layout_button1), "clicked", 
		       G_CALLBACK(dialog_change_layout_cb), dialog);
      g_signal_connect(G_OBJECT(dialog->layout_button2), "clicked", 
		       G_CALLBACK(dialog_change_layout_cb), dialog);
      g_signal_connect(G_OBJECT(dialog->panel_layout_button1), "clicked", 
		       G_CALLBACK(dialog_change_panel_layout_cb), dialog);
      g_signal_connect(G_OBJECT(dialog->panel_layout_button2), "clicked", 
		       G_CALLBACK(dialog_change_panel_layout_cb), dialog);
      g_signal_connect(G_OBJECT(dialog->panel_layout_button3), "clicked", 
		       G_CALLBACK(dialog_change_panel_layout_cb), dialog);
      g_signal_connect(G_OBJECT(dialog->maintain_size_button), "toggled", 
		       G_CALLBACK(dialog_change_maintain_size_cb), dialog);
      g_signal_connect(G_OBJECT(dialog->target_size_spin), "value_changed",  
		       G_CALLBACK(dialog_change_target_empty_area_cb), dialog);


    } else if (AMITK_IS_DATA_SET(object)) {
      GtkWidget * windows_widget;
      GtkWidget * scrolled;

      scrolled = gtk_scrolled_window_new(NULL, NULL);
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				     GTK_POLICY_NEVER,
				     GTK_POLICY_AUTOMATIC);

      windows_widget = amitk_window_edit_new(AMITK_DATA_SET(object), NULL);
      gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), windows_widget);
      gtk_widget_show(windows_widget);

      gtk_table_attach(GTK_TABLE(packing_table), scrolled, 0,4,
		       table_row, table_row+1, 0, GTK_FILL|GTK_EXPAND, X_PADDING, Y_PADDING);
      gtk_widget_show(scrolled);

    }
    
    gtk_widget_show(packing_table);
  }


  /* ----------------------------------------
     Specify Color Page 
     ---------------------------------------- */
  if (AMITK_IS_ROI(object) || AMITK_IS_FIDUCIAL_MARK(object)) {
    rgba_t color_rgba;
    GdkColor color_gdk;
    guint16 alpha_gdk;
    GtkWidget * color_sel;


    packing_table = gtk_table_new(2,2,FALSE);
    label = gtk_label_new(_("Color"));
    table_row=0;
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), packing_table, label);
    gtk_widget_show(label);

    check_button = gtk_check_button_new_with_label ("Specify Color");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), 
				 AMITK_IS_ROI(object) ? AMITK_ROI_SPECIFY_COLOR(object) :
				 AMITK_FIDUCIAL_MARK_SPECIFY_COLOR(object));
    g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(dialog_specify_color_cb), dialog);
    gtk_table_attach(GTK_TABLE(packing_table), check_button,0,1,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(check_button);
    table_row++;

    
    color_rgba = AMITK_IS_ROI(object) ? AMITK_ROI_COLOR(object) : AMITK_FIDUCIAL_MARK_COLOR(object);
    color_gdk.red = color_rgba.r << 8;
    color_gdk.green = color_rgba.g << 8;
    color_gdk.blue = color_rgba.b << 8;
    alpha_gdk = color_rgba.a << 8;
  

    color_sel = gtk_color_selection_new();
    gtk_color_selection_set_has_opacity_control(GTK_COLOR_SELECTION(color_sel), TRUE);
    gtk_color_selection_set_has_palette(GTK_COLOR_SELECTION(color_sel), TRUE);
    gtk_color_selection_set_current_alpha(GTK_COLOR_SELECTION(color_sel), alpha_gdk);
    gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(color_sel), &color_gdk);
    g_signal_connect(G_OBJECT(color_sel), "color_changed", G_CALLBACK(dialog_change_color_cb), dialog);
    gtk_table_attach(GTK_TABLE(packing_table), color_sel, 0, 2,
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(color_sel);
    table_row++;


    gtk_widget_show(packing_table);
  }


  /* ----------------------------------------
     Immutables Page:
     ---------------------------------------- */

  immutables = FALSE;
  if (AMITK_IS_STUDY(object))
    immutables = TRUE;
  else if (AMITK_IS_DATA_SET(object))
    immutables = TRUE;
  else if (AMITK_IS_ROI(object)) {
    if (AMITK_ROI_TYPE_ISOCONTOUR(object))
      immutables = TRUE;
  }
    
  if (immutables) {

    packing_table = gtk_table_new(12,5,FALSE);
    label = gtk_label_new(_("Immutables"));
    table_row=0;
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), packing_table, label);
    gtk_widget_show(label);
    
    if (AMITK_IS_STUDY(object)) {
      label = gtk_label_new(_("Max-Min Voxel Dim"));
      gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(label);
      
      dialog->voxel_dim_entry = gtk_entry_new();
      gtk_editable_set_editable(GTK_EDITABLE(dialog->voxel_dim_entry), FALSE);
      gtk_table_attach(GTK_TABLE(packing_table), dialog->voxel_dim_entry,1,2,
		       table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(dialog->voxel_dim_entry);
      
      table_row++;
      
    } else if (AMITK_IS_ROI(object)) {
      if (AMITK_ROI_TYPE_ISOCONTOUR(object)) {

	label = gtk_label_new(_("Isocontour Min Specified Value"));
	gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
			 table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
	gtk_widget_show(label);
	
	dialog->isocontour_min_value_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(dialog->isocontour_min_value_entry), FALSE);
	gtk_table_attach(GTK_TABLE(packing_table), dialog->isocontour_min_value_entry,1,2,
			 table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
	gtk_widget_show(dialog->isocontour_min_value_entry);
	
	table_row++;

	label = gtk_label_new(_("Isocontour Max Specified Value"));
	gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
			 table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
	gtk_widget_show(label);
	
	dialog->isocontour_max_value_entry = gtk_entry_new();
	gtk_editable_set_editable(GTK_EDITABLE(dialog->isocontour_max_value_entry), FALSE);
	gtk_table_attach(GTK_TABLE(packing_table), dialog->isocontour_max_value_entry,1,2,
			 table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
	gtk_widget_show(dialog->isocontour_max_value_entry);
	
	table_row++;
      }
    } else if (AMITK_IS_DATA_SET(object)) {
      AmitkDim i_dim;
      GtkWidget * entry;
      gdouble memory_used;
      AmitkRawData * rd;
      gint prefix=0;

      rd = AMITK_DATA_SET_RAW_DATA(object);
      memory_used = amitk_raw_data_size_data_mem(rd);

      if ((memory_used/1024.0) > 1.0) {
	memory_used /= 1024.0;
	prefix=1;
      }

      if ((memory_used/1024.0) > 1.0) {
	memory_used /= 1024.0;
	prefix=2;
      }

      if ((memory_used/1024.0) > 1.0) {
	memory_used /= 1024.0;
	prefix=3;
      }

      /* how big in memory the raw data is */
      switch(prefix) {
      case 3:
	label = gtk_label_new(_("Memory Used (GB):"));
	break;
      case 2:
	label = gtk_label_new(_("Memory Used (MB):"));
	break;
      case 1:
	label = gtk_label_new(_("Memory Used (KB):"));
	break;
      case 0:
      default:
	label = gtk_label_new(_("Memory Used (bytes):"));
	break;
      }
      gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(label);
      
      entry = gtk_entry_new();
      temp_string = g_strdup_printf("%5.3f", memory_used);
      gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
      g_free(temp_string);
      gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
      gtk_table_attach(GTK_TABLE(packing_table), entry,
		       1,2, table_row, table_row+1, 
		       GTK_FILL, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(entry);
      table_row++;
      
      /* widget to tell you the internal data format */
      label = gtk_label_new(_("Data Format:"));
      gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(label);
      
      entry = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(entry), 
			 amitk_format_names[AMITK_DATA_SET_FORMAT(object)]);
      gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
      gtk_table_attach(GTK_TABLE(packing_table), entry,
		       1,2, table_row, table_row+1, 
		       GTK_FILL, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(entry);
      table_row++;
      
      /* a separator for clarity */
      hseparator = gtk_hseparator_new();
      gtk_table_attach(GTK_TABLE(packing_table), hseparator,0,2,
		       table_row, table_row+1, GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
      gtk_widget_show(hseparator);
      table_row++;
      
      /* widget to tell you the scaling format */
      label = gtk_label_new(_("Scale Format:"));
      gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(label);
      
      entry = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(entry), 
			 amitk_scaling_menu_names[AMITK_DATA_SET_SCALING_TYPE(object)]);
      gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
      gtk_table_attach(GTK_TABLE(packing_table), entry,
		       1,2, table_row, table_row+1, 
		       GTK_FILL, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(entry);
      table_row++;
      
      /* a separator for clarity */
      hseparator = gtk_hseparator_new();
      gtk_table_attach(GTK_TABLE(packing_table), hseparator,0,2,
		       table_row, table_row+1, GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
      gtk_widget_show(hseparator);
      table_row++;
      
      /* widgets to display the data set dimensions */
      label = gtk_label_new(_("Data Set Dimensions (voxels)"));
      gtk_table_attach(GTK_TABLE(packing_table), label, 0,2,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(label);
      table_row++;
      
      /**************/
      for (i_dim=0; i_dim < AMITK_DIM_NUM; i_dim++) {

	label = gtk_label_new(amitk_dim_get_name(i_dim));
	gtk_table_attach(GTK_TABLE(packing_table), label, 0,1,
			 table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
	gtk_widget_show(label);
	
	entry = gtk_entry_new();
	temp_string = g_strdup_printf("%d", voxel_get_dim(AMITK_DATA_SET_DIM(object),i_dim));
	gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
	g_free(temp_string);
	gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
	gtk_table_attach(GTK_TABLE(packing_table), entry,1,2,
			 table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
	gtk_widget_show(entry);
	table_row++;
      }


      /* a separator for clarity */
      vseparator = gtk_vseparator_new();
      gtk_table_attach(GTK_TABLE(packing_table), vseparator,2,3,
		       0, table_row, GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
      gtk_widget_show(vseparator);

      table_row=0;

      /* MRI parameters */
      label = gtk_label_new(_("MRI Parameters"));
      gtk_table_attach(GTK_TABLE(packing_table), label, 3,5,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(label);
      table_row++;



      /* inversion time */
      label = gtk_label_new(_("Inversion Time (ms):"));
      gtk_table_attach(GTK_TABLE(packing_table), label, 3,4,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(label);
      
      entry = gtk_entry_new();
      temp_string = g_strdup_printf("%f", AMITK_DATA_SET_INVERSION_TIME(object));
      gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
      g_free(temp_string);
			 
      gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
      gtk_table_attach(GTK_TABLE(packing_table), entry,
		       4,5, table_row, table_row+1, 
		       GTK_FILL, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(entry);
      table_row++;

      /* echo time */
      label = gtk_label_new(_("Echo Time (ms):"));
      gtk_table_attach(GTK_TABLE(packing_table), label, 3,4,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(label);
      
      entry = gtk_entry_new();
      temp_string = g_strdup_printf("%f", AMITK_DATA_SET_ECHO_TIME(object));
      gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
      g_free(temp_string);
			 
      gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
      gtk_table_attach(GTK_TABLE(packing_table), entry,
		       4,5, table_row, table_row+1, 
		       GTK_FILL, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(entry);
      table_row++;

      /* b factor */
      label = gtk_label_new(_("Diffusion B Value:"));
      gtk_table_attach(GTK_TABLE(packing_table), label, 3,4,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(label);
      
      entry = gtk_entry_new();
      temp_string = g_strdup_printf("%f", AMITK_DATA_SET_DIFFUSION_B_VALUE(object));
      gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
      g_free(temp_string);
			 
      gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
      gtk_table_attach(GTK_TABLE(packing_table), entry,
		       4,5, table_row, table_row+1, 
		       GTK_FILL, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(entry);
      table_row++;

      /* diffusion direction */
      label = gtk_label_new(_("Diffusion Direction:"));
      gtk_table_attach(GTK_TABLE(packing_table), label, 3,4,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(label);
      
      entry = gtk_entry_new();
      temp_string = g_strdup_printf("%5.3f %5.3f %5.3f", 
				    AMITK_DATA_SET_DIFFUSION_DIRECTION(object).x,
				    AMITK_DATA_SET_DIFFUSION_DIRECTION(object).y,
				    AMITK_DATA_SET_DIFFUSION_DIRECTION(object).z);
      gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
      g_free(temp_string);
			 
      gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
      gtk_table_attach(GTK_TABLE(packing_table), entry,
		       4,5, table_row, table_row+1, 
		       GTK_FILL, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(entry);
      table_row++;

      /* a separator for clarity */
      hseparator = gtk_hseparator_new();
      gtk_table_attach(GTK_TABLE(packing_table), hseparator,3,5,
		       table_row, table_row+1, GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
      gtk_widget_show(hseparator);
      table_row++;

      /* additional parameters */
      label = gtk_label_new(_("Misc. Parameters"));
      gtk_table_attach(GTK_TABLE(packing_table), label, 3,5,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(label);
      table_row++;

      /* series number */
      label = gtk_label_new(_("Series Number:"));
      gtk_table_attach(GTK_TABLE(packing_table), label, 3,4,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(label);
      
      entry = gtk_entry_new();
      temp_string = g_strdup_printf("%d", AMITK_DATA_SET_SERIES_NUMBER(object));
      gtk_entry_set_text(GTK_ENTRY(entry), temp_string);
      g_free(temp_string);
			 
      gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
      gtk_table_attach(GTK_TABLE(packing_table), entry,
		       4,5, table_row, table_row+1, 
		       GTK_FILL, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(entry);
      table_row++;

      /* dicom image type */
      label = gtk_label_new(_("Dicom Image Type:"));
      gtk_table_attach(GTK_TABLE(packing_table), label, 3,4,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(label);
      
      entry = gtk_entry_new();
      if (AMITK_DATA_SET_DICOM_IMAGE_TYPE(object) != NULL)
	gtk_entry_set_text(GTK_ENTRY(entry), AMITK_DATA_SET_DICOM_IMAGE_TYPE(object));
			 
      gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
      gtk_table_attach(GTK_TABLE(packing_table), entry,
		       4,5, table_row, table_row+1, 
		       GTK_FILL, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(entry);
      table_row++;
    }

    gtk_widget_show(packing_table);
  }

  return;
}

static void dialog_update_entries(AmitkObjectDialog * dialog) {
  
  AmitkAxis i_axis;
  AmitkPoint center;
  gchar * temp_str;
  gint i;
  gboolean immutables;
  gboolean center_valid = TRUE;

  /* object name */
  g_signal_handlers_block_by_func(G_OBJECT(dialog->name_entry),G_CALLBACK(dialog_change_name_cb), dialog);
  if (AMITK_OBJECT_NAME(dialog->object) != NULL)
    gtk_entry_set_text(GTK_ENTRY(dialog->name_entry), AMITK_OBJECT_NAME(dialog->object));
  g_signal_handlers_unblock_by_func(G_OBJECT(dialog->name_entry), G_CALLBACK(dialog_change_name_cb), dialog);
  
  if (AMITK_IS_ROI(dialog->object)) {

    g_signal_handlers_block_by_func(G_OBJECT(dialog->roi_type_menu),G_CALLBACK(dialog_change_roi_type_cb), dialog);
    gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->roi_type_menu), AMITK_ROI_TYPE(dialog->object));
    g_signal_handlers_unblock_by_func(G_OBJECT(dialog->roi_type_menu),G_CALLBACK(dialog_change_roi_type_cb), dialog);

  } else if (AMITK_IS_DATA_SET(dialog->object)) {

    g_signal_handlers_block_by_func(G_OBJECT(dialog->scan_date_entry),G_CALLBACK(dialog_change_scan_date_cb), dialog);
    gtk_entry_set_text(GTK_ENTRY(dialog->scan_date_entry), AMITK_DATA_SET_SCAN_DATE(dialog->object));
    g_signal_handlers_unblock_by_func(G_OBJECT(dialog->scan_date_entry),G_CALLBACK(dialog_change_scan_date_cb), dialog);

    g_signal_handlers_block_by_func(G_OBJECT(dialog->subject_name_entry),G_CALLBACK(dialog_change_subject_name_cb), dialog);
    gtk_entry_set_text(GTK_ENTRY(dialog->subject_name_entry), AMITK_DATA_SET_SUBJECT_NAME(dialog->object));
    g_signal_handlers_unblock_by_func(G_OBJECT(dialog->subject_name_entry),G_CALLBACK(dialog_change_subject_name_cb), dialog);

    g_signal_handlers_block_by_func(G_OBJECT(dialog->subject_id_entry),G_CALLBACK(dialog_change_subject_id_cb), dialog);
    gtk_entry_set_text(GTK_ENTRY(dialog->subject_id_entry), AMITK_DATA_SET_SUBJECT_ID(dialog->object));
    g_signal_handlers_unblock_by_func(G_OBJECT(dialog->subject_id_entry),G_CALLBACK(dialog_change_subject_id_cb), dialog);

    g_signal_handlers_block_by_func(G_OBJECT(dialog->subject_dob_entry),G_CALLBACK(dialog_change_subject_dob_cb), dialog);
    gtk_entry_set_text(GTK_ENTRY(dialog->subject_dob_entry), AMITK_DATA_SET_SUBJECT_DOB(dialog->object));
    g_signal_handlers_unblock_by_func(G_OBJECT(dialog->subject_dob_entry),G_CALLBACK(dialog_change_subject_dob_cb), dialog);

    g_signal_handlers_block_by_func(G_OBJECT(dialog->modality_menu),G_CALLBACK(dialog_change_modality_cb), dialog);
    gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->modality_menu), AMITK_DATA_SET_MODALITY(dialog->object));
    g_signal_handlers_unblock_by_func(G_OBJECT(dialog->modality_menu),G_CALLBACK(dialog_change_modality_cb), dialog);

    g_signal_handlers_block_by_func(G_OBJECT(dialog->subject_orientation_menu),G_CALLBACK(dialog_change_subject_orientation_cb), dialog);
    gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->subject_orientation_menu), AMITK_DATA_SET_SUBJECT_ORIENTATION(dialog->object));
    g_signal_handlers_unblock_by_func(G_OBJECT(dialog->subject_orientation_menu),G_CALLBACK(dialog_change_subject_orientation_cb), dialog);

    g_signal_handlers_block_by_func(G_OBJECT(dialog->subject_sex_menu),G_CALLBACK(dialog_change_subject_sex_cb), dialog);
    gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->subject_sex_menu), AMITK_DATA_SET_SUBJECT_SEX(dialog->object));
    g_signal_handlers_unblock_by_func(G_OBJECT(dialog->subject_sex_menu),G_CALLBACK(dialog_change_subject_sex_cb), dialog);

    g_signal_handlers_block_by_func(G_OBJECT(dialog->scaling_factor_spin),
				    G_CALLBACK(dialog_change_scale_factor_cb), dialog);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->scaling_factor_spin),
			      AMITK_DATA_SET_SCALE_FACTOR(dialog->object));
    g_signal_handlers_unblock_by_func(G_OBJECT(dialog->scaling_factor_spin),
				      G_CALLBACK(dialog_change_scale_factor_cb), dialog);

    g_signal_handlers_block_by_func(G_OBJECT(dialog->dose_spin),
				    G_CALLBACK(dialog_change_dose_cb), dialog);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->dose_spin),
			      amitk_dose_unit_convert_to(AMITK_DATA_SET_INJECTED_DOSE(dialog->object),
							 AMITK_DATA_SET_DISPLAYED_DOSE_UNIT(dialog->object)));
    g_signal_handlers_unblock_by_func(G_OBJECT(dialog->dose_spin),
				      G_CALLBACK(dialog_change_dose_cb), dialog);
    g_signal_handlers_block_by_func(G_OBJECT(dialog->dose_unit_menu),
				    G_CALLBACK(dialog_change_dose_unit_cb), dialog);
    gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->dose_unit_menu), 
			     AMITK_DATA_SET_DISPLAYED_DOSE_UNIT(dialog->object));
    g_signal_handlers_unblock_by_func(G_OBJECT(dialog->dose_unit_menu),
				      G_CALLBACK(dialog_change_dose_unit_cb), dialog);


    g_signal_handlers_block_by_func(G_OBJECT(dialog->weight_spin),
				    G_CALLBACK(dialog_change_weight_cb), dialog);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->weight_spin),
			      amitk_weight_unit_convert_to(AMITK_DATA_SET_SUBJECT_WEIGHT(dialog->object),
							   AMITK_DATA_SET_DISPLAYED_WEIGHT_UNIT(dialog->object)));
    g_signal_handlers_unblock_by_func(G_OBJECT(dialog->weight_spin),
				      G_CALLBACK(dialog_change_weight_cb), dialog);
    g_signal_handlers_block_by_func(G_OBJECT(dialog->weight_unit_menu),
				    G_CALLBACK(dialog_change_weight_unit_cb), dialog);
    gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->weight_unit_menu), 
			     AMITK_DATA_SET_DISPLAYED_WEIGHT_UNIT(dialog->object));
    g_signal_handlers_unblock_by_func(G_OBJECT(dialog->weight_unit_menu),
				      G_CALLBACK(dialog_change_weight_unit_cb), dialog);

    g_signal_handlers_block_by_func(G_OBJECT(dialog->cylinder_spin),
				    G_CALLBACK(dialog_change_cylinder_cb), dialog);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->cylinder_spin),
			      amitk_cylinder_unit_convert_to(AMITK_DATA_SET_CYLINDER_FACTOR(dialog->object),
							     AMITK_DATA_SET_DISPLAYED_CYLINDER_UNIT(dialog->object)));
    g_signal_handlers_unblock_by_func(G_OBJECT(dialog->cylinder_spin),
				      G_CALLBACK(dialog_change_cylinder_cb), dialog);
    g_signal_handlers_block_by_func(G_OBJECT(dialog->cylinder_unit_menu),
				    G_CALLBACK(dialog_change_cylinder_unit_cb), dialog);
    gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->cylinder_unit_menu), 
			     AMITK_DATA_SET_DISPLAYED_CYLINDER_UNIT(dialog->object));
    g_signal_handlers_unblock_by_func(G_OBJECT(dialog->cylinder_unit_menu),
				      G_CALLBACK(dialog_change_cylinder_unit_cb), dialog);

  } else if (AMITK_IS_STUDY(dialog->object)) {

    g_signal_handlers_block_by_func(G_OBJECT(dialog->creation_date_entry),G_CALLBACK(dialog_change_creation_date_cb), dialog);
    gtk_entry_set_text(GTK_ENTRY(dialog->creation_date_entry), AMITK_STUDY_CREATION_DATE(dialog->object));
    g_signal_handlers_unblock_by_func(G_OBJECT(dialog->creation_date_entry),G_CALLBACK(dialog_change_creation_date_cb), dialog);


  }

  if (AMITK_IS_VOLUME(dialog->object)) {
    if (AMITK_VOLUME_VALID(dialog->object)) 
      center = amitk_volume_get_center(AMITK_VOLUME(dialog->object));
    else
      center_valid = FALSE;

  } else if (AMITK_IS_STUDY(dialog->object)) {
    center = AMITK_STUDY_VIEW_CENTER(dialog->object);

  } else if (AMITK_IS_FIDUCIAL_MARK(dialog->object)) {
    center = AMITK_FIDUCIAL_MARK_GET(dialog->object);

  } else
    g_return_if_reached();


  for (i_axis=0; i_axis<AMITK_AXIS_NUM; i_axis++) {
    g_signal_handlers_block_by_func(G_OBJECT(dialog->center_spin[i_axis]), 
				    G_CALLBACK(dialog_change_center_cb), dialog);
    gtk_widget_set_sensitive(GTK_WIDGET(dialog->center_spin[i_axis]), center_valid);
    if (center_valid)
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->center_spin[i_axis]), 
				point_get_component(center, i_axis));
    g_signal_handlers_unblock_by_func(G_OBJECT(dialog->center_spin[i_axis]), 
				      G_CALLBACK(dialog_change_center_cb), dialog);

    if (AMITK_IS_DATA_SET(dialog->object)) {
      g_signal_handlers_block_by_func(G_OBJECT(dialog->voxel_size_spin[i_axis]), 
				      G_CALLBACK(dialog_change_voxel_size_cb), dialog);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->voxel_size_spin[i_axis]), 
				point_get_component(AMITK_DATA_SET_VOXEL_SIZE(dialog->object), i_axis));
      g_signal_handlers_unblock_by_func(G_OBJECT(dialog->voxel_size_spin[i_axis]), G_CALLBACK(dialog_change_voxel_size_cb), dialog);
    }
  }

  if (AMITK_IS_ROI(dialog->object)) {
    if ((AMITK_ROI_TYPE(dialog->object) != AMITK_ROI_TYPE_ISOCONTOUR_3D) &&
	(AMITK_ROI_TYPE(dialog->object) != AMITK_ROI_TYPE_FREEHAND_3D)) {
      for (i_axis=0; i_axis<AMITK_AXIS_NUM; i_axis++) {
	if (((AMITK_ROI_TYPE(dialog->object) != AMITK_ROI_TYPE_ISOCONTOUR_2D) &&
	     (AMITK_ROI_TYPE(dialog->object) != AMITK_ROI_TYPE_FREEHAND_2D)) || 
	    (i_axis == AMITK_AXIS_Z)) {
	  g_signal_handlers_block_by_func(G_OBJECT(dialog->dimension_spin[i_axis]), G_CALLBACK(dialog_change_dim_cb), dialog);
	  gtk_widget_set_sensitive(GTK_WIDGET(dialog->dimension_spin[i_axis]), center_valid);
	  gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->dimension_spin[i_axis]), 
				    point_get_component(AMITK_VOLUME_CORNER(dialog->object), i_axis));
	  g_signal_handlers_unblock_by_func(G_OBJECT(dialog->dimension_spin[i_axis]), G_CALLBACK(dialog_change_dim_cb), dialog);
	}
      }
    }
  }


  /* Preferences Page */
  if (AMITK_IS_STUDY(dialog->object)) {

      dialog_update_roi_sample_item(dialog);

      g_signal_handlers_block_by_func(G_OBJECT(dialog->roi_width_spin),  G_CALLBACK(dialog_change_roi_width_cb), dialog);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->roi_width_spin), AMITK_STUDY_CANVAS_ROI_WIDTH(dialog->object));
      g_signal_handlers_unblock_by_func(G_OBJECT(dialog->roi_width_spin),  G_CALLBACK(dialog_change_roi_width_cb), dialog);

#ifdef AMIDE_LIBGNOMECANVAS_AA
      g_signal_handlers_block_by_func(G_OBJECT(dialog->roi_transparency_spin),  G_CALLBACK(dialog_change_roi_transparency_cb), dialog);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->roi_transparency_spin), AMITK_STUDY_CANVAS_ROI_TRANSPARENCY(dialog->object));
      g_signal_handlers_unblock_by_func(G_OBJECT(dialog->roi_transparency_spin),  G_CALLBACK(dialog_change_roi_transparency_cb), dialog);
#else
      g_signal_handlers_block_by_func(G_OBJECT(dialog->line_style_menu), G_CALLBACK(dialog_change_line_style_cb), dialog);
      gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->line_style_menu), AMITK_STUDY_CANVAS_LINE_STYLE(dialog->object));
      g_signal_handlers_unblock_by_func(G_OBJECT(dialog->line_style_menu), G_CALLBACK(dialog_change_line_style_cb), dialog);
      g_signal_handlers_block_by_func(G_OBJECT(dialog->fill_roi_button), G_CALLBACK(dialog_change_fill_roi_cb), dialog);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->fill_roi_button), 
				   AMITK_STUDY_CANVAS_FILL_ROI(dialog->object));
      g_signal_handlers_unblock_by_func(G_OBJECT(dialog->fill_roi_button), G_CALLBACK(dialog_change_fill_roi_cb), dialog);
#endif

      g_signal_handlers_block_by_func(G_OBJECT(dialog->layout_button1), G_CALLBACK(dialog_change_layout_cb), dialog);
      g_signal_handlers_block_by_func(G_OBJECT(dialog->layout_button2),  G_CALLBACK(dialog_change_layout_cb), dialog);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->layout_button1), 
				   (AMITK_STUDY_CANVAS_LAYOUT(dialog->object) == AMITK_LAYOUT_LINEAR));
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->layout_button2), 
				   (AMITK_STUDY_CANVAS_LAYOUT(dialog->object) == AMITK_LAYOUT_ORTHOGONAL));
      g_signal_handlers_unblock_by_func(G_OBJECT(dialog->layout_button1), G_CALLBACK(dialog_change_layout_cb), dialog);
      g_signal_handlers_unblock_by_func(G_OBJECT(dialog->layout_button2),  G_CALLBACK(dialog_change_layout_cb), dialog);

      g_signal_handlers_block_by_func(G_OBJECT(dialog->panel_layout_button1), G_CALLBACK(dialog_change_panel_layout_cb), dialog);
      g_signal_handlers_block_by_func(G_OBJECT(dialog->panel_layout_button2),  G_CALLBACK(dialog_change_panel_layout_cb), dialog);
      g_signal_handlers_block_by_func(G_OBJECT(dialog->panel_layout_button3),  G_CALLBACK(dialog_change_panel_layout_cb), dialog);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->panel_layout_button1), 
				   (AMITK_STUDY_PANEL_LAYOUT(dialog->object) == AMITK_PANEL_LAYOUT_MIXED));
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->panel_layout_button2), 
				   (AMITK_STUDY_PANEL_LAYOUT(dialog->object) == AMITK_PANEL_LAYOUT_LINEAR_X));
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->panel_layout_button3), 
				   (AMITK_STUDY_PANEL_LAYOUT(dialog->object) == AMITK_PANEL_LAYOUT_LINEAR_Y));
      g_signal_handlers_unblock_by_func(G_OBJECT(dialog->panel_layout_button1), G_CALLBACK(dialog_change_panel_layout_cb), dialog);
      g_signal_handlers_unblock_by_func(G_OBJECT(dialog->panel_layout_button2), G_CALLBACK(dialog_change_panel_layout_cb), dialog);
      g_signal_handlers_unblock_by_func(G_OBJECT(dialog->panel_layout_button3), G_CALLBACK(dialog_change_panel_layout_cb), dialog);

      g_signal_handlers_block_by_func(G_OBJECT(dialog->maintain_size_button), G_CALLBACK(dialog_change_maintain_size_cb), dialog);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->maintain_size_button), 
				   AMITK_STUDY_CANVAS_MAINTAIN_SIZE(dialog->object));
      g_signal_handlers_unblock_by_func(G_OBJECT(dialog->maintain_size_button), G_CALLBACK(dialog_change_maintain_size_cb), dialog);
      
      g_signal_handlers_block_by_func(G_OBJECT(dialog->target_size_spin), G_CALLBACK(dialog_change_target_empty_area_cb), dialog);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->target_size_spin), 
				AMITK_STUDY_CANVAS_TARGET_EMPTY_AREA(dialog->object));
      g_signal_handlers_unblock_by_func(G_OBJECT(dialog->target_size_spin), G_CALLBACK(dialog_change_target_empty_area_cb), dialog);

  }

  /* Immutables Page */
  immutables = FALSE;
  if (AMITK_IS_STUDY(dialog->object))
    immutables = TRUE;
  else if (AMITK_IS_DATA_SET(dialog->object))
    immutables = TRUE;
  else if (AMITK_IS_ROI(dialog->object))
    if (AMITK_ROI_TYPE_ISOCONTOUR(dialog->object))
      immutables = TRUE;
    
  if (immutables) {

    if (AMITK_IS_STUDY(dialog->object)) {
      temp_str = g_strdup_printf("%f", AMITK_STUDY_VOXEL_DIM(dialog->object));
      gtk_entry_set_text(GTK_ENTRY(dialog->voxel_dim_entry), temp_str);
      g_free(temp_str);

    } else if AMITK_IS_ROI(dialog->object) {
      if (AMITK_ROI_TYPE_ISOCONTOUR(dialog->object)) {
	temp_str = g_strdup_printf("%f", AMITK_ROI_ISOCONTOUR_MIN_VALUE(dialog->object));
	gtk_entry_set_text(GTK_ENTRY(dialog->isocontour_min_value_entry), temp_str);
	g_free(temp_str);
	gtk_widget_set_sensitive(dialog->isocontour_min_value_entry, 
				 AMITK_ROI_ISOCONTOUR_RANGE(dialog->object) != AMITK_ROI_ISOCONTOUR_RANGE_BELOW_MAX);

	temp_str = g_strdup_printf("%f", AMITK_ROI_ISOCONTOUR_MAX_VALUE(dialog->object));
	gtk_entry_set_text(GTK_ENTRY(dialog->isocontour_max_value_entry), temp_str);
	g_free(temp_str);
	gtk_widget_set_sensitive(dialog->isocontour_max_value_entry, 
				 AMITK_ROI_ISOCONTOUR_RANGE(dialog->object) != AMITK_ROI_ISOCONTOUR_RANGE_ABOVE_MIN);
      }
    } else if (AMITK_IS_DATA_SET(dialog->object)) {
      g_signal_handlers_block_by_func(G_OBJECT(dialog->start_spin), 
				      G_CALLBACK(dialog_change_scan_start_cb), dialog);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->start_spin),
				AMITK_DATA_SET_SCAN_START(dialog->object));
      g_signal_handlers_unblock_by_func(G_OBJECT(dialog->start_spin), 
					G_CALLBACK(dialog_change_scan_start_cb), dialog);
      
      /* iterate throught the frames */
      for (i=0; i< AMITK_DATA_SET_NUM_FRAMES(dialog->object); i++) {
	g_signal_handlers_block_by_func(G_OBJECT(dialog->duration_spins[i]), 
					G_CALLBACK(dialog_change_frame_duration_cb), dialog);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->duration_spins[i]),
				  amitk_data_set_get_frame_duration(AMITK_DATA_SET(dialog->object),i));
	g_signal_handlers_unblock_by_func(G_OBJECT(dialog->duration_spins[i]), 
					  G_CALLBACK(dialog_change_frame_duration_cb), dialog);
      }

      /* iterate throught the gates */
      for (i=0; i< AMITK_DATA_SET_NUM_GATES(dialog->object); i++) {
	g_signal_handlers_block_by_func(G_OBJECT(dialog->gate_spins[i]), 
					G_CALLBACK(dialog_change_gate_time_cb), dialog);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->gate_spins[i]),
				  amitk_data_set_get_gate_time(AMITK_DATA_SET(dialog->object),i));
	g_signal_handlers_unblock_by_func(G_OBJECT(dialog->gate_spins[i]), 
					  G_CALLBACK(dialog_change_gate_time_cb), dialog);
      }
    }
  }

  return;
}


/* set which toggle button is depressed */
static void dialog_update_interpolation(AmitkObjectDialog * dialog) {
  
  AmitkInterpolation i_interpolation;
  AmitkInterpolation interpolation;

  interpolation = AMITK_DATA_SET_INTERPOLATION(dialog->object);
  for (i_interpolation=0; i_interpolation < AMITK_INTERPOLATION_NUM; i_interpolation++)
    g_signal_handlers_block_by_func(G_OBJECT(dialog->interpolation_button[i_interpolation]),
				    G_CALLBACK(dialog_change_interpolation_cb), dialog);
  /* need the button pressed to get the display to update correctly */
  gtk_button_pressed(GTK_BUTTON(dialog->interpolation_button[interpolation]));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->interpolation_button[interpolation]),TRUE);

  for (i_interpolation=0; i_interpolation < AMITK_INTERPOLATION_NUM; i_interpolation++)
    g_signal_handlers_unblock_by_func(G_OBJECT(dialog->interpolation_button[i_interpolation]),
				      G_CALLBACK(dialog_change_interpolation_cb), dialog);

  return;
}

/* set which combo box item is present */
static void dialog_update_rendering(AmitkObjectDialog * dialog) {
  
  g_signal_handlers_block_by_func(G_OBJECT(dialog->rendering_menu), G_CALLBACK(dialog_change_rendering_cb), dialog);
  gtk_combo_box_set_active(GTK_COMBO_BOX(dialog->rendering_menu), AMITK_DATA_SET_RENDERING(dialog->object));
  g_signal_handlers_unblock_by_func(G_OBJECT(dialog->rendering_menu), G_CALLBACK(dialog_change_rendering_cb), dialog);

  return;
}


static void dialog_update_conversion(AmitkObjectDialog * dialog) {


  AmitkConversion i_conversion;
  AmitkConversion conversion;

  conversion = AMITK_DATA_SET_CONVERSION(dialog->object);
  for (i_conversion=0; i_conversion < AMITK_CONVERSION_NUM; i_conversion++)
    g_signal_handlers_block_by_func(G_OBJECT(dialog->conversion_button[i_conversion]),
				    G_CALLBACK(dialog_conversion_cb), dialog);
  /* need the button pressed to get the display to update correctly */
  gtk_button_pressed(GTK_BUTTON(dialog->conversion_button[conversion]));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->conversion_button[conversion]),TRUE);

  for (i_conversion=0; i_conversion < AMITK_CONVERSION_NUM; i_conversion++)
    g_signal_handlers_unblock_by_func(G_OBJECT(dialog->conversion_button[i_conversion]),
				      G_CALLBACK(dialog_conversion_cb), dialog);

  switch (conversion) {
  case AMITK_CONVERSION_PERCENT_ID_PER_CC:
    gtk_widget_set_sensitive(dialog->scaling_factor_spin, FALSE);
    gtk_widget_set_sensitive(dialog->dose_spin, TRUE);
    gtk_widget_set_sensitive(dialog->weight_spin, FALSE);
    gtk_widget_set_sensitive(dialog->cylinder_spin, TRUE);
    break;
  case AMITK_CONVERSION_SUV:
    gtk_widget_set_sensitive(dialog->scaling_factor_spin, FALSE);
    gtk_widget_set_sensitive(dialog->dose_spin, TRUE);
    gtk_widget_set_sensitive(dialog->weight_spin, TRUE);
    gtk_widget_set_sensitive(dialog->cylinder_spin, TRUE);
    break;
  case AMITK_CONVERSION_STRAIGHT:
  default:
    gtk_widget_set_sensitive(dialog->scaling_factor_spin, TRUE);
    gtk_widget_set_sensitive(dialog->dose_spin, FALSE);
    gtk_widget_set_sensitive(dialog->weight_spin, FALSE);
    gtk_widget_set_sensitive(dialog->cylinder_spin, FALSE);
    break;
  }

}

static void dialog_update_roi_sample_item(AmitkObjectDialog * dialog) {

  ui_common_update_sample_roi_item(dialog->roi_item,
				   AMITK_STUDY_CANVAS_ROI_WIDTH(dialog->object),
#ifdef AMIDE_LIBGNOMECANVAS_AA
				   AMITK_STUDY_CANVAS_ROI_TRANSPARENCY(dialog->object)
#else
				   AMITK_STUDY_CANVAS_LINE_STYLE(dialog->object)
#endif
				   );
  
  return;
}

static void dialog_change_interpolation_cb(GtkWidget * widget, gpointer data) {

  AmitkObjectDialog * dialog = data;
  AmitkInterpolation interpolation;

  interpolation = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"interpolation"));
  amitk_data_set_set_interpolation(AMITK_DATA_SET(dialog->object), interpolation);
  
  return;
}

static void dialog_change_rendering_cb(GtkWidget * widget, gpointer data) {

  AmitkObjectDialog * dialog = data;

  g_return_if_fail(AMITK_IS_DATA_SET(dialog->object));
  amitk_data_set_set_rendering(AMITK_DATA_SET(dialog->object), 
			       gtk_combo_box_get_active(GTK_COMBO_BOX(widget)));
  return;
}

static void dialog_conversion_cb(GtkWidget * widget, gpointer data) {

  AmitkObjectDialog * dialog = data;
  AmitkConversion conversion;

  conversion = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"conversion"));
  amitk_data_set_set_conversion(AMITK_DATA_SET(dialog->object), conversion);
  
  return;
}

static void dialog_set_view_center_to_origin_cb(GtkWidget * widget, gpointer data) {

  AmitkStudy * study = data;
  AmitkPoint shift;
  GList * objects;

  g_return_if_fail(AMITK_IS_STUDY(data));
  
  /* move all children by the given amount */
  objects = AMITK_OBJECT_CHILDREN(study);
  shift = point_neg(AMITK_STUDY_VIEW_CENTER(study));

  while (objects != NULL) {
    amitk_space_shift_offset(AMITK_SPACE(objects->data), shift);
    objects = objects->next;
  }		    

  /* and reset the view center */
  amitk_study_set_view_center(study, zero_point);

  return;
}

/* function called when the aspect ratio button gets clicked */
static void dialog_aspect_ratio_cb(GtkWidget * widget, gpointer data) {

  AmitkObjectDialog * dialog=data;

  dialog->aspect_ratio = 
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  return;
}


/* function called when the name of the roi has been changed */
static void dialog_change_name_cb(GtkWidget * widget, gpointer data) {

  gchar * new_name;
  AmitkObjectDialog * dialog = data;

  /* get the contents of the name entry box and save it */
  new_name = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  amitk_object_set_name(dialog->object, new_name);
  g_free(new_name);

  return;
}


/* function called when the creation date of the study has been changed */
static void dialog_change_creation_date_cb(GtkWidget * widget, gpointer data) {

  gchar * new_date;
  AmitkObjectDialog * dialog = data;

  /* get the contents of the name entry box and save it */
  new_date = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  amitk_study_set_creation_date(AMITK_STUDY(dialog->object), new_date);
  g_free(new_date);

  return;
}

/* function called when the scan date of the data set has been changed */
static void dialog_change_scan_date_cb(GtkWidget * widget, gpointer data) {

  gchar * new_date;
  AmitkObjectDialog * dialog = data;

  g_return_if_fail(AMITK_IS_DATA_SET(dialog->object));

  /* get the contents of the name entry box and save it */
  new_date = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  amitk_data_set_set_scan_date(AMITK_DATA_SET(dialog->object), new_date);
  g_free(new_date);

  return;
}

/* function called when the subject name of the data set has been changed */
static void dialog_change_subject_name_cb(GtkWidget * widget, gpointer data) {

  gchar * new_name;
  AmitkObjectDialog * dialog = data;

  g_return_if_fail(AMITK_IS_DATA_SET(dialog->object));

  /* get the contents of the name entry box and save it */
  new_name = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  amitk_data_set_set_subject_name(AMITK_DATA_SET(dialog->object), new_name);
  g_free(new_name);

  return;
}

/* function called when the subject id of the data set has been changed */
static void dialog_change_subject_id_cb(GtkWidget * widget, gpointer data) {

  gchar * new_id;
  AmitkObjectDialog * dialog = data;

  g_return_if_fail(AMITK_IS_DATA_SET(dialog->object));

  /* get the contents of the id entry box and save it */
  new_id = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  amitk_data_set_set_subject_id(AMITK_DATA_SET(dialog->object), new_id);
  g_free(new_id);

  return;
}

/* function called when the subject date of birth of the volume has been changed */
static void dialog_change_subject_dob_cb(GtkWidget * widget, gpointer data) {

  gchar * new_dob;
  AmitkObjectDialog * dialog = data;

  g_return_if_fail(AMITK_IS_DATA_SET(dialog->object));

  /* get the contents of the name entry box and save it */
  new_dob = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
  amitk_data_set_set_subject_dob(AMITK_DATA_SET(dialog->object), new_dob);
  g_free(new_dob);

  return;
}



static void dialog_change_center_cb(GtkWidget * widget, gpointer data) {

  gdouble temp_val;
  AmitkAxis axis;
  AmitkPoint old_center;
  AmitkPoint new_center;
  AmitkObjectDialog * dialog=data;

  /* figure out which widget this is */
  axis = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "axis")); 

  if (AMITK_IS_VOLUME(dialog->object)) 
    old_center = amitk_volume_get_center(AMITK_VOLUME(dialog->object)); /* in base coords */
  else if (AMITK_IS_STUDY(dialog->object))
    old_center = AMITK_STUDY_VIEW_CENTER(dialog->object);
  else if (AMITK_IS_FIDUCIAL_MARK(dialog->object))
    old_center = AMITK_FIDUCIAL_MARK_GET(dialog->object);
  else
    g_return_if_reached();

  new_center = old_center;
    
  temp_val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(dialog->center_spin[axis]));
  point_set_component(&new_center, axis, temp_val);

  if (AMITK_IS_STUDY(dialog->object)) 
    amitk_study_set_view_center(AMITK_STUDY(dialog->object), new_center);
  else /* recalculate the object's offset based on the new dimensions/center/and axis */
    amitk_space_shift_offset(AMITK_SPACE(dialog->object), point_sub(new_center, old_center));

    
  dialog_update_entries(dialog);

  return;
}

static void dialog_change_dim_cb(GtkWidget * widget, gpointer data) {

  gdouble temp_val;
  AmitkAxis axis;
  AmitkPoint shift;
  AmitkPoint new_corner;
  AmitkPoint old_corner;
  AmitkCorners temp_corner;
  AmitkObjectDialog * dialog=data;

  /* figure out which widget this is */
  axis = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "axis")); 

  /* initialize the center and dimension variables based on the old roi info */
  if (AMITK_IS_VOLUME(dialog->object))
    new_corner = old_corner = AMITK_VOLUME_CORNER(dialog->object); /* in object's coords */
  else
    g_return_if_reached();

  temp_val = 	gtk_spin_button_get_value(GTK_SPIN_BUTTON(dialog->dimension_spin[axis]));    
  point_set_component(&new_corner, axis, fabs(temp_val));

  /* recalculate the object's offset based on the new dimensions/center/and axis */
  if (AMITK_IS_ROI(dialog->object)) {
    temp_corner[0] = amitk_space_s2b(AMITK_SPACE(dialog->object), new_corner);
    temp_corner[1] = amitk_space_s2b(AMITK_SPACE(dialog->object), old_corner);
    shift = point_cmult(-0.5, point_sub(temp_corner[0], temp_corner[1]));
  }
  amitk_space_shift_offset(AMITK_SPACE(dialog->object), shift);
  
  /* reset the far corner */
  if (AMITK_IS_ROI(dialog->object)) {
    amitk_volume_set_corner(AMITK_VOLUME(dialog->object), new_corner);
    
    if ((AMITK_ROI_TYPE(dialog->object) == AMITK_ROI_TYPE_ISOCONTOUR_2D) ||
	(AMITK_ROI_TYPE(dialog->object) == AMITK_ROI_TYPE_FREEHAND_2D)) {
      AmitkPoint new_voxel_size = AMITK_ROI_VOXEL_SIZE(dialog->object);
      new_voxel_size.z = new_corner.z;
      amitk_roi_set_voxel_size(AMITK_ROI(dialog->object), new_voxel_size);
    }
  }

  dialog_update_entries(dialog);

  return;
}


static void dialog_change_voxel_size_cb(GtkWidget * widget, gpointer data) {

  gdouble temp_val;
  AmitkAxis start_axis, end_axis, axis;
  AmitkAxis i_axis;
  AmitkObjectDialog * dialog=data;
  AmitkPoint new_voxel_size=one_point;
  amide_real_t scale;
  AmitkPoint center;

  g_return_if_fail(AMITK_IS_DATA_SET(dialog->object));

  /* figure out which widget this is */
  axis = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "axis")); 

  new_voxel_size = AMITK_DATA_SET_VOXEL_SIZE(dialog->object);
  center = amitk_volume_get_center(AMITK_VOLUME(dialog->object));

  /* which ones we need to change */
  if (dialog->aspect_ratio) {
    start_axis = axis;
    end_axis = start_axis+1;
  } else {
    start_axis = 0;
    end_axis = AMITK_AXIS_NUM;
  }
  
  for (i_axis = start_axis; i_axis < end_axis; i_axis++) {
    temp_val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(dialog->voxel_size_spin[i_axis]));
    
    if (temp_val > EPSILON) { /* can't be having negative/very small numbers */
      switch(i_axis) {
      case AMITK_AXIS_X:
	if (dialog->aspect_ratio) {
	  scale = temp_val/new_voxel_size.x;
	  new_voxel_size.y = scale*new_voxel_size.y;
	  new_voxel_size.z = scale*new_voxel_size.z;
	}
	new_voxel_size.x = temp_val;
	break;
      case AMITK_AXIS_Y:
	if (dialog->aspect_ratio) {
	  scale = temp_val/new_voxel_size.y;
	  new_voxel_size.x = scale*new_voxel_size.x;
	  new_voxel_size.z = scale*new_voxel_size.z;
	}
	new_voxel_size.y = temp_val;
	break;
      case AMITK_AXIS_Z:
	if (dialog->aspect_ratio) {
	  scale = temp_val/new_voxel_size.z;
	  new_voxel_size.y = scale*new_voxel_size.y;
	  new_voxel_size.x = scale*new_voxel_size.x;
	}
	new_voxel_size.z = temp_val;
	break;
      default:
	g_return_if_reached(); /* error */
	break; 
      }
    }
  }
    
  amitk_data_set_set_voxel_size(AMITK_DATA_SET(dialog->object), new_voxel_size);
  amitk_volume_set_center(AMITK_VOLUME(dialog->object), center); /* preserve center location */
  
  dialog_update_entries(dialog);

  return;
}


static void dialog_change_scale_factor_cb(GtkWidget * widget, gpointer data) {

  gdouble temp_val;
  AmitkObjectDialog * dialog=data;

  temp_val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

  if (fabs(temp_val) > EPSILON) { 
    /* make sure it's a valid number and avoid zero */
    amitk_data_set_set_scale_factor(AMITK_DATA_SET(dialog->object), temp_val);
  }

  dialog_update_entries(dialog);
  return;
}

static void dialog_change_dose_cb(GtkWidget * widget, gpointer data) {

  amide_data_t injected_dose;
  AmitkObjectDialog * dialog=data;

  injected_dose = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

  if (injected_dose >= 0.0) {
    injected_dose = amitk_dose_unit_convert_from(injected_dose,
						 AMITK_DATA_SET_DISPLAYED_DOSE_UNIT(dialog->object));
    amitk_data_set_set_injected_dose(AMITK_DATA_SET(dialog->object), injected_dose);
  }

  dialog_update_entries(dialog);
  return;

}
static void dialog_change_weight_cb(GtkWidget * widget, gpointer data) {

  amide_data_t subject_weight;
  AmitkObjectDialog * dialog=data;

  subject_weight = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

  /* we allow zero, so that the user can put in a proxy for "I don't know" */
  if (subject_weight >= 0.0) {
    subject_weight = amitk_weight_unit_convert_from(subject_weight,
						    AMITK_DATA_SET_DISPLAYED_WEIGHT_UNIT(dialog->object));
    amitk_data_set_set_subject_weight(AMITK_DATA_SET(dialog->object), subject_weight);
  }

  dialog_update_entries(dialog);
  return;

}

static void dialog_change_cylinder_cb(GtkWidget * widget, gpointer data) {

  gdouble cylinder_factor;
  AmitkObjectDialog * dialog=data;

  cylinder_factor = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

  /* make sure it's a valid number and avoid zero */
  if (fabs(cylinder_factor) > EPSILON) { 
    cylinder_factor = amitk_cylinder_unit_convert_from(cylinder_factor,
						       AMITK_DATA_SET_DISPLAYED_CYLINDER_UNIT(dialog->object));

    amitk_data_set_set_cylinder_factor(AMITK_DATA_SET(dialog->object), cylinder_factor);
  }

  dialog_update_entries(dialog);
  return;

}

static void dialog_change_scan_start_cb(GtkWidget * widget, gpointer data) {

  AmitkObjectDialog * dialog=data;
  
  amitk_data_set_set_scan_start(AMITK_DATA_SET(dialog->object), 
				gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)));    
  dialog_update_entries(dialog); 

  return;
}


static void dialog_change_frame_duration_cb(GtkWidget * widget, gpointer data) {

  AmitkObjectDialog * dialog=data;
  guint i;

  i = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "frame"));

  amitk_data_set_set_frame_duration(AMITK_DATA_SET(dialog->object),i, 
				    gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)));
  dialog_update_entries(dialog);

  return;
}

static void dialog_change_gate_time_cb(GtkWidget * widget, gpointer data) {

  AmitkObjectDialog * dialog=data;
  guint i;

  i = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "gate"));

  amitk_data_set_set_gate_time(AMITK_DATA_SET(dialog->object),i, 
			       gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)));
  dialog_update_entries(dialog);

  return;
}



/* function to change an roi's type */
static void dialog_change_roi_type_cb(GtkWidget * widget, gpointer data) {

  AmitkObjectDialog * dialog = data;

  amitk_roi_set_type(AMITK_ROI(dialog->object), 
		     gtk_combo_box_get_active(GTK_COMBO_BOX(widget)));
  return;
}

/* function called when the modality type of a data set gets changed */
static void dialog_change_modality_cb(GtkWidget * widget, gpointer data) {

  AmitkObjectDialog * dialog = data;

  g_return_if_fail(AMITK_IS_DATA_SET(dialog->object));
  amitk_data_set_set_modality(AMITK_DATA_SET(dialog->object), 
			      gtk_combo_box_get_active(GTK_COMBO_BOX(widget)));

  return;
}

/* function called when the subject orientation of a data set gets changed */
static void dialog_change_subject_orientation_cb(GtkWidget * widget, gpointer data) {

  AmitkObjectDialog * dialog = data;

  g_return_if_fail(AMITK_IS_DATA_SET(dialog->object));

  /* figure out which menu item called me */
  amitk_data_set_set_subject_orientation(AMITK_DATA_SET(dialog->object), 
					 gtk_combo_box_get_active(GTK_COMBO_BOX(widget)));

  return;
}

/* function called when the subject sex of a data set gets changed */
static void dialog_change_subject_sex_cb(GtkWidget * widget, gpointer data) {

  AmitkObjectDialog * dialog = data;

  g_return_if_fail(AMITK_IS_DATA_SET(dialog->object));

  /* figure out which menu item called me */
  amitk_data_set_set_subject_sex(AMITK_DATA_SET(dialog->object), 
				 gtk_combo_box_get_active(GTK_COMBO_BOX(widget)));

  return;
}

/* function called when the displayed dose unit type of a data set gets changed */
static void dialog_change_dose_unit_cb(GtkWidget * widget, gpointer data) {

  AmitkObjectDialog * dialog = data;
  AmitkDoseUnit dose_unit;
  amide_data_t injected_dose;

  g_return_if_fail(AMITK_IS_DATA_SET(dialog->object));
  dose_unit = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

  if (dose_unit != AMITK_DATA_SET_DISPLAYED_DOSE_UNIT(dialog->object)) {
    injected_dose = amitk_dose_unit_convert_to(AMITK_DATA_SET_INJECTED_DOSE(dialog->object),
					       AMITK_DATA_SET_DISPLAYED_DOSE_UNIT(dialog->object));
    injected_dose = amitk_dose_unit_convert_from(injected_dose, dose_unit);
    amitk_data_set_set_displayed_dose_unit(AMITK_DATA_SET(dialog->object), dose_unit);
    amitk_data_set_set_injected_dose(AMITK_DATA_SET(dialog->object), injected_dose);
  }

  return;
}

/* function called when the displayed weight unit type of a data set gets changed */
static void dialog_change_weight_unit_cb(GtkWidget * widget, gpointer data) {

  AmitkObjectDialog * dialog = data;
  AmitkWeightUnit weight_unit;
  amide_data_t subject_weight;

  g_return_if_fail(AMITK_IS_DATA_SET(dialog->object));
  weight_unit = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

  if (weight_unit != AMITK_DATA_SET_DISPLAYED_WEIGHT_UNIT(dialog->object)) {
    subject_weight = amitk_weight_unit_convert_to(AMITK_DATA_SET_SUBJECT_WEIGHT(dialog->object),
						  AMITK_DATA_SET_DISPLAYED_WEIGHT_UNIT(dialog->object));
    subject_weight = amitk_weight_unit_convert_from(subject_weight, weight_unit);
    amitk_data_set_set_displayed_weight_unit(AMITK_DATA_SET(dialog->object), weight_unit);
    amitk_data_set_set_subject_weight(AMITK_DATA_SET(dialog->object), subject_weight);
  }

  return;
}

/* function called when the displayed cylinder unit type of a data set gets changed */
static void dialog_change_cylinder_unit_cb(GtkWidget * widget, gpointer data) {

  AmitkObjectDialog * dialog = data;
  AmitkCylinderUnit cylinder_unit;
  amide_data_t cylinder_factor;

  g_return_if_fail(AMITK_IS_DATA_SET(dialog->object));
  cylinder_unit = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

  if (cylinder_unit != AMITK_DATA_SET_DISPLAYED_CYLINDER_UNIT(dialog->object)) {
    cylinder_factor = amitk_cylinder_unit_convert_to(AMITK_DATA_SET_CYLINDER_FACTOR(dialog->object),
						     AMITK_DATA_SET_DISPLAYED_CYLINDER_UNIT(dialog->object));
    cylinder_factor = amitk_cylinder_unit_convert_from(cylinder_factor, cylinder_unit);
    amitk_data_set_set_displayed_cylinder_unit(AMITK_DATA_SET(dialog->object), cylinder_unit);
    amitk_data_set_set_cylinder_factor(AMITK_DATA_SET(dialog->object), cylinder_factor);
  }

  return;
}


static void dialog_change_roi_width_cb(GtkWidget * widget, gpointer data){
  AmitkObjectDialog * dialog = data;
  g_return_if_fail(AMITK_IS_STUDY(dialog->object));
  amitk_study_set_canvas_roi_width(AMITK_STUDY(dialog->object),
				   gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget)));

  return;
}

#ifdef AMIDE_LIBGNOMECANVAS_AA
static void dialog_change_roi_transparency_cb(GtkWidget * widget, gpointer data){
  AmitkObjectDialog * dialog = data;
  g_return_if_fail(AMITK_IS_STUDY(dialog->object));
  amitk_study_set_canvas_roi_transparency(AMITK_STUDY(dialog->object),
					  gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)));

  return;
}

#else
static void dialog_change_line_style_cb(GtkWidget * widget, gpointer data) {
  AmitkObjectDialog * dialog = data;
  g_return_if_fail(AMITK_IS_STUDY(dialog->object));
  amitk_study_set_canvas_line_style(AMITK_STUDY(dialog->object), 
				    gtk_combo_box_get_active(GTK_COMBO_BOX(widget)));
  return;
}

static void dialog_change_fill_roi_cb(GtkWidget * widget, gpointer data) {
  AmitkObjectDialog * dialog = data;
  g_return_if_fail(AMITK_IS_STUDY(dialog->object));
  amitk_study_set_canvas_fill_roi(AMITK_STUDY(dialog->object),
				  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
  return;
}
#endif

static void dialog_change_layout_cb(GtkWidget * widget, gpointer data) {
  AmitkObjectDialog * dialog = data;
  AmitkLayout new_layout;
  g_return_if_fail(AMITK_IS_STUDY(dialog->object));
  new_layout = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "layout"));
  amitk_study_set_canvas_layout(AMITK_STUDY(dialog->object),new_layout);
  return;
}

static void dialog_change_panel_layout_cb(GtkWidget * widget, gpointer data) {
  AmitkObjectDialog * dialog = data;
  AmitkPanelLayout new_panel_layout;
  g_return_if_fail(AMITK_IS_STUDY(dialog->object));
  new_panel_layout = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "panel_layout"));
  amitk_study_set_panel_layout(AMITK_STUDY(dialog->object),new_panel_layout);
  return;
}

static void dialog_change_maintain_size_cb(GtkWidget * widget, gpointer data) {
  AmitkObjectDialog * dialog = data;
  g_return_if_fail(AMITK_IS_STUDY(dialog->object));
  amitk_study_set_canvas_maintain_size(AMITK_STUDY(dialog->object),
				       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
  return;
}

static void dialog_change_target_empty_area_cb(GtkWidget * widget, gpointer data) {
  AmitkObjectDialog * dialog = data;
  g_return_if_fail(AMITK_IS_STUDY(dialog->object));
  amitk_study_set_canvas_target_empty_area(AMITK_STUDY(dialog->object),
					   gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)));
  return;
}


static void dialog_specify_color_cb(GtkWidget * widget, gpointer data) {
  AmitkObjectDialog * dialog = data;

  if (AMITK_IS_ROI(dialog->object))
    amitk_roi_set_specify_color(AMITK_ROI(dialog->object),
				gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
  else if (AMITK_IS_FIDUCIAL_MARK(dialog->object))
    amitk_fiducial_mark_set_specify_color(AMITK_FIDUCIAL_MARK(dialog->object),
					  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
  else
    g_return_if_reached();

  return;
}

static void dialog_change_color_cb(GtkWidget * widget, gpointer data) {
  AmitkObjectDialog * dialog = data;
  rgba_t new_color;
  GdkColor color_gdk;
  guint16 alpha;


  gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(widget), &color_gdk);
  alpha = gtk_color_selection_get_current_alpha(GTK_COLOR_SELECTION(widget));

  new_color.r = color_gdk.red >> 8;
  new_color.g = color_gdk.green >> 8;
  new_color.b = color_gdk.blue >> 8;
  new_color.a = alpha >> 8;

  if (AMITK_IS_ROI(dialog->object))
    amitk_roi_set_color(AMITK_ROI(dialog->object), new_color);
  else if (AMITK_IS_FIDUCIAL_MARK(dialog->object))
    amitk_fiducial_mark_set_color(AMITK_FIDUCIAL_MARK(dialog->object),new_color);
  else
    g_return_if_reached();
}


/* function called when we hit the apply button */
static void dialog_response_cb(GtkDialog* dialog, gint response_id, gpointer data) {

  gboolean return_val;

  g_return_if_fail(AMITK_IS_OBJECT_DIALOG(dialog));

  switch(response_id) {
  case AMITK_RESPONSE_REVERT:
    /* copy the old info on over */
    amitk_object_copy_in_place(AMITK_OBJECT_DIALOG(dialog)->object,
			       AMITK_OBJECT_DIALOG(dialog)->original_object);
    dialog_update_entries(AMITK_OBJECT_DIALOG(dialog));
  break;

  case GTK_RESPONSE_HELP:
    if (AMITK_IS_DATA_SET(AMITK_OBJECT_DIALOG(dialog)->object))
      amide_call_help("data-set-dialog");
    else if (AMITK_IS_ROI(AMITK_OBJECT_DIALOG(dialog)->object))
      amide_call_help("roi-dialog");
    else if (AMITK_IS_FIDUCIAL_MARK(AMITK_OBJECT_DIALOG(dialog)->object))
      amide_call_help("fiducial-marker-dialog");
    else if (AMITK_IS_STUDY(AMITK_OBJECT_DIALOG(dialog)->object))
      amide_call_help("study-dialog");
    break;

  case GTK_RESPONSE_CLOSE:
    g_signal_emit_by_name(G_OBJECT(dialog), "delete_event", NULL, &return_val);
    if (!return_val) gtk_widget_destroy(GTK_WIDGET(dialog));
    break;
  default:
    break;
  }


  return;
}



GtkWidget* amitk_object_dialog_new (AmitkObject * object) {

  AmitkObjectDialog *dialog;
  gchar * temp_string;
  AmitkObject * study;

  g_return_val_if_fail(AMITK_IS_OBJECT(object), NULL);
  
  /* find study - use for the layout */
  if (AMITK_IS_STUDY(object))
    study = object;
  else
    study = amitk_object_get_parent_of_type(object, AMITK_OBJECT_TYPE_STUDY); /* unreferenced pointer */
  g_return_val_if_fail(AMITK_IS_STUDY(study), NULL);

  if (object->dialog != NULL) {
    if (AMITK_IS_OBJECT_DIALOG(object->dialog))
	return NULL; /* already modifying */
    else if (AMITK_IS_THRESHOLD(object->dialog))
      gtk_widget_destroy(GTK_WIDGET(object->dialog));
    else
      g_return_val_if_reached(NULL);
  }


  dialog = g_object_new (AMITK_TYPE_OBJECT_DIALOG, NULL);
  object->dialog = G_OBJECT(dialog);
  object_dialog_construct(dialog, object, AMITK_STUDY_CANVAS_LAYOUT(study));

  g_signal_connect_swapped(G_OBJECT(object), "space_changed", 
			   G_CALLBACK(dialog_update_entries), dialog);

  if (AMITK_IS_STUDY(object)) {
    g_signal_connect_swapped(G_OBJECT(object), "voxel_dim_or_zoom_changed", 
			     G_CALLBACK(dialog_update_entries), dialog);
    g_signal_connect_swapped(G_OBJECT(object), "view_center_changed", 
			     G_CALLBACK(dialog_update_entries), dialog);
    g_signal_connect_swapped(G_OBJECT(object), "canvas_roi_preference_changed",
			     G_CALLBACK(dialog_update_entries), dialog);
    g_signal_connect_swapped(G_OBJECT(object), "canvas_general_preference_changed",
			     G_CALLBACK(dialog_update_entries), dialog);
    g_signal_connect_swapped(G_OBJECT(object), "canvas_target_preference_changed",
			     G_CALLBACK(dialog_update_entries), dialog);
    g_signal_connect_swapped(G_OBJECT(object), "canvas_layout_preference_changed",
			     G_CALLBACK(dialog_update_entries), dialog);
    g_signal_connect_swapped(G_OBJECT(object), "panel_layout_preference_changed",
			     G_CALLBACK(dialog_update_entries), dialog);
  }

  if (AMITK_IS_VOLUME(object)) {
    g_signal_connect_swapped(G_OBJECT(object), "volume_changed", 
			     G_CALLBACK(dialog_update_entries), dialog);
  }

  if (AMITK_IS_ROI(object)) {
    g_signal_connect_swapped(G_OBJECT(object), "roi_changed", 
			     G_CALLBACK(dialog_update_entries), dialog);
  }
  if (AMITK_IS_FIDUCIAL_MARK(object)) {
    g_signal_connect_swapped(G_OBJECT(object), "fiducial_mark_changed",
			     G_CALLBACK(dialog_update_entries), dialog);
  }
  if (AMITK_IS_DATA_SET(object)) {
    g_signal_connect_swapped(G_OBJECT(object), "data_set_changed", 
			     G_CALLBACK(dialog_update_entries), dialog);
    g_signal_connect_swapped(G_OBJECT(object), "interpolation_changed", 
			     G_CALLBACK(dialog_update_interpolation), dialog);
    g_signal_connect_swapped(G_OBJECT(object), "rendering_changed", 
			     G_CALLBACK(dialog_update_rendering), dialog);
    g_signal_connect_swapped(G_OBJECT(object), "conversion_changed", 
			     G_CALLBACK(dialog_update_conversion), dialog);
  }

  /* fill in values */
  dialog_update_entries(dialog);

  temp_string = g_strdup_printf(_("Modification Dialog: %s"),AMITK_OBJECT_NAME(object));
  gtk_window_set_title (GTK_WINDOW (dialog), temp_string);
  g_free(temp_string);

  return GTK_WIDGET (dialog);
}

