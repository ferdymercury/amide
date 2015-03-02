/* rendering.c
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

#ifdef AMIDE_LIBVOLPACK_SUPPORT

#include <glib.h>
#include <stdlib.h>
#include <math.h>
#include "rendering.h"


/* external variables */
gchar * rendering_quality_names[] = {"Highest Quality and Slowest",
				     "High Quality and Medium Speed",
				     "Medium Quality and Fast",
				     "Low Quality and Fastest"};
gchar * pixel_type_names[] = {"Opacity",
			      "Grayscale"};
			      

rendering_t * rendering_context_free(rendering_t * context) {
  
  classification_t i_class;

  if (context== NULL)
    return context;

  /* sanity checks */
  g_return_val_if_fail(context->reference_count > 0, NULL);

  context->reference_count--; 

#ifdef AMIDE_DEBUG
  if (context->reference_count == 0)
    g_print("freeing rendering context of %s\n",context->name);
#endif

  /* if we've removed all reference's, free the context */
  if (context->reference_count == 0) {
    if (context->type == VOLUME)
      context->volume = volume_free(context->volume);
    else
      context->roi = roi_free(context->roi);
    g_free(context->rendering_vol);
    g_free(context->name);
    if (context->vpc != NULL)
      vpDestroyContext(context->vpc);
    g_free(context->image);
    for (i_class = 0; i_class < NUM_CLASSIFICATIONS; i_class++) {
      g_free(context->ramp_x[i_class]);
      g_free(context->ramp_y[i_class]);
    }
    g_free(context);
    context = NULL;
  }

  return context;

}



/* either volume or roi must be NULL */
rendering_t * rendering_context_init(volume_t * volume, 
				     roi_t * roi,
				     const realspace_t render_coord_frame, 
				     const realpoint_t render_far_corner, 
				     const floatpoint_t voxel_size, 
				     const amide_time_t start, 
				     const amide_time_t duration, 
				     const interpolation_t interpolation) {

  rendering_t * new_context = NULL;
  gint i;
  gint init_density_ramp_x[] = RENDERING_DENSITY_RAMP_X;
  gfloat init_density_ramp_y[] = RENDERING_DENSITY_RAMP_Y;
  gint init_gradient_ramp_x[] = RENDERING_GRADIENT_RAMP_X;
  gfloat init_gradient_ramp_y[] = RENDERING_GRADIENT_RAMP_Y;

  g_return_val_if_fail((volume != NULL) || (roi != NULL), NULL);

  if ((new_context =  (rendering_t *) g_malloc(sizeof(rendering_t))) == NULL) {
    g_warning("%s: couldn't allocate space for rendering context",PACKAGE);
    return NULL;
  }
  new_context->reference_count = 1;
  
  /* start initializing what we can */
  new_context->vpc = vpCreateContext();
  if (volume != NULL) {
    new_context->type = VOLUME;
    new_context->volume = volume_copy(volume);
    new_context->roi = NULL;
    new_context->name = g_strdup(volume->name);
    new_context->color_table = volume->color_table;
  } else { /* roi != NULL */
    new_context->type = ROI;
    new_context->volume = NULL;
    new_context->roi = roi_copy(roi);
    new_context->name = g_strdup(roi->name);
    new_context->color_table = BW_LINEAR;
  }
  new_context->pixel_type = RENDERING_DEFAULT_PIXEL_TYPE;
  new_context->image = NULL;
  new_context->rendering_vol = NULL;
  new_context->curve_type[DENSITY_CLASSIFICATION] = CURVE_LINEAR;
  new_context->curve_type[GRADIENT_CLASSIFICATION] = CURVE_LINEAR;
  new_context->initial_coord_frame = new_context->current_coord_frame = render_coord_frame;
  new_context->start = start;
  new_context->duration = duration;

  /* figure out the size of our context */
  new_context->dim.x = ceil(render_far_corner.x/voxel_size);
  new_context->dim.y = ceil(render_far_corner.y/voxel_size);
  new_context->dim.z = ceil(render_far_corner.z/voxel_size);
  new_context->voxel_size.x = voxel_size;
  new_context->voxel_size.y = voxel_size;
  new_context->voxel_size.z = voxel_size;


#if AMIDE_DEBUG
  g_print("\tSetting up a Rendering Context\n");
#endif

  /* setup the density and gradient ramps */
  new_context->num_points[DENSITY_CLASSIFICATION] = RENDERING_DENSITY_RAMP_POINTS;
  if ((new_context->ramp_x[DENSITY_CLASSIFICATION] = 
       (gint *) g_malloc(new_context->num_points[DENSITY_CLASSIFICATION]*sizeof(gint))) == NULL) {
    g_warning("%s: couldn't allocate space for density ramp x",PACKAGE);
    return NULL;
  }
  if ((new_context->ramp_y[DENSITY_CLASSIFICATION] = 
       (gfloat *) g_malloc(new_context->num_points[DENSITY_CLASSIFICATION]*sizeof(gfloat))) == NULL) {
    g_warning("%s: couldn't allocate space for density ramp y",PACKAGE);
    return NULL;
  }
  for (i=0;i<new_context->num_points[DENSITY_CLASSIFICATION];i++) {
    new_context->ramp_x[DENSITY_CLASSIFICATION][i] = init_density_ramp_x[i];
    new_context->ramp_y[DENSITY_CLASSIFICATION][i] = init_density_ramp_y[i];
  }

  new_context->num_points[GRADIENT_CLASSIFICATION] = RENDERING_GRADIENT_RAMP_POINTS;
  if ((new_context->ramp_x[GRADIENT_CLASSIFICATION] = 
       (gint *) g_malloc(new_context->num_points[GRADIENT_CLASSIFICATION]*sizeof(gint))) == NULL) {
    g_warning("%s: couldn't allocate space for gradient ramp x",PACKAGE);
    return NULL;
  }
  if ((new_context->ramp_y[GRADIENT_CLASSIFICATION] = 
       (gfloat *) g_malloc(new_context->num_points[GRADIENT_CLASSIFICATION]*sizeof(gfloat))) == NULL) {
    g_warning("%s: couldn't allocate space for gradient ramp y",PACKAGE);
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
    g_warning("%s: Error Setting the Rendering Density Ramp", PACKAGE);
    new_context = rendering_context_free(new_context);
    return new_context;
  }

  /* setup the context's gradient classification */
  if (vpRamp(new_context->gradient_ramp, sizeof(gfloat), 
	     new_context->num_points[GRADIENT_CLASSIFICATION],
	     new_context->ramp_x[GRADIENT_CLASSIFICATION], 
	     new_context->ramp_y[GRADIENT_CLASSIFICATION]) != VP_OK) {
    g_warning("%s: Error Setting the Rendering Gradient Ramp", PACKAGE);
    new_context = rendering_context_free(new_context);
    return new_context;
  }

  /* tell the rendering context info on the voxel structure */
  if (vpSetVoxelSize(new_context->vpc,  RENDERING_BYTES_PER_VOXEL, RENDERING_VOXEL_FIELDS, 
		     RENDERING_SHADE_FIELDS, RENDERING_CLSFY_FIELDS) != VP_OK) {
    g_warning("%s: Error Setting the Rendering Voxel Size (%s): %s", 
	      PACKAGE, new_context->name, vpGetErrorString(vpGetError(new_context->vpc)));
    new_context = rendering_context_free(new_context);
    return new_context;
  }

  /* now tell the rendering context the location of each field in voxel, 
     do this for each field in the context */
  if (vpSetVoxelField (new_context->vpc,RENDERING_NORMAL_FIELD, RENDERING_NORMAL_SIZE, 
		       RENDERING_NORMAL_OFFSET, RENDERING_NORMAL_MAX) != VP_OK) {
    g_warning("%s: Error Specifying the Rendering Voxel Fields (%s, NORMAL): %s", 
	      PACKAGE, new_context->name, vpGetErrorString(vpGetError(new_context->vpc)));
    new_context = rendering_context_free(new_context);
    return new_context;
  }
  if (vpSetVoxelField (new_context->vpc,RENDERING_DENSITY_FIELD, RENDERING_DENSITY_SIZE, 
		       RENDERING_DENSITY_OFFSET, RENDERING_DENSITY_MAX) != VP_OK) {
    g_warning("%s: Error Specifying the Rendering Voxel Fields (%s, DENSITY): %s", 
	      PACKAGE, new_context->name, vpGetErrorString(vpGetError(new_context->vpc)));
    new_context = rendering_context_free(new_context);
    return new_context;
  }
  if (vpSetVoxelField (new_context->vpc,RENDERING_GRADIENT_FIELD, RENDERING_GRADIENT_SIZE, 
		       RENDERING_GRADIENT_OFFSET, RENDERING_GRADIENT_MAX) != VP_OK) {
    g_warning("%s: Error Specifying the Rendering Voxel Fields (%s, GRADIENT): %s",
	      PACKAGE, new_context->name, vpGetErrorString(vpGetError(new_context->vpc)));
    new_context = rendering_context_free(new_context);
    return new_context;
  }

  /* apply density classification to the vpc */
  if (vpSetClassifierTable(new_context->vpc, RENDERING_DENSITY_PARAM, RENDERING_DENSITY_FIELD, 
			   new_context->density_ramp,
			   sizeof(new_context->density_ramp)) != VP_OK){
    g_warning("%s: Error Setting the Rendering Classifier Table (%s, DENSITY): %s",
	      PACKAGE, new_context->name, vpGetErrorString(vpGetError(new_context->vpc)));
    new_context = rendering_context_free(new_context);
    return new_context;
  }

  /* apply it to the different vpc's */
  if (vpSetClassifierTable(new_context->vpc, RENDERING_GRADIENT_PARAM, RENDERING_GRADIENT_FIELD, 
			   new_context->gradient_ramp,
			   sizeof(new_context->gradient_ramp)) != VP_OK){
    g_warning("%s: Error Setting the Classifier Table (%s, GRADIENT): %s",
	      PACKAGE, new_context->name,  vpGetErrorString(vpGetError(new_context->vpc)));
    new_context = rendering_context_free(new_context);
    return new_context;
  }

  /* now copy the volume/roi data into the rendering context */
  rendering_context_load_object(new_context, interpolation);

  /* and finally, set up the structure into which the rendering will be returned */
  rendering_context_set_image(new_context, new_context->pixel_type, RENDERING_DEFAULT_ZOOM);

  return new_context;
}


/* function to reload the context's concept of a volume when necessary */
void rendering_context_reload_objects(rendering_t * rendering_context, 
				     const amide_time_t new_start,
				     const amide_time_t new_duration, 
				     const interpolation_t interpolation) {

  amide_time_t old_start, old_duration;

  old_start = rendering_context->start;
  old_duration = rendering_context->duration;

  rendering_context->start = new_start;
  rendering_context->duration = new_duration;

  /* check if we really need to do anything */
  if (rendering_context->type == ROI) 
    return;
  if (volume_frame(rendering_context->volume, new_start) == 
      volume_frame(rendering_context->volume, old_start))
    if (volume_frame(rendering_context->volume, new_start+new_duration) ==
	volume_frame(rendering_context->volume, old_start+old_duration))
      return;

  /* allright, reload the context's data */
  rendering_context_load_object(rendering_context, interpolation);

}



/* function to update the rendering structure's concept of the volume or roi */
void rendering_context_load_object(rendering_t * rendering_context, 
				   const interpolation_t interpolation) {

  voxelpoint_t i_voxel, j_voxel;
  rendering_density_t * density; /* buffer for density data */
  guint density_size;/* size of density data */
  guint context_size;/* size of context */
#if AMIDE_DEBUG
  div_t x;
  gint divider;
#endif

  /* tell the context the dimensions of our rendering context */
  if (vpSetVolumeSize(rendering_context->vpc, rendering_context->dim.x, 
		      rendering_context->dim.y, rendering_context->dim.z) != VP_OK) {
    g_warning("%s: Error Setting the Context Size (%s): %s", 
	      PACKAGE, rendering_context->name, 
	      vpGetErrorString(vpGetError(rendering_context->vpc)));
    return;
  }

  /* allocate space for the raw data and the context */
  density_size =  rendering_context->dim.x *  rendering_context->dim.y *  
    rendering_context->dim.z * RENDERING_DENSITY_SIZE;
  context_size =  rendering_context->dim.x *  rendering_context->dim.y * 
     rendering_context->dim.z * RENDERING_BYTES_PER_VOXEL;

  if ((density = (rendering_density_t * ) g_malloc(density_size)) == NULL) {
    g_warning("%s: Could not allocate space for density data for %s", 
	      PACKAGE, rendering_context->name);
    return;
  }


  g_free(rendering_context->rendering_vol);
  if ((rendering_context->rendering_vol = (rendering_voxel_t * ) g_malloc(context_size)) == NULL) {
    g_warning("%s: Could not allocate space for rendering context volume for %s", 
	      PACKAGE, rendering_context->name);
    g_free(density);
    return;
  }

  vpSetRawVoxels(rendering_context->vpc, rendering_context->rendering_vol, context_size, 
		 RENDERING_BYTES_PER_VOXEL,  rendering_context->dim.x * RENDERING_BYTES_PER_VOXEL,
		 rendering_context->dim.x* rendering_context->dim.y * RENDERING_BYTES_PER_VOXEL);

#if AMIDE_DEBUG
  g_print("\tCopying Data into Rendering Context Volume\t");
  divider = ((rendering_context->dim.z/20.0) < 1) ? 1 : (rendering_context->dim.z/20.0);
#endif


  switch(rendering_context->type) {
  case ROI: 
    {
      realpoint_t roi_corner[2], center, radius;
      floatpoint_t height;
      realpoint_t temp_rp;
      gint temp_int;
      
      /* get the roi corners in roi space */
      roi_corner[0] = realspace_base_coord_to_alt(rs_offset(rendering_context->roi->coord_frame), 
						  rendering_context->roi->coord_frame);
      roi_corner[1] = rendering_context->roi->corner;
      
      radius = rp_cmult(0.5, rp_diff(roi_corner[1],roi_corner[0]));
      center = rp_add(rp_cmult(0.5,roi_corner[1]), rp_cmult(0.5,roi_corner[0]));
      height = fabs(roi_corner[1].z-roi_corner[0].z);
      j_voxel.t = j_voxel.z=i_voxel.t=0;
      for (i_voxel.z = 0; i_voxel.z < rendering_context->dim.z; i_voxel.z++) {
#ifdef AMIDE_DEBUG
	x = div(i_voxel.z,divider);
	if (x.rem == 0) g_print(".");
#endif 
	for (i_voxel.y = 0; i_voxel.y <  rendering_context->dim.y; i_voxel.y++)
	  for (i_voxel.x = 0; i_voxel.x <  rendering_context->dim.x; i_voxel.x++) {
	    VOXEL_TO_REALPOINT(i_voxel, rendering_context->voxel_size, temp_rp);
	    temp_rp = realspace_alt_coord_to_alt(temp_rp, 
						 rendering_context->current_coord_frame,
						 rendering_context->roi->coord_frame);
	    switch(rendering_context->roi->type) {
	    case ISOCONTOUR_2D:
	    case ISOCONTOUR_3D:
	      ROI_REALPOINT_TO_VOXEL(rendering_context->roi, temp_rp, j_voxel);
	      if (data_set_includes_voxel(rendering_context->roi->isocontour, j_voxel)) {
		temp_int = *DATA_SET_UBYTE_POINTER(rendering_context->roi->isocontour, j_voxel);
		if (temp_int == 2)
		  temp_int = RENDERING_DENSITY_MAX;
		else if (temp_int == 1)
		  temp_int = RENDERING_DENSITY_MAX/2.0;
	      } else
		temp_int = 0;
	      break;
	    case ELLIPSOID:
	      if (rp_in_ellipsoid(temp_rp, center, radius)) 
		temp_int = RENDERING_DENSITY_MAX;
	      else temp_int = 0;
	      break;
	    case BOX:
	      if (rp_in_box(temp_rp, roi_corner[0], roi_corner[1])) 
		temp_int = RENDERING_DENSITY_MAX;
	      else temp_int = 0;
	      break;
	    case CYLINDER:
	      if (rp_in_elliptic_cylinder(temp_rp, center, height, radius)) 
		temp_int = RENDERING_DENSITY_MAX;
	      else temp_int = 0;
	      break;
	    default:
	      g_warning("%s: unexpected case in %s at line %d",PACKAGE, __FILE__, __LINE__);
	      g_free(density);
	      return;
	      break;
	    }
	    density[i_voxel.x+
		   i_voxel.y*rendering_context->dim.x+
		   (rendering_context->dim.z-i_voxel.z-1)*
		   rendering_context->dim.y*
		   rendering_context->dim.x] = temp_int;
	  }
      }
    }
    break;
  case VOLUME:
    {
      realspace_t slice_coord_frame;
      realpoint_t slice_far_corner;
      volume_t * slice;
      realpoint_t new_offset;
      amide_data_t temp_val, scale;

      scale = ((gdouble) RENDERING_DENSITY_MAX) / 
	(rendering_context->volume->threshold_max-rendering_context->volume->threshold_min);
      
      /* define the slice that we're trying to pull out of the data set */
      slice_far_corner.x = rendering_context->dim.x * rendering_context->voxel_size.x;
      slice_far_corner.y = rendering_context->dim.y * rendering_context->voxel_size.y;
      slice_far_corner.z = 0.0;
      new_offset = realspace_base_coord_to_alt(rs_offset(rendering_context->initial_coord_frame),
					       rendering_context->current_coord_frame);
      slice_coord_frame = rendering_context->initial_coord_frame;
      new_offset.z -= rendering_context->voxel_size.z;
      
      /* copy the info from a volume structure into our rendering_volume structure */
      j_voxel.t = j_voxel.z = i_voxel.t=0;
      for (i_voxel.z = 0; i_voxel.z < rendering_context->dim.z; i_voxel.z++) {
#ifdef AMIDE_DEBUG
	x = div(i_voxel.z,divider);
	if (x.rem == 0) g_print(".");
#endif 
	
	slice_far_corner.z += rendering_context->voxel_size.z;
	new_offset.z += rendering_context->voxel_size.z; 
	rs_set_offset(&(slice_coord_frame), 
		      realspace_alt_coord_to_base(new_offset, rendering_context->initial_coord_frame));
	slice = volume_get_slice(rendering_context->volume, 
				 rendering_context->start, 
				 rendering_context->duration, 
				 rendering_context->voxel_size, 
				 slice_coord_frame,
				 slice_far_corner, interpolation, FALSE);
	
	/* note, I think volpack's z direction is off.... hence the reversal */
	for (j_voxel.y = i_voxel.y = 0; i_voxel.y <  rendering_context->dim.y; j_voxel.y++, i_voxel.y++)
	  for (j_voxel.x = i_voxel.x = 0; i_voxel.x <  rendering_context->dim.x; j_voxel.x++, i_voxel.x++) {
	    temp_val = scale * (VOLUME_FLOAT_0D_SCALING_CONTENTS(slice,j_voxel)
				-rendering_context->volume->threshold_min);
	    if (temp_val > RENDERING_DENSITY_MAX) temp_val = RENDERING_DENSITY_MAX;
	    if (temp_val < 0.0) temp_val = 0.0;
	    density[i_voxel.x+
		   i_voxel.y*rendering_context->dim.x+
		   (rendering_context->dim.z-i_voxel.z-1)*
		   rendering_context->dim.y*
		   rendering_context->dim.x] = temp_val;
	  }
	slice = volume_free(slice);
      }
    }
    break;
  default: 
    g_warning("%s: unexpected case in %s at line %d",PACKAGE, __FILE__, __LINE__);
    break;
  }

#ifdef AMIDE_DEBUG
  g_print("\n");
#endif

  /* compute surface normals (for shading) and gradient magnitudes (for classification) */
  if (vpVolumeNormals(rendering_context->vpc, density, density_size, RENDERING_DENSITY_FIELD, 
		      RENDERING_GRADIENT_FIELD, RENDERING_NORMAL_FIELD) != VP_OK) {
    g_warning("%s: Error Computing the Rendering Normals (%s): %s",
	      PACKAGE, rendering_context->name, 
	      vpGetErrorString(vpGetError(rendering_context->vpc)));
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
  if (vpMinMaxOctreeThreshold(rendering_context->vpc, RENDERING_DENSITY_PARAM, 
			      RENDERING_OCTREE_DENSITY_THRESH) != VP_OK) {
    g_warning("%s: Error Setting Rendering Octree Threshold (%s, DENSITY): %s",
	      PACKAGE, rendering_context->name, 
	      vpGetErrorString(vpGetError(rendering_context->vpc)));
    return;
  }
  if (vpMinMaxOctreeThreshold(rendering_context->vpc, RENDERING_GRADIENT_PARAM, 
			      RENDERING_OCTREE_GRADIENT_THRESH) != VP_OK) {
    g_warning("%s: Error Setting Rendering Octree Threshold (%s, GRADIENT): %s",
	      PACKAGE, rendering_context->name, 
	      vpGetErrorString(vpGetError(rendering_context->vpc)));
    return;
  }
  
#if AMIDE_DEBUG
  g_print("\tCreating the Min/Max Octree\n");
#endif
  
  /* create the min/max octree */
  if (vpCreateMinMaxOctree(rendering_context->vpc, 0, RENDERING_OCTREE_BASE_NODE_SIZE) != VP_OK) {
    g_warning("%s: Error Generating Octree (%s): %s",
	      PACKAGE, rendering_context->name, 
	      vpGetErrorString(vpGetError(rendering_context->vpc)));
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
  if (vpSetMaterial(rendering_context->vpc, VP_MATERIAL0, VP_SHINYNESS, VP_BOTH_SIDES,0.0,0.0,0.0) != VP_OK){
    g_warning("%s: Error Setting the Rendering Material (%s, SHINYNESS): %s",
	      PACKAGE, rendering_context->name, 
	      vpGetErrorString(vpGetError(rendering_context->vpc)));
    return;
  }

  /* set the shading parameters */
  if (vpSetLookupShader(rendering_context->vpc, 1, 1, RENDERING_NORMAL_FIELD, 
			rendering_context->shade_table, sizeof(rendering_context->shade_table), 
			0, NULL, 0) != VP_OK){
    g_warning("%s: Error Setting the Rendering Shader (%s): %s",
	      PACKAGE, rendering_context->name, 
	      vpGetErrorString(vpGetError(rendering_context->vpc)));
    return;
  }
  
  /* and do the shade table stuff (this fills in the shade table I believe) */
  if (vpShadeTable(rendering_context->vpc) != VP_OK){
    g_warning("%s: Error Shading Table for Rendering (%s): %s",
	      PACKAGE, rendering_context->name, 
	      vpGetErrorString(vpGetError(rendering_context->vpc)));
    return;
  }
  
  return;
}




/* sets the rotation transform for a rendering context */
void rendering_context_set_rotation(rendering_t * context, axis_t dir, gdouble rotation) {

  vpMatrix4 m; 
  guint i,j;
  realpoint_t axis;

  /* rotate the axis */
  realspace_rotate_on_axis(&context->current_coord_frame,
			   rs_specific_axis(context->current_coord_frame, dir),
			   rotation);

  /* initalize */
  for (i=0;i<4;i++)
    for (j=0;j<4;j++)
      if (i == j)
	m[i][j]=1;
      else
	m[i][j]=0;

  /* and setup the matrix we're feeding into volpack */
  for (i=0;i<3;i++) {
    axis = rs_specific_axis(context->current_coord_frame, i);
    m[i][0] = axis.x;
    m[i][1] = axis.y;
    m[i][2] = axis.z;
  }

  /* set the rotation */
  if (vpSetMatrix(context->vpc, m) != VP_OK)
    g_warning("%s: Error Rotating Rendering (%s): %s",
	      PACKAGE, context->name, vpGetErrorString(vpGetError(context->vpc)));

  return;
}

/* reset the rotation transform for a rendering context */
void rendering_context_reset_rotation(rendering_t * context) {

  /* reset the coord frame */
  context->current_coord_frame = context->initial_coord_frame;

  /* reset the rotation */
  rendering_context_set_rotation(context, XAXIS, 0.0);

  return;
}


/* set the speed versus quality parameters of a context */
void rendering_context_set_quality(rendering_t * context, rendering_quality_t quality) {

  gdouble max_ray_opacity, min_voxel_opacity;

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
    g_warning("%s: Error Setting Rendering Max Ray Opacity (%s): %s",
	      PACKAGE, context->name, vpGetErrorString(vpGetError(context->vpc)));
  }

  /* set the minimum voxel opacity (the render ignores voxels with values below this*/
  if (vpSetd(context->vpc, VP_MIN_VOXEL_OPACITY, min_voxel_opacity) != VP_OK) {
    g_warning("%s: Error Setting the Min Voxel Opacity (%s): %s", 
	      PACKAGE, context->name, vpGetErrorString(vpGetError(context->vpc)));
  }

  return;
}

/* function to set up the image that we'll be getting back from the rendering */
void rendering_context_set_image(rendering_t * context, pixel_type_t pixel_type, gdouble zoom) {

  guint volpack_pixel_type;
  intpoint_t size_dim; /* size in one dimension, note that height == width */

  switch (pixel_type) {
  case GRAYSCALE:
    volpack_pixel_type = VP_LUMINANCE;
    break;
  case OPACITY:
  default:
    volpack_pixel_type = VP_ALPHA;
    break;
  }
  size_dim = ceil(zoom*REALPOINT_MAX(context->dim));
  g_free(context->image);
  if ((context->image = 
       (guchar * ) g_malloc(size_dim*size_dim*sizeof(guchar))) == NULL) {
    g_warning("%s: Could not allocate space for Rendering Image for %s", 
	      PACKAGE, context->name);
    return;
  }

  context->pixel_type = pixel_type;
  if (vpSetImage(context->vpc, (guchar *) context->image, size_dim,
		 size_dim, size_dim* RENDERING_DENSITY_SIZE, volpack_pixel_type)) {
    g_warning("%s: Error Switching the Rendering Image Pixel Return Type (%s): %s",
	      PACKAGE, context->name, vpGetErrorString(vpGetError(context->vpc)));
  }
  return;
}

/* switch the state of depth cueing (TRUE is on) */
void rendering_context_set_depth_cueing(rendering_t * context, gboolean state) {

  if (vpEnable(context->vpc, VP_DEPTH_CUE, state) != VP_OK) {
      g_warning("%s: Error Setting the Rendering Depth Cue (%s): %s",
		PACKAGE, context->name, vpGetErrorString(vpGetError(context->vpc)));
  }

  return;

}


/* set the depth cueing parameters*/
void rendering_context_set_depth_cueing_parameters(rendering_t * context, 
						   gdouble front_factor, gdouble density) {

  /* the defaults should be 1.0 and 1.0 */
  if (vpSetDepthCueing(context->vpc, front_factor, density) != VP_OK){
    g_warning("%s: Error Enabling Rendering Depth Cueing (%s): %s",
	      PACKAGE, context->name, vpGetErrorString(vpGetError(context->vpc)));
  }

  return;
}


/* to render a context... */
void rendering_context_render(rendering_t * context)
{

  if (context->vpc != NULL) 
    if (vpRenderRawVolume(context->vpc) != VP_OK) {
      g_warning("%s: Error Rendering the Volume (%s): %s", 
		PACKAGE, context->name, vpGetErrorString(vpGetError(context->vpc)));
      return;
    }

  return;
}





/* free up a list of rendering contexts */
rendering_list_t * rendering_list_free(rendering_list_t * rendering_list) {
  
  if (rendering_list == NULL)
    return rendering_list;

  /* sanity check */
  g_return_val_if_fail(rendering_list->reference_count > 0, NULL);

  rendering_list->reference_count--;



  /* things to do if our reference count is zero */
  if (rendering_list->reference_count == 0) {
    /* recursively delete rest of list */
    rendering_list->next = rendering_list_free(rendering_list->next); 

    rendering_list->rendering_context = rendering_context_free(rendering_list->rendering_context);
    g_free(rendering_list);
    rendering_list = NULL;
  }

  return rendering_list;
}


/* the recursive part of rendering_list_init */
static rendering_list_t * rendering_list_init_recurse(volume_list_t * volumes, 
						      roi_list_t * rois,
						      const realspace_t render_coord_frame,
						      const realpoint_t render_far_corner, 
						      const floatpoint_t voxel_size, 
						      const amide_time_t start, 
						      const amide_time_t duration,
						      const interpolation_t interpolation) {

  rendering_list_t * temp_rendering_list = NULL;

  if ((volumes == NULL) && (rois == NULL)) return temp_rendering_list;

  if ((temp_rendering_list = (rendering_list_t *) g_malloc(sizeof(rendering_list_t))) == NULL) {
    return temp_rendering_list;
  }
  temp_rendering_list->reference_count = 1;
  
  if (volumes != NULL) {
    temp_rendering_list->rendering_context = 
      rendering_context_init(volumes->volume, NULL, render_coord_frame, render_far_corner,
			     voxel_size, start, duration, interpolation);
    temp_rendering_list->next = 
      rendering_list_init_recurse(volumes->next, rois, render_coord_frame, render_far_corner,
				  voxel_size, start, duration, interpolation);
  } else { /* rois != NULL */
    temp_rendering_list->rendering_context = 
      rendering_context_init(NULL, rois->roi, render_coord_frame, render_far_corner,
			     voxel_size, start, duration, interpolation);
    temp_rendering_list->next = 
      rendering_list_init_recurse(volumes, rois->next, render_coord_frame, render_far_corner,
				  voxel_size, start, duration, interpolation);
  }
  
  return temp_rendering_list;
}
					      
					      

/* returns an initialized rendering list */
rendering_list_t * rendering_list_init(volume_list_t * volumes, roi_list_t * rois,
				       realspace_t render_coord_frame, const amide_time_t start, 
				       const amide_time_t duration,const interpolation_t interpolation) {
  
  realpoint_t render_corner[2];
  realpoint_t render_far_corner;
  realpoint_t size;
  floatpoint_t voxel_size;
  
  if ((volumes == NULL) && (rois == NULL)) return NULL;

  /* figure out all encompasing corners for the slices based on our viewing axis */
  if (volumes != NULL) 
    volumes_get_view_corners(volumes, render_coord_frame, render_corner);
  else
    rois_get_view_corners(rois, render_coord_frame, render_corner);
  render_corner[0] = realspace_alt_coord_to_base(render_corner[0], render_coord_frame);
  render_corner[1] = realspace_alt_coord_to_base(render_corner[1], render_coord_frame);
  size = rp_sub(render_corner[1], render_corner[0]);
  rs_set_offset(&render_coord_frame, render_corner[0]);
  render_far_corner = realspace_base_coord_to_alt(render_corner[1], render_coord_frame);

  /* figure out what size our rendering voxels will be */
  if (volumes != NULL)
    voxel_size = volumes_max_min_voxel_size(volumes);
  else if (rois != NULL)
    voxel_size = rois_max_min_voxel_size(rois);
  else
    voxel_size = 1.0; /* mm */
  if (FLOATPOINT_EQUAL(voxel_size, 0.0)) voxel_size = 1.0; /* make sure voxel size is sane */

  /* and generate our rendering list */
  return  rendering_list_init_recurse(volumes, rois, render_coord_frame, render_far_corner,
				      voxel_size, start, duration, interpolation);
}


/* reloads the rendering list when needed */
void rendering_list_reload_objects(rendering_list_t * rendering_list, const amide_time_t start, 
				   const amide_time_t duration, const interpolation_t interpolation) {

  if (rendering_list != NULL) {
    /* reload this context */
    rendering_context_reload_objects(rendering_list->rendering_context, start, duration, interpolation);
    /* and do the next */
    rendering_list_reload_objects(rendering_list->next, start, duration, interpolation);
  }

  return;
}
    


/* sets the rotation transform for a list of rendering context */
void rendering_list_set_rotation(rendering_list_t * rendering_list, axis_t dir, gdouble rotation) {


  while (rendering_list != NULL) {
    rendering_context_set_rotation(rendering_list->rendering_context, dir, rotation);
    rendering_list = rendering_list->next;
  }

  return;
}

/* resets the rotation transform for a list of rendering context */
void rendering_list_reset_rotation(rendering_list_t * rendering_list) {


  while (rendering_list != NULL) {
    rendering_context_reset_rotation(rendering_list->rendering_context);
    rendering_list = rendering_list->next;
  }

  return;
}

/* to set the quality on a list of contexts */
void rendering_list_set_quality(rendering_list_t * rendering_list, rendering_quality_t quality) {

  while (rendering_list != NULL) {
    rendering_context_set_quality(rendering_list->rendering_context, quality);
    rendering_list = rendering_list->next;
  }

  return;
}

/* set the return image parameters  for a list of contexts */
void rendering_list_set_image(rendering_list_t * rendering_list, pixel_type_t pixel_type, gdouble zoom) {

  while (rendering_list != NULL) {
    rendering_context_set_image(rendering_list->rendering_context, pixel_type, zoom);
    rendering_list = rendering_list->next;
  }

  return;
}

/* turn depth cueing on/off for a list of contexts */
void rendering_list_set_depth_cueing(rendering_list_t * rendering_list, gboolean state) {

  while (rendering_list != NULL) {
    rendering_context_set_depth_cueing(rendering_list->rendering_context, state);
    rendering_list = rendering_list->next;
  }

  return;
}


/* sets the depth cueing parameters for alist of contexts */
void rendering_list_set_depth_cueing_parameters(rendering_list_t * rendering_list, 
						gdouble front_factor, gdouble density) {

  while (rendering_list != NULL) {
    rendering_context_set_depth_cueing_parameters(rendering_list->rendering_context, 
						  front_factor, density);
    rendering_list = rendering_list->next;
  }

  return;
}


/* to render a list of contexts... */
void rendering_list_render(rendering_list_t * rendering_list)
{

  while (rendering_list != NULL) {
    rendering_context_render(rendering_list->rendering_context);
    rendering_list = rendering_list->next;
  }

  return;
}

/* count the rendering lists */
guint rendering_list_count(rendering_list_t * rendering_list) {
  if (rendering_list != NULL)
    return (1+rendering_list_count(rendering_list->next));
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
  //  vpIdentityMatrix(vpc[PET_VOLUME]);

  
  /*  if (vpWindow(vpc[PET_VOLUME], VP_PARALLEL, -0.5, 0.5, -0.5, 0.5, -0.5, 0.5) != VP_OK){
    g_warning("Error Setting the Window (PET_VOLUME): %s",vpGetErrorString(vpGetError(vpc[PET_VOLUME])));
    return FALSE;
  } */

  /*  if (vpCurrentMatrix(vpc[PET_VOLUME], VP_MODEL) != VP_OK){
    g_warning("Error Setting the Model (PET_VOLUME): %s",vpGetErrorString(vpGetError(vpc[PET_VOLUME])));
    return FALSE;
  }  */

  



#endif



