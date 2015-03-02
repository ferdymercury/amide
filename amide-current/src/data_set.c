/* data_set.c
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

#include "config.h"
#include <glib.h>
#include "data_set.h"
#include "raw_data.h"
#include <sys/stat.h>

/* external variables */
guint data_format_sizes[] = {sizeof(guint8),
			     sizeof(gint8),
			     sizeof(guint16),
			     sizeof(gint16),
			     sizeof(guint32),
			     sizeof(gint32),
			     sizeof(gfloat),
			     sizeof(gdouble),
			     1};

gchar * data_format_names[] = {"Unsigned Byte (8 bit)", \
			       "Signed Byte (8 bit)", \
			       "Unsigned Short (16 bit)", \
			       "Signed Short (16 bit)", \
			       "Unsigned Integer (32 bit)", \
			       "Signed Integer (32 bit)", \
			       "Float (32 bit)", \
			       "Double (64 bit)"};


/* removes a reference to a data_set, frees up the data_set if no more references */
data_set_t * data_set_free(data_set_t * data_set) {

  if (data_set == NULL)
    return data_set;

  /* sanity checks */
  g_return_val_if_fail(data_set->reference_count > 0, NULL);
  
  /* remove a reference count */
  data_set->reference_count--;

  /* if we've removed all reference's, free the data_set */
  if (data_set->reference_count == 0) {
#ifdef AMIDE_DEBUG
    //    g_print("freeing data_set\n");
#endif
    g_free(data_set->data);
    g_free(data_set);
    data_set = NULL;
  }

  return data_set;
}

/* returns an initialized data_set structure */
data_set_t * data_set_init(void) {

  data_set_t * temp_data_set;

  if ((temp_data_set = 
       (data_set_t *) g_malloc(sizeof(data_set_t))) == NULL) {
    g_warning("%s: Couldn't allocate memory for the data_set structure",PACKAGE);
    return NULL;
  }
  temp_data_set->reference_count = 1;

  /* put in some sensable values */
  temp_data_set->dim = voxelpoint_zero;
  temp_data_set->data = NULL;
  temp_data_set->format = FLOAT;

  return temp_data_set;
}


/* function to write out the information content of a data_set into an xml
   file.  Returns a string containing the name of the file. */
gchar * data_set_write_xml(data_set_t * data_set, gchar * study_directory, gchar * data_set_name) {

  gchar * data_set_xml_filename;
  gchar * data_set_raw_filename;
  guint count;
  struct stat file_info;
  xmlDocPtr doc;
  guint num_elements;
  FILE * file_pointer;

  /* make a guess as to our filename */
  count = 1;
  data_set_xml_filename = g_strdup_printf("%s.xml", data_set_name);
  data_set_raw_filename = g_strdup_printf("%s.dat", data_set_name);

  /* see if this file already exists */
  while (stat(data_set_xml_filename, &file_info) == 0) {
    g_free(data_set_xml_filename);
    g_free(data_set_raw_filename);
    count++;
    data_set_xml_filename = g_strdup_printf("%s_%d.xml", data_set_name, count);
    data_set_raw_filename = g_strdup_printf("%s_%d.dat", data_set_name, count);
  }

  /* and we now have unique filenames */
#ifdef AMIDE_DEBUG
  g_print("\t- saving data set in file %s\n",data_set_xml_filename);
#endif

  /* write the data_set xml file */
  doc = xmlNewDoc("1.0");
  doc->children = xmlNewDocNode(doc, NULL, "Data_set", data_set_name);
  xml_save_voxelpoint(doc->children, "dim", data_set->dim);
  xml_save_string(doc->children,"raw_data_format", 
		  raw_data_format_names[raw_data_format_raw(data_set->format)]);

  /* store the name of our associated data file */
  xml_save_string(doc->children, "raw_data_file", data_set_raw_filename);

  /* and save */
  xmlSaveFile(data_set_xml_filename, doc);

  /* and we're done with the xml stuff*/
  xmlFreeDoc(doc);

  /* now to save the raw data */

  /* sanity check */
#if (SIZE_OF_AMIDE_DATA_T != 4)
#error "Haven't yet written support for 8 byte (double) data in data_set_write_xml"
#endif
  
  /* write it on out */
  if ((file_pointer = fopen(data_set_raw_filename, "w")) == NULL) {
    g_warning("%s: couldn't save raw data file: %s",PACKAGE, data_set_raw_filename);
    g_free(data_set_xml_filename);
    g_free(data_set_raw_filename);
    return NULL;
  }

  num_elements = data_set_size_data_mem(data_set);
  if (fwrite(data_set->data, 1, num_elements, file_pointer) != num_elements) {
    g_warning("%s: incomplete save of raw data file: %s",PACKAGE, data_set_raw_filename);
    g_free(data_set_xml_filename);
    g_free(data_set_raw_filename);
    return NULL;
  }
  fclose(file_pointer);

  g_free(data_set_raw_filename);
  return data_set_xml_filename;
}


/* function to load in a data set xml file */
data_set_t * data_set_load_xml(gchar * data_set_xml_filename, const gchar * study_directory) { 

  xmlDocPtr doc;
  data_set_t * new_data_set;
  xmlNodePtr nodes;
  raw_data_format_t i_raw_data_format, raw_data_format;
  gchar * temp_string;
  gchar * data_set_raw_filename;

  new_data_set = data_set_init();

  /* parse the xml file */
  if ((doc = xmlParseFile(data_set_xml_filename)) == NULL) {
    g_warning("%s: Couldn't Parse AMIDE data_set xml file %s/%s",PACKAGE, study_directory,data_set_xml_filename);
    return data_set_free(new_data_set);
  }

  /* get the root of our document */
  if ((nodes = xmlDocGetRootElement(doc)) == NULL) {
    g_warning("%s: AMIDE Data Set xml file doesn't appear to have a root: %s/%s",
	      PACKAGE, study_directory,data_set_xml_filename);
    return data_set_free(new_data_set);
  }

  /* get the document tree */
  nodes = nodes->children;

  /* figure out the data format */
  temp_string = xml_get_string(nodes, "raw_data_format");
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
  raw_data_format = FLOAT_BE; /* sensible guess in case we don't figure it out from the file */
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
  raw_data_format = FLOAT_LE; /* sensible guess in case we don't figure it out from the file */
#endif
  if (temp_string != NULL)
    for (i_raw_data_format=0; i_raw_data_format < NUM_RAW_DATA_FORMATS; i_raw_data_format++) 
      if (g_strcasecmp(temp_string, raw_data_format_names[i_raw_data_format]) == 0)
	raw_data_format = i_raw_data_format;
  g_free(temp_string);
  new_data_set->format = raw_data_format_data(raw_data_format);

  /* get the rest of the parameters */
  new_data_set->dim = xml_get_voxelpoint(nodes, "dim");

  /* get the name of our associated data file */
  data_set_raw_filename = xml_get_string(nodes, "raw_data_file");

  /* now load in the raw data */
#ifdef AMIDE_DEBUG
  g_print("reading data from file %s\n", data_set_raw_filename);
#endif
  new_data_set = raw_data_read_file(data_set_raw_filename, new_data_set, raw_data_format, 0);
   
  /* and we're done */
  g_free(data_set_raw_filename);
  xmlFreeDoc(doc);

  return new_data_set;
}



/* adds one to the reference count of a data_set */
data_set_t * data_set_add_reference(data_set_t * data_set) {

  if (data_set != NULL)
    data_set->reference_count++;

  return data_set;
}














