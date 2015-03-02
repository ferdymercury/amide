/* tb_fads.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2003 Andy Loening
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
#include "amitk_progress_dialog.h"
#include "fads.h"
#include "tb_fads.h"
#include "pixmaps.h"
#include "ui_common.h"



#ifdef AMIDE_LIBGSL_SUPPORT

#define SPIN_BUTTON_X_SIZE 100
#define MAX_ITERATIONS 1e8
static const char * wizard_name = N_("Factor Analysis Wizard");



static const char *svd_page_text =
N_("This page allows the computation of the singular value decomposition "
   "for the data set in question.  These values can give you an idea of "
   "how many important factors the data set has."
   "\n\n"
   "This process can be extremely slow, so skip this page if you already "
   "know the answer.");

static const char * finish_page_text = 
N_("When the apply button is hit, the appropriate factor analysis data "
   "structures will be created, and placed underneath the given data set "
   "in the study tree\n");

static gchar * start_page_text = 
N_("Welcome to the factor analysis of dynamic structures wizard."
   "\n"
   "None of this code has been validated, and it's probably wrong,"
   "so use at your own risk");


static gchar * not_enough_frames_text =
N_("Welcome to the factor analysis of dynamic structures wizard."
   "\n"
   "This wizard only works with dynamic studies");


typedef enum {
  INTRO_PAGE,
  SVD_PAGE,
  FACTOR_CHOICE_PAGE,
  PARAMETERS_PAGE,
  OUTPUT_PAGE,
  NUM_PAGES
} which_page_t;

/* data structures */
typedef struct tb_fads_t {
  GtkWidget * dialog;

  AmitkDataSet * data_set;
  gint num_factors;
  gint max_iterations;
  gdouble stopping_criteria;
  gdouble beta;
  gdouble k12;
  gdouble k21;
  gdouble k23;
  gdouble k32;
  fads_type_t fads_type;
  fads_minimizer_algorithm_t algorithm;

  GtkWidget * table[NUM_PAGES];
  GtkWidget * page[NUM_PAGES];
  GtkWidget * progress_dialog;
  GtkWidget * algorithm_menu;
  GtkWidget * num_factors_spin;
  GtkWidget * num_iterations_spin;
  GtkWidget * stopping_criteria_spin;
  GtkWidget * beta_spin;
  GtkWidget * k12_spin;
  GtkWidget * k21_spin;
  GtkWidget * svd_tree;
  GtkWidget * blood_add_button;
  GtkWidget * blood_remove_button;
  GtkWidget * blood_tree;
  GtkTextBuffer * explanation_buffer;

  guint reference_count;
} tb_fads_t;



static void cancel_cb(GtkWidget* widget, gpointer data);
static gboolean delete_event(GtkWidget * widget, GdkEvent * event, gpointer data);

static tb_fads_t * tb_fads_free(tb_fads_t * tb_fads);
static tb_fads_t * tb_fads_init(void);

static void set_text(tb_fads_t * tb_fads);
static gchar * get_filename(tb_fads_t * tb_fads);

static void prepare_page_cb(GtkWidget * page, gpointer * druid, gpointer data);

static void fads_type_cb(GtkWidget * widget, gpointer data);
static void algorithm_cb(GtkWidget * widget, gpointer data);
static void svd_pressed_cb(GtkButton * button, gpointer data);
static void blood_cell_edited(GtkCellRendererText *cellrenderertext,
			      gchar *arg1, gchar *arg2,gpointer data);
static void add_blood_pressed_cb(GtkButton * button, gpointer data);
static void remove_blood_pressed_cb(GtkButton * button, gpointer data);
static void num_factors_spinner_cb(GtkSpinButton * spin_button, gpointer data);
static void max_iterations_spinner_cb(GtkSpinButton * spin_button, gpointer data);
static void stopping_criteria_spinner_cb(GtkSpinButton * spin_button, gpointer data);
static void beta_spinner_cb(GtkSpinButton * spin_button, gpointer data);
static void k12_spinner_cb(GtkSpinButton * spin_button, gpointer data);
static void k21_spinner_cb(GtkSpinButton * spin_button, gpointer data);

static void finish_cb(GtkWidget* widget, gpointer druid, gpointer data);


static void set_text(tb_fads_t * tb_fads) {

  gchar * temp_string;
  GtkTextIter iter;
  GdkPixbuf * pixbuf;

  /* the output page text */
  if (tb_fads->page[OUTPUT_PAGE] != NULL) {
    temp_string = g_strdup_printf(_("%s\nMethod Picked: %s"), 
				  finish_page_text, fads_type_explanation[tb_fads->fads_type]);
    gnome_druid_page_edge_set_text (GNOME_DRUID_PAGE_EDGE(tb_fads->page[OUTPUT_PAGE]),
				    temp_string);
    g_free(temp_string);
  }

  /* the factor analysis type picking page */
  if (tb_fads->explanation_buffer != NULL) {
    gtk_text_buffer_set_text (tb_fads->explanation_buffer, 
			      fads_type_explanation[tb_fads->fads_type], -1);

    if (fads_type_xpm[tb_fads->fads_type] != NULL) {
      gtk_text_buffer_get_end_iter(tb_fads->explanation_buffer, &iter);
      gtk_text_buffer_insert(tb_fads->explanation_buffer, &iter, "\n", -1);

      pixbuf = gdk_pixbuf_new_from_xpm_data(fads_type_xpm[tb_fads->fads_type]);
      gtk_text_buffer_get_end_iter(tb_fads->explanation_buffer, &iter);
      gtk_text_buffer_insert_pixbuf(tb_fads->explanation_buffer, &iter, pixbuf);
      g_object_unref(pixbuf);
    }
  }

  return;
}

/* function to save the generated roi statistics */
static gchar * get_filename(tb_fads_t * tb_fads) {
  
  GtkWidget * fs;
  gchar * analysis_name;
  gint response_id;
  gchar * save_filename;

  fs = gtk_file_selection_new(_("Filename for Factor Data"));

  /* take a guess at the filename */
  analysis_name = g_strdup_printf("%s_fads_analysis.csv",AMITK_OBJECT_NAME(tb_fads->data_set));
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(fs), analysis_name);
  g_free(analysis_name);

  /* run the dialog */
  response_id = gtk_dialog_run(GTK_DIALOG(fs));
  save_filename = g_strdup(ui_common_file_selection_get_name(fs));
  gtk_widget_destroy(GTK_WIDGET(fs));

  if (response_id == GTK_RESPONSE_OK)
    return save_filename;
  else
    return NULL;
}


static void prepare_page_cb(GtkWidget * page, gpointer * druid, gpointer data) {
 
  tb_fads_t * tb_fads = data;
  fads_type_t i_fads_type;
  fads_minimizer_algorithm_t i_algorithm;
  which_page_t which_page;
  GtkWidget * label;
  GtkWidget * button;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkListStore * store;
  gint table_row;
  gint max_table_row;
  GtkWidget * table;
  GtkWidget * scrolled;
  GtkWidget * option_menu;
  GtkWidget * menu;
  GtkWidget * menuitem;
  GtkWidget * view;
  GtkWidget * vseparator;
  GtkWidget * hseparator;
  GtkTextBuffer *buffer;
  gboolean num_factors;
  gboolean blood_entries;
  gboolean num_iterations;
  gboolean k_values;

  which_page = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(page), "which_page"));

  /* -------------- set the buttons correctly ---------------- */
  //  switch(which_page) {
  //  case PARAMETERS_PAGE
  //    gnome_druid_set_buttons_sensitive(GNOME_DRUID(druid), FALSE, TRUE, TRUE, TRUE);
  //    break;
  //  default:
  //    gnome_druid_set_buttons_sensitive(GNOME_DRUID(druid), TRUE, TRUE, TRUE, TRUE);
  //    break;
  //  }

  /* --- create the widgets for the pages as needed */
  if (tb_fads->table[which_page] == NULL) {
    table = gtk_table_new(8,6,FALSE);
    gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(page)->vbox), 
		       table, TRUE, TRUE, 5);
    tb_fads->table[which_page]=table;
    
    table_row=0;
      
    switch(which_page) {
    case SVD_PAGE:
      view = gtk_text_view_new ();
      buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
      gtk_text_buffer_set_text (buffer, svd_page_text, -1);
      gtk_table_attach(GTK_TABLE(table), view, 0,1, table_row,table_row+2,
			 FALSE,FALSE, X_PADDING, Y_PADDING);
      gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
      gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
      gtk_widget_set_size_request(view,300,-1);


      /* a separator for clarity */
      vseparator = gtk_vseparator_new();
      gtk_table_attach(GTK_TABLE(table), vseparator, 1,2,table_row, table_row+2,
		       0, GTK_FILL, X_PADDING, Y_PADDING);


      /* do I need to compute factors? */
      button = gtk_button_new_with_label(_("Compute Singular Values?"));
      g_signal_connect(G_OBJECT(button), "pressed", G_CALLBACK(svd_pressed_cb), tb_fads);
      gtk_table_attach(GTK_TABLE(table), button, 2,3, table_row,table_row+1,
			 FALSE,FALSE, X_PADDING, Y_PADDING);
      table_row++;

      /* the scroll widget which the list will go into */
      scrolled = gtk_scrolled_window_new(NULL,NULL);
      gtk_widget_set_size_request(scrolled,200,200);
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				     GTK_POLICY_AUTOMATIC,
				     GTK_POLICY_AUTOMATIC);
      gtk_table_attach(GTK_TABLE(table), scrolled, 2, 3, table_row, table_row+1, 
		       X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL, X_PADDING, Y_PADDING);

      /* the table itself */
      store = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_DOUBLE);
      tb_fads->svd_tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
      g_object_unref(store); /* above command adds a reference */

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes("singular entry", renderer,"text", 0, NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (tb_fads->svd_tree), column);

      renderer = gtk_cell_renderer_text_new ();
      column = gtk_tree_view_column_new_with_attributes("value", renderer,"text", 1, NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (tb_fads->svd_tree), column);

      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tb_fads->svd_tree));
      gtk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);
      gtk_container_add(GTK_CONTAINER(scrolled),tb_fads->svd_tree);
      table_row++;
      break;

    case FACTOR_CHOICE_PAGE:
      /* ask for the fads method to use */
      label = gtk_label_new(_("FADS Method:"));
      gtk_table_attach(GTK_TABLE(table), label, 0,1, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);

      option_menu = gtk_option_menu_new();
      menu = gtk_menu_new();
	
      for (i_fads_type = 0; i_fads_type < NUM_FADS_TYPES; i_fads_type++) {
	menuitem = gtk_menu_item_new_with_label(fads_type_name[i_fads_type]);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_show(menuitem);
      }

      
      gtk_option_menu_set_menu(GTK_OPTION_MENU(option_menu), menu);
      gtk_option_menu_set_history(GTK_OPTION_MENU(option_menu), tb_fads->fads_type);
      g_signal_connect(G_OBJECT(option_menu), "changed", G_CALLBACK(fads_type_cb), tb_fads);
      gtk_table_attach(GTK_TABLE(table), option_menu, 1,2, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);
      table_row++;


      hseparator = gtk_hseparator_new();
      gtk_table_attach(GTK_TABLE(table), hseparator, 0,2,table_row, table_row+1,
		       GTK_FILL, 0, X_PADDING, Y_PADDING);
      table_row++;


      /* the explanation */
      view = gtk_text_view_new ();
      tb_fads->explanation_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
      gtk_text_buffer_set_text (tb_fads->explanation_buffer, 
				fads_type_explanation[tb_fads->fads_type], -1);
      gtk_table_attach(GTK_TABLE(table), view, 0,2, table_row,table_row+1,
			 FALSE,FALSE, X_PADDING, Y_PADDING);
      gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
      gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
      gtk_widget_set_size_request(view,300,-1);
      table_row++;


      break;

    case PARAMETERS_PAGE:
      /* ask for the minimizer algorithm to use */
      label = gtk_label_new(_("Minimization Algorithm:"));
      gtk_table_attach(GTK_TABLE(table), label, 0,1, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);

      menu = gtk_menu_new();
	
      for (i_algorithm = 0; i_algorithm < NUM_FADS_MINIMIZERS; i_algorithm++) {
	menuitem = gtk_menu_item_new_with_label(fads_minimizer_algorithm_name[i_algorithm]);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_show(menuitem);
      }

      
      tb_fads->algorithm_menu = gtk_option_menu_new();
      gtk_option_menu_set_menu(GTK_OPTION_MENU(tb_fads->algorithm_menu), menu);
      gtk_option_menu_set_history(GTK_OPTION_MENU(tb_fads->algorithm_menu), tb_fads->algorithm);
      g_signal_connect(G_OBJECT(tb_fads->algorithm_menu), "changed", G_CALLBACK(algorithm_cb), tb_fads);
      gtk_table_attach(GTK_TABLE(table), tb_fads->algorithm_menu, 1,2, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);
      table_row++;


      /* max # of iterations */
      label = gtk_label_new(_("Max. # of iterations:"));
      gtk_table_attach(GTK_TABLE(table), label, 0,1, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);

      tb_fads->num_iterations_spin =  gtk_spin_button_new_with_range(1, MAX_ITERATIONS, 1);
      gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tb_fads->num_iterations_spin),0);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_fads->num_iterations_spin), tb_fads->max_iterations);
      gtk_widget_set_size_request(tb_fads->num_iterations_spin, SPIN_BUTTON_X_SIZE, -1);
      g_signal_connect(G_OBJECT(tb_fads->num_iterations_spin), "value_changed",  
		       G_CALLBACK(max_iterations_spinner_cb), tb_fads);
      gtk_table_attach(GTK_TABLE(table), tb_fads->num_iterations_spin, 1,2, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);
      table_row++;

      /* stopping criteria */
      label = gtk_label_new(_("Stopping Criteria:"));
      gtk_table_attach(GTK_TABLE(table), label, 0,1, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);

      tb_fads->stopping_criteria_spin = gtk_spin_button_new_with_range(EPSILON, 1.0, tb_fads->stopping_criteria);
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tb_fads->stopping_criteria_spin), FALSE);
      gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tb_fads->stopping_criteria_spin), 7);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_fads->stopping_criteria_spin), tb_fads->stopping_criteria);
      gtk_widget_set_size_request(tb_fads->stopping_criteria_spin, SPIN_BUTTON_X_SIZE, -1);
      g_signal_connect(G_OBJECT(tb_fads->stopping_criteria_spin), "value_changed",  
		       G_CALLBACK(stopping_criteria_spinner_cb), tb_fads);
      gtk_table_attach(GTK_TABLE(table), tb_fads->stopping_criteria_spin, 1,2, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);
      table_row++;

      /* stopping criteria */
      label = gtk_label_new(_("Beta:"));
      gtk_table_attach(GTK_TABLE(table), label, 0,1, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);

      tb_fads->beta_spin = gtk_spin_button_new_with_range(0.0, 1.0, 0.01);
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tb_fads->beta_spin), FALSE);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_fads->beta_spin), tb_fads->beta);
      gtk_widget_set_size_request(tb_fads->beta_spin, SPIN_BUTTON_X_SIZE, -1);
      g_signal_connect(G_OBJECT(tb_fads->beta_spin), "value_changed",  
		       G_CALLBACK(beta_spinner_cb), tb_fads);
      gtk_table_attach(GTK_TABLE(table), tb_fads->beta_spin, 1,2, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);
      table_row++;

      /* how many factors to solve for? */
      label = gtk_label_new(_("# of Factors to use"));
      gtk_table_attach(GTK_TABLE(table), label, 0,1, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);

      tb_fads->num_factors_spin =  gtk_spin_button_new_with_range(1, AMITK_DATA_SET_NUM_FRAMES(tb_fads->data_set), 1);
      gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tb_fads->num_factors_spin),0);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_fads->num_factors_spin), tb_fads->num_factors);
      gtk_widget_set_size_request(tb_fads->num_factors_spin, SPIN_BUTTON_X_SIZE, -1);
      g_signal_connect(G_OBJECT(tb_fads->num_factors_spin), "value_changed",  
		       G_CALLBACK(num_factors_spinner_cb), tb_fads);
      gtk_table_attach(GTK_TABLE(table), tb_fads->num_factors_spin, 1,2, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);
      table_row++;



      /* k12 criteria */
      label = gtk_label_new(_("initial k12 (1/s):"));
      gtk_table_attach(GTK_TABLE(table), label, 0,1, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);

      tb_fads->k12_spin = gtk_spin_button_new_with_range(0.0, G_MAXDOUBLE, 0.01);
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tb_fads->k12_spin), FALSE);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_fads->k12_spin), tb_fads->k12);
      gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tb_fads->k12_spin), 7);
      gtk_widget_set_size_request(tb_fads->k12_spin, SPIN_BUTTON_X_SIZE, -1);
      g_signal_connect(G_OBJECT(tb_fads->k12_spin), "value_changed",  
		       G_CALLBACK(k12_spinner_cb), tb_fads);
      gtk_table_attach(GTK_TABLE(table), tb_fads->k12_spin, 1,2, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);
      table_row++;

      /* k21 criteria */
      label = gtk_label_new(_("initial K21 (1/s):"));
      gtk_table_attach(GTK_TABLE(table), label, 0,1, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);

      tb_fads->k21_spin = gtk_spin_button_new_with_range(0.0, G_MAXDOUBLE, 0.01);
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tb_fads->k21_spin), FALSE);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_fads->k21_spin), tb_fads->k21);
      gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tb_fads->k21_spin), 7);
      gtk_widget_set_size_request(tb_fads->k21_spin, SPIN_BUTTON_X_SIZE, -1);
      g_signal_connect(G_OBJECT(tb_fads->k21_spin), "value_changed",  
		       G_CALLBACK(k21_spinner_cb), tb_fads);
      gtk_table_attach(GTK_TABLE(table), tb_fads->k21_spin, 1,2, table_row,table_row+1,
		       FALSE,FALSE, X_PADDING, Y_PADDING);
      // table_row++;


      max_table_row = table_row;
      table_row = 0;
      /* a separator for clarity */
      vseparator = gtk_vseparator_new();
      gtk_table_attach(GTK_TABLE(table), vseparator, 2,3,table_row, table_row+6,
		       0, GTK_FILL, X_PADDING, Y_PADDING);

      /* A table to add blood sample measurements */
      tb_fads->blood_add_button = gtk_button_new_with_label(_("Add Blood Sample"));
      g_signal_connect(G_OBJECT(tb_fads->blood_add_button), "pressed", 
		       G_CALLBACK(add_blood_pressed_cb), tb_fads);
      gtk_table_attach(GTK_TABLE(table), tb_fads->blood_add_button, 3,4, table_row,table_row+1,
			 FALSE,FALSE, X_PADDING, Y_PADDING);
      table_row++;

      /* the scroll widget which the list will go into */
      scrolled = gtk_scrolled_window_new(NULL,NULL);
      gtk_widget_set_size_request(scrolled,200,200);
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				     GTK_POLICY_AUTOMATIC,
				     GTK_POLICY_AUTOMATIC);
      gtk_table_attach(GTK_TABLE(table), scrolled, 3, 4, table_row, max_table_row, 
		       X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL, X_PADDING, Y_PADDING);

      /* the table itself */
      store = gtk_list_store_new(2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
      tb_fads->blood_tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
      g_object_unref(store); /* above command adds a reference */

      renderer = gtk_cell_renderer_text_new ();
      g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
      g_object_set_data(G_OBJECT(renderer),"column", GINT_TO_POINTER(0));
      g_signal_connect(G_OBJECT(renderer), "edited", G_CALLBACK(blood_cell_edited), tb_fads);
      column = gtk_tree_view_column_new_with_attributes("time pt (s)", renderer,"text", 0, NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (tb_fads->blood_tree), column);

      renderer = gtk_cell_renderer_text_new ();
      g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
      g_object_set_data(G_OBJECT(renderer),"column", GINT_TO_POINTER(1));
      g_signal_connect(G_OBJECT(renderer), "edited", G_CALLBACK(blood_cell_edited), tb_fads);
      column = gtk_tree_view_column_new_with_attributes("value", renderer,"text", 1,NULL);
      gtk_tree_view_append_column (GTK_TREE_VIEW (tb_fads->blood_tree), column);

      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tb_fads->blood_tree));
      gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
      gtk_container_add(GTK_CONTAINER(scrolled),tb_fads->blood_tree);
      table_row+=4;

      tb_fads->blood_remove_button = gtk_button_new_with_label(_("Remove Blood Sample"));
      g_signal_connect(G_OBJECT(tb_fads->blood_remove_button), "pressed", 
		       G_CALLBACK(remove_blood_pressed_cb), tb_fads);
      gtk_table_attach(GTK_TABLE(table), tb_fads->blood_remove_button, 3,4, table_row,table_row+1,
			 FALSE,FALSE, X_PADDING, Y_PADDING);
      table_row++;


      break;
    default:
      table = NULL;
      g_error("unhandled case in %s at line %d\n", __FILE__, __LINE__);
      break;
    }
    gtk_widget_show_all(table);
  }

  /* hide options we don't need */
  switch(which_page) {
  case PARAMETERS_PAGE:

    switch(tb_fads->fads_type) {
    case FADS_TYPE_PCA:
      k_values = FALSE;
      blood_entries = FALSE;
      num_factors = TRUE;
      num_iterations = FALSE;
      break;
    case FADS_TYPE_TWO_COMPARTMENT:
      k_values = TRUE;
      blood_entries = TRUE;
      num_factors = TRUE;
      num_iterations = TRUE;
      break;
    case FADS_TYPE_PLS:
      k_values = FALSE;
      blood_entries = TRUE;
      num_factors = TRUE;
      num_iterations = TRUE;
      break;
    default:
      g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
      k_values = FALSE;
      blood_entries = TRUE;
      num_factors = TRUE;
      num_iterations = TRUE;
      break;
    }

    gtk_widget_set_sensitive(tb_fads->blood_tree, blood_entries);
    gtk_widget_set_sensitive(tb_fads->blood_remove_button, blood_entries);
    gtk_widget_set_sensitive(tb_fads->blood_add_button, blood_entries);
    gtk_widget_set_sensitive(tb_fads->algorithm_menu, num_iterations);
    gtk_widget_set_sensitive(tb_fads->num_factors_spin, num_factors);
    gtk_widget_set_sensitive(tb_fads->num_iterations_spin, num_iterations);
    gtk_widget_set_sensitive(tb_fads->stopping_criteria_spin, num_iterations);
    gtk_widget_set_sensitive(tb_fads->beta_spin, num_iterations);
    gtk_widget_set_sensitive(tb_fads->k12_spin, k_values);
    gtk_widget_set_sensitive(tb_fads->k21_spin, k_values);

  default:
    break;
  }

}

static void svd_pressed_cb(GtkButton * button, gpointer data) {

  tb_fads_t * tb_fads = data;
  gdouble * factors = NULL;
  gint num_factors;
  gint i;
  GtkTreeIter iter;
  GtkTreeModel * model;
  
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tb_fads->svd_tree));
  gtk_list_store_clear(GTK_LIST_STORE(model));  /* make sure the list is clear */

  /* calculate factors */
  ui_common_place_cursor(UI_CURSOR_WAIT, tb_fads->table[PARAMETERS_PAGE]);
  fads_svd_factors(tb_fads->data_set, &num_factors, &factors);
  ui_common_remove_cursor(UI_CURSOR_WAIT, tb_fads->table[PARAMETERS_PAGE]);

  for (i=0; i<num_factors; i++) {
    gtk_list_store_append (GTK_LIST_STORE(model), &iter);  /* Acquire an iterator */
    gtk_list_store_set (GTK_LIST_STORE(model), &iter,0,i+1,1,factors[i],-1);
  }

  /* garbage collection */
  if (factors != NULL)
    g_free(factors);
  
  return;
}

static void blood_cell_edited(GtkCellRendererText *cellrenderertext,
			      gchar *arg1, gchar *arg2, gpointer data) {

  tb_fads_t * tb_fads = data;
  GtkTreePath * path;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gint error_val;
  gdouble value;
  gint row;
  gint column;
  gchar * temp_string;

  error_val = sscanf(arg2, "%lf", &value);
  if (error_val != 0) {
    error_val = sscanf(arg1, "%d", &row);

    column = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cellrenderertext), "column"));

    if (error_val != 0) {
      model = gtk_tree_view_get_model(GTK_TREE_VIEW(tb_fads->blood_tree));

      /* switch the following 3 lines with
	 path = gtk_tree_path_new_from_indices(row, -1);
	 when gtk 2.2 is ready */
      temp_string = g_strdup_printf("%d", row);
      path = gtk_tree_path_new_from_string(temp_string);
      g_free(temp_string);

      if (gtk_tree_model_get_iter(model, &iter, path)) {
	gtk_list_store_set (GTK_LIST_STORE(model), &iter,column,value, -1, -1);
      }
      gtk_tree_path_free(path);
    }
  }
  
  return;
}


static void add_blood_pressed_cb(GtkButton * button, gpointer data) {

  tb_fads_t * tb_fads = data;
  GtkTreeIter iter;
  GtkTreeModel * model;
  
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tb_fads->blood_tree));
  
  gtk_list_store_append (GTK_LIST_STORE(model), &iter);  /* Acquire an iterator */
  gtk_list_store_set (GTK_LIST_STORE(model), &iter,0,0.0,1,0.0,-1);

  return;
}

static void remove_blood_pressed_cb(GtkButton * button, gpointer data) {

  tb_fads_t * tb_fads = data;
  GtkTreeIter iter;
  GtkTreeSelection * selection;
  GtkTreeModel * model;
  
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tb_fads->blood_tree));

  if(gtk_tree_selection_get_selected(selection, &model, &iter)) {
    gtk_list_store_remove(GTK_LIST_STORE(model), &iter);  
  }

  return;
}



static void fads_type_cb(GtkWidget * widget, gpointer data) {
  tb_fads_t * tb_fads = data;
  tb_fads->fads_type = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));
  set_text(tb_fads);
  return;
}

static void algorithm_cb(GtkWidget * widget, gpointer data) {
  tb_fads_t * tb_fads = data;
  tb_fads->algorithm = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));
  set_text(tb_fads);
  return;
}

static void num_factors_spinner_cb(GtkSpinButton * spin_button, gpointer data) {
  tb_fads_t * tb_fads = data;
  tb_fads->num_factors = gtk_spin_button_get_value_as_int(spin_button);
  return;
}

static void max_iterations_spinner_cb(GtkSpinButton * spin_button, gpointer data) {
  tb_fads_t * tb_fads = data;
  tb_fads->max_iterations = gtk_spin_button_get_value_as_int(spin_button);
  return;
}

static void stopping_criteria_spinner_cb(GtkSpinButton * spin_button, gpointer data) {
  tb_fads_t * tb_fads = data;
  tb_fads->stopping_criteria = gtk_spin_button_get_value(spin_button);
  return;
}

static void beta_spinner_cb(GtkSpinButton * spin_button, gpointer data) {
  tb_fads_t * tb_fads = data;
  tb_fads->beta = gtk_spin_button_get_value(spin_button);
  return;
}

static void k12_spinner_cb(GtkSpinButton * spin_button, gpointer data) {
  tb_fads_t * tb_fads = data;
  tb_fads->k12 = gtk_spin_button_get_value(spin_button);
  return;
}

static void k21_spinner_cb(GtkSpinButton * spin_button, gpointer data) {
  tb_fads_t * tb_fads = data;
  tb_fads->k21 = gtk_spin_button_get_value(spin_button);
  return;
}

/* function called when the finish button is hit */
static void finish_cb(GtkWidget* widget, gpointer druid, gpointer data) {

  tb_fads_t * tb_fads = data;
  gchar * output_filename;
  gint num=0;
  gint i;
  gint * frames=NULL;
  amide_data_t * vals=NULL;
  amide_data_t value;
  amide_time_t time;
  GtkTreeModel *model;
  GtkTreeIter iter;
  
    

  output_filename = get_filename(tb_fads);
  if (output_filename == NULL) return; /* no filename, no go */

  /* get the blood values */
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tb_fads->blood_tree));
  num = gtk_tree_model_iter_n_children(model, NULL);

  if (num > 0) {
    if ((frames = g_try_new(gint, num)) == NULL) {
      g_warning(_("failed malloc for frames array"));
      return;
    }
    
    if ((vals = g_try_new(amide_data_t, num)) == NULL) {
      g_warning(_("failed malloc for vals array"));
      g_free(frames);
      return;
    }

    for (i=0; i<num ;i++) {
      if (i==0)
	gtk_tree_model_get_iter_first(model,&iter);
      else
	gtk_tree_model_iter_next(model,&iter);

      gtk_tree_model_get(model, &iter, 0, &time, 1, &value, -1);

      vals[i] = value;
      frames[i] = amitk_data_set_get_frame(tb_fads->data_set, time);

    }

#if AMIDE_DEBUG
      g_print("blood values: frame\tval\n");
      for (i=0; i<num ;i++) 
	g_print("                 %d\t%g\n", frames[i], vals[i]);
#endif
  }

  ui_common_place_cursor(UI_CURSOR_WAIT, tb_fads->table[PARAMETERS_PAGE]);
  switch(tb_fads->fads_type) {
  case FADS_TYPE_PCA:
    fads_pca(tb_fads->data_set, tb_fads->num_factors, output_filename,
	     amitk_progress_dialog_update, tb_fads->progress_dialog);
    break;
  case FADS_TYPE_PLS:
    fads_pls(tb_fads->data_set, tb_fads->num_factors, tb_fads->algorithm, tb_fads->max_iterations,
	     tb_fads->stopping_criteria,
	     tb_fads->beta, output_filename, num, frames, vals, 
	     amitk_progress_dialog_update, tb_fads->progress_dialog);
    break;
  case FADS_TYPE_TWO_COMPARTMENT:
    fads_two_comp(tb_fads->data_set, tb_fads->algorithm, tb_fads->max_iterations, 
		  tb_fads->num_factors-1, tb_fads->k12, tb_fads->k21, tb_fads->stopping_criteria,
		  output_filename, num, frames, vals, amitk_progress_dialog_update, tb_fads->progress_dialog);
    break;
  default:
    g_error("fads type %d not defined", tb_fads->fads_type);
    break;
  }
  ui_common_remove_cursor(UI_CURSOR_WAIT, tb_fads->table[PARAMETERS_PAGE]);

  if (frames != NULL) {
    g_free(frames);
    frames = NULL;
  }

  if (vals != NULL) {
    g_free(vals);
    vals = NULL;
  }

  /* close the dialog box */
  cancel_cb(widget, data);

  return;
}




/* function called to cancel the dialog */
static void cancel_cb(GtkWidget* widget, gpointer data) {

  tb_fads_t * tb_fads = data;
  GtkWidget * dialog = tb_fads->dialog;
  gboolean return_val;

  /* run the delete event function */
  g_signal_emit_by_name(G_OBJECT(dialog), "delete_event", NULL, &return_val);
  if (!return_val) gtk_widget_destroy(dialog);

  return;
}

/* function called on a delete event */
static gboolean delete_event(GtkWidget * widget, GdkEvent * event, gpointer data) {

  tb_fads_t * tb_fads = data;

  /* trash collection */
  tb_fads = tb_fads_free(tb_fads);

  return FALSE;
}


static tb_fads_t * tb_fads_free(tb_fads_t * tb_fads) {

  gboolean return_val;

  /* sanity checks */
  g_return_val_if_fail(tb_fads != NULL, NULL);
  g_return_val_if_fail(tb_fads->reference_count > 0, NULL);

  /* remove a reference count */
  tb_fads->reference_count--;

  /* things to do if we've removed all reference's */
  if (tb_fads->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing tb_fads\n");
#endif

    if (tb_fads->data_set != NULL) {
      g_object_unref(tb_fads->data_set);
      tb_fads->data_set = NULL;
    }

    if (tb_fads->progress_dialog != NULL) {
      g_signal_emit_by_name(G_OBJECT(tb_fads->progress_dialog), "delete_event", NULL, &return_val);
      tb_fads->progress_dialog = NULL;
    }
    
    g_free(tb_fads);
    tb_fads = NULL;
  }

  return tb_fads;
}

static tb_fads_t * tb_fads_init(void) {

  which_page_t i_page;
  tb_fads_t * tb_fads;

  /* alloc space for the data structure for passing ui info */
  if ((tb_fads = g_try_new(tb_fads_t,1)) == NULL) {
    g_warning(_("couldn't allocate space for tb_fads_t"));
    return NULL;
  }

  tb_fads->reference_count = 1;
  tb_fads->dialog = NULL;
  tb_fads->data_set = NULL;
  tb_fads->num_factors = 2;
  tb_fads->max_iterations = 1e6;
  tb_fads->stopping_criteria = 1e-2;
  tb_fads->beta = 0.0;
  tb_fads->k12 = 0.01;
  tb_fads->k21 = 0.1;
  tb_fads->k23 = 0.01;
  tb_fads->k32 = 0.1;
  tb_fads->fads_type = FADS_TYPE_PCA;
  //  tb_fads->algorithm = FADS_MINIMIZER_VECTOR_BFGS;
  tb_fads->algorithm = FADS_MINIMIZER_CONJUGATE_PR;
  tb_fads->explanation_buffer = NULL;
  for (i_page=0; i_page<NUM_PAGES; i_page++)
    tb_fads->table[i_page] = NULL;


  return tb_fads;
}





void tb_fads(AmitkDataSet * active_ds) {

  tb_fads_t * tb_fads;
  GdkPixbuf * logo;
  GtkWidget * druid;
  which_page_t i_page;


  g_return_if_fail(AMITK_IS_DATA_SET(active_ds));
  
  logo = gdk_pixbuf_new_from_xpm_data(amide_logo_xpm);

  tb_fads = tb_fads_init();
  tb_fads->data_set = g_object_ref(active_ds);

  tb_fads->dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(G_OBJECT(tb_fads->dialog), "delete_event", G_CALLBACK(delete_event), tb_fads);

  tb_fads->progress_dialog = amitk_progress_dialog_new(GTK_WINDOW(tb_fads->dialog));

  druid = gnome_druid_new();
  gtk_container_add(GTK_CONTAINER(tb_fads->dialog), druid);
  g_signal_connect(G_OBJECT(druid), "cancel", G_CALLBACK(cancel_cb), tb_fads);


  /* --------------- initial page ------------------ */
  tb_fads->page[INTRO_PAGE]= 
    gnome_druid_page_edge_new_with_vals(GNOME_EDGE_START, TRUE, wizard_name,
					((AMITK_DATA_SET_NUM_FRAMES(tb_fads->data_set) > 1) ? 
					 start_page_text : not_enough_frames_text),
					logo, NULL, NULL);
  g_object_set_data(G_OBJECT(tb_fads->page[INTRO_PAGE]),"which_page", GINT_TO_POINTER(INTRO_PAGE));
  gnome_druid_append_page(GNOME_DRUID(druid),  GNOME_DRUID_PAGE(tb_fads->page[INTRO_PAGE]));
  gnome_druid_set_buttons_sensitive(GNOME_DRUID(druid), FALSE, 
				    AMITK_DATA_SET_NUM_FRAMES(tb_fads->data_set) > 1, TRUE, TRUE);
  for (i_page=INTRO_PAGE+1; i_page<OUTPUT_PAGE; i_page++) {
    tb_fads->page[i_page] = gnome_druid_page_standard_new_with_vals(wizard_name,logo,NULL);
    g_object_set_data(G_OBJECT(tb_fads->page[i_page]),"which_page", GINT_TO_POINTER(i_page));
    
    /* note, the _connect_after is a workaround, it should just be _connect */
    /* the problem, is gnome_druid currently overwrites button sensitivity in it's own prepare routine*/
    g_signal_connect_after(G_OBJECT(tb_fads->page[i_page]), "prepare", 
			   G_CALLBACK(prepare_page_cb), tb_fads);
    gnome_druid_append_page(GNOME_DRUID(druid), GNOME_DRUID_PAGE(tb_fads->page[i_page]));
  }

    
  /* ----------------  conclusion page ---------------------------------- */
  tb_fads->page[OUTPUT_PAGE] = 
    gnome_druid_page_edge_new_with_vals(GNOME_EDGE_FINISH, TRUE,
					wizard_name,NULL, logo,NULL, NULL);
  set_text(tb_fads);
  g_object_set_data(G_OBJECT(tb_fads->page[i_page]),"which_page", GINT_TO_POINTER(i_page));
  g_signal_connect(G_OBJECT(tb_fads->page[OUTPUT_PAGE]), "finish", G_CALLBACK(finish_cb), tb_fads);
  gnome_druid_append_page(GNOME_DRUID(druid),  GNOME_DRUID_PAGE(tb_fads->page[OUTPUT_PAGE]));



  g_object_unref(logo);
  gtk_widget_show_all(tb_fads->dialog);

  return;
}



#endif /* AMIDE_LIBGSL_SUPPORT */








