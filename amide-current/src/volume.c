/* volume.c
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

#include <sys/time.h>
#include <sys/stat.h>
#include "config.h"
#include <glib.h>
#include <math.h>
#include <sys/stat.h>
#ifdef AMIDE_DEBUG
#include <sys/timeb.h>
#endif
#include "amide.h"
#include "color_table.h"
#include "volume.h"
#include "objects.h"
#include "raw_data.h"

/* external variables */
gchar * interpolation_names[] = {"Nearest Neighbhor", 
				 "2x2x1 Filter", 
				 "2x2x2 Filter", 
				 //				 "Bilinear", 
				 "Trilinear"};

gchar * interpolation_explanations[] = {
  "interpolate using nearest neighbhor (fast)", 
  "interpolate using a 2x2x1 blurring matrix (medium)",
  "interpolate using a 2x2x2 blurring matrix (slow)",
  //  "interpolate using bilinear interpolation (middle)",
  "interpolate using trilinear interpolation (slow)"
};

gchar * modality_names[] = {"PET", \
			    "SPECT", \
			    "CT", \
			    "MRI", \
			    "Other"};

/* removes a reference to a volume, frees up the volume if no more references */
volume_t * volume_free(volume_t * volume) {

  if (volume == NULL)
    return volume;

  /* sanity checks */
  g_return_val_if_fail(volume->reference_count > 0, NULL);
  
  /* remove a reference count */
  volume->reference_count--;

  /* if we've removed all reference's, free the volume */
  if (volume->reference_count == 0) {
#ifdef AMIDE_DEBUG
    g_print("freeing volume: %s\n",volume->name);
#endif
    g_free(volume->data);
    g_free(volume->frame_duration);
    g_free(volume->distribution);
    g_free(volume->scan_date);
    g_free(volume->name);
    g_free(volume);
    volume = NULL;
  }

  return volume;
}

/* returns an initialized volume structure */
volume_t * volume_init(void) {

  volume_t * temp_volume;
  axis_t i_axis;
  time_t current_time;

  if ((temp_volume = 
       (volume_t *) g_malloc(sizeof(volume_t))) == NULL) {
    g_warning("%s: Couldn't allocate memory for the volume structure",PACKAGE);
    return NULL;
  }
  temp_volume->reference_count = 1;

  /* put in some sensable values */
  temp_volume->name = NULL;
  temp_volume->scan_date = NULL;
  temp_volume->modality = PET;
  temp_volume->voxel_size = realpoint_init;
  temp_volume->dim = voxelpoint_init;
  temp_volume->num_frames = 0;
  temp_volume->data = NULL;
  temp_volume->conversion = 1.0;
  temp_volume->scan_start = 0.0;
  temp_volume->frame_duration = NULL;
  temp_volume->color_table = BW_LINEAR;
  temp_volume->distribution = NULL;
  temp_volume->coord_frame.offset=realpoint_init;
  for (i_axis=0;i_axis<NUM_AXIS;i_axis++)
    temp_volume->coord_frame.axis[i_axis] = default_axis[i_axis];
  temp_volume->corner = realpoint_init;

  /* set the scan date to the current time, good for an initial guess */
  time(&current_time);
  volume_set_scan_date(temp_volume, ctime(&current_time));
  g_strdelimit(temp_volume->scan_date, "\n", ' '); /* turns newlines to white space */
  g_strstrip(temp_volume->scan_date); /* removes trailing and leading white space */

  return temp_volume;
}


/* function to write out the information content of a volume into an xml
   file.  Returns a string containing the name of the file. */
gchar * volume_write_xml(volume_t * volume, gchar * directory) {

  gchar * file_name;
  gchar * raw_data_name;
  guint count;
  struct stat file_info;
  xmlDocPtr doc;
  guint num_elements;
  FILE * file_pointer;

  /* make a guess as to our filename */
  count = 1;
  file_name = g_strdup_printf("Volume_%s.xml", volume->name);
  raw_data_name = g_strdup_printf("Volume_%s.dat", volume->name);

  /* see if this file already exists */
  while (stat(file_name, &file_info) == 0) {
    g_free(file_name);
    g_free(raw_data_name);
    count++;
    file_name = g_strdup_printf("Volume_%s_%d.xml", volume->name, count);
    raw_data_name = g_strdup_printf("Volume_%s_%d.dat", volume->name, count);
  }

  /* and we now have unique filenames */
#ifdef AMIDE_DEBUG
  g_print("\t- saving volume %s in file %s\n",volume->name, file_name);
#endif

  /* write the volume xml file */
  doc = xmlNewDoc("1.0");
  doc->children = xmlNewDocNode(doc, NULL, "Volume", volume->name);
  xml_save_string(doc->children, "scan_date", volume->scan_date);
  xml_save_string(doc->children, "modality", modality_names[volume->modality]);
  xml_save_data(doc->children, "conversion", volume->conversion);
  xml_save_realpoint(doc->children, "voxel_size", volume->voxel_size);
  xml_save_voxelpoint(doc->children, "dim", volume->dim);
  xml_save_int(doc->children, "num_frames", volume->num_frames);
  xml_save_time(doc->children, "scan_start", volume->scan_start);
  xml_save_times(doc->children, "frame_duration", volume->frame_duration, volume->num_frames);
  xml_save_string(doc->children, "color_table", color_table_names[volume->color_table]);
  xml_save_data(doc->children, "max", volume->max);
  xml_save_data(doc->children, "min", volume->min);
  xml_save_data(doc->children, "threshold_max", volume->threshold_max);
  xml_save_data(doc->children, "threshold_min", volume->threshold_min);
  xml_save_realspace(doc->children, "coord_frame", volume->coord_frame);
  xml_save_realpoint(doc->children, "corner", volume->corner);

  /* store the name of our associated data file */
  xml_save_string(doc->children, "raw_data", raw_data_name);
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
  xml_save_string(doc->children, "data_format", data_format_names[FLOAT_BE]);
#else
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
  xml_save_string(doc->children, "data_format", data_format_names[FLOAT_LE]);
#else
#error "must specify G_LITTLE_ENDIAN or G_BIG_ENDIAN)"
#endif
#endif

  /* and save */
  xmlSaveFile(file_name, doc);

  /* and we're done with the xml stuff*/
  xmlFreeDoc(doc);

  /* now to save the raw data */

  /* sanity check */
#if (SIZE_OF_VOLUME_DATA_T != 4)
#error "Haven't yet written support for 8 byte (double) volume data in volume_write_xml"
#endif
  
  /* write it on out */
  if ((file_pointer = fopen(raw_data_name, "w")) == NULL) {
    g_warning("%s: couldn't save raw data file: %s",PACKAGE, raw_data_name);
    g_free(file_name);
    g_free(raw_data_name);
    return NULL;
  }

  num_elements = volume_size_data_mem(volume);
  if (fwrite(volume->data, 1, num_elements, file_pointer) != num_elements) {
    g_warning("%s: incomplete save of raw data file: %s",PACKAGE, raw_data_name);
    g_free(file_name);
    g_free(raw_data_name);
    return NULL;
  }
  fclose(file_pointer);

  g_free(raw_data_name);
  return file_name;
}


/* function to load in a volume xml file */
volume_t * volume_load_xml(gchar * file_name, const gchar * directory) {

  xmlDocPtr doc;
  volume_t * new_volume;
  xmlNodePtr nodes;
  modality_t i_modality;
  color_table_t i_color_table;
  data_format_t i_data_format, data_format;
  gchar * temp_string;
  gchar * raw_data_name;
  gchar * scan_date;
  time_t modification_time;
  struct stat file_info;

  new_volume = volume_init();

  /* parse the xml file */
  if ((doc = xmlParseFile(file_name)) == NULL) {
    g_warning("%s: Couldn't Parse AMIDE volume xml file %s/%s",PACKAGE, directory,file_name);
    volume_free(new_volume);
    return new_volume;
  }

  /* get the root of our document */
  if ((nodes = xmlDocGetRootElement(doc)) == NULL) {
    g_warning("%s: AMIDE volume xml file doesn't appear to have a root: %s/%s",
	      PACKAGE, directory,file_name);
    volume_free(new_volume);
    return new_volume;
  }

  /* get the volume name */
  temp_string = xml_get_string(nodes->children, "text");
  if (temp_string != NULL) {
    volume_set_name(new_volume,temp_string);
    g_free(temp_string);
  }

  /* get the document tree */
  nodes = nodes->children;

  /* get the date the scan was made */
  scan_date = xml_get_string(nodes, "scan_date");

  /* put in the last time the study file was modified,
     if no creation date was specified */
  if (scan_date == NULL) {
    stat(file_name, &file_info);
    modification_time = file_info.st_mtime;
    scan_date = g_strdup(ctime(&modification_time));
    g_strdelimit(scan_date, "\n", ' '); /* turns newlines to white space */
    g_strstrip(scan_date); /* removes trailing and leading white space */
    g_warning("%s: no scan date found in image data, educated guess is %s", 
	      PACKAGE, scan_date);
  }
  volume_set_scan_date(new_volume, scan_date);
  g_free(scan_date);
  

  /* figure out the modality */
  temp_string = xml_get_string(nodes, "modality");
  if (temp_string != NULL)
    for (i_modality=0; i_modality < NUM_MODALITIES; i_modality++) 
      if (g_strcasecmp(temp_string, modality_names[i_modality]) == 0)
	new_volume->modality = i_modality;
  g_free(temp_string);

  /* figure out the color table */
  temp_string = xml_get_string(nodes, "color_table");
  if (temp_string != NULL)
    for (i_color_table=0; i_color_table < NUM_COLOR_TABLES; i_color_table++) 
      if (g_strcasecmp(temp_string, color_table_names[i_color_table]) == 0)
	new_volume->color_table = i_color_table;
  g_free(temp_string);

  /* and figure out the rest of the parameters */

  /* get the rest of the parameters */
  new_volume->voxel_size = xml_get_realpoint(nodes, "voxel_size");
  new_volume->dim = xml_get_voxelpoint(nodes, "dim");
  new_volume->conversion =  xml_get_data(nodes, "conversion");
  new_volume->num_frames = xml_get_int(nodes, "num_frames");
  new_volume->scan_start = xml_get_time(nodes, "scan_start");
  new_volume->frame_duration = xml_get_times(nodes, "frame_duration", new_volume->num_frames);
  new_volume->threshold_max =  xml_get_data(nodes, "threshold_max");
  new_volume->threshold_min =  xml_get_data(nodes, "threshold_min");
  new_volume->coord_frame = xml_get_realspace(nodes, "coord_frame");
  new_volume->corner = xml_get_realpoint(nodes, "corner");

  /* get the name of our associated data file */
  raw_data_name = xml_get_string(nodes, "raw_data");

  /* and figure out the data format */
  temp_string = xml_get_string(nodes, "data_format");
#if (G_BYTE_ORDER == G_BIG_ENDIAN)
  data_format = FLOAT_BE; /* sensible guess in case we don't figure it out from the file */
#else /* (G_BYTE_ORDER == G_LITTLE_ENDIAN) */
  data_format = FLOAT_LE; /* sensible guess in case we don't figure it out from the file */
#endif
  if (temp_string != NULL)
    for (i_data_format=0; i_data_format < NUM_DATA_FORMATS; i_data_format++) 
      if (g_strcasecmp(temp_string, data_format_names[i_data_format]) == 0)
	data_format = i_data_format;
  g_free(temp_string);

  /* now load in the raw data */
#ifdef AMIDE_DEBUG
  g_print("reading volume %s from file %s\n", new_volume->name, raw_data_name);
#endif
  new_volume = raw_data_read_file(raw_data_name, new_volume, data_format, 0);
   
  /* and we're done */
  g_free(raw_data_name);
  xmlFreeDoc(doc);

  return new_volume;
}


/* makes a new volume item which is a copy of a previous volume's information. 
   Notes: 
   - does not make a copy of the source volume's data
   - does not make a copy of the distribution data */
volume_t * volume_copy(volume_t * src_volume) {

  volume_t * dest_volume;
  guint i;

  /* sanity checks */
  g_return_val_if_fail(src_volume != NULL, NULL);

  dest_volume = volume_init();

  /* copy the data elements */
  dest_volume->modality = src_volume->modality;
  dest_volume->voxel_size = src_volume->voxel_size;
  dest_volume->dim = src_volume->dim;
  dest_volume->conversion = src_volume->conversion;
  dest_volume->num_frames = src_volume->num_frames;
  dest_volume->max = src_volume->max;
  dest_volume->min = src_volume->min;
  dest_volume->color_table= src_volume->color_table;
  dest_volume->threshold_max = src_volume->threshold_max;
  dest_volume->threshold_min = src_volume->threshold_min;
  dest_volume->coord_frame = src_volume->coord_frame;
  dest_volume->corner = src_volume->corner;

  /* make a separate copy in memory of the volume's name and scan date */
  volume_set_name(dest_volume, src_volume->name);
  volume_set_scan_date(dest_volume, src_volume->scan_date);

  /* make a separate copy in memory of the volume's frame durations */
  dest_volume->frame_duration = volume_get_frame_duration_mem(dest_volume);
  if (dest_volume->frame_duration == NULL) {
    g_warning("%s: couldn't allocate space for the frame duration info",PACKAGE);
    dest_volume = volume_free(dest_volume);
    return dest_volume;
  }
  for (i=0;i<dest_volume->num_frames;i++)
    dest_volume->frame_duration[i] = src_volume->frame_duration[i];

  return dest_volume;
}

/* adds one to the reference count of a volume */
volume_t * volume_add_reference(volume_t * volume) {

  volume->reference_count++;

  return volume;
}


/* sets the name of a volume
   note: new_name is copied rather then just being referenced by volume */
void volume_set_name(volume_t * volume, gchar * new_name) {

  g_free(volume->name); /* free up the memory used by the old name */
  volume->name = g_strdup(new_name); /* and assign the new name */

  return;
}

/* sets the creation date of a volume
   note: new_date is copied rather then just being referenced by volume 
   note: no error checking is done on the date, as of yet */
void volume_set_scan_date(volume_t * volume, gchar * new_date) {

  g_free(volume->scan_date); /* free up the memory used by the old date */
  volume->scan_date = g_strdup(new_date); /* and assign the new date */

  return;
}



/* figure out the center of the volume in real coords */
realpoint_t volume_calculate_center(const volume_t * volume) {

  realpoint_t center;
  realpoint_t corner[2];

  /* get the far corner (in volume coords) */
  corner[0] = realspace_base_coord_to_alt(volume->coord_frame.offset, volume->coord_frame);
  corner[1] = volume->corner;
 
  /* the center in volume coords is then just half the far corner */
  REALPOINT_MADD(0.5,corner[1], 0.5,corner[0], center);
  
  /* now, translate that into real coords */
  center = realspace_alt_coord_to_base(center, volume->coord_frame);

  return center;
}

/* figure out the real point that corresponds to the voxel coordinates */
inline realpoint_t volume_voxel_to_realpoint(const volume_t * volume, const voxelpoint_t i) {

  realpoint_t real_p;

  real_p.x = (((floatpoint_t) i.x) + 0.5) * volume->voxel_size.x;
  real_p.y = (((floatpoint_t) i.y) + 0.5) * volume->voxel_size.y; 
  real_p.z = (((floatpoint_t) i.z) + 0.5) * volume->voxel_size.z; 

  return real_p;
}

/* figure out the voxel point that corresponds to the real coordinates */
inline voxelpoint_t volume_realpoint_to_voxel(const volume_t * volume, const realpoint_t real) {
  
  voxelpoint_t voxel_p;

  voxel_p.x = floor(real.x/volume->voxel_size.x);
  voxel_p.y = floor(real.y/volume->voxel_size.y);
  voxel_p.z = floor(real.z/volume->voxel_size.z);
  
  return voxel_p;
}

/* check to make sure a given voxel is inside a volume */
inline gboolean volume_includes_voxel(const volume_t * volume, const voxelpoint_t voxel) {

  if ( (voxel.x < 0) || 
       (voxel.y < 0) || 
       (voxel.z < 0) ||
       (voxel.x >= volume->dim.x) || 
       (voxel.y >= volume->dim.y) ||
       (voxel.z >= volume->dim.z))
    return FALSE;
  else
    return TRUE;
}

/* returns the start time of the given frame */
volume_time_t volume_start_time(const volume_t * volume, guint frame) {

  volume_time_t time;
  guint i_frame;

  if (frame >= volume->num_frames)
    frame = volume->num_frames-1;

  time = volume->scan_start;

  for(i_frame=0;i_frame<frame;i_frame++)
    time += volume->frame_duration[i_frame];

  return time;
}

/* returns the end time of a given frame */
volume_time_t volume_end_time(const volume_t * volume, guint frame) {

  if (frame >= volume->num_frames)
    frame = volume->num_frames-1;

  return volume_start_time(volume, frame) + volume->frame_duration[frame];
}

/* return the minimal frame duration in this volume */
volume_time_t volume_min_frame_duration(const volume_t * volume) {

  volume_time_t min_frame_duration;
  guint i_frame;

  min_frame_duration = volume->frame_duration[0];

  for(i_frame=1;i_frame<volume->num_frames;i_frame++)
    if (volume->frame_duration[i_frame] < min_frame_duration)
      min_frame_duration = volume->frame_duration[i_frame];

  return min_frame_duration;
}

/* free up a list of volumes */
volume_list_t * volume_list_free(volume_list_t * volume_list) {
  
  if (volume_list == NULL)
    return volume_list;

  /* sanity check */
  g_return_val_if_fail(volume_list->reference_count > 0, NULL);

  /* remove a reference count */
  volume_list->reference_count--;

  /* things we always do */
  volume_list->volume = volume_free(volume_list->volume);

  /* recursively delete rest of list */
  volume_list->next = volume_list_free(volume_list->next); 

  /* things to do if our reference count is zero */
  if (volume_list->reference_count == 0) {
    g_free(volume_list);
    volume_list = NULL;
  }

  return volume_list;
}

/* returns an initialized volume list node structure */
volume_list_t * volume_list_init(void) {
  
  volume_list_t * temp_volume_list;
  
  if ((temp_volume_list = 
       (volume_list_t *) g_malloc(sizeof(volume_list_t))) == NULL) {
    return NULL;
  }
  temp_volume_list->reference_count = 1;
  
  temp_volume_list->volume = NULL;
  temp_volume_list->next = NULL;
  
  return temp_volume_list;
}

/* function to write a list of volumes as xml data.  Function calls
   volume_write_xml to writeout each volume, and adds information about the
   volume file to the node_list. */
void volume_list_write_xml(volume_list_t *list, xmlNodePtr node_list, gchar * directory) {

  gchar * file_name;

  if (list != NULL) { 
    file_name = volume_write_xml(list->volume, directory);
    xmlNewChild(node_list, NULL,"Volume_file", file_name);
    g_free(file_name);
    
    /* and recurse */
    volume_list_write_xml(list->next, node_list, directory);
  }

  return;
}


/* function to load in a list of volume xml nodes */
volume_list_t * volume_list_load_xml(xmlNodePtr node_list, const gchar * directory) {

  gchar * file_name;
  volume_list_t * new_volume_list;
  volume_t * new_volume;

  if (node_list != NULL) {
    /* first, recurse on through the list */
    new_volume_list = volume_list_load_xml(node_list->next, directory);

    /* load in this node */
    file_name = xml_get_string(node_list->children, "text");
    new_volume = volume_load_xml(file_name,directory);
    new_volume_list = volume_list_add_volume_first(new_volume_list, new_volume);
    new_volume = volume_free(new_volume);
    g_free(file_name);

  } else
    new_volume_list = NULL;

  return new_volume_list;
}


/* adds one to the reference count of a volume list element*/
volume_list_t * volume_list_add_reference(volume_list_t * volume_list_element) {

  volume_list_element->reference_count++;

  return volume_list_element;
}


/* function to check that a volume is in a volume list */
gboolean volume_list_includes_volume(volume_list_t * volume_list,  volume_t * volume) {

  while (volume_list != NULL)
    if (volume_list->volume == volume)
      return TRUE;
    else
      volume_list = volume_list->next;

  return FALSE;
}


/* function to add a volume to the end of a volume list */
volume_list_t * volume_list_add_volume(volume_list_t * volume_list, volume_t * vol) {

  volume_list_t * temp_list = volume_list;
  volume_list_t * prev_list = NULL;

  /* get to the end of the list */
  while (temp_list != NULL) {
    prev_list = temp_list;
    temp_list = temp_list->next;
  }
  
  /* get a new volume_list data structure */
  temp_list = volume_list_init();

  /* add the volume to this new list item */
  temp_list->volume = volume_add_reference(vol);

  if (volume_list == NULL)
    return temp_list;
  else {
    prev_list->next = temp_list;
    return volume_list;
  }
}


/* function to add a volume onto a volume list as the first item*/
volume_list_t * volume_list_add_volume_first(volume_list_t * volume_list, volume_t * vol) {

  volume_list_t * temp_list;

  temp_list = volume_list_add_volume(NULL,vol);
  temp_list->next = volume_list;

  return temp_list;
}


/* function to remove a volume list item from a volume list */
volume_list_t * volume_list_remove_volume(volume_list_t * volume_list, volume_t * vol) {

  volume_list_t * temp_list = volume_list;
  volume_list_t * prev_list = NULL;

  while (temp_list != NULL) {
    if (temp_list->volume == vol) {
      if (prev_list == NULL)
	volume_list = temp_list->next;
      else
	prev_list->next = temp_list->next;
      temp_list->next = NULL;
      temp_list = volume_list_free(temp_list);
    } else {
      prev_list = temp_list;
      temp_list = temp_list->next;
    }
  }

  return volume_list;
}


/* makes a new volume_list which is a copy of a previous volume_list */
volume_list_t * volume_list_copy(volume_list_t * src_volume_list) {

  volume_list_t * dest_volume_list;

  /* sanity check */
  g_return_val_if_fail(src_volume_list != NULL, NULL);

  dest_volume_list = volume_list_init();

  /* make a separate copy in memory of the volume this list item points to */
  dest_volume_list->volume = volume_copy(src_volume_list->volume);
    
  /* and make copies of the rest of the elements in this list */
  if (src_volume_list->next != NULL)
    dest_volume_list->next = volume_list_copy(src_volume_list->next);

  return dest_volume_list;
}


/* function to get the earliest time in a list of volumes */
volume_time_t volume_list_start_time(volume_list_t * volume_list) {
  volume_time_t start, temp;

  /* sanity check */
  g_return_val_if_fail(volume_list != NULL, 0.0);

  start = volume_start_time(volume_list->volume,0);
  if (volume_list->next != NULL) {
    temp = volume_list_start_time(volume_list->next);
    if (temp < start)
      start = temp;
  }
  
  return start;
}

/* function to get the latest time in a list of volumes */
volume_time_t volume_list_end_time(volume_list_t * volume_list) {
  volume_time_t end, temp;

  /* sanity check */
  g_return_val_if_fail(volume_list != NULL, 0.0);

  end = volume_end_time(volume_list->volume,volume_list->volume->num_frames-1);
  if (volume_list->next != NULL) {
    temp = volume_list_end_time(volume_list->next);
    if (temp > end)
      end = temp;
  }
  
  return end;
}

/* function to return the minimum frame duration of a list of volumes */
volume_time_t volume_list_min_frame_duration(volume_list_t * volume_list) {
  volume_time_t min_duration, temp;

  /* sanity check */
  g_return_val_if_fail(volume_list != NULL, 0.0);

  min_duration = volume_min_frame_duration(volume_list->volume);
  if (volume_list->next != NULL) {
    temp = volume_list_min_frame_duration(volume_list->next);
    if (temp < min_duration)
      min_duration = temp;
  }
  
  return min_duration;
}



/* takes a volume and a view_axis, and gives the corners necessary for
   the view to totally encompass the volume in the base coord frame */
void volume_get_view_corners(const volume_t * volume,
			     const realspace_t view_coord_frame,
			     realpoint_t view_corner[]) {

  realpoint_t volume_corner[2];

  g_assert(volume!=NULL);

  volume_corner[0] = realspace_base_coord_to_alt(volume->coord_frame.offset,
						 volume->coord_frame);
  volume_corner[1] = volume->corner;

  /* look at all eight corners of our cube, figure out the min and max coords */
  realspace_get_enclosing_corners(volume->coord_frame, volume_corner,
				  view_coord_frame, view_corner);

  /* and return the corners in the base coord frame */
  view_corner[0] = realspace_alt_coord_to_base(view_corner[0], view_coord_frame);
  view_corner[1] = realspace_alt_coord_to_base(view_corner[1], view_coord_frame);
  return;
}


/* takes a list of volumes and a view axis, and give the corners
   necessary to totally encompass the volume in the base coord frame */
void volumes_get_view_corners(volume_list_t * volumes,
			      const realspace_t view_coord_frame,
			      realpoint_t view_corner[]) {

  volume_list_t * temp_volumes;
  realpoint_t temp_corner[2];

  g_assert(volumes!=NULL);

  temp_volumes = volumes;
  volume_get_view_corners(temp_volumes->volume,view_coord_frame,view_corner);
  view_corner[0] = realspace_base_coord_to_alt(view_corner[0], view_coord_frame);
  view_corner[1] = realspace_base_coord_to_alt(view_corner[1], view_coord_frame);
  temp_volumes = volumes->next;

  while (temp_volumes != NULL) {
    volume_get_view_corners(temp_volumes->volume,view_coord_frame,temp_corner);
    temp_corner[0] = realspace_base_coord_to_alt(temp_corner[0], view_coord_frame);
    temp_corner[1] = realspace_base_coord_to_alt(temp_corner[1], view_coord_frame);
    view_corner[0].x = (view_corner[0].x < temp_corner[0].x) ? view_corner[0].x : temp_corner[0].x;
    view_corner[0].y = (view_corner[0].y < temp_corner[0].y) ? view_corner[0].y : temp_corner[0].y;
    view_corner[0].z = (view_corner[0].z < temp_corner[0].z) ? view_corner[0].z : temp_corner[0].z;
    view_corner[1].x = (view_corner[1].x > temp_corner[1].x) ? view_corner[1].x : temp_corner[1].x;
    view_corner[1].y = (view_corner[1].y > temp_corner[1].y) ? view_corner[1].y : temp_corner[1].y;
    view_corner[1].z = (view_corner[1].z > temp_corner[1].z) ? view_corner[1].z : temp_corner[1].z;
    temp_volumes = temp_volumes->next;
  }
  view_corner[0] = realspace_alt_coord_to_base(view_corner[0], view_coord_frame);
  view_corner[1] = realspace_alt_coord_to_base(view_corner[1], view_coord_frame);

  return;
}


/* returns the minimum dimensional width of the list of volumes */
floatpoint_t volumes_min_voxel_size(volume_list_t * volumes) {

  floatpoint_t min_voxel_size, temp;

  g_assert(volumes!=NULL);

  if (volumes->next == NULL)
    min_voxel_size = REALPOINT_MIN_DIM(volumes->volume->voxel_size);
  else {
    min_voxel_size = volumes_min_voxel_size(volumes->next);
    temp = REALPOINT_MIN_DIM(volumes->volume->voxel_size);
    if (temp < min_voxel_size) min_voxel_size = temp;
  }

  return min_voxel_size;
}

/* returns the maximal dimensional size of a list of volumes */
floatpoint_t volumes_max_size(volume_list_t * volumes) {

  floatpoint_t temp, max_dim;

  if (volumes == NULL)
    return 0.0;
  else {
    max_dim = volumes_max_size(volumes->next);
    temp = REALPOINT_MAX_DIM(volumes->volume->voxel_size) * REALPOINT_MAX(volumes->volume->dim);
    if (temp > max_dim)
      max_dim = temp;
  }

  return max_dim;
}


/* returns the minimum dimensional width of the volumes with the largest voxel size */
floatpoint_t volumes_max_min_voxel_size(volume_list_t * volumes) {

  floatpoint_t min_voxel_size, temp;

  g_assert(volumes!=NULL);

  /* figure out what our voxel size is going to be for our returned
     slices.  I'm going to base this on the volume with the largest
     minimum voxel dimension, this way the user doesn't suddenly get a
     huge image when adding in a study with small voxels (i.e. a CT
     scan).  The user can always increase the zoom later*/
  /* figure out out thickness */
  if (volumes->next == NULL)
    min_voxel_size = REALPOINT_MIN_DIM(volumes->volume->voxel_size);
  else {
    min_voxel_size = volumes_min_voxel_size(volumes->next);
    temp = REALPOINT_MIN_DIM(volumes->volume->voxel_size);
    if (temp > min_voxel_size) min_voxel_size = temp;
  }

  return min_voxel_size;
}


/* returns the maximum dimension of the given volumes (in voxels) */
intpoint_t volumes_max_dim(volume_list_t * volumes) {

  intpoint_t max_dim, temp;

  g_assert(volumes!=NULL);

  max_dim = 0;
  while (volumes != NULL) {
    temp = ceil(REALPOINT_MAX_DIM(volumes->volume->dim));
    if (temp > max_dim) max_dim = temp;
    volumes = volumes->next;
  }

  return max_dim;
}


/* returns the length of the assembly of volumes wrt the given axis */
floatpoint_t volumes_get_width(volume_list_t * volumes, const realspace_t view_coord_frame) {

  volume_list_t * temp_volumes;
  realpoint_t view_corner[2];
  realpoint_t temp_corner[2];

  g_assert(volumes!=NULL);

  temp_volumes = volumes;
  volume_get_view_corners(temp_volumes->volume,view_coord_frame,view_corner);
  temp_volumes = volumes->next;
  while (temp_volumes != NULL) {
    volume_get_view_corners(temp_volumes->volume,view_coord_frame,temp_corner);
    view_corner[0].x = (view_corner[0].x < temp_corner[0].x) ? view_corner[0].x : temp_corner[0].x;
    view_corner[0].y = (view_corner[0].y < temp_corner[0].y) ? view_corner[0].y : temp_corner[0].y;
    view_corner[0].z = (view_corner[0].z < temp_corner[0].z) ? view_corner[0].z : temp_corner[0].z;
    view_corner[1].x = (view_corner[1].x > temp_corner[1].x) ? view_corner[1].x : temp_corner[1].x;
    view_corner[1].y = (view_corner[1].y > temp_corner[1].y) ? view_corner[1].y : temp_corner[1].y;
    view_corner[1].z = (view_corner[1].z > temp_corner[1].z) ? view_corner[1].z : temp_corner[1].z;
    temp_volumes = temp_volumes->next;
  }

  view_corner[0] = realspace_base_coord_to_alt(view_corner[0],view_coord_frame);
  view_corner[1] = realspace_base_coord_to_alt(view_corner[1],view_coord_frame);
  return fabs(view_corner[1].x-view_corner[0].x);
}

/* returns the length of the assembly of volumes wrt the given axis */
floatpoint_t volumes_get_height(volume_list_t * volumes, const realspace_t view_coord_frame) {

  volume_list_t * temp_volumes;
  realpoint_t view_corner[2];
  realpoint_t temp_corner[2];

  g_assert(volumes!=NULL);

  temp_volumes = volumes;
  volume_get_view_corners(temp_volumes->volume,view_coord_frame,view_corner);
  temp_volumes = volumes->next;
  while (temp_volumes != NULL) {
    volume_get_view_corners(temp_volumes->volume,view_coord_frame,temp_corner);
    view_corner[0].x = (view_corner[0].x < temp_corner[0].x) ? view_corner[0].x : temp_corner[0].x;
    view_corner[0].y = (view_corner[0].y < temp_corner[0].y) ? view_corner[0].y : temp_corner[0].y;
    view_corner[0].z = (view_corner[0].z < temp_corner[0].z) ? view_corner[0].z : temp_corner[0].z;
    view_corner[1].x = (view_corner[1].x > temp_corner[1].x) ? view_corner[1].x : temp_corner[1].x;
    view_corner[1].y = (view_corner[1].y > temp_corner[1].y) ? view_corner[1].y : temp_corner[1].y;
    view_corner[1].z = (view_corner[1].z > temp_corner[1].z) ? view_corner[1].z : temp_corner[1].z;
    temp_volumes = temp_volumes->next;
  }

  view_corner[0] = realspace_base_coord_to_alt(view_corner[0],view_coord_frame);
  view_corner[1] = realspace_base_coord_to_alt(view_corner[1],view_coord_frame);
  return fabs(view_corner[1].y-view_corner[0].y);
}

/* returns the length of the assembly of volumes wrt the given axis */
floatpoint_t volumes_get_length(volume_list_t * volumes,  const realspace_t view_coord_frame) {

  volume_list_t * temp_volumes;
  realpoint_t view_corner[2];
  realpoint_t temp_corner[2];

  g_assert(volumes!=NULL);

  temp_volumes = volumes;
  volume_get_view_corners(temp_volumes->volume,view_coord_frame,view_corner);
  temp_volumes = volumes->next;
  while (temp_volumes != NULL) {
    volume_get_view_corners(temp_volumes->volume,view_coord_frame,temp_corner);
    view_corner[0].x = (view_corner[0].x < temp_corner[0].x) ? view_corner[0].x : temp_corner[0].x;
    view_corner[0].y = (view_corner[0].y < temp_corner[0].y) ? view_corner[0].y : temp_corner[0].y;
    view_corner[0].z = (view_corner[0].z < temp_corner[0].z) ? view_corner[0].z : temp_corner[0].z;
    view_corner[1].x = (view_corner[1].x > temp_corner[1].x) ? view_corner[1].x : temp_corner[1].x;
    view_corner[1].y = (view_corner[1].y > temp_corner[1].y) ? view_corner[1].y : temp_corner[1].y;
    view_corner[1].z = (view_corner[1].z > temp_corner[1].z) ? view_corner[1].z : temp_corner[1].z;
    temp_volumes = temp_volumes->next;
  }

  view_corner[0] = realspace_base_coord_to_alt(view_corner[0],view_coord_frame);
  view_corner[1] = realspace_base_coord_to_alt(view_corner[1],view_coord_frame);
  return fabs(view_corner[1].z-view_corner[0].z);
}





/* generate a volume that contains an axis figure, this is used by the rendering ui */
volume_t * volume_get_axis_volume(guint x_width, guint y_width, guint z_width) {

  volume_t * axis_volume;
  realpoint_t center, radius;
  floatpoint_t height;
  realspace_t object_coord_frame;
  voxelpoint_t i;
  volume_data_t min, max, temp;


  axis_volume = volume_init();

  /* initialize what variables we want */
  volume_set_name(axis_volume, "rendering axis volume");
  axis_volume->dim.x  = x_width;
  axis_volume->dim.y = y_width;
  axis_volume->dim.z = z_width;
  axis_volume->conversion = 1.0;
  axis_volume->num_frames = 1;
  axis_volume->voxel_size.x = axis_volume->voxel_size.y = axis_volume->voxel_size.z = 1.0;
  axis_volume->corner.x = axis_volume->voxel_size.x * axis_volume->dim.x;
  axis_volume->corner.y = axis_volume->voxel_size.y * axis_volume->dim.y;
  axis_volume->corner.z = axis_volume->voxel_size.z * axis_volume->dim.z;

  
  if ((axis_volume->frame_duration = volume_get_frame_duration_mem(axis_volume)) == NULL) {
    g_warning("%s: couldn't allocate space for the axis volume frame duration array",PACKAGE);
    axis_volume = volume_free(axis_volume);
    return axis_volume;
  }
  axis_volume->frame_duration[0]=1.0;

  if ((axis_volume->data = volume_get_data_mem(axis_volume)) == NULL) {
    g_warning("%s: couldn't allocate space for the slice",PACKAGE);
    axis_volume = volume_free(axis_volume);
    return axis_volume;
  }

  /* initialize our volume */
  for (i.z = 0; i.z < axis_volume->dim.z; i.z++) 
    for (i.y = 0; i.y < axis_volume->dim.y; i.y++) 
      for (i.x = 0; i.x < axis_volume->dim.x; i.x++) 
	VOLUME_SET_CONTENT(axis_volume,0,i)=0.0;

  /* figure out an appropriate radius for our cylinders and spheres */
  radius.x = REALPOINT_MIN_DIM(axis_volume->corner)/24;
  radius.y = radius.x;
  radius.z = radius.x;

  /* start generating our axis */

 
  /* draw the z axis */
  center.z = (axis_volume->corner.z*4.5/8.0);
  height = (axis_volume->corner.z*3.0/8.0);
  center.x = axis_volume->corner.x/2;
  center.y = axis_volume->corner.y/2;
  object_coord_frame = realspace_get_view_coord_frame(axis_volume->coord_frame, TRANSVERSE);
  objects_place_elliptic_cylinder(axis_volume, 0, object_coord_frame, 
				  center, radius, height, AXIS_VOLUME_DENSITY);

                                                        
  /* draw the y axis */
  center.y = (axis_volume->corner.y*5.0/8.0);
  height = (axis_volume->corner.y*4.0/8.0);
  center.x = axis_volume->corner.x/2;
  center.z = axis_volume->corner.z/2;
  object_coord_frame = realspace_get_view_coord_frame(axis_volume->coord_frame, CORONAL);
  objects_place_elliptic_cylinder(axis_volume, 0, object_coord_frame, 
				  center, radius, height, AXIS_VOLUME_DENSITY);

 /* draw the x axis */
  center.x = (axis_volume->corner.x*5.0/8.0);
  height = (axis_volume->corner.x*4.0/8.0);
  center.y = axis_volume->corner.y/2;
  center.z = axis_volume->corner.z/2;
  object_coord_frame = realspace_get_view_coord_frame(axis_volume->coord_frame, SAGITTAL);
  objects_place_elliptic_cylinder(axis_volume, 0, object_coord_frame, 
				  center, radius, height, AXIS_VOLUME_DENSITY);

  radius.z = radius.y = radius.x = REALPOINT_MIN_DIM(axis_volume->corner)/12;
  center.x = (axis_volume->corner.x*7.0/8.0);
  objects_place_ellipsoid(axis_volume, 0, object_coord_frame, 
			  center, radius, AXIS_VOLUME_DENSITY);

  /* and figure out the max and min */
  max = 0.0;
  min = 0.0;
  for (i.z = 0; i.z < axis_volume->dim.z;i.z++)
    for (i.y = 0; i.y < axis_volume->dim.y; i.y++) 
      for (i.x = 0; i.x <axis_volume->dim.x; i.x++) {
	temp = VOLUME_CONTENTS(axis_volume, 0, i);
	if (temp > max)
	  max = temp;
	else if (temp < min)
	  min = temp;
      }

  axis_volume->max = max;
  axis_volume->min = min;
  axis_volume->threshold_max = max;
  axis_volume->threshold_min = min;

  return axis_volume;
}




/* returns a "2D" slice from a volume */
volume_t * volume_get_slice(const volume_t * volume,
			    const volume_time_t start,
			    const volume_time_t duration,
			    const realpoint_t  requested_voxel_size,
			    const realspace_t slice_coord_frame,
			    const realpoint_t far_corner,
			    const interpolation_t interpolation) {

  /* zp_start, where on the zp axis to start the slice, zp (z_prime) corresponds
     to the rotated axises, if negative, choose the midpoint */

  volume_t * slice;
  voxelpoint_t i,j;
  intpoint_t z;
  floatpoint_t volume_min_voxel_size, max_diff, voxel_length, z_steps;
  floatpoint_t real_value[8];
  realpoint_t alt,temp_p;
  realpoint_t half_size[NUM_AXIS], stride[NUM_AXIS], last[NUM_AXIS];
  axis_t i_axis;
  guint l;
  volume_data_t weight, max, min, temp, total_weight;
  guint start_frame, num_frames, i_frame;
  volume_time_t volume_start, volume_end;
  gchar * temp_string;
  realpoint_t box_rp[8];
  voxelpoint_t box_vp[8];
  volume_data_t box_value[8];
  realpoint_t slice_rp, volume_rp,diff, nearest_rp;
  voxelpoint_t volume_vp;
  volume_data_t weight1, weight2;

  /* sanity check */
  g_assert(volume != NULL);
  g_return_val_if_fail(volume->data != NULL, NULL);


  /* ----- figure out what frames of this volume to include ----*/

  /* figure out what frame to start at */
  start_frame = 0; 
  i_frame = 0;
  while (i_frame < volume->num_frames) {
    volume_start = volume_start_time(volume,i_frame);
    if (start > volume_start-CLOSE)
      start_frame = i_frame;
    i_frame++;
  }
  volume_start = volume_start_time(volume, start_frame);

  /* and figure out what frame to end at */
  num_frames = volume->num_frames + 1;
  i_frame = start_frame;
  while ((i_frame < volume->num_frames) && (num_frames > volume->num_frames)) {
    volume_end = volume_start_time(volume,i_frame)+volume->frame_duration[i_frame];
    if ((start+duration) < volume_end+CLOSE)
      num_frames = i_frame-start_frame+1;
    i_frame++;
  }
  if (num_frames+start_frame > volume->num_frames)
    num_frames = volume->num_frames-start_frame;
  volume_end = volume_start_time(volume,start_frame+num_frames-1) + 
    volume->frame_duration[start_frame+num_frames-1];

  /* ------------------------- */




  /* initialize the return slice */
  slice = volume_init();
  slice->num_frames = 1;
  slice->dim.x = ceil(fabs(far_corner.x)/requested_voxel_size.x);
  slice->dim.y = ceil(fabs(far_corner.y)/requested_voxel_size.y);
  slice->dim.z = 1;
  slice->voxel_size = requested_voxel_size;
  slice->coord_frame = slice_coord_frame;
  slice->corner.x = far_corner.x;
  slice->corner.y = far_corner.y;
  slice->corner.z = requested_voxel_size.z;
  slice->scan_start = volume_start;
  temp_string =  g_strdup_printf("slice from volume %s: size x %5.3f y %5.3f z %5.3f", 
				 volume->name, slice->corner.x, 
				 slice->corner.y, slice->corner.z);
  volume_set_name(slice,temp_string);
  volume_set_scan_date(slice, volume->scan_date);
  g_free(temp_string);

  if ((slice->frame_duration = volume_get_frame_duration_mem(slice)) == NULL) {
    g_warning("%s: couldn't allocate space for the slice frame duration array",PACKAGE);
    slice = volume_free(slice);
    return slice;
  }
  slice->frame_duration[0] = volume_end-volume_start;
  if ((slice->data = volume_get_data_mem(slice)) == NULL) {
    g_warning("%s: couldn't allocate space for the slice",PACKAGE);
    slice = volume_free(slice);
    return slice;
  }

  /* ----------------------- */



#ifdef AMIDE_DEBUG
  {
    realpoint_t real_corner[2];
    /* convert to real space */
    real_corner[0] = slice->coord_frame.offset;
    real_corner[1] = realspace_alt_coord_to_base(slice->corner,
						 slice->coord_frame);
    g_print("new slice from volume %s\t---------------------\n",volume->name);
    g_print("\tdim\t\tx %d\t\ty %d\t\tz %d\n",
	    slice->dim.x, slice->dim.y, slice->dim.z);
    g_print("\treal corner[0]\tx %5.4f\ty %5.4f\tz %5.4f\n",
	    real_corner[0].x,real_corner[0].y,real_corner[0].z);
    g_print("\treal corner[1]\tx %5.4f\ty %5.4f\tz %5.4f\n",
	    real_corner[1].x,real_corner[1].y,real_corner[1].z);
    g_print("\tvolume\t\tstart\t%5.4f\tend\t%5.3f\tframes %d to %d\n",
	    volume_start, volume_end,start_frame,start_frame+num_frames-1);
  }
#endif


  /* voxel_length is the length of a voxel given the coordinate frame of the slice.
     this is used to figure out how many iterations in the z direction we need to do */
  alt.x = alt.y = 0.0;
  alt.z = 1.0;
  alt = realspace_alt_dim_to_alt(alt, slice->coord_frame, volume->coord_frame);
  alt = rp_mult(alt, volume->voxel_size);
  voxel_length = REALPOINT_MAGNITUDE(alt);
  z_steps = slice->voxel_size.z/voxel_length;

#ifdef AMIDE_DEBUG
  //  g_print("\tz steps\t%5.3f\tvoxel length %5.3f (mm)\n",z_steps, voxel_length);
#endif

  /* figure out what stepping one voxel in a given direction in our slice
     cooresponds to in our volume */
  for (i_axis = 0; i_axis < NUM_AXIS; i_axis++) {
    alt.x = (i_axis == XAXIS) ? slice->voxel_size.x : 0.0;
    alt.y = (i_axis == YAXIS) ? slice->voxel_size.y : 0.0;
    alt.z = (i_axis == ZAXIS) ? voxel_length : 0.0;
    alt = realspace_alt_coord_to_base(alt, slice->coord_frame);
    REALPOINT_SUB(alt, slice->coord_frame.offset, alt);
    REALPOINT_ADD(alt, volume->coord_frame.offset, alt);
    stride[i_axis] = realspace_base_coord_to_alt(alt, volume->coord_frame);
  }

  /* figure out what point in the volume we're going to start at */
  alt.x = slice->voxel_size.x/2.0;
  alt.y = slice->voxel_size.y/2.0;
  alt.z = voxel_length/2.0;
  volume_rp = realspace_alt_coord_to_alt(alt, slice->coord_frame, volume->coord_frame);

  /* get some important dimensional information */
  volume_min_voxel_size = REALPOINT_MIN_DIM(volume->voxel_size);

  /* figure out how to step half a volume voxel's size in a given direction */
  for (i_axis = 0 ; i_axis < NUM_AXIS ; i_axis++) {
    alt.x = (i_axis == XAXIS) ? (1.0-CLOSE)*(volume_min_voxel_size/2.0) : 0.0;
    alt.y = (i_axis == YAXIS) ? (1.0-CLOSE)*(volume_min_voxel_size/2.0) : 0.0;
    alt.z = (i_axis == ZAXIS) ? (1.0-CLOSE)*(volume_min_voxel_size/2.0) : 0.0;
    half_size[i_axis] = realspace_alt_dim_to_alt(alt, slice->coord_frame, volume->coord_frame);
  }


  /* initialize our volume */
  i.z = 0;
  for (i.y = 0; i.y < slice->dim.y; i.y++) 
    for (i.x = 0; i.x < slice->dim.x; i.x++) 
      VOLUME_SET_CONTENT(slice,0,i)=0.0;


  switch(interpolation) {

  case TWO_BY_TWO:
    /* iterate over the number of frames we'll be incorporating into this slice */
    for (i_frame = start_frame; i_frame < start_frame+num_frames;i_frame++)
      
      /* iterate over the number of planes we'll be compressing into this slice */
      for (z = 0; z < ceil(z_steps-SMALL)-SMALL; z++) { 
	last[ZAXIS] = volume_rp;
	weight = ( floor(z_steps) > z) ? 1.0 : z_steps-floor(z_steps);
	weight = weight/4.0;
	weight /= num_frames * z_steps;
	
	/* iterate over the y dimension */
	for (i.y = 0; i.y < slice->dim.y; i.y++) {
	  last[YAXIS] = volume_rp;
	  /* and iteratate over x */
	  for (i.x = 0; i.x < slice->dim.x; i.x++) {
	    
	    /* get the locations and values of the four voxels in the real volume
	       which are closest to our slice's voxel */
	    total_weight = 0;
	    
	    /* find the closest voxels in volume space and their values*/
	    for (l=0; l<4; l++) {
	      
	      if (l & 0x1) REALPOINT_SUB(volume_rp, half_size[XAXIS],temp_p);
	      else REALPOINT_ADD(volume_rp, half_size[XAXIS],temp_p);
	      
	      if (l & 0x2) REALPOINT_SUB(temp_p, half_size[YAXIS], temp_p);
	      else REALPOINT_ADD(temp_p, half_size[YAXIS],temp_p);

	      j = volume_realpoint_to_voxel(volume, temp_p);
	      
	      /* get the value of the closest voxel in real space */
	      if (!volume_includes_voxel(volume,j)) real_value[l] = EMPTY;
	      else real_value[l] = VOLUME_CONTENTS(volume,i_frame,j);
	      
	      VOLUME_SET_CONTENT(slice,0,i)+=weight*real_value[l];
	    }
	    REALPOINT_ADD(volume_rp, stride[XAXIS], volume_rp); 
	  }
	  REALPOINT_ADD(last[YAXIS], stride[YAXIS], volume_rp);
	}
	REALPOINT_ADD(last[ZAXIS], stride[ZAXIS], volume_rp); 
      }
    break;

  case TWO_BY_TWO_BY_TWO:
    /* iterate over the number of frames we'll be incorporating into this slice */
    for (i_frame = start_frame; i_frame < start_frame+num_frames;i_frame++)
      
      /* iterate over the number of planes we'll be compressing into this slice */
      for (z = 0; z < ceil(z_steps-SMALL)-SMALL; z++) { 
	last[ZAXIS] = volume_rp;
	weight = ( floor(z_steps) > z) ? 1.0 : z_steps-floor(z_steps);
	weight = weight/8.0;
	weight /= num_frames * z_steps;
	
	/* iterate over the y dimension */
	for (i.y = 0; i.y < slice->dim.y; i.y++) {
	  last[YAXIS] = volume_rp;
	  /* and iteratate over x */
	  for (i.x = 0; i.x < slice->dim.x; i.x++) {
	    
	    /* get the locations and values of the four voxels in the real volume
	       which are closest to our slice's voxel */
	    total_weight = 0;
	    
	    /* find the closest voxels in volume space and their values*/
	    for (l=0; l<8; l++) {
	      
	      if (l & 0x1) REALPOINT_SUB(volume_rp, half_size[XAXIS],temp_p);
	      else REALPOINT_ADD(volume_rp, half_size[XAXIS],temp_p);
	      
	      if (l & 0x2) REALPOINT_SUB(temp_p, half_size[YAXIS], temp_p);
	      else REALPOINT_ADD(temp_p, half_size[YAXIS],temp_p);
	      
	      if (l & 0x4) REALPOINT_SUB(temp_p, half_size[ZAXIS],temp_p);
	      else REALPOINT_ADD(temp_p, half_size[ZAXIS],temp_p);
	      
	      j = volume_realpoint_to_voxel(volume, temp_p);
	      
	      /* get the value of the closest voxel in real space */
	      if (!volume_includes_voxel(volume,j)) real_value[l] = EMPTY;
	      else real_value[l] = VOLUME_CONTENTS(volume,i_frame,j);
	      
	      VOLUME_SET_CONTENT(slice,0,i)+=weight*real_value[l];
	    }
	    REALPOINT_ADD(volume_rp, stride[XAXIS], volume_rp); 
	  }
	  REALPOINT_ADD(last[YAXIS], stride[YAXIS], volume_rp);
	}
	REALPOINT_ADD(last[ZAXIS], stride[ZAXIS], volume_rp); 
      }
    break;
  case TRILINEAR:
    /* iterate over the frames we'll be incorporating into this slice */
    for (i_frame = start_frame; i_frame < start_frame+num_frames;i_frame++) {

      /* iterate over the number of planes we'll be compressing into this slice */
      for (z = 0; z < ceil(z_steps-SMALL)-SMALL; z++) {
	
	/* the slices z_coordinate for this iteration's slice voxel */
	slice_rp.z = z*voxel_length+voxel_length/2.0;

	/* weight is between 0 and 1, this is used to weight the last voxel 
	   in the slice's z direction, and in averaging frames */
	if (floor(z_steps) > z)
	  weight = 1.0/(num_frames * z_steps);
	else
	  weight = (z_steps-floor(z_steps)) / (num_frames * z_steps);

	/* iterate over the y dimension */
	for (i.y = 0; i.y < slice->dim.y; i.y++) {

	  /* the slice y_coordinate of the center of this iteration's slice voxel */
	  slice_rp.y = (((floatpoint_t) i.y)*slice->corner.y/slice->dim.y) + slice->voxel_size.y/2.0;
	  
	  /* the slice x coord of the center of the first slice voxel in this loop */
	  slice_rp.x = slice->voxel_size.x/2.0;

	  /* iterate over the x dimension */
	  for (i.x = 0; i.x < slice->dim.x; i.x++) {

	    /* translate the current point in slice space into the volume's coordinate frame */
	    volume_rp = realspace_alt_coord_to_alt(slice_rp, 
						   slice->coord_frame, 
						   volume->coord_frame);

	    /* get the nearest neighbor in the volume to this slice voxel */
	    volume_vp = volume_realpoint_to_voxel(volume, volume_rp);
	    nearest_rp = volume_voxel_to_realpoint(volume, volume_vp);

	    /* figure out which way to go to get the nearest voxels to our slice voxel*/
	    REALPOINT_SUB(volume_rp, nearest_rp, diff);

	    /* figure out which voxels to look at */
	    for (l=0; l<8; l=l+1) {
	      if (diff.x < 0)
		box_vp[l].x = (l & 0x1) ? volume_vp.x-1 : volume_vp.x;
	      else /* diff.x >= 0 */
		box_vp[l].x = (l & 0x1) ? volume_vp.x : volume_vp.x+1;
	      if (diff.y < 0)
		box_vp[l].y = (l & 0x2) ? volume_vp.y-1 : volume_vp.y;
	      else /* diff.y >= 0 */
		box_vp[l].y = (l & 0x2) ? volume_vp.y : volume_vp.y+1;
	      if (diff.z < 0)
		box_vp[l].z = (l & 0x4) ? volume_vp.z-1 : volume_vp.z;
	      else /* diff.z >= 0 */
		box_vp[l].z = (l & 0x4) ? volume_vp.z : volume_vp.z+1;

	      box_rp[l] = volume_voxel_to_realpoint(volume, box_vp[l]);

	      /* get the value of the point on the box */
	      if (volume_includes_voxel(volume, box_vp[l]))
		box_value[l] = VOLUME_CONTENTS(volume, i_frame, box_vp[l]);
	      else
		box_value[l] = EMPTY;
	    }
	    
	    /* do the x direction linear interpolation of the sets of two points */
	    for (l=0;l<8;l=l+2) {
	      max_diff = box_rp[l+1].x-box_rp[l].x;
	      weight1 = ((max_diff - (volume_rp.x - box_rp[l].x))/max_diff);
	      weight2 = ((max_diff - (box_rp[l+1].x - volume_rp.x))/max_diff);
	      box_value[l] = (box_value[l] * weight1) + (box_value[l+1] * weight2);
	    }

	    /* do the y direction linear interpolation of the sets of two points */
	    for (l=0;l<8;l=l+4) {
	      max_diff = box_rp[l+2].y-box_rp[l].y;
	      weight1 = ((max_diff - (volume_rp.y - box_rp[l].y))/max_diff);
	      weight2 = ((max_diff - (box_rp[l+2].y - volume_rp.y))/max_diff);
	      box_value[l] = (box_value[l] * weight1) + (box_value[l+2] * weight2);
	    }

	    /* do the z direction linear interpolation of the sets of two points */
	    for (l=0;l<8;l=l+8) {
	      max_diff = box_rp[l+4].z-box_rp[l].z;
	      weight1 = ((max_diff - (volume_rp.z - box_rp[l].z))/max_diff);
	      weight2 = ((max_diff - (box_rp[l+4].z - volume_rp.z))/max_diff);
	      box_value[l] = (box_value[l] * weight1) + (box_value[l+4] * weight2);
	    }

	    VOLUME_SET_CONTENT(slice,0,i)+=weight*box_value[0];

	    slice_rp.x += slice->voxel_size.x; 
	  }
	}
      }
    }
    break;
  case NEAREST_NEIGHBOR:
  default:  

    /* iterate over the number of frames we'll be incorporating into this slice */
    for (i_frame = start_frame; i_frame < start_frame+num_frames;i_frame++) {

      /* iterate over the number of planes we'll be compressing into this slice */
      for (z = 0; z < ceil(z_steps-SMALL)-SMALL; z++) { 
	last[ZAXIS] = volume_rp;

	/* weight is between 0 and 1, this is used to weight the last voxel 
	   in the slice's z direction, and in averaging frames */
	if (floor(z_steps) > z)
	  weight = 1.0/(num_frames * z_steps);
	else
	  weight = (z_steps-floor(z_steps)) / (num_frames * z_steps);
	
	/* iterate over the y dimension */
	for (i.y = 0; i.y < slice->dim.y; i.y++) {
	  last[YAXIS] = volume_rp;

	  /* and iteratate over x */
	  for (i.x = 0; i.x < slice->dim.x; i.x++) {
	    j = volume_realpoint_to_voxel(volume, volume_rp);
	    if (!volume_includes_voxel(volume,j))
	      VOLUME_SET_CONTENT(slice,0,i) += weight*EMPTY;
	    else
	      VOLUME_SET_CONTENT(slice,0,i) += weight*VOLUME_CONTENTS(volume,i_frame,j);
	    REALPOINT_ADD(volume_rp, stride[XAXIS], volume_rp); 
	  }
	  REALPOINT_ADD(last[YAXIS], stride[YAXIS], volume_rp);
	}
	REALPOINT_ADD(last[ZAXIS], stride[ZAXIS], volume_rp); 
      }
    }
    break;
  }

  /* figure out the max and min */
  max = 0.0;
  min = 0.0;
  for (i.y = 0; i.y < slice->dim.y; i.y++) 
    for (i.x = 0; i.x < slice->dim.x; i.x++) {
      temp = VOLUME_CONTENTS(slice, 0, i);
      if (temp > max)
	max = temp;
      else if (temp < min)
	min = temp;
    }
  slice->max = max;
  slice->min = min;

  return slice;
}



/* give a list ov volumes, returns a list of slices of equal size and orientation
   intersecting these volumes */
volume_list_t * volumes_get_slices(volume_list_t * volumes,
				   const volume_time_t start,
				   const volume_time_t duration,
				   const floatpoint_t thickness,
				   const realspace_t view_coord_frame,
				   const floatpoint_t zoom,
				   const interpolation_t interpolation) {


  axis_t i_axis;
  realpoint_t voxel_size;
  realspace_t slice_coord_frame;
  realpoint_t view_corner[2];
  realpoint_t slice_corner;
  volume_list_t * slices=NULL;
  volume_t * temp_slice;
  floatpoint_t min_voxel_size;
#ifdef AMIDE_DEBUG
  //  struct timeb timing1, timing2;
  struct timeval tv1;
  struct timeval tv2;
  gdouble time1;
  gdouble time2;
  gdouble total_time;
#endif

  /* thickness (of slice) in mm, negative indicates set this value
     according to voxel size */

#ifdef AMIDE_DEBUG
  /* let's do some timing */
  gettimeofday(&tv1, NULL);
#endif

  /* sanity check */
  g_assert(volumes != NULL);

  /* get the minimum voxel dimension of this list of volumes */
  min_voxel_size = volumes_max_min_voxel_size(volumes);
  g_return_val_if_fail(min_voxel_size > 0.0, NULL);

  /* compensate for zoom */
  min_voxel_size = (1/zoom) * min_voxel_size;
  voxel_size.x = voxel_size.y = min_voxel_size;
  voxel_size.z = (thickness > 0) ? thickness : min_voxel_size;

  /* figure out all encompasing corners for the slices based on our viewing axis */
  volumes_get_view_corners(volumes, view_coord_frame, view_corner);

  /* adjust the z' dimension of the view's corners */
  view_corner[0] = realspace_base_coord_to_alt(view_corner[0],view_coord_frame);
  view_corner[0].z=0;
  view_corner[0] = realspace_alt_coord_to_base(view_corner[0],view_coord_frame);

  view_corner[1] = realspace_base_coord_to_alt(view_corner[1],view_coord_frame);
  view_corner[1].z=voxel_size.z;
  view_corner[1] = realspace_alt_coord_to_base(view_corner[1],view_coord_frame);

  /* figure out the coordinate frame for the slices based on the corners returned */
  for(i_axis=0;i_axis<NUM_AXIS;i_axis++) 
    slice_coord_frame.axis[i_axis] = view_coord_frame.axis[i_axis];
  slice_coord_frame.offset = view_corner[0];
  slice_corner = realspace_base_coord_to_alt(view_corner[1],slice_coord_frame);

  /* and get the slices */
  while (volumes != NULL) {
    temp_slice = volume_get_slice(volumes->volume, start, duration, voxel_size, 
				  slice_coord_frame, slice_corner, interpolation);
    slices = volume_list_add_volume(slices,temp_slice);
    temp_slice = volume_free(temp_slice);
    volumes = volumes->next;
  }

#ifdef AMIDE_DEBUG
  /* and wrapup our timing */
  gettimeofday(&tv2, NULL);
  time1 = ((double) tv1.tv_sec) + ((double) tv1.tv_usec)/1000000.0;
  time2 = ((double) tv2.tv_sec) + ((double) tv2.tv_usec)/1000000.0;
  total_time = time2-time1;
  g_print("######## Slice Generating Took %5.3f (s) #########\n",total_time);

#endif

  return slices;
}

















