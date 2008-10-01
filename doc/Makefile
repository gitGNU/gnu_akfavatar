SHELL = /bin/sh
LANG = C
MAKEINFO = makeinfo
TEXI2DVI = texi2dvi
INSTALL_PROGRAM = $(INSTALL) -m 755
INSTALL_DATA = $(INSTALL) -m 644

# comment out if you prefer black on white
USECSS = --css-include akfavatar.css

.PHONY: all txt info html dvi pdf ps man \
	clean mostlyclean distclean maintainer-clean maintainerclean

all: txt info html dvi pdf ps

pasref.txt: ../pascal/akfavatar.pas
	sed -n '/^unit/,/^implementation/p' < ../pascal/akfavatar.pas \
	  | sed '/^implementation/ d' > $@

akfavatar.h: ../akfavatar.h
	-cp ../akfavatar.h $@

info: akfavatar-en.info akfavatar-de.info

akfavatar-en.info: akfavatar-en.texinfo pasref.txt akfavatar.h
	-$(MAKEINFO) akfavatar-en.texinfo

akfavatar-de.info: akfavatar-de.texinfo pasref.txt akfavatar.h
	-$(MAKEINFO) --enable-encoding akfavatar-de.texinfo

html: akfavatar-en.html akfavatar-de.html

akfavatar-en.html: akfavatar-en.texinfo pasref.txt akfavatar.h
	-$(MAKEINFO) --html --no-split $(USECSS) akfavatar-en.texinfo

akfavatar-de.html: akfavatar-de.texinfo pasref.txt akfavatar.h
	-$(MAKEINFO) --html --no-split $(USECSS) akfavatar-de.texinfo

txt: akfavatar-en.txt akfavatar-de.txt

akfavatar-en.txt: akfavatar-en.texinfo pasref.txt akfavatar.h
	-$(MAKEINFO) --plaintext -o $@ akfavatar-en.texinfo

akfavatar-de.txt: akfavatar-de.texinfo pasref.txt akfavatar.h
	-$(MAKEINFO) --enable-encoding --plaintext -o $@ akfavatar-de.texinfo

dvi: akfavatar-en.dvi akfavatar-de.dvi

akfavatar-en.dvi: akfavatar-en.texinfo pasref.txt akfavatar.h
	-$(TEXI2DVI) akfavatar-en.texinfo

akfavatar-de.dvi: akfavatar-de.texinfo pasref.txt akfavatar.h
	-$(TEXI2DVI) akfavatar-de.texinfo

pdf: akfavatar-en.pdf akfavatar-de.pdf

akfavatar-en.pdf: akfavatar-en.texinfo pasref.txt akfavatar.h pdfmeta-en
	-$(TEXI2DVI) --pdf akfavatar-en.texinfo
	-pdftk akfavatar-en.pdf update_info pdfmeta-en output tmp.pdf \
	   && mv tmp.pdf akfavatar-en.pdf

akfavatar-de.pdf: akfavatar-de.texinfo pasref.txt akfavatar.h pdfmeta-de
	-$(TEXI2DVI) --pdf akfavatar-de.texinfo
	-pdftk akfavatar-de.pdf update_info pdfmeta-de output tmp.pdf \
	   && mv tmp.pdf akfavatar-de.pdf

ps: akfavatar-en.ps akfavatar-de.ps

akfavatar-en.ps: akfavatar-en.dvi
	-dvips -Pwww -o $@ akfavatar-en.dvi

akfavatar-de.ps: akfavatar-de.dvi
	-dvips -Pwww -o $@ akfavatar-de.dvi

docbook: akfavatar-en.texinfo akfavatar-de.texinfo pasref.txt akfavatar.h
	-$(MAKEINFO) --docbook akfavatar-en.texinfo
	-$(MAKEINFO) --docbook akfavatar-de.texinfo

man: avatarsay.1 avtcmd.1

avatarsay.1: ../avatarsay.c
	-help2man --info-page=akfavatar-en --source=AKFAvatar \
	  -o avatarsay.1 avatarsay

avtcmd.1: ../avtcmd
	-help2man --info-page=akfavatar-en --source=AKFAvatar \
	  -o avtcmd.1 ../avtcmd

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
	-rm -f akfavatar.h
	-rm -f pasref.txt
	-rm -f akfavatar-en.info akfavatar-en.html
	-rm -f akfavatar-de.info akfavatar-de.html
	-rm -f avatarsay.1

maintainerclean: maintainer-clean
