/* tb_roi_analysis_dialog.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2017 Andy Loening
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

#ifndef __TB_ROI_ANALYSIS_DIALOG_H__
#define __TB_ROI_ANALYSIS_DIALOG_H__


/* header files always needed with this one */
#include "amitk_study.h"

/* external functions */
void tb_roi_analysis(AmitkStudy * study, AmitkPreferences * preferences, GtkWindow * parent);
GtkWidget * tb_roi_analysis_init_dialog(GtkWindow * parent);


#endif /* __TB_ROI_ANALYSIS_DIALOG_H__ */





