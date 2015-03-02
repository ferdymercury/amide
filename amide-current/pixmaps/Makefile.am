

pixmapdir = $(datadir)/pixmaps

pixmap_DATA = \
	amide_logo.png \
	amide_file_logo.png

EXTRA_DIST  = $(pixmap_DATA) 


pixbuf_csource=$(GDK_PIXBUF_CSOURCE)
BUILT_SOURCES = \
	amide_logo.h \
	amide_logo_small.h \
	thresholding_per_slice_icon.h \
	thresholding_per_frame_icon.h \
	thresholding_interpolate_frames_icon.h \
	thresholding_global_icon.h \
	three_compartment.h \
	two_compartment.h \
	window_abdomen.h \
	window_brain.h \
	window_extremities.h \
	window_liver.h \
	window_lung.h \
	window_pelvis_soft_tissue.h \
	window_skull_base.h \
	window_spine_a.h \
	window_spine_b.h \
	window_thorax_soft_tissue.h \
	align_pt_icon.h \
	study_icon.h \
	box_icon.h \
	cylinder_icon.h \
	ellipsoid_icon.h \
	isocontour_2d_icon.h \
	isocontour_3d_icon.h \
	freehand_2d_icon.h \
	freehand_3d_icon.h \
	min_max_icon.h \
	center_width_icon.h
##	window_bone_icon.h \
##	window_soft_tissue_icon.h \




.png.h:
	$(pixbuf_csource) --raw --extern --name=$(subst .png,,$<) $< > $@

CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = *~


