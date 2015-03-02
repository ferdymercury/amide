/* amitk_raw_data.c
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

#include <sys/stat.h>
#include <stdio.h>

#include "amitk_raw_data.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"

#define DATA_CONTENT(data, dim, i) ((data)[(i).x + (dim).x*(i).y])

/* external variables */
guint amitk_format_sizes[] = {
  sizeof(amitk_format_UBYTE_t),
  sizeof(amitk_format_SBYTE_t),
  sizeof(amitk_format_USHORT_t),
  sizeof(amitk_format_SSHORT_t),
  sizeof(amitk_format_UINT_t),
  sizeof(amitk_format_SINT_t),
  sizeof(amitk_format_FLOAT_t),
  sizeof(amitk_format_DOUBLE_t),
  1
};

gchar * amitk_format_names[] = {
  "Unsigned Byte (8 bit)", 
  "Signed Byte (8 bit)", 
  "Unsigned Short (16 bit)", 
  "Signed Short (16 bit)", 
  "Unsigned Integer (32 bit)", 
  "Signed Integer (32 bit)", 
  "Float (32 bit)", 
  "Double (64 bit)"
};
 

guint amitk_raw_format_sizes[] = {
  sizeof(amitk_format_UBYTE_t),
  sizeof(amitk_format_SBYTE_t),
  sizeof(amitk_format_USHORT_t),
  sizeof(amitk_format_SSHORT_t),
  sizeof(amitk_format_UINT_t),
  sizeof(amitk_format_SINT_t),
  sizeof(amitk_format_FLOAT_t),
  sizeof(amitk_format_DOUBLE_t),
  sizeof(amitk_format_USHORT_t),
  sizeof(amitk_format_SSHORT_t),
  sizeof(amitk_format_UINT_t),
  sizeof(amitk_format_SINT_t),
  sizeof(amitk_format_FLOAT_t),
  sizeof(amitk_format_DOUBLE_t),
  sizeof(amitk_format_UINT_t),
  sizeof(amitk_format_SINT_t),
  sizeof(amitk_format_FLOAT_t),
  1,
};

gchar * amitk_raw_format_names[] = {
  "Unsigned Byte (8 bit)", 
  "Signed Byte (8 bit)", 
  "Unsigned Short, Little Endian (16 bit)", 
  "Signed Short, Little Endian (16 bit)", 
  "Unsigned Integer, Little Endian (32 bit)", 
  "Signed Integer, Little Endian (32 bit)", 
  "Float, Little Endian (32 bit)", 
  "Double, Little Endian (64 bit)", 
  "Unsigned Short, Big Endian (16 bit)", 
  "Signed Short, Big Endian (16 bit)", 
  "Unsigned Integer, Big Endian (32 bit)", 
  "Signed Integer, Big Endian (32 bit)", 
  "Float, Big Endian (32 bit)", 
  "Double, Big Endian (64 bit)", 
  "Unsigned Integer, PDP (32 bit)", 
  "Signed Integer, PDP (32 bit)", 
  "Float, PDP/VAX (32 bit)", 
  "ASCII (8 bit)"
};


//enum {
//  LAST_SIGNAL
//};

static void raw_data_class_init          (AmitkRawDataClass *klass);
static void raw_data_init                (AmitkRawData      *object);
static void raw_data_finalize            (GObject           *object);
static GObject * parent_class;
//static guint     raw_data_signals[LAST_SIGNAL];



GType amitk_raw_data_get_type(void) {

  static GType raw_data_type = 0;

  if (!raw_data_type)
    {
      static const GTypeInfo raw_data_info =
      {
	sizeof (AmitkRawDataClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) raw_data_class_init,
	(GClassFinalizeFunc) NULL,
	NULL,		/* class_data */
	sizeof (AmitkRawData),
	0,			/* n_preallocs */
	(GInstanceInitFunc) raw_data_init,
	NULL /* value table */
      };
      
      raw_data_type = g_type_register_static (G_TYPE_OBJECT, "AmitkRawData", &raw_data_info, 0);
    }
  
  return raw_data_type;
}


static void raw_data_class_init (AmitkRawDataClass * class) {

  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent(class);
  
  gobject_class->finalize = raw_data_finalize;

}

static void raw_data_init (AmitkRawData * raw_data) {

  raw_data->dim = zero_voxel;
  raw_data->data = NULL;
  raw_data->format = AMITK_FORMAT_DOUBLE;

  return;
}


static void raw_data_finalize (GObject *object) {

  AmitkRawData * raw_data = AMITK_RAW_DATA(object);

  if (raw_data->data != NULL) {
#ifdef AMIDE_DEBUG
    //    g_print("\tfreeing raw data\n");
#endif
    g_free(raw_data->data);
    raw_data->data = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


AmitkRawData * amitk_raw_data_new (void) {

  AmitkRawData * raw_data;

  raw_data = g_object_new(amitk_raw_data_get_type(), NULL);

  return raw_data;
}







/* reads the contents of a raw data file into an amide raw data structure,
   note: file_offset is bytes for a binary file, lines for an ascii file
*/
AmitkRawData * amitk_raw_data_import_raw_file(const gchar * file_name, 
					      AmitkRawFormat raw_format,
					      AmitkVoxel dim,
					      guint file_offset) {

  FILE * file_pointer=NULL;
  void * file_buffer=NULL;
  size_t bytes_per_slice=0;
  size_t bytes_read;
  AmitkVoxel i;
  gint error_code, j;
  AmitkRawData * raw_data=NULL;;

  if (file_name == NULL) {
    g_warning("raw_data_read called with no filename");
    goto error;
  }

  if ((raw_data = amitk_raw_data_new()) == NULL) {
    g_warning("couldn't allocate space for the raw data set structure");
    goto error;
  }

  /* figure out what internal data format we should be using */
  raw_data->format = amitk_raw_format_to_format(raw_format);
  
  /* allocate the space for the data set */
  raw_data->dim = dim;
  if ((raw_data->data = amitk_raw_data_get_data_mem(raw_data)) == NULL) {
    g_warning("couldn't allocate space for the raw data");
    goto error;
  }

  /* open the raw data file for reading */
  if (raw_format == AMITK_RAW_FORMAT_ASCII_NE) {
    if ((file_pointer = fopen(file_name, "r")) == NULL) {
      g_warning("couldn't open raw data file %s", file_name);
      goto error;
    }
  } else { /* note, rb==r on any POSIX compliant system (i.e. Linux). */
    if ((file_pointer = fopen(file_name, "rb")) == NULL) {
      g_warning("couldn't open raw data file %s", file_name);
      goto error;
    }
  }
  
  /* jump forward by the given offset */
  if (raw_format == AMITK_RAW_FORMAT_ASCII_NE) {
    for (j=0; j<file_offset; j++)
      if ((error_code = fscanf(file_pointer, "%*f")) < 0) { /*EOF is usually -1 */
	g_warning("could not step forward %d elements in raw data file:\n\t%s\n\treturned error: %d",
		  j+1, file_name, error_code);
	goto error;
    }
  } else { /* binary */
    if (fseek(file_pointer, file_offset, SEEK_SET) != 0) {
      g_warning("could not seek forward %d bytes in raw data file:\n\t%s",
		file_offset, file_name);
      goto error;
    }
  }
    
  /* read in the contents of the file */
  if (raw_format != AMITK_RAW_FORMAT_ASCII_NE) { /* ASCII handled in the loop below */
    bytes_per_slice = amitk_raw_format_calc_num_bytes_per_slice(raw_data->dim, raw_format);
    if ((file_buffer = (void *) g_malloc(bytes_per_slice)) == NULL) {
      g_warning("couldn't malloc %d bytes for file buffer\n", bytes_per_slice);
      goto error;
    }
  }

  /* iterate over the # of frames */
  error_code = 1;
  for (i.t = 0; i.t < raw_data->dim.t; i.t++) {
#ifdef AMIDE_DEBUG
    g_print("\tloading frame %d\n",i.t);
#endif
    for (i.z = 0; (i.z < raw_data->dim.z) && (error_code >= 0) ; i.z++) {

      /* read in the contents of the file */
      if (raw_format != AMITK_RAW_FORMAT_ASCII_NE) { /* ASCII handled in the loop below */
	bytes_read = fread(file_buffer, 1, bytes_per_slice, file_pointer );
	if (bytes_read != bytes_per_slice) {
	  g_warning("read wrong number of elements from raw data file:\n\t%s\n\texpected %d\tgot %d", 
		    file_name, bytes_per_slice, bytes_read);
	  goto error;
	}
      }
  
    
      /* and convert the data */
      switch (raw_format) {
	
      case AMITK_RAW_FORMAT_ASCII_NE:
	
	/* copy this frame into the data set */
	for (i.y = 0; (i.y < raw_data->dim.y) && (error_code >= 0); i.y++) {
	  for (i.x = 0; (i.x < raw_data->dim.x) && (error_code >= 0); i.x++) {
	    if (raw_data->format == AMITK_FORMAT_DOUBLE)
	      error_code = fscanf(file_pointer, "%lf", AMITK_RAW_DATA_DOUBLE_POINTER(raw_data,i));
	    else // (raw_data->format == FLOAT)
	      error_code = fscanf(file_pointer, "%f", AMITK_RAW_DATA_FLOAT_POINTER(raw_data,i));
	    if (error_code == 0) error_code = EOF; /* if we couldn't read, may as well be EOF*/
	  }
	}
	
	if (error_code < 0) { /* EOF = -1 (usually) */
	  g_warning("could not read ascii file after %d elements, file or parameters are erroneous:\n\t%s",
		    i.x + 
		    raw_data->dim.x*i.y +
		    raw_data->dim.x*raw_data->dim.y*i.z +
		    raw_data->dim.x*raw_data->dim.y*raw_data->dim.z*i.t,
		    file_name);
	  goto error;
	}
	
	break;
	
      case AMITK_RAW_FORMAT_FLOAT_PDP:
	{
	  guint32 * data = file_buffer;
	  guint32 temp;
	  gfloat * float_p;

	  /* copy this frame into the data set */
	  for (i.y = 0; i.y < raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data->dim.x; i.x++) {
	      /* keep compiler from generating bad code with a noop,occurs with gcc 3.0.3 */
	      if (i.x == -1) g_print("no_op\n");
	      
	      temp = GUINT32_FROM_PDP(DATA_CONTENT(data, raw_data->dim,i));
	      float_p = (void *) &temp;
	      AMITK_RAW_DATA_FLOAT_SET_CONTENT(raw_data,i) = *float_p;
	    }
	}
	break;
      case AMITK_RAW_FORMAT_SINT_PDP:
	{
	  gint32 * data=file_buffer;
	  
	  /* copy this frame into the data set */
	  for (i.y = 0; i.y < raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data->dim.x; i.x++)
	      AMITK_RAW_DATA_SINT_SET_CONTENT(raw_data,i) = 
		GINT32_FROM_PDP(DATA_CONTENT(data, raw_data->dim,i));
	}
	break;
      case AMITK_RAW_FORMAT_UINT_PDP:
	{
	  guint32 * data=file_buffer;
	  
	  /* copy this frame into the data set */
	  for (i.y = 0; i.y < raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data->dim.x; i.x++) {
	      /* keep compiler from generating bad code with a noop,occurs with gcc 3.0.3 */
	      if (i.x == -1) g_print("no_op\n");

	      AMITK_RAW_DATA_UINT_SET_CONTENT(raw_data,i) = 
		GUINT32_FROM_PDP(DATA_CONTENT(data, raw_data->dim,i));
	    }
	}
	break;
	
      case AMITK_RAW_FORMAT_DOUBLE_BE: 
	{
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
	  guint64 * data = file_buffer;
	  guint64 temp;
	  gdouble * double_p;
#else
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
	  gdouble * data = file_buffer;
#else
#error "need to specify G_BIG_ENDIAN or G_LITTLE_ENDIAN"	
#endif
#endif
	  
	  /* copy this frame into the data set */
	  for (i.y = 0; i.y < raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data->dim.x; i.x++) {
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
	      temp = 
		GUINT64_FROM_BE(DATA_CONTENT(data, raw_data->dim, i));
	      double_p = (void *) &temp;
	      AMITK_RAW_DATA_DOUBLE_SET_CONTENT(raw_data,i) = *double_p;
#else /* G_BIG_ENDIAN */
	      AMITK_RAW_DATA_DOUBLE_SET_CONTENT(raw_data,i) = DATA_CONTENT(data, raw_data->dim, i);
#endif
              }
	}
	break;
      case AMITK_RAW_FORMAT_FLOAT_BE:
	{
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
	  guint32 * data = file_buffer;
	  guint32 temp;
	  gfloat * float_p;
#else /* (G_BYTE_ORDER == G_BIG_ENDIAN) */
	  gfloat * data = file_buffer;
#endif
	  
	  /* copy this frame into the data set */
	  for (i.y = 0; i.y < raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data->dim.x; i.x++) {
	      /* keep compiler from generating bad code with a noop, occurs with gcc 3.0.3 */
	      if (i.x == -1) g_print("no op\n");

#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
	      temp = GUINT32_FROM_BE(DATA_CONTENT(data, raw_data->dim, i));
	      float_p = (void *) &temp;
	      AMITK_RAW_DATA_FLOAT_SET_CONTENT(raw_data,i) = *float_p;
#else /* G_BIG_ENDIAN */
	      AMITK_RAW_DATA_FLOAT_SET_CONTENT(raw_data,i) = DATA_CONTENT(data, raw_data->dim, i);
#endif
	    }
	}
	break;
      case AMITK_RAW_FORMAT_SINT_BE:
	{
	  gint32 * data=file_buffer;
	  
	  /* copy this frame into the data set */
	  for (i.y = 0; i.y < raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data->dim.x; i.x++)
	      AMITK_RAW_DATA_SINT_SET_CONTENT(raw_data,i) = 
		GINT32_FROM_BE(DATA_CONTENT(data, raw_data->dim, i));
	}
	break;
      case AMITK_RAW_FORMAT_UINT_BE:
	{
	  guint32 * data=file_buffer;
	  
	  /* copy this frame into the data set */
	  for (i.y = 0; i.y < raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data->dim.x; i.x++) {
	      /* keep compiler from generating bad code with a noop occurs with gcc 3.0.3 */
	      if (i.x == -1) g_print("no op\n");

	      AMITK_RAW_DATA_UINT_SET_CONTENT(raw_data,i) = 
		GUINT32_FROM_BE(DATA_CONTENT(data, raw_data->dim, i));
	    }
	}
	break;
      case AMITK_RAW_FORMAT_SSHORT_BE:
	{
	  gint16 * data=file_buffer;
	  
	  /* copy this frame into the data set */
	  for (i.y = 0; i.y < raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data->dim.x; i.x++)
	      AMITK_RAW_DATA_SSHORT_SET_CONTENT(raw_data,i) = 
		GINT16_FROM_BE(DATA_CONTENT(data, raw_data->dim, i));
	}
	break;
      case AMITK_RAW_FORMAT_USHORT_BE:
	{
	  guint16 * data=file_buffer;
	  
	  /* copy this frame into the data set */
	  for (i.y = 0; i.y < raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data->dim.x; i.x++)
	      AMITK_RAW_DATA_USHORT_SET_CONTENT(raw_data,i) =
		GUINT16_FROM_BE(DATA_CONTENT(data, raw_data->dim, i));
	}
	break;
	

	
	
      case AMITK_RAW_FORMAT_DOUBLE_LE:
	{
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
	  guint64 * data = file_buffer;
	  guint64 temp;
	  gdouble * double_p;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
	  gdouble * data = file_buffer;
#endif
	  
	  /* copy this frame into the data set */
	  for (i.y = 0; i.y < raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data->dim.x; i.x++) {
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
	      temp = GUINT64_FROM_LE(DATA_CONTENT(data, raw_data->dim, i));
	      double_p = (void *) &temp;
	      AMITK_RAW_DATA_DOUBLE_SET_CONTENT(raw_data,i) = *double_p;
#else /* G_LITTLE_ENDIAN */
	      AMITK_RAW_DATA_DOUBLE_SET_CONTENT(raw_data,i) = DATA_CONTENT(data, raw_data->dim, i);
#endif
	    }
	}
	break;




      case AMITK_RAW_FORMAT_FLOAT_LE:
	{
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
	  guint32 * data = file_buffer;
	  guint32 temp;
	  gfloat * float_p;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
	  gfloat * data = file_buffer;
#endif

	  /* copy this frame into the data set */
	  for (i.y = 0; i.y < raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data->dim.x; i.x++) {
	      
	      /* keep compiler from generating bad code with a noop,occurs with gcc 3.0.3 */
	      if (i.x == -1) g_print("no op\n");
	      
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
	      temp = GUINT32_FROM_LE(DATA_CONTENT(data, raw_data->dim, i));
	      float_p = (void *) &temp;
	      AMITK_RAW_DATA_FLOAT_SET_CONTENT(raw_data,i) = *float_p;
#else /* G_LITTLE_ENDIAN */
	      AMITK_RAW_DATA_FLOAT_SET_CONTENT(raw_data,i) = DATA_CONTENT(data, raw_data->dim, i);
#endif
	    }
	}
	break;




      case AMITK_RAW_FORMAT_SINT_LE:
	{
	  gint32 * data=file_buffer;
	  
	  /* copy this frame into the data set */
	  for (i.y = 0; i.y < raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data->dim.x; i.x++)
	      AMITK_RAW_DATA_SINT_SET_CONTENT(raw_data,i) =
		GINT32_FROM_LE(DATA_CONTENT(data, raw_data->dim, i));
	  
	}
	break;
      case AMITK_RAW_FORMAT_UINT_LE:
	{
	  guint32 * data=file_buffer;
	  
	  /* copy this frame into the data set */
	  for (i.y = 0; i.y < raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data->dim.x; i.x++) {
	      /* keep compiler from generating bad code with a noop,occurs with gcc 3.0.3 */
	      if (i.x == -1) g_print("no_op\n");

	      AMITK_RAW_DATA_UINT_SET_CONTENT(raw_data,i) = 
		GUINT32_FROM_LE(DATA_CONTENT(data, raw_data->dim, i));
	    }
	}
	break;
      case AMITK_RAW_FORMAT_SSHORT_LE:
	{
	  gint16 * data=file_buffer;
	  
	  /* copy this frame into the data set */
	  for (i.y = 0; i.y < raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data->dim.x; i.x++)
	      AMITK_RAW_DATA_SSHORT_SET_CONTENT(raw_data,i) =
		GINT16_FROM_LE(DATA_CONTENT(data, raw_data->dim, i));

	}
	break;
      case AMITK_RAW_FORMAT_USHORT_LE:
	{
	  guint16 * data=file_buffer;
	  
	  /* copy this frame into the data set */
	  for (i.y = 0; i.y < raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data->dim.x; i.x++)
	      AMITK_RAW_DATA_USHORT_SET_CONTENT(raw_data,i) =
		GUINT16_FROM_LE(DATA_CONTENT(data, raw_data->dim, i));

	}
	break;
      case AMITK_RAW_FORMAT_SBYTE_NE:
	{
	  gint8 * data=file_buffer;
	  
	  /* copy this frame into the data set */
	  for (i.y = 0; i.y < raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data->dim.x; i.x++)
	      AMITK_RAW_DATA_SBYTE_SET_CONTENT(raw_data,i) = DATA_CONTENT(data, raw_data->dim, i);
	}
	break;
      case AMITK_RAW_FORMAT_UBYTE_NE:
      default:
	{
	  guint8 * data=file_buffer;
	  
	  /* copy this frame into the data set */
	  for (i.y = 0; i.y < raw_data->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data->dim.x; i.x++)
	      AMITK_RAW_DATA_UBYTE_SET_CONTENT(raw_data,i) = DATA_CONTENT(data, raw_data->dim, i);
	}
	break;
      }   
    }
  }

  fclose(file_pointer);

  /* garbage collection */
  g_free(file_buffer);
  
  return raw_data;

  error:

  if (raw_data != NULL)
    g_object_unref(raw_data);

  if (file_pointer != NULL)
    fclose(file_pointer);

  if (file_buffer != NULL)
    g_free(file_buffer);

  return NULL;
}


/* function to write out the information content of a raw_data set into an xml
   file.  Returns a string containing the name of the file. */
gchar * amitk_raw_data_write_xml(AmitkRawData * raw_data, const gchar * name) {

  gchar * xml_filename;
  gchar * raw_filename;
  guint count;
  struct stat file_info;
  xmlDocPtr doc;
  FILE * file_pointer;

  /* make a guess as to our filename */
  count = 1;
  xml_filename = g_strdup_printf("%s.xml", name);
  raw_filename = g_strdup_printf("%s.dat", name);

  /* see if this file already exists */
  while ((stat(xml_filename, &file_info) == 0) ||(stat(raw_filename, &file_info) == 0)) {
    g_free(xml_filename);
    g_free(raw_filename);
    count++;
    xml_filename = g_strdup_printf("%s_%d.xml", name, count);
    raw_filename = g_strdup_printf("%s_%d.dat", name, count);
  }

  /* and we now have unique filenames */
#ifdef AMIDE_DEBUG
  g_print("\t- saving raw data in file %s\n",xml_filename);
#endif

  /* write the xml file */
  doc = xmlNewDoc("1.0");
  doc->children = xmlNewDocNode(doc, NULL, "raw_data", name);
  amitk_voxel_write_xml(doc->children, "dim", raw_data->dim);
  xml_save_string(doc->children,"raw_format", 
		  amitk_raw_format_names[amitk_format_to_raw_format(raw_data->format)]);

  /* store the name of our associated data file */
  xml_save_string(doc->children, "raw_data_file", raw_filename);

  /* and save */
  xmlSaveFile(xml_filename, doc);

  /* and we're done with the xml stuff*/
  xmlFreeDoc(doc);

  /* now to save the raw data */

  /* write it on out */
  if ((file_pointer = fopen(raw_filename, "w")) == NULL) {
    g_warning("couldn't save raw data file: %s",raw_filename);
    g_free(xml_filename);
    g_free(raw_filename);
    return NULL;
  }

  if (fwrite(raw_data->data, 
	     amitk_format_sizes[AMITK_RAW_DATA_FORMAT(raw_data)],
	     amitk_raw_data_num_voxels(raw_data), 
	     file_pointer) != amitk_raw_data_num_voxels(raw_data)) {
    g_warning("incomplete save of raw data file: %s",raw_filename);
    fclose(file_pointer);
    g_free(xml_filename);
    g_free(raw_filename);
    return NULL;
  }
  fclose(file_pointer);

  g_free(raw_filename);
  return xml_filename;
}


/* function to load in a raw data xml file */
AmitkRawData * amitk_raw_data_read_xml(gchar * xml_filename) {

  xmlDocPtr doc;
  AmitkRawData * raw_data;
  xmlNodePtr nodes;
  AmitkRawFormat i_raw_format, raw_format;
  gchar * temp_string;
  gchar * raw_filename;
  AmitkVoxel dim;

  /* parse the xml file */
  if ((doc = xmlParseFile(xml_filename)) == NULL) {
    g_warning("Couldn't Parse AMIDE raw_data xml file %s",xml_filename);
    return NULL;
  }

  /* get the root of our document */
  if ((nodes = xmlDocGetRootElement(doc)) == NULL) {
    g_warning("Raw data xml file doesn't appear to have a root: %s", xml_filename);
    return NULL;
  }

  /* get the document tree */
  nodes = nodes->children;

  dim = amitk_voxel_read_xml(nodes, "dim");

  /* figure out the data format */
  temp_string = xml_get_string(nodes, "raw_format");
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
  raw_format = AMITK_RAW_FORMAT_DOUBLE_BE; /* sensible guess in case we don't figure it out from the file */
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
  raw_format = AMITK_RAW_FORMAT_DOUBLE_LE; /* sensible guess in case we don't figure it out from the file */
#endif
  if (temp_string != NULL)
    for (i_raw_format=0; i_raw_format < AMITK_RAW_FORMAT_NUM; i_raw_format++) 
      if (g_strcasecmp(temp_string, amitk_raw_format_names[i_raw_format]) == 0)
	raw_format = i_raw_format;
  g_free(temp_string);

  /* get the name of our associated data file */
  raw_filename = xml_get_string(nodes, "raw_data_file");

  /* now load in the raw data */
#ifdef AMIDE_DEBUG
  g_print("reading data from file %s\n", raw_filename);
#endif
  raw_data = amitk_raw_data_import_raw_file(raw_filename, raw_format, dim, 0);
   
  /* and we're done */
  g_free(raw_filename);
  xmlFreeDoc(doc);

  return raw_data;
}

amide_data_t amitk_raw_data_get_value(const AmitkRawData * rd, const AmitkVoxel i) {

  g_return_val_if_fail(AMITK_IS_RAW_DATA(rd), EMPTY);
  
  if (!amitk_raw_data_includes_voxel(rd, i)) return EMPTY;

  /* hand everything off to the data type specific function */
  switch(rd->format) {
  case AMITK_FORMAT_UBYTE:
    return AMITK_RAW_DATA_UBYTE_CONTENT(rd, i);
  case AMITK_FORMAT_SBYTE:
    return AMITK_RAW_DATA_SBYTE_CONTENT(rd, i);
  case AMITK_FORMAT_USHORT:
    return AMITK_RAW_DATA_USHORT_CONTENT(rd, i);
  case AMITK_FORMAT_SSHORT:
    return AMITK_RAW_DATA_SSHORT_CONTENT(rd, i);
  case AMITK_FORMAT_UINT:
    return AMITK_RAW_DATA_UINT_CONTENT(rd, i);
  case AMITK_FORMAT_SINT:
    return AMITK_RAW_DATA_SINT_CONTENT(rd, i);
  case AMITK_FORMAT_FLOAT:
    return AMITK_RAW_DATA_FLOAT_CONTENT(rd, i);
  case AMITK_FORMAT_DOUBLE:
    return AMITK_RAW_DATA_DOUBLE_CONTENT(rd, i);
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    return EMPTY;
  }
}




/* take in one of the raw data formats, and return the corresponding data format */
AmitkFormat amitk_raw_format_to_format(AmitkRawFormat raw_format) {

  AmitkFormat format;

  switch(raw_format) {

  case AMITK_RAW_FORMAT_UBYTE_NE: 
    format = AMITK_FORMAT_UBYTE;
    break;
  case AMITK_RAW_FORMAT_SBYTE_NE: 
    format = AMITK_FORMAT_SBYTE;
    break;
  case AMITK_RAW_FORMAT_USHORT_LE: 
  case AMITK_RAW_FORMAT_USHORT_BE: 
    format = AMITK_FORMAT_USHORT;
    break;
  case AMITK_RAW_FORMAT_SSHORT_LE:
  case AMITK_RAW_FORMAT_SSHORT_BE:
    format = AMITK_FORMAT_SSHORT;
    break;
  case AMITK_RAW_FORMAT_UINT_LE:
  case AMITK_RAW_FORMAT_UINT_BE:
  case AMITK_RAW_FORMAT_UINT_PDP:
    format = AMITK_FORMAT_UINT;
    break;
  case AMITK_RAW_FORMAT_SINT_LE:
  case AMITK_RAW_FORMAT_SINT_BE:
  case AMITK_RAW_FORMAT_SINT_PDP:
    format = AMITK_FORMAT_SINT;
    break;
  case AMITK_RAW_FORMAT_FLOAT_LE:
  case AMITK_RAW_FORMAT_FLOAT_BE:
  case AMITK_RAW_FORMAT_FLOAT_PDP:
#if (SIZE_OF_AMIDE_T == 4)
  case AMITK_RAW_FORMAT_ASCII_NE:
#endif
    format = AMITK_FORMAT_FLOAT;
    break;
  case AMITK_RAW_FORMAT_DOUBLE_LE:
  case AMITK_RAW_FORMAT_DOUBLE_BE:
#if (SIZE_OF_AMIDE_T == 8)
  case AMITK_RAW_FORMAT_ASCII_NE:
#endif
    format = AMITK_FORMAT_DOUBLE;
    break;
  default:
    g_warning("unexpected case in %s at line %d", __FILE__, __LINE__);
    format = AMITK_FORMAT_FLOAT; /* take a wild guess */
    break;
  }
   
  return format;
}



/* tells you what raw data format corresponds to the given data format */
AmitkRawFormat amitk_format_to_raw_format(AmitkFormat format) {

  AmitkRawFormat raw_format;

  switch(format) {

  case AMITK_FORMAT_UBYTE: 
    raw_format = AMITK_RAW_FORMAT_UBYTE_NE;
    break;
  case AMITK_FORMAT_SBYTE: 
    raw_format = AMITK_RAW_FORMAT_SBYTE_NE;
    break;
  case AMITK_FORMAT_USHORT: 
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
    raw_format = AMITK_RAW_FORMAT_USHORT_BE;
#else
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
    raw_format = AMITK_RAW_FORMAT_USHORT_LE;
#else
#error "must specify G_LITTLE_ENDIAN or G_BIG_ENDIAN)"
#endif
#endif
    break;
  case AMITK_FORMAT_SSHORT:
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
    raw_format = AMITK_RAW_FORMAT_SSHORT_BE;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
    raw_format = AMITK_RAW_FORMAT_SSHORT_LE;
#endif
    break;
  case AMITK_FORMAT_UINT:
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
    raw_format = AMITK_RAW_FORMAT_UINT_BE;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
    raw_format = AMITK_RAW_FORMAT_UINT_LE;
#endif
    break;
  case AMITK_FORMAT_SINT:
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
    raw_format = AMITK_RAW_FORMAT_SINT_BE;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
    raw_format = AMITK_RAW_FORMAT_SINT_LE;
#endif
    break;
  case AMITK_FORMAT_FLOAT:
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
    raw_format = AMITK_RAW_FORMAT_FLOAT_BE;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
    raw_format = AMITK_RAW_FORMAT_FLOAT_LE;
#endif
    break;
  case AMITK_FORMAT_DOUBLE:
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
    raw_format = AMITK_RAW_FORMAT_DOUBLE_BE;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
    raw_format = AMITK_RAW_FORMAT_DOUBLE_LE;
#endif
    break;
  default:
    g_warning("unexpected case in %s at line %d", __FILE__, __LINE__);
    raw_format = AMITK_RAW_FORMAT_UBYTE_NE; /* take a wild guess */
    break;
  }
   
  return raw_format;
}






