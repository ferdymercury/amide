/* amitk_line_profile.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2003-2014 Andy Loening
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

#include "amide_config.h"

#include "amitk_line_profile.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"

enum {
  LINE_PROFILE_CHANGED,
  LAST_SIGNAL
};

static void line_profile_class_init          (AmitkLineProfileClass *klass);
static void line_profile_init                (AmitkLineProfile      *object);
static void line_profile_finalize            (GObject           *object);
static GObjectClass * parent_class;
static guint     line_profile_signals[LAST_SIGNAL];



GType amitk_line_profile_get_type(void) {

  static GType line_profile_type = 0;

  if (!line_profile_type)
    {
      static const GTypeInfo line_profile_info =
      {
	sizeof (AmitkLineProfileClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) line_profile_class_init,
	(GClassFinalizeFunc) NULL,
	NULL,		/* class_data */
	sizeof (AmitkLineProfile),
	0,			/* n_preallocs */
	(GInstanceInitFunc) line_profile_init,
	NULL /* value table */
      };
      
      line_profile_type = g_type_register_static (G_TYPE_OBJECT, "AmitkLineProfile", &line_profile_info, 0);
    }
  
  return line_profile_type;
}


static void line_profile_class_init (AmitkLineProfileClass * class) {

  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent(class);
  
  gobject_class->finalize = line_profile_finalize;

  line_profile_signals[LINE_PROFILE_CHANGED] =
    g_signal_new ("line_profile_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkLineProfileClass, line_profile_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);

}

static void line_profile_init (AmitkLineProfile * line_profile) {

  line_profile->view = AMITK_VIEW_TRANSVERSE;
  line_profile->angle=0.0;
  line_profile->visible=FALSE;

  line_profile->start_point = zero_point;
  line_profile->end_point = zero_point;

  return;
}


static void line_profile_finalize (GObject *object) {

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


AmitkLineProfile * amitk_line_profile_new (void) {

  AmitkLineProfile * line_profile;

  line_profile = g_object_new(amitk_line_profile_get_type(), NULL);

  return line_profile;
}




void amitk_line_profile_copy_in_place(AmitkLineProfile * dest_profile,
				      const AmitkLineProfile * src_profile) {

  g_return_if_fail(AMITK_IS_LINE_PROFILE(src_profile));
  g_return_if_fail(AMITK_IS_LINE_PROFILE(dest_profile));

  dest_profile->view = AMITK_LINE_PROFILE_VIEW(src_profile);
  dest_profile->angle = AMITK_LINE_PROFILE_ANGLE(src_profile);
  dest_profile->visible = AMITK_LINE_PROFILE_VISIBLE(src_profile);
  
  return;
}


void amitk_line_profile_set_view(AmitkLineProfile * line_profile, AmitkView view) {

  g_return_if_fail(AMITK_IS_LINE_PROFILE(line_profile));
  g_return_if_fail(view >= 0);
  g_return_if_fail(view < AMITK_VIEW_NUM);

  if (AMITK_LINE_PROFILE_VIEW(line_profile) != view) {
    line_profile->view = view;
    g_signal_emit(G_OBJECT(line_profile), line_profile_signals[LINE_PROFILE_CHANGED], 0);
  }

  return;
}

void amitk_line_profile_set_angle(AmitkLineProfile * line_profile, amide_real_t angle) {

  g_return_if_fail(AMITK_IS_LINE_PROFILE(line_profile));

  if (angle > M_PI) angle-=2.0*M_PI;
  if (angle < -M_PI) angle+=2.0*M_PI;

  if (!REAL_EQUAL(AMITK_LINE_PROFILE_ANGLE(line_profile), angle)) {
    line_profile->angle = angle;
    g_signal_emit(G_OBJECT(line_profile), line_profile_signals[LINE_PROFILE_CHANGED], 0);
  }

  return;
}

void amitk_line_profile_set_visible(AmitkLineProfile * line_profile, gboolean visible) {

  g_return_if_fail(AMITK_IS_LINE_PROFILE(line_profile));

  if (AMITK_LINE_PROFILE_VISIBLE(line_profile) != visible) {
    line_profile->visible = visible;
    g_signal_emit(G_OBJECT(line_profile), line_profile_signals[LINE_PROFILE_CHANGED], 0);
  }

  return;
}



void amitk_line_profile_set_start_point(AmitkLineProfile * line_profile,
					const AmitkPoint start_point) {

  g_return_if_fail(AMITK_IS_LINE_PROFILE(line_profile));

  if (!POINT_EQUAL(AMITK_LINE_PROFILE_START_POINT(line_profile), start_point)) {
    line_profile->start_point = start_point;
    g_signal_emit(G_OBJECT(line_profile), line_profile_signals[LINE_PROFILE_CHANGED], 0);
  }

  return;
}


void amitk_line_profile_set_end_point(AmitkLineProfile * line_profile,
				      const AmitkPoint end_point) {

  g_return_if_fail(AMITK_IS_LINE_PROFILE(line_profile));

  if (!POINT_EQUAL(AMITK_LINE_PROFILE_END_POINT(line_profile), end_point)) {
    line_profile->end_point = end_point;
    g_signal_emit(G_OBJECT(line_profile), line_profile_signals[LINE_PROFILE_CHANGED], 0);
  }

  return;
}

