/* libmdc_interface.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2014 Andy Loening
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

#ifdef AMIDE_LIBMDC_SUPPORT

#ifndef __LIBMDC_INTERFACE_H__
#define __LIBMDC_INTERFACE_H__

/* includes always needed with this file */
#include "amitk_data_set.h"

typedef enum {
  LIBMDC_NONE,
  LIBMDC_RAW, 
  LIBMDC_ASCII, 
  LIBMDC_GIF, 
  LIBMDC_ACR, 
  LIBMDC_CONC, 
  LIBMDC_ECAT6, 
  LIBMDC_ECAT7, 
  LIBMDC_INTF, 
  LIBMDC_ANLZ, 
  LIBMDC_DICM,
  LIBMDC_NIFTI,
  LIBMDC_NUM_FORMATS
} libmdc_format_t;

typedef enum {
  LIBMDC_IMPORT_GUESS,
  LIBMDC_IMPORT_GIF,
  LIBMDC_IMPORT_ACR, 
  LIBMDC_IMPORT_CONC, 
  LIBMDC_IMPORT_ECAT6, 
  LIBMDC_IMPORT_ECAT7, 
  LIBMDC_IMPORT_INTF, 
  LIBMDC_IMPORT_ANLZ, 
  LIBMDC_IMPORT_DICM,
  LIBMDC_IMPORT_NIFTI,
  LIBMDC_NUM_IMPORT_METHODS
} libmdc_import_t;

typedef enum {
  LIBMDC_EXPORT_ACR, 
  LIBMDC_EXPORT_CONC, 
  LIBMDC_EXPORT_ECAT6, 
  LIBMDC_EXPORT_INTF, 
  LIBMDC_EXPORT_ANLZ, 
  LIBMDC_EXPORT_DICM,
  LIBMDC_EXPORT_NIFTI,
  LIBMDC_NUM_EXPORT_METHODS
} libmdc_export_t;

/* external functions */
gboolean libmdc_supports(libmdc_format_t format);
AmitkDataSet * libmdc_import(const gchar * filename, 
			     const libmdc_format_t libmdc_format,
			     AmitkPreferences * preferences,
			     AmitkUpdateFunc update_func,
			     gpointer update_data);

/* voxel_size only used if resliced=TRUE */
/* if bounding_box == NULL, will create its own using the minimal necessary */
gboolean libmdc_export(AmitkDataSet * ds,
		       const gchar * filename, 
		       const libmdc_format_t libmdc_format,
		       const gboolean resliced,
		       const AmitkPoint voxel_size,
		       const AmitkVolume * bounding_box,
		       AmitkUpdateFunc update_func,
		       gpointer update_data);

extern libmdc_format_t libmdc_import_to_format[];
extern gchar * libmdc_import_menu_names[];
extern gchar * libmdc_import_menu_explanations[];

extern libmdc_format_t libmdc_export_to_format[];
extern gchar * libmdc_export_menu_names[];
extern gchar * libmdc_export_menu_explanations[];

#endif /* __LIBMDC_INTERFACE_H__ */
#endif /* AMIDE_LIBMDC_SUPPORT */


