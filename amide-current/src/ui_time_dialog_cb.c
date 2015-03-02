/* ui_time_dialog_cb.c
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
#include "study.h"
#include "ui_study.h"
#include "ui_time_dialog.h"
#include "ui_time_dialog_cb.h"


/* we've selected a row in the list of times */
void ui_time_dialog_cb_select_row(GtkCList * clist, gint row, gint column, 
				  GdkEventButton *event, gpointer data) {

  ui_study_t * ui_study = data;
  volume_t * volume;
  amide_time_t start, end, old_end;
  ui_time_dialog_t * new_time;
  guint frame;
  gchar * temp_string;
  gint error;
  
  /* reset out start and duration based on what just got selected */

  volume = gtk_clist_get_row_data(GTK_CLIST(clist), row); /* the volume we've selected */
  if (volume == NULL)
    return;

  /* the frame of the volume we've selected */
  if (gtk_clist_get_text(GTK_CLIST(clist), row, 2, &temp_string) == 0) {
    g_warning("%s: Couldn't figure out the frame number",PACKAGE);
    return;
  }
  /* convert string to an integer */
  error = sscanf(temp_string, "%d", &frame);
  /* note, don't g_free(temp_string), as it's just a pointer into the clist structure */
  if (error==EOF) return; /* make sure everything's kosher */

  start = volume_start_time(volume, frame);
  end = start+volume->frame_duration[frame];

  /* save our new times */
  new_time = gtk_object_get_data(GTK_OBJECT(ui_study->time_dialog), "new_time");

  /* simple left click selects indicates the choosen frame should be
     used as the time range, any other type of click
     (i.e. shift-click, right click, middle click) does an "inclusive"
     click, i.e. the time range will cover all previously selected
     frames and the currently selected frame */
  if ((event->state & GDK_SHIFT_MASK) || (event->button == 2) || (event->button == 3)) {
    old_end = new_time->time+new_time->duration;
    if (start < new_time->time)
      new_time->time = start+CLOSE;
    if (end > old_end)
      new_time->duration = end-new_time->time-CLOSE;
    else
      new_time->duration = old_end-new_time->time-CLOSE;
  } else {
    new_time->time = start+CLOSE;
    new_time->duration = end-new_time->time-CLOSE;
  }
  
  /* tell the property box that we've changed */
  gnome_property_box_changed(GNOME_PROPERTY_BOX(ui_study->time_dialog));

  /* update the clist */
  ui_time_dialog_set_times(ui_study);

  /* and set our view of the list to something nice */
  gtk_clist_moveto(GTK_CLIST(clist), row, 0, 0.5, 0);

  return;
}

/* we've unselected a row in the list of times */
void ui_time_dialog_cb_unselect_row(GtkCList * clist, gint row, gint column, GdkEventButton *event, gpointer data) {

  ui_study_t * ui_study = data;
  GList * selected_rows;
  gint selected_row;
  volume_t * volume;
  ui_time_dialog_t * new_time;
  amide_time_t start,end;
  gint error;
  gchar * temp_string;
  guint frame;

  new_time = gtk_object_get_data(GTK_OBJECT(ui_study->time_dialog), "new_time");

  /* iterate through the list, and update new_time based on the selected volumes */
  /* note, I'm doing some non-kosher stuff by gtk standards, but they don't have 
     a function that tells me if a given row is selected or not... */
  selected_rows = clist->selection;
  /* initialize new_time */
  if (selected_rows != NULL) {
    selected_row = GPOINTER_TO_INT(selected_rows->data);
    volume = gtk_clist_get_row_data(GTK_CLIST(clist), selected_row); 

    /* the frame of the volume we've selected */
    if (gtk_clist_get_text(GTK_CLIST(clist), selected_row, 2, &temp_string) == 0) {
      g_warning("%s: Couldn't figure out the frame number",PACKAGE);
      return;
    }
    /* convert string to an integer */
    error = sscanf(temp_string, "%d", &frame);
    /* note, don't g_free(temp_string), as it's just a pointer into the clist structure */
    if (error==EOF) return; /* make sure everything's kosher */

    start = volume_start_time(volume, frame);
    end = start+volume->frame_duration[frame];

    new_time->time = start+CLOSE;
    new_time->duration = end-start-2*CLOSE;
    
    selected_rows = selected_rows->next;
  }
  /* and iterate through what's left of the list */

  while (selected_rows != NULL) {
    selected_row = GPOINTER_TO_INT(selected_rows->data);
    volume = gtk_clist_get_row_data(GTK_CLIST(clist), selected_row); 

    /* the frame of the volume we've selected */
    if (gtk_clist_get_text(GTK_CLIST(clist), selected_row, 2, &temp_string) == 0) {
      g_warning("%s: Couldn't figure out the frame number",PACKAGE);
      return;
    }
    /* convert string to an integer */
    error = sscanf(temp_string, "%d", &frame);
    /* note, don't g_free(temp_string), as it's just a pointer into the clist structure */
    if (error==EOF) return; /* make sure everything's kosher */

    start = volume_start_time(volume, frame);
    end = start+volume->frame_duration[frame];

    if (start < new_time->time)
      new_time->time = start+CLOSE;
    if (end > (new_time->time+new_time->duration))
      new_time->duration = end-new_time->time - 2*CLOSE;

    selected_rows = selected_rows->next;
  }

  /* tell the property box that we've changed */
  gnome_property_box_changed(GNOME_PROPERTY_BOX(ui_study->time_dialog));

  /* update the clist */
  ui_time_dialog_set_times(ui_study);

  /* and set our view of the list to something nice */
  gtk_clist_moveto(GTK_CLIST(clist), row, 0, 0.5, 0);

  return;
}


/* function called when we hit the apply button */
void ui_time_dialog_cb_apply(GtkWidget* widget, gint page_number, gpointer data) {
  
  ui_study_t * ui_study = data;
  ui_time_dialog_t * new_time;
  
  /* we'll apply all page changes at once */
  if (page_number != -1)
    return;

  /* reset the time */
  new_time = gtk_object_get_data(GTK_OBJECT(ui_study->time_dialog), "new_time");
  study_set_view_time(ui_study->study, new_time->time);
  study_set_view_duration(ui_study->study, new_time->duration);

  /* through some new text onto the time popup button */
  ui_study_update_time_button(ui_study);

  /* redraw the volumes */
  ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_IMAGE);

  return;
}

/* callback for the help button */
void ui_time_dialog_cb_help(GnomePropertyBox *time_dialog, gint page_number, gpointer data) {

  GnomeHelpMenuEntry help_ref={PACKAGE,"basics.html#TIME-DIALOG-HELP"};

  gnome_help_display (0, &help_ref);

  return;
}

/* function called to destroy the time dialog */
gboolean ui_time_dialog_cb_close(GtkWidget* widget, gpointer data) {

  ui_study_t * ui_study = data;
  ui_time_dialog_t * new_time;

  /* trash collection */
  new_time = gtk_object_get_data(GTK_OBJECT(ui_study->time_dialog), "new_time");
  g_free(new_time);

  /* make sure the pointer in ui_study do this dialog is nulled */
  ui_study->time_dialog = NULL;

  return FALSE;
}

