SHELL = /bin/sh
LANG = C
MAKEINFO = makeinfo
TEXI2DVI = texi2dvi
INSTALL_PROGRAM = $(INSTALL) -m 755
INSTALL_DATA = $(INSTALL) -m 644

.PHONY: all txt info html dvi pdf ps man \
	clean mostlyclean distclean maintainerclean

all: txt info html dvi pdf ps

info: akfavatar-en.info

akfavatar-en.info: akfavatar-en.texinfo
	$(MAKEINFO) $<

html: akfavatar-en.texinfo
	$(MAKEINFO) --html $<

txt: akfavatar-en.txt

akfavatar-en.txt: akfavatar-en.texinfo
	$(MAKEINFO) --plaintext -o $@ $<

dvi: akfavatar-en.dvi

akfavatar-en.dvi: akfavatar-en.texinfo
	$(TEXI2DVI) $<

pdf: akfavatar-en.pdf

akfavatar-en.pdf: akfavatar-en.texinfo
	$(TEXI2DVI) --pdf $<

ps: akfavatar-en.ps

akfavatar-en.ps: akfavatar-en.texinfo

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

