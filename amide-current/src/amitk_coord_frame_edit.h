/* amitk_coord_frame_edit.h
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


#ifndef __AMITK_COORD_FRAME_EDIT_H__
#define __AMITK_COORD_FRAME_EDIT_H__

/* includes we always need with this widget */
#include <gnome.h>
#include "realspace.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define AMITK_TYPE_COORD_FRAME_EDIT            (amitk_coord_frame_edit_get_type ())
#define AMITK_COORD_FRAME_EDIT(obj)            (GTK_CHECK_CAST ((obj), AMITK_TYPE_COORD_FRAME_EDIT, AmitkCoordFrameEdit))
#define AMITK_COORD_FRAME_EDIT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), AMITK_TYPE_COORD_FRAME_EDIT, AmitkCoordFrameEditClass))
#define AMITK_IS_COORD_FRAME_EDIT(obj)         (GTK_CHECK_TYPE ((obj), AMITK_TYPE_COORD_FRAME_EDIT))
#define AMITK_IS_COORD_FRAME_EDIT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_COORD_FRAME_EDIT))

typedef struct _AmitkCoordFrameEdit             AmitkCoordFrameEdit;
typedef struct _AmitkCoordFrameEditClass        AmitkCoordFrameEditClass;


struct _AmitkCoordFrameEdit
{
  GtkVBox vbox;

  GtkWidget * entry[NUM_AXIS][NUM_AXIS];

  realspace_t * coord_frame;
  realpoint_t center_of_rotation; /* in base coord frame */
  realspace_t * view_coord_frame;

};

struct _AmitkCoordFrameEditClass
{
  GtkVBoxClass parent_class;

  void (* coord_frame_changed) (AmitkCoordFrameEdit *CoordFrameEdit);
};  


GtkType       amitk_coord_frame_edit_get_type          (void);
GtkWidget*    amitk_coord_frame_edit_new               (realspace_t * coord_frame, 
		  				        realpoint_t center_of_rotation,
							realspace_t * view_coord_frame);
void          amitk_coord_frame_edit_change_center     (AmitkCoordFrameEdit * cfe,
							realpoint_t center_of_rotation);
void          amitk_coord_frame_edit_change_offset     (AmitkCoordFrameEdit * cfe,
							realpoint_t offset);
#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __AMITK_COORD_FRAME_EDIT_H__ */

