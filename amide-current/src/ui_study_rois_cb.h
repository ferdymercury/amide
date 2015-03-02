/* ui_study_rois_cb.h
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

/* defines */
#define UI_STUDY_ROIS_SHIFT_BUTTON 1
#define UI_STUDY_ROIS_SHIFT_MASK GDK_BUTTON1_MASK
#define UI_STUDY_ROIS_ROTATE_BUTTON 2
#define UI_STUDY_ROIS_ROTATE_MASK GDK_BUTTON2_MASK
#define UI_STUDY_ROIS_RESIZE_BUTTON 3
#define UI_STUDY_ROIS_RESIZE_MASK GDK_BUTTON3_MASK



/* external functions */
gboolean ui_study_rois_cb_roi_event(GtkWidget* widget, GdkEvent * event, gpointer data);
void ui_study_rois_cb_calculate_all(GtkWidget * widget, gpointer data);
void ui_study_rois_cb_calculate_selected(GtkWidget * widget, gpointer data);
void ui_study_rois_cb_calculate(ui_study_t * ui_study, gboolean All);
void ui_study_rois_cb_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data);
