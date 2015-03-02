/* tb_profile.c
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
#include <libgnomecanvas/libgnomecanvas.h>
#include "amide.h"
#include "amitk_study.h"
#include "tb_profile.h"
#include "ui_common.h"
#ifdef AMIDE_LIBGSL_SUPPORT
#include <gsl/gsl_multifit_nlin.h>
#endif


#define CANVAS_WIDTH 400
#define CANVAS_HEIGHT 300
#define TEXT_HEIGHT 12.0
#define EDGE_SPACING 2.0

#define NUM_COLOR_ROTATIONS 6
guint32 color_rotation[NUM_COLOR_ROTATIONS] = {
  0x00FFFFFF,
  0xFFFF00FF,
  0xFF00FFFF,
  0xFF0000FF,
  0x00FF00FF,
  0x0000FFFF,
};

/* data structures */
typedef struct tb_profile_t {

  GtkWidget * dialog;
  AmitkStudy * study;
  AmitkPreferences * preferences;
  GtkWidget * angle_spin;
  guint idle_handler_id;
  GtkWidget * canvas;
  GtkWidget * text;
  gdouble min_x, max_x;
  gdouble scale_x;

  GPtrArray * results;


  /* gaussian fit stuff */
  GnomeCanvasItem * x_label[2];
  gboolean calc_gaussian_fit;
  gdouble initial_x;
  gdouble x_limit[2];
  gboolean fix_x;
  gboolean fix_dc_zero;
  GnomeCanvasItem * x_limit_item[2];

  guint reference_count;
} tb_profile_t;


typedef struct result_t {
  gchar * name;
  GPtrArray * line;
  GnomeCanvasItem * line_item;
  amide_data_t min_y, max_y, peak_location;
  gdouble scale_y;
  GnomeCanvasItem * y_label[2];
  GnomeCanvasItem * legend;

  /* gaussian fit stuff */
  gdouble b_fit, b_err;
  gdouble p_fit, p_err;
  gdouble c_fit, c_err;
  gdouble s_fit, s_err;
  gint iterations;
  gint status;
  GnomeCanvasItem * fit_item;

} result_t;


static GPtrArray * results_free(GPtrArray * results);
static tb_profile_t * profile_free(tb_profile_t * tb_profile);
static tb_profile_t * profile_init(void);
#ifdef AMIDE_LIBGSL_SUPPORT
static gboolean canvas_event_cb(GtkWidget* widget,  GdkEvent * event, gpointer data);
static void calc_gaussian_fit_cb(GtkWidget * button, gpointer data);
static void fix_x_cb(GtkWidget * widget, gpointer data);
static void fix_dc_zero_cb(GtkWidget * widget, gpointer data);
static double calc_gaussian(double b, double p, double c, double s, double loc);
static int gaussian_f(const gsl_vector * func_p, void *params, gsl_vector * f);
static int gaussian_df (const gsl_vector * func_p, void *params,  gsl_matrix * J);
static int gaussian_fdf (const gsl_vector * func_p, void *params, gsl_vector * f, gsl_matrix * J);
static void fit_gaussian(tb_profile_t * tb_profile);
static void display_gaussian_fit(tb_profile_t * tb_profile);
#endif
static void export_profiles(tb_profile_t * tb_profile);
static void save_profiles(const gchar * save_filename, tb_profile_t * tb_profile);
static void recalc_profiles(tb_profile_t * tb_profile);
static gboolean update_while_idle(gpointer data);
static void response_cb (GtkDialog * dialog, gint response_id, gpointer data);
static void destroy_cb(GtkObject * object, gpointer data);
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * delete_event, gpointer data);
static void view_center_changed_cb(AmitkStudy * study, gpointer data);
static void selections_changed_cb(AmitkObject * object, gpointer data);
static void profile_changed_cb(AmitkLineProfile * line_profile, gpointer data);
static void profile_switch_view_cb(GtkWidget * widget, gpointer data);
static void profile_angle_cb(GtkWidget * widget, gpointer data);
static void profile_update_entries(tb_profile_t * tb_profile);



static GPtrArray * results_free(GPtrArray * results) {

  gint i;
  result_t * result;

  if (results == NULL) return NULL;

  for (i=0; i < results->len; i++) {
    result = g_ptr_array_index(results, i);
    g_ptr_array_free(result->line, TRUE);
    if (result->name != NULL)
      g_free(result->name);
    if (result->line_item != NULL)
      gtk_object_destroy(GTK_OBJECT(result->line_item));
    if (result->y_label[0] != NULL)
      gtk_object_destroy(GTK_OBJECT(result->y_label[0]));
    if (result->y_label[1] != NULL)
      gtk_object_destroy(GTK_OBJECT(result->y_label[1]));
    if (result->legend != NULL)
      gtk_object_destroy(GTK_OBJECT(result->legend));
    if (result->fit_item != NULL)
      gtk_object_destroy(GTK_OBJECT(result->fit_item));
  }
  g_ptr_array_free(results, TRUE);

  return NULL;
}

static tb_profile_t * profile_free(tb_profile_t * tb_profile) {

  /* sanity checks */
  g_return_val_if_fail(tb_profile != NULL, NULL);
  g_return_val_if_fail(tb_profile->reference_count > 0, NULL);

  /* remove a reference count */
  tb_profile->reference_count--;

  /* things to do if we've removed all reference's */
  if (tb_profile->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing tb_profile\n");
#endif

    if (tb_profile->study != NULL) {
      /* disconnect any signal handlers */
      g_signal_handlers_disconnect_by_func(G_OBJECT(tb_profile->study), 
					   view_center_changed_cb, tb_profile);
      g_signal_handlers_disconnect_by_func(G_OBJECT(tb_profile->study), 
					   selections_changed_cb, tb_profile);
      g_signal_handlers_disconnect_by_func(G_OBJECT(AMITK_STUDY_LINE_PROFILE(tb_profile->study)), 
					   profile_changed_cb, tb_profile);

      amitk_object_unref(tb_profile->study);
      tb_profile->study = NULL;

    }

    if (tb_profile->preferences != NULL) {
      g_object_unref(tb_profile->preferences);
      tb_profile->preferences = NULL;
    }

    if (tb_profile->idle_handler_id != 0) {
      g_source_remove(tb_profile->idle_handler_id);
      tb_profile->idle_handler_id = 0;
    }

    if (tb_profile->results != NULL){
      tb_profile->results = results_free(tb_profile->results);
    }

    g_free(tb_profile);
    tb_profile = NULL;
  }
#ifdef AMIDE_DEBUG
  else {
    g_print("unrefering tb_profile\n");
  }
#endif

  return tb_profile;

}

static tb_profile_t * profile_init(void) {

  tb_profile_t * tb_profile;

  if ((tb_profile = g_try_new(tb_profile_t,1)) == NULL) {
    g_warning(_("couldn't allocate memory space for tb_profile_t"));
    return NULL;
  }

  tb_profile->reference_count = 1;
  tb_profile->study = NULL;
  tb_profile->preferences = NULL;
  tb_profile->dialog = NULL;
  tb_profile->idle_handler_id = 0;
  tb_profile->results = NULL;
  tb_profile->x_label[0] = NULL;
  tb_profile->x_label[1] = NULL;
  tb_profile->calc_gaussian_fit = TRUE;
  tb_profile->initial_x = -1.0;
  tb_profile->fix_x = FALSE;
  tb_profile->fix_dc_zero=FALSE;
  tb_profile->x_limit_item[0] = NULL;
  tb_profile->x_limit_item[1] = NULL;


  return tb_profile;
}


static gchar * results_as_string(tb_profile_t * tb_profile) {

  gchar * results;
  result_t * result;
  AmitkLineProfileDataElement * element;
  gint i,j;
  time_t current_time;
  
  /* intro information */
  time(&current_time);
  results = g_strdup_printf(_("# Profiles on Study: %s\tGenerated on: %s"),
			    AMITK_OBJECT_NAME(tb_profile->study), ctime(&current_time));
  
  for (i=0; i < tb_profile->results->len; i++) {
    result = g_ptr_array_index(tb_profile->results, i);
    amitk_append_str(&results, _("#\n# Profile on: %s\n"), result->name);
  
#ifdef AMIDE_LIBGSL_SUPPORT
    if (tb_profile->calc_gaussian_fit) {
      amitk_append_str(&results, _("# Gaussian Fit: b + p * e^(-0.5*(x-c)^2/s^2)\n"));
      amitk_append_str(&results,_("#\titerations used %d, status %s\n"),
		       result->iterations, gsl_strerror(result->status));
      if (tb_profile->fix_dc_zero)
	amitk_append_str(&results,"#\tb    = 0 %s\n",_("(fixed)"));
      else
	amitk_append_str(&results,"#\tb    = %.5g +/- %.5g\n",result->b_fit, result->b_err);
      amitk_append_str(&results,"#\tp    = %.5g +/- %.5g\n",result->p_fit, result->p_err);
      if (tb_profile->fix_x)
	amitk_append_str(&results,"#\tc    = %.5g mm %s\n",result->c_fit,_("(fixed)"));
      else
	amitk_append_str(&results,"#\tc    = %.5g +/- %.5g mm\n",result->c_fit, result->c_err);
      amitk_append_str(&results,"#\ts    = %.5g +/- %.5g\n",result->s_fit, result->s_err);
      amitk_append_str(&results,"#\tfwhm = %.5g +/- %.5g mm\n",
		       SIGMA_TO_FWHM*(result->s_fit), SIGMA_TO_FWHM*(result->s_err));
      amitk_append_str(&results,"#\tfwtm = %.5g +/- %.5g mm\n",
		       SIGMA_TO_FWTM*(result->s_fit), SIGMA_TO_FWTM*(result->s_err));
      amitk_append_str(&results,"#\n");
    }
#endif
    amitk_append_str(&results,_("# x\tvalue\n"));
    for (j=0; j<result->line->len; j++) {
      element = g_ptr_array_index(result->line, j);
      amitk_append_str(&results,"%g\t%g\n", element->location, element->value);
    }  
  }
  
  return results;
}



/* function to save the generated profile */
static void export_profiles(tb_profile_t * tb_profile) {
  
  GtkWidget * file_chooser;
  gchar * temp_string;
  GList * data_sets;
  GList * temp_data_sets;
  gchar * filename;

  /* sanity checks */
  g_return_if_fail(tb_profile != NULL);
  data_sets = 
    amitk_object_get_selected_children_of_type(AMITK_OBJECT(tb_profile->study),
					       AMITK_OBJECT_TYPE_DATA_SET, AMITK_SELECTION_ANY, TRUE);
  g_return_if_fail(data_sets != NULL);

  /* the rest of this function runs the file selection dialog box */
  file_chooser = gtk_file_chooser_dialog_new(_("Export Profile"), 
					     GTK_WINDOW(tb_profile->dialog), /* parent window */
					     GTK_FILE_CHOOSER_ACTION_SAVE,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					     NULL);
  gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(file_chooser), TRUE);
  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(file_chooser), TRUE);
  amitk_preferences_set_file_chooser_directory(tb_profile->preferences, file_chooser); /* set the default directory if applicable */

  /* take a guess at the filename */
  filename = g_strdup_printf("%s_profile_{%s",
				 AMITK_OBJECT_NAME(tb_profile->study),
				 AMITK_OBJECT_NAME(data_sets->data));
  temp_data_sets = data_sets->next;
  while (temp_data_sets != NULL) {
    temp_string = g_strdup_printf("%s+%s",filename,AMITK_OBJECT_NAME(temp_data_sets->data));
    g_free(filename);
    filename = temp_string;
    temp_data_sets = temp_data_sets->next;
  }
  temp_string = g_strdup_printf("%s}.tsv",filename);
  g_free(filename);
  filename = temp_string;
  amitk_objects_unref(data_sets);

  gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(file_chooser), filename);
  g_free(filename);

  /* run the save dialog */
  if (gtk_dialog_run(GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT) 
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (file_chooser));
  else
    filename = NULL;
  gtk_widget_destroy(file_chooser);

  if (filename != NULL) {
    /* allright, save the data */
    save_profiles(filename, tb_profile);
    g_free(filename);
  }

  return;
}

static void save_profiles(const gchar * save_filename, tb_profile_t * tb_profile) {

  FILE * file_pointer;
  gchar * results;

  if ((file_pointer = fopen(save_filename, "w")) == NULL) {
    g_warning(_("couldn't open: %s for writing profiles"), save_filename);
    return;
  }

  results = results_as_string(tb_profile);
  fprintf(file_pointer, "%s", results);
  g_free(results);

  fclose(file_pointer);

  return;
}


#ifdef AMIDE_LIBGSL_SUPPORT

static gboolean canvas_event_cb(GtkWidget* widget,  GdkEvent * event, gpointer data) {
  tb_profile_t * tb_profile = data;
  AmitkCanvasPoint canvas_cpoint;
  GnomeCanvas * canvas;
  gdouble x_point;
  gboolean find_new_initial = FALSE;
  AmitkLineProfileDataElement * element;
  gboolean initialized;
  gdouble peak_x=0.0;
  gdouble peak_y=0.0;
  int i,j;
  result_t * result;
  

  canvas = GNOME_CANVAS(widget);
  gnome_canvas_window_to_world(canvas, event->button.x, event->button.y, &canvas_cpoint.x, &canvas_cpoint.y);
  gnome_canvas_w2c_d(canvas, canvas_cpoint.x, canvas_cpoint.y, &canvas_cpoint.x, &canvas_cpoint.y);
  switch (event->type) {

  case GDK_BUTTON_RELEASE:
    x_point = ((canvas_cpoint.x-EDGE_SPACING)/tb_profile->scale_x)+tb_profile->min_x;
    switch (event->button.button) {
    case 1:
      if (x_point < tb_profile->x_limit[1])
	tb_profile->x_limit[0] = x_point;
      find_new_initial = TRUE;
      break;
    case 3:
      if (x_point > tb_profile->x_limit[0])
	tb_profile->x_limit[1] = x_point;
      find_new_initial = TRUE;
      break;
    case 2:
    default:
      tb_profile->initial_x = x_point;
      break;
    }

    /* find new peaks */
    if (find_new_initial) {
      tb_profile->initial_x = -1.0;
      
      for (i=0; i < tb_profile->results->len; i++) {
	result = g_ptr_array_index(tb_profile->results, i);
	g_return_val_if_fail(result != NULL, FALSE);
	
	initialized = FALSE;
	for (j=0; j<result->line->len; j++) {
	  element = g_ptr_array_index(result->line, j);
	  if ((element->location >= tb_profile->x_limit[0]) &&
	      (element->location <= tb_profile->x_limit[1])) {
	    if ((!initialized) || (peak_y < element->value)) {
	      peak_x = element->location;
	      peak_y = element->value;
	      initialized = TRUE;
	    } 
	  }
	}
	
	if (initialized) result->peak_location = peak_x;
      }
    }

    /* redisplay */
    if (tb_profile->calc_gaussian_fit)
      display_gaussian_fit(tb_profile);
    break;

  default:
    break;
  }


  return FALSE;
}

static void calc_gaussian_fit_cb(GtkWidget * widget, gpointer data) {
  tb_profile_t * tb_profile = data;

  tb_profile->calc_gaussian_fit = 
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  if (tb_profile->calc_gaussian_fit)
    display_gaussian_fit(tb_profile);
  else
    recalc_profiles(tb_profile); /* removes old gaussian fit */

  return;
}

static void fix_x_cb(GtkWidget * widget, gpointer data) {
  tb_profile_t * tb_profile = data;

  tb_profile->fix_x =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  if (tb_profile->calc_gaussian_fit)
    display_gaussian_fit(tb_profile);

  return;
}

static void fix_dc_zero_cb(GtkWidget * widget, gpointer data) {
  tb_profile_t * tb_profile = data;

  tb_profile->fix_dc_zero =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  if (tb_profile->calc_gaussian_fit)
    display_gaussian_fit(tb_profile);

  return;
}



static double calc_gaussian(double s, double p, double c, double b, double loc) {

  double diff;
  diff = loc-c;

  return b + p * (exp(-0.5*diff*diff/(s*s)));
}

typedef struct params_t {
  GPtrArray * line;
  gboolean fix_x;
  gdouble x;
  gboolean fix_dc_zero;
  gdouble x_limit[2];
} params_t;

static int gaussian_f(const gsl_vector * func_p, void * data, gsl_vector * f) {

  params_t * params = data;

  double b;
  double p;
  double c;
  double s;
  gint i;
  AmitkLineProfileDataElement * element;

  /* get the parameters */
  i = 0;
  s = gsl_vector_get(func_p, i++);
  p = gsl_vector_get(func_p, i++);
  if (params->fix_x) c = params->x;
  else c = gsl_vector_get(func_p, i++);
  if (params->fix_dc_zero) b = 0.0;
  else b = gsl_vector_get(func_p, i++);

  for (i = 0; i < params->line->len; i++) {
    element = g_ptr_array_index(params->line, i);
    if ((element->location > params->x_limit[0]) &&
	(element->location < params->x_limit[1])) 
      gsl_vector_set(f, i, calc_gaussian(s,p,c,b,element->location) - element->value);
    else
      gsl_vector_set(f, i, 0.0);
  }

  return GSL_SUCCESS;
}

static int gaussian_df (const gsl_vector * func_p, void *data,  gsl_matrix * J) {

  params_t * params = data;
  double p;
  double c;
  double s;
  gint i,j;
  AmitkLineProfileDataElement * element;
  double diff;
  double inner;

  /* get the parameters */
  i = 0;
  s = gsl_vector_get(func_p, i++);
  p = gsl_vector_get(func_p, i++);
  if (params->fix_x) c = params->x;
  else c = gsl_vector_get(func_p, i++);

  for (i = 0; i < params->line->len; i++) {
    element = g_ptr_array_index(params->line, i);
    diff = element->location-c;
    inner = exp(-0.5*(diff)*(diff)/(s*s));

    j = 0;
    if ((element->location > params->x_limit[0]) &&
	(element->location < params->x_limit[1])) {
      gsl_matrix_set (J, i, j++, p*inner*diff*diff/(s*s*s));
      gsl_matrix_set (J, i, j++, inner);
      if (!params->fix_x)
	gsl_matrix_set (J, i, j++, p*inner*diff/(s*s));
      if (!params->fix_dc_zero)
	gsl_matrix_set (J, i, j++, 1);
    } else {
      gsl_matrix_set (J, i, j++, 0.0);
      gsl_matrix_set (J, i, j++, 0.0);
      if (!params->fix_x)
	gsl_matrix_set (J, i, j++, 0.0);
      if (!params->fix_dc_zero)
	gsl_matrix_set (J, i, j++, 0.0);
    }
  }

  return GSL_SUCCESS;
}


static int gaussian_fdf (const gsl_vector * func_p, void *params, gsl_vector * f, gsl_matrix * J) {
  gaussian_f (func_p, params, f);
  gaussian_df (func_p, params, J);
  return GSL_SUCCESS;
}


/* we're fitting the following function 
   b + p * exp(-0.5 * ((x-c)/s)^2)
*/
static void fit_gaussian(tb_profile_t * tb_profile) {

  gint i,j;
  result_t * result;
  gsl_multifit_fdfsolver * solver;
  gsl_matrix *covar;
  gsl_multifit_function_fdf fdf;
  gsl_vector * init_p;
  gint iter;
  gint status;
  gint num_p;
  params_t params;
  gdouble initial_x;

  num_p = 2;
  if (!tb_profile->fix_x) num_p++;
  if (!tb_profile->fix_dc_zero) num_p++;

  covar = gsl_matrix_alloc (num_p, num_p);
  g_return_if_fail(covar != NULL);

  init_p = gsl_vector_alloc(num_p);
  fdf.f = &gaussian_f;
  fdf.df = &gaussian_df;
  fdf.fdf = &gaussian_fdf;
  fdf.p = num_p;

  /* fit each profile */
  for (i=0; i < tb_profile->results->len; i++) {
    result = g_ptr_array_index(tb_profile->results, i);
    g_return_if_fail(result != NULL);

    /* figure out where we'd like to start along x*/
    initial_x = (tb_profile->initial_x >= 0.0) ? tb_profile->initial_x : result->peak_location;

    /* initialize parameters */
    j=0;
    gsl_vector_set(init_p, j++, 1.0); /* s - sigma argument, proportional to width */
    gsl_vector_set(init_p, j++, result->max_y); /* peak val */
    if (!tb_profile->fix_x)
      gsl_vector_set(init_p, j++, initial_x); /* x offset val */
    if (!tb_profile->fix_dc_zero)
      gsl_vector_set(init_p, j++, result->min_y); /* b - DC val */

    /* alloc the solver */
    solver = gsl_multifit_fdfsolver_alloc (gsl_multifit_fdfsolver_lmder,result->line->len, num_p);
    g_return_if_fail(solver != NULL);

    /* assign the data we're fitting */
    params.line = result->line;
    params.fix_x = tb_profile->fix_x;
    params.x = initial_x;
    params.x_limit[0] = tb_profile->x_limit[0];
    params.x_limit[1] = tb_profile->x_limit[1];
    params.fix_dc_zero = tb_profile->fix_dc_zero;
    fdf.params = &params;
    fdf.n = result->line->len;
    gsl_multifit_fdfsolver_set (solver, &fdf, init_p);

    /* and iterate */
    iter = 0;
    do {
      iter++;
      status = gsl_multifit_fdfsolver_iterate (solver);
      if (status) break;
      status = gsl_multifit_test_delta (solver->dx, solver->x, 1e-4, 1e-4);
    }
    while ((status == GSL_CONTINUE) && (iter < 100));

    gsl_multifit_covar (solver->J, 0.0, covar);

    j=0;
    result->s_fit = gsl_vector_get(solver->x, j++);
    result->p_fit = gsl_vector_get(solver->x, j++);
    if (tb_profile->fix_x)
      result->c_fit = initial_x;
    else
      result->c_fit = gsl_vector_get(solver->x, j++);
    if (tb_profile->fix_dc_zero)
      result->b_fit = 0.0;
    else
      result->b_fit = gsl_vector_get(solver->x, j++);


    j=0;
    result->s_err = sqrt(gsl_matrix_get(covar,j,j));
    j++;
    result->p_err = sqrt(gsl_matrix_get(covar,j,j));
    j++;
    if (tb_profile->fix_x)
      result->c_err = 0.0;
    else {
      result->c_err = sqrt(gsl_matrix_get(covar,j,j));
      j++;
    }
    if (tb_profile->fix_dc_zero) 
      result->b_err = 0.0;
    else {
      result->b_err = sqrt(gsl_matrix_get(covar,j,j));
      j++;
    }

    result->iterations = iter;
    result->status = status;

    /* cleanup */
    gsl_multifit_fdfsolver_free(solver);
  }

  gsl_matrix_free(covar);
  gsl_vector_free(init_p);

  return;
}


static void display_gaussian_fit(tb_profile_t * tb_profile) {

  gint i, j;
  GtkTextBuffer *buffer;
  GtkTextIter start_iter, end_iter;
  gchar * results_str;
  gdouble value;
  GnomeCanvasPoints * points;
  div_t x;
  guint color;
  result_t * result;
  double loc;

  /* perform gaussian fits */
  fit_gaussian(tb_profile);

  /* make the x limit lines */
  if (tb_profile->x_limit_item[0] != NULL)
    gtk_object_destroy(GTK_OBJECT(tb_profile->x_limit_item[0]));
  if (tb_profile->x_limit_item[1] != NULL)
    gtk_object_destroy(GTK_OBJECT(tb_profile->x_limit_item[1]));
  
  points = gnome_canvas_points_new(2);
  points->coords[0] = points->coords[2] = 
    tb_profile->scale_x*(tb_profile->x_limit[0]-tb_profile->min_x)+EDGE_SPACING;
  points->coords[1] = EDGE_SPACING+CANVAS_HEIGHT/4.0;
  points->coords[3] = 3*CANVAS_HEIGHT/4.0+EDGE_SPACING;
  tb_profile->x_limit_item[0] = 
    gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(tb_profile->canvas)),
			  gnome_canvas_line_get_type(),
			  "points", points,
			  "fill_color", "white",
#ifndef AMIDE_LIBGNOMECANVAS_AA
			  "line_style", GDK_LINE_ON_OFF_DASH,
#endif
			  "width_units", 1.0,
			  NULL);
  gnome_canvas_points_unref(points);
  
  points = gnome_canvas_points_new(2);
  points->coords[0] = points->coords[2] =
    tb_profile->scale_x*(tb_profile->x_limit[1]-tb_profile->min_x)+EDGE_SPACING;
  points->coords[1] = EDGE_SPACING+CANVAS_HEIGHT/4.0;
  points->coords[3] = 3*CANVAS_HEIGHT/4.0+EDGE_SPACING;
  tb_profile->x_limit_item[1] = 
    gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(tb_profile->canvas)),
			  gnome_canvas_line_get_type(),
			  "points", points,
			  "fill_color", "white",
#ifndef AMIDE_LIBGNOMECANVAS_AA
			  "line_style", GDK_LINE_ON_OFF_DASH,
#endif
			  "width_units", 1.0,
			  NULL);
  gnome_canvas_points_unref(points);

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tb_profile->text));

  /* delete any text in the buffer */
  gtk_text_buffer_get_start_iter(buffer, &start_iter);
  gtk_text_buffer_get_end_iter(buffer, &end_iter);
  gtk_text_buffer_delete(buffer, &start_iter, &end_iter);

  /* write out the gaussian equation */
  gtk_text_buffer_set_text (buffer, _("fit = b + p * e^(-0.5*(x-c)^2/s^2)"), -1);


  for (i=0; i < tb_profile->results->len; i++) {
    result = g_ptr_array_index(tb_profile->results, i);

    points = gnome_canvas_points_new(CANVAS_WIDTH);
    for (j=0; j<CANVAS_WIDTH; j++) {
      loc = ((((gdouble) j)-EDGE_SPACING)/tb_profile->scale_x)+tb_profile->min_x;
      value = calc_gaussian(result->s_fit, result->p_fit, result->c_fit,result->b_fit, loc);
      points->coords[2*j+0] = (gdouble) j;
      points->coords[2*j+1] = CANVAS_HEIGHT-EDGE_SPACING-result->scale_y*(value-result->min_y);
    }

    /* figure out the color we'll use */
    x = div(i, NUM_COLOR_ROTATIONS);
    if (tb_profile->results->len < 2) {
      color = 0xFFFFFFFF;
    } else {
      color = color_rotation[x.rem];
    }


    /* make sure it's destroyed */
    if (result->fit_item != NULL)
      gtk_object_destroy(GTK_OBJECT(result->fit_item));

    /* and (re)create it */
    result->fit_item = 
      gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(tb_profile->canvas)),
			    gnome_canvas_line_get_type(),
			    "points", points,
			    "fill_color_rgba", color,
#ifndef AMIDE_LIBGNOMECANVAS_AA
			    "line_style", GDK_LINE_ON_OFF_DASH,
#endif
			    "width_units", 1.0,
			    NULL);
    gnome_canvas_points_unref(points);

    results_str = 
      g_strdup_printf(_("\n\ngaussian fit on: %s\n"
			"iterations used %d, status %s\n"
			"b    = %.5g +/- %.5g %s\n"
			"p    = %.5g +/- %.5g\n"
			"c    = %.5g +/- %.5g mm %s\n"
			"s    = %.5g +/- %.5g\n"
			"fwhm = %.5g +/- %.5g mm\n"
			"fwtm = %.5g +/- %.5g mm"),
		      result->name,
		      result->iterations, gsl_strerror (result->status),
		      result->b_fit, result->b_err,
		      tb_profile->fix_dc_zero ? _("(fixed)"): "",
		      result->p_fit, result->p_err,
		      result->c_fit, result->c_err,
		      tb_profile->fix_x ? _("(fixed)") : "",
		      result->s_fit, result->s_err,
		      SIGMA_TO_FWHM*(result->s_fit), SIGMA_TO_FWHM*(result->s_err),
		      SIGMA_TO_FWTM*(result->s_fit), SIGMA_TO_FWTM*(result->s_err));
    gtk_text_buffer_insert_at_cursor(buffer, results_str, -1);
    g_free(results_str);
  }
    
  return;
}
#endif



static void recalc_profiles(tb_profile_t * tb_profile) {
  
  profile_update_entries(tb_profile);

  if (tb_profile->idle_handler_id == 0)
    tb_profile->idle_handler_id = 
      g_idle_add_full(G_PRIORITY_HIGH_IDLE,update_while_idle, tb_profile, NULL);


  return;
}


static gboolean update_while_idle(gpointer data) {
  
  tb_profile_t * tb_profile = data;
  gboolean initialized=FALSE;
  GList * data_sets;
  GList * temp_data_sets;
  GPtrArray * one_line;
  gint i,j;
  AmitkLineProfileDataElement * element;
  GnomeCanvasPoints * points;
  result_t * result;
  div_t x;
  gchar * label;

  ui_common_place_cursor(UI_CURSOR_WAIT, tb_profile->canvas);
  data_sets = amitk_object_get_selected_children_of_type(AMITK_OBJECT(tb_profile->study), 
							 AMITK_OBJECT_TYPE_DATA_SET, 
							 AMITK_SELECTION_ANY, TRUE);

  /* discard the old results... this also erases them from canvas */
  tb_profile->results = results_free(tb_profile->results);
  tb_profile->results = g_ptr_array_new();

  /* recalc profiles */
  tb_profile->max_x = tb_profile->min_x = 0.0;
  temp_data_sets = data_sets;
  while (temp_data_sets != NULL) {
    amitk_data_set_get_line_profile(AMITK_DATA_SET(temp_data_sets->data),
				    AMITK_STUDY_VIEW_START_TIME(tb_profile->study),
				    AMITK_STUDY_VIEW_DURATION(tb_profile->study),
				    AMITK_LINE_PROFILE_START_POINT(AMITK_STUDY_LINE_PROFILE(tb_profile->study)),
				    AMITK_LINE_PROFILE_END_POINT(AMITK_STUDY_LINE_PROFILE(tb_profile->study)),
				    &one_line);

    if (one_line->len > 1) { /* need at least two points for a valid line */

      result = g_malloc(sizeof(result_t));
      g_return_val_if_fail(result != NULL, FALSE);
      result->name = g_strdup(AMITK_OBJECT_NAME(temp_data_sets->data));
      result->line_item = NULL;
      result->y_label[0] = NULL;
      result->y_label[1] = NULL;
      result->legend = NULL;
      result->line = one_line;
      result->fit_item = NULL;

      /* get max/min y values */

      /* get max/min values */
      element = g_ptr_array_index(one_line, 0);
      result->max_y = result->min_y = element->value;
      result->peak_location = element->location;
      if (!initialized) {
	tb_profile->min_x = tb_profile->max_x = element->location;
	initialized = TRUE;
      }
      for (j=0; j<one_line->len; j++) {
	element = g_ptr_array_index(one_line, j);
	if (element->location < tb_profile->min_x) tb_profile->min_x = element->location;
	if (element->location > tb_profile->max_x) tb_profile->max_x = element->location;
	if (element->value < result->min_y) result->min_y = element->value;
	if (element->value > result->max_y) {
	  result->max_y = element->value;
	  result->peak_location = element->location;
	}
      }

      g_ptr_array_add(tb_profile->results, result);
    } else
      g_ptr_array_free(one_line, TRUE);
    
    temp_data_sets = temp_data_sets->next;
    
  }

  label = g_strdup_printf("%g", tb_profile->min_x);
  if (tb_profile->x_label[0] != NULL) {
    gnome_canvas_item_set(tb_profile->x_label[0], "text", label, NULL);
  } else {
    tb_profile->x_label[0] = 
      gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(tb_profile->canvas)), 
			    gnome_canvas_text_get_type(),
			    "anchor", GTK_ANCHOR_SOUTH_WEST, "text", label,
			    "x", (gdouble) EDGE_SPACING+5.0,
			    "y", (gdouble) CANVAS_HEIGHT-EDGE_SPACING,
			    "fill_color", "white",
			    "font_desc", amitk_fixed_font_desc, NULL);
  }
  g_free(label);

  label = g_strdup_printf("%g", tb_profile->max_x);
  if (tb_profile->x_label[1] != NULL) {
    gnome_canvas_item_set(tb_profile->x_label[1], "text", label, NULL);
  } else {
    tb_profile->x_label[1] = 
      gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(tb_profile->canvas)), 
			    gnome_canvas_text_get_type(),
			    "anchor", GTK_ANCHOR_SOUTH_EAST, "text", label,
			    "x", (gdouble) CANVAS_WIDTH-EDGE_SPACING,
			    "y", (gdouble) CANVAS_HEIGHT-EDGE_SPACING,
			    "fill_color", "white",
			    "font_desc", amitk_fixed_font_desc, NULL);
  }
  g_free(label);



  tb_profile->scale_x = (CANVAS_WIDTH-2*EDGE_SPACING)/(tb_profile->max_x-tb_profile->min_x);
  /* generate new lines */
  for (i=0; i < tb_profile->results->len; i++) {
    result = g_ptr_array_index(tb_profile->results, i);

    points = gnome_canvas_points_new(result->line->len);

    result->scale_y = (CANVAS_HEIGHT-2*EDGE_SPACING)/(result->max_y-result->min_y);
    for (j=0; j<result->line->len; j++) {
      element = g_ptr_array_index(result->line, j);
      points->coords[2*j+0] = tb_profile->scale_x*(element->location-tb_profile->min_x)+EDGE_SPACING;
      points->coords[2*j+1] = CANVAS_HEIGHT-EDGE_SPACING-result->scale_y*(element->value-result->min_y);
    }

    x = div(i, NUM_COLOR_ROTATIONS);
    result->line_item = gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(tb_profile->canvas)),
					      gnome_canvas_line_get_type(),
					      "points", points,
					      "fill_color_rgba", color_rotation[x.rem],
					      "width_units", 1.0,
					      NULL);
    gnome_canvas_points_unref(points);

    label = g_strdup_printf("%g", result->min_y);
    result->y_label[0] = 
      gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(tb_profile->canvas)), 
			    gnome_canvas_text_get_type(),
			    "anchor", GTK_ANCHOR_SOUTH_WEST, "text", label,
			    "x", (gdouble) EDGE_SPACING,
			    "y", (gdouble) CANVAS_HEIGHT-EDGE_SPACING-TEXT_HEIGHT-(tb_profile->results->len-i-1)*TEXT_HEIGHT,
			    "fill_color_rgba", color_rotation[x.rem],
			    "font_desc", amitk_fixed_font_desc, NULL);
    g_free(label);
    
    label = g_strdup_printf("%g", result->max_y);
    result->y_label[1] = 
      gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(tb_profile->canvas)), 
			    gnome_canvas_text_get_type(),
			    "anchor", GTK_ANCHOR_NORTH_WEST, "text", label,
			    "x", (gdouble) EDGE_SPACING,
			    "y", (gdouble) EDGE_SPACING+i*TEXT_HEIGHT,
			    "fill_color_rgba", color_rotation[x.rem],
			    "font_desc", amitk_fixed_font_desc, NULL);
    g_free(label);

    result->legend = 
      gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(tb_profile->canvas)), 
			    gnome_canvas_text_get_type(),
			    "anchor", GTK_ANCHOR_NORTH_EAST, 
			    "text", result->name,
			    "x", (gdouble) CANVAS_WIDTH-EDGE_SPACING,
			    "y", (gdouble) EDGE_SPACING+i*TEXT_HEIGHT,
			    "fill_color_rgba", color_rotation[x.rem],
			    "font_desc", amitk_fixed_font_desc, NULL);

  tb_profile->initial_x = -1.0;
  tb_profile->x_limit[0] = tb_profile->min_x;
  tb_profile->x_limit[1] = tb_profile->max_x;

#ifdef AMIDE_LIBGSL_SUPPORT
    if (tb_profile->calc_gaussian_fit)
      display_gaussian_fit(tb_profile);
#endif
  }


  /* and we're done */
  ui_common_remove_wait_cursor(tb_profile->canvas);
  data_sets = amitk_objects_unref(data_sets);

  tb_profile->idle_handler_id=0;

  return FALSE;
}


static void response_cb (GtkDialog * dialog, gint response_id, gpointer data) {
  
  tb_profile_t * tb_profile = data;
  gint return_val;
  GtkClipboard * clipboard;
  gchar * results;

  switch(response_id) {
  case AMITK_RESPONSE_SAVE_AS:
    export_profiles(tb_profile);
    break;

  case AMITK_RESPONSE_COPY:
    results = results_as_string(tb_profile);

    /* fill in select/button2 clipboard (X11) */
    clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    gtk_clipboard_set_text(clipboard, results, -1);

    /* fill in copy/paste clipboard (Win32 and Gnome) */
    clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clipboard, results, -1);

    g_free(results);
    break;

  case GTK_RESPONSE_HELP:
    amide_call_help("profile-dialog");
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


static void destroy_cb(GtkObject * object, gpointer data) {
  
  tb_profile_t * tb_profile = data;

  /* make the line profile invisible */
  amitk_line_profile_set_visible(AMITK_STUDY_LINE_PROFILE(tb_profile->study), FALSE);

  /* free the associated data structure */
  tb_profile = profile_free(tb_profile);

}

/* function called to destroy the dialog */
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {
  return FALSE;
}


static void view_center_changed_cb(AmitkStudy * study, gpointer data) {
  tb_profile_t * tb_profile=data;
  recalc_profiles(tb_profile);
  return;
}

/* somewhat wasteful... as we update the profiles if any object changes
   selection, not just data sets... but this greatly simplifies bookkeeping */
static void selections_changed_cb(AmitkObject * object, gpointer data) {
  tb_profile_t * tb_profile=data;
  recalc_profiles(tb_profile);
  return;
}

static void profile_changed_cb(AmitkLineProfile * line_profile, gpointer data) {
  tb_profile_t * tb_profile=data;
  recalc_profiles(tb_profile);
  return;
}

static void profile_switch_view_cb(GtkWidget * widget, gpointer data) {
  tb_profile_t * tb_profile=data;
  AmitkView view;

  view = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "view"));
  amitk_line_profile_set_view(AMITK_STUDY_LINE_PROFILE(tb_profile->study), view);

  return;
}

static void profile_angle_cb(GtkWidget * widget, gpointer data) {

  gdouble temp_val;
  tb_profile_t * tb_profile = data;

  temp_val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));

  temp_val *= M_PI/180.0; /* convert to radians */
  amitk_line_profile_set_angle(AMITK_STUDY_LINE_PROFILE(tb_profile->study), temp_val);

  profile_update_entries(tb_profile);
  return;
}

static void profile_update_entries(tb_profile_t * tb_profile) {
  amide_real_t angle;

  g_signal_handlers_block_by_func(G_OBJECT(tb_profile->angle_spin),
				  G_CALLBACK(profile_angle_cb), tb_profile);
  angle = AMITK_LINE_PROFILE_ANGLE(AMITK_STUDY_LINE_PROFILE(tb_profile->study));
  angle *= 180.0/M_PI;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(tb_profile->angle_spin),angle);
			    
  g_signal_handlers_unblock_by_func(G_OBJECT(tb_profile->angle_spin),
				    G_CALLBACK(profile_angle_cb), tb_profile);

  return;
}


void tb_profile(AmitkStudy * study, AmitkPreferences * preferences, GtkWindow * parent) {

  GtkWidget * dialog;
  gchar * title;
  GtkWidget * table;
  gint table_row;
  tb_profile_t * tb_profile;
  AmitkView i_view;
  GtkWidget * radio_button[3];
  GtkWidget * hbox;
  GtkWidget * label;
#ifdef AMIDE_LIBGSL_SUPPORT
  GtkWidget * check_button;
  GdkColor color;
#endif
  
  tb_profile = profile_init();
  tb_profile->study = g_object_ref(study);
  tb_profile->preferences = g_object_ref(preferences);

  /* make the line profile visible */
  amitk_line_profile_set_visible(AMITK_STUDY_LINE_PROFILE(tb_profile->study), TRUE);

  /* start setting up the widget we'll display the info from */
  title = g_strdup_printf(_("%s Profile Tool: Study %s"), PACKAGE, 
			  AMITK_OBJECT_NAME(tb_profile->study));
  dialog = gtk_dialog_new_with_buttons(title, GTK_WINDOW(parent),
				       GTK_DIALOG_DESTROY_WITH_PARENT,
				       GTK_STOCK_SAVE_AS, AMITK_RESPONSE_SAVE_AS,
				       GTK_STOCK_COPY, AMITK_RESPONSE_COPY,
				       GTK_STOCK_HELP, GTK_RESPONSE_HELP,
				       GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
				       NULL);
  g_free(title);
  gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);

  /* setup the callbacks for the dialog */
  g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(response_cb), tb_profile);
  g_signal_connect(G_OBJECT(dialog), "delete_event", G_CALLBACK(delete_event_cb), tb_profile);
  g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(destroy_cb), tb_profile);

  /* setup the callbacks for detecting if the line profile has changed */
  g_signal_connect(G_OBJECT(tb_profile->study), "view_center_changed", 
		   G_CALLBACK(view_center_changed_cb), tb_profile);
  g_signal_connect(G_OBJECT(tb_profile->study), "object_child_selection_changed",
		   G_CALLBACK(selections_changed_cb), tb_profile);
  g_signal_connect(G_OBJECT(AMITK_STUDY_LINE_PROFILE(tb_profile->study)), "line_profile_changed", 
		   G_CALLBACK(profile_changed_cb), tb_profile);


  /* make the widgets for this dialog box */
  table = gtk_table_new(5,3,FALSE);
  table_row=0;
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);
  gtk_widget_show(table);
  
  
  /* which view do we want the profile on */
  label = gtk_label_new(_("Line Profile on:"));
  gtk_table_attach(GTK_TABLE(table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(label);
    
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_table_attach(GTK_TABLE(table), hbox,1,3,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(hbox);
    
  /* the radio buttons */
  for (i_view=0; i_view < AMITK_VIEW_NUM; i_view++) {
    if (i_view == 0)
      radio_button[i_view] = 
	gtk_radio_button_new_with_label(NULL,amitk_view_get_name(i_view));
    else
      radio_button[i_view] = 
	gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_button[0]),
						    amitk_view_get_name(i_view));
    gtk_box_pack_start(GTK_BOX(hbox), radio_button[i_view], FALSE, FALSE, 3);
    g_object_set_data(G_OBJECT(radio_button[i_view]), "view", GINT_TO_POINTER(i_view));
    gtk_widget_show(radio_button[i_view]);
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button[AMITK_LINE_PROFILE_VIEW(AMITK_STUDY_LINE_PROFILE(tb_profile->study))]), TRUE);
  for (i_view=0; i_view < AMITK_VIEW_NUM; i_view++) 
    g_signal_connect(G_OBJECT(radio_button[i_view]), "clicked", G_CALLBACK(profile_switch_view_cb), tb_profile);
    
  table_row++;


  /* changing the angle */
  label = gtk_label_new(_("Angle (degrees):"));
  gtk_table_attach(GTK_TABLE(table), label, 0,1,
		   table_row, table_row+1, 0, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(label);

  tb_profile->angle_spin = gtk_spin_button_new_with_range(-G_MAXDOUBLE, G_MAXDOUBLE, 1.0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(tb_profile->angle_spin), FALSE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tb_profile->angle_spin),3);
  g_signal_connect(G_OBJECT(tb_profile->angle_spin), "value_changed", 
		   G_CALLBACK(profile_angle_cb), tb_profile);
  gtk_table_attach(GTK_TABLE(table),tb_profile->angle_spin,1,3, 
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(tb_profile->angle_spin);
  table_row++;


  /* the canvas */
#ifdef AMIDE_LIBGNOMECANVAS_AA
  tb_profile->canvas = gnome_canvas_new_aa();
#else
  tb_profile->canvas = gnome_canvas_new();
#endif
  gtk_table_attach(GTK_TABLE(table), tb_profile->canvas, 0, 3, table_row, table_row+1, 
		   X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);
#ifdef AMIDE_LIBGSL_SUPPORT
  g_signal_connect(G_OBJECT(tb_profile->canvas), "event",
		   G_CALLBACK(canvas_event_cb), tb_profile);
#endif
  gtk_widget_set_size_request(tb_profile->canvas, CANVAS_WIDTH+1, CANVAS_HEIGHT+1);
  gnome_canvas_set_scroll_region(GNOME_CANVAS(tb_profile->canvas), 0.0, 0.0,
				 CANVAS_WIDTH, CANVAS_HEIGHT);
  gtk_widget_show(tb_profile->canvas);
  table_row++;

  /* draw a black background on the canvas */
  gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(tb_profile->canvas)),
			gnome_canvas_rect_get_type(),
			"x1",0.0, "y1", 0.0, 
			"x2", (gdouble) CANVAS_WIDTH, "y2", (gdouble) CANVAS_HEIGHT,
			"fill_color_rgba", 0x000000FF,
			NULL);

  
  /* the fit results */
#ifdef AMIDE_LIBGSL_SUPPORT
  check_button = gtk_check_button_new_with_label (_("calculate gaussian fit"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), tb_profile->calc_gaussian_fit);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(calc_gaussian_fit_cb), tb_profile);
  gtk_table_attach(GTK_TABLE(table), check_button,0,3,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(check_button);
  table_row++;

  check_button = gtk_check_button_new_with_label (_("fix x location (c)"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), tb_profile->fix_x);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(fix_x_cb), tb_profile);
  gtk_table_attach(GTK_TABLE(table), check_button,0,3,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(check_button);
  table_row++;

  check_button = gtk_check_button_new_with_label (_("fix dc value to zero (b)"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), tb_profile->fix_dc_zero);
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(fix_dc_zero_cb), tb_profile);
  gtk_table_attach(GTK_TABLE(table), check_button,0,3,
		   table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(check_button);
  table_row++;

  tb_profile->text = gtk_text_view_new ();
  gtk_table_attach(GTK_TABLE(table), tb_profile->text, 0, 3, table_row, table_row+1, 
		   X_PACKING_OPTIONS | GTK_FILL, Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);
  gtk_widget_modify_font (tb_profile->text, amitk_fixed_font_desc);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(tb_profile->text), FALSE);
  gtk_widget_show(tb_profile->text);
  table_row++;

  /* set it to black */
  gdk_color_parse ("black", &color);
  gtk_widget_modify_base(tb_profile->text, GTK_STATE_NORMAL, &color);
  gdk_color_parse ("white", &color);
  gtk_widget_modify_text (tb_profile->text, GTK_STATE_NORMAL, &color);
#endif

  /* and show all our widgets */
  gtk_widget_show(dialog);

  /* and make sure they have the right stuff in them */
  recalc_profiles(tb_profile);

  return;
}













