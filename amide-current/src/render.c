/* rendering.c
 *
 * Part of amide - Amide's a Medical Image Dataset Examiner
 * Copyright (C) 2001-2003 Andy Loening
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

#ifdef AMIDE_LIBVOLPACK_SUPPORT

#include <glib.h>
#include <stdlib.h>
#include "render.h"
#include "amitk_roi.h"
#include "amitk_data_set_DOUBLE_0D_SCALING.h"


/* external variables */
gchar * rendering_quality_names[] = {"Highest Quality and Slowest",
				     "High Quality and Medium Speed",
				     "Medium Quality and Fast",
				     "Low Quality and Fastest"};
gchar * pixel_type_names[] = {"Opacity",
			      "Grayscale"};
			      
static void classify_volume(rendering_t * rendering);

rendering_t * rendering_unref(rendering_t * rendering) {
  
  classification_t i_class;

  if (rendering== NULL)
    return rendering;

  /* sanity checks */
  g_return_val_if_fail(rendering->ref_count > 0, NULL);

  rendering->ref_count--; 

#ifdef AMIDE_DEBUG
  if (rendering->ref_count == 0)
    g_print("freeing rendering context of %s\n",rendering->name);
#endif

  /* if we've removed all reference's, free the context */
  if (rendering->ref_count == 0) {
    if (rendering->object != NULL) {
      g_object_unref(rendering->object);
      rendering->object = NULL;
    }

    if (rendering->rendering_data != NULL) {
      g_free(rendering->rendering_data);
      rendering->rendering_data = NULL;
    }

    if (rendering->name != NULL) {
      g_free(rendering->name);
      rendering->name = NULL;
    }

    if (rendering->vpc != NULL) {
      vpDestroyContext(rendering->vpc);
      rendering->vpc = NULL;
    }

    if (rendering->image != NULL) {
      g_free(rendering->image);
      rendering->image = NULL;
    }

    for (i_class = 0; i_class < NUM_CLASSIFICATIONS; i_class++) {
      g_free(rendering->ramp_x[i_class]);
      g_free(rendering->ramp_y[i_class]);
    }
    
    if (rendering->volume != NULL) {
      g_object_unref(rendering->volume);
      rendering->volume = NULL;
    }

    g_free(rendering);
    rendering = NULL;
  }

  return rendering;

}



/* either volume or object must be NULL */
rendering_t * rendering_init(const AmitkObject * object,
			     AmitkVolume * rendering_volume,
			     const amide_real_t voxel_size, 
			     const amide_time_t start, 
			     const amide_time_t duration,
			     const gboolean zero_fill,
			     const gboolean optimize_rendering,
			     gboolean (* update_func)(), 
			     gpointer update_data) {

  rendering_t * new_rendering = NULL;
  gint i;
  gint init_density_ramp_x[] = RENDERING_DENSITY_RAMP_X;
  gfloat init_density_ramp_y[] = RENDERING_DENSITY_RAMP_Y;
  gint init_gradient_ramp_x[] = RENDERING_GRADIENT_RAMP_X;
  gfloat init_gradient_ramp_y[] = RENDERING_GRADIENT_RAMP_Y;

  g_return_val_if_fail((AMITK_IS_DATA_SET(object) || AMITK_IS_ROI(object)), NULL);

  if ((new_rendering =  g_try_new(rendering_t,1)) == NULL) {
    g_warning("couldn't allocate space for rendering context");
    return NULL;
  }
  new_rendering->ref_count = 1;
  new_rendering->need_rerender = TRUE;
  new_rendering->need_reclassify = TRUE;
  
  /* start initializing what we can */
  new_rendering->vpc = vpCreateContext();
  new_rendering->object = amitk_object_copy(object);
  new_rendering->name = g_strdup(AMITK_OBJECT_NAME(object));
  if (AMITK_IS_DATA_SET(object))
    new_rendering->color_table = AMITK_DATA_SET_COLOR_TABLE(object);
  else 
    new_rendering->color_table = AMITK_COLOR_TABLE_BW_LINEAR;
  new_rendering->pixel_type = RENDERING_DEFAULT_PIXEL_TYPE;

  new_rendering->image = NULL;
  new_rendering->rendering_data = NULL;
  new_rendering->curve_type[DENSITY_CLASSIFICATION] = CURVE_LINEAR;
  new_rendering->curve_type[GRADIENT_CLASSIFICATION] = CURVE_LINEAR;
  new_rendering->volume = AMITK_VOLUME(amitk_object_copy(AMITK_OBJECT(rendering_volume)));
  new_rendering->start = start;
  new_rendering->duration = duration;
  new_rendering->zero_fill = zero_fill;
  new_rendering->optimize_rendering = optimize_rendering;

  /* figure out the size of our context */
  new_rendering->dim.x = ceil(AMITK_VOLUME_X_CORNER(rendering_volume)/voxel_size);
  new_rendering->dim.y = ceil(AMITK_VOLUME_Y_CORNER(rendering_volume)/voxel_size);
  new_rendering->dim.z = ceil(AMITK_VOLUME_Z_CORNER(rendering_volume)/voxel_size);
  new_rendering->voxel_size = voxel_size;

  /* adjust the thresholding if needed */
  if (AMITK_IS_DATA_SET(new_rendering->object))
    if (AMITK_DATA_SET_THRESHOLDING(new_rendering->object) == AMITK_THRESHOLDING_PER_SLICE) {
      amitk_data_set_set_thresholding(AMITK_DATA_SET(new_rendering->object), AMITK_THRESHOLDING_GLOBAL);
      g_warning("\"Per Slice\" thresholding illogical for conversion to volume rendering, using \"Global\" thresholding ");
    }
      


#if AMIDE_DEBUG
  g_print("\tSetting up a Rendering Context\n");
#endif

  /* setup the density and gradient ramps */
  new_rendering->num_points[DENSITY_CLASSIFICATION] = RENDERING_DENSITY_RAMP_POINTS;
  if ((new_rendering->ramp_x[DENSITY_CLASSIFICATION] = 
       g_try_new(gint, new_rendering->num_points[DENSITY_CLASSIFICATION])) == NULL) {
    g_warning("couldn't allocate space for density ramp x");
    return NULL;
  }
  if ((new_rendering->ramp_y[DENSITY_CLASSIFICATION] = 
       g_try_new(gfloat,new_rendering->num_points[DENSITY_CLASSIFICATION])) == NULL) {
    g_warning("couldn't allocate space for density ramp y");
    return NULL;
  }
  for (i=0;i<new_rendering->num_points[DENSITY_CLASSIFICATION];i++) {
    new_rendering->ramp_x[DENSITY_CLASSIFICATION][i] = init_density_ramp_x[i];
    new_rendering->ramp_y[DENSITY_CLASSIFICATION][i] = init_density_ramp_y[i];
  }

  new_rendering->num_points[GRADIENT_CLASSIFICATION] = RENDERING_GRADIENT_RAMP_POINTS;
  if ((new_rendering->ramp_x[GRADIENT_CLASSIFICATION] = 
       g_try_new(gint,new_rendering->num_points[GRADIENT_CLASSIFICATION])) == NULL) {
    g_warning("couldn't allocate space for gradient ramp x");
    return NULL;
  }
  if ((new_rendering->ramp_y[GRADIENT_CLASSIFICATION] = 
       g_try_new(gfloat,new_rendering->num_points[GRADIENT_CLASSIFICATION])) == NULL) {
    g_warning("couldn't allocate space for gradient ramp y");
    return NULL;
  }
  for (i=0;i<new_rendering->num_points[GRADIENT_CLASSIFICATION];i++) {
    new_rendering->ramp_x[GRADIENT_CLASSIFICATION][i] = init_gradient_ramp_x[i];
    new_rendering->ramp_y[GRADIENT_CLASSIFICATION][i] = init_gradient_ramp_y[i];
  }




  /* setup the context's classification function */
  if (vpRamp(new_rendering->density_ramp, sizeof(gfloat), 
	     new_rendering->num_points[DENSITY_CLASSIFICATION],
	     new_rendering->ramp_x[DENSITY_CLASSIFICATION], 
	     new_rendering->ramp_y[DENSITY_CLASSIFICATION]) != VP_OK){
    g_warning("Error Setting the Rendering Density Ramp");
    new_rendering = rendering_unref(new_rendering);
    return new_rendering;
  }

  /* setup the context's gradient classification */
  if (vpRamp(new_rendering->gradient_ramp, sizeof(gfloat), 
	     new_rendering->num_points[GRADIENT_CLASSIFICATION],
	     new_rendering->ramp_x[GRADIENT_CLASSIFICATION], 
	     new_rendering->ramp_y[GRADIENT_CLASSIFICATION]) != VP_OK) {
    g_warning("Error Setting the Rendering Gradient Ramp");
    new_rendering = rendering_unref(new_rendering);
    return new_rendering;
  }

  /* tell the rendering context info on the voxel structure */
  if (vpSetVoxelSize(new_rendering->vpc,  RENDERING_BYTES_PER_VOXEL, RENDERING_VOXEL_FIELDS, 
		     RENDERING_SHADE_FIELDS, RENDERING_CLSFY_FIELDS) != VP_OK) {
    g_warning("Error Setting the Rendering Voxel Size (%s): %s", 
	      new_rendering->name, vpGetErrorString(vpGetError(new_rendering->vpc)));
    new_rendering = rendering_unref(new_rendering);
    return new_rendering;
  }

  /* now tell the rendering context the location of each field in voxel, 
     do this for each field in the context */
  if (vpSetVoxelField (new_rendering->vpc,RENDERING_NORMAL_FIELD, RENDERING_NORMAL_SIZE, 
		       RENDERING_NORMAL_OFFSET, RENDERING_NORMAL_MAX) != VP_OK) {
    g_warning("Error Specifying the Rendering Voxel Fields (%s, NORMAL): %s", 
	      new_rendering->name, vpGetErrorString(vpGetError(new_rendering->vpc)));
    new_rendering = rendering_unref(new_rendering);
    return new_rendering;
  }
  if (vpSetVoxelField (new_rendering->vpc,RENDERING_DENSITY_FIELD, RENDERING_DENSITY_SIZE, 
		       RENDERING_DENSITY_OFFSET, RENDERING_DENSITY_MAX) != VP_OK) {
    g_warning("Error Specifying the Rendering Voxel Fields (%s, DENSITY): %s", 
	      new_rendering->name, vpGetErrorString(vpGetError(new_rendering->vpc)));
    new_rendering = rendering_unref(new_rendering);
    return new_rendering;
  }
  if (vpSetVoxelField (new_rendering->vpc,RENDERING_GRADIENT_FIELD, RENDERING_GRADIENT_SIZE, 
		       RENDERING_GRADIENT_OFFSET, RENDERING_GRADIENT_MAX) != VP_OK) {
    g_warning("Error Specifying the Rendering Voxel Fields (%s, GRADIENT): %s",
	      new_rendering->name, vpGetErrorString(vpGetError(new_rendering->vpc)));
    new_rendering = rendering_unref(new_rendering);
    return new_rendering;
  }

  /* apply density classification to the vpc */
  if (vpSetClassifierTable(new_rendering->vpc, RENDERING_DENSITY_PARAM, RENDERING_DENSITY_FIELD, 
			   new_rendering->density_ramp,
			   sizeof(new_rendering->density_ramp)) != VP_OK){
    g_warning("Error Setting the Rendering Classifier Table (%s, DENSITY): %s",
	      new_rendering->name, vpGetErrorString(vpGetError(new_rendering->vpc)));
    new_rendering = rendering_unref(new_rendering);
    return new_rendering;
  }

  /* apply it to the different vpc's */
  if (vpSetClassifierTable(new_rendering->vpc, RENDERING_GRADIENT_PARAM, RENDERING_GRADIENT_FIELD, 
			   new_rendering->gradient_ramp,
			   sizeof(new_rendering->gradient_ramp)) != VP_OK){
    g_warning("Error Setting the Classifier Table (%s, GRADIENT): %s",
	      new_rendering->name,  vpGetErrorString(vpGetError(new_rendering->vpc)));
    new_rendering = rendering_unref(new_rendering);
    return new_rendering;
  }

  /* now copy the object data into the rendering context */
  if (rendering_load_object(new_rendering, update_func, update_data) != TRUE) {
    new_rendering = rendering_unref(new_rendering);
    return new_rendering;
  }

  /* and finally, set up the structure into which the rendering will be returned */
  rendering_set_image(new_rendering, new_rendering->pixel_type, RENDERING_DEFAULT_ZOOM);

  return new_rendering;
}


/* function to reload the rendering context's concept of an object when necessary */
gboolean rendering_reload_object(rendering_t * rendering, 
				 const amide_time_t new_start,
				 const amide_time_t new_duration,
				 gboolean (* update_func)(),
				 gpointer update_data) {
  
  amide_time_t old_start, old_duration;


  old_start = rendering->start;
  old_duration = rendering->duration;

  rendering->start = new_start;
  rendering->duration = new_duration;

  /* only need to do stuff for dynamic data sets */
  if (!AMITK_IS_DATA_SET(rendering->object))
    return TRUE;
  if (!AMITK_DATA_SET_DYNAMIC(rendering->object))
    return TRUE;

  if (amitk_data_set_get_frame(AMITK_DATA_SET(rendering->object), new_start) == 
      amitk_data_set_get_frame(AMITK_DATA_SET(rendering->object), old_start))
    if (amitk_data_set_get_frame(AMITK_DATA_SET(rendering->object), new_start+new_duration) ==
	amitk_data_set_get_frame(AMITK_DATA_SET(rendering->object), old_start+old_duration))
      return TRUE;

  /* allright, reload the rendering context's data */
  rendering->need_rerender = TRUE;
  rendering->need_reclassify = TRUE; 
  return rendering_load_object(rendering, update_func, update_data);
}



/* function to update the rendering structure's concept of the object */
gboolean rendering_load_object(rendering_t * rendering, 
			       gboolean (* update_func)(), gpointer update_data) {

  AmitkVoxel i_voxel, j_voxel;
  rendering_density_t * density; /* buffer for density data */
  guint density_size;/* size of density data */
  guint context_size;/* size of context */
  div_t x;
  gint divider;
  gchar * temp_string;
  gboolean continue_work=TRUE;

  /* tell the volpack context the dimensions of our rendering context */
  if (vpSetVolumeSize(rendering->vpc, rendering->dim.x, 
		      rendering->dim.y, rendering->dim.z) != VP_OK) {
    g_warning("Error Setting the Context Size (%s): %s", 
	      rendering->name, 
	      vpGetErrorString(vpGetError(rendering->vpc)));
    return FALSE;
  }

  /* allocate space for the raw data and the context */
  density_size =  rendering->dim.x *  rendering->dim.y *  
    rendering->dim.z * RENDERING_DENSITY_SIZE;
  context_size =  rendering->dim.x *  rendering->dim.y * 
     rendering->dim.z * RENDERING_BYTES_PER_VOXEL;

  if ((density = (rendering_density_t * ) g_try_malloc(density_size)) == NULL) {
    g_warning("Could not allocate space for density data for %s", 
	      rendering->name);
    return FALSE;
  }


  if (rendering->rendering_data != NULL) {
    g_free(rendering->rendering_data);
    rendering->rendering_data = NULL;
  }

  if ((rendering->rendering_data = (rendering_voxel_t * ) g_try_malloc(context_size)) == NULL) {
    g_warning("Could not allocate space for rendering context volume for %s", 
	      rendering->name);
    g_free(density);
    return FALSE;
  }

  vpSetRawVoxels(rendering->vpc, rendering->rendering_data, context_size, 
		 RENDERING_BYTES_PER_VOXEL,  rendering->dim.x * RENDERING_BYTES_PER_VOXEL,
		 rendering->dim.x* rendering->dim.y * RENDERING_BYTES_PER_VOXEL);

  /* setup the progress information */
  if (update_func != NULL) {
    temp_string = g_strdup_printf("Converting for rendering: %s", rendering->name);
    continue_work = (*update_func)(update_data, temp_string, (gdouble) 0.0);
    g_free(temp_string);
  }
  divider = ((rendering->dim.z/AMIDE_UPDATE_DIVIDER) < 1) ? 1 : (rendering->dim.z/AMIDE_UPDATE_DIVIDER);

  if (AMITK_IS_ROI(rendering->object)) {

    AmitkPoint center, radius;
    amide_real_t height;
    AmitkPoint temp_point;
    AmitkPoint box_corner;
    gint temp_int;
    AmitkVoxel start, end;
    AmitkCorners intersection_corners;
    AmitkPoint voxel_size;

    voxel_size.x = voxel_size.y = voxel_size.z = rendering->voxel_size;
    
    radius = point_cmult(0.5, AMITK_VOLUME_CORNER(rendering->object));
    center = amitk_space_b2s(AMITK_SPACE(rendering->object),
			     amitk_volume_get_center(AMITK_VOLUME(rendering->object)));
    height = AMITK_VOLUME_Z_CORNER(rendering->object);
    box_corner = AMITK_VOLUME_CORNER(rendering->object);
    j_voxel.t = j_voxel.z=i_voxel.t=0;

    /* figure out the intersection between the rendering volume and the roi */
    if (!amitk_volume_volume_intersection_corners(rendering->volume, 
						  AMITK_VOLUME(rendering->object), 
						  intersection_corners)) {
      end = zero_voxel;
    } else {
      POINT_TO_VOXEL(intersection_corners[0], voxel_size, 0, start);
      POINT_TO_VOXEL(intersection_corners[1], voxel_size, 0, end);
    }
    
    g_return_val_if_fail(end.x < rendering->dim.x, FALSE);
    g_return_val_if_fail(end.y < rendering->dim.y, FALSE);
    g_return_val_if_fail(end.z < rendering->dim.z, FALSE);

    for (i_voxel.z = start.z; (i_voxel.z <= end.z) && (continue_work); i_voxel.z++) {
      if (update_func != NULL) {
	x = div(i_voxel.z,divider);
	if (x.rem == 0) 
	  continue_work = (*update_func)(update_data, NULL, (gdouble) i_voxel.z/(end.z-start.z));
      }

      for (i_voxel.y = start.y; i_voxel.y <=  end.y; i_voxel.y++)
	for (i_voxel.x = start.x; i_voxel.x <=  end.x; i_voxel.x++) {
	  VOXEL_TO_POINT(i_voxel, voxel_size, temp_point);
	  temp_point = amitk_space_s2s(AMITK_SPACE(rendering->volume),
				       AMITK_SPACE(rendering->object),temp_point);
	  switch(AMITK_ROI_TYPE(rendering->object)) {
	  case AMITK_ROI_TYPE_ISOCONTOUR_2D:
	  case AMITK_ROI_TYPE_ISOCONTOUR_3D:
	    POINT_TO_VOXEL(temp_point, AMITK_ROI(rendering->object)->voxel_size, 0, j_voxel);
	    if (amitk_raw_data_includes_voxel(AMITK_ROI(rendering->object)->isocontour, j_voxel)) {
	      temp_int = *AMITK_RAW_DATA_UBYTE_POINTER(AMITK_ROI(rendering->object)->isocontour, j_voxel);
	      if (temp_int == 2)
		temp_int = RENDERING_DENSITY_MAX;
	      else if (temp_int == 1)
		temp_int = RENDERING_DENSITY_MAX/2.0;
	    } else
	      temp_int = 0;
	    break;
	  case AMITK_ROI_TYPE_ELLIPSOID:
	    if (point_in_ellipsoid(temp_point, center, radius)) 
	      temp_int = RENDERING_DENSITY_MAX;
	    else temp_int = 0;
	    break;
	  case AMITK_ROI_TYPE_BOX:
	    if (point_in_box(temp_point, box_corner))
	      temp_int = RENDERING_DENSITY_MAX;
	    else temp_int = 0;
	    break;
	  case AMITK_ROI_TYPE_CYLINDER:
	    if (point_in_elliptic_cylinder(temp_point, center, height, radius)) 
	      temp_int = RENDERING_DENSITY_MAX;
	    else temp_int = 0;
	    break;
	  default:
	    temp_int=0;
	    g_assert(TRUE); /* assert if we ever get here */
	    break;
	  }

	  /* note, volpack needs a mirror reversal on the z axis */
	  density[i_voxel.x +
		  i_voxel.y*rendering->dim.x+
		  (rendering->dim.z-i_voxel.z-1)*rendering->dim.y*rendering->dim.x] = temp_int;
	}
    }



  } else { /* DATA SET */

    AmitkVolume * slice_volume;
    AmitkDataSet * slice;
    AmitkPoint new_offset;
    AmitkPoint temp_corner;
    amide_data_t temp_val, scale;
    amide_data_t max, min;

    
    slice_volume = AMITK_VOLUME(amitk_object_copy(AMITK_OBJECT(rendering->volume)));

    /* define the slice that we're trying to pull out of the data set */
    temp_corner = AMITK_VOLUME_CORNER(slice_volume);
    temp_corner.z = rendering->voxel_size;
    amitk_volume_set_corner(slice_volume, temp_corner);

    new_offset = zero_point; /* in slice_volume space */
    new_offset.z = rendering->voxel_size; 

    /* copy the info from a data set structure into our rendering_volume structure */
    j_voxel.t = j_voxel.z = i_voxel.t=0;
    for (i_voxel.z = 0; ((i_voxel.z < rendering->dim.z) && continue_work); i_voxel.z++) {
      if  (update_func != 0) {
	x = div(i_voxel.z,divider);
	if (x.rem == 0)
	    continue_work = (*update_func)(update_data, NULL, (gdouble) i_voxel.z/rendering->dim.z);
      }
      
      slice = amitk_data_set_get_slice(AMITK_DATA_SET(rendering->object), 
				       rendering->start, 
				       rendering->duration, 
				       rendering->voxel_size, 
				       slice_volume, FALSE);

      amitk_data_set_get_thresholding_max_min(AMITK_DATA_SET_SLICE_PARENT(slice),
					      AMITK_DATA_SET(slice),
					      rendering->start, 
					      rendering->duration, &max, &min);
      scale = ((amide_data_t) RENDERING_DENSITY_MAX) / (max-min);

      /* note, volpack needs a mirror reversal on the z axis */
      if (rendering->zero_fill) {
	for (j_voxel.y = i_voxel.y = 0; i_voxel.y <  rendering->dim.y; j_voxel.y++, i_voxel.y++)
	  for (j_voxel.x = i_voxel.x = 0; i_voxel.x <  rendering->dim.x; j_voxel.x++, i_voxel.x++) {
	    temp_val = scale * (AMITK_DATA_SET_DOUBLE_0D_SCALING_CONTENT(slice,j_voxel)-min);
	    if (temp_val > RENDERING_DENSITY_MAX) temp_val = 0.0;
	    if (temp_val < 0.0) temp_val = 0.0;
	    density[i_voxel.x+
		    i_voxel.y*rendering->dim.x+
		    (rendering->dim.z-i_voxel.z-1)* rendering->dim.y* rendering->dim.x] = temp_val;
	  }
      } else {
	for (j_voxel.y = i_voxel.y = 0; i_voxel.y <  rendering->dim.y; j_voxel.y++, i_voxel.y++)
	  for (j_voxel.x = i_voxel.x = 0; i_voxel.x <  rendering->dim.x; j_voxel.x++, i_voxel.x++) {
	    temp_val = scale * (AMITK_DATA_SET_DOUBLE_0D_SCALING_CONTENT(slice,j_voxel)-min);
	    if (temp_val > RENDERING_DENSITY_MAX) temp_val = RENDERING_DENSITY_MAX;
	    if (temp_val < 0.0) temp_val = 0.0;
	    density[i_voxel.x+
		    i_voxel.y*rendering->dim.x+
		    (rendering->dim.z-i_voxel.z-1)* rendering->dim.y* rendering->dim.x] = temp_val;
	  }
      }
      g_object_unref(slice);

      /* advance the requested slice volume for next iteration */
      amitk_space_set_offset(AMITK_SPACE(slice_volume), 
			     amitk_space_s2b(AMITK_SPACE(slice_volume), new_offset));
    }
    g_object_unref(slice_volume);
  }

  /* if we quit, get out of here */
  if (update_func != NULL) 
    continue_work = (*update_func)(update_data, NULL, (gdouble) 2.0); /* remove progress bar */

  if (!continue_work) {
    g_free(density);
    return FALSE;
  }

  /* compute surface normals (for shading) and gradient magnitudes (for classification) */
  if (vpVolumeNormals(rendering->vpc, density, density_size, RENDERING_DENSITY_FIELD, 
		      RENDERING_GRADIENT_FIELD, RENDERING_NORMAL_FIELD) != VP_OK) {
    g_warning("Error Computing the Rendering Normals (%s): %s",
	      rendering->name, vpGetErrorString(vpGetError(rendering->vpc)));
    g_free(density);
    return FALSE;
  }                   

  /* we're now done with the density volume, free it */
  g_free(density);

  /* we'll be using min-max octree's as the classifying functions will probably be changed a lot */
  /* octrees supposedly allow faster classification */
  if (rendering->optimize_rendering) { 
#if AMIDE_DEBUG
    g_print("\tCreating the Min/Max Octree\n");
#endif
    
    /* set the thresholds on the min-max octree */
    if (vpMinMaxOctreeThreshold(rendering->vpc, RENDERING_DENSITY_PARAM, 
				RENDERING_OCTREE_DENSITY_THRESH) != VP_OK) {
      g_warning("Error Setting Rendering Octree Threshold (%s, DENSITY): %s",
		rendering->name, vpGetErrorString(vpGetError(rendering->vpc)));
      return FALSE;
    }
    if (vpMinMaxOctreeThreshold(rendering->vpc, RENDERING_GRADIENT_PARAM, 
				RENDERING_OCTREE_GRADIENT_THRESH) != VP_OK) {
      g_warning("Error Setting Rendering Octree Threshold (%s, GRADIENT): %s",
		rendering->name, vpGetErrorString(vpGetError(rendering->vpc)));
      return FALSE;
    }

    /* create the min/max octree */
    if (vpCreateMinMaxOctree(rendering->vpc, 0, RENDERING_OCTREE_BASE_NODE_SIZE) != VP_OK) {
      g_warning("Error Generating Octree (%s): %s", rendering->name, 
		vpGetErrorString(vpGetError(rendering->vpc)));
      return FALSE;
    }

  }

  /* set the initial ambient property, as I don't like the volpack default */
  /*  if (vpSetMaterial(vpc[which], VP_MATERIAL0, VP_AMBIENT, VP_BOTH_SIDES, 0.0, 0.0, 0.0)  != VP_OK){
    g_warning("Error Setting the Material (%s, AMBIENT): %s",vol_name[which], vpGetErrorString(vpGetError(vpc[which])));
    return FALSE;
    }*/


  /*  if (vpSetMaterial(vpc[PET_VOLUME], VP_MATERIAL0, VP_DIFFUSE, VP_BOTH_SIDES, 0.35, 0.35, 0.35)  != VP_OK){
    g_warning("Error Setting the Material (PET_VOLUME, DIFFUSE): %s",vpGetErrorString(vpGetError(vpc[PET_VOLUME])));
    return FALSE;
    }*/

  /*  if (vpSetMaterial(vpc[PET_VOLUME], VP_MATERIAL0, VP_SPECULAR, VP_BOTH_SIDES, 0.39, 0.39, 0.39) != VP_OK){
      g_warning("Error Setting the Material (PET_VOLUME, SPECULAR): %s",vpGetErrorString(vpGetError(vpc[PET_VOLUME])));
    return FALSE;
  }  */


  /* set the initial shinyness, volpack's default is something shiny, I set shiny to zero */
  if (vpSetMaterial(rendering->vpc, VP_MATERIAL0, VP_SHINYNESS, VP_BOTH_SIDES,0.0,0.0,0.0) != VP_OK){
    g_warning("Error Setting the Rendering Material (%s, SHINYNESS): %s",
	      rendering->name, 
	      vpGetErrorString(vpGetError(rendering->vpc)));
    return FALSE;
  }

  /* set the shading parameters */
  if (vpSetLookupShader(rendering->vpc, 1, 1, RENDERING_NORMAL_FIELD, 
			rendering->shade_table, sizeof(rendering->shade_table), 
			0, NULL, 0) != VP_OK){
    g_warning("Error Setting the Rendering Shader (%s): %s",
	      rendering->name, 
	      vpGetErrorString(vpGetError(rendering->vpc)));
    return FALSE;
  }
  
  /* and do the shade table stuff (this fills in the shade table I believe) */
  if (vpShadeTable(rendering->vpc) != VP_OK){
    g_warning("Error Shading Table for Rendering (%s): %s",
	      rendering->name, 
	      vpGetErrorString(vpGetError(rendering->vpc)));
    return FALSE;
  }
  
  return TRUE;
}

static void classify_volume(rendering_t * rendering) {

  return;
}


static void set_space(rendering_t * rendering) {

  vpMatrix4 m; 
  guint i,j;
  AmitkPoint axis;

  /* initalize */
  for (i=0;i<4;i++)
    for (j=0;j<4;j++)
      if (i == j)
	m[i][j]=1;
      else
	m[i][j]=0;

  /* and setup the matrix we're feeding into volpack */
  for (i=0;i<3;i++) {
    axis = amitk_space_get_axis(AMITK_SPACE(rendering->volume), i);
    m[i][0] = axis.x;
    m[i][1] = axis.y;
    m[i][2] = axis.z;
  }

  /* we want to rotate the data set */
  if (vpCurrentMatrix(rendering->vpc, VP_MODEL) != VP_OK)
    g_warning("Error Setting The Item To Rotate (%s): %s",
	      rendering->name, vpGetErrorString(vpGetError(rendering->vpc)));

  /* set the rotation */
  if (vpSetMatrix(rendering->vpc, m) != VP_OK)
    g_warning("Error Rotating Rendering (%s): %s",
	      rendering->name, vpGetErrorString(vpGetError(rendering->vpc)));

}

/* set the rotation space for a rendering context */
void rendering_set_space(rendering_t * rendering, AmitkSpace * space) {

  rendering->need_rerender = TRUE;
  amitk_space_copy_in_place(AMITK_SPACE(rendering->volume), space);
  set_space(rendering);
  return;
}

/* sets the rotation transform for a rendering context */
void rendering_set_rotation(rendering_t * rendering, AmitkAxis dir, gdouble rotation) {

  rendering->need_rerender = TRUE;

  /* rotate the axis */
  amitk_space_rotate_on_vector(AMITK_SPACE(rendering->volume),
			       amitk_space_get_axis(AMITK_SPACE(rendering->volume), dir),
			       rotation, zero_point);
  set_space(rendering);
  return;
}

/* reset the rotation transform for a rendering context */
void rendering_reset_rotation(rendering_t * rendering) {

  AmitkSpace * new_space;
  
  rendering->need_rerender = TRUE;

  /* reset the coord frame */
  new_space = amitk_space_new(); /* base space */
  amitk_space_copy_in_place(AMITK_SPACE(rendering->volume), new_space);
  g_object_unref(new_space);

  /* reset the rotation */
  rendering_set_rotation(rendering, AMITK_AXIS_X, 0.0);

  return;
}


/* set the speed versus quality parameters of a rendering context */
void rendering_set_quality(rendering_t * rendering, rendering_quality_t quality) {

  gdouble max_ray_opacity, min_voxel_opacity;

  rendering->need_rerender = TRUE;

  /* set the rendering speed parameters MAX_RAY_OPACITY and MIN_VOXEL_OPACITY*/
  switch (quality) {
  case HIGH:
    max_ray_opacity = 0.99;
    min_voxel_opacity = 0.01;
    break;
    break;
  case FAST:
    max_ray_opacity = 0.95;
    min_voxel_opacity = 0.05;
    break;
  case FASTEST:
    max_ray_opacity = 0.9;
    min_voxel_opacity = 0.1;
    break;
  case HIGHEST:
  default:
    max_ray_opacity = 1.0;
    min_voxel_opacity = 0.0;
    break;
  }


  /* set the maximum ray opacity (the renderer quits follow a ray if this value is reached */
  if (vpSetd(rendering->vpc, VP_MAX_RAY_OPACITY, max_ray_opacity) != VP_OK){
    g_warning("Error Setting Rendering Max Ray Opacity (%s): %s",
	      rendering->name, vpGetErrorString(vpGetError(rendering->vpc)));
  }

  /* set the minimum voxel opacity (the render ignores voxels with values below this*/
  if (vpSetd(rendering->vpc, VP_MIN_VOXEL_OPACITY, min_voxel_opacity) != VP_OK) {
    g_warning("Error Setting the Min Voxel Opacity (%s): %s", 
	      rendering->name, vpGetErrorString(vpGetError(rendering->vpc)));
  }

  rendering->need_reclassify = TRUE; 

  return;
}

/* function to set up the image that we'll be getting back from the rendering */
void rendering_set_image(rendering_t * rendering, pixel_type_t pixel_type, gdouble zoom) {

  guint volpack_pixel_type;
  amide_intpoint_t size_dim; /* size in one dimension, note that height == width */

  rendering->need_rerender = TRUE;

  switch (pixel_type) {
  case GRAYSCALE:
    volpack_pixel_type = VP_LUMINANCE;
    break;
  case OPACITY:
  default:
    volpack_pixel_type = VP_ALPHA;
    break;
  }
  size_dim = ceil(zoom*POINT_MAX(rendering->dim));
  g_free(rendering->image);
  if ((rendering->image = g_try_new(guchar,size_dim*size_dim)) == NULL) {
    g_warning("Could not allocate space for Rendering Image for %s", 
	      rendering->name);
    return;
  }

  rendering->pixel_type = pixel_type;
  if (vpSetImage(rendering->vpc, (guchar *) rendering->image, size_dim,
		 size_dim, size_dim* RENDERING_DENSITY_SIZE, volpack_pixel_type)) {
    g_warning("Error Switching the Rendering Image Pixel Return Type (%s): %s",
	      rendering->name, vpGetErrorString(vpGetError(rendering->vpc)));
  }
  return;
}

/* switch the state of depth cueing (TRUE is on) */
void rendering_set_depth_cueing(rendering_t * rendering, gboolean state) {

  rendering->need_rerender = TRUE;

  if (vpEnable(rendering->vpc, VP_DEPTH_CUE, state) != VP_OK) {
      g_warning("Error Setting the Rendering Depth Cue (%s): %s",
		rendering->name, vpGetErrorString(vpGetError(rendering->vpc)));
  }

  return;

}


/* set the depth cueing parameters*/
void rendering_set_depth_cueing_parameters(rendering_t * rendering, 
					   gdouble front_factor, gdouble density) {

  rendering->need_rerender = TRUE;

  /* the defaults should be 1.0 and 1.0 */
  if (vpSetDepthCueing(rendering->vpc, front_factor, density) != VP_OK){
    g_warning("Error Enabling Rendering Depth Cueing (%s): %s",
	      rendering->name, vpGetErrorString(vpGetError(rendering->vpc)));
  }

  return;
}


/* to render a rendering context... */
void rendering_render(rendering_t * rendering)
{

  if (rendering->need_rerender) {
    if (rendering->vpc != NULL) {
      if (rendering->optimize_rendering) {
	if (rendering->need_reclassify) {
#if AMIDE_DEBUG
	  g_print("\tClassifying\n");
#endif
	  if (vpClassifyVolume(rendering->vpc) != VP_OK) {
	    g_warning("Error Classifying the Volume (%s): %s",
		      rendering->name,vpGetErrorString(vpGetError(rendering->vpc))); 
	    return; 
	  }
	}
	if (vpRenderClassifiedVolume(rendering->vpc) != VP_OK) {
	  g_warning("Error Rendering the Classified Volume (%s): %s", 
		    rendering->name, vpGetErrorString(vpGetError(rendering->vpc)));
	  return;
	}
      } else {
	if (vpRenderRawVolume(rendering->vpc) != VP_OK) {
	  g_warning("Error Rendering the Volume (%s): %s", 
		    rendering->name, vpGetErrorString(vpGetError(rendering->vpc)));
	  return;
	}
      }
    }
  }
  
  rendering->need_rerender = FALSE;
  rendering->need_reclassify = FALSE;

  return;
}





/* free up a list of rendering contexts */
renderings_t * renderings_unref(renderings_t * renderings) {
  
  if (renderings == NULL)
    return renderings;

  /* sanity check */
  g_return_val_if_fail(renderings->ref_count > 0, NULL);

  renderings->ref_count--;



  /* things to do if our ref count is zero */
  if (renderings->ref_count == 0) {
    /* recursively delete rest of list */
    renderings->next = renderings_unref(renderings->next); 

    renderings->rendering = rendering_unref(renderings->rendering);
    g_free(renderings);
    renderings = NULL;
  }

  return renderings;
}


/* the recursive part of renderings_init */
static renderings_t * renderings_init_recurse(GList * objects,
					      AmitkVolume * render_volume,
					      const amide_real_t voxel_size, 
					      const amide_time_t start, 
					      const amide_time_t duration,
					      const gboolean zero_fill,
					      const gboolean optimize_rendering,
					      gboolean (* update_func)(), 
					      gpointer update_data) {

  renderings_t * temp_renderings;
  rendering_t * new_rendering;
  renderings_t * rest_of_list;

  if (objects == NULL) return NULL;

  /* recurse first */
  rest_of_list = renderings_init_recurse(objects->next, render_volume, voxel_size, start, duration, 
					 zero_fill, optimize_rendering, update_func, update_data);

  new_rendering = rendering_init(objects->data, render_volume,voxel_size, start, duration, 
				 zero_fill, optimize_rendering, update_func, update_data);

  if (new_rendering != NULL) {
    if ((temp_renderings = g_try_new(renderings_t, 1)) == NULL) {
      return temp_renderings;
    }
    temp_renderings->ref_count = 1;
    temp_renderings->rendering = new_rendering; 
    temp_renderings->next = rest_of_list;
    return temp_renderings;
  } else {
    return rest_of_list;
  }
}
					      
					      

/* returns an initialized rendering list */
renderings_t * renderings_init(GList * objects,const amide_time_t start, const amide_time_t duration,
			       const gboolean zero_fill, const gboolean optimize_rendering, 
			       gboolean (* update_func)(), gpointer update_data) {
  
  AmitkCorners render_corner;
  amide_real_t voxel_size;
  AmitkVolume * render_volume;
  renderings_t * return_list;
  
  if (objects == NULL) return NULL;

  /* figure out all encompasing corners for the slices based on our viewing axis */
  render_volume = amitk_volume_new(); /* base coordinate frame */
  amitk_volumes_get_enclosing_corners(objects, AMITK_SPACE(render_volume), render_corner);
  amitk_space_set_offset(AMITK_SPACE(render_volume), render_corner[0]);
  amitk_volume_set_corner(render_volume, amitk_space_b2s(AMITK_SPACE(render_volume), render_corner[1]));

  /* figure out what size our rendering voxels will be */
  voxel_size = amitk_data_sets_get_min_voxel_size(objects);
  if (voxel_size < 0.0) 
    voxel_size = amitk_rois_get_max_min_voxel_size(objects);
  if (voxel_size < 0.0)
    voxel_size = 1.0;

  /* and generate our rendering list */
  return_list = renderings_init_recurse(objects, render_volume,voxel_size, start, duration, 
					zero_fill, optimize_rendering, update_func, update_data);
  g_object_unref(render_volume);
  return return_list;
}


/* reloads the rendering list when needed, returns FALSE if not everything was done correctly */
gboolean renderings_reload_objects(renderings_t * renderings, 
				   const amide_time_t start, 
				   const amide_time_t duration,
				   gboolean (* update_func)(), 
				   gpointer update_data) {

  gboolean correct=TRUE;

  if (renderings != NULL) {
    /* reload this rendering context */
    correct = rendering_reload_object(renderings->rendering, start, duration, update_func, update_data);
    /* and do the next */
    correct = 
      correct && 
      renderings_reload_objects(renderings->next, start, duration, update_func, update_data);
  }

  return correct;
}
    


/* sets the space for a list of rendering context */
void renderings_set_space(renderings_t * renderings, AmitkSpace * space) {


  while (renderings != NULL) {
    rendering_set_space(renderings->rendering, space);
    renderings = renderings->next;
  }

  return;
}

/* sets the rotation transform for a list of rendering context */
void renderings_set_rotation(renderings_t * renderings, AmitkAxis dir, gdouble rotation) {


  while (renderings != NULL) {
    rendering_set_rotation(renderings->rendering, dir, rotation);
    renderings = renderings->next;
  }

  return;
}

/* resets the rotation transform for a list of rendering context */
void renderings_reset_rotation(renderings_t * renderings) {


  while (renderings != NULL) {
    rendering_reset_rotation(renderings->rendering);
    renderings = renderings->next;
  }

  return;
}

/* to set the quality on a list of rendering contexts */
void renderings_set_quality(renderings_t * renderings, rendering_quality_t quality) {

  while (renderings != NULL) {
    rendering_set_quality(renderings->rendering, quality);
    renderings = renderings->next;
  }

  return;
}

/* set the return image parameters  for a list of rendering contexts */
void renderings_set_zoom(renderings_t * renderings, gdouble zoom) {

  while (renderings != NULL) {
    rendering_set_image(renderings->rendering, 
			renderings->rendering->pixel_type, 
			zoom);
    renderings = renderings->next;
  }

  return;
}

/* turn depth cueing on/off for a list of rendering contexts */
void renderings_set_depth_cueing(renderings_t * renderings, gboolean state) {

  while (renderings != NULL) {
    rendering_set_depth_cueing(renderings->rendering, state);
    renderings = renderings->next;
  }

  return;
}


/* sets the depth cueing parameters for a list of rendering contexts */
void renderings_set_depth_cueing_parameters(renderings_t * renderings, 
					    gdouble front_factor, gdouble density) {

  while (renderings != NULL) {
    rendering_set_depth_cueing_parameters(renderings->rendering, 
					  front_factor, density);
    renderings = renderings->next;
  }

  return;
}


/* to render a list of rendering contexts... */
void renderings_render(renderings_t * renderings)
{

  while (renderings != NULL) {
    rendering_render(renderings->rendering);
    renderings = renderings->next;
  }

  return;
}

/* count the rendering lists */
guint renderings_count(renderings_t * renderings) {
  if (renderings != NULL)
    return (1+renderings_count(renderings->next));
  else
    return 0;
}




  /* function to set a material parameter */
/*  gboolean render_set_material_shinyness(gint material_num, gdouble shinyness) */
/*  { */
/*    gint i; */

/*    for (i=0; i<NUM_VOLUMES; i++) { */

      /* reset the shinyness of the material */ 
/*      if (vpSetMaterial(vpc[i], material_num, VP_SHINYNESS, VP_BOTH_SIDES,shinyness,0.0,0.0) != VP_OK){ */
/*        g_warning("Error ReSetting the Material (%s, SHINYNESS): %s",vol_name[i],vpGetErrorString(vpGetError(vpc[i]))); */
/*        return FALSE; */
/*      } */
    
      /* reset the shading parameters */ 
/*      if (vpSetLookupShader(vpc[i], 1, 1, NORMAL_FIELD, shade_table, sizeof(shade_table), 0, NULL, 0) != VP_OK){ */
/*        g_warning("Error ReSetting the Shader (%s): %s",vol_name[i], vpGetErrorString(vpGetError(vpc[i]))); */
/*      return FALSE; */
/*      } */
    
      /* and redo the shade table stuff (this fills in the shade table I believe) */
/*      if (vpShadeTable(vpc[i]) != VP_OK){ */
/*        g_warning("Error ReShading Table (%s): %s",vol_name[i], vpGetErrorString(vpGetError(vpc[i]))); */
/*        return FALSE; */
/*      } */
/*    } */

/*    return TRUE; */
/*  }   */



/* --------------------------------------------------------------------------------*/
/* this is stuff that got ripped out of render_load_volume at some point and 
 * at some point in the future will be put into their own callbacks */
  
  /*  if (vpSetMaterial(vpc[PET_VOLUME], VP_MATERIAL0, VP_AMBIENT, VP_BOTH_SIDES, 0.18, 0.18, 0.18)  != VP_OK){
    g_warning("Error Setting the Material (PET_VOLUME, AMBIENT): %s",vpGetErrorString(vpGetError(vpc[PET_VOLUME])));
    return FALSE;
    }*/


  /*  if (vpSetMaterial(vpc[PET_VOLUME], VP_MATERIAL0, VP_DIFFUSE, VP_BOTH_SIDES, 0.35, 0.35, 0.35)  != VP_OK){
    g_warning("Error Setting the Material (PET_VOLUME, DIFFUSE): %s",vpGetErrorString(vpGetError(vpc[PET_VOLUME])));
    return FALSE;
    }*/

  /*  if (vpSetMaterial(vpc[PET_VOLUME], VP_MATERIAL0, VP_SPECULAR, VP_BOTH_SIDES, 0.39, 0.39, 0.39) != VP_OK){
    g_warning("Error Setting the Material (PET_VOLUME, SPECULAR): %s",vpGetErrorString(vpGetError(vpc[PET_VOLUME])));
    return FALSE;
  }  */

  /*  if (vpSetMaterial(vpc[PET_VOLUME], VP_MATERIAL0, VP_SHINYNESS, VP_BOTH_SIDES,10.0,0.0,0.0) != VP_OK){
    g_warning("Error Setting the Material (PET_VOLUME, SHINYNESS): %s",vpGetErrorString(vpGetError(vpc[PET_VOLUME])));
    return FALSE;
    }*/

  /*  if (vpSetLight(vpc[PET_VOLUME], VP_LIGHT0, VP_DIRECTION, 0.3, 0.3, 1.0) != VP_OK){
    g_warning("Error Setting the Light (PET_VOLUME, DIRECTION): %s",vpGetErrorString(vpGetError(vpc[PET_VOLUME])));
    return FALSE;
    }*/
  
  /*if (vpSetLight(vpc[PET_VOLUME], VP_LIGHT0, VP_COLOR, 1.0, 1.0, 1.0) != VP_OK){
    g_warning("Error Setting the Light (PET_VOLUME, COLOR): %s",vpGetErrorString(vpGetError(vpc[PET_VOLUME])));
    return FALSE;
  }  */

  /*  if (vpEnable(vpc[PET_VOLUME], VP_LIGHT0, 1) != VP_OK){
    g_warning("Error Enabling Lighting (PET_VOLUME): %s",vpGetErrorString(vpGetError(vpc[PET_VOLUME])));
    return FALSE;
  }  */


  /* set the initial viewing parameters */

  /*  if (vpSeti(vpc[PET_VOLUME], VP_CONCAT_MODE, VP_CONCAT_LEFT) != VP_OK){
    g_warning("Error Enabling Concat (PET_VOLUME): %s",vpGetErrorString(vpGetError(vpc[PET_VOLUME])));
    return FALSE;
  }  */


  /*  if (vpCurrentMatrix(vpc[PET_VOLUME], VP_PROJECT) != VP_OK){
    g_warning("Error Setting Projection (PET_VOLUME): %s",vpGetErrorString(vpGetError(vpc[PET_VOLUME])));
    return FALSE;
    }*/

  
  /* Load an identity matrix into the vpc, return is always VP_OK */
  /*  vpIdentityMatrix(vpc[PET_VOLUME]); */

  
  /*  if (vpWindow(vpc[PET_VOLUME], VP_PARALLEL, -0.5, 0.5, -0.5, 0.5, -0.5, 0.5) != VP_OK){
    g_warning("Error Setting the Window (PET_VOLUME): %s",vpGetErrorString(vpGetError(vpc[PET_VOLUME])));
    return FALSE;
  } */

  /*  if (vpCurrentMatrix(vpc[PET_VOLUME], VP_MODEL) != VP_OK){
    g_warning("Error Setting the Model (PET_VOLUME): %s",vpGetErrorString(vpGetError(vpc[PET_VOLUME])));
    return FALSE;
  }  */

  



#endif



