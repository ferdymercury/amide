/* medcon_import.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2003 Andy Loening
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

#ifndef __MEDCON_IMPORT_H__
#define __MEDCON_IMPORT_H__

/* includes always needed with this file */
#include "amitk_data_set.h"

typedef enum {
  LIBMDC_NONE,
  LIBMDC_RAW, 
  LIBMDC_ASCII, 
  LIBMDC_GIF, 
  LIBMDC_ACR, 
  LIBMDC_INW, 
  LIBMDC_CONC, 
  LIBMDC_ECAT6, 
  LIBMDC_ECAT7, 
  LIBMDC_INTF, 
  LIBMDC_ANLZ, 
  LIBMDC_DICM,
  LIBMDC_NUM_IMPORT_METHODS
} libmdc_import_method_t;

/* external functions */
gboolean medcon_import_supports(libmdc_import_method_t submethod);
AmitkDataSet * medcon_import(const gchar * filename, 
			     libmdc_import_method_t submethod,
			     gboolean (*update_func)(),
			     gpointer update_data);

extern gchar * libmdc_menu_names[];
extern gchar * libmdc_menu_explanations[];

#endif /* __MEDCON_IMPORT_H__ */
#endif


