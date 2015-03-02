/* render.h
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

#ifdef AMIDE_LIBVOLPACK_SUPPORT

#ifndef __RENDER_H__
#define __RENDER_H__

/* header files that are always needed with this file */
#include <volpack.h>
#include "amitk_object.h"
#include "amitk_data_set.h"

/* -------------- structures and such ------------- */

typedef gshort rendering_normal_t;
typedef guchar rendering_density_t;
typedef guchar rendering_gradient_t;
typedef enum {DENSITY_CLASSIFICATION, GRADIENT_CLASSIFICATION, NUM_CLASSIFICATIONS} classification_t;
typedef enum {HIGHEST, HIGH, FAST, FASTEST, NUM_QUALITIES} rendering_quality_t;
typedef enum {OPACITY, GRAYSCALE, NUM_PIXEL_TYPES} pixel_type_t;
typedef enum {CURVE_LINEAR, CURVE_SPLINE, NUM_CURVE_TYPES} curve_type_t;

typedef struct {        /*   contents of a voxel */
  rendering_normal_t normal;        /*   encoded surface normal vector */
  rendering_density_t density;	/*   original density */
  rendering_gradient_t gradient;	/*   original gradient */
} rendering_voxel_t;


/* dummy variable used in some macros below */
rendering_voxel_t * dummy_voxel;  


/* ----------- defines ------------- */

#define RENDERING_BYTES_PER_VOXEL	sizeof(rendering_voxel_t)/* voxel size in bytes */
#define RENDERING_VOXEL_FIELDS	        3	/* number of fields in voxel */
#define RENDERING_SHADE_FIELDS	        2	/* number of fields used for shading
						   (normal and density); must be the
						   1st fields of RawVoxel */
#define RENDERING_CLSFY_FIELDS	        2	/* number of fields used for classifying
						   (density and gradient); can be any fields
						   in the RawVoxel */


#define RENDERING_NORMAL_FIELD  	0
#define RENDERING_NORMAL_OFFSET 	vpFieldOffset(dummy_voxel, normal)
#define RENDERING_NORMAL_SIZE   	sizeof(rendering_normal_t)
#define RENDERING_NORMAL_MAX    	VP_NORM_MAX /*7923 last time I checked */


#define RENDERING_DENSITY_FIELD 	1
#define RENDERING_DENSITY_OFFSET	vpFieldOffset(dummy_voxel, density)
#define RENDERING_DENSITY_SIZE   	sizeof(rendering_density_t)
#define RENDERING_DENSITY_MAX   	VP_SCALAR_MAX /*255 last time I checked */

#define RENDERING_GRADIENT_FIELD	2
#define RENDERING_GRADIENT_OFFSET	vpFieldOffset(dummy_voxel, gradient)
#define RENDERING_GRADIENT_SIZE 	sizeof(rendering_gradient_t)
#define RENDERING_GRADIENT_MAX  	VP_GRAD_MAX /* 221 last time I checked */

#define RENDERING_DENSITY_PARAM		0      /* classification parameter */
#define RENDERING_GRADIENT_PARAM	1      /* classification parameter */

/* initial density and gradient ramps */
//#define RENDERING_DENSITY_RAMP_X {0.0, 20, RENDERING_DENSITY_MAX}
//#define RENDERING_DENSITY_RAMP_Y {0.0, 1.0, 1.0}
//#define RENDERING_DENSITY_RAMP_POINTS 3
#define RENDERING_DENSITY_RAMP_X {0.0, RENDERING_DENSITY_MAX}
#define RENDERING_DENSITY_RAMP_Y {0.0, 1.0}
#define RENDERING_DENSITY_RAMP_POINTS 2

#define RENDERING_GRADIENT_RAMP_X {0.0, RENDERING_GRADIENT_MAX}
#define RENDERING_GRADIENT_RAMP_Y {0.0, 1.0}
#define RENDERING_GRADIENT_RAMP_Y_FLAT {0.2, 0.2}
#define RENDERING_GRADIENT_RAMP_POINTS 2

#define RENDERING_OCTREE_DENSITY_THRESH 	4
#define RENDERING_OCTREE_GRADIENT_THRESH	4
#define RENDERING_OCTREE_BASE_NODE_SIZE 	4

#define RENDERING_DEFAULT_ZOOM 1.0
#define RENDERING_DEFAULT_QUALITY HIGHEST
#define RENDERING_DEFAULT_PIXEL_TYPE GRAYSCALE
#define RENDERING_DEFAULT_DEPTH_CUEING FALSE
#define RENDERING_DEFAULT_FRONT_FACTOR 1.0
#define RENDERING_DEFAULT_DENSITY 1.0

/* ------------ some more structures ------------ */

/* our rendering context structure */
typedef struct _rendering_t {
  vpContext * vpc;      /*  VolPack rendering Context */
  AmitkObject * object;
  gchar * name;
  AmitkColorTable color_table;
  pixel_type_t pixel_type;
  amide_time_t start;
  amide_time_t duration;
  gint view_start_gate;
  gint view_end_gate;
  AmitkVolume * transformed_volume; /* volume in rendering space in which the data resides  */
  AmitkVolume * extraction_volume; /* set on init, used for extracting data into the context */
  rendering_voxel_t * rendering_data;
  amide_real_t voxel_size; /* volpack needs isotropic voxels */
  AmitkVoxel dim; /* dimensions of our rendering_data and image */
  guchar * image;
  gfloat shade_table[RENDERING_NORMAL_MAX+1];	/* shading lookup table */
  gfloat density_ramp[RENDERING_DENSITY_MAX+1]; /* opacity as a function */
  gfloat gradient_ramp[RENDERING_GRADIENT_MAX+1]; /* opacity as a function */
  gint * ramp_x[NUM_CLASSIFICATIONS];
  gfloat * ramp_y[NUM_CLASSIFICATIONS];
  guint num_points[NUM_CLASSIFICATIONS];
  curve_type_t curve_type[NUM_CLASSIFICATIONS];
  gboolean zero_fill;
  gboolean optimize_rendering;
  gboolean need_rerender;
  gboolean need_reclassify;
  guint ref_count;
} rendering_t;


/* a list of rendering contexts */
typedef struct _renderings_t renderings_t;
struct _renderings_t {
  rendering_t * rendering;
  guint ref_count;
  renderings_t * next;
};



/* notes:
   the update function needs to have the folowing syntax:
   gboolean update_func(gchar * message, gfloat fraction, gpointer data);
*/

/* external functions */
rendering_t * rendering_unref(rendering_t * rendering);
rendering_t * rendering_init(const AmitkObject * object,
			     AmitkVolume * rendering_volume,
			     const amide_real_t min_voxel_size, 
			     const amide_time_t start, 
			     const amide_time_t duration,
			     const gboolean zero_fill,
			     const gboolean optimize_rendering,
			     const gboolean no_gradient_opacity,
			     AmitkUpdateFunc update_func,
			     gpointer update_data);
gboolean rendering_reload_object(rendering_t * rendering, 
				 const amide_time_t new_start,
				 const amide_time_t new_duration,
				 AmitkUpdateFunc update_func,
				 gpointer update_data);
gboolean rendering_load_object(rendering_t * rendering, 
			       AmitkUpdateFunc update_func,
			       gpointer update_data);
void rendering_set_space(rendering_t * rendering, AmitkSpace * space);
void rendering_set_rotation(rendering_t * rendering, AmitkAxis dir, gdouble rotation);
void rendering_reset_rotation(rendering_t * rendering);
void rendering_set_quality(rendering_t * rendering, rendering_quality_t quality);
void rendering_set_image(rendering_t * rendering, pixel_type_t pixel_type, gdouble zoom);
void rendering_set_depth_cueing(rendering_t * rendering, gboolean state);
void rendering_set_depth_cueing_parameters(rendering_t * rendering, 
						   gdouble front_factor, gdouble density);
void rendering_render(rendering_t * rendering);
renderings_t * renderings_unref(renderings_t * renderings);
renderings_t * renderings_init(GList * objects, 
			       const amide_time_t start, 
			       const amide_time_t duration, 
			       const gboolean zero_fill,
			       const gboolean optimize_rendering,
			       const gboolean no_gradient_opacity,
			       const amide_real_t fov,
			       const AmitkPoint view_center,
			       AmitkUpdateFunc update_func,
			       gpointer update_data);
gboolean renderings_reload_objects(renderings_t * renderings, 
				   const amide_time_t start, 
				   const amide_time_t duration,
				   AmitkUpdateFunc update_func,
				   gpointer update_data);
void renderings_set_space(renderings_t * renderings, AmitkSpace * space);
void renderings_set_rotation(renderings_t * renderings, AmitkAxis dir, gdouble rotation);
void renderings_reset_rotation(renderings_t * renderings);
void renderings_set_quality(renderings_t * renderlings, rendering_quality_t quality);
void renderings_set_zoom(renderings_t * renderings, gdouble zoom);
void renderings_set_depth_cueing(renderings_t * renderings, gboolean state);
void renderings_set_depth_cueing_parameters(renderings_t * renderings, 
					    gdouble front_factor, gdouble density);
void renderings_render(renderings_t * renderings);
guint renderings_count(renderings_t * renderings);

/* external variables */
extern gchar * rendering_quality_names[];
extern gchar * pixel_type_names[];





/* external funnctions from render.c */
//gboolean render_set_material_shinyness(gint material_num, gdouble shinyness);

#endif /*  __RENDERING_H__ */
#endif /* AMIDE_LIBVOLPACK_SUPPORT */
