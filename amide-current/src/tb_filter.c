/* tb_filter.c
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


#include "amide_config.h"
#include "amide.h"
#include "amitk_data_set.h"
#include "amitk_progress_dialog.h"
#include "tb_filter.h"


#define LABEL_WIDTH 375

#define MIN_FIR_FILTER_SIZE 7
#define MAX_FIR_FILTER_SIZE 31
#define MIN_NONLINEAR_FILTER_SIZE 3
#define MAX_NONLINEAR_FILTER_SIZE 11
#define DEFAULT_GAUSSIAN_FILTER_SIZE 15
#define DEFAULT_MEDIAN_FILTER_SIZE 3

#define MAX_FWHM 100.0 /* mm */
#define MIN_FWHM 0.0 /* mm */

static const char * wizard_name = N_("Data Set Filtering Wizard");

static const char * finish_page_text = 
N_("When the apply button is hit, a new data set will be created "
   "and placed into the study's tree, consisting of the appropriately "
   "filtered data\n");

#ifdef AMIDE_LIBGSL_SUPPORT
static const char * gaussian_filter_text = 
N_("The Gaussian filter is an effective smoothing filter");
#endif


static const char * median_3d_filter_text = 
N_("Median filter work relatively well at preserving edges while\n"
   "removing speckle noise.\n"
   "\n"
   "This filter is the 3D median filter, so the neighborhood used for\n"
   "determining the median will be KSxKSxKS, KS=kernel size");

static const char * median_linear_filter_text = 
N_("Median filters work relatively well at preserving edges while\n"
   "removing speckle noise.\n"
   "\n"
   "This filter is the 1D median filter, so the neighboorhood used for\n"
   "determining the median will be of the given kernel size, and the\n"
   "data set will be filtered 3x (once for each direction).");

#ifndef AMIDE_LIBGSL_SUPPORT
static const char *no_libgsl_text =
N_("This filter requires support from the GNU Scientific Library (GSL).\n"
   "This version of AMIDE has not been compiled with GSL support enabled.");
#endif


typedef enum {
  PICK_FILTER_PAGE,
  GAUSSIAN_FILTER_PAGE,
  MEDIAN_LINEAR_FILTER_PAGE,
  MEDIAN_3D_FILTER_PAGE,
  CONCLUSION_PAGE,
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

  GtkWidget * page[NUM_PAGES];
  GtkWidget * progress_dialog;

  guint reference_count;
} tb_filter_t;


static void filter_cb(GtkWidget * widget, gpointer data);
static void kernel_size_spinner_cb(GtkSpinButton * spin_button, gpointer data);
static void fwhm_spinner_cb(GtkSpinButton * spin_button, gpointer data);

static void apply_cb(GtkAssistant * assistant, gpointer data);
static void close_cb(GtkAssistant * assistant, gpointer data);
static gint forward_page_function (gint current_page, gpointer data);

static tb_filter_t * tb_filter_free(tb_filter_t * tb_filter);
static tb_filter_t * tb_filter_init(void);

static GtkWidget * create_page(tb_filter_t * tb_filter, which_page_t i_page);



/* function to change the color table */
static void filter_cb(GtkWidget * widget, gpointer data) {

  tb_filter_t * tb_filter = data;
  tb_filter->filter = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
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
static void apply_cb(GtkAssistant * assistant, gpointer data) {

  tb_filter_t * tb_filter = data;
  AmitkDataSet * filtered;

  /* disable the buttons */
  gtk_widget_set_sensitive(GTK_WIDGET(assistant), FALSE);

  /* generate the new data set */
  filtered = amitk_data_set_get_filtered(tb_filter->data_set, 
  					 tb_filter->filter,
  					 tb_filter->kernel_size,
  					 tb_filter->fwhm,
					 amitk_progress_dialog_update,
					 tb_filter->progress_dialog);

  if (filtered != NULL) {
    /* and add the new data set to the study */
    amitk_object_add_child(AMITK_OBJECT(tb_filter->study), AMITK_OBJECT(filtered)); /* this adds a reference to the data set*/
    amitk_object_unref(filtered); /* so remove a reference */
  } else 
    g_warning("Failed to generate filtered data set");

  return;
}


/* function called to cancel the dialog */
static void close_cb(GtkAssistant * assistant, gpointer data) {

  tb_filter_t * tb_filter = data;
  GtkWidget * dialog = tb_filter->dialog;

  tb_filter = tb_filter_free(tb_filter); /* trash collection */
  gtk_widget_destroy(dialog);

  return;
}

static gint forward_page_function (gint current_page, gpointer data) {

  tb_filter_t * tb_filter=data;

  switch(current_page) {
  case PICK_FILTER_PAGE:
    return GAUSSIAN_FILTER_PAGE+tb_filter->filter;
    break;
  case GAUSSIAN_FILTER_PAGE:
  case MEDIAN_LINEAR_FILTER_PAGE:
  case MEDIAN_3D_FILTER_PAGE:
    return CONCLUSION_PAGE;
    break;
  default:
    return current_page+1;
    break;
  }
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
      amitk_object_unref(tb_filter->data_set);
      tb_filter->data_set = NULL;
    }

    if (tb_filter->study != NULL) {
      amitk_object_unref(tb_filter->study);
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

  tb_filter_t * tb_filter;

  /* alloc space for the data structure for passing ui info */
  if ((tb_filter = g_try_new(tb_filter_t,1)) == NULL) {
    g_warning(_("couldn't allocate memory space for tb_filter_t"));
    return NULL;
  }

  tb_filter->reference_count = 1;
  tb_filter->filter = AMITK_FILTER_GAUSSIAN; /* default filter */
  tb_filter->dialog = NULL;
  tb_filter->study = NULL;
  tb_filter->kernel_size=3;
  tb_filter->fwhm = 1.0;

  return tb_filter;
}



static GtkWidget * create_page(tb_filter_t * tb_filter, which_page_t i_page) {

  GtkWidget * label;
  GtkWidget * spin_button;
  gint table_row;
  gint table_column;
  AmitkFilter i_filter;
  GtkWidget * table;
  GtkWidget * menu;

  table = gtk_table_new(3,3,FALSE);
    
  table_row=0;
  table_column=0;
      
  switch(i_page) {
  case PICK_FILTER_PAGE:
    label = gtk_label_new(_("Which Filter"));
    gtk_table_attach(GTK_TABLE(table), label, 
		     table_column,table_column+1, table_row,table_row+1,
		     FALSE,FALSE, X_PADDING, Y_PADDING);
    
    menu = gtk_combo_box_new_text();
    for (i_filter=0; i_filter<AMITK_FILTER_NUM; i_filter++) 
      gtk_combo_box_append_text(GTK_COMBO_BOX(menu), amitk_filter_get_name(i_filter));
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu), tb_filter->filter);
    g_signal_connect(G_OBJECT(menu), "changed", G_CALLBACK(filter_cb), tb_filter);
    gtk_table_attach(GTK_TABLE(table), menu, 
		     table_column+1, table_column+2, table_row, table_row+1,
		     FALSE, FALSE, X_PADDING, Y_PADDING);
    gtk_widget_show_all(menu);
    table_row++;
    
    break;
  case GAUSSIAN_FILTER_PAGE:
#ifdef AMIDE_LIBGSL_SUPPORT
    tb_filter->kernel_size = DEFAULT_GAUSSIAN_FILTER_SIZE;
    
    label = gtk_label_new(_(gaussian_filter_text));
    gtk_table_attach(GTK_TABLE(table), label, 
		     table_column,table_column+2, table_row,table_row+1,
		     FALSE,FALSE, X_PADDING, Y_PADDING);
    table_row++;
    
    /* the kernel selection */
    label = gtk_label_new(_("Kernel Size"));
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
    
    label = gtk_label_new(_("FWHM (mm)"));
    gtk_table_attach(GTK_TABLE(table), label, 
		     table_column,table_column+1, table_row,table_row+1,
		     FALSE,FALSE, X_PADDING, Y_PADDING);
    
    spin_button =  gtk_spin_button_new_with_range(MIN_FWHM, MAX_FWHM,0.2);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_button), FALSE);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), tb_filter->fwhm);
    g_signal_connect(G_OBJECT(spin_button), "value_changed",  
		     G_CALLBACK(fwhm_spinner_cb), tb_filter);
    g_signal_connect(G_OBJECT(spin_button), "output",
		     G_CALLBACK(amitk_spin_button_scientific_output), NULL);
    gtk_table_attach(GTK_TABLE(table), spin_button, 
		     table_column+1,table_column+2, table_row,table_row+1,
		     FALSE,FALSE, X_PADDING, Y_PADDING);
#else /* no libgsl support */
    label = gtk_label_new(_(no_libgsl_text));
    gtk_table_attach(GTK_TABLE(table), label, 
		     table_column,table_column+2, table_row,table_row+1,
		     FALSE,FALSE, X_PADDING, Y_PADDING);
    table_row++;
#endif
    break;
  case MEDIAN_3D_FILTER_PAGE:
  case MEDIAN_LINEAR_FILTER_PAGE:
    tb_filter->kernel_size = DEFAULT_MEDIAN_FILTER_SIZE;
    
    label = gtk_label_new((i_page == MEDIAN_3D_FILTER_PAGE) ? _(median_3d_filter_text) : _(median_linear_filter_text));
    gtk_table_attach(GTK_TABLE(table), label, 
		     table_column,table_column+2, table_row,table_row+1,
		     FALSE,FALSE, X_PADDING, Y_PADDING);
    table_row++;
    
    /* the kernel selection */
    label = gtk_label_new(_("Kernel Size"));
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
  default:
    table = NULL;
    g_error("unhandled case in %s at line %d\n", __FILE__, __LINE__);
    break;
  }

  return table;
}



void tb_filter(AmitkStudy * study, AmitkDataSet * active_ds, GtkWindow * parent) {

  tb_filter_t * tb_filter;
  GdkPixbuf * logo;
  which_page_t i_page;

  if (active_ds == NULL) {
    g_warning(_("No data set is currently marked as active"));
    return;
  }
  
  tb_filter = tb_filter_init();
  tb_filter->study = amitk_object_ref(study);
  tb_filter->data_set = amitk_object_ref(active_ds);

  /* take a guess at a good fwhm */
  tb_filter->fwhm = point_min_dim(AMITK_DATA_SET_VOXEL_SIZE(tb_filter->data_set));

  tb_filter->dialog = gtk_assistant_new();
  gtk_window_set_transient_for(GTK_WINDOW(tb_filter->dialog), parent);
  gtk_window_set_destroy_with_parent(GTK_WINDOW(tb_filter->dialog), TRUE);
  g_signal_connect(G_OBJECT(tb_filter->dialog), "cancel", G_CALLBACK(close_cb), tb_filter);
  g_signal_connect(G_OBJECT(tb_filter->dialog), "close", G_CALLBACK(close_cb), tb_filter);
  g_signal_connect(G_OBJECT(tb_filter->dialog), "apply", G_CALLBACK(apply_cb), tb_filter);
  gtk_assistant_set_forward_page_func(GTK_ASSISTANT(tb_filter->dialog),
				      forward_page_function,
				      tb_filter, NULL);

  tb_filter->progress_dialog = amitk_progress_dialog_new(GTK_WINDOW(tb_filter->dialog));


  /* --------------intro page and the various filter pages ------------------- */
  for (i_page=PICK_FILTER_PAGE; i_page<CONCLUSION_PAGE; i_page++) {
    tb_filter->page[i_page] = create_page(tb_filter, i_page);
    gtk_assistant_append_page(GTK_ASSISTANT(tb_filter->dialog), tb_filter->page[i_page]);
  }
    

  /* ----------------  conclusion page ---------------------------------- */
  tb_filter->page[CONCLUSION_PAGE] = gtk_label_new(_(finish_page_text));
  gtk_widget_set_size_request(tb_filter->page[CONCLUSION_PAGE],LABEL_WIDTH, -1);
  gtk_label_set_line_wrap(GTK_LABEL(tb_filter->page[CONCLUSION_PAGE]), TRUE);
  gtk_assistant_append_page(GTK_ASSISTANT(tb_filter->dialog), tb_filter->page[CONCLUSION_PAGE]);
  gtk_assistant_set_page_type(GTK_ASSISTANT(tb_filter->dialog), tb_filter->page[CONCLUSION_PAGE],
			      GTK_ASSISTANT_PAGE_CONFIRM);


  /* things for all pages */
  logo = gtk_widget_render_icon(GTK_WIDGET(tb_filter->dialog), "amide_icon_logo", GTK_ICON_SIZE_DIALOG, 0);
  for (i_page=0; i_page<NUM_PAGES; i_page++) {
    gtk_assistant_set_page_header_image(GTK_ASSISTANT(tb_filter->dialog), tb_filter->page[i_page], logo);
    gtk_assistant_set_page_title(GTK_ASSISTANT(tb_filter->dialog), tb_filter->page[i_page], _(wizard_name));
    gtk_assistant_set_page_complete(GTK_ASSISTANT(tb_filter->dialog), tb_filter->page[i_page], TRUE); /* all pages have default values */
    g_object_set_data(G_OBJECT(tb_filter->page[i_page]),"which_page", GINT_TO_POINTER(i_page));
  }
  g_object_unref(logo);

#ifndef AMIDE_LIBGSL_SUPPORT
  gtk_assistant_set_page_complete(GTK_ASSISTANT(tb_filter->dialog), tb_filter->page[GAUSSIAN_FILTER_PAGE], FALSE);
#endif

  gtk_widget_show_all(tb_filter->dialog);

  return;
}











