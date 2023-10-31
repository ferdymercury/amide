amide_sources = files(
  'amide.h',
  'amide.c',
  'amide_intl.h',
  'amide_gconf.c',
  'amide_gconf.h',
  'amide_gnome.c',
  'amide_gnome.h',
  'amitk_common.c',
  'amitk_common.h',
  'amitk_canvas.c',
  'amitk_canvas_object.c',
  'amitk_color_table.c',
  'amitk_color_table_menu.c',
  'amitk_data_set.c',
  'amitk_dial.c',
  'amitk_fiducial_mark.c',
  'amitk_filter.c',
  'amitk_line_profile.c',
  'amitk_object.c',
  'amitk_object_dialog.c',
  'amitk_point.c',
  'amitk_preferences.c',
  'amitk_progress_dialog.c',
  'amitk_raw_data.c',
  'amitk_roi.c',
  'amitk_space.c',
  'amitk_space_edit.c',
  'amitk_study.c',
  'amitk_threshold.c',
  'amitk_tree_view.c',
  'amitk_volume.c',
  'amitk_window_edit.c',
  'alignment_mutual_information.c',
  'alignment_mutual_information.h',
  'alignment_procrustes.c',
  'alignment_procrustes.h',
  'analysis.c',
  'analysis.h',
  'dcmtk_interface.cc',
  'dcmtk_interface.h',
  'fads.c',
  'fads.h',
  'image.c',
  'image.h',
  'legacy.c',
  'legacy.h',
  'libecat_interface.c',
  'libecat_interface.h',
  'libmdc_interface.c',
  'libmdc_interface.h',
  'mpeg_encode.c',
  'mpeg_encode.h',
  'pixmaps.c',
  'pixmaps.h',
  'raw_data_import.c',
  'raw_data_import.h',
  'render.c',
  'render.h',
  'tb_alignment.c',
  'tb_alignment.h',
  'tb_crop.c',
  'tb_crop.h',
  'tb_distance.c',
  'tb_distance.h',
  'tb_export_data_set.c',
  'tb_export_data_set.h',
  'tb_fads.c',
  'tb_fads.h',
  'tb_filter.c',
  'tb_filter.h',
  'tb_fly_through.c',
  'tb_fly_through.h',
  'tb_math.c',
  'tb_math.h',
  'tb_profile.c',
  'tb_profile.h',
  'tb_roi_analysis.c',
  'tb_roi_analysis.h',
  'ui_common.c',
  'ui_common.h',
  'ui_gate_dialog.c',
  'ui_gate_dialog.h',
  'ui_preferences_dialog.c',
  'ui_preferences_dialog.h',
  'ui_render.c',
  'ui_render.h',
  'ui_render_dialog.c',
  'ui_render_dialog.h',
  'ui_render_movie.c',
  'ui_render_movie.h',
  'ui_series.c',
  'ui_series.h',
  'ui_study.c',
  'ui_study.h',
  'ui_study_cb.c',
  'ui_study_cb.h',
  'ui_time_dialog.c',
  'ui_time_dialog.h',
  'vistaio_interface.c',
  'vistaio_interface.h',
  'xml.c',
  'xml.h',
)

amitk_h_sources = files(
  'amitk_canvas.h',
  'amitk_canvas_object.h',
  'amitk_color_table.h',
  'amitk_color_table_menu.h',
  'amitk_common.h',
  'amitk_dial.h',
  'amitk_data_set.h',
  'amitk_fiducial_mark.h',
  'amitk_filter.h',
  'amitk_line_profile.h',
  'amitk_object.h',
  'amitk_object_dialog.h',
  'amitk_point.h',
  'amitk_preferences.h',
  'amitk_progress_dialog.h',
  'amitk_raw_data.h',
  'amitk_roi.h',
  'amitk_space_edit.h',
  'amitk_space.h',
  'amitk_study.h',
  'amitk_threshold.h',
  'amitk_tree_view.h',
  'amitk_type.h',
  'amitk_volume.h',
  'amitk_window_edit.h',
)

amide_sources += amitk_h_sources

shapes = [
'ELLIPSOID',
'CYLINDER',
'BOX',
'ISOCONTOUR_2D',
'ISOCONTOUR_3D',
'FREEHAND_2D',
'FREEHAND_3D',
]

amitk_roi_variable_c = files ()
amitk_roi_variable_h = files ()

foreach shape : shapes
	amitk_roi_variable_c += configure_file(
		output: f'amitk_roi_@shape@.c',
		input: 'amitk_roi_variable_type.c',
		configuration: {'VARIABLE_TYPE' : shape}
	)
	amitk_roi_variable_h += configure_file(
		output: f'amitk_roi_@shape@.h',
		input: 'amitk_roi_variable_type.h',
		configuration: {'VARIABLE_TYPE' : shape}
	)
endforeach

amide_sources += amitk_roi_variable_c + amitk_roi_variable_h

types = [
	'UBYTE',
	'SBYTE',
	'USHORT',
	'SSHORT',
	'UINT',
	'SINT',
	'FLOAT',
	'DOUBLE',
]

dimensions = ['0D', '1D', '2D']

modes = ['SCALING', 'SCALING_INTERCEPT']

amitk_raw_data_variable_c = files()
amitk_raw_data_variable_h = files()
amitk_data_set_variable_c = files()
amitk_data_set_variable_h = files()

foreach type : types
	amitk_raw_data_variable_c += configure_file(
		output: f'amitk_raw_data_@type@.c',
		input: 'amitk_raw_data_variable_type.c',
		configuration: {'VARIABLE_TYPE' : type}
	)
	amitk_raw_data_variable_h += configure_file(
  		output: f'amitk_raw_data_@type@.h',
  		input: 'amitk_raw_data_variable_type.h',
  		configuration: {'VARIABLE_TYPE' : type}
  	)
	foreach dimension : dimensions
		foreach mode : modes
			amitk_data_set_variable_c+= configure_file(
 				output: f'amitk_data_set_@type@_@dimension@_@mode@.c',
 				input: 'amitk_data_set_variable_type.c',
 				configuration: {'VARIABLE_TYPE' : type, 'SCALE_DIM':dimension, 'INTERCEPT': mode}
 			)
		endforeach
		amitk_data_set_variable_h+= configure_file(
				output: f'amitk_data_set_@type@_@dimension@_SCALING.h',
				input: 'amitk_data_set_variable_type.h',
				configuration: {'VARIABLE_TYPE' : type, 'SCALE_DIM':dimension}
		)
	endforeach

endforeach

amide_sources += amitk_raw_data_variable_c + amitk_raw_data_variable_h
amide_sources += amitk_data_set_variable_c + amitk_data_set_variable_h

amide_sources += gnome.genmarshal('amitk_marshal',
	sources: ['amitk_marshal.list'],
	#install_header: true,
	prefix: 'amitk_marshal',
)

amide_sources += gnome.mkenums_simple('amitk_type_builtins', sources: amitk_h_sources)

#Mandatory deps
amide_deps = [
	dependency('glib-2.0', version: '>=2.56.0'),
	dependency('gobject-2.0', version: '>=2.56.0'),
	dependency('gtk+-2.0', version: '>=2.24.0'),
	dependency('libxml-2.0', version: '>=2.9.13'),
	dependency('libgnomecanvas-2.0', version: '>= 2.30.3'),
	dependency('intl'),
	dependency('threads', required: false),
]
#We do want canvas by default.
config_h.set10('AMIDE_LIBGNOMECANVAS_AA', true)

#FFMPEG Dependency
ffmpeg_opt = get_option('use_ffmpeg')
if not ffmpeg_opt.disabled()
	ffmpeg_dep = dependency('libavcodec', version: '>=51.45.0')
	ffmpeg_util_dep = dependency('libavutil', required: false)
	config_h.set10('AMIDE_FFMPEG_SUPPORT', ffmpeg_dep.found())
	amide_deps += [ffmpeg_dep, ffmpeg_util_dep]
else
	config_h.set10('AMIDE_FFMPEG_SUPPORT', false)
endif

#Vistaio Library Dependency
vistaio_opt = get_option('use_vistaio')
if not vistaio_opt.disabled()
	vistaio_dep = dependency('libvistaio', version: '>=1.2.17',required: vistaio_opt)
	config_h.set10('AMIDE_VISTAIO_SUPPORT', vistaio_dep.found())
	amide_deps += vistaio_dep
else
	config_h.set10('AMIDE_VISTAIO_SUPPORT', false)
endif

#OpenJPEG2000 Library Dependency
openjp2_opt = get_option('use_jpeg2000')
if not openjp2_opt.disabled()
	openjp2_dep = dependency('libopenjp2', version:'>=2.1.0', required: openjp2_opt)
	config_h.set10('AMIDE_LIBOPENJP2_SUPPORT', openjp2_dep.found())
	amide_deps += openjp2_dep
else
	config_h.set10('AMIDE_LIBOPENJP2_SUPPORT', false)
endif


#GNOME-VFS Dependency
gnome_vfs_opt = get_option('use_gnome_vfs')
if not gnome_vfs_opt.disabled()
	gnome_vfs_dep = dependency('gnome-vfs-2.0', version: '>=2.16.0', required: gnome_vfs_opt )
	config_h.set10('AMIDE_USE_GNOME_VFS', gnome_vfs_dep.found())
	amide_deps += gnome_vfs_dep
else
	config_h.set('AMIDE_USE_GNOME_VFS', false)
endif

#GNU Scientific Library Dependency
gsl_opt = get_option('use_gsl')
if not gsl_opt.disabled()
	gsl_dep = dependency('gsl', 'libgsl', required: gsl_opt )
	config_h.set10('AMIDE_LIBGSL_SUPPORT', gsl_dep.found())
	amide_deps += gsl_dep
else
	config_h.set10('AMIDE_LIBGSL_SUPPORT', false)
endif

#Volpack Library Dependency
volpack_opt = get_option('use_volpack')
if not volpack_opt.disabled()
	volpack_dep = dependency('libvolpack-1', 'libvolpack', required: false)

	if not volpack_dep.found()
		volpack_dep = cc.find_library('libvolpack', has_headers: ['volpack.h'], required: false)
	endif
	config_h.set10('AMIDE_LIBVOLPACK_SUPPORT', volpack_dep.found())
	amide_deps += volpack_dep
else
	config_h.set10('AMIDE_LIBVOLPACK_SUPPORT', false)
endif


# MDC Library Dependency
mdc_opt = get_option('use_mdc')
if not mdc_opt.disabled()
	mdc_dep = dependency('libmdc', required: false)

	if not mdc_dep.found()
		xmedcon = find_program('xmedcon-config', required: false)
        if xmedcon.found()
			mdc_dep = cc.find_library('libmdc',
			has_headers:['medcon.h'],
			header_args: run_command('xmedcon-config', '--cflags', check: true).stdout().split() + run_command('xmedcon-config', '--libs', check:true).stdout().split(),
			required: false)
		endif
	endif
	config_h.set10('AMIDE_LIBMDC_SUPPORT', mdc_dep.found())
	amide_deps += mdc_dep
else
	config_h.set10('AMIDE_LIBMDC_SUPPORT', false)
endif

#DCMTK Library Dependency
dcmtk_opt = get_option('use_dcmtk')
if not dcmtk_opt.disabled()
	dcmdata_dep = dependency('dcmtk', 'DCMTK', required: false)

	if not dcmdata_dep.found()
		dcmdata_dep = cc.find_library('libdcmdata',
		has_headers: [
		'dcmtk/dcmdata/dcddirif.h',
        'dcmtk/dcmdata/dctk.h',
        'dcmtk/dcmdata/dcrledrg.h',
        'dcmtk/dcmjpeg/djdecode.h',
        'dcmtk/ofstd/ofcond.h',
        ],
		#header_include_directories: [include_directories('dcmtk/dcmdata', is_system:true)] ,
		required: true
		)
	endif
	config_h.set10('AMIDE_LIBDCMDATA_SUPPORT', dcmdata_dep.found())
	amide_deps += dcmdata_dep
else
	config_h.set10('AMIDE_LIBDCMDATA_SUPPORT', false)
endif

#GCONF Dependency
gconf_opt = get_option('use_gconf')
if not gconf_opt.disabled()
	gconf_dep = dependency('gconf-2.0',version: '>=2.14.0', required: gconf_opt )
	config_h.set10('AMIDE_USE_GCONF', gconf_dep.found())
	amide_deps += gconf_dep
else
	config_h.set10('AMIDE_USE_GCONF', false)
endif


amide_sources += amide_resources

amide_sources += configure_file(output: meson.project_name() + '_config.h',
	configuration: config_h)

executable('amide', amide_sources,
	dependencies: amide_deps,
	install: true,
)
