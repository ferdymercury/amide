/* tb_crop.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2002-2005 Andy Loening
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


#include "amide_config.h"
#include "image.h"
#include "tb_crop.h"
#include "amitk_threshold.h"
#include "amitk_progress_dialog.h"
#include "pixmaps.h"
#include "ui_common.h"


#define AXIS_WIDTH 120
#define AXIS_HEIGHT 120
#define CURSOR_SIZE 1

static const char * wizard_name = N_("Data Set Cropping Wizard");

static const char * finish_page_text = 
N_("When the apply button is hit, a new data set will be created\n"
   "and placed into the study's tree, consisting of the appropriately\n"
   "cropped data\n");

typedef enum {
  TRANSVERSE_PAGE,
  CORONAL_PAGE,
  SAGITTAL_PAGE,
  DATA_CONVERSION_PAGE,
  FINISH_PAGE,
  NUM_PAGES
} which_page_t;

typedef enum {
  RANGE_MIN,
  RANGE_MAX,
  NUM_RANGES
} range_t;

/* data structures */
typedef struct tb_crop_t {
  GtkWidget * dialog;

  AmitkVoxel range[NUM_RANGES];
  AmitkFormat format;
  AmitkScalingType scaling_type;
  guint frame;
  guint gate;
  gdouble zoom;
  amide_data_t threshold_max;
  amide_data_t threshold_min;
  AmitkColorTable color_table;
  gboolean threshold_info_set;

  AmitkStudy * study;
  AmitkDataSet * data_set;
  AmitkDataSet * projections[AMITK_VIEW_NUM];
  GtkWidget * canvas[AMITK_VIEW_NUM];
  GnomeCanvasItem * image[AMITK_VIEW_NUM];
  gint canvas_width[AMITK_VIEW_NUM];
  gint canvas_height[AMITK_VIEW_NUM];
  GnomeCanvasItem * line[AMITK_VIEW_NUM][2][NUM_RANGES];
  GtkWidget * threshold[AMITK_VIEW_NUM];

  GList * update_view;
  gint idle_handler_id;

  GtkWidget * zoom_spinner[AMITK_VIEW_NUM];
  GtkWidget * frame_spinner[AMITK_VIEW_NUM];
  GtkWidget * gate_spinner[AMITK_VIEW_NUM];
  GtkWidget * spinner[AMITK_VIEW_NUM][AMITK_DIM_NUM][NUM_RANGES];
  GtkWidget * table[NUM_PAGES];
  GtkWidget * progress_dialog;

  guint reference_count;
} tb_crop_t;


static void cancel_cb(GtkWidget* widget, gpointer data);
static gboolean delete_event(GtkWidget * widget, GdkEvent * event, gpointer data);

static tb_crop_t * tb_crop_free(tb_crop_t * tb_crop);
static tb_crop_t * tb_crop_init(void);


static void prepare_page_cb(GtkWidget * page, gpointer * druid, gpointer data);

static void zoom_spinner_cb(GtkSpinButton * button, gpointer data);
static void frame_spinner_cb(GtkSpinButton * button, gpointer data);
static void gate_spinner_cb(GtkSpinButton * button, gpointer data);
static void spinner_cb(GtkSpinButton * button, gpointer data);
static void projection_thresholds_changed_cb(AmitkDataSet * projection, gpointer data);
static void change_format_cb(GtkWidget * widget, gpointer data);
static void change_scaling_type_cb(GtkWidget * widget, gpointer data);


static void update_crop_lines(tb_crop_t * tb_crop, AmitkView view);
static void add_canvas_update(tb_crop_t * tb_crop, AmitkView view);
static gboolean update_canvas_while_idle(gpointer tb_crop);

static void finish_cb(GtkWidget* widget, gpointer druid, gpointer data);


static void prepare_page_cb(GtkWidget * page, gpointer * druid, gpointer data) {
 
  tb_crop_t * tb_crop = data;
  which_page_t which_page;
  AmitkView view;
  GtkWidget * label;
  GtkWidget * spin_button;
  gint table_row;
  gint table_column;
  AmitkDim i_dim;
  AmitkFormat i_format;
  AmitkScalingType i_scaling_type;
  range_t i_range;
  gchar * temp_string;
  GtkWidget * axis_canvas;
  GtkWidget * vseparator;
  GtkWidget * entry;
  GtkWidget * menu;
#if 1
  GtkWidget * option_menu;
  GtkWidget * menuitem;
#endif
  GtkWidget * hseparator;

  which_page = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(page), "which_page"));
  view = which_page-TRANSVERSE_PAGE;

  /* -------------- set the buttons correctly ---------------- */
  switch(which_page) {
  case TRANSVERSE_PAGE:
    gnome_druid_set_buttons_sensitive(GNOME_DRUID(druid), FALSE, TRUE, TRUE, TRUE);
    gnome_druid_set_show_finish(GNOME_DRUID(druid), FALSE);
    break;
  default:
    gnome_druid_set_buttons_sensitive(GNOME_DRUID(druid), TRUE, TRUE, TRUE, TRUE);
    gnome_druid_set_show_finish(GNOME_DRUID(druid), FALSE);
    break;
  }

  /* ---------------- finish up the pages as needed ---------------- */
  switch(which_page) {
  case TRANSVERSE_PAGE:
  case CORONAL_PAGE:
  case SAGITTAL_PAGE:
    if (tb_crop->table[view] == NULL) {
      tb_crop->table[view] = gtk_table_new(5,8,FALSE);
      gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(page)->vbox), 
			 tb_crop->table[view], TRUE, TRUE, 5);
      
      table_row=0;
      table_column=0;
      
      /* the zoom selection */
      label = gtk_label_new(_("zoom"));
      gtk_table_attach(GTK_TABLE(tb_crop->table[view]), label, 
		       table_column,table_column+1, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);
      
      spin_button =  
	gtk_spin_button_new_with_range(0.2,5.0,0.2);
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_button), FALSE);
      gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_button),2);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), tb_crop->zoom);
      g_object_set_data(G_OBJECT(spin_button), "which_view", GINT_TO_POINTER(view));
      g_signal_connect(G_OBJECT(spin_button), "value_changed",  
		       G_CALLBACK(zoom_spinner_cb), tb_crop);
      gtk_table_attach(GTK_TABLE(tb_crop->table[view]), spin_button, 
		       table_column+1,table_column+2, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);
      tb_crop->zoom_spinner[view] = spin_button;
      table_row++;
      
      /* the gate selection */
      if (AMITK_DATA_SET_NUM_GATES(tb_crop->data_set) > 1) {
	label = gtk_label_new(_("gate"));
	gtk_table_attach(GTK_TABLE(tb_crop->table[view]), label, 
			 table_column,table_column+1, table_row,table_row+1,
			 FALSE,FALSE, X_PADDING, Y_PADDING);
	
	spin_button =  
	  gtk_spin_button_new_with_range(0,AMITK_DATA_SET_NUM_GATES(tb_crop->data_set)-1,1);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_button), FALSE);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_button),0);
	g_object_set_data(G_OBJECT(spin_button), "which_view", GINT_TO_POINTER(view));
	g_signal_connect(G_OBJECT(spin_button), "value_changed",  
			 G_CALLBACK(gate_spinner_cb), tb_crop);
	gtk_table_attach(GTK_TABLE(tb_crop->table[view]), spin_button, 
			 table_column+1,table_column+2, table_row,table_row+1,
			 FALSE,FALSE, X_PADDING, Y_PADDING);
	tb_crop->gate_spinner[view] = spin_button;
	table_row++;
      }
      
      /* the frame selection */
      if (AMITK_DATA_SET_NUM_FRAMES(tb_crop->data_set) > 1) {
	label = gtk_label_new(_("frame"));
	gtk_table_attach(GTK_TABLE(tb_crop->table[view]), label, 
			 table_column,table_column+1, table_row,table_row+1,
			 FALSE,FALSE, X_PADDING, Y_PADDING);
	
	spin_button =  
	  gtk_spin_button_new_with_range(0,AMITK_DATA_SET_NUM_FRAMES(tb_crop->data_set)-1,1);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_button), FALSE);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_button),0);
	g_object_set_data(G_OBJECT(spin_button), "which_view", GINT_TO_POINTER(view));
	g_signal_connect(G_OBJECT(spin_button), "value_changed",  
			 G_CALLBACK(frame_spinner_cb), tb_crop);
	gtk_table_attach(GTK_TABLE(tb_crop->table[view]), spin_button, 
			 table_column+1,table_column+2, table_row,table_row+1,
			 FALSE,FALSE, X_PADDING, Y_PADDING);
	tb_crop->frame_spinner[view] = spin_button;
	table_row++;
      }
      
      /* the projection */
#ifdef AMIDE_LIBGNOMECANVAS_AA
      tb_crop->canvas[view] = gnome_canvas_new_aa();
#else
      tb_crop->canvas[view] = gnome_canvas_new();
#endif
      gtk_table_attach(GTK_TABLE(tb_crop->table[view]), tb_crop->canvas[view], 
		       table_column,table_column+2,table_row,table_row+4, 
		       FALSE,FALSE, X_PADDING, Y_PADDING);
      add_canvas_update(tb_crop, view);
      /* wait for canvas to update, this allows the projection to get made */
      while (tb_crop->idle_handler_id != 0) gtk_main_iteration();
      
      table_row=0;
      table_column += 2;
      
      /* a separator for clarity */
      vseparator = gtk_vseparator_new();
      gtk_table_attach(GTK_TABLE(tb_crop->table[view]), vseparator,
		       table_column, table_column+1, table_row, table_row+5, 
		       0, GTK_FILL, X_PADDING, Y_PADDING);
      
      
      table_row=0;
      table_column += 1;
      /* the range selectors */
      for (i_dim=0; i_dim<AMITK_DIM_NUM; i_dim++) {
	
	if (voxel_get_dim(AMITK_DATA_SET_DIM(tb_crop->data_set), i_dim) > 1) {
	  
	  temp_string = g_strdup_printf(_("%s range:"), amitk_dim_get_name(i_dim));
	  label = gtk_label_new(temp_string);
	  g_free(temp_string);
	  gtk_table_attach(GTK_TABLE(tb_crop->table[view]), label, 
			   table_column,table_column+1, table_row,table_row+1,
			   FALSE,FALSE, X_PADDING, Y_PADDING);
	  
	  for (i_range=0; i_range<NUM_RANGES; i_range++) {
	    spin_button =  
	      gtk_spin_button_new_with_range(0,voxel_get_dim(AMITK_DATA_SET_DIM(tb_crop->data_set), i_dim)-1,1);
	    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_button), FALSE);
	    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_button),0);
	    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), 
				      voxel_get_dim(tb_crop->range[i_range], i_dim));
	    g_object_set_data(G_OBJECT(spin_button), "which_view", GINT_TO_POINTER(view));
	    g_object_set_data(G_OBJECT(spin_button), "which_dim", GINT_TO_POINTER(i_dim));
	    g_object_set_data(G_OBJECT(spin_button), "which_range", GINT_TO_POINTER(i_range));
	    g_signal_connect(G_OBJECT(spin_button), "value_changed",  
			     G_CALLBACK(spinner_cb), tb_crop);
	    gtk_table_attach(GTK_TABLE(tb_crop->table[view]), spin_button, 
			     table_column+1+i_range,table_column+2+i_range, table_row,table_row+1,
			     FALSE,FALSE, X_PADDING, Y_PADDING);
	    tb_crop->spinner[view][i_dim][i_range] = spin_button;
	  }
	  table_row++;
	}
      }

      /* the axis display */
#ifdef AMIDE_LIBGNOMECANVAS_AA
      axis_canvas = gnome_canvas_new_aa();
#else
      axis_canvas = gnome_canvas_new();
#endif
      ui_common_draw_view_axis(GNOME_CANVAS(axis_canvas), 0, 0, view, 
			       AMITK_LAYOUT_LINEAR, AXIS_WIDTH, AXIS_HEIGHT);
      gtk_widget_set_size_request(axis_canvas, AXIS_WIDTH, AXIS_HEIGHT);
      gnome_canvas_set_scroll_region(GNOME_CANVAS(axis_canvas), 0.0, 0.0, 
				     3.0*AXIS_WIDTH, AXIS_HEIGHT);
      gtk_table_attach(GTK_TABLE(tb_crop->table[view]), axis_canvas, 
		       table_column,table_column+3,table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);
      table_row++;
      
      
      table_column += 1+NUM_RANGES;
      table_row = 0;
      /* a separator for clarity */
      vseparator = gtk_vseparator_new();
      gtk_table_attach(GTK_TABLE(tb_crop->table[view]), vseparator,
		       table_column, table_column+1, table_row, table_row+5, 
		       0, GTK_FILL, X_PADDING, Y_PADDING);
      
      
      table_row=0;
      table_column += 1;
      /* the threshold */
      tb_crop->threshold[view] = amitk_threshold_new(tb_crop->projections[view], 
						     AMITK_THRESHOLD_LINEAR_LAYOUT,
						     GTK_WINDOW(tb_crop->dialog), TRUE);
      gtk_table_attach(GTK_TABLE(tb_crop->table[view]), tb_crop->threshold[view],
		       table_column,table_column+1,table_row,table_row+5, 
		       FALSE,FALSE, X_PADDING, Y_PADDING);
      gtk_widget_show_all(tb_crop->table[view]);
    }
    break;
  case DATA_CONVERSION_PAGE:
    if (tb_crop->table[DATA_CONVERSION_PAGE] == NULL) {
      tb_crop->table[DATA_CONVERSION_PAGE] = gtk_table_new(3,3,FALSE);
      table_row = 0;
      table_column=0;
      gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(page)->vbox), 
			 tb_crop->table[view], TRUE, TRUE, 5);
      
      /* widget to tell you the internal data format */
      label = gtk_label_new(_("Current Data Format:"));
      gtk_table_attach(GTK_TABLE(tb_crop->table[DATA_CONVERSION_PAGE]), label, 0,1,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      
      entry = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(entry), 
			 amitk_format_names[AMITK_DATA_SET_FORMAT(tb_crop->data_set)]);
      gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
      gtk_table_attach(GTK_TABLE(tb_crop->table[DATA_CONVERSION_PAGE]), entry,
		       1,2, table_row, table_row+1, 
		       GTK_FILL, 0, X_PADDING, Y_PADDING);
      
      /* widget to tell you the scaling format */
      label = gtk_label_new(_("Current Scale Format:"));
      gtk_table_attach(GTK_TABLE(tb_crop->table[DATA_CONVERSION_PAGE]), label, 3,4,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      
      entry = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(entry), 
			 amitk_scaling_menu_names[AMITK_DATA_SET_SCALING_TYPE(tb_crop->data_set)]);
      gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
      gtk_table_attach(GTK_TABLE(tb_crop->table[DATA_CONVERSION_PAGE]), entry, 4,5, 
		       table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
      table_row++;
      
      /* a separator for clarity */
      hseparator = gtk_hseparator_new();
      gtk_table_attach(GTK_TABLE(tb_crop->table[DATA_CONVERSION_PAGE]), hseparator,0,5,
		       table_row, table_row+1, GTK_FILL, GTK_FILL, X_PADDING, Y_PADDING);
      table_row++;
      
      /* widget to tell you the internal data format */
      label = gtk_label_new(_("Output Data Format:"));
      gtk_table_attach(GTK_TABLE(tb_crop->table[DATA_CONVERSION_PAGE]), label, 0,1,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      
#if 1
      menu = gtk_menu_new();
      for (i_format=0; i_format<AMITK_FORMAT_NUM; i_format++) {
	menuitem = gtk_menu_item_new_with_label(amitk_format_names[i_format]);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
      }
      
      option_menu = gtk_option_menu_new();
      gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
      gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), tb_crop->format);
      g_signal_connect(G_OBJECT(option_menu), "changed", G_CALLBACK(change_format_cb), tb_crop);
      gtk_table_attach(GTK_TABLE(tb_crop->table[DATA_CONVERSION_PAGE]), option_menu,
#else
      menu = gtk_combo_box_new_text();
      for (i_format=0; i_format<AMITK_FORMAT_NUM; i_format++) 
	gtk_combo_box_append_text(GTK_COMBO_BOX(menu), amitk_format_names[i_format]);
      gtk_combo_box_set_active(GTK_COMBO_BOX(menu), tb_crop->format);
      g_signal_connect(G_OBJECT(menu), "changed", G_CALLBACK(change_format_cb), tb_crop);
      gtk_table_attach(GTK_TABLE(tb_crop->table[DATA_CONVERSION_PAGE]), menu,
#endif
		       1,2, table_row,table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
      
      
      /* widget to tell you the scaling format */
      label = gtk_label_new(_("Output Scale Format:"));
      gtk_table_attach(GTK_TABLE(tb_crop->table[DATA_CONVERSION_PAGE]), label, 3,4,
		       table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
      
#if 1
      menu = gtk_menu_new();
      for (i_scaling_type=0; i_scaling_type<AMITK_SCALING_TYPE_NUM; i_scaling_type++) {
	menuitem = gtk_menu_item_new_with_label(amitk_scaling_menu_names[i_scaling_type]);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
      }
      
      option_menu = gtk_option_menu_new();
      gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
      gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), tb_crop->scaling_type);
      g_signal_connect(G_OBJECT(option_menu), "changed", G_CALLBACK(change_scaling_type_cb), tb_crop);
      gtk_table_attach(GTK_TABLE(tb_crop->table[DATA_CONVERSION_PAGE]), option_menu,
#else
      menu = gtk_combo_box_new_text();
      for (i_scaling_type=0; i_scaling_type<AMITK_SCALING_TYPE_NUM; i_scaling_type++) 
	gtk_combo_box_append_text(GTK_COMBO_BOX(menu), amitk_scaling_menu_names[i_scaling_type]);
      gtk_combo_box_set_active(GTK_COMBO_BOX(menu), tb_crop->scaling_type);
      g_signal_connect(G_OBJECT(menu), "changed", G_CALLBACK(change_scaling_type_cb), tb_crop);
      gtk_table_attach(GTK_TABLE(tb_crop->table[DATA_CONVERSION_PAGE]), menu,
#endif
		       4,5, table_row,table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
      
      gtk_widget_show_all(tb_crop->table[DATA_CONVERSION_PAGE]);
    }
    break;
  default:
    break;
  }

  /* ---------------- set entries appropriately ---------------- */
  switch(which_page) {
  case TRANSVERSE_PAGE:
  case CORONAL_PAGE:
  case SAGITTAL_PAGE:

    g_signal_handlers_block_by_func(G_OBJECT(tb_crop->zoom_spinner[view]), 
				    G_CALLBACK(zoom_spinner_cb), tb_crop);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_crop->zoom_spinner[view]), 
			      tb_crop->zoom);
    g_signal_handlers_unblock_by_func(G_OBJECT(tb_crop->zoom_spinner[view]), 
				      G_CALLBACK(zoom_spinner_cb), tb_crop);

    if (AMITK_DATA_SET_NUM_GATES(tb_crop->data_set) > 1) {
      g_signal_handlers_block_by_func(G_OBJECT(tb_crop->gate_spinner[view]), 
				      G_CALLBACK(gate_spinner_cb), tb_crop);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_crop->gate_spinner[view]), 
				tb_crop->gate);
      g_signal_handlers_unblock_by_func(G_OBJECT(tb_crop->gate_spinner[view]), 
					G_CALLBACK(gate_spinner_cb), tb_crop);
    }

    if (AMITK_DATA_SET_NUM_FRAMES(tb_crop->data_set) > 1) {
      g_signal_handlers_block_by_func(G_OBJECT(tb_crop->frame_spinner[view]), 
				      G_CALLBACK(frame_spinner_cb), tb_crop);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_crop->frame_spinner[view]), 
				tb_crop->frame);
      g_signal_handlers_unblock_by_func(G_OBJECT(tb_crop->frame_spinner[view]), 
					G_CALLBACK(frame_spinner_cb), tb_crop);
    }

    for (i_dim=0; i_dim<AMITK_DIM_NUM; i_dim++) {
      if (voxel_get_dim(AMITK_DATA_SET_DIM(tb_crop->data_set), i_dim) > 1) {
	for (i_range=0; i_range<NUM_RANGES; i_range++) {
	  g_signal_handlers_block_by_func(G_OBJECT(tb_crop->spinner[view][i_dim][i_range]), 
					  G_CALLBACK(spinner_cb), tb_crop);
	  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_crop->spinner[view][i_dim][i_range]), 
				    voxel_get_dim(tb_crop->range[i_range], i_dim));
	  g_signal_handlers_unblock_by_func(G_OBJECT(tb_crop->spinner[view][i_dim][i_range]), 
					    G_CALLBACK(spinner_cb), tb_crop);
	}
      }
    }

    gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(tb_crop->canvas[view]), 
				     tb_crop->zoom);
    gtk_widget_set_size_request(tb_crop->canvas[view], 
				tb_crop->zoom*tb_crop->canvas_width[view],
				tb_crop->zoom*tb_crop->canvas_height[view]);

    add_canvas_update(tb_crop, view);
    update_crop_lines(tb_crop, view);

    amitk_data_set_set_color_table(tb_crop->projections[view], tb_crop->color_table);
    amitk_data_set_set_threshold_min(tb_crop->projections[view], 0, tb_crop->threshold_min);
    amitk_data_set_set_threshold_max(tb_crop->projections[view], 0, tb_crop->threshold_max);

   break;
  default:
    break;
  }


  return;
}


static void zoom_spinner_cb(GtkSpinButton * spin_button, gpointer data) {

  tb_crop_t * tb_crop = data;
  AmitkView view;
  gdouble value;

  value = gtk_spin_button_get_value(spin_button);
  view = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(spin_button), "which_view"));

  if (value < 0.2)
    tb_crop->zoom = 0.2;
  else if (value >= 5.0)
    tb_crop->zoom = 5.0;
  else
    tb_crop->zoom = value;
  
  gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(tb_crop->canvas[view]), tb_crop->zoom);
  gtk_widget_set_size_request(tb_crop->canvas[view], 
			      tb_crop->zoom*tb_crop->canvas_width[view],
			      tb_crop->zoom*tb_crop->canvas_height[view]);

  return;
}

static void frame_spinner_cb(GtkSpinButton * spin_button, gpointer data) {

  tb_crop_t * tb_crop = data;
  AmitkView view;
  AmitkView i_view;
  gint int_value;

  int_value = gtk_spin_button_get_value_as_int(spin_button);
  view = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(spin_button), "which_view"));

  if (int_value < 0)
    int_value = 0;
  else if (int_value >= AMITK_DATA_SET_NUM_FRAMES(tb_crop->data_set))
    int_value = AMITK_DATA_SET_NUM_FRAMES(tb_crop->data_set)-1;
  
  
  if (int_value != tb_crop->frame) {
    tb_crop->frame = int_value;
    
    g_signal_handlers_block_by_func(G_OBJECT(tb_crop->frame_spinner[view]), 
				    G_CALLBACK(frame_spinner_cb), tb_crop);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_crop->frame_spinner[view]), 
			      tb_crop->frame);
    g_signal_handlers_unblock_by_func(G_OBJECT(tb_crop->frame_spinner[view]), 
				      G_CALLBACK(frame_spinner_cb), tb_crop);
    
    /* unref all the computed projections */
    for (i_view=0; i_view < AMITK_VIEW_NUM; i_view++) 
      if (tb_crop->projections[i_view] != NULL)
	tb_crop->projections[i_view] = amitk_object_unref(tb_crop->projections[i_view]);
    
    /* just update the current projection for now */
    add_canvas_update(tb_crop, view);
  }
  

  return;
}

static void gate_spinner_cb(GtkSpinButton * spin_button, gpointer data) {

  tb_crop_t * tb_crop = data;
  AmitkView view;
  AmitkView i_view;
  gint int_value;

  int_value = gtk_spin_button_get_value_as_int(spin_button);
  view = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(spin_button), "which_view"));

  if (int_value < 0)
    int_value = 0;
  else if (int_value >= AMITK_DATA_SET_NUM_GATES(tb_crop->data_set))
    int_value = AMITK_DATA_SET_NUM_GATES(tb_crop->data_set)-1;
  
  
  if (int_value != tb_crop->gate) {
    tb_crop->gate = int_value;
    
    g_signal_handlers_block_by_func(G_OBJECT(tb_crop->gate_spinner[view]), 
				    G_CALLBACK(gate_spinner_cb), tb_crop);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_crop->gate_spinner[view]), 
			      tb_crop->gate);
    g_signal_handlers_unblock_by_func(G_OBJECT(tb_crop->gate_spinner[view]), 
				      G_CALLBACK(gate_spinner_cb), tb_crop);
    
    /* unref all the computed projections */
    for (i_view=0; i_view < AMITK_VIEW_NUM; i_view++) 
      if (tb_crop->projections[i_view] != NULL)
	tb_crop->projections[i_view] = amitk_object_unref(tb_crop->projections[i_view]);
    
    /* just update the current projection for now */
    add_canvas_update(tb_crop, view);
  }
  

  return;
}


static void spinner_cb(GtkSpinButton * spin_button, gpointer data) {

  tb_crop_t * tb_crop = data;
  AmitkView view;
  AmitkDim dim;
  range_t which_range;
  gint int_value;
  gboolean valid = FALSE;

  view = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(spin_button), "which_view"));
  dim = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(spin_button), "which_dim"));
  which_range = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(spin_button), "which_range"));

  int_value = gtk_spin_button_get_value_as_int(spin_button);
  
  if (which_range == RANGE_MIN) {
    if (int_value <= voxel_get_dim(tb_crop->range[RANGE_MAX], dim)) {
      valid = TRUE;
    } else {
      int_value = voxel_get_dim(tb_crop->range[RANGE_MAX], dim);
    }
  } else { /* RANGE_MAX */
    if (int_value >= voxel_get_dim(tb_crop->range[RANGE_MIN], dim)) {
      valid = TRUE;
    } else {
      int_value = voxel_get_dim(tb_crop->range[RANGE_MIN], dim);
    }
  }
  
  voxel_set_dim(&(tb_crop->range[which_range]), dim, int_value);
  
  
  g_signal_handlers_block_by_func(G_OBJECT(tb_crop->spinner[view][dim][which_range]), 
				  G_CALLBACK(spinner_cb), tb_crop);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_crop->spinner[view][dim][which_range]), 
			    voxel_get_dim(tb_crop->range[which_range], dim));
  g_signal_handlers_unblock_by_func(G_OBJECT(tb_crop->spinner[view][dim][which_range]), 
				    G_CALLBACK(spinner_cb), tb_crop);
  
  update_crop_lines(tb_crop, view);
  

  return;
}




static void projection_thresholds_changed_cb(AmitkDataSet * projection, gpointer data) {

  tb_crop_t * tb_crop = data;
  AmitkView view;

  view = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(projection), "which_view"));

  tb_crop->color_table = AMITK_DATA_SET_COLOR_TABLE(projection);
  tb_crop->threshold_max = AMITK_DATA_SET_THRESHOLD_MAX(projection, 0);
  tb_crop->threshold_min = AMITK_DATA_SET_THRESHOLD_MIN(projection, 0);

  add_canvas_update(tb_crop, view);
  update_crop_lines(tb_crop, view);

  return;
}

/* function called to change the desired format */
static void change_format_cb(GtkWidget * widget, gpointer data) {
  tb_crop_t * tb_crop = data;
#if 1
  tb_crop->format = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));
#else
  tb_crop->format = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
#endif
  return;
}

/* function called to change the desired scaling */
static void change_scaling_type_cb(GtkWidget * widget, gpointer data) {
  tb_crop_t * tb_crop = data;
#if 1
  tb_crop->scaling_type = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));
#else
  tb_crop->scaling_type = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
#endif
  return;
}


static void update_crop_lines(tb_crop_t * tb_crop, AmitkView view) {

  GnomeCanvasPoints * points;
  range_t i_range;
  rgba_t outline_color;
  gint j;
  gint x_range[NUM_RANGES];
  gint y_range[NUM_RANGES];

  if (tb_crop->canvas[view] == NULL) return;

  points = gnome_canvas_points_new(2);
  outline_color = 
    amitk_color_table_outline_color(AMITK_DATA_SET_COLOR_TABLE(tb_crop->projections[view]), FALSE);

  switch(view) {
  case AMITK_VIEW_CORONAL:
    for (i_range=0; i_range<NUM_RANGES; i_range++) {
      x_range[i_range] = tb_crop->range[i_range].x;
      y_range[i_range] = tb_crop->range[i_range].z;
    }
    break;
  case AMITK_VIEW_SAGITTAL:
    for (i_range=0; i_range<NUM_RANGES; i_range++) {
      x_range[i_range] = tb_crop->range[i_range].y;
      y_range[i_range] = tb_crop->range[i_range].z;
    }
    break;
  case AMITK_VIEW_TRANSVERSE:
  default:
    for (i_range=0; i_range<NUM_RANGES; i_range++) {
      x_range[i_range] = tb_crop->range[i_range].x;
      y_range[i_range] = tb_crop->range[i_range].y;
    }
    break;
  }

  for (j=0; j<2; j++) {
    for (i_range=0; i_range<NUM_RANGES; i_range++) {

      /* the 0.5 is because the points specify the middle of the line, not the northwest corner */
      if (j==0) {/* x direction */

	if (i_range == RANGE_MIN)
	  points->coords[0]=points->coords[2] = x_range[i_range]+0.5;
	else
	  points->coords[0]=points->coords[2] = x_range[i_range]+0.5+2*CURSOR_SIZE;

	if (view == AMITK_VIEW_TRANSVERSE) {
	  points->coords[1] = tb_crop->canvas_height[view]-y_range[RANGE_MIN]-1+0.5-CURSOR_SIZE;
	  points->coords[3] = tb_crop->canvas_height[view]-y_range[RANGE_MAX]-1+0.5-CURSOR_SIZE;
	} else { /* our z direction points down */
	  points->coords[1] = y_range[RANGE_MIN]+0.5;
	  points->coords[3] = y_range[RANGE_MAX]+0.5+2*CURSOR_SIZE;
	}


      } else { /* y direction, compensate for X Window's coordinate axis */

	if (i_range == RANGE_MIN) {
	  if (view == AMITK_VIEW_TRANSVERSE)
	    points->coords[1]=points->coords[3] = tb_crop->canvas_height[view]-y_range[i_range]+0.5-CURSOR_SIZE;
	  else /* our z direction points down */
	    points->coords[1]=points->coords[3] = y_range[i_range]+0.5;
	} else {
	  if (view == AMITK_VIEW_TRANSVERSE)
	    points->coords[1]=points->coords[3] = tb_crop->canvas_height[view]-y_range[i_range]-1+0.5-2*CURSOR_SIZE;
	  else /* our z direction points down */
	    points->coords[1]=points->coords[3] = y_range[i_range]+0.5+2*CURSOR_SIZE;
	}

	points->coords[0] = x_range[RANGE_MIN]+0.5;
	points->coords[2] = x_range[RANGE_MAX]+0.5+2*CURSOR_SIZE;
      }
      
      if (tb_crop->line[view][j][i_range] == NULL) {
	tb_crop->line[view][j][i_range] = 
	  gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(tb_crop->canvas[view])), 
				gnome_canvas_line_get_type(),
				"points", points, 
				"fill_color_rgba", amitk_color_table_rgba_to_uint32(outline_color),
				"width_units", 1.0,
				"cap_style", GDK_CAP_PROJECTING,
				NULL);
      } else {
	gnome_canvas_item_set(tb_crop->line[view][j][i_range], "points", points, 
			      "fill_color_rgba", amitk_color_table_rgba_to_uint32(outline_color),NULL);
      }
    }
  }

  gnome_canvas_points_unref(points);

  return;
}

static void add_canvas_update(tb_crop_t * tb_crop, AmitkView view) {

  if (g_list_index(tb_crop->update_view, GINT_TO_POINTER(view)) < 0)
    tb_crop->update_view = g_list_append(tb_crop->update_view, GINT_TO_POINTER(view));

  if (tb_crop->idle_handler_id == 0)
    tb_crop->idle_handler_id = 
      g_idle_add_full(G_PRIORITY_HIGH_IDLE,update_canvas_while_idle, tb_crop, NULL);

}

static gboolean update_canvas_while_idle(gpointer data) {
  tb_crop_t * tb_crop = data;
  GdkPixbuf * pixbuf;
  AmitkView view, i_view;

  while (tb_crop->update_view != NULL) {
    view = GPOINTER_TO_INT(tb_crop->update_view->data);
    tb_crop->update_view = g_list_remove(tb_crop->update_view, GINT_TO_POINTER(view));

    /* create the projections if we haven't already */
    if (tb_crop->projections[view] == NULL) 
      amitk_data_set_get_projections(tb_crop->data_set, tb_crop->frame, tb_crop->gate, 
				     tb_crop->projections, 
				     amitk_progress_dialog_update, tb_crop->progress_dialog);

    for (i_view=0; i_view<AMITK_VIEW_NUM; i_view++) {
      if (tb_crop->projections[i_view] != NULL) {
	if (!tb_crop->threshold_info_set) {
	  tb_crop->threshold_info_set = TRUE;
	  tb_crop->color_table = AMITK_DATA_SET_COLOR_TABLE(tb_crop->projections[i_view]);
	  tb_crop->threshold_max = AMITK_DATA_SET_THRESHOLD_MAX(tb_crop->projections[i_view], 0);
	  tb_crop->threshold_min = AMITK_DATA_SET_THRESHOLD_MIN(tb_crop->projections[i_view], 0);
	} else {
	  amitk_data_set_set_color_table(tb_crop->projections[i_view], tb_crop->color_table);
	  amitk_data_set_set_threshold_max(tb_crop->projections[i_view], 0, tb_crop->threshold_max);
	  amitk_data_set_set_threshold_min(tb_crop->projections[i_view], 0, tb_crop->threshold_min);
	}
	g_object_set_data(G_OBJECT(tb_crop->projections[i_view]), "which_view", GINT_TO_POINTER(i_view));
	g_signal_connect(G_OBJECT(tb_crop->projections[i_view]), "thresholds_changed",
			 G_CALLBACK(projection_thresholds_changed_cb), tb_crop);
	g_signal_connect(G_OBJECT(tb_crop->projections[i_view]), "color_table_changed",
			 G_CALLBACK(projection_thresholds_changed_cb), tb_crop);
	//	if (tb_crop->threshold[i_view] != NULL)
	//	  amitk_threshold_new_data_set(AMITK_THRESHOLD(tb_crop->threshold[i_view]), 
	//				       tb_crop->projections[i_view]);
      }
    }

      
    if ((tb_crop->canvas[view] != NULL) && (tb_crop->projections[view] != NULL)) {
      /* make a pixbuf based on the projection */
      pixbuf = image_from_projection(tb_crop->projections[view]);
      
      if (tb_crop->image[view] == NULL) {/* create the canvas image if we don't have it */
	tb_crop->image[view] = gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(tb_crop->canvas[view])),
						     gnome_canvas_pixbuf_get_type(),
						     "pixbuf", pixbuf, 
						     "x", (gdouble) CURSOR_SIZE, 
						     "y", (gdouble) CURSOR_SIZE, 
						     NULL);
	tb_crop->canvas_width[view] =gdk_pixbuf_get_width(pixbuf)+2*CURSOR_SIZE;
	tb_crop->canvas_height[view] = gdk_pixbuf_get_height(pixbuf)+2*CURSOR_SIZE;
	gtk_widget_set_size_request(tb_crop->canvas[view], 
				    tb_crop->canvas_width[view],
				    tb_crop->canvas_height[view]);
	
	gnome_canvas_set_scroll_region(GNOME_CANVAS(tb_crop->canvas[view]), 0,0,
				       tb_crop->canvas_width[view],
				       tb_crop->canvas_height[view]);
      } else {
	gnome_canvas_item_set(tb_crop->image[view], "pixbuf", pixbuf, NULL);
      }
      g_object_unref(pixbuf);
    }
  }

  tb_crop->idle_handler_id=0;
  
  return FALSE;
}

/* function called when the finish button is hit */
static void finish_cb(GtkWidget* widget, gpointer druid, gpointer data) {

  tb_crop_t * tb_crop = data;
  AmitkDataSet * cropped;

  /* disable the buttons */
  gtk_widget_set_sensitive(GTK_WIDGET(druid), FALSE);

  /* generate the new data set */
  cropped = amitk_data_set_get_cropped(tb_crop->data_set, 
				       tb_crop->range[RANGE_MIN],
				       tb_crop->range[RANGE_MAX],
				       tb_crop->format,
				       tb_crop->scaling_type,
				       amitk_progress_dialog_update, 
				       tb_crop->progress_dialog);

  /* if successful, add the new data set to the study and quit*/
  if (cropped != NULL) {
    amitk_object_add_child(AMITK_OBJECT(tb_crop->study), 
			   AMITK_OBJECT(cropped)); /* this adds a reference to the data set*/
    amitk_object_unref(cropped); /* so remove a reference */

    /* close the dialog box */
    cancel_cb(widget, data);
  } else {

    /* reenable the buttons */
    gtk_widget_set_sensitive(GTK_WIDGET(druid), TRUE);
  }

  return;
}





/* function called to cancel the dialog */
static void cancel_cb(GtkWidget* widget, gpointer data) {

  tb_crop_t * tb_crop = data;
  GtkWidget * dialog = tb_crop->dialog;
  gboolean return_val;

  /* run the delete event function */
  g_signal_emit_by_name(G_OBJECT(dialog), "delete_event", NULL, &return_val);
  if (!return_val) gtk_widget_destroy(dialog);

  return;
}

/* function called on a delete event */
static gboolean delete_event(GtkWidget * widget, GdkEvent * event, gpointer data) {

  tb_crop_t * tb_crop = data;

  /* trash collection */
  tb_crop = tb_crop_free(tb_crop);

  return FALSE;
}






static tb_crop_t * tb_crop_free(tb_crop_t * tb_crop) {

  AmitkView i_view;
  gboolean return_val;

  /* sanity checks */
  g_return_val_if_fail(tb_crop != NULL, NULL);
  g_return_val_if_fail(tb_crop->reference_count > 0, NULL);

  /* remove a reference count */
  tb_crop->reference_count--;

  /* things to do if we've removed all reference's */
  if (tb_crop->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing tb_crop\n");
#endif

    if (tb_crop->idle_handler_id != 0) {
      g_source_remove(tb_crop->idle_handler_id);
      tb_crop->idle_handler_id = 0;
    }

    if (tb_crop->data_set != NULL) {
      amitk_object_unref(tb_crop->data_set);
      tb_crop->data_set = NULL;
    }

    if (tb_crop->study != NULL) {
      amitk_object_unref(tb_crop->study);
      tb_crop->study = NULL;
    }
    
    for (i_view=0; i_view < AMITK_VIEW_NUM; i_view++) {
      if (tb_crop->projections[i_view] != NULL) {
	amitk_object_unref(tb_crop->projections[i_view]);
	tb_crop->projections[i_view] = NULL;
      }
    }

    if (tb_crop->progress_dialog != NULL) {
      g_signal_emit_by_name(G_OBJECT(tb_crop->progress_dialog), "delete_event", NULL, &return_val);
      tb_crop->progress_dialog = NULL;
    }

    g_free(tb_crop);
    tb_crop = NULL;
  }

  return tb_crop;

}

static tb_crop_t * tb_crop_init(void) {

  AmitkView i_view;
  tb_crop_t * tb_crop;
  gint j;
  range_t i_range;
  which_page_t i_page;

  /* alloc space for the data structure for passing ui info */
  if ((tb_crop = g_try_new(tb_crop_t,1)) == NULL) {
    g_warning(_("couldn't allocate space for tb_crop_t"));
    return NULL;
  }

  tb_crop->reference_count = 1;
  tb_crop->frame = 0;
  tb_crop->gate=0;
  tb_crop->dialog = NULL;
  tb_crop->data_set = NULL;
  tb_crop->study = NULL;
  tb_crop->zoom = 1.0;
  tb_crop->threshold_info_set = FALSE;
  tb_crop->format = AMITK_FORMAT_FLOAT;
  tb_crop->scaling_type = AMITK_SCALING_TYPE_0D;

  for (i_range=0; i_range<NUM_RANGES; i_range++)
    tb_crop->range[i_range] = zero_voxel;
  for (i_page=0; i_page < NUM_PAGES; i_page++)
    tb_crop->table[i_page]=NULL;
  for (i_view=0; i_view < AMITK_VIEW_NUM; i_view++) {
    tb_crop->image[i_view] = NULL;
    tb_crop->projections[i_view] = NULL;
    tb_crop->threshold[i_view] = NULL;
    tb_crop->canvas[i_view]=NULL;
    for (j=0; j<2; j++) 
      for (i_range=0; i_range<NUM_RANGES; i_range++) 
	tb_crop->line[i_view][j][i_range]=NULL;
  }
  tb_crop->update_view=NULL;
  tb_crop->idle_handler_id = 0;

  return tb_crop;
}


void tb_crop(AmitkStudy * study, AmitkDataSet * active_ds) {

  tb_crop_t * tb_crop;
  GdkPixbuf * logo;
  AmitkView i_view;
  GtkWidget * druid;
  GtkWidget * page;

  if (active_ds == NULL) {
    g_warning(_("No data set is currently marked as active"));
    return;
  }
  

  logo = gdk_pixbuf_new_from_inline(-1, amide_logo_small, FALSE, NULL);

  tb_crop = tb_crop_init();
  tb_crop->study = amitk_object_ref(study);
  tb_crop->data_set = amitk_object_ref(active_ds);
  tb_crop->format = AMITK_DATA_SET_FORMAT(active_ds);
  tb_crop->scaling_type = AMITK_DATA_SET_SCALING_TYPE(active_ds);
  tb_crop->frame = amitk_data_set_get_frame(active_ds, AMITK_STUDY_VIEW_START_TIME(study));
  tb_crop->gate = AMITK_DATA_SET_VIEW_START_GATE(active_ds);

  tb_crop->dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(G_OBJECT(tb_crop->dialog), "delete_event",
		   G_CALLBACK(delete_event), tb_crop);

  tb_crop->progress_dialog = amitk_progress_dialog_new(GTK_WINDOW(tb_crop->dialog));

  druid = gnome_druid_new();
  gtk_container_add(GTK_CONTAINER(tb_crop->dialog), druid);
  g_signal_connect(G_OBJECT(druid), "cancel", 
		   G_CALLBACK(cancel_cb), tb_crop);


  tb_crop->range[RANGE_MAX] = voxel_sub(AMITK_DATA_SET_DIM(tb_crop->data_set), one_voxel);


  for (i_view=0; i_view<NUM_PAGES-1; i_view++) {
    page = gnome_druid_page_standard_new_with_vals(wizard_name,logo,NULL);
    g_object_set_data(G_OBJECT(page),"which_page", GINT_TO_POINTER(i_view+TRANSVERSE_PAGE));
    
    /* note, the _connect_after is a workaround, it should just be _connect */
    /* the problem, is gnome_druid currently overwrites button sensitivity in its own prepare routine*/
    g_signal_connect_after(G_OBJECT(page), "prepare", 
			   G_CALLBACK(prepare_page_cb), tb_crop);
    gnome_druid_append_page(GNOME_DRUID(druid), GNOME_DRUID_PAGE(page));
  }

    
    

    
  /* ----------------  conclusion page ---------------------------------- */
  page = gnome_druid_page_edge_new_with_vals(GNOME_EDGE_FINISH, TRUE,
					     wizard_name,finish_page_text, logo,NULL, NULL);
  g_signal_connect(G_OBJECT(page), "finish",
		   G_CALLBACK(finish_cb), tb_crop);
  gnome_druid_append_page(GNOME_DRUID(druid),  GNOME_DRUID_PAGE(page));

  g_object_unref(logo);
  gtk_widget_show_all(tb_crop->dialog);
  return;
}











