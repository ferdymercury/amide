/* rendering.c
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

#ifdef AMIDE_LIBVOLPACK_SUPPORT

#include <glib.h>
#include <stdlib.h>
#include "rendering.h"
#include "amitk_roi.h"
#include "amitk_data_set_DOUBLE_0D_SCALING.h"


/* external variables */
gchar * rendering_quality_names[] = {"Highest Quality and Slowest",
				     "High Quality and Medium Speed",
				     "Medium Quality and Fast",
				     "Low Quality and Fastest"};
gchar * pixel_type_names[] = {"Opacity",
			      "Grayscale"};
			      

rendering_t * rendering_context_unref(rendering_t * context) {
  
  classification_t i_class;

  if (context== NULL)
    return context;

  /* sanity checks */
  g_return_val_if_fail(context->ref_count > 0, NULL);

  context->ref_count--; 

#ifdef AMIDE_DEBUG
  if (context->ref_count == 0)
    g_print("freeing rendering context of %s\n",context->name);
#endif

  /* if we've removed all reference's, free the context */
  if (context->ref_count == 0) {
    if (context->object != NULL) {
      g_object_unref(context->object);
      context->object = NULL;
    }

    g_free(context->rendering_data);
    g_free(context->name);
    if (context->vpc != NULL)
      vpDestroyContext(context->vpc);
    g_free(context->image);
    for (i_class = 0; i_class < NUM_CLASSIFICATIONS; i_class++) {
      g_free(context->ramp_x[i_class]);
      g_free(context->ramp_y[i_class]);
    }
    g_object_unref(context->volume);
    g_free(context);
    context = NULL;
  }

  return context;

}



/* either volume or object must be NULL */
rendering_t * rendering_context_init(const AmitkObject * object,
				     AmitkVolume * rendering_volume,
				     const amide_real_t voxel_size, 
				     const amide_time_t start, 
				     const amide_time_t duration, 
				     const AmitkInterpolation interpolation) {

  rendering_t * new_context = NULL;
  gint i;
  gint init_density_ramp_x[] = RENDERING_DENSITY_RAMP_X;
  gfloat init_density_ramp_y[] = RENDERING_DENSITY_RAMP_Y;
  gint init_gradient_ramp_x[] = RENDERING_GRADIENT_RAMP_X;
  gfloat init_gradient_ramp_y[] = RENDERING_GRADIENT_RAMP_Y;

  g_return_val_if_fail((AMITK_IS_DATA_SET(object) || AMITK_IS_ROI(object)), NULL);

  if ((new_context =  g_new(rendering_t,1)) == NULL) {
    g_warning("couldn't allocate space for rendering context");
    return NULL;
  }
  new_context->ref_count = 1;
  new_context->need_rerender = TRUE;
  
  /* start initializing what we can */
  new_context->vpc = vpCreateContext();
  new_context->object = amitk_object_copy(object);
  new_context->name = g_strdup(AMITK_OBJECT_NAME(object));
  if (AMITK_IS_DATA_SET(object))
    new_context->color_table = AMITK_DATA_SET_COLOR_TABLE(object);
  else 
    new_context->color_table = AMITK_COLOR_TABLE_BW_LINEAR;
  new_context->pixel_type = RENDERING_DEFAULT_PIXEL_TYPE;

  new_context->image = NULL;
  new_context->rendering_data = NULL;
  new_context->curve_type[DENSITY_CLASSIFICATION] = CURVE_LINEAR;
  new_context->curve_type[GRADIENT_CLASSIFICATION] = CURVE_LINEAR;
  new_context->volume = AMITK_VOLUME(amitk_object_copy(AMITK_OBJECT(rendering_volume)));
  new_context->start = start;
  new_context->duration = duration;

  /* figure out the size of our context */
  new_context->dim.x = ceil(AMITK_VOLUME_X_CORNER(rendering_volume)/voxel_size);
  new_context->dim.y = ceil(AMITK_VOLUME_Y_CORNER(rendering_volume)/voxel_size);
  new_context->dim.z = ceil(AMITK_VOLUME_Z_CORNER(rendering_volume)/voxel_size);
  new_context->voxel_size = voxel_size;

  /* adjust the thresholding if needed */
  if (AMITK_IS_DATA_SET(new_context->object))
    if (AMITK_DATA_SET_THRESHOLDING(new_context->object) == AMITK_THRESHOLDING_PER_SLICE) {
      amitk_data_set_set_thresholding(AMITK_DATA_SET(new_context->object), AMITK_THRESHOLDING_GLOBAL);
      g_warning("\"Per Slice\" thresholding illogical for conversion to volume rendering, using \"Global\" thresholding ");
    }
      


#if AMIDE_DEBUG
  g_print("\tSetting up a Rendering Context\n");
#endif

  /* setup the density and gradient ramps */
  new_context->num_points[DENSITY_CLASSIFICATION] = RENDERING_DENSITY_RAMP_POINTS;
  if ((new_context->ramp_x[DENSITY_CLASSIFICATION] = 
       g_new(gint, new_context->num_points[DENSITY_CLASSIFICATION])) == NULL) {
    g_warning("couldn't allocate space for density ramp x");
    return NULL;
  }
  if ((new_context->ramp_y[DENSITY_CLASSIFICATION] = 
       g_new(gfloat,new_context->num_points[DENSITY_CLASSIFICATION])) == NULL) {
    g_warning("couldn't allocate space for density ramp y");
    return NULL;
  }
  for (i=0;i<new_context->num_points[DENSITY_CLASSIFICATION];i++) {
    new_context->ramp_x[DENSITY_CLASSIFICATION][i] = init_density_ramp_x[i];
    new_context->ramp_y[DENSITY_CLASSIFICATION][i] = init_density_ramp_y[i];
  }

  new_context->num_points[GRADIENT_CLASSIFICATION] = RENDERING_GRADIENT_RAMP_POINTS;
  if ((new_context->ramp_x[GRADIENT_CLASSIFICATION] = 
       g_new(gint,new_context->num_points[GRADIENT_CLASSIFICATION])) == NULL) {
    g_warning("couldn't allocate space for gradient ramp x");
    return NULL;
  }
  if ((new_context->ramp_y[GRADIENT_CLASSIFICATION] = 
       g_new(gfloat,new_context->num_points[GRADIENT_CLASSIFICATION])) == NULL) {
    g_warning("couldn't allocate space for gradient ramp y");
    return NULL;
  }
  for (i=0;i<new_context->num_points[GRADIENT_CLASSIFICATION];i++) {
    new_context->ramp_x[GRADIENT_CLASSIFICATION][i] = init_gradient_ramp_x[i];
    new_context->ramp_y[GRADIENT_CLASSIFICATION][i] = init_gradient_ramp_y[i];
  }




  /* setup the context's classification function */
  if (vpRamp(new_context->density_ramp, sizeof(gfloat), 
	     new_context->num_points[DENSITY_CLASSIFICATION],
	     new_context->ramp_x[DENSITY_CLASSIFICATION], 
	     new_context->ramp_y[DENSITY_CLASSIFICATION]) != VP_OK){
    g_warning("Error Setting the Rendering Density Ramp");
    new_context = rendering_context_unref(new_context);
    return new_context;
  }

  /* setup the context's gradient classification */
  if (vpRamp(new_context->gradient_ramp, sizeof(gfloat), 
	     new_context->num_points[GRADIENT_CLASSIFICATION],
	     new_context->ramp_x[GRADIENT_CLASSIFICATION], 
	     new_context->ramp_y[GRADIENT_CLASSIFICATION]) != VP_OK) {
    g_warning("Error Setting the Rendering Gradient Ramp");
    new_context = rendering_context_unref(new_context);
    return new_context;
  }

  /* tell the rendering context info on the voxel structure */
  if (vpSetVoxelSize(new_context->vpc,  RENDERING_BYTES_PER_VOXEL, RENDERING_VOXEL_FIELDS, 
		     RENDERING_SHADE_FIELDS, RENDERING_CLSFY_FIELDS) != VP_OK) {
    g_warning("Error Setting the Rendering Voxel Size (%s): %s", 
	      new_context->name, vpGetErrorString(vpGetError(new_context->vpc)));
    new_context = rendering_context_unref(new_context);
    return new_context;
  }

  /* now tell the rendering context the location of each field in voxel, 
     do this for each field in the context */
  if (vpSetVoxelField (new_context->vpc,RENDERING_NORMAL_FIELD, RENDERING_NORMAL_SIZE, 
		       RENDERING_NORMAL_OFFSET, RENDERING_NORMAL_MAX) != VP_OK) {
    g_warning("Error Specifying the Rendering Voxel Fields (%s, NORMAL): %s", 
	      new_context->name, vpGetErrorString(vpGetError(new_context->vpc)));
    new_context = rendering_context_unref(new_context);
    return new_context;
  }
  if (vpSetVoxelField (new_context->vpc,RENDERING_DENSITY_FIELD, RENDERING_DENSITY_SIZE, 
		       RENDERING_DENSITY_OFFSET, RENDERING_DENSITY_MAX) != VP_OK) {
    g_warning("Error Specifying the Rendering Voxel Fields (%s, DENSITY): %s", 
	      new_context->name, vpGetErrorString(vpGetError(new_context->vpc)));
    new_context = rendering_context_unref(new_context);
    return new_context;
  }
  if (vpSetVoxelField (new_context->vpc,RENDERING_GRADIENT_FIELD, RENDERING_GRADIENT_SIZE, 
		       RENDERING_GRADIENT_OFFSET, RENDERING_GRADIENT_MAX) != VP_OK) {
    g_warning("Error Specifying the Rendering Voxel Fields (%s, GRADIENT): %s",
	      new_context->name, vpGetErrorString(vpGetError(new_context->vpc)));
    new_context = rendering_context_unref(new_context);
    return new_context;
  }

  /* apply density classification to the vpc */
  if (vpSetClassifierTable(new_context->vpc, RENDERING_DENSITY_PARAM, RENDERING_DENSITY_FIELD, 
			   new_context->density_ramp,
			   sizeof(new_context->density_ramp)) != VP_OK){
    g_warning("Error Setting the Rendering Classifier Table (%s, DENSITY): %s",
	      new_context->name, vpGetErrorString(vpGetError(new_context->vpc)));
    new_context = rendering_context_unref(new_context);
    return new_context;
  }

  /* apply it to the different vpc's */
  if (vpSetClassifierTable(new_context->vpc, RENDERING_GRADIENT_PARAM, RENDERING_GRADIENT_FIELD, 
			   new_context->gradient_ramp,
			   sizeof(new_context->gradient_ramp)) != VP_OK){
    g_warning("Error Setting the Classifier Table (%s, GRADIENT): %s",
	      new_context->name,  vpGetErrorString(vpGetError(new_context->vpc)));
    new_context = rendering_context_unref(new_context);
    return new_context;
  }

  /* now copy the object data into the rendering context */
  rendering_context_load_object(new_context, interpolation);

  /* and finally, set up the structure into which the rendering will be returned */
  rendering_context_set_image(new_context, new_context->pixel_type, RENDERING_DEFAULT_ZOOM);

  return new_context;
}


/* function to reload the context's concept of an object when necessary */
void rendering_context_reload_objects(rendering_t * context, 
				      const amide_time_t new_start,
				      const amide_time_t new_duration, 
				      const AmitkInterpolation interpolation) {
  
  amide_time_t old_start, old_duration;


  old_start = context->start;
  old_duration = context->duration;

  context->start = new_start;
  context->duration = new_duration;

  /* only need to do stuff for dynamic data sets */
  if (!AMITK_IS_DATA_SET(context->object))
    return;
  if (!AMITK_DATA_SET_DYNAMIC(context->object))
    return;

  if (amitk_data_set_get_frame(AMITK_DATA_SET(context->object), new_start) == 
      amitk_data_set_get_frame(AMITK_DATA_SET(context->object), old_start))
    if (amitk_data_set_get_frame(AMITK_DATA_SET(context->object), new_start+new_duration) ==
	amitk_data_set_get_frame(AMITK_DATA_SET(context->object), old_start+old_duration))
      return;

  /* allright, reload the context's data */
  context->need_rerender = TRUE;
  rendering_context_load_object(context, interpolation);

}



/* function to update the rendering structure's concept of the object */
void rendering_context_load_object(rendering_t * context, 
				   const AmitkInterpolation interpolation) {

  AmitkVoxel i_voxel, j_voxel;
  rendering_density_t * density; /* buffer for density data */
  guint density_size;/* size of density data */
  guint context_size;/* size of context */
#if AMIDE_DEBUG
  div_t x;
  gint divider;
#endif

  /* tell the context the dimensions of our rendering context */
  if (vpSetVolumeSize(context->vpc, context->dim.x, 
		      context->dim.y, context->dim.z) != VP_OK) {
    g_warning("Error Setting the Context Size (%s): %s", 
	      context->name, 
	      vpGetErrorString(vpGetError(context->vpc)));
    return;
  }

  /* allocate space for the raw data and the context */
  density_size =  context->dim.x *  context->dim.y *  
    context->dim.z * RENDERING_DENSITY_SIZE;
  context_size =  context->dim.x *  context->dim.y * 
     context->dim.z * RENDERING_BYTES_PER_VOXEL;

  if ((density = (rendering_density_t * ) g_malloc(density_size)) == NULL) {
    g_warning("Could not allocate space for density data for %s", 
	      context->name);
    return;
  }


  g_free(context->rendering_data);
  if ((context->rendering_data = g_new0(rendering_voxel_t, context_size)) == NULL) {
    g_warning("Could not allocate space for rendering context volume for %s", 
	      context->name);
    g_free(density);
    return;
  }

  vpSetRawVoxels(context->vpc, context->rendering_data, context_size, 
		 RENDERING_BYTES_PER_VOXEL,  context->dim.x * RENDERING_BYTES_PER_VOXEL,
		 context->dim.x* context->dim.y * RENDERING_BYTES_PER_VOXEL);

#if AMIDE_DEBUG
  g_print("\tCopying Data into Rendering Context Volume\t");
  divider = ((context->dim.z/20.0) < 1) ? 1 : (context->dim.z/20.0);
#endif


  if (AMITK_IS_ROI(context->object)) {

    AmitkPoint center, radius;
    amide_real_t height;
    AmitkPoint temp_point;
    AmitkPoint box_corner;
    gint temp_int;
    AmitkVoxel start, end;
    AmitkCorners intersection_corners;
    AmitkPoint voxel_size;

    voxel_size.x = voxel_size.y = voxel_size.z = context->voxel_size;
    
    radius = point_cmult(0.5, AMITK_VOLUME_CORNER(context->object));
    center = amitk_space_b2s(AMITK_SPACE(context->object),
			     amitk_volume_center(AMITK_VOLUME(context->object)));
    height = AMITK_VOLUME_Z_CORNER(context->object);
    box_corner = AMITK_VOLUME_CORNER(context->object);
    j_voxel.t = j_voxel.z=i_voxel.t=0;

    /* figure out the intersection between the rendering volume and the roi */
    if (!amitk_volume_volume_intersection_corners(context->volume, 
						  AMITK_VOLUME(context->object), 
						  intersection_corners)) {
      end = zero_voxel;
    } else {
      POINT_TO_VOXEL(intersection_corners[0], voxel_size, 0, start);
      POINT_TO_VOXEL(intersection_corners[1], voxel_size, 0, end);
    }
    
    g_return_if_fail(end.x < context->dim.x);
    g_return_if_fail(end.y < context->dim.y);
    g_return_if_fail(end.z < context->dim.z);

    for (i_voxel.z = start.z; i_voxel.z <= end.z; i_voxel.z++) {
#ifdef AMIDE_DEBUG
      x = div(i_voxel.z,divider);
      if (x.rem == 0) g_print(".");
#endif 
      for (i_voxel.y = start.y; i_voxel.y <=  end.y; i_voxel.y++)
	for (i_voxel.x = start.x; i_voxel.x <=  end.x; i_voxel.x++) {
	  VOXEL_TO_POINT(i_voxel, voxel_size, temp_point);
	  temp_point = amitk_space_s2s(AMITK_SPACE(context->volume),
				       AMITK_SPACE(context->object),temp_point);
	  switch(AMITK_ROI_TYPE(context->object)) {
	  case AMITK_ROI_TYPE_ISOCONTOUR_2D:
	  case AMITK_ROI_TYPE_ISOCONTOUR_3D:
	    POINT_TO_VOXEL(temp_point, AMITK_ROI(context->object)->voxel_size, 0, j_voxel);
	    if (amitk_raw_data_includes_voxel(AMITK_ROI(context->object)->isocontour, j_voxel)) {
	      temp_int = *AMITK_RAW_DATA_UBYTE_POINTER(AMITK_ROI(context->object)->isocontour, j_voxel);
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
	      g_warning("unexpected case in %s at line %d",__FILE__, __LINE__);
	      g_free(density);
	      return;
	      break;
	  }

	  /* note, volpack needs a mirror reversal on the z and x axis */
	  density[(context->dim.x-i_voxel.x-1)+
		  i_voxel.y*context->dim.x+
		  (context->dim.z-i_voxel.z-1)*context->dim.y*context->dim.x] = temp_int;
	}
    }



  } else { /* DATA SET */

    AmitkVolume * slice_volume;
    AmitkDataSet * slice;
    AmitkPoint new_offset;
    AmitkPoint temp_corner;
    amide_data_t temp_val, scale;
    amide_data_t max, min;

    
    slice_volume = AMITK_VOLUME(amitk_object_copy(AMITK_OBJECT(context->volume)));

    /* define the slice that we're trying to pull out of the data set */
    temp_corner = AMITK_VOLUME_CORNER(slice_volume);
    temp_corner.z = context->voxel_size;
    amitk_volume_set_corner(slice_volume, temp_corner);

    new_offset = zero_point; /* in slice_volume space */
    new_offset.z = context->voxel_size; 

    /* copy the info from a data set structure into our rendering_volume structure */
    j_voxel.t = j_voxel.z = i_voxel.t=0;
    for (i_voxel.z = 0; i_voxel.z < context->dim.z; i_voxel.z++) {
#ifdef AMIDE_DEBUG
      x = div(i_voxel.z,divider);
      if (x.rem == 0) g_print(".");
#endif 
      
      slice = amitk_data_set_get_slice(AMITK_DATA_SET(context->object), 
				       context->start, 
				       context->duration, 
				       context->voxel_size, 
				       slice_volume, interpolation, FALSE);

      amitk_data_set_get_thresholding_max_min(AMITK_DATA_SET_SLICE_PARENT(slice),
					      AMITK_DATA_SET(slice),
					      context->start, 
					      context->duration, &max, &min);
      scale = ((amide_data_t) RENDERING_DENSITY_MAX) / (max-min);

      /* note, volpack needs a mirror reversal on the z and x axis */
      for (j_voxel.y = i_voxel.y = 0; i_voxel.y <  context->dim.y; j_voxel.y++, i_voxel.y++)
	for (j_voxel.x = i_voxel.x = 0; i_voxel.x <  context->dim.x; j_voxel.x++, i_voxel.x++) {
	  temp_val = scale * (AMITK_DATA_SET_DOUBLE_0D_SCALING_CONTENT(slice,j_voxel)-min);
	  if (temp_val > RENDERING_DENSITY_MAX) temp_val = RENDERING_DENSITY_MAX;
	  if (temp_val < 0.0) temp_val = 0.0;
	  density[(context->dim.x-i_voxel.x-1)+
		  i_voxel.y*context->dim.x+
		  (context->dim.z-i_voxel.z-1)* context->dim.y* context->dim.x] = temp_val;
	}
      g_object_unref(slice);

      /* advance the requested slice volume for next iteration */
      amitk_space_set_offset(AMITK_SPACE(slice_volume), 
			     amitk_space_s2b(AMITK_SPACE(slice_volume), new_offset));
    }
    g_object_unref(slice_volume);
  }

#ifdef AMIDE_DEBUG
  g_print("\n");
#endif

  /* compute surface normals (for shading) and gradient magnitudes (for classification) */
  if (vpVolumeNormals(context->vpc, density, density_size, RENDERING_DENSITY_FIELD, 
		      RENDERING_GRADIENT_FIELD, RENDERING_NORMAL_FIELD) != VP_OK) {
    g_warning("Error Computing the Rendering Normals (%s): %s",
	      context->name, vpGetErrorString(vpGetError(context->vpc)));
    g_free(density);
    return;
  }                   

  /* we're now done with the density volume, free it */
  g_free(density);

  /* we'll be using min-max octree's as the classifying functions will probably be changed a lot
   * also, this octrees should be fast enough for this computer to get acceptable rendering speeds */
  /* classify the volume */
  /*  if (vpClassifyVolume(vpc[PET_VOLUME]) != VP_OK) {
    g_warning("Error Classifying the Volume: %s",vpGetErrorString(vpGetError(vpc[PET_VOLUME]))); 
    return FALSE; 
  } 
  */

  /* set the thresholds on the min-max octree */
  if (vpMinMaxOctreeThreshold(context->vpc, RENDERING_DENSITY_PARAM, 
			      RENDERING_OCTREE_DENSITY_THRESH) != VP_OK) {
    g_warning("Error Setting Rendering Octree Threshold (%s, DENSITY): %s",
	      context->name, vpGetErrorString(vpGetError(context->vpc)));
    return;
  }
  if (vpMinMaxOctreeThreshold(context->vpc, RENDERING_GRADIENT_PARAM, 
			      RENDERING_OCTREE_GRADIENT_THRESH) != VP_OK) {
    g_warning("Error Setting Rendering Octree Threshold (%s, GRADIENT): %s",
	      context->name, vpGetErrorString(vpGetError(context->vpc)));
    return;
  }
  
#if AMIDE_DEBUG
  g_print("\tCreating the Min/Max Octree\n");
#endif
  
  /* create the min/max octree */
  if (vpCreateMinMaxOctree(context->vpc, 0, RENDERING_OCTREE_BASE_NODE_SIZE) != VP_OK) {
    g_warning("Error Generating Octree (%s): %s", context->name, 
	      vpGetErrorString(vpGetError(context->vpc)));
    return;
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
  if (vpSetMaterial(context->vpc, VP_MATERIAL0, VP_SHINYNESS, VP_BOTH_SIDES,0.0,0.0,0.0) != VP_OK){
    g_warning("Error Setting the Rendering Material (%s, SHINYNESS): %s",
	      context->name, 
	      vpGetErrorString(vpGetError(context->vpc)));
    return;
  }

  /* set the shading parameters */
  if (vpSetLookupShader(context->vpc, 1, 1, RENDERING_NORMAL_FIELD, 
			context->shade_table, sizeof(context->shade_table), 
			0, NULL, 0) != VP_OK){
    g_warning("Error Setting the Rendering Shader (%s): %s",
	      context->name, 
	      vpGetErrorString(vpGetError(context->vpc)));
    return;
  }
  
  /* and do the shade table stuff (this fills in the shade table I believe) */
  if (vpShadeTable(context->vpc) != VP_OK){
    g_warning("Error Shading Table for Rendering (%s): %s",
	      context->name, 
	      vpGetErrorString(vpGetError(context->vpc)));
    return;
  }
  
  return;
}




/* sets the rotation transform for a rendering context */
void rendering_context_set_rotation(rendering_t * context, AmitkAxis dir, gdouble rotation) {

  vpMatrix4 m; 
  guint i,j;
  AmitkPoint axis;

  context->need_rerender = TRUE;

  /* rotate the axis */
  amitk_space_rotate_on_vector(AMITK_SPACE(context->volume),
			       amitk_space_get_axis(AMITK_SPACE(context->volume), dir),
			       rotation, zero_point);

  /* initalize */
  for (i=0;i<4;i++)
    for (j=0;j<4;j++)
      if (i == j)
	m[i][j]=1;
      else
	m[i][j]=0;

  /* and setup the matrix we're feeding into volpack */
  for (i=0;i<3;i++) {
    axis = amitk_space_get_axis(AMITK_SPACE(context->volume), i);
    m[i][0] = axis.x;
    m[i][1] = axis.y;
    m[i][2] = axis.z;
  }

  /* we want to rotate the data set */
  if (vpCurrentMatrix(context->vpc, VP_MODEL) != VP_OK)
    g_warning("Error Setting The Item To Rotate (%s): %s",
	      context->name, vpGetErrorString(vpGetError(context->vpc)));

  /* set the rotation */
  if (vpSetMatrix(context->vpc, m) != VP_OK)
    g_warning("Error Rotating Rendering (%s): %s",
	      context->name, vpGetErrorString(vpGetError(context->vpc)));

  return;
}

/* reset the rotation transform for a rendering context */
void rendering_context_reset_rotation(rendering_t * context) {

  AmitkSpace * new_space;
  
  context->need_rerender = TRUE;

  /* reset the coord frame */
  new_space = amitk_space_new(); /* base space */
  amitk_space_copy_in_place(AMITK_SPACE(context->volume), new_space);
  g_object_unref(new_space);

  /* reset the rotation */
  rendering_context_set_rotation(context, AMITK_AXIS_X, 0.0);

  return;
}


/* set the speed versus quality parameters of a context */
void rendering_context_set_quality(rendering_t * context, rendering_quality_t quality) {

  gdouble max_ray_opacity, min_voxel_opacity;

  context->need_rerender = TRUE;

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
  if (vpSetd(context->vpc, VP_MAX_RAY_OPACITY, max_ray_opacity) != VP_OK){
    g_warning("Error Setting Rendering Max Ray Opacity (%s): %s",
	      context->name, vpGetErrorString(vpGetError(context->vpc)));
  }

  /* set the minimum voxel opacity (the render ignores voxels with values below this*/
  if (vpSetd(context->vpc, VP_MIN_VOXEL_OPACITY, min_voxel_opacity) != VP_OK) {
    g_warning("Error Setting the Min Voxel Opacity (%s): %s", 
	      context->name, vpGetErrorString(vpGetError(context->vpc)));
  }

  return;
}

/* function to set up the image that we'll be getting back from the rendering */
void rendering_context_set_image(rendering_t * context, pixel_type_t pixel_type, gdouble zoom) {

  guint volpack_pixel_type;
  amide_intpoint_t size_dim; /* size in one dimension, note that height == width */

  context->need_rerender = TRUE;

  switch (pixel_type) {
  case GRAYSCALE:
    volpack_pixel_type = VP_LUMINANCE;
    break;
  case OPACITY:
  default:
    volpack_pixel_type = VP_ALPHA;
    break;
  }
  size_dim = ceil(zoom*POINT_MAX(context->dim));
  g_free(context->image);
  if ((context->image = g_new(guchar,size_dim*size_dim)) == NULL) {
    g_warning("Could not allocate space for Rendering Image for %s", 
	      context->name);
    return;
  }

  context->pixel_type = pixel_type;
  if (vpSetImage(context->vpc, (guchar *) context->image, size_dim,
		 size_dim, size_dim* RENDERING_DENSITY_SIZE, volpack_pixel_type)) {
    g_warning("Error Switching the Rendering Image Pixel Return Type (%s): %s",
	      context->name, vpGetErrorString(vpGetError(context->vpc)));
  }
  return;
}

/* switch the state of depth cueing (TRUE is on) */
void rendering_context_set_depth_cueing(rendering_t * context, gboolean state) {

  context->need_rerender = TRUE;

  if (vpEnable(context->vpc, VP_DEPTH_CUE, state) != VP_OK) {
      g_warning("Error Setting the Rendering Depth Cue (%s): %s",
		context->name, vpGetErrorString(vpGetError(context->vpc)));
  }

  return;

}


/* set the depth cueing parameters*/
void rendering_context_set_depth_cueing_parameters(rendering_t * context, 
						   gdouble front_factor, gdouble density) {

  context->need_rerender = TRUE;

  /* the defaults should be 1.0 and 1.0 */
  if (vpSetDepthCueing(context->vpc, front_factor, density) != VP_OK){
    g_warning("Error Enabling Rendering Depth Cueing (%s): %s",
	      context->name, vpGetErrorString(vpGetError(context->vpc)));
  }

  return;
}


/* to render a context... */
void rendering_context_render(rendering_t * context)
{

  if (context->need_rerender)
    if (context->vpc != NULL) 
      if (vpRenderRawVolume(context->vpc) != VP_OK) {
	g_warning("Error Rendering the Volume (%s): %s", 
		  context->name, vpGetErrorString(vpGetError(context->vpc)));
	return;
      }
  
  context->need_rerender = FALSE;

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

    renderings->context = rendering_context_unref(renderings->context);
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
					      const AmitkInterpolation interpolation) {

  renderings_t * temp_renderings = NULL;

  if (objects == NULL) return temp_renderings;

  if ((temp_renderings = g_new(renderings_t, 1)) == NULL) {
    return temp_renderings;
  }
  temp_renderings->ref_count = 1;
  
  temp_renderings->context = 
    rendering_context_init(objects->data, render_volume,voxel_size, start, duration, interpolation);
  temp_renderings->next = 
    renderings_init_recurse(objects->next, render_volume, voxel_size, start, duration, interpolation);
  
  return temp_renderings;
}
					      
					      

/* returns an initialized rendering list */
renderings_t * renderings_init(GList * objects,const amide_time_t start, 
			       const amide_time_t duration,const AmitkInterpolation interpolation) {
  
  AmitkCorners render_corner;
  amide_real_t voxel_size;
  AmitkVolume * render_volume;
  
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
  return renderings_init_recurse(objects, render_volume,voxel_size, start, duration, interpolation);
  g_object_unref(render_volume);
}


/* reloads the rendering list when needed */
void renderings_reload_objects(renderings_t * renderings, const amide_time_t start, 
			       const amide_time_t duration, const AmitkInterpolation interpolation) {

  if (renderings != NULL) {
    /* reload this context */
    rendering_context_reload_objects(renderings->context, start, duration, interpolation);
    /* and do the next */
    renderings_reload_objects(renderings->next, start, duration, interpolation);
  }

  return;
}
    


/* sets the rotation transform for a list of rendering context */
void renderings_set_rotation(renderings_t * renderings, AmitkAxis dir, gdouble rotation) {


  while (renderings != NULL) {
    rendering_context_set_rotation(renderings->context, dir, rotation);
    renderings = renderings->next;
  }

  return;
}

/* resets the rotation transform for a list of rendering context */
void renderings_reset_rotation(renderings_t * renderings) {


  while (renderings != NULL) {
    rendering_context_reset_rotation(renderings->context);
    renderings = renderings->next;
  }

  return;
}

/* to set the quality on a list of contexts */
void renderings_set_quality(renderings_t * renderings, rendering_quality_t quality) {

  while (renderings != NULL) {
    rendering_context_set_quality(renderings->context, quality);
    renderings = renderings->next;
  }

  return;
}

/* set the return image parameters  for a list of contexts */
void renderings_set_zoom(renderings_t * renderings, gdouble zoom) {

  while (renderings != NULL) {
    rendering_context_set_image(renderings->context, 
				renderings->context->pixel_type, 
				zoom);
    renderings = renderings->next;
  }

  return;
}

/* turn depth cueing on/off for a list of contexts */
void renderings_set_depth_cueing(renderings_t * renderings, gboolean state) {

  while (renderings != NULL) {
    rendering_context_set_depth_cueing(renderings->context, state);
    renderings = renderings->next;
  }

  return;
}


/* sets the depth cueing parameters for alist of contexts */
void renderings_set_depth_cueing_parameters(renderings_t * renderings, 
					    gdouble front_factor, gdouble density) {

  while (renderings != NULL) {
    rendering_context_set_depth_cueing_parameters(renderings->context, 
						  front_factor, density);
    renderings = renderings->next;
  }

  return;
}


/* to render a list of contexts... */
void renderings_render(renderings_t * renderings)
{

  while (renderings != NULL) {
    rendering_context_render(renderings->context);
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



