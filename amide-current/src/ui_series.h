/* ui_series.h
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

#ifndef __UI_SERIES_H__
#define __UI_SERIES_H__

/* header files that are always needed with this file */
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "amitk_study.h"

#define UI_SERIES_L_MARGIN 2.0
#define UI_SERIES_R_MARGIN UI_SERIES_L_MARGIN
#define UI_SERIES_TOP_MARGIN 2.0
#define UI_SERIES_BOTTOM_MARGIN 15.0

/* external functions */
void ui_series_create(AmitkStudy * study, 
		      AmitkObject * active_object, 
		      GList * selected_objects,
		      AmitkPreferences * preferences);
GtkWidget * ui_series_init_dialog_create(AmitkStudy * study,
					 GtkWindow * parent);

#endif /* UI_SERIES_H */



