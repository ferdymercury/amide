/* ui_alignment_dialog.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2002 Andy Loening
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
#include "ui_alignment_dialog_cb.h"
#include "ui_alignment_dialog.h"
#include "ui_common.h"
#include "../pixmaps/amide_logo.xpm"


#ifdef AMIDE_LIBGSL_SUPPORT
static gchar * start_page_text = 
"Welcome to the data set alignment wizard, used for\n"
"aligning one medical image data set with another.\n"
"\n"
"Currently, only semiautomated registration has been\n"
"implemented inside of AMIDE.\n";


static gchar * data_set_error_page_text = 
"There is only one data set in this study.  There needs\n"
"to be at least two data sets to perform an alignment\n";

#else /* NO LIBGSL SUPPORT */
static gchar * start_page_text = 
"Welcome to the data set alignment wizard, used for\n"
"aligning one medical image data set with another.\n"
"\n"
"Currently, only semiautomated registration has been,\n"
"implemented inside of AMIDE.  This feature requires,\n"
"support from the GNU Scientific Library (libgsl).  This\n"
"copy of AMIDE has not been compiled with support for\n"
"libgsl, so it cannot perform semiautomated registration.\n";
#endif

static gchar * align_pts_error_page_text =
"There must exist at least 4 pairs of alignment\n"
"points between the two data sets to perform an\n"
"alignment.\n";

static gchar * end_page_text =
"The alignment has been calculated, press Finish to\n"
"apply, or Cancel to quit\n";

/* destroy a ui_alignment data structure */
ui_alignment_t * ui_alignment_free(ui_alignment_t * ui_alignment) {

  if (ui_alignment == NULL) return ui_alignment;

  /* sanity checks */
  g_return_val_if_fail(ui_alignment->reference_count > 0, NULL);

  /* remove a reference count */
  ui_alignment->reference_count--;

  /* things to do if we've removed all reference's */
  if (ui_alignment->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing ui_alignment\n");
#endif

    ui_alignment->volumes = volumes_unref(ui_alignment->volumes);
    ui_alignment->volume_moving = volume_unref(ui_alignment->volume_moving);
    ui_alignment->volume_fixed = volume_unref(ui_alignment->volume_fixed);
    ui_alignment->align_pts = align_pts_unref(ui_alignment->align_pts);
    ui_alignment->coord_frame = rs_unref(ui_alignment->coord_frame);

    gdk_imlib_kill_image(ui_alignment->logo);

    g_free(ui_alignment);
    ui_alignment = NULL;
  }

  return ui_alignment;

}

/* malloc and initialize a ui_alignment data structure */
static ui_alignment_t * ui_alignment_init(void) {

  ui_alignment_t * ui_alignment;

  /* alloc space for the data structure for passing ui info */
  if ((ui_alignment = (ui_alignment_t *) g_malloc(sizeof(ui_alignment_t))) == NULL) {
    g_warning("couldn't allocate space for ui_alignment_t");
    return NULL;
  }

  ui_alignment->reference_count = 1;
  ui_alignment->dialog = NULL;
  ui_alignment->druid = NULL;
  ui_alignment->volume_moving = NULL;
  ui_alignment->volume_fixed = NULL;
  ui_alignment->align_pts = NULL;
  ui_alignment->coord_frame = NULL;

  /* set any needed parameters */
  ui_alignment->logo = NULL;

  return ui_alignment;
}


/* function that sets up an align point dialog */
void ui_alignment_dialog_create(ui_study_t * ui_study) {
  
  ui_alignment_t * ui_alignment;
  guint count;
  GtkWidget * table;
  GtkWidget * vseparator;
  GtkWidget * vbox;
  volumes_t * volumes;
  gchar * temp_strings[1];
  

  ui_alignment = ui_alignment_init();
  ui_alignment->volumes = volumes_ref(study_volumes(ui_study->study));

  ui_alignment->dialog = gtk_window_new(GTK_WINDOW_DIALOG);
  gtk_signal_connect(GTK_OBJECT(ui_alignment->dialog), "delete_event",
		     GTK_SIGNAL_FUNC(ui_alignment_dialog_cb_delete_event),
		     ui_alignment);

  ui_alignment->druid = gnome_druid_new();
  gtk_container_add(GTK_CONTAINER(ui_alignment->dialog), ui_alignment->druid);
  gtk_signal_connect(GTK_OBJECT(ui_alignment->druid), "cancel", 
		     GTK_SIGNAL_FUNC(ui_alignment_dialog_cb_cancel), ui_alignment);
  gtk_object_set_data(GTK_OBJECT(ui_alignment->druid), "ui_study", ui_study);

  ui_alignment->logo = gdk_imlib_create_image_from_xpm_data(amide_logo_xpm);


  /* figure out how many volumes there are */
  count = volumes_count(ui_alignment->volumes);


  /* --------------- initial page ------------------ */
#ifdef AMIDE_LIBGSL_SUPPORT
  ui_alignment->pages[INTRO_PAGE]= 
    gnome_druid_page_start_new_with_vals("Data Set Alignment Wizard",
					 (count >= 2) ? start_page_text : data_set_error_page_text,
					 ui_alignment->logo, NULL);
  gnome_druid_append_page(GNOME_DRUID(ui_alignment->druid), 
			  GNOME_DRUID_PAGE(ui_alignment->pages[INTRO_PAGE]));
  if (count < 2) gnome_druid_set_buttons_sensitive(GNOME_DRUID(ui_alignment->druid), FALSE, FALSE, TRUE);
#else /* NO LIBGSL SUPPORT */
  ui_alignment->pages[INTRO_PAGE]= 
    gnome_druid_page_start_new_with_vals("Data Set Alignment Wizard",
					 start_page_text,
					 ui_alignment->logo, NULL);
  gnome_druid_append_page(GNOME_DRUID(ui_alignment->druid), 
			  GNOME_DRUID_PAGE(ui_alignment->pages[INTRO_PAGE]));
  gnome_druid_set_buttons_sensitive(GNOME_DRUID(ui_alignment->druid), FALSE, FALSE, TRUE);
#endif



  /*------------------ pick your volumes page ------------------ */
  ui_alignment->pages[VOLUME_PAGE] = 
    gnome_druid_page_standard_new_with_vals("Data Set Selection", ui_alignment->logo);
  gnome_druid_append_page(GNOME_DRUID(ui_alignment->druid), 
			  GNOME_DRUID_PAGE(ui_alignment->pages[VOLUME_PAGE]));
  vbox = GNOME_DRUID_PAGE_STANDARD(ui_alignment->pages[VOLUME_PAGE])->vbox;
  gtk_object_set_data(GTK_OBJECT(ui_alignment->pages[VOLUME_PAGE]), 
		      "which_page", GINT_TO_POINTER(VOLUME_PAGE));
  gtk_signal_connect(GTK_OBJECT(ui_alignment->pages[VOLUME_PAGE]), "next", 
		     GTK_SIGNAL_FUNC(ui_alignment_dialog_cb_next_page), ui_alignment);
  gtk_signal_connect(GTK_OBJECT(ui_alignment->pages[VOLUME_PAGE]), "prepare", 
		     GTK_SIGNAL_FUNC(ui_alignment_dialog_cb_prepare_page), ui_alignment);

  table = gtk_table_new(3,3,FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 5);
    
    
  temp_strings[0] = g_strdup("Align Data Set: (moving)");
  ui_alignment->clist_volume_moving = gtk_clist_new_with_titles(1, temp_strings);
  g_free(temp_strings[0]);
  gtk_clist_set_selection_mode(GTK_CLIST(ui_alignment->clist_volume_moving), GTK_SELECTION_SINGLE);
  gtk_object_set_data(GTK_OBJECT(ui_alignment->clist_volume_moving), "fixed", GINT_TO_POINTER(FALSE));
  gtk_signal_connect(GTK_OBJECT(ui_alignment->clist_volume_moving), "select_row",
		     GTK_SIGNAL_FUNC(ui_alignment_dialog_cb_select_volume),
		     ui_alignment);
  gtk_signal_connect(GTK_OBJECT(ui_alignment->clist_volume_moving), "unselect_row",
		     GTK_SIGNAL_FUNC(ui_alignment_dialog_cb_unselect_volume),
		     ui_alignment);
  gtk_table_attach(GTK_TABLE(table),GTK_WIDGET(ui_alignment->clist_volume_moving), 
		   0,1,0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL | GTK_EXPAND,X_PADDING, Y_PADDING);
  
  volumes = ui_alignment->volumes;
  count = 0;
  while (volumes != NULL) {
    temp_strings[0] = g_strdup_printf("%s", volumes->volume->name);
    gtk_clist_append(GTK_CLIST(ui_alignment->clist_volume_moving), temp_strings);
    g_free(temp_strings[0]);
    gtk_clist_set_row_data(GTK_CLIST(ui_alignment->clist_volume_moving), count, volumes->volume);
    
    volumes = volumes->next;
    count++;
  }
  
  vseparator = gtk_vseparator_new();
  gtk_table_attach(GTK_TABLE(table), vseparator,
		   1,2,0,2, 
		   0, GTK_FILL, X_PADDING, Y_PADDING);
  
  temp_strings[0] = g_strdup("With Data Set: (fixed)");
  ui_alignment->clist_volume_fixed = gtk_clist_new_with_titles(1, temp_strings);
  g_free(temp_strings[0]);
  gtk_clist_set_selection_mode(GTK_CLIST(ui_alignment->clist_volume_fixed), GTK_SELECTION_SINGLE);
  gtk_object_set_data(GTK_OBJECT(ui_alignment->clist_volume_fixed), "fixed", GINT_TO_POINTER(TRUE));
  gtk_signal_connect(GTK_OBJECT(ui_alignment->clist_volume_fixed), "select_row",
		     GTK_SIGNAL_FUNC(ui_alignment_dialog_cb_select_volume),
		     ui_alignment);
  gtk_signal_connect(GTK_OBJECT(ui_alignment->clist_volume_fixed), "unselect_row",
		     GTK_SIGNAL_FUNC(ui_alignment_dialog_cb_unselect_volume),
		     ui_alignment);
  gtk_table_attach(GTK_TABLE(table),GTK_WIDGET(ui_alignment->clist_volume_fixed), 
		   2,3,0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL | GTK_EXPAND,X_PADDING, Y_PADDING);
  
  volumes = ui_alignment->volumes;
  count = 0;
  while (volumes != NULL) {
    temp_strings[0] = g_strdup_printf("%s", volumes->volume->name);
    gtk_clist_append(GTK_CLIST(ui_alignment->clist_volume_fixed), temp_strings);
    g_free(temp_strings[0]);
    gtk_clist_set_row_data(GTK_CLIST(ui_alignment->clist_volume_fixed), count, volumes->volume);
    
    volumes = volumes->next;
    count++;
  }
  
  /*------------------ pick your alignment points page ------------------ */
  ui_alignment->pages[ALIGN_PTS_PAGE] = 
    gnome_druid_page_standard_new_with_vals("Alignment Points Selection", ui_alignment->logo);
  vbox = GNOME_DRUID_PAGE_STANDARD(ui_alignment->pages[ALIGN_PTS_PAGE])->vbox;
  gtk_object_set_data(GTK_OBJECT(ui_alignment->pages[ALIGN_PTS_PAGE]), 
		      "which_page", GINT_TO_POINTER(ALIGN_PTS_PAGE));
  gnome_druid_append_page(GNOME_DRUID(ui_alignment->druid), 
			  GNOME_DRUID_PAGE(ui_alignment->pages[ALIGN_PTS_PAGE]));
  gtk_signal_connect(GTK_OBJECT(ui_alignment->pages[ALIGN_PTS_PAGE]), "prepare", 
		     GTK_SIGNAL_FUNC(ui_alignment_dialog_cb_prepare_page), ui_alignment);

  table = gtk_table_new(2,2,FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 5);
    
    
  temp_strings[0] = g_strdup("Use Which Points for Alignment:");
  ui_alignment->clist_points = gtk_clist_new_with_titles(1, temp_strings);
  g_free(temp_strings[0]);
  gtk_clist_set_selection_mode(GTK_CLIST(ui_alignment->clist_points), GTK_SELECTION_MULTIPLE);
  gtk_signal_connect(GTK_OBJECT(ui_alignment->clist_points), "select_row",
		     GTK_SIGNAL_FUNC(ui_alignment_dialog_cb_select_point),
		     ui_alignment);
  gtk_signal_connect(GTK_OBJECT(ui_alignment->clist_points), "unselect_row",
		     GTK_SIGNAL_FUNC(ui_alignment_dialog_cb_unselect_point),
		     ui_alignment);
  gtk_table_attach(GTK_TABLE(table),GTK_WIDGET(ui_alignment->clist_points), 
		   0,1,0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL | GTK_EXPAND,X_PADDING, Y_PADDING);
  

  /* ----------------  conclusion page ---------------------------------- */
  ui_alignment->pages[CONCLUSION_PAGE] = 
    gnome_druid_page_finish_new_with_vals("Conclusion", end_page_text,
					  ui_alignment->logo, NULL);
  gtk_object_set_data(GTK_OBJECT(ui_alignment->pages[CONCLUSION_PAGE]), 
		      "which_page", GINT_TO_POINTER(CONCLUSION_PAGE));
  gtk_signal_connect(GTK_OBJECT(ui_alignment->pages[CONCLUSION_PAGE]), "prepare", 
		     GTK_SIGNAL_FUNC(ui_alignment_dialog_cb_prepare_page), ui_alignment);
  gtk_signal_connect(GTK_OBJECT(ui_alignment->pages[CONCLUSION_PAGE]), "finish",
		     GTK_SIGNAL_FUNC(ui_alignment_dialog_cb_finish), ui_alignment);
  gnome_druid_append_page(GNOME_DRUID(ui_alignment->druid), 
			  GNOME_DRUID_PAGE(ui_alignment->pages[CONCLUSION_PAGE]));

  /* --------------- page shown if no alignment points ------------------ */
  ui_alignment->pages[NO_ALIGN_PTS_PAGE] =
    gnome_druid_page_finish_new_with_vals("Alignment Error", align_pts_error_page_text, 
					  ui_alignment->logo, NULL);
  gtk_object_set_data(GTK_OBJECT(ui_alignment->pages[NO_ALIGN_PTS_PAGE]), 
		      "which_page", GINT_TO_POINTER(NO_ALIGN_PTS_PAGE));
  gtk_signal_connect(GTK_OBJECT(ui_alignment->pages[NO_ALIGN_PTS_PAGE]), "prepare", 
		     GTK_SIGNAL_FUNC(ui_alignment_dialog_cb_prepare_page), ui_alignment);
  gtk_signal_connect(GTK_OBJECT(ui_alignment->pages[NO_ALIGN_PTS_PAGE]), "back", 
		     GTK_SIGNAL_FUNC(ui_alignment_dialog_cb_back_page), ui_alignment);
  gnome_druid_append_page(GNOME_DRUID(ui_alignment->druid), 
			  GNOME_DRUID_PAGE(ui_alignment->pages[NO_ALIGN_PTS_PAGE]));


  gtk_widget_show_all(ui_alignment->dialog);
  return;
}











