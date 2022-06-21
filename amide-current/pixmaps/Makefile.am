

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

EXTRA_DIST  = $(pixmap_DATA)


pixbuf_csource=$(GDK_PIXBUF_CSOURCE)
BUILT_SOURCES = \
	align_pt.h \
	amide_logo.h \
	fuse_type_blend.h \
	fuse_type_overlay.h \
	interpolation_nearest_neighbor.h \
	interpolation_trilinear.h \
	layout_linear.h \
	layout_orthogonal.h \
	panels_linear_x.h \
	panels_linear_y.h \
	panels_mixed.h \
	roi_box.h \
	roi_cylinder.h \
	roi_ellipsoid.h \
	roi_isocontour_2d.h \
	roi_isocontour_3d.h \
	roi_freehand_2d.h \
	roi_freehand_3d.h \
	study.h \
	target.h \
	threshold_style_min_max.h \
	threshold_style_center_width.h \
	thresholding.h \
	thresholding_per_slice.h \
	thresholding_per_frame.h \
	thresholding_interpolate_frames.h \
	thresholding_global.h \
	three_compartment.h \
	transfer_function.h \
	two_compartment.h \
	view_transverse.h \
	view_coronal.h \
	view_sagittal.h \
	view_single.h \
	view_linked.h \
	view_linked3.h \
	window_abdomen.h \
	window_brain.h \
	window_extremities.h \
	window_liver.h \
	window_lung.h \
	window_pelvis_soft_tissue.h \
	window_skull_base.h \
	window_spine_a.h \
	window_spine_b.h \
	window_thorax_soft_tissue.h

##	window_bone_icon.h \
##	window_soft_tissue_icon.h \



.png.h:
	$(pixbuf_csource) --raw --extern --name=$(subst .png,,$<) $< > $@

CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = *~


