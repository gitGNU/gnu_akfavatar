srcdir = ..

SHELL = /bin/sh
PS2PDF = ps2pdf

.PHONY: all txt info html dvi pdf ps man \
	clean mostlyclean distclean maintainer-clean maintainerclean

.SUFFIXES: .man .txt .html .ps .pdf .dvi

all: txt info html ps pdf

info:

html: lua-akfavatar.en.html lua-akfavatar-reference.en.html

txt: lua-akfavatar-reference.en.txt lua-akfavatar.en.txt

pdf: lua-akfavatar.en.pdf lua-akfavatar-reference.en.pdf

ps: lua-akfavatar-reference.en.ps lua-akfavatar.en.ps

dvi: lua-akfavatar.en.dvi lua-akfavatar-reference.en.dvi

docbook:

man:


.man.html:
	-groff -man -Thtml $< > $@

.man.txt:
	-groff -man -Tutf8 $< | col -bx > $@

.man.pdf:
	-groff -man -Tps $< | $(PS2PDF) - $@

.man.ps:
	-groff -man -Tps $< > $@

.man.dvi:
	-groff -man -Tdvi $< > $@


$(srcdir)/doc/pasref.txt: $(srcdir)/pascal/akfavatar.pas
	sed -n '/^unit/,/^implementation/p' $(srcdir)/pascal/akfavatar.pas \
	  | sed '/^implementation/ d' > $@

clean:
	-rm -f *~

mostlyclean: clean

distclean: clean
	-rm -f *.txt
	-rm -f *.html
	-rm -f *.dvi
	-rm -f *.ps
	-rm -f *.pdf

maintainer-clean: distclean
	@echo 'This command is intended for maintainers to use; it'
	@echo 'deletes files that may need special tools to rebuild.'

maintainerclean: maintainer-clean
