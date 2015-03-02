/* amitk_threshold.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2003 Andy Loening
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
#include "amitk_progress_dialog.h"
#include "amitk_threshold.h"
#include "pixmaps.h"
#include "image.h"
#include "amitk_marshal.h"

#define THRESHOLD_COLOR_SCALE_SEPARATION 30.0
#define THRESHOLD_COLOR_SCALE_WIDTH 16.0
#define THRESHOLD_COLOR_SCALES_WIDTH (2.0*THRESHOLD_COLOR_SCALE_WIDTH+THRESHOLD_COLOR_SCALE_SEPARATION)
#define THRESHOLD_COLOR_SCALE_HEIGHT (gdouble) AMITK_DATA_SET_DISTRIBUTION_SIZE
#define THRESHOLD_HISTOGRAM_WIDTH (gdouble) IMAGE_DISTRIBUTION_WIDTH
#define THRESHOLD_TRIANGLE_WIDTH 16.0
#define THRESHOLD_TRIANGLE_HEIGHT 12.0
#define THRESHOLD_SPIN_BUTTON_DIGITS 4 /* how many digits after the decimal point */

#define MENU_COLOR_SCALE_HEIGHT 8
#define MENU_COLOR_SCALE_WIDTH 30


static gchar * thresholding_names[] = {
  "per slice", 
  "per frame", 
  "interpolated between frames",
  "global"
};

static gchar * thresholding_explanations[] = {
  "threshold the images based on the max and min values in the current slice",
  "threshold the images based on the max and min values in the current frame",
  "threshold the images based on max and min values interpolated from the reference frame thresholds",
  "threshold the images based on the max and min values of the entire data set",
};

static void threshold_class_init (AmitkThresholdClass *klass);
static void threshold_init (AmitkThreshold *threshold);
static void threshold_destroy (GtkObject *object);
static void threshold_show_all (GtkWidget * widget);
static void threshold_construct(AmitkThreshold * threshold, AmitkThresholdLayout layout);
static void threshold_add_data_set(AmitkThreshold * threshold, AmitkDataSet * ds);
static void threshold_remove_data_set(AmitkThreshold * threshold);
static void threshold_realize_color_table_menu_item_cb(GtkWidget * hbox, gpointer data);
static void threshold_realize_type_icon_cb(GtkWidget * hbox, gpointer data);
static void threshold_update_histogram(AmitkThreshold * threshold);
static void threshold_update_spin_buttons(AmitkThreshold * threshold);
static void threshold_update_arrow(AmitkThreshold * threshold, AmitkThresholdArrow arrow);
static void threshold_update_color_scale(AmitkThreshold * threshold, AmitkThresholdScale scale);
static void threshold_update_connector_lines(AmitkThreshold * threshold, AmitkThresholdScale scale);
static void threshold_update_color_scales(AmitkThreshold * threshold);
static void threshold_update_layout(AmitkThreshold * threshold);
static void threshold_update_type(AmitkThreshold * threshold);
static gint threshold_arrow_cb(GtkWidget* widget, GdkEvent * event, gpointer data);
static void color_table_cb(GtkWidget * widget, gpointer data);
static void data_set_changed_cb(AmitkDataSet * ds, AmitkThreshold* threshold);
static void threshold_ref_frame_cb(GtkWidget* widget, gpointer data);
static void threshold_spin_button_cb(GtkWidget* widget, gpointer data);
static void thresholding_cb(GtkWidget * widget, gpointer data);
static void threshold_update(AmitkThreshold * threshold);

static void threshold_dialog_class_init (AmitkThresholdDialogClass *klass);
static void threshold_dialog_init (AmitkThresholdDialog *threshold_dialog);
static void threshold_dialog_construct(AmitkThresholdDialog * dialog, GtkWindow * parent, AmitkDataSet * data_set);
static void threshold_dialog_realize_cb(GtkWidget * dialog, gpointer data);

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
  
  AmitkThreshold * threshold;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (AMITK_IS_THRESHOLD(widget));

  threshold = AMITK_THRESHOLD(widget);
  
  gtk_widget_show(widget);

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
  GtkWidget * label;
  GtkWidget * option_menu;
  GtkWidget * menu;
  GtkWidget * menuitem;
  GtkWidget * pix_box;
  AmitkColorTable i_color_table;
  AmitkThresholding i_thresholding;
  guint i_ref,i_frame;
  gchar * temp_string;
  AmitkThresholdEntry i_entry;
  GtkTooltips * radio_button_tips;
  amide_data_t step;

  /* we're using two tables packed into a box */
  if (layout == AMITK_THRESHOLD_BOX_LAYOUT)
    main_box = gtk_hbox_new(FALSE,0);
  else /* layout == AMITK_THRESHOLD_LINEAR_LAYOUT */
    main_box = gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(threshold), main_box);


  /*---------------------------------------
    left table 
    --------------------------------------- */
  
  left_table = gtk_table_new(7,5,FALSE);
  left_row=0;
  gtk_box_pack_start(GTK_BOX(main_box), left_table, TRUE,TRUE,0);
  
  if (!threshold->minimal) {
    for (i_ref=0; i_ref<2; i_ref++) {
      threshold->percent_label[i_ref] = gtk_label_new("Percent");
      gtk_table_attach(GTK_TABLE(left_table), threshold->percent_label[i_ref], 
		       1+2*i_ref,2+2*i_ref,left_row,left_row+1,
		       X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
      /* show/hide taken care of by threshold_update_layout */
      
      threshold->absolute_label[i_ref] = gtk_label_new("Absolute");
      gtk_table_attach(GTK_TABLE(left_table), threshold->absolute_label[i_ref], 
		       2+2*i_ref,3+2*i_ref,left_row,left_row+1,
		       X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
      /* show/hide taken care of by threshold_update_layout */
    }
    left_row++;
    
    
    label = gtk_label_new("Max Threshold");
    gtk_table_attach(GTK_TABLE(left_table),  label, 0,1,left_row,left_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);
    
    label = gtk_label_new("Min Threshold");
    gtk_table_attach(GTK_TABLE(left_table), label, 0,1, left_row+1,left_row+2,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);
    

    step = AMITK_DATA_SET_GLOBAL_MAX(threshold->data_set)-AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set);
    step /= 100;
    if (step == 0.0) step = EPSILON;

    for (i_ref=0; i_ref<2; i_ref++) {
      threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MAX_ABSOLUTE] = 
	gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, step);
      threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MIN_ABSOLUTE] = 
	gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, step);
      threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MAX_PERCENT] = 
	gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 1.0);
      threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MIN_PERCENT] = 
	gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 1.0);
      for (i_entry=0; i_entry< AMITK_THRESHOLD_ENTRY_NUM_ENTRIES; i_entry++) {
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(threshold->spin_button[i_ref][i_entry]), 
				   THRESHOLD_SPIN_BUTTON_DIGITS);
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

  /* color table selector */
  label = gtk_label_new("color table:");
  gtk_table_attach(GTK_TABLE(left_table), label, 0,1, left_row,left_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(label);

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();

  for(i_color_table=0; i_color_table<AMITK_COLOR_TABLE_NUM; i_color_table++) {
    menuitem = gtk_menu_item_new();
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(menuitem), hbox);

    pix_box = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), pix_box, FALSE, FALSE, 3);
    gtk_widget_show(pix_box);
    g_signal_connect(G_OBJECT(pix_box), "realize",  G_CALLBACK(threshold_realize_color_table_menu_item_cb), 
		     GINT_TO_POINTER(i_color_table));

    label = gtk_label_new(color_table_menu_names[i_color_table]);
    gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 3);
    gtk_widget_show(label);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  }

  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  g_signal_connect(G_OBJECT(option_menu), "changed", G_CALLBACK(color_table_cb), threshold);
  gtk_widget_set_size_request(option_menu, 185, -1);
  gtk_table_attach(GTK_TABLE(left_table), option_menu, 1,5, left_row,left_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show_all(option_menu);
  threshold->color_table_menu = option_menu;

  left_row++;

  if (!threshold->minimal) {
    /* threshold type selection */
    label = gtk_label_new("threshold type");
    gtk_table_attach(GTK_TABLE(left_table), label, 0,1, left_row,left_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);
    
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(left_table), hbox, 1,5, left_row,left_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(hbox);

    radio_button_tips = gtk_tooltips_new();

    for (i_thresholding = 0; i_thresholding < AMITK_THRESHOLDING_NUM; i_thresholding++) {
      
      threshold->type_button[i_thresholding] = gtk_radio_button_new(NULL);
      pix_box = gtk_hbox_new(FALSE, 0);
      gtk_container_add(GTK_CONTAINER(threshold->type_button[i_thresholding]), pix_box);
      gtk_widget_show(pix_box);
      g_signal_connect(G_OBJECT(pix_box), "realize", 
		       G_CALLBACK(threshold_realize_type_icon_cb), GINT_TO_POINTER(i_thresholding));
      
      gtk_box_pack_start(GTK_BOX(hbox), threshold->type_button[i_thresholding], FALSE, FALSE, 3);
      gtk_widget_show(threshold->type_button[i_thresholding]);
      gtk_tooltips_set_tip(radio_button_tips, threshold->type_button[i_thresholding], 
			   thresholding_names[i_thresholding],
			   thresholding_explanations[i_thresholding]);
      
      g_object_set_data(G_OBJECT(threshold->type_button[i_thresholding]), "thresholding", 
			GINT_TO_POINTER(i_thresholding));
      
      if (i_thresholding != 0)
	gtk_radio_button_set_group(GTK_RADIO_BUTTON(threshold->type_button[i_thresholding]), 
				   gtk_radio_button_get_group(GTK_RADIO_BUTTON(threshold->type_button[0])));
      
      g_signal_connect(G_OBJECT(threshold->type_button[i_thresholding]), "clicked",  
		       G_CALLBACK(thresholding_cb), threshold);
    }
    threshold_update_type(threshold);
  }




  /*---------------------------------------
    right table 
    --------------------------------------- */
  right_table = gtk_table_new(4,5,FALSE);
  right_row=0;
  gtk_box_pack_start(GTK_BOX(main_box), right_table, TRUE,TRUE,0);

  /* color table selector */
  threshold->ref_frame_label[0] = gtk_label_new("ref frame 0:");
  gtk_table_attach(GTK_TABLE(right_table), threshold->ref_frame_label[0], 1,2, right_row,right_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  /* show/hide taken care of by threshold_update_layout */
  
  threshold->ref_frame_label[1] = gtk_label_new("ref frame 1:");
  gtk_table_attach(GTK_TABLE(right_table), threshold->ref_frame_label[1], 3,4, right_row,right_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  /* show/hide taken care of by threshold_update_layout */

  for (i_ref=0; i_ref<2; i_ref++) {

    threshold->threshold_ref_frame_menu[i_ref] = gtk_option_menu_new();
    menu = gtk_menu_new();

    for (i_frame=0; i_frame<AMITK_DATA_SET_NUM_FRAMES(threshold->data_set); i_frame++) {
      temp_string = g_strdup_printf("%d",i_frame);
      menuitem = gtk_menu_item_new_with_label(temp_string);
      g_free(temp_string);
      
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
      g_object_set_data(G_OBJECT(menuitem), "frame", GINT_TO_POINTER(i_frame));
      g_object_set_data(G_OBJECT(menuitem), "which_ref", GINT_TO_POINTER(i_ref));
      g_signal_connect(G_OBJECT(menuitem), "activate", 
		       G_CALLBACK(threshold_ref_frame_cb), threshold);
    }

    gtk_option_menu_set_menu(GTK_OPTION_MENU(threshold->threshold_ref_frame_menu[i_ref]), menu);
    gtk_table_attach(GTK_TABLE(right_table), threshold->threshold_ref_frame_menu[i_ref],
		     2+i_ref*2,3+i_ref*2, right_row,right_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_set_size_request(threshold->threshold_ref_frame_menu[i_ref], 50, -1);
    /* show/hide taken care of by threshold_update_layout */
    gtk_option_menu_set_history(GTK_OPTION_MENU(threshold->threshold_ref_frame_menu[i_ref]), 
				AMITK_DATA_SET_THRESHOLD_REF_FRAME(threshold->data_set,i_ref));
  }


  /* puts these labels on bottom for a linear layout */
  if (layout == AMITK_THRESHOLD_LINEAR_LAYOUT) right_row = 3;
  else right_row++;

  if (!threshold->minimal) {
    label = gtk_label_new("distribution");
    gtk_table_attach(GTK_TABLE(right_table),  label, 0,1, right_row, right_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);
  }

  for (i_ref=0; i_ref<2; i_ref++) {
    threshold->full_label[i_ref] = gtk_label_new("full");
    gtk_table_attach(GTK_TABLE(right_table),  threshold->full_label[i_ref], 
		     1+2*i_ref,2+2*i_ref, right_row, right_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    /* show/hide taken care of by threshold_update_layout */

    threshold->scaled_label[i_ref] = gtk_label_new("scaled");
    gtk_table_attach(GTK_TABLE(right_table),  threshold->scaled_label[i_ref], 
		     2+2*i_ref,3+2*i_ref, right_row, right_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    /* show/hide taken care of by threshold_update_layout */

  }

  if (layout == AMITK_THRESHOLD_LINEAR_LAYOUT) right_row = 1;
  else right_row++;


  /* the histogram */
  if (!threshold->minimal) {
    threshold->histogram = gnome_canvas_new_aa();
    gtk_table_attach(GTK_TABLE(right_table), threshold->histogram, 0,1,right_row,right_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL, X_PADDING, Y_PADDING);
    gtk_widget_set_size_request(threshold->histogram,
				IMAGE_DISTRIBUTION_WIDTH,
				AMITK_DATA_SET_DISTRIBUTION_SIZE + 2*THRESHOLD_TRIANGLE_HEIGHT);
    gnome_canvas_set_scroll_region(GNOME_CANVAS(threshold->histogram), 
				   0.0, THRESHOLD_TRIANGLE_WIDTH/2.0,
				   IMAGE_DISTRIBUTION_WIDTH,
				   (THRESHOLD_TRIANGLE_WIDTH/2.0 + AMITK_DATA_SET_DISTRIBUTION_SIZE));
    gtk_widget_show(threshold->histogram);
  }



  /* the color scale */
  for (i_ref=0; i_ref<2; i_ref++) {
    threshold->color_scales[i_ref] = gnome_canvas_new_aa();
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

  /* update what's on the histogram */
  threshold_update_histogram(threshold);

  /* make sure the right widgets are shown or hidden */
  threshold_update_layout(threshold);

  gtk_widget_show(left_table);
  gtk_widget_show(right_table);
  gtk_widget_show(main_box);

  /* update what's on the pull down menu */
  gtk_option_menu_set_history(GTK_OPTION_MENU(threshold->color_table_menu),
			      AMITK_DATA_SET_COLOR_TABLE(threshold->data_set));
  return;
}


static void threshold_add_data_set(AmitkThreshold * threshold, AmitkDataSet * ds) {

  gint i;

  g_return_if_fail(threshold->data_set == NULL);

  threshold->data_set = g_object_ref(ds);

  for (i=0; i<2; i++) {
    threshold->threshold_max[i] = AMITK_DATA_SET_THRESHOLD_MAX(ds, i);
    threshold->threshold_min[i] = AMITK_DATA_SET_THRESHOLD_MIN(ds, i);
  }

  g_signal_connect(G_OBJECT(ds), "scale_factor_changed", G_CALLBACK(data_set_changed_cb), threshold);
  g_signal_connect(G_OBJECT(ds), "color_table_changed", G_CALLBACK(data_set_changed_cb), threshold);
  g_signal_connect(G_OBJECT(ds), "thresholding_changed", G_CALLBACK(data_set_changed_cb), threshold);

  return;
}

static void threshold_remove_data_set(AmitkThreshold * threshold) {

  if (threshold->data_set == NULL) return;

  AMITK_OBJECT(threshold->data_set)->dialog = NULL;
  g_signal_handlers_disconnect_by_func(G_OBJECT(threshold->data_set), data_set_changed_cb, threshold);
  g_object_unref(threshold->data_set);
  threshold->data_set = NULL;

  return;
}

/* this gets called when the hbox inside of a menu item is realized */
static void threshold_realize_color_table_menu_item_cb(GtkWidget * pix_box, gpointer data) {

  AmitkColorTable color_table = GPOINTER_TO_INT(data);
  GdkPixbuf * pixbuf;
  GtkWidget * image;

  g_return_if_fail(pix_box != NULL);

  /* make sure we don't get called again */
  if (gtk_container_get_children(GTK_CONTAINER(pix_box)) != NULL) return;

  pixbuf = image_from_colortable(color_table, MENU_COLOR_SCALE_WIDTH, MENU_COLOR_SCALE_HEIGHT,
				 0, MENU_COLOR_SCALE_WIDTH-1, 0, MENU_COLOR_SCALE_WIDTH-1, TRUE);
  image = gtk_image_new_from_pixbuf(pixbuf);
  g_object_unref(pixbuf);

  gtk_box_pack_start(GTK_BOX(pix_box), image, FALSE, FALSE, 0);
  gtk_widget_show(image);

  return;
}

/* this gets called when the hbox inside one of the threshold_type radio buttons is realized */
static void threshold_realize_type_icon_cb(GtkWidget * pix_box, gpointer data) {

  AmitkThresholding thresholding = GPOINTER_TO_INT(data);
  GdkPixbuf * pixbuf;
  GtkWidget * image;

  g_return_if_fail(pix_box != NULL);

  /* make sure we don't get called again */
  if (gtk_container_get_children(GTK_CONTAINER(pix_box)) != NULL) return;

  pixbuf = gdk_pixbuf_new_from_xpm_data(icon_thresholding[thresholding]);
  image = gtk_image_new_from_pixbuf(pixbuf);
  g_object_unref(pixbuf);

  gtk_box_pack_start(GTK_BOX(pix_box), image, FALSE, FALSE, 0);
  gtk_widget_show(image);

  return;
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
    g_warning("Threshold has no style?\n");
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

  if (threshold->minimal) return; /* no spin buttons in minimal configuration */

  scale = (AMITK_DATA_SET_GLOBAL_MAX(threshold->data_set)-
	   AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set));
  if (scale < EPSILON) scale = EPSILON;

  for (i_ref=0; i_ref<threshold->visible_refs; i_ref++) {
    for (i_entry=0; i_entry< AMITK_THRESHOLD_ENTRY_NUM_ENTRIES; i_entry++)
      g_signal_handlers_block_by_func(G_OBJECT(threshold->spin_button[i_ref][i_entry]),
				      G_CALLBACK(threshold_spin_button_cb), threshold);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MAX_PERCENT]),
			      (100*(threshold->threshold_max[i_ref]-
				    AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set))/scale));
    
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MAX_ABSOLUTE]),
			      threshold->threshold_max[i_ref]);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MIN_PERCENT]),
			      (100*(threshold->threshold_min[i_ref]-
				    AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set))/scale));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(threshold->spin_button[i_ref][AMITK_THRESHOLD_ENTRY_MIN_ABSOLUTE]),
			      threshold->threshold_min[i_ref]);

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

  global_diff = 
    AMITK_DATA_SET_GLOBAL_MAX(threshold->data_set)- 
    AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set);
  if (global_diff < EPSILON) global_diff = EPSILON;

  for (i_ref=0; i_ref<threshold->visible_refs; i_ref++) {

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
	  (1-(threshold->threshold_min[i_ref]- AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set))/global_diff);
      top = point;
      bottom = point+THRESHOLD_TRIANGLE_HEIGHT;
      if (threshold->threshold_min[i_ref] < AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set))
	down_pointing=TRUE;
      fill_color = "white";
      break;
      
    case AMITK_THRESHOLD_ARROW_FULL_MAX:
      left = 0;
      right = THRESHOLD_TRIANGLE_WIDTH;
      if (EQUAL_ZERO(global_diff))
	point = THRESHOLD_TRIANGLE_HEIGHT+THRESHOLD_COLOR_SCALE_HEIGHT;
      else
	point = THRESHOLD_TRIANGLE_HEIGHT + 
	  THRESHOLD_COLOR_SCALE_HEIGHT * 
	  (1-(threshold->threshold_max[i_ref]-AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set))/global_diff);
      top = point-THRESHOLD_TRIANGLE_HEIGHT;
      bottom = point;
      if (threshold->threshold_max[i_ref] > AMITK_DATA_SET_GLOBAL_MAX(threshold->data_set)) 
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
      fill_color = "white";
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
    gnome_canvas_points_unref(points);

    /* need to force an update to get smooth action */
    gnome_canvas_update_now(GNOME_CANVAS(threshold->color_scales[i_ref]));
  }
    
  return;
}


/* function called to update the color scales */
static void threshold_update_color_scale(AmitkThreshold * threshold, AmitkThresholdScale scale) {

  gdouble x,y;
  guint i_ref;
  GdkPixbuf * pixbuf;

  for (i_ref=0; i_ref<threshold->visible_refs; i_ref++) {
    
    switch (scale) {
      
    case AMITK_THRESHOLD_SCALE_FULL:
      pixbuf = 
	image_from_colortable(AMITK_DATA_SET_COLOR_TABLE(threshold->data_set),
			      THRESHOLD_COLOR_SCALE_WIDTH,
			      THRESHOLD_COLOR_SCALE_HEIGHT,
			      threshold->threshold_min[i_ref],
			      threshold->threshold_max[i_ref],
			      AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set),
			      AMITK_DATA_SET_GLOBAL_MAX(threshold->data_set),
			      FALSE);
      x = THRESHOLD_TRIANGLE_WIDTH;
      y = THRESHOLD_TRIANGLE_HEIGHT;
      break;
      
    case AMITK_THRESHOLD_SCALE_SCALED:
      pixbuf = 
	image_from_colortable(AMITK_DATA_SET_COLOR_TABLE(threshold->data_set),
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
    AMITK_DATA_SET_GLOBAL_MAX(threshold->data_set)- 
    AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set);
  if (global_diff < EPSILON) global_diff = EPSILON;

  for (i_ref=0; i_ref<threshold->visible_refs; i_ref++) {

    initial_diff = 
      threshold->initial_max[i_ref]-
      threshold->initial_min[i_ref];
    if (initial_diff < EPSILON) initial_diff = EPSILON;

    /* update the lines that connect the max arrows of each color scale */
    line_points = gnome_canvas_points_new(2);
    line_points->coords[0] = THRESHOLD_COLOR_SCALE_WIDTH+THRESHOLD_TRIANGLE_WIDTH;
    temp = (1.0-(threshold->threshold_max[i_ref]-
		 AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set))/global_diff);
    if (temp < 0.0) temp = 0.0;
    line_points->coords[1] = THRESHOLD_TRIANGLE_HEIGHT + THRESHOLD_COLOR_SCALE_HEIGHT * temp;
    line_points->coords[2] = THRESHOLD_COLOR_SCALE_WIDTH+
      THRESHOLD_TRIANGLE_WIDTH+THRESHOLD_COLOR_SCALE_SEPARATION;
    temp = (1.0-(threshold->threshold_max[i_ref]-threshold->initial_min[i_ref])/initial_diff);
    if ((temp < 0.0) || (scale==AMITK_THRESHOLD_SCALE_FULL) || isnan(temp)) temp = 0.0;
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

    /* update the lines that connect the min arrows of each color scale */
    line_points = gnome_canvas_points_new(2);
    line_points->coords[0] = THRESHOLD_COLOR_SCALE_WIDTH+THRESHOLD_TRIANGLE_WIDTH;
    temp = (1.0-(threshold->threshold_min[i_ref]-
		 AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set))/global_diff);
    if (temp > 1.0) temp = 1.0;
    line_points->coords[1] = THRESHOLD_TRIANGLE_HEIGHT + THRESHOLD_COLOR_SCALE_HEIGHT * temp;
    line_points->coords[2] = THRESHOLD_COLOR_SCALE_WIDTH+
      THRESHOLD_TRIANGLE_WIDTH+THRESHOLD_COLOR_SCALE_SEPARATION;
    temp = (1.0-(threshold->threshold_min[i_ref]-threshold->initial_min[i_ref])/initial_diff);
    if ((temp > 1.0) || (scale==AMITK_THRESHOLD_SCALE_FULL) || isnan(temp)) temp = 1.0;
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
  }

  return;
} 


/* update all the color scales */
static void threshold_update_color_scales(AmitkThreshold * threshold) {
  guint i_ref;

  threshold_update_color_scale(threshold, AMITK_THRESHOLD_SCALE_FULL);

  for (i_ref=0; i_ref<threshold->visible_refs; i_ref++) {
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



/* set which toggle button is depressed */
static void threshold_update_type(AmitkThreshold * threshold) {
  
  AmitkThresholding i_thresholding;

  if (threshold->minimal) return; /* no changing of threshold type in minimal setup */

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


/* function called when the max or min triangle is moved 
 * mostly taken from Pennington's fine book */
static gint threshold_arrow_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {

  AmitkThreshold * threshold = data;
  gdouble item_x, item_y;
  gdouble delta;
  amide_data_t temp;
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
	
	switch (which_threshold_arrow) {
	  case AMITK_THRESHOLD_ARROW_FULL_MAX:
	    delta *= (AMITK_DATA_SET_GLOBAL_MAX(threshold->data_set) - 
		      AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set));
	    temp = threshold->initial_max[which_ref] + delta;
	    if (temp < AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set))
	      temp = AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set);
	    if ((temp-EPSILON*fabs(temp)) < threshold->threshold_min[which_ref])
	      temp = threshold->threshold_min[which_ref]+EPSILON*fabs(threshold->threshold_min[which_ref]);
	    threshold->threshold_max[which_ref] = temp;
	    threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_FULL_MAX);
	    threshold_update_connector_lines(threshold, AMITK_THRESHOLD_SCALE_FULL);
	    break;
	  case AMITK_THRESHOLD_ARROW_FULL_MIN:
	    delta *= (AMITK_DATA_SET_GLOBAL_MAX(threshold->data_set) - 
		      AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set));
	    temp = threshold->initial_min[which_ref] + delta;
	    if (temp < AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set)) 
	      temp = AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set);
	    if ((temp+EPSILON*fabs(temp)) > threshold->threshold_max[which_ref]) 
	      temp = threshold->threshold_max[which_ref]-EPSILON*fabs(threshold->threshold_max[which_ref]);
	    if (temp > AMITK_DATA_SET_GLOBAL_MAX(threshold->data_set)) 
	      temp = AMITK_DATA_SET_GLOBAL_MAX(threshold->data_set);
	    threshold->threshold_min[which_ref] = temp;
	    threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_FULL_MIN);
	    threshold_update_connector_lines(threshold, AMITK_THRESHOLD_SCALE_FULL);
	    break;
	  case AMITK_THRESHOLD_ARROW_SCALED_MAX:
	    delta *= (threshold->initial_max[which_ref] - threshold->initial_min[which_ref]);
	    temp = threshold->initial_max[which_ref] + delta;
	    if ((temp-EPSILON*fabs(temp)) < threshold->threshold_min[which_ref]) 
	      temp = threshold->threshold_min[which_ref]+EPSILON*fabs(threshold->threshold_min[which_ref]);
	    if (temp < AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set))
	      temp = AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set);
	    threshold->threshold_max[which_ref] = temp;
	    threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_FULL_MAX);
	    threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_SCALED_MAX);
	    threshold_update_color_scale(threshold, AMITK_THRESHOLD_SCALE_SCALED);
	    threshold_update_connector_lines(threshold, AMITK_THRESHOLD_SCALE_SCALED);
	    break;
	  case AMITK_THRESHOLD_ARROW_SCALED_MIN:
	    delta *= (threshold->initial_max[which_ref] - threshold->initial_min[which_ref]);
	    temp = threshold->initial_min[which_ref] + delta;
	    if (temp < AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set)) 
	      temp = AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set);
	    if (temp >= threshold->threshold_max[which_ref]) 
	      temp = threshold->threshold_max[which_ref];
	    if ((temp+EPSILON*fabs(temp)) > threshold->threshold_max[which_ref]) 
	      temp = threshold->threshold_max[which_ref]-EPSILON*fabs(threshold->threshold_max[which_ref]);
	    if (temp > AMITK_DATA_SET_GLOBAL_MAX(threshold->data_set)) 
	      temp = AMITK_DATA_SET_GLOBAL_MAX(threshold->data_set);
	    threshold->threshold_min[which_ref] = temp;
	    threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_FULL_MIN);
	    threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_SCALED_MIN);
	    threshold_update_color_scale(threshold, AMITK_THRESHOLD_SCALE_SCALED);
	    threshold_update_connector_lines(threshold, AMITK_THRESHOLD_SCALE_SCALED);
	    break;
	  default:
	    break;
	  }

	threshold_update_color_scale(threshold, AMITK_THRESHOLD_SCALE_FULL);
	threshold_update_spin_buttons(threshold);   /* show the current val in the spin button's */
      }
      break;

    case GDK_BUTTON_RELEASE:
      switch (which_threshold_arrow) {
      case AMITK_THRESHOLD_ARROW_FULL_MAX:
	amitk_data_set_set_threshold_max(threshold->data_set, which_ref,
					 threshold->threshold_max[which_ref]);
	break;
      case AMITK_THRESHOLD_ARROW_FULL_MIN:
	amitk_data_set_set_threshold_min(threshold->data_set, which_ref,
					 threshold->threshold_min[which_ref]);
	break;
      case AMITK_THRESHOLD_ARROW_SCALED_MAX:
	amitk_data_set_set_threshold_max(threshold->data_set, which_ref,
					 threshold->threshold_max[which_ref]);
	break;
      case AMITK_THRESHOLD_ARROW_SCALED_MIN:
	amitk_data_set_set_threshold_min(threshold->data_set, which_ref,
					 threshold->threshold_min[which_ref]);
	break;
      default:
	break;
      }

      gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);

      /* reset the scaled color scale */
      threshold->initial_max[which_ref] = threshold->threshold_max[which_ref];
      threshold->initial_min[which_ref] = threshold->threshold_min[which_ref];
      
      threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_SCALED_MAX);
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

  /* figure out which color table menu item called me */
  i_color_table = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));

  /* check if we actually changed values */
  if (AMITK_DATA_SET_COLOR_TABLE(threshold->data_set) != i_color_table) {
    amitk_data_set_set_color_table(AMITK_DATA_SET(threshold->data_set), i_color_table);
  }
  
  return;
}

static void data_set_changed_cb(AmitkDataSet * ds, AmitkThreshold * threshold) {

  gint i;

  g_return_if_fail(AMITK_IS_DATA_SET(ds));
  g_return_if_fail(AMITK_IS_THRESHOLD(threshold));

  for (i=0; i<2; i++) {
    threshold->threshold_max[i] = AMITK_DATA_SET_THRESHOLD_MAX(ds, i);
    threshold->threshold_min[i] = AMITK_DATA_SET_THRESHOLD_MIN(ds, i);
  }
  threshold_update(threshold);

  return;
}

/* function to update the reference frames */
static void threshold_ref_frame_cb(GtkWidget * widget, gpointer data) {

  guint which_ref,frame;
  AmitkThreshold * threshold = data;

  /* figure out which menu item called me */
  which_ref = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"which_ref"));
  frame = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"frame"));

  /* check if we actually changed values */
  if (frame != AMITK_DATA_SET_THRESHOLD_REF_FRAME(threshold->data_set, which_ref)) {
    
    if ((which_ref==0) && (frame > AMITK_DATA_SET_THRESHOLD_REF_FRAME(threshold->data_set,1)))
      frame =AMITK_DATA_SET_THRESHOLD_REF_FRAME(threshold->data_set,1);
    else if ((which_ref==1) && (frame < AMITK_DATA_SET_THRESHOLD_REF_FRAME(threshold->data_set,0)))
      frame = AMITK_DATA_SET_THRESHOLD_REF_FRAME(threshold->data_set,0);
    gtk_option_menu_set_history(GTK_OPTION_MENU(threshold->threshold_ref_frame_menu[which_ref]), frame);

    /* and inact the changes */
    amitk_data_set_set_threshold_ref_frame(threshold->data_set, which_ref, frame);

  }
  
  return;
}


static void threshold_spin_button_cb(GtkWidget* widget, gpointer data) {

  AmitkThreshold * threshold = data;
  gdouble temp;
  AmitkThresholdEntry which_threshold_entry;
  gboolean update = FALSE;
  guint which_ref;

  /* figure out which of the arrows triggered the callback */
  which_threshold_entry = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "type"));
  which_ref = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "which_ref"));

  temp = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
 
  /* make sure it's a valid floating point */
  switch (which_threshold_entry) {
    case AMITK_THRESHOLD_ENTRY_MAX_PERCENT:
      temp = 
	(AMITK_DATA_SET_GLOBAL_MAX(threshold->data_set)-
	 AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set)) * temp/100.0+
	AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set);
    case AMITK_THRESHOLD_ENTRY_MAX_ABSOLUTE:
      if ((temp > threshold->threshold_min[which_ref]) && 
	  (temp > AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set))) {
	amitk_data_set_set_threshold_max(threshold->data_set, which_ref, temp);
	threshold->threshold_max[which_ref] = temp;
	update = TRUE;
      }
      break;
    case AMITK_THRESHOLD_ENTRY_MIN_PERCENT:
      temp = 
	((AMITK_DATA_SET_GLOBAL_MAX(threshold->data_set)-
	  AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set)) * temp/100.0)+
	AMITK_DATA_SET_GLOBAL_MIN(threshold->data_set);
    case AMITK_THRESHOLD_ENTRY_MIN_ABSOLUTE:
      if ((temp < AMITK_DATA_SET_GLOBAL_MAX(threshold->data_set)) && 
	  (temp < threshold->threshold_max[which_ref])) {
	amitk_data_set_set_threshold_min(threshold->data_set, which_ref, temp);
	threshold->threshold_min[which_ref] = temp;
	update = TRUE;
      }
    default:
      break;
  }

  /* need to manually twiddle the spin button's, rejected values of 
     amitk_data_set_set_threshold_whatever won't elicit a signal */
  threshold_update_spin_buttons(threshold);

  return;
}


static void thresholding_cb(GtkWidget * widget, gpointer data) {

  AmitkThreshold * threshold = data;
  AmitkThresholding thresholding;

  /* figure out which menu item called me */
  thresholding = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget),"thresholding"));

  /* check if we actually changed values */
  if (AMITK_DATA_SET_THRESHOLDING(threshold->data_set) != thresholding) {
    /* and inact the changes */
    amitk_data_set_set_thresholding(threshold->data_set, thresholding);
    if (thresholding == AMITK_THRESHOLDING_INTERPOLATE_FRAMES) 
      threshold->visible_refs=2;
    else 
      threshold->visible_refs=1;

    threshold_update(threshold);
  }
  
  return;
}

/* function to tell the threshold widget to redraw (i.e. the data set's info's changes */
static void threshold_update(AmitkThreshold * threshold) {

  g_return_if_fail(threshold != NULL);

  /* update what's on the pull down menu */
  gtk_option_menu_set_history(GTK_OPTION_MENU(threshold->color_table_menu), 
			      AMITK_DATA_SET_COLOR_TABLE(threshold->data_set));

  threshold_update_layout(threshold);
  threshold_update_spin_buttons(threshold);
  threshold_update_color_scales(threshold);
  threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_FULL_MIN);
  threshold_update_arrow(threshold, AMITK_THRESHOLD_ARROW_FULL_MAX);
  threshold_update_type(threshold);

  return;

}


/* function to load a new data set into the widget */
void amitk_threshold_new_data_set(AmitkThreshold * threshold, AmitkDataSet * new_data_set) {


  g_return_if_fail(AMITK_IS_THRESHOLD(threshold));
  g_return_if_fail(AMITK_IS_DATA_SET(new_data_set));

  threshold_remove_data_set(threshold);
  threshold_add_data_set(threshold, new_data_set);

  threshold_update(threshold);
  threshold_update_histogram(threshold);

  return;
}


GtkWidget* amitk_threshold_new (AmitkDataSet * ds,
				AmitkThresholdLayout layout,
				GtkWindow * parent,
				gboolean minimal) {

  AmitkThreshold * threshold;
  AmitkThresholdArrow i_arrow;

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), NULL);

  threshold = g_object_new (amitk_threshold_get_type (), NULL);
  threshold_add_data_set(threshold, ds);

  if (AMITK_DATA_SET_THRESHOLDING(threshold->data_set) == 
      AMITK_THRESHOLDING_INTERPOLATE_FRAMES) 
    threshold->visible_refs=2;
  else 
    threshold->visible_refs=1;
  threshold->minimal = minimal;

  threshold->progress_dialog = amitk_progress_dialog_new(parent);
  threshold_construct(threshold, layout);

  threshold_update_spin_buttons(threshold); /* update what's in the spin button widgets */
  threshold_update_color_scales(threshold);
  for (i_arrow=0;i_arrow< AMITK_THRESHOLD_ARROW_NUM_ARROWS;i_arrow++)
    threshold_update_arrow(threshold, i_arrow);

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
  GtkObjectClass *gtkobject_class;

  gtkobject_class = (GtkObjectClass*) klass;

  threshold_dialog_parent_class = g_type_class_peek_parent(klass);

}

static void threshold_dialog_init (AmitkThresholdDialog * dialog)
{

  gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);

  dialog->vbox = gtk_vbox_new (FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 10);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), dialog->vbox);
  gtk_widget_show (dialog->vbox);

  /* make the data set title thingie */
  dialog->data_set_label = gtk_label_new(NULL);
  gtk_box_pack_start(GTK_BOX(dialog->vbox), dialog->data_set_label, FALSE, FALSE,0);
  gtk_widget_show(dialog->data_set_label);

  /* hookup a callback to load in a pixmap icon when realized */
  g_signal_connect(G_OBJECT(dialog), "realize", G_CALLBACK(threshold_dialog_realize_cb), NULL);

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
  temp_string = g_strdup_printf("data set: %s\n",AMITK_OBJECT_NAME(data_set));
  gtk_label_set_text (GTK_LABEL(dialog->data_set_label), temp_string);
  g_free(temp_string);

  return;
}


/* this gets called when the threshold is realized */
static void threshold_dialog_realize_cb(GtkWidget * dialog, gpointer data) {

  GdkPixbuf * pixbuf;

  g_return_if_fail(GTK_IS_WINDOW(dialog));

  /* setup the threshold's icon */
  pixbuf = gdk_pixbuf_new_from_xpm_data(icon_threshold_xpm);
  gtk_window_set_icon(GTK_WINDOW(dialog), pixbuf);
  g_object_unref(pixbuf);

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
  gtk_window_set_title (GTK_WINDOW (dialog), "Threshold Dialog");
  gtk_window_set_transient_for(GTK_WINDOW (dialog), parent);
  gtk_window_set_destroy_with_parent(GTK_WINDOW (dialog), TRUE);

  return GTK_WIDGET (dialog);
}


/* function to load a new data set into the widget */
void amitk_threshold_dialog_new_data_set(AmitkThresholdDialog * dialog, AmitkDataSet * new_data_set) {

  gchar * temp_string;

  g_return_if_fail(AMITK_IS_THRESHOLD_DIALOG(dialog));
  g_return_if_fail(AMITK_IS_DATA_SET(new_data_set));

  amitk_threshold_new_data_set(AMITK_THRESHOLD(dialog->threshold), new_data_set);

  /* reset the title */
  temp_string = g_strdup_printf("data set: %s\n",AMITK_OBJECT_NAME(new_data_set));
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
  GtkObjectClass *gtkobject_class;

  gtkobject_class = (GtkObjectClass*) klass;

  thresholds_dialog_parent_class = g_type_class_peek_parent(klass);

}


static void thresholds_dialog_init (AmitkThresholdsDialog * dialog)
{
  GtkWidget * main_vbox;

  dialog->thresholds = NULL;

  gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
  main_vbox = gtk_vbox_new (FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 10);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  dialog->notebook = gtk_notebook_new();
  gtk_container_add (GTK_CONTAINER (main_vbox), dialog->notebook);
  gtk_widget_show (dialog->notebook);

  /* hookup a callback to load in a pixmap icon when realized */
  g_signal_connect(G_OBJECT(dialog), "realize", G_CALLBACK(threshold_dialog_realize_cb), NULL);
}

/* this gets called after we have a list of data set */
static void thresholds_dialog_construct(AmitkThresholdsDialog * dialog, 
					GtkWindow * parent,
					GList * objects) {

  GtkWidget * threshold;
  GtkWidget * label;

  while (objects != NULL) {
    if (AMITK_IS_DATA_SET(objects->data)) {
      threshold = amitk_threshold_new(objects->data, AMITK_THRESHOLD_LINEAR_LAYOUT, 
				     parent, FALSE);
      dialog->thresholds = g_list_append(dialog->thresholds, threshold);

      label = gtk_label_new(AMITK_OBJECT_NAME(objects->data));

      gtk_notebook_append_page(GTK_NOTEBOOK(dialog->notebook), threshold, label);
      
      gtk_widget_show(threshold);
      gtk_widget_show(label);

    }
    objects = objects->next;
  }

  return;
}



GtkWidget* amitk_thresholds_dialog_new (GList * objects, GtkWindow * parent) {
  AmitkThresholdsDialog *dialog;

  g_return_val_if_fail(objects != NULL, NULL);

  dialog = g_object_new(AMITK_TYPE_THRESHOLDS_DIALOG, NULL);
  thresholds_dialog_construct(dialog, parent, objects);
  gtk_window_set_title (GTK_WINDOW (dialog), "Threshold Dialog");

  gtk_window_set_transient_for(GTK_WINDOW (dialog), parent);
  gtk_window_set_destroy_with_parent(GTK_WINDOW (dialog), TRUE);


  return GTK_WIDGET (dialog);
}

