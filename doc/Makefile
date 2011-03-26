srcdir = ..

SHELL = /bin/sh
PS2PDF = ps2pdf

.PHONY: all txt info html dvi pdf ps man \
	clean mostlyclean distclean maintainer-clean maintainerclean

all: txt info html ps pdf

info:

html: lua-akfavatar.en.html lua-akfavatar-reference.en.html

lua-akfavatar-reference.en.html: $(srcdir)/doc/lua-akfavatar-reference.en.man
	-groff -man -Thtml $(srcdir)/doc/lua-akfavatar-reference.en.man > $@

lua-akfavatar.en.html: $(srcdir)/doc/lua-akfavatar.en.man
	-groff -man -Thtml $(srcdir)/doc/lua-akfavatar.en.man > $@

txt: lua-akfavatar-reference.en.txt lua-akfavatar.en.txt

lua-akfavatar-reference.en.txt: $(srcdir)/doc/lua-akfavatar-reference.en.man
	-groff -man -Tutf8 $(srcdir)/doc/lua-akfavatar-reference.en.man | col -bx > $@

lua-akfavatar.en.txt: $(srcdir)/doc/lua-akfavatar.en.man
	-groff -man -Tutf8 $(srcdir)/doc/lua-akfavatar.en.man | col -bx > $@

pdf: lua-akfavatar.en.pdf lua-akfavatar-reference.en.pdf

lua-akfavatar.en.pdf: $(srcdir)/doc/lua-akfavatar.en.man
	-groff -man -Tps $(srcdir)/doc/lua-akfavatar.en.man | $(PS2PDF) - $@

lua-akfavatar-reference.en.pdf: $(srcdir)/doc/lua-akfavatar-reference.en.man
	-groff -man -Tps $(srcdir)/doc/lua-akfavatar-reference.en.man | $(PS2PDF) - $@

ps: lua-akfavatar-reference.en.ps lua-akfavatar.en.ps

lua-akfavatar-reference.en.ps: $(srcdir)/doc/lua-akfavatar-reference.en.man
	-groff -man -Tps $(srcdir)/doc/lua-akfavatar-reference.en.man > $@

lua-akfavatar.en.ps: $(srcdir)/doc/lua-akfavatar.en.man
	-groff -man -Tps $(srcdir)/doc/lua-akfavatar.en.man > $@

dvi: lua-akfavatar.en.dvi lua-akfavatar-reference.en.dvi

lua-akfavatar-reference.en.dvi: $(srcdir)/doc/lua-akfavatar-reference.en.man
	-groff -man -Tdvi $(srcdir)/doc/lua-akfavatar-reference.en.man > $@

lua-akfavatar.en.dvi: $(srcdir)/doc/lua-akfavatar.en.man
	-groff -man -Tdvi $(srcdir)/doc/lua-akfavatar.en.man > $@

docbook:

man:


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
