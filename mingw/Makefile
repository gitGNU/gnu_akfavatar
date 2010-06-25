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

# link with dynamic lib (different names!)
LUA_CFLAGS =
LUA_LDFLAGS = -llua51

# name for binary packages
BINPKGNAME=akfavatar-$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT).bin

VERSIONSTR=$(VERSION).$(SUBVERSION).$(REVISION)$(VARIANT)

# if you change this, you also have to change the file avatar-default.c
default_avatar = $(srcdir)/male_user.xpm

SHELL = /bin/sh
.SUFFIXES:

all: akfavatar.dll libakfavatar.a libavtaddons.a avatarsay.exe example.exe \
     lua-akfavatar.exe

avtsayinfo.rc: $(srcdir)/mingw/avtsayinfo.in
	sed -e "s/@VERSION@/$(VERSION)/; s/@SUBVERSION@/$(SUBVERSION)/" \
	  -e "s/@REVISION@/$(REVISION)/; s/@VERSIONSTR@/$(VERSIONSTR)/" \
	  $(srcdir)/mingw/avtsayinfo.in > $@

libinfo.rc: $(srcdir)/mingw/libinfo.in
	sed -e "s/@VERSION@/$(VERSION)/; s/@SUBVERSION@/$(SUBVERSION)/" \
	  -e "s/@REVISION@/$(REVISION)/; s/@VERSIONSTR@/$(VERSIONSTR)/" \
	  $(srcdir)/mingw/libinfo.in > $@

icon.wo: $(srcdir)/mingw/icon.rc $(srcdir)/mingw/akfavatar.ico
	 $(WINDRES) -o $@ -i $(srcdir)/mingw/icon.rc

avtsayinfo.wo: avtsayinfo.rc $(srcdir)/mingw/akfavatar.ico
	$(WINDRES) -I$(srcdir)/mingw -o $@ -i avtsayinfo.rc

libinfo.wo: libinfo.rc
	$(WINDRES) -o $@ -i libinfo.rc

# alert.au should be in the sourcecode-distribution
$(srcdir)/alert.au: $(srcdir)/alert.flac
	@echo "create headerless old AU file"
	sox $(srcdir)/alert.flac -t .ul --rate 8000 --channels 1 $@

alert.c: $(srcdir)/alert.au
	$(AWK) -f $(srcdir)/data2c -v name=avt_alert_data \
	  $(srcdir)/alert.au > $@

avatar-default.wo: $(srcdir)/avatar-default.c $(default_avatar)
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -I. -c -o $@ $(srcdir)/avatar-default.c

filechooser.wo: $(srcdir)/filechooser.c \
                 $(srcdir)/akfavatar.h $(srcdir)/avtaddons.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/filechooser.c

colorchooser.wo: $(srcdir)/colorchooser.c $(srcdir)/akfavatar.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/colorchooser.c

rgb.h: $(srcdir)/rgb.txt
	$(AWK) -f $(srcdir)/rgb2c $(srcdir)/rgb.txt > $@

avatar.wo: $(srcdir)/avatar.c $(srcdir)/akfavatar.h version.h rgb.h \
             $(srcdir)/btn.xpm $(srcdir)/btn_yes.xbm $(srcdir)/btn_no.xbm \
             $(srcdir)/btn_up.xbm $(srcdir)/btn_down.xbm \
             $(srcdir)/btn_left.xbm $(srcdir)/btn_right.xbm \
             $(srcdir)/btn_ff.xbm $(srcdir)/btn_fb.xbm \
             $(srcdir)/btn_pause.xbm $(srcdir)/btn_stop.xbm \
             $(srcdir)/btn_help.xbm $(srcdir)/btn_cancel.xbm \
             $(srcdir)/btn_eject.xbm $(srcdir)/btn_circle.xbm \
             $(srcdir)/balloonpointer.xbm $(srcdir)/circle.xbm \
             $(srcdir)/akfavatar.xpm
	$(CC) -c -I. $(SDL_CFLAGS) $(CFLAGS) -o $@ $(srcdir)/avatar.c

avatar-audio.wo: $(srcdir)/avatar-audio.c $(srcdir)/akfavatar.h alert.c
	$(CC) -c $(SDL_CFLAGS) $(CFLAGS) -I. -o $@ $(srcdir)/avatar-audio.c

ucsfont7x14.c: $(srcdir)/7x14.bdf
	$(AWK) -f $(srcdir)/bdf2c $(srcdir)/7x14.bdf > $@

ucsfont9x18.c: $(srcdir)/9x18.bdf
	$(AWK) -f $(srcdir)/bdf2c $(srcdir)/9x18.bdf > $@

font.wo: $(srcdir)/font.c ucsfont7x14.c ucsfont9x18.c
	$(CC) -c $(SDL_CFLAGS) $(CFLAGS) -I. -o $@ $(srcdir)/font.c

# static library
libakfavatar.a: avatar.wo avatar-default.wo avatar-audio.wo font.wo
	$(AR) rcu $@ avatar.wo avatar-default.wo avatar-audio.wo font.wo
	$(RANLIB) $@

akfavatar.dll: avatar.wo avatar-default.wo avatar-audio.wo font.wo libinfo.wo 
	$(CC) -shared -o $@ avatar.wo avatar-default.wo avatar-audio.wo \
	        font.wo libinfo.wo \
	        -Wl,-no-undefined,--enable-runtime-pseudo-reloc \
	        -Wl,--output-def,akfavatar.def \
	        -Wl,--out-implib,libakfavatar.dll.a \
	        $(SDL_LDFLAGS) $(LDFLAGS)

avtwindows.wo: $(srcdir)/mingw/avtwindows.c
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/mingw/avtwindows.c

askdrive.wo: $(srcdir)/mingw/askdrive.c \
	     $(srcdir)/avtaddons.h $(srcdir)/akfavatar.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/mingw/askdrive.c

# this is just a dummy stub!
avttermstub.wo: $(srcdir)/mingw/avttermstub.c $(srcdir)/avtaddons.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -I$(srcdir) -c -o $@ $(srcdir)/mingw/avttermstub.c

arch.wo: $(srcdir)/arch.c $(srcdir)/avtaddons.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/arch.c

avtccio.wo: $(srcdir)/avtccio.c $(srcdir)/avtaddons.h $(srcdir)/akfavatar.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/avtccio.c

avtcwio.wo: $(srcdir)/avtcwio.c $(srcdir)/avtaddons.h $(srcdir)/akfavatar.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/avtcwio.c

avtreadfile.wo: $(srcdir)/avtreadfile.c $(srcdir)/avtaddons.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/avtreadfile.c

libavtaddons.a: filechooser.wo askdrive.wo arch.wo avttermstub.wo \
                avtccio.wo avtcwio.wo avtmsg.wo avtreadfile.wo colorchooser.wo
	$(AR) rcu $@ filechooser.wo askdrive.wo arch.wo avttermstub.wo \
	             avtccio.wo avtcwio.wo avtmsg.wo avtreadfile.wo \
	             colorchooser.wo
	-$(RANLIB) $@

avatarsay.wo: $(srcdir)/avatarsay.c $(srcdir)/akfavatar.h
	$(CC) -I. $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/avatarsay.c

# from the mingw directory!
avtmsg.wo: $(srcdir)/mingw/avtmsg.c $(srcdir)/avtaddons.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/mingw/avtmsg.c

avatarsay: avatarsay.exe

avatarsay-d: avatarsay-d.exe

avatarsay-s: avatarsay-s.exe

avatarsay.exe: avatarsay-d.exe
	cp avatarsay-d.exe avatarsay.exe

# uses dynamic lib
avatarsay-d.exe: avatarsay.wo libavtaddons.a akfavatar.dll \
	         avtwindows.wo avtsayinfo.wo
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -o $@ avatarsay.wo \
	  avtwindows.wo avtsayinfo.wo \
	  -L. -lavtaddons -lakfavatar $(SDL_LDFLAGS) $(LDFLAGS)

# uses static lib
avatarsay-s.exe: avatarsay.wo libavtaddons.a libakfavatar.a \
	       avtwindows.wo avtsayinfo.wo
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -o $@ avatarsay.wo \
	  avtwindows.wo  avtsayinfo.wo \
	  libavtaddons.a libakfavatar.a $(SDL_LDFLAGS) $(LDFLAGS)

example: example.exe

example.exe: $(srcdir)/example.c akfavatar.dll icon.wo
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -o $@ $(srcdir)/example.c icon.wo \
			-L. -lakfavatar $(SDL_LDFLAGS) $(LDFLAGS)

lua-akfavatar.exe: lua-akfavatar-prg.wo lua-avt.wo luaavtstarter-rc.wo \
		libavtaddons.a
	$(CC) -o $@ lua-akfavatar-prg.wo lua-avt.wo luaavtstarter-rc.wo \
	    -L. -lavtaddons -lakfavatar \
	    $(LUA_LDFLAGS) $(SDL_LDFLAGS) $(LDFLAGS)

lua-avt.wo: $(srcdir)/lua/lua-avt.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(LUA_CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/lua/lua-avt.c

lua-akfavatar-prg.wo: $(srcdir)/lua/lua-akfavatar-prg.c
	$(CC) $(CFLAGS) $(LUA_CFLAGS) $(SDL_CFLAGS) -I$(srcdir) -c -o $@ \
	 $(srcdir)/lua/lua-akfavatar-prg.c

lua-akfavatar.rc: $(srcdir)/mingw/lua-akfavatar-rc.in
	sed -e "s/@VERSION@/$(VERSION)/; s/@SUBVERSION@/$(SUBVERSION)/" \
	  -e "s/@REVISION@/$(REVISION)/; s/@VERSIONSTR@/$(VERSIONSTR)/" \
	  $(srcdir)/mingw/lua-akfavatar-rc.in > $@

luaavtstarter.rc: $(srcdir)/mingw/luaavtstarter-rc.in
	sed -e "s/@VERSION@/$(VERSION)/; s/@SUBVERSION@/$(SUBVERSION)/" \
	  -e "s/@REVISION@/$(REVISION)/; s/@VERSIONSTR@/$(VERSIONSTR)/" \
	  $(srcdir)/mingw/luaavtstarter-rc.in > $@

luaavtstarter-rc.wo: luaavtstarter.rc $(srcdir)/mingw/akfavatar.ico
	$(WINDRES) -I $(srcdir)/mingw/ -o $@ -i luaavtstarter.rc

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
	-fpcwavatar -dRELEASE -o$@ -Fi$(srcdir)/pascal \
	  $(srcdir)/pascal/multiply.pas

multiplizieren.exe: $(srcdir)/pascal/multiply.pas
	-fpcwavatar -dRELEASE -ddeutsch -o$@ -Fi$(srcdir)/pascal \
	  $(srcdir)/pascal/multiply.pas

dist-bin: avatarsay.exe
	$(MAKE) -f $(srcdir)/doc/Makefile srcdir=$(srcdir) html txt
	mkdir $(BINPKGNAME)
	cp $(srcdir)/doc/akfavatar-en.html $(srcdir)/doc/akfavatar-de.html $(BINPKGNAME)/
	cp akfavatar-en.txt akfavatar-de.txt $(BINPKGNAME)/
	cp akfavatar.dll $(BINPKGNAME)/
	cp avatarsay.exe $(srcdir)/fsdemo-??.avt $(BINPKGNAME)/
	-cp lua-akfavatar.exe $(BINPKGNAME)/
	cp $(srcdir)/lua/multiply.lua $(srcdir)/lua/interactive.lua $(BINPKGNAME)/
	cp $(srcdir)/lua/textview.lua $(BINPKGNAME)/
	cp $(srcdir)/lua/hello.lua $(BINPKGNAME)/
	cp $(srcdir)/gnu-head.xpm $(srcdir)/gnu-head.xbm $(BINPKGNAME)/
	cp $(srcdir)/COPYING $(BINPKGNAME)/gpl-3.0.txt
	cp $(srcdir)/AUTHORS $(BINPKGNAME)/AUTHORS.txt
	cp $(srcdir)/teacher.xpm $(BINPKGNAME)/
	cp $(srcdir)/female_user.xpm $(srcdir)/male_user.xpm $(BINPKGNAME)/
	-cp $(srcdir)/misc/SDL.dll $(BINPKGNAME)/
	-cp $(srcdir)/misc/lua51.dll $(BINPKGNAME)/
	todos $(BINPKGNAME)/fsdemo-??.avt
	todos $(BINPKGNAME)/*.bat
	todos $(BINPKGNAME)/*.txt
	todos $(BINPKGNAME)/*.lua
	-rm -f $(BINPKGNAME).w32.zip
	zip -9vr $(BINPKGNAME).w32.zip $(BINPKGNAME)/
	rm -r $(BINPKGNAME)/
