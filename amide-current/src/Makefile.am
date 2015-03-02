if AMIDE_OS_WIN32

AMIDE_LDADD_WIN32 = -lwsock32 ../win32/amiderc.o
AMIDE_LDFLAGS_WIN32 = -mwindows
#INCLUDES = \
#	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" 

else

AMIDE_LDADD_WIN32 = 
AMIDE_LDFLAGS_WIN32 = 

endif

INCLUDES = \
	-DAMIDE_PREFIX=\""$(prefix)"\"		\
	-DAMIDE_SYSCONFDIR=\""$(sysconfdir)"\"	\
	-DAMIDE_DATADIR=\""$(datadir)"\"	\
	-DAMIDE_LIBDIR=\""$(libdir)"\"			
##	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"  \

AM_CFLAGS = \
	$(OPTIMIZATION_CFLAGS) \
	$(GSL_CFLAGS) \
	$(LIBFAME_CFLAGS) \
	$(AMIDE_GTK_CFLAGS) \
	$(AMIDE_DEBUG_CFLAGS) \
	$(AMIDE_CHECK_OBSOLETE_CFLAGS) \
	$(AMIDE_LIBDCMDATA_CFLAGS) \
	-I/usr/local/include \
	$(XMEDCON_CFLAGS) \
	$(FFMPEG_CFLAGS) 



## c++ gets the same includes
AM_CXXFLAGS = $(AM_CFLAGS)

bin_PROGRAMS = amide

# for some reason, the -wsock32 added by AMIDE_LDADD_WIN32 on windows has
# to be behind the DCMTK stuff
amide_LDADD = \
	$(GSL_LIBS) \
	$(LIBFAME_LIBS) \
	$(AMIDE_LIBECAT_LIBS) \
	$(AMIDE_LIBVOLPACK_LIBS) \
	$(AMIDE_GTK_LIBS) \
	$(XMEDCON_LIBS) \
	$(FFMPEG_LIBS) \
	$(AMIDE_LIBDCMDATA_LIBS) \
	$(AMIDE_LDADD_WIN32) 

## 2007.10.28, gcc 3.4.4 the below may no longer be an issue, as 
## xmedcon can now be glib2/gtk2 based. But haven't tested
##
## 2005.01.08, gcc 3.3.3
## note, for some reason, if XMEDCON_LIBS is before AMIDE_GTK_LIBS,
## an executable is made that dumps core on startup....
## this is probably because xmedcon is linked with glib 1.2... while 
## amide needs glib 2.*, and something is clashing in name space



amide_LDFLAGS = \
	$(AMIDE_LDFLAGS_WIN32)

amide_SOURCES = \
	$(MARSHAL_SOURCES) \
	$(TYPE_BUILTINS_SOURCES) \
	$(AMITK_RAW_DATA_VARIABLE_H) \
	$(AMITK_RAW_DATA_VARIABLE_C) \
	$(AMITK_DATA_SET_VARIABLE_H) \
	$(AMITK_DATA_SET_VARIABLE_C) \
	$(AMITK_ROI_VARIABLE_H) \
	$(AMITK_ROI_VARIABLE_C) \
	$(AMITK_H_SOURCES) \
	amide.h \
	amide.c \
	amide_intl.h \
	amide_gconf.c \
	amide_gconf.h \
	amide_gnome.c \
	amide_gnome.h \
	amitk_common.c \
	amitk_common.h \
	amitk_canvas.c \
	amitk_canvas_object.c \
	amitk_color_table.c \
	amitk_color_table_menu.c \
	amitk_data_set.c \
	amitk_dial.c \
	amitk_fiducial_mark.c \
	amitk_filter.c \
	amitk_line_profile.c \
	amitk_object.c \
	amitk_object_dialog.c \
	amitk_point.c \
	amitk_preferences.c \
	amitk_progress_dialog.c \
	amitk_raw_data.c \
	amitk_roi.c \
	amitk_space.c \
	amitk_space_edit.c \
	amitk_study.c \
	amitk_threshold.c \
	amitk_tree_view.c \
	amitk_volume.c \
	amitk_window_edit.c \
	alignment_mutual_information.c \
	alignment_mutual_information.h \
	alignment_procrustes.c \
	alignment_procrustes.h \
	analysis.c \
	analysis.h \
	dcmtk_interface.cc \
	dcmtk_interface.h \
	fads.c \
	fads.h \
	image.c \
	image.h \
	legacy.c \
	legacy.h \
	libecat_interface.c \
	libecat_interface.h \
	libmdc_interface.c \
	libmdc_interface.h \
	mpeg_encode.c \
	mpeg_encode.h \
	pixmaps.c \
	pixmaps.h \
	raw_data_import.c \
	raw_data_import.h \
	render.c \
	render.h \
	tb_alignment.c \
	tb_alignment.h \
	tb_crop.c \
	tb_crop.h \
	tb_distance.c \
	tb_distance.h \
	tb_export_data_set.c \
	tb_export_data_set.h \
	tb_fads.c \
	tb_fads.h \
	tb_filter.c \
	tb_filter.h \
	tb_fly_through.c \
	tb_fly_through.h \
	tb_math.c \
	tb_math.h \
	tb_profile.c \
	tb_profile.h \
	tb_roi_analysis.c \
	tb_roi_analysis.h \
	ui_common.c \
	ui_common.h \
	ui_gate_dialog.c \
	ui_gate_dialog.h \
	ui_preferences_dialog.c \
	ui_preferences_dialog.h \
	ui_render.c \
	ui_render.h \
	ui_render_dialog.c \
	ui_render_dialog.h \
	ui_render_movie.c \
	ui_render_movie.h \
	ui_series.c \
	ui_series.h \
	ui_study.c \
	ui_study.h \
	ui_study_cb.c \
	ui_study_cb.h \
	ui_time_dialog.c \
	ui_time_dialog.h \
	xml.c \
	xml.h

AMITK_H_SOURCES = \
	amitk_canvas.h \
	amitk_canvas_object.h \
	amitk_color_table.h \
	amitk_color_table_menu.h \
	amitk_common.h \
	amitk_dial.h \
	amitk_data_set.h \
	amitk_fiducial_mark.h \
	amitk_filter.h \
	amitk_line_profile.h \
	amitk_object.h \
	amitk_object_dialog.h \
	amitk_point.h \
	amitk_preferences.h \
	amitk_progress_dialog.h \
	amitk_raw_data.h \
	amitk_roi.h \
	amitk_space_edit.h \
	amitk_space.h \
	amitk_study.h \
	amitk_threshold.h \
	amitk_tree_view.h \
	amitk_type.h \
	amitk_volume.h \
	amitk_window_edit.h 

AMITK_ROI_VARIABLE_C = \
	amitk_roi_ELLIPSOID.c \
	amitk_roi_CYLINDER.c \
	amitk_roi_BOX.c \
	amitk_roi_ISOCONTOUR_2D.c \
	amitk_roi_ISOCONTOUR_3D.c \
	amitk_roi_FREEHAND_2D.c \
	amitk_roi_FREEHAND_3D.c

AMITK_ROI_VARIABLE_H = \
	amitk_roi_ELLIPSOID.h \
	amitk_roi_CYLINDER.h \
	amitk_roi_BOX.h \
	amitk_roi_ISOCONTOUR_2D.h \
	amitk_roi_ISOCONTOUR_3D.h \
	amitk_roi_FREEHAND_2D.h \
	amitk_roi_FREEHAND_3D.h

AMITK_RAW_DATA_VARIABLE_C = \
	amitk_raw_data_UBYTE.c \
	amitk_raw_data_SBYTE.c \
	amitk_raw_data_USHORT.c \
	amitk_raw_data_SSHORT.c \
	amitk_raw_data_UINT.c \
	amitk_raw_data_SINT.c \
	amitk_raw_data_FLOAT.c \
	amitk_raw_data_DOUBLE.c 

AMITK_RAW_DATA_VARIABLE_H = \
	amitk_raw_data_UBYTE.h \
	amitk_raw_data_SBYTE.h \
	amitk_raw_data_USHORT.h \
	amitk_raw_data_SSHORT.h \
	amitk_raw_data_UINT.h \
	amitk_raw_data_SINT.h \
	amitk_raw_data_FLOAT.h \
	amitk_raw_data_DOUBLE.h 

AMITK_DATA_SET_VARIABLE_C = \
	amitk_data_set_UBYTE_0D_SCALING.c \
	amitk_data_set_UBYTE_1D_SCALING.c \
	amitk_data_set_UBYTE_2D_SCALING.c \
	amitk_data_set_SBYTE_0D_SCALING.c \
	amitk_data_set_SBYTE_1D_SCALING.c \
	amitk_data_set_SBYTE_2D_SCALING.c \
	amitk_data_set_USHORT_0D_SCALING.c \
	amitk_data_set_USHORT_1D_SCALING.c \
	amitk_data_set_USHORT_2D_SCALING.c \
	amitk_data_set_SSHORT_0D_SCALING.c \
	amitk_data_set_SSHORT_1D_SCALING.c \
	amitk_data_set_SSHORT_2D_SCALING.c \
	amitk_data_set_UINT_0D_SCALING.c \
	amitk_data_set_UINT_1D_SCALING.c \
	amitk_data_set_UINT_2D_SCALING.c \
	amitk_data_set_SINT_0D_SCALING.c \
	amitk_data_set_SINT_1D_SCALING.c \
	amitk_data_set_SINT_2D_SCALING.c \
	amitk_data_set_FLOAT_0D_SCALING.c \
	amitk_data_set_FLOAT_1D_SCALING.c \
	amitk_data_set_FLOAT_2D_SCALING.c \
	amitk_data_set_DOUBLE_0D_SCALING.c \
	amitk_data_set_DOUBLE_1D_SCALING.c \
	amitk_data_set_DOUBLE_2D_SCALING.c \
	amitk_data_set_UBYTE_0D_SCALING_INTERCEPT.c \
	amitk_data_set_UBYTE_1D_SCALING_INTERCEPT.c \
	amitk_data_set_UBYTE_2D_SCALING_INTERCEPT.c \
	amitk_data_set_SBYTE_0D_SCALING_INTERCEPT.c \
	amitk_data_set_SBYTE_1D_SCALING_INTERCEPT.c \
	amitk_data_set_SBYTE_2D_SCALING_INTERCEPT.c \
	amitk_data_set_USHORT_0D_SCALING_INTERCEPT.c \
	amitk_data_set_USHORT_1D_SCALING_INTERCEPT.c \
	amitk_data_set_USHORT_2D_SCALING_INTERCEPT.c \
	amitk_data_set_SSHORT_0D_SCALING_INTERCEPT.c \
	amitk_data_set_SSHORT_1D_SCALING_INTERCEPT.c \
	amitk_data_set_SSHORT_2D_SCALING_INTERCEPT.c \
	amitk_data_set_UINT_0D_SCALING_INTERCEPT.c \
	amitk_data_set_UINT_1D_SCALING_INTERCEPT.c \
	amitk_data_set_UINT_2D_SCALING_INTERCEPT.c \
	amitk_data_set_SINT_0D_SCALING_INTERCEPT.c \
	amitk_data_set_SINT_1D_SCALING_INTERCEPT.c \
	amitk_data_set_SINT_2D_SCALING_INTERCEPT.c \
	amitk_data_set_FLOAT_0D_SCALING_INTERCEPT.c \
	amitk_data_set_FLOAT_1D_SCALING_INTERCEPT.c \
	amitk_data_set_FLOAT_2D_SCALING_INTERCEPT.c \
	amitk_data_set_DOUBLE_0D_SCALING_INTERCEPT.c \
	amitk_data_set_DOUBLE_1D_SCALING_INTERCEPT.c \
	amitk_data_set_DOUBLE_2D_SCALING_INTERCEPT.c 

AMITK_DATA_SET_VARIABLE_H = \
	amitk_data_set_UBYTE_0D_SCALING.h \
	amitk_data_set_UBYTE_1D_SCALING.h \
	amitk_data_set_UBYTE_2D_SCALING.h \
	amitk_data_set_SBYTE_0D_SCALING.h \
	amitk_data_set_SBYTE_1D_SCALING.h \
	amitk_data_set_SBYTE_2D_SCALING.h \
	amitk_data_set_USHORT_0D_SCALING.h \
	amitk_data_set_USHORT_1D_SCALING.h \
	amitk_data_set_USHORT_2D_SCALING.h \
	amitk_data_set_SSHORT_0D_SCALING.h \
	amitk_data_set_SSHORT_1D_SCALING.h \
	amitk_data_set_SSHORT_2D_SCALING.h \
	amitk_data_set_UINT_0D_SCALING.h \
	amitk_data_set_UINT_1D_SCALING.h \
	amitk_data_set_UINT_2D_SCALING.h \
	amitk_data_set_SINT_0D_SCALING.h \
	amitk_data_set_SINT_1D_SCALING.h \
	amitk_data_set_SINT_2D_SCALING.h \
	amitk_data_set_FLOAT_0D_SCALING.h \
	amitk_data_set_FLOAT_1D_SCALING.h \
	amitk_data_set_FLOAT_2D_SCALING.h \
	amitk_data_set_DOUBLE_0D_SCALING.h \
	amitk_data_set_DOUBLE_1D_SCALING.h \
	amitk_data_set_DOUBLE_2D_SCALING.h 

VARIABLE_INPUT = \
	variable_type.m4

AMITK_ROI_VARIABLE_C_SRC = amitk_roi_variable_type.c 
AMITK_ROI_VARIABLE_H_SRC = amitk_roi_variable_type.h

AMITK_RAW_DATA_VARIABLE_C_SRC = amitk_raw_data_variable_type.c
AMITK_RAW_DATA_VARIABLE_H_SRC = amitk_raw_data_variable_type.h

AMITK_DATA_SET_VARIABLE_C_SRC = amitk_data_set_variable_type.c 
AMITK_DATA_SET_VARIABLE_H_SRC = amitk_data_set_variable_type.h 


$(AMITK_ROI_VARIABLE_H): $(VARIABLE_INPUT) $(AMITK_ROI_VARIABLE_H_SRC)
	@for i in $(AMITK_ROI_VARIABLE_H) ; \
            do \
                rm -f $$i ;\
                echo "$(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i" ;\
                $(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i; \
                chmod -w $$i; \
            done

$(AMITK_ROI_VARIABLE_C): $(VARIABLE_INPUT) $(AMITK_ROI_VARIABLE_C_SRC) $(AMITK_ROI_VARIABLE_H)
	@for i in $(AMITK_ROI_VARIABLE_C) ; \
            do \
                rm -f $$i ;\
                echo "$(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i" ;\
                $(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i; \
                chmod -w $$i; \
            done

$(AMITK_RAW_DATA_VARIABLE_C): $(VARIABLE_INPUT) $(AMITK_RAW_DATA_VARIABLE_C_SRC) $(AMITK_RAW_DATA_VARIABLE_H)
	@for i in $(AMITK_RAW_DATA_VARIABLE_C) ; \
            do \
                rm -f $$i ;\
                echo "$(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i" ;\
                $(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i; \
                chmod -w $$i; \
            done

$(AMITK_RAW_DATA_VARIABLE_H): $(VARIABLE_INPUT) $(AMITK_RAW_DATA_VARIABLE_H_SRC)
	@for i in $(AMITK_RAW_DATA_VARIABLE_H) ; \
            do \
                rm -f $$i ;\
                echo "$(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i" ;\
                $(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i; \
                chmod -w $$i; \
            done


$(AMITK_DATA_SET_VARIABLE_C): $(VARIABLE_INPUT) $(AMITK_DATA_SET_VARIABLE_C_SRC) $(AMITK_DATA_SET_VARIABLE_H)
	@for i in $(AMITK_DATA_SET_VARIABLE_C) ; \
            do \
                rm -f $$i ;\
                echo "$(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i" ;\
                $(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i; \
                chmod -w $$i; \
            done

$(AMITK_DATA_SET_VARIABLE_H): $(VARIABLE_INPUT) $(AMITK_DATA_SET_VARIABLE_H_SRC)
	@for i in $(AMITK_DATA_SET_VARIABLE_H) ; \
            do \
                rm -f $$i ;\
                echo "$(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i" ;\
                $(M4) -P -Dm4_SourceFile=$$i $(VARIABLE_INPUT) > $$i; \
                chmod -w $$i; \
            done

## $(amide_la_OBJECTS): $(MARSHAL_SOURCES)
## BUILT_SOURCES =	\
##	$(MARSHAL_SOURCES) \
##	$(TYPE_BUILTINS_SOURCES) \


MARSHAL_SOURCES = \
	amitk_marshal.c	\
	amitk_marshal.h 

amitk_marshal.h: amitk_marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=amitk_marshal > $@
amitk_marshal.c: amitk_marshal.list $(GLIB_GENMARSHAL) amitk_marshal.h
	$(GLIB_GENMARSHAL) $< --body --prefix=amitk_marshal > $@

TYPE_BUILTINS_SOURCES = \
	amitk_type_builtins.h \
	amitk_type_builtins.c 

TEMP_FILES = \
	xgen-atbh \
	xgen-atbc

STAMP_FILES = \
	stamp-amitk_type_builtins.h \
	stamp-amitk_type_builtins.c

## fancy rebuild rules stolen from the Makefile.am from gtk-2.0
$(srcdir)/amitk_type_builtins.h: stamp-amitk_type_builtins.h $(AMITK_DATA_SET_VARIABLE_H) $(AMITK_RAW_DATA_VARIABLE_H)
	@true
stamp-amitk_type_builtins.h: $(AMITK_H_SOURCES) Makefile
	( cd $(srcdir) && glib-mkenums \
		--fhead "#ifndef __AMITK_TYPE_BUILTINS_H__\n#define __AMITK_TYPE_BUILTINS_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define AMITK_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* __AMITK_TYPE_BUILTINS_H__ */" \
		$(AMITK_H_SOURCES))  >> xgen-atbh \
	&& (cmp -s xgen-atbh $(srcdir)/amitk_type_builtins.h || cp xgen-atbh $(srcdir)/amitk_type_builtins.h ) \
	&& rm -f xgen-atbh \
	&& echo timestamp > $(@F)



$(srcdir)/amitk_type_builtins.c: stamp-amitk_type_builtins.c
	@true
stamp-amitk_type_builtins.c: $(AMITK_H_SOURCES) Makefile amitk_type_builtins.h
	( cd $(srcdir) && glib-mkenums \
		--fhead "#include <gtk/gtk.h>\n#include \"amide_config.h\"\n#include \"amitk_common.h\"\n#include \"amitk_canvas.h\"\n#include \"amitk_data_set.h\"\n#include \"amitk_filter.h\"\n#include \"amitk_object.h\"\n#include \"amitk_point.h\"\n#include \"amitk_raw_data.h\"\n#include \"amitk_roi.h\"\n#include \"amitk_space.h\"\n#include \"amitk_threshold.h\"\n#include \"amitk_tree_view.h\"\n" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
		--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(AMITK_H_SOURCES) ) > xgen-atbc \
	&& cp xgen-atbc $(srcdir)/amitk_type_builtins.c  \
	&& rm -f xgen-atbc \
	&& echo timestamp > $(@F)




CLEANFILES = \
	$(AMITK_ROI_VARIABLE_C) \
	$(AMITK_ROI_VARIABLE_H) \
	$(AMITK_DATA_SET_VARIABLE_C) \
	$(AMITK_DATA_SET_VARIABLE_H) \
	$(AMITK_RAW_DATA_VARIABLE_C) \
	$(AMITK_RAW_DATA_VARIABLE_H) \
	$(MARSHAL_SOURCES) \
	$(TYPE_BUILTINS_SOURCES) \
	$(TEMP_FILES) \
	$(STAMP_FILES) 

DISTCLEANFILES = *~ 

