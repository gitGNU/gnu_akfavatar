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

# for make dist-bin
lua_dll = $(srcdir)/misc/lua51.dll
sdl_dll = $(srcdir)/misc/SDL.dll

# Ogg Vorbis files with how many channels should be supported?
# output is limited to 2 channels, but this value may be higher
vorbis_max_channels = 8

ROFF = groff

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

all: akfavatar.dll libakfavatar.a libavtaddons.a \
     lua-akfavatar.exe lua-akfavatar.dll base64.dll akfavatar-vorbis.dll \
     akfavatar-graphic.dll iconv.exe about

nolua: akfavatar.dll libakfavatar.a libavtaddons.a iconv.exe

avtsayinfo.rc: $(srcdir)/mingw/avtsayinfo.in config-mingw
	sed -e "s/@VERSION@/$(VERSION)/; s/@SUBVERSION@/$(SUBVERSION)/" \
	  -e "s/@REVISION@/$(REVISION)/; s/@VERSIONSTR@/$(VERSIONSTR)/" \
	  $(srcdir)/mingw/avtsayinfo.in > $@

libinfo.rc: $(srcdir)/mingw/libinfo.in config-mingw
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
	sox $(srcdir)/alert.flac --comment "" -e mu-law -c 1 $@ rate 16k

alert.c: $(srcdir)/alert.au
	$(AWK) -f $(srcdir)/data2c -v name=avt_alert_data \
	  $(srcdir)/alert.au > $@

avatar-default.wo: $(srcdir)/avatar-default.c $(default_avatar)
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -I$(srcdir) -I. -c -o $@ $(srcdir)/avatar-default.c

filechooser.wo: $(srcdir)/filechooser.c $(srcdir)/avtinternals.h \
                 $(srcdir)/akfavatar.h $(srcdir)/avtaddons.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/filechooser.c

colorchooser.wo: $(srcdir)/colorchooser.c $(srcdir)/akfavatar.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/colorchooser.c

rgb.h: $(srcdir)/rgb.txt
	$(AWK) -f $(srcdir)/rgb2c $(srcdir)/rgb.txt > $@

avatar.wo: $(srcdir)/avatar.c $(srcdir)/akfavatar.h $(srcdir)/avtinternals.h \
             version.h rgb.h \
             $(srcdir)/btn.xpm $(srcdir)/btn_yes.xbm $(srcdir)/btn_no.xbm \
             $(srcdir)/btn_up.xbm $(srcdir)/btn_down.xbm \
             $(srcdir)/btn_left.xbm $(srcdir)/btn_right.xbm \
             $(srcdir)/btn_ff.xbm $(srcdir)/btn_fb.xbm \
             $(srcdir)/btn_pause.xbm $(srcdir)/btn_stop.xbm \
             $(srcdir)/btn_help.xbm $(srcdir)/btn_cancel.xbm \
             $(srcdir)/btn_eject.xbm $(srcdir)/btn_circle.xbm \
             $(srcdir)/balloonpointer.xbm $(srcdir)/circle.xbm \
             $(srcdir)/akfavatar.xpm
	$(CC) -c -I$(srcdir) -I. -I$(srcdir)/mingw $(SDL_CFLAGS) $(CFLAGS) \
	    -o $@ $(srcdir)/avatar.c

avatar-audio.wo: $(srcdir)/avatar-audio.c $(srcdir)/akfavatar.h \
                  $(srcdir)/avtinternals.h alert.c
	$(CC) -c $(SDL_CFLAGS) $(CFLAGS) -I$(srcdir) -I. -o $@ $(srcdir)/avatar-audio.c

ucsfont7x14.c: $(srcdir)/7x14.bdf $(srcdir)/bdf2c
	$(AWK) -f $(srcdir)/bdf2c $(srcdir)/7x14.bdf > $@

ucsfont9x18.c: $(srcdir)/9x18.bdf $(srcdir)/bdf2c
	$(AWK) -f $(srcdir)/bdf2c $(srcdir)/9x18.bdf > $@

font.wo: $(srcdir)/font.c ucsfont7x14.c ucsfont9x18.c
	$(CC) -c $(SDL_CFLAGS) $(CFLAGS) -I. -o $@ $(srcdir)/font.c

win_iconv.wo: $(srcdir)/mingw/win_iconv.c
	$(CC) -c $(CFLAGS) -o $@ $(srcdir)/mingw/win_iconv.c

iconv.exe: $(srcdir)/mingw/win_iconv.c
	$(CC) -DMAKE_EXE -o $@ $(CFLAGS) $(srcdir)/mingw/win_iconv.c

iconv: iconv.exe

# static library
libakfavatar.a: avatar.wo avatar-default.wo avatar-audio.wo font.wo \
	  win_iconv.wo
	$(AR) rcu $@ avatar.wo avatar-default.wo avatar-audio.wo font.wo \
	  win_iconv.wo
	$(RANLIB) $@

akfavatar.dll: avatar.wo avatar-default.wo avatar-audio.wo font.wo libinfo.wo \
	  win_iconv.wo
	$(CC) -shared -o $@ avatar.wo avatar-default.wo avatar-audio.wo \
	        font.wo libinfo.wo win_iconv.wo \
	        -Wl,-no-undefined,--enable-runtime-pseudo-reloc \
	        -Wl,--output-def,akfavatar.def \
	        -Wl,--out-implib,libakfavatar.dll.a \
	        $(SDL_LDFLAGS) $(LDFLAGS)

avtwindows.wo: $(srcdir)/mingw/avtwindows.c $(srcdir)/akfavatar.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/mingw/avtwindows.c

askdrive.wo: $(srcdir)/mingw/askdrive.c \
	     $(srcdir)/avtaddons.h $(srcdir)/akfavatar.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/mingw/askdrive.c

# this is just a dummy stub!
avttermstub.wo: $(srcdir)/mingw/avttermstub.c $(srcdir)/avtaddons.h $(srcdir)/akfavatar.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -I$(srcdir) -c -o $@ $(srcdir)/mingw/avttermstub.c

arch.wo: $(srcdir)/arch.c $(srcdir)/avtaddons.h $(srcdir)/akfavatar.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/arch.c

avtccio.wo: $(srcdir)/avtccio.c $(srcdir)/avtaddons.h $(srcdir)/akfavatar.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/avtccio.c

avtcwio.wo: $(srcdir)/avtcwio.c $(srcdir)/avtaddons.h $(srcdir)/akfavatar.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/avtcwio.c

avtreadfile.wo: $(srcdir)/avtreadfile.c $(srcdir)/avtaddons.h $(srcdir)/akfavatar.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -c -o $@ $(srcdir)/avtreadfile.c

libavtaddons.a: filechooser.wo askdrive.wo arch.wo avttermstub.wo \
                avtccio.wo avtcwio.wo avtmsg.wo avtgrmsg.wo avtreadfile.wo \
                colorchooser.wo avtvorbis.wo stb_vorbis.wo
	$(AR) rcu $@ filechooser.wo askdrive.wo arch.wo avttermstub.wo \
	             avtccio.wo avtcwio.wo avtmsg.wo avtgrmsg.wo \
	             avtreadfile.wo colorchooser.wo avtvorbis.wo stb_vorbis.wo
	-$(RANLIB) $@

# from the mingw directory!
avtmsg.wo: $(srcdir)/mingw/avtmsg.c $(srcdir)/avtaddons.h \
	   $(srcdir)/akfavatar.h $(srcdir)/avtinternals.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/mingw/avtmsg.c

avtgrmsg.wo: $(srcdir)/avtgrmsg.c $(srcdir)/akfavatar.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/avtgrmsg.c

avatarsay.wo: $(srcdir)/avatarsay.c $(srcdir)/akfavatar.h \
              $(srcdir)/avtinternals.h
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -I$(srcdir) -I. -c -o $@ $(srcdir)/avatarsay.c

avatarsay: avatarsay.exe

avatarsay-s: avatarsay-s.exe

# uses dynamic lib
avatarsay.exe: avatarsay.wo akfavatar.dll \
	         avtwindows.wo avtsayinfo.wo \
	         arch.wo avttermstub.wo colorchooser.wo filechooser.wo \
	         askdrive.wo avtreadfile.wo avtmsg.wo avtgrmsg.wo avtsayinfo.wo
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -o $@ avatarsay.wo avtwindows.wo \
	  arch.wo avttermstub.wo colorchooser.wo filechooser.wo \
	  askdrive.wo avtreadfile.wo avtmsg.wo avtgrmsg.wo avtsayinfo.wo \
	  -L. -lakfavatar $(SDL_LDFLAGS) $(LDFLAGS)

example: example.exe

example.exe: $(srcdir)/example.c akfavatar.dll icon.wo
	$(CC) $(SDL_CFLAGS) $(CFLAGS) -o $@ $(srcdir)/example.c icon.wo \
			-L. -lakfavatar $(SDL_LDFLAGS) $(LDFLAGS)

lua-akfavatar.exe: lua-akfavatar-prg.wo lua-avt.wo luaavtstarter-rc.wo \
		filechooser.wo askdrive.wo colorchooser.wo avtreadfile.wo \
		avtmsg.wo avtgrmsg.wo
	$(CC) -o $@ lua-akfavatar-prg.wo lua-avt.wo luaavtstarter-rc.wo \
	    filechooser.wo askdrive.wo colorchooser.wo avtreadfile.wo \
	    avtmsg.wo avtgrmsg.wo \
	    -L. -lakfavatar \
	    $(LUA_LDFLAGS) $(SDL_LDFLAGS) $(LDFLAGS)

lua-akfavatar.dll: lua-avt.wo filechooser.wo askdrive.wo colorchooser.wo \
	           avtmsg.wo avtgrmsg.wo
	$(CC) -o $@ -shared lua-avt.wo filechooser.wo askdrive.wo \
	     colorchooser.wo avtmsg.wo avtgrmsg.wo \
	     -L. -lakfavatar $(LUA_LDFLAGS) $(LDFLAGS) -mwindows

base64.dll: lbase64.wo
	$(CC) -o $@ -shared lbase64.wo $(LUA_LDFLAGS) $(LDFLAGS)

lua-avt.wo: $(srcdir)/lua/lua-avt.c $(srcdir)/akfavatar.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(LUA_CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/lua/lua-avt.c

lbase64.wo: $(srcdir)/lua/lbase64.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(LUA_CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/lua/lbase64.c

lua-akfavatar-prg.wo: $(srcdir)/lua/lua-akfavatar-prg.c $(srcdir)/akfavatar.h
	$(CC) $(CFLAGS) $(LUA_CFLAGS) $(SDL_CFLAGS) -I$(srcdir) -c -o $@ \
	 $(srcdir)/lua/lua-akfavatar-prg.c

akfavatar-vorbis.dll: akfavatar-vorbis.wo avtvorbis.wo stb_vorbis.wo
	$(CC) -o $@ -shared akfavatar-vorbis.wo avtvorbis.wo stb_vorbis.wo \
	  -L. -lakfavatar $(LUA_LDFLAGS) $(LDFLAGS)

avtvorbis.wo: $(srcdir)/avtvorbis.c $(srcdir)/avtaddons.h $(srcdir)/akfavatar.h \
	      $(srcdir)/stb_vorbis.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -I$(srcdir) -c -o $@ $(srcdir)/avtvorbis.c

akfavatar-vorbis.wo: $(srcdir)/lua/akfavatar-vorbis.c $(srcdir)/akfavatar.h \
	             $(srcdir)/stb_vorbis.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(LUA_CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/lua/akfavatar-vorbis.c

stb_vorbis.wo: $(srcdir)/stb_vorbis.c
	$(CC) $(CFLAGS) -I$(srcdir) \
	  -DSTB_VORBIS_NO_PUSHDATA_API \
	  -DSTB_VORBIS_MAX_CHANNELS=$(vorbis_max_channels) \
	  -c -o $@ $(srcdir)/stb_vorbis.c

akfavatar-graphic.dll: akfavatar-graphic.wo
	$(CC) -o $@ -shared akfavatar-graphic.wo \
	   -L. -lakfavatar -lm $(LUA_LDFLAGS) $(LDFLAGS)

akfavatar-graphic.wo: $(srcdir)/lua/akfavatar-graphic.c $(srcdir)/akfavatar.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(LUA_CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/lua/akfavatar-graphic.c

lua-akfavatar.rc: $(srcdir)/mingw/lua-akfavatar-rc.in config-mingw
	sed -e "s/@VERSION@/$(VERSION)/; s/@SUBVERSION@/$(SUBVERSION)/" \
	  -e "s/@REVISION@/$(REVISION)/; s/@VERSIONSTR@/$(VERSIONSTR)/" \
	  $(srcdir)/mingw/lua-akfavatar-rc.in > $@

luaavtstarter.rc: $(srcdir)/mingw/luaavtstarter-rc.in config-mingw
	sed -e "s/@VERSION@/$(VERSION)/; s/@SUBVERSION@/$(SUBVERSION)/" \
	  -e "s/@REVISION@/$(REVISION)/; s/@VERSIONSTR@/$(VERSIONSTR)/" \
	  $(srcdir)/mingw/luaavtstarter-rc.in > $@

luaavtstarter-rc.wo: luaavtstarter.rc $(srcdir)/mingw/akfavatar.ico
	$(WINDRES) -I$(srcdir)/mingw/ -o $@ -i luaavtstarter.rc

about: $(srcdir)/lua/documentation/English/lua-akfavatar.about \
       $(srcdir)/lua/documentation/Deutsch/lua-akfavatar.about \
       $(srcdir)/lua/documentation/English/lua-akfavatar-ref.about \
       $(srcdir)/lua/documentation/Deutsch/lua-akfavatar-ref.about

$(srcdir)/lua/documentation/English/lua-akfavatar.about: $(srcdir)/doc/lua-akfavatar.en.man
	-$(ROFF) -c -man -Tutf8 $(srcdir)/doc/lua-akfavatar.en.man > $@

$(srcdir)/lua/documentation/Deutsch/lua-akfavatar.about: $(srcdir)/doc/lua-akfavatar.de.man
	-$(ROFF) -c -man -Tutf8 $(srcdir)/doc/lua-akfavatar.de.man > $@

$(srcdir)/lua/documentation/English/lua-akfavatar-ref.about: \
  $(srcdir)/doc/lua-akfavatar-ref.en.man
	-$(ROFF) -c -man -Tutf8 \
	  $(srcdir)/doc/lua-akfavatar-ref.en.man > $@

$(srcdir)/lua/documentation/Deutsch/lua-akfavatar-ref.about: \
  $(srcdir)/doc/lua-akfavatar-ref.de.man
	-$(ROFF) -c -man -Tutf8 \
	  $(srcdir)/doc/lua-akfavatar-ref.de.man > $@

# the folowing targets are for the maintainer
# don't use them unless you know what you are doing

multiply.exe: $(srcdir)/pascal/multiply.pas
	-fpcwavatar -dRELEASE -o$@ -Fi$(srcdir)/pascal \
	  $(srcdir)/pascal/multiply.pas

multiplizieren.exe: $(srcdir)/pascal/multiply.pas
	-fpcwavatar -dRELEASE -ddeutsch -o$@ -Fi$(srcdir)/pascal \
	  $(srcdir)/pascal/multiply.pas

dist-bin: all $(lua_dll) $(sdl_dll)
	$(MAKE) -f $(srcdir)/doc/Makefile VPATH="$(srcdir)/doc" srcdir="$(srcdir)" pdf
	mkdir $(BINPKGNAME)
	mkdir $(BINPKGNAME)/lua
	mkdir $(BINPKGNAME)/lua/documentation
	mkdir $(BINPKGNAME)/lua/documentation/English
	mkdir $(BINPKGNAME)/lua/documentation/Deutsch
	mkdir $(BINPKGNAME)/lua/akfavatar
	mkdir $(BINPKGNAME)/include
	cp akfavatar.dll $(BINPKGNAME)/
	cp libavtaddons.a $(BINPKGNAME)/
	cp $(srcdir)/akfavatar.h $(srcdir)/pascal/akfavatar.pas $(BINPKGNAME)/include/
	cp $(srcdir)/avtaddons.h $(BINPKGNAME)/include/
	-cp $(srcdir)/akfavatar.h.about $(BINPKGNAME)/include/
	-cp $(srcdir)/avtaddons.h.about $(BINPKGNAME)/include/
	cp $(srcdir)/fsdemo-??.avt $(BINPKGNAME)/
	cp lua-akfavatar.exe $(BINPKGNAME)/
	cp base64.dll $(BINPKGNAME)/
	cp akfavatar-vorbis.dll $(BINPKGNAME)/
	cp akfavatar-graphic.dll $(BINPKGNAME)/
	cp $(srcdir)/lua/multiplication.lua $(BINPKGNAME)/lua/
	cp $(srcdir)/lua/interactive_lua.lua $(BINPKGNAME)/lua/
	cp $(srcdir)/lua/audioplayer.lua $(BINPKGNAME)/lua/
	cp $(srcdir)/lua/function_plotter.lua $(BINPKGNAME)/lua/
	cp $(srcdir)/lua/clock.lua $(BINPKGNAME)/lua/
	cp $(srcdir)/lua/textview.lua $(BINPKGNAME)/lua/
	cp $(srcdir)/lua/hello.lua $(BINPKGNAME)/lua/
	cp $(srcdir)/lua/story1-en.lua $(srcdir)/lua/story1-de.lua $(BINPKGNAME)/lua/
	cp $(srcdir)/lua/fs-quiz-en.lua $(BINPKGNAME)/lua/
	cp $(srcdir)/lua/fs-quiz-de.lua $(BINPKGNAME)/lua/
	cp $(srcdir)/lua/base64-encode.lua $(BINPKGNAME)/lua/
	cp $(srcdir)/lua/characters_overview.lua $(BINPKGNAME)/lua/
	cp $(srcdir)/lua/characters_tables.lua $(BINPKGNAME)/lua/
	cp $(srcdir)/lua/colors_overview.lua $(BINPKGNAME)/lua/
	cp $(srcdir)/lua/documentation/English/base64.about $(BINPKGNAME)/lua/documentation/English/
	cp $(srcdir)/lua/documentation/English/akfavatar-graphic.about $(BINPKGNAME)/lua/documentation/English/
	cp $(srcdir)/lua/documentation/English/interactive_lua.about $(BINPKGNAME)/lua/documentation/English/
	cp $(srcdir)/lua/documentation/English/audioplayer.about $(BINPKGNAME)/lua/documentation/English/
	cp $(srcdir)/lua/documentation/English/function_plotter.about $(BINPKGNAME)/lua/documentation/English/
	cp $(srcdir)/lua/documentation/English/base64-encode.about $(BINPKGNAME)/lua/documentation/English/
	cp $(srcdir)/lua/documentation/English/utf8.about $(BINPKGNAME)/lua/documentation/English/
	cp $(srcdir)/lua/documentation/English/lang.about $(BINPKGNAME)/lua/documentation/English/
	-cp $(srcdir)/lua/documentation/English/lua-akfavatar.about $(BINPKGNAME)/lua/documentation/English/
	-cp $(srcdir)/lua/documentation/English/lua-akfavatar-ref.about $(BINPKGNAME)/lua/documentation/English/
	cp $(srcdir)/lua/documentation/Deutsch/base64.about $(BINPKGNAME)/lua/documentation/Deutsch/
	cp $(srcdir)/lua/documentation/Deutsch/akfavatar-graphic.about $(BINPKGNAME)/lua/documentation/Deutsch/
	cp $(srcdir)/lua/documentation/Deutsch/interactive_lua.about $(BINPKGNAME)/lua/documentation/Deutsch/
	cp $(srcdir)/lua/documentation/Deutsch/audioplayer.about $(BINPKGNAME)/lua/documentation/Deutsch/
	cp $(srcdir)/lua/documentation/Deutsch/function_plotter.about $(BINPKGNAME)/lua/documentation/Deutsch/
	cp $(srcdir)/lua/documentation/Deutsch/base64-encode.about $(BINPKGNAME)/lua/documentation/Deutsch/
	cp $(srcdir)/lua/documentation/Deutsch/utf8.about $(BINPKGNAME)/lua/documentation/Deutsch/
	cp $(srcdir)/lua/documentation/Deutsch/lang.about $(BINPKGNAME)/lua/documentation/Deutsch/
	-cp $(srcdir)/lua/documentation/Deutsch/lua-akfavatar.about $(BINPKGNAME)/lua/documentation/Deutsch/
	-cp $(srcdir)/lua/documentation/Deutsch/lua-akfavatar-ref.about $(BINPKGNAME)/lua/documentation/Deutsch/
	cp lua-akfavatar.en.pdf $(BINPKGNAME)/
	cp lua-akfavatar.de.pdf $(BINPKGNAME)/
	cp lua-akfavatar-ref.en.pdf $(BINPKGNAME)/
	cp lua-akfavatar-ref.de.pdf $(BINPKGNAME)/
	cp $(srcdir)/lua/akfavatar/dir.??.about $(BINPKGNAME)/lua/akfavatar/
	cp $(srcdir)/lua/akfavatar/utf8.lua $(BINPKGNAME)/lua/akfavatar/
	cp $(srcdir)/lua/akfavatar/lang.lua $(BINPKGNAME)/lua/akfavatar/
	cp $(srcdir)/lua/akfavatar/person.lua $(BINPKGNAME)/lua/akfavatar/
	cp $(srcdir)/lua/akfavatar/female_user.lua $(BINPKGNAME)/lua/akfavatar/
	cp $(srcdir)/lua/akfavatar/male_user.lua $(BINPKGNAME)/lua/akfavatar/
	cp $(srcdir)/lua/akfavatar/info_icon.lua $(BINPKGNAME)/lua/akfavatar/
	cp $(srcdir)/lua/akfavatar/audio1.lua $(BINPKGNAME)/lua/akfavatar/
	cp $(srcdir)/lua/akfavatar/gnu_head.lua $(BINPKGNAME)/lua/akfavatar/
	cp $(srcdir)/lua/akfavatar/teacher.lua $(BINPKGNAME)/lua/akfavatar/
	cp $(srcdir)/lua/akfavatar/akfoerster.lua $(BINPKGNAME)/lua/akfavatar/
	cp $(srcdir)/lua/akfavatar/positive.lua $(BINPKGNAME)/lua/akfavatar/
	cp $(srcdir)/lua/akfavatar/negative.lua $(BINPKGNAME)/lua/akfavatar/
	cp $(srcdir)/lua/akfavatar/neutral.lua $(BINPKGNAME)/lua/akfavatar/
	cp $(srcdir)/lua/akfavatar/questionary.lua $(BINPKGNAME)/lua/akfavatar/
	cp $(srcdir)/lua/akfavatar/Fragebogen.lua $(BINPKGNAME)/lua/akfavatar/
	cp $(srcdir)/lua/akfavatar/ar.lua $(BINPKGNAME)/lua/akfavatar/
	cp $(srcdir)/lua/akfavatar/avtdemo.lua $(BINPKGNAME)/lua/akfavatar/
	cp $(srcdir)/gnu-head.xpm $(srcdir)/gnu-head.xbm $(BINPKGNAME)/
	cp $(srcdir)/AUTHORS $(BINPKGNAME)/AUTHORS.txt
	cp $(srcdir)/COPYING $(BINPKGNAME)/COPYING.txt
	cp $(srcdir)/COPYING.LESSER $(BINPKGNAME)/COPYING-LESSER.txt
	cp $(srcdir)/NEWS $(BINPKGNAME)/NEWS.txt
	cp $(srcdir)/teacher.xpm $(srcdir)/akfoerster.xpm $(BINPKGNAME)/
	cp $(srcdir)/female_user.xpm $(srcdir)/male_user.xpm $(BINPKGNAME)/
	cp $(srcdir)/audio1.xpm $(BINPKGNAME)/
	cp $(sdl_dll) $(BINPKGNAME)/
	cp $(srcdir)/README-SDL $(BINPKGNAME)/README-SDL.txt
	cp $(lua_dll) $(BINPKGNAME)/
	cp $(srcdir)/README-Lua $(BINPKGNAME)/README-Lua.txt
	$(STRIP) $(BINPKGNAME)/*.exe
	$(STRIP) $(BINPKGNAME)/*.dll
	todos $(BINPKGNAME)/fsdemo-??.avt
	todos $(BINPKGNAME)/*.bat
	todos $(BINPKGNAME)/*.txt
	todos $(BINPKGNAME)/lua/*.lua
	todos $(BINPKGNAME)/lua/akfavatar/*.lua
	todos $(BINPKGNAME)/lua/akfavatar/*.txt
	todos $(BINPKGNAME)/include/*
	-rm -f $(BINPKGNAME).w32.zip
	zip -9vr $(BINPKGNAME).w32.zip $(BINPKGNAME)/
	rm -r $(BINPKGNAME)/
