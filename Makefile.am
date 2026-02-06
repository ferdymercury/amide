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

gsettings_SCHEMAS = com.github.ferdymercury.amide.gschema.xml

@GSETTINGS_RULES@

EXTRA_DIST = \
	$(gsettings_SCHEMAS) \
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
