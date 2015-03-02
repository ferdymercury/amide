/* tb_fads.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2003-2014 Andy Loening
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
#include "amitk_progress_dialog.h"
#include "fads.h"
#include "tb_fads.h"
#include "ui_common.h"


#ifdef AMIDE_LIBGSL_SUPPORT

#define LABEL_WIDTH 400
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
N_("Welcome to the factor analysis of dynamic structures wizard. "
   "\n\n"
   "None of this code has been validated, and it's probably wrong, "
   "so use at your own risk");


static gchar * not_enough_frames_text =
  N_("Welcome to the factor analysis of dynamic structures wizard. "
     "\n\n"
     "This wizard only works with dynamic studies");


static gchar * curve_explanation_text =
  N_("You can select a text file to set the initial starting point for the fitted curves."
     "\n\n"
     "The file should have one line per time point, with one column for each parameter");

static gchar * no_curve_explanation_text =
  N_("Selected factor analysis method does not utilize initial curves");


typedef enum {
  INTRO_PAGE,
  SVD_PAGE,
  FACTOR_CHOICE_PAGE,
  PARAMETERS_PAGE,
  CURVES_PAGE,
  CONCLUSION_PAGE,
  NUM_PAGES
} which_page_t;

/* data structures */
typedef struct tb_fads_t {
  GtkWidget * dialog;

  AmitkDataSet * data_set;
  AmitkPreferences * preferences;
  gint num_factors;
  gint max_iterations;
  gdouble stopping_criteria;
  gboolean sum_factors_equal_one;
  gdouble beta;
  gdouble k12;
  gdouble k21;
  gdouble k23;
  gdouble k32;
  fads_type_t fads_type;
  fads_minimizer_algorithm_t algorithm;
  GArray * initial_curves; 

  GtkWidget * page[NUM_PAGES];
  GtkWidget * progress_dialog;
  GtkWidget * algorithm_menu;
  GtkWidget * num_factors_spin;
  GtkWidget * num_iterations_spin;
  GtkWidget * stopping_criteria_spin;
  GtkWidget * sum_factors_equal_one_button;
  GtkWidget * beta_spin;
  GtkWidget * k12_spin;
  GtkWidget * k21_spin;
  GtkWidget * svd_tree;
  GtkWidget * blood_add_button;
  GtkWidget * blood_remove_button;
  GtkWidget * blood_tree;
  GtkWidget * curve_add_button;
  GtkWidget * curve_remove_button;
  GtkWidget * curve_view;
  GtkTextBuffer * curve_text;
  GtkTextBuffer * explanation_buffer;

  guint reference_count;
} tb_fads_t;



static void set_text(tb_fads_t * tb_fads);
static void update_curve_text(tb_fads_t * tb_fads, gboolean allow_curve_entries);
static gchar * get_filename(tb_fads_t * tb_fads);

static void fads_type_cb(GtkWidget * widget, gpointer data);
static void algorithm_cb(GtkWidget * widget, gpointer data);
static void svd_pressed_cb(GtkButton * button, gpointer data);
static void blood_cell_edited(GtkCellRendererText *cellrenderertext,
			      gchar *arg1, gchar *arg2,gpointer data);
static void add_blood_pressed_cb(GtkButton * button, gpointer data);
static void remove_blood_pressed_cb(GtkButton * button, gpointer data);
static void read_in_curve_file(tb_fads_t * tb_fads, gchar * filename);
static void add_curve_pressed_cb(GtkButton * button, gpointer data);
static void remove_curve_pressed_cb(GtkButton * button, gpointer data);
static void num_factors_spinner_cb(GtkSpinButton * spin_button, gpointer data);
static void max_iterations_spinner_cb(GtkSpinButton * spin_button, gpointer data);
static void stopping_criteria_spinner_cb(GtkSpinButton * spin_button, gpointer data);
static void sum_factors_equal_one_toggle_cb(GtkToggleButton * button, gpointer data);
static void beta_spinner_cb(GtkSpinButton * spin_button, gpointer data);
static void k12_spinner_cb(GtkSpinButton * spin_button, gpointer data);
static void k21_spinner_cb(GtkSpinButton * spin_button, gpointer data);

static void apply_cb(GtkAssistant * assistant, gpointer data);
static void close_cb(GtkAssistant * assistant, gpointer data);

static tb_fads_t * tb_fads_free(tb_fads_t * tb_fads);
static tb_fads_t * tb_fads_init(void);


static void prepare_page_cb(GtkAssistant * wizard, GtkWidget * page, gpointer data);
static GtkWidget * create_page(tb_fads_t * tb_fads, which_page_t i_page);



static void set_text(tb_fads_t * tb_fads) {

  gchar * temp_string;
  GtkTextIter iter;
  GdkPixbuf * pixbuf;

  /* the output page text */
  if (tb_fads->page[CONCLUSION_PAGE] != NULL) {
    temp_string = g_strdup_printf(_("%s\nMethod Picked: %s"), 
				  _(finish_page_text), _(fads_type_explanation[tb_fads->fads_type]));
    gtk_label_set_text(GTK_LABEL(tb_fads->page[CONCLUSION_PAGE]),temp_string);
    g_free(temp_string);
  }

  /* the factor analysis type picking page */
  if (tb_fads->explanation_buffer != NULL) {
    gtk_text_buffer_set_text (tb_fads->explanation_buffer, 
			      _(fads_type_explanation[tb_fads->fads_type]), -1);

    if (fads_type_icon[tb_fads->fads_type] != NULL) {
      gtk_text_buffer_get_end_iter(tb_fads->explanation_buffer, &iter);
      gtk_text_buffer_insert(tb_fads->explanation_buffer, &iter, "\n", -1);

      pixbuf = gdk_pixbuf_new_from_inline(-1,fads_type_icon[tb_fads->fads_type] , FALSE, NULL);
      gtk_text_buffer_get_end_iter(tb_fads->explanation_buffer, &iter);
      gtk_text_buffer_insert_pixbuf(tb_fads->explanation_buffer, &iter, pixbuf);
      g_object_unref(pixbuf);
    }
  }

  return;
}

static void update_curve_text(tb_fads_t * tb_fads, gboolean allow_curve_entries) {
  gint i;
  gint j;
  gint k;
  GString * text_str;

  if (!allow_curve_entries) { /* using a method that doesn't use initial curves */
    gtk_text_buffer_set_text(tb_fads->curve_text, _(no_curve_explanation_text), -1);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tb_fads->curve_view), GTK_WRAP_WORD);
  } else if (tb_fads->initial_curves == NULL) {
    gtk_text_buffer_set_text(tb_fads->curve_text, _(curve_explanation_text), -1);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tb_fads->curve_view), GTK_WRAP_WORD);
  } else {
    text_str = g_string_new("frame/curve:");
    for (j=0; j < tb_fads->num_factors; j++) 
      g_string_append_printf(text_str, " %10d", j);
    g_string_append(text_str, "\n\n");

    for (i=0,j=0,k=0; i < tb_fads->initial_curves->len; i++,j++) {
      if (j >= tb_fads->num_factors) {
	g_string_append(text_str, "\n");
	j=0;
      }
      if (j==0) { /* new row, first column */
	g_string_append_printf(text_str, "%12d", k);
	k++;
      } 
      g_string_append_printf(text_str, " %10.9g", 
			     g_array_index(tb_fads->initial_curves, gdouble, i));
    }
    g_string_append(text_str, "\n");

    gtk_text_buffer_set_text(tb_fads->curve_text, text_str->str, -1);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tb_fads->curve_view), GTK_WRAP_NONE);
    g_string_free(text_str, TRUE);
  }

  return;
}



/* function to get a name to save the output as: 
   returned string will need to be free'd if not NULL */
static gchar * get_filename(tb_fads_t * tb_fads) {
  
  GtkWidget * file_chooser;
  gchar * analysis_name;
  gchar * save_filename;

  file_chooser = gtk_file_chooser_dialog_new (_("Filename for Factor Data"),
					      GTK_WINDOW(tb_fads->dialog), /* parent window */
					      GTK_FILE_CHOOSER_ACTION_SAVE,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					      NULL);
  gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(file_chooser), TRUE);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (file_chooser), TRUE);
  amitk_preferences_set_file_chooser_directory(tb_fads->preferences, file_chooser); /* set the default directory if applicable */

  /* take a guess at the filename */
  analysis_name = g_strdup_printf("%s_fads_analysis.tsv",AMITK_OBJECT_NAME(tb_fads->data_set));
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (file_chooser), analysis_name);
  g_free(analysis_name);

  /* run the dialog */
  if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT) 
    save_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));
  else
    save_filename = NULL;
  gtk_widget_destroy (file_chooser);

  return save_filename;

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
  ui_common_place_cursor(UI_CURSOR_WAIT, tb_fads->page[PARAMETERS_PAGE]);
  fads_svd_factors(tb_fads->data_set, &num_factors, &factors);
  ui_common_remove_wait_cursor(tb_fads->page[PARAMETERS_PAGE]);

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


 
static void read_in_curve_file(tb_fads_t * tb_fads, gchar * filename) {

  FILE * file;
  gdouble return_val;
  gchar line[1024];
  gchar **  string_chunks;
  gint i;


  if ((file = fopen(filename, "r")) == NULL) {
    g_warning(_("Could not open file: %s"), filename);
    return;
  }

  if (tb_fads->initial_curves != NULL) 
    g_array_free(tb_fads->initial_curves, TRUE);
  tb_fads->initial_curves = g_array_new(FALSE, TRUE, sizeof(gdouble));

  while(fgets(line, 1024, file) != NULL) {
    if (line[0] != '#') {/* skip comment lines */
      /* split the line by tabs */
      string_chunks = g_strsplit(line, "\t", -1);
      i=0;
      while (string_chunks[i] != NULL) {
	if (sscanf(string_chunks[i], "%lf", &return_val) == 1)
	  tb_fads->initial_curves = g_array_append_val(tb_fads->initial_curves, return_val);
	i++;
      }
      g_strfreev(string_chunks);
    }
  }

  if (tb_fads->initial_curves->len == 0) {
    g_warning(_("Could not find any numerical values in file: %s\n"), filename);
    g_array_free(tb_fads->initial_curves, TRUE);
    tb_fads->initial_curves = NULL;
  }


  update_curve_text(tb_fads, TRUE);

  fclose(file);


  return;
}


static void add_curve_pressed_cb(GtkButton * button, gpointer data) {

  tb_fads_t * tb_fads = data;
  GtkWidget * file_chooser;
  gchar * filename;

  /* get the name of the file to import */
  file_chooser = gtk_file_chooser_dialog_new (_("Import Curve File"),
					      GTK_WINDOW(tb_fads->dialog), /* parent window */
					      GTK_FILE_CHOOSER_ACTION_OPEN,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					      NULL);
  gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(file_chooser), TRUE);
  amitk_preferences_set_file_chooser_directory(tb_fads->preferences, file_chooser); 

  if (gtk_dialog_run (GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT) 
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));
  else
    filename = NULL;
  gtk_widget_destroy (file_chooser);

  if (filename == NULL) return;
  if (!ui_common_check_filename(filename)) {
    g_warning(_("Inappropriate Filename: %s"), filename);
    g_free(filename);
    return;
  }

#ifdef AMIDE_DEBUG
  g_print("Curve file to import: %s\n",filename);
#endif

  read_in_curve_file(tb_fads, filename);

  g_free(filename);
  return;
}

static void remove_curve_pressed_cb(GtkButton * button, gpointer data) {

  tb_fads_t * tb_fads = data;

  if (tb_fads->initial_curves != NULL) {
    g_array_free(tb_fads->initial_curves, TRUE);
    tb_fads->initial_curves = NULL;
  }

  /* and reset the explanation text */
  gtk_text_buffer_set_text(tb_fads->curve_text,
			   _(curve_explanation_text), -1);

  return;
}



static void fads_type_cb(GtkWidget * widget, gpointer data) {
  tb_fads_t * tb_fads = data;
  tb_fads->fads_type = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  set_text(tb_fads);
  return;
}

static void algorithm_cb(GtkWidget * widget, gpointer data) {
  tb_fads_t * tb_fads = data;
  tb_fads->algorithm = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
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

static void sum_factors_equal_one_toggle_cb(GtkToggleButton * button, gpointer data) {
  tb_fads_t * tb_fads = data;
  tb_fads->sum_factors_equal_one = gtk_toggle_button_get_active(button);
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
static void apply_cb(GtkAssistant * assistant, gpointer data) {

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

  ui_common_place_cursor(UI_CURSOR_WAIT, tb_fads->page[CONCLUSION_PAGE]);
  switch(tb_fads->fads_type) {
  case FADS_TYPE_PCA:
    fads_pca(tb_fads->data_set, tb_fads->num_factors, output_filename,
	     amitk_progress_dialog_update, tb_fads->progress_dialog);
    break;
  case FADS_TYPE_PLS:
    fads_pls(tb_fads->data_set, tb_fads->num_factors, tb_fads->algorithm, tb_fads->max_iterations,
	     tb_fads->stopping_criteria, tb_fads->sum_factors_equal_one,
	     tb_fads->beta, output_filename, num, frames, vals, tb_fads->initial_curves,
	     amitk_progress_dialog_update, tb_fads->progress_dialog);
    break;
  case FADS_TYPE_TWO_COMPARTMENT:
    fads_two_comp(tb_fads->data_set, tb_fads->algorithm, tb_fads->max_iterations, 
		  tb_fads->num_factors-1, tb_fads->k12, tb_fads->k21, tb_fads->stopping_criteria,
		  tb_fads->sum_factors_equal_one,
		  output_filename, num, frames, vals, 
		  amitk_progress_dialog_update, tb_fads->progress_dialog);
    break;
  default:
    g_error("fads type %d not defined", tb_fads->fads_type);
    break;
  }
  ui_common_remove_wait_cursor(tb_fads->page[CONCLUSION_PAGE]);

  if (frames != NULL) {
    g_free(frames);
    frames = NULL;
  }

  if (vals != NULL) {
    g_free(vals);
    vals = NULL;
  }

  if (output_filename != NULL) {
    g_free(output_filename);
    output_filename = NULL;
  }

  return;
}




/* function called to close the dialog */
static void close_cb(GtkAssistant * assistant, gpointer data) {

  tb_fads_t * tb_fads = data;
  GtkWidget * dialog = tb_fads->dialog;

  tb_fads = tb_fads_free(tb_fads); /* trash collection */
  gtk_widget_destroy(dialog);

  return;
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

    if (tb_fads->initial_curves != NULL) {
      g_array_free(tb_fads->initial_curves, TRUE);
      tb_fads->initial_curves = NULL;
    }

    if (tb_fads->data_set != NULL) {
      amitk_object_unref(tb_fads->data_set);
      tb_fads->data_set = NULL;
    }

    if (tb_fads->preferences != NULL) {
      g_object_unref(tb_fads->preferences);
      tb_fads->preferences = NULL;
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

  tb_fads_t * tb_fads;

  /* alloc space for the data structure for passing ui info */
  if ((tb_fads = g_try_new(tb_fads_t,1)) == NULL) {
    g_warning(_("couldn't allocate memory space for tb_fads_t"));
    return NULL;
  }

  tb_fads->reference_count = 1;
  tb_fads->dialog = NULL;
  tb_fads->data_set = NULL;
  tb_fads->preferences = NULL;
  tb_fads->num_factors = 2;
  tb_fads->max_iterations = 1e6;
  tb_fads->stopping_criteria = 1e-2;
  tb_fads->sum_factors_equal_one = FALSE;
  tb_fads->beta = 0.0;
  tb_fads->k12 = 0.01;
  tb_fads->k21 = 0.1;
  tb_fads->k23 = 0.01;
  tb_fads->k32 = 0.1;
  tb_fads->fads_type = FADS_TYPE_PCA;
  //  tb_fads->algorithm = FADS_MINIMIZER_VECTOR_BFGS;
  tb_fads->algorithm = FADS_MINIMIZER_CONJUGATE_PR;
  tb_fads->initial_curves = NULL;
  tb_fads->explanation_buffer = NULL;
  tb_fads->progress_dialog = NULL;

  return tb_fads;
}



static void prepare_page_cb(GtkAssistant * wizard, GtkWidget * page, gpointer data) {
 
  tb_fads_t * tb_fads = data;
  which_page_t which_page;
  gboolean num_factors;
  gboolean blood_entries;
  gboolean curve_entries;
  gboolean num_iterations;
  gboolean k_values;

  which_page = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(page), "which_page"));

  switch(tb_fads->fads_type) {
  case FADS_TYPE_PCA:
    k_values = FALSE;
    blood_entries = FALSE;
    curve_entries = FALSE;
    num_factors = TRUE;
    num_iterations = FALSE;
    break;
  case FADS_TYPE_TWO_COMPARTMENT:
    k_values = TRUE;
    blood_entries = TRUE;
    curve_entries = FALSE;
    num_factors = TRUE;
    num_iterations = TRUE;
    break;
  case FADS_TYPE_PLS:
    k_values = FALSE;
    blood_entries = TRUE;
    curve_entries = TRUE;
    num_factors = TRUE;
    num_iterations = TRUE;
    break;
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    k_values = FALSE;
    blood_entries = TRUE;
    curve_entries = TRUE;
    num_factors = TRUE;
    num_iterations = TRUE;
    break;
  }

  switch(which_page) {
  case PARAMETERS_PAGE:
    /* fill in the proper text for the option to add initial curves */
    gtk_text_buffer_set_text(tb_fads->curve_text,
			     curve_entries ? _(curve_explanation_text) : "", -1);

    /* hide options we don't need */
    gtk_widget_set_sensitive(tb_fads->blood_tree, blood_entries);
    gtk_widget_set_sensitive(tb_fads->blood_remove_button, blood_entries);
    gtk_widget_set_sensitive(tb_fads->blood_add_button, blood_entries);
    gtk_widget_set_sensitive(tb_fads->algorithm_menu, num_iterations);
    gtk_widget_set_sensitive(tb_fads->num_factors_spin, num_factors);
    gtk_widget_set_sensitive(tb_fads->num_iterations_spin, num_iterations);
    gtk_widget_set_sensitive(tb_fads->stopping_criteria_spin, num_iterations);
    gtk_widget_set_sensitive(tb_fads->sum_factors_equal_one_button, num_iterations);
    gtk_widget_set_sensitive(tb_fads->beta_spin, num_iterations);
    gtk_widget_set_sensitive(tb_fads->k12_spin, k_values);
    gtk_widget_set_sensitive(tb_fads->k21_spin, k_values);
    break;

  case CURVES_PAGE:
    /* hide options we don't need */
    gtk_widget_set_sensitive(tb_fads->curve_remove_button, curve_entries);
    gtk_widget_set_sensitive(tb_fads->curve_add_button, curve_entries);
    update_curve_text(tb_fads, curve_entries);

  default:
    break;
  }

}


static GtkWidget * create_page(tb_fads_t * tb_fads, which_page_t i_page) {

  fads_type_t i_fads_type;
  fads_minimizer_algorithm_t i_algorithm;
  GtkWidget * label;
  GtkWidget * button;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkListStore * store;
  gint table_row;
  GtkWidget * table;
  GtkWidget * scrolled;
  GtkWidget * menu;
  GtkWidget * view;
  GtkWidget * vseparator;
  GtkWidget * hseparator;
  GtkTextBuffer *buffer;

  table = gtk_table_new(8,6,FALSE);
  table_row=0;
      
  switch(i_page) {
  case SVD_PAGE:
    view = gtk_text_view_new ();
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
    gtk_text_buffer_set_text (buffer, _(svd_page_text), -1);
    gtk_table_attach(GTK_TABLE(table), view, 0,1, table_row,table_row+2,
		     FALSE,FALSE, X_PADDING, Y_PADDING);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
    gtk_widget_set_size_request(view,300,-1);
    
    /* a separator for clarity */
    vseparator = gtk_vseparator_new();
    gtk_table_attach(GTK_TABLE(table), vseparator, 1,2,table_row, table_row+2,0, GTK_FILL, X_PADDING, Y_PADDING);

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
    column = gtk_tree_view_column_new_with_attributes(_("singular entry"), renderer,"text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tb_fads->svd_tree), column);
    
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes(_("value"), renderer,"text", 1, NULL);
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
    menu = gtk_combo_box_new_text();
    
    for (i_fads_type = 0; i_fads_type < NUM_FADS_TYPES; i_fads_type++) 
      gtk_combo_box_append_text(GTK_COMBO_BOX(menu), _(fads_type_name[i_fads_type]));
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu), tb_fads->fads_type);
    g_signal_connect(G_OBJECT(menu), "changed", G_CALLBACK(fads_type_cb), tb_fads);
    gtk_table_attach(GTK_TABLE(table), menu, 1,2, table_row,table_row+1,
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
			      _(fads_type_explanation[tb_fads->fads_type]), -1);
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
    
    tb_fads->algorithm_menu = gtk_combo_box_new_text();
    for (i_algorithm = 0; i_algorithm < NUM_FADS_MINIMIZERS; i_algorithm++) 
      gtk_combo_box_append_text(GTK_COMBO_BOX(tb_fads->algorithm_menu), 
				_(fads_minimizer_algorithm_name[i_algorithm]));
    gtk_combo_box_set_active(GTK_COMBO_BOX(tb_fads->algorithm_menu), tb_fads->algorithm);
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
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_fads->stopping_criteria_spin), tb_fads->stopping_criteria);
    gtk_widget_set_size_request(tb_fads->stopping_criteria_spin, SPIN_BUTTON_X_SIZE, -1);
    g_signal_connect(G_OBJECT(tb_fads->stopping_criteria_spin), "value_changed",  
		     G_CALLBACK(stopping_criteria_spinner_cb), tb_fads);
    g_signal_connect(G_OBJECT(tb_fads->stopping_criteria_spin), "output",
		     G_CALLBACK(amitk_spin_button_scientific_output), NULL);
    gtk_table_attach(GTK_TABLE(table), tb_fads->stopping_criteria_spin, 1,2, table_row,table_row+1,
		     FALSE,FALSE, X_PADDING, Y_PADDING);
    table_row++;

    /* whether to force sum of factors to equal one */
    label = gtk_label_new(_("Constrain sum of factors = 1:"));
    gtk_table_attach(GTK_TABLE(table), label, 0,1, table_row,table_row+1,
		     FALSE,FALSE, X_PADDING, Y_PADDING);
    
    tb_fads->sum_factors_equal_one_button =  gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb_fads->sum_factors_equal_one_button),
				 tb_fads->sum_factors_equal_one);
    g_signal_connect(G_OBJECT(tb_fads->sum_factors_equal_one_button), "toggled",  
		     G_CALLBACK(sum_factors_equal_one_toggle_cb), tb_fads);
    gtk_table_attach(GTK_TABLE(table), tb_fads->sum_factors_equal_one_button, 1,2, table_row,table_row+1,
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
    gtk_widget_set_size_request(tb_fads->k12_spin, SPIN_BUTTON_X_SIZE, -1);
    g_signal_connect(G_OBJECT(tb_fads->k12_spin), "value_changed",  
		     G_CALLBACK(k12_spinner_cb), tb_fads);
    g_signal_connect(G_OBJECT(tb_fads->k12_spin), "output",
		     G_CALLBACK(amitk_spin_button_scientific_output), NULL);
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
    gtk_widget_set_size_request(tb_fads->k21_spin, SPIN_BUTTON_X_SIZE, -1);
    g_signal_connect(G_OBJECT(tb_fads->k21_spin), "value_changed",  
		     G_CALLBACK(k21_spinner_cb), tb_fads);
    g_signal_connect(G_OBJECT(tb_fads->k21_spin), "output",
		     G_CALLBACK(amitk_spin_button_scientific_output), NULL);
    gtk_table_attach(GTK_TABLE(table), tb_fads->k21_spin, 1,2, table_row,table_row+1,
		     FALSE,FALSE, X_PADDING, Y_PADDING);
    // table_row++;

    table_row = 0;
    /* a separator for clarity */
    vseparator = gtk_vseparator_new();
    gtk_table_attach(GTK_TABLE(table), vseparator, 2,3,table_row, table_row+6, 0, GTK_FILL, X_PADDING, Y_PADDING);
    
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
    gtk_table_attach(GTK_TABLE(table), scrolled, 3, 4, table_row, table_row+4, 
		     X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL, X_PADDING, Y_PADDING);
    table_row+=4;
    
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
    
    tb_fads->blood_remove_button = gtk_button_new_with_label(_("Remove Blood Sample"));
    g_signal_connect(G_OBJECT(tb_fads->blood_remove_button), "pressed", 
		     G_CALLBACK(remove_blood_pressed_cb), tb_fads);
    gtk_table_attach(GTK_TABLE(table), tb_fads->blood_remove_button, 3,4, table_row,table_row+1,
		     FALSE,FALSE, X_PADDING, Y_PADDING);

    break;
  case CURVES_PAGE:
    /* A button to add initial curves */
    tb_fads->curve_add_button = gtk_button_new_with_label(_("Add Initial Curves"));
    g_signal_connect(G_OBJECT(tb_fads->curve_add_button), "pressed", 
		     G_CALLBACK(add_curve_pressed_cb), tb_fads);
    gtk_table_attach(GTK_TABLE(table), tb_fads->curve_add_button, 0,1, table_row,table_row+1,
		     FALSE,FALSE, X_PADDING, Y_PADDING);
    table_row++;

    /* the scroll widget which the list will go into */
    scrolled = gtk_scrolled_window_new(NULL,NULL);
    gtk_widget_set_size_request(scrolled,200,200);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    gtk_table_attach(GTK_TABLE(table), scrolled, 0, 1, table_row, table_row+4,
		     X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL, X_PADDING, Y_PADDING);
    table_row+=4;
    
    /* the curve text area */
    tb_fads->curve_view = gtk_text_view_new();
    tb_fads->curve_text = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tb_fads->curve_view));
    gtk_widget_modify_font(tb_fads->curve_view, amitk_fixed_font_desc);

    gtk_text_view_set_editable(GTK_TEXT_VIEW(tb_fads->curve_view), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled),tb_fads->curve_view);
    
    tb_fads->curve_remove_button = gtk_button_new_with_label(_("Remove Initial Curves"));
    g_signal_connect(G_OBJECT(tb_fads->curve_remove_button), "pressed", 
		     G_CALLBACK(remove_curve_pressed_cb), tb_fads);
    gtk_table_attach(GTK_TABLE(table), tb_fads->curve_remove_button, 0,1, table_row,table_row+1,
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


void tb_fads(AmitkDataSet * active_ds, AmitkPreferences * preferences, GtkWindow * parent) {

  tb_fads_t * tb_fads;
  GdkPixbuf * logo;
  which_page_t i_page;


  g_return_if_fail(AMITK_IS_DATA_SET(active_ds));

  tb_fads = tb_fads_init();
  tb_fads->data_set = amitk_object_ref(active_ds);
  tb_fads->preferences = g_object_ref(preferences);


  tb_fads->dialog = gtk_assistant_new();
  gtk_window_set_transient_for(GTK_WINDOW(tb_fads->dialog), parent);
  gtk_window_set_destroy_with_parent(GTK_WINDOW(tb_fads->dialog), TRUE);
  g_signal_connect(G_OBJECT(tb_fads->dialog), "cancel", G_CALLBACK(close_cb), tb_fads);
  g_signal_connect(G_OBJECT(tb_fads->dialog), "close", G_CALLBACK(close_cb), tb_fads);
  g_signal_connect(G_OBJECT(tb_fads->dialog), "apply", G_CALLBACK(apply_cb), tb_fads);
  g_signal_connect(G_OBJECT(tb_fads->dialog), "prepare",  G_CALLBACK(prepare_page_cb), tb_fads);

  tb_fads->progress_dialog = amitk_progress_dialog_new(GTK_WINDOW(tb_fads->dialog));

  /* --------------- initial page ------------------ */
  tb_fads->page[INTRO_PAGE]= gtk_label_new(((AMITK_DATA_SET_NUM_FRAMES(tb_fads->data_set) > 1) ? 
					    _(start_page_text) : _(not_enough_frames_text)));
  gtk_widget_set_size_request(tb_fads->page[INTRO_PAGE],LABEL_WIDTH, -1);
  gtk_label_set_line_wrap(GTK_LABEL(tb_fads->page[INTRO_PAGE]), TRUE);
  gtk_assistant_append_page(GTK_ASSISTANT(tb_fads->dialog), tb_fads->page[INTRO_PAGE]);
  gtk_assistant_set_page_type(GTK_ASSISTANT(tb_fads->dialog), tb_fads->page[INTRO_PAGE],
			      GTK_ASSISTANT_PAGE_INTRO);
  gtk_assistant_set_page_complete(GTK_ASSISTANT(tb_fads->dialog), tb_fads->page[INTRO_PAGE], 
				  (AMITK_DATA_SET_NUM_FRAMES(tb_fads->data_set) > 1));


  /* --------------- the pages in the middle ---------------- */
  for (i_page=INTRO_PAGE+1; i_page<CONCLUSION_PAGE; i_page++) {
    tb_fads->page[i_page] = create_page(tb_fads, i_page);
    gtk_assistant_append_page(GTK_ASSISTANT(tb_fads->dialog), tb_fads->page[i_page]);
    gtk_assistant_set_page_complete(GTK_ASSISTANT(tb_fads->dialog), tb_fads->page[i_page], TRUE); /* all pages have default values */
  }

    
  /* ----------------  conclusion page ---------------------------------- */
  tb_fads->page[CONCLUSION_PAGE] = gtk_label_new("?");
  set_text(tb_fads);
  gtk_widget_set_size_request(tb_fads->page[CONCLUSION_PAGE],LABEL_WIDTH, -1);
  gtk_label_set_line_wrap(GTK_LABEL(tb_fads->page[CONCLUSION_PAGE]), TRUE);
  gtk_assistant_append_page(GTK_ASSISTANT(tb_fads->dialog), tb_fads->page[CONCLUSION_PAGE]);
  gtk_assistant_set_page_type(GTK_ASSISTANT(tb_fads->dialog), tb_fads->page[CONCLUSION_PAGE],
			      GTK_ASSISTANT_PAGE_CONFIRM);
  gtk_assistant_set_page_complete(GTK_ASSISTANT(tb_fads->dialog), tb_fads->page[CONCLUSION_PAGE], TRUE);



  logo = gtk_widget_render_icon(GTK_WIDGET(tb_fads->dialog), "amide_icon_logo", GTK_ICON_SIZE_DIALOG, 0);
  for (i_page=0; i_page<NUM_PAGES; i_page++) {
    gtk_assistant_set_page_header_image(GTK_ASSISTANT(tb_fads->dialog), tb_fads->page[i_page], logo);
    gtk_assistant_set_page_title(GTK_ASSISTANT(tb_fads->dialog), tb_fads->page[i_page], _(wizard_name));
    g_object_set_data(G_OBJECT(tb_fads->page[i_page]),"which_page", GINT_TO_POINTER(i_page));
  }
  g_object_unref(logo);

  gtk_widget_show_all(tb_fads->dialog);

  return;
}



#endif /* AMIDE_LIBGSL_SUPPORT */








