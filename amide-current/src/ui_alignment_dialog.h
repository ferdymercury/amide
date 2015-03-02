/* ui_alignment_dialog.h
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


/* typedefs */

typedef enum {
  INTRO_PAGE, 
  VOLUME_PAGE, 
  ALIGN_PTS_PAGE, 
  CONCLUSION_PAGE, 
  NO_ALIGN_PTS_PAGE,
  NUM_PAGES
} which_page_t;

/* data structures */
typedef struct ui_alignment_t {
  GtkWidget * dialog;
  GtkWidget * druid;
  GtkWidget * pages[NUM_PAGES];
  GtkWidget * clist_volume_moving;
  GtkWidget * clist_volume_fixed;
  GtkWidget * clist_points;
  GdkImlibImage * logo;

  volume_list_t * volumes;
  volume_t * volume_moving;
  volume_t * volume_fixed;
  align_pts_t * align_pts;
  realspace_t coord_frame; /* the new coord frame for the moving volume */

  guint reference_count;
} ui_alignment_t;

/* external functions */
ui_alignment_t * ui_alignment_free(ui_alignment_t * ui_alignment);
void ui_alignment_dialog_create(ui_study_t * ui_study);







