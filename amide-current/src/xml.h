/* xml.c - convience functions for working with xml files 
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

#ifndef __XML_H__
#define __XML_H__

/* header files that are always associated with this header file */
#include "amitk_type.h"
#include <libxml/tree.h>
#include <libxml/parser.h>

G_BEGIN_DECLS

/* functions */
void xml_convert_radix_to_local(gchar * string);
gboolean xml_check_file_32bit_okay(guint64 value);
gboolean xml_node_exists(xmlNodePtr nodes, const gchar * descriptor);
xmlNodePtr xml_get_node(xmlNodePtr nodes, const gchar * descriptor);
gchar * xml_get_string(xmlNodePtr nodes, const gchar * descriptor);
amide_time_t xml_get_time(xmlNodePtr nodes, const gchar * descriptor, gchar **perror_buf);
amide_time_t * xml_get_times(xmlNodePtr nodes, const gchar * descriptor, guint num_times, gchar **perror_buf);
amide_data_t xml_get_data(xmlNodePtr nodes, const gchar * descriptor, gchar **perror_buf);
amide_data_t xml_get_data_with_default(xmlNodePtr nodes, const gchar * descriptor, amide_data_t default_data);
amide_real_t xml_get_real(xmlNodePtr node, const gchar * descriptor, gchar **perror_buf);
amide_real_t xml_get_real_with_default(xmlNodePtr node, const gchar * descriptor, amide_real_t default_real);
gboolean xml_get_boolean(xmlNodePtr nodes, const gchar * descriptor, gchar **perror_buf);
gboolean xml_get_boolean_with_default(xmlNodePtr nodes, const gchar * descriptor, gboolean default_boolean);
gint xml_get_int(xmlNodePtr nodes, const gchar * descriptor, gchar **perror_buf);
gint xml_get_int_with_default(xmlNodePtr nodes, const gchar * descriptor, gint default_int);
guint xml_get_uint(xmlNodePtr nodes, const gchar * descriptor, gchar **perror_buf);
guint xml_get_uint_with_default(xmlNodePtr nodes, const gchar * descriptor, guint default_uint);
void xml_get_location_and_size(xmlNodePtr nodes, const gchar * descriptor, 
			       guint64 * location, guint64 * size, gchar **perror_buf);
void xml_save_string(xmlNodePtr node, const gchar * descriptor, const gchar * string);
void xml_save_time(xmlNodePtr node, const gchar * descriptor, const amide_time_t num);
void xml_save_times(xmlNodePtr node, const gchar * descriptor, const amide_time_t * numbers, const int num);
void xml_save_data(xmlNodePtr node, const gchar * descriptor, const amide_data_t num);
void xml_save_real(xmlNodePtr node, const gchar * descriptor, const amide_real_t num);
void xml_save_boolean(xmlNodePtr node, const gchar * descriptor, const gboolean value);
void xml_save_int(xmlNodePtr node, const gchar * descriptor, const gint num);
void xml_save_uint(xmlNodePtr node, const gchar * descriptor, const guint num);
void xml_save_location_and_size(xmlNodePtr node, const gchar * descriptor, 
				const guint64 location, const guint64 size);
xmlDocPtr xml_open_doc(gchar * filename, FILE * study_file, guint64 location, guint64 size, gchar ** perror_buf);

G_END_DECLS
#endif /* __XML_H__ */

