imagemagick = find_program('convert', required:true)

amide_ico = custom_target(
	'amide_logo.ico',
	output: 'amide.ico',
	input: amide_logo,
	command: [imagemagick, '-density 256x256', '-background transparent', '@INPUT@', '-define icon:auto-resize' ,'-colors 256', '@OUTPUT@']
)

windres = find_program('windres', required: true)

//

inno_script = configure_file(
	input: 'amide.iss.in',
	output: 'amide.iss',
	configuration: {
		'PACKAGE': meson.project_name(),
		'VERSION': meson.project_version(),
		'BUILD_PATH': meson.project_build_root(),
		'SOURCE_PATH': meson.project_source_root(),
		'PROJECT_URL' : conf_data.get('PROJECT_URL'),
		'PROJECT_ISSUES_URL' : conf_data.get('PROJECT_URL') / 'issues',
		'MSYS_PATH': get_option('msys_path'),
	}
)


