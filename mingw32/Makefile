# This file is in the Public Domain - AKFoerster

include version

srcdir = ..

TARGET = i586-mingw32msvc
CC = $(TARGET)-gcc
AR = $(TARGET)-ar
WINDRES = $(TARGET)-windres
RANLIB = $(TARGET)-ranlib
CFLAGS = -O2 -Wall -Wextra
LDFLAGS = 

SDL_CONFIG = /usr/$(TARGET)/bin/sdl-config
SDLCFLAGS := $(shell $(SDL_CONFIG) --cflags)
SDLLDFLAGS := $(shell $(SDL_CONFIG) --libs)

# name for binary packages
BINPKGNAME=akfavatar-$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT).bin

VERSIONSTR=$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT)

prefix = /usr/$(TARGET)
libdir = $(prefix)/lib
includedir = $(prefix)/include

SHELL = /bin/sh
.SUFFIXES:

all: akfavatar.dll libakfavatar.a libavtcio.a avatarsay.exe

avtsayinfo.rc: Makefile
	@{ \
	echo "AppIcon ICON \"akfavatar.ico\""; \
	echo; \
	echo "LANGUAGE 7, 1"; \
	echo; \
	echo "1 VERSIONINFO"; \
	echo "FILEVERSION $(VERSION), $(SUBVERSION), $(REVISION), 0"; \
	echo "PRODUCTVERSION $(VERSION), $(SUBVERSION), $(REVISION), 0"; \
	echo "FILEFLAGSMASK 0x3F"; \
	echo "FILEOS 0x4"; \
	echo "FILETYPE 0x1"; \
	echo "BEGIN"; \
	echo " BLOCK \"StringFileInfo\""; \
	echo " BEGIN"; \
	echo "  BLOCK \"040704E4\""; \
	echo "  BEGIN"; \
	echo "   VALUE \"CompanyName\", \"AKFoerster\""; \
	echo "   VALUE \"FileDescription\",";\
	echo "     \"Demo-Programm und lustiger Text-Betrachter\""; \
	echo "   VALUE \"FileVersion\", \"$(VERSIONSTR)\""; \
	echo "   VALUE \"InternalName\", \"avatarsay\""; \
	echo "   VALUE \"LegalCopyright\","; \
	echo "     \"Andreas K. F\366rster, GPLv3+\""; \
	echo "   VALUE \"OriginalFilename\", \"avatarsay.exe\""; \
	echo "   VALUE \"Comments\", "; \
	echo "      \"urspr\374nglich f\374r GNU/Linux geschrieben\""; \
	echo "   VALUE \"ProductName\", \"AKFAvatar\""; \
	echo "   VALUE \"ProductVersion\", \"$(VERSIONSTR)\""; \
	echo "  END"; \
	echo " END"; \
	echo " BLOCK \"VarFileInfo\""; \
	echo "  BEGIN"; \
	echo "   VALUE \"Translation\", 0x407, 1252, 0x0409, 1252"; \
	echo " END"; \
	echo "END"; \
	echo; \
	echo "LANGUAGE 9, 1"; \
	echo; \
	echo "1 VERSIONINFO"; \
	echo "FILEVERSION $(VERSION), $(SUBVERSION), $(REVISION), 0"; \
	echo "PRODUCTVERSION $(VERSION), $(SUBVERSION), $(REVISION), 0"; \
	echo "FILEFLAGSMASK 0x3F"; \
	echo "FILEOS 0x4"; \
	echo "FILETYPE 0x1"; \
	echo "BEGIN"; \
	echo " BLOCK \"StringFileInfo\""; \
	echo " BEGIN"; \
	echo "  BLOCK \"040904E4\""; \
	echo "  BEGIN"; \
	echo "   VALUE \"CompanyName\", \"AKFoerster\""; \
	echo "   VALUE \"FileDescription\",";\
	echo "     \"program for demos and fancy text-viewer\""; \
	echo "   VALUE \"FileVersion\", \"$(VERSIONSTR)\""; \
	echo "   VALUE \"InternalName\", \"avatarsay\""; \
	echo "   VALUE \"LegalCopyright\","; \
	echo "     \"Andreas K. F\366rster, GPLv3+\""; \
	echo "   VALUE \"OriginalFilename\", \"avatarsay.exe\""; \
	echo "   VALUE \"Comments\", \"primarily written for GNU/Linux\""; \
	echo "   VALUE \"ProductName\", \"AKFAvatar\""; \
	echo "   VALUE \"ProductVersion\", \"$(VERSIONSTR)\""; \
	echo "  END"; \
	echo " END"; \
	echo " BLOCK \"VarFileInfo\""; \
	echo "  BEGIN"; \
	echo "   VALUE \"Translation\", 0x409, 1252, 0x0407, 1252"; \
	echo " END"; \
	echo "END"; } > $@

libinfo.rc: Makefile
	@{ \
	echo "LANGUAGE 7, 1"; \
	echo; \
	echo "1 VERSIONINFO"; \
	echo "FILEVERSION $(VERSION), $(SUBVERSION), $(REVISION), 0"; \
	echo "PRODUCTVERSION $(VERSION), $(SUBVERSION), $(REVISION), 0"; \
	echo "FILEFLAGSMASK 0x3F"; \
	echo "FILEOS 0x4"; \
	echo "FILETYPE 0x2"; \
	echo "BEGIN"; \
	echo " BLOCK \"StringFileInfo\""; \
	echo " BEGIN"; \
	echo "  BLOCK \"040704E4\""; \
	echo "  BEGIN"; \
	echo "   VALUE \"CompanyName\", \"AKFoerster\""; \
	echo "   VALUE \"FileDescription\", \"AKFAvatar Bibliothek\""; \
	echo "   VALUE \"FileVersion\", \"$(VERSIONSTR)\""; \
	echo "   VALUE \"InternalName\", \"akfavatar\""; \
	echo "   VALUE \"LegalCopyright\","; \
	echo "     \"Andreas K. F\366rster, GPLv3+\""; \
	echo "   VALUE \"OriginalFilename\", \"akfavatar.dll\""; \
	echo "   VALUE \"Comments\", "; \
	echo "      \"urspr\374nglich f\374r GNU/Linux geschrieben\""; \
	echo "   VALUE \"ProductName\", \"AKFAvatar\""; \
	echo "   VALUE \"ProductVersion\", \"$(VERSIONSTR)\""; \
	echo "  END"; \
	echo " END"; \
	echo " BLOCK \"VarFileInfo\""; \
	echo "  BEGIN"; \
	echo "   VALUE \"Translation\", 0x407, 1252, 0x0409, 1252"; \
	echo " END"; \
	echo "END"; \
	echo; \
	echo "LANGUAGE 9, 1"; \
	echo; \
	echo "1 VERSIONINFO"; \
	echo "FILEVERSION $(VERSION), $(SUBVERSION), $(REVISION), 0"; \
	echo "PRODUCTVERSION $(VERSION), $(SUBVERSION), $(REVISION), 0"; \
	echo "FILEFLAGSMASK 0x3F"; \
	echo "FILEOS 0x4"; \
	echo "FILETYPE 0x2"; \
	echo "BEGIN"; \
	echo " BLOCK \"StringFileInfo\""; \
	echo " BEGIN"; \
	echo "  BLOCK \"040904E4\""; \
	echo "  BEGIN"; \
	echo "   VALUE \"CompanyName\", \"AKFoerster\""; \
	echo "   VALUE \"FileDescription\", \"AKFAvatar library\""; \
	echo "   VALUE \"FileVersion\", \"$(VERSIONSTR)\""; \
	echo "   VALUE \"InternalName\", \"akfavatar\""; \
	echo "   VALUE \"LegalCopyright\","; \
	echo "     \"Andreas K. F\366rster, GPLv3+\""; \
	echo "   VALUE \"OriginalFilename\", \"akfavatar.dll\""; \
	echo "   VALUE \"Comments\", \"primarily written for GNU/Linux\""; \
	echo "   VALUE \"ProductName\", \"AKFAvatar\""; \
	echo "   VALUE \"ProductVersion\", \"$(VERSIONSTR)\""; \
	echo "  END"; \
	echo " END"; \
	echo " BLOCK \"VarFileInfo\""; \
	echo "  BEGIN"; \
	echo "   VALUE \"Translation\", 0x409, 1252, 0x0407, 1252"; \
	echo " END"; \
	echo "END"; } > $@

icon.o: icon.rc akfavatar.ico
	$(WINDRES) -o $@ -i icon.rc

avtsayinfo.o: avtsayinfo.rc akfavatar.ico
	$(WINDRES) -o $@ -i avtsayinfo.rc

libinfo.o: libinfo.rc
	$(WINDRES) -o $@ -i libinfo.rc

# don't use $(CC) because it is a cross-compiler
bin2c: $(srcdir)/bin2c.c
	cc -o $@ $(srcdir)/bin2c.c

# don't use $(CC) because it is a cross-compiler
bdfread: $(srcdir)/bdfread.c
	cc -o $@ $(srcdir)/bdfread.c

gnu-head.c: $(srcdir)/gnu-head.bmp bin2c
	./bin2c avatar_image < $(srcdir)/gnu-head.bmp > $@

# bell.au should be in the sourcecode-distribution
$(srcdir)/bell.au: $(srcdir)/bell.flac
	@echo "create headerless old AU file"
	sox $(srcdir)/bell.flac -t .ul --rate 8000 --channels 1 $@

bell.c: $(srcdir)/bell.au bin2c
	./bin2c avt_bell_data < $(srcdir)/bell.au > $@

avatar-default.o: $(srcdir)/avatar-default.c gnu-head.c
	$(CC) $(SDLCFLAGS) $(CFLAGS) -I. -c -o $@ $(srcdir)/avatar-default.c

avtfilechooser.o: $(srcdir)/avtfilechooser.c $(srcdir)/akfavatar.h
	$(CC) $(SDLCFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/avtfilechooser.c

avatar.o: $(srcdir)/avatar.c $(srcdir)/akfavatar.h $(srcdir)/version.h \
             $(srcdir)/btn-cont.xpm $(srcdir)/balloonpointer.xpm \
             $(srcdir)/circle.xpm $(srcdir)/akfavatar.xpm
	$(CC) $(SDLCFLAGS) $(CFLAGS) -c $(srcdir)/avatar.c

avatar-audio.o: $(srcdir)/avatar-audio.c $(srcdir)/akfavatar.h bell.c
	$(CC) -c $(SDLCFLAGS) $(CFLAGS) -I. -o $@ $(srcdir)/avatar-audio.c

ucsfont7x14.c: $(srcdir)/7x14.bdf bdfread
	./bdfread $(srcdir)/7x14.bdf > $@

ucsfont9x18.c: $(srcdir)/9x18.bdf bdfread
	./bdfread $(srcdir)/9x18.bdf > $@

font.o: $(srcdir)/font.c ucsfont7x14.c ucsfont9x18.c
	$(CC) -c $(SDLCFLAGS) $(CFLAGS) -I. -o $@ $(srcdir)/font.c

# static library
libakfavatar.a: avatar.o avatar-default.o avatar-audio.o font.o
	$(AR) rcu $@ avatar.o avatar-default.o avatar-audio.o font.o
	$(RANLIB) $@

akfavatar.dll: avatar.o avatar-default.o avatar-audio.o font.o libinfo.o 
	$(CC) -shared -o $@ avatar.o avatar-default.o avatar-audio.o \
	        font.o libinfo.o \
	        -Wl,-no-undefined,--enable-runtime-pseudo-reloc \
	        -Wl,--output-def,akfavatar.def \
	        -Wl,--out-implib,libakfavatar.dll.a \
	        $(SDLLDFLAGS) $(LDFLAGS)

avtccio.o: $(srcdir)/avtccio.c $(srcdir)/avtcio.h $(srcdir)/akfavatar.h
	$(CC) $(SDLCFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/avtccio.c

avtcwio.o: $(srcdir)/avtcwio.c $(srcdir)/avtcio.h $(srcdir)/akfavatar.h
	$(CC) $(SDLCFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/avtcwio.c

libavtcio.a: avtccio.o avtcwio.o
	$(AR) rcu $@ avtccio.o avtcwio.o
	-$(RANLIB) $@

avatarsay.o: $(srcdir)/avatarsay.c $(srcdir)/akfavatar.h
	$(CC) $(SDLCFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/avatarsay.c

# from the mingw32 directory!
avtmsg.o: $(srcdir)/mingw32/avtmsg.c $(srcdir)/avtmsg.h
	$(CC) $(SDLCFLAGS) $(CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/mingw32/avtmsg.c

avtwindows.o: $(srcdir)/mingw32/avtwindows.c
	$(CC) $(SDLCFLAGS) $(CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/mingw32/avtwindows.c

arch.o: $(srcdir)/arch.c $(srcdir)/arch.h
	$(CC) $(CFLAGS) -c -o $@ $(srcdir)/arch.c

avatarsay: avatarsay.exe

avatarsay-d: avatarsay-d.exe

avatarsay-s: avatarsay-s.exe

avatarsay.exe: avatarsay-d.exe
	mv avatarsay-d.exe avatarsay.exe

# uses dynamic lib
avatarsay-d.exe: avatarsay.o avtfilechooser.o avtmsg.o akfavatar.dll \
	         arch.o avtsayinfo.o avtwindows.o
	$(CC) $(SDLCFLAGS) $(CFLAGS) -o $@ avatarsay.o avtfilechooser.o \
	  avtmsg.o avtsayinfo.o arch.o avtwindows.o \
	  -L. -lakfavatar $(SDLLDFLAGS) $(LDFLAGS)

# uses static lib
avatarsay-s.exe: avatarsay.o avtfilechooser.o avtmsg.o libakfavatar.a \
	       arch.o avtsayinfo.o avtwindows.o
	$(CC) $(SDLCFLAGS) $(CFLAGS) -o $@ avatarsay.o avtfilechooser.o \
	  avtmsg.o avtsayinfo.o arch.o avtwindows.o \
	  libakfavatar.a $(SDLLDFLAGS) $(LDFLAGS)

example: example.exe

example.exe: $(srcdir)/example.c akfavatar.dll icon.o
	$(CC) $(SDLCFLAGS) $(CFLAGS) -o $@ $(srcdir)/example.c icon.o \
			-L. -lakfavatar $(SDLLDFLAGS) $(LDFLAGS)

install: akfavatar.dll libakfavatar.a
	install -m 644 $(srcdir)/akfavatar.h $(includedir)
	install -m 644 $(srcdir)/avtcio.h $(includedir)
	install -m 644 libakfavatar.a libakfavatar.dll.a $(libdir)
	install -m 644 libavtcio.a $(libdir)
	install -m 644 akfavatar.dll $(libdir)

install-strip: akfavatar.dll libakfavatar.a
	install -m 644 $(srcdir)/akfavatar.h $(includedir)
	install -m 644 $(srcdir)/avtcio.h $(includedir)
	install -m 644 libakfavatar.a libakfavatar.dll.a $(libdir)
	install -m 644 libavtcio.a $(libdir)
	install -m 644 -s akfavatar.dll $(libdir)

clean:
	-rm -f *~ *.o *.ppu *.gpi
	-rm -f libinfo.rc avtsayinfo.rc
	-rm -f bell.c gnu-head.c
	-rm -f ucsfont7x14.c ucsfont9x18.c
	-rm -f bin2c bin2c.exe
	-rm -f bdfread bdfread.exe

distclean: clean
	-rm -f *.a *.so *.exe avatarsay-*.zip
	-rm -f akfavatar.def akfavatar.dll akfavatar.lib
	-rm -f version
	-rm -f akfavatar*.zip

# the folowing targets are for the maintainer
# don't use them unless you know what you are doing

multiply.exe: $(srcdir)/pascal/multiply.pas
	-fpcwavatar -dRELEASE -o$@ $(srcdir)/pascal/multiply.pas

multiplizieren.exe: $(srcdir)/pascal/multiply.pas
	-fpcwavatar -dRELEASE -ddeutsch -o$@ $(srcdir)/pascal/multiply.pas

dist-bin: avatarsay.exe multiply.exe multiplizieren.exe
	make -C $(srcdir)/doc html
	mkdir $(BINPKGNAME)
	cp $(srcdir)/doc/akfavatar-en.html $(srcdir)/doc/akfavatar-de.html $(BINPKGNAME)/
	cp akfavatar.dll $(BINPKGNAME)/
	#cp libakfavatar.dll.a $(BINPKGNAME)/
	#cp $(srcdir)/akfavatar.h akfavatar.def $(srcdir)/pascal/akfavatar.pas $(BINPKGNAME)/
	cp avatarsay.exe $(srcdir)/fsdemo-?? $(BINPKGNAME)/
	cp fsdemo-??.bat $(BINPKGNAME)/
	cp $(srcdir)/COPYING $(BINPKGNAME)/gpl-3.0.txt
	cp $(srcdir)/AUTHORS $(BINPKGNAME)/AUTHORS.txt
	cp $(srcdir)/pascal/correct.wav $(srcdir)/pascal/wrong.wav $(BINPKGNAME)/
	-cp multiply.exe multiplizieren.exe $(BINPKGNAME)/
	strip $(BINPKGNAME)/avatarsay.exe $(BINPKGNAME)/akfavatar.dll
	todos $(BINPKGNAME)/fsdemo-??
	todos $(BINPKGNAME)/*.bat
	todos $(BINPKGNAME)/*.txt
	-rm -f $(BINPKGNAME).w32.zip
	zip -9vr $(BINPKGNAME).w32.zip $(BINPKGNAME)/
	rm -r $(BINPKGNAME)/
