

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
	window_bone_icon.h \
	window_soft_tissue_icon.h \
	align_pt_icon.h \
	study_icon.h \
	box_icon.h \
	cylinder_icon.h \
	ellipsoid_icon.h \
	isocontour_2d_icon.h \
	isocontour_3d_icon.h 




.png.h:
	$(pixbuf_csource) --raw --extern --name=$(subst .png,,$<) $< > $@

CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = *~


