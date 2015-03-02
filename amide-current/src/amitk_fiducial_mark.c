/* amitk_fiducial_mark.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2003 Andy Loening
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
  LAST_SIGNAL
};


static void          fiducial_mark_class_init          (AmitkFiducialMarkClass *klass);
static void          fiducial_mark_init                (AmitkFiducialMark      *fiducial_mark);
static void          fiducial_mark_finalize            (GObject              *object);
static AmitkObject * fiducial_mark_copy                (const AmitkObject          *object);
static void          fiducial_mark_copy_in_place       (AmitkObject * dest_object, const AmitkObject * src_object);
static void          fiducial_mark_write_xml           (const AmitkObject * object, xmlNodePtr nodes);
static gchar *       fiducial_mark_read_xml            (AmitkObject * object, xmlNodePtr nodes, gchar *error_buf);

static AmitkObjectClass * parent_class;
/* static guint        fiducial_mark_signals[LAST_SIGNAL]; */


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

}


static void fiducial_mark_init (AmitkFiducialMark * fiducial_mark) {

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
 
  g_return_if_fail(AMITK_IS_FIDUCIAL_MARK(src_object));
  g_return_if_fail(AMITK_IS_FIDUCIAL_MARK(dest_object));

  AMITK_OBJECT_CLASS (parent_class)->object_copy_in_place (dest_object, src_object);
}


static void fiducial_mark_write_xml(const AmitkObject * object, xmlNodePtr nodes) {

  AMITK_OBJECT_CLASS(parent_class)->object_write_xml(object, nodes);

  return;
}

static gchar * fiducial_mark_read_xml(AmitkObject * object, xmlNodePtr nodes, gchar * error_buf ) {

  AmitkFiducialMark * mark;
  AmitkPoint point;

  mark = AMITK_FIDUCIAL_MARK(object);

  error_buf = AMITK_OBJECT_CLASS(parent_class)->object_read_xml(object, nodes, error_buf);

  /* legacy cruft.  the "point" option was eliminated in version 0.7.11, just 
     using the space's offset instead */
  if (xml_node_exists(nodes, "point")) {
    point = amitk_point_read_xml(nodes, "point", &error_buf);
    point = amitk_space_s2b(AMITK_SPACE(mark), point);
    amitk_space_set_offset(AMITK_SPACE(mark), point);
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



