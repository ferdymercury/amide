sizes = {
'16x16': 16,
'16x16@2x': 32,
'32x32': 32,
'32x32@2x': 64,
'128x128': 128,
'128x128@2x': 256,
'256x256': 256,
'256x256@2x': 512,
'512x512': 512,
'512x512@2x': 1024,
}

imagemagick = find_program('convert', required: true)

images = []

foreach name, size : sizes
# TODO: Generate png from amide_logo
endforeach

plist = configure_file(
	input: 'amide.plist.in',
	output : 'amide.plist',
	configuration : {
		'VERSION' : meson.project_version(),
	}
)

# TODO: Generate the actual executable and .app folder