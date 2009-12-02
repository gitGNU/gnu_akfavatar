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

all: lua-avt.so akfavatar-lua

# package
lua-avt.so: lua-avt.lo
	$(CC) $(CFLAGS) -fpic -shared -o $@ lua-avt.lo \
	  -L. -lavtaddons -lakfavatar $(LDFLAGS)
# don't use LUA_LDFLAGS!

# standalone program
akfavatar-lua: akfavatar-lua.o lua-avt.o
	$(CC) -o $@ akfavatar-lua.o lua-avt.o \
	     -L. -lavtaddons -lakfavatar \
	     $(LUA_LDFLAGS) $(LDFLAGS)

lua-avt.o: $(srcdir)/lua/lua-avt.c
	$(CC) $(CFLAGS) $(LUA_CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/lua/lua-avt.c

lua-avt.lo: $(srcdir)/lua/lua-avt.c
	$(CC) $(CFLAGS) $(LUA_CFLAGS) -I$(srcdir) -fpic -c -o $@ \
	  $(srcdir)/lua/lua-avt.c

akfavatar-lua.o: $(srcdir)/lua/akfavatar-lua.c
	$(CC) $(CFLAGS) $(LUA_CFLAGS) -I$(srcdir) -c -o $@ \
	  $(srcdir)/lua/akfavatar-lua.c

install: lua-avt.so akfavatar-lua
	-[ -d $(DESTDIR)$(luacdir) ] || $(INSTALL) -d $(DESTDIR)$(luacdir)
	$(INSTALL) -m 644 lua-avt.so $(DESTDIR)$(luacdir)
	$(INSTALL) -m 755 akfavatar-lua $(DESTDIR)$(bindir)

clean:
	-rm -f lua-avt.o lua-avt.lo lua-avt.wo
	-rm -f akfavatar-lua.o akfavatar-lua.wo
	-rm -f luac.out
	-rm -f *~

distclean: clean
	-rm -f akfavatar-lua
	-rm -f lua-avt.so
	-rm -f akfavatar-lua.exe lua-avt.dll
