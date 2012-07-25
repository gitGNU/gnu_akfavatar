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
      akfavatar-term.en.html akfavatar-term.de.html \
      base64.en.html base64.de.html \
      akfavatar.utf8.en.html akfavatar.utf8.de.html \
      function_plotter.en.html function_plotter.de.html \
      interactive_lua.en.html interactive_lua.de.html \
      audioplayer.en.html audioplayer.de.html

txt: lua-akfavatar-ref.en.txt lua-akfavatar-ref.de.txt \
     lua-akfavatar.en.txt lua-akfavatar.de.txt \
     akfavatar-graphic.en.txt akfavatar-graphic.de.txt \
     akfavatar-term.en.txt akfavatar-term.de.txt \
     base64.en.txt base64.de.txt \
     akfavatar.utf8.en.txt akfavatar.utf8.de.txt \
     function_plotter.en.txt function_plotter.de.txt \
     interactive_lua.en.txt interactive_lua.de.txt \
     audioplayer.en.txt audioplayer.de.txt

pdf: lua-akfavatar.en.pdf lua-akfavatar.de.pdf \
     akfavatar.h.pdf avtaddons.h.pdf \
     lua-akfavatar-ref.en.pdf lua-akfavatar-ref.de.pdf \
     akfavatar-graphic.en.pdf akfavatar-graphic.de.pdf \
     akfavatar-term.en.pdf akfavatar-term.de.pdf \
     base64.en.pdf base64.de.pdf \
     akfavatar.utf8.en.pdf akfavatar.utf8.de.pdf \
     function_plotter.en.pdf function_plotter.de.pdf \
     interactive_lua.en.pdf interactive_lua.de.pdf \
     audioplayer.en.pdf audioplayer.de.pdf

ps: lua-akfavatar.en.ps lua-akfavatar.de.ps \
    akfavatar.h.ps avtaddons.h.ps \
    lua-akfavatar-ref.en.ps lua-akfavatar-ref.de.ps \
    akfavatar-graphic.en.ps akfavatar-graphic.de.ps \
    akfavatar-term.en.ps akfavatar-term.de.ps \
    base64.en.ps base64.de.ps \
    akfavatar.utf8.en.ps akfavatar.utf8.de.ps \
    function_plotter.en.ps function_plotter.de.ps \
    interactive_lua.en.ps interactive_lua.de.ps \
    audioplayer.en.ps audioplayer.de.ps

dvi: lua-akfavatar.en.dvi lua-akfavatar.de.dvi \
     lua-akfavatar-ref.en.dvi lua-akfavatar-ref.de.dvi \
     akfavatar-graphic.en.dvi akfavatar-graphic.de.dvi \
     akfavatar-term.en.dvi akfavatar-term.de.dvi \
     base64.en.dvi base64.de.dvi \
     akfavatar.utf8.en.dvi akfavatar.utf8.de.dvi \
     function_plotter.en.dvi function_plotter.de.dvi \
     interactive_lua.en.dvi interactive_lua.de.dvi \
     audioplayer.en.dvi audioplayer.de.dvi

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
