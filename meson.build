project('amide', ['c', 'cpp'],
	version: '1.0.7',
	meson_version: '>= 0.61.2',
	default_options: [ 'warning_level=2', 'werror=false', 'c_std=gnu17', 'cpp_std=c++17' ],
)

i18n = import('i18n')
gnome = import('gnome')
cc = meson.get_compiler('cpp')

config_h = configuration_data()
config_h.set_quoted('VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', 'amide')
config_h.set_quoted('PACKAGE', meson.project_name())
config_h.set_quoted('AMIDE_DATADIR', get_option('datadir') / meson.project_name())
config_h.set10('AMIDE_DEBUG', get_option('buildtype').startswith('debug'))
config_h.set_quoted('LOCALEDIR', get_option('prefix') / get_option('localedir'))
config_h.set10('ENABLE_NLS', true)
add_project_arguments(['-I' + meson.project_build_root()], language: 'cpp')

project_c_args = []
test_c_args = [
	'-Wcast-align',
	'-Wdeclaration-after-statement',
	'-Werror=address',
	'-Werror=array-bounds',
	'-Werror=empty-body',
	'-Werror=implicit',
	'-Werror=implicit-function-declaration',
	'-Werror=incompatible-pointer-types',
	'-Werror=init-self',
	'-Werror=int-conversion',
	'-Werror=int-to-pointer-cast',
	'-Werror=main',
	'-Werror=misleading-indentation',
	'-Werror=missing-braces',
	'-Werror=missing-include-dirs',
	'-Werror=nonnull',
	'-Werror=overflow',
	'-Werror=parenthesis',
	'-Werror=pointer-arith',
	'-Werror=pointer-to-int-cast',
	'-Werror=redundant-decls',
	'-Werror=return-type',
	'-Werror=sequence-point',
	'-Werror=shadow',
	'-Werror=strict-prototypes',
	'-Werror=trigraphs',
	#'-Werror=undef', maybe for later?
	'-Werror=write-strings',
	'-Wformat-nonliteral',
	'-Wignored-qualifiers',
	'-Wimplicit-function-declaration',
	'-Wlogical-op',
	'-Wmissing-declarations',
	'-Wmissing-format-attribute',
	'-Wmissing-include-dirs',
	'-Wmissing-noreturn',
	'-Wnested-externs',
	'-Wno-cast-function-type',
	'-Wno-dangling-pointer',
	'-Wno-missing-field-initializers',
	'-Wno-sign-compare',
	'-Wno-unused-parameter',
	'-Wold-style-definition',
	'-Wpointer-arith',
	'-Wredundant-decls',
	'-Wstrict-prototypes',
	'-Wswitch-default',
	'-Wswitch-enum',
	'-Wundef',
	'-Wuninitialized',
	'-Wunused',
	'-fno-strict-aliasing',
	['-Werror=format-security', '-Werror=format=2'],
]
if get_option('buildtype') != 'plain'
	test_c_args += '-fstack-protector-strong'
endif
foreach arg: test_c_args
	if cc.has_multi_arguments(arg)
		project_c_args += arg
	endif
endforeach
add_project_arguments(project_c_args, language: 'cpp')

subdir('data')
subdir('src')
subdir('po')
doc_opt = get_option('generate_docs')
if doc_opt.enabled()
	subdir('docs')
endif

os = host_machine.system()

if os == 'windows'
	subdir('windows')
elif os == 'darwin'
	subdir('macos')
else
	subdir('linux')
	gnome.post_install(
		glib_compile_schemas: true,
		gtk_update_icon_cache: true,
		update_desktop_database: true,
	)
endif
