amide_helpdir = $(datadir)/gnome/help/amide/C

amide_help_DATA = 	\
	amide.html	\
	topic.dat

SGML_FILES =			\
	amide.sgml		\
	basics.sgml		\
	file-handling.sgml	\
	importing.sgml		\
	rendering.sgml		\
	rois.sgml		\
	volumes.sgml		

PERL_FILES = 			\
	make-index.pl		\
	make-docbook.pl	

EXTRA_DIST = \
	$(SGML_FILES)		\
	$(PERL_FILES)		\
	topic.dat

all: 

amide.html: amide/amide.html
	-cd $(srcdir) && cp amide/amide.html .

amide/amide.html: $(SGML_FILES)
	-cd $(srcdir) && db2html amide.sgml

dist-hook:
	mkdir $(distdir)/amide
	-cp $(srcdir)/amide/*.html amide/*.css $(distdir)/amide
	-cp $(srcdir)/amide.html $(distdir)
	mkdir $(distdir)/images
	-cp $(srcdir)/images/*.gif $(srcdir)/images/*.jpg $(distdir)/images

install-data-local: amide.html
	$(mkinstalldirs) $(DESTDIR)$(amide_helpdir)/images
	-for file in $(srcdir)/amide/*.html $(srcdir)/amide/*.css; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(amide_helpdir)/$$basefile; \
	done
	-for file in $(srcdir)/images/*.jpg $(srcdir)/images/*.gif; do \
	$(INSTALL_DATA) $$file $(DESTDIR)$(amide_helpdir)/images;\
	done

amide.ps: amide.sgml
	-db2ps $<

amide.rtf: amide.sgml	
	-db2rtf $<



DISTCLEANFILES = *~


