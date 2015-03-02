static GSList * add_intersection_point(GSList * points_list, realpoint_t new_point) {
  realpoint_t * ppoint;

  ppoint = (realpoint_t * ) g_malloc(sizeof(realpoint_t));
  *ppoint = new_point;

  return  g_slist_prepend(points_list, ppoint);
}

static GSList * add_intersection_point_last(GSList * points_list, realpoint_t new_point) {
  realpoint_t * ppoint;

  ppoint = (realpoint_t * ) g_malloc(sizeof(realpoint_t));
  *ppoint = new_point;

  return  g_slist_append(points_list, ppoint);
}







static gboolean is_intersection_point(const roi_t * roi, 
				      const volume_t * slice,
#ifdef BOX_DEFINED
				      realpoint_t roi_corner[],
#endif
#ifdef CYLINDER_DEFINED
				      floatpoint_t height,
#endif
#if (ELLIPSOID_DEFINED == 1 || CYLINDER_DEFINED == 1)
				      realpoint_t center,
				      realpoint_t radius,
#endif
				      voxelpoint_t test_vp) {

  realpoint_t edge_rp[4], test_rp;
  guint i;
  gboolean any, all, voxel_in;
#if (ISOCONTOUR_2D_DEFINED == 1 || ISOCONTOUR_3D_DEFINED)
  voxelpoint_t roi_vp;
#endif

  edge_rp[0].x = slice->voxel_size.x*test_vp.x;
  edge_rp[0].y = slice->voxel_size.y*test_vp.y;
  edge_rp[0].z = slice->voxel_size.z*test_vp.z;
  test_vp.x++;
  edge_rp[1].x = slice->voxel_size.x*test_vp.x;
  edge_rp[1].y = slice->voxel_size.y*test_vp.y;
  edge_rp[1].z = slice->voxel_size.z*test_vp.z;
  test_vp.y++;
  edge_rp[2].x = slice->voxel_size.x*test_vp.x;
  edge_rp[2].y = slice->voxel_size.y*test_vp.y;
  edge_rp[2].z = slice->voxel_size.z*test_vp.z;
  test_vp.x--;
  edge_rp[3].x = slice->voxel_size.x*test_vp.x;
  edge_rp[3].y = slice->voxel_size.y*test_vp.y;
  edge_rp[3].z = slice->voxel_size.z*test_vp.z;

  any= FALSE;
  all = TRUE;
  for (i=0;i<4;i++) {
    test_rp = realspace_alt_coord_to_alt(edge_rp[i], slice->coord_frame, roi->coord_frame);
#ifdef BOX_DEFINED
    voxel_in = realpoint_in_box(test_rp, roi_corner[0],roi_corner[1]);
#endif
#ifdef CYLINDER_DEFINED
    voxel_in = realpoint_in_elliptic_cylinder(test_rp, center, height, radius);
#endif
#ifdef ELLIPSOID_DEFINED
    voxel_in = realpoint_in_ellipsoid(test_rp,center,radius);
#endif
#if (ISOCONTOUR_2D_DEFINED == 1 || ISOCONTOUR_3D_DEFINED)
    ROI_REALPOINT_TO_VOXEL(roi, test_rp, roi_vp);
    if (data_set_includes_voxel(roi->isocontour, roi_vp))
      voxel_in = *DATA_SET_UBYTE_POINTER(roi->isocontour, roi_vp);
    else
      voxel_in = FALSE;
#endif
    all = all && voxel_in;
    any = any || voxel_in;
  }

  if (!all && any)
    return TRUE;
  else
    return FALSE;
}









static GSList * find_intersection_points(const roi_t * roi, 
					 const volume_t * slice, 
					 data_set_t * checked_ds, 
#ifdef BOX_DEFINED
					 realpoint_t roi_corner[],
#endif
#ifdef CYLINDER_DEFINED
					 floatpoint_t height,
#endif
#if (ELLIPSOID_DEFINED == 1 || CYLINDER_DEFINED == 1)
					 realpoint_t center,
					 realpoint_t radius,
#endif
					 voxelpoint_t test_vp) {

  GSList * points_list = NULL;
  realpoint_t test_rp, volume_rp;
  voxelpoint_t temp_vp;
  gboolean edge;
#if (ISOCONTOUR_2D_DEFINED == 1 || ISOCONTOUR_3D_DEFINED)
  voxelpoint_t roi_vp;
#endif

  /* if we've already found this point, return */
  if (*DATA_SET_UBYTE_POINTER(checked_ds, test_vp) == TRUE)
    return points_list;

  /* make sure we're still on the volume */
  if (!data_set_includes_voxel(slice->data_set, test_vp))
    return points_list;

  /* if this point is at the edge of the view slice, if it's in the ROI we'll add it to the points list */
  if (vp_equal(test_vp, voxelpoint_zero) || 
      ((test_vp.x == slice->data_set->dim.x-1) &&
       (test_vp.y == slice->data_set->dim.y-1) &&
       (test_vp.z == slice->data_set->dim.z-1))) {

    VOLUME_VOXEL_TO_REALPOINT(slice, test_vp, volume_rp);
    test_rp = realspace_alt_coord_to_alt(volume_rp, slice->coord_frame, roi->coord_frame);

#ifdef BOX_DEFINED
    edge = realpoint_in_box(test_rp, roi_corner[0],roi_corner[1]);
#endif
#ifdef CYLINDER_DEFINED
    edge = realpoint_in_elliptic_cylinder(test_rp, center, height, radius);
#endif
#ifdef ELLIPSOID_DEFINED
    edge = realpoint_in_ellipsoid(test_rp,center,radius);
#endif
#if (ISOCONTOUR_2D_DEFINED == 1 || ISOCONTOUR_3D_DEFINED)
    ROI_REALPOINT_TO_VOXEL(roi, test_rp, roi_vp);
    if (data_set_includes_voxel(roi->isocontour, roi_vp)) 
      edge = *DATA_SET_UBYTE_POINTER(roi->isocontour, roi_vp);
    else
      edge = FALSE;
#endif

  } else {  /* check that this is really an edge point */

    edge = is_intersection_point(roi, slice, 
#ifdef BOX_DEFINED
				 roi_corner,
#endif
#ifdef CYLINDER_DEFINED
				 height,
#endif
#if (ELLIPSOID_DEFINED == 1 || CYLINDER_DEFINED == 1)
				 center,
				 radius,
#endif
				 test_vp);
  }

  if (edge) {
    DATA_SET_UBYTE_SET_CONTENT(checked_ds, test_vp) = TRUE; /* mark point as done */

    /* test the surround eight points */
    temp_vp = test_vp;
    temp_vp.x--;
    points_list = find_intersection_points(roi, slice, checked_ds, 
#ifdef BOX_DEFINED
					   roi_corner,
#endif
#ifdef CYLINDER_DEFINED
					   height,
#endif
#if (ELLIPSOID_DEFINED == 1 || CYLINDER_DEFINED == 1)
					   center,
					   radius,
#endif
					   temp_vp);
    
    if (points_list == NULL) {
      temp_vp.y--;
      temp_vp.x++;
      points_list = find_intersection_points(roi, slice, checked_ds, 
#ifdef BOX_DEFINED
					     roi_corner,
#endif
#ifdef CYLINDER_DEFINED
					     height,
#endif
#if (ELLIPSOID_DEFINED == 1 || CYLINDER_DEFINED == 1)
					     center,
					     radius,
#endif
					     temp_vp);
    }    
    
    
    if (points_list == NULL) {
      temp_vp.y++;
      temp_vp.x++;
      points_list = find_intersection_points(roi, slice, checked_ds, 
#ifdef BOX_DEFINED
					     roi_corner,
#endif
#ifdef CYLINDER_DEFINED
					     height,
#endif
#if (ELLIPSOID_DEFINED == 1 || CYLINDER_DEFINED == 1)
					     center,
					     radius,
#endif
					     temp_vp);
    }    
    
    
    
    if (points_list == NULL) {
      temp_vp.x--;
      temp_vp.y++;
      points_list = find_intersection_points(roi, slice, checked_ds, 
#ifdef BOX_DEFINED
					     roi_corner,
#endif
#ifdef CYLINDER_DEFINED
					     height,
#endif
#if (ELLIPSOID_DEFINED == 1 || CYLINDER_DEFINED == 1)
					     center,
					     radius,
#endif
					     temp_vp);
    }    
    

    
    
    if (points_list == NULL) {
      temp_vp.x--;
      points_list = find_intersection_points(roi, slice, checked_ds, 
#ifdef BOX_DEFINED
					     roi_corner,
#endif
#ifdef CYLINDER_DEFINED
					     height,
#endif
#if (ELLIPSOID_DEFINED == 1 || CYLINDER_DEFINED == 1)
					     center,
					     radius,
#endif
					     temp_vp);
    }    
    
    
    
    
    if (points_list == NULL) {
      temp_vp.y--;
      temp_vp.y--;
      points_list = find_intersection_points(roi, slice, checked_ds, 
#ifdef BOX_DEFINED
					     roi_corner,
#endif
#ifdef CYLINDER_DEFINED
					     height,
#endif
#if (ELLIPSOID_DEFINED == 1 || CYLINDER_DEFINED == 1)
					     center,
					     radius,
#endif
					     temp_vp);
    }    
    
    
    
    
    if (points_list == NULL) {
      temp_vp.x++;
      temp_vp.x++;
      points_list = find_intersection_points(roi, slice, checked_ds, 
#ifdef BOX_DEFINED
					     roi_corner,
#endif
#ifdef CYLINDER_DEFINED
					     height,
#endif
#if (ELLIPSOID_DEFINED == 1 || CYLINDER_DEFINED == 1)
					     center,
					     radius,
#endif
					     temp_vp);
    }    
    
    
    
    
    if (points_list == NULL) {
      temp_vp.y++;
      temp_vp.y++;
      points_list = find_intersection_points(roi, slice, checked_ds, 
#ifdef BOX_DEFINED
					     roi_corner,
#endif
#ifdef CYLINDER_DEFINED
					     height,
#endif
#if (ELLIPSOID_DEFINED == 1 || CYLINDER_DEFINED == 1)
					     center,
					     radius,
#endif
					     temp_vp);
    }    

    /* and add this point to the list */
    VOLUME_VOXEL_TO_REALPOINT(slice, test_vp, volume_rp);
    points_list = add_intersection_point(points_list, volume_rp);
  }

  return points_list;
}

GSList * roi_`'m4_Variable_Type`'_get_volume_intersection_points(const roi_t * roi,
								 const volume_t * view_slice) {
								 

  GSList * return_points;
  realpoint_t view_rp,temp_rp, slice_corner[2], roi_corner[2];
  voxelpoint_t i, temp_vp;
  gboolean edge;
  data_set_t * temp_ds;
#if (ELLIPSOID_DEFINED == 1 || CYLINDER_DEFINED == 1)
  realpoint_t radius, center;
#endif
#ifdef CYLINDER_DEFINED
  floatpoint_t height;
#endif
#if (ISOCONTOUR_2D_DEFINED ==1 || ISOCONTOUR_3D_DEFINED ==1)
  voxelpoint_t roi_vp;
#endif

  /* sanity checks */
  g_assert(view_slice->data_set->dim.z == 1);

  /* make sure we've already defined this guy */
  if (roi_undrawn(roi))  return NULL;

#ifdef AMIDE_DEBUG
  g_print("roi %s --------------------\n",roi->name);
  g_print("\t\tcorner\tx %5.3f\ty %5.3f\tz %5.3f\n",
	     roi->corner.x,roi->corner.y,roi->corner.z);
#endif

  /* get the roi corners in roi space */
  roi_corner[0] = realspace_base_coord_to_alt(rs_offset(roi->coord_frame), roi->coord_frame);
  roi_corner[1] = roi->corner;

#if (ELLIPSOID_DEFINED == 1 || CYLINDER_DEFINED == 1)
  /* figure out the radius in each direction */
  radius = rp_cmult(0.5, rp_diff(roi_corner[1],roi_corner[0]));
  g_print("radius %5.3f %5.3f %5.3f\n", radius.x, radius.y, radius.z);
  /* figure out the center of the object in it's space*/
  center = rp_add(rp_cmult(0.5,roi_corner[1]), rp_cmult(0.5,roi_corner[0]));
  g_print("center %5.3f %5.3f %5.3f\n", center.x, center.y, center.z);
#endif
#ifdef CYLINDER_DEFINED
  /* figure out the height */
  height = fabs(roi_corner[1].z-roi_corner[0].z);
  g_print("height %5.3f\n",height);
#endif



  /* get the corners of the view slice in view coordinate space */
  slice_corner[0] = realspace_base_coord_to_alt(rs_offset(view_slice->coord_frame), view_slice->coord_frame);
  slice_corner[1] = view_slice->corner;

  /* find the first edge point */
  view_rp.z = (slice_corner[0].z+slice_corner[1].z)/2.0;
  view_rp.y = slice_corner[0].y;
  edge = FALSE;
  i.t = i.z = i.y = 0;
  while ((i.y <= view_slice->data_set->dim.y) && (!edge)) {

    view_rp.x = slice_corner[0].x;
    i.x = 0;
    while ((i.x <= view_slice->data_set->dim.x) && (!edge)) {

      temp_rp = realspace_alt_coord_to_alt(view_rp, view_slice->coord_frame, roi->coord_frame);
#ifdef BOX_DEFINED
      edge = realpoint_in_box(temp_rp, roi_corner[0],roi_corner[1]);
#endif
#ifdef CYLINDER_DEFINED
      edge = realpoint_in_elliptic_cylinder(temp_rp, center, height, radius);
#endif
#ifdef ELLIPSOID_DEFINED
      edge = realpoint_in_ellipsoid(temp_rp,center,radius);
#endif
#if (ISOCONTOUR_2D_DEFINED == 1 || ISOCONTOUR_3D_DEFINED)
      ROI_REALPOINT_TO_VOXEL(roi, temp_rp, roi_vp);
      if (data_set_includes_voxel(roi->isocontour, roi_vp))
	edge = *DATA_SET_UBYTE_POINTER(roi->isocontour, roi_vp);
      else
	edge = FALSE;
#endif
      if (edge) {
	if (i.x != 0) i.x--; 
	if (i.y != 0) i.y--; 
	temp_vp = i;
      }
      view_rp.x += view_slice->voxel_size.x;
      i.x++;
    }
    view_rp.y += view_slice->voxel_size.y;
    i.y++;
  }

  if (!edge)
    return NULL; /* no intersection points */

  g_print("voxel %d %d %d %d\n",
	  temp_vp.x,
	  temp_vp.y,
	  temp_vp.z,
	  temp_vp.t);
  
  /* find the intersection points based on our initial edge point */
  temp_ds = data_set_UBYTE_2D_init(FALSE, view_slice->data_set->dim.y, view_slice->data_set->dim.x);
  return_points = find_intersection_points(roi, view_slice, temp_ds, 
#ifdef BOX_DEFINED
					   roi_corner,
#endif
#ifdef CYLINDER_DEFINED
					   height,
#endif
#if (ELLIPSOID_DEFINED == 1 || CYLINDER_DEFINED == 1)
					   center,
					   radius,
#endif
					   temp_vp);
  temp_ds = data_set_free(temp_ds);

  /* add the initial edge point to the end of the list so that we have a continous line */
  VOLUME_VOXEL_TO_REALPOINT(view_slice, temp_vp, view_rp);
  return_points = add_intersection_point_last(return_points, view_rp);

  return return_points;
}

