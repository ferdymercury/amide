/* ui_time_dialog.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001 Andy Loening
 *
 * Author: Andy Loening <loening@ucla.edu>
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

#include "config.h"
#include <gnome.h>
#include <math.h>
#include "amide.h"
#include "study.h"
#include "image.h"
#include "ui_threshold.h"
#include "ui_series.h"
#include "ui_roi.h"
#include "ui_volume.h"
#include "ui_study.h"
#include "ui_time_dialog.h"
#include "ui_time_dialog_callbacks.h"


static gchar * column_names[] =  {"Start (s)", \
				  "End (s)", \
				  "Frame #", \
				  "Volume"};

/* function to setup the time combo widget */
void ui_time_dialog_set_times(ui_study_t * ui_study) {
  
  ui_volume_list_t * ui_volume_list;
  volume_t * min_volume;
  GtkWidget * clist;
  guint num_volumes;
  guint i_volume, min_volume_num;
  guint * frames;
  gboolean * volume_used;
  guint total_frames=0;
  guint current_frames;
  volume_time_t min, temp;
  gchar * temp_strings[4];
  ui_time_dialog_t * new_time;

  if (ui_study->time_dialog == NULL)
    return;

  /* get the pointer to the list widget */
  clist = gtk_object_get_data(GTK_OBJECT(ui_study->time_dialog), "clist");
  gtk_clist_clear(GTK_CLIST(clist)); /* make sure the list is clear */

  /* get the pointer to our new time structure */
  new_time = gtk_object_get_data(GTK_OBJECT(ui_study->time_dialog), "new_time");

  /* count the number of volumes */
  ui_volume_list = ui_study->current_volumes;
  num_volumes=0;
  while (ui_volume_list != NULL) {
    num_volumes++;
    total_frames += ui_volume_list->volume->num_frames;
    ui_volume_list = ui_volume_list->next;
  }

  /* get space for the array that'll take care of which frame of which volume we're looking at*/
  frames = (guint *) g_malloc(num_volumes+sizeof(guint));
  if ((frames == NULL) && (num_volumes !=0)) {
    g_warning("%s: can't count frames or allocate memory!",PACKAGE);
    return;
  }
  volume_used = (gboolean *) g_malloc(num_volumes+sizeof(gboolean));
  if (volume_used == NULL) {
    g_warning("%s: coudn't find memory for a dang boolean array?",PACKAGE);
    g_free(frames);
    return;
  }

  /* Block signals to this list widget.  I need to do this, as the
     only way I could figure out how to get rows highlighted is
     throught the gtk_clist_select_row function, which emits a
     "select_row" signal.  Unblocking is at the end of this function */
  gtk_signal_handler_block_by_func(GTK_OBJECT(clist),
				   GTK_SIGNAL_FUNC(ui_time_dialog_callbacks_select_row),
				   ui_study);


  /* initialize */
  for (i_volume = 0; i_volume<num_volumes;i_volume++) {
    frames[i_volume]=0;
    volume_used[i_volume]=FALSE;
  }

  /* start generating our list of options */
  current_frames=0;
  while (current_frames < total_frames) {
    ui_volume_list = ui_study->current_volumes;

    /* initialize the variables with the first volume on the volumes list */
    i_volume=0;
    while (ui_volume_list->volume->num_frames <= frames[i_volume]) {
      ui_volume_list = ui_volume_list->next; /* advancing to a volume that still has frames left */
      i_volume++;
    }
    min_volume = ui_volume_list->volume;
    min_volume_num = i_volume;
    min = volume_start_time(min_volume,frames[i_volume]);

    ui_volume_list = ui_volume_list->next;
    i_volume++;
    while (ui_volume_list != NULL) {
      if (frames[i_volume] < ui_volume_list->volume->num_frames) {
	temp = volume_start_time(ui_volume_list->volume, frames[i_volume]);
	if (temp < min) {
	  min_volume = ui_volume_list->volume;
	  min = temp;
	  min_volume_num = i_volume;
	}	
      }
      i_volume++;
      ui_volume_list = ui_volume_list->next;
    }
    
    /* we now have the next minimum start time */
    
    /* setup the corresponding list entry */
    temp_strings[0] = g_strdup_printf("%5.1fs", min);
    temp_strings[1] = g_strdup_printf("%5.1fs", min+min_volume->frame_duration[frames[min_volume_num]]);
    temp_strings[2] =  g_strdup_printf("%d", frames[min_volume_num]);
    temp_strings[3] = g_strdup_printf("%s", min_volume->name);
    gtk_clist_append(GTK_CLIST(clist),temp_strings);
    gtk_clist_set_row_data(GTK_CLIST(clist), current_frames, min_volume);
    g_free(temp_strings[0]);
    g_free(temp_strings[1]);
    g_free(temp_strings[2]);
    g_free(temp_strings[3]);

    /* figure out if this row is suppose to be selected */
    if (((new_time->time <= min) 
	 && 
	 ((new_time->time+new_time->duration) > min))
	||
	((new_time->time <= (min+min_volume->frame_duration[frames[min_volume_num]])) 
	 &&
	 ((new_time->time+new_time->duration) > (min+min_volume->frame_duration[frames[min_volume_num]])))
	||
	((new_time->time > (min)) 
	 &&
	 ((new_time->time+new_time->duration) < (min+min_volume->frame_duration[frames[min_volume_num]])))
	) {
      gtk_clist_select_row(GTK_CLIST(clist), current_frames,0);
      volume_used[min_volume_num]=TRUE;
    }

    /* special case #1
       this is the first frame in the volume and it's still behind the time, so select it anyway */
    if ((!volume_used[min_volume_num]) && 
	(frames[min_volume_num] == 0) && 
	(new_time->time < min)) {
      gtk_clist_select_row(GTK_CLIST(clist), current_frames,0);
      volume_used[min_volume_num]=TRUE;
    }

    /* special case #2
       this is the last frame in the volume and no other frame has been selected, so select this one anyway */
    if ((!volume_used[min_volume_num]) && 
	(frames[min_volume_num] == min_volume->num_frames-1) && 
	(new_time->time > min + min_volume->frame_duration[frames[min_volume_num]])) {
      gtk_clist_select_row(GTK_CLIST(clist), current_frames,0);
      volume_used[min_volume_num]=TRUE;
    }



    frames[min_volume_num]++;
    current_frames++;
  }

  /* freeup our allocated data structures */
  g_free(frames);
  g_free(volume_used);

  /* done updating the clist, we can reconnect signals now */
  gtk_signal_handler_unblock_by_func(GTK_OBJECT(clist),
				     GTK_SIGNAL_FUNC(ui_time_dialog_callbacks_select_row),
				     ui_study);
  
  return;
}




/* create the time slection dialog */
void ui_time_dialog_create(ui_study_t * ui_study) {

  GtkWidget * time_dialog;
  gchar * temp_string = NULL;
  GtkWidget * packing_table;
  GtkWidget * clist;
  GtkWidget * label;
  GtkWidget * scrolled;
  guint table_row = 0;
  ui_time_dialog_t * new_time;

  /* sanity checks */
  if (ui_study->time_dialog != NULL)
    return;

  temp_string = g_strdup_printf("%s: Time Dialog",PACKAGE);
  time_dialog = gnome_property_box_new();
  gtk_window_set_title(GTK_WINDOW(time_dialog), temp_string);
  g_free(temp_string);
  ui_study->time_dialog = time_dialog; /* save a pointer to the dialog */
  gtk_window_set_policy(GTK_WINDOW(time_dialog), FALSE, TRUE, FALSE);

  /* make (and save a pointer to) a structure to temporary hold the new time and duration */
  new_time = (ui_time_dialog_t *) g_malloc(sizeof(ui_time_dialog_t));
  new_time->time = study_view_time(ui_study->study);
  new_time->duration = study_view_duration(ui_study->study);
  gtk_object_set_data(GTK_OBJECT(time_dialog), "new_time", new_time);
  
					   

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(time_dialog), "close",
		     GTK_SIGNAL_FUNC(ui_time_dialog_callbacks_close_event),
		     ui_study);
  gtk_signal_connect(GTK_OBJECT(time_dialog), "apply",
		     GTK_SIGNAL_FUNC(ui_time_dialog_callbacks_apply),
		     ui_study);
  gtk_signal_connect(GTK_OBJECT(time_dialog), "help",
		     GTK_SIGNAL_FUNC(ui_time_dialog_callbacks_help),
		     ui_study);

  /* start making the widgets for this dialog box */
  packing_table = gtk_table_new(1,1,FALSE);
  label = gtk_label_new("Time Interval");
  table_row=0;
  gnome_property_box_append_page (GNOME_PROPERTY_BOX(time_dialog), GTK_WIDGET(packing_table), label);

  /* the scroll widget which the list will go into */
  scrolled = gtk_scrolled_window_new(NULL,NULL);
  gtk_widget_set_usize(scrolled,150,150);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				 GTK_POLICY_AUTOMATIC,
				 GTK_POLICY_AUTOMATIC);
  gtk_table_attach(GTK_TABLE(packing_table), GTK_WIDGET(scrolled), 0,1, 
		   table_row, table_row+1, 
		   X_PACKING_OPTIONS | GTK_FILL, 
		   Y_PACKING_OPTIONS | GTK_FILL, 
		   X_PADDING, Y_PADDING);
  table_row++;

  /* and the list itself */
  clist = gtk_clist_new_with_titles(4, column_names);
  gtk_clist_set_selection_mode(GTK_CLIST(clist), GTK_SELECTION_MULTIPLE);
  gtk_object_set_data(GTK_OBJECT(clist), "time_dialog", time_dialog);
  gtk_object_set_data(GTK_OBJECT(time_dialog), "clist", clist);
  gtk_signal_connect(GTK_OBJECT(clist), "select_row",
		     GTK_SIGNAL_FUNC(ui_time_dialog_callbacks_select_row),
		     ui_study);
  gtk_signal_connect(GTK_OBJECT(clist), "unselect_row",
		     GTK_SIGNAL_FUNC(ui_time_dialog_callbacks_unselect_row),
		     ui_study);
  gtk_signal_connect(GTK_OBJECT(clist), "button_press_event",
		     GTK_SIGNAL_FUNC(ui_time_dialog_callbacks_select_row),
		     ui_study);
  gtk_container_add(GTK_CONTAINER(scrolled),clist);
  ui_time_dialog_set_times(ui_study);





  /* and show all our widgets */
  gtk_widget_show_all(time_dialog);

  return;
}

