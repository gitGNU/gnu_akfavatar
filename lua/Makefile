CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = #-DLUA_USE_APICHECK

LUA_CFLAGS  := $(shell pkg-config lua5.1 --cflags)
LUA_LDFLAGS := $(shell pkg-config lua5.1 --libs)

srcdir = ..

prefix = /usr/local
luacdir = $(prefix)/lib/lua/5.1/
bindir = $(prefix)/bin

INSTALL = install

all: lua-akfavatar

# package -- doesn't really work as such, so don't use!
lua-akfavatar.so: lua-avt.lo filechooser.lo colorchooser.lo avtmsg.lo
	$(CC) $(CFLAGS) -fpic -shared -o $@ lua-avt.lo \
	  filechooser.lo colorchooser.lo avtmsg.lo -L. -lakfavatar $(LDFLAGS)
# don't use LUA_LDFLAGS!

# starter
lua-akfavatar: lua-akfavatar-prg.o lua-avt.o libavtaddons.a
	$(CC) -o $@ lua-akfavatar-prg.o lua-avt.o \
	     -L. -lavtaddons -lakfavatar \
	     $(LUA_LDFLAGS) $(LDFLAGS)

lua-akfavatar-prg.o: $(srcdir)/lua/lua-akfavatar-prg.c
	$(CC) $(CFLAGS) $(LUA_CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/lua/lua-akfavatar-prg.c

lua-avt.o: $(srcdir)/lua/lua-avt.c
	$(CC) $(CFLAGS) $(LUA_CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/lua/lua-avt.c

filechooser.lo: $(srcdir)/filechooser.c \
                $(srcdir)/akfavatar.h $(srcdir)/avtaddons.h
	$(CC) $(CFLAGS) -fpic -c -o $@ $(srcdir)/filechooser.c

colorchooser.lo: $(srcdir)/colorchooser.c \
                $(srcdir)/akfavatar.h $(srcdir)/avtaddons.h
	$(CC) $(CFLAGS) -fpic -c -o $@ $(srcdir)/colorchooser.c

avtmsg.lo: $(srcdir)/avtmsg.c $(srcdir)/avtaddons.h
	$(CC) $(CFLAGS) -fpic -c -o $@ $(srcdir)/avtmsg.c

lua-avt.lo: $(srcdir)/lua/lua-avt.c
	$(CC) $(CFLAGS) $(LUA_CFLAGS) -I$(srcdir) -fpic -c -o $@ \
	    $(srcdir)/lua/lua-avt.c

install: lua-akfavatar
#	-[ -d $(DESTDIR)$(luacdir) ] || $(INSTALL) -d $(DESTDIR)$(luacdir)
#	$(INSTALL) -m 644 lua-akfavatar.so $(DESTDIR)$(luacdir)
	$(INSTALL) -m 755 lua-akfavatar $(DESTDIR)$(bindir)

clean:
	-rm -f lua-akfavatar.o lua-avt.lo lua-avt.wo
	-rm -f lua-akfavatar-prg.o lua-akfavatar-prg.wo
	-rm -f luac.out
	-rm -f luaavtstarter.rc
	-rm -f *~

distclean: clean
	-rm -f lua-akfavatar
	-rm -f lua-akfavatar.so
	-rm -f lua-akfavatar.exe lua-akfavatar.dll
