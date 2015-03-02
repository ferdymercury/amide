/* amitk_threshold.c
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

/* adapted from gtkcolorsel.c */


#include "amide_config.h"
#include "amitk_common.h"
#include "amitk_progress_dialog.h"
#include "amitk_threshold.h"
#include "pixmaps.h"
#include "image.h"
#include "amitk_marshal.h"
#include "amitk_color_table_menu.h"

#define THRESHOLD_COLOR_SCALE_SEPARATION 30.0
#define THRESHOLD_COLOR_SCALE_WIDTH 16.0
#define THRESHOLD_COLOR_SCALES_WIDTH (2.0*THRESHOLD_COLOR_SCALE_WIDTH+THRESHOLD_COLOR_SCALE_SEPARATION)
#define THRESHOLD_COLOR_SCALE_HEIGHT (gdouble) AMITK_DATA_SET_DISTRIBUTION_SIZE
#define THRESHOLD_HISTOGRAM_WIDTH (gdouble) IMAGE_DISTRIBUTION_WIDTH
#define THRESHOLD_TRIANGLE_WIDTH 16.0
#define THRESHOLD_TRIANGLE_HEIGHT 12.0


/* internal variables */
static gchar * thresholding_names[] = {
  N_("per slice"), 
  N_("per frame"), 
  N_("interpolated between frames"),
  N_("global")
};

/* thresholding explanations:
  per slice - threshold the images based on the max and min values in the current slice
  per frame - threshold the images based on the max and min values in the current frame
  interpolated between frames - threshold the images based on max and min values interpolated from the reference frame thresholds
  global - threshold the images based on the max and min values of the entire data set
*/

static gchar * threshold_style_names[] = {
  N_("Min/Max"),
  N_("Center/Width")
};
  
/* threshold style explanations
  Min/Max - threshold by setting min and max values - Nuclear Medicine Style
  Center/Width - theshold by setting a window center and width - Radiology Style
*/

static void threshold_class_init (AmitkThresholdClass *klass);
static void threshold_init (AmitkThreshold *threshold);
static void threshold_destroy (GtkObject *object);
static void threshold_show_all (GtkWidget * widget);
static void threshold_construct(AmitkThreshold * threshold, AmitkThresholdLayout layout);
static void threshold_add_data_set(AmitkThreshold * threshold, AmitkDataSet * ds);
static void threshold_remove_data_set(AmitkThreshold * threshold);
static gint threshold_visible_refs(AmitkDataSet * data_set);
static void threshold_update_histogram(AmitkThreshold * threshold);
static void threshold_update_spin_buttons(AmitkThreshold * threshold);
static void threshold_update_arrow(AmitkThreshold * threshold, AmitkThresholdArrow arrow);
static void threshold_update_color_scale(AmitkThreshold * threshold, AmitkThresholdScale scale);
static void threshold_update_connector_lines(AmitkThreshold * threshold, AmitkThresholdScale scale);
static void threshold_update_color_scales(AmitkThreshold * threshold);
static void threshold_update_layout(AmitkThreshold * threshold);
static void threshold_update_style(AmitkThreshold * threshold);
static void threshold_update_type(AmitkThreshold * threshold);
static void threshold_update_absolute_label(AmitkThreshold * threshold);
static void threshold_update_windowing(AmitkThreshold * threshold);
static void threshold_update_ref_frames(AmitkThreshold * threshold);
static void threshold_update_color_table(AmitkThreshold * threshold, AmitkViewMode view_mode);
static void threshold_update_color_tables(AmitkThreshold * threshold);
static void ds_scale_factor_changed_cb(AmitkDataSet * ds, AmitkThreshold* threshold);
static void ds_color_table_changed_cb(AmitkDataSet * ds, AmitkViewMode view_mode, AmitkThreshold* threshold);
static void ds_thresholding_changed_cb(AmitkDataSet * ds, AmitkThreshold* threshold);
static void ds_threshold_style_changed_cb(AmitkDataSet * ds, AmitkThreshold* threshold);
static void ds_thresholds_changed_cb(AmitkDataSet * ds, AmitkThreshold* threshold);
static void ds_conversion_changed_cb(AmitkDataSet * ds, AmitkThreshold* threshold);
static void ds_modality_changed_cb(AmitkDataSet * ds, AmitkThreshold* threshold);
static void study_view_mode_changed_cb(AmitkStudy * study, AmitkThreshold * threshold);
static gint threshold_arrow_cb(GtkWidget* widget, GdkEvent * event, gpointer AmitkThreshold);
static void color_table_cb(GtkWidget * widget, gpointer data);
static void color_table_independent_cb(GtkWidget * widget, gpointer AmitkThreshold);
static void threshold_ref_frame_cb(GtkWidget* widget, gpointer data);
static void threshold_spin_button_cb(GtkWidget* widget, gpointer data);
static void thresholding_cb(GtkWidget * widget, gpointer data);
static void threshold_style_cb(GtkWidget * widget, gpointer data);
static void windowing_cb(GtkWidget * widget, gpointer data);

static void init_common(GtkWindow * threshold_dialog);

static void threshold_dialog_class_init (AmitkThresholdDialogClass *klass);
static void threshold_dialog_init (AmitkThresholdDialog *threshold_dialog);
static void threshold_dialog_construct(AmitkThresholdDialog * dialog, GtkWindow * parent, AmitkDataSet * data_set);

static void thresholds_dialog_class_init (AmitkThresholdsDialogClass *klass);
static void thresholds_dialog_init (AmitkThresholdsDialog *thresholds_dialog);
static void thresholds_dialog_construct(AmitkThresholdsDialog * thresholds_dialog, GtkWindow * parent, GList * data_sets);

static GtkVBoxClass *threshold_parent_class;
static GtkDialogClass *threshold_dialog_parent_class;
static GtkDialogClass *thresholds_dialog_parent_class;

static GdkCursor * threshold_cursor = NULL;

GType amitk_threshold_get_type (void) {

  static GType threshold_type = 0;

  if (!threshold_type)
    {
      static const GTypeInfo threshold_info =
      {
	sizeof (AmitkThresholdClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) threshold_class_init,
	(GClassFinalizeFunc) NULL,
	NULL, /* class data */
	sizeof (AmitkThreshold),
	0, /* # preallocs */
	(GInstanceInitFunc) threshold_init,
	NULL /* value table */
      };

      threshold_type = g_type_register_static(GTK_TYPE_VBOX, "AmitkThreshold", &threshold_info, 0);
    }

  return threshold_type;
}

static void threshold_class_init (AmitkThresholdClass *klass)
{
  GtkObjectClass *gtkobject_class;
  GtkWidgetClass *widget_class;

  gtkobject_class = (GtkObjectClass*) klass;
  widget_class    = (GtkWidgetClass*) klass;

  threshold_parent_class = g_type_class_peek_parent(klass);

  gtkobject_class->destroy = threshold_destroy;
  widget_class->show_all = threshold_show_all;

}

static void threshold_init (AmitkThreshold *threshold)
{

  AmitkThresholdScale i_scale;
  AmitkThresholdLine i_line;
  guint i_ref;



  /* initialize some critical stuff */
  for (i_ref=0; i_ref<2; i_ref++) {
    for (i_scale=0;i_scale<AMITK_THRESHOLD_SCALE_NUM_SCALES;i_scale++) {
      threshold->color_scale_image[i_ref][i_scale] = NULL;
    }
    for (i_line=0;i_line<AMITK_THRESHOLD_LINE_NUM_LINES; i_line++)
      threshold->connector_line[i_ref][i_line] = NULL;
  }
  threshold->histogram_image = NULL;

  if (threshold_cursor == NULL)
    threshold_cursor = gdk_cursor_new(GDK_SB_V_DOUBLE_ARROW);

}
static void threshold_destroy (GtkObject * object) {

  AmitkThreshold * threshold;

  g_return_if_fail (object != NULL);
  g_return_if_fail (AMITK_IS_THRESHOLD (object));

  threshold = AMITK_THRESHOLD (object);

  threshold_remove_data_set(threshold);

  if (GTK_OBJECT_CLASS (threshold_parent_class)->destroy)
    (* GTK_OBJECT_CLASS (threshold_parent_class)->destroy) (object);
}

/* this is used to catch the show_all signal, which would show the
   widgets that we're currently might be hiding */
static void threshold_show_all (GtkWidget * widget) {
  
  /*  AmitkThreshold * threshold; */
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (AMITK_IS_THRESHOLD(widget));

  /* threshold = AMITK_THRESHOLD(widget); */
  
  gtk_widget_show(widget);

}

void amitk_threshold_style_widgets(GtkWidget ** radio_buttons, GtkWidget * hbox) {

  AmitkThresholdStyle i_threshold_style;
  GtkWidget * image;

  for (i_threshold_style = 0; i_threshold_style < AMITK_THRESHOLD_STYLE_NUM; i_threshold_style++) {
      
    radio_buttons[i_threshold_style] = gtk_radio_button_new(NULL);
    switch(i_threshold_style) {
    case AMITK_THRESHOLD_STYLE_MIN_MAX:
      image = gtk_image_new_from_stock("amide_icon_threshold_style_min_max",GTK_ICON_SIZE_LARGE_TOOLBAR);
      break;
    case AMITK_THRESHOLD_STYLE_CENTER_WIDTH:
      image = gtk_image_new_from_stock("amide_icon_threshold_style_center_width",GTK_ICON_SIZE_LARGE_TOOLBAR);
      break;
    default:
      image = NULL;
      g_error("unexpected case in %s at line %d",__FILE__, __LINE__);
      break;
    }
    gtk_button_set_image(GTK_BUTTON(radio_buttons[i_threshold_style]), image);
      
    gtk_box_pack_start(GTK_BOX(hbox), radio_buttons[i_threshold_style], FALSE, FALSE, 3);
    gtk_widget_show(radio_buttons[i_threshold_style]);
    gtk_widget_set_tooltip_text(radio_buttons[i_threshold_style],
				threshold_style_names[i_threshold_style]);
      
    g_object_set_data(G_OBJECT(radio_buttons[i_threshold_style]), "threshold_style", 
		      GINT_TO_POINTER(i_threshold_style));
      
    if (i_threshold_style != 0)
      gtk_radio_button_set_group(GTK_RADIO_BUTTON(radio_buttons[i_threshold_style]), 
				 gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])));
      
  }
}


/* this gets called after we have a data set */
static void threshold_construct(AmitkThreshold * threshold, 
			        AmitkThresholdLayout layout) {

  GtkWidget * right_table;
  GtkWidget * left_table;
  GtkWidget * main_box;
  GtkWidget * hbox;
  guint right_row;
  guint left_row;
  gchar * temp_string;
  GtkWidget * label;
  GtkWidget * button;
  GtkWidget * image;
  AmitkThresholding i_thresholding;
  AmitkWindow i_window;
  guint i_ref;
  AmitkThresholdEntry i_entry;
  AmitkThresholdStyle i_threshold_style;
  AmitkLimit i_limit;
  AmitkViewMode i_view_mode;
  div_t x;


  /* we're using two tables packed into a box */
  if (layout == AMITK_THRESHOLD_BOX_LAYOUT)
    main_box = gtk_hbox_new(FALSE,0);
  else /* layout == AMITK_THRESHOLD_LINEAR_LAYOUT */
    main_box = gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(threshold), main_box);


  /*---------------------------------------
    left table 
    --------------------------------------- */
  
  left_table = gtk_table_new(8,5,FALSE);
  left_row=0;
  gtk_box_pack_start(GTK_BOX(main_box), left_table, TRUE,TRUE,0);
  
  if (!threshold->minimal) {
    for (i_ref=0; i_ref<2; i_ref++) {
      threshold->percent_label[i_ref] = gtk_label_new(_("Percent"));
      gtk_table_attach(GTK_TABLE(left_table), threshold->percent_label[i_ref], 
		       1+2*i_ref,2+2*i_ref,left_row,left_row+1,
		       X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
      /* show/hide taken care of by threshold_update_layout */
      
      threshold->absolute_label[i_ref] = gtk_label_new("");
      gtk_table_attach(GTK_TABLE(left_table), threshold->absolute_label[i_ref], 
		       2+2*i_ref,3+2*i_ref,left_row,left_row+1,
		       X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
      /* show/hide taken care of by threshold_update_layout */
    }
    left_row++;
    
    
    for (i_limit=0; i_limit < AMITK_LIMIT_NUM; i_limit++) {
      threshold->min_max_label[i_limit] = gtk_label_new(NULL);
      gtk_table_attach(GTK_TABLE(left_table),  threshold->min_max_label[i_limit], 0,1,left_row+1-i_limit,left_row+2-i_limit,
		       X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
      gtk_widget_show(threshold->min_max_label[i_limit]);
    }
    

    for (i_ref=0; i_ref<2; i_ref++) {
      threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MAX_ABSOLUTE] = 
	gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 1.0);
      threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MIN_ABSOLUTE] = 
	gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 1.0);
      threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MAX_PERCENT] = 
	gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 1.0);
      threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MIN_PERCENT] = 
	gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 1.0);

      g_signal_connect(G_OBJECT(threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MAX_ABSOLUTE]), 
		       "output", G_CALLBACK(amitk_spin_button_scientific_output), NULL);
      g_signal_connect(G_OBJECT(threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MIN_ABSOLUTE]), 
		       "output", G_CALLBACK(amitk_spin_button_scientific_output), NULL);
      gtk_spin_button_set_digits(GTK_SPIN_BUTTON(threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MAX_PERCENT]), 1);
      gtk_spin_button_set_digits(GTK_SPIN_BUTTON(threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MIN_PERCENT]), 1);

      for (i_entry=0; i_entry< AMITK_THRESHOLD_ENTRY_NUM_ENTRIES; i_entry++) {
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(threshold->spin_button[i_ref][i_entry]), FALSE);
	g_object_set_data(G_OBJECT(threshold->spin_button[i_ref][i_entry]), "type", GINT_TO_POINTER(i_entry));
	g_object_set_data(G_OBJECT(threshold->spin_button[i_ref][i_entry]), "which_ref", GINT_TO_POINTER(i_ref));
	g_signal_connect(G_OBJECT(threshold->spin_button[i_ref][i_entry]), "value_changed",  
			 G_CALLBACK(threshold_spin_button_cb), threshold);
      }
      gtk_table_attach(GTK_TABLE(left_table),  threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MAX_PERCENT], 1+2*i_ref,2+2*i_ref,
		       left_row,left_row+1, X_PACKING_OPTIONS | GTK_FILL,  0,  X_PADDING, Y_PADDING);
      gtk_table_attach(GTK_TABLE(left_table), threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MAX_ABSOLUTE], 2+2*i_ref,3+2*i_ref,
		       left_row,left_row+1,X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
      gtk_table_attach(GTK_TABLE(left_table), threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MIN_PERCENT], 1+2*i_ref,2+2*i_ref, 
		       left_row+1,left_row+2, X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
      gtk_table_attach(GTK_TABLE(left_table), threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MIN_ABSOLUTE], 2+2*i_ref,3+2*i_ref, 
		       left_row+1, left_row+2, X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
      /* show/hide taken care of by threshold_update_layout */
    }
    left_row+=2;
  }

  /* color table selectors */
  for (i_view_mode=0; i_view_mode < AMITK_VIEW_MODE_NUM; i_view_mode++) {
    
    if (i_view_mode == AMITK_VIEW_MODE_SINGLE) 
      threshold->color_table_label[i_view_mode] = gtk_label_new(_("Color Table:"));
    else {
      temp_string = g_strdup_printf(_("Color Table %d:"), i_view_mode+1);
      threshold->color_table_label[i_view_mode] = gtk_label_new(temp_string);
      g_free(temp_string);
    }
    gtk_table_attach(GTK_TABLE(left_table), threshold->color_table_label[i_view_mode], 
		     0,1, left_row,left_row+1, X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);

    threshold->color_table_hbox[i_view_mode] = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(left_table), threshold->color_table_hbox[i_view_mode], 
		     1,5, left_row,left_row+1, X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);

    if (i_view_mode == AMITK_VIEW_MODE_SINGLE) {
      threshold->color_table_independent[i_view_mode] = NULL;
    } else {
      threshold->color_table_independent[i_view_mode] = gtk_check_button_new();
      g_object_set_data(G_OBJECT(threshold->color_table_independent[i_view_mode]), "view_mode", GINT_TO_POINTER(i_view_mode));
      g_signal_connect(G_OBJECT(threshold->color_table_independent[i_view_mode]), "toggled", G_CALLBACK(color_table_independent_cb), threshold);
      gtk_box_pack_start(GTK_BOX(threshold->color_table_hbox[i_view_mode]), 
			 threshold->color_table_independent[i_view_mode], FALSE, FALSE, 0);
      gtk_widget_set_tooltip_text(threshold->color_table_independent[i_view_mode],
				  _("if not enabled, the primary color table will be used for this set of views"));
      gtk_widget_show(threshold->color_table_independent[i_view_mode]);
    } 

    threshold->color_table_menu[i_view_mode] = amitk_color_table_menu_new();
    g_object_set_data(G_OBJECT(threshold->color_table_menu[i_view_mode]), "view_mode", GINT_TO_POINTER(i_view_mode));
    g_signal_connect(G_OBJECT(threshold->color_table_menu[i_view_mode]), "changed", G_CALLBACK(color_table_cb), threshold);
    gtk_box_pack_start(GTK_BOX(threshold->color_table_hbox[i_view_mode]), 
		       threshold->color_table_menu[i_view_mode], TRUE, TRUE, 0);
    gtk_widget_show(threshold->color_table_menu[i_view_mode]);
    left_row++;
  }

  if (!threshold->minimal) {
    /* threshold type selection */
    label = gtk_label_new(_("Threshold Type"));
    gtk_table_attach(GTK_TABLE(left_table), label, 0,1, left_row,left_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);
    
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(left_table), hbox, 1,5, left_row,left_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(hbox);
    left_row++;

    for (i_thresholding = 0; i_thresholding < AMITK_THRESHOLDING_NUM; i_thresholding++) {
      
      threshold->type_button[i_thresholding] = gtk_radio_button_new(NULL);
      switch(i_thresholding) {
      case AMITK_THRESHOLDING_PER_SLICE:
	image = gtk_image_new_from_stock("amide_icon_thresholding_per_slice",GTK_ICON_SIZE_LARGE_TOOLBAR);
	break;
      case AMITK_THRESHOLDING_PER_FRAME:
	image = gtk_image_new_from_stock("amide_icon_thresholding_per_frame",GTK_ICON_SIZE_LARGE_TOOLBAR);
	break;
      case AMITK_THRESHOLDING_INTERPOLATE_FRAMES:
	image = gtk_image_new_from_stock("amide_icon_thresholding_interpolate_frames",GTK_ICON_SIZE_LARGE_TOOLBAR);
	break;
      case AMITK_THRESHOLDING_GLOBAL:
	image = gtk_image_new_from_stock("amide_icon_thresholding_global",GTK_ICON_SIZE_LARGE_TOOLBAR);
	break;
      default:
	image = NULL;
	g_error("unexpected case in %s at line %d",__FILE__, __LINE__);
	break;
      }
      gtk_button_set_image(GTK_BUTTON(threshold->type_button[i_thresholding]), image);
      gtk_box_pack_start(GTK_BOX(hbox), threshold->type_button[i_thresholding], FALSE, FALSE, 3);
      gtk_widget_show(threshold->type_button[i_thresholding]);
      gtk_widget_set_tooltip_text(threshold->type_button[i_thresholding], 
				  thresholding_names[i_thresholding]);
      
      g_object_set_data(G_OBJECT(threshold->type_button[i_thresholding]), "thresholding", 
			GINT_TO_POINTER(i_thresholding));
      
      if (i_thresholding != 0)
	gtk_radio_button_set_group(GTK_RADIO_BUTTON(threshold->type_button[i_thresholding]), 
				   gtk_radio_button_get_group(GTK_RADIO_BUTTON(threshold->type_button[0])));
      
      g_signal_connect(G_OBJECT(threshold->type_button[i_thresholding]), "clicked",  
		       G_CALLBACK(thresholding_cb), threshold);
    }
  }


  /* windowing buttons */
  if (!threshold->minimal) {

    /* threshold type selection */
    label = gtk_label_new(_("Threshold Style"));
    gtk_table_attach(GTK_TABLE(left_table), label, 0,1, left_row,left_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);
    
    hbox = gtk_hbox_new(FALSE, 0);
    amitk_threshold_style_widgets(threshold->style_button, hbox);
    gtk_table_attach(GTK_TABLE(left_table), hbox, 1,5, left_row,left_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    for (i_threshold_style = 0; i_threshold_style < AMITK_THRESHOLD_STYLE_NUM; i_threshold_style++) 
      g_signal_connect(G_OBJECT(threshold->style_button[i_threshold_style]), "clicked",  
		       G_CALLBACK(threshold_style_cb), threshold);
    gtk_widget_show(hbox);
    left_row++;


    threshold->window_label = gtk_label_new("Apply Window");
    gtk_table_attach(GTK_TABLE(left_table), threshold->window_label,
		     0,1,left_row,left_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    /* show/hide taken care of by threshold_update_window */

    threshold->window_vbox = gtk_vbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(left_table), threshold->window_vbox,
		     1,5,left_row,left_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    /* show/hide taken care of by threshold_update_window */

    for (i_window=0; i_window < AMITK_WINDOW_NUM; i_window++) {
      x = div(i_window, 5);
      if (x.rem == 0) {
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(threshold->window_vbox), hbox, FALSE, FALSE, 3);
	gtk_widget_show(hbox);
      }
      button = gtk_button_new();
      image = gtk_image_new_from_stock(windowing_icons[i_window],GTK_ICON_SIZE_LARGE_TOOLBAR);
      gtk_button_set_image(GTK_BUTTON(button), image);
      g_object_set_data(G_OBJECT(button), "which_window",GINT_TO_POINTER(i_window));
      gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 3);
      gtk_widget_show(button);

      g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(windowing_cb), threshold);
      gtk_widget_set_tooltip_text(button, _(amitk_window_names[i_window]));
    }

    left_row++;
  }

  /*---------------------------------------
    right table 
    --------------------------------------- */
  right_table = gtk_table_new(4,5,FALSE);
  right_row=0;
  gtk_box_pack_start(GTK_BOX(main_box), right_table, TRUE,TRUE,0);

  /* color table selector */
  threshold->ref_frame_label[0] = gtk_label_new(_("ref. frame 0:"));
  gtk_table_attach(GTK_TABLE(right_table), threshold->ref_frame_label[0], 1,2, right_row,right_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  /* show/hide taken care of by threshold_update_layout */
  
  threshold->ref_frame_label[1] = gtk_label_new(_("ref. frame 1:"));
  gtk_table_attach(GTK_TABLE(right_table), threshold->ref_frame_label[1], 3,4, right_row,right_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  /* show/hide taken care of by threshold_update_layout */

  for (i_ref=0; i_ref<2; i_ref++) {
    threshold->threshold_ref_frame_menu[i_ref] = gtk_combo_box_new_text();
    gtk_table_attach(GTK_TABLE(right_table), threshold->threshold_ref_frame_menu[i_ref],
		     2+i_ref*2,3+i_ref*2, right_row,right_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_set_size_request(threshold->threshold_ref_frame_menu[i_ref], 50, -1);
    g_object_set_data(G_OBJECT(threshold->threshold_ref_frame_menu[i_ref]), 
		      "which_ref", GINT_TO_POINTER(i_ref));
    g_signal_connect(G_OBJECT(threshold->threshold_ref_frame_menu[i_ref]), "changed", 
		     G_CALLBACK(threshold_ref_frame_cb), threshold);
    /* show/hide taken care of by threshold_update_layout */
  }


  /* puts these labels on bottom for a linear layout */
  if (layout == AMITK_THRESHOLD_LINEAR_LAYOUT) right_row = 3;
  else right_row++;

  if (!threshold->minimal) {
    threshold->histogram_label = gtk_label_new(_("distribution"));
    gtk_table_attach(GTK_TABLE(right_table),  threshold->histogram_label, 
		     0,1, right_row, right_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    /* show/hide taken care of by threshold_update_layout */
  }

  for (i_ref=0; i_ref<2; i_ref++) {
    threshold->full_label[i_ref] = gtk_label_new(_("full"));
    gtk_table_attach(GTK_TABLE(right_table),  threshold->full_label[i_ref], 
		     1+2*i_ref,2+2*i_ref, right_row, right_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    /* show/hide taken care of by threshold_update_layout */

    threshold->scaled_label[i_ref] = gtk_label_new(_("scaled"));
    gtk_table_attach(GTK_TABLE(right_table),  threshold->scaled_label[i_ref], 
		     2+2*i_ref,3+2*i_ref, right_row, right_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    /* show/hide taken care of by threshold_update_layout */

  }

  if (layout == AMITK_THRESHOLD_LINEAR_LAYOUT) right_row = 1;
  else right_row++;


  /* the histogram */
  if (!threshold->minimal) {
#ifdef AMIDE_LIBGNOMECANVAS_AA
    threshold->histogram = gnome_canvas_new_aa();
#else
    threshold->histogram = gnome_canvas_new();
#endif
    gtk_table_attach(GTK_TABLE(right_table), threshold->histogram, 0,1,right_row,right_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL, X_PADDING, Y_PADDING);
    gtk_widget_set_size_request(threshold->histogram,
				IMAGE_DISTRIBUTION_WIDTH,
				AMITK_DATA_SET_DISTRIBUTION_SIZE + 2*THRESHOLD_TRIANGLE_HEIGHT);
    gnome_canvas_set_scroll_region(GNOME_CANVAS(threshold->histogram), 
				   0.0, THRESHOLD_TRIANGLE_WIDTH/2.0,
				   IMAGE_DISTRIBUTION_WIDTH,
				   (THRESHOLD_TRIANGLE_WIDTH/2.0 + AMITK_DATA_SET_DISTRIBUTION_SIZE));
    /* show/hide taken care of by threshold_update_layout */
  }



  /* the color scale */
  for (i_ref=0; i_ref<2; i_ref++) {
#ifdef AMIDE_LIBGNOMECANVAS_AA
    threshold->color_scales[i_ref] = gnome_canvas_new_aa();
#else
    threshold->color_scales[i_ref] = gnome_canvas_new();
#endif
    gtk_table_attach(GTK_TABLE(right_table),  threshold->color_scales[i_ref], 
		     1+2*i_ref,3+2*i_ref,right_row,right_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL, X_PADDING, Y_PADDING);
    gnome_canvas_set_scroll_region(GNOME_CANVAS(threshold->color_scales[i_ref]), 0.0, 0.0,
				   THRESHOLD_COLOR_SCALES_WIDTH+2* THRESHOLD_TRIANGLE_WIDTH,
				   THRESHOLD_COLOR_SCALE_HEIGHT + 2*THRESHOLD_TRIANGLE_HEIGHT + 1);
    gtk_widget_set_size_request(threshold->color_scales[i_ref],
				THRESHOLD_COLOR_SCALES_WIDTH+2* THRESHOLD_TRIANGLE_WIDTH,
				THRESHOLD_COLOR_SCALE_HEIGHT + 2*THRESHOLD_TRIANGLE_HEIGHT + 1);
    /* show/hide taken care of by threshold_update_layout */
  }

  right_row++;

  gtk_widget_show(left_table);
  gtk_widget_show(right_table);
  gtk_widget_show(main_box);

  return;
}


static void threshold_add_data_set(AmitkThreshold * threshold, AmitkDataSet * ds) {

  gint i;
  AmitkStudy * study;

  g_return_if_fail(threshold->data_set == NULL);

  threshold->data_set = amitk_object_ref(ds);

  /* explicitly make sure the min/max values have been calculated on the new data set, 
   this is to make sure a progress box gets pushed up if these haven't been calculated eyt */
  amitk_data_set_calc_min_max_if_needed(threshold->data_set,
					amitk_progress_dialog_update,
					threshold->progress_dialog);

  for (i=0; i<2; i++) {
    threshold->threshold_max[i] = AMITK_DATA_SET_THRESHOLD_MAX(ds, i);
    threshold->threshold_min[i] = AMITK_DATA_SET_THRESHOLD_MIN(ds, i);
  }

  study = AMITK_STUDY(amitk_object_get_parent_of_type(AMITK_OBJECT(ds), AMITK_OBJECT_TYPE_STUDY)); /* unreferenced pointer */
  if (study != NULL)
    threshold->view_mode = AMITK_STUDY_VIEW_MODE(study);
  else
    threshold->view_mode = AMITK_VIEW_MODE_SINGLE;


  g_signal_connect(G_OBJECT(ds), "scale_factor_changed", G_CALLBACK(ds_scale_factor_changed_cb), threshold);
  g_signal_connect(G_OBJECT(ds), "color_table_changed", G_CALLBACK(ds_color_table_changed_cb), threshold);
  g_signal_connect(G_OBJECT(ds), "color_table_independent_changed", G_CALLBACK(ds_color_table_changed_cb), threshold);
  g_signal_connect(G_OBJECT(ds), "thresholding_changed", G_CALLBACK(ds_thresholding_changed_cb), threshold);
  g_signal_connect(G_OBJECT(ds), "threshold_style_changed", G_CALLBACK(ds_threshold_style_changed_cb), threshold);
  g_signal_connect(G_OBJECT(ds), "thresholds_changed", G_CALLBACK(ds_thresholds_changed_cb), threshold);
  g_signal_connect(G_OBJECT(ds), "conversion_changed", G_CALLBACK(ds_conversion_changed_cb), threshold);
  g_signal_connect(G_OBJECT(ds), "modality_changed", G_CALLBACK(ds_modality_changed_cb), threshold);
  if (study != NULL)
    g_signal_connect(G_OBJECT(study), "view_mode_changed", G_CALLBACK(study_view_mode_changed_cb), threshold);

  return;
}

static void threshold_remove_data_set(AmitkThreshold * threshold) {

  AmitkStudy * study;

  if (threshold->data_set == NULL) return;
  study = AMITK_STUDY(amitk_object_get_parent_of_type(AMITK_OBJECT(threshold->data_set), AMITK_OBJECT_TYPE_STUDY)); /* unreferenced pointer */

  AMITK_OBJECT(threshold->data_set)->dialog = NULL;
  g_signal_handlers_disconnect_by_func(G_OBJECT(threshold->data_set), ds_scale_factor_changed_cb, threshold);
  g_signal_handlers_disconnect_by_func(G_OBJECT(threshold->data_set), ds_color_table_changed_cb, threshold);
  g_signal_handlers_disconnect_by_func(G_OBJECT(threshold->data_set), ds_thresholding_changed_cb, threshold);
  g_signal_handlers_disconnect_by_func(G_OBJECT(threshold->data_set), ds_threshold_style_changed_cb, threshold);
  g_signal_handlers_disconnect_by_func(G_OBJECT(threshold->data_set), ds_thresholds_changed_cb, threshold);
  g_signal_handlers_disconnect_by_func(G_OBJECT(threshold->data_set), ds_conversion_changed_cb, threshold);
  g_signal_handlers_disconnect_by_func(G_OBJECT(threshold->data_set), ds_modality_changed_cb, threshold);
  if (study != NULL)
    g_signal_handlers_disconnect_by_func(G_OBJECT(study), study_view_mode_changed_cb, threshold);
  threshold->data_set = amitk_object_unref(threshold->data_set);

  return;
}


/* return how many visible reference points we should have */
static gint threshold_visible_refs(AmitkDataSet * data_set) {

  if (AMITK_DATA_SET_THRESHOLDING(data_set) == 
      AMITK_THRESHOLDING_INTERPOLATE_FRAMES) 
    return 2;
  else
    return 1;
}

/* refresh what's on the histogram */
static void threshold_update_histogram(AmitkThreshold * threshold) {

  rgb_t fg;
  GtkStyle * widget_style;
  GdkPixbuf * pixbuf;

  if (threshold->minimal) return; /* no histogram in minimal configuration */

  /* figure out what colors to use for the distribution image */
  widget_style = gtk_widget_get_style(GTK_WIDGET(threshold));
  if (widget_style == NULL) {
    g_warning(_("Threshold has no style?\n"));
    widget_style = gtk_style_new();
  }

  fg.r = widget_style->fg[GTK_STATE_NORMAL].red >> 8;
  fg.g = widget_style->fg[GTK_STATE_NORMAL].green >> 8;
  fg.b = widget_style->fg[GTK_STATE_NORMAL].blue >> 8;

  pixbuf = image_of_distribution(threshold->data_set, fg, 
				 amitk_progress_dialog_update, 
				 threshold->progress_dialog);

  if (pixbuf != NULL) {
    if (threshold->histogram_image != NULL)
      gnome_canvas_item_set(threshold->histogram_image, "pixbuf", pixbuf, NULL);
    else 
      threshold->histogram_image = 
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(threshold->histogram)),
			      gnome_canvas_pixbuf_get_type(), "pixbuf", pixbuf,
			      "x", 0.0, "y", ((gdouble) THRESHOLD_TRIANGLE_HEIGHT),  NULL);
    g_object_unref(pixbuf);
  }

  return;
}

/* function to update the spin button widgets */
static void threshold_update_spin_buttons(AmitkThreshold * threshold) {

  guint i_ref;
  AmitkThresholdEntry i_entry;
  amide_data_t scale;
  amide_data_t step;
  amide_data_t min_val, max_val, min_percent, max_percent;

  if (threshold->minimal) return; /* no spin buttons in minimal configuration */
  g_return_if_fail(AMITK_IS_DATA_SET(threshold->data_set));

  scale = (amitk_data_set_get_global_max(threshold->data_set) -
	   amitk_data_set_get_global_min(threshold->data_set));
  step = scale / 100.0;
  if (scale < EPSILON) {
    scale = EPSILON;
    step = EPSILON;
  }

  for (i_ref=0; i_ref< threshold_visible_refs(threshold->data_set); i_ref++) {

    switch (AMITK_DATA_SET_THRESHOLD_STYLE(threshold->data_set)) {
    case AMITK_THRESHOLD_STYLE_CENTER_WIDTH:
      max_val = (threshold->threshold_max[i_ref]+threshold->threshold_min[i_ref])/2; /* center */
      min_val = (threshold->threshold_max[i_ref]-threshold->threshold_min[i_ref]); /* width */
      
      max_percent = 100*(max_val-amitk_data_set_get_global_min(threshold->data_set))/scale;
      min_percent = 100*min_val/scale;
      break;

    case AMITK_THRESHOLD_STYLE_MIN_MAX:
    default:
      max_val = threshold->threshold_max[i_ref];
      min_val = threshold->threshold_min[i_ref];

      max_percent = 100*(max_val-amitk_data_set_get_global_min(threshold->data_set))/scale;
      min_percent = 100*(min_val-amitk_data_set_get_global_min(threshold->data_set))/scale;
      break;
    }

    for (i_entry=0; i_entry< AMITK_THRESHOLD_ENTRY_NUM_ENTRIES; i_entry++)
      g_signal_handlers_block_by_func(G_OBJECT(threshold->spin_button[i_ref][i_entry]),
				      G_CALLBACK(threshold_spin_button_cb), threshold);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MAX_PERCENT]),max_percent);
    gtk_spin_button_set_increments(GTK_SPIN_BUTTON(threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MAX_ABSOLUTE]),
				   step, 10.0*step);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MAX_ABSOLUTE]),max_val);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MIN_PERCENT]),min_percent);
    gtk_spin_button_set_increments(GTK_SPIN_BUTTON(threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MIN_ABSOLUTE]),
				   step, 10.0*step);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MIN_ABSOLUTE]), min_val);

    for (i_entry=0; i_entry< AMITK_THRESHOLD_ENTRY_NUM_ENTRIES; i_entry++)
      g_signal_handlers_unblock_by_func(G_OBJECT(threshold->spin_button[i_ref][i_entry]),
					G_CALLBACK(threshold_spin_button_cb), threshold);
  }

  return;
}


static void threshold_update_arrow(AmitkThreshold * threshold, AmitkThresholdArrow arrow) {

  GnomeCanvasPoints * points;
  gdouble left, right, point, top, bottom;
  gboolean up_pointing=FALSE;
  gboolean down_pointing=FALSE;
  gchar * fill_color;
  guint i_ref;
  amide_data_t initial_diff;
  amide_data_t global_diff;
  guint i;
  amide_data_t center;

  global_diff = 
    amitk_data_set_get_global_max(threshold->data_set)- 
    amitk_data_set_get_global_min(threshold->data_set);
  if (global_diff < EPSILON) global_diff = 1.0; /* non sensicle */

  for (i_ref=0; i_ref< threshold_visible_refs(threshold->data_set); i_ref++) {

    points = gnome_canvas_points_new(3);
    initial_diff = 
      threshold->initial_max[i_ref]-
      threshold->initial_min[i_ref];
    if (initial_diff < EPSILON) initial_diff = EPSILON;

    switch (arrow) {
      
    case AMITK_THRESHOLD_ARROW_FULL_MIN:
      left = 0;
      right = THRESHOLD_TRIANGLE_WIDTH;
      if (EQUAL_ZERO(global_diff))
	point = THRESHOLD_TRIANGLE_HEIGHT+THRESHOLD_COLOR_SCALE_HEIGHT;
      else
	point = THRESHOLD_TRIANGLE_HEIGHT + 
	  THRESHOLD_COLOR_SCALE_HEIGHT * 
	  (1-(threshold->threshold_min[i_ref]- amitk_data_set_get_global_min(threshold->data_set))/global_diff);
      top = point;
      bottom = point+THRESHOLD_TRIANGLE_HEIGHT;
      if (threshold->threshold_min[i_ref] < amitk_data_set_get_global_min(threshold->data_set))
	down_pointing=TRUE;
      fill_color = "white";
      break;

    case AMITK_THRESHOLD_ARROW_FULL_CENTER:
      center = (threshold->threshold_max[i_ref]+threshold->threshold_min[i_ref])/2.0;
      left = 0;
      right = THRESHOLD_TRIANGLE_WIDTH;
      if (EQUAL_ZERO(global_diff))
	point = THRESHOLD_TRIANGLE_HEIGHT+THRESHOLD_COLOR_SCALE_HEIGHT;
      else
	point = THRESHOLD_TRIANGLE_HEIGHT + 
	  THRESHOLD_COLOR_SCALE_HEIGHT * 
	  (1-(center - amitk_data_set_get_global_min(threshold->data_set))/global_diff);
      top = point-THRESHOLD_TRIANGLE_HEIGHT/1.5;
      bottom = point+THRESHOLD_TRIANGLE_HEIGHT/1.5;
      if (center < amitk_data_set_get_global_min(threshold->data_set))
	down_pointing=TRUE;
      else if (center > amitk_data_set_get_global_max(threshold->data_set))
	up_pointing = TRUE;
      fill_color = "gray";
      break;
      
    case AMITK_THRESHOLD_ARROW_FULL_MAX:
      left = 0;
      right = THRESHOLD_TRIANGLE_WIDTH;
      if (EQUAL_ZERO(global_diff))
	point = THRESHOLD_TRIANGLE_HEIGHT+THRESHOLD_COLOR_SCALE_HEIGHT;
      else
	point = THRESHOLD_TRIANGLE_HEIGHT + 
	  THRESHOLD_COLOR_SCALE_HEIGHT * 
	  (1-(threshold->threshold_max[i_ref]-amitk_data_set_get_global_min(threshold->data_set))/global_diff);
      top = point-THRESHOLD_TRIANGLE_HEIGHT;
      bottom = point;
      if (threshold->threshold_max[i_ref] > amitk_data_set_get_global_max(threshold->data_set)) 
	up_pointing=TRUE; /* want upward pointing max arrow */
      fill_color = "black";
      break;
      
    case AMITK_THRESHOLD_ARROW_SCALED_MIN:
      left = THRESHOLD_COLOR_SCALES_WIDTH+2*THRESHOLD_TRIANGLE_WIDTH;
      right = THRESHOLD_COLOR_SCALES_WIDTH+THRESHOLD_TRIANGLE_WIDTH;
      if (EQUAL_ZERO(initial_diff))
	  point = THRESHOLD_TRIANGLE_HEIGHT+THRESHOLD_COLOR_SCALE_HEIGHT;
      else
	point = 
	  THRESHOLD_TRIANGLE_HEIGHT + THRESHOLD_COLOR_SCALE_HEIGHT * 
	  (1-(threshold->threshold_min[i_ref]-threshold->initial_min[i_ref])/initial_diff);
      top = point;
      bottom = point+THRESHOLD_TRIANGLE_HEIGHT;
      if (threshold->threshold_min[i_ref] < threshold->initial_min[i_ref])
	down_pointing=TRUE;
      else if (threshold->threshold_min[i_ref] > threshold->initial_max[i_ref])
	up_pointing=TRUE;
      fill_color = "white";
      break;
      
    case AMITK_THRESHOLD_ARROW_SCALED_CENTER:
      center = (threshold->threshold_max[i_ref]+threshold->threshold_min[i_ref])/2.0;
      left = THRESHOLD_COLOR_SCALES_WIDTH+2*THRESHOLD_TRIANGLE_WIDTH;
      right = THRESHOLD_COLOR_SCALES_WIDTH+THRESHOLD_TRIANGLE_WIDTH;
      if (EQUAL_ZERO(initial_diff))
	  point = THRESHOLD_TRIANGLE_HEIGHT+THRESHOLD_COLOR_SCALE_HEIGHT;
      else
	point = 
	  THRESHOLD_TRIANGLE_HEIGHT + THRESHOLD_COLOR_SCALE_HEIGHT * 
	  (1-(center-threshold->initial_min[i_ref])/initial_diff);
      top = point-THRESHOLD_TRIANGLE_HEIGHT/1.5;
      bottom = point+THRESHOLD_TRIANGLE_HEIGHT/1.5;
      if (center < threshold->initial_min[i_ref])
	down_pointing=TRUE;
      else if (center > threshold->initial_max[i_ref])
	up_pointing=TRUE;
      fill_color = "gray";
      break;
      
    case AMITK_THRESHOLD_ARROW_SCALED_MAX:
      left = THRESHOLD_COLOR_SCALES_WIDTH+2*THRESHOLD_TRIANGLE_WIDTH;
      right = THRESHOLD_COLOR_SCALES_WIDTH+THRESHOLD_TRIANGLE_WIDTH;
      if (EQUAL_ZERO(initial_diff))
	  point = THRESHOLD_TRIANGLE_HEIGHT;
      else
	point = THRESHOLD_TRIANGLE_HEIGHT + THRESHOLD_COLOR_SCALE_HEIGHT * 
	  (1-(threshold->threshold_max[i_ref]-threshold->initial_min[i_ref])/initial_diff);
      top = point-THRESHOLD_TRIANGLE_HEIGHT;
      bottom = point;
      if (threshold->threshold_max[i_ref] > threshold->initial_max[i_ref])
	up_pointing=TRUE;
      else if (threshold->threshold_max[i_ref] < threshold->initial_min[i_ref])
	down_pointing=TRUE;
      fill_color = "black";
      break;
      
    default:
      return;
    } /* end switch (arrow) */

    if (up_pointing) {
      points->coords[0] = left;
      points->coords[1] = THRESHOLD_TRIANGLE_HEIGHT;
      points->coords[2] = (left+right)/2.0;
      points->coords[3] = 0;
      points->coords[4] = right;
      points->coords[5] = THRESHOLD_TRIANGLE_HEIGHT;
    } else if (down_pointing) {
      points->coords[0] = left;
      points->coords[1] = THRESHOLD_COLOR_SCALE_HEIGHT + THRESHOLD_TRIANGLE_HEIGHT;
      points->coords[2] = (left+right)/2.0;
      points->coords[3] = THRESHOLD_COLOR_SCALE_HEIGHT + 2*THRESHOLD_TRIANGLE_HEIGHT;
      points->coords[4] = right;
      points->coords[5] = THRESHOLD_COLOR_SCALE_HEIGHT + THRESHOLD_TRIANGLE_HEIGHT;
    } else {
      points->coords[0] = left;
      points->coords[1] = bottom;
      points->coords[2] = left;
      points->coords[3] = top;
      points->coords[4] = right;
      points->coords[5] = point;
    }

    /* sanity check */
    for (i=0; i < 6; i++)
      if (!finite(points->coords[i]))
	points->coords[i] = 0.0;


    /* destroy the center arrow if needed */
    if ((AMITK_DATA_SET_THRESHOLD_STYLE(threshold->data_set) == AMITK_THRESHOLD_STYLE_MIN_MAX) && 
	((arrow == AMITK_THRESHOLD_ARROW_FULL_CENTER) || (arrow == AMITK_THRESHOLD_ARROW_SCALED_CENTER)) ) {
      if (threshold->arrow[i_ref][arrow] != NULL) {
	gtk_object_destroy(GTK_OBJECT(threshold->arrow[i_ref][arrow]));
	threshold->arrow[i_ref][arrow] = NULL;
      }
    } else { /* otherwise, do all the drawing */
      
      if (threshold->arrow[i_ref][arrow] != NULL)
	gnome_canvas_item_set(threshold->arrow[i_ref][arrow], "points", points, NULL);
      else {
	threshold->arrow[i_ref][arrow] = 
	  gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(threshold->color_scales[i_ref])),
				gnome_canvas_polygon_get_type(),
				"points", points,
				"fill_color", fill_color,
				"outline_color", "black",
				"width_pixels", 2,
				NULL);
	g_object_set_data(G_OBJECT(threshold->arrow[i_ref][arrow]), "type", GINT_TO_POINTER(arrow));
	g_object_set_data(G_OBJECT(threshold->arrow[i_ref][arrow]), "which_ref", GINT_TO_POINTER(i_ref));
	g_signal_connect(G_OBJECT(threshold->arrow[i_ref][arrow]), "event",
			 G_CALLBACK(threshold_arrow_cb), threshold);
      }
    }
    gnome_canvas_points_unref(points);
  }
    
  return;
}


/* function called to update the color scales */
static void threshold_update_color_scale(AmitkThreshold * threshold, AmitkThresholdScale scale) {

  gdouble x,y;
  guint i_ref;
  GdkPixbuf * pixbuf;

  for (i_ref=0; i_ref< threshold_visible_refs(threshold->data_set); i_ref++) {
    
    switch (scale) {
      
    case AMITK_THRESHOLD_SCALE_FULL:
      pixbuf = 
	image_from_colortable(AMITK_DATA_SET_COLOR_TABLE(threshold->data_set, AMITK_VIEW_MODE_SINGLE),
			      THRESHOLD_COLOR_SCALE_WIDTH,
			      THRESHOLD_COLOR_SCALE_HEIGHT,
			      threshold->threshold_min[i_ref],
			      threshold->threshold_max[i_ref],
			      amitk_data_set_get_global_min(threshold->data_set),
			      amitk_data_set_get_global_max(threshold->data_set),
			      FALSE);
      x = THRESHOLD_TRIANGLE_WIDTH;
      y = THRESHOLD_TRIANGLE_HEIGHT;
      break;
      
    case AMITK_THRESHOLD_SCALE_SCALED:
      pixbuf = 
	image_from_colortable(AMITK_DATA_SET_COLOR_TABLE(threshold->data_set, AMITK_VIEW_MODE_SINGLE),
			      THRESHOLD_COLOR_SCALE_WIDTH,
			      THRESHOLD_COLOR_SCALE_HEIGHT,
			      threshold->threshold_min[i_ref],
			      threshold->threshold_max[i_ref],
			      threshold->initial_min[i_ref],
			      threshold->initial_max[i_ref],
			      FALSE);
      x = THRESHOLD_COLOR_SCALE_WIDTH+THRESHOLD_TRIANGLE_WIDTH+THRESHOLD_COLOR_SCALE_SEPARATION;
      y = THRESHOLD_TRIANGLE_HEIGHT;
      break;
      
    default:
      pixbuf = NULL;
      x = y = 0.0;
      g_return_if_reached();
      break;
      
    }
   
    if (threshold->color_scale_image[i_ref][scale] != NULL) {
      gnome_canvas_item_set(threshold->color_scale_image[i_ref][scale], "pixbuf", pixbuf, NULL);
    } else {
      threshold->color_scale_image[i_ref][scale] = 
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(threshold->color_scales[i_ref])),
			      gnome_canvas_pixbuf_get_type(), "pixbuf", pixbuf,  "x", x, "y", y, NULL);
    }
    g_object_unref(pixbuf);

  }

  return;
}

static void threshold_update_connector_lines(AmitkThreshold * threshold, AmitkThresholdScale scale) {

  gdouble temp;
  GnomeCanvasPoints * line_points;
  guint i_ref;
  amide_data_t initial_diff;
  amide_data_t global_diff;

  global_diff = 
    amitk_data_set_get_global_max(threshold->data_set)- 
    amitk_data_set_get_global_min(threshold->data_set);
  if (global_diff < EPSILON) global_diff = 1.0; /* non-sensicle */

  for (i_ref=0; i_ref< threshold_visible_refs(threshold->data_set); i_ref++) {

    initial_diff = threshold->initial_max[i_ref]- threshold->initial_min[i_ref];
    if (initial_diff < EPSILON) initial_diff = EPSILON;

    /* update the line that connect the max arrows of each color scale */
    line_points = gnome_canvas_points_new(2);
    line_points->coords[0] = THRESHOLD_COLOR_SCALE_WIDTH+THRESHOLD_TRIANGLE_WIDTH;
    temp = (1.0-(threshold->threshold_max[i_ref]-
		 amitk_data_set_get_global_min(threshold->data_set))/global_diff);
    if (temp < 0.0) temp = 0.0;
    line_points->coords[1] = THRESHOLD_TRIANGLE_HEIGHT + THRESHOLD_COLOR_SCALE_HEIGHT * temp;
    line_points->coords[2] = THRESHOLD_COLOR_SCALE_WIDTH+
      THRESHOLD_TRIANGLE_WIDTH+THRESHOLD_COLOR_SCALE_SEPARATION;
    temp = (1.0-(threshold->threshold_max[i_ref]-threshold->initial_min[i_ref])/initial_diff);
    if ((temp < 0.0) || (scale==AMITK_THRESHOLD_SCALE_FULL) || isnan(temp)) temp = 0.0;
    else if (temp > 1.0) temp = 1.0;
    line_points->coords[3] = THRESHOLD_TRIANGLE_HEIGHT + THRESHOLD_COLOR_SCALE_HEIGHT * temp;
    if (threshold->connector_line[i_ref][AMITK_THRESHOLD_LINE_MAX] != NULL)
      gnome_canvas_item_set(threshold->connector_line[i_ref][AMITK_THRESHOLD_LINE_MAX],
			    "points", line_points, NULL);
    else
      threshold->connector_line[i_ref][AMITK_THRESHOLD_LINE_MAX] =
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(threshold->color_scales[i_ref])),
			      gnome_canvas_line_get_type(),
			      "points", line_points, 
			      "fill_color", "black",
			      "width_units", 1.0, NULL);
    gnome_canvas_points_unref(line_points);

    /* update the line that connect the min arrows of each color scale */
    line_points = gnome_canvas_points_new(2);
    line_points->coords[0] = THRESHOLD_COLOR_SCALE_WIDTH+THRESHOLD_TRIANGLE_WIDTH;
    temp = (1.0-(threshold->threshold_min[i_ref]-
		 amitk_data_set_get_global_min(threshold->data_set))/global_diff);
    if (temp > 1.0) temp = 1.0;
    line_points->coords[1] = THRESHOLD_TRIANGLE_HEIGHT + THRESHOLD_COLOR_SCALE_HEIGHT * temp;
    line_points->coords[2] = THRESHOLD_COLOR_SCALE_WIDTH+
      THRESHOLD_TRIANGLE_WIDTH+THRESHOLD_COLOR_SCALE_SEPARATION;
    temp = (1.0-(threshold->threshold_min[i_ref]-threshold->initial_min[i_ref])/initial_diff);
    if ((temp > 1.0) || (scale==AMITK_THRESHOLD_SCALE_FULL) || isnan(temp)) temp = 1.0;
    else if (temp < 0.0) temp = 0.0;
    line_points->coords[3] = THRESHOLD_TRIANGLE_HEIGHT + THRESHOLD_COLOR_SCALE_HEIGHT * temp;
    if (threshold->connector_line[i_ref][AMITK_THRESHOLD_LINE_MIN] != NULL)
      gnome_canvas_item_set(threshold->connector_line[i_ref][AMITK_THRESHOLD_LINE_MIN],
			    "points", line_points, NULL);
    else
      threshold->connector_line[i_ref][AMITK_THRESHOLD_LINE_MIN] =
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(threshold->color_scales[i_ref])),
			      gnome_canvas_line_get_type(),
			      "points", line_points, 
			      "fill_color", "black",
			      "width_units", 1.0, NULL);
    gnome_canvas_points_unref(line_points);

    /* update the line that connects the center arrows of each color scale */
    if (AMITK_DATA_SET_THRESHOLD_STYLE(threshold->data_set) == AMITK_THRESHOLD_STYLE_CENTER_WIDTH) {
      line_points = gnome_canvas_points_new(2);
      line_points->coords[0] = THRESHOLD_COLOR_SCALE_WIDTH+THRESHOLD_TRIANGLE_WIDTH;
      temp = (1.0-((threshold->threshold_max[i_ref]+threshold->threshold_min[i_ref])/2.0-
		   amitk_data_set_get_global_min(threshold->data_set))/global_diff);
      if (temp > 1.0) temp = 1.0;
      else if (temp < 0.0) temp = 0.0;
      line_points->coords[1] = THRESHOLD_TRIANGLE_HEIGHT + THRESHOLD_COLOR_SCALE_HEIGHT * temp;
      line_points->coords[2] = THRESHOLD_COLOR_SCALE_WIDTH+
	THRESHOLD_TRIANGLE_WIDTH+THRESHOLD_COLOR_SCALE_SEPARATION;
      temp = (1.0-((threshold->threshold_max[i_ref]+threshold->threshold_min[i_ref])/2.0-
		   threshold->initial_min[i_ref])/initial_diff);
      if ((scale == AMITK_THRESHOLD_SCALE_FULL) || isnan(temp)) temp = 0.5;
      else if (temp > 1.0) temp = 1.0;
      else if (temp < 0.0) temp = 0.0;
      line_points->coords[3] = THRESHOLD_TRIANGLE_HEIGHT + THRESHOLD_COLOR_SCALE_HEIGHT * temp;
      if (threshold->connector_line[i_ref][AMITK_THRESHOLD_LINE_CENTER] != NULL)
	gnome_canvas_item_set(threshold->connector_line[i_ref][AMITK_THRESHOLD_LINE_CENTER],
			      "points", line_points, NULL);
      else
	threshold->connector_line[i_ref][AMITK_THRESHOLD_LINE_CENTER] =
	  gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(threshold->color_scales[i_ref])),
				gnome_canvas_line_get_type(),
				"points", line_points, 
				"fill_color", "black",
				"width_units", 1.0, NULL);
      gnome_canvas_points_unref(line_points);
    } else { /* hide if not CENTER_WIDTH style */
      if (threshold->connector_line[i_ref][AMITK_THRESHOLD_LINE_CENTER] != NULL) {
	gtk_object_destroy(GTK_OBJECT(threshold->connector_line[i_ref][AMITK_THRESHOLD_LINE_CENTER]));
	threshold->connector_line[i_ref][AMITK_THRESHOLD_LINE_CENTER] = NULL;
      }
    }
  }

  return;
} 


/* update all the color scales */
static void threshold_update_color_scales(AmitkThreshold * threshold) {
  guint i_ref;

  threshold_update_color_scale(threshold, AMITK_THRESHOLD_SCALE_FULL);

  for (i_ref=0; i_ref< threshold_visible_refs(threshold->data_set); i_ref++) {
    threshold->initial_min[i_ref] = threshold->threshold_min[i_ref];
    threshold->initial_max[i_ref] = threshold->threshold_max[i_ref];
  }
  
  threshold_update_color_scale(threshold, AMITK_THRESHOLD_SCALE_SCALED);
  threshold_update_connector_lines(threshold, AMITK_THRESHOLD_SCALE_SCALED);
  return;
}


/* update which widgets are shown or hidden */
static void threshold_update_layout(AmitkThreshold * threshold) {

  guint i_ref;
  AmitkThresholdEntry i_entry;

  g_return_if_fail(AMITK_IS_DATA_SET(threshold->data_set));

  for (i_ref=0; i_ref<2; i_ref++) {


    if ((i_ref==1) && 
	(AMITK_DATA_SET_THRESHOLDING(threshold->data_set) != AMITK_THRESHOLDING_INTERPOLATE_FRAMES)) {

      if (!threshold->minimal) {
	gtk_widget_hide(threshold->percent_label[i_ref]);
	gtk_widget_hide(threshold->absolute_label[i_ref]);
	gtk_widget_hide(threshold->full_label[i_ref]);
	gtk_widget_hide(threshold->scaled_label[i_ref]);
	gtk_widget_hide(threshold->color_scales[i_ref]);
      
	for (i_entry=0; i_entry< AMITK_THRESHOLD_ENTRY_NUM_ENTRIES; i_entry++)
	  gtk_widget_hide(threshold->spin_button[i_ref][i_entry]);
      }
      
    } else {

      gtk_widget_show(threshold->color_scales[i_ref]);
      if (!threshold->minimal) {
	gtk_widget_show(threshold->percent_label[i_ref]);
	gtk_widget_show(threshold->absolute_label[i_ref]);
	gtk_widget_show(threshold->full_label[i_ref]);
	gtk_widget_show(threshold->scaled_label[i_ref]);
      
	for (i_entry=0; i_entry< AMITK_THRESHOLD_ENTRY_NUM_ENTRIES; i_entry++)
	  gtk_widget_show(threshold->spin_button[i_ref][i_entry]);
      }
    }
    
    if (!threshold->minimal) {

      if (AMITK_DATA_SET_THRESHOLDING(threshold->data_set) == AMITK_THRESHOLDING_GLOBAL) {
	gtk_widget_show(threshold->histogram_label);
	gtk_widget_show(threshold->histogram);
      } else {
	gtk_widget_hide(threshold->histogram_label);
	gtk_widget_hide(threshold->histogram);
      }

      if (AMITK_DATA_SET_THRESHOLDING(threshold->data_set) != AMITK_THRESHOLDING_INTERPOLATE_FRAMES) {
	gtk_widget_hide(threshold->ref_frame_label[i_ref]);
	gtk_widget_hide(threshold->threshold_ref_frame_menu[i_ref]);
      } else {
	gtk_widget_show(threshold->ref_frame_label[i_ref]);
	gtk_widget_show_all(threshold->threshold_ref_frame_menu[i_ref]);
      }
    }
    
  }

  return;
}

static void threshold_update_style(AmitkThreshold * threshold) {
  AmitkThresholdStyle i_style;

  if (threshold->minimal) return; /* no style in minimal configuration */

  switch(AMITK_DATA_SET_THRESHOLD_STYLE(threshold->data_set)) {
  case AMITK_THRESHOLD_STYLE_CENTER_WIDTH:
    gtk_label_set_text(GTK_LABEL(threshold->min_max_label[AMITK_LIMIT_MAX]), _("Center"));
    gtk_label_set_text(GTK_LABEL(threshold->min_max_label[AMITK_LIMIT_MIN]), _("Width"));
    break;
  case AMITK_THRESHOLD_STYLE_MIN_MAX:
  default:
    gtk_label_set_text(GTK_LABEL(threshold->min_max_label[AMITK_LIMIT_MAX]), _("Max Threshold"));
    gtk_label_set_text(GTK_LABEL(threshold->min_max_label[AMITK_LIMIT_MIN]), _("Min Threshold"));
    break;
  }


  for (i_style=0; i_style<AMITK_THRESHOLD_STYLE_NUM; i_style++) 
    g_signal_handlers_block_by_func(G_OBJECT(threshold->style_button[i_style]), G_CALLBACK(threshold_style_cb), threshold);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(threshold->style_button[AMITK_DATA_SET_THRESHOLD_STYLE(threshold->data_set)]),
			       TRUE);
  for (i_style=0; i_style<AMITK_THRESHOLD_STYLE_NUM; i_style++) 
    g_signal_handlers_unblock_by_func(G_OBJECT(threshold->style_button[i_style]), G_CALLBACK(threshold_style_cb), threshold);
  return;
}


/* set which toggle button is depressed */
static void threshold_update_type(AmitkThreshold * threshold) {
  
  AmitkThresholding i_thresholding;

  if (threshold->minimal) return; /* no changing of threshold type in minimal setup */
  g_return_if_fail(AMITK_IS_DATA_SET(threshold->data_set));

  for (i_thresholding=0; i_thresholding < AMITK_THRESHOLDING_NUM; i_thresholding++)
    g_signal_handlers_block_by_func(G_OBJECT(threshold->type_button[i_thresholding]),
				    G_CALLBACK(thresholding_cb), threshold);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(threshold->type_button[AMITK_DATA_SET_THRESHOLDING(threshold->data_set)]),
			       TRUE);
  for (i_thresholding=0; i_thresholding < AMITK_THRESHOLDING_NUM; i_thresholding++)
    g_signal_handlers_unblock_by_func(G_OBJECT(threshold->type_button[i_thresholding]),
				     G_CALLBACK(thresholding_cb), threshold);
  gtk_widget_set_sensitive(threshold->type_button[AMITK_THRESHOLDING_PER_FRAME],
			   (AMITK_DATA_SET_DYNAMIC(threshold->data_set)));
  gtk_widget_set_sensitive(threshold->type_button[AMITK_THRESHOLDING_INTERPOLATE_FRAMES],
			   (AMITK_DATA_SET_DYNAMIC(threshold->data_set)));

  return;
}

/* set what the "absolute" label says */
static void threshold_update_absolute_label(AmitkThreshold * threshold) {

  guint i_ref;

  if (threshold->minimal) return; /* not shown in minimal setup */
  g_return_if_fail(AMITK_IS_DATA_SET(threshold->data_set));

  for (i_ref=0; i_ref<2; i_ref++) {

    switch (AMITK_DATA_SET_CONVERSION(threshold->data_set)) {
    case AMITK_CONVERSION_SUV:
    case AMITK_CONVERSION_PERCENT_ID_PER_CC:
      gtk_label_set_text(GTK_LABEL(threshold->absolute_label[i_ref]), 
			 amitk_conversion_names[AMITK_DATA_SET_CONVERSION(threshold->data_set)]);
      break;
    case AMITK_CONVERSION_STRAIGHT:
    default:
      gtk_label_set_text(GTK_LABEL(threshold->absolute_label[i_ref]), _("Absolute"));
      break;
    }

  }

  return;
}

/* function to update whether windowing buttons are shown */
static void threshold_update_windowing(AmitkThreshold * threshold) {

  if (threshold->minimal) return; /* not shown in minimal setup */
  g_return_if_fail(AMITK_IS_DATA_SET(threshold->data_set));

  if (AMITK_DATA_SET_MODALITY(threshold->data_set) == AMITK_MODALITY_CT) {
    gtk_widget_show(threshold->window_label);
    gtk_widget_show(threshold->window_vbox);
  } else {
    gtk_widget_hide(threshold->window_label);
    gtk_widget_hide(threshold->window_vbox);
  }

  return;
}

/* function to update what are in the ref frame menus */
static void threshold_update_ref_frames(AmitkThreshold * threshold) {

  GtkTreeModel * model;
  guint i_ref, i_frame;
  gchar * temp_string;

  if (threshold->minimal) return; /* not shown in minimal setup */
  g_return_if_fail(AMITK_IS_DATA_SET(threshold->data_set));

  for (i_ref=0; i_ref<2; i_ref++) {

    g_signal_handlers_block_by_func(G_OBJECT(threshold->threshold_ref_frame_menu[i_ref]),
				    G_CALLBACK(threshold_ref_frame_cb), threshold);

    /* remove the old menu */
    model = gtk_combo_box_get_model (GTK_COMBO_BOX(threshold->threshold_ref_frame_menu[i_ref]));
    gtk_list_store_clear(GTK_LIST_STORE(model));

    /* make new menu */
    for (i_frame=0; i_frame<AMITK_DATA_SET_NUM_FRAMES(threshold->data_set); i_frame++) {
      temp_string = g_strdup_printf("%d",i_frame);
      gtk_combo_box_append_text(GTK_COMBO_BOX(threshold->threshold_ref_frame_menu[i_ref]),
				temp_string);
      g_free(temp_string);
     }
    gtk_combo_box_set_active(GTK_COMBO_BOX(threshold->threshold_ref_frame_menu[i_ref]), 
			     AMITK_DATA_SET_THRESHOLD_REF_FRAME(threshold->data_set,i_ref));
				 
    g_signal_handlers_unblock_by_func(G_OBJECT(threshold->threshold_ref_frame_menu[i_ref]),
				      G_CALLBACK(threshold_ref_frame_cb), threshold);
  }

  return;
}

static void threshold_update_color_table(AmitkThreshold * threshold, AmitkViewMode view_mode) {

  g_return_if_fail(AMITK_IS_DATA_SET(threshold->data_set));


  g_signal_handlers_block_by_func(G_OBJECT(threshold->color_table_menu[view_mode]), G_CALLBACK(color_table_cb), threshold);
  gtk_combo_box_set_active(GTK_COMBO_BOX(threshold->color_table_menu[view_mode]), 
			   AMITK_DATA_SET_COLOR_TABLE(threshold->data_set, view_mode));
  g_signal_handlers_unblock_by_func(G_OBJECT(threshold->color_table_menu[view_mode]), G_CALLBACK(color_table_cb), threshold);

  if (view_mode == AMITK_VIEW_MODE_SINGLE) {
  } else {
    g_signal_handlers_block_by_func(G_OBJECT(threshold->color_table_independent[view_mode]), G_CALLBACK(color_table_independent_cb), threshold);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(threshold->color_table_independent[view_mode]),
				 AMITK_DATA_SET_COLOR_TABLE_INDEPENDENT(threshold->data_set, view_mode));
    g_signal_handlers_unblock_by_func(G_OBJECT(threshold->color_table_independent[view_mode]), G_CALLBACK(color_table_independent_cb), threshold);

    gtk_widget_set_sensitive(threshold->color_table_menu[view_mode],
			     AMITK_DATA_SET_COLOR_TABLE_INDEPENDENT(threshold->data_set, view_mode));
  }

  return;
}

static void threshold_update_color_tables(AmitkThreshold * threshold) {

  AmitkViewMode i_view_mode;

  for (i_view_mode=0; i_view_mode < AMITK_VIEW_MODE_NUM; i_view_mode++) {

    if (i_view_mode <= threshold->view_mode) {
      gtk_widget_show(threshold->color_table_label[i_view_mode]);
      gtk_widget_show(threshold->color_table_hbox[i_view_mode]);
    } else {
      /* never reached for AMITK_VIEW_MODE_SINGLE */
      gtk_widget_hide(threshold->color_table_label[i_view_mode]);
      gtk_widget_hide(threshold->color_table_hbox[i_view_mode]);
    }

    threshold_update_color_table(threshold, i_view_mode);
  }


  return;
}



static void ds_scale_factor_changed_cb(AmitkDataSet * ds, AmitkThreshold * threshold) {

  gint i;

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  g_return_if_fail(AMITK_IS_THRESHOLD(threshold));

  for (i=0; i<2; i++) {
    threshold->threshold_max[i] = AMITK_DATA_SET_THRESHOLD_MAX(ds, i);
    threshold->threshold_min[i] = AMITK_DATA_SET_THRESHOLD_MIN(ds, i);
  }

  threshold_update_spin_buttons(threshold);

  return;
}

static void ds_color_table_changed_cb(AmitkDataSet * ds, AmitkViewMode view_mode, AmitkThreshold * threshold) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  g_return_if_fail(AMITK_IS_THRESHOLD(threshold));

  threshold_update_color_table(threshold, view_mode);
  threshold_update_color_scales(threshold);

  return;
}

static void ds_thresholding_changed_cb(AmitkDataSet * ds, AmitkThreshold * threshold) {

  AmitkThresholdArrow i_arrow;

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  g_return_if_fail(AMITK_IS_THRESHOLD(threshold));

  /* need to make sure the second reference items are made before they're shown in update_layout */
  threshold_update_color_scales(threshold); 
  for (i_arrow=0;i_arrow< AMITK_THRESHOLD_ARROW_NUM_ARROWS;i_arrow++)
    threshold_update_arrow(threshold, i_arrow);

  threshold_update_type(threshold);
#if 0
  threshold_update_ref_frames(threshold);
#endif
  threshold_update_layout(threshold);

  return;
}

static void ds_threshold_style_changed_cb(AmitkDataSet * ds, AmitkThreshold * threshold) {

  AmitkThresholdScale i_scale;

  threshold_update_style(threshold);
  threshold_update_spin_buttons(threshold);
  for (i_scale=0; i_scale < AMITK_THRESHOLD_SCALE_NUM_SCALES; i_scale++)
    threshold_update_connector_lines(threshold, i_scale);
  threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_FULL_CENTER);
  threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_SCALED_CENTER);
  return;
}

static void ds_thresholds_changed_cb(AmitkDataSet * ds, AmitkThreshold * threshold) {

  gint i;

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  g_return_if_fail(AMITK_IS_THRESHOLD(threshold));

  for (i=0; i<2; i++) {
    threshold->threshold_max[i] = AMITK_DATA_SET_THRESHOLD_MAX(ds, i);
    threshold->threshold_min[i] = AMITK_DATA_SET_THRESHOLD_MIN(ds, i);
  }

  threshold_update_connector_lines(threshold, AMITK_THRESHOLD_SCALE_FULL);
  threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_FULL_MIN);
  threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_FULL_CENTER);
  threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_FULL_MAX);
  threshold_update_ref_frames(threshold);
  threshold_update_spin_buttons(threshold);

  return;
}

static void ds_conversion_changed_cb(AmitkDataSet * ds, AmitkThreshold * threshold) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  g_return_if_fail(AMITK_IS_THRESHOLD(threshold));

  threshold_update_spin_buttons(threshold);
  threshold_update_absolute_label(threshold);
  
  return;
}

static void ds_modality_changed_cb(AmitkDataSet * ds, AmitkThreshold * threshold) {

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  g_return_if_fail(AMITK_IS_THRESHOLD(threshold));

  threshold_update_windowing(threshold);

  return;
}

static void study_view_mode_changed_cb(AmitkStudy * study, AmitkThreshold * threshold) {

  g_return_if_fail(AMITK_IS_STUDY(study));
  g_return_if_fail(AMITK_IS_THRESHOLD(threshold));

  threshold->view_mode = AMITK_STUDY_VIEW_MODE(study);
  threshold_update_color_tables(threshold);

  return;
}


/* function called when the max or min triangle is moved 
 * mostly taken from Pennington's fine book */
static gint threshold_arrow_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {

  AmitkThreshold * threshold = data;
  gdouble item_x, item_y;
  gdouble delta;
  amide_data_t temp, center, width, max, min;
  AmitkThresholdArrow which_threshold_arrow;
  guint which_ref;

  which_ref = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "which_ref"));
  
  /* get the location of the event, and convert it to our coordinate system */
  item_x = event->button.x;
  item_y = event->button.y;
  gnome_canvas_item_w2i(GNOME_CANVAS_ITEM(widget)->parent, &item_x, &item_y);

  /* figure out which of the arrows triggered the callback */
  which_threshold_arrow = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "type"));

  /* switch on the event which called this */
  switch (event->type)
    {

    case GDK_BUTTON_PRESS:
      threshold->initial_y[which_ref] = item_y;
      threshold->initial_min[which_ref] = threshold->threshold_min[which_ref];
      threshold->initial_max[which_ref] = threshold->threshold_max[which_ref];
      gnome_canvas_item_grab(GNOME_CANVAS_ITEM(threshold->arrow[which_ref][which_threshold_arrow]),
			     GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			     threshold_cursor,
			     event->button.time);
      break;

    case GDK_MOTION_NOTIFY:
      delta = threshold->initial_y[which_ref] - item_y;

      if (event->motion.state & GDK_BUTTON1_MASK) {
	delta /= THRESHOLD_COLOR_SCALE_HEIGHT;

	/* calculate how much the arrow has been moved in terms of data values */
	switch(which_threshold_arrow) {
	case AMITK_THRESHOLD_ARROW_FULL_MAX:
	case AMITK_THRESHOLD_ARROW_FULL_CENTER:
	case AMITK_THRESHOLD_ARROW_FULL_MIN:
	  delta *= (amitk_data_set_get_global_max(threshold->data_set) - 
		    amitk_data_set_get_global_min(threshold->data_set));
	  break;
	case AMITK_THRESHOLD_ARROW_SCALED_MAX:
	case AMITK_THRESHOLD_ARROW_SCALED_CENTER:
	case AMITK_THRESHOLD_ARROW_SCALED_MIN:
	  delta *= (threshold->initial_max[which_ref] - threshold->initial_min[which_ref]);
	  break;
	default:
	  delta=0; /* shouldn't get here */
	  break;
	}
	
	switch (which_threshold_arrow) {
	case AMITK_THRESHOLD_ARROW_FULL_MAX:
	case AMITK_THRESHOLD_ARROW_SCALED_MAX:
	  temp = threshold->initial_max[which_ref] + delta;
	  if (temp < amitk_data_set_get_global_min(threshold->data_set))
	    temp = amitk_data_set_get_global_min(threshold->data_set);
	  if (AMITK_DATA_SET_THRESHOLD_STYLE(threshold->data_set) == AMITK_THRESHOLD_STYLE_CENTER_WIDTH) {
	    center = (threshold->initial_max[which_ref]+threshold->initial_min[which_ref])/2.0;
	    if ((temp-EPSILON*fabs(temp)) < center) temp = center+EPSILON*fabs(center);
	    width = 2.0*(temp-center);
	    threshold->threshold_max[which_ref] = center+width/2.0;
	    threshold->threshold_min[which_ref] = center-width/2.0;
	    threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_FULL_MIN);
	    threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_FULL_MAX);
	    if (which_threshold_arrow == AMITK_THRESHOLD_ARROW_SCALED_MAX) {
	      threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_SCALED_MAX);
	      threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_SCALED_MIN);
	    }
	  } else {
	    if ((temp-EPSILON*fabs(temp)) < threshold->threshold_min[which_ref]) 
	      temp = threshold->threshold_min[which_ref]+EPSILON*fabs(threshold->threshold_min[which_ref]);
	    threshold->threshold_max[which_ref] = temp;
	    threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_FULL_MAX);
	    if (which_threshold_arrow == AMITK_THRESHOLD_ARROW_SCALED_MAX)
	      threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_SCALED_MAX);
	  }

	  if (which_threshold_arrow == AMITK_THRESHOLD_ARROW_FULL_MAX)
	    threshold_update_connector_lines(threshold, AMITK_THRESHOLD_SCALE_FULL);
	  else { 
	    threshold_update_color_scale(threshold, AMITK_THRESHOLD_SCALE_SCALED);
	    threshold_update_connector_lines(threshold, AMITK_THRESHOLD_SCALE_SCALED);
	  }
	  break;

	case AMITK_THRESHOLD_ARROW_FULL_CENTER:
	case AMITK_THRESHOLD_ARROW_SCALED_CENTER:
	  g_return_val_if_fail(AMITK_DATA_SET_THRESHOLD_STYLE(threshold->data_set) == AMITK_THRESHOLD_STYLE_CENTER_WIDTH, FALSE);
	  center = (threshold->initial_max[which_ref]+threshold->initial_min[which_ref])/2.0;
	  width = (threshold->initial_max[which_ref]-threshold->initial_min[which_ref]);
	  temp = center+delta;
	  if (temp < amitk_data_set_get_global_min(threshold->data_set))
	    temp = amitk_data_set_get_global_min(threshold->data_set);
	  if (temp > amitk_data_set_get_global_max(threshold->data_set)) 
	    temp = amitk_data_set_get_global_max(threshold->data_set);
	  threshold->threshold_max[which_ref] = temp+width/2.0;
	  threshold->threshold_min[which_ref] = temp-width/2.0;
	  threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_FULL_MIN);
	  threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_FULL_CENTER);
	  threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_FULL_MAX);
	  if (which_threshold_arrow == AMITK_THRESHOLD_ARROW_SCALED_CENTER) {
	    threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_SCALED_MIN);
	    threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_SCALED_CENTER);
	    threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_SCALED_MAX);
	  }
	  if (which_threshold_arrow == AMITK_THRESHOLD_ARROW_FULL_CENTER)
	    threshold_update_connector_lines(threshold, AMITK_THRESHOLD_SCALE_FULL);
	  else { 
	    threshold_update_color_scale(threshold, AMITK_THRESHOLD_SCALE_SCALED);
	    threshold_update_connector_lines(threshold, AMITK_THRESHOLD_SCALE_SCALED);
	  }
	  break;

	case AMITK_THRESHOLD_ARROW_FULL_MIN:
	case AMITK_THRESHOLD_ARROW_SCALED_MIN:
	  temp = threshold->initial_min[which_ref] + delta;
	  if (temp > amitk_data_set_get_global_max(threshold->data_set)) 
	    temp = amitk_data_set_get_global_max(threshold->data_set);
	  //	  if (temp < amitk_data_set_get_global_min(threshold->data_set)) 
	  //	    temp = amitk_data_set_get_global_min(threshold->data_set);
	  if (AMITK_DATA_SET_THRESHOLD_STYLE(threshold->data_set) == AMITK_THRESHOLD_STYLE_CENTER_WIDTH) {
	    center = (threshold->initial_max[which_ref]+threshold->initial_min[which_ref])/2.0;
	    if ((temp+EPSILON*fabs(temp)) > center) temp = center-EPSILON*fabs(center);
	    width = 2.0*(center-temp);
	    threshold->threshold_max[which_ref] = center+width/2.0;
	    threshold->threshold_min[which_ref] = center-width/2.0;
	    threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_FULL_MIN);
	    threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_FULL_MAX);
	    if (which_threshold_arrow == AMITK_THRESHOLD_ARROW_SCALED_MIN) {
	      threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_SCALED_MAX);
	      threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_SCALED_MIN);
	    }
	  } else {
	    if ((temp+EPSILON*fabs(temp)) > threshold->threshold_max[which_ref]) 
	      temp = threshold->threshold_max[which_ref]-EPSILON*fabs(threshold->threshold_max[which_ref]);
	    threshold->threshold_min[which_ref] = temp;
	    threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_FULL_MIN);
	    if (which_threshold_arrow == AMITK_THRESHOLD_ARROW_SCALED_MIN)
	      threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_SCALED_MIN);
	  }
	  if (which_threshold_arrow == AMITK_THRESHOLD_ARROW_FULL_MIN)
	    threshold_update_connector_lines(threshold, AMITK_THRESHOLD_SCALE_FULL);
	  else { 
	    threshold_update_color_scale(threshold, AMITK_THRESHOLD_SCALE_SCALED);
	    threshold_update_connector_lines(threshold, AMITK_THRESHOLD_SCALE_SCALED);
	  }
	  break;
	default:
	  break;
	}

	threshold_update_color_scale(threshold, AMITK_THRESHOLD_SCALE_FULL);
	threshold_update_spin_buttons(threshold);   /* show the current val in the spin button's */
      }
      break;

    case GDK_BUTTON_RELEASE:

      if (AMITK_DATA_SET_THRESHOLD_STYLE(threshold->data_set) == AMITK_THRESHOLD_STYLE_MIN_MAX) {
	switch (which_threshold_arrow) {
	case AMITK_THRESHOLD_ARROW_SCALED_MAX:
	case AMITK_THRESHOLD_ARROW_FULL_MAX:
	  amitk_data_set_set_threshold_max(threshold->data_set, which_ref,
					   threshold->threshold_max[which_ref]);
	  break;
	case AMITK_THRESHOLD_ARROW_SCALED_MIN:
	case AMITK_THRESHOLD_ARROW_FULL_MIN:
	  amitk_data_set_set_threshold_min(threshold->data_set, which_ref,
					   threshold->threshold_min[which_ref]);
	  break;
	default:
	  g_return_val_if_reached(FALSE);
	  break;
	} 
      } else { /* AMITK_THRESHOLD_STYLE_CENTER_WIDTH */
	/* need to do this, as the first set_threshold call will cause
	   threshold->threshold_* to change due to a callback */
	max = threshold->threshold_max[which_ref];
	min = threshold->threshold_min[which_ref];
	amitk_data_set_set_threshold_max(threshold->data_set, which_ref,max);
	amitk_data_set_set_threshold_min(threshold->data_set, which_ref,min);
      } 


      gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);

      /* reset the scaled color scale */
      threshold->initial_max[which_ref] = threshold->threshold_max[which_ref];
      threshold->initial_min[which_ref] = threshold->threshold_min[which_ref];
      
      threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_SCALED_MAX);
      threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_SCALED_CENTER);
      threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_SCALED_MIN);
      threshold_update_color_scale(threshold, AMITK_THRESHOLD_SCALE_SCALED);
      threshold_update_connector_lines(threshold, AMITK_THRESHOLD_SCALE_SCALED);

      break;

    default:
      break;
    }
      
  return FALSE;
}


/* function to change the color table */
static void color_table_cb(GtkWidget * widget, gpointer data) {

  AmitkColorTable i_color_table;
  AmitkThreshold * threshold = data;
  AmitkViewMode view_mode;

  i_color_table = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  view_mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "view_mode"));

  /* check if we actually changed values */
  if (AMITK_DATA_SET_COLOR_TABLE(threshold->data_set, view_mode) != i_color_table) 
    amitk_data_set_set_color_table(AMITK_DATA_SET(threshold->data_set), view_mode, i_color_table);
  
  return;
}

static void color_table_independent_cb(GtkWidget * widget, gpointer data) {

  AmitkThreshold * threshold = data;
  AmitkViewMode view_mode;
  gboolean state;

  view_mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "view_mode"));

  /* get the state of the button */
  state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  if (AMITK_DATA_SET_COLOR_TABLE_INDEPENDENT(threshold->data_set, view_mode) != state)
    amitk_data_set_set_color_table_independent(AMITK_DATA_SET(threshold->data_set), view_mode, state);

  return;
}

/* function to update the reference frames */
static void threshold_ref_frame_cb(GtkWidget * widget, gpointer data) {

  guint which_ref,frame;
  AmitkThreshold * threshold = data;

  /* figure out which menu item called me */
  which_ref = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"which_ref"));
  frame = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

  /* check if we actually changed values */
  if (frame != AMITK_DATA_SET_THRESHOLD_REF_FRAME(threshold->data_set, which_ref)) {
    
    if ((which_ref==0) && (frame > AMITK_DATA_SET_THRESHOLD_REF_FRAME(threshold->data_set,1)))
      frame =AMITK_DATA_SET_THRESHOLD_REF_FRAME(threshold->data_set,1);
    else if ((which_ref==1) && (frame < AMITK_DATA_SET_THRESHOLD_REF_FRAME(threshold->data_set,0)))
      frame = AMITK_DATA_SET_THRESHOLD_REF_FRAME(threshold->data_set,0);

    /* and inact the changes */
    amitk_data_set_set_threshold_ref_frame(threshold->data_set, which_ref, frame);

  }
  
  return;
}


static void threshold_spin_button_cb(GtkWidget* widget, gpointer data) {

  AmitkThreshold * threshold = data;
  amide_data_t global_diff, temp;
  amide_data_t center, width;
  amide_data_t min_val=0.0;
  amide_data_t max_val=0.0;
  gboolean max_val_changed=FALSE;
  gboolean min_val_changed=FALSE;
  AmitkThresholdEntry which_threshold_entry;
  guint which_ref;


  /* figure out which of the arrows triggered the callback */
  which_threshold_entry = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "type"));
  which_ref = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "which_ref"));

  temp = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

  global_diff = 
    amitk_data_set_get_global_max(threshold->data_set)- 
    amitk_data_set_get_global_min(threshold->data_set);
  if (global_diff < EPSILON) global_diff = EPSILON; /* non sensicle */


  switch (AMITK_DATA_SET_THRESHOLD_STYLE(threshold->data_set)) {

  case AMITK_THRESHOLD_STYLE_CENTER_WIDTH:
    switch (which_threshold_entry) {
      /* max are the "center" entries */
    case AMITK_THRESHOLD_ENTRY_MAX_PERCENT:
      temp = (global_diff*temp/100.0)+amitk_data_set_get_global_min(threshold->data_set);
    case AMITK_THRESHOLD_ENTRY_MAX_ABSOLUTE:
      center = temp;
      width = threshold->threshold_max[which_ref]-threshold->threshold_min[which_ref];
      break;
      /* min are the "width" entries */
    case AMITK_THRESHOLD_ENTRY_MIN_PERCENT:
      temp = (global_diff*temp/100.0);
    case AMITK_THRESHOLD_ENTRY_MIN_ABSOLUTE:
    default:
      center = (threshold->threshold_max[which_ref]+threshold->threshold_min[which_ref])/2.0;
      width = temp;
      break;
    }
    max_val = center+width/2.0;
    min_val = center-width/2.0;
    max_val_changed=TRUE;
    min_val_changed=TRUE;
    break;

  case AMITK_THRESHOLD_STYLE_MIN_MAX:
  default:
    switch (which_threshold_entry) {
    case AMITK_THRESHOLD_ENTRY_MAX_PERCENT:
      temp = (global_diff*temp/100.0)+amitk_data_set_get_global_min(threshold->data_set);
    case AMITK_THRESHOLD_ENTRY_MAX_ABSOLUTE:
      max_val = temp;
      max_val_changed = TRUE;
      break;
    case AMITK_THRESHOLD_ENTRY_MIN_PERCENT:
      temp = (global_diff * temp/100.0)+amitk_data_set_get_global_min(threshold->data_set);
    case AMITK_THRESHOLD_ENTRY_MIN_ABSOLUTE:
    default:
      min_val = temp;
      min_val_changed = TRUE;
      break;
    }
  }
  
  if (min_val_changed) {
    /* make sure it's a valid floating point */
      if ((min_val < amitk_data_set_get_global_max(threshold->data_set)) && 
	  (min_val < threshold->threshold_max[which_ref])) {
      }
	amitk_data_set_set_threshold_min(threshold->data_set, which_ref, min_val);
	threshold->threshold_min[which_ref] = min_val;

  } 
  if (max_val_changed) {
      if ((max_val > threshold->threshold_min[which_ref]) && 
	  (max_val > amitk_data_set_get_global_min(threshold->data_set))) {
      }
	amitk_data_set_set_threshold_max(threshold->data_set, which_ref, max_val);
	threshold->threshold_max[which_ref] = max_val;
  }

  /* need to manually twiddle the spin button's, rejected values of 
     amitk_data_set_set_threshold_whatever won't elicit a signal */
  threshold_update_spin_buttons(threshold);

  return;
}


static void thresholding_cb(GtkWidget * widget, gpointer data) {

  AmitkThreshold * threshold = data;
  AmitkThresholding thresholding;

  thresholding = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"thresholding"));

  /* check if we actually changed values */
  if (AMITK_DATA_SET_THRESHOLDING(threshold->data_set) != thresholding) 
    amitk_data_set_set_thresholding(threshold->data_set, thresholding);
  
  return;
}

static void threshold_style_cb(GtkWidget * widget, gpointer data) {

  AmitkThreshold * threshold = data;

  amitk_data_set_set_threshold_style(threshold->data_set,
				     GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "threshold_style")));
  return;
}


static void windowing_cb(GtkWidget * widget, gpointer data) {

  AmitkThreshold * threshold = data;
  AmitkWindow window;
  gint i;

  window = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "which_window"));
  for (i=0; i<2; i++) {
    amitk_data_set_set_threshold_min(threshold->data_set, i, 
				     AMITK_DATA_SET_THRESHOLD_WINDOW(threshold->data_set, window, AMITK_LIMIT_MIN));
    amitk_data_set_set_threshold_max(threshold->data_set, i, 
				     AMITK_DATA_SET_THRESHOLD_WINDOW(threshold->data_set, window, AMITK_LIMIT_MAX));
  }
}


/* function to load a new data set into the widget */
void amitk_threshold_new_data_set(AmitkThreshold * threshold, AmitkDataSet * new_data_set) {

  AmitkThresholdArrow i_arrow;

  g_return_if_fail(AMITK_IS_THRESHOLD(threshold));
  g_return_if_fail(AMITK_IS_DATA_SET(new_data_set));

  threshold_remove_data_set(threshold);
  threshold_add_data_set(threshold, new_data_set);

  threshold_update_histogram(threshold);
  threshold_update_layout(threshold);
  threshold_update_color_tables(threshold);
  threshold_update_color_scales(threshold);
  threshold_update_spin_buttons(threshold);
  threshold_update_type(threshold);
  threshold_update_style(threshold);
  threshold_update_absolute_label(threshold);
  threshold_update_windowing(threshold);
  threshold_update_ref_frames(threshold);

  /* FULL Arrows obviously need to get redrawn.  SCALED arrows are also
     redrawn, as they may not have yet been drawn */
  for (i_arrow=0;i_arrow< AMITK_THRESHOLD_ARROW_NUM_ARROWS;i_arrow++)
    threshold_update_arrow(threshold, i_arrow);

  return;
}


/* data set can be NULL */
GtkWidget* amitk_threshold_new (AmitkDataSet * ds,
				AmitkThresholdLayout layout,
				GtkWindow * parent,
				gboolean minimal) {

  AmitkThreshold * threshold;

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), NULL);
  g_return_val_if_fail(parent != NULL, NULL);

  threshold = g_object_new (amitk_threshold_get_type (), NULL);

  threshold->minimal = minimal;

  threshold->progress_dialog = amitk_progress_dialog_new(parent); /* gets killed with parent */
  threshold_construct(threshold, layout);

  if (ds != NULL) 
    amitk_threshold_new_data_set(threshold, ds);

  return GTK_WIDGET (threshold);
}






GType amitk_threshold_dialog_get_type (void)
{
  static GType threshold_dialog_type = 0;

  if (!threshold_dialog_type)
    {
      GTypeInfo threshold_dialog_info =
      {
	sizeof (AmitkThresholdDialogClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) threshold_dialog_class_init,
	(GClassFinalizeFunc) NULL,
	NULL, /* class data */
	sizeof (AmitkThresholdDialog),
	0,   /* # preallocs */
	(GInstanceInitFunc) threshold_dialog_init,
	NULL   /* value table */
      };

      threshold_dialog_type = 
	g_type_register_static(GTK_TYPE_DIALOG, "AmitkThresholdDialog", &threshold_dialog_info, 0);
    }

  return threshold_dialog_type;
}

static void threshold_dialog_class_init (AmitkThresholdDialogClass *klass)
{
  /*  GtkObjectClass *gtkobject_class; */

  /* gtkobject_class = (GtkObjectClass*) klass; */

  threshold_dialog_parent_class = g_type_class_peek_parent(klass);

}

static void init_common(GtkWindow * dialog) {

  GdkPixbuf * pixbuf;

  g_return_if_fail(GTK_IS_WINDOW(dialog));

  pixbuf =  
    gtk_widget_render_icon(GTK_WIDGET(dialog), "amide_icon_thresholding", -1, 0);
  gtk_window_set_icon(dialog, pixbuf);
  g_object_unref(pixbuf);

  return;
}


static void threshold_dialog_init (AmitkThresholdDialog * dialog) {

  init_common(GTK_WINDOW(dialog));

  gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);

  dialog->vbox = gtk_vbox_new (FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 10);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), dialog->vbox);
  gtk_widget_show (dialog->vbox);

  /* make the data set title thingie */
  dialog->data_set_label = gtk_label_new(NULL);
  gtk_box_pack_start(GTK_BOX(dialog->vbox), dialog->data_set_label, FALSE, FALSE,0);
  gtk_widget_show(dialog->data_set_label);
  
  return;
}

/* this gets called after we have a data_set */
static void threshold_dialog_construct(AmitkThresholdDialog * dialog, 
				       GtkWindow * parent,
				       AmitkDataSet * data_set) {

  gchar * temp_string;

  dialog->threshold = amitk_threshold_new(data_set, AMITK_THRESHOLD_LINEAR_LAYOUT, 
					  parent, FALSE);
  gtk_box_pack_end(GTK_BOX(dialog->vbox), dialog->threshold, TRUE, TRUE, 0);
  gtk_widget_show(dialog->threshold);

  /* reset the title */
  temp_string = g_strdup_printf(_("Data Set: %s\n"),AMITK_OBJECT_NAME(data_set));
  gtk_label_set_text (GTK_LABEL(dialog->data_set_label), temp_string);
  g_free(temp_string);

  return;
}


GtkWidget* amitk_threshold_dialog_new (AmitkDataSet * data_set, GtkWindow * parent)
{
  AmitkThresholdDialog *dialog;

  g_return_val_if_fail(AMITK_IS_DATA_SET(data_set), NULL);

  if (AMITK_OBJECT(data_set)->dialog != NULL)
    return NULL;

  dialog = g_object_new(AMITK_TYPE_THRESHOLD_DIALOG, NULL);
  threshold_dialog_construct(dialog, parent, data_set);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Threshold Dialog"));
  gtk_window_set_transient_for(GTK_WINDOW (dialog), parent);
  gtk_window_set_destroy_with_parent(GTK_WINDOW (dialog), TRUE);
  gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE); /* allows auto shrink */


  return GTK_WIDGET (dialog);
}


/* function to load a new data set into the widget */
void amitk_threshold_dialog_new_data_set(AmitkThresholdDialog * dialog, AmitkDataSet * new_data_set) {

  gchar * temp_string;

  g_return_if_fail(AMITK_IS_THRESHOLD_DIALOG(dialog));
  g_return_if_fail(AMITK_IS_DATA_SET(new_data_set));

  amitk_threshold_new_data_set(AMITK_THRESHOLD(dialog->threshold), new_data_set);

  /* reset the title */
  temp_string = g_strdup_printf(_("Data Set: %s\n"),AMITK_OBJECT_NAME(new_data_set));
  gtk_label_set_text (GTK_LABEL(dialog->data_set_label), temp_string);
  g_free(temp_string);

  return;
}

/* function to return a pointer to the data set that this dialog currently referes to */
AmitkDataSet * amitk_threshold_dialog_data_set(AmitkThresholdDialog * dialog) {

  return AMITK_THRESHOLD(dialog->threshold)->data_set;
}




/* -------------- thresholds_dialog ---------------- */

GType amitk_thresholds_dialog_get_type (void)
{
  static GType thresholds_dialog_type = 0;

  if (!thresholds_dialog_type)
    {
      GTypeInfo thresholds_dialog_info =
      {
	sizeof (AmitkThresholdsDialogClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) thresholds_dialog_class_init,
	(GClassFinalizeFunc) NULL,
	NULL, /* class data */
	sizeof (AmitkThresholdsDialog),
	0,   /* # preallocs */
	(GInstanceInitFunc) thresholds_dialog_init,
	NULL   /* value table */
      };

      thresholds_dialog_type = 
	g_type_register_static(GTK_TYPE_DIALOG, "AmitkThresholdsDialog", &thresholds_dialog_info, 0);
    }

  return thresholds_dialog_type;
}


static void thresholds_dialog_class_init (AmitkThresholdsDialogClass *klass)
{
  /*  GtkObjectClass *gtkobject_class; */

  /* gtkobject_class = (GtkObjectClass*) klass; */

  thresholds_dialog_parent_class = g_type_class_peek_parent(klass);

}


static void thresholds_dialog_init (AmitkThresholdsDialog * dialog)
{
  GtkWidget * main_vbox;

  init_common(GTK_WINDOW(dialog));

  dialog->thresholds = NULL;

  gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
  main_vbox = gtk_vbox_new (FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 10);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  dialog->notebook = gtk_notebook_new();
  gtk_container_add (GTK_CONTAINER (main_vbox), dialog->notebook);
  gtk_widget_show (dialog->notebook);

}

/* this gets called after we have a list of data set */
static void thresholds_dialog_construct(AmitkThresholdsDialog * dialog, 
					GtkWindow * parent,
					GList * data_sets) {

  GtkWidget * threshold;
  GtkWidget * label;

  while (data_sets != NULL) {
    g_return_if_fail(AMITK_IS_DATA_SET(data_sets->data));

    threshold = amitk_threshold_new(data_sets->data, AMITK_THRESHOLD_LINEAR_LAYOUT, 
				    parent, FALSE);
    dialog->thresholds = g_list_append(dialog->thresholds, threshold);
    
    label = gtk_label_new(AMITK_OBJECT_NAME(data_sets->data));

    gtk_notebook_append_page(GTK_NOTEBOOK(dialog->notebook), threshold, label);
      
    gtk_widget_show(threshold);
    gtk_widget_show(label);

    data_sets = data_sets->next;
  }

  return;
}



GtkWidget* amitk_thresholds_dialog_new (GList * objects, GtkWindow * parent) {

  AmitkThresholdsDialog *dialog;
  GList * data_sets;

  g_return_val_if_fail(objects != NULL, NULL);

  /* check that the list contains at leat 1 data set */
  data_sets = amitk_objects_get_of_type(objects, AMITK_OBJECT_TYPE_DATA_SET, FALSE);
  g_return_val_if_fail(data_sets != NULL, NULL);

  dialog = g_object_new(AMITK_TYPE_THRESHOLDS_DIALOG, NULL);
  thresholds_dialog_construct(dialog, parent, data_sets);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Threshold Dialog"));
  
  gtk_window_set_transient_for(GTK_WINDOW (dialog), parent);
  gtk_window_set_destroy_with_parent(GTK_WINDOW (dialog), TRUE);
  gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE); /* allows auto shrink */
  
  amitk_objects_unref(data_sets);
  
  return GTK_WIDGET (dialog);

}
