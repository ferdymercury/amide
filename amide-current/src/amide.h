/* amide.h
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

#ifndef __AMIDE_H__
#define __AMIDE_H__

#include <glib.h>

/* define a macro glib should have */
#define g_try_new(struct_type, n_structs)           \
    ((struct_type *) g_try_malloc (((gsize) sizeof (struct_type)) * ((gsize) (n_structs))))

#define AMITK_RESPONSE_EXECUTE 1


/* defines how many times we want the progress bar to be updated over the course of an action */
#define AMIDE_UPDATE_DIVIDER 40.0 /* must be float point */

/* some basic defines for packing tables */
#define X_PACKING_OPTIONS GTK_EXPAND
#define Y_PACKING_OPTIONS GTK_EXPAND
#define X_PADDING 5
#define Y_PADDING 5                 

/* typedef's */
typedef enum {
  AMITK_LAYOUT_LINEAR, 
  AMITK_LAYOUT_ORTHOGONAL,
  AMITK_LAYOUT_NUM
} AmitkLayout;


typedef enum {
  AMITK_EYE_LEFT, 
  AMITK_EYE_RIGHT, 
  AMITK_EYE_NUM
} AmitkEye;


typedef enum {
  AMITK_HELP_INFO_BLANK,
  AMITK_HELP_INFO_CANVAS_DATA_SET,
  AMITK_HELP_INFO_CANVAS_ROI,
  AMITK_HELP_INFO_CANVAS_FIDUCIAL_MARK,
  AMITK_HELP_INFO_CANVAS_ISOCONTOUR_ROI,
  AMITK_HELP_INFO_CANVAS_NEW_ROI,
  AMITK_HELP_INFO_CANVAS_NEW_ISOCONTOUR_ROI,
  AMITK_HELP_INFO_CANVAS_SHIFT_DATA_SET,
  AMITK_HELP_INFO_CANVAS_ROTATE_DATA_SET,
  AMITK_HELP_INFO_TREE_DATA_SET,
  AMITK_HELP_INFO_TREE_ROI,
  AMITK_HELP_INFO_TREE_FIDUCIAL_MARK,
  AMITK_HELP_INFO_TREE_STUDY,
  AMITK_HELP_INFO_TREE_NONE,
  AMITK_HELP_INFO_UPDATE_LOCATION,
  AMITK_HELP_INFO_UPDATE_THETA,
  AMITK_HELP_INFO_UPDATE_SHIFT,
  AMITK_HELP_INFO_NUM
} AmitkHelpInfo;

/* setup the types for various internal data formats */
/* note, don't change these unless you want to go digging through the
   code for locations where the signal marshallers expect the type
   to be DOUBLE.  Would also need to update the REAL_EQUAL type
   functions in amitk_point.h
*/

typedef gdouble amide_data_t;
#define SIZE_OF_AMIDE_DATA_T 8
#define AMITK_TYPE_DATA G_TYPE_DOUBLE

typedef gdouble amide_time_t;
#define SIZE_OF_AMIDE_TIME_T 8
#define AMITK_TYPE_TIME G_TYPE_DOUBLE

typedef gdouble amide_real_t;
#define SIZE_OF_AMIDE_REAL_T 8
#define AMITK_TYPE_REAL G_TYPE_DOUBLE

/* size of a point in integer space */
typedef gint16 amide_intpoint_t;
#define SIZE_OF_AMIDE_INTPOINT_T 2;



/* external variables */
extern gchar * view_names[];
extern gchar * object_menu_names[];

/* external functions */
void amide_register_window(gpointer * widget);
void amide_unregister_window(gpointer * widget);
void amide_unregister_all_windows(void);

#endif /* __AMIDE_H__ */
