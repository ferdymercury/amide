/* xml.c - convience functions for working with xml files 
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2014 Andy Loening
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
#include "xml.h"
#include <errno.h>
#include <string.h>
#include <locale.h>
#include "amitk_common.h"

#define BOOLEAN_STRING_MAX_LENGTH 10 /* when we stop checking */
static char * true_string = "true";
static char * false_string = "false";

/* automagically converts the string to having the right radix for the current locale */
/* this crap needs to be used because on windows, setlocale isn't implemented on mingw */
void xml_convert_radix_to_local(gchar * conv_str) {

  gboolean period=TRUE;
  gdouble temp_float;
  gchar * radix_ptr;

  /* mingw doesn't really support setlocale, so use the following to figure out if we're using
     a period or comma for the radix */
  sscanf("1,9", "%lf", &temp_float);
  if (temp_float > 1.1) period=FALSE;

  if (period == FALSE) { /* using comma's */
    while ((radix_ptr = strchr(conv_str, '.')) != NULL)
      *radix_ptr = ',';
  } else { /* using period */
    while ((radix_ptr = strchr(conv_str, ',')) != NULL)
      *radix_ptr = '.';
  }

  return;
}

/* returns FALSE if we'll have problems reading this file on a 32bit system */
gboolean xml_check_file_32bit_okay(guint64 value) {

#if !defined (G_PLATFORM_WIN32)
  /* for some reason, 64bit calculations on windows returns garbage */
  if (sizeof(long) < sizeof(guint64)) {
    guint64 check = ((guint64) value) >> ((guint64) 31); /* long is signed, so 31 bits */
    if (check > 0) 
      return FALSE;
  }
#endif


  // the following doesn't work on win32 either
  //  if (sizeof(long) < sizeof(guint64)) {
  //    guint32 * value32;
  //    value32 = (guint32 *) &value;
  //
  //#if (G_BYTE_ORDER == G_BIG_ENDIAN)
  //    if (value32[0] > 0)
  //      return FALSE;
  //#else /* little endian */
  //    if (value32[1] > 0)
  //      return FALSE;
  //#endif
  //  }

  return TRUE;
}

/* utility functions */
gboolean xml_node_exists(xmlNodePtr nodes, const gchar * descriptor) {

  if (xml_get_node(nodes, descriptor) != NULL)
    return TRUE;
  else
    return FALSE;
}

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

  gchar * xml_str;
  gchar * return_str;

  xml_str = (gchar *) xmlNodeGetContent(xml_get_node(nodes, descriptor));
  if (xml_str != NULL)
    return_str = g_strdup_printf("%s", xml_str);
  else
    return_str = NULL;
  free(xml_str);

  return return_str;
}






amide_time_t xml_get_time(xmlNodePtr nodes, const gchar * descriptor, gchar ** perror_buf) {

  gchar * temp_str;
  amide_time_t return_time;
  gint error=0;
  gchar * saved_locale;
  
  saved_locale = g_strdup(setlocale(LC_NUMERIC,NULL));
  setlocale(LC_NUMERIC,"POSIX");

  temp_str = xml_get_string(nodes, descriptor);

  if (temp_str != NULL) {

    xml_convert_radix_to_local(temp_str);

#if (SIZE_OF_AMIDE_TIME_T == 8)
    /* convert to double */
    error = sscanf(temp_str, "%lf", &return_time);
#elif (SIZE_OF_AMIDE_TIME_T == 4)
    /* convert to float */
    error = sscanf(temp_str, "%f", &return_time);
#else
#error "Unknown size for SIZE_OF_AMIDE_TIME_T"
#endif
    g_free(temp_str);
  }
  
  if ((temp_str == NULL) || (error == EOF)) {
    return_time = 0.0;
    amitk_append_str_with_newline(perror_buf,_("Couldn't read time value for %s, substituting %5.3f"),
				  descriptor, return_time);
  }

  setlocale(LC_NUMERIC, saved_locale);
  g_free(saved_locale);
  return return_time;
}



amide_time_t * xml_get_times(xmlNodePtr nodes, const gchar * descriptor, guint num_times, gchar ** perror_buf) {

  gchar * temp_str;
  gchar ** string_chunks;
  amide_time_t * return_times=NULL;
  gint error;
  guint i;
  gchar * saved_locale;
  gboolean corrupted=FALSE;
  
  saved_locale = g_strdup(setlocale(LC_NUMERIC,NULL));
  setlocale(LC_NUMERIC,"POSIX");

  temp_str = xml_get_string(nodes, descriptor);

  if ((return_times = g_try_new(amide_time_t,num_times)) == NULL) {
    amitk_append_str_with_newline(perror_buf, _("Couldn't allocate memory space for time data"));
    return return_times;
  }

  if (temp_str != NULL) {
    /* split-up the string so we can process it */
    string_chunks = g_strsplit(temp_str, "\t", num_times);
    g_free(temp_str);
    
    for (i=0; (i<num_times) && (!corrupted);i++) {

      if (string_chunks[i] == NULL) 
	  corrupted = TRUE;
      else {

	xml_convert_radix_to_local(string_chunks[i]);

#if (SIZE_OF_AMIDE_TIME_T == 8)
	/* convert to doubles */
	error = sscanf(string_chunks[i], "%lf", &(return_times[i]));
#elif (SIZE_OF_AMIDE_TIME_T == 4)
	/* convert to float */
	error = sscanf(string_chunks[i], "%f", &(return_times[i]));
#else
#error "Unknown size for SIZE_OF_AMIDE_TIME_T"
#endif

	if ((error == EOF)) corrupted = TRUE;
      }
    }

    if (corrupted) {
      for (i=0; i<num_times; i++) return_times[i]=1.0;
      amitk_append_str_with_newline(perror_buf, _("XIF File appears corrupted, setting frame/gate time to 1"));
    }

    g_strfreev(string_chunks);
  }

  if (temp_str == NULL) {
    amitk_append_str_with_newline(perror_buf,_("Couldn't read value for %s, substituting 1"),descriptor);
    for (i=0; i<num_times; i++) return_times[i]=1.0;
  }

  setlocale(LC_NUMERIC, saved_locale);
  g_free(saved_locale);
  return return_times;
}



amide_data_t xml_get_data(xmlNodePtr nodes, const gchar * descriptor, gchar **perror_buf) {

  gchar * temp_str;
  amide_data_t return_data;
  gint error=0;
  gchar * saved_locale;
  
  saved_locale = g_strdup(setlocale(LC_NUMERIC,NULL));
  setlocale(LC_NUMERIC,"POSIX");

  temp_str = xml_get_string(nodes, descriptor);

  if (temp_str != NULL) {

    xml_convert_radix_to_local(temp_str);

#if (SIZE_OF_AMIDE_DATA_T == 8)
    /* convert to doubles */
    error = sscanf(temp_str, "%lf", &return_data);
#elif (SIZE_OF_AMIDE_DATA_T == 4)
    /* convert to float */
    error = sscanf(temp_str, "%f", &return_data);
#else
#error "Unknown size for SIZE_OF_AMIDE_DATA_T"
#endif
    
    g_free(temp_str);
 
  }

  if ((temp_str == NULL) || (error == EOF)) {
    return_data = 0.0;
    amitk_append_str_with_newline(perror_buf,_("Couldn't read value for %s, substituting %5.3f"),
				  descriptor, return_data);
  }

  setlocale(LC_NUMERIC, saved_locale);
  g_free(saved_locale);
  return return_data;
}

amide_data_t xml_get_data_with_default(xmlNodePtr nodes, const gchar * descriptor, amide_data_t default_data) {

  if (xml_node_exists(nodes, descriptor))
    return xml_get_data(nodes, descriptor, NULL);
  else
    return default_data;
}


amide_real_t xml_get_real(xmlNodePtr nodes, const gchar * descriptor, gchar **perror_buf) {

  gchar * temp_str;
  amide_real_t return_data;
  gint error=0;
  gchar * saved_locale;
  
  saved_locale = g_strdup(setlocale(LC_NUMERIC,NULL));
  setlocale(LC_NUMERIC,"POSIX");

  temp_str = xml_get_string(nodes, descriptor);
  
  if (temp_str != NULL) {

    xml_convert_radix_to_local(temp_str);

#if (SIZE_OF_AMIDE_REAL_T == 8)
    /* convert to doubles */
    error = sscanf(temp_str, "%lf", &return_data);
#elif (SIZE_OF_AMIDE_REAL_T == 4)
    /* convert to float */
    error = sscanf(temp_str, "%f", &return_data);
#else
#error "Unkown size for SIZE_OF_AMIDE_REAL_T"
#endif

    g_free(temp_str);
    if ((error == EOF))  return_data = 0.0;
  } else
    return_data = 0.0;

  setlocale(LC_NUMERIC, saved_locale);
  g_free(saved_locale);
  return return_data;
}

amide_real_t xml_get_real_with_default(xmlNodePtr nodes, const gchar * descriptor, amide_real_t default_real) {

  if (xml_node_exists(nodes, descriptor))
    return xml_get_data(nodes, descriptor, NULL);
  else
    return default_real;
}

gboolean xml_get_boolean(xmlNodePtr nodes, const gchar * descriptor, gchar **perror_buf) {

  gchar * temp_str;
  gboolean value;
  
  temp_str = xml_get_string(nodes, descriptor);

  if (temp_str == NULL) {
    amitk_append_str_with_newline(perror_buf,_("Couldn't read value for %s, substituting FALSE"),descriptor);
    return FALSE;
  }
  if (g_ascii_strncasecmp(temp_str, true_string, BOOLEAN_STRING_MAX_LENGTH) == 0)
    value = TRUE;
  else 
    value = FALSE;
  g_free(temp_str);

  return value;
}

gboolean xml_get_boolean_with_default(xmlNodePtr nodes, const gchar * descriptor, gboolean default_boolean) {

  if (xml_node_exists(nodes, descriptor))
    return xml_get_boolean(nodes, descriptor, NULL);
  else
    return default_boolean;
}

gint xml_get_int(xmlNodePtr nodes, const gchar * descriptor, gchar **perror_buf) {

  gchar * temp_str;
  gint return_int;
  gint error=0;

  temp_str = xml_get_string(nodes, descriptor);

  if (temp_str != NULL) {
    /* convert to an int */
    error = sscanf(temp_str, "%d", &return_int);
    g_free(temp_str);
  } 

  if ((temp_str == NULL) || (error == EOF)) {
    return_int = 0;
    amitk_append_str_with_newline(perror_buf,_("Couldn't read value for %s, substituting %d"),
				  descriptor, return_int);
  }

  return return_int;
}


gint xml_get_int_with_default(xmlNodePtr nodes, const gchar * descriptor, gint default_int) {

  if (xml_node_exists(nodes, descriptor))
    return xml_get_int(nodes, descriptor, NULL);
  else
    return default_int;
}

guint xml_get_uint(xmlNodePtr nodes, const gchar * descriptor, gchar **perror_buf) {

  gchar * temp_str;
  guint return_uint;
  gint error=0;

  temp_str = xml_get_string(nodes, descriptor);

  if (temp_str != NULL) {
    /* convert to an int */
    error = sscanf(temp_str, "%u", &return_uint);
    g_free(temp_str);
  } 

  if ((temp_str == NULL) || (error == EOF)) {
    return_uint = 0;
    amitk_append_str_with_newline(perror_buf,_("Couldn't read value for %s, substituting %u"),
				  descriptor, return_uint);
  }

  return return_uint;
}


guint xml_get_uint_with_default(xmlNodePtr nodes, const gchar * descriptor, guint default_uint) {

  if (xml_node_exists(nodes, descriptor))
    return xml_get_uint(nodes, descriptor, NULL);
  else
    return default_uint;
}


void xml_get_location_and_size(xmlNodePtr nodes, const gchar * descriptor, 
			       guint64 * location, guint64 * size, gchar **perror_buf) {

  gchar * temp_str;
  gint error=0;

  temp_str = xml_get_string(nodes, descriptor);

  if (temp_str != NULL) {
#if (SIZEOF_LONG == 8)
    error = sscanf(temp_str, "0x%lx 0x%lx", location, size);
#else
#if (SIZEOF_LONG_LONG == 8)
    error = sscanf(temp_str, "0x%llx 0x%llx", location, size);
#else
#error "Either LONG or LONG_LONG needs to by 8 bytes long"
#endif
#endif
    g_free(temp_str);
  } 

  if ((temp_str == NULL) || (error == EOF)) {
    *location = 0x0;
    *size = 0x0;
    amitk_append_str_with_newline(perror_buf,_("Couldn't read value for %s, substituting 0x%llx 0x%llx"),
				  descriptor, *location, *size);
  }

  return;
}










/* ----------------- the save functions ------------------ */


void xml_save_string(xmlNodePtr node, const gchar * descriptor, const gchar * string) {
  
  xmlNewChild(node, NULL, (xmlChar *) descriptor, (xmlChar *) string);

  return;
}



void xml_save_time(xmlNodePtr node, const gchar * descriptor, const amide_time_t num) {

#ifdef OLD_WIN32_HACKS
  gchar temp_str[128];
#else
  gchar * temp_str;
#endif
  gchar * saved_locale;
  
  saved_locale = g_strdup(setlocale(LC_NUMERIC,NULL));
  setlocale(LC_NUMERIC,"POSIX");

#ifdef OLD_WIN32_HACKS
  snprintf(temp_str, 128, "%10.9f", num);
#else
  temp_str = g_strdup_printf("%10.9f", num);
#endif
  xml_save_string(node, descriptor, temp_str);
#ifndef OLD_WIN32_HACKS
  g_free(temp_str);
#endif

  setlocale(LC_NUMERIC, saved_locale);
  g_free(saved_locale);
  return;
}


/* This is the old version fo xml_save_times.  This was replaced with the version below because
   the glib g_str functions didn't seem to respect the locale on win32 
void xml_save_times(xmlNodePtr node, const gchar * descriptor, const amide_time_t * numbers, const int num) {

  gchar * temp_str;
  int i;
  gchar * saved_locale;
  
  saved_locale = g_strdup(setlocale(LC_NUMERIC,NULL));
  setlocale(LC_NUMERIC,"POSIX");

  if (num == 0)
    xml_save_string(node, descriptor, NULL);
  else {
    temp_str = g_strdup_printf("%10.9f", numbers[0]);
    for (i=1; i < num; i++) {
      temp_str = g_strdup_printf("%s\t%10.9f", temp_str, numbers[i]);
    }
    xml_save_string(node, descriptor, temp_str);
    g_free(temp_str);
  }

  setlocale(LC_NUMERIC, saved_locale);
  g_free(saved_locale);
  return;
}
*/

void xml_save_times(xmlNodePtr node, const gchar * descriptor, const amide_time_t * numbers, const int num) {

  gchar num_str[128];
  gchar * temp_str2=NULL;
  gchar * temp_str=NULL;
  int i;
  gchar * saved_locale;
  
  saved_locale = g_strdup(setlocale(LC_NUMERIC,NULL));
  setlocale(LC_NUMERIC,"POSIX");

  if (num == 0)
    xml_save_string(node, descriptor, NULL);
  else {
    for (i=0; i < num; i++) {

      snprintf(num_str, 128, "%10.9f", numbers[i]);

      temp_str2=temp_str;
      if (temp_str2 == NULL) {
	temp_str=malloc(sizeof(char)*(strlen(num_str)+1));
	strcpy(temp_str, num_str);
      } else {
	temp_str=malloc(sizeof(char)*(strlen(num_str)+strlen(temp_str2)+2));
	strcpy(temp_str, temp_str2);
	strcat(temp_str, "\t");
	free(temp_str2);
	strcat(temp_str, num_str);
      }
    }
    xml_save_string(node, descriptor, temp_str);
    g_free(temp_str);
  }

  setlocale(LC_NUMERIC, saved_locale);
  g_free(saved_locale);
  return;
}

void xml_save_data(xmlNodePtr node, const gchar * descriptor, const amide_data_t num) {

#ifdef OLD_WIN32_HACKS
  gchar temp_str[128];
#else
  gchar * temp_str;
#endif
  gchar * saved_locale;
  
  saved_locale = g_strdup(setlocale(LC_NUMERIC,NULL));
  setlocale(LC_NUMERIC,"POSIX");

#ifdef OLD_WIN32_HACKS
  snprintf(temp_str, 128, "%10.9f", num);
#else
  temp_str = g_strdup_printf("%10.9f", num);
#endif
  xml_save_string(node, descriptor, temp_str);
#ifndef OLD_WIN32_HACKS
  g_free(temp_str);
#endif

  setlocale(LC_NUMERIC, saved_locale);
  g_free(saved_locale);
  return;
}

void xml_save_real(xmlNodePtr node, const gchar * descriptor, const amide_real_t num) {

#ifdef OLD_WIN32_HACKS
  gchar temp_str[128];
#else
  gchar * temp_str;
#endif
  gchar * saved_locale;
  
  saved_locale = g_strdup(setlocale(LC_NUMERIC,NULL));
  setlocale(LC_NUMERIC,"POSIX");

#ifdef OLD_WIN32_HACKS
  snprintf(temp_str, 128, "%10.9f", num);
#else
  temp_str = g_strdup_printf("%10.9f", num);
#endif
  xml_save_string(node, descriptor, temp_str);
#ifndef OLD_WIN32_HACKS
  g_free(temp_str);
#endif

  setlocale(LC_NUMERIC, saved_locale);
  g_free(saved_locale);
  return;
}


void xml_save_boolean(xmlNodePtr node, const gchar * descriptor, const gboolean value) {

  if (value)
    xml_save_string(node, descriptor, true_string);
  else
    xml_save_string(node, descriptor, false_string);
}

void xml_save_int(xmlNodePtr node, const gchar * descriptor, const gint num) {

  gchar * temp_str;

  temp_str = g_strdup_printf("%d", num);
  xml_save_string(node, descriptor, temp_str);
  g_free(temp_str);

  return;
}

void xml_save_uint(xmlNodePtr node, const gchar * descriptor, const guint num) {

  gchar * temp_str;

  temp_str = g_strdup_printf("%u", num);
  xml_save_string(node, descriptor, temp_str);
  g_free(temp_str);

  return;
}

void xml_save_location_and_size(xmlNodePtr node, const gchar * descriptor, 
				const guint64 location, const guint64 size) {

  gchar * temp_str;

#if (SIZEOF_LONG == 8)
  temp_str = g_strdup_printf("0x%lx 0x%lx", location, size);
#else
#if (SIZEOF_LONG_LONG == 8)
  temp_str = g_strdup_printf("0x%llx 0x%llx", location, size);
#endif
#endif

  xml_save_string(node, descriptor, temp_str);
  g_free(temp_str);

  return;
}






/* will return an xmlDocPtr if given either a xml filename,
   or a prexisting FILE (along with location and size of the
   xml data in the FILE) */
xmlDocPtr xml_open_doc(gchar * xml_filename, FILE * study_file,
		       guint64 location, guint64 size,
		       gchar ** perror_buf) {
 

  xmlDocPtr doc;
  gchar * xml_buffer;
  long location_long;
  size_t size_size;
  size_t bytes_read;

  if (study_file == NULL) { /* directory format */
    if ((doc = xmlParseFile(xml_filename)) == NULL) {
      amitk_append_str_with_newline(perror_buf,_("Couldn't Parse AMIDE xml file %s"),xml_filename);
      return NULL;
    }


  } else { /* flat file format */

    /* check for file size problems */
    if (!xml_check_file_32bit_okay(location)) {
      g_warning(_("File to large to read on 32bit platform."));
      return NULL;
    }
    location_long = location;

    if (!xml_check_file_32bit_okay(size)) {
      g_warning(_("File to large to read on 32bit platform.")); 
      return NULL;
    }
    size_size = size;

    xml_buffer = g_try_new(gchar, size_size);
    g_return_val_if_fail(xml_buffer != NULL, NULL);

    if (fseek(study_file, location_long,SEEK_SET) != 0) {
      amitk_append_str_with_newline(perror_buf, _("Could not seek to location %lx in file."), location_long);
      return NULL;
    }

    bytes_read = fread(xml_buffer, sizeof(gchar), size_size, study_file);
    if (bytes_read != size_size) {
      amitk_append_str_with_newline(perror_buf, _("Only read %x bytes from file, expected %x"), bytes_read, size_size);
      return NULL;
    }

    /* and actually  process the xml */
    doc = xmlParseMemory(xml_buffer, size_size);
    g_free(xml_buffer);
  }


  return doc;
}



