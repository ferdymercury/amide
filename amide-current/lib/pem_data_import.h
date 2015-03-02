/* pem_data_import.h
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

/* defines */
#define PEM_DATA_X 30
#define PEM_DATA_Y 30
#define PEM_DATA_Z 24
#define PEM_DATA_FRAMES 1
#define PEM_DATA_FORMAT ASCII_NE
#define PEM_FILE_OFFSET 1
#define PEM_VOXEL_SIZE_X 2.0;
#define PEM_VOXEL_SIZE_Y 2.0;
#define PEM_VOXEL_SIZE_Z 2.0;


/* internal functions */

/* external functions */
volume_t * pem_data_import(const gchar * pem_data_filename, const gchar * pem_model_filename);






