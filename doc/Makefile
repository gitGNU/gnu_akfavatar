VPATH = ..
srcdir= ..
builddir := $(shell pwd)
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

html: lua-akfavatar.en.html lua-akfavatar.de.html \
      akfavatar.h.html avtaddons.h.html \
      lua-akfavatar-ref.en.html lua-akfavatar-ref.de.html \
      akfavatar-graphic.en.html akfavatar-graphic.de.html \
      akfavatar-term.en.html akfavatar-term.de.html

txt: lua-akfavatar-ref.en.txt lua-akfavatar-ref.de.txt \
     lua-akfavatar.en.txt lua-akfavatar.de.txt \
     akfavatar-graphic.en.txt akfavatar-graphic.de.txt \
     akfavatar-term.en.txt akfavatar-term.de.txt

pdf: lua-akfavatar.en.pdf lua-akfavatar.de.pdf \
     akfavatar.h.pdf avtaddons.h.pdf \
     lua-akfavatar-ref.en.pdf lua-akfavatar-ref.de.pdf \
     akfavatar-graphic.en.pdf akfavatar-graphic.de.pdf \
     akfavatar-term.en.pdf akfavatar-term.de.pdf

ps: lua-akfavatar.en.ps lua-akfavatar.de.ps \
    akfavatar.h.ps avtaddons.h.ps \
    lua-akfavatar-ref.en.ps lua-akfavatar-ref.de.ps \
    akfavatar-graphic.en.ps akfavatar-graphic.de.ps \
    akfavatar-term.en.ps akfavatar-term.de.ps

dvi: lua-akfavatar.en.dvi lua-akfavatar.de.dvi \
     lua-akfavatar-ref.en.dvi lua-akfavatar-ref.de.dvi \
     akfavatar-graphic.en.dvi akfavatar-graphic.de.dvi \
     akfavatar-term.en.dvi akfavatar-term.de.dvi

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

akfavatar.h.ps: $(srcdir)/akfavatar.h
	-(cd $(srcdir); \
	  $(ENSCRIPT) -E --color -wPostScript -p$(builddir)/$@ akfavatar.h; )

avtaddons.h.ps: $(srcdir)/avtaddons.h
	-(cd $(srcdir); \
	  $(ENSCRIPT) -E --color -wPostScript -p$(builddir)/$@ avtaddons.h; )

akfavatar.h.pdf: akfavatar.h.ps
	$(PS2PDF) akfavatar.h.ps $@

avtaddons.h.pdf: avtaddons.h.ps
	$(PS2PDF) avtaddons.h.ps $@

akfavatar.h.html: $(srcdir)/akfavatar.h
	-(cd $(srcdir); \
	  $(ENSCRIPT) -takfavatar.h -E --color -whtml -p$(builddir)/$@ akfavatar.h; )

avtaddons.h.html: $(srcdir)/avtaddons.h
	-(cd $(srcdir); \
	  $(ENSCRIPT) -tavtaddons.h -E --color -whtml -p$(builddir)/$@ avtaddons.h; )

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
