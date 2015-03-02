/* ui_preferences_dialog.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2004 Andy Loening
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
#ifndef AMIDE_WIN32_HACKS
#include <libgnome/libgnome.h>
#endif
#include "ui_study.h"
#include "ui_preferences_dialog.h"
#include "pixmaps.h"
#include "amitk_canvas.h"
#include "amitk_color_table_menu.h"
#include "amitk_threshold.h"
#include "ui_common.h"


static gchar * study_preference_text = 
N_("These preferences are used only for new studies.  \n"
   "Use the study modification dialog to change these \n"
   "parameters for the current study.");

static gchar * data_set_preference_text = 
N_("These preferences are used only for new data sets.  \n"
   "Use the data set modification dialog to change these \n"
   "parameters for the current data set.");



#ifndef AMIDE_WIN32_HACKS /* don't get saved anyway, so no sense in showing them */
static void roi_width_cb(GtkWidget * widget, gpointer data);
#ifndef AMIDE_LIBGNOMECANVAS_AA
static void line_style_cb(GtkWidget * widget, gpointer data);
#endif
static void fill_isocontour_cb(GtkWidget * widget, gpointer data);
static void layout_cb(GtkWidget * widget, gpointer data);
static void maintain_size_cb(GtkWidget * widget, gpointer data);
static void target_empty_area_cb(GtkWidget * widget, gpointer data);
static void window_threshold_cb(GtkWidget * widget, gpointer data);
#endif

static void warnings_to_console_cb(GtkWidget * widget, gpointer data);
static void save_on_exit_cb(GtkWidget * widget, gpointer data);
static void save_xif_as_directory_cb(GtkWidget * widget, gpointer data);
static void response_cb (GtkDialog * dialog, gint response_id, gpointer data);
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer preferences);


#ifndef AMIDE_WIN32_HACKS

/* function called when the roi width has been changed */
static void roi_width_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  gint new_roi_width;
  GnomeCanvasItem * roi_item;

  g_return_if_fail(ui_study->study != NULL);

  new_roi_width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  amitk_preferences_set_canvas_roi_width(ui_study->preferences, new_roi_width);

  /* update the roi indicator, don't use new_roi_width, as value may be
     rejected by amitk_preferences_set_canvas_roi_width */
  roi_item = g_object_get_data(G_OBJECT(ui_study->preferences->dialog), "roi_item");
  gnome_canvas_item_set(roi_item, "width_pixels", 
			AMITK_PREFERENCES_CANVAS_ROI_WIDTH(ui_study->preferences), NULL);

  return;
}


#ifndef AMIDE_LIBGNOMECANVAS_AA
/* function to change the line style */
static void line_style_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study=data;
  GdkLineStyle new_line_style;
  GnomeCanvasItem * roi_item;

  g_return_if_fail(ui_study->study != NULL);

  /* figure out which menu item called me */
#if 1
  new_line_style = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));
#else
  new_line_style = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
#endif
  amitk_preferences_set_canvas_line_style(ui_study->preferences, new_line_style);

  /* update the roi indicator */
  roi_item = g_object_get_data(G_OBJECT(ui_study->preferences->dialog), "roi_item");
  gnome_canvas_item_set(roi_item, "line_style", new_line_style, NULL);

  return;
}
#endif

static void fill_isocontour_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  gboolean fill_isocontour;

  fill_isocontour = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  amitk_preferences_set_canvas_fill_isocontour(ui_study->preferences, fill_isocontour);

  return;
}

/* function called to change the layout */
static void layout_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  AmitkLayout new_layout;

  new_layout = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "layout"));
  amitk_preferences_set_canvas_layout(ui_study->preferences, new_layout);

  return;
}

static void maintain_size_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  gboolean canvas_maintain_size;

  canvas_maintain_size = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  amitk_preferences_set_canvas_maintain_size(ui_study->preferences, canvas_maintain_size);

  return;
}


/* function called when the roi width has been changed */
static void target_empty_area_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  gint new_target_empty_area;
  
  new_target_empty_area = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  amitk_preferences_set_canvas_target_empty_area(ui_study->preferences, 
						 new_target_empty_area);
  return;
}

/* function called when the roi width has been changed */
static void window_threshold_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  amide_data_t new_threshold;
  AmitkWindow which_window;
  AmitkLimit which_limit;

  g_return_if_fail(ui_study->study != NULL);

  new_threshold = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  which_window = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "which_window"));
  which_limit = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "which_limit"));

  amitk_preferences_set_default_window(ui_study->preferences, which_window, 
				       which_limit, new_threshold);
  return;
}


#endif /* AMIDE_WIN32_HACKS */



static void warnings_to_console_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  gboolean warnings_to_console;

  warnings_to_console = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  amitk_preferences_set_warnings_to_console(ui_study->preferences, warnings_to_console);

  return;
}


static void save_on_exit_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  gboolean prompt_for_save_on_exit;

  prompt_for_save_on_exit = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  amitk_preferences_set_prompt_for_save_on_exit(ui_study->preferences, prompt_for_save_on_exit);

  return;
}

static void save_xif_as_directory_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  gboolean save_as_directory;

  save_as_directory = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  amitk_preferences_set_xif_as_directory(ui_study->preferences, save_as_directory);

  return;
}



/* changing the color table of a rendering context */
static void color_table_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study=data;
  AmitkModality modality;
  AmitkColorTable color_table;

  modality = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "modality"));
#if 1
  color_table = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));
#else
  color_table = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
#endif
  amitk_preferences_set_color_table(ui_study->preferences, modality, color_table);

  return;
}

static void response_cb (GtkDialog * dialog, gint response_id, gpointer data) {
  
  gint return_val;

  switch(response_id) {

  case GTK_RESPONSE_HELP:
    amide_call_help("preferences-dialog");
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

/* function called to destroy the preferences dialog */
gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {

  AmitkPreferences * preferences = data;

  amitk_preferences_set_dialog(preferences, NULL);

  return FALSE;
}




/* function that sets up the preferences dialog */
void ui_preferences_dialog_create(ui_study_t * ui_study) {
  
  GtkWidget * dialog;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * label;
  GtkWidget * check_button;
  GtkWidget * notebook;
  guint table_row;

#ifndef AMIDE_WIN32_HACKS
  GtkWidget * maintain_size_button;
  GtkWidget * roi_width_spin;
  GtkWidget * target_size_spin;
  GtkWidget * line_style_menu;
  GtkWidget * fill_isocontour_button;
  GtkWidget * layout_button1;
  GtkWidget * layout_button2;
  GtkWidget * hseparator;
  GtkWidget * menu;
  GtkWidget * window_spins[AMITK_WINDOW_NUM][AMITK_LIMIT_NUM];
  AmitkModality i_modality;
  GnomeCanvasItem * roi_item;
  AmitkWindow i_window;
  AmitkLimit i_limit;
#endif




  /* sanity checks */
  g_return_if_fail(ui_study != NULL);
  g_return_if_fail(ui_study->preferences != NULL);
  
  /* only have one preference dialog */
  if (AMITK_PREFERENCES_DIALOG(ui_study->preferences) != NULL) {
    g_warning("A Preferences Dialog is already open");
    return;
  }
    
  temp_string = g_strdup_printf(_("%s: Preferences Dialog"), PACKAGE);
  dialog = gtk_dialog_new_with_buttons (temp_string,  GTK_WINDOW(ui_study->app),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_HELP, GTK_RESPONSE_HELP,
					GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					NULL);
  gtk_window_set_title(GTK_WINDOW(dialog), temp_string);
  g_free(temp_string);
  amitk_preferences_set_dialog(ui_study->preferences, dialog);

  /* setup the callbacks for the dialog */
  g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(response_cb), ui_study);
  g_signal_connect(G_OBJECT(dialog), "delete_event", G_CALLBACK(delete_event_cb), ui_study->preferences);

  notebook = gtk_notebook_new();
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), notebook);

  
#ifndef AMIDE_WIN32_HACKS

  /* ---------------------------
     ROI/Canvas page 
     --------------------------- */


  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(4,5,FALSE);
  label = gtk_label_new(_("ROI/View Preferences"));
  table_row=0;
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), packing_table, label);


  /* warn that these preferences are only for new stuff */
  label = gtk_label_new(study_preference_text);
  gtk_table_attach(GTK_TABLE(packing_table), label, 
		   0,3, table_row, table_row+1,
		   0, 0, X_PADDING, Y_PADDING);
  table_row++;

  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator, 
		   0, 3, table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  ui_common_study_preferences_widgets(packing_table, table_row,
				      &roi_width_spin, &roi_item, &line_style_menu, &fill_isocontour_button,
				      &layout_button1, &layout_button2, &maintain_size_button,
				      &target_size_spin);


  gtk_spin_button_set_value(GTK_SPIN_BUTTON(roi_width_spin), 
			    AMITK_PREFERENCES_CANVAS_ROI_WIDTH(ui_study->preferences));
  g_signal_connect(G_OBJECT(roi_width_spin), "value_changed",  G_CALLBACK(roi_width_cb), ui_study);

  gnome_canvas_item_set(roi_item, 
			"width_pixels", AMITK_PREFERENCES_CANVAS_ROI_WIDTH(ui_study->preferences),
			"line_style", AMITK_PREFERENCES_CANVAS_LINE_STYLE(ui_study->preferences),
			NULL);
  g_object_set_data(G_OBJECT(dialog), "roi_item", roi_item);

#ifndef AMIDE_LIBGNOMECANVAS_AA
#if 1
  gtk_option_menu_set_history(GTK_OPTION_MENU(line_style_menu),
			      AMITK_PREFERENCES_CANVAS_LINE_STYLE(ui_study->preferences));
#else
  gtk_combo_box_set_active(GTK_COMBO_BOX(line_style_menu),
			   AMITK_PREFERENCES_CANVAS_LINE_STYLE(ui_study->preferences));
#endif
  g_signal_connect(G_OBJECT(line_style_menu), "changed", G_CALLBACK(line_style_cb), ui_study);
#endif

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fill_isocontour_button), 
			       AMITK_PREFERENCES_CANVAS_FILL_ISOCONTOUR(ui_study->preferences));
  g_signal_connect(G_OBJECT(fill_isocontour_button), "toggled", G_CALLBACK(fill_isocontour_cb), ui_study);


  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(layout_button1), 
			       (AMITK_PREFERENCES_CANVAS_LAYOUT(ui_study->preferences) == AMITK_LAYOUT_LINEAR));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(layout_button2), 
			       (AMITK_PREFERENCES_CANVAS_LAYOUT(ui_study->preferences) == AMITK_LAYOUT_ORTHOGONAL));
  g_signal_connect(G_OBJECT(layout_button1), "clicked", G_CALLBACK(layout_cb), ui_study);
  g_signal_connect(G_OBJECT(layout_button2), "clicked", G_CALLBACK(layout_cb), ui_study);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(maintain_size_button), 
			       AMITK_PREFERENCES_CANVAS_MAINTAIN_SIZE(ui_study->preferences));
  g_signal_connect(G_OBJECT(maintain_size_button), "toggled", G_CALLBACK(maintain_size_cb), ui_study);

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(target_size_spin), 
			    AMITK_PREFERENCES_CANVAS_TARGET_EMPTY_AREA(ui_study->preferences));
  g_signal_connect(G_OBJECT(target_size_spin), "value_changed",  G_CALLBACK(target_empty_area_cb), ui_study);






  /* ----------------------
     Threshold Windows
     ---------------------- */

  packing_table = gtk_table_new(4,2,FALSE);
  label = gtk_label_new(_("Threshold Windows"));
  table_row=0;
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), packing_table, label);

  /* warn that these preferences are only for new stuff */
  label = gtk_label_new(data_set_preference_text);
  gtk_table_attach(GTK_TABLE(packing_table), label, 
		   0,3, table_row, table_row+1,
		   0, 0, X_PADDING, Y_PADDING);
  table_row++;

  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator, 
		   0, 3, table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  ui_common_data_set_preferences_widgets(packing_table, table_row,
					 window_spins);
  
  for (i_window=0; i_window<AMITK_WINDOW_NUM; i_window++) {
    for (i_limit=0; i_limit< AMITK_LIMIT_NUM; i_limit++) {
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(window_spins[i_window][i_limit]), 
				AMITK_PREFERENCES_DEFAULT_WINDOW(ui_study->preferences, i_window, i_limit));
      g_signal_connect(G_OBJECT(window_spins[i_window][i_limit]), "value_changed",  
		       G_CALLBACK(window_threshold_cb), ui_study);
    }
  }


  /* ----------------------
     Default color tables 
     ---------------------- */
  packing_table = gtk_table_new(4,2,FALSE);
  label = gtk_label_new(_("Default Color Tables"));
  table_row=0;
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), packing_table, label);

  for (i_modality=0; i_modality < AMITK_MODALITY_NUM; i_modality++) {

    /* color table selector */
    temp_string = g_strdup_printf(_("default %s color table:"), 
				  amitk_modality_get_name(i_modality));
    label = gtk_label_new(temp_string);
    g_free(temp_string);
    gtk_table_attach(GTK_TABLE(packing_table), label, 0,1, table_row,table_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
    gtk_widget_show(label);

    menu = amitk_color_table_menu_new();
    gtk_table_attach(GTK_TABLE(packing_table), menu, 1,2, table_row,table_row+1,
		     X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
#if 1
    gtk_option_menu_set_history(GTK_OPTION_MENU(menu),
				AMITK_PREFERENCES_DEFAULT_COLOR_TABLE(ui_study->preferences, i_modality));
#else
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu),
			     AMITK_PREFERENCES_DEFAULT_COLOR_TABLE(ui_study->preferences, i_modality));
#endif
    g_object_set_data(G_OBJECT(menu), "modality", GINT_TO_POINTER(i_modality));
    g_signal_connect(G_OBJECT(menu), "changed",  G_CALLBACK(color_table_cb), ui_study);
    gtk_widget_show(menu);

    table_row++;
  }

#endif /* AMIDE_WIN32_HACKS */


  /* ---------------------------
     Miscellaneous stuff
     --------------------------- */


  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(4,5,FALSE);
  label = gtk_label_new(_("Miscellaneous"));
  table_row=0;
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), packing_table, label);

  check_button = gtk_check_button_new_with_label(_("Send Warning Messages to Console:"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), 
			       AMITK_PREFERENCES_WARNINGS_TO_CONSOLE(ui_study->preferences));
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(warnings_to_console_cb), ui_study);
  gtk_table_attach(GTK_TABLE(packing_table), check_button, 
		   0,1, table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  check_button = gtk_check_button_new_with_label(_("Don't Prompt for \"Save Changes\" on Exit:"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), 
			       AMITK_PREFERENCES_PROMPT_FOR_SAVE_ON_EXIT(ui_study->preferences));
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(save_on_exit_cb), ui_study);
  gtk_table_attach(GTK_TABLE(packing_table), check_button, 
		   0,1, table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  check_button = gtk_check_button_new_with_label(_("Save .XIF file as directory:"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), 
			       AMITK_PREFERENCES_SAVE_XIF_AS_DIRECTORY(ui_study->preferences));
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(save_xif_as_directory_cb), ui_study);
  gtk_table_attach(GTK_TABLE(packing_table), check_button, 
		   0,1, table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  /* and show all our widgets */
  gtk_widget_show_all(dialog);

  return;
}











