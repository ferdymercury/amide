
INCLUDES = \
	-DPREFIX=\""$(prefix)"\"			\
	-DSYSCONFDIR=\""$(sysconfdir)"\"		\
	-DDATADIR=\""$(datadir)"\"			\
	-DLIBDIR=\""$(libdir)"\"			
#	-DGNOMELOCALEDIR=\""$(datadir)/locale\""

CFLAGS = \
	@CFLAGS@ \
	$(AMIDE_GNOME_CFLAGS) \
	$(AMIDE_DEBUG_CFLAGS) \
	 -DGTK_DISABLE_DEPRECATED 
#	-DGNOME_DISABLE_DEPRECATED

bin_PROGRAMS = amide

amide_LDADD = \
	$(AMIDE_LIBGSL_LIBS) \
	$(AMIDE_LIBECAT_LIBS) \
	$(AMIDE_LIBMDC_LIBS) \
	$(AMIDE_LIBVOLPACK_LIBS) \
	$(AMIDE_GNOME_LIBS) \
	-L/usr/local/lib 


amide_SOURCES = \
	gtkdial.c \
	gtkdial.h \
	$(MARSHAL_SOURCES) \
	$(TYPE_BUILTINS_SOURCES) \
	$(AMITK_RAW_DATA_VARIABLE_H) \
	$(AMITK_RAW_DATA_VARIABLE_C) \
	$(AMITK_DATA_SET_VARIABLE_H) \
	$(AMITK_DATA_SET_VARIABLE_C) \
	$(AMITK_ROI_VARIABLE_H) \
	$(AMITK_ROI_VARIABLE_C) \
	$(AMITK_H_SOURCES) \
	amide.c \
	amide_limits.h \
	alignment.c \
	alignment.h \
	analysis.c \
	analysis.h \
	amitk_canvas.c \
	amitk_color_table.c \
	amitk_data_set.c \
	amitk_dir_sel.c \
	amitk_fiducial_mark.c \
	amitk_filter.c \
	amitk_point.c \
	amitk_object.c \
	amitk_object_dialog.c \
	amitk_raw_data.c \
	amitk_roi.c \
	amitk_space_edit.c \
	amitk_space.c \
	amitk_study.c \
	amitk_threshold.c \
	amitk_tree.c \
	amitk_volume.c \
	cti_import.c \
	cti_import.h \
	idl_data_import.c \
	idl_data_import.h \
	image.c \
	image.h \
	legacy.c \
	legacy.h \
	medcon_import.c \
	medcon_import.h \
	mpeg_encode.c \
	mpeg_encode.h \
	pixmaps.c \
	pixmaps.h \
	raw_data_import.c \
	raw_data_import.h \
	rendering.c \
	rendering.h \
	ui_alignment_dialog.c \
	ui_alignment_dialog.h \
	ui_common.c \
	ui_common.h \
	ui_crop_dialog.c \
	ui_crop_dialog.h \
	ui_filter_dialog.c \
	ui_filter_dialog.h \
	ui_fly_through.c \
	ui_fly_through.h \
	ui_preferences_dialog.c \
	ui_preferences_dialog.h \
	ui_rendering.c \
	ui_rendering.h \
	ui_rendering_dialog.c \
	ui_rendering_dialog.h \
	ui_rendering_movie_dialog.c \
	ui_rendering_movie_dialog.h \
	ui_roi_analysis_dialog.c \
	ui_roi_analysis_dialog.h \
	ui_series.c \
	ui_series.h \
	ui_study.c \
	ui_study.h \
	ui_study_cb.c \
	ui_study_cb.h \
	ui_study_menus.c \
	ui_study_menus.h \
	ui_study_toolbar.c \
	ui_study_toolbar.h \
	ui_time_dialog.c \
	ui_time_dialog.h \
	xml.c \
	xml.h

AMITK_H_SOURCES = \
	amide.h \
	amitk_canvas.h \
	amitk_color_table.h \
	amitk_dir_sel.h \
	amitk_data_set.h \
	amitk_fiducial_mark.h \
	amitk_filter.h \
	amitk_object.h \
	amitk_object_dialog.h \
	amitk_point.h \
	amitk_raw_data.h \
	amitk_roi.h \
	amitk_space_edit.h \
	amitk_space.h \
	amitk_study.h \
	amitk_threshold.h \
	amitk_tree.h \
	amitk_volume.h 

AMITK_ROI_VARIABLE_C = \
	amitk_roi_ELLIPSOID.c \
	amitk_roi_CYLINDER.c \
	amitk_roi_BOX.c \
	amitk_roi_ISOCONTOUR_2D.c \
	amitk_roi_ISOCONTOUR_3D.c 

AMITK_ROI_VARIABLE_H = \
	amitk_roi_ELLIPSOID.h \
	amitk_roi_CYLINDER.h \
	amitk_roi_BOX.h \
	amitk_roi_ISOCONTOUR_2D.h \
	amitk_roi_ISOCONTOUR_3D.h

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
	amitk_data_set_DOUBLE_2D_SCALING.c 

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
		--fhead "#include <gtk/gtk.h>\n#include \"amide_config.h\"\n#include \"amitk_canvas.h\"\n#include \"amitk_data_set.h\"\n#include \"amitk_filter.h\"\n#include \"amitk_object.h\"\n#include \"amitk_point.h\"\n#include \"amitk_raw_data.h\"\n#include \"amitk_roi.h\"\n#include \"amitk_space.h\"\n#include \"amitk_threshold.h\"\n" \
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

