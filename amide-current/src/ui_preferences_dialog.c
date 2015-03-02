/* ui_preferences_dialog.c
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


#include "amide_config.h"
#include "amide.h"
#include "ui_study.h"
#include "ui_preferences_dialog.h"
#include "amitk_canvas.h"
#include "amitk_color_table_menu.h"
#include "amitk_threshold.h"
#include "amitk_window_edit.h"
#include "ui_common.h"


static gchar * study_preference_text = 
N_("These preferences are used only for new studies.  \n"
   "Use the study modification dialog to change these \n"
   "parameters for the current study.");

static gchar * data_set_preference_text = 
N_("These preferences are used only for new data sets.  \n"
   "Use the data set modification dialog to change these \n"
   "parameters for the current data set.");


static void update_roi_sample_item(ui_study_t * ui_study);
static void roi_width_cb(GtkWidget * widget, gpointer data);
#ifdef AMIDE_LIBGNOMECANVAS_AA
static void roi_transparency_cb(GtkWidget * widget, gpointer data);
#else
static void line_style_cb(GtkWidget * widget, gpointer data);
static void fill_roi_cb(GtkWidget * widget, gpointer data);
#endif
static void layout_cb(GtkWidget * widget, gpointer data);
static void panel_layout_cb(GtkWidget * widget, gpointer data);
static void maintain_size_cb(GtkWidget * widget, gpointer data);
static void target_empty_area_cb(GtkWidget * widget, gpointer data);
static void threshold_style_cb(GtkWidget * widget, gpointer data);

static void warnings_to_console_cb(GtkWidget * widget, gpointer data);
static void save_on_exit_cb(GtkWidget * widget, gpointer data);
static void which_default_directory_cb(GtkWidget * widget, gpointer data);
static void default_directory_cb(GtkWidget * fc, gpointer data);
static void response_cb (GtkDialog * dialog, gint response_id, gpointer data);
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer preferences);


/* function called to update the roi sampe item */
static void update_roi_sample_item(ui_study_t * ui_study) {
  GnomeCanvasItem * roi_item;

  roi_item = g_object_get_data(G_OBJECT(ui_study->preferences->dialog), "roi_item");
  ui_common_update_sample_roi_item(roi_item,
				   AMITK_PREFERENCES_CANVAS_ROI_WIDTH(ui_study->preferences),
#ifdef AMIDE_LIBGNOMECANVAS_AA
				   AMITK_PREFERENCES_CANVAS_ROI_TRANSPARENCY(ui_study->preferences)
#else
				   AMITK_PREFERENCES_CANVAS_LINE_STYLE(ui_study->preferences)
#endif
				   );

  return;
}

/* function called when the roi width has been changed */
static void roi_width_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  gint new_roi_width;

  g_return_if_fail(ui_study->study != NULL);

  new_roi_width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  amitk_preferences_set_canvas_roi_width(ui_study->preferences, new_roi_width);
  update_roi_sample_item(ui_study);

  return;
}


#ifdef AMIDE_LIBGNOMECANVAS_AA
/* function to change the roi internal transparency */
static void roi_transparency_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study=data;
  gdouble new_transparency;

  g_return_if_fail(ui_study->study != NULL);

  new_transparency = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
  amitk_preferences_set_canvas_roi_transparency(ui_study->preferences, new_transparency);
  update_roi_sample_item(ui_study);

  return;
}

#else
/* function to change the line style */
static void line_style_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study=data;
  GdkLineStyle new_line_style;
  GnomeCanvasItem * roi_item;

  g_return_if_fail(ui_study->study != NULL);

  /* figure out which menu item called me */
  new_line_style = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  amitk_preferences_set_canvas_line_style(ui_study->preferences, new_line_style);
  update_roi_sample_item(ui_study);

  return;
}

static void fill_roi_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  amitk_preferences_set_canvas_fill_roi(ui_study->preferences, 
					gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));

  return;
}
#endif

/* function called to change the layout */
static void layout_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  amitk_preferences_set_canvas_layout(ui_study->preferences, 
				      GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "layout")));
  return;
}

/* function called to change the panel layout */
static void panel_layout_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  amitk_preferences_set_panel_layout(ui_study->preferences, 
				     GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "panel_layout")));

  return;
}

static void maintain_size_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  amitk_preferences_set_canvas_maintain_size(ui_study->preferences, 
					     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
  return;
}


/* function called when the roi width has been changed */
static void target_empty_area_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  amitk_preferences_set_canvas_target_empty_area(ui_study->preferences, 
						 gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget)));
  return;
}

static void threshold_style_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  amitk_preferences_set_threshold_style(ui_study->preferences, 
					GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "threshold_style")));

  return;
}


static void warnings_to_console_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  amitk_preferences_set_warnings_to_console(ui_study->preferences, 
					    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
  return;
}


static void save_on_exit_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  amitk_preferences_set_prompt_for_save_on_exit(ui_study->preferences, 
						gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));

  return;
}

static void which_default_directory_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study = data;
  amitk_preferences_set_which_default_directory(ui_study->preferences, 
						gtk_combo_box_get_active(GTK_COMBO_BOX(widget)));
  return;
}


static void default_directory_cb(GtkWidget * fc, gpointer data) { 

  ui_study_t * ui_study = data;
  gchar * str;

  str = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fc));
  amitk_preferences_set_default_directory(ui_study->preferences, str);
  g_free(str);

  return;
}


/* changing the color table of a rendering context */
static void color_table_cb(GtkWidget * widget, gpointer data) {

  ui_study_t * ui_study=data;
  AmitkModality modality;
  AmitkColorTable color_table;

  modality = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "modality"));
  color_table = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
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

  GtkWidget * maintain_size_button;
  GtkWidget * roi_width_spin;
  GtkWidget * target_size_spin;
#ifdef AMIDE_LIBGNOMECANVAS_AA
  GtkWidget * roi_transparency_spin;
#else
  GtkWidget * line_style_menu;
  GtkWidget * fill_roi_button;
#endif
  GtkWidget * layout_button1;
  GtkWidget * layout_button2;
  GtkWidget * panel_layout_button1;
  GtkWidget * panel_layout_button2;
  GtkWidget * panel_layout_button3;
  GtkWidget * hseparator;
  GtkWidget * menu;
  GtkWidget * windows_widget;
  GtkWidget * scrolled;
  GtkWidget * entry;
  AmitkModality i_modality;
  AmitkWhichDefaultDirectory i_which_default_directory;
  GnomeCanvasItem * roi_item;
  AmitkThresholdStyle i_threshold_style;
  GtkWidget * style_buttons[AMITK_THRESHOLD_STYLE_NUM];
  GtkWidget * hbox;

  /* sanity checks */
  g_return_if_fail(ui_study != NULL);
  g_return_if_fail(ui_study->preferences != NULL);
  
  /* only have one preference dialog */
  if (AMITK_PREFERENCES_DIALOG(ui_study->preferences) != NULL) {
    g_warning("A Preferences Dialog is already open");
    return;
  }
    
  temp_string = g_strdup_printf(_("%s: Preferences Dialog"), PACKAGE);
  dialog = gtk_dialog_new_with_buttons (temp_string,  ui_study->window,
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
				      &roi_width_spin, &roi_item, 
#ifdef AMIDE_LIBGNOMECANVAS_AA
				      &roi_transparency_spin,
#else
				      &line_style_menu, &fill_roi_button,
#endif
				      &layout_button1, &layout_button2, 
				      &panel_layout_button1,&panel_layout_button2,&panel_layout_button3,
				      &maintain_size_button,
				      &target_size_spin);


  gtk_spin_button_set_value(GTK_SPIN_BUTTON(roi_width_spin), 
			    AMITK_PREFERENCES_CANVAS_ROI_WIDTH(ui_study->preferences));
  g_signal_connect(G_OBJECT(roi_width_spin), "value_changed",  G_CALLBACK(roi_width_cb), ui_study);

  /* update the sample roi display */
  g_object_set_data(G_OBJECT(dialog), "roi_item", roi_item);
  update_roi_sample_item(ui_study);

#ifdef AMIDE_LIBGNOMECANVAS_AA
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(roi_transparency_spin),
			    AMITK_PREFERENCES_CANVAS_ROI_TRANSPARENCY(ui_study->preferences));
  g_signal_connect(G_OBJECT(roi_transparency_spin), "value_changed",  G_CALLBACK(roi_transparency_cb), ui_study);
#else
  gtk_combo_box_set_active(GTK_COMBO_BOX(line_style_menu),
			   AMITK_PREFERENCES_CANVAS_LINE_STYLE(ui_study->preferences));
  g_signal_connect(G_OBJECT(line_style_menu), "changed", G_CALLBACK(line_style_cb), ui_study);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fill_roi_button), 
			       AMITK_PREFERENCES_CANVAS_FILL_ROI(ui_study->preferences));
  g_signal_connect(G_OBJECT(fill_roi_button), "toggled", G_CALLBACK(fill_roi_cb), ui_study);
#endif


  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(layout_button1), 
			       (AMITK_PREFERENCES_CANVAS_LAYOUT(ui_study->preferences) == AMITK_LAYOUT_LINEAR));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(layout_button2), 
			       (AMITK_PREFERENCES_CANVAS_LAYOUT(ui_study->preferences) == AMITK_LAYOUT_ORTHOGONAL));
  g_signal_connect(G_OBJECT(layout_button1), "clicked", G_CALLBACK(layout_cb), ui_study);
  g_signal_connect(G_OBJECT(layout_button2), "clicked", G_CALLBACK(layout_cb), ui_study);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(panel_layout_button1), 
			       (AMITK_PREFERENCES_PANEL_LAYOUT(ui_study->preferences) == AMITK_PANEL_LAYOUT_MIXED));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(panel_layout_button2), 
			       (AMITK_PREFERENCES_PANEL_LAYOUT(ui_study->preferences) == AMITK_PANEL_LAYOUT_LINEAR_X));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(panel_layout_button3), 
			       (AMITK_PREFERENCES_PANEL_LAYOUT(ui_study->preferences) == AMITK_PANEL_LAYOUT_LINEAR_Y));
  g_signal_connect(G_OBJECT(panel_layout_button1), "clicked", G_CALLBACK(panel_layout_cb), ui_study);
  g_signal_connect(G_OBJECT(panel_layout_button2), "clicked", G_CALLBACK(panel_layout_cb), ui_study);
  g_signal_connect(G_OBJECT(panel_layout_button3), "clicked", G_CALLBACK(panel_layout_cb), ui_study);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(maintain_size_button), 
			       AMITK_PREFERENCES_CANVAS_MAINTAIN_SIZE(ui_study->preferences));
  g_signal_connect(G_OBJECT(maintain_size_button), "toggled", G_CALLBACK(maintain_size_cb), ui_study);

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(target_size_spin), 
			    AMITK_PREFERENCES_CANVAS_TARGET_EMPTY_AREA(ui_study->preferences));
  g_signal_connect(G_OBJECT(target_size_spin), "value_changed",  G_CALLBACK(target_empty_area_cb), ui_study);

  gtk_widget_show_all(packing_table);





  /* ----------------------
     Threshold Windows
     ---------------------- */

  packing_table = gtk_table_new(4,2,FALSE);
  label = gtk_label_new(_("Thresholding"));
  table_row=0;
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), packing_table, label);

  /* warn that these preferences are only for new stuff */
  label = gtk_label_new(data_set_preference_text);
  gtk_table_attach(GTK_TABLE(packing_table), label, 
		   0,3, table_row, table_row+1,
		   0, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(label);
  table_row++;

  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(packing_table), hseparator, 
		   0, 3, table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(hseparator);
  table_row++;

  
  /* threshold type selection */
  label = gtk_label_new(_("Threshold Style"));
  gtk_table_attach(GTK_TABLE(packing_table), label, 0,1, table_row,table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_widget_show(label);
    
  hbox = gtk_hbox_new(FALSE, 0);
  amitk_threshold_style_widgets(style_buttons, hbox);
  gtk_table_attach(GTK_TABLE(packing_table), hbox, 1,2, table_row,table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(style_buttons[AMITK_PREFERENCES_THRESHOLD_STYLE(ui_study->preferences)]),
			       TRUE);
  for (i_threshold_style = 0; i_threshold_style < AMITK_THRESHOLD_STYLE_NUM; i_threshold_style++) 
    g_signal_connect(G_OBJECT(style_buttons[i_threshold_style]), "clicked",  
		     G_CALLBACK(threshold_style_cb), ui_study);
  gtk_widget_show(hbox);


  table_row++;


  /* draw the window widgets */
  scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				 GTK_POLICY_NEVER,
				 GTK_POLICY_AUTOMATIC);


  windows_widget = amitk_window_edit_new(NULL, ui_study->preferences);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), windows_widget);
  gtk_widget_show(windows_widget);

  gtk_table_attach(GTK_TABLE(packing_table), scrolled, 0,2, table_row,table_row+1,
		   X_PACKING_OPTIONS | GTK_FILL, GTK_FILL|GTK_EXPAND, X_PADDING, Y_PADDING);
  gtk_widget_show(scrolled);

  gtk_widget_show(packing_table);



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
    gtk_combo_box_set_active(GTK_COMBO_BOX(menu),
			     AMITK_PREFERENCES_COLOR_TABLE(ui_study->preferences, i_modality));
    g_object_set_data(G_OBJECT(menu), "modality", GINT_TO_POINTER(i_modality));
    g_signal_connect(G_OBJECT(menu), "changed",  G_CALLBACK(color_table_cb), ui_study);
    gtk_widget_show(menu);

    table_row++;
  }

  gtk_widget_show_all(packing_table);


  /* ---------------------------
     Miscellaneous stuff
     --------------------------- */


  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(4,5,FALSE);
  label = gtk_label_new(_("Miscellaneous"));
  table_row=0;
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), packing_table, label);

  label = gtk_label_new(_("Send Warning Messages to Console:"));
  gtk_table_attach(GTK_TABLE(packing_table), label, 
		   0,1, table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);

  check_button = gtk_check_button_new();
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), 
			       AMITK_PREFERENCES_WARNINGS_TO_CONSOLE(ui_study->preferences));
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(warnings_to_console_cb), ui_study);
  gtk_table_attach(GTK_TABLE(packing_table), check_button, 
		   1,2, table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  label = gtk_label_new(_("Prompt for \"Save Changes\" on Exit:"));
  gtk_table_attach(GTK_TABLE(packing_table), label, 
		   0,1, table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);

  check_button = gtk_check_button_new();
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), 
			       AMITK_PREFERENCES_PROMPT_FOR_SAVE_ON_EXIT(ui_study->preferences));
  g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(save_on_exit_cb), ui_study);
  gtk_table_attach(GTK_TABLE(packing_table), check_button, 
		   1,2, table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  label = gtk_label_new(_("Which Default Directory:"));
  gtk_table_attach(GTK_TABLE(packing_table), label, 
		   0,1, table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);

  menu = gtk_combo_box_new_text();
  for (i_which_default_directory=0; i_which_default_directory < AMITK_WHICH_DEFAULT_DIRECTORY_NUM; i_which_default_directory++)
    gtk_combo_box_append_text(GTK_COMBO_BOX(menu),
			      amitk_which_default_directory_names[i_which_default_directory]);
  gtk_combo_box_set_active(GTK_COMBO_BOX(menu),
			   AMITK_PREFERENCES_WHICH_DEFAULT_DIRECTORY(ui_study->preferences));
  g_signal_connect(G_OBJECT(menu), "changed", G_CALLBACK(which_default_directory_cb), ui_study);
  gtk_table_attach(GTK_TABLE(packing_table), menu, 
		   1,2, table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;


  label = gtk_label_new(_("Specified Directory:"));
  gtk_table_attach(GTK_TABLE(packing_table), label, 
		   0,1, table_row, table_row+1,
		   GTK_FILL, 0, X_PADDING, Y_PADDING);

  entry =  gtk_file_chooser_button_new(_("Default Directory:"),
  				       GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  amitk_preferences_set_file_chooser_directory(ui_study->preferences, entry); /* set the default directory if applicable */
  g_signal_connect(G_OBJECT(entry), "current-folder-changed", G_CALLBACK(default_directory_cb), ui_study);
  gtk_table_attach(GTK_TABLE(packing_table), entry, 
		   1,2, table_row, table_row+1,
		   GTK_FILL|GTK_EXPAND, 0, X_PADDING, Y_PADDING);

  /* make sure what's being shown in the above widget is consistent with what we have,
     this only comes up if we've never set a default directory before */
  if (AMITK_PREFERENCES_DEFAULT_DIRECTORY(ui_study->preferences) == NULL)
    default_directory_cb(entry, ui_study);

  table_row++;

  gtk_widget_show_all(packing_table);

  /* and show all our widgets */
  gtk_widget_show(notebook);
  gtk_widget_show(dialog);

  return;
}











