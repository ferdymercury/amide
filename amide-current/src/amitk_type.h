/* amitk_type.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2003-2014 Andy Loening
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

#ifndef __AMITK_TYPE_H__
#define __AMITK_TYPE_H__


#include <glib.h>

G_BEGIN_DECLS

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


typedef gboolean (*AmitkUpdateFunc)      (gpointer, char *, gdouble);


G_END_DECLS

#endif /* __AMITK_TYPE_H__ */
