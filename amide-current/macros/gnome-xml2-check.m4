dnl
dnl GNOME_XML2_HOOK (script-if-xml-found, failflag)
dnl
dnl If failflag is "failure", script aborts due to lack of XML2
dnl 
dnl Check for availability of the libxml2 library
dnl the XML2 parser uses libz if available too
dnl

AC_DEFUN([GNOME_XML2_HOOK],[
	AC_PATH_PROG(XML2_CONFIG,xml2-config,no)
	if test "$XML2_CONFIG" = no; then
		if test x$2 = xfailure; then
			AC_MSG_ERROR(Could not find xml2-config)
		fi
	fi
	GNOME_XML2_CFLAGS=`$XML2_CONFIG --cflags`
	AC_SUBST(GNOME_XML2_CFLAGS)
	AC_CHECK_LIB(xml, xmlNewDoc, [
		$1
		GNOME_XML2_LIBS=`$XML2_CONFIG --libs`
	], [
		if test x$2 = xfailure; then 
			AC_MSG_ERROR(Could not link sample xml2 program)
		fi
	], `$XML2_CONFIG --libs`)
	AC_SUBST(GNOME_XML2_LIBS)
	AC_DEFINE(GNOME_XML2_SUPPORT)
])

AC_DEFUN([GNOME_XML2_CHECK], [
	GNOME_XML2_HOOK([],failure)
])
