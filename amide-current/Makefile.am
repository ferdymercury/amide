ACLOCAL_AMFLAGS = \
	-I /usr/local/share/aclocal \
	-I macros

# note, win32 needs to be before src
SUBDIRS = \
	pixmaps \
	win32 \
	src \
	doc \
	man \
	help \
	po \
	etc


EXTRA_DIST = \
	intltool-extract.in \
	intltool-merge.in    \
	intltool-update.in

CLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update

DISTCLEANFILES = *~ 
