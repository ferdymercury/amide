/* amitk_threshold.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2002 Andy Loening
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

/* adapted from gtkcolorsel.c */


#include "config.h"
#include "amitk_threshold.h"
#include "image.h"
#include <math.h>
#include <gdk-pixbuf/gnome-canvas-pixbuf.h> 
#include "../pixmaps/icon_threshold.xpm"
#include "../pixmaps/icon_threshold_per_slice.xpm"
#include "../pixmaps/icon_threshold_per_frame.xpm"
#include "../pixmaps/icon_threshold_interpolate_frames.xpm"
#include "../pixmaps/icon_threshold_global.xpm"

#define THRESHOLD_DEFAULT_ENTRY_WIDTH 75.0
#define THRESHOLD_COLOR_SCALE_SEPARATION 30.0
#define THRESHOLD_COLOR_SCALE_WIDTH 16.0
#define THRESHOLD_COLOR_SCALES_WIDTH (2.0*THRESHOLD_COLOR_SCALE_WIDTH+THRESHOLD_COLOR_SCALE_SEPARATION)
#define THRESHOLD_COLOR_SCALE_HEIGHT (gdouble) VOLUME_DISTRIBUTION_SIZE
#define THRESHOLD_HISTOGRAM_WIDTH (gdouble) IMAGE_DISTRIBUTION_WIDTH
#define THRESHOLD_TRIANGLE_WIDTH 16.0
#define THRESHOLD_TRIANGLE_HEIGHT 12.0

#define MENU_COLOR_SCALE_HEIGHT 8
#define MENU_COLOR_SCALE_WIDTH 30


enum {
  THRESHOLD_CHANGED,
  COLOR_CHANGED,
  LAST_SIGNAL
} amitk_threshold_signals;

static void threshold_class_init (AmitkThresholdClass *klass);
static void threshold_init (AmitkThreshold *threshold);
static void threshold_destroy (GtkObject *object);
static void threshold_show_all (GtkWidget * widget);
static void threshold_construct(AmitkThreshold * threshold, amitk_threshold_layout_t layout);
static void threshold_realize_color_table_menu_item_cb(GtkWidget * hbox, gpointer data);
static void threshold_realize_type_icon_cb(GtkWidget * hbox, gpointer data);
static void threshold_update_histogram(AmitkThreshold * threshold);
static void threshold_update_entries(AmitkThreshold * threshold);
static void threshold_update_arrow(AmitkThreshold * threshold, which_threshold_arrow_t arrow);
static void threshold_update_color_scale(AmitkThreshold * threshold, which_threshold_scale_t scale);
static void threshold_update_connector_lines(AmitkThreshold * threshold, which_threshold_scale_t scale);
static void threshold_update_color_scales(AmitkThreshold * threshold);
static void threshold_update_layout(AmitkThreshold * threshold);
static void threshold_update_type(AmitkThreshold * threshold);
static gint threshold_arrow_cb(GtkWidget* widget, GdkEvent * event, gpointer data);
static void threshold_color_table_cb(GtkWidget * widget, gpointer data);
static void threshold_ref_frame_cb(GtkWidget* widget, gpointer data);
static void threshold_entry_cb(GtkWidget* widget, gpointer data);
static void threshold_type_cb(GtkWidget * widget, gpointer data);

static void threshold_dialog_class_init (AmitkThresholdDialogClass *klass);
static void threshold_dialog_init (AmitkThresholdDialog *threshold_dialog);
static void threshold_dialog_construct(AmitkThresholdDialog * dialog, volume_t * volume);
static void threshold_dialog_realize_cb(GtkWidget * dialog, gpointer data);
static void threshold_dialog_threshold_changed_cb(GtkWidget * threshold, gpointer dialog);
static void threshold_dialog_color_changed_cb(GtkWidget * threshold, gpointer dialog);

static void thresholds_dialog_class_init (AmitkThresholdsDialogClass *klass);
static void thresholds_dialog_init (AmitkThresholdsDialog *thresholds_dialog);
static void thresholds_dialog_construct(AmitkThresholdsDialog * thresholds_dialog, volumes_t * volumes);
static void thresholds_dialog_threshold_changed_cb(GtkWidget * threshold, gpointer thresholds_dialog);
static void thresholds_dialog_color_changed_cb(GtkWidget * threshold, gpointer thresholds_dialog);

static GtkVBox *threshold_parent_class = NULL;
static GtkWindowClass *threshold_dialog_parent_class = NULL;
static GtkWindowClass *thresholds_dialog_parent_class = NULL;
static guint threshold_signals[LAST_SIGNAL] = {0};
static guint threshold_dialog_signals[LAST_SIGNAL] = {0};
static guint thresholds_dialog_signals[LAST_SIGNAL] = {0};

static GdkCursor * threshold_cursor = NULL;

GtkType amitk_threshold_get_type (void) {

  static GtkType threshold_type = 0;

  if (!threshold_type)
    {
      static const GtkTypeInfo threshold_info =
      {
	"AmitkThreshold",
	sizeof (AmitkThreshold),
	sizeof (AmitkThresholdClass),
	(GtkClassInitFunc) threshold_class_init,
	(GtkObjectInitFunc) threshold_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      threshold_type = gtk_type_unique (GTK_TYPE_VBOX, &threshold_info);
    }

  return threshold_type;
}

static void threshold_class_init (AmitkThresholdClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) klass;
  widget_class = (GtkWidgetClass*) klass;

  threshold_parent_class = gtk_type_class (GTK_TYPE_VBOX);

  threshold_signals[THRESHOLD_CHANGED] =
    gtk_signal_new ("threshold_changed",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (AmitkThresholdClass, threshold_changed),
		    gtk_marshal_NONE__NONE, GTK_TYPE_NONE, 0);
  threshold_signals[COLOR_CHANGED] =
    gtk_signal_new ("color_changed",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (AmitkThresholdClass, color_changed),
		    gtk_marshal_NONE__NONE, GTK_TYPE_NONE, 0);
  gtk_object_class_add_signals (object_class, threshold_signals, LAST_SIGNAL);

  object_class->destroy = threshold_destroy;
  widget_class->show_all = threshold_show_all;

}

static void threshold_init (AmitkThreshold *threshold)
{

  which_threshold_scale_t i_scale;
  which_threshold_line_t i_line;
  guint i_ref;



  /* initialize some critical stuff */
  for (i_ref=0; i_ref<2; i_ref++) {
    for (i_scale=0;i_scale<NUM_THRESHOLD_SCALES;i_scale++) {
      threshold->color_scale_image[i_ref][i_scale] = NULL;
      threshold->color_scale_rgb[i_ref][i_scale] = NULL;
    }
    for (i_line=0;i_line<NUM_THRESHOLD_LINES; i_line++)
      threshold->connector_line[i_ref][i_line] = NULL;
  }
  threshold->histogram_image = NULL;
  threshold->histogram_rgb = NULL;

  if (threshold_cursor == NULL)
    threshold_cursor = gdk_cursor_new(GDK_SB_V_DOUBLE_ARROW);

}
static void threshold_destroy (GtkObject * object) {

  AmitkThreshold * threshold;
  which_threshold_scale_t i_scale;
  uint i_ref;

  g_return_if_fail (object != NULL);
  g_return_if_fail (AMITK_IS_THRESHOLD (object));

  threshold =  AMITK_THRESHOLD (object);

  threshold->volume = volume_unref(threshold->volume);
  for (i_ref=0; i_ref<2; i_ref++)
    for (i_scale=0;i_scale<NUM_THRESHOLD_SCALES;i_scale++)
      if (threshold->color_scale_rgb[i_ref][i_scale] != NULL)
	gdk_pixbuf_unref(threshold->color_scale_rgb[i_ref][i_scale]);
  if (threshold->histogram_rgb != NULL)
    gdk_pixbuf_unref(threshold->histogram_rgb);

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

/* this gets called after we have a volume */
static void threshold_construct(AmitkThreshold * threshold, 
				amitk_threshold_layout_t layout) {

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
  color_table_t i_color_table;
  threshold_t i_threshold_type;
  guint i_ref,i_frame;
  gchar * temp_string;
  which_threshold_entry_t i_entry;
  GtkTooltips * radio_button_tips;

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

  for (i_ref=0; i_ref<2; i_ref++) {
    for (i_entry=0; i_entry<NUM_THRESHOLD_ENTRIES; i_entry++) {
      threshold->entry[i_ref][i_entry] = gtk_entry_new();
      gtk_widget_set_usize(threshold->entry[i_ref][i_entry], THRESHOLD_DEFAULT_ENTRY_WIDTH,0);
      gtk_object_set_data(GTK_OBJECT(threshold->entry[i_ref][i_entry]), "type", GINT_TO_POINTER(i_entry));
      gtk_object_set_data(GTK_OBJECT(threshold->entry[i_ref][i_entry]), "which_ref", GINT_TO_POINTER(i_ref));
      gtk_signal_connect(GTK_OBJECT(threshold->entry[i_ref][i_entry]), "activate",  
			 GTK_SIGNAL_FUNC(threshold_entry_cb), threshold);
    }
    gtk_table_attach(GTK_TABLE(left_table),  threshold->entry[i_ref][MAX_PERCENT], 1+2*i_ref,2+2*i_ref,
		     left_row,left_row+1, X_PACKING_OPTIONS | GTK_FILL,  0,  X_PADDING, Y_PADDING);
    gtk_table_attach(GTK_TABLE(left_table), threshold->entry[i_ref][MAX_ABSOLUTE], 2+2*i_ref,3+2*i_ref,
		     left_row,left_row+1,X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_table_attach(GTK_TABLE(left_table), threshold->entry[i_ref][MIN_PERCENT], 1+2*i_ref,2+2*i_ref, 
		     left_row+1,left_row+2, X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_table_attach(GTK_TABLE(left_table), threshold->entry[i_ref][MIN_ABSOLUTE], 2+2*i_ref,3+2*i_ref, 
		     left_row+1, left_row+2, X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    /* show/hide taken care of by threshold_update_layout */
  }
  left_row+=2;

  /* color table selector */
  label = gtk_label_new("color table:");
  gtk_table_attach(GTK_TABLE(left_table), label, 0,1, left_row,left_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(label);

  option_menu = gtk_option_menu_new();
  menu = gtk_menu_new();

  for(i_color_table=0; i_color_table<NUM_COLOR_TABLES; i_color_table++) {
    menuitem = gtk_menu_item_new();
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(menuitem), hbox);

    pix_box = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), pix_box, FALSE, FALSE, 3);
    gtk_widget_show(pix_box);
    gtk_signal_connect(GTK_OBJECT(pix_box), "realize", 
		       threshold_realize_color_table_menu_item_cb, GINT_TO_POINTER(i_color_table));

    label = gtk_label_new(color_table_names[i_color_table]);
    gtk_box_pack_end(GTK_BOX(hbox), label, FALSE, FALSE, 3);
    gtk_widget_show(label);

    gtk_menu_append(GTK_MENU(menu), menuitem);
    gtk_object_set_data(GTK_OBJECT(menuitem), "color_table", GINT_TO_POINTER(i_color_table));
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
    		       GTK_SIGNAL_FUNC(threshold_color_table_cb), threshold);
  }

  gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
  gtk_widget_set_usize(option_menu, 185, -1);
  gtk_table_attach(GTK_TABLE(left_table), option_menu, 1,5, left_row,left_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show_all(option_menu);
  threshold->color_table_menu = option_menu;

  left_row++;


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

  for (i_threshold_type = 0; i_threshold_type < NUM_THRESHOLD_TYPES; i_threshold_type++) {

    threshold->type_button[i_threshold_type] = gtk_radio_button_new(NULL);
    pix_box = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(threshold->type_button[i_threshold_type]), pix_box);
    gtk_widget_show(pix_box);
    gtk_signal_connect(GTK_OBJECT(pix_box), "realize", 
		       threshold_realize_type_icon_cb, GINT_TO_POINTER(i_threshold_type));

    gtk_box_pack_start(GTK_BOX(hbox), threshold->type_button[i_threshold_type], FALSE, FALSE, 3);
    gtk_widget_show(threshold->type_button[i_threshold_type]);
    gtk_tooltips_set_tip(radio_button_tips, threshold->type_button[i_threshold_type], 
			 threshold_type_names[i_threshold_type],
			 threshold_type_explanations[i_threshold_type]);

    gtk_object_set_data(GTK_OBJECT(threshold->type_button[i_threshold_type]), "threshold_type", 
			GINT_TO_POINTER(i_threshold_type));

    if (i_threshold_type != 0)
      gtk_radio_button_set_group(GTK_RADIO_BUTTON(threshold->type_button[i_threshold_type]), 
				 gtk_radio_button_group(GTK_RADIO_BUTTON(threshold->type_button[0])));

    gtk_signal_connect(GTK_OBJECT(threshold->type_button[i_threshold_type]), "clicked",  
		       GTK_SIGNAL_FUNC(threshold_type_cb), threshold);
  }
  threshold_update_type(threshold);




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

    for (i_frame=0; i_frame<threshold->volume->data_set->dim.t; i_frame++) {
      temp_string = g_strdup_printf("%d",i_frame);
      menuitem = gtk_menu_item_new_with_label(temp_string);
      g_free(temp_string);
      
      gtk_menu_append(GTK_MENU(menu), menuitem);
      gtk_object_set_data(GTK_OBJECT(menuitem), "frame", GINT_TO_POINTER(i_frame));
      gtk_object_set_data(GTK_OBJECT(menuitem), "which_ref", GINT_TO_POINTER(i_ref));
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate", 
      			 GTK_SIGNAL_FUNC(threshold_ref_frame_cb), threshold);
    }

    gtk_option_menu_set_menu(GTK_OPTION_MENU(threshold->threshold_ref_frame_menu[i_ref]), menu);
    gtk_table_attach(GTK_TABLE(right_table), threshold->threshold_ref_frame_menu[i_ref],
		     2+i_ref*2,3+i_ref*2, right_row,right_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_set_usize(threshold->threshold_ref_frame_menu[i_ref], 50, -1);
    /* show/hide taken care of by threshold_update_layout */
    gtk_option_menu_set_history(GTK_OPTION_MENU(threshold->threshold_ref_frame_menu[i_ref]), 
				threshold->volume->threshold_ref_frame[i_ref]);
  }


  /* puts these labels on bottom for a linear layout */
  if (layout == AMITK_THRESHOLD_LINEAR_LAYOUT) right_row = 3;
  else right_row++;

  label = gtk_label_new("distribution");
  gtk_table_attach(GTK_TABLE(right_table),  label, 0,1, right_row, right_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(label);

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


  /* the canvas */
  threshold->histogram = gnome_canvas_new_aa();
  gtk_table_attach(GTK_TABLE(right_table), threshold->histogram, 0,1,right_row,right_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL, X_PADDING, Y_PADDING);
  gtk_widget_set_usize(threshold->histogram,
		       IMAGE_DISTRIBUTION_WIDTH,
		       VOLUME_DISTRIBUTION_SIZE + 2*THRESHOLD_TRIANGLE_HEIGHT);
  gnome_canvas_set_scroll_region(GNOME_CANVAS(threshold->histogram), 
				 0.0, THRESHOLD_TRIANGLE_WIDTH/2.0,
				 IMAGE_DISTRIBUTION_WIDTH,
				 (THRESHOLD_TRIANGLE_WIDTH/2.0 + VOLUME_DISTRIBUTION_SIZE));
  gtk_widget_show(threshold->histogram);



  /* the color scale */
  for (i_ref=0; i_ref<2; i_ref++) {
    threshold->color_scales[i_ref] = gnome_canvas_new_aa();
    gtk_table_attach(GTK_TABLE(right_table),  threshold->color_scales[i_ref], 
		     1+2*i_ref,3+2*i_ref,right_row,right_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL, X_PADDING, Y_PADDING);
    gnome_canvas_set_scroll_region(GNOME_CANVAS(threshold->color_scales[i_ref]), 0.0, 0.0,
				   THRESHOLD_COLOR_SCALES_WIDTH+2* THRESHOLD_TRIANGLE_WIDTH,
				   THRESHOLD_COLOR_SCALE_HEIGHT + 2*THRESHOLD_TRIANGLE_HEIGHT + 1);
    gtk_widget_set_usize(threshold->color_scales[i_ref],
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
			      threshold->volume->color_table);
  return;
}


/* this gets called when the hbox inside of a menu item is realized */
static void threshold_realize_color_table_menu_item_cb(GtkWidget * pix_box, gpointer data) {

  color_table_t color_table = GPOINTER_TO_INT(data);
  GdkPixbuf * pixbuf;
  GdkPixmap * pm;
  GdkBitmap * bm;
  GtkWidget * pixmap;

  g_return_if_fail(pix_box != NULL);

  /* make sure we don't get called again */
  if (gtk_container_children(GTK_CONTAINER(pix_box)) != NULL) return;

  pixbuf = image_from_colortable(color_table, MENU_COLOR_SCALE_WIDTH, MENU_COLOR_SCALE_HEIGHT,
				 0, MENU_COLOR_SCALE_WIDTH-1, 0, MENU_COLOR_SCALE_WIDTH-1, TRUE);
  gdk_pixbuf_render_pixmap_and_mask(pixbuf, &pm, &bm, 0);
  gdk_pixbuf_unref(pixbuf);

  pixmap = gtk_pixmap_new(pm, bm);
  gtk_box_pack_start(GTK_BOX(pix_box), pixmap, FALSE, FALSE, 0);
  gtk_widget_show(pixmap);

  return;
}

/* this gets called when the hbox inside one of the threshold_type radio buttons is realized */
static void threshold_realize_type_icon_cb(GtkWidget * pix_box, gpointer data) {

  threshold_t threshold_type = GPOINTER_TO_INT(data);
  GdkPixmap * pm;
  GdkBitmap * bm;
  GtkWidget * pixmap;
  gchar ** icon_threshold_type[NUM_THRESHOLD_TYPES] = {
    icon_threshold_per_slice_xpm,
    icon_threshold_per_frame_xpm,
    icon_threshold_interpolate_frames_xpm,
    icon_threshold_global_xpm};

  g_return_if_fail(pix_box != NULL);

  /* make sure we don't get called again */
  if (gtk_container_children(GTK_CONTAINER(pix_box)) != NULL) return;

  pm = gdk_pixmap_create_from_xpm_d(gtk_widget_get_parent_window(pix_box), &bm,NULL,
				    icon_threshold_type[threshold_type]);
  pixmap = gtk_pixmap_new(pm, bm);
  gtk_box_pack_start(GTK_BOX(pix_box), pixmap, FALSE, FALSE, 0);
  gtk_widget_show(pixmap);

  return;
}


/* refresh what's on the histogram */
static void threshold_update_histogram(AmitkThreshold * threshold) {

  rgb_t fg, bg;
  GtkStyle * widget_style;

  /* figure out what colors to use for the distribution image */
  widget_style = gtk_widget_get_style(GTK_WIDGET(threshold));
  if (widget_style == NULL) {
    g_warning("Threshold has no style?\n");
    widget_style = gtk_style_new();
  }

  bg.r = widget_style->bg[GTK_STATE_NORMAL].red >> 8;
  bg.g = widget_style->bg[GTK_STATE_NORMAL].green >> 8;
  bg.b = widget_style->bg[GTK_STATE_NORMAL].blue >> 8;

  fg.r = widget_style->fg[GTK_STATE_NORMAL].red >> 8;
  fg.g = widget_style->fg[GTK_STATE_NORMAL].green >> 8;
  fg.b = widget_style->fg[GTK_STATE_NORMAL].blue >> 8;

  /* get the distribution image */
  if (threshold->histogram_rgb != NULL)
    gdk_pixbuf_unref(threshold->histogram_rgb);
  threshold->histogram_rgb = image_of_distribution(threshold->volume, fg, bg);

  if (threshold->histogram_image != NULL)
    gnome_canvas_item_set(threshold->histogram_image, "pixbuf", threshold->histogram_rgb, NULL);
  else
    threshold->histogram_image = 
      gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(threshold->histogram)),
			    gnome_canvas_pixbuf_get_type(),
			    "pixbuf", threshold->histogram_rgb,
			    "x", 0.0, "y", ((gdouble) THRESHOLD_TRIANGLE_HEIGHT),  NULL);
  return;
}

/* function to update the entry widgets */
static void threshold_update_entries(AmitkThreshold * threshold) {

  gchar * string;
  guint i_ref;
  amide_data_t min;

  for (i_ref=0; i_ref<threshold->visible_refs; i_ref++) {
    string = g_strdup_printf("%5.2f",(100*(threshold->volume->threshold_max[i_ref]-threshold->volume->global_min)/
				      (threshold->volume->global_max- threshold->volume->global_min)));
    gtk_entry_set_text(GTK_ENTRY(threshold->entry[i_ref][MAX_PERCENT]), string);
    g_free(string);

    string = g_strdup_printf("%5.3f",threshold->volume->threshold_max[i_ref]);
    gtk_entry_set_text(GTK_ENTRY(threshold->entry[i_ref][MAX_ABSOLUTE]), string);
    g_free(string);

    /* for some reason, some versions of glib fuck up when using g_strdup_printf with very small floating points */
    if (fabs(threshold->volume->threshold_min[i_ref])<CLOSE)
      min = CLOSE;
    else
      min = threshold->volume->threshold_min[i_ref];

    string = g_strdup_printf("%5.2f",(100*(min-threshold->volume->global_min)/ 
				      (threshold->volume->global_max - threshold->volume->global_min)));
    gtk_entry_set_text(GTK_ENTRY(threshold->entry[i_ref][MIN_PERCENT]), string);
    g_free(string);

    string = g_strdup_printf("%5.3f", min);
    gtk_entry_set_text(GTK_ENTRY(threshold->entry[i_ref][MIN_ABSOLUTE]), string);
    g_free(string);
  }
  return;
}


static void threshold_update_arrow(AmitkThreshold * threshold, which_threshold_arrow_t arrow) {

  GnomeCanvasPoints * points;
  gdouble left, right, point, top, bottom;
  gboolean up_pointing=FALSE;
  gboolean down_pointing=FALSE;
  gchar * fill_color;
  guint i_ref;

  for (i_ref=0; i_ref<threshold->visible_refs; i_ref++) {

    points = gnome_canvas_points_new(3);

    switch (arrow) {
      
    case THRESHOLD_FULL_MIN_ARROW:
      left = 0;
      right = THRESHOLD_TRIANGLE_WIDTH;
      point = THRESHOLD_TRIANGLE_HEIGHT + 
	THRESHOLD_COLOR_SCALE_HEIGHT * 
	(1-(threshold->volume->threshold_min[i_ref]-threshold->volume->global_min)/
	 (threshold->volume->global_max-threshold->volume->global_min));
      top = point;
      bottom = point+THRESHOLD_TRIANGLE_HEIGHT;
      if (threshold->volume->threshold_min[i_ref] < threshold->volume->global_min)
	down_pointing=TRUE;
      fill_color = "white";
      break;
      
    case THRESHOLD_FULL_MAX_ARROW:
      left = 0;
      right = THRESHOLD_TRIANGLE_WIDTH;
      point = THRESHOLD_TRIANGLE_HEIGHT + 
	THRESHOLD_COLOR_SCALE_HEIGHT * 
	(1-(threshold->volume->threshold_max[i_ref]-threshold->volume->global_min)/
	 (threshold->volume->global_max-threshold->volume->global_min));
      top = point-THRESHOLD_TRIANGLE_HEIGHT;
      bottom = point;
      if (threshold->volume->threshold_max[i_ref] > threshold->volume->global_max) 
	up_pointing=TRUE; /* want upward pointing max arrow */
      fill_color = "black";
      break;
      
    case THRESHOLD_SCALED_MIN_ARROW:
      left = THRESHOLD_COLOR_SCALES_WIDTH+2*THRESHOLD_TRIANGLE_WIDTH;
      right = THRESHOLD_COLOR_SCALES_WIDTH+THRESHOLD_TRIANGLE_WIDTH;
      point = THRESHOLD_TRIANGLE_HEIGHT + 
	THRESHOLD_COLOR_SCALE_HEIGHT * 
	(1-(threshold->volume->threshold_min[i_ref]-threshold->initial_min[i_ref])/
	 (threshold->initial_max[i_ref]-threshold->initial_min[i_ref]));
      top = point;
      bottom = point+THRESHOLD_TRIANGLE_HEIGHT;
      if (threshold->volume->threshold_min[i_ref] < threshold->initial_min[i_ref])
	down_pointing=TRUE;
      fill_color = "white";
      break;
      
    case THRESHOLD_SCALED_MAX_ARROW:
      left = THRESHOLD_COLOR_SCALES_WIDTH+2*THRESHOLD_TRIANGLE_WIDTH;
      right = THRESHOLD_COLOR_SCALES_WIDTH+THRESHOLD_TRIANGLE_WIDTH;
      point = THRESHOLD_TRIANGLE_HEIGHT + 
	THRESHOLD_COLOR_SCALE_HEIGHT * 
	(1-(threshold->volume->threshold_max[i_ref]-threshold->initial_min[i_ref])/
	 (threshold->initial_max[i_ref]-threshold->initial_min[i_ref]));
      top = point-THRESHOLD_TRIANGLE_HEIGHT;
      bottom = point;
      if (threshold->volume->threshold_max[i_ref] > threshold->initial_max[i_ref])
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
      gtk_object_set_data(GTK_OBJECT(threshold->arrow[i_ref][arrow]), "type", GINT_TO_POINTER(arrow));
      gtk_object_set_data(GTK_OBJECT(threshold->arrow[i_ref][arrow]), "which_ref", GINT_TO_POINTER(i_ref));
      gtk_signal_connect(GTK_OBJECT(threshold->arrow[i_ref][arrow]), "event",
			 GTK_SIGNAL_FUNC(threshold_arrow_cb),
			 threshold);
    }
    gnome_canvas_points_unref(points);
  }
    
  return;
}


/* function called to update the color scales */
static void threshold_update_color_scale(AmitkThreshold * threshold, which_threshold_scale_t scale) {

  gdouble x,y;
  guint i_ref;
  

  for (i_ref=0; i_ref<threshold->visible_refs; i_ref++) {
    /* update the color scale pixbuf's */
    if (threshold->color_scale_rgb[i_ref][scale] != NULL)
      gdk_pixbuf_unref(threshold->color_scale_rgb[i_ref][scale]);
    
    switch (scale) {
      
    case THRESHOLD_FULL:
      threshold->color_scale_rgb[i_ref][scale] =
	image_from_colortable(threshold->volume->color_table,
			      THRESHOLD_COLOR_SCALE_WIDTH,
			      THRESHOLD_COLOR_SCALE_HEIGHT,
			      threshold->volume->threshold_min[i_ref],
			      threshold->volume->threshold_max[i_ref],
			      threshold->volume->global_min,
			      threshold->volume->global_max,
			      FALSE);
      x = THRESHOLD_TRIANGLE_WIDTH;
      y = THRESHOLD_TRIANGLE_HEIGHT;
      break;
      
    case THRESHOLD_SCALED:
      threshold->color_scale_rgb[i_ref][scale] =
	image_from_colortable(threshold->volume->color_table,
			      THRESHOLD_COLOR_SCALE_WIDTH,
			      THRESHOLD_COLOR_SCALE_HEIGHT,
			      threshold->volume->threshold_min[i_ref],
			      threshold->volume->threshold_max[i_ref],
			      threshold->initial_min[i_ref],
			      threshold->initial_max[i_ref],
			      FALSE);
      x = THRESHOLD_COLOR_SCALE_WIDTH+THRESHOLD_TRIANGLE_WIDTH+THRESHOLD_COLOR_SCALE_SEPARATION;
      y = THRESHOLD_TRIANGLE_HEIGHT;
      break;
      
    default:
      x = y = 0.0;
      break;
      
    }
   
    if (threshold->color_scale_image[i_ref][scale] != NULL) {
      gnome_canvas_item_set(threshold->color_scale_image[i_ref][scale], "pixbuf", 
			    threshold->color_scale_rgb[i_ref][scale], NULL);
    } else {
      threshold->color_scale_image[i_ref][scale] = 
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(threshold->color_scales[i_ref])),
			      gnome_canvas_pixbuf_get_type(),
			      "pixbuf", threshold->color_scale_rgb[i_ref][scale],
			      "x", x, "y", y, NULL);
    }
  }

  return;
}

static void threshold_update_connector_lines(AmitkThreshold * threshold, which_threshold_scale_t scale) {

  gdouble temp;
  GnomeCanvasPoints * line_points;
  guint i_ref;

  for (i_ref=0; i_ref<threshold->visible_refs; i_ref++) {

    /* update the lines that connect the max arrows of each color scale */
    line_points = gnome_canvas_points_new(2);
    line_points->coords[0] = THRESHOLD_COLOR_SCALE_WIDTH+THRESHOLD_TRIANGLE_WIDTH;
    temp = (1.0-(threshold->volume->threshold_max[i_ref]-threshold->volume->global_min)/
	    (threshold->volume->global_max-threshold->volume->global_min));
    if (temp < 0.0) temp = 0.0;
    line_points->coords[1] = THRESHOLD_TRIANGLE_HEIGHT + THRESHOLD_COLOR_SCALE_HEIGHT * temp;
    line_points->coords[2] = THRESHOLD_COLOR_SCALE_WIDTH+
      THRESHOLD_TRIANGLE_WIDTH+THRESHOLD_COLOR_SCALE_SEPARATION;
    temp = (1.0-(threshold->volume->threshold_max[i_ref]-threshold->initial_min[i_ref])/
	    (threshold->initial_max[i_ref]-threshold->initial_min[i_ref]));
    if ((temp < 0.0) || (scale==THRESHOLD_FULL) || isnan(temp)) temp = 0.0;
    line_points->coords[3] = THRESHOLD_TRIANGLE_HEIGHT + THRESHOLD_COLOR_SCALE_HEIGHT * temp;
    if (threshold->connector_line[i_ref][THRESHOLD_MAX_LINE] != NULL)
      gnome_canvas_item_set(threshold->connector_line[i_ref][THRESHOLD_MAX_LINE],
			    "points", line_points, NULL);
    else
      threshold->connector_line[i_ref][THRESHOLD_MAX_LINE] =
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(threshold->color_scales[i_ref])),
			      gnome_canvas_line_get_type(),
			      "points", line_points, 
			      "fill_color", "black",
			      "width_units", 1.0, NULL);
    gnome_canvas_points_unref(line_points);

    /* update the lines that connect the min arrows of each color scale */
    line_points = gnome_canvas_points_new(2);
    line_points->coords[0] = THRESHOLD_COLOR_SCALE_WIDTH+THRESHOLD_TRIANGLE_WIDTH;
    temp = (1.0-(threshold->volume->threshold_min[i_ref]-threshold->volume->global_min)/
	    (threshold->volume->global_max-threshold->volume->global_min));
    if (temp > 1.0) temp = 1.0;
    line_points->coords[1] = THRESHOLD_TRIANGLE_HEIGHT + THRESHOLD_COLOR_SCALE_HEIGHT * temp;
    line_points->coords[2] = THRESHOLD_COLOR_SCALE_WIDTH+
      THRESHOLD_TRIANGLE_WIDTH+THRESHOLD_COLOR_SCALE_SEPARATION;
    temp = (1.0-(threshold->volume->threshold_min[i_ref]-threshold->initial_min[i_ref])/
	    (threshold->initial_max[i_ref]-threshold->initial_min[i_ref]));
    if ((temp > 1.0) || (scale==THRESHOLD_FULL) || isnan(temp)) temp = 1.0;
    line_points->coords[3] = THRESHOLD_TRIANGLE_HEIGHT + THRESHOLD_COLOR_SCALE_HEIGHT * temp;
    if (threshold->connector_line[i_ref][THRESHOLD_MIN_LINE] != NULL)
      gnome_canvas_item_set(threshold->connector_line[i_ref][THRESHOLD_MIN_LINE],
			    "points", line_points, NULL);
    else
      threshold->connector_line[i_ref][THRESHOLD_MIN_LINE] =
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
    threshold_update_color_scale(threshold, THRESHOLD_FULL);

    for (i_ref=0; i_ref<threshold->visible_refs; i_ref++) {
      threshold->initial_min[i_ref] = threshold->volume->threshold_min[i_ref];
      threshold->initial_max[i_ref] = threshold->volume->threshold_max[i_ref];
    }

    threshold_update_color_scale(threshold, THRESHOLD_SCALED);
    threshold_update_connector_lines(threshold, THRESHOLD_SCALED);
    return;
}


/* update which widgets are shown or hidden */
static void threshold_update_layout(AmitkThreshold * threshold) {

  guint i_ref;
  which_threshold_entry_t i_entry;


  for (i_ref=0; i_ref<2; i_ref++) {


    if ((i_ref==1) && (threshold->volume->threshold_type != THRESHOLD_INTERPOLATE_FRAMES)) {

      gtk_widget_hide(threshold->percent_label[i_ref]);
      gtk_widget_hide(threshold->absolute_label[i_ref]);
      gtk_widget_hide(threshold->full_label[i_ref]);
      gtk_widget_hide(threshold->scaled_label[i_ref]);
      gtk_widget_hide(threshold->color_scales[i_ref]);
      
      for (i_entry=0; i_entry<NUM_THRESHOLD_ENTRIES; i_entry++)
	gtk_widget_hide(threshold->entry[i_ref][i_entry]);
      
    } else {
      
      gtk_widget_show(threshold->percent_label[i_ref]);
      gtk_widget_show(threshold->absolute_label[i_ref]);
      gtk_widget_show(threshold->full_label[i_ref]);
      gtk_widget_show(threshold->scaled_label[i_ref]);
      gtk_widget_show(threshold->color_scales[i_ref]);
      
      for (i_entry=0; i_entry<NUM_THRESHOLD_ENTRIES; i_entry++)
	gtk_widget_show(threshold->entry[i_ref][i_entry]);
    }
    
    if (threshold->volume->threshold_type != THRESHOLD_INTERPOLATE_FRAMES) {
      gtk_widget_hide(threshold->ref_frame_label[i_ref]);
      gtk_widget_hide(threshold->threshold_ref_frame_menu[i_ref]);
    } else {
      gtk_widget_show(threshold->ref_frame_label[i_ref]);
      gtk_widget_show_all(threshold->threshold_ref_frame_menu[i_ref]);
    }
    
  }
}



/* set which toggle button is depressed */
static void threshold_update_type(AmitkThreshold * threshold) {
  
  threshold_t i_threshold_type;

  for (i_threshold_type=0; i_threshold_type < NUM_THRESHOLD_TYPES; i_threshold_type++)
    gtk_signal_handler_block_by_func(GTK_OBJECT(threshold->type_button[i_threshold_type]),
				     GTK_SIGNAL_FUNC(threshold_type_cb),
				     threshold);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(threshold->type_button[threshold->volume->threshold_type]),
			       TRUE);
  for (i_threshold_type=0; i_threshold_type < NUM_THRESHOLD_TYPES; i_threshold_type++)
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(threshold->type_button[i_threshold_type]),
				       GTK_SIGNAL_FUNC(threshold_type_cb),
				       threshold);
  gtk_widget_set_sensitive(threshold->type_button[THRESHOLD_PER_FRAME],
			   (volume_dynamic(threshold->volume)));
  gtk_widget_set_sensitive(threshold->type_button[THRESHOLD_INTERPOLATE_FRAMES],
			   (volume_dynamic(threshold->volume)));

}


/* function called when the max or min triangle is moved 
 * mostly taken from Pennington's fine book */
static gint threshold_arrow_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {

  AmitkThreshold * threshold = data;
  gdouble item_x, item_y;
  gdouble delta;
  amide_data_t temp;
  which_threshold_arrow_t which_threshold_arrow;
  guint which_ref;

  which_ref = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "which_ref"));
  
  /* get the location of the event, and convert it to our coordinate system */
  item_x = event->button.x;
  item_y = event->button.y;
  gnome_canvas_item_w2i(GNOME_CANVAS_ITEM(widget)->parent, &item_x, &item_y);

  /* figure out which of the arrows triggered the callback */
  which_threshold_arrow = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "type"));

  /* switch on the event which called this */
  switch (event->type)
    {

    case GDK_BUTTON_PRESS:
      threshold->initial_y[which_ref] = item_y;
      threshold->initial_max[which_ref] = threshold->volume->threshold_max[which_ref];
      threshold->initial_min[which_ref] = threshold->volume->threshold_min[which_ref];
      gnome_canvas_item_grab(GNOME_CANVAS_ITEM(threshold->arrow[which_ref][which_threshold_arrow]),
			     GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			     threshold_cursor,
			     event->button.time);
      break;

    case GDK_MOTION_NOTIFY:
      delta = threshold->initial_y[which_ref] - item_y;

      if (event->motion.state & GDK_BUTTON1_MASK) {
	delta /= THRESHOLD_COLOR_SCALE_HEIGHT;
	
	switch (which_threshold_arrow) 
	  {
	  case THRESHOLD_FULL_MAX_ARROW:
	    delta *= (threshold->volume->global_max - threshold->volume->global_min);
	    temp = threshold->initial_max[which_ref] + delta;
	    if (temp <= threshold->volume->threshold_min[which_ref]) temp = threshold->volume->threshold_min[which_ref];
	    if (temp-CLOSE < threshold->volume->threshold_min[which_ref]) temp = threshold->volume->threshold_min[which_ref]+CLOSE;
	    threshold->volume->threshold_max[which_ref] = temp;
	    threshold_update_arrow(threshold, THRESHOLD_FULL_MAX_ARROW);
	    threshold_update_connector_lines(threshold, THRESHOLD_FULL);
	    break;
	  case THRESHOLD_FULL_MIN_ARROW:
	    delta *= (threshold->volume->global_max - threshold->volume->global_min);
	    temp = threshold->initial_min[which_ref] + delta;
	    if (temp < threshold->volume->global_min) temp = threshold->volume->global_min;
	    if (temp+CLOSE > threshold->volume->threshold_max[which_ref]) temp = threshold->volume->threshold_max[which_ref]-CLOSE;
	    if (temp > threshold->volume->global_max) temp = threshold->volume->global_max;
	    threshold->volume->threshold_min[which_ref] = temp;
	    threshold_update_arrow(threshold, THRESHOLD_FULL_MIN_ARROW);
	    threshold_update_connector_lines(threshold, THRESHOLD_FULL);
	    break;
	  case THRESHOLD_SCALED_MAX_ARROW:
	    delta *= (threshold->initial_max[which_ref] - threshold->initial_min[which_ref]);
	    temp = threshold->initial_max[which_ref] + delta;
	    if (temp <= threshold->volume->threshold_min[which_ref]) temp = threshold->volume->threshold_min[which_ref];
	    if (temp < 0.0) temp = 0.0;
	    if (temp-CLOSE < threshold->volume->threshold_min[which_ref]) temp = threshold->volume->threshold_min[which_ref]+CLOSE;
	    threshold->volume->threshold_max[which_ref] = temp;
	    threshold_update_arrow(threshold, THRESHOLD_FULL_MAX_ARROW);
	    threshold_update_arrow(threshold, THRESHOLD_SCALED_MAX_ARROW);
	    threshold_update_color_scale(threshold, THRESHOLD_SCALED);
	    threshold_update_connector_lines(threshold, THRESHOLD_SCALED);
	    break;
	  case THRESHOLD_SCALED_MIN_ARROW:
	    delta *= (threshold->initial_max[which_ref] - threshold->initial_min[which_ref]);
	    temp = threshold->initial_min[which_ref] + delta;
	    if (temp < threshold->volume->global_min) temp = threshold->volume->global_min;
	    if (temp >= threshold->volume->threshold_max[which_ref]) temp = threshold->volume->threshold_max[which_ref];
	    if (temp+CLOSE > threshold->volume->threshold_max[which_ref]) temp = threshold->volume->threshold_max[which_ref]-CLOSE;
	    if (temp > threshold->volume->global_max) temp = threshold->volume->global_max;
	    threshold->volume->threshold_min[which_ref] = temp;
	    threshold_update_arrow(threshold, THRESHOLD_FULL_MIN_ARROW);
	    threshold_update_arrow(threshold, THRESHOLD_SCALED_MIN_ARROW);
	    threshold_update_color_scale(threshold, THRESHOLD_SCALED);
	    threshold_update_connector_lines(threshold, THRESHOLD_SCALED);
	    break;
	  default:
	    break;
	  }

	threshold_update_color_scale(threshold, THRESHOLD_FULL);
	threshold_update_entries(threshold);   /* reset the entry widgets */
      }
      break;

    case GDK_BUTTON_RELEASE:
      gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
      threshold_update_entries(threshold);   /* reset the entry widgets */

      /* reset the scaled color scale */
      threshold->initial_max[which_ref] = threshold->volume->threshold_max[which_ref];
      threshold->initial_min[which_ref] = threshold->volume->threshold_min[which_ref];
      threshold_update_arrow(threshold, THRESHOLD_SCALED_MAX_ARROW);
      threshold_update_arrow(threshold, THRESHOLD_SCALED_MIN_ARROW);
      threshold_update_color_scale(threshold, THRESHOLD_SCALED);
      threshold_update_connector_lines(threshold, THRESHOLD_SCALED);

      /* signal we've changed */
      gtk_signal_emit (GTK_OBJECT (threshold), threshold_signals[THRESHOLD_CHANGED]);
      break;

    default:
      break;
    }
      
  return FALSE;
}


/* function to change the color table */
static void threshold_color_table_cb(GtkWidget * widget, gpointer data) {

  color_table_t i_color_table;
  AmitkThreshold * threshold = data;

  /* figure out which color table menu item called me */
  i_color_table = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"color_table"));

  /* check if we actually changed values */
  if (threshold->volume->color_table != i_color_table) {

    /* and inact the changes */
    threshold->volume->color_table = i_color_table;
    threshold_update_color_scales(threshold);

    /* signal we've changed */
    gtk_signal_emit (GTK_OBJECT (threshold), threshold_signals[COLOR_CHANGED]);
  }
  
  return;
}

/* function to update the reference frames */
static void threshold_ref_frame_cb(GtkWidget * widget, gpointer data) {

  guint which_ref,frame;
  AmitkThreshold * threshold = data;

  /* figure out which menu item called me */
  which_ref = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"which_ref"));
  frame = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"frame"));

  /* check if we actually changed values */
  if (frame != threshold->volume->threshold_ref_frame[which_ref]) {
    
    if ((which_ref==0) && (frame > threshold->volume->threshold_ref_frame[1]))
      frame =threshold->volume->threshold_ref_frame[1];
    else if ((which_ref==1) && (frame < threshold->volume->threshold_ref_frame[0]))
      frame = threshold->volume->threshold_ref_frame[0];
    gtk_option_menu_set_history(GTK_OPTION_MENU(threshold->threshold_ref_frame_menu[which_ref]), frame);

    /* and inact the changes */
    threshold->volume->threshold_ref_frame[which_ref]=frame;

    /* signal we've changed */
    gtk_signal_emit (GTK_OBJECT (threshold), threshold_signals[THRESHOLD_CHANGED]);
  }
  
  return;
}


static void threshold_entry_cb(GtkWidget* widget, gpointer data) {

  AmitkThreshold * threshold = data;
  gchar * string;
  gint error;
  gdouble temp;
  which_threshold_entry_t which_threshold_entry;
  gboolean update = FALSE;
  guint which_ref;

  /* figure out which of the arrows triggered the callback */
  which_threshold_entry = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "type"));
  which_ref = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "which_ref"));

  /* get the current entry */
  string = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);

  /* convert it to a floating point */
  error = sscanf(string, "%lf", &temp);
  g_free(string);
    
  /* make sure it's a valid floating point */
  if (!(error == EOF)) {
    switch (which_threshold_entry) 
      {
      case MAX_PERCENT:
	temp = (threshold->volume->global_max-threshold->volume->global_min) * temp/100.0+threshold->volume->global_min;
      case MAX_ABSOLUTE:
	if (temp > threshold->volume->threshold_min[which_ref]) {
	  threshold->volume->threshold_max[which_ref] = temp;
	  update = TRUE;
	}
	break;
      case MIN_PERCENT:
	temp = ((threshold->volume->global_max-threshold->volume->global_min) * temp/100.0)+threshold->volume->global_min;
      case MIN_ABSOLUTE:
	if ((temp < threshold->volume->global_max) && 
	    (temp < threshold->volume->threshold_max[which_ref])) {
	  threshold->volume->threshold_min[which_ref] = temp;
	  update = TRUE;
	}
      default:
	break;
      }
  }

  threshold_update_entries(threshold);
  /* if we had a valid change, update the canvases */
  if (update) {
    threshold_update_color_scales(threshold);
    threshold_update_arrow(threshold, THRESHOLD_FULL_MIN_ARROW);
    threshold_update_arrow(threshold, THRESHOLD_FULL_MAX_ARROW);

    /* signal we've changed */
    gtk_signal_emit (GTK_OBJECT (threshold), threshold_signals[THRESHOLD_CHANGED]);

  }

  return;
}


static void threshold_type_cb(GtkWidget * widget, gpointer data) {

  AmitkThreshold * threshold = data;
  threshold_t threshold_type;

  /* figure out which menu item called me */
  threshold_type = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget),"threshold_type"));

  /* check if we actually changed values */
  if (threshold->volume->threshold_type != threshold_type) {
    /* and inact the changes */
    threshold->volume->threshold_type = threshold_type;
    if (threshold->volume->threshold_type == THRESHOLD_INTERPOLATE_FRAMES) 
      threshold->visible_refs=2;
    else 
      threshold->visible_refs=1;

    amitk_threshold_update(threshold);

    /* signal we've changed */
    gtk_signal_emit (GTK_OBJECT (threshold), threshold_signals[THRESHOLD_CHANGED]);

  }
  
  return;
}



/* function to load a new volume into the widget */
void amitk_threshold_new_volume(AmitkThreshold * threshold, volume_t * new_volume) {

  g_return_if_fail(threshold != NULL);
  g_return_if_fail(new_volume != NULL);

  threshold->volume = volume_unref(threshold->volume); /* remove ref to the old volume */
  threshold->volume = volume_ref(new_volume); /* add a reference to the new volume */

  /* and upate the widget */
  amitk_threshold_update(threshold);
  
  return;
}

/* function to tell the threshold widget to redraw (i.e. the volume info's changes */
void amitk_threshold_update(AmitkThreshold * threshold) {

  g_return_if_fail(threshold != NULL);

  /* update what's on the pull down menu */
  gtk_option_menu_set_history(GTK_OPTION_MENU(threshold->color_table_menu), threshold->volume->color_table);

  threshold_update_layout(threshold);
  threshold_update_histogram(threshold);
  threshold_update_entries(threshold); /* update what's in the entry widgets */
  threshold_update_color_scales(threshold);
  threshold_update_arrow(threshold, THRESHOLD_FULL_MIN_ARROW);
  threshold_update_arrow(threshold, THRESHOLD_FULL_MAX_ARROW);
  threshold_update_type(threshold);

  return;

}


GtkWidget* amitk_threshold_new (volume_t * volume, 
				amitk_threshold_layout_t layout) {

  AmitkThreshold * threshold;
  which_threshold_arrow_t i_arrow;

  g_return_val_if_fail(volume != NULL, NULL);

  threshold = gtk_type_new (amitk_threshold_get_type ());

  threshold->volume = volume_ref(volume);
  if (threshold->volume->threshold_type == THRESHOLD_INTERPOLATE_FRAMES) 
    threshold->visible_refs=2;
  else 
    threshold->visible_refs=1;

  threshold_construct(threshold, layout);
  threshold_update_entries(threshold); /* update what's in the entry widgets */
  threshold_update_color_scales(threshold);
  for (i_arrow=0;i_arrow<NUM_THRESHOLD_ARROWS;i_arrow++)
    threshold_update_arrow(threshold, i_arrow);

  return GTK_WIDGET (threshold);
}






GtkType amitk_threshold_dialog_get_type (void)
{
  static GtkType threshold_dialog_type = 0;

  if (!threshold_dialog_type)
    {
      GtkTypeInfo threshold_dialog_info =
      {
	"AmitkThresholdDialog",
	sizeof (AmitkThresholdDialog),
	sizeof (AmitkThresholdDialogClass),
	(GtkClassInitFunc) threshold_dialog_class_init,
	(GtkObjectInitFunc) threshold_dialog_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      threshold_dialog_type = gtk_type_unique (gnome_dialog_get_type(), &threshold_dialog_info);
    }

  return threshold_dialog_type;
}

static void threshold_dialog_class_init (AmitkThresholdDialogClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) klass;

  threshold_dialog_parent_class = gtk_type_class (gnome_dialog_get_type());

  threshold_dialog_signals[THRESHOLD_CHANGED] =
    gtk_signal_new ("threshold_changed",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (AmitkThresholdDialogClass, threshold_changed),
		    gtk_marshal_NONE__NONE, GTK_TYPE_NONE, 0);
  threshold_dialog_signals[COLOR_CHANGED] =
    gtk_signal_new ("color_changed",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (AmitkThresholdDialogClass, color_changed),
		    gtk_marshal_NONE__NONE, GTK_TYPE_NONE, 0);
  gtk_object_class_add_signals (object_class, threshold_dialog_signals, LAST_SIGNAL);

}

static void threshold_dialog_init (AmitkThresholdDialog * dialog)
{

  gnome_dialog_append_buttons (GNOME_DIALOG (dialog),
			       GNOME_STOCK_BUTTON_CLOSE,
			       NULL);
  gnome_dialog_set_close(GNOME_DIALOG(dialog), TRUE);

  dialog->vbox = gtk_vbox_new (FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 10);
  gtk_container_add (GTK_CONTAINER (GNOME_DIALOG(dialog)->vbox), dialog->vbox);
  gtk_widget_show (dialog->vbox);

  /* make the volume title thingie */
  dialog->volume_label = gtk_label_new(NULL);
  gtk_box_pack_start(GTK_BOX(dialog->vbox), dialog->volume_label, FALSE, FALSE,0);
  gtk_widget_show(dialog->volume_label);

  /* hookup a callback to load in a pixmap icon when realized */
  gtk_signal_connect(GTK_OBJECT(dialog), "realize", GTK_SIGNAL_FUNC(threshold_dialog_realize_cb), NULL);
  return;
}

/* this gets called after we have a volume */
static void threshold_dialog_construct(AmitkThresholdDialog * dialog, 
				       volume_t * volume) {

  gchar * temp_string;

  dialog->threshold = amitk_threshold_new(volume, AMITK_THRESHOLD_LINEAR_LAYOUT);
  gtk_box_pack_end(GTK_BOX(dialog->vbox), dialog->threshold, TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT(dialog->threshold), "threshold_changed",
		     GTK_SIGNAL_FUNC(threshold_dialog_threshold_changed_cb), dialog);
  gtk_signal_connect(GTK_OBJECT(dialog->threshold), "color_changed",
		     GTK_SIGNAL_FUNC(threshold_dialog_color_changed_cb), dialog);
  gtk_widget_show(dialog->threshold);

  /* reset the title */
  temp_string = g_strdup_printf("data set: %s\n",volume->name);
  gtk_label_set_text (GTK_LABEL(dialog->volume_label), temp_string);
  g_free(temp_string);

  return;
}


/* this gets called when the threshold is realized */
static void threshold_dialog_realize_cb(GtkWidget * dialog, gpointer data) {
  GdkPixmap *pm;
  GdkBitmap *bm;

  g_return_if_fail(dialog != NULL);

  /* setup the threshold's icon */
  pm = gdk_pixmap_create_from_xpm_d(dialog->window, &bm,NULL,icon_threshold_xpm),
  gdk_window_set_icon(dialog->window, NULL, pm, bm);

  return;
}

static void threshold_dialog_threshold_changed_cb(GtkWidget * threshold, gpointer data) {
  AmitkThresholdDialog * dialog = data;
  /* signal we've changed */
  gtk_signal_emit (GTK_OBJECT(dialog), threshold_dialog_signals[THRESHOLD_CHANGED]);
  return;
}


static void threshold_dialog_color_changed_cb(GtkWidget * threshold, gpointer data) {
  AmitkThresholdDialog * dialog = data;
  /* signal we've changed */
  gtk_signal_emit (GTK_OBJECT(dialog), threshold_dialog_signals[COLOR_CHANGED]);
  return;
}


GtkWidget* amitk_threshold_dialog_new (volume_t * volume)
{
  AmitkThresholdDialog *dialog;

  g_return_val_if_fail(volume != NULL, NULL);

  dialog = gtk_type_new (AMITK_TYPE_THRESHOLD_DIALOG);
  threshold_dialog_construct(dialog, volume);
  gtk_window_set_title (GTK_WINDOW (dialog), "Threshold Dialog");

  return GTK_WIDGET (dialog);
}


/* function to load a new volume into the widget */
void amitk_threshold_dialog_new_volume(AmitkThresholdDialog * dialog, volume_t * new_volume) {

  gchar * temp_string;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(new_volume != NULL);

  amitk_threshold_new_volume(AMITK_THRESHOLD(dialog->threshold), new_volume);

  /* reset the title */
  temp_string = g_strdup_printf("data set: %s\n",new_volume->name);
  gtk_label_set_text (GTK_LABEL(dialog->volume_label), temp_string);
  g_free(temp_string);

  return;
}

/* function to return a pointer to the volume that this dialog currently referes to */
volume_t * amitk_threshold_dialog_volume(AmitkThresholdDialog * dialog) {

  return AMITK_THRESHOLD(dialog->threshold)->volume;
}




/* -------------- thresholds_dialog ---------------- */

GtkType amitk_thresholds_dialog_get_type (void)
{
  static GtkType thresholds_dialog_type = 0;

  if (!thresholds_dialog_type)
    {
      GtkTypeInfo thresholds_dialog_info =
      {
	"AmitkThresholdsDialog",
	sizeof (AmitkThresholdsDialog),
	sizeof (AmitkThresholdsDialogClass),
	(GtkClassInitFunc) thresholds_dialog_class_init,
	(GtkObjectInitFunc) thresholds_dialog_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      thresholds_dialog_type = gtk_type_unique (gnome_dialog_get_type(), &thresholds_dialog_info);
    }

  return thresholds_dialog_type;
}


static void thresholds_dialog_class_init (AmitkThresholdsDialogClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) klass;

  thresholds_dialog_parent_class = gtk_type_class (gnome_dialog_get_type());

  thresholds_dialog_signals[THRESHOLD_CHANGED] =
    gtk_signal_new ("threshold_changed",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (AmitkThresholdsDialogClass, threshold_changed),
		    gtk_marshal_NONE__NONE, GTK_TYPE_NONE, 0);
  thresholds_dialog_signals[COLOR_CHANGED] =
    gtk_signal_new ("color_changed",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (AmitkThresholdsDialogClass, color_changed),
		    gtk_marshal_NONE__NONE, GTK_TYPE_NONE, 0);
  gtk_object_class_add_signals (object_class, thresholds_dialog_signals, LAST_SIGNAL);

}


static void thresholds_dialog_init (AmitkThresholdsDialog * dialog)
{
  GtkWidget * main_vbox;

  dialog->thresholds = NULL;

  gnome_dialog_append_buttons (GNOME_DIALOG (dialog),
			       GNOME_STOCK_BUTTON_CLOSE,
			       NULL);
  gnome_dialog_set_close(GNOME_DIALOG(dialog), TRUE);

  main_vbox = gtk_vbox_new (FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 10);
  gtk_container_add (GTK_CONTAINER (GNOME_DIALOG(dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  dialog->notebook = gtk_notebook_new();
  gtk_container_add (GTK_CONTAINER (main_vbox), dialog->notebook);
  gtk_widget_show (dialog->notebook);

  /* hookup a callback to load in a pixmap icon when realized */
  gtk_signal_connect(GTK_OBJECT(dialog), "realize", GTK_SIGNAL_FUNC(threshold_dialog_realize_cb), NULL);
}

/* this gets called after we have a volume */
static void thresholds_dialog_construct(AmitkThresholdsDialog * dialog, 
					volumes_t * volumes) {

  GtkWidget * threshold;
  GtkWidget * label;

  while (volumes != NULL) {
    threshold = amitk_threshold_new(volumes->volume, AMITK_THRESHOLD_LINEAR_LAYOUT);
    dialog->thresholds = g_list_append(dialog->thresholds, threshold);

    label = gtk_label_new(volumes->volume->name);

    gtk_notebook_append_page(GTK_NOTEBOOK(dialog->notebook), threshold, label);
    
    gtk_signal_connect(GTK_OBJECT(threshold), "threshold_changed",
		       GTK_SIGNAL_FUNC(thresholds_dialog_threshold_changed_cb), dialog);
    gtk_signal_connect(GTK_OBJECT(threshold), "color_changed",
		       GTK_SIGNAL_FUNC(thresholds_dialog_color_changed_cb), dialog);

    gtk_widget_show(threshold);
    gtk_widget_show(label);

    volumes = volumes->next;
  }

  return;
}

static void thresholds_dialog_threshold_changed_cb(GtkWidget * threshold, gpointer data) {
  AmitkThresholdsDialog * thresholds_dialog = data;
  /* signal we've changed */
  gtk_signal_emit (GTK_OBJECT(thresholds_dialog), thresholds_dialog_signals[THRESHOLD_CHANGED]);
  return;
}


static void thresholds_dialog_color_changed_cb(GtkWidget * threshold, gpointer data) {
  AmitkThresholdsDialog * thresholds_dialog = data;
  /* signal we've changed */
  gtk_signal_emit (GTK_OBJECT(thresholds_dialog), thresholds_dialog_signals[COLOR_CHANGED]);
  return;
}


GtkWidget* amitk_thresholds_dialog_new (volumes_t * volumes)
{
  AmitkThresholdsDialog *dialog;

  g_return_val_if_fail(volumes != NULL, NULL);

  dialog = gtk_type_new (AMITK_TYPE_THRESHOLDS_DIALOG);
  thresholds_dialog_construct(dialog, volumes);
  gtk_window_set_title (GTK_WINDOW (dialog), "Threshold Dialog");

  return GTK_WIDGET (dialog);
}

