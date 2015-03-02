/* ui_series.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2014 Andy Loening
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
#include "amide_gconf.h"
#include "amitk_common.h"
#include "amitk_threshold.h"
#include "amitk_progress_dialog.h"
#include "amitk_canvas_object.h"
#include "amitk_tree_view.h"
#include "image.h"
#include "ui_common.h"
#include "ui_series.h"

typedef enum {
  OVER_SPACE,
  OVER_FRAMES,
  OVER_GATES,
  NUM_SERIES_TYPES,
} series_type_t;


/* external variables */
static gchar * series_names[] = {
  N_("over Space"), 
  N_("over Time"),
  N_("over Gates")
};

static gchar * series_explanations[] = {
  N_("Look at a series of images over a spatial dimension"), 
  N_("Look at a series of images over time"), 
  N_("Look at a series of images over gates")
};

#define UPDATE_NONE 0
#define UPDATE_SERIES 0x1
#define GCONF_AMIDE_SERIES "SERIES"



/* ui_series data structures */
typedef struct ui_series_t {
  GtkWindow * window;
  GtkWidget * window_vbox;
  GList * slice_cache;
  gint max_slice_cache_size;
  GList * objects;
  AmitkDataSet * active_ds;
  GtkWidget * canvas;
  gint canvas_height;
  gint canvas_width;
  GnomeCanvasItem ** images;
  GnomeCanvasItem ** captions;
  GList ** items;
  GtkWidget * thresholds_dialog;
  guint num_slices, rows, columns;
  AmitkVolume * volume;
  amide_time_t view_time;
  AmitkFuseType fuse_type;
  amide_real_t pixel_dim;
  series_type_t series_type;
  AmitkPreferences * preferences;
  GtkWidget * progress_dialog;
  gboolean in_generation;
  gboolean quit_generation;
  gint roi_width;
#ifdef AMIDE_LIBGNOMECANVAS_AA
  gdouble roi_transparency;
#else
  GdkLineStyle line_style;
  gboolean fill_roi;
#endif
  gint pixbuf_width;
  gint pixbuf_height;

  /* for "OVER SPACE" series */
  amide_time_t view_duration;
  amide_real_t start_z;
  amide_real_t z_point; /* current slice offset z component*/
  amide_real_t end_z;

  /* for "OVER TIME" series */
  guint view_frame;
  amide_time_t start_time;
  amide_time_t * frame_durations; /* an array of frame durations */

  /* for "OVER GATES" series */
  gint view_gate;
  gint num_gates;

  guint next_update;
  guint idle_handler_id;

  guint reference_count;
} ui_series_t;



static void scroll_change_cb(GtkAdjustment* adjustment, gpointer data);
static void canvas_size_change_cb(GtkWidget * widget, GtkAllocation * allocation, gpointer ui_series);
static void export_cb(GtkAction * action, gpointer data);
static void changed_cb(gpointer dummy, gpointer ui_series);
static void color_table_changed_cb(gpointer dummy, AmitkViewMode view_mode, gpointer ui_series);
static void data_set_invalidate_slice_cache(AmitkDataSet *ds, gpointer ui_series);
static void threshold_cb(GtkAction * action, gpointer data);
static void close_cb(GtkAction * action, gpointer data);
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data);


static void menus_toolbar_create(ui_series_t * ui_series);

static ui_series_t * ui_series_unref(ui_series_t * ui_series);
static ui_series_t * ui_series_init(GtkWindow * window, GtkWidget * window_vbox);
static GtkAdjustment * ui_series_create_scroll_adjustment(ui_series_t * ui_series);
static void add_update(ui_series_t * ui_series);
static gboolean update_immediate(gpointer ui_series);
static void read_series_preferences(series_type_t * series_type, AmitkView * view);


/* function called by the adjustment in charge for scrolling */
static void scroll_change_cb(GtkAdjustment* adjustment, gpointer data) {

  ui_series_t * ui_series = data;

  switch(ui_series->series_type) {
  case OVER_GATES:
    ui_series->view_gate = adjustment->value;
    break;
  case OVER_FRAMES:
    ui_series->view_frame = adjustment->value;
    break;
  case OVER_SPACE:
  default:
    ui_series->z_point = adjustment->value-AMITK_VOLUME_Z_CORNER(ui_series->volume)/2.0;
    break;
  }
  add_update(ui_series);

  return;
}

static void canvas_size_change_cb(GtkWidget * widget, GtkAllocation * allocation, gpointer data) {
  ui_series_t * ui_series = data;

  ui_series->canvas_width = allocation->width;
  ui_series->canvas_height = allocation->height;
  add_update(ui_series);

  return;
}




/* function to save the series as an image */
static void export_cb(GtkAction * action, gpointer data) {
  
  ui_series_t * ui_series = data;
  GList * objects;
  GtkWidget * file_chooser;
  gchar * filename=NULL;
  gchar * data_set_names = NULL;
  GdkPixbuf * pixbuf;

  file_chooser = gtk_file_chooser_dialog_new(_("Export to Image"),
					     GTK_WINDOW(ui_series->window), /* parent window */
					     GTK_FILE_CHOOSER_ACTION_SAVE,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					     NULL);
  gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(file_chooser), TRUE);
  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(file_chooser), TRUE);
  amitk_preferences_set_file_chooser_directory(ui_series->preferences, file_chooser); /* set the default directory if applicable */

  /* take a guess at the filename */
  objects = ui_series->objects;
  while (objects != NULL) {
    if (AMITK_IS_DATA_SET(objects->data)) {
      if (data_set_names == NULL)
	filename = g_strdup(AMITK_OBJECT_NAME(objects->data));
      else
	filename = g_strdup_printf("%s+%s",data_set_names, 
				      AMITK_OBJECT_NAME(objects->data));
      g_free(data_set_names);
      data_set_names = filename;
    }
    objects = objects->next;
  }

  if (data_set_names == NULL) {
    objects = ui_series->objects;
    while (objects != NULL) {
      if (data_set_names == NULL)
	filename = g_strdup(AMITK_OBJECT_NAME(objects->data));
      else
	filename = g_strdup_printf("%s+%s",data_set_names, 
				      AMITK_OBJECT_NAME(objects->data));
      g_free(data_set_names);
      data_set_names = filename;
      objects = objects->next;
    }
  }

  switch(ui_series->series_type) {
  case OVER_GATES:
    filename = g_strdup_printf("%s_gated.jpg",  data_set_names);
    break;
  case OVER_FRAMES:
  case OVER_SPACE:
  default:
    filename = g_strdup_printf("%s.jpg", data_set_names);
    break;
  }
  gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(file_chooser), filename);
  g_free(data_set_names);
  g_free(filename); 

  if (gtk_dialog_run(GTK_DIALOG (file_chooser)) == GTK_RESPONSE_ACCEPT) 
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (file_chooser));
  else
    filename = NULL;
  gtk_widget_destroy(file_chooser);

  if (filename == NULL) return; /* inappropriate name or don't want to overwrite */

  pixbuf = amitk_get_pixbuf_from_canvas(GNOME_CANVAS(ui_series->canvas),0,0,
					ui_series->columns*(ui_series->pixbuf_width+UI_SERIES_R_MARGIN+UI_SERIES_L_MARGIN),
					ui_series->rows*(ui_series->pixbuf_height+UI_SERIES_TOP_MARGIN+UI_SERIES_BOTTOM_MARGIN));

  if (pixbuf == NULL) {
    g_warning(_("ui_series canvas failed to return a valid image"));
    return;
  }
  
  if (gdk_pixbuf_save (pixbuf, filename, "jpeg", NULL, "quality", "100", NULL) == FALSE) {
    g_warning(_("Failure Saving File: %s"),filename);
    return;
  }

  g_object_unref(pixbuf);

  return;
}


/* function called when a data set changed */
static void changed_cb(gpointer dummy, gpointer data) {
  ui_series_t * ui_series=data;
  add_update(ui_series);
  return;
}

static void color_table_changed_cb(gpointer dummy, AmitkViewMode view_mode, gpointer ui_series) {
  g_return_if_fail(view_mode == AMITK_VIEW_MODE_SINGLE);
  changed_cb(dummy, ui_series);
  return;
}

static void data_set_invalidate_slice_cache(AmitkDataSet *ds, gpointer data) {
  ui_series_t * ui_series=data;

  if (ui_series->slice_cache != NULL) {
    ui_series->slice_cache = amitk_objects_unref(ui_series->slice_cache);
  }

  add_update(ui_series);
  return;
}



static gboolean thresholds_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data) {
  ui_series_t * ui_series = data;

  /* just keeping track on whether or not the threshold widget is up */
  ui_series->thresholds_dialog = NULL;

  return FALSE;
}

/* function called when we hit the threshold button */
static void threshold_cb(GtkAction * action, gpointer data) {

  ui_series_t * ui_series = data;

  if (ui_series->thresholds_dialog != NULL)
    return;

  if (!amitk_objects_has_type(ui_series->objects, AMITK_OBJECT_TYPE_DATA_SET, FALSE)) {
    g_warning(_("No data sets to threshold\n"));
    return;
  }

  ui_common_place_cursor(UI_CURSOR_WAIT, ui_series->canvas);

  ui_series->thresholds_dialog = 
    amitk_thresholds_dialog_new(ui_series->objects, ui_series->window);
  g_signal_connect(G_OBJECT(ui_series->thresholds_dialog), "delete_event",
		   G_CALLBACK(thresholds_delete_event), ui_series);
  gtk_widget_show(ui_series->thresholds_dialog);

  ui_common_remove_wait_cursor(ui_series->canvas);

  return;
}



/* function ran when closing a series window */
static void close_cb(GtkAction * action, gpointer data) {

  ui_series_t * ui_series = data;
  GtkWindow * window = ui_series->window;
  gboolean return_val;

  /* run the delete event function */
  g_signal_emit_by_name(G_OBJECT(window), "delete_event", NULL, &return_val);
  if (!return_val) gtk_widget_destroy(GTK_WIDGET(window));

  return;
}

/* function to run for a delete_event */
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_series_t * ui_series = data;
  GtkWindow * window = ui_series->window;

  /* trying to close while we're generating */
  if (ui_series->in_generation) {
    ui_series->quit_generation=TRUE;
    return TRUE;
  }

  /* free the associated data structure */
  ui_series = ui_series_unref(ui_series);

  /* tell the rest of the program this window is no longer here */
  amide_unregister_window((gpointer) window);

  return FALSE;
}





static const GtkActionEntry normal_items[] = {
  /* Toplevel */
  { "FileMenu", NULL, N_("_File") },
  { "HelpMenu", NULL, N_("_Help") },
  
  /* File menu */
  { "ExportSeries", NULL, N_("_Export Series"), NULL, N_("Export the series to a JPEG image file"), G_CALLBACK(export_cb)},
  { "Close", GTK_STOCK_CLOSE, NULL, "<control>W", N_("Close the series dialog"), G_CALLBACK (close_cb)},
  
  /* Toolbar items */
  { "Threshold", "amide_icon_thresholding", N_("Threshold"), NULL, N_("Set the thresholds and colormaps for the data sets in the series view"), G_CALLBACK(threshold_cb)}
};

static const char *ui_description =
"<ui>"
"  <menubar name='MainMenu'>"
"    <menu action='FileMenu'>"
"      <menuitem action='ExportSeries'/>"
"      <separator/>"
"      <menuitem action='Close'/>"
"    </menu>"
HELP_MENU_UI_DESCRIPTION
"  </menubar>"
"  <toolbar name='ToolBar'>"
"    <toolitem action='Threshold'/>"
"  </toolbar>"
"</ui>";

/* function to setup the menus for the series ui */
static void menus_toolbar_create(ui_series_t * ui_series) {

  GtkWidget *menubar;
  GtkWidget *toolbar;
  GtkActionGroup *action_group;
  GtkUIManager *ui_manager;
  GtkAccelGroup *accel_group;
  GError *error;
      
  /* sanity check */
  g_assert(ui_series!=NULL);

  /* create an action group with all the menu actions */
  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions(action_group, normal_items, G_N_ELEMENTS(normal_items),ui_series);
  gtk_action_group_add_actions(action_group, ui_common_help_menu_items, G_N_ELEMENTS(ui_common_help_menu_items),ui_series);

  /* create the ui manager, and add the actions and accel's */
  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (ui_series->window, accel_group);

  /* create the actual menu/toolbar ui */
  error = NULL;
  if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, &error)) {
    g_warning ("%s: building menus failed in %s: %s", PACKAGE, __FILE__, error->message);
    g_error_free (error);
    return;
  }

  /* pack in the menu and toolbar */
  menubar = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
  gtk_box_pack_start (GTK_BOX (ui_series->window_vbox), menubar, FALSE, FALSE, 0);

  toolbar = gtk_ui_manager_get_widget (ui_manager, "/ToolBar");
  gtk_box_pack_start (GTK_BOX (ui_series->window_vbox), toolbar, FALSE, FALSE, 0);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
			      
  return;

}




/* destroy a ui_series data structure */
static ui_series_t * ui_series_unref(ui_series_t * ui_series) {

  GList * temp_objects;
  gboolean return_val;
  gint i;

  g_return_val_if_fail(ui_series != NULL, NULL);

  /* sanity checks */
  g_return_val_if_fail(ui_series->reference_count > 0, NULL);

  /* remove a reference count */
  ui_series->reference_count--;

  /* things to do if we've removed all reference's */
  if (ui_series->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing ui_series\n");
#endif
    if (ui_series->idle_handler_id != 0) {
      g_source_remove(ui_series->idle_handler_id);
      ui_series->idle_handler_id = 0;
    }

    if (ui_series->active_ds != NULL) 
      ui_series->active_ds = amitk_object_unref(ui_series->active_ds);

    if (ui_series->objects != NULL) {

      /* disconnect and signals */
      temp_objects = ui_series->objects;
      while (temp_objects != NULL) {
	if (AMITK_IS_DATA_SET(temp_objects->data)) {
	  g_signal_handlers_disconnect_by_func(G_OBJECT(temp_objects->data),
					       G_CALLBACK(data_set_invalidate_slice_cache), ui_series);
	  g_signal_handlers_disconnect_by_func(G_OBJECT(temp_objects->data),
					       G_CALLBACK(color_table_changed_cb), ui_series);
	}
	g_signal_handlers_disconnect_by_func(G_OBJECT(temp_objects->data),
					     G_CALLBACK(changed_cb), ui_series);

	temp_objects = temp_objects->next;
      }
      amitk_objects_unref(ui_series->objects);
      ui_series->objects = NULL;
    }

    if (ui_series->slice_cache != NULL) {
      ui_series->slice_cache = amitk_objects_unref(ui_series->slice_cache);
    }

    if (ui_series->volume != NULL) {
      amitk_object_unref(ui_series->volume);
      ui_series->volume = NULL;
    }

    if (ui_series->thresholds_dialog != NULL) {
      gtk_widget_destroy(ui_series->thresholds_dialog);
      ui_series->thresholds_dialog = NULL;
    }

    if (ui_series->preferences != NULL) {
      g_object_unref(ui_series->preferences);
      ui_series->preferences = NULL;
    }

    if (ui_series->progress_dialog != NULL) {
      g_signal_emit_by_name(G_OBJECT(ui_series->progress_dialog), "delete_event", NULL, &return_val);
      ui_series->progress_dialog = NULL;
    }

    if (ui_series->images != NULL) {
      g_free(ui_series->images);
      ui_series->images = NULL;
    }

    if (ui_series->captions != NULL) {
      g_free(ui_series->captions);
      ui_series->captions = NULL;
    }

    if (ui_series->items != NULL) {
      for (i=0; i < ui_series->num_slices; i++) {
	if (ui_series->items[i] != NULL) {
	  g_list_free(ui_series->items[i]);
	  ui_series->items[i] = NULL;
	}
      }
      g_free(ui_series->items); 
      ui_series->items = NULL;
    }

    if (ui_series->frame_durations != NULL) {
      g_free(ui_series->frame_durations);
      ui_series->frame_durations = NULL;
    }

    g_free(ui_series);
    ui_series = NULL;
  }

  return ui_series;

}


/* allocate and initialize a ui_series data structure */
static ui_series_t * ui_series_init(GtkWindow * window, GtkWidget * window_vbox) {

  ui_series_t * ui_series;

  /* alloc space for the data structure for passing ui info */
  if ((ui_series = g_try_new(ui_series_t,1)) == NULL) {
    g_warning(_("couldn't allocate memory space for ui_series_t"));
    return NULL;
  }
  ui_series->reference_count = 1;

  /* set any needed parameters */
  ui_series->window = window;
  ui_series->window_vbox = window_vbox;
  ui_series->slice_cache = NULL;
  ui_series->max_slice_cache_size=10;
  ui_series->num_slices = 0;
  ui_series->rows = 0;
  ui_series->columns = 0;
  ui_series->canvas_height = 0;
  ui_series->canvas_width = 1;
  ui_series->images = NULL;
  ui_series->captions = NULL;
  ui_series->items = NULL;
  ui_series->objects = NULL;
  ui_series->active_ds = NULL;
  ui_series->fuse_type = AMITK_FUSE_TYPE_BLEND;
  ui_series->pixel_dim = 1.0;
  ui_series->volume = NULL;
  ui_series->view_time = 0.0;
  ui_series->series_type = OVER_SPACE;
  ui_series->thresholds_dialog = NULL;
  ui_series->preferences = NULL;
  ui_series->progress_dialog = amitk_progress_dialog_new(ui_series->window);
  ui_series->in_generation=FALSE;
  ui_series->quit_generation=FALSE;
  ui_series->roi_width = 1.0;
#ifdef AMIDE_LIBGNOMECANVAS_AA
  ui_series->roi_transparency = 0.5;
#else
  ui_series->line_style = 0;
  ui_series->fill_roi = TRUE;
#endif
  ui_series->pixbuf_width = 0;
  ui_series->pixbuf_height = 0;

  ui_series->view_duration = 1.0;
  ui_series->start_z = 0.0;
  ui_series->end_z = 0.0;
  ui_series->z_point = 0.0;

  ui_series->view_frame = 0;
  ui_series->start_time = 0.0;
  ui_series->frame_durations = NULL;

  ui_series->view_gate = 0;
  ui_series->num_gates = 1;

  ui_series->next_update = UPDATE_NONE;
  ui_series->idle_handler_id = 0;

  return ui_series;
}


/* function to make the adjustments for the scrolling scale */
static GtkAdjustment * ui_series_create_scroll_adjustment(ui_series_t * ui_series) { 

  amide_real_t thickness;
  GtkObject * adjustment;

  switch(ui_series->series_type) {
  case OVER_GATES:
    adjustment = gtk_adjustment_new(ui_series->view_gate, 0, ui_series->num_slices, 1,1,1);
    break;
  case OVER_FRAMES:
    adjustment = gtk_adjustment_new(ui_series->view_frame, 0, ui_series->num_slices, 1,1,1);
    break;
  case OVER_SPACE:
  default:
    thickness = AMITK_VOLUME_Z_CORNER(ui_series->volume);
    adjustment = gtk_adjustment_new(ui_series->z_point-thickness/2.0,
				    ui_series->start_z+thickness/2.0,
				    ui_series->end_z-thickness/2.0,
				    thickness, thickness, thickness);
    break;
  }

  return GTK_ADJUSTMENT(adjustment);
}


static void add_update(ui_series_t * ui_series) {


  ui_series->next_update = ui_series->next_update | UPDATE_SERIES;
  if (ui_series->idle_handler_id == 0) {
    ui_common_place_cursor_no_wait(UI_CURSOR_WAIT, ui_series->canvas);
    ui_series->idle_handler_id = 
      g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,update_immediate, ui_series, NULL);
  }

  return;
}


/* funtion to update the canvas */
static gboolean update_immediate(gpointer data) {

  ui_series_t * ui_series = data;
  AmitkPoint temp_point;
  amide_time_t temp_time, temp_duration;
  gint temp_gate;
  gint i, start_i;
  gdouble x, y;
  AmitkVolume * view_volume;
  gint image_width, image_height;
  gchar * temp_string;
  GdkPixbuf * pixbuf;
  gboolean can_continue=TRUE;
  gboolean return_val = TRUE;
  GList * objects;
  GnomeCanvasItem * item;
  rgba_t outline_color;
  gint rows, columns;

  ui_series->in_generation=TRUE;

  temp_string = g_strdup_printf(_("Slicing for series"));
  amitk_progress_dialog_set_text(AMITK_PROGRESS_DIALOG(ui_series->progress_dialog), temp_string);
  g_free(temp_string);

  /* allocate space for the following if this is the first time through */
  if (ui_series->images == NULL) {
    if ((ui_series->images = g_try_new(GnomeCanvasItem *,ui_series->num_slices)) == NULL) {
      g_warning(_("couldn't allocate memory space for pointers to image GnomeCanvasItem's"));
      return_val = FALSE;
      goto exit_update;
    }
    if ((ui_series->captions = g_try_new(GnomeCanvasItem *,ui_series->num_slices)) == NULL) {
      g_warning(_("couldn't allocate memory space for pointers to caption GnomeCanvasItem's"));
      return_val = FALSE;
      goto exit_update;
    }
    if ((ui_series->items = g_try_new(GList *,ui_series->num_slices)) == NULL) {
      g_warning(_("couldn't allocate memory space for pointers to GnomeCanavasItem lists"));
      return_val = FALSE;
      goto exit_update;
    }
    for (i=0; i < ui_series->num_slices ; i++) {
      ui_series->images[i] = NULL;
      ui_series->captions[i] = NULL;
      ui_series->items[i] = NULL;
    }
  }


  image_width = ui_series->pixbuf_width + UI_SERIES_R_MARGIN + UI_SERIES_L_MARGIN;
  image_height = ui_series->pixbuf_height + UI_SERIES_TOP_MARGIN + UI_SERIES_BOTTOM_MARGIN;

  /* figure out how many images we can display at once */
  columns = floor(ui_series->canvas_width/image_width);
  if (columns < 1) columns = 1;
  rows = floor(ui_series->canvas_height/image_height);
  if (rows < 1) rows = 1;

  /* compensate for cases where we don't need all the rows */
  if ((columns * rows) > ui_series->num_slices)
    rows = ceil((double) ui_series->num_slices/(double) columns);

  /* if we've changed rows or columns, delete prexisting canvas objects */
  if ((ui_series->rows != rows) || (ui_series->columns != columns)) {
    ui_series->rows = rows;
    ui_series->columns = columns;

    for (i=0; i < ui_series->num_slices ; i++) {
      if (ui_series->images[i] != NULL) {
	gtk_object_destroy(GTK_OBJECT(ui_series->images[i]));
	ui_series->images[i] = NULL;
      }
      if (ui_series->captions[i] != NULL) {
	gtk_object_destroy(GTK_OBJECT(ui_series->captions[i]));
	ui_series->captions[i] = NULL;
      }
      if (ui_series->items[i] != NULL) {
	while (ui_series->items[i] != NULL) { 
	  item = ui_series->items[i]->data;
	  ui_series->items[i] = g_list_remove(ui_series->items[i], item);
	  gtk_object_destroy(GTK_OBJECT(item));
	}
	ui_series->items[i] = NULL;
      }
    }

    gnome_canvas_set_scroll_region(GNOME_CANVAS(ui_series->canvas), 0.0, 0.0, 
				   (double) (ui_series->columns*image_width), 
				   (double) (ui_series->rows*image_height));
  }

  /* figure out what's the first image we want to display */
  switch(ui_series->series_type) {
  case OVER_GATES:
    start_i = ui_series->view_gate;
    break;
  case OVER_FRAMES:
    start_i = ui_series->view_frame;
    break;
  case OVER_SPACE:
  default:
    start_i = ui_series->num_slices*((ui_series->z_point-ui_series->start_z)/
				     (ui_series->end_z-ui_series->start_z-AMITK_VOLUME_Z_CORNER(ui_series->volume)));
    break;
  }

  /* correct i for special cases */
  if (ui_series->num_slices < ui_series->columns*ui_series->rows)
    start_i=0;
  else if (start_i < (ui_series->columns*ui_series->rows/2.0))
    start_i=0;
  else if (start_i > (ui_series->num_slices - ui_series->columns*ui_series->rows))
    start_i = ui_series->num_slices - ui_series->columns*ui_series->rows;
  else
    start_i = start_i-ui_series->columns*ui_series->rows/2.0;

  temp_time = ui_series->start_time;
  temp_point = zero_point;
  temp_gate = -1;

  if (ui_series->series_type == OVER_FRAMES) {
    for (i=0;i< start_i; i++)
      temp_time += ui_series->frame_durations[i];
    temp_time -= ui_series->frame_durations[start_i];/* well get added back 1st time through loop */
    temp_duration = ui_series->frame_durations[start_i]; 
  } else {
    temp_time = ui_series->view_time;
    temp_duration = ui_series->view_duration;
  }

  x = y = 0.0;

  view_volume = AMITK_VOLUME(amitk_object_copy(AMITK_OBJECT(ui_series->volume)));

  for (i=start_i; 
       (((i-start_i) < (ui_series->rows*ui_series->columns)) && 
	(i < ui_series->num_slices) &&
	can_continue &&
	(!ui_series->quit_generation));
	i++) {

    switch (ui_series->series_type) {
    case OVER_GATES:
      temp_gate=i;
      break;
    case OVER_FRAMES:
      temp_time += temp_duration; /* duration from last time through the loop */
      temp_duration = ui_series->frame_durations[i];
      break;
    case OVER_SPACE:
    default:
      temp_point.z = i*AMITK_VOLUME_Z_CORNER(ui_series->volume)+ui_series->start_z;
      break;
    }

    amitk_space_set_offset(AMITK_SPACE(view_volume), 
			   amitk_space_s2b(AMITK_SPACE(ui_series->volume), temp_point));
    
    /* figure out the next x,y spot to put this guy */
    y = floor((i-start_i)/ui_series->columns)*image_height;
    x = (i-start_i-ui_series->columns*floor((i-start_i)/ui_series->columns))*image_width;

    if (amitk_objects_has_type(ui_series->objects, AMITK_OBJECT_TYPE_DATA_SET, FALSE)) {
      pixbuf = image_from_data_sets(NULL,
				    &(ui_series->slice_cache),
				    ui_series->max_slice_cache_size,
				    ui_series->objects,
				    ui_series->active_ds,
				    temp_time+EPSILON*fabs(temp_time),
				    temp_duration-EPSILON*fabs(temp_duration),
				    temp_gate,
				    ui_series->pixel_dim,
				    view_volume,
				    ui_series->fuse_type,
				    AMITK_VIEW_MODE_SINGLE);
    
      if (ui_series->images[i-start_i] == NULL) 
	ui_series->images[i-start_i] = 
	  gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(ui_series->canvas)),
				gnome_canvas_pixbuf_get_type(),
				"pixbuf", pixbuf,
				"x", x+UI_SERIES_L_MARGIN,
				"y", y+UI_SERIES_TOP_MARGIN,
				NULL);
      else
	gnome_canvas_item_set(ui_series->images[i-start_i], "pixbuf", pixbuf, NULL);
      g_object_unref(pixbuf);
    }

    /* draw the rest of the objects */
    while (ui_series->items[i-start_i] != NULL) { /* first, delete the old objects */
      item = ui_series->items[i-start_i]->data;
      ui_series->items[i-start_i] = g_list_remove(ui_series->items[i-start_i], item);
      gtk_object_destroy(GTK_OBJECT(item));
    }

    /* add the new item to the canvas */
    objects = ui_series->objects;
    while (objects != NULL) {
      if (AMITK_IS_FIDUCIAL_MARK(objects->data) || AMITK_IS_ROI(objects->data)) {
	if (AMITK_IS_DATA_SET(AMITK_OBJECT_PARENT(objects->data)))
	  outline_color = 
	    amitk_color_table_outline_color(AMITK_DATA_SET_COLOR_TABLE(AMITK_OBJECT_PARENT(objects->data), AMITK_VIEW_MODE_SINGLE), TRUE);
	else
	  outline_color = amitk_color_table_outline_color(AMITK_COLOR_TABLE_BW_LINEAR, TRUE);

	item = amitk_canvas_object_draw(GNOME_CANVAS(ui_series->canvas), 
					view_volume, objects->data,
					AMITK_VIEW_MODE_SINGLE, NULL,
					ui_series->pixel_dim,
					ui_series->pixbuf_width, 
					ui_series->pixbuf_height,
					x+UI_SERIES_L_MARGIN, y+UI_SERIES_TOP_MARGIN,
					outline_color, 
					ui_series->roi_width,
#ifdef AMIDE_LIBGNOMECANVAS_AA
					ui_series->roi_transparency
#else
					ui_series->line_style,
					ui_series->fill_roi
#endif
					);
	if (item != NULL)
	  ui_series->items[i-start_i] = g_list_append(ui_series->items[i-start_i], item);
      }
      objects = objects->next;
    }


    /* write the caption */
    switch (ui_series->series_type) {
    case OVER_GATES:
      temp_string = g_strdup_printf("gate %d", temp_gate);
      break;
    case OVER_FRAMES:
      temp_string = g_strdup_printf("%2.1f-%2.1f s", temp_time, temp_time+temp_duration);
      break;
    case OVER_SPACE:
    default:
      temp_string = g_strdup_printf("%2.1f-%2.1f mm", temp_point.z, temp_point.z+AMITK_VOLUME_Z_CORNER(ui_series->volume));
      break;
    }

    if (ui_series->captions[i-start_i] == NULL) 
      ui_series->captions[i-start_i] =
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(ui_series->canvas)),
			      gnome_canvas_text_get_type(),
			      "justification", GTK_JUSTIFY_LEFT,
			      "anchor", GTK_ANCHOR_NORTH_WEST,
			      "text", temp_string,
			      "x", x+UI_SERIES_L_MARGIN,
			      "y", y+image_height-UI_SERIES_BOTTOM_MARGIN,
			      "fill_color", "black", 
			      "font_desc", amitk_fixed_font_desc, 
			      NULL);
    else
      gnome_canvas_item_set(ui_series->captions[i-start_i],
			    "text", temp_string,
			    NULL);
    g_free(temp_string);

    can_continue = amitk_progress_dialog_set_fraction(AMITK_PROGRESS_DIALOG(ui_series->progress_dialog),
						      (i-start_i)/((gdouble) ui_series->rows*ui_series->columns));
  }
  amitk_object_unref(view_volume);

  return_val = FALSE;


 exit_update:

  amitk_progress_dialog_set_fraction(AMITK_PROGRESS_DIALOG(ui_series->progress_dialog), 2.0); /* hide progress dialog */
  ui_common_remove_wait_cursor(ui_series->canvas);

  ui_series->idle_handler_id=0;
  ui_series->quit_generation=FALSE;
  ui_series->in_generation=FALSE;

  return return_val;
}

static void read_series_preferences(series_type_t * series_type, AmitkView * view) {

  *series_type = amide_gconf_get_int(GCONF_AMIDE_SERIES,"Type");
  *view = amide_gconf_get_int(GCONF_AMIDE_SERIES,"View");

  return;
}

/* function that sets up the series dialog */
void ui_series_create(AmitkStudy * study, 
		      AmitkObject * active_object,
		      GList * selected_objects,
		      AmitkPreferences * preferences) {

 
  ui_series_t * ui_series;
  GtkWindow * window;
  GtkWidget * window_vbox;
  gchar * title = NULL;
  GtkWidget * packing_table;
  GtkAdjustment * adjustment;
  GtkWidget * scale;
  amide_time_t min_duration;
  GList * temp_objects;
  guint num_data_sets = 0;
  guint total_data_set_frames=0;
  gint width, height;
  series_type_t series_type;
  AmitkView view;

  read_series_preferences(&series_type, &view);

  /* sanity checks */
  g_return_if_fail(AMITK_IS_STUDY(study));

  title = g_strdup_printf(_("Series: %s (%s - %s)"), AMITK_OBJECT_NAME(study),
			  amitk_view_get_name(view), _(series_names[series_type]));
  window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
  gtk_window_set_title(window, title);
  g_free(title);
  gtk_window_set_resizable(window, TRUE);
  width = 0.5*gdk_screen_width();
  height = 0.5*gdk_screen_height();
  gtk_window_set_default_size(window, width, height);

  window_vbox = gtk_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (window), window_vbox);

  ui_series = ui_series_init(window, window_vbox);
  ui_series->preferences = g_object_ref(preferences);
  ui_series->series_type = series_type;
#ifdef AMIDE_LIBGNOMECANVAS_AA
  ui_series->roi_transparency = AMITK_STUDY_CANVAS_ROI_TRANSPARENCY(study);
#else
  ui_series->line_style = AMITK_STUDY_CANVAS_LINE_STYLE(study);
  ui_series->fill_roi = AMITK_STUDY_CANVAS_FILL_ROI(study);
#endif
  ui_series->roi_width = AMITK_STUDY_CANVAS_ROI_WIDTH(study);

  ui_series->objects = amitk_objects_ref(selected_objects);
  if (ui_series->objects == NULL) {
    g_warning(_("Need selected objects to create a series"));
    ui_series_unref(ui_series);
    return;
  }

  if (active_object != NULL)
    if (AMITK_IS_DATA_SET(active_object))
      ui_series->active_ds = amitk_object_ref(active_object); /* save a pointer to which object is active */

  /* setup the callbacks for the dialog */
  g_signal_connect(G_OBJECT(ui_series->window), "delete_event", G_CALLBACK(delete_event_cb), ui_series);

  /* save the coordinate space of the series and some other parameters */
  ui_series->volume = amitk_volume_new();
  amitk_space_set_view_space(AMITK_SPACE(ui_series->volume), view, 
			     AMITK_STUDY_CANVAS_LAYOUT(study));
  amitk_volumes_calc_display_volume(selected_objects, AMITK_SPACE(ui_series->volume), 
				    AMITK_STUDY_VIEW_CENTER(study),
				    AMITK_STUDY_VIEW_THICKNESS(study),
				    AMITK_STUDY_FOV(study),
				    ui_series->volume);
  
  ui_series->fuse_type = AMITK_STUDY_FUSE_TYPE(study);
  ui_series->view_time = AMITK_STUDY_VIEW_START_TIME(study);
  min_duration = amitk_data_sets_get_min_frame_duration(ui_series->objects);
  ui_series->view_duration =  
    (min_duration > AMITK_STUDY_VIEW_DURATION(study)) ?  min_duration : AMITK_STUDY_VIEW_DURATION(study);
  ui_series->pixel_dim = (1/AMITK_STUDY_ZOOM(study)) * AMITK_STUDY_VOXEL_DIM(study);

  ui_series->pixbuf_width = ceil(AMITK_VOLUME_X_CORNER(ui_series->volume)/ui_series->pixel_dim);
  ui_series->pixbuf_height = ceil(AMITK_VOLUME_Y_CORNER(ui_series->volume)/ui_series->pixel_dim);

  /* count the number of data sets */
  temp_objects = ui_series->objects;
  while (temp_objects != NULL) {
    if (AMITK_IS_DATA_SET(temp_objects->data)) {
      num_data_sets++;
      total_data_set_frames += AMITK_DATA_SET_NUM_FRAMES(temp_objects->data);
    }
    temp_objects = temp_objects->next;
  }


  /* do some initial calculations */
  switch (ui_series->series_type) {
  case OVER_GATES:

    temp_objects = ui_series->objects;
    ui_series->num_gates = 1;
    while (temp_objects != NULL) {
      if (AMITK_IS_DATA_SET(temp_objects->data)) {
	if (AMITK_DATA_SET_NUM_GATES(temp_objects->data) > ui_series->num_gates)
	  ui_series->num_gates = AMITK_DATA_SET_NUM_GATES(temp_objects->data);
      }
      temp_objects = temp_objects->next;
    }
    ui_series->num_slices = ui_series->num_gates;

    break;
  case OVER_FRAMES:
    {
      amide_time_t current_start = 0.0;
      amide_time_t last_start, current_end, temp_time;
      guint * frames;
      guint i;
      guint series_frame = 0;
      gboolean done;
      gboolean valid;
      
      if (num_data_sets == 0) {
	g_warning(_("Need selected data sets to generate a series of slices over time"));
	ui_series_unref(ui_series);
	return;
      }
      
      /* get space for the array that'll take care of which frame of which data set we're looking at*/
      frames = g_try_new(guint,num_data_sets);
      if (frames == NULL) {
	g_warning(_("unable to allocate memory space for frames"));
	ui_series_unref(ui_series);
	return;
      }
      for (i=0;i<num_data_sets;i++) frames[i]=0; /* initialize */
      
      /* get space for the array that'll hold the series' frame durations, overestimate the size */
      ui_series->frame_durations = g_try_new(amide_time_t,total_data_set_frames+2);
      if (ui_series->frame_durations == NULL) {
	g_warning(_("unable to allocate memory for frame durations"));
	g_free(frames);
	ui_series_unref(ui_series);
	return;
      }
      
      /* do the initial case */
      temp_objects = ui_series->objects;
      i=0;
      valid = FALSE;
      while (temp_objects != NULL) {
	if (AMITK_IS_DATA_SET(temp_objects->data)) {
	  if (AMITK_DATA_SET_NUM_FRAMES(temp_objects->data) != frames[i]) {
	    temp_time = amitk_data_set_get_start_time(AMITK_DATA_SET(temp_objects->data), frames[i]);
	    if (!valid) /* first valid data set */ {
	      current_start = temp_time;
	      valid = TRUE;
	    } else if (temp_time < current_start) {
	      current_start = temp_time;
	    }
	  }
	  i++;
	}
	temp_objects = temp_objects->next;
      }
      
      ui_series->start_time = current_start;
      /* ignore any frames boundaries close to this one */
      temp_objects = ui_series->objects;
      i=0;
      while (temp_objects != NULL) {
	if (AMITK_IS_DATA_SET(temp_objects->data)) {
	  temp_time = amitk_data_set_get_start_time(AMITK_DATA_SET(temp_objects->data), frames[i]);
	  if (REAL_EQUAL(ui_series->start_time, temp_time))
	  frames[i]++;
	  i++;
	}
	temp_objects = temp_objects->next;
      }
    
      done = FALSE;
      last_start = ui_series->start_time;
      while (!done) {
	
	/* check if we're done */
	temp_objects = ui_series->objects;
	i=0;
	done = TRUE;
	while (temp_objects != NULL) {
	  if (AMITK_IS_DATA_SET(temp_objects->data)) {
	    if (frames[i] != AMITK_DATA_SET_NUM_FRAMES(temp_objects->data)) 
	      done = FALSE;
	    i++; 
	  }
	  temp_objects = temp_objects->next;
	}
	
	if (!done) {
	  /* check for the next earliest start time */
	  temp_objects = ui_series->objects;
	  i=0;
	  valid = FALSE;
	  while (temp_objects != NULL) {
	    if (AMITK_IS_DATA_SET(temp_objects->data)) {
	      if (AMITK_DATA_SET_NUM_FRAMES(temp_objects->data) != frames[i]) {
		temp_time = amitk_data_set_get_start_time(AMITK_DATA_SET(temp_objects->data), frames[i]);
		if (!valid) /* first valid data set */ {
		  current_start = temp_time;
		  valid = TRUE;
		} else if (temp_time < current_start) {
		  current_start = temp_time;
		}
	      }
	      i++;
	    }
	    temp_objects = temp_objects->next;
	  }
	  
	  /* allright, found the next start time */
	  ui_series->frame_durations[series_frame] = current_start-last_start;
	  series_frame++;
	  last_start = current_start;
	  
	  /* and ignore any frames boundaries close to this one */
	  temp_objects = ui_series->objects;
	  i=0;
	  while (temp_objects != NULL) {
	    if (AMITK_IS_DATA_SET(temp_objects->data)) {
	      if (AMITK_DATA_SET_NUM_FRAMES(temp_objects->data) != frames[i]) {
		temp_time = amitk_data_set_get_start_time(AMITK_DATA_SET(temp_objects->data), frames[i]);
	      if (REAL_EQUAL(current_start, temp_time))
		frames[i]++;
	      }
	    i++;
	    }
	    temp_objects = temp_objects->next;
	  }
	}
      }
      
      /* need to get the last frame */
      temp_objects = ui_series->objects;
      i=0;
      current_end = amitk_data_set_get_end_time(temp_objects->data, frames[i]-1);
      temp_objects = temp_objects->next;
      i++;
      while (temp_objects != NULL) {
	if (AMITK_IS_DATA_SET(temp_objects->data)) {
	  temp_time = amitk_data_set_get_end_time(AMITK_DATA_SET(temp_objects->data), frames[i]-1);
	  if (temp_time > current_end) {
	    current_end = temp_time;
	  }
	  i++;
	}
	temp_objects = temp_objects->next;
      }
      ui_series->frame_durations[series_frame] = current_end-last_start;
      series_frame++;
      
      /* save how many frames we'll need */
      ui_series->num_slices = series_frame;
      
      /* garbage collection */
      g_free(frames);
      
      /* figure out the view_frame */
      temp_time = ui_series->start_time;
      for (i=0; i<ui_series->num_slices;i++) {
	if ((temp_time <= ui_series->view_time) && 
	    (ui_series->view_time < temp_time+ui_series->frame_durations[i]))
	  ui_series->view_frame = i;
	temp_time += ui_series->frame_durations[i];
      }
    }
    break;
  case OVER_SPACE:
    {
      AmitkCorners view_corners;

      amitk_volumes_get_enclosing_corners(ui_series->objects, AMITK_SPACE(ui_series->volume), view_corners);
      ui_series->start_z = view_corners[0].z;
      ui_series->end_z = view_corners[1].z;
      ui_series->num_slices = ceil((ui_series->end_z-ui_series->start_z)/AMITK_VOLUME_Z_CORNER(ui_series->volume));
    }
    break;
  default:
    g_error("unexpected case in %s at line %d",__FILE__, __LINE__);
    break;
  }

  ui_series->max_slice_cache_size = ui_series->num_slices*num_data_sets+5;

  /* connect the thresholding and color table signals */
  temp_objects = ui_series->objects;
  while (temp_objects != NULL) {
    if (AMITK_IS_DATA_SET(temp_objects->data)) {
      g_signal_connect(G_OBJECT(temp_objects->data), "thresholding_changed",
		       G_CALLBACK(changed_cb), ui_series);
      g_signal_connect(G_OBJECT(temp_objects->data), "thresholds_changed",
		       G_CALLBACK(changed_cb), ui_series);
      g_signal_connect(G_OBJECT(temp_objects->data), "color_table_changed",
		       G_CALLBACK(color_table_changed_cb), ui_series);
      g_signal_connect(G_OBJECT(temp_objects->data), "invalidate_slice_cache",
		       G_CALLBACK(data_set_invalidate_slice_cache), ui_series);
      g_signal_connect(G_OBJECT(temp_objects->data), "interpolation_changed", 
		       G_CALLBACK(changed_cb), ui_series);
      g_signal_connect(G_OBJECT(temp_objects->data), "rendering_changed", 
		       G_CALLBACK(changed_cb), ui_series);
    } 
    g_signal_connect(G_OBJECT(temp_objects->data), "space_changed", 
		     G_CALLBACK(changed_cb), ui_series);
    temp_objects = temp_objects->next;
  }


  menus_toolbar_create(ui_series); /* setup the menu and toolbar */

  /* make the widgets for this dialog box */
  packing_table = gtk_table_new(1,2,FALSE);
  gtk_box_pack_start (GTK_BOX (ui_series->window_vbox), packing_table, TRUE,TRUE, 0);

  /* setup the canvas */
#ifdef AMIDE_LIBGNOMECANVAS_AA
  ui_series->canvas = gnome_canvas_new_aa();
#else
  ui_series->canvas = gnome_canvas_new();
#endif
  g_signal_connect(G_OBJECT(ui_series->canvas), "size_allocate", 
		   G_CALLBACK(canvas_size_change_cb), ui_series);

  gtk_table_attach(GTK_TABLE(packing_table), 
		   ui_series->canvas, 0,1,1,2,
		   X_PACKING_OPTIONS | GTK_FILL,
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);

  /* make a nice scroll bar */
  adjustment = ui_series_create_scroll_adjustment(ui_series);
  scale = gtk_hscale_new(adjustment);
  if ((ui_series->series_type == OVER_FRAMES) || (ui_series->series_type == OVER_GATES))
    gtk_scale_set_digits(GTK_SCALE(scale), 0); /* want integer for frames */
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_DISCONTINUOUS);
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(scale), 0,1,0,1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  g_signal_connect(G_OBJECT(adjustment), "value_changed", G_CALLBACK(scroll_change_cb), ui_series);

  /* show all our widgets */
  gtk_widget_show_all(GTK_WIDGET(ui_series->window));
  amide_register_window((gpointer) ui_series->window);

  return;
}



static void init_series_type_cb(GtkWidget * widget, gpointer data);
static void init_view_cb(GtkWidget * widget, gpointer data);



static void init_series_type_cb(GtkWidget * widget, gpointer data) {
  amide_gconf_set_int(GCONF_AMIDE_SERIES,"Type", 
		      GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "series_type")));
  return;
}

static void init_view_cb(GtkWidget * widget, gpointer data) {
  amide_gconf_set_int(GCONF_AMIDE_SERIES,"View",
		      GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "view")));
  return;
}

/* function to setup a dialog to allow us to choose options for the series */
GtkWidget * ui_series_init_dialog_create(AmitkStudy * study, GtkWindow * parent) {
  
  GtkWidget * dialog;
  gchar * temp_string;
  GtkWidget * table;
  guint table_row;
  GtkWidget * label;
  GtkWidget * radio_button[4];
  GtkWidget * hseparator;
  GtkWidget * scrolled;
  GtkWidget * tree_view;
  series_type_t i_series_type;
  AmitkView i_view;
  series_type_t series_type;
  AmitkView view;

  read_series_preferences(&series_type, &view);

  temp_string = g_strdup_printf(_("%s: Series Initialization Dialog"), PACKAGE);
  dialog = gtk_dialog_new_with_buttons (temp_string,  parent,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CLOSE, 
					GTK_STOCK_EXECUTE, AMITK_RESPONSE_EXECUTE,
					NULL);
  gtk_window_set_title(GTK_WINDOW(dialog), temp_string);
  g_free(temp_string);

  /* setup the callbacks for the dialog */
  g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(ui_common_init_dialog_response_cb), NULL);

  gtk_container_set_border_width(GTK_CONTAINER(dialog), 10);

  /* start making the widgets for this dialog box */
  table = gtk_table_new(7,3,FALSE);
  table_row=0;
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), table);

  /* what series type do we want */
  label = gtk_label_new(_("Series Type:"));
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 
		   table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);

  for (i_series_type = 0; i_series_type < NUM_SERIES_TYPES; i_series_type++) {
    if (i_series_type == 0) 
      radio_button[i_series_type] = 
	gtk_radio_button_new_with_label(NULL, _(series_names[i_series_type]));
    else
      radio_button[i_series_type] = 
	gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_button[0]), 
						    _(series_names[i_series_type]));
    gtk_widget_set_tooltip_text(radio_button[i_series_type], 
				_(series_explanations[i_series_type]));
    gtk_table_attach(GTK_TABLE(table), radio_button[i_series_type], 1, 2, 
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    g_object_set_data(G_OBJECT(radio_button[i_series_type]), "series_type", 
		      GINT_TO_POINTER(i_series_type));
    table_row++;
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button[series_type]), TRUE);

  for (i_series_type = 0; i_series_type < NUM_SERIES_TYPES; i_series_type++) {
    g_signal_connect(G_OBJECT(radio_button[i_series_type]), "clicked", G_CALLBACK(init_series_type_cb), NULL);
  }

  hseparator = gtk_hseparator_new();
  gtk_table_attach(GTK_TABLE(table), hseparator, 0,2,
		   table_row, table_row+1,GTK_FILL, 0, X_PADDING, Y_PADDING);
  table_row++;

  /* what view type do we want */
  label = gtk_label_new(_("View Type:"));
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 
		   table_row, table_row+1, X_PACKING_OPTIONS, 0, X_PADDING, Y_PADDING);

  for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view++) {
    if (i_view == 0) 
      radio_button[i_view] = 
	gtk_radio_button_new_with_label(NULL, amitk_view_get_name(i_view));
    else
      radio_button[i_view] = 
	gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_button[0]), 
						    amitk_view_get_name(i_view));
    gtk_table_attach(GTK_TABLE(table), radio_button[i_view], 1, 2, 
		     table_row, table_row+1, GTK_FILL, 0, X_PADDING, Y_PADDING);
    g_object_set_data(G_OBJECT(radio_button[i_view]), "view",
		      GINT_TO_POINTER(i_view));
    table_row++;
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button[view]), TRUE);


  for (i_view = 0; i_view < AMITK_VIEW_NUM; i_view++) {
    g_signal_connect(G_OBJECT(radio_button[i_view]), "clicked", G_CALLBACK(init_view_cb), NULL);
  }

  tree_view = amitk_tree_view_new(AMITK_TREE_VIEW_MODE_MULTIPLE_SELECTION,NULL, NULL);
  g_object_set_data(G_OBJECT(dialog), "tree_view", tree_view);
  amitk_tree_view_set_study(AMITK_TREE_VIEW(tree_view), study);
  amitk_tree_view_expand_object(AMITK_TREE_VIEW(tree_view), AMITK_OBJECT(study));

  /* make a scrolled area for the tree */
  scrolled = gtk_scrolled_window_new(NULL,NULL);  
  gtk_widget_set_size_request(scrolled,250,-1);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), 
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), 
					tree_view);
  gtk_table_attach(GTK_TABLE(table), scrolled, 2,3,
		   0, table_row,GTK_FILL, GTK_FILL | GTK_EXPAND, X_PADDING, Y_PADDING);

  /* and show all our widgets */
  gtk_widget_show_all(dialog);

  return dialog;
}



