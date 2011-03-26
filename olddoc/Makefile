srcdir = ..

SHELL = /bin/sh
LANG = C
MAKEINFO = makeinfo
TEXI2HTML = texi2html
TEXI2DVI = texi2dvi
PS2PDF = ps2pdf
INSTALL_PROGRAM = $(INSTALL) -m 755
INSTALL_DATA = $(INSTALL) -m 644

# comment out if you prefer black on white
USECSS = --css-include=$(srcdir)/olddoc/akfavatar.css

.PHONY: all txt info html dvi pdf ps man \
	clean mostlyclean distclean maintainer-clean maintainerclean

all: txt info html dvi pdf ps

$(srcdir)/olddoc/pasref.txt: $(srcdir)/pascal/akfavatar.pas
	sed -n '/^unit/,/^implementation/p' $(srcdir)/pascal/akfavatar.pas \
	  | sed '/^implementation/ d' > $@

info: $(srcdir)/olddoc/akfavatar-en.info $(srcdir)/olddoc/akfavatar-de.info

$(srcdir)/olddoc/akfavatar-en.info: $(srcdir)/olddoc/akfavatar-en.texinfo \
	    $(srcdir)/olddoc/lua-ref-en.texinfo \
	    $(srcdir)/olddoc/lua-akfavatar-en.texinfo \
	    $(srcdir)/olddoc/pasref.txt $(srcdir)/akfavatar.h version.texi
	-$(MAKEINFO) -I. -I$(srcdir) -I$(srcdir)/doc -o $@ \
	  $(srcdir)/olddoc/akfavatar-en.texinfo

$(srcdir)/olddoc/akfavatar-de.info: $(srcdir)/olddoc/akfavatar-de.texinfo \
	    $(srcdir)/olddoc/lua-ref-en.texinfo \
	    $(srcdir)/olddoc/lua-akfavatar-de.texinfo \
	    $(srcdir)/olddoc/pasref.txt $(srcdir)/akfavatar.h version.texi
	-$(MAKEINFO) -I. -I$(srcdir) -I$(srcdir)/doc -o $@ --enable-encoding \
	  $(srcdir)/olddoc/akfavatar-de.texinfo

html: $(srcdir)/olddoc/akfavatar-en.html $(srcdir)/olddoc/akfavatar-de.html

$(srcdir)/olddoc/akfavatar-en.html: $(srcdir)/olddoc/akfavatar-en.texinfo \
	       $(srcdir)/olddoc/lua-ref-en.texinfo \
	       $(srcdir)/olddoc/lua-akfavatar-en.texinfo \
	       $(srcdir)/olddoc/pasref.txt $(srcdir)/akfavatar.h version.texi
	-$(TEXI2HTML) $(USECSS) -I . -I $(srcdir) -I $(srcdir)/doc \
	      --output=$@ \
	      $(srcdir)/olddoc/akfavatar-en.texinfo

$(srcdir)/olddoc/akfavatar-de.html: $(srcdir)/olddoc/akfavatar-de.texinfo \
	           $(srcdir)/olddoc/lua-ref-en.texinfo \
	          $(srcdir)/olddoc/lua-akfavatar-de.texinfo \
	           $(srcdir)/olddoc/pasref.txt \
	           $(srcdir)/akfavatar.h version.texi
	-$(TEXI2HTML) $(USECSS) -I . -I $(srcdir) -I $(srcdir)/doc \
	      --output=$@ \
	      $(srcdir)/olddoc/akfavatar-de.texinfo

txt: akfavatar-en.txt akfavatar-de.txt

akfavatar-en.txt: $(srcdir)/olddoc/akfavatar-en.texinfo \
	          $(srcdir)/olddoc/lua-ref-en.texinfo \
	          $(srcdir)/olddoc/lua-akfavatar-en.texinfo \
	          $(srcdir)/olddoc/pasref.txt \
	          $(srcdir)/akfavatar.h version.texi
	-$(MAKEINFO) -I. -I$(srcdir) -I$(srcdir)/doc --plaintext -o $@ \
	      $(srcdir)/olddoc/akfavatar-en.texinfo

akfavatar-de.txt: $(srcdir)/olddoc/akfavatar-de.texinfo \
	          $(srcdir)/olddoc/lua-akfavatar-de.texinfo \
	          $(srcdir)/olddoc/lua-ref-en.texinfo \
	          $(srcdir)/olddoc/pasref.txt \
	          $(srcdir)/akfavatar.h version.texi
	-$(MAKEINFO) -I. -I$(srcdir) -I$(srcdir)/doc --enable-encoding \
	             --plaintext -o $@ \
	             $(srcdir)/olddoc/akfavatar-de.texinfo

dvi: akfavatar-en.dvi akfavatar-de.dvi

akfavatar-en.dvi: $(srcdir)/olddoc/akfavatar-en.texinfo \
	          $(srcdir)/olddoc/lua-ref-en.texinfo \
	          $(srcdir)/olddoc/lua-akfavatar-en.texinfo \
	          $(srcdir)/olddoc/pasref.txt \
	          $(srcdir)/akfavatar.h version.texi
	-$(TEXI2DVI) --build=tidy -I . -I $(srcdir) -I $(srcdir)/doc \
	             $(srcdir)/olddoc/akfavatar-en.texinfo

akfavatar-de.dvi: $(srcdir)/olddoc/akfavatar-de.texinfo \
	          $(srcdir)/olddoc/lua-ref-en.texinfo \
	          $(srcdir)/olddoc/lua-akfavatar-de.texinfo \
	          $(srcdir)/olddoc/pasref.txt \
	          $(srcdir)/akfavatar.h version.texi
	-$(TEXI2DVI) --build=tidy -I . -I $(srcdir) -I $(srcdir)/doc \
	             $(srcdir)/olddoc/akfavatar-de.texinfo

pdf: akfavatar-en.pdf akfavatar-de.pdf

akfavatar-en.pdf: $(srcdir)/olddoc/akfavatar-en.texinfo \
	          $(srcdir)/olddoc/lua-akfavatar-en.texinfo \
	          $(srcdir)/olddoc/lua-ref-en.texinfo \
	          $(srcdir)/olddoc/pasref.txt \
	          $(srcdir)/akfavatar.h $(srcdir)/olddoc/pdfmeta-en version.texi
	-$(TEXI2DVI) --build=tidy -I . -I $(srcdir) -I $(srcdir)/doc --pdf \
	             $(srcdir)/olddoc/akfavatar-en.texinfo
	-pdftk akfavatar-en.pdf update_info $(srcdir)/olddoc/pdfmeta-en \
	       output tmp.pdf \
	   && mv tmp.pdf akfavatar-en.pdf

akfavatar-de.pdf: $(srcdir)/olddoc/akfavatar-de.texinfo \
	          $(srcdir)/olddoc/lua-akfavatar-de.texinfo \
	          $(srcdir)/olddoc/lua-ref-en.texinfo \
	          $(srcdir)/olddoc/pasref.txt \
	          $(srcdir)/akfavatar.h $(srcdir)/olddoc/pdfmeta-de version.texi
	-$(TEXI2DVI) --build=tidy -I . -I $(srcdir) -I $(srcdir)/doc --pdf \
	             $(srcdir)/olddoc/akfavatar-de.texinfo
	-pdftk akfavatar-de.pdf update_info $(srcdir)/olddoc/pdfmeta-de \
	       output tmp.pdf \
	   && mv tmp.pdf akfavatar-de.pdf

ps: akfavatar-en.ps akfavatar-de.ps

akfavatar-en.ps: akfavatar-en.dvi
	-dvips -Pwww -o $@ akfavatar-en.dvi

akfavatar-de.ps: akfavatar-de.dvi
	-dvips -Pwww -o $@ akfavatar-de.dvi

docbook: $(srcdir)/olddoc/akfavatar-en.texinfo \
	 $(srcdir)/olddoc/akfavatar-de.texinfo \
	 $(srcdir)/olddoc/lua-akfavatar-en.texinfo \
	 $(srcdir)/olddoc/lua-akfavatar-de.texinfo \
	 $(srcdir)/olddoc/pasref.txt $(srcdir)/akfavatar.h version.texi
	-$(MAKEINFO) -I. -I$(srcdir) -I$(srcdir)/doc --docbook \
	             $(srcdir)/olddoc/akfavatar-en.texinfo
	-$(MAKEINFO) -I. -I$(srcdir) -I$(srcdir)/doc --docbook \
	             $(srcdir)/olddoc/akfavatar-de.texinfo

man: $(srcdir)/olddoc/avatarsay.1 $(srcdir)/olddoc/avtcmd.1

$(srcdir)/olddoc/avatarsay.1: $(srcdir)/avatarsay.c $(srcdir)/configure
	-help2man --info-page=akfavatar-en --source=AKFAvatar \
	  -o $@ avatarsay

$(srcdir)/olddoc/avtcmd.1: $(srcdir)/avtcmd
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
	-rm -f avatarsay.1 avtcmd.1

maintainerclean: maintainer-clean
