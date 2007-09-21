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

info: akfavatar-en.info akfavatar-de.info

akfavatar-en.info: akfavatar-en.texinfo
	$(MAKEINFO) $<

akfavatar-de.info: akfavatar-de.texinfo
	$(MAKEINFO) --enable-encoding $<

html: akfavatar-en.texinfo akfavatar-de.texinfo
	$(MAKEINFO) --html --enable-encoding $(USECSS) akfavatar-en.texinfo
	$(MAKEINFO) --html --enable-encoding $(USECSS) akfavatar-de.texinfo

txt: akfavatar-en.txt akfavatar-de.txt

akfavatar-en.txt: akfavatar-en.texinfo
	$(MAKEINFO) --plaintext -o $@ $<

akfavatar-de.txt: akfavatar-de.texinfo
	$(MAKEINFO) --enable-encoding --plaintext -o $@ $<

dvi: akfavatar-en.dvi akfavatar-de.dvi

akfavatar-en.dvi: akfavatar-en.texinfo
	$(TEXI2DVI) $<

akfavatar-de.dvi: akfavatar-de.texinfo
	$(TEXI2DVI) $<

pdf: akfavatar-en.pdf akfavatar-de.pdf

akfavatar-en.pdf: akfavatar-en.texinfo
	$(TEXI2DVI) --pdf $<

akfavatar-de.pdf: akfavatar-de.texinfo
	$(TEXI2DVI) --pdf $<

ps: akfavatar-en.ps akfavatar-de.ps

akfavatar-en.ps: akfavatar-en.dvi
	dvips -Pwww -o $@ $<

akfavatar-de.ps: akfavatar-de.dvi
	dvips -Pwww -o $@ $<

man: avatarsay.1

avatarsay.1:
	help2man --info-page=akfavatar-en -o avatarsay.1 avatarsay

clean:
	-rm -f *~
	-rm -f *.aux *.cp *.cps *.fn *.ky *.pg *.tp *.vr
	-rm -f *.log *.toc

mostlyclean: clean

distclean: clean
	-rm -rf akfavatar-en/
	-rm -f version.texi
	-rm -f akfavatar-en.pdf akfavatar-en.ps akfavatar-en.dvi

maintainerclean: distclean
	-rm -f akfavatar-en.info akfavatar-en.txt avatarsay.1

