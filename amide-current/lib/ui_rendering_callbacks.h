/* ui_rendering_callbacks.h
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
#ifdef AMIDE_LIBVOLPACK_SUPPORT


/* external functions */
void ui_rendering_callbacks_render(GtkWidget * widget, gpointer data);
void ui_rendering_callbacks_immediate(GtkWidget * widget, gpointer data);
void ui_rendering_callbacks_rotate(GtkAdjustment * adjustment, gpointer data);
void ui_rendering_parameters_pressed(GtkWidget * widget, gpointer data);
#ifdef AMIDE_MPEG_ENCODE_SUPPORT
void ui_rendering_movie_pressed(GtkWidget * widget, gpointer data);
#endif
void ui_rendering_callbacks_delete_event(GtkWidget* widget, GdkEvent * event, gpointer data);




#endif
