

pixmapdir = $(datadir)/pixmaps

pixmap_DATA = \
	amide_logo.png \
	amide_file_logo.png

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


