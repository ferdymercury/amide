/* ui_study_rois_cb.c
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
#include "ui_common.h"
#include "ui_study.h"
#include "ui_study_rois_cb.h"


/* function called when an event occurs on the roi item 
   notes:
   - new roi's are handled by ui_study_cb_canvas_event
   - widget should generally by GnomeCanvasLine type */
gboolean ui_study_rois_cb_roi_event(GtkWidget* widget, GdkEvent * event, gpointer data) {

  ui_study_t * ui_study = data;
  realpoint_t real_loc, canvas_rp; 
  view_t i_view;
  canvaspoint_t canvas_cp, diff;
  realpoint_t t[3]; /* temp variables */
  realpoint_t new_center, new_radius, roi_center, roi_radius;
  realspace_t * canvas_coord_frame_p;
  realpoint_t * canvas_far_corner;
  realpoint_t canvas_zoom;
  roi_t * roi;
  volume_list_t * current_slices[NUM_VIEWS];
  GdkPixbuf * rgb_image;
  gint rgb_width, rgb_height;
  static realpoint_t center, radius;
  static view_t view_static;
  static realpoint_t initial_real_loc;
  static realpoint_t initial_canvas_rp;
  static canvaspoint_t last_pic_cp;
  static gboolean dragging = FALSE;
  static ui_roi_list_t * current_roi_list_item = NULL;
  static GnomeCanvasItem * roi_item = NULL;
  static double theta;
  static realpoint_t roi_zoom;
  static realpoint_t shift;


  /* sanity checks */
  if (ui_study->current_mode == VOLUME_MODE) 
    return TRUE;
  if (ui_study->current_volume == NULL)
    return TRUE; 
  if (study_volumes(ui_study->study) == NULL)
    return TRUE;
  for (i_view=0; i_view<NUM_VIEWS; i_view++) {
    current_slices[i_view] = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[i_view]), "slices");
    if (current_slices[i_view] == NULL) {
      g_warning("null slice\n");
      return TRUE;
    }
  }

		   
  /* iterate through all the currently drawn roi's to figure out which
     roi this item corresponds to and what canvas it's on */
  if (roi_item == NULL) {
    view_static = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(widget), "view"));
    roi = gtk_object_get_data(GTK_OBJECT(widget), "roi");
    current_roi_list_item = ui_roi_list_get_ui_roi(ui_study->current_rois, roi);
  }
      
  /* get the location of the event, and convert it to the canvas coordinates */
  gnome_canvas_w2c_d(ui_study->canvas[view_static], event->button.x, event->button.y, 
		     &canvas_cp.x, &canvas_cp.y);

  /* get the coordinate frame and the far corner for the current canvas,
     the canvas_corner is in the canvas_coord_frame.  Also get the rgb_image
     for it's height and width */
  canvas_coord_frame_p = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view_static]), "coord_frame");
  canvas_far_corner = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view_static]), "far_corner");
  rgb_image = gtk_object_get_data(GTK_OBJECT(ui_study->canvas[view_static]), "rgb_image");
  rgb_width = gdk_pixbuf_get_width(rgb_image);
  rgb_height = gdk_pixbuf_get_height(rgb_image);



  /* Convert the event location info to real units */
  canvas_rp = ui_study_cp_2_rp(ui_study->canvas[view_static], canvas_cp, 
				study_view_thickness(ui_study->study));
  real_loc = realspace_alt_coord_to_base(canvas_rp, *canvas_coord_frame_p);

  /* switch on the event which called this */
  switch (event->type)
    {

    case GDK_ENTER_NOTIFY:
      ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, real_loc);
      ui_common_place_cursor(UI_CURSOR_OLD_ROI_MODE, GTK_WIDGET(ui_study->canvas[view_static]));
      break;


    case GDK_LEAVE_NOTIFY:
      /* yes, do it twice, there's a bug where if the canvas gets covered by a menu,
	 no LEAVE_NOTIFY is called for the GDK_ENTER_NOTIFY */
      ui_common_remove_cursor(GTK_WIDGET(ui_study->canvas[view_static]));
      ui_common_remove_cursor(GTK_WIDGET(ui_study->canvas[view_static]));
      break;
      


    case GDK_BUTTON_PRESS:
      ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, real_loc);
      dragging = TRUE;

      /* last second sanity check */
      if (current_roi_list_item == NULL) {
	g_warning("%s: null current_roi_list_item?", PACKAGE);
	return TRUE;
      }

      if (event->button.button == UI_STUDY_ROIS_SHIFT_BUTTON)
	gnome_canvas_item_grab(GNOME_CANVAS_ITEM(widget),
			       GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			       ui_common_cursor[UI_CURSOR_OLD_ROI_SHIFT],
			       event->button.time);
      else if (event->button.button == UI_STUDY_ROIS_ROTATE_BUTTON)
	gnome_canvas_item_grab(GNOME_CANVAS_ITEM(widget),
			       GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			       ui_common_cursor[UI_CURSOR_OLD_ROI_ROTATE],
			       event->button.time);
      else if (event->button.button == UI_STUDY_ROIS_RESIZE_BUTTON)
	gnome_canvas_item_grab(GNOME_CANVAS_ITEM(widget),
			       GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			       ui_common_cursor[UI_CURSOR_OLD_ROI_RESIZE],
			       event->button.time);
      else /* what the hell button got pressed? */
	return TRUE;


      /* save the roi we're going to manipulate for future use */
      roi_item = current_roi_list_item->canvas_roi[view_static];

      /* save some values in static variables for future use */
      initial_real_loc = real_loc;
      initial_canvas_rp = canvas_rp;
      last_pic_cp = canvas_cp;
      theta = 0.0;
      roi_zoom.x = roi_zoom.y = roi_zoom.z = 1.0;
      shift = realpoint_zero;

      t[0] = 
	realspace_base_coord_to_alt(rs_offset(current_roi_list_item->roi->coord_frame),
				    current_roi_list_item->roi->coord_frame);
      t[1] = current_roi_list_item->roi->corner;
      REALPOINT_MADD(0.5,t[1],0.5,t[0],center);
      REALPOINT_DIFF(t[1],t[0],radius);
      REALPOINT_CMULT(0.5,radius,radius);

      break;

    case GDK_MOTION_NOTIFY:
      ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, real_loc);
      if (dragging && 
	  ((event->motion.state & 
	    (UI_STUDY_ROIS_SHIFT_MASK | UI_STUDY_ROIS_ROTATE_MASK | UI_STUDY_ROIS_RESIZE_MASK)))) {

	/* last second sanity check */
	if (current_roi_list_item == NULL) {
	  g_warning("%s: null current_roi_list_item?", PACKAGE);
	  return TRUE;
	}


	if (event->motion.state & UI_STUDY_ROIS_RESIZE_MASK) {
	  /* RESIZE button Pressed, we're scaling the object */
	  /* note, I'd like to use the function "gnome_canvas_item_scale"
	     but this isn't defined in the current version of gnome.... 
	     so I'll have to do a whole bunch of shit*/
	  /* also, this "zoom" strategy won't work if the object is not
	     aligned with the view.... oh well... good enough for now... */
	  double affine[6];
	  realpoint_t item_center;
	  double cos_r, sin_r, rot;
	  
	  /* calculating the radius and center wrt the canvas */
	  roi_radius = realspace_alt_dim_to_alt(radius, current_roi_list_item->roi->coord_frame,
						*canvas_coord_frame_p);
	  roi_center = realspace_alt_coord_to_alt(center, current_roi_list_item->roi->coord_frame,
						  *canvas_coord_frame_p);
	  t[0] = rp_diff(initial_canvas_rp, roi_center);
	  t[1] = rp_diff(canvas_rp,roi_center);
	  
	  /* figure out what the center of the roi is in gnome_canvas_item coords */
	  /* compensate for X's origin being top left (not bottom left) */
	  item_center.x = ((roi_center.x/(*canvas_far_corner).x)
			   *rgb_width
			   +UI_STUDY_TRIANGLE_HEIGHT);
	  item_center.y = 
	    rgb_height - 
	    ((roi_center.y/(*canvas_far_corner).y)
	     *rgb_height)
	    +UI_STUDY_TRIANGLE_HEIGHT;

	  /* figure out the zoom we're specifying via the canvas */
	  canvas_zoom.x = (roi_radius.x+t[1].x)/(roi_radius.x+t[0].x);
	  canvas_zoom.y = (roi_radius.x+t[1].y)/(roi_radius.x+t[0].y);
	  canvas_zoom.z = 1.0;

	  //	  g_print("----------------------\n");
	  //	  g_print("canvas_zoom\t%5.3f %5.3f %5.3f\n",canvas_zoom.x,canvas_zoom.y,canvas_zoom.z);
	  /* translate the canvas zoom into the ROI's coordinate frame */
	  t[2].x = t[2].y = t[2].z = 1.0;
	  REALPOINT_SUB(canvas_zoom,t[2],roi_zoom);
	  roi_zoom =  realspace_alt_coord_to_base(roi_zoom, *canvas_coord_frame_p);
	  roi_zoom = rp_sub(roi_zoom, rs_offset(*canvas_coord_frame_p)); 
	  roi_zoom = rp_add(roi_zoom, rs_offset(current_roi_list_item->roi->coord_frame));
	  roi_zoom = realspace_base_coord_to_alt(roi_zoom, current_roi_list_item->roi->coord_frame);
	  REALPOINT_ADD(roi_zoom,t[2],roi_zoom);
	  //	  g_print("roi_zoom\t%5.3f %5.3f %5.3f\n",roi_zoom.x,roi_zoom.y,roi_zoom.z);

	  /* get the portions of the roi_zoom that are in the plane of the canvas */
	  //	  canvas_zoom = realspace_coord_to_orthogonal_view(roi_zoom, view_static);

	  /* do a wild ass affine matrix so that we can scale while preserving angles */

	  /* first, figure out how much the roi is rotated in the plane of the canvas */
	  rot = 0; // need to come up with a way of getting the in-plane rotation of the ROI

	  /* precompute cos and sin of rot */
	  cos_r = cos(rot);
	  sin_r = sin(rot);

	  //	  g_print("rot\t%5.3f\tcos_r %5.3f\tsin_r %5.3f\n",(rot/M_PI)*180, cos_r, sin_r);
	  //	  g_print("new zoom\t%5.3f %5.3f %5.3f\n",  canvas_zoom.x, canvas_zoom.y, canvas_zoom.z);

	  /* and compute the affine matrix */
	  affine[0] = canvas_zoom.x * cos_r * cos_r + canvas_zoom.y * sin_r * sin_r;
	  affine[1] = (canvas_zoom.x-canvas_zoom.y)* cos_r * sin_r;
	  affine[2] = affine[1];
	  affine[3] = canvas_zoom.x * sin_r * sin_r + canvas_zoom.y * cos_r * cos_r;
	  affine[4] = item_center.x - item_center.x*canvas_zoom.x*cos_r*cos_r 
	    - item_center.x*canvas_zoom.y*sin_r*sin_r + (canvas_zoom.y-canvas_zoom.x)*item_center.y*cos_r*sin_r;
	  affine[5] = item_center.y - item_center.y*canvas_zoom.y*cos_r*cos_r 
	    - item_center.y*canvas_zoom.x*sin_r*sin_r + (canvas_zoom.y-canvas_zoom.x)*item_center.x*cos_r*sin_r;
	  gnome_canvas_item_affine_absolute(GNOME_CANVAS_ITEM(roi_item),affine);

	} else if (event->motion.state & UI_STUDY_ROIS_ROTATE_MASK) {
	  /* rotate button Pressed, we're rotating the object */
	  /* note, I'd like to use the function "gnome_canvas_item_rotate"
	     but this isn't defined in the current version of gnome.... 
	     so I'll have to do a whole bunch of shit*/
	  double affine[6];
	  realpoint_t item_center;

	  //	  gnome_canvas_item_i2w_affine(GNOME_CANVAS_ITEM(roi_item),affine);
	  roi_center = realspace_alt_coord_to_alt(center,
						  current_roi_list_item->roi->coord_frame,
						  *canvas_coord_frame_p);
	  t[0] = rp_sub(initial_canvas_rp,roi_center);
	  t[1] = rp_sub(canvas_rp,roi_center);

	  /* figure out theta */
	  theta = acos(REALPOINT_DOT_PRODUCT(t[0],t[1])/
		       (REALPOINT_MAGNITUDE(t[0]) * REALPOINT_MAGNITUDE(t[1])));
	  
	  /* correct for the fact that acos is always positive by using the cross product */
	  if ((t[0].x*t[1].y-t[0].y*t[1].x) > 0.0)
	    theta = -theta;

	  /* figure out what the center of the roi is in canvas_item coords */
	  /* compensate for x's origin being top left (ours is bottom left) */
	  item_center.x = ((roi_center.x/(*canvas_far_corner).x)
			   *rgb_width
			   +UI_STUDY_TRIANGLE_HEIGHT);
	  item_center.y = 
	    rgb_height - 
	    ((roi_center.y/(*canvas_far_corner).y)
	     *rgb_height)
	    +UI_STUDY_TRIANGLE_HEIGHT;
	  affine[0] = cos(theta);
	  affine[1] = sin(theta);
	  affine[2] = -affine[1];
	  affine[3] = affine[0];
	  affine[4] = (1.0-affine[0])*item_center.x+affine[1]*item_center.y;
	  affine[5] = (1.0-affine[3])*item_center.y+affine[2]*item_center.x;
	  gnome_canvas_item_affine_absolute(GNOME_CANVAS_ITEM(roi_item),affine);

	} else if (event->motion.state & UI_STUDY_ROIS_SHIFT_MASK) {
	  /* shift button pressed, we're shifting the object */
	  /* do movement calculations */
	  shift = rp_sub(real_loc, initial_real_loc);
	  diff = cp_sub(canvas_cp, last_pic_cp);
	  /* convert the location back to world coordiantes */
	  gnome_canvas_item_i2w(GNOME_CANVAS_ITEM(roi_item)->parent, &diff.x, &diff.y);
	  gnome_canvas_item_move(GNOME_CANVAS_ITEM(roi_item),diff.x,diff.y); 
	} else {
	  g_warning("%s: reached unsuspected condition in ui_study_rois_cb.c", PACKAGE);
	  dragging = FALSE;
	  return TRUE;
	}
	last_pic_cp = canvas_cp;
      }
      break;
      
    case GDK_BUTTON_RELEASE:
      ui_study_update_help_info(ui_study, HELP_INFO_UPDATE_LOCATION, real_loc);
      gnome_canvas_item_ungrab(GNOME_CANVAS_ITEM(widget), event->button.time);
      dragging = FALSE;
      roi_item = NULL;

      /* last second sanity check */
      if (current_roi_list_item == NULL) {
	g_warning("%s: null current_roi_list_item?", PACKAGE);
	return TRUE;
      }

      /* compensate for the fact that our origin is left bottom, not X's left top */
      /* also, compensate for sagittal being a left-handed coordinate frame */
      if (view_static != SAGITTAL)
	theta = -theta;

#ifdef AMIDE_DEBUG
      g_print("roi changes\n");
      g_print("\tshift\t%5.3f\t%5.3f\t%5.3f\n",shift.x,shift.y,shift.z);
      g_print("\troi_zoom\t%5.3f\t%5.3f\t%5.3f\n",roi_zoom.x,roi_zoom.y,roi_zoom.z);
      g_print("\ttheta\t%5.3f\n",theta);
#endif

      /* apply our changes to the roi */

      if (event->button.button == UI_STUDY_ROIS_SHIFT_BUTTON) {
	/* ------------- apply any shift done -------------- */
	rs_set_offset(&current_roi_list_item->roi->coord_frame,
		      rp_add(shift, rs_offset(current_roi_list_item->roi->coord_frame)));
      } else if (event->button.button == UI_STUDY_ROIS_RESIZE_BUTTON) {
	/* ------------- apply any zoom done -------------- */
	REALPOINT_MULT(roi_zoom,radius,new_radius); 
	
	/* allright, figure out the new lower left corner */
	REALPOINT_SUB(center, new_radius, t[0]);
	
	/* the new roi offset will be the new lower left corner */
	rs_set_offset(&current_roi_list_item->roi->coord_frame,
		      realspace_alt_coord_to_base(t[0],
						  current_roi_list_item->roi->coord_frame));
	
	/* and the new upper right corner is simply twice the radius */
	REALPOINT_CMULT(2,new_radius,t[1]);
	current_roi_list_item->roi->corner = t[1];
	
      } else if (event->button.button == UI_STUDY_ROIS_ROTATE_BUTTON) {
	
	/* ------------- apply any rotation done -------------- */
	new_center = /* get the center in real coords */
	  realspace_alt_coord_to_base(center,
				      current_roi_list_item->roi->coord_frame);
	
	/* reset the offset of the roi coord_frame to the object's center */
	rs_set_offset(&current_roi_list_item->roi->coord_frame, new_center);

	/* now rotate the roi coord_frame axis */
	realspace_rotate_on_axis(&current_roi_list_item->roi->coord_frame,
				 rs_specific_axis(*canvas_coord_frame_p, ZAXIS),
				 theta);
	
	/* and recaculate the offset */
	new_center = /* get the center in roi coords */
	  realspace_base_coord_to_alt(new_center,
				      current_roi_list_item->roi->coord_frame);
	REALPOINT_SUB(new_center,radius,t[0]);
	t[0] = /* get the new offset in real coords */
	  realspace_alt_coord_to_base(t[0],
				      current_roi_list_item->roi->coord_frame);
	rs_set_offset(&current_roi_list_item->roi->coord_frame, t[0]); 
	
      } else 
	return TRUE; /* shouldn't get here */

      if (event->button.button == UI_STUDY_ROIS_SHIFT_BUTTON) {
	/* if we were moving the ROI, reset the view_center to the center of the current roi */
	t[0] = realspace_base_coord_to_alt(rs_offset(current_roi_list_item->roi->coord_frame),
					   current_roi_list_item->roi->coord_frame);
	t[1] = current_roi_list_item->roi->corner;
	center = rp_add(rp_cmult(0.5, t[0]), rp_cmult(0.5, t[1]));
	center = realspace_alt_coord_to_alt(center,
					    current_roi_list_item->roi->coord_frame,
					    study_coord_frame(ui_study->study));
	study_set_view_center(ui_study->study, center);

	/* update everything */
	ui_study_update_canvas(ui_study, NUM_VIEWS, UPDATE_ALL);
      } else { 
	/* we just need to update the ROI's otherwise */
	/* update the roi's */
	for (i_view=0;i_view<NUM_VIEWS;i_view++) 
	  current_roi_list_item->canvas_roi[i_view] =
	    ui_study_update_canvas_roi(ui_study,i_view,
				       current_roi_list_item->canvas_roi[i_view],
				       current_roi_list_item->roi);
      }
      
      break;
    default:
      break;
    }
      
  return FALSE;
}





void ui_study_rois_cb_calculate_all(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;

  ui_study_rois_cb_calculate(ui_study, TRUE);

  return;

}

void ui_study_rois_cb_calculate_selected(GtkWidget * widget, gpointer data) {
  ui_study_t * ui_study = data;

  ui_study_rois_cb_calculate(ui_study, FALSE);

  return;
}





/* analyse the roi's, if All is true, do all roi's, otherwise only
   do selected roi's */
void ui_study_rois_cb_calculate(ui_study_t * ui_study, gboolean all) {

  GnomeApp * app;
  gchar * title;
  gchar * line;
  guint j;
  GtkWidget * packing_table;
  GtkWidget * text;
  roi_list_t * current_roi_list;
  roi_list_t * temp_roi_list;
  roi_analysis_t analysis;
  ui_roi_list_t * temp_ui_roi_list;
  ui_volume_list_t * current_ui_volume_list;
  GdkFont * courier_font;

  /* load in a nice courier font to use */
  courier_font = gdk_font_load("-*-courier-*-*-*-*-*-*-*-*-*-*-*-*");

  /* figure out which volume we're dealing with */
  current_ui_volume_list = ui_study->current_volumes;

  /* get the list of roi's we're going to be calculating over */
  if (all)
    current_roi_list = roi_list_copy(study_rois(ui_study->study));
  else {
    temp_ui_roi_list = ui_study->current_rois;
    current_roi_list = NULL;
    while (temp_ui_roi_list != NULL) {
      current_roi_list = roi_list_add_roi(current_roi_list, temp_ui_roi_list->roi);
      temp_ui_roi_list = temp_ui_roi_list->next;
    }
  }

  /* start setting up the widget we'll display the info from */
  title = g_strdup_printf("Roi Analysis: Study %s", study_name(ui_study->study));
  app = GNOME_APP(gnome_app_new(PACKAGE, title));
  g_free(title);

  /* setup the callbacks for app */
  gtk_signal_connect(GTK_OBJECT(app), "delete_event",
                     GTK_SIGNAL_FUNC(ui_study_rois_cb_delete_event),
                     ui_study);
                      

 /* make the widgets for this dialog box */
  packing_table = gtk_table_new(1,1,FALSE);
  gnome_app_set_contents(app, GTK_WIDGET(packing_table));
                          

  /* create the text widget */
  text = gtk_text_new(NULL,NULL);
  gtk_text_set_editable(GTK_TEXT(text), FALSE); /* don't want user to edit */
  gtk_text_set_word_wrap(GTK_TEXT(text), FALSE); 
  gtk_text_set_line_wrap(GTK_TEXT(text), FALSE);
  gtk_widget_set_usize(GTK_WIDGET(text), 700,0);
  
  line = g_strdup_printf("Roi Analysis:\t\tStudy: %s\n",
			 study_name(ui_study->study));
  gtk_text_insert(GTK_TEXT(text), courier_font, NULL, NULL, line, -1);
  g_free(line);

  line = g_strdup_printf("---------------------------------------------------------------------------------------------\n");
  gtk_text_insert(GTK_TEXT(text), courier_font, NULL, NULL, line, -1);
  g_free(line);


  while (current_ui_volume_list != NULL) {

    /*what volume we're calculating on */
    line = g_strdup_printf("Calculating on Volume:\t%s\t using external scaling factor:\t%5.3f\n", 
			   current_ui_volume_list->volume->name, 
			   current_ui_volume_list->volume->external_scaling);
    gtk_text_insert(GTK_TEXT(text), courier_font, NULL, NULL, line, -1);
    g_free(line);
    
    
    /* iterate over the roi's */
    temp_roi_list = current_roi_list;
    while(temp_roi_list != NULL) {
      
      line = g_strdup_printf("\tRoi: %s\tType: %s\tGrain Size %s\n", 
			     temp_roi_list->roi->name, 
			     roi_type_names[temp_roi_list->roi->type],
			     roi_grain_names[temp_roi_list->roi->grain]);
      gtk_text_insert(GTK_TEXT(text), courier_font, NULL, NULL, line, -1);
      g_free(line);
      
      line = g_strdup_printf("\t%-11s\t%-11s\t%-11s\t%-11s\t%-11s\t%-11s\t%-11s\t%-11s\t%-11s\n",
			     "   Frame", "Time Midpt", "  Voxels", "   Mean", "    Var", "  Std Dev", "   Std E", "    Min", "    Max");
      gtk_text_insert(GTK_TEXT(text), courier_font, NULL, NULL, line, -1);
      g_free(line);
      
      /* iterate over the volume's frames */
      for (j=0;j<current_ui_volume_list->volume->data_set->dim.t;j++) {
	/* calculate the info for this roi */
	analysis = roi_calculate_analysis(temp_roi_list->roi,current_ui_volume_list->volume,temp_roi_list->roi->grain,j);
	
	/* print the info for this roi */
	line = 
	  g_strdup_printf("\t%11d\t% 10.3f\t% 10.3f\t%10.3f\t% 10.3f\t% 10.3f\t% 10.3f\t% 10.3f\t% 10.3f\n",
			  j,
			  analysis.time_midpoint,
			  analysis.voxels,
			  analysis.mean,
			  analysis.var,
			  sqrt(analysis.var),
			  sqrt(analysis.var/analysis.voxels),
			  analysis.min,
			  analysis.max);
	gtk_text_insert(GTK_TEXT(text), courier_font, NULL, NULL, line, -1);
	g_free(line);
      }
      temp_roi_list = temp_roi_list->next; /* go to next roi */
    }
    current_ui_volume_list = current_ui_volume_list->next; /* go to the next volume */
  }

  /* and throw the text box into the packing table */
  gtk_table_attach(GTK_TABLE(packing_table),
		   GTK_WIDGET(text), 0,1,0,1,
		   X_PACKING_OPTIONS | GTK_FILL,
		   Y_PACKING_OPTIONS | GTK_FILL,
		   X_PADDING, Y_PADDING);

  /* deallocate our roi list if we created it */
  current_roi_list = roi_list_free(current_roi_list);

  /* and show all our widgets */
  gtk_widget_show_all(GTK_WIDGET(app));

  /* unref our font */
  gdk_font_unref(courier_font);

  return;
}


void ui_study_rois_cb_delete_event(GtkWidget* widget, 
					  GdkEvent * event, 
					  gpointer data) {

  /* ui_study_t * ui_study = data; */

  /* destroy the widget */
  gtk_widget_destroy(widget);

  return;
}









