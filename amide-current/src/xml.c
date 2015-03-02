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


#include "amide_config.h"
#include "xml.h"

#define BOOLEAN_STRING_MAX_LENGTH 10 /* when we stop checking */
static char * true_string = "true";
static char * false_string = "false";

/* ----------------- the load functions ------------------ */


/* go through a list of nodes, and return a pointer to the node
   matching the descriptor */
xmlNodePtr xml_get_node(xmlNodePtr nodes, const gchar * descriptor) {

  if (nodes == NULL)
    return NULL;
  else {
    if (xmlStrcmp(nodes->name, (const xmlChar *) descriptor) == 0)
      return nodes;
    else
      return xml_get_node(nodes->next, descriptor);
  }
}



/* go through a list of nodes, and return the text which matches the descriptor */
gchar * xml_get_string(xmlNodePtr nodes, const gchar * descriptor) {
  return xmlNodeGetContent(xml_get_node(nodes, descriptor));
}






amide_time_t xml_get_time(xmlNodePtr nodes, const gchar * descriptor) {

  gchar * temp_string;
  amide_time_t return_time;
  gint error;

  temp_string = xml_get_string(nodes, descriptor);

  if (temp_string != NULL) {

#if (SIZE_OF_AMIDE_TIME_T == 8)
    /* convert to double */
    error = sscanf(temp_string, "%lf", &return_time);
#elif (SIZE_OF_AMIDE_TIME_T == 4)
    /* convert to float */
    error = sscanf(temp_string, "%f", &return_time);
#else
#error "Unknown size for SIZE_OF_AMIDE_TIME_T"
#endif
    g_free(temp_string);
  }
  

  if ((temp_string == NULL) || (error == EOF)) {
    return_time = 0.0;
    g_warning("Couldn't read time value for %s, substituting %5.3f",descriptor,
	      return_time);
  }

  return return_time;
}



amide_time_t * xml_get_times(xmlNodePtr nodes, const gchar * descriptor, guint num_times) {

  gchar * temp_string;
  gchar ** string_chunks;
  amide_time_t * return_times;
  gint error;
  guint i;

  temp_string = xml_get_string(nodes, descriptor);

  if (temp_string != NULL) {

    if ((return_times = g_new(amide_time_t,num_times)) == NULL) {
      g_warning("Couldn't allocate space for time data");
      return return_times;
    }
    
    /* split-up the string so we can process it */
    string_chunks = g_strsplit(temp_string, "\t", num_times);
    g_free(temp_string);
    
    for (i=0;i<num_times;i++) {

#if (SIZE_OF_AMIDE_TIME_T == 8)
      /* convert to doubles */
      error = sscanf(string_chunks[i], "%lf", &(return_times[i]));
#elif (SIZE_OF_AMIDE_TIME_T == 4)
      /* convert to float */
      error = sscanf(string_chunks[i], "%f", &(return_times[i]));
#else
#error "Unknown size for SIZE_OF_AMIDE_TIME_T"
#endif
      
      if ((error == EOF))  return_times[i] = 0.0;
    }

    g_strfreev(string_chunks);

  }

  if (temp_string == NULL) {
    g_warning("Couldn't read value for %s, substituting zero",descriptor);
    if ((return_times = g_new(amide_time_t,1)) == NULL) {
      g_warning("Couldn't allocate space for time data");
      return return_times;
    }
    return_times[0] = 0.0;
  }

  return return_times;
}



amide_data_t xml_get_data(xmlNodePtr nodes, const gchar * descriptor) {

  gchar * temp_string;
  amide_data_t return_data;
  gint error;

  temp_string = xml_get_string(nodes, descriptor);

  if (temp_string != NULL) {

#if (SIZE_OF_AMIDE_DATA_T == 8)
    /* convert to doubles */
    error = sscanf(temp_string, "%lf", &return_data);
#elif (SIZE_OF_AMIDE_DATA_T == 4)
    /* convert to float */
    error = sscanf(temp_string, "%f", &return_data);
#else
#error "Unknown size for SIZE_OF_AMIDE_DATA_T"
#endif
    
    g_free(temp_string);
 
  }

  if ((temp_string == NULL) || (error == EOF)) {
    return_data = 0.0;
    g_warning("Couldn't read value for %s, substituting %5.3f",descriptor, return_data);
  }

  return return_data;
}

amide_real_t xml_get_real(xmlNodePtr nodes, const gchar * descriptor) {

  gchar * temp_string;
  amide_real_t return_data;
  gint error;

  temp_string = xml_get_string(nodes, descriptor);
  
  if (temp_string != NULL) {

#if (SIZE_OF_AMIDE_REAL_T == 8)
    /* convert to doubles */
    error = sscanf(temp_string, "%lf", &return_data);
#elif (SIZE_OF_AMIDE_REAL_T == 4)
    /* convert to float */
    error = sscanf(temp_string, "%f", &return_data);
#else
#error "Unkown size for SIZE_OF_AMIDE_REAL_T"
#endif

    g_free(temp_string);
    if ((error == EOF))  return_data = 0.0;
  } else
    return_data = 0.0;

  return return_data;
}

gboolean xml_get_boolean(xmlNodePtr nodes, const gchar * descriptor) {
  gchar * temp_string;
  gboolean value;
  
  temp_string = xml_get_string(nodes, descriptor);

  if (temp_string == NULL) {
    /* TRUE tends to be better for backwards compatibility */
    g_warning("Couldn't read value for %s, substituting TRUE",descriptor);
    return TRUE;
  }
  if (g_ascii_strncasecmp(temp_string, true_string, BOOLEAN_STRING_MAX_LENGTH) == 0)
    value = TRUE;
  else 
    value = FALSE;
  g_free(temp_string);

  return value;
}

gint xml_get_int(xmlNodePtr nodes, const gchar * descriptor) {

  gchar * temp_string;
  gint return_int;
  gint error;

  temp_string = xml_get_string(nodes, descriptor);

  if (temp_string != NULL) {
    /* convert to an int */
    error = sscanf(temp_string, "%d", &return_int);
    g_free(temp_string);
  } 

  if ((temp_string == NULL) || (error == EOF)) {
    return_int = 0;
    g_warning("Couldn't read value for %s, substituting %d",descriptor, return_int);
  }

  return return_int;
}










/* ----------------- the save functions ------------------ */


void xml_save_string(xmlNodePtr node, const gchar * descriptor, const gchar * string) {
  
  xmlNewChild(node, NULL, descriptor, string);

  return;
}



void xml_save_time(xmlNodePtr node, const gchar * descriptor, const amide_time_t num) {

  gchar * temp_string;

  temp_string = g_strdup_printf("%10.9f", num);
  xml_save_string(node, descriptor, temp_string);
  g_free(temp_string);

  return;
}


void xml_save_times(xmlNodePtr node, const gchar * descriptor, const amide_time_t * numbers, const int num) {

  gchar * temp_string;
  int i;

  if (num == 0)
    xml_save_string(node, descriptor, NULL);
  else {
    temp_string = g_strdup_printf("%10.9f", numbers[0]);
    for (i=1; i < num; i++) {
      temp_string = g_strdup_printf("%s\t%10.9f", temp_string, numbers[i]);
    }
    xml_save_string(node, descriptor, temp_string);
    g_free(temp_string);
  }

  return;
}

void xml_save_data(xmlNodePtr node, const gchar * descriptor, const amide_data_t num) {

  gchar * temp_string;

  temp_string = g_strdup_printf("%10.9f", num);
  xml_save_string(node, descriptor, temp_string);
  g_free(temp_string);

  return;
}

void xml_save_real(xmlNodePtr node, const gchar * descriptor, const amide_real_t num) {

  gchar * temp_string;

  temp_string = g_strdup_printf("%10.9f", num);
  xml_save_string(node, descriptor, temp_string);
  g_free(temp_string);

  return;
}


void xml_save_boolean(xmlNodePtr node, const gchar * descriptor, const gboolean value) {

  if (value)
    xml_save_string(node, descriptor, true_string);
  else
    xml_save_string(node, descriptor, false_string);
}

void xml_save_int(xmlNodePtr node, const gchar * descriptor, const int num) {

  gchar * temp_string;

  temp_string = g_strdup_printf("%d", num);
  xml_save_string(node, descriptor, temp_string);
  g_free(temp_string);

  return;
}











