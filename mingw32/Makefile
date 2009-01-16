# This file is in the Public Domain - AKFoerster

include version

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

# don't use $(CC) becaus it is a cross-compiler
../bin2c: ../bin2c.c
	cc -o $@ ../bin2c.c

../gnu-head.c: ../gnu-head.bmp ../bin2c
	../bin2c avatar_image < ../gnu-head.bmp > $@

../gnu-small.c: ../gnu-small.bmp ../bin2c
	../bin2c avatar_image < ../gnu-small.bmp > $@

../keybtn.c: ../keybtn.bmp ../bin2c
	../bin2c keybtn < ../keybtn.bmp > $@

../bell.c: ../bell.wav ../bin2c
	../bin2c avt_bell_data < ../bell.wav > $@

avatar-default.o: ../avatar-default.c ../gnu-head.c ../gnu-small.c
	$(CC) $(CFLAGS) $(SDLCFLAGS) -c -o $@ ../avatar-default.c

avtfilechooser.o: ../avtfilechooser.c ../akfavatar.h
	$(CC) $(CFLAGS) $(SDLCFLAGS) -c -o $@ ../avtfilechooser.c

avatar.o: ../avatar.c ../akfavatar.h ../version.h \
            ../keybtn.c ../balloonpointer.xpm ../circle.xpm ../akfavatar.xpm
	$(CC) $(CFLAGS) $(SDLCFLAGS) -c ../avatar.c

avatar-audio.o: ../avatar-audio.c ../akfavatar.h ../bell.c
	$(CC) -c $(CFLAGS) $(SDLCFLAGS) -o $@ ../avatar-audio.c

font.o: ../font.c ../ucsfont7x14.c ../ucsfont4x6.c ../ucsfont9x18.c
	$(CC) -c $(CFLAGS) $(SDLCFLAGS) -o $@ ../font.c

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
		$(LDFLAGS) $(SDLLDFLAGS)

avtccio.o: ../avtccio.c ../avtcio.h ../akfavatar.h
	$(CC) $(CFLAGS) $(SDLCFLAGS) -I. -c -o $@ ../avtccio.c

avtcwio.o: ../avtcwio.c ../avtcio.h ../akfavatar.h
	$(CC) $(CFLAGS) $(SDLCFLAGS) -I. -c -o $@ ../avtcwio.c

libavtcio.a: avtccio.o avtcwio.o
	$(AR) rcu $@ avtccio.o avtcwio.o
	-$(RANLIB) $@

avatarsay.o: ../avatarsay.c ../akfavatar.h
	$(CC) $(CFLAGS) $(SDLCFLAGS) -c -o $@ ../avatarsay.c

# from this directory!
avtmsg.o: avtmsg.c ../avtmsg.h
	$(CC) $(CFLAGS) $(SDLCFLAGS) -I.. -c -o $@ avtmsg.c

arch.o: ../arch.c ../arch.h
	$(CC) $(CFLAGS) -c -o $@ ../arch.c

avtwindows.o: avtwindows.c
	$(CC) $(CFLAGS) $(SDLCFLAGS) -I.. -c -o $@ avtwindows.c

avatarsay.exe: avatarsay-d.exe
	mv avatarsay-d.exe avatarsay.exe

# uses dynamic lib
avatarsay-d.exe: avatarsay.o avtfilechooser.o avtmsg.o akfavatar.dll \
	         arch.o avtsayinfo.o avtwindows.o
	$(CC) $(CFLAGS) $(SDLCFLAGS) -o $@ avatarsay.o avtfilechooser.o \
	  avtmsg.o avtsayinfo.o arch.o avtwindows.o \
	  -L. -lakfavatar $(LDFLAGS) $(SDLLDFLAGS)

# uses static lib
avatarsay-s.exe: avatarsay.o avtfilechooser.o avtmsg.o libakfavatar.a \
	       arch.o avtsayinfo.o avtwindows.o
	$(CC) $(CFLAGS) $(SDLCFLAGS) -o $@ avatarsay.o avtfilechooser.o \
	  avtmsg.o avtsayinfo.o arch.o avtwindows.o \
	  libakfavatar.a $(LDFLAGS) $(SDLLDFLAGS)

example: example.exe

example.exe: ../example.c akfavatar.dll icon.o
	$(CC) $(CFLAGS) $(SDLCFLAGS) -o $@ ../example.c icon.o \
			-L. -lakfavatar $(LDFLAGS) $(SDLLDFLAGS)

install: akfavatar.dll libakfavatar.a
	install -m 644 ../akfavatar.h $(includedir)
	install -m 644 ../avtcio.h $(includedir)
	install -m 644 libakfavatar.a libakfavatar.dll.a $(libdir)
	install -m 644 libavtcio.a $(libdir)
	install -m 644 akfavatar.dll $(libdir)

install-strip: akfavatar.dll libakfavatar.a
	install -m 644 ../akfavatar.h $(includedir)
	install -m 644 ../avtcio.h $(includedir)
	install -m 644 libakfavatar.a libakfavatar.dll.a $(libdir)
	install -m 644 libavtcio.a $(libdir)
	install -m 644 -s akfavatar.dll $(libdir)

clean:
	-rm -f *~ *.o *.ppu *.gpi
	-rm -f libinfo.rc avtsayinfo.rc

distclean: clean
	-rm -f *.a *.so *.exe avatarsay-*.zip
	-rm -f akfavatar.def akfavatar.dll akfavatar.lib
	-rm -f version
	-rm -f akfavatar*.zip

# the folowing targets are for the maintainer
# don't use them unless you know what you are doing

multiply.exe: ../pascal/multiply.pas
	-fpcwavatar -dRELEASE -o./$@ ../pascal/multiply.pas

multiplizieren.exe: ../pascal/multiply.pas
	-fpcwavatar -dRELEASE -ddeutsch -o./$@ ../pascal/multiply.pas

dist-bin: avatarsay.exe multiply.exe multiplizieren.exe
	make -C ../doc html
	mkdir $(BINPKGNAME)
	cp ../doc/akfavatar-en.html ../doc/akfavatar-de.html $(BINPKGNAME)/
	cp akfavatar.dll $(BINPKGNAME)/
	#cp libakfavatar.dll.a $(BINPKGNAME)/
	#cp ../akfavatar.h akfavatar.def ../pascal/akfavatar.pas $(BINPKGNAME)/
	cp avatarsay.exe ../fsdemo-?? $(BINPKGNAME)/
	cp fsdemo-??.bat $(BINPKGNAME)/
	cp ../COPYING $(BINPKGNAME)/gpl-3.0.txt
	cp ../AUTHORS $(BINPKGNAME)/AUTHORS.txt
	cp ../pascal/correct.wav ../pascal/wrong.wav $(BINPKGNAME)/
	-cp multiply.exe multiplizieren.exe $(BINPKGNAME)/
	strip $(BINPKGNAME)/avatarsay.exe $(BINPKGNAME)/akfavatar.dll
	todos $(BINPKGNAME)/fsdemo-??
	todos $(BINPKGNAME)/*.bat
	todos $(BINPKGNAME)/*.txt
	-rm -f $(BINPKGNAME).w32.zip
	zip -9vr $(BINPKGNAME).w32.zip $(BINPKGNAME)/
	rm -r $(BINPKGNAME)/
