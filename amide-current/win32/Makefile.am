if AMIDE_OS_WIN32

BUILT_SOURCES = \
        amide.ico \
        amide_file.ico \
        amiderc.o


amide.ico: amide_logo16x16.png amide_logo32x32.png amide_logo64x64.png amide_logo128x128.png
	png2ico.exe amide.ico amide_logo16x16.png amide_logo32x32.png amide_logo64x64.png amide_logo128x128.png

amide_file.ico: amide_file_logo16x16.png amide_file_logo32x32.png amide_file_logo64x64.png amide_file_logo128x128.png
	png2ico.exe amide_file.ico amide_file_logo16x16.png amide_file_logo32x32.png amide_file_logo64x64.png amide_file_logo128x128.png

amiderc.o: amiderc.rc amide.ico amide_file.ico
	windres -i amiderc.rc -o amiderc.o
endif


CLEANFILES = amiderc.o amide.ico amide_file.ico
DISTCLEANFILES = *~


