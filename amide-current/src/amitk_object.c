/* amitk_object.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2002 Andy Loening
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

#include "amide_config.h"

#include <sys/stat.h>
#include <string.h>

#include "amitk_object.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"
#include "amitk_study.h"



enum {
  OBJECT_NAME_CHANGED,
  OBJECT_SELECTION_CHANGED,
  OBJECT_COPY,
  OBJECT_COPY_IN_PLACE,
  OBJECT_WRITE_XML,
  OBJECT_READ_XML,
  OBJECT_ADD_CHILD,
  OBJECT_REMOVE_CHILD,
  LAST_SIGNAL
};

static void          object_class_init          (AmitkObjectClass *klass);
static void          object_init                (AmitkObject      *object);
static void          object_finalize            (GObject          *object);
static void          object_rotate_on_vector    (AmitkSpace       *space,
						 AmitkPoint      *vector,
						 gdouble          theta,
						 AmitkPoint      *rotation_point);
static void          object_invert_axis         (AmitkSpace       *space, 
						 AmitkAxis        which_axis,
						 AmitkPoint       * center_of_inversion);
static void          object_shift_offset        (AmitkSpace       *space, 
						 AmitkPoint      *shift);
static void          object_transform           (AmitkSpace * space, 
						 AmitkSpace * transform_space);
static void          object_transform_axes      (AmitkSpace * space, 
						 AmitkAxes    transform_axes,
						 AmitkPoint * center_of_rotation);
static AmitkObject * object_copy                (const AmitkObject * object);
static void          object_copy_in_place       (AmitkObject * dest_object, const AmitkObject * src_object);
static void          object_write_xml           (const AmitkObject * object, xmlNodePtr nodes);
static void          object_read_xml            (AmitkObject * object, xmlNodePtr nodes);
static void          object_add_child           (AmitkObject * object, AmitkObject * child);
static void          object_remove_child        (AmitkObject * object, AmitkObject * child);

static AmitkSpaceClass * parent_class;
static guint        object_signals[LAST_SIGNAL];



GType amitk_object_get_type(void) {

  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
	sizeof (AmitkObjectClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) object_class_init,
	(GClassFinalizeFunc) NULL,
	NULL,		/* class_data */
	sizeof (AmitkObject),
	0,			/* n_preallocs */
	(GInstanceInitFunc) object_init,
	NULL /* value table */
      };
      
      object_type = g_type_register_static (AMITK_TYPE_SPACE, "AmitkObject", &object_info, 0);
    }

  return object_type;
}


static void object_class_init (AmitkObjectClass * class) {

  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  AmitkSpaceClass * space_class = AMITK_SPACE_CLASS(class);

  parent_class = g_type_class_peek_parent(class);

  gobject_class->finalize = object_finalize;

  space_class->space_shift = object_shift_offset;
  space_class->space_rotate = object_rotate_on_vector;
  space_class->space_invert = object_invert_axis;
  space_class->space_transform = object_transform;
  space_class->space_transform_axes = object_transform_axes;

  class->object_name_changed = NULL;
  class->object_selection_changed = NULL;
  class->object_copy = object_copy;
  class->object_copy_in_place = object_copy_in_place;
  
  class->object_write_xml = object_write_xml;
  class->object_read_xml = object_read_xml;
  class->object_add_child = object_add_child;
  class->object_remove_child = object_remove_child;

  object_signals[OBJECT_NAME_CHANGED] =
    g_signal_new ("object_name_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkObjectClass, object_name_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE,0);
  object_signals[OBJECT_SELECTION_CHANGED] =
    g_signal_new ("object_selection_changed",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkObjectClass, object_selection_changed),
		  NULL, NULL, amitk_marshal_NONE__NONE,
		  G_TYPE_NONE, 0);
  object_signals[OBJECT_COPY] =
    g_signal_new ("object_copy",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkObjectClass, object_copy),
		  NULL, NULL, amitk_marshal_OBJECT__NONE,
		  AMITK_TYPE_OBJECT,0);
  object_signals[OBJECT_COPY_IN_PLACE] =
    g_signal_new ("object_copy_in_place",
		  G_TYPE_FROM_CLASS(class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET(AmitkObjectClass, object_copy_in_place),
		  NULL, NULL, amitk_marshal_NONE__OBJECT,
		  G_TYPE_NONE, 1, AMITK_TYPE_OBJECT);
  object_signals[OBJECT_WRITE_XML] =
    g_signal_new ("object_write_xml",
  		  G_TYPE_FROM_CLASS(class),
  		  G_SIGNAL_RUN_LAST,
  		  G_STRUCT_OFFSET(AmitkObjectClass, object_write_xml),
  		  NULL, NULL, amitk_marshal_NONE__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);
  object_signals[OBJECT_READ_XML] =
    g_signal_new ("object_read_xml",
  		  G_TYPE_FROM_CLASS(class),
  		  G_SIGNAL_RUN_LAST,
  		  G_STRUCT_OFFSET(AmitkObjectClass, object_read_xml),
  		  NULL, NULL, amitk_marshal_NONE__POINTER,
		  G_TYPE_NONE, 1,
		  G_TYPE_POINTER);
  object_signals[OBJECT_ADD_CHILD] =
    g_signal_new ("object_add_child",
  		  G_TYPE_FROM_CLASS(class),
  		  G_SIGNAL_RUN_FIRST,
  		  G_STRUCT_OFFSET(AmitkObjectClass, object_add_child),
  		  NULL, NULL, amitk_marshal_NONE__OBJECT,
		  G_TYPE_NONE, 1,
		  AMITK_TYPE_OBJECT);
  object_signals[OBJECT_REMOVE_CHILD] =
    g_signal_new ("object_remove_child",
  		  G_TYPE_FROM_CLASS(class),
  		  G_SIGNAL_RUN_FIRST,
  		  G_STRUCT_OFFSET(AmitkObjectClass, object_remove_child),
  		  NULL, NULL, amitk_marshal_NONE__OBJECT,
		  G_TYPE_NONE, 1,
		  AMITK_TYPE_OBJECT);
}


static void object_init (AmitkObject * object) {

  AmitkSelection i_selection;

  object->name = NULL;
  object->parent = NULL;
  object->children = NULL;
  object->dialog = NULL;
  for (i_selection = 0; i_selection < AMITK_SELECTION_NUM; i_selection++)
    object->selected[i_selection] = FALSE;

}


static void object_finalize (GObject *object)
{
  AmitkObject * amitk_object = AMITK_OBJECT(object);
  AmitkObject * child;

  /* propogate signal to children */
  /* can't call amitk_object_remove_children, as this leads to a g_signal_emit, which we can't
     do on amitk_object as ref count is now zero */
  while (amitk_object->children != NULL) {
    child = amitk_object->children->data;
    child->parent = NULL;
    amitk_object->children = g_list_remove(amitk_object->children, child);
    g_object_unref(child);
  }
  amitk_object_set_parent(amitk_object, NULL);

  if (amitk_object->name != NULL) {
#ifdef AMIDE_DEBUG
    if (!AMITK_IS_DATA_SET(amitk_object))
      g_print("\tfreeing %s\n", AMITK_OBJECT_NAME(amitk_object));
#endif
    g_free(amitk_object->name);
    amitk_object->name = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void object_rotate_on_vector(AmitkSpace * space,
				    AmitkPoint * vector,
				    gdouble theta,
				    AmitkPoint * rotation_point) {
  AmitkObject * object;
  GList * children;

  g_return_if_fail(AMITK_IS_OBJECT(space));
  object = AMITK_OBJECT(space);

  /* rotate children */
  children = object->children;
  while (children != NULL) {
    amitk_space_rotate_on_vector(children->data, *vector, theta, *rotation_point);
    children = children->next;
  }

  AMITK_SPACE_CLASS(parent_class)->space_rotate (space, vector, theta, rotation_point);
  return;
}

static void object_invert_axis (AmitkSpace * space, AmitkAxis which_axis,
				AmitkPoint * center_of_inversion) {

  AmitkObject * object;
  GList * children;

  g_return_if_fail(AMITK_IS_OBJECT(space));
  object = AMITK_OBJECT(space);

  /* invert children */
  children = object->children;
  while (children != NULL) {
    amitk_space_invert_axis(children->data, which_axis, *center_of_inversion);
    children = children->next;
  }

  AMITK_SPACE_CLASS(parent_class)->space_invert (space, which_axis, center_of_inversion);
}


static void object_shift_offset(AmitkSpace * space, AmitkPoint * shift) {

  AmitkObject * object;
  GList * children;

  g_return_if_fail(AMITK_IS_OBJECT(space));
  object = AMITK_OBJECT(space);

  children = object->children;
  while (children != NULL) {
    amitk_space_shift_offset(children->data, *shift);
    children = children->next;
  }

  AMITK_SPACE_CLASS(parent_class)->space_shift (space, shift);
}



static void object_transform(AmitkSpace * space, AmitkSpace * transform_space) {

  AmitkObject * object;
  GList * children;

  g_return_if_fail(AMITK_IS_OBJECT(space));
  object = AMITK_OBJECT(space);

  children = object->children;
  while (children != NULL) {
    amitk_space_transform(children->data, transform_space);
    children = children->next;
  }

  AMITK_SPACE_CLASS(parent_class)->space_transform (space, transform_space);
}

static void object_transform_axes(AmitkSpace * space, AmitkAxes   transform_axes, 
				  AmitkPoint * center_of_rotation) {

  AmitkObject * object;
  GList * children;

  g_return_if_fail(AMITK_IS_OBJECT(space));
  object = AMITK_OBJECT(space);

  children = object->children;
  while (children != NULL) {
    amitk_space_transform_axes(children->data, transform_axes, *center_of_rotation);
    children = children->next;
  }

  AMITK_SPACE_CLASS(parent_class)->space_transform_axes (space, transform_axes, center_of_rotation);
}

static gboolean object_change_selection(AmitkObject * object, const gboolean state, const gint which) {

  return TRUE; /* returns if the state was set, higher level objects may not change state */
}


static AmitkObject * object_copy (const AmitkObject * object) {

  AmitkObject * copy;

  copy = amitk_object_new();

  amitk_object_copy_in_place(copy, object);

  return copy;
}

void object_copy_in_place(AmitkObject * dest_object, const AmitkObject * src_object) {
 
  amitk_space_copy_in_place(AMITK_SPACE(dest_object), AMITK_SPACE(src_object));
  amitk_object_set_name(dest_object, AMITK_OBJECT_NAME(src_object));

  /* don't copy object->dialog, as that's the dialog for modifying the old object */
  
  return;
}

static void object_write_xml (const AmitkObject * object, xmlNodePtr nodes) {

  xmlNodePtr children_nodes;
  AmitkSelection i_selection;

  amitk_space_write_xml(nodes, "coordinate_space", AMITK_SPACE(object));

  children_nodes = xmlNewChild(nodes, NULL, "children", NULL);
  amitk_objects_write_xml(AMITK_OBJECT_CHILDREN(object), children_nodes);

  for (i_selection = 0; i_selection < AMITK_SELECTION_NUM; i_selection++)
    xml_save_boolean(nodes, amitk_selection_get_name(i_selection),
		     object->selected[i_selection]);

  return;
}

static void object_read_xml (AmitkObject * object, xmlNodePtr nodes) {

  AmitkSpace * space;
  xmlNodePtr children_nodes;
  GList * children;
  AmitkSelection i_selection;

  space = amitk_space_read_xml(nodes, "coordinate_space");
  amitk_space_copy_in_place(AMITK_SPACE(object), space);
  g_object_unref(space);

  children_nodes = xml_get_node(nodes, "children");
  children = amitk_objects_read_xml(children_nodes->children);
  amitk_object_add_children(object, children);
  children = amitk_objects_unref(children);

  for (i_selection = 0; i_selection < AMITK_SELECTION_NUM; i_selection++)
    object->selected[i_selection] = xml_get_boolean(nodes, amitk_selection_get_name(i_selection));

  return;
}

static void object_add_child(AmitkObject * object, AmitkObject * child) {

  g_object_ref(child);
  object->children = g_list_append(object->children, child);
  child->parent = object;

  return;
}

static void object_remove_child(AmitkObject * object, AmitkObject * child) {

  child->parent = NULL;
  object->children = g_list_remove(object->children, child);
  g_object_unref(child);

  return;
}


AmitkObject * amitk_object_new (void) {

  AmitkObject * object;
  object = g_object_new(amitk_object_get_type(), NULL);

  return object;
}

gchar * amitk_object_write_xml(AmitkObject * object) {

  gchar * xml_filename;
  const gchar * object_name;
  guint count;
  struct stat file_info;
  xmlDocPtr doc;
  xmlNodePtr nodes;

  AMITK_IS_OBJECT(object);

  /* make a guess as to our filename */
  if (AMITK_IS_DATA_SET(object))
    object_name = amitk_object_type_get_name(AMITK_OBJECT_TYPE_DATA_SET);
  else if (AMITK_IS_STUDY(object))
    object_name = amitk_object_type_get_name(AMITK_OBJECT_TYPE_STUDY);
  else if (AMITK_IS_FIDUCIAL_MARK(object))
    object_name = amitk_object_type_get_name(AMITK_OBJECT_TYPE_FIDUCIAL_MARK);
  else if (AMITK_IS_ROI(object))
    object_name = amitk_object_type_get_name(AMITK_OBJECT_TYPE_ROI);
  else if (AMITK_IS_VOLUME(object))
    object_name = amitk_object_type_get_name(AMITK_OBJECT_TYPE_VOLUME);
  else
    g_return_val_if_reached(NULL);

  count = 1;
  xml_filename = g_strdup_printf("%s_%s.xml", object_name, AMITK_OBJECT_NAME(object));

  /* see if this file already exists */
  while (stat(xml_filename, &file_info) == 0) {
    g_free(xml_filename);
    count++;
    xml_filename = g_strdup_printf("%s_%s_%d.xml", object_name, AMITK_OBJECT_NAME(object), count);
  }

  /* and we now have a unique filename */
#ifdef AMIDE_DEBUG
  g_print("\t- saving object %s in %s\n",AMITK_OBJECT_NAME(object), xml_filename);
#endif

  /* write the roi xml file */
  doc = xmlNewDoc("1.0");

  doc->children = xmlNewDocNode(doc, NULL, "amide_data_file_version", AMIDE_FILE_VERSION);

  nodes = xmlNewChild(doc->children, NULL, object_name, AMITK_OBJECT_NAME(object));
  g_signal_emit(G_OBJECT(object), object_signals[OBJECT_WRITE_XML], 0, nodes);

  /* and save */
  xmlSaveFile(xml_filename, doc);

  /* and we're done */
  xmlFreeDoc(doc);

  return xml_filename;
}

AmitkObject * amitk_object_read_xml(gchar * xml_filename) {

  AmitkObject * new_object;
  xmlDocPtr doc;
  xmlNodePtr doc_nodes;
  xmlNodePtr type_node;
  xmlNodePtr version_node;
  xmlNodePtr object_node;
  gchar * version;
  gchar * name;
  AmitkObjectType i_type, type;

  /* parse the xml file */
  if ((doc = xmlParseFile(xml_filename)) == NULL) {
    g_warning("Couldn't Parse AMIDE xml file:\t%s", xml_filename);
    return NULL;
  }

  /* get the root of our document */
  if ((doc_nodes = xmlDocGetRootElement(doc)) == NULL) {
    g_warning("AMIDE xml file doesn't appear to have a root:\n\t%s", xml_filename);
    return NULL;
  }
  version_node = doc_nodes->children;

  /* look at the file version */
  version = xml_get_string(version_node, "text");
  g_return_val_if_fail(version != NULL, NULL);

  
  /* figure out what type of object is in here */
  i_type = 0;
  type = AMITK_OBJECT_TYPE_NUM;
  while ((type == AMITK_OBJECT_TYPE_NUM) && (i_type < AMITK_OBJECT_TYPE_NUM)) {
    type_node = xml_get_node(version_node, amitk_object_type_get_name(i_type));
    if (type_node != NULL) 
      type = i_type;
    i_type++;
  }


  switch(type) {
  case AMITK_OBJECT_TYPE_STUDY:
    new_object = AMITK_OBJECT(amitk_study_new());
    break;
  case AMITK_OBJECT_TYPE_FIDUCIAL_MARK:
    new_object = AMITK_OBJECT(amitk_fiducial_mark_new());
    break;
  case AMITK_OBJECT_TYPE_DATA_SET:
    new_object = AMITK_OBJECT(amitk_data_set_new());
    break;
  case AMITK_OBJECT_TYPE_ROI:
    new_object = AMITK_OBJECT(amitk_roi_new(0));
    break;
  case AMITK_OBJECT_TYPE_VOLUME:
    new_object = AMITK_OBJECT(amitk_volume_new());
    break;
  default:
    g_return_val_if_reached(NULL);
    break;
  }

  object_node = type_node->children;
  name = xml_get_string(object_node, "text");
  amitk_object_set_name(new_object, name);
  g_free(name);

#ifdef AMIDE_DEBUG
  g_print("\treading: %s\ttype: %s\tfile version: %s\n", 
	  AMITK_OBJECT_NAME(new_object),
	  amitk_object_type_get_name(type), version);
#endif

  g_signal_emit(G_OBJECT(new_object), object_signals[OBJECT_READ_XML], 0, object_node);

  g_free(version);

  return new_object;
}

AmitkObject * amitk_object_copy(const AmitkObject * object) {

  AmitkObject * new_object;

  g_return_val_if_fail(AMITK_IS_OBJECT(object), NULL);

  g_signal_emit(G_OBJECT(object), object_signals[OBJECT_COPY],0, &new_object);

  return new_object;
}

void amitk_object_copy_in_place(AmitkObject * dest_object, const AmitkObject * src_object) {
 
  g_return_if_fail(AMITK_IS_OBJECT(src_object));
  g_return_if_fail(AMITK_IS_OBJECT(dest_object));

  g_signal_emit(G_OBJECT(dest_object), object_signals[OBJECT_COPY_IN_PLACE],0, src_object);
  return;
}

void amitk_object_set_name(AmitkObject * object, const gchar * new_name) {

  g_return_if_fail(AMITK_IS_OBJECT(object));
 
  if (object->name != NULL)
    g_free(object->name);
  object->name = g_strdup(new_name);
  g_signal_emit(G_OBJECT(object), object_signals[OBJECT_NAME_CHANGED],0);

  return;
}

/* note, AMITK_SELECTION_ALL means all selections */
gboolean amitk_object_get_selected(AmitkObject * object, const AmitkSelection which_selection) {

  AmitkSelection i_selection;
  gboolean return_val = FALSE;

  g_return_val_if_fail(AMITK_IS_OBJECT(object), FALSE);
  g_return_val_if_fail(which_selection >= 0, FALSE);
  g_return_val_if_fail(which_selection <= AMITK_SELECTION_ALL, FALSE);
  g_return_val_if_fail(which_selection != AMITK_SELECTION_NUM, FALSE);

  if (which_selection == AMITK_SELECTION_ALL) {
    for (i_selection=0; i_selection < AMITK_SELECTION_NUM; i_selection++)
      return_val = object->selected[i_selection] || return_val;
  } else
    return_val = object->selected[which_selection];

  return return_val;
}

void amitk_object_set_selected(AmitkObject * object, const gboolean selection, const AmitkSelection which_selection) {

  AmitkSelection i_selection;
  GList * children;
  gboolean changed=FALSE;

  g_return_if_fail(AMITK_IS_OBJECT(object));
  g_return_if_fail(which_selection >= 0);
  g_return_if_fail(which_selection <= AMITK_SELECTION_ALL);
  g_return_if_fail(which_selection != AMITK_SELECTION_NUM);

  if (which_selection == AMITK_SELECTION_ALL) {
    for (i_selection=0; i_selection < AMITK_SELECTION_NUM; i_selection++) {
      if (object->selected[i_selection] != selection) {
	object->selected[i_selection] = selection;
	changed = TRUE;
      }
    }
  } else {
    if (object->selected[which_selection] != selection) {
      object->selected[which_selection] = selection;
      changed = TRUE;
    }
  }

  if (selection == FALSE) { /* propagate unselect to children */
    children = AMITK_OBJECT_CHILDREN(object);
    while (children != NULL) {
      amitk_object_set_selected(children->data, selection, which_selection);
      children = children->next;
    }
  }

  if (changed) 
    g_signal_emit(G_OBJECT(object), object_signals[OBJECT_SELECTION_CHANGED], 0);

  return;
}


void amitk_object_set_parent(AmitkObject * object,AmitkObject * parent) {
  
  gboolean ref_added = FALSE;

  g_return_if_fail(AMITK_IS_OBJECT(object));
  g_return_if_fail(AMITK_IS_OBJECT(parent) || (parent == NULL));

  if (object->parent != NULL) {
    g_object_ref(object);
    ref_added = TRUE;
    amitk_object_remove_child(object->parent, object);
  }

  if (parent != NULL) {
    amitk_object_add_child(parent, object);
  }

  if (ref_added == TRUE)
    g_object_unref(object);

  return;
}


void amitk_object_add_child(AmitkObject * object, AmitkObject * child) {

  g_return_if_fail(AMITK_IS_OBJECT(object));
  g_return_if_fail(AMITK_IS_OBJECT(child));

  /* check that it's not already in the list */
  g_return_if_fail(g_list_find(object->children, child) == NULL);

  g_signal_emit(G_OBJECT(object), object_signals[OBJECT_ADD_CHILD], 0, child);

  return;
}

void amitk_object_add_children(AmitkObject * object, GList * children) {

  if (children == NULL)  return;

  amitk_object_add_child(object, AMITK_OBJECT(children->data));
  amitk_object_add_children(object, children->next);

  return;
}

gboolean amitk_object_remove_child(AmitkObject * object, AmitkObject * child) {

  g_return_val_if_fail(AMITK_IS_OBJECT(object), FALSE);
  g_return_val_if_fail(AMITK_IS_OBJECT(child), FALSE);

  /* check if it's not in the list */
  g_return_val_if_fail(g_list_find(object->children, child) != NULL, FALSE);

  //  child->parent = NULL;
  //  object->children = g_list_remove(object->children, child);
  //  g_object_unref(child);
  g_signal_emit(G_OBJECT(object), object_signals[OBJECT_REMOVE_CHILD], 0, child);

  return TRUE;
}

gboolean amitk_object_remove_children(AmitkObject * object, GList * children) {

  gboolean valid;

  g_return_val_if_fail(AMITK_IS_OBJECT(object), FALSE);

  if (children == NULL) 
    return TRUE;

  valid = amitk_object_remove_children(object, children->next);
  valid = valid && amitk_object_remove_child(object, AMITK_OBJECT(children->data));

 return valid;
}


/* returns the type of the given object */
gboolean amitk_object_compare_object_type(AmitkObject * object, AmitkObjectType type) {

  switch(type) {
  case AMITK_OBJECT_TYPE_STUDY:
    return AMITK_IS_STUDY(object);
    break;
  case AMITK_OBJECT_TYPE_FIDUCIAL_MARK:
    return AMITK_IS_FIDUCIAL_MARK(object);
    break;
  case AMITK_OBJECT_TYPE_DATA_SET:
    return AMITK_IS_DATA_SET(object);
    break;
  case AMITK_OBJECT_TYPE_ROI:
    return AMITK_IS_ROI(object);
    break;
  case AMITK_OBJECT_TYPE_VOLUME:
    return AMITK_IS_VOLUME(object);
    break;
  default:
    g_return_val_if_reached(FALSE);
    break;
  }
}

/* return a referenced pointer 
   goes up the tree from the given object, finding the first parent of type "type"
   returns NULL if no appropriate parent found */
AmitkObject * amitk_object_get_parent_of_type(AmitkObject * object,
					      const AmitkObjectType type) {

  g_return_val_if_fail(AMITK_IS_OBJECT(object), NULL);

  if (AMITK_OBJECT_PARENT(object) == NULL)  /* top node */
    return NULL;
  else if (amitk_object_compare_object_type(AMITK_OBJECT_PARENT(object),type))
    return g_object_ref(AMITK_OBJECT_PARENT(object));
  else
    return amitk_object_get_parent_of_type(AMITK_OBJECT_PARENT(object), type); /* recurse */
}



/* returns a referenced list of children of the given object which
   are of the given type.  Will recurse if specified */
GList * amitk_object_get_children_of_type(AmitkObject * object, 
					  const AmitkObjectType type,
					  const gboolean recurse) {

  GList * children;
  GList * return_objects=NULL;
  GList * children_objects;
  AmitkObject * child;

  g_return_val_if_fail(AMITK_IS_OBJECT(object), NULL);

  children = AMITK_OBJECT_CHILDREN(object);
  while(children != NULL) {
    child = AMITK_OBJECT(children->data);

    if (amitk_object_compare_object_type(child,type))
      return_objects = g_list_append(return_objects, g_object_ref(child));

    if (recurse) { /* get child's objects */
      children_objects = amitk_object_get_children_of_type(AMITK_OBJECT(child), type, recurse);
      return_objects = g_list_concat(return_objects, children_objects);
    }

    children = children->next;
  }

  return return_objects;
}

/* indicates if any children of the given node has been selected */
gboolean amitk_object_selected_children(AmitkObject * object, 
					const AmitkSelection which_selection,
					gboolean recurse) {


  GList * children;
  AmitkObject * child;

  g_return_val_if_fail(AMITK_IS_OBJECT(object), FALSE);

  children = AMITK_OBJECT_CHILDREN(object);
  while(children != NULL) {
    child = AMITK_OBJECT(children->data);

    if (amitk_object_get_selected(child, which_selection))
      return TRUE;

    if (recurse) { /* check child's objects */
      if (amitk_object_selected_children(AMITK_OBJECT(child), which_selection, recurse))
	  return TRUE;
    }

    children = children->next;
  }

  return FALSE;
}

/* returns a referenced list of selected objects of type "type" that are children
   of the given node (usually the study object). Will recurse if specified*/
GList * amitk_object_get_selected_children(AmitkObject * object, 
					   const AmitkSelection which_selection,
					   gboolean recurse) {


  GList * children;
  GList * return_objects=NULL;
  GList * children_objects;
  AmitkObject * child;

  g_return_val_if_fail(AMITK_IS_OBJECT(object), NULL);

  children = AMITK_OBJECT_CHILDREN(object);
  while(children != NULL) {
    child = AMITK_OBJECT(children->data);

    if (amitk_object_get_selected(child, which_selection))
      return_objects = g_list_append(return_objects, g_object_ref(child));

    if (recurse) { /* get child's objects */
      children_objects = amitk_object_get_selected_children(AMITK_OBJECT(child), which_selection, recurse);
      return_objects = g_list_concat(return_objects, children_objects);
    }

    children = children->next;
  }

  return return_objects;
}

/* returns a referenced list of selected objects of type "type" that are children
   of the given node (usually the study object). Will recurse if specified*/
GList * amitk_object_get_selected_children_of_type(AmitkObject * object, 
						   const AmitkObjectType type,
						   const AmitkSelection which_selection,
						   gboolean recurse) {


  GList * children;
  GList * return_objects=NULL;
  GList * children_objects;
  AmitkObject * child;

  g_return_val_if_fail(AMITK_IS_OBJECT(object), NULL);

  children = AMITK_OBJECT_CHILDREN(object);
  while(children != NULL) {
    child = AMITK_OBJECT(children->data);

    if ((amitk_object_compare_object_type(child,type)) && (amitk_object_get_selected(child, which_selection)))
      return_objects = g_list_append(return_objects, g_object_ref(child));

    if (recurse) { /* get child's objects */
      children_objects = amitk_object_get_selected_children_of_type(AMITK_OBJECT(child), type, which_selection, recurse);
      return_objects = g_list_concat(return_objects, children_objects);
    }

    children = children->next;
  }

  return return_objects;
}


/* returns a copy of a list of objects, with a reference added for each copy */
GList * amitk_objects_ref(GList * objects) {

  GList * return_list;

  if (objects == NULL) return NULL;

  return_list = amitk_objects_ref(objects->next); /* recurse */

  if (AMITK_IS_OBJECT(objects->data)) {
    g_object_ref(G_OBJECT(objects->data)); /* add ref to this item */
    return_list = g_list_prepend(return_list, objects->data);
  } else
    g_return_val_if_reached(return_list);

  return return_list;
}

/* unrefs the objects in the given list and frees the list */
GList * amitk_objects_unref(GList * objects) {

  AmitkObject * object;

  if (objects == NULL) return NULL;

  objects->next = amitk_objects_unref(objects->next); /* recurse */

  if (AMITK_IS_OBJECT(objects->data)) {
    object = objects->data;
    g_list_remove(objects, object);
    g_object_unref(G_OBJECT(object));
  } else
    g_return_val_if_reached(NULL);

  return NULL;
}

gint amitk_objects_count(GList * objects) {

  gint count;

  if (objects == NULL) 
    return 0;

  if (AMITK_IS_OBJECT(objects->data))
    count = 1;
  else
    count = 0;

  /* count data sets that are children */
  count += amitk_objects_count(AMITK_OBJECT_CHILDREN(objects->data));

  /* add this count too the counts from the rest of the objects */
  return count+amitk_objects_count(objects->next);
}


/* return a pointer (unreferenced) to the first amitk_object in the list with the given name */
AmitkObject * amitk_objects_find_object_by_name(GList * objects, const gchar * name) {

  AmitkObject * object;

  if (objects == NULL)
    return NULL;
  
  object = objects->data;
  if (AMITK_IS_OBJECT(object))
    if (strcmp(AMITK_OBJECT_NAME(object), name) == 0)
      return object;

  return amitk_objects_find_object_by_name(objects->next, name);
}

/* goes through the two pairs of lists, and counts the number of names
   in list 1 that match a name in list 2 */
gint amitk_objects_count_pairs_by_name(GList * objects1, GList * objects2) {
  
  gint count=0;

  while (objects1 != NULL) {

    if (AMITK_IS_OBJECT(objects1->data))
      if (amitk_objects_find_object_by_name(objects2, AMITK_OBJECT_NAME(objects1->data)))
	count++;
    objects1 = objects1->next;
  }

  return count;
}

void amitk_objects_write_xml(GList * objects, xmlNodePtr node_list) {

  gchar * object_filename;

  if (objects == NULL) return;

  object_filename = amitk_object_write_xml(objects->data);
  xmlNewChild(node_list, NULL, "object_file", object_filename);
  free(object_filename);

  /* and recurse */
  amitk_objects_write_xml(objects->next, node_list);
  
  return;
}

GList * amitk_objects_read_xml(xmlNodePtr node_list) {

  GList * objects;
  AmitkObject * object;
  gchar * filename;

  if (node_list == NULL) return NULL;

  /* recurse */
  objects = amitk_objects_read_xml(node_list->next);

  /* and add this node */
  filename = xml_get_string(node_list, "object_file");
  object = amitk_object_read_xml(filename);
  g_free(filename);

  if (object != NULL)
    objects = g_list_prepend(objects, object);
  
  return objects;
}


const gchar * amitk_object_type_get_name(const AmitkObjectType type) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_OBJECT_TYPE);
  enum_value = g_enum_get_value(enum_class, type);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

const gchar * amitk_selection_get_name(const AmitkSelection type) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_SELECTION);
  enum_value = g_enum_get_value(enum_class, type);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}

