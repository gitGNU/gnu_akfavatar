VPATH = ..
srcdir= ..
SHELL = /bin/sh
ROFF = groff
PS2PDF = ps2pdf
ENSCRIPT = enscript

.PHONY: all txt info html dvi pdf ps man \
	clean mostlyclean distclean maintainer-clean maintainerclean

.SUFFIXES: .h .man .html .txt .pdf .ps .dvi

# does anybody still need dvi?
all: txt info html ps pdf

info:

html: lua-akfavatar.en.html lua-akfavatar-reference.en.html \
      akfavatar.h.html avtaddons.h.html

txt: lua-akfavatar-reference.en.txt lua-akfavatar.en.txt

pdf: lua-akfavatar.en.pdf lua-akfavatar-reference.en.pdf \
    akfavatar.h.pdf avtaddons.h.pdf

ps: lua-akfavatar-reference.en.ps lua-akfavatar.en.ps \
    akfavatar.h.ps avtaddons.h.ps

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

# enscript lists the directory, so we need a copy
akfavatar.h: $(srcdir)/akfavatar.h
	cp $(srcdir)/akfavatar.h $@

avtaddons.h: $(srcdir)/avtaddons.h
	cp $(srcdir)/avtaddons.h $@

akfavatar.h.ps: akfavatar.h
	-$(ENSCRIPT) -E --color -wPostScript -p$@ akfavatar.h

avtaddons.h.ps: avtaddons.h
	-$(ENSCRIPT) -E --color -wPostScript -p$@ avtaddons.h

akfavatar.h.pdf: akfavatar.h.ps
	$(PS2PDF) akfavatar.h.ps $@

avtaddons.h.pdf: avtaddons.h.ps
	$(PS2PDF) avtaddons.h.ps $@

akfavatar.h.html: akfavatar.h
	-$(ENSCRIPT) -takfavatar.h -E --color -whtml -p$(builddir)/$@ akfavatar.h

avtaddons.h.html: avtaddons.h
	-$(ENSCRIPT) -tavtaddons.h -E --color -whtml -p$(builddir)/$@ avtaddons.h

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
