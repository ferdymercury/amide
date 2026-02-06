

pixmapdir = $(datadir)/pixmaps

pixmap_DATA = \
    align_pt.png \
    amide_file_logo.png \
    amide_logo.png \
    fuse_type_blend.png \
    fuse_type_overlay.png \
    interpolation_nearest_neighbor.png \
    interpolation_trilinear.png \
    layout_linear.png \
    layout_orthogonal.png \
    panels_linear_x.png \
    panels_linear_y.png \
    panels_mixed.png \
    roi_box.png \
    roi_cylinder.png \
    roi_ellipsoid.png \
    roi_freehand_2d.png \
    roi_freehand_3d.png \
    roi_isocontour_2d.png \
    roi_isocontour_3d.png \
    study.png \
    target.png \
    three_compartment.png \
    threshold_style_center_width.png \
    threshold_style_min_max.png \
    thresholding.png \
    thresholding_global.png \
    thresholding_interpolate_frames.png \
    thresholding_per_frame.png \
    thresholding_per_slice.png \
    transfer_function.png \
    two_compartment.png \
    view_coronal.png \
    view_linked.png \
    view_linked3.png \
    view_sagittal.png \
    view_single.png \
    view_transverse.png \
    window_abdomen.png \
    window_bone.png \
    window_brain.png \
    window_extremities.png \
    window_liver.png \
    window_lung.png \
    window_pelvis_soft_tissue.png \
    window_skull_base.png \
    window_soft_tissue.png \
    window_spine_a.png \
    window_spine_b.png \
    window_thorax_soft_tissue.png

EXTRA_DIST = $(pixmap_DATA) \
	     icons.xml \
	     align_pt.png \
	     amide_logo.png \
	     fuse_type_blend.png \
	     fuse_type_overlay.png \
	     interpolation_nearest_neighbor.png \
	     interpolation_trilinear.png \
	     layout_linear.png \
	     layout_orthogonal.png \
	     panels_linear_x.png \
	     panels_linear_y.png \
	     panels_mixed.png \
	     roi_box.png \
	     roi_cylinder.png \
	     roi_ellipsoid.png \
	     roi_isocontour_2d.png \
	     roi_isocontour_3d.png \
	     roi_freehand_2d.png \
	     roi_freehand_3d.png \
	     study.png \
	     target.png \
	     threshold_style_min_max.png \
	     threshold_style_center_width.png \
	     thresholding.png \
	     thresholding_per_slice.png \
	     thresholding_per_frame.png \
	     thresholding_interpolate_frames.png \
	     thresholding_global.png \
	     three_compartment.png \
	     transfer_function.png \
	     two_compartment.png \
	     view_transverse.png \
	     view_coronal.png \
	     view_sagittal.png \
	     view_single.png \
	     view_linked.png \
	     view_linked3.png \
	     window_abdomen.png \
	     window_brain.png \
	     window_extremities.png \
	     window_liver.png \
	     window_lung.png \
	     window_pelvis_soft_tissue.png \
	     window_skull_base.png \
	     window_spine_a.png \
	     window_spine_b.png \
	     window_thorax_soft_tissue.png


compile_resources = $(GLIB_COMPILE_RESOURCES)
BUILT_SOURCES = icons.c

##	window_bone_icon.h \
##	window_soft_tissue_icon.h \



icons.c: icons.xml
	$(compile_resources) --generate-source $<

CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = *~


