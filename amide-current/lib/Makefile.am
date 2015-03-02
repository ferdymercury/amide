SUBDIRS = \
	lib_src

if BUILD_LIBAMIDE
INCLUDES = -DGNOMELOCALEDIR=\""$(datadir)/locale\"" -I/usr/local/include
CFLAGS = @CFLAGS@ $(EXTRA_GNOME_CFLAGS) $(AMIDE_DEBUG_CFLAGS) $(GNOME_XML2_CFLAGS)
lib_LTLIBRARIES = libamide.la
libamideincludedir = $(includedir)/amide

libamideinclude_HEADERS = \
	amide_export.h

libamide_la_SOURCES = \
	$(amide_SOURCES) \
	amide_export.c 
endif

libamide_la_LDFLAGS = -version-info 1:0:0



amide_SOURCES = \
	gtkdial.c \
	gtkdial.h \
	gtkdirsel.c \
	gtkdirsel.h \
	$(DATA_SET_VARIABLE_H) \
	$(DATA_SET_VARIABLE_C) \
	$(VOLUME_VARIABLE_H) \
	$(VOLUME_VARIABLE_C) \
	$(ROI_VARIABLE_H) \
	$(ROI_VARIABLE_C) \
	$(ANALYSIS_VARIABLE_H) \
	$(ANALYSIS_VARIABLE_C) \
	amide.c \
	amide.h \
	align_pt.c \
	align_pt.h \
	alignment.c \
	alignment.h \
	analysis.c \
	analysis.h \
	amitk_canvas.c \
	amitk_canvas.h \
	amitk_coord_frame_edit.c \
	amitk_coord_frame_edit.h \
	amitk_spin_button.c \
	amitk_spin_button.h \
	amitk_tree.c \
	amitk_tree.h \
	amitk_tree_item.c \
	amitk_tree_item.h \
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
	ui_align_pt_dialog.c \
	ui_align_pt_dialog.h \
	ui_align_pt_dialog_cb.c \
	ui_align_pt_dialog_cb.h \
	ui_alignment_dialog.c \
	ui_alignment_dialog.h \
	ui_alignment_dialog_cb.c \
	ui_alignment_dialog_cb.h \
	ui_common.c \
	ui_common.h \
	ui_common_cb.c \
	ui_common_cb.h \
	ui_preferences_dialog.c \
	ui_preferences_dialog.h \
	ui_preferences_dialog_cb.c \
	ui_preferences_dialog_cb.h \
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
	ui_roi_dialog.c \
	ui_roi_dialog.h \
	ui_roi_dialog_cb.c \
	ui_roi_dialog_cb.h \
	ui_roi_analysis_dialog.c \
	ui_roi_analysis_dialog.h \
	ui_roi_analysis_dialog_cb.c \
	ui_roi_analysis_dialog_cb.h \
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
	ui_time_dialog.c \
	ui_time_dialog.h \
	ui_time_dialog_cb.c \
	ui_time_dialog_cb.h \
	ui_volume_dialog.c \
	ui_volume_dialog.h \
	ui_volume_dialog_cb.c \
	ui_volume_dialog_cb.h \
	volume.c \
	volume.h \
	xml.c \
	xml.h

ANALYSIS_VARIABLE_C = \
	analysis_ELLIPSOID.c \
	analysis_CYLINDER.c \
	analysis_BOX.c \
	analysis_ISOCONTOUR_2D.c \
	analysis_ISOCONTOUR_3D.c 

ANALYSIS_VARIABLE_H = \
	analysis_ELLIPSOID.h \
	analysis_CYLINDER.h \
	analysis_BOX.h \
	analysis_ISOCONTOUR_2D.h \
	analysis_ISOCONTOUR_3D.h

ROI_VARIABLE_C = \
	roi_ELLIPSOID.c \
	roi_CYLINDER.c \
	roi_BOX.c \
	roi_ISOCONTOUR_2D.c \
	roi_ISOCONTOUR_3D.c 

ROI_VARIABLE_H = \
	roi_ELLIPSOID.h \
	roi_CYLINDER.h \
	roi_BOX.h \
	roi_ISOCONTOUR_2D.h \
	roi_ISOCONTOUR_3D.h

DATA_SET_VARIABLE_C = \
	data_set_UBYTE.c \
	data_set_SBYTE.c \
	data_set_USHORT.c \
	data_set_SSHORT.c \
	data_set_UINT.c \
	data_set_SINT.c \
	data_set_FLOAT.c \
	data_set_DOUBLE.c 

DATA_SET_VARIABLE_H = \
	data_set_UBYTE.h \
	data_set_SBYTE.h \
	data_set_USHORT.h \
	data_set_SSHORT.h \
	data_set_UINT.h \
	data_set_SINT.h \
	data_set_FLOAT.h \
	data_set_DOUBLE.h 

VOLUME_VARIABLE_C = \
	volume_UBYTE_0D_SCALING.c \
	volume_UBYTE_1D_SCALING.c \
	volume_UBYTE_2D_SCALING.c \
	volume_SBYTE_0D_SCALING.c \
	volume_SBYTE_1D_SCALING.c \
	volume_SBYTE_2D_SCALING.c \
	volume_USHORT_0D_SCALING.c \
	volume_USHORT_1D_SCALING.c \
	volume_USHORT_2D_SCALING.c \
	volume_SSHORT_0D_SCALING.c \
	volume_SSHORT_1D_SCALING.c \
	volume_SSHORT_2D_SCALING.c \
	volume_UINT_0D_SCALING.c \
	volume_UINT_1D_SCALING.c \
	volume_UINT_2D_SCALING.c \
	volume_SINT_0D_SCALING.c \
	volume_SINT_1D_SCALING.c \
	volume_SINT_2D_SCALING.c \
	volume_FLOAT_0D_SCALING.c \
	volume_FLOAT_1D_SCALING.c \
	volume_FLOAT_2D_SCALING.c \
	volume_DOUBLE_0D_SCALING.c \
	volume_DOUBLE_1D_SCALING.c \
	volume_DOUBLE_2D_SCALING.c 

VOLUME_VARIABLE_H = \
	volume_UBYTE_0D_SCALING.h \
	volume_UBYTE_1D_SCALING.h \
	volume_UBYTE_2D_SCALING.h \
	volume_SBYTE_0D_SCALING.h \
	volume_SBYTE_1D_SCALING.h \
	volume_SBYTE_2D_SCALING.h \
	volume_USHORT_0D_SCALING.h \
	volume_USHORT_1D_SCALING.h \
	volume_USHORT_2D_SCALING.h \
	volume_SSHORT_0D_SCALING.h \
	volume_SSHORT_1D_SCALING.h \
	volume_SSHORT_2D_SCALING.h \
	volume_UINT_0D_SCALING.h \
	volume_UINT_1D_SCALING.h \
	volume_UINT_2D_SCALING.h \
	volume_SINT_0D_SCALING.h \
	volume_SINT_1D_SCALING.h \
	volume_SINT_2D_SCALING.h \
	volume_FLOAT_0D_SCALING.h \
	volume_FLOAT_1D_SCALING.h \
	volume_FLOAT_2D_SCALING.h \
	volume_DOUBLE_0D_SCALING.h \
	volume_DOUBLE_1D_SCALING.h \
	volume_DOUBLE_2D_SCALING.h 

VARIABLE_INPUT = \
	variable_type.m4

ANALYSIS_VARIABLE_C_SRC = analysis_variable_type.c 
ANALYSIS_VARIABLE_H_SRC = analysis_variable_type.h 

ROI_VARIABLE_C_SRC = roi_variable_type.c 
ROI_VARIABLE_H_SRC = roi_variable_type.h

DATA_SET_VARIABLE_C_SRC = data_set_variable_type.c
DATA_SET_VARIABLE_H_SRC = data_set_variable_type.h

VOLUME_VARIABLE_C_SRC = volume_variable_type.c 
VOLUME_VARIABLE_H_SRC = volume_variable_type.h 

$(ANALYSIS_VARIABLE_C): $(VARIABLE_INPUT) $(ANALYSIS_VARIABLE_C_SRC) $(ANALYSIS_VARIABLE_H)
	@for i in $(ANALYSIS_VARIABLE_C) ; \
            do \
                rm -f $$i ;\
                echo "$(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i" ;\
                $(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i; \
                chmod -w $$i; \
            done

$(ANALYSIS_VARIABLE_H): $(VARIABLE_INPUT) $(ANALYSIS_VARIABLE_H_SRC)
	@for i in $(ANALYSIS_VARIABLE_H) ; \
            do \
                rm -f $$i ;\
                echo "$(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i" ;\
                $(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i; \
                chmod -w $$i; \
            done

$(ROI_VARIABLE_C): $(VARIABLE_INPUT) $(ROI_VARIABLE_C_SRC) $(ROI_VARIABLE_H)
	@for i in $(ROI_VARIABLE_C) ; \
            do \
                rm -f $$i ;\
                echo "$(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i" ;\
                $(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i; \
                chmod -w $$i; \
            done

$(ROI_VARIABLE_H): $(VARIABLE_INPUT) $(ROI_VARIABLE_H_SRC)
	@for i in $(ROI_VARIABLE_H) ; \
            do \
                rm -f $$i ;\
                echo "$(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i" ;\
                $(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i; \
                chmod -w $$i; \
            done

$(DATA_SET_VARIABLE_C): $(VARIABLE_INPUT) $(DATA_SET_VARIABLE_C_SRC) $(DATA_SET_VARIABLE_H)
	@for i in $(DATA_SET_VARIABLE_C) ; \
            do \
                rm -f $$i ;\
                echo "$(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i" ;\
                $(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i; \
                chmod -w $$i; \
            done

$(DATA_SET_VARIABLE_H): $(VARIABLE_INPUT) $(DATA_SET_VARIABLE_H_SRC)
	@for i in $(DATA_SET_VARIABLE_H) ; \
            do \
                rm -f $$i ;\
                echo "$(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i" ;\
                $(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i; \
                chmod -w $$i; \
            done


$(VOLUME_VARIABLE_C): $(VARIABLE_INPUT) $(VOLUME_VARIABLE_C_SRC) $(VOLUME_VARIABLE_H)
	@for i in $(VOLUME_VARIABLE_C) ; \
            do \
                rm -f $$i ;\
                echo "$(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i" ;\
                $(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i; \
                chmod -w $$i; \
            done

$(VOLUME_VARIABLE_H): $(VARIABLE_INPUT) $(VOLUME_VARIABLE_H_SRC)
	@for i in $(VOLUME_VARIABLE_H) ; \
            do \
                rm -f $$i ;\
                echo "$(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i" ;\
                $(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i; \
                chmod -w $$i; \
            done


CLEANFILES = \
	$(ANALYSIS_VARIABLE_C) \
	$(ANALYSIS_VARIABLE_H) \
	$(ROI_VARIABLE_C) \
	$(ROI_VARIABLE_H) \
	$(VOLUME_VARIABLE_C) \
	$(VOLUME_VARIABLE_H) \
	$(DATA_SET_VARIABLE_C) \
	$(DATA_SET_VARIABLE_H)

DISTCLEANFILES = \
        $(libamide_la_SOURCES) \
        $(libamideinclude_HEADERS) \
        $(VARIABLE_INPUT) \
	$(ANALYSIS_VARIABLE_C_SRC) \
	$(ANALYSIS_VARIABLE_H_SRC) \
	$(ROI_VARIABLE_C_SRC) \
	$(ROI_VARIABLE_H_SRC) \
	$(VOLUME_VARIABLE_C_SRC) \
	$(VOLUME_VARIABLE_H_SRC) \
	$(DATA_SET_VARIABLE_C_SRC) \
	$(DATA_SET_VARIABLE_H_SRC) \
        *~
