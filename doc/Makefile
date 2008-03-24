SHELL = /bin/sh
LANG = C
MAKEINFO = makeinfo
TEXI2DVI = texi2dvi
INSTALL_PROGRAM = $(INSTALL) -m 755
INSTALL_DATA = $(INSTALL) -m 644

# comment out if you prefer black on white
USECSS = --css-include akfavatar.css

.PHONY: all txt info html dvi pdf ps man \
	clean mostlyclean distclean maintainerclean

all: txt info html dvi pdf ps

pasref.txt: ../pascal/akfavatar.pas
	sed -n '/^unit/,/^implementation/p' < ../pascal/akfavatar.pas \
	  | sed '/^implementation/ d' > $@

info: akfavatar-en.info akfavatar-de.info

akfavatar-en.info: akfavatar-en.texinfo pasref.txt
	-$(MAKEINFO) akfavatar-en.texinfo

akfavatar-de.info: akfavatar-de.texinfo pasref.txt
	-$(MAKEINFO) --enable-encoding akfavatar-de.texinfo

html: akfavatar-en.texinfo akfavatar-de.texinfo pasref.txt
	-$(MAKEINFO) --html --enable-encoding $(USECSS) akfavatar-en.texinfo
	-$(MAKEINFO) --html --enable-encoding $(USECSS) akfavatar-de.texinfo

txt: akfavatar-en.txt akfavatar-de.txt

akfavatar-en.txt: akfavatar-en.texinfo pasref.txt
	-$(MAKEINFO) --plaintext -o $@ akfavatar-en.texinfo

akfavatar-de.txt: akfavatar-de.texinfo pasref.txt
	-$(MAKEINFO) --enable-encoding --plaintext -o $@ akfavatar-de.texinfo

dvi: akfavatar-en.dvi akfavatar-de.dvi

akfavatar-en.dvi: akfavatar-en.texinfo pasref.txt
	-$(TEXI2DVI) akfavatar-en.texinfo

akfavatar-de.dvi: akfavatar-de.texinfo pasref.txt
	-$(TEXI2DVI) akfavatar-de.texinfo

pdf: akfavatar-en.pdf akfavatar-de.pdf

akfavatar-en.pdf: akfavatar-en.texinfo pasref.txt pdfmeta-en
	-$(TEXI2DVI) --pdf akfavatar-en.texinfo
	-pdftk akfavatar-en.pdf update_info pdfmeta-en output tmp.pdf \
	   && mv tmp.pdf akfavatar-en.pdf

akfavatar-de.pdf: akfavatar-de.texinfo pasref.txt pdfmeta-de
	-$(TEXI2DVI) --pdf akfavatar-de.texinfo
	-pdftk akfavatar-de.pdf update_info pdfmeta-de output tmp.pdf \
	   && mv tmp.pdf akfavatar-de.pdf

ps: akfavatar-en.ps akfavatar-de.ps

akfavatar-en.ps: akfavatar-en.dvi
	-dvips -Pwww -o $@ akfavatar-en.dvi

akfavatar-de.ps: akfavatar-de.dvi
	-dvips -Pwww -o $@ akfavatar-de.dvi

docbook: akfavatar-en.texinfo akfavatar-de.texinfo pasref.txt
	-$(MAKEINFO) --docbook akfavatar-en.texinfo
	-$(MAKEINFO) --docbook akfavatar-de.texinfo

man: avatarsay.1

avatarsay.1: ../avatarsay.c
	-help2man --info-page=akfavatar-en -o avatarsay.1 avatarsay

clean:
	-rm -f *~
	-rm -f *.aux *.cp *.cps *.fn *.ky *.pg *.tp *.vr
	-rm -f *.log *.toc

mostlyclean: clean

distclean: clean
	-rm -rf akfavatar-en/
	-rm -rf akfavatar-de/
	-rm -f version.texi
	-rm -f pasref.txt
	-rm -f akfavatar-en.pdf akfavatar-en.ps akfavatar-en.dvi
	-rm -f akfavatar-de.pdf akfavatar-de.ps akfavatar-de.dvi

maintainerclean: distclean
	-rm -f akfavatar-en.info akfavatar-en.txt
	-rm -f akfavatar-de.info akfavatar-de.txt
	-rm -f avatarsay.1

