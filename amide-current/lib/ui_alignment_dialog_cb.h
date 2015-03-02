/* ui_alignment_dialog_cb.h
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




/* external functions */
gboolean ui_alignment_dialog_cb_next_page(GtkWidget * page, gpointer *druid, gpointer data);
gboolean ui_alignment_dialog_cb_back_page(GtkWidget * page, gpointer *druid, gpointer data);
void ui_alignment_dialog_cb_prepare_page(GtkWidget * page, gpointer * druid, gpointer data);
void ui_alignment_dialog_cb_select_volume(GtkCList * clist, gint row, gint column,
					  GdkEventButton * event, gpointer data);
void ui_alignment_dialog_cb_unselect_volume(GtkCList * clist, gint row, gint column,
					    GdkEventButton * event, gpointer data);
void ui_alignment_dialog_cb_select_point(GtkCList * clist, gint row, gint column,
					 GdkEventButton * event, gpointer data);
void ui_alignment_dialog_cb_unselect_point(GtkCList * clist, gint row, gint column,
					   GdkEventButton * event, gpointer data);
void ui_alignment_dialog_cb_finish(GtkWidget* widget, gpointer druid, gpointer data);
void ui_alignment_dialog_cb_cancel(GtkWidget* widget, gpointer data);
gboolean ui_alignment_dialog_cb_delete_event(GtkWidget * widget, GdkEvent * event, gpointer data);
