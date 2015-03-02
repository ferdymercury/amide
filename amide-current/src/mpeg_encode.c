/* mpeg_encode.c - interface to the mpeg_encode application
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

#ifdef AMIDE_MPEG_ENCODE_SUPPORT

#include <sys/stat.h>
#include "ui_common.h"
#include "mpeg_encode.h"



/* encodes all the filenames specified in the file list and
   deletes the files.  If clean_only is true, no mpeg will be
   encoded, by the files will still be deleted */
void mpeg_encode(const gchar * temp_dir, GList * file_list, 
		 gchar * output_filename, GTimeVal current_time,
		 gboolean clean_only) {

  gint i;
  gchar * temp_string;
  struct stat file_info;
  FILE * param_file;
  GList * temp_list;
  gchar * param_filename = NULL;
  gchar * frame_filename;
  gint return_val = 1;

  if (!clean_only) {
    i = 0;
    do { /* get a unique file name */
      if (i > 0) g_free(param_filename); 
      i++;
      param_filename = g_strdup_printf("%s/%ld_%d_amide_rendering.param",
				       temp_dir, current_time.tv_sec, i);
    } while (stat(param_filename, &file_info) == 0);
    
    /* open the parameter file for writing */
    if ((param_file = fopen(param_filename, "w")) != NULL) {
      fprintf(param_file, "PATTERN\tIBBPBBPBBPBBPBBP\n");
      fprintf(param_file, "OUTPUT\t%s\n",output_filename);
      fprintf(param_file, "INPUT_DIR\t%s\n","");
      fprintf(param_file, "INPUT\n");
      
      temp_list = file_list;
      while (temp_list != NULL) {
	frame_filename = temp_list->data;
	fprintf(param_file, "%s\n",frame_filename);
	temp_list = temp_list->next;
      }
      fprintf(param_file, "END_INPUT\n");
      fprintf(param_file, "BASE_FILE_FORMAT\tJPEG\n");
      fprintf(param_file, "INPUT_CONVERT\t*\n");
      fprintf(param_file, "GOP_SIZE\t16\n");
      fprintf(param_file, "SLICES_PER_FRAME\t1\n");
      fprintf(param_file, "PIXEL\tHALF\n");
      fprintf(param_file, "RANGE\t10\n");
      fprintf(param_file, "PSEARCH_ALG\tEXHAUSTIVE\n");
      fprintf(param_file, "BSEARCH_ALG\tCROSS2\n");
      fprintf(param_file, "IQSCALE\t8\n");
      fprintf(param_file, "PQSCALE\t10\n");
      fprintf(param_file, "BQSCALE\t25\n");
      fprintf(param_file, "BQSCALE\t25\n");
      fprintf(param_file, "REFERENCE_FRAME\tDECODED\n");
      fprintf(param_file, "FRAME_RATE\t%d\n",FRAMES_PER_SECOND);
      fprintf(param_file, "FORCE_ENCODE_LAST_FRAME\n");
      
      file_list = g_list_append(file_list, param_filename);
      fclose(param_file);
      
      /* delete the previous .mpg file if it existed */
      if (stat(output_filename, &file_info) == 0) {
	if (S_ISREG(file_info.st_mode)) {
	  if (unlink(output_filename) != 0) {
	    g_warning("Couldn't unlink file: %s",output_filename);
	    return_val = -1;
	  }
	} else {
	  g_warning("Unrecognized file type for file: %s, couldn't delete", output_filename);
	  return_val = -1;
	}
      }
      
      if (return_val != -1) {
	/* and run the mpeg encoding process */
	temp_string = g_strdup_printf("%s -quiet 1 -no_frame_summary %s",AMIDE_MPEG_ENCODE_BIN, param_filename);
	return_val = system(temp_string);
	g_free(temp_string);
	
	if ((return_val == -1) || (return_val == 127)) {
	  g_warning("executing of %s for creation of mpeg movie was unsucessful", 
		    AMIDE_MPEG_ENCODE_BIN);
	  return_val = -1;
	}
      }
    }
  }

  /* do some cleanups */
  while (file_list != NULL) {
    frame_filename = file_list->data;
    file_list = g_list_remove(file_list, frame_filename);
    unlink(frame_filename);
    g_free(frame_filename);
  }
    
  return;
}












#endif /* AMIDE_MPEG_ENCODE_SUPPORT */

