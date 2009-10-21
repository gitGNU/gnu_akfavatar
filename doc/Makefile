srcdir = ..

SHELL = /bin/sh
LANG = C
MAKEINFO = makeinfo
TEXI2HTML = texi2html
TEXI2DVI = texi2dvi
INSTALL_PROGRAM = $(INSTALL) -m 755
INSTALL_DATA = $(INSTALL) -m 644

# comment out if you prefer black on white
USECSS = --css-include=$(srcdir)/doc/akfavatar.css

.PHONY: all txt info html dvi pdf ps man \
	clean mostlyclean distclean maintainer-clean maintainerclean

all: txt info html dvi pdf ps

$(srcdir)/doc/pasref.txt: $(srcdir)/pascal/akfavatar.pas
	sed -n '/^unit/,/^implementation/p' $(srcdir)/pascal/akfavatar.pas \
	  | sed '/^implementation/ d' > $@

info: $(srcdir)/doc/akfavatar-en.info $(srcdir)/doc/akfavatar-de.info

$(srcdir)/doc/akfavatar-en.info: $(srcdir)/doc/akfavatar-en.texinfo \
	    $(srcdir)/doc/pasref.txt $(srcdir)/akfavatar.h version.texi
	-$(MAKEINFO) -I. -I$(srcdir) -I$(srcdir)/doc -o $@ \
	  $(srcdir)/doc/akfavatar-en.texinfo

$(srcdir)/doc/akfavatar-de.info: $(srcdir)/doc/akfavatar-de.texinfo \
	    $(srcdir)/doc/pasref.txt $(srcdir)/akfavatar.h version.texi
	-$(MAKEINFO) -I. -I$(srcdir) -I$(srcdir)/doc -o $@ --enable-encoding \
	  $(srcdir)/doc/akfavatar-de.texinfo

html: $(srcdir)/doc/akfavatar-en.html $(srcdir)/doc/akfavatar-de.html

$(srcdir)/doc/akfavatar-en.html: $(srcdir)/doc/akfavatar-en.texinfo \
	       $(srcdir)/doc/pasref.txt $(srcdir)/akfavatar.h version.texi
	-$(TEXI2HTML) $(USECSS) -I . -I $(srcdir) -I $(srcdir)/doc \
	      --output=$@ \
	      $(srcdir)/doc/akfavatar-en.texinfo

akfavatar-de.html: $(srcdir)/doc/akfavatar-de.texinfo \
                   $(srcdir)/doc/pasref.txt \
	           (srcdir)/akfavatar.h version.texi
	-$(TEXI2HTML) $(USECSS) -I . -I $(srcdir) -I $(srcdir)/doc \
	      --output=$@ \
	      $(srcdir)/doc/akfavatar-de.texinfo

txt: akfavatar-en.txt akfavatar-de.txt

akfavatar-en.txt: $(srcdir)/doc/akfavatar-en.texinfo \
                  $(srcdir)/doc/pasref.txt \
	          $(srcdir)/akfavatar.h version.texi
	-$(MAKEINFO) -I. -I$(srcdir) -I$(srcdir)/doc --plaintext -o $@ \
	      $(srcdir)/doc/akfavatar-en.texinfo

akfavatar-de.txt: $(srcdir)/doc/akfavatar-de.texinfo \
                  $(srcdir)/doc/pasref.txt \
	          $(srcdir)/akfavatar.h version.texi
	-$(MAKEINFO) -I. -I$(srcdir) -I$(srcdir)/doc --enable-encoding \
	             --plaintext -o $@ \
	             $(srcdir)/doc/akfavatar-de.texinfo

dvi: akfavatar-en.dvi akfavatar-de.dvi

akfavatar-en.dvi: $(srcdir)/doc/akfavatar-en.texinfo \
                  $(srcdir)/doc/pasref.txt \
	          $(srcdir)/akfavatar.h version.texi
	-$(TEXI2DVI) --build=tidy -I . -I $(srcdir) -I $(srcdir)/doc \
	             $(srcdir)/doc/akfavatar-en.texinfo

akfavatar-de.dvi: $(srcdir)/doc/akfavatar-de.texinfo \
                  $(srcdir)/doc/pasref.txt \
	          $(srcdir)/akfavatar.h version.texi
	-$(TEXI2DVI) --build=tidy -I . -I $(srcdir) -I $(srcdir)/doc \
	             $(srcdir)/doc/akfavatar-de.texinfo

pdf: akfavatar-en.pdf akfavatar-de.pdf

akfavatar-en.pdf: $(srcdir)/doc/akfavatar-en.texinfo \
                  $(srcdir)/doc/pasref.txt \
	          $(srcdir)/akfavatar.h $(srcdir)/doc/pdfmeta-en version.texi
	-$(TEXI2DVI) --build=tidy -I . -I $(srcdir) -I $(srcdir)/doc --pdf \
	             $(srcdir)/doc/akfavatar-en.texinfo
	-pdftk akfavatar-en.pdf update_info $(srcdir)/doc/pdfmeta-en \
	       output tmp.pdf \
	   && mv tmp.pdf akfavatar-en.pdf

akfavatar-de.pdf: $(srcdir)/doc/akfavatar-de.texinfo \
                  $(srcdir)/doc/pasref.txt \
	          $(srcdir)/akfavatar.h $(srcdir)/doc/pdfmeta-de version.texi
	-$(TEXI2DVI) --build=tidy -I . -I $(srcdir) -I $(srcdir)/doc --pdf \
	             $(srcdir)/doc/akfavatar-de.texinfo
	-pdftk akfavatar-de.pdf update_info $(srcdir)/doc/pdfmeta-de \
	       output tmp.pdf \
	   && mv tmp.pdf akfavatar-de.pdf

ps: akfavatar-en.ps akfavatar-de.ps

akfavatar-en.ps: akfavatar-en.dvi
	-dvips -Pwww -o $@ akfavatar-en.dvi

akfavatar-de.ps: akfavatar-de.dvi
	-dvips -Pwww -o $@ akfavatar-de.dvi

docbook: $(srcdir)/doc/akfavatar-en.texinfo \
         $(srcdir)/doc/akfavatar-de.texinfo \
         $(srcdir)/doc/pasref.txt $(srcdir)/akfavatar.h version.texi
	-$(MAKEINFO) -I. -I$(srcdir) -I$(srcdir)/doc --docbook \
	             $(srcdir)/doc/akfavatar-en.texinfo
	-$(MAKEINFO) -I. -I$(srcdir) -I$(srcdir)/doc --docbook \
	             $(srcdir)/doc/akfavatar-de.texinfo

man: $(srcdir)/doc/avatarsay.1 $(srcdir)/doc/avtcmd.1

$(srcdir)/doc/avatarsay.1: $(srcdir)/avatarsay.c $(srcdir)/configure
	-help2man --info-page=akfavatar-en --source=AKFAvatar \
	  -o $@ avatarsay

$(srcdir)/doc/avtcmd.1: $(srcdir)/avtcmd
	-help2man --info-page=akfavatar-en --source=AKFAvatar \
	  -o $@ $(srcdir)/avtcmd

clean:
	-rm -f *~
	-rm -f *.aux *.cp *.cps *.fn *.ky *.pg *.tp *.vr
	-rm -f *.log *.toc

mostlyclean: clean

distclean: clean
	-rm -rf akfavatar-en/
	-rm -rf akfavatar-de/
	-rm -f version.texi
	-rm -f akfavatar-en.pdf akfavatar-en.ps akfavatar-en.dvi 
	-rm -f akfavatar-en.txt akfavatar-en.xml
	-rm -f akfavatar-de.pdf akfavatar-de.ps akfavatar-de.dvi
	-rm -f akfavatar-de.txt akfavatar-de.xml

maintainer-clean: distclean
	@echo 'This command is intended for maintainers to use; it'
	@echo 'deletes files that may need special tools to rebuild.'
	-rm -f pasref.txt
	-rm -f akfavatar-en.info akfavatar-en.html
	-rm -f akfavatar-de.info akfavatar-de.html
	-rm -f avatarsay.1

maintainerclean: maintainer-clean
