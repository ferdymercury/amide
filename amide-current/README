=====
AMIDE
=====

AMIDE stands for: AMIDE's a Medical Image Data Examiner

AMIDE is intended for viewing and analyzing 3D medical imaging data
sets.  For more information on AMIDE, check out the AMIDE web page at:
	http://amide.sourceforge.net

AMIDE is licensed under the terms of the GNU GPL included in the file
COPYING.



Requirements
------------


1) Compiler: 

I currently use gcc-4.6.  The later 3.* series (e.g. 3.3) should work
as well, along with version 2.95.  Early 3.* Versions of gcc will
quite likely generate compilation errors (and make AMIDE unstable) if
optimizations are used when compiling.



2) GTK+:

The current series of AMIDE requires GTK+-2, at least version 2.16.
I'm currently developing on a Fedora Core 15 system, although other
distributions of Linux with equivalent library support should work.

Scrollkeeper is required for generating the help documentation.  If
you don't care about that, it's not needed.

3) Additional libraries:
   libgnomecanvas
   libxml-2
   libgnomeui-2 (not needed on win32)

These are various other libraries are needed for installation, most of
which you will most likely already have installed if you have GTK+.


Optional Packages
-----------------


1) (X)MedCon/libmdc

(X)MedCon includes a library (libmdc) which allows AMIDE to import the
following formats; Acr/Nema 2.0, Analyze (SPM), Concorde microPET,
DICOM 3.0, ECAT/Matrix 6/7, InterFile3.3 and Gif87a/89a.

(X)MedCon can be obtained from: 
	http://xmedcon.sourceforge.net


2) DCMTK - DICOM Toolkit

DCMTK provides expanded support for DICOM files, allowing the reading
in of many clinical format DICOM datasets that (X)MedCon doesn't
support.  Version 3.6.0 is required. It can be downloaded at:
	http://dicom.offis.de/dcmtk.php.en


3) z_matrix_70/libecat

This library can be used as an alternative for importing ECAT 6/7
files, and is released under a fairly restrictive license.  It can be
found on the AMIDE sourceforge website, or at it's original site:

	ftp://dormeur.topo.ucl.ac.be/pub/ecat/z_matrix_70/ecat.tar.gz

The source file off of amide.sourceforge.net is preferable, as it
includes a Makefile which will make a shared library, and a small
patch.  Since the license for libecat is non-GPL compatible, you
really should only link to it as a shared library.  A README file is
included with the tarball that explains how to configure/compile the
library.  RPM packages are also available off the AMIDE web site.


4) volpack/libvolpack

Volpack includes a library (libvolpack) which is used for the optional
volume rendering component of AMIDE.  

The original version can be found at:
	http://graphics.stanford.edu/software/volpack/

The version available on the AMIDE web site is preferable, as it's
been updated to compile cleanly under Linux.  RPM packages are also
available.


5) ffmpeg (libavcodec) [alternatively libfame]

Another optional package, the ffmpeg library is used for generating
MPEG-1 movies from series of rendered images and for generating
fly-through movies.

Information, code, and binaries for ffmpeg can be found at;
	     http://ffmpeg.mplayerhq.hu

and Linux RPM binaries are available at:
    http://rpmfusion.org/

If for whatever reasons you do not want to use the ffmpeg package for
generating MPEG-1 movies, there is still code in AMIDE for using the
libfame package for doing this. Information, code, and binaries for
libfame can be found at: 
	http://fame.sourceforge.net

and Linux RPM binaries are available at:
	http://atrpms.net/name/libfame/



Building
--------

See the file INSTALL for info on compiling and installing.  If you
don't feel like reading that, try:

	./configure
	make 
	make install         (as root)


If you're wondering about configuration options, a lot of information
can be obtained by running:

	./configure --help



Building gtk-doc files
----------------------

The majority of the source code for AMIDE is structured as a library
extension of GTK, called AMITK. Documentation for this library can be
built using gtk-doc as follows:
      ./configure --enable-libdcmdata=no --enable-gtk-doc=yes
      make
