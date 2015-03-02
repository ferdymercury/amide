/* amitk_fiducial_mark.c
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

#include "amide_config.h"

#include "amitk_fiducial_mark.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"

enum {
  FIDUCIAL_MARK_CHANGED,
  LAST_SIGNAL
};


static void          fiducial_mark_class_init          (AmitkFiducialMarkClass *klass);
static void          fiducial_mark_init                (AmitkFiducialMark      *fiducial_mark);
static void          fiducial_mark_finalize            (GObject              *object);
static AmitkObject * fiducial_mark_copy                (const AmitkObject          *object);
static void          fiducial_mark_copy_in_place       (AmitkObject * dest_object, const AmitkObject * src_object);
static void          fiducial_mark_write_xml           (const AmitkObject   *object, 
							xmlNodePtr           nodes,
							FILE                *study_file);
static gchar *       fiducial_mark_read_xml            (AmitkObject         *object, 
							xmlNodePtr           nodes, 
							FILE                *study_file,
							gchar               *error_buf);

static AmitkObjectClass * parent_class;
static guint        fiducial_mark_signals[LAST_SIGNAL];


GType amitk_fiducial_mark_get_type(void) {

  static GType fiducial_mark_type = 0;

  if (!fiducial_mark_type)
    {
      static const GTypeInfo fiducial_mark_info =
      {
	sizeof (AmitkFiducialMarkClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) fiducial_mark_class_init,
	(GClassFinalizeFunc) NULL,
	NULL,		/* class_data */
	sizeof (AmitkFiducialMark),
	0,			/* n_preallocs */
	(GInstanceInitFunc) fiducial_mark_init,
	NULL /* value table */
      };
      
      fiducial_mark_type = g_type_register_static (AMITK_TYPE_OBJECT, "AmitkFiducialMark", &fiducial_mark_info, 0);
    }

  return fiducial_mark_type;
}


static void fiducial_mark_class_init (AmitkFiducialMarkClass * class) {

  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  AmitkObjectClass * object_class = AMITK_OBJECT_CLASS(class);

  parent_class = g_type_class_peek_parent(class);

  object_class->object_copy = fiducial_mark_copy;
  object_class->object_copy_in_place = fiducial_mark_copy_in_place;
  object_class->object_write_xml = fiducial_mark_write_xml;
  object_class->object_read_xml = fiducial_mark_read_xml;

  gobject_class->finalize = fiducial_mark_finalize;

  fiducial_mark_signals[FIDUCIAL_MARK_CHANGED] =
    g_signal_new ("fiducial_mark_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkFiducialMarkClass, fiducial_mark_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);

}


static void fiducial_mark_init (AmitkFiducialMark * fiducial_mark) {

  fiducial_mark->specify_color = FALSE;
  fiducial_mark->color = amitk_color_table_uint32_to_rgba(AMITK_OBJECT_DEFAULT_COLOR);

  return; 
}


static void fiducial_mark_finalize (GObject * object) {
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static AmitkObject * fiducial_mark_copy (const AmitkObject * object ) {

  AmitkFiducialMark * copy;

  g_return_val_if_fail(AMITK_IS_FIDUCIAL_MARK(object), NULL);

  copy = amitk_fiducial_mark_new();
  amitk_object_copy_in_place(AMITK_OBJECT(copy), object);

  return AMITK_OBJECT(copy);
}

static void fiducial_mark_copy_in_place(AmitkObject * dest_object, const AmitkObject * src_object) {

  AmitkFiducialMark * src_fm;
  AmitkFiducialMark * dest_fm;
 
  g_return_if_fail(AMITK_IS_FIDUCIAL_MARK(src_object));
  g_return_if_fail(AMITK_IS_FIDUCIAL_MARK(dest_object));

  src_fm = AMITK_FIDUCIAL_MARK(src_object);
  dest_fm = AMITK_FIDUCIAL_MARK(dest_object);

  dest_fm->specify_color = AMITK_FIDUCIAL_MARK_SPECIFY_COLOR(src_fm);
  dest_fm->color = AMITK_FIDUCIAL_MARK_COLOR(src_fm);

  AMITK_OBJECT_CLASS (parent_class)->object_copy_in_place (dest_object, src_object);
}


static void fiducial_mark_write_xml(const AmitkObject * object, xmlNodePtr nodes, FILE *study_file) {

  AmitkFiducialMark * fm;

  AMITK_OBJECT_CLASS(parent_class)->object_write_xml(object, nodes, study_file);

  fm = AMITK_FIDUCIAL_MARK(object);

  xml_save_boolean(nodes, "specify_color", AMITK_FIDUCIAL_MARK_SPECIFY_COLOR(fm));
  xml_save_uint(nodes, "color", amitk_color_table_rgba_to_uint32(AMITK_FIDUCIAL_MARK_COLOR(fm)));

  return;
}

static gchar * fiducial_mark_read_xml(AmitkObject * object, xmlNodePtr nodes, 
				      FILE * study_file, gchar * error_buf ) {

  AmitkFiducialMark * fm;
  AmitkPoint point;

  fm = AMITK_FIDUCIAL_MARK(object);

  error_buf = AMITK_OBJECT_CLASS(parent_class)->object_read_xml(object, nodes, study_file, error_buf);

  amitk_fiducial_mark_set_specify_color(fm, xml_get_boolean_with_default(nodes, "specify_color",
									 AMITK_FIDUCIAL_MARK_SPECIFY_COLOR(fm)));
  amitk_fiducial_mark_set_color(fm, amitk_color_table_uint32_to_rgba(xml_get_uint_with_default(nodes, "color", AMITK_OBJECT_DEFAULT_COLOR)));

  /* legacy cruft.  the "point" option was eliminated in version 0.7.11, just 
     using the space's offset instead */
  if (xml_node_exists(nodes, "point")) {
    point = amitk_point_read_xml(nodes, "point", &error_buf);
    point = amitk_space_s2b(AMITK_SPACE(fm), point);
    amitk_space_set_offset(AMITK_SPACE(fm), point);
  }

  return error_buf;
}


AmitkFiducialMark * amitk_fiducial_mark_new (void) {

  AmitkFiducialMark * fiducial_mark;

  fiducial_mark = g_object_new(amitk_fiducial_mark_get_type(), NULL);

  return fiducial_mark;
}



/* new point should be in the base coordinate space */
void amitk_fiducial_mark_set(AmitkFiducialMark * fiducial_mark, AmitkPoint new_point) {

  g_return_if_fail(AMITK_IS_FIDUCIAL_MARK(fiducial_mark));

  if (!POINT_EQUAL(AMITK_FIDUCIAL_MARK_GET(fiducial_mark), new_point)) {
    amitk_space_set_offset(AMITK_SPACE(fiducial_mark), new_point);
  }
  return;
}


/* whether we want to use the specified color or have the program choose a decent color */
void amitk_fiducial_mark_set_specify_color(AmitkFiducialMark * fiducial_mark, gboolean specify_color) {

  g_return_if_fail(AMITK_IS_FIDUCIAL_MARK(fiducial_mark));

  if (specify_color != AMITK_FIDUCIAL_MARK_SPECIFY_COLOR(fiducial_mark)) {
    fiducial_mark->specify_color = specify_color;
    g_signal_emit(G_OBJECT(fiducial_mark), fiducial_mark_signals[FIDUCIAL_MARK_CHANGED], 0);
  }
  return;
}

/* color to draw the fiducial_mark in, if we choose specify_color */
void amitk_fiducial_mark_set_color(AmitkFiducialMark * fiducial_mark, rgba_t new_color) {

  rgba_t old_color;

  g_return_if_fail(AMITK_IS_FIDUCIAL_MARK(fiducial_mark));
  old_color = AMITK_FIDUCIAL_MARK_COLOR(fiducial_mark);

  if ((old_color.r != new_color.r) ||
      (old_color.g != new_color.g) ||
      (old_color.b != new_color.b) ||
      (old_color.a != new_color.a)) {
    fiducial_mark->color = new_color;
    g_signal_emit(G_OBJECT(fiducial_mark), fiducial_mark_signals[FIDUCIAL_MARK_CHANGED], 0);
  }
  return;
}




