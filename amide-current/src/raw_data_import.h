/* raw_data_import.h
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000 Andy Loening
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

typedef enum {UBYTE, SBYTE, 
	      USHORT_LE, SSHORT_LE, 
	      UINT_LE, SINT_LE, 
	      FLOAT_LE, DOUBLE_LE, 
	      USHORT_BE, SSHORT_BE, 
	      UINT_BE, SINT_BE,
	      FLOAT_BE, DOUBLE_BE,
	      NUM_DATA_FORMATS} data_format_t;


/* raw_data information structure */
typedef struct raw_data_info_t {
  guint total_file_size;
  data_format_t data_format;
  guint offset;
  GtkWidget * num_bytes_label;
  GtkWidget * dialog;
  amide_volume_t * temp_volume;
} raw_data_info_t;

/* internal functions */
void raw_data_import_dialog(gchar * raw_data_filename, raw_data_info_t * raw_data_info);

/* external functions */
guint raw_data_calc_num_bytes(raw_data_info_t * raw_data_info);
guint raw_data_ui_num_bytes(raw_data_info_t * raw_data_info);
amide_volume_t * raw_data_import(gchar * filename);

/* external variables */
extern gchar * data_format_names[];
extern guint data_sizes[];





