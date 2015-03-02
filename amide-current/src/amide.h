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

/* some basic defines for packing tables */
#define X_PACKING_OPTIONS GTK_EXPAND
#define Y_PACKING_OPTIONS GTK_EXPAND
#define X_PADDING 5
#define Y_PADDING 5                 

/* typedef's */
typedef enum {TRANSVERSE, CORONAL, SAGITTAL, NUM_VIEWS} view_t;

/* setup the types for various internal data formats */
//typedef gdouble volume_data_t;
typedef gfloat volume_data_t;
typedef gdouble volume_time_t;


/* external variables */
extern gchar * view_names[];


