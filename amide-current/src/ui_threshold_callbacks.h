/* ui_threshold_callbacks.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000 Andy Loening
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
gint ui_threshold_callbacks_max_arrow(GtkWidget* widget, GdkEvent * event,
				      gpointer data);
void ui_threshold_callbacks_max_percentage(GtkWidget* widget, gpointer data);
void ui_threshold_callbacks_max_absolute(GtkWidget* widget, gpointer data);
gint ui_threshold_callbacks_min_arrow(GtkWidget* widget, GdkEvent * event,
				      gpointer data);
void ui_threshold_callbacks_min_percentage(GtkWidget* widget, gpointer data);
void ui_threshold_callbacks_min_absolute(GtkWidget* widget, gpointer data);
void ui_threshold_callbacks_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data);
