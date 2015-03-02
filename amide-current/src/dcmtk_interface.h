/* dcmtk_interface.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2005-2014 Andy Loening
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

#ifdef AMIDE_LIBDCMDATA_SUPPORT

#ifndef __DCMTK_INTERFACE_H__
#define __DCMTK_INTERFACE_H__

/* includes always needed with this file */
#include "amitk_data_set.h"

G_BEGIN_DECLS

extern const gchar * dcmtk_version;

/* external functions */
gboolean dcmtk_test_dicom(const gchar * filename);
GList * dcmtk_import(const gchar * filename, 
		     gchar ** pstudyname,
		     AmitkPreferences * preferences,
		     AmitkUpdateFunc update_func,
		     gpointer update_data);
gboolean dcmtk_export(AmitkDataSet * ds, 
		      const gchar * dir_or_filename,
		      const gchar * studyname,
		      const gboolean resliced,
		      const AmitkPoint voxel_size,
		      const AmitkVolume * bounding_box,
		      AmitkUpdateFunc update_func,
		      gpointer update_data);

G_END_DECLS

#endif /* __DCMTK_INTERFACE_H__ */
#endif /* AMIDE_LIBDCMDATA_SUPPORT */


