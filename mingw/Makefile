# This file is in the Public Domain - AKFoerster

srcdir = ..

CC = gcc
AR = ar
AWK = awk
STRIP = strip
WINDRES = windres
RANLIB = ranlib
SDL_CONFIG = sdl-config

CFLAGS = -O2 -Wall -Wextra
LDFLAGS =

prefix = /usr/$(TARGET)

include config-mingw

libdir = $(prefix)/lib
includedir = $(prefix)/include

SDL_CFLAGS := $(shell $(SDL_CONFIG) --cflags)
SDL_LDFLAGS := $(shell $(SDL_CONFIG) --libs)

# name for binary packages
BINPKGNAME=akfavatar-$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT).bin

VERSIONSTR=$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT)

# if you change this, you also have to change the file avatar-default.c
default_avatar = $(srcdir)/male_user.xpm

SHELL = /bin/sh
.SUFFIXES:

all: akfavatar.dll libakfavatar.a libavtaddons.a avatarsay.exe example.exe

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

icon.o: $(srcdir)/mingw/icon.rc $(srcdir)/mingw/akfavatar.ico
	$(WINDRES) -o $@ -i $(srcdir)/mingw/icon.rc

avtsayinfo.o: avtsayinfo.rc $(srcdir)/mingw/akfavatar.ico
	$(WINDRES) -I$(srcdir)/mingw -o $@ -i avtsayinfo.rc

libinfo.o: libinfo.rc
	$(WINDRES) -o $@ -i libinfo.rc

# alert.au should be in the sourcecode-distribution
$(srcdir)/alert.au: $(srcdir)/alert.flac
	@echo "create headerless old AU file"
	sox $(srcdir)/alert.flac -t .ul --rate 8000 --channels 1 $@

alert.c: $(srcdir)/alert.au
	$(AWK) -f $(srcdir)/data2c -v name=avt_alert_data \
	  $(srcdir)/alert.au > $@

avatar-default.o: $(srcdir)/avatar-default.c $(default_avatar)
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -I. -c -o $@ $(srcdir)/avatar-default.c

filechooser.o: $(srcdir)/filechooser.c \
                 $(srcdir)/akfavatar.h $(srcdir)/avtaddons.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/filechooser.c

colorchooser.o: $(srcdir)/colorchooser.c $(srcdir)/akfavatar.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/colorchooser.c

rgb.h: $(srcdir)/rgb.txt
	$(AWK) -f $(srcdir)/rgb2c $(srcdir)/rgb.txt > $@

avatar.o: $(srcdir)/avatar.c $(srcdir)/akfavatar.h version.h rgb.h \
             $(srcdir)/btn.xpm $(srcdir)/btn_yes.xbm $(srcdir)/btn_no.xbm \
             $(srcdir)/btn_up.xbm $(srcdir)/btn_down.xbm \
             $(srcdir)/btn_left.xbm $(srcdir)/btn_right.xbm \
             $(srcdir)/btn_ff.xbm $(srcdir)/btn_fb.xbm \
             $(srcdir)/btn_pause.xbm $(srcdir)/btn_stop.xbm \
             $(srcdir)/btn_help.xbm $(srcdir)/btn_cancel.xbm \
             $(srcdir)/btn_eject.xbm $(srcdir)/btn_circle.xbm \
             $(srcdir)/balloonpointer.xbm $(srcdir)/circle.xbm \
             $(srcdir)/akfavatar.xpm
	$(CC) -I. $(SDL_CFLAGS) $(CFLAGS) -c $(srcdir)/avatar.c

avatar-audio.o: $(srcdir)/avatar-audio.c $(srcdir)/akfavatar.h alert.c
	$(CC) -c $(SDL_CFLAGS) $(CFLAGS) -I. -o $@ $(srcdir)/avatar-audio.c

ucsfont7x14.c: $(srcdir)/7x14.bdf
	$(AWK) -f $(srcdir)/bdf2c $(srcdir)/7x14.bdf > $@

ucsfont9x18.c: $(srcdir)/9x18.bdf
	$(AWK) -f $(srcdir)/bdf2c $(srcdir)/9x18.bdf > $@

font.o: $(srcdir)/font.c ucsfont7x14.c ucsfont9x18.c
	$(CC) -c $(SDL_CFLAGS) $(CFLAGS) -I. -o $@ $(srcdir)/font.c

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
	        $(SDL_LDFLAGS) $(LDFLAGS)

avtwindows.o: $(srcdir)/mingw/avtwindows.c
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/mingw/avtwindows.c

askdrive.o: $(srcdir)/mingw/askdrive.c \
	    $(srcdir)/avtaddons.h $(srcdir)/akfavatar.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/mingw/askdrive.c

# this is just a dummy stub!
avttermstub.o: $(srcdir)/mingw/avttermstub.c $(srcdir)/avtaddons.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -I$(srcdir) -c -o $@ $(srcdir)/mingw/avttermstub.c

arch.o: $(srcdir)/arch.c $(srcdir)/avtaddons.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/arch.c

avtccio.o: $(srcdir)/avtccio.c $(srcdir)/avtaddons.h $(srcdir)/akfavatar.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/avtccio.c

avtcwio.o: $(srcdir)/avtcwio.c $(srcdir)/avtaddons.h $(srcdir)/akfavatar.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/avtcwio.c

avtreadfile.o: $(srcdir)/avtreadfile.c $(srcdir)/avtaddons.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/avtreadfile.c

libavtaddons.a: filechooser.o askdrive.o arch.o avttermstub.o \
                avtccio.o avtcwio.o avtmsg.o avtreadfile.o colorchooser.o
	$(AR) rcu $@ filechooser.o askdrive.o arch.o avttermstub.o \
	             avtccio.o avtcwio.o avtmsg.o avtreadfile.o \
	             colorchooser.o
	-$(RANLIB) $@

avatarsay.o: $(srcdir)/avatarsay.c $(srcdir)/akfavatar.h
	$(CC) -I. $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/avatarsay.c

# from the mingw directory!
avtmsg.o: $(srcdir)/mingw/avtmsg.c $(srcdir)/avtaddons.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/mingw/avtmsg.c

avatarsay: avatarsay.exe

avatarsay-d: avatarsay-d.exe

avatarsay-s: avatarsay-s.exe

avatarsay.exe: avatarsay-d.exe
	cp avatarsay-d.exe avatarsay.exe

# uses dynamic lib
avatarsay-d.exe: avatarsay.o libavtaddons.a akfavatar.dll \
	         avtwindows.o avtsayinfo.o
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -o $@ avatarsay.o \
	  avtwindows.o avtsayinfo.o \
	  -L. -lavtaddons -lakfavatar $(SDL_LDFLAGS) $(LDFLAGS)

# uses static lib
avatarsay-s.exe: avatarsay.o libavtaddons.a libakfavatar.a \
	       avtwindows.o avtsayinfo.o
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -o $@ avatarsay.o \
	  avtwindows.o  avtsayinfo.o \
	  libavtaddons.a libakfavatar.a $(SDL_LDFLAGS) $(LDFLAGS)

example: example.exe

example.exe: $(srcdir)/example.c akfavatar.dll icon.o
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -o $@ $(srcdir)/example.c icon.o \
			-L. -lakfavatar $(SDL_LDFLAGS) $(LDFLAGS)

install: akfavatar.dll libakfavatar.a
	install -m 644 $(srcdir)/akfavatar.h $(includedir)
	install -m 644 $(srcdir)/avtaddons.h $(includedir)
	install -m 644 libakfavatar.a libakfavatar.dll.a $(libdir)
	install -m 644 libavtaddons.a $(libdir)
	install -m 644 akfavatar.dll $(libdir)

install-strip: akfavatar.dll libakfavatar.a
	install -m 644 $(srcdir)/akfavatar.h $(includedir)
	install -m 644 $(srcdir)/avtaddons.h $(includedir)
	install -m 644 libakfavatar.a libakfavatar.dll.a $(libdir)
	install -m 644 libavtaddons.a $(libdir)
	install -m 644 akfavatar.dll $(libdir)
	$(STRIP) $(libdir)/akfavatar.dll

clean:
	-rm -f *~ *.o *.ppu *.gpi
	-rm -f libinfo.rc avtsayinfo.rc
	-rm -f alert.c
	-rm -f rgb.h
	-rm -f ucsfont7x14.c ucsfont9x18.c
	-rm -f bdfread bdfread.exe

distclean: clean
	-rm -f *.a *.so *.exe avatarsay-*.zip
	-rm -f akfavatar.def akfavatar.dll akfavatar.lib
	-rm -f config.mak
	-rm -f akfavatar*.zip

# the folowing targets are for the maintainer
# don't use them unless you know what you are doing

multiply.exe: $(srcdir)/pascal/multiply.pas
	-fpcwavatar -dRELEASE -o$@ $(srcdir)/pascal/multiply.pas

multiplizieren.exe: $(srcdir)/pascal/multiply.pas
	-fpcwavatar -dRELEASE -ddeutsch -o$@ $(srcdir)/pascal/multiply.pas

dist-bin: avatarsay.exe multiply.exe multiplizieren.exe
	$(MAKE) -f $(srcdir)/doc/Makefile srcdir=$(srcdir) html txt
	mkdir $(BINPKGNAME)
	cp $(srcdir)/doc/akfavatar-en.html $(srcdir)/doc/akfavatar-de.html $(BINPKGNAME)/
	cp akfavatar-en.txt akfavatar-de.txt $(BINPKGNAME)/
	cp akfavatar.dll $(BINPKGNAME)/
	#cp libakfavatar.dll.a $(BINPKGNAME)/
	#cp $(srcdir)/akfavatar.h akfavatar.def $(srcdir)/pascal/akfavatar.pas $(BINPKGNAME)/
	cp avatarsay.exe $(srcdir)/fsdemo-??.avt $(BINPKGNAME)/
	cp $(srcdir)/gnu-head.xpm $(srcdir)/gnu-head.xbm $(BINPKGNAME)/
	cp $(srcdir)/COPYING $(BINPKGNAME)/gpl-3.0.txt
	cp $(srcdir)/AUTHORS $(BINPKGNAME)/AUTHORS.txt
	cp $(srcdir)/pascal/positive.au $(srcdir)/pascal/negative.au $(BINPKGNAME)/
	-cp multiply.exe multiplizieren.exe $(BINPKGNAME)/
	cp $(srcdir)/teacher.xpm $(BINPKGNAME)/
	cp $(srcdir)/female_user.xpm $(srcdir)/male_user.xpm $(BINPKGNAME)/
	#$(STRIP) $(BINPKGNAME)/avatarsay.exe $(BINPKGNAME)/akfavatar.dll
	todos $(BINPKGNAME)/fsdemo-??.avt
	todos $(BINPKGNAME)/*.bat
	todos $(BINPKGNAME)/*.txt
	-rm -f $(BINPKGNAME).w32.zip
	zip -9vr $(BINPKGNAME).w32.zip $(BINPKGNAME)/
	rm -r $(BINPKGNAME)/
