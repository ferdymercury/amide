/* align_pt.h
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

#ifndef __ALIGN_PT_H__
#define __ALIGN_PT_H__

/* header files that are always needed with this file */
#include "xml.h"


/* an alignment point */
typedef struct align_pt_t {
  gchar * name;
  realpoint_t point; /* wrt the parent volume's coord frame */

  /* parameters calculated at run time */
  guint ref_count;
} align_pt_t;

/* a list of alignment points */
typedef struct _align_pts_t align_pts_t;
struct _align_pts_t {
  align_pt_t * align_pt;
  guint ref_count;
  align_pts_t * next;
};


/* functions */
align_pt_t * align_pt_unref(align_pt_t * align_pt);
align_pt_t * align_pt_init(void);
align_pt_t * align_pt_ref(align_pt_t * align_pt);
gchar * align_pt_write_xml(align_pt_t * pt, gchar * study_directory, 
			   gchar *volume_name);
align_pt_t * align_pt_load_xml(gchar * pt_xml_filename, const gchar * study_directory);
void align_pt_set_name(align_pt_t * align_pt, gchar * new_name);
align_pt_t * align_pt_copy(align_pt_t * src_align_pt);
align_pts_t * align_pts_unref(align_pts_t * alignt_pts);
align_pts_t * align_pts_init(align_pt_t * align_pt);
align_pts_t * align_pts_add_ref(align_pts_t * align_pts);
void align_pts_write_xml(align_pts_t * pts, xmlNodePtr node_list, 
			 gchar * study_directory, gchar * volume_name);
align_pts_t * align_pts_load_xml(xmlNodePtr node_list, const gchar * study_directory);
align_pt_t * align_pts_find_pt(align_pts_t * align_pts, align_pt_t * find_pt);
gboolean align_pts_includes_pt(align_pts_t * align_pts, align_pt_t * includes_pt);
align_pt_t * align_pts_find_pt_by_name(align_pts_t * align_pts, gchar * name);
gboolean align_pts_includes_pt_by_name(align_pts_t * align_pts, gchar * name);
align_pts_t * align_pts_add_pt(align_pts_t * align_pts, align_pt_t * new_pt);
align_pts_t * align_pts_add_pt_first(align_pts_t * align_pts, align_pt_t * new_pt);
align_pts_t * align_pts_remove_pt(align_pts_t * align_pts, align_pt_t * pt);
guint align_pts_count(align_pts_t * pts);
guint align_pts_count_pairs_by_name(align_pts_t * pts1, align_pts_t * pts2);

#endif /* __ALIGN_PT_H__ */
