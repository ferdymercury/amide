INCLUDES = \
   -DGNOMELOCALEDIR=\""$(datadir)/locale\""  \
	-I/usr/local/include

bin_PROGRAMS = amide

amide_SOURCES = \
	amide.c \
	amide.h \
	color_table.c \
	color_table.h \
	color_table2.h \
	cti_import.c \
	cti_import.h \
	idl_data_import.c \
	idl_data_import.h \
	image.c \
	image.h \
	medcon_import.c \
	medcon_import.h \
	objects.c \
	objects.h \
	realspace.c \
	realspace.h \
	raw_data.c \
	raw_data.h \
	raw_data_import.c \
	raw_data_import.h \
	raw_data_import_callbacks.c \
	raw_data_import_callbacks.h \
	rendering.c \
	rendering.h \
	roi.c \
	roi.h \
	study.c \
	study.h \
	ui_main.c \
	ui_main.h \
	ui_main_callbacks.c \
	ui_main_callbacks.h \
	ui_main_menus.c \
	ui_main_menus.h \
	ui_rendering.c \
	ui_rendering.h \
	ui_rendering_callbacks.c \
	ui_rendering_callbacks.h \
	ui_rendering_dialog.c \
	ui_rendering_dialog.h \
	ui_rendering_dialog_callbacks.c \
	ui_rendering_dialog_callbacks.h \
	ui_roi.c \
	ui_roi.h \
	ui_roi_dialog.c \
	ui_roi_dialog.h \
	ui_roi_dialog_callbacks.c \
	ui_roi_dialog_callbacks.h \
	ui_series.c \
	ui_series.h \
	ui_series2.h \
	ui_series_callbacks.c \
	ui_series_callbacks.h \
	ui_study.c \
	ui_study.h \
	ui_study_callbacks.c \
	ui_study_callbacks.h \
	ui_study_dialog.c \
	ui_study_dialog.h \
	ui_study_dialog_callbacks.c \
	ui_study_dialog_callbacks.h \
	ui_study_menus.c \
	ui_study_menus.h \
	ui_study_rois_callbacks.c \
	ui_study_rois_callbacks.h \
	ui_threshold.c \
	ui_threshold.h \
	ui_threshold2.h \
	ui_threshold_callbacks.c \
	ui_threshold_callbacks.h \
	ui_time_dialog.c \
	ui_time_dialoh.h \
	ui_time_dialog_callbacks.c \
	ui_time_dialoh_callbacks.h \
	ui_volume.c \
	ui_volume.h \
	ui_volume_dialog.c \
	ui_volume_dialog.h \
	ui_volume_dialog_callbacks.c \
	ui_volume_dialog_callbacks.h \
	volume.c \
	volume.h \
	xml.c \
	xml.h

amide_LDADD = \
	$(GNOMEUI_LIBS) \
	$(GNOME_LIBDIR) \
	$(GNOME_XML2_LIBS) \
	$(INTLLIBS) \
	$(AMIDE_LIBECAT_LIBS) \
	$(AMIDE_LIBMDC_LIBS) \
	$(AMIDE_LIBVOLPACK_LIBS) \
	-L/usr/local/lib 

