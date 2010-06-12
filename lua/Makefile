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

all: lua-akfavatar.so lua-avt-starter

# package
lua-akfavatar.so: lua-akfavatar.lo filechooser.lo avtmsg.lo
	$(CC) $(CFLAGS) -fpic -shared -o $@ lua-akfavatar.lo \
	  filechooser.lo avtmsg.lo -L. -lakfavatar $(LDFLAGS)
# don't use LUA_LDFLAGS!

# starter
lua-avt-starter: lua-avt-starter.o lua-akfavatar.o
	$(CC) -o $@ lua-avt-starter.o lua-akfavatar.o \
	     -L. -lavtaddons -lakfavatar \
	     $(LUA_LDFLAGS) $(LDFLAGS)

lua-avt-starter.o: $(srcdir)/lua/lua-avt-starter.c
	$(CC) $(CFLAGS) $(LUA_CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/lua/lua-avt-starter.c

lua-akfavatar.o: $(srcdir)/lua/lua-akfavatar.c
	$(CC) $(CFLAGS) $(LUA_CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/lua/lua-akfavatar.c

filechooser.lo: $(srcdir)/filechooser.c \
                $(srcdir)/akfavatar.h $(srcdir)/avtaddons.h
	$(CC) $(CFLAGS) -fpic -c -o $@ $(srcdir)/filechooser.c

avtmsg.lo: $(srcdir)/avtmsg.c $(srcdir)/avtaddons.h
	$(CC) $(CFLAGS) -fpic -c -o $@ $(srcdir)/avtmsg.c

lua-akfavatar.lo: $(srcdir)/lua/lua-akfavatar.c
	$(CC) $(CFLAGS) $(LUA_CFLAGS) -I$(srcdir) -fpic -c -o $@ \
	  $(srcdir)/lua/lua-akfavatar.c

install: lua-akfavatar.so lua-avt-starter
	-[ -d $(DESTDIR)$(luacdir) ] || $(INSTALL) -d $(DESTDIR)$(luacdir)
	$(INSTALL) -m 644 lua-akfavatar.so $(DESTDIR)$(luacdir)
	$(INSTALL) -m 755 lua-avt-starter $(DESTDIR)$(bindir)

clean:
	-rm -f lua-akfavatar.o lua-akfavatar.lo lua-akfavatar.wo
	-rm -f akfavatar-lua.o akfavatar-lua.wo lua-avt-starter.wo
	-rm -f luac.out
	-rm -f luaavtstarter.rc
	-rm -f *~

distclean: clean
	-rm -f akfavatar-lua
	-rm -f lua-akfavatar.so
	-rm -f lua-avt-starter
	-rm -f akfavatar-lua.exe lua-avt-starter.exe lua-akfavatar.dll
