/* image.c
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

#include "amide_config.h"
#include "image.h"
#include "amitk_data_set_DOUBLE_0D_SCALING.h"
#include "amitk_study.h"


#define OBJECT_ICON_XSIZE 24
#define OBJECT_ICON_YSIZE 24

guchar CT_icon_data[] = { 
  0,   0,   0,   0,   0,   0,   0,   0,  17,  43,  35,  35,  43,  43,  43,  26,  35,   8,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,  43,  79, 105, 105, 131, 149, 158, 149, 149, 105, 149,  79,  52,   8,   0,   0,   0,   0,
  0,   0,   0,   0,   0,  61,  96, 149, 175, 123, 105, 114, 246, 140, 105, 123, 123, 149, 131,  79,   0,   0,   0,   0,
  0,   0,   0,   0,  52,  96, 175, 123,  70,  61,  70,  87,  96,  96, 105, 105,  96,  87, 114, 131,  61,   0,   0,   0,
  0,   0,   0,  17, 105, 114, 114,  43,  70,  87,  87,  96, 105,  96,  96,  96,  96,  96,  79, 149, 114,   8,   0,   0,
  17, 26,   0,  61, 105,  96,  26,  52,  79,  87,  96,  96,  96,  96,  96,  96,  96,  96,  96, 158, 105,  52,   0,   0,
  70, 61,  79,  96, 105,  43,  26,  70,  87,  87,  87,  87,  96,  96,  96,  96,  96,  96, 105, 123,  96,  87,  35,   0,
  43, 79,  96, 123, 158,  17,  43,  79,  87,  87,  87,  87,  87,  96,  96,  96,  96,  96,  87,  79, 140, 105,  79,   0,
  17, 87,  96, 123, 131,   8,  52,  70,  87,  87,  87,  87,  79,  79,  79,  87,  87,  79,  52,   8, 123, 158, 114,   8,
  35, 96,  96, 114,  52,  26,  61,  70,  87,  87,  87,  79,  70,  70,  70,  61,  61,  52,  17,   0, 114, 131,  96,  26,
  8,  70,  96, 184,  26,  26,  61,  70,  70,  87,  96,  79,  61,  61,  61,  52,  52,  43,  35,  17,  79, 114,  87,  43,
  0,  52,  96, 184,  17,  26,  52,  61,  70,  79,  96,  79,  52,  52,  43,  43,  43,  43,  35,  17,  61, 211, 140,  35,
  0,  61,  96,  96,  17,   8,  26,  43,  52,  70,  87,  52,  26,  26,  17,  26,  26,   8,   8,   0,  87, 158,  96,   0,
  0,  17,  96, 114,  35,   8,  17,  26,   8,  17,  35,   8,  26,  17,   8,   8,   0,   0,   8,  35, 114,  87,  52,   0,
  0,   8,  96, 175, 123,   0,   0,   8,   8,   0,  52,   8,   8,  17,   8,  35,   8,  26,   8,  26, 193, 105,  17,   0,
  0,   0,  70, 114, 140,  26,  17,   8,   8,  26,  26,   8,  17,  17,  17,  17,  26,  17,   0,  87, 202, 114,   0,   0,
  0,   0,  43,  96, 105,  43,   8,  17,  17,  26,  17,   0,  52,  43,   0,   8,  26,   8,  43, 140,  79,  26,   0,   0,
  0,   0,   8,  70,  96,  96,  52,  17,   8,   8,   8,  35, 184,  79,   8,  17,  52,  61,  79,  96,  79,   0,   0,   0,
  0,   0,   0,   8,  79,  96, 175, 158,  35,  17,  61, 219, 255, 228,  87,  96, 202, 228, 131,  96,  17,   0,   0,   0,
  0,   0,   0,   0,  43, 105, 149, 184, 149, 123, 131, 211, 131, 237, 149, 114,  96, 123, 114,  26,   0,   0,   0,   0,
  0,   0,   0,   0,   0,  43,  79,  96, 105, 105, 114, 228, 228, 228, 131, 105, 105, 105,  35,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,  17,  61,  79,  96, 114, 140, 167, 123, 123, 105,  87,  35,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,  17,  79,  96, 105, 105, 105,  87,  70,  17,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,  61,  70,  35,  17,  17,   8,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

guchar PET_icon_data[] = {
  3,    8,  25,  35,  21,  22,  50,  96, 148, 171, 134, 109, 145, 177, 156, 134,  86,  54,  42,  40,  42,  25,  11,   3,
  8,   27,  29,  25,  24,  53, 120, 162, 187, 216, 224, 184, 211, 224, 198, 185, 147, 108,  61,  33,  34,  32,  26,  10,
  28,  34,  18,  25,  88, 129, 160, 154, 166, 214, 234, 203, 210, 211, 185, 176, 169, 156, 133,  83,  37,  21,  28,  26,
  37,  24,  21,  75, 158, 161, 154, 142, 180, 234, 255, 224, 221, 220, 212, 186, 162, 160, 169, 165, 101,  32,  18,  37,
  28,  25,  70, 139, 163, 141, 122, 147, 197, 226, 242, 196, 195, 218, 206, 200, 148, 116, 155, 188, 153,  72,  24,  41,
  29,  45, 107, 165, 171, 136, 116, 160, 198, 203, 216, 165, 158, 201, 189, 193, 155, 125, 153, 168, 171, 125,  53,  36,
  45, 104, 147, 172, 173, 141, 100, 122, 157, 179, 190, 153, 147, 181, 166, 134, 102,  97, 122, 163, 187, 179, 124,  43,
  76, 163, 184, 178, 158, 116,  73,  76,  89, 121, 154, 137, 138, 144, 102,  83,  70,  80, 114, 170, 195, 200, 162,  64,
  80, 158, 168, 165, 148, 104,  74,  81,  76,  81,  93,  96,  94,  85,  66,  65,  62,  69,  96, 145, 179, 193, 174,  91,
  85, 158, 173, 169, 132, 104,  93,  93,  96, 101,  82,  67,  58,  70,  75,  83,  77,  83, 113, 153, 174, 190, 180, 101,
  74, 158, 166, 176, 172, 170, 144, 120, 128, 150, 138,  89,  82, 122, 157, 139, 107, 140, 165, 182, 187, 184, 171, 102,
  61, 152, 178, 181, 186, 185, 163, 132, 137, 200, 195, 147, 128, 176, 217, 161, 115, 148, 168, 195, 193, 178, 162,  89,
  30, 117, 164, 168, 170, 157, 155, 137, 117, 184, 187, 145, 138, 174, 164, 125, 120, 147, 160, 169, 188, 176, 129,  52,
  26,  99, 160, 174, 177, 162, 150, 130, 118, 118, 102,  89,  81,  96, 120, 108, 126, 162, 173, 155, 165, 171, 125,  43,
  42,  84, 156, 177, 162, 164, 157, 130,  97, 102,  73,  53,  56,  68, 102, 114, 114, 148, 168, 173, 174, 166, 108,  37,
  41,  69, 133, 179, 186, 165, 147, 122,  99,  96,  60,  50,  50,  48,  88, 110, 130, 158, 158, 176, 185, 145,  85,  42,
  36,  41,  99, 171, 187, 174, 157, 122,  91,  62,  51,  66,  69,  53,  66,  93, 146, 165, 177, 185, 196, 108,  42,  38,
  43,  35,  72, 157, 180, 160, 126, 106,  82,  75,  83,  94, 100,  83,  65,  89, 134, 153, 154, 179, 184,  92,  30,  28,
  33,  24,  38, 108, 176, 179, 139,  98,  85, 114, 144, 146, 147, 130,  93,  82, 110, 126, 150, 195, 152,  46,  21,  22,
  22,  33,  34,  53, 163, 187, 153, 129, 107, 140, 172, 169, 168, 161, 117,  92, 132, 173, 184, 165,  92,  34,  30,  13,
  16,  29,  30,  19, 104, 177, 197, 180, 156, 158, 177, 168, 169, 169, 141, 136, 181, 210, 172,  81,  36,  33,  29,   6,
  5,    6,  13,  17,  41,  81, 152, 179, 193, 170, 178, 165, 162, 165, 164, 181, 187, 163,  86,  13,  19,  26,  12,   3,
  3,    3,   6,  29,  41,  33,  49,  89, 133, 154, 155, 118, 140, 164, 166, 138,  86,  44,  24,  26,  26,   9,   5,   3,
  3,    8,  18,  21,  24,  37,  34,  26,  40,  60,  62,  36,  51,  73,  61,  50,  25,  22,  29,  22,  12,   5,   6,   6
};

  
guchar MRI_icon_data[] = {
  0,   0,   0,   0,   0,   0,   0,   0,   6,  10,  16,  14,  15,  13,   3,   2,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   3,  22,  19,  32,  58,  24,  22,  50,  16,  19,  12,   1,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   4,  24,  42, 153, 222, 133, 111, 108, 160, 162,  99,  22,  22,   1,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   2,  24,  80, 150, 147, 169, 122, 164, 141, 116, 113, 170, 156,  40,  21,   1,   0,   0,   0,   0,
  0,   0,   0,   0,  19,  47, 169, 134, 132, 148, 157, 139, 120, 116, 120, 146, 116, 174,  15,  21,   0,   0,   0,   0,
  0,   0,   0,   5,  21, 175, 230, 165, 138, 199, 204, 202, 179, 152, 135, 116, 141, 155,  87,  20,   4,   0,   0,   0,
  0,   0,   0,  14,  70, 146, 213, 152, 149, 178, 206, 148, 159, 152, 155, 150, 120, 141, 185,  35,  12,   0,   0,   0,
  0,   0,   0,  24, 112, 135, 134, 132, 223, 185, 150, 120, 148, 137, 232, 193, 121, 139, 105, 102,  11,   0,   0,   0,
  0,   0,   0,  23, 142, 147, 133, 134, 184, 240, 141, 121, 116, 181, 241, 148, 110,  96, 116, 104,  13,   0,   0,   0,
  0,   0,   7,  28, 158, 152, 144, 142, 115, 129, 168, 202, 196, 157, 124,  93, 117,  92, 101, 105,  12,   0,   0,   0,
  0,   0,   7,  14, 147, 122, 129, 205, 116, 131, 143, 222, 201, 122, 111, 134, 152,  90, 116, 108,  14,   0,   0,   0,
  0,   0,   3,  12, 111, 111, 192, 195, 122,  96, 120, 217, 181, 102,  94, 131, 197,  96,  85,  79,  20,   0,   0,   0,
  0,   0,   0,  16,  70, 114, 186, 203, 164, 134, 101, 225, 185,  78, 132, 150, 207, 156, 123,  37,  20,   0,   0,   0,
  0,   0,   0,  17,  23, 138, 181, 207, 148, 165, 183, 228, 210, 164, 164, 155, 223, 125, 111,  15,  21,   0,   0,   0,
  0,   0,   0,  15,  15, 122, 206, 197, 192, 174, 210, 208, 206, 187, 164, 213, 252, 148,  94,  28,  16,   0,   0,   0,
  0,   0,   0,  17,  48,  40, 132, 106, 162, 172, 201, 196, 186, 190, 175, 156, 183, 144,  53,  37,   3,   0,   0,   0,
  0,   0,   0,   7,  48,  37,  73,  24,  71, 122, 167, 186, 190, 165, 128,  70,  29,  76,  37,  52,   0,   0,   0,   0,
  0,   0,   0,   0,  44,  31,  59,  48,  71,  68,  25, 165, 123,  42,  62,  53,  35,  40,  52,  38,   0,   0,   0,   0,
  0,   0,   0,   0,  12,  41,  53, 144, 149,  99,  35, 110,  51,  75, 107, 176,  84,  28,  59,   4,   0,   0,   0,   0,
  0,   0,   0,   0,   0,  22,  83, 253, 255, 123, 108,  68,  61,  69, 198, 253, 203,  37,  24,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   5,  22, 186, 205,  61,  25,  58,  32,  64, 144, 224, 178,  26,   2,   1,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   6,  14,  30,  13,  25,  21,  29,  17,  49,   5,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   8,  31,  37,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  12,   6,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};


guchar SPECT_icon_data[] = {
  17,  17,  36,  56,  51,  46,  36,  31,  29,  31,  39,  63,  93, 115, 147, 147, 102,  78,  41,  22,  17,  17,  19,  12,
  12,  24,  61, 125, 102,  80,  71,  63,  53,  58,  51,  73,  95, 132, 152, 139, 112,  80,  39,  22,  17,  17,  19,  12,
  2,   17,  66, 166, 139, 102,  78,  76,  76,  83,  71,  73, 102, 134, 139, 115,  98,  73,  34,  17,  17,  17,  12,   7,
  4,   22,  98, 203, 174, 139, 117, 107, 110, 112, 100,  98, 122, 139, 134, 107,  90,  58,  22,  17,  17,  12,   4,   0,
  9,   29,  61, 102, 115, 112, 100, 105, 115, 132, 132, 132, 139, 137, 129,  98,  85,  46,  19,  17,  14,   9,   2,   0,
  17,  31,  29,  39,  51,  78,  53,  44,  51,  71, 115, 139, 137, 132, 147, 107,  78,  31,  19,  17,  14,   4,   0,   0,
  14,  46, 110, 129, 112, 117, 107,  95,  76,  56,  90, 132, 120, 110, 144, 117,  76,  34,  22,  19,  17,   2,   0,   0,
  12,  41, 156, 250, 232, 191, 164, 159, 166, 127,  85, 122, 122, 120, 125, 110,  73,  39,  26,  14,  17,   9,   0,   0,
  19,  41, 112, 178, 213, 228, 196, 144, 171, 186, 110, 137, 122, 117, 127, 110,  85,  73,  61,  31,  19,  14,   2,   0,
  31,  76,  95,  98, 127, 147, 152, 129, 169, 178, 120, 127, 100, 100, 125, 107,  98, 107, 115,  95,  51,  22,   9,   2,
  46, 112, 112,  95, 110, 152, 201, 191, 203, 198, 171, 152, 112, 110, 201, 129,  83,  78,  85, 110, 112,  53,  19,   2,
  36, 115, 142, 125, 142, 191, 205, 203, 255, 235, 205, 178, 183, 201, 250, 142,  63,  58,  71,  98, 156, 156,  63,  14,
  14,  71, 107, 120, 164, 149, 122, 100, 159, 210, 147,  98,  90,  76,  83,  76,  61,  63,  61,  90, 147, 178, 120,  34,
  24,  63, 115, 154, 156, 125, 110, 105, 169, 188,  98,  46,  51,  44,  44,  39,  44,  49,  58,  80, 120, 139, 154,  71,
  58, 112, 147, 166, 159, 110, 107, 110, 152, 154,  88,  46,  46,  46,  39,  31,  34,  29,  41,  49,  71,  98, 152,  98,
  78, 149, 142, 139, 134,  90,  98,  95,  63,  44,  41,  53,  61,  53,  44,  41,  41,  41,  44,  46,  53,  80, 149, 112,
  66, 149, 110,  95,  83,  76,  71,  61,  39,  34,  36,  39,  49,  44,  44,  49,  41,  44,  49,  46,  61,  93, 152, 102,
  31, 120, 105,  85,  80,  76,  61,  58,  51,  41,  34,  36,  39,  36,  36,  36,  36,  41,  49,  61,  80, 110, 156,  78,
  7,   83, 110, 110,  76,  66,  53,  53,  56,  51,  49,  51,  51,  36,  36,  36,  36,  39,  53,  68,  88, 137, 134,  46,
  4,   39, 102, 137, 100,  71,  63,  53,  58,  66,  71,  63,  61,  53,  46,  36,  44,  51,  56,  78, 110, 149,  80,  14,
  2,    7,  49, 120, 144, 107,  78,  78,  73,  76,  76,  85,  85,  66,  63,  63,  66,  73,  80, 107, 147, 112,  34,   4,
  0,    0,   7,  51, 115, 159, 125, 107, 100,  95,  98, 105, 100,  88,  93,  93,  90, 110, 139, 149, 102,  36,   7,   0,
  0,    0,   0,   7,  36, 102, 144, 166, 154, 139, 142, 132, 120, 122, 127, 137, 149, 154, 125,  76,  29,   7,   2,   0,
  0,    0,   0,   0,   0,  12,  49,  98, 127, 159, 174, 166, 166, 171, 159, 147, 117,  68,  29,   9,   2,   0,   2,   0
};


/* callback function used for freeing the pixel data in a gdkpixbuf */
static void image_free_rgb_data(guchar * pixels, gpointer data) {
  g_free(pixels);
  return;
}



/* note, return offset and corner are in base coordinate frame */
GdkPixbuf * image_slice_intersection(const AmitkRoi * roi,
				     const AmitkVolume * canvas_slice,
				     const amide_real_t pixel_size,
#ifdef AMIDE_LIBGNOMECANVAS_AA
				     const gdouble transparency,
#else
				     const gboolean fill_roi,
#endif
				     rgba_t color,
				     AmitkPoint * return_offset,
				     AmitkPoint * return_corner) {

  GdkPixbuf * temp_image;
  AmitkDataSet * intersection;
  AmitkVoxel i;
  guchar * rgba_data;
  AmitkVoxel dim;
#ifdef AMIDE_LIBGNOMECANVAS_AA
  guchar transparency_byte;

  transparency_byte = transparency * 0xFF;
#endif

  
  intersection = amitk_roi_get_intersection_slice(roi, canvas_slice, pixel_size 
#ifndef AMIDE_LIBGNOMECANVAS_AA
						  , fill_roi
#endif
						  );
  if (intersection == NULL) return NULL;

  dim = AMITK_DATA_SET_DIM(intersection);
  if ((dim.x == 0) && (dim.y ==0)) {
    amitk_object_unref(intersection);
    return NULL;
  }

  if ((rgba_data = g_try_new(guchar,4*dim.x*dim.y)) == NULL) {
    g_warning(_("couldn't allocate memory for rgba_data for roi image"));
    amitk_object_unref(intersection);
    return NULL;
  }

  i.z = i.g = i.t = 0;
  for (i.y=0 ; i.y < dim.y; i.y++)
    for (i.x=0 ; i.x < dim.x; i.x++)
      if (AMITK_RAW_DATA_UBYTE_CONTENT(intersection->raw_data, i) == 1) {
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+0] = color.r;
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+1] = color.g;
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+2] = color.b;
#ifdef AMIDE_LIBGNOMECANVAS_AA
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+3] = color.a;
#else
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+3] = 0xFF;
#endif
#ifdef AMIDE_LIBGNOMECANVAS_AA
      }	else if (AMITK_RAW_DATA_UBYTE_CONTENT(intersection->raw_data, i) == 2) {
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+0] = color.r;
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+1] = color.g;
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+2] = color.b;
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+3] = transparency_byte;
#endif
      }	else {
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+0] = 0x00;
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+1] = 0x00;
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+2] = 0x00;
	rgba_data[(dim.y-i.y-1)*dim.x*4 + i.x*4+3] = 0x00;
      }

  temp_image = gdk_pixbuf_new_from_data(rgba_data, GDK_COLORSPACE_RGB, TRUE,8,
					dim.x,dim.y,dim.x*4*sizeof(guchar),
					image_free_rgb_data, NULL);

  if (return_offset != NULL)
    *return_offset = AMITK_SPACE_OFFSET(intersection);
  if (return_corner != NULL)
    *return_corner = amitk_space_s2b(AMITK_SPACE(intersection), AMITK_VOLUME_CORNER(intersection));

  amitk_object_unref(intersection);

  return temp_image;
}

/* function to return a blank image */
GdkPixbuf * image_blank(const amide_intpoint_t width, const amide_intpoint_t height, rgba_t image_color) {
  
  GdkPixbuf * temp_image;
  amide_intpoint_t i,j;
  guchar * rgb_data;
  
  if ((rgb_data = g_try_new(guchar, 3*width*height)) == NULL) {
    g_warning(_("couldn't allocate memory for rgb_data for blank image"));
    return NULL;
  }

  for (i=0 ; i < height; i++)
    for (j=0; j < width; j++) {
      rgb_data[i*width*3+j*3+0] = image_color.r;
      rgb_data[i*width*3+j*3+1] = image_color.g;
      rgb_data[i*width*3+j*3+2] = image_color.b;
    }
  
  temp_image = gdk_pixbuf_new_from_data(rgb_data, GDK_COLORSPACE_RGB,
					FALSE,8,width,height,width*3*sizeof(guchar),
					image_free_rgb_data, NULL);

  return temp_image;
}


/* function returns a GdkPixbuf from 8 bit data */
GdkPixbuf * image_from_8bit(const guchar * image, 
			    const amide_intpoint_t width, 
			    const amide_intpoint_t height,
			    const AmitkColorTable color_table) {
  
  GdkPixbuf * temp_image;
  amide_intpoint_t i,j;
  guchar * rgb_data;
  rgba_t rgba_temp;
  guint location;

  if ((rgb_data = g_try_new(guchar,3*width*height)) == NULL) {
    g_warning(_("couldn't allocate memory for image from 8 bit data"));
    return NULL;
  }


  for (i=0 ; i < height; i++)
    for (j=0; j < width; j++) {
      /* note, line below compensates for X's origin being top left, not bottom left */
      rgba_temp = amitk_color_table_lookup(image[(height-i-1)*width+j], color_table, 0, 0xFF);
      location = i*width*3+j*3;
      rgb_data[location+0] = rgba_temp.r;
      rgb_data[location+1] = rgba_temp.g;
      rgb_data[location+2] = rgba_temp.b;

    }

  /* generate the GdkPixbuf from the rgb_data */
  temp_image = gdk_pixbuf_new_from_data(rgb_data, GDK_COLORSPACE_RGB,
					FALSE,8,width,height,width*3*sizeof(guchar),
					image_free_rgb_data, NULL);
  
  return temp_image;
}

#ifdef AMIDE_LIBVOLPACK_SUPPORT
/* function returns a GdkPixbuf from a rendering context */
GdkPixbuf * image_from_renderings(renderings_t * renderings, 
				gint16 image_width, gint16 image_height,
				AmideEye eyes, 
				gdouble eye_angle, 
				gint16 eye_width) {

  gint image_num;
  guint32 total_alpha;
  rgba16_t * rgba16_data;
  guchar * char_data;
  AmitkVoxel i;
  rgba_t rgba_temp;
  guint location;
  GdkPixbuf * temp_image;
  gint total_width;
  gdouble rot;
  AmideEye i_eye;
  gint j;

  total_width = image_width+(eyes-1)*eye_width;

  /* allocate and initialize space for a temporary storage buffer */
  if ((rgba16_data = g_try_new(rgba16_t,total_width * image_height)) == NULL) {
    g_warning(_("couldn't allocate memory for rgba16_data for transferring rendering to image"));
    return NULL;
  }
  for (j=0; j<total_width*image_height; j++) {
    rgba16_data[j].r = 0;
    rgba16_data[j].g = 0;
    rgba16_data[j].b = 0;
    rgba16_data[j].a = 0;
  }

  /* iterate through the eyes and rendering contexts, 
     tranfering the image data into the temp storage buffer */
  while (renderings != NULL) {

    for (i_eye = 0; i_eye < eyes; i_eye ++) {

      if (eyes == 1) {
	rendering_render(renderings->rendering);
      } else {
	rot = (-0.5 + i_eye*1.0) * eye_angle;
	rot = M_PI*rot/180; /* convert to radians */

	rendering_set_rotation(renderings->rendering, AMITK_AXIS_Y, -rot);
	rendering_render(renderings->rendering);
	rendering_set_rotation(renderings->rendering, AMITK_AXIS_Y, rot);
      }

      i.t = i.g = i.z = 0;
      for (i.y = 0; i.y < image_height; i.y++) 
	for (i.x = 0; i.x < image_width; i.x++) {
	  rgba_temp = amitk_color_table_lookup(renderings->rendering->image[i.x+i.y*image_width], 
					       renderings->rendering->color_table, 
					       0, RENDERING_DENSITY_MAX);
	  /* compensate for the fact that X defines the origin as top left, not bottom left */
	  location = (image_height-i.y-1)*total_width+i.x+i_eye*eye_width;
	  total_alpha = rgba16_data[location].a + rgba_temp.a;
	  if (i_eye == 0)
	    image_num = 1;
	  else if ((i.x+i_eye*eye_width) >= image_width)
	    image_num = 1;
	  else
	    image_num = 2;

	  if (total_alpha == 0) {
	    rgba16_data[location].r = (((image_num-1)*rgba16_data[location].r + 
					rgba_temp.r)/
				       ((gdouble) image_num));
	    rgba16_data[location].g = (((image_num-1)*rgba16_data[location].g + 
					rgba_temp.g)/
				       ((gdouble) image_num));
	    rgba16_data[location].b = (((image_num-1)*rgba16_data[location].b + 
					rgba_temp.b)/
				       ((gdouble) image_num));
	  } else if (rgba16_data[location].a == 0) {
	    rgba16_data[location].r = rgba_temp.r;
	    rgba16_data[location].g = rgba_temp.g;
	    rgba16_data[location].b = rgba_temp.b;
	    rgba16_data[location].a = rgba_temp.a;
	  } else if (rgba_temp.a != 0) {
	    rgba16_data[location].r = ((rgba16_data[location].r*rgba16_data[location].a 
					+ rgba_temp.r*rgba_temp.a)/
				       ((gdouble) total_alpha));
	    rgba16_data[location].g = ((rgba16_data[location].g*rgba16_data[location].a 
					+ rgba_temp.g*rgba_temp.a)/
				       ((gdouble) total_alpha));
	    rgba16_data[location].b = ((rgba16_data[location].b*rgba16_data[location].a 
					+ rgba_temp.b*rgba_temp.a)/
				       ((gdouble) total_alpha));
	    rgba16_data[location].a = total_alpha;
	  }
	}
    }      
    renderings = renderings->next;
  }

  /* allocate space for the true rgb buffer */
  if ((char_data = g_try_new(guchar,3*image_height * total_width)) == NULL) {
    g_warning(_("couldn't allocate memory for char_data for rendering image"));
    g_free(rgba16_data);
    return NULL;
  }

  /* now convert our temp rgb data to real rgb data */
  for (j=0; j<total_width*image_height; j++) {
    char_data[3*j+0] = rgba16_data[j].r < 0xFF ? rgba16_data[j].r : 0xFF;
    char_data[3*j+1] = rgba16_data[j].g < 0xFF ? rgba16_data[j].g : 0xFF;
    char_data[3*j+2] = rgba16_data[j].b < 0xFF ? rgba16_data[j].b : 0xFF;
  }

  /* from the rgb_data, generate a GdkPixbuf */
  temp_image = gdk_pixbuf_new_from_data(char_data, GDK_COLORSPACE_RGB,FALSE,8,
					total_width,image_height,total_width*3*sizeof(guchar),
					image_free_rgb_data, NULL);
  
  /* free up any unneeded allocated memory */
  g_free(rgba16_data);
  
  return temp_image;
}
#endif

/* function to make the bar graph to put next to the color_strip image */
GdkPixbuf * image_of_distribution(AmitkDataSet * ds, rgb_t fg,
				  AmitkUpdateFunc update_func,
				  gpointer update_data) {

  GdkPixbuf * temp_image;
  guchar * rgba_data;
  amide_intpoint_t k,l;
  AmitkVoxel j;
  amide_data_t max, scale;
  AmitkRawData * distribution;
  gint dim_x;

  /* make sure we have a distribution calculated */
  amitk_data_set_calc_distribution(ds, update_func, update_data);
  distribution = AMITK_DATA_SET_DISTRIBUTION(ds);
  if(distribution==NULL) {
    dim_x = AMITK_DATA_SET_DISTRIBUTION_SIZE;
  } else {
    dim_x = AMITK_RAW_DATA_DIM_X(distribution);
  }

  if ((rgba_data = g_try_new(guchar,4*IMAGE_DISTRIBUTION_WIDTH*dim_x)) == NULL) {
    g_warning(_("couldn't allocate memory for rgba_data for bar_graph"));
    return NULL;
  }

  if (distribution == NULL) {
    for (l=0 ; l < dim_x ; l++) {
      for (k=0; k < IMAGE_DISTRIBUTION_WIDTH; k++) {
	rgba_data[l*IMAGE_DISTRIBUTION_WIDTH*4+k*4+0] = 0x00;
	rgba_data[l*IMAGE_DISTRIBUTION_WIDTH*4+k*4+1] = 0x00;
	rgba_data[l*IMAGE_DISTRIBUTION_WIDTH*4+k*4+2] = 0x00;
	rgba_data[l*IMAGE_DISTRIBUTION_WIDTH*4+k*4+3] = 0x00;
      }
    }
  } else {
    /* figure out the max of the distribution, so we can normalize the distribution to the width */
    max = 0.0;
    j.t = j.g = j.z = j.y = 0;
    for (j.x = 0; j.x < dim_x ; j.x++) 
    if (*AMITK_RAW_DATA_DOUBLE_POINTER(distribution,j) > max)
      max = *AMITK_RAW_DATA_DOUBLE_POINTER(distribution,j);
    if (max/IMAGE_DISTRIBUTION_WIDTH != 0.0)
      scale = ((gdouble) IMAGE_DISTRIBUTION_WIDTH)/max;
    else
      scale = 0;
    
    /* figure out what the rgb data is */
    j.t = j.g = j.z = j.y = 0;
    for (l=0 ; l < dim_x ; l++) {
      j.x = dim_x-l-1;
      for (k=0; k < floor(((gdouble) IMAGE_DISTRIBUTION_WIDTH)-
			  scale*(*AMITK_RAW_DATA_DOUBLE_POINTER(distribution,j))) ; k++) {
	rgba_data[l*IMAGE_DISTRIBUTION_WIDTH*4+k*4+0] = 0x00;
	rgba_data[l*IMAGE_DISTRIBUTION_WIDTH*4+k*4+1] = 0x00;
	rgba_data[l*IMAGE_DISTRIBUTION_WIDTH*4+k*4+2] = 0x00;
	rgba_data[l*IMAGE_DISTRIBUTION_WIDTH*4+k*4+3] = 0x00;
      }
      for ( ; k < IMAGE_DISTRIBUTION_WIDTH ; k++) {
	rgba_data[l*IMAGE_DISTRIBUTION_WIDTH*4+k*4+0] = fg.r;
	rgba_data[l*IMAGE_DISTRIBUTION_WIDTH*4+k*4+1] = fg.g;
	rgba_data[l*IMAGE_DISTRIBUTION_WIDTH*4+k*4+2] = fg.b;
	rgba_data[l*IMAGE_DISTRIBUTION_WIDTH*4+k*4+3] = 0xFF;
      }
    }
  }

  /* generate the pixbuf image */
  temp_image = gdk_pixbuf_new_from_data(rgba_data, GDK_COLORSPACE_RGB,
					TRUE,8,IMAGE_DISTRIBUTION_WIDTH,
					dim_x,
					IMAGE_DISTRIBUTION_WIDTH*4*sizeof(guchar),
					image_free_rgb_data, NULL);
  
  if (temp_image == NULL) g_error("NULL Distribution Image created\n");

  return temp_image;
}


/* function to make the color_strip image */
GdkPixbuf * image_from_colortable(const AmitkColorTable color_table,
				  const amide_intpoint_t width, 
				  const amide_intpoint_t height,
				  const amide_data_t min,
				  const amide_data_t max,
				  const amide_data_t data_set_min,
				  const amide_data_t data_set_max,
				  const gboolean horizontal) {

  amide_intpoint_t i,j;
  rgba_t rgba;
  GdkPixbuf * temp_image;
  amide_data_t datum;
  guchar * rgb_data;

  if ((rgb_data = g_try_new(guchar,3*width*height)) == NULL) {
    g_warning(_("couldn't allocate memory for rgb_data for color_strip"));
    return NULL;
  }

  if (horizontal) {
      for (i=0; i < width; i++) {
      datum = ((((gdouble) width-i)/width) * (data_set_max-data_set_min))+data_set_min;
      datum = (data_set_max-data_set_min)*(datum-min)/(max-min)+data_set_min;
      
      rgba = amitk_color_table_lookup(datum, color_table, data_set_min, data_set_max);
      for (j=0; j < height; j++) {
	rgb_data[j*width*3+i*3+0] = rgba.r;
	rgb_data[j*width*3+i*3+1] = rgba.g;
	rgb_data[j*width*3+i*3+2] = rgba.b;
      }
    }
  } else {
    for (j=0; j < height; j++) {
      datum = ((((gdouble) height-j)/height) * (data_set_max-data_set_min))+data_set_min;
      datum = (data_set_max-data_set_min)*(datum-min)/(max-min)+data_set_min;
      
      rgba = amitk_color_table_lookup(datum, color_table, data_set_min, data_set_max);
      for (i=0; i < width; i++) {
	rgb_data[j*width*3+i*3+0] = rgba.r;
	rgb_data[j*width*3+i*3+1] = rgba.g;
	rgb_data[j*width*3+i*3+2] = rgba.b;
      }
    }
  }

  
  temp_image = gdk_pixbuf_new_from_data(rgb_data, GDK_COLORSPACE_RGB,
					FALSE,8,width,height,width*3*sizeof(guchar),
					image_free_rgb_data, NULL);
  
  return temp_image;
}


GdkPixbuf * image_from_projection(AmitkDataSet * projection) {

  guchar * rgb_data;
  AmitkVoxel i;
  AmitkVoxel dim;
  amide_data_t max,min;
  GdkPixbuf * temp_image;
  rgba_t rgba_temp;
  AmitkColorTable color_table;
  
  /* sanity checks */
  g_return_val_if_fail(AMITK_IS_DATA_SET(projection), NULL);
  
  dim = AMITK_DATA_SET_DIM(projection);

  if ((rgb_data = g_try_new(guchar,3*dim.x*dim.y)) == NULL) {
    g_warning(_("couldn't allocate memory for rgba_data for projection image"));
    return NULL;
  }

  amitk_data_set_get_thresholding_min_max(projection, projection,
					  AMITK_DATA_SET_SCAN_START(projection),
					  amitk_data_set_get_frame_duration(projection,0),
					  &min, &max);
      
  color_table = AMITK_DATA_SET_COLOR_TABLE(projection, AMITK_VIEW_MODE_SINGLE);

  i.t = i.g = i.z = 0;
  for (i.y = 0; i.y < dim.y; i.y++) 
    for (i.x = 0; i.x < dim.x; i.x++) {
      rgba_temp =
	amitk_color_table_lookup(AMITK_DATA_SET_DOUBLE_0D_SCALING_CONTENT(projection,i), 
				 color_table,min, max);
	  
      /* compensate for the fact that X defines the origin as top left, not bottom left */
	rgb_data[(dim.y-i.y-1)*dim.x*3 + i.x*3+0] = rgba_temp.r;
	rgb_data[(dim.y-i.y-1)*dim.x*3 + i.x*3+1] = rgba_temp.g;
	rgb_data[(dim.y-i.y-1)*dim.x*3 + i.x*3+2] = rgba_temp.b;
    }

  /* from the rgb_data, generate a GdkPixbuf */
  temp_image = gdk_pixbuf_new_from_data(rgb_data, GDK_COLORSPACE_RGB,
  					FALSE,8,dim.x,dim.y,dim.x*3*sizeof(guchar),
  					image_free_rgb_data, NULL);

  return temp_image;
}


GdkPixbuf * image_from_slice(AmitkDataSet * slice, AmitkViewMode view_mode) {

  guchar * rgba_data;
  AmitkVoxel i;
  AmitkVoxel dim;
  amide_data_t max,min;
  GdkPixbuf * temp_image;
  guint index;
  rgba_t rgba_temp;
  AmitkColorTable color_table;

  /* sanity checks */
  g_return_val_if_fail(AMITK_IS_DATA_SET(slice), NULL);
  g_return_val_if_fail(AMITK_DATA_SET_SLICE_PARENT(slice) != NULL, NULL);
  
  dim = AMITK_DATA_SET_DIM(slice);

  if ((rgba_data = g_try_new(guchar,4*dim.x*dim.y)) == NULL) {
    g_warning(_("couldn't allocate memory for rgba_data for slice image"));
    return NULL;
  }

  amitk_data_set_get_thresholding_min_max(AMITK_DATA_SET_SLICE_PARENT(slice),
					  AMITK_DATA_SET(slice),
					  AMITK_DATA_SET_SCAN_START(slice),
					  amitk_data_set_get_frame_duration(slice,0),
					  &min, &max);
      
  color_table = amitk_data_set_get_color_table_to_use(AMITK_DATA_SET_SLICE_PARENT(slice), view_mode);

  i.t = i.g = i.z = 0;
  index=0;

  /* compensate for the fact that X defines the origin as top left, not bottom left */
  for (i.y = dim.y-1; i.y >= 0; i.y--) 
    for (i.x = 0; i.x < dim.x; i.x++, index+=4) {
      rgba_temp =
	amitk_color_table_lookup(AMITK_DATA_SET_DOUBLE_0D_SCALING_CONTENT(slice,i), color_table,min, max);
      
	rgba_data[index+0] = rgba_temp.r;
	rgba_data[index+1] = rgba_temp.g;
	rgba_data[index+2] = rgba_temp.b;
	rgba_data[index+3] = rgba_temp.a;
    }

  /* from the rgb_data, generate a GdkPixbuf */
  temp_image = gdk_pixbuf_new_from_data(rgba_data, GDK_COLORSPACE_RGB,
  					TRUE,8,dim.x,dim.y,dim.x*4*sizeof(guchar),
  					image_free_rgb_data, NULL);

  return temp_image;
}

/* note, generally call this function with gate -1, only use the gate
   parameter if you want to override the data set's specified gate */
GdkPixbuf * image_from_data_sets(GList ** pdisp_slices,
				 GList ** pslice_cache,
				 const gint max_slice_cache_size,
				 GList * objects,
				 const AmitkDataSet * active_ds,
				 const amide_time_t start,
				 const amide_time_t duration,
				 const amide_intpoint_t gate,
				 const amide_real_t pixel_size,
				 const AmitkVolume * view_volume,
				 const AmitkFuseType fuse_type,
				 const AmitkViewMode view_mode) {

  gint slice_num;
  guint32 total_alpha;
  guchar * rgb_data;
  rgba16_t * rgba16_data;
  guint location;
  AmitkVoxel i;
  AmitkVoxel dim;
  amide_data_t max,min;
  GdkPixbuf * temp_image;
  rgba_t rgba_temp;
  GList * slices;
  GList * temp_slices;
  AmitkDataSet * slice;
  AmitkColorTable color_table;
  AmitkDataSet * overlay_slice = NULL;
  gint j;
  AmitkCanvasPoint pixel_size2;
  

  /* sanity checks */
  g_return_val_if_fail(objects != NULL, NULL);

  pixel_size2.x = pixel_size2.y = pixel_size;
  slices = amitk_data_sets_get_slices(objects, pslice_cache, max_slice_cache_size,
				      start, duration, gate, pixel_size2,view_volume);
  g_return_val_if_fail(slices != NULL, NULL);

  /* get the dimensions.  since all slices have the same dimensions, we'll just get the first */
  dim = AMITK_DATA_SET_DIM(slices->data);

  /* allocate and initialize space for a temporary storage buffer */
  rgba16_data = g_try_new(rgba16_t,dim.y*dim.x);
  g_return_val_if_fail(rgba16_data != NULL, NULL);

  for (j=0; j<dim.y*dim.x; j++) {
    rgba16_data[j].r = 0;
    rgba16_data[j].g = 0;
    rgba16_data[j].b = 0;
    rgba16_data[j].a = 0;
  }

  /* iterate through all the slices */
  temp_slices = slices;
  slice_num = 0;

  while (temp_slices != NULL) {
    slice = temp_slices->data;
    if ((fuse_type == AMITK_FUSE_TYPE_OVERLAY) && (AMITK_DATA_SET_SLICE_PARENT(slice) == active_ds)) {
      overlay_slice = slice;
    } else { /* blend this slice */
      slice_num++;

      amitk_data_set_get_thresholding_min_max(AMITK_DATA_SET_SLICE_PARENT(slice),
					      AMITK_DATA_SET(slice),
					      start, duration, &min, &max);
      
      
      color_table = amitk_data_set_get_color_table_to_use(AMITK_DATA_SET_SLICE_PARENT(slice), view_mode);
      /* now add this slice into the rgba16 data */
      i.t = i.g = i.z = 0;
      location=0;
      /* compensate for the fact that X defines the origin as top left, not bottom left */
      for (i.y = dim.y-1; i.y >= 0; i.y--) 
	for (i.x = 0; i.x < dim.x; i.x++, location++) {
	  rgba_temp = 
	    amitk_color_table_lookup(AMITK_DATA_SET_DOUBLE_0D_SCALING_CONTENT(slice,i), color_table,min, max);
	  
	  total_alpha = rgba16_data[location].a + rgba_temp.a;
	  if (total_alpha == 0) {
	    rgba16_data[location].r = (((slice_num-1)*rgba16_data[location].r + 
					rgba_temp.r)/
				       ((gdouble) slice_num));
	    rgba16_data[location].g = (((slice_num-1)*rgba16_data[location].g + 
					rgba_temp.g)/
				       ((gdouble) slice_num));
	    rgba16_data[location].b = (((slice_num-1)*rgba16_data[location].b + 
					rgba_temp.b)/
				       ((gdouble) slice_num));
	  } else if (rgba16_data[location].a == 0) {
	    rgba16_data[location].r = rgba_temp.r;
	    rgba16_data[location].g = rgba_temp.g;
	    rgba16_data[location].b = rgba_temp.b;
	    rgba16_data[location].a = rgba_temp.a;
	  } else if (rgba_temp.a != 0) {
	    rgba16_data[location].r = ((rgba16_data[location].r*rgba16_data[location].a 
					+ rgba_temp.r*rgba_temp.a)/
				       ((gdouble) total_alpha));
	    rgba16_data[location].g = ((rgba16_data[location].g*rgba16_data[location].a 
					+ rgba_temp.g*rgba_temp.a)/
				       ((gdouble) total_alpha));
	    rgba16_data[location].b = ((rgba16_data[location].b*rgba16_data[location].a 
					+ rgba_temp.b*rgba_temp.a)/
				       ((gdouble) total_alpha));
	    rgba16_data[location].a = total_alpha;
	  }
	}
    }
    temp_slices = temp_slices->next;
  }

  /* allocate space for the true rgb buffer */
  rgb_data = g_try_new(guchar,3*dim.y*dim.x);
  g_return_val_if_fail(rgb_data != NULL, NULL);

  /* now convert our temp rgb data to real rgb data */
  i.z = 0;
  location=0;
  for (i.y = 0; i.y < dim.y; i.y++) 
    for (i.x = 0; i.x < dim.x; i.x++, location++) {
      rgb_data[3*location+0] = rgba16_data[location].r < 0xFF ? rgba16_data[location].r : 0xFF;
      rgb_data[3*location+1] = rgba16_data[location].g < 0xFF ? rgba16_data[location].g : 0xFF;
      rgb_data[3*location+2] = rgba16_data[location].b < 0xFF ? rgba16_data[location].b : 0xFF;
    }

  /* if we have a data set we're overlaying, add it in now */
  if (overlay_slice != NULL) {
      amitk_data_set_get_thresholding_min_max(AMITK_DATA_SET_SLICE_PARENT(overlay_slice),
					      AMITK_DATA_SET(overlay_slice),
					      start, duration, &min, &max);
      
      color_table = amitk_data_set_get_color_table_to_use(AMITK_DATA_SET_SLICE_PARENT(overlay_slice), view_mode);

      i.t = i.g = i.z = 0;
      for (i.y = 0; i.y < dim.y; i.y++) 
	for (i.x = 0; i.x < dim.x; i.x++) {
	  rgba_temp = 
	    amitk_color_table_lookup(AMITK_DATA_SET_DOUBLE_0D_SCALING_CONTENT(overlay_slice,i), 
				     color_table,min, max);

	  /* compensate for the fact that X defines the origin as top left, not bottom left */
	  location = (dim.y - i.y - 1)*dim.x+i.x;
	  if (rgba_temp.a != 0) {
	    rgb_data[3*location+0] = rgba_temp.r;
	    rgb_data[3*location+1] = rgba_temp.g;
	    rgb_data[3*location+2] = rgba_temp.b;
	  }
	}
  }
  

  /* from the rgb_data, generate a GdkPixbuf */
  temp_image = gdk_pixbuf_new_from_data(rgb_data, GDK_COLORSPACE_RGB,
  					FALSE,8,dim.x,dim.y,dim.x*3*sizeof(guchar),
  					image_free_rgb_data, NULL);

  /* cleanup */
  g_free(rgba16_data);

  if (pdisp_slices != NULL) {
    amitk_objects_unref((*pdisp_slices));
    *pdisp_slices = slices; 
  } else {
    amitk_objects_unref(slices);
  }

  return temp_image;
}








/* get the icon to use for this modality */
GdkPixbuf * image_get_data_set_pixbuf(AmitkDataSet * ds) {

  GdkPixbuf * pixbuf;
  guchar * object_icon_data;

  g_return_val_if_fail(AMITK_IS_DATA_SET(ds), NULL);

  switch (AMITK_DATA_SET_MODALITY(ds)) {
  case AMITK_MODALITY_SPECT:
    object_icon_data = SPECT_icon_data;
    break;
  case AMITK_MODALITY_MRI:
    object_icon_data = MRI_icon_data;
    break;
  case AMITK_MODALITY_PET:
    object_icon_data = PET_icon_data;
    break;
  case AMITK_MODALITY_CT:
  case AMITK_MODALITY_OTHER:
  default:
    object_icon_data = CT_icon_data;
      break;
  }
  pixbuf = image_from_8bit(object_icon_data, 
			   OBJECT_ICON_XSIZE,OBJECT_ICON_YSIZE,
			   AMITK_DATA_SET_COLOR_TABLE(ds, AMITK_VIEW_MODE_SINGLE));

  return pixbuf;

}
