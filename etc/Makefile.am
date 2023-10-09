EXTRA_DIST = \
	amide.spec.in \
	amide.spec \
	amide-$(VERSION)-1.info \
	amide.desktop.in 



Applicationsdir = $(datadir)/applications/
Applications_in_files = amide.desktop.in
Applications_DATA = $(Applications_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

DISTCLEANFILES = \
	$(Applications_DATA) \
	*~ 	\
	amide-*.info \
	amide-*.iss
