ACLOCAL_AMFLAGS = -I m4


if DISABLE_DOC
HELPDIR = 
else
HELPDIR = help
endif


# note, win32 needs to be before src
SUBDIRS = \
	pixmaps \
	win32 \
	macosx \
	src \
	doc \
	man \
	po \
	etc \
	$(HELPDIR)


EXTRA_DIST = \
	intltool-extract.in \
	intltool-merge.in    \
	intltool-update.in \
	gnome-doc-utils.make \
	m4   

CLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update


DISTCLEANFILES = *~ 

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper --enable-gtk-doc
