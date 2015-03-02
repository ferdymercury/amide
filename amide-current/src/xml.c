/* xml.c - convience functions for working with xml files 
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
#include "amide.h"
#include "xml.h"



/* ----------------- the load functions ------------------ */


/* go through a list of nodes, and return a pointer to the node
   matching the descriptor */
xmlNodePtr xml_get_node(xmlNodePtr nodes, gchar * descriptor) {

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
gchar * xml_get_string(xmlNodePtr nodes, gchar * descriptor) {
  return xmlNodeGetContent(xml_get_node(nodes, descriptor));
}



realpoint_t xml_get_realpoint(xmlNodePtr nodes, gchar * descriptor) {

  gchar * temp_string;
  realpoint_t return_point;
  gint error;

  temp_string = xml_get_string(nodes, descriptor);

  if (temp_string != NULL) {
#if (SIZE_OF_FLOATPOINT_T == 8)
    /* convert to doubles */
    error = sscanf(temp_string, "%lf\t%lf\t%lf", &(return_point.x), &(return_point.y), &(return_point.z));
#elif (SIZE_OF_FLOATPOINT_T == 4)
    /* convert to float */
    error = sscanf(temp_string, "%f\t%f\t%f", &(return_point.x), &(return_point.y), &(return_point.z));
#else
#error "Unknown size for SIZE_OF_FLOATPOINT_T"
#endif
    g_free(temp_string);
  }

  if ((temp_string == NULL) || (error == EOF)) {
    g_warning("%s: Couldn't read value for %s, substituting null",PACKAGE, descriptor);
    return_point = realpoint_init;
  }

  return return_point;

}






voxelpoint_t xml_get_voxelpoint(xmlNodePtr nodes, gchar * descriptor) {

  gchar * temp_string;
  voxelpoint_t return_point;
  gint x,y,z;
  gint error;

  temp_string = xml_get_string(nodes, descriptor);

  if (temp_string != NULL) {

    /* convert to a voxelpoint */
    error = sscanf(temp_string,"%d\t%d\t%d", &x,&y,&z);
    g_free(temp_string);
    
    return_point.x = x;
    return_point.y = y;
    return_point.z = z;

  } 

  if ((temp_string == NULL) || (error == EOF)) {
    g_warning("%s: Couldn't read value for %s, substituting null",PACKAGE, descriptor);
    return_point.x = return_point.y = return_point.z = 0;
  }

  return return_point;
}



volume_time_t xml_get_time(xmlNodePtr nodes, gchar * descriptor) {

  gchar * temp_string;
  volume_time_t return_time;
  gint error;

  temp_string = xml_get_string(nodes, descriptor);

  if (temp_string != NULL) {

#if (SIZE_OF_VOLUME_TIME_T == 8)
    /* convert to doubles */
    error = sscanf(temp_string, "%lf", &return_time);
#elif (SIZE_OF_VOLUME_TIME_T == 4)
    /* convert to float */
    error = sscanf(temp_string, "%f", &return_time);
#else
#error "Unknown size for SIZE_OF_VOLUME_TIME_T"
#endif
    g_free(temp_string);
  }
  

  if ((temp_string == NULL) || (error == EOF)) {
    g_warning("%s: Couldn't read value for %s, substituting null",PACKAGE, descriptor);
    return_time = 0.0;
  }

  return return_time;
}



volume_time_t * xml_get_times(xmlNodePtr nodes, gchar * descriptor, guint num_times) {

  gchar * temp_string;
  gchar ** string_chunks;
  volume_time_t * return_times;
  gint error;
  guint i;

  temp_string = xml_get_string(nodes, descriptor);

  if (temp_string != NULL) {

    if ((return_times = (volume_time_t * ) g_malloc(num_times*sizeof(volume_time_t))) == NULL) {
      g_warning("%s: Couldn't allocate space for time data",PACKAGE);
      return return_times;
    }
    
    /* split-up the string so we can process it */
    string_chunks = g_strsplit(temp_string, "\t", num_times);
    g_free(temp_string);
    
    for (i=0;i<num_times;i++) {

#if (SIZE_OF_VOLUME_TIME_T == 8)
      /* convert to doubles */
      error = sscanf(string_chunks[i], "%lf", &(return_times[i]));
#elif (SIZE_OF_VOLUME_TIME_T == 4)
      /* convert to float */
      error = sscanf(string_chunks[i], "%f", &(return_times[i]));
#else
#error "Unknown size for SIZE_OF_VOLUME_TIME_T"
#endif
      
      if ((error == EOF))  return_times[i] = 0.0;
    }

    g_strfreev(string_chunks);

  }

  if (temp_string == NULL) {
    g_warning("%s: Couldn't read value for %s, substituting null",PACKAGE, descriptor);
    if ((return_times = (volume_time_t * ) g_malloc(1*sizeof(volume_time_t))) == NULL) {
      g_warning("%s: Couldn't allocate space for time data",PACKAGE);
      return return_times;
    }
    return_times[0] = 0.0;
  }

  return return_times;
}



volume_data_t xml_get_data(xmlNodePtr nodes, gchar * descriptor) {

  gchar * temp_string;
  volume_data_t return_data;
  gint error;

  temp_string = xml_get_string(nodes, descriptor);

  if (temp_string != NULL) {

#if (SIZE_OF_VOLUME_DATA_T == 8)
    /* convert to doubles */
    error = sscanf(temp_string, "%lf", &return_data);
#elif (SIZE_OF_VOLUME_DATA_T == 4)
    /* convert to float */
    error = sscanf(temp_string, "%f", &return_data);
#else
#error "Unknown size for SIZE_OF_VOLUME_DATA_T"
#endif
    
    g_free(temp_string);
 
  }

  if ((temp_string == NULL) || (error == EOF)) {
    g_warning("%s: Couldn't read value for %s, substituting null",PACKAGE, descriptor);
    return_data = 0.0;
  }

  return return_data;
}

floatpoint_t xml_get_floatpoint(xmlNodePtr nodes, gchar * descriptor) {

  gchar * temp_string;
  floatpoint_t return_data;
  gint error;

  temp_string = xml_get_string(nodes, descriptor);
  
  if (temp_string != NULL) {

#if (SIZE_OF_FLOATPOINT_T == 8)
    /* convert to doubles */
    error = sscanf(temp_string, "%lf", &return_data);
#elif (SIZE_OF_FLOATPOINT_T == 4)
    /* convert to float */
    error = sscanf(temp_string, "%f", &return_data);
#else
#error "Unkown size for SIZE_OF_FLOATPOINT_T"
#endif

    g_free(temp_string);
    if ((error == EOF))  return_data = 0.0;
  } else
    return_data = 0.0;

  return return_data;
}


gint xml_get_int(xmlNodePtr nodes, gchar * descriptor) {

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
    g_warning("%s: Couldn't read value for %s, substituting null",PACKAGE, descriptor);
    return_int = 0;
  }

  return return_int;
}




realspace_t xml_get_realspace(xmlNodePtr nodes, gchar * descriptor) {

  gchar * temp_string;
  axis_t i_axis;
  realspace_t new_space;

  temp_string = g_strdup_printf("%s_offset", descriptor);
  new_space.offset = xml_get_realpoint(nodes,temp_string);
  g_free(temp_string);

  for (i_axis=0;i_axis<NUM_AXIS;i_axis++) {
    temp_string = g_strdup_printf("%s_%s", descriptor, axis_names[i_axis]);
    new_space.axis[i_axis] = xml_get_realpoint(nodes,temp_string);
    if (REALPOINT_EQUAL(new_space.axis[i_axis], realpoint_init))
	new_space.axis[i_axis] = default_axis[i_axis];
    g_free(temp_string);
  }

  return new_space;
}














/* ----------------- the save functions ------------------ */


void xml_save_string(xmlNodePtr node, gchar * descriptor, gchar * string) {
  
  xmlNewChild(node, NULL, descriptor, string);

  return;
}

void xml_save_realpoint(xmlNodePtr node, gchar * descriptor, realpoint_t rp) {

  gchar * temp_string;

  temp_string = g_strdup_printf("%10.9f\t%10.9f\t%10.9f",rp.x, rp.y,rp.z);
  xml_save_string(node, descriptor, temp_string);
  g_free(temp_string);

  return;
}

void xml_save_voxelpoint(xmlNodePtr node, gchar * descriptor, voxelpoint_t vp) {

  gchar * temp_string;

  temp_string = g_strdup_printf("%d\t%d\t%d",vp.x, vp.y,vp.z);
  xml_save_string(node, descriptor, temp_string);
  g_free(temp_string);

  return;
}



void xml_save_time(xmlNodePtr node, gchar * descriptor, volume_time_t num) {

  gchar * temp_string;

  temp_string = g_strdup_printf("%10.9f", num);
  xml_save_string(node, descriptor, temp_string);
  g_free(temp_string);

  return;
}


void xml_save_times(xmlNodePtr node, gchar * descriptor, volume_time_t * numbers, int num) {

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

void xml_save_data(xmlNodePtr node, gchar * descriptor, volume_data_t num) {

  gchar * temp_string;

  temp_string = g_strdup_printf("%10.9f", num);
  xml_save_string(node, descriptor, temp_string);
  g_free(temp_string);

  return;
}

void xml_save_floatpoint(xmlNodePtr node, gchar * descriptor, floatpoint_t num) {

  gchar * temp_string;

  temp_string = g_strdup_printf("%10.9f", num);
  xml_save_string(node, descriptor, temp_string);
  g_free(temp_string);

  return;
}


void xml_save_int(xmlNodePtr node, gchar * descriptor, int num) {

  gchar * temp_string;

  temp_string = g_strdup_printf("%d", num);
  xml_save_string(node, descriptor, temp_string);
  g_free(temp_string);

  return;
}



void xml_save_realspace(xmlNodePtr node, gchar * descriptor, realspace_t coord_frame) {

  gchar * temp_string;
  axis_t i_axis;

  temp_string = g_strdup_printf("%s_offset", descriptor);
  xml_save_realpoint(node,temp_string, coord_frame.offset);
  g_free(temp_string);

  for (i_axis=0;i_axis<NUM_AXIS;i_axis++) {
    temp_string = g_strdup_printf("%s_%s", descriptor, axis_names[i_axis]);
    xml_save_realpoint(node,temp_string, coord_frame.axis[i_axis]);
    g_free(temp_string);
  }

  return;
}





