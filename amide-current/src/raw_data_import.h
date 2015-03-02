/* raw_data_import.h
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


/* header files that are always needed with this file */
#include "raw_data.h"

/* raw_data information structure */
typedef struct raw_data_info_t {
  gchar * filename;
  guint total_file_size;
  raw_data_format_t raw_data_format;
  guint offset;
  GtkWidget * num_bytes_label1;
  GtkWidget * num_bytes_label2;
  GtkWidget * read_offset_label;
  GtkWidget * dialog;
  volume_t * volume;
} raw_data_info_t;

/* internal functions */
void raw_data_import_dialog(raw_data_info_t * raw_data_info);

/* external functions */
void raw_data_update_offset_label(raw_data_info_t * raw_data_info);
guint raw_data_update_num_bytes(raw_data_info_t * raw_data_info);
volume_t * raw_data_import(gchar * filename);







