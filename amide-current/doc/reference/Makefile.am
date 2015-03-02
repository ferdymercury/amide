## Process this file with automake to produce Makefile.in

# The name of the module.
DOC_MODULE=amitk

# The top-level SGML file.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# Extra options to supply to gtkdoc-scan
#SCAN_OPTIONS=--deprecated-guards="GTK_ENABLE_BROKEN|GTK_DISABLE_DEPRECATED" 

# The directory containing the source code. Relative to $(srcdir)
DOC_SOURCE_DIR=../../src

# Extra options to pass to gtkdoc-scangobj
# SCANGOBJ_OPTIONS=--type-init-func="gtk_type_init(0)" 
SCANGOBJ_OPTIONS=

# Extra options to supply to gtkdoc-mkdb
#MKDB_OPTIONS=--main-sgml-file=$(DOC_MAIN_SGML_FILE) --sgml-mode --output-format=xml
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-mktmpl
# e.g. MKTMPL_OPTIONS=--only-section-tmpl
MKTMPL_OPTIONS=

# Extra options to supply to gtkdoc-fixref
#FIXXREF_OPTIONS=--extra-dir=../gdk-pixbuf/html --extra-dir=../gdk/html
FIXREF_OPTIONS=

# Used for dependencies
HFILE_GLOB=$(top_srcdir)/src/*.h
CFILE_GLOB=$(top_srcdir)/src/*.c

# Header files to ignore when scanning
IGNORE_HFILES=			\
	alignment_mutal_information.h		\
	alignment_procrustes.h	\
	amide_intl.h		\
	amitk_dial.h            \
	analysis.h		\
	dcmtk_interface.h	\
	fads.h			\
	image.h			\
	legacy.h		\
	libecat_interface.h 	\
	libmdc_interface.h 	\
	mpeg_encode.h		\
	pixmaps.h		\
	raw_data_import.h	\
	render.h		\
	tb_alignment.h		\
	tb_crop.h		\
	tb_fads.h		\
	tb_filter.h		\
	tb_fly_through.h	\
	tb_math.h		\
	tb_profile.h		\
	tb_roi_analysis.h	\
	ui_common.h   		\
	ui_gate_dialog.h	\
	ui_preferences_dialog.h	\
	ui_render.h          	\
	ui_render_dialog.h	\
	ui_render_movie.h 	\
	ui_series.h     	\
	ui_study.h         	\
	ui_study_cb.h		\
	ui_study_menus.h        \
	ui_time_dialog.h	\
	xml.h

ALL_OBJ_FILES= \
	 $(shell ls ${top_srcdir}/src/*.o)

IGNORE_OBJ_FILES= \
	${top_srcdir}/src/amide.o 

OBJ_FILES= $(filter-out $(IGNORE_OBJ_FILES),$(ALL_OBJ_FILES))

# Images to copy into HTML directory
HTML_IMAGES = 

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE)
content_files = \
	version.xml \
	theory.sgml

# CFLAGS and LDFLAGS for compiling gtkdoc-scangobj with your library.
# Only needed if you are using gtkdoc-scangobj to dynamically query widget
# signals and properties.
GTKDOC_CFLAGS=	\
	-I$(top_srcdir)/src	\
	-I$(top_builddir)/src \
	-I$(top_srcdir)		\
	-I$(top_builddir)	\
	$(GSL_CFLAGS) \
	$(LIBFAME_CFLAGS) \
	$(AMIDE_GTK_CFLAGS) \
	$(AMIDE_DEBUG_CFLAGS) \
	-I/usr/local/include \
	$(XMEDCON_CFLAGS) \
	-DG_DISABLE_DEPRECATED  \
	-UAMIDE_LIBDCMDATA_SUPPORT

GTKDOC_LIBS=	\
	$(GSL_LIBS) \
	$(LIBFAME_LIBS) \
	$(AMIDE_LIBECAT_LIBS) \
	$(AMIDE_LIBVOLPACK_LIBS) \
	$(AMIDE_GTK_LIBS) \
	$(XMEDCON_LIBS) \
	$(FFMPEG_LIBS) \
	$(AMIDE_LIBDCMDATA_LIBS) \
	$(AMIDE_LDADD_WIN32) \
	$(OBJ_FILES)


# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
INCLUDES = \
	@CFLAGS@					\
	-I$(top_srcdir) 				\
	-I$(top_builddir) 				\
	-DAMIDE_LIBECAT_SUPPORT                         \
	-DAMIDE_LIBMDC_SUPPORT                          

# Extra options to supply to gtkdoc-fixref
# FIXXREF_OPTIONS=--extra-dir=../gdk-pixbuf/html

include $(top_srcdir)/gtk-doc.make

# Other files to distribute
EXTRA_DIST += version.xml.in

CLEANFILES = \
	tmpl/* \
	xml/*

DISTCLEANFILES = \
	*~ \
	amitk-sections.txt \
	amitk-undocumented.txt \
	amitk-decl.txt \
	amitk-decl-list.txt \
	amitk-scan.c \
	amitk-scan.o \
	amitk.types

