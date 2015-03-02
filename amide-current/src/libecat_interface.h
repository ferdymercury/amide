/* libecat_interface.h
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


#ifdef AMIDE_LIBECAT_SUPPORT

#ifndef __LIBECAT_INTERFACE_H__
#define __LIBECAT_INTERFACE_H__

/* headers always needed with this guy */
#include "amitk_data_set.h"

/* external functions */
AmitkDataSet * libecat_import(const gchar * filename,
			      AmitkPreferences * preferences,
			      AmitkUpdateFunc update_func,
			      gpointer update_data);

#endif /* __LIBECAT_INTERFACE_H__ */

#endif /* AMIDE_LIBECAT_SUPPORT */


