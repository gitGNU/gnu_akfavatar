VPATH = .
SHELL = /bin/sh
ROFF = groff
PS2PDF = ps2pdf

.PHONY: all txt info html dvi pdf ps man \
	clean mostlyclean distclean maintainer-clean maintainerclean

.SUFFIXES: .man .html .txt .pdf .ps .dvi

# does anybody still need dvi?
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
	-$(ROFF) -man -Thtml $< >$@

.man.txt:
	-$(ROFF) -man -Tutf8 $< | col -bx >$@

.man.pdf:
	-$(ROFF) -man -Tps $< | $(PS2PDF) - $@

.man.ps:
	-$(ROFF) -man -Tps $< >$@

.man.dvi:
	-$(ROFF) -man -Tdvi $< >$@

$(srcdir)/doc/pasref.txt: $(srcdir)/pascal/akfavatar.pas
	sed -n '/^unit/,/^implementation/p' $(srcdir)/pascal/akfavatar.pas \
	  | sed '/^implementation/ d' > $@

clean:
	-rm -f *~

mostlyclean: clean

distclean: clean
	-rm -f version.texi

maintainer-clean: distclean
	@echo 'This command is intended for maintainers to use; it'
	@echo 'deletes files that may need special tools to rebuild.'
	-rm -f pasref.txt

maintainerclean: maintainer-clean
