/* volume.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2000-2002 Andy Loening
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
#include <gtk/gtk.h>
#include <math.h>
#include <time.h>
#ifdef AMIDE_DEBUG
#include <sys/timeb.h>
#endif
#include "raw_data.h"
#include "raw_data_import.h"
#include "idl_data_import.h"
#include "pem_data_import.h"
#include "cti_import.h"
#include "medcon_import.h"


/* external variables */
gchar * interpolation_names[] = {"Nearest Neighbhor", 
				 "Trilinear"};

gchar * interpolation_explanations[] = {
  "interpolate using nearest neighbhor (fast)", 
  "interpolate using trilinear interpolation (slow)"
};

gchar * modality_names[] = {"PET", \
			    "SPECT", \
			    "CT", \
			    "MRI", \
			    "Other"};

gchar * import_menu_names[] = {
  "",
  "_Raw Data", 
  "_PEM Technologies", 
  "_IDL Output",
#ifdef AMIDE_LIBECAT_SUPPORT
  "_CTI 6.4/7.0 via libecat",
#endif
#ifdef AMIDE_LIBMDC_SUPPORT
  "(_X)medcon importing"
#endif
};
  
gchar * import_menu_explanations[] = {
  "",
  "Import file as raw data",
  "Import a PEM Technologies File (raw ASCII)",
  "Import an IDL created file, generally from the microCT's volume rendering tool",
  "Import a CTI 6.4 or 7.0 file using the libecat library",
  "Import via the (X)medcon importing library (libmdc)",
};


/* removes a reference to a volume, frees up the volume if no more references */
volume_t * volume_unref(volume_t * volume) {

  if (volume == NULL) return volume;

  /* sanity checks */
  g_return_val_if_fail(volume->ref_count > 0, NULL);
  
  volume->ref_count--;

  /* if we've removed all reference's, free remaining data structures */
  if (volume->ref_count == 0) {
#ifdef AMIDE_DEBUG
    //    g_print("freeing volume: %s\n",volume->name);
    //    g_print("freeing volume: %s\n\tdata:\t",volume->name);
#endif
    volume->data_set = data_set_unref(volume->data_set);
#ifdef AMIDE_DEBUG
    //    if (volume->internal_scaling->ref_count == 1)
    //      g_print("\tinternal scaling:\t");
#endif
    volume->internal_scaling = data_set_unref(volume->internal_scaling);
#ifdef AMIDE_DEBUG
    //    if (volume->current_scaling->ref_count == 1)
    //      g_print("\tcurrent scaling:\t");
#endif
    volume->current_scaling = data_set_unref(volume->current_scaling);
#ifdef AMIDE_DEBUG
    //    if (volume->distribution != NULL)
    //      if (volume->distribution->ref_count == 1)
    //	g_print("\tdistribution:\t");
#endif
    volume->coord_frame = rs_unref(volume->coord_frame);
    volume->distribution = data_set_unref(volume->distribution);
    g_free(volume->frame_duration);
    g_free(volume->frame_max);
    g_free(volume->frame_min);
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
  time_t current_time;

  if ((temp_volume = (volume_t *) g_malloc(sizeof(volume_t))) == NULL) {
    g_warning("%s: Couldn't allocate memory for the volume structure",PACKAGE);
    return temp_volume;
  }
  temp_volume->ref_count = 1;

  /* put in some sensable values */
  temp_volume->name = NULL;
  temp_volume->scan_date = NULL;
  temp_volume->data_set = NULL;
  temp_volume->current_scaling = NULL;
  temp_volume->frame_duration = NULL;
  temp_volume->frame_max = NULL;
  temp_volume->frame_min = NULL;
  temp_volume->global_max = 0.0;
  temp_volume->global_min = 0.0;
  temp_volume->threshold_type = THRESHOLD_GLOBAL;
  temp_volume->threshold_ref_frame[0]=0;
  temp_volume->threshold_ref_frame[1]=0;
  temp_volume->distribution = NULL;
  temp_volume->modality = PET;
  temp_volume->voxel_size = zero_rp;
  if ((temp_volume->internal_scaling = data_set_FLOAT_0D_SCALING_init(1.0)) == NULL) {
    g_warning("%s: Couldn't allocate memory for internal scaling structure", PACKAGE);
    return volume_unref(temp_volume);
  }
  volume_set_scaling(temp_volume, 1.0);
  temp_volume->scan_start = 0.0;
  temp_volume->color_table = BW_LINEAR;
  temp_volume->coord_frame = NULL;
  temp_volume->corner = zero_rp;
  temp_volume->align_pts = NULL;

  /* set the scan date to the current time, good for an initial guess */
  time(&current_time);
  volume_set_scan_date(temp_volume, ctime(&current_time));
  g_strdelimit(temp_volume->scan_date, "\n", ' '); /* turns newlines to white space */
  g_strstrip(temp_volume->scan_date); /* removes trailing and leading white space */

  return temp_volume;
}


/* function to write out the information content of a volume into an xml
   file.  Returns a string containing the name of the file. */
gchar * volume_write_xml(volume_t * volume, gchar * study_directory) {

  gchar * volume_xml_filename;
  guint count;
  struct stat file_info;
  xmlDocPtr doc;
  gchar * data_set_xml_filename;
  gchar * data_set_name;
  gchar * internal_scaling_xml_filename;
  gchar * internal_scaling_name;
  xmlNodePtr pts_nodes;

  /* make a guess as to our filename */
  count = 1;
  volume_xml_filename = g_strdup_printf("Volume_%s.xml", volume->name);

  /* see if this file already exists */
  while (stat(volume_xml_filename, &file_info) == 0) {
    g_free(volume_xml_filename);
    count++;
    volume_xml_filename = g_strdup_printf("Volume_%s_%d.xml", volume->name, count);
  }

  /* and we now have unique filenames */
#ifdef AMIDE_DEBUG
  g_print("\t- saving volume %s in file %s\n",volume->name, volume_xml_filename);
#endif

  /* write the volume xml file */
  doc = xmlNewDoc("1.0");
  doc->children = xmlNewDocNode(doc, NULL, "Volume", volume->name);
  xml_save_string(doc->children, "scan_date", volume->scan_date);
  xml_save_string(doc->children, "modality", modality_names[volume->modality]);
  xml_save_data(doc->children, "external_scaling", volume->external_scaling);
  xml_save_realpoint(doc->children, "voxel_size", volume->voxel_size);
  xml_save_time(doc->children, "scan_start", volume->scan_start);
  xml_save_times(doc->children, "frame_duration", volume->frame_duration, volume->data_set->dim.t);
  xml_save_string(doc->children, "threshold_type", threshold_type_names[volume->threshold_type]);
  xml_save_string(doc->children, "color_table", color_table_names[volume->color_table]);
  xml_save_data(doc->children, "threshold_max", volume->threshold_max[0]);
  xml_save_data(doc->children, "threshold_min", volume->threshold_min[0]);
  xml_save_data(doc->children, "threshold_max_1", volume->threshold_max[1]);
  xml_save_data(doc->children, "threshold_min_1", volume->threshold_min[1]);
  xml_save_int(doc->children, "threshold_ref_frame_0", volume->threshold_ref_frame[0]);
  xml_save_int(doc->children, "threshold_ref_frame_1", volume->threshold_ref_frame[1]);
  xml_save_realspace(doc->children, "coord_frame", volume->coord_frame);

  data_set_name = g_strdup_printf("Data_set_%s",volume->name);
  data_set_xml_filename = data_set_write_xml(volume->data_set, study_directory, data_set_name);
  g_free(data_set_name);
  xml_save_string(doc->children, "data_set_file", data_set_xml_filename);
  g_free(data_set_xml_filename);

  internal_scaling_name = g_strdup_printf("Scaling_factors_%s",volume->name);
  internal_scaling_xml_filename = data_set_write_xml(volume->internal_scaling, study_directory, internal_scaling_name);
  g_free(internal_scaling_name);
  xml_save_string(doc->children, "internal_scaling_file", internal_scaling_xml_filename);
  g_free(internal_scaling_xml_filename);

  /* save our alignment points */
  pts_nodes = xmlNewChild(doc->children, NULL, "Alignment_points", NULL);
  align_pts_write_xml(volume->align_pts, pts_nodes, study_directory, volume->name);

  /* and save */
  xmlSaveFile(volume_xml_filename, doc);

  /* and we're done with the xml stuff*/
  xmlFreeDoc(doc);

  return volume_xml_filename;
}


/* function to load in a volume xml file */
volume_t * volume_load_xml(gchar * volume_xml_filename, const gchar * study_directory) {

  xmlDocPtr doc;
  volume_t * new_volume;
  xmlNodePtr nodes;
  xmlNodePtr pts_nodes;
  modality_t i_modality;
  color_table_t i_color_table;
  threshold_t i_threshold_type;
  gchar * temp_string;
  gchar * scan_date;
  time_t modification_time;
  struct stat file_info;
  gchar * data_set_xml_filename;
  gchar * internal_scaling_xml_filename;

  new_volume = volume_init();

  /* parse the xml file */
  if ((doc = xmlParseFile(volume_xml_filename)) == NULL) {
    g_warning("%s: Couldn't Parse AMIDE volume xml file %s/%s",PACKAGE, study_directory,volume_xml_filename);
    return volume_unref(new_volume);
  }

  /* get the root of our document */
  if ((nodes = xmlDocGetRootElement(doc)) == NULL) {
    g_warning("%s: AMIDE volume xml file doesn't appear to have a root: %s/%s",
	      PACKAGE, study_directory,volume_xml_filename);
    return volume_unref(new_volume);
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
    stat(volume_xml_filename, &file_info);
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





  /* load in our data set */
  data_set_xml_filename = xml_get_string(nodes, "data_set_file");
  internal_scaling_xml_filename = xml_get_string(nodes, "internal_scaling_file");
  if ((data_set_xml_filename != NULL) && (internal_scaling_xml_filename != NULL)) {
    new_volume->data_set = data_set_load_xml(data_set_xml_filename,study_directory);
    new_volume->internal_scaling = data_set_load_xml(internal_scaling_xml_filename,study_directory);

    /* parameters that aren't in older versions and default values aren't good enough*/
    volume_set_scaling(new_volume, xml_get_data(nodes, "external_scaling"));

  } else {
    /* ---- legacy cruft ----- */

    gchar * raw_data_filename;
    raw_data_format_t i_raw_data_format, raw_data_format;
    voxelpoint_t temp_dim;

    g_warning("%s: no data_set file, will continue with the assumption of a .xif format previous to 1.4", 
	      PACKAGE);

    /* get the name of our associated data file */
    raw_data_filename = xml_get_string(nodes, "raw_data");

    /* and figure out the data format */
    temp_string = xml_get_string(nodes, "data_format");
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

    temp_dim = xml_get_voxelpoint3D(nodes, "dim");
    temp_dim.t = xml_get_int(nodes, "num_frames");
    volume_set_scaling(new_volume,  xml_get_data(nodes, "conversion"));
    
    /* now load in the raw data */
#ifdef AMIDE_DEBUG
    g_print("reading volume %s from file %s\n", new_volume->name, raw_data_filename);
#endif
    new_volume->data_set = raw_data_read_file(raw_data_filename, raw_data_format, temp_dim, 0);
    
    g_free(raw_data_filename);
    /* -------- end legacy cruft -------- */
  }

  /* load in the alignment points */
  pts_nodes = xml_get_node(nodes, "Alignment_points");
  if (pts_nodes != NULL) {
    pts_nodes = pts_nodes->children;
    if (pts_nodes != NULL)
      new_volume->align_pts = align_pts_load_xml(pts_nodes, study_directory);
  }

  /* and figure out the rest of the parameters */
  new_volume->voxel_size = xml_get_realpoint(nodes, "voxel_size");
  new_volume->scan_start = xml_get_time(nodes, "scan_start");
  new_volume->frame_duration = xml_get_times(nodes, "frame_duration", new_volume->data_set->dim.t);
  new_volume->threshold_max[0] =  xml_get_data(nodes, "threshold_max");
  new_volume->threshold_min[0] =  xml_get_data(nodes, "threshold_min");
  new_volume->threshold_max[1] =  xml_get_data(nodes, "threshold_max_1");
  new_volume->threshold_min[1] =  xml_get_data(nodes, "threshold_min_1");
  new_volume->threshold_ref_frame[0] = xml_get_int(nodes,"threshold_ref_frame_0");
  new_volume->threshold_ref_frame[1] = xml_get_int(nodes,"threshold_ref_frame_1");
  new_volume->coord_frame = xml_get_realspace(nodes, "coord_frame");

  /* figure out the thresholding */
  temp_string = xml_get_string(nodes, "threshold_type");
  if (temp_string != NULL)
    for (i_threshold_type=0; i_threshold_type < NUM_THRESHOLD_TYPES; i_threshold_type++) 
      if (g_strcasecmp(temp_string, threshold_type_names[i_threshold_type]) == 0)
	new_volume->threshold_type = i_threshold_type;
  g_free(temp_string);


  /* recalc the temporary parameters */
  volume_recalc_far_corner(new_volume);
  volume_calc_frame_max_min(new_volume);
  volume_calc_global_max_min(new_volume);

  /* and we're done */
  xmlFreeDoc(doc);
  
  return new_volume;
}


/* function to import a file into a volume
   note: model_name is a secondary file needed by PEM, usually going to be set to NULL */
volume_t * volume_import_file(import_method_t import_method, int submethod,
			      const gchar * import_filename, gchar * model_filename) {
  volume_t * import_volume;
  gchar * import_filename_base;
  gchar * import_filename_extension;
  gchar ** frags=NULL;

  /* sanity checks */
  if (import_filename == NULL)
    return NULL;

  /* if we're guessing how to import.... */
  if (import_method == AMIDE_GUESS) {

    /* extract the extension of the file */
    import_filename_base = g_basename(import_filename);
    g_strreverse(import_filename_base);
    frags = g_strsplit(import_filename_base, ".", 2);
    import_filename_extension = frags[0];
    g_strreverse(import_filename_base);
    g_strreverse(import_filename_extension);

    
    if ((g_strcasecmp(import_filename_extension, "dat")==0) ||
	(g_strcasecmp(import_filename_extension, "raw")==0))  
      /* .dat and .raw are assumed to be raw data */
      import_method = RAW_DATA;
    else if (g_strcasecmp(import_filename_extension, "idl")==0)
      /* if it appears to be some sort of an idl file */
      import_method = IDL_DATA;
#ifdef AMIDE_LIBECAT_SUPPORT      
    else if ((g_strcasecmp(import_filename_extension, "img")==0) ||
	     (g_strcasecmp(import_filename_extension, "v")==0) ||
	     (g_strcasecmp(import_filename_extension, "atn")==0) ||
	     (g_strcasecmp(import_filename_extension, "scn")==0))
      /* if it appears to be a cti file */
      import_method = LIBECAT_DATA;
#endif
    else /* fallback methods */
#ifdef AMIDE_LIBMDC_SUPPORT
      /* try passing it to the libmdc library.... */
      import_method = LIBMDC_DATA;
#else
    { /* unrecognized file type */
      g_warning("%s: Guessing raw data, as extension %s not recognized on file:\n\t%s", 
		PACKAGE, import_filename_extension, import_filename);
      import_method = RAW_DATA;
    }
#endif
  }    

  switch (import_method) {

  case IDL_DATA:
    if ((import_volume=idl_data_import(import_filename)) == NULL)
      g_warning("%s: Could not interpret as an IDL file:\n\t%s",PACKAGE, import_filename);
    break;
    
  case PEM_DATA:
    if ((import_volume= pem_data_import(import_filename, model_filename)) == NULL)
      g_warning("%s: Could not interpret as a PEM technologies model file:\n\t%s",PACKAGE,model_filename);
    break;
    
#ifdef AMIDE_LIBECAT_SUPPORT      
  case LIBECAT_DATA:
    if ((import_volume=cti_import(import_filename)) == NULL) 
      g_warning("%s: Could not interpret as a CTI file using libecat:\n\t%s",PACKAGE, import_filename);
    break;
#endif
#ifdef AMIDE_LIBMDC_SUPPORT
  case LIBMDC_DATA:
    if ((import_volume=medcon_import(import_filename, submethod)) == NULL) 
      g_warning("%s: Could not interpret using (X)medcon/libmdc file:\n\t%s",PACKAGE, import_filename);
    break;
#endif
  case RAW_DATA:
  default:
    if ((import_volume=raw_data_import(import_filename)) == NULL)
      g_warning("%s: Could not interpret as a raw data file:\n\t%s", PACKAGE, import_filename);
    break;
  }

  if (import_volume != NULL) {
    /* set the thresholds */
    import_volume->threshold_max[0] = import_volume->threshold_max[1] = import_volume->global_max;
    import_volume->threshold_min[0] = import_volume->threshold_min[1] =
      (import_volume->global_min > 0.0) ? import_volume->global_min : 0.0;
    import_volume->threshold_ref_frame[1] = import_volume->data_set->dim.t-1;
  }

  /* freeup our strings, note, the other two are just pointers into these strings */
  g_strfreev(frags);

  return import_volume;
}


/* makes a new volume item which is a copy of a previous volume's information. 
   Notes: 
   - does not make a copy of the source volume's data, just adds a reference
   - does not make a copy of the distribution data, just adds a reference */
volume_t * volume_copy(volume_t * src_volume) {

  volume_t * dest_volume;
  guint i;

  /* sanity checks */
  g_return_val_if_fail(src_volume != NULL, NULL);

  dest_volume = volume_init();

  /* copy the data elements */
  dest_volume->data_set = data_set_ref(src_volume->data_set);
  dest_volume->distribution = data_set_ref(src_volume->distribution);
  dest_volume->internal_scaling = data_set_ref(src_volume->internal_scaling);
  dest_volume->current_scaling = NULL;
  dest_volume->modality = src_volume->modality;
  dest_volume->voxel_size = src_volume->voxel_size;
  volume_set_scaling(dest_volume, src_volume->external_scaling);
  dest_volume->global_max = src_volume->global_max;
  dest_volume->global_min = src_volume->global_min;
  dest_volume->color_table= src_volume->color_table;
  dest_volume->threshold_type = src_volume->threshold_type;
  dest_volume->threshold_max[0] = src_volume->threshold_max[0];
  dest_volume->threshold_max[1] = src_volume->threshold_max[1];
  dest_volume->threshold_min[0] = src_volume->threshold_min[0];
  dest_volume->threshold_min[1] = src_volume->threshold_min[1];
  dest_volume->threshold_ref_frame[0] = src_volume->threshold_ref_frame[0];
  dest_volume->threshold_ref_frame[1] = src_volume->threshold_ref_frame[1];
  dest_volume->coord_frame = rs_copy(src_volume->coord_frame);
  dest_volume->corner = src_volume->corner;

  /* make a separate copy in memory of the volume's name and scan date */
  volume_set_name(dest_volume, src_volume->name);
  volume_set_scan_date(dest_volume, src_volume->scan_date);

  /* make a separate copy in memory of the volume's frame durations and max/min values */
  dest_volume->frame_duration = volume_get_frame_duration_mem(dest_volume);
  dest_volume->frame_max = volume_get_frame_max_min_mem(dest_volume);
  dest_volume->frame_min = volume_get_frame_max_min_mem(dest_volume);
  if ((dest_volume->frame_duration == NULL) || 
      (dest_volume->frame_max == NULL) || 
      (dest_volume->frame_min == NULL)) {
    g_warning("%s: couldn't allocate space for the frame duration/max/or min info",PACKAGE);
    dest_volume = volume_unref(dest_volume);
    return dest_volume;
  }
  for (i=0;i<dest_volume->data_set->dim.t;i++) {
    dest_volume->frame_duration[i] = src_volume->frame_duration[i];
    dest_volume->frame_max[i] = src_volume->frame_max[i];
    dest_volume->frame_min[i] = src_volume->frame_min[i];
  }

  return dest_volume;
}

/* adds one to the reference count of a volume */
volume_t * volume_ref(volume_t * volume) {
  g_return_val_if_fail(volume != NULL, NULL);   /* sanity checks */
  volume->ref_count++;
  return volume;
}

/* adds an alignment point to a volume */
void volume_add_align_pt(volume_t * volume, align_pt_t * new_pt) {
  g_return_if_fail(volume != NULL);
  volume->align_pts = align_pts_add_pt(volume->align_pts, new_pt);
  return;
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

/* used when changing the external scaling, so that we can keep a pregenerated scaling factor */
void volume_set_scaling(volume_t * volume, amide_data_t new_external_scaling) {

  voxelpoint_t i;

  /* sanity check */
  g_return_if_fail(volume->internal_scaling != NULL);

  volume->external_scaling = new_external_scaling;
  if (volume->current_scaling != NULL)
    if ((volume->current_scaling->dim.x != volume->internal_scaling->dim.x) ||
	(volume->current_scaling->dim.y != volume->internal_scaling->dim.y) ||
	(volume->current_scaling->dim.z != volume->internal_scaling->dim.z) ||
	(volume->current_scaling->dim.t != volume->internal_scaling->dim.t))
      volume->current_scaling = data_set_unref(volume->current_scaling);
  
  if (volume->current_scaling == NULL) {
    if ((volume->current_scaling = data_set_init()) == NULL) {
      g_warning("%s: Couldn't allocate space for current scaling stucture", PACKAGE);
      return;
    }
    volume->current_scaling->dim = volume->internal_scaling->dim;
    volume->current_scaling->format = volume->internal_scaling->format;
    volume->current_scaling->data = data_set_get_data_mem(volume->current_scaling);
  }

  for(i.t = 0; i.t < volume->current_scaling->dim.t; i.t++) 
    for (i.z = 0; i.z < volume->current_scaling->dim.z; i.z++) 
      for (i.y = 0; i.y < volume->current_scaling->dim.y; i.y++) 
	for (i.x = 0; i.x < volume->current_scaling->dim.x; i.x++) 
	  *DATA_SET_FLOAT_POINTER(volume->current_scaling, i) = 
	    volume->external_scaling *
	    *DATA_SET_FLOAT_POINTER(volume->internal_scaling, i);

  return;
}

/* figure out the center of the volume in the base coord frame */
realpoint_t volume_center(const volume_t * volume) {

  realpoint_t center;
  realpoint_t corner[2];

  /* get the far corner (in volume coords) */
  corner[0] = realspace_base_coord_to_alt(rs_offset(volume->coord_frame), volume->coord_frame);
  corner[1] = volume->corner;
 
  /* the center in volume coords is then just half the far corner */
  center = rp_cmult(0.5, rp_add(corner[1],corner[0]));
  
  /* now, translate that into real coords */
  center = realspace_alt_coord_to_base(center, volume->coord_frame);

  return center;
}

/* set the center of the volume in the base coord frame */
/* make sure you set the volume's corner before calling this */
void volume_set_center(volume_t * volume, realpoint_t center) {

  realpoint_t offset;
  center = realspace_base_coord_to_alt(center, volume->coord_frame);
  offset = rp_sub(center, rp_cmult(0.5, volume->corner));
  offset = realspace_alt_coord_to_base(offset, volume->coord_frame);

  rs_set_offset(volume->coord_frame, offset);

  return;
}


/* returns the start time of the given frame */
amide_time_t volume_start_time(const volume_t * volume, guint frame) {

  amide_time_t time;
  guint i_frame;

  if (frame >= volume->data_set->dim.t)
    frame = volume->data_set->dim.t-1;

  time = volume->scan_start;

  for(i_frame=0;i_frame<frame;i_frame++)
    time += volume->frame_duration[i_frame];

  return time;
}

/* returns the end time of a given frame */
amide_time_t volume_end_time(const volume_t * volume, guint frame) {

  if (frame >= volume->data_set->dim.t)
    frame = volume->data_set->dim.t-1;

  return volume_start_time(volume, frame) + volume->frame_duration[frame];
}

/* returns the frame that corresponds to the time */
guint volume_frame(const volume_t * volume, const amide_time_t time) {

  amide_time_t start, end;
  guint i_frame;

  start = volume_start_time(volume, 0);
  if (time <= start)
    return 0;

  for(i_frame=0; i_frame <volume->data_set->dim.t; i_frame++) {
    start = volume_start_time(volume, i_frame);
    end = volume_end_time(volume, i_frame);
    if ((start <= time) && (time <= end))
      return i_frame;
  }

  /* must be past the end */
  return volume->data_set->dim.t-1;
}

/* return the minimal frame duration in this volume */
amide_time_t volume_min_frame_duration(const volume_t * volume) {

  amide_time_t min_frame_duration;
  guint i_frame;

  min_frame_duration = volume->frame_duration[0];
  
  for(i_frame=1;i_frame<volume->data_set->dim.t;i_frame++)
    if (volume->frame_duration[i_frame] < min_frame_duration)
      min_frame_duration = volume->frame_duration[i_frame];

  return min_frame_duration;
}

/* function to recalculate the far corner of a volume */
void volume_recalc_far_corner(volume_t * volume) {

  REALPOINT_MULT(volume->data_set->dim, volume->voxel_size, volume->corner);
  return;
}

/* function to calculate the global max and min, the frame max and mins need to already be calculated */
void volume_calc_global_max_min(volume_t * volume) {

  guint i;

  volume->global_max = volume->frame_max[0];
  volume->global_min = volume->frame_min[0];
  for (i=1; i<volume->data_set->dim.t; i++) {
    if (volume->global_max < volume->frame_max[i]) 
      volume->global_max = volume->frame_max[i];
    if (volume->global_min > volume->frame_min[i])
      volume->global_min = volume->frame_min[i];
  }

#ifdef AMIDE_DEBUG
  g_print("\tglobal max %5.3f global min %5.3f\n",volume->global_max,volume->global_min);
#endif
   
  return;
}

/* function to calculate the max and min over the data frames */
void volume_calc_frame_max_min(volume_t * volume) {

  /* sanity checks */
  g_return_if_fail(volume->data_set != NULL);

  /* malloc the arrays if we haven't already */
  if (volume->frame_max == NULL) {
    if ((volume->frame_max = volume_get_frame_max_min_mem(volume)) == NULL) {
      g_warning("%s: couldn't allocate space for the frame max info",PACKAGE);
      return;
    }
    if ((volume->frame_min = volume_get_frame_max_min_mem(volume)) == NULL) {
      g_warning("%s: couldn't allocate space for the frame min info",PACKAGE);
      return;
    }
  }

  g_return_if_fail(volume->frame_max != NULL);
  g_return_if_fail(volume->frame_min != NULL);

  /* hand everything off to the data type specific function */
  switch(volume->data_set->format) {
  case UBYTE:
    if (volume->internal_scaling->dim.z > 1) 
      volume_UBYTE_2D_SCALING_calc_frame_max_min(volume);
    else if (volume->internal_scaling->dim.t > 1)
      volume_UBYTE_1D_SCALING_calc_frame_max_min(volume);
    else 
      volume_UBYTE_0D_SCALING_calc_frame_max_min(volume);
    break;
  case SBYTE:
    if (volume->internal_scaling->dim.z > 1) 
      volume_SBYTE_2D_SCALING_calc_frame_max_min(volume);
    else if (volume->internal_scaling->dim.t > 1)
      volume_SBYTE_1D_SCALING_calc_frame_max_min(volume);
    else 
      volume_SBYTE_0D_SCALING_calc_frame_max_min(volume);
    break;
  case USHORT:
    if (volume->internal_scaling->dim.z > 1) 
      volume_USHORT_2D_SCALING_calc_frame_max_min(volume);
    else if (volume->internal_scaling->dim.t > 1)
      volume_USHORT_1D_SCALING_calc_frame_max_min(volume);
    else 
      volume_USHORT_0D_SCALING_calc_frame_max_min(volume);
    break;
  case SSHORT:
    if (volume->internal_scaling->dim.z > 1) 
      volume_SSHORT_2D_SCALING_calc_frame_max_min(volume);
    else if (volume->internal_scaling->dim.t > 1)
      volume_SSHORT_1D_SCALING_calc_frame_max_min(volume);
    else 
      volume_SSHORT_0D_SCALING_calc_frame_max_min(volume);
    break;
  case UINT:
    if (volume->internal_scaling->dim.z > 1) 
      volume_UINT_2D_SCALING_calc_frame_max_min(volume);
    else if (volume->internal_scaling->dim.t > 1)
      volume_UINT_1D_SCALING_calc_frame_max_min(volume);
    else 
      volume_UINT_0D_SCALING_calc_frame_max_min(volume);
    break;
  case SINT:
    if (volume->internal_scaling->dim.z > 1) 
      volume_SINT_2D_SCALING_calc_frame_max_min(volume);
    else if (volume->internal_scaling->dim.t > 1)
      volume_SINT_1D_SCALING_calc_frame_max_min(volume);
    else 
      volume_SINT_0D_SCALING_calc_frame_max_min(volume);
    break;
  case FLOAT:
    if (volume->internal_scaling->dim.z > 1) 
      volume_FLOAT_2D_SCALING_calc_frame_max_min(volume);
    else if (volume->internal_scaling->dim.t > 1)
      volume_FLOAT_1D_SCALING_calc_frame_max_min(volume);
    else 
      volume_FLOAT_0D_SCALING_calc_frame_max_min(volume);
    break;
  case DOUBLE:
    if (volume->internal_scaling->dim.z > 1) 
      volume_DOUBLE_2D_SCALING_calc_frame_max_min(volume);
    else if (volume->internal_scaling->dim.t > 1)
      volume_DOUBLE_1D_SCALING_calc_frame_max_min(volume);
    else 
      volume_DOUBLE_0D_SCALING_calc_frame_max_min(volume);
    break;
  default:
    g_warning("%s: unexpected case in %s at line %d", PACKAGE, __FILE__, __LINE__);
    break;
  }

  return;
}

amide_data_t volume_max(volume_t * volume, amide_time_t start, amide_time_t duration) {

  guint start_frame;
  guint end_frame;
  guint i_frame;
  amide_time_t used_start, used_end;
  amide_time_t volume_start, volume_end;
  amide_data_t max;
  amide_data_t time_weight;

  /* figure out what frames of this volume to use */
  used_start = start;
  start_frame = volume_frame(volume, used_start);
  volume_start = volume_start_time(volume, start_frame);

  used_end = start+duration;
  end_frame = volume_frame(volume, used_end);
  volume_end = volume_end_time(volume, end_frame);

  if (start_frame == end_frame) {
    max = volume->frame_max[start_frame];
  } else {
    if (volume_end < used_end)
      used_end = volume_end;
    if (volume_start > used_start)
      used_start = volume_start;

    max = 0;
    for (i_frame=start_frame; i_frame <= end_frame; i_frame++) {

      if (i_frame == start_frame)
	time_weight = (volume_end_time(volume, start_frame)-used_start)/(used_end-used_start);
      else if (i_frame == end_frame)
	time_weight = (used_end-volume_start_time(volume, end_frame))/(used_end-used_start);
      else
	time_weight = volume->frame_duration[i_frame]/(used_end-used_start);

      max += time_weight*volume->frame_max[i_frame];
    }
  }

  return max;
}

amide_data_t volume_min(volume_t * volume, amide_time_t start, amide_time_t duration) {


  guint start_frame;
  guint end_frame;
  guint i_frame;
  amide_time_t used_start, used_end;
  amide_time_t volume_start, volume_end;
  amide_data_t min;
  amide_data_t time_weight;

  /* figure out what frames of this volume to use */
  used_start = start;
  start_frame = volume_frame(volume, used_start);
  volume_start = volume_start_time(volume, start_frame);

  used_end = start+duration;
  end_frame = volume_frame(volume, used_end);
  volume_end = volume_end_time(volume, end_frame);

  if (start_frame == end_frame) {
    min = volume->frame_min[start_frame];
  } else {
    if (volume_end < used_end)
      used_end = volume_end;
    if (volume_start > used_start)
      used_start = volume_start;

    min = 0;
    for (i_frame=start_frame; i_frame <= end_frame; i_frame++) {

      if (i_frame == start_frame)
	time_weight = (volume_end_time(volume, start_frame)-used_start)/(used_end-used_start);
      else if (i_frame == end_frame)
	time_weight = (used_end-volume_start_time(volume, end_frame))/(used_end-used_start);
      else
	time_weight = volume->frame_duration[i_frame]/(used_end-used_start);

      min += time_weight*volume->frame_min[i_frame];
    }
  }

  return min;
}



/* generate the distribution array for a volume */
void volume_generate_distribution(volume_t * volume) {

  /* hand everything off to the data type specific function */
  switch(volume->data_set->format) {
  case UBYTE:
    if (volume->internal_scaling->dim.z > 1) 
      volume_UBYTE_2D_SCALING_generate_distribution(volume);
    else if (volume->internal_scaling->dim.t > 1)
      volume_UBYTE_1D_SCALING_generate_distribution(volume);
    else 
      volume_UBYTE_0D_SCALING_generate_distribution(volume);
    break;
  case SBYTE:
    if (volume->internal_scaling->dim.z > 1) 
      volume_SBYTE_2D_SCALING_generate_distribution(volume);
    else if (volume->internal_scaling->dim.t > 1)
      volume_SBYTE_1D_SCALING_generate_distribution(volume);
    else 
      volume_SBYTE_0D_SCALING_generate_distribution(volume);
    break;
  case USHORT:
    if (volume->internal_scaling->dim.z > 1) 
      volume_USHORT_2D_SCALING_generate_distribution(volume);
    else if (volume->internal_scaling->dim.t > 1)
      volume_USHORT_1D_SCALING_generate_distribution(volume);
    else 
      volume_USHORT_0D_SCALING_generate_distribution(volume);
    break;
  case SSHORT:
    if (volume->internal_scaling->dim.z > 1) 
      volume_SSHORT_2D_SCALING_generate_distribution(volume);
    else if (volume->internal_scaling->dim.t > 1)
      volume_SSHORT_1D_SCALING_generate_distribution(volume);
    else 
      volume_SSHORT_0D_SCALING_generate_distribution(volume);
    break;
  case UINT:
    if (volume->internal_scaling->dim.z > 1) 
      volume_UINT_2D_SCALING_generate_distribution(volume);
    else if (volume->internal_scaling->dim.t > 1)
      volume_UINT_1D_SCALING_generate_distribution(volume);
    else 
      volume_UINT_0D_SCALING_generate_distribution(volume);
    break;
  case SINT:
    if (volume->internal_scaling->dim.z > 1) 
      volume_SINT_2D_SCALING_generate_distribution(volume);
    else if (volume->internal_scaling->dim.t > 1)
      volume_SINT_1D_SCALING_generate_distribution(volume);
    else 
      volume_SINT_0D_SCALING_generate_distribution(volume);
    break;
  case FLOAT:
    if (volume->internal_scaling->dim.z > 1) 
      volume_FLOAT_2D_SCALING_generate_distribution(volume);
    else if (volume->internal_scaling->dim.t > 1)
      volume_FLOAT_1D_SCALING_generate_distribution(volume);
    else 
      volume_FLOAT_0D_SCALING_generate_distribution(volume);
    break;
  case DOUBLE:
    if (volume->internal_scaling->dim.z > 1) 
      volume_DOUBLE_2D_SCALING_generate_distribution(volume);
    else if (volume->internal_scaling->dim.t > 1)
      volume_DOUBLE_1D_SCALING_generate_distribution(volume);
    else 
      volume_DOUBLE_0D_SCALING_generate_distribution(volume);
    break;
  default:
    g_warning("%s: unexpected case in %s at line %d", PACKAGE, __FILE__, __LINE__);
    break;
  }

  return;
}

  
amide_data_t volume_value(const volume_t * volume, const voxelpoint_t i) {

  if (volume == NULL) return EMPTY;
  if (!data_set_includes_voxel(volume->data_set, i)) return EMPTY;

  /* hand everything off to the data type specific function */
  switch(volume->data_set->format) {
  case UBYTE:
    if (volume->internal_scaling->dim.z > 1) 
      return VOLUME_UBYTE_2D_SCALING_CONTENTS(volume,i);
    else if (volume->internal_scaling->dim.t > 1)
      return VOLUME_UBYTE_1D_SCALING_CONTENTS(volume,i);
    else 
      return VOLUME_UBYTE_0D_SCALING_CONTENTS(volume,i);
    break;
  case SBYTE:
    if (volume->internal_scaling->dim.z > 1) 
      return VOLUME_SBYTE_2D_SCALING_CONTENTS(volume,i);
    else if (volume->internal_scaling->dim.t > 1)
      return VOLUME_SBYTE_1D_SCALING_CONTENTS(volume,i);
    else 
      return VOLUME_SBYTE_0D_SCALING_CONTENTS(volume,i);
    break;
  case USHORT:
    if (volume->internal_scaling->dim.z > 1) 
      return VOLUME_USHORT_2D_SCALING_CONTENTS(volume,i);
    else if (volume->internal_scaling->dim.t > 1)
      return VOLUME_USHORT_1D_SCALING_CONTENTS(volume,i);
    else 
      return VOLUME_USHORT_0D_SCALING_CONTENTS(volume,i);
    break;
  case SSHORT:
    if (volume->internal_scaling->dim.z > 1) 
      return VOLUME_SSHORT_2D_SCALING_CONTENTS(volume,i);
    else if (volume->internal_scaling->dim.t > 1)
      return VOLUME_SSHORT_1D_SCALING_CONTENTS(volume,i);
    else 
      return VOLUME_SSHORT_0D_SCALING_CONTENTS(volume,i);
    break;
  case UINT:
    if (volume->internal_scaling->dim.z > 1) 
      return VOLUME_UINT_2D_SCALING_CONTENTS(volume,i);
    else if (volume->internal_scaling->dim.t > 1)
      return VOLUME_UINT_1D_SCALING_CONTENTS(volume,i);
    else 
      return VOLUME_UINT_0D_SCALING_CONTENTS(volume,i);
    break;
  case SINT:
    if (volume->internal_scaling->dim.z > 1) 
      return VOLUME_SINT_2D_SCALING_CONTENTS(volume,i);
    else if (volume->internal_scaling->dim.t > 1)
      return VOLUME_SINT_1D_SCALING_CONTENTS(volume,i);
    else 
      return VOLUME_SINT_0D_SCALING_CONTENTS(volume,i);
    break;
  case FLOAT:
    if (volume->internal_scaling->dim.z > 1) 
      return VOLUME_FLOAT_2D_SCALING_CONTENTS(volume,i);
    else if (volume->internal_scaling->dim.t > 1)
      return VOLUME_FLOAT_1D_SCALING_CONTENTS(volume,i);
    else 
      return VOLUME_FLOAT_0D_SCALING_CONTENTS(volume,i);
    break;
  case DOUBLE:
    if (volume->internal_scaling->dim.z > 1) 
      return VOLUME_DOUBLE_2D_SCALING_CONTENTS(volume,i);
    else if (volume->internal_scaling->dim.t > 1)
      return VOLUME_DOUBLE_1D_SCALING_CONTENTS(volume,i);
    else 
      return VOLUME_DOUBLE_0D_SCALING_CONTENTS(volume,i);
    break;
  default:
    g_warning("%s: unexpected case in %s at line %d", PACKAGE, __FILE__, __LINE__);
    return 0.0;
    break;
  }
}


/* free up a list of volumes */
volumes_t * volumes_unref(volumes_t * volumes) {
  
  volumes_t * return_list;

  if (volumes == NULL) return volumes;

  /* sanity check */
  g_return_val_if_fail(volumes->ref_count > 0, NULL);

  /* remove a reference count */
  volumes->ref_count--;


  /* things to do if our reference count is zero */
  if (volumes->ref_count == 0) {
    /* recursively delete rest of list */
    return_list = volumes_unref(volumes->next); 
    volumes->next = NULL;

    volumes->volume = volume_unref(volumes->volume);
    g_free(volumes);
    volumes = NULL;
  } else
    return_list = volumes;

  return return_list;
}

/* returns an initialized volume list node structure */
volumes_t * volumes_init(volume_t * volume) {
  
  volumes_t * temp_volumes;
  
  if ((temp_volumes = (volumes_t *) g_malloc(sizeof(volumes_t))) == NULL) {
    g_warning("%s: Couldn't allocate memory for the volume list",PACKAGE);
    return temp_volumes;
  }
  temp_volumes->ref_count = 1;
  
  temp_volumes->volume = volume_ref(volume);
  temp_volumes->next = NULL;
  
  return temp_volumes;
}

/* count the number of volumes in a volume list */
guint volumes_count(volumes_t * list) {
  if (list == NULL) return 0;
  else return (1+volumes_count(list->next));
}

/* function to write a list of volumes as xml data.  Function calls
   volume_write_xml to writeout each volume, and adds information about the
   volume file to the node_list. */
void volumes_write_xml(volumes_t *list, xmlNodePtr node_list, gchar * study_directory) {

  gchar * volume_xml_filename;

  if (list != NULL) { 
    volume_xml_filename = volume_write_xml(list->volume, study_directory);
    xmlNewChild(node_list, NULL,"Volume_file", volume_xml_filename);
    g_free(volume_xml_filename);
    
    /* and recurse */
    volumes_write_xml(list->next, node_list, study_directory);
  }

  return;
}


/* function to load in a list of volume xml nodes */
volumes_t * volumes_load_xml(xmlNodePtr node_list, const gchar * study_directory) {

  gchar * file_name;
  volumes_t * new_volumes;
  volume_t * new_volume;

  if (node_list != NULL) {
    /* first, recurse on through the list */
    new_volumes = volumes_load_xml(node_list->next, study_directory);

    /* load in this node */
    file_name = xml_get_string(node_list->children, "text");
    new_volume = volume_load_xml(file_name,study_directory);
    new_volumes = volumes_add_volume_first(new_volumes, new_volume);
    new_volume = volume_unref(new_volume);
    g_free(file_name);

  } else
    new_volumes = NULL;

  return new_volumes;
}

/* makes a new volumes which is a copy of a previous volumes */
volumes_t * volumes_copy(volumes_t * src_volumes) {

  volumes_t * dest_volumes;
  volume_t * temp_volume;

  /* sanity check */
  g_return_val_if_fail(src_volumes != NULL, NULL);

  /* make a separate copy in memory of the volume this list item points to */
  temp_volume = volume_copy(src_volumes->volume); 

  dest_volumes = volumes_init(temp_volume); 
  temp_volume = volume_unref(temp_volume); /* no longer need a reference outside of the list's reference */

  /* and make copies of the rest of the elements in this list */
  if (src_volumes->next != NULL)
    dest_volumes->next = volumes_copy(src_volumes->next);
  
  return dest_volumes;
}

/* adds one to the reference count of a volume list element*/
volumes_t * volumes_ref(volumes_t * volumes) {
  g_return_val_if_fail(volumes != NULL, NULL);
  volumes->ref_count++;
  return volumes;
}


/* function to check that a volume is in a volume list */
gboolean volumes_includes_volume(volumes_t * volumes,  volume_t * volume) {

  while (volumes != NULL)
    if (volumes->volume == volume)
      return TRUE;
    else
      volumes = volumes->next;

  return FALSE;
}


/* function to add a volume to the end of a volume list */
volumes_t * volumes_add_volume(volumes_t * volumes, volume_t * vol) {

  volumes_t * temp_list = volumes;
  volumes_t * prev_list = NULL;

  g_return_val_if_fail(vol != NULL, volumes);

  /* get to the end of the list */
  while (temp_list != NULL) {
    prev_list = temp_list;
    temp_list = temp_list->next;
  }
  
  /* get a new volumes data structure */
  temp_list = volumes_init(vol);

  if (volumes == NULL)
    return temp_list;
  else {
    prev_list->next = temp_list;
    return volumes;
  }
}


/* function to add a volume onto a volume list as the first item*/
volumes_t * volumes_add_volume_first(volumes_t * volumes, volume_t * vol) {

  volumes_t * temp_list;

  g_return_val_if_fail(vol != NULL, volumes);

  temp_list = volumes_add_volume(NULL,vol);
  temp_list->next = volumes;

  return temp_list;
}


/* function to remove a volume list item from a volume list */
volumes_t * volumes_remove_volume(volumes_t * volumes, volume_t * vol) {

  volumes_t * temp_list = volumes;
  volumes_t * prev_list = NULL;

  while (temp_list != NULL) {
    if (temp_list->volume == vol) {
      if (prev_list == NULL)
	volumes = temp_list->next;
      else
	prev_list->next = temp_list->next;
      temp_list->next = NULL;
      temp_list = volumes_unref(temp_list);
    } else {
      prev_list = temp_list;
      temp_list = temp_list->next;
    }
  }

  return volumes;
}


/* function to get the earliest time in a list of volumes */
amide_time_t volumes_start_time(volumes_t * volumes) {
  amide_time_t start, temp;

  /* sanity check */
  g_return_val_if_fail(volumes != NULL, 0.0);

  start = volume_start_time(volumes->volume,0);
  if (volumes->next != NULL) {
    temp = volumes_start_time(volumes->next);
    if (temp < start)
      start = temp;
  }
  
  return start;
}

/* function to get the latest time in a list of volumes */
amide_time_t volumes_end_time(volumes_t * volumes) {
  amide_time_t end, temp;

  /* sanity check */
  g_return_val_if_fail(volumes != NULL, 0.0);

  end = volume_end_time(volumes->volume,volumes->volume->data_set->dim.t-1);
  if (volumes->next != NULL) {
    temp = volumes_end_time(volumes->next);
    if (temp > end)
      end = temp;
  }
  
  return end;
}

/* function to return the minimum frame duration of a list of volumes */
amide_time_t volumes_min_frame_duration(volumes_t * volumes) {
  amide_time_t min_duration, temp;

  /* sanity check */
  g_return_val_if_fail(volumes != NULL, 0.0);

  min_duration = volume_min_frame_duration(volumes->volume);
  if (volumes->next != NULL) {
    temp = volumes_min_frame_duration(volumes->next);
    if (temp < min_duration)
      min_duration = temp;
  }
  
  return min_duration;
}


/* takes a volume and a view_axis, and gives the corners necessary for 
   the view to totally encompass the volume in the view coord frame */
void volume_get_view_corners(const volume_t * volume,
			     const realspace_t * view_coord_frame,
			     realpoint_t view_corner[]) {

  realpoint_t volume_corner[2];

  g_assert(volume!=NULL);

  volume_corner[0] = realspace_base_coord_to_alt(rs_offset(volume->coord_frame),
						 volume->coord_frame);
  volume_corner[1] = volume->corner;

  /* look at all eight corners of our cube, figure out the min and max coords */
  realspace_get_enclosing_corners(volume->coord_frame, volume_corner,
				  view_coord_frame, view_corner);
  return;
}


/* takes a list of volumes and a view axis, and give the corners
   necessary to totally encompass the volume in the view coord frame */
void volumes_get_view_corners(volumes_t * volumes,
			      const realspace_t * view_coord_frame,
			      realpoint_t view_corner[]) {

  volumes_t * temp_volumes;
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

  return;
}


/* returns the minimum voxel dimensions of the list of volumes */
floatpoint_t volumes_min_voxel_size(volumes_t * volumes) {

  floatpoint_t min_voxel_size, temp;

  g_assert(volumes!=NULL);

  if (volumes->next == NULL)
    min_voxel_size = rp_min_dim(volumes->volume->voxel_size);
  else {
    min_voxel_size = volumes_min_voxel_size(volumes->next);
    temp = rp_min_dim(volumes->volume->voxel_size);
    if (temp < min_voxel_size) min_voxel_size = temp;
  }

  return min_voxel_size;
}

/* returns the maximal dimensional size of a list of volumes */
floatpoint_t volumes_max_size(volumes_t * volumes) {

  floatpoint_t temp, max_dim;

  if (volumes == NULL)
    return 0.0;
  else {
    max_dim = volumes_max_size(volumes->next);
    temp = rp_max_dim(volumes->volume->voxel_size) * REALPOINT_MAX(volumes->volume->data_set->dim);
    if (temp > max_dim)
      max_dim = temp;
  }

  return max_dim;
}


/* returns the minimum dimensional width of the volumes with the largest voxel size */
floatpoint_t volumes_max_min_voxel_size(volumes_t * volumes) {

  floatpoint_t min_voxel_size, temp;

  g_assert(volumes!=NULL);

  /* figure out what our voxel size is going to be for our returned
     slices.  I'm going to base this on the volume with the largest
     minimum voxel dimension, this way the user doesn't suddenly get a
     huge image when adding in a study with small voxels (i.e. a CT
     scan).  The user can always increase the zoom later*/
  /* figure out out thickness */
  if (volumes->next == NULL)
    min_voxel_size = rp_min_dim(volumes->volume->voxel_size);
  else {
    min_voxel_size = volumes_max_min_voxel_size(volumes->next);
    temp = rp_min_dim(volumes->volume->voxel_size);
    if (temp > min_voxel_size) min_voxel_size = temp;
  }

  return min_voxel_size;
}


/* returns the maximum dimension of the given volumes (in voxels) */
intpoint_t volumes_max_dim(volumes_t * volumes) {

  intpoint_t max_dim, temp;

  g_assert(volumes!=NULL);

  max_dim = 0;
  while (volumes != NULL) {
    temp = ceil(vp_max_dim(volumes->volume->data_set->dim));
    if (temp > max_dim) max_dim = temp;
    volumes = volumes->next;
  }

  return max_dim;
}



/* generate a volume that contains an axis figure, this is used by the rendering ui */
//volume_t * volume_get_axis_volume(guint x_width, guint y_width, guint z_width) {
//
//  volume_t * axis_volume;
//  realpoint_t center, radius;
//  floatpoint_t height;
//  realspace_t object_coord_frame;
//
//  /* initialize our data structures */
//  if ((axis_volume = volume_init()) == NULL) {
//    g_warning("%s: couldn't allocate space for the axis volume structure", PACKAGE);
//    return axis_volume;
//  }
//  if ((axis_volume->data_set = data_set_init()) == NULL) {
//    g_warning("%s: couldn't allocate space for the axis data set structure", PACKAGE);
//    return volume_unref(axis_volume);
//  }
//
//  /* initialize what variables we want */
//  volume_set_name(axis_volume, "rendering axis volume");
//  axis_volume->data_set->dim.x  = x_width;
//  axis_volume->data_set->dim.y = y_width;
//  axis_volume->data_set->dim.z = z_width;
//  axis_volume->data_set->dim.t = 1;
//  axis_volume->data_set->format = FLOAT;
//  volume_set_scaling(axis_volume, 1.0);
//  axis_volume->voxel_size.x = axis_volume->voxel_size.y = axis_volume->voxel_size.z = 1.0;
//  volume_recalc_far_corner(axis_volume);
//  
//  if ((axis_volume->frame_duration = volume_get_frame_duration_mem(axis_volume)) == NULL) {
//    g_warning("%s: couldn't allocate space for the axis volume frame duration array",PACKAGE);
//    return volume_unref(axis_volume);
//  }
//  axis_volume->frame_duration[0]=1.0;
//
//  if ((axis_volume->data_set->data = data_set_get_data_mem(axis_volume->data_set)) == NULL) {
//    g_warning("%s: couldn't allocate space for the slice",PACKAGE);
//    return volume_unref(axis_volume);
//  }
//
//  /* initialize our data set */
//  data_set_FLOAT_initialize_data(axis_volume->data_set, 0.0);
//
//  /* figure out an appropriate radius for our cylinders and spheres */
//  radius.x = rp_min_dim(axis_volume->corner)/24;
//  radius.y = radius.x;
//  radius.z = radius.x;

  /* start generating our axis */

 
  /* draw the z axis */
//  center.z = (axis_volume->corner.z*4.5/8.0);
//  height = (axis_volume->corner.z*3.0/8.0);
//  center.x = axis_volume->corner.x/2;
//  center.y = axis_volume->corner.y/2;
//  object_coord_frame = realspace_get_view_coord_frame(axis_volume->coord_frame, TRANSVERSE);
//  objects_place_elliptic_cylinder(axis_volume, 0, object_coord_frame, 
//				  center, radius, height, AXIS_VOLUME_DENSITY);

                                                        
  /* draw the y axis */
//  center.y = (axis_volume->corner.y*5.0/8.0);
//  height = (axis_volume->corner.y*4.0/8.0);
//  center.x = axis_volume->corner.x/2;
//  center.z = axis_volume->corner.z/2;
//  object_coord_frame = realspace_get_view_coord_frame(axis_volume->coord_frame, CORONAL);
//  objects_place_elliptic_cylinder(axis_volume, 0, object_coord_frame, 
//				  center, radius, height, AXIS_VOLUME_DENSITY);

 /* draw the x axis */
//  center.x = (axis_volume->corner.x*5.0/8.0);
//  height = (axis_volume->corner.x*4.0/8.0);
//  center.y = axis_volume->corner.y/2;
//  center.z = axis_volume->corner.z/2;
//  object_coord_frame = realspace_get_view_coord_frame(axis_volume->coord_frame, SAGITTAL);
//  objects_place_elliptic_cylinder(axis_volume, 0, object_coord_frame, 
//				  center, radius, height, AXIS_VOLUME_DENSITY);

//  radius.z = radius.y = radius.x = rp_min_dim(axis_volume->corner)/12;
//  center.x = (axis_volume->corner.x*7.0/8.0);
//  objects_place_ellipsoid(axis_volume, 0, object_coord_frame, 
//			  center, radius, AXIS_VOLUME_DENSITY);

  /* and figure out the max and min */
//  volume_recalc_max_min(axis_volume);
//
//  axis_volume->threshold_max = axis_volume->max;
//  axis_volume->threshold_min = axis_volume->min;
//
//  return axis_volume;
//}



/* returns a "2D" slice from a volume */
/* far_corner should be in the slice_coord_frame */
volume_t * volume_get_slice(const volume_t * volume,
			    const amide_time_t start,
			    const amide_time_t duration,
			    const realpoint_t  requested_voxel_size,
			    realspace_t * slice_coord_frame,
			    const realpoint_t far_corner,
			    const interpolation_t interpolation,
			    const gboolean need_calc_max_min) {

  volume_t * slice;

  /* sanity check */
  g_assert(volume != NULL);
  g_return_val_if_fail(volume->data_set != NULL, NULL);


  /* hand everything off to the data type specific slicer */
  switch(volume->data_set->format) {
  case UBYTE:
    if (volume->internal_scaling->dim.z > 1) 
      slice = volume_UBYTE_2D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
						slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    else if (volume->internal_scaling->dim.t > 1)
      slice = volume_UBYTE_1D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
						slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    else 
      slice = volume_UBYTE_0D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
						slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    break;
  case SBYTE:
    if (volume->internal_scaling->dim.z > 1) 
      slice = volume_SBYTE_2D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
						slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    else if (volume->internal_scaling->dim.t > 1)
      slice = volume_SBYTE_1D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
						slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    else 
      slice = volume_SBYTE_0D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
						slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    break;
  case USHORT:
    if (volume->internal_scaling->dim.z > 1) 
      slice = volume_USHORT_2D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
						 slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    else if (volume->internal_scaling->dim.t > 1)
      slice = volume_USHORT_1D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
						 slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    else 
      slice = volume_USHORT_0D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
						 slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    break;
  case SSHORT:
    if (volume->internal_scaling->dim.z > 1) 
      slice = volume_SSHORT_2D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
						 slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    else if (volume->internal_scaling->dim.t > 1)
      slice = volume_SSHORT_1D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
						 slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    else 
      slice = volume_SSHORT_0D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
						 slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    break;
  case UINT:
    if (volume->internal_scaling->dim.z > 1) 
      slice = volume_UINT_2D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
					       slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    else if (volume->internal_scaling->dim.t > 1)
      slice = volume_UINT_1D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
					       slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    else 
      slice = volume_UINT_0D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
					       slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    break;
  case SINT:
    if (volume->internal_scaling->dim.z > 1) 
      slice = volume_SINT_2D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
					       slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    else if (volume->internal_scaling->dim.t > 1)
      slice = volume_SINT_1D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
					       slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    else 
      slice = volume_SINT_0D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
					       slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    break;
  case FLOAT:
    if (volume->internal_scaling->dim.z > 1) 
      slice = volume_FLOAT_2D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
						slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    else if (volume->internal_scaling->dim.t > 1)
      slice = volume_FLOAT_1D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
						slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    else 
      slice = volume_FLOAT_0D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
						slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    break;
  case DOUBLE:
    if (volume->internal_scaling->dim.z > 1) 
      slice = volume_DOUBLE_2D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
						 slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    else if (volume->internal_scaling->dim.t > 1)
      slice = volume_DOUBLE_1D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
						 slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    else 
      slice = volume_DOUBLE_0D_SCALING_get_slice(volume, start, duration, requested_voxel_size,
						 slice_coord_frame, far_corner, interpolation, need_calc_max_min);
    break;
  default:
    slice = NULL;
    g_warning("%s: unexpected case in %s at line %d", PACKAGE, __FILE__, __LINE__);
    break;
  }

  return slice;
}





/* give a list ov volumes, returns a list of slices of equal size and orientation
   intersecting these volumes */
/* thickness (of slice) in mm, negative indicates set this value according to voxel size */
volumes_t * volumes_get_slices(volumes_t * volumes,
			       const amide_time_t start,
			       const amide_time_t duration,
			       const floatpoint_t thickness,
			       const floatpoint_t voxel_dim,
			       const realspace_t * view_coord_frame,
			       const interpolation_t interpolation,
			       const gboolean need_calc_max_min) {


  realpoint_t voxel_size;
  realspace_t * slice_coord_frame;
  realpoint_t view_corner[2];
  realpoint_t slice_corner;
  volumes_t * slices=NULL;
  volume_t * temp_slice;
#ifdef AMIDE_DEBUG
  struct timeval tv1;
  struct timeval tv2;
  gdouble time1;
  gdouble time2;
  gdouble total_time;
#endif


#ifdef AMIDE_DEBUG
  /* let's do some timing */
  gettimeofday(&tv1, NULL);
#endif

  /* sanity check */
  g_assert(volumes != NULL);

  voxel_size.x = voxel_size.y = voxel_dim;
  voxel_size.z = (thickness > 0) ? thickness : voxel_dim;

  /* figure out all encompasing corners for the slices based on our viewing axis */
  volumes_get_view_corners(volumes, view_coord_frame, view_corner);

  /* adjust the z' dimension of the view's corners */
  view_corner[0].z=0;
  view_corner[1].z=voxel_size.z;

  /* figure out the coordinate frame for the slices based on the corners returned */
  slice_coord_frame = rs_init();
  rs_set_axis(slice_coord_frame, rs_all_axis(view_coord_frame));
  rs_set_offset(slice_coord_frame, realspace_alt_coord_to_base(view_corner[0],view_coord_frame));
  slice_corner = realspace_alt_coord_to_alt(view_corner[1],view_coord_frame, slice_coord_frame);

  /* and get the slices */
  while (volumes != NULL) {
    temp_slice = volume_get_slice(volumes->volume, start, duration, voxel_size, 
				  slice_coord_frame, slice_corner, interpolation, need_calc_max_min);
    slices = volumes_add_volume(slices,temp_slice);
    temp_slice = volume_unref(temp_slice);
    volumes = volumes->next;
  }
  slice_coord_frame = rs_unref(slice_coord_frame);

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

















