/* tb_filter.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2002-2003 Andy Loening
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
#include "amitk_data_set.h"
#include "amitk_progress_dialog.h"
#include "tb_filter.h"
#include "pixmaps.h"

#define MIN_FIR_FILTER_SIZE 7
#define MAX_FIR_FILTER_SIZE 101
#define MIN_NONLINEAR_FILTER_SIZE 3
#define MAX_NONLINEAR_FILTER_SIZE 11
#define DEFAULT_FILTER AMITK_FILTER_GAUSSIAN
#define DEFAULT_GAUSSIAN_FILTER_SIZE 25
#define DEFAULT_MEDIAN_FILTER_SIZE 3

#define MAX_FWHM 100.0 /* mm */
#define MIN_FWHM 0.0 /* mm */

static const char * wizard_name = "Data Set Filtering Wizard";

static const char * finish_page_text = 
"When the apply button is hit, a new data set will be created "
"and placed into the study's tree, consisting of the appropriately "
"filtered data\n";

static const char * gaussian_filter_text = 
"The Gaussian filter is an effective smoothing filter";


static const char * median_3d_filter_text = 
"Median filter work relatively well at preserving edges while\n"
"removing speckle noise.\n"
"\n"
"This filter is the 3D median filter, so the neighborhood used for\n"
"determining the median will be KSxKSxKS, KS=kernel size";

static const char * median_linear_filter_text = 
"Median filters work relatively well at preserving edges while\n"
"removing speckle noise.\n"
"\n"
"This filter is the 1D median filter, so the neighboorhood used for\n"
"determining the median will be of the given kernel size, and the\n"
"data set will be filtered 3x (once for each direction).";


typedef enum {
  PICK_FILTER_PAGE,
  GAUSSIAN_FILTER_PAGE,
  MEDIAN_LINEAR_FILTER_PAGE,
  MEDIAN_3D_FILTER_PAGE,
  OUTPUT_PAGE,
  NUM_PAGES
} which_page_t;

/* data structures */
typedef struct tb_filter_t {
  GtkWidget * dialog;

  AmitkFilter filter;
  gint kernel_size;
  amide_real_t fwhm;

  AmitkDataSet * data_set;
  AmitkStudy * study;

  GtkWidget * table[NUM_PAGES];
  GtkWidget * page[NUM_PAGES];
  GtkWidget * progress_dialog;

  guint reference_count;
} tb_filter_t;


static gboolean next_page_cb(GtkWidget * page, gpointer *druid, gpointer data);
static gboolean back_page_cb(GtkWidget * page, gpointer *druid, gpointer data);
static void prepare_page_cb(GtkWidget * page, gpointer * druid, gpointer data);

static void filter_cb(GtkWidget * widget, gpointer data);
static void kernel_size_spinner_cb(GtkSpinButton * spin_button, gpointer data);
static void fwhm_spinner_cb(GtkSpinButton * spin_button, gpointer data);

static void finish_cb(GtkWidget* widget, gpointer druid, gpointer data);
static void cancel_cb(GtkWidget* widget, gpointer data);
static gboolean delete_event(GtkWidget * widget, GdkEvent * event, gpointer data);

static tb_filter_t * tb_filter_free(tb_filter_t * tb_filter);
static tb_filter_t * tb_filter_init(void);




static gboolean next_page_cb(GtkWidget * page, gpointer *druid, gpointer data) {

  tb_filter_t * tb_filter = data;
  which_page_t which_page;

  which_page = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(page), "which_page"));

  switch(which_page) {
  case PICK_FILTER_PAGE:
    gnome_druid_set_page(GNOME_DRUID(druid), 
			 GNOME_DRUID_PAGE(tb_filter->page[GAUSSIAN_FILTER_PAGE+tb_filter->filter]));

    break;
  default:
    gnome_druid_set_page(GNOME_DRUID(druid), 
			 GNOME_DRUID_PAGE(tb_filter->page[OUTPUT_PAGE]));
    break;
  }

  return TRUE;
}

static gboolean back_page_cb(GtkWidget * page, gpointer *druid, gpointer data) {

  tb_filter_t * tb_filter = data;
  which_page_t which_page;

  which_page = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(page), "which_page"));

  switch(which_page) {
  case OUTPUT_PAGE:
    gnome_druid_set_page(GNOME_DRUID(druid), 
			 GNOME_DRUID_PAGE(tb_filter->page[GAUSSIAN_FILTER_PAGE+tb_filter->filter]));
    break;
  default:
    gnome_druid_set_page(GNOME_DRUID(druid), 
			 GNOME_DRUID_PAGE(tb_filter->page[PICK_FILTER_PAGE]));
    break;
  }

  return TRUE;
}


static void prepare_page_cb(GtkWidget * page, gpointer * druid, gpointer data) {
 
  tb_filter_t * tb_filter = data;
  which_page_t which_page;
  GtkWidget * label;
  GtkWidget * spin_button;
  gint table_row;
  gint table_column;
  AmitkFilter i_filter;
  GtkWidget * table;
  GtkWidget * option_menu;
  GtkWidget * menu;
  GtkWidget * menuitem;

  which_page = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(page), "which_page"));


  /* -------------- set the buttons correctly ---------------- */
  switch(which_page) {
  case PICK_FILTER_PAGE:
    gnome_druid_set_buttons_sensitive(GNOME_DRUID(druid), FALSE, TRUE, TRUE, TRUE);
    break;
  default:
    gnome_druid_set_buttons_sensitive(GNOME_DRUID(druid), TRUE, TRUE, TRUE, TRUE);
    break;
  }

  /* --- create the widgets for the pages as needed */
  if (tb_filter->table[which_page] == NULL) {
    table = gtk_table_new(3,3,FALSE);
    gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(page)->vbox), 
		       table, TRUE, TRUE, 5);
    tb_filter->table[which_page]=table;
    
    table_row=0;
    table_column=0;
      
    switch(which_page) {
    case PICK_FILTER_PAGE:
      label = gtk_label_new("Which Filter");
      gtk_table_attach(GTK_TABLE(table), label, 
		       table_column,table_column+1, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);

      option_menu = gtk_option_menu_new();
      menu = gtk_menu_new();

      for (i_filter=0; i_filter<AMITK_FILTER_NUM; i_filter++) {
	menuitem = gtk_menu_item_new_with_label(amitk_filter_get_name(i_filter));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
      }

      gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
      g_signal_connect(G_OBJECT(option_menu), "changed", G_CALLBACK(filter_cb), tb_filter);
      gtk_table_attach(GTK_TABLE(table), option_menu, 
		       table_column+1, table_column+2, table_row, table_row+1,
		       FALSE, FALSE, X_PADDING, Y_PADDING);
      gtk_widget_show_all(option_menu);
      table_row++;
      
      break;
    case GAUSSIAN_FILTER_PAGE:
      tb_filter->kernel_size = DEFAULT_GAUSSIAN_FILTER_SIZE;

      label = gtk_label_new(gaussian_filter_text);
      gtk_table_attach(GTK_TABLE(table), label, 
		       table_column,table_column+2, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);
      table_row++;


      /* the kernel selection */
      label = gtk_label_new("Kernel Size");
      gtk_table_attach(GTK_TABLE(table), label, 
		       table_column,table_column+1, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);
	
      spin_button =  gtk_spin_button_new_with_range(MIN_FIR_FILTER_SIZE, 
						    MAX_FIR_FILTER_SIZE,2);
      gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_button),0);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), 
				tb_filter->kernel_size);
      g_signal_connect(G_OBJECT(spin_button), "value_changed",  
      			 G_CALLBACK(kernel_size_spinner_cb), tb_filter);
      gtk_table_attach(GTK_TABLE(table), spin_button, 
			 table_column+1,table_column+2, table_row,table_row+1,
			 FALSE,FALSE, X_PADDING, Y_PADDING);
      table_row++;

      label = gtk_label_new("FWHM (mm)");
      gtk_table_attach(GTK_TABLE(table), label, 
		       table_column,table_column+1, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);
	
      spin_button =  gtk_spin_button_new_with_range(MIN_FWHM, MAX_FWHM,0.2);
      gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_button),3);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), tb_filter->fwhm);
      g_signal_connect(G_OBJECT(spin_button), "value_changed",  
      			 G_CALLBACK(fwhm_spinner_cb), tb_filter);
      gtk_table_attach(GTK_TABLE(table), spin_button, 
			 table_column+1,table_column+2, table_row,table_row+1,
			 FALSE,FALSE, X_PADDING, Y_PADDING);
      break;
    case MEDIAN_3D_FILTER_PAGE:
    case MEDIAN_LINEAR_FILTER_PAGE:
      tb_filter->kernel_size = DEFAULT_MEDIAN_FILTER_SIZE;

      if (which_page == MEDIAN_3D_FILTER_PAGE)
	label = gtk_label_new(median_3d_filter_text);
      else
	label = gtk_label_new(median_linear_filter_text);
      gtk_table_attach(GTK_TABLE(table), label, 
		       table_column,table_column+2, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);
      table_row++;

      /* the kernel selection */
      label = gtk_label_new("Kernel Size");
      gtk_table_attach(GTK_TABLE(table), label, 
		       table_column,table_column+1, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);
	
      spin_button =  gtk_spin_button_new_with_range(MIN_NONLINEAR_FILTER_SIZE, 
						    MAX_NONLINEAR_FILTER_SIZE,2);
      gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin_button),0);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), 
				tb_filter->kernel_size);
      g_signal_connect(G_OBJECT(spin_button), "value_changed",  
      			 G_CALLBACK(kernel_size_spinner_cb), tb_filter);
      gtk_table_attach(GTK_TABLE(table), spin_button, 
			 table_column+1,table_column+2, table_row,table_row+1,
			 FALSE,FALSE, X_PADDING, Y_PADDING);
      table_row++;
      break;
    case OUTPUT_PAGE:
    default:
      table = NULL;
      g_warning("unhandled case in %s at line %d\n", __FILE__, __LINE__);
      break;
    }
    gtk_widget_show_all(table);
  }

}

/* function to change the color table */
static void filter_cb(GtkWidget * widget, gpointer data) {

  tb_filter_t * tb_filter = data;
  tb_filter->filter = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));
  return;
}

static void kernel_size_spinner_cb(GtkSpinButton * spin_button, gpointer data) {

  tb_filter_t * tb_filter = data;
  gint int_value;

  int_value = gtk_spin_button_get_value_as_int(spin_button);

  if (!(int_value & 0x1)) {
    int_value++; /* make it odd */
  }

  tb_filter->kernel_size = int_value;
  g_signal_handlers_block_by_func(G_OBJECT(spin_button),
				  G_CALLBACK(kernel_size_spinner_cb), tb_filter);
  gtk_spin_button_set_value(spin_button, tb_filter->kernel_size); 
  g_signal_handlers_unblock_by_func(G_OBJECT(spin_button),
				    G_CALLBACK(kernel_size_spinner_cb), tb_filter);

  return;
}

static void fwhm_spinner_cb(GtkSpinButton * spin_button, gpointer data) {

  tb_filter_t * tb_filter = data;
  amide_real_t value;

  value = gtk_spin_button_get_value(spin_button);

  if (value < MIN_FWHM)
    value = MIN_FWHM;
  else if (value > MAX_FWHM) 
    value = MAX_FWHM;

  tb_filter->fwhm = value;
  g_signal_handlers_block_by_func(G_OBJECT(spin_button),
				  G_CALLBACK(fwhm_spinner_cb), tb_filter);
  gtk_spin_button_set_value(spin_button, tb_filter->fwhm);
  g_signal_handlers_unblock_by_func(G_OBJECT(spin_button),
				    G_CALLBACK(fwhm_spinner_cb), tb_filter);

  return;
}


/* function called when the finish button is hit */
static void finish_cb(GtkWidget* widget, gpointer druid, gpointer data) {

  tb_filter_t * tb_filter = data;
  AmitkDataSet * filtered;

  /* generate the new data set */
  filtered = amitk_data_set_get_filtered(tb_filter->data_set, 
  					 tb_filter->filter,
  					 tb_filter->kernel_size,
  					 tb_filter->fwhm,
					 amitk_progress_dialog_update,
					 tb_filter->progress_dialog);

  /* and add the new data set to the study */
  amitk_object_add_child(AMITK_OBJECT(tb_filter->study), AMITK_OBJECT(filtered)); /* this adds a reference to the data set*/
  g_object_unref(filtered); /* so remove a reference */

  /* close the dialog box */
  cancel_cb(widget, data);

  return;
}




/* function called to cancel the dialog */
static void cancel_cb(GtkWidget* widget, gpointer data) {

  tb_filter_t * tb_filter = data;
  GtkWidget * dialog = tb_filter->dialog;
  gboolean return_val;

  /* run the delete event function */
  g_signal_emit_by_name(G_OBJECT(dialog), "delete_event", NULL, &return_val);
  if (!return_val) gtk_widget_destroy(dialog);

  return;
}

/* function called on a delete event */
static gboolean delete_event(GtkWidget * widget, GdkEvent * event, gpointer data) {

  tb_filter_t * tb_filter = data;

  /* trash collection */
  tb_filter = tb_filter_free(tb_filter);

  return FALSE;
}






static tb_filter_t * tb_filter_free(tb_filter_t * tb_filter) {

  gboolean return_val;

  /* sanity checks */
  g_return_val_if_fail(tb_filter != NULL, NULL);
  g_return_val_if_fail(tb_filter->reference_count > 0, NULL);

  /* remove a reference count */
  tb_filter->reference_count--;

  /* things to do if we've removed all reference's */
  if (tb_filter->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing tb_filter\n");
#endif

    if (tb_filter->data_set != NULL) {
      g_object_unref(tb_filter->data_set);
      tb_filter->data_set = NULL;
    }

    if (tb_filter->study != NULL) {
      g_object_unref(tb_filter->study);
      tb_filter->study = NULL;
    }

    if (tb_filter->progress_dialog != NULL) {
      g_signal_emit_by_name(G_OBJECT(tb_filter->progress_dialog), "delete_event", NULL, &return_val);
      tb_filter->progress_dialog = NULL;
    }
    
    g_free(tb_filter);
    tb_filter = NULL;
  }

  return tb_filter;
}

static tb_filter_t * tb_filter_init(void) {

  which_page_t i_page;
  tb_filter_t * tb_filter;

  /* alloc space for the data structure for passing ui info */
  if ((tb_filter = g_try_new(tb_filter_t,1)) == NULL) {
    g_warning("couldn't allocate space for tb_filter_t");
    return NULL;
  }

  tb_filter->reference_count = 1;
  tb_filter->filter = DEFAULT_FILTER;
  tb_filter->dialog = NULL;
  tb_filter->study = NULL;
  tb_filter->kernel_size=3;
  tb_filter->fwhm = 1.0;
  for (i_page=0; i_page<NUM_PAGES; i_page++)
    tb_filter->table[i_page] = NULL;


  return tb_filter;
}


void tb_filter(AmitkStudy * study, AmitkDataSet * active_ds) {

  tb_filter_t * tb_filter;
  GdkPixbuf * logo;
  GtkWidget * druid;
  which_page_t i_page;

  if (active_ds == NULL) {
    g_warning("No data set is currently marked as active");
    return;
  }
  
  logo = gdk_pixbuf_new_from_xpm_data(amide_logo_xpm);

  tb_filter = tb_filter_init();
  tb_filter->study = g_object_ref(study);
  tb_filter->data_set = g_object_ref(active_ds);

  /* take a guess at a good fwhm */
  tb_filter->fwhm = point_min_dim(AMITK_DATA_SET_VOXEL_SIZE(tb_filter->data_set));

  tb_filter->dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(G_OBJECT(tb_filter->dialog), "delete_event", G_CALLBACK(delete_event), tb_filter);

  tb_filter->progress_dialog = amitk_progress_dialog_new(GTK_WINDOW(tb_filter->dialog));

  druid = gnome_druid_new();
  gtk_container_add(GTK_CONTAINER(tb_filter->dialog), druid);
  g_signal_connect(G_OBJECT(druid), "cancel", G_CALLBACK(cancel_cb), tb_filter);


  for (i_page=PICK_FILTER_PAGE; i_page<OUTPUT_PAGE; i_page++) {
    tb_filter->page[i_page] = gnome_druid_page_standard_new_with_vals(wizard_name,logo,NULL);
    g_object_set_data(G_OBJECT(tb_filter->page[i_page]),"which_page", GINT_TO_POINTER(i_page));
    
    /* note, the _connect_after is a workaround, it should just be _connect */
    /* the problem, is gnome_druid currently overwrites button sensitivity in it's own prepare routine*/
    g_signal_connect_after(G_OBJECT(tb_filter->page[i_page]), "prepare", 
			   G_CALLBACK(prepare_page_cb), tb_filter);
    g_signal_connect(G_OBJECT(tb_filter->page[i_page]), "next", G_CALLBACK(next_page_cb), tb_filter);
    g_signal_connect(G_OBJECT(tb_filter->page[i_page]), "back", G_CALLBACK(back_page_cb), tb_filter);
    gnome_druid_append_page(GNOME_DRUID(druid), GNOME_DRUID_PAGE(tb_filter->page[i_page]));
  }

    
  /* ----------------  conclusion page ---------------------------------- */
  tb_filter->page[OUTPUT_PAGE] = 
    gnome_druid_page_edge_new_with_vals(GNOME_EDGE_FINISH, TRUE,
					wizard_name,finish_page_text, logo,NULL, NULL);
  g_object_set_data(G_OBJECT(tb_filter->page[i_page]),"which_page", GINT_TO_POINTER(i_page));
  g_signal_connect(G_OBJECT(tb_filter->page[OUTPUT_PAGE]), "finish", G_CALLBACK(finish_cb), tb_filter);
  g_signal_connect(G_OBJECT(tb_filter->page[OUTPUT_PAGE]), "back", G_CALLBACK(back_page_cb), tb_filter);
  gnome_druid_append_page(GNOME_DRUID(druid),  GNOME_DRUID_PAGE(tb_filter->page[OUTPUT_PAGE]));

  g_object_unref(logo);
  gtk_widget_show_all(tb_filter->dialog);

  return;
}











