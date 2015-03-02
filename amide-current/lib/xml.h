/* xml.c - convience functions for working with xml files 
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

#ifndef __XML_H__
#define __XML_H__

/* header files that are always associated with this header file */
#include <libxml/tree.h>
#include <libxml/parser.h>
#include "realspace.h"



/* functions */

xmlNodePtr xml_get_node(xmlNodePtr nodes, gchar * descriptor);
gchar * xml_get_string(xmlNodePtr nodes, gchar * descriptor);
realpoint_t xml_get_realpoint(xmlNodePtr nodes, gchar * descriptor);
voxelpoint_t xml_get_voxelpoint(xmlNodePtr nodes, gchar * descriptor);
amide_time_t xml_get_time(xmlNodePtr nodes, gchar * descriptor);
amide_time_t * xml_get_times(xmlNodePtr nodes, gchar * descriptor, guint num_times);
amide_data_t xml_get_data(xmlNodePtr nodes, gchar * descriptor);
floatpoint_t xml_get_floatpoint(xmlNodePtr node, gchar * descriptor);
gint xml_get_int(xmlNodePtr nodes, gchar * descriptor);
realspace_t * xml_get_realspace(xmlNodePtr nodes, gchar * descriptor);
void xml_save_string(xmlNodePtr node, gchar * descriptor, gchar * string);
void xml_save_realpoint(xmlNodePtr node, gchar * descriptor, realpoint_t rp);
void xml_save_voxelpoint(xmlNodePtr node, gchar * descriptor, voxelpoint_t vp);
void xml_save_time(xmlNodePtr node, gchar * descriptor, amide_time_t num);
void xml_save_times(xmlNodePtr node, gchar * descriptor, amide_time_t * numbers, int num);
void xml_save_data(xmlNodePtr node, gchar * descriptor, amide_data_t num);
void xml_save_floatpoint(xmlNodePtr node, gchar * descriptor, floatpoint_t num);
void xml_save_int(xmlNodePtr node, gchar * descriptor, int num);
void xml_save_realspace(xmlNodePtr node, gchar * descriptor, const realspace_t * coord_frame);

/* legacy cruft */
voxelpoint_t xml_get_voxelpoint3D(xmlNodePtr nodes, gchar * descriptor);

#endif /* __XML_H__ */

