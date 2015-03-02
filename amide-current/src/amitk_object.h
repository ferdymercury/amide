/* amitk_object.h
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

#ifndef __AMITK_OBJECT_H__
#define __AMITK_OBJECT_H__


#include "amitk_space.h"

G_BEGIN_DECLS

#define	AMITK_TYPE_OBJECT		(amitk_object_get_type ())
#define AMITK_OBJECT(object)		(G_TYPE_CHECK_INSTANCE_CAST ((object), AMITK_TYPE_OBJECT, AmitkObject))
#define AMITK_OBJECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), AMITK_TYPE_OBJECT, AmitkObjectClass))
#define AMITK_IS_OBJECT(object)		(G_TYPE_CHECK_INSTANCE_TYPE ((object), AMITK_TYPE_OBJECT))
#define AMITK_IS_OBJECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), AMITK_TYPE_OBJECT))
#define	AMITK_OBJECT_GET_CLASS(object)	(G_TYPE_CHECK_GET_CLASS ((object), AMITK_TYPE_OBJECT, AmitkObjectClass))

#define AMITK_OBJECT_CHILDREN(object)   (AMITK_OBJECT(object)->children)
#define AMITK_OBJECT_PARENT(object)     (AMITK_OBJECT(object)->parent)
#define AMITK_OBJECT_NAME(object)       ((const gchar *) (AMITK_OBJECT(object)->name))

typedef struct _AmitkObjectClass AmitkObjectClass;
typedef struct _AmitkObject AmitkObject;

typedef enum {
  AMITK_OBJECT_TYPE_STUDY,
  AMITK_OBJECT_TYPE_DATA_SET,
  AMITK_OBJECT_TYPE_FIDUCIAL_MARK,
  AMITK_OBJECT_TYPE_ROI,
  AMITK_OBJECT_TYPE_VOLUME,
  AMITK_OBJECT_TYPE_NUM
} AmitkObjectType;


/* note that the numbered entries need to correspond to the enteries in AmitkViewMode */
typedef enum {
  AMITK_SELECTION_SELECTED_0,
  AMITK_SELECTION_SELECTED_1,
  AMITK_SELECTION_SELECTED_2,
  AMITK_SELECTION_NUM,
  AMITK_SELECTION_ANY,
  AMITK_SELECTION_ALL,
} AmitkSelection;


struct _AmitkObject
{
  AmitkSpace space;

  gchar * name;

  gboolean selected[AMITK_SELECTION_NUM];
  AmitkObject * parent;
  GList * children;

  GObject * dialog; 
};

struct _AmitkObjectClass
{
  AmitkSpaceClass space_class;

  void (* object_name_changed)            (AmitkObject * object);
  void (* object_selection_changed)       (AmitkObject * object);
  void (* object_child_selection_changed) (AmitkObject * object);
  AmitkObject * (* object_copy)           (const AmitkObject * object);
  void (* object_copy_in_place)           (AmitkObject * dest_object, const AmitkObject * src_object);
  void (* object_write_xml)               (const AmitkObject * object, xmlNodePtr nodes, FILE * study_file);
  gchar * (* object_read_xml)             (AmitkObject * object, xmlNodePtr nodes, FILE * study_file, gchar * error_buf);
  void (* object_add_child)               (AmitkObject * object, AmitkObject * child);
  void (* object_remove_child)            (AmitkObject * object, AmitkObject * child);
       
};



/* Application-level methods */

GType	        amitk_object_get_type	             (void);
AmitkObject *   amitk_object_new                     (void);
void            amitk_object_write_xml               (AmitkObject * object,
						      FILE * study_file,
						      gchar ** output_filename,
						      guint64 * location,
						      guint64 * size);
AmitkObject *   amitk_object_read_xml                (gchar * xml_filename, 
						      FILE * study_file, 
						      guint64 location,
						      guint64 size, 
						      gchar ** perror_buf);
AmitkObject *   amitk_object_copy                    (const AmitkObject * object);
void            amitk_object_copy_in_place           (AmitkObject * dest_object,
						      const AmitkObject * src_object);
void            amitk_object_set_name                (AmitkObject * object, 
						      const gchar * new_name);
gboolean        amitk_object_get_selected            (const AmitkObject * object,
						      const AmitkSelection which_selection);
void            amitk_object_set_selected            (AmitkObject * object, 
						      const gboolean selection, 
						      const AmitkSelection which_selection);
#define         amitk_object_select(obj, which)      (amitk_object_set_selected((obj), (TRUE), (which)))
#define         amitk_object_unselect(obj, which)    (amitk_object_set_selected((obj), (FALSE), (which)))
void            amitk_object_set_parent              (AmitkObject * object,
						      AmitkObject * parent);
void            amitk_object_add_child               (AmitkObject * object,
						      AmitkObject * child);
void            amitk_object_add_children            (AmitkObject * object,
						      GList * children);
gboolean        amitk_object_remove_child            (AmitkObject * object,
						      AmitkObject * child);
gboolean        amitk_object_remove_children         (AmitkObject * object, 
						      GList * children);
gboolean amitk_object_compare_object_type            (AmitkObject * object, 
						      AmitkObjectType type);
AmitkObject *   amitk_object_get_parent_of_type      (AmitkObject * object,
						      const AmitkObjectType type);
GList *         amitk_object_get_children_of_type    (AmitkObject * object,
						      const AmitkObjectType type,
						      const gboolean recurse);
gboolean        amitk_object_selected_children       (AmitkObject * object, 
						      const AmitkSelection which_selection,
						      gboolean recurse);
GList *         amitk_object_get_selected_children   (AmitkObject * object,
						      const AmitkSelection which_selection,
						      const gboolean recurse);
GList *         amitk_object_get_selected_children_of_type    (AmitkObject * object,
							       const AmitkObjectType type,
							       const AmitkSelection which_selection,
							       const gboolean recurse);
gpointer        amitk_object_ref                     (gpointer object);
gpointer        amitk_object_unref                   (gpointer object);
GList *         amitk_objects_ref                    (GList * objects);
GList *         amitk_objects_unref                  (GList * objects);
gint            amitk_objects_count                  (GList * objects);
AmitkObject *   amitk_objects_find_object_by_name    (GList * objects, const gchar * name);
gint            amitk_objects_count_pairs_by_name    (GList * objects1, GList * objects2);
GList *         amitk_objects_get_of_type            (GList * objects,
						      const AmitkObjectType type,
						      const gboolean recurse);
gboolean        amitk_objects_has_type               (GList * objects, 
						      const AmitkObjectType type,
						      const gboolean recurse);
void            amitk_objects_write_xml              (GList * objects, 
						      xmlNodePtr node_list,
						      FILE * study_file);
GList *         amitk_objects_read_xml               (xmlNodePtr node_list,
						      FILE * study_file,
						      gchar **perror_buf);

const gchar *   amitk_object_type_get_name           (const AmitkObjectType type);
const gchar *   amitk_selection_get_name             (const AmitkSelection type);

extern gchar * amide_data_file_xml_tag;
extern gchar * amide_data_file_xml_start_tag;
extern gchar * amide_data_file_xml_end_tag;

G_END_DECLS

#endif /* __AMITK_OBJECT_H__ */
