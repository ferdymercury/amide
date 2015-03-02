/* ui_series.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2003 Andy Loening
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
#include <libgnomeui/libgnomeui.h>
#include "amitk_threshold.h"
#include "amitk_progress_dialog.h"
#include "amitk_canvas_object.h"
#include "image.h"
#include "ui_common.h"
#include "ui_series.h"
#include "ui_study_menus.h"
#include "pixmaps.h"

/* external variables */
static gchar * series_names[] = {
  N_("over Space"), 
  N_("over Time")
};

#define UPDATE_NONE 0
#define UPDATE_SERIES 0x1


/* ui_series data structures */
typedef struct ui_series_t {
  GnomeApp * app; 
  GList * slice_cache;
  gint max_slice_cache_size;
  GList * objects;
  AmitkDataSet * active_ds;
  GnomeCanvas * canvas;
  GnomeCanvasItem ** images;
  GnomeCanvasItem ** captions;
  GList ** items;
  GtkWidget * thresholds_dialog;
  guint num_slices, rows, columns;
  AmitkVolume * volume;
  amide_time_t view_time;
  AmitkFuseType fuse_type;
  amide_real_t pixel_dim;
  series_t type;
  GtkWidget * progress_dialog;
  gboolean in_generation;
  gboolean quit_generation;
  gint roi_width;
  GdkLineStyle line_style;
  gint pixbuf_width;
  gint pixbuf_height;

  /* for "PLANES" series */
  amide_time_t view_duration;
  amide_real_t start_z;
  amide_real_t z_point; /* current slice offset z component*/
  amide_real_t end_z;

  /* for "FRAMES" series */
  guint view_frame;
  amide_time_t start_time;
  amide_time_t * frame_durations; /* an array of frame durations */

  guint next_update;
  guint idle_handler_id;

  guint reference_count;
} ui_series_t;



static void scroll_change_cb(GtkAdjustment* adjustment, gpointer data);
//static void export_cb(GtkWidget * widget, gpointer data);
static void changed_cb(gpointer dummy, gpointer ui_series);
static void data_set_invalidate_slice_cache(AmitkDataSet *ds, gpointer ui_series);
static void threshold_cb(GtkWidget * widget, gpointer data);
static void close_cb(GtkWidget* widget, gpointer data);
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data);


static void menus_create(ui_series_t * ui_series);
static void toolbar_create(ui_series_t * ui_series);

static ui_series_t * ui_series_unref(ui_series_t * ui_series);
static ui_series_t * ui_series_init(GnomeApp * app);
static GtkAdjustment * ui_series_create_scroll_adjustment(ui_series_t * ui_series);
static void add_update(ui_series_t * ui_series);
static gboolean update_immediate(gpointer ui_series);


/* function called by the adjustment in charge for scrolling */
static void scroll_change_cb(GtkAdjustment* adjustment, gpointer data) {

  ui_series_t * ui_series = data;

  if (ui_series->type == PLANES)
    ui_series->z_point = adjustment->value-AMITK_VOLUME_Z_CORNER(ui_series->volume)/2.0;
  else
    ui_series->view_frame = adjustment->value;

  add_update(ui_series);

  return;
}

/* function to save the series as an external data format */
//static void export_cb(GtkWidget * widget, gpointer data) {
  
  //  ui_series_t * ui_series = data;

  /* this function would require being able to transform a canvas back into
     a single image/pixbuf/etc.... don't know how to do this yet */

//  return;
//}


/* function called when a data set changed */
static void changed_cb(gpointer dummy, gpointer data) {
  ui_series_t * ui_series=data;
  add_update(ui_series);
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
static void threshold_cb(GtkWidget * widget, gpointer data) {

  ui_series_t * ui_series = data;

  if (ui_series->thresholds_dialog != NULL)
    return;

  if (!amitk_objects_has_type(ui_series->objects, AMITK_OBJECT_TYPE_DATA_SET, FALSE)) {
    g_warning(_("No data sets to threshold\n"));
    return;
  }

  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_series->canvas));

  ui_series->thresholds_dialog = 
    amitk_thresholds_dialog_new(ui_series->objects, GTK_WINDOW(ui_series->app));
  g_signal_connect(G_OBJECT(ui_series->thresholds_dialog), "delete_event",
		   G_CALLBACK(thresholds_delete_event), ui_series);
  gtk_widget_show(ui_series->thresholds_dialog);

  ui_common_remove_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_series->canvas));

  return;
}



/* function ran when closing a series window */
static void close_cb(GtkWidget* widget, gpointer data) {

  ui_series_t * ui_series = data;
  GtkWidget * app = GTK_WIDGET(ui_series->app);
  gboolean return_val;

  /* run the delete event function */
  g_signal_emit_by_name(G_OBJECT(app), "delete_event", NULL, &return_val);
  if (!return_val) gtk_widget_destroy(app);

  return;
}

/* function to run for a delete_event */
static gboolean delete_event_cb(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_series_t * ui_series = data;
  GtkWidget * app = GTK_WIDGET(ui_series->app);

  /* trying to close while we're generating */
  if (ui_series->in_generation) {
    ui_series->quit_generation=TRUE;
    return TRUE;
  }

  /* free the associated data structure */
  ui_series = ui_series_unref(ui_series);

  /* tell the rest of the program this window is no longer here */
  amide_unregister_window((gpointer) app);

  return FALSE;
}






/* function to setup the menus for the series ui */
static void menus_create(ui_series_t * ui_series) {

  /* defining the menus for the series ui interface */
  GnomeUIInfo file_menu[] = {
    //    GNOMEUIINFO_ITEM_DATA(N_("_Export Series"),
    //			  N_("Export the series view to an image file (JPEG/TIFF/PNG/etc.)"),
    //			  ui_series_cb_export,
    //			  ui_series, NULL),
    //    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_MENU_CLOSE_ITEM(close_cb, ui_series),
    GNOMEUIINFO_END
  };

  /* and the main menu definition */
  GnomeUIInfo series_main_menu[] = {
    GNOMEUIINFO_MENU_FILE_TREE(file_menu),
    GNOMEUIINFO_MENU_HELP_TREE(ui_common_help_menu),
    GNOMEUIINFO_END
  };


  /* sanity check */
  g_assert(ui_series!=NULL);


  /* make the menu */
  gnome_app_create_menus(GNOME_APP(ui_series->app), series_main_menu);

  return;

}



      

/* function to setup the toolbar for the seriesui */
void toolbar_create(ui_series_t * ui_series) {

  GtkWidget * toolbar;

  /* the toolbar definitions */
  GnomeUIInfo series_main_toolbar[] = {
    GNOMEUIINFO_ITEM_DATA(NULL,
			  N_("Set the thresholds and colormaps for the data sets in the series view"),
			  threshold_cb,
			  ui_series, icon_threshold_xpm),
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_END
  };

  /* sanity check */
  g_assert(ui_series!=NULL);
  
  /* make the toolbar */
  toolbar = gtk_toolbar_new();
  gnome_app_fill_toolbar(GTK_TOOLBAR(toolbar), series_main_toolbar, NULL);

  /* add our toolbar to our app */
  gnome_app_set_toolbar(GNOME_APP(ui_series->app), GTK_TOOLBAR(toolbar));

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
      gtk_idle_remove(ui_series->idle_handler_id);
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
      for (i=0; i < ui_series->rows*ui_series->columns; i++) {
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
static ui_series_t * ui_series_init(GnomeApp * app) {

  ui_series_t * ui_series;

  /* alloc space for the data structure for passing ui info */
  if ((ui_series = g_try_new(ui_series_t,1)) == NULL) {
    g_warning(_("couldn't allocate space for ui_series_t"));
    return NULL;
  }
  ui_series->reference_count = 1;

  /* set any needed parameters */
  ui_series->app = app;
  ui_series->slice_cache = NULL;
  ui_series->max_slice_cache_size=10;
  ui_series->num_slices = 0;
  ui_series->rows = 0;
  ui_series->columns = 0;
  ui_series->images = NULL;
  ui_series->captions = NULL;
  ui_series->items = NULL;
  ui_series->objects = NULL;
  ui_series->active_ds = NULL;
  ui_series->fuse_type = AMITK_FUSE_TYPE_BLEND;
  ui_series->pixel_dim = 1.0;
  ui_series->volume = NULL;
  ui_series->view_time = 0.0;
  ui_series->type = PLANES;
  ui_series->thresholds_dialog = NULL;
  ui_series->progress_dialog = amitk_progress_dialog_new(GTK_WINDOW(ui_series->app));
  ui_series->in_generation=FALSE;
  ui_series->quit_generation=FALSE;
  ui_series->roi_width = 1.0;
  ui_series->line_style = 0;
  ui_series->pixbuf_width = 0;
  ui_series->pixbuf_height = 0;

  ui_series->view_duration = 1.0;
  ui_series->start_z = 0.0;
  ui_series->end_z = 0.0;
  ui_series->z_point = 0.0;

  ui_series->view_frame = 0;
  ui_series->start_time = 0.0;
  ui_series->frame_durations = NULL;

  ui_series->next_update = UPDATE_NONE;
  ui_series->idle_handler_id = 0;

  return ui_series;
}


/* function to make the adjustments for the scrolling scale */
static GtkAdjustment * ui_series_create_scroll_adjustment(ui_series_t * ui_series) { 

  amide_real_t thickness;

  if (ui_series->type == PLANES) {
    thickness = AMITK_VOLUME_Z_CORNER(ui_series->volume);
    return GTK_ADJUSTMENT(gtk_adjustment_new(ui_series->z_point-thickness/2.0,
					     ui_series->start_z+thickness/2.0,
					     ui_series->end_z-thickness/2.0,
					     thickness, thickness, thickness));
  } else { /* FRAMES */
    return GTK_ADJUSTMENT(gtk_adjustment_new(ui_series->view_frame, 0, ui_series->num_slices, 1,1,1));
  }
}


static void add_update(ui_series_t * ui_series) {

  ui_series->next_update = ui_series->next_update | UPDATE_SERIES;
  if (ui_series->idle_handler_id == 0)
    ui_series->idle_handler_id = 
      gtk_idle_add_priority(G_PRIORITY_HIGH_IDLE,update_immediate, ui_series);

  return;
}


/* funtion to update the canvas */
static gboolean update_immediate(gpointer data) {

  ui_series_t * ui_series = data;
  AmitkPoint temp_point;
  amide_time_t temp_time, temp_duration;
  gint i, start_i;
  gint width, height;
  gdouble x, y;
  GtkRequisition requisition;
  AmitkVolume * view_volume;
  gint image_width, image_height;
  gchar * temp_string;
  GdkPixbuf * pixbuf;
  gboolean can_continue=TRUE;
  gboolean return_val = TRUE;
  GList * objects;
  GnomeCanvasItem * item;
  rgba_t outline_color;

  ui_series->in_generation=TRUE;
  ui_common_place_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_series->canvas));

  temp_string = g_strdup_printf(_("Slicing for series"));
  amitk_progress_dialog_set_text(AMITK_PROGRESS_DIALOG(ui_series->progress_dialog), temp_string);
  g_free(temp_string);

  /* get the view axis to use*/
  width = 0.9*gdk_screen_width();
  height = 0.8*gdk_screen_height();

  image_width = ui_series->pixbuf_width + UI_SERIES_R_MARGIN + UI_SERIES_L_MARGIN;
  image_height = ui_series->pixbuf_height + UI_SERIES_TOP_MARGIN + UI_SERIES_BOTTOM_MARGIN;


  /* allocate space for pointers to our canvas_images, captions, and item lists, if needed */
  if (ui_series->images == NULL) {
    /* figure out how many images we can display at once */
    ui_series->columns = floor(width/image_width);
    if (ui_series->columns < 1) ui_series->columns = 1;
    ui_series->rows = floor(height/image_height);
    if (ui_series->rows < 1) ui_series->rows = 1;

    /* compensate for cases where we don't need all the rows */
    if ((ui_series->columns * ui_series->rows) > ui_series->num_slices)
      ui_series->rows = ceil((double) ui_series->num_slices/(double) ui_series->columns);

    if ((ui_series->images = g_try_new(GnomeCanvasItem *,ui_series->rows*ui_series->columns)) == NULL) {
      g_warning(_("couldn't allocate space for pointers to image GnomeCanvasItem's"));
      return_val = FALSE;
      goto exit_update;
    }
    if ((ui_series->captions = g_try_new(GnomeCanvasItem *,ui_series->rows*ui_series->columns)) == NULL) {
      g_warning(_("couldn't allocate space for pointers to caption GnomeCanvasItem's"));
      return_val = FALSE;
      goto exit_update;
    }
    if ((ui_series->items = g_try_new(GList *,ui_series->rows*ui_series->columns)) == NULL) {
      g_warning(_("couldn't allocate space for pointers to GnomeCanavasItem lists"));
      return_val = FALSE;
      goto exit_update;
    }
    for (i=0; i < ui_series->columns * ui_series->rows ; i++) {
      ui_series->images[i] = NULL;
      ui_series->captions[i] = NULL;
      ui_series->items[i] = NULL;
    }
  }


  /* figure out what's the first image we want to display */
  if (ui_series->type == PLANES) 
    start_i = ui_series->num_slices*((ui_series->z_point-ui_series->start_z)/
				     (ui_series->end_z-ui_series->start_z-AMITK_VOLUME_Z_CORNER(ui_series->volume)));
  else  /* FRAMES */
    start_i = ui_series->view_frame;

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
  if (ui_series->type == PLANES) {
    temp_time = ui_series->view_time;
    temp_duration = ui_series->view_duration;
  } else { /* frames */
    for (i=0;i< start_i; i++)
      temp_time += ui_series->frame_durations[i];
    temp_time -= ui_series->frame_durations[start_i];/* well get added back 1st time through loop */
    temp_duration = ui_series->frame_durations[start_i]; 
  }
  x = y = 0.0;

  view_volume = AMITK_VOLUME(amitk_object_copy(AMITK_OBJECT(ui_series->volume)));

  for (i=start_i; 
       (((i-start_i) < (ui_series->rows*ui_series->columns)) && 
	(i < ui_series->num_slices) &&
	can_continue &&
	(!ui_series->quit_generation));
	i++) {

    if (ui_series->type == PLANES) {
      temp_point.z = i*AMITK_VOLUME_Z_CORNER(ui_series->volume)+ui_series->start_z;
    } else { /* frames */
      temp_time += temp_duration; /* duration from last time through the loop */
      temp_duration = ui_series->frame_durations[i];
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
				    ui_series->pixel_dim,
				    view_volume,
				    ui_series->fuse_type);
    
      if (ui_series->images[i-start_i] == NULL) 
	ui_series->images[i-start_i] = gnome_canvas_item_new(gnome_canvas_root(ui_series->canvas),
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
	    amitk_color_table_outline_color(AMITK_DATA_SET_COLOR_TABLE(AMITK_OBJECT_PARENT(objects->data)), TRUE);
	else
	  outline_color = amitk_color_table_outline_color(AMITK_COLOR_TABLE_BW_LINEAR, TRUE);

	item = amitk_canvas_object_draw(ui_series->canvas, view_volume, objects->data, NULL,
					ui_series->pixel_dim,
					ui_series->pixbuf_width, 
					ui_series->pixbuf_height,
					x+UI_SERIES_L_MARGIN, y+UI_SERIES_TOP_MARGIN,
					outline_color, ui_series->roi_width,
					ui_series->line_style);
	if (item != NULL)
	  ui_series->items[i-start_i] = g_list_append(ui_series->items[i-start_i], item);
      }
      objects = objects->next;
    }


    /* write the caption */
    if (ui_series->type == PLANES)
      temp_string = g_strdup_printf("%2.1f-%2.1f mm", temp_point.z, temp_point.z+AMITK_VOLUME_Z_CORNER(ui_series->volume));
    else /* frames */
      temp_string = g_strdup_printf("%2.1f-%2.1f s", temp_time, temp_time+temp_duration);
    if (ui_series->captions[i-start_i] == NULL) 
      ui_series->captions[i-start_i] =
	gnome_canvas_item_new(gnome_canvas_root(ui_series->canvas),
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

  /* readjust widith and height for what we really used */
  width = ui_series->columns*image_width;
  height =ui_series->rows   *image_height;

  /* reset the min size of the widget */
  gnome_canvas_set_scroll_region(ui_series->canvas, 0.0, 0.0, 
				 (double) width, (double) height);

  requisition.width = width;
  requisition.height = height;
  gtk_widget_size_request(GTK_WIDGET(ui_series->canvas), &requisition);
  gtk_widget_set_size_request(GTK_WIDGET(ui_series->canvas), width, height);
  /* the requisition thing should work.... I'll use this for now for now... */


 exit_update:

  amitk_progress_dialog_set_fraction(AMITK_PROGRESS_DIALOG(ui_series->progress_dialog), 2.0); /* hide progress dialog */
  ui_common_remove_cursor(UI_CURSOR_WAIT, GTK_WIDGET(ui_series->canvas));

  ui_series->next_update = UPDATE_NONE;
  if (ui_series->idle_handler_id != 0) {
    gtk_idle_remove(ui_series->idle_handler_id);
    ui_series->idle_handler_id=0;
  }

  ui_series->quit_generation=FALSE;
  ui_series->in_generation=FALSE;

  return return_val;
}

/* function that sets up the series dialog */
void ui_series_create(AmitkStudy * study, AmitkObject * active_object,
		      AmitkView view, AmitkVolume * canvas_view, 
		      gint roi_width, GdkLineStyle line_style, series_t series_type) {
 
  ui_series_t * ui_series;
  GnomeApp * app;
  gchar * title = NULL;
  GtkWidget * packing_table;
  GtkAdjustment * adjustment;
  GtkWidget * scale;
  amide_time_t min_duration;
  GList * temp_objects;
  guint num_data_sets = 0;
  guint total_data_set_frames=0;

  /* sanity checks */
  g_return_if_fail(AMITK_IS_STUDY(study));

  title = g_strdup_printf(_("Series: %s (%s - %s)"), AMITK_OBJECT_NAME(study),
			  view_names[view], series_names[series_type]);
  app = GNOME_APP(gnome_app_new(PACKAGE, title));
  g_free(title);
  gtk_window_set_resizable(GTK_WINDOW(app), TRUE);

  ui_series = ui_series_init(app);
  ui_series->type = series_type;
  ui_series->line_style = line_style;
  ui_series->roi_width = roi_width;

  ui_series->objects = amitk_object_get_selected_children(AMITK_OBJECT(study), AMITK_VIEW_MODE_SINGLE, TRUE);
  if (ui_series->objects == NULL) {
    g_warning(_("Need selected objects to create a series"));
    ui_series_unref(ui_series);
    return;
  }

  if (active_object != NULL)
    if (AMITK_IS_DATA_SET(active_object))
      ui_series->active_ds = amitk_object_ref(active_object); /* save a pointer to which object is active */

  /* setup the callbacks for app */
  g_signal_connect(G_OBJECT(app), "realize", G_CALLBACK(ui_common_window_realize_cb), NULL);
  g_signal_connect(G_OBJECT(app), "delete_event", G_CALLBACK(delete_event_cb), ui_series);

  /* save the coordinate space of the series and some other parameters */
  ui_series->volume = AMITK_VOLUME(amitk_object_copy(AMITK_OBJECT(canvas_view)));

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
  if (ui_series->type == PLANES) {
    AmitkCorners view_corners;

    amitk_volumes_get_enclosing_corners(ui_series->objects, AMITK_SPACE(ui_series->volume), view_corners);
    ui_series->start_z = view_corners[0].z;
    ui_series->end_z = view_corners[1].z;
    ui_series->num_slices = ceil((ui_series->end_z-ui_series->start_z)/AMITK_VOLUME_Z_CORNER(ui_series->volume));

  } else { /* FRAMES */
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
      g_warning(_("unable to allocate memory for frames"));
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

  
  ui_series->max_slice_cache_size = ui_series->num_slices*num_data_sets+5;

  /* connect the thresholding and color table signals */
  temp_objects = ui_series->objects;
  while (temp_objects != NULL) {
    if (AMITK_IS_DATA_SET(temp_objects->data)) {
      g_signal_connect(G_OBJECT(temp_objects->data), "thresholding_changed",
		       G_CALLBACK(changed_cb), ui_series);
      g_signal_connect(G_OBJECT(temp_objects->data), "color_table_changed",
		       G_CALLBACK(changed_cb), ui_series);
      g_signal_connect(G_OBJECT(temp_objects->data), "invalidate_slice_cache",
		       G_CALLBACK(data_set_invalidate_slice_cache), ui_series);
      g_signal_connect(G_OBJECT(temp_objects->data), "interpolation_changed", 
		       G_CALLBACK(changed_cb), ui_series);
    } 
    g_signal_connect(G_OBJECT(temp_objects->data), "space_changed", 
		     G_CALLBACK(changed_cb), ui_series);
    temp_objects = temp_objects->next;
  }


  menus_create(ui_series); /* setup the menu */
  toolbar_create(ui_series); /* setup the toolbar */

  /* make the widgets for this dialog box */
  packing_table = gtk_table_new(1,2,FALSE);
  gnome_app_set_contents(app, GTK_WIDGET(packing_table));

  /* setup the canvas */
#ifdef AMIDE_LIBGNOMECANVAS_OLD
  ui_series->canvas = GNOME_CANVAS(gnome_canvas_new());
#else
  ui_series->canvas = GNOME_CANVAS(gnome_canvas_new_aa());
#endif
  update_immediate(ui_series); /* fill in the canvas */
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(ui_series->canvas), 0,1,1,2,
		   X_PACKING_OPTIONS | GTK_FILL,
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);

  /* make a nice scroll bar */
  adjustment = ui_series_create_scroll_adjustment(ui_series);
  scale = gtk_hscale_new(adjustment);
  if (ui_series->type == FRAMES) 
    gtk_scale_set_digits(GTK_SCALE(scale), 0); /* want integer for frames */
  gtk_range_set_update_policy(GTK_RANGE(scale), GTK_UPDATE_DISCONTINUOUS);
  gtk_table_attach(GTK_TABLE(packing_table), 
		   GTK_WIDGET(scale), 0,1,0,1,
		   X_PACKING_OPTIONS | GTK_FILL, 0, X_PADDING, Y_PADDING);
  g_signal_connect(G_OBJECT(adjustment), "value_changed", G_CALLBACK(scroll_change_cb), ui_series);

  /* show all our widgets */
  gtk_widget_show_all(GTK_WIDGET(app));
  amide_register_window((gpointer) app);

  return;
}







