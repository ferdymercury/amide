
INCLUDES = -DGNOMELOCALEDIR=\""$(datadir)/locale\"" -I/usr/local/include
CFLAGS = $(AMIDE_CFLAGS) @CFLAGS@ $(EXTRA_GNOME_CFLAGS) $(AMIDE_DEBUG_CFLAGS)

bin_PROGRAMS = amide

amide_SOURCES = \
	gtkdial.c \
	gtkdial.h \
	$(DATA_SET_VARIABLE_SRCS) \
	$(VOLUME_VARIABLE_SRCS) \
	amide.c \
	amide.h \
	amitk_threshold.c \
	amitk_threshold.h \
	color_table.c \
	color_table.h \
	cti_import.c \
	cti_import.h \
	data_set.c \
	data_set.h \
	idl_data_import.c \
	idl_data_import.h \
	image.c \
	image.h \
	medcon_import.c \
	medcon_import.h \
	pem_data_import.c \
	pem_data_import.h \
	raw_data.c \
	raw_data.h \
	raw_data_import.c \
	raw_data_import.h \
	raw_data_import_cb.c \
	raw_data_import_cb.h \
	realspace.c \
	realspace.h \
	rendering.c \
	rendering.h \
	roi.c \
	roi.h \
	study.c \
	study.h \
	ui_common.c \
	ui_common.h \
	ui_common_cb.c \
	ui_common_cb.h \
	ui_rendering.c \
	ui_rendering.h \
	ui_rendering_cb.c \
	ui_rendering_cb.h \
	ui_rendering_dialog.c \
	ui_rendering_dialog.h \
	ui_rendering_dialog_cb.c \
	ui_rendering_dialog_cb.h \
	ui_rendering_menus.c \
	ui_rendering_menus.h \
	ui_rendering_movie_dialog.c \
	ui_rendering_movie_dialog.h \
	ui_rendering_movie_dialog_cb.c \
	ui_rendering_movie_dialog_cb.h \
	ui_roi.c \
	ui_roi.h \
	ui_roi_dialog.c \
	ui_roi_dialog.h \
	ui_roi_dialog_cb.c \
	ui_roi_dialog_cb.h \
	ui_series.c \
	ui_series.h \
	ui_series_cb.c \
	ui_series_cb.h \
	ui_series_menus.c \
	ui_series_menus.h \
	ui_series_toolbar.c \
	ui_series_toolbar.h \
	ui_study.c \
	ui_study.h \
	ui_study_cb.c \
	ui_study_cb.h \
	ui_study_dialog.c \
	ui_study_dialog.h \
	ui_study_dialog_cb.c \
	ui_study_dialog_cb.h \
	ui_study_menus.c \
	ui_study_menus.h \
	ui_study_toolbar.c \
	ui_study_toolbar.h \
	ui_study_rois_cb.c \
	ui_study_rois_cb.h \
	ui_time_dialog.c \
	ui_time_dialog.h \
	ui_time_dialog_cb.c \
	ui_time_dialog_cb.h \
	ui_volume.c \
	ui_volume.h \
	ui_volume_dialog.c \
	ui_volume_dialog.h \
	ui_volume_dialog_cb.c \
	ui_volume_dialog_cb.h \
	volume.c \
	volume.h \
	xml.c \
	xml.h

amide_LDADD = \
	$(GNOMEUI_LIBS) \
	$(GNOME_LIBDIR) \
	$(GNOME_XML2_LIBS) \
	$(AMIDE_LIBECAT_LIBS) \
	$(AMIDE_LIBMDC_LIBS) \
	$(AMIDE_LIBVOLPACK_LIBS) \
	$(EXTRA_GNOME_LIBS) \
	-L/usr/local/lib 


DATA_SET_VARIABLE_SRCS = \
	data_set_UBYTE.c \
	data_set_UBYTE.h \
	data_set_SBYTE.c \
	data_set_SBYTE.h \
	data_set_USHORT.c \
	data_set_USHORT.h \
	data_set_SSHORT.c \
	data_set_SSHORT.h \
	data_set_UINT.c \
	data_set_UINT.h \
	data_set_SINT.c \
	data_set_SINT.h \
	data_set_FLOAT.c \
	data_set_FLOAT.h \
	data_set_DOUBLE.c \
	data_set_DOUBLE.h 

VOLUME_VARIABLE_SRCS = \
	volume_UBYTE_0D_SCALING.c \
	volume_UBYTE_0D_SCALING.h \
	volume_UBYTE_1D_SCALING.c \
	volume_UBYTE_1D_SCALING.h \
	volume_UBYTE_2D_SCALING.c \
	volume_UBYTE_2D_SCALING.h \
	volume_SBYTE_0D_SCALING.c \
	volume_SBYTE_0D_SCALING.h \
	volume_SBYTE_1D_SCALING.c \
	volume_SBYTE_1D_SCALING.h \
	volume_SBYTE_2D_SCALING.c \
	volume_SBYTE_2D_SCALING.h \
	volume_USHORT_0D_SCALING.c \
	volume_USHORT_0D_SCALING.h \
	volume_USHORT_1D_SCALING.c \
	volume_USHORT_1D_SCALING.h \
	volume_USHORT_2D_SCALING.c \
	volume_USHORT_2D_SCALING.h \
	volume_SSHORT_0D_SCALING.c \
	volume_SSHORT_0D_SCALING.h \
	volume_SSHORT_1D_SCALING.c \
	volume_SSHORT_1D_SCALING.h \
	volume_SSHORT_2D_SCALING.c \
	volume_SSHORT_2D_SCALING.h \
	volume_UINT_0D_SCALING.c \
	volume_UINT_0D_SCALING.h \
	volume_UINT_1D_SCALING.c \
	volume_UINT_1D_SCALING.h \
	volume_UINT_2D_SCALING.c \
	volume_UINT_2D_SCALING.h \
	volume_SINT_0D_SCALING.c \
	volume_SINT_0D_SCALING.h \
	volume_SINT_1D_SCALING.c \
	volume_SINT_1D_SCALING.h \
	volume_SINT_2D_SCALING.c \
	volume_SINT_2D_SCALING.h \
	volume_FLOAT_0D_SCALING.c \
	volume_FLOAT_0D_SCALING.h \
	volume_FLOAT_1D_SCALING.c \
	volume_FLOAT_1D_SCALING.h \
	volume_FLOAT_2D_SCALING.c \
	volume_FLOAT_2D_SCALING.h \
	volume_DOUBLE_0D_SCALING.c \
	volume_DOUBLE_0D_SCALING.h \
	volume_DOUBLE_1D_SCALING.c \
	volume_DOUBLE_1D_SCALING.h \
	volume_DOUBLE_2D_SCALING.c \
	volume_DOUBLE_2D_SCALING.h 

VARIABLE_INPUT = \
	variable_type.m4

DATA_SET_VARIABLE_INCLUDES = \
	data_set_variable_type.c \
	data_set_variable_type.h 

VOLUME_VARIABLE_INCLUDES = \
	volume_variable_type.c \
	volume_variable_type.h 

$(DATA_SET_VARIABLE_SRCS): $(VARIABLE_INPUT) $(DATA_SET_VARIABLE_INCLUDES)
	@for i in $(DATA_SET_VARIABLE_SRCS) ; \
            do \
                rm -f $$i ;\
                echo "$(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i" ;\
                $(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i; \
                chmod -w $$i; \
            done

$(VOLUME_VARIABLE_SRCS): $(VARIABLE_INPUT) $(VOLUME_VARIABLE_INCLUDES)
	@for i in $(VOLUME_VARIABLE_SRCS) ; \
            do \
                rm -f $$i ;\
                echo "$(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i" ;\
                $(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i; \
                chmod -w $$i; \
            done


CLEANFILES = $(VOLUME_VARIABLE_SRCS) $(DATA_SET_VARIABLE_SRCS)
DISTCLEANFILES = *~
