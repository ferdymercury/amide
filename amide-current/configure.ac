dnl ===============================================================================
dnl              Configure script for AMIDE
dnl ===============================================================================
AC_PREREQ(2.59)
AC_INIT([amide],[1.0.5],[amide-users@lists.sourceforge.net])

AC_CONFIG_SRCDIR(src/amide.c)
AC_CONFIG_HEADERS(amide_config.h)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

AM_MAINTAINER_MODE([enable])

AC_PROG_CC
AC_PROG_CXX
AM_PROG_LIBTOOL

AC_CHECK_HEADERS(unistd.h, AC_DEFINE(HAVE_UNISTD_H))
AC_CHECK_SIZEOF(long,8)
AC_CHECK_SIZEOF(long long,8)

AC_CHECK_FUNCS(strptime)

dnl ================= translation =======================================

AM_GLIB_GNU_GETTEXT
GETTEXT_PACKAGE=amide
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Define the gettext package to be used])

ALL_LINGUAS="zh_CN zh_TW"
AC_PROG_INTLTOOL

dnl m4 required for compiling
AC_PATH_PROGS(M4, gnum4 gm4 m4, NOT_FOUND)
if test $M4 = "NOT_FOUND"; then
   echo "Could not find m4 on this system, please install to compile AMIDE."
   exit 1
fi

dnl disabled 2010.02.20, hopefully this is no longer an issue
dnl =====================================================================
dnl pick our optimization level, OSX's version of gcc doesn't handle -O6
dnl also, need the -headerpad_max_install_name to allow switching
dnl the library paths with strings that have longer lengths
dnl case "$host_os" in
dnl darwin*) 	CFLAGS="-g -O2 -Wall"
dnl		LDFLAGS="-headerpad_max_install_names";;
dnl *)		CFLAGS="-g -O6 -Wall"
dnl 		CXXFLAGS="-g -O6 -Wall";;
dnl esac

dnl added 2014.01.24
dnl make sure an optimization level is set. On jhbuild (os x) the CFLAGS gets
dnl overwritten, so we're using this to make sure some optimization is on
dnl when compiling. This is especially important to speeding up the 
dnl reslicing algorithm
OPTIMIZATION_CFLAGS="-O2"
echo "setting optimization level to $OPTIMIZATION_CFLAGS"
AC_SUBST(OPTIMIZATION_CFLAGS)

dnl 2013.11.02 - remove check for ancient versions of gcc from 13 years ago
dnl if test $CC = "gcc"; then
dnl   echo -n "checking version of gcc... "
dnl   CC_VERSION=`$CC -dumpversion`
dnl   if (test $CC_VERSION = "2.96" || test $$CC_VERSION = "2.95.4"); then
dnl	echo $CC_VERSION
dnl	echo "******************************************************************"
dnl	echo "This version of gcc is a beta compiler and often incorrectly"
dnl	echo "compiles this program.  Please upgrade to the current version of"
dnl	echo "gcc (3.2.1 as of this writing) or downgrade to 2.95.3"
dnl	echo "******************************************************************"
dnl	echo -n "continue anyway? (y/n) "
dnl	read continue_with_bad_gcc
dnl	if test $continue_with_bad_gcc = "y"; then
dnl		echo "removing optimizations to reduce potential of compiler error"
dnl		CFLAGS="-g -O0"
dnl	else
dnl		exit 1
dnl	fi
dnl   else
dnl	echo "$CC_VERSION, good"
dnl   fi
dnl fi









##############################################################
### Checks for optional libraries
##############################################################

dnl check for various libraries and programs that we might need
AM_PATH_GSL(1.1.1, FOUND_LIBGSL=yes, FOUND_LIBGSL=no)
AC_CHECK_LIB(ecat, matrix_open, FOUND_LIBECAT=yes, FOUND_LIBECAT=no, -L/sw/lib)
AC_CHECK_LIB(volpack, vpGetErrorString, FOUND_VOLPACK=yes, FOUND_VOLPACK=no, -lm -L/sw/lib -L/usr/local/lib)
AM_PATH_XMEDCON(0.10.0, FOUND_XMEDCON=yes, FOUND_XMEDCON=no)



dnl switch to C++ for DCMTK library stuff - also, if pthread is on the platform, probably need that
dnl autoconf doesn't have a nice macro for checking for c++ libraries, therefore the below:
AC_CHECK_LIB(pthread, pthread_mutex_init, THREAD_LIBS="-lpthread", THREAD_LIBS="")
AMIDE_LIBDCMDATA_LIBS="-L/usr/local/dicom/lib -L/usr/lib64/dcmtk -L/usr/lib/dcmtk -ldcmdata -loflog -lofstd -lz $THREAD_LIBS"
AMIDE_LIBDCMDATA_CFLAGS="-I/usr/local/dicom/include"
saved_libs="${LIBS}"
LIBS="${LIBS} ${AMIDE_LIBDCMDATA_LIBS}"
saved_cxxflags="${CXXFLAGS}"                                              
CXXFLAGS="${CXXFLAGS} ${AMIDE_LIBDCMDATA_CFLAGS} -DHAVE_CONFIG_H"
FOUND_DCMDATA=no
AC_LANG_CPLUSPLUS
AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <dcmtk/dcmdata/dcddirif.h>],
					  [DicomDirInterface dcm_dir;
					   dcm_dir.writeDicomDir();
					   ])],
    [FOUND_DCMDATA=yes],                                           
     [AC_MSG_WARN([libdcmdata was not found.])])
LIBS="${saved_libs}"
CXXFLAGS="${saved_cxxflags}"                                                                                        


dnl trying to phase out libfame use in favor of ffmpeg
PKG_CHECK_MODULES(FFMPEG, [
    libavcodec >= 51.45.0,
    libavutil
], FOUND_FFMPEG=yes, FOUND_FFMPEG=no);


dnl Let people compile without debugging information
AC_ARG_ENABLE(
	amide_debug, 
	[  --enable-amide-debug	  Compile with debugging messages and gdb debugging support [default=yes]], 
	enable_amide_debug="$enableval", 
	enable_amide_debug=yes)

if test $enable_amide_debug = yes; then
	echo "compiling with AMIDE debugging messages"
	AMIDE_DEBUG_CFLAGS="-ggdb -Wall"

	AC_SUBST(AMIDE_DEBUG_CFLAGS)
	AC_DEFINE(AMIDE_DEBUG, 1, Define to compile AMIDE with debugging information)
else
	echo "compiling without AMIDE debugging messages"
fi

dnl compile with warnings for obsolete Glib/GTK stuff
AC_ARG_ENABLE(
	amide_check_obsolete, 
	[  --enable-amide-check-obsolete  Compile with checks for old GLib/GTK functions [default=no]], 
	enable_amide_check_obsolete="$enableval", 
	enable_amide_check_obsolete=no)

if test $enable_amide_check_obsolete = yes; then
	echo "compiling with checks for obsolete GLib/GTK functions"
	AMIDE_CHECK_OBSOLETE_CFLAGS="-DG_DISABLE_DEPRECATED -DGDK_DISABLE_SINGLE_INCLUDES -DGDK_DISABLE_DEPRECATED -DGDK_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_DEPRECATED -DGTK_DISABLE_SINGLE_INCLUDES"

	AC_SUBST(AMIDE_CHECK_OBSOLETE_CFLAGS)
	AC_DEFINE(AMIDE_CHECK_OBSOLETE, 1, Define to compile with checks for obsolete GLib/GTK functions)
else
	echo "compiling without checks for obsolete GLib/GTK functions"
fi
 


dnl check if we're building on windows
dnl if you need to know OS in .c code, use G_PLATFORM_WIN32
AC_MSG_CHECKING([for native Win32])
case "$host" in
  *-*-mingw*)
    native_win32=yes
    ;;
  *)
    native_win32=no
    ;;
esac
AC_MSG_RESULT([$native_win32])
AM_CONDITIONAL(AMIDE_OS_WIN32, test "$native_win32" = yes)

dnl specify that we're building Mac OS X with native GTK support
AC_ARG_ENABLE(
	native_gtk_osx,
	[  --enable-native-gtk-osx Compile with native GTK support on Mac OS X [default=no]],
	enable_native_gtk_osx="$enableval",
	enable_native_gtk_osx=no)

case "$host" in
     *-apple-*)
     on_a_macintosh=yes
     ;;
     *)
     on_a_macintosh=no
     ;;
esac

if (test $enable_native_gtk_osx = yes) && (test $on_a_macintosh = yes); then
   echo "compiling with native GTK support on Mac OS X"
   AC_DEFINE(AMIDE_NATIVE_GTK_OSX, 1, Define to compile with native GTK support on Mac OS X)
fi
if (test $enable_native_gtk_osx = no) && (test $on_a_macintosh = yes); then
   echo "compiling with X11 GTK support (non-native) on Mac OS X"
fi
if (test $enable_native_gtk_osx = yes) && (test $on_a_macintosh = no); then
   echo "ignoring --enable-native-gtk-osx option, as OS other than Mac OS X"
fi

dnl let people compile without the GNU Scientific Library
AC_ARG_ENABLE(
	libgsl,
	[  --enable-libgsl	  Compile with the GNU Scientific Library [default=yes]],
	enable_libgsl="$enableval",
	enable_libgsl=yes)

if (test $enable_libgsl = yes) && (test $FOUND_LIBGSL = yes); then
	echo "compiling with GNU Scientific Library support"
	AC_DEFINE(AMIDE_LIBGSL_SUPPORT, 1, Define to compile with the GNU Scientific Library)
else
	echo "compiling without the GNU Scientific Library"
fi


dnl Let people compile without having libecat (z_matrix_70)
AC_ARG_ENABLE(
	libecat, 
	[  --enable-libecat	  Compile with the libecat/CTI library [default=yes]],
	enable_libecat="$enableval",
	enable_libecat=yes)

if (test $enable_libecat = yes) && (test $FOUND_LIBECAT = yes); then
	echo "compiling with libecat/CTI file support"
	AMIDE_LIBECAT_LIBS="-lecat"
	AC_SUBST(AMIDE_LIBECAT_LIBS)
	AC_DEFINE(AMIDE_LIBECAT_SUPPORT, 1, Define to compile with libecat)
else
	echo "compiling without libecat/CTI file support"
fi



dnl Let people compile without having libmdc
AC_ARG_ENABLE(
	libmdc, 
	[  --enable-libmdc	  Compile with the xmedcon/libmdc library [default=yes]], 
	enable_libmdc="$enableval", 
	enable_libmdc=yes)

if (test $enable_libmdc = yes) && (test $FOUND_XMEDCON = yes); then
	echo "compiling with xmedcon/libmdc file support"
	AC_DEFINE(AMIDE_LIBMDC_SUPPORT, 1, Define to compile with (X)MedCon)

	dnl 2007.10.28 - new xmedcon's should be using glib2/gtk2, dont' have to overwrite XMEDCON_LIBS
	dnl XMEDCON_LIBS="-L/usr/local/lib -L/usr/local/xmedcon/lib -lmdc"
	AC_SUBST(XMEDCON_LIBS)
else
	echo "compiling without xmedcon/libmdc file support"
fi


dnl Let people compile without rendering/libvolpack
AC_ARG_ENABLE(
	libvolpack, 
	[  --enable-libvolpack	  Compile in libvolpack rendering support [default=yes]], 
	enable_libvolpack="$enableval", 
	enable_libvolpack=yes)

if (test $enable_libvolpack = yes) && (test $FOUND_VOLPACK = yes); then
	echo "compiling with libvolpack rendering support "
	AMIDE_LIBVOLPACK_LIBS="-lvolpack"
	AC_SUBST(AMIDE_LIBVOLPACK_LIBS)
	AC_DEFINE(AMIDE_LIBVOLPACK_SUPPORT, 1, Define to compile with VolPack)
else
	echo "compiling without libvolpack rendering support"
fi


dnl Let people compile without mpeg movie generation/ffmpeg
AC_ARG_ENABLE(
	ffmpeg,
	[  --enable-ffmpeg   	  Compile in ffmpeg (libavcodec) mpeg encoding support [default=yes]], 
	enable_ffmpeg="$enableval", 
	enable_ffmpeg=yes)

if (test $enable_ffmpeg = yes) && (test $FOUND_FFMPEG = yes); then
	echo "compiling with ffmpeg (libavcodec) mpeg encoding support "
	AC_DEFINE(AMIDE_FFMPEG_SUPPORT, 1, Define to compile with ffmpeg)
	AC_SUBST(FFMPEG_LIBS)
	AC_SUBST(FFMPEG_CFLAGS)
else
	echo "compiling without ffmpeg (libavcodec) mpeg encoding support"

	dnl Let people compile with libfame mpeg movie generation if not using ffmpeg
	AM_PATH_LIBFAME(0.9.1, FOUND_LIBFAME=yes, FOUND_LIBFAME=no)
	dnl AC_CHECK_LIB(libfame, fame_init, FOUND_LIBFAME=yes, FOUND_LIBFAME=no)
	AC_ARG_ENABLE(
		libfame,
			[  --enable-libfame   	  Compile in libfame mpeg encoding support if not using ffmpeg [default=no]], 
			enable_libfame="$enableval", 		 
			enable_libfame=no)	

	if (test $enable_libfame = yes) && (test $FOUND_LIBFAME = yes); then
		echo "compiling with libfame mpeg encoding support "
		AC_DEFINE(AMIDE_LIBFAME_SUPPORT, 1, Define to compile with Libfame)
	else
		echo "compiling without libfame mpeg encoding support"
	fi
fi

dnl Let people compile without the DCMTK library
AC_ARG_ENABLE(
	libdcmdata,
	[  --enable-libdcmdata	  Compile in DCMTK support for DICOM files [default=yes]], 
	enable_libdcmdata="$enableval", 
	enable_libdcmdata=yes)

if (test $enable_libdcmdata = yes) && (test $FOUND_DCMDATA = yes); then
	echo "compiling with DCMTK support for DICOM files"
	AC_SUBST(AMIDE_LIBDCMDATA_LIBS)
	AC_SUBST(AMIDE_LIBDCMDATA_CFLAGS)
	AC_DEFINE(AMIDE_LIBDCMDATA_SUPPORT, 1, Define to compile with DCMTK)
else
	echo "compiling without DCMTK support for DICOM files"
fi



###############################
# Check for gtk/gnome stuff
##############################

PKG_CHECK_MODULES(AMIDE_GTK,[
	glib-2.0	>= 2.16.0
	gobject-2.0	>= 2.16.0
	gtk+-2.0	>= 2.16.0
	libxml-2.0	>= 2.4.12
	libgnomecanvas-2.0 >= 2.0.0
])

## add in gconf if not on win32 or macos x 
## gconf stuff is encapsulated in amide_gconf.c
if (test $native_win32 = no) && (test $enable_native_gtk_osx = no); then
   PKG_CHECK_MODULES(AMIDE_GTK_EXTRA_GCONF,[
		gconf-2.0	>= 2.14.0
   ])
   AMIDE_GTK_LIBS="$AMIDE_GTK_LIBS $AMIDE_GTK_EXTRA_GCONF_LIBS"
   AMIDE_GTK_CFLAGS="$AMIDE_GTK_CFLAGS $AMIDE_GTK_EXTRA_GCONF_CFLAGS"
fi

## add in gnome-vfs if not on win32 or gtk-osx
## gnome-vfs is only used by amide_gnome.c and only on unix
if (test $native_win32 = no) && (test $enable_native_gtk_osx = no); then
   PKG_CHECK_MODULES(AMIDE_GTK_EXTRA_GVFS,[
		gnome-vfs-2.0   >= 2.16.0
   ])
   AMIDE_GTK_LIBS="$AMIDE_GTK_LIBS $AMIDE_GTK_EXTRA_GVFS_LIBS"
   AMIDE_GTK_CFLAGS="$AMIDE_GTK_CFLAGS $AMIDE_GTK_EXTRA_GVFS_CFLAGS"
fi
 
AC_SUBST(AMIDE_GTK_LIBS)
AC_SUBST(AMIDE_GTK_CFLAGS)

dnl glib-genmarshal
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

dnl This is installed from GTK+ 2.0's gdk-pixbuf
AC_PATH_PROG(GDK_PIXBUF_CSOURCE, gdk-pixbuf-csource)
test -z "$GDK_PIXBUF_CSOURCE" && AC_MSG_ERROR([
*** You need the gdk-pixbuf-csource tool which is installed
*** from GTK+ 2's gdk-pixbuf.
***
*** Either the location where you installed your GTK+ 2 is
*** not in your PATH or something is screwed up with your
*** GTK+ 2 installation
])


dnl libgnomecanvas anti-aliasing has been buggy in the past. I originally thought
dnl this never would be fixed. Was unclear if the issues were in libgnomecanvas
dnl or libart. In any case, as of libgnomecanvas 2.30.3 things seem stable, 
dnl so I'm re-enabling support for now
dnl
dnl record if our version of libgnomecanvas can handle anti-aliasing
PKG_CHECK_MODULES(libgnomecanvas_greater_than_230, 
		[libgnomecanvas-2.0 >= 2.3.0], 
		enable_antialiasing=yes, enable_antialiasing=no)
if test $enable_antialiasing = yes; then
   echo "compiling with libgnomecanvas antialiasing support"
   AC_DEFINE(AMIDE_LIBGNOMECANVAS_AA, 1, Define to compile AMIDE with an antialiased canvas)
else
   echo "compiling without libgnomecanvas antialiasing support"
   AC_DEFINE(AMIDE_LIBGNOMECANVAS_AA, 0, Define to compile AMIDE with an antialiased canvas)
fi


################################################################
# Checks for gtk-doc and docbook-tools, from gtk's configure.in
################################################################

dnl if we define --enable-gtk-doc, we build the library documentation
GTK_DOC_CHECK([1.10])

dnl the following is for the application documentation
AC_ARG_ENABLE(doc,
        [  --disable-doc           Do not build documentation.],,)

if test "x${enable_doc}" = "x" ; then
  enable_doc=yes
fi

AC_MSG_CHECKING(whether documentation should be built)
if test ${enable_doc} = no; then
  AC_MSG_RESULT([no])
else
  AC_MSG_RESULT([yes])
fi
AM_CONDITIONAL(DISABLE_DOC, test ${enable_doc} = no)

if test ${enable_doc} = yes; then
   dnl building documentatin
   GNOME_DOC_INIT

else
  dnl not building documentation
  ENABLE_SK_TRUE="#"
  ENABLE_SK_FALSE=""  
  HAVE_GNOME_DOC_UTILS_TRUE="#"
  HAVE_GNOME_DOC_UTILS_FALSE=""
fi

AC_CHECK_PROG(DB2HTML, db2html, true, false)
AM_CONDITIONAL(HAVE_DOCBOOK, $DB2HTML)

AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)

AC_CONFIG_FILES([
Makefile
pixmaps/Makefile
src/Makefile
win32/Makefile
macosx/Makefile
macosx/amide.plist
doc/Makefile
doc/reference/Makefile
doc/reference/version.xml
help/Makefile
etc/Makefile
etc/amide-$VERSION-1.info:etc/amide.info.in
etc/amide-$VERSION-1.iss:etc/amide.iss.in
etc/amide.spec
man/Makefile
po/Makefile.in])
AC_OUTPUT 
dnl help/C/Makefile
dnl help/es/Makefile

dnl Move the spec file so it won't get eaten by make distclean
if [ test -f etc/amide.spec ]; then
   echo "Moving SPEC file to base directory"
   mv etc/amide.spec ./
fi
