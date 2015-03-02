/* amitk_raw_data.c
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

#include <sys/stat.h>
#include <stdio.h>

#include "amitk_raw_data.h"
#include "amitk_marshal.h"
#include "amitk_type_builtins.h"

#define DATA_CONTENT(data, dim, voxel) ((data)[(voxel).x + (dim).x*(voxel).y])

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

gboolean amitk_format_signed[AMITK_FORMAT_NUM] = {
  FALSE,
  TRUE,
  FALSE,
  TRUE,
  FALSE,
  TRUE,
  TRUE,
  TRUE
};

gchar * amitk_format_names[AMITK_FORMAT_NUM] = {
  N_("Unsigned Byte (8 bit)"), 
  N_("Signed Byte (8 bit)"), 
  N_("Unsigned Short (16 bit)"), 
  N_("Signed Short (16 bit)"), 
  N_("Unsigned Integer (32 bit)"), 
  N_("Signed Integer (32 bit)"), 
  N_("Float (32 bit)"), 
  N_("Double (64 bit)")
};

amide_data_t amitk_format_max[AMITK_FORMAT_NUM] = {
  255.0,
  127.0,
  G_MAXUSHORT,
  G_MAXSHORT,
  G_MAXUINT,
  G_MAXINT,
  G_MAXFLOAT,
  G_MAXDOUBLE
};

amide_data_t amitk_format_min[AMITK_FORMAT_NUM] = {
  0.0,
  -128.0,
  0.0,
  G_MINSHORT,
  0.0,
  G_MININT,
  -G_MAXFLOAT,
  -G_MAXDOUBLE
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


/* what used to be saved in the file */
gchar * amitk_raw_format_legacy_names[] = {
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

/* what's shown to the user - can be translated */
gchar * amitk_raw_format_names[] = {
  N_("Unsigned Byte (8 bit)"), 
  N_("Signed Byte (8 bit)"), 
  N_("Unsigned Short, Little Endian (16 bit)"), 
  N_("Signed Short, Little Endian (16 bit)"), 
  N_("Unsigned Integer, Little Endian (32 bit)"), 
  N_("Signed Integer, Little Endian (32 bit)"), 
  N_("Float, Little Endian (32 bit)"), 
  N_("Double, Little Endian (64 bit)"), 
  N_("Unsigned Short, Big Endian (16 bit)"), 
  N_("Signed Short, Big Endian (16 bit)"), 
  N_("Unsigned Integer, Big Endian (32 bit)"), 
  N_("Signed Integer, Big Endian (32 bit)"), 
  N_("Float, Big Endian (32 bit)"), 
  N_("Double, Big Endian (64 bit)"), 
  N_("Unsigned Integer, PDP (32 bit)"), 
  N_("Signed Integer, PDP (32 bit)"), 
  N_("Float, PDP/VAX (32 bit)"), 
  N_("ASCII (8 bit)")
};


//enum {
//  LAST_SIGNAL
//};

static void raw_data_class_init          (AmitkRawDataClass *klass);
static void raw_data_init                (AmitkRawData      *object);
static void raw_data_finalize            (GObject           *object);
static GObjectClass * parent_class;
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
    //g_print("\tfreeing raw data\n");
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



AmitkRawData* amitk_raw_data_new_with_data(AmitkFormat format, AmitkVoxel dim) {

  AmitkRawData * raw_data;
  
  raw_data = amitk_raw_data_new();
  g_return_val_if_fail(raw_data != NULL, NULL);
 
  raw_data->format = format;
  raw_data->dim = dim;

  /* allocate the space for the data */
  raw_data->data = amitk_raw_data_get_data_mem(raw_data);
  if (raw_data->data == NULL) {
    g_object_unref(raw_data);
    return NULL;
  }

  return raw_data;
}


/* initalize a 2D slice, with values initialized to 0 */
AmitkRawData * amitk_raw_data_new_2D_with_data0(AmitkFormat format, amide_intpoint_t y_dim, amide_intpoint_t x_dim) {

  AmitkRawData * raw_data;
  AmitkVoxel dim;

  dim.x = x_dim;
  dim.y = y_dim;
  dim.z = dim.g = dim.t = 1;
  raw_data = amitk_raw_data_new_with_data0(format, dim);
  g_return_val_if_fail(raw_data != NULL, NULL);

  return raw_data;
}


/* initalize a 2D slice, with values initialized to 0 */
AmitkRawData * amitk_raw_data_new_3D_with_data0(AmitkFormat format, amide_intpoint_t z_dim, amide_intpoint_t y_dim, amide_intpoint_t x_dim) {

  AmitkRawData * raw_data;
  AmitkVoxel dim;

  dim.x = x_dim;
  dim.y = y_dim;
  dim.z = z_dim;
  dim.g = dim.t = 1;
  raw_data = amitk_raw_data_new_with_data0(format, dim);
  g_return_val_if_fail(raw_data != NULL, NULL);

  return raw_data;
}



/* same as amitk_raw_data_new_with_data, except allocated data memory is initialized to 0 */
AmitkRawData* amitk_raw_data_new_with_data0(AmitkFormat format, AmitkVoxel dim) {

  AmitkRawData * raw_data;
  
  raw_data = amitk_raw_data_new();
  g_return_val_if_fail(raw_data != NULL, NULL);
 
  raw_data->format = format;
  raw_data->dim = dim;

  /* allocate the space for the data */
  raw_data->data = amitk_raw_data_get_data_mem0(raw_data);
  if (raw_data->data == NULL) {
    g_object_unref(raw_data);
    return NULL;
  }

  return raw_data;
}




/* reads the contents of a raw data file into an amide raw data structure,

   notes: 

   1. file_offset is bytes for a binary file, lines for an ascii file
   2. either file_name, of existing_file need to be specified.  
      If existing_file is not being used, it must be NULL
*/
AmitkRawData * amitk_raw_data_import_raw_file(const gchar * file_name, 
					      FILE * existing_file,
					      AmitkRawFormat raw_format,
					      AmitkVoxel dim,
					      long file_offset,
					      AmitkUpdateFunc update_func,
					      gpointer update_data) {

  FILE * new_file_pointer=NULL;
  FILE * file_pointer=NULL;
  void * file_buffer=NULL;
  size_t bytes_per_slice=0;
  size_t bytes_read;
  AmitkVoxel i;
  gint error_code, j;
  AmitkRawData * raw_data=NULL;;
  gchar * temp_string;
  gint total_planes;
  gint i_plane;
  div_t x;
  gint divider;
  gboolean continue_work = TRUE;

  g_return_val_if_fail((file_name != NULL) || (existing_file != NULL), NULL);

  if (update_func != NULL) {
    temp_string = g_strdup_printf(_("Reading: %s"), (file_name != NULL) ? file_name : "raw data");
    continue_work = (*update_func)(update_data, temp_string, (gdouble) 0.0);
    g_free(temp_string);
  }
  total_planes = dim.z*dim.t*dim.g;
  divider = ((total_planes/AMITK_UPDATE_DIVIDER) < 1) ? 1 : (total_planes/AMITK_UPDATE_DIVIDER);

  raw_data = amitk_raw_data_new_with_data(amitk_raw_format_to_format(raw_format), dim);
  if (raw_data == NULL) {
    g_warning(_("couldn't allocate memory space for the raw data set structure"));
    goto error_condition;
  }

  /* open the raw data file for reading */
  if (existing_file == NULL) {
    if (raw_format == AMITK_RAW_FORMAT_ASCII_8_NE) {
      if ((new_file_pointer = fopen(file_name, "r")) == NULL) {
	g_warning(_("couldn't open raw data file %s"), file_name);
	goto error_condition;
      }
    } else { /* note, rb==r on any POSIX compliant system (i.e. Linux). */
      if ((new_file_pointer = fopen(file_name, "rb")) == NULL) {
	g_warning(_("couldn't open raw data file %s"), file_name);
	goto error_condition;
      }
    }
    file_pointer = new_file_pointer;
  } else {
    file_pointer = existing_file;
  }
  
  /* jump forward by the given offset */
  if (raw_format == AMITK_RAW_FORMAT_ASCII_8_NE) {
    for (j=0; j<file_offset; j++)
      if ((error_code = fscanf(file_pointer, "%*f")) < 0) { /*EOF is usually -1 */
	g_warning(_("could not step forward %d elements in raw data file:\n\treturned error: %d"),
		  j+1, error_code);
	goto error_condition;
    }
  } else { /* binary */
    if (fseek(file_pointer, file_offset, SEEK_SET) != 0) {
      g_warning(_("could not seek forward %ld bytes in raw data file"),file_offset);
      goto error_condition;
    }
  }
    
  /* read in the contents of the file */
  if (raw_format != AMITK_RAW_FORMAT_ASCII_8_NE) { /* ASCII handled in the loop below */
    bytes_per_slice = amitk_raw_format_calc_num_bytes_per_slice(dim, raw_format);
    if ((file_buffer = (void *) g_try_malloc(bytes_per_slice)) == NULL) {
      g_warning(_("couldn't malloc %zd bytes for file buffer\n"), bytes_per_slice);
      goto error_condition;
    }
  }

  /* iterate over the # of frames */
  error_code = 1;
  i_plane=0;
  for (i.t = 0; (i.t < dim.t) && (error_code >= 0) && (continue_work); i.t++) {
    for (i.g = 0; (i.g < dim.g) && (error_code >= 0) && (continue_work); i.g++) {
      for (i.z = 0; (i.z < dim.z) && (error_code >= 0) && (continue_work);  i.z++, i_plane++) {

	if (update_func != NULL) {
	  
	  x = div(i_plane,divider);
	  if (x.rem == 0)
	    continue_work = (*update_func)(update_data, NULL, ((gdouble) i_plane)/((gdouble) total_planes));
	}

	/* read in the contents of the file */
	if (raw_format != AMITK_RAW_FORMAT_ASCII_8_NE) { /* ASCII handled in the loop below */
	  bytes_read = fread(file_buffer, 1, bytes_per_slice, file_pointer );
	  if (bytes_read != bytes_per_slice) {
	    g_warning(_("read wrong # of elements from raw data, expected %zd, got %zd"), bytes_per_slice, bytes_read);
	    goto error_condition;
	  }
	}
	
	
	/* and convert the data */
	switch (raw_format) {
	  
	case AMITK_RAW_FORMAT_ASCII_8_NE:
	  
	  /* copy this frame into the data set */
	  i.x = 0;
	  for (i.y = 0; (i.y < dim.y) && (error_code >= 0); i.y++) {
	    for (i.x = 0; (i.x < dim.x) && (error_code >= 0); i.x++) {
	      if (raw_data->format == AMITK_FORMAT_DOUBLE)
		error_code = fscanf(file_pointer, "%lf", AMITK_RAW_DATA_DOUBLE_POINTER(raw_data,i));
	      else // (raw_data->format == FLOAT)
		error_code = fscanf(file_pointer, "%f", AMITK_RAW_DATA_FLOAT_POINTER(raw_data,i));
	      if (error_code == 0) error_code = EOF; /* if we couldn't read, may as well be EOF*/
	    }
	  }
	  
	  if (error_code < 0) { /* EOF = -1 (usually) */
	    g_warning(_("could not read ascii file after %d elements, file or parameters are erroneous"),
		      i.x + 
		      dim.x*i.y +
		      dim.x*dim.y*i_plane);
	    goto error_condition;
	  }
	
	  break;
	
	case AMITK_RAW_FORMAT_FLOAT_32_PDP:
	  {
	    guint32 * data = file_buffer;
	    guint32 temp;
	    gfloat * float_p;
	    
	    /* copy this frame into the data set */
	    for (i.y = 0; i.y < dim.y; i.y++) 
	      for (i.x = 0; i.x < dim.x; i.x++) {
		/* keep compiler from generating bad code with a noop,occurs with gcc 3.0.3 */
		if (i.x == -1) g_print("no_op\n");
		
		temp = GUINT32_FROM_PDP(DATA_CONTENT(data, dim,i));
		float_p = (void *) &temp;
		AMITK_RAW_DATA_FLOAT_SET_CONTENT(raw_data,i) = *float_p;
	      }
	  }
	  break;
	case AMITK_RAW_FORMAT_SINT_32_PDP:
	  {
	    gint32 * data=file_buffer;
	    
	    /* copy this frame into the data set */
	    for (i.y = 0; i.y < dim.y; i.y++) 
	      for (i.x = 0; i.x < dim.x; i.x++)
		AMITK_RAW_DATA_SINT_SET_CONTENT(raw_data,i) = 
		  GINT32_FROM_PDP(DATA_CONTENT(data, dim,i));
	  }
	  break;
	case AMITK_RAW_FORMAT_UINT_32_PDP:
	  {
	    guint32 * data=file_buffer;
	    
	    /* copy this frame into the data set */
	    for (i.y = 0; i.y < dim.y; i.y++) 
	      for (i.x = 0; i.x < dim.x; i.x++) {
		/* keep compiler from generating bad code with a noop,occurs with gcc 3.0.3 */
		if (i.x == -1) g_print("no_op\n");
		
		AMITK_RAW_DATA_UINT_SET_CONTENT(raw_data,i) = 
		  GUINT32_FROM_PDP(DATA_CONTENT(data, dim,i));
	      }
	  }
	  break;
	  
	case AMITK_RAW_FORMAT_DOUBLE_64_BE: 
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
	    for (i.y = 0; i.y < dim.y; i.y++) 
	      for (i.x = 0; i.x < dim.x; i.x++) {
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
		temp = GUINT64_FROM_BE(DATA_CONTENT(data, dim, i));
		double_p = (void *) &temp;
		if (i.x == -1) g_print("no op\n"); /* gets around compiler bug in gcc 4.1.2-27 */
		AMITK_RAW_DATA_DOUBLE_SET_CONTENT(raw_data,i) = *double_p;
#else /* G_BIG_ENDIAN */
		AMITK_RAW_DATA_DOUBLE_SET_CONTENT(raw_data,i) = DATA_CONTENT(data, dim, i);
#endif
              }
	  }
	  break;
	case AMITK_RAW_FORMAT_FLOAT_32_BE:
	  {
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
	    guint32 * data = file_buffer;
	    guint32 temp;
	    gfloat * float_p;
#else /* (G_BYTE_ORDER == G_BIG_ENDIAN) */
	    gfloat * data = file_buffer;
#endif
	    
	    /* copy this frame into the data set */
	    for (i.y = 0; i.y < dim.y; i.y++) 
	      for (i.x = 0; i.x < dim.x; i.x++) {
		/* keep compiler from generating bad code with a noop, occurs with gcc 3.0.3 */
		if (i.x == -1) g_print("no op\n");
		
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
		temp = GUINT32_FROM_BE(DATA_CONTENT(data, dim, i));
		float_p = (void *) &temp;
		if (i.x == -1) g_print("no op\n"); /* gets around compiler bug in gcc 4.1.2-27 */
		AMITK_RAW_DATA_FLOAT_SET_CONTENT(raw_data,i) = *float_p;
#else /* G_BIG_ENDIAN */
		AMITK_RAW_DATA_FLOAT_SET_CONTENT(raw_data,i) = DATA_CONTENT(data, dim, i);
#endif
	      }
	  }
	  break;
	case AMITK_RAW_FORMAT_SINT_32_BE:
	  {
	    gint32 * data=file_buffer;
	    
	    /* copy this frame into the data set */
	    for (i.y = 0; i.y < dim.y; i.y++) 
	      for (i.x = 0; i.x < dim.x; i.x++)
		AMITK_RAW_DATA_SINT_SET_CONTENT(raw_data,i) = 
		  GINT32_FROM_BE(DATA_CONTENT(data, dim, i));
	  }
	  break;
	case AMITK_RAW_FORMAT_UINT_32_BE:
	  {
	    guint32 * data=file_buffer;
	    
	    /* copy this frame into the data set */
	    for (i.y = 0; i.y < dim.y; i.y++) 
	      for (i.x = 0; i.x < dim.x; i.x++) {
		/* keep compiler from generating bad code with a noop occurs with gcc 3.0.3 */
		if (i.x == -1) g_print("no op\n");
		
		AMITK_RAW_DATA_UINT_SET_CONTENT(raw_data,i) = 
		  GUINT32_FROM_BE(DATA_CONTENT(data, dim, i));
	      }
	  }
	  break;
	case AMITK_RAW_FORMAT_SSHORT_16_BE:
	  {
	    gint16 * data=file_buffer;
	    
	    /* copy this frame into the data set */
	    for (i.y = 0; i.y < dim.y; i.y++) 
	      for (i.x = 0; i.x < dim.x; i.x++)
		AMITK_RAW_DATA_SSHORT_SET_CONTENT(raw_data,i) = 
		  GINT16_FROM_BE(DATA_CONTENT(data, dim, i));
	  }
	  break;
	case AMITK_RAW_FORMAT_USHORT_16_BE:
	  {
	    guint16 * data=file_buffer;
	    
	    /* copy this frame into the data set */
	    for (i.y = 0; i.y < dim.y; i.y++) 
	      for (i.x = 0; i.x < dim.x; i.x++)
		AMITK_RAW_DATA_USHORT_SET_CONTENT(raw_data,i) =
		  GUINT16_FROM_BE(DATA_CONTENT(data, dim, i));
	  }
	  break;
	  
	  
	  
	  
	case AMITK_RAW_FORMAT_DOUBLE_64_LE:
	  {
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
	    guint64 * data = file_buffer;
	    guint64 temp;
	    gdouble * double_p;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
	    gdouble * data = file_buffer;
#endif
	    
	    /* copy this frame into the data set */
	    for (i.y = 0; i.y < dim.y; i.y++) 
	      for (i.x = 0; i.x < dim.x; i.x++) {
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
		temp = GUINT64_FROM_LE(DATA_CONTENT(data, dim, i));
		double_p = (void *) &temp;
		if (i.x == -1) g_print("no op\n"); /* gets around compiler bug in gcc 4.1.2-27 */
		AMITK_RAW_DATA_DOUBLE_SET_CONTENT(raw_data,i) = *double_p;
#else /* G_LITTLE_ENDIAN */
		AMITK_RAW_DATA_DOUBLE_SET_CONTENT(raw_data,i) = DATA_CONTENT(data, dim, i);
#endif
	      }

	  }
	  break;
	  
	  
	  
	  
	case AMITK_RAW_FORMAT_FLOAT_32_LE:
	  {
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
	    guint32 * data = file_buffer;
	    guint32 temp;
	    gfloat * float_p;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
	    gfloat * data = file_buffer;
#endif
	    
	    /* copy this frame into the data set */
	    for (i.y = 0; i.y < dim.y; i.y++) 
	      for (i.x = 0; i.x < dim.x; i.x++) {
		
		/* keep compiler from generating bad code with a noop,occurs with gcc 3.0.3 */
		if (i.x == -1) g_print("no op\n");
		
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
		temp = GUINT32_FROM_LE(DATA_CONTENT(data, dim, i));
		float_p = (void *) &temp;
		if (i.x == -1) g_print("no op\n"); /* gets around compiler bug in gcc 4.1.2-27 */
		AMITK_RAW_DATA_FLOAT_SET_CONTENT(raw_data,i) = *float_p;
#else /* G_LITTLE_ENDIAN */
		AMITK_RAW_DATA_FLOAT_SET_CONTENT(raw_data,i) = DATA_CONTENT(data, dim, i);
#endif
	      }
	  }
	  break;
	  

	  
	  
	case AMITK_RAW_FORMAT_SINT_32_LE:
	  {
	    gint32 * data=file_buffer;
	    
	    /* copy this frame into the data set */
	    for (i.y = 0; i.y < dim.y; i.y++) 
	      for (i.x = 0; i.x < dim.x; i.x++)
		AMITK_RAW_DATA_SINT_SET_CONTENT(raw_data,i) =
		  GINT32_FROM_LE(DATA_CONTENT(data, dim, i));
	    
	  }
	  break;
	case AMITK_RAW_FORMAT_UINT_32_LE:
	  {
	    guint32 * data=file_buffer;
	    
	    /* copy this frame into the data set */
	    for (i.y = 0; i.y < dim.y; i.y++) 
	      for (i.x = 0; i.x < dim.x; i.x++) {
		/* keep compiler from generating bad code with a noop,occurs with gcc 3.0.3 */
		if (i.x == -1) g_print("no_op\n");
		
		AMITK_RAW_DATA_UINT_SET_CONTENT(raw_data,i) = 
		  GUINT32_FROM_LE(DATA_CONTENT(data, dim, i));
	      }
	  }
	  break;
	case AMITK_RAW_FORMAT_SSHORT_16_LE:
	  {
	    gint16 * data=file_buffer;
	    
	    /* copy this frame into the data set */
	    for (i.y = 0; i.y < dim.y; i.y++) 
	      for (i.x = 0; i.x < dim.x; i.x++)
		AMITK_RAW_DATA_SSHORT_SET_CONTENT(raw_data,i) =
		  GINT16_FROM_LE(DATA_CONTENT(data, dim, i));
	    
	  }
	  break;
	case AMITK_RAW_FORMAT_USHORT_16_LE:
	  {
	    guint16 * data=file_buffer;
	    
	    /* copy this frame into the data set */
	    for (i.y = 0; i.y < dim.y; i.y++) 
	      for (i.x = 0; i.x < dim.x; i.x++)
		AMITK_RAW_DATA_USHORT_SET_CONTENT(raw_data,i) =
		  GUINT16_FROM_LE(DATA_CONTENT(data, dim, i));
	    
	  }
	  break;
	case AMITK_RAW_FORMAT_SBYTE_8_NE:
	  {
	    gint8 * data=file_buffer;
	    
	    /* copy this frame into the data set */
	    for (i.y = 0; i.y < dim.y; i.y++) 
	      for (i.x = 0; i.x < dim.x; i.x++)
		AMITK_RAW_DATA_SBYTE_SET_CONTENT(raw_data,i) = DATA_CONTENT(data, dim, i);
	  }
	  break;
	case AMITK_RAW_FORMAT_UBYTE_8_NE:
	default:
	  {
	    guint8 * data=file_buffer;
	    
	    /* copy this frame into the data set */
	    for (i.y = 0; i.y < dim.y; i.y++) 
	      for (i.x = 0; i.x < dim.x; i.x++)
		AMITK_RAW_DATA_UBYTE_SET_CONTENT(raw_data,i) = DATA_CONTENT(data, dim, i);
	  }
	  break;
	}   
      }
    }
  }

  goto exit_condition;




 error_condition:

  if (raw_data != NULL)
    g_object_unref(raw_data);
  raw_data = NULL;




 exit_condition:

  if (new_file_pointer != NULL)
    fclose(new_file_pointer);

  if (file_buffer != NULL)
    g_free(file_buffer);

  if (update_func != NULL) 
    (*update_func)(update_data, NULL, (gdouble) 2.0); 

  return raw_data;

}


/* function to write out the information content of a raw_data set into an xml
   file.  Returns a string containing the name of the file. */
void amitk_raw_data_write_xml(AmitkRawData * raw_data, const gchar * name, 
			      FILE *study_file, gchar ** output_filename, guint64 * plocation,
			      guint64 * psize) {

  gchar * xml_filename=NULL;
  gchar * raw_filename=NULL;
  guint count;
  struct stat file_info;
  xmlDocPtr doc;
  FILE * file_pointer;
  guint64 location, size;
  size_t num_wrote;
  size_t num_to_write;
  size_t num_to_write_this_time;
  size_t bytes_per_unit;
  size_t total_to_write;
  size_t total_wrote = 0;

  if (study_file == NULL) {
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

    /* Note, "wb" is same as "w" on Unix, but not in Windows */
    if ((file_pointer = fopen(raw_filename, "wb")) == NULL) {
      g_warning(_("couldn't save raw data file: %s"),raw_filename);
      g_free(xml_filename);
      g_free(raw_filename);
      return;
    }
  } else {
    file_pointer = study_file;
  }
  
  /* write it on out.  */
  location = ftell(file_pointer);
  num_to_write = amitk_raw_data_num_voxels(raw_data);
  bytes_per_unit = amitk_format_sizes[AMITK_RAW_DATA_FORMAT(raw_data)]; 
  total_to_write = num_to_write;
   
  /* write in small chunks (<=16MB) to get around a bad samba/cygwin interaction */
  while(num_to_write > 0) {
    if (num_to_write*bytes_per_unit > 0x1000000) 
      num_to_write_this_time = 0x1000000/bytes_per_unit;
    else
      num_to_write_this_time = num_to_write;
    num_to_write -= num_to_write_this_time;
    
    num_wrote = fwrite((raw_data->data + total_wrote*bytes_per_unit),
		       bytes_per_unit, num_to_write_this_time, file_pointer);
    total_wrote += num_wrote;
    
    if (num_wrote != num_to_write_this_time) {
      g_warning(_("incomplete save of raw data, wrote %zd (bytes), needed %zd (bytes), file: %s"),
		total_wrote*bytes_per_unit, 
		total_to_write*bytes_per_unit,
		raw_filename);
      g_free(xml_filename);
      g_free(raw_filename);
      if (study_file == NULL) fclose(file_pointer);
      return;
    }
  }
 
  
  size = ftell(file_pointer)-location;
  if (study_file == NULL) fclose(file_pointer);
    
  /* write the xml portion */
  doc = xmlNewDoc((xmlChar *) "1.0");
  doc->children = xmlNewDocNode(doc, NULL, (xmlChar *) "raw_data", (xmlChar *) name);
  amitk_voxel_write_xml(doc->children, "dim", raw_data->dim);
  xml_save_string(doc->children,"raw_format", 
		  amitk_raw_format_get_name(amitk_format_to_raw_format(raw_data->format)));

  /* store the info on our associated data */
  if (study_file == NULL) {
    xml_save_string(doc->children, "raw_data_file", raw_filename);
    g_free(raw_filename);
  } else {
    xml_save_location_and_size(doc->children, "raw_data_location_and_size", location, size);
  }

  /* and save */
  if (study_file == NULL) {
    xmlSaveFile(xml_filename, doc);
    if (output_filename != NULL) *output_filename = xml_filename;
    else g_free(xml_filename);
  } else {
    *plocation = ftell(study_file);
    xmlDocDump(study_file, doc);
    *psize = ftell(study_file)-*plocation;
  }

  /* and we're done with the xml stuff*/
  xmlFreeDoc(doc);


  return;
}


/* function to load in a raw data xml file */
AmitkRawData * amitk_raw_data_read_xml(gchar * xml_filename,
				       FILE * study_file,
				       guint64 location,
				       guint64 size,
				       gchar ** perror_buf,
				       AmitkUpdateFunc update_func,
				       gpointer update_data) {

  xmlDocPtr doc;
  AmitkRawData * raw_data;
  xmlNodePtr nodes;
  AmitkRawFormat i_raw_format, raw_format;
  gchar * temp_string;
  gchar * raw_filename=NULL;
  guint64 offset, dummy;
  long offset_long=0;
  AmitkVoxel dim;


  if ((doc = xml_open_doc(xml_filename, study_file, location, size, perror_buf)) == NULL)
    return NULL; /* function already appends the error message */

  /* get the root of our document */
  if ((nodes = xmlDocGetRootElement(doc)) == NULL) {
    amitk_append_str_with_newline(perror_buf,_("Raw data xml file doesn't appear to have a root: %s"), 
				  xml_filename);
    return NULL;
  }

  /* get the document tree */
  nodes = nodes->children;

  dim = amitk_voxel_read_xml(nodes, "dim", perror_buf);

  /* figure out the data format */
  temp_string = xml_get_string(nodes, "raw_format");
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
  raw_format = AMITK_RAW_FORMAT_DOUBLE_64_BE; /* sensible guess in case we don't figure it out from the file */
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
  raw_format = AMITK_RAW_FORMAT_DOUBLE_64_LE; /* sensible guess in case we don't figure it out from the file */
#endif
  if (temp_string != NULL)
    for (i_raw_format=0; i_raw_format < AMITK_RAW_FORMAT_NUM; i_raw_format++) 
      if (g_ascii_strcasecmp(temp_string, amitk_raw_format_get_name(i_raw_format)) == 0)
	raw_format = i_raw_format;

  /* also need to check against legacy names for files created before amide version 0.7.11 */
  for (i_raw_format=0; i_raw_format < AMITK_RAW_FORMAT_NUM; i_raw_format++) 
    if (g_ascii_strcasecmp(temp_string, amitk_raw_format_legacy_names[i_raw_format]) == 0)
      raw_format = i_raw_format;

  g_free(temp_string);
  
  /* get the filename or location of our associated data */
  if (study_file == NULL) {
    raw_filename = xml_get_string(nodes, "raw_data_file");
    /* now load in the raw data */
#ifdef AMIDE_DEBUG
    g_print("reading data from file %s\n", raw_filename);
#endif
  } else {
    xml_get_location_and_size(nodes, "raw_data_location_and_size", &offset, &dummy, perror_buf);

    /* check for file size problems */
    if (!xml_check_file_32bit_okay(offset)) {
      amitk_append_str_with_newline(perror_buf, _("File to large to read on 32bit platform."));
      return NULL;
    }
    offset_long = offset;
  }


  raw_data = amitk_raw_data_import_raw_file(raw_filename, study_file, raw_format, dim, offset_long, 
					    update_func, update_data);

  /* and we're done */
  if (raw_filename != NULL) g_free(raw_filename);
  xmlFreeDoc(doc);

  return raw_data;
}

amide_data_t amitk_raw_data_get_value(const AmitkRawData * rd, const AmitkVoxel i) {

  g_return_val_if_fail(AMITK_IS_RAW_DATA(rd), EMPTY);
  
  if (!amitk_raw_data_includes_voxel(rd, i)) return EMPTY;

  /* hand everything off to the data type specific function */
  switch(AMITK_RAW_DATA_FORMAT(rd)) {
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

gpointer amitk_raw_data_get_pointer(const AmitkRawData * rd, const AmitkVoxel i) {

  g_return_val_if_fail(AMITK_IS_RAW_DATA(rd), NULL);
  g_return_val_if_fail(amitk_raw_data_includes_voxel(rd, i), NULL);

  /* hand everything off to the data type specific function */
  switch(AMITK_RAW_DATA_FORMAT(rd)) {
  case AMITK_FORMAT_UBYTE:
    return AMITK_RAW_DATA_UBYTE_POINTER(rd, i);
  case AMITK_FORMAT_SBYTE:
    return AMITK_RAW_DATA_SBYTE_POINTER(rd, i);
  case AMITK_FORMAT_USHORT:
    return AMITK_RAW_DATA_USHORT_POINTER(rd, i);
  case AMITK_FORMAT_SSHORT:
    return AMITK_RAW_DATA_SSHORT_POINTER(rd, i);
  case AMITK_FORMAT_UINT:
    return AMITK_RAW_DATA_UINT_POINTER(rd, i);
  case AMITK_FORMAT_SINT:
    return AMITK_RAW_DATA_SINT_POINTER(rd, i);
  case AMITK_FORMAT_FLOAT:
    return AMITK_RAW_DATA_FLOAT_POINTER(rd, i);
  case AMITK_FORMAT_DOUBLE:
    return AMITK_RAW_DATA_DOUBLE_POINTER(rd, i);
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    return NULL;
  }
}



/* take in one of the raw data formats, and return the corresponding data format */
AmitkFormat amitk_raw_format_to_format(AmitkRawFormat raw_format) {

  AmitkFormat format;

  switch(raw_format) {

  case AMITK_RAW_FORMAT_UBYTE_8_NE: 
    format = AMITK_FORMAT_UBYTE;
    break;
  case AMITK_RAW_FORMAT_SBYTE_8_NE: 
    format = AMITK_FORMAT_SBYTE;
    break;
  case AMITK_RAW_FORMAT_USHORT_16_LE: 
  case AMITK_RAW_FORMAT_USHORT_16_BE: 
    format = AMITK_FORMAT_USHORT;
    break;
  case AMITK_RAW_FORMAT_SSHORT_16_LE:
  case AMITK_RAW_FORMAT_SSHORT_16_BE:
    format = AMITK_FORMAT_SSHORT;
    break;
  case AMITK_RAW_FORMAT_UINT_32_LE:
  case AMITK_RAW_FORMAT_UINT_32_BE:
  case AMITK_RAW_FORMAT_UINT_32_PDP:
    format = AMITK_FORMAT_UINT;
    break;
  case AMITK_RAW_FORMAT_SINT_32_LE:
  case AMITK_RAW_FORMAT_SINT_32_BE:
  case AMITK_RAW_FORMAT_SINT_32_PDP:
    format = AMITK_FORMAT_SINT;
    break;
  case AMITK_RAW_FORMAT_FLOAT_32_LE:
  case AMITK_RAW_FORMAT_FLOAT_32_BE:
  case AMITK_RAW_FORMAT_FLOAT_32_PDP:
#if (SIZE_OF_AMIDE_T == 4)
  case AMITK_RAW_FORMAT_ASCII_8_NE:
#endif
    format = AMITK_FORMAT_FLOAT;
    break;
  case AMITK_RAW_FORMAT_DOUBLE_64_LE:
  case AMITK_RAW_FORMAT_DOUBLE_64_BE:
#if (SIZE_OF_AMIDE_T == 8)
  case AMITK_RAW_FORMAT_ASCII_8_NE:
#endif
    format = AMITK_FORMAT_DOUBLE;
    break;
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
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
    raw_format = AMITK_RAW_FORMAT_UBYTE_8_NE;
    break;
  case AMITK_FORMAT_SBYTE: 
    raw_format = AMITK_RAW_FORMAT_SBYTE_8_NE;
    break;
  case AMITK_FORMAT_USHORT: 
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
    raw_format = AMITK_RAW_FORMAT_USHORT_16_BE;
#else
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
    raw_format = AMITK_RAW_FORMAT_USHORT_16_LE;
#else
#error "must specify G_LITTLE_ENDIAN or G_BIG_ENDIAN)"
#endif
#endif
    break;
  case AMITK_FORMAT_SSHORT:
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
    raw_format = AMITK_RAW_FORMAT_SSHORT_16_BE;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
    raw_format = AMITK_RAW_FORMAT_SSHORT_16_LE;
#endif
    break;
  case AMITK_FORMAT_UINT:
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
    raw_format = AMITK_RAW_FORMAT_UINT_32_BE;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
    raw_format = AMITK_RAW_FORMAT_UINT_32_LE;
#endif
    break;
  case AMITK_FORMAT_SINT:
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
    raw_format = AMITK_RAW_FORMAT_SINT_32_BE;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
    raw_format = AMITK_RAW_FORMAT_SINT_32_LE;
#endif
    break;
  case AMITK_FORMAT_FLOAT:
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
    raw_format = AMITK_RAW_FORMAT_FLOAT_32_BE;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
    raw_format = AMITK_RAW_FORMAT_FLOAT_32_LE;
#endif
    break;
  case AMITK_FORMAT_DOUBLE:
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
    raw_format = AMITK_RAW_FORMAT_DOUBLE_64_BE;
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
    raw_format = AMITK_RAW_FORMAT_DOUBLE_64_LE;
#endif
    break;
  default:
    g_error("unexpected case in %s at line %d", __FILE__, __LINE__);
    raw_format = AMITK_RAW_FORMAT_UBYTE_8_NE; /* take a wild guess */
    break;
  }
   
  return raw_format;
}


const gchar * amitk_raw_format_get_name(const AmitkRawFormat raw_format) {

  GEnumClass * enum_class;
  GEnumValue * enum_value;

  enum_class = g_type_class_ref(AMITK_TYPE_RAW_FORMAT);
  enum_value = g_enum_get_value(enum_class, raw_format);
  g_type_class_unref(enum_class);

  return enum_value->value_nick;
}




