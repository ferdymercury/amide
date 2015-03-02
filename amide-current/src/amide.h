/* amide.h
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

#ifndef __AMIDE_H__
#define __AMIDE_H__

/* some basic defines for packing tables */
#define X_PACKING_OPTIONS GTK_EXPAND
#define Y_PACKING_OPTIONS GTK_EXPAND
#define X_PADDING 5
#define Y_PADDING 5                 

/* typedef's */
typedef enum {TRANSVERSE, CORONAL, SAGITTAL, NUM_VIEWS} view_t;


/* setup the types for various internal data formats */
/* note, don't change the amide_data_t unless you want to do a 
   lot of digging through the code, a lot of assumptions are
   based on the data type being float */
typedef gfloat amide_data_t;
#define SIZE_OF_AMIDE_DATA_T 4
typedef gdouble amide_time_t;
#define SIZE_OF_AMIDE_TIME_T 8
typedef gdouble floatpoint_t;
#define SIZE_OF_FLOATPOINT_T 8
typedef gint16 intpoint_t;
#define SIZE_OF_INTPOINT_T 2;



/* external variables */
extern gchar * view_names[];

/* external functions */
void amide_register_window(gpointer * widget);
void amide_unregister_window(gpointer * widget);
void amide_unregister_all_windows(void);


#endif /* __AMIDE_H__ */
