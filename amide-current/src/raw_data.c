/* raw_data.c
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
#include <gnome.h>
#include <matrix.h>
#include "raw_data.h"
#include <sys/stat.h>
 
/* external variables */        
gchar * raw_data_format_names[] = {"Unsigned Byte (8 bit)", \
				   "Signed Byte (8 bit)", \
				   "Unsigned Short, Little Endian (16 bit)", \
				   "Signed Short, Little Endian (16 bit)", \
				   "Unsigned Integer, Little Endian (32 bit)", \
				   "Signed Integer, Little Endian (32 bit)", \
				   "Float, Little Endian (32 bit)", \
				   "Double, Little Endian (64 bit)", \
				   "Unsigned Short, Big Endian (16 bit)", \
				   "Signed Short, Big Endian (16 bit)", \
				   "Unsigned Integer, Big Endian (32 bit)", \
				   "Signed Integer, Big Endian (32 bit)", \
				   "Float, Big Endian (32 bit)", \
				   "Double, Big Endian (64 bit)", \
				   "Unsigned Integer, PDP (32 bit)", \
				   "Signed Integer, PDP (32 bit)", \
				   "Float, PDP/VAX (32 bit)", \
				   "ASCII (8 bit)"};

guint raw_data_sizes[] = {sizeof(guint8),
			  sizeof(gint8),
			  sizeof(guint16),
			  sizeof(gint16),
			  sizeof(guint32),
			  sizeof(gint32),
			  sizeof(gfloat),
			  sizeof(gdouble),
			  sizeof(guint16),
			  sizeof(gint16),
			  sizeof(guint32),
			  sizeof(gint32),
			  sizeof(gfloat),
			  sizeof(gdouble),
			  sizeof(guint32),
			  sizeof(gint32),
			  sizeof(gfloat),
			  1};



/* take in one of the raw data formats, and return the corresponding data format */
data_format_t raw_data_format_data(raw_data_format_t raw_data_format) {

  data_format_t data_format;

  switch(raw_data_format) {

  case UBYTE_NE: 
    {
      data_format = UBYTE;
      break;
    }
  case SBYTE_NE: 
    {
      data_format = SBYTE;
      break;
    }
  case USHORT_LE: 
  case USHORT_BE: 
    {
      data_format = USHORT;
      break;
    }
  case SSHORT_LE:
  case SSHORT_BE:
    {
      data_format = SSHORT;
      break;
    }
  case UINT_LE:
  case UINT_BE:
  case UINT_PDP:
    {
      data_format = UINT;
      break;
    }
  case SINT_LE:
  case SINT_BE:
  case SINT_PDP:
    {
      data_format = SINT;
      break;
    }
  case FLOAT_LE:
  case FLOAT_BE:
  case FLOAT_PDP:
    {
      data_format = FLOAT;
      break;
    }
  case DOUBLE_LE:
  case DOUBLE_BE:
  case ASCII_NE:
    {
      data_format = DOUBLE;
      break;
    }
  default:
    {
      g_warning("%s: unexpected case in %s at line %d", PACKAGE, __FILE__, __LINE__);
      data_format = FLOAT; /* take a wild guess */
      break;
    }
  }
   
  return data_format;
}



/* take in one of the data formats, and return the corresponding raw data format's */
raw_data_format_t raw_data_format_raw(data_format_t data_format) {

  raw_data_format_t raw_data_format;

  switch(data_format) {

  case UBYTE: 
    {
      raw_data_format = UBYTE_NE;
      break;
    }
  case SBYTE: 
    {
      raw_data_format = SBYTE_NE;
      break;
    }
  case USHORT: 
    {
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
      raw_data_format = USHORT_BE;
#else
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
      raw_data_format = USHORT_LE;
#else
#error "must specify G_LITTLE_ENDIAN or G_BIG_ENDIAN)"
#endif
#endif
      break;
    }
  case SSHORT:
    {
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
      raw_data_format = SSHORT_BE;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
      raw_data_format = SSHORT_LE;
#endif
      break;
    }
  case UINT:
    {
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
      raw_data_format = UINT_BE;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
      raw_data_format = UINT_LE;
#endif
      break;
    }
  case SINT:
    {
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
      raw_data_format = SINT_BE;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
      raw_data_format = SINT_LE;
#endif
      break;
    }
  case FLOAT:
    {
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
      raw_data_format = FLOAT_BE;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
      raw_data_format = FLOAT_LE;
#endif
      break;
    }
  case DOUBLE:
    {
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
      raw_data_format = DOUBLE_BE;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
      raw_data_format = DOUBLE_LE;
#endif
      break;
    }
  default:
    {
      g_warning("%s: unexpected case in %s at line %d", PACKAGE, __FILE__, __LINE__);
      raw_data_format = UBYTE_NE; /* take a wild guess */
      break;
    }
  }
   
  return raw_data_format;
}



/* calculate the number of bytes that will be read from the file*/
guint raw_data_calc_num_bytes(voxelpoint_t dim, raw_data_format_t raw_data_format) {

  return (dim.x * dim.y * dim.z * dim.t *  raw_data_sizes[raw_data_format]);

}




/* reads the contents of a raw data file into an amide data_set structure,
   note: raw_data_set should be pre initialized with the appropriate dim
         and conversion,  but format will be filled in 
   note: file_offset is bytes for a binary file, lines for an ascii file
*/
data_set_t * raw_data_read_file(const gchar * file_name, 
				data_set_t * raw_data_set,
				raw_data_format_t raw_data_format,
				guint file_offset) {

  FILE * file_pointer;
  void * file_buffer=NULL;
  size_t bytes_read;
  size_t bytes_to_read;
  voxelpoint_t i;
  gint error, j;

  if (raw_data_set == NULL) {
    g_warning("%s: raw_data_read called with NULL data_set",PACKAGE);
    return raw_data_set;
  }

  if (file_name == NULL) {
    g_warning("%s: raw_data_read called with no filename",PACKAGE);
    return data_set_free(raw_data_set);
  }

  /* figure out what internal data format we should be using */
  raw_data_set->format =  raw_data_format_data(raw_data_format);
  
  /* malloc the space for the data set */
  g_free(raw_data_set->data); /* just in case */
  if ((raw_data_set->data = data_set_get_data_mem(raw_data_set)) == NULL) {
    g_warning("%s: couldn't allocate space for the raw data",PACKAGE);
    return data_set_free(raw_data_set);
  }

  /* open the raw data file for reading */
  if (raw_data_format == ASCII_NE) {
    if ((file_pointer = fopen(file_name, "r")) == NULL) {
      g_warning("%s: couldn't open raw data file %s", PACKAGE,file_name);
      return data_set_free(raw_data_set);
    }
  } else { /* note, rb==r on any POSIX compliant system (i.e. Linux). */
    if ((file_pointer = fopen(file_name, "rb")) == NULL) {
      g_warning("%s: couldn't open raw data file %s", PACKAGE,file_name);
      return data_set_free(raw_data_set);
    }
  }
  
  /* jump forward by the given offset */
  if (raw_data_format == ASCII_NE) {
    for (j=0; j<file_offset; j++)
      if ((error = fscanf(file_pointer, "%*f")) < 0) { /*EOF is usually -1 */
	g_warning("%s: could not step forward %d elements in raw data file:\n\t%s\n\treturned error: %d",
		  PACKAGE, j+1, file_name, error);
	fclose(file_pointer);
	return data_set_free(raw_data_set);
    }
  } else { /* binary */
    if (fseek(file_pointer, file_offset, SEEK_SET) != 0) {
      g_warning("%s: could not seek forward %d bytes in raw data file:\n\t%s",
		PACKAGE, file_offset, file_name);
      fclose(file_pointer);
      return data_set_free(raw_data_set);
    }
  }
    
  /* read in the contents of the file */
  if (raw_data_format == ASCII_NE) {
    ; /* this case is handled in the loop below */
  } else {
    bytes_to_read = raw_data_calc_num_bytes(raw_data_set->dim, raw_data_format);
    file_buffer = (void *) g_malloc(bytes_to_read);
    bytes_read = fread(file_buffer, 1, bytes_to_read, file_pointer );
    if (bytes_read != bytes_to_read) {
      g_warning("%s: read wrong number of elements from raw data file:\n\t%s\n\texpected %d\tgot %d", 
		PACKAGE,file_name, bytes_to_read, bytes_read);
      g_free(file_buffer);
      fclose(file_pointer);
      return data_set_free(raw_data_set);
    }
    fclose(file_pointer);
  }
  
  /* iterate over the # of frames */
  for (i.t = 0; i.t < raw_data_set->dim.t; i.t++) {
#ifdef AMIDE_DEBUG
    g_print("\tloading frame %d\n",i.t);
#endif
    
    /* and convert the data */
    switch (raw_data_format) {

    case ASCII_NE:
      error = 1;

      /* copy this frame into the data set */
      for (i.z = 0; (i.z < raw_data_set->dim.z) && (error >= 0) ; i.z++) {
	for (i.y = 0; (i.y < raw_data_set->dim.y) && (error >= 0); i.y++) {
	  for (i.x = 0; (i.x < raw_data_set->dim.x) && (error >= 0); i.x++) {
#if (SIZE_OF_AMIDE_DATA_T == 8)
	    /* convert to doubles */
	    error = fscanf(file_pointer, "%lf", DATA_SET_FLOAT_POINTER(raw_data_set,i));
#elif (SIZE_OF_AMIDE_DATA_T == 4)
	    /* convert to float */
	    error = fscanf(file_pointer, "%f", DATA_SET_FLOAT_POINTER(raw_data_set,i));
#else
#error "Unknown size for SIZE_OF_FLOATPOINT_T"
#endif
	    if (error == 0) error = EOF; /* if we couldn't read, may as well be EOF*/
	  }
	}
      }

      fclose(file_pointer);
      if (error < 0) { /* EOF = -1 (usually) */
	g_warning("%s: could not read ascii file after %d elements, file or parameters are erroneous:\n\t%s",
		  PACKAGE, 
		  i.x + 
		  raw_data_set->dim.x*i.y +
		  raw_data_set->dim.x*raw_data_set->dim.y*i.z +
		  raw_data_set->dim.x*raw_data_set->dim.y*raw_data_set->dim.z*i.t,
		  file_name);
      }

      break;
	
    case FLOAT_PDP:
      {
	guint32 * data = file_buffer;
	guint32 temp;
	gfloat * float_p;

	/* copy this frame into the data set */
	for (i.z = 0; i.z < raw_data_set->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_set->dim.x; i.x++) {
	      temp = GUINT32_FROM_PDP(RAW_DATA_CONTENT(data, raw_data_set->dim, i));
	      float_p = (void *) &temp;
	      DATA_SET_FLOAT_SET_CONTENT(raw_data_set,i) = *float_p;
	    }
      }
      break;
    case SINT_PDP:
      {
	gint32 * data=file_buffer;
	  
	/* copy this frame into the data_set */
	for (i.z = 0; i.z < raw_data_set->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_set->dim.x; i.x++)
	      DATA_SET_SINT_SET_CONTENT(raw_data_set,i) = 
		GINT32_FROM_PDP(RAW_DATA_CONTENT(data, raw_data_set->dim, i));
      }
      break;
    case UINT_PDP:
      {
	guint32 * data=file_buffer;
	  
	/* copy this frame into the data set */
	for (i.z = 0; i.z < raw_data_set->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_set->dim.x; i.x++)
	      DATA_SET_UINT_SET_CONTENT(raw_data_set,i) = 
		GUINT32_FROM_PDP(RAW_DATA_CONTENT(data, raw_data_set->dim, i));
      }
      break;

    case DOUBLE_BE: 
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
	for (i.z = 0; i.z < raw_data_set->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_set->dim.x; i.x++) {
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
	      temp = 
		GUINT64_FROM_BE(RAW_DATA_CONTENT(data, raw_data_set->dim, i));
	      double_p = (void *) &temp;
	      DATA_SET_DOUBLE_SET_CONTENT(raw_data_set,i) = *double_p;
#else /* G_BIG_ENDIAN */
	      DATA_SET_DOUBLE_SET_CONTENT(raw_data_set,i) = RAW_DATA_CONTENT(data, raw_data_set->dim, i);
#endif
              }
      }
      break;
    case FLOAT_BE:
      {
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
	guint32 * data = file_buffer;
	guint32 temp;
	gfloat * float_p;
#else /* (G_BYTE_ORDER == G_BIG_ENDIAN) */
	gfloat * data = file_buffer;
#endif

	/* copy this frame into the data set */
	for (i.z = 0; i.z < raw_data_set->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_set->dim.x; i.x++) {
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
	      temp = GUINT32_FROM_BE(RAW_DATA_CONTENT(data, raw_data_set->dim, i));
	      float_p = (void *) &temp;
	      DATA_SET_FLOAT_SET_CONTENT(raw_data_set,i) = *float_p;
#else /* G_BIG_ENDIAN */
	      DATA_SET_FLOAT_SET_CONTENT(raw_data_set,i) = RAW_DATA_CONTENT(data, raw_data_set->dim, i);
#endif
	    }
      }
      break;
    case SINT_BE:
      {
	gint32 * data=file_buffer;
	  
	/* copy this frame into the data_set */
	for (i.z = 0; i.z < raw_data_set->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_set->dim.x; i.x++)
	      DATA_SET_SINT_SET_CONTENT(raw_data_set,i) = 
		GINT32_FROM_BE(RAW_DATA_CONTENT(data, raw_data_set->dim, i));
      }
      break;
    case UINT_BE:
      {
	guint32 * data=file_buffer;
	  
	/* copy this frame into the data set */
	for (i.z = 0; i.z < raw_data_set->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_set->dim.x; i.x++)
	      DATA_SET_UINT_SET_CONTENT(raw_data_set,i) = 
		GUINT32_FROM_BE(RAW_DATA_CONTENT(data, raw_data_set->dim, i));
      }
      break;
    case SSHORT_BE:
      {
	gint16 * data=file_buffer;
	  
	/* copy this frame into the data set */
	for (i.z = 0; i.z < raw_data_set->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_set->dim.x; i.x++)
	      DATA_SET_SSHORT_SET_CONTENT(raw_data_set,i) = 
		GINT16_FROM_BE(RAW_DATA_CONTENT(data, raw_data_set->dim, i));
      }
      break;
    case USHORT_BE:
      {
	guint16 * data=file_buffer;
	  
	/* copy this frame into the data set */
	for (i.z = 0; i.z < raw_data_set->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_set->dim.x; i.x++)
	      DATA_SET_USHORT_SET_CONTENT(raw_data_set,i) =
		GUINT16_FROM_BE(RAW_DATA_CONTENT(data, raw_data_set->dim, i));
      }
      break;




    case DOUBLE_LE:
      {
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
	guint64 * data = file_buffer;
	guint64 temp;
	gdouble * double_p;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
	gdouble * data = file_buffer;
#endif

	/* copy this frame into the data set */
	for (i.z = 0; i.z < raw_data_set->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_set->dim.x; i.x++) {
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
	      temp = GUINT64_FROM_LE(RAW_DATA_CONTENT(data, raw_data_set->dim, i));
	      double_p = (void *) &temp;
	      DATA_SET_DOUBLE_SET_CONTENT(raw_data_set,i) = *double_p;
#else /* G_LITTLE_ENDIAN */
	      DATA_SET_DOUBLE_SET_CONTENT(raw_data_set,i) = RAW_DATA_CONTENT(data, raw_data_set->dim, i);
#endif
               }
      }
      break;




    case FLOAT_LE:
      {
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
	guint32 * data = file_buffer;
	guint32 temp;
	gfloat * float_p;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
	gfloat * data = file_buffer;
#endif

	/* copy this frame into the data set */
	for (i.z = 0; i.z < raw_data_set->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_set->dim.x; i.x++) {
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
	      temp = GUINT32_FROM_LE(RAW_DATA_CONTENT(data, raw_data_set->dim, i));
	      float_p = (void *) &temp;
	      DATA_SET_FLOAT_SET_CONTENT(raw_data_set,i) = *float_p;
#else /* G_LITTLE_ENDIAN */
	      DATA_SET_FLOAT_SET_CONTENT(raw_data_set,i) = RAW_DATA_CONTENT(data, raw_data_set->dim, i);

#endif
	    }
      }
      break;




    case SINT_LE:
      {
	gint32 * data=file_buffer;
	  
	/* copy this frame into the data set */
	for (i.z = 0; i.z < raw_data_set->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_set->dim.x; i.x++)
	      DATA_SET_SINT_SET_CONTENT(raw_data_set,i) =
		GINT32_FROM_LE(RAW_DATA_CONTENT(data, raw_data_set->dim, i));
	  
      }
      break;
    case UINT_LE:
      {
	guint32 * data=file_buffer;

	/* copy this frame into the data set */
	for (i.z = 0; i.z < raw_data_set->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_set->dim.x; i.x++)
	      DATA_SET_UINT_SET_CONTENT(raw_data_set,i) = 
		GUINT32_FROM_LE(RAW_DATA_CONTENT(data, raw_data_set->dim, i));
      }
      break;
    case SSHORT_LE:
      {
	gint16 * data=file_buffer;
	  
	/* copy this frame into the data set */
	for (i.z = 0; i.z < raw_data_set->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_set->dim.x; i.x++)
	      DATA_SET_SSHORT_SET_CONTENT(raw_data_set,i) =
		GINT16_FROM_LE(RAW_DATA_CONTENT(data, raw_data_set->dim, i));

      }
      break;
    case USHORT_LE:
      {
	guint16 * data=file_buffer;
	  
	/* copy this frame into the data set */
	for (i.z = 0; i.z < raw_data_set->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_set->dim.x; i.x++)
	      DATA_SET_USHORT_SET_CONTENT(raw_data_set,i) =
		GUINT16_FROM_LE(RAW_DATA_CONTENT(data, raw_data_set->dim, i));

      }
      break;
    case SBYTE_NE:
      {
	gint8 * data=file_buffer;
	  
	/* copy this frame into the data set */
	for (i.z = 0; i.z < raw_data_set->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_set->dim.x; i.x++)
	      DATA_SET_SBYTE_SET_CONTENT(raw_data_set,i) = RAW_DATA_CONTENT(data, raw_data_set->dim, i);
      }
      break;
    case UBYTE_NE:
    default:
      {
	guint8 * data=file_buffer;
	  
	/* copy this frame into the data set */
	for (i.z = 0; i.z < raw_data_set->dim.z ; i.z++) 
	  for (i.y = 0; i.y < raw_data_set->dim.y; i.y++) 
	    for (i.x = 0; i.x < raw_data_set->dim.x; i.x++)
	      DATA_SET_UBYTE_SET_CONTENT(raw_data_set,i) = RAW_DATA_CONTENT(data, raw_data_set->dim, i);
      }
      break;
    }   
  }      

  /* garbage collection */
  g_free(file_buffer);
  
  return raw_data_set;
}






/* reads the contents of a raw data file into an amide volume data structure,
   note: raw_data_volume->data_set should be pre initialized with the appropriate dim 
   note: conversion factors should also be appropriately pre-initialized if needed
   note: if raw_data_volume->frame_duration is null, fake values will be entered
   note: file_offset is bytes for a binary file, lines for an ascii file
*/
volume_t * raw_data_read_volume(const gchar * file_name, 
				volume_t * raw_data_volume,
				raw_data_format_t raw_data_format,
				guint file_offset) {

  guint t;

  if (raw_data_volume == NULL) {
    g_warning("%s: raw_data_read called with NULL volume",PACKAGE);
    return raw_data_volume;
  }

  /* read in the data set */
  raw_data_volume->data_set = 
    raw_data_read_file(file_name, raw_data_volume->data_set, raw_data_format, file_offset);
  if (raw_data_volume->data_set == NULL) {
    g_warning("%s: raw_data_read_file failed returning NULL data set",PACKAGE);
    return raw_data_volume;
  }


  /* put in fake frame durations if needed */
  if (raw_data_volume->frame_duration == NULL) {
  
    /* allocate space for the array containing info on the duration of the frames */
    if ((raw_data_volume->frame_duration = 
	 volume_get_frame_duration_mem(raw_data_volume)) == NULL) {
      g_warning("%s: couldn't allocate space for the frame duration info",PACKAGE);
      raw_data_volume = volume_free(raw_data_volume);
      return raw_data_volume;
    }

    /* iterate over the # of frames */
    for (t = 0; t < raw_data_volume->data_set->dim.t; t++)
      raw_data_volume->frame_duration[t] = 1.0;
  }

  /* set the max/min values in the volume */
  volume_recalc_max_min(raw_data_volume);

  return raw_data_volume;
}


