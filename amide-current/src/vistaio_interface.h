/* vistaio_interface.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2016-2017 Gert Wollny
 *
 * Author: Gert Wollny <gw.fossdev@gmail.com>
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


#ifdef AMIDE_VISTAIO_SUPPORT

#ifndef __VISTAIO_INTERFACE_H__
#define __VISTAIO_INTERFACE_H__


/* includes always needed with this file */
#include "amitk_data_set.h"

gboolean vistaio_test_vista(gchar *filename); 


/* external functions */
AmitkDataSet * vistaio_import(const gchar * filename, 
			      AmitkPreferences * preferences,
			      AmitkUpdateFunc update_func,
			      gpointer update_data);

/* voxel_size only used if resliced=TRUE */
/* if bounding_box == NULL, will create its own using the minimal necessary */
gboolean vistaio_export(AmitkDataSet * ds,
			const gchar * filename, 
			const gboolean resliced,
			const AmitkPoint voxel_size,
			const AmitkVolume * bounding_box,
			AmitkUpdateFunc update_func,
			gpointer update_data);


#endif

#endif /*AMIDE_VISTAIO_SUPPORT*/
