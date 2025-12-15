# AMIDE

AMIDE stands for: AMIDE's a Medical Image Data Examiner

AMIDE is intended for viewing and analyzing 3D medical imaging data
sets. For more information on AMIDE, check out the AMIDE web page at:
http://amide.sourceforge.net

AMIDE is licensed under the terms of the GNU GPL included in the file
COPYING.

Instruction manuals can be found [here.](https://amide.sourceforge.net/documentation.html)

## Quick Linux instructions

	cd /tmp/
	git clone --depth=1 https://github.com/ferdymercury/amide
	# In Ubuntu 22:
	sudo apt install libgnomecanvas2-dev libgconf2-dev libmdc2-dev libvolpack1-dev libavcodec-dev meson gtk-doc-tools gtk-desktop-utils intltool libxml2-dev libgsl-dev libdcmtk-dev
	
    # Might be required due to how libdcmdata refuses to use pkg-config and instead just offers CMake modules
    sudo apt install cmake

	sudo mkdir -p /opt/amide
	sudo chown $USER /opt/amide/
	cd amide
	mkdir build-aux
	cd build-aux
	meson -Dprefix=/opt/amide ..
	ninja install

For system-wide install, do not use a prefix when running meson, and then just:

	sudo ninja install

## Requirements

### Compiler:

I currently use gcc-6.3. The later 3.* series (e.g. 3.3) should work as well, along with version 2.95. Early 3.*
Versions of gcc will quite likely generate compilation errors (and make AMIDE unstable) if optimizations are used
when compiling.

### GTK+:

The current series of AMIDE requires GTK+-2, at least version 2.16. I'm currently developing on a Fedora Core 24
system, although other distributions of Linux with equivalent library support should work.

Scrollkeeper is required for generating the help documentation. If you don't care about that, it's not needed.

### Additional libraries:

- libgnomecanvas
- libxml-2
- libgnomeui-2 (not needed on win32)

These are various other libraries are needed for installation, most of which you will most likely already have installed
if you have GTK+.

## Optional Packages

### (X)MedCon/libmdc

(X)MedCon includes a library (libmdc) which allows AMIDE to import the
following formats; Acr/Nema 2.0, Analyze (SPM), Concorde microPET,
DICOM 3.0, ECAT/Matrix 6/7, InterFile3.3 and Gif87a/89a.

(X)MedCon can be obtained from:
http://xmedcon.sourceforge.net

### DCMTK - DICOM Toolkit

DCMTK provides expanded support for DICOM files, allowing the reading in of many clinical format DICOM datasets
that (X)MedCon doesn't support. Version 3.6.0 is required. It can be downloaded
at: http://dicom.offis.de/dcmtk.php.en

### volpack/libvolpack

Volpack includes a library (libvolpack) which is used for the optional volume rendering component of AMIDE.

The original version can be found at: http://graphics.stanford.edu/software/volpack/

The version available on the AMIDE website is preferable, as it's been updated to compile cleanly under Linux. RPM
packages are also available.

### ffmpeg (libavcodec)

Another optional package, the ffmpeg library is used for generating MPEG-1 movies from series of rendered images and
for generating fly-through movies.

Information, code, and binaries for ffmpeg can be found at:
http://ffmpeg.mplayerhq.hu

and Linux RPM binaries are available at:
http://rpmfusion.org/

## Building

Check the `meson_options.txt` file or run `meson --help` to get the list of options to enable/disable
