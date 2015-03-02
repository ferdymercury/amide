ACLOCAL_AMFLAGS = \
	-I /usr/local/share/aclocal \
	-I m4

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
	gnome-doc-utils.make    

CLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update


DISTCLEANFILES = *~ 

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper --enable-gtk-doc
